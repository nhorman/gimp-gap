/*  gap_story_dialog.h
 *
 *  This module handles GAP storyboard dialog editor
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
 * version 1.3.27a; 2004/03/15  hof: gap_story_get_velem
 * version 1.3.25a; 2004/01/23  hof: created
 */

#ifndef _GAP_STORY_DIALOG_H
#define _GAP_STORY_DIALOG_H

#include "libgimp/gimp.h"
#include "gap_story_main.h"
#include "gap_story_properties.h"

void    gap_storyboard_dialog(GapStbMainGlobalParams *gpp);

void    gap_story_dlg_pw_render_all(GapStbPropWidget *pw);
void    gap_story_pw_single_clip_playback(GapStbPropWidget *pw);

GapStoryVideoElem * gap_story_dlg_get_velem(GapStbMainGlobalParams *sgpp
              ,const char *video_filename
	      ,gint32 seltrack
	      ,const char *preferred_decoder
	      );
guchar *    gap_story_dlg_fetch_vthumb(GapStbMainGlobalParams *sgpp
        	   ,const char *video_filename
		   ,gint32 framenr
		   ,gint32 seltrack
	           ,const char *preferred_decoder
		   , gint32 *th_bpp
		   , gint32 *th_width
		   , gint32 *th_height
		   );
guchar *    gap_story_dlg_fetch_vthumb_no_store(GapStbMainGlobalParams *sgpp
              ,const char *video_filename
	      ,gint32 framenr
	      ,gint32 seltrack
	      ,const char *preferred_decoder
	      , gint32   *th_bpp
	      , gint32   *th_width
	      , gint32   *th_height
	      , gboolean *file_read_flag
	      , gint32   *video_id
	      );
GapVThumbElem * gap_story_dlg_add_vthumb(GapStbMainGlobalParams *sgpp
			,gint32 framenr
	                ,guchar *th_data
			,gint32 th_width
			,gint32 th_height
			,gint32 th_bpp
			,gint32 video_id
			);

void  gap_story_dlg_recreate_tab_widgets(GapStbTabWidgets *tabw
				  ,GapStbMainGlobalParams *sgpp
				  );
void  gap_story_dlg_render_default_icon(GapStoryElem *stb_elem, GapPView   *pv_ptr);
void  gap_story_dlg_tabw_update_frame_label (GapStbTabWidgets *tabw
                           , GapStbMainGlobalParams *sgpp
                           );

guchar * gap_story_dlg_fetch_videoframe(GapStbMainGlobalParams *sgpp
                   , const char *gva_videofile
                   , gint32 framenumber
                   , gint32 seltrack
                   , const char *preferred_decoder
                   , gdouble delace
                   , gint32 *th_bpp
                   , gint32 *th_width
                   , gint32 *th_height
                   , gboolean do_scale
                   );

#endif
