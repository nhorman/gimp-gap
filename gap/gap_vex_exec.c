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




/* ---------------------
 * p_extract_audio
 * ---------------------
 * - extract audio and optional save to wav file
 */
void
p_extract_audio(GapVexMainGlobalParams *gpp,  t_GVA_Handle   *gvahand, gdouble samples_to_read,  gboolean wav_save)
{
#ifdef GAP_ENABLE_VIDEOAPI_SUPPORT
   int l_audio_channels;
   int l_sample_rate;
   long l_audio_samples;
   unsigned short *left_ptr;
   unsigned short *right_ptr;
   unsigned short *l_lptr;
   unsigned short *l_rptr;
   long   l_to_read;
   gint64 l_left_to_read;
   long   l_block_read;
   gdouble l_progress;
   FILE  *fp_wav;

   l_audio_channels = gvahand->audio_cannels;
   l_sample_rate    = gvahand->samplerate;
   l_audio_samples  = gvahand->total_aud_samples;

   if(gap_debug)
   {
     printf("Channels:%d samplerate:%d samples:%d  samples_to_read: %.0f\n"
          , (int)l_audio_channels
          , (int)l_sample_rate
          , (int)l_audio_samples
          , (float)samples_to_read
          );
   }
   
  fp_wav = NULL;
  if(wav_save) 
  {
    fp_wav = g_fopen(gpp->val.audiofile, "wb");
  }
  
  if((fp_wav) || (!wav_save))
  {
     gint32 l_bytes_per_sample;
     gint32 l_ii;

     if(l_audio_channels == 1) { l_bytes_per_sample = 2;}  /* mono */
     else                      { l_bytes_per_sample = 4;}  /* stereo */

     if(wav_save) 
     {
       /* write the header */
       gap_audio_wav_write_header(fp_wav
                      , (gint32)samples_to_read
                      , l_audio_channels            /* cannels 1 or 2 */
                      , l_sample_rate
                      , l_bytes_per_sample
                      , 16                          /* 16 bit sample resolution */
                      );
    }
    if(gap_debug) printf("samples_to_read:%d\n", (int)samples_to_read);

    /* audio block read (blocksize covers playbacktime for 250 frames */
    l_left_to_read = samples_to_read;
    l_block_read = (double)(250.0) / (double)gvahand->framerate * (double)l_sample_rate;
    l_to_read = MIN(l_left_to_read, l_block_read);

    /* allocate audio buffers */
    left_ptr = g_malloc0((sizeof(short) * l_block_read) + 16);
    right_ptr = g_malloc0((sizeof(short) * l_block_read) + 16);

    while(l_to_read > 0)
    {
       l_lptr = left_ptr;
       l_rptr = right_ptr;
       /* read the audio data of channel 0 (left or mono) */
       GVA_get_audio(gvahand
                 ,l_lptr                /* Pointer to pre-allocated buffer if int16's */
                 ,1                     /* Channel to decode */
                 ,(gdouble)l_to_read    /* Number of samples to decode */
                 ,GVA_AMOD_CUR_AUDIO    /* read from current audio position (and advance) */
                 );
      if((l_audio_channels > 1) && (wav_save))
      {
         /* read the audio data of channel 2 (right)
          * NOTE: GVA_get_audio has advanced the stream position,
          *       so we have to set GVA_AMOD_REREAD to read from
          *       the same startposition as for channel 1 (left).
          */
         GVA_get_audio(gvahand
                   ,l_rptr                /* Pointer to pre-allocated buffer if int16's */
                   ,2                     /* Channel to decode */
                   ,l_to_read             /* Number of samples to decode */
                   ,GVA_AMOD_REREAD       /* read from */
                 );
      }
      l_left_to_read -= l_to_read;

      if(wav_save) 
      {
        /* write 16 bit wave datasamples 
         * sequence mono:    (lo, hi)
         * sequence stereo:  (lo_left, hi_left, lo_right, hi_right)
         */
        for(l_ii=0; l_ii < l_to_read; l_ii++)
        {
           gap_audio_wav_write_gint16(fp_wav, *l_lptr);
           l_lptr++;
           if(l_audio_channels > 1)
           {
             gap_audio_wav_write_gint16(fp_wav, *l_rptr);
             l_rptr++;
           }
         }
      }


      l_to_read = MIN(l_left_to_read, l_block_read);

      /* calculate progress */
      l_progress = (gdouble)(samples_to_read - l_left_to_read) / ((gdouble)samples_to_read + 1.0);
      if(gap_debug) printf("l_progress:%f\n", (float)l_progress);
      if (gpp->val.run_mode != GIMP_RUN_NONINTERACTIVE)
      {
        gimp_progress_update (l_progress);
      }
    }
    if(wav_save) 
    {
      /* close wavfile */
      fclose(fp_wav);
    }

    /* free audio buffers */
    g_free(left_ptr);
    g_free(right_ptr);
  }
#endif
  return;
}  /* end p_extract_audio */



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
        gdouble l_samples_to_read;
        gdouble l_extracted_frames;

        if(gap_debug) printf("EXTRACTING audio, writing to file %s\n", gpp->val.audiofile);

        if (gpp->val.exact_seek != 0)
        {
          /* close and reopen to make sure that we start at begin */

          gvahand->image_id = -1;   /* prenvent API from deleting that image at close */
          GVA_close(gvahand);

          /* --------- (RE)OPEN the videofile --------------- */
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
        }

        if(gvahand->audio_cannels > 0)
        {
          /* seek needed only if extract starts not at pos 1 */
          if(l_pos > 1)
          {
            if (gpp->val.run_mode != GIMP_RUN_NONINTERACTIVE)
            {
               gimp_progress_init (_("Seek Audio Position..."));
            }

            /* check for exact frame_seek */
            if (gpp->val.exact_seek != 0)
            {
               gint32 l_seek_framenumber;
               
               
               l_seek_framenumber = l_pos;
               if(l_pos_unit == GVA_UPOS_PRECENTAGE)
               {
                 l_seek_framenumber = gvahand->total_frames * l_pos;
               }
               
               l_samples_to_read = (gdouble)(l_seek_framenumber) 
                                / (gdouble)gvahand->framerate * (gdouble)gvahand->samplerate;

               /* extract just for exact positioning (without save to wav file) */
               if(gap_debug) printf("extract just for exact positioning (without save to wav file)\n");
               p_extract_audio(gpp, gvahand, l_samples_to_read,  FALSE);
            }
            else
            {
              /* audio pos 1 frame before video pos
                * example: extract frame 1 upto 2
                * results in audio range 0 upto 2
                * this way we can get the audioduration of frame 1 
                */
               GVA_seek_audio(gvahand, l_pos -1, l_pos_unit);
            }
          }

          if (gpp->val.run_mode != GIMP_RUN_NONINTERACTIVE)
          {
             gimp_progress_init (_("Extracting Audio..."));
          }

          l_extracted_frames = framenumber - framenumber1;
          if(l_extracted_frames > 1)
          {
            l_samples_to_read = (gdouble)(l_extracted_frames +1.0)
	                      / (gdouble)gvahand->framerate 
			      * (gdouble)gvahand->samplerate;
            if(gap_debug) printf("A: l_samples_to_read %.0f l_extracted_frames:%d\n"
	                         , (float)l_samples_to_read
				 ,(int)l_extracted_frames
				 );
          }
          else
          {
            l_samples_to_read = (gdouble)(l_expected_frames +1.0) 
	                      / (gdouble)gvahand->framerate 
			      * (gdouble)gvahand->samplerate;
            if(gap_debug) printf("B: l_samples_to_read %.0f l_extracted_frames:%d l_expected_frames:%d\n"
	                        , (float)l_samples_to_read
				,(int)l_extracted_frames
				,(int)l_expected_frames
				);
          }

          /* extract and save to wav file */
          if(gap_debug) printf("extract (with save to wav file)\n");
          p_extract_audio(gpp, gvahand, l_samples_to_read,  TRUE);
        }
        
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
