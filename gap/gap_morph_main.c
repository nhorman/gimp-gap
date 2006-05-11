/*  gap_morph_main.c
 *  creation of morphing animations (transform source image into des. image) by Wolfgang Hofer
 *  2004/02/11
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
 * version 1.3.26a; 2004/02/12  hof: created
 */

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gap_morph_main.h"
#include "gap_morph_exec.h"
#include "gap_morph_dialog.h"
#include "gap_pview_da.h"

/* for pointfile loader (workaround) */
#include "gap_mov_dialog.h"
#include "gap_mov_exec.h"



#include "gap-intl.h"

/* Defines */
#define PLUG_IN_NAME        "plug_in_gap_morph_layers"
#define PLUG_IN_PRINT_NAME  "Morph Layers"
#define PLUG_IN_IMAGE_TYPES "RGBA, GRAYA"
#define PLUG_IN_AUTHOR      "Wolfgang Hofer (hof@gimp.org)"
#define PLUG_IN_COPYRIGHT   "Wolfgang Hofer"


int gap_debug = 0;  /* 1 == print debug infos , 0 dont print debug infos */

static GapMorphGlobalParams global_params =
{
  GIMP_RUN_INTERACTIVE
, -1          /* gint32  image_id */

, 1           /* gint32  tween_steps */
, -1          /* gint32  fdst_layer_id */
, -1          /* gint32  osrc_layer_id */
, NULL        /* GapMorphWorkPoint  *master_wp_list */

, "\0"        /* char workpoint_file_lower[1024] */
, "\0"        /* char workpoint_file_upper[1024] */
, TRUE        /* gboolean            create_tween_layers */
, FALSE       /* gboolean            multiple_pointsets */
, FALSE       /* gboolean            use_quality_wp_selection */
, FALSE       /* gboolean            use_gravity */
, 2.0         /* gdouble             gravity_intensity */
, 100.0       /* gdouble             affect_radius */
, GAP_MORPH_RENDER_MODE_MORPH  /* gint32 render_mode */
, FALSE       /* gboolean            do_progress */
, 0.0         /* gdouble             master_progress */
, 0.0         /* gdouble             layer_progress_step */
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
                  { GIMP_PDB_INT32,    "run_mode", "Interactive, non-interactive"},
                  { GIMP_PDB_IMAGE,    "image", "Input image" },
                  { GIMP_PDB_DRAWABLE, "src_drawable", "source drawable (usually a layer)"},
                  { GIMP_PDB_DRAWABLE, "dst_drawable", "destination drawable (usually a layer)"},
                  { GIMP_PDB_INT32,    "tween_steps", "number of layers to create (or modify) below the dst_layer"},
                  { GIMP_PDB_INT32,    "render_mode", "0: Do Morph transformation, 1: do only Warp transformation"},
                  { GIMP_PDB_INT32,    "create_tween_layers", "TRUE: Do create tween layers,  FALSE: operate on existing layers"},
                  { GIMP_PDB_STRING,   "workpoint_file_1", "Name of a Morph/Warp workpointfile"
		                                           "(create such file(s) with the save button in the GUI at INTERACTIVE runmode)"},
                  { GIMP_PDB_STRING,   "workpoint_file_2", "Name of an ptional 2nd Morph/Warp workpointfile."
		                                           " (pass an empty string or the same name as the 1st file"
							   " if you want to operate with one workpoint file)"},
  };


MAIN ()

static void query (void)
{
  gimp_plugin_domain_register (GETTEXT_PACKAGE, LOCALEDIR);

  /* the actual installation of the plugin */
  gimp_install_procedure (PLUG_IN_NAME,
                          "Image Layer Morphing",
                          "This plug-in creates new layers by transforming the src_drawable to dst_drawable, "
                          "the transformation type depends on the render_mode parameter. "
			  "for MORPH render_mode (0) it is a combination of 2 warp deformation actions "
			  "and cross-blending, commonly known as morphing."
                          "The tween_steps parameter controls how much new layers to create. "
			  "(or how much layers to modify depending on the create_tween_layers parameter) "
                          "source and destination may differ in size and can be in different images. "
			  "for WARP render_mode (1) there will be just Move Deformation. "
			  "Deformation is controled by workpoints. Workpoints are created and edited with the help "
			  "of the INTERACTIVE GUI and saved to file(s). "
			  "For the NON-INTERACTIVE runmode you must provide the filename of such a file. "
			  "Provide 2 filenames if you want to operate with multiple workpoint sets. "
			  "In that case your workpoint files can have a 2 digit numberpart. "
			  "This will implicite select all filenames with numbers inbetween as well."
			  ,
                          PLUG_IN_AUTHOR,
                          PLUG_IN_COPYRIGHT,
                          GAP_VERSION_WITH_DATE,
                          N_("Morph..."),
                          PLUG_IN_IMAGE_TYPES,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (in_args),
                          0,        /* G_N_ELEMENTS (out_args) */
                          in_args,
                          NULL      /* out_args */
                          );

  // gimp_plugin_menu_branch_register("<Image>", "Video");
  gimp_plugin_menu_register (PLUG_IN_NAME, N_("<Image>/Video/"));
}

static void
run (const gchar *name,          /* name of plugin */
     gint nparams,               /* number of in-paramters */
     const GimpParam * param,    /* in-parameters */
     gint *nreturn_vals,         /* number of out-parameters */
     GimpParam ** return_vals)   /* out-parameters */
{
  const gchar *l_env;
  gint32       image_id = -1;
  gint32       drawable_id = -1;
  GapMorphGlobalParams  *mgpp = &global_params;
  gboolean      run_flag;

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

  run_flag = FALSE;
  /* initialize the return of the status */
  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
  *nreturn_vals = 1;
  *return_vals = values;


  /* get image */
  image_id = param[1].data.d_int32;
  drawable_id = param[2].data.d_drawable;
  mgpp->do_progress = FALSE;


  switch (run_mode)
  {
    case GIMP_RUN_INTERACTIVE:
      /* Possibly retrieve data from a previous run */
      gimp_get_data (PLUG_IN_NAME, mgpp);
      mgpp->osrc_layer_id          = drawable_id;
      mgpp->fdst_layer_id          = gap_morph_exec_find_dst_drawable(image_id, drawable_id);
      run_flag = gap_morph_dialog(mgpp);
      mgpp->do_progress            = TRUE;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /* check to see if invoked with the correct number of parameters */
      if (nparams == G_N_ELEMENTS (in_args))
      {
          mgpp->multiple_pointsets     = TRUE;  /* use pointset(s) from file */
	  
	  /* set defaults for params that may be specified in the workpointfiles
	   * (the defaults will take effect only if the file does not contain such settings)
	   */
	  mgpp->use_quality_wp_selection = FALSE;
	  mgpp->use_gravity = FALSE;
	  mgpp->gravity_intensity = 2.0;
	  mgpp->affect_radius = 100.0;
	  
          mgpp->osrc_layer_id          = param[2].data.d_drawable;
          mgpp->fdst_layer_id          = param[3].data.d_drawable;
          mgpp->tween_steps            = param[4].data.d_int32;
          mgpp->render_mode            = param[5].data.d_int32;
          mgpp->create_tween_layers    = param[6].data.d_int32;
	  if(param[7].data.d_string != NULL)
	  {
	    if(param[7].data.d_string[0] != '\0')
	    {
	      g_snprintf(mgpp->workpoint_file_lower
	              , sizeof(mgpp->workpoint_file_lower)
		      , "%s", param[7].data.d_string);
	      g_snprintf(mgpp->workpoint_file_upper
	              , sizeof(mgpp->workpoint_file_upper)
		      , "%s", param[7].data.d_string);
	    }
	    else
	    {
              printf("%s: noninteractive call requires a not-empty workpoint_file_1 parameter\n"
        	    , PLUG_IN_NAME
        	    );
              status = GIMP_PDB_CALLING_ERROR;
	    }
	  }
	  else
	  {
            printf("%s: noninteractive call requires a not-NULL workpoint_file_1 parameter\n"
        	    , PLUG_IN_NAME
        	    );
            status = GIMP_PDB_CALLING_ERROR;
	  }
	  
	  if(param[8].data.d_string != NULL)
	  {
	    if(param[8].data.d_string[0] != '\0')
	    {
	      g_snprintf(mgpp->workpoint_file_upper
	              , sizeof(mgpp->workpoint_file_upper)
		      , "%s", param[8].data.d_string);
	    }
	  }
	  
	  run_flag = TRUE;
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
      gimp_get_data (PLUG_IN_NAME, mgpp);
      mgpp->osrc_layer_id          = drawable_id;
      mgpp->fdst_layer_id          = gap_morph_exec_find_dst_drawable(image_id, drawable_id);
      run_flag = gap_morph_dialog(mgpp);
      mgpp->do_progress            = TRUE;
      break;

    default:
      break;
  }

  if ((status == GIMP_PDB_SUCCESS)
  && (run_flag))
  {

    mgpp->image_id = image_id;
    mgpp->run_mode = run_mode;
    
    gap_morph_execute(mgpp);

    /* Store variable states for next run */
    if (run_mode == GIMP_RUN_INTERACTIVE)
      gimp_set_data (PLUG_IN_NAME, mgpp, sizeof (GapMorphGlobalParams));
  }
  values[0].data.d_status = status;
}       /* end run */


 
