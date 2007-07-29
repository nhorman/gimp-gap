/* gap_morph_exec.c
 * 2004.02.12 hof (Wolfgang Hofer)
 * layer morphing worker procedures
 *
 */
/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* revision history:
 * gimp    2.0.2b; 2004/07/28  hof: added p_mix_layers
 * gimp    2.0.2b; 2004/07/17  hof: p_pixel_warp_core: changed workpoint selection (both FAST and Quality strategy)
 * gimp    2.0.2a; 2004/04/07  hof: created
 */

#include "config.h"

/* SYTEM (UNIX) includes */
#include "math.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib/gstdio.h>

/* GIMP includes */
#include "gtk/gtk.h"
#include "libgimp/gimp.h"

/* GAP includes */
#include "gap-intl.h"
#include "gap_image.h"
#include "gap_morph_main.h"
#include "gap_morph_exec.h"
#include "gap_layer_copy.h"


/* GAP_MORPH_USE_SIMPLE_PIXEL_WARP is used for development test only
 * (gives bad quality results, you should NOT enable this option)
 */
#undef  GAP_MORPH_USE_SIMPLE_PIXEL_WARP
//#define  GAP_MORPH_USE_SIMPLE_PIXEL_WARP

#define GAP_MORPH_NEAR_SQR_RADIUS  64
#define GAP_MORPH_NEAR_RADIUS  4
#define GAP_MORPH_FAR_RADIUS  40
#define GAP_MORPH_TOL_FACT_NEAR  3.0
#define GAP_MORPH_TOL_FACT_FAR   1.5

#define GAP_MORPH_8_SEKTORS 8
#define GAP_MORPH_TOL_FAKTOR 2.25
#define GAP_MORPH_MIN_TOL_FAKTOR 4.5
#define GAP_MORPH_XY_REL_FAKTOR 5

#define MIX_VALUE(factor, a, b) ((a * (1.0 - factor)) +  (b * factor))

typedef struct GapMorphExeLayerstack
{
  gint32    *src_layers;
  gint       src_nlayers;
  gint32    *dst_layers;
  gint       dst_nlayers;
  gint       src1_idx;
  gint       src2_idx;
  gint       dst1_idx;
  gint       dst2_idx;

  gint32 available_wp_sets;
  GapMorphWarpCoreAPI **tab_wp_sets;
  
} GapMorphExeLayerstack;

extern int gap_debug;

static inline gdouble     p_get_tolerance(gdouble dist);
static void               p_clip_gimp_pixel_fetcher_get_pixel(GimpPixelFetcher *pft
                                   ,GimpDrawable     *drawable
				   ,gint xi
				   ,gint yi
				   ,guchar *pixel
				   );
static void               p_bilinear_get_pixel(GimpPixelFetcher *pft
				         , GimpDrawable     *drawable
                                         , gdouble needx
                                         , gdouble needy
                                         , guchar *dest
					 , gint dst_bpp
                                         );
static gdouble            p_linear_advance(gdouble total_steps
                                     ,gdouble current_step
                                     ,gdouble val_from
                                     ,gdouble val_to
                                     );
static inline gint        p_calc_sector(GapMorphWorkPoint *wp, gdouble dx, gdouble dy);
static inline gint        p_calc_angle_core(GapMorphWorkPoint *wp, gdouble dx, gdouble dy, gint num_sectors);
static gint               p_calc_angle(GapMorphWorkPoint *wp, gint32 in_x, gint32 in_y);
static void               p_pixel_warp_core(GapMorphWarpCoreAPI *wcap
        			      , gint32        in_x
        			      , gint32        in_y
        			      , gdouble *out_pick_x
				      , gdouble *out_pick_y
        			      );
static void               p_pixel_warp_pick(GapMorphWarpCoreAPI *wcap
                                      , gint32        in_x
                                      , gint32        in_y
				      , gint          set_idx
				      , gdouble      *out_pick_x
				      , gdouble      *out_pick_y
                                      );
static void               p_pixel_warp_multipick(GapMorphWarpCoreAPI *wcap_1
                                      , GapMorphWarpCoreAPI *wcap_2
		                      , gdouble       wp_mix_factor
                                      , gint32        in_x
                                      , gint32        in_y
				      , gdouble      *out_pick_x
				      , gdouble      *out_pick_y
                                      );
GapMorphWorkPoint *       p_calculate_work_point_movement(gdouble total_steps
                               ,gdouble current_step
			       ,GapMorphWorkPoint *master_list
			       ,gint32 src_layer_id
			       ,gint32 dst_layer_id
                               ,gint32 curr_width
                               ,gint32 curr_height
                               ,gboolean forward_move
                               );

static gint32 p_get_tween_steps_and_layerstacks(GapMorphGlobalParams *mgpp, GapMorphExeLayerstack *mlayers);

#define GAP_MORPH_WORKPOINT_FILE_HEADER "GAP-MORPH workpoint file"


typedef struct GapMorphExePickCache
{
  gboolean valid;
  gdouble  xx;
  gdouble  yy;
  gdouble  pick_x;
  gdouble  pick_y;
  
} GapMorphExePickCache;

#define GAP_MORPH_PCK_CACHE_SIZE 32
static  GapMorphExePickCache gloabl_pick_cache[GAP_MORPH_PCK_CACHE_SIZE][2];
static  gint gloabl_pick_cache_idx[2];


/* ---------------------------------
 * p_get_tolerance
 * ---------------------------------
 */
static inline gdouble
p_get_tolerance(gdouble dist)
{
  gdouble factor;
  
  if(dist <= GAP_MORPH_NEAR_RADIUS)
  {
    return(GAP_MORPH_TOL_FACT_NEAR);
  }
  if(dist >= GAP_MORPH_FAR_RADIUS)
  {
    return(GAP_MORPH_TOL_FACT_FAR);
  }
  
  factor = (dist - GAP_MORPH_NEAR_RADIUS) / GAP_MORPH_FAR_RADIUS;
  return(  (GAP_MORPH_TOL_FACT_NEAR * (1 -factor)) 
         + (GAP_MORPH_TOL_FACT_FAR *  factor)
	);
}  /* end  p_get_tolerance */


/* ---------------------------------
 * gap_moprh_exec_save_workpointfile
 * ---------------------------------
 */
gboolean
gap_moprh_exec_save_workpointfile(const char *filename
                                 , GapMorphGUIParams *mgup
                                 )
{
  FILE *l_fp;
  GapMorphWorkPoint *wp;
  gint32 src_layer_width;
  gint32 src_layer_height;
  gint32 dst_layer_width;
  gint32 dst_layer_height;
  guchar l_red, l_green, l_blue;

  if(filename == NULL) return -1;

  l_fp = g_fopen(filename, "w+");
  if(l_fp != NULL)
  {
    src_layer_width  = gimp_drawable_width(mgup->mgpp->osrc_layer_id);
    src_layer_height = gimp_drawable_height(mgup->mgpp->osrc_layer_id);
    dst_layer_width  = gimp_drawable_width(mgup->mgpp->fdst_layer_id);
    dst_layer_height = gimp_drawable_height(mgup->mgpp->fdst_layer_id);


    fprintf(l_fp, "%s\n", GAP_MORPH_WORKPOINT_FILE_HEADER);
    fprintf(l_fp, "LAYER-SIZES: %d  %d  %d %d  # src_width src_height dst_width dst_height\n"
                  ,(int)src_layer_width
                  ,(int)src_layer_height
                  ,(int)dst_layer_width
                  ,(int)dst_layer_height
                  );
    fprintf(l_fp, "TWEEN-STEPS: %d\n"
                  ,(int)mgup->mgpp->tween_steps
                  );
    fprintf(l_fp, "AFFECT-RADIUS:");
    gap_lib_fprintf_gdouble(l_fp, mgup->mgpp->affect_radius, 4, 2, " ");
    fprintf(l_fp, " # pixels\n");

    fprintf(l_fp, "INTENSITY:");
    if(mgup->mgpp->use_gravity)
    {
      gap_lib_fprintf_gdouble(l_fp, mgup->mgpp->gravity_intensity, 2, 3, " ");
      fprintf(l_fp, "\n");
    }
    else
    {
      fprintf(l_fp, "0 # OFF\n");
    }

    if(mgup->mgpp->use_quality_wp_selection)
    {
      fprintf(l_fp, "QUALITY-WP-SELECT: 1 # TRUE\n");
    }
    else
    {
      fprintf(l_fp, "QUALITY-WP-SELECT: 0 # FALSE (faster but lower quality)\n");
    }

    gimp_rgb_get_uchar (&mgup->pointcolor, &l_red, &l_green, &l_blue);
    fprintf(l_fp, "POINTCOLOR: %d %d %d\n"
                  ,(int)l_red
                  ,(int)l_green
                  ,(int)l_blue
                  );
    gimp_rgb_get_uchar (&mgup->curr_pointcolor, &l_red, &l_green, &l_blue);
    fprintf(l_fp, "CURR-POINTCOLOR: %d %d %d\n"
                  ,(int)l_red
                  ,(int)l_green
                  ,(int)l_blue
                  );


    fprintf(l_fp, "# WP: src_x  src_y  dst_x dst_y\n");

    for(wp = mgup->mgpp->master_wp_list; wp != NULL; wp = (GapMorphWorkPoint *)wp->next)
    {
      fprintf(l_fp, "WP: ");
      gap_lib_fprintf_gdouble(l_fp, wp->osrc_x, 4, 3, " ");
      gap_lib_fprintf_gdouble(l_fp, wp->osrc_y, 4, 3, " ");
      gap_lib_fprintf_gdouble(l_fp, wp->fdst_x, 4, 3, " ");
      gap_lib_fprintf_gdouble(l_fp, wp->fdst_y, 4, 3, " ");

      fprintf(l_fp, "\n");   /* terminate the output line */
    }

    fclose(l_fp);
    return (TRUE);
  }
  return (FALSE);
}  /* end gap_moprh_exec_save_workpointfile */


/* ----------------------------------
 * p_load_workpointfile
 * ----------------------------------
 * return root pointer to the loaded workpoint list on SUCCESS
 * return NULL if load failed.
 */
GapMorphWorkPoint *
p_load_workpointfile(const char *filename
                                 , gint32 src_layer_id
				 , gint32 dst_layer_id
				 , gint32 *tween_steps
				 , gdouble *affect_radius
				 , gdouble *gravity_intensity
				 , gboolean *use_gravity
				 , gboolean *use_quality_wp_selection
                                 )
{
#define POINT_REC_MAX 512
#define MAX_NUMVALUES_PER_LINE 17
  FILE   *l_fp;
  GapMorphWorkPoint *wp;
  GapMorphWorkPoint *wp_list;
  gint32 src_layer_width;
  gint32 src_layer_height;
  gint32 dst_layer_width;
  gint32 dst_layer_height;
  gdouble src_scale_x;
  gdouble src_scale_y;
  gdouble dst_scale_x;
  gdouble dst_scale_y;
  
  
  char    l_buff[POINT_REC_MAX +1 ];
  char   *l_ptr;
  gint    l_cnt;
  gdouble l_farr[MAX_NUMVALUES_PER_LINE];


  src_layer_width  = gimp_drawable_width(src_layer_id);
  src_layer_height = gimp_drawable_height(src_layer_id);
  dst_layer_width  = gimp_drawable_width(dst_layer_id);
  dst_layer_height = gimp_drawable_height(dst_layer_id);
  src_scale_x = 1.0;
  src_scale_y = 1.0;
  dst_scale_x = 1.0;
  dst_scale_y = 1.0;
  wp_list = NULL;
  wp = NULL;
  if(filename == NULL) return(NULL);

  *use_gravity = FALSE;
  *use_quality_wp_selection = FALSE;
  l_fp = g_fopen(filename, "r");
  if(l_fp != NULL)
  {
    gint l_len;
    
    fgets (l_buff, POINT_REC_MAX, l_fp);
    l_len = strlen(GAP_MORPH_WORKPOINT_FILE_HEADER);
    if(strncmp(l_buff, GAP_MORPH_WORKPOINT_FILE_HEADER, l_len) != 0)
    {
      printf("** error file: %s is no workpointfile (header is missing)\n", filename);
      fclose(l_fp);
      return (NULL);
    }
    
    while (NULL != fgets (l_buff, POINT_REC_MAX, l_fp))
    {
       /* skip leading blanks */
       l_ptr = l_buff;
       while(*l_ptr == ' ') { l_ptr++; }

       /* check if line empty or comment only (starts with '#') */
       if((*l_ptr != '#') && (*l_ptr != '\n') && (*l_ptr != '\0'))
       {
         l_len = strlen("LAYER-SIZES:");
         if(strncmp(l_ptr, "LAYER-SIZES:", l_len) == 0)
         {
           l_ptr += l_len;
           l_cnt = gap_lib_sscan_flt_numbers(l_ptr, &l_farr[0], MAX_NUMVALUES_PER_LINE);
           if(l_cnt >= 4)
           {
             src_scale_x =  MAX(1,src_layer_width)  / MAX(1,l_farr[0]);
             src_scale_y =  MAX(1,src_layer_height) / MAX(1,l_farr[1]);
             dst_scale_x =  MAX(1,dst_layer_width)  / MAX(1,l_farr[2]);
             dst_scale_y =  MAX(1,dst_layer_height) / MAX(1,l_farr[3]);
           }
           else
           {
             printf("** error file: %s is corrupted (LAYER-SIZES: record requires 4 numbers)\n"
                   , filename);
             fclose(l_fp);
             return (NULL);
           }
         }

         l_len = strlen("TWEEN-STEPS:");
         if(strncmp(l_ptr, "TWEEN-STEPS:", l_len) == 0)
         {
           l_ptr += l_len;
           l_cnt = gap_lib_sscan_flt_numbers(l_ptr, &l_farr[0], MAX_NUMVALUES_PER_LINE);
           if(l_cnt >= 1)
           {
             *tween_steps = MAX(1,l_farr[0]);
           }
           else
           {
             printf("** error file: %s is corrupted (TWEEN-STEPS record requires 1 number)\n"
                   , filename);
             fclose(l_fp);
             return (NULL);
           }
         }

         l_len = strlen("AFFECT-RADIUS:");
         if(strncmp(l_ptr, "AFFECT-RADIUS:", l_len) == 0)
         {
           l_ptr += l_len;
           l_cnt = gap_lib_sscan_flt_numbers(l_ptr, &l_farr[0], MAX_NUMVALUES_PER_LINE);
           if(l_cnt >= 1)
           {
             *affect_radius = MAX(0,l_farr[0]);
           }
           else
           {
             printf("** error file: %s is corrupted (AFFECT-RADIUS record requires 1 number)\n"
                   , filename);
             fclose(l_fp);
             return (NULL);
           }
         }

         l_len = strlen("INTENSITY:");
         if(strncmp(l_ptr, "INTENSITY:", l_len) == 0)
         {
           l_ptr += l_len;
           l_cnt = gap_lib_sscan_flt_numbers(l_ptr, &l_farr[0], MAX_NUMVALUES_PER_LINE);
           if(l_cnt >= 1)
           {
             *gravity_intensity = MAX(0,l_farr[0]);
	     if(*gravity_intensity > 0.0)
	     {
	       *use_gravity = TRUE;
	     }
           }
           else
           {
             printf("** error file: %s is corrupted (INTENSITY record requires 1 number)\n"
                   , filename);
             fclose(l_fp);
             return (NULL);
           }
         }

         l_len = strlen("QUALITY-WP-SELECT:");
         if(strncmp(l_ptr, "QUALITY-WP-SELECT:", l_len) == 0)
         {
           l_ptr += l_len;
           l_cnt = gap_lib_sscan_flt_numbers(l_ptr, &l_farr[0], MAX_NUMVALUES_PER_LINE);
           if(l_cnt >= 1)
           {
	     *use_quality_wp_selection = FALSE;
             if(l_farr[0] != 0)
	     {
	       *use_quality_wp_selection = TRUE;
	     }
           }
           else
           {
             printf("** error file: %s is corrupted (QUALITY-WP-SELECT record requires 1 number)\n"
                   , filename);
             fclose(l_fp);
             return (NULL);
           }
         }
         
         l_len = strlen("WP:");
         if(strncmp(l_ptr, "WP:", l_len) == 0)
         {
           l_ptr += l_len;
           l_cnt = gap_lib_sscan_flt_numbers(l_ptr, &l_farr[0], MAX_NUMVALUES_PER_LINE);
           if(l_cnt >= 4)
           {
             wp = g_new(GapMorphWorkPoint, 1);
             
             /* scale the loaded workpoints 
              * (to fit the current layer)
              */
             wp->osrc_x = l_farr[0] * src_scale_x;
             wp->osrc_y = l_farr[1] * src_scale_y;
             wp->fdst_x = l_farr[2] * dst_scale_x;
             wp->fdst_y = l_farr[3] * dst_scale_y;

             wp->src_x = wp->osrc_x;
             wp->src_y = wp->osrc_y;
             wp->dst_x = wp->fdst_x;
             wp->dst_y = wp->fdst_y;
             
             wp->next = wp_list;
             wp_list = wp;
           }
           else
           {
             printf("** error file: %s is corrupted (WP: record requires 4 numbers)\n"
                   , filename);
             fclose(l_fp);
             return (NULL);
           }
         }
         
       }
    }

    fclose(l_fp);
  }
  return (wp_list);
}  /* end p_load_workpointfile */


/* ----------------------------------
 * gap_moprh_exec_load_workpointfile
 * ----------------------------------
 * return root pointer to the loaded workpoint list on SUCCESS
 * return NULL if load failed.
 */
GapMorphWorkPoint *
gap_moprh_exec_load_workpointfile(const char *filename
                                 , GapMorphGUIParams *mgup
                                 )
{
  GapMorphWorkPoint *wp_list;
  
  wp_list = p_load_workpointfile(filename
                                ,mgup->mgpp->osrc_layer_id
				,mgup->mgpp->fdst_layer_id
				,&mgup->mgpp->tween_steps
				,&mgup->mgpp->affect_radius
				,&mgup->mgpp->gravity_intensity
				,&mgup->mgpp->use_gravity
				,&mgup->mgpp->use_quality_wp_selection
				);
  return(wp_list);
}  /* end gap_moprh_exec_load_workpointfile */

/* ---------------------------------
 * p_load_workpoint_set
 * ---------------------------------
 * load workpoint set from file.
 * in case file not exists or other loading troubles
 * use the master workpoint list as the default workpoint set.
 */
static GapMorphWarpCoreAPI*
p_load_workpoint_set(const char *filename
		    ,GapMorphGlobalParams  *mgpp
		    ,GapMorphExeLayerstack *mlayers
		    )
{
  GapMorphWarpCoreAPI *wps;
  gint32              tween_steps;
  
  wps = g_new(GapMorphWarpCoreAPI ,1);
  wps->wp_list = p_load_workpointfile(filename
                                ,mgpp->osrc_layer_id
				,mgpp->fdst_layer_id
				,&tween_steps
				,&wps->affect_radius
				,&wps->gravity_intensity
				,&wps->use_gravity
				,&wps->use_quality_wp_selection
				);
  if(wps->wp_list == NULL)
  {
    wps->wp_list = mgpp->master_wp_list;
    wps->affect_radius = mgpp->affect_radius;
    wps->gravity_intensity = mgpp->gravity_intensity;
    wps->use_gravity = mgpp->use_gravity;
    wps->use_quality_wp_selection = mgpp->use_quality_wp_selection;
  }
  return(wps);
}   /* end p_load_workpoint_set */



/* ---------------------------------
 * p_build_wp_set_table
 * ---------------------------------
 */
static void
p_build_wp_set_table(GapMorphGlobalParams  *mgpp
                    ,GapMorphExeLayerstack *mlayers
		    )
{
  long lower_num;
  long upper_num;
  gchar *lower_basename;
  gchar *upper_basename;
  gchar *lower_ext;
  gchar *upper_ext;
  
  lower_basename = gap_lib_alloc_basename(mgpp->workpoint_file_lower
                                         ,&lower_num);
  upper_basename = gap_lib_alloc_basename(mgpp->workpoint_file_upper
                                         ,&upper_num);
  lower_ext = gap_lib_alloc_extension(mgpp->workpoint_file_lower);
  upper_ext = gap_lib_alloc_extension(mgpp->workpoint_file_upper);
  
  if((lower_num != upper_num)
  && (strcmp(lower_basename, upper_basename) == 0)
  && (strcmp(lower_ext, upper_ext) == 0))
  {
    gchar *wp_filename;
    gint   ii;
    gint   wp_idx;
    
    
    mlayers->available_wp_sets = 0;
    for(ii=MIN(lower_num, upper_num); ii <= MAX(upper_num, lower_num); ii++)
    {
      wp_filename = g_strdup_printf("%s%02d%s"
                                   ,lower_basename
				   ,(int)ii
				   ,lower_ext
				   );
      if(g_file_test(wp_filename, G_FILE_TEST_EXISTS))
      {
        mlayers->available_wp_sets++;
      }
      g_free(wp_filename);
    }

    mlayers->tab_wp_sets = g_new(GapMorphWarpCoreAPI*, mlayers->available_wp_sets);
    wp_idx = 0;
    for(ii=MIN(lower_num, upper_num); ii <= MAX(upper_num, lower_num); ii++)
    {
      wp_filename = g_strdup_printf("%s%02d%s"
                                   ,lower_basename
				   ,(int)ii
				   ,lower_ext
				   );
      if(g_file_test(wp_filename, G_FILE_TEST_EXISTS))
      {
        if(wp_idx < mlayers->available_wp_sets)
	{
	  if(gap_debug)
	  {
	    printf("p_build_wp_set_table: LOADING workpoint file: %s\n", wp_filename);
	  }
	  mlayers->tab_wp_sets[wp_idx] = p_load_workpoint_set(wp_filename, mgpp, mlayers);
	}
	wp_idx++;
      }
      g_free(wp_filename);
    }
    
  }
  else
  {
    /* create 2 workpoint sets from upper and lower file */
    mlayers->available_wp_sets = 2;
    mlayers->tab_wp_sets = g_new(GapMorphWarpCoreAPI*, mlayers->available_wp_sets);
    mlayers->tab_wp_sets[0] = p_load_workpoint_set(mgpp->workpoint_file_lower, mgpp, mlayers);
    mlayers->tab_wp_sets[1] = p_load_workpoint_set(mgpp->workpoint_file_upper, mgpp, mlayers);
  }
  
  g_free(lower_basename);
  g_free(upper_basename);
  g_free(lower_ext);
  g_free(upper_ext);
  
}   /* end p_build_wp_set_table */



/* ---------------------------------
 * gap_morph_exec_find_dst_drawable
 * ---------------------------------
 * search for a 2.nd layer in image_id
 * that has same size as the input layer_id
 * (prefere layer on top of the input layer_id)
 */
gint32  
gap_morph_exec_find_dst_drawable(gint32 image_id, gint32 layer_id)
{
  gint l_width;
  gint l_height;
  gint    l_ii;
  gint    l_nlayers;
  gint32 *l_layers_list;
  gint32  l_ret_layer_id;
 
  l_ret_layer_id = -1;
  l_width = gimp_drawable_width(layer_id);
  l_height = gimp_drawable_height(layer_id);

  l_layers_list = gimp_image_get_layers(image_id, &l_nlayers);
  if(l_layers_list == NULL)
  {
     return (-1);
  }
  
  for(l_ii=0; l_ii < l_nlayers; l_ii++)
  {
    if(l_layers_list[l_ii] != layer_id)
    {
      if((gimp_drawable_width(l_layers_list[l_ii]) == l_width)
      && (gimp_drawable_height(l_layers_list[l_ii]) == l_height))
      {
         l_ret_layer_id = l_layers_list[l_ii];
         break;
      }
      if(l_ret_layer_id == -1)
      {
        l_ret_layer_id = l_layers_list[l_ii];
      }
    }
  }
  g_free (l_layers_list);
  return(l_ret_layer_id);
}  /* end gap_morph_exec_find_dst_drawable */


/* -----------------------------------
 * p_clip_gimp_pixel_fetcher_get_pixel
 * -----------------------------------
 */
static void
p_clip_gimp_pixel_fetcher_get_pixel(GimpPixelFetcher *pft
                                   ,GimpDrawable     *drawable
				   ,gint xi
				   ,gint yi
				   ,guchar *pixel
				   )
{
  if((xi >= 0)
  && (yi >= 0)
  && (xi < (gint)drawable->width)
  && (yi < (gint)drawable->height))
  {
    gimp_pixel_fetcher_get_pixel (pft, xi, yi, pixel);
  }
  else
  {
    /* deliver full transparent black pixel when out of bounds */
    pixel[0] = 0;
    pixel[1] = 0;
    pixel[2] = 0;
    pixel[3] = 0;
  }
}  /* end p_clip_gimp_pixel_fetcher_get_pixel */

/* ------------------------
 * p_bilinear_get_pixel
 * ------------------------
 * pixel picking at gdouble needx/needy koords
 * picks 4 pixels at nearest integer koordinates
 * and calculate the value by bilinear interpolation.
 */
static void
p_bilinear_get_pixel(GimpPixelFetcher *pft
                    ,GimpDrawable     *drawable
                    , gdouble needx
		    , gdouble needy
		    , guchar *dest
		    , gint dst_bpp
		    )
{
  guchar  pixel[4][4];
  guchar  values[4];
  guchar  val;

  gint    xi, yi;
  guint k;

  if (needx >= 0.0)
    xi = (int) needx;
  else
    xi = -((int) -needx + 1);

  if (needy >= 0.0)
    yi = (int) needy;
  else
    yi = -((int) -needy + 1);

  p_clip_gimp_pixel_fetcher_get_pixel (pft, drawable, xi,     yi,     pixel[0]);
  p_clip_gimp_pixel_fetcher_get_pixel (pft, drawable, xi + 1, yi,     pixel[1]);
  p_clip_gimp_pixel_fetcher_get_pixel (pft, drawable, xi,     yi + 1, pixel[2]);
  p_clip_gimp_pixel_fetcher_get_pixel (pft, drawable, xi + 1, yi + 1, pixel[3]);

  for (k = 0; k < drawable->bpp; k++)
  {
    values[0] = pixel[0][k];
    values[1] = pixel[1][k];
    values[2] = pixel[2][k];
    values[3] = pixel[3][k];
    val = gimp_bilinear_8 (needx, needy, values);

    *dest++ = val;
  }

}  /* end p_bilinear_get_pixel */


/* ---------------------------------
 * p_linear_advance
 * ---------------------------------
 */
static gdouble
p_linear_advance(gdouble total_steps
                ,gdouble current_step
                ,gdouble val_from
                ,gdouble val_to
                )
{
   gdouble     delta;

   if(total_steps < 1) return(val_from);

   delta = ((val_to - val_from) / total_steps) * (total_steps - current_step);
   return (val_to - delta);
}  /* end p_linear_advance */

/* ----------------------------------
 * gap_morph_exec_free_workpoint_list
 * ----------------------------------
 */
void
gap_morph_exec_free_workpoint_list(GapMorphWorkPoint **wp_list)
{
  if(wp_list)
  {
    GapMorphWorkPoint *wp;
    GapMorphWorkPoint *wp_next;

    wp_next = NULL;
    for(wp = *wp_list; wp != NULL; wp = wp_next)
    {
      wp_next = wp->next;
      g_free(wp);
    }
    
    *wp_list = NULL;
  }
}  /* end gap_morph_exec_free_workpoint_list */


/* ---------------------------------
 * p_calc_sector
 * ---------------------------------
 * return sector index (an integer val 0 upto 7)
 * (is much faster than calculating the angle, but restricted to 8 sectors)
 */
static inline gint
p_calc_sector(GapMorphWorkPoint *wp, gdouble dx, gdouble dy)
{
  if(dx >= 0)
  {
    if(dy >= 0)
    {
      if(dx >= dy)
      {
        return(3);        /*  91 - 135 degree */
      }
      return(2);          /* 136 - 180 degree */
    }
    if(dx >= abs(dy))
    {
      return(4);          /* 181 - 225 degree */
    }
    return(5);            /* 226 - 270 degree */
  }
  
  if(dy >= 0)
  {
    if(abs(dx) >= dy)
    {
      return(0);          /*   0 -  45 degree */
    }
    return(1);            /*  46 -  90 degree */
  }

  if(abs(dx) >= abs(dy))
  {
      return(7);          /* 316 - 360 degree */
  }
  return(6);              /* 271 - 315 degree */
  
}  /* end p_calc_sector */



/* ---------------------------------
 * p_calc_angle_core
 * ---------------------------------
 * calculate the angle and return as sector (rounded to sectorsize)
 */
static inline gint
p_calc_angle_core(GapMorphWorkPoint *wp, gdouble dx, gdouble dy, gint num_sectors)
{
  gdouble angle_rad;
  gdouble sector;

  if(dx == 0)
  {
    if(dy < 0)
    {
      wp->angle_deg  = 270;
    }
    else
    {
      if(dy > 0)
      {
        wp->angle_deg  = 90;
      }
      else
      {
        wp->angle_deg  = 0;
      }
    }
  }
  else
  {
    gdouble adx;
    gdouble ady;

    adx = abs(dx);
    ady = abs(dy);

    angle_rad = atan(ady / adx);

    /* angle_rad = atan(abs(dy) / abs(dx)); */ /* this version crashes sometimes, dont know why ? */
    
     
    wp->angle_deg  = 180.0 * (angle_rad / G_PI);

    if(dx > 0)
    {
      if(dy < 0)
      {
        wp->angle_deg  = 360 - wp->angle_deg;
      }
    }
    else
    {
      if(dy > 0)
      {
        wp->angle_deg  = 180 - wp->angle_deg;
      }
      else
      {
        wp->angle_deg  += 180;
      }
    }
  }
  
  sector = (wp->angle_deg / 360.0) * num_sectors;
  return(CLAMP((gint)sector, 0 , (num_sectors-1)));
  
}   /* end p_calc_angle_core */


/* ---------------------------------
 * p_calc_angle
 * ---------------------------------
 * calculate the angle (rounded to sectorsize) and distance
 * between in_x/in_y and the workpoint
 */
static gint
p_calc_angle(GapMorphWorkPoint *wp, gint32 in_x, gint32 in_y)
{
  gdouble dx;
  gdouble dy;
  gdouble adx;
  gdouble ady;
  
  if(wp)
  {
    dx = in_x - wp->dst_x;
    dy = in_y - wp->dst_y;

    wp->sqr_dist = (dx * dx ) + (dy * dy);
    wp->dist = sqrt(wp->sqr_dist);

	adx = abs(dx);
	ady = abs(dy);
        if(adx > ady)
	{
	  wp->xy_relation = (gint)(0.5 + (GAP_MORPH_XY_REL_FAKTOR * (ady / adx)));
	}
	else
	{
	  if(dy != 0)
	  {
	    wp->xy_relation = (gint)(0.5 + (GAP_MORPH_XY_REL_FAKTOR * (adx / ady)));
	  }
	  else
	  {
	    wp->xy_relation = 0;
	  }
	}

    return(p_calc_angle_core(wp, dx, dy, GAP_MORPH_8_SEKTORS));
  }
  return (0);

}   /* end p_calc_angle */


/* ---------------------------------
 * p_pixel_warp_core
 * ---------------------------------
 *  the movement vektor for in_x/in_y is calculated by mixing
 *  the movement vektors of the selected workpoints,
 *  weighted by invers distances of the workpoint to in_x/in_y
 *
 *  there are 2 strategies (FAST / QUALITY) for selecting relevant
 *  workpoints. 
 *  FAST strategy uses just the nearest workpoints in 8 sektors,
 *  QUALITY looks for all workpoints per sektor within a tolerance radius
 *  but discards points with similar angle and greater distance
 *
 *  in case there is no workpoint available
 *       the pixel is picked by simply scaling in_x/in_y to ssrc koords
 *
 */
static void
p_pixel_warp_core(GapMorphWarpCoreAPI *wcap
            , gint32        in_x
            , gint32        in_y
            , gdouble *out_pick_x
	    , gdouble *out_pick_y
            )
{
  register GapMorphWorkPoint *wp;
  GapMorphWorkPoint *wp_sek;
  GapMorphWorkPoint *wp_sektor_tab[GAP_MORPH_8_SEKTORS];
  gdouble     tab_tol[GAP_MORPH_8_SEKTORS];
  
  /* index tables to access neigbour sectors */
  static gint tab_l1_idx[GAP_MORPH_8_SEKTORS] = {7,0,1,2,3,4,5,6};
  static gint tab_l2_idx[GAP_MORPH_8_SEKTORS] = {6,7,0,1,2,3,4,5};

  static gint tab_r1_idx[GAP_MORPH_8_SEKTORS] = {1,2,3,4,5,6,7,0};
  static gint tab_r2_idx[GAP_MORPH_8_SEKTORS] = {2,3,4,5,6,7,0,1};

  gdouble            vektor_x;  /* movement vektor for the current point at in_x/in_y */
  gdouble            vektor_y;
  gdouble            new_vektor_x;
  gdouble            new_vektor_y;
  gdouble            weight;
  gdouble            sum_inv_dist;
  gboolean           direct_hit_flag;
  gint32             sek_idx;
  gdouble            min_sqr_dist;
  gdouble            limit_sqr_dist;
  gdouble dx;
  gdouble dy;
  gdouble adx;
  gdouble ady;



  /* reset sektor tab */
  for(sek_idx=0; sek_idx < GAP_MORPH_8_SEKTORS; sek_idx++)
  {
    wp_sektor_tab[sek_idx] = NULL;
  }
  
  vektor_x = 0;
  vektor_y = 0;
  direct_hit_flag = FALSE;
  min_sqr_dist = wcap->sqr_affect_radius;


  /* loop1 calculate square distance, 
   * check for direct hits
   * and build sector table (nearest workpoints foreach sector
   */
  for(wp=wcap->wp_list; wp != NULL; wp = (GapMorphWorkPoint *)wp->next)
  {
    dx = in_x - wp->dst_x;
    dy = in_y - wp->dst_y;
    
    wp->sqr_dist =  (dx * dx ) + (dy * dy);
    wp->sek_idx = 0;
    
    if(wp->sqr_dist <= wcap->sqr_affect_radius)
    {
      if(wp->sqr_dist < 1)
      {
	new_vektor_x = (wp->src_x - wp->dst_x);
	new_vektor_y = (wp->src_y - wp->dst_y);

	if(direct_hit_flag)
	{
          vektor_x = (new_vektor_x + vektor_x) / 2.0;
          vektor_y = (new_vektor_y + vektor_y) / 2.0;
	}
	else
	{
          vektor_x = new_vektor_x;
          vektor_y = new_vektor_y;
	}
	direct_hit_flag = TRUE;
      }
      else
      {
        wp->is_alive = TRUE;

	/* sektor index 8 sectors */
        sek_idx = p_calc_sector(wp, dx, dy);
	wp->sek_idx = sek_idx;

	if((wp_sek = wp_sektor_tab[sek_idx]) != NULL)
	{
	  /* there is already a workpoint for this sektor */
	  if(wp->sqr_dist <= wp_sek->sqr_dist)
	  {
	    /* set current workpoint as the new nearest point for this sektor */
	    wp_sektor_tab[sek_idx] = wp;
	  }
	  else
	  {
	    continue;
	  }
	}
	else
	{
	  /* found 1.st workpint for this sektor */
	  wp_sektor_tab[sek_idx] = wp;
	}
	wp->next_sek = NULL;  /* init empty sektors list */
        if(wp->sqr_dist < min_sqr_dist)
	{
          min_sqr_dist = wp->sqr_dist;
	}

      }
    }
  }


  if(!direct_hit_flag)
  {
    GapMorphWorkPoint *wp_selected_list;

    sum_inv_dist = 0;
    wp_selected_list = NULL;
    

    /* calculate sector sqr_distance limit table for the 8 sektors */
    if(min_sqr_dist <= GAP_MORPH_NEAR_SQR_RADIUS)
    {
      limit_sqr_dist = min_sqr_dist * GAP_MORPH_MIN_TOL_FAKTOR;
      for(sek_idx = 0; sek_idx < GAP_MORPH_8_SEKTORS; sek_idx++)
      {
        tab_tol[sek_idx] = limit_sqr_dist;
      }
    }
    else
    {
      limit_sqr_dist = wcap->sqr_affect_radius;

      for(sek_idx = 0; sek_idx < GAP_MORPH_8_SEKTORS; sek_idx++)
      {
	wp_sek = wp_sektor_tab[sek_idx];

	if(wp_sek)
	{
 	  GapMorphWorkPoint *wp_l_sek;  /* left neigbour sektor */
	  GapMorphWorkPoint *wp_r_sek;  /* right neigbour sektor */
          gdouble l_tol;
          gdouble r_tol;

          /* xy_relation is the representation of the angle
	   * (is used to compare for similar angle in steps 10tiems finer than the 8 sektors.
	   *  and is faster to calculate than the real angle)
	   */
	  adx = abs(in_x - wp_sek->dst_x);
	  ady = abs(in_y - wp_sek->dst_y);
          if(adx > ady)
	  {
	    wp_sek->xy_relation = (gint)(0.5 + (GAP_MORPH_XY_REL_FAKTOR * (ady / adx)));
	  }
	  else
	  {
	    if(ady != 0)
	    {
	      wp_sek->xy_relation = (gint)(0.5 + (GAP_MORPH_XY_REL_FAKTOR * (adx / ady)));
	    }
	    else
	    {
	      wp_sek->xy_relation = 0;
	    }
	  }

	  wp_l_sek = wp_sektor_tab[tab_l1_idx[sek_idx]];
	  wp_r_sek = wp_sektor_tab[tab_r1_idx[sek_idx]];
	  if(wp_l_sek == NULL)
	  {
	    wp_l_sek = wp_sektor_tab[tab_l2_idx[sek_idx]];
	  }
	  if(wp_r_sek == NULL)
	  {
             wp_r_sek = wp_sektor_tab[tab_r2_idx[sek_idx]];
	  }

	  if(wp_l_sek)
	  {
            if(wp_r_sek)
	    {
              l_tol = wp_l_sek->sqr_dist * GAP_MORPH_TOL_FAKTOR;
	      r_tol = wp_r_sek->sqr_dist * GAP_MORPH_TOL_FAKTOR;
              tab_tol[sek_idx] = MIN(MAX(l_tol, r_tol), (wp_sek->sqr_dist * GAP_MORPH_TOL_FAKTOR));
	    }
	    else
	    {
              tab_tol[sek_idx] = wp_sek->sqr_dist * GAP_MORPH_TOL_FAKTOR;
	    }
	  }
	  else
	  {
            tab_tol[sek_idx] = wp_sek->sqr_dist * GAP_MORPH_TOL_FAKTOR;
	  }
	}
	else
	{
          tab_tol[sek_idx] = limit_sqr_dist;
	}
      }

    }


    
    if(wcap->use_quality_wp_selection)
    {
      /* using the QUALITY workpoint selection strategy:
       * select all points per sector within the sektor specific tolerance
       * but discard those points that have same angle as another nearer point
       * in the same sektor
       */
      for(wp=wcap->wp_list; wp != NULL; wp = (GapMorphWorkPoint *)wp->next)
      {
        sek_idx = CLAMP(wp->sek_idx, 0, (GAP_MORPH_8_SEKTORS -1));

	if(wp->sqr_dist <= tab_tol[sek_idx])
	{
	  gboolean sel_flag;
	  
	  sel_flag = TRUE;
	  
          /* xy_relation is the representation of the angle
	   * (is used to compare for similar angle in steps 10tiems finer than the 8 sektors.
	   *  and is faster to calculate than the real angle)
	   */
	  adx = abs(in_x - wp->dst_x);
	  ady = abs(in_y - wp->dst_y);
          if(adx > ady)
	  {
	    wp->xy_relation = (gint)(0.5 + (GAP_MORPH_XY_REL_FAKTOR * (ady / adx)));
	  }
	  else
	  {
	    if(ady != 0)
	    {
	      wp->xy_relation = (gint)(0.5 + (GAP_MORPH_XY_REL_FAKTOR * (adx / ady)));
	    }
	    else
	    {
	      wp->xy_relation = 0;
	    }
	  }
	  
	  
	  for(wp_sek = wp_sektor_tab[sek_idx]; wp_sek != NULL; wp_sek = (GapMorphWorkPoint *)wp_sek->next_sek)
	  {
	    if(wp_sek == wp)
	    {
	      sel_flag = FALSE;  /* dont add the same point twice to the sektors list */
	      continue;
	    }
	    if((wp_sek->is_alive) 
	    && (wp->xy_relation == wp_sek->xy_relation))  /* check for (nearly) same angle */
	    {
	      if(wp->sqr_dist <  wp_sek->sqr_dist)
	      {
	        /* discard the (farer) point from the sektor list by flag
		 * (is faster than remove the list element)
		 */
                wp_sek->is_alive = FALSE;
	      }
	      else
	      {
	        sel_flag = FALSE;
	      }
	      break;
	    }
	  }
	  
	  if(sel_flag)
	  {
	    /* add wp to the sektor list */
	    wp->next_sek = wp_sektor_tab[sek_idx];
	    wp_sektor_tab[sek_idx] = wp;
	  }
	}
      }
    }
    
    /* build selection list by merging all 8 sektor sublists where workpoint is still alive
     * for the FAST strategy each sektor sublist ha s just one (the nearest) or zero elements
     * the QUALITY strategy can have more points per sektor within the sektor specific tolerance.
     */
    for(sek_idx = 0; sek_idx < GAP_MORPH_8_SEKTORS; sek_idx++)
    {
      for(wp=wp_sektor_tab[sek_idx]; wp != NULL; wp = (GapMorphWorkPoint *)wp->next_sek)
      {
        if((wp->is_alive) 
	&& (wp->sqr_dist <= tab_tol[sek_idx]))
	{
	  wp->dist = sqrt(wp->sqr_dist);
          if(wcap->printf_flag)
	  {
	     /* this is a debug feature for tuning the warp algorithm */
	     printf("sek: %02d, ang: %.2f, dist:%.3f, sqr_dist: %.3f TAB_TOL:%.3f SELECTED\n"
	           ,(int)sek_idx
		   ,(float)wp->angle_deg
		   ,(float)wp->dist
		   ,(float)wp->sqr_dist
		   ,(float)tab_tol[sek_idx]
		   );
	  }
	  
	  /* add point to selection list */
	  wp->next_selected = wp_selected_list;
	  wp_selected_list = wp;

          wp->inv_dist = 1.0 / wp->dist;
          sum_inv_dist += wp->inv_dist;
	}
      }
    }    /* end  for(sek_idx..) */
    

    /* build result vektor as weightened mix of all selected workpoint vektors */
    for(wp = wp_selected_list; wp != NULL; wp = wp->next_selected)
    {
      wp->gravity = 1.0;
      if(wcap->use_gravity)
      {
	wp->gravity -= pow((MIN(wp->sqr_dist,wcap->sqr_affect_radius) / wcap->sqr_affect_radius) , wcap->gravity_intensity);
      }

      new_vektor_x = (wp->src_x - wp->dst_x);
      new_vektor_y = (wp->src_y - wp->dst_y);

      weight = (wp->inv_dist / sum_inv_dist) * wp->gravity;
      vektor_x += (new_vektor_x * weight);
      vektor_y += (new_vektor_y * weight);
      
      if(wcap->printf_flag)
      {
        wp->warp_weight = weight;
        printf("wp->src_x/y:  %.3f / %.3f  wp->dst_x/y: %.3f / %.3f sek_idx: %d xy_rel:%d dist:%.3f weight: %.7f vek: %.3f / %.3f \n"
	      , (float)wp->src_x
              , (float)wp->src_y
              , (float)wp->dst_x
              , (float)wp->dst_y
	      , (int)  wp->sek_idx
	      , (int)  wp->xy_relation
              , (float)wp->dist
              , (float)weight
              , (float)vektor_x
              , (float)vektor_y
              );
      }
    }
  }  /* end direct_hit_flag */

  *out_pick_x = (in_x + vektor_x) * wcap->scale_x;
  *out_pick_y = (in_y + vektor_y) * wcap->scale_y;

}  /* end p_pixel_warp_core */






/* -----------------------------------
 * gap_morph_exec_get_warp_pick_koords
 * -----------------------------------
 * this procedure is used for the show option (a debug feature in the GUI)
 * it is not used for the rendering, but should give an idea
 * how selecting of relevant workpoints will work and set
 * the weight values for all workpoints for later display in the GUI
 * This was the major quality tuning tool for the development of the warp
 * algorithm
 */
void
gap_morph_exec_get_warp_pick_koords(GapMorphWorkPoint *wp_list
                                      , gint32        in_x
                                      , gint32        in_y
				      , gdouble       scale_x
				      , gdouble       scale_y
				      , gboolean      use_quality_wp_selection
				      , gboolean      use_gravity
				      , gdouble       gravity_intensity
				      , gdouble       affect_radius
				      , gdouble *out_pick_x
				      , gdouble *out_pick_y
                                      )
{
  GapMorphWarpCoreAPI  wcap_struct;
  GapMorphWarpCoreAPI *wcap;
  GapMorphWorkPoint *wp;
  gdouble            pick_x;
  gdouble            pick_y;
  gdouble            sum_pick_x;
  gdouble            sum_pick_y;
  gint xx;
  gint yy;
  gint nn;


  wcap = &wcap_struct;
  
  wcap->sqr_affect_radius = affect_radius * affect_radius;
  wcap->wp_list = wp_list;
  wcap->scale_x = scale_x;
  wcap->scale_y = scale_y;
  wcap->use_gravity = use_gravity;
  wcap->gravity_intensity = gravity_intensity;
  wcap->printf_flag = TRUE;
  wcap->use_quality_wp_selection = use_quality_wp_selection;
  
  
  for(wp=wp_list; wp != NULL; wp = (GapMorphWorkPoint *)wp->next)
  {
    wp->warp_weight = -1;  /* for debug display in warp_info_label */
    wp->dist = p_calc_angle(wp, in_x, in_y); /* for debug display in warp_info_label */
    if(wp->dist < 1)
    {
      wp->warp_weight = -4;  /* for debug display in warp_info_label */
    }
  }

  printf("==== scale: %.2f / %.2f  in_x:%02d in_y:%02d == \n"
     , (float)scale_x
     , (float)scale_y
     , (int)in_x
     , (int)in_y
     );
  nn = 0;
  sum_pick_x = 0;
  sum_pick_y = 0;

#ifdef   GAP_MORPH_USE_SIMPLE_PIXEL_WARP
  p_pixel_warp_core(wcap
                   ,in_x
                   ,in_y
                   ,&pick_x
                   ,&pick_y
		   );


#else
  for(xx=MAX(0,in_x -3); xx <= in_x +3; xx+=3)
  {
    wcap->printf_flag = FALSE;
    for(yy=MAX(0,in_y -3); yy <= in_y +3; yy+=3)
    {
      p_pixel_warp_core(wcap
                       ,xx
                       ,yy
                       ,&pick_x
                       ,&pick_y
		       );
       sum_pick_x += pick_x;		       
       sum_pick_y += pick_y;
       nn++;		       
    }
  }
  
  pick_x = sum_pick_x / (gdouble)nn;
  pick_y = sum_pick_y / (gdouble)nn;

  /* this call is for reset wp->angel and other things to the center pick point */
  wcap->printf_flag = TRUE;
  p_pixel_warp_core(wcap
                   ,in_x
                   ,in_y
                   ,out_pick_x
                   ,out_pick_y
		   );

#endif



  *out_pick_x = pick_x;
  *out_pick_y = pick_y;

  printf("pick_x/y :  %.3f / %.3f\n"
            , (float)*out_pick_x
            , (float)*out_pick_y
            );

}  /* end gap_morph_exec_get_warp_pick_koords */



/* ---------------------------------
 * p_pixel_warp_pick
 * ---------------------------------
 */
static void
p_pixel_warp_pick(GapMorphWarpCoreAPI *wcap
            , gint32        in_x
            , gint32        in_y
	    , gint          set_idx
	    , gdouble      *out_pick_x
	    , gdouble      *out_pick_y
            )
{
  gdouble            pick_x;
  gdouble            pick_y;
  gdouble            sum_pick_x;
  gdouble            sum_pick_y;
  gint xx;
  gint yy;
  gint nn;
  GapMorphExePickCache *pcp;

#ifdef GAP_MORPH_USE_SIMPLE_PIXEL_WARP
        p_pixel_warp_core(wcap
                       ,in_x
                       ,in_y
                       ,out_pick_x
                       ,out_pick_y
		       );

#else
  /* the implementation of the   p_pixel_warp_core
   * produces unwanted hard edges at those places where
   * the selection of relevant near workpoints
   * switches to another set of workpoints with
   * signifikant different movement settings.
   * To comensate this unwanted effect, we do multiple picks 
   * and use the average pick koordinates.
   * We use gloabl_pick_cache to reduce the effective
   * number of p_pixel_warp_core calls
   */
  nn = 0;
  sum_pick_x = 0;		       
  sum_pick_y = 0;		       
  for(xx=MAX(0,in_x -3); xx <= in_x +3; xx+=3)
  {
    for(yy=MAX(0,in_y -3); yy <= in_y +3; yy+=3)
    {
      gint ii;
      
      for(ii=0; ii < GAP_MORPH_PCK_CACHE_SIZE; ii++)
      {
        pcp = &gloabl_pick_cache[ii][set_idx];
        if ((pcp->valid)
	&&  (pcp->xx == xx)
	&&  (pcp->yy == yy))
	{
	  pick_x = pcp->pick_x;
	  pick_y = pcp->pick_y;
	  
	  /* printf("** CACHE HIT\n"); */
	  break;
	}
      }
      if(ii == GAP_MORPH_PCK_CACHE_SIZE)
      {
        /* printf("** must do pixel warp\n"); */
        p_pixel_warp_core(wcap
                       ,xx
                       ,yy
                       ,&pick_x
                       ,&pick_y
		       );
         /* save pick koords in cache table */
         pcp = &gloabl_pick_cache[gloabl_pick_cache_idx[set_idx]][set_idx];

	 pcp->xx     = xx;
	 pcp->yy     = yy;
	 pcp->pick_x = pick_x;
	 pcp->pick_y = pick_y;
	 pcp->valid  = TRUE;
	 gloabl_pick_cache_idx[set_idx]++;
	 if(gloabl_pick_cache_idx[set_idx] >= GAP_MORPH_PCK_CACHE_SIZE)
	 {
	   gloabl_pick_cache_idx[set_idx] = 0;
	 }
       }
       sum_pick_x += (pick_x - ((in_x - xx) * wcap->scale_x));
       sum_pick_y += (pick_y - ((in_y - yy) * wcap->scale_y));
       nn++;		       
    }
  }
  *out_pick_x = sum_pick_x / (gdouble)nn;
  *out_pick_y = sum_pick_y / (gdouble)nn;

#endif
}  /* end p_pixel_warp_pick */


/* ---------------------------------
 * p_pixel_warp_multipick
 * ---------------------------------
 */
static void
p_pixel_warp_multipick(GapMorphWarpCoreAPI *wcap_1
                      ,GapMorphWarpCoreAPI *wcap_2
		      ,gdouble wp_mix_factor
                      , gint32        in_x
                      , gint32        in_y
		      , gdouble      *out_pick_x
		      , gdouble      *out_pick_y
                      )
{
  gdouble            pick_x_1;
  gdouble            pick_y_1;
  gdouble            pick_x_2;
  gdouble            pick_y_2;

  pick_x_1 = 0;
  pick_y_1 = 0;
  pick_x_2 = 0;
  pick_y_2 = 0;
  
  if(wp_mix_factor != 0.0)
  {
    p_pixel_warp_pick(wcap_1
	  , in_x
	  , in_y
	  , 0                      /* set_idx */
	  , &pick_x_1
	  , &pick_y_1
          );
  }
  
  if(wp_mix_factor != 1.0)
  {
    p_pixel_warp_pick(wcap_1
	  , in_x
	  , in_y
	  , 1                      /* set_idx */
	  , &pick_x_2
	  , &pick_y_2
          );
  }
  
  *out_pick_x = (pick_x_1 * wp_mix_factor) + (pick_x_2 * (1 - wp_mix_factor));
  *out_pick_y = (pick_y_1 * wp_mix_factor) + (pick_y_2 * (1 - wp_mix_factor));

}  /* end p_pixel_warp_multipick */


/* ---------------------------------
 * p_calculate_work_point_movement
 * ---------------------------------
 * return the newly created workpoint list
 * the list contains duplicates of the input workpoint master_list
 * with transformed point koordinates, according to current movement
 * step and scaling.
 * 
 * forward_move == TRUE:
 *   dup->fdest_x,y := master->fdest_x,y
 *   dup->orsc_x,y  := master->orsc_x,y
 *   dup->src_x,y   := scale (master->osrc_x,y)
 *   dup->dst_x,y   := linear_advance(master->orsc_x,y  --> master->fdest_x,y)
 *
 * forward_move == FALSE:
 *   dup->fdest_x,y := master->osrc_x,y
 *   dup->orsc_x,y  := master->fdest_x,y
 *   dup->src_x,y   := scale (master->fdest_x,y)
 *   dup->dst_x,y   := linear_advance(master->fdest_x,y  --> master->orsc_x,y)
 *
 */
GapMorphWorkPoint *
p_calculate_work_point_movement(gdouble total_steps
                               ,gdouble current_step
			       ,GapMorphWorkPoint *master_list
			       ,gint32 src_layer_id
			       ,gint32 dst_layer_id
                               ,gint32 curr_width
                               ,gint32 curr_height
                               ,gboolean forward_move
                               )
{
  GapMorphWorkPoint *wp_list;
  GapMorphWorkPoint *wp_elem_orig;
  GapMorphWorkPoint *wp_elem_dup;
  GapMorphWorkPoint *wp_elem_prev;
  gdouble            scalex;
  gdouble            scaley;


  if(gap_debug) printf ("p_calculate_work_point_movement: START\n");

  if(forward_move)
  {
    scalex = (gdouble)curr_width  / (gdouble)gimp_drawable_width(src_layer_id);
    scaley = (gdouble)curr_height / (gdouble)gimp_drawable_height(src_layer_id);
  }
  else
  {
    scalex = (gdouble)curr_width  / (gdouble)gimp_drawable_width(dst_layer_id);
    scaley = (gdouble)curr_height / (gdouble)gimp_drawable_height(dst_layer_id);
  }
  
  wp_list = NULL;
  wp_elem_prev = NULL;
  
  for(wp_elem_orig = master_list; wp_elem_orig != NULL; wp_elem_orig = (GapMorphWorkPoint *)wp_elem_orig->next)
  {
    wp_elem_dup = g_new(GapMorphWorkPoint, 1);
    wp_elem_dup->next = NULL;
    if(forward_move)
    {
      wp_elem_dup->fdst_x = wp_elem_orig->fdst_x;
      wp_elem_dup->fdst_y = wp_elem_orig->fdst_y;
      wp_elem_dup->osrc_x = wp_elem_orig->osrc_x;
      wp_elem_dup->osrc_y = wp_elem_orig->osrc_y;
      wp_elem_dup->dst_x = p_linear_advance((gdouble)total_steps
                                ,(gdouble)current_step
                                ,(gdouble)wp_elem_orig->osrc_x
                                ,(gdouble)wp_elem_orig->fdst_x
                                );
      wp_elem_dup->dst_y = p_linear_advance((gdouble)total_steps
                                ,(gdouble)current_step
                                ,(gdouble)wp_elem_orig->osrc_y
                                ,(gdouble)wp_elem_orig->fdst_y
                                );
      wp_elem_dup->src_x = scalex * wp_elem_orig->osrc_x;
      wp_elem_dup->src_y = scaley * wp_elem_orig->osrc_y;
    }
    else
    {
      wp_elem_dup->fdst_x = wp_elem_orig->osrc_x;
      wp_elem_dup->fdst_y = wp_elem_orig->osrc_y;
      wp_elem_dup->osrc_x = wp_elem_orig->fdst_x;
      wp_elem_dup->osrc_y = wp_elem_orig->fdst_y;
      wp_elem_dup->dst_x = p_linear_advance((gdouble)total_steps
                                ,(gdouble)(total_steps - current_step)
                                ,(gdouble)wp_elem_orig->fdst_x
                                ,(gdouble)wp_elem_orig->osrc_x
                                );
      wp_elem_dup->dst_y = p_linear_advance((gdouble)total_steps
                                ,(gdouble)(total_steps - current_step)
                                ,(gdouble)wp_elem_orig->fdst_y
                                ,(gdouble)wp_elem_orig->osrc_y
                                );
      wp_elem_dup->src_x = scalex * wp_elem_orig->fdst_x;
      wp_elem_dup->src_y = scaley * wp_elem_orig->fdst_y;
    }
    

    if(gap_debug)
    {
      printf("WP_DUP: osrc: %03.2f / %03.2f   fdst: %03.2f / %03.2f src: %03.2f / %03.2f   dst: %03.2f / %03.2f\n"
          ,(float)wp_elem_dup->osrc_x
          ,(float)wp_elem_dup->osrc_y
          ,(float)wp_elem_dup->fdst_x
          ,(float)wp_elem_dup->fdst_y
          ,(float)wp_elem_dup->src_x
          ,(float)wp_elem_dup->src_y
          ,(float)wp_elem_dup->dst_x
          ,(float)wp_elem_dup->dst_y
          );
    }



    if(wp_elem_prev == NULL)
    {
      wp_list = wp_elem_dup;
    }
    else
    {
      wp_elem_prev->next = wp_elem_dup;
    }
    wp_elem_prev = wp_elem_dup;
  }
  
  return(wp_list);
  
}  /* end p_calculate_work_point_movement */


/* ---------------------------------
 * p_layer_warp_move
 * ---------------------------------
 */
static void
p_layer_warp_move (GapMorphWorkPoint     *wp_list_1
                  ,GapMorphWorkPoint     *wp_list_2
                  , gint32                src_layer_id
                  , gint32                dst_layer_id
		  , GapMorphGlobalParams *mgpp
		  , GapMorphWarpCoreAPI  *p1
		  , GapMorphWarpCoreAPI  *p2
		  , gdouble               wp_mix_factor
                  )
{
  GapMorphWarpCoreAPI  wcap_struct_1;
  GapMorphWarpCoreAPI  wcap_struct_2;
  GapMorphWarpCoreAPI *wcap_1;
  GapMorphWarpCoreAPI *wcap_2;
  GimpDrawable *src_drawable;
  GimpDrawable *dst_drawable;
  GimpPixelFetcher *src_pixfet;
  GimpPixelRgn    dstPR;
  gpointer        pr;
  guchar         *pixel_ptr;
  guint32         l_row;
  guint32         l_col;
  gdouble         scale_x;
  gdouble         scale_y;
  gdouble         l_max_progress;
  gdouble         l_progress;
  gint ii;
  gint set_idx;

  wcap_1 = &wcap_struct_1;
  wcap_2 = &wcap_struct_2;

  wcap_1->wp_list = wp_list_1;
  wcap_2->wp_list = wp_list_2;
  
  wcap_1->sqr_affect_radius = mgpp->affect_radius * mgpp->affect_radius;
  wcap_1->use_gravity = mgpp->use_gravity;
  wcap_1->gravity_intensity = mgpp->gravity_intensity;
  if(p1)
  {
    wcap_1->sqr_affect_radius = p1->affect_radius * p1->affect_radius;
    wcap_1->use_gravity = p1->use_gravity;
    wcap_1->gravity_intensity = p1->gravity_intensity;
  }
  
  wcap_2->sqr_affect_radius = mgpp->affect_radius * mgpp->affect_radius;
  wcap_2->use_gravity = mgpp->use_gravity;
  wcap_2->gravity_intensity = mgpp->gravity_intensity;
  if(p2)
  {
    wcap_2->sqr_affect_radius = p2->sqr_affect_radius * p2->sqr_affect_radius;
    wcap_2->use_gravity = p2->use_gravity;
    wcap_2->gravity_intensity = p2->gravity_intensity;
  }
  

  wcap_1->printf_flag = FALSE;
  wcap_2->printf_flag = FALSE;

  wcap_1->use_quality_wp_selection = mgpp->use_quality_wp_selection;
  wcap_2->use_quality_wp_selection = mgpp->use_quality_wp_selection;
  
  /* clear the global cache for picking koordinates */
  for(set_idx=0; set_idx < 2; set_idx++)
  {
    for(ii=0; ii < GAP_MORPH_PCK_CACHE_SIZE; ii++)
    {
       gloabl_pick_cache[ii][set_idx].valid = FALSE;
    }
    gloabl_pick_cache_idx[set_idx] = 0;
  }

  src_drawable = gimp_drawable_get (src_layer_id);
  dst_drawable = gimp_drawable_get (dst_layer_id);

  if(src_drawable == NULL)                   { return; }
  if(dst_drawable == NULL)                   { return; }
  if(src_drawable->bpp != dst_drawable->bpp) { return; }
  if((src_drawable->bpp != 4)  && (src_drawable->bpp != 2))
  { 
    return;
  }


  /* if src and dst size not equal: caluclate scaling factors x/y */
  scale_x = src_drawable->width / MAX(1,dst_drawable->width);
  scale_y = src_drawable->height / MAX(1,dst_drawable->height);

  wcap_1->scale_x = scale_x;
  wcap_2->scale_x = scale_x;
  wcap_1->scale_y = scale_y;
  wcap_2->scale_y = scale_y;

  /* init Pixel Fetcher for source drawable  */
  src_pixfet = gimp_pixel_fetcher_new (src_drawable, FALSE /*shadow*/);
  if(src_drawable == NULL)                   { return; }

  
  /* init Pixel Region  */
  gimp_pixel_rgn_init (&dstPR, dst_drawable
                      , 0
                      , 0
                      , dst_drawable->width
                      , dst_drawable->height
                      , TRUE      /* dirty */
                      , TRUE      /* shadow */
                       );

  l_max_progress = dst_drawable->width * dst_drawable->height;
  l_progress = 0;


  for (pr = gimp_pixel_rgns_register (1, &dstPR);
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr))
  {
     guint x;
     guint y;
     guchar *dest;

     dest = dstPR.data;
     for (y = 0; y < dstPR.h; y++)
     {
        pixel_ptr = dest;
	l_row = dstPR.y + y;
	
        for (x = 0; x < dstPR.w; x++)
	{
           gdouble            pick_x;
           gdouble            pick_y;
	   
	   l_col = dstPR.x + x;
	   if(mgpp->multiple_pointsets)
	   {
	     /* pick based on 2 sets of workpoints */
             p_pixel_warp_multipick(wcap_1  /// XXXXX list1
	           , wcap_2                 /// XXXXX list2
		   , wp_mix_factor
		   , l_col
		   , l_row
		   , &pick_x
		   , &pick_y
                   );
	   }
	   else
	   {
	     /* pick based on a single set of workpoints */
             p_pixel_warp_pick(wcap_1
	           , l_col
		   , l_row
		   , 0                      /* set_idx */
		   , &pick_x
		   , &pick_y
                   );
	   }
           p_bilinear_get_pixel (src_pixfet
	                        ,src_drawable
				,pick_x
				,pick_y
				,pixel_ptr
				,dst_drawable->bpp);
           pixel_ptr += dstPR.bpp;
	}
        dest += dstPR.rowstride;
     }
     
     if(mgpp->do_progress)
     {
       l_progress += (dstPR.w * dstPR.h);
       gimp_progress_update(mgpp->master_progress 
                           + (mgpp->layer_progress_step * (l_progress /l_max_progress))
			   );
     }

  }

  gimp_drawable_flush (dst_drawable);
  gimp_drawable_merge_shadow (dst_drawable->drawable_id, TRUE);
  gimp_drawable_update (dst_layer_id
                      , 0
                      , 0
                      , dst_drawable->width
                      , dst_drawable->height
                      );
  gimp_pixel_fetcher_destroy (src_pixfet);
  gimp_drawable_detach (src_drawable);
  gimp_drawable_detach (dst_drawable);

}   /* end p_layer_warp_move */

/* ---------------------------------
 * p_mix_layers
 * ---------------------------------
 * create a layer by mixing bg_layer_id and top_layer_id
 */
static gint32
p_mix_layers (gint32  curr_image_id
                  ,gint32  bg_layer_id
                  ,gint32  top_layer_id
                  ,gint32  curr_width
		  ,gint32  curr_height
		  ,gdouble curr_mix_factor  /* 0.0 <= mix <= 1.0 */
                  )
{
  gint32        dst_layer_id;
  GimpDrawable *top_drawable;
  GimpDrawable *bg_drawable;
  GimpDrawable *dst_drawable;
  GimpPixelRgn    dstPR;
  GimpPixelRgn    topPR;
  GimpPixelRgn    bgPR;
  gpointer        pr;


  top_drawable = gimp_drawable_get (top_layer_id);
  bg_drawable  = gimp_drawable_get (bg_layer_id);

  if(top_drawable == NULL)                   { return (-1); }
  if(bg_drawable == NULL)                    { return (-1); }
  if(top_drawable->bpp < 3)
  {
    dst_layer_id = gimp_layer_new(curr_image_id, "morph_tween"
                               , curr_width
                               , curr_height
                               , GIMP_GRAYA_IMAGE
                               , 100.0      /* full opaque */
                               , GIMP_NORMAL_MODE
                               );
  }
  else
  {
    dst_layer_id = gimp_layer_new(curr_image_id, "morph_tween"
                               , curr_width
                               , curr_height
                               , GIMP_RGBA_IMAGE
                               , 100.0      /* full opaque */
                               , GIMP_NORMAL_MODE
                               );
  }

  gimp_image_add_layer(curr_image_id, dst_layer_id, 0);


  dst_drawable = gimp_drawable_get (dst_layer_id);
  if(dst_drawable == NULL)                   { return (-1); }



  
  /* init Pixel Region  */
  gimp_pixel_rgn_init (&dstPR, dst_drawable
                      , 0
                      , 0
                      , dst_drawable->width
                      , dst_drawable->height
                      , TRUE      /* dirty */
                      , TRUE      /* shadow */
                       );
  gimp_pixel_rgn_init (&topPR, top_drawable
                      , 0
                      , 0
                      , top_drawable->width
                      , top_drawable->height
                      , FALSE      /* dirty */
                      , FALSE      /* shadow */
                       );
  gimp_pixel_rgn_init (&bgPR, bg_drawable
                      , 0
                      , 0
                      , bg_drawable->width
                      , bg_drawable->height
                      , FALSE      /* dirty */
                      , FALSE      /* shadow */
                       );

  for (pr = gimp_pixel_rgns_register (3, &dstPR, &topPR, &bgPR);
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr))
  {
     guint x;
     guint y;
     guchar *dest;
     guchar *top;
     guchar *bg;
     guchar         *pixel_ptr;
     guchar         *top_ptr;
     guchar         *bg_ptr;

     dest = dstPR.data;
     top  = topPR.data;
     bg   = bgPR.data;
     for (y = 0; y < dstPR.h; y++)
     {
        pixel_ptr = dest;
        top_ptr = top;
        bg_ptr = bg;
	
        for (x = 0; x < dstPR.rowstride ; x++)
	{
	   gdouble val;
	   
	   val = MIX_VALUE(curr_mix_factor, (gdouble)(*bg_ptr), (gdouble)(*top_ptr));
	   *pixel_ptr = (guchar)val;
	   
           pixel_ptr++;
           top_ptr++;
           bg_ptr++;
	}
        dest += dstPR.rowstride;
        top  += topPR.rowstride;
        bg   += bgPR.rowstride;
     }
  }

  gimp_drawable_flush (dst_drawable);
  gimp_drawable_merge_shadow (dst_drawable->drawable_id, TRUE);
  gimp_drawable_update (dst_layer_id
                      , 0
                      , 0
                      , dst_drawable->width
                      , dst_drawable->height
                      );
  gimp_drawable_detach (top_drawable);
  gimp_drawable_detach (bg_drawable);
  gimp_drawable_detach (dst_drawable);
  return(dst_layer_id);

}   /* end p_mix_layers */

/* ---------------------------------
 * p_create_morph_tween_frame
 * ---------------------------------
 * return the newly created tween morphed layer
 */
static gint32
p_create_morph_tween_frame(gint32 total_steps
                          ,gint32 current_step
                          ,GapMorphGlobalParams *mgpp
			  ,GapMorphExeLayerstack *mlayers
                          )
{
   GimpDrawable *dst_drawable;
   gint32  curr_image_id;  /* a temp image of current layer size */
   gint32  bg_layer_id;
   gint32  top_layer_id;
   gint32  merged_layer_id;
   gint32  curr_width;
   gint32  curr_height;
   gdouble curr_opacity;
   GapMorphWorkPoint *curr_wp_list_1;
   GapMorphWorkPoint *curr_wp_list_2;
   GapMorphWarpCoreAPI *wp_set_1;
   GapMorphWarpCoreAPI *wp_set_2;
   gint32 src_layer_id;
   gint32 dst_layer_id;
   gboolean forward_move;
   gdouble  wp_mix_factor;
   
   forward_move = TRUE;
   src_layer_id = mgpp->osrc_layer_id;
   dst_layer_id = mgpp->fdst_layer_id;

   wp_set_1 = NULL;
   wp_set_2 = NULL;
   wp_mix_factor = 1.0;

   if(mgpp->create_tween_layers)
   {
     /* size of the new frame */
     curr_width = p_linear_advance((gdouble)total_steps
                                  ,(gdouble)current_step
                                  ,(gdouble)gimp_drawable_width(src_layer_id)
                                  ,(gdouble)gimp_drawable_width(dst_layer_id)
                                  );
     curr_height = p_linear_advance((gdouble)total_steps
                                  ,(gdouble)current_step
                                  ,(gdouble)gimp_drawable_height(src_layer_id)
                                  ,(gdouble)gimp_drawable_height(dst_layer_id)
                                  );
   }
   else
   {
     gint ii;
     
     ii = mlayers->dst1_idx - (current_step -1);
     
     /* operate on existing destination layers */
     dst_layer_id = mlayers->dst_layers[ii];
     curr_width = gimp_drawable_width(dst_layer_id);
     curr_height = gimp_drawable_height(dst_layer_id);
     
     ii = p_linear_advance((gdouble)total_steps
                                  ,(gdouble)current_step
                                  ,(gdouble)mlayers->src1_idx
                                  ,(gdouble)mlayers->src2_idx
                                  );

     /* in this case also operate with changing source layers (if there are more than one) */
     src_layer_id = mlayers->src_layers[ii];
   }

   if((mgpp->multiple_pointsets)
   && (mlayers->available_wp_sets > 1))
   {
     gint wps_idx;
     gint wps_idx_2;
     gdouble wp_stepspeed;
     gdouble ref;
     
     wp_stepspeed = (gdouble)(mlayers->available_wp_sets -1) / (gdouble)MIN((total_steps -1),1);
     ref = (gdouble)((gdouble)(current_step -1) * wp_stepspeed);
     wps_idx = (gint)ref;
     wps_idx_2 = MIN(wps_idx+1, (mlayers->available_wp_sets -1));

     wp_set_1 = mlayers->tab_wp_sets[wps_idx];
     wp_set_2 = mlayers->tab_wp_sets[wps_idx_2];
     wp_mix_factor = ref - (gdouble)wps_idx;
   }


   dst_drawable = gimp_drawable_get (dst_layer_id);
   
   /* create the tween frame image */
   curr_image_id = gimp_image_new(curr_width, curr_height, GIMP_RGB);

   /* add empty BG layer */
   if(dst_drawable->bpp < 3)
   {
     bg_layer_id = gimp_layer_new(curr_image_id, "bg_morph_layer"
                               , curr_width
                               , curr_height
                               , GIMP_GRAYA_IMAGE
                               , 100.0      /* full opaque */
                               , GIMP_NORMAL_MODE
                               );
   }
   else
   {
     bg_layer_id = gimp_layer_new(curr_image_id, "bg_morph_layer"
                               , curr_width
                               , curr_height
                               , GIMP_RGBA_IMAGE
                               , 100.0      /* full opaque */
                               , GIMP_NORMAL_MODE
                               );
   }
   gimp_image_add_layer(curr_image_id, bg_layer_id, 0);

   /* add empty TOP layer */
   curr_opacity = p_linear_advance((gdouble)total_steps
                                ,(gdouble)current_step
                                ,(gdouble)0.0
                                ,(gdouble)100.0
                                );
   top_layer_id = gimp_layer_new(curr_image_id, "top_morph_layer"
                               , curr_width
                               , curr_height
                               , GIMP_RGBA_IMAGE
                               , curr_opacity
                               , GIMP_NORMAL_MODE
                               );
   gimp_image_add_layer(curr_image_id, top_layer_id, 0);

   


   if(!mgpp->render_mode == GAP_MORPH_RENDER_MODE_WARP)
   {
     if((wp_set_1) && (wp_set_2))
     {
       /* multi workpoint settings for src_layer */
       curr_wp_list_1 = p_calculate_work_point_movement((gdouble)total_steps
                                                     ,(gdouble)current_step
                                                     ,wp_set_1->wp_list
						     ,mgpp->osrc_layer_id
						     ,mgpp->fdst_layer_id
                                                     ,curr_width
                                                     ,curr_height
                                                     ,forward_move
                                                     );

       curr_wp_list_2 = p_calculate_work_point_movement((gdouble)total_steps
                                                     ,(gdouble)current_step
                                                     ,wp_set_2->wp_list
						     ,mgpp->osrc_layer_id
						     ,mgpp->fdst_layer_id
                                                     ,curr_width
                                                     ,curr_height
                                                     ,forward_move
                                                     );

       /* warp the BG layer */
       p_layer_warp_move ( curr_wp_list_1
                         , curr_wp_list_2
                	 , src_layer_id
                	 , bg_layer_id
			 , mgpp
			 , wp_set_1
			 , wp_set_2
			 , wp_mix_factor
                	 );

       gap_morph_exec_free_workpoint_list(&curr_wp_list_1);
       gap_morph_exec_free_workpoint_list(&curr_wp_list_2);
     }
     else
     {
       /* simple workpoint settings for src_layer */
       curr_wp_list_1 = p_calculate_work_point_movement((gdouble)total_steps
                                                     ,(gdouble)current_step
                                                     ,mgpp->master_wp_list
						     ,mgpp->osrc_layer_id
						     ,mgpp->fdst_layer_id
                                                     ,curr_width
                                                     ,curr_height
                                                     ,forward_move
                                                     );

       /* warp the BG layer */
       p_layer_warp_move ( curr_wp_list_1
                         , NULL
                	 , src_layer_id
                	 , bg_layer_id
			 , mgpp
			 , NULL
			 , NULL
			 , wp_mix_factor
                	 );

       gap_morph_exec_free_workpoint_list(&curr_wp_list_1);
     }
     forward_move = FALSE;
     mgpp->master_progress += mgpp->layer_progress_step;
   }
   
   if((wp_set_1) && (wp_set_2))
   {
     /* milti workpoint settings for sc_layer */
     curr_wp_list_1 = p_calculate_work_point_movement((gdouble)total_steps
                                                   ,(gdouble)current_step
                                                   ,wp_set_1->wp_list
						   ,mgpp->osrc_layer_id
						   ,mgpp->fdst_layer_id
                                                   ,curr_width
                                                   ,curr_height
                                                   ,forward_move
                                                   );
     
     curr_wp_list_2 = p_calculate_work_point_movement((gdouble)total_steps
                                                   ,(gdouble)current_step
                                                   ,wp_set_2->wp_list
						   ,mgpp->osrc_layer_id
						   ,mgpp->fdst_layer_id
                                                   ,curr_width
                                                   ,curr_height
                                                   ,forward_move
                                                   );
     
     /* warp the TOP layer */
     p_layer_warp_move ( curr_wp_list_1
                       , curr_wp_list_2
                       , dst_layer_id
                       , top_layer_id
		       , mgpp
		       , wp_set_1
		       , wp_set_2
		       , wp_mix_factor
                       );

     gap_morph_exec_free_workpoint_list(&curr_wp_list_1);
     gap_morph_exec_free_workpoint_list(&curr_wp_list_2);
   }
   else
   {
     /* simple workpoint settings for sc_layer */
     curr_wp_list_1 = p_calculate_work_point_movement((gdouble)total_steps
                                                   ,(gdouble)current_step
                                                   ,mgpp->master_wp_list
						   ,mgpp->osrc_layer_id
						   ,mgpp->fdst_layer_id
                                                   ,curr_width
                                                   ,curr_height
                                                   ,forward_move
                                                   );

     /* warp the TOP layer */
     p_layer_warp_move ( curr_wp_list_1
                       , NULL
                       , dst_layer_id
                       , top_layer_id
		       , mgpp
		       , NULL
		       , NULL
		       , wp_mix_factor
                       );

     gap_morph_exec_free_workpoint_list(&curr_wp_list_1);
   }


   if(mgpp->render_mode == GAP_MORPH_RENDER_MODE_WARP)
   {
     /* for warp only mode all is done at this point */
     return(top_layer_id);
   }

   /* merge BG and TOP Layer (does mix opacity according to current step) */
//   merged_layer_id = 
//   gap_image_merge_visible_layers(curr_image_id, GIMP_EXPAND_AS_NECESSARY);


   merged_layer_id = 
   p_mix_layers(curr_image_id
               ,bg_layer_id
	       ,top_layer_id
	       ,curr_width
	       ,curr_height
	       ,(gdouble)(curr_opacity / 100.0)
	       );

   /* DEBUG code: show duplicate of the temporary tween image */
   if(FALSE)
   {
     gint32 dup_id;

     dup_id = gimp_image_duplicate(curr_image_id);
     gimp_display_new (dup_id);

   }
   gimp_drawable_detach (dst_drawable);
   return(merged_layer_id);

}  /* end p_create_morph_tween_frame  */


/* ---------------------------------
 * p_get_tween_steps_and_layerstacks
 * ---------------------------------
 * get layerstack for both src and dst
 * 
 * return the constraint tween_steps value
 * when operating on existing destination layers
 */
static gint32
p_get_tween_steps_and_layerstacks(GapMorphGlobalParams *mgpp, GapMorphExeLayerstack *mlayers)
{
  gint32 src_image_id;
  gint32 dst_image_id;
  gint32 tween_steps;
  gint32 ii;
  
  tween_steps = 0;
  mlayers->src_nlayers = 0;
  mlayers->src_layers = NULL;
  mlayers->dst_nlayers = 0;
  mlayers->dst_layers = NULL;

  mlayers->src1_idx = -1;
  mlayers->src2_idx = -1;
  mlayers->dst1_idx = -1;
  mlayers->dst2_idx = -1;
  src_image_id = -1;
  dst_image_id = -1;


  
  if(mgpp->osrc_layer_id >= 0)
  {
    src_image_id = gimp_drawable_get_image(mgpp->osrc_layer_id);
    
    mlayers->src_layers = gimp_image_get_layers (src_image_id
                                             ,&mlayers->src_nlayers
				             );
    for(ii=0; ii < mlayers->src_nlayers; ii++)
    {
      if(gap_debug) printf("src: id[%d]: %d\n", (int)ii, (int)mlayers->src_layers[ii] );
      if(mlayers->src_layers[ii] == mgpp->osrc_layer_id)
      {
        mlayers->src1_idx = ii;  /* src at 1.st step */
        mlayers->src2_idx = ii;  /* src at last step */
	break;
      }
    }
  }
  
  if(mgpp->fdst_layer_id >= 0)
  {
    tween_steps = mgpp->tween_steps;
    
    dst_image_id = gimp_drawable_get_image(mgpp->fdst_layer_id);
    mlayers->dst_layers = gimp_image_get_layers (dst_image_id
                                             ,&mlayers->dst_nlayers
				             );
    for(ii=0; ii < mlayers->dst_nlayers; ii++)
    {

      if(gap_debug) printf("dst: id[%d]: %d\n", (int)ii, (int)mlayers->dst_layers[ii] );

      if(mlayers->dst_layers[ii] == mgpp->fdst_layer_id)
      {
        mlayers->dst2_idx = ii;  /* dst at last step */
	break;
      }
    }
  }

  if(mgpp->create_tween_layers)
  {
    mlayers->dst1_idx = mlayers->dst2_idx + (tween_steps -1);
    return(tween_steps);
  }
  
  if(mgpp->render_mode == GAP_MORPH_RENDER_MODE_WARP)
  {
    if(dst_image_id == src_image_id)
    {
      if(mlayers->dst2_idx > mlayers->src1_idx)
      {
        gint32 tmp;
        /* source layer is above the destination layer
	 * in this case swap src with dst layer
	 */
	tmp = mgpp->fdst_layer_id;
	mgpp->fdst_layer_id = mgpp->osrc_layer_id;
	mgpp->osrc_layer_id = tmp;
	
	tmp = mlayers->dst2_idx;
	mlayers->dst2_idx = mlayers->src1_idx;
	mlayers->src1_idx = tmp;
	mlayers->src2_idx = tmp;
      }
      tween_steps = 1 + (mlayers->src1_idx - mlayers->dst2_idx );
    }
    else
    {
      tween_steps = MIN(tween_steps, (mlayers->dst_nlayers - mlayers->dst2_idx) );
    }
  }

  mlayers->dst1_idx = mlayers->dst2_idx + (tween_steps -1);
  if(mgpp->render_mode == GAP_MORPH_RENDER_MODE_WARP)
  {
    mgpp->osrc_layer_id = mlayers->dst_layers[mlayers->dst1_idx];
  }
  
  mlayers->src1_idx = MIN((mlayers->src2_idx + (tween_steps -1)) , (mlayers->src_nlayers -1));

  return(tween_steps);
}   /* end p_get_tween_steps_and_layerstacks */


/* ---------------------------------
 * gap_morph_execute
 * ---------------------------------
 * layer morphing master procedure
 */
gint32  
gap_morph_execute(GapMorphGlobalParams *mgpp)
{
  gint32 total_steps;
  gint32 current_step;
  gint32 last_step;
  gint32 new_layer_id;
  gint32 cp_layer_id;
  gint32 dst_image_id;
  gint32 dst_stack_position;
  GapMorphExeLayerstack mlayers_struct;
  GapMorphExeLayerstack *mlayers;
  
  mlayers = &mlayers_struct;
  mlayers->tab_wp_sets = NULL;
  mlayers->available_wp_sets = 0;
  
  mgpp->tween_steps = p_get_tween_steps_and_layerstacks(mgpp, mlayers);
  
  if(mgpp->multiple_pointsets)
  {
    /* check for available workpointfile sets and load them */
    p_build_wp_set_table(mgpp ,mlayers);
  }
  
  if(gap_debug) printf("gap_morph_execute: START src_layer:%d dst_layer:%d  tween_steps:%d\n"
         ,(int)mgpp->osrc_layer_id
         ,(int)mgpp->fdst_layer_id
         ,(int)mgpp->tween_steps
         );

  if(mgpp->osrc_layer_id == mgpp->fdst_layer_id)
  {
     /* for identical layers we do only warp
      * (fading identical images makes no sense, force warp only mode)
      */
    mgpp->render_mode = GAP_MORPH_RENDER_MODE_WARP;
  }

  if(mgpp->do_progress)
  {
    if(mgpp->render_mode == GAP_MORPH_RENDER_MODE_MORPH)
    {
      gimp_progress_init(_("creating morph tween layers..."));
    }
    else
    {
      gimp_progress_init(_("creating warp tween layers..."));
    }
    
  }

  cp_layer_id = -1;
  dst_image_id = gimp_drawable_get_image(mgpp->fdst_layer_id);
  dst_stack_position = 1;

  /* findout stackposition (where to insert new tween layer(s)
   * in the destination image.
   * (one below the mgpp->fdst_layer_id Layer)
   * where 0 is on top of stack
   */
  {
    gint       ii;
    gint       l_nlayers;
    gint32    *l_layers_list;

    l_layers_list = gimp_image_get_layers(dst_image_id, &l_nlayers);
    for(ii=0; ii < l_nlayers; ii++)
    {
      if(l_layers_list[ii] == mgpp->fdst_layer_id)
      {
        dst_stack_position = ii +1;
	break;
      }
    }
    if(l_layers_list)
    {
      g_free (l_layers_list);
    }
  }

  
  last_step = mgpp->tween_steps;
  total_steps = 1 + mgpp->tween_steps;
  mgpp->master_progress = 0.0;
  mgpp->layer_progress_step = 0.5 / (gdouble)(MAX(1, last_step));

  if(mgpp->render_mode == GAP_MORPH_RENDER_MODE_WARP)
  {
    /* warp_only mode (equal layer)
     * must do one extra step to render the final distortion step
     * (in morph mode this makes no sense, because this extra step
     *  would deliver the destination layer again as result)
     */
    last_step = mgpp->tween_steps;
    total_steps = mgpp->tween_steps;

    mgpp->layer_progress_step = 1.0 / (gdouble)(MAX(1, last_step));
  }

  
  for(current_step=1; current_step <= last_step; current_step++)
  {
    new_layer_id = p_create_morph_tween_frame(total_steps
                          ,current_step
                          ,mgpp
			  ,mlayers
                          );
    if(new_layer_id < 0)
    {
      continue;
      break;
    }
    else
    {
      gint32 tween_image_id;
      gint   l_src_offset_x, l_src_offset_y;    /* layeroffsets as they were in src_image */
      
      if(mgpp->create_tween_layers)
      {
	cp_layer_id = gap_layer_copy_to_dest_image(dst_image_id
                                     ,new_layer_id
                                     ,100.0           /* Opacity */
                                     ,0               /* NORMAL */
                                     ,&l_src_offset_x
                                     ,&l_src_offset_y
                                     );
	gimp_image_add_layer(dst_image_id
                            , cp_layer_id
                            , dst_stack_position
                            );
      }
      else
      {
        gint ii;

        /* replace content of current dst_layer[ii] by content of new_layer_id */
        ii = mlayers->dst1_idx - (current_step -1);
	gap_layer_copy_content(mlayers->dst_layers[ii], new_layer_id);
      }
      tween_image_id = gimp_drawable_get_image(new_layer_id);
      gimp_image_delete(tween_image_id);
    }


    if(mgpp->do_progress)
    {
      mgpp->master_progress = (gdouble)current_step / (gdouble)(MAX(1, last_step));
      gimp_progress_update(mgpp->master_progress);
    }

    
  }
  return(cp_layer_id);
  
}  /* end gap_morph_execute */
