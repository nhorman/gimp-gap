/*  gap_story_main.c
 *  This module handles GAP storyboard level1 editing
 *  2004/01/23
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

static char *plug_in_version_fmt =  "%d.%d.%d; 2004/01/26";

/* Revision history
 * version 1.3.25a; 2004/01/26  hof: created
 */

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gap_story_main.h"
#include "gap_story_dialog.h"
#include "gap_player_dialog.h"
#include "gap_pview_da.h"

#include "gap-intl.h"

/* Defines */
#define PLUG_IN_NAME        "plug_in_gap_storyboard_edit"
#define PLUG_IN_PRINT_NAME  "Storyboard Editor"
#define PLUG_IN_IMAGE_TYPES "RGB*, INDEXED*, GRAY*"
#define PLUG_IN_AUTHOR      "Wolfgang Hofer (hof@gimp.org)"
#define PLUG_IN_COPYRIGHT   "Wolfgang Hofer"


int gap_debug = 0;  /* 1 == print debug infos , 0 dont print debug infos */ 

static GapStbMainGlobalParams global_params =
{
  GIMP_RUN_INTERACTIVE
, FALSE       /* initialized     run */
, FALSE       /* gboolean     run */
, -1          /* gint32  image_id */
, "\0"        /* storyboard_filename */
, "\0"        /* cliplist_filename */
, NULL        /*  stb  storyboard pointer */
, NULL        /*  cll  cliplist pointer */
, NULL        /*  curr_selection  (list holds all selected items) */
, NULL        /*  plp  player param pointer */

, NULL        /*  GapStbTabWidgets  *stb_widgets */
, NULL        /*  GapStbTabWidgets  *cll_widgets */
, NULL        /*  GapStoryVideoElem *video_list */
, NULL        /*  GapVThumbElem     *vthumb_list */
, NULL        /*  t_GVA_Handle      *gvahand */
, NULL        /*  gchar             *gva_videofile */
, NULL        /*  GtkWidget         *progress_bar */
, FALSE       /*  gboolean           gva_lock */
, FALSE       /*  gboolean           cancel_video_api */
, FALSE       /*  gboolean           auto_vthumb */
, FALSE       /*  gboolean           auto_vthumb_refresh_canceled */
, FALSE       /*  gboolean           in_player_call */
, NULL        /*  GtkWidget *shell_window */

, NULL        /*  GtkWidget *menu_item_win_vthumbs */

, NULL        /*  GtkWidget *menu_item_stb_save */
, NULL        /*  GtkWidget *menu_item_stb_save_as */
, NULL        /*  GtkWidget *menu_item_stb_playback */
, NULL        /*  GtkWidget *menu_item_stb_properties */
, NULL        /*  GtkWidget *menu_item_stb_encode */
, NULL        /*  GtkWidget *menu_item_stb_close */

, NULL        /*  GtkWidget *menu_item_cll_save */
, NULL        /*  GtkWidget *menu_item_cll_save_as */
, NULL        /*  GtkWidget *menu_item_cll_add_clip */
, NULL        /*  GtkWidget *menu_item_cll_playback */
, NULL        /*  GtkWidget *menu_item_cll_properties */
, NULL        /*  GtkWidget *menu_item_cll_encode */
, NULL        /*  GtkWidget *menu_item_cll_close */


   
};


static void  query (void);
static void  run (const gchar *name,
                  gint nparams,              /* number of parameters passed in */
                  const GimpParam * param,   /* parameters passed in */
                  gint *nreturn_vals,        /* number of parameters returned */
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
                  { GIMP_PDB_INT32,    "run_mode", "Interactive"},
                  { GIMP_PDB_IMAGE,    "image", "Input image" },
                  { GIMP_PDB_DRAWABLE, "drawable", "UNUSED"},
  };

/* Functions */

MAIN ()

static void query (void)
{
  char *l_plug_in_version;

  /* get version numbers from config.h (that is derived from ../configure.in) */
  l_plug_in_version = g_strdup_printf(plug_in_version_fmt
                                    ,GAP_MAJOR_VERSION
				    ,GAP_MINOR_VERSION
				    ,GAP_MICRO_VERSION
				    );
  
  gimp_plugin_domain_register (GETTEXT_PACKAGE, LOCALEDIR);

  /* the actual installation of the plugin */
  gimp_install_procedure (PLUG_IN_NAME,
                          "Storyboardfile Editor",
                          "This plug-in is an interactive GUI to create edit storyboard level1 files, "
                          "storyboard level1 files are videoframe playlist textfiles"
			  "that can be used for playback and encoding",
                          PLUG_IN_AUTHOR,
                          PLUG_IN_COPYRIGHT,
                          l_plug_in_version,
                          N_("<Image>/Video/Storyboard..."),
                          PLUG_IN_IMAGE_TYPES,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (in_args),
                          0,        /* G_N_ELEMENTS (out_args) */
                          in_args,
                          NULL      /* out_args */
                          );
 
  g_free(l_plug_in_version);
}  /* end query */

static void
run (const gchar *name,          /* name of plugin */
     gint nparams,               /* number of in-paramters */
     const GimpParam * param,    /* in-parameters */
     gint *nreturn_vals,         /* number of out-parameters */
     GimpParam ** return_vals)   /* out-parameters */
{
  const gchar *l_env;
  gint32       image_id = -1;
  GapStbMainGlobalParams  *sgpp = &global_params;

  /* Get the runmode from the in-parameters */
  GimpRunMode run_mode = param[0].data.d_int32;

  /* status variable, use it to check for errors in invocation usualy only
   * during non-interactive calling 
   */
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;

  /* always return at least the status to the caller. */
  static GimpParam values[1];

  INIT_I18N();


  l_env = g_getenv("GAP_DEBUG");
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
      gimp_get_data (PLUG_IN_NAME, sgpp);
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /* check to see if invoked with the correct number of parameters */
      /* if (nparams == G_N_ELEMENTS (in_args)) */
      {
        printf("%s: noninteractive call NOT supported.\n"
              , PLUG_IN_NAME
              );
        status = GIMP_PDB_CALLING_ERROR;
      }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /* Possibly retrieve data from a previous run */
      gimp_get_data (PLUG_IN_NAME, sgpp);

      break;

    default:
      break;
  }

  if (status == GIMP_PDB_SUCCESS)
  {
    
    sgpp->image_id = image_id;
    sgpp->initialized = FALSE;
    sgpp->run_mode = run_mode;
    sgpp->plp = NULL;
    sgpp->stb = NULL;
    sgpp->cll = NULL;
    gap_storyboard_dialog(sgpp);
  
    /* Store variable states for next run */
    if (run_mode == GIMP_RUN_INTERACTIVE)
      gimp_set_data (PLUG_IN_NAME, sgpp, sizeof (GapStbMainGlobalParams));
  }
  values[0].data.d_status = status;
}	/* end run */
