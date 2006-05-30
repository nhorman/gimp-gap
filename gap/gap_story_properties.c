/*  gap_story_properties.c
 *
 *  This module handles GAP storyboard dialog properties window
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
 * version 1.3.26a; 2004/02/21  hof: created
 */


#include "config.h"

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <glib/gstdio.h>

#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gap_story_main.h"
#include "gap_story_dialog.h"
#include "gap_story_file.h"
#include "gap_story_properties.h"
// #include "gap_player_dialog.h"
// #include "gap_pdb_calls.h"
#include "gap_pview_da.h"
#include "gap_stock.h"
#include "gap_lib.h"
// #include "gap_image.h"
#include "gap_vin.h"
#include "gap_timeconv.h"
#include "gap_thumbnail.h"


#include "gap-intl.h"


#define GAP_STORY_CLIP_PROP_HELP_ID  "plug-in-gap-storyboard-clip-prop"

#define GAP_STORY_RESPONSE_RESET 1
#define GAP_STORY_RESPONSE_SCENE_SPLIT 2
#define GAP_STORY_RESPONSE_SCENE_END   3
#define PW_ENTRY_WIDTH        300
#define PW_COMMENT_WIDTH      480
#define PW_SCALE_WIDTH        300
#define PW_SPIN_BUTTON_WIDTH   80

/* GAP_STORY_SCENE_DIFF_THRESHOLD max: 196608 (256 * 256) * 3 */
#define GAP_STORY_SCENE_DIFF_THRESHOLD_MAX  196608
#define GAP_STORY_SCENE_DIFF_THRESHOLD      3000
#define GAP_STORY_SCENE_DIFF_DIFF_THRESHOLD 700

/* GAP_STORY_SCENE_CMP_IGNORE: 0.0 upto 0.5
 * a value of 0.0 does not tolerate "Appering Objects"
 * and force a new scene if objects apper in a scene.
 * A value of 0.5% ignore big color differences for 50% of the checked
 * blocks in a frame. This is tolerant for big Objects appearing in a scene
 * but may fail to detect differences for most other scene changes.
 */
#define GAP_STORY_SCENE_CMP_IGNORE          0.15
#define GAP_STORY_SCENE_CMP_THRESHOLD       8000

#define GAP_STORY_PROPERTIES_PREVIOUS_FILENAME "GAP_STORY_PROPERTIES_PREVIOUS_FILENAME"

extern int gap_debug;  /* 1 == print debug infos , 0 dont print debug infos */

static void     p_pw_prop_reset_all(GapStbPropWidget *pw);
static void     p_pw_prop_response(GtkWidget *widget
                  , gint       response_id
                  , GapStbPropWidget *pw
                  );

static const char * p_pw_get_preferred_decoder(GapStbPropWidget *pw);
static gdouble  p_overall_colordiff(guchar *buf1, guchar *buf2
                                   , gint32 th_width
                                   , gint32 th_height
                                   , gint32 th_bpp
                                   , gdouble cmp_ignore);
static void     p_pw_auto_scene_split(GapStbPropWidget *pw, gboolean all_scenes);



static void     p_pw_timer_go_job(GapStbPropWidget *pw);
static void     p_pv_pview_render_immediate (GapStbPropWidget *pw);
static void     p_pv_pview_render (GapStbPropWidget *pw);
static gboolean p_pw_preview_events_cb (GtkWidget *widget
                       , GdkEvent  *event
                       , GapStbPropWidget *pw);
static void     p_pw_check_ainfo_range(GapStbPropWidget *pw, char *filename);
static void     p_pw_filename_entry_update_cb(GtkWidget *widget, GapStbPropWidget *pw);
static void     p_pw_filename_changed(const char *filename, GapStbPropWidget *pw);
static void     p_filesel_pw_ok_cb (GtkWidget *widget, GapStbPropWidget *pw);
static void     p_filesel_pw_close_cb ( GtkWidget *widget, GapStbPropWidget *pw);
static void     p_pw_filesel_button_cb ( GtkWidget *w, GapStbPropWidget *pw);
static void     p_pw_comment_entry_update_cb(GtkWidget *widget, GapStbPropWidget *pw);
static void     p_pw_fmac_entry_update_cb(GtkWidget *widget, GapStbPropWidget *pw);
static void     p_pw_update_info_labels(GapStbPropWidget *pw);
static void     p_pw_update_framenr_labels(GapStbPropWidget *pw, gint32 framenr);
static void     p_pw_update_properties(GapStbPropWidget *pw);
static void     p_pw_gint32_adjustment_callback(GtkObject *obj, gint32 *val);
static void     p_pw_pingpong_toggle_update_callback(GtkWidget *widget, GapStbPropWidget *pw);
static void     p_radio_delace_none_callback(GtkWidget *widget, GapStbPropWidget *pw);
static void     p_radio_delace_odd_callback(GtkWidget *widget, GapStbPropWidget *pw);
static void     p_radio_delace_even_callback(GtkWidget *widget, GapStbPropWidget *pw);
static void     p_delace_spinbutton_cb(GtkObject *obj, GapStbPropWidget *pw);
static void     p_step_density_spinbutton_cb(GtkObject *obj, GapStbPropWidget *pw);



/* ---------------------------------
 * p_pw_prop_reset_all
 * ---------------------------------
 */
static void
p_pw_prop_reset_all(GapStbPropWidget *pw)
{
  gboolean pingpong_state;
  gboolean comment_set;

  if(pw->stb_elem_refptr)
  {
    pw->stb_refptr->unsaved_changes = TRUE;
    gap_story_elem_copy(pw->stb_elem_refptr, pw->stb_elem_bck);

    /* must be first to set the record_type */
    p_pw_update_info_labels(pw);

    pingpong_state = (pw->stb_elem_refptr->playmode == GAP_STB_PM_PINGPONG);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pw->pingpong_toggle), pingpong_state);

    if(pw->stb_elem_refptr->orig_filename)
    {
      gtk_entry_set_text(GTK_ENTRY(pw->pw_filename_entry), pw->stb_elem_bck->orig_filename);
    }
    comment_set = FALSE;
    if(pw->stb_elem_refptr->comment)
    {
      if(pw->stb_elem_refptr->comment->orig_src_line)
      {
        gtk_entry_set_text(GTK_ENTRY(pw->comment_entry), pw->stb_elem_refptr->comment->orig_src_line);
        comment_set = TRUE;
      }
    }
    if(!comment_set)
    {
        gtk_entry_set_text(GTK_ENTRY(pw->comment_entry), "");
    }

    gtk_adjustment_set_value(GTK_ADJUSTMENT(pw->pw_spinbutton_from_adj), pw->stb_elem_refptr->from_frame);
    gtk_adjustment_set_value(GTK_ADJUSTMENT(pw->pw_spinbutton_to_adj), pw->stb_elem_refptr->to_frame);
    gtk_adjustment_set_value(GTK_ADJUSTMENT(pw->pw_spinbutton_loops_adj), pw->stb_elem_refptr->nloop);
    gtk_adjustment_set_value(GTK_ADJUSTMENT(pw->pw_spinbutton_step_density_adj), pw->stb_elem_refptr->step_density);

    p_pw_update_properties(pw);
  }

}  /* end p_pw_prop_reset_all */


/* ---------------------------------
 * p_pw_prop_response
 * ---------------------------------
 */
static void
p_pw_prop_response(GtkWidget *widget
                  , gint       response_id
                  , GapStbPropWidget *pw
                  )
{
  GtkWidget *dlg;
  GapStbMainGlobalParams  *sgpp;

  sgpp = pw->sgpp;
  switch (response_id)
  {
    case GAP_STORY_RESPONSE_RESET:
      if((pw->stb_elem_bck)
      && (pw->stb_elem_refptr)
      && (!pw->scene_detection_busy))
      {
        p_pw_prop_reset_all(pw);
      }
      break;
    case GAP_STORY_RESPONSE_SCENE_END:
    case GAP_STORY_RESPONSE_SCENE_SPLIT:
      if((!pw->scene_detection_busy)
      && (!sgpp->gva_lock)
      && (pw->pw_filesel == NULL))
      {
        gboolean all_scenes;

        pw->stb_refptr->unsaved_changes = TRUE;
        pw->scene_detection_busy = TRUE;
        all_scenes = FALSE;
        if(response_id == GAP_STORY_RESPONSE_SCENE_SPLIT)
        {
          all_scenes = TRUE;
        }

        p_pw_auto_scene_split(pw, all_scenes);
        gtk_adjustment_set_value(GTK_ADJUSTMENT(pw->pw_spinbutton_to_adj)
                            , pw->stb_elem_refptr->to_frame);
        p_pv_pview_render(pw);
        pw->scene_detection_busy = FALSE;
      }

      if(!pw->close_flag)
      {
        break;
      }

      /* the p_pw_auto_scene_split procedure was cancelled (by close_flag)
       * in this case run into the close code
       */
    case GTK_RESPONSE_CLOSE:
    default:
      if((response_id == GTK_RESPONSE_CLOSE)
      && (pw->scene_detection_busy))
      {
        /* we were called from within the p_pw_auto_scene_split procedure
         * that has not finished yet. in this case just set the close_flag
         * The p_pw_auto_scene_split procedure will stop at next loop
         * if close_flag  == TRUE
         */
        pw->close_flag = TRUE;
        return;
      }
      if(pw->pw_filesel)
      {
        p_filesel_pw_close_cb(pw->pw_filesel, pw);
      }
      if(pw->go_timertag >= 0)
      {
        g_source_remove(pw->go_timertag);
      }
      pw->go_timertag = -1;
      dlg = pw->pw_prop_dialog;
      if(dlg)
      {
        pw->pw_prop_dialog = NULL;
        gtk_widget_destroy(dlg);
      }
      break;
  }
}  /* end p_pw_prop_response */


/* ---------------------------------
 * p_pw_get_preferred_decoder
 * ---------------------------------
 */
static const char *
p_pw_get_preferred_decoder(GapStbPropWidget *pw)
{
  return(gap_story_get_preferred_decoder(pw->stb_refptr, pw->stb_elem_refptr));
}  /* end p_pw_get_preferred_decoder */



/* -------------------------------
 * p_overall_colordiff
 * -------------------------------
 * guess overall color difference
 * for 2 RGB (or RGBA) databuffers of same size.
 *
 * the imagedata is processed in sub blocks of 8x8 pixel
 * The returned difference is 0 if all checked blocks
 * are equal in color.
 * A maximum difference of 196608 (256 * 256) * 3
 * will be returned if buf2 is the negative of buf1
 * (full black versus full white)
 *
 * big differences are ignored until there are
 * more than GAP_STORY_SCENE_CMP_IGNORE percent (0.0 upto 1.0)
 *  of the compared blocks having big differences
 * (if Objects enter a scene there are typical some blocks
 *  with big differences that should not be triggered as new scene start)
 *
 * For performance reasons we check
 * only 1 Block in a square of 2x2 Blocks
 */
gdouble
p_overall_colordiff(guchar *buf1, guchar *buf2
                   , gint32 th_width
                   , gint32 th_height
                   , gint32 th_bpp
                   , gdouble cmp_ignore)
{
  gint32 stepsize, bpp_step;
  gint32 checked_blocks;
  gint32 pixels_per_block;
  gint32 ii, xx, yy, jj, x, y;
  gdouble color_diff;
  gdouble color_diff_sum;
  gint32 rdif, gdif, bdif;
  gint32 rpick1, gpick1, bpick1;
  gint32 rpick2, gpick2, bpick2;
  gint32 pick_radius;
  gint32 rowstride;
  gint32 count_big_diff;
  gdouble ignore_big_diff_blocks;

  color_diff_sum = 0;
  checked_blocks = 0;
  count_big_diff = 0;

  pick_radius = MIN(8, th_width);
  pick_radius = MIN(pick_radius, th_height);

  rowstride = th_width * th_bpp;
  stepsize = pick_radius * 2;
  bpp_step = stepsize * th_bpp;
  pixels_per_block = pick_radius * pick_radius;
  ignore_big_diff_blocks = (th_height / stepsize) * (th_width / stepsize);
  ignore_big_diff_blocks *= cmp_ignore;

  for(y = 0; y < th_height - pick_radius; y += stepsize)
  {
    ii = y * rowstride;
    for(x = 0; x < th_width - pick_radius; x += stepsize)
    {
      rpick1 = 0;  rpick2 = 0;
      gpick1 = 0;  gpick2 = 0;
      bpick1 = 0;  bpick2 = 0;

      /* calculate summary color of one block */
      for(yy = 0; yy < pick_radius; yy++)
      {
        jj = ii + (yy * rowstride);
        for(xx = 0; xx < pick_radius; xx++)
        {
          rpick1 += buf1[jj];
          rpick2 += buf2[jj];
          gpick1 += buf1[jj +1];
          gpick2 += buf2[jj +1];
          bpick1 += buf1[jj +2];
          bpick2 += buf2[jj +2];
          jj += th_bpp;
        }
      }

      rdif = abs(rpick1 - rpick2) / pixels_per_block;
      gdif = abs(gpick1 - gpick2) / pixels_per_block;
      bdif = abs(bpick1 - bpick2) / pixels_per_block;

      color_diff =  ( (rdif * rdif)
                    + (gdif * gdif)
                    + (bdif * bdif)
                    );
      if((color_diff < GAP_STORY_SCENE_CMP_THRESHOLD)
      || (count_big_diff > ignore_big_diff_blocks))
      {
        color_diff_sum += color_diff;
        checked_blocks++;
      }
      else
      {
        /* count blocks with big differences */
        count_big_diff++;
      }
      ii += bpp_step;
    }
  }

  return(color_diff_sum / MAX(checked_blocks,1));
}  /* end p_overall_colordiff */


/* -------------------------------
 * p_pw_auto_scene_split
 * -------------------------------
 */
static void
p_pw_auto_scene_split(GapStbPropWidget *pw, gboolean all_scenes)
{
  guchar *prev_th_data;
  guchar *th_data;
  gint32 l_th_width;
  gint32 l_th_height;
  gint32 l_th_bpp;
  gint32 l_1st_th_width;
  gint32 l_1st_th_height;
  gint32 l_1st_th_bpp;
  gint32 framenr;
  gint32 framenr_max;
  gint32 video_id;
  gdouble diff;
  gdouble sum_diff;
  gdouble num_diff;
  gdouble diff_threshold;
  gdouble diff_diff_threshold;
  gdouble l_cmp_ignore;
  gboolean drop_th_data;
  gboolean drop_prev_th_data;
  GapStoryElem *stb_elem;
  GapStbMainGlobalParams  *sgpp;
  GdkPixbuf *pixbuf;
  GdkPixbuf *prev_pixbuf;

  stb_elem = pw->stb_elem_refptr;
  sgpp = pw->sgpp;
  if(stb_elem == NULL)
  {
    return;
  }
  if((stb_elem->record_type != GAP_STBREC_VID_MOVIE)
  && (stb_elem->record_type != GAP_STBREC_VID_FRAMES))
  {
    g_message(_("Automatic scene detection operates only "
                "on cliptypes MOVIE and FRAMES"));
    return;
  }
  if ((stb_elem->record_type == GAP_STBREC_VID_MOVIE)
  &&  (sgpp->auto_vthumb == FALSE))
  {
    g_message(_("Scene detection depends on video thumbnails. "
                "Please enable videothumbnails (in the Windows Menu)"));
    return;
  }

  pixbuf = NULL;
  prev_pixbuf = NULL;
  prev_th_data = NULL;
  framenr = stb_elem->from_frame;
  framenr_max = stb_elem->to_frame;

  if((framenr_max <= framenr)
  || (all_scenes == FALSE))
  {
    /* allow search until end */
    framenr_max =999999;
  }

  drop_th_data = FALSE;
  drop_prev_th_data = FALSE;
  l_cmp_ignore = GAP_STORY_SCENE_CMP_IGNORE;

  diff_threshold = GAP_STORY_SCENE_DIFF_THRESHOLD;
  diff_diff_threshold = GAP_STORY_SCENE_DIFF_DIFF_THRESHOLD;
  sum_diff = 0;
  num_diff = 0;
  if(stb_elem->record_type == GAP_STBREC_VID_MOVIE)
  {
    prev_th_data = gap_story_dlg_fetch_vthumb(sgpp
              ,stb_elem->orig_filename
              ,stb_elem->from_frame
              ,stb_elem->seltrack
              ,p_pw_get_preferred_decoder(pw)
              ,&l_th_bpp
              ,&l_th_width
              ,&l_th_height
              );
  }
  else
  {
    char *filename;

    filename = gap_lib_alloc_fname(stb_elem->basename
                                   , stb_elem->from_frame
                                   , stb_elem->ext
                                   );
    if(filename)
    {
      prev_pixbuf = gap_thumb_file_load_pixbuf_thumbnail(filename
                                      , &l_th_width
                                      , &l_th_height
                                      , &l_th_bpp
                                      );
      g_free(filename);
      if(prev_pixbuf)
      {
        prev_th_data = gdk_pixbuf_get_pixels(prev_pixbuf);
      }
      else
      {
        g_message(_("Scene detection for cliptype FRAMES "
                    "depends on thumbnails. "
                    "Please create thumbnails for your frames and then try again."));
        return;
      }

    }
  }
  if(prev_th_data == NULL)
  {
    return;
  }
  l_1st_th_width  = l_th_width;
  l_1st_th_height = l_th_height;
  l_1st_th_bpp    = l_th_bpp;

  while(TRUE)
  {
    framenr++;
    /* fetch vthumb from vthumb list or read from videofile
     * without adding to the vthumb list
     * (the list would grow too much if all thumbnails of all handled
     *  videos were added.
     *  therefore we add only vthumbs of the startframes of a new scene)
     *
     */
    if(stb_elem->record_type == GAP_STBREC_VID_MOVIE)
    {
      th_data = gap_story_dlg_fetch_vthumb_no_store(sgpp
              ,stb_elem->orig_filename
              ,framenr
              ,stb_elem->seltrack
              ,p_pw_get_preferred_decoder(pw)
              ,&l_th_bpp
              ,&l_th_width
              ,&l_th_height
              ,&drop_th_data   /* TRUE if th_data was not already in vthumb list */
              ,&video_id
              );
    }
    else
    {
      char *filename;

      th_data = NULL;
      filename = gap_lib_alloc_fname(stb_elem->basename
                                   , framenr
                                   , stb_elem->ext
                                   );
      if(filename)
      {
        pixbuf = gap_thumb_file_load_pixbuf_thumbnail(filename
                                        , &l_th_width
                                        , &l_th_height
                                        , &l_th_bpp
                                        );
        if(pixbuf)
        {
          th_data = gdk_pixbuf_get_pixels(pixbuf);
        }
        else
        {
          if(g_file_test(filename, G_FILE_TEST_EXISTS))
          {
            g_message(_("Scene detection for cliptype FRAMES "
                        "depends on thumbnails. "
                        "Please create thumbnails for your frames and then try again."));
          }
        }
        g_free(filename);
      }
    }


    if((th_data == NULL)
    || (l_1st_th_width  != l_th_width)
    || (l_1st_th_height != l_th_height)
    || (l_1st_th_bpp    != l_th_bpp))
    {
      /* STOP because we cant compare,
       * we have no current frame of same size as prev. frame
       */
      stb_elem->to_frame = framenr -1;
      return;
    }

    diff = p_overall_colordiff(prev_th_data
                              ,th_data
                              ,l_th_width
                              ,l_th_height
                              ,l_th_bpp
                              ,l_cmp_ignore
                              );


    /* visible trace the current frame in the pview widget */
    gap_pview_render_from_buf (pw->pv_ptr
                    , th_data
                    , l_th_width
                    , l_th_height
                    , l_th_bpp
                    , FALSE         /* Dont allow_grab_src_data */
                    );

    p_pw_update_framenr_labels(pw, framenr);

    sum_diff += diff;
    num_diff++;

    if(gap_debug)
    {
      printf("SCENE: frame_max:%d frame:%d  THRES:%d AVG_DIFF:%d DIFF:%d\n"
      , (int)framenr_max
      , (int)framenr
      , (int)diff_threshold
      , (int)(sum_diff /  num_diff)
      , (int)diff
      );
    }

    /* check if diff is bigger than abs threshold
     * or if we have more than 4 frames:
     *   if current diff is 8 times bigger than the average diff
     * we force a new SCENE
     * we also do new SCENE if Scene detection was stopped by close_flag
     * or if frame_max (limit) is reached
     */
    if((diff > diff_threshold)
    || ((num_diff > 4) && (diff > (8 * sum_diff /  num_diff)) && (diff > diff_diff_threshold))
    || (framenr >= framenr_max)
    || (pw->close_flag))
    {
      diff = 0;
      sum_diff = 0;
      num_diff = 0;

      /* raise the thresholds for the first frame after the cut
       * (avoid ultra short scenes with less than 3 frames)
       */
      diff_threshold = GAP_STORY_SCENE_DIFF_THRESHOLD_MAX * 2;
      diff_diff_threshold = GAP_STORY_SCENE_DIFF_THRESHOLD_MAX * 2;

      stb_elem->to_frame = framenr -1;
      if(!all_scenes)
      {
        return;
      }
      else
      {
        GapStoryElem *stb_elem_new;

        if((framenr >= framenr_max)
        || (pw->close_flag))
        {
          return;
        }

        if(gap_debug) printf("AUTO SCENE NEW_ELEM:\n");

        /* add a new Element for the next scene */
        stb_elem_new = gap_story_elem_duplicate(stb_elem);

        stb_elem_new->from_frame = framenr;
        stb_elem_new->to_frame = framenr;
        gap_story_elem_calculate_nframes(stb_elem_new);

        stb_elem_new->next = stb_elem->next;
        stb_elem->next = stb_elem_new;
        stb_elem = stb_elem_new;

        if(gap_debug)
	{
	  printf("AUTO SCENE NEW_ELEM linked to list: drop:%d, video_id:%d\n"
	       ,(int)drop_th_data
	       ,(int)video_id
	       );
	}

        if((stb_elem->record_type == GAP_STBREC_VID_MOVIE)
        && (drop_th_data)
        && (video_id >= 0))
        {
           if(gap_debug) printf("AUTO SCENE ADD VTHUMB:\n");
           drop_th_data = FALSE;
           gap_story_dlg_add_vthumb(sgpp
                                   ,framenr
                                   ,th_data
                                   ,l_th_width
                                   ,l_th_height
                                   ,l_th_bpp
                                   ,video_id
                                   );
           /* refresh storyboard layout and thumbnail list widgets */
           gap_story_dlg_recreate_tab_widgets(pw->tabw
                                  ,sgpp
                                  );
        }
        if(stb_elem->record_type != GAP_STBREC_VID_MOVIE)
        {
           /* refresh storyboard layout and thumbnail list widgets */
           gap_story_dlg_recreate_tab_widgets(pw->tabw
                                  ,sgpp
                                  );
        }
      }

    }

    if((prev_th_data) && (drop_prev_th_data))
    {
      /* throw away video thumbnails for frames that
       * are not start of a scene
       * or were not already in the vthum list before
       */
      if(stb_elem->record_type == GAP_STBREC_VID_MOVIE)
      {
        g_free(prev_th_data);
      }
      else
      {
        g_object_unref(prev_pixbuf);
        prev_pixbuf = NULL;
      }
      prev_th_data = NULL;
    }
    if(stb_elem->record_type == GAP_STBREC_VID_MOVIE)
    {
      prev_th_data = th_data;
    }
    else
    {
      prev_th_data = gdk_pixbuf_get_pixels(pixbuf);
      prev_pixbuf = pixbuf;
    }
    drop_prev_th_data = drop_th_data;
    diff_threshold = MAX(GAP_STORY_SCENE_DIFF_THRESHOLD, (diff_threshold / 4));
    diff_diff_threshold = MAX(GAP_STORY_SCENE_DIFF_DIFF_THRESHOLD, (diff_diff_threshold / 4));

    /* g_main_context_iteration makes sure that
     *  gtk does refresh widgets,  and react on events while the scene detection
     *  is busy.
     */
    while(g_main_context_iteration(NULL, FALSE));

  }
}  /* end p_pw_auto_scene_split */


/* ---------------------------------
 * p_pv_pview_render_immediate
 * ---------------------------------
 * 1.) fetch thumbnal pixbuf data,
 * 2.) if no thumbnail available
 *     try to load full image (and create the thumbnail for next usage)
 * 3.) if neither thumbnail nor image could be fetched
 *        render a default icon
 *
 */
static void
p_pv_pview_render_immediate (GapStbPropWidget *pw)
{
   gint32  l_th_width;
   gint32  l_th_height;
   gint32  l_th_bpp;
   GdkPixbuf *pixbuf;
   char     *l_frame_filename;


   /* init preferred width and height
    * (as hint for the thumbnail loader to decide
    *  if thumbnail is to fetch from normal or large thumbnail directory
    *  just for the case when both sizes are available)
    */
   l_th_width = 128;
   l_th_height = 128;

   if(pw->stb_elem_refptr->record_type == GAP_STBREC_VID_MOVIE)
   {
     guchar *l_th_data;
     /* if(gap_debug) printf("RENDER: p_pv_pview_render MOVIE Thumbnail\n"); */

     l_th_data = gap_story_dlg_fetch_vthumb(pw->sgpp
              ,pw->stb_elem_refptr->orig_filename
              ,pw->stb_elem_refptr->from_frame
              ,pw->stb_elem_refptr->seltrack
              ,p_pw_get_preferred_decoder(pw)
              ,&l_th_bpp
              ,&l_th_width
              ,&l_th_height
              );
     if(l_th_data)
     {
       gboolean l_th_data_was_grabbed;

       l_th_data_was_grabbed = gap_pview_render_from_buf (pw->pv_ptr
                    , l_th_data
                    , l_th_width
                    , l_th_height
                    , l_th_bpp
                    , TRUE         /* allow_grab_src_data */
                    );
       if(!l_th_data_was_grabbed)
       {
         /* the gap_pview_render_from_buf procedure can grab the l_th_data
          * instead of making a ptivate copy for later use on repaint demands.
          * if such a grab happened it returns TRUE.
          * (this is done for optimal performance reasons)
          * in such a case the caller must NOT free the src_data (l_th_data) !!!
          */
          g_free(l_th_data);
       }
       l_th_data = NULL;
       p_pw_update_framenr_labels(pw, pw->stb_elem_refptr->from_frame);
       return;
     }

   }

   l_frame_filename = gap_story_get_filename_from_elem(pw->stb_elem_refptr);
   if(l_frame_filename == NULL)
   {
     /* no filename available, use default icon */
     gap_story_dlg_render_default_icon(pw->stb_elem_refptr, pw->pv_ptr);
     return;
   }

   pixbuf = gap_thumb_file_load_pixbuf_thumbnail(l_frame_filename
                                    , &l_th_width, &l_th_height
                                    , &l_th_bpp
                                    );
   if(pixbuf)
   {
     gap_pview_render_from_pixbuf (pw->pv_ptr, pixbuf);
     g_object_unref(pixbuf);
   }
   else
   {
      gint32  l_image_id;

      l_image_id = gap_lib_load_image(l_frame_filename);

      if (l_image_id < 0)
      {
        /* could not read the image
         */
        if(gap_debug) printf("p_frame_widget_render: fetch failed, using DEFAULT_ICON\n");
        gap_story_dlg_render_default_icon(pw->stb_elem_refptr, pw->pv_ptr);
      }
      else
      {
        /* there is no need for undo on this scratch image
         * so we turn undo off for performance reasons
         */
        gimp_image_undo_disable (l_image_id);
        gap_pview_render_from_image (pw->pv_ptr, l_image_id);

        /* create thumbnail (to speed up acces next time) */
        gap_thumb_cond_gimp_file_save_thumbnail(l_image_id, l_frame_filename);

        gimp_image_delete(l_image_id);
      }
   }

   p_pw_update_framenr_labels(pw, pw->stb_elem_refptr->from_frame);

}       /* end p_pv_pview_render_immediate */

/* ------------------
 * p_pw_timer_go_job
 * ------------------
 */
static void
p_pw_timer_go_job(GapStbPropWidget *pw)
{
  GapStbMainGlobalParams  *sgpp;

  /*if(gap_debug) printf("\np_pw_timer_go_job: START\n");*/
  sgpp = pw->sgpp;

  if((pw)
  && (sgpp))
  {

    if(pw->go_timertag >= 0)
    {
      g_source_remove(pw->go_timertag);
    }
    pw->go_timertag = -1;

    if((pw->go_job_framenr >= 0)
    ||(pw->go_render_all_request))
    {
      if(sgpp->gva_lock)
      {
        /* the video api is still busy with fetching the previous frame
         * (do not disturb, but setup the go_timer for a next try
         * after 96 milliseconds)
         */
        pw->go_timertag = (gint32) g_timeout_add(96, (GtkFunction)p_pw_timer_go_job, pw);

        /*if(gap_debug) printf("p_pw_timer_go_job: TRY LATER (96msec) %06d\n", (int)pw->go_job_framenr); */
      }
      else
      {
        if(pw->stb_elem_refptr)
        {
          if(pw->go_render_all_request)
          {
            if (pw->go_job_framenr >= 0)
            {
              pw->stb_elem_refptr->from_frame = pw->go_job_framenr;
              p_pv_pview_render_immediate(pw);
            }
            gap_story_dlg_pw_render_all(pw);
            pw->go_render_all_request = FALSE;

            /* sometimes the displayed thumbnail does not match with the displayed
             * from_frame number at this point. (dont know exactly why)
             * the simple workaround is just render twice
             * and now it seems to work fine.
             * (rendering twice should not be too slow, since the requested videothumbnails
             *  are now cached in memory)
             */
            p_pv_pview_render_immediate(pw);
            gap_story_dlg_pw_render_all(pw);
          }
          else
          {
            if (pw->go_job_framenr >= 0)
            {
              pw->stb_elem_refptr->from_frame = pw->go_job_framenr;
              p_pv_pview_render_immediate(pw);

            }
          }

          pw->go_job_framenr = -1;

        }

      }
    }

  }
}  /* end p_pw_timer_go_job */


/* ---------------------------------
 * p_pv_pview_render
 * ---------------------------------
 */
static void
p_pv_pview_render (GapStbPropWidget *pw)
{
   if(pw->stb_elem_refptr->record_type != GAP_STBREC_VID_MOVIE)
   {
     p_pv_pview_render_immediate(pw);
     return;
   }
   pw->go_job_framenr = pw->stb_elem_refptr->from_frame;

   /* videothumb is rendered deferred (16 millisec) via timer */
   if(pw->go_timertag < 0)
   {
     pw->go_timertag = (gint32) g_timeout_add(16, (GtkFunction)p_pw_timer_go_job, pw);
   }

}  /* end p_pv_pview_render */

/* ---------------------------------
 * p_pw_preview_events_cb
 * ---------------------------------
 */
static gboolean
p_pw_preview_events_cb (GtkWidget *widget
                       , GdkEvent  *event
                       , GapStbPropWidget *pw)
{
  GdkEventExpose *eevent;
  GdkEventButton *bevent;
  GapStbMainGlobalParams  *sgpp;

  if ((pw->stb_elem_refptr == NULL)
  ||  (pw->stb_refptr == NULL))
  {
    /* the frame_widget is not initialized or it is just a dummy, no action needed */
    return FALSE;
  }
  sgpp = pw->sgpp;

  switch (event->type)
  {
    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;

      if((!pw->scene_detection_busy)
      && (!sgpp->gva_lock)
      && (pw->go_timertag < 0))
      {
        gap_story_pw_single_clip_playback(pw);
      }
      return FALSE;
      break;

    case GDK_EXPOSE:
      if(gap_debug) printf("p_pw_preview_events_cb GDK_EXPOSE widget:%d  da_wgt:%d\n"
                              , (int)widget
                              , (int)pw->pv_ptr->da_widget
                              );

      eevent = (GdkEventExpose *) event;

      if(widget == pw->pv_ptr->da_widget)
      {
        gap_pview_repaint(pw->pv_ptr);
        gdk_flush ();
      }

      break;

    default:
      break;
  }

  return FALSE;
}       /* end  p_pw_preview_events_cb */




/* ==================================================== START FILESEL stuff ======  */

/* --------------------------------
 * p_pw_check_ainfo_range
 * --------------------------------
 */
static void
p_pw_check_ainfo_range(GapStbPropWidget *pw, char *filename)
{
  GapAnimInfo *ainfo_ptr;
  gdouble l_lower;
  gdouble l_upper;
  gdouble l_val;

  if(gap_debug) printf("PROP AINFO CHECK\n");

  /* default: allow maximum range
   * (for movies we dont know the exactnumber of frames
   *  until the movie is read sequential until EOF
   *  that would be much to slow, must live with unconstrained range spinbuttons)
   */
  l_lower = 1;
  l_upper = 99999; /* default for unknown total_frames */

  if(pw->stb_elem_refptr->record_type == GAP_STBREC_VID_FRAMES)
  {
    ainfo_ptr = gap_lib_alloc_ainfo_from_name(filename, GIMP_RUN_NONINTERACTIVE);
    if(ainfo_ptr)
    {
      if(0== gap_lib_dir_ainfo(ainfo_ptr))
      {

         l_lower = MIN(ainfo_ptr->last_frame_nr, ainfo_ptr->first_frame_nr);
         l_upper = MAX(ainfo_ptr->last_frame_nr, ainfo_ptr->first_frame_nr);

      }
      gap_lib_free_ainfo(&ainfo_ptr);
    }
  }
  else
  {
    if(pw->stb_elem_refptr->record_type == GAP_STBREC_VID_MOVIE)
    {
      GapStoryVideoElem *velem;

      if(gap_debug) printf("PROP AINFO CHECK --> GAP_STBREC_VID_MOVIE\n");
      velem = gap_story_dlg_get_velem(pw->sgpp
                           ,pw->stb_elem_refptr->orig_filename
                           ,pw->stb_elem_refptr->seltrack
                           ,p_pw_get_preferred_decoder(pw)
                           );
      if(velem)
      {
        l_upper = velem->total_frames;
      }
    }
  }

  l_val = gtk_adjustment_get_value(GTK_ADJUSTMENT(pw->pw_spinbutton_from_adj));
  GTK_ADJUSTMENT(pw->pw_spinbutton_from_adj)->lower = l_lower;
  GTK_ADJUSTMENT(pw->pw_spinbutton_from_adj)->upper = l_upper;
  if((l_val < l_lower) || (l_val > l_upper))
  {
    gtk_adjustment_set_value(GTK_ADJUSTMENT(pw->pw_spinbutton_from_adj)
                            , CLAMP(l_val, l_lower, l_upper));
  }

  l_val = gtk_adjustment_get_value(GTK_ADJUSTMENT(pw->pw_spinbutton_to_adj));
  GTK_ADJUSTMENT(pw->pw_spinbutton_to_adj)->lower = l_lower;
  GTK_ADJUSTMENT(pw->pw_spinbutton_to_adj)->upper = l_upper;
  if((l_val < l_lower) || (l_val > l_upper))
  {
    gtk_adjustment_set_value(GTK_ADJUSTMENT(pw->pw_spinbutton_to_adj)
                            , CLAMP(l_val, l_lower, l_upper));
  }

}  /* end p_pw_check_ainfo_range */

/* --------------------------------
 * p_pw_filename_entry_update_cb
 * --------------------------------
 */
static void
p_pw_filename_entry_update_cb(GtkWidget *widget, GapStbPropWidget *pw)
{
  if(pw->scene_detection_busy)
  {
     return;
  }
  p_pw_filename_changed(gtk_entry_get_text(GTK_ENTRY(widget)), pw);
}  /* end p_pw_filename_entry_update_cb */


/* --------------------------------
 * p_pw_filename_changed
 * --------------------------------
 */
static void
p_pw_filename_changed(const char *filename, GapStbPropWidget *pw)
{
  if(pw == NULL)  { return; }
  if(pw->stb_elem_refptr == NULL)  { return; }

  pw->stb_refptr->unsaved_changes = TRUE;

  if(filename)
  {
    gint len;
    len = strlen(filename);
    if(len > 0)
    {
      gimp_set_data(GAP_STORY_PROPERTIES_PREVIOUS_FILENAME, filename, len+1);
    }
  }
  gap_story_upd_elem_from_filename(pw->stb_elem_refptr, filename);

  p_pw_check_ainfo_range(pw, pw->stb_elem_refptr->orig_filename);

  /* update pw stuff */
  p_pw_update_properties(pw);

}  /* end p_pw_filename_changed */


/* --------------------------------
 * p_filesel_pw_ok_cb
 * --------------------------------
 */
static void
p_filesel_pw_ok_cb (GtkWidget *widget
                   ,GapStbPropWidget *pw)
{
  const gchar *filename;
  gchar *dup_filename;

  if(pw == NULL) return;
  if(pw->pw_filesel == NULL) return;

  dup_filename = NULL;
  filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (pw->pw_filesel));
  if(filename)
  {
    if(g_file_test(filename, G_FILE_TEST_EXISTS))
    {
      /* defere p_pw_filename_changed until filesel widget has closed
       * (new videofiles opened for the 1.st time
       *  may take very long time for index creation)
       */
      dup_filename = g_strdup(filename);
    }
    else
    {
      gtk_entry_set_text(GTK_ENTRY(pw->pw_filename_entry), filename);
    }
  }

  gtk_widget_destroy(GTK_WIDGET(pw->pw_filesel));

  if(dup_filename)
  {
      gtk_entry_set_text(GTK_ENTRY(pw->pw_filename_entry), dup_filename);
      p_pw_filename_changed(dup_filename, pw);
      g_free(dup_filename);
  }

  pw->pw_filesel = NULL;
}  /* end p_filesel_pw_ok_cb */


/* -----------------------------
 * p_filesel_pw_close_cb
 * -----------------------------
 */
static void
p_filesel_pw_close_cb ( GtkWidget *widget
                      , GapStbPropWidget *pw)
{
  if(pw->pw_filesel == NULL) return;

  gtk_widget_destroy(GTK_WIDGET(pw->pw_filesel));
  pw->pw_filesel = NULL;   /* now filesel_story is closed */

}  /* end p_filesel_pw_close_cb */



/* -----------------------------
 * p_pw_filesel_button_cb
 * -----------------------------
 */
static void
p_pw_filesel_button_cb ( GtkWidget *w
                       , GapStbPropWidget *pw)
{
  GtkWidget *filesel = NULL;

  if(pw->scene_detection_busy)
  {
     return;
  }

  if(pw->pw_filesel != NULL)
  {
     gtk_window_present(GTK_WINDOW(pw->pw_filesel));
     return;   /* filesel is already open */
  }
  if(pw->stb_elem_refptr == NULL) { return; }

  filesel = gtk_file_selection_new ( _("Set Image or Frame Filename"));
  pw->pw_filesel = filesel;

  gtk_window_set_position (GTK_WINDOW (filesel), GTK_WIN_POS_MOUSE);
  g_signal_connect (GTK_FILE_SELECTION (filesel)->ok_button,
                    "clicked", G_CALLBACK (p_filesel_pw_ok_cb),
                    pw);
  g_signal_connect (GTK_FILE_SELECTION (filesel)->cancel_button,
                    "clicked", G_CALLBACK (p_filesel_pw_close_cb),
                    pw);
  g_signal_connect (filesel, "destroy",
                    G_CALLBACK (p_filesel_pw_close_cb),
                    pw);


  if(pw->stb_elem_refptr->orig_filename)
  {
    gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesel),
                                     pw->stb_elem_refptr->orig_filename);
  }
  else
  {
    gchar *previous_filename = NULL;
    gint name_length;

    /* for creating new clips we have no orig_filename in the properties
     * in this case we use the previous used name in this session
     * as default
     */

    name_length = gimp_get_data_size(GAP_STORY_PROPERTIES_PREVIOUS_FILENAME);
    if(name_length > 0)
    {
      previous_filename = g_malloc(name_length);
      gimp_get_data(GAP_STORY_PROPERTIES_PREVIOUS_FILENAME, previous_filename);

    }
    if(previous_filename)
    {
      gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesel),
                                     previous_filename);
      g_free(previous_filename);
    }
  }
  gtk_widget_show (filesel);

}  /* end p_pw_filesel_button_cb */


/* ==================================================== END FILESEL stuff ======  */



/* ----------------------------
 * p_pw_comment_entry_update_cb
 * ----------------------------
 */
static void
p_pw_comment_entry_update_cb(GtkWidget *widget, GapStbPropWidget *pw)
{
  if(pw == NULL) { return; }
  if(pw->stb_elem_refptr == NULL) { return; }
  if(pw->stb_refptr)
  {
    pw->stb_refptr->unsaved_changes = TRUE;
  }

  if(pw->stb_elem_refptr->comment == NULL)
  {
    pw->stb_elem_refptr->comment = gap_story_new_elem(GAP_STBREC_VID_COMMENT);
  }

  if(pw->stb_elem_refptr->comment)
  {
    if(pw->stb_elem_refptr->comment->orig_src_line)
    {
      g_free(pw->stb_elem_refptr->comment->orig_src_line);
    }
    pw->stb_elem_refptr->comment->orig_src_line = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));
  }
}  /* end p_pw_comment_entry_update_cb */


/* ----------------------------
 * p_pw_fmac_entry_update_cb
 * ----------------------------
 */
static void
p_pw_fmac_entry_update_cb(GtkWidget *widget, GapStbPropWidget *pw)
{
  if(pw == NULL) { return; }
  if(pw->stb_elem_refptr == NULL) { return; }
  if(pw->stb_refptr)
  {
    pw->stb_refptr->unsaved_changes = TRUE;
  }

  if(pw->stb_elem_refptr->filtermacro_file)
  {
    g_free(pw->stb_elem_refptr->filtermacro_file);
  }

  pw->stb_elem_refptr->filtermacro_file = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));
}  /* end p_pw_fmac_entry_update_cb */


/* ---------------------------------
 * p_pw_update_info_labels
 * ---------------------------------
 */
static void
p_pw_update_info_labels(GapStbPropWidget *pw)
{
  char    txt_buf[100];
  gdouble l_speed_fps;
  gboolean l_sensitive;
  gboolean l_mov_sensitive;
  GtkWidget  *scale;
  GtkWidget  *spinbutton;

  if(pw == NULL) { return; }
  if(pw->stb_elem_refptr == NULL) { return; }

  gap_story_elem_calculate_nframes(pw->stb_elem_refptr);

  l_sensitive = FALSE;
  l_mov_sensitive = FALSE;
  switch(pw->stb_elem_refptr->record_type)
  {
    case GAP_STBREC_VID_SILENCE:
      gtk_label_set_text ( GTK_LABEL(pw->cliptype_label), _("EMPTY"));
      break;
    case GAP_STBREC_VID_COLOR:
      gtk_label_set_text ( GTK_LABEL(pw->cliptype_label), _("COLOR"));
      break;
    case GAP_STBREC_VID_IMAGE:
      gtk_label_set_text ( GTK_LABEL(pw->cliptype_label), _("SINGLE-IMAGE"));
      break;
    case GAP_STBREC_VID_ANIMIMAGE:
      l_sensitive = TRUE;
      gtk_label_set_text ( GTK_LABEL(pw->cliptype_label), _("ANIM-IMAGE"));
      break;
    case GAP_STBREC_VID_FRAMES:
      l_sensitive = TRUE;
      gtk_label_set_text ( GTK_LABEL(pw->cliptype_label), _("FRAME-IMAGES"));
      break;
    case GAP_STBREC_VID_MOVIE:
      l_sensitive = TRUE;
      l_mov_sensitive = TRUE;
      gtk_label_set_text ( GTK_LABEL(pw->cliptype_label), _("MOVIE"));
      break;
    case GAP_STBREC_VID_COMMENT:
      gtk_label_set_text ( GTK_LABEL(pw->cliptype_label), _("COMMENT"));
      break;
    default:
      gtk_label_set_text ( GTK_LABEL(pw->cliptype_label), _("** UNKNOWN **"));
  }

  /* record_type dependent sensitivity */
  gtk_widget_set_sensitive(pw->pingpong_toggle, l_sensitive);
  spinbutton = GTK_WIDGET(g_object_get_data (G_OBJECT (pw->pw_spinbutton_from_adj), "spinbutton"));
  if(spinbutton)
  {
    gtk_widget_set_sensitive(spinbutton, l_sensitive);
  }
  scale = GTK_WIDGET(g_object_get_data (G_OBJECT (pw->pw_spinbutton_from_adj), "scale"));
  if(scale)
  {
    gtk_widget_set_sensitive(scale, l_sensitive);
  }
  spinbutton = GTK_WIDGET(g_object_get_data (G_OBJECT (pw->pw_spinbutton_to_adj), "spinbutton"));
  if(spinbutton)
  {
    gtk_widget_set_sensitive(spinbutton, l_sensitive);
  }
  scale = GTK_WIDGET(g_object_get_data (G_OBJECT (pw->pw_spinbutton_to_adj), "scale"));
  if(scale)
  {
    gtk_widget_set_sensitive(scale, l_sensitive);
  }

  spinbutton = GTK_WIDGET(g_object_get_data (G_OBJECT (pw->pw_spinbutton_step_density_adj), "spinbutton"));
  if(spinbutton)
  {
    gtk_widget_set_sensitive(spinbutton, l_sensitive);
  }
  scale = GTK_WIDGET(g_object_get_data (G_OBJECT (pw->pw_spinbutton_step_density_adj), "scale"));
  if(scale)
  {
    gtk_widget_set_sensitive(scale, l_sensitive);
  }


  spinbutton = GTK_WIDGET(g_object_get_data (G_OBJECT (pw->pw_spinbutton_seltrack_adj), "spinbutton"));
  if(spinbutton)
  {
    gtk_widget_set_sensitive(spinbutton, l_mov_sensitive);
  }
  scale = GTK_WIDGET(g_object_get_data (G_OBJECT (pw->pw_spinbutton_seltrack_adj), "scale"));
  if(scale)
  {
    gtk_widget_set_sensitive(scale, l_mov_sensitive);
  }

  g_snprintf(txt_buf, sizeof(txt_buf), _("%d (frames)")
            ,(int)pw->stb_elem_refptr->nframes
            );
  gtk_label_set_text ( GTK_LABEL(pw->dur_frames_label), txt_buf);

  l_speed_fps = GAP_STORY_DEFAULT_FRAMERATE;
  if(pw->stb_refptr)
  {
    if(pw->stb_refptr->master_framerate > 0)
    {
      l_speed_fps = pw->stb_refptr->master_framerate;
    }
  }
  gap_timeconv_framenr_to_timestr( pw->stb_elem_refptr->nframes
                         , l_speed_fps
                         , txt_buf
                         , sizeof(txt_buf)
                         );
  gtk_label_set_text ( GTK_LABEL(pw->dur_time_label), txt_buf);
}  /* end p_pw_update_info_labels */


/* ---------------------------------
 * p_pw_update_framenr_labels
 * ---------------------------------
 */
static void
p_pw_update_framenr_labels(GapStbPropWidget *pw, gint32 framenr)
{
  char    txt_buf[100];
  gdouble l_speed_fps;

  if(pw == NULL) { return; }
  if(pw->stb_elem_refptr == NULL) { return; }


  g_snprintf(txt_buf, sizeof(txt_buf), "%d  "
            ,(int)framenr
            );
  gtk_label_set_text ( GTK_LABEL(pw->pw_framenr_label), txt_buf);

  l_speed_fps = GAP_STORY_DEFAULT_FRAMERATE;
  if(pw->stb_refptr)
  {
    if(pw->stb_refptr->master_framerate > 0)
    {
      l_speed_fps = pw->stb_refptr->master_framerate;
    }
  }

  // TODO: can not show the exact frametime for
  // clips that do not start at frame 1

  gap_timeconv_framenr_to_timestr(framenr -1
                         , l_speed_fps
                         , txt_buf
                         , sizeof(txt_buf)
                         );
  gtk_label_set_text ( GTK_LABEL(pw->pw_frametime_label), txt_buf);
}  /* end p_pw_update_framenr_labels */


/* ---------------------------------
 * p_pw_update_properties
 * ---------------------------------
 * 1) update the info labels in the properties dialog window
 * 2) deferred update the storyboard table tabw in the master dialog window,
 *    where pw is atteched to.
 *    (immediate reflect the change of properties in the corresponding table.)
 */
static void
p_pw_update_properties(GapStbPropWidget *pw)
{
  p_pw_update_info_labels(pw);

  pw->go_render_all_request = TRUE;
  pw->go_job_framenr = pw->stb_elem_refptr->from_frame;
  if(pw->go_timertag < 0)
  {
    pw->go_timertag = (gint32) g_timeout_add(16, (GtkFunction)p_pw_timer_go_job, pw);
  }

}  /* end p_pw_update_properties */


/* ---------------------------------
 * p_pw_gint32_adjustment_callback
 * ---------------------------------
 */
static void
p_pw_gint32_adjustment_callback(GtkObject *obj, gint32 *val)
{
  GapStbPropWidget *pw;
  gint32 l_val;


  pw = g_object_get_data( G_OBJECT(obj), "pw" );
  if(pw)
  {
    if(pw->stb_elem_refptr)
    {
      l_val = RINT (GTK_ADJUSTMENT(obj)->value);
      if(gap_debug) printf("gint32_adjustment_callback: old_val:%d val:%d\n", (int)*val ,(int)l_val );
      if(l_val != *val)
      {
        *val = l_val;
        pw->stb_refptr->unsaved_changes = TRUE;
        p_pw_update_properties(pw);
      }
    }
  }

}  /* end p_pw_gint32_adjustment_callback */


/* ------------------------------------
 * p_pw_pingpong_toggle_update_callback
 * ------------------------------------
 */
static void
p_pw_pingpong_toggle_update_callback(GtkWidget *widget, GapStbPropWidget *pw)
{
  if(pw)
  {
    if(pw->stb_elem_refptr)
    {
      pw->stb_refptr->unsaved_changes = TRUE;

      if (GTK_TOGGLE_BUTTON (widget)->active)
      {
        pw->stb_elem_refptr->playmode = GAP_STB_PM_PINGPONG;
      }
      else
      {
        pw->stb_elem_refptr->playmode = GAP_STB_PM_NORMAL;
      }
      p_pw_update_properties(pw);
    }
  }
}  /* end p_pw_pingpong_toggle_update_callback */

/* ---------------------------------
 * p_radio_delace_none_callback
 * ---------------------------------
 */
static void
p_radio_delace_none_callback(GtkWidget *widget, GapStbPropWidget *pw)
{
  if((pw) && (GTK_TOGGLE_BUTTON (widget)->active))
  {
    pw->delace_mode = 0;
    gtk_widget_set_sensitive(pw->pw_spinbutton_delace, FALSE);
    if(pw->stb_elem_refptr)
    {
      pw->stb_elem_refptr->delace = pw->delace_mode + CLAMP(pw->delace_threshold, 0.0, 0.999999);
    }
  }
}  /* end p_radio_delace_none_callback */

/* ---------------------------------
 * p_radio_delace_odd_callback
 * ---------------------------------
 */
static void
p_radio_delace_odd_callback(GtkWidget *widget, GapStbPropWidget *pw)
{
  if((pw) && (GTK_TOGGLE_BUTTON (widget)->active))
  {
    pw->delace_mode = 1;
    gtk_widget_set_sensitive(pw->pw_spinbutton_delace, TRUE);
    if(pw->stb_elem_refptr)
    {
      pw->stb_elem_refptr->delace = pw->delace_mode + CLAMP(pw->delace_threshold, 0.0, 0.999999);
    }
  }
}  /* end p_radio_delace_odd_callback */

/* ---------------------------------
 * p_radio_delace_even_callback
 * ---------------------------------
 */
static void
p_radio_delace_even_callback(GtkWidget *widget, GapStbPropWidget *pw)
{
  if((pw) && (GTK_TOGGLE_BUTTON (widget)->active))
  {
    pw->delace_mode = 2;
    gtk_widget_set_sensitive(pw->pw_spinbutton_delace, TRUE);
    if(pw->stb_elem_refptr)
    {
      pw->stb_elem_refptr->delace = pw->delace_mode + CLAMP(pw->delace_threshold, 0.0, 0.999999);
    }
  }
}  /* end p_radio_delace_even_callback */

/* ---------------------------------
 * p_delace_spinbutton_cb
 * ---------------------------------
 */
static void
p_delace_spinbutton_cb(GtkObject *obj, GapStbPropWidget *pw)
{
  if(pw)
  {
    pw->delace_threshold = (GTK_ADJUSTMENT(pw->pw_spinbutton_delace_adj)->value);
    if(pw->stb_elem_refptr)
    {
      pw->stb_elem_refptr->delace = pw->delace_mode + CLAMP(pw->delace_threshold, 0.0, 0.999999);
    }
  }
}  /* end p_delace_spinbutton_cb */


/* ---------------------------------
 * p_step_density_spinbutton_cb
 * ---------------------------------
 */
static void
p_step_density_spinbutton_cb(GtkObject *obj, GapStbPropWidget *pw)
{
  gdouble l_val;
  if(pw)
  {
    l_val = (GTK_ADJUSTMENT(pw->pw_spinbutton_step_density_adj)->value);
    if(pw->stb_elem_refptr)
    {
      if(pw->stb_elem_refptr->step_density != l_val)
      {
        pw->stb_elem_refptr->step_density = l_val;
        gap_story_elem_calculate_nframes(pw->stb_elem_refptr);
        pw->stb_refptr->unsaved_changes = TRUE;
        p_pw_update_properties(pw);
      }
    }
  }
}  /* end p_step_density_spinbutton_cb */

/* -------------------------------
 * gap_story_pw_properties_dialog
 * -------------------------------
 */
GtkWidget *
gap_story_pw_properties_dialog (GapStbPropWidget *pw)
{
  GtkWidget *dlg;
  GtkWidget *main_vbox;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *entry;
  GtkWidget *check_button;
  GtkWidget *vbox2;
  GtkWidget *event_box;
  GtkWidget *hbox;
  gint       row;
  GtkObject *adj;
  GtkWidget *alignment;
  gint32     l_check_size;
  GapStbTabWidgets *tabw;
  gdouble           thumb_scale;

  if(pw == NULL) { return (NULL); }
  if(pw->pw_prop_dialog != NULL) { return(NULL); }   /* is already open */

  tabw = (GapStbTabWidgets *)pw->tabw;
  if(tabw == NULL) { return (NULL); }

  if(pw->stb_elem_bck)
  {
    dlg = gimp_dialog_new (_("Clip Properties"), "gap_story_clip_properties"
                         ,NULL, 0
                         ,gimp_standard_help_func, GAP_STORY_CLIP_PROP_HELP_ID

                         ,GIMP_STOCK_RESET, GAP_STORY_RESPONSE_RESET
                         ,_("Find Scene End"), GAP_STORY_RESPONSE_SCENE_END
                         ,_("Auto Scene Split"), GAP_STORY_RESPONSE_SCENE_SPLIT
                         ,GTK_STOCK_CLOSE,  GTK_RESPONSE_CLOSE
                         ,NULL);
  }
  else
  {
    dlg = gimp_dialog_new (_("Clip Properties"), "gap_story_clip_properties"
                         ,NULL, 0
                         ,gimp_standard_help_func, GAP_STORY_CLIP_PROP_HELP_ID

                         ,_("Find Scene End"), GAP_STORY_RESPONSE_SCENE_END
                         ,_("Auto Scene Split"), GAP_STORY_RESPONSE_SCENE_SPLIT
                         ,GTK_STOCK_CLOSE,  GTK_RESPONSE_CLOSE
                         ,NULL);
  }

  pw->pw_prop_dialog = dlg;

  g_signal_connect (G_OBJECT (dlg), "response",
                    G_CALLBACK (p_pw_prop_response),
                    pw);


  main_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dlg)->vbox), main_vbox);

  /*  the frame  */
  frame = gimp_frame_new (_("Properties"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (15, 4, FALSE);
  pw->master_table = table;
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  row = 0;

  /* the cliptype label */
  label = gtk_label_new (_("Clip Type:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE(table), label, 0, 1, row, row+1);
  gtk_widget_show (label);

  /* the cliptype label content */
  label = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE(table), label, 1, 3, row, row+1);
  gtk_widget_show (label);
  pw->cliptype_label = label;

  row++;

  /* the filename label */
  label = gtk_label_new (_("File:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE(table), label, 0, 1, row, row+1);
  gtk_widget_show (label);

  /* the filename entry */
  entry = gtk_entry_new ();
  gtk_widget_set_size_request(entry, PW_ENTRY_WIDTH, -1);
  if(pw->stb_elem_refptr)
  {
    if(pw->stb_elem_refptr->orig_filename)
    {
      gtk_entry_set_text(GTK_ENTRY(entry), pw->stb_elem_refptr->orig_filename);
    }
  }
  gtk_table_attach_defaults (GTK_TABLE(table), entry, 1, 2, row, row+1);
  g_signal_connect(G_OBJECT(entry), "changed",
                     G_CALLBACK(p_pw_filename_entry_update_cb),
                     pw);
  gtk_widget_show (entry);
  pw->pw_filename_entry = entry;

  /* the filesel invoker button */
  button = gtk_button_new_with_label ("...");
  gtk_table_attach_defaults (GTK_TABLE(table), button, 2, 3, row, row+1);
  g_signal_connect(G_OBJECT(button), "clicked",
                     G_CALLBACK(p_pw_filesel_button_cb),
                     pw);
  gtk_widget_show (button);


  row++;

  /* the from spinbutton */
  adj = gimp_scale_entry_new (GTK_TABLE (pw->master_table), 0, row++,
                              _("From:"), PW_SCALE_WIDTH, PW_SPIN_BUTTON_WIDTH,
                              pw->stb_elem_refptr->from_frame,
                              1.0, 999999.0,  /* lower/upper */
                              1.0, 10.0,      /* step, page */
                              0,              /* digits */
                              TRUE,           /* constrain */
                              1.0, 999999.0,  /* lower/upper unconstrained */
                              _("framenumber of the first frame "
                                "in the clip range"), NULL);
  pw->pw_spinbutton_from_adj = adj;
  g_object_set_data(G_OBJECT(adj), "pw", pw);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (p_pw_gint32_adjustment_callback),
                    &pw->stb_elem_refptr->from_frame);
  {
    GtkWidget *scale;

    scale = GTK_WIDGET(g_object_get_data (G_OBJECT (pw->pw_spinbutton_from_adj), "scale"));
    if(scale)
    {
      gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
    }
  }


  row++;

  /* the to spinbutton */
  adj = gimp_scale_entry_new (GTK_TABLE (pw->master_table), 0, row++,
                              _("To:"), PW_SCALE_WIDTH, PW_SPIN_BUTTON_WIDTH,
                              pw->stb_elem_refptr->to_frame,
                              1.0, 999999.0,  /* lower/upper */
                              1.0, 10.0,      /* step, page */
                              0,              /* digits */
                              TRUE,           /* constrain */
                              1.0, 999999.0,  /* lower/upper unconstrained */
                              _("framenumber of the last frame "
                                "in the clip range"), NULL);
  pw->pw_spinbutton_to_adj = adj;
  g_object_set_data(G_OBJECT(adj), "pw", pw);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (p_pw_gint32_adjustment_callback),
                    &pw->stb_elem_refptr->to_frame);


  row++;
  /* the loops spinbutton */
  adj = gimp_scale_entry_new (GTK_TABLE (pw->master_table), 0, row++,
                              _("Loops:"), PW_SCALE_WIDTH, PW_SPIN_BUTTON_WIDTH,
                              pw->stb_elem_refptr->nloop,
                              1.0, 1000.0,  /* lower/upper */
                              1.0, 10.0,      /* step, page */
                              0,              /* digits */
                              TRUE,           /* constrain */
                              1.0, 1000.0,  /* lower/upper unconstrained */
                              _("number of loops "
                                "(how often to play the framerange)"), NULL);
  pw->pw_spinbutton_loops_adj = adj;
  g_object_set_data(G_OBJECT(adj), "pw", pw);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (p_pw_gint32_adjustment_callback),
                    &pw->stb_elem_refptr->nloop);


  row++;
  /* pingpong */
  label = gtk_label_new(_("Pingpong:"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE (table), label, 0, 1, row, row + 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);


  /* check button */
  check_button = gtk_check_button_new_with_label (" ");
  gtk_table_attach ( GTK_TABLE (table), check_button, 1, 2, row, row+1, GTK_FILL, 0, 0, 0);
  {
    gboolean pingpong_state;

    pingpong_state = (pw->stb_elem_refptr->playmode == GAP_STB_PM_PINGPONG);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button), pingpong_state);

  }
  gimp_help_set_help_data(check_button, _("ON: Play clip in pingpong mode"), NULL);
  gtk_widget_show(check_button);
  pw->pingpong_toggle = check_button;
  g_signal_connect (G_OBJECT (check_button), "toggled",
                    G_CALLBACK (p_pw_pingpong_toggle_update_callback),
                    pw);



  row++;
  /* the step_density spinbutton */
  adj = gimp_scale_entry_new (GTK_TABLE (pw->master_table), 0, row++,
                              _("Stepsize:"), PW_SCALE_WIDTH, PW_SPIN_BUTTON_WIDTH,
                              pw->stb_elem_refptr->step_density,
                              0.125, 8.0,     /* lower/upper */
                              0.125, 0.5,     /* step, page */
                              6,              /* digits */
                              TRUE,           /* constrain */
                              0.125, 8.0,     /* lower/upper unconstrained */
                              _("Stepsize density. Use 1.0 for normal 1:1 frame by frame steps. "
                                "a value of 0.5 shows each input frame 2 times. "
                                "a value of 2.0 shows only every 2.nd input frame"), NULL);
  pw->pw_spinbutton_step_density_adj = adj;
  g_object_set_data(G_OBJECT(adj), "pw", pw);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (p_step_density_spinbutton_cb),
                    pw);



  row++;
  /* the seltrack spinbutton */
  adj = gimp_scale_entry_new (GTK_TABLE (pw->master_table), 0, row++,
                              _("Videotrack:"), PW_SCALE_WIDTH, PW_SPIN_BUTTON_WIDTH,
                              pw->stb_elem_refptr->seltrack,
                              1.0, 100.0,  /* lower/upper */
                              1.0, 10.0,      /* step, page */
                              0,              /* digits */
                              TRUE,           /* constrain */
                              1.0, 100.0,  /* lower/upper unconstrained */
                              _("select input videotrack "
                                "(most videofiles have just 1 track)"), NULL);
  pw->pw_spinbutton_seltrack_adj = adj;
  g_object_set_data(G_OBJECT(adj), "pw", pw);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (p_pw_gint32_adjustment_callback),
                    &pw->stb_elem_refptr->seltrack);

  row++;

  /* the Deinterlace Mode label */
  label = gtk_label_new (_("Deinterlace:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE(table), label, 0, 1, row, row+1);
  gtk_widget_show (label);

  /* delace mode radio button group */
  {
    GSList    *radio_group = NULL;
    GtkWidget *radio_table;
    GtkWidget *radio_button;
    gint      l_idx;
    gboolean  l_radio_pressed;

    /* radio_table */
    radio_table = gtk_table_new (1, 3, FALSE);

    l_idx = 0;
    /* radio button delace_mode None */
    radio_button = gtk_radio_button_new_with_label ( radio_group, _("None") );
    radio_group = gtk_radio_button_get_group ( GTK_RADIO_BUTTON (radio_button) );
    gtk_table_attach ( GTK_TABLE (radio_table), radio_button, l_idx, l_idx+1
                     , 0, 1, GTK_FILL | GTK_EXPAND, 0, 0, 0);

    l_radio_pressed = (pw->delace_mode == 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_button),
                                     l_radio_pressed);
    gimp_help_set_help_data(radio_button, _("Read videoframes 1:1 without de-interlace filter"), NULL);

    gtk_widget_show (radio_button);
    g_signal_connect ( G_OBJECT (radio_button), "toggled",
                       G_CALLBACK (p_radio_delace_none_callback),
                       pw);

    l_idx++;
    /* radio button delace_mode odd */
    radio_button = gtk_radio_button_new_with_label ( radio_group, _("Odd") );
    radio_group = gtk_radio_button_get_group ( GTK_RADIO_BUTTON (radio_button) );
    gtk_table_attach ( GTK_TABLE (radio_table), radio_button, l_idx, l_idx+1
                     , 0, 1, GTK_FILL | GTK_EXPAND, 0, 0, 0);

    l_radio_pressed = (pw->delace_mode == 1);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_button),
                                     l_radio_pressed);
    gimp_help_set_help_data(radio_button, _("Apply odd-lines filter when reading videoframes"), NULL);

    gtk_widget_show (radio_button);
    g_signal_connect ( G_OBJECT (radio_button), "toggled",
                       G_CALLBACK (p_radio_delace_odd_callback),
                       pw);

    l_idx++;
    /* radio button delace_mode even */
    radio_button = gtk_radio_button_new_with_label ( radio_group, _("Even") );
    radio_group = gtk_radio_button_get_group ( GTK_RADIO_BUTTON (radio_button) );
    gtk_table_attach ( GTK_TABLE (radio_table), radio_button, l_idx, l_idx+1
                     , 0, 1, GTK_FILL | GTK_EXPAND, 0, 0, 0);

    l_radio_pressed = (pw->delace_mode == 2);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_button),
                                     l_radio_pressed);
    gimp_help_set_help_data(radio_button, _("Apply even-lines filter when reading videoframes"), NULL);

    gtk_widget_show (radio_button);
    g_signal_connect ( G_OBJECT (radio_button), "toggled",
                       G_CALLBACK (p_radio_delace_even_callback),
                       pw);

    gtk_widget_show (radio_table);
    gtk_table_attach_defaults (GTK_TABLE(table), radio_table, 1, 2, row, row+1);
  }

  /* delace threshold spinbutton */
  {
    GtkWidget *spinbutton;

    pw->pw_spinbutton_delace_adj = gtk_adjustment_new (pw->delace_threshold
                                           , 0.0
                                           , 0.999
                                           , 0.1, 0.1, 0.0);
    spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (pw->pw_spinbutton_delace_adj), 0.1, 3);
    pw->pw_spinbutton_delace = spinbutton;
    gtk_widget_set_size_request (spinbutton, PW_SPIN_BUTTON_WIDTH, -1);
    gtk_widget_show (spinbutton);
    gtk_table_attach_defaults (GTK_TABLE(table), spinbutton, 2, 3, row, row+1);

    gimp_help_set_help_data (spinbutton, _("deinterlacing threshold: "
                                           "0.0 no interpolation "
                                           "0.999 smooth interpolation")
                                           , NULL);
    g_signal_connect (G_OBJECT (pw->pw_spinbutton_delace_adj), "value_changed",
                      G_CALLBACK (p_delace_spinbutton_cb),
                      pw);
    if(pw->delace_mode == 0)
    {
      gtk_widget_set_sensitive(pw->pw_spinbutton_delace, FALSE);
    }
  }

  row++;


  /* the duration label */
  label = gtk_label_new (_("Duration:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE(table), label, 0, 1, row, row+1);
  gtk_widget_show (label);

  /* the frame duration label */
  label = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE(table), label, 1, 2, row, row+1);
  gtk_widget_show (label);
  pw->dur_frames_label = label;

  /* the time duration label  */
  label = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE(table), label, 2, 3, row, row+1);
  gtk_widget_show (label);
  pw->dur_time_label = label;


  /* the vox2  */
  vbox2 = gtk_vbox_new (FALSE, 1);

  /* Create an EventBox (container for the preview drawing_area)
   */
  event_box = gtk_event_box_new ();
  gtk_container_add (GTK_CONTAINER (event_box), vbox2);
  gtk_widget_set_events (event_box, GDK_BUTTON_PRESS_MASK);
  g_signal_connect (G_OBJECT (event_box), "button_press_event",
                    G_CALLBACK (p_pw_preview_events_cb),
                    pw);

  gtk_table_attach_defaults (GTK_TABLE(table), event_box, 3, 4, 0, row+1);

  /*  The thumbnail preview  */
  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (vbox2), alignment, FALSE, FALSE, 1);
  gtk_widget_show (alignment);

  /* thumbnail 3 times bigger */
  thumb_scale = 1.0;
  if(tabw->thumbsize < 256)
  {
    thumb_scale = 256 / MAX(tabw->thumbsize, 1);
  }
  l_check_size = (thumb_scale * tabw->thumbsize) / 16;
  pw->pv_ptr = gap_pview_new( (thumb_scale * (gdouble)tabw->thumb_width) + 4.0
                            , (thumb_scale * (gdouble)tabw->thumb_height) + 4.0
                            , l_check_size
                            , NULL   /* no aspect_frame is used */
                            );

  gtk_widget_set_events (pw->pv_ptr->da_widget, GDK_EXPOSURE_MASK);
  gtk_container_add (GTK_CONTAINER (alignment), pw->pv_ptr->da_widget);
  gtk_widget_show (pw->pv_ptr->da_widget);
  g_signal_connect (G_OBJECT (pw->pv_ptr->da_widget), "event",
                    G_CALLBACK (p_pw_preview_events_cb),
                    pw);

  hbox = gtk_hbox_new (FALSE, 1);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 1);

  /* the framenr label  */
  label = gtk_label_new ("000000");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 1);
  gtk_widget_show (label);
  pw->pw_framenr_label = label;

  /* the framenr label  */
  label = gtk_label_new ("mm:ss:msec");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 1);
  gtk_widget_show (label);
  pw->pw_frametime_label = label;



  gtk_widget_show (vbox2);
  gtk_widget_show (event_box);





  row++;

  /* the comment label */
  label = gtk_label_new (_("Comment:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE(table), label, 0, 1, row, row+1);
  gtk_widget_show (label);


  /* the comment entry */
  entry = gtk_entry_new ();
  gtk_widget_set_size_request(entry, PW_COMMENT_WIDTH, -1);
  if(pw->stb_elem_refptr)
  {
    if(pw->stb_elem_refptr->comment)
    {
      if(pw->stb_elem_refptr->comment->orig_src_line)
      {
        gtk_entry_set_text(GTK_ENTRY(entry), pw->stb_elem_refptr->comment->orig_src_line);
      }
    }
  }
  gtk_table_attach_defaults (GTK_TABLE(table), entry, 1, 4, row, row+1);
  g_signal_connect(G_OBJECT(entry), "changed",
                     G_CALLBACK(p_pw_comment_entry_update_cb),
                     pw);
  gtk_widget_show (entry);
  pw->comment_entry = entry;



  row++;

  /* the filtermacro label */
  label = gtk_label_new (_("Filtermacro:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE(table), label, 0, 1, row, row+1);
  gtk_widget_show (label);



  /* the filtermacro entry */
  entry = gtk_entry_new ();
  gtk_widget_set_size_request(entry, PW_ENTRY_WIDTH, -1);
  if(pw->stb_elem_refptr)
  {
    if(pw->stb_elem_refptr->filtermacro_file)
    {
      gtk_entry_set_text(GTK_ENTRY(entry), pw->stb_elem_refptr->filtermacro_file);
    }
  }
  gtk_table_attach_defaults (GTK_TABLE(table), entry, 1, 4, row, row+1);
  g_signal_connect(G_OBJECT(entry), "changed",
                     G_CALLBACK(p_pw_fmac_entry_update_cb),
                     pw);
  gtk_widget_show (entry);
  pw->fmac_entry = entry;




  if(pw->stb_elem_refptr)
  {
    if((pw->stb_elem_refptr->record_type == GAP_STBREC_VID_FRAMES)
    || (pw->stb_elem_refptr->record_type == GAP_STBREC_VID_MOVIE))
    {
      /* for framerange clips constraint from / to spinbuttons */
      p_pw_check_ainfo_range(pw, pw->stb_elem_refptr->orig_filename);
    }
  }



  /*  Show the main containers  */

  gtk_widget_show (main_vbox);

  return(dlg);
}  /* end gap_story_pw_properties_dialog */

/* -------------------------------
 * gap_story_stb_elem_properties_dialog
 * -------------------------------
 */
void
gap_story_stb_elem_properties_dialog ( GapStbTabWidgets *tabw
                                     , GapStoryElem *stb_elem
                                     , GapStoryBoard *stb_dst)
{
  GapStbPropWidget *pw;

  /* check if already open */
  for(pw=tabw->pw; pw!=NULL; pw=(GapStbPropWidget *)pw->next)
  {
    if(pw->stb_elem_refptr->story_id == stb_elem->story_id)
    {
      if(pw->pw_prop_dialog)
      {
        /* Properties for the selected element already open
         * bring the window to front
         */
        gtk_window_present(GTK_WINDOW(pw->pw_prop_dialog));
        return ;
      }
      /* we found a dead element (that is already closed)
       * reuse that element to open a new clip properties dialog window
       */
      break;
    }
  }

  if(pw==NULL)
  {
    pw = g_new(GapStbPropWidget ,1);
    pw->next = tabw->pw;
    tabw->pw = pw;
    pw->stb_elem_bck = NULL;
  }
  if(pw->stb_elem_bck)
  {
    gap_story_elem_free(&pw->stb_elem_bck);
  }
  pw->stb_elem_bck = NULL;
  pw->pw_prop_dialog = NULL;
  pw->pw_filesel = NULL;
  pw->stb_elem_refptr = stb_elem;
  pw->stb_refptr = stb_dst;
  pw->sgpp = tabw->sgpp;
  pw->tabw = tabw;
  pw->go_render_all_request = FALSE;
  pw->go_job_framenr = -1;
  pw->go_timertag = -1;
  pw->scene_detection_busy = FALSE;
  pw->close_flag = FALSE;
  pw->delace_mode = stb_elem->delace;
  pw->delace_threshold = stb_elem->delace - (gdouble)pw->delace_mode;

  if(stb_elem)
  {
    if((stb_elem->orig_filename)
    || (stb_elem->basename))
    {
      pw->stb_elem_bck = gap_story_elem_duplicate(stb_elem);
      if(stb_elem->comment)
      {
        if(stb_elem->comment->orig_src_line)
        {
          pw->stb_elem_bck->comment = gap_story_elem_duplicate(stb_elem->comment);
        }
      }
    }
  }


  pw->pw_prop_dialog = gap_story_pw_properties_dialog(pw);
  if(pw->pw_prop_dialog)
  {
    gtk_widget_show(pw->pw_prop_dialog);
    p_pw_update_info_labels(pw);
    p_pv_pview_render(pw);
  }
}  /* end gap_story_stb_elem_properties_dialog */

/* -------------------------------
 * gap_story_fw_properties_dialog
 * -------------------------------
 */
void
gap_story_fw_properties_dialog (GapStbFrameWidget *fw)
{
  GapStbTabWidgets *tabw;

  if(fw == NULL) { return; }
  if(fw->stb_elem_refptr == NULL)  { return; }

  tabw = (GapStbTabWidgets *)fw->tabw;
  if(tabw == NULL)  { return; }


  /* type check, this dialog handles only stb_elements that are video clips */
  if(gap_story_elem_is_video(fw->stb_elem_refptr))
  {
    gap_story_stb_elem_properties_dialog(tabw, fw->stb_elem_refptr, fw->stb_refptr);
  }
}  /* end gap_story_fw_properties_dialog */

