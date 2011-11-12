/*  gap_fg_matting_main.c
 *    foreground extraction based on alpha matting algorithm.
 *    Render transparancy for a layer based on a tri-map drawable (provided by the user).
 *    The tri-map defines what pixels are FOREGROUND (white) BACKGROUND (black) or UNDEFINED (gray).
 *    foreground extraction affects the UNDEFINED pixels and calculates transparency for those pixels.
 *
 *    This module handles PDB registration as GIMP plug-in
 *  2011/10/05
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
 *  (2011/10/05)  2.7.0       hof: created
 */
int gap_debug = 1;  /* 1 == print debug infos , 0 dont print debug infos */
#define GAP_DEBUG_DECLARED 1


#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gap_fg_matting_main.h"
#include "gap_fg_matting_exec.h"
#include "gap_fg_tile_manager.h"
#include "gap_fg_matting_dialog.h"
#include "gap_fg_matting.h"
#include "gap_fg_regions.h"

#include "gap-intl.h"

static GapFgExtractValues fgVals =
{
   -1        /* input_drawable_id */
 , -1        /* tri_map_drawable_id */
 , FALSE     /* create_layermask   */
 , TRUE      /* lock_color */
// , TRUE     /* create_result */
// , 1.0       /* colordiff_threshold */
};




static void  query (void);
static void  run (const gchar *name,          /* name of plugin */
     gint nparams,               /* number of in-paramters */
     const GimpParam * param,    /* in-parameters */
     gint *nreturn_vals,         /* number of out-parameters */
     GimpParam ** return_vals);  /* out-parameters */


static gint32 gap_drawable_foreground_extract (GimpDrawable              *drawable,
                                 GimpDrawable              *maskDrawable,
                                 GapFgExtractValues        *fgValPtr
                                 );
static GappMattingState * gap_drawable_foreground_extract_matting_init (GimpDrawable *drawable,
                                               gint          mask_x,
                                               gint          mask_y,
                                               gint          mask_width,
                                               gint          mask_height);
static void  gap_drawable_foreground_extract_matting (GimpDrawable       *maskDrawable,
                                         GimpDrawable       *resultDrawable,
                                         GappMattingState   *state,
                                         gfloat              start_percentage
                                         );
static void  gap_drawable_foreground_extract_matting_done (GappMattingState *state);

static gint32  p_tri_map_preprocessing (GimpDrawable *drawable, GapFgExtractValues *fgValPtr, gint32 *dummyLayerId);



/* Global Variables */
GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,   /* init_proc  */
  NULL,   /* quit_proc  */
  query,  /* query_proc */
  run     /* run_proc   */
};

static const GimpParamDef in_args[] =
{
    { GIMP_PDB_INT32,    "run-mode",      "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",         "Input image"                  },
    { GIMP_PDB_DRAWABLE, "drawable",      "Input layer (RGB or RGBA)"               },
    { GIMP_PDB_DRAWABLE, "tri-mask",      "tri-mask drawable that defines "
                                             "konwn forground (>= 240), known background (0) and unkonwn (128) values"
                                             "This drawable must have same size as the Input-Layer "
                                             "specify -1 to use the layer-mask of the input drawable as tri-map."},
    { GIMP_PDB_INT32,    "create-result",    "0 .. overwrite the input layer with the processing result. "
                                             "1 .. return the result as new layer (and add the resulting layer above the input layer)" },
    { GIMP_PDB_INT32,    "create-layermask", "0 .. render transparency as alpha channel. "
                                             "1 .. render transparency as layer mask." },
    { GIMP_PDB_INT32,    "lock-color",       "0 .. remove background color in semi transparent pixels. "
                                             "1 .. keep original colors for all pixels (affect only transparency)." },
    { GIMP_PDB_FLOAT,    "colordiff-threshold", "colordiff threshold in range #.# to #.#." }
};


static const GimpParamDef return_vals[] = {
    { GIMP_PDB_DRAWABLE, "resulting-layer",  "depending on result-mode this may be the input drawable or a newly created layer" }
};

static gint global_number_in_args = G_N_ELEMENTS (in_args);
static gint global_number_out_args = G_N_ELEMENTS (return_vals);


/* Functions */

MAIN ()

static void query (void)
{

  gimp_plugin_domain_register (GETTEXT_PACKAGE, LOCALEDIR);


  /* the actual installation of the plugin */
  gimp_install_procedure (PLUG_IN_NAME,
                          "Set layer opacity by foreground extraction based on alpha matting algorithm.",
                          "This plug-in renders the opacity for the specified Input drawable (a layer), "
                          "according to the supplied tri-mask, where the tri mask defines the opacity in the corresponding "
                          "pixel in the resulting layer. "
                          "White pixels (>= 240) in the tri-mask will set corresponding result pixels to fully opaque, "
                          "Black pixels (0) in the tri-mask will set corresponding result pixels to fully tranparent, "
                          "Gray pixels in the tri-mask mark UNDEFINED areas and will trigger cthe calculation of transparency (and optionally color) "
                          "of the corresponding result pixel, depending on its color compared with near foreground and backround pixels. "
                          "the result is written to a newly created layer or optionally overwrites the input layer. "
                          "in case where the input layer has already an alpha channel fully transparent pixels in the input layer"
                          "are not affected "
                          "(e.g. keep their full transparency, even if the tri-map marks this pixels as foreground) "
                          " ",
                          PLUG_IN_AUTHOR,
                          PLUG_IN_COPYRIGHT,
                          GAP_VERSION_WITH_DATE,
                          N_("Foreground Extract..."),
                          PLUG_IN_IMAGE_TYPES,
                          GIMP_PLUGIN,
                          global_number_in_args,
                          global_number_out_args,
                          in_args,
                          return_vals);

  {
    /* Menu names */
    const char *menupath_image_video_layer_attr = N_("<Image>/Video/Layer/Attributes/");
    const char *menupath_image_layer_tranparency = N_("<Image>/Layer/Transparency/");


    //gimp_plugin_menu_branch_register("<Image>", "Video");
    //gimp_plugin_menu_branch_register("<Image>/Video", "Layer");

    gimp_plugin_menu_register (PLUG_IN_NAME, menupath_image_video_layer_attr);
    gimp_plugin_menu_register (PLUG_IN_NAME, menupath_image_layer_tranparency);
  }

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
  gint32       activeDrawableId = -1;
  gboolean doProgress;



  /* Get the runmode from the in-parameters */
  GimpRunMode run_mode = param[0].data.d_int32;

  /* status variable, use it to check for errors in invocation usualy only
     during non-interactive calling */
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;

  /* always return at least the status to the caller. */
  static GimpParam values[2];

  INIT_I18N();

  l_env = g_getenv("GAP_DEBUG");
  if(l_env != NULL)
  {
    if((*l_env != 'n') && (*l_env != 'N')) gap_debug = 1;
  }

  if(gap_debug)
  {
    printf("\n\nDEBUG: run %s\n", name);
  }


  doProgress = FALSE;

  /* initialize the return of the status */
  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
  values[1].type = GIMP_PDB_DRAWABLE;
  values[1].data.d_drawable = -1;
  *nreturn_vals = 2;
  *return_vals = values;

  gap_fg_matting_init_default_vals(&fgVals);

  /* get image and drawable */
  image_id = param[1].data.d_int32;
  activeDrawableId = param[2].data.d_drawable;

  /* Possibly retrieve data from a previous run */
  gimp_get_data (name, &fgVals);
  fgVals.input_drawable_id = activeDrawableId;

  /* how are we running today? */
  switch (run_mode)
  {
    case GIMP_RUN_INTERACTIVE:
      /* Get information from the dialog */
      if (!gap_fg_matting_dialog(&fgVals))
      {
        /* return without processing in case the Dialog was cancelled */
        return;
      }
      doProgress = TRUE;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /* check to see if invoked with the correct number of parameters */
      if (nparams == global_number_in_args)
      {
          fgVals.input_drawable_id = activeDrawableId;
          fgVals.tri_map_drawable_id  = (gint32)  param[3].data.d_drawable;
          fgVals.create_layermask     = (param[4].data.d_int32 == 0) ? FALSE : TRUE;
          fgVals.lock_color           = (param[5].data.d_int32 == 0) ? FALSE : TRUE;
//          fgVals.create_result        = (param[6].data.d_int32 == 0) ? FALSE : TRUE;
//          fgVals.colordiff_threshold  = (gdouble) param[7].data.d_float;
      }
      else
      {
        status = GIMP_PDB_CALLING_ERROR;
      }

      break;

    case GIMP_RUN_WITH_LAST_VALS:
      break;

    default:
      break;
  }

  if (status == GIMP_PDB_SUCCESS)
  {
	gboolean doFlush;

	doFlush =  (run_mode == GIMP_RUN_INTERACTIVE);
    fgVals.input_drawable_id    = (gint32)  param[2].data.d_drawable;

    /* Run the main function */
    values[1].data.d_drawable =
        gap_fg_matting_exec_apply_run(image_id, param[2].data.d_drawable, doProgress, doFlush, &fgVals);
    if (values[1].data.d_drawable < 0)
    {
       status = GIMP_PDB_CALLING_ERROR;
    }

    /* If run mode is interactive, flush displays, else (script) don't
     * do it, as the screen updates would make the scripts slow
     */
    if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      gimp_displays_flush ();
    }

    /* Store variable states for next run */
    if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      gimp_set_data (name, &fgVals, sizeof (GapFgExtractValues));
    }

  }
  values[0].data.d_status = status;


}       /* end run */

