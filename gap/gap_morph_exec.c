#define K_GRAVITY_POWER 2
#define K_GRAVITY_RADIUS 125
//#define K_AFFECT_POWER 1
#define K_AFFECT_RADIUS  125

#define GAP_MORPH_SECTORS  8
#define GAP_MORPH_SECTOR_TABSIZE  16
#define GAP_MORPH_AFFECT_FACTOR  0

// TO check:
//    p_pixel_warp:  // need better algorithm. the current implementation
//                   // produces rippled distortions between workpoints

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
 * gimp    1.3.26a; 2004/04/07  hof: created
 */

#include "config.h"

/* SYTEM (UNIX) includes */
#include "stdio.h"
#include "math.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* GIMP includes */
#include "gtk/gtk.h"
#include "libgimp/gimp.h"

/* GAP includes */
#include "gap-intl.h"
#include "gap_image.h"
#include "gap_morph_main.h"
#include "gap_morph_exec.h"
#include "gap_layer_copy.h"

extern int gap_debug;

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
static inline gint32      p_calc_sector(GapMorphWorkPoint *wp, gint32 in_x, gint32 in_y);
static inline gdouble     p_calc_sqr_distance(GapMorphWorkPoint *wp, gint32 in_x, gint32 in_y);
static inline gdouble     p_calc_distance(GapMorphWorkPoint *wp, gint32 in_x, gint32 in_y);
static void               p_pixel_warp_core(GapMorphWorkPoint *wp_list
        			      , gint32        in_x
        			      , gint32        in_y
        			      , gdouble       scale_x
        			      , gdouble       scale_y
        			      , gdouble *out_pick_x
				      , gdouble *out_pick_y
				      , gboolean      printf_flag
        			      );
static void               p_pixel_warp(GapMorphWorkPoint *wp_list
                                      , gint32        in_x
                                      , gint32        in_y
                                      , GimpPixelFetcher *pixfet
				      , GimpDrawable     *drawable
                                      , guchar           *pixel
                                      , gint              bpp
                                      , gdouble           scale_x
                                      , gdouble           scale_y
                                      );
GapMorphWorkPoint *       p_calculate_work_point_movement(gdouble total_steps
                               ,gdouble current_step
                               ,GapMorphGlobalParams *mgpp
                               ,gint32 curr_width
                               ,gint32 curr_height
                               ,gboolean forward_move
                               );

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
static  GapMorphExePickCache gloabl_pick_cache[GAP_MORPH_PCK_CACHE_SIZE];
static  gint gloabl_pick_cache_idx;

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

  l_fp = fopen(filename, "w+");
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
 * gap_moprh_exec_load_workpointfile
 * ----------------------------------
 * return root pointer to the loaded workpoint list in SUCCESS
 * return NULL if load failed.
 */
GapMorphWorkPoint *
gap_moprh_exec_load_workpointfile(const char *filename
                                 , GapMorphGUIParams *mgup
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


  src_layer_width  = gimp_drawable_width(mgup->mgpp->osrc_layer_id);
  src_layer_height = gimp_drawable_height(mgup->mgpp->osrc_layer_id);
  dst_layer_width  = gimp_drawable_width(mgup->mgpp->fdst_layer_id);
  dst_layer_height = gimp_drawable_height(mgup->mgpp->fdst_layer_id);
  src_scale_x = 1.0;
  src_scale_y = 1.0;
  dst_scale_x = 1.0;
  dst_scale_y = 1.0;
  wp_list = NULL;
  wp = NULL;
  if(filename == NULL) return(NULL);

  l_fp = fopen(filename, "r");
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
             mgup->mgpp->tween_steps = MAX(1,l_farr[0]);
           }
           else
           {
             printf("** error file: %s is corrupted (TWEEN-STEPS record requires 1 number)\n"
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
}  /* end gap_moprh_exec_load_workpointfile */



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
//   return (val_from + delta);

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
 *
 * the sector of the workpoint in relation to the center in_x/in_y
 */
static inline gint32
p_calc_sector(GapMorphWorkPoint *wp, gint32 in_x, gint32 in_y)
{
  gint32 dx;
  gint32 dy;
  
  dx = in_x - wp->dst_x;
  dy = in_y - wp->dst_y;
  
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
 * p_calc_sqr_distance
 * ---------------------------------
 * calculate the distance
 * between in_y/in_y and the workpoint
 */
static inline gdouble
p_calc_sqr_distance(GapMorphWorkPoint *wp, gint32 in_x, gint32 in_y)
{
  gdouble adx, ady;

  adx = abs(in_x - wp->dst_x);
  ady = abs(in_y - wp->dst_y);

  return ((adx * adx ) + (ady * ady));
}  /* end p_calc_sqr_distance */

/* ---------------------------------
 * p_calc_distance
 * ---------------------------------
 * calculate the distance
 * between in_y/in_y and the workpoint
 */
static inline gdouble
p_calc_distance(GapMorphWorkPoint *wp, gint32 in_x, gint32 in_y)
{
  gdouble adx, ady;

  adx = abs(in_x - wp->dst_x);
  ady = abs(in_y - wp->dst_y);

  return (sqrt((adx * adx ) + (ady * ady)));
}  /* end p_calc_distance */


/* ---------------------------------
 * p_pixel_warp_core
 * ---------------------------------
 *  the movement vektor for in_x/in_y is calculated by mixing
 *  the movement vektors of the nearest workpoints in 8 sektors
 *  weighted by distances of the workpoint to in_x/in_y
 *
 *  in case there is no workpoint available
 *       the pixel is picked by simply scaling in_x/in_y to ssrc koords
 *
 */
static void
p_pixel_warp_core(GapMorphWorkPoint *wp_list
            , gint32        in_x
            , gint32        in_y
            , gdouble       scale_x
            , gdouble       scale_y
            , gdouble *out_pick_x
	    , gdouble *out_pick_y
	    , gboolean      printf_flag
            )
{
  register GapMorphWorkPoint *wp;
  GapMorphWorkPoint *wp_sektor_tab[GAP_MORPH_SECTOR_TABSIZE];
  gdouble            vektor_x;  /* movement vektor for the current point at in_x/in_y */
  gdouble            vektor_y;
  gdouble            new_vektor_x;
  gdouble            new_vektor_y;
  gdouble            sqr_dist;
  gdouble            weight;
  gdouble            sum_inv_dist;
  gboolean           direct_hit_flag;
  gdouble           sqr_affect_radius;
  gdouble           affect_factor;
  gdouble           sqr_gravity_radius;
  gdouble           gravity_power;
  gint32            sek_idx;


  sqr_affect_radius = (K_AFFECT_RADIUS * K_AFFECT_RADIUS);
  gravity_power = K_GRAVITY_POWER;
  sqr_gravity_radius = (K_GRAVITY_RADIUS * K_GRAVITY_RADIUS);
  affect_factor = GAP_MORPH_AFFECT_FACTOR;

  
  /* reset sektor tab */
  for(sek_idx=0; sek_idx < GAP_MORPH_SECTOR_TABSIZE; sek_idx++)
  {
    wp_sektor_tab[sek_idx] = NULL;
  }
  
  vektor_x = 0;
  vektor_y = 0;
  sum_inv_dist = 0;
  direct_hit_flag = FALSE;

  /* loop1 calculate square distance, 
   * check for direct hits
   * and build sector table (nearest workpoints foreach sector
   */
  for(wp=wp_list; wp != NULL; wp = (GapMorphWorkPoint *)wp->next)
  {
    sqr_dist = p_calc_sqr_distance(wp, in_x, in_y);
    wp->sqr_dist = sqr_dist;
    
    if(sqr_dist <= sqr_affect_radius)
    {
      if(sqr_dist < 1)
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
        GapMorphWorkPoint *wp_sek;
	
	sek_idx = p_calc_sector(wp, in_x, in_y);
        wp_sek = wp_sektor_tab[sek_idx];
	
	if(wp_sek)
	{
	  /* there is already a workpoint for this sektor */
	  if(sqr_dist <= wp_sek->sqr_dist)
	  {
            if(FALSE)  /* accept 2.nd nearest point too */
	    {
	      GapMorphWorkPoint *wp_sek2;

              wp_sek2 = wp_sektor_tab[GAP_MORPH_SECTORS + sek_idx];
	      if(wp_sek2)
	      {
		sum_inv_dist -= wp_sek2->inv_dist;
	      }
	      
              /* save the existing wp_sek as 2.nd nearest point for this sektor */
	      wp_sektor_tab[GAP_MORPH_SECTORS + sek_idx] = wp_sek;
	    }
	    else
	    {
	      sum_inv_dist -= wp_sek->inv_dist;
	    }
	  
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
	
	wp->dist = sqrt(sqr_dist);
	
	wp->inv_dist = 1.0 / (wp->dist + (affect_factor * sqr_dist));
	sum_inv_dist += wp->inv_dist;
	
	
      }
    }
  }

  if(!direct_hit_flag)
  {
    /* loop2: build result vektor as weightened mix of all workpoint vektors */
    for(sek_idx=0; sek_idx < GAP_MORPH_SECTOR_TABSIZE; sek_idx++)
    {
      wp = wp_sektor_tab[sek_idx];
      if(wp)
      {
        wp->gravity = 1.0;
	if(sqr_gravity_radius > 1)
	{
	  wp->gravity -= pow((MIN(wp->sqr_dist,sqr_gravity_radius) / sqr_gravity_radius) , gravity_power);
	}

	new_vektor_x = (wp->src_x - wp->dst_x);
	new_vektor_y = (wp->src_y - wp->dst_y);

	weight = (wp->inv_dist / sum_inv_dist) * wp->gravity;
	vektor_x += (new_vektor_x * weight);
	vektor_y += (new_vektor_y * weight);
	if(printf_flag)
	{
          wp->warp_weight = weight;
          printf("wp->src_x/y:  %.3f / %.3f  wp->dst_x/y: %.3f / %.3f dist:%.3f weight: %.7f vek: %.3f / %.3f \n"
        	, (float)wp->src_x
        	, (float)wp->src_y
        	, (float)wp->dst_x
        	, (float)wp->dst_y
        	, (float)wp->dist
        	, (float)weight
        	, (float)vektor_x
        	, (float)vektor_y
        	);
	}

      }
    }
  }

  *out_pick_x = (in_x + vektor_x) * scale_x;
  *out_pick_y = (in_y + vektor_y) * scale_y;

}  /* end p_pixel_warp_core */






/* -----------------------------------
 * gap_morph_exec_get_warp_pick_koords
 * -----------------------------------
 */
void
gap_morph_exec_get_warp_pick_koords(GapMorphWorkPoint *wp_list
                                      , gint32        in_x
                                      , gint32        in_y
				      , gdouble       scale_x
				      , gdouble       scale_y
				      , gdouble *out_pick_x
				      , gdouble *out_pick_y
                                      )
{
  register GapMorphWorkPoint *wp;
  gdouble            pick_x;
  gdouble            pick_y;
  gdouble            sum_pick_x;
  gdouble            sum_pick_y;
  gint xx;
  gint yy;
  gint nn;

  for(wp=wp_list; wp != NULL; wp = (GapMorphWorkPoint *)wp->next)
  {
    wp->warp_weight = -1;  /* for debug display in warp_info_label */
    wp->dist = p_calc_distance(wp, in_x, in_y); /* for debug display in warp_info_label */
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
  for(xx=MAX(0,in_x -3); xx <= in_x +3; xx+=3)
  {
    for(yy=MAX(0,in_y -3); yy <= in_y +3; yy+=3)
    {
      p_pixel_warp_core(wp_list
                       ,xx
                       ,yy
                       ,scale_x
                       ,scale_y
                       ,&pick_x
                       ,&pick_y
		       ,FALSE         /* printf_flag */
		       );
       sum_pick_x += pick_x;		       
       sum_pick_y += pick_y;
       nn++;		       
    }
  }
  
  pick_x = sum_pick_x / (gdouble)nn;
  pick_y = sum_pick_y / (gdouble)nn;


  p_pixel_warp_core(wp_list
                   ,in_x
                   ,in_y
                   ,scale_x
                   ,scale_y
                   ,out_pick_x
                   ,out_pick_y
		   ,TRUE         /* printf_flag */
		   );



  *out_pick_x = pick_x;
  *out_pick_y = pick_y;

  printf("pick_x/y :  %.3f / %.3f\n"
            , (float)*out_pick_x
            , (float)*out_pick_y
            );

}  /* end gap_morph_exec_get_warp_pick_koords */



/* ---------------------------------
 * p_pixel_warp
 * ---------------------------------
 */
static void
p_pixel_warp(GapMorphWorkPoint *wp_list
            , gint32        in_x
            , gint32        in_y
            , GimpPixelFetcher *pixfet
	    , GimpDrawable     *drawable
            , guchar           *pixel
            , gint              pixel_bpp
            , gdouble           scale_x
            , gdouble           scale_y
            )
{
  gdouble            pick_x;
  gdouble            pick_y;
  gdouble            sum_pick_x;
  gdouble            sum_pick_y;
  gint xx;
  gint yy;
  gint nn;

#ifdef GAP_MORPH_USE_SIMPLE_PIXEL_WARP
        p_pixel_warp_core(wp_list
                       ,in_x
                       ,in_y
                       ,scale_x
                       ,scale_y
                       ,&pick_x
                       ,&pick_y
		       ,FALSE         /* printf_flag */
		       );

#else
  /* the implementation of the   p_pixel_warp_core
   * produces unwanted hard edges at those places where
   * the selection of relevant near workpoints
   * switches to another set of workpoints with
   * signifikant different movement settings.
   * This workaround does multiple picks 
   * and uses the average pick koordinates to compensate
   * this effect.
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
        if ((gloabl_pick_cache[ii].valid)
	&&  (gloabl_pick_cache[ii].xx == xx)
	&&  (gloabl_pick_cache[ii].yy == yy))
	{
	  pick_x = gloabl_pick_cache[ii].pick_x;
	  pick_y = gloabl_pick_cache[ii].pick_y;
	  
	  /* printf("** CACHE HIT\n"); */
	  break;
	}
      }
      if(ii == GAP_MORPH_PCK_CACHE_SIZE)
      {
        /* printf("** must do pixel warp\n"); */
        p_pixel_warp_core(wp_list
                       ,xx
                       ,yy
                       ,scale_x
                       ,scale_y
                       ,&pick_x
                       ,&pick_y
		       ,FALSE         /* printf_flag */
		       );
         /* save pick koords in cache table */
	 gloabl_pick_cache[gloabl_pick_cache_idx].xx     = xx;
	 gloabl_pick_cache[gloabl_pick_cache_idx].yy     = yy;
	 gloabl_pick_cache[gloabl_pick_cache_idx].pick_x = pick_x;
	 gloabl_pick_cache[gloabl_pick_cache_idx].pick_y = pick_y;
	 gloabl_pick_cache[gloabl_pick_cache_idx].valid  = TRUE;
	 gloabl_pick_cache_idx++;
	 if(gloabl_pick_cache_idx >= GAP_MORPH_PCK_CACHE_SIZE)
	 {
	   gloabl_pick_cache_idx = 0;
	 }
       }
       sum_pick_x += (pick_x - ((in_x - xx) * scale_x));
       sum_pick_y += (pick_y - ((in_y - yy) * scale_y));
       nn++;		       
    }
  }
  pick_x = sum_pick_x / (gdouble)nn;
  pick_y = sum_pick_y / (gdouble)nn;

#endif
  
  
  p_bilinear_get_pixel (pixfet, drawable, pick_x, pick_y, pixel, pixel_bpp);
}  /* end p_pixel_warp */


/* ---------------------------------
 * p_calculate_work_point_movement
 * ---------------------------------
 * return the newly created workpoint list
 * the list contains duplicates of the input workpoint list
 * with transformed point koordinates, according to current movement
 * step.
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
                               ,GapMorphGlobalParams *mgpp
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
    scalex = (gdouble)curr_width  / (gdouble)gimp_drawable_width(mgpp->osrc_layer_id);
    scaley = (gdouble)curr_height / (gdouble)gimp_drawable_height(mgpp->osrc_layer_id);
  }
  else
  {
    scalex = (gdouble)curr_width  / (gdouble)gimp_drawable_width(mgpp->fdst_layer_id);
    scaley = (gdouble)curr_height / (gdouble)gimp_drawable_height(mgpp->fdst_layer_id);
  }
  
  wp_list = NULL;
  wp_elem_prev = NULL;
  
  for(wp_elem_orig = mgpp->master_wp_list; wp_elem_orig != NULL; wp_elem_orig = (GapMorphWorkPoint *)wp_elem_orig->next)
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
p_layer_warp_move (GapMorphWorkPoint *wp_list
                  , gint32 src_layer_id
                  , gint32 dst_layer_id
		  , GapMorphGlobalParams *mgpp
                  )
{
  GimpDrawable *src_drawable;
  GimpDrawable *dst_drawable;
  GimpPixelFetcher *src_pixfet;
  GimpPixelRgn    dstPR;
  gpointer        pr;
  guchar         *buf_ptr;
  guchar         *pixel_ptr;
  guint32         l_row;
  guint32         l_col;
  gdouble         scale_x;
  gdouble         scale_y;
  gdouble         l_max_progress;
  gdouble         l_progress;
  gint ii;

  /* clear the global cache for picking koordinates */
  for(ii=0; ii < GAP_MORPH_PCK_CACHE_SIZE; ii++)
  {
     gloabl_pick_cache[ii].valid = FALSE;
  }
  gloabl_pick_cache_idx = 0;

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

  /* init Pixel Fetcher for source drawable  */
  src_pixfet = gimp_pixel_fetcher_new (src_drawable, FALSE /*shadow*/);
  if(src_drawable == NULL)                   { return; }

  buf_ptr = g_malloc(dst_drawable->width * dst_drawable->bpp);
  if(buf_ptr == NULL)                        { return; }
  
  
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

     if(gap_debug)
     {
       printf("PR: w: %d  h:%d     x:%d  y:%d\n"
	   , (int)dstPR.w
	   , (int)dstPR.h
	   , (int)dstPR.x
	   , (int)dstPR.y
	   );
     }

     dest = dstPR.data;
     for (y = 0; y < dstPR.h; y++)
     {
        pixel_ptr = dest;
	l_row = dstPR.y + y;
	
        for (x = 0; x < dstPR.w; x++)
	{
	   l_col = dstPR.x + x;
           p_pixel_warp(wp_list, l_col, l_row
                   , src_pixfet
		   , src_drawable
                   , pixel_ptr
                   , dst_drawable->bpp
                   , scale_x, scale_y
                   );
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
  g_free(buf_ptr);

}   /* end p_layer_warp_move */


/* ---------------------------------
 * p_create_morph_tween_frame
 * ---------------------------------
 * return the newly created tween morphed layer
 */
gint32
p_create_morph_tween_frame(gint32 total_steps
                          ,gint32 current_step
                          ,GapMorphGlobalParams *mgpp
                          )
{
   GimpDrawable *src_drawable;
   gint32  curr_image_id;  /* a temp image of current layer size */
   gint32  bg_layer_id;
   gint32  top_layer_id;
   gint32  merged_layer_id;
   gint32  curr_width;
   gint32  curr_height;
   gdouble curr_opacity;
   GapMorphWorkPoint *curr_wp_list;
   gint32 src_layer_id;
   gint32 dst_layer_id;
   
   src_layer_id = mgpp->osrc_layer_id;
   dst_layer_id = mgpp->fdst_layer_id;

   src_drawable = gimp_drawable_get (src_layer_id);

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

   /* create the tween frame image */
   curr_image_id = gimp_image_new(curr_width, curr_height, GIMP_RGB);

   /* add empty BG layer */
   if(src_drawable->bpp < 3)
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
   
   /* workpoint settings for sc_layer */
   curr_wp_list = p_calculate_work_point_movement((gdouble)total_steps
                                                 ,(gdouble)current_step
                                                 ,mgpp
                                                 ,curr_width
                                                 ,curr_height
                                                 ,TRUE    /* forward_move */
                                                 );
                                                 
   /* warp the BG layer */
   p_layer_warp_move ( curr_wp_list
                     , src_layer_id
                     , bg_layer_id
		     , mgpp
                     );

   gap_morph_exec_free_workpoint_list(&curr_wp_list);

   if(mgpp->warp_and_morph_mode == FALSE)
   {
     /* for warp only mode all is done at this point */
     return(bg_layer_id);
   }

   /* workpoint settings for sc_layer */
   curr_wp_list = p_calculate_work_point_movement((gdouble)total_steps
                                                 ,(gdouble)current_step
                                                 ,mgpp
                                                 ,curr_width
                                                 ,curr_height
                                                 ,FALSE /* no forward move */
                                                 );
   
   /* warp the TOP layer */
   p_layer_warp_move ( curr_wp_list
                     , dst_layer_id
                     , top_layer_id
		     , mgpp
                     );

   gap_morph_exec_free_workpoint_list(&curr_wp_list);


// XXXX /* debug show image and stop before merge */
//gimp_display_new(curr_image_id);
//return -1;

   /* merge BG and TOP Layer (does mix opacity according to current step) */
   merged_layer_id = 
   gap_image_merge_visible_layers(curr_image_id, GIMP_EXPAND_AS_NECESSARY);

   return(merged_layer_id);

}  /* end p_create_morph_tween_frame  */



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
  
  if(gap_debug) printf("gap_morph_execute: START src_layer:%d dst_layer:%d  tween_steps:%d\n"
         ,(int)mgpp->osrc_layer_id
         ,(int)mgpp->fdst_layer_id
         ,(int)mgpp->tween_steps
         );

  mgpp->warp_and_morph_mode = TRUE;
  if(mgpp->osrc_layer_id == mgpp->fdst_layer_id)
  {
     /* for identical layers we do only warp
      * (fading identical images makes no sense)
      */
    mgpp->warp_and_morph_mode = FALSE;
  }

  if(mgpp->do_progress)
  {
    if(mgpp->warp_and_morph_mode)
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

  if(mgpp->warp_and_morph_mode == FALSE)
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
