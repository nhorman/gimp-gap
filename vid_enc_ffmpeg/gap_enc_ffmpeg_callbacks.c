/* gap_enc_ffmpeg_callbacks.c
 * 2004.05.12 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * This Module contains FFMPEG specific Video Encoder GUI Callback Procedures
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
 * version 2.1.0a;  2004.05.12 : created
 */

#include <config.h>

#include <gtk/gtk.h>

#include "gap_enc_ffmpeg_main.h"
#include "gap_enc_ffmpeg_gui.h"
#include "gap_enc_ffmpeg_callbacks.h"

/* Includes for encoder specific extra LIBS */
#include "avformat.h"
#include "avcodec.h"

/* ---------------------------------
 * on_ff_response
 * ---------------------------------
 */
void
on_ff_response (GtkWidget *widget,
                 gint       response_id,
                 GapGveFFMpegGlobalParams *gpp)
{
  GtkWidget *dialog;
  char *l_msg;

  if(gpp)
  {
    gpp->val.run = FALSE;
  }

  switch (response_id)
  {
    case GTK_RESPONSE_OK:
      if(gpp)
      {
        if (gpp->val.vid_width % 2 != 0)
        {
            l_msg = g_strdup_printf("Error:\nWidth (%d) must be an integer multiple of 2",
                                 (int)gpp->val.vid_width);
           g_message(l_msg);
           g_free(l_msg);
           return;
        }
        if (gpp->val.vid_height % 2 != 0)
        {
            l_msg = g_strdup_printf("Error:\nHeight (%d) must be an integer multiple of 2",
                               (int)gpp->val.vid_height);
            g_message(l_msg);
            g_free(l_msg);
            return;
        }
        gpp->val.run = TRUE;
      }
      /* now run into the default case, to close the shell_window (dont break) */
    default:
      dialog = NULL;
      if(gpp)
      {
        dialog = gpp->shell_window;
        if(dialog)
        {
          gpp->shell_window = NULL;
          gtk_widget_destroy (dialog);
        }
      }
      gtk_main_quit ();
      break;
  }
}  /* end on_ff_response */


/* --- optionmenus with string  results
 * --- accessing data direct, using GAP_ENC_FFGUI_ENC_MENU_ITEM_ENC_PTR
 */

void
on_ff_fileformat_optionmenu  (GtkWidget     *wgt_item,
                              GapGveFFMpegGlobalParams *gpp)
{
 gchar *name;

 if(gap_debug) printf("CB: on_ff_fileformat_optionmenu\n");

 if(gpp == NULL) return;

 name = (gchar *) g_object_get_data (G_OBJECT (wgt_item)
                                   , GAP_ENC_FFGUI_ENC_MENU_ITEM_ENC_PTR);

 if(name)
 {
    if(gap_debug) printf("name: %s\n", name);
    if(strcmp(gpp->evl.format_name, name) != 0)
    {
      g_snprintf(gpp->evl.format_name, sizeof(gpp->evl.format_name), "%s", name);
      gap_enc_ffgui_set_default_codecs(gpp, TRUE);               /* update info labels AND set CODEC optionmenus */
    }
 }
}

void
on_ff_vid_codec_optionmenu  (GtkWidget     *wgt_item,
                             GapGveFFMpegGlobalParams *gpp)
{
 gchar *name;

 if(gap_debug) printf("CB: on_ff_vid_codec_optionmenu\n");

 if(gpp == NULL) return;

 name = (gchar *) g_object_get_data (G_OBJECT (wgt_item)
                                   , GAP_ENC_FFGUI_ENC_MENU_ITEM_ENC_PTR);

 if(name)
 {
    if(gap_debug) printf("name: %s\n", name);
    g_snprintf(gpp->evl.vcodec_name, sizeof(gpp->evl.vcodec_name), "%s", name);
 }
}

void
on_ff_aud_codec_optionmenu  (GtkWidget     *wgt_item,
                             GapGveFFMpegGlobalParams *gpp)
{
 gchar *name;

 if(gap_debug) printf("CB: on_ff_aud_codec_optionmenu\n");

 if(gpp == NULL) return;

 name = (gchar *) g_object_get_data (G_OBJECT (wgt_item)
                                     , GAP_ENC_FFGUI_ENC_MENU_ITEM_ENC_PTR);

 if(name)
 {
    if(gap_debug) printf("name: %s\n", name);
    g_snprintf(gpp->evl.acodec_name, sizeof(gpp->evl.acodec_name), "%s", name);
 }
}

/* --- optionmenus with gint32 results
 * --- access data by Index GAP_GVE_MENU_ITEM_INDEX_KEY
 */

void
on_ff_aud_bitrate_optionmenu  (GtkWidget     *wgt_item,
                               GapGveFFMpegGlobalParams *gpp)
{
  gint       l_idx;
  gint       l_krate;

 if(gap_debug) printf("CB: on_ff_aud_bitrate_optionmenu\n");

 if(gpp == NULL) return;

 l_idx = (gint) g_object_get_data (G_OBJECT (wgt_item), GAP_GVE_MENU_ITEM_INDEX_KEY);

 if(gap_debug) printf("CB: on_ff_aud_bitrate_optionmenu index: %d\n", (int)l_idx);

 l_krate = gap_enc_ffgui_gettab_audio_krate(l_idx);
 if(gpp->evl.audio_bitrate != l_krate)
 {
    gpp->evl.audio_bitrate = l_krate;
    gap_enc_ffgui_init_main_dialog_widgets(gpp);     /* for update audio_bitrate spinbutton */
 }
}

void
on_ff_motion_estimation_optionmenu  (GtkWidget     *wgt_item,
                                     GapGveFFMpegGlobalParams *gpp)
{
  gint       l_idx;

 if(gap_debug) printf("CB: on_ff_motion_estimation_optionmenu\n");

 if(gpp == NULL) return;

 l_idx = (gint) g_object_get_data (G_OBJECT (wgt_item), GAP_GVE_MENU_ITEM_INDEX_KEY);

 if(gap_debug) printf("CB: on_ff_motion_estimation_optionmenu index: %d\n", (int)l_idx);
 gpp->evl.motion_estimation = gap_enc_ffgui_gettab_motion_est(l_idx);
}

void
on_ff_dct_algo_optionmenu  (GtkWidget     *wgt_item,
                           GapGveFFMpegGlobalParams *gpp)
{
  gint       l_idx;

 if(gap_debug) printf("CB: on_ff_dct_algo_optionmenu\n");

 if(gpp == NULL) return;

 l_idx = (gint) g_object_get_data (G_OBJECT (wgt_item), GAP_GVE_MENU_ITEM_INDEX_KEY);

 if(gap_debug) printf("CB: on_ff_dct_algo_optionmenu index: %d\n", (int)l_idx);
 gpp->evl.dct_algo = gap_enc_ffgui_gettab_dct_algo(l_idx);
}


void
on_ff_idct_algo_optionmenu  (GtkWidget     *wgt_item,
                           GapGveFFMpegGlobalParams *gpp)
{
  gint       l_idx;

 if(gap_debug) printf("CB: on_ff_idct_algo_optionmenu\n");

 if(gpp == NULL) return;

 l_idx = (gint) g_object_get_data (G_OBJECT (wgt_item), GAP_GVE_MENU_ITEM_INDEX_KEY);

 if(gap_debug) printf("CB: on_ff_idct_algo_optionmenu index: %d\n", (int)l_idx);
 gpp->evl.idct_algo = gap_enc_ffgui_gettab_idct_algo(l_idx);
}

void
on_ff_mb_decision_optionmenu  (GtkWidget     *wgt_item,
                           GapGveFFMpegGlobalParams *gpp)
{
  gint       l_idx;

 if(gap_debug) printf("CB: on_ff_mb_decision_optionmenu\n");

 if(gpp == NULL) return;

 l_idx = (gint) g_object_get_data (G_OBJECT (wgt_item), GAP_GVE_MENU_ITEM_INDEX_KEY);

 if(gap_debug) printf("CB: on_ff_mb_decision_optionmenu index: %d\n", (int)l_idx);
 gpp->evl.mb_decision = gap_enc_ffgui_gettab_idct_algo(l_idx);
}


void
on_ff_presets_optionmenu  (GtkWidget     *wgt_item,
                           GapGveFFMpegGlobalParams *gpp)
{
  gint       l_idx;

 if(gap_debug) printf("CB: on_ff_presets_optionmenu\n");

 if(gpp == NULL) return;

 l_idx = (gint) g_object_get_data (G_OBJECT (wgt_item), GAP_GVE_MENU_ITEM_INDEX_KEY);

 if(gap_debug) printf("CB: on_ff_presets_optionmenu index: %d\n", (int)l_idx);

 if((l_idx <= GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS) && (l_idx > 0))
 {
   /* index 0 is used for OOPS do not use preset menu entry and is not a PRESET
    * menu_index 1  does access presets[0]
    */
   l_idx--;

   if(gap_debug) printf ("** encoder PRESET: %d\n", (int)l_idx);
   gap_enc_ffmpeg_main_init_preset_params(&gpp->evl, l_idx);
   gap_enc_ffgui_init_main_dialog_widgets(gpp);                /* update all wdgets */
 }
}






/* ---------------------------------
 * fsb fileselct  callbacks
 * ---------------------------------
 */


void
on_fsb__fileselection_destroy          (GtkObject       *object,
                                        GapGveFFMpegGlobalParams *gpp)
{
 if(gap_debug) printf("CB: on_fsb__fileselection_destroy\n");

 if(gpp == NULL) return;

 gpp->fsb__fileselection = NULL;
}


void
on_fsb__ok_button_clicked              (GtkButton       *button,
                                        GapGveFFMpegGlobalParams *gpp)
{
 const gchar *filename;
 GtkEntry *entry;

 if(gap_debug) printf("CB: on_fsb__ok_button_clicked\n");
 if(gpp == NULL) return;

 if(gpp->fsb__fileselection)
 {
   filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (gpp->fsb__fileselection));
   g_snprintf(gpp->evl.passlogfile, sizeof(gpp->evl.passlogfile), "%s"
             , filename);
   entry = GTK_ENTRY(gpp->ff_passlogfile_entry);
   if(entry)
   {
        gtk_entry_set_text(entry, filename);
   }
   on_fsb__cancel_button_clicked(NULL, (gpointer)gpp);
 }
}


void
on_fsb__cancel_button_clicked          (GtkButton       *button,
                                        GapGveFFMpegGlobalParams *gpp)
{
 if(gap_debug) printf("CB: on_fsb__cancel_button_clicked\n");
 if(gpp == NULL) return;

 if(gpp->fsb__fileselection)
 {
   gtk_widget_destroy(gpp->fsb__fileselection);
   gpp->fsb__fileselection = NULL;
 }
}

/* ---------------------------------
 * FFMPEG ENCODER Dialog
 * ---------------------------------
 */


/* ---------------------------------
 * Other Widgets
 * ---------------------------------
 */




void
on_ff_aud_bitrate_spinbutton_changed   (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp)
{
  GtkAdjustment *adj;

 if(gap_debug) printf("CB: on_ff_aud_bitrate_spinbutton_changed\n");


 if(gpp == NULL) return;
 adj = GTK_ADJUSTMENT(gpp->ff_aud_bitrate_spinbutton_adj);
 if(adj)
 {
   if(gap_debug) printf("spin value: %f\n", (float)adj->value );

   if((gint32)adj->value != gpp->evl.audio_bitrate)
   {
     gpp->evl.audio_bitrate = (gint32)adj->value;
   }
 }
}


void
on_ff_vid_bitrate_spinbutton_changed   (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp)
{
  GtkAdjustment *adj;

 if(gap_debug) printf("CB: on_ff_vid_bitrate_spinbutton_changed\n");


 if(gpp == NULL) return;
 adj = GTK_ADJUSTMENT(gpp->ff_vid_bitrate_spinbutton_adj);
 if(adj)
 {
   if(gap_debug) printf("spin value: %f\n", (float)adj->value );

   if((gint32)adj->value != gpp->evl.video_bitrate)
   {
     gpp->evl.video_bitrate = (gint32)adj->value;
   }
 }
}


void
on_ff_qscale_spinbutton_changed        (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp)
{
  GtkAdjustment *adj;

 if(gap_debug) printf("CB: on_ff_qscale_spinbutton_changed\n");


 if(gpp == NULL) return;
 adj = GTK_ADJUSTMENT(gpp->ff_qscale_spinbutton_adj);
 if(adj)
 {
   if(gap_debug) printf("spin value: %f\n", (float)adj->value );

   if((gint32)adj->value != gpp->evl.qscale)
   {
     gpp->evl.qscale = (gint32)adj->value;
   }
 }
}


void
on_ff_qmin_spinbutton_changed          (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp)
{
  GtkAdjustment *adj;

 if(gap_debug) printf("CB: on_ff_qmin_spinbutton_changed\n");


 if(gpp == NULL) return;
 adj = GTK_ADJUSTMENT(gpp->ff_qmin_spinbutton_adj);
 if(adj)
 {
   if(gap_debug) printf("spin value: %f\n", (float)adj->value );

   if((gint32)adj->value != gpp->evl.qmin)
   {
     gpp->evl.qmin = (gint32)adj->value;
   }
 }
}


void
on_ff_qmax_spinbutton_changed          (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp)
{
  GtkAdjustment *adj;

 if(gap_debug) printf("CB: on_ff_qmax_spinbutton_changed\n");


 if(gpp == NULL) return;
 adj = GTK_ADJUSTMENT(gpp->ff_qmax_spinbutton_adj);
 if(adj)
 {
   if(gap_debug) printf("spin value: %f\n", (float)adj->value );

   if((gint32)adj->value != gpp->evl.qmax)
   {
     gpp->evl.qmax = (gint32)adj->value;
   }
 }
}


void
on_ff_qdiff_spinbutton_changed         (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp)
{
  GtkAdjustment *adj;

 if(gap_debug) printf("CB: on_ff_qdiff_spinbutton_changed\n");


 if(gpp == NULL) return;
 adj = GTK_ADJUSTMENT(gpp->ff_qdiff_spinbutton_adj);
 if(adj)
 {
   if(gap_debug) printf("spin value: %f\n", (float)adj->value );

   if((gint32)adj->value != gpp->evl.qdiff)
   {
     gpp->evl.qdiff = (gint32)adj->value;
   }
 }
}


void
on_ff_gop_size_spinbutton_changed      (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp)
{
  GtkAdjustment *adj;

 if(gap_debug) printf("CB: on_ff_gop_size_spinbutton_changed\n");


 if(gpp == NULL) return;
 adj = GTK_ADJUSTMENT(gpp->ff_gop_size_spinbutton_adj);
 if(adj)
 {
   if(gap_debug) printf("spin value: %f\n", (float)adj->value );

   if((gint32)adj->value != gpp->evl.gop_size)
   {
     gpp->evl.gop_size = (gint32)adj->value;
   }
 }
}


void
on_ff_intra_checkbutton_toggled       (GtkToggleButton *checkbutton,
                                        GapGveFFMpegGlobalParams *gpp)
{
 if(gap_debug) printf("CB: on_ff_intra_checkbutton_toggled\n");

 if(gpp == NULL) return;

  if (checkbutton->active)
  {
     gpp->evl.intra = TRUE;
     gtk_widget_set_sensitive(gpp->ff_gop_size_spinbutton ,FALSE);
  }
  else
  {
     gpp->evl.intra = FALSE;
     gtk_widget_set_sensitive(gpp->ff_gop_size_spinbutton ,TRUE);
  }
}


void
on_ff_aic_checkbutton_toggled (GtkToggleButton *checkbutton,
                                        GapGveFFMpegGlobalParams *gpp)
{
 if(gpp == NULL) return;

  if (checkbutton->active)
  {
     gpp->evl.aic = TRUE;
  }
  else
  {
     gpp->evl.aic = FALSE;
  }
}

void
on_ff_umv_checkbutton_toggled (GtkToggleButton *checkbutton,
                                        GapGveFFMpegGlobalParams *gpp)
{
 if(gpp == NULL) return;

  if (checkbutton->active)
  {
     gpp->evl.umv = TRUE;
  }
  else
  {
     gpp->evl.umv = FALSE;
  }
}

void
on_ff_bitexact_checkbutton_toggled (GtkToggleButton *checkbutton,
                                        GapGveFFMpegGlobalParams *gpp)
{
 if(gpp == NULL) return;

  if (checkbutton->active)
  {
     gpp->evl.bitexact = TRUE;
  }
  else
  {
     gpp->evl.bitexact = FALSE;
  }
}

void
on_ff_aspect_checkbutton_toggled (GtkToggleButton *checkbutton,
                                  GapGveFFMpegGlobalParams *gpp)
{
 if(gpp == NULL) return;

  if (checkbutton->active)
  {
     gpp->evl.set_aspect_ratio = TRUE;
  }
  else
  {
     gpp->evl.set_aspect_ratio = FALSE;
  }
}


void
on_ff_b_frames_checkbutton_toggled    (GtkToggleButton *checkbutton,
                                        GapGveFFMpegGlobalParams *gpp)
{
 if(gap_debug) printf("CB: on_ff_b_frames_checkbutton_toggled\n");

 if(gpp == NULL) return;

  if (checkbutton->active)
  {
     gpp->evl.b_frames = 1; 
     /* dont use TRUE for b_frames flag
      * because this flag is also used as max number of b-frames
      */
  }
  else
  {
     gpp->evl.b_frames = 0;
  }
}


void
on_ff_mv4_checkbutton_toggled         (GtkToggleButton *checkbutton,
                                        GapGveFFMpegGlobalParams *gpp)
{
 if(gap_debug) printf("CB: on_ff_mv4_checkbutton_toggled\n");

 if(gpp == NULL) return;

  if (checkbutton->active)
  {
     gpp->evl.mv4 = TRUE;
  }
  else
  {
     gpp->evl.mv4 = FALSE;
  }
}


void
on_ff_partitioning_checkbutton_toggled
                                        (GtkToggleButton *checkbutton,
                                        GapGveFFMpegGlobalParams *gpp)
{
 if(gap_debug) printf("CB: on_ff_partitioning_checkbutton_toggled\n");

 if(gpp == NULL) return;

  if (checkbutton->active)
  {
     gpp->evl.partitioning = TRUE;
  }
  else
  {
     gpp->evl.partitioning = FALSE;
  }
}


void
on_ff_qblur_spinbutton_changed         (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp)
{
  GtkAdjustment *adj;

 if(gap_debug) printf("CB: on_ff_qblur_spinbutton_changed\n");

 if(gpp == NULL) return;
 adj = GTK_ADJUSTMENT(gpp->ff_qblur_spinbutton_adj);
 if(adj)
 {
   if(gap_debug) printf("spin value: %f\n", (float)adj->value );

   if((gdouble)adj->value != gpp->evl.qblur)
   {
     gpp->evl.qblur = (gdouble)adj->value;
   }
 }
}


void
on_ff_qcomp_spinbutton_changed         (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp)
{
  GtkAdjustment *adj;

 if(gap_debug) printf("CB: on_ff_qcomp_spinbutton_changed\n");

 if(gpp == NULL) return;
 adj = GTK_ADJUSTMENT(gpp->ff_qcomp_spinbutton_adj);
 if(adj)
 {
   if(gap_debug) printf("spin value: %f\n", (float)adj->value );

   if((gdouble)adj->value != gpp->evl.qcomp)
   {
     gpp->evl.qcomp = (gdouble)adj->value;
   }
 }
}


void
on_ff_rc_init_cplx_spinbutton_changed  (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp)
{
  GtkAdjustment *adj;

 if(gap_debug) printf("CB: on_ff_rc_init_cplx_spinbutton_changed\n");

 if(gpp == NULL) return;
 adj = GTK_ADJUSTMENT(gpp->ff_rc_init_cplx_spinbutton_adj);
 if(adj)
 {
   if(gap_debug) printf("spin value: %f\n", (float)adj->value );

   if((gdouble)adj->value != gpp->evl.rc_init_cplx)
   {
     gpp->evl.rc_init_cplx = (gdouble)adj->value;
   }
 }
}


void
on_ff_b_qfactor_spinbutton_changed     (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp)
{
  GtkAdjustment *adj;

 if(gap_debug) printf("CB: on_ff_b_qfactor_spinbutton_changed\n");

 if(gpp == NULL) return;
 adj = GTK_ADJUSTMENT(gpp->ff_b_qfactor_spinbutton_adj);
 if(adj)
 {
   if(gap_debug) printf("spin value: %f\n", (float)adj->value );

   if((gdouble)adj->value != gpp->evl.b_qfactor)
   {
     gpp->evl.b_qfactor = (gdouble)adj->value;
   }
 }
}


void
on_ff_i_qfactor_spinbutton_changed     (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp)
{
  GtkAdjustment *adj;

 if(gap_debug) printf("CB: on_ff_i_qfactor_spinbutton_changed\n");

 if(gpp == NULL) return;
 adj = GTK_ADJUSTMENT(gpp->ff_i_qfactor_spinbutton_adj);
 if(adj)
 {
   if(gap_debug) printf("spin value: %f\n", (float)adj->value );

   if((gdouble)adj->value != gpp->evl.i_qfactor)
   {
     gpp->evl.i_qfactor = (gdouble)adj->value;
   }
 }
}


void
on_ff_b_qoffset_spinbutton_changed     (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp)
{
  GtkAdjustment *adj;

 if(gap_debug) printf("CB: on_ff_b_qoffset_spinbutton_changed\n");

 if(gpp == NULL) return;
 adj = GTK_ADJUSTMENT(gpp->ff_b_qoffset_spinbutton_adj);
 if(adj)
 {
   if(gap_debug) printf("spin value: %f\n", (float)adj->value );

   if((gdouble)adj->value != gpp->evl.b_qoffset)
   {
     gpp->evl.b_qoffset = (gdouble)adj->value;
   }
 }
}


void
on_ff_i_qoffset_spinbutton_changed     (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp)
{
  GtkAdjustment *adj;

 if(gap_debug) printf("CB: on_ff_i_qoffset_spinbutton_changed\n");

 if(gpp == NULL) return;
 adj = GTK_ADJUSTMENT(gpp->ff_i_qoffset_spinbutton_adj);
 if(adj)
 {
   if(gap_debug) printf("spin value: %f\n", (float)adj->value );

   if((gdouble)adj->value != gpp->evl.i_qoffset)
   {
     gpp->evl.i_qoffset = (gdouble)adj->value;
   }
 }
}


void
on_ff_bitrate_tol_spinbutton_changed   (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp)
{
  GtkAdjustment *adj;

 if(gap_debug) printf("CB: on_ff_bitrate_tol_spinbutton_changed\n");

 if(gpp == NULL) return;
 adj = GTK_ADJUSTMENT(gpp->ff_bitrate_tol_spinbutton_adj);
 if(adj)
 {
   if(gap_debug) printf("spin value: %f\n", (float)adj->value );

   if((gint32)adj->value != gpp->evl.bitrate_tol)
   {
     gpp->evl.bitrate_tol = (gint32)adj->value;
   }
 }
}


void
on_ff_maxrate_tol_spinbutton_changed   (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp)
{
  GtkAdjustment *adj;

 if(gap_debug) printf("CB: on_ff_maxrate_tol_spinbutton_changed\n");

 if(gpp == NULL) return;
 adj = GTK_ADJUSTMENT(gpp->ff_maxrate_tol_spinbutton_adj);
 if(adj)
 {
   if(gap_debug) printf("spin value: %f\n", (float)adj->value );

   if((gint32)adj->value != gpp->evl.maxrate_tol)
   {
     gpp->evl.maxrate_tol = (gint32)adj->value;
   }
 }
}


void
on_ff_minrate_tol_spinbutton_changed   (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp)
{
  GtkAdjustment *adj;

 if(gap_debug) printf("CB: on_ff_minrate_tol_spinbutton_changed\n");

 if(gpp == NULL) return;
 adj = GTK_ADJUSTMENT(gpp->ff_minrate_tol_spinbutton_adj);
 if(adj)
 {
   if(gap_debug) printf("spin value: %f\n", (float)adj->value );

   if((gint32)adj->value != gpp->evl.minrate_tol)
   {
     gpp->evl.minrate_tol = (gint32)adj->value;
   }
 }
}


void
on_ff_bufsize_spinbutton_changed       (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp)
{
  GtkAdjustment *adj;

 if(gap_debug) printf("CB: on_ff_bufsize_spinbutton_changed\n");

 if(gpp == NULL) return;
 adj = GTK_ADJUSTMENT(gpp->ff_bufsize_spinbutton_adj);
 if(adj)
 {
   if(gap_debug) printf("spin value: %f\n", (float)adj->value );

   if((gint32)adj->value != gpp->evl.bufsize)
   {
     gpp->evl.bufsize = (gint32)adj->value;
   }
 }
}


void
on_ff_passlogfile_entry_changed        (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp)
{
 if(gap_debug) printf("CB: on_ff_passlogfile_entry_changed\n");

 if(gpp)
 {
    g_snprintf(gpp->evl.passlogfile, sizeof(gpp->evl.passlogfile), "%s"
              , gtk_entry_get_text(GTK_ENTRY(editable)));
 }
}


void
on_ff_passlogfile_filesel_button_clicked
                                        (GtkButton       *button,
                                        GapGveFFMpegGlobalParams *gpp)
{
 if(gap_debug) printf("CB: on_ff_passlogfile_filesel_button_clicked\n");

 if(gpp)
 {
   if(gpp->fsb__fileselection != NULL)
   {
     gtk_window_present(GTK_WINDOW(gpp->fsb__fileselection));
     return;
   }

   gpp->fsb__fileselection = gap_enc_ffgui_create_fsb__fileselection(gpp);
   if(gpp->fsb__fileselection)
   {
     gtk_widget_show (gpp->fsb__fileselection);
   }
 }
}


void
on_ff_pass_checkbutton_toggled        (GtkToggleButton *checkbutton,
                                        GapGveFFMpegGlobalParams *gpp)
{
 if(gap_debug) printf("CB: on_ff_pass_checkbutton_toggled\n");

 if(gpp == NULL) return;

  if (checkbutton->active)
  {
     gpp->evl.pass = 2;
  }
  else
  {
     gpp->evl.pass = 1;
  }
}


void
on_ff_title_entry_changed              (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp)
{
 if(gap_debug) printf("CB: on_ff_title_entry_changed\n");

 if(gpp)
 {
    g_snprintf(gpp->evl.title, sizeof(gpp->evl.title), "%s"
              , gtk_entry_get_text(GTK_ENTRY(editable)));
 }
}


void
on_ff_author_entry_changed             (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp)
{
 if(gap_debug) printf("CB: on_ff_author_entry_changed\n");

 if(gpp)
 {
    g_snprintf(gpp->evl.author, sizeof(gpp->evl.author), "%s"
              , gtk_entry_get_text(GTK_ENTRY(editable)));
 }
}


void
on_ff_copyright_entry_changed          (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp)
{
 if(gap_debug) printf("CB: on_ff_copyright_entry_changed\n");

 if(gpp)
 {
    g_snprintf(gpp->evl.copyright, sizeof(gpp->evl.copyright), "%s"
              , gtk_entry_get_text(GTK_ENTRY(editable)));
 }
}


void
on_ff_comment_entry_changed            (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp)
{
 if(gap_debug) printf("CB: on_ff_comment_entry_changed\n");

 if(gpp)
 {
    g_snprintf(gpp->evl.comment, sizeof(gpp->evl.comment), "%s"
              , gtk_entry_get_text(GTK_ENTRY(editable)));
 }

}


void
on_ff_strict_spinbutton_changed        (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp)
{
  GtkAdjustment *adj;

 if(gap_debug) printf("CB: on_ff_strict_spinbutton_changed\n");

 if(gpp == NULL) return;
 adj = GTK_ADJUSTMENT(gpp->ff_strict_spinbutton_adj);
 if(adj)
 {
   if(gap_debug) printf("spin value: %f\n", (float)adj->value );

   if((gint32)adj->value != gpp->evl.strict)
   {
     gpp->evl.strict = (gint32)adj->value;
   }
 }

}



void
on_ff_mb_qmin_spinbutton_changed       (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp)
{
  GtkAdjustment *adj;

 if(gap_debug) printf("CB: on_ff_mb_qmin_spinbutton_changed\n");

 if(gpp == NULL) return;
 adj = GTK_ADJUSTMENT(gpp->ff_mb_qmin_spinbutton_adj);
 if(adj)
 {
   if(gap_debug) printf("spin value: %f\n", (float)adj->value );

   if((gint32)adj->value != gpp->evl.mb_qmin)
   {
     gpp->evl.mb_qmin = (gint32)adj->value;
   }
 }

}


void
on_ff_mb_qmax_spinbutton_changed       (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp)
{
  GtkAdjustment *adj;

 if(gap_debug) printf("CB: on_ff_mb_qmax_spinbutton_changed\n");

 if(gpp == NULL) return;
 adj = GTK_ADJUSTMENT(gpp->ff_mb_qmax_spinbutton_adj);
 if(adj)
 {
   if(gap_debug) printf("spin value: %f\n", (float)adj->value );

   if((gint32)adj->value != gpp->evl.mb_qmax)
   {
     gpp->evl.mb_qmax = (gint32)adj->value;
   }
 }

}


void
on_ff_dont_recode_checkbutton_toggled  (GtkToggleButton *checkbutton,
                                        GapGveFFMpegGlobalParams *gpp)
{
 if(gap_debug) printf("CB: on_ff_dont_recode_checkbutton_toggled\n");

 if(gpp == NULL) return;

  if (checkbutton->active)
  {
     gpp->evl.dont_recode_flag = TRUE;
  }
  else
  {
     gpp->evl.dont_recode_flag = FALSE;
  }
}
