/* gap_enc_ffmpeg_main.h
 *    global_params structure for GIMP/GAP FFMPEG Video Encoder
 */
/*
 * Changelog:
 * version 2.1.0a;  2005/03/27  hof: support for ffmpeg 0.4.9 and later CVS versions (LIBAVCODEC_BUILD  >= 4744)
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


/* Includes for encoder specific extra LIBS */
#include "avformat.h"
#include "avcodec.h"


#undef HAVE_FULL_FFMPEG

#if FFMPEG_VERSION_INT ==  0x000408
#define HAVE_OLD_FFMPEG_0408
#else
#undef  HAVE_OLD_FFMPEG_0408
#if LIBAVCODEC_BUILD >= 4744
#define HAVE_FULL_FFMPEG
#endif
#endif



#define GAP_HELP_ID_FFMPEG_PARAMS         "plug-in-gap-encpar-ffmpeg"
#define GAP_PLUGIN_NAME_FFMPEG_PARAMS     "plug-in-gap-encpar-ffmpeg"
#define GAP_PLUGIN_NAME_FFMPEG_ENCODE     "plug-in-gap-enc-ffmpeg"

#define GAP_FFMPEG_CURRENT_VID_EXTENSION  "plug-in-gap-enc-ffmpeg-CURRENT-VIDEO-EXTENSION"

#define GAP_GVE_FFMPEG_PRESET_00_NONE           0
#define GAP_GVE_FFMPEG_PRESET_01_DIVX_DEFAULT   1
#define GAP_GVE_FFMPEG_PRESET_02_DIVX_BEST      2
#define GAP_GVE_FFMPEG_PRESET_03_DIVX_LOW       3
#define GAP_GVE_FFMPEG_PRESET_04_DIVX_MS        4
#define GAP_GVE_FFMPEG_PRESET_05_MPEG1_VCD      5
#define GAP_GVE_FFMPEG_PRESET_06_MPEG1_BEST     6
#define GAP_GVE_FFMPEG_PRESET_07_MPEG2_SVCD     7
#define GAP_GVE_FFMPEG_PRESET_08_MPEG2_DVD      8
#define GAP_GVE_FFMPEG_PRESET_09_REAL           9
#define GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS      10

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

#define GAP_GVE_FFMPEG_MOTION_ESTIMATION_01_ZERO   ME_ZERO
#define GAP_GVE_FFMPEG_MOTION_ESTIMATION_02_FULL   ME_FULL
#define GAP_GVE_FFMPEG_MOTION_ESTIMATION_03_LOG    ME_LOG
#define GAP_GVE_FFMPEG_MOTION_ESTIMATION_04_PHODS  ME_PHODS
#define GAP_GVE_FFMPEG_MOTION_ESTIMATION_05_EPZS   ME_EPZS
#define GAP_GVE_FFMPEG_MOTION_ESTIMATION_06_X1     ME_X1
#define GAP_GVE_FFMPEG_MOTION_ESTIMATION_MAX_ELEMENTS   6



#define GAP_GVE_FFMPEG_DCT_ALGO_00_AUTO        0
#define GAP_GVE_FFMPEG_DCT_ALGO_01_FASTINT     1
#define GAP_GVE_FFMPEG_DCT_ALGO_02_INT         2
#define GAP_GVE_FFMPEG_DCT_ALGO_03_MMX         3
#define GAP_GVE_FFMPEG_DCT_ALGO_04_MLIB        4
#define GAP_GVE_FFMPEG_DCT_ALGO_05_ALTIVEC     5
#define GAP_GVE_FFMPEG_DCT_ALGO_06_FAAN        6
#define GAP_GVE_FFMPEG_DCT_ALGO_MAX_ELEMENTS   7


#define GAP_GVE_FFMPEG_IDCT_ALGO_00_AUTO        0
#define GAP_GVE_FFMPEG_IDCT_ALGO_01_INT         1
#define GAP_GVE_FFMPEG_IDCT_ALGO_02_SIMPLE      2
#define GAP_GVE_FFMPEG_IDCT_ALGO_03_SIMPLEMMX   3
#define GAP_GVE_FFMPEG_IDCT_ALGO_04_LIBMPEG2MMX 4
#define GAP_GVE_FFMPEG_IDCT_ALGO_05_PS2         5
#define GAP_GVE_FFMPEG_IDCT_ALGO_06_MLIB        6
#define GAP_GVE_FFMPEG_IDCT_ALGO_07_ARM         7
#define GAP_GVE_FFMPEG_IDCT_ALGO_08_ALTIVEC     8
#define GAP_GVE_FFMPEG_IDCT_ALGO_09_SH4         9
#define GAP_GVE_FFMPEG_IDCT_ALGO_10_SIMPLEARM  10
#define GAP_GVE_FFMPEG_IDCT_ALGO_11_H264       11
#define GAP_GVE_FFMPEG_IDCT_ALGO_MAX_ELEMENTS  12

#define GAP_GVE_FFMPEG_MB_DECISION_00_SIMPLE    0
#define GAP_GVE_FFMPEG_MB_DECISION_01_BITS      1
#define GAP_GVE_FFMPEG_MB_DECISION_02_RD        2
#define GAP_GVE_FFMPEG_MB_DECISION_MAX_ELEMENTS    3

#define GAP_GVE_FFMPEG_ASPECT_00_AUTO    0
#define GAP_GVE_FFMPEG_ASPECT_01_3_2     1
#define GAP_GVE_FFMPEG_ASPECT_02_4_3     2
#define GAP_GVE_FFMPEG_ASPECT_03_16_9    3
#define GAP_GVE_FFMPEG_ASPECT_MAX_ELEMENTS    4


#ifdef HAVE_FULL_FFMPEG
#define GAP_GVE_FFMPEG_CMP_00_SAD     FF_CMP_SAD
#define GAP_GVE_FFMPEG_CMP_01_SSE     FF_CMP_SSE
#define GAP_GVE_FFMPEG_CMP_02_SATD    FF_CMP_SATD
#define GAP_GVE_FFMPEG_CMP_03_DCT     FF_CMP_DCT
#define GAP_GVE_FFMPEG_CMP_04_PSNR    FF_CMP_PSNR
#define GAP_GVE_FFMPEG_CMP_05_BIT     FF_CMP_BIT
#define GAP_GVE_FFMPEG_CMP_06_RD      FF_CMP_RD
#define GAP_GVE_FFMPEG_CMP_07_ZERO    FF_CMP_ZERO
#define GAP_GVE_FFMPEG_CMP_08_VSAD    FF_CMP_VSAD
#define GAP_GVE_FFMPEG_CMP_09_VSSE    FF_CMP_VSSE
#define GAP_GVE_FFMPEG_CMP_10_NSSE    FF_CMP_NSSE
#define GAP_GVE_FFMPEG_CMP_11_W53     FF_CMP_W53
#define GAP_GVE_FFMPEG_CMP_12_W97     FF_CMP_W97
#define GAP_GVE_FFMPEG_CMP_13_DCTMAX  FF_CMP_DCTMAX 
#define GAP_GVE_FFMPEG_CMP_14_CHROMA  FF_CMP_CHROMA
#define GAP_GVE_FFMPEG_CMP_MAX_ELEMENTS   15
#else
#define GAP_GVE_FFMPEG_CMP_00_SAD     0
#define GAP_GVE_FFMPEG_CMP_01_SSE     1
#define GAP_GVE_FFMPEG_CMP_02_SATD    2
#define GAP_GVE_FFMPEG_CMP_03_DCT     3
#define GAP_GVE_FFMPEG_CMP_04_PSNR    4
#define GAP_GVE_FFMPEG_CMP_05_BIT     5
#define GAP_GVE_FFMPEG_CMP_06_RD      6
#define GAP_GVE_FFMPEG_CMP_07_ZERO    7
#define GAP_GVE_FFMPEG_CMP_08_VSAD    8
#define GAP_GVE_FFMPEG_CMP_09_VSSE    9
#define GAP_GVE_FFMPEG_CMP_10_NSSE    10
#define GAP_GVE_FFMPEG_CMP_11_W53     11
#define GAP_GVE_FFMPEG_CMP_12_W97     12
#define GAP_GVE_FFMPEG_CMP_13_DCTMAX  13 
#define GAP_GVE_FFMPEG_CMP_14_CHROMA  256
#define GAP_GVE_FFMPEG_CMP_MAX_ELEMENTS   15
#endif


#ifdef HAVE_FULL_FFMPEG
#define GAP_GVE_FFMPEG_CODER_TYPE_00_VLC       FF_CODER_TYPE_VLC      
#define GAP_GVE_FFMPEG_CODER_TYPE_01_AC        FF_CODER_TYPE_AC
#define GAP_GVE_FFMPEG_CODER_TYPE_MAX_ELEMENTS 2        
#else
#define GAP_GVE_FFMPEG_CODER_TYPE_00_VLC       0        
#define GAP_GVE_FFMPEG_CODER_TYPE_01_AC        1
#define GAP_GVE_FFMPEG_CODER_TYPE_MAX_ELEMENTS 2        
#endif

#ifdef HAVE_FULL_FFMPEG
#define GAP_GVE_FFMPEG_PREDICTOR_00_LEFT        FF_PRED_LEFT
#define GAP_GVE_FFMPEG_PREDICTOR_01_PLANE       FF_PRED_PLANE
#define GAP_GVE_FFMPEG_PREDICTOR_02_MEDIAN      FF_PRED_MEDIAN
#define GAP_GVE_FFMPEG_PREDICTOR_MAX_ELEMENTS   3
#else
#define GAP_GVE_FFMPEG_PREDICTOR_00_LEFT        0
#define GAP_GVE_FFMPEG_PREDICTOR_01_PLANE       1
#define GAP_GVE_FFMPEG_PREDICTOR_02_MEDIAN      2 
#define GAP_GVE_FFMPEG_PREDICTOR_MAX_ELEMENTS   3
#endif


#ifdef HAVE_FULL_FFMPEG
#define GAP_GVE_FF_QP2LAMBDA   FF_QP2LAMBDA
#else
#define GAP_GVE_FF_QP2LAMBDA   118
#endif



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
  gdouble qscale;       /* changed from int to float with ffmpeg 0.4.9pre1 */
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


  /* new params (introduced with ffmpeg 0.4.9pre1) */
  gint32   thread_count;
  gint32   mb_cmp;
  gint32   ildct_cmp;
  gint32   sub_cmp;
  gint32   cmp;
  gint32   pre_cmp;
  gint32   pre_me;
  gdouble  lumi_mask;
  gdouble  dark_mask;
  gdouble  scplx_mask;
  gdouble  tcplx_mask;
  gdouble  p_mask;
  gint32   qns;

  gint32 use_ss;
  gint32 use_aiv;
  gint32 use_obmc;
  gint32 use_loop;
  gint32 use_alt_scan;
  gint32 use_trell;
  gint32 use_mv0;
  gint32 do_normalize_aqp;
  gint32 use_scan_offset;
  gint32 closed_gop;
  gint32 use_qpel;
  gint32 use_qprd;
  gint32 use_cbprd;
  gint32 do_interlace_dct;
  gint32 do_interlace_me;
  gint32 video_lmin;
  gint32 video_lmax;
  gint32 video_lelim;
  gint32 video_celim;
  gint32 video_intra_quant_bias;
  gint32 video_inter_quant_bias;
  gint32 me_threshold;
  gint32 mb_threshold;
  gint32 intra_dc_precision;
  gint32 error_rate;
  gint32 noise_reduction;
  gint32 sc_threshold;
  gint32 me_range;
  gint32 coder;
  gint32 context;
  gint32 predictor;
  gint32 nsse_weight;
  gint32 subpel_quality;


  /* new parms (found CVS version 2005.03.02 == LIBAVCODEC_BUILD 4744 ) */
  gint32 strict_gop;
  gint32 no_output;
  gint32 video_mb_lmin;
  gint32 video_mb_lmax;

  gint32 video_profile;
  gint32 video_level;
  gint32 frame_skip_threshold;
  gint32 frame_skip_factor;
  gint32 frame_skip_exp;
  gint32 frame_skip_cmp;

  gint32  mux_rate;
  gint32  mux_packet_size;
  gdouble mux_preload;
  gdouble mux_max_delay;


  gint32  ntsc_width;
  gint32  ntsc_height;
  gint32  pal_width;
  gint32  pal_height;
  
} GapGveFFMpegValues;




typedef struct GapGveFFMpegGlobalParams {   /* nick: gpp */
  GapGveCommonValues   val;
  GapGveEncAInfo       ainfo;
  GapGveEncList        *ecp;

  GapGveFFMpegValues   evl;

  GtkWidget *shell_window;
  gboolean   startup;
  gboolean   ffpar_save_flag;
  char       ffpar_filename[1024];
  GtkWidget *ffpar_fileselection;   /* ffmpeg video encoder parameter file */
  GtkWidget *fsb__fileselection;    /* passlog file */

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

  GtkWidget *ff_mb_cmp_combo;
  GtkWidget *ff_ildct_cmp_combo;
  GtkWidget *ff_sub_cmp_combo;
  GtkWidget *ff_cmp_combo;
  GtkWidget *ff_pre_cmp_combo;
  GtkWidget *ff_frame_skip_cmp_combo;
  GtkWidget *ff_coder_combo;
  GtkWidget *ff_predictor_combo;
  
  GtkWidget *ff_use_ss_checkbutton;
  GtkWidget *ff_use_aiv_checkbutton;
  GtkWidget *ff_use_obmc_checkbutton;
  GtkWidget *ff_use_loop_checkbutton;
  GtkWidget *ff_use_alt_scan_checkbutton;
  GtkWidget *ff_use_trell_checkbutton;
  GtkWidget *ff_use_mv0_checkbutton;
  GtkWidget *ff_do_normalize_aqp_checkbutton;
  GtkWidget *ff_use_scan_offset_checkbutton;
  GtkWidget *ff_closed_gop_checkbutton;
  GtkWidget *ff_use_qpel_checkbutton;
  GtkWidget *ff_use_qprd_checkbutton;
  GtkWidget *ff_use_cbprd_checkbutton;
  GtkWidget *ff_do_interlace_dct_checkbutton;
  GtkWidget *ff_do_interlace_me_checkbutton;
  GtkWidget *ff_strict_gop_checkbutton;
  GtkWidget *ff_no_output_checkbutton;

  GtkWidget *ff_mux_rate_spinbutton;
  GtkWidget *ff_mux_packet_size_spinbutton;
  GtkWidget *ff_mux_preload_spinbutton;
  GtkWidget *ff_mux_max_delay_spinbutton;


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

  GtkObject *ff_mux_rate_adj;
  GtkObject *ff_mux_packet_size_adj;
  GtkObject *ff_mux_preload_adj;
  GtkObject *ff_mux_max_delay_adj;

} GapGveFFMpegGlobalParams;


void  gap_enc_ffmpeg_main_init_preset_params(GapGveFFMpegValues *epp, gint preset_idx);

#endif
