/*  gap_wr_trans.c
 *    wrapper plugins to flip or roatate Layer  by Wolfgang Hofer
 *  2005/05/01
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
 * 1.0.0; 2005/05/01         hof: created
 */

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gap_lastvaldesc.h"

#include "gap-intl.h"


/* Defines */
#define PLUG_IN_NAME_180    "plug_in_wr_layer_rotate_180"
#define PLUG_IN_NAME_90     "plug_in_wr_layer_rotate_90"
#define PLUG_IN_NAME_270    "plug_in_wr_layer_rotate_270"
#define PLUG_IN_NAME_HOR    "plug_in_wr_layer_flip_horizontal"
#define PLUG_IN_NAME_VER    "plug_in_wr_layer_flip_vetrical"
#define PLUG_IN_IMAGE_TYPES "RGB*, INDEXED*, GRAY*"
#define PLUG_IN_AUTHOR      "Wolfgang Hofer (hof@gimp.org)"
#define PLUG_IN_COPYRIGHT   "Wolfgang Hofer"

typedef enum
{
   GAP_TRANS_UNDEFINED
  ,GAP_TRANS_FLIP_HOR
  ,GAP_TRANS_FLIP_VER
  ,GAP_TRANS_ROT_90
  ,GAP_TRANS_ROT_180
  ,GAP_TRANS_ROT_270
} GapTransLayerMode;


typedef struct {
  gint32  dummy;
} TransValues;

static TransValues glob_vals =
{
  0           /* dummy */
};

static void  iter_query ();
static void  query (void);
static void  run (const gchar *name,          /* name of plugin */
     gint nparams,               /* number of in-paramters */
     const GimpParam * param,    /* in-parameters */
     gint *nreturn_vals,         /* number of out-parameters */
     GimpParam ** return_vals);  /* out-parameters */

static gint32 p_transform_layer(gint32 image_id, gint32 drawable_id, GapTransLayerMode trans_mode);


/* Global Variables */
int gap_debug = 0;  /* 1 == print debug infos , 0 dont print debug infos */

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
                  { GIMP_PDB_DRAWABLE, "drawable", "Input drawable ()"},
  };


static gint global_number_in_args = G_N_ELEMENTS (in_args);
static gint global_number_out_args = 0;




/* Functions */

MAIN ()



static void query (void)
{

  static GimpLastvalDef lastvals[] =
  {
    GIMP_LASTVALDEF_GINT32          (GIMP_ITER_FALSE,  glob_vals.dummy,  "dummy"),
  };

  gimp_plugin_domain_register (GETTEXT_PACKAGE, LOCALEDIR);

  /* registration for last values buffer structure (useful for animated filter apply) */
  gimp_lastval_desc_register(PLUG_IN_NAME_90,
                             &glob_vals,
                             sizeof(glob_vals),
                             G_N_ELEMENTS (lastvals),
                             lastvals);

  gimp_lastval_desc_register(PLUG_IN_NAME_180,
                             &glob_vals,
                             sizeof(glob_vals),
                             G_N_ELEMENTS (lastvals),
                             lastvals);

  gimp_lastval_desc_register(PLUG_IN_NAME_270,
                             &glob_vals,
                             sizeof(glob_vals),
                             G_N_ELEMENTS (lastvals),
                             lastvals);

  gimp_lastval_desc_register(PLUG_IN_NAME_HOR,
                             &glob_vals,
                             sizeof(glob_vals),
                             G_N_ELEMENTS (lastvals),
                             lastvals);


  gimp_lastval_desc_register(PLUG_IN_NAME_VER,
                             &glob_vals,
                             sizeof(glob_vals),
                             G_N_ELEMENTS (lastvals),
                             lastvals);


  /* the actual installation of the plugin */
  gimp_install_procedure (PLUG_IN_NAME_HOR,
                          "Flip Layer horizontal",
                          "This plug-in is a wrapper for gimp flip functionality, "
                          "and provides the typical PDB interface for calling it as "
                          "filter in animations."
                          "(for the use with GAP Video Frame manipulation)",
                          PLUG_IN_AUTHOR,
                          PLUG_IN_COPYRIGHT,
                          GAP_VERSION_WITH_DATE,
                          N_("Flip Horizontal"),
                          PLUG_IN_IMAGE_TYPES,
                          GIMP_PLUGIN,
                          global_number_in_args,
                          global_number_out_args,
                          in_args,
                          NULL);

  gimp_install_procedure (PLUG_IN_NAME_VER,
                          "Flip Layer vertical",
                          "This plug-in is a wrapper for gimp flip functionality, "
                          "and provides the typical PDB interface for calling it as "
                          "filter in animations."
                          "(for the use with GAP Video Frame manipulation)",
                          PLUG_IN_AUTHOR,
                          PLUG_IN_COPYRIGHT,
                          GAP_VERSION_WITH_DATE,
                          N_("Flip Vertical"),
                          PLUG_IN_IMAGE_TYPES,
                          GIMP_PLUGIN,
                          global_number_in_args,
                          global_number_out_args,
                          in_args,
                          NULL);

  gimp_install_procedure (PLUG_IN_NAME_90,
                          "Rotate Layer by 90 degree",
                          "This plug-in is a wrapper for gimp simple rotate functionality, "
                          "and provides the typical PDB interface for calling it as "
                          "filter in animations."
                          "(for the use with GAP Video Frame manipulation)",
                          PLUG_IN_AUTHOR,
                          PLUG_IN_COPYRIGHT,
                          GAP_VERSION_WITH_DATE,
                          N_("Rotate 90 degrees CW"),
                          PLUG_IN_IMAGE_TYPES,
                          GIMP_PLUGIN,
                          global_number_in_args,
                          global_number_out_args,
                          in_args,
                          NULL);


  gimp_install_procedure (PLUG_IN_NAME_180,
                          "Rotate Layer by 180 degree",
                          "This plug-in is a wrapper for gimp simple rotate functionality, "
                          "and provides the typical PDB interface for calling it as "
                          "filter in animations."
                          "(for the use with GAP Video Frame manipulation)",
                          PLUG_IN_AUTHOR,
                          PLUG_IN_COPYRIGHT,
                          GAP_VERSION_WITH_DATE,
                          N_("Rotate 180 degrees"),
                          PLUG_IN_IMAGE_TYPES,
                          GIMP_PLUGIN,
                          global_number_in_args,
                          global_number_out_args,
                          in_args,
                          NULL);

  gimp_install_procedure (PLUG_IN_NAME_270,
                          "Rotate Layer by 270 degree",
                          "This plug-in is a wrapper for gimp simple rotate functionality, "
                          "and provides the typical PDB interface for calling it as "
                          "filter in animations."
                          "(for the use with GAP Video Frame manipulation)",
                          PLUG_IN_AUTHOR,
                          PLUG_IN_COPYRIGHT,
                          GAP_VERSION_WITH_DATE,
                          N_("Rotate 90 degrees CCW"),
                          PLUG_IN_IMAGE_TYPES,
                          GIMP_PLUGIN,
                          global_number_in_args,
                          global_number_out_args,
                          in_args,
                          NULL);
  {
    /* Menu names */
    const char *menupath_image_video_layer_transform = N_("<Image>/Video/Layer/Transform/");

    //gimp_plugin_menu_branch_register("<Image>", "Video");
    //gimp_plugin_menu_branch_register("<Image>/Video", "Layer");
    //gimp_plugin_menu_branch_register("<Image>/Video/Layer", "Transform");

    gimp_plugin_menu_register (PLUG_IN_NAME_HOR, menupath_image_video_layer_transform);
    gimp_plugin_menu_register (PLUG_IN_NAME_VER, menupath_image_video_layer_transform);
    gimp_plugin_menu_register (PLUG_IN_NAME_90,  menupath_image_video_layer_transform);
    gimp_plugin_menu_register (PLUG_IN_NAME_180, menupath_image_video_layer_transform);
    gimp_plugin_menu_register (PLUG_IN_NAME_270, menupath_image_video_layer_transform);
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
  gint32       drawable_id = -1;
  gint32       trans_drawable_id = -1;
  GapTransLayerMode trans_mode;

  trans_mode = GAP_TRANS_UNDEFINED;

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

  if(gap_debug) printf("\n\nDEBUG: run %s\n", name);

  /* initialize the return of the status */
  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
  values[1].type = GIMP_PDB_DRAWABLE;
  values[1].data.d_drawable = -1;
  *nreturn_vals = 1;
  *return_vals = values;


  /* get image and drawable */
  image_id = param[1].data.d_int32;
  drawable_id = param[2].data.d_int32;

  if (strcmp (name, PLUG_IN_NAME_HOR) == 0)
  {
     trans_mode = GAP_TRANS_FLIP_HOR;
  }

  if (strcmp (name, PLUG_IN_NAME_VER) == 0)
  {
    trans_mode = GAP_TRANS_FLIP_VER;
  }

  if (strcmp (name, PLUG_IN_NAME_90) == 0)
  {
    trans_mode = GAP_TRANS_ROT_90;
  }

  if (strcmp (name, PLUG_IN_NAME_180) == 0)
  {
    trans_mode = GAP_TRANS_ROT_180;
  }

  if (strcmp (name, PLUG_IN_NAME_270) == 0)
  {
    trans_mode = GAP_TRANS_ROT_270;
  }
  
  if(trans_mode != GAP_TRANS_UNDEFINED)
  {
    if(gimp_drawable_is_layer(drawable_id))
    {
      /* here the action starts, we transform the drawable */
      trans_drawable_id = p_transform_layer(image_id
                                           , drawable_id
					   , trans_mode
					   );
      if (trans_drawable_id < 0)
      {
	 status = GIMP_PDB_CALLING_ERROR;
      }
      else
      {
         values[1].data.d_drawable = drawable_id;

	 /* Store variable states for next run
	  * (the parameters for the transform wrapper plugins are stored
	  *  even if they contain just a dummy
	  *  this is done to fullfill the GIMP-GAP LAST_VALUES conventions
	  *  for filtermacro and animated calls)
	  */
	 if (run_mode == GIMP_RUN_INTERACTIVE)
	 {
	   gimp_set_data (name, &glob_vals, sizeof (TransValues));
	 }
      }
    }
    else
    {
       status = GIMP_PDB_CALLING_ERROR;
       if (run_mode == GIMP_RUN_INTERACTIVE)
       {
         g_message(_("The plug-in %s\noperates only on layers\n"
	             "(but was called on mask or channel)")
	          , name
		  );
       }
    }
 
  }


  if (status == GIMP_PDB_SUCCESS)
  {

    /* If run mode is interactive, flush displays, else (script) don't
     * do it, as the screen updates would make the scripts slow 
     */
    if (run_mode != GIMP_RUN_NONINTERACTIVE)
      gimp_displays_flush ();

  }
  values[0].data.d_status = status;
}	/* end run */



/* --------------------------
 * p_transform_layer
 * --------------------------
 */
static gint32
p_transform_layer(gint32 image_id, gint32 drawable_id, GapTransLayerMode trans_mode)
{
  gboolean auto_center;
  gboolean clip_result;
  gdouble  axis;
  gboolean has_selection;
  gboolean non_empty;
  gint     x1, y1, x2, y2;
  gint32   sav_selection_id;
  gint32   trans_drawable_id;
  gint32   center_x;
  gint32   center_y;

  gimp_image_undo_group_start(image_id);
  
  sav_selection_id = -1;
  has_selection  = gimp_selection_bounds(image_id, &non_empty, &x1, &y1, &x2, &y2);
    
  center_x = gimp_drawable_width(drawable_id) / 2;
  center_y = gimp_drawable_height(drawable_id) / 2;
  if(has_selection)
  {
    sav_selection_id = gimp_selection_save(image_id);
  }
    

  trans_drawable_id = -1;
  auto_center = TRUE;
  clip_result = TRUE;
  
  /* here the action starts, we transform the drawable */
  switch(trans_mode)
  {
    case GAP_TRANS_FLIP_HOR:
      clip_result = TRUE;
      axis = (gdouble)(gimp_drawable_width(drawable_id)) / 2.0;
      trans_drawable_id = gimp_drawable_transform_flip_simple(drawable_id
                                   ,GIMP_ORIENTATION_HORIZONTAL
				   ,auto_center
				   ,axis
				   ,clip_result
				   );
      break;
    case GAP_TRANS_FLIP_VER:
      clip_result = TRUE;
      axis = (gdouble)(gimp_drawable_height(drawable_id)) / 2.0;
      trans_drawable_id = gimp_drawable_transform_flip_simple(drawable_id
                                   ,GIMP_ORIENTATION_VERTICAL
				   ,auto_center
				   ,axis
				   ,clip_result
				   );
      break;
    case GAP_TRANS_ROT_90:
      clip_result = FALSE;
      trans_drawable_id = gimp_drawable_transform_rotate_simple(drawable_id
                                  ,GIMP_ROTATE_90
				  ,auto_center
				  ,center_x
				  ,center_y
				  ,clip_result
				  );
      break;
    case GAP_TRANS_ROT_180:
      clip_result = FALSE;
      trans_drawable_id = gimp_drawable_transform_rotate_simple(drawable_id
                                  ,GIMP_ROTATE_180
				  ,auto_center
				  ,center_x
				  ,center_y
				  ,clip_result
				  );
      break;
    case GAP_TRANS_ROT_270:
      clip_result = FALSE;
      trans_drawable_id = gimp_drawable_transform_rotate_simple(drawable_id
                                  ,GIMP_ROTATE_270
				  ,auto_center
				  ,center_x
				  ,center_y
				  ,clip_result
				  );
      break;
    default:
      break;
  }
  
      
  if((has_selection)
  && (sav_selection_id >= 0))
  {
    /* if there was a selection, the transform (flip or simple rotate) operation
     * is restricted to the selected region. in this case
     * the treansform has created a new drawable as floating selection.
     *
     * we automatically anchor the floating selection and restore the orginal selection,
     * because our plug-in is typically called more than once
     * in non-interactive sequence on all layers of the same image
     * 
     * this would not work for the 2.nd call if the 1.st call produces a floating selection.
     */

    gimp_floating_sel_anchor (gimp_image_get_floating_sel (image_id));

    gimp_selection_load(sav_selection_id);
    gimp_image_remove_channel(image_id, sav_selection_id);
    gimp_drawable_delete(sav_selection_id);
  }

  gimp_image_undo_group_end(image_id);

  return (trans_drawable_id);

}  /* end p_transform_layer */
