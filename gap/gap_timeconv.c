/*  gap_player_dialog.h
 *
 *  This module handles conversions framenumber/Rate  --> timestring (mm:ss:msec)
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
 * version 1.3.14c; 2003/06/14  hof: created
 */

#include "config.h"

#include <stdio.h>

#include "libgimp/gimp.h"
#include "gap_timeconv.h"
#include "gap-intl.h"

/* -------------------------------
 * p_conv_framenr_to_timestr
 * -------------------------------
 * convert Input framenr at given framerate (unit is frames/sec)
 * into time string formated as mm:ss:msec
 * (minutes:seconds:milliseconds)
 *
 * the caller must provide a pointer to txt buffer
 * where the output is written into at txt_size.
 *
 * the txt buffer size should be at last 12 bytes
 */
void
p_conv_framenr_to_timestr( gint32 framenr, gdouble framerate, gchar *txt, gint txt_size)
{
  gdouble  msec_per_frame;
  gint32   tmsec;
  gint32   tms;
  gint32   tsec;
  gint32   tmin;

  if(framerate > 0)
  {
    msec_per_frame = 1000.0 / framerate;
  }
  else
  {
    msec_per_frame = 1000.0;
  } 

  if(framenr >= 0)
  {
    tmsec = framenr * msec_per_frame;

    tms = tmsec % 1000;
    tsec = (tmsec / 1000) % 60;
    tmin = tmsec / 60000;
    
    txt = g_snprintf(txt, txt_size,"%02d:%02d:%03d"
              , (int)tmin, (int)tsec, (int)tms);
  }
  else
  { txt = g_snprintf(txt, txt_size, "??:??:???");
  }

}  /* end p_conv_framenr_to_timestr */

