/*  gap_story_file.h
 *
 *  This module handles GAP storyboard file 
 *  parsing of storyboard level1 files (load informations into a list)
 *  and (re)write storyboard files from the list (back to storyboard file)
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
 * version 1.3.25b; 2004/01/23  hof: created
 */

#ifndef _GAP_STORY_FILE_H
#define _GAP_STORY_FILE_H

#include "libgimp/gimp.h"
#include "gap_lib.h"
#include "gap_story_syntax.h"




/* GapStoryVideoType enum values are superset of GapLibAinfoType
 * from the sourcefile gap_lib.h
 */
  typedef enum
  {
     GAP_STBV_SILENCE
    ,GAP_STBV_COLOR        
    ,GAP_STBV_IMAGE        
    ,GAP_STBV_ANIMIMAGE        
    ,GAP_STBV_FRAMES       
    ,GAP_STBV_MOVIE 
    ,GAP_STBV_COMMENT
    ,GAP_STBV_UNKNOWN
  } GapStoryVideoType;

  typedef enum
  {
     GAP_STB_PM_NORMAL
    ,GAP_STB_PM_PINGPONG        
  } GapStoryVideoPlaymode;

  typedef enum
  {
     GAP_STB_MASTER_TYPE_UNDEFINED
    ,GAP_STB_MASTER_TYPE_STORYBOARD
    ,GAP_STB_MASTER_TYPE_CLIPLIST        
  } GapStoryMasterType;


  typedef struct GapStoryElem {
    gint32                 story_id;
    gint32                 story_orig_id;
    gboolean               selected;
    GapStoryVideoType      record_type;
    GapStoryVideoPlaymode  playmode;
    gint32                 track;
    
    char  *orig_filename;   /* full filename (use that for IMAGE and MOVIE Files */
    char  *orig_src_line;   /* without \n, used to store header, comment and unknown lines */
    
                            /* basename + ext are used for FRAME range elements only */ 
    gchar *basename;        /* path+filename (without number part and without extension */
    gchar *ext;             /* extenson ".xcf" ".jpg" ... including the dot */
    gint32     seltrack;    /* selected videotrack in a videofile (for GAP_FRN_MOVIE) */
    gint32     exact_seek;  /* 0 fast seek, 1 exact seek (for GAP_FRN_MOVIE) */
    gdouble    delace;      /* 0.0 no deinterlace, 1.0-1.99 odd 2.0-2.99 even rows  (for GAP_FRN_MOVIE) */
    
    gchar *filtermacro_file;
    gint32 from_frame;
    gint32 to_frame;
    gint32 nloop;          /* 1 play one time */
    
    gint32 nframes;        /* if playmode == normal
                            * then frames = nloop * (ABS(from_frame - to_frame) + 1);
                            * else frames = (nloop * 2 * ABS(from_frame - to_frame)) + 1;
			    */

    gdouble step_density;  /* 1.0 for normal stepsize
                            * 2.0 use every 2.nd frame (double speed at same framerate)
			    * 0.5 use each frame twice (half speed at same framerate)
			    */
    gint32 file_line_nr;   /* line Number in the storyboard file */
    struct GapStoryElem  *comment;
    struct GapStoryElem  *next;
  } GapStoryElem;


  typedef struct GapStoryBoard {
     GapStoryElem  *stb_elem;
     gchar         *storyboardfile;
     GapStoryMasterType master_type;
     gint32         master_width;
     gint32         master_height;
     gdouble        master_framerate;

     gint32         layout_cols;
     gint32         layout_rows;
     gint32         layout_thumbsize;

     gchar         *preferred_decoder;
     
     /* for error handling while parsing */
     gchar         *errtext;
     gchar         *errline;
     gint32         errline_nr;
     gchar         *warntext;
     gchar         *warnline;
     gint32         warnline_nr;
     gint32         curr_nr;
     gchar         *currline;      /* dont g_free this one ! */

     gboolean       unsaved_changes;
  }  GapStoryBoard;

 
  typedef struct GapStoryLocateRet {
     GapStoryElem  *stb_elem;
     gint32        ret_framenr;
     gboolean      locate_ok;
  }  GapStoryLocateRet;

void                gap_story_debug_print_list(GapStoryBoard *stb);
void                gap_story_debug_print_elem(GapStoryElem *stb_elem);


GapStoryBoard *     gap_story_new_story_board(const char *filename);
GapStoryBoard *     gap_story_parse(const gchar *filename);
void                gap_story_elem_calculate_nframes(GapStoryElem *stb_elem);
GapStoryLocateRet * gap_story_locate_framenr(GapStoryBoard *stb, gint32 in_framenr, gint32 in_track);

void                gap_story_lists_merge(GapStoryBoard *stb_dst
                         , GapStoryBoard *stb_src
			 , gint32 story_id
			 , gboolean insert_after);
gint32              gap_story_find_last_selected(GapStoryBoard *stb);
GapStoryElem *      gap_story_elem_find_by_story_id(GapStoryBoard *stb, gint32 story_id);


gboolean            gap_story_save(GapStoryBoard *stb, const char *filename);
GapStoryElem *      gap_story_new_elem(GapStoryVideoType record_type);
void                gap_story_upd_elem_from_filename(GapStoryElem *stb_elem,  const char *filename);
gboolean            gap_story_filename_is_videofile_by_ext(const char *filename);
gboolean            gap_story_filename_is_videofile(const char *filename);
void                gap_story_elem_free(GapStoryElem **stb_elem);
void                gap_story_free_storyboard(GapStoryBoard **stb_ptr);
void                gap_story_list_append_elem(GapStoryBoard *stb, GapStoryElem *stb_elem);

gint32              gap_story_get_framenr_by_story_id(GapStoryBoard *stb, gint32 story_id);
char *              gap_story_fetch_filename(GapStoryBoard *stb, gint32 framenr);
char *              gap_story_get_filename_from_elem(GapStoryElem *stb_elem);
char *              gap_story_get_filename_from_elem_nr(GapStoryElem *stb_elem, gint32 in_framenr);
GapStoryElem *      gap_story_fetch_nth_active_elem(GapStoryBoard *stb
                                                     , gint32 seq_nr
						     , gint32 in_track
						     );
GapAnimInfo *       gap_story_fake_ainfo_from_stb(GapStoryBoard *stb_ptr);

GapStoryElem *      gap_story_elem_duplicate(GapStoryElem *stb_elem);
void                gap_story_elem_copy(GapStoryElem *stb_elem_dst, GapStoryElem *stb_elem_src);

GapStoryBoard *     gap_story_duplicate(GapStoryBoard *stb_ptr);
GapStoryBoard *     gap_story_duplicate_sel_only(GapStoryBoard *stb_ptr);
void                gap_story_remove_sel_elems(GapStoryBoard *stb);

gint32              gap_story_count_active_elements(GapStoryBoard *stb_ptr, gint32 in_track);
void                gap_story_get_master_size(GapStoryBoard *stb_ptr
                         ,gint32 *width
			 ,gint32 *height);
void                gap_story_selection_all_set(GapStoryBoard *stb, gboolean sel_state);



#endif
