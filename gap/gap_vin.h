/*  gap_vin.h
 *
 *  This module handles GAP video info files (_vin.gap)
 *  _vin.gap files store global informations about an animation
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
 * version 1.3.14a; 2003/05/24  hof: created (splitted off from gap_pdb_calls module)
 */

#ifndef _GAP_VIN_H
#define _GAP_VIN_H

#include "libgimp/gimp.h"

typedef struct t_video_info {
   gdouble     framerate;    /* playback rate in frames per second */
   gint32      timezoom;
} t_video_info;


typedef struct t_textfile_lines {
   char    *line;
   gint32   line_nr;
   void    *next;
} t_textfile_lines;


char *p_alloc_video_info_name(char *basename);
int   p_set_video_info(t_video_info *vin_ptr, char *basename);
t_video_info *p_get_video_info(char *basename);

#endif
