/*  gap_player_main.c
 *  video (preview) playback of animframes based on thumbnails  by Wolfgang Hofer
 *  2003/06/11
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* Revision history
 * version 1.3.16a; 2003/06/26  hof: updated version
 * version 1.3.15a; 2003/06/21  hof: created
 */

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gap_player_main.h"
#include "gap_player_dialog.h"
#include "gap_pview_da.h"

#include "gap-intl.h"

/* Defines */
#define PLUG_IN_NAME        "plug_in_gap_videoframes_player"
#define PLUG_IN_PRINT_NAME  "Videopreview Player"
#define PLUG_IN_VERSION     "v1.3.16 (2003/06/26)"
#define PLUG_IN_IMAGE_TYPES "RGB*, INDEXED*, GRAY*"
#define PLUG_IN_AUTHOR      "Wolfgang Hofer (hof@gimp.org)"
#define PLUG_IN_COPYRIGHT   "Wolfgang Hofer"


int gap_debug = 0;  /* 1 == print debug infos , 0 dont print debug infos */ 

static t_global_params global_params =
{
  GIMP_RUN_INTERACTIVE
, -1          /* gint32  image_id */
, NULL        /* t_anim_info *ainfo_ptr */
  
, FALSE       /*  gboolean   autostart */
, TRUE        /*  gboolean   use_thumbnails */
, TRUE        /*  gboolean   exact_timing */
, FALSE       /*  gboolean   play_is_active */
, TRUE        /*  gboolean   play_selection_only */
, TRUE        /*  gboolean   play_loop */
, FALSE       /*  gboolean   play_pingpong */
, FALSE       /*  gboolean   play_backward */

, -1          /*  gint32     play_timertag */

, 0           /*  gint32   begin_frame */
, 0           /*  gint32   end_frame */
, 0           /*  gint32   play_current_framenr */
, 0           /*  gint32   pb_stepsize */
, 24.0        /*  gdouble  speed;     playback speed fps */
, 24.0        /*  gdouble  original_speed;    playback speed fps */
, 24.0        /*  gdouble  prev_speed;    previous playback speed fps */
, 256         /*  gint32   pv_pixelsize  32 upto 512 */
, 256         /*  gint32   pv_width  32 upto 512 */
, 256         /*  gint32   pv_height  32 upto 512 */
  
, FALSE       /*  gboolean in_feedback */
, FALSE       /*  gboolean in_timer_playback */
, FALSE       /*  gboolean in_resize */

, 0           /*  gint32  go_job_framenr */
, -1          /*  gint32  go_timertag */
, -1          /*  gint32  go_base_framenr */
, -1          /*  gint32  go_base */
, 0           /*  gint32  pingpong_count */
  
, NULL        /*  t_pview   *pv_ptr */
, NULL        /*  GtkWidget *shell_window */
, NULL        /*  GtkObject *from_spinbutton_adj */
, NULL        /*  GtkObject *to_spinbutton_adj */
, NULL        /*  GtkObject *framenr_spinbutton_adj */
, NULL        /*  GtkObject *speed_spinbutton_adj */
, NULL        /*  GtkObject *size_spinbutton_adj */
   
, NULL        /*  GtkWidget *status_label */
, NULL        /*  GtkWidget *timepos_label */
, NULL        /*  GtkWidget *table11 */
, NULL        /*  GtkWidget *size_spinbutton */

, NULL        /*  Gtimer    *gtimer */
, 0.0         /*  gdouble   cycle_time_secs  */
, 0.0         /*  gdouble   rest_secs  */
, 0.0         /*  gdouble   delay_secs  */
, 0.0         /*  gdouble   framecnt  */
, 0           /*  gint32    resize_count */
, 0           /*  gint32    old_resize_width */
, 0           /*  gint32    old_resize_height */
};


static void  query (void);
static void  run (char *name,
                  int nparams,            /* number of parameters passed in */
                  GimpParam * param,         /* parameters passed in */
                  int *nreturn_vals,      /* number of parameters returned */
                  GimpParam ** return_vals); /* parameters to be returned */


/* Global Variables */
GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,   /* init_proc  */
  NULL,   /* quit_proc  */
  query,  /* query_proc */
  run     /* run_proc   */
};

static GimpParamDef in_args[] = {
                  { GIMP_PDB_INT32,    "run_mode", "Interactive, non-interactive"},
                  { GIMP_PDB_IMAGE,    "image", "Input image" },
                  { GIMP_PDB_DRAWABLE, "drawable", "UNUSED"},
                  { GIMP_PDB_INT32,    "range_from", "framenr begin of selected range"},
                  { GIMP_PDB_INT32,    "range_to", "framenr end of selected range"},
                  { GIMP_PDB_INT32,    "autostart", "(0) FALSE: no autostart, (1) TRUE: automatc playback start"},
                  { GIMP_PDB_INT32,    "use_thumbnails", "(0) FALSE: playback original size frame images, (1) TRUE playback using thumbnails where available (much faster)"},
                  { GIMP_PDB_INT32,    "exact_timing", "(0) FALSE: disable frame dropping, (1) TRUE: allow dropping frames to keep exact timing"},
                  { GIMP_PDB_INT32,    "play_selection_only", "(0) FALSE: play all frames, (1) TRUE: play selected range only"},
                  { GIMP_PDB_INT32,    "play_loop", "(0) FALSE: play once, (1) TRUE: play in loop"},
                  { GIMP_PDB_INT32,    "play_pingpong", "(0) FALSE: normal play, (1) TRUE: bidirectional play"},
                  { GIMP_PDB_FLOAT,    "speed", "-1 use original speed  framerate in frames/sec (valid range is 1.0 upto 250.0)"},
                  { GIMP_PDB_INT32,    "size_width", "-1 use standard size"},
                  { GIMP_PDB_INT32,    "size_height", "-1 use standard size"},
  };

/* Functions */

MAIN ()

static void query (void)
{
  INIT_I18N();

  /* the actual installation of the plugin */
  gimp_install_procedure (PLUG_IN_NAME,
                          "Video Preview Playback",
                          "This plug-in does videoplayback, "
                          "based on thumbnail preview or full sized anim frames.",
                          PLUG_IN_AUTHOR,
                          PLUG_IN_COPYRIGHT,
                          PLUG_IN_VERSION,
                          N_("<Image>/Video/Playback..."),
                          PLUG_IN_IMAGE_TYPES,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (in_args),
                          0,        /* G_N_ELEMENTS (out_args) */
                          in_args,
                          NULL      /* out_args */
                          );
 
}

static void
run (char *name,                /* name of plugin */
     int nparams,               /* number of in-paramters */
     GimpParam * param,            /* in-parameters */
     int *nreturn_vals,         /* number of out-parameters */
     GimpParam ** return_vals)     /* out-parameters */
{
  char       *l_env;
  gint32      image_id = -1;
  t_global_params  *gpp = &global_params;

  /* Get the runmode from the in-parameters */
  GimpRunMode run_mode = param[0].data.d_int32;

  /* status variable, use it to check for errors in invocation usualy only
   * during non-interactive calling 
   */
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;

  /* always return at least the status to the caller. */
  static GimpParam values[1];

  l_env = getenv("GAP_DEBUG");
  if(l_env != NULL)
  {
    if((*l_env != 'n') && (*l_env != 'N')) gap_debug = 1;
  }

  if(gap_debug) fprintf(stderr, "\n\nDEBUG: run %s\n", name);

  /* initialize the return of the status */
  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
  *nreturn_vals = 1;
  *return_vals = values;


  /* get image and drawable */
  image_id = param[1].data.d_int32;


  switch (run_mode)
  {
    case GIMP_RUN_INTERACTIVE:
      /* Possibly retrieve data from a previous run */
      gimp_get_data (PLUG_IN_NAME, gpp);
      gpp->autostart = FALSE;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /* check to see if invoked with the correct number of parameters */
      if (nparams == G_N_ELEMENTS (in_args))
      {
          gpp->begin_frame          = (gint) param[3].data.d_int32;
          gpp->end_frame            = (gint) param[4].data.d_int32;
          gpp->autostart            = (gint) param[5].data.d_int32;
          gpp->use_thumbnails       = (gint) param[6].data.d_int32;
          gpp->exact_timing         = (gint) param[7].data.d_int32;
          gpp->play_selection_only  = (gint) param[8].data.d_int32;
          gpp->play_loop            = (gint) param[9].data.d_int32;
          gpp->play_pingpong        = (gint) param[10].data.d_int32;
          gpp->speed                = (gdouble) param[10].data.d_float;
          gpp->pv_width             = (gint) param[11].data.d_int32;
          gpp->pv_height            = (gint) param[12].data.d_int32;
          
          gpp->pv_pixelsize = MAX(gpp->pv_width, gpp->pv_height);
      }
      else
      {
        printf("%s: noninteractive call wrong nuber %d of params were passed. (%d params are required)\n"
              , PLUG_IN_NAME
              , (int)nparams
              , (int)G_N_ELEMENTS (in_args)
              );
        status = GIMP_PDB_CALLING_ERROR;
      }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /* Possibly retrieve data from a previous run */
      gimp_get_data (PLUG_IN_NAME, gpp);
      gpp->autostart = FALSE;

      break;

    default:
      break;
  }

  if (status == GIMP_PDB_SUCCESS)
  {
    
    gpp->image_id = image_id;
    gpp->run_mode = run_mode;
    p_playback_dialog(gpp);
  
    /* Store variable states for next run */
    if (run_mode == GIMP_RUN_INTERACTIVE)
      gimp_set_data (PLUG_IN_NAME, gpp, sizeof (t_global_params));
  }
  values[0].data.d_status = status;
}	/* end run */
