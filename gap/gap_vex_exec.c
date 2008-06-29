/*
 * gap_vex_exec.c
 * Video Extract GUI and worker procedures
 *   based on gap_vid_api (GVA)
 */

/*
 * Changelog:
 * 2003/04/19 v1.2.1a:  created
 */

/*
 * Copyright
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


#include <glib/gstdio.h>

#include "gap_vex_exec.h"
#include "gap_vex_dialog.h"
#include "gap_audio_wav.h"


/* -------------------
 * p_gap_set_framerate
 * -------------------
 */
static void
p_gap_set_framerate(gint32 image_id, gdouble framerate)
{
  GimpParam     *l_params;
  gint           l_retvals;

  if (gap_debug)  printf("DEBUG: before p_gap_set_framerate %f\n", (float)framerate);
  l_params = gimp_run_procedure ("plug_in_gap_set_framerate",
                          &l_retvals,
                          GIMP_PDB_INT32,    GIMP_RUN_NONINTERACTIVE,
                          GIMP_PDB_IMAGE,    image_id,
                          GIMP_PDB_DRAWABLE, 0,
                          GIMP_PDB_FLOAT,    framerate,
                          GIMP_PDB_END);

  g_free(l_params);                               
}  /* end p_gap_set_framerate */



/* ------------------------------
 * gap_vex_exe_extract_videorange
 * ------------------------------
 * - the productive procedure for extracting frames and/or audio
 *   from a videofile.
 */
void
gap_vex_exe_extract_videorange(GapVexMainGlobalParams *gpp)
{
#ifdef GAP_ENABLE_VIDEOAPI_SUPPORT
  t_GVA_Handle   *gvahand;
  t_GVA_PosUnit  l_pos_unit;
  t_GVA_RetCode  l_rc;

  GimpRunMode l_save_run_mode;
  gchar *framename;
  gint32 framenumber;
  gint32 framenumber1;
  gint32 framenumber1_delta;
  gdouble l_pos;
  gdouble l_pos_end;
  gdouble l_progress;
  gdouble l_expected_frames;
  gint    l_overwrite_mode;
  gint    l_overwrite_mode_audio;

  l_overwrite_mode_audio = 0;

  if(gap_debug)
  {
      printf("RUN p_mpg_fetch_range with parameters:\n");
      printf("videoname    : %s\n", gpp->val.videoname);
      printf("begin_percent: %f\n", (float)gpp->val.begin_percent);
      printf("end_percent  : %f\n", (float)gpp->val.end_percent);
      printf("begin_frame  : %d\n", (int)gpp->val.begin_frame);
      printf("end_frame    : %d\n", (int)gpp->val.end_frame);
      printf("pos_unit     : %d\n", (int)gpp->val.pos_unit);
      printf("audiofile    : %s\n", gpp->val.audiofile);
      printf("basename     : %s\n", gpp->val.basename);
      printf("extension    : %s\n", gpp->val.extension);
      printf("basenum      : %d\n", (int)gpp->val.basenum);
      printf("fn_digits    : %d\n", (int)gpp->val.fn_digits);
      printf("multilayer   : %d\n", (int)gpp->val.multilayer);
      printf("disable_mmx  : %d\n", (int)gpp->val.disable_mmx);
      printf("videotrack   : %d\n", (int)gpp->val.videotrack);
      printf("audiotrack   : %d\n", (int)gpp->val.audiotrack);
      printf("ow_mode      : %d\n", (int)gpp->val.ow_mode);
      printf("preferred_decoder : %s\n", gpp->val.preferred_decoder);
      printf("exact_seek   : %d\n", (int)gpp->val.exact_seek);
      printf("deinterlace  : %d\n", (int)gpp->val.deinterlace);
      printf("delace_threshold: %f\n", (float)gpp->val.delace_threshold);
  }

  l_save_run_mode = GIMP_RUN_INTERACTIVE;  /* for the 1.st call of saving a non xcf frame */
  l_overwrite_mode = 0;

  gpp->val.image_ID = -1;
  
  /* --------- OPEN the videofile --------------- */
  gvahand = GVA_open_read_pref(gpp->val.videoname
                           ,gpp->val.videotrack
                           ,gpp->val.audiotrack
                           ,gpp->val.preferred_decoder
                           , FALSE  /* use MMX if available (disable_mmx == FALSE) */
                           );
  if(gvahand == NULL)
  {
     return;
  }

  /* ------ check if we have to extract audio (ask for audio overwrite) ---------- */
  if((gvahand->atracks > 0)
  && (gpp->val.audiotrack > 0))
  {
     l_overwrite_mode_audio = 0;
     l_overwrite_mode_audio = gap_vex_dlg_overwrite_dialog(gpp
                                 , gpp->val.audiofile
				 , l_overwrite_mode_audio
				 );
  }


  framenumber = gpp->val.basenum;

  if(gpp->val.pos_unit == 0)
  {
    l_pos_unit = GVA_UPOS_FRAMES;
    l_pos_end  = gpp->val.end_frame;
    l_pos      = gpp->val.begin_frame;
    l_expected_frames = 1 + (l_pos_end - l_pos);
    framenumber1_delta = framenumber - l_pos;
    if(gpp->val.basenum <= 0)
    {
      framenumber = l_pos;
      framenumber1_delta = 0;
    }
  }
  else
  {
    l_pos_unit = GVA_UPOS_PRECENTAGE;
    l_pos_end  = (gpp->val.end_percent / 100.0);
    l_pos      = (gpp->val.begin_percent / 100.0);
    l_expected_frames = gvahand->total_frames * (l_pos_end - l_pos);
    framenumber1_delta = framenumber - (gvahand->total_frames * l_pos);
    if(gpp->val.basenum <= 0)
    {
      framenumber = gvahand->total_frames * l_pos;
      framenumber1_delta = 0;
    }
  }
  framenumber1 = framenumber;


  /* ------ extract Video ---------- */
  if((gvahand->vtracks > 0)
  && (gpp->val.videotrack > 0))
  {
     gint32  delace[2];
     gint32  framenumber_fil;
     gint    iid;
     gint    iid_max;

     iid_max = 1;
     delace[0] = gpp->val.deinterlace;
     if((gpp->val.deinterlace == GAP_VEX_DELACE_ODD_X2)
     || (gpp->val.deinterlace == GAP_VEX_DELACE_EVEN_X2))
     {
       iid_max = 2;
       framenumber_fil = (framenumber * 2) -1;

       if(gpp->val.deinterlace == GAP_VEX_DELACE_ODD_X2) 
       {
         delace[0] = GAP_VEX_DELACE_ODD;
         delace[1] = GAP_VEX_DELACE_EVEN;
       }
       else
       {
         delace[0] = GAP_VEX_DELACE_EVEN;
         delace[1] = GAP_VEX_DELACE_ODD;
       }
     }


    /* check if we need an INTERACTIVE Dummy save to set default parameters
     * for further frame save operation.
     * The dummy save is done at begin of processing
     * because the handling of the 1st frame may
     * occure after a significant delay, caused by seeking in large videofiles.
     */
    if((strcmp(gpp->val.extension, ".xcf") == 0)
    || (strcmp(gpp->val.extension, ".XCF") == 0))
    {
      /* Native GIMP format needs no save params
       * and can be called NON_INTERACTIVE for all frames
       */
      l_save_run_mode = GIMP_RUN_NONINTERACTIVE;
    }
    else
    {
      gchar *l_dummyname;
      gint32 l_dummy_image_id;
      gint32 l_empty_layer_id;

      l_dummy_image_id = gimp_image_new(32, 32, GIMP_RGB);
      l_empty_layer_id = gimp_layer_new(l_dummy_image_id, "background",
                          32, 32,
                          GIMP_RGB_IMAGE,
                          100.0,     /* Opacity full opaque */     
                          GIMP_NORMAL_MODE);
      gimp_image_add_layer(l_dummy_image_id, l_empty_layer_id, 0);
      gap_layer_clear_to_color(l_empty_layer_id, 0.0, 0.0, 0.0, 1.0);

      l_save_run_mode = GIMP_RUN_INTERACTIVE;  /* for the 1.st call of saving a non xcf frame */

      /* must use same basename and extension for the dummyname
       * because setup of jpeg save params for further non interactive save operation
       * depend on a key that includes the same basename and extension. 
       */
      l_dummyname = gap_lib_alloc_fname6(&gpp->val.basename[0]
                                        ,99999999
                                        ,&gpp->val.extension[0]
                                        ,8  /* use full 8 digits for the numberpart */
                                        );
      gimp_image_set_filename(l_dummy_image_id, l_dummyname);
      gap_lib_save_named_image(l_dummy_image_id
                           , l_dummyname
                           , l_save_run_mode
                           );

      gap_image_delete_immediate(l_dummy_image_id);
      g_remove(l_dummyname);                       
      g_free(l_dummyname);       
      l_save_run_mode = GIMP_RUN_WITH_LAST_VALS;     /* for all further calls */
    }


    if (gpp->val.run_mode != GIMP_RUN_NONINTERACTIVE)
    {
      if(gpp->val.videotrack > 0)
      {
        gimp_progress_init (_("Seek Frame Position..."));
        gvahand->do_gimp_progress = TRUE;
      }
    }

    if(gpp->val.exact_seek != 0) /* exact frame_seek */
    {
      gint32 l_seekstep;
      gint32 l_seek_framenumber;
 

      if(1==1)
      {
        gvahand->emulate_seek = TRUE;  /* force seq. reads even if we have a video index */
        l_rc = GVA_seek_frame(gvahand, l_pos, l_pos_unit);
      }
      else
      {
        /* dead code (older and slower seek emulation
	 * implementation outside the API) 
         */
        l_seek_framenumber = l_pos;
        if(l_pos_unit == GVA_UPOS_PRECENTAGE)
        {
          l_seek_framenumber = gvahand->total_frames * l_pos;
        }

        for(l_seekstep = 1; l_seekstep < l_seek_framenumber; l_seekstep++)
        {
          /* fetch one frame to buffer gvahand->frame_data
           * (and proceed position to next frame) 
           */
          l_rc = GVA_get_next_frame(gvahand);
          if(l_rc != GVA_RET_OK)
          {
            break;
          }
          l_progress = (gdouble)l_seekstep / l_seek_framenumber;
          if (gpp->val.run_mode != GIMP_RUN_NONINTERACTIVE)
          {
            gimp_progress_update (l_progress);
          }
        }
      }
    }
    else
    {
      l_rc = GVA_seek_frame(gvahand, l_pos, l_pos_unit);
    }

    if (gpp->val.run_mode != GIMP_RUN_NONINTERACTIVE)
    {
       gimp_progress_init (_("Extracting Frames..."));
    }
    while(1)
    {
       /* fetch one frame to buffer gvahand->frame_data
        * (and proceed position to next frame) 
        */
       l_rc = GVA_get_next_frame(gvahand);
       if(l_rc != GVA_RET_OK)
       {
         break;
       }
       if(gpp->val.multilayer == 0)
       {
         if((gpp->val.deinterlace == GAP_VEX_DELACE_ODD_X2) 
	 || (gpp->val.deinterlace == GAP_VEX_DELACE_EVEN_X2))
         {
           framenumber_fil = (framenumber * 2) -1;
         }
         else
         {
           framenumber_fil = framenumber;
         }

         /* loop once (or twice for splitting deinterlace modes) */
         for(iid=0; iid < iid_max; iid++)
         {
           /* convert fetched frame from buffer to gimp image gvahand->image_id
            */
           l_rc = GVA_frame_to_gimp_layer(gvahand
                                         ,TRUE   /* delete_mode */
                                         ,framenumber - framenumber1_delta
                                         ,delace[iid]
                                         ,gpp->val.delace_threshold
                                         );
           if(l_rc != GVA_RET_OK)
           {
             break;
           }
           framename = gap_lib_alloc_fname6(&gpp->val.basename[0]
                                    ,(long)(framenumber_fil + iid)
                                    ,&gpp->val.extension[0]
                                    ,gpp->val.fn_digits
                                    );
           gimp_image_set_filename(gvahand->image_id, framename);
           gpp->val.image_ID = gvahand->image_id;

           l_overwrite_mode = gap_vex_dlg_overwrite_dialog(gpp
	                             , framename
				     , l_overwrite_mode
				     );
           if (l_overwrite_mode < 0)
           {
               g_free(framename);       
               break;
           }
           else
           {
              gint32 l_sav_rc;
              
              l_sav_rc = gap_lib_save_named_image(gvahand->image_id
                           , framename
                           , l_save_run_mode
                           );
              if (l_sav_rc < 0)
              {
                g_message(_("failed to save file:\n'%s'"), framename);
                break;
              }
           }
           g_free(framename);       
         }
       }
       else
       {
         /* creeate one multilayer image */
         l_rc = GVA_frame_to_gimp_layer(gvahand
                                       ,FALSE   /* delete_mode FALSE: keep layers */
                                       ,framenumber - framenumber1_delta
                                       ,delace[0]
                                       ,gpp->val.delace_threshold
                                       );
         if(l_rc != GVA_RET_OK)
         {
           break;
         }
         gpp->val.image_ID = gvahand->image_id;
         
         if((gpp->val.deinterlace == GAP_VEX_DELACE_ODD_X2)
	 || (gpp->val.deinterlace == GAP_VEX_DELACE_EVEN_X2))
         {
           /* deinterlace the other set of rows as extra layer (even/odd) */
           l_rc = GVA_frame_to_gimp_layer(gvahand
                                       ,FALSE   /* delete_mode FALSE: keep layers */
                                       ,framenumber - framenumber1_delta
                                       ,delace[1]
                                       ,gpp->val.delace_threshold
                                       );
           if(l_rc != GVA_RET_OK)
           {
             break;
           }
         }
         
       }

       framenumber++;

       l_progress = 0.95 * ((gdouble)(framenumber - framenumber1) / l_expected_frames);

       if(gap_debug) printf("\nPROGRESS expected N[%d] frames:%f  PROGRESS: %f\n\n"
         , (int)(framenumber - framenumber1)
         , (float)l_expected_frames
         , (float)l_progress);

       if (gpp->val.run_mode != GIMP_RUN_NONINTERACTIVE)
       {
         gimp_progress_update (l_progress);
       }
       if((framenumber - framenumber1) >= l_expected_frames)
       {
          break;
       }
    }
  }


  /* ------ extract Audio ---------- */
  if((gvahand->atracks > 0)
  && (gpp->val.audiotrack > 0))
  {
     if (l_overwrite_mode_audio >= 0)
     {
       gdouble l_extracted_frames;
       gboolean do_progress;
       
       l_extracted_frames = framenumber - framenumber1;
       
       do_progress = TRUE;
       if(gpp->val.run_mode != GIMP_RUN_NONINTERACTIVE)
       {
         do_progress = FALSE;
       }

       gap_audio_extract_from_videofile(gpp->val.videoname
             , gpp->val.audiotrack
             , gpp->val.preferred_decoder
             , gpp->val.exact_seek
             , l_pos_unit
             , l_pos
             , l_extracted_frames
             , l_expected_frames
             , do_progress
             , NULL         /* GtkWidget *progressBar  using NULL for gimp_progress */
             , NULL         /* fptr_progress_callback */
             , NULL         /* user_data */
             );
     }
  }


  if((gpp->val.image_ID >= 0)
  && (gpp->val.multilayer == 0)
  && (gpp->val.videotrack > 0))
  {
    p_gap_set_framerate(gpp->val.image_ID, gvahand->framerate);
  }


  l_progress = 1.0;
  if (gpp->val.run_mode != GIMP_RUN_NONINTERACTIVE)
  {
    gimp_progress_update (l_progress);
  }

  
  if(gpp->val.image_ID >= 0)
  {
    gimp_image_undo_enable(gpp->val.image_ID);
    gimp_display_new(gpp->val.image_ID);
    gimp_displays_flush();
    gvahand->image_id = -1;   /* prenvent API from deleting that image at close */
  }

  GVA_close(gvahand);

/* endif GAP_ENABLE_VIDEOAPI_SUPPORT */
#endif
  return;
}  /* end gap_vex_exe_extract_videorange */
