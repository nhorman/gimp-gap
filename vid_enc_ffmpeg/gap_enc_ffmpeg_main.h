/* gap_enc_ffmpeg_main.h
 *    global_params structure for GIMP/GAP FFMPEG Video Encoder
 */
/*
 * Changelog:
 * version 2.1.0a;  2004/11/05  hof: replaced deprecated option menu by gimp_int_combo_box
 * version 2.1.0a;  2004.06.05   hof: update params from ffmpeg 0.4.6 to 0.4.8
 * version 2.1.0a;  2004.05.07   created
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

#ifndef GAP_ENC_FFMPEG_MAIN_H
#define GAP_ENC_FFMPEG_MAIN_H

#include <config.h>
#include "gap_gvetypes.h"

#define GAP_HELP_ID_FFMPEG_PARAMS         "plug-in-gap-encpar-ffmpeg"
#define GAP_PLUGIN_NAME_FFMPEG_PARAMS     "plug_in_gap_encpar_ffmpeg"
#define GAP_PLUGIN_NAME_FFMPEG_ENCODE     "plug_in_gap_enc_ffmpeg"

#define GAP_FFMPEG_CURRENT_VID_EXTENSION  "plug_in_gap_enc_ffmpeg_CURRENT_VIDEO_EXTENSION"

#define GAP_GVE_FFMPEG_PRESET_00_NONE           0
#define GAP_GVE_FFMPEG_PRESET_01_DIVX_DEFAULT   1
#define GAP_GVE_FFMPEG_PRESET_02_DIVX_BEST      2
#define GAP_GVE_FFMPEG_PRESET_03_DIVX_LOW       3
#define GAP_GVE_FFMPEG_PRESET_04_MPEG1_VCD      4
#define GAP_GVE_FFMPEG_PRESET_05_MPEG1_BEST     5
#define GAP_GVE_FFMPEG_PRESET_06_MPEG2_VBR      6
#define GAP_GVE_FFMPEG_PRESET_07_REAL           7
#define GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS      8

#define GAP_GVE_FFMPEG_AUDIO_KBIT_RATE_00_32   0
#define GAP_GVE_FFMPEG_AUDIO_KBIT_RATE_01_40   1
#define GAP_GVE_FFMPEG_AUDIO_KBIT_RATE_02_48   2
#define GAP_GVE_FFMPEG_AUDIO_KBIT_RATE_03_56   3
#define GAP_GVE_FFMPEG_AUDIO_KBIT_RATE_04_64   4
#define GAP_GVE_FFMPEG_AUDIO_KBIT_RATE_05_80   5
#define GAP_GVE_FFMPEG_AUDIO_KBIT_RATE_06_96   6
#define GAP_GVE_FFMPEG_AUDIO_KBIT_RATE_07_112  7
#define GAP_GVE_FFMPEG_AUDIO_KBIT_RATE_08_128  8
#define GAP_GVE_FFMPEG_AUDIO_KBIT_RATE_09_160  9
#define GAP_GVE_FFMPEG_AUDIO_KBIT_RATE_10_192  10
#define GAP_GVE_FFMPEG_AUDIO_KBIT_RATE_11_224  11
#define GAP_GVE_FFMPEG_AUDIO_KBIT_RATE_12_256  12
#define GAP_GVE_FFMPEG_AUDIO_KBIT_RATE_13_320  13
#define GAP_GVE_FFMPEG_AUDIO_KBIT_RATE_MAX_ELEMENTS   14

#define GAP_GVE_FFMPEG_MOTION_ESTIMATION_00_ZERO   0
#define GAP_GVE_FFMPEG_MOTION_ESTIMATION_01_FULL   1
#define GAP_GVE_FFMPEG_MOTION_ESTIMATION_02_LOG    2
#define GAP_GVE_FFMPEG_MOTION_ESTIMATION_03_PHODS  3
#define GAP_GVE_FFMPEG_MOTION_ESTIMATION_04_EPZS   4
#define GAP_GVE_FFMPEG_MOTION_ESTIMATION_05_X1     5
#define GAP_GVE_FFMPEG_MOTION_ESTIMATION_MAX_ELEMENTS   6

#define GAP_GVE_FFMPEG_DCT_ALGO_00_AUTO        0
#define GAP_GVE_FFMPEG_DCT_ALGO_01_FASTINT     1
#define GAP_GVE_FFMPEG_DCT_ALGO_02_INT         2
#define GAP_GVE_FFMPEG_DCT_ALGO_03_MMX         3
#define GAP_GVE_FFMPEG_DCT_ALGO_04_MLIB        4
#define GAP_GVE_FFMPEG_DCT_ALGO_05_ALTIVEC     5
#define GAP_GVE_FFMPEG_DCT_ALGO_MAX_ELEMENTS   6

#define GAP_GVE_FFMPEG_IDCT_ALGO_00_AUTO        0
#define GAP_GVE_FFMPEG_IDCT_ALGO_01_INT         1
#define GAP_GVE_FFMPEG_IDCT_ALGO_02_SIMPLE      2
#define GAP_GVE_FFMPEG_IDCT_ALGO_03_SIMPLEMMX   3
#define GAP_GVE_FFMPEG_IDCT_ALGO_04_LIBMPEG2MMX 4
#define GAP_GVE_FFMPEG_IDCT_ALGO_05_PS2         5
#define GAP_GVE_FFMPEG_IDCT_ALGO_06_MLIB        6
#define GAP_GVE_FFMPEG_IDCT_ALGO_07_ARM         7
#define GAP_GVE_FFMPEG_IDCT_ALGO_08_ALTIVEC     8
#define GAP_GVE_FFMPEG_IDCT_ALGO_MAX_ELEMENTS   9

#define GAP_GVE_FFMPEG_MB_DECISION_00_SIMPLE    0
#define GAP_GVE_FFMPEG_MB_DECISION_01_BITS      1
#define GAP_GVE_FFMPEG_MB_DECISION_02_RD        2
#define GAP_GVE_FFMPEG_MB_DECISION_MAX_ELEMENTS    3

#define GAP_GVE_FFMPEG_ASPECT_00_AUTO    0
#define GAP_GVE_FFMPEG_ASPECT_01_3_2     1
#define GAP_GVE_FFMPEG_ASPECT_02_4_3     2
#define GAP_GVE_FFMPEG_ASPECT_03_16_9    3
#define GAP_GVE_FFMPEG_ASPECT_MAX_ELEMENTS    4

/* ffmpeg specific encoder params */
typedef struct {
  char    current_vid_extension[80];

  /* ffmpeg options */
  char    format_name[20];
  char    vcodec_name[20];
  char    acodec_name[20];
  char    title[40];
  char    author[40];
  char    copyright[40];
  char    comment[80];
  char    passlogfile[512];
  gint32  pass;

  gint32  audio_bitrate;
  gint32  video_bitrate;
  gint32  gop_size;
  gint32  intra;
  gint32  qscale;
  gint32  qmin;
  gint32  qmax;
  gint32  qdiff;
  gdouble qblur;
  gdouble qcomp;
  gdouble rc_init_cplx;
  gdouble b_qfactor;
  gdouble i_qfactor;
  gdouble b_qoffset;
  gdouble i_qoffset;
  gint32  bitrate_tol;
  gint32  maxrate_tol;
  gint32  minrate_tol;
  gint32  bufsize;
  gint32  motion_estimation;
  gint32  dct_algo;
  gint32  idct_algo;
  gint32  strict;
  gint32  mb_qmin;
  gint32  mb_qmax;
  gint32  mb_decision;
  gint32  aic;
  gint32  umv;
  gint32  b_frames;
  gint32  mv4;
  gint32  partitioning;
  gint32  packet_size;
 
  /* ?? debug options */
  gint32  benchmark;
  gint32  hex_dump;
  gint32  psnr;
  gint32  vstats;
  gint32  bitexact;
  gint32  set_aspect_ratio;
  gdouble factor_aspect_ratio;  /* 0.0 == auto detect from pixelsizes */
  /* extras */
  gboolean dont_recode_flag;

} GapGveFFMpegValues;




typedef struct GapGveFFMpegGlobalParams {   /* nick: gpp */
  GapGveCommonValues   val;
  GapGveEncAInfo       ainfo;
  GapGveEncList        *ecp;

  GapGveFFMpegValues   evl;

  GtkWidget *shell_window;
  GtkWidget *fsb__fileselection;

  GtkWidget *ff_aspect_combo;
  GtkWidget *ff_aud_bitrate_combo;
  GtkWidget *ff_aud_bitrate_spinbutton;
  GtkWidget *ff_aud_codec_combo;
  GtkWidget *ff_author_entry;
  GtkWidget *ff_b_frames_spinbutton;
  GtkWidget *ff_b_qfactor_spinbutton;
  GtkWidget *ff_b_qoffset_spinbutton;
  GtkWidget *ff_basic_info_label;
  GtkWidget *ff_bitrate_tol_spinbutton;
  GtkWidget *ff_bufsize_spinbutton;
  GtkWidget *ff_comment_entry;
  GtkWidget *ff_copyright_entry;
  GtkWidget *ff_dct_algo_combo;
  GtkWidget *ff_dont_recode_checkbutton;
  GtkWidget *ff_fileformat_combo;
  GtkWidget *ff_gop_size_spinbutton;
  GtkWidget *ff_bitexact_checkbutton;
  GtkWidget *ff_aspect_checkbutton;
  GtkWidget *ff_aic_checkbutton;
  GtkWidget *ff_umv_checkbutton;
  GtkWidget *ff_i_qfactor_spinbutton;
  GtkWidget *ff_i_qoffset_spinbutton;
  GtkWidget *ff_idct_algo_combo;
  GtkWidget *ff_intra_checkbutton;
  GtkWidget *ff_maxrate_tol_spinbutton;
  GtkWidget *ff_mb_qmax_spinbutton;
  GtkWidget *ff_mb_qmin_spinbutton;
  GtkWidget *ff_mb_decision_combo;
  GtkWidget *ff_minrate_tol_spinbutton;
  GtkWidget *ff_motion_estimation_combo;
  GtkWidget *ff_mv4_checkbutton;
  GtkWidget *ff_partitioning_checkbutton;
  GtkWidget *ff_pass_checkbutton;
  GtkWidget *ff_passlogfile_entry;
  GtkWidget *ff_presets_combo;
  GtkWidget *ff_qblur_spinbutton;
  GtkWidget *ff_qcomp_spinbutton;
  GtkWidget *ff_qdiff_spinbutton;
  GtkWidget *ff_qmax_spinbutton;
  GtkWidget *ff_qmin_spinbutton;
  GtkWidget *ff_qscale_spinbutton;
  GtkWidget *ff_rc_init_cplx_spinbutton;
  GtkWidget *ff_strict_spinbutton;
  GtkWidget *ff_title_entry;
  GtkWidget *ff_vid_bitrate_spinbutton;
  GtkWidget *ff_vid_codec_combo;


  GtkObject *ff_aud_bitrate_spinbutton_adj;
  GtkObject *ff_vid_bitrate_spinbutton_adj;
  GtkObject *ff_qscale_spinbutton_adj;
  GtkObject *ff_qmin_spinbutton_adj;
  GtkObject *ff_qmax_spinbutton_adj;
  GtkObject *ff_qdiff_spinbutton_adj;
  GtkObject *ff_gop_size_spinbutton_adj;
  GtkObject *ff_qblur_spinbutton_adj;
  GtkObject *ff_qcomp_spinbutton_adj;
  GtkObject *ff_rc_init_cplx_spinbutton_adj;
  GtkObject *ff_b_qfactor_spinbutton_adj;
  GtkObject *ff_i_qfactor_spinbutton_adj;
  GtkObject *ff_b_qoffset_spinbutton_adj;
  GtkObject *ff_i_qoffset_spinbutton_adj;
  GtkObject *ff_bitrate_tol_spinbutton_adj;
  GtkObject *ff_maxrate_tol_spinbutton_adj;
  GtkObject *ff_minrate_tol_spinbutton_adj;
  GtkObject *ff_bufsize_spinbutton_adj;
  GtkObject *ff_strict_spinbutton_adj;
  GtkObject *ff_mb_qmin_spinbutton_adj;
  GtkObject *ff_mb_qmax_spinbutton_adj;
  GtkObject *ff_b_frames_spinbutton_adj;

} GapGveFFMpegGlobalParams;


void  gap_enc_ffmpeg_main_init_preset_params(GapGveFFMpegValues *epp, gint preset_idx);


#endif
