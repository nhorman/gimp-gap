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
     void *next_selected;


     
  } GapMorphWorkPoint;


typedef struct GapMorphGlobalParams  { /* nickname: mgpp */
  GimpRunMode  run_mode;
  gint32       image_id;

  gint32              tween_steps;
  gint32              fdst_layer_id;
  gint32              osrc_layer_id;
  GapMorphWorkPoint  *master_wp_list;


  char                workpoint_file[1024];

  gboolean            warp_and_morph_mode;
  gboolean            do_progress;
  gdouble             master_progress;
  gdouble             layer_progress_step;


} GapMorphGlobalParams;


#endif

