/*
- TODO:
  - dont_recode_flag  is still experimental code
    but now basically works, but sometimes does not produce clean output videos
    (need further fixes of framerate, pic_number and timecode informations
     in the copied uncompressed frame chunks)
  - p_ffmpeg_write_frame   :    l_force_keyframe
     still need a working methode to force the encoder to write a keyframe
     (in case after a sequence of copied uncompressed chunks that ends up in P or B frame)
     as workaround we can use the option "Intra Only" 

- RealVideo fileformat: Video+Audio encoding does not work. (unusable results) playback  blocks.
                        (Video only seems OK)
- finish implementation of 2-pass handling
*/

/* gap_enc_main_ffmpeg.c
 *  by hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * This is the MAIN Module for the GAP FFMEG video encoder
 *  This encoder supports encoding of multiple Video/Audio formats and CODECS
 *  (such as AVI/DivX, MPEG1, MPEG2,..)
 *  it is based on the ffmpeg libraries.
 *
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
 * version 2.1.0a;  2004.06.10   hof: removed deinterlace option
 *                               delace was not implemented in the encoder module
 *                               and probably will never be implemented here for reasons a) and b)
 *                               a) the GIMP plug-in-deinterlace can be called via GAP-Filtermacros
 *                               b) for videoframe input the GVA Api provides already deinterlace while fetching frames
 *                                  (see delace options for STORYBOARD VID_FRAMES)
 * version 2.1.0a;  2004.06.05   hof: update params from ffmpeg 0.4.6 to 0.4.8
 * version 1.2.2b;  2003.05.29   hof: created
 */

#include <config.h>



#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>


#include "gap-intl.h"

#include "gap_libgapvidutil.h"
#include "gap_libgimpgap.h"
#include "gap_enc_ffmpeg_main.h"
#include "gap_enc_ffmpeg_gui.h"

#include "gap_audio_wav.h"

/* FFMPEG defaults */
#define DEFAULT_PASS_LOGFILENAME "ffmpeg2pass"
#define MAX_AUDIO_PACKET_SIZE 16384


/* Includes for encoder specific extra LIBS */
#include "avformat.h"
#include "avcodec.h"





static char *gap_enc_ffmpeg_version = "2.1.0a; 2004/05/28";



typedef struct t_ffmpeg_handle
{
 AVFormatContext *output_context;
 AVOutputFormat  *file_oformat;
 AVFormatParameters *ap;

 AVStream        *vid_stream;
 AVStream        *aud_stream;
 AVCodecContext  *vid_codec_context;
 AVCodecContext  *aud_codec_context;
 AVCodec         *vid_codec;
 AVCodec         *aud_codec;
 AVFrame         *big_picture_codec;

 uint8_t         *yuv420_buffer;
 int              yuv420_buffer_size;

 uint8_t         *video_buffer;
 uint8_t         *audio_buffer;
 int              video_buffer_size;
 int              audio_buffer_size;
 int              video_stream_index;
 int              audio_stream_index;
 FILE            *passlog_fp;


 int frame_width;
 int frame_height;
 int frame_topBand;
 int frame_bottomBand;
 int frame_leftBand;
 int frame_rightBand;
 int frame_rate;
 int video_bit_rate;
 int video_bit_rate_tolerance;
 int video_qscale;
 int video_qmin;
 int video_qmax;
 int video_qdiff;
 float video_qblur;
 float video_qcomp;
 char *video_rc_override_string;
 char *video_rc_eq;
 int video_rc_buffer_size;
 float video_rc_buffer_aggressivity;
 int video_rc_max_rate;
 int video_rc_min_rate;
 float video_rc_initial_cplx;
 float video_b_qfactor;
 float video_b_qoffset;
 float video_i_qfactor;
 float video_i_qoffset;
 int me_method;
 int video_disable;
 int video_codec_id;
 int same_quality;
 int b_frames;
 int mb_decision;
 int aic;
 int umv;
 int use_4mv;
 int workaround_bugs;
 int error_resilience;
 int error_concealment;
 int dct_algo;
 int idct_algo;
 int strict;
 int mb_qmin;
 int mb_qmax;
 int use_part;
 int packet_size;

 int gop_size;
 int intra_only;
 int audio_sample_rate;
 int audio_bit_rate;
 int audio_disable;
 int audio_channels;
 int audio_codec_id;

 int64_t recording_time;
 int file_overwrite;
 char *str_title;
 char *str_author;
 char *str_copyright;
 char *str_comment;
 int do_benchmark;
 int do_hex_dump;
 int do_play;
 int do_psnr;
 int do_vstats;
 int do_pass;
 char *pass_logfilename;
 int audio_stream_copy;
 int video_stream_copy;
} t_ffmpeg_handle;



/* ------------------------
 * global gap DEBUG switch
 * ------------------------
 */

/* int gap_debug = 1; */    /* print debug infos */
/* int gap_debug = 0; */    /* 0: dont print debug infos */

int gap_debug = 0;

GapGveFFMpegGlobalParams global_params;
int global_nargs_ffmpeg_enc_par;

static void query(void);
static void run(const gchar *name
              , gint n_params
	      , const GimpParam *param
              , gint *nreturn_vals
	      , GimpParam **return_vals);
static void   p_gimp_get_data(const char *key, void *buffer, gint expected_size);
static void   p_ffmpeg_init_default_params(GapGveFFMpegValues *epp);

static t_ffmpeg_handle * p_ffmpeg_open(GapGveFFMpegGlobalParams *gpp
                                      , gint32 current_pass
                                      , long audio_samplerate
                                      , long audio_channels
                                      );
static int    p_ffmpeg_write_frame_chunk(t_ffmpeg_handle *ffh, gint32 encoded_size);
static int    p_ffmpeg_write_frame(t_ffmpeg_handle *ffh, gboolean force_keyframe);
static int    p_ffmpeg_write_audioframe(t_ffmpeg_handle *ffh, guchar *audio_buf, int frame_bytes);
static void   p_ffmpeg_close(t_ffmpeg_handle *ffh);
static gint   p_ffmpeg_encode(GapGveFFMpegGlobalParams *gpp);



GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc */
  NULL,  /* quit_proc */
  query, /* query_proc */
  run,   /* run_proc */
};


/* ------------------------
 * MAIN query and run
 * ------------------------
 */

MAIN ()

static void
query ()
{
  gchar      *l_ecp_key;

  /* video encoder standard parameters (same for each encoder)  */
  static GimpParamDef args_ffmpeg_enc[] =
  {
    {GIMP_PDB_INT32, "run_mode", "non-interactive"},
    {GIMP_PDB_IMAGE, "image", "Input image (must be one of the input frames, or is ignored if storyboard_file is used"},
    {GIMP_PDB_DRAWABLE, "drawable", "Input drawable (unused)"},
    {GIMP_PDB_STRING, "videofile", "filename of AVI or MPEG video (to write)"},
    {GIMP_PDB_INT32, "range_from", "number of first frame"},
    {GIMP_PDB_INT32, "range_to", "number of last frame"},
    {GIMP_PDB_INT32, "vid_width", "Width of resulting Video Frames (all Frames are scaled to this width)"},
    {GIMP_PDB_INT32, "vid_height", "Height of resulting Video Frames (all Frames are scaled to this height)"},
    {GIMP_PDB_INT32, "vid_format", "videoformat:  0=comp., 1=PAL, 2=NTSC, 3=SECAM, 4=MAC, 5=unspec"},
    {GIMP_PDB_FLOAT, "framerate", "framerate in frames per seconds"},
    {GIMP_PDB_INT32, "samplerate", "audio samplerate in samples per seconds (is ignored .wav files are used)"},
    {GIMP_PDB_STRING, "audfile", "optional audiodata file .wav must contain uncompressed 16 bit samples. pass empty string if no audiodata should be included"},
    {GIMP_PDB_INT32, "use_rest", "0 == use default values for all encoder specific params, "
                                 "1 == use encoder specific params set by the parameter procedure:"
                                 GAP_PLUGIN_NAME_FFMPEG_PARAMS
                                 },
    {GIMP_PDB_STRING, "filtermacro_file", "macro to apply on each handled frame. (textfile with filter plugin names and LASTVALUE bufferdump"},
    {GIMP_PDB_STRING, "storyboard_file", "textfile with list of one or more framesequences"},
    {GIMP_PDB_INT32,  "input_mode", "0 ... image is one of the frames to encode, range_from/to params refere to numberpart of the other frameimages on disc. \n"
                                    "1 ... image is multilayer, range_from/to params refere to layer index. \n"
				    "2 ... image is ignored, input is specified by storyboard_file parameter."},
  };
  static int nargs_ffmpeg_enc = sizeof(args_ffmpeg_enc) / sizeof(args_ffmpeg_enc[0]);

  /* video encoder specific parameters */
  static GimpParamDef args_ffmpeg_enc_par[] =
  {
    {GIMP_PDB_INT32, "run_mode", "interactive, non-interactive"},
    {GIMP_PDB_STRING, "key_stdpar", "key to get standard video encoder params via gimp_get_data"},

    {GIMP_PDB_STRING, "format_name", "fileformat shortname (AVI,MPEG..) execute ffmpeg -formats to get list of supported formats"},
    {GIMP_PDB_STRING, "vcodec_name", "video CODEC shortname. execute the commandline tool: ffmpeg -formats to get list of supported CODECS"},
    {GIMP_PDB_STRING, "acodec_name", "audio CODEC shortname. execute the commandline tool: ffmpeg -formats to get list of supported CODECS"},

    {GIMP_PDB_STRING, "title", "set the title"},
    {GIMP_PDB_STRING, "author", "set the author"},
    {GIMP_PDB_STRING, "copyright", "set the copyright"},
    {GIMP_PDB_STRING, "comment", "set the comment"},
    {GIMP_PDB_STRING, "passlogfile", "logfilename for two pass encoding"},
    {GIMP_PDB_INT32,  "pass", "1: do single pass encoding, 2: do 2 pass encoding"},

    {GIMP_PDB_INT32,  "audio_bitrate", "set audio bitrate (in kbit/s)"},
    {GIMP_PDB_INT32,  "video_bitrate", "set video bitrate (in kbit/s)"},
    {GIMP_PDB_INT32,  "gop_size", "set the group of picture size"},
    {GIMP_PDB_INT32,  "intra", "1:use only intra frames (I), 0:use I,P,B frames"},
    {GIMP_PDB_INT32,  "qscale", "(0 upto 31) use fixed video quantiser scale (VBR)"},
    {GIMP_PDB_INT32,  "qmin", "(0 upto 31) min video quantiser scale (VBR)"},
    {GIMP_PDB_INT32,  "qmax", "(0 upto 31) max video quantiser scale (VBR)"},
    {GIMP_PDB_INT32,  "qdiff", "(0 upto 31) max difference between the quantiser scale (VBR)"},
    {GIMP_PDB_FLOAT,  "qblur", "video quantiser scale blur (VBR)"},
    {GIMP_PDB_FLOAT,  "qcomp", "video quantiser scale compression (VBR)"},
    {GIMP_PDB_FLOAT,  "rc_init_cplx", "initial complexity for 1-pass encoding"},
    {GIMP_PDB_FLOAT,  "b_qfactor", "qp factor between p and b frames"},
    {GIMP_PDB_FLOAT,  "i_qfactor", "qp factor between p and i frames"},
    {GIMP_PDB_FLOAT,  "b_qoffset", "qp offset between p and b frames"},
    {GIMP_PDB_FLOAT,  "i_qoffset", "qp offset between p and i frames"},
    {GIMP_PDB_INT32,  "bitrate_tol", "set video bitrate tolerance (in kbit/s)"},
    {GIMP_PDB_INT32,  "maxrate_tol", "set max video bitrate tolerance (in kbit/s)"},
    {GIMP_PDB_INT32,  "minrate_tol", "set min video bitrate tolerance (in kbit/s)"},
    {GIMP_PDB_INT32,  "bufsize", "set ratecontrol buffere size (in kbit)"},
    {GIMP_PDB_INT32,  "motion_estimation", "set motion estimation method"
                      " 1:zero(fastest), 2:full(best), 3:log, 4:phods, 5:epzs(recommanded), 6:x1"},
    {GIMP_PDB_INT32,  "dct_algo", "set dct algorithm. 0:AUTO, 1:FAST_INT, 2:INT, 3:MMX, 4:MLIB, 5:ALTIVEC"},
    {GIMP_PDB_INT32,  "idct_algo", "set idct algorithm. 0:AUTO, 1:INT, 2:SIMPLE, 3:SIMPLEMMX, 4:LIBMPEG2MMX, 5:PS2, 6:MLIB, 7:ARM, 8:ALTIVEC"},
    {GIMP_PDB_INT32,  "strict", "how strictly to follow the standards  0:"},
    {GIMP_PDB_INT32,  "mb_qmin", "(1 upto 31) min macroblock quantiser scale (VBR)"},
    {GIMP_PDB_INT32,  "mb_qmax", "(1 upto 31) max macroblock quantiser scale (VBR)"},
    {GIMP_PDB_INT32,  "mb_decision", "macroblock decision mode  0:SIMPLE (use mb_cmp) 1:BITS (chooses the one which needs the fewest bits) 2:RD (rate distoration) "},
    {GIMP_PDB_INT32,  "aic", "0:OFF, 1:enable Advanced intra coding (h263+)"},
    {GIMP_PDB_INT32,  "umv", "0:OFF, 1:enable Unlimited Motion Vector (h263+)"},
    {GIMP_PDB_INT32,  "b_frames", "0:OFF, 1: use B frames (only MPEG-4 CODEC)"},
    {GIMP_PDB_INT32,  "mv4", "0:OFF, 1:use four motion vector by macroblock (only MPEG-4 CODEC)"},
    {GIMP_PDB_INT32,  "partitioning", "0:OFF, 1:use data partitioning (only MPEG-4 CODEC)"},
    {GIMP_PDB_INT32,  "packet_size", "packet size in bits (use 0 for default)"},
    {GIMP_PDB_INT32,  "benchmark", "0:OFF, 1:add timings for benchmarking"},
    {GIMP_PDB_INT32,  "hex_dump", "0:OFF, 1:dump each input packet"},
    {GIMP_PDB_INT32,  "psnr", "UNUSED since ffmpeg 0.4.6  0:OFF, 1:calculate PSNR of compressed frames"},
    {GIMP_PDB_INT32,  "vstats", "0:OFF, 1:dump video coding statistics to file"},
    {GIMP_PDB_INT32,  "bitexact", "0:OFF, 1:only use bit exact algorithms (for codec testing)"},
    {GIMP_PDB_INT32,  "set_aspect_ratio", "0:OFF, 1:set video aspect_ratio"},
    {GIMP_PDB_FLOAT,  "factor_aspect_ratio", "0..autodetect from pixelsize, or with/height value"},
    {GIMP_PDB_INT32,  "dont_recode", "0:OFF, 1:try to copy videoframes 1:1 without recode (if possible and input is an mpeg videofile)"}
  };
  static int nargs_ffmpeg_enc_par = sizeof(args_ffmpeg_enc_par) / sizeof(args_ffmpeg_enc_par[0]);

  static GimpParamDef *return_vals = NULL;
  static int nreturn_vals = 0;

  /* video encoder standard query (same for each encoder) */
  static GimpParamDef args_in_ecp[] =
  {
    {GIMP_PDB_STRING, "param_name", "name of the parameter, supported: menu_name, video_extension"},
  };

  static GimpParamDef args_out_ecp[] =
  {
    {GIMP_PDB_STRING, "param_value", "parmeter value"},
  };

  static int nargs_in_enp = sizeof(args_in_ecp) / sizeof(args_in_ecp[0]);
  static int nargs_out_enp = (sizeof(args_out_ecp) / sizeof(args_out_ecp[0]));

  INIT_I18N();

  global_nargs_ffmpeg_enc_par = nargs_ffmpeg_enc_par;

  gimp_install_procedure(GAP_PLUGIN_NAME_FFMPEG_ENCODE,
                         _("ffmpeg video encoding for anim frames. Menu: @FFMPEG@"),
                         _("This plugin does video encoding of animframes based on libavformat."
                         " (also known as FFMPGEG encoder)."
                         " The (optional) audiodata must be RIFF WAVEfmt (.wav) file."
                         " .wav files can be mono (1) or stereo (2channels) audiodata and must be 16bit uncompressed."
                         " IMPORTANT:  Non-interactive callers should first call "
                         "\"" GAP_PLUGIN_NAME_FFMPEG_PARAMS  "\""
                         " to set encoder specific paramters, then set the use_rest parameter to 1 to use them."),
                         "Wolfgang Hofer (hof@gimp.org)",
                         "Wolfgang Hofer",
                         gap_enc_ffmpeg_version,
                         NULL,                      /* has no Menu entry, just a PDB interface */
                         "RGB*, INDEXED*, GRAY*",
                         GIMP_PLUGIN,
                         nargs_ffmpeg_enc, nreturn_vals,
                         args_ffmpeg_enc, return_vals);



  gimp_install_procedure(GAP_PLUGIN_NAME_FFMPEG_PARAMS,
                         _("Set Parameters for GAP ffmpeg video encoder Plugin"),
                         _("This plugin sets ffmpeg specific video encoding parameters."),
                         "Wolfgang Hofer (hof@gimp.org)",
                         "Wolfgang Hofer",
                         gap_enc_ffmpeg_version,
                         NULL,                      /* has no Menu entry, just a PDB interface */
                         NULL,
                         GIMP_PLUGIN,
                         nargs_ffmpeg_enc_par, nreturn_vals,
                         args_ffmpeg_enc_par, return_vals);


  l_ecp_key = g_strdup_printf("%s%s", GAP_QUERY_PREFIX_VIDEO_ENCODERS, GAP_PLUGIN_NAME_FFMPEG_ENCODE);
  gimp_install_procedure(l_ecp_key,
                         _("Get GUI Parameters for GAP ffmpeg video encoder"),
                         _("This plugin returns ffmpeg encoder specific parameters."),
                         "Wolfgang Hofer (hof@gimp.org)",
                         "Wolfgang Hofer",
                         gap_enc_ffmpeg_version,
                         NULL,                      /* has no Menu entry, just a PDB interface */
                         NULL,
                         GIMP_PLUGIN,
                         nargs_in_enp , nargs_out_enp,
                         args_in_ecp, args_out_ecp);


  g_free(l_ecp_key);
}       /* end query */


static void
run (const gchar      *name,
     gint              n_params,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  GapGveFFMpegValues *epp;
  GapGveFFMpegGlobalParams *gpp;

  static GimpParam values[1];
  gint32     l_rc;
  const char *l_env;
  char       *l_ecp_key1;
  char       *l_encoder_key;

  gpp = &global_params;
  epp = &gpp->evl;
  *nreturn_vals = 1;
  *return_vals = values;
  l_rc = 0;

  INIT_I18N();

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_SUCCESS;

  l_env = g_getenv("GAP_DEBUG_ENC");
  if(l_env != NULL)
  {
    if((*l_env != 'n') && (*l_env != 'N')) gap_debug = 1;
  }

  if(gap_debug) printf("\n\nSTART of PlugIn: %s\n", name);

  l_ecp_key1 = g_strdup_printf("%s%s", GAP_QUERY_PREFIX_VIDEO_ENCODERS, GAP_PLUGIN_NAME_FFMPEG_ENCODE);
  l_encoder_key = g_strdup(GAP_PLUGIN_NAME_FFMPEG_ENCODE);


  p_ffmpeg_init_default_params(epp);

  if (strcmp (name, l_ecp_key1) == 0)
  {
      /* this interface replies to the queries of the common encoder gui */
      gchar *param_name;

      param_name = param[0].data.d_string;
      if(gap_debug) printf("query for param_name: %s\n", param_name);
      *nreturn_vals = 2;

      values[1].type = GIMP_PDB_STRING;
      if(strcmp (param_name, GAP_VENC_PAR_MENU_NAME) == 0)
      {
        values[1].data.d_string = g_strdup("FFMPEG");
      }
      else if (strcmp (param_name, GAP_VENC_PAR_VID_EXTENSION) == 0)
      {
        /* check if current extension was already set
         * (in a previous call to the encoder GUI Procedure)
         */
        if(gimp_get_data_size(GAP_FFMPEG_CURRENT_VID_EXTENSION) == sizeof(gpp->evl.current_vid_extension))
        {
          gimp_get_data(GAP_FFMPEG_CURRENT_VID_EXTENSION, gpp->evl.current_vid_extension);
          values[1].data.d_string = g_strdup(gpp->evl.current_vid_extension);
        }
        else
        {
          /* if nothing was set use .avi as the default extension */
          values[1].data.d_string = g_strdup(".avi");
        }
      }
      else if (strcmp (param_name, GAP_VENC_PAR_SHORT_DESCRIPTION) == 0)
      {
        values[1].data.d_string =
          g_strdup(_("FFMPEG Encoder\n"
                     "writes AVI/DivX or MPEG1, MPEG2 (DVD) or MPEG4 encoded videos\n"
                     "based on FFMPEG by Fabrice Bellard"
                     )
                  );
      }
      else if (strcmp (param_name, GAP_VENC_PAR_GUI_PROC) == 0)
      {
        values[1].data.d_string = g_strdup(GAP_PLUGIN_NAME_FFMPEG_PARAMS);
      }
      else
      {
        values[1].data.d_string = g_strdup("\0");
      }
  }
  else if (strcmp (name, GAP_PLUGIN_NAME_FFMPEG_PARAMS) == 0)
  {
      /* this interface sets the encoder specific parameters */
      gint l_set_it;
      gchar  *l_key_stdpar;

      gpp->val.run_mode = param[0].data.d_int32;
      l_key_stdpar = param[1].data.d_string;
      gpp->val.vid_width = 320;
      gpp->val.vid_height = 200;
      p_gimp_get_data(l_key_stdpar, &gpp->val, sizeof(GapGveCommonValues));

      if(gap_debug)  printf("rate: %f  w:%d h:%d\n", (float)gpp->val.framerate, (int)gpp->val.vid_width, (int)gpp->val.vid_height);

      l_set_it = TRUE;
      if (gpp->val.run_mode == GIMP_RUN_NONINTERACTIVE)
      {
        /* set video encoder specific params */
        if (n_params != global_nargs_ffmpeg_enc_par)
        {
          values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
          l_set_it = FALSE;
        }
        else
        {
           gint32 l_ii;

           /* get parameters in NON_INTERACTIVE mode */
           l_ii = 3;
           if(param[l_ii].data.d_string)
           {
             g_snprintf(epp->format_name, sizeof(epp->format_name), "%s", param[l_ii].data.d_string);
           }
           l_ii++;
           if(param[l_ii].data.d_string)
           {
             g_snprintf(epp->vcodec_name, sizeof(epp->vcodec_name), "%s", param[l_ii].data.d_string);
           }
           l_ii++;
           if(param[l_ii].data.d_string)
           {
             g_snprintf(epp->acodec_name, sizeof(epp->acodec_name), "%s", param[l_ii].data.d_string);
           }
           l_ii++;
           if(param[l_ii].data.d_string)
           {
             g_snprintf(epp->title, sizeof(epp->title), "%s", param[l_ii].data.d_string);
           }
           l_ii++;
           if(param[l_ii].data.d_string)
           {
             g_snprintf(epp->author, sizeof(epp->author), "%s", param[l_ii].data.d_string);
           }
           l_ii++;
           if(param[l_ii].data.d_string)
           {
             g_snprintf(epp->copyright, sizeof(epp->copyright), "%s", param[l_ii].data.d_string);
           }
           l_ii++;
           if(param[l_ii].data.d_string)
           {
             g_snprintf(epp->comment, sizeof(epp->comment), "%s", param[l_ii].data.d_string);
           }
           l_ii++;
           if(param[l_ii].data.d_string)
           {
             g_snprintf(epp->passlogfile, sizeof(epp->passlogfile), "%s", param[l_ii].data.d_string);
           }
           l_ii++;

           epp->audio_bitrate       = param[l_ii++].data.d_int32;
           epp->video_bitrate       = param[l_ii++].data.d_int32;
           epp->gop_size            = param[l_ii++].data.d_int32;
           epp->intra               = param[l_ii++].data.d_int32;
           epp->qscale              = param[l_ii++].data.d_int32;
           epp->qmin                = param[l_ii++].data.d_int32;
           epp->pass                = param[l_ii++].data.d_int32;
           epp->qmax                = param[l_ii++].data.d_int32;
           epp->qdiff               = param[l_ii++].data.d_int32;
           epp->qblur               = param[l_ii++].data.d_float;
           epp->qcomp               = param[l_ii++].data.d_float;
           epp->rc_init_cplx        = param[l_ii++].data.d_float;
           epp->b_qfactor           = param[l_ii++].data.d_float;
           epp->i_qfactor           = param[l_ii++].data.d_float;
           epp->b_qoffset           = param[l_ii++].data.d_float;
           epp->i_qoffset           = param[l_ii++].data.d_float;
           epp->bitrate_tol         = param[l_ii++].data.d_int32;
           epp->maxrate_tol         = param[l_ii++].data.d_int32;
           epp->minrate_tol         = param[l_ii++].data.d_int32;
           epp->bufsize             = param[l_ii++].data.d_int32;
           epp->motion_estimation   = param[l_ii++].data.d_int32;
           epp->dct_algo            = param[l_ii++].data.d_int32;
           epp->idct_algo           = param[l_ii++].data.d_int32;
           epp->strict              = param[l_ii++].data.d_int32;
           epp->mb_qmin             = param[l_ii++].data.d_int32;
           epp->mb_qmax             = param[l_ii++].data.d_int32;
           epp->mb_decision         = param[l_ii++].data.d_int32;
           epp->aic                 = param[l_ii++].data.d_int32;
           epp->umv                 = param[l_ii++].data.d_int32;
           epp->b_frames            = param[l_ii++].data.d_int32;
           epp->mv4                 = param[l_ii++].data.d_int32;
           epp->partitioning        = param[l_ii++].data.d_int32;
           epp->packet_size         = param[l_ii++].data.d_int32;
           epp->benchmark           = param[l_ii++].data.d_int32;
           epp->hex_dump            = param[l_ii++].data.d_int32;
           epp->psnr                = param[l_ii++].data.d_int32;
           epp->vstats              = param[l_ii++].data.d_int32;
           epp->bitexact            = param[l_ii++].data.d_int32;
           epp->set_aspect_ratio    = param[l_ii++].data.d_int32;
           epp->factor_aspect_ratio = param[l_ii++].data.d_float;

           epp->dont_recode_flag    = param[l_ii++].data.d_int32;
        }
      }
      else
      {
        /* try to read encoder specific params */
        p_gimp_get_data(l_encoder_key, epp, sizeof(GapGveFFMpegValues));

        if(0 != gap_enc_ffgui_ffmpeg_encode_dialog(gpp))
        {
          l_set_it = FALSE;
        }
      }

      if(l_set_it)
      {
         if(gap_debug) printf("Setting Encoder specific Params\n");
         gimp_set_data(l_encoder_key, epp, sizeof(GapGveFFMpegValues));
         gimp_set_data(GAP_FFMPEG_CURRENT_VID_EXTENSION
                       , &gpp->evl.current_vid_extension[0]
                       , sizeof(gpp->evl.current_vid_extension));

      }

  }
  else   if (strcmp (name, GAP_PLUGIN_NAME_FFMPEG_ENCODE) == 0)
  {
      char *l_base;
      int   l_l;

      /* run the video encoder procedure */

      gpp->val.run_mode = param[0].data.d_int32;

      /* get image_ID and animinfo */
      gpp->val.image_ID    = param[1].data.d_image;
      gap_gve_misc_get_ainfo(gpp->val.image_ID, &gpp->ainfo);

      /* set initial (default) values */
      l_base = g_strdup(gpp->ainfo.basename);
      l_l = strlen(l_base);

      if (l_l > 0)
      {
         if(l_base[l_l -1] == '_')
         {
           l_base[l_l -1] = '\0';
         }
      }
      if(gap_debug) printf("Init Default parameters for %s base: %s\n", name, l_base);
      g_snprintf(gpp->val.videoname, sizeof(gpp->val.videoname), "%s.mpg", l_base);

      gpp->val.audioname1[0] = '\0';
      gpp->val.filtermacro_file[0] = '\0';
      gpp->val.storyboard_file[0] = '\0';
      gpp->val.framerate = gpp->ainfo.framerate;
      gpp->val.range_from = gpp->ainfo.curr_frame_nr;
      gpp->val.range_to   = gpp->ainfo.last_frame_nr;
      gpp->val.samplerate = 0;
      gpp->val.vid_width  = gimp_image_width(gpp->val.image_ID) - (gimp_image_width(gpp->val.image_ID) % 16);
      gpp->val.vid_height = gimp_image_height(gpp->val.image_ID) - (gimp_image_height(gpp->val.image_ID) % 16);
      gpp->val.vid_format = VID_FMT_NTSC;
      gpp->val.input_mode = GAP_RNGTYPE_FRAMES;

      g_free(l_base);

      if (n_params != GAP_VENC_NUM_STANDARD_PARAM)
      {
        values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
      }
      else
      {
        if(gap_debug) printf("Reading Standard parameters for %s\n", name);

        if (param[3].data.d_string[0] != '\0') { g_snprintf(gpp->val.videoname, sizeof(gpp->val.videoname), "%s", param[3].data.d_string); }
        if (param[4].data.d_int32 >= 0) { gpp->val.range_from =    param[4].data.d_int32; }
        if (param[5].data.d_int32 >= 0) { gpp->val.range_to   =    param[5].data.d_int32; }
        if (param[6].data.d_int32 > 0)  { gpp->val.vid_width  =    param[6].data.d_int32; }
        if (param[7].data.d_int32 > 0)  { gpp->val.vid_height  =   param[7].data.d_int32; }
        if (param[8].data.d_int32 > 0)  { gpp->val.vid_format  =   param[8].data.d_int32; }
        gpp->val.framerate   = param[9].data.d_float;
        gpp->val.samplerate  = param[10].data.d_int32;
        g_snprintf(gpp->val.audioname1, sizeof(gpp->val.audioname1), "%s", param[11].data.d_string);

        /* use ffmpeg specific encoder parameters (0==run with default values) */
        if (param[12].data.d_int32 == 0)
        {
          if(gap_debug) printf("Running the Encoder %s with Default Values\n", name);
        }
        else
        {
          /* try to read encoder specific params */
          p_gimp_get_data(name, epp, sizeof(GapGveFFMpegValues));
        }
        if (param[13].data.d_string[0] != '\0') { g_snprintf(gpp->val.filtermacro_file, sizeof(gpp->val.filtermacro_file), "%s", param[13].data.d_string); }
        if (param[14].data.d_string[0] != '\0') { g_snprintf(gpp->val.storyboard_file, sizeof(gpp->val.storyboard_file), "%s", param[14].data.d_string); }
        if (param[15].data.d_int32 >= 0) { gpp->val.input_mode   =    param[15].data.d_int32; }
      }

      if (values[0].data.d_status == GIMP_PDB_SUCCESS)
      {
         if (l_rc >= 0 )
         {
            l_rc = p_ffmpeg_encode(gpp);
            /* delete images in the cache
             * (the cache may have been filled while parsing
             * and processing a storyboard file)
             */
             gap_gve_story_drop_image_cache();
         }
      }
  }
  else
  {
      values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
  }

 if(l_rc < 0)
 {
    values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
 }

}       /* end run */


/* --------------------------------
 * p_gimp_get_data
 * --------------------------------
 */
static void
p_gimp_get_data(const char *key, void *buffer, gint expected_size)
{
  if(gimp_get_data_size(key) == expected_size)
  {
      if(gap_debug) printf("p_gimp_get_data: key:%s\n", key);
      gimp_get_data(key, buffer);
  }
  else
  {
     if(gap_debug)
     {
       printf("p_gimp_get_data key:%s failed\n", key);
       printf("gimp_get_data_size:%d  expected size:%d\n"
             , (int)gimp_get_data_size(key)
             , (int)expected_size);
     }
  }
}  /* end p_gimp_get_data */


/* ----------------------------------
 * gap_enc_ffmpeg_main_init_preset_params
 * ----------------------------------
 * Encoding parameter Presets (Preset Values are just a guess and are not OK, have to check ....)
 * 0 .. DivX default (ffmpeg defaults, OK)
 * 1 .. DivX Best
 * 2 .. DivX Low
 * 3 .. MPEG1 VCD
 * 4 .. MPEG1 Best
 * 5 .. MPEG2 DVD
 * 6 .. Real Video
 */
void
gap_enc_ffmpeg_main_init_preset_params(GapGveFFMpegValues *epp, gint preset_idx)
{
  gint l_idx;
  /*                                                                DivX def   best   low       VCD   MPGbest   DVD     Real */

  static gint32 tab_pass[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]              =  {     1,      1,      1,      1,      1,      1,      1 };
  static gint32 tab_audio_bitrate[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]     =  {   160,    192,     96,    128,    192,    160,     96 };
  static gint32 tab_video_bitrate[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]     =  {  5000,   5000,   1500,   3000,   6000,   6000,   1200 };
  static gint32 tab_gop_size[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]          =  {    12,     12,     96,     24,     12,     24,     12 };
  static gint32 tab_intra[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]             =  {     0,      0,      0,      0,      0,      0,      0 };
  static gint32 tab_qscale[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]            =  {     0,      0,      0,      0,      0,      1,      0 };
  static gint32 tab_qmin[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]              =  {     2,      1,     10,      0,      1,      2,     10 };
  static gint32 tab_qmax[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]              =  {    31,      4,     31,      0,      4,      8,     31 };
  static gint32 tab_qdiff[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]             =  {     3,      2,      9,      3,      2,      2,     10 };

  static float  tab_qblur[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]             =  {   0.5,     0.5,    0.5,   0.5,    0.5,    0.5,    0.5 };
  static float  tab_qcomp[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]             =  {   0.5,     0.5,    0.5,   0.5,    0.5,    0.5,    0.5 };
  static float  tab_rc_init_cplx[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]      =  {   0.0,     0.0,    0.0,   0.0,    0.0,    0.0,    0.0 };
  static float  tab_b_qfactor[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]         =  {  1.25,    1.25,   1.25,  1.25,   1.25,   1.25,   1.25 };
  static float  tab_i_qfactor[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]         =  {  -0.8,    -0.8,   -0.8,  -0.8,   -0.8,   -0.8,   -0.8 };
  static float  tab_b_qoffset[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]         =  {  1.25,    1.25,   1.25,  1.25,   1.25,   1.25,   1.25 };
  static float  tab_i_qoffset[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]         =  {   0.0,     0.0,    0.0,   0.0,    0.0,    0.0,    0.0 };

  static gint32 tab_bitrate_tol[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]       =  {  4000,   1000,   6000,      0,   1000,   2000,   5000 };
  static gint32 tab_maxrate_tol[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]       =  {     0,      0,      0,      0,      0,      0,      0 };
  static gint32 tab_minrate_tol[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]       =  {     0,      0,      0,      0,      0,      0,      0 };
  static gint32 tab_bufsize[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]           =  {     0,      0,      0,      0,      0,      0,      0 };
  static gint32 tab_motion_estimation[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS] =  {     5,      2,      1,      5,      2,      2,      1 };
  static gint32 tab_dct_algo[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]          =  {     0,      0,      0,      0,      0,      0,      0 };
  static gint32 tab_idct_algo[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]         =  {     0,      0,      0,      0,      0,      0,      0 };
  static gint32 tab_strict[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]            =  {     0,      0,      0,      0,      0,      0,      0 };
  static gint32 tab_mb_qmin[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]           =  {     2,      2,      2,      2,      2,      2,      2 };
  static gint32 tab_mb_qmax[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]           =  {    31,     31,     31,     31,     31,     31,     31 };
  static gint32 tab_mb_decision[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]       =  {     0,      1,      0,      0,      1,      1,      0 };
  static gint32 tab_aic[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]               =  {     0,      0,      0,      0,      0,      0,      0 };
  static gint32 tab_umv[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]               =  {     0,      0,      0,      0,      0,      0,      0 };

  static gint32 tab_b_frames[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]          =  {     0,      0,      0,      0,      0,      0,      0 };
  static gint32 tab_mv4[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]               =  {     0,      0,      0,      0,      0,      0,      0 };
  static gint32 tab_partitioning[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]      =  {     0,      0,      0,      0,      0,      0,      0 };
  static gint32 tab_packet_size[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]       =  {     0,      0,      0,      0,      0,      0,      0 };
  static gint32 tab_bitexact[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]          =  {     0,      0,      0,      0,      0,      0,      0 };
  static gint32 tab_aspect[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]            =  {     1,      1,      1,      1,      1,      1,      1 };
  static gint32 tab_aspect_fact[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]       =  {   0.0,    0.0,    0.0,    0.0,    0.0,    0.0,    0.0 };

  static char*  tab_format_name[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]       =  { "avi",  "avi", "avi",  "vcd", "mpeg", "vob", "rm" };
  static char*  tab_vcodec_name[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]       =  { "msmpeg4",  "msmpeg4", "msmpeg4",  "mpeg1video", "mpeg1video", "mpeg2video", "rv10" };
  static char*  tab_acodec_name[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]       =  { "mp2",  "mp2", "mp2",  "mp2", "mp2", "mp2", "ac3" };


  if(gap_debug) printf("gap_enc_ffmpeg_main_init_preset_params\n");

  l_idx = CLAMP(preset_idx, 0, GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS -1);

  g_snprintf(epp->format_name, sizeof(epp->format_name), tab_format_name[l_idx]);   /* "avi" */
  g_snprintf(epp->vcodec_name, sizeof(epp->vcodec_name), tab_vcodec_name[l_idx]);   /* "msmpeg4" */
  g_snprintf(epp->acodec_name, sizeof(epp->acodec_name), tab_acodec_name[l_idx]);   /* "mp2" */


  epp->pass                = tab_pass[l_idx];
  epp->audio_bitrate       = tab_audio_bitrate[l_idx];   /* 160 kbit */
  epp->video_bitrate       = tab_video_bitrate[l_idx];   /* 5000 kbit */
  epp->gop_size            = tab_gop_size[l_idx];        /* 12 group of pictures size (frames) */

  epp->intra               = tab_intra[l_idx];           /* 0:FALSE 1:TRUE */
  epp->qscale              = tab_qscale[l_idx];          /* 0  0..31 */
  epp->qmin                = tab_qmin[l_idx];            /* 2  0..31 */
  epp->qmax                = tab_qmax[l_idx];            /* 31  0..31 */
  epp->qdiff               = tab_qdiff[l_idx];           /* 3 0..31 */

  epp->qblur               = tab_qblur[l_idx];           /* 0.5; */
  epp->qcomp               = tab_qcomp[l_idx];           /* 0.5; */
  epp->rc_init_cplx        = tab_rc_init_cplx[l_idx];    /* 0.0; */
  epp->b_qfactor           = tab_b_qfactor[l_idx];       /* 1.25; */
  epp->i_qfactor           = tab_i_qfactor[l_idx];       /* -0.8; */
  epp->b_qoffset           = tab_b_qoffset[l_idx];       /* 1.25; */
  epp->i_qoffset           = tab_i_qoffset[l_idx];       /* 0.0; */

  epp->bitrate_tol         = tab_bitrate_tol[l_idx];     /* 4000;  kbit */
  epp->maxrate_tol         = tab_maxrate_tol[l_idx];     /* 0;     kbit */
  epp->minrate_tol         = tab_minrate_tol[l_idx];     /* 0;     kbit */
  epp->bufsize             = tab_bufsize[l_idx];         /* 0;     kbit */
  epp->motion_estimation   = tab_motion_estimation[l_idx];  /* 5   1:zero(fastest), 2:full(best), 3:log, 4:phods, 5:epzs(recommanded), 6:x1 */
  epp->dct_algo            = tab_dct_algo[l_idx];        /* 0 */
  epp->idct_algo           = tab_idct_algo[l_idx];       /* 0 */
  epp->strict              = tab_strict[l_idx];          /* 0 */
  epp->mb_qmin             = tab_mb_qmin[l_idx];         /* 2 */
  epp->mb_qmax             = tab_mb_qmax[l_idx];         /* 31 */
  epp->mb_decision         = tab_mb_decision[l_idx];     /* 0:FF_MB_DECISION_SIMPLE  1:FF_MB_DECISION_BITS 2:FF_MB_DECISION_RD */
  epp->aic                 = tab_aic[l_idx];             /* 0:FALSE 1:TRUE */
  epp->umv                 = tab_umv[l_idx];             /* 0:FALSE 1:TRUE */
  epp->b_frames            = tab_b_frames[l_idx];        /* 0:FALSE 1:TRUE */
  epp->mv4                 = tab_mv4[l_idx];             /* 0:FALSE 1:TRUE */
  epp->partitioning        = tab_partitioning[l_idx];    /* 0:FALSE 1:TRUE */
  epp->packet_size         = tab_packet_size[l_idx];
  epp->benchmark           = 0;      /* 0:FALSE 1:TRUE */
  epp->hex_dump            = 0;      /* 0:FALSE 1:TRUE */
  epp->psnr                = 0;      /* 0:FALSE 1:TRUE */
  epp->vstats              = 0;      /* 0:FALSE 1:TRUE */
  epp->bitexact            = tab_bitexact[l_idx];       /* 0:FALSE 1:TRUE */
  epp->set_aspect_ratio    = tab_aspect[l_idx];         /* 0:FALSE 1:TRUE */
  epp->factor_aspect_ratio = tab_aspect_fact[l_idx];    /* 0:auto, or width/height */
}   /* end gap_enc_ffmpeg_main_init_preset_params */


/* --------------------------------
 * p_ffmpeg_init_default_params
 * --------------------------------
 */
static void
p_ffmpeg_init_default_params(GapGveFFMpegValues *epp)
{
  g_snprintf(epp->title, sizeof(epp->title), "%s", "\0");
  g_snprintf(epp->author, sizeof(epp->author), "%s", "\0");
  g_snprintf(epp->copyright, sizeof(epp->copyright), "%s", "\0");
  g_snprintf(epp->comment, sizeof(epp->comment), "%s", "\0");
  g_snprintf(epp->passlogfile, sizeof(epp->passlogfile), DEFAULT_PASS_LOGFILENAME);

  gap_enc_ffmpeg_main_init_preset_params(epp, 0);
  epp->dont_recode_flag    = 0;      /* 0:FALSE 1:TRUE */
}   /* end p_ffmpeg_init_default_params */



/* ---------------
 * p_ffmpeg_open
 * ---------------
 */
static t_ffmpeg_handle *
p_ffmpeg_open(GapGveFFMpegGlobalParams *gpp
             , gint32 current_pass
             , long audio_samplerate
             , long audio_channels)
{
  t_ffmpeg_handle *ffh;
  GapGveFFMpegValues   *epp;
  AVCodecContext *video_enc = NULL;
  AVCodecContext *audio_enc = NULL;
  gint  size;


  if(gap_debug) printf("\np_ffmpeg_open START\n");

  epp = &gpp->evl;

  if(((gpp->val.vid_width % 2) != 0) || ((gpp->val.vid_height % 2) != 0))
  {
     printf(_("Frame width and height must be a multiple of 2\n"));
     return (NULL);
  }

  av_register_all();  /* register all fileformats and codecs before we can use th lib */

  ffh = g_malloc0(sizeof(t_ffmpeg_handle));
  ffh->vid_codec = NULL;
  ffh->aud_codec = NULL;
  ffh->big_picture_codec = avcodec_alloc_frame();


  ffh->frame_width                  = (int)gpp->val.vid_width;
  ffh->frame_height                 = (int)gpp->val.vid_height;
  ffh->frame_topBand                = 0;
  ffh->frame_bottomBand             = 0;
  ffh->frame_leftBand               = 0;
  ffh->frame_rightBand              = 0;
  ffh->frame_rate                   = gpp->val.framerate * DEFAULT_FRAME_RATE_BASE;
  ffh->video_bit_rate               = epp->video_bitrate * 1000;
  ffh->video_bit_rate_tolerance     = epp->bitrate_tol * 1000;
  ffh->video_qscale                 = epp->qscale;
  ffh->video_qmin                   = epp->qmin;
  ffh->video_qmax                   = epp->qmax;
  ffh->video_qdiff                  = epp->qdiff;
  ffh->video_qblur                  = (float)epp->qblur;
  ffh->video_qcomp                  = (float)epp->qcomp;
  ffh->video_rc_override_string     = NULL;
  ffh->video_rc_eq                  =  "tex^qComp";
  ffh->video_rc_buffer_size         = epp->bufsize;
  ffh->video_rc_buffer_aggressivity = 1.0;
  ffh->video_rc_max_rate            = epp->maxrate_tol;
  ffh->video_rc_min_rate            = epp->minrate_tol;
  ffh->video_rc_initial_cplx        = (float)epp->rc_init_cplx;
  ffh->video_b_qfactor              = (float)epp->b_qfactor;
  ffh->video_b_qoffset              = (float)epp->b_qoffset;
  ffh->video_i_qfactor              = (float)epp->i_qfactor;
  ffh->video_i_qoffset              = (float)epp->i_qoffset;
  ffh->me_method                    = epp->motion_estimation;
  ffh->video_disable                = 0;
  ffh->video_codec_id               = CODEC_ID_NONE;
  ffh->same_quality                 = 0;
  ffh->b_frames                     = epp->b_frames;
  ffh->mb_decision                  = epp->mb_decision;
  ffh->aic                          = epp->aic;
  ffh->umv                          = epp->umv;
  ffh->use_4mv                      = epp->mv4;
  ffh->workaround_bugs              = FF_BUG_AUTODETECT;
  ffh->error_resilience             = 2;
  ffh->error_concealment            = 3;
  ffh->dct_algo                     = epp->dct_algo;
  ffh->idct_algo                    = epp->idct_algo;
  ffh->strict                       = epp->strict;
  ffh->mb_qmin                      = epp->mb_qmin;
  ffh->mb_qmax                      = epp->mb_qmax;
  ffh->use_part                     = epp->partitioning;
  ffh->packet_size                  = epp->packet_size;

  ffh->gop_size                     = epp->gop_size;
  ffh->intra_only                   = epp->intra;
  ffh->audio_sample_rate            = audio_samplerate;              /* 44100; */
  ffh->audio_bit_rate               = epp->audio_bitrate * 1000;     /* 64000; */
  ffh->audio_disable                = 1;
  ffh->audio_channels               = 1;
  ffh->audio_codec_id               = CODEC_ID_NONE;

  ffh->recording_time               = 0;
  ffh->file_overwrite               = 0;
  ffh->str_title = NULL;            if (epp->title[0] != '\0')     {ffh->str_title     = &epp->title[0];}
  ffh->str_author = NULL;           if (epp->author[0] != '\0')    {ffh->str_author    = &epp->author[0];}
  ffh->str_copyright = NULL;        if (epp->copyright[0] != '\0') {ffh->str_copyright = &epp->copyright[0];}
  ffh->str_comment = NULL;          if (epp->comment[0] != '\0')   {ffh->str_comment   = &epp->comment[0];}
  ffh->do_benchmark                 = epp->benchmark;
  ffh->do_hex_dump                  = epp->hex_dump;
  ffh->do_play                      = 0;
  ffh->do_psnr                      = epp->psnr;
  ffh->do_vstats                    = epp->vstats;
  ffh->do_pass                      = 0;
  ffh->pass_logfilename = NULL;     if (epp->passlogfile[0] != '\0')   {ffh->pass_logfilename   = &epp->passlogfile[0];}
  ffh->audio_stream_copy            = 0;
  ffh->video_stream_copy            = 0;

  /* ------------ File Format  -------   */

  ffh->file_oformat = guess_format(epp->format_name, gpp->val.videoname, NULL);
  if (!ffh->file_oformat)
  {
     printf("Unknown output format: %s\n", epp->format_name);
     g_free(ffh);
     return(NULL);
  }

  if(gap_debug)
  {
    printf("AVOutputFormat ffh->file_oformat opened\n");
    printf("  name: %s\n", ffh->file_oformat->name);
    printf("  long_name: %s\n", ffh->file_oformat->long_name);
    printf("  extensions: %s\n", ffh->file_oformat->extensions);
    printf("  priv_data_size: %d\n", (int)ffh->file_oformat->priv_data_size);
    printf("  ID default audio_codec: %d\n", (int)ffh->file_oformat->audio_codec);
    printf("  ID default video_codec: %d\n", (int)ffh->file_oformat->video_codec);
    printf("  FPTR write_header: %d\n", (int)ffh->file_oformat->write_header);
    printf("  FPTR write_packet: %d\n", (int)ffh->file_oformat->write_packet);
    printf("  FPTR write_trailer: %d\n", (int)ffh->file_oformat->write_trailer);
    printf("  flags: %d\n", (int)ffh->file_oformat->flags);
    printf("\n");
  }


  ffh->output_context = g_malloc0(sizeof(AVFormatContext));
  ffh->output_context->oformat = ffh->file_oformat;
  strcpy(ffh->output_context->filename, &gpp->val.videoname[0]);



  /* ------------ Video CODEC  -------   */

  ffh->vid_codec = avcodec_find_encoder_by_name(epp->vcodec_name);
  if(!ffh->vid_codec)
  {
     printf("Unknown Video CODEC: %s\n", epp->vcodec_name);
     g_free(ffh);
     return(NULL);
  }
  if(ffh->vid_codec->type != CODEC_TYPE_VIDEO)
  {
     printf("CODEC: %s is no VIDEO CODEC!\n", epp->vcodec_name);
     g_free(ffh);
     return(NULL);
  }

  /* set stream 0 (video stream) */
  ffh->vid_stream = g_malloc0(sizeof(AVStream));
  avcodec_get_context_defaults(&ffh->vid_stream->codec);
  ffh->output_context->nb_streams = 1;  /* number of streams */
  ffh->output_context->streams[0] = ffh->vid_stream;
  ffh->vid_stream->index = 0;
  ffh->vid_stream->id = 0;        /* XXXx ? dont know how to init this ? */

  /* set Video codec context */
  ffh->vid_codec_context = &ffh->vid_stream->codec;


  /* some formats need to know about the coded_frame
   * and get the information from the AVCodecContext coded_frame Pointer
   */
  if(gap_debug) printf("(B) ffh->vid_codec_context:%d\n", (int)ffh->vid_codec_context);
  ffh->vid_codec_context->coded_frame = ffh->big_picture_codec;

  video_enc = ffh->vid_codec_context;

  video_enc->codec_type = CODEC_TYPE_VIDEO;
  video_enc->codec_id = ffh->vid_codec->id;

  video_enc->bit_rate = epp->video_bitrate * 1000;
  video_enc->bit_rate_tolerance = epp->bitrate_tol * 1000;
  video_enc->frame_rate_base = DEFAULT_FRAME_RATE_BASE;
  video_enc->frame_rate = gpp->val.framerate * DEFAULT_FRAME_RATE_BASE;
  video_enc->width = gpp->val.vid_width;
  video_enc->height = gpp->val.vid_height;

  if(!strcmp(epp->format_name, "mp4") 
  || !strcmp(epp->format_name, "mov") 
  || !strcmp(epp->format_name, "3gp"))
  {
    video_enc->flags |= CODEC_FLAG_GLOBAL_HEADER;
  }


  video_enc->aspect_ratio = 0;
  if(epp->set_aspect_ratio)
  {
    if(epp->factor_aspect_ratio == 0.0)
    {
      video_enc->aspect_ratio = (gdouble)gpp->val.vid_width 
                              / (gdouble)(MAX(1,gpp->val.vid_height));
    }
    else
    {
      video_enc->aspect_ratio = epp->factor_aspect_ratio;
    }
  }

  if(!epp->intra)  { video_enc->gop_size = epp->gop_size;  }
  else             { video_enc->gop_size = 0;              }

  if(epp->qscale)
  {
    video_enc->flags |= CODEC_FLAG_QSCALE;
    ffh->vid_stream->quality = epp->qscale;   /* video_enc->quality = epp->qscale; */
  }

  /* CODEC_FLAG_HQ no longer supported in ffmpeg-0.4.8  */
  /*
   *  if(epp->hq_settings)
   *  {
   *     video_enc->flags |= CODEC_FLAG_HQ;
   *  }
   */

  if(epp->bitexact)
  {
    /* only use bit exact algorithms (for codec testing) */
    video_enc->flags |= CODEC_FLAG_BITEXACT;
  }
  
  /* mb_decision changed in ffmpeg-0.4.8
   *  0: FF_MB_DECISION_SIMPLE
   *  1: FF_MB_DECISION_BITS
   *  2: FF_MB_DECISION_RD
   */
  video_enc->mb_decision = epp->mb_decision;

  if(epp->umv)
  {
     video_enc->flags |= CODEC_FLAG_H263P_UMV;
  }

  if(epp->aic)
  {
     video_enc->flags |= CODEC_FLAG_H263P_AIC;
  }

  if (epp->mv4)
  {
      /* video_enc->flags |= CODEC_FLAG_HQ; */  /* CODEC_FLAG_HQ no longer supported in ffmpeg-0.4.8  */
      video_enc->mb_decision = FF_MB_DECISION_BITS; /* FIXME remove */
      video_enc->flags |= CODEC_FLAG_4MV;
  }

  if(epp->partitioning)
  {
      video_enc->flags |= CODEC_FLAG_PART;
  }

  if (epp->b_frames)
  {
    if (video_enc->codec_id != CODEC_ID_MPEG4)
    {
       printf("\n### Warning: B frames encoding only supported by MPEG-4. (option ignored)\n");
    }
    else
    {
      video_enc->max_b_frames = epp->b_frames;
      video_enc->b_frame_strategy = 0;
      video_enc->b_quant_factor = 2.0;
    }
  }

  video_enc->qmin      = epp->qmin;
  video_enc->qmax      = epp->qmax;
  video_enc->max_qdiff = epp->qdiff;
  video_enc->qblur     = (float)epp->qblur;
  video_enc->qcompress = (float)epp->qcomp;
  video_enc->rc_eq     = "tex^qComp";

  video_enc->rc_override_count      =0;
  video_enc->rc_max_rate            = epp->maxrate_tol;
  video_enc->rc_min_rate            = epp->minrate_tol;
  video_enc->rc_buffer_size         = epp->bufsize;
  video_enc->rc_buffer_aggressivity = 1.0;
  video_enc->rc_initial_cplx        = (float)epp->rc_init_cplx;
  video_enc->i_quant_factor         = (float)epp->i_qfactor;
  video_enc->b_quant_factor         = (float)epp->b_qfactor;
  video_enc->i_quant_offset         = (float)epp->i_qoffset;
  video_enc->b_quant_offset         = (float)epp->b_qoffset;
  video_enc->dct_algo               = epp->dct_algo;
  video_enc->idct_algo              = epp->idct_algo;
  video_enc->strict_std_compliance  = epp->strict;
  video_enc->debug                  = 0;
  video_enc->mb_qmin                = epp->mb_qmin;
  video_enc->mb_qmax                = epp->mb_qmax;

  if(epp->packet_size)
  {
      video_enc->rtp_mode           = 1;
      video_enc->rtp_payload_size   = epp->packet_size;
  }

  if (epp->psnr) {  video_enc->flags |= CODEC_FLAG_PSNR; }

  video_enc->me_method = epp->motion_estimation;
  video_enc->pix_fmt = PIX_FMT_YUV420P;     /* PIX_FMT_YUV444P;  PIX_FMT_YUV420P;  PIX_FMT_BGR24;  PIX_FMT_RGB24; */


  if (epp->title[0] != '\0')
  {
    g_snprintf(ffh->output_context->title, sizeof(ffh->output_context->title), "%s", &epp->title[0]);
  }
  if (epp->author[0] != '\0')
  {
    g_snprintf(ffh->output_context->author, sizeof(ffh->output_context->author), "%s", &epp->author[0]);
  }
  if (epp->copyright[0] != '\0')
  {
    g_snprintf(ffh->output_context->copyright, sizeof(ffh->output_context->copyright), "%s", &epp->copyright[0]);
  }
  if (epp->comment[0] != '\0')
  {
    g_snprintf(ffh->output_context->comment, sizeof(ffh->output_context->comment), "%s", &epp->comment[0]);
  }


  /* ----------------- 2 pass handling ------------------------ */
  if (current_pass > 0)
  {
      if (current_pass == 1)
      {
          video_enc->flags |= CODEC_FLAG_PASS1;
      } else
      {
          video_enc->flags |= CODEC_FLAG_PASS2;
      }
  }
  ffh->passlog_fp = NULL;

  if (video_enc->flags & (CODEC_FLAG_PASS1 | CODEC_FLAG_PASS2))
  {
      char logfilename[1024];
      FILE *fp;
      int size;
      char *logbuffer;

      snprintf(logfilename, sizeof(logfilename), "%s.log", epp->passlogfile);
      if (video_enc->flags & CODEC_FLAG_PASS1)
      {
          fp = fopen(logfilename, "w");
          if (!fp)
          {
              perror(logfilename);
              return(NULL);
          }
          ffh->passlog_fp = fp;
      }
      else
      {
          /* read the log file */
          fp = fopen(logfilename, "r");
          if (!fp)
          {
              perror(logfilename);
              return(NULL);
          }
          fseek(fp, 0, SEEK_END);
          size = ftell(fp);
          fseek(fp, 0, SEEK_SET);
          logbuffer = g_malloc(size + 1);

          fread(logbuffer, 1, size, fp);
          fclose(fp);
          logbuffer[size] = '\0';
          video_enc->stats_in = logbuffer;
      }
  }

  /* dont know why */
  /* *(ffh->vid_stream->codec) = *video_enc; */    /* XXX(#) copy codec context */

  /* open output FILE */
  g_snprintf(ffh->output_context->filename, sizeof(ffh->output_context->filename), "%s", &gpp->val.videoname[0]);
  if (url_fopen(&ffh->output_context->pb, gpp->val.videoname, URL_WRONLY) < 0)
  {
    printf("Could not url_fopen: %s\n", gpp->val.videoname);
    return(NULL);
  }

  /* open video codec */
  if (avcodec_open(ffh->vid_codec_context, ffh->vid_codec) < 0)
  {
    printf("could not avcodec_open codec: %s\n", epp->vcodec_name);
    return(NULL);
  }

  /* ------------ Start Audio CODEC init -------   */
  if(audio_channels > 0)
  {
    if(gap_debug) printf("p_ffmpeg_open Audio CODEC start init\n");

    ffh->aud_codec = avcodec_find_encoder_by_name(epp->acodec_name);
    if(!ffh->aud_codec)
    {
       printf("Unknown Audio CODEC: %s\n", epp->acodec_name);
    }
    else
    {
      if(ffh->aud_codec->type != CODEC_TYPE_AUDIO)
      {
         printf("CODEC: %s is no VIDEO CODEC!\n", epp->acodec_name);
         ffh->aud_codec = NULL;
      }
      else
      {
        /* set stream 1 (audio stream) */
        ffh->aud_stream = g_malloc0(sizeof(AVStream));
        avcodec_get_context_defaults(&ffh->aud_stream->codec);
        ffh->output_context->nb_streams += 1;  /* number of streams */
        ffh->output_context->streams[1] = ffh->aud_stream;
        ffh->aud_stream->index = 1;
        ffh->aud_stream->id = 1;        /* XXXx ? dont know how to init this ? */
        ffh->aud_codec_context = &ffh->aud_stream->codec;

        /* set codec context */
        /*  ffh->aud_codec_context = avcodec_alloc_context();*/
        audio_enc = ffh->aud_codec_context;

        audio_enc->codec_type = CODEC_TYPE_AUDIO;
        audio_enc->codec_id = ffh->aud_codec->id;

        audio_enc->bit_rate = epp->audio_bitrate * 1000;
        audio_enc->sample_rate = audio_samplerate;  /* was read from wav file */
        audio_enc->channels = audio_channels;       /* 1=mono, 2 = stereo (from wav file) */


        /* open audio codec */
        if (avcodec_open(ffh->aud_codec_context, ffh->aud_codec) < 0)
        {
          printf("could not avcodec_open codec: %s\n", epp->acodec_name);
          ffh->aud_codec = NULL;
        }
      }
    }
  }
  /* ------------ End Audio CODEC init -------   */

  ffh->ap = g_malloc0(sizeof(AVFormatParameters));
  ffh->ap->sample_rate = audio_samplerate;
  ffh->ap->channels = audio_channels;
  ffh->ap->frame_rate_base = DEFAULT_FRAME_RATE_BASE;
  ffh->ap->frame_rate = gpp->val.framerate * DEFAULT_FRAME_RATE_BASE;
  ffh->ap->width = gpp->val.vid_width;
  ffh->ap->height = gpp->val.vid_height;

  ffh->ap->image_format = NULL;
  ffh->ap->pix_fmt = PIX_FMT_YUV420P;

  /* tv standard, NTSC, PAL, SECAM */
  switch(gpp->val.vid_format)
  {
   /* gpp->val.vid_format possible values are:
    *    0=comp., 1=PAL, 2=NTSC, 3=SECAM, 4=MAC, 5=unspec
    * FFMPEG supports PAL, NTSC and SECAM
    */
   case VID_FMT_PAL:
     ffh->ap->standard = "PAL";
     break;
   case VID_FMT_NTSC:
     ffh->ap->standard = "NTSC";
     break;
   case VID_FMT_SECAM:
     ffh->ap->standard = "SECAM";
     break;
   default:
     ffh->ap->standard = "NTSC";
     break;
  }

  if (av_set_parameters(ffh->output_context, ffh->ap) < 0)
  {
    printf("Invalid output format parameters\n");
    return(NULL);
  }

  /* ------------ media file output  -------   */
  av_write_header(ffh->output_context);

  size = gpp->val.vid_width * gpp->val.vid_height;

  /* allocate video_buffer (should be large enough for one ENCODED frame) */
  ffh->video_buffer_size =  size * 4;  /* (1024*1024); */
  ffh->video_buffer = g_malloc0(ffh->video_buffer_size);
  ffh->video_stream_index = 0;

  /* allocate yuv420_buffer (for the RAW image data YUV 4:2:0) */
  ffh->yuv420_buffer_size = size + (size / 2);
  ffh->yuv420_buffer = g_malloc0(ffh->yuv420_buffer_size);



  /* ------------ audio output  -------   */
  ffh->audio_buffer_size =  4 * MAX_AUDIO_PACKET_SIZE;
  ffh->audio_buffer = g_malloc0(ffh->audio_buffer_size);
  ffh->audio_stream_index = 1;

  if(gap_debug) printf("\np_ffmpeg_open END\n");

  return(ffh);
}  /* end p_ffmpeg_open */


/* --------------------------
 * p_ffmpeg_write_frame_chunk
 * --------------------------
 */
static int
p_ffmpeg_write_frame_chunk(t_ffmpeg_handle *ffh, gint32 encoded_size)
{
  int ret;

  ret = 0;

  if(gap_debug)
  {
     AVCodec  *codec;

     codec = ffh->vid_codec_context->codec;

     printf("p_ffmpeg_write_frame_chunk: START codec: %d\n", (int)codec);
     printf("\n-------------------------\n");
     printf("name: %s\n", codec->name);
     printf("type: %d\n", codec->type);
     printf("id: %d\n",   codec->id);
  }

  if(ffh->output_context)
  {
    if(gap_debug)  printf("before av_write_frame  encoded_size:%d\n", (int)encoded_size );

    ret = av_write_frame(ffh->output_context, ffh->video_stream_index, ffh->video_buffer, encoded_size);

    if(gap_debug)  printf("after av_write_frame  encoded_size:%d\n", (int)encoded_size );
  }

  return (ret);
}  /* end p_ffmpeg_write_frame_chunk */



/* --------------------
 * p_ffmpeg_write_frame
 * --------------------
 */
static int
p_ffmpeg_write_frame(t_ffmpeg_handle *ffh, gboolean force_keyframe)
{
  AVFrame   *big_picture_yuv;
  AVPicture *picture_yuv;
  AVPicture *picture_codec;
  int encoded_size;
  int ret;
  uint8_t  *l_convert_buffer;

  ret = 0;
  l_convert_buffer = NULL;

  if(gap_debug)
  {
     AVCodec  *codec;

     codec = ffh->vid_codec_context->codec;

     printf("p_ffmpeg_write_frame: START codec: %d\n", (int)codec);
     printf("\n-------------------------\n");
     printf("name: %s\n", codec->name);
     printf("type: %d\n", codec->type);
     printf("id: %d\n",   codec->id);
     printf("priv_data_size: %d\n",   codec->priv_data_size);
     printf("capabilities: %d\n",   codec->capabilities);
     printf("init fptr: %d\n",   (int)codec->init);
     printf("encode fptr: %d\n",   (int)codec->encode);
     printf("close fptr: %d\n",   (int)codec->close);
     printf("decode fptr: %d\n",   (int)codec->decode);

  }

  /* picture to feed the codec */
  picture_codec = (AVPicture *)ffh->big_picture_codec;

  /* source picture (YUV420) */
  big_picture_yuv = avcodec_alloc_frame();
  picture_yuv = (AVPicture *)big_picture_yuv;

  if(ffh->vid_codec_context->pix_fmt == PIX_FMT_YUV420P)
  {
    if(gap_debug) printf("USE YUV420 (no pix_fmt convert needed)\n");

    /* most of the codecs wants YUV420
      * (we can use the picture in ffh->yuv420_buffer without pix_fmt conversion
      */
    avpicture_fill(picture_codec
                  ,ffh->yuv420_buffer
                  ,PIX_FMT_YUV420P          /* PIX_FMT_RGB24, PIX_FMT_RGBA32, PIX_FMT_BGRA32 */
                  ,ffh->frame_width
                  ,ffh->frame_height
                  );
  }
  else
  {
    /* allocate buffer for image conversion large enough for for uncompressed RGBA32 colormodel */
    l_convert_buffer = g_malloc(4 * ffh->frame_width * ffh->frame_height);

    if(gap_debug)
    {
      printf("HAVE TO convert  pix_fmt: %d\n (PIX_FMT_YUV420P %d)"
            , (int)ffh->vid_codec_context->pix_fmt
	    , (int)PIX_FMT_YUV420P
	    );
    }


    /* init destination picture structure (the codec context tells us what pix_fmt is needed)
     */
     avpicture_fill(picture_codec
                  ,l_convert_buffer
                  ,ffh->vid_codec_context->pix_fmt          /* PIX_FMT_RGB24, PIX_FMT_RGBA32, PIX_FMT_BGRA32 */
                  ,ffh->frame_width
                  ,ffh->frame_height
                  );
    /* source picture (has YUV420P colormodel) */
     avpicture_fill(picture_yuv
                  ,ffh->yuv420_buffer
                  ,PIX_FMT_YUV420P
                  ,ffh->frame_width
                  ,ffh->frame_height
                  );

    /* AVFrame is the new structure introduced in FFMPEG 0.4.6,
     * (same as AVPicture but with additional members at the end)
     */

    if(gap_debug) printf("before img_convert  pix_fmt: %d  (YUV420:%d)\n", (int)ffh->vid_codec_context->pix_fmt, (int)PIX_FMT_YUV420P);

    /* convert to pix_fmt needed by the codec */
    img_convert(picture_codec, ffh->vid_codec_context->pix_fmt  /* dst */
               ,picture_yuv, PIX_FMT_BGR24                    /* src */
               ,ffh->frame_width
               ,ffh->frame_height
               );

    if(gap_debug)  printf("after img_convert\n");
  }

  /* AVFrame is the new structure introduced in FFMPEG 0.4.6,
   * (same as AVPicture but with additional members at the end)
   */
  ffh->big_picture_codec->quality = ffh->vid_stream->quality;

  if(force_keyframe)
  {
    /* TODO: howto force the encoder to write an I frame ??
     *   ffh->big_picture_codec->key_frame could be ignored
     *   because this seems to be just an information
     *   reported by the encoder.
     */
    /* ffh->vid_codec_context->key_frame = 1; */
    ffh->big_picture_codec->key_frame = 1;
  }
  else
  {
    ffh->big_picture_codec->key_frame = 0;
  }

  if(ffh->output_context)
  {
    if(gap_debug)  printf("before avcodec_encode_video\n");

    encoded_size = avcodec_encode_video(ffh->vid_codec_context
                           ,ffh->video_buffer, ffh->video_buffer_size
                           ,ffh->big_picture_codec);


    if(gap_debug)  printf("before av_write_frame  encoded_size:%d\n", (int)encoded_size );

    ret = av_write_frame(ffh->output_context, ffh->video_stream_index, ffh->video_buffer, encoded_size);
  }


  /* if we are in pass1 of a two pass encoding run, output log */
  if (ffh->passlog_fp && ffh->vid_codec_context->stats_out)
  {
    fprintf(ffh->passlog_fp, "%s", ffh->vid_codec_context->stats_out);
  }

  if(gap_debug)  printf("before free picture structures\n");

  g_free(big_picture_yuv);

  if(l_convert_buffer) g_free(l_convert_buffer);

  return (ret);
}  /* end p_ffmpeg_write_frame */


/* -------------------------
 * p_ffmpeg_write_audioframe
 * -------------------------
 */
static int
p_ffmpeg_write_audioframe(t_ffmpeg_handle *ffh, guchar *audio_buf, int frame_bytes)
{
  int encoded_size;
  int ret;

  ret = 0;

  if(ffh->output_context)
  {
    encoded_size = avcodec_encode_audio(ffh->aud_codec_context
                           ,ffh->audio_buffer, ffh->audio_buffer_size
                           ,(short *)audio_buf);
    ret = av_write_frame(ffh->output_context, ffh->audio_stream_index, ffh->audio_buffer, encoded_size);
  }
  return(ret);
}  /* end p_ffmpeg_write_audioframe */


/* ---------------
 * p_ffmpeg_close
 * ---------------
 */
static void
p_ffmpeg_close(t_ffmpeg_handle *ffh)
{

  if(ffh->big_picture_codec)
  {
    g_free(ffh->big_picture_codec);
    ffh->big_picture_codec = NULL;
  }

  if(ffh->output_context)
  {
    /* ?? TODO check if should close the code first ?? */
    av_write_trailer(ffh->output_context);
    url_fclose(&ffh->output_context->pb);
  }

  if(ffh->vid_codec_context)
  {
    avcodec_close(ffh->vid_codec_context);
    ffh->vid_codec_context = NULL;
    /* do not attempt to free ffh->vid_codec_context (it points to ffh->vid_stream->codec) */
  }

  if(ffh->aud_codec_context)
  {
    avcodec_close(ffh->aud_codec_context);
    ffh->aud_codec_context = NULL;
    /* do not attempt to free ffh->aud_codec_context (it points to ffh->aud_stream->codec) */
  }

  if (ffh->passlog_fp)
  {
    fclose(ffh->passlog_fp);
    ffh->passlog_fp = NULL;
  }


  if(ffh->video_buffer)
  {
     g_free(ffh->video_buffer);
     ffh->video_buffer = NULL;
  }
  if(ffh->yuv420_buffer)
  {
     g_free(ffh->yuv420_buffer);
     ffh->yuv420_buffer = NULL;
  }
  if(ffh->audio_buffer)
  {
     g_free(ffh->audio_buffer);
     ffh->audio_buffer = NULL;
  }
}  /* end p_ffmpeg_close */


/* ============================================================================
 * p_ffmpeg_encode
 *    The main "productive" routine
 *    ffmpeg encoding of anim frames, based on ffmpeg lib (by Fabrice Bellard)
 *    Audio encoding is Optional.
 *    (wav_audiofile must be provided in that case)
 *
 * returns   value >= 0 if all went ok
 *           (or -1 on error)
 * ============================================================================
 */
static gint
p_ffmpeg_encode(GapGveFFMpegGlobalParams *gpp)
{
  GapGveFFMpegValues   *epp;
  t_ffmpeg_handle     *ffh;
  GapGveStoryVidHandle        *l_vidhand = NULL;
  gint32        l_tmp_image_id = -1;
  gint32        l_layer_id = -1;
  GimpDrawable *l_drawable = NULL;
  long          l_cur_frame_nr;
  long          l_step, l_begin, l_end;
  gdouble       l_percentage, l_percentage_step;
  int           l_rc;

  FILE *l_fp_inwav = NULL;
  gint32 datasize;
  gint32 audio_size = 0;
  gint32 audio_stereo = 0;
  long  l_sample_rate = 22050;
  long  l_channels = 2;
  long  l_bytes_per_sample = 4;
  long  l_bits = 16;
  long  l_samples = 0;


  gint32 wavsize = 0; /* Data size of the wav file */
  long audio_margin = 8192; /* The audio chunk size */
  long audio_filled_100 = 0;
  long audio_per_frame_x100 = -1;
  long frames_per_second_x100 = 2500;
  gdouble frames_per_second_x100f = 2500;
  char databuffer[300000]; /* For transferring audio data */


  epp = &gpp->evl;

  if(gap_debug)
  {
     printf("p_ffmpeg_encode: START\n");
     printf("  videoname: %s\n", gpp->val.videoname);
     printf("  audioname1: %s\n", gpp->val.audioname1);
     printf("  basename: %s\n", gpp->ainfo.basename);
     printf("  extension: %s\n", gpp->ainfo.extension);
     printf("  range_from: %d\n", (int)gpp->val.range_from);
     printf("  range_to: %d\n", (int)gpp->val.range_to);
     printf("  framerate: %f\n", (float)gpp->val.framerate);
     printf("  samplerate: %d\n", (int)gpp->val.samplerate);
     printf("  vid_width: %d\n", (int)gpp->val.vid_width);
     printf("  vid_height: %d\n", (int)gpp->val.vid_height);
     printf("  image_ID: %d\n", (int)gpp->val.image_ID);
     printf("  storyboard_file: %s\n\n", gpp->val.storyboard_file);
     printf("  input_mode: %d\n", gpp->val.input_mode);

     printf("  acodec_name: %s\n", epp->acodec_name);
     printf("  vcodec_name: %s\n", epp->vcodec_name);
     printf("  format_name: %s\n", epp->format_name);
  }

  l_rc = 0;
  l_layer_id = -1;

  /* make list of frameranges (input frames to feed the encoder) */
  { gint32 l_total_framecount;
  l_vidhand = gap_gve_story_open_vid_handle (gpp->val.input_mode
                                       ,gpp->val.image_ID
				       ,gpp->val.storyboard_file
				       ,gpp->ainfo.basename
                                       ,gpp->ainfo.extension
                                       ,gpp->val.range_from
                                       ,gpp->val.range_to
                                       ,&l_total_framecount
                                       );
  }

  /* TODO check for overwrite */


  if (gap_debug) printf("Creating ffmpeg file.\n");


  /* check and open audio wave file */
  l_fp_inwav = NULL;
  if(0 == gap_audio_wav_file_check( gpp->val.audioname1
                                , &l_sample_rate
                                , &l_channels
                                , &l_bytes_per_sample
                                , &l_bits
                                , &l_samples
                                ))
  {
    /* l_bytes_per_sample: 8bitmono: 1, 16bit_mono: 2, 8bitstereo: 2, 16bitstereo: 4 */
    wavsize = l_samples * l_bytes_per_sample;
    audio_size = l_bits;                          /*  16 or 8 */
    audio_stereo = (l_channels ==2) ? TRUE : FALSE;

    /* open WAVE file and set position to the 1.st audio databyte */
    l_fp_inwav = gap_audio_wav_open_seek_data(gpp->val.audioname1);
  }
  else
  {
    l_channels = 0;
  }


  /* OPEN the video file for write (create)
   *  successful open returns initialized FFMPEG handle structure
   */
  ffh = p_ffmpeg_open(gpp, 0, l_sample_rate, l_channels);
  if(ffh == NULL)
  {
    l_rc = -1;
  }

  if(l_fp_inwav)
  {
    ffh->audio_disable = 0; /* enable AUDIO */
  }

  l_percentage = 0.0;
  if(gpp->val.run_mode == GIMP_RUN_INTERACTIVE)
  {
    gimp_progress_init(_("FFMPEG Video Encoding .."));
  }


  /* Calculations for encoding the sound
   */
  frames_per_second_x100f = (100.0 * gpp->val.framerate) + 0.5;
  frames_per_second_x100 = frames_per_second_x100f;
  audio_per_frame_x100 = (100 * 100 * l_sample_rate * l_bytes_per_sample) / MAX(1,frames_per_second_x100);

  /* ?? dont sure if to use fix audo_margin of 8192 or
   * if its ok to set audio_margin to audio_per_frame
   */
  /* audio_margin = audio_per_frame_x100 / 100; */
  if(ffh->aud_codec_context)
  {
     /* the audio codec gives us the correct audio frame size, in samples */
     if (gap_debug) printf("Codec ontext frame_size:%d  channels:%d\n"
                           , (int)ffh->aud_codec_context->frame_size
                           , (int)ffh->aud_codec_context->channels
                           );

      audio_margin = ffh->aud_codec_context->frame_size * 2 * ffh->aud_codec_context->channels;
  }

  if(gap_debug) printf("audio_margin: %d\n", (int)audio_margin);


  /* special setup (makes it possible to code sequences backwards)
   * (NOTE: Audio is NEVER played backwards)
   */
  if(gpp->val.range_from > gpp->val.range_to)
  {
    l_step  = -1;     /* operate in descending (reverse) order */
    l_percentage_step = 1.0 / ((1.0 + gpp->val.range_from) - gpp->val.range_to);

  }
  else
  {
    l_step  = 1;      /* operate in ascending order */
    l_percentage_step = 1.0 / ((1.0 + gpp->val.range_to) - gpp->val.range_from);
  }
  l_begin = gpp->val.range_from;
  l_end   = gpp->val.range_to;

  l_cur_frame_nr = l_begin;
  while(l_rc >= 0)
  {
    gboolean l_fetch_ok;
    gboolean l_force_keyframe;
    gint32   l_video_frame_chunk_size;

    /* must fetch the frame into gimp_image  */
    /* load the current frame image, and transform (flatten, convert to RGB, scale, macro, etc..) */

    if(gap_debug) printf("\nFFenc: before gap_gve_story_fetch_composite_image_or_chunk\n");

    l_fetch_ok = gap_gve_story_fetch_composite_image_or_chunk(l_vidhand
                                           , l_cur_frame_nr
                                           , (gint32)  gpp->val.vid_width
                                           , (gint32)  gpp->val.vid_height
                                           , gpp->val.filtermacro_file
                                           , &l_layer_id           /* output */
                                           , &l_tmp_image_id       /* output */
                                           , epp->dont_recode_flag
                                           , epp->vcodec_name
                                           , &l_force_keyframe
                                           , ffh->video_buffer
                                           , &l_video_frame_chunk_size
                                           , ffh->video_buffer_size    /* IN max size */
                                           );

    if(gap_debug) printf("\nFFenc: after gap_gve_story_fetch_composite_image_or_chunk image_id:%d layer_id:%d\n", (int)l_tmp_image_id , (int) l_layer_id);

    /* this block is done foreach handled video frame */
    if(l_fetch_ok)
    {

      if (l_video_frame_chunk_size > 0)
      {
        /* dont recode, just copy video chunk to output videofile */
        p_ffmpeg_write_frame_chunk(ffh, l_video_frame_chunk_size);
      }
      else   /* encode one VIDEO FRAME */
      {
        l_drawable = gimp_drawable_get (l_layer_id);
        if (gap_debug) printf("DEBUG: %s encoding frame %d\n", epp->vcodec_name, (int)l_cur_frame_nr);


        /* fill the yuv420_buffer with current frame image data */
        gap_gve_raw_YUV420P_drawable_encode(l_drawable, ffh->yuv420_buffer);

        /* store the compressed video frame */
        if (gap_debug) printf("GAP_FFMPEG: Writing frame nr. %d\n", (int)l_cur_frame_nr);

        p_ffmpeg_write_frame(ffh, l_force_keyframe);

        /* destroy the tmp image */
        gimp_image_delete(l_tmp_image_id);
      }

      /* encode AUDIO FRAME (audio packet for playbacktime of one frame) */
      /* As long as there is a video frame, "fill" the fake audio buffer */
      if (l_fp_inwav)
      {
        audio_filled_100 += audio_per_frame_x100;
      }

      /* Now "empty" the fake audio buffer, until it goes under the margin again */
      while ((audio_filled_100 >= (audio_margin * 100)) && (wavsize > 0))
      {
        if (gap_debug) printf("Audio processing: Audio buffer at %ld bytes.\n", audio_filled_100);
        if (wavsize >= audio_margin)
        {
            datasize = fread(databuffer, 1, audio_margin, l_fp_inwav);
            if (datasize != audio_margin)
            {
              printf("Warning: Read from wav file failed. (non-critical)\n");
            }
            wavsize -= audio_margin;
        }
        else
        {
            datasize = fread(databuffer, 1, wavsize, l_fp_inwav);
            if (datasize != wavsize)
            {
              printf("Warning: Read from wav file failed. (non-critical)\n");
            }
            wavsize = 0;
        }

        if (gap_debug) printf("Now encoding audio frame datasize:%d, audio codec:%s\n", (int)datasize, epp->acodec_name);

        /****** AVI_write_audio(l_avifile, databuffer, datasize); *****/
        p_ffmpeg_write_audioframe(ffh, databuffer, datasize);

        audio_filled_100 -= audio_margin * 100;
      }
    }
    else  /* if fetch_ok */
    {
      l_rc = -1;
    }


    l_percentage += l_percentage_step;
    if(gap_debug) printf("PROGRESS: %f\n", (float) l_percentage);
    if(gpp->val.run_mode == GIMP_RUN_INTERACTIVE)
    {
      gimp_progress_update (l_percentage);
    }

    /* advance to next frame */
    if((l_cur_frame_nr == l_end) || (l_rc < 0))
    {
       break;
    }
    l_cur_frame_nr += l_step;

  }  /* end loop foreach frame */

  if(ffh != NULL)
  {
    p_ffmpeg_close(ffh);
    g_free(ffh);
  }

  if(l_fp_inwav)
  {
    fclose(l_fp_inwav);
  }

  return l_rc;
}    /* end p_ffmpeg_encode */
