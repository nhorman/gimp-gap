/* gap_gve_misc_util.c
 *
 *  GAP video encoder tool procedures
 *   - filehandling
 *   - GAP filename (framename) handling
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
 * version 1.2.1a;  2004.05.14   hof: created
 */

/* SYTEM (UNIX) includes */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>


/* GIMP includes */
#include "gtk/gtk.h"
/* #include "libgimp/stdplugins-intl.h" */
#include "libgimp/gimp.h"


#include "gap_gve_misc_util.h"

/*************************************************************
 *          TOOL FUNCTIONS                                   *
 *************************************************************/


/* --------------------------
 * gap_gve_misc_get_ainfo
 * --------------------------
 * get animinfo using
 * a PDB call to the corresponding GAP Base Functionality.
 * - check if the directory, where the the given image's filename resides as file
 *   has more frames with equal basename and extension.
 * fill the ainfo structure with information
 * about the first and last framenumber and number of frames (frame_cnt)
 */
void
gap_gve_misc_get_ainfo(gint32 image_ID, GapGveEncAInfo *ainfo)
{
   static char     *l_called_proc = "plug_in_gap_get_animinfo";
   GimpParam       *return_vals;
   int              nreturn_vals;
   gint32           dummy_layer_id;

   dummy_layer_id = gap_image_get_any_layer(image_ID);

   return_vals = gimp_run_procedure (l_called_proc,
                                 &nreturn_vals,
				 GIMP_PDB_INT32, GIMP_RUN_NONINTERACTIVE,
                                 GIMP_PDB_IMAGE, image_ID,
                                 GIMP_PDB_DRAWABLE, dummy_layer_id,
                                 GIMP_PDB_END);

   if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
   {
      ainfo->first_frame_nr = return_vals[1].data.d_int32;
      ainfo->last_frame_nr =  return_vals[2].data.d_int32;
      ainfo->curr_frame_nr =  return_vals[3].data.d_int32;
      ainfo->frame_cnt =      return_vals[4].data.d_int32;
      g_snprintf(ainfo->basename, sizeof(ainfo->basename), "%s", return_vals[5].data.d_string);
      g_snprintf(ainfo->extension, sizeof(ainfo->extension), "%s", return_vals[6].data.d_string);
      ainfo->framerate = return_vals[7].data.d_float;

      gimp_destroy_params(return_vals, nreturn_vals);

      return;
   }

   gimp_destroy_params(return_vals, nreturn_vals);
   printf("Error: PDB call of %s failed, image_ID: %d\n", l_called_proc, (int)image_ID);

}  /* end gap_gve_misc_get_ainfo */



/* ---------------------------------
 * p_snprintf_master_encoder_progress_keyname
 * ---------------------------------
 *
 */
static void
p_snprintf_master_encoder_progress_keyname(char *key, gint32 key_max_size, gint32 master_encoder_id)
{
  g_snprintf(key, key_max_size, "GAP_MASTER_ENCODER_PROGRESS_%d", (int)master_encoder_id);
}

static void
p_snprintf_master_encoder_cancel_keyname(char *key, gint32 key_max_size, gint32 master_encoder_id)
{
  g_snprintf(key, key_max_size, "GAP_MASTER_ENCODER_CANCEL_%d", (int)master_encoder_id);
}


/* ------------------------------------------
 * gap_gve_misc_initGapGveMasterEncoderStatus
 * ---------................-----------------
 * This pocedure is typically called at start of video encoding
 */
void
gap_gve_misc_initGapGveMasterEncoderStatus(GapGveMasterEncoderStatus *encStatus
   , gint32 master_encoder_id, gint32 total_frames)
{
  encStatus->master_encoder_id = master_encoder_id;
  encStatus->total_frames = total_frames;
  encStatus->frames_processed = 0;
  encStatus->frames_encoded = 0;
  encStatus->frames_copied_lossless = 0;
//  encStatus->pidOfRunningEncoder = 0;

  gap_gve_misc_do_master_encoder_progress(encStatus);
}

/* ----------------------------------------
 * gap_gve_misc_do_master_encoder_progress
 * ----------------------------------------
 * This pocedure is typically called in video encoder plug-ins
 * to report the current encoding status.
 * (including the process id of the encoder plug-in)
 */
void
gap_gve_misc_do_master_encoder_progress(GapGveMasterEncoderStatus *encStatus)
{
  char key[50];
  
  p_snprintf_master_encoder_progress_keyname(&key[0], sizeof(key), encStatus->master_encoder_id);
  gimp_set_data(key, encStatus, sizeof(GapGveMasterEncoderStatus));
}


/* ---------------------------------------------
 * gap_gve_misc_is_master_encoder_cancel_request
 * ---------------------------------------------
 * This pocedure is typically called in video encoder plug-ins
 * to query for cancel request (Cancel button pressed in the master encoder)
 */
gboolean
gap_gve_misc_is_master_encoder_cancel_request(GapGveMasterEncoderStatus *encStatus)
{
  char key[50];
  gboolean cancelRequest;

  cancelRequest = FALSE;
  p_snprintf_master_encoder_cancel_keyname(&key[0], sizeof(key), encStatus->master_encoder_id);
  if(gimp_get_data_size(key) == sizeof(gboolean))
  {
      gimp_get_data(key, &cancelRequest);
  }
  return (cancelRequest);
}


/* -----------------------------------------
 * gap_gve_misc_get_master_encoder_progress
 * -----------------------------------------
 * This pocedure is typically called in the master encoder dialog
 * to get the status of the running video encoder plug-in.
 */
void
gap_gve_misc_get_master_encoder_progress(GapGveMasterEncoderStatus *encStatus)
{
  char key[50];
  
  p_snprintf_master_encoder_progress_keyname(&key[0], sizeof(key), encStatus->master_encoder_id);

  if(gimp_get_data_size(key) == sizeof(GapGveMasterEncoderStatus))
  {
      if(gap_debug)
      {
        printf("p_gimp_get_data: key:%s\n", key);
      }
      gimp_get_data(key, encStatus);
  }
  else
  {
     if(gap_debug)
     {
       printf("ERROR: gimp_get_data key:%s failed\n", key);
       printf("ERROR: gimp_get_data_size:%d  expected size:%d\n"
             , (int)gimp_get_data_size(key)
             , (int)sizeof(GapGveMasterEncoderStatus));
     }
  }
}


/* ----------------------------------------------
 * gap_gve_misc_set_master_encoder_cancel_request
 * ----------------------------------------------
 * This pocedure is typically called in the master video encoder
 * to request the already started video encoder plug-in to terminate.
 */
void
gap_gve_misc_set_master_encoder_cancel_request(GapGveMasterEncoderStatus *encStatus, gboolean cancelRequest)
{
  char key[50];

  p_snprintf_master_encoder_cancel_keyname(&key[0], sizeof(key), encStatus->master_encoder_id);
  gimp_set_data(key, &cancelRequest, sizeof(gboolean));
}

