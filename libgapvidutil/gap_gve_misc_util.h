/* gap_gve_misc_util.h
 *
 *  GAP common encoder tool procedures
 *  with no dependencies to external libraries.
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
 * version 1.2.1a;  2004.05.14   hof: created
 */

#ifndef GAP_GVE_MISC_UTIL_H
#define GAP_GVE_MISC_UTIL_H

#include "libgimp/gimp.h"


typedef struct GapGveEncAInfo {
   long         first_frame_nr;
   long         last_frame_nr;
   long         curr_frame_nr;
   long         frame_cnt;
   char         basename[1024];    /* may include path */
   char         extension[50];
   gdouble      framerate;
} GapGveEncAInfo;

/* --------------------------*/
/* PROCEDURE DECLARATIONS    */
/* --------------------------*/


void        gap_gve_misc_get_ainfo(gint32 image_ID, GapGveEncAInfo *ainfo);



extern int gap_debug;


#endif        /* GAP_GVE_MISC_UTIL_H */
