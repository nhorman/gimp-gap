/* gap_enc_avi_main.h
 *    global_params structure for GIMP/GAP AVU Video Encoder
 */
/*
 * Changelog:
 * version 2.1.0a;  2004.06.12   created
 */

/*
 * Copyright
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

#ifndef GAP_ENC_AVI_MAIN_H
#define GAP_ENC_AVI_MAIN_H

#include <config.h>
#include "gap_gvetypes.h"



#define GAP_PLUGIN_NAME_AVI_PARAMS     "extension_gap_encpar_avi1"
#define GAP_MENUNAME                   "AVI1"


#define GAP_AVI_VIDCODEC_00_JPEG   0
#define GAP_AVI_VIDCODEC_01_RAW    1
#define GAP_AVI_VIDCODEC_02_XVID   2
#define GAP_AVI_VIDCODEC_MAX_ELEMENTS   3



/* avi specific encoder params  */
typedef struct {
  char   codec_name[50];
  gint32 APP0_marker;

  /* for the builtin "JPEG" CODEC */
  gint32 dont_recode_frames;
  gint32 jpeg_interlaced;
  gint32 jpeg_quality;         /* 0..100% */
  gint32 jpeg_odd_even;

  /* for the "xvid" (Open DivX) CODEC */
  GapGveXvidValues xvid;

} GapGveAviValues;

typedef struct GapGveAviGlobalParams {   /* nick: gpp */
  GapGveCommonValues   val;
  GapGveEncAInfo       ainfo;
  GapGveEncList        *ecp;

  GapGveAviValues   evl;

  GtkWidget *shell_window;
  GtkWidget *notebook_main;

  GtkWidget *combo_codec;
  GtkWidget *app0_checkbutton;
  GtkWidget *jpg_dont_recode_checkbutton;
  GtkWidget *jpg_interlace_checkbutton;
  GtkWidget *jpg_odd_first_checkbutton;
  GtkWidget *jpg_quality_spinbutton;
  GtkObject *jpg_quality_spinbutton_adj;

  GtkWidget *xvid_rc_kbitrate_spinbutton;
  GtkObject *xvid_rc_kbitrate_spinbutton_adj;
  gint32     xvid_kbitrate;
  GtkWidget *xvid_rc_reaction_delay_spinbutton;
  GtkObject *xvid_rc_reaction_delay_spinbutton_adj;
  GtkWidget *xvid_rc_avg_period_spinbutton;
  GtkObject *xvid_rc_avg_period_spinbutton_adj;
  GtkWidget *xvid_rc_buffer_spinbutton;
  GtkObject *xvid_rc_buffer_spinbutton_adj;
  GtkWidget *xvid_max_quantizer_spinbutton;
  GtkObject *xvid_max_quantizer_spinbutton_adj;
  GtkWidget *xvid_min_quantizer_spinbutton;
  GtkObject *xvid_min_quantizer_spinbutton_adj;
  GtkWidget *xvid_max_key_interval_spinbutton;
  GtkObject *xvid_max_key_interval_spinbutton_adj;
  GtkWidget *xvid_quality_spinbutton;
  GtkObject *xvid_quality_spinbutton_adj;
  

} GapGveAviGlobalParams;


void gap_enc_avi_main_init_default_params(GapGveAviValues *epp);

#endif
