/*  gap_player_main.h
 *
 *  This module handles GAP video playback
 *  based on thumbnail files
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
 * version 1.3.19a; 2003/09/07  hof: audiosupport (based on wavplay, for UNIX only),
 * version 1.3.15a; 2003/06/21  hof: created
 */

#ifndef _GAP_PLAYER_MAIN_H
#define _GAP_PLAYER_MAIN_H

#include "libgimp/gimp.h"
#include "gap_lib.h"
#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include "gap_pview_da.h"

#define MAX_AUDIOFILE_LEN 1024

typedef struct t_global_params {
  GimpRunMode  run_mode;
  gint32       image_id;

  t_anim_info *ainfo_ptr;
  
  gboolean   autostart;
  gboolean   use_thumbnails;
  gboolean   exact_timing;      /* TRUE: allow drop frames fro exact timing, FALSE: disable drop */
  gboolean   play_is_active;
  gboolean   play_selection_only;
  gboolean   play_loop;
  gboolean   play_pingpong;
  gboolean   play_backward;

  gint32     play_timertag;

  gint32   begin_frame;
  gint32   end_frame;
  gint32   play_current_framenr;
  gint32   pb_stepsize;
  gdouble  speed;             /* playback speed fps */
  gdouble  original_speed;    /* playback speed fps */
  gdouble  prev_speed;        /* previous playback speed fps */
  gint32   pv_pixelsize;      /* 32 upto 512 */
  gint32   pv_width;
  gint32   pv_height;
  
  /* lockflags */  
  gboolean in_feedback;
  gboolean in_timer_playback;
  gboolean in_resize;         /* for disable resize while initial startup */

  gint32   go_job_framenr;
  gint32   go_timertag;
  gint32   go_base_framenr;
  gint32   go_base;
  gint32   pingpong_count;
  
  /* GUI widget pointers */
  t_pview   *pv_ptr;
  GtkWidget *shell_window;  
  GtkObject *from_spinbutton_adj;
  GtkObject *to_spinbutton_adj;
  GtkObject *framenr_spinbutton_adj;
  GtkObject *speed_spinbutton_adj;
  GtkObject *size_spinbutton_adj;

  GtkWidget *status_label;
  GtkWidget *timepos_label;
  GtkWidget *table11;
  GtkWidget *size_spinbutton;

  GTimer    *gtimer;
  gdouble   cycle_time_secs;
  gdouble   rest_secs;
  gdouble   delay_secs;
  gdouble   framecnt;
  
  gint32    resize_count;
  gint32    old_resize_width;
  gint32    old_resize_height;
  
  /* audio stuff */
  gboolean  audio_enable;
  gchar     audio_filename[MAX_AUDIOFILE_LEN];
  gint32    audio_frame_offset;
  guint32   audio_samplerate;
  guint32   audio_required_samplerate;
  guint32   audio_bits;
  guint32   audio_channels;
  guint32   audio_samples;  
  gint32    audio_status;
  gdouble   audio_volume;   /* 0.0 upto 1.0 */
  
  GtkWidget *audio_filename_entry;
  GtkWidget *audio_offset_time_label;
  GtkWidget *audio_total_time_label;
  GtkWidget *audio_total_frames_label;
  GtkWidget *audio_samples_label;
  GtkWidget *audio_samplerate_label;
  GtkWidget *audio_bits_label;
  GtkWidget *audio_channels_label;
  GtkObject *audio_volume_spinbutton_adj;
  GtkObject *audio_frame_offset_spinbutton_adj;
  GtkWidget *audio_filesel;
 
} t_global_params;

#define AUSTAT_NONE             0
#define AUSTAT_SERVER_STARTED   1
#define AUSTAT_FILENAME_SET     2
#define AUSTAT_PLAYING          3

#define MIN_SAMPLERATE   8000 
#define MAX_SAMPLERATE   48000

#endif
