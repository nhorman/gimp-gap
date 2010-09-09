// TODOS: levels of non-black edge pixels are too dark (stretch range...)
//        check colordiff (that seems not work properly on draft locate test image coords x:25 y:23  and x:25 y:24 !!!

/* gap_edge_detection.h
 *    by hof (Wolfgang Hofer)
 *  2010/08/08
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
 * version 2.7.0;             hof: created
 */

#ifndef _GAP_EDGE_DETECTION_H
#define _GAP_EDGE_DETECTION_H

/* SYTEM (UNIX) includes */
#include <stdio.h>
#include <stdlib.h>

/* GIMP includes */
#include "gtk/gtk.h"
#include "libgimp/gimp.h"



/* ----------------------------------------
 * gap_edgeDetection
 * ----------------------------------------
 *
 * returns the drawable id of a newly created channel
 * representing edges of the specified image.
 *
 * black pixels indicate areas of same or similar colors,
 * white indicates sharp edges.
 *
 */
gint32 gap_edgeDetection(gint32  refDrawableId
  , gdouble edgeColordiffThreshold
  , gint32 *countEdgePixels
  );




#endif
