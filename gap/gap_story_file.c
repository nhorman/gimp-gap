/*  gap_story_file.c
 *
 *  This module handles GAP storyboard file 
 *  parsing of storyboard level1 files (load informations into a list)
 *  and (re)write storyboard files from the list (back to storyboard file)
 *
 *  storyboard level 1 files are simplified storyboard files
 *   supported are:
 *    + play single image(s)
 *    + play frames (range of images numbered in GAP style)
 *    + read frames from videofiles (.mpg, .avi,   ..)
 *
 *   not supported are following level2 features:
 *    - audio processing
 *    - more than 1 video track
 *    - transformations (such as fade between tracks, scaling, ....)
 *    - multilayer animated image (gif anims)
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
 * version 1.3.25b; 2004/01/24  hof: created
 */

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include <libgimp/gimp.h>

#include "gap_story_syntax.h"
#include "gap_story_file.h"
#include "gap_lib.h"
#include "gap_vin.h"


#include "gap-intl.h"

extern int gap_debug;  /* 1 == print debug infos , 0 dont print debug infos */

static gint32 global_stb_elem_id = 0;


/* ----------------------------------------------------
 * gap_story_debug_print_elem
 * ----------------------------------------------------
 */
void
gap_story_debug_print_elem(GapStoryElem *stb_elem)
{
  if(stb_elem == NULL)
  {
    printf("\ngap_story_debug_print_elem:  stb_elem is NULL\n");
  }
  else
  {
    char *tab_rec_types[] =
       { "GAP_STBREC_VID_SILENCE"
       , "GAP_STBREC_VID_COLOR"      
       , "GAP_STBREC_VID_IMAGE"
       , "GAP_STBREC_VID_ANIMIMAGE"
       , "GAP_STBREC_VID_FRAMES"
       , "GAP_STBREC_VID_MOVIE"
       , "GAP_STBREC_VID_COMMENT"
       , "GAP_STBREC_VID_UNKNOWN"
       , "GAP_STBREC_AUD_SILENCE"
       , "GAP_STBREC_AUD_SOUND"
       , "GAP_STBREC_AUD_MOVIE"
       , "GAP_STBREC_ATT_OPACITY"  
       , "GAP_STBREC_ATT_ZOOM_X"  
       , "GAP_STBREC_ATT_ZOOM_Y"  
       , "GAP_STBREC_ATT_MOVE_X"  
       , "GAP_STBREC_ATT_MOVE_Y" 
       , "GAP_STBREC_ATT_FIT_SIZE"
       };
  
    printf("\ngap_story_debug_print_elem:  stb_elem: %d\n", (int)stb_elem);
 
    printf("gap_story_debug_print_elem: START\n");
    printf("record_type: %d  %s\n", (int)stb_elem->record_type
                                  , tab_rec_types[stb_elem->record_type]
				  );
    printf("story_id: %d\n", (int)stb_elem->story_id);
    printf("story_orig_id: %d\n", (int)stb_elem->story_orig_id);
    printf("selected: %d\n", (int)stb_elem->selected);
    printf("playmode: %d\n", (int)stb_elem->playmode);
    printf("track: %d\n", (int)stb_elem->track);
    printf("from_frame: %d\n", (int)stb_elem->from_frame);
    printf("to_frame: %d\n", (int)stb_elem->to_frame);
    printf("nloop: %d\n", (int)stb_elem->nloop);
    printf("seltrack: %d\n", (int)stb_elem->seltrack);
    printf("exact_seek: %d\n", (int)stb_elem->exact_seek);
    printf("delace: %f\n", (float)stb_elem->delace);
    printf("nframes: %d\n", (int)stb_elem->nframes);
    printf("step_density: %f\n", (float)stb_elem->step_density);
    printf("aud_play_from_sec: %f\n", (float)stb_elem->aud_play_from_sec);
    printf("aud_play_to_sec: %f\n", (float)stb_elem->aud_play_to_sec);
    printf("aud_volume: %f\n", (float)stb_elem->aud_volume);
    printf("aud_volume_start: %f\n", (float)stb_elem->aud_volume_start);
    printf("aud_fade_in_sec: %f\n", (float)stb_elem->aud_fade_in_sec);
    printf("aud_volume_end: %f\n", (float)stb_elem->aud_volume_end);
    printf("aud_fade_out_sec: %f\n", (float)stb_elem->aud_fade_out_sec);
    printf("file_line_nr: %d\n", (int)stb_elem->file_line_nr);
    if(stb_elem->orig_filename)
    {
      printf("orig_filename :%s:\n", stb_elem->orig_filename);
    }
    else
    {
      printf("orig_filename: (NULL)\n");
    }
    
    if(stb_elem->orig_src_line)
    {
      printf("orig_src_line :%s:\n", stb_elem->orig_src_line);
    }
    else
    {
      printf("orig_src_line: (NULL)\n");
    }

    
    if(stb_elem->basename)
    {
      printf("basename :%s:\n", stb_elem->basename);
    }
    else
    {
      printf("basename: (NULL)\n");
    }
    
    if(stb_elem->ext)
    {
      printf("ext :%s:\n", stb_elem->ext);
    }
    else
    {
      printf("ext: (NULL)\n");
    }

    
    if(stb_elem->filtermacro_file)
    {
      printf("filtermacro_file :%s:\n", stb_elem->filtermacro_file);
    }
    else
    {
      printf("filtermacro_file: (NULL)\n");
    }

    if(stb_elem->preferred_decoder)
    {
      printf("preferred_decoder :%s:\n", stb_elem->preferred_decoder);
    }
    else
    {
      printf("preferred_decoder: (NULL)\n");
    }



    printf("comment ptr:%d\n", (int)stb_elem->comment);
    printf("next    ptr:%d\n", (int)stb_elem->next);
  }
}  /* end gap_story_debug_print_elem */


/* -----------------------------
 * gap_story_debug_print_list
 * -----------------------------
 */
void
gap_story_debug_print_list(GapStoryBoard *stb)
{
  GapStoryElem *stb_elem;
  gint ii;
  
  printf("\n\ngap_story_debug_print_list:  START stb: %d\n", (int)stb);

  printf("master_type : %d\n", (int)stb->master_type );
  printf("master_width : %d\n", (int)stb->master_width );
  printf("master_height : %d\n", (int)stb->master_height );
  printf("master_framerate : %f\n", (float)stb->master_framerate );
  printf("layout_cols : %d\n", (int)stb->layout_cols );
  printf("layout_rows : %d\n", (int)stb->layout_rows );
  printf("layout_thumbsize : %d\n", (int)stb->layout_thumbsize );
  
  ii = 0;
  for(stb_elem = stb->stb_elem; stb_elem != NULL;  stb_elem = stb_elem->next)
  {
    ii++;
    printf("\nElement # (%03d)\n", (int)ii);
    
    gap_story_debug_print_elem(stb_elem);
  }

  printf("\ngap_story_debug_print_list:  END stb: %d\n", (int)stb);
}  /* end gap_story_debug_print_list */


/* -----------------------------
 * p_check_image_numpart
 * -----------------------------
 */
static void
p_check_image_numpart (GapStoryElem *stb_elem, char *filename)
{
  char *l_basnam;
  long  l_number;

  l_basnam = gap_lib_alloc_basename(filename, &l_number);
  if(l_basnam)
  {
    if(l_number > 0)
    {
      /* the image has a numberpart and probably one of series of frames */
      stb_elem->from_frame = l_number;
      stb_elem->to_frame   = l_number;
    }
    g_free(l_basnam);
  }
}  /* end p_check_image_numpart */ 


/* -----------------------------
 * p_calculate_rangesize
 * -----------------------------
 * calculate number of (output)frames within the specified range
 * respecting the step_density
 * at normal step_density of 1.0 number input and output frames are equal
 * the range from: 000003 to 000005 delivers 3 frames as rangesize
 *
 * with step_density of 0.5 the same range results in 6 output frames
 * (because each frame is shown 2 times).
 *
 * with step_density of 2.0 the same range delivers 2 frames
 * because we show only every 2.nd frame (1.5 is rounded to 2)
 */
static gint32
p_calculate_rangesize(GapStoryElem *stb_elem)
{
  gdouble fnr;
  
  if(stb_elem->step_density <= 0)
  {
    /* force leagal step_density value */
    stb_elem->step_density = 1.0;
  }
  
  fnr = (gdouble)(ABS(stb_elem->from_frame - stb_elem->to_frame) +1);
  fnr = fnr / stb_elem->step_density;

  return((gint32)MAX( (fnr+0.5), 1 ));
  
}  /* end p_calculate_rangesize */

static gint32
p_calculate_rangesize_pingpong(GapStoryElem *stb_elem)
{
  gdouble fnr;
  
  if(stb_elem->step_density <= 0)
  {
    /* force leagal step_density value */
    stb_elem->step_density = 1.0;
  }
  
  fnr = 2.0 * (gdouble)(ABS(stb_elem->from_frame - stb_elem->to_frame));
  fnr = fnr / stb_elem->step_density;

  return((gint32)MAX( (fnr+0.5), 1 ));
  
}  /* end p_calculate_rangesize_pingpong */


/* ------------------------------
 * p_calculate_pingpong_framenr
 * ------------------------------
 * A typical pingpong sequence looks like this (at rangesize of 4 frames)
 *
 *
 *   IN framenr:  1  2  3  4  5  6  7  8  9 10 11 12 13 14 ...
 *   RET:         1  2  3  4  3  2  1  2  3  4  3  2  1  2 ....
 *               +----------+------+-----------+-----+-----
 *                PING        PONG  PING        PONG     .....
 */
static gint32
p_calculate_pingpong_framenr(gint32 framenr, gint32 rangesize, gdouble step_density)
{
  gint32  ret_framenr;
  gint32  l_pingpongsize;
  gdouble fnr;


  if ((framenr <= 1) || (rangesize <= 1))
  {
     return 1;
  }

  l_pingpongsize = (2 * rangesize) -2;

  ret_framenr = framenr % l_pingpongsize;
  if(ret_framenr > rangesize)
  {
    ret_framenr = (2 + l_pingpongsize) - ret_framenr;
  }
  else
  {
    if(ret_framenr == 0)
    {
      ret_framenr = 2;
    }
  }

  fnr = 1.0 + ((gdouble)(ret_framenr -1) * step_density);
  return ((gint32)(fnr + 0.5));
  
}  /* end p_calculate_pingpong_framenr */


/* ------------------------------
 * p_set_stb_error
 * ------------------------------
 * print the passed errtext to stdout
 * and store linenumber, errtext and sourcecode line
 * of the 1.st error that occured
 */
static void
p_set_stb_error(GapStoryBoard *stb, char *errtext)
{
  printf("** error: %s\n   [at line:%d] %s\n"
        , errtext
        , (int)stb->curr_nr
        , stb->currline
        );
  if(stb->errtext == NULL)
  {
     if(errtext)
     {
       stb->errtext     = g_strdup(errtext);
     }
     else
     {
       stb->errtext     = g_strdup(_("internal error"));
     }
     stb->errline_nr  = stb->curr_nr;
     if(stb->currline) 
     {
       stb->errline     = g_strdup(stb->currline);
     }
  }
}  /* end p_set_stb_error */


/* ------------------------------
 * p_set_stb_warning
 * ------------------------------
 * print the passed errtext to stdout
 * and store linenumber, errtext and sourcecode line
 * of the 1.st error that occured
 */
static void
p_set_stb_warning(GapStoryBoard *stb, char *warntext)
{
  printf("## warning: %s\n   [at line:%d] %s\n"
        , warntext
        , (int)stb->curr_nr
        , stb->currline
        );
  if(stb->warntext == NULL)
  {
     if(warntext)
     {
       stb->warntext     = g_strdup(warntext);
     }
     else
     {
       stb->warntext     = g_strdup(_("internal error"));
     }
     stb->warnline_nr  = stb->curr_nr;
     if(stb->currline) 
     {
       stb->warnline     = g_strdup(stb->currline);
     }
  }
}  /* end p_set_stb_warning */


/* ------------------------------
 * gap_story_new_story_board
 * ------------------------------
 */
GapStoryBoard *
gap_story_new_story_board(const char *filename)
{
  GapStoryBoard *stb;
  
  stb = g_new(GapStoryBoard, 1);
  if(stb)
  {
    stb->master_type = GAP_STB_MASTER_TYPE_UNDEFINED;
    stb->master_width = -1;
    stb->master_height = -1;
    stb->master_framerate = -1.0;
    stb->master_volume = -1.0;
    stb->master_samplerate = -1;
    
    stb->layout_cols = -1;
    stb->layout_rows = -1;
    stb->layout_thumbsize = -1;
    
    stb->stb_elem = NULL;
    stb->storyboardfile = NULL;
    if(filename)
    {
      stb->storyboardfile = g_strdup(filename);
    }

    stb->curr_nr  = 0;
    stb->currline  = NULL;
    stb->errline_nr  = 0;
    stb->errtext  = NULL;
    stb->errline  = NULL;
    stb->warnline_nr  = 0;
    stb->warntext  = NULL;
    stb->warnline  = NULL;

    stb->preferred_decoder  = NULL;

    stb->unsaved_changes = TRUE;
  }
  
  return(stb);
}  /* end gap_story_new_story_board */


/* ------------------------------
 * gap_story_new_elem
 * ------------------------------
 */
GapStoryElem *
gap_story_new_elem(GapStoryRecordType record_type)
{
  GapStoryElem *stb_elem;
  
  stb_elem = g_new(GapStoryElem, 1);
  if(stb_elem)
  {
    /* init members for level1 VID Record types */
    stb_elem->story_id = global_stb_elem_id++;
    stb_elem->story_orig_id = -1;
    stb_elem->selected = FALSE;
    stb_elem->record_type = record_type;
    stb_elem->playmode = GAP_STB_PM_NORMAL;
    stb_elem->track = 1;
    stb_elem->orig_filename = NULL;
    stb_elem->orig_src_line = NULL;
    stb_elem->basename = NULL;
    stb_elem->ext = NULL;
    stb_elem->seltrack = 1;
    stb_elem->exact_seek = 0;
    stb_elem->delace = 0.0;
    stb_elem->filtermacro_file = NULL;
    stb_elem->preferred_decoder = NULL;
    stb_elem->from_frame = 1;
    stb_elem->to_frame = 1;
    stb_elem->nloop = 1;
    stb_elem->nframes = -1;         /* -1 : for not specified in file */ 
    stb_elem->step_density = 1.0;   /* normal stepsize 1:1 */ 
    stb_elem->file_line_nr = -1;  

    /* init members for level2 VID Record types */
    stb_elem->vid_wait_untiltime_sec = 0.0;
    stb_elem->color_red = 0.0;
    stb_elem->color_green = 0.0;
    stb_elem->color_blue = 0.0;
    stb_elem->color_alpha = 0.0;
           
    /* init members for attribute Record types */
    stb_elem->att_keep_proportions = FALSE;
    stb_elem->att_fit_width = TRUE;
    stb_elem->att_fit_height = TRUE;

    stb_elem->att_value_from = 0.0;
    stb_elem->att_value_to = 0.0;
    stb_elem->att_value_dur = 0;        /* number of frames to change from -> to value */

    /* init members for Audio Record types */
    stb_elem->aud_filename = NULL;
    stb_elem->aud_seltrack = 1;          /* selected audiotrack in a videofile (for GAP_AUT_MOVIE) */
    stb_elem->aud_wait_untiltime_sec = 0.0;
    stb_elem->aud_play_from_sec = 0.0;
    stb_elem->aud_play_to_sec   = 0.0;
    stb_elem->aud_volume_start  = 1.0;
    stb_elem->aud_volume        = 1.0;
    stb_elem->aud_volume_end    = 1.0;
    stb_elem->aud_fade_in_sec   = 0.0;
    stb_elem->aud_fade_out_sec  = 0.0;

    /* init list pointers */
    stb_elem->comment = NULL;
    stb_elem->next = NULL;
  }
  
  if(gap_debug) printf("gap_story_new_elem: RETURN stb_elem ptr: %d\n", (int)stb_elem );
  return(stb_elem);
}  /* end gap_story_new_elem */


/* --------------------------------------
 * gap_story_filename_is_videofile_by_ext
 * --------------------------------------
 */
gboolean
gap_story_filename_is_videofile_by_ext(const char *filename)
{
  char  *l_ext;

  l_ext = gap_lib_alloc_extension(filename);
  if(l_ext)
  {
    if(*l_ext == '.')
    {
      l_ext++;
    }
    if( (strcmp(l_ext, "mpeg") == 0)
    ||  (strcmp(l_ext, "mpg") == 0)
    ||  (strcmp(l_ext, "MPEG") == 0)
    ||  (strcmp(l_ext, "MPG") == 0)
    ||  (strcmp(l_ext, "avi") == 0)
    ||  (strcmp(l_ext, "AVI") == 0)
    ||  (strcmp(l_ext, "mov") == 0)   /* Quicktime videos */
    ||  (strcmp(l_ext, "MOV") == 0)
    ||  (strcmp(l_ext, "vob") == 0)   /* DVD MPEG2 videos */
    ||  (strcmp(l_ext, "VOB") == 0)
    ||  (strcmp(l_ext, "ifo") == 0)   /* DVD MPEG2 videos */
    ||  (strcmp(l_ext, "IFO") == 0)
    ||  (strcmp(l_ext, "toc") == 0)   /* libmpeg3 typical table of video content files */
    ||  (strcmp(l_ext, "TOC") == 0)
    ||  (strcmp(l_ext, "rm") == 0)
    ||  (strcmp(l_ext, "RM") == 0)
    ||  (strcmp(l_ext, "wmv") == 0)
    ||  (strcmp(l_ext, "WMV") == 0)
    ||  (strcmp(l_ext, "asf") == 0)
    ||  (strcmp(l_ext, "ASF") == 0)
    )
    {
      return (TRUE);
    }
  }
  return(FALSE);
}  /* end gap_story_filename_is_videofile_by_ext */

/* --------------------------------
 * gap_story_filename_is_videofile
 * --------------------------------
 */
gboolean
gap_story_filename_is_videofile(const char *filename)
{

  if(gap_story_filename_is_videofile_by_ext(filename))
  {
     /* TODO: additional check for video should be done by the check sig procedure
      * of the video API that will be part of gimp-gap in future.
      * as long as we have no API, just check for known common vidoefile extensions
      *
      * The check for extension is also needed (in future too), because some video decoders
      * are able to read some imageformats (such as jpeg png ..)
      * and would open OK even on such single images.
      * But here we want to return FALSE if filename is an image.
      */
      return (TRUE);
  }
  return(FALSE);
  
}  /* end gap_story_filename_is_videofile */

/* --------------------------------
 * gap_story_upd_elem_from_filename
 * --------------------------------
 */
void
gap_story_upd_elem_from_filename(GapStoryElem *stb_elem,  const char *filename)
{
  if(stb_elem == NULL) { return; }
  if(stb_elem->basename)
  {
    g_free(stb_elem->basename);
  }
  stb_elem->basename = NULL;
  
  if(stb_elem->ext)
  {
    g_free(stb_elem->ext);
  }
  stb_elem->ext = NULL;


  if(stb_elem->orig_filename)
  {
    g_free(stb_elem->orig_filename);
  }
  stb_elem->orig_filename = g_strdup(filename);
  stb_elem->record_type = GAP_STBREC_VID_UNKNOWN;
  
  if(g_file_test(filename, G_FILE_TEST_EXISTS))
  {
    long   l_number;
    
    stb_elem->basename = gap_lib_alloc_basename(filename, &l_number);
    stb_elem->ext = gap_lib_alloc_extension(filename);
    if(gap_story_filename_is_videofile(filename))
    {
      stb_elem->record_type = GAP_STBREC_VID_MOVIE;
    }
    else
    {
      if(l_number > 0)
      {
	stb_elem->record_type = GAP_STBREC_VID_FRAMES;
      }
      else
      {
	stb_elem->record_type = GAP_STBREC_VID_IMAGE;
      }
    }
  }
}  /* end gap_story_upd_elem_from_filename */


/* ----------------------------------------------------
 * p_free_stb_elem
 * ----------------------------------------------------
 * free all strings that were allocated as members 
 * of the passed Storyboard Element
 */
static void
p_free_stb_elem(GapStoryElem *stb_elem)
{
  if(stb_elem->orig_filename)     { g_free(stb_elem->orig_filename);}
  if(stb_elem->orig_src_line)     { g_free(stb_elem->orig_src_line);}
  if(stb_elem->basename)          { g_free(stb_elem->basename);}
  if(stb_elem->ext)               { g_free(stb_elem->ext);}
  if(stb_elem->filtermacro_file)  { g_free(stb_elem->filtermacro_file);}
  if(stb_elem->preferred_decoder) { g_free(stb_elem->preferred_decoder);}
}  /* end p_free_stb_elem */


/* ------------------------------
 * gap_story_elem_free
 * ------------------------------
 */
void
gap_story_elem_free(GapStoryElem **stb_elem)
{
  GapStoryElem *stb_comment;
  GapStoryElem *stb_next;
  
  if(stb_elem)
  {
    if(*stb_elem)
    {
      for(stb_comment = (*stb_elem)->comment; stb_comment!= NULL; stb_comment = stb_next)
      {
	stb_next = (GapStoryElem *)stb_comment->next;
	p_free_stb_elem(stb_comment);
	g_free(stb_comment);
      }
      p_free_stb_elem(*stb_elem);
      g_free(*stb_elem);
      *stb_elem = NULL;
    }
  }
}  /* end gap_story_elem_free */

/* ----------------------------------------------------
 * p_free_stb_elem_list
 * ----------------------------------------------------
 * free all the elements in the passed stb_list
 * including the branches that may be attached at the comment pointer
 */
static void
p_free_stb_elem_list(GapStoryElem *stb_list)
{
  GapStoryElem *stb_elem;
  GapStoryElem *stb_comment;
  GapStoryElem *stb_next;

  if(gap_debug) printf("p_free_stb_elem_list START\n");

  for(stb_elem = stb_list; stb_elem != NULL; stb_elem = stb_next)
  {
    for(stb_comment = stb_elem->comment; stb_comment!= NULL; stb_comment = stb_next)
    {
      stb_next = (GapStoryElem *)stb_comment->next;
      p_free_stb_elem(stb_comment);
      g_free(stb_comment);
    }

    stb_next = (GapStoryElem *)stb_elem->next;
    p_free_stb_elem(stb_elem);
    g_free(stb_elem);
  }

  if(gap_debug) printf("p_free_stb_elem_list END\n");
}	/* end p_free_stb_elem_list */

/* ----------------------------------------------------
 * gap_story_free_storyboard
 * ----------------------------------------------------
 */
void
gap_story_free_storyboard(GapStoryBoard **stb_ptr)
{
  GapStoryBoard *stb;
  
  stb = *stb_ptr;
  if(stb)
  {
    if(stb->stb_elem)
    {
      p_free_stb_elem_list(stb->stb_elem);
    }
    if(stb->storyboardfile)
    {
      g_free(stb->storyboardfile);
    }
    if(stb->errtext)
    {
      g_free(stb->errtext);
    }
    if(stb->errline)
    {
      g_free(stb->errline);
    }
    if(stb->warntext)
    {
      g_free(stb->warntext);
    }
    if(stb->warnline)
    {
      g_free(stb->warnline);
    }
    
    /* Note: Dont try to g_free stb->currline
     * it points to memory that is controlled (and already free'd) by the parser
     */
    
  }
  *stb_ptr = NULL;
}  /* end gap_story_free_storyboard */




/* ----------------------------------------------------
 * gap_story_list_append_elem
 * ----------------------------------------------------
 * add new stb_elem at end of the stb->stb_elem list
 * if the existing list's tail element has record_type == GAP_STBREC_VID_COMMENT
 * (or more record_type == GAP_STBREC_VID_COMMENT elements in sequence)
 * the comment-tail is cut off the existing list
 * and assigned to the comment pointer of the new element.
 * In both cases the new element will be the last in the stb->stb_elem list
 */
void
gap_story_list_append_elem(GapStoryBoard *stb, GapStoryElem *stb_elem)
{
  GapStoryElem *stb_listend;
  GapStoryElem *stb_non_comment;

  if(gap_debug)
  {
    printf("gap_story_list_append_elem: START\n");
    gap_story_debug_print_elem(stb_elem);
    printf("\n\n");
  }

  stb_non_comment = NULL;
  if((stb)  && (stb_elem))
  {
     stb->unsaved_changes = TRUE;
     if (stb->stb_elem == NULL)
     {
       /* 1. element (or returned list) starts the list */
       stb->stb_elem = stb_elem;
     }
     else
     {
       /* link stb_elem (that can be a single ement or list) to the end of frn_list */
       stb_listend = stb->stb_elem;
       while(stb_listend->next != NULL)
       {
          if(stb_listend->record_type != GAP_STBREC_VID_COMMENT)
          {
            stb_non_comment = stb_listend;
          }
          stb_listend = (GapStoryElem *)stb_listend->next;
       }
       if((stb_listend->record_type == GAP_STBREC_VID_COMMENT)
       && (stb_elem->record_type != GAP_STBREC_VID_COMMENT))
       {
         /* the last elem in the list is a comment.
          * in that case assign this (and all comments back to
          * the last non comment line)
          * to the comment pointer of the current element that is to add now,
          * and add the element.
          */
         if(stb_non_comment == NULL)  
         {
           stb_elem->comment = (GapStoryElem *)stb->stb_elem;
           stb->stb_elem = (GapStoryElem *)stb_elem;
         }
         else
         {
           /*  Old list:
            *
            *  +------+          +------+          +------+
            *  |non-  |          |#comm1|          |#comm2|          
            *  |comm  |---next-->|      |---next-->|      |---next-->| 
            *  +------+          +------+          +------+
            *
            *  New list:
            *
            *  +------+          +------+
            *  |non-  |          |new   |
            *  |com   |---next-->|elem  |---next-->| 
            *  +------+          +------+
            *                        |
            *                        |               +------+          +------+
            *                        |               |#comm1|          |#comm2|          
            *                        +--- comment--->|      |---next-->|      |---next-->| 
            *	                                     +------+          +------+
            *
            *
            */
           stb_elem->comment = (GapStoryElem *)stb_non_comment->next;
           stb_non_comment->next = stb_elem;
         }
       }
       else
       {
         stb_listend->next = (GapStoryElem *)stb_elem;
       }
     }
  }
}  /* end gap_story_list_append_elem */



/* ----------------------------------------------------
 * gap_story_lists_merge
 * ----------------------------------------------------
 * insert all elements of stb_src into stb_dst
 * at position after (or before) the elment with matching story_id.
 *
 * use story_id -1 (or any other invalid story_id) if you want to
 * append src at end of dst list.
 *
 * IMPORTANT: the elements move from stb_src to stb_dst list,
 *            the src list is always emty after calling this procedure.
 *
 * this procedure is typical used for pasting a selection list (src)
 * into another storyboard list.
 */
void
gap_story_lists_merge(GapStoryBoard *stb_dst
                         , GapStoryBoard *stb_src
			 , gint32 story_id
			 , gboolean insert_after)
{
  GapStoryElem *stb_elem_prev;
  GapStoryElem *stb_elem;
  GapStoryElem *stb_elem_last_src;

  if((stb_dst) && (stb_src))
  {
    stb_dst->unsaved_changes = TRUE;
    stb_src->unsaved_changes = TRUE;
    stb_elem_last_src = stb_src->stb_elem;
    if(stb_elem_last_src == NULL)
    {
      return;  /* src list is already emty, nothing left to do */
    }
    
    /* find last source element */
    while(TRUE)
    {
      if(stb_elem_last_src->next == NULL)
      {
       break;
      }
      else
      {
        stb_elem_last_src = stb_elem_last_src->next;
      }
    }
    
    if(stb_dst->stb_elem == NULL)
    {
      /* destination list is emty, just move src elems there and done */ 
      stb_dst->stb_elem = stb_src->stb_elem;
      stb_src->stb_elem = NULL;
    }
    
    /* find position in dst list and insert the src list there */ 
    stb_elem_prev = NULL;
    for(stb_elem = stb_dst->stb_elem; stb_elem != NULL;  stb_elem = stb_elem->next)
    {
      if((stb_elem->story_id == story_id))
      {
        if(insert_after)
	{
	  stb_elem_last_src->next = stb_elem->next;
	  stb_elem->next = stb_src->stb_elem;
	}
	else
	{
 	  stb_elem_last_src->next = stb_elem;
	  
          if(stb_elem_prev == NULL)
	  {
	    /* insert before the 1.st element */
	    stb_dst->stb_elem = stb_src->stb_elem;
	  }
	  else
	  {
	    stb_elem_prev->next = stb_src->stb_elem;
	  }
	}
 	stb_src->stb_elem = NULL;
	return;
	
      }
      stb_elem_prev = stb_elem;
    }
    
    if(stb_elem_prev)  /* should never be NULL at this point */
    {
      /* add src list at end of dst list */
      stb_elem_prev->next = stb_src->stb_elem;
      stb_src->stb_elem = NULL;
    }
  }
 
  
}  /* end gap_story_lists_merge */



/* ---------------------------------
 * gap_story_find_last_selected
 * ---------------------------------
 */
gint32
gap_story_find_last_selected(GapStoryBoard *stb)
{
  GapStoryElem *stb_elem;
  gint32 story_id;

  story_id = -1;
  if(stb)
  {
    for(stb_elem = stb->stb_elem; stb_elem != NULL;  stb_elem = stb_elem->next)
    {
      if(stb_elem->selected)
      {
	story_id = stb_elem->story_id;
      }
    }
  }
  return (story_id);
  
}  /* end gap_story_find_last_selected */



/* ---------------------------------
 * gap_story_elem_find_by_story_id
 * ---------------------------------
 */
GapStoryElem *
gap_story_elem_find_by_story_id(GapStoryBoard *stb, gint32 story_id)
{
  GapStoryElem *stb_elem;

  if(stb)
  {
    for(stb_elem = stb->stb_elem; stb_elem != NULL;  stb_elem = stb_elem->next)
    {
      if(stb_elem->story_id == story_id)
      {
	return(stb_elem);
      }
    }
  }
  return (NULL);
  
}  /* end gap_story_elem_find_by_story_id */



/* ----------------------------------------------------
 * p_flip_dir_separators
 * ----------------------------------------------------
 * Replace all / and \ characters by G_DIR_SEPARATOR
 */
static void
p_flip_dir_seperators(char *ptr)
{
  while(ptr)
  {
    if((*ptr == '\\') || (*ptr == '/'))
    {
      *ptr = G_DIR_SEPARATOR;
    }
    if(*ptr == '\0')
    {
      break;
    }
    ptr++;
  }
}  /* end p_flip_dir_seperators */


/* ----------------------------------------------------
 * p_fetch_string
 * ----------------------------------------------------
 * scan a whitespace terminated word or a string
 * optional enclosed in "double quotes" from a buffer
 * located at *scan_ptr
 * and set *scan_ptr to the character after the scanned string
 *
 * The returned string is the fetched value part of the string
 * without quotes, terminated with \0.
 * if the string has leading or trailing keyword, the keyword
 * is stripped off, and delivered as additional output parameter.
 * - Both the returned string and the out_keyword
 *   must be freed by the  caller after usage.
 * - if the scan_ptr is at end of the line, the returned string
 *   will contain only the terminating \0 byte.
 * Further output:
 *   keyword  (NULL if no keyword is present)
 *   else deliver keyword part that mybe specified in 2 ways:
 *     KEYWORD:valuestring
 *     KEYWORD:"c:\valuestring"
 *     valuestring=KEYWORD       ## this is old deprecated style
 *     if the string has keywords both in new and old style
 *     then ignore the old styled keyword
 */
static char *
p_fetch_string(char **scan_ptr
              ,char **out_keyword
              )
{
  guint  l_char1_idx;
  guint  l_start_idx;
  guint  l_end_idx;
  gint   l_key1_idx;
  gint   l_key2_idx;
  guint  l_len;
  gulong l_size;
  guint  l_idx;
  char *buff;
  char *keyword;
  char *ret_string;
  char l_termchar;
  gboolean l_enable_end_idx;

  buff = *scan_ptr;
  
  *out_keyword = NULL;
  keyword = NULL;

  /* if(gap_debug) printf("p_fetch_string buff START, buff:%s:\n", buff); */

  /* skip leading whitespaces */
  for(l_idx=0;l_idx<4000;l_idx++)
  {
    if((buff[l_idx] != ' ') && (buff[l_idx] != '\t'))
    {
      break;
    }
  }

  l_key1_idx = -1;
  l_key2_idx = -1;
  l_termchar = ' ';
  l_char1_idx = l_idx;
  if (buff[l_idx] == '"')
  {
    l_termchar = '"';
    l_idx++;
  }
  l_start_idx = l_idx;
  l_end_idx   = l_start_idx + 1;
  l_enable_end_idx = TRUE;

  /* search for end of the string (termchar or newline, tab or zero) */
  for( ;l_idx<4000;l_idx++)
  {
    if((buff[l_idx] == l_termchar)
    || (buff[l_idx] == '\t')
    || (buff[l_idx] == '\n')
    || (buff[l_idx] == '\0')
    )
    {
      /* check if end is already set */
      if(l_enable_end_idx)
      {
        l_end_idx = l_idx;
	l_enable_end_idx = FALSE;
      }
      
      if (buff[l_idx] != '"')
      {
        break;
      }
      /* continue scann after doublequotes (until EOL or whitespace) */
      l_termchar = ' ';
    }
    if(l_termchar != '"')
    {
      if((buff[l_idx] == ':')
      && (l_key1_idx < 0))
      {
        /* have a leading keyword (== named parameter) */
        l_key1_idx = l_idx;
        l_len = l_idx - l_char1_idx;
        l_size = l_len +1;   /* for the \0 byte at end of string */

        keyword = g_malloc(l_size);
        memcpy(keyword, &buff[l_char1_idx], l_len );
        keyword[l_len] = '\0';

        l_start_idx = l_idx + 1;
	if(buff[l_start_idx] == '"')
	{
          l_termchar = '"';
          l_start_idx++;
	  l_idx++;
	}
        l_end_idx   = l_start_idx + 1;
	l_enable_end_idx = TRUE;
      }
      if((buff[l_idx] == '=')
      && (l_key2_idx < 0))
      {
        /* have a trailing keyword (== named parameter old deprecated style) */
        l_key2_idx = l_idx;
        l_end_idx = l_idx;
	l_enable_end_idx = FALSE;
      }
    }
  }

  l_len = l_end_idx - l_start_idx;
  l_size = l_len +1;   /* for the \0 byte at end of string */

  /* copy the string */
  ret_string = g_malloc(l_size);

  if(ret_string)
  {
    if(l_len > 0)
    {
      memcpy(ret_string, &buff[l_start_idx], l_len );
    }
    ret_string[l_len] = '\0';
  }

  if((l_key2_idx >= 0)
  && (keyword == NULL))
  {
    /* deliver keyword (old sytle) */
    l_len = l_idx - (l_key2_idx +1);
    l_size = l_len +1;   /* for the \0 byte at end of string */
    keyword = g_malloc(l_size);
    memcpy(keyword, &buff[l_key2_idx+1], l_len );
    keyword[l_len] = '\0';
  }

    
  /* advance scan position */
  *scan_ptr = &buff[l_idx];

  if(gap_debug)
  {
    printf("p_fetch_string:return:%s: start:%d end:%d\n"
          , ret_string
	  , (int)l_start_idx
	  , (int)l_end_idx
	  );
    if(keyword)
    {
      printf("p_fetch_string: keyword:%s  key1_idx:%d, key2_idx:%d\n"
          , keyword
          , (int)l_key1_idx
          , (int)l_key2_idx
	  );
    }
    else
    {
      printf("p_fetch_string: keyword: IS NULL  key1_idx:%d, key2_idx:%d\n"
          , (int)l_key1_idx
          , (int)l_key2_idx
	  );
    }
  }

  *out_keyword = keyword;
  return (ret_string);
}	/* end p_fetch_string */
  

/* ------------------------------
 * p_scan_gint32
 * ------------------------------
 */
static gint32 
p_scan_gint32(char *ptr, gint32 min, gint32 max, GapStoryBoard *stb)
{
  char *l_errtxt;
  char *l_end_ptr;
  long   l_num;

  l_num = -1;
  if(ptr)
  {
    if(*ptr != '\0')
    {
      l_end_ptr = ptr;
      l_num = strtol(ptr, &l_end_ptr, 10);
      if (ptr != l_end_ptr)
      {
         if((l_num >= min) && (l_num <= max))
         {
           return ((gint32)l_num);
         }
      }

      if(stb)
      {
        l_errtxt = g_strdup_printf(_("illegal number: %s (valid range is %d upto %d)\n")
                                  , ptr, (int)min, (int)max);
        p_set_stb_error(stb, l_errtxt);
        g_free(l_errtxt);
      }
    }
  }
  return (l_num);
}  /* end p_scan_gint32 */


/* ------------------------------
 * p_scan_gdouble
 * ------------------------------
 */
static gdouble 
p_scan_gdouble(char *ptr, gdouble min, gdouble max, GapStoryBoard *stb)
{
  char *l_errtxt;
  char *l_end_ptr;
  double l_num;

  l_num = 1.0;
  if(ptr)
  {
    if(*ptr != '\0')
    {
      l_end_ptr = ptr;
      l_num = g_ascii_strtod(ptr, &l_end_ptr);
      if (ptr != l_end_ptr)
      {
         if((l_num >= min) && (l_num <= max))
         {
           return ((gdouble)l_num);
         }
      }

      if(stb)
      {
        l_errtxt = g_strdup_printf(_("illegal number: %s (valid range is %.3f upto %.3f)")
                                  , ptr, (float)min, (float)max);
        p_set_stb_error(stb, l_errtxt);
        g_free(l_errtxt);
      }
    }
    
  }
  return (l_num);
}  /* end p_scan_gdouble */





/* ------------------------------
 * p_story_parse_line
 * ------------------------------
 * parse one storyboardfile line
 * if the line is syntactical OK
 *    add The Line as new stb_elem (at end of the stb_elem list)
 * if the line has ERRORS
 *    the errtxt memer in the GapStoryBoard struct is set
 *    to the errormessage of the 1st error that occured.
 *
 * missing Header is tolerated but:
 * - lines with unsupported record type are rejected if no header is present
 *   in case where valid header is present, unsupported lines are tolerated
 *   for compatibility with future syntax extension.
 *
 * this way the parser will accept textfiles that are simlpe playlists of imagefilenames
 * (one name per line) without any keywords 
 */
static void
p_story_parse_line(GapStoryBoard *stb, char *longline, gint32 longlinenr, char *multi_lines)
{
#define GAP_MAX_STB_PARAMS_PER_LINE 14  
  char *l_scan_ptr;
  char *l_record_key;
  char *l_parname;
  char *l_wordval[GAP_MAX_STB_PARAMS_PER_LINE];
  gint ii;
  GapStoryElem *stb_elem;
  

  if(gap_debug) printf("p_story_parse_line: %d %s\n", (int)longlinenr, longline);

  l_scan_ptr = longline;
  stb->currline = longline;
  stb->curr_nr = longlinenr;


  /* clear array of values */
  for(ii=0; ii < GAP_MAX_STB_PARAMS_PER_LINE; ii++)
  {
    l_wordval[ii] = g_strdup("\0");
  }
  
  /* get the record key (1.st space seperated word) */
  l_wordval[0]    = p_fetch_string(&l_scan_ptr, &l_parname);
  l_record_key = l_wordval[0];
  if(l_parname)
  {
    g_free(l_parname);
  }
  
  /* assign parameters to their expected position params may be specified
   * a) at expected position
   *        RECORD_IDENTIFIER  val1  val2  val3
   * b) by keyword
   *        RECORD_IDENTIFIER  key:val2  key3:val3  key1:val1
   * mixed variants are possible as long as positions are unique 
   */
  for(ii=1; ii < GAP_MAX_STB_PARAMS_PER_LINE; ii++)
  {
    gchar *l_value;
    gint   l_key_idx;

// printf("\n%s   ii:%d\n", l_record_key, (int)ii);
    l_parname = NULL;
    l_value = p_fetch_string(&l_scan_ptr, &l_parname);
// printf("%s   ii:%d (after SCANN)\n", l_record_key, (int)ii);
    if(l_value)
    {
      if(*l_value != '\0')
      {
	l_key_idx = -1;
	if(l_parname)
	{
          l_key_idx = gap_stb_syntax_get_parname_idx(l_record_key
                                                    ,l_parname
                                                    );

// printf("%s   parname:%s: key_idx:%d\n", l_record_key, l_parname, (int)l_key_idx);

          g_free(l_parname);
          l_parname = NULL;
	}
	if(l_key_idx > 0)
	{
          if(*l_wordval[l_key_idx] != '\0')
	  {
            p_set_stb_error(stb, _("same parameter used more than once"));
	  }
          else
	  {
	    g_free(l_wordval[l_key_idx]);
	    l_wordval[l_key_idx] = g_strdup(l_value);
	  }
	}
	else
	{
          if(*l_wordval[ii] != '\0')
	  {
            p_set_stb_error(stb, _("conflict: positional parameter shadows named parameter"));
	  }
          else
	  {
	    g_free(l_wordval[l_key_idx]);
	    l_wordval[ii] = g_strdup(l_value);
	  }
	}
      }
      g_free(l_value);
    }
// printf("%s   ii:%d (END)\n", l_record_key, (int)ii);
  }
 
  if(gap_debug)
  { 
    for(ii=0; ii < GAP_MAX_STB_PARAMS_PER_LINE; ii++)
    {
      printf("  l_wordval[%02d]: %s\n"
            ,(int)ii
	    ,l_wordval[ii]
	    );
    }
  }
  
  
  /* check for Header */
  if(longlinenr == 1)
  {
    stb->master_type = GAP_STB_MASTER_TYPE_UNDEFINED;
    
    if(strcmp(l_record_key, GAP_STBKEY_STB_HEADER) == 0)
    {
      stb->master_type = GAP_STB_MASTER_TYPE_STORYBOARD;
    }
    if(strcmp(l_record_key, GAP_STBKEY_CLIP_HEADER) == 0)
    {
      stb->master_type = GAP_STB_MASTER_TYPE_CLIPLIST;
    }
    
    if (stb->master_type != GAP_STB_MASTER_TYPE_UNDEFINED)
    {
      goto cleanup;  /* HDR processed OK */
    }
    else
    {
       char *l_errtxt;

       l_errtxt = g_strdup_printf(_("Header not found!\n(line 1 must start with:  %s or %s")
                                 ,  GAP_STBKEY_STB_HEADER
                                 ,  GAP_STBKEY_CLIP_HEADER
                                 );
       p_set_stb_warning(stb, l_errtxt);
       g_free(l_errtxt);
    }
    
  }

  if ((*l_record_key == '#')
  ||  (*l_record_key == '\n')
  ||  (*l_record_key == '\0'))
  {
    stb_elem = gap_story_new_elem(GAP_STBREC_VID_COMMENT);
    if(stb_elem)
    {
        stb_elem->orig_src_line = g_strdup(multi_lines);
        stb_elem->file_line_nr = longlinenr;
        gap_story_list_append_elem(stb, stb_elem);
    }
    goto cleanup;   /* ignore blank and comments lines */
  }

  /* Master informations: GAP_STBKEY_VID_MASTER_SIZE */
  if (strcmp(l_record_key, GAP_STBKEY_VID_MASTER_SIZE) == 0)
  {
      if(*l_wordval[1]) { stb->master_width     = p_scan_gint32(l_wordval[1], 1, 999999, stb); }
      if(*l_wordval[2]) { stb->master_height    = p_scan_gint32(l_wordval[2], 1, 999999, stb); }
      goto cleanup;  /* master RECORD does not create frame range elements */
  }
  /* Master informations: GAP_STBKEY_VID_MASTER_FRAMERATE */
  if (strcmp(l_record_key, GAP_STBKEY_VID_MASTER_FRAMERATE) == 0)
  {
      if(*l_wordval[1]) { stb->master_framerate = p_scan_gdouble(l_wordval[1], 0.1, 999.9, stb); }
      goto cleanup;  /* master RECORD does not create frame range elements */
  }
  /* Master informations: GAP_STBKEY_VID_PREFERRED_DECODER */
  if (strcmp(l_record_key, GAP_STBKEY_VID_PREFERRED_DECODER) == 0)
  {
      if(*l_wordval[1]) { stb->preferred_decoder = g_strdup(l_wordval[1]); }
      goto cleanup;  /* master RECORD does not create frame range elements */
  }


  /* Master informations: GAP_STBKEY_AUD_MASTER_VOLUME */
  if (strcmp(l_record_key, GAP_STBKEY_AUD_MASTER_VOLUME) == 0)
  {
      if(*l_wordval[1]) { stb->master_volume = p_scan_gdouble(l_wordval[1], 0.0, 10.0, stb); }
      goto cleanup;  /* master RECORD does not create frame range elements */
  }

  /* Master informations: GAP_STBKEY_AUD_MASTER_SAMPLERATE */
  if (strcmp(l_record_key, GAP_STBKEY_AUD_MASTER_SAMPLERATE) == 0)
  {
      if(*l_wordval[1]) { stb->master_samplerate = p_scan_gint32(l_wordval[1], 1, 999999, stb); }
      goto cleanup;  /* master RECORD does not create frame range elements */
  }


  /* Master informations: GAP_STBKEY_LAYOUT_SIZE */
  if (strcmp(l_record_key, GAP_STBKEY_LAYOUT_SIZE) == 0)
  {
      if(*l_wordval[1]) { stb->layout_cols         = p_scan_gint32(l_wordval[1], 1, 999999, stb); }
      if(*l_wordval[2]) { stb->layout_rows         = p_scan_gint32(l_wordval[2], 1, 999999, stb); }
      if(*l_wordval[3]) { stb->layout_thumbsize    = p_scan_gint32(l_wordval[3], 1, 999999, stb); }
      goto cleanup;  /* master RECORD does not create frame range elements */
  }


  /* check VID_* ATTRIUTE Records -----------------------------------  */
  {
    char *l_track_ptr      = l_wordval[1];
    char *l_from_ptr       = l_wordval[2];
    char *l_to_ptr         = l_wordval[3];
    char *l_dur_ptr        = l_wordval[4];


    /* ATTRIBUTE: GAP_STBKEY_VID_OPACITY */
    if (strcmp(l_record_key, GAP_STBKEY_VID_OPACITY) == 0)
    {
      stb_elem = gap_story_new_elem(GAP_STBREC_ATT_OPACITY);
      if(stb_elem)
      {
        stb_elem->file_line_nr = longlinenr;
        stb_elem->orig_src_line = g_strdup(multi_lines);
        if(*l_track_ptr)    { stb_elem->track = p_scan_gint32(l_track_ptr,  1, GAP_STB_MAX_VID_TRACKS,    stb); }
        if(*l_from_ptr)     { stb_elem->att_value_from = p_scan_gdouble(l_from_ptr, 0.0, 1.0, stb); }
        if(*l_to_ptr)       { stb_elem->att_value_to   = p_scan_gdouble(l_to_ptr,   0.0, 1.0, stb); }
        else                { stb_elem->att_value_to   = stb_elem->att_value_from; }
        if(*l_dur_ptr)      { stb_elem->att_value_dur  = p_scan_gint32(l_dur_ptr,  1, 999, stb); }
        
        gap_story_list_append_elem(stb, stb_elem);
      }
      goto cleanup;
    }

    /* ATTRIBUTE: GAP_STBKEY_VID_ZOOM_X */
    if (strcmp(l_record_key, GAP_STBKEY_VID_ZOOM_X) == 0)
    {
      stb_elem = gap_story_new_elem(GAP_STBREC_ATT_ZOOM_X);
      if(stb_elem)
      {
        stb_elem->file_line_nr = longlinenr;
        stb_elem->orig_src_line = g_strdup(multi_lines);
        if(*l_track_ptr)    { stb_elem->track = p_scan_gint32(l_track_ptr,  1, GAP_STB_MAX_VID_TRACKS,    stb); }
        if(*l_from_ptr)     { stb_elem->att_value_from = p_scan_gdouble(l_from_ptr, 0.0001, 999.9, stb); }
        if(*l_to_ptr)       { stb_elem->att_value_to   = p_scan_gdouble(l_to_ptr,   0.0001, 999.9, stb); }
        else                { stb_elem->att_value_to   = stb_elem->att_value_from; }
        if(*l_dur_ptr)      { stb_elem->att_value_dur  = p_scan_gint32(l_dur_ptr,  1, 999, stb); }
        
        gap_story_list_append_elem(stb, stb_elem);
      }
      goto cleanup;
    }

    /* ATTRIBUTE: GAP_STBKEY_VID_ZOOM_Y */
    if (strcmp(l_record_key, GAP_STBKEY_VID_ZOOM_Y) == 0)
    {
      stb_elem = gap_story_new_elem(GAP_STBREC_ATT_ZOOM_Y);
      if(stb_elem)
      {
        stb_elem->file_line_nr = longlinenr;
        stb_elem->orig_src_line = g_strdup(multi_lines);
        if(*l_track_ptr)    { stb_elem->track = p_scan_gint32(l_track_ptr,  1, GAP_STB_MAX_VID_TRACKS,    stb); }
        if(*l_from_ptr)     { stb_elem->att_value_from = p_scan_gdouble(l_from_ptr, 0.0001, 999.9, stb); }
        if(*l_to_ptr)       { stb_elem->att_value_to   = p_scan_gdouble(l_to_ptr,   0.0001, 999.9, stb); }
        else                { stb_elem->att_value_to   = stb_elem->att_value_from; }
        if(*l_dur_ptr)      { stb_elem->att_value_dur  = p_scan_gint32(l_dur_ptr,  1, 999, stb); }
        
        gap_story_list_append_elem(stb, stb_elem);
      }
      goto cleanup;
    }

    /* ATTRIBUTE: GAP_STBKEY_VID_MOVE_X */
    if (strcmp(l_record_key, GAP_STBKEY_VID_MOVE_X) == 0)
    {
      stb_elem = gap_story_new_elem(GAP_STBREC_ATT_MOVE_X);
      if(stb_elem)
      {
        stb_elem->file_line_nr = longlinenr;
        stb_elem->orig_src_line = g_strdup(multi_lines);
        if(*l_track_ptr)    { stb_elem->track = p_scan_gint32(l_track_ptr,  1, GAP_STB_MAX_VID_TRACKS,    stb); }
        if(*l_from_ptr)     { stb_elem->att_value_from = p_scan_gdouble(l_from_ptr, -99999.9, 99999.9, stb); }
        if(*l_to_ptr)       { stb_elem->att_value_to   = p_scan_gdouble(l_to_ptr,   -99999.9, 99999.9, stb); }
        else                { stb_elem->att_value_to   = stb_elem->att_value_from; }
        if(*l_dur_ptr)      { stb_elem->att_value_dur  = p_scan_gint32(l_dur_ptr,  1, 999, stb); }
        
        gap_story_list_append_elem(stb, stb_elem);
      }
      goto cleanup;
    }

    /* ATTRIBUTE: GAP_STBKEY_VID_MOVE_Y */
    if (strcmp(l_record_key, GAP_STBKEY_VID_MOVE_Y) == 0)
    {
      stb_elem = gap_story_new_elem(GAP_STBREC_ATT_MOVE_Y);
      if(stb_elem)
      {
        stb_elem->file_line_nr = longlinenr;
        stb_elem->orig_src_line = g_strdup(multi_lines);
        if(*l_track_ptr)    { stb_elem->track = p_scan_gint32(l_track_ptr,  1, GAP_STB_MAX_VID_TRACKS,    stb); }
        if(*l_from_ptr)     { stb_elem->att_value_from = p_scan_gdouble(l_from_ptr, -99999.9, 99999.9, stb); }
        if(*l_to_ptr)       { stb_elem->att_value_to   = p_scan_gdouble(l_to_ptr,   -99999.9, 99999.9, stb); }
        else                { stb_elem->att_value_to   = stb_elem->att_value_from; }
        if(*l_dur_ptr)      { stb_elem->att_value_dur  = p_scan_gint32(l_dur_ptr,  1, 999, stb); }
        
        gap_story_list_append_elem(stb, stb_elem);
      }
      goto cleanup;
    }
 
 
    /* ATTRIBUTE: GAP_STBKEY_VID_FIT_SIZE */
    if (strcmp(l_record_key, GAP_STBKEY_VID_FIT_SIZE) == 0)
    {
      stb_elem = gap_story_new_elem(GAP_STBREC_ATT_FIT_SIZE);
      if(stb_elem)
      {
        stb_elem->file_line_nr = longlinenr;
        stb_elem->orig_src_line = g_strdup(multi_lines);
        stb_elem->att_keep_proportions = FALSE;
        stb_elem->att_fit_width = TRUE;
        stb_elem->att_fit_height = TRUE;

        if(*l_track_ptr)    { stb_elem->track = p_scan_gint32(l_track_ptr,  1, GAP_STB_MAX_VID_TRACKS,    stb); }
        if(*l_from_ptr != '\0')
        {
          if((strncmp(l_from_ptr, "none", strlen("none")) ==0)
          || (strncmp(l_from_ptr, "NONE", strlen("NONE")) ==0))
          {
            stb_elem->att_fit_width = FALSE;
            stb_elem->att_fit_height = FALSE;
          }
          else
          {
            if((strncmp(l_from_ptr, "both", strlen("both")) ==0)
            || (strncmp(l_from_ptr, "BOTH", strlen("BOTH")) ==0))
            {
              stb_elem->att_fit_width = TRUE;
              stb_elem->att_fit_height = TRUE;
            }
            else
            {
              if((strncmp(l_from_ptr, "width", strlen("width")) ==0)
              || (strncmp(l_from_ptr, "WIDTH", strlen("WIDTH")) ==0))
              {
                stb_elem->att_fit_width = TRUE;
                stb_elem->att_fit_height = FALSE;
              }
              else
              {
                if((strncmp(l_from_ptr, "height", strlen("height")) ==0)
                || (strncmp(l_from_ptr, "HEIGHT", strlen("HEIGHT")) ==0))
                {
                  stb_elem->att_fit_width = FALSE;
                  stb_elem->att_fit_height = TRUE;
                }
                else
                {
                   char *l_errtxt;
                 
                   l_errtxt = g_strdup_printf(_("illegal keyword: %s (expected keywords are: width, height, both, none")
                                             , l_from_ptr);
                   p_set_stb_error(stb, l_errtxt);
                   g_free(l_errtxt);
                }
              }
            }
          }
        }
        
        if(*l_to_ptr != '\0')
        {
          if((strncmp(l_to_ptr, "keep_prop", strlen("keep_prop")) ==0)
          || (strncmp(l_to_ptr, "KEEP_PROP", strlen("KEEP_PROP")) ==0))
          {
            stb_elem->att_keep_proportions = TRUE;
          }
          else
          {
            if((strncmp(l_to_ptr, "change_prop", strlen("change_prop")) ==0)
            || (strncmp(l_to_ptr, "CHANGE_PROP", strlen("CHANGE_PROP")) ==0))
            {
              stb_elem->att_keep_proportions = FALSE;
            }
            else
            {
                   char *l_errtxt;
                   l_errtxt = g_strdup_printf(_("illegal keyword: %s (expected keywords are: keep_proportions, change_proportions")
                                             , l_to_ptr);
                   p_set_stb_warning(stb, l_errtxt);
                   g_free(l_errtxt);
            }
          }
        }
        
        gap_story_list_append_elem(stb, stb_elem);
      }
      goto cleanup;
      
    }  /* END ATTRIBUTE: GAP_STBKEY_VID_FIT_SIZE */

 
  }  /* END check VID ATTRIBUTES */



  /* IMAGE: GAP_STBKEY_VID_PLAY_IMAGE  */
  if (strcmp(l_record_key, GAP_STBKEY_VID_PLAY_IMAGE) == 0)
  {
    char *l_track_ptr      = l_wordval[1];
    char *l_filename_ptr   = l_wordval[2];
    char *l_nloops_ptr     = l_wordval[3];
    char *l_macro_ptr      = l_wordval[4];

    stb_elem = gap_story_new_elem(GAP_STBREC_VID_IMAGE);
    if(stb_elem)
    {
      stb_elem->file_line_nr = longlinenr;
      stb_elem->orig_src_line = g_strdup(multi_lines);

      if(*l_filename_ptr) { p_flip_dir_seperators(l_filename_ptr); 
                            stb_elem->orig_filename = g_strdup(l_filename_ptr);
                          }      
      if(*l_track_ptr)    { stb_elem->track = p_scan_gint32(l_track_ptr,  1, GAP_STB_MAX_VID_TRACKS,  stb); }
      if(*l_nloops_ptr)   { stb_elem->nloop = p_scan_gint32(l_nloops_ptr, 1, 999999, stb); }
      if(*l_macro_ptr)    { p_flip_dir_seperators(l_macro_ptr);
                            stb_elem->filtermacro_file = g_strdup(l_macro_ptr);
                          }

      p_check_image_numpart(stb_elem, l_filename_ptr);

      gap_story_list_append_elem(stb, stb_elem);
    }
    
    goto cleanup;

  }  /* end VID_PLAY_IMAGE */

  /* FRAMES: GAP_STBKEY_VID_PLAY_FRAMES */
  if (strcmp(l_record_key, GAP_STBKEY_VID_PLAY_FRAMES) == 0)
  {
    char *l_track_ptr      = l_wordval[1];
    char *l_basename_ptr   = l_wordval[2];
    char *l_ext_ptr        = l_wordval[3];
    char *l_from_ptr       = l_wordval[4];
    char *l_to_ptr         = l_wordval[5];
    char *l_pingpong_ptr   = l_wordval[6];
    char *l_nloops_ptr     = l_wordval[7];
    char *l_stepsize_ptr   = l_wordval[8];
    char *l_macro_ptr      = l_wordval[9];

    stb_elem = gap_story_new_elem(GAP_STBREC_VID_FRAMES);
    if(stb_elem)
    {
      stb_elem->file_line_nr = longlinenr;
      stb_elem->orig_src_line = g_strdup(multi_lines);

      if(*l_track_ptr)    { stb_elem->track      = p_scan_gint32(l_track_ptr,   1, GAP_STB_MAX_VID_TRACKS, stb); }
      if(*l_from_ptr)     { stb_elem->from_frame = p_scan_gint32(l_from_ptr,    1, 999999, stb); }
      if(*l_to_ptr)       { stb_elem->to_frame   = p_scan_gint32(l_to_ptr,      1, 999999, stb); }
      if(*l_nloops_ptr)   { stb_elem->nloop      = p_scan_gint32(l_nloops_ptr,  1, 999999, stb); }
      if(*l_stepsize_ptr) { stb_elem->step_density = p_scan_gdouble(l_stepsize_ptr,  0.125, 99.9, stb); }
      if(*l_basename_ptr) { p_flip_dir_seperators(l_basename_ptr); 
                            stb_elem->basename = g_strdup(l_basename_ptr);
                          }
      if(*l_ext_ptr)      { if(*l_ext_ptr == '.') stb_elem->ext = g_strdup(l_ext_ptr);
                            else                  stb_elem->ext = g_strdup_printf(".%s", l_ext_ptr);
                          }
      if(*l_macro_ptr)    { p_flip_dir_seperators(l_macro_ptr);
                            stb_elem->filtermacro_file = g_strdup(l_macro_ptr);
                          }
      if(*l_pingpong_ptr) { if ((strncmp(l_pingpong_ptr, "PINGPONG", strlen("PINGPONG")) == 0)
                            ||  (strncmp(l_pingpong_ptr, "pingpong", strlen("pingpong")) == 0))
                            {
                              stb_elem->playmode = GAP_STB_PM_PINGPONG;
                            }
                          }
      if((*l_basename_ptr) 
      && (*l_from_ptr)
      && (*l_ext_ptr))
      {
	stb_elem->orig_filename = gap_lib_alloc_fname(stb_elem->basename
	                        		     ,stb_elem->from_frame
						     ,stb_elem->ext
						     ); 
      }

      gap_story_list_append_elem(stb, stb_elem);
    }
    
    goto cleanup;

  }   /* end VID_PLAY_FRAMES */

  /* FRAMES: GAP_STBKEY_VID_PLAY_MOVIE */
  if (strcmp(l_record_key, GAP_STBKEY_VID_PLAY_MOVIE) == 0)
  {
    char *l_track_ptr      = l_wordval[1];
    char *l_filename_ptr   = l_wordval[2];
    char *l_from_ptr       = l_wordval[3];
    char *l_to_ptr         = l_wordval[4];
    char *l_pingpong_ptr   = l_wordval[5];
    char *l_nloops_ptr     = l_wordval[6];
    char *l_seltrack_ptr   = l_wordval[7];
    char *l_exact_seek_ptr = l_wordval[8];
    char *l_delace_ptr     = l_wordval[9];
    char *l_stepsize_ptr   = l_wordval[10];
    char *l_macro_ptr      = l_wordval[11];
    char *l_decoder_ptr    = l_wordval[12];

    stb_elem = gap_story_new_elem(GAP_STBREC_VID_MOVIE);
    if(stb_elem)
    {
      stb_elem->file_line_nr = longlinenr;
      stb_elem->orig_src_line = g_strdup(multi_lines);

      if(*l_track_ptr)    { stb_elem->track      = p_scan_gint32(l_track_ptr, 1, GAP_STB_MAX_VID_TRACKS, stb); }
      if(*l_filename_ptr) { p_flip_dir_seperators(l_filename_ptr); 
                            stb_elem->orig_filename = g_strdup(l_filename_ptr);
                          }
      if(*l_from_ptr)     { stb_elem->from_frame = p_scan_gint32(l_from_ptr,    1, 999999, stb); }
      if(*l_to_ptr)       { stb_elem->to_frame   = p_scan_gint32(l_to_ptr,      1, 999999, stb); }
      if(*l_nloops_ptr)   { stb_elem->nloop      = p_scan_gint32(l_nloops_ptr,  1, 999999, stb); }

      if(*l_seltrack_ptr)   { stb_elem->seltrack     = p_scan_gint32(l_seltrack_ptr,  1, 999999, stb); }
      if(*l_exact_seek_ptr) { stb_elem->exact_seek   = p_scan_gint32(l_exact_seek_ptr,  0, 1, stb); }
      if(*l_delace_ptr)     { stb_elem->delace       = p_scan_gdouble(l_delace_ptr, 0.0, 3.0, stb); }
      if(*l_stepsize_ptr)   { stb_elem->step_density = p_scan_gdouble(l_stepsize_ptr,  0.125, 99.9, stb); }

      if(*l_macro_ptr)    { p_flip_dir_seperators(l_macro_ptr);
                            stb_elem->filtermacro_file = g_strdup(l_macro_ptr);
                          }
      if(*l_decoder_ptr)  { stb_elem->preferred_decoder = g_strdup(l_decoder_ptr);
                          }
      if(*l_pingpong_ptr) { if ((strncmp(l_pingpong_ptr, "PINGPONG", strlen("PINGPONG")) == 0)
                            ||  (strncmp(l_pingpong_ptr, "pingpong", strlen("pingpong")) == 0))
                            {
                              stb_elem->playmode = GAP_STB_PM_PINGPONG;
                            }
                          }

      gap_story_list_append_elem(stb, stb_elem);
    }
    
    goto cleanup;

  }   /* end VID_PLAY_MOVIE */

  /* VID_PLAY_COLOR frame(s): GAP_STBKEY_VID_PLAY_COLOR  */
  if (strcmp(l_record_key, GAP_STBKEY_VID_PLAY_COLOR) == 0)
  {
    char *l_track_ptr      = l_wordval[1];
    char *l_red_ptr        = l_wordval[2];
    char *l_green_ptr      = l_wordval[3];
    char *l_blue_ptr       = l_wordval[4];
    char *l_alpha_ptr      = l_wordval[5];
    char *l_nloops_ptr     = l_wordval[6];

    stb_elem = gap_story_new_elem(GAP_STBREC_VID_COLOR);
    if(stb_elem)
    {
      stb_elem->file_line_nr = longlinenr;
      stb_elem->orig_src_line = g_strdup(multi_lines);

      if(*l_track_ptr)    { stb_elem->track = p_scan_gint32(l_track_ptr,  1, GAP_STB_MAX_VID_TRACKS, stb); }
      if(*l_red_ptr)      { stb_elem->color_red = p_scan_gdouble(l_red_ptr, 0.0, 1.0, stb); }
      if(*l_green_ptr)    { stb_elem->color_green = p_scan_gdouble(l_green_ptr, 0.0, 1.0, stb); }
      if(*l_blue_ptr)     { stb_elem->color_blue = p_scan_gdouble(l_blue_ptr, 0.0, 1.0, stb); }
      if(*l_alpha_ptr)    { stb_elem->color_alpha = p_scan_gdouble(l_alpha_ptr, 0.0, 1.0, stb); }
      if(*l_nloops_ptr)   { stb_elem->nloop = p_scan_gint32(l_nloops_ptr, 1, 999999, stb); }

      gap_story_list_append_elem(stb, stb_elem);
    }
    
    goto cleanup;

  }  /* end VID_PLAY_COLOR */

  /* FRAMES: GAP_STBKEY_VID_PLAY_ANIMIMAGE */
  if (strcmp(l_record_key, GAP_STBKEY_VID_PLAY_ANIMIMAGE) == 0)
  {
    char *l_track_ptr      = l_wordval[1];
    char *l_filename_ptr   = l_wordval[2];
    char *l_from_ptr       = l_wordval[3];
    char *l_to_ptr         = l_wordval[4];
    char *l_pingpong_ptr   = l_wordval[5];
    char *l_nloops_ptr     = l_wordval[6];
    char *l_macro_ptr      = l_wordval[7];

    stb_elem = gap_story_new_elem(GAP_STBREC_VID_ANIMIMAGE);
    if(stb_elem)
    {
      stb_elem->file_line_nr = longlinenr;
      stb_elem->orig_src_line = g_strdup(multi_lines);
      if(*l_track_ptr)    { stb_elem->track      = p_scan_gint32(l_track_ptr, 1, GAP_STB_MAX_VID_TRACKS, stb); }
      if(*l_filename_ptr) { p_flip_dir_seperators(l_filename_ptr); 
                            stb_elem->orig_filename = g_strdup(l_filename_ptr);
                          }
      if(*l_from_ptr)     { stb_elem->from_frame = p_scan_gint32(l_from_ptr,    1, 999999, stb); }
      if(*l_to_ptr)       { stb_elem->to_frame   = p_scan_gint32(l_to_ptr,      1, 999999, stb); }
      if(*l_nloops_ptr)   { stb_elem->nloop      = p_scan_gint32(l_nloops_ptr,  1, 999999, stb); }


      if(*l_macro_ptr)    { p_flip_dir_seperators(l_macro_ptr);
                            stb_elem->filtermacro_file = g_strdup(l_macro_ptr);
                          }
      if(*l_pingpong_ptr) { if ((strncmp(l_pingpong_ptr, "PINGPONG", strlen("PINGPONG")) == 0)
                            ||  (strncmp(l_pingpong_ptr, "pingpong", strlen("pingpong")) == 0))
                            {
                              stb_elem->playmode = GAP_STB_PM_PINGPONG;
                            }
                          }

      gap_story_list_append_elem(stb, stb_elem);
    }
    
    goto cleanup;

  }   /* end STORYBOARD_VID_PLAY_ANIMIMAGE */


  /* ATTRIBUTE: GAP_STBKEY_VID_SILENCE */
  if (strcmp(l_record_key, GAP_STBKEY_VID_SILENCE) == 0)
  {
    char *l_track_ptr      = l_wordval[1];
    char *l_nloops_ptr     = l_wordval[2];
    char *l_wait_until_ptr = l_wordval[3];

    stb_elem = gap_story_new_elem(GAP_STBREC_VID_SILENCE);
    if(stb_elem)
    {
      stb_elem->file_line_nr = longlinenr;
      stb_elem->orig_src_line = g_strdup(multi_lines);
      if(*l_track_ptr)      { stb_elem->track      = p_scan_gint32(l_track_ptr,   1, GAP_STB_MAX_VID_TRACKS, stb); }
      if(*l_nloops_ptr)     { stb_elem->nloop      = p_scan_gint32(l_nloops_ptr,  1, 999999, stb); }
      if(*l_wait_until_ptr) { stb_elem->vid_wait_untiltime_sec = p_scan_gdouble(l_wait_until_ptr, 0.0, 999999.9, stb); }

      gap_story_list_append_elem(stb, stb_elem);
    }
    goto cleanup;
  }


  /* ATTRIBUTE: GAP_STBKEY_AUD_SILENCE */
  if (strcmp(l_record_key, GAP_STBKEY_AUD_SILENCE) == 0)
  {
    char *l_track_ptr      = l_wordval[1];
    char *l_dur_sec_ptr    = l_wordval[2];
    char *l_wait_until_ptr = l_wordval[3];

    stb_elem = gap_story_new_elem(GAP_STBREC_AUD_SILENCE);
    if(stb_elem)
    {
      stb_elem->file_line_nr = longlinenr;
      stb_elem->orig_src_line = g_strdup(multi_lines);
      stb_elem->aud_play_from_sec = 0.0;
      if(*l_track_ptr)      { stb_elem->track      = p_scan_gint32(l_track_ptr,   1, GAP_STB_MAX_AUD_TRACKS,    stb); }
      if(*l_dur_sec_ptr)    { stb_elem->aud_play_to_sec        = p_scan_gdouble(l_dur_sec_ptr,  0.0, 999999.9, stb); }
      if(*l_wait_until_ptr) { stb_elem->aud_wait_untiltime_sec = p_scan_gdouble(l_wait_until_ptr, 0.0, 999999.9, stb); }

      gap_story_list_append_elem(stb, stb_elem);
    }
    goto cleanup;
  }  
  
  /* ATTRIBUTE: GAP_STBKEY_AUD_PLAY_SOUND */
  if (strcmp(l_record_key, GAP_STBKEY_AUD_PLAY_SOUND) == 0)
  {
    char *l_track_ptr         = l_wordval[1];
    char *l_filename_ptr      = l_wordval[2];
    char *l_from_sec_ptr      = l_wordval[3];
    char *l_to_sec_ptr        = l_wordval[4];
    char *l_volume_ptr        = l_wordval[5];
    char *l_vol_start_ptr     = l_wordval[6];
    char *l_fade_in_sec_ptr   = l_wordval[7];
    char *l_vol_end_ptr       = l_wordval[8];
    char *l_fade_out_sec_ptr  = l_wordval[9];
    char *l_nloops_ptr        = l_wordval[10];


    stb_elem = gap_story_new_elem(GAP_STBREC_AUD_SOUND);
    if(stb_elem)
    {
      stb_elem->file_line_nr = longlinenr;
      stb_elem->orig_src_line = g_strdup(multi_lines);
      stb_elem->aud_play_to_sec = 9999.9;  /* 9999.9 is default for end of audiofile */
      if(*l_track_ptr)        { stb_elem->track      = p_scan_gint32(l_track_ptr,   1, GAP_STB_MAX_AUD_TRACKS,    stb); }
      if(*l_filename_ptr)     { p_flip_dir_seperators(l_filename_ptr); 
                                stb_elem->aud_filename = g_strdup(l_filename_ptr);
                                stb_elem->orig_filename = g_strdup(l_filename_ptr);
                              }
      if(*l_from_sec_ptr)     { stb_elem->aud_play_from_sec = p_scan_gdouble(l_from_sec_ptr,     0.0, 9999.9, stb); }
      if(*l_to_sec_ptr)       { stb_elem->aud_play_to_sec   = p_scan_gdouble(l_to_sec_ptr,       0.0, 9999.9, stb); }
      if(*l_volume_ptr)       { stb_elem->aud_volume        = p_scan_gdouble(l_volume_ptr,       0.0, 10.0,   stb); }
      if(*l_vol_start_ptr)    { stb_elem->aud_volume_start  = p_scan_gdouble(l_vol_start_ptr,    0.0, 10.0,   stb); }
      if(*l_fade_in_sec_ptr)  { stb_elem->aud_fade_in_sec   = p_scan_gdouble(l_fade_in_sec_ptr,  0.0, 9999.9, stb); }
      if(*l_vol_end_ptr)      { stb_elem->aud_volume_end    = p_scan_gdouble(l_vol_end_ptr,      0.0, 10.0,   stb); }
      if(*l_fade_out_sec_ptr) { stb_elem->aud_fade_out_sec  = p_scan_gdouble(l_fade_out_sec_ptr, 0.0, 9999.9, stb); }
      if(*l_nloops_ptr)       { stb_elem->nloop             = p_scan_gint32(l_nloops_ptr,  1, 999999, stb); }

//printf("\n##++ GAP_STBREC_AUD_SOUND\n");
//gap_story_debug_print_elem(stb_elem);

      gap_story_list_append_elem(stb, stb_elem);
    }
    goto cleanup;
  }  

  
  /* ATTRIBUTE: GAP_STBKEY_AUD_PLAY_MOVIE */
  if (strcmp(l_record_key, GAP_STBKEY_AUD_PLAY_MOVIE) == 0)
  {
    char *l_track_ptr         = l_wordval[1];
    char *l_filename_ptr      = l_wordval[2];
    char *l_from_sec_ptr      = l_wordval[3];
    char *l_to_sec_ptr        = l_wordval[4];
    char *l_volume_ptr        = l_wordval[5];
    char *l_vol_start_ptr     = l_wordval[6];
    char *l_fade_in_sec_ptr   = l_wordval[7];
    char *l_vol_end_ptr       = l_wordval[8];
    char *l_fade_out_sec_ptr  = l_wordval[9];
    char *l_nloops_ptr        = l_wordval[10];
    char *l_seltrack_ptr      = l_wordval[11];
    char *l_decoder_ptr       = l_wordval[12];


    stb_elem = gap_story_new_elem(GAP_STBREC_AUD_MOVIE);
    if(stb_elem)
    {
      stb_elem->file_line_nr = longlinenr;
      stb_elem->orig_src_line = g_strdup(multi_lines);
      stb_elem->aud_play_to_sec = 9999.9;  /* 9999.9 is default for end of audiofile */
      if(*l_track_ptr)        { stb_elem->track      = p_scan_gint32(l_track_ptr,   1, GAP_STB_MAX_AUD_TRACKS,    stb); }
      if(*l_filename_ptr)     { p_flip_dir_seperators(l_filename_ptr); 
                                stb_elem->aud_filename = g_strdup(l_filename_ptr);
                                stb_elem->orig_filename = g_strdup(l_filename_ptr);
                              }
      if(*l_from_sec_ptr)     { stb_elem->aud_play_from_sec = p_scan_gdouble(l_from_sec_ptr,     0.0, 9999.9, stb); }
      if(*l_to_sec_ptr)       { stb_elem->aud_play_to_sec   = p_scan_gdouble(l_to_sec_ptr,       0.0, 9999.9, stb); }
      if(*l_volume_ptr)       { stb_elem->aud_volume        = p_scan_gdouble(l_volume_ptr,       0.0, 10.0,   stb); }
      if(*l_vol_start_ptr)    { stb_elem->aud_volume_start  = p_scan_gdouble(l_vol_start_ptr,    0.0, 10.0,   stb); }
      if(*l_fade_in_sec_ptr)  { stb_elem->aud_fade_in_sec   = p_scan_gdouble(l_fade_in_sec_ptr,  0.0, 9999.9, stb); }
      if(*l_vol_end_ptr)      { stb_elem->aud_volume_end    = p_scan_gdouble(l_vol_end_ptr,      0.0, 10.0,   stb); }
      if(*l_fade_out_sec_ptr) { stb_elem->aud_fade_out_sec  = p_scan_gdouble(l_fade_out_sec_ptr, 0.0, 9999.9, stb); }
      if(*l_nloops_ptr)       { stb_elem->nloop             = p_scan_gint32(l_nloops_ptr,    1, 999999, stb); }
      if(*l_seltrack_ptr)     { stb_elem->aud_seltrack      = p_scan_gint32(l_seltrack_ptr,  1, 999999, stb); }
      if(*l_decoder_ptr)      { stb_elem->preferred_decoder = g_strdup(l_decoder_ptr);
                              }

      gap_story_list_append_elem(stb, stb_elem);
    }
    goto cleanup;
  }  


  /* accept lines with filenames as implicite VID_PLAY_IMAGE commands */
  {
    char *l_filename_ptr;
    
    l_filename_ptr = g_strdup(l_record_key);
    if(l_filename_ptr)
    {
      p_flip_dir_seperators(l_filename_ptr);

      if(g_file_test(l_filename_ptr, G_FILE_TEST_EXISTS))
      {
        stb_elem = gap_story_new_elem(GAP_STBREC_VID_IMAGE);

	if(stb_elem)
	{
	  stb_elem->file_line_nr = longlinenr;
	  stb_elem->orig_filename = g_strdup(l_record_key); 
          stb_elem->orig_src_line = g_strdup(multi_lines);

          p_check_image_numpart(stb_elem, l_filename_ptr);
 
	  gap_story_list_append_elem(stb, stb_elem);
	}

	goto cleanup;
      }
      else
      {
        g_free(l_filename_ptr);
      }
    }
  }


  /* Unsupported Lines */
  if (stb->master_type == GAP_STB_MASTER_TYPE_UNDEFINED)
  {
    /* unsupported lines raise an error for files without correct Header
     */
    p_set_stb_error(stb, _("Unsupported line was igonored"));
  }
  else
  {
    /* accept unsupported lines (with just a warning)
     * because the file has correct Header
     */
    p_set_stb_warning(stb, _("Unsupported line was igonored"));
  }


  stb_elem = gap_story_new_elem(GAP_STBREC_VID_UNKNOWN);
  if(stb_elem)
  {
      stb_elem->orig_src_line = g_strdup(multi_lines);
      stb_elem->file_line_nr = longlinenr;

      gap_story_list_append_elem(stb, stb_elem);
  }


cleanup:
  for(ii=0; ii < GAP_MAX_STB_PARAMS_PER_LINE; ii++)
  {
    g_free(l_wordval[ii]);
    l_wordval[ii] = NULL;
  }

}  /* end p_story_parse_line */


/* ------------------------------
 * gap_story_parse
 * ------------------------------
 * Parse the STORYBOARD or CLIPBOARD File
 * and RETURN the filecontent converted to a
 * GapStoryBoard List structure (that should be freed by the caller
 * after use with the procedure gap_story_free)
 *
 * This parser makes only syntactical checks, but does not verify
 * if the files or frames are existent and accessable.
 *
 * The 1.st error that occured is reported in the errtext pointer
 * errtext is a member of the returned GapStoryBoard structure
 * (if no error occured the errtxt pointer is  NULL)
 *
 * The caller is responsible to free the returned GapStoryBoard structure.
 * this should be done by calling gap_story_free_storyboard after usage.
 */
GapStoryBoard *
gap_story_parse(const gchar *filename)
{
  GapVinTextFileLines *txf_ptr_root;
  GapVinTextFileLines *txf_ptr;
  GapStoryBoard *stb;
  gchar *longline;
  gchar *multi_lines;
  gint32 longlinenr;
  gint32 line_nr;
  
  stb = gap_story_new_story_board(filename);
  if(stb == NULL)
  {
    return (NULL);
  }
  txf_ptr_root = gap_vin_load_textfile(filename);

  longline = NULL;
  multi_lines = NULL;
  longlinenr = 0;
  line_nr = 0;
  for(txf_ptr = txf_ptr_root; txf_ptr != NULL; txf_ptr = (GapVinTextFileLines *) txf_ptr->next)
  {
    gint l_len;

    line_nr++;
    if(gap_debug) printf("line_nr: %d\n", (int)line_nr);
    
    l_len = strlen(txf_ptr->line);

    /* chop \n */
    if(txf_ptr->line[l_len-1] == '\n')
    {
      txf_ptr->line[l_len-1] = '\0';
      l_len--;
    }
    if(txf_ptr->line[l_len-1] == '\a')
    {
      txf_ptr->line[l_len-1] = '\0';
      l_len--;
    }

    /* concatenate long lines */
    if(multi_lines == NULL)
    {
      multi_lines = g_strdup(txf_ptr->line);
    }
    else
    {
      gchar *l_ml;
      
      l_ml = g_strdup_printf("%s\n%s", multi_lines, txf_ptr->line);
      g_free(multi_lines);
      multi_lines = l_ml;
    }
    
    /* handle long lines with backslash at line end
     * concatenate those lines with blank inbetween for
     * the following syntax check.
     */
    if (txf_ptr->line[l_len-1] == '\\')
    {
      if(longline == NULL) 
      {
        longline = g_strdup(txf_ptr->line);
      }
      else
      {
        char *l_line;
        
        l_line = g_strdup_printf("%s %s", longline, txf_ptr->line);
        g_free(longline);
        longline = l_line;
      }
      if(longlinenr == 0) 
      {
        longlinenr = line_nr;
      }
    }
    else
    {
      if(longline == NULL)
      {
        p_story_parse_line(stb, txf_ptr->line, line_nr, multi_lines);
      }
      else
      {
        char *l_line;
        
        l_line = g_strdup_printf("%s %s", longline, txf_ptr->line);
        g_free(longline);
        longline = NULL;
        p_story_parse_line(stb, l_line, longlinenr, multi_lines);
        g_free(l_line);
      }
      longlinenr = 0;
      if(multi_lines)
      {
        g_free(multi_lines);
	multi_lines = NULL;
      }
    }
  }
  if(longline)
  {
    p_story_parse_line(stb, longline, longlinenr, multi_lines);
    if(multi_lines)
    {
        g_free(multi_lines);
    }
  }

  if(txf_ptr_root)
  {
    gap_vin_free_textfile_lines(txf_ptr_root);
  }

  
  /* calculate nframes 
   *  nframes is the number of frames to handle (by a player or encoder)
   *  to process the stb_elem respecting nloops and playmode
   */
  if(stb->stb_elem == NULL)
  {
    char *l_errtext;


    stb->currline = "\0";
    l_errtext = g_strdup_printf(_("the passed filename %s "
                                  "has irrelevant content or could not be opened by the parser" )
				  , filename);
    p_set_stb_error(stb, l_errtext);
    g_free(l_errtext);
  }
  else
  {
    GapStoryElem *stb_elem;

    if(gap_debug) printf("gap_story_parse: CALCULATE NFRAMES\n");
    
    for(stb_elem = stb->stb_elem; stb_elem != NULL; stb_elem = (GapStoryElem *)stb_elem->next)
    {
      if(gap_debug) 
      {
        printf("\nCALCULATE NFRAMES for stb_elem: %d\n", (int)stb_elem);
	gap_story_debug_print_elem(stb_elem);
      }
      
      if(stb_elem->nframes <= 0)
      {
        gap_story_elem_calculate_nframes(stb_elem);
      }
    }
  }

  if(gap_debug) printf("gap_story_parse: RET ptr:%d\n", (int)stb);

  stb->unsaved_changes = FALSE;
  return(stb);  
}  /* end gap_story_parse */


/* --------------------------------
 * gap_story_elem_calculate_nframes
 * --------------------------------
 */
void 
gap_story_elem_calculate_nframes(GapStoryElem *stb_elem)
{
  if((stb_elem->record_type == GAP_STBREC_VID_SILENCE)
  || (stb_elem->record_type == GAP_STBREC_VID_IMAGE)
  || (stb_elem->record_type == GAP_STBREC_VID_COLOR))
  {
    stb_elem->nframes = stb_elem->nloop;
  }
  if((stb_elem->record_type == GAP_STBREC_VID_FRAMES)
  || (stb_elem->record_type == GAP_STBREC_VID_MOVIE))
  {
    /* calculate number of frames in this clip
     * respecting playmode and nloop count
     */
    if ((stb_elem->playmode == GAP_STB_PM_PINGPONG) 
    &&  (stb_elem->from_frame != stb_elem->to_frame))
    {
      //// stb_elem->nframes = (stb_elem->nloop * 2 * ABS(stb_elem->from_frame - stb_elem->to_frame));
      stb_elem->nframes = p_calculate_rangesize_pingpong(stb_elem) * stb_elem->nloop;
    }
    else
    {
      //// stb_elem->nframes = stb_elem->nloop * (ABS(stb_elem->from_frame - stb_elem->to_frame) + 1);
      stb_elem->nframes = p_calculate_rangesize(stb_elem) * stb_elem->nloop;
    }
  }
}  /* end gap_story_elem_calculate_nframes */


/* XXXXXXXXXXXXX Save Storyboard from Liststructure to File XXXXXXXXXXXXXX */


/* ------------------------------
 * p_story_save_fprint_comment
 * ------------------------------
 */
static void
p_story_save_fprint_comment(FILE *fp, GapStoryElem *stb_elem)
{
  /* Print a comment line (add # character if needed) */
  if(stb_elem->orig_src_line)
  {
    if(*stb_elem->orig_src_line != '#')
    {
      fprintf(fp, "#");
    }
    fprintf(fp, "%s", stb_elem->orig_src_line);
  }
  fprintf(fp, "\n");
}  /* end p_story_save_fprint_comment */

/* ------------------------------
 * p_story_save_comments
 * ------------------------------
 */
static void
p_story_save_comments(FILE *fp, GapStoryElem *stb_elem_node)
{
  GapStoryElem *stb_elem;
  
  if(stb_elem_node)
  {
    for(stb_elem = stb_elem_node->comment; stb_elem != NULL; stb_elem = (GapStoryElem *)stb_elem->next)
    {
      p_story_save_fprint_comment(fp, stb_elem);
    }
  }
  
}  /* end p_story_save_comments */

/* ------------------------------
 * gap_story_save
 * ------------------------------
 */
gboolean
gap_story_save(GapStoryBoard *stb, const char *filename)
{
  FILE *fp;
  GapStoryElem *stb_elem;
      
  
  fp = fopen(filename, "w");
  if(fp)
  {
    GapStbSyntaxParnames l_parnam_tab;
    
    /* print Header */
    if(stb->master_type == GAP_STB_MASTER_TYPE_STORYBOARD)
    {
      fprintf(fp, "%s\n", GAP_STBKEY_STB_HEADER);
    }
    else
    {
      fprintf(fp, "%s\n", GAP_STBKEY_CLIP_HEADER);
    }
    
    /* print MASTER_SIZE */
    if((stb->master_width > 0) || (stb->master_height > 0))
    {
      gap_stb_syntax_get_parname_tab(GAP_STBKEY_VID_MASTER_SIZE
                                    ,&l_parnam_tab
				    );
      fprintf(fp, "%s         %s:%d %s:%d\n"
           , GAP_STBKEY_VID_MASTER_SIZE
	   , l_parnam_tab.parname[1]
           , (int)stb->master_width
	   , l_parnam_tab.parname[2]
           , (int)stb->master_height
           );
    }

    /* print MASTER_FRAMERATE */
    if(stb->master_framerate > 0.0)
    {
       gchar l_dbl_str[G_ASCII_DTOSTR_BUF_SIZE];

       /* setlocale independent float string */
       g_ascii_dtostr(&l_dbl_str[0]
                     ,G_ASCII_DTOSTR_BUF_SIZE
                     ,stb->master_framerate
                     );
       gap_stb_syntax_get_parname_tab(GAP_STBKEY_VID_MASTER_FRAMERATE
                                    ,&l_parnam_tab
				    );
       fprintf(fp, "%s    %s:%s\n"
           , GAP_STBKEY_VID_MASTER_FRAMERATE
	   , l_parnam_tab.parname[1]
           , l_dbl_str
           );
    }

    if((stb->layout_cols > 0) || (stb->layout_rows > 0) || (stb->layout_thumbsize > 0))
    {
      /* print LAYOUT_SIZE */
      gap_stb_syntax_get_parname_tab(GAP_STBKEY_LAYOUT_SIZE
                                    ,&l_parnam_tab
				    );
      fprintf(fp, "%s         %s:%d %s:%d %s:%d\n"
           , GAP_STBKEY_LAYOUT_SIZE
 	   , l_parnam_tab.parname[1]
           , (int)stb->layout_cols
 	   , l_parnam_tab.parname[2]
           , (int)stb->layout_rows
 	   , l_parnam_tab.parname[3]
           , (int)stb->layout_thumbsize
           );
     }
    
    /* print PREFERRED_DECODER */
    if(stb->preferred_decoder > 0)
    {
      if(*stb->preferred_decoder != '\0')
      {
	gap_stb_syntax_get_parname_tab(GAP_STBKEY_VID_PREFERRED_DECODER
                                      ,&l_parnam_tab
				      );
	fprintf(fp, "%s   %s:\"%s\"\n"
             , GAP_STBKEY_VID_PREFERRED_DECODER
	     , l_parnam_tab.parname[1]
             , stb->preferred_decoder
             );
      }
    }
   
    for(stb_elem = stb->stb_elem; stb_elem != NULL; stb_elem = (GapStoryElem *)stb_elem->next)
    {
      gchar l_dbl_str_step_dendity[G_ASCII_DTOSTR_BUF_SIZE];

      /* setlocale independent float string */
      g_ascii_dtostr(&l_dbl_str_step_dendity[0]
                     ,G_ASCII_DTOSTR_BUF_SIZE
                     ,stb_elem->step_density
                     );

      switch(stb_elem->record_type)
      {
        case GAP_STBREC_VID_FRAMES:
          {
            gchar *l_playmode_str;
            
            p_story_save_comments(fp, stb_elem);
            l_playmode_str = NULL;
            if(stb_elem->playmode == GAP_STB_PM_PINGPONG)
            {
              l_playmode_str = g_strdup("pingpong");
            }
            else
            {
              l_playmode_str = g_strdup("normal");
            }

            
            gap_stb_syntax_get_parname_tab(GAP_STBKEY_VID_PLAY_FRAMES
                                    ,&l_parnam_tab
				    );
            fprintf(fp, "%s     %s:%d %s:\"%s\" %s:%s %s:%06d %s:%06d %s:%s %s:%d %s:%s"
                 , GAP_STBKEY_VID_PLAY_FRAMES
 	         , l_parnam_tab.parname[1]
                 , (int)stb_elem->track
 	         , l_parnam_tab.parname[2]
                 , stb_elem->basename
 	         , l_parnam_tab.parname[3]
                 , stb_elem->ext
 	         , l_parnam_tab.parname[4]
                 , (int)stb_elem->from_frame
 	         , l_parnam_tab.parname[5]
                 , (int)stb_elem->to_frame
 	         , l_parnam_tab.parname[6]
                 , l_playmode_str
 	         , l_parnam_tab.parname[7]
                 , (int)stb_elem->nloop
		 , l_parnam_tab.parname[8]
		 , l_dbl_str_step_dendity
                 );
            g_free(l_playmode_str);

            if(stb_elem->filtermacro_file)
            {
              fprintf(fp, " %s:\"%s\""
 	         , l_parnam_tab.parname[9]
                 , stb_elem->filtermacro_file
                 );
            }
            fprintf(fp, "\n");
          }
	  if(gap_debug)
	  {
	    printf("\nSAVED stb_elem\n");
	    gap_story_debug_print_elem(stb_elem);
	  }
          break;
        case GAP_STBREC_VID_IMAGE:
          gap_stb_syntax_get_parname_tab(GAP_STBKEY_VID_PLAY_IMAGE
                                    ,&l_parnam_tab
				    );
          p_story_save_comments(fp, stb_elem);
          fprintf(fp, "%s      %s:%d %s:\"%s\" %s:%d"
                 , GAP_STBKEY_VID_PLAY_IMAGE
 	         , l_parnam_tab.parname[1]
                 , (int)stb_elem->track
 	         , l_parnam_tab.parname[2]
                 , stb_elem->orig_filename
 	         , l_parnam_tab.parname[3]
                 , (int)stb_elem->nloop
                 );
          if(stb_elem->filtermacro_file)
          {
            fprintf(fp, " %s:\"%s\""
 	         , l_parnam_tab.parname[4]
                 , stb_elem->filtermacro_file
                 );
          }
          fprintf(fp, "\n");
	  if(gap_debug)
	  {
	    printf("\nSAVED stb_elem\n");
	    gap_story_debug_print_elem(stb_elem);
	  }
          break;
        case GAP_STBREC_VID_MOVIE:
          {
            gchar *l_playmode_str;
	    gint   l_delace_int;
            
            gap_stb_syntax_get_parname_tab(GAP_STBKEY_VID_PLAY_MOVIE
                                    ,&l_parnam_tab
				    );
            p_story_save_comments(fp, stb_elem);
            l_playmode_str = NULL;
            if(stb_elem->playmode == GAP_STB_PM_PINGPONG)
            {
              l_playmode_str = g_strdup("pingpong");
            }
            else
            {
              l_playmode_str = g_strdup("normal");
            }
            l_delace_int = (int)stb_elem->delace;
            fprintf(fp, "%s      %s:%d %s:\"%s\" %s:%06d %s:%06d %s:%s %s:%d "
	                "%s:%d %s:%d %s:%d.%d %s:%s"
                 , GAP_STBKEY_VID_PLAY_MOVIE
 	         , l_parnam_tab.parname[1]
                 , (int)stb_elem->track
 	         , l_parnam_tab.parname[2]
                 , stb_elem->orig_filename
 	         , l_parnam_tab.parname[3]
                 , (int)stb_elem->from_frame
 	         , l_parnam_tab.parname[4]
                 , (int)stb_elem->to_frame
 	         , l_parnam_tab.parname[5]
                 , l_playmode_str
 	         , l_parnam_tab.parname[6]
                 , (int)stb_elem->nloop
 	         , l_parnam_tab.parname[7]
		 , (int)stb_elem->seltrack
 	         , l_parnam_tab.parname[8]
		 , (int)stb_elem->exact_seek
 	         , l_parnam_tab.parname[9]
		 , (int)l_delace_int
		 , (int)((gdouble)(1000.0 * (stb_elem->delace - (gdouble)l_delace_int)))
		 , l_parnam_tab.parname[10]
		 , l_dbl_str_step_dendity
                 );
            g_free(l_playmode_str);

            if(stb_elem->filtermacro_file)
            {
              fprintf(fp, " %s:\"%s\""
 	         , l_parnam_tab.parname[11]
                 , stb_elem->filtermacro_file
                 );
            }
            if(stb_elem->preferred_decoder)
            {
              fprintf(fp, " %s:\"%s\""
 	         , l_parnam_tab.parname[12]
                 , stb_elem->preferred_decoder
                 );
            }
            fprintf(fp, "\n");
          }
	  if(gap_debug)
	  {
	    printf("\nSAVED stb_elem\n");
	    gap_story_debug_print_elem(stb_elem);
	  }
          break;
        case GAP_STBREC_VID_COMMENT:
          p_story_save_fprint_comment(fp, stb_elem);
          break;
        case GAP_STBREC_VID_UNKNOWN:
        default:
          p_story_save_comments(fp, stb_elem);
          /* keep unknown lines 1:1 as it was at read
           * for compatibility to STORYBOARD Level 2 files and future extensions
           */
          fprintf(fp, "%s\n", stb_elem->orig_src_line);
          break;
      }
    }
    
    fclose(fp);
    stb->unsaved_changes = FALSE;
    return (TRUE);
  }
   
  return (FALSE);
}  /* end gap_story_save */




/* XXXXXXXXXXXX ACCESS to Storyboard frames XXXXXXXXXXXXXXXXXXXXX */

/* ------------------------------
 * p_story_get_filename_from_elem
 * ------------------------------
 * call this procedure with in_framenr == -1
 * to get the first frame_nr of the Clip represented by stb_elem
 */
static char *
p_story_get_filename_from_elem (GapStoryElem *stb_elem, gint32 in_framenr)
{
  char *filename;
  
  filename = NULL;
  if(stb_elem)
  {
    switch(stb_elem->record_type)
    {
      case GAP_STBREC_VID_MOVIE:
      case GAP_STBREC_VID_IMAGE:
        filename = g_strdup(stb_elem->orig_filename);
	break;
      case GAP_STBREC_VID_FRAMES:
	if(in_framenr < 0)
	{
	  in_framenr = stb_elem->from_frame;
	}
	filename = gap_lib_alloc_fname(stb_elem->basename
	                              , in_framenr
				      , stb_elem->ext
				      );

	break;
      default:
	break;
    }
  }
  return(filename);
}  /* end p_story_get_filename_from_elem */


/* --------------------------------
 * gap_story_get_filename_from_elem
 * gap_story_get_filename_from_elem_nr
 * --------------------------------
 */
char *
gap_story_get_filename_from_elem (GapStoryElem *stb_elem)
{
  return(p_story_get_filename_from_elem(stb_elem, -1));
}  /* end gap_story_get_filename_from_elem */

char *
gap_story_get_filename_from_elem_nr (GapStoryElem *stb_elem, gint32 in_framenr)
{
  return(p_story_get_filename_from_elem(stb_elem, in_framenr));
}  /* end gap_story_get_filename_from_elem_nr */


/* ------------------------------
 * gap_story_locate_framenr
 * ------------------------------
 * IN: stbd_list  The Liststructure that represents the Storyboardfile
 * IN: in_framenr The Framenumber in sequence to locate
 * IN: in_track   The Video Track (for later use with multiple tracksupport in storyboard level2)
 *                for level1 storyboard files the caller always should supply
 *                the value 1 for in_track.
 * OUT:
 *   the returned structure GapStoryLocateRet contains a pointer
 *   to the corresponding GapStoryElem (dont gfree that stb_elem pointer)
 *   the caller can build the full frame name by concatenating
 *     stb_elem->basename + ret_framenr + extension (for record type FRAMES)
 *     stb_elem->basename                           (for record type SINGLEIMAGE)
 *
 *   locate_ok flag == FALSE indicates that the given in_framenr was not
 *                     within valid range
 */
GapStoryLocateRet *
gap_story_locate_framenr(GapStoryBoard *stb, gint32 in_framenr, gint32 in_track)
{
  GapStoryLocateRet *ret_elem;
  GapStoryElem      *stb_elem;
  gint32             l_framenr;
  
  ret_elem = g_new(GapStoryLocateRet, 1);
  ret_elem->stb_elem = NULL;
  ret_elem->locate_ok = FALSE;

  l_framenr = in_framenr;
  if (l_framenr < 1)
  {
    return(ret_elem);
  }
  
  for(stb_elem = stb->stb_elem; stb_elem != NULL;  stb_elem = stb_elem->next)
  {
    if(stb_elem->track != in_track)
    {
      continue;
    }
  
    if (l_framenr > stb_elem->nframes)
    {
      /* the wanted in_framenr is not within the range of the current stb_elem */
      l_framenr -= stb_elem->nframes;
    }
    else
    {
      gint32  l_rangesize;
      gdouble fnr;
      
      l_rangesize = p_calculate_rangesize(stb_elem);
      
      /* the wanted in_framenr is within the range of the current stb_elem
       * now figure out the local framenr
       *  (respecting normal|invers order, nloop count and normal|pingpong playmode
       *   and step_density setting)
       */
      if (stb_elem->to_frame >= stb_elem->from_frame)
      {
        /* normal range */
        if (stb_elem->playmode == GAP_STB_PM_PINGPONG)
        {
          ret_elem->ret_framenr = p_calculate_pingpong_framenr(l_framenr
	                                                      ,l_rangesize
							      ,stb_elem->step_density
							      );
        }
        else
        {
	  fnr = (gdouble)((l_framenr -1) % l_rangesize) * stb_elem->step_density;
	  ret_elem->ret_framenr = 1 + (gint32)fnr;
        }
        ret_elem->ret_framenr += (stb_elem->from_frame -1); /* add rangeoffset */
      }
      else
      {
        /* inverse range */
        if (stb_elem->playmode == GAP_STB_PM_PINGPONG)
        {
          ret_elem->ret_framenr = 1 + l_rangesize - p_calculate_pingpong_framenr(l_framenr
	                                                                        ,l_rangesize
										,stb_elem->step_density
										);
        }
        else
        {
	  fnr = (gdouble)((l_rangesize - l_framenr) % l_rangesize) * stb_elem->step_density;
	  ret_elem->ret_framenr = 1 + (gint32)fnr;
        }
        ret_elem->ret_framenr += (stb_elem->to_frame -1);  /* add rangeoffset */
      }
      ret_elem->locate_ok = TRUE;
      ret_elem->stb_elem = stb_elem;
      break;
    }
  }
  
  return(ret_elem);
  
}  /* end gap_story_locate_framenr */


/* ---------------------------------
 * gap_story_get_framenr_by_story_id
 * ---------------------------------
 * return framnumber of the 1.st frame of stb_elem with story_id
 * for the 1.st stb_elem in the list, 1 is returned.
 */
gint32
gap_story_get_framenr_by_story_id(GapStoryBoard *stb, gint32 story_id)
{
  GapStoryElem      *stb_elem;
  gint32             l_framenr;
  gint32             in_track;

  l_framenr = 1;
  in_track = 1;
  
  for(stb_elem = stb->stb_elem; stb_elem != NULL;  stb_elem = stb_elem->next)
  {
    if(stb_elem->track != in_track)
    {
      continue;
    }
    if(stb_elem->story_id == story_id)
    {
      break;
    }
    gap_story_elem_calculate_nframes(stb_elem);
    l_framenr += stb_elem->nframes;
  }
  return(l_framenr);
  
}  /* end gap_story_get_framenr_by_story_id */ 

/* ------------------------------
 * gap_story_fetch_filename
 * ------------------------------
 * return filename of the requested frame
 * or NULL in case when no frame was found for the requested framenr
 */
char *
gap_story_fetch_filename(GapStoryBoard *stb, gint32 framenr)
{
  GapStoryLocateRet *stb_ret;
  char *filename;
  
  filename = NULL;
  stb_ret = gap_story_locate_framenr(stb
                                    , framenr
				    , 1              /* gint32 in_track */
				    );
  if(stb_ret)
  {
     if((stb_ret->locate_ok)
     && (stb_ret->stb_elem))
     {
       filename = p_story_get_filename_from_elem(stb_ret->stb_elem
                                               , stb_ret->ret_framenr
					       );
     }
     g_free(stb_ret);
  }
  return (filename);
}  /* end gap_story_fetch_filename */ 


/* -----------------------------
 * gap_story_get_master_size
 * -----------------------------
 * deliver master_width and master_height
 *
 * if no master size information is present
 * in the storyboard file
 * then deliver size of the 1.st frame
 * that will be fetched to peek the size.
 */
void
gap_story_get_master_size(GapStoryBoard *stb
                         ,gint32 *width
			 ,gint32 *height)
{
  char *filename;
 
  *width = MAX(stb->master_width, 0);
  *height = MAX(stb->master_height, 0);

  if((stb->master_width > 0)
  && (stb->master_height > 0))
  {
    return;
  }
  
  /* load 1.st image to peek width/height
   * (that were not supplied as master size information
   * in the storyboard file)
   */
  {
    GapStoryLocateRet *stb_ret;
    
    stb_ret = gap_story_locate_framenr(stb
                                    , 1     /* framenr */
				    , 1     /* gint32 in_track */
				    );
    if(stb_ret)
    {
      if((stb_ret->locate_ok)
      && (stb_ret->stb_elem))
      {
        switch(stb_ret->stb_elem->record_type)
	{
	  case GAP_STBREC_VID_IMAGE:
	  case GAP_STBREC_VID_ANIMIMAGE:
	  case GAP_STBREC_VID_FRAMES:
            filename = p_story_get_filename_from_elem(stb_ret->stb_elem
                                               , stb_ret->ret_framenr
					       );
	    if(filename)
	    {
	      gint32 l_image_id;

	      l_image_id = gap_lib_load_image(filename);
	      if(l_image_id >= 0)
	      {
		*width = gimp_image_width(l_image_id);;
		*height = gimp_image_height(l_image_id);

		gimp_image_delete(l_image_id);
	      }
	      g_free(filename);
	    }
	    break;
	  case GAP_STBREC_VID_MOVIE:
	    // TODO: open videofile to get width/height from 1.st frame
	    break;
	  default:
	    break;
	}
      }
      g_free(stb_ret);
    }
  }
  
  if(*width < 1)
  {
    *width = MAX(stb->master_width, 320);
    *height = MAX(stb->master_height, 200);
  }
  
  
}  /* end gap_story_get_master_size */

/* -----------------------------
 * gap_story_fake_ainfo_from_stb
 * -----------------------------
 * returned a faked GapAnimInfo structure
 * that tells about the total frames in the storyboard level1 file 
 *
 * all char pointers are set to NULL
 * the caller is responsible to free the returned struct.
 */
GapAnimInfo * 
gap_story_fake_ainfo_from_stb(GapStoryBoard *stb)
{
  GapAnimInfo   *l_ainfo_ptr;
  GapStoryElem      *stb_elem;
  gint32             l_framenr;

  if(stb == NULL)
  {
    return(NULL);
  }

  l_ainfo_ptr = (GapAnimInfo*)g_malloc0(sizeof(GapAnimInfo));
  
  if(l_ainfo_ptr)
  {
    gint32 l_width;
    gint32 l_height;

    l_ainfo_ptr->basename = NULL;
    l_ainfo_ptr->new_filename = NULL;
    l_ainfo_ptr->extension = NULL;
    l_ainfo_ptr->image_id = -1;
    l_ainfo_ptr->old_filename = NULL;
    l_ainfo_ptr->basename = NULL;
    l_ainfo_ptr->extension = NULL;

    /* l_ainfo_ptr->run_mode = XXXXXXXXXXX; */

    gap_story_get_master_size(stb
                               ,&l_width
			       ,&l_height
			       );
    l_ainfo_ptr->width = l_width;
    l_ainfo_ptr->height = l_height;

    l_framenr = 0;
    for(stb_elem = stb->stb_elem; stb_elem != NULL;  stb_elem = stb_elem->next)
    {
      if((stb_elem->track != 1)
      || (stb_elem->record_type == GAP_STBREC_VID_COMMENT)
      || (stb_elem->record_type == GAP_STBREC_VID_UNKNOWN))
      {
	continue;
      }
      l_framenr += stb_elem->nframes;
    }

    l_ainfo_ptr->curr_frame_nr = 1;
    l_ainfo_ptr->first_frame_nr = 1;
    l_ainfo_ptr->frame_cnt = l_framenr;
    l_ainfo_ptr->last_frame_nr = l_framenr;

    if(gap_debug) printf("gap_story_fake_ainfo_from_stb: RES l_framenr:%d\n", (int)l_framenr);
    
    
  }
  
  return(l_ainfo_ptr);
  
}  /* end gap_story_fake_ainfo_from_stb */


/* -----------------------------
 * gap_story_elem_duplicate
 * -----------------------------
 * make a duplicate of the element (without the comment sublists)
 */
GapStoryElem *     
gap_story_elem_duplicate(GapStoryElem      *stb_elem)
{
  GapStoryElem      *stb_elem_dup;

  stb_elem_dup = gap_story_new_elem(stb_elem->record_type);
  if(stb_elem_dup)
  {
    /*
     * stb_elem_dup->story_id   ...  keep id as initialized by gap_story_new_elem
     * stb_elem_dup->selected   ...  keep FALSE as initialized by gap_story_new_elem
     */
    stb_elem_dup->story_orig_id   = stb_elem->story_id;
    stb_elem_dup->playmode        = stb_elem->playmode;
    stb_elem_dup->track           = stb_elem->track;
    if(stb_elem->orig_filename)     stb_elem_dup->orig_filename     = g_strdup(stb_elem->orig_filename);
    if(stb_elem->orig_src_line)     stb_elem_dup->orig_src_line     = g_strdup(stb_elem->orig_src_line);
    if(stb_elem->basename)          stb_elem_dup->basename          = g_strdup(stb_elem->basename);
    if(stb_elem->ext)               stb_elem_dup->ext               = g_strdup(stb_elem->ext);
    if(stb_elem->filtermacro_file)  stb_elem_dup->filtermacro_file  = g_strdup(stb_elem->filtermacro_file);
    if(stb_elem->preferred_decoder) stb_elem_dup->preferred_decoder = g_strdup(stb_elem->preferred_decoder);
    stb_elem_dup->seltrack        = stb_elem->seltrack;
    stb_elem_dup->exact_seek      = stb_elem->exact_seek;
    stb_elem_dup->delace          = stb_elem->delace;
    stb_elem_dup->from_frame      = stb_elem->from_frame;
    stb_elem_dup->to_frame        = stb_elem->to_frame;
    stb_elem_dup->nloop           = stb_elem->nloop;
    stb_elem_dup->nframes         = stb_elem->nframes; 
    stb_elem_dup->step_density    = stb_elem->step_density; 
    stb_elem_dup->file_line_nr    = stb_elem->file_line_nr;  
    stb_elem_dup->comment = NULL;
    stb_elem_dup->next = NULL;

  }
  return (stb_elem_dup);
}  /* end gap_story_elem_duplicate */


/* -----------------------------
 * gap_story_elem_copy
 * -----------------------------
 * make a copy of the element content (including the first comment element)
 */
void     
gap_story_elem_copy(GapStoryElem *stb_elem_dst, GapStoryElem *stb_elem)
{

  if((stb_elem_dst) && (stb_elem))
  {
    if(stb_elem_dst->orig_filename)     g_free(stb_elem_dst->orig_filename);
    if(stb_elem_dst->orig_src_line)     g_free(stb_elem_dst->orig_src_line);
    if(stb_elem_dst->basename)          g_free(stb_elem_dst->basename);
    if(stb_elem_dst->ext)	        g_free(stb_elem_dst->ext);
    if(stb_elem_dst->filtermacro_file)  g_free(stb_elem_dst->filtermacro_file);
    if(stb_elem_dst->preferred_decoder) g_free(stb_elem_dst->preferred_decoder);

    stb_elem_dst->orig_filename     = NULL;
    stb_elem_dst->orig_src_line     = NULL;
    stb_elem_dst->basename          = NULL;
    stb_elem_dst->ext               = NULL;
    stb_elem_dst->filtermacro_file  = NULL;
    stb_elem_dst->preferred_decoder = NULL;

    stb_elem_dst->record_type = stb_elem->record_type;
    /*
     * stb_elem_dst->story_id   ...  keep id as initialized by gap_story_new_elem
     * stb_elem_dst->selected   ...  keep FALSE as initialized by gap_story_new_elem
     */
    stb_elem_dst->story_orig_id   = stb_elem->story_id;
    stb_elem_dst->playmode        = stb_elem->playmode;
    stb_elem_dst->track           = stb_elem->track;
    if(stb_elem->orig_filename)     stb_elem_dst->orig_filename     = g_strdup(stb_elem->orig_filename);
    if(stb_elem->orig_src_line)     stb_elem_dst->orig_src_line     = g_strdup(stb_elem->orig_src_line);
    if(stb_elem->basename)          stb_elem_dst->basename          = g_strdup(stb_elem->basename);
    if(stb_elem->ext)               stb_elem_dst->ext               = g_strdup(stb_elem->ext);
    if(stb_elem->filtermacro_file)  stb_elem_dst->filtermacro_file  = g_strdup(stb_elem->filtermacro_file);
    if(stb_elem->preferred_decoder) stb_elem_dst->preferred_decoder = g_strdup(stb_elem->preferred_decoder);

    stb_elem_dst->seltrack        = stb_elem->seltrack;
    stb_elem_dst->exact_seek      = stb_elem->exact_seek;
    stb_elem_dst->delace          = stb_elem->delace;
    stb_elem_dst->from_frame      = stb_elem->from_frame;
    stb_elem_dst->to_frame        = stb_elem->to_frame;
    stb_elem_dst->nloop           = stb_elem->nloop;
    stb_elem_dst->nframes         = stb_elem->nframes; 
    stb_elem_dst->step_density    = stb_elem->step_density; 
    stb_elem_dst->file_line_nr    = stb_elem->file_line_nr;  
    if(stb_elem_dst->comment)       gap_story_elem_free(&stb_elem_dst->comment);
    if(stb_elem->comment)           stb_elem_dst->comment = gap_story_elem_duplicate(stb_elem->comment);

    /* stb_elem_dst->next */ /* dont change the next pointer */

  }
}  /* end gap_story_elem_copy */


/* -----------------------------
 * p_story_board_duplicate
 * -----------------------------
 * the input stb_ptr is not copied if there are errors
 *   in this case NULL is returned.
 *
 * make a duplicate of the list (without the comment sublists)
 */
static GapStoryBoard *     
p_story_board_duplicate(GapStoryBoard *stb_ptr, gboolean copy_all_all)
{
  GapStoryBoard     *stb_dup;
  GapStoryElem      *stb_elem_dup;
  GapStoryElem      *stb_elem;

  if(stb_ptr == NULL)
  {
    return (NULL);
  }
  if(stb_ptr->errtext)
  {
    return (NULL);
  }

  stb_dup = gap_story_new_story_board(stb_ptr->storyboardfile);
  if(stb_dup == NULL)
  {
    return (NULL);
  }
  stb_dup->master_type = stb_ptr->master_type;
  stb_dup->master_width = stb_ptr->master_width;
  stb_dup->master_height = stb_ptr->master_height;
  stb_dup->master_framerate = stb_ptr->master_framerate;
  stb_dup->layout_cols = stb_ptr->layout_cols;
  stb_dup->layout_rows = stb_ptr->layout_rows;
  stb_dup->layout_thumbsize = stb_ptr->layout_thumbsize;
  stb_dup->unsaved_changes = TRUE;
  
  for(stb_elem = stb_ptr->stb_elem; stb_elem != NULL;  stb_elem = stb_elem->next)
  {
    if((stb_elem->selected)
    || (copy_all_all))
    {
      stb_elem_dup = gap_story_elem_duplicate(stb_elem);
      if(stb_elem_dup)
      {
	gap_story_list_append_elem(stb_dup, stb_elem_dup);
      }
    }
  }

  if (gap_debug)
  {
    printf("\np_story_board_duplicate: dump of the DUP List\n");
    gap_story_debug_print_list(stb_dup);
  }
 
  return(stb_dup);
  
}  /* end p_story_board_duplicate  */
 

/* -----------------------------
 * gap_story_duplicate
 * -----------------------------
 * the input stb_ptr is not copied if there are errors
 *   in this case NULL is returned.
 *
 * make a duplicate of the list (without the comment sublists)
 */
GapStoryBoard *     
gap_story_duplicate(GapStoryBoard *stb_ptr)
{
  return(p_story_board_duplicate(stb_ptr, TRUE));
}  /* end gap_story_duplicate  */

/* -----------------------------
 * gap_story_duplicate_sel_only
 * -----------------------------
 * the input stb_ptr is not copied if there are errors
 *   in this case NULL is returned.
 *
 * make a filtered duplicate of the list that contains only elements with the selected flag set
 * (without the comment sublists)
 */
GapStoryBoard *     
gap_story_duplicate_sel_only(GapStoryBoard *stb_ptr)
{
  return(p_story_board_duplicate(stb_ptr, FALSE));
}  /* end gap_story_duplicate_sel_only  */

/* ----------------------------------------------------
 * gap_story_remove_sel_elems
 * ----------------------------------------------------
 * remove and discard all selected stb_elem ents in the list.
 */
void
gap_story_remove_sel_elems(GapStoryBoard *stb)
{
  GapStoryElem *stb_elem_prev;
  GapStoryElem *stb_elem_next;
  GapStoryElem *stb_elem;

  if(stb)
  {
    stb_elem_prev = NULL;
    for(stb_elem = stb->stb_elem; stb_elem != NULL;  stb_elem = stb_elem_next)
    {
      stb_elem_next = (GapStoryElem *)stb_elem->next;
      if(stb_elem->selected)
      {
        stb->unsaved_changes = TRUE;
        if(stb_elem == stb->stb_elem)
	{
	  /* remove the 1.st elem of the list */
	  stb->stb_elem = stb_elem_next;
	}
	else
	{
	  stb_elem_prev->next = stb_elem->next;
	}
	stb_elem->next = NULL;  /* ensure that no other elements 
	                         * will be discarded by the following
				 * call to p_free_stb_elem_list
				 */
        p_free_stb_elem_list(stb_elem);
      }
      else
      {
        stb_elem_prev = stb_elem;
      }
    }
  }
}  /* end gap_story_remove_sel_elems */


/* -----------------------------
 * gap_story_count_active_elements
 * -----------------------------
 */
gint32
gap_story_count_active_elements(GapStoryBoard *stb_ptr, gint32 in_track)
{
  gint32             cnt_active_elements;
  GapStoryElem      *stb_elem;
  
  cnt_active_elements = 0;
  for(stb_elem = stb_ptr->stb_elem; stb_elem != NULL;  stb_elem = stb_elem->next)
  {
    if(stb_elem->track == in_track)
    {
      if((stb_elem->record_type == GAP_STBREC_VID_SILENCE)
      || (stb_elem->record_type == GAP_STBREC_VID_COLOR)
      || (stb_elem->record_type == GAP_STBREC_VID_IMAGE)
      || (stb_elem->record_type == GAP_STBREC_VID_ANIMIMAGE)
      || (stb_elem->record_type == GAP_STBREC_VID_FRAMES)
      || (stb_elem->record_type == GAP_STBREC_VID_MOVIE)
      )
      {
        cnt_active_elements++;
      }
    }
  }
  
  return(cnt_active_elements);
}  /* end gap_story_count_active_elements */


/* ------------------------------
 * gap_story_fetch_nth_active_elem
 * ------------------------------
 * find the nth active element in the storyboard list
 * specified by seq_nr.  (the first elem has seq_nr == 1)
 * return a pointer to the element (if found)
 * or return NULL if no element was found at seq_nr list-position found 
 */
GapStoryElem *
gap_story_fetch_nth_active_elem(GapStoryBoard *stb
                              , gint32 seq_nr
			      , gint32 in_track
			      )
{
  gint32             cnt_active_elements;
  GapStoryElem *stb_elem;
  
  cnt_active_elements = 0;
  for(stb_elem = stb->stb_elem; stb_elem != NULL;  stb_elem = stb_elem->next)
  {
    if(stb_elem->track == in_track)
    {
      if((stb_elem->record_type == GAP_STBREC_VID_SILENCE)
      || (stb_elem->record_type == GAP_STBREC_VID_COLOR)
      || (stb_elem->record_type == GAP_STBREC_VID_IMAGE)
      || (stb_elem->record_type == GAP_STBREC_VID_ANIMIMAGE)
      || (stb_elem->record_type == GAP_STBREC_VID_FRAMES)
      || (stb_elem->record_type == GAP_STBREC_VID_MOVIE)
      )
      {
        cnt_active_elements++;
	if(cnt_active_elements == seq_nr)
	{
	  return(stb_elem);
	}
      }
    }
  }
  
  return(NULL);
}  /* end gap_story_fetch_nth_active_elem */




/* ---------------------------------
 * gap_story_selection_all_set
 * ---------------------------------
 */
void
gap_story_selection_all_set(GapStoryBoard *stb, gboolean sel_state)
{
  GapStoryElem *stb_elem;
  
  if(stb  == NULL)   { return; }
  
  for(stb_elem = stb->stb_elem; stb_elem != NULL;  stb_elem = stb_elem->next)
  {
    stb_elem->selected = sel_state;
  }

}  /* end gap_story_selection_all_set */


/* ---------------------------------
 * gap_story_get_preferred_decoder
 * ---------------------------------
 * get the name of preferred_deceoder
 * in that order:
 * 1.) from current storyboard element (if available)
 * 2.) from general storyboard setting (if available)
 * 3.) return NULL if both cases do not provide a preferred_deceoder
 *
 * Do not free the returned pointer.
 */
const char *
gap_story_get_preferred_decoder(GapStoryBoard *stb, GapStoryElem *stb_elem)
{
  const char *preferred_decoder;

  /* use the file-global preferred_decoder per default */ 
  preferred_decoder = NULL;
  
  if(stb_elem)
  {
    if(stb_elem->preferred_decoder)
    {
      if(*stb_elem->preferred_decoder != '\0')
      {
        preferred_decoder = stb_elem->preferred_decoder;
      }
    }
  }
  
  if((stb) && (preferred_decoder == NULL))
  {
    if(stb->preferred_decoder)
    {
      if(*stb->preferred_decoder != '\0')
      {
        preferred_decoder = stb->preferred_decoder;
      }
    }
  }

  return(preferred_decoder);

}  /* end gap_story_get_preferred_decoder */
