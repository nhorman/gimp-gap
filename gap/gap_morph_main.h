/*  gap_morph_main.h
 *
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* revision history:
 * version 1.3.15a; 2004/02/12  hof: created
 */

#ifndef _GAP_MORPH_MAIN_H
#define _GAP_MORPH_MAIN_H

#include "libgimp/gimp.h"
#include "gap_lib.h"
#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include "gap_pview_da.h"

/* render_mode Radio Buttons */
#define   GAP_MORPH_RENDER_MODE_MORPH    0
#define   GAP_MORPH_RENDER_MODE_WARP     1    

#define   GAP_MORPH_WORKPOINT_FILENAME_MAX_LENGTH     1024 

typedef struct GapMorphWorkPoint { /* nickname: wp */
     gdouble fdst_x;   /* final dest koord (as set by user for last dest. frame) */
     gdouble fdst_y;
     gdouble osrc_x;   /* start source koord (as set by user for the 1.st frame) */
     gdouble osrc_y;

     gdouble dst_x;    /* koord trans */
     gdouble dst_y;
     gdouble src_x;    /* osrc_x scaled to koords of current (dest) frame */
     gdouble src_y;

     void *next;
     
     /* for calculations per pixel */
     gdouble warp_weight;
     gdouble gravity;
     gdouble sqr_dist;
     gdouble dist;
     gdouble inv_dist;    /* 1 / sqr_distance */
     gdouble angle_deg;
     void *next_selected;
     
  } GapMorphWorkPoint;


typedef struct GapMorphGlobalParams  { /* nickname: mgpp */
  GimpRunMode  run_mode;
  gint32       image_id;

  gint32              tween_steps;
  gint32              fdst_layer_id;
  gint32              osrc_layer_id;


  GapMorphWorkPoint  *master_wp_list;

  char                workpoint_file_lower[GAP_MORPH_WORKPOINT_FILENAME_MAX_LENGTH];
  char                workpoint_file_upper[GAP_MORPH_WORKPOINT_FILENAME_MAX_LENGTH];

  gboolean            create_tween_layers;       /* FALSE: operate on existing layers only */
  gboolean            multiple_pointsets;        /* FALSE: use the default workpointset master_wp_list
                                                  * TRUE: use lower_wp_list and upper_wp_list
						  *       foreach handled frame the
						  *       lower and upper list are fetched from 
						  *       best matching workpointfile.
						  *       (using the numberpart of the filename)
						  */
  gboolean            use_fast_wp_selection;           /* TRUE: */
  gboolean            use_gravity;           /* TRUE: */
  gdouble             gravity_intensity;     /* 1.0 upto 5 (gravity power) */
  gdouble             affect_radius;         /* distortion pixelradius (0 == no gravity) */

  gint32              render_mode;
  gboolean            do_progress;
  gdouble             master_progress;
  gdouble             layer_progress_step;


} GapMorphGlobalParams;

typedef struct GapMorphWarpCoreAPI  { /* nickname: wcap */
  GapMorphWorkPoint *wp_list;
  gboolean      use_fast_wp_selection;
  gboolean      use_gravity;
  gdouble       gravity_intensity;
  gdouble       affect_radius;         /* distortion pixelradius (0 == no gravity) */
  gdouble       sqr_affect_radius;
  
  
  gdouble       scale_x;
  gdouble       scale_y;
  gboolean      printf_flag;
  gint          num_sectors;  /* 8 (fast) or 24 quality algorithm */
  
}  GapMorphWarpCoreAPI;

#endif

