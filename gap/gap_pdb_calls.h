/*  gap_pdb_calls.h
 *
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
 * version 1.3.5a;  2002/04/20  hof: p_gimp_layer_new_from_drawable. (removed set_drabale)
 * version 1.3.4a;  2002/03/12  hof: removed duplicate wrappers that are available in libgimp too.
 * version 1.2.2b;  2001/12/09  hof: wrappers for tattoo procedures
 * version 1.1.16a; 2000/02/05  hof: path lockedstaus
 * version 1.1.15b; 2000/01/30  hof: image parasites
 * version 1.1.15a; 2000/01/26  hof: pathes, removed gimp 1.0.x support
 * version 1.1.14a; 2000/01/06  hof: thumbnail save/load,
 *                              Procedures for video_info file
 * version 0.98.00; 1998/11/30  hof: all PDB-calls of GIMP PDB-Procedures
 */

#ifndef _GAP_PDB_CALLS_H
#define _GAP_PDB_CALLS_H

#include "libgimp/gimp.h"

typedef struct t_video_info {
   gdouble     framerate;    /* playback rate in frames per second */
   gint32      timezoom;
} t_video_info;


gint p_pdb_procedure_available(char *proc_name);

gint32 p_gimp_rotate_degree(gint32 drawable_id, gboolean interpolation, gdouble angle_deg);

gboolean p_gimp_displays_reconnect(gint32 old_image_id, gint32 new_image_id);
gint32   p_gimp_layer_new_from_drawable(gint32 drawable_id, gint32 dst_image_id);

gint   p_gimp_file_save_thumbnail(gint32 image_id, char* filename);
gint   p_gimp_file_load_thumbnail(char* filename, gint32 *th_width, gint32 *th_height, gint32 *th_data_count, unsigned char **th_data);

gint   p_gimp_image_thumbnail(gint32 image_id, gint32 width, gint32 height,
                              gint32 *th_width, gint32 *th_height, gint32 *th_bpp,
			      gint32 *th_data_count, unsigned char **th_data);

char *p_alloc_video_info_name(char *basename);
int   p_set_video_info(t_video_info *vin_ptr, char *basename);
t_video_info *p_get_video_info(char *basename);
#endif
