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
 * p_gap_build_enc_status_filename
 * ---------------------------------
 * build the filename that is used for communication
 * the encoder status between the GIMP-GAP master videoencoder and the
 * video encoder plug-in (that typically runs in a separate process)
 */
static char *
p_gap_build_enc_status_filename(gint32 master_encoder_id)
{
   char *filename;
   char *buf;
   
   buf = g_strdup_printf("gap_master_videoencoder_progress_%d", master_encoder_id);
   
   filename = g_build_filename(gimp_directory(), buf, NULL);

   g_free(buf);
   return (filename);
}  /* end p_gap_build_enc_status_filename */


/* ---------------------------------------
 * p_gap_build_enc_cancel_request_filename
 * ---------------------------------------
 * build the filename that is used to indicate cancel a running video encoder process.
 * (if this file exists cancel request is TRUE)
 */
static char *
p_gap_build_enc_cancel_request_filename(gint32 master_encoder_id)
{
   char *filename;
   char *buf;
   
   buf = g_strdup_printf("gap_master_videoencoder_cancel_%d", master_encoder_id);
   
   filename = g_build_filename(gimp_directory(), buf, NULL);

   g_free(buf);
   return (filename);
}  /* end p_gap_build_enc_cancel_request_filename */



/* ---------------------------------------
 * gap_gve_misc_cleanup_GapGveMasterEncoder
 * ---------------------------------------
 * cleanup (remove communication files)
 * typically called by the master videoencoder when encoding has finished.
 */
void
gap_gve_misc_cleanup_GapGveMasterEncoder(gint32 master_encoder_id)
{
   char *filename;
   
   filename = p_gap_build_enc_cancel_request_filename(master_encoder_id);

   if(g_file_test(filename, G_FILE_TEST_EXISTS))
   {
     g_remove(filename);
   }
   g_free(filename);

   filename = p_gap_build_enc_status_filename(master_encoder_id);
   if(g_file_test(filename, G_FILE_TEST_EXISTS))
   {
     g_remove(filename);
   }
   g_free(filename);

}  /* end gap_gve_misc_cleanup_GapGveMasterEncoder */


/* ------------------------------------------
 * gap_gve_misc_initGapGveMasterEncoderStatus
 * ------------------------------------------
 * This pocedure is typically called at start of video encoding
 */
void
gap_gve_misc_initGapGveMasterEncoderStatus(GapGveMasterEncoderStatus *encStatus
   , gint32 master_encoder_id, gint32 total_frames)
{
  gap_gve_misc_cleanup_GapGveMasterEncoder(master_encoder_id);

  encStatus->master_encoder_id = master_encoder_id;
  encStatus->total_frames = total_frames;
  encStatus->frames_processed = 0;
  encStatus->frames_encoded = 0;
  encStatus->frames_copied_lossless = 0;
  encStatus->current_pass = 0;
//  encStatus->pidOfRunningEncoder = 0;

  gap_gve_misc_do_master_encoder_progress(encStatus);
}


/* ------------------------------------------
 * p_write_encoder_status
 * ------------------------------------------
 * write current encoder status to binary file
 * (the file is used for communication between the encoder process
 * and the master videoencoder GUI process)
 */
static void
p_write_encoder_status(GapGveMasterEncoderStatus *encStatus)
{
  FILE *fp;
  char *filename;
   
  filename = p_gap_build_enc_status_filename(encStatus->master_encoder_id);

  if(gap_debug)
  {
     printf("p_write_encoder_status:  frames_processed:%d, PID:%d  filename:%s\n"
        , (int) encStatus->frames_processed
        , (int) getpid()
        , filename
        );
  }


  fp = fopen(filename, "wb");
  if(fp)
  {
    fwrite(encStatus, sizeof(GapGveMasterEncoderStatus), 1, fp);
    fclose(fp);
  }
  g_free(filename);
}

/* ------------------------------------------
 * p_read_encoder_status
 * ------------------------------------------
 * read current encoder status from binary file
 * (the file is used for communication between the encoder process
 * and the master videoencoder GUI process)
 */
static void
p_read_encoder_status(GapGveMasterEncoderStatus *encStatus)
{
  FILE *fp;
  char *filename;
  gint32 master_encoder_id;
  
  master_encoder_id = encStatus->master_encoder_id;
  filename = p_gap_build_enc_status_filename(master_encoder_id);

  fp = fopen(filename, "rb");
  if(fp)
  {
    GapGveMasterEncoderStatus encBuffer;
    
    fread(&encBuffer, sizeof(GapGveMasterEncoderStatus), 1, fp);
    fclose(fp);
    
    if(master_encoder_id == encBuffer.master_encoder_id)
    {
      memcpy(encStatus, &encBuffer,  sizeof(GapGveMasterEncoderStatus));
    }
    else
    {
       printf("p_read_encoder_status:  ERROR unexpected contetent in communication  filename:%s\n   PID:%d  id expected: %d but found: %d\n"
          , filename
          , (int) getpid()
          , (int) master_encoder_id
          , (int) encBuffer.master_encoder_id
          );
    }

    if(gap_debug)
    {
       printf("p_read_encoder_status:  frames_processed:%d, PID:%d  filename:%s\n"
          , (int) encStatus->frames_processed
          , (int) getpid()
          , filename
          );
    }
  }
  else
  {
       printf("p_read_encoder_status:  ERROR cold not open communication  filename:%s PID:%d\n"
          , filename
          , (int) getpid()
          );
  }
  
  g_free(filename);
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
  p_write_encoder_status(encStatus);
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
  gboolean cancelRequest;
  char *filename;
   
  cancelRequest = FALSE;
  filename = p_gap_build_enc_cancel_request_filename(encStatus->master_encoder_id);

  if(g_file_test(filename, G_FILE_TEST_EXISTS))
  {
    cancelRequest = TRUE;
  }

  g_free(filename);
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
  p_read_encoder_status(encStatus);
  return;
}


/* ----------------------------------------------
 * gap_gve_misc_set_master_encoder_cancel_request
 * ----------------------------------------------
 * This pocedure is typically called in the master video encoder
 * to request the already started video encoder plug-in to terminate.
 * (the request is indicated by creating a file with a special name,
 * its content is just comment and not relevant)
 */
void
gap_gve_misc_set_master_encoder_cancel_request(GapGveMasterEncoderStatus *encStatus, gboolean cancelRequest)
{
  FILE *fp;
  char *filename;
   
  filename = p_gap_build_enc_cancel_request_filename(encStatus->master_encoder_id);

  fp = fopen(filename, "w");
  if(fp)
  {
    fprintf(fp, "GAP videoencoder CANCEL requested\n");
    fclose(fp);
  }
  g_free(filename);
}

