/*  gap_story_dialog.c
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* revision history:
 * version 1.3.27a; 2004/03/15  hof: videothumbnails are kept in memory
 *                                   for the startframes in all MOVIE clips
 *                                   (common list for both storyboard and cliplist)
 *                                   Need define GAP_ENABLE_VIDEOAPI_SUPPORT
 *                                   to handle thumbnails for cliptype GAP_STBREC_VID_MOVIE
 * version 1.3.25a; 2004/02/22  hof: created
 */


#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gap_story_main.h"
#include "gap_story_dialog.h"
#include "gap_story_file.h"
#include "gap_player_dialog.h"
#include "gap_pdb_calls.h"
#include "gap_pview_da.h"
#include "gap_stock.h"
#include "gap_lib.h"
#include "gap_image.h"
#include "gap_vin.h"
#include "gap_timeconv.h"
#include "gap_thumbnail.h"
#include "gap_arr_dialog.h"


#include "gap-intl.h"

#define GAP_STB_PLAYER_HELP_ID    "plug-in-gap-storyboard-player"


extern int gap_debug;  /* 1 == print debug infos , 0 dont print debug infos */
static gint32 global_stb_video_id = 0;

typedef void (*GapStbMenuCallbackFptr)(GtkWidget *widget, GapStbMainGlobalParams *sgpp);


static GapStoryBoard *p_tabw_get_stb_ptr (GapStbTabWidgets *tabw);
static GapStbTabWidgets *  p_new_stb_tab_widgets(GapStbMainGlobalParams *sgpp
                                                , GapStoryMasterType type);
static void     p_render_all_frame_widgets (GapStbTabWidgets *tabw);

static void     p_frame_widget_init_empty (GapStbTabWidgets *tabw, GapStbFrameWidget *fw);
static void     p_frame_widget_render (GapStbFrameWidget *fw);
static void     p_frame_widget_destroy(GapStbFrameWidget *fw);

static gint32   p_get_max_rowpage(gint32 act_elems, gint32 cols, gint rows);

static void     p_story_set_range_cb(GapPlayerAddClip *plac_ptr);

static void     p_story_call_player(GapStbMainGlobalParams *sgpp
                   , GapStoryBoard *stb
		   , char *imagename
		   , gint32 imagewidth
		   , gint32 imageheight
		   , gint32 image_id
		   , gint32 begin_frame
		   , gint32 end_frame
		   , gboolean play_all
		   , gint32 seltrack
		   , gdouble delace
		   , const char *preferred_decoder
		   );
static void     p_call_master_encoder(GapStbMainGlobalParams *sgpp
                   , GapStoryBoard *stb
		   );



static gboolean p_cliplist_reload (GapStbMainGlobalParams *sgpp);
static gboolean p_storyboard_reload (GapStbMainGlobalParams *sgpp);

static void     p_player_cll_mode_cb (GtkWidget *w, GdkEventButton  *bevent, GapStbMainGlobalParams *sgpp);
static void     p_player_stb_mode_cb (GtkWidget *w, GdkEventButton  *bevent, GapStbMainGlobalParams *sgpp);
static void     p_player_img_mode_cb (GtkWidget *w, GapStbMainGlobalParams *sgpp);
static void     p_cancel_button_cb (GtkWidget *w, GapStbMainGlobalParams *sgpp);

static void     p_tabw_master_prop_dialog(GapStbTabWidgets *tabw);
static void     p_tabw_add_elem (GapStbTabWidgets *tabw, GapStbMainGlobalParams *sgpp, GapStoryBoard *stb_dst);


static void     p_filesel_tabw_close_cb ( GtkWidget *widget, GapStbTabWidgets *tabw);
static gboolean p_filesel_tabw_ok (GtkWidget *widget, GapStbTabWidgets *tabw);
static void     p_filesel_tabw_load_ok_cb (GtkWidget *widget, GapStbTabWidgets *tabw);
static void     p_filesel_tabw_save_ok_cb ( GtkWidget *widget, GapStbTabWidgets *tabw);
static void     p_tabw_load_file_cb ( GtkWidget *w, GapStbTabWidgets *tabw);
static void     p_tabw_file_save_as_cb ( GtkWidget *w, GapStbTabWidgets *tabw);
static void     p_tabw_filename_entry_cb(GtkWidget *widget, GapStbTabWidgets *tabw);
static void     p_rowpage_spinbutton_cb(GtkEditable     *editable,
                                     GapStbTabWidgets *tabw);
static void     p_vscale_changed_cb(GtkObject *adj, GapStbTabWidgets *tabw);
static void     p_single_clip_playback(GapStbFrameWidget *fw);
static gboolean p_frame_widget_preview_events_cb (GtkWidget *widget,
                             GdkEvent  *event,
                             GapStbFrameWidget *fw);
static void     p_tabw_destroy_properties_dlg (GapStbTabWidgets *tabw, gboolean destroy_all);
static void     p_tabw_edit_cut_cb (GtkWidget *w, GapStbTabWidgets *tabw);
static void     p_tabw_edit_copy_cb (GtkWidget *w, GapStbTabWidgets *tabw);
static void     p_tabw_edit_paste_cb (GtkWidget *w, GapStbTabWidgets *tabw);
static void     p_tabw_edit_paste_at_story_id (GapStbMainGlobalParams  *sgpp
                    , GapStoryBoard *stb_dst
		    , GapStbTabWidgets *tabw
		    , gint32 story_id
		    , gboolean insert_after);

static void     p_tabw_update_selection(GapStbTabWidgets  *tabw);
static void     p_selection_add(GapStbFrameWidget *fw);
static void     p_selection_extend(GapStbFrameWidget *fw);
static void     p_selection_replace(GapStbFrameWidget *fw);

static void     p_tabw_sensibility (GapStbMainGlobalParams *sgpp
                   ,GapStoryBoard *stb
		   ,GapStbTabWidgets *tabw);
static void     p_widget_sensibility (GapStbMainGlobalParams *sgpp);


static gboolean p_close_one_or_both_lists(GapStbMainGlobalParams *sgpp
                         ,gboolean check_cliplist
			 ,gboolean check_storyboard
			 );

static void     p_menu_win_quit_cb (GtkWidget *widget, GapStbMainGlobalParams *sgpp);
static void     p_menu_win_help_cb (GtkWidget *widget, GapStbMainGlobalParams *sgpp);
static void     p_menu_win_properties_cb (GtkWidget *widget, GapStbMainGlobalParams *sgpp);
static void     p_menu_win_vthumbs_toggle_cb (GtkWidget *widget, GapStbMainGlobalParams *sgpp);

static void     p_menu_cll_new_cb (GtkWidget *widget, GapStbMainGlobalParams *sgpp);
static void     p_menu_cll_open_cb (GtkWidget *widget, GapStbMainGlobalParams *sgpp);
static void     p_menu_cll_save_cb (GtkWidget *widget, GapStbMainGlobalParams *sgpp);
static void     p_menu_cll_save_cb_as (GtkWidget *widget, GapStbMainGlobalParams *sgpp);
static void     p_menu_cll_playback_cb (GtkWidget *widget, GapStbMainGlobalParams *sgpp);
static void     p_menu_cll_properties_cb (GtkWidget *widget, GapStbMainGlobalParams *sgpp);
static void     p_menu_cll_add_elem_cb (GtkWidget *widget, GapStbMainGlobalParams *sgpp);
static void     p_menu_cll_toggle_edmode (GtkWidget *widget, GapStbMainGlobalParams *sgpp);
static void     p_menu_cll_encode_cb (GtkWidget *widget, GapStbMainGlobalParams *sgpp);
static void     p_menu_cll_close_cb (GtkWidget *widget, GapStbMainGlobalParams *sgpp);

static void     p_menu_stb_new_cb (GtkWidget *widget, GapStbMainGlobalParams *sgpp);
static void     p_menu_stb_open_cb (GtkWidget *widget, GapStbMainGlobalParams *sgpp);
static void     p_menu_stb_save_cb (GtkWidget *widget, GapStbMainGlobalParams *sgpp);
static void     p_menu_stb_save_cb_as (GtkWidget *widget, GapStbMainGlobalParams *sgpp);
static void     p_menu_stb_playback_cb (GtkWidget *widget, GapStbMainGlobalParams *sgpp);
static void     p_menu_stb_properties_cb (GtkWidget *widget, GapStbMainGlobalParams *sgpp);
static void     p_menu_stb_add_elem_cb (GtkWidget *widget, GapStbMainGlobalParams *sgpp);
static void     p_menu_stb_toggle_edmode (GtkWidget *widget, GapStbMainGlobalParams *sgpp);
static void     p_menu_stb_encode_cb (GtkWidget *widget, GapStbMainGlobalParams *sgpp);
static void     p_menu_stb_close_cb (GtkWidget *widget, GapStbMainGlobalParams *sgpp);


static GtkWidget*  p_make_menu_bar_item(GtkWidget *menu_bar, gchar *label);
static GtkWidget*  p_make_item_with_image(GtkWidget *parent, const gchar *stock_id, 
		     GapStbMenuCallbackFptr fptr, GapStbMainGlobalParams *sgpp);
static GtkWidget*  p_make_item_with_label(GtkWidget *parent, gchar *label, 
		     GapStbMenuCallbackFptr fptr, GapStbMainGlobalParams *sgpp);
static GtkWidget*  p_make_check_item_with_label(GtkWidget *parent, gchar *label, 
		     GapStbMenuCallbackFptr fptr, GapStbMainGlobalParams *sgpp
		     ,gboolean initial_value);

static void        p_make_menu_global(GapStbMainGlobalParams *sgpp, GtkWidget *menu_bar);
static void        p_make_menu_cliplist(GapStbMainGlobalParams *sgpp, GtkWidget *menu_bar);
static void        p_make_menu_storyboard(GapStbMainGlobalParams *sgpp, GtkWidget *menu_bar);
static void        p_make_menu_help(GapStbMainGlobalParams *sgpp, GtkWidget *menu_bar);

static gboolean    p_check_unsaved_changes (GapStbMainGlobalParams *sgpp
                           , gboolean check_cliplist
			   , gboolean check_storyboard
			   );
static void        p_tabw_update_frame_label (GapStbTabWidgets *tabw
                           , GapStbMainGlobalParams *sgpp
			   );


static gboolean    p_vid_progress_callback(gdouble progress
                       ,gpointer user_data
                       );
static void        p_close_videofile(GapStbMainGlobalParams *sgpp);
static void        p_open_videofile(GapStbMainGlobalParams *sgpp
                     , const char *filename
		     , gint32 seltrack
		     , const char *preferred_decoder
		     );
static guchar *    p_fetch_videoframe(GapStbMainGlobalParams *sgpp
                   , const char *gva_videofile
		   , gint32 framenumber
		   , gint32 seltrack
		   , const char *preferred_decoder
		   , gdouble delace
		   , gint32 *th_bpp
		   , gint32 *th_width
		   , gint32 *th_height
		   );
GapVThumbElem *   p_fetch_vthumb_elem(GapStbMainGlobalParams *sgpp
        	   ,const char *video_filename
		   ,gint32 framenr
		   ,gint32 seltrack
		   ,const char *preferred_decoder
		   );



static void    story_dlg_response (GtkWidget *widget,
                 gint       response_id,
                 GapStbMainGlobalParams *sgpp);


static void     p_recreate_tab_widgets(GapStoryBoard *stb
                       ,GapStbTabWidgets *tabw
		       ,gint32 mount_col
		       ,gint32 mount_row
		       ,GapStbMainGlobalParams *sgpp
		      );
static void     p_create_button_bar(GapStbTabWidgets *tabw
	           ,gint32 mount_col
		   ,gint32 mount_row
                   ,GapStbMainGlobalParams *sgpp
		   ,gboolean with_open_and_save
	           ,gint32 mount_vs_col
		   ,gint32 mount_vs_row
		   );


/* -----------------------------
 * gap_story_dlg_pw_render_all
 * -----------------------------
 */
void 
gap_story_dlg_pw_render_all(GapStbPropWidget *pw)
{
  if(pw == NULL) { return; }
  if(pw->tabw == NULL) { return; }
  
  p_render_all_frame_widgets(pw->tabw);
  p_tabw_update_frame_label(pw->tabw, pw->sgpp);
}  /* end gap_story_dlg_pw_render_all */

/* -----------------------------
 * p_tabw_get_stb_ptr
 * -----------------------------
 */
static GapStoryBoard *
p_tabw_get_stb_ptr (GapStbTabWidgets *tabw)
{
  GapStoryBoard *l_stb;
  GapStbMainGlobalParams *sgpp;

  if(tabw == NULL) { return(NULL); }
  
  sgpp = (GapStbMainGlobalParams *)tabw->sgpp;
  if(sgpp == NULL)  { return(NULL); }

  l_stb = sgpp->stb;
  if(tabw->type == GAP_STB_MASTER_TYPE_CLIPLIST)
  {
    l_stb = sgpp->cll;
  }
  return(l_stb);

}  /* end p_tabw_get_stb_ptr */ 


/* -----------------------------
 * p_new_stb_tab_widgets
 * -----------------------------
 */
static GapStbTabWidgets *
p_new_stb_tab_widgets(GapStbMainGlobalParams *sgpp, GapStoryMasterType type)
{
  GapStbTabWidgets *tabw;
  
  tabw = g_new(GapStbTabWidgets, 1);
  if(tabw)
  {
    tabw->type = type;
    if(type == GAP_STB_MASTER_TYPE_STORYBOARD)
    {
      tabw->cols = 10;
      tabw->rows = 2;
      tabw->thumbsize = 88;
      tabw->filename_refptr = sgpp->storyboard_filename;
      tabw->filename_maxlen = sizeof(sgpp->storyboard_filename);
      tabw->edmode = GAP_STB_EDMO_FRAME_NUMBER;
    }
    else
    {
      tabw->filename_refptr = sgpp->cliplist_filename;
      tabw->filename_maxlen = sizeof(sgpp->cliplist_filename);
      tabw->cols = 5;
      tabw->rows = 4;
      tabw->thumbsize = 88;
      tabw->edmode = GAP_STB_EDMO_SEQUENCE_NUMBER;
    }
    tabw->filesel = NULL;
    tabw->filename_entry = NULL;
    tabw->load_button = NULL;
    tabw->save_button = NULL;
    tabw->play_button = NULL;
    tabw->edit_paste_button = NULL;
    tabw->edit_cut_button = NULL;
    tabw->edit_copy_button = NULL;
    
    tabw->thumb_width = tabw->thumbsize;
    tabw->thumb_height = tabw->thumbsize;
    tabw->rowpage = 1;
    
    tabw->mount_table = NULL;
    tabw->frame_with_name = NULL;
    tabw->fw_gtk_table = NULL;
    tabw->rowpage_spinbutton_adj = NULL;
    tabw->rowpage_vscale_adj = NULL;
    tabw->total_rows_label = NULL;
    
    tabw->pw = NULL;
    tabw->fw_tab = NULL;
    tabw->fw_tab_size = 0;  /* start up with empty fw table of 0 elements */
    tabw->master_dlg_open  = FALSE;
    tabw->sgpp = sgpp;
  }
  return(tabw);
}  /* end p_new_stb_tab_widgets */


/* -----------------------------
 * p_render_all_frame_widgets
 * -----------------------------
 * assign frame_filename and stb_elem_refptr
 * to all frame widgets
 * according to the current rowpage setting (scroll).
 */
static void
p_render_all_frame_widgets (GapStbTabWidgets *tabw)
{
  GapStoryBoard *l_stb;
  GapStbMainGlobalParams *sgpp_ptr;
  GapStbFrameWidget *fw;
  gint32 ii;
  gint32 seq_nr;
  gint32 l_act_elems = 0;
  

//printf("(1) START p_render_all_frame_widgets\n");

  if(tabw == NULL)
  {
    return;
  }

  sgpp_ptr = (GapStbMainGlobalParams *)tabw->sgpp;
  if(sgpp_ptr)
  {
    sgpp_ptr->auto_vthumb_refresh_canceled = FALSE;
  }
  l_stb = p_tabw_get_stb_ptr(tabw);
 
  if(l_stb)
  {
    l_act_elems = gap_story_count_active_elements(l_stb, 1 /* track */ );   
  }
  
  
  /* init buffers for the frame widgets */
  seq_nr = (tabw->rowpage -1) * tabw->cols;
  for(ii=0; ii < tabw->fw_tab_size; ii++)
  {
    seq_nr++;

//printf("(2) p_render_all_frame_widgets act_elems: %d ii:%d  seq_nr: %d\n"
//	       , (int)l_act_elems
//               , (int)ii
//	       , (int)seq_nr
//	        );

    fw = tabw->fw_tab[ii];
//printf("(3) p_render_all_frame_widgets fw: %d\n", (int)fw );
//printf("(4) p_render_all_frame_widgets fw->frame_filename: %d\n", (int)&fw->frame_filename );
    if(fw->frame_filename)
    {
      g_free(fw->frame_filename);
      fw->frame_filename = NULL;
    }
    fw->tabw = tabw;
    fw->stb_refptr = l_stb;
    fw->stb_elem_refptr = gap_story_fetch_nth_active_elem(l_stb, seq_nr, 1 /* track*/);
    if(fw->stb_elem_refptr)
    {
      fw->frame_filename = gap_story_get_filename_from_elem(fw->stb_elem_refptr);
      fw->seq_nr = seq_nr;
      
      p_frame_widget_render(fw);
      if(sgpp_ptr)
      {
       if(sgpp_ptr->cancel_video_api)
       {
         /* if user did press cancel during videoseek or videoindex creation
	  * we set the flag auto_vthumb_refresh_canceled TRUE
	  * this Flag stays TRUE until the next start of this refresh procedure.
	  * All further video items waiting for refresh are then rendered 
	  * with the default icon (instead of accessing the videofile) in this refresh cycle.
	  * otherwise the user has to cancel each video item seperately
	  */
         sgpp_ptr->auto_vthumb_refresh_canceled = TRUE;
       }
      }
    }

  }
 
  /* render selection state for all displayed framewidgets */ 
  p_tabw_update_selection(tabw);
}  /* end p_render_all_frame_widgets */


/* ---------------------------------
 * p_frame_widget_init_empty
 * ---------------------------------
 * this procedure does initialize
 * the passed frame_widget, by creating all
 * the gtk_widget stuff to build up the frame_widget
 */
static void
p_frame_widget_init_empty (GapStbTabWidgets *tabw, GapStbFrameWidget *fw)
{
  GtkWidget *alignment;
  gint32     l_check_size;

  if(gap_debug) printf("p_frame_widget_init_empty START\n");

  /* create all sub-widgets for the given frame widget */
  fw->pv_ptr        = NULL;
  fw->seq_nr = 0;
  fw->frame_filename = NULL;
  fw->stb_elem_refptr = NULL;
  fw->stb_refptr = NULL;
  fw->tabw = tabw;
  fw->sgpp = tabw->sgpp;

  fw->vbox = gtk_vbox_new (FALSE, 1);

  /* the vox2  */
  fw->vbox2 = gtk_vbox_new (FALSE, 1);

  /* Create an EventBox (container for labels and the preview drawing_area)
   * used to handle selection events
   */
  fw->event_box = gtk_event_box_new ();
  gtk_container_add (GTK_CONTAINER (fw->event_box), fw->vbox2);
  gtk_widget_set_events (fw->event_box, GDK_BUTTON_PRESS_MASK);
  g_signal_connect (G_OBJECT (fw->event_box), "button_press_event",
                    G_CALLBACK (p_frame_widget_preview_events_cb),
                    fw);

  gtk_box_pack_start (GTK_BOX (fw->vbox), fw->event_box, FALSE, FALSE, 1);


  /*  The frame preview  */
  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (fw->vbox2), alignment, FALSE, FALSE, 2);
  gtk_widget_show (alignment);

  l_check_size = tabw->thumbsize / 16;
  fw->pv_ptr = gap_pview_new( tabw->thumb_width + 4
                            , tabw->thumb_height + 4
                            , l_check_size
                            , NULL   /* no aspect_frame is used */
                            );

  gtk_widget_set_events (fw->pv_ptr->da_widget, GDK_EXPOSURE_MASK);
  gtk_container_add (GTK_CONTAINER (alignment), fw->pv_ptr->da_widget);
  gtk_widget_show (fw->pv_ptr->da_widget);
  g_signal_connect (G_OBJECT (fw->pv_ptr->da_widget), "event",
                    G_CALLBACK (p_frame_widget_preview_events_cb),
                    fw);

  /* the hbox  */
  fw->hbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (fw->vbox2), fw->hbox, FALSE, FALSE, 2);

  /*  the key label */
  fw->key_label = gtk_label_new ("key");
  gtk_box_pack_start (GTK_BOX (fw->hbox), fw->key_label, FALSE, FALSE, 2);
  gtk_widget_show (fw->key_label);

  /*  the frame timing label */
  fw->val_label = gtk_label_new ("val");
  gtk_box_pack_start (GTK_BOX (fw->hbox), fw->val_label, FALSE, FALSE, 2);
  gtk_widget_show (fw->val_label);


  gtk_widget_show (fw->hbox);
  gtk_widget_show (fw->vbox2);
  gtk_widget_show (fw->event_box);
  gtk_widget_show (fw->vbox);

  if(gap_debug) printf("p_frame_widget_init_empty END\n");

}  /* end p_frame_widget_init_empty */


/* ---------------------------------
 * p_frame_widget_render
 * ---------------------------------
 * 1.) fetch thumbnal pixbuf data,
 * 2.) if no thumbnail available
 *     try to load full image (and create the thumbnail for next usage)
 * 3.) if neither thumbnail nor image could be fetched 
 *        render a default icon
 *
 */
static void
p_frame_widget_render (GapStbFrameWidget *fw)
{
   gint32  l_th_width;
   gint32  l_th_height;
   gint32  l_th_bpp;
   GdkPixbuf *pixbuf;


   if((fw->key_label) && (fw->val_label))
   {
     char *l_txt;
     gint32 l_framenr;
     GapStbTabWidgets *tabw;
     char  txt_buf[100];
    
     tabw = (GapStbTabWidgets *)fw->tabw;
     
     l_txt = g_strdup_printf("[%d]", (int)fw->seq_nr);
     gtk_label_set_text ( GTK_LABEL(fw->key_label), l_txt);
     g_free(l_txt);
     
     switch(tabw->edmode)
     {
      case GAP_STB_EDMO_FRAME_NUMBER:
         if(fw->stb_refptr)
	 {
           l_framenr = gap_story_get_framenr_by_story_id(fw->stb_refptr
	                                        	,fw->stb_elem_refptr->story_id
							);
           l_txt = g_strdup_printf("%06d", (int)l_framenr);
           gtk_label_set_text ( GTK_LABEL(fw->val_label), l_txt);
           g_free(l_txt);
           break;
	 }
       case GAP_STB_EDMO_TIMECODE:
         if(fw->stb_refptr)
	 {
	   gdouble l_framerate;
	   
	   l_framerate = fw->stb_refptr->master_framerate;
	   if(l_framerate < 1)
	   {
	     l_framerate = GAP_STORY_DEFAULT_FRAMERATE;
	   }
           l_framenr = gap_story_get_framenr_by_story_id(fw->stb_refptr
	                                        	,fw->stb_elem_refptr->story_id
							);
           gap_timeconv_framenr_to_timestr(l_framenr 
                             , (gdouble)l_framerate
                             , txt_buf
                             , sizeof(txt_buf)
                             );
           gtk_label_set_text ( GTK_LABEL(fw->val_label), txt_buf);
           break;
	 }
       default:
         gtk_label_set_text ( GTK_LABEL(fw->val_label), " ");
         break;
     }
   }

   /* init preferred width and height
    * (as hint for the thumbnail loader to decide
    *  if thumbnail is to fetch from normal or large thumbnail directory
    *  just for the case when both sizes are available)
    */
   l_th_width = 128;
   l_th_height = 128;

   if(fw->stb_elem_refptr->record_type == GAP_STBREC_VID_MOVIE)
   {
     guchar *l_th_data;
     
     /*if(gap_debug) printf("RENDER: p_frame_widget_render MOVIE Thumbnail\n"); */
     
     l_th_data = gap_story_dlg_fetch_vthumb(fw->sgpp
              ,fw->stb_elem_refptr->orig_filename
	      ,fw->stb_elem_refptr->from_frame
	      ,fw->stb_elem_refptr->seltrack
	      ,gap_story_get_preferred_decoder(fw->stb_refptr, fw->stb_elem_refptr)
	      ,&l_th_bpp
	      ,&l_th_width
	      ,&l_th_height
	      );
     if(l_th_data)
     {
       gboolean l_th_data_was_grabbed;
       
       l_th_data_was_grabbed = gap_pview_render_from_buf (fw->pv_ptr
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
       return;
     }
   
   }
  
   if(fw->frame_filename == NULL)
   {
     /* no filename available, use default icon */
     gap_pview_render_default_icon(fw->pv_ptr);
     return;
   }
   
   pixbuf = gap_thumb_file_load_pixbuf_thumbnail(fw->frame_filename
                                    , &l_th_width, &l_th_height
				    , &l_th_bpp
                                    );
   if(pixbuf)
   {
     gap_pview_render_from_pixbuf (fw->pv_ptr, pixbuf);
     g_object_unref(pixbuf);
   }
   else
   {
      gint32  l_image_id;

      l_image_id = gap_lib_load_image(fw->frame_filename);

      if (l_image_id < 0)
      {
        /* could not read the image
         */
        if(gap_debug) printf("p_frame_widget_render: fetch failed, using DEFAULT_ICON\n");
        gap_pview_render_default_icon(fw->pv_ptr);
      }
      else
      {
	/* there is no need for undo on this scratch image
	 * so we turn undo off for performance reasons
	 */
	gimp_image_undo_disable (l_image_id);
        gap_pview_render_from_image (fw->pv_ptr, l_image_id);
	
	/* create thumbnail (to speed up acces next time) */
	gap_thumb_cond_gimp_file_save_thumbnail(l_image_id, fw->frame_filename);
	
        gimp_image_delete(l_image_id);
      }
   }				    

}       /* end p_frame_widget_render */



/* ---------------------------------
 * p_frame_widget_destroy
 * ---------------------------------
 */
static void
p_frame_widget_destroy(GapStbFrameWidget *fw)
{
  gap_pview_reset(fw->pv_ptr);

  if(fw->frame_filename) { g_free(fw->frame_filename); }
  fw->frame_filename = NULL;
  fw->stb_elem_refptr = NULL;
  fw->stb_refptr = NULL;
  fw->tabw = NULL;

  gtk_widget_destroy(fw->vbox);
  
}  /* end p_frame_widget_destroy */



/* -----------------------------
 * p_get_max_rowpage
 * -----------------------------
 */
static gint32
p_get_max_rowpage(gint32 act_elems, gint32 cols, gint rows)
{
  gint32 l_max_rowpage;
  
  if(cols < 1)
  {
    l_max_rowpage = (MAX(act_elems, 1) -1);
  }
  else
  {
    l_max_rowpage = ((MAX(act_elems, 1) -1) / cols);
  }
  l_max_rowpage =  1 + (l_max_rowpage - (MAX(rows,1) -1));
  return(MAX(l_max_rowpage,1));
  
}  /* end p_get_max_rowpage */



/* -----------------------------
 * p_story_set_range_cb
 * -----------------------------
 */
static void
p_story_set_range_cb(GapPlayerAddClip *plac_ptr)
{
  if(gap_debug)
  {
    printf("p_story_set_range_cb:\n");
    printf("  FROM : %d\n", (int)plac_ptr->range_from);
    printf("  TO : %d\n", (int)plac_ptr->range_to);
  }
  if(plac_ptr->ainfo_ptr)
  {
    GapStbMainGlobalParams  *sgpp;
    GapStbTabWidgets *tabw;
    GapStoryBoard *stb_dst;
    GapStoryElem *stb_elem;
    
    if(gap_debug)
    {
      printf("  basename  :%s:\n", plac_ptr->ainfo_ptr->basename);
      printf("  extension :%s:\n", plac_ptr->ainfo_ptr->extension);
      printf("  frame_cnt :%d\n",  (int)plac_ptr->ainfo_ptr->frame_cnt);
      printf("  old_filename :%s\n",  plac_ptr->ainfo_ptr->old_filename);
    }
    
    sgpp = (GapStbMainGlobalParams *)plac_ptr->user_data_ptr;
    if(sgpp)
    {
      stb_dst = (GapStoryBoard *)sgpp->cll;
      tabw = sgpp->cll_widgets;
      if(stb_dst == NULL)
      {
        stb_dst = (GapStoryBoard *)sgpp->stb;
        tabw = sgpp->stb_widgets;
      }

      if(stb_dst)
      {
        stb_elem = gap_story_new_elem(GAP_STBREC_VID_UNKNOWN);
	if(stb_elem)
	{
          gap_story_upd_elem_from_filename(stb_elem, plac_ptr->ainfo_ptr->old_filename);
          stb_elem->from_frame = plac_ptr->range_from;
          stb_elem->to_frame   = plac_ptr->range_to;
	  gap_story_elem_calculate_nframes(stb_elem);
	  if(plac_ptr->ainfo_ptr)
	  {
	    if(plac_ptr->ainfo_ptr->ainfo_type == GAP_AINFO_MOVIE)
	    {
	      stb_elem->record_type = GAP_STBREC_VID_MOVIE;
	      stb_elem->seltrack    = plac_ptr->ainfo_ptr->seltrack;
	      stb_elem->exact_seek  = 1;
	      stb_elem->delace      = plac_ptr->ainfo_ptr->delace;
	      //stb_elem->density   = plac_ptr->ainfo_ptr->density;
	      
	    }
	  }

	  gap_story_list_append_elem(stb_dst, stb_elem);

	  /* refresh storyboard layout and thumbnail list widgets */
	  p_recreate_tab_widgets( stb_dst
                        	 ,tabw
				 ,tabw->mount_col
				 ,tabw->mount_row
				 ,sgpp
				 );
	  p_render_all_frame_widgets(tabw);
	}
      }
    }
    
  }
  
}  /* end p_story_set_range_cb */


/* -----------------------------
 * p_story_call_player
 * -----------------------------
 * IN: stb       Pointer to a stroyboard list structure 
 * IN: imagename of one frame to playback in normal mode (NULL for storyboard playback)
 * IN: image_id  of one frame to playback in normal mode (use -1 for storyboard playback)
 *
 * IN: begin_frame  use -1 to start play from  1.st frame
 * IN: end_frame    use -1 to start play until last frame
 *
 * stb_mode:    stb != NULL, imagename == NULL, image_id == -1
 * imagemode:   stb == NULL, imagename != NULL, image_id == -1
 * normalmode:  stb == NULL, imagename == NULL, image_id >= 0
 *
 * Call the Player
 * If it is the 1.st call or the player window has closed since last call 
 *    create the player widget
 * else 
 *    reset the player widget
 */
void    
p_story_call_player(GapStbMainGlobalParams *sgpp
                   ,GapStoryBoard *stb
		   ,char *imagename
		   ,gint32 imagewidth
		   ,gint32 imageheight
		   ,gint32 image_id
		   , gint32 begin_frame
		   , gint32 end_frame
		   , gboolean play_all
		   , gint32 seltrack
		   , gdouble delace
		   , const char *preferred_decoder
		   )
{
  GapStoryBoard *stb_dup;
  
  if(sgpp->in_player_call)
  {
    /* this procedure is already active, and locked against
     * calls while busy
     */
    return;
  }


  sgpp->in_player_call = TRUE;
  p_close_videofile(sgpp);
  
  stb_dup = NULL;
  if(stb)
  {
    /* make a copy of the storyboard list 
     * for the internal use in the player
     */
    if(play_all)
    {
      stb_dup = gap_story_duplicate(stb);
    }
    else
    {
      stb_dup = gap_story_duplicate_sel_only(stb);
    }
    if(stb_dup)
    {
      if(stb_dup->stb_elem == NULL)
      {
        /* there was no selection, now try to playback the 
	 * whole storyboard
	 */
        gap_story_free_storyboard(&stb_dup);
        stb_dup = gap_story_duplicate(stb);
      }
    }
    if(stb_dup == NULL)
    {
      return;  /* in case of errors: NO playback possible */
      sgpp->in_player_call = FALSE;
    }
  }
  
 
  if(sgpp->plp)
  {
    if(sgpp->plp->stb_ptr)
    {
      /* free the (old duplicate) of the storyboard list
       * that was used in the previous
       * player call
       */
      gap_story_free_storyboard(&sgpp->plp->stb_ptr);
    }
    
    if((sgpp->plp->shell_window == NULL) 
    && (sgpp->plp->docking_container == NULL))
    {
      /*if(gap_debug)*/ printf("Player shell has gone, force Reopen now\n");

      gap_player_dlg_cleanup(sgpp->plp);
      g_free(sgpp->plp);
      sgpp->plp = NULL;
    }
  }

  if(sgpp->plp == NULL)
  {
printf("p_story_call_player: 1.st start\n");

    /* 1. START mode */
    sgpp->plp = (GapPlayerMainGlobalParams *)g_malloc0(sizeof(GapPlayerMainGlobalParams));

    if(sgpp->plp)
    {
      sgpp->plp->standalone_mode = FALSE;  /* player acts as widget and does not call gtk_main_quit */
      sgpp->plp->docking_container = sgpp->player_frame;
      
      sgpp->plp->help_id = GAP_STB_PLAYER_HELP_ID;
      if(sgpp->plp->docking_container)
      {
        /* the player widget is created without Help button
	 * when it is part of the storyboard window
	 */
        sgpp->plp->help_id = NULL;
      }

      sgpp->plp->autostart = TRUE;
      sgpp->plp->caller_range_linked = FALSE;
      sgpp->plp->use_thumbnails = TRUE;
      sgpp->plp->exact_timing = TRUE;
      sgpp->plp->play_selection_only = TRUE;
      sgpp->plp->play_loop = FALSE;
      sgpp->plp->play_pingpong = FALSE;
      sgpp->plp->play_backward = FALSE;

      sgpp->plp->stb_ptr = stb_dup;
      sgpp->plp->image_id = image_id;
      sgpp->plp->imagename = NULL;
      if(imagename)
      {
        sgpp->plp->imagename = g_strdup(imagename);
      }
      sgpp->plp->imagewidth = imagewidth;
      sgpp->plp->imageheight = imageheight;

      sgpp->plp->play_current_framenr = 0;
      sgpp->plp->begin_frame = begin_frame;
      sgpp->plp->end_frame = end_frame;

      sgpp->plp->fptr_set_range = p_story_set_range_cb;
      sgpp->plp->user_data_ptr = sgpp;
      sgpp->plp->seltrack = seltrack;
      sgpp->plp->delace = delace;
      sgpp->plp->preferred_decoder = g_strdup(preferred_decoder);
      sgpp->plp->force_open_as_video = FALSE;   /* FALSE: try video open only for known videofile extensions */
      sgpp->plp->have_progress_bar = TRUE;
      
      gap_player_dlg_create(sgpp->plp);

    }
  }
  else
  {
printf("p_story_call_player: RE start\n");
    /* RESTART mode */
    gap_player_dlg_restart(sgpp->plp
                      , TRUE               /* gboolean autostart */
		      , image_id
		      , imagename
		      , imagewidth
		      , imageheight
		      , stb_dup
		      , begin_frame
		      , end_frame
		      , TRUE              /* gboolean play_selection_only */
		      , seltrack
		      , delace
		      , preferred_decoder
		      , FALSE             /* force_open_as_video */
		      );

  }

  sgpp->in_player_call = FALSE;
}  /* end p_story_call_player */



/* -----------------------------
 * p_call_master_encoder
 * -----------------------------
 * INTERACTIVE call of the GAP master videoencoder dialog plug-in
 */
static void
p_call_master_encoder(GapStbMainGlobalParams *sgpp
                     , GapStoryBoard *stb
		   )
{
#define GAP_PLUG_IN_MASTER_ENCODER  "plug_in_gap_vid_encode_master"
  GimpParam* l_params;
  gint       l_retvals;
  gint       l_rc;

  gint32 vid_width;
  gint32 vid_height;

  if(gap_debug) 
  {
    printf("p_call_master_encoder\n");
  }

  if((sgpp == NULL)
  || (stb == NULL))
  {
    return;
  }
  
  l_rc = -1;
  gap_story_get_master_size(stb, &vid_width, &vid_height);


  /* generic call of GAP master video encoder plugin */
  l_params = gimp_run_procedure (GAP_PLUG_IN_MASTER_ENCODER,
                     &l_retvals,
                     GIMP_PDB_INT32,  GIMP_RUN_INTERACTIVE,
                     GIMP_PDB_IMAGE,  sgpp->image_id,  /* pass the image where invoked from */
                     GIMP_PDB_DRAWABLE, -1,
                     GIMP_PDB_STRING, "video_output.avi",
                     GIMP_PDB_INT32,  1,            /* range_from */
                     GIMP_PDB_INT32,  2,            /* range_to */
                     GIMP_PDB_INT32,  vid_width,
                     GIMP_PDB_INT32,  vid_height,
                     GIMP_PDB_INT32,  1,           /* 1 VID_FMT_PAL,  2 VID_FMT_NTSC */
                     GIMP_PDB_FLOAT,  MAX(stb->master_framerate, 1.0), 
                     GIMP_PDB_INT32,  44100,       /* samplerate */
                     GIMP_PDB_STRING, NULL,        /* audfile l_16bit_wav_file */
                     GIMP_PDB_STRING, NULL,        /* vid_enc_plugin */
                     GIMP_PDB_STRING, NULL,        /* filtermacro_file */
                     GIMP_PDB_STRING, stb->storyboardfile,        /* storyboard_file */
                     GIMP_PDB_INT32,  2,           /* GAP_RNGTYPE_STORYBOARD input_mode */
                     GIMP_PDB_END);
  switch(l_params[0].data.d_status)
  {
    case GIMP_PDB_SUCCESS:
      l_rc = 0;
      break;
    case GIMP_PDB_CANCEL:
      break;
    default:
      printf("gap_story_dialog:p_call_master_encoder\n"
             "PDB calling error %s\n"
	     "status: %d\n"
            , GAP_PLUG_IN_MASTER_ENCODER
	    , (int)l_params[0].data.d_status
	    );
      break;
  }
  g_free(l_params);


  
}  /* end p_call_master_encoder */

/* -----------------------------
 * p_storyboard_reload
 * -----------------------------
 * return TRUE on successful reload
 */
static gboolean
p_storyboard_reload (GapStbMainGlobalParams *sgpp)
{
  gboolean l_ret;
  
  l_ret = FALSE;
  
  if(sgpp)
  {
    GapStoryBoard *stb;
    
    /* load the new storyboard structure from file */
    stb = gap_story_parse(sgpp->storyboard_filename);
    if(stb)
    {
      if(stb->errtext != NULL)
      {
	g_message(_("** ERROR: Storyboard parser reported:\n%s\n"), stb->errtext);
	gap_story_free_storyboard(&stb);
      }
      else
      {
	if(sgpp->stb)
	{
	  if(sgpp->stb_widgets)
	  {
            p_tabw_destroy_properties_dlg (sgpp->stb_widgets, TRUE /* destroy_all*/ );
	  }

	  /* drop the old storyboard structure */
	  gap_story_free_storyboard(&sgpp->stb);
	}
	sgpp->stb = stb;
	sgpp->stb->master_type = GAP_STB_MASTER_TYPE_STORYBOARD;
        l_ret = TRUE;
	
	/* refresh storyboard layout and thumbnail list widgets */
        p_recreate_tab_widgets( sgpp->stb
                         ,sgpp->stb_widgets
			 ,sgpp->stb_widgets->mount_col
			 ,sgpp->stb_widgets->mount_row
			 ,sgpp
			 );
	p_render_all_frame_widgets(sgpp->stb_widgets);
      }
    }
    
  }
  return (l_ret);
}  /* end p_storyboard_reload */

/* -----------------------------
 * p_cliplist_reload
 * -----------------------------
 * return TRUE on successful reload
 */
static gboolean
p_cliplist_reload (GapStbMainGlobalParams *sgpp)
{
  gboolean l_ret;
  
  l_ret = FALSE;
  
  if(sgpp)
  {
    GapStoryBoard *stb;
    
    /* load the new storyboard structure from file */
    stb = gap_story_parse(sgpp->cliplist_filename);
    if(stb)
    {
      if(stb->errtext != NULL)
      {
	g_message(_("** ERROR: Storyboard parser reported:\n%s\n"), stb->errtext);
	gap_story_free_storyboard(&stb);
      }
      else
      {
	if(sgpp->cll)
	{
	  if(sgpp->cll_widgets)
	  {
            p_tabw_destroy_properties_dlg (sgpp->cll_widgets, TRUE /* destroy_all*/ );
	  }
	  /* drop the old storyboard structure */
	  gap_story_free_storyboard(&sgpp->cll);
	}
	sgpp->cll = stb;
	sgpp->cll->master_type = GAP_STB_MASTER_TYPE_CLIPLIST;
        l_ret = TRUE;
	
	/* refresh storyboard layout and thumbnail list widgets */
        p_recreate_tab_widgets( sgpp->cll
                         ,sgpp->cll_widgets
			 ,sgpp->cll_widgets->mount_col
			 ,sgpp->cll_widgets->mount_row
			 ,sgpp
			 );
	p_render_all_frame_widgets(sgpp->cll_widgets);
      }
    }
    
  }
  return (l_ret);
}  /* end p_cliplist_reload */

/*
 * DIALOG and callback stuff
 */

/* -----------------------------
 * p_player_cll_mode_cb
 * -----------------------------
 */
static void
p_player_cll_mode_cb (GtkWidget *w
                     , GdkEventButton  *bevent
		     , GapStbMainGlobalParams *sgpp)
{
  gint32 imagewidth;
  gint32 imageheight;
  gboolean play_all;
  
  play_all = TRUE;
  if(bevent)
  {
    if(bevent->state & GDK_SHIFT_MASK)
    {
      play_all = FALSE;
    }
  }
  
  if(sgpp->cll)
  {
    gap_story_get_master_size(sgpp->cll, &imagewidth, &imageheight);
    p_story_call_player(sgpp
	        	 ,sgpp->cll
			 ,NULL      /* no imagename */
			 ,imagewidth
			 ,imageheight
			 ,-1	    /* image_id (unused in storyboard playback mode) */
			 ,-1        /* play from start */
			 ,-1        /* play until end */
			 ,play_all
			 ,1         /* seltrack */
			 ,0.0       /* delace */
			 ,gap_story_get_preferred_decoder(sgpp->cll, NULL)
			 );
  }
}  /* end p_player_cll_mode_cb */

/* -----------------------------
 * p_player_stb_mode_cb
 * -----------------------------
 */
static void
p_player_stb_mode_cb (GtkWidget *w
                     , GdkEventButton  *bevent
		     , GapStbMainGlobalParams *sgpp)
{
  gint32 imagewidth;
  gint32 imageheight;
  gboolean play_all;
  
  play_all = TRUE;
  if(bevent)
  {
    if(bevent->state & GDK_SHIFT_MASK)
    {
      play_all = FALSE;
    }
  }
  if(sgpp->stb)
  {
    gap_story_get_master_size(sgpp->stb, &imagewidth, &imageheight);
    p_story_call_player(sgpp
	        	 ,sgpp->stb
			 ,NULL      /* no imagename */
			 ,imagewidth
			 ,imageheight
			 ,-1	    /* image_id (unused in storyboard playback mode) */
			 ,-1        /* play from start */
			 ,-1        /* play until end */
			 ,play_all
			 ,1         /* seltrack */
			 ,0.0       /* delace */
			 ,gap_story_get_preferred_decoder(sgpp->stb, NULL)
			 );  
  }
}  /* end p_player_stb_mode_cb */


/* -----------------------------
 * p_player_img_mode_cb
 * -----------------------------
 * this is used to init the player window
 * with image where the storyboard was invoked from.
 */
static void
p_player_img_mode_cb (GtkWidget *w,
		      GapStbMainGlobalParams *sgpp)
{
  gint32 imagewidth;
  gint32 imageheight;
  long   framenr;
  char  *basename;
  char  *imagename;
  
  
  
  imagewidth = gimp_image_width(sgpp->image_id);
  imageheight = gimp_image_height(sgpp->image_id);
  imagename = gimp_image_get_filename(sgpp->image_id);

  basename = gap_lib_alloc_basename(imagename, &framenr);
  
  if(basename)
  {
    p_story_call_player(sgpp
	             ,NULL              /* Play Normal mode without storyboard */
		     ,NULL              /* no imagename mode */
		     ,imagewidth
		     ,imageheight
		     ,sgpp->image_id
		     ,framenr         /* play from start */
		     ,framenr         /* play until end */
		     ,FALSE      /* play all */
		     ,1         /* seltrack */
		     ,0.0       /* delace */
		     ,"libavformat"
		     );  
    g_free(basename);
  }
  
  if(imagename)
  {
    g_free(imagename);
  }
  

}  /* end p_player_img_mode_cb */



/* -----------------------------
 * p_cancel_button_cb
 * -----------------------------
 */
static void
p_cancel_button_cb (GtkWidget *w,
		    GapStbMainGlobalParams *sgpp)
{

  printf("VIDEO-CANCEL BUTTON clicked\n");
  
  if(sgpp)
  {
    sgpp->cancel_video_api = TRUE;
    sgpp->auto_vthumb_refresh_canceled = TRUE;
  }

}  /* end p_cancel_button_cb */



/* -----------------------------------
 * p_tabw_add_elem
 * -----------------------------------
 */
static void
p_tabw_add_elem (GapStbTabWidgets *tabw, GapStbMainGlobalParams *sgpp, GapStoryBoard *stb_dst)
{
  GapStoryElem *stb_elem;
  if(gap_debug) printf("p_tabw_add_elem\n");

  if(sgpp == NULL)    { return; }
  if(tabw == NULL)    { return; }
  if(stb_dst == NULL) { return; }

  stb_elem = gap_story_new_elem(GAP_STBREC_VID_IMAGE);
  /* stb_elem->orig_filename = g_strdup("empty.xcf"); */
  stb_elem->from_frame = 1;
  stb_elem->to_frame = 1;
  stb_elem->nloop = 1;
  stb_elem->nframes = 1;
  if(stb_elem)
  {
    gap_story_list_append_elem(stb_dst, stb_elem);

    /* refresh storyboard layout and thumbnail list widgets */
    p_recreate_tab_widgets( stb_dst
                        	 ,tabw
				 ,tabw->mount_col
				 ,tabw->mount_row
				 ,sgpp
				 );
    p_render_all_frame_widgets(tabw);
    gap_story_stb_elem_properties_dialog(tabw, stb_elem, stb_dst);
  }

  p_tabw_update_frame_label(tabw, sgpp);
}  /* end p_tabw_add_elem */



/* -----------------------------
 * p_filesel_tabw_close_cb
 * -----------------------------
 */
static void
p_filesel_tabw_close_cb ( GtkWidget *widget
		        , GapStbTabWidgets *tabw)
{
  if(tabw->filesel == NULL) return;

  gtk_widget_destroy(GTK_WIDGET(tabw->filesel));
  tabw->filesel = NULL;   /* now filesel_story is closed */
  
}  /* end p_filesel_tabw_close_cb */


/* --------------------------------
 * p_filesel_tabw_ok
 * --------------------------------
 */
static gboolean
p_filesel_tabw_ok (GtkWidget *widget
		   ,GapStbTabWidgets *tabw)
{
  const gchar *filename;
  gboolean     rc;

  rc = FALSE;
  if(tabw == NULL) return (rc);
  if(tabw->filesel == NULL) return (rc);

  filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (tabw->filesel));
  if(filename)
  {
    if(tabw->filename_refptr)
    {
      g_snprintf(tabw->filename_refptr
              , tabw->filename_maxlen
	      , "%s"
              , filename
	      );
      gtk_entry_set_text(GTK_ENTRY(tabw->filename_entry), filename);
    }
    rc = TRUE;
  }

  gtk_widget_destroy(GTK_WIDGET(tabw->filesel));
  tabw->filesel = NULL;

  return(rc);
}  /* end p_filesel_tabw_ok */

/* --------------------------------
 * p_filesel_tabw_load_ok_cb
 * --------------------------------
 */
static void
p_filesel_tabw_load_ok_cb (GtkWidget *widget
		   ,GapStbTabWidgets *tabw)
{
  if(p_filesel_tabw_ok(widget, tabw))
  {
    /* (re)load storyboard from new filename */
    if(tabw->type == GAP_STB_MASTER_TYPE_STORYBOARD)
    {
      p_storyboard_reload(tabw->sgpp);
    }
    else
    {
      p_cliplist_reload(tabw->sgpp);
    }
  }
}  /* end p_filesel_tabw_load_ok_cb */

/* --------------------------------
 * p_filesel_tabw_save_ok_cb
 * --------------------------------
 */
static void
p_filesel_tabw_save_ok_cb ( GtkWidget *widget
			  , GapStbTabWidgets *tabw)
{
  gboolean l_wr_permission;

  if(tabw)
  {
    if(p_filesel_tabw_ok(widget, tabw))
    {
      l_wr_permission = gap_arr_overwrite_file_dialog(tabw->filename_refptr);

      if(l_wr_permission)
      {
        if(tabw->type == GAP_STB_MASTER_TYPE_STORYBOARD)
	{
          /* save storyboard filename */
          p_menu_stb_save_cb(widget, tabw->sgpp);
	}
	else
	{
          /* save cliplist filename */
          p_menu_cll_save_cb(widget, tabw->sgpp);
	}
      }
    }
  }
}  /* end p_filesel_tabw_save_ok_cb */


/* -----------------------------
 * p_tabw_load_file_cb
 * -----------------------------
 */
static void
p_tabw_load_file_cb ( GtkWidget *w
		    , GapStbTabWidgets *tabw)
{
  GapStbMainGlobalParams *sgpp;
  GtkWidget *filesel = NULL;
  gboolean   drop_unsaved_changes = TRUE;

printf("##### (1) p_tabw_load_file_cb tabw->filesel:%d\n", (int)tabw->filesel );
  if(tabw->filesel != NULL)
  {
     gtk_window_present(GTK_WINDOW(tabw->filesel));
     return;   /* filesel is already open */
  }
  sgpp = (GapStbMainGlobalParams *)tabw->sgpp;
  if(sgpp == NULL) { return; }


  if(tabw->type == GAP_STB_MASTER_TYPE_STORYBOARD)
  {
    drop_unsaved_changes = p_check_unsaved_changes(tabw->sgpp
                           , FALSE  /* check_cliplist */
			   , TRUE   /* check_storyboard */
			   );
    if(drop_unsaved_changes)
    {
        p_tabw_destroy_properties_dlg(tabw, TRUE /* destroy_all */ );
	/* drop the old storyboard structure */
	gap_story_free_storyboard(&sgpp->stb);
	p_recreate_tab_widgets( sgpp->stb
                           ,sgpp->stb_widgets
			   ,sgpp->stb_widgets->mount_col
			   ,sgpp->stb_widgets->mount_row
			   ,sgpp
			   );
    }
    else
    {
      return;
    }
  }
  else
  {
    drop_unsaved_changes = p_check_unsaved_changes(tabw->sgpp
                           , TRUE   /* check_cliplist */
			   , FALSE  /* check_storyboard */
			   );
    if(drop_unsaved_changes)
    {
        p_tabw_destroy_properties_dlg(tabw, TRUE /* destroy_all */ );
	/* drop the old cliplist structure */
	gap_story_free_storyboard(&sgpp->cll);
	p_recreate_tab_widgets( sgpp->cll
                           ,sgpp->cll_widgets
			   ,sgpp->cll_widgets->mount_col
			   ,sgpp->cll_widgets->mount_row
			   ,sgpp
			   );
    }
    else
    {
      return;
    }
  }
  

  
  if(tabw->type == GAP_STB_MASTER_TYPE_STORYBOARD)
  {
    filesel = gtk_file_selection_new ( _("Load Storyboard"));
  }
  else
  {
    filesel = gtk_file_selection_new ( _("Load Cliplist"));
  }
  tabw->filesel = filesel;

  gtk_window_position (GTK_WINDOW (filesel), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button),
		      "clicked", (GtkSignalFunc) p_filesel_tabw_load_ok_cb,
		      tabw);
  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->cancel_button),
		      "clicked", (GtkSignalFunc) p_filesel_tabw_close_cb,
		      tabw);
  gtk_signal_connect (GTK_OBJECT (filesel), "destroy",
		      (GtkSignalFunc) p_filesel_tabw_close_cb,
		      tabw);
  gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesel),
                                     tabw->filename_refptr);
  gtk_widget_show (filesel);

}  /* end p_tabw_load_file_cb */



/* -----------------------------
 * p_tabw_file_save_as_cb
 * -----------------------------
 */
static void
p_tabw_file_save_as_cb ( GtkWidget *w
		       , GapStbTabWidgets *tabw)
{
  GtkWidget *filesel;

  if(tabw->filesel != NULL)
  {
     gtk_window_present(GTK_WINDOW(tabw->filesel));
     return;   /* filesel is already open */
  }

  filesel = gtk_file_selection_new ( _("Save Storyboard file"));
  tabw->filesel = filesel;

  gtk_window_position (GTK_WINDOW (filesel), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button),
		      "clicked", (GtkSignalFunc) p_filesel_tabw_save_ok_cb,
		      tabw);
  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->cancel_button),
		      "clicked", (GtkSignalFunc) p_filesel_tabw_close_cb,
		      tabw);
  gtk_signal_connect (GTK_OBJECT (filesel), "destroy",
		      (GtkSignalFunc) p_filesel_tabw_close_cb,
		      tabw);
  gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesel),
                                     tabw->filename_refptr);
  gtk_widget_show (filesel);

}  /* end p_tabw_file_save_as_cb */


/* ---------------------------------
 * p_tabw_filename_entry_cb
 * ---------------------------------
 */
static void
p_tabw_filename_entry_cb(GtkWidget *widget, GapStbTabWidgets *tabw)
{
  if(tabw)
  {
    if(tabw->filename_refptr)
    {
      g_snprintf(tabw->filename_refptr
            , tabw->filename_maxlen
	    , "%s"
            , gtk_entry_get_text(GTK_ENTRY(widget))
	    );
    }
  }
}  /* end p_tabw_filename_entry_cb */




/* -----------------------------
 * p_rowpage_spinbutton_cb
 * -----------------------------
 */
static void
p_rowpage_spinbutton_cb( GtkEditable     *editable
                       , GapStbTabWidgets *tabw)
{
  gint32 rowpage;
  
  if(tabw == NULL) { return; }
  
  rowpage = (gint32)GTK_ADJUSTMENT(tabw->rowpage_spinbutton_adj)->value;
  if(rowpage != tabw->rowpage)
  {
    GapStoryBoard *stb_dst;
    
    tabw->rowpage = rowpage;
    stb_dst = p_tabw_get_stb_ptr(tabw);
    
    if(stb_dst)
    {
      /* refresh storyboard layout and thumbnail list widgets */
      p_recreate_tab_widgets( stb_dst
                            ,tabw
			    ,tabw->mount_col
			    ,tabw->mount_row
			    ,tabw->sgpp
			    );
      p_render_all_frame_widgets(tabw);
    }
  }
  if(rowpage != (gint32)GTK_ADJUSTMENT(tabw->rowpage_vscale_adj)->value)
  {
    gtk_adjustment_set_value(GTK_ADJUSTMENT(tabw->rowpage_vscale_adj), (gdouble)rowpage);
  }
  
}  /* end p_rowpage_spinbutton_cb */


/* ------------------------------
 * p_vscale_changed_cb
 * ------------------------------
 */
static void
p_vscale_changed_cb(GtkObject *adj, GapStbTabWidgets *tabw)
{
  gint32  value;

  if(tabw == NULL) { return;}

  if((adj) && (tabw->rowpage_spinbutton_adj))
  {
    value = (gint32)GTK_ADJUSTMENT(adj)->value;
    if(value != tabw->rowpage)
    {
      /* set value in the rowpage spinbutton 
       * (this fires another callback for update of tabw->rowpage;)
       */
      gtk_adjustment_set_value(GTK_ADJUSTMENT(tabw->rowpage_spinbutton_adj), (gdouble)value);
      
    }
  }
  
}  /* end p_vscale_changed_cb */


/* ---------------------------------
 * gap_story_pw_single_clip_playback
 * ---------------------------------
 */
void
gap_story_pw_single_clip_playback(GapStbPropWidget *pw)
{
  gchar *imagename;
  GapStbTabWidgets *tabw;
  
  tabw = (GapStbTabWidgets *)pw->tabw;
  
  if((tabw) && (pw->sgpp))
  {
    imagename = gap_story_get_filename_from_elem(pw->stb_elem_refptr);
    p_story_call_player(pw->sgpp
	             ,NULL             /* No storyboard pointer */
		     ,imagename
		     ,tabw->thumb_width
		     ,tabw->thumb_height
		     ,-1	    /* image_id (unused in imagename based playback mode) */
		     ,pw->stb_elem_refptr->from_frame      /* play from */
		     ,pw->stb_elem_refptr->to_frame        /* play until */
     		     ,TRUE      /* play all */
		     ,pw->stb_elem_refptr->seltrack
		     ,pw->stb_elem_refptr->delace
		     ,gap_story_get_preferred_decoder(pw->stb_refptr, pw->stb_elem_refptr)
		     );  
    g_free(imagename);
  }
}  /* end gap_story_pw_single_clip_playback */


/* ---------------------------------
 * p_single_clip_playback
 * ---------------------------------
 */
static void
p_single_clip_playback(GapStbFrameWidget *fw)
{
  gchar *imagename;
  GapStbTabWidgets *tabw;
  
  tabw = (GapStbTabWidgets *)fw->tabw;
  
  if((tabw) && (fw->sgpp))
  {
    imagename = gap_story_get_filename_from_elem(fw->stb_elem_refptr);
    p_story_call_player(fw->sgpp
	             ,NULL             /* No storyboard pointer */
		     ,imagename
		     ,tabw->thumb_width
		     ,tabw->thumb_height
		     ,-1	    /* image_id (unused in imagename based playback mode) */
		     ,fw->stb_elem_refptr->from_frame      /* play from */
		     ,fw->stb_elem_refptr->to_frame        /* play until */
		     ,TRUE                                 /* play all */
		     ,fw->stb_elem_refptr->seltrack
		     ,fw->stb_elem_refptr->delace
		     ,gap_story_get_preferred_decoder(fw->stb_refptr, fw->stb_elem_refptr)
		     );  
    g_free(imagename);
  }


}  /* end p_single_clip_playback */


/* ---------------------------------
 * p_frame_widget_preview_events_cb
 * ---------------------------------
 * handles events for all frame_widgets (in the dyn_table)
 * - Expose of pview drawing_area
 * - Doubleclick (goto)
 * - Selections (Singleclick, and key-modifiers)
 */
static gboolean
p_frame_widget_preview_events_cb (GtkWidget *widget,
                             GdkEvent  *event,
                             GapStbFrameWidget *fw)
{
  GdkEventExpose *eevent;
  GdkEventButton *bevent;

  if(fw == NULL)
  {
//    fw = (GapStbFrameWidget *) g_object_get_data (G_OBJECT (widget), "frame_widget");
//    if(fw == NULL)
//    {
      return FALSE;
//    }
  }

  if ((fw->stb_elem_refptr == NULL)
  ||  (fw->stb_refptr == NULL))
  {
    /* the frame_widget is not initialized or it is just a dummy, no action needed */
    return FALSE;
  }

  switch (event->type)
  {
    case GDK_2BUTTON_PRESS:
      if(gap_debug) printf("p_frame_widget_preview_events_cb GDK_2BUTTON_PRESS (doubleclick)\n");
      p_single_clip_playback(fw);
      return TRUE;

    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;

      if(gap_debug) printf("p_frame_widget_preview_events_cb GDK_BUTTON_PRESS button:%d seq_nr:%d widget:%d  da_wgt:%d\n"
                              , (int)bevent->button
                              , (int)fw->seq_nr
                              , (int)widget
                              , (int)fw->pv_ptr->da_widget
                              );
      if(fw->stb_elem_refptr == NULL)
      {
        /* perform no actions on dummy entries in the dyn_table */
        return TRUE;
      }

      if (bevent->button == 3)
      {
          /* right mousebutton (3) shows the PopUp Menu */
          //navi_ops_menu_set_sensitive();
          //naviD->paste_at_frame = fw->frame_nr;
          //gtk_menu_popup (GTK_MENU (naviD->ops_menu),
          //             NULL, NULL, NULL, NULL,
          //             3, bevent->time);

        // XXXXXXXXXXXXXX debug
        {
          gap_story_fw_properties_dialog(fw);

          return TRUE;
	}
      }

      /*  handle selctions, according to modifier keys */
      if (event->button.state & GDK_CONTROL_MASK)
      {
         if(gap_debug) printf("p_frame_widget_preview_events_cb SELECTION GDK_CONTROL_MASK (ctrl modifier)\n");
         p_selection_add(fw);
      }
      else if (event->button.state & GDK_SHIFT_MASK)
      {
         if(gap_debug) printf("p_frame_widget_preview_events_cb SELECTION GDK_SHIFT_MASK  (shift modifier)\n");
         p_selection_extend(fw);
      }
      else if (event->button.state & GDK_MOD1_MASK)
      {
         if(gap_debug) printf("p_frame_widget_preview_events_cb SELECTION GDK_MOD1_MASK  (alt modifier)\n");
         p_selection_add(fw);
      }
      else
      {
         if(gap_debug) printf("p_frame_widget_preview_events_cb SELECTION (no modifier)\n");
	 p_selection_replace(fw);
      }

      break;

    case GDK_EXPOSE:
      if(gap_debug) printf("p_frame_widget_preview_events_cb GDK_EXPOSE seq_nr:%d widget:%d  da_wgt:%d\n"
                              , (int)fw->seq_nr
                              , (int)widget
                              , (int)fw->pv_ptr->da_widget
                              );

      eevent = (GdkEventExpose *) event;

      if(widget == fw->pv_ptr->da_widget)
      {
        //navi_preview_extents();
        //navi_frame_widget_time_label_update(fw);
        gap_pview_repaint(fw->pv_ptr);
        gdk_flush ();
      }

      break;

    default:
      break;
  }

  return FALSE;
}       /* end  p_frame_widget_preview_events_cb */


/* -----------------------------------
 * p_tabw_destroy_properties_dlg
 * -----------------------------------
 * destroy the open property dialog(s) attached to tabw 
 * destroy_all == FALSE: destroy only the invalid elents
 *                       (that are no members of the tabw->xxx list)
 * destroy_all == FALSE: destroy all elents
 */
static void
p_tabw_destroy_properties_dlg (GapStbTabWidgets *tabw, gboolean destroy_all)
{
  GapStbPropWidget *pw;
  GapStbPropWidget *pw_prev;
  GapStbPropWidget *pw_next;
  GapStoryBoard    *stb;

  if(tabw==NULL) { return; }
  stb = p_tabw_get_stb_ptr(tabw);
  if(stb==NULL)  { return; }

  pw_prev = NULL;
  for(pw = tabw->pw; pw != NULL; pw = pw_next)
  {
    pw_next = (GapStbPropWidget *)pw->next;
 
    if(pw->stb_elem_bck)
    {
      gap_story_elem_free(&pw->stb_elem_bck);
    }

    if(pw->stb_elem_refptr)
    {
      gboolean destroy_elem;

      destroy_elem = TRUE;
      if(!destroy_all)
      {
        GapStoryElem *stb_elem;

	for(stb_elem = stb->stb_elem; stb_elem != NULL;  stb_elem = stb_elem->next)
	{
	  if(stb_elem == pw->stb_elem_refptr)
	  {
	    destroy_elem = FALSE;
	    break;
	  }
	}
      }
      
      if(destroy_elem)
      {
        if(pw->pw_prop_dialog) 
	{
	  gtk_widget_destroy(pw->pw_prop_dialog);
	}
	if(pw_prev)
	{
	  pw_prev->next = pw_next;
	}
	else
	{
	  tabw->pw = pw_next;
	}
	g_free(pw);
      }
      else
      {
        pw_prev = pw;
      }
    }
    
  }
}  /* end p_tabw_destroy_properties_dlg */ 


/* ---------------------------------
 * p_tabw_edit_cut_cb
 * ---------------------------------
 */
static void
p_tabw_edit_cut_cb ( GtkWidget *w
		    , GapStbTabWidgets *tabw)
{
  GapStbMainGlobalParams  *sgpp;
  GapStoryBoard *stb_dst;

  if(gap_debug) printf("p_tabw_edit_cut_cb\n");

  sgpp = (GapStbMainGlobalParams  *)tabw->sgpp;
  if(sgpp == NULL) { return; }

  
  if(sgpp->curr_selection)
  {
     gap_story_free_storyboard(&sgpp->curr_selection);
  }

  stb_dst = p_tabw_get_stb_ptr(tabw);
  if(stb_dst)
  {
    sgpp->curr_selection = gap_story_duplicate_sel_only(stb_dst);
    gap_story_remove_sel_elems(stb_dst);

    p_tabw_destroy_properties_dlg (tabw, FALSE /* DONT destroy_all*/ );

    /* refresh storyboard layout and thumbnail list widgets */
    p_recreate_tab_widgets( stb_dst
                         ,tabw
			 ,tabw->mount_col
			 ,tabw->mount_row
			 ,sgpp
			 );
    p_render_all_frame_widgets(tabw);
    p_widget_sensibility(sgpp);
  }
  
  
}  /* end p_tabw_edit_cut_cb */

/* ---------------------------------
 * p_tabw_edit_copy_cb
 * ---------------------------------
 */
static void
p_tabw_edit_copy_cb ( GtkWidget *w
		    , GapStbTabWidgets *tabw)
{
  GapStbMainGlobalParams  *sgpp;
  GapStoryBoard *stb_dst;

  if(gap_debug) printf("p_tabw_edit_copy_cb\n");

  sgpp = (GapStbMainGlobalParams  *)tabw->sgpp;
  if(sgpp == NULL) { return; }

  
  if(sgpp->curr_selection)
  {
     gap_story_free_storyboard(&sgpp->curr_selection);
  }

  stb_dst = p_tabw_get_stb_ptr(tabw);
  if(stb_dst)
  {
    sgpp->curr_selection = gap_story_duplicate_sel_only(stb_dst);
  }

  p_widget_sensibility(sgpp);
  
}  /* end p_tabw_edit_copy_cb */

/* ---------------------------------
 * p_tabw_edit_paste_cb
 * ---------------------------------
 * paste after the last selected element
 * (or append at end of list if there was no selection)
 */
static void
p_tabw_edit_paste_cb ( GtkWidget *w
		    , GapStbTabWidgets *tabw)
{
  GapStbMainGlobalParams  *sgpp;
  GapStoryBoard *stb_dst;
  gint32         story_id;

  if(gap_debug) printf("p_tabw_edit_paste_cb\n");
  sgpp = (GapStbMainGlobalParams  *)tabw->sgpp;
  if(sgpp == NULL) { return; }

  stb_dst = p_tabw_get_stb_ptr(tabw);
  if(stb_dst)
  {
    story_id = gap_story_find_last_selected(stb_dst);
    p_tabw_edit_paste_at_story_id(sgpp
                               ,stb_dst
			       ,tabw
			       ,story_id
			       ,TRUE      /* insert_after == TRUE */
			       );
  }
			       
}  /* end p_tabw_edit_paste_cb */

/* ---------------------------------
 * p_tabw_edit_paste_at_story_id
 * ---------------------------------
 */
static void
p_tabw_edit_paste_at_story_id (GapStbMainGlobalParams  *sgpp
                    , GapStoryBoard *stb_dst
		    , GapStbTabWidgets *tabw
		    , gint32 story_id
		    , gboolean insert_after)
{
  GapStoryBoard *stb_sel_dup;

  if(gap_debug) printf("p_tabw_edit_paste_cb\n");

  if(sgpp == NULL) { return; }
  if(sgpp->curr_selection == NULL) { return; }
  
  stb_sel_dup = gap_story_duplicate(sgpp->curr_selection);
  if(stb_sel_dup)
  {
    /* clear all selections of the destination list before pasting into it */
    gap_story_selection_all_set(stb_dst, FALSE);
    
    gap_story_lists_merge(stb_dst
                      , stb_sel_dup
		      , story_id
		      , insert_after
		      );
    gap_story_free_storyboard(&stb_sel_dup);
  }
  //
  printf("p_tabw_edit_paste_at_story_id: TODO: check if pasted elem is off screen and scroll if TRUE\n");


  /* refresh storyboard layout and thumbnail list widgets */
  p_recreate_tab_widgets( stb_dst
                         ,tabw
			 ,tabw->mount_col
			 ,tabw->mount_row
			 ,sgpp
			 );
  p_render_all_frame_widgets(tabw);

}  /* end p_tabw_edit_paste_at_story_id */

/* ---------------------------------
 * p_tabw_update_selection
 * ---------------------------------
 */
static void
p_tabw_update_selection(GapStbTabWidgets  *tabw)
{
  GapStbFrameWidget *fw;
  GapStbMainGlobalParams  *sgpp;
  GdkColor      *bg_color;
  GdkColor      *fg_color;
  gint ii;

  if(tabw == NULL) { return; }

  sgpp = (GapStbMainGlobalParams  *)tabw->sgpp;
  if(sgpp == NULL) { return; }

  for(ii=0; ii < tabw->fw_tab_size; ii++)
  {
    fw = tabw->fw_tab[ii];

    bg_color = &sgpp->shell_window->style->bg[GTK_STATE_NORMAL];
    fg_color = &sgpp->shell_window->style->fg[GTK_STATE_NORMAL];
    if(fw->stb_elem_refptr)
    {
      if(fw->stb_elem_refptr->selected)
      {
	 bg_color = &sgpp->shell_window->style->bg[GTK_STATE_SELECTED];
	 fg_color = &sgpp->shell_window->style->fg[GTK_STATE_SELECTED];
      }
    }

    if(gap_debug) printf("p_tabw_update_selection: GTK_STYLE_SET_BACKGROUND bg_color: %d\n", (int)bg_color);

    /* Note: Gtk does not know about selcted items, since selections are handled
     * external by gap_navigator_dialog code.
     * using SELECTED or NORMAL color from the shell window to paint this private selection
     * (must use always the NORMAL state here, because for Gtk the event_box is never selected)
     */
    gtk_widget_modify_bg(fw->event_box, GTK_STATE_NORMAL, bg_color);
    gtk_widget_modify_fg(fw->key_label, GTK_STATE_NORMAL, fg_color);
    gtk_widget_modify_fg(fw->val_label, GTK_STATE_NORMAL, fg_color);
  }

}  /* end p_tabw_update_selection */



/* ---------------------------------
 * p_selection_add
 * ---------------------------------
 */
static void
p_selection_add(GapStbFrameWidget *fw)
{
  GapStbTabWidgets  *tabw;
  GapStbMainGlobalParams *sgpp;

  tabw = (GapStbTabWidgets  *)fw->tabw;
  if(tabw == NULL)   { return; }
  
  sgpp = (GapStbMainGlobalParams *)tabw->sgpp;
  if(sgpp == NULL)   { return; }

  /* clear all selected flags in the other list */
  if(tabw->type == GAP_STB_MASTER_TYPE_STORYBOARD)
  {
    gap_story_selection_all_set(sgpp->cll, FALSE);
  }
  else
  {
    gap_story_selection_all_set(sgpp->stb, FALSE);
  }
  
  if(fw->stb_elem_refptr)
  {
      if(fw->stb_elem_refptr->selected)
      {
         fw->stb_elem_refptr->selected = FALSE;
      }
      else
      {
         fw->stb_elem_refptr->selected = TRUE;
      }
  }

  /* update both list */
  p_tabw_update_selection(sgpp->cll_widgets);
  p_tabw_update_selection(sgpp->stb_widgets);

  p_widget_sensibility(sgpp);  
}  /* end p_selection_add */


/* ---------------------------------
 * p_selection_extend
 * ---------------------------------
 */
static void
p_selection_extend(GapStbFrameWidget *fw)
{
  GapStbTabWidgets  *tabw;
  GapStbMainGlobalParams *sgpp;
  GapStoryBoard *stb;
  GapStoryElem *stb_elem;
  gint32 story_id;
  gint32 last_sel_story_id;
  gboolean sel_state;

  tabw = (GapStbTabWidgets  *)fw->tabw;
  if(tabw == NULL)   { return; }
  
  sgpp = (GapStbMainGlobalParams *)tabw->sgpp;
  if(sgpp == NULL)   { return; }

  /* clear all selected flags in the other list */
  if(tabw->type == GAP_STB_MASTER_TYPE_STORYBOARD)
  {
    gap_story_selection_all_set(sgpp->cll, FALSE);
  }
  else
  {
    gap_story_selection_all_set(sgpp->stb, FALSE);
  }

  stb = p_tabw_get_stb_ptr(tabw);
  if(stb == NULL)   { return; }

  if(fw->stb_elem_refptr == NULL)   { return; }

  last_sel_story_id = gap_story_find_last_selected(stb);

  fw->stb_elem_refptr->selected = TRUE;      /* always select the clicked element */
  story_id = fw->stb_elem_refptr->story_id;

  sel_state = FALSE;
  
  if((last_sel_story_id >= 0) && (story_id != last_sel_story_id))
  {
    for(stb_elem = stb->stb_elem; stb_elem != NULL;  stb_elem = stb_elem->next)
    {
      if((stb_elem->story_id == last_sel_story_id))
      {
        if(story_id == -1)
	{
            break;
	}
	sel_state = TRUE;
	last_sel_story_id = -1;
      }
      if((stb_elem->story_id == story_id))
      {
	if(last_sel_story_id == -1)
	{
          break;
        }
	sel_state = TRUE;
	story_id = -1;
      }
      if(stb_elem->selected)
      {
	/* set selected beginning at the 1.st already seleected elem 
	 * and all further elements until element with story_id
	 */
	sel_state = TRUE;
      }
      stb_elem->selected = sel_state;
    }
  }

  /* update both list */
  p_tabw_update_selection(sgpp->cll_widgets);
  p_tabw_update_selection(sgpp->stb_widgets);
  p_widget_sensibility(sgpp);
}  /* end p_selection_extend */

/* ---------------------------------
 * p_selection_replace
 * ---------------------------------
 */
static void
p_selection_replace(GapStbFrameWidget *fw)
{
  GapStbTabWidgets  *tabw;
  GapStbMainGlobalParams *sgpp;
  gboolean   old_selection_state;

  old_selection_state = FALSE;
  if(fw->stb_elem_refptr)
  {
    old_selection_state = fw->stb_elem_refptr->selected;
  }
 
  tabw = (GapStbTabWidgets  *)fw->tabw;
  if(tabw == NULL)   { return; }
  
  sgpp = (GapStbMainGlobalParams *)tabw->sgpp;
  if(sgpp == NULL)   { return; }
  
  /* clear all selected flags in both lists */
  gap_story_selection_all_set(sgpp->stb, FALSE);
  gap_story_selection_all_set(sgpp->cll, FALSE);
 
  if(fw->stb_elem_refptr)
  {
    fw->stb_elem_refptr->selected = old_selection_state;
  }
 
  p_selection_add(fw);
  
}  /* end p_selection_replace */



/* -----------------------------
 * p_tabw_sensibility
 * -----------------------------
 */
static void
p_tabw_sensibility (GapStbMainGlobalParams *sgpp
                   ,GapStoryBoard *stb
		   ,GapStbTabWidgets *tabw)
{
  gboolean l_sensitive;
  gboolean l_sensitive_cc;

  if(sgpp == NULL) { return; }
  if(tabw == NULL) { return; }
  if(sgpp->initialized == FALSE) { return; }

  l_sensitive = FALSE;
  if(stb)
  {
    if(stb->stb_elem)
    {
      l_sensitive  = TRUE;
    }
  }  

  if(tabw->save_button)  gtk_widget_set_sensitive(tabw->save_button, l_sensitive);
  if(tabw->play_button)  gtk_widget_set_sensitive(tabw->play_button, l_sensitive);
 
  l_sensitive  = FALSE;
  l_sensitive_cc = FALSE;
  if(stb)
  {
    GapStoryElem      *stb_elem;
 
    /* check if there is at least one elem in the paste buffer */
    if(sgpp->curr_selection)
    {
      if(sgpp->curr_selection->stb_elem)
      {
        l_sensitive  = TRUE;
      }
    }
    
    /* check if there is at least one selected element */
    for(stb_elem = stb->stb_elem; stb_elem != NULL;  stb_elem = stb_elem->next)
    {
      if(stb_elem->selected)
       {
        l_sensitive_cc = TRUE;
	break;
      }
    }
  }

  if(tabw->edit_paste_button) gtk_widget_set_sensitive(tabw->edit_paste_button, l_sensitive);

  if(tabw->edit_cut_button) gtk_widget_set_sensitive(tabw->edit_cut_button, l_sensitive_cc);
  if(tabw->edit_copy_button) gtk_widget_set_sensitive(tabw->edit_copy_button, l_sensitive_cc);
  
}  /* end p_tabw_sensibility */


/* -----------------------------
 * p_widget_sensibility
 * -----------------------------
 */
static void
p_widget_sensibility (GapStbMainGlobalParams *sgpp)
{
  gboolean l_sensitive;
  gboolean l_sensitive_add;
  gboolean l_sensitive_encode;

  if(gap_debug) printf("p_tabw_set_sensibility\n");

  if(sgpp == NULL) { return; }
  if(sgpp->initialized == FALSE) { return; }

  l_sensitive  = FALSE;
  l_sensitive_add = FALSE;
  l_sensitive_encode = FALSE;
  if(sgpp->cll)
  {
    l_sensitive_add = TRUE;
    if(sgpp->cll->stb_elem)
    {
      l_sensitive  = TRUE;
      if(!sgpp->cll->unsaved_changes)
      {
        /* encoder call only available when saved */
        l_sensitive_encode = TRUE;
      }
    }
  }
  if(sgpp->menu_item_cll_save)       gtk_widget_set_sensitive(sgpp->menu_item_cll_save, l_sensitive);
  if(sgpp->menu_item_cll_save_as)    gtk_widget_set_sensitive(sgpp->menu_item_cll_save_as, l_sensitive);
  if(sgpp->menu_item_cll_playback)   gtk_widget_set_sensitive(sgpp->menu_item_cll_playback, l_sensitive);
  if(sgpp->menu_item_cll_properties) gtk_widget_set_sensitive(sgpp->menu_item_cll_properties, l_sensitive);
  if(sgpp->menu_item_cll_encode)     gtk_widget_set_sensitive(sgpp->menu_item_cll_close, l_sensitive);
  if(sgpp->menu_item_cll_close)      gtk_widget_set_sensitive(sgpp->menu_item_cll_encode, l_sensitive_encode);

  if(sgpp->menu_item_cll_add_clip)   gtk_widget_set_sensitive(sgpp->menu_item_cll_add_clip, l_sensitive_add);
  
  if(sgpp->cll_widgets)
  {
    p_tabw_sensibility(sgpp, sgpp->cll, sgpp->cll_widgets);
  }
  
  
  l_sensitive  = FALSE;
  l_sensitive_add = FALSE;
  l_sensitive_encode = FALSE;
  if(sgpp->stb)
  {
    l_sensitive_add = TRUE;
    if(sgpp->stb->stb_elem)
    {
      l_sensitive  = TRUE;
      if(!sgpp->stb->unsaved_changes)
      {
        /* encoder call only available when saved */
        l_sensitive_encode = TRUE;
      }
    }
  }
  if(sgpp->menu_item_stb_save)       gtk_widget_set_sensitive(sgpp->menu_item_stb_save, l_sensitive);
  if(sgpp->menu_item_stb_save_as)    gtk_widget_set_sensitive(sgpp->menu_item_stb_save_as, l_sensitive);
  if(sgpp->menu_item_stb_playback)   gtk_widget_set_sensitive(sgpp->menu_item_stb_playback, l_sensitive);
  if(sgpp->menu_item_stb_properties) gtk_widget_set_sensitive(sgpp->menu_item_stb_properties, l_sensitive);
  if(sgpp->menu_item_stb_encode)     gtk_widget_set_sensitive(sgpp->menu_item_stb_encode, l_sensitive);
  if(sgpp->menu_item_stb_close)      gtk_widget_set_sensitive(sgpp->menu_item_stb_close, l_sensitive);

  if(sgpp->menu_item_stb_add_clip)   gtk_widget_set_sensitive(sgpp->menu_item_stb_add_clip, l_sensitive_add);
  
  if(sgpp->stb_widgets)
  {
    p_tabw_sensibility(sgpp, sgpp->stb, sgpp->stb_widgets);
  }

}  /* end p_widget_sensibility */ 


/* -----------------------------
 * p_close_one_or_both_lists
 * -----------------------------
 * check for unsaved changes and ask for close permission if there are such changes.
 *
 * return: FALSE if there were unsaved changes 
 *               AND the user has REFUSED permission to close
 *               (by pressing the CANCEL Button)
 *
 * return: TRUE  if the close was performed.
 */
static gboolean
p_close_one_or_both_lists(GapStbMainGlobalParams *sgpp
                         ,gboolean check_cliplist
			 ,gboolean check_storyboard
			 )
{
  gboolean  l_rc;
  
  l_rc = TRUE;
  if(sgpp)
  {
    l_rc = p_check_unsaved_changes(sgpp
                              , check_cliplist
			      , check_storyboard
			      );
    if(l_rc)
    {
      if(check_cliplist)
      {
        p_tabw_destroy_properties_dlg (sgpp->cll_widgets, TRUE /* destroy_all*/ );
	/* drop the old cliplist structure */
	gap_story_free_storyboard(&sgpp->cll);
	p_recreate_tab_widgets( sgpp->cll
                           ,sgpp->cll_widgets
			   ,sgpp->cll_widgets->mount_col
			   ,sgpp->cll_widgets->mount_row
			   ,sgpp
			   );
      }
      
      if(check_storyboard)
      {
        p_tabw_destroy_properties_dlg (sgpp->stb_widgets, TRUE /* destroy_all*/ );
	/* drop the old storyboard structure */
	gap_story_free_storyboard(&sgpp->stb);
	p_recreate_tab_widgets( sgpp->stb
                           ,sgpp->stb_widgets
			   ,sgpp->stb_widgets->mount_col
			   ,sgpp->stb_widgets->mount_row
			   ,sgpp
			   );
      }
      
    }
  } 
  return(l_rc);
}  /* end p_close_one_or_both_lists */



/* -----------------------------
 * p_menu_win_quit_cb
 * -----------------------------
 */
static void
p_menu_win_quit_cb (GtkWidget *widget, GapStbMainGlobalParams *sgpp)
{
  if(gap_debug) printf("p_menu_win_quit_cb\n");

  if(p_close_one_or_both_lists(sgpp
                              , TRUE  /* check_cliplist */
			      , TRUE  /* check_storyboard */
			      ))
  {
    story_dlg_response(widget, GTK_RESPONSE_CANCEL, sgpp);
  }

}  /* end p_menu_win_quit_cb */ 

/* -----------------------------
 * p_menu_win_help_cb
 * -----------------------------
 */
static void
p_menu_win_help_cb (GtkWidget *widget, GapStbMainGlobalParams *sgpp)
{
  if(gap_debug) printf("p_menu_win_help_cb\n");

  if(sgpp)
  {
    gimp_standard_help_func(GAP_STORYBOARD_EDIT_HELP_ID, sgpp->shell_window);
  }

}  /* end p_menu_win_help_cb */ 

/* -----------------------------
 * p_menu_win_properties_cb
 * -----------------------------
 */
static void
p_menu_win_properties_cb (GtkWidget *widget, GapStbMainGlobalParams *sgpp)
{
  if(gap_debug) printf("p_menu_win_properties_cb\n");

}  /* end p_menu_win_properties_cb */ 


/* -----------------------------
 * p_menu_win_vthumbs_toggle_cb
 * -----------------------------
 */
static void
p_menu_win_vthumbs_toggle_cb (GtkWidget *widget, GapStbMainGlobalParams *sgpp)
{
  if(gap_debug) printf("p_menu_win_vthumbs_toggle_cb\n");
  if(sgpp)
  {
    if(sgpp->menu_item_win_vthumbs)
    {
#ifndef GAP_ENABLE_VIDEOAPI_SUPPORT
      g_message(_("GIMP-GAP is compiled without videoapi support. "
                  "Therfore thumbnails for videoframes can not be displayed."));
#endif
      sgpp->auto_vthumb = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(sgpp->menu_item_win_vthumbs));
      if(sgpp->stb_widgets)
      {
	p_render_all_frame_widgets(sgpp->stb_widgets);
      }
      if(sgpp->cll_widgets)
      {
	p_render_all_frame_widgets(sgpp->cll_widgets);
      }
    }
  }

}  /* end p_menu_win_vthumbs_toggle_cb */ 




/* -----------------------------
 * p_menu_cll_new_cb
 * -----------------------------
 */
static void
p_menu_cll_new_cb (GtkWidget *widget, GapStbMainGlobalParams *sgpp)
{
  if(gap_debug) printf("p_menu_cll_new_cb\n");
  
  if(p_close_one_or_both_lists(sgpp
                           , TRUE   /* check_cliplist */
			   , FALSE  /* check_storyboard */
			   ))
  {
    sgpp->cll = gap_story_new_story_board(NULL);
    sgpp->cll->master_type = GAP_STB_MASTER_TYPE_CLIPLIST;
    p_tabw_master_prop_dialog(sgpp->cll_widgets);

  }
}  /* end p_menu_cll_new_cb */ 

/* -----------------------------
 * p_menu_cll_open_cb
 * -----------------------------
 */
static void
p_menu_cll_open_cb (GtkWidget *widget, GapStbMainGlobalParams *sgpp)
{
  if(gap_debug) printf("p_menu_cll_open_cb\n");
  if(sgpp->cll_widgets)
  {
      p_tabw_load_file_cb (widget, sgpp->cll_widgets);
  }
  
}  /* end p_menu_cll_open_cb */ 



/* -----------------------------
 * p_menu_cll_save_cb
 * -----------------------------
 */
static void
p_menu_cll_save_cb (GtkWidget *widget, GapStbMainGlobalParams *sgpp)
{
  gboolean l_save_ok;
  gint l_errno;

  if(gap_debug) printf("p_menu_cll_save_cb\n");
  
  if(sgpp->cll == NULL)          { return; }
  if(sgpp->cll->errtext != NULL)
  {
    printf("p_menu_cll_save_cb: ** INTERNAL ERROR save of corrupted storyboard canceled\n");
    return;
  }
	
  /* save cliplist as new filename */
  l_save_ok = gap_story_save(sgpp->cll, sgpp->cliplist_filename);
  l_errno = errno;

  if(!l_save_ok)
  {
     g_message (_("Failed to write cliplistfile\n"
                  "filename: '%s':\n%s")
	       ,  sgpp->cliplist_filename, g_strerror (l_errno));
  }
  
  p_tabw_update_frame_label(sgpp->cll_widgets, sgpp);
}  /* end p_menu_cll_save_cb */ 

/* -----------------------------
 * p_menu_cll_save_cb_as
 * -----------------------------
 */
static void
p_menu_cll_save_cb_as (GtkWidget *widget, GapStbMainGlobalParams *sgpp)
{
  if(gap_debug) printf("p_menu_cll_save_cb_as\n");
  p_tabw_file_save_as_cb(widget, sgpp->cll_widgets);
  p_tabw_update_frame_label(sgpp->cll_widgets, sgpp);
}  /* end p_menu_cll_save_cb_as */ 

/* -----------------------------
 * p_menu_cll_playback_cb
 * -----------------------------
 */
static void
p_menu_cll_playback_cb (GtkWidget *widget, GapStbMainGlobalParams *sgpp)
{
  if(gap_debug) printf("p_menu_cll_playback_cb\n");
  p_player_cll_mode_cb(widget, NULL, sgpp);
}  /* end p_menu_cll_playback_cb */ 

/* -----------------------------
 * p_menu_cll_properties_cb
 * -----------------------------
 */
static void
p_menu_cll_properties_cb (GtkWidget *widget, GapStbMainGlobalParams *sgpp)
{
  if(gap_debug) printf("p_menu_cll_properties_cb\n");
  p_tabw_master_prop_dialog(sgpp->cll_widgets);
}  /* end p_menu_cll_properties_cb */ 

/* -----------------------------
 * p_menu_cll_add_elem_cb
 * -----------------------------
 */
static void
p_menu_cll_add_elem_cb (GtkWidget *widget, GapStbMainGlobalParams *sgpp)
{
  if(gap_debug) printf("p_menu_cll_add_elem_cb\n");

  if(sgpp == NULL) { return; }
  p_tabw_add_elem(sgpp->cll_widgets, sgpp, sgpp->cll);
}  /* end p_menu_cll_add_elem_cb */ 

/* -----------------------------
 * p_menu_cll_toggle_edmode
 * -----------------------------
 */
static void
p_menu_cll_toggle_edmode (GtkWidget *widget, GapStbMainGlobalParams *sgpp)
{
  if(gap_debug) printf("p_menu_cll_close_cb\n");

  if(sgpp->cll_widgets)
  {
    switch(sgpp->cll_widgets->edmode)
    {
      case GAP_STB_EDMO_SEQUENCE_NUMBER:
        sgpp->cll_widgets->edmode = GAP_STB_EDMO_FRAME_NUMBER;
	break;
      case GAP_STB_EDMO_FRAME_NUMBER:
        sgpp->cll_widgets->edmode = GAP_STB_EDMO_TIMECODE;
	break;
      default:
        sgpp->cll_widgets->edmode = GAP_STB_EDMO_SEQUENCE_NUMBER;
	break;
    }
    p_render_all_frame_widgets(sgpp->cll_widgets);
  }
}  /* end p_menu_cll_toggle_edmode */ 


/* -----------------------------
 * p_menu_cll_encode_cb
 * -----------------------------
 */
static void
p_menu_cll_encode_cb (GtkWidget *widget, GapStbMainGlobalParams *sgpp)
{
  if(gap_debug) printf("p_menu_cll_encode_cb\n");

  p_call_master_encoder(sgpp, sgpp->cll);
}  /* end p_menu_cll_encode_cb */ 


/* -----------------------------
 * p_menu_cll_close_cb
 * -----------------------------
 */
static void
p_menu_cll_close_cb (GtkWidget *widget, GapStbMainGlobalParams *sgpp)
{
  if(gap_debug) printf("p_menu_cll_close_cb\n");

  p_close_one_or_both_lists(sgpp
                           , TRUE   /* check_cliplist */
			   , FALSE  /* check_storyboard */
			   );
}  /* end p_menu_cll_close_cb */ 










/* -----------------------------
 * p_menu_stb_new_cb
 * -----------------------------
 */
static void
p_menu_stb_new_cb (GtkWidget *widget, GapStbMainGlobalParams *sgpp)
{
  if(gap_debug) printf("p_menu_stb_new_cb\n");
  
  if(p_close_one_or_both_lists(sgpp
                           , FALSE  /* check_cliplist */
			   , TRUE   /* check_storyboard */
			   ))
  {
    sgpp->stb = gap_story_new_story_board(NULL);
    sgpp->stb->master_type = GAP_STB_MASTER_TYPE_STORYBOARD;
    p_tabw_master_prop_dialog(sgpp->stb_widgets);
  }
}  /* end p_menu_stb_new_cb */ 


/* -----------------------------
 * p_menu_stb_open_cb
 * -----------------------------
 */
static void
p_menu_stb_open_cb (GtkWidget *widget, GapStbMainGlobalParams *sgpp)
{
  if(gap_debug) printf("p_menu_stb_open_cb\n");
  if(sgpp->stb_widgets)
  {
      p_tabw_load_file_cb (widget, sgpp->stb_widgets);
  }
  
}  /* end p_menu_stb_open_cb */ 



/* -----------------------------
 * p_menu_stb_save_cb
 * -----------------------------
 */
static void
p_menu_stb_save_cb (GtkWidget *widget, GapStbMainGlobalParams *sgpp)
{
  gboolean l_save_ok;
  gint l_errno;

  if(gap_debug) printf("p_menu_stb_save_cb\n");
  
  if(sgpp->stb == NULL)          { return; }
  if(sgpp->stb->errtext != NULL)
  {
    printf("p_menu_stb_save_cb: ** INTERNAL ERROR save of corrupted storyboard canceled\n");
    return;
  }
	
  /* save storyboard as new filename */
  l_save_ok = gap_story_save(sgpp->stb, sgpp->storyboard_filename);
  l_errno = errno;

  if(!l_save_ok)
  {
     g_message (_("Failed to write storyboardfile\n"
                  "filename: '%s':\n%s")
	       ,  sgpp->storyboard_filename, g_strerror (l_errno));
  }

  p_tabw_update_frame_label(sgpp->stb_widgets, sgpp);
}  /* end p_menu_stb_save_cb */ 

/* -----------------------------
 * p_menu_stb_save_cb_as
 * -----------------------------
 */
static void
p_menu_stb_save_cb_as (GtkWidget *widget, GapStbMainGlobalParams *sgpp)
{
  if(gap_debug) printf("p_menu_stb_save_cb_as\n");
  p_tabw_file_save_as_cb(widget, sgpp->stb_widgets);
  p_tabw_update_frame_label(sgpp->stb_widgets, sgpp);
}  /* end p_menu_stb_save_cb_as */ 

/* -----------------------------
 * p_menu_stb_playback_cb
 * -----------------------------
 */
static void
p_menu_stb_playback_cb (GtkWidget *widget, GapStbMainGlobalParams *sgpp)
{
  if(gap_debug) printf("p_menu_stb_playback_cb\n");
  p_player_stb_mode_cb(widget, NULL, sgpp);
}  /* end p_menu_stb_playback_cb */ 

/* -----------------------------
 * p_menu_stb_properties_cb
 * -----------------------------
 */
static void
p_menu_stb_properties_cb (GtkWidget *widget, GapStbMainGlobalParams *sgpp)
{
  if(gap_debug) printf("p_menu_stb_properties_cb\n");
  p_tabw_master_prop_dialog(sgpp->stb_widgets);
}  /* end p_menu_stb_properties_cb */ 

/* -----------------------------
 * p_menu_stb_add_elem_cb
 * -----------------------------
 */
static void
p_menu_stb_add_elem_cb (GtkWidget *widget, GapStbMainGlobalParams *sgpp)
{
  if(gap_debug) printf("p_menu_stb_add_elem_cb\n");

  if(sgpp == NULL) { return; }
  p_tabw_add_elem(sgpp->stb_widgets, sgpp, sgpp->stb);
}  /* end p_menu_stb_add_elem_cb */ 

/* -----------------------------
 * p_menu_stb_toggle_edmode
 * -----------------------------
 */
static void
p_menu_stb_toggle_edmode (GtkWidget *widget, GapStbMainGlobalParams *sgpp)
{
  if(gap_debug) printf("p_menu_stb_close_cb\n");

  if(sgpp->stb_widgets)
  {
    switch(sgpp->stb_widgets->edmode)
    {
      case GAP_STB_EDMO_SEQUENCE_NUMBER:
        sgpp->stb_widgets->edmode = GAP_STB_EDMO_FRAME_NUMBER;
	break;
      case GAP_STB_EDMO_FRAME_NUMBER:
        sgpp->stb_widgets->edmode = GAP_STB_EDMO_TIMECODE;
	break;
      default:
        sgpp->stb_widgets->edmode = GAP_STB_EDMO_SEQUENCE_NUMBER;
	break;
    }
    p_render_all_frame_widgets(sgpp->stb_widgets);
  }
}  /* end p_menu_stb_toggle_edmode */ 


/* -----------------------------
 * p_menu_stb_encode_cb
 * -----------------------------
 */
static void
p_menu_stb_encode_cb (GtkWidget *widget, GapStbMainGlobalParams *sgpp)
{
  if(gap_debug) printf("p_menu_stb_encode_cb\n");

  p_call_master_encoder(sgpp, sgpp->stb);
}  /* end p_menu_stb_encode_cb */ 


/* -----------------------------
 * p_menu_stb_close_cb
 * -----------------------------
 */
static void
p_menu_stb_close_cb (GtkWidget *widget, GapStbMainGlobalParams *sgpp)
{
  if(gap_debug) printf("p_menu_stb_close_cb\n");

  p_close_one_or_both_lists(sgpp
                           , FALSE  /* check_cliplist */
			   , TRUE   /* check_storyboard */
			   );

}  /* end p_menu_stb_close_cb */ 



/* -----------------------------
 * p_make_menu_bar_item
 * -----------------------------
 */
static GtkWidget*
p_make_menu_bar_item(GtkWidget *menu_bar, gchar *label)
{
   GtkWidget *menu = gtk_menu_new();
   GtkWidget *item = gtk_menu_item_new_with_mnemonic(label);
   GtkWidget *tearoff = gtk_tearoff_menu_item_new();

   gtk_menu_shell_insert(GTK_MENU_SHELL(menu), tearoff, 0);
   gtk_widget_show(tearoff);
   gtk_widget_show(item);

   gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);
   gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), item);
//   gtk_menu_set_accel_group(GTK_MENU(menu), accelerator_group);

   return menu;
}  /* end p_make_menu_bar_item */



/* -----------------------------
 * p_make_item_with_image
 * -----------------------------
 */
static GtkWidget*
p_make_item_with_image(GtkWidget *parent, const gchar *stock_id, 
		     GapStbMenuCallbackFptr fptr, GapStbMainGlobalParams *sgpp)
{
   GtkWidget *item;

   item = gtk_image_menu_item_new_from_stock(stock_id
                                            , NULL         // accelerator_group
					    );

   gtk_menu_shell_append(GTK_MENU_SHELL(parent), item);
   gtk_widget_show(item);
   g_signal_connect(item, "activate", G_CALLBACK(fptr), sgpp);
   return item;
}  /* end p_make_item_with_image */


/* -----------------------------
 * p_make_item_with_label
 * -----------------------------
 */
static GtkWidget*
p_make_item_with_label(GtkWidget *parent, gchar *label, 
		     GapStbMenuCallbackFptr fptr, GapStbMainGlobalParams *sgpp)
{
   GtkWidget *item;

   item = gtk_menu_item_new_with_mnemonic(label);

   gtk_menu_shell_append(GTK_MENU_SHELL(parent), item);
   gtk_widget_show(item);
   g_signal_connect(item, "activate", G_CALLBACK(fptr), sgpp);
   return item;
}  /* end p_make_item_with_label */


/* -----------------------------
 * p_make_check_item_with_label
 * -----------------------------
 */
static GtkWidget*
p_make_check_item_with_label(GtkWidget *parent, gchar *label 
		     , GapStbMenuCallbackFptr fptr
		     , GapStbMainGlobalParams *sgpp
		     , gboolean initial_value
		     )
{
   GtkWidget *item;

   item = gtk_check_menu_item_new_with_mnemonic(label);
   gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), initial_value);

   gtk_menu_shell_append(GTK_MENU_SHELL(parent), item);
   gtk_widget_show(item);
   g_signal_connect(item, "activate", G_CALLBACK(fptr), sgpp);
   return item;
}  /* end p_make_check_item_with_label */

/* -----------------------------
 * p_make_menu_global
 * -----------------------------
 */
static void
p_make_menu_global(GapStbMainGlobalParams *sgpp, GtkWidget *menu_bar)
{
   GtkWidget *file_menu = p_make_menu_bar_item(menu_bar, _("Global"));
   
   p_make_item_with_label(file_menu, _("Properties")
                          , p_menu_win_properties_cb
			  , sgpp
			  );

   sgpp->menu_item_win_vthumbs =			  
   p_make_check_item_with_label(file_menu, _("Videothumbnails")
                          , p_menu_win_vthumbs_toggle_cb
			  , sgpp
			  , sgpp->auto_vthumb
			  );
			  
   p_make_item_with_image(file_menu, GTK_STOCK_QUIT
                          , p_menu_win_quit_cb
			  , sgpp
			  );
   
}  /* end p_make_menu_global */

/* -----------------------------
 * p_make_menu_help
 * -----------------------------
 */
static void
p_make_menu_help(GapStbMainGlobalParams *sgpp, GtkWidget *menu_bar)
{
   GtkWidget *file_menu = p_make_menu_bar_item(menu_bar, _("Help"));
   
   p_make_item_with_image(file_menu, GTK_STOCK_HELP
                          , p_menu_win_help_cb
			  , sgpp
			  );
}  /* end p_make_menu_help */

/* -----------------------------
 * p_make_menu_cliplist
 * -----------------------------
 */
static void
p_make_menu_cliplist(GapStbMainGlobalParams *sgpp, GtkWidget *menu_bar)
{
   GtkWidget *file_menu = p_make_menu_bar_item(menu_bar, _("Cliplist"));
   
   p_make_item_with_image(file_menu, GTK_STOCK_NEW
                          , p_menu_cll_new_cb
			  , sgpp
			  );

   p_make_item_with_image(file_menu, GTK_STOCK_OPEN
                          , p_menu_cll_open_cb
			  , sgpp
			  );
   sgpp->menu_item_cll_save =			  
   p_make_item_with_image(file_menu, GTK_STOCK_SAVE
                          , p_menu_cll_save_cb
			  , sgpp
			  );

   sgpp->menu_item_cll_save_as =			  
   p_make_item_with_image(file_menu, GTK_STOCK_SAVE_AS
                          , p_menu_cll_save_cb_as
			  , sgpp
			  );

   sgpp->menu_item_cll_playback =			  
   p_make_item_with_label(file_menu, _("Playback")
                          , p_menu_cll_playback_cb
			  , sgpp
			  );

   sgpp->menu_item_cll_properties =			  
   p_make_item_with_label(file_menu, _("Properties")
                          , p_menu_cll_properties_cb
			  , sgpp
			  );

   sgpp->menu_item_cll_add_clip =			  
   p_make_item_with_label(file_menu, _("Create Clip")
                          , p_menu_cll_add_elem_cb
			  , sgpp
			  );

   p_make_item_with_label(file_menu, _("Toggle Unit")
                          , p_menu_cll_toggle_edmode
			  , sgpp
			  );

   sgpp->menu_item_cll_encode =			  
   p_make_item_with_label(file_menu, _("Encode")
                          , p_menu_cll_encode_cb
			  , sgpp
			  );
			  
   sgpp->menu_item_cll_close =			  
   p_make_item_with_image(file_menu, GTK_STOCK_CLOSE
                          , p_menu_cll_close_cb
			  , sgpp
			  );
  
}  /* end p_make_menu_cliplist */



/* -----------------------------
 * p_make_menu_storyboard
 * -----------------------------
 */
static void
p_make_menu_storyboard(GapStbMainGlobalParams *sgpp, GtkWidget *menu_bar)
{
   GtkWidget *file_menu = p_make_menu_bar_item(menu_bar, _("_Storyboard"));
   
   p_make_item_with_image(file_menu, GTK_STOCK_NEW
                          , p_menu_stb_new_cb
			  , sgpp
			  );

   p_make_item_with_image(file_menu, GTK_STOCK_OPEN
                          , p_menu_stb_open_cb
			  , sgpp
			  );
   sgpp->menu_item_stb_save =			  
   p_make_item_with_image(file_menu, GTK_STOCK_SAVE
                          , p_menu_stb_save_cb
			  , sgpp
			  );

   sgpp->menu_item_stb_save_as =			  
   p_make_item_with_image(file_menu, GTK_STOCK_SAVE_AS
                          , p_menu_stb_save_cb_as
			  , sgpp
			  );

   sgpp->menu_item_stb_playback =			  
   p_make_item_with_label(file_menu, _("Playback")
                          , p_menu_stb_playback_cb
			  , sgpp
			  );

   sgpp->menu_item_stb_properties =			  
   p_make_item_with_label(file_menu, _("Properties")
                          , p_menu_stb_properties_cb
			  , sgpp
			  );

   sgpp->menu_item_stb_add_clip =			  
   p_make_item_with_label(file_menu, _("Create Clip")
                          , p_menu_stb_add_elem_cb
			  , sgpp
			  );
 
   p_make_item_with_label(file_menu, _("Toggle Unit")
                          , p_menu_stb_toggle_edmode
			  , sgpp
			  );

   sgpp->menu_item_stb_encode =			  
   p_make_item_with_label(file_menu, _("Encode")
                          , p_menu_stb_encode_cb
			  , sgpp
			  );

   sgpp->menu_item_stb_close =			  
   p_make_item_with_image(file_menu, GTK_STOCK_CLOSE
                          , p_menu_stb_close_cb
			  , sgpp
			  );
   
}  /* end p_make_menu_storyboard */



/* -----------------------------
 * p_check_unsaved_changes
 * -----------------------------
 */
static gboolean
p_check_unsaved_changes (GapStbMainGlobalParams *sgpp
                           , gboolean check_cliplist
			   , gboolean check_storyboard
			   )
{
  static  GapArrButtonArg  l_argv[2];
  static  GapArrArg  argv[2];

  if(sgpp)
  {
    gint    l_rc;
    gchar  *l_cll_msg;
    gchar  *l_stb_msg;
    gint    l_ii;
    
    l_cll_msg = NULL;
    l_stb_msg = NULL;
    l_ii = 0;
    if((check_cliplist) && (sgpp->cll))
    {
      if(sgpp->cll->unsaved_changes)
      {
        l_cll_msg = g_strdup_printf(_("Unsaved cliplist changes '%s'")
	                       , sgpp->cliplist_filename);
        gap_arr_arg_init(&argv[l_ii], GAP_ARR_WGT_LABEL);
        argv[l_ii].label_txt = l_cll_msg;
        l_ii++;
      }
    }

    if((check_storyboard) && (sgpp->stb))
    {
      if(sgpp->stb->unsaved_changes)
      {
        l_stb_msg = g_strdup_printf(_("Unsaved storyboard changes '%s'")
	                        , sgpp->storyboard_filename);
        gap_arr_arg_init(&argv[l_ii], GAP_ARR_WGT_LABEL);
        argv[l_ii].label_txt = l_stb_msg;
        l_ii++;
      }
    }

    if(l_ii > 0)
    {
      if((check_storyboard) && (check_cliplist))
      {
        l_argv[1].but_txt  = _("Quit Storyboard");
      }
      else
      {
        l_argv[1].but_txt  = _("Continue");
      }
      
      l_argv[1].but_val  = 0;

      l_argv[0].but_txt  = GTK_STOCK_CANCEL;
      l_argv[0].but_val  = -1;


      l_rc = gap_arr_std_dialog ( _("Storyboard unsaved changes"),
                                    _("Storyboard unsaved changes"),
				     l_ii, argv,
				     2, l_argv, -1);

      if(l_cll_msg)  { g_free(l_cll_msg); }
      if(l_stb_msg)  { g_free(l_stb_msg); }

      if(l_rc < 0)
      {
	return (FALSE);   /* CANCEL was pressed, dont overwrite */
      }
    }
  }
  return (TRUE);

}  /* end p_check_unsaved_changes */ 


/* -----------------------------
 * p_tabw_update_frame_label
 * -----------------------------
 * display frame label,
 * if valid storyboard available add the filename
 * and show the string "(modified)" if there are unsaved changes
 *
 * then calculate max_rowpage 
 * and set upper limit for the tabw->rowpage_spinbutton_adj widget
 */
static void
p_tabw_update_frame_label (GapStbTabWidgets *tabw, GapStbMainGlobalParams *sgpp)
{
  gint32 l_max_rowpage;
  GapStoryBoard *stb_dst;

  if(tabw == NULL) { return; }
  if(sgpp == NULL) { return; }
  
  if((tabw->frame_with_name)
  && (tabw->filename_refptr))
  {
    gchar *l_hdr_txt = NULL;
    gchar *l_fil_txt = NULL;
    gchar *l_mod_txt = NULL;
    gchar *l_txt = NULL;

    l_fil_txt = g_strdup(" ");
    l_mod_txt = g_strdup(" ");

    if(tabw->type == GAP_STB_MASTER_TYPE_STORYBOARD)
    {
      l_hdr_txt = g_strdup(_("Storyboard"));
      if(sgpp->stb)
      {
        g_free(l_fil_txt);
        l_fil_txt = g_strdup(tabw->filename_refptr);
	if(sgpp->stb->unsaved_changes)
	{
          g_free(l_mod_txt);
	  l_mod_txt = g_strdup(_("(modified)"));
	}
      }
    }
    else
    {
      l_hdr_txt = g_strdup(_("Cliplist"));
      if(sgpp->cll)
      {
        g_free(l_fil_txt);
        l_fil_txt = g_strdup(tabw->filename_refptr);
	if(sgpp->cll->unsaved_changes)
	{
          g_free(l_mod_txt);
	  l_mod_txt = g_strdup(_("(modified)"));
	}
      }
    }
    l_txt = g_strdup_printf("%s: %s %s"
                           ,l_hdr_txt
			   ,l_fil_txt
			   ,l_mod_txt
			   );
    gtk_frame_set_label (GTK_FRAME (tabw->frame_with_name), l_txt);

    g_free(l_hdr_txt);
    g_free(l_fil_txt);
    g_free(l_mod_txt);
    g_free(l_txt);

  }


  l_max_rowpage = 1;
  stb_dst = p_tabw_get_stb_ptr(tabw);
  if(stb_dst)
  {
    gint32 l_act_elems = 0;
    
    l_act_elems = gap_story_count_active_elements(stb_dst, 1 /* track */ );   
    l_max_rowpage = p_get_max_rowpage(l_act_elems, tabw->cols, tabw->rows);
  }
  if(tabw->rowpage_spinbutton_adj)
  {
    GTK_ADJUSTMENT(tabw->rowpage_spinbutton_adj)->lower = 1;
    GTK_ADJUSTMENT(tabw->rowpage_spinbutton_adj)->upper = l_max_rowpage;
    if(GTK_ADJUSTMENT(tabw->rowpage_spinbutton_adj)->value > l_max_rowpage)
    {
      gtk_adjustment_set_value(GTK_ADJUSTMENT(tabw->rowpage_spinbutton_adj), (gdouble)l_max_rowpage);
    }
  }
  if(tabw->rowpage_vscale_adj)
  {
    gdouble upper_limit;
    gdouble lower_limit;
    gdouble page_increment;
    gdouble page_size;

    page_size = (gdouble)tabw->rows;
    page_increment = (gdouble)(page_size / 2);
    lower_limit = 1.0;
    upper_limit = l_max_rowpage + page_size;

    //if(gap_debug)
    {
      printf("\n######rowpage_vscale_adj : %d  tabw->rows:%d\n", (int)l_max_rowpage ,(int)tabw->rows);
      printf("lower_limit %f\n", (float)lower_limit );
      printf("upper_limit %f\n", (float)upper_limit );
      printf("page_size %f\n", (float) page_size);
      printf("page_increment %f\n", (float)page_increment );
      printf("value          %f\n", (float)GTK_ADJUSTMENT(tabw->rowpage_vscale_adj)->value );
    }

    GTK_ADJUSTMENT(tabw->rowpage_vscale_adj)->lower = lower_limit;
    GTK_ADJUSTMENT(tabw->rowpage_vscale_adj)->upper = upper_limit;
    GTK_ADJUSTMENT(tabw->rowpage_vscale_adj)->page_increment = page_increment;
    GTK_ADJUSTMENT(tabw->rowpage_vscale_adj)->page_size = page_size;
    if(GTK_ADJUSTMENT(tabw->rowpage_vscale_adj)->value > l_max_rowpage)
    {
      gtk_adjustment_set_value(GTK_ADJUSTMENT(tabw->rowpage_vscale_adj), (gdouble)l_max_rowpage);
    }

    if(lower_limit+page_size >= upper_limit)
    {
      gtk_widget_hide(tabw->rowpage_vscale);
    }
    else
    {
      gtk_widget_show(tabw->rowpage_vscale);
      gtk_widget_queue_draw(tabw->rowpage_vscale);
    }

  }
  
  if(tabw->total_rows_label)
  {
    char max_rowpage_txt[40];
    
    g_snprintf(max_rowpage_txt, sizeof(max_rowpage_txt), "%d", (int)l_max_rowpage);
    gtk_label_set_text(GTK_LABEL(tabw->total_rows_label), max_rowpage_txt);
  }
 
  p_widget_sensibility(sgpp);
  
}  /* end p_tabw_update_frame_label */ 






/* --------------------------------
 * p_vid_progress_callback
 * --------------------------------
 * return: TRUE: cancel videoapi immediate
 *         FALSE: continue
 */
static gboolean
p_vid_progress_callback(gdouble progress
                       ,gpointer user_data
                       )
{
  GapStbMainGlobalParams *sgpp;
  
  sgpp = (GapStbMainGlobalParams *)user_data;
  if(sgpp == NULL) { return (TRUE); }
  if(sgpp->progress_bar == NULL) { return (TRUE); }

  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(sgpp->progress_bar), progress);

  /* g_main_context_iteration makes sure that
   *  gtk does refresh widgets,  and react on events while the videoapi
   *  is busy with searching for the next frame.
   */
  while(g_main_context_iteration(NULL, FALSE));


  return(sgpp->cancel_video_api);

  /* return (TRUE); */ /* cancel video api if playback was stopped */
  
}  /* end p_vid_progress_callback */


/* --------------------------------
 * p_close_videofile
 * --------------------------------
 */
static void
p_close_videofile(GapStbMainGlobalParams *sgpp)
{
#ifdef GAP_ENABLE_VIDEOAPI_SUPPORT
  if(sgpp->gvahand)
  {
    if(sgpp->gva_videofile) g_free(sgpp->gva_videofile);
    GVA_close(sgpp->gvahand);

    sgpp->gvahand =  NULL;
    sgpp->gva_videofile = NULL;
  }
#endif
}  /* end p_close_videofile */


/* --------------------------------
 * p_open_videofile
 * --------------------------------
 */
static void
p_open_videofile(GapStbMainGlobalParams *sgpp
                , const char *filename
		, gint32 seltrack
		, const char *preferred_decoder
		)
{
#ifdef GAP_ENABLE_VIDEOAPI_SUPPORT
  gboolean vindex_permission;
  char *vindex_file;
  char *create_vindex_msg;
  
  p_close_videofile(sgpp);
  vindex_file = NULL;
  create_vindex_msg = NULL;

  sgpp->gvahand =  GVA_open_read_pref(filename
	                          , seltrack
				  , 1 /* aud_track */
				  , preferred_decoder
				  , FALSE  /* use MMX if available (disable_mmx == FALSE) */
				  );
  if(sgpp->gvahand)
  {
    sgpp->gva_videofile = g_strdup(filename);
    /* sgpp->gvahand->emulate_seek = TRUE; */
    sgpp->gvahand->do_gimp_progress = FALSE;

    sgpp->gvahand->progress_cb_user_data = sgpp;
    sgpp->gvahand->fptr_progress_callback = p_vid_progress_callback;

    /* set decoder name as progress idle text */
    {
      t_GVA_DecoderElem *dec_elem;

      dec_elem = (t_GVA_DecoderElem *)sgpp->gvahand->dec_elem;
      if(dec_elem->decoder_name)
      {
	create_vindex_msg = g_strdup_printf(_("Creating Index (decoder: %s)"), dec_elem->decoder_name);
        vindex_file = GVA_build_videoindex_filename(sgpp->gva_videofile
                                             ,1  /* track */
					     ,dec_elem->decoder_name
					     );
      }
      else
      {
	create_vindex_msg = g_strdup_printf(_("Creating Index"));
      }
    }

    sgpp->cancel_video_api = FALSE;

    /* check for permission to create a videoindex file */
    vindex_permission = gap_arr_create_vindex_permission(sgpp->gva_videofile, vindex_file);
    
    if(vindex_permission)
    {
printf("STORY: DEBUG: create vidindex start\n");
      if(sgpp->progress_bar)
      {
	 gtk_progress_bar_set_text(GTK_PROGRESS_BAR(sgpp->progress_bar), create_vindex_msg);
      }
      sgpp->gvahand->create_vindex = TRUE;
      GVA_count_frames(sgpp->gvahand);
printf("STORY: DEBUG: create vidindex done\n");
    }

    if(sgpp->progress_bar)
    {
       gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(sgpp->progress_bar), 0.0);
       if(sgpp->cancel_video_api)
       {
         gtk_progress_bar_set_text(GTK_PROGRESS_BAR(sgpp->progress_bar)
	                          ,_("Canceled"));
       }
       else
       {
         gtk_progress_bar_set_text(GTK_PROGRESS_BAR(sgpp->progress_bar), " ");
       }
    }

    if(vindex_file)
    {
      g_free(vindex_file);
    }
    
    if(create_vindex_msg)
    {
      g_free(create_vindex_msg);
    }

  }
#endif
  
}  /* end p_open_videofile */



/* -----------------------------
 * p_fetch_videoframe
 * -----------------------------
 */
static guchar *
p_fetch_videoframe(GapStbMainGlobalParams *sgpp
                   , const char *gva_videofile
		   , gint32 framenumber
		   , gint32 seltrack
		   , const char *preferred_decoder
		   , gdouble delace
		   , gint32 *th_bpp
		   , gint32 *th_width
		   , gint32 *th_height
		   )
{
  guchar *th_data;
  gint32 vthumb_size;
  
  th_data = NULL;
#ifdef GAP_ENABLE_VIDEOAPI_SUPPORT

  if(sgpp->gva_lock)
  {
    return(NULL);
  }
  sgpp->gva_lock = TRUE;

  if((sgpp->gva_videofile) && (sgpp->gvahand))
  {
    if((strcmp(sgpp->gva_videofile, gva_videofile) != 0)
    ||(seltrack != (sgpp->gvahand->vid_track+1)))
    {
//printf(" VIDFETCH (--) CALL p_close_videofile\n");
//printf(" VIDFETCH (--) sgpp->gva_videofile: %s\n", sgpp->gva_videofile);
//printf(" VIDFETCH (--) sgpp->gvahand->vid_track: %d\n", (int)sgpp->gvahand->vid_track);
//printf(" VIDFETCH (--) gva_videofile: %s\n", gva_videofile);
//printf(" VIDFETCH (--) seltrack: %d\n", (int)seltrack);
      p_close_videofile(sgpp);
    }
  }
  
//printf(" VIDFETCH (-0) sgpp->gvahand: %d  framenumber:%06d\n", (int)sgpp->gvahand, (int)framenumber);
  if(sgpp->gvahand == NULL)
  {
    p_open_videofile(sgpp, gva_videofile, seltrack, preferred_decoder);
  }
  
  if(sgpp->gvahand)
  {
     gint32 l_deinterlace;
     gdouble l_threshold;
     gboolean do_scale;
     
     if(sgpp->progress_bar)
     {
       t_GVA_DecoderElem *dec_elem;
       char *videoseek_msg;
       
       videoseek_msg = NULL;
       dec_elem = (t_GVA_DecoderElem *)sgpp->gvahand->dec_elem;
       if(dec_elem->decoder_name)
       {
         videoseek_msg = g_strdup_printf(_("Videoseek (decoder: %s)"), dec_elem->decoder_name);
       }
       else
       {
         videoseek_msg = g_strdup_printf(_("Videoseek"));
       }
       
       gtk_progress_bar_set_text(GTK_PROGRESS_BAR(sgpp->progress_bar), videoseek_msg);
       g_free(videoseek_msg);
     }
     
     do_scale = TRUE;
     vthumb_size = 256;
     if(sgpp->gvahand->width > sgpp->gvahand->height)
     {
       *th_width = vthumb_size;
       *th_height = (sgpp->gvahand->height * vthumb_size) / sgpp->gvahand->width;
     }
     else
     {
       *th_width = vthumb_size;
       *th_width = (sgpp->gvahand->width * vthumb_size) / sgpp->gvahand->height;
     }
     *th_bpp = sgpp->gvahand->frame_bpp;
     
     /* split delace value: integer part is deinterlace mode, rest is threshold */                  
     l_deinterlace = delace;
     l_threshold = delace - (gdouble)l_deinterlace;

     sgpp->cancel_video_api = FALSE;

//printf(" VIDFETCH (-6) current_seek_nr:%d current_frame_nr:%d\n", (int)sgpp->gvahand->current_seek_nr  ,(int)sgpp->gvahand->current_frame_nr );
     /* fetch the wanted framenr  */
     th_data = GVA_fetch_frame_to_buffer(sgpp->gvahand
                , do_scale
                , framenumber
                , l_deinterlace
                , l_threshold
		, th_bpp
		, th_width
		, th_height
                );
//printf(" VIDFETCH (-7) current_seek_nr:%d current_frame_nr:%d\n", (int)sgpp->gvahand->current_seek_nr  ,(int)sgpp->gvahand->current_frame_nr );
     if(sgpp->progress_bar)
     {
       gtk_progress_bar_set_text(GTK_PROGRESS_BAR(sgpp->progress_bar), " ");
       gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(sgpp->progress_bar), 0.0);
     }
  }
  
  if(th_data)
  {
    /* throw away frame data because the fetch was cancelled
     * (an empty display is better than trash or the wrong frame)
     */
    if(sgpp->cancel_video_api)
    {
      g_free(th_data);
      th_data = NULL;
    }
  }

  sgpp->gva_lock = FALSE;

#endif
  return (th_data);
}  /* end p_fetch_videoframe */ 


/* --------------------------------
 * gap_story_dlg_get_velem
 * --------------------------------
 * search the Videofile List
 * for a matching Videofile element
 * IF FOUND: return pointer to that element
 * ELSE:     try to create a Videofile Element and return
 *           pointer to the newly created Element
 *           or NULL if no Element could be created.
 *
 * DO NOT g_free the returned Element !
 * it is just a reference to the original List
 * and should be kept until the program (The Storyboard Plugin) exits.
 */
GapStoryVideoElem *
gap_story_dlg_get_velem(GapStbMainGlobalParams *sgpp
              ,const char *video_filename
	      ,gint32 seltrack
	      ,const char *preferred_decoder
	      )
{
  GapStoryVideoElem *velem;
  
  if(sgpp == NULL) { return (NULL); }

  /* check for known video_filenames */
  for(velem = sgpp->video_list; velem != NULL; velem = (GapStoryVideoElem *)velem->next)
  {
    if((strcmp(velem->video_filename, video_filename) == 0)
    && (seltrack == velem->seltrack))
    {
      return(velem);
    }
  }
  
#ifdef GAP_ENABLE_VIDEOAPI_SUPPORT
  if(!sgpp->auto_vthumb)
  {
    return(NULL);
  }

  if(sgpp->auto_vthumb_refresh_canceled)
  {
    return(NULL);
  }


  p_open_videofile(sgpp, video_filename, seltrack, preferred_decoder);
  if(sgpp->gvahand)
  {
    /* video_filename is new, add a new element */
    velem = g_new(GapStoryVideoElem, 1);
    if(velem == NULL) { return (NULL); }

    velem->video_id = global_stb_video_id++;
    velem->seltrack = seltrack;
    velem->video_filename = g_strdup(video_filename);
    velem->total_frames = sgpp->gvahand->total_frames;
    if(!sgpp->gvahand->all_frames_counted)
    {
	 /* frames are not counted yet,
	  * and the total_frames information is just a guess
	  * in this case we assume 10 times more frames
	  */
	 velem->total_frames *= 10;
    }

    velem->next = sgpp->video_list;
    sgpp->video_list = velem;
  }

#endif
  return(velem);
  
}  /* end gap_story_dlg_get_velem */


/* ------------------------------
 * gap_story_dlg_add_vthumb
 * ------------------------------
 * create a new video thumbnail element and
 * add it to the vthumb list (at begin)
 */
GapVThumbElem *
gap_story_dlg_add_vthumb(GapStbMainGlobalParams *sgpp
			,gint32 framenr
	                ,guchar *th_data
			,gint32 th_width
			,gint32 th_height
			,gint32 th_bpp
			,gint32 video_id
			)
{
  GapVThumbElem     *vthumb;

  if(sgpp == NULL) { return(NULL); }

  vthumb = g_new(GapVThumbElem ,1);
  if(vthumb)
  {
    vthumb->th_data = th_data;
    vthumb->th_width = th_width;
    vthumb->th_height = th_height;
    vthumb->th_bpp = th_bpp;
    vthumb->framenr = framenr;
    vthumb->video_id = video_id;
    vthumb->next = sgpp->vthumb_list;
    sgpp->vthumb_list = vthumb;
  }
  return(vthumb);
}  /* end gap_story_dlg_add_vthumb */
			


/* ------------------------------
 * p_fetch_vthumb_elem
 * ------------------------------
 * search the Video Thumbnail List
 * for a matching Thumbnail element
 * IF FOUND: return pointer to that element
 * ELSE:     try to create a Thumbnail Element and return
 *           pointer to the newly created Element
 *           or NULL if no Element could be created.
 *
 * DO NOT g_free the returned Element !
 * it is just a reference to the original List
 * and should be kept until the program (The Storyboard Plugin) exits.
 */
GapVThumbElem *
p_fetch_vthumb_elem(GapStbMainGlobalParams *sgpp
              ,const char *video_filename
	      ,gint32 framenr
	      ,gint32 seltrack
	      ,const char *preferred_decoder
	      )
{
  GapStoryVideoElem *velem;
  GapVThumbElem     *vthumb;
  guchar *th_data;
  gint32 th_width;
  gint32 th_height;
  gint32 th_bpp;
  
  if(sgpp == NULL) { return (NULL); }
  
  velem = gap_story_dlg_get_velem(sgpp
              ,video_filename
	      ,seltrack
	      ,preferred_decoder
	      );
	      
  if(velem == NULL) { return (NULL); }
 
  /* check for known viedo_thumbnails */
  for(vthumb = sgpp->vthumb_list; vthumb != NULL; vthumb = (GapVThumbElem *)vthumb->next)
  {
    if((velem->video_id == vthumb->video_id)
    && (framenr == vthumb->framenr))
    {
      /* we found the wanted video thumbnail */
      return(vthumb);
    }
  }

  if(!sgpp->auto_vthumb)
  {
    return(NULL);
  }

  if(sgpp->auto_vthumb_refresh_canceled)
  {
    return(NULL);
  }
  /* Videothumbnail not known yet,
   * we try to create it now
   */
  
  /* Fetch the wanted Frame from the Videofile */
  th_data = p_fetch_videoframe(sgpp
                   , video_filename
		   , framenr
		   , seltrack
		   , preferred_decoder
		   , 1.5               /* delace */
		   , &th_bpp
		   , &th_width
		   , &th_height
		   );
  
  if(th_data == NULL ) { return (NULL); }

  vthumb = gap_story_dlg_add_vthumb(sgpp
                          ,framenr
			  ,th_data
			  ,th_width
			  ,th_height
			  ,th_bpp
			  ,velem->video_id
			  );
  
  
  
  vthumb = g_new(GapVThumbElem ,1);
  if(vthumb == NULL ) { return (NULL); }
  
  vthumb->th_data = th_data;
  vthumb->th_width = th_width;
  vthumb->th_height = th_height;
  vthumb->th_bpp = th_bpp;
  vthumb->framenr = framenr;
  vthumb->video_id = velem->video_id;
  vthumb->next = sgpp->vthumb_list;
  sgpp->vthumb_list = vthumb;
  
  return(vthumb);
  
}  /* end p_fetch_vthumb_elem */




/* ------------------------------
 * gap_story_dlg_fetch_vthumb
 * ------------------------------
 * RETURN a copy of the videothumbnail data
 *        or NULL if fetch was not successful
 *        the caller is responsible to g_free the returned data
 *        after usage.
 */
guchar *
gap_story_dlg_fetch_vthumb(GapStbMainGlobalParams *sgpp
              ,const char *video_filename
	      ,gint32 framenr
	      ,gint32 seltrack
	      ,const char *preferred_decoder
	      , gint32 *th_bpp
	      , gint32 *th_width
	      , gint32 *th_height
	      )
{
  GapVThumbElem *vthumb;
  guchar *th_data;


  th_data = NULL;
  vthumb = p_fetch_vthumb_elem(sgpp
              ,video_filename
	      ,framenr
	      ,seltrack
	      ,preferred_decoder
	      );
  if(vthumb)
  {
    gint32 th_size;
    
    th_size = vthumb->th_width * vthumb->th_height * vthumb->th_bpp;
    th_data = g_malloc(th_size);
    if(th_data)
    {
      memcpy(th_data, vthumb->th_data, th_size);
      *th_width = vthumb->th_width;
      *th_height = vthumb->th_height;
      *th_bpp = vthumb->th_bpp;
    }
  }
  return(th_data);	     
}  /* end gap_story_dlg_fetch_vthumb */



/* -----------------------------------
 * gap_story_dlg_fetch_vthumb_no_store
 * -----------------------------------
 * RETURN a pointer of the videothumbnail data
 *        or thumbnail data read from file (indicated by file_read_flag = TRUE)
 * the caller must g_free the returned data if file_read_flag = TRUE
 * but MUST NOT g_free the returned data if file_read_flag = FALSE
 *
 * the video_id (identifier of the videofile in the video files list)
 * is passed in the output parameter video_id
 */
guchar *
gap_story_dlg_fetch_vthumb_no_store(GapStbMainGlobalParams *sgpp
              ,const char *video_filename
	      ,gint32 framenr
	      ,gint32 seltrack
	      ,const char *preferred_decoder
	      , gint32   *th_bpp
	      , gint32   *th_width
	      , gint32   *th_height
	      , gboolean *file_read_flag
	      , gint32   *video_id
	      )
{
  GapStoryVideoElem *velem;
  GapVThumbElem     *vthumb;
  guchar *th_data;
  
  if(sgpp == NULL) { return (NULL); }

  *video_id = -1;
  *file_read_flag = FALSE;
  velem = gap_story_dlg_get_velem(sgpp
              ,video_filename
	      ,seltrack
	      ,preferred_decoder
	      );
	      
  if(velem == NULL) { return (NULL); }
 
  *video_id = velem->video_id;
 
  /* check for known viedo_thumbnails */
  for(vthumb = sgpp->vthumb_list; vthumb != NULL; vthumb = (GapVThumbElem *)vthumb->next)
  {
    if((velem->video_id == vthumb->video_id)
    && (framenr == vthumb->framenr))
    {
      /* we found the wanted video thumbnail */
      *th_width = vthumb->th_width;
      *th_height = vthumb->th_height;
      *th_bpp = vthumb->th_bpp;
      return(vthumb->th_data);
    }
  }

  if(!sgpp->auto_vthumb)
  {
    return(NULL);
  }

  /* Videothumbnail not known yet,
   * we try to create it now
   */
  
  /* Fetch the wanted Frame from the Videofile */
  th_data = p_fetch_videoframe(sgpp
                   , video_filename
		   , framenr
		   , seltrack
		   , preferred_decoder
		   , 1.5               /* delace */
		   , th_bpp
		   , th_width
		   , th_height
		   );

  *file_read_flag = TRUE;
  return(th_data); 

}  /* end gap_story_dlg_fetch_vthumb_no_store */




/* ---------------------------------
 * story_dlg_response
 * ---------------------------------
 */
static void
story_dlg_response (GtkWidget *widget,
                 gint       response_id,
                 GapStbMainGlobalParams *sgpp)
{
  GtkWidget *dialog;

  switch (response_id)
  {
    case GTK_RESPONSE_OK:
      if(sgpp)
      {
	if (GTK_WIDGET_VISIBLE (sgpp->shell_window))
	  gtk_widget_hide (sgpp->shell_window);

	sgpp->run = TRUE;
      }

    default:
      dialog = NULL;
      if(sgpp)
      {
        p_close_videofile(sgpp);
        dialog = sgpp->shell_window;
	if(dialog)
	{
          sgpp->shell_window = NULL;
          gtk_widget_destroy (dialog);
	}
      }
      gtk_main_quit ();
      break;
  }
}  /* end story_dlg_response */




/* -----------------------------
 * p_recreate_tab_widgets
 * -----------------------------
 * destroy old fw_table and all the attached frame_widgets if they exist.
 * then newly create that stuff with empty frame_widgets.
 * the caller should invoke the p_frame_widgets_render procedure afterwards.
 */
static void
p_recreate_tab_widgets(GapStoryBoard *stb
                       ,GapStbTabWidgets *tabw
		       ,gint32 mount_col
		       ,gint32 mount_row
		       ,GapStbMainGlobalParams *sgpp
		      )
{
  /*if(gap_debug)*/ printf("p_recreate_tab_widgets:START stb:%d tabw:%d\n", (int)stb ,(int)tabw);

  if(tabw)
  {
    GtkWidget *table;
    gint col;
    gint row;
    gint ii;
    gint32 l_max_ii = 0;
    gint32 l_act_elems = 0;

//printf("p_recreate_tab_widgets:(2)\n");

    if(tabw->mount_table == NULL)
    {
      printf("Internal error: tabw->mount_table is NULL\n");
      return;
    }
    
    tabw->sgpp = sgpp;
    
    if(tabw->fw_tab)
    {
      /* destroy the old table of frame widgets */      
      for(ii=0; ii < tabw->fw_tab_size; ii++)
      {
        p_frame_widget_destroy(tabw->fw_tab[ii]);
	g_free(tabw->fw_tab[ii]);
      }
      g_free(tabw->fw_tab);
      tabw->fw_tab = NULL;
    }
    
    if(tabw->fw_gtk_table)
    {
      /* destroy the old table widget */
      gtk_widget_destroy(tabw->fw_gtk_table);
      tabw->fw_gtk_table = NULL;
    }

    tabw->mount_col = mount_col;
    tabw->mount_row = mount_row;

    if(stb)
    {
      gint32 l_width = 1;
      gint32 l_height = 1;
      
      if(stb->layout_cols > 0)       { tabw->cols = stb->layout_cols; }
      if(stb->layout_rows > 0)       { tabw->rows = stb->layout_rows; }
      if(stb->layout_thumbsize > 0)  { tabw->thumbsize = stb->layout_thumbsize; }
      
      gap_story_get_master_size(stb
                               ,&l_width
			       ,&l_height
			       );
      if(l_width > l_height)
      {
        tabw->thumb_width = tabw->thumbsize;
        tabw->thumb_height = tabw->thumbsize * l_height / l_width;
      }
      else
      {
        tabw->thumb_width = tabw->thumbsize * l_width / l_height;
        tabw->thumb_height = tabw->thumbsize;
      }
 
      l_act_elems = gap_story_count_active_elements(stb, 1 /* track */ );   
    }

    /*if(gap_debug)*/ printf("p_recreate_tab_widgets: rows:%d cols:%d\n", (int)tabw->rows ,(int)tabw->cols );
    
    table = gtk_table_new (tabw->rows, tabw->cols, TRUE);
    gtk_widget_show (table);
    tabw->fw_gtk_table = table;
    
    gtk_table_attach (GTK_TABLE (tabw->mount_table)
                      , tabw->fw_gtk_table
                      , tabw->mount_col, tabw->mount_col+1
                      , tabw->mount_row, tabw->mount_row+1
                      , (GtkAttachOptions) (GTK_FILL | GTK_EXPAND)
                      , (GtkAttachOptions) (GTK_FILL | GTK_EXPAND)
                      , 0, 0);
 

    /* allocate as much frame widgets as can be displayed
     * in the table
     */
    tabw->fw_tab_size = MIN((tabw->rows * tabw->cols), l_act_elems);
    tabw->fw_tab = g_malloc0(sizeof(GapStbFrameWidget *) * tabw->fw_tab_size);

    /* init buffers for the frame widgets */
    for(ii=0; ii < tabw->fw_tab_size; ii++)
    {
      tabw->fw_tab[ii] = g_new(GapStbFrameWidget, 1);
      p_frame_widget_init_empty(tabw, tabw->fw_tab[ii]);
    }

    ii = 0;
    l_max_ii = l_act_elems - ((tabw->rowpage -1) * MAX(tabw->cols,1));
    for(row=0; row < tabw->rows; row++)
    {
      for(col=0; col < tabw->cols; col++)
      {
	 if(ii < l_max_ii)
	 {
	   GapStbFrameWidget *fw;
	  
	   fw = tabw->fw_tab[ii];
           gtk_table_attach (GTK_TABLE (tabw->fw_gtk_table)
                	, fw->vbox
                	, col, col+1
                	, row, row+1
                	, (GtkAttachOptions) (GTK_FILL | GTK_EXPAND)
                	, (GtkAttachOptions) (GTK_FILL | GTK_EXPAND)
                	, 0, 0);
	 }
	 else
	 {
           char *l_txt;
           GtkWidget *label;

	   /* dummy label */
	   l_txt = g_strdup_printf(" ");

	   label = gtk_label_new (l_txt);
	   gtk_widget_show (label);


           gtk_table_attach (GTK_TABLE (tabw->fw_gtk_table)
                	, label                          // tabw->fw_tab[ii].vbox
                	, col, col+1
                	, row, row+1
                	, (GtkAttachOptions) (GTK_FILL | GTK_EXPAND)
                	, (GtkAttachOptions) (GTK_FILL | GTK_EXPAND)
                	, 0, 0);

	   g_free(l_txt);
         }
	 ii++;
      }
    }

    /* display frame label (if valid storyboard available add the filename)
     * and set upper limit for the tabw->rowpage_spinbutton_adj widget
     */
    p_tabw_update_frame_label(tabw, sgpp);

  }
}  /* end p_recreate_tab_widgets */


/* ----------------------------------
 * gap_story_dlg_recreate_tab_widgets
 * ----------------------------------
 */

void  gap_story_dlg_recreate_tab_widgets(GapStbTabWidgets *tabw
				  ,GapStbMainGlobalParams *sgpp
				  )
{
  GapStoryBoard *stb_dst;

  if(tabw)
  {
    stb_dst = p_tabw_get_stb_ptr(tabw);

    /* clear selections when scene detection adds new clips */
    gap_story_selection_all_set(stb_dst, FALSE);

    /* refresh storyboard layout and thumbnail list widgets */
    p_recreate_tab_widgets( stb_dst
                           ,tabw
			   ,tabw->mount_col
			   ,tabw->mount_row
			   ,sgpp
			   );
    p_render_all_frame_widgets(tabw);
  }
}  /* end gap_story_dlg_recreate_tab_widgets */

/* -----------------------------
 * p_create_button_bar
 * -----------------------------
 * create button bar (cut,copy,paste,play and rowpage spinbutton)
 * and rowpage vscale widgets
 */
static void
p_create_button_bar(GapStbTabWidgets *tabw
	           ,gint32 mount_col
		   ,gint32 mount_row
                   ,GapStbMainGlobalParams *sgpp
		   ,gboolean with_open_and_save
	           ,gint32 mount_vs_col
		   ,gint32 mount_vs_row
		   )
{
  /* rowpage spinbutton and labels */
  GtkWidget *hbox;
  GtkWidget *hbox2;
  GtkWidget *entry;
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *vscale;
  GtkWidget *spinbutton;
  GtkObject *spinbutton_adj;
  GtkObject *adj;

  /* the hbox */
  hbox = gtk_hbox_new (FALSE, 2);
  gtk_widget_show (hbox);


  gtk_table_attach (GTK_TABLE (tabw->mount_table)
                  , hbox
                  , mount_col, mount_col+1
                  , mount_row, mount_row+1
                  , (GtkAttachOptions) (GTK_FILL | GTK_EXPAND)
                  , (GtkAttachOptions) (0)
                  , 0, 0);


  /* the hbox2 */
  hbox2 = gtk_hbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (hbox2), 2);
  gtk_box_pack_start (GTK_BOX (hbox), hbox2, FALSE, FALSE, 0);
  gtk_widget_show (hbox2);


  if(with_open_and_save)
  {
    /*  The Load button  */
    button = gtk_button_new_from_stock (GTK_STOCK_OPEN );
    gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);
    g_signal_connect (G_OBJECT (button), "clicked",
		      G_CALLBACK (p_tabw_load_file_cb),
		      tabw);
    if(tabw->type == GAP_STB_MASTER_TYPE_STORYBOARD)
    {
      gimp_help_set_help_data (button,
			     _("Load storyboard file"),
			     NULL);
    }
    else
    {
      gimp_help_set_help_data (button,
			     _("Load cliplist file"),
			     NULL);
    }
    gtk_widget_show (button);

    /*  The Save button  */
    button = gtk_button_new_from_stock (GTK_STOCK_SAVE );
    tabw->save_button = button;
    gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);
    if(tabw->type == GAP_STB_MASTER_TYPE_STORYBOARD)
    {
      g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (p_menu_stb_save_cb),
		    tabw->sgpp);
      gimp_help_set_help_data (button,
			     _("Save storyboard to file"),
			     NULL);
    }
    else
    {
      g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (p_menu_cll_save_cb),
		    tabw->sgpp);
      gimp_help_set_help_data (button,
			     _("Save cliplist to file"),
			     NULL);
    }
    gtk_widget_show (button);
  }



  /*  The Cut button  */
  button = gtk_button_new_from_stock (GTK_STOCK_CUT );
  tabw->edit_cut_button = button;
  gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (button), "clicked",
		  G_CALLBACK (p_tabw_edit_cut_cb),
		  tabw);
  gimp_help_set_help_data (button,
			   _("Cut a clip"),
			   NULL);
  gtk_widget_show (button);

  /*  The Copy button  */
  button = gtk_button_new_from_stock (GTK_STOCK_COPY );
  tabw->edit_copy_button = button;
  gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (button), "clicked",
		  G_CALLBACK (p_tabw_edit_copy_cb),
		  tabw);
  gimp_help_set_help_data (button,
			   _("Copy a clip"),
			   NULL);
  gtk_widget_show (button);

  /*  The Paste button  */
  button = gtk_button_new_from_stock (GTK_STOCK_PASTE );
  tabw->edit_paste_button = button;
  gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (button), "clicked",
		  G_CALLBACK (p_tabw_edit_paste_cb),
		  tabw);
  gimp_help_set_help_data (button,
			   _("Paste a clip after last (selected) element"),
			   NULL);
  gtk_widget_show (button);


  /*  The Play Button */
  button = gtk_button_new_from_stock (GAP_STOCK_PLAY);
  tabw->play_button = button;
  gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);
  if(tabw->type == GAP_STB_MASTER_TYPE_STORYBOARD)
  {
    g_signal_connect (G_OBJECT (button), "button_press_event",
		    G_CALLBACK (p_player_stb_mode_cb),
		    sgpp);
    gimp_help_set_help_data (button,
			   _("Playback storyboard file. SHIFT: Play only selected clips"),
			   NULL);
  }
  else
  {
    g_signal_connect (G_OBJECT (button), "button_press_event",
		    G_CALLBACK (p_player_cll_mode_cb),
		    sgpp);
    gimp_help_set_help_data (button,
			   _("Playback cliplist. SHIFT: Play only selected clips"),
			   NULL);
  }
  gtk_widget_show (button);

  /*  The filename entry */
  entry = gtk_entry_new();
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_entry_set_text(GTK_ENTRY(entry), tabw->filename_refptr);
  g_signal_connect(G_OBJECT(entry), "changed",
		 G_CALLBACK (p_tabw_filename_entry_cb),
		 tabw);
  tabw->filename_entry = entry;
  /*  gtk_widget_show (entry); */



  /* Row label */
  label = gtk_label_new (_("Row:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);
  gtk_widget_show (label);

  /* rowpage spinbutton */
  spinbutton_adj = gtk_adjustment_new ( tabw->rowpage
                                       , 1   /* min */
                                       , 1   /* max */
                                       , 1, 10, 10);
  tabw->rowpage_spinbutton_adj = spinbutton_adj;					   
  spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (spinbutton_adj), 1, 0);
  gtk_widget_show (spinbutton);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 2);

  gtk_widget_set_size_request (spinbutton, 80, -1);
  gimp_help_set_help_data (spinbutton, _("Top rownumber"), NULL);
  g_signal_connect (G_OBJECT (spinbutton), "value_changed",
                  G_CALLBACK (p_rowpage_spinbutton_cb),
                  tabw);

  /* of label */
  label = gtk_label_new (_("of:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);
  gtk_widget_show (label);

  /* total rows label */
  label = gtk_label_new ("1");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);
  gtk_widget_show (label);
  tabw->total_rows_label = label;


  /* rowpage vscale */
  adj = gtk_adjustment_new ( tabw->rowpage
                                       , 1   /* min */
                                       , 1   /* max */
                                       , 1, 10, 10);
  vscale = gtk_vscrollbar_new (GTK_ADJUSTMENT(adj));

  gtk_range_set_update_policy (GTK_RANGE (vscale), GTK_UPDATE_DELAYED); /* GTK_UPDATE_CONTINUOUS */

  gtk_table_attach (GTK_TABLE (tabw->mount_table)
                  , vscale
                  , mount_vs_col, mount_vs_col+1
                  , mount_vs_row, mount_vs_row+1
                  , (GtkAttachOptions) (0)
                  , (GtkAttachOptions) (GTK_FILL)
                  , 0, 0);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
                      (GtkSignalFunc)p_vscale_changed_cb,
                      tabw);
  /* startup with invisble vscale */
  gtk_widget_hide (vscale);
    
  tabw->rowpage_vscale_adj = adj;					   
  tabw->rowpage_vscale = vscale;					   
 
      
}  /* end p_create_button_bar */


/* -----------------------------
 * gap_storyboard_dialog
 * -----------------------------
 */
void
gap_storyboard_dialog(GapStbMainGlobalParams *sgpp)
{
  GtkWidget *dialog;
  GtkWidget  *vbox;
  GtkWidget  *hbox;
  GtkWidget  *stb_frame;
  GtkWidget  *stb_vbox;

  GtkWidget  *clp_frame;
  GtkWidget  *clp_vbox;
  GtkWidget  *player_frame;
  GtkWidget  *table;
  GtkWidget  *menu_bar;

  /* Init UI  */
  gimp_ui_init ("storyboard", FALSE);
  gap_stock_init();

  /* workaround:
   *  the current implementation of the STORYBOARD dialog
   *  crashes if the invoker image is not a numbered anim frame. (dont know why)
   *  The following workaround checks for anim frame to avoid the crash
   *  but both crash and this check should be removed in the future.
   */
  {
    GapAnimInfo *ainfo_ptr;
    int chk_rc;

    ainfo_ptr = gap_lib_alloc_ainfo(sgpp->image_id, sgpp->run_mode);
    gap_lib_dir_ainfo(ainfo_ptr);

    if(ainfo_ptr != NULL)
    {
       chk_rc = gap_lib_chk_framerange(ainfo_ptr);
       gap_lib_free_ainfo(&ainfo_ptr);
       
       if(0 != chk_rc)
       {
         return;
       }
    }
  }

  /*  The Storyboard dialog  */
  sgpp->run = FALSE;
  sgpp->curr_selection = NULL;

  sgpp->video_list = NULL;
  sgpp->vthumb_list = NULL;
  sgpp->gvahand = NULL;
  sgpp->gva_videofile = NULL;
  sgpp->progress_bar = NULL;
  sgpp->gva_lock = FALSE;
  sgpp->auto_vthumb = FALSE;
  sgpp->auto_vthumb = FALSE;
  sgpp->auto_vthumb_refresh_canceled = FALSE;

  sgpp->menu_item_cll_save = NULL;
  sgpp->menu_item_cll_save_as = NULL;
  sgpp->menu_item_cll_add_clip = NULL;
  sgpp->menu_item_cll_playback = NULL;
  sgpp->menu_item_cll_properties = NULL;
  sgpp->menu_item_cll_close = NULL;

  sgpp->menu_item_stb_save = NULL;
  sgpp->menu_item_stb_save_as = NULL;
  sgpp->menu_item_stb_add_clip = NULL;
  sgpp->menu_item_stb_playback = NULL;
  sgpp->menu_item_stb_properties = NULL;
  sgpp->menu_item_stb_close = NULL;

  /*  The dialog and main vbox  */
  /* the help_id is passed as NULL to avoid creation of the HELP button
   * (the Help Button would be the only button in the action area and results
   *  in creating an extra row
   *  additional note: the Storyboard dialog provides
   *  Help via Menu-Item
   */
  dialog = gimp_dialog_new (_("Storyboard"), "storyboard",
                               NULL, 0,
			       gimp_standard_help_func, NULL, /* GAP_STORYBOARD_EDIT_HELP_ID */
                               NULL);

  sgpp->shell_window = dialog;
  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (story_dlg_response),
                    sgpp);

  /* the vbox */
  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), vbox,
		      TRUE, TRUE, 0);
  gtk_widget_show (vbox);


  /* the menu_bar */
  menu_bar = gtk_menu_bar_new();
  gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, TRUE, 0);

  p_make_menu_global(sgpp, menu_bar);
  p_make_menu_cliplist(sgpp, menu_bar);
  p_make_menu_storyboard(sgpp, menu_bar);
  p_make_menu_help(sgpp, menu_bar);

  gtk_widget_show(menu_bar);
 
  /* the hbox */
  hbox = gtk_hbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);


  /* XXXXXXXXXXX Start of the CLIPLIST widgets  XXXXXXXXXXXX */
  
  /* the clp_frame */
  clp_frame = gimp_frame_new ( _("Cliplist") );
  gtk_frame_set_shadow_type (GTK_FRAME (clp_frame) ,GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (hbox), clp_frame, TRUE, TRUE, 0);
  gtk_widget_show (clp_frame);

  /* the clp_vbox */
  clp_vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (clp_vbox), 2);
  gtk_container_add (GTK_CONTAINER (clp_frame), clp_vbox);
  gtk_widget_show (clp_vbox);
 
  /* mount_table for the cliplist table */
  table = gtk_table_new (2, 2, FALSE);
  gtk_widget_show (table);
  gtk_box_pack_start (GTK_BOX (clp_vbox), table, TRUE, TRUE, 0);

  /* init Table widgets (tabw struct for cliplist) */
  sgpp->cll_widgets = p_new_stb_tab_widgets(sgpp, GAP_STB_MASTER_TYPE_CLIPLIST);
  sgpp->cll_widgets->mount_table = table;
  sgpp->cll_widgets->frame_with_name = clp_frame;

  /* create CLIPLIST Table widgets */
  p_recreate_tab_widgets( sgpp->cll
                         ,sgpp->cll_widgets
			 ,0    /* mount_col */
			 ,0    /* mount_row */
			 ,sgpp
			 );

  /* button bar */
  p_create_button_bar(sgpp->cll_widgets
			 ,0    /* mount_col */
			 ,1    /* mount_row */
			 ,sgpp
			 ,FALSE  /*  with_open_and_save == FALSE */
			 ,1    /* mount_vs_col */
			 ,0    /* mount_vs_row */
			 );


  /* XXXXXXXXXXX Player Frame  XXXXXXXXXXXX */
  /* the player_frame */
  player_frame = gimp_frame_new ( _("Playback") );
  gtk_frame_set_shadow_type (GTK_FRAME (player_frame) ,GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (hbox), player_frame, TRUE, TRUE, 0);
  gtk_widget_show (player_frame);
  sgpp->player_frame = player_frame;

  /* XXXXXXXXXXX Start of the STORYBOARD widgets  XXXXXXXXXXXX */



  /* the stb_frame */
  stb_frame = gimp_frame_new ( _("Storyboard") );
  gtk_frame_set_shadow_type (GTK_FRAME (stb_frame) ,GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (vbox), stb_frame, TRUE, TRUE, 0);
  gtk_widget_show (stb_frame);

  /* the stb_vbox */
  stb_vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (stb_vbox), 2);
  gtk_container_add (GTK_CONTAINER (stb_frame), stb_vbox);
  gtk_widget_show (stb_vbox);


  /* mount_table for the storyboard table */
  table = gtk_table_new (2, 2, FALSE);
  gtk_widget_show (table);
  gtk_box_pack_start (GTK_BOX (stb_vbox), table, TRUE, TRUE, 0);

  /* init Table widgets (tabw struct for storyboard) */
  sgpp->stb_widgets = p_new_stb_tab_widgets(sgpp, GAP_STB_MASTER_TYPE_STORYBOARD);
  sgpp->stb_widgets->mount_table = table;
  sgpp->stb_widgets->frame_with_name = stb_frame;



  /* create STORYBOARD Table widgets */
  p_recreate_tab_widgets( sgpp->stb
                         ,sgpp->stb_widgets
			 ,0    /* mount_col */
			 ,0    /* mount_row */
			 ,sgpp
			 );
  /* button bar */
  p_create_button_bar(sgpp->stb_widgets
			 ,0    /* mount_col */
			 ,1    /* mount_row */
			 ,sgpp
			 ,TRUE  /*  with_open_and_save == FALSE */
			 ,1    /* mount_vs_col */
			 ,0    /* mount_vs_row */
			 );

  /* the hbox */
  hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 0);
  gtk_widget_show (hbox);



  /*  The Video ProgressBar */

  {
    GtkWidget *button;
    GtkWidget *progress_bar;

    button = gtk_button_new_with_label (_("Cancel"));  
    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
    gtk_widget_show (button);
    g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (p_cancel_button_cb),
		    sgpp);
     
     
    progress_bar = gtk_progress_bar_new ();
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress_bar), " ");
    gtk_widget_show (progress_bar);

    gtk_box_pack_start (GTK_BOX (hbox), progress_bar, TRUE, TRUE, 0);
  
    sgpp->progress_bar = progress_bar;
  }


//  /*  The Play Frames Button (DEBUG widget) */
//  { GtkWidget *button;
//  button = gtk_button_new_with_label (_("Play Frames"));
//  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, FALSE, 0);
//  g_signal_connect (G_OBJECT (button), "clicked",
//		    G_CALLBACK (p_player_img_mode_cb),
//		    sgpp);
//  gtk_widget_show (button);
//  gimp_help_set_help_data (button,
//			   _("Playback normal mode (frames from invoking image)"),
//			   NULL);
//  }
  sgpp->initialized = TRUE;
  p_widget_sensibility(sgpp);
  gtk_widget_show (dialog);
  
  /* init player window */
  p_player_img_mode_cb(NULL, sgpp);
  
  gtk_main ();
  gdk_flush ();

}  /* end gap_storyboard_dialog */




/* -----------------------------------
 * p_tabw_master_prop_dialog
 * ------------------------------------
 * // TODO:  replace this arr_dialog by scaling widget (see gap_resi_dialog.c)
 */
static void
p_tabw_master_prop_dialog(GapStbTabWidgets *tabw)
{
  static GapArrArg  argv[6];
  static char *radio_args[4]  = { N_("automatic"), "libmpeg3", "libavformat", "quicktime4linux" };
  gint   l_ii;
  gint   l_ii_width;
  gint   l_ii_height;
  gint   l_ii_rate;
  gint   l_ii_preferred_decoder;
  GapStbMainGlobalParams *sgpp;
  GapStoryBoard *stb_dst;
  gint32   l_master_width;
  gint32   l_master_height;
  gchar    buf_preferred_decoder[60];

  stb_dst = p_tabw_get_stb_ptr (tabw);
  if(stb_dst == NULL) { return; }
  if(tabw->master_dlg_open)  { return; }
  sgpp = (GapStbMainGlobalParams *)tabw->sgpp;

  tabw->master_dlg_open  = TRUE;
  
  gap_story_get_master_size(stb_dst, &l_master_width, &l_master_height);
			 
  l_ii = 0; l_ii_width = l_ii;
  gap_arr_arg_init(&argv[l_ii], GAP_ARR_WGT_FILESEL);
  argv[l_ii].label_txt = _("Name:");
  argv[l_ii].entry_width = 250;       /* pixel */
  if(tabw == sgpp->stb_widgets)
  {
    argv[l_ii].help_txt  = _("Name of the Storyboardfile");
    argv[l_ii].text_buf_len = sizeof(sgpp->storyboard_filename);
    argv[l_ii].text_buf_ret = &sgpp->storyboard_filename[0];
  }
  else
  {
    argv[l_ii].help_txt  = _("Name of the Cliplistfile");
    argv[l_ii].text_buf_len = sizeof(sgpp->cliplist_filename);
    argv[l_ii].text_buf_ret = &sgpp->cliplist_filename[0];
  }

  l_ii++; l_ii_width = l_ii;
  gap_arr_arg_init(&argv[l_ii], GAP_ARR_WGT_INT_PAIR);
  argv[l_ii].constraint = TRUE;
  argv[l_ii].label_txt = _("Width:");
  argv[l_ii].help_txt  = _("Width in pixels");
  argv[l_ii].int_min   = (gint)1;
  argv[l_ii].int_max   = (gint)9999;
  argv[l_ii].int_ret   = (gint)l_master_width;
  argv[l_ii].has_default = TRUE;
  argv[l_ii].int_default = (gint)l_master_width;

  l_ii++; l_ii_height = l_ii;
  gap_arr_arg_init(&argv[l_ii], GAP_ARR_WGT_INT_PAIR);
  argv[l_ii].constraint = TRUE;
  argv[l_ii].label_txt = _("Height:");
  argv[l_ii].help_txt  = _("Height in pixels)");
  argv[l_ii].int_min   = (gint)1;
  argv[l_ii].int_max   = (gint)9999;
  argv[l_ii].int_ret   = (gint)l_master_height;
  argv[l_ii].has_default = TRUE;
  argv[l_ii].int_default = (gint)l_master_height;


  l_ii++; l_ii_rate = l_ii;
  gap_arr_arg_init(&argv[l_ii], GAP_ARR_WGT_FLT_PAIR);
  argv[l_ii].constraint = TRUE;
  argv[l_ii].label_txt = _("Framerate:");
  argv[l_ii].help_txt  = _("Framerate in frames/sec.");
  argv[l_ii].flt_min   = -1.0;
  argv[l_ii].flt_max   = 999;
  argv[l_ii].flt_ret   = GAP_STORY_DEFAULT_FRAMERATE;
  if(stb_dst->master_framerate > 0)
  {
    argv[l_ii].flt_ret   = stb_dst->master_framerate;
  }
  argv[l_ii].has_default = TRUE;
  argv[l_ii].flt_default = argv[l_ii].flt_ret;

  l_ii++; l_ii_preferred_decoder = l_ii;
  buf_preferred_decoder[0] = '\0';
  if(stb_dst->preferred_decoder)
  {
    if(*stb_dst->preferred_decoder != '\0')
    {
      g_snprintf(buf_preferred_decoder
	       , sizeof(buf_preferred_decoder)
	       , "%s"
	       , stb_dst->preferred_decoder
	       );
    }
  }
  gap_arr_arg_init(&argv[l_ii], GAP_ARR_WGT_OPT_ENTRY);
  argv[l_ii].label_txt = _("Decoder:");
  argv[l_ii].help_txt  = _("Select preferred video decoder library, "
        	       "or leave empty for automatic selection."
		       "The decoder setting is only relevant if "
		       "videoclips are used (but not for frames "
		       "that are imagefiles)");
  argv[l_ii].radio_argc  = G_N_ELEMENTS(radio_args);
  argv[l_ii].radio_argv = radio_args;
  argv[l_ii].radio_ret  = 0;
  argv[l_ii].has_default = TRUE;
  argv[l_ii].int_default = 0;
  argv[l_ii].text_buf_ret = buf_preferred_decoder;
  argv[l_ii].text_buf_len = sizeof(buf_preferred_decoder);
  argv[l_ii].text_buf_default = g_strdup(buf_preferred_decoder);


  l_ii++;
  gap_arr_arg_init(&argv[l_ii], GAP_ARR_WGT_DEFAULT_BUTTON);
  argv[l_ii].label_txt =  _("Reset");                /* should use GIMP_STOCK_RESET if possible */
  argv[l_ii].help_txt  = _("Reset parameters to default size");

  if(TRUE == gap_arr_ok_cancel_dialog( _("Master Properties"),
                                 _("Settings"), 
                                  G_N_ELEMENTS(argv), argv))
  {
     stb_dst->master_width = (gint32)(argv[l_ii_width].int_ret);
     stb_dst->master_height = (gint32)(argv[l_ii_height].int_ret);
     stb_dst->master_framerate = (gint32)(argv[l_ii_rate].flt_ret);
     if(*buf_preferred_decoder)
     {
       if(stb_dst->preferred_decoder)
       {
         if(strcmp(stb_dst->preferred_decoder, buf_preferred_decoder) != 0)
	 {
           g_free(stb_dst->preferred_decoder);
	   stb_dst->preferred_decoder = g_strdup(buf_preferred_decoder);
	   
	   /* close the videohandle (if open)
	    * this ensures that the newly selected decoder
	    * can be used at the next videofile access attempt
	    */
           p_close_videofile(sgpp);
	 }
       }
       else
       {
	   stb_dst->preferred_decoder = g_strdup(buf_preferred_decoder);
           p_close_videofile(sgpp);
       }
     }
     else
     {
       if(stb_dst->preferred_decoder)
       {
         g_free(stb_dst->preferred_decoder);
       }
       stb_dst->preferred_decoder = NULL;
     }
    
     /* refresh storyboard layout and thumbnail list widgets */
     p_recreate_tab_widgets( stb_dst
                        	 ,tabw
				 ,tabw->mount_col
				 ,tabw->mount_row
				 ,sgpp
				 );
     p_render_all_frame_widgets(tabw);
     p_widget_sensibility(sgpp);

  }
  tabw->master_dlg_open  = FALSE;
  
}  /* end p_tabw_master_prop_dialog */

