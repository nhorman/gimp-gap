/* gap_onion_main.c
 * 2001.11.20 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * This Module handles ONION Skin Layers in the GIMP Video Menu.
 * Onion Layer(s) usually do show previous (or next) frame(s)
 * of the video in the current frame.
 *
 *   Video/OnionSkin/Configuration        ... GUI to configure, create abd delete onionskin Layer(s) for framerange
 *   Video/OnionSkin/Create or Replace    ... create or replace onionskin Layer(s) and set visible.
 *   Video/OnionSkin/Delete               ... delete onionskin Layer(s)
 *   Video/OnionSkin/Toggle Visibility    ... show/hide onionskin layer(s)
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
 * version 1.3.16b; 2003.07.06   hof: new parameter farn_opaque (for cross-fading support)
 * version 1.3.14a; 2003.05.24   hof: integration into gimp-gap-1.3.14
 * version 1.3.12a; 2003.05.03   hof: started port to gimp-1.3 / gtk+2.2
 * version 1.2.2a;  2001.11.20   hof: created
 */

#include <gap_onion_main.h>
#include <gap_onion_worker.h>
#include <gap_onion_dialog.h>


static char *gap_onion_version = "1.3.16b; 2003/07/06";


/* ------------------------
 * global gap DEBUG switch
 * ------------------------
 */

/* int gap_debug = 1; */    /* print debug infos */
/* int gap_debug = 0; */    /* 0: dont print debug infos */

int gap_debug = 0;
t_global_params global_params;


static void query(void);
static void run(char *name, int nparam, GimpParam *param,
                int *nretvals, GimpParam **retvals);

GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc */
  NULL,  /* quit_proc */
  query, /* query_proc */
  run,   /* run_proc */
};

  static GimpParamDef args_onion_cfg[] =
  {
    {GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive"},
    {GIMP_PDB_IMAGE, "image", "Input image (the current videoframe)"},
    {GIMP_PDB_DRAWABLE, "drawable", "Input drawable (unused)"},
    {GIMP_PDB_INT32, "num_olayers", "Number of Onion Layers"},
    {GIMP_PDB_INT32, "ref_delta", "Reference Frame Delta:  +- 1 ... n  Default: -1 "},
    {GIMP_PDB_INT32, "ref_cycle", "Reference is Cycle   : TRUE/FALSE  (TRUE .. last frame has frame 0 as next frame)"},
    {GIMP_PDB_INT32, "stack_pos", "Place OnionLayer(s) on Stackposition 0..n Default: 1"},
    {GIMP_PDB_INT32, "stack_top", "TRUE Stack Position is relative from TOP, FALSE relative to Bottom"},
    {GIMP_PDB_FLOAT, "opacity", "OnionOpacity: 0.0..100.0%"},
    {GIMP_PDB_FLOAT, "opacity_delta", "OnionOpacityDelta: 0..100%  Default: 80"},
    {GIMP_PDB_INT32, "ignore_botlayers", "Ignore N Bottom Sourcelayers (0 .. use full picture, 1 ignor bg layer) "},
    {GIMP_PDB_INT32, "select_mode", "Mode how to identify a layer: 0-3 by layername 0=equal, 1=prefix, 2=suffix, 3=contains, 6=all visible (ignore select_string)"},
    {GIMP_PDB_INT32, "select_case", "0: ignore case 1: select_string is case sensitive"},
    {GIMP_PDB_INT32, "select_invert", "0: select normal 1: invert (select all unselected layers)"},
    {GIMP_PDB_STRING, "select_string", "string to match with layername (how to match is defined by select_mode)"},
    {GIMP_PDB_INT32, "range_from", "first affected frame (ignored if run is not 2 or 3)"},
    {GIMP_PDB_INT32, "range_to", "last affected frame (ignored if run is not 2 or 3)"},
    {GIMP_PDB_INT32, "run", "0 .. do nothing, 1..set params for this session, 2..set and create or replace onionlayers for selected framerange  3..delete onionlayers from selected famerange "},
    {GIMP_PDB_INT32, "farn_opaque", "TRUE..farest neighbour frame has highest opacity, FALSE: nearest has highest opacity"},
  };
  static int nargs_onion_cfg = sizeof(args_onion_cfg) / sizeof(args_onion_cfg[0]);

  static GimpParamDef args_onion_visi[] =
  {
    {GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive"},
    {GIMP_PDB_IMAGE, "image", "Input image (the current videoframe)"},
    {GIMP_PDB_DRAWABLE, "drawable", "Input drawable (unused)"},
    {GIMP_PDB_INT32, "visible_mode", "0: set invisible 1: set visible 2: toggle visibility"},
  };
  static int nargs_onion_visi = sizeof(args_onion_visi) / sizeof(args_onion_visi[0]);

  static GimpParamDef args_onion_std[] =
  {
    {GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive"},
    {GIMP_PDB_IMAGE, "image", "Input image (the current videoframe)"},
    {GIMP_PDB_DRAWABLE, "drawable", "Input drawable (unused)"},
  };
  static int nargs_onion_std = sizeof(args_onion_std) / sizeof(args_onion_std[0]);

  static GimpParamDef *return_vals = NULL;
  static int nreturn_vals = 0;


/* ------------------------
 * MAIN query and run
 * ------------------------
 */

MAIN ()

static void
query ()
{


  INIT_I18N();

  gimp_install_procedure(GAP_PLUGIN_NAME_ONION_CFG,
             _("This plugin sets Configuration for Onion Layers in Videofames"),
                         _("This plugin is the configuration GUI for Onion layers."
                         " Onion Layer(s) usually do show previous (or next) frame(s)"
                         " of the video in the current frame."
                         " Onion Layers are not created automatically. You have to create or delete them manually"
                         " using the menu Video/OnionSkin/make or Video/OnionSkin/delete or call the Procedures "
                         GAP_PLUGIN_NAME_ONION_APPLY " "
                         GAP_PLUGIN_NAME_ONION_DEL " "
                         " The configuration can be saved in the gimprc parameter file."),
                         "Wolfgang Hofer (hof@gimp.org)",
                         "Wolfgang Hofer",
                         gap_onion_version,
                         N_("<Image>/Video/Onionskin/Configuration..."),
                         "RGB*, INDEXED*, GRAY*",
                         GIMP_PLUGIN,
                         nargs_onion_cfg, nreturn_vals,
                         args_onion_cfg, return_vals);

  gimp_install_procedure(GAP_PLUGIN_NAME_ONION_APPLY,
             _("This plugin creates or replaces Onionskin Layer(s)"),
                         _("This plugin creates or updates Onionskin Layers in the current Videoframe."
                         " Onion Layer(s) usually do show previous (or next) frame(s)"
                         " of the video. At 1.st call  in the current frame."
                         " This Plugin runs NONINTERACTIVE only. It depends on the configuration settings"
                         " made by Video/Onionskin/Config or call of the plugin: "
                         GAP_PLUGIN_NAME_ONION_CFG " "
                         " if no configuration is found, default settings are used"),
                         "Wolfgang Hofer (hof@gimp.org)",
                         "Wolfgang Hofer",
                         gap_onion_version,
                         N_("<Image>/Video/Onionskin/Create or Replace"),
                         "RGB*, INDEXED*, GRAY*",
                         GIMP_PLUGIN,
                         nargs_onion_std, nreturn_vals,
                         args_onion_std, return_vals);


  gimp_install_procedure(GAP_PLUGIN_NAME_ONION_DEL,
             _("This plugin removes OnionSkin Layer(s)"),
                         _("This plugin removes Onion Skin Layers from the current Videoframe."
                         " Onion Layer(s) usually do show previous (or next) frame(s)"
                         " of the video."),
                         "Wolfgang Hofer (hof@gimp.org)",
                         "Wolfgang Hofer",
                         gap_onion_version,
                         N_("<Image>/Video/Onionskin/Delete"),
                         "RGB*, INDEXED*, GRAY*",
                         GIMP_PLUGIN,
                         nargs_onion_std, nreturn_vals,
                         args_onion_std, return_vals);


  gimp_install_procedure(GAP_PLUGIN_NAME_ONION_VISI,
             _("This plugin toggles visibility of OnionSkin Layer(s)"),
                         _("This plugin sets visibility for all onionskin Layers in the current Videoframe."),
                         "Wolfgang Hofer (hof@gimp.org)",
                         "Wolfgang Hofer",
                         gap_onion_version,
                         N_("<Image>/Video/Onionskin/Toggle Visibility"),
                         "RGB*, INDEXED*, GRAY*",
                         GIMP_PLUGIN,
                         nargs_onion_visi, nreturn_vals,
                         args_onion_visi, return_vals);
}       /* end query */


/* optional: include files with GUI and some worker procedures here
 * (so we can compile and install using gimptool without any makefile)
 */
#ifdef INCLUDE_ALL_C_FILES
#include "gap_match.c"
#include "gap_lib.c"
#include "gap_onion_gui.c"
#include "gap_onion_worker.c"
#endif

static void
run (char    *name,
     int      n_params,
     GimpParam  *param,
     int     *nreturn_vals,
     GimpParam **return_vals)
{
  t_global_params *gpp;

  static GimpParam values[1];
  gint32     l_rc;
  gint32     l_lock_image_id;
  char       *l_env;

  gpp = &global_params;
  *nreturn_vals = 1;
  *return_vals = values;
  l_rc = 0;

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_SUCCESS;

  l_env = (char*) g_getenv("GAP_DEBUG");
  if(l_env != NULL)
  {
    if((*l_env != 'n') && (*l_env != 'N')) gap_debug = 1;
  }

  if(gap_debug) fprintf(stderr, "\n\ngap_onion_main: debug name = %s\n", name);

  gpp->run_mode = param[0].data.d_int32;
  INIT_I18N();


  p_init_default_values(gpp); /* init with default values */
  p_get_data_onion_cfg(gpp);  /* get current params (if there are any) */

  gpp->cache.count = 0;    /* start with empty image cache */

  /* get image_ID */
  gpp->image_ID    = param[1].data.d_image;
  l_lock_image_id  = gpp->image_ID;

  /* ---------------------------
   * check for LOCKS
   * ---------------------------
   */
  if(p_gap_lock_is_locked(l_lock_image_id, gpp->run_mode))
  {
       values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
       return ;
  }

  /* set LOCK on current image (for all gap_plugins) */
  p_gap_lock_set(l_lock_image_id);


  /* get animinfo */
  p_plug_in_gap_get_animinfo(gpp->image_ID, &gpp->ainfo);
  gpp->range_from            = gpp->ainfo.curr_frame_nr;
  gpp->range_to              = gpp->ainfo.last_frame_nr;

  if (strcmp (name, GAP_PLUGIN_NAME_ONION_CFG) == 0)
  {
    /* -----------------------
     * CONFIG
     * -----------------------
     */
      if (gpp->run_mode == GIMP_RUN_NONINTERACTIVE)
      {
        if (n_params != nargs_onion_cfg)
        {
          values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
        }
        else
        {
          gpp->val.num_olayers        = param[3].data.d_int32;
          gpp->val.ref_delta          = param[4].data.d_int32;
          gpp->val.ref_cycle          = param[5].data.d_int32;
          gpp->val.stack_pos          = param[6].data.d_int32;
          gpp->val.stack_top          = param[7].data.d_int32;
          gpp->val.opacity            = param[8].data.d_float;
          gpp->val.opacity_delta      = param[9].data.d_float;
          gpp->val.ignore_botlayers   = param[10].data.d_int32;
          gpp->val.select_mode        = param[11].data.d_int32;
          gpp->val.select_case        = param[12].data.d_int32;
          gpp->val.select_invert      = param[13].data.d_int32;
          if (param[14].data.d_string != NULL)
          {
            g_snprintf(&gpp->val.select_string[0]
                      , sizeof(gpp->val.select_string)
                      , "%s", param[14].data.d_string
                      );
          }
          gpp->range_from            = param[15].data.d_int32;
          gpp->range_to              = param[16].data.d_int32;
          gpp->val.run               = param[17].data.d_int32;
          gpp->val.farn_opaque       = param[18].data.d_int32;
        }
      }
      else if(gpp->run_mode != GIMP_RUN_INTERACTIVE)
      {
         values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
      }

      if (values[0].data.d_status == GIMP_PDB_SUCCESS)
      {
         if(gpp->run_mode == GIMP_RUN_INTERACTIVE)
         {
            l_rc = p_onion_cfg_dialog (gpp);
            if(gap_debug) printf("MAIN after p_onion_cfg_dialog ------------------\n");
         }
         if (l_rc >= 0)
         {
           if(gpp->val.run != GAP_ONION_RUN_CANCEL)
           {
             l_rc = p_set_data_onion_cfg(gpp, GAP_PLUGIN_NAME_ONION_CFG);
           }
           if((gpp->val.run == GAP_ONION_RUN_APPLY)
           || (gpp->val.run == GAP_ONION_RUN_DELETE))
           {
             l_rc = p_onion_range(gpp);
           }
         }
      }
  }
  else if (strcmp (name, GAP_PLUGIN_NAME_ONION_APPLY) == 0)
  {
    /* -----------------------
     * MAKE
     * -----------------------
     * store params also with name GAP_PLUGIN_NAME_ONION_APPLY
     * This makes it possible, to call this plugin as filter
     * for a selected range of frames.
     * (using plug_in_gap_modify  and selecting plug_in_onionskin_make as filter)
     */
     p_set_data_onion_cfg(gpp, GAP_PLUGIN_NAME_ONION_APPLY);
     l_rc = p_onion_apply(gpp, FALSE /* do not use_cache */ );
  }
  else if (strcmp (name, GAP_PLUGIN_NAME_ONION_DEL) == 0)
  {
    /* -----------------------
     * DEL
     * -----------------------
     */
     p_set_data_onion_cfg(gpp, GAP_PLUGIN_NAME_ONION_DEL);
     l_rc = p_onion_delete(gpp);
  }
  else if (strcmp (name, GAP_PLUGIN_NAME_ONION_VISI) == 0)
  {
    /* -----------------------
     * VISI
     * -----------------------
     */
     gint32  l_visi_mode;

     l_visi_mode = GAP_ONION_VISI_TOGGLE;  /* interactive mode always uses toggle */

     if (gpp->run_mode == GIMP_RUN_NONINTERACTIVE)
     {
        if (n_params != nargs_onion_visi)
        {
          values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
        }
        else
        {
          l_visi_mode  = param[3].data.d_int32;
        }
     }

     if (values[0].data.d_status == GIMP_PDB_SUCCESS)
     {
        p_set_data_onion_cfg(gpp, GAP_PLUGIN_NAME_ONION_VISI);
        l_rc = p_onion_visibility(gpp, l_visi_mode);
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

  if (gpp->run_mode != GIMP_RUN_NONINTERACTIVE)
  {
    gimp_displays_flush();
  }

  /* remove LOCK on this image for all gap_plugins */
  p_gap_lock_remove(l_lock_image_id);
}       /* end run */
