/* gap_cme_main.c
 * 2001.05.26 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * This is the Master GUI Module for all GIMP-GAP compatible video encoder PlugIns
 * The Master handles
 *  - Parameter input for Video+Audio encoding,
 *  - Audio Preprocessing (convert audio input file(s) to 16bit RIFF Wave format)
 *  - selection and execution of the desired videoencoder Plug-In.
 *
 * videoencoder plug-ins are registered in the gimp-PDB like any other plug-in
 * but have special names (starting with the prefix: plug_in_gap_enc_ )
 * and must provide an API (PDB-procedures) for query their name and other properties.
 * (the Master uses this API to identify all available videoencoders dynamically at runtime)
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
 * version 2.1.0a;  2004.05.06   hof: integration into gimp-gap project
 * version 1.2.2c;  2003.01.09   hof: allow video encoding of multilayer images
 *                                    (image must be saved before the encoder call)
 * version 1.2.2b;  2002.10.28   hof: added filtermacro_file, storyboard_file
 * version 1.2.1a;  2001.05.26   hof: created
 */

#include <config.h>
#include <pthread.h>
#include <stdlib.h>

#include "gap-intl.h"

#include "gap_cme_main.h"
#include "gap_cme_gui.h"

#include "gap_file_util.h"
#include "gap_gve_sox.h"
#include "gap_gve_story.h"
#include "gap_libgimpgap.h"


static char *gap_cme_version_fmt =  "%d.%d.%da; 2004/05/06";


/* ------------------------
 * global gap DEBUG switch
 * ------------------------
 */

/* int gap_debug = 1; */    /* print debug infos */
/* int gap_debug = 0; */    /* 0: dont print debug infos */

int gap_debug = 0;
GapCmeGlobalParams global_params;

static void query(void);
static void  run (const gchar *name,          /* name of plugin */
     gint nparams,               /* number of in-paramters */
     const GimpParam * param,    /* in-parameters */
     gint *nreturn_vals,         /* number of out-parameters */
     GimpParam ** return_vals);  /* out-parameters */

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

/* ----------------------------------------
 * query
 * ----------------------------------------
 */
static void
query ()
{
  char  *gap_cme_version;

  /* get version numbers from config.h (that is derived from ../configure.in) */
  gap_cme_version = g_strdup_printf(gap_cme_version_fmt
                                    ,GAP_MAJOR_VERSION
                                    ,GAP_MINOR_VERSION
                                    ,GAP_MICRO_VERSION
                                    );
  gimp_plugin_domain_register (GETTEXT_PACKAGE, LOCALEDIR);

  static GimpParamDef args_qt_enc[] =
  {
    {GIMP_PDB_INT32,    "run_mode", "Interactive, non-interactive"},
    {GIMP_PDB_IMAGE,    "image", "Input image"},
    {GIMP_PDB_DRAWABLE, "drawable", "Input drawable (unused)"},
    {GIMP_PDB_STRING,   "vidfile", "filename of quicktime video (to write)"},
    {GIMP_PDB_INT32,    "range_from", "number of first frame"},
    {GIMP_PDB_INT32,    "range_to", "number of last frame"},
    {GIMP_PDB_INT32,    "vid_width", "Width of resulting Video Frames (all Frames are scaled to this width)"},
    {GIMP_PDB_INT32,    "vid_height", "Height of resulting Video Frames (all Frames are scaled to this height)"},
    {GIMP_PDB_INT32,    "vid_format", "videoformat:  0=comp., 1=PAL, 2=NTSC, 3=SECAM, 4=MAC, 5=unspec"},
    {GIMP_PDB_FLOAT,    "framerate", "framerate in frames per seconds"},
    {GIMP_PDB_INT32,    "samplerate", "audio samplerate in samples per seconds (.wav files are resampled using sox, if needed)"},
    {GIMP_PDB_STRING,   "audfile", "optional audiodata file .wav or any audiodata compatible to sox (see manpage of sox for more info)"},
    {GIMP_PDB_STRING,   "vid_enc_plugin", "name of a gap_video_encoder plugin choose one of these strings: "
                                "\"" GAP_PLUGIN_NAME_QT1_ENCODE    "\""
                                "\"" GAP_PLUGIN_NAME_QT2_ENCODE    "\""
                                "\"" GAP_PLUGIN_NAME_SINGLEFRAMES_ENCODE "\""
                                "\"" GAP_PLUGIN_NAME_FFMPEG_ENCODE "\""
                                "\"" GAP_PLUGIN_NAME_MPG1_ENCODE   "\""
                                "\"" GAP_PLUGIN_NAME_MPG2_ENCODE   "\""
                               },
    {GIMP_PDB_STRING,   "filtermacro_file", "macro to apply on each handled frame. (textfile with filter plugin names and LASTVALUE bufferdump"},
    {GIMP_PDB_STRING,   "storyboard_file", "textfile with list of one or more images, framesequences, videoclips or audioclips (see storyboard docs for more information)"},
  };
  static int nargs_qt_enc = sizeof(args_qt_enc) / sizeof(args_qt_enc[0]);

  static GimpParamDef *return_vals = NULL;
  static int nreturn_vals = 0;

  INIT_I18N();

  gimp_install_procedure(GAP_CME_PLUGIN_NAME_VID_ENCODE_MASTER,
                         _("This plugin is the master dialog for video + audio encoding"),
                         _("This plugin is a common GUI for all available video + audio encoding plugins"
                         " it operates on a selected range of animframes or storyboard files."
                         " The (optional) audio inputdata (param: audfile) is transformed to RIFF WAVE format (16Bit PCM)"
                         " and passed to the selected videoencoder plug-in as temporary file."
                         " (or direct if format and samplerate already matches the desired target samplerate)."
                         " The videoformat is defined with vid_enc_plugin parameter. The specified plugin "
                         " is called with the Parameters specified in the dialog. for noninteractive calls"
                         " default values will be used. (you may call the desired plugin directly if you"
                         " want to specify non-interacive parameters"),
                         "Wolfgang Hofer (hof@gimp.org)",
                         "Wolfgang Hofer",
                         gap_cme_version,
                         N_("<Image>/Video/Encode/Master Videoencoder"),
                         "RGB*, INDEXED*, GRAY*",
                         GIMP_PLUGIN,
                         nargs_qt_enc, nreturn_vals,
                         args_qt_enc, return_vals);


}       /* end query */


/* ----------------------------------------
 * p_call_encoder_procedure
 * ----------------------------------------
 */
static gint
p_call_encoder_procedure(GapCmeGlobalParams *gpp)
{
  GimpParam* l_params;
  gint   l_retvals;
  gint             l_nparams;
  gint             l_nreturn_vals;
  GimpPDBProcType   l_proc_type;
  gchar            *l_proc_blurb;
  gchar            *l_proc_help;
  gchar            *l_proc_author;
  gchar            *l_proc_copyright;
  gchar            *l_proc_date;
  GimpParamDef    *l_paramdef;
  GimpParamDef    *l_return_vals;
  char *l_msg;
  gint  l_use_encoderspecific_params;
  gint  l_rc;
  gchar            *l_16bit_wav_file;

  l_rc = -1;

  l_use_encoderspecific_params = 0;  /* run with default values */


  if(gpp->val.ecp_sel.vid_enc_plugin[0] == '\0')
  {
     printf("p_call_encoder_procedure: No encoder available (exit)\n");
     return -1;
  }

  if(gpp->val.ecp_sel.gui_proc[0] != '\0')
  {
    l_use_encoderspecific_params = 1;  /* run with encoder specific values */
  }

  if(gap_debug)
  {
     printf("p_call_encoder_procedure %s: START\n", gpp->val.ecp_sel.vid_enc_plugin);
     printf("  videoname: %s\n", gpp->val.videoname);
     printf("  audioname1: %s\n", gpp->val.audioname1);
     printf("  basename: %s\n", gpp->ainfo.basename);
     printf("  extension: %s\n", gpp->ainfo.extension);
     printf("  range_from: %d\n", (int)gpp->val.range_from);
     printf("  range_to: %d\n", (int)gpp->val.range_to);
     printf("  framerate: %f\n", (float)gpp->val.framerate);
     printf("  samplerate: %d\n", (int)gpp->val.samplerate);
     printf("  wav_samplerate: %d\n", (int)gpp->val.wav_samplerate1);
     printf("  vid_width: %d\n", (int)gpp->val.vid_width);
     printf("  vid_height: %d\n", (int)gpp->val.vid_height);
     printf("  vid_format: %d\n", (int)gpp->val.vid_format);
     printf("  image_ID: %d\n", (int)gpp->val.image_ID);
     printf("  l_use_encoderspecific_params: %d\n", (int)l_use_encoderspecific_params);
     printf("  filtermacro_file: %s\n", gpp->val.filtermacro_file);
     printf("  storyboard_file: %s\n", gpp->val.storyboard_file);
  }

  if(FALSE == gimp_procedural_db_proc_info (gpp->val.ecp_sel.vid_enc_plugin,
                          &l_proc_blurb,
                          &l_proc_help,
                          &l_proc_author,
                          &l_proc_copyright,
                          &l_proc_date,
                          &l_proc_type,
                          &l_nparams,
                          &l_nreturn_vals,
                          &l_paramdef,
                          &l_return_vals))
  {
     l_msg = g_strdup_printf(_("Required Plugin %s not available"), gpp->val.ecp_sel.vid_enc_plugin);
     if(gpp->val.run_mode == GIMP_RUN_INTERACTIVE)
     {
       g_message(l_msg);
     }
     g_free(l_msg);
     return -1;
  }

  l_16bit_wav_file = &gpp->val.audioname1[0];
  if(gpp->val.tmp_audfile[0] != '\0')
  {
     l_16bit_wav_file = &gpp->val.tmp_audfile[0];
  }


  /* generic call of GAP video encoder plugin */
  l_params = gimp_run_procedure (gpp->val.ecp_sel.vid_enc_plugin,
                     &l_retvals,
                     GIMP_PDB_INT32,  gpp->val.run_mode,
                     GIMP_PDB_IMAGE,  gpp->val.image_ID,
                     GIMP_PDB_DRAWABLE, -1,
                     GIMP_PDB_STRING, gpp->val.videoname,
                     GIMP_PDB_INT32,  gpp->val.range_from,
                     GIMP_PDB_INT32,  gpp->val.range_to,
                     GIMP_PDB_INT32,  gpp->val.vid_width,
                     GIMP_PDB_INT32,  gpp->val.vid_height,
                     GIMP_PDB_INT32,  gpp->val.vid_format,
                     GIMP_PDB_FLOAT,  gpp->val.framerate,
                     GIMP_PDB_INT32,  gpp->val.samplerate,
                     GIMP_PDB_STRING, l_16bit_wav_file,
                     GIMP_PDB_INT32,  l_use_encoderspecific_params,
                     GIMP_PDB_STRING, gpp->val.filtermacro_file,
                     GIMP_PDB_STRING, gpp->val.storyboard_file,
                     GIMP_PDB_END);
  if(l_params[0].data.d_status == GIMP_PDB_SUCCESS)
  {
    l_rc = 0;
  }
  g_free(l_params);

  if(l_rc < 0)
  {
     l_msg = g_strdup_printf(_("Call of Required Plugin %s failed"), gpp->val.ecp_sel.vid_enc_plugin);
     if(gpp->val.run_mode == GIMP_RUN_INTERACTIVE)
     {
       g_message(l_msg);
     }
     g_free(l_msg);
  }


  return (l_rc);
}  /* end p_call_encoder_procedure */

/* ----------------------------------------
 * gap_cme_main_get_global_params
 * ----------------------------------------
 */
GapCmeGlobalParams *
gap_cme_main_get_global_params(void)
{
  return (&global_params);  
} /* end gap_cme_main_get_global_params */

/* ----------------------------------------
 * run
 * ----------------------------------------
 */
static void
run (const gchar *name,          /* name of plugin */
     gint n_params,               /* number of in-paramters */
     const GimpParam * param,    /* in-parameters */
     gint *nreturn_vals,         /* number of out-parameters */
     GimpParam ** return_vals)   /* out-parameters */
{
  GapCmeGlobalParams *gpp;

  static GimpParam values[1];
  gint32     l_rc;
  const char *l_env;

  gpp = gap_cme_main_get_global_params();
  *nreturn_vals = 1;
  *return_vals = values;
  l_rc = 0;

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_SUCCESS;

  l_env = g_getenv("GAP_DEBUG_ENC");
  if(l_env != NULL)
  {
    if((*l_env != 'n') && (*l_env != 'N')) gap_debug = 1;
  }

  if(gap_debug) fprintf(stderr, "\n\ngap_qt_main: debug name = %s\n", name);

  gpp->val.gui_proc_tid = 0;
  gpp->val.run_mode = param[0].data.d_int32;

  INIT_I18N();

  gap_gve_sox_init_config(&gpp->val);

  if (strcmp (name, GAP_CME_PLUGIN_NAME_VID_ENCODE_MASTER) == 0)
  {
      char *l_base;
      int   l_l;

      /* get image_ID and animinfo */
      gpp->val.image_ID    = param[1].data.d_image;
      gap_gve_misc_get_ainfo(gpp->val.image_ID, &gpp->ainfo);

      /* set initial values */
      l_base = g_strdup(gpp->ainfo.basename);
      l_l = strlen(l_base);

      if (l_l > 0)
      {
         if(l_base[l_l -1] == '_')
         {
           l_base[l_l -1] = '\0';
         }
      }
      g_snprintf(gpp->val.videoname, sizeof(gpp->val.videoname), "%s.mov", l_base);
      g_free(l_base);

      gpp->val.audioname1[0] = '\0';
      gpp->val.filtermacro_file[0] = '\0';
      gpp->val.storyboard_file[0] = '\0';
      gpp->val.framerate = gpp->ainfo.framerate;
      gpp->val.range_from = gpp->ainfo.curr_frame_nr;
      gpp->val.range_to   = gpp->ainfo.last_frame_nr;
      gpp->val.samplerate = 44100;
      gpp->val.vid_width  = gimp_image_width(gpp->val.image_ID);
      gpp->val.vid_height = gimp_image_height(gpp->val.image_ID);
      gpp->val.vid_format = VID_FMT_NTSC;

      /* g_snprintf(gpp->val.ecp_sel.vid_enc_plugin, sizeof(gpp->val.ecp_sel.vid_enc_plugin), "%s", GAP_PLUGIN_NAME_MPG1_ENCODE); */
      gpp->val.ecp_sel.vid_enc_plugin[0] = '\0';

      if (gpp->val.run_mode == GIMP_RUN_NONINTERACTIVE)
      {
        if (n_params != GAP_VENC_NUM_STANDARD_PARAM)
        {
          values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
        }
        else
        {
           g_snprintf(gpp->val.videoname, sizeof(gpp->val.videoname), "%s", param[3].data.d_string);
           if (param[4].data.d_int32 >= 0) { gpp->val.range_from =    param[4].data.d_int32; }
           if (param[5].data.d_int32 >= 0) { gpp->val.range_to   =    param[5].data.d_int32; }
           if (param[6].data.d_int32 > 0)  { gpp->val.vid_width  =    param[6].data.d_int32; }
           if (param[7].data.d_int32 > 0)  { gpp->val.vid_height  =   param[7].data.d_int32; }
           if (param[8].data.d_int32 >= 0) { gpp->val.vid_format  =   param[8].data.d_int32; }
           gpp->val.framerate   = param[9].data.d_float;
           gpp->val.samplerate  = param[10].data.d_int32;
           g_snprintf(gpp->val.audioname1, sizeof(gpp->val.audioname1), "%s", param[11].data.d_string);
           g_snprintf(gpp->val.ecp_sel.vid_enc_plugin, sizeof(gpp->val.ecp_sel.vid_enc_plugin), "%s", param[12].data.d_string);
           g_snprintf(gpp->val.filtermacro_file, sizeof(gpp->val.filtermacro_file), "%s", param[13].data.d_string);
           g_snprintf(gpp->val.storyboard_file, sizeof(gpp->val.storyboard_file), "%s", param[14].data.d_string);
       }
      }
      else if(gpp->val.run_mode != GIMP_RUN_INTERACTIVE)
      {
         values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
      }

      if (values[0].data.d_status == GIMP_PDB_SUCCESS)
      {
         if(gpp->val.run_mode == GIMP_RUN_INTERACTIVE)
         {
            if(gap_debug) printf("MAIN before gap_cme_gui_master_encoder_dialog ------------------\n");
            l_rc = gap_cme_gui_master_encoder_dialog (gpp);
            if(gap_debug) printf("MAIN after gap_cme_gui_master_encoder_dialog ------------------\n");

            /* delete images in the cache
             * (the cache may have been filled while parsing
             * storyboard file in the common dialog
             */
             gap_gve_story_drop_image_cache();
            if(gap_debug) printf("MAIN after gap_gve_story_drop_image_cache ------------------\n");
         }
         else
         {
            /* gap_gve_sox_chk_and_resample was already called
             * in the interactive dialog ( at the OK button checks.)
             * but in noninteractive modes we have to resample now.
             */
            l_rc =   gap_gve_sox_chk_and_resample(&gpp->val);
         }


         if (l_rc >= 0 )
         {
            char *l_tmpname;
            gint32 l_tmp_image_id;

            l_tmpname = NULL;
            l_tmp_image_id = -1;

            if((gpp->ainfo.last_frame_nr - gpp->ainfo.first_frame_nr == 0)
            && (gpp->val.storyboard_file[0] == '\0'))
            {
              char *l_current_name;

              if((strcmp(gpp->ainfo.extension, ".xcf") == 0)
              || (strcmp(gpp->ainfo.extension, ".xjt") == 0))
              {
                /* for xcf and xjt just save without making a temp copy */
                l_tmpname = gimp_image_get_filename(gpp->val.image_ID);
              }
              else
              {
                /* prepare encoder params to run the encoder on the (saved) duplicate of the image */
                g_snprintf(gpp->ainfo.basename, sizeof(gpp->ainfo.basename), "%s", l_tmpname);

                /* save a temporary copy of the image */
                l_tmp_image_id = gimp_image_duplicate(gpp->val.image_ID);

                /* l_tmpname = gimp_temp_name("xcf"); */
                l_current_name = gimp_image_get_filename(gpp->val.image_ID);
                l_tmpname = g_strdup_printf("%s_temp_copy.xcf", l_current_name);
                gimp_image_set_filename (l_tmp_image_id, l_tmpname);
                gpp->ainfo.extension[0] = '\0';
                gpp->val.image_ID = l_tmp_image_id;
                g_free(l_current_name);
              }

              gimp_file_save(GIMP_RUN_NONINTERACTIVE
                            , gpp->val.image_ID
                            , 1 /* dummy layer_id */
                            ,l_tmpname
                            ,l_tmpname
                            );

            }

            /* ------------------------------------------- HERE WE GO, start Video Encoder ---- */
            l_rc = p_call_encoder_procedure(gpp);

            if(l_tmp_image_id >= 0)
            {
              gap_image_delete_immediate(l_tmp_image_id);
              remove(l_tmpname);
            }
            if(l_tmpname)
            {
              g_free(l_tmpname);
            }
         }

         if(gpp->val.tmp_audfile[0] != '\0')
         {
           remove(gpp->val.tmp_audfile);
         }

#ifdef GAP_USE_PTHREAD
         /* is the encoder specific gui_thread still open ? */
         if(gpp->val.gui_proc_tid != 0)
         {
            /* wait until thread exits */
            pthread_join(gpp->val.gui_proc_tid, 0);
         }
#endif
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

}  /* end run */
