/* gap_mod_layer.c
 * 1998.10.14 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * This Module contains:
 * modify Layer (perform actions (like raise, set visible, apply filter)
 *               - foreach selected layer
 *               - in each frame of the selected framerange)
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
 * gimp   1.3.25a;   2004/01/21  hof: message text fixes (# 132030)
 * gimp   1.3.24a;   2004/01/17  hof: added Layermask Handling
 * gimp   1.3.20d;   2003/09/20  hof: sourcecode cleanup,
 *                                    added new Actionmodes for Selction handling
 * gimp   1.3.20b;   2003/09/20  hof: gap_db_browser_dialog new param image_id
 * gimp   1.3.15a;   2003/06/21  hof: textspacing
 * gimp   1.3.14a;   2003/05/17  hof: placed OK button right.
 * gimp   1.3.12a;   2003/05/01  hof: merge into CVS-gimp-gap project
 * gimp   1.3.11a;   2003/01/18  hof: Conditional framesave
 * gimp   1.3.8a;    2002/09/21  hof: gap_lastvaldesc
 * gimp   1.3.4b;    2002/03/24  hof: support COMMON_ITERATOR
 * gimp   1.3.4a;    2002/03/12  hof: replaced private pdb-wrappers
 * gimp   1.1.29b;   2000/11/30  hof: use g_snprintf
 * gimp   1.1.28a;   2000/11/05  hof: check for GIMP_PDB_SUCCESS (not for FALSE)
 * gimp   1.1.6;     1999/06/21  hof: bugix: wrong iterator total_steps and direction
 * gimp   1.1.15.1;  1999/05/08  hof: bugix (dont mix GimpImageType with GimpImageBaseType)
 * version 0.98.00   1998.11.27  hof: - use new module gap_pdb_calls.h
 * version 0.97.00   1998.10.19  hof: - created module
 */

/* SYTEM (UNIX) includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

/* GIMP includes */
#include "gtk/gtk.h"
#include "config.h"
#include "gap-intl.h"
#include "libgimp/gimp.h"

/* GAP includes */
#include "gap_lastvaldesc.h"
#include "gap_arr_dialog.h"
#include "gap_filter.h"
#include "gap_filter_pdb.h"
#include "gap_dbbrowser_utils.h"
#include "gap_pdb_calls.h"
#include "gap_match.h"
#include "gap_lib.h"
#include "gap_layer_copy.h"
#include "gap_image.h"
#include "gap_range_ops.h"
#include "gap_mod_layer.h"


extern      int gap_debug; /* ==0  ... dont print debug infos */


/* ============================================================================
 * p_layer_modify_dialog
 *   retcode
 *               0   ... Generate Paramfile
 *               1   ... Generate Paramfile and start mpeg_encode
 * ============================================================================
 */
static
int p_layer_modify_dialog(GapAnimInfo *ainfo_ptr,
                   gint32 *range_from,  gint32 *range_to,
                   gint32 *action_mode, gint32 *sel_mode,
                   gint32 *sel_case,    gint32 *sel_invert,
                   char *sel_pattern,   char   *new_layername)
{
  static GapArrArg  argv[9];
  static GapArrButtonArg  b_argv[2];
  gint   l_rc;

  /* Layer select modes */
  static char *sel_args[7]    = { N_("Pattern is equal to layer name"),
                                  N_("Pattern is start of layer name"),
                                  N_("Pattern is end of layer name"),
                                  N_("Pattern is a part of layer name"),
                                  N_("Pattern is a list of layerstack numbers"),
                                  N_("Pattern is a list of reverse layerstack numbers"),
                                  N_("All visible (ignore pattern)")
                                  };
  static char *sel_help[7]    = { N_("Select all layers where layer name is equal to pattern"),
                                  N_("Select all layers where layer name starts with pattern"),
                                  N_("Select all layers where layer name ends up with pattern"),
                                  N_("Select all layers where layer name contains pattern"),
                                  N_("Select layerstack positions where 0 is the top layer.\nExample: 0, 4-5, 8"),
                                  N_("Select layerstack positions where 0 is the background layer.\nExample: 0, 4-5, 8"),
                                  N_("Select all visible layers")
                                  };

  /* action items what to do with the selected layer(s) */
  static char *action_args[32]= { N_("Set layer(s) visible"),
                                  N_("Set layer(s) invisible"),
                                  N_("Set layer(s) linked"),
                                  N_("Set layer(s) unlinked"),
                                  N_("Raise layer(s)"),
                                  N_("Lower layer(s)"),
                                  N_("Merge layer(s) expand as necessary"),
                                  N_("Merge layer(s) clipped to image"),
                                  N_("Merge layer(s) clipped to bg-layer"),
                                  N_("Apply filter on layer(s)"),
                                  N_("Duplicate layer(s)"),
                                  N_("Delete layer(s)"),
                                  N_("Rename layer(s)"),
                                  N_("Replace selection"),
                                  N_("Add selection"),
                                  N_("Subtract selection"),
                                  N_("Intersect selection"),
                                  N_("Selection none"),
                                  N_("Selection all"),
                                  N_("Selection invert"),
                                  N_("Save selection to channel"),
				  N_("Load selection from channel"),
                                  N_("Delete channel (by name)"),
				  N_("Add alpha channel"),
				  N_("Add white layermask (opaque)"),
				  N_("Add black layermask (transparent)"),
				  N_("Add layermask from alpha"),
				  N_("Add layermask transfer from alpha"),
				  N_("Add layermask from selection"),
				  N_("Add layermask from bw copy"),
				  N_("Delete layermask"),
				  N_("Apply layermask")
                                  };


/*
  static char *action_help[32]  = {"set all selected layers visible",
                                  "set all selected layers  invisible",
                                  "set all selected layers  linked",
                                  "set all selected layers  unlinked",
                                  "raise all selected layers",
                                  "lower all selected layers",
                                  "merge expand as necessary",
                                  "merge clipped to image",
                                  "merge clipped to bg-layer",
                                  "APPLY FILTER to all selected layers",
                                  "duplicate all selected layers",
                                  "delete REMOVES all selected layers",
                                  "rename all selected layers\nto NewLayername.",

				  "Replace Selection by Selection of the invoking Frame Image",
				  "Add Selection",
				  "Subtract Selection",
				  "Intersect Selection",
				  "Clear Selection in all handled Frames",
				  "Select All in all handled Frames",
				  "Invert Selection in all handled Frames",
				  "Save Selection as Channel",
				  "Load Selection from Channel",
				  "Delete Channel by Name"
                                  };
*/

  guint get_textize_loop;

  for (get_textize_loop = 0; get_textize_loop < G_N_ELEMENTS (sel_args) ; get_textize_loop++) {
    sel_args[get_textize_loop] = gettext(sel_args[get_textize_loop]);
    sel_help[get_textize_loop] = gettext(sel_help[get_textize_loop]);
  }
  for (get_textize_loop = 0; get_textize_loop < G_N_ELEMENTS (action_args); get_textize_loop++)
    action_args[get_textize_loop] = gettext(action_args[get_textize_loop]);

  l_rc = -1;

  /* the 2 Action Buttons */
    b_argv[0].but_txt  = GTK_STOCK_CANCEL;
    b_argv[0].but_val  = -1;
    b_argv[1].but_txt  = GTK_STOCK_OK;
    b_argv[1].but_val  = 0;

  gap_arr_arg_init(&argv[0], GAP_ARR_WGT_LABEL);
  argv[0].label_txt = _("Perform function on one or more layer(s)\n"
                        "in all frames of the selected frame range\n");

  gap_arr_arg_init(&argv[1], GAP_ARR_WGT_INT_PAIR);
  argv[1].constraint = TRUE;
  argv[1].label_txt = _("From Frame:");
  argv[1].help_txt  = _("First handled frame");
  argv[1].int_min   = (gint)ainfo_ptr->first_frame_nr;
  argv[1].int_max   = (gint)ainfo_ptr->last_frame_nr;
  argv[1].int_ret   = (gint)ainfo_ptr->curr_frame_nr;

  gap_arr_arg_init(&argv[2], GAP_ARR_WGT_INT_PAIR);
  argv[2].constraint = TRUE;
  argv[2].label_txt = _("To Frame:");
  argv[2].help_txt  = _("Last handled frame");
  argv[2].int_min   = (gint)ainfo_ptr->first_frame_nr;
  argv[2].int_max   = (gint)ainfo_ptr->last_frame_nr;
  argv[2].int_ret   = (gint)ainfo_ptr->last_frame_nr;


  /* Layer select mode RADIO buttons */
  gap_arr_arg_init(&argv[3], GAP_ARR_WGT_RADIO);
  argv[3].label_txt = _("Layer Selection:");
  argv[3].radio_argc = G_N_ELEMENTS (sel_args);
  argv[3].radio_argv = sel_args;
  argv[3].radio_help_argv = sel_help;
  argv[3].radio_ret  = 4;

  /* Layer select pattern string */
  sel_pattern[0] = '0';
  sel_pattern[1] = '\0';
  gap_arr_arg_init(&argv[4], GAP_ARR_WGT_TEXT);
  argv[4].label_txt = _("Layer Pattern:");
  argv[4].entry_width = 140;       /* pixel */
  argv[4].help_txt  = _("String to identify layer names or layerstack position numbers. Example: 0,3-5");
  argv[4].text_buf_len = MAX_LAYERNAME;
  argv[4].text_buf_ret = sel_pattern;

  /* case sensitive checkbutton */
  gap_arr_arg_init(&argv[5], GAP_ARR_WGT_TOGGLE);
  argv[5].label_txt = _("Case sensitive");
  argv[5].help_txt  = _("Lowercase and uppercase letters are considered as different");
  argv[5].int_ret   = 1;

  /* invert selection checkbutton */
  gap_arr_arg_init(&argv[6], GAP_ARR_WGT_TOGGLE);
  argv[6].label_txt = _("Invert Layer Selection");
  argv[6].help_txt  = _("Perform actions on all unselected layers");
  argv[6].int_ret   = 0;

  /* desired action to perform OPTIONMENU  */
  gap_arr_arg_init(&argv[7], GAP_ARR_WGT_OPTIONMENU);
  argv[7].label_txt = _("Function:");
  argv[7].radio_argc = G_N_ELEMENTS (action_args);
  argv[7].radio_argv = action_args;
  /* argv[7].radio_help_argv = action_help */
  argv[7].help_txt = _("Function to be performed on all selected layers");
  argv[7].radio_ret  = 0;

  /* a new name for the handled Layer(s) */
  *new_layername = '\0';
  gap_arr_arg_init(&argv[8], GAP_ARR_WGT_TEXT);
  argv[8].label_txt = _("Layer Name\n or Channel Name:");
  argv[8].entry_width = 140;       /* pixel */
  argv[8].help_txt  = _("Layer (or channel) name for all handled layers (or channels),\n"
                        "where the string '[######]' is replaced by the frame number.\n"
			"This name is only relevant for the functions:\n"
			" Duplicate layer(s)\n"
			" Rename layer(s)\n"
			" Save selection to channel\n"
			" Load selection from channel\n"
			" Delete channel (by name)"
			);
  argv[8].text_buf_len = MAX_LAYERNAME;
  argv[8].text_buf_ret = new_layername;


  l_rc =  gap_arr_std_dialog( _("Frames Modify"),
                                _("Settings"),
                                 9,   argv,      /* widget array */
                                 2,   b_argv,    /* button array */
                                 0);

  /* return the entered values */
  *range_from           = argv[1].int_ret;
  *range_to             = argv[2].int_ret;
  *sel_mode             = argv[3].int_ret;
                       /*     [4] sel_pattern  */
  *sel_case             = argv[5].int_ret;
  *sel_invert           = argv[6].int_ret;
  *action_mode          = argv[7].int_ret;
                       /*     [8] l_new_layername */


  return (l_rc);
}	/* end p_layer_modify_dialog */


/* ============================================================================
 * p_pitstop_dialog
 *   return -1  on CANCEL
 *           0  on Continue (OK)
 * ============================================================================
 */
static gint
p_pitstop_dialog(gint text_flag, char *filter_procname)
{
  const gchar      *l_env;
  gchar            *l_msg;
  static GapArrButtonArg  l_but_argv[2];
  gint              l_but_argc;
  gint              l_argc;
  static GapArrArg  l_argv[1];
  gint              l_continue;




  l_but_argv[0].but_txt  = _("Continue");
  l_but_argv[0].but_val  = 0;
  l_but_argv[1].but_txt  = GTK_STOCK_CANCEL;
  l_but_argv[1].but_val  = -1;

  l_but_argc = 2;
  l_argc = 0;

  /* optional dialog between both calls (to see the effect of 1.call) */
  l_env = g_getenv("GAP_FILTER_PITSTOP");
  if(l_env != NULL)
  {
     if((*l_env == 'N') || (*l_env == 'n'))
     {
       return 0;  /* continue without question */
     }
  }
  if(text_flag == 0)
  {
     l_msg = g_strdup_printf (_("2nd call of %s\n(define end-settings)"), filter_procname);
  }
  else
  {
     l_msg = g_strdup_printf ( _("Non-Interactive call of %s\n(for all selected layers)"), filter_procname);
  }
  l_continue = gap_arr_std_dialog ( _("Animated Filter Apply"), l_msg,
  				   l_argc,     l_argv,
  				   l_but_argc, l_but_argv, 0);
  g_free (l_msg);

  return (l_continue);

}	/* end p_pitstop_dialog */


/* ============================================================================
 * gap_mod_get_1st_selected
 *   return index of the 1.st selected layer
 *   or -1 if no selection was found
 * ============================================================================
 */
int
gap_mod_get_1st_selected (GapModLayliElem * layli_ptr, gint nlayers)
{
  int  l_idx;

  for(l_idx = 0; l_idx < nlayers; l_idx++)
  {
    if(layli_ptr[l_idx].selected != FALSE)
    {
      return (l_idx);
    }
  }
  return(-1);
}	/* end gap_mod_get_1st_selected */


/* ============================================================================
 * gap_mod_alloc_layli
 * returns   pointer to a new allocated image_id of the new created multilayer image
 *           (or NULL on error)
 * ============================================================================
 */

GapModLayliElem *
gap_mod_alloc_layli(gint32 image_id, gint32 *l_sel_cnt, gint *nlayers,
              gint32 sel_mode,
              gint32 sel_case,
	      gint32 sel_invert,
              char *sel_pattern )
{
  gint32 *l_layers_list;
  gint32  l_layer_id;
  gint32  l_idx;
  GapModLayliElem *l_layli_ptr;
  char      *l_layername;

  *l_sel_cnt = 0;

  l_layers_list = gimp_image_get_layers(image_id, nlayers);
  if(l_layers_list == NULL)
  {
    return(NULL);
  }

  l_layli_ptr = g_new0(GapModLayliElem, (*nlayers));
  if(l_layli_ptr == NULL)
  {
     g_free (l_layers_list);
     return(NULL);
  }

  for(l_idx = 0; l_idx < (*nlayers); l_idx++)
  {
    l_layer_id = l_layers_list[l_idx];
    l_layername = gimp_drawable_get_name(l_layer_id);
    l_layli_ptr[l_idx].layer_id  = l_layer_id;
    l_layli_ptr[l_idx].visible   = gimp_drawable_get_visible(l_layer_id);
    l_layli_ptr[l_idx].selected  = gap_match_layer(l_idx,
                                                 l_layername,
                                                 sel_pattern,
						 sel_mode,
						 sel_case,
						 sel_invert,
						 *nlayers, l_layer_id);
    if(l_layli_ptr[l_idx].selected != FALSE)
    {
      (*l_sel_cnt)++;  /* count all selected layers */
    }
    if(gap_debug) printf("gap: gap_mod_alloc_layli [%d] id:%d, sel:%d name:%s:\n",
                         (int)l_idx, (int)l_layer_id,
			 (int)l_layli_ptr[l_idx].selected, l_layername);
    g_free (l_layername);
  }

  g_free (l_layers_list);

  return( l_layli_ptr );
}		/* end gap_mod_alloc_layli */


/* ============================================================================
 * p_raise_layer
 *   raise layer (check if possible before)
 *   (without the check each failed attempt would open an inf window)
 * ============================================================================
 */
static void
p_raise_layer (gint32 image_id, gint32 layer_id, GapModLayliElem * layli_ptr, gint nlayers)
{
  if(! gimp_drawable_has_alpha (layer_id)) return; /* has no alpha channel */

  if(layli_ptr[0].layer_id == layer_id)  return;   /* is already on top */

  gimp_image_raise_layer(image_id, layer_id);
}	/* end p_raise_layer */

static void
p_lower_layer (gint32 image_id, gint32 layer_id, GapModLayliElem * layli_ptr, gint nlayers)
{
  if(! gimp_drawable_has_alpha (layer_id)) return; /* has no alpha channel */

  if(layli_ptr[nlayers-1].layer_id == layer_id)  return;   /* is already on bottom */

  if(nlayers > 1)
  {
    if((layli_ptr[nlayers-2].layer_id == layer_id)
    && (! gimp_drawable_has_alpha (layli_ptr[nlayers-1].layer_id)))
    {
      /* the layer is one step above a "bottom-layer without alpha" */
      return;
    }
  }

  gimp_image_lower_layer(image_id, layer_id);
}	/* end p_lower_layer */


/* ============================================================================
 * p_selection_combine
 *
 *    combine selections
 * ============================================================================
 */
static void
p_selection_combine(gint32 image_id
                   ,gint32 master_channel_id
		   ,GimpChannelOps  operation
		   )
{
  gint32   l_new_channel_id;
  gint32   l_sel_channel_id;

  l_sel_channel_id = gimp_image_get_selection(image_id);
  l_new_channel_id = gimp_channel_copy(l_sel_channel_id);

  /* copy the initial selection master_channel_id
   * to the newly create image
   */
  gap_layer_copy_content( l_new_channel_id      /* dst_drawable_id  */
                        , master_channel_id   /* src_drawable_id  */
			);
  gimp_channel_combine_masks(l_sel_channel_id
                            ,l_new_channel_id
			    ,operation
			    , 0
			    , 0
			    );
  gimp_drawable_delete(l_new_channel_id);

}  /* end p_selection_combine */

/* ============================================================================
 * p_apply_action
 *
 *    perform function (defined by action_mode)
 *    on all selcted layer(s)
 *
 * returns   0 if all done OK
 *           (or -1 on error)
 * ============================================================================
 */

static int
p_apply_action(gint32 image_id,
	      gint32 action_mode,
	      GapModLayliElem *layli_ptr,
	      gint nlayers,
	      gint32 sel_cnt,

	      long  from,
	      long  to,
	      long  curr,
	      char *new_layername,
	      char *filter_procname,
	      gint32 master_image_id
	      )
{
  int   l_idx;
  int   l_rc;
  gint32  l_layer_id;
  gint32  l_layermask_id;
  gint32  l_new_layer_id;
  gint    l_merge_mode;
  gint    l_vis_result;
  char    l_name_buff[MAX_LAYERNAME];
  gint32  l_master_channel_id;

  if(gap_debug) fprintf(stderr, "gap: p_apply_action START\n");

  l_rc = 0;
  l_merge_mode = -44; /* none of the flatten modes */

  if(action_mode == GAP_MOD_ACM_MERGE_EXPAND) l_merge_mode = GAP_RANGE_OPS_FLAM_MERG_EXPAND;
  if(action_mode == GAP_MOD_ACM_MERGE_IMG)    l_merge_mode = GAP_RANGE_OPS_FLAM_MERG_CLIP_IMG;
  if(action_mode == GAP_MOD_ACM_MERGE_BG)     l_merge_mode = GAP_RANGE_OPS_FLAM_MERG_CLIP_BG;

  l_master_channel_id = gimp_image_get_selection(master_image_id);


  /* merge actions require one call per image */
  if(l_merge_mode != (-44))
  {
      if(sel_cnt < 2)
      {
        return(0);  /* OK, nothing to merge */
      }

     l_vis_result = FALSE;

     /* set selected layers visible, all others invisible for merge */
     for(l_idx = 0; l_idx < nlayers; l_idx++)
     {
       if(layli_ptr[l_idx].selected == FALSE)
       {
          gimp_drawable_set_visible(layli_ptr[l_idx].layer_id, FALSE);
       }
       else
       {
          if(gimp_drawable_get_visible(layli_ptr[l_idx].layer_id))
	  {
	    /* result will we visible if at least one of the
	     * selected layers was visible before
	     */
	    l_vis_result = TRUE;
	  }
          gimp_drawable_set_visible(layli_ptr[l_idx].layer_id, TRUE);
       }
     }

     /* merge all visible layers (i.e. all selected layers) */
     l_layer_id = gimp_image_merge_visible_layers (image_id, l_merge_mode);
     if(l_vis_result == FALSE)
     {
        gimp_drawable_set_visible(l_layer_id, FALSE);
     }

     /* if new_layername is available use that name
      * for the new merged layer
      */
     if (!gap_match_string_is_empty (new_layername))
     {
	 gap_match_substitute_framenr(&l_name_buff[0], sizeof(l_name_buff),
	                      new_layername, curr);
	 gimp_drawable_set_name(l_layer_id, &l_name_buff[0]);
     }

     /* restore visibility flags after merge */
     for(l_idx = 0; l_idx < nlayers; l_idx++)
     {
       if(layli_ptr[l_idx].selected == FALSE)
       {
         gimp_drawable_set_visible(layli_ptr[l_idx].layer_id,
                                   layli_ptr[l_idx].visible);
       }
     }

     return(0);
  }

  /* -----------------------------*/
  /* non-merge actions require calls foreach selected layer */
  for(l_idx = 0; (l_idx < nlayers) && (l_rc == 0); l_idx++)
  {
    l_layer_id = layli_ptr[l_idx].layer_id;

    /* apply function defined by action_mode */
    if(layli_ptr[l_idx].selected != FALSE)
    {
      if(gap_debug) fprintf(stderr, "gap: p_apply_action on selected LayerID:%d layerstack:%d\n",
                           (int)l_layer_id, (int)l_idx);
      switch(action_mode)
      {
        case GAP_MOD_ACM_SET_VISIBLE:
          gimp_drawable_set_visible(l_layer_id, TRUE);
	  break;
        case GAP_MOD_ACM_SET_INVISIBLE:
          gimp_drawable_set_visible(l_layer_id, FALSE);
	  break;
        case GAP_MOD_ACM_SET_LINKED:
          gimp_drawable_set_linked(l_layer_id, TRUE);
	  break;
        case GAP_MOD_ACM_SET_UNLINKED:
          gimp_drawable_set_linked(l_layer_id, FALSE);
	  break;
        case GAP_MOD_ACM_RAISE:
	  p_raise_layer(image_id, l_layer_id, layli_ptr, nlayers);
	  break;
        case GAP_MOD_ACM_LOWER:
	  p_lower_layer(image_id, l_layer_id, layli_ptr, nlayers);
	  break;
        case GAP_MOD_ACM_APPLY_FILTER:
	  l_rc = gap_filt_pdb_call_plugin(filter_procname,
	                       image_id,
			       l_layer_id,
			       GIMP_RUN_WITH_LAST_VALS);
          if(gap_debug) fprintf(stderr, "gap: p_apply_action FILTER:%s rc =%d\n",
                                filter_procname, (int)l_rc);
	  break;
        case GAP_MOD_ACM_DUPLICATE:
	  l_new_layer_id = gimp_layer_copy(l_layer_id);
	  gimp_image_add_layer (image_id, l_new_layer_id, -1);
	  if (!gap_match_string_is_empty (new_layername))
	  {
	      gap_match_substitute_framenr(&l_name_buff[0], sizeof(l_name_buff),
	                           new_layername, curr);
	      gimp_drawable_set_name(l_new_layer_id, &l_name_buff[0]);
	  }
	  break;
        case GAP_MOD_ACM_DELETE:
	  gimp_image_remove_layer(image_id, l_layer_id);
	  break;
        case GAP_MOD_ACM_RENAME:
	  gap_match_substitute_framenr(&l_name_buff[0], sizeof(l_name_buff),
	                        new_layername, curr);
	  gimp_drawable_set_name(l_layer_id, &l_name_buff[0]);
	  break;
        case GAP_MOD_ACM_SEL_REPLACE:
	  {
            gint32 l_sel_channel_id;

	    gimp_selection_all(image_id);
            l_sel_channel_id = gimp_image_get_selection(image_id);
            /* copy the initial selection l_master_channel_id
	     * to the newly create image
	     */
            gap_layer_copy_content( l_sel_channel_id      /* dst_drawable_id  */
                                  , l_master_channel_id   /* src_drawable_id  */
			          );
	  }
	  break;
        case GAP_MOD_ACM_SEL_ADD:
	  p_selection_combine(image_id, l_master_channel_id, GIMP_CHANNEL_OP_ADD);
	  break;
        case GAP_MOD_ACM_SEL_SUTRACT:
	  p_selection_combine(image_id, l_master_channel_id, GIMP_CHANNEL_OP_SUBTRACT);
	  break;
        case GAP_MOD_ACM_SEL_INTERSECT:
	  p_selection_combine(image_id, l_master_channel_id, GIMP_CHANNEL_OP_INTERSECT);
	  break;
        case GAP_MOD_ACM_SEL_NONE:
	  gimp_selection_none(image_id);
	  break;
        case GAP_MOD_ACM_SEL_ALL:
	  gimp_selection_all(image_id);
	  break;
        case GAP_MOD_ACM_SEL_INVERT:
	  gimp_selection_invert(image_id);
	  break;
        case GAP_MOD_ACM_SEL_SAVE:
          {
	    gint32 l_sel_channel_id;
	    l_sel_channel_id = gimp_selection_save(image_id);
            if(*new_layername != '\0')
	    {
	      gap_match_substitute_framenr(&l_name_buff[0], sizeof(l_name_buff),
	                                   new_layername, curr);
	      gimp_drawable_set_name(l_sel_channel_id, &l_name_buff[0]);
	    }
	  }
	  break;
        case GAP_MOD_ACM_SEL_LOAD:
          {
            if(*new_layername != '\0')
	    {
	      gint   *l_channels;
	      gint    n_channels;
	      gint    l_ii;
	      gchar  *l_channelname;

	      gap_match_substitute_framenr(&l_name_buff[0], sizeof(l_name_buff),
	                                   new_layername, curr);

	      l_channels = gimp_image_get_channels(image_id, &n_channels);
	      for(l_ii=0; l_ii < n_channels; l_ii++)
	      {
	        l_channelname = gimp_drawable_get_name(l_channels[l_ii]);
		if(l_channelname)
		{
		  if(strcmp(l_channelname,&l_name_buff[0] ) == 0)
		  {
                    gimp_selection_load(l_channels[l_ii]);
		    g_free(l_channelname);
		    break;
		  }
		  g_free(l_channelname);
		}
	      }
	      if(l_channels)
	      {
	        g_free(l_channels);
	      }
	    }
	  }
	  break;
        case GAP_MOD_ACM_SEL_DELETE:
          {
            if(*new_layername != '\0')
	    {
	      gint   *l_channels;
	      gint    n_channels;
	      gint    l_ii;
	      gchar  *l_channelname;

	      gap_match_substitute_framenr(&l_name_buff[0], sizeof(l_name_buff),
	                                   new_layername, curr);

	      l_channels = gimp_image_get_channels(image_id, &n_channels);
	      for(l_ii=0; l_ii < n_channels; l_ii++)
	      {
	        l_channelname = gimp_drawable_get_name(l_channels[l_ii]);
		if(l_channelname)
		{
		  if(strcmp(l_channelname,&l_name_buff[0] ) == 0)
		  {
                    gimp_image_remove_channel(image_id, l_channels[l_ii]);
		  }
		  g_free(l_channelname);
		}
	      }
	      if(l_channels)
	      {
	        g_free(l_channels);
	      }
	    }
	  }
	  break;
        case GAP_MOD_ACM_ADD_ALPHA:
	  if(!gimp_drawable_has_alpha(l_layer_id))
	  {
	    gimp_layer_add_alpha(l_layer_id);
	  }
	  break;
        case GAP_MOD_ACM_LMASK_WHITE:
	  if(gimp_layer_get_mask(l_layer_id) >= 0)
	  {
	    gimp_layer_remove_mask (l_layer_id, GIMP_MASK_APPLY);
	  }
	  l_layermask_id = gimp_layer_create_mask(l_layer_id, GIMP_ADD_WHITE_MASK);
	  gimp_layer_add_mask(l_layer_id, l_layermask_id);
	  break;
        case GAP_MOD_ACM_LMASK_BLACK:
	  if(gimp_layer_get_mask(l_layer_id) >= 0)
	  {
	    gimp_layer_remove_mask (l_layer_id, GIMP_MASK_APPLY);
	  }
	  l_layermask_id = gimp_layer_create_mask(l_layer_id, GIMP_ADD_BLACK_MASK);
	  gimp_layer_add_mask(l_layer_id, l_layermask_id);
	  break;
        case GAP_MOD_ACM_LMASK_ALPHA:
	  if(gimp_layer_get_mask(l_layer_id) >= 0)
	  {
	    gimp_layer_remove_mask (l_layer_id, GIMP_MASK_APPLY);
	  }
	  l_layermask_id = gimp_layer_create_mask(l_layer_id, GIMP_ADD_ALPHA_MASK);
	  gimp_layer_add_mask(l_layer_id, l_layermask_id);
	  break;
        case GAP_MOD_ACM_LMASK_TALPHA:
	  if(gimp_layer_get_mask(l_layer_id) >= 0)
	  {
	    gimp_layer_remove_mask (l_layer_id, GIMP_MASK_APPLY);
	  }
	  l_layermask_id = gimp_layer_create_mask(l_layer_id, GIMP_ADD_ALPHA_TRANSFER_MASK);
	  gimp_layer_add_mask(l_layer_id, l_layermask_id);
	  break;
        case GAP_MOD_ACM_LMASK_SEL:
	  if(gimp_layer_get_mask(l_layer_id) >= 0)
	  {
	    gimp_layer_remove_mask (l_layer_id, GIMP_MASK_APPLY);
	  }
	  l_layermask_id = gimp_layer_create_mask(l_layer_id, GIMP_ADD_SELECTION_MASK);
	  break;
        case GAP_MOD_ACM_LMASK_BWCOPY:
	  if(gimp_layer_get_mask(l_layer_id) >= 0)
	  {
	    gimp_layer_remove_mask (l_layer_id, GIMP_MASK_APPLY);
	  }
	  l_layermask_id = gimp_layer_create_mask(l_layer_id, GIMP_ADD_COPY_MASK);
	  gimp_layer_add_mask(l_layer_id, l_layermask_id);
	  break;
        case GAP_MOD_ACM_LMASK_DELETE:
	  if(gimp_layer_get_mask(l_layer_id) >= 0)
	  {
	    gimp_layer_remove_mask (l_layer_id, GIMP_MASK_DISCARD);
	  }
	  break;
        case GAP_MOD_ACM_LMASK_APPLY:
	  if(gimp_layer_get_mask(l_layer_id) >= 0)
	  {
	    gimp_layer_remove_mask (l_layer_id, GIMP_MASK_APPLY);
	  }
	  break;
        default:
	  break;
      }
    }
  }

  return (l_rc);
}	/* end p_apply_action */


/* ============================================================================
 * p_do_filter_dialogs
 *    additional dialog steps
 *    a) gap_pdb_browser (select the filter)
 *    b) 1st interactive filtercall
 *    c) 1st pitstop dialog
 * ============================================================================
 */
static int
p_do_filter_dialogs(GapAnimInfo *ainfo_ptr,
                    gint32 image_id, gint32 *dpy_id,
                    GapModLayliElem * layli_ptr, gint nlayers ,
                    char *filter_procname, int filt_len,
		    gint *plugin_data_len,
		    GapFiltPdbApplyMode *apply_mode
		    )
{
  GapDbBrowserResult  l_browser_result;
  gint32   l_layer_id;
  int      l_rc;
  int      l_idx;
  static char l_key_from[512];

  /* GAP-PDB-Browser Dialog */
  /* ---------------------- */
  if(gap_db_browser_dialog( _("Select Filter for Animated Apply on Frames"),
                            _("Apply Constant"),
                            _("Apply Varying"),
                            gap_filt_pdb_constraint_proc,
                            gap_filt_pdb_constraint_proc_sel1,
                            gap_filt_pdb_constraint_proc_sel2,
                            &l_browser_result,
			    image_id) < 0)
  {
      if(gap_debug) fprintf(stderr, "DEBUG: gap_db_browser_dialog cancelled\n");
      return -1;
  }

  strncpy(filter_procname, l_browser_result.selected_proc_name, filt_len-1);
  filter_procname[filt_len-1] = '\0';
  if(l_browser_result.button_nr == 1) *apply_mode = GAP_PAPP_VARYING_LINEAR;
  else                                *apply_mode = GAP_PAPP_CONSTANT;

  /* 1.st INTERACTIV Filtercall dialog */
  /* --------------------------------- */
  /* check for the Plugin */

  l_rc = gap_filt_pdb_procedure_available(filter_procname, GAP_PTYP_CAN_OPERATE_ON_DRAWABLE);
  if(l_rc < 0)
  {
     fprintf(stderr, "ERROR: Plugin not available or wrong type %s\n", filter_procname);
     return -1;
  }

  /* get 1.st selected layer (of 1.st handled frame in range ) */
  l_idx = gap_mod_get_1st_selected(layli_ptr, nlayers);
  if(l_idx < 0)
  {
     fprintf(stderr, "ERROR: No layer selected in 1.st handled frame\n");
     return (-1);
  }
  l_layer_id = layli_ptr[l_idx].layer_id;

  /* open a view for the 1.st handled frame */
  *dpy_id = gimp_display_new (image_id);

  l_rc = gap_filt_pdb_call_plugin(filter_procname, image_id, l_layer_id, GIMP_RUN_INTERACTIVE);

  /* OOPS: cant delete the display here, because
   *       closing the last display seems to free up
   *       at least parts of the image,
   *       and causes crashes if the image_id is used
   *       in further gimp procedures
   */
  /* gimp_display_delete(*dpy_id); */

  /* get values, then store with suffix "_ITER_FROM" */
  *plugin_data_len = gap_filt_pdb_get_data(filter_procname);
  if(*plugin_data_len > 0)
  {
     g_snprintf(l_key_from, sizeof(l_key_from), "%s_ITER_FROM", filter_procname);
     gap_filt_pdb_set_data(l_key_from, *plugin_data_len);
  }
  else
  {
    return (-1);
  }

  if(*apply_mode != GAP_PAPP_VARYING_LINEAR)
  {
    return (p_pitstop_dialog(1, filter_procname));
  }

  return(0);
}	/* end p_do_filter_dialogs */


/* ============================================================================
 * p_do_2nd_filter_dialogs
 *    d) [ 2nd interactive filtercall
 *    e)   2nd pitstop dialog ]
 *
 *   (temporary) open the last frame of the range
 *   get its 1.st selected laye
 *   and do the Interctive Filtercall (to get the end-values)
 *
 * then close everything (without save).
 * (the last frame will be processed later, with all its selected layers)
 * ============================================================================
 */
static gint
p_do_2nd_filter_dialogs(char *filter_procname,
                        GapFiltPdbApplyMode  l_apply_mode,
			char *last_frame_filename,
			gint32 sel_mode, gint32 sel_case,
			gint32 sel_invert, char *sel_pattern
                       )
{
  gint32   l_layer_id;
  gint32   l_dpy_id;
  int      l_rc;
  int      l_idx;
  static char l_key_to[512];
  static char l_key_from[512];
  gint32  l_last_image_id;
  GapModLayliElem *l_layli_ptr;
  gint       l_nlayers;
  gint32     l_sel_cnt;
  gint       l_plugin_data_len;

  l_layli_ptr = NULL;
  l_rc = -1;       /* assume cancel or error */
  l_last_image_id = -1;
  l_dpy_id = -1;

  /* 2.nd INTERACTIV Filtercall dialog */
  /* --------------------------------- */
  if(last_frame_filename == NULL)
  {
    return (-1);  /* there is no 2.nd frame for 2.nd filter call */
  }

  if(p_pitstop_dialog(0, filter_procname) < 0)
     goto cleanup;

  /* load last frame into temporary image */
  l_last_image_id = gap_lib_load_image(last_frame_filename);
  if (l_last_image_id < 0)
     goto cleanup;

  /* get informations (id, visible, selected) about all layers */
  l_layli_ptr = gap_mod_alloc_layli(l_last_image_id, &l_sel_cnt, &l_nlayers,
                               sel_mode, sel_case, sel_invert, sel_pattern);

  if (l_layli_ptr == NULL)
     goto cleanup;

  /* get 1.st selected layer (of last handled frame in range ) */
  l_idx = gap_mod_get_1st_selected(l_layli_ptr, l_nlayers);
  if(l_idx < 0)
  {
     gap_arr_msg_win (GIMP_RUN_INTERACTIVE, _("GAP Modify: No layer selected in last handled frame"));
     goto cleanup;
  }
  l_layer_id = l_layli_ptr[l_idx].layer_id;

  /* open a view for the last handled frame */
  l_dpy_id = gimp_display_new (l_last_image_id);

  /* 2.nd INTERACTIV Filtercall dialog */
  /* --------------------------------- */
  l_rc = gap_filt_pdb_call_plugin(filter_procname, l_last_image_id, l_layer_id, GIMP_RUN_INTERACTIVE);

  /* get values, then store with suffix "_ITER_TO" */
  l_plugin_data_len = gap_filt_pdb_get_data(filter_procname);
  if(l_plugin_data_len <= 0)
     goto cleanup;

   g_snprintf(l_key_to, sizeof(l_key_to), "%s_ITER_TO", filter_procname);
   gap_filt_pdb_set_data(l_key_to, l_plugin_data_len);

   /* get FROM values */
   g_snprintf(l_key_from, sizeof(l_key_from), "%s_ITER_FROM", filter_procname);
   l_plugin_data_len = gap_filt_pdb_get_data(l_key_from);
   gap_filt_pdb_set_data(filter_procname, l_plugin_data_len);

  l_rc = p_pitstop_dialog(1, filter_procname);

cleanup:
  if(l_dpy_id >= 0)        gimp_display_delete(l_dpy_id);
  if(l_last_image_id >= 0) gimp_image_delete(l_last_image_id);
  if(l_layli_ptr != NULL)  g_free(l_layli_ptr);

  return (l_rc);


}	/* end p_do_2nd_filter_dialogs */


/* ============================================================================
 * p_frames_modify
 *
 *   foreach frame of the range (given by range_from and range_to)
 *   perform function defined by action_mode
 *   on all selected layer(s) described by sel_mode, sel_case
 *                                         sel_invert and sel_pattern
 * returns   0 if all done OK
 *           (or -1 on error or cancel)
 * ============================================================================
 */
static gint32
p_frames_modify(GapAnimInfo *ainfo_ptr,
                   long range_from, long range_to,
                   gint32 action_mode, gint32 sel_mode,
                   gint32 sel_case, gint32 sel_invert,
                   char *sel_pattern, char *new_layername)
{
  long    l_cur_frame_nr;
  long    l_step, l_begin, l_end;
  gint32  l_tmp_image_id;
  gint32  l_dpy_id;
  gint       l_nlayers;
  gdouble    l_percentage, l_percentage_step;
  int        l_rc;
  int        l_idx;
  gint32     l_sel_cnt;
  GapModLayliElem *l_layli_ptr;

  GimpParam     *l_params;
  gint        l_retvals;
  gint        l_plugin_data_len;
  char       l_filter_procname[256];
  char      *l_plugin_iterator;
  gdouble    l_cur_step;
  gint       l_total_steps;
  GapFiltPdbApplyMode  l_apply_mode;
  char         *l_last_frame_filename;
  gint          l_count;




  if(gap_debug) fprintf(stderr, "gap: p_frames_modify START, action_mode=%d  sel_mode=%d case=%d, invert=%d patt:%s:\n",
        (int)action_mode, (int)sel_mode, (int)sel_case, (int)sel_invert, sel_pattern);

  l_percentage = 0.0;
  if(ainfo_ptr->run_mode == GIMP_RUN_INTERACTIVE)
  {
    gimp_progress_init( _("Modifying frames/layer(s)..."));
  }


  l_begin = range_from;
  l_end   = range_to;
  l_tmp_image_id = -1;
  l_layli_ptr = NULL;
  l_rc = 0;
  l_plugin_iterator = NULL;
  l_plugin_data_len = 0;
  l_apply_mode = GAP_PAPP_CONSTANT;
  l_dpy_id = -1;
  l_last_frame_filename = NULL;

  /* init step direction */
  if(range_from > range_to)
  {
    l_step  = -1;     /* operate in descending (reverse) order */
    l_percentage_step = 1.0 / ((1.0 + range_from) - range_to);

    if(range_to < ainfo_ptr->first_frame_nr)
    { l_begin = ainfo_ptr->first_frame_nr;
    }
    if(range_from > ainfo_ptr->last_frame_nr)
    { l_end = ainfo_ptr->last_frame_nr;
    }

    l_total_steps = l_begin - l_end;
  }
  else
  {
    l_step  = 1;      /* operate in ascending order */
    l_percentage_step = 1.0 / ((1.0 + range_to) - range_from);

    if(range_from < ainfo_ptr->first_frame_nr)
    { l_begin = ainfo_ptr->first_frame_nr;
    }
    if(range_to > ainfo_ptr->last_frame_nr)
    { l_end = ainfo_ptr->last_frame_nr;
    }

    l_total_steps = l_end - l_begin;
  }

  l_cur_step = l_total_steps;

  l_cur_frame_nr = l_begin;
  while(1)              /* loop foreach frame in range */
  {
    if(gap_debug) fprintf(stderr, "p_frames_modify While l_cur_frame_nr = %d\n", (int)l_cur_frame_nr);

    /* build the frame name */
    if(ainfo_ptr->new_filename != NULL) g_free(ainfo_ptr->new_filename);
    ainfo_ptr->new_filename = gap_lib_alloc_fname(ainfo_ptr->basename,
                                        l_cur_frame_nr,
                                        ainfo_ptr->extension);
    if(ainfo_ptr->new_filename == NULL)
       goto error;

    /* load current frame into temporary image */
    l_tmp_image_id = gap_lib_load_image(ainfo_ptr->new_filename);
    if(l_tmp_image_id < 0)
       goto error;

    /* get informations (id, visible, selected) about all layers */
    l_layli_ptr = gap_mod_alloc_layli(l_tmp_image_id, &l_sel_cnt, &l_nlayers,
                                sel_mode, sel_case, sel_invert, sel_pattern);

    if(l_layli_ptr == NULL)
    {
       printf("gap: p_frames_modify: cant alloc layer info list\n");
       goto error;
    }

    if((l_cur_frame_nr == l_begin) && (action_mode == GAP_MOD_ACM_APPLY_FILTER))
    {
      /* ------------- 1.st frame: extra dialogs for APPLY_FILTER ---------- */

      if(l_sel_cnt < 1)
      {
         gap_arr_msg_win(GIMP_RUN_INTERACTIVE, _("No selected layer in start frame"));
         goto error;
      }

      if(l_begin != l_end)
      {
        l_last_frame_filename = gap_lib_alloc_fname(ainfo_ptr->basename,
                                        l_end,
                                        ainfo_ptr->extension);
      }

      /* additional dialog steps  a) gap_pdb_browser (select the filter)
       *                          b) 1st interactive filtercall
       *                          c) 1st pitstop dialog
       *                          d) [ 2nd interactive filtercall
       *                          e)   2nd pitstop dialog ]
       */

      l_rc = p_do_filter_dialogs(ainfo_ptr,
                                 l_tmp_image_id, &l_dpy_id,
                                 l_layli_ptr, l_nlayers,
                                &l_filter_procname[0], sizeof(l_filter_procname),
                                &l_plugin_data_len,
				&l_apply_mode
				 );

      if(l_last_frame_filename != NULL)
      {
        if((l_rc == 0) && (l_apply_mode == GAP_PAPP_VARYING_LINEAR))
	{
          l_rc = p_do_2nd_filter_dialogs(&l_filter_procname[0],
				   l_apply_mode,
				   l_last_frame_filename,
				   sel_mode, sel_case, sel_invert, sel_pattern
				  );
        }

        g_free(l_last_frame_filename);
        l_last_frame_filename = NULL;
      }

      /* the 1st selected layer has been filtered
       * in the INTERACTIVE call b)
       * therefore we unselect this layer, to avoid
       * a 2nd processing
       */
      l_idx = gap_mod_get_1st_selected(l_layli_ptr, l_nlayers);
      if(l_idx >= 0)
      {
        l_layli_ptr[l_idx].selected = FALSE;
        l_sel_cnt--;
      }

      /* check for matching Iterator PluginProcedures */
      if(l_apply_mode == GAP_PAPP_VARYING_LINEAR )
      {
        l_plugin_iterator =  gap_filt_pdb_get_iterator_proc(&l_filter_procname[0], &l_count);
      }
    }

    if(l_rc != 0)
      goto error;

    /* perform function (defined by action_mode) on selcted layer(s) */
    l_rc = p_apply_action(l_tmp_image_id,
		   action_mode,
		   l_layli_ptr,
		   l_nlayers,
		   l_sel_cnt,
		   l_begin, l_end, l_cur_frame_nr,
		   new_layername,
		   &l_filter_procname[0],
		   ainfo_ptr->image_id     /* MASTER_image_id */
		   );
    if(l_rc != 0)
    {
      if(gap_debug) fprintf(stderr, "gap: p_frames_modify p_apply-action failed. rc=%d\n", (int)l_rc);
      goto error;
    }

    /* free layli info table for the current frame */
    if(l_layli_ptr != NULL)
    {
      g_free(l_layli_ptr);
      l_layli_ptr = NULL;
    }

    /* check if the resulting image has at least one layer */
    gap_image_prevent_empty_image(l_tmp_image_id);

    /* save current frame with same name */
    l_rc = gap_lib_save_named_frame(l_tmp_image_id, ainfo_ptr->new_filename);
    if(l_rc < 0)
    {
      printf("gap: p_frames_modify save frame %d failed.\n", (int)l_cur_frame_nr);
      goto error;
    }
    else l_rc = 0;

    /* iterator call (for filter apply with varying values) */
    if((action_mode == GAP_MOD_ACM_APPLY_FILTER)
    && (l_plugin_iterator != NULL) && (l_apply_mode == GAP_PAPP_VARYING_LINEAR ))
    {
       l_cur_step -= 1.0;
        /* call plugin-specific iterator, to modify
         * the plugin's last_values
         */
       if(gap_debug) fprintf(stderr, "DEBUG: calling iterator %s  current frame:%d\n",
        		       l_plugin_iterator, (int)l_cur_frame_nr);
       if(strcmp(l_plugin_iterator, GIMP_PLUGIN_GAP_COMMON_ITER) == 0)
       {
         l_params = gimp_run_procedure (l_plugin_iterator,
		           &l_retvals,
		           GIMP_PDB_INT32,   GIMP_RUN_NONINTERACTIVE,
		           GIMP_PDB_INT32,   l_total_steps,      /* total steps  */
		           GIMP_PDB_FLOAT,   (gdouble)l_cur_step,    /* current step */
		           GIMP_PDB_INT32,   l_plugin_data_len, /* length of stored data struct */
                           GIMP_PDB_STRING,  &l_filter_procname[0],       /* the common iterator needs the plugin name as additional param */
			   GIMP_PDB_END);
       }
       else
       {
         l_params = gimp_run_procedure (l_plugin_iterator,
	  		     &l_retvals,
	  		     GIMP_PDB_INT32,   GIMP_RUN_NONINTERACTIVE,
	  		     GIMP_PDB_INT32,   l_total_steps,          /* total steps  */
	  		     GIMP_PDB_FLOAT,   (gdouble)l_cur_step,    /* current step */
	  		     GIMP_PDB_INT32,   l_plugin_data_len, /* length of stored data struct */
	  		     GIMP_PDB_END);
       }
       if (l_params[0].data.d_status != GIMP_PDB_SUCCESS)
       {
         fprintf(stderr, "ERROR: iterator %s  failed\n", l_plugin_iterator);
         l_rc = -1;
       }

       g_free(l_params);
    }


    /* close display (if open) */
    if (l_dpy_id >= 0)
    {
      gimp_display_delete(l_dpy_id);
      l_dpy_id = -1;
    }

    /* destroy the tmp image */
    gimp_image_delete(l_tmp_image_id);

    if(l_rc != 0)
      goto error;



    if(ainfo_ptr->run_mode == GIMP_RUN_INTERACTIVE)
    {
      l_percentage += l_percentage_step;
      gimp_progress_update (l_percentage);
    }

    /* advance to next frame */
    if(l_cur_frame_nr == l_end)
       break;
    l_cur_frame_nr += l_step;

  }		/* end while(1)  loop foreach frame in range */

  if(gap_debug) fprintf(stderr, "p_frames_modify End OK\n");

  return 0;

error:
  if(gap_debug) fprintf(stderr, "gap: p_frames_modify exit with Error\n");

  if (l_dpy_id >= 0)
  {
      gimp_display_delete(l_dpy_id);
      l_dpy_id = -1;
  }
  if(l_tmp_image_id >= 0) gimp_image_delete(l_tmp_image_id);
  if(l_layli_ptr != NULL) g_free(l_layli_ptr);
  if(l_plugin_iterator != NULL)  g_free(l_plugin_iterator);
  return -1;

}		/* end p_frames_modify */


/* ============================================================================
 * gap_mod_layer
 * ============================================================================
 */

gint gap_mod_layer(GimpRunMode run_mode, gint32 image_id,
                   gint32 range_from,  gint32 range_to,
                   gint32 action_mode, gint32 sel_mode,
                   gint32 sel_case, gint32 sel_invert,
                   char *sel_pattern, char *new_layername)
{
  int    l_rc;
  GapAnimInfo *ainfo_ptr;
  gint32    l_from;
  gint32    l_to;
  gint32    l_action_mode;
  gint32    l_sel_mode;
  gint32    l_sel_case;
  gint32    l_sel_invert;

  char      l_sel_pattern[MAX_LAYERNAME];
  char      l_new_layername[MAX_LAYERNAME];

  l_rc = 0;


  ainfo_ptr = gap_lib_alloc_ainfo(image_id, run_mode);
  if(ainfo_ptr != NULL)
  {

    if (0 == gap_lib_dir_ainfo(ainfo_ptr))
    {
      if(run_mode == GIMP_RUN_INTERACTIVE)
      {
         l_rc = p_layer_modify_dialog (ainfo_ptr, &l_from, &l_to,
	                               &l_action_mode,
				       &l_sel_mode, &sel_case, &sel_invert,
				       &l_sel_pattern[0], &l_new_layername[0]);

      }
      else
      {
         l_from        = range_from;
         l_to          = range_to;
	 l_action_mode = action_mode;
	 l_sel_mode    = sel_mode;
	 l_sel_case    = sel_case;
	 l_sel_invert  = sel_invert;

         strncpy(&l_sel_pattern[0], sel_pattern, sizeof(l_sel_pattern) -1);
	 l_sel_pattern[sizeof(l_sel_pattern) -1] = '\0';
         strncpy(&l_new_layername[0], new_layername, sizeof(l_new_layername) -1);
	 l_new_layername[sizeof(l_new_layername) -1] = '\0';
      }

      if(l_rc >= 0)
      {
         if(gap_lib_gap_check_save_needed(ainfo_ptr->image_id))
         {
           l_rc = gap_lib_save_named_frame(ainfo_ptr->image_id, ainfo_ptr->old_filename);
         }
         if(l_rc >= 0)
         {
           l_rc = p_frames_modify(ainfo_ptr, l_from, l_to,
	                          l_action_mode,
				  l_sel_mode, sel_case, sel_invert,
				  &l_sel_pattern[0], &l_new_layername[0]
				 );
           gap_lib_load_named_frame(ainfo_ptr->image_id, ainfo_ptr->old_filename);
         }
      }


    }
    gap_lib_free_ainfo(&ainfo_ptr);
  }


  return(l_rc);
}
