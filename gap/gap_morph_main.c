/*  gap_morph_main.c
 *  creation of morphing animations (transform source image into des. image) by Wolfgang Hofer
 *  2004/02/11
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* Revision history
 * version 1.3.26a; 2004/02/12  hof: created
 */

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gap_morph_main.h"
#include "gap_morph_exec.h"
#include "gap_morph_dialog.h"
#include "gap_pview_da.h"

/* for pointfile loader (workaround) */
#include "gap_mov_dialog.h"
#include "gap_mov_exec.h"



#include "gap-intl.h"

/* Defines */
#define PLUG_IN_NAME        "plug_in_gap_morph_layers"
#define PLUG_IN_PRINT_NAME  "Morph Layers"
#define PLUG_IN_VERSION     "v1.3.26 (2004/02/14)"
#define PLUG_IN_IMAGE_TYPES "RGBA, GRAYA"
#define PLUG_IN_AUTHOR      "Wolfgang Hofer (hof@gimp.org)"
#define PLUG_IN_COPYRIGHT   "Wolfgang Hofer"


int gap_debug = 0;  /* 1 == print debug infos , 0 dont print debug infos */

static GapMorphGlobalParams global_params =
{
  GIMP_RUN_INTERACTIVE
, -1          /* gint32  image_id */

, 1           /* gint32  tween_steps */
, -1          /* gint32  fdst_layer_id */
, -1          /* gint32  osrc_layer_id */
, NULL        /* GapMorphWorkPoint  *master_wp_list */

, "\0"        /* char workpoint_file_lower[1024] */
, "\0"        /* char workpoint_file_upper[1024] */
, TRUE        /* gboolean            create_tween_layers */
, FALSE       /* gboolean            multiple_pointsets */
, TRUE        /* gboolean            use_fast_wp_selection */
, FALSE       /* gboolean            use_gravity */
, 2.0         /* gdouble             gravity_intensity */
, 100.0       /* gdouble             affect_radius */
, GAP_MORPH_RENDER_MODE_MORPH  /* gint32 render_mode */
, FALSE       /* gboolean            do_progress */
, 0.0         /* gdouble             master_progress */
, 0.0         /* gdouble             layer_progress_step */
};


static void  query (void);
static void  run (const gchar *name,
                  gint nparams,              /* number of parameters passed in */
                  const GimpParam * param,   /* parameters passed in */
                  gint *nreturn_vals,        /* number of parameters returned */
                  GimpParam ** return_vals); /* parameters to be returned */


/* Global Variables */
GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,   /* init_proc  */
  NULL,   /* quit_proc  */
  query,  /* query_proc */
  run     /* run_proc   */
};

// XXXXXXXXX TODO: PDB API for morph-points
static GimpParamDef in_args[] = {
                  { GIMP_PDB_INT32,    "run_mode", "Interactive, non-interactive"},
                  { GIMP_PDB_IMAGE,    "image", "Input image" },
                  { GIMP_PDB_DRAWABLE, "src_drawable", "source drawable (usually a layer)"},
                  { GIMP_PDB_DRAWABLE, "dst_drawable", "destination drawable (usually a layer)"},
                  { GIMP_PDB_INT32,    "tween_steps", "number of layers to create between src_layer and dst_layer"},
  };

/* Functions */


/* ============================================================================
 * p_sscan_flt_numbers
 * ============================================================================
 * scan the blank seperated buffer for 2 integer and 13 float numbers.
 * always use "." as decimalpoint in the float numbers regardless to LANGUAGE settings
 * return a counter that tells how many numbers were scanned successfully
 */
static gint
p_sscan_flt_numbers(gchar   *buf
                  , gdouble *farr
		  , gint     farr_max
		  )
{
  gint  l_cnt;
  gchar *nptr;
  gchar *endptr;

  l_cnt =0;
  nptr  = buf;
  while(l_cnt < farr_max)
  {
    endptr = nptr;
    farr[l_cnt] = g_ascii_strtod(nptr, &endptr);
    if(nptr == endptr)
    {
      break;  /* pointer was not advanced because no valid floatnumber was scanned */
    }
    nptr = endptr;

    l_cnt++;  /* count successful scans */
  }

  return (l_cnt);
}  /* end p_sscan_flt_numbers */


/* ============================================================================
 * p_load_move_path_pointfile
 * ============================================================================
 * return 0 if Load was OK,
 * return -2 when load has read inconsistent pointfile
 *           and the pointtable needs to be reset (dialog has to call p_reset_points)
 */
gint
p_load_move_path_pointfile(char *filename, GapMovValues *pvals)
{
#define POINT_REC_MAX 512
#define MAX_NUMVALUES_PER_LINE 17
  FILE   *l_fp;
  gint    l_idx;
  char    l_buff[POINT_REC_MAX +1 ];
  char   *l_ptr;
  gint    l_cnt;
  gint    l_rc;
  gint    l_i1, l_i2;
  gdouble l_farr[MAX_NUMVALUES_PER_LINE];


  l_rc = -1;
  if(filename == NULL) return(l_rc);

  l_fp = fopen(filename, "r");
  if(l_fp != NULL)
  {
    l_idx = -1;
    while (NULL != fgets (l_buff, POINT_REC_MAX, l_fp))
    {
       /* skip leading blanks */
       l_ptr = l_buff;
       while(*l_ptr == ' ') { l_ptr++; }

       /* check if line empty or comment only (starts with '#') */
       if((*l_ptr != '#') && (*l_ptr != '\n') && (*l_ptr != '\0'))
       {
         l_cnt = p_sscan_flt_numbers(l_ptr, &l_farr[0], MAX_NUMVALUES_PER_LINE);
	 l_i1 = (gint)l_farr[0];
	 l_i2 = (gint)l_farr[1];
         if(l_idx == -1)
         {
           if((l_cnt < 2) || (l_i2 > GAP_MOV_MAX_POINT) || (l_i1 > l_i2))
           {
             break;
            }
           pvals->point_idx     = l_i1;
           pvals->point_idx_max = l_i2 -1;
           l_idx = 0;
         }
         else
         {
	   gdouble num_optional_params;
	   gint    key_idx;
           /* the older format used in GAP.1.2 has 6 or 7 integer numbers per line
            * and should be compatible and readable by this code.
            *
            * the new format has 2 integer values (p_x, p_y)
            * and 5 float values (w_resize, h_resize, opacity, rotation, feather_radius)
	    * and 1 int value num_optional_params (telling how much will follow)
            * the rest of the line is optional
            *  8  additional float values (transformation factors) 7th upto 14th parameter
            *  1  integer values (keyframe) as 7th parameter
            *         or as 15th parameter (if transformation factors are present too)
            */
           if((l_cnt != 6) && (l_cnt != 7)   /* for compatibility to old format */
	   && (l_cnt != 8) && (l_cnt != 9) && (l_cnt != 16) && (l_cnt != 17))
           {
             /* invalid pointline format detected */
             l_rc = -2;  /* have to call p_reset_points() when called from dialog window */
             break;
           }
           pvals->point[l_idx].keyframe_abs = 0;
           pvals->point[l_idx].keyframe = 0;
           pvals->point[l_idx].p_x      = l_i1;
           pvals->point[l_idx].p_y      = l_i2;
           pvals->point[l_idx].ttlx     = 1.0;
           pvals->point[l_idx].ttly     = 1.0;
           pvals->point[l_idx].ttrx     = 1.0;
           pvals->point[l_idx].ttry     = 1.0;
           pvals->point[l_idx].tblx     = 1.0;
           pvals->point[l_idx].tbly     = 1.0;
           pvals->point[l_idx].tbrx     = 1.0;
           pvals->point[l_idx].tbry     = 1.0;
           pvals->point[l_idx].w_resize = l_farr[2];
           pvals->point[l_idx].h_resize = l_farr[3];
           pvals->point[l_idx].opacity  = l_farr[4];
           pvals->point[l_idx].rotation = l_farr[5];
           pvals->point[l_idx].sel_feather_radius = 0.0;
           if(l_cnt >= 8)
	   {
             pvals->point[l_idx].sel_feather_radius = l_farr[6];
	     num_optional_params = l_farr[7];
	   }
           if(l_cnt >= 16)
	   {
             pvals->point[l_idx].ttlx = l_farr[8];
             pvals->point[l_idx].ttly = l_farr[9];
             pvals->point[l_idx].ttrx = l_farr[10];
             pvals->point[l_idx].ttry = l_farr[11];
             pvals->point[l_idx].tblx = l_farr[12];
             pvals->point[l_idx].tbly = l_farr[13];
             pvals->point[l_idx].tbrx = l_farr[14];
             pvals->point[l_idx].tbry = l_farr[15];
	   }
	   key_idx = -1;
	   if(l_idx > 0)
	   {
	     switch(l_cnt)
	     {
	       case 7:
	           key_idx = 6; /* for compatibilty with old format */
	           break;
	       case 9:
	           key_idx = 8;
	           break;
	       case 17:
	           key_idx = 16;
	           break;
	     }
	   }
	   if(key_idx > 0)
	   {
             pvals->point[l_idx].keyframe = l_farr[key_idx];
             pvals->point[l_idx].keyframe_abs = 0; // gap_mov_exec_conv_keyframe_to_abs(l_farr[key_idx], pvals);
	   }
           l_idx ++;
         }

         if(l_idx > pvals->point_idx_max) break;
       }
    }

    fclose(l_fp);
    if(l_idx >= 0)
    {
       l_rc = 0;  /* OK if we found at least one valid Controlpoint in the file */
    }
  }
  return (l_rc);
}


/* --------------------------
 * p_load_test_morph_points
 * --------------------------
 * load morph points from 2 pointfiles (saved by move path)
 * this is a temporary workaround until we have a GUI
 * to enter morph points
 */
GapMorphWorkPoint *
p_load_test_morph_points(char *src_filename, char *dst_filename)
{
  GapMovValues src_pvals;
  GapMovValues dst_pvals;
  GapMovValues *spvals;
  GapMovValues *dpvals;
  GapMorphWorkPoint *wp;
  GapMorphWorkPoint *wp_prev;
  GapMorphWorkPoint *wp_list;
  gint   src_ok;
  gint   dst_ok;
  gint   l_idx;
  
  spvals = &src_pvals;
  dpvals = &dst_pvals;
  wp_prev = NULL;
  wp_list = NULL;
  
  src_ok = p_load_move_path_pointfile(src_filename, &src_pvals);
  dst_ok = p_load_move_path_pointfile(dst_filename, &dst_pvals);
  
  if(spvals->point_idx_max != dpvals->point_idx_max)
  {
    printf("*** ERROR number of points differs!\n");
    return(NULL);
  }
  
  for(l_idx=0; l_idx <= spvals->point_idx_max; l_idx++)
  {
    wp = g_new(GapMorphWorkPoint, 1);
    wp->next = NULL;
    wp->fdst_x = dpvals->point[l_idx].p_x;
    wp->fdst_y = dpvals->point[l_idx].p_y;
    wp->osrc_x = spvals->point[l_idx].p_x;
    wp->osrc_y = spvals->point[l_idx].p_y;

    wp->dst_x = wp->fdst_x;
    wp->dst_y = wp->fdst_y;
    wp->src_x = wp->osrc_x;
    wp->src_y = wp->osrc_y;
    
    if(wp_prev)
    {
      wp_prev->next = wp;
    }
    else
    {
      wp_list = wp;
    }
    wp_prev = wp;
    
    printf("WP: osrc: %03.2f / %03.2f   fdst: %03.2f / %03.2f src: %03.2f / %03.2f   dst: %03.2f / %03.2f\n"
          ,(float)wp->osrc_x
          ,(float)wp->osrc_y
          ,(float)wp->fdst_x
          ,(float)wp->fdst_y
          ,(float)wp->src_x
          ,(float)wp->src_y
          ,(float)wp->dst_x
          ,(float)wp->dst_y
	  );
  }
  
  return(wp_list);
  
}  /* end p_load_test_morph_points */



MAIN ()

static void query (void)
{
  gimp_plugin_domain_register (GETTEXT_PACKAGE, LOCALEDIR);

  /* the actual installation of the plugin */
  gimp_install_procedure (PLUG_IN_NAME,
                          "Image Layer Morphing",
                          "This plug-in creates new layers by transforming the src_drawable to dst_drawable, "
                          "the transformation is a combination of warping and blending, commonly known"
                          "as morphing. the tween_steps parameter controls how much new layers to create."
                          "source and destination may differ in size.",
                          PLUG_IN_AUTHOR,
                          PLUG_IN_COPYRIGHT,
                          PLUG_IN_VERSION,
                          N_("<Image>/Video/Morph..."),
                          PLUG_IN_IMAGE_TYPES,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (in_args),
                          0,        /* G_N_ELEMENTS (out_args) */
                          in_args,
                          NULL      /* out_args */
                          );

}

static void
run (const gchar *name,          /* name of plugin */
     gint nparams,               /* number of in-paramters */
     const GimpParam * param,    /* in-parameters */
     gint *nreturn_vals,         /* number of out-parameters */
     GimpParam ** return_vals)   /* out-parameters */
{
  const gchar *l_env;
  gint32       image_id = -1;
  gint32       drawable_id = -1;
  GapMorphGlobalParams  *mgpp = &global_params;
  gboolean      run_flag;

  /* Get the runmode from the in-parameters */
  GimpRunMode run_mode = param[0].data.d_int32;

  /* status variable, use it to check for errors in invocation usualy only
   * during non-interactive calling
   */
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;

  /* always return at least the status to the caller. */
  static GimpParam values[1];

  INIT_I18N();


  l_env = g_getenv("GAP_DEBUG");
  if(l_env != NULL)
  {
    if((*l_env != 'n') && (*l_env != 'N')) gap_debug = 1;
  }

  if(gap_debug) fprintf(stderr, "\n\nDEBUG: run %s\n", name);

  run_flag = FALSE;
  /* initialize the return of the status */
  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
  *nreturn_vals = 1;
  *return_vals = values;


  /* get image */
  image_id = param[1].data.d_int32;
  drawable_id = param[2].data.d_drawable;
  mgpp->do_progress = FALSE;


  switch (run_mode)
  {
    case GIMP_RUN_INTERACTIVE:
      /* Possibly retrieve data from a previous run */
      gimp_get_data (PLUG_IN_NAME, mgpp);
      mgpp->osrc_layer_id          = drawable_id;
      mgpp->fdst_layer_id          = gap_morph_exec_find_dst_drawable(image_id, drawable_id);
      run_flag = gap_morph_dialog(mgpp);
      mgpp->do_progress            = TRUE;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /* check to see if invoked with the correct number of parameters */
      if (nparams == G_N_ELEMENTS (in_args))
      {
          mgpp->osrc_layer_id          = param[2].data.d_drawable;
          mgpp->fdst_layer_id          = param[3].data.d_drawable;
          mgpp->tween_steps            = param[4].data.d_int32;
	  run_flag = TRUE;
      }
      else
      {
        printf("%s: noninteractive call wrong nuber %d of params were passed. (%d params are required)\n"
              , PLUG_IN_NAME
              , (int)nparams
              , (int)G_N_ELEMENTS (in_args)
              );
        status = GIMP_PDB_CALLING_ERROR;
      }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /* Possibly retrieve data from a previous run */
      gimp_get_data (PLUG_IN_NAME, mgpp);
      mgpp->osrc_layer_id          = drawable_id;
      mgpp->fdst_layer_id          = gap_morph_exec_find_dst_drawable(image_id, drawable_id);
      run_flag = gap_morph_dialog(mgpp);
      mgpp->do_progress            = TRUE;
      break;

    default:
      break;
  }

  if ((status == GIMP_PDB_SUCCESS)
  && (run_flag))
  {

    mgpp->image_id = image_id;
    mgpp->run_mode = run_mode;
    
    /* debug init */
//    mgpp->tween_steps = 1;
//    mgpp->master_wp_list = p_load_test_morph_points("/home/hof/tmp/morph_point_src.txt"
//                                                   ,"/home/hof/tmp/morph_point_dst.txt"
//						   );
    gap_morph_execute(mgpp);

    /* Store variable states for next run */
    if (run_mode == GIMP_RUN_INTERACTIVE)
      gimp_set_data (PLUG_IN_NAME, mgpp, sizeof (GapMorphGlobalParams));
  }
  values[0].data.d_status = status;
}       /* end run */


 
