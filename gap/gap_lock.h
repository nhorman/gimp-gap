/* gap_lock.h
 * 2002.04.21 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins   LOCKING
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
 * 1.3.5a; 2002/04/21   hof: created (new gap locking procedures with locktable)
 */

#ifndef _GAP_LOCK_H
#define _GAP_LOCK_H

#include "libgimp/gimp.h"

gboolean  p_gap_lock_is_locked(gint32 image_id, GimpRunMode run_mode);
void      p_gap_lock_set(gint32 image_id);
void      p_gap_lock_remove(gint32 image_id);


#endif


