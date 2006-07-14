/*  gap_story_file.c
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
 * version 2.3.0;   2006/06/14  hof: added storyboard support for layer_masks,
 *                                   overlapping frames (are converted to shadow tracks at processing)
 *                                   and image filpping
 * version 1.3.25b; 2004/01/24  hof: created
 */

#include "config.h"

#include <string.h>
#include <stdlib.h>


#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include <libgimp/gimp.h>

#include "gap_story_syntax.h"
#include "gap_story_main.h"
#include "gap_story_file.h"
#include "gap_lib.h"
#include "gap_vin.h"


#include "gap-intl.h"

extern int gap_debug;  /* 1 == print debug infos , 0 dont print debug infos */


#define MAX_CHARS_ERRORTEXT_LONG  200
#define MAX_CHARS_ERRORTEXT_GUI   50
#define MAX_TOLERATED_UNPRINTABLE_CHARACTERS 500

static gint32 global_stb_id = 0;
static gint32 global_stb_elem_id = 0;
static gint32 global_mask_name_number = 1000;

static const char *gtab_att_transition_key_words[GAP_STB_ATT_TYPES_ARRAY_MAX] =
       { GAP_STBKEY_VID_OPACITY
       , GAP_STBKEY_VID_MOVE_X
       , GAP_STBKEY_VID_MOVE_Y
       , GAP_STBKEY_VID_ZOOM_X
       , GAP_STBKEY_VID_ZOOM_Y
       };


/* ----------------------------------------------------
 * p_record_type_to_string
 * ----------------------------------------------------
 */
static const char *
p_record_type_to_string(GapStoryRecordType record_type)
{
  switch(record_type)
  {
    case GAP_STBREC_VID_SILENCE:     return("GAP_STBREC_VID_SILENCE"); break;
    case GAP_STBREC_VID_COLOR:       return("GAP_STBREC_VID_COLOR"); break;
    case GAP_STBREC_VID_IMAGE:       return("GAP_STBREC_VID_IMAGE"); break;
    case GAP_STBREC_VID_ANIMIMAGE:   return("GAP_STBREC_VID_ANIMIMAGE"); break;
    case GAP_STBREC_VID_FRAMES:      return("GAP_STBREC_VID_FRAMES"); break;
    case GAP_STBREC_VID_MOVIE:       return("GAP_STBREC_VID_MOVIE"); break;
    case GAP_STBREC_VID_COMMENT:     return("GAP_STBREC_VID_COMMENT"); break;
    case GAP_STBREC_VID_UNKNOWN:     return("GAP_STBREC_VID_UNKNOWN"); break;
    case GAP_STBREC_AUD_SILENCE:     return("GAP_STBREC_AUD_SILENCE"); break;
    case GAP_STBREC_AUD_SOUND:       return("GAP_STBREC_AUD_SOUND"); break;
    case GAP_STBREC_AUD_MOVIE:       return("GAP_STBREC_AUD_MOVIE"); break;
    case GAP_STBREC_ATT_TRANSITION:  return("GAP_STBREC_ATT_TRANSITION"); break;
  }
  return("** UNDEFINED RECORD TYPE **");
}  /* end p_record_type_to_string */


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
    printf("\ngap_story_debug_print_elem:  stb_elem: %d\n", (int)stb_elem);
 
    printf("gap_story_debug_print_elem: START\n");
    printf("record_type: %d  %s\n", (int)stb_elem->record_type
                                  , p_record_type_to_string(stb_elem->record_type)
                                  );
    printf("story_id: %d\n", (int)stb_elem->story_id);
    printf("story_orig_id: %d\n", (int)stb_elem->story_orig_id);
    printf("selected: %d\n", (int)stb_elem->selected);
    printf("playmode: %d\n", (int)stb_elem->playmode);
    printf("track: %d", (int)stb_elem->track);
    
    if(stb_elem->track == GAP_STB_MASK_TRACK_NUMBER)
    {
      printf(" (MASK-DEFINITION track)");
    }
    printf("\n");
    
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
    printf("aud_min_play_sec: %f\n", (float)stb_elem->aud_min_play_sec);
    printf("aud_max_play_sec: %f\n", (float)stb_elem->aud_max_play_sec);
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

    if(stb_elem->record_type == GAP_STBREC_ATT_TRANSITION)
    {
      gint ii;
      for(ii=0; ii < GAP_STB_ATT_TYPES_ARRAY_MAX; ii++)
      {
        if(stb_elem->att_arr_enable[ii])
        {
          printf("  [%d] %s  from: %f  to: %f   dur: %d\n"
            , (int)ii
            , gtab_att_transition_key_words[ii]
            , (float)stb_elem->att_arr_value_from[ii]
            , (float)stb_elem->att_arr_value_to[ii]
            , (int)stb_elem->att_arr_value_dur[ii]
            );
        }
      }
      printf(" overlap: %d\n", (int)stb_elem->att_overlap);
    }

    printf(" flip_request: %d\n", (int)stb_elem->flip_request);
    if(stb_elem->mask_name)
    {
      printf("mask_name :%s:\n", stb_elem->mask_name);
    }
    else
    {
      printf("mask_name: (NULL)\n");
    }
    printf("mask_anchor:   %d\n", (int)stb_elem->mask_anchor);
    printf("mask_stepsize: %f\n", (float)stb_elem->mask_stepsize);
    printf("mask_disable:  %d\n", (int)stb_elem->mask_disable);

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

  printf("master_type         : %d\n", (int)stb->master_type );
  printf("master_width        : %d\n", (int)stb->master_width );
  printf("master_height       : %d\n", (int)stb->master_height );
  printf("master_framerate    : %f\n", (float)stb->master_framerate );
  printf("master_aspect_ratio : %f (%d : %d)\n"
            , (float)stb->master_aspect_ratio
            , (int)stb->master_aspect_width
            , (int)stb->master_aspect_height
            );
  printf("layout_cols         : %d\n", (int)stb->layout_cols );
  printf("layout_rows         : %d\n", (int)stb->layout_rows );
  printf("layout_thumbsize    : %d\n", (int)stb->layout_thumbsize );

  printf("stb_parttype        : %d\n", (int)stb->stb_parttype );
  printf("stb_unique_id       : %d\n", (int)stb->stb_unique_id );

  ii = 0;
  for(stb_elem = stb->stb_elem; stb_elem != NULL;  stb_elem = stb_elem->next)
  {
    ii++;
    if(stb_elem->track == GAP_STB_MASK_TRACK_NUMBER)
    {
      printf("\nMask-Element # (%03d)\n", (int)ii);
    }
    else
    {
      printf("\nElement # (%03d)\n", (int)ii);
    }
    
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
 * p_dup_printable_stringprefix
 * ------------------------------
 */
char *
p_dup_printable_stringprefix(GapStoryBoard *stb, const char *str, gint32 length)
{
  char *l_dup_str;
  gint ii;
  
  l_dup_str = g_malloc0(length + 1);
  
  for(ii=0; ii < length; ii++)
  {
    if(g_ascii_isprint(str[ii]))
    {
      l_dup_str[ii] = str[ii];
    }
    else
    {
      l_dup_str[ii] = '?';
      stb->count_unprintable_chars++;
    }
  }
  l_dup_str[ii] = '\0';
  return(l_dup_str);
}  /* end p_dup_printable_stringprefix */


/* ------------------------------
 * p_set_stb_error
 * ------------------------------
 * print the passed errtext to stdout
 * and store linenumber, errtext and sourcecode line
 * of the 1.st error that occurred
 */
static void
p_set_stb_error(GapStoryBoard *stb, char *errtext)
{
  char *currline_string;
  

  if(stb->count_unprintable_chars < MAX_TOLERATED_UNPRINTABLE_CHARACTERS)
  {
    currline_string = p_dup_printable_stringprefix(stb, stb->currline, MAX_CHARS_ERRORTEXT_LONG);
    printf("** error: %s\n   [at line:%d] %s\n"
        , errtext
        , (int)stb->curr_nr
        , currline_string
        );
    g_free(currline_string);
  }
  
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
       stb->errline     = p_dup_printable_stringprefix(stb
                            , stb->currline
                            , MAX_CHARS_ERRORTEXT_GUI);
     }
  }
}  /* end p_set_stb_error */


/* ------------------------------
 * p_set_stb_warning
 * ------------------------------
 * print the passed errtext to stdout
 * and store linenumber, errtext and sourcecode line
 * of the 1.st warning that occurred
 */
static void
p_set_stb_warning(GapStoryBoard *stb, char *warntext)
{
  char *currline_string;
  
  if(stb->count_unprintable_chars < MAX_TOLERATED_UNPRINTABLE_CHARACTERS)
  {
    currline_string = p_dup_printable_stringprefix(stb, stb->currline, MAX_CHARS_ERRORTEXT_LONG);

    printf("## warning: %s\n   [at line:%d] %s\n"
        , warntext
        , (int)stb->curr_nr
        , currline_string
        );
    g_free(currline_string);
  }
  
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
       stb->warnline     = p_dup_printable_stringprefix(stb
                            , stb->currline
                            , MAX_CHARS_ERRORTEXT_GUI);
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

    stb->master_aspect_ratio = 0.0;  /* 0.0 for none */
    stb->master_aspect_width = 0;
    stb->master_aspect_height = 0;
    
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
    stb->count_unprintable_chars = 0;

    stb->preferred_decoder  = NULL;

    stb->stb_parttype  = 0;
    stb->stb_unique_id  = global_stb_id++;

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
    stb_elem->flip_request = GAP_STB_FLIP_NONE;
    stb_elem->mask_name = NULL;
    stb_elem->mask_anchor = GAP_MSK_ANCHOR_CLIP;
    stb_elem->mask_disable = FALSE;
    stb_elem->mask_stepsize = 1.0;

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
    stb_elem->att_overlap = 0;

    /* init transition attributes */
    {
      gint ii;
      for(ii=0; ii < GAP_STB_ATT_TYPES_ARRAY_MAX; ii++)
      {
        stb_elem->att_arr_enable[ii] = FALSE;
        stb_elem->att_arr_value_from[ii] = gap_story_get_default_attribute(ii);
        stb_elem->att_arr_value_to[ii] = gap_story_get_default_attribute(ii);
        stb_elem->att_arr_value_dur[ii] = 1;        /* number of frames to change from -> to value */
      }
    }
    
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
    stb_elem->aud_min_play_sec  = -1.0;
    stb_elem->aud_max_play_sec  = -1.0;
    stb_elem->aud_framerate     = GAP_STORY_DEFAULT_FRAMERATE;

    /* init list pointers */
    stb_elem->comment = NULL;
    stb_elem->next = NULL;
  }
  
  if(gap_debug) printf("gap_story_new_elem: RETURN stb_elem ptr: %d\n", (int)stb_elem );
  return(stb_elem);
}  /* end gap_story_new_elem */


/* ------------------------------
 * gap_story_new_mask_elem
 * ------------------------------
 */
GapStoryElem *
gap_story_new_mask_elem(GapStoryRecordType record_type)
{
  GapStoryElem *stb_elem;
  
  stb_elem = gap_story_new_elem(record_type);
  if(stb_elem)
  {
    /* forcce defaults for mask definition elements */
    stb_elem->track = GAP_STB_MASK_TRACK_NUMBER;
    stb_elem->playmode = GAP_STB_PM_NORMAL;
  }
  return(stb_elem);
}  /* end gap_story_new_mask_elem */

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
long
gap_story_upd_elem_from_filename(GapStoryElem *stb_elem,  const char *filename)
{
  long current_frame;
  
  current_frame = -1;
  
  if(stb_elem == NULL) { return(current_frame); }
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
        current_frame = l_number;
      }
      else
      {
        stb_elem->record_type = GAP_STBREC_VID_IMAGE;
      }
    }
  }
  return (current_frame);
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
  if(stb_elem->mask_name)         { g_free(stb_elem->mask_name);}
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
}       /* end p_free_stb_elem_list */


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
            *                                        +------+          +------+
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
 * p_get_last_non_comment_elem
 * ----------------------------------------------------
 */
static GapStoryElem *
p_get_last_non_comment_elem(GapStoryBoard *stb, gint32 in_track)
{
  GapStoryElem *stb_elem_non_comment;
  GapStoryElem *stb_elem;
  
  stb_elem_non_comment = NULL;
  for(stb_elem = stb->stb_elem; stb_elem != NULL; stb_elem = (GapStoryElem *)stb_elem->next)
  {
    if(stb_elem->track != in_track)
    {
      continue;
    }
    if(stb_elem->record_type != GAP_STBREC_VID_COMMENT)
    {
      stb_elem_non_comment = stb_elem;
    }
  }
  
  return (stb_elem_non_comment);
  
}  /* end p_get_last_non_comment_elem */


/* ----------------------------------------------------
 * gap_story_set_fit_size_attributes
 * ----------------------------------------------------
 */
static void
gap_story_set_fit_size_attributes(GapStoryBoard *stb
                        , GapStoryElem *target_stb_elem
                        , gint32 story_id
                        , gint32 in_track)
{
  GapStoryElem      *stb_elem;
  
  for(stb_elem = stb->stb_elem; stb_elem != NULL;  stb_elem = stb_elem->next)
  {
    if(stb_elem->track != in_track)
    {
      continue;
    }
    if(stb_elem->record_type == GAP_STBREC_ATT_TRANSITION)
    {
      target_stb_elem->att_keep_proportions = stb_elem->att_keep_proportions;
      target_stb_elem->att_fit_width = stb_elem->att_fit_width;
      target_stb_elem->att_fit_height = stb_elem->att_fit_height;
    }
    
    if(stb_elem->story_id == story_id)
    {
      break;
    }
  }
  
}  /* end gap_story_set_fit_size_attributes */


/* ----------------------------------------------------
 * p_get_att_transition_elem
 * ----------------------------------------------------
 */
static GapStoryElem *
p_get_att_transition_elem(GapStoryBoard *stb, gboolean *elem_already_in_the_list, gint32 in_track)
{
  GapStoryElem *stb_elem;
  
  stb_elem = p_get_last_non_comment_elem(stb, in_track);
  if(stb_elem)
  {
    if(stb_elem->record_type == GAP_STBREC_ATT_TRANSITION)
    {
      *elem_already_in_the_list = TRUE;
      return (stb_elem);
    }
  }
     
  *elem_already_in_the_list = FALSE;
  stb_elem =  gap_story_new_elem(GAP_STBREC_ATT_TRANSITION);
  stb_elem->nframes = 0;
  stb_elem->track = in_track;
  gap_story_set_fit_size_attributes(stb, stb_elem, -1, in_track);
  return (stb_elem);
}  /* end p_get_att_transition_elem */




/* ----------------------------------------------------
 * gap_story_generate_unique_maskname
 * ----------------------------------------------------
 */
char *
gap_story_generate_unique_maskname(GapStoryBoard *stb_ptr)
{
  char *mask_name;
  
  while(1==1)
  {
    GapStoryElem      *stb_elem_ref;
    
    mask_name = g_strdup_printf("m%d", (int)global_mask_name_number);
    global_mask_name_number++;

    if(gap_debug)
    {
      printf("gap_story_generate_unique_maskname: %s\n", mask_name);
    }
    
    stb_elem_ref = gap_story_find_mask_definition_by_name(stb_ptr, mask_name);
    if(stb_elem_ref == NULL)
    {
      /* no element found, name is unique */
      return(mask_name);
    }
    
    /* the generated mask_name is already in use, try another one */
    g_free(mask_name);
    
  }
}  /* end gap_story_generate_unique_maskname */


/* ----------------------------------------------------
 * gap_story_update_mask_name_references
 * ----------------------------------------------------
 */
gboolean
gap_story_update_mask_name_references(GapStoryBoard *stb_ptr
  , const char *mask_name_new
  , const char *mask_name_old
  )
{
  GapStoryElem      *stb_elem;
  GapStoryElem      *stb_elem_ref;

  /* check if there are already references to mask_name_new */
  stb_elem_ref = gap_story_find_mask_reference_by_name(stb_ptr, mask_name_new);
  if(stb_elem_ref)
  {
    /* update failed, because mask_name_new has name conflict
     * with other mask references
     */
    if(gap_debug)
    {
      printf("gap_story_update_mask_name_references: name conflict %s\n", mask_name_new);
      //gap_story_debug_print_elem(stb_elem_ref);
    }
    return (FALSE);
  }

  /* replace referneces */
  for(stb_elem = stb_ptr->stb_elem; stb_elem != NULL;  stb_elem = stb_elem->next)
  {
    if((stb_elem->track != GAP_STB_MASK_TRACK_NUMBER)
    && (stb_elem->mask_name != NULL))
    {
      if(strcmp(stb_elem->mask_name, mask_name_old) == 0)
      {
        g_free(stb_elem->mask_name);
        stb_elem->mask_name = g_strdup(mask_name_new);
        if(gap_debug)
        {
          printf("gap_story_update_mask_name_references: update old_name: %s\n", mask_name_old);
          gap_story_debug_print_elem(stb_elem);
        }
        
        
      }
    }
  }
  return(TRUE);
}  /* end gap_story_update_mask_name_references */


/* ----------------------------------------------------
 * p_equal_null_strings
 * ----------------------------------------------------
 * return TRUE if both strings are equal or both are NULL.
 */
static gboolean
p_null_strcmp(const char *str1, const char *str2)
{
  if(str1 != NULL)
  {
    if(str2 != NULL)
    {
      if(strcmp(str1 , str2) == 0)
      {
        return (TRUE);
      }
    }
  }
  else
  {
    if(str2 == NULL)
    {
        return (TRUE);
    }
  }
  return (FALSE);
}  /* end p_null_strcmp */


/* ----------------------------------------------------
 * p_story_is_maskdef_equal
 * ----------------------------------------------------
 * check if mask definition relevant attributes are equal
 */
static gboolean
p_story_is_maskdef_equal(GapStoryElem *stb_elem, GapStoryElem *stb_maskdef_elem)
{
  if(stb_maskdef_elem->record_type  != stb_elem->record_type)
  {
        return (FALSE);
  }

  if(!p_null_strcmp(stb_maskdef_elem->orig_filename , stb_elem->orig_filename))
  {
        return (FALSE);
  }
  if(!p_null_strcmp(stb_maskdef_elem->basename , stb_elem->basename))
  {
        return (FALSE);
  }
  if(!p_null_strcmp(stb_maskdef_elem->ext , stb_elem->ext))
  {
        return (FALSE);
  }
  if(!p_null_strcmp(stb_maskdef_elem->preferred_decoder , stb_elem->preferred_decoder))
  {
        return (FALSE);
  }

  if(stb_maskdef_elem->from_frame  != stb_elem->from_frame)
  {
        return (FALSE);
  }
  if(stb_maskdef_elem->to_frame  != stb_elem->to_frame)
  {
        return (FALSE);
  }
  if(stb_maskdef_elem->nframes  != stb_elem->nframes)
  {
        return (FALSE);
  }
  if(stb_maskdef_elem->seltrack  != stb_elem->seltrack)
  {
        return (FALSE);
  }
  if(stb_maskdef_elem->exact_seek  != stb_elem->exact_seek)
  {
        return (FALSE);
  }
  if(stb_maskdef_elem->delace  != stb_elem->delace)
  {
        return (FALSE);
  }
  if(stb_maskdef_elem->flip_request  != stb_elem->flip_request)
  {
        return (FALSE);
  }

  return(TRUE);

}  /* end p_story_is_maskdef_equal */


/* ----------------------------------------------------
 * gap_story_find_maskdef_equal_to_ref_elem
 * ----------------------------------------------------
 * find and return a mask definition element
 * with maskdef relevant attributes that are equal with the reference element.
 * Note: if the refernce element is a mask definition element of the specified
 * storyboard it will be compared against itself and found.
 */
GapStoryElem *
gap_story_find_maskdef_equal_to_ref_elem(GapStoryBoard *stb_ptr, GapStoryElem *stb_ref_elem)
{
  GapStoryElem *stb_elem;
  
  for(stb_elem = stb_ptr->stb_elem; stb_elem != NULL; stb_elem = stb_elem->next)
  {
  
  
  
    if(stb_elem->track == GAP_STB_MASK_TRACK_NUMBER)
    {
      if(gap_debug)
      {
          printf("gap_story_find_maskdef_equal_to_ref_elem  ??? checking element:\n");
          gap_story_debug_print_elem(stb_elem);
      }


      if (p_story_is_maskdef_equal(stb_elem, stb_ref_elem))
      {
        return (stb_elem);
      }
    }
  }

  return (NULL);
}  /* end gap_story_find_maskdef_equal_to_ref_elem */


/* ----------------------------------------------------
 * p_process_maskdefs_and_split_off_vtrack_elems
 * ----------------------------------------------------
 * returns a newly created storyboard with clip elements ready to insert
 * at desired position of the destination storyboard.
 * the newly created storyboard is a copy of the source storyboard
 * where the elements of hidden mask definition tracks
 * are NOT included. (those elements are already processed in this procedure
 * and added to the destination storyboard if necessary.
 * -- for implicite copy of mask definitions --
 * this includes the required updates in case of mask_name uniqueness conflicts)
 *
 * the elements in the returned storyboard all have set the
 * paste destination video track number that was specified via parameter
 * dst_vtrack, the mask_name references are updated in cases where
 * new unique mask_name was generated.
 */
static GapStoryBoard *
p_process_maskdefs_and_split_off_vtrack_elems(GapStoryBoard *stb_dst
                         , GapStoryBoard *stb_src
                         , gint32   dst_vtrack
                         )
{
  GapStoryBoard *stb_vtrack_src;
  GapStoryElem  *stb_elem_dup;
  GapStoryElem  *stb_elem;

  stb_vtrack_src = gap_story_new_story_board(stb_src->storyboardfile);
  
  if(gap_debug)
  {
    printf("***** p_process_maskdefs_and_split_off_vtrack_elems dst_vtrack:%d\n"
        ,(int)dst_vtrack
        );
    gap_story_debug_print_list(stb_src);
  }
  
  if(dst_vtrack != GAP_STB_MASK_TRACK_NUMBER)
  {
    /* we are pasting into a normal video track
     * check if there are maskdefinitions in the src storyboard that have to
     * be pasted implicite (into mask definition track at end of destination storyboard)
     */
    for(stb_elem = stb_src->stb_elem; stb_elem != NULL; stb_elem = stb_elem->next)
    {
      if(stb_elem->track == GAP_STB_HIDDEN_MASK_TRACK_NUMBER)
      {
        gboolean maskdef_copy_required;
        gchar *new_mask_name;
        GapStoryElem *stb_maskdef_elem;
      
        new_mask_name = NULL;
        maskdef_copy_required = TRUE;


        stb_maskdef_elem = gap_story_find_mask_definition_by_name(stb_dst, stb_elem->mask_name);
        if(stb_maskdef_elem)
        {
          if(p_story_is_maskdef_equal(stb_elem, stb_maskdef_elem))
          {
            /* the destination storyboard already includes this
             * maskdefinition, therefore no implicite copy
             * is necessary
             */
            maskdef_copy_required = FALSE;
          }
          else
          {
            new_mask_name = gap_story_generate_unique_maskname(stb_dst);
            gap_story_update_mask_name_references(stb_src, new_mask_name, stb_elem->mask_name);
          }
        }

        if(maskdef_copy_required)
        {
          stb_elem_dup = gap_story_elem_duplicate(stb_elem);
          stb_elem_dup->track = GAP_STB_MASK_TRACK_NUMBER;
          if(new_mask_name)
          {
            if(stb_elem_dup->mask_name)
            {
              g_free(stb_elem_dup->mask_name);
            }
            stb_elem_dup->mask_name = g_strdup(new_mask_name);
          }
          gap_story_list_append_elem(stb_dst, stb_elem_dup);
        }
        if(new_mask_name)
        {
          g_free(new_mask_name);
        }
      }
    }  
  }

  /* loop 2 copy vtrack elements (also handles the case
   * of explicite pasting elemts into the mask definition track)
   */
  for(stb_elem = stb_src->stb_elem; stb_elem != NULL; stb_elem = stb_elem->next)
  {
    if((dst_vtrack == GAP_STB_MASK_TRACK_NUMBER)
    && (stb_elem->record_type == GAP_STBREC_ATT_TRANSITION))
    {
      /* never paste transitions into mask definition tracks */
      continue;
    }
    if(stb_elem->track != GAP_STB_HIDDEN_MASK_TRACK_NUMBER)
    {
      /* remove mask references if destination is the mask definition track
       * and source track was no mask definition
       */
      stb_elem_dup = gap_story_elem_duplicate(stb_elem);
      if(dst_vtrack == GAP_STB_MASK_TRACK_NUMBER)
      {
        if(stb_elem_dup->mask_name)
        {
          GapStoryElem      *stb_elem_ref;
         
          stb_elem_ref = gap_story_find_mask_definition_by_name(stb_dst, stb_elem_dup->mask_name); 
          if(stb_elem_ref)
          {
            /* the same mask name is already in use in the destination
             * storyboard mask definition track
             * clear the mask_name to force creation of a new unique mask_name
             */
            g_free(stb_elem_dup->mask_name);
            stb_elem_dup->mask_name = NULL;
          }
        }
        
        /* mask definitions require a unique name
         * therefore generate one if there is none
         */
        if(stb_elem_dup->mask_name == NULL)
        {
          stb_elem_dup->mask_name = gap_story_generate_unique_maskname(stb_dst);
        }
      }
      else
      {
        if(stb_elem_dup->track == GAP_STB_MASK_TRACK_NUMBER)
        {
          /* pasting a mask definition into a normal video track
           * must remove mask_name
           */
          if(stb_elem_dup->mask_name)
          {
            g_free(stb_elem_dup->mask_name);
            stb_elem_dup->mask_name = NULL;
          }
        }
      }
      stb_elem_dup->track = dst_vtrack;
      gap_story_list_append_elem(stb_vtrack_src, stb_elem_dup);
    }
  }  

  return(stb_vtrack_src);
}  /* end p_process_maskdefs_and_split_off_vtrack_elems */


/* ----------------------------------------------------
 * p_list_insert_at_story_id
 * ----------------------------------------------------
 */
static void
p_list_insert_at_story_id(GapStoryBoard *stb_dst
                         , GapStoryBoard *stb_vtrack_src
                         , gint32 story_id
                         , gboolean insert_after
                         )
{
  GapStoryElem *stb_elem_prev;
  GapStoryElem *stb_elem;
  GapStoryElem *stb_elem_last_src;
  
  
  stb_elem_last_src = stb_vtrack_src->stb_elem;
  if(stb_elem_last_src == NULL)
  {
    return;  /* src list is already empty, nothing left to do */
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
    /* destination list is empty, just move src elems there and done */ 
    stb_dst->stb_elem = stb_vtrack_src->stb_elem;
    stb_vtrack_src->stb_elem = NULL;
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
        stb_elem->next = stb_vtrack_src->stb_elem;
      }
      else
      {
        stb_elem_last_src->next = stb_elem;
        
        if(stb_elem_prev == NULL)
        {
          /* insert before the 1.st element */
          stb_dst->stb_elem = stb_vtrack_src->stb_elem;
        }
        else
        {
          stb_elem_prev->next = stb_vtrack_src->stb_elem;
        }
      }
      stb_vtrack_src->stb_elem = NULL;
      return;
      
    }
    stb_elem_prev = stb_elem;
  }
  
  if(stb_elem_prev)  /* should never be NULL at this point */
  {
    /* add src list at end of dst list */
    stb_elem_prev->next = stb_vtrack_src->stb_elem;
    stb_vtrack_src->stb_elem = NULL;
  }

}  /* end p_list_insert_at_story_id */



/* ----------------------------------------------------
 * gap_story_lists_merge
 * ----------------------------------------------------
 * insert all elements of stb_src into stb_dst
 * at position after (or before) the elment with matching story_id.
 *
 * use story_id -1 (or any other invalid story_id) if you want to
 * append src at end of dst list.
 *
 * this procedure is typical used for pasting a selection list (src)
 * into another storyboard list at the specified video track number
 * (parameter dst_vtrack).
 *
 * NOTE: mask definition refrences in the source storyboard are
 * modified to fit the needs for the destionation storyboard
 * at processing. Typically the source storyboard stb_src
 * should be destroyed after calling this procedure.
 */
void
gap_story_lists_merge(GapStoryBoard *stb_dst
                         , GapStoryBoard *stb_src
                         , gint32 story_id
                         , gboolean insert_after
                         , gint32   dst_vtrack
                         )
{
  if((stb_dst) && (stb_src))
  {
    GapStoryBoard *stb_vtrack_src;

    if(stb_src->stb_elem == NULL)
    {
      return;  /* src list is empty, nothing to do */
    }
    
    stb_dst->unsaved_changes = TRUE;
    stb_src->unsaved_changes = TRUE;

    stb_vtrack_src = p_process_maskdefs_and_split_off_vtrack_elems(stb_dst
                                   , stb_src
                                   , dst_vtrack
                                   );

    p_list_insert_at_story_id(stb_dst
                         , stb_vtrack_src
                         , story_id
                         , insert_after
                         );
    gap_story_free_storyboard(&stb_vtrack_src);
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


/* ------------------------------------
 * gap_story_elem_find_by_story_orig_id
 * ------------------------------------
 */
GapStoryElem *
gap_story_elem_find_by_story_orig_id(GapStoryBoard *stb, gint32 story_orig_id)
{
  GapStoryElem *stb_elem;

  if(stb)
  {
    for(stb_elem = stb->stb_elem; stb_elem != NULL;  stb_elem = stb_elem->next)
    {
      if(stb_elem->story_orig_id == story_orig_id)
      {
        return(stb_elem);
      }
    }
  }
  return (NULL);
  
}  /* end gap_story_elem_find_by_story_orig_id */

/* ----------------------------------------------------
 * p_flip_dir_separators
 * ----------------------------------------------------
 * Replace all / and \ characters by G_DIR_SEPARATOR
 */
static void
p_flip_dir_separators(char *ptr)
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
}  /* end p_flip_dir_separators */


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
}       /* end p_fetch_string */
  

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



/* -----------------------------
 * p_assign_parsed_flip_value
 * -----------------------------
 * parse typical (optional) video parameters via predfined pointers.
 * if the relevant pointer points to a not null string
 * then scann the parameter and assign it the specified stb_elem
 */
static void
p_assign_parsed_flip_value(GapStoryElem *stb_elem
  , char *flip_ptr
  )
{
  if(flip_ptr)
  {
    if(*flip_ptr)
    {
      switch(*flip_ptr)
      {
        case 'n':
        case 'N':
          stb_elem->flip_request = GAP_STB_FLIP_NONE;
          break;
        case 'h':
        case 'H':
          stb_elem->flip_request = GAP_STB_FLIP_HOR;
          break;
        case 'v':
        case 'V':
          stb_elem->flip_request = GAP_STB_FLIP_VER;
          break;
        case 'b':
        case 'B':
          stb_elem->flip_request = GAP_STB_FLIP_BOTH;
          break;
      }
    }
  }

}  /* end p_assign_parsed_flip_value */


/* -----------------------------
 * p_assign_parsed_video_values
 * -----------------------------
 * parse typical (optional) video parameters via predfined pointers.
 * if the relevant pointer points to a not null string
 * then scann the parameter and assign it the specified stb_elem
 */
static void
p_assign_parsed_video_values(GapStoryElem *stb_elem
  , GapStoryBoard *stb
  , char *nloops_ptr
  , char *stepsize_ptr
  , char *pingpong_ptr
  , char *macro_ptr
  , char *flip_ptr
  , char *mask_name_ptr
  , char *mask_anchor_ptr
  , char *mask_stepsize_ptr
  , char *mask_disable_ptr
  )
{
  if(nloops_ptr)
  {
    if(*nloops_ptr)
    {
      stb_elem->nloop = p_scan_gint32(nloops_ptr,  1, 999999, stb);
    }
  }
  
  if(stepsize_ptr)
  {
    if(*stepsize_ptr)
    {
      stb_elem->step_density = p_scan_gdouble(stepsize_ptr,  0.125, 100.0, stb);
    }
  }

  if(pingpong_ptr)
  {
    if(*pingpong_ptr)
    {
      if ((strncmp(pingpong_ptr, "PINGPONG", strlen("PINGPONG")) == 0)
      ||  (strncmp(pingpong_ptr, "pingpong", strlen("pingpong")) == 0))
      {
        stb_elem->playmode = GAP_STB_PM_PINGPONG;
      }
    }
  }

  if(macro_ptr)
  {
    if(*macro_ptr)
    {
      stb_elem->filtermacro_file = g_strdup(macro_ptr);
      p_flip_dir_separators(stb_elem->filtermacro_file);
    }
  }

  p_assign_parsed_flip_value(stb_elem, flip_ptr);

  if(mask_name_ptr)
  {
    if(*mask_name_ptr)
    {
      stb_elem->mask_name = g_strdup(mask_name_ptr);
    }
  }

  if(mask_anchor_ptr)
  {
    if(*mask_anchor_ptr)
    {
      switch(*mask_anchor_ptr)
      {
        case 'c':
        case 'C':
          stb_elem->mask_anchor = GAP_MSK_ANCHOR_CLIP;
          break;
        case 'm':
        case 'M':
          stb_elem->mask_anchor = GAP_MSK_ANCHOR_MASTER;
          break;
      }
    }
  }

  stb_elem->mask_stepsize = 1.0;
  if(mask_stepsize_ptr)
  {
    if(*mask_stepsize_ptr)
    {
      stb_elem->mask_stepsize = p_scan_gdouble(mask_stepsize_ptr,  0.125, 100.0, stb);
    }
  }

  if(mask_disable_ptr)
  {
    if(*mask_disable_ptr)
    {
      switch(*mask_disable_ptr)
      {
        case 'y':
        case 'Y':
          stb_elem->mask_disable = TRUE;
          break;
        case 'n':
        case 'N':
          stb_elem->mask_disable = FALSE;
          break;
      }
    }
  }

}  /* end p_assign_parsed_video_values */

/* ------------------------------
 * p_story_parse_line
 * ------------------------------
 * parse one storyboardfile line
 * if the line is syntactical OK
 *    add The Line as new stb_elem (at end of the stb_elem list)
 *    if it is a layer mask definition add it as stb_elem with track GAP_STB_MASK_TRACK_NUMBER
 * if the line has ERRORS
 *    the errtxt memer in the GapStoryBoard struct is set
 *    to the errormessage of the 1st error that occurred.
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
#define GAP_MAX_STB_PARAMS_PER_LINE 18
  char *l_scan_ptr;
  char *l_record_key;
  char *l_parname;
  char *l_wordval[GAP_MAX_STB_PARAMS_PER_LINE];
  gint ii;
  GapStoryElem *stb_elem;
  
  if(gap_debug)
  {
    printf("p_story_parse_line: %d %s\n", (int)longlinenr, longline);
  }

  l_scan_ptr = longline;
  stb->currline = longline;
  stb->curr_nr = longlinenr;


  /* clear array of values */
  for(ii=0; ii < GAP_MAX_STB_PARAMS_PER_LINE; ii++)
  {
    l_wordval[ii] = g_strdup("\0");
  }
  
  /* get the record key (1.st space separated word) */
  g_free(l_wordval[0]);
  l_wordval[0]    = p_fetch_string(&l_scan_ptr, &l_parname);
  l_record_key = l_wordval[0];
  if(l_parname)
  {
    g_free(l_parname);
  }



  /* handle comments */
  if (*l_record_key == '#')
  {
    stb_elem = gap_story_new_elem(GAP_STBREC_VID_COMMENT);
    if(stb_elem)
    {
        stb_elem->orig_src_line = g_strdup(multi_lines);
        stb_elem->file_line_nr = longlinenr;
        gap_story_list_append_elem(stb, stb_elem);
    }
    goto cleanup;
  }

  /* ignore empty lines */  
  if ((*l_record_key == '\n')
  ||  (*l_record_key == '\0'))
  {
    goto cleanup;
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

    if((gap_debug) && (1==0))
    {
      printf("\n%s   ii:%d\n", l_record_key, (int)ii);
    }
    l_parname = NULL;
    l_value = p_fetch_string(&l_scan_ptr, &l_parname);

    if((gap_debug) && (1==0))
    {
      printf("%s   ii:%d (after SCANN)\n", l_record_key, (int)ii);
    }
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

          if((gap_debug) && (1==0))
          {
             printf("%s   parname:%s: key_idx:%d\n", l_record_key, l_parname, (int)l_key_idx);
          }

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
            g_free(l_wordval[ii]);
            l_wordval[ii] = g_strdup(l_value);
          }
        }
      }
      g_free(l_value);
    }
    /* if(gap_debug) printf("%s   ii:%d (END)\n", l_record_key, (int)ii); */
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
  /* Master informations: GAP_STBKEY_VID_MASTER_FRAME_ASPECT */
  if (strcmp(l_record_key, GAP_STBKEY_VID_MASTER_FRAME_ASPECT) == 0)
  {
      if(*l_wordval[1]) { stb->master_aspect_width     = p_scan_gint32(l_wordval[1], 1, 999999, stb); }
      if(*l_wordval[2]) { stb->master_aspect_height    = p_scan_gint32(l_wordval[2], 1, 999999, stb); }

      if(stb->master_aspect_height > 0)
      {
        stb->master_aspect_ratio = (gdouble)stb->master_aspect_width
                                 / (gdouble)stb->master_aspect_height;
      }
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

  /* =========== Vid Attributes ===== start =========== */
  if ((strcmp(l_record_key, GAP_STBKEY_VID_OPACITY)  == 0)
  ||  (strcmp(l_record_key, GAP_STBKEY_VID_ZOOM_X)   == 0)
  ||  (strcmp(l_record_key, GAP_STBKEY_VID_ZOOM_Y)   == 0)
  ||  (strcmp(l_record_key, GAP_STBKEY_VID_MOVE_X)   == 0)
  ||  (strcmp(l_record_key, GAP_STBKEY_VID_MOVE_Y)   == 0)
  ||  (strcmp(l_record_key, GAP_STBKEY_VID_FIT_SIZE) == 0)
  ||  (strcmp(l_record_key, GAP_STBKEY_VID_OVERLAP)  == 0))
  {
    char *l_track_ptr      = l_wordval[1];
    char *l_from_ptr       = l_wordval[2];
    char *l_to_ptr         = l_wordval[3];
    char *l_dur_ptr        = l_wordval[4];

    gint32 l_track_nr;
    
    l_track_nr = 1;
    if(*l_track_ptr)    
    { 
      l_track_nr = p_scan_gint32(l_track_ptr,  1, GAP_STB_MAX_VID_TRACKS,    stb);
    }


    /* ATTRIBUTE: GAP_STBKEY_VID_OVERLAP */
    if (strcmp(l_record_key, GAP_STBKEY_VID_OVERLAP) == 0)
    {
      char *l_overlap_nframes_ptr       = l_wordval[2];

      gboolean elem_already_in_the_list = FALSE;
      
      stb_elem =  p_get_att_transition_elem(stb, &elem_already_in_the_list, l_track_nr);
      if(stb_elem)
      {
        stb_elem->file_line_nr = longlinenr;
        stb_elem->orig_src_line = g_strdup(multi_lines);
        stb_elem->track = l_track_nr;
        if(*l_overlap_nframes_ptr)
        {
          stb_elem->att_overlap = p_scan_gint32(l_overlap_nframes_ptr,  0, 999999, stb);
        }
        
        if(elem_already_in_the_list != TRUE)
        {
          gap_story_list_append_elem(stb, stb_elem);
        }
      }
      goto cleanup;
    }


    /* ATTRIBUTE: GAP_STBKEY_VID_OPACITY */
    if (strcmp(l_record_key, GAP_STBKEY_VID_OPACITY) == 0)
    {
      gboolean elem_already_in_the_list = FALSE;
      ii = GAP_STB_ATT_TYPE_OPACITY;
      
      stb_elem =  p_get_att_transition_elem(stb, &elem_already_in_the_list, l_track_nr);
      if(stb_elem)
      {
        stb_elem->att_arr_enable[ii] = TRUE;
        stb_elem->file_line_nr = longlinenr;
        stb_elem->orig_src_line = g_strdup(multi_lines);
        stb_elem->track = l_track_nr;
        if(*l_from_ptr)     { stb_elem->att_arr_value_from[ii] = p_scan_gdouble(l_from_ptr, 0.0, 1.0, stb); }
        if(*l_to_ptr)       { stb_elem->att_arr_value_to[ii]   = p_scan_gdouble(l_to_ptr,   0.0, 1.0, stb); }
        else                { stb_elem->att_arr_value_to[ii]   = stb_elem->att_arr_value_from[ii]; }
        if(*l_dur_ptr)      { stb_elem->att_arr_value_dur[ii]  = p_scan_gint32(l_dur_ptr,  1, 999, stb); }
        
        if(elem_already_in_the_list != TRUE)
        {
          gap_story_list_append_elem(stb, stb_elem);
        }
      }
      goto cleanup;
    }

    /* ATTRIBUTE: GAP_STBKEY_VID_ZOOM_X */
    if (strcmp(l_record_key, GAP_STBKEY_VID_ZOOM_X) == 0)
    {
      gboolean elem_already_in_the_list = FALSE;
      ii = GAP_STB_ATT_TYPE_ZOOM_X;
      
      stb_elem =  p_get_att_transition_elem(stb, &elem_already_in_the_list, l_track_nr);
      if(stb_elem)
      {
        stb_elem->att_arr_enable[ii] = TRUE;
        stb_elem->file_line_nr = longlinenr;
        stb_elem->orig_src_line = g_strdup(multi_lines);
        stb_elem->track = l_track_nr;
        if(*l_from_ptr)     { stb_elem->att_arr_value_from[ii] = p_scan_gdouble(l_from_ptr, 0.0001, 999.9, stb); }
        if(*l_to_ptr)       { stb_elem->att_arr_value_to[ii]   = p_scan_gdouble(l_to_ptr,   0.0001, 999.9, stb); }
        else                { stb_elem->att_arr_value_to[ii]   = stb_elem->att_arr_value_from[ii]; }
        if(*l_dur_ptr)      { stb_elem->att_arr_value_dur[ii]  = p_scan_gint32(l_dur_ptr,  1, 999, stb); }
        
        if(elem_already_in_the_list != TRUE)
        {
          gap_story_list_append_elem(stb, stb_elem);
        }
      }
      goto cleanup;
    }

    /* ATTRIBUTE: GAP_STBKEY_VID_ZOOM_Y */
    if (strcmp(l_record_key, GAP_STBKEY_VID_ZOOM_Y) == 0)
    {
      gboolean elem_already_in_the_list = FALSE;
      ii = GAP_STB_ATT_TYPE_ZOOM_Y;
      
      stb_elem =  p_get_att_transition_elem(stb, &elem_already_in_the_list, l_track_nr);
      if(stb_elem)
      {
        stb_elem->att_arr_enable[ii] = TRUE;
        stb_elem->file_line_nr = longlinenr;
        stb_elem->orig_src_line = g_strdup(multi_lines);
        stb_elem->track = l_track_nr;
        if(*l_from_ptr)     { stb_elem->att_arr_value_from[ii] = p_scan_gdouble(l_from_ptr, 0.0001, 999.9, stb); }
        if(*l_to_ptr)       { stb_elem->att_arr_value_to[ii]   = p_scan_gdouble(l_to_ptr,   0.0001, 999.9, stb); }
        else                { stb_elem->att_arr_value_to[ii]   = stb_elem->att_arr_value_from[ii]; }
        if(*l_dur_ptr)      { stb_elem->att_arr_value_dur[ii]  = p_scan_gint32(l_dur_ptr,  1, 999, stb); }
        
        if(elem_already_in_the_list != TRUE)
        {
          gap_story_list_append_elem(stb, stb_elem);
        }
      }
      goto cleanup;
    }

    /* ATTRIBUTE: GAP_STBKEY_VID_MOVE_X */
    if (strcmp(l_record_key, GAP_STBKEY_VID_MOVE_X) == 0)
    {
      gboolean elem_already_in_the_list = FALSE;
      ii = GAP_STB_ATT_TYPE_MOVE_X;
      
      stb_elem =  p_get_att_transition_elem(stb, &elem_already_in_the_list, l_track_nr);
      if(stb_elem)
      {
        stb_elem->att_arr_enable[ii] = TRUE;
        stb_elem->file_line_nr = longlinenr;
        stb_elem->orig_src_line = g_strdup(multi_lines);
        stb_elem->track = l_track_nr;
        if(*l_from_ptr)     { stb_elem->att_arr_value_from[ii] = p_scan_gdouble(l_from_ptr, -99999.9, 99999.9, stb); }
        if(*l_to_ptr)       { stb_elem->att_arr_value_to[ii]   = p_scan_gdouble(l_to_ptr,   -99999.9, 99999.9, stb); }
        else                { stb_elem->att_arr_value_to[ii]   = stb_elem->att_arr_value_from[ii]; }
        if(*l_dur_ptr)      { stb_elem->att_arr_value_dur[ii]  = p_scan_gint32(l_dur_ptr,  1, 999, stb); }
        
        if(elem_already_in_the_list != TRUE)
        {
          gap_story_list_append_elem(stb, stb_elem);
        }
      }
      goto cleanup;
    }

    /* ATTRIBUTE: GAP_STBKEY_VID_MOVE_Y */
    if (strcmp(l_record_key, GAP_STBKEY_VID_MOVE_Y) == 0)
    {
      gboolean elem_already_in_the_list = FALSE;
      ii = GAP_STB_ATT_TYPE_MOVE_Y;
      
      stb_elem =  p_get_att_transition_elem(stb, &elem_already_in_the_list, l_track_nr);
      if(stb_elem)
      {
        stb_elem->att_arr_enable[ii] = TRUE;
        stb_elem->file_line_nr = longlinenr;
        stb_elem->orig_src_line = g_strdup(multi_lines);
        stb_elem->track = l_track_nr;
        if(*l_from_ptr)     { stb_elem->att_arr_value_from[ii] = p_scan_gdouble(l_from_ptr, -99999.9, 99999.9, stb); }
        if(*l_to_ptr)       { stb_elem->att_arr_value_to[ii]   = p_scan_gdouble(l_to_ptr,   -99999.9, 99999.9, stb); }
        else                { stb_elem->att_arr_value_to[ii]   = stb_elem->att_arr_value_from[ii]; }
        if(*l_dur_ptr)      { stb_elem->att_arr_value_dur[ii]  = p_scan_gint32(l_dur_ptr,  1, 999, stb); }
        
        if(elem_already_in_the_list != TRUE)
        {
          gap_story_list_append_elem(stb, stb_elem);
        }
      }
      goto cleanup;
    } 
 
    /* ATTRIBUTE: GAP_STBKEY_VID_FIT_SIZE */
    if (strcmp(l_record_key, GAP_STBKEY_VID_FIT_SIZE) == 0)
    {
      gboolean elem_already_in_the_list = FALSE;
      stb_elem =  p_get_att_transition_elem(stb, &elem_already_in_the_list, l_track_nr);
      if(stb_elem)
      {
        stb_elem->file_line_nr = longlinenr;
        stb_elem->orig_src_line = g_strdup(multi_lines);
        stb_elem->att_keep_proportions = FALSE;
        stb_elem->att_fit_width = TRUE;
        stb_elem->att_fit_height = TRUE;
        stb_elem->nframes = 0;

        stb_elem->track = l_track_nr;
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
          if((strncmp(l_to_ptr, "keep", strlen("keep")) ==0)
          || (strncmp(l_to_ptr, "KEEP", strlen("KEEP")) ==0))
          {
            stb_elem->att_keep_proportions = TRUE;
          }
          else
          {
            if((strncmp(l_to_ptr, "change", strlen("change")) ==0)
            || (strncmp(l_to_ptr, "CHANGE", strlen("CHANGE")) ==0))
            {
              stb_elem->att_keep_proportions = FALSE;
            }
            else
            {
                   char *l_errtxt;
                   l_errtxt = g_strdup_printf(_("illegal keyword: %s (expected keywords are: keep, change")
                                             , l_to_ptr);
                   p_set_stb_warning(stb, l_errtxt);
                   g_free(l_errtxt);
            }
          }
        }
        
        if(elem_already_in_the_list != TRUE)
        {
          gap_story_list_append_elem(stb, stb_elem);
        }
      }
      goto cleanup;
      
    }  /* END ATTRIBUTE: GAP_STBKEY_VID_FIT_SIZE */

  }

  /* =========== Vid Attributes ===== end ============= */

  /* =========== Video element ====== start =========== */

  /* IMAGE: GAP_STBKEY_VID_PLAY_IMAGE  */
  if (strcmp(l_record_key, GAP_STBKEY_VID_PLAY_IMAGE) == 0)
  {
    char *l_track_ptr      = l_wordval[1];
    char *l_filename_ptr   = l_wordval[2];
    char *l_nloops_ptr     = l_wordval[3];
    char *l_macro_ptr      = l_wordval[4];
    char *l_flip_ptr          = l_wordval[5];
    char *l_mask_name_ptr     = l_wordval[6];
    char *l_mask_anchor_ptr   = l_wordval[7];
    char *l_mask_stepsize_ptr = l_wordval[8];
    char *l_mask_disable_ptr  = l_wordval[9];

    stb_elem = gap_story_new_elem(GAP_STBREC_VID_IMAGE);
    if(stb_elem)
    {
      stb_elem->file_line_nr = longlinenr;
      stb_elem->orig_src_line = g_strdup(multi_lines);

      if(*l_filename_ptr) { p_flip_dir_separators(l_filename_ptr); 
                            stb_elem->orig_filename = g_strdup(l_filename_ptr);
                          }      
      if(*l_track_ptr)    { stb_elem->track = p_scan_gint32(l_track_ptr,  1, GAP_STB_MAX_VID_TRACKS,  stb); }
      p_assign_parsed_video_values(stb_elem, stb
          , l_nloops_ptr
          , NULL              /* l_stepsize_ptr */
          , NULL              /* l_pingpong_ptr */
          , l_macro_ptr
          , l_flip_ptr
          , l_mask_name_ptr
          , l_mask_anchor_ptr
          , l_mask_stepsize_ptr
          , l_mask_disable_ptr
          );

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
    char *l_flip_ptr          = l_wordval[10];
    char *l_mask_name_ptr     = l_wordval[11];
    char *l_mask_anchor_ptr   = l_wordval[12];
    char *l_mask_stepsize_ptr = l_wordval[13];
    char *l_mask_disable_ptr  = l_wordval[14];

    stb_elem = gap_story_new_elem(GAP_STBREC_VID_FRAMES);
    if(stb_elem)
    {
      stb_elem->file_line_nr = longlinenr;
      stb_elem->orig_src_line = g_strdup(multi_lines);

      if(*l_track_ptr)    { stb_elem->track      = p_scan_gint32(l_track_ptr,   1, GAP_STB_MAX_VID_TRACKS, stb); }
      if(*l_from_ptr)     { stb_elem->from_frame = p_scan_gint32(l_from_ptr,    0, 999999, stb); }
      if(*l_to_ptr)       { stb_elem->to_frame   = p_scan_gint32(l_to_ptr,      0, 999999, stb); }
      if(*l_basename_ptr) { p_flip_dir_separators(l_basename_ptr); 
                            stb_elem->basename = g_strdup(l_basename_ptr);
                          }
      if(*l_ext_ptr)      { if(*l_ext_ptr == '.') stb_elem->ext = g_strdup(l_ext_ptr);
                            else                  stb_elem->ext = g_strdup_printf(".%s", l_ext_ptr);
                          }

      p_assign_parsed_video_values(stb_elem, stb
          , l_nloops_ptr
          , l_stepsize_ptr
          , l_pingpong_ptr
          , l_macro_ptr
          , l_flip_ptr
          , l_mask_name_ptr
          , l_mask_anchor_ptr
          , l_mask_stepsize_ptr
          , l_mask_disable_ptr
          );

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
    char *l_flip_ptr          = l_wordval[13];
    char *l_mask_name_ptr     = l_wordval[14];
    char *l_mask_anchor_ptr   = l_wordval[15];
    char *l_mask_stepsize_ptr = l_wordval[16];
    char *l_mask_disable_ptr  = l_wordval[17];

    stb_elem = gap_story_new_elem(GAP_STBREC_VID_MOVIE);
    if(stb_elem)
    {
      stb_elem->file_line_nr = longlinenr;
      stb_elem->orig_src_line = g_strdup(multi_lines);

      if(*l_track_ptr)    { stb_elem->track      = p_scan_gint32(l_track_ptr, 1, GAP_STB_MAX_VID_TRACKS, stb); }
      if(*l_filename_ptr) { p_flip_dir_separators(l_filename_ptr); 
                            stb_elem->orig_filename = g_strdup(l_filename_ptr);
                          }
      if(*l_from_ptr)     { stb_elem->from_frame = p_scan_gint32(l_from_ptr,    1, 999999, stb); }
      if(*l_to_ptr)       { stb_elem->to_frame   = p_scan_gint32(l_to_ptr,      1, 999999, stb); }

      if(*l_seltrack_ptr)   { stb_elem->seltrack     = p_scan_gint32(l_seltrack_ptr,  1, 999999, stb); }
      if(*l_exact_seek_ptr) { stb_elem->exact_seek   = p_scan_gint32(l_exact_seek_ptr,  0, 1, stb); }
      if(*l_delace_ptr)     { stb_elem->delace       = p_scan_gdouble(l_delace_ptr, 0.0, 3.0, stb); }
      if(*l_decoder_ptr)    { stb_elem->preferred_decoder = g_strdup(l_decoder_ptr);
                            }

      p_assign_parsed_video_values(stb_elem, stb
          , l_nloops_ptr
          , l_stepsize_ptr
          , l_pingpong_ptr
          , l_macro_ptr
          , l_flip_ptr
          , l_mask_name_ptr
          , l_mask_anchor_ptr
          , l_mask_stepsize_ptr
          , l_mask_disable_ptr
          );



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
    char *l_macro_ptr      = l_wordval[7];
    char *l_flip_ptr          = l_wordval[8];
    char *l_mask_name_ptr     = l_wordval[9];
    char *l_mask_anchor_ptr   = l_wordval[10];
    char *l_mask_stepsize_ptr = l_wordval[11];
    char *l_mask_disable_ptr  = l_wordval[12];

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

      p_assign_parsed_video_values(stb_elem, stb
          , l_nloops_ptr
          , NULL                /* l_stepsize_ptr */
          , NULL                /* l_pingpong_ptr */
          , l_macro_ptr
          , l_flip_ptr
          , l_mask_name_ptr
          , l_mask_anchor_ptr
          , l_mask_stepsize_ptr
          , l_mask_disable_ptr
          );

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
    char *l_stepsize_ptr   = l_wordval[7];
    char *l_macro_ptr      = l_wordval[8];
    char *l_flip_ptr          = l_wordval[9];
    char *l_mask_name_ptr     = l_wordval[10];
    char *l_mask_anchor_ptr   = l_wordval[11];
    char *l_mask_stepsize_ptr = l_wordval[12];
    char *l_mask_disable_ptr  = l_wordval[13];

    stb_elem = gap_story_new_elem(GAP_STBREC_VID_ANIMIMAGE);
    if(stb_elem)
    {
      stb_elem->file_line_nr = longlinenr;
      stb_elem->orig_src_line = g_strdup(multi_lines);
      if(*l_track_ptr)    { stb_elem->track      = p_scan_gint32(l_track_ptr, 1, GAP_STB_MAX_VID_TRACKS, stb); }
      if(*l_filename_ptr) { p_flip_dir_separators(l_filename_ptr); 
                            stb_elem->orig_filename = g_strdup(l_filename_ptr);
                          }
      if(*l_from_ptr)     { stb_elem->from_frame = p_scan_gint32(l_from_ptr,    0, 999999, stb); }
      if(*l_to_ptr)       { stb_elem->to_frame   = p_scan_gint32(l_to_ptr,      0, 999999, stb); }

      p_assign_parsed_video_values(stb_elem, stb
          , l_nloops_ptr
          , l_stepsize_ptr
          , l_pingpong_ptr
          , l_macro_ptr
          , l_flip_ptr
          , l_mask_name_ptr
          , l_mask_anchor_ptr
          , l_mask_stepsize_ptr
          , l_mask_disable_ptr
          );

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

  /* =========== Video element ====== end ============ */

  /* =========== Audio element ====== start =========== */

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
      if(*l_filename_ptr)     { p_flip_dir_separators(l_filename_ptr); 
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

      if(gap_debug)
      {
        printf("\n##++ GAP_STBREC_AUD_SOUND\n");
        gap_story_debug_print_elem(stb_elem);
      }

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
    char *l_from_ptr          = l_wordval[13];
    char *l_to_ptr            = l_wordval[14];
    char *l_frate_ptr         = l_wordval[15];
    gint32  l_from_frame;
    gint32  l_to_frame;
    gint32  l_min_frame;
    gint32  l_max_frame;
    gdouble l_framerate;


    stb_elem = gap_story_new_elem(GAP_STBREC_AUD_MOVIE);
    if(stb_elem)
    {
      stb_elem->file_line_nr = longlinenr;
      stb_elem->orig_src_line = g_strdup(multi_lines);
      stb_elem->aud_play_to_sec = 9999.9;  /* 9999.9 is default for end of audiofile */
      if(*l_track_ptr)        { stb_elem->track      = p_scan_gint32(l_track_ptr,   1, GAP_STB_MAX_AUD_TRACKS,    stb); }
      if(*l_filename_ptr)     { p_flip_dir_separators(l_filename_ptr); 
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

      /* optional positioning in unit frames */
      l_from_frame = -1;
      l_to_frame = -1;
      
      /* per default we use the master framerate for frame base positioning
       * (correctly we should get the original framerate used in the videofile 
       *  where the audio is extracted from)
       */
      l_framerate = stb->master_framerate;

      if(*l_from_ptr)     { l_from_frame = p_scan_gint32(l_from_ptr,     0, 999999, stb); }
      if(*l_to_ptr)       { l_to_frame   = p_scan_gint32(l_to_ptr,       0, 999999, stb); }
      if(*l_frate_ptr)    { l_framerate  = p_scan_gdouble(l_frate_ptr,   1.0, 999.9, stb); }

      if(l_framerate < 1)
      {
        l_framerate = 1;
      }

      l_min_frame = MIN(l_from_frame, l_to_frame);
      l_max_frame = MAX(l_from_frame, l_to_frame);
      
      if((l_min_frame > 0) && (l_max_frame > 0))
      {
        /* framenumbers starts at 1. frame one has 0 sec starttime */ 
        if(gap_debug)
        {
          printf("AUD_MOVIE framerate based timerange calculation  from:%d to:%d  l_framerate: %f\n"
             , (int)l_min_frame
             , (int)l_max_frame
             , (float)l_framerate
             );
        }

        stb_elem->aud_play_from_sec = (gdouble)(l_min_frame -1) / l_framerate;
        stb_elem->from_frame = l_min_frame;
        stb_elem->aud_framerate = l_framerate;

        stb_elem->aud_play_to_sec = (gdouble)(l_max_frame) / l_framerate;
        stb_elem->to_frame = l_max_frame;
      }

      gap_story_list_append_elem(stb, stb_elem);
    }
    goto cleanup;
  }  

  /* =========== Audio element ====== end ============= */


  /* =========== MASK definitions === start =========== */

  /* MASK: GAP_STBKEY_MASK_IMAGE  */
  if (strcmp(l_record_key, GAP_STBKEY_MASK_IMAGE) == 0)
  {
    char *l_mask_name_ptr  = l_wordval[1];
    char *l_filename_ptr   = l_wordval[2];
    char *l_flip_ptr       = l_wordval[3];

    stb_elem = gap_story_new_mask_elem(GAP_STBREC_VID_IMAGE);
    if(stb_elem)
    {
      stb_elem->file_line_nr = longlinenr;
      stb_elem->orig_src_line = g_strdup(multi_lines);

      if(*l_filename_ptr)  { p_flip_dir_separators(l_filename_ptr); 
                             stb_elem->orig_filename = g_strdup(l_filename_ptr);
                           }      
      if(*l_mask_name_ptr) { stb_elem->mask_name = g_strdup(l_mask_name_ptr); }
      
      p_assign_parsed_flip_value(stb_elem, l_flip_ptr);
      
      stb_elem->nframes = 1;

      p_check_image_numpart(stb_elem, l_filename_ptr);

      gap_story_list_append_elem(stb, stb_elem);
    }
    
    goto cleanup;

  }  /* end MASK_IMAGE */

  /* MASK: GAP_STBKEY_MASK_FRAMES */
  if (strcmp(l_record_key, GAP_STBKEY_MASK_FRAMES) == 0)
  {
    char *l_mask_name_ptr  = l_wordval[1];
    char *l_basename_ptr   = l_wordval[2];
    char *l_ext_ptr        = l_wordval[3];
    char *l_from_ptr       = l_wordval[4];
    char *l_to_ptr         = l_wordval[5];
    char *l_flip_ptr       = l_wordval[6];

    stb_elem = gap_story_new_mask_elem(GAP_STBREC_VID_FRAMES);
    if(stb_elem)
    {
      stb_elem->file_line_nr = longlinenr;
      stb_elem->orig_src_line = g_strdup(multi_lines);

      if(*l_mask_name_ptr) { stb_elem->mask_name = g_strdup(l_mask_name_ptr); }
      if(*l_from_ptr)      { stb_elem->from_frame = p_scan_gint32(l_from_ptr,    0, 999999, stb); }
      if(*l_to_ptr)        { stb_elem->to_frame   = p_scan_gint32(l_to_ptr,      0, 999999, stb); }
      if(*l_basename_ptr)  { p_flip_dir_separators(l_basename_ptr); 
                             stb_elem->basename = g_strdup(l_basename_ptr);
                           }
      if(*l_ext_ptr)       { if(*l_ext_ptr == '.') stb_elem->ext = g_strdup(l_ext_ptr);
                             else                  stb_elem->ext = g_strdup_printf(".%s", l_ext_ptr);
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
      
      p_assign_parsed_flip_value(stb_elem, l_flip_ptr);

      stb_elem->nframes = ABS(stb_elem->from_frame - stb_elem->to_frame) + 1;
 
      gap_story_list_append_elem(stb, stb_elem);
    }
    
    goto cleanup;

  }   /* end MASK FRAMES */

  /* MASK: GAP_STBKEY_MASK_MOVIE */
  if (strcmp(l_record_key, GAP_STBKEY_MASK_MOVIE) == 0)
  {
    char *l_mask_name_ptr  = l_wordval[1];
    char *l_filename_ptr   = l_wordval[2];
    char *l_from_ptr       = l_wordval[3];
    char *l_to_ptr         = l_wordval[4];
    char *l_seltrack_ptr   = l_wordval[5];
    char *l_exact_seek_ptr = l_wordval[6];
    char *l_delace_ptr     = l_wordval[7];
    char *l_decoder_ptr    = l_wordval[8];
    char *l_flip_ptr       = l_wordval[9];

    stb_elem = gap_story_new_mask_elem(GAP_STBREC_VID_MOVIE);
    if(stb_elem)
    {
      stb_elem->file_line_nr = longlinenr;
      stb_elem->orig_src_line = g_strdup(multi_lines);

      if(*l_mask_name_ptr)  { stb_elem->mask_name = g_strdup(l_mask_name_ptr); }
      if(*l_filename_ptr)   { p_flip_dir_separators(l_filename_ptr); 
                              stb_elem->orig_filename = g_strdup(l_filename_ptr);
                            }
      if(*l_from_ptr)       { stb_elem->from_frame = p_scan_gint32(l_from_ptr,    1, 999999, stb); }
      if(*l_to_ptr)         { stb_elem->to_frame   = p_scan_gint32(l_to_ptr,      1, 999999, stb); }

      if(*l_seltrack_ptr)   { stb_elem->seltrack     = p_scan_gint32(l_seltrack_ptr,  1, 999999, stb); }
      if(*l_exact_seek_ptr) { stb_elem->exact_seek   = p_scan_gint32(l_exact_seek_ptr,  0, 1, stb); }
      if(*l_delace_ptr)     { stb_elem->delace       = p_scan_gdouble(l_delace_ptr, 0.0, 3.0, stb); }

      if(*l_decoder_ptr)    { stb_elem->preferred_decoder = g_strdup(l_decoder_ptr);
                            }
      
      p_assign_parsed_flip_value(stb_elem, l_flip_ptr);

      stb_elem->nframes = ABS(stb_elem->from_frame - stb_elem->to_frame) + 1;

      gap_story_list_append_elem(stb, stb_elem);
    }
    
    goto cleanup;

  }   /* end MASK_MOVIE */

  /* MASK: GAP_STBKEY_MASK_ANIMIMAGE */
  if (strcmp(l_record_key, GAP_STBKEY_MASK_ANIMIMAGE) == 0)
  {
    char *l_mask_name_ptr  = l_wordval[1];
    char *l_filename_ptr   = l_wordval[2];
    char *l_from_ptr       = l_wordval[3];
    char *l_to_ptr         = l_wordval[4];
    char *l_flip_ptr       = l_wordval[5];

    stb_elem = gap_story_new_mask_elem(GAP_STBREC_VID_ANIMIMAGE);
    if(stb_elem)
    {
      stb_elem->file_line_nr = longlinenr;
      stb_elem->orig_src_line = g_strdup(multi_lines);
      if(*l_mask_name_ptr) { stb_elem->mask_name = g_strdup(l_mask_name_ptr); }
      if(*l_filename_ptr)  { p_flip_dir_separators(l_filename_ptr); 
                             stb_elem->orig_filename = g_strdup(l_filename_ptr);
                           }
      if(*l_from_ptr)      { stb_elem->from_frame = p_scan_gint32(l_from_ptr,    0, 999999, stb); }
      if(*l_to_ptr)        { stb_elem->to_frame   = p_scan_gint32(l_to_ptr,      0, 999999, stb); }
      
      p_assign_parsed_flip_value(stb_elem, l_flip_ptr);

      stb_elem->nframes = ABS(stb_elem->from_frame - stb_elem->to_frame) + 1;

      gap_story_list_append_elem(stb, stb_elem);
    }
    
    goto cleanup;

  }   /* end MASK_ANIMIMAGE */


  /* =========== MASK definitions === end ============= */



  /* accept lines with filenames as implicite VID_PLAY_IMAGE commands */
  {
    char *l_filename_ptr;
    
    l_filename_ptr = g_strdup(l_record_key);
    if(l_filename_ptr)
    {
      p_flip_dir_separators(l_filename_ptr);

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
    p_set_stb_error(stb, _("Unsupported line was ignored"));
  }
  else
  {
    /* accept unsupported lines (with just a warning)
     * because the file has correct Header
     */
    p_set_stb_warning(stb, _("Unsupported line was ignored"));
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
 * The 1.st error that occurred is reported in the errtext pointer
 * errtext is a member of the returned GapStoryBoard structure
 * (if no error occurred the errtxt pointer is  NULL)
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
 * p_story_elem_calculate_nframes
 * --------------------------------
 * returns number of overlapping frames
 */
static gint32 
p_story_elem_calculate_nframes(GapStoryElem *stb_elem)
{
  if((stb_elem->record_type == GAP_STBREC_VID_SILENCE)
  || (stb_elem->record_type == GAP_STBREC_VID_IMAGE)
  || (stb_elem->record_type == GAP_STBREC_VID_COLOR))
  {
    stb_elem->nframes = stb_elem->nloop;
  }
  if((stb_elem->record_type == GAP_STBREC_VID_FRAMES)
  || (stb_elem->record_type == GAP_STBREC_VID_ANIMIMAGE)
  || (stb_elem->record_type == GAP_STBREC_VID_MOVIE))
  {
    /* calculate number of frames in this clip
     * respecting playmode and nloop count
     */
    if ((stb_elem->playmode == GAP_STB_PM_PINGPONG) 
    &&  (stb_elem->from_frame != stb_elem->to_frame))
    {
      stb_elem->nframes = p_calculate_rangesize_pingpong(stb_elem) * stb_elem->nloop;
    }
    else
    {
      stb_elem->nframes = p_calculate_rangesize(stb_elem) * stb_elem->nloop;
    }
  }
  if(stb_elem->record_type == GAP_STBREC_ATT_TRANSITION)
  {
      stb_elem->nframes = 0;
      return(MAX(0, stb_elem->att_overlap));
  }
  return (0);
  
}  /* end p_story_elem_calculate_nframes */

/* --------------------------------
 * gap_story_elem_calculate_nframes
 * --------------------------------
 */
void 
gap_story_elem_calculate_nframes(GapStoryElem *stb_elem)
{
  p_story_elem_calculate_nframes(stb_elem);
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
 * p_story_save_att_overlap
 * ------------------------------
 */
static void
p_story_save_att_overlap(FILE *fp, GapStoryElem *stb_elem)
{
  if(stb_elem->att_overlap > 0)
  {
    GapStbSyntaxParnames l_parnam_tab;

    gap_stb_syntax_get_parname_tab(GAP_STBKEY_VID_OVERLAP
                                ,&l_parnam_tab
                                );
    fprintf(fp, "%s         %s:%d %s:%d\n"
                 , GAP_STBKEY_VID_OVERLAP
                 , l_parnam_tab.parname[1]              , (int)stb_elem->track
                 , l_parnam_tab.parname[2]              , (int)stb_elem->att_overlap
                 );

  }
}  /* end p_story_save_att_overlap */


/* ------------------------------
 * p_story_save_att_transition
 * ------------------------------
 */
static void
p_story_save_att_transition(FILE *fp, GapStoryElem *stb_elem)
{
  gint ii;
  GapStbSyntaxParnames l_parnam_tab;
  const char *tab_fit_modes[4] =
    { "none"
    , "width"
    , "height"
    , "both"
    };
  const char *tab_prop_modes[2] =
    { "change"
    , "keep"
    };

  gint fmi;
  gint pmi;

  p_story_save_att_overlap(fp, stb_elem);
  
  fmi = 0;
  if(stb_elem->att_fit_width == TRUE)
  {
    fmi += 1;
  }
  if(stb_elem->att_fit_height == TRUE)
  {
    fmi += 2;
  }
  
  pmi = 0;
  if(stb_elem->att_keep_proportions == TRUE)
  {
    pmi = 1;
  }
  
  gap_stb_syntax_get_parname_tab(GAP_STBKEY_VID_FIT_SIZE
                                ,&l_parnam_tab
                                );
  fprintf(fp, "%s        %s:%d %s:%s %s:%s\n"
                 , GAP_STBKEY_VID_FIT_SIZE
                 , l_parnam_tab.parname[1]              , (int)stb_elem->track
                 , l_parnam_tab.parname[2]              , tab_fit_modes[fmi]
                 , l_parnam_tab.parname[3]              , tab_prop_modes[pmi]
                 );

  
  for(ii=0; ii < GAP_STB_ATT_TYPES_ARRAY_MAX; ii++)
  {
    if(stb_elem->att_arr_enable[ii])
    {
      gchar l_from_str[G_ASCII_DTOSTR_BUF_SIZE];
      gchar l_to_str[G_ASCII_DTOSTR_BUF_SIZE];

      gap_stb_syntax_get_parname_tab(gtab_att_transition_key_words[ii]
                                    ,&l_parnam_tab
                                    );

      g_ascii_dtostr(&l_from_str[0]
                     ,G_ASCII_DTOSTR_BUF_SIZE
                     ,stb_elem->att_arr_value_from[ii]
                     );
      g_ascii_dtostr(&l_to_str[0]
                     ,G_ASCII_DTOSTR_BUF_SIZE
                     ,stb_elem->att_arr_value_to[ii]
                     );

      fprintf(fp, "%s         "
                 , gtab_att_transition_key_words[ii]
                 );
      if(ii !=0 )
      {
        /* print one extra space
         * VID_MOVE_X, VID_MOVE_Y, VID_ZOOM_X, VID_ZOOM_Y
         * are 1 character shorter than VID_OPACITY
         */
        fprintf(fp, " ");
      }
                       
      fprintf(fp, "%s:%d %s:%s %s:%s %s:%d\n"
                 , l_parnam_tab.parname[1]              , (int)stb_elem->track
                 , l_parnam_tab.parname[2]              , l_from_str
                 , l_parnam_tab.parname[3]              , l_to_str
                 , l_parnam_tab.parname[4]              , stb_elem->att_arr_value_dur[ii]
                 );
    }
  }
  
}  /* end p_story_save_att_transition */


/* ---------------------------------
 * p_story_save_flip_param
 * ---------------------------------
 * print the optional flip and mask parameters
 * to file (only in case where specified and necessary)
 */
static void
p_story_save_flip_param(FILE *fp, GapStoryElem *stb_elem
  , const char *parname_flip
  )
{
  switch(stb_elem->flip_request)
  {
    case GAP_STB_FLIP_HOR:
      fprintf(fp, " %s:hor"
               , parname_flip
               );
      break;    
    case GAP_STB_FLIP_VER:
      fprintf(fp, " %s:ver"
               , parname_flip
               );
      break;    
    case GAP_STB_FLIP_BOTH:
      fprintf(fp, " %s:both"
               , parname_flip
               );
      break;    
    default:
      /* do not print the default GAP_STB_FLIP_NONE to file */
      break;    
  }

}  /* end p_story_save_flip_param */


/* ------------------------------
 * p_story_save_header
 * ------------------------------
 */
static void
p_story_save_header(GapStoryBoard *stb, FILE *fp)
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

  /* print MASTER_FRAME_ASPECT */
  if((stb->master_aspect_width > 0) && (stb->master_aspect_height > 0))
  {
    gap_stb_syntax_get_parname_tab(GAP_STBKEY_VID_MASTER_FRAME_ASPECT
                                  ,&l_parnam_tab
                                  );
    fprintf(fp, "%s         %s:%d %s:%d\n"
         , GAP_STBKEY_VID_MASTER_SIZE
         , l_parnam_tab.parname[1]
         , (int)stb->master_aspect_width
         , l_parnam_tab.parname[2]
         , (int)stb->master_aspect_height
         );
  }


  /* print MASTER_SAMPLERATE */
  if(stb->master_samplerate > 0.0)
  {
     gap_stb_syntax_get_parname_tab(GAP_STBKEY_AUD_MASTER_SAMPLERATE
                                  ,&l_parnam_tab
                                  );
     fprintf(fp, "%s   %s:%d\n"
         , GAP_STBKEY_AUD_MASTER_SAMPLERATE
         , l_parnam_tab.parname[1]
         , (int)stb->master_samplerate
         );
  }

  /* print MASTER_VOLUME */
  if(stb->master_volume >= 0.0)
  {
     gchar l_dbl_str[G_ASCII_DTOSTR_BUF_SIZE];

     /* setlocale independent float string */
     g_ascii_dtostr(&l_dbl_str[0]
                   ,G_ASCII_DTOSTR_BUF_SIZE
                   ,stb->master_volume
                   );
     gap_stb_syntax_get_parname_tab(GAP_STBKEY_AUD_MASTER_VOLUME
                                  ,&l_parnam_tab
                                  );
     fprintf(fp, "%s       %s:%s\n"
         , GAP_STBKEY_AUD_MASTER_VOLUME
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

}  /* end p_story_save_header */


/* ------------------------------
 * p_story_save_mask_elem_list
 * ------------------------------
 * save elements of the mask track as mask definition records.
 */
static void
p_story_save_mask_elem_list(GapStoryBoard *stb, FILE *fp)
{
  GapStoryElem *stb_elem;

  for(stb_elem = stb->stb_elem; stb_elem != NULL; stb_elem = (GapStoryElem *)stb_elem->next)
  {
    GapStbSyntaxParnames l_parnam_tab;
    
    if(stb_elem->track != GAP_STB_MASK_TRACK_NUMBER)
    {
      continue;
    }
    
    switch(stb_elem->record_type)
    {
      case GAP_STBREC_VID_FRAMES:
        {
          gap_stb_syntax_get_parname_tab(GAP_STBKEY_MASK_FRAMES
                                  ,&l_parnam_tab
                                  );
          fprintf(fp, "%s     %s:\"%s\" %s:\"%s\" %s:%s %s:%06d %s:%06d"
               , GAP_STBKEY_MASK_FRAMES
               , l_parnam_tab.parname[1], stb_elem->mask_name
               , l_parnam_tab.parname[2], stb_elem->basename
               , l_parnam_tab.parname[3], stb_elem->ext
               , l_parnam_tab.parname[4], (int)stb_elem->from_frame
               , l_parnam_tab.parname[5], (int)stb_elem->to_frame
               );
          p_story_save_flip_param(fp, stb_elem, l_parnam_tab.parname[6]);
          fprintf(fp, "\n");
        }
        break;
      case GAP_STBREC_VID_IMAGE:
        {
          gap_stb_syntax_get_parname_tab(GAP_STBKEY_MASK_IMAGE
                                  ,&l_parnam_tab
                                  );
          fprintf(fp, "%s     %s:\"%s\" %s:\"%s\""
               , GAP_STBKEY_MASK_IMAGE
               , l_parnam_tab.parname[1], stb_elem->mask_name
               , l_parnam_tab.parname[2], stb_elem->orig_filename
               );
          p_story_save_flip_param(fp, stb_elem, l_parnam_tab.parname[3]);
          fprintf(fp, "\n");
        }
        break;
      case GAP_STBREC_VID_ANIMIMAGE:
        {
          gap_stb_syntax_get_parname_tab(GAP_STBKEY_MASK_ANIMIMAGE
                                  ,&l_parnam_tab
                                  );
          fprintf(fp, "%s     %s:\"%s\" %s:\"%s\" %s:%06d %s:%06d"
               , GAP_STBKEY_MASK_ANIMIMAGE
               , l_parnam_tab.parname[1], stb_elem->mask_name
               , l_parnam_tab.parname[2], stb_elem->orig_filename
               , l_parnam_tab.parname[3], (int)stb_elem->from_frame
               , l_parnam_tab.parname[4], (int)stb_elem->to_frame
               );
          p_story_save_flip_param(fp, stb_elem, l_parnam_tab.parname[5]);
          fprintf(fp, "\n");
        }
        break;
      case GAP_STBREC_VID_MOVIE:
        {
          gint   l_delace_int;
          gap_stb_syntax_get_parname_tab(GAP_STBKEY_MASK_MOVIE
                                  ,&l_parnam_tab
                                  );
          l_delace_int = (int)stb_elem->delace;
          fprintf(fp, "%s     %s:\"%s\" %s:\"%s\" %s:%06d %s:%06d %s:%d %s:%d %s:%d.%d"
               , GAP_STBKEY_MASK_MOVIE
               , l_parnam_tab.parname[1], stb_elem->mask_name
               , l_parnam_tab.parname[2], stb_elem->orig_filename
               , l_parnam_tab.parname[3], (int)stb_elem->from_frame
               , l_parnam_tab.parname[4], (int)stb_elem->to_frame
               
               , l_parnam_tab.parname[5], (int)stb_elem->seltrack
               , l_parnam_tab.parname[6], (int)stb_elem->exact_seek
               , l_parnam_tab.parname[7], (int)l_delace_int
               , (int)((gdouble)(1000.0 * (stb_elem->delace - (gdouble)l_delace_int)))
               );
          if(stb_elem->preferred_decoder)
          {
            fprintf(fp, " %s:\"%s\""
               , l_parnam_tab.parname[8]
               , stb_elem->preferred_decoder
               );
          }
          p_story_save_flip_param(fp, stb_elem, l_parnam_tab.parname[9]);

          fprintf(fp, "\n");
        }
        break;
      default:
        break;
    }

  }
}  /* end p_story_save_mask_elem_list */


/* ------------------------------
 * p_story_save_audio_list
 * ------------------------------
 */
static void
p_story_save_audio_list(GapStoryBoard *stb, FILE *fp, gint32 save_track)
{
  GapStoryElem *stb_elem;
  GapStbSyntaxParnames l_parnam_tab;
  gboolean empty_track;


  empty_track = TRUE;
  for(stb_elem = stb->stb_elem; stb_elem != NULL; stb_elem = (GapStoryElem *)stb_elem->next)
  {
    gchar l_dbl_str_step_density[G_ASCII_DTOSTR_BUF_SIZE];
    gchar l_dbl_str_mask_stepsize[G_ASCII_DTOSTR_BUF_SIZE];
    gchar l_dbl_str[G_ASCII_DTOSTR_BUF_SIZE];


    if((stb_elem->track != save_track)
    || (!gap_story_elem_is_audio(stb_elem)))
    {
      continue;
    }
    else
    {
      if(empty_track)
      {
        /* add empty line at start of new track */
        fprintf(fp, "\n");
      }
      empty_track = FALSE;
    }
   

    /* setlocale independent float string */
    g_ascii_dtostr(&l_dbl_str_step_density[0]
                   ,G_ASCII_DTOSTR_BUF_SIZE
                   ,stb_elem->step_density
                   );
    g_ascii_dtostr(&l_dbl_str_mask_stepsize[0]
                   ,G_ASCII_DTOSTR_BUF_SIZE
                   ,stb_elem->mask_stepsize
                   );

    switch(stb_elem->record_type)
    {
      case GAP_STBREC_AUD_MOVIE:
        {
          gap_stb_syntax_get_parname_tab(GAP_STBKEY_AUD_PLAY_MOVIE
                                  ,&l_parnam_tab
                                  );
          p_story_save_comments(fp, stb_elem);

          fprintf(fp, "%s      %s:%d %s:\"%s\" "
               , GAP_STBKEY_AUD_PLAY_MOVIE
               , l_parnam_tab.parname[1], (int)stb_elem->track
               , l_parnam_tab.parname[2], stb_elem->orig_filename
               );

          if(stb_elem->to_frame <= 0.0)
          {
            /* use values in seconds when
             * no valid frame range values are available
             */
            g_ascii_dtostr(&l_dbl_str[0]
                     ,G_ASCII_DTOSTR_BUF_SIZE
                     ,stb_elem->aud_play_from_sec
                     );
               
            fprintf(fp, "%s:%s "
                  , l_parnam_tab.parname[3], l_dbl_str
                 );
                 
            g_ascii_dtostr(&l_dbl_str[0]
                     ,G_ASCII_DTOSTR_BUF_SIZE
                     ,stb_elem->aud_play_to_sec
                     );
                 
            fprintf(fp, "%s:%s "
                 , l_parnam_tab.parname[4], l_dbl_str
                 );
          }     
          g_ascii_dtostr(&l_dbl_str[0]
                   ,G_ASCII_DTOSTR_BUF_SIZE
                   ,stb_elem->aud_volume
                   );
               
          fprintf(fp, "%s:%s "
               , l_parnam_tab.parname[5], l_dbl_str
               );

          if((stb_elem->aud_fade_in_sec > 0.0)
          || (stb_elem->aud_fade_out_sec > 0.0))
          {
            g_ascii_dtostr(&l_dbl_str[0]
                   ,G_ASCII_DTOSTR_BUF_SIZE
                   ,stb_elem->aud_volume_start
                   );
            fprintf(fp, "%s:%s "
                   , l_parnam_tab.parname[6], l_dbl_str
                   );

            g_ascii_dtostr(&l_dbl_str[0]
                   ,G_ASCII_DTOSTR_BUF_SIZE
                   ,stb_elem->aud_fade_in_sec
                   );
            fprintf(fp, "%s:%s "
                   , l_parnam_tab.parname[7], l_dbl_str
                   );

            g_ascii_dtostr(&l_dbl_str[0]
                   ,G_ASCII_DTOSTR_BUF_SIZE
                   ,stb_elem->aud_volume_end
                   );
            fprintf(fp, "%s:%s "
                   , l_parnam_tab.parname[8], l_dbl_str
                   );

            g_ascii_dtostr(&l_dbl_str[0]
                   ,G_ASCII_DTOSTR_BUF_SIZE
                   ,stb_elem->aud_fade_out_sec
                   );
               
            fprintf(fp, "%s:%s "
                   , l_parnam_tab.parname[9], l_dbl_str
                   );
          }

          fprintf(fp, "%s:%d %s:%d "
               , l_parnam_tab.parname[10], (int)stb_elem->nloop
               , l_parnam_tab.parname[11], (int)stb_elem->aud_seltrack
               );

          if(stb_elem->preferred_decoder)
          {
            fprintf(fp, " %s:\"%s\""
               , l_parnam_tab.parname[12]
               , stb_elem->preferred_decoder
               );
          }
               
          if(stb_elem->to_frame > 0)
          {
            g_ascii_dtostr(&l_dbl_str[0]
                   ,G_ASCII_DTOSTR_BUF_SIZE
                   ,stb_elem->aud_framerate
                   );
            fprintf(fp, "%s:%06d %s:%06d %s:%s "
               , l_parnam_tab.parname[13], (int)stb_elem->from_frame
               , l_parnam_tab.parname[14], (int)stb_elem->to_frame
               , l_parnam_tab.parname[15], l_dbl_str
               );
          }

          fprintf(fp, "\n");
        }
        break;
      case GAP_STBREC_AUD_SILENCE:
        {
          gap_stb_syntax_get_parname_tab(GAP_STBKEY_AUD_SILENCE
                                  ,&l_parnam_tab
                                  );
          p_story_save_comments(fp, stb_elem);

          g_ascii_dtostr(&l_dbl_str[0]
                   ,G_ASCII_DTOSTR_BUF_SIZE
                   ,stb_elem->aud_play_to_sec
                   );
          fprintf(fp, "%s      %s:%d %s:%s "
               , GAP_STBKEY_AUD_SILENCE
               , l_parnam_tab.parname[1], (int)stb_elem->track
               , l_parnam_tab.parname[2], l_dbl_str
               );
          if(stb_elem->aud_wait_untiltime_sec > 0.0)
          {
            g_ascii_dtostr(&l_dbl_str[0]
                   ,G_ASCII_DTOSTR_BUF_SIZE
                   ,stb_elem->aud_wait_untiltime_sec
                   );
            fprintf(fp, "%s:%s "
               , l_parnam_tab.parname[3], l_dbl_str
               );
          }
          fprintf(fp, "\n");
        }
        break;
      case GAP_STBREC_AUD_SOUND:
        {
          gap_stb_syntax_get_parname_tab(GAP_STBKEY_AUD_PLAY_SOUND
                                  ,&l_parnam_tab
                                  );
          p_story_save_comments(fp, stb_elem);

          fprintf(fp, "%s      %s:%d %s:\"%s\" "
               , GAP_STBKEY_AUD_PLAY_SOUND
               , l_parnam_tab.parname[1], (int)stb_elem->track
               , l_parnam_tab.parname[2], stb_elem->orig_filename
               );

          g_ascii_dtostr(&l_dbl_str[0]
                   ,G_ASCII_DTOSTR_BUF_SIZE
                   ,stb_elem->aud_play_from_sec
                   );
             
          fprintf(fp, "%s:%s "
                , l_parnam_tab.parname[3], l_dbl_str
               );
               
          g_ascii_dtostr(&l_dbl_str[0]
                   ,G_ASCII_DTOSTR_BUF_SIZE
                   ,stb_elem->aud_play_to_sec
                   );
               
          fprintf(fp, "%s:%s "
               , l_parnam_tab.parname[4], l_dbl_str
               );

          g_ascii_dtostr(&l_dbl_str[0]
                   ,G_ASCII_DTOSTR_BUF_SIZE
                   ,stb_elem->aud_volume
                   );
               
          fprintf(fp, "%s:%s "
               , l_parnam_tab.parname[5], l_dbl_str
               );

          if((stb_elem->aud_fade_in_sec > 0.0)
          || (stb_elem->aud_fade_out_sec > 0.0))
          {
            g_ascii_dtostr(&l_dbl_str[0]
                   ,G_ASCII_DTOSTR_BUF_SIZE
                   ,stb_elem->aud_volume_start
                   );
            fprintf(fp, "%s:%s "
                   , l_parnam_tab.parname[6], l_dbl_str
                   );

            g_ascii_dtostr(&l_dbl_str[0]
                   ,G_ASCII_DTOSTR_BUF_SIZE
                   ,stb_elem->aud_fade_in_sec
                   );
            fprintf(fp, "%s:%s "
                   , l_parnam_tab.parname[7], l_dbl_str
                   );

            g_ascii_dtostr(&l_dbl_str[0]
                   ,G_ASCII_DTOSTR_BUF_SIZE
                   ,stb_elem->aud_volume_end
                   );
            fprintf(fp, "%s:%s "
                   , l_parnam_tab.parname[8], l_dbl_str
                   );

            g_ascii_dtostr(&l_dbl_str[0]
                   ,G_ASCII_DTOSTR_BUF_SIZE
                   ,stb_elem->aud_fade_out_sec
                   );
               
            fprintf(fp, "%s:%s "
                   , l_parnam_tab.parname[9], l_dbl_str
                   );
          }

          fprintf(fp, "%s:%d "
               , l_parnam_tab.parname[10], (int)stb_elem->nloop
               );


          fprintf(fp, "\n");
        }
        break;

      default:
        p_story_save_comments(fp, stb_elem);
        /* keep unknown lines 1:1 as it was at read
         * for compatibility to STORYBOARD Level 2 files and future extensions
         */
        fprintf(fp, "%s\n", stb_elem->orig_src_line);
        break;
    }
  }
}  /* end p_story_save_audio_list */

/* ---------------------------------
 * p_story_save_flip_and_mask_params
 * ---------------------------------
 * print the optional flip and mask parameters
 * to file (only in case where specified and necessary)
 */
static void
p_story_save_flip_and_mask_params(FILE *fp, GapStoryElem *stb_elem
  , const char *parname_flip
  , const char *parname_mask_name
  , const char *parname_mask_anchor
  , const char *parname_mask_stepsize
  , const char *parname_mask_disable
  )
{
  p_story_save_flip_param(fp, stb_elem, parname_flip);

  if(stb_elem->mask_name)
  {
    fprintf(fp, " %s:\"%s\""
           , parname_mask_name
           , stb_elem->mask_name
           );
    
    switch(stb_elem->mask_anchor)
    {
      case GAP_MSK_ANCHOR_CLIP:
        fprintf(fp, " %s:clip"
               , parname_mask_anchor
               );
        break;    
      case GAP_MSK_ANCHOR_MASTER:
        fprintf(fp, " %s:master"
               , parname_mask_anchor
               );
        break;    
    }
    
    if(stb_elem->mask_stepsize != 1.0)
    {
      gchar l_dbl_str_mask_stepsize[G_ASCII_DTOSTR_BUF_SIZE];
      g_ascii_dtostr(&l_dbl_str_mask_stepsize[0]
                   ,G_ASCII_DTOSTR_BUF_SIZE
                   ,stb_elem->mask_stepsize
                   );
      fprintf(fp, " %s:%s"
               , parname_mask_stepsize
               , l_dbl_str_mask_stepsize
               );
    }

    if(stb_elem->mask_disable)
    {
       fprintf(fp, " %s:yes"
               , parname_mask_disable
               );
    }
    


  }

}  /* end p_story_save_flip_and_mask_params */


/* ------------------------------
 * p_story_save_video_list
 * ------------------------------
 */
static void
p_story_save_video_list(GapStoryBoard *stb, FILE *fp, gint32 save_track)
{
  static const gchar *l_playmode_normal = "normal";
  static const gchar *l_playmode_pingpong = "pingpong";
  GapStoryElem *stb_elem;
  GapStbSyntaxParnames l_parnam_tab;
  gboolean empty_track;


  empty_track = TRUE;
  for(stb_elem = stb->stb_elem; stb_elem != NULL; stb_elem = (GapStoryElem *)stb_elem->next)
  {
    gchar l_dbl_str_step_density[G_ASCII_DTOSTR_BUF_SIZE];
    
    const gchar *l_playmode_str;
    if(stb_elem->playmode == GAP_STB_PM_PINGPONG)
    {
      l_playmode_str = l_playmode_pingpong;
    }
    else
    {
      l_playmode_str = l_playmode_normal;
    }
    



    if((stb_elem->track != save_track)
    || (gap_story_elem_is_audio(stb_elem)))
    {
      continue;
    }
    else
    {
      if(empty_track)
      {
        /* add empty line at start of new track */
        fprintf(fp, "\n");
      }
      empty_track = FALSE;
    }
   

    /* setlocale independent float string */
    g_ascii_dtostr(&l_dbl_str_step_density[0]
                   ,G_ASCII_DTOSTR_BUF_SIZE
                   ,stb_elem->step_density
                   );

    switch(stb_elem->record_type)
    {
      case GAP_STBREC_VID_FRAMES:
        {
          p_story_save_comments(fp, stb_elem);
          
          gap_stb_syntax_get_parname_tab(GAP_STBKEY_VID_PLAY_FRAMES
                                  ,&l_parnam_tab
                                  );
          fprintf(fp, "%s     %s:%d %s:\"%s\" %s:%s %s:%06d %s:%06d %s:%s %s:%d %s:%s"
               , GAP_STBKEY_VID_PLAY_FRAMES
               , l_parnam_tab.parname[1], (int)stb_elem->track
               , l_parnam_tab.parname[2], stb_elem->basename
               , l_parnam_tab.parname[3], stb_elem->ext
               , l_parnam_tab.parname[4], (int)stb_elem->from_frame
               , l_parnam_tab.parname[5], (int)stb_elem->to_frame
               , l_parnam_tab.parname[6], l_playmode_str
               , l_parnam_tab.parname[7], (int)stb_elem->nloop
               , l_parnam_tab.parname[8], l_dbl_str_step_density
               );

          if(stb_elem->filtermacro_file)
          {
            fprintf(fp, " %s:\"%s\""
               , l_parnam_tab.parname[9]
               , stb_elem->filtermacro_file
               );
          }
          
          p_story_save_flip_and_mask_params(fp, stb_elem
                  , l_parnam_tab.parname[10]  /* parname_flip  */
                  , l_parnam_tab.parname[11]  /* parname_mask_name */
                  , l_parnam_tab.parname[12]  /* parname_mask_anchor */
                  , l_parnam_tab.parname[13]  /* parname_mask_stepsize */
                  , l_parnam_tab.parname[14]  /* parname_mask_disable */
                  );
          
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
               , l_parnam_tab.parname[1], (int)stb_elem->track
               , l_parnam_tab.parname[2], stb_elem->orig_filename
               , l_parnam_tab.parname[3], (int)stb_elem->nloop
               );
        if(stb_elem->filtermacro_file)
        {
          fprintf(fp, " %s:\"%s\""
               , l_parnam_tab.parname[4]
               , stb_elem->filtermacro_file
               );
        }

        p_story_save_flip_and_mask_params(fp, stb_elem
                  , l_parnam_tab.parname[5]  /* parname_flip  */
                  , l_parnam_tab.parname[6]  /* parname_mask_name */
                  , l_parnam_tab.parname[7]  /* parname_mask_anchor */
                  , l_parnam_tab.parname[8]  /* parname_mask_stepsize */
                  , l_parnam_tab.parname[9]  /* parname_mask_disable */
                  );
        fprintf(fp, "\n");
        if(gap_debug)
        {
          printf("\nSAVED stb_elem\n");
          gap_story_debug_print_elem(stb_elem);
        }
        break;
      case GAP_STBREC_VID_MOVIE:
        {
          gint   l_delace_int;
          
          gap_stb_syntax_get_parname_tab(GAP_STBKEY_VID_PLAY_MOVIE
                                  ,&l_parnam_tab
                                  );
          p_story_save_comments(fp, stb_elem);
          l_delace_int = (int)stb_elem->delace;
          fprintf(fp, "%s      %s:%d %s:\"%s\" %s:%06d %s:%06d %s:%s %s:%d "
                      "%s:%d %s:%d %s:%d.%d %s:%s"
               , GAP_STBKEY_VID_PLAY_MOVIE
               , l_parnam_tab.parname[1], (int)stb_elem->track
               , l_parnam_tab.parname[2], stb_elem->orig_filename
               , l_parnam_tab.parname[3], (int)stb_elem->from_frame
               , l_parnam_tab.parname[4], (int)stb_elem->to_frame
               , l_parnam_tab.parname[5], l_playmode_str
               , l_parnam_tab.parname[6], (int)stb_elem->nloop
               , l_parnam_tab.parname[7], (int)stb_elem->seltrack
               , l_parnam_tab.parname[8], (int)stb_elem->exact_seek
               , l_parnam_tab.parname[9], (int)l_delace_int
               , (int)((gdouble)(1000.0 * (stb_elem->delace - (gdouble)l_delace_int)))
               , l_parnam_tab.parname[10], l_dbl_str_step_density
               );

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
          p_story_save_flip_and_mask_params(fp, stb_elem
                  , l_parnam_tab.parname[13]  /* parname_flip  */
                  , l_parnam_tab.parname[14]  /* parname_mask_name */
                  , l_parnam_tab.parname[15]  /* parname_mask_anchor */
                  , l_parnam_tab.parname[16]  /* parname_mask_stepsize */
                  , l_parnam_tab.parname[17]  /* parname_mask_disable */
                  );
          fprintf(fp, "\n");
        }
        if(gap_debug)
        {
          printf("\nSAVED stb_elem\n");
          gap_story_debug_print_elem(stb_elem);
        }
        break;
      case GAP_STBREC_VID_ANIMIMAGE:
        {
          p_story_save_comments(fp, stb_elem);
          
          gap_stb_syntax_get_parname_tab(GAP_STBKEY_VID_PLAY_ANIMIMAGE
                                  ,&l_parnam_tab
                                  );
          fprintf(fp, "%s     %s:%d %s:\"%s\" %s:%06d %s:%06d %s:%s %s:%d %s:%s"
               , GAP_STBKEY_VID_PLAY_ANIMIMAGE
               , l_parnam_tab.parname[1], (int)stb_elem->track
               , l_parnam_tab.parname[2], stb_elem->orig_filename
               , l_parnam_tab.parname[3], (int)stb_elem->from_frame
               , l_parnam_tab.parname[4], (int)stb_elem->to_frame
               , l_parnam_tab.parname[5], l_playmode_str
               , l_parnam_tab.parname[6], (int)stb_elem->nloop
               , l_parnam_tab.parname[7], l_dbl_str_step_density
               );

          if(stb_elem->filtermacro_file)
          {
            fprintf(fp, " %s:\"%s\""
               , l_parnam_tab.parname[8]
               , stb_elem->filtermacro_file
               );
          }
          
          p_story_save_flip_and_mask_params(fp, stb_elem
                  , l_parnam_tab.parname[9]  /* parname_flip  */
                  , l_parnam_tab.parname[10]  /* parname_mask_name */
                  , l_parnam_tab.parname[11]  /* parname_mask_anchor */
                  , l_parnam_tab.parname[12]  /* parname_mask_stepsize */
                  , l_parnam_tab.parname[13]  /* parname_mask_disable */
                  );
          
          fprintf(fp, "\n");
        }
        if(gap_debug)
        {
          printf("\nSAVED stb_elem\n");
          gap_story_debug_print_elem(stb_elem);
        }
        break;


      case GAP_STBREC_ATT_TRANSITION:
        p_story_save_att_transition(fp, stb_elem);
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
}  /* end p_story_save_video_list */


/* ------------------------------
 * gap_story_save
 * ------------------------------
 */
gboolean
gap_story_save(GapStoryBoard *stb, const char *filename)
{
  FILE *fp;
  
  fp = g_fopen(filename, "w");
  if(fp)
  {
    gint32 track;
    
    p_story_save_header(stb, fp);
    p_story_save_mask_elem_list(stb, fp);
    
    for(track=1; track < GAP_STB_MAX_VID_TRACKS; track++)
    {
      if(track != GAP_STB_MASK_TRACK_NUMBER)
      {
        p_story_save_video_list(stb, fp, track);
      }
    }
    for(track=1; track < GAP_STB_MAX_AUD_TRACKS; track++)
    {
      p_story_save_audio_list(stb, fp, track);
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



/* ---------------------------------
 * p_story_local_wanted_framenr
 * ---------------------------------
 * precondition: the wanted_framenr is a local reference within
 * the specified stb_elem
 *   wanted_framenr <= stb_elem->nframes
 */
static void
p_story_local_wanted_framenr(GapStoryLocateRet *ret_elem, GapStoryElem *stb_elem, gint32 wanted_framenr)
{
  gint32  l_rangesize;
  gdouble fnr;

  if(wanted_framenr > stb_elem->nframes)
  {
    printf("INTERNAL ERROR: p_story_local_wanted_framenr wanted_framenr:%d overflow (limit:%d)\n"
          ,(int)wanted_framenr
          ,(int)stb_elem->nframes
          );
    return;
  }

  l_rangesize = p_calculate_rangesize(stb_elem);

  /* figure out the local framenr
   *  (respecting normal|invers order, nloop count and normal|pingpong playmode
   *   and step_density setting)
   */
  if (stb_elem->to_frame >= stb_elem->from_frame)
  {
    /* normal range */
    if (stb_elem->playmode == GAP_STB_PM_PINGPONG)
    {
      ret_elem->ret_framenr = p_calculate_pingpong_framenr(wanted_framenr
                                                          ,l_rangesize
                                                          ,stb_elem->step_density
                                                          );
    }
    else
    {
      fnr = (gdouble)((wanted_framenr -1) % l_rangesize) * stb_elem->step_density;
      ret_elem->ret_framenr = 1 + (gint32)fnr;
    }
    ret_elem->ret_framenr += (stb_elem->from_frame -1); /* add rangeoffset */
  }
  else
  {
    /* inverse range */
    if (stb_elem->playmode == GAP_STB_PM_PINGPONG)
    {
      ret_elem->ret_framenr = 1 + l_rangesize - p_calculate_pingpong_framenr(wanted_framenr
                                                                            ,l_rangesize
                                                                            ,stb_elem->step_density
                                                                            );
    }
    else
    {
      fnr = (gdouble)((l_rangesize - wanted_framenr) % l_rangesize) * stb_elem->step_density;
      ret_elem->ret_framenr = 1 + (gint32)fnr;
    }
    ret_elem->ret_framenr += (stb_elem->to_frame -1);  /* add rangeoffset */
  }
  ret_elem->locate_ok = TRUE;
  ret_elem->stb_elem = stb_elem;

}  /* end p_story_local_wanted_framenr */


 
/* ---------------------------------
 * gap_story_locate_expanded_framenr
 * ---------------------------------
 * IMPOTANT: this procedure operates on expanded frame numbers.
 * it does not respect overlapping scenarios.
 *
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
gap_story_locate_expanded_framenr(GapStoryBoard *stb, gint32 in_framenr, gint32 in_track)
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
      /* the wanted in_framenr is within the range of the current stb_elem
       * now figure out the local framenr
       *  (respecting normal|invers order, nloop count and normal|pingpong playmode
       *   and step_density setting)
       */
      p_story_local_wanted_framenr(ret_elem, stb_elem, l_framenr);
      break;
    }
  }
  
  return(ret_elem);
  
}  /* end gap_story_locate_expanded_framenr */


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
 * NOTE: in case of overlapping within the specified track
 *       the overlapping frames 
 *       (that will be placed in the shadow track at storyboard render processing)
 *       can NOT be accessed with this procedure.
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

    if(stb_elem->record_type == GAP_STBREC_ATT_TRANSITION)
    {
      /* expand the wanted target framenr by adding the number of overlapping frames
       * this results in skipping the overlapped part
       */
      l_framenr += stb_elem->att_overlap;
    }
  
    if (l_framenr > stb_elem->nframes)
    {
      /* the wanted in_framenr is not within the range of the current stb_elem */
      l_framenr -= stb_elem->nframes;
    }
    else
    {
      /* the wanted in_framenr is within the range of the current stb_elem
       * now figure out the local framenr
       *  (respecting normal|invers order, nloop count and normal|pingpong playmode
       *   and step_density setting)
       */
      p_story_local_wanted_framenr(ret_elem, stb_elem, l_framenr);
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
 *
 * NOTES:
 * - overlapping frames are respected (and not counted)
 */
gint32
gap_story_get_framenr_by_story_id(GapStoryBoard *stb, gint32 story_id, gint32 in_track)
{
  GapStoryElem      *stb_elem;
  gint32             l_framenr;
  gint32             l_framecount;
  gint32             l_overlap_request;

  l_overlap_request = 0;
  l_framecount = 0;

  for(stb_elem = stb->stb_elem; stb_elem != NULL;  stb_elem = stb_elem->next)
  {
    gint32             l_overlap_new;
    gint32             l_overlap_actual;

    if(stb_elem->track != in_track)
    {
      continue;
    }
    if(gap_story_elem_is_video_relevant(stb_elem) != TRUE)
    {
      continue;
    }
    if(stb_elem->story_id == story_id)
    {
      break;
    }

    l_overlap_new = p_story_elem_calculate_nframes(stb_elem);
    l_overlap_request += l_overlap_new;
    l_framecount = l_framecount + stb_elem->nframes;
    l_overlap_actual = MIN(MAX(0, l_framecount), l_overlap_request);
    
    l_framecount -= l_overlap_actual;
    l_overlap_request -= l_overlap_actual;
  }
  l_framenr = l_framecount + 1;
  return(l_framenr);
  
}  /* end gap_story_get_framenr_by_story_id */ 

 
/* ------------------------------------------
 * gap_story_get_expanded_framenr_by_story_id
 * ------------------------------------------
 * return framnumber of the 1.st frame of stb_elem with story_id
 * for the 1.st stb_elem in the list, 1 is returned.
 * NOTE: this procedure delivers the expanded framenumber because
 * it ignores overlapping within a track.
 */
gint32
gap_story_get_expanded_framenr_by_story_id(GapStoryBoard *stb, gint32 story_id, gint32 in_track)
{
  GapStoryElem      *stb_elem;
  gint32             l_framenr;
  gint32             l_framecount;

  l_framecount = 0;
  
  for(stb_elem = stb->stb_elem; stb_elem != NULL;  stb_elem = stb_elem->next)
  {
    if(stb_elem->track != in_track)
    {
      continue;
    }
    if(gap_story_elem_is_video_relevant(stb_elem) != TRUE)
    {
      continue;
    }
    if(stb_elem->story_id == story_id)
    {
      break;
    }
    p_story_elem_calculate_nframes(stb_elem);
    l_framecount = l_framecount + stb_elem->nframes;
  }
  l_framenr = l_framecount + 1;
  return(l_framenr);
  
}  /* end gap_story_get_expanded_framenr_by_story_id */ 

/* ------------------------------
 * gap_story_get_master_pixelsize
 * ------------------------------
 * deliver master_width and master_height
 *
 * if no master size information is present
 * in the storyboard file
 * then deliver size of the 1.st frame of track 1
 * that will be fetched to peek the size.
 */
void
gap_story_get_master_pixelsize(GapStoryBoard *stb
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
    *width = MAX(stb->master_width, 720);
    *height = MAX(stb->master_height, 576);
  }
  
  
}  /* end gap_story_get_master_pixelsize */


/* ------------------------------------------
 * gap_story_get_master_size_respecting_aspect
 * ------------------------------------------
 * deliver master_width and master_height
 * if the storyboard forces a specified aspect ratio (4:3 or 16:9)
 * then adjust the height to match this aspect ratio.
 *
 * if no master size information is present
 * in the storyboard file
 * then deliver size of the 1.st frame of track 1
 * that will be fetched to peek the size.
 */
void
gap_story_get_master_size_respecting_aspect(GapStoryBoard *stb
                         ,gint32 *width
                         ,gint32 *height)
{
  gap_story_get_master_pixelsize(stb, width, height);

  if(stb->master_aspect_height > 0)
  {
    stb->master_aspect_ratio = (gdouble)stb->master_aspect_width
                             / (gdouble)stb->master_aspect_height;
  }

  if(stb->master_aspect_ratio > 0.0)
  {
    gdouble asp_height;
    
    asp_height = (gdouble)(*width) / stb->master_aspect_ratio;
    *height = (gint32)asp_height;
  }
}  /* end gap_story_get_master_size_respecting_aspect */


/* ---------------------------------
 * p_story_count_frames_in_track
 * ---------------------------------
 * return totoal number of frames in the specified track
 *
 * NOTES:
 * - overlapping frames are respected (and not counted)
 */
gint32
p_story_count_frames_in_track(GapStoryBoard *stb, gint32 in_track)
{
  GapStoryElem      *stb_elem;
  gint32             l_framecount;
  gint32             l_rest_frames;
  gint32             l_overlap_request;

  l_overlap_request = 0;
  l_framecount = 0;

  for(stb_elem = stb->stb_elem; stb_elem != NULL;  stb_elem = stb_elem->next)
  {
    gint32             l_overlap_handled_in_this_clip;

    if(stb_elem->track != in_track)
    {
      continue;
    }
    if(gap_story_elem_is_video_relevant(stb_elem) != TRUE)
    {
      continue;
    }

    l_overlap_request += p_story_elem_calculate_nframes(stb_elem);

    l_overlap_handled_in_this_clip = MIN(l_overlap_request, stb_elem->nframes);
    l_rest_frames = stb_elem->nframes - l_overlap_handled_in_this_clip;
    
 
    l_framecount += l_rest_frames;
    l_overlap_request -= l_overlap_handled_in_this_clip;
  }

  if(gap_debug)
  {
    printf("p_story_count_frames_in_track END: in_track:%d l_framecount:%d\n"
      ,(int)in_track
      ,(int)l_framecount
      );
  }
  
  return(l_framecount);
  
}  /* end p_story_count_frames_in_track */ 

/* -----------------------------
 * gap_story_fake_ainfo_from_stb
 * -----------------------------
 * returned a faked GapAnimInfo structure
 * that tells about the total frames in the storyboard file in the specified in_track.
 * (if in_track is < 1 count total frames of the longest video track)
 *
 * all char pointers are set to NULL
 * the caller is responsible to free the returned struct.
 */
GapAnimInfo * 
gap_story_fake_ainfo_from_stb(GapStoryBoard *stb, gint32 in_track)
{
  GapAnimInfo   *l_ainfo_ptr;
  gint32         l_max_framecount;

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

    gap_story_get_master_size_respecting_aspect(stb
                               ,&l_width
                               ,&l_height
                               );
    l_ainfo_ptr->width = l_width;
    l_ainfo_ptr->height = l_height;

    l_max_framecount = 0;
    if(in_track < 0)
    {
      gint32 l_framecount;
      gint32 l_track;
      for(l_track=1; l_track <= GAP_STB_MAX_VID_TRACKS; l_track++)
      {
        if(l_track != GAP_STB_MASK_TRACK_NUMBER)
        {
          l_framecount = p_story_count_frames_in_track(stb, l_track); 
          l_max_framecount = MAX(l_framecount, l_max_framecount);
        }
      }
    }
    else
    {
      l_max_framecount = p_story_count_frames_in_track(stb ,in_track); 
    }

    l_ainfo_ptr->curr_frame_nr = 1;
    l_ainfo_ptr->first_frame_nr = 1;
    l_ainfo_ptr->frame_cnt = l_max_framecount;
    l_ainfo_ptr->last_frame_nr = l_max_framecount;

    if(gap_debug)
    {
      printf("gap_story_fake_ainfo_from_stb: RES l_max_framecount:%d\n"
            , (int)l_max_framecount
            );
    }
    
    
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

    stb_elem_dup->vid_wait_untiltime_sec  = stb_elem->vid_wait_untiltime_sec;  
    stb_elem_dup->color_red               = stb_elem->color_red;  
    stb_elem_dup->color_green             = stb_elem->color_green;  
    stb_elem_dup->color_blue              = stb_elem->color_blue;  
    stb_elem_dup->color_alpha             = stb_elem->color_alpha;  
    stb_elem_dup->att_keep_proportions    = stb_elem->att_keep_proportions;  
    stb_elem_dup->att_fit_width           = stb_elem->att_fit_width;  
    stb_elem_dup->att_fit_height          = stb_elem->att_fit_height;  

    /* copy all transition attributes */
    {
      gint ii;
      for(ii=0; ii < GAP_STB_ATT_TYPES_ARRAY_MAX; ii++)
      {
        stb_elem_dup->att_arr_enable[ii]     = stb_elem->att_arr_enable[ii];
        stb_elem_dup->att_arr_value_from[ii] = stb_elem->att_arr_value_from[ii];
        stb_elem_dup->att_arr_value_to[ii]   = stb_elem->att_arr_value_to[ii];
        stb_elem_dup->att_arr_value_dur[ii]  = stb_elem->att_arr_value_dur[ii];
      }
    }
    stb_elem_dup->flip_request            = stb_elem->flip_request;  
    stb_elem_dup->att_overlap             = stb_elem->att_overlap;  
    if(stb_elem->mask_name)               stb_elem_dup->mask_name = g_strdup(stb_elem->mask_name);
    stb_elem_dup->mask_anchor             = stb_elem->mask_anchor;  
    stb_elem_dup->mask_stepsize           = stb_elem->mask_stepsize;  
    stb_elem_dup->mask_disable            = stb_elem->mask_disable;  


    if(stb_elem->aud_filename)            stb_elem_dup->aud_filename = g_strdup(stb_elem->aud_filename);
    stb_elem_dup->aud_seltrack            = stb_elem->aud_seltrack;  
    stb_elem_dup->aud_wait_untiltime_sec  = stb_elem->aud_wait_untiltime_sec;  
    stb_elem_dup->aud_play_from_sec       = stb_elem->aud_play_from_sec;  
    stb_elem_dup->aud_play_to_sec         = stb_elem->aud_play_to_sec;  
    stb_elem_dup->aud_volume_start        = stb_elem->aud_volume_start;  
    stb_elem_dup->aud_volume              = stb_elem->aud_volume;  
    stb_elem_dup->aud_volume_end          = stb_elem->aud_volume_end;  
    stb_elem_dup->aud_fade_in_sec         = stb_elem->aud_fade_in_sec;  
    stb_elem_dup->aud_fade_out_sec        = stb_elem->aud_fade_out_sec;  
    stb_elem_dup->aud_min_play_sec        = stb_elem->aud_min_play_sec;  
    stb_elem_dup->aud_max_play_sec        = stb_elem->aud_max_play_sec;  
    stb_elem_dup->aud_framerate           = stb_elem->aud_framerate;  

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
    if(stb_elem_dst->ext)               g_free(stb_elem_dst->ext);
    if(stb_elem_dst->filtermacro_file)  g_free(stb_elem_dst->filtermacro_file);
    if(stb_elem_dst->preferred_decoder) g_free(stb_elem_dst->preferred_decoder);
    if(stb_elem_dst->mask_name)         g_free(stb_elem_dst->mask_name);
    if(stb_elem_dst->aud_filename)      g_free(stb_elem_dst->aud_filename);


    stb_elem_dst->orig_filename     = NULL;
    stb_elem_dst->orig_src_line     = NULL;
    stb_elem_dst->basename          = NULL;
    stb_elem_dst->ext               = NULL;
    stb_elem_dst->filtermacro_file  = NULL;
    stb_elem_dst->preferred_decoder = NULL;
    stb_elem_dst->mask_name = NULL;
    stb_elem_dst->aud_filename = NULL;

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
    if(stb_elem->mask_name)         stb_elem_dst->mask_name         = g_strdup(stb_elem->mask_name);
    if(stb_elem->aud_filename)      stb_elem_dst->aud_filename      = g_strdup(stb_elem->aud_filename);

    stb_elem_dst->seltrack        = stb_elem->seltrack;
    stb_elem_dst->exact_seek      = stb_elem->exact_seek;
    stb_elem_dst->delace          = stb_elem->delace;
    stb_elem_dst->from_frame      = stb_elem->from_frame;
    stb_elem_dst->to_frame        = stb_elem->to_frame;
    stb_elem_dst->nloop           = stb_elem->nloop;
    stb_elem_dst->nframes         = stb_elem->nframes; 
    stb_elem_dst->step_density    = stb_elem->step_density; 
    stb_elem_dst->file_line_nr    = stb_elem->file_line_nr;  


    stb_elem_dst->vid_wait_untiltime_sec  = stb_elem->vid_wait_untiltime_sec;  
    stb_elem_dst->color_red               = stb_elem->color_red;  
    stb_elem_dst->color_green             = stb_elem->color_green;  
    stb_elem_dst->color_blue              = stb_elem->color_blue;  
    stb_elem_dst->color_alpha             = stb_elem->color_alpha;  
    stb_elem_dst->att_keep_proportions    = stb_elem->att_keep_proportions;  
    stb_elem_dst->att_fit_width           = stb_elem->att_fit_width;  
    stb_elem_dst->att_fit_height          = stb_elem->att_fit_height;  

    /* copy all transition attributes */
    {
      gint ii;
      for(ii=0; ii < GAP_STB_ATT_TYPES_ARRAY_MAX; ii++)
      {
        stb_elem_dst->att_arr_enable[ii]     = stb_elem->att_arr_enable[ii];
        stb_elem_dst->att_arr_value_from[ii] = stb_elem->att_arr_value_from[ii];
        stb_elem_dst->att_arr_value_to[ii]   = stb_elem->att_arr_value_to[ii];
        stb_elem_dst->att_arr_value_dur[ii]  = stb_elem->att_arr_value_dur[ii];
      }
    }

    stb_elem_dst->flip_request            = stb_elem->flip_request;  
    stb_elem_dst->att_overlap             = stb_elem->att_overlap;
 
    stb_elem_dst->mask_anchor             = stb_elem->mask_anchor;  
    stb_elem_dst->mask_stepsize           = stb_elem->mask_stepsize;  
    stb_elem_dst->mask_disable            = stb_elem->mask_disable;  



    stb_elem_dst->aud_seltrack            = stb_elem->aud_seltrack;  
    stb_elem_dst->aud_wait_untiltime_sec  = stb_elem->aud_wait_untiltime_sec;  
    stb_elem_dst->aud_play_from_sec       = stb_elem->aud_play_from_sec;  
    stb_elem_dst->aud_play_to_sec         = stb_elem->aud_play_to_sec;  
    stb_elem_dst->aud_volume_start        = stb_elem->aud_volume_start;  
    stb_elem_dst->aud_volume              = stb_elem->aud_volume;  
    stb_elem_dst->aud_volume_end          = stb_elem->aud_volume_end;  
    stb_elem_dst->aud_fade_in_sec         = stb_elem->aud_fade_in_sec;  
    stb_elem_dst->aud_fade_out_sec        = stb_elem->aud_fade_out_sec;  
    stb_elem_dst->aud_min_play_sec        = stb_elem->aud_min_play_sec;  
    stb_elem_dst->aud_max_play_sec        = stb_elem->aud_max_play_sec;  
    stb_elem_dst->aud_framerate           = stb_elem->aud_framerate;  



    if(stb_elem_dst->comment)       gap_story_elem_free(&stb_elem_dst->comment);
    if(stb_elem->comment)           stb_elem_dst->comment = gap_story_elem_duplicate(stb_elem->comment);

    /* stb_elem_dst->next */ /* dont change the next pointer */

  }
}  /* end gap_story_elem_copy */


/* ------------------------------------------------
 * gap_story_find_mask_definition_by_name
 * ------------------------------------------------
 */
GapStoryElem *
gap_story_find_mask_definition_by_name(GapStoryBoard *stb_ptr, const char *mask_name)
{
  GapStoryElem      *stb_elem;
  
  for(stb_elem = stb_ptr->stb_elem; stb_elem != NULL;  stb_elem = stb_elem->next)
  {
    if((stb_elem->track == GAP_STB_MASK_TRACK_NUMBER)
    && (stb_elem->mask_name != NULL))
    {
      if(strcmp(stb_elem->mask_name, mask_name) == 0)
      {
        return(stb_elem);
      }
    }
  }
  
  return (NULL);
}  /* end gap_story_find_mask_definition_by_name */


/* ------------------------------------------------
 * gap_story_find_mask_reference_by_name
 * ------------------------------------------------
 * returns the 1.st element that rfers to the specified mas_name
 */
GapStoryElem *
gap_story_find_mask_reference_by_name(GapStoryBoard *stb_ptr, const char *mask_name)
{
  GapStoryElem      *stb_elem;
  
  for(stb_elem = stb_ptr->stb_elem; stb_elem != NULL;  stb_elem = stb_elem->next)
  {
    if((stb_elem->track != GAP_STB_MASK_TRACK_NUMBER)
    && (stb_elem->mask_name != NULL))
    {
      if(strcmp(stb_elem->mask_name, mask_name) == 0)
      {
        return(stb_elem);
      }
    }
  }
  
  return (NULL);
}  /* end gap_story_find_mask_reference_by_name */


/* ------------------------------------------------
 * p_story_board_duplicate_refered_mask_definitions
 * ------------------------------------------------
 */
static void
p_story_board_duplicate_refered_mask_definitions(GapStoryBoard *stb_dest
    ,GapStoryBoard *stb_src
    )
{
  GapStoryElem      *stb_elem_dup;
  GapStoryElem      *stb_elem;
  
  if(gap_debug)
  {
    printf("@@@@@ p_story_board_duplicate_refered_mask_definitions\n");
    gap_story_debug_print_list(stb_src);
  }
  
  for(stb_elem = stb_src->stb_elem; stb_elem != NULL;  stb_elem = stb_elem->next)
  {
    if((stb_elem->track == GAP_STB_MASK_TRACK_NUMBER)
    && (stb_elem->mask_name != NULL))
    {
      GapStoryElem      *stb_elem_ref;
      
      stb_elem_ref = gap_story_find_mask_reference_by_name(stb_dest, stb_elem->mask_name);
      if(stb_elem_ref)
      {
        /* copy the element to the destination storyboard
         * beacause it is a mask definition
         * that is refered in the destination storyboard.
         */
        stb_elem_dup = gap_story_elem_duplicate(stb_elem);
        if(stb_elem_dup)
        {
          stb_elem_dup->track = GAP_STB_HIDDEN_MASK_TRACK_NUMBER;
          gap_story_list_append_elem(stb_dest, stb_elem_dup);
        }
      }
      
    }
  }

}  /* end p_story_board_duplicate_refered_mask_definitions */



/* -----------------------------
 * p_story_board_duplicate
 * -----------------------------
 * make a duplicate of the storyboard and its list of elements.
 * (but without the comment sublists)
 *
 * the input stb_ptr is not copied if there are errors
 *   in this case NULL is returned.
 *
 * IN: copy_all   FALSE: copy only selcted elements
 * IN: in_vtrack   -1 copy all video tracks, 1..n copy only selected video track
 * IN: in_atrack   -1 copy all audio tracks, 1..n copy only selected audio track
 *
 * NOTE: mask definitions (elements of the track GAP_STB_MASK_TRACK_NUMBER)
 * are automatically included in the duplicate when 
 * the copied video elements refere to them.
 */
static GapStoryBoard *     
p_story_board_duplicate(GapStoryBoard *stb_ptr, gboolean copy_all_all
  , gint32 in_vtrack
  , gint32 in_atrack)
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
  stb_dup->master_aspect_ratio = stb_ptr->master_aspect_ratio;
  stb_dup->master_aspect_width = stb_ptr->master_aspect_width;
  stb_dup->master_aspect_height = stb_ptr->master_aspect_height;

  stb_dup->master_samplerate = stb_ptr->master_samplerate;
  stb_dup->master_volume = stb_ptr->master_volume;
  if(stb_ptr->preferred_decoder)
  {
    stb_dup->preferred_decoder = g_strdup(stb_ptr->preferred_decoder);
  }

  stb_dup->layout_cols = stb_ptr->layout_cols;
  stb_dup->layout_rows = stb_ptr->layout_rows;
  stb_dup->layout_thumbsize = stb_ptr->layout_thumbsize;

  stb_dup->stb_parttype  = 1;     /* duplicate is just the selected part */
  if(copy_all_all)
  {
     stb_dup->stb_parttype  = 0;  /* duplicate is a full copy */
  }
  stb_dup->stb_unique_id  = global_stb_id++;  /* duplicate has its own id */

  stb_dup->unsaved_changes = TRUE;

  
  for(stb_elem = stb_ptr->stb_elem; stb_elem != NULL;  stb_elem = stb_elem->next)
  {
    gboolean trackselect_flag;
    gboolean dup_flag;
    
    trackselect_flag = FALSE;
    dup_flag = FALSE;
    
    if(gap_story_elem_is_audio(stb_elem))
    {
      if((stb_elem->track == in_atrack)
      || (in_atrack < 0))
      {
        trackselect_flag = TRUE;
      }
    }
    else
    {
      if((stb_elem->track == in_vtrack)
      || (in_vtrack < 0))
      {
        trackselect_flag = TRUE;
      }
    }
    
    if(trackselect_flag)
    {
      if((stb_elem->selected)
      || (copy_all_all))
      {
        dup_flag = TRUE;
      }
    }

    if(dup_flag)
    {
        stb_elem_dup = gap_story_elem_duplicate(stb_elem);
        if(stb_elem_dup)
        {
          gap_story_list_append_elem(stb_dup, stb_elem_dup);
        }
    }
  }
  
  /* copy corresponding mask definitions.
   * this is not required if the requested in_vtrack refers to
   * the mask definition track. Or if the copy already included
   * all video tracks.
   */
  if((in_vtrack != GAP_STB_MASK_TRACK_NUMBER)
  &&(in_vtrack > 0))
  {
    p_story_board_duplicate_refered_mask_definitions(stb_dup, stb_ptr);
  }

  if(gap_debug)
  {
    printf("\np_story_board_duplicate: dump of the DUP List copy_all:%d in_atrack;%d in_vtrack:%d\n"
      ,(int)copy_all_all
      ,(int)in_atrack
      ,(int)in_vtrack
      );
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
  return(p_story_board_duplicate(stb_ptr, TRUE, -1, -1));
}  /* end gap_story_duplicate  */


/* ---------------------------------------
 * gap_story_enable_hidden_maskdefinitions
 * ---------------------------------------
 * the input stb_ptr is not copied if there are errors
 *   in this case NULL is returned.
 *
 * make a duplicate of the list restricted to specified video track.
 *   (without the comment sublists, and without any audio tracks)
 *   
 */
void    
gap_story_enable_hidden_maskdefinitions(GapStoryBoard *stb_ptr)
{
  GapStoryElem  *stb_elem;

  for(stb_elem = stb_ptr->stb_elem; stb_elem != NULL;  stb_elem = stb_elem->next)
  {
    if(stb_elem->track == GAP_STB_HIDDEN_MASK_TRACK_NUMBER)
    {
      stb_elem->track = GAP_STB_MASK_TRACK_NUMBER;
    }
  }
}  /* end gap_story_enable_hidden_maskdefinitions  */

/* -----------------------------
 * gap_story_duplicate_vtrack
 * -----------------------------
 * the input stb_ptr is not copied if there are errors
 *   in this case NULL is returned.
 *
 * make a duplicate of the list restricted to specified video track.
 *   (without the comment sublists, and without any audio tracks)
 *   
 */
GapStoryBoard *     
gap_story_duplicate_vtrack(GapStoryBoard *stb_ptr, gint32 in_vtrack)
{
  GapStoryBoard *stb_dup;

  stb_dup = p_story_board_duplicate(stb_ptr, TRUE, in_vtrack, 0);
  gap_story_enable_hidden_maskdefinitions(stb_dup);

  return(stb_dup);
}  /* end gap_story_duplicate_vtrack  */

/* -----------------------------
 * gap_story_duplicate_sel_only
 * -----------------------------
 * the input stb_ptr is not copied if there are errors
 *   in this case NULL is returned.
 *
 * make a filtered duplicate of the list that contains only elements of the specified track
 * with the selected flag set.
 * (use -1 to specify all tracks, including the mask definition track)
 *
 * the duplicated list will contain mask definition elements in the hidden
 * mask track if selected elements have mask references and in_vtrack > 0 was specified.
 * 
 * (without the comment sublists)
 */
GapStoryBoard *     
gap_story_duplicate_sel_only(GapStoryBoard *stb_ptr, gint32 in_vtrack)
{
  return(p_story_board_duplicate(stb_ptr, FALSE, in_vtrack, -1));
}  /* end gap_story_duplicate_sel_only  */

/* -----------------------------------------------
 * gap_story_set_properties_like_sample_storyboard
 * -----------------------------------------------
 */
void
gap_story_set_properties_like_sample_storyboard (GapStoryBoard *stb
  , GapStoryBoard *stb_sample)
{
  if(stb)
  {
    if(stb_sample)
    {
      stb->master_width       = stb_sample->master_width;
      stb->master_height      = stb_sample->master_height;
      stb->master_framerate   = stb_sample->master_framerate;

      stb->master_aspect_ratio   = stb_sample->master_aspect_ratio;
      stb->master_aspect_width   = stb_sample->master_aspect_width;
      stb->master_aspect_height  = stb_sample->master_aspect_height;

      stb->master_samplerate  = stb_sample->master_samplerate;
      stb->master_volume      = stb_sample->master_volume;
      stb->unsaved_changes = TRUE;
      if(stb_sample->preferred_decoder)
      {
        stb->preferred_decoder  = g_strdup(stb_sample->preferred_decoder);
      }
    }
  }
}  /* end gap_story_set_properties_like_sample_storyboard */



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


/* -------------------------------
 * gap_story_is_active_element
 * -------------------------------
 */
gboolean
gap_story_is_active_element(GapStoryElem *stb_elem)
{
  if((stb_elem->record_type == GAP_STBREC_VID_SILENCE)
  || (stb_elem->record_type == GAP_STBREC_VID_COLOR)
  || (stb_elem->record_type == GAP_STBREC_VID_IMAGE)
  || (stb_elem->record_type == GAP_STBREC_VID_ANIMIMAGE)
  || (stb_elem->record_type == GAP_STBREC_VID_FRAMES)
  || (stb_elem->record_type == GAP_STBREC_VID_MOVIE)
  || (stb_elem->record_type == GAP_STBREC_ATT_TRANSITION)
  )
  {
    return (TRUE);
  }
  
  return (FALSE);

}  /* end gap_story_is_active_element */

/* -------------------------------
 * gap_story_count_active_elements
 * -------------------------------
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
      if(gap_story_is_active_element(stb_elem))
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
      if(gap_story_is_active_element(stb_elem))
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
 * gap_story_set_vtrack
 * ---------------------------------
 * set selection state in all elements to specified sel_state
 */
void
gap_story_set_vtrack(GapStoryBoard *stb, gint32 vtrack)
{
  GapStoryElem *stb_elem;
  
  if(stb  == NULL)   { return; }
  
  for(stb_elem = stb->stb_elem; stb_elem != NULL;  stb_elem = stb_elem->next)
  {
    if(gap_story_elem_is_video_relevant(stb_elem))
    {
      stb_elem->track = vtrack;
    }
  }

}  /* end gap_story_set_vtrack */


/* ---------------------------------
 * gap_story_selection_all_set
 * ---------------------------------
 * set selection state in all elements to specified sel_state
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
 * gap_story_selection_by_story_id
 * ---------------------------------
 */
void
gap_story_selection_by_story_id(GapStoryBoard *stb, gboolean sel_state, gint32 story_id)
{
  GapStoryElem *stb_elem;
  
  if(stb == NULL)   { return; }
  
  for(stb_elem = stb->stb_elem; stb_elem != NULL;  stb_elem = stb_elem->next)
  {
    if(stb_elem->story_id == story_id)
    {
      stb_elem->selected = sel_state;
      return;
    }
  }

}  /* end gap_story_selection_by_story_id */


/* ------------------------------------------
 * gap_story_selection_from_ref_list_orig_ids
 * ------------------------------------------
 * set selection for all elements with story_id matching to orig_story_id 
 * of at least one element in the 2.dn storyboard (specified via stb_ref)
 */
void
gap_story_selection_from_ref_list_orig_ids(GapStoryBoard *stb, gboolean sel_state, GapStoryBoard *stb_ref)
{
  GapStoryElem *stb_elem;
  
  if((stb == NULL) || (stb_ref == NULL))  { return; }
  
  for(stb_elem = stb_ref->stb_elem; stb_elem != NULL;  stb_elem = stb_elem->next)
  {
    gap_story_selection_by_story_id(stb, sel_state, stb_elem->story_orig_id);
  }

}  /* end gap_story_selection_from_ref_list_orig_ids */


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


/* --------------------------------
 * gap_story_set_aud_movie_min_max
 * --------------------------------
 * analyze the storyboard elements from type GAP_STBREC_AUD_MOVIE
 * and set the aud_min_play_sec and aud_max_play_sec information
 * if the clip is the only reference to a videofile
 * these values are equal to aud_play_from_sec, aud_play_to_sec
 * if there are more references the min/max of all references is set
 * (in all refrencing GAP_STBREC_AUD_MOVIE elements)
 *
 * this information is provided for optimized audio extraction
 * at storyboard audio processing.
 */
void
gap_story_set_aud_movie_min_max(GapStoryBoard *stb)
{
  GapStoryElem *stb_element;
  GapStoryElem *stb_elem;

  /* init all min/max values with negative value */
  for(stb_elem = stb->stb_elem; stb_elem != NULL;  stb_elem = stb_elem->next)
  {
    stb_elem->aud_min_play_sec = -1.0;
    stb_elem->aud_max_play_sec = -1.0;
  }


  /* Loop foreach Element in the STB */
  for(stb_element = stb->stb_elem; stb_element != NULL;  stb_element = stb_element->next)
  {
    if((stb_element->record_type == GAP_STBREC_AUD_MOVIE)
    && (stb_element->aud_min_play_sec < 0.0))
    {
      gdouble min_play_sec;
      gdouble max_play_sec;
      
      min_play_sec = stb_element->aud_play_from_sec;
      max_play_sec = stb_element->aud_play_to_sec;

      /* search for more references to same audiotrack in the same videofile */
      for(stb_elem = stb_element->next; stb_elem != NULL;  stb_elem = stb_elem->next)
      {
        if((stb_elem->aud_filename)
        && (stb_element->aud_filename)
        && (stb_elem->record_type  == stb_element->record_type)
        && (stb_elem->aud_seltrack == stb_element->aud_seltrack))
        {
          if(strcmp(stb_elem->aud_filename, stb_element->aud_filename) == 0)
          {
            min_play_sec = MIN(min_play_sec, stb_elem->aud_play_from_sec);
            max_play_sec = MAX(max_play_sec, stb_elem->aud_play_to_sec);
          }
        }
      }
      
      /* set min/max values in all references */
      for(stb_elem = stb_element; stb_elem != NULL;  stb_elem = stb_elem->next)
      {
        if((stb_elem->aud_filename)
        && (stb_element->aud_filename)
        && (stb_elem->record_type  == stb_element->record_type)
        && (stb_elem->aud_seltrack == stb_element->aud_seltrack))
        {
          if(strcmp(stb_elem->aud_filename, stb_element->aud_filename) == 0)
          {
            stb_elem->aud_min_play_sec = min_play_sec;
            stb_elem->aud_max_play_sec = max_play_sec;
          }
        }
      }

    }
  }

}  /* end gap_story_set_aud_movie_min_max */


/* --------------------------
 * gap_story_elem_is_audio
 * --------------------------
 */
gboolean
gap_story_elem_is_audio(GapStoryElem *stb_elem)
{
  switch(stb_elem->record_type)
  {
    case GAP_STBREC_AUD_SILENCE:
    case GAP_STBREC_AUD_SOUND:
    case GAP_STBREC_AUD_MOVIE:
      return (TRUE);
      break;
    default:
      return (FALSE);
  }
  
  return (FALSE);
}  /* end gap_story_elem_is_audio */


/* --------------------------
 * gap_story_elem_is_video
 * --------------------------
 */
gboolean
gap_story_elem_is_video(GapStoryElem *stb_elem)
{
  switch(stb_elem->record_type)
  {
    case GAP_STBREC_VID_SILENCE:
    case GAP_STBREC_VID_COLOR:
    case GAP_STBREC_VID_IMAGE:
    case GAP_STBREC_VID_ANIMIMAGE:
    case GAP_STBREC_VID_FRAMES:
    case GAP_STBREC_VID_MOVIE:
      return (TRUE);
      break;
    default:
      return (FALSE);
  }
  
  return (FALSE);
}  /* end gap_story_elem_is_video */


/* --------------------------------
 * gap_story_elem_is_video_relevant
 * --------------------------------
 * check if the specified element is relevant
 * for calculating video frames.
 * currently all video elements and the transition attribute (due to overlap)
 * are relevant elements.
 */
gboolean
gap_story_elem_is_video_relevant(GapStoryElem *stb_elem)
{
  if(stb_elem->record_type == GAP_STBREC_ATT_TRANSITION)
  {
    return (TRUE);
  }
  return (gap_story_elem_is_video(stb_elem));
  
}  /* end gap_story_elem_is_video_relevant */


/* --------------------------
 * gap_story_del_audio_track
 * --------------------------
 * delete all elements of the specified audio track
 */
void
gap_story_del_audio_track(GapStoryBoard *stb
                         ,gint aud_track
                         )
{
  GapStoryElem *stb_elem;
  GapStoryElem *stb_prev;
  GapStoryElem *stb_next;


  stb_prev = NULL;
  for(stb_elem = stb->stb_elem; stb_elem != NULL;  stb_elem = stb_next)
  {
    stb_next = (GapStoryElem *)stb_elem->next;

    if((stb_elem->track == aud_track)
    && (gap_story_elem_is_audio(stb_elem)))
    {
      /* unlink the current element */
      if(stb_prev)
      {
        stb_prev->next = stb_next;
      }
      else
      {
        /* unlink the 1.st elem in the storyboard list */
        stb->stb_elem = stb_next;
      }
      
      gap_story_elem_free(&stb_elem);
    }
    else
    {
      stb_prev = stb_elem;
    }
  }

}  /* end gap_story_del_audio_track */





/* --------------------------
 * p_get_video_framerate
 * --------------------------
 */
gdouble
p_get_video_framerate(const char *videofile
                     ,gint32 videotrack
                     ,gint32 seltrack
                     ,const char *preferred_decoder
                     )
{
  gdouble l_video_framerate = 0.0;
#ifdef GAP_ENABLE_VIDEOAPI_SUPPORT
  t_GVA_Handle *gvahand= NULL;

  gvahand = GVA_open_read_pref(videofile
                              , videotrack
                              , seltrack
                              , preferred_decoder
                              , FALSE  /* use MMX if available (disable_mmx == FALSE) */
                              );
 l_video_framerate = gvahand->framerate;
 GVA_close(gvahand);
 
#endif

  return(l_video_framerate);
}  /* end p_get_video_framerate */

/* --------------------------
 * gap_story_gen_otone_audio
 * --------------------------
 * OUT: first_non_matching_framerate 0 if all is OK, else value of 1st non matching framerate.
 */
gboolean
gap_story_gen_otone_audio(GapStoryBoard *stb
                         ,gint vid_track
                         ,gint aud_track
                         ,gint aud_seltrack
                         ,gboolean replace_existing_aud_track
                         ,gdouble *first_non_matching_framerate
                         )
{
  GapStoryElem *stb_elem;
  GapStoryElem *stb_elem_new;
  gdouble      l_std_framerate;
  gdouble      l_framerate;
  
  gchar       *l_videoname = NULL;
  gdouble      l_video_framerate = 0.0;
  gint32       l_overlapcount;
 
  l_std_framerate = MAX(0.1, stb->master_framerate);
  l_framerate = l_std_framerate;
  
  *first_non_matching_framerate = 0; 
  
  /* handling for already existing audio track */
  if(replace_existing_aud_track)
  {
    gap_story_del_audio_track(stb, aud_track);
  }
  else
  {
    for(stb_elem = stb->stb_elem; stb_elem != NULL;  stb_elem = stb_elem->next)
    {
      if((stb_elem->track == aud_track)
      && (gap_story_elem_is_audio(stb_elem)))
      {
        /* there are already elements for the specified aud_track
         * and replace is not required.
         */
        return(FALSE);
      }
    }
  }

  l_overlapcount = 0;

  /* generate the otone track (at the lists tail) */
  for(stb_elem = stb->stb_elem; stb_elem != NULL;  stb_elem = stb_elem->next)
  {
    if(stb_elem->track == vid_track)
    {
      gint32 l_overlap_in_this_elem;
      
      l_overlap_in_this_elem = MIN(l_overlapcount, stb_elem->nframes);

      switch(stb_elem->record_type)
      {
        case GAP_STBREC_ATT_TRANSITION:
          l_overlapcount += stb_elem->att_overlap;
          break;
        case GAP_STBREC_VID_MOVIE:
          /* add otne for the movie when playing normal forward */
          if((stb_elem->playmode == GAP_STB_PM_NORMAL)
          && (stb_elem->step_density == 1.0)
          && (stb_elem->from_frame < stb_elem->to_frame)
          && (stb_elem->nframes > l_overlap_in_this_elem)
          )
          {
            stb_elem_new = gap_story_new_elem(GAP_STBREC_AUD_MOVIE);
            if(stb_elem_new)
            {
              gboolean l_check_videorate;
              gint32 l_from_frame;
              
              l_from_frame = stb_elem->from_frame + l_overlap_in_this_elem;
              
              l_check_videorate = TRUE;
              
              /* findout framerate of the source video file */
              if (l_videoname != NULL)
              {
                if(strcmp(l_videoname, stb_elem->orig_filename) == 0)
                {
                  /* the orig_filename matches the current videofile (that is already checked)
                   * we can skip the check and use the known l_video_framerate
                   */
                  l_framerate = l_video_framerate;
                  l_check_videorate = FALSE;
                }
                else
                {
                  g_free(l_videoname);
                  l_videoname = NULL;
                  l_video_framerate = l_std_framerate;
                }
              }
              if (l_check_videorate)
              {
                l_videoname = g_strdup(stb_elem->orig_filename);
                l_framerate = p_get_video_framerate(l_videoname
                                                   ,1
                                                   ,1
                                                   ,stb_elem->preferred_decoder
                                                   );
                if(l_framerate <= 0.0)
                {
                  l_framerate = l_std_framerate;
                }
                l_video_framerate = l_framerate;
              }
 
            
              /* initialise the new AUDIO element */ 
              if((*first_non_matching_framerate == 0.0)
              && (l_framerate != stb->master_framerate))
              {
                *first_non_matching_framerate = l_framerate;
              }
            
              stb_elem_new->aud_play_from_sec = 0.0;
              stb_elem_new->track             = aud_track;
              stb_elem_new->from_frame        = l_from_frame;
              stb_elem_new->to_frame          = stb_elem->to_frame;
              stb_elem_new->aud_filename      = g_strdup(stb_elem->aud_filename);
              stb_elem_new->orig_filename     = g_strdup(stb_elem->orig_filename);
              
              stb_elem_new->aud_play_from_sec = (gdouble)l_from_frame / l_framerate;
              stb_elem_new->aud_play_to_sec   = (gdouble)stb_elem->to_frame / l_framerate;
              stb_elem_new->aud_wait_untiltime_sec = 0.0;
              stb_elem_new->aud_volume             = 1.0;
              stb_elem_new->aud_volume_start       = 1.0;
              stb_elem_new->aud_fade_in_sec        = 0.0;
              stb_elem_new->aud_volume_end         = 1.0;
              stb_elem_new->aud_fade_out_sec       = 0.0;
              stb_elem_new->nloop                  = stb_elem->nloop;
              stb_elem_new->aud_seltrack           = aud_seltrack;
              stb_elem_new->preferred_decoder      = g_strdup(stb_elem->preferred_decoder);

              stb_elem_new->aud_framerate          = l_framerate;

              gap_story_list_append_elem(stb, stb_elem_new);
            }
            break;
          }
          /* no break at this point, 
           * same handling as all other vid record types is required
           */
        case GAP_STBREC_VID_SILENCE:
        case GAP_STBREC_VID_COLOR:
        case GAP_STBREC_VID_IMAGE:
        case GAP_STBREC_VID_ANIMIMAGE:
        case GAP_STBREC_VID_FRAMES:

          if(stb_elem->nframes > l_overlap_in_this_elem)
          {
            /* add silence for the duration of those elements */
            stb_elem_new = gap_story_new_elem(GAP_STBREC_AUD_SILENCE);
            if(stb_elem_new)
            {
              stb_elem_new->aud_play_from_sec      = 0.0;
              stb_elem_new->track                  = aud_track;
              stb_elem_new->aud_play_from_sec      = 0.0;
              stb_elem_new->aud_play_to_sec        = (stb_elem->nframes - l_overlap_in_this_elem)
                                                     / l_std_framerate;
              stb_elem_new->aud_wait_untiltime_sec = 0.0;

              gap_story_list_append_elem(stb, stb_elem_new);
            }
          }
          
          break;
        case GAP_STBREC_VID_COMMENT:
        case GAP_STBREC_VID_UNKNOWN:
          break;
        default:
          break;
      }
      
      l_overlapcount = (l_overlapcount - l_overlap_in_this_elem);
      
      
    }
  }

  return(TRUE);
  
}  /* end gap_story_gen_otone_audio */


/* -------------------------------
 * gap_story_get_default_attribute
 * -------------------------------
 */
gdouble
gap_story_get_default_attribute(gint att_typ_idx)
{
   if ((att_typ_idx == GAP_STB_ATT_TYPE_OPACITY)
   ||  (att_typ_idx == GAP_STB_ATT_TYPE_ZOOM_X)
   ||  (att_typ_idx == GAP_STB_ATT_TYPE_ZOOM_Y))
   {
     return (1.0);  /* 1.0 indicates fully opaque, or no scaling */
   }
   return (0.0);  /* indicates centerd positioning */
}  /* end gap_story_get_default_attribute */


/* ------------------------------------------
 * gap_story_file_calculate_render_attributes
 * ------------------------------------------
 * perform calculations for rendering a layer or image (specified by frame_width, frame_height)
 * onto a destination image (specified by vid_width, vid_height)
 *
 * according to storyboard attribute settings
 * (specified by
 *   fit_width
 *   fit_height
 *   keep_proportions
 *   scale_x       1.0 .. original width,  2.0 double width     
 *   scale_y       1.0 .. original height, 2.0 double height  
 *   move_x        (-1.0 .. left outside, 0.0 centered, +1.0 right outside)
 *   move_y        (-1.0 .. top  outside, 0.0 centered, +1.0 bottom outside)
 *   opacity       0.0 .. fully transparent, 1.0 fully opaque
 * )
 *
 * the results are written to the speccified result_attr structure,
 * and shall be applied to the layer. 
 *    
 */
void
gap_story_file_calculate_render_attributes(GapStoryCalcAttr *result_attr
    , gint32 view_vid_width
    , gint32 view_vid_height
    , gint32 vid_width
    , gint32 vid_height
    , gint32 frame_width
    , gint32 frame_height
    , gboolean keep_proportions
    , gboolean fit_width
    , gboolean fit_height
    , gdouble opacity
    , gdouble scale_x
    , gdouble scale_y
    , gdouble move_x
    , gdouble move_y
    )
{
  gint32 result_width;
  gint32 result_height;
  gint32 result_ofsx;
  gint32 result_ofsy;
  gint32 l_tmp_width;
  gint32 l_tmp_height;
  gint32 center_x_ofs;
  gint32 center_y_ofs;
  
  
  if(result_attr == NULL)
  {
    return;
  }

  result_width = -1;
  result_height = -1;
  l_tmp_width = frame_width;
  l_tmp_height = frame_height;
  
  if(keep_proportions)
  {
    gdouble l_vid_prop;
    gdouble l_frame_prop;

    l_vid_prop = (gdouble)vid_width / (gdouble)vid_height;
    l_frame_prop = (gdouble)frame_width / (gdouble)frame_height;


    if((fit_width) && (fit_height))
    {
      l_tmp_width = vid_height * l_frame_prop;
      l_tmp_height = vid_height;
      
      if(l_tmp_width > vid_width)
      {
        l_tmp_width = vid_width;
        l_tmp_height = vid_width / l_frame_prop;
      }

      /* scaling */
      result_width  = MAX(1, (l_tmp_width  * scale_x));
      result_height = MAX(1, (l_tmp_height * scale_y));

    }
    else
    {
      if(fit_height)
      {
         l_tmp_width = vid_height * l_frame_prop;
      }
      if(fit_width)
      {
         l_tmp_height = vid_width / l_frame_prop;
      }
    }

  }

  /* scaling (if not already done at proportion calculation) */
  if(result_width < 0)
  {
    if(fit_width)
    {
      result_width  = MAX(1, (vid_width  * scale_x));
    }
    else
    {
      result_width  = MAX(1, (l_tmp_width  * scale_x));
    }

    if(fit_height)
    {
      result_height = MAX(1, (vid_height * scale_y));
    }
    else
    {
      result_height = MAX(1, (l_tmp_height * scale_y));
    }
  }


  /* offset calculation */
  {
    
    center_x_ofs = (vid_width/2) -  (result_width/2);
    center_y_ofs = (vid_height/2) - (result_height/2);
    
    result_ofsx  = center_x_ofs + ((result_width / 2)  * move_x) + ((vid_width / 2)  * move_x);
    result_ofsy  = center_y_ofs + ((result_height / 2 ) * move_y) + ((vid_height / 2 ) * move_y);
    
  }

  result_attr->opacity = CLAMP(opacity * 100.0, 0.0, 100.0);
  result_attr->width = result_width;
  result_attr->height = result_height;
  result_attr->x_offs = result_ofsx + ((view_vid_width - vid_width) / 2);
  result_attr->y_offs = result_ofsy + ((view_vid_height - vid_height) / 2);

  /* debug output */
  if(gap_debug)
  {
    
    printf ("\n\nCALC size and OFFS opacity: %f", (float)result_attr->opacity);
    {
      printf (" fit_width: ");
      if (fit_width) { printf ("ON "); } else  { printf ("OFF "); }
      printf (" fit_height: ");
      if (fit_height) { printf ("ON "); } else  { printf ("OFF "); }
      printf (" keep_prop: ");
      if (keep_proportions) { printf ("ON "); } else  { printf ("OFF "); }
      printf ("\n");
    }
    printf (" view: (%d x %d) master: (%d x %d) frame: (%d x %d)\n"
              ,(int)view_vid_width
              ,(int)view_vid_height
              ,(int)vid_width
              ,(int)vid_height
              ,(int)frame_width
              ,(int)frame_height
              );
    printf (" scale_x:%.3f scale_y:%.3f\n", (float)scale_x , (float) scale_y);
    printf (" move_x:%.3f move_y:%.3f\n", (float)move_x , (float) move_y);
    printf (" result_width:%d result_height:%d\n", (int)result_width , (int) result_height);
    printf (" center_x_ofs:%d center_y_ofs:%d  result_ofsx: %d  result_ofs_y: %d\n"
              , (int)center_x_ofs
              , (int)center_y_ofs
              , (int)result_ofsx
              , (int)result_ofsy
              );
    printf (" OFSX:%d OFSY:%d\n", (int)result_attr->x_offs , (int) result_attr->y_offs);

  }
  
}  /* end gap_story_file_calculate_render_attributes */
