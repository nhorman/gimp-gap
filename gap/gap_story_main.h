/*  gap_story_main.h
 *
 *  This module handles GAP storyboard level1 editing
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
 * version 1.3.25a; 2004/01/23  hof: created
 */

#ifndef _GAP_STORY_MAIN_H
#define _GAP_STORY_MAIN_H

#include "libgimp/gimp.h"
#include "gap_lib.h"
#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include "gap_pview_da.h"
#include "gap_story_file.h"
#include "gap_player_main.h"

#define GAP_STORY_MAX_STORYFILENAME_LEN 2048
#define GAP_STORY_MAX_CLIP_WIDGETS 2000
#define GAP_STORY_DEFAULT_FRAMERATE 24.0

#ifdef GAP_ENABLE_VIDEOAPI_SUPPORT
#include "gap_vid_api.h"
#else
#ifndef GAP_STUBTYPE_GVA_HANDLE
typedef gpointer t_GVA_Handle;
#define GAP_STUBTYPE_GVA_HANDLE
#endif
#endif

typedef enum
  {
     GAP_STB_EDMO_SEQUENCE_NUMBER
    ,GAP_STB_EDMO_FRAME_NUMBER        
    ,GAP_STB_EDMO_TIMECODE        
  } GapStoryElemDisplayMode;

typedef struct GapStoryVideoElem {
  gchar *video_filename;
  gint32 seltrack;
  gint32 video_id;
  gint32 total_frames;
  
  void *next;
}  GapStoryVideoElem;


typedef struct GapVThumbElem {
  guchar *th_data;
  gint32 th_width;
  gint32 th_height;
  gint32 th_bpp;
  gint32 framenr;
  gint32 video_id;
  
  void  *next;
}  GapVThumbElem;


typedef struct GapStbPropWidget  /* nickname: pw */
{
  GapStoryElem  *stb_elem_bck;     /* backup for use at reset button pressed */
  GapStoryElem  *stb_elem_refptr;  /* never g_free this one ! */
  GapStoryBoard *stb_refptr;       /* never g_free this one ! */
  void  *sgpp;               /* never g_free this one ! */
  void  *tabw;               /* never g_free this one ! (pointer to parent GapStbTabWidgets) */

  gint32   go_job_framenr;
  gboolean go_render_all_request;
  gint32   go_timertag;
  gboolean scene_detection_busy;
  gboolean close_flag;
  
  gdouble     delace_threshold;
  gint32      delace_mode;

  GapPView   *pv_ptr;            /* for gap preview rendering based on drawing_area */
  GtkWidget  *pw_prop_dialog;
  GtkWidget  *pw_filesel;
  GtkWidget  *pw_filename_entry;
  GtkWidget  *master_table;
  GtkWidget  *cliptype_label;
  GtkWidget  *dur_frames_label;
  GtkWidget  *dur_time_label;
  GtkWidget  *pingpong_toggle;
  GtkWidget  *comment_entry;
  GtkObject  *pw_spinbutton_from_adj;
  GtkObject  *pw_spinbutton_to_adj;
  GtkObject  *pw_spinbutton_loops_adj;
  GtkObject  *pw_spinbutton_seltrack_adj;
  GtkObject  *pw_spinbutton_delace_adj;
  GtkWidget  *pw_spinbutton_delace;
  GtkObject  *pw_spinbutton_step_density_adj;
  GtkWidget  *pw_framenr_label;
  GtkWidget  *pw_frametime_label;

  struct GapStbPropWidget *next;
} GapStbPropWidget;

typedef struct GapStbFrameWidget  /* nickname: fw */
{
  GtkWidget *event_box;
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *hbox;
  GtkWidget *key_label;      /* for the 6-digit framenumber */
  GtkWidget *val_label;
  GapPView   *pv_ptr;            /* for gap preview rendering based on drawing_area */

  GapStoryElem  *stb_elem_refptr;  /* never g_free this one ! */
  GapStoryBoard *stb_refptr;       /* never g_free this one ! */

  gint32    seq_nr;          /* position in the storyboard list (starting at 1) */
  char      *frame_filename;

  void  *sgpp;               /* never g_free this one ! */
  void  *tabw;               /* never g_free this one ! (pointer to parent GapStbTabWidgets) */
} GapStbFrameWidget;


typedef struct GapStbTabWidgets  /* nickname: tabw */
{
  GapStoryMasterType type;
  GapStbFrameWidget  **fw_tab;
  gint32 fw_tab_size;
  gint32 mount_col;
  gint32 mount_row;
  gint32 cols;
  gint32 rows;
  gint32 thumbsize;
  gint32 thumb_width;
  gint32 thumb_height;
  gint32 rowpage;

  gboolean  master_dlg_open;

  GtkWidget *mount_table;
  GtkWidget *fw_gtk_table;
  GtkWidget *frame_with_name;
  GtkWidget *total_rows_label;
  GtkObject *rowpage_spinbutton_adj;
  GtkObject *rowpage_vscale_adj;
  GtkWidget *rowpage_vscale;

  GtkWidget *filesel;
  GtkWidget *filename_entry;
  char      *filename_refptr;   /* never g_free this one ! */
  gint32     filename_maxlen;
  GtkWidget *load_button;
  GtkWidget *save_button;
  GtkWidget *play_button;

  GtkWidget *edit_cut_button;
  GtkWidget *edit_copy_button;
  GtkWidget *edit_paste_button;

  GapStbPropWidget *pw;
  GapStoryElemDisplayMode edmode;
  void  *sgpp;               /* never g_free this one ! */

} GapStbTabWidgets;


typedef struct GapStbMainGlobalParams  /* nickname: sgpp */
{
  GimpRunMode  run_mode;
  gboolean     initialized;   /* FALSE at startup */
  gboolean     run;
  gint32       image_id;

  gchar        storyboard_filename[GAP_STORY_MAX_STORYFILENAME_LEN];
  gchar        cliplist_filename[GAP_STORY_MAX_STORYFILENAME_LEN];

  GapStoryBoard *stb;
  GapStoryBoard *cll;
  GapStoryBoard *curr_selection;
  GapPlayerMainGlobalParams *plp;

  /* GUI widget pointers */
  GapStbTabWidgets  *stb_widgets;
  GapStbTabWidgets  *cll_widgets;
  
  GapStoryVideoElem *video_list;
  GapVThumbElem     *vthumb_list;
  t_GVA_Handle      *gvahand;
  gchar             *gva_videofile;
  GtkWidget         *progress_bar;
  gboolean           gva_lock;
  gboolean           cancel_video_api;
  gboolean           auto_vthumb;
  gboolean           auto_vthumb_refresh_canceled;
  gboolean           in_player_call;

  GtkWidget *shell_window;  
  GtkWidget *player_frame;  

  GtkWidget *menu_item_win_vthumbs;

  GtkWidget *menu_item_stb_save;
  GtkWidget *menu_item_stb_save_as;
  GtkWidget *menu_item_stb_add_clip;
  GtkWidget *menu_item_stb_playback;
  GtkWidget *menu_item_stb_properties;
  GtkWidget *menu_item_stb_close;

  GtkWidget *menu_item_cll_save;
  GtkWidget *menu_item_cll_save_as;
  GtkWidget *menu_item_cll_add_clip;
  GtkWidget *menu_item_cll_playback;
  GtkWidget *menu_item_cll_properties;
  GtkWidget *menu_item_cll_close;
  

} GapStbMainGlobalParams;


#endif
