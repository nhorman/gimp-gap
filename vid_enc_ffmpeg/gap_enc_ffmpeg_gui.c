/* gap_enc_ffmpeg_gui.c
 * 2003.01.07 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * This Module contains FFMPEG specific Video Encoder GUI Procedures
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
 * version 2.1.0a;  2004.06.05   hof: update params from ffmpeg 0.4.6 to 0.4.8
 * version 2.1.0a;  2004.05.12   hof: integration into gimp-gap project
 * version 1.2.2c;  2003.05.29   hof: dont_recode_flag
 * version 1.2.2c;  2003.01.07   hof: created
 */

#include <config.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>



#include "gap_libgapvidutil.h"
#include "gap-intl.h"

#include "gap_libgapvidutil.h"
#include "gap_libgimpgap.h"

#include "gap_enc_ffmpeg_main.h"
#include "gap_enc_ffmpeg_gui.h"
#include "gap_enc_ffmpeg_callbacks.h"

/* Includes for encoder specific extra LIBS */
#include "avformat.h"
#include "avcodec.h"

typedef struct
{
  char *name;
  gint  menu_idx;
  void *next;
} t_string_optionmenu_elem;

static void     p_replace_optionmenu_file_format(GapGveFFMpegGlobalParams *gpp);
static void     p_replace_optionmenu_vid_codec(GapGveFFMpegGlobalParams *gpp);
static void     p_replace_optionmenu_aud_codec(GapGveFFMpegGlobalParams *gpp);
static void     p_init_spinbuttons(GapGveFFMpegGlobalParams *gpp);
static void     p_init_vid_checkbuttons(GapGveFFMpegGlobalParams *gpp);
static void     p_init_entry_widgets(GapGveFFMpegGlobalParams *gpp);
static char*    p_init_optionmenu_actual_nameidx(GapGveFFMpegGlobalParams *gpp, GtkWidget *wgt, t_string_optionmenu_elem *list, char *name);

static gint     gtab_motion_est[GAP_GVE_FFMPEG_MOTION_ESTIMATION_MAX_ELEMENTS] =  { 1, 2, 3, 4, 5, 6 };
static gint     gtab_dct_algo[GAP_GVE_FFMPEG_DCT_ALGO_MAX_ELEMENTS] =  { 0, 1, 2, 3, 4, 5 };
static gint     gtab_idct_algo[GAP_GVE_FFMPEG_IDCT_ALGO_MAX_ELEMENTS] =  { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
static gint     gtab_mb_decision[GAP_GVE_FFMPEG_MB_DECISION_MAX_ELEMENTS] =  { FF_MB_DECISION_SIMPLE, FF_MB_DECISION_BITS, FF_MB_DECISION_RD };
static gint     gtab_audio_krate[GAP_GVE_FFMPEG_AUDIO_KBIT_RATE_MAX_ELEMENTS] = { 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320 };
static gdouble  gtab_aspect[GAP_GVE_FFMPEG_ASPECT_MAX_ELEMENTS] =  { 0.0, 1.333333333, 1.777777778 };

t_string_optionmenu_elem  *glist_vid_codec = NULL;
t_string_optionmenu_elem  *glist_aud_codec = NULL;
t_string_optionmenu_elem  *glist_fileformat = NULL;



/* --------------------------------
 * p_replace_optionmenu_file_format
 * --------------------------------
 * replace the fileformat optionmenu by dynamic menu
 * (we get the format names from the FFMPEG Lib)
 */
static void
p_replace_optionmenu_file_format(GapGveFFMpegGlobalParams *gpp)
{
  GtkWidget *menu_item;
  GtkWidget *menu;
  gint  l_active_idx;
  gint  l_idx;
  AVOutputFormat *ofmt;
  t_string_optionmenu_elem  *elem_fileformat;


  if(gap_debug) printf("p_replace_optionmenu_file_format: START\n");

  menu = gtk_menu_new ();
  glist_fileformat = NULL;


  l_idx = 0;
  l_active_idx = 0;

  for(ofmt = first_oformat; ofmt != NULL; ofmt = ofmt->next)
  {
     char *menu_name;
     char *object_data;

     object_data = (char*)ofmt->name;

     menu_name = g_strdup_printf("[%s] %s", object_data, ofmt->long_name);

     if(gap_debug)
     {
        printf("p_replace_optionmenu_file_format: val[%d]: %s\n", (int)l_idx, menu_name);

        printf("MIME_TYPE: %s\n", ofmt->mime_type);
        printf("EXTENSIONS: %s\n", ofmt->extensions);
        printf("Default VID_CODEC ID: %d\n", ofmt->video_codec);
        printf("Default AUD_CODEC ID: %d\n", ofmt->audio_codec);
        printf("flags: %d\n", (int)ofmt->flags );
     }

     /* Filter filefromats (not interested in audio and single image fileformats, pipe ...)
      * - mime_type must be != NULL and should start with "video"
      * - format must support video codec  (codec_id != 0)
      * - formats that support both video_codec and audio_code are accepted
      *   (even if the mime type is not "video")
      */
     if((ofmt->mime_type != NULL) && (ofmt->video_codec != 0))
     {
       if((strncmp(ofmt->mime_type, "video", 5) == 0) || (ofmt->audio_codec != 0))
       {
         menu_item = gtk_menu_item_new_with_label(menu_name);
         elem_fileformat = (t_string_optionmenu_elem *) g_malloc0(sizeof(t_string_optionmenu_elem));
         elem_fileformat->menu_idx = l_idx;
         elem_fileformat->name     = g_strdup(object_data);
         elem_fileformat->next     = glist_fileformat;
         glist_fileformat = elem_fileformat;

         gtk_widget_show (menu_item);
         gtk_menu_append (GTK_MENU (menu), menu_item);

         g_signal_connect (G_OBJECT (menu_item), "activate",
                           G_CALLBACK(on_ff_fileformat_optionmenu),
                          (gpointer)gpp);
         g_object_set_data (G_OBJECT (menu_item)
	                    , GAP_ENC_FFGUI_ENC_MENU_ITEM_ENC_PTR
			    , (gpointer)object_data);

         if(strcmp(object_data, gpp->evl.format_name) == 0)
         {
             gtk_menu_item_activate(GTK_MENU_ITEM (menu_item));
             l_active_idx = l_idx;
         }
         l_idx++;
       }
     }
  }

  gtk_option_menu_set_menu (GTK_OPTION_MENU (gpp->ff_fileformat_optionmenu)
                           , menu);
  gtk_option_menu_set_history (GTK_OPTION_MENU (gpp->ff_fileformat_optionmenu)
                           , l_active_idx);
}  /* end p_replace_optionmenu_file_format */


/* --------------------------------
 * p_replace_optionmenu_vid_codec
 * --------------------------------
 * replace the vid_codec optionmenu by dynamic menu
 * (we get the video codecs from the FFMPEG Lib)
 */
static void
p_replace_optionmenu_vid_codec(GapGveFFMpegGlobalParams *gpp)
{
  GtkWidget *menu_item;
  GtkWidget *menu;
  gint  l_active_idx;
  gint  l_idx;
  AVCodec *avcodec;
  t_string_optionmenu_elem  *elem_vid_codec;


  if(gap_debug) printf("p_replace_optionmenu_vid_codec: START\n");

  menu = gtk_menu_new ();

  glist_vid_codec = NULL;

  l_idx = 0;
  l_active_idx = 0;

  for(avcodec = first_avcodec; avcodec != NULL; avcodec = avcodec->next)
  {
     char *menu_name;
     char *object_data;
     if ((avcodec->encode) && (avcodec->type == CODEC_TYPE_VIDEO))
     {
       object_data = (char *)avcodec->name;

       menu_name = g_strdup(object_data);
       menu_item = gtk_menu_item_new_with_label(menu_name);

       if(gap_debug)
       {
          printf("p_replace_optionmenu_vid_codec: val[%d]: %s\n", (int)l_idx, menu_name);
       }
       elem_vid_codec = (t_string_optionmenu_elem *) g_malloc0(sizeof(t_string_optionmenu_elem));
       elem_vid_codec->menu_idx = l_idx;
       elem_vid_codec->name     = g_strdup(object_data);
       elem_vid_codec->next     = glist_vid_codec;
       glist_vid_codec = elem_vid_codec;

       gtk_widget_show (menu_item);
       gtk_menu_append (GTK_MENU (menu), menu_item);

       g_signal_connect (G_OBJECT (menu_item), "activate",
                             G_CALLBACK(on_ff_vid_codec_optionmenu),
                                     (gpointer)gpp);
       g_object_set_data (G_OBJECT (menu_item)
                          , GAP_ENC_FFGUI_ENC_MENU_ITEM_ENC_PTR
			  , (gpointer)object_data);

       if(strcmp(object_data, gpp->evl.vcodec_name) == 0)
       {
           gtk_menu_item_activate(GTK_MENU_ITEM (menu_item));
           l_active_idx = l_idx;
       }
       l_idx++;
     }
  }

  gtk_option_menu_set_menu (GTK_OPTION_MENU (gpp->ff_vid_codec_optionmenu)
                           , menu);
  gtk_option_menu_set_history (GTK_OPTION_MENU (gpp->ff_vid_codec_optionmenu)
                           , l_active_idx);
}  /* end p_replace_optionmenu_vid_codec */

/* --------------------------------
 * p_replace_optionmenu_aud_codec
 * --------------------------------
 * replace the aud_codec optionmenu by dynamic menu
 * (we get the audio codecs from the FFMPEG Lib)
 */
static void
p_replace_optionmenu_aud_codec(GapGveFFMpegGlobalParams *gpp)
{
  GtkWidget *menu_item;
  GtkWidget *menu;
  gint  l_active_idx;
  gint  l_idx;
  AVCodec *avcodec;
  t_string_optionmenu_elem  *elem_aud_codec;


  if(gap_debug) printf("p_replace_optionmenu_aud_codec: START\n");

  menu = gtk_menu_new ();

  glist_aud_codec = NULL;

  l_idx = 0;
  l_active_idx = 0;

  for(avcodec = first_avcodec; avcodec != NULL; avcodec = avcodec->next)
  {
     char *menu_name;
     char *object_data;
     if ((avcodec->encode) && (avcodec->type == CODEC_TYPE_AUDIO))
     {
       object_data = (char *)avcodec->name;

       menu_name = g_strdup(object_data);
       menu_item = gtk_menu_item_new_with_label(menu_name);

       if(gap_debug)
       {
          printf("p_replace_optionmenu_aud_codec: val[%d]: %s\n", (int)l_idx, menu_name);
       }
       elem_aud_codec = (t_string_optionmenu_elem *) g_malloc0(sizeof(t_string_optionmenu_elem));
       elem_aud_codec->menu_idx = l_idx;
       elem_aud_codec->name     = g_strdup(object_data);
       elem_aud_codec->next     = glist_aud_codec;
       glist_aud_codec = elem_aud_codec;

       gtk_widget_show (menu_item);
       gtk_menu_append (GTK_MENU (menu), menu_item);

       g_signal_connect (G_OBJECT (menu_item), "activate",
                             G_CALLBACK(on_ff_aud_codec_optionmenu),
                                     (gpointer)gpp);
       g_object_set_data (G_OBJECT (menu_item)
                          , GAP_ENC_FFGUI_ENC_MENU_ITEM_ENC_PTR
			  , (gpointer)object_data);

       if(strcmp(object_data, gpp->evl.acodec_name) == 0)
       {
           gtk_menu_item_activate(GTK_MENU_ITEM (menu_item));
           l_active_idx = l_idx;
       }
       l_idx++;
     }
  }

  gtk_option_menu_set_menu (GTK_OPTION_MENU (gpp->ff_aud_codec_optionmenu)
                           , menu);
  gtk_option_menu_set_history (GTK_OPTION_MENU (gpp->ff_aud_codec_optionmenu)
                           , l_active_idx);
}  /* end p_replace_optionmenu_aud_codec */



/* ----------------------------
 * gap_enc_ffgui_set_default_codecs
 * ----------------------------
 * - findout the default CODECS and Extensions for the current selected fileformat
 *   and show this informations in the info label widget.
 * - opütional (if set_codec_menus == TRUE)
 *   set both VIDEO_CODEC and AUDIO_CODEC optionmenu to these default CODECS
 */
void
gap_enc_ffgui_set_default_codecs(GapGveFFMpegGlobalParams *gpp, gboolean set_codec_menus)
{
   AVOutputFormat *ofmt;
   AVCodec *aud_codec;
   AVCodec *vid_codec;
   char *name;
   char *l_ext_one;
   guint l_ii;

   if(gpp == NULL) return;
   if(gpp->shell_window == NULL) return;

   ofmt = guess_format(gpp->evl.format_name, NULL, NULL);

   if(ofmt)
   {
     char *info_msg;
     char *default_vid_codec;
     char *default_aud_codec;

     /* find default CODECS for the fileformat */
     vid_codec = avcodec_find_encoder(ofmt->video_codec);
     aud_codec = avcodec_find_encoder(ofmt->audio_codec);

     if(vid_codec)
     {
       default_vid_codec = g_strdup(vid_codec->name);
       if(set_codec_menus)
       {
         /* set optionmenu */
         name = p_init_optionmenu_actual_nameidx(gpp, gpp->ff_vid_codec_optionmenu, glist_vid_codec, default_vid_codec);

         if (name)
         {
            g_snprintf(gpp->evl.vcodec_name, sizeof(gpp->evl.vcodec_name), "%s", name);
         }
       }
     }
     else
     {
       default_vid_codec = g_strdup(_("NOT SUPPORTED"));
     }
     if(aud_codec)
     {
       default_aud_codec = g_strdup(aud_codec->name);
       if(set_codec_menus)
       {
         /* set optionmenu */
         name = p_init_optionmenu_actual_nameidx(gpp, gpp->ff_aud_codec_optionmenu, glist_aud_codec, default_aud_codec);
         if (name)
         {
            g_snprintf(gpp->evl.acodec_name, sizeof(gpp->evl.acodec_name), "%s", name);
         }
       }
     }
     else
     {
       default_aud_codec = g_strdup(_("NOT SUPPORTED"));
     }

     info_msg = g_strdup_printf(_("Selected Fileformat : [%s] %s\n"
                                  "Recommanded Video CODEC : %s\n"
                                  "Recommanded Audio CODEC : %s\n"
                                  "Extension(s): %s"
                                 )
                               , ofmt->name
                               , ofmt->long_name
                               , default_vid_codec
                               , default_aud_codec
                               , ofmt->extensions
                               );
     /* store the current video extension
      * ofmt->extensions may contain more than 1 extension (comma seperated)
      * but we always use only the 1.st one.
      */
     if(ofmt->extensions)
     {
       l_ext_one = g_strdup_printf(".%s", ofmt->extensions);
     }
     else
     {
       /* mpeg1 system format (VCD) has no extensions (is that a bug in the lib ?)
        * assume .mpg for videoformats without extensions as workaround
        */
       l_ext_one = g_strdup(".mpg");
     }

     for(l_ii=0; l_ii < sizeof(gpp->evl.current_vid_extension)-2; l_ii++)
     {
          if ((l_ext_one[l_ii] == '\0')
          ||  (l_ext_one[l_ii] == ';')
          ||  (l_ext_one[l_ii] == ',')
          ||  (l_ext_one[l_ii] == ':'))
          {
            gpp->evl.current_vid_extension[l_ii] = '\0';
            break;
          }
          gpp->evl.current_vid_extension[l_ii] = l_ext_one[l_ii];
     }
     g_free(l_ext_one);

     /* update info label with default codec names and extensions
      * for the selected fileformat
      */
     gtk_label_set_text(GTK_LABEL(gpp->ff_basic_info_label), info_msg);

     g_free(info_msg);
     g_free(default_vid_codec);
     g_free(default_aud_codec);
   }

}  /* end gap_enc_ffgui_set_default_codecs */



/* --------------------------------
 * p_init_optionmenu_actual_nameidx
 * --------------------------------
 */
static char *
p_init_optionmenu_actual_nameidx(GapGveFFMpegGlobalParams *gpp, GtkWidget *wgt, t_string_optionmenu_elem *list, char *name)
{
  t_string_optionmenu_elem  *elem;


  if(gap_debug) printf("p_init_optionmenu_actual_nameidx START search: %s\n", name);
  for(elem = list; elem != NULL; elem = (t_string_optionmenu_elem  *)elem->next)
  {
    if(strcmp(name, elem->name) == 0)
    {
      gtk_option_menu_set_history (GTK_OPTION_MENU (wgt), elem->menu_idx);
      if(gap_debug) printf("p_init_optionmenu_actual_nameidx START found: %s\n", name);
      return(elem->name);
    }
  }
  return (NULL);
}  /* end p_init_optionmenu_actual_nameidx */

/* ---------------------------------
 * p_init_gint_optionmenu_actual_idx
 * ---------------------------------
 */
static void
p_init_gint_optionmenu_actual_idx(GapGveFFMpegGlobalParams *gpp, GtkWidget *wgt, gint *gtab_ptr, gint val, gint maxidx)
{
  gint l_idx;

  for(l_idx = 0; l_idx < maxidx; l_idx++)
  {
    if(val == gtab_ptr[l_idx])
    {
      gtk_option_menu_set_history (GTK_OPTION_MENU (wgt), l_idx);
      return;
    }
  }
}  /* end p_init_gint_optionmenu_actual_idx */

/* ------------------------------------
 * p_init_gdouble_optionmenu_actual_idx
 * ------------------------------------
 */
static void
p_init_gdouble_optionmenu_actual_idx(GapGveFFMpegGlobalParams *gpp, GtkWidget *wgt, gdouble *gtab_ptr, gdouble val, gint maxidx)
{
  gint l_idx;

  for(l_idx = 0; l_idx < maxidx; l_idx++)
  {
    if(val == gtab_ptr[l_idx])
    {
      gtk_option_menu_set_history (GTK_OPTION_MENU (wgt), l_idx);
      return;
    }
  }
}  /* end p_init_gdouble_optionmenu_actual_idx */

/* --------------------------------
 * p_init_optionmenu_vals
 * --------------------------------
 */
static void
p_init_optionmenu_vals(GapGveFFMpegGlobalParams *gpp)
{
  char *name;

  p_init_gint_optionmenu_actual_idx(gpp, gpp->ff_motion_estimation_optionmenu
                              , &gtab_motion_est[0]
                              , gpp->evl.motion_estimation
                              , GAP_GVE_FFMPEG_MOTION_ESTIMATION_MAX_ELEMENTS
                              );
  p_init_gint_optionmenu_actual_idx(gpp, gpp->ff_dct_algo_optionmenu
                              , &gtab_dct_algo[0]
                              , gpp->evl.dct_algo
                              , GAP_GVE_FFMPEG_DCT_ALGO_MAX_ELEMENTS
                              );
  p_init_gint_optionmenu_actual_idx(gpp, gpp->ff_idct_algo_optionmenu
                              , &gtab_idct_algo[0]
                              , gpp->evl.idct_algo
                              , GAP_GVE_FFMPEG_IDCT_ALGO_MAX_ELEMENTS
                              );
  p_init_gint_optionmenu_actual_idx(gpp, gpp->ff_mb_decision_optionmenu
                              , &gtab_mb_decision[0]
                              , gpp->evl.mb_decision
                              , GAP_GVE_FFMPEG_MB_DECISION_MAX_ELEMENTS
                              );

  p_init_gint_optionmenu_actual_idx(gpp, gpp->ff_aud_bitrate_optionmenu
                              , &gtab_audio_krate[0]
                              , gpp->evl.audio_bitrate
                              , GAP_GVE_FFMPEG_AUDIO_KBIT_RATE_MAX_ELEMENTS
                              );


  p_init_gdouble_optionmenu_actual_idx(gpp, gpp->ff_aspect_optionmenu
                              , &gtab_aspect[0]
                              , gpp->evl.factor_aspect_ratio
                              , GAP_GVE_FFMPEG_ASPECT_MAX_ELEMENTS
                              );

  name = p_init_optionmenu_actual_nameidx(gpp, gpp->ff_fileformat_optionmenu, glist_fileformat, gpp->evl.format_name);
  name = p_init_optionmenu_actual_nameidx(gpp, gpp->ff_vid_codec_optionmenu,  glist_vid_codec,  gpp->evl.vcodec_name);
  name = p_init_optionmenu_actual_nameidx(gpp, gpp->ff_aud_codec_optionmenu,  glist_aud_codec,  gpp->evl.acodec_name);

}  /* end p_init_optionmenu_vals */


/* --------------------------------
 * p_set_option_menu_callbacks
 * --------------------------------
 */
static void
p_set_option_menu_callbacks(GapGveFFMpegGlobalParams *gpp)
{
  /* dynamic optionmenus
   * (using Index GAP_ENC_FFGUI_ENC_MENU_ITEM_ENC_PTR as attached object data)
   *
   * the entries for available fileformats and codecs
   * are set up by queries to the FFMPEG avcodec library
   */

  av_register_all();  /* register all fileformats and codecs before we can use the lib */

  p_replace_optionmenu_file_format(gpp);
  p_replace_optionmenu_vid_codec(gpp);
  p_replace_optionmenu_aud_codec(gpp);

}  /* end p_set_option_menu_callbacks */



/* --------------------------------
 * p_init_spinbuttons
 * --------------------------------
 */
static void
p_init_spinbuttons(GapGveFFMpegGlobalParams *gpp)
{
  GtkAdjustment *adj;

  if(gap_debug) printf("p_init_spinbuttons\n");

  adj = GTK_ADJUSTMENT(gpp->ff_aud_bitrate_spinbutton_adj);
  gtk_adjustment_set_value(adj, (gfloat)gpp->evl.audio_bitrate);

  adj = GTK_ADJUSTMENT(gpp->ff_vid_bitrate_spinbutton_adj);
  gtk_adjustment_set_value(adj, (gfloat)gpp->evl.video_bitrate);

  adj = GTK_ADJUSTMENT(gpp->ff_qscale_spinbutton_adj);
  gtk_adjustment_set_value(adj, (gfloat)gpp->evl.qscale);

  adj = GTK_ADJUSTMENT(gpp->ff_qmin_spinbutton_adj);
  gtk_adjustment_set_value(adj, (gfloat)gpp->evl.qmin);

  adj = GTK_ADJUSTMENT(gpp->ff_qmax_spinbutton_adj);
  gtk_adjustment_set_value(adj, (gfloat)gpp->evl.qmax);

  adj = GTK_ADJUSTMENT(gpp->ff_qdiff_spinbutton_adj);
  gtk_adjustment_set_value(adj, (gfloat)gpp->evl.qdiff);


  adj = GTK_ADJUSTMENT(gpp->ff_gop_size_spinbutton_adj);
  gtk_adjustment_set_value(adj, (gfloat)gpp->evl.gop_size);



  adj = GTK_ADJUSTMENT(gpp->ff_qblur_spinbutton_adj);
  gtk_adjustment_set_value(adj, (gfloat)gpp->evl.qblur);

  adj = GTK_ADJUSTMENT(gpp->ff_qcomp_spinbutton_adj);
  gtk_adjustment_set_value(adj, (gfloat)gpp->evl.qcomp);

  adj = GTK_ADJUSTMENT(gpp->ff_rc_init_cplx_spinbutton_adj);
  gtk_adjustment_set_value(adj, (gfloat)gpp->evl.rc_init_cplx);

  adj = GTK_ADJUSTMENT(gpp->ff_b_qfactor_spinbutton_adj);
  gtk_adjustment_set_value(adj, (gfloat)gpp->evl.b_qfactor);

  adj = GTK_ADJUSTMENT(gpp->ff_i_qfactor_spinbutton_adj);
  gtk_adjustment_set_value(adj, (gfloat)gpp->evl.i_qfactor);

  adj = GTK_ADJUSTMENT(gpp->ff_b_qoffset_spinbutton_adj);
  gtk_adjustment_set_value(adj, (gfloat)gpp->evl.b_qoffset);

  adj = GTK_ADJUSTMENT(gpp->ff_i_qoffset_spinbutton_adj);
  gtk_adjustment_set_value(adj, (gfloat)gpp->evl.i_qoffset);

  adj = GTK_ADJUSTMENT(gpp->ff_bitrate_tol_spinbutton_adj);
  gtk_adjustment_set_value(adj, (gfloat)gpp->evl.bitrate_tol);

  adj = GTK_ADJUSTMENT(gpp->ff_maxrate_tol_spinbutton_adj);
  gtk_adjustment_set_value(adj, (gfloat)gpp->evl.maxrate_tol);

  adj = GTK_ADJUSTMENT(gpp->ff_minrate_tol_spinbutton_adj);
  gtk_adjustment_set_value(adj, (gfloat)gpp->evl.minrate_tol);

  adj = GTK_ADJUSTMENT(gpp->ff_bufsize_spinbutton_adj);
  gtk_adjustment_set_value(adj, (gfloat)gpp->evl.bufsize);

  adj = GTK_ADJUSTMENT(gpp->ff_strict_spinbutton_adj);
  gtk_adjustment_set_value(adj, (gfloat)gpp->evl.strict);

  adj = GTK_ADJUSTMENT(gpp->ff_mb_qmin_spinbutton_adj);
  gtk_adjustment_set_value(adj, (gfloat)gpp->evl.mb_qmin);

  adj = GTK_ADJUSTMENT(gpp->ff_mb_qmax_spinbutton_adj);
  gtk_adjustment_set_value(adj, (gfloat)gpp->evl.mb_qmax);

}  /* end p_init_spinbuttons */


/* --------------------------------
 * p_init_vid_checkbuttons
 * --------------------------------
 */
static void
p_init_vid_checkbuttons(GapGveFFMpegGlobalParams *gpp)
{
  gint32 flag;

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gpp->ff_intra_checkbutton)
                               , gpp->evl.intra);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gpp->ff_bitexact_checkbutton)
                               , gpp->evl.bitexact);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gpp->ff_aspect_checkbutton)
                               , gpp->evl.set_aspect_ratio);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gpp->ff_aic_checkbutton)
                               , gpp->evl.aic);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gpp->ff_umv_checkbutton)
                               , gpp->evl.umv);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gpp->ff_b_frames_checkbutton)
                               , gpp->evl.b_frames);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gpp->ff_mv4_checkbutton)
                               , gpp->evl.mv4);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gpp->ff_partitioning_checkbutton)
                               , gpp->evl.partitioning);

  flag = (gpp->evl.pass == 2) ? TRUE : FALSE;
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gpp->ff_pass_checkbutton), flag);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gpp->ff_dont_recode_checkbutton)
                               , gpp->evl.dont_recode_flag);
}   /* end p_init_vid_checkbuttons */

/* --------------------------------
 * p_init_entry_widgets
 * --------------------------------
 */
static void
p_init_entry_widgets(GapGveFFMpegGlobalParams *gpp)
{
  if(gap_debug) printf("p_init_entry_widgets\n");

  gtk_entry_set_text(GTK_ENTRY(gpp->ff_passlogfile_entry), gpp->evl.passlogfile);
  gtk_entry_set_text(GTK_ENTRY(gpp->ff_title_entry), gpp->evl.title);
  gtk_entry_set_text(GTK_ENTRY(gpp->ff_author_entry), gpp->evl.author);
  gtk_entry_set_text(GTK_ENTRY(gpp->ff_copyright_entry), gpp->evl.copyright);
  gtk_entry_set_text(GTK_ENTRY(gpp->ff_comment_entry), gpp->evl.comment);

}  /* end p_init_entry_widgets */

/* --------------------------------
 * gap_enc_ffgui_init_main_dialog_widgets
 * --------------------------------
 */
void
gap_enc_ffgui_init_main_dialog_widgets(GapGveFFMpegGlobalParams *gpp)
{
  if(gap_debug) printf("gap_enc_ffgui_init_main_dialog_widgets: Start INIT\n");

  /* put initial values to the widgets */

  p_init_spinbuttons(gpp);
  p_init_entry_widgets(gpp);
  p_init_vid_checkbuttons(gpp);
  p_init_optionmenu_vals(gpp);

  gap_enc_ffgui_set_default_codecs(gpp, FALSE);               /* update info labels but DONT set CODEC optionmenus */
}  /* end gap_enc_ffgui_init_main_dialog_widgets */


/* --------------------------------
 * gap_enc_ffgui_gettab_motion_est
 * --------------------------------
 */
gint
gap_enc_ffgui_gettab_motion_est(gint idx)
{
 if((idx >= GAP_GVE_FFMPEG_MOTION_ESTIMATION_MAX_ELEMENTS) || (idx < 1))
 {
    idx = 0;
 }

 return(gtab_motion_est[idx]);

}  /* end gap_enc_ffgui_gettab_motion_est */

/* --------------------------------
 * gap_enc_ffgui_gettab_dct_algo
 * --------------------------------
 */
gint
gap_enc_ffgui_gettab_dct_algo(gint idx)
{
 if((idx >= GAP_GVE_FFMPEG_DCT_ALGO_MAX_ELEMENTS) || (idx < 1))
 {
    idx = 0;
 }

 return (gtab_dct_algo[idx]);

}  /* end gap_enc_ffgui_gettab_dct_algo */

/* --------------------------------
 * gap_enc_ffgui_gettab_idct_algo
 * --------------------------------
 */
gint
gap_enc_ffgui_gettab_idct_algo(gint idx)
{
 if((idx >= GAP_GVE_FFMPEG_IDCT_ALGO_MAX_ELEMENTS) || (idx < 1))
 {
    idx = 0;
 }

 return(gtab_idct_algo[idx]);

}  /* end gap_enc_ffgui_gettab_idct_algo */

/* --------------------------------
 * gap_enc_ffgui_gettab_mb_decision
 * --------------------------------
 */
gint
gap_enc_ffgui_gettab_mb_decision(gint idx)
{
 if((idx >= GAP_GVE_FFMPEG_MB_DECISION_MAX_ELEMENTS) || (idx < 1))
 {
    idx = 0;
 }

 return(gtab_mb_decision[idx]);

}  /* end gap_enc_ffgui_gettab_mb_decision */

/* --------------------------------
 * gap_enc_ffgui_gettab_audio_krate
 * --------------------------------
 */
gint
gap_enc_ffgui_gettab_audio_krate(gint idx)
{
 if((idx >= GAP_GVE_FFMPEG_AUDIO_KBIT_RATE_MAX_ELEMENTS) || (idx < 1))
 {
    idx = 0;
 }

 return(gtab_audio_krate[idx]);

}  /* end gap_enc_ffgui_gettab_audio_krate */




/* --------------------------------
 * gap_enc_ffgui_gettab_aspect
 * --------------------------------
 */
gdouble
gap_enc_ffgui_gettab_aspect(gint idx)
{
 if((idx >= GAP_GVE_FFMPEG_ASPECT_MAX_ELEMENTS) || (idx < 1))
 {
    idx = 0;
 }

 return(gtab_aspect[idx]);

}  /* end gap_enc_ffgui_gettab_aspect */




/* --------------------------------
 * gap_enc_ffgui_create_fsb__fileselection
 * --------------------------------
 */
GtkWidget*
gap_enc_ffgui_create_fsb__fileselection (GapGveFFMpegGlobalParams *gpp)
{
  GtkWidget *fsb__fileselection;
  GtkWidget *fsb__ok_button;
  GtkWidget *fsb__cancel_button;

  fsb__fileselection = gtk_file_selection_new (_("Select File"));
  gtk_container_set_border_width (GTK_CONTAINER (fsb__fileselection), 10);

  fsb__ok_button = GTK_FILE_SELECTION (fsb__fileselection)->ok_button;
  gtk_widget_show (fsb__ok_button);
  GTK_WIDGET_SET_FLAGS (fsb__ok_button, GTK_CAN_DEFAULT);

  fsb__cancel_button = GTK_FILE_SELECTION (fsb__fileselection)->cancel_button;
  gtk_widget_show (fsb__cancel_button);
  GTK_WIDGET_SET_FLAGS (fsb__cancel_button, GTK_CAN_DEFAULT);

  g_signal_connect (G_OBJECT (fsb__fileselection), "destroy",
                      G_CALLBACK (on_fsb__fileselection_destroy),
                      gpp);
  g_signal_connect (G_OBJECT (fsb__ok_button), "clicked",
                      G_CALLBACK (on_fsb__ok_button_clicked),
                      gpp);
  g_signal_connect (G_OBJECT (fsb__cancel_button), "clicked",
                      G_CALLBACK (on_fsb__cancel_button_clicked),
                      gpp);

  return fsb__fileselection;
}  /* end gap_enc_ffgui_create_fsb__fileselection */

/* --------------------------------
 * p_create_fsb__fileselection
 * --------------------------------
 */
GtkWidget*
p_create_fsb__fileselection (GapGveFFMpegGlobalParams *gpp)
{
  GtkWidget *shell_window;
  GtkWidget *dialog_vbox2;
  GtkWidget *notebook1;
  GtkWidget *frame_preset;
  GtkWidget *frame1;
  GtkWidget *frame2;
  GtkWidget *frame3;
  GtkWidget *frame4;
  GtkWidget *frame5;
  GtkWidget *frame6;
  GtkWidget *frame7;

  GtkWidget *table_preset;
  GtkWidget *table1;
  GtkWidget *table2;
  GtkWidget *table3;
  GtkWidget *table4;
  GtkWidget *table5;
  GtkWidget *table6;

  GtkWidget *nb1_label1;
  GtkWidget *nb1_label2;
  GtkWidget *nb1_label3;
  GtkWidget *nb1_label4;
  GtkWidget *nb1_label5;
  GtkWidget *ff_qblur_label;
  GtkWidget *label6;
  GtkWidget *label7;
  GtkWidget *label8;
  GtkWidget *label9;
  GtkWidget *label10;
  GtkWidget *label11;
  GtkWidget *label12;
  GtkWidget *label13;
  GtkWidget *label14;
  GtkWidget *label16;
  GtkWidget *label17;
  GtkWidget *label18;
  GtkWidget *label19;
  GtkWidget *label20;
  GtkWidget *label22;
  GtkWidget *label23;
  GtkWidget *label24;
  GtkWidget *label25;
  GtkWidget *label26;
  GtkWidget *label27;
  GtkWidget *label28;
  GtkWidget *label29;
  GtkWidget *label30;
  GtkWidget *label31;
  GtkWidget *label32;
  GtkWidget *label33;
  GtkWidget *label34;
  GtkWidget *label35;
  GtkWidget *label36;
  GtkWidget *label37;
  GtkWidget *label38;
  GtkWidget *label39;

  guint label23_key;


  GtkWidget *ff_fileformat_optionmenu;
  GtkWidget *ff_fileformat_optionmenu_menu;
  GtkWidget *glade_menuitem;
  GtkWidget *ff_vid_codec_optionmenu;
  GtkWidget *ff_vid_codec_optionmenu_menu;
  GtkWidget *ff_aud_codec_optionmenu;
  GtkWidget *ff_aud_codec_optionmenu_menu;
  GtkObject *ff_aud_bitrate_spinbutton_adj;
  GtkWidget *ff_aud_bitrate_spinbutton;
  GtkObject *ff_vid_bitrate_spinbutton_adj;
  GtkWidget *ff_vid_bitrate_spinbutton;
  GtkWidget *ff_presets_optionmenu;
  GtkWidget *ff_presets_optionmenu_menu;
  GtkObject *ff_qscale_spinbutton_adj;
  GtkWidget *ff_qscale_spinbutton;
  GtkObject *ff_qmin_spinbutton_adj;
  GtkWidget *ff_qmin_spinbutton;
  GtkObject *ff_qmax_spinbutton_adj;
  GtkWidget *ff_qmax_spinbutton;
  GtkObject *ff_qdiff_spinbutton_adj;
  GtkWidget *ff_qdiff_spinbutton;
  GtkWidget *ff_basic_info_label;
  GtkWidget *ff_aud_bitrate_optionmenu;
  GtkWidget *ff_aud_bitrate_optionmenu_menu;
  GtkWidget *ff_motion_estimation_optionmenu;
  GtkWidget *ff_motion_estimation_optionmenu_menu;
  GtkWidget *ff_dct_algo_optionmenu;
  GtkWidget *ff_dct_algo_optionmenu_menu;
  GtkWidget *ff_idct_algo_optionmenu;
  GtkWidget *ff_idct_algo_optionmenu_menu;
  GtkObject *ff_gop_size_spinbutton_adj;
  GtkWidget *ff_gop_size_spinbutton;
  GtkWidget *ff_intra_checkbutton;
  GtkWidget *ff_bitexact_checkbutton;
  GtkWidget *ff_aspect_checkbutton;
  GtkWidget *ff_aspect_optionmenu;
  GtkWidget *ff_aspect_optionmenu_menu;
  GtkWidget *ff_aic_checkbutton;
  GtkWidget *ff_umv_checkbutton;
  GtkWidget *ff_mb_decision_optionmenu;
  GtkWidget *ff_mb_decision_optionmenu_menu;
  GtkWidget *ff_b_frames_checkbutton;
  GtkWidget *ff_mv4_checkbutton;
  GtkWidget *ff_partitioning_checkbutton;
  GtkWidget *ff_dont_recode_checkbutton;
  GtkObject *ff_qblur_spinbutton_adj;
  GtkWidget *ff_qblur_spinbutton;
  GtkObject *ff_qcomp_spinbutton_adj;
  GtkWidget *ff_qcomp_spinbutton;
  GtkObject *ff_rc_init_cplx_spinbutton_adj;
  GtkWidget *ff_rc_init_cplx_spinbutton;
  GtkObject *ff_b_qfactor_spinbutton_adj;
  GtkWidget *ff_b_qfactor_spinbutton;
  GtkObject *ff_i_qfactor_spinbutton_adj;
  GtkWidget *ff_i_qfactor_spinbutton;
  GtkObject *ff_b_qoffset_spinbutton_adj;
  GtkWidget *ff_b_qoffset_spinbutton;
  GtkObject *ff_i_qoffset_spinbutton_adj;
  GtkWidget *ff_i_qoffset_spinbutton;
  GtkObject *ff_bitrate_tol_spinbutton_adj;
  GtkWidget *ff_bitrate_tol_spinbutton;
  GtkObject *ff_maxrate_tol_spinbutton_adj;
  GtkWidget *ff_maxrate_tol_spinbutton;
  GtkObject *ff_minrate_tol_spinbutton_adj;
  GtkWidget *ff_minrate_tol_spinbutton;
  GtkObject *ff_bufsize_spinbutton_adj;
  GtkWidget *ff_bufsize_spinbutton;
  GtkObject *ff_strict_spinbutton_adj;
  GtkWidget *ff_strict_spinbutton;
  GtkObject *ff_mb_qmin_spinbutton_adj;
  GtkWidget *ff_mb_qmin_spinbutton;
  GtkObject *ff_mb_qmax_spinbutton_adj;
  GtkWidget *ff_mb_qmax_spinbutton;
  GtkWidget *ff_passlogfile_entry;
  GtkWidget *ff_passlogfile_filesel_button;
  GtkWidget *ff_pass_checkbutton;
  GtkWidget *ff_2pass_info_label;
  GtkWidget *ff_title_entry;
  GtkWidget *ff_author_entry;
  GtkWidget *ff_copyright_entry;
  GtkWidget *ff_comment_entry;
  GtkWidget *ff_filecomment_label;
  GtkAccelGroup *accel_group;


  accel_group = gtk_accel_group_new ();


  shell_window = gimp_dialog_new (_("FFMPEG Video Encode Parameters"),
                         GAP_PLUGIN_NAME_FFMPEG_PARAMS,
                         NULL, 0,
                         gimp_standard_help_func, GAP_PLUGIN_NAME_FFMPEG_PARAMS ".html",

                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                         GTK_STOCK_OK,     GTK_RESPONSE_OK,
                         NULL);

  g_signal_connect (G_OBJECT (shell_window), "response",
                    G_CALLBACK (on_ff_response),
                    gpp);


  dialog_vbox2 = GTK_DIALOG (shell_window)->vbox;
  gtk_widget_show (dialog_vbox2);



  /* ================== */
  /* the preset frame   */
  /* ================== */
  frame_preset = gtk_frame_new (_("Parameter Presets"));
  gtk_widget_show (frame_preset);

  gtk_box_pack_start (GTK_BOX (dialog_vbox2), frame_preset, TRUE, TRUE, 4);
 
  /* the preset table */
  table_preset = gtk_table_new (2, 1, FALSE);
  gtk_widget_show (table_preset);
  gtk_container_add (GTK_CONTAINER (frame_preset), table_preset);
  gtk_container_set_border_width (GTK_CONTAINER (table_preset), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table_preset), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table_preset), 4);


  /* the presets optionmenu */
  ff_presets_optionmenu = gtk_option_menu_new ();
  gtk_widget_show (ff_presets_optionmenu);
  gtk_table_attach (GTK_TABLE (table_preset), ff_presets_optionmenu, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (ff_presets_optionmenu, _("Predefined encoder parameter settings"), NULL);
  ff_presets_optionmenu_menu = gtk_menu_new ();
  glade_menuitem = gtk_menu_item_new_with_label (_("OOPS do not change Params"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_presets_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_PRESET_00_NONE);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_presets_optionmenu_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("DivX VBR Default"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_presets_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_PRESET_01_DIVX_DEFAULT);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_presets_optionmenu_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("DivX VBR, Best Quality"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_presets_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_PRESET_02_DIVX_BEST);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_presets_optionmenu_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("DivX VBR, Low Quality"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_presets_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_PRESET_03_DIVX_LOW);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_presets_optionmenu_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("MPEG1 fixed Rate (VCD)"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_presets_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_PRESET_04_MPEG1_VCD);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_presets_optionmenu_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("MPEG1 VBR, Best Quality"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_presets_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_PRESET_05_MPEG1_BEST);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_presets_optionmenu_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("MPEG2 VBR (DVD)"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_presets_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_PRESET_06_MPEG2_VBR);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_presets_optionmenu_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("Real Video"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_presets_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_PRESET_07_REAL);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_presets_optionmenu_menu), glade_menuitem);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (ff_presets_optionmenu), ff_presets_optionmenu_menu);




  /* the info label */
  ff_basic_info_label = gtk_label_new ("this text is replaced by the current"
                                       " preset/parameter description at runtime");
  gtk_widget_show (ff_basic_info_label);
  gtk_table_attach (GTK_TABLE (table_preset), ff_basic_info_label, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  gtk_label_set_justify (GTK_LABEL (ff_basic_info_label), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (ff_basic_info_label), 0, 0.5);


  /* ================== */
  /* the parmeter frame */
  /* ================== */
  frame1 = gtk_frame_new (_("Parameter Values"));
  gtk_widget_show (frame1);
  gtk_box_pack_start (GTK_BOX (dialog_vbox2), frame1, TRUE, TRUE, 4);

  /* the notebook for detailed parameter settings */
  notebook1 = gtk_notebook_new ();
  gtk_widget_show (notebook1);
  gtk_container_add (GTK_CONTAINER (frame1), notebook1);
  gtk_container_set_border_width (GTK_CONTAINER (notebook1), 4);

  frame3 = gtk_frame_new (_("FFMpeg Basic Encoder Options"));
  gtk_widget_show (frame3);
  gtk_container_add (GTK_CONTAINER (notebook1), frame3);
  gtk_container_set_border_width (GTK_CONTAINER (frame3), 4);

  /* the notebook page label for basic options */
  nb1_label1 = gtk_label_new (_("Basic Options"));
  gtk_widget_show (nb1_label1);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook1), 0), nb1_label1);


  table1 = gtk_table_new (11, 3, FALSE);
  gtk_widget_show (table1);
  gtk_container_add (GTK_CONTAINER (frame3), table1);
  gtk_container_set_border_width (GTK_CONTAINER (table1), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table1), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table1), 4);

  label6 = gtk_label_new (_("Fileformat:           "));
  gtk_widget_show (label6);
  gtk_table_attach (GTK_TABLE (table1), label6, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label6), 0, 0.5);

  label7 = gtk_label_new (_("Video CODEC:"));
  gtk_widget_show (label7);
  gtk_table_attach (GTK_TABLE (table1), label7, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label7), 0, 0.5);

  label8 = gtk_label_new (_("Audio CODEC:"));
  gtk_widget_show (label8);
  gtk_table_attach (GTK_TABLE (table1), label8, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label8), 0, 0.5);

  /* the fileformat  optionmenu */
  ff_fileformat_optionmenu = gtk_option_menu_new ();
  gtk_widget_show (ff_fileformat_optionmenu);
  gtk_table_attach (GTK_TABLE (table1), ff_fileformat_optionmenu, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (ff_fileformat_optionmenu, _("The output multimedia fileformat"), NULL);

  /* ff_fileformat_optionmenu_menu is replaced later in procedure p_replace_optionmenu_file_format */
  ff_fileformat_optionmenu_menu = gtk_menu_new ();
  glade_menuitem = gtk_menu_item_new_with_label (_("avi"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_fileformat_optionmenu_menu), glade_menuitem);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (ff_fileformat_optionmenu), ff_fileformat_optionmenu_menu);
  gtk_option_menu_set_history (GTK_OPTION_MENU (ff_fileformat_optionmenu), 1);

  /* ff_vid_codec_optionmenu is replaced later in procedure p_replace_optionmenu_file_format */
  ff_vid_codec_optionmenu = gtk_option_menu_new ();
  gtk_widget_show (ff_vid_codec_optionmenu);
  gtk_table_attach (GTK_TABLE (table1), ff_vid_codec_optionmenu, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (ff_vid_codec_optionmenu, _("The video codec"), NULL);
  ff_vid_codec_optionmenu_menu = gtk_menu_new ();
  glade_menuitem = gtk_menu_item_new_with_label (_("mpeg1video"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_vid_codec_optionmenu_menu), glade_menuitem);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (ff_vid_codec_optionmenu), ff_vid_codec_optionmenu_menu);

  /* ff_aud_codec_optionmenu is replaced later in procedure p_replace_optionmenu_file_format */
  ff_aud_codec_optionmenu = gtk_option_menu_new ();
  gtk_widget_show (ff_aud_codec_optionmenu);
  gtk_table_attach (GTK_TABLE (table1), ff_aud_codec_optionmenu, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (ff_aud_codec_optionmenu, _("The audio codec"), NULL);
  ff_aud_codec_optionmenu_menu = gtk_menu_new ();
  glade_menuitem = gtk_menu_item_new_with_label (_("mp2"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_aud_codec_optionmenu_menu), glade_menuitem);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (ff_aud_codec_optionmenu), ff_aud_codec_optionmenu_menu);
  gtk_option_menu_set_history (GTK_OPTION_MENU (ff_aud_codec_optionmenu), 1);

  /* the audio bitrate label */
  label9 = gtk_label_new (_("Audio Bitrate:"));
  gtk_widget_show (label9);
  gtk_table_attach (GTK_TABLE (table1), label9, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label9), 0, 0.5);

  /* the audio bitrate spinbutton */
  ff_aud_bitrate_spinbutton_adj = gtk_adjustment_new (160, 0, 500, 10, 100, 100);
  ff_aud_bitrate_spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (ff_aud_bitrate_spinbutton_adj), 1, 0);
  gtk_widget_show (ff_aud_bitrate_spinbutton);
  gtk_table_attach (GTK_TABLE (table1), ff_aud_bitrate_spinbutton, 1, 2, 3, 4,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (ff_aud_bitrate_spinbutton, _("Audio bitrate in kBit/sec"), NULL);


  /* the audio bitrate spinbutton */
  ff_aud_bitrate_optionmenu = gtk_option_menu_new ();
  gtk_widget_show (ff_aud_bitrate_optionmenu);
  gtk_table_attach (GTK_TABLE (table1), ff_aud_bitrate_optionmenu, 2, 3, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (ff_aud_bitrate_optionmenu, _("common used Audio Bitrates"), NULL);

  /* the audio bitrate optionmenu */
  ff_aud_bitrate_optionmenu_menu = gtk_menu_new ();
  glade_menuitem = gtk_menu_item_new_with_label (_("32"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_aud_bitrate_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_AUDIO_KBIT_RATE_00_32);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_aud_bitrate_optionmenu_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("40"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_aud_bitrate_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_AUDIO_KBIT_RATE_01_40);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_aud_bitrate_optionmenu_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("48"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_aud_bitrate_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_AUDIO_KBIT_RATE_02_48);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_aud_bitrate_optionmenu_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("56"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_aud_bitrate_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_AUDIO_KBIT_RATE_03_56);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_aud_bitrate_optionmenu_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("64"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_aud_bitrate_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_AUDIO_KBIT_RATE_04_64);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_aud_bitrate_optionmenu_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("80"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_aud_bitrate_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_AUDIO_KBIT_RATE_05_80);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_aud_bitrate_optionmenu_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("96"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_aud_bitrate_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_AUDIO_KBIT_RATE_06_96);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_aud_bitrate_optionmenu_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("112"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_aud_bitrate_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_AUDIO_KBIT_RATE_07_112);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_aud_bitrate_optionmenu_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("128"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_aud_bitrate_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_AUDIO_KBIT_RATE_08_128);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_aud_bitrate_optionmenu_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("160"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_aud_bitrate_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_AUDIO_KBIT_RATE_09_160);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_aud_bitrate_optionmenu_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("192"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_aud_bitrate_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_AUDIO_KBIT_RATE_10_192);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_aud_bitrate_optionmenu_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("224"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_aud_bitrate_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_AUDIO_KBIT_RATE_11_224);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_aud_bitrate_optionmenu_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("256"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_aud_bitrate_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_AUDIO_KBIT_RATE_12_256);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_aud_bitrate_optionmenu_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("320"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_aud_bitrate_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_AUDIO_KBIT_RATE_13_320);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_aud_bitrate_optionmenu_menu), glade_menuitem);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (ff_aud_bitrate_optionmenu), ff_aud_bitrate_optionmenu_menu);




  /* the video bitrate label */
  label10 = gtk_label_new (_("Video Bitrate:"));
  gtk_widget_show (label10);
  gtk_table_attach (GTK_TABLE (table1), label10, 0, 1, 4, 5,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label10), 0, 0.5);

  /* the video bitrate spinbutton */
  ff_vid_bitrate_spinbutton_adj = gtk_adjustment_new (1000, 0, 10000, 100, 1000, 1000);
  ff_vid_bitrate_spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (ff_vid_bitrate_spinbutton_adj), 1, 0);
  gtk_widget_show (ff_vid_bitrate_spinbutton);
  gtk_table_attach (GTK_TABLE (table1), ff_vid_bitrate_spinbutton, 1, 2, 4, 5,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (ff_vid_bitrate_spinbutton, _("Video bitrate kBit/sec"), NULL);

  /* the qscale label */
  label11 = gtk_label_new (_("qscale:"));
  gtk_widget_show (label11);
  gtk_table_attach (GTK_TABLE (table1), label11, 0, 1, 5, 6,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label11), 0, 0.5);

  /* the qscale spinbutton */
  ff_qscale_spinbutton_adj = gtk_adjustment_new (0, 0, 31, 1, 10, 10);
  ff_qscale_spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (ff_qscale_spinbutton_adj), 1, 0);
  gtk_widget_show (ff_qscale_spinbutton);
  gtk_table_attach (GTK_TABLE (table1), ff_qscale_spinbutton, 1, 2, 5, 6,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (ff_qscale_spinbutton, _("use fixed video quantiser scale (VBR) (0=const Bitrate)"), NULL);



  /* the qmin label */
  label12 = gtk_label_new (_("qmin:"));
  gtk_widget_show (label12);
  gtk_table_attach (GTK_TABLE (table1), label12, 0, 1, 6, 7,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label12), 0, 0.5);

  /* the qmin spinbutton */
  ff_qmin_spinbutton_adj = gtk_adjustment_new (1, 0, 31, 1, 10, 10);
  ff_qmin_spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (ff_qmin_spinbutton_adj), 1, 0);
  gtk_widget_show (ff_qmin_spinbutton);
  gtk_table_attach (GTK_TABLE (table1), ff_qmin_spinbutton, 1, 2, 6, 7,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (ff_qmin_spinbutton, _("min video quantiser scale (VBR)"), NULL);

  /* the qmax label */
  label13 = gtk_label_new (_("qmax:"));
  gtk_widget_show (label13);
  gtk_table_attach (GTK_TABLE (table1), label13, 0, 1, 7, 8,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label13), 0, 0.5);

  /* the qmax spinbutton */
  ff_qmax_spinbutton_adj = gtk_adjustment_new (1, 0, 31, 1, 10, 10);
  ff_qmax_spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (ff_qmax_spinbutton_adj), 1, 0);
  gtk_widget_show (ff_qmax_spinbutton);
  gtk_table_attach (GTK_TABLE (table1), ff_qmax_spinbutton, 1, 2, 7, 8,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (ff_qmax_spinbutton, _("max video quantiser scale (VBR)"), NULL);


  /* the qdiff label */
  label14 = gtk_label_new (_("qdiff:"));
  gtk_widget_show (label14);
  gtk_table_attach (GTK_TABLE (table1), label14, 0, 1, 8, 9,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label14), 0, 0.5);

  /* the qdiff spinbutton */
  ff_qdiff_spinbutton_adj = gtk_adjustment_new (3, 0, 100, 1, 10, 10);
  ff_qdiff_spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (ff_qdiff_spinbutton_adj), 1, 0);
  gtk_widget_show (ff_qdiff_spinbutton);
  gtk_table_attach (GTK_TABLE (table1), ff_qdiff_spinbutton, 1, 2, 8, 9,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (ff_qdiff_spinbutton, _("max difference between the quantiser scale (VBR)"), NULL);




  frame4 = gtk_frame_new (_("FFMpeg Expert Encoder Algorithms and Flags"));
  gtk_widget_show (frame4);
  gtk_container_add (GTK_CONTAINER (notebook1), frame4);
  gtk_container_set_border_width (GTK_CONTAINER (frame4), 4);

  /* the notebook page label for expert algorithms */
  nb1_label2 = gtk_label_new (_("Expert Algorithms"));
  gtk_widget_show (nb1_label2);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook1), 1), nb1_label2);

  table2 = gtk_table_new (7, 3, FALSE);
  gtk_widget_show (table2);
  gtk_container_add (GTK_CONTAINER (frame4), table2);
  gtk_container_set_border_width (GTK_CONTAINER (table2), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table2), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table2), 4);

  label16 = gtk_label_new (_("Motion Estimation:"));
  gtk_widget_show (label16);
  gtk_table_attach (GTK_TABLE (table2), label16, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label16), 0, 0.5);

  label17 = gtk_label_new (_("DCT algorithm:"));
  gtk_widget_show (label17);
  gtk_table_attach (GTK_TABLE (table2), label17, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label17), 0, 0.5);

  label18 = gtk_label_new (_("IDCT algorithm:"));
  gtk_widget_show (label18);
  gtk_table_attach (GTK_TABLE (table2), label18, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label18), 0, 0.5);

  /* the motion estimation algorithm optionmenu */
  ff_motion_estimation_optionmenu = gtk_option_menu_new ();
  gtk_widget_show (ff_motion_estimation_optionmenu);
  gtk_table_attach (GTK_TABLE (table2), ff_motion_estimation_optionmenu, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (ff_motion_estimation_optionmenu, _("Select algorithm for motion estimation"), NULL);

  ff_motion_estimation_optionmenu_menu = gtk_menu_new ();
  glade_menuitem = gtk_menu_item_new_with_label (_("1 zero (fastest)"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_motion_estimation_optionmenu_menu), glade_menuitem);
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_motion_estimation_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_MOTION_ESTIMATION_00_ZERO);
  glade_menuitem = gtk_menu_item_new_with_label (_("2 full (best)"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_motion_estimation_optionmenu_menu), glade_menuitem);
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_motion_estimation_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_MOTION_ESTIMATION_01_FULL);
  glade_menuitem = gtk_menu_item_new_with_label (_("3 log"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_motion_estimation_optionmenu_menu), glade_menuitem);
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_motion_estimation_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_MOTION_ESTIMATION_02_LOG);
  glade_menuitem = gtk_menu_item_new_with_label (_("4 phods"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_motion_estimation_optionmenu_menu), glade_menuitem);
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_motion_estimation_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_MOTION_ESTIMATION_03_PHODS);
  glade_menuitem = gtk_menu_item_new_with_label (_("5 epzs (recommanded)"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_motion_estimation_optionmenu_menu), glade_menuitem);
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_motion_estimation_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_MOTION_ESTIMATION_04_EPZS);
  glade_menuitem = gtk_menu_item_new_with_label (_("6 x1"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_motion_estimation_optionmenu_menu), glade_menuitem);
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_motion_estimation_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_MOTION_ESTIMATION_05_X1);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (ff_motion_estimation_optionmenu), ff_motion_estimation_optionmenu_menu);
  gtk_option_menu_set_history (GTK_OPTION_MENU (ff_motion_estimation_optionmenu), 4);

  
  /* the DCT algorithm optionmenu */
  ff_dct_algo_optionmenu = gtk_option_menu_new ();
  gtk_widget_show (ff_dct_algo_optionmenu);
  gtk_table_attach (GTK_TABLE (table2), ff_dct_algo_optionmenu, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (ff_dct_algo_optionmenu, _("Select algorithm for DCT"), NULL);


  ff_dct_algo_optionmenu_menu = gtk_menu_new ();
  glade_menuitem = gtk_menu_item_new_with_label (_("0 auto"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_dct_algo_optionmenu_menu), glade_menuitem);
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_dct_algo_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_DCT_ALGO_00_AUTO);
  glade_menuitem = gtk_menu_item_new_with_label (_("1 fast int"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_dct_algo_optionmenu_menu), glade_menuitem);
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_dct_algo_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_DCT_ALGO_01_FASTINT);
  glade_menuitem = gtk_menu_item_new_with_label (_("2 int"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_dct_algo_optionmenu_menu), glade_menuitem);
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_dct_algo_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_DCT_ALGO_02_INT);
  glade_menuitem = gtk_menu_item_new_with_label (_("3 mmx"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_dct_algo_optionmenu_menu), glade_menuitem);
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_dct_algo_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_DCT_ALGO_03_MMX);
  glade_menuitem = gtk_menu_item_new_with_label (_("4 mlib"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_dct_algo_optionmenu_menu), glade_menuitem);
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_dct_algo_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_DCT_ALGO_04_MLIB);
  glade_menuitem = gtk_menu_item_new_with_label (_("5 altivec"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_dct_algo_optionmenu_menu), glade_menuitem);
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_dct_algo_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_DCT_ALGO_05_ALTIVEC);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (ff_dct_algo_optionmenu), ff_dct_algo_optionmenu_menu);

  /* the IDCT algorithm optionmenu */
  ff_idct_algo_optionmenu = gtk_option_menu_new ();
  gtk_widget_show (ff_idct_algo_optionmenu);
  gtk_table_attach (GTK_TABLE (table2), ff_idct_algo_optionmenu, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (ff_idct_algo_optionmenu, _("Select algorithm for IDCT"), NULL);


  ff_idct_algo_optionmenu_menu = gtk_menu_new ();
  glade_menuitem = gtk_menu_item_new_with_label (_("0 auto"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_idct_algo_optionmenu_menu), glade_menuitem);
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_idct_algo_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_IDCT_ALGO_00_AUTO);
  glade_menuitem = gtk_menu_item_new_with_label (_("1 int"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_idct_algo_optionmenu_menu), glade_menuitem);
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_idct_algo_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_IDCT_ALGO_01_INT);
  glade_menuitem = gtk_menu_item_new_with_label (_("2 simple"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_idct_algo_optionmenu_menu), glade_menuitem);
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_idct_algo_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_IDCT_ALGO_02_SIMPLE);
  glade_menuitem = gtk_menu_item_new_with_label (_("3 simple mmx"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_idct_algo_optionmenu_menu), glade_menuitem);
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_idct_algo_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_IDCT_ALGO_03_SIMPLEMMX);
  glade_menuitem = gtk_menu_item_new_with_label (_("4 libmpeg2mmx"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_idct_algo_optionmenu_menu), glade_menuitem);
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_idct_algo_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_IDCT_ALGO_04_LIBMPEG2MMX);
  glade_menuitem = gtk_menu_item_new_with_label (_("5 ps2"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_idct_algo_optionmenu_menu), glade_menuitem);
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_idct_algo_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_IDCT_ALGO_05_PS2);
  glade_menuitem = gtk_menu_item_new_with_label (_("6 mlib"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_idct_algo_optionmenu_menu), glade_menuitem);
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_idct_algo_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_IDCT_ALGO_06_MLIB);
  glade_menuitem = gtk_menu_item_new_with_label (_("7 arm"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_idct_algo_optionmenu_menu), glade_menuitem);
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_idct_algo_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_IDCT_ALGO_07_ARM);
  glade_menuitem = gtk_menu_item_new_with_label (_("8 altivec"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_idct_algo_optionmenu_menu), glade_menuitem);
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_idct_algo_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_IDCT_ALGO_08_ALTIVEC);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (ff_idct_algo_optionmenu), ff_idct_algo_optionmenu_menu);


  /* the MB_DECISION label */
  label19 = gtk_label_new (_("MB Decision:"));
  gtk_widget_show (label19);
  gtk_table_attach (GTK_TABLE (table2), label19, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label19), 0, 0.5);


  /* the MB_DECISION optionmenu */
  ff_mb_decision_optionmenu = gtk_option_menu_new ();
  gtk_widget_show (ff_mb_decision_optionmenu);
  gtk_table_attach (GTK_TABLE (table2), ff_mb_decision_optionmenu, 1, 2, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (ff_mb_decision_optionmenu, _("Select algorithm for macroblock decision"), NULL);


  ff_mb_decision_optionmenu_menu = gtk_menu_new ();
  glade_menuitem = gtk_menu_item_new_with_label (_("simple (use mb_cmp)"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_mb_decision_optionmenu_menu), glade_menuitem);
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_mb_decision_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_MB_DECISION_00_SIMPLE);
  glade_menuitem = gtk_menu_item_new_with_label (_("bits (the one which needs fewest bits)"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_mb_decision_optionmenu_menu), glade_menuitem);
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_mb_decision_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_MB_DECISION_01_BITS);
  glade_menuitem = gtk_menu_item_new_with_label (_("rate distoration"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_mb_decision_optionmenu_menu), glade_menuitem);
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_mb_decision_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_MB_DECISION_02_RD);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (ff_mb_decision_optionmenu), ff_mb_decision_optionmenu_menu);


  /* the GOP label */
  label19 = gtk_label_new (_("GOP:"));
  gtk_widget_show (label19);
  gtk_table_attach (GTK_TABLE (table2), label19, 0, 1, 4, 5,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label19), 0, 0.5);

  /* the GOP spinbutton */
  ff_gop_size_spinbutton_adj = gtk_adjustment_new (12, 0, 100, 1, 10, 10);
  ff_gop_size_spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (ff_gop_size_spinbutton_adj), 1, 0);
  gtk_widget_show (ff_gop_size_spinbutton);
  gtk_table_attach (GTK_TABLE (table2), ff_gop_size_spinbutton, 1, 2, 4, 5,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (ff_gop_size_spinbutton, _("Group of picture size"), NULL);


  /* Flags label */
  label20 = gtk_label_new (_("Flags:"));
  gtk_widget_show (label20);
  gtk_table_attach (GTK_TABLE (table2), label20, 0, 1, 5, 6,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label20), 0, 0.5);

  frame2 = gtk_frame_new (NULL);
  gtk_widget_show (frame2);
  gtk_table_attach (GTK_TABLE (table2), frame2, 1, 2, 5, 6,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  {
    int flags_row;

  table3 = gtk_table_new (8, 2, FALSE);
  gtk_widget_show (table3);
  gtk_container_add (GTK_CONTAINER (frame2), table3);
  gtk_container_set_border_width (GTK_CONTAINER (table3), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table3), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table3), 4);

  flags_row = 0;

  ff_intra_checkbutton = gtk_check_button_new_with_label (_("Intra Only"));
  gtk_widget_show (ff_intra_checkbutton);
  gtk_table_attach (GTK_TABLE (table3), ff_intra_checkbutton, 0, 1, flags_row, flags_row+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (ff_intra_checkbutton, _("use only intra frames (I)"), NULL);

  flags_row++;

  ff_aic_checkbutton = gtk_check_button_new_with_label (_("Advanced intra coding"));
  gtk_widget_show (ff_aic_checkbutton);
  gtk_table_attach (GTK_TABLE (table3), ff_aic_checkbutton, 0, 1, flags_row, flags_row+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (ff_aic_checkbutton, _("activate intra frame coding (only h263+ CODEC)"), NULL);



  flags_row++;

  ff_umv_checkbutton = gtk_check_button_new_with_label (_("Unlimited motion vector"));
  gtk_widget_show (ff_umv_checkbutton);
  gtk_table_attach (GTK_TABLE (table3), ff_umv_checkbutton, 0, 1, flags_row, flags_row+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (ff_umv_checkbutton, _("enable unlimited motion vector (only h263+ CODEC)"), NULL);


  flags_row++;

  ff_b_frames_checkbutton = gtk_check_button_new_with_label (_("B frames"));
  gtk_widget_show (ff_b_frames_checkbutton);
  gtk_table_attach (GTK_TABLE (table3), ff_b_frames_checkbutton, 0, 1, flags_row, flags_row+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (ff_b_frames_checkbutton, _("use B frames (only MPEG-4 CODEC)"), NULL);


  flags_row++;

  ff_mv4_checkbutton = gtk_check_button_new_with_label (_("4 Motion Vectors"));
  gtk_widget_show (ff_mv4_checkbutton);
  gtk_table_attach (GTK_TABLE (table3), ff_mv4_checkbutton, 0, 1, flags_row, flags_row+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (ff_mv4_checkbutton, _("use four motion vector by macroblock (only MPEG-4 CODEC)"), NULL);


  flags_row++;

  ff_partitioning_checkbutton = gtk_check_button_new_with_label (_("Partitioning"));
  gtk_widget_show (ff_partitioning_checkbutton);
  gtk_table_attach (GTK_TABLE (table3), ff_partitioning_checkbutton, 0, 1, flags_row, flags_row+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (ff_partitioning_checkbutton, _("use data partitioning (only MPEG-4 CODEC)"), NULL);


  flags_row++;

  ff_bitexact_checkbutton = gtk_check_button_new_with_label (_("Bitexact"));
  gtk_widget_show (ff_bitexact_checkbutton);
  gtk_table_attach (GTK_TABLE (table3), ff_bitexact_checkbutton, 0, 1, flags_row, flags_row+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (ff_bitexact_checkbutton, _("only use bit exact algorithms (for codec testing)"), NULL);


  flags_row++;

  ff_aspect_checkbutton = gtk_check_button_new_with_label (_("Set Aspectratio"));
  gtk_widget_show (ff_aspect_checkbutton);
  gtk_table_attach (GTK_TABLE (table3), ff_aspect_checkbutton, 0, 1, flags_row, flags_row+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (ff_aspect_checkbutton, _("store aspectratio information (width/height) in the output video"), NULL);
   
  /* the ASPECT optionmenu */
  ff_aspect_optionmenu = gtk_option_menu_new ();
  gtk_widget_show (ff_aspect_optionmenu);
  gtk_table_attach (GTK_TABLE (table3), ff_aspect_optionmenu, 1, 2, flags_row, flags_row+1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (ff_aspect_optionmenu, _("Select aspect ratio"), NULL);


  ff_aspect_optionmenu_menu = gtk_menu_new ();
  glade_menuitem = gtk_menu_item_new_with_label (_("auto from pixelsize"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_aspect_optionmenu_menu), glade_menuitem);
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_aspect_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_ASPECT_00_AUTO);
  glade_menuitem = gtk_menu_item_new_with_label (_("4:3"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_aspect_optionmenu_menu), glade_menuitem);
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_aspect_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_ASPECT_01_4_3);
  glade_menuitem = gtk_menu_item_new_with_label (_("16:9"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (ff_aspect_optionmenu_menu), glade_menuitem);
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_ff_aspect_optionmenu),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_GVE_FFMPEG_ASPECT_02_16_9);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (ff_aspect_optionmenu), ff_aspect_optionmenu_menu);




  }


  ff_dont_recode_checkbutton = gtk_check_button_new_with_label (_("Dont Recode"));
  gtk_widget_show (ff_dont_recode_checkbutton);
  gtk_table_attach (GTK_TABLE (table2), ff_dont_recode_checkbutton, 1, 2, 6, 7,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (ff_dont_recode_checkbutton
                        , _("Bypass the FFMPEG Vidoencoder where inputframes can "
			    "be copied 1:1 from an input MPEG videofile."
			    "This experimental feature provides lossless MPEG "
			    "video cut, but works only for the MPEG Fileformats.")
			, NULL);



  frame5 = gtk_frame_new (_("FFMpeg Expert Encoder Options"));
  gtk_widget_show (frame5);
  gtk_container_add (GTK_CONTAINER (notebook1), frame5);
  gtk_container_set_border_width (GTK_CONTAINER (frame5), 4);

  /* the notebook page label for expert encoder options */
  nb1_label3 = gtk_label_new (_("Expert Options"));
  gtk_widget_show (nb1_label3);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook1), 2), nb1_label3);


  table4 = gtk_table_new (14, 3, FALSE);
  gtk_widget_show (table4);
  gtk_container_add (GTK_CONTAINER (frame5), table4);
  gtk_container_set_border_width (GTK_CONTAINER (table4), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table4), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table4), 4);

  ff_qblur_label = gtk_label_new (_("qblur:                 "));
  gtk_widget_show (ff_qblur_label);
  gtk_table_attach (GTK_TABLE (table4), ff_qblur_label, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (ff_qblur_label), 0, 0.5);

  label22 = gtk_label_new (_("qcomp:"));
  gtk_widget_show (label22);
  gtk_table_attach (GTK_TABLE (table4), label22, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label22), 0, 0.5);

  label23 = gtk_label_new ("");
  label23_key = gtk_label_parse_uline (GTK_LABEL (label23),
                                   _("rc-init-cplx:"));
  gtk_widget_show (label23);
  gtk_table_attach (GTK_TABLE (table4), label23, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label23), 0, 0.5);

  label24 = gtk_label_new (_("b-qfactor:"));
  gtk_widget_show (label24);
  gtk_table_attach (GTK_TABLE (table4), label24, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label24), 0, 0.5);

  label25 = gtk_label_new (_("i-qfactor:"));
  gtk_widget_show (label25);
  gtk_table_attach (GTK_TABLE (table4), label25, 0, 1, 4, 5,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label25), 0, 0.5);

  label26 = gtk_label_new (_("b-qoffset:"));
  gtk_widget_show (label26);
  gtk_table_attach (GTK_TABLE (table4), label26, 0, 1, 5, 6,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label26), 0, 0.5);

  label27 = gtk_label_new (_("i-qoffset:"));
  gtk_widget_show (label27);
  gtk_table_attach (GTK_TABLE (table4), label27, 0, 1, 6, 7,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label27), 0, 0.5);

  label28 = gtk_label_new (_("Bitrate Tol:"));
  gtk_widget_show (label28);
  gtk_table_attach (GTK_TABLE (table4), label28, 0, 1, 7, 8,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label28), 0, 0.5);

  label29 = gtk_label_new (_("Maxrate Tol:"));
  gtk_widget_show (label29);
  gtk_table_attach (GTK_TABLE (table4), label29, 0, 1, 8, 9,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label29), 0, 0.5);

  label30 = gtk_label_new (_("Minrate Tol:"));
  gtk_widget_show (label30);
  gtk_table_attach (GTK_TABLE (table4), label30, 0, 1, 9, 10,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label30), 0, 0.5);

  label31 = gtk_label_new (_("Bufsize:"));
  gtk_widget_show (label31);
  gtk_table_attach (GTK_TABLE (table4), label31, 0, 1, 10, 11,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label31), 0, 0.5);

  ff_qblur_spinbutton_adj = gtk_adjustment_new (0.5, 0, 100, 0.25, 10, 10);
  ff_qblur_spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (ff_qblur_spinbutton_adj), 1, 2);
  gtk_widget_show (ff_qblur_spinbutton);
  gtk_table_attach (GTK_TABLE (table4), ff_qblur_spinbutton, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (ff_qblur_spinbutton, _("video quantiser scale blur (VBR)"), NULL);

  ff_qcomp_spinbutton_adj = gtk_adjustment_new (0.5, 0, 100, 0.25, 10, 10);
  ff_qcomp_spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (ff_qcomp_spinbutton_adj), 1, 2);
  gtk_widget_show (ff_qcomp_spinbutton);
  gtk_table_attach (GTK_TABLE (table4), ff_qcomp_spinbutton, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (ff_qcomp_spinbutton, _("video quantiser scale compression (VBR)"), NULL);

  ff_rc_init_cplx_spinbutton_adj = gtk_adjustment_new (1.25, 0, 100, 0.25, 10, 10);
  ff_rc_init_cplx_spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (ff_rc_init_cplx_spinbutton_adj), 1, 2);
  gtk_widget_show (ff_rc_init_cplx_spinbutton);
  gtk_table_attach (GTK_TABLE (table4), ff_rc_init_cplx_spinbutton, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (ff_rc_init_cplx_spinbutton, _("initial complexity for 1-pass encoding"), NULL);

  ff_b_qfactor_spinbutton_adj = gtk_adjustment_new (1.25, -100, 100, 0.2, 10, 10);
  ff_b_qfactor_spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (ff_b_qfactor_spinbutton_adj), 1, 2);
  gtk_widget_show (ff_b_qfactor_spinbutton);
  gtk_table_attach (GTK_TABLE (table4), ff_b_qfactor_spinbutton, 1, 2, 3, 4,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (ff_b_qfactor_spinbutton, _("qp factor between p and b frames"), NULL);

  ff_i_qfactor_spinbutton_adj = gtk_adjustment_new (-0.8, -100, 100, 0.2, 10, 10);
  ff_i_qfactor_spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (ff_i_qfactor_spinbutton_adj), 1, 2);
  gtk_widget_show (ff_i_qfactor_spinbutton);
  gtk_table_attach (GTK_TABLE (table4), ff_i_qfactor_spinbutton, 1, 2, 4, 5,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (ff_i_qfactor_spinbutton, _("qp factor between p and i frames"), NULL);

  ff_b_qoffset_spinbutton_adj = gtk_adjustment_new (1.25, 0, 100, 0.25, 10, 10);
  ff_b_qoffset_spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (ff_b_qoffset_spinbutton_adj), 1, 2);
  gtk_widget_show (ff_b_qoffset_spinbutton);
  gtk_table_attach (GTK_TABLE (table4), ff_b_qoffset_spinbutton, 1, 2, 5, 6,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (ff_b_qoffset_spinbutton, _("qp offset between p and b frames"), NULL);

  ff_i_qoffset_spinbutton_adj = gtk_adjustment_new (0, 0, 100, 0.25, 10, 10);
  ff_i_qoffset_spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (ff_i_qoffset_spinbutton_adj), 1, 2);
  gtk_widget_show (ff_i_qoffset_spinbutton);
  gtk_table_attach (GTK_TABLE (table4), ff_i_qoffset_spinbutton, 1, 2, 6, 7,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (ff_i_qoffset_spinbutton, _("qp offset between p and i frames"), NULL);

  ff_bitrate_tol_spinbutton_adj = gtk_adjustment_new (4200, 0, 10000, 100, 1000, 10);
  ff_bitrate_tol_spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (ff_bitrate_tol_spinbutton_adj), 1, 0);
  gtk_widget_show (ff_bitrate_tol_spinbutton);
  gtk_table_attach (GTK_TABLE (table4), ff_bitrate_tol_spinbutton, 1, 2, 7, 8,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (ff_bitrate_tol_spinbutton, _("set video bitrate tolerance (in kbit/s)"), NULL);

  ff_maxrate_tol_spinbutton_adj = gtk_adjustment_new (0, 0, 10000, 100, 1000, 10);
  ff_maxrate_tol_spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (ff_maxrate_tol_spinbutton_adj), 1, 0);
  gtk_widget_show (ff_maxrate_tol_spinbutton);
  gtk_table_attach (GTK_TABLE (table4), ff_maxrate_tol_spinbutton, 1, 2, 8, 9,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (ff_maxrate_tol_spinbutton, _("set max video bitrate tolerance (in kbit/s)"), NULL);

  ff_minrate_tol_spinbutton_adj = gtk_adjustment_new (0, 0, 10000, 100, 1000, 10);
  ff_minrate_tol_spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (ff_minrate_tol_spinbutton_adj), 1, 0);
  gtk_widget_show (ff_minrate_tol_spinbutton);
  gtk_table_attach (GTK_TABLE (table4), ff_minrate_tol_spinbutton, 1, 2, 9, 10,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (ff_minrate_tol_spinbutton, _("set min video bitrate tolerance (in kbit/s)"), NULL);

  ff_bufsize_spinbutton_adj = gtk_adjustment_new (0, 0, 10000, 100, 1000, 10);
  ff_bufsize_spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (ff_bufsize_spinbutton_adj), 1, 0);
  gtk_widget_show (ff_bufsize_spinbutton);
  gtk_table_attach (GTK_TABLE (table4), ff_bufsize_spinbutton, 1, 2, 10, 11,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (ff_bufsize_spinbutton, _("set ratecontrol buffere size (in kbit)"), NULL);

  label37 = gtk_label_new (_("strictness:"));
  gtk_widget_show (label37);
  gtk_table_attach (GTK_TABLE (table4), label37, 0, 1, 11, 12,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label37), 0, 0.5);

  ff_strict_spinbutton_adj = gtk_adjustment_new (1, 0, 100, 1, 10, 10);
  ff_strict_spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (ff_strict_spinbutton_adj), 1, 0);
  gtk_widget_show (ff_strict_spinbutton);
  gtk_table_attach (GTK_TABLE (table4), ff_strict_spinbutton, 1, 2, 11, 12,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (ff_strict_spinbutton, _("how strictly to follow the standards"), NULL);

  label38 = gtk_label_new (_("mb-qmin:"));
  gtk_widget_show (label38);
  gtk_table_attach (GTK_TABLE (table4), label38, 0, 1, 12, 13,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label38), 0, 0.5);

  label39 = gtk_label_new (_("mb-qmax:"));
  gtk_widget_show (label39);
  gtk_table_attach (GTK_TABLE (table4), label39, 0, 1, 13, 14,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label39), 0, 0.5);

  ff_mb_qmin_spinbutton_adj = gtk_adjustment_new (0, 0, 31, 1, 10, 10);
  ff_mb_qmin_spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (ff_mb_qmin_spinbutton_adj), 1, 0);
  gtk_widget_show (ff_mb_qmin_spinbutton);
  gtk_table_attach (GTK_TABLE (table4), ff_mb_qmin_spinbutton, 1, 2, 12, 13,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (ff_mb_qmin_spinbutton, _("min macroblock quantiser scale (VBR)"), NULL);

  ff_mb_qmax_spinbutton_adj = gtk_adjustment_new (31, 0, 31, 1, 10, 10);
  ff_mb_qmax_spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (ff_mb_qmax_spinbutton_adj), 1, 0);
  gtk_widget_show (ff_mb_qmax_spinbutton);
  gtk_table_attach (GTK_TABLE (table4), ff_mb_qmax_spinbutton, 1, 2, 13, 14,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (ff_mb_qmax_spinbutton, _("max macroblock quantiser scale (VBR)"), NULL);


  frame6 = gtk_frame_new (_("FFMpeg 2 Pass Expert settings"));
  gtk_widget_show (frame6);
  gtk_container_add (GTK_CONTAINER (notebook1), frame6);
  gtk_container_set_border_width (GTK_CONTAINER (frame6), 4);

  /* the notebook page label for 2 Pass Expert settings */
  nb1_label4 = gtk_label_new (_("Expert 2Pass"));
  gtk_widget_show (nb1_label4);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook1), 3), nb1_label4);




  table5 = gtk_table_new (3, 3, FALSE);
  gtk_widget_show (table5);
  gtk_container_add (GTK_CONTAINER (frame6), table5);
  gtk_container_set_border_width (GTK_CONTAINER (table5), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table5), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table5), 4);

  label32 = gtk_label_new (_("Pass Logfile:        "));
  gtk_widget_show (label32);
  gtk_table_attach (GTK_TABLE (table5), label32, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label32), 0, 0.5);

  ff_passlogfile_entry = gtk_entry_new ();
  gtk_widget_show (ff_passlogfile_entry);
  gtk_table_attach (GTK_TABLE (table5), ff_passlogfile_entry, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (ff_passlogfile_entry, _("The pass logfile is only used as workfile for 2-pass encoding"), NULL);

  ff_passlogfile_filesel_button = gtk_button_new_with_label (_("..."));
  gtk_widget_show (ff_passlogfile_filesel_button);
  gtk_table_attach (GTK_TABLE (table5), ff_passlogfile_filesel_button, 2, 3, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (ff_passlogfile_filesel_button, _("Select pass logfile via file browser"), NULL);

  ff_pass_checkbutton = gtk_check_button_new_with_label (_("2 Pass Encoding"));
  gtk_widget_show (ff_pass_checkbutton);
  gtk_table_attach (GTK_TABLE (table5), ff_pass_checkbutton, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (ff_pass_checkbutton, _("Activate 2 pass encoding when set"), NULL);

  ff_2pass_info_label = gtk_label_new ("");
  gtk_widget_show (ff_2pass_info_label);
  gtk_table_attach (GTK_TABLE (table5), ff_2pass_info_label, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (ff_2pass_info_label), 0, 0.5);


  frame7 = gtk_frame_new (_("FFMpeg File Comment settings"));
  gtk_widget_show (frame7);
  gtk_container_add (GTK_CONTAINER (notebook1), frame7);
  gtk_container_set_border_width (GTK_CONTAINER (frame7), 3);

  /* the notebook page label for file comment settings */
  nb1_label5 = gtk_label_new (_("File Comment"));
  gtk_widget_show (nb1_label5);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook1), 4), nb1_label5);



  table6 = gtk_table_new (5, 2, FALSE);
  gtk_widget_show (table6);
  gtk_container_add (GTK_CONTAINER (frame7), table6);
  gtk_container_set_border_width (GTK_CONTAINER (table6), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table6), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table6), 4);

  label33 = gtk_label_new (_("Title:                  "));
  gtk_widget_show (label33);
  gtk_table_attach (GTK_TABLE (table6), label33, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label33), 0, 0.5);

  label34 = gtk_label_new (_("Author:"));
  gtk_widget_show (label34);
  gtk_table_attach (GTK_TABLE (table6), label34, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label34), 0, 0.5);

  label35 = gtk_label_new (_("Copyright:"));
  gtk_widget_show (label35);
  gtk_table_attach (GTK_TABLE (table6), label35, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label35), 0, 0.5);

  label36 = gtk_label_new (_("Comment:"));
  gtk_widget_show (label36);
  gtk_table_attach (GTK_TABLE (table6), label36, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label36), 0, 0.5);

  ff_title_entry = gtk_entry_new_with_max_length (40);
  gtk_widget_show (ff_title_entry);
  gtk_table_attach (GTK_TABLE (table6), ff_title_entry, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  ff_author_entry = gtk_entry_new_with_max_length (40);
  gtk_widget_show (ff_author_entry);
  gtk_table_attach (GTK_TABLE (table6), ff_author_entry, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  ff_copyright_entry = gtk_entry_new_with_max_length (40);
  gtk_widget_show (ff_copyright_entry);
  gtk_table_attach (GTK_TABLE (table6), ff_copyright_entry, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  ff_comment_entry = gtk_entry_new_with_max_length (80);
  gtk_widget_show (ff_comment_entry);
  gtk_table_attach (GTK_TABLE (table6), ff_comment_entry, 1, 2, 3, 4,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  ff_filecomment_label = gtk_label_new (_("\nText Tags will be inserted in the\nresulting Video for all non blank entry widgets.  "));
  gtk_widget_show (ff_filecomment_label);
  gtk_table_attach (GTK_TABLE (table6), ff_filecomment_label, 1, 2, 4, 5,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (ff_filecomment_label), 0, 0.5);



  g_signal_connect (G_OBJECT (ff_aud_bitrate_spinbutton), "changed",
                      G_CALLBACK (on_ff_aud_bitrate_spinbutton_changed),
                      gpp);
  g_signal_connect (G_OBJECT (ff_vid_bitrate_spinbutton), "changed",
                      G_CALLBACK (on_ff_vid_bitrate_spinbutton_changed),
                      gpp);
  g_signal_connect (G_OBJECT (ff_qscale_spinbutton), "changed",
                      G_CALLBACK (on_ff_qscale_spinbutton_changed),
                      gpp);
  g_signal_connect (G_OBJECT (ff_qmin_spinbutton), "changed",
                      G_CALLBACK (on_ff_qmin_spinbutton_changed),
                      gpp);
  g_signal_connect (G_OBJECT (ff_qmax_spinbutton), "changed",
                      G_CALLBACK (on_ff_qmax_spinbutton_changed),
                      gpp);
  g_signal_connect (G_OBJECT (ff_qdiff_spinbutton), "changed",
                      G_CALLBACK (on_ff_qdiff_spinbutton_changed),
                      gpp);
  g_signal_connect (G_OBJECT (ff_gop_size_spinbutton), "changed",
                      G_CALLBACK (on_ff_gop_size_spinbutton_changed),
                      gpp);
  g_signal_connect (G_OBJECT (ff_intra_checkbutton), "toggled",
                      G_CALLBACK (on_ff_intra_checkbutton_toggled),
                      gpp);
  g_signal_connect (G_OBJECT (ff_aic_checkbutton), "toggled",
                      G_CALLBACK (on_ff_aic_checkbutton_toggled),
                      gpp);
  g_signal_connect (G_OBJECT (ff_umv_checkbutton), "toggled",
                      G_CALLBACK (on_ff_umv_checkbutton_toggled),
                      gpp);
  g_signal_connect (G_OBJECT (ff_bitexact_checkbutton), "toggled",
                      G_CALLBACK (on_ff_bitexact_checkbutton_toggled),
                      gpp);
  g_signal_connect (G_OBJECT (ff_aspect_checkbutton), "toggled",
                      G_CALLBACK (on_ff_aspect_checkbutton_toggled),
                      gpp);
  g_signal_connect (G_OBJECT (ff_b_frames_checkbutton), "toggled",
                      G_CALLBACK (on_ff_b_frames_checkbutton_toggled),
                      gpp);
  g_signal_connect (G_OBJECT (ff_mv4_checkbutton), "toggled",
                      G_CALLBACK (on_ff_mv4_checkbutton_toggled),
                      gpp);
  g_signal_connect (G_OBJECT (ff_partitioning_checkbutton), "toggled",
                      G_CALLBACK (on_ff_partitioning_checkbutton_toggled),
                      gpp);
  g_signal_connect (G_OBJECT (ff_dont_recode_checkbutton), "toggled",
                      G_CALLBACK (on_ff_dont_recode_checkbutton_toggled),
                      gpp);
  g_signal_connect (G_OBJECT (ff_qblur_spinbutton), "changed",
                      G_CALLBACK (on_ff_qblur_spinbutton_changed),
                      gpp);
  g_signal_connect (G_OBJECT (ff_qcomp_spinbutton), "changed",
                      G_CALLBACK (on_ff_qcomp_spinbutton_changed),
                      gpp);
  g_signal_connect (G_OBJECT (ff_rc_init_cplx_spinbutton), "changed",
                      G_CALLBACK (on_ff_rc_init_cplx_spinbutton_changed),
                      gpp);
  g_signal_connect (G_OBJECT (ff_b_qfactor_spinbutton), "changed",
                      G_CALLBACK (on_ff_b_qfactor_spinbutton_changed),
                      gpp);
  g_signal_connect (G_OBJECT (ff_i_qfactor_spinbutton), "changed",
                      G_CALLBACK (on_ff_i_qfactor_spinbutton_changed),
                      gpp);
  g_signal_connect (G_OBJECT (ff_b_qoffset_spinbutton), "changed",
                      G_CALLBACK (on_ff_b_qoffset_spinbutton_changed),
                      gpp);
  g_signal_connect (G_OBJECT (ff_i_qoffset_spinbutton), "changed",
                      G_CALLBACK (on_ff_i_qoffset_spinbutton_changed),
                      gpp);
  g_signal_connect (G_OBJECT (ff_bitrate_tol_spinbutton), "changed",
                      G_CALLBACK (on_ff_bitrate_tol_spinbutton_changed),
                      gpp);
  g_signal_connect (G_OBJECT (ff_maxrate_tol_spinbutton), "changed",
                      G_CALLBACK (on_ff_maxrate_tol_spinbutton_changed),
                      gpp);
  g_signal_connect (G_OBJECT (ff_minrate_tol_spinbutton), "changed",
                      G_CALLBACK (on_ff_minrate_tol_spinbutton_changed),
                      gpp);
  g_signal_connect (G_OBJECT (ff_bufsize_spinbutton), "changed",
                      G_CALLBACK (on_ff_bufsize_spinbutton_changed),
                      gpp);
  g_signal_connect (G_OBJECT (ff_strict_spinbutton), "changed",
                      G_CALLBACK (on_ff_strict_spinbutton_changed),
                      gpp);
  g_signal_connect (G_OBJECT (ff_mb_qmin_spinbutton), "changed",
                      G_CALLBACK (on_ff_mb_qmin_spinbutton_changed),
                      gpp);
  g_signal_connect (G_OBJECT (ff_mb_qmax_spinbutton), "changed",
                      G_CALLBACK (on_ff_mb_qmax_spinbutton_changed),
                      gpp);
  g_signal_connect (G_OBJECT (ff_passlogfile_entry), "changed",
                      G_CALLBACK (on_ff_passlogfile_entry_changed),
                      gpp);
  g_signal_connect (G_OBJECT (ff_passlogfile_filesel_button), "clicked",
                      G_CALLBACK (on_ff_passlogfile_filesel_button_clicked),
                      gpp);
  g_signal_connect (G_OBJECT (ff_pass_checkbutton), "toggled",
                      G_CALLBACK (on_ff_pass_checkbutton_toggled),
                      gpp);
  g_signal_connect (G_OBJECT (ff_title_entry), "changed",
                      G_CALLBACK (on_ff_title_entry_changed),
                      gpp);
  g_signal_connect (G_OBJECT (ff_author_entry), "changed",
                      G_CALLBACK (on_ff_author_entry_changed),
                      gpp);
  g_signal_connect (G_OBJECT (ff_copyright_entry), "changed",
                      G_CALLBACK (on_ff_copyright_entry_changed),
                      gpp);
  g_signal_connect (G_OBJECT (ff_comment_entry), "changed",
                      G_CALLBACK (on_ff_comment_entry_changed),
                      gpp);

  gtk_widget_add_accelerator (ff_rc_init_cplx_spinbutton, "grab_focus", accel_group,
                              label23_key, GDK_MOD1_MASK, (GtkAccelFlags) 0);

  gtk_window_add_accel_group (GTK_WINDOW (shell_window), accel_group);


  gpp->ff_aspect_optionmenu               = ff_aspect_optionmenu;
  gpp->ff_aud_bitrate_optionmenu          = ff_aud_bitrate_optionmenu;
  gpp->ff_aud_bitrate_spinbutton          = ff_aud_bitrate_spinbutton;
  gpp->ff_aud_codec_optionmenu            = ff_aud_codec_optionmenu;
  gpp->ff_author_entry                    = ff_author_entry;
  gpp->ff_b_frames_checkbutton            = ff_b_frames_checkbutton;
  gpp->ff_b_qfactor_spinbutton            = ff_b_qfactor_spinbutton;
  gpp->ff_b_qoffset_spinbutton            = ff_b_qoffset_spinbutton;
  gpp->ff_basic_info_label                = ff_basic_info_label;
  gpp->ff_bitrate_tol_spinbutton          = ff_bitrate_tol_spinbutton;
  gpp->ff_bufsize_spinbutton              = ff_bufsize_spinbutton;
  gpp->ff_comment_entry                   = ff_comment_entry;
  gpp->ff_copyright_entry                 = ff_copyright_entry;
  gpp->ff_dct_algo_optionmenu             = ff_dct_algo_optionmenu;
  gpp->ff_dont_recode_checkbutton         = ff_dont_recode_checkbutton;
  gpp->ff_fileformat_optionmenu           = ff_fileformat_optionmenu;
  gpp->ff_gop_size_spinbutton             = ff_gop_size_spinbutton;
  gpp->ff_aic_checkbutton                 = ff_aic_checkbutton;
  gpp->ff_umv_checkbutton                 = ff_umv_checkbutton;
  gpp->ff_bitexact_checkbutton            = ff_bitexact_checkbutton;
  gpp->ff_aspect_checkbutton              = ff_aspect_checkbutton;
  gpp->ff_i_qfactor_spinbutton            = ff_i_qfactor_spinbutton;
  gpp->ff_i_qoffset_spinbutton            = ff_i_qoffset_spinbutton;
  gpp->ff_idct_algo_optionmenu            = ff_idct_algo_optionmenu;
  gpp->ff_intra_checkbutton               = ff_intra_checkbutton;
  gpp->ff_maxrate_tol_spinbutton          = ff_maxrate_tol_spinbutton;
  gpp->ff_mb_qmax_spinbutton              = ff_mb_qmax_spinbutton;
  gpp->ff_mb_qmin_spinbutton              = ff_mb_qmin_spinbutton;
  gpp->ff_mb_decision_optionmenu          = ff_mb_decision_optionmenu;
  gpp->ff_minrate_tol_spinbutton          = ff_minrate_tol_spinbutton;
  gpp->ff_motion_estimation_optionmenu    = ff_motion_estimation_optionmenu;
  gpp->ff_mv4_checkbutton                 = ff_mv4_checkbutton;
  gpp->ff_partitioning_checkbutton        = ff_partitioning_checkbutton;
  gpp->ff_pass_checkbutton                = ff_pass_checkbutton;
  gpp->ff_passlogfile_entry               = ff_passlogfile_entry;
  gpp->ff_presets_optionmenu              = ff_presets_optionmenu;
  gpp->ff_qblur_spinbutton                = ff_qblur_spinbutton;
  gpp->ff_qcomp_spinbutton                = ff_qcomp_spinbutton;
  gpp->ff_qdiff_spinbutton                = ff_qdiff_spinbutton;
  gpp->ff_qmax_spinbutton                 = ff_qmax_spinbutton;
  gpp->ff_qmin_spinbutton                 = ff_qmin_spinbutton;
  gpp->ff_qscale_spinbutton               = ff_qscale_spinbutton;
  gpp->ff_rc_init_cplx_spinbutton         = ff_rc_init_cplx_spinbutton;
  gpp->ff_strict_spinbutton               = ff_strict_spinbutton;
  gpp->ff_title_entry                     = ff_title_entry;
  gpp->ff_vid_bitrate_spinbutton          = ff_vid_bitrate_spinbutton;
  gpp->ff_vid_codec_optionmenu            = ff_vid_codec_optionmenu;



  gpp->ff_aud_bitrate_spinbutton_adj      = ff_aud_bitrate_spinbutton_adj;
  gpp->ff_vid_bitrate_spinbutton_adj      = ff_vid_bitrate_spinbutton_adj;
  gpp->ff_qscale_spinbutton_adj           = ff_qscale_spinbutton_adj;
  gpp->ff_qmin_spinbutton_adj             = ff_qmin_spinbutton_adj;
  gpp->ff_qmax_spinbutton_adj             = ff_qmax_spinbutton_adj;
  gpp->ff_qdiff_spinbutton_adj            = ff_qdiff_spinbutton_adj;
  gpp->ff_gop_size_spinbutton_adj         = ff_gop_size_spinbutton_adj;
  gpp->ff_qblur_spinbutton_adj            = ff_qblur_spinbutton_adj;
  gpp->ff_qcomp_spinbutton_adj            = ff_qcomp_spinbutton_adj;
  gpp->ff_rc_init_cplx_spinbutton_adj     = ff_rc_init_cplx_spinbutton_adj;
  gpp->ff_b_qfactor_spinbutton_adj        = ff_b_qfactor_spinbutton_adj;
  gpp->ff_i_qfactor_spinbutton_adj        = ff_i_qfactor_spinbutton_adj;
  gpp->ff_b_qoffset_spinbutton_adj        = ff_b_qoffset_spinbutton_adj;
  gpp->ff_i_qoffset_spinbutton_adj        = ff_i_qoffset_spinbutton_adj;
  gpp->ff_bitrate_tol_spinbutton_adj      = ff_bitrate_tol_spinbutton_adj;
  gpp->ff_maxrate_tol_spinbutton_adj      = ff_maxrate_tol_spinbutton_adj;
  gpp->ff_minrate_tol_spinbutton_adj      = ff_minrate_tol_spinbutton_adj;
  gpp->ff_bufsize_spinbutton_adj          = ff_bufsize_spinbutton_adj;
  gpp->ff_strict_spinbutton_adj           = ff_strict_spinbutton_adj;
  gpp->ff_mb_qmin_spinbutton_adj          = ff_mb_qmin_spinbutton_adj;
  gpp->ff_mb_qmax_spinbutton_adj          = ff_mb_qmax_spinbutton_adj;

  return shell_window;
}  /* end p_create_fsb__fileselection */


/* ----------------------------------
 * gap_enc_ffgui_ffmpeg_encode_dialog
 * ----------------------------------
 *   return  0 .. OK
 *          -1 .. in case of Error or cancel
 */
gint
gap_enc_ffgui_ffmpeg_encode_dialog(GapGveFFMpegGlobalParams *gpp)
{
  if(gap_debug) printf("gap_enc_ffgui_ffmpeg_encode_dialog: Start\n");

  gimp_ui_init ("gap_video_extract", FALSE);
  gap_stock_init();

  /* ---------- sub dialog windows ----------*/
  gpp->fsb__fileselection = NULL;   /* used for pass log fileselection */
  gpp->ecp = NULL;

  if(gap_debug) printf("gap_enc_ffgui_ffmpeg_encode_dialog: Before p_create_fsb__fileselection\n");

  /* ---------- dialog ----------*/
  gpp->shell_window = p_create_fsb__fileselection (gpp);

  if(gap_debug) printf("p_ffmpeg_encode_dialog: After p_create_fsb__fileselection\n");

  p_set_option_menu_callbacks(gpp);
  gap_enc_ffgui_init_main_dialog_widgets(gpp);
  gtk_widget_show (gpp->shell_window);

  gpp->val.run = 0;
  gtk_main ();

  if(gap_debug) printf("p_ffmpeg_encode_dialog: A F T E R gtk_main run:%d\n", (int)gpp->val.run);

  if(gpp->val.run)
  {
    return 0;
  }
  return -1;
}    /* end gap_enc_ffgui_ffmpeg_encode_dialog (productive version) */
