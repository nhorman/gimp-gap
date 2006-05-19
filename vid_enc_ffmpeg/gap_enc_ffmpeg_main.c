/*
- TODO:
  - dont_recode_flag  is still experimental code
    but now partly works, but sometimes does not produce clean output videos
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
 * version 2.1.0a;  2005.07.16   hof: base support for encoding of multiple tracks
 *                                    video is still limited to 1 track
 *                                    multiple audio tracks are possible
 *                                    when a playlist (file containing filenames of audiofiles)
 *                                    is passed rather than a single input audiofile
 * version 2.1.0a;  2005.03.27   hof: support ffmpeg  LIBAVCODEC_BUILD  >= 4744,
 *                                    changed Noninteractive PDB-API (use paramfile)
 *                                    changed presets (are still a wild guess)
 * version 2.1.0a;  2004.11.02   hof: added patch from pippin (allows compile with FFMPEG-0.4.0.9pre1)
 *                               support for the incompatible older (stable) FFMPEG 0.4.8 version is available
 *                               via precompiler checks HAVE_OLD_FFMPEG_0408
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


#include <errno.h>

#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>


#include "gap-intl.h"

#include "gap_libgapvidutil.h"
#include "gap_libgimpgap.h"
#include "gap_enc_ffmpeg_main.h"
#include "gap_enc_ffmpeg_gui.h"
#include "gap_enc_ffmpeg_par.h"

#include "gap_audio_wav.h"

/* FFMPEG defaults */
#define DEFAULT_PASS_LOGFILENAME "ffmpeg2pass"
#define MAX_AUDIO_PACKET_SIZE 16384


/* define some Constant values
 * (needed just in case we are compiling with older ffmpeg releases)
 */
#ifdef FF_PROFILE_UNKNOWN
#define GAP_FF_PROFILE_UNKNOWN FF_PROFILE_UNKNOWN
#else
#define GAP_FF_PROFILE_UNKNOWN -99
#endif

#ifdef FF_LEVEL_UNKNOWN
#define GAP_FF_LEVEL_UNKNOWN FF_LEVEL_UNKNOWN
#else
#define GAP_FF_LEVEL_UNKNOWN -99
#endif

#ifdef FF_CMP_DCTMAX
#define GAP_FF_CMP_DCTMAX FF_CMP_DCTMAX
#else
#define GAP_FF_CMP_DCTMAX -99
#endif


#define MAX_VIDEO_STREAMS 1
#define MAX_AUDIO_STREAMS 16

typedef struct t_audio_work
{
  FILE *fp_inwav;
  char *referred_wavfile;   /* filename of the reffereed wavfile, extended to absolute path */
  gint32 wavsize; /* Data size of the wav file */
  long audio_margin; /*8192 The audio chunk size (8192) */
  long audio_filled_100;
  long audio_per_frame_x100;
  long frames_per_second_x100;
  gdouble frames_per_second_x100f;
  char *databuffer; /* 300000 dyn. allocated bytes for transferring audio data */

  long  sample_rate;
  long  channels;
  long  bytes_per_sample;
  long  bits;
  long  samples;

} t_audio_work;


typedef struct t_awk_array
{
  gint          audio_tracks;           /* == max_ast */
  t_audio_work  awk[MAX_AUDIO_STREAMS];
} t_awk_array;


typedef struct t_ffmpeg_video
{
 int              video_stream_index;
 AVStream        *vid_stream;
 AVCodecContext  *vid_codec_context;
 AVCodec         *vid_codec;
 AVFrame         *big_picture_codec;
 uint8_t         *yuv420_buffer;
 int              yuv420_buffer_size;
 uint8_t         *video_buffer;
 int              video_buffer_size;
 uint8_t         *video_dummy_buffer;
 int              video_dummy_buffer_size;
 uint8_t         *yuv420_dummy_buffer;
 int              yuv420_dummy_buffer_size;
 FILE            *passlog_fp;
} t_ffmpeg_video;

typedef struct t_ffmpeg_audio
{
 int              audio_stream_index;
 AVStream        *aud_stream;
 AVCodecContext  *aud_codec_context;
 AVCodec         *aud_codec;
 uint8_t         *audio_buffer;
 int              audio_buffer_size;
} t_ffmpeg_audio;




typedef struct t_ffmpeg_handle
{
 AVFormatContext *output_context;
 AVOutputFormat  *file_oformat;
 AVFormatParameters *ap;

 t_ffmpeg_video  vst[MAX_VIDEO_STREAMS];
 t_ffmpeg_audio  ast[MAX_AUDIO_STREAMS];

 int max_vst;   /* used video streams,  0 <= max_vst <= MAX_VIDEO_STREAMS */
 int max_ast;   /* used audio streams,  0 <= max_ast <= MAX_AUDIO_STREAMS */

 int frame_width;
 int frame_height;
 int frame_topBand;
 int frame_bottomBand;
 int frame_leftBand;
 int frame_rightBand;
 int frame_rate;
 int video_bit_rate;
 int video_bit_rate_tolerance;
 float video_qscale;
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
 int audio_bit_rate;
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
 int64_t frame_duration_pts;

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

static void              p_init_audio_workdata(t_awk_array *awp);
static void              p_open_inwav_and_buffers(t_awk_array *awp
                        	      , const char *wavefile
                        	      , gint ii
				      , long sample_rate
				      , long channels
				      , long bytes_per_sample
				      , long bits
				      , long samples
				      );
static void              p_sound_precalculations(t_ffmpeg_handle *ffh
                		      , t_awk_array *awp
                		      , GapGveFFMpegGlobalParams *gpp
				      );
static void              p_process_audio_frame(t_ffmpeg_handle *ffh, t_awk_array *awp);
static void              p_close_audio_input_files(t_awk_array *awp);
static void              p_open_audio_input_files(t_awk_array *awp, GapGveFFMpegGlobalParams *gpp);

static void              p_ffmpeg_open_init(t_ffmpeg_handle *ffh, GapGveFFMpegGlobalParams *gpp);
static gboolean          p_init_video_codec(t_ffmpeg_handle *ffh
        			      , gint ii
        			      , GapGveFFMpegGlobalParams *gpp
        			      , gint32 current_pass);

static gboolean          p_init_and_open_audio_codec(t_ffmpeg_handle *ffh
        			      , gint ii
        			      , GapGveFFMpegGlobalParams *gpp
        			      , long audio_samplerate
        			      , long audio_channels);
static t_ffmpeg_handle * p_ffmpeg_open(GapGveFFMpegGlobalParams *gpp
                                      , gint32 current_pass
				      , t_awk_array *awp
                                      , gint video_tracks
                                      );
static int    p_ffmpeg_write_frame_chunk(t_ffmpeg_handle *ffh, gint32 encoded_size, gint vid_track);
static int    p_ffmpeg_write_frame(t_ffmpeg_handle *ffh, gboolean force_keyframe, gint vid_track);
static int    p_ffmpeg_write_audioframe(t_ffmpeg_handle *ffh, guchar *audio_buf, int frame_bytes, gint aud_track);
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
    {GIMP_PDB_STRING, "audfile", "optional audiodata file .wav must contain uncompressed 16 bit samples."
                                 " pass empty string if no audiodata should be included."
				 " optional you can pass a textfile as audiofile. this textfile must contain "
				 " one filename of a .wav file per line and allows encoding of "
				 " multiple audio tracks because each of the specified audiofiles "
				 " is encoded as ist own audiostream (works only if the selected multimedia fileformat "
				 " has multiple audio track support. For example multilingual MPEG2 .vob files "
				 " do support multiple audio streams)"},
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

    {GIMP_PDB_STRING, "ffmpeg_encode_parameter_file", "name of an ffmpeg encoder parameter file."
                                                      " Such a file can be created via save button in the"
						      " INTERACTIVE runmode"},


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
                         " (also known as FFMPEG encoder)."
                         " The (optional) audiodata must be RIFF WAVEfmt (.wav) file."
                         " .wav files can be mono (1) or stereo (2channels) audiodata and must be 16bit uncompressed."
                         " IMPORTANT:  Non-interactive callers should first call "
                         "\"" GAP_PLUGIN_NAME_FFMPEG_PARAMS  "\""
                         " to set encoder specific paramters, then set the use_rest parameter to 1 to use them."),
                         "Wolfgang Hofer (hof@gimp.org)",
                         "Wolfgang Hofer",
                         GAP_VERSION_WITH_DATE,
                         NULL,                      /* has no Menu entry, just a PDB interface */
                         "RGB*, INDEXED*, GRAY*",
                         GIMP_PLUGIN,
                         nargs_ffmpeg_enc, nreturn_vals,
                         args_ffmpeg_enc, return_vals);



  gimp_install_procedure(GAP_PLUGIN_NAME_FFMPEG_PARAMS,
                         _("Set Parameters for GAP ffmpeg video encoder Plugin"),
                         _("This plugin sets ffmpeg specific video encoding parameters."
			   " Non-interactive callers must provide a parameterfile,"
			   " Interactive calls provide a dialog window to specify and optionally save the parameters."),
                         "Wolfgang Hofer (hof@gimp.org)",
                         "Wolfgang Hofer",
                         GAP_VERSION_WITH_DATE,
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
                         GAP_VERSION_WITH_DATE,
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
           /* get parameters from file in NON_INTERACTIVE mode */
           if(param[2].data.d_string)
           {
             g_snprintf(gpp->ffpar_filename, sizeof(gpp->ffpar_filename), "%s", param[2].data.d_string);
	     if(g_file_test(gpp->ffpar_filename, G_FILE_TEST_EXISTS))
	     {
               gap_ffpar_get(gpp->ffpar_filename, &gpp->evl);
               l_set_it = TRUE;
	     }
	     else
	     {
               values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
               l_set_it = FALSE;
	     }
           }
	   else
	   {
             values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
             l_set_it = FALSE;
	   }

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
 * 3 .. DivX MSMPEG default
 * 4 .. MPEG1 VCD
 * 5 .. MPEG1 Best
 * 6 .. MPEG2 SVCD
 * 7 .. MPEG2 DVD
 * 8 .. Real Video default
 */
void
gap_enc_ffmpeg_main_init_preset_params(GapGveFFMpegValues *epp, gint preset_idx)
{
  gint l_idx;
  /*                                                                        DivX def      best       low   MS-def         VCD	MPGbest      SVCD 	DVD 	 Real */
  static gint32 tab_ntsc_width[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]        =  {     0,        0,        0,  	0,        352,        0,      480,      720,	    0 };
  static gint32 tab_ntsc_height[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]       =  {     0,        0,        0,  	0,        240,        0,      480,      480,	    0 };

  static gint32 tab_pal_width[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]         =  {     0,        0,        0,  	0,        352,        0,      480,      720,	    0 };
  static gint32 tab_pal_height[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]        =  {     0,        0,        0,  	0,        288,        0,      576,      576,	    0 };

  static gint32 tab_pass[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]              =  {     1,        1,        1,  	1,          1,        1,	1,	  1,	    1 };
  static gint32 tab_audio_bitrate[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]     =  {   160,      192,       96,     160,        224,      192,      224,	192,	  160 };
  static gint32 tab_video_bitrate[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]     =  {   200,     1500,      150,     200,       1150,     6000,     2040,     6000,	 1200 };
  static gint32 tab_gop_size[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]          =  {    12,       12,       96,      12,         18,       12,       18,	 18,	   18 };
  static gint32 tab_intra[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]             =  {     0,        0,        0,  	0,          0,        0,	0,	  0,	    0 };

//static float  tab_qscale[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]            =  {     2,        1,        6,  	2,          0,        1,	1,	  1,	    2 };
  static float  tab_qscale[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]            =  {     2,        1,        6,  	2,          1,        1,	1,	  1,	    2 };

  static gint32 tab_qmin[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]              =  {     2,        2,        8,  	2,          2,        2,	2,	  2,	    2 };
  static gint32 tab_qmax[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]              =  {    31,        8,       31,      31,         31,        8,       16,	  8,	   31 };
  static gint32 tab_qdiff[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]             =  {     3,        2,        9,  	3,          3,        2,	2,	  2,	    3 };

  static float  tab_qblur[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]             =  {   0.5,       0.5,      0.5,    0.5,        0.5,      0.5,      0.5,	0.5,	  0.5 };
  static float  tab_qcomp[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]             =  {   0.5,       0.5,      0.5,    0.5,        0.5,      0.5,      0.5,	0.5,	  0.5 };
  static float  tab_rc_init_cplx[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]      =  {   0.0,       0.0,      0.0,    0.0,        0.0,      0.0,      0.0,	0.0,	  0.0 };
  static float  tab_b_qfactor[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]         =  {  1.25,      1.25,     1.25,   1.25,       1.25,     1.25,     1.25,     1.25,	 1.25 };
  static float  tab_i_qfactor[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]         =  {  -0.8,      -0.8,     -0.8,   -0.8,       -0.8,     -0.8,     -0.8,     -0.8,	 -0.8 };
  static float  tab_b_qoffset[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]         =  {  1.25,      1.25,     1.25,   1.25,       1.25,     1.25,     1.25,     1.25,	 1.25 };
  static float  tab_i_qoffset[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]         =  {   0.0,       0.0,      0.0,    0.0,        0.0,      0.0,      0.0,	0.0,	  0.0 };

  static gint32 tab_bitrate_tol[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]       =  {  4000,     6000,     2000,    4000,          0,     4000,     2000,     2000,	 5000 };
  static gint32 tab_maxrate_tol[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]       =  {     0,        0,        0,  	0,       1150,        0,     2516,     9000,	    0 };
  static gint32 tab_minrate_tol[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]       =  {     0,        0,        0,  	0,       1150,        0,	0,	  0,	    0 };
  static gint32 tab_bufsize[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]           =  {     0,        0,        0,  	0,         40,        0,      224,	224,	    0 };

  static gint32 tab_motion_estimation[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS] =  {     5,        5,        5,  	5,          5,        5,	5,	  5,	    5 };

  static gint32 tab_dct_algo[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]          =  {     0,        0,        0,  	0,          0,        0,	0,	  0,	    0 };
  static gint32 tab_idct_algo[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]         =  {     0,        0,        0,  	0,          0,        0,	0,	  0,	    0 };
  static gint32 tab_strict[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]            =  {     0,        0,        0,  	0,          0,        0,	0,	  0,	    0 };
  static gint32 tab_mb_qmin[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]           =  {     2,        2,        8,  	2,          2,        2,	2,	  2,	    2 };
  static gint32 tab_mb_qmax[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]           =  {    31,       18,       31,      31,         31,       18,       31,	 31,	   31 };
  static gint32 tab_mb_decision[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]       =  {     0,        0,        0,  	0,          0,        0,	0,	  0,	    0 };
  static gint32 tab_aic[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]               =  {     0,        0,        0,  	0,          0,        0,	0,	  0,	    0 };
  static gint32 tab_umv[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]               =  {     0,        0,        0,  	0,          0,        0,	0,	  0,	    0 };

  static gint32 tab_b_frames[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]          =  {     2,        2,        4,  	0,          0,        0,	2,	  2,	    0 };
  static gint32 tab_mv4[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]               =  {     0,        0,        0,  	0,          0,        0,	0,	  0,	    0 };
  static gint32 tab_partitioning[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]      =  {     0,        0,        0,  	0,          0,        0,	0,	  0,	    0 };
  static gint32 tab_packet_size[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]       =  {     0,        0,        0,  	0,          0,        0,	0,	  0,	    0 };
  static gint32 tab_bitexact[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]          =  {     0,        0,        0,  	0,          0,        0,	0,	  0,	    0 };
  static gint32 tab_aspect[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]            =  {     1,        1,        1,  	1,          1,        1,	1,	  1,	    1 };
  static gint32 tab_aspect_fact[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]       =  {   0.0,      0.0,      0.0,     0.0,        0.0,      0.0,      0.0,	0.0,	  0.0 };

  static char*  tab_format_name[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]       =  { "avi",    "avi",    "avi",     "avi",     "vcd",   "mpeg",   "svcd",    "vob",     "rm" };
  static char*  tab_vcodec_name[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]       =  { "mpeg4", "mpeg4", "mpeg4", "msmpeg4", "mpeg1video", "mpeg1video", "mpeg2video", "mpeg2video", "rv10" };
  static char*  tab_acodec_name[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]       =  { "mp2",     "mp2",   "mp2",     "mp2",      "mp2",    "mp2",    "mp2",   "ac3",    "ac3" };

  static gint32 tab_mux_rate[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]          =  {     0,         0,     	0,	0,     1411200,       0,       0,   10080000,        0 };
  static gint32 tab_mux_packet_size[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]   =  {     0,         0,     	0,	0,        2324,    2324,    2324,       2048,        0 };
  static float  tab_mux_preload[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]       =  {   0.5,       0.5,      0.5,    0.5,        0.44,     0.5,     0.5,        0.5,      0.5 };
  static float  tab_mux_max_delay[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]     =  {   0.7,       0.7,      0.7,    0.7,         0.7,     0.7,     0.7,        0.7,      0.7 };
  static gint32 tab_use_scann_offset[GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS]  =  {     0,         0,     	0,	0,           0,       0,       1,          0,        0 };


  l_idx = CLAMP(preset_idx, 0, GAP_GVE_FFMPEG_PRESET_MAX_ELEMENTS -1);
  if(gap_debug)
  {
    printf("gap_enc_ffmpeg_main_init_preset_params L_IDX:%d\n", (int)l_idx);
  }

  g_snprintf(epp->format_name, sizeof(epp->format_name), tab_format_name[l_idx]);   /* "avi" */
  g_snprintf(epp->vcodec_name, sizeof(epp->vcodec_name), tab_vcodec_name[l_idx]);   /* "msmpeg4" */
  g_snprintf(epp->acodec_name, sizeof(epp->acodec_name), tab_acodec_name[l_idx]);   /* "mp2" */


  epp->pass                = tab_pass[l_idx];
  epp->audio_bitrate       = tab_audio_bitrate[l_idx];   /* 160 kbit */
  epp->video_bitrate       = tab_video_bitrate[l_idx];   /* 5000 kbit */
  epp->gop_size            = tab_gop_size[l_idx];        /* 12 group of pictures size (frames) */

  epp->intra               = tab_intra[l_idx];           /* 0:FALSE 1:TRUE */
  epp->qscale              = tab_qscale[l_idx];          /* 0  0.01 .. 255 (0..31 in old fmpeg 0.4.8) */
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
  epp->b_frames            = tab_b_frames[l_idx];        /* 0-8: number of B-frames per GOP */
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


  /* new parms (added after ffmpeg 0.4.8)
   * TODO: findout valid value ranges and provide GUI widgets for those options
   */
  epp->thread_count                 = 1;
  epp->mb_cmp                       = GAP_GVE_FFMPEG_CMP_00_SAD;
  epp->ildct_cmp                    = GAP_GVE_FFMPEG_CMP_08_VSAD;
  epp->sub_cmp                      = GAP_GVE_FFMPEG_CMP_00_SAD;
  epp->cmp                          = GAP_GVE_FFMPEG_CMP_00_SAD;
  epp->pre_cmp                      = GAP_GVE_FFMPEG_CMP_00_SAD;
  epp->pre_me                       = 0;
  epp->lumi_mask                    = 0;
  epp->dark_mask                    = 0;
  epp->scplx_mask                   = 0;
  epp->tcplx_mask                   = 0;
  epp->p_mask                       = 0;
  epp->qns                          = 0;

  epp->use_ss                       = 0;
  epp->use_aiv                      = 0;
  epp->use_obmc                     = 0;
  epp->use_loop                     = 0;
  epp->use_alt_scan                 = 0;
  epp->use_trell                    = 0;
  epp->use_mv0                      = 0;
  epp->do_normalize_aqp             = 0;
  epp->use_scan_offset              = tab_use_scann_offset[l_idx];
  epp->closed_gop                   = 0;
  epp->strict_gop                   = 0;
  epp->use_qpel                     = 0;
  epp->use_qprd                     = 0;
  epp->use_cbprd                    = 0;
  epp->do_interlace_dct             = 0;
  epp->do_interlace_me              = 0;
  epp->no_output                    = 0;
  epp->video_lmin                   = 2*GAP_GVE_FF_QP2LAMBDA;
  epp->video_lmax                   = 31*GAP_GVE_FF_QP2LAMBDA;
  epp->video_mb_lmin                = 2*GAP_GVE_FF_QP2LAMBDA;
  epp->video_mb_lmax                = 31*GAP_GVE_FF_QP2LAMBDA;
  epp->video_lelim                  = 0;
  epp->video_celim                  = 0;
  epp->video_intra_quant_bias       = FF_DEFAULT_QUANT_BIAS;
  epp->video_inter_quant_bias       = FF_DEFAULT_QUANT_BIAS;
  epp->me_threshold                 = 0;
  epp->mb_threshold                 = 0;
  epp->intra_dc_precision           = 8;


  epp->error_rate                   = 0;
  epp->noise_reduction              = 0;
  epp->sc_threshold                 = 0;
  epp->me_range                     = 0;
  epp->coder                        = 0;
  epp->context                      = 0;
  epp->predictor                    = 0;
  epp->video_profile                = GAP_FF_PROFILE_UNKNOWN;
  epp->video_level                  = GAP_FF_LEVEL_UNKNOWN;
  epp->nsse_weight                  = 8;
  epp->subpel_quality               = 8;
  epp->frame_skip_threshold         = 0;
  epp->frame_skip_factor            = 0;
  epp->frame_skip_exp               = 0;
  epp->frame_skip_cmp               = GAP_FF_CMP_DCTMAX;

  epp->mux_rate                     = tab_mux_rate[l_idx];
  epp->mux_packet_size              = tab_mux_packet_size[l_idx];
  epp->mux_preload                  = tab_mux_preload[l_idx];
  epp->mux_max_delay                = tab_mux_max_delay[l_idx];

  epp->ntsc_width                   = tab_ntsc_width[l_idx];
  epp->ntsc_height                  = tab_ntsc_height[l_idx];
  epp->pal_width                    = tab_pal_width[l_idx];
  epp->pal_height                   = tab_pal_height[l_idx];

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




/* ---------------------
 * p_init_audio_workdata
 * ---------------------
 */
static void
p_init_audio_workdata(t_awk_array *awp)
{
  gint ii;

  awp->audio_tracks = 0;

  /* clear all elements of the audio stream and audio work tables */
  for (ii=0; ii < MAX_AUDIO_STREAMS; ii++)
  {
    awp->awk[ii].fp_inwav = NULL;
    awp->awk[ii].referred_wavfile = NULL;
    awp->awk[ii].wavsize = 0;
    awp->awk[ii].audio_margin = 8192;
    awp->awk[ii].audio_filled_100 = 0;
    awp->awk[ii].audio_per_frame_x100 = -1;
    awp->awk[ii].frames_per_second_x100 = 2500;
    awp->awk[ii].frames_per_second_x100f = 2500;
    awp->awk[ii].databuffer = NULL;
    awp->awk[ii].sample_rate = 22050;
    awp->awk[ii].channels = 2;
    awp->awk[ii].bytes_per_sample = 4;
    awp->awk[ii].bits = 16;
    awp->awk[ii].samples = 0;

  }
}  /* end p_init_audio_workdata */




/* ------------------------
 * p_open_inwav_and_buffers
 * ------------------------
 * open input wavefile for the audio stram specified by its index ii
 * allocate buffers and init audio relevant informations
 */
static void
p_open_inwav_and_buffers(t_awk_array *awp
                        , const char *wavefile
                        , gint ii
			, long sample_rate
			, long channels
			, long bytes_per_sample
			, long bits
			, long samples
			)
{
  FILE *fp_inwav;

  /* l_bytes_per_sample: 8bitmono: 1, 16bit_mono: 2, 8bitstereo: 2, 16bitstereo: 4 */
  awp->awk[ii].wavsize = samples * bytes_per_sample;
  awp->awk[ii].bytes_per_sample = bytes_per_sample;
  awp->awk[ii].samples = samples;
  awp->awk[ii].sample_rate = sample_rate;
  awp->awk[ii].channels = channels;
  awp->awk[ii].bits = bits;

  /* open WAVE file and set position to the 1.st audio databyte */
  fp_inwav = gap_audio_wav_open_seek_data(wavefile);

  awp->awk[ii].fp_inwav = fp_inwav;
  awp->awk[ii].audio_margin = 8192;
  awp->awk[ii].audio_filled_100 = 0;
  awp->awk[ii].audio_per_frame_x100 = -1;
  awp->awk[ii].frames_per_second_x100 = 2500;
  awp->awk[ii].frames_per_second_x100f = 2500;
  awp->awk[ii].databuffer = g_malloc(300000);

}  /* end p_open_inwav_and_buffers */



/* -----------------------
 * p_sound_precalculations
 * -----------------------
 */
static void
p_sound_precalculations(t_ffmpeg_handle *ffh
                      , t_awk_array *awp
                      , GapGveFFMpegGlobalParams *gpp)
{
  gint ii;

  for (ii=0; ii < MIN(ffh->max_ast, awp->audio_tracks); ii++)
  {
    /* Calculations for encoding the sound
     */
    awp->awk[ii].frames_per_second_x100f = (100.0 * gpp->val.framerate) + 0.5;
    awp->awk[ii].frames_per_second_x100 = awp->awk[ii].frames_per_second_x100f;
    awp->awk[ii].audio_per_frame_x100 = (100 * 100 * awp->awk[ii].sample_rate
                                         * awp->awk[ii].bytes_per_sample)
                                         / MAX(1,awp->awk[ii].frames_per_second_x100);

    /* ?? dont sure if to use fix audo_margin of 8192 or
     * if its ok to set audio_margin to audio_per_frame
     */
    /* audio_margin = audio_per_frame_x100 / 100; */
    if(ffh->ast[ii].aud_codec_context)
    {
       /* the audio codec gives us the correct audio frame size, in samples */
       if (gap_debug) printf("Codec ontext frame_size[%d]:%d  channels:%d\n"
                             , (int)ii
                             , (int)ffh->ast[ii].aud_codec_context->frame_size
                             , (int)ffh->ast[ii].aud_codec_context->channels
                             );

       awp->awk[ii].audio_margin = ffh->ast[ii].aud_codec_context->frame_size * 2 * ffh->ast[ii].aud_codec_context->channels;
    }

    if(gap_debug) printf("audio_margin[%d]: %d\n", (int)ii, (int)awp->awk[ii].audio_margin);
  }


}  /* end p_sound_precalculations */


/* ----------------------
 * p_process_audio_frame
 * ----------------------
 * encode + write audio one frame (for each audio track)
 * requires an already opened multimedia output file context
 * and audio codecs initilalized and opened for each input
 * track. (ffh->ast)
 * Further all audio buffers have to be allocated
 * and all audio input file handles must be opened read
 *
 * The read position in those file will be advanced
 * when the content of the corresponding input buffers
 * falls down below the margin.
 */
static void
p_process_audio_frame(t_ffmpeg_handle *ffh, t_awk_array *awp)
{
  gint ii;
  gint32 datasize;

  /* foreach audio_track
   * (number of audioinput tracks should always match
   *  the max audio streams)
   */
  for (ii=0; ii < MIN(ffh->max_ast, awp->audio_tracks); ii++)
  {
      if (gap_debug)
      {
        printf("p_process_audio_frame: audio treack: ii=%d.\n", (int)ii);
      }
      /* encode AUDIO FRAME (audio packet for playbacktime of one frame) */
      /* As long as there is a video frame, "fill" the fake audio buffer */
      if (awp->awk[ii].fp_inwav)
      {
        awp->awk[ii].audio_filled_100 += awp->awk[ii].audio_per_frame_x100;
      }

      /* Now "empty" the audio buffer, until it goes under the margin again */
      while ((awp->awk[ii].audio_filled_100 >= (awp->awk[ii].audio_margin * 100)) && (awp->awk[ii].wavsize > 0))
      {
        if (gap_debug) printf("p_process_audio_frame: Audio buffer at %ld bytes.\n", awp->awk[ii].audio_filled_100);
        if (awp->awk[ii].wavsize >= awp->awk[ii].audio_margin)
        {
            datasize = fread(awp->awk[ii].databuffer
                           , 1
                           , awp->awk[ii].audio_margin
                           , awp->awk[ii].fp_inwav
                           );
            if (datasize != awp->awk[ii].audio_margin)
            {
              printf("Warning: Read from wav file failed. (non-critical)\n");
            }
            awp->awk[ii].wavsize -= awp->awk[ii].audio_margin;
        }
        else
        {
            datasize = fread(awp->awk[ii].databuffer
                           , 1
                           , awp->awk[ii].wavsize
                           , awp->awk[ii].fp_inwav
                           );
            if (datasize != awp->awk[ii].wavsize)
            {
              printf("Warning: Read from wav file failed. (non-critical)\n");
            }
            awp->awk[ii].wavsize = 0;
        }

        if (gap_debug) printf("Now encoding audio frame datasize:%d\n", (int)datasize);

        p_ffmpeg_write_audioframe(ffh, awp->awk[ii].databuffer, datasize, ii /* aud_track */);

        awp->awk[ii].audio_filled_100 -= awp->awk[ii].audio_margin * 100;
      }
  }


}  /* end p_process_audio_frame */


/* -------------------------
 * p_close_audio_input_files
 * -------------------------
 * close input files for all audio tracks
 * and free their databuffer
 */
static void
p_close_audio_input_files(t_awk_array *awp)
{
  gint ii;

  for (ii=0; ii < awp->audio_tracks; ii++)
  {
    if (awp->awk[ii].fp_inwav)
    {
      fclose(awp->awk[ii].fp_inwav);
      awp->awk[ii].fp_inwav = NULL;
    }

    if(awp->awk[ii].referred_wavfile)
    {
      g_free(awp->awk[ii].referred_wavfile);
      awp->awk[ii].referred_wavfile = NULL;
    }


    if(awp->awk[ii].databuffer)
    {
      g_free(awp->awk[ii].databuffer);
      awp->awk[ii].databuffer = NULL;
    }

  }
}  /* end p_close_audio_input_files */


/* -------------------------
 * p_open_audio_input_files
 * -------------------------
 *
 */
static void
p_open_audio_input_files(t_awk_array *awp, GapGveFFMpegGlobalParams *gpp)
{
  long  l_bytes_per_sample = 4;
  long  l_sample_rate = 22050;
  long  l_channels = 2;
  long  l_bits = 16;
  long  l_samples = 0;
  gint  ii = 0;

  /* check and open audio wave file */
  if(0 == gap_audio_wav_file_check( gpp->val.audioname1
                                , &l_sample_rate
                                , &l_channels
                                , &l_bytes_per_sample
                                , &l_bits
                                , &l_samples
                                ))
  {
    p_open_inwav_and_buffers(awp
                            , gpp->val.audioname1
			    , ii
                            , l_sample_rate
			    , l_channels
			    , l_bytes_per_sample
			    , l_bits
			    , l_samples
			    );
    awp->audio_tracks = 1;
  }
  else
  {
    FILE *l_fp;
    char         l_buf[4000];

    l_channels = 0;
    awp->audio_tracks = 0;

    /* check if gpp->val.audioname1
     * is a playlist referring to more than one inputfile
     * and try to open those input wavefiles
     * (but limited upto MAX_AUDIO_STREAMS input wavefiles)
     */
    l_fp = fopen(gpp->val.audioname1, "r");
    if(l_fp)
    {
      ii = 0;
      while(NULL != fgets(l_buf, sizeof(l_buf)-1, l_fp))
      {
        if((l_buf[0] == '#') || (l_buf[0] == '\n') || (l_buf[0] == '\0'))
	{
	  continue;  /* skip comment lines, and empty lines */
	}

        l_buf[sizeof(l_buf) -1] = '\0';  /* make sure we have a terminated string */
        gap_file_chop_trailingspace_and_nl(&l_buf[0]);

	if(ii < MAX_AUDIO_STREAMS)
	{
          int    l_rc;

          awp->awk[ii].referred_wavfile = gap_file_make_abspath_filename(&l_buf[0], gpp->val.audioname1);

          l_rc = gap_audio_wav_file_check(awp->awk[ii].referred_wavfile
                                , &l_sample_rate
                                , &l_channels
                                , &l_bytes_per_sample
                                , &l_bits
                                , &l_samples
                                );
          if(l_rc == 0)
          {
            p_open_inwav_and_buffers(awp
                            , awp->awk[ii].referred_wavfile
			    , ii
                            , l_sample_rate
			    , l_channels
			    , l_bytes_per_sample
			    , l_bits
			    , l_samples
			    );

	    ii++;
            awp->audio_tracks = ii;
	  }
	  else
	  {
            g_message(_("The file: %s\n"
	              "has unexpect content that will be ignored\n"
		      "you should specify an audio file in RIFF WAVE fileformat\n"
		      "or a textfile containing filenames of such audio files")
		     , gpp->val.audioname1
		     );
	    break;
	  }
	}
	else
	{
          g_message(_("The file: %s\n"
	              "contains too much audio-input tracks\n"
		      "(only %d tracks are used, the rest are ignored)")
		   , gpp->val.audioname1
		   , (int) MAX_AUDIO_STREAMS
		   );
	  break;
	}
	l_buf[0] = '\0';
      }

      fclose(l_fp);
    }
  }

}  /* end p_open_audio_input_files */







/* ------------------
 * p_ffmpeg_open_init
 * ------------------
 */
static void
p_ffmpeg_open_init(t_ffmpeg_handle *ffh, GapGveFFMpegGlobalParams *gpp)
{
  gint ii;
  GapGveFFMpegValues   *epp;

  epp = &gpp->evl;

  /* clear all elements of the video stream tables */
  for (ii=0; ii < MAX_VIDEO_STREAMS; ii++)
  {
    ffh->vst[ii].video_stream_index = 0;
    ffh->vst[ii].vid_stream = NULL;
    ffh->vst[ii].vid_codec_context = NULL;
    ffh->vst[ii].vid_codec = NULL;
    ffh->vst[ii].big_picture_codec = avcodec_alloc_frame();
    ffh->vst[ii].yuv420_buffer = NULL;
    ffh->vst[ii].yuv420_buffer_size = 0;
    ffh->vst[ii].video_buffer = NULL;
    ffh->vst[ii].video_buffer_size = 0;
    ffh->vst[ii].video_dummy_buffer = NULL;
    ffh->vst[ii].video_dummy_buffer_size = 0;
    ffh->vst[ii].yuv420_dummy_buffer = NULL;
    ffh->vst[ii].yuv420_dummy_buffer_size = 0;
    ffh->vst[ii].passlog_fp = NULL;
  }


  /* clear all elements of the audio stream and audio work tables */
  for (ii=0; ii < MAX_AUDIO_STREAMS; ii++)
  {
    ffh->ast[ii].audio_stream_index = 0;
    ffh->ast[ii].aud_stream = NULL;
    ffh->ast[ii].aud_codec_context = NULL;
    ffh->ast[ii].aud_codec = NULL;
    ffh->ast[ii].audio_buffer = NULL;
    ffh->ast[ii].audio_buffer_size = 0;
  }

  /* initialize common things */
  ffh->frame_width                  = (int)gpp->val.vid_width;
  ffh->frame_height                 = (int)gpp->val.vid_height;
  ffh->frame_topBand                = 0;
  ffh->frame_bottomBand             = 0;
  ffh->frame_leftBand               = 0;
  ffh->frame_rightBand              = 0;
  ffh->frame_duration_pts           = (1.0 / gpp->val.framerate) * AV_TIME_BASE;
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
  ffh->video_rc_max_rate            = epp->maxrate_tol * 1000;
  ffh->video_rc_min_rate            = epp->minrate_tol * 1000;
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
  ffh->audio_bit_rate               = epp->audio_bitrate * 1000;     /* 64000; */
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

}  /* end p_ffmpeg_open_init */

/* ------------------
 * p_init_video_codec
 * ------------------
 * init settings for the video codec
 * (just prepare for opening, but dont open yet)
 */
static gboolean
p_init_video_codec(t_ffmpeg_handle *ffh
             , gint ii
             , GapGveFFMpegGlobalParams *gpp
             , gint32 current_pass)
{
  GapGveFFMpegValues   *epp;
  AVCodecContext *video_enc = NULL;

  epp = &gpp->evl;


  ffh->vst[ii].vid_codec = avcodec_find_encoder_by_name(epp->vcodec_name);
  if(!ffh->vst[ii].vid_codec)
  {
     printf("Unknown Video CODEC: %s\n", epp->vcodec_name);
     g_free(ffh);
     return(FALSE); /* error */
  }
  if(ffh->vst[ii].vid_codec->type != CODEC_TYPE_VIDEO)
  {
     printf("CODEC: %s is no VIDEO CODEC!\n", epp->vcodec_name);
     g_free(ffh);
     return(FALSE); /* error */
  }


  /* set stream 0 (video stream) */

#ifdef HAVE_OLD_FFMPEG_0408
  ffh->output_context->nb_streams += 1;  /* number of streams */
  ffh->vst[ii].vid_stream = g_malloc0(sizeof(AVStream));
  avcodec_get_context_defaults(&ffh->vst[ii].vid_stream->codec);
  ffh->vst[ii].vid_stream->index = ii;
  ffh->vst[ii].vid_stream->id = ii;        /* XXXx ? dont know how to init this ? = or ii ? */
  ffh->output_context->streams[ii] = ffh->vst[ii].vid_stream;
#else
  ffh->vst[ii].vid_stream = av_new_stream(ffh->output_context, ii /* vid_stream_index */ );

  if(gap_debug)
  {
    printf("p_ffmpeg_open ffh->vst[%d].vid_stream: %d   ffh->output_context->streams[%d]: %d nb_streams:%d\n"
      ,(int)ii
      ,(int)ffh->vst[ii].vid_stream
      ,(int)ii
      ,(int)ffh->output_context->streams[ii]
      ,(int)ffh->output_context->nb_streams
      );
  }
#endif


  /* set Video codec context */
  ffh->vst[ii].vid_codec_context = &ffh->vst[ii].vid_stream->codec;

  /* some formats need to know about the coded_frame
   * and get the information from the AVCodecContext coded_frame Pointer
   */
  if(gap_debug) printf("(B) ffh->vst[ii].vid_codec_context:%d\n", (int)ffh->vst[ii].vid_codec_context);
  ffh->vst[ii].vid_codec_context->coded_frame = ffh->vst[ii].big_picture_codec;

  video_enc = ffh->vst[ii].vid_codec_context;

  video_enc->codec_type = CODEC_TYPE_VIDEO;
  video_enc->codec_id = ffh->vst[ii].vid_codec->id;

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


#ifdef HAVE_OLD_FFMPEG_0408
  if(epp->qscale)
  {
    video_enc->flags |= CODEC_FLAG_QSCALE;
    ffh->vst[ii].vid_stream->quality = epp->qscale;   /* video_enc->quality = epp->qscale; */
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
#else

  if(epp->qscale)
  {
    video_enc->flags |= CODEC_FLAG_QSCALE;
    ffh->vst[ii].vid_stream->quality = FF_QP2LAMBDA * epp->qscale;   /* video_enc->quality = epp->qscale; */
  }


  /**
   * AVRational sample aspect ratio (0 if unknown).
   * numerator and denominator must be relative prime and smaller then 256 for some video standards
   * - encoding: set by user.
   * - decoding: set by lavc.
   */
  video_enc->sample_aspect_ratio.num = 0;
  video_enc->sample_aspect_ratio.den = 1;
  if(epp->set_aspect_ratio)
  {
    gdouble l_aspect_factor;
    gdouble l_dbl;


    if(epp->factor_aspect_ratio == 0.0)
    {
      l_aspect_factor = (gdouble)gpp->val.vid_width / (gdouble)(MAX(1,gpp->val.vid_height));
    }
    else
    {
      l_aspect_factor = epp->factor_aspect_ratio;
    }

    /* dont understand why to multiply aspect by height and divide by width
     * but seen this in ffmpeg.c sourcecode that seems to work
     * A typical 720x576 frame
     *   with  4:3 format will result in NUM:16  DEN:15
     *   with 16:9 format will result in NUM: 64 DEN:45
     */
    l_dbl = l_aspect_factor * (gdouble)gpp->val.vid_height / (gdouble)MAX(1,gpp->val.vid_width);

    video_enc->sample_aspect_ratio = av_d2q(l_dbl, 255);

    if(gap_debug)
    {
      printf("ASPECT ratio (w/h): %f av_d2q NUM:%d  DEN:%d\n"
            ,(float)l_aspect_factor
	    ,(int)video_enc->sample_aspect_ratio.num
            ,(int)video_enc->sample_aspect_ratio.den
	    );
    }

  }

  /* the following options were added after ffmpeg 0.4.8 */

  video_enc->thread_count   = epp->thread_count;
  video_enc->mb_cmp         = epp->mb_cmp;
  video_enc->ildct_cmp      = epp->ildct_cmp;
  video_enc->me_sub_cmp     = epp->sub_cmp;
  video_enc->me_cmp         = epp->cmp;
  video_enc->me_pre_cmp     = epp->pre_cmp;
  video_enc->pre_me         = epp->pre_me;
  video_enc->lumi_masking   = epp->lumi_mask;
  video_enc->dark_masking   = epp->dark_mask;
  video_enc->spatial_cplx_masking    = epp->scplx_mask;
  video_enc->temporal_cplx_masking   = epp->tcplx_mask;
  video_enc->p_masking               = epp->p_mask;
  video_enc->quantizer_noise_shaping = epp->qns;
#endif

  if(!epp->intra)  { video_enc->gop_size = epp->gop_size;  }
  else             { video_enc->gop_size = 0;              }


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

#ifdef HAVE_OLD_FFMPEG_0408
#else
  if (epp->use_ss)
  {
    video_enc->flags |= CODEC_FLAG_H263P_SLICE_STRUCT;
  }

  if (epp->use_aiv)
  {
    video_enc->flags |= CODEC_FLAG_H263P_AIV;
  }

  if (epp->use_obmc)
  {
      video_enc->flags |= CODEC_FLAG_OBMC;
  }
  if (epp->use_loop)
  {
      video_enc->flags |= CODEC_FLAG_LOOP_FILTER;
  }

  if (epp->use_alt_scan)
  {
    video_enc->flags |= CODEC_FLAG_ALT_SCAN;
  }
  if (epp->use_trell)
  {
    video_enc->flags |= CODEC_FLAG_TRELLIS_QUANT;
  }
  if (epp->use_mv0)
  {
    video_enc->flags |= CODEC_FLAG_MV0;
  }
  if (epp->do_normalize_aqp)
  {
    video_enc->flags |= CODEC_FLAG_NORMALIZE_AQP;
  }
  if (epp->use_scan_offset)
  {
    video_enc->flags |= CODEC_FLAG_SVCD_SCAN_OFFSET;
  }
  if (epp->closed_gop)
  {
    video_enc->flags |= CODEC_FLAG_CLOSED_GOP;
  }
#ifdef HAVE_FULL_FFMPEG
  if (epp->strict_gop)
  {
    video_enc->flags2 |= CODEC_FLAG2_STRICT_GOP;
  }
  if (epp->no_output)
  {
    video_enc->flags2 |= CODEC_FLAG2_NO_OUTPUT;
  }
#endif
  if (epp->use_qpel)
  {
    video_enc->flags |= CODEC_FLAG_QPEL;
  }
  if (epp->use_qprd)
  {
    video_enc->flags |= CODEC_FLAG_QP_RD;
  }
  if (epp->use_cbprd)
  {
    video_enc->flags |= CODEC_FLAG_CBP_RD;
  }
  if (epp->do_interlace_dct)
  {
    video_enc->flags |= CODEC_FLAG_INTERLACED_DCT;
  }
  if (epp->do_interlace_me)
  {
    video_enc->flags |= CODEC_FLAG_INTERLACED_ME;
  }
#endif


  if(epp->aic)
  {
     video_enc->flags |= CODEC_FLAG_H263P_AIC;
  }

  if (epp->mv4)
  {
      /* video_enc->flags |= CODEC_FLAG_HQ; */  /* CODEC_FLAG_HQ no longer supported in ffmpeg-0.4.8  */
#ifdef HAVE_OLD_FFMPEG_0408
      video_enc->mb_decision = FF_MB_DECISION_BITS; /* FIXME remove */
#endif
      video_enc->flags |= CODEC_FLAG_4MV;
  }

  if(epp->partitioning)
  {
      video_enc->flags |= CODEC_FLAG_PART;
  }

  if ((epp->b_frames > 0) && (!epp->intra))
  {
    /* hof: TODO: here we should check if the selected codec
     * has the ability to encode B_frames
     * (dont know how to do yet)
     *
     * as workaround i do check for the MSMPEG4 codec ID's
     * because these codecs do crash (at least the version of the ffmpeg-0.4.8 release)
     * when b_frames setting > 0 is used
     */
    switch(video_enc->codec_id)
    {
      case CODEC_ID_MSMPEG4V1:
      case CODEC_ID_MSMPEG4V2:
      case CODEC_ID_MSMPEG4V3:
	if(gpp->val.run_mode == GIMP_RUN_NONINTERACTIVE)
	{
          printf("\n### Warning: B frames encoding not supported by selected codec. (option ignored)\n");
	}
	else
	{
          g_message("Warning: B frames encoding not supported by selected codec. (option ignored)\n");
	}
        break;
      default:
	video_enc->max_b_frames = epp->b_frames;
	video_enc->b_frame_strategy = 0;
	video_enc->b_quant_factor = 2.0;
        break;
    }
  }

  video_enc->qmin      = epp->qmin;
  video_enc->qmax      = epp->qmax;
  video_enc->max_qdiff = epp->qdiff;
  video_enc->qblur     = (float)epp->qblur;
  video_enc->qcompress = (float)epp->qcomp;
  video_enc->rc_eq     = "tex^qComp";

  video_enc->rc_override_count      =0;
  video_enc->rc_max_rate            = epp->maxrate_tol * 1000;
  video_enc->rc_min_rate            = epp->minrate_tol * 1000;
  video_enc->rc_buffer_size         = epp->bufsize * 8*1024;
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

#ifdef HAVE_OLD_FFMPEG_0408
#else
  video_enc->rc_initial_buffer_occupancy = video_enc->rc_buffer_size * 3/4;
  video_enc->lmin                   = epp->video_lmin;
  video_enc->lmax                   = epp->video_lmax;
  video_enc->rc_qsquish             = 1.0;  //experimental, (can be removed)
  video_enc->luma_elim_threshold    = epp->video_lelim;
  video_enc->chroma_elim_threshold  = epp->video_celim;

  video_enc->intra_quant_bias       = epp->video_intra_quant_bias;
  video_enc->inter_quant_bias       = epp->video_inter_quant_bias;

  video_enc->me_threshold           = epp->me_threshold;
  if(epp->me_threshold)
  {
    video_enc->debug |= FF_DEBUG_MV;
  }

  video_enc->mb_threshold           = epp->mb_threshold;
  video_enc->intra_dc_precision     = epp->intra_dc_precision - 8;

  video_enc->error_rate             = epp->error_rate;
  video_enc->noise_reduction        = epp->noise_reduction;
  video_enc->scenechange_threshold  = epp->sc_threshold;
  video_enc->me_range               = epp->me_range;
  video_enc->coder_type             = epp->coder;
  video_enc->context_model          = epp->context;
  video_enc->prediction_method      = epp->predictor;

  video_enc->nsse_weight            = epp->nsse_weight;
  video_enc->me_subpel_quality      = epp->subpel_quality;

#ifdef HAVE_FULL_FFMPEG
  video_enc->mb_lmin                = epp->video_mb_lmin;
  video_enc->mb_lmax                = epp->video_mb_lmax;
  video_enc->profile                = epp->video_profile;
  video_enc->level                  = epp->video_level;
  video_enc->frame_skip_threshold   = epp->frame_skip_threshold;
  video_enc->frame_skip_factor      = epp->frame_skip_factor;
  video_enc->frame_skip_exp         = epp->frame_skip_exp;
  video_enc->frame_skip_cmp         = epp->frame_skip_cmp;
#endif

#endif

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
  ffh->vst[ii].passlog_fp = NULL;

  if (video_enc->flags & (CODEC_FLAG_PASS1 | CODEC_FLAG_PASS2))
  {
      char logfilename[1024];
      FILE *fp;
      int size;
      char *logbuffer;
      gint l_errno;

      snprintf(logfilename, sizeof(logfilename), "%s.log", epp->passlogfile);
      if (video_enc->flags & CODEC_FLAG_PASS1)
      {
          fp = fopen(logfilename, "w");
          l_errno = errno;
          if (!fp)
          {
	      if(gpp->val.run_mode == GIMP_RUN_NONINTERACTIVE)
	      {
                perror(logfilename);
	      }
	      else
	      {
		g_message(_("Could not create pass logfile:"
			   "'%s'"
			   "%s")
			   ,logfilename
			   ,g_strerror (l_errno) );
	      }
              return(FALSE);  /* error */
          }
          ffh->vst[ii].passlog_fp = fp;
      }
      else
      {
          /* read the log file */
          fp = fopen(logfilename, "r");
          l_errno = errno;
          if (!fp)
          {
	      if(gpp->val.run_mode == GIMP_RUN_NONINTERACTIVE)
	      {
                perror(logfilename);
	      }
	      else
	      {
		g_message(_("Could not open pass logfile:"
			   "'%s'"
			   "%s")
			   ,logfilename
			   ,g_strerror (l_errno) );
	      }
              return(FALSE);  /* error */
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
  /* *(ffh->vst[ii].vid_stream->codec) = *video_enc; */    /* XXX(#) copy codec context */


  return (TRUE); /* OK */
}  /* end p_init_video_codec */


/* ---------------------------
 * p_init_and_open_audio_codec
 * ---------------------------
 */
static gboolean
p_init_and_open_audio_codec(t_ffmpeg_handle *ffh
             , gint ii
             , GapGveFFMpegGlobalParams *gpp
             , long audio_samplerate
             , long audio_channels)
{
  GapGveFFMpegValues   *epp;
  AVCodecContext *audio_enc = NULL;

  epp = &gpp->evl;

  /* ------------ Start Audio CODEC init -------   */
  if(audio_channels > 0)
  {
    if(gap_debug) printf("p_ffmpeg_open Audio CODEC start init\n");

    ffh->ast[ii].aud_codec = avcodec_find_encoder_by_name(epp->acodec_name);
    if(!ffh->ast[ii].aud_codec)
    {
       printf("Unknown Audio CODEC: %s\n", epp->acodec_name);
    }
    else
    {
      if(ffh->ast[ii].aud_codec->type != CODEC_TYPE_AUDIO)
      {
         printf("CODEC: %s is no VIDEO CODEC!\n", epp->acodec_name);
         ffh->ast[ii].aud_codec = NULL;
      }
      else
      {
        gint ast_index;

        /* audio stream indexes start after last video stream */
        ast_index = ffh->max_vst + ii;

        /* set stream 1 (audio stream) */
#ifdef HAVE_OLD_FFMPEG_0408
        ffh->output_context->nb_streams += 1;  /* number of streams */
        ffh->ast[ii].aud_stream = g_malloc0(sizeof(AVStream));
        avcodec_get_context_defaults(&ffh->ast[ii].aud_stream->codec);
        ffh->ast[ii].aud_stream->index = ast_index;
        ffh->ast[ii].aud_stream->id = ast_index;        /* XXXx ? dont know how to init this ? */
        ffh->output_context->streams[ast_index] = ffh->ast[ii].aud_stream;
#else
        ffh->ast[ii].aud_stream = av_new_stream(ffh->output_context, ast_index /* aud_stream_index */ );
#endif
        ffh->ast[ii].aud_codec_context = &ffh->ast[ii].aud_stream->codec;

        /* set codec context */
        /*  ffh->ast[ii].aud_codec_context = avcodec_alloc_context();*/
        audio_enc = ffh->ast[ii].aud_codec_context;

        audio_enc->codec_type = CODEC_TYPE_AUDIO;
        audio_enc->codec_id = ffh->ast[ii].aud_codec->id;

        audio_enc->bit_rate = epp->audio_bitrate * 1000;
        audio_enc->sample_rate = audio_samplerate;  /* was read from wav file */
        audio_enc->channels = audio_channels;       /* 1=mono, 2 = stereo (from wav file) */


        /* open audio codec */
        if (avcodec_open(ffh->ast[ii].aud_codec_context, ffh->ast[ii].aud_codec) < 0)
        {
	  if(gpp->val.run_mode == GIMP_RUN_NONINTERACTIVE)
	  {
            printf("could not avcodec_open audio-codec: %s\n", epp->acodec_name);
	  }
	  else
	  {
	    g_message("could not open audio codec: %s\n", epp->acodec_name);
	  }
          ffh->ast[ii].aud_codec = NULL;
        }

#ifdef HAVE_OLD_FFMPEG_0408
#else
        /* the following options were added after ffmpeg 0.4.8 */
        audio_enc->thread_count = 1;
        audio_enc->strict_std_compliance = epp->strict;
#endif


      }
    }
  }

  return (TRUE); /* OK */

} /* end p_init_and_open_audio_codec */



/* ---------------
 * p_ffmpeg_open
 * ---------------
 * master open procedure
 */
static t_ffmpeg_handle *
p_ffmpeg_open(GapGveFFMpegGlobalParams *gpp
             , gint32 current_pass
	     , t_awk_array *awp
             , gint video_tracks
	     )
{
  t_ffmpeg_handle *ffh;
  GapGveFFMpegValues   *epp;
  gint ii;

  if(gap_debug) printf("\np_ffmpeg_open START\n");

  epp = &gpp->evl;

  if(((gpp->val.vid_width % 2) != 0) || ((gpp->val.vid_height % 2) != 0))
  {
    if(gpp->val.run_mode == GIMP_RUN_NONINTERACTIVE)
    {
      printf(_("Frame width and height must be a multiple of 2\n"));
    }
    else
    {
      g_message(_("Frame width and height must be a multiple of 2\n"));
    }
    return (NULL);
  }

  ffh = g_malloc0(sizeof(t_ffmpeg_handle));
  ffh->max_vst = MIN(MAX_VIDEO_STREAMS, video_tracks);
  ffh->max_ast = MIN(MAX_AUDIO_STREAMS, awp->audio_tracks);

  av_register_all();  /* register all fileformats and codecs before we can use the lib */

  p_ffmpeg_open_init(ffh, gpp);

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


#ifdef HAVE_OLD_FFMPEG_0408
  ffh->output_context = g_malloc0(sizeof(AVFormatContext));
#else
  ffh->output_context = av_alloc_format_context();
#endif
  ffh->output_context->oformat = ffh->file_oformat;
  strcpy(ffh->output_context->filename, &gpp->val.videoname[0]);


  ffh->output_context->nb_streams = 0;  /* number of streams */

  /* ------------ video codec -------------- */

  for (ii=0; ii < ffh->max_vst; ii++)
  {
    gboolean codec_ok;

    codec_ok = p_init_video_codec(ffh, ii, gpp, current_pass);
    if(!codec_ok)
    {
      g_free(ffh);
      return(NULL);
    }
  }


  /* open output FILE */
  g_snprintf(ffh->output_context->filename, sizeof(ffh->output_context->filename), "%s", &gpp->val.videoname[0]);
  if (url_fopen(&ffh->output_context->pb, gpp->val.videoname, URL_WRONLY) < 0)
  {
    gint l_errno;

    l_errno = errno;
    g_message(_("Could not create videofile:"
	       "'%s'"
	       "%s")
	       ,gpp->val.videoname
	       ,g_strerror (l_errno) );
    return(NULL);
  }

  /* open video codec(s) */
  for (ii=0; ii < ffh->max_vst; ii++)
  {
    if (avcodec_open(ffh->vst[ii].vid_codec_context, ffh->vst[ii].vid_codec) < 0)
    {
      if(gpp->val.run_mode == GIMP_RUN_NONINTERACTIVE)
      {
        printf("could not avcodec_open video-codec: %s\n", epp->vcodec_name);
      }
      else
      {
        g_message("could not open video codec: %s\n", epp->vcodec_name);
      }
      return(NULL);
    }
  }

  /* ------------ audio codec -------------- */

  for (ii=0; ii < MIN(ffh->max_ast, awp->audio_tracks); ii++)
  {
    gboolean codec_ok;
    codec_ok = p_init_and_open_audio_codec(ffh
                                         , ii
					 , gpp
					 , awp->awk[ii].sample_rate
					 , awp->awk[ii].channels
					 );
    if(!codec_ok)
    {
      g_free(ffh);
      return(NULL);
    }
  }

  /* media file format parameters */

  ffh->ap = g_malloc0(sizeof(AVFormatParameters));

  /* we use sample rate and channels from the 1.st audio track for
   * the overall format parameters
   * (dont know if ffmpeg allows different settings for the audio tracks
   *  and how to deal with this)
   */
  ffh->ap->sample_rate = awp->awk[0].sample_rate;
  ffh->ap->channels = awp->awk[0].channels;
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

  /* ------------ buffers for video output  -------   */
  for (ii=0; ii < ffh->max_vst; ii++)
  {
    gint  size;
    size = gpp->val.vid_width * gpp->val.vid_height;

    /* allocate video_buffer (should be large enough for one ENCODED frame) */
    ffh->vst[ii].video_buffer_size =  size * 4;  /* (1024*1024); */
    ffh->vst[ii].video_buffer = g_malloc0(ffh->vst[ii].video_buffer_size);
    ffh->vst[ii].video_stream_index = ii;

    ffh->vst[ii].video_dummy_buffer_size =  size * 4;  /* (1024*1024); */
    ffh->vst[ii].video_dummy_buffer = g_malloc0(ffh->vst[ii].video_dummy_buffer_size);

    /* allocate yuv420_buffer (for the RAW image data YUV 4:2:0) */
    ffh->vst[ii].yuv420_buffer_size = size + (size / 2);
    ffh->vst[ii].yuv420_buffer = g_malloc0(ffh->vst[ii].yuv420_buffer_size);

    ffh->vst[ii].yuv420_dummy_buffer_size = size + (size / 2);
    ffh->vst[ii].yuv420_dummy_buffer = g_malloc0(ffh->vst[ii].yuv420_dummy_buffer_size);

    memset(ffh->vst[ii].yuv420_dummy_buffer, 0, ffh->vst[ii].yuv420_dummy_buffer_size);
  }

  /* ------------ buffers for audio output  -------   */
  for (ii=0; ii < ffh->max_ast; ii++)
  {
    ffh->ast[ii].audio_buffer_size =  4 * MAX_AUDIO_PACKET_SIZE;
    ffh->ast[ii].audio_buffer = g_malloc0(ffh->ast[ii].audio_buffer_size);
    /* audio stream indexes start after last video stream */
    ffh->ast[ii].audio_stream_index = ffh->max_vst + ii;
  }


#ifdef HAVE_FULL_FFMPEG
    ffh->output_context->packet_size = epp->mux_packet_size;
    ffh->output_context->mux_rate    = epp->mux_rate;
    ffh->output_context->preload     = (int)(epp->mux_preload*AV_TIME_BASE);
    ffh->output_context->max_delay   = (int)(epp->mux_max_delay*AV_TIME_BASE);
#endif

  if(gap_debug)
  {
    printf("\np_ffmpeg_open END\n");
  }

  return(ffh);

}  /* end p_ffmpeg_open */


/* --------------------------
 * p_ffmpeg_write_frame_chunk
 * --------------------------
 */
static int
p_ffmpeg_write_frame_chunk(t_ffmpeg_handle *ffh, gint32 encoded_size, gint vid_track)
{
  int ret;
  int encoded_dummy_size;
  int ii;

  ret = 0;
  ii = ffh->vst[vid_track].video_stream_index;


  if(gap_debug)
  {
     AVCodec  *codec;

     codec = ffh->vst[ii].vid_codec_context->codec;

     printf("p_ffmpeg_write_frame_chunk: START codec: %d  track:%d\n", (int)codec, (int)vid_track);
     printf("\n-------------------------\n");
     printf("name: %s\n", codec->name);
     printf("type: %d\n", codec->type);
     printf("id: %d\n",   codec->id);
  }

  if(ffh->output_context)
  {
    if(gap_debug)  printf("before av_write_frame  encoded_size:%d\n", (int)encoded_size );

#ifdef HAVE_OLD_FFMPEG_0408
    ret = av_write_frame(ffh->output_context, ffh->vst[ii].video_stream_index, ffh->vst[ii].video_buffer, encoded_size);
#else
    {
      AVPacket pkt;
      av_init_packet (&pkt);
      gint chunk_frame_type;

      /*
       * FFMPEG 0.4.9 and later versions require monotone ascending timestamps
       * therefore we encode a fully black dummy frame
       * this is done to force codec internal advance of timestamps (PTS and DTS)
       * and to let the codec know about the handled frame (internal picture_number count)
       * TODO:
       *   find a faster way to fix PTS
       */
      {
	AVFrame   *big_picture_yuv;
	AVPicture *picture_yuv;
	AVPicture *picture_codec;

	/* picture to feed the codec */
	picture_codec = (AVPicture *)ffh->vst[ii].big_picture_codec;

	/* source picture (YUV420) */
	big_picture_yuv = avcodec_alloc_frame();
	picture_yuv = (AVPicture *)big_picture_yuv;

	/* Feed the encoder with a (black) dummy frame
         */
	avpicture_fill(picture_codec
                      ,ffh->vst[ii].yuv420_dummy_buffer
                      ,PIX_FMT_YUV420P          /* PIX_FMT_RGB24, PIX_FMT_RGBA32, PIX_FMT_BGRA32 */
                      ,ffh->frame_width
                      ,ffh->frame_height
                      );

	ffh->vst[ii].big_picture_codec->quality = ffh->vst[ii].vid_stream->quality;
	ffh->vst[ii].big_picture_codec->key_frame = 1;

        encoded_dummy_size = avcodec_encode_video(ffh->vst[ii].vid_codec_context
                               ,ffh->vst[ii].video_dummy_buffer, ffh->vst[ii].video_dummy_buffer_size
                               ,ffh->vst[ii].big_picture_codec);

	g_free(big_picture_yuv);
      }

      if(encoded_dummy_size == 0)
      {
        /* on size 0 the encoder has buffered the dummy frame.
	 * this unwanted frame might be added when there is a next non-dummy frame
	 * to encode and to write. (dont know if there is a chance to clear
	 * the encoder's internal buffer in this case.)
	 * in my tests this case did not happen yet....
	 * .. but display a warning to find out in further test
	 */
	 g_message(_("Black dummy frame was added"));
      }

      chunk_frame_type = GVA_util_check_mpg_frame_type(ffh->vst[ii].video_buffer, encoded_size);
      if(chunk_frame_type == 1)  /* check for intra frame type */
      {
        pkt.flags |= PKT_FLAG_KEY;
      }


      if(gap_debug)
      {
        printf("CHUNK picture_number: enc_dummy_size:%d  frame_type:%d pic_number: %d %d PTS:%d\n"
	      , (int)encoded_dummy_size
	      , (int)chunk_frame_type
	      , (int)ffh->vst[ii].vid_codec_context->coded_frame->coded_picture_number
	      , (int)ffh->vst[ii].vid_codec_context->coded_frame->display_picture_number
	      , (int)ffh->vst[ii].vid_codec_context->coded_frame->pts
	      );
      }

      pkt.pts = ffh->vst[ii].vid_codec_context->coded_frame->pts;
      pkt.dts = AV_NOPTS_VALUE;  /* let av_write_frame calculate the decompression timestamp */
      pkt.stream_index = ffh->vst[ii].video_stream_index;
      pkt.data = ffh->vst[ii].video_buffer;
      pkt.size = encoded_size;
      ret = av_write_frame(ffh->output_context, &pkt);
    }
#endif
    if(gap_debug)  printf("after av_write_frame  encoded_size:%d\n", (int)encoded_size );
  }

  return (ret);
}  /* end p_ffmpeg_write_frame_chunk */



/* --------------------
 * p_ffmpeg_write_frame
 * --------------------
 */
static int
p_ffmpeg_write_frame(t_ffmpeg_handle *ffh, gboolean force_keyframe, gint vid_track)
{
  AVFrame   *big_picture_yuv;
  AVPicture *picture_yuv;
  AVPicture *picture_codec;
  int encoded_size;
  int ret;
  uint8_t  *l_convert_buffer;
  int ii;

  ii = ffh->vst[vid_track].video_stream_index;
  ret = 0;
  l_convert_buffer = NULL;

  if(gap_debug)
  {
     AVCodec  *codec;

     codec = ffh->vst[ii].vid_codec_context->codec;

     printf("p_ffmpeg_write_frame: START codec: %d track:%d\n", (int)codec, (int)vid_track);
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
  picture_codec = (AVPicture *)ffh->vst[ii].big_picture_codec;

  /* source picture (YUV420) */
  big_picture_yuv = avcodec_alloc_frame();
  picture_yuv = (AVPicture *)big_picture_yuv;

  if(ffh->vst[ii].vid_codec_context->pix_fmt == PIX_FMT_YUV420P)
  {
    if(gap_debug) printf("USE YUV420 (no pix_fmt convert needed)\n");

    /* most of the codecs wants YUV420
      * (we can use the picture in ffh->vst[ii].yuv420_buffer without pix_fmt conversion
      */
    avpicture_fill(picture_codec
                  ,ffh->vst[ii].yuv420_buffer
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
            , (int)ffh->vst[ii].vid_codec_context->pix_fmt
	    , (int)PIX_FMT_YUV420P
	    );
    }


    /* init destination picture structure (the codec context tells us what pix_fmt is needed)
     */
     avpicture_fill(picture_codec
                  ,l_convert_buffer
                  ,ffh->vst[ii].vid_codec_context->pix_fmt          /* PIX_FMT_RGB24, PIX_FMT_RGBA32, PIX_FMT_BGRA32 */
                  ,ffh->frame_width
                  ,ffh->frame_height
                  );
    /* source picture (has YUV420P colormodel) */
     avpicture_fill(picture_yuv
                  ,ffh->vst[ii].yuv420_buffer
                  ,PIX_FMT_YUV420P
                  ,ffh->frame_width
                  ,ffh->frame_height
                  );

    /* AVFrame is the new structure introduced in FFMPEG 0.4.6,
     * (same as AVPicture but with additional members at the end)
     */

    if(gap_debug) printf("before img_convert  pix_fmt: %d  (YUV420:%d)\n", (int)ffh->vst[ii].vid_codec_context->pix_fmt, (int)PIX_FMT_YUV420P);

    /* convert to pix_fmt needed by the codec */
    img_convert(picture_codec, ffh->vst[ii].vid_codec_context->pix_fmt  /* dst */
               ,picture_yuv, PIX_FMT_BGR24                    /* src */
               ,ffh->frame_width
               ,ffh->frame_height
               );

    if(gap_debug)  printf("after img_convert\n");
  }

  /* AVFrame is the new structure introduced in FFMPEG 0.4.6,
   * (same as AVPicture but with additional members at the end)
   */
  ffh->vst[ii].big_picture_codec->quality = ffh->vst[ii].vid_stream->quality;

  if(force_keyframe)
  {
    /* TODO: howto force the encoder to write an I frame ??
     *   ffh->vst[ii].big_picture_codec->key_frame could be ignored
     *   because this seems to be just an information
     *   reported by the encoder.
     */
    /* ffh->vst[ii].vid_codec_context->key_frame = 1; */
    ffh->vst[ii].big_picture_codec->key_frame = 1;
  }
  else
  {
    ffh->vst[ii].big_picture_codec->key_frame = 0;
  }

  if(ffh->output_context)
  {
    if(gap_debug)  printf("before avcodec_encode_video\n");

    encoded_size = avcodec_encode_video(ffh->vst[ii].vid_codec_context
                           ,ffh->vst[ii].video_buffer, ffh->vst[ii].video_buffer_size
                           ,ffh->vst[ii].big_picture_codec);


    /* if zero size, it means the image was buffered */
    if(encoded_size != 0)
    {
      if(gap_debug)  printf("before av_write_frame  encoded_size:%d\n", (int)encoded_size );
#ifdef HAVE_OLD_FFMPEG_0408
      ret = av_write_frame(ffh->output_context
                         , ffh->vst[ii].video_stream_index
                         , ffh->vst[ii].video_buffer
                         , encoded_size);
#else
      {
	AVPacket pkt;
	av_init_packet (&pkt);

	if(gap_debug)
	{
          printf("before av_write_frame  encoded_size:%d\n", (int)encoded_size );
	}

	pkt.pts = ffh->vst[ii].vid_codec_context->coded_frame->pts;
	pkt.stream_index = ffh->vst[ii].video_stream_index;
	pkt.data = ffh->vst[ii].video_buffer;
	pkt.size = encoded_size;
	if(ffh->vst[ii].vid_codec_context->coded_frame->key_frame)
	{
          pkt.flags |= PKT_FLAG_KEY;
	}
	ret = av_write_frame(ffh->output_context, &pkt);

	if(gap_debug)
	{
	  printf("after av_write_frame  encoded_size:%d\n", (int)encoded_size );
	}
      }
#endif
    }
  }


  /* if we are in pass1 of a two pass encoding run, output log */
  if (ffh->vst[ii].passlog_fp && ffh->vst[ii].vid_codec_context->stats_out)
  {
    fprintf(ffh->vst[ii].passlog_fp, "%s", ffh->vst[ii].vid_codec_context->stats_out);
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
p_ffmpeg_write_audioframe(t_ffmpeg_handle *ffh, guchar *audio_buf, int frame_bytes, gint aud_track)
{
  int encoded_size;
  int ret;
  int ii;

  ret = 0;
  ii = aud_track;

  if(ffh->output_context)
  {
    if(gap_debug)
    {
      printf("p_ffmpeg_write_audioframe: START audo track ii=%d\n", (int)ii);
      printf("ffh->ast[%d].audio_buffer: %d\n", (int)ii, (int)ffh->ast[ii].audio_buffer);
      printf("ffh->ast[%d].audio_buffer_size: %d\n", (int)ii, ffh->ast[ii].audio_buffer_size);
      printf("audio_buf: %d\n", (int)audio_buf);
    }
    encoded_size = avcodec_encode_audio(ffh->ast[ii].aud_codec_context
                           ,ffh->ast[ii].audio_buffer, ffh->ast[ii].audio_buffer_size
                           ,(short *)audio_buf);
#ifdef HAVE_OLD_FFMPEG_0408
    ret = av_write_frame(ffh->ast[ii].output_context, ffh->ast[ii].audio_stream_index, ffh->ast[ii].audio_buffer, encoded_size);
#else
    {
      AVPacket pkt;
      av_init_packet (&pkt);

      if(gap_debug)  printf("before av_write_frame  encoded_size:%d\n", (int)encoded_size );

      pkt.pts = ffh->ast[ii].aud_codec_context->coded_frame->pts;
      pkt.stream_index = ffh->ast[ii].audio_stream_index;
      pkt.data = ffh->ast[ii].audio_buffer;
      pkt.size = encoded_size;
      ret = av_write_frame(ffh->output_context, &pkt);

      if(gap_debug)  printf("after av_write_frame  encoded_size:%d\n", (int)encoded_size );
    }
#endif
  }
  return(ret);
}  /* end p_ffmpeg_write_audioframe */


/* ---------------
 * p_ffmpeg_close
 * ---------------
 * write multimeda format trailer, close all video and audio codecs
 * and free all related buffers
 */
static void
p_ffmpeg_close(t_ffmpeg_handle *ffh)
{
  gint ii;

  for (ii=0; ii < ffh->max_vst; ii++)
  {
    if(ffh->vst[ii].big_picture_codec)
    {
      g_free(ffh->vst[ii].big_picture_codec);
      ffh->vst[ii].big_picture_codec = NULL;
    }
  }

  if(ffh->output_context)
  {
    /* ?? TODO check if should close the code first ?? */
    av_write_trailer(ffh->output_context);
    url_fclose(&ffh->output_context->pb);
  }

  for (ii=0; ii < ffh->max_vst; ii++)
  {
    if(ffh->vst[ii].vid_codec_context)
    {
      avcodec_close(ffh->vst[ii].vid_codec_context);
      ffh->vst[ii].vid_codec_context = NULL;
      /* do not attempt to free ffh->vst[ii].vid_codec_context (it points to ffh->vst[ii].vid_stream->codec) */
    }

    if (ffh->vst[ii].passlog_fp)
    {
      fclose(ffh->vst[ii].passlog_fp);
      ffh->vst[ii].passlog_fp = NULL;
    }


    if(ffh->vst[ii].video_buffer)
    {
       g_free(ffh->vst[ii].video_buffer);
       ffh->vst[ii].video_buffer = NULL;
    }
    if(ffh->vst[ii].video_dummy_buffer)
    {
       g_free(ffh->vst[ii].video_dummy_buffer);
       ffh->vst[ii].video_dummy_buffer = NULL;
    }
    if(ffh->vst[ii].yuv420_buffer)
    {
       g_free(ffh->vst[ii].yuv420_buffer);
       ffh->vst[ii].yuv420_buffer = NULL;
    }
    if(ffh->vst[ii].yuv420_dummy_buffer)
    {
       g_free(ffh->vst[ii].yuv420_dummy_buffer);
       ffh->vst[ii].yuv420_dummy_buffer = NULL;
    }

  }


  for (ii=0; ii < ffh->max_ast; ii++)
  {
    if(ffh->ast[ii].aud_codec_context)
    {
      avcodec_close(ffh->ast[ii].aud_codec_context);
      ffh->ast[ii].aud_codec_context = NULL;
      /* do not attempt to free ffh->ast[ii].aud_codec_context (it points to ffh->ast[ii].aud_stream->codec) */
    }

    if(ffh->ast[ii].audio_buffer)
    {
       g_free(ffh->ast[ii].audio_buffer);
       ffh->ast[ii].audio_buffer = NULL;
    }
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
  GapGveFFMpegValues   *epp = NULL;
  t_ffmpeg_handle     *ffh = NULL;
  GapGveStoryVidHandle        *l_vidhand = NULL;
  gint32        l_tmp_image_id = -1;
  gint32        l_layer_id = -1;
  GimpDrawable *l_drawable = NULL;
  long          l_cur_frame_nr;
  long          l_step, l_begin, l_end;
  gdouble       l_percentage, l_percentage_step;
  int           l_rc;
  gint32        l_max_master_frame_nr;
  gint32        l_cnt_encoded_frames;
  gint32        l_cnt_reused_frames;
  gint          l_video_tracks = 0;

  t_awk_array   l_awk_arr;
  t_awk_array   *awp;


  epp = &gpp->evl;
  awp = &l_awk_arr;
  l_cnt_encoded_frames = 0;
  l_cnt_reused_frames = 0;
  p_init_audio_workdata(awp);

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
  {
    gint32 l_total_framecount;
    l_vidhand = gap_gve_story_open_vid_handle (gpp->val.input_mode
                                       ,gpp->val.image_ID
				       ,gpp->val.storyboard_file
				       ,gpp->ainfo.basename
                                       ,gpp->ainfo.extension
                                       ,gpp->val.range_from
                                       ,gpp->val.range_to
                                       ,&l_total_framecount
                                       );

    l_video_tracks = 1;
  }

  /* TODO check for overwrite (in case we are called non-interactive) */


  if (gap_debug) printf("Creating ffmpeg file.\n");


  /* check and open audio wave file(s) */
  p_open_audio_input_files(awp, gpp);


  /* OPEN the video file for write (create)
   *  successful open returns initialized FFMPEG handle structure
   */
  ffh = p_ffmpeg_open(gpp, 0, awp, l_video_tracks);
  if(ffh == NULL)
  {
    p_close_audio_input_files(awp);
    return(-1);     /* FFMPEG open Failed */
  }


  l_percentage = 0.0;
  if(gpp->val.run_mode == GIMP_RUN_INTERACTIVE)
  {
    gimp_progress_init(_("FFMPEG initializing for video encoding .."));
  }


  /* Calculations for encoding the sound */
  p_sound_precalculations(ffh, awp, gpp);


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
  l_max_master_frame_nr = abs(l_end - l_begin) + 1;

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
                                           , ffh->vst[0].video_buffer
                                           , &l_video_frame_chunk_size
                                           , ffh->vst[0].video_buffer_size    /* IN max size */
					   , gpp->val.framerate
					   , l_max_master_frame_nr
                                           );

    if(gap_debug) printf("\nFFenc: after gap_gve_story_fetch_composite_image_or_chunk image_id:%d layer_id:%d\n", (int)l_tmp_image_id , (int) l_layer_id);

    /* this block is done foreach handled video frame */
    if(l_fetch_ok)
    {

      if (l_video_frame_chunk_size > 0)
      {
        l_cnt_reused_frames++;
        if (gap_debug)
	{
	  printf("DEBUG: 1:1 copy of frame %d\n", (int)l_cur_frame_nr);
	}

        /* dont recode, just copy video chunk to output videofile */
        p_ffmpeg_write_frame_chunk(ffh, l_video_frame_chunk_size, 0 /* vid_track */);
      }
      else   /* encode one VIDEO FRAME */
      {
        l_cnt_encoded_frames++;
        l_drawable = gimp_drawable_get (l_layer_id);

	if (gap_debug)
	{
	  printf("DEBUG: %s encoding frame %d\n"
	        , epp->vcodec_name
		, (int)l_cur_frame_nr
		);
	}


        /* fill the yuv420_buffer with current frame image data */
        gap_gve_raw_YUV420P_drawable_encode(l_drawable, ffh->vst[0].yuv420_buffer);

        /* store the compressed video frame */
        if (gap_debug) printf("GAP_FFMPEG: Writing frame nr. %d\n", (int)l_cur_frame_nr);

        p_ffmpeg_write_frame(ffh, l_force_keyframe, 0 /* vid_track */);

        /* destroy the tmp image */
        gimp_image_delete(l_tmp_image_id);
      }

      /* encode AUDIO FRAME (audio data for playbacktime of one frame) */
      p_process_audio_frame(ffh, awp);

    }
    else  /* if fetch_ok */
    {
      l_rc = -1;
    }


    l_percentage += l_percentage_step;
    if(gap_debug) printf("PROGRESS: %f\n", (float) l_percentage);
    if(gpp->val.run_mode == GIMP_RUN_INTERACTIVE)
    {
      char *msg;

      if (l_video_frame_chunk_size > 0)
      {
        msg = g_strdup_printf(_("FFMPEG lossless copy frame %d (%d)")
                           ,(int)l_cnt_encoded_frames + l_cnt_reused_frames
                           ,(int)l_max_master_frame_nr
			   );
      }
      else
      {
        msg = g_strdup_printf(_("FFMPEG encoding frame %d (%d)")
                           ,(int)l_cnt_encoded_frames + l_cnt_reused_frames
                           ,(int)l_max_master_frame_nr
			   );
      }
      gimp_progress_init(msg);
      g_free(msg);
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

  p_close_audio_input_files(awp);

  if(l_vidhand)
  {
    if(gap_debug)
    {
      gap_gve_story_debug_print_framerange_list(l_vidhand->frn_list, -1);
    }
    gap_gve_story_close_vid_handle(l_vidhand);
  }

  /* statistics */
  printf("encoded       frames: %d\n", (int)l_cnt_encoded_frames);
  printf("1:1 copied    frames: %d\n", (int)l_cnt_reused_frames);
  printf("total handled frames: %d\n", (int)l_cnt_encoded_frames + l_cnt_reused_frames);

  return l_rc;
}    /* end p_ffmpeg_encode */
