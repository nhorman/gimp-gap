/* gap_mov_render.h
 * 1997.11.06 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * Render utility Procedures for GAP MovePath
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
#ifndef _GAP_MOV_RENDER_H
#define _GAP_MOV_RENDER_H

/* revision history:
 * gimp    1.3.14a; 2003/05/24  hof: created (splitted off from gap_mov_dialog)
 */
 

#include "gap_mov_dialog.h"

gint   p_mov_render(gint32 image_id, t_mov_values *val_ptr, t_mov_current *cur_ptr);
gint32 p_get_flattened_layer (gint32 image_id, GimpMergeType mergemode);
gint   p_mov_fetch_src_frame(t_mov_values *pvals,  gint32 wanted_frame_nr);

#endif
