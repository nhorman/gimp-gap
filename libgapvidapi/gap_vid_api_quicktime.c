/* gap_vid_api_quicktime.c
 *
 * GAP Video read API implementation of quicktime
 * based wrappers to read various videofile formats
 *
 * 2003.05.09   hof created
 *
 */

/* ================================================ QUICKTIME
 * quicktime                                        QUICKTIME
 * ================================================ QUICKTIME
 * ================================================ QUICKTIME
 */
#ifdef ENABLE_GVA_LIBQUICKTIME
#include "quicktime.h"

gboolean
p_wrapper_quicktime_check_sig(char *filename)
{
  /* quicktime_check_sig returns 1 if the file is a quicktime file */
  return (1 == quicktime_check_sig(filename));
}

/* -----------------------------
 * p_wrapper_quicktime_open_read
 * -----------------------------
 */
void
p_wrapper_quicktime_open_read(char *filename, t_GVA_Handle *gvahand)
{
    quicktime_t*  handle;
    gint      repeat_count;

    if(gap_debug) printf("p_wrapper_quicktime_open_read: START filename: %s\n", filename);

  /* workaround:
   *  sometimes open succeeds, but quicktime4linux does not recognize any video or audio channel
   *  dont know whats wrong here.
   *  as workaround this API tries repeart the open for 3 times (only if no tracks can be found)
   */
  for(repeat_count = 0; repeat_count < 3; repeat_count++)
  {
    handle = quicktime_open(filename
                           , TRUE      /* int rd */
                           , FALSE     /* int wr */
                           );

    gvahand->decoder_handle = (void *)handle;
    if(gvahand->decoder_handle)
    {
      gvahand->frame_bpp = 3;

      if(quicktime_has_video(handle))
      {
        gvahand->vtracks          = quicktime_video_tracks(handle);
        gvahand->vid_track        = CLAMP(gvahand->vid_track, 0, gvahand->vtracks -1);

        gvahand->total_frames     = quicktime_video_length(handle, (int)gvahand->vid_track);
        gvahand->framerate        = quicktime_frame_rate(handle, (int)gvahand->vid_track);
        gvahand->width            = quicktime_video_width(handle, (int)gvahand->vid_track);
        gvahand->height           = quicktime_video_height(handle, (int)gvahand->vid_track);
      }
      else
      {
        gvahand->total_frames     = 0;
        gvahand->framerate        = 1.0;
        gvahand->width            = 0;
        gvahand->height           = 0;
      }
      if(quicktime_has_audio(handle))
      {
        gvahand->atracks            = quicktime_audio_tracks(handle);
        gvahand->aud_track          = CLAMP(gvahand->aud_track, 0, gvahand->atracks -1);

        gvahand->samplerate         = quicktime_sample_rate(handle, (int)gvahand->aud_track);
        gvahand->audio_cannels      = quicktime_track_channels(handle, (int)gvahand->aud_track);
        gvahand->total_aud_samples  = quicktime_audio_length(handle, (int)gvahand->aud_track);
        gvahand->audio_playtime_sec = (gdouble)gvahand->total_aud_samples / (gdouble)gvahand->samplerate;
      }

      if((gvahand->atracks > 0) || (gvahand->vtracks > 0))
      {
        break;                    /* OK video and /or audio found */
      }
      else
      {
        if(gap_debug) printf("quicktime Open problems attempt:%d\n", (int)repeat_count);
        quicktime_close(handle);  /* workaround: close and try open another time */
        quicktime_check_sig(filename);
        gvahand->decoder_handle = NULL;
      }
    }

  }    /* end for repeat_count */
}  /* end p_wrapper_quicktime_open_read */


/* -----------------------------
 * p_wrapper_quicktime_close
 * -----------------------------
 */
void
p_wrapper_quicktime_close(t_GVA_Handle *gvahand)
{
  quicktime_close((quicktime_t *)gvahand->decoder_handle);
}


/* ----------------------------------
 * p_wrapper_quicktime_get_next_frame
 * ----------------------------------
 *  TODO:
 *        - how to call CODEC on the buffers
 *               - Quicktime ? are internal CODECS applied automatc ?
 */
t_GVA_RetCode
p_wrapper_quicktime_get_next_frame(t_GVA_Handle *gvahand)
{
  quicktime_t *handle;
  int      l_rc;

  handle = (quicktime_t *)gvahand->decoder_handle;
  /* Decode or the frame into a frame buffer. */
  /* All the frame buffers passed to these functions are unsigned char */
  /* rows with 3 bytes per pixel.  The byte order per 3 byte pixel is */
  /* RGB. */
  l_rc = quicktime_decode_video(handle
                               ,gvahand->row_pointers
                               ,(int)gvahand->vid_track
                               );
  if(l_rc == 0)
  {
    gvahand->current_frame_nr = gvahand->current_seek_nr;
    gvahand->current_seek_nr++;
    return(GVA_RET_OK);
  }

  if(l_rc == 1)  { return(GVA_RET_EOF); }

  return(GVA_RET_ERROR);
}  /* end p_wrapper_quicktime_get_next_frame */


/* ------------------------------
 * p_wrapper_quicktime_seek_frame
 * ------------------------------
 */
t_GVA_RetCode
p_wrapper_quicktime_seek_frame(t_GVA_Handle *gvahand, gdouble pos, t_GVA_PosUnit pos_unit)
{
  quicktime_t *handle;
  int      l_rc;
  longest  l_frame_pos;

  handle = (quicktime_t *)gvahand->decoder_handle;

  switch(pos_unit)
  {
    case GVA_UPOS_FRAMES:
      l_frame_pos = (longest)pos;
      break;
    case GVA_UPOS_SECS:
      l_frame_pos = (longest)round(pos * gvahand->framerate);
      break;
    case GVA_UPOS_PRECENTAGE:
      l_frame_pos = (longest)GVA_percent_2_frame(gvahand->total_frames, pos);
      break;
    default:
      l_frame_pos = (longest)pos;
      break;
  }

  l_rc = quicktime_set_video_position(handle
                                     ,l_frame_pos
                                     ,(int)gvahand->vid_track
                                     );

  if(l_rc == 0)
  {
    gvahand->current_seek_nr = (gint32)l_frame_pos;
    return(GVA_RET_OK);
  }
  if(l_rc == 1)  { return(GVA_RET_EOF); }

  return(GVA_RET_ERROR);
}  /* end p_wrapper_quicktime_seek_frame */

/* ------------------------------
 * p_wrapper_quicktime_seek_audio
 * ------------------------------
 *  TODO: - Retocde evaluation (what is OK, EOF or ERROR ?)
 */
t_GVA_RetCode
p_wrapper_quicktime_seek_audio(t_GVA_Handle *gvahand, gdouble pos, t_GVA_PosUnit pos_unit)
{
  quicktime_t *handle;
  int      l_rc;
  longest  l_sample_pos;
  gint32   l_frame_pos;

  handle = (quicktime_t *)gvahand->decoder_handle;

  switch(pos_unit)
  {
    case GVA_UPOS_FRAMES:
      l_sample_pos = (longest)GVA_frame_2_samples(gvahand->framerate, gvahand->samplerate, pos);
      break;
    case GVA_UPOS_SECS:
      l_sample_pos = (longest)round(pos / MAX(gvahand->samplerate, 1.0));
      break;
    case GVA_UPOS_PRECENTAGE:
      l_frame_pos= (longest)GVA_percent_2_frame(gvahand->total_frames, pos);
      l_sample_pos = (longest)GVA_frame_2_samples(gvahand->framerate, gvahand->samplerate, l_frame_pos);
      break;
    default:
      l_sample_pos = 0;
      break;
  }

  l_rc = quicktime_set_audio_position(handle
                                     ,l_sample_pos
                                     ,(int)gvahand->aud_track
                                     );

  if(l_rc == 0)
  {
    gvahand->current_sample = (gint32)l_sample_pos;
    return(GVA_RET_OK);
  }
  if(l_rc == 1)  { return(GVA_RET_EOF); }

  return(GVA_RET_ERROR);
}  /* end p_wrapper_quicktime_seek_audio */


/* ------------------------------
 * p_wrapper_quicktime_get_audio
 * ------------------------------
 */
t_GVA_RetCode
p_wrapper_quicktime_get_audio(t_GVA_Handle *gvahand
             ,gint16 *output_i
             ,gint32 channel
             ,gdouble samples
             ,t_GVA_AudPos mode_flag
             )
{
  quicktime_t *handle;
  int      l_rc;

  handle = (quicktime_t *)gvahand->decoder_handle;

  if(mode_flag == GVA_AMOD_FRAME)
  {
      p_wrapper_quicktime_seek_audio(gvahand
                                    , (gdouble) gvahand->current_frame_nr
                                    , GVA_UPOS_FRAMES
                                    );
  }

  if(mode_flag == GVA_AMOD_REREAD)
  {
    printf("p_wrapper_quicktime_get_audio: GVA_AMOD_REREAD not implemented\n");
  }

  /* TODO:
   *   howto specify desired AUDIO TRACK ??  gvahand->aud_track
   *   (are channel numbers unique or local to audio track ???)
   */
    l_rc = quicktime_decode_audio(handle
                                 ,(int16_t *)output_i
                                 ,NULL                /*  float *output_f */
                                 ,(long)samples
                                 ,(int)channel
                                 );

  if(l_rc == 0)
  {
    gvahand->current_sample += samples;  /* keep track of current sample position */
    return(GVA_RET_OK);
  }
  if(l_rc == 1)  { return(GVA_RET_EOF); }

  return(GVA_RET_ERROR);
}  /* end p_wrapper_quicktime_get_audio */


/* ----------------------------------
 * p_wrapper_quicktime_count_frames
 * ----------------------------------
 * (re)open a separated handle for counting
 * to ensure that stream positions are not affected by the count.
 */
t_GVA_RetCode
p_wrapper_quicktime_count_frames(t_GVA_Handle *gvahand)
{
  quicktime_t*  handle;
  int      l_rc;
  t_GVA_Handle copy_gvahand;
  gdouble  l_progress_step;

  gvahand->percentage_done = 0.0;

  /* WORKAROUND:
   *   a) quicktime does not really need count because total_frames seems to be exact value
   *   b) this implementatation does not work, because quicktime_decode_video always
   *      returnes wit retcode == 0 and gives us no chance to stop at EOF
   * this are 2 reasons to do not cont at all.
   */
return(GVA_RET_OK);



  copy_gvahand.frame_data = NULL;
  copy_gvahand.row_pointers = NULL;
  copy_gvahand.width = gvahand->width;
  copy_gvahand.height = gvahand->height;
  copy_gvahand.frame_bpp = gvahand->frame_bpp;

  /* allocate frame_data and row_pointers for one frame (minimal cachesize 1 == no chaching)
   */
  p_build_frame_cache(&copy_gvahand, 1);



  handle = quicktime_open(gvahand->filename
                          , TRUE      /* int rd */
                          , FALSE     /* int wr */
                          );

  /* percentage_done is just a guess, because we dont know
   * the exact total_frames number before.
   * (we assume that there may be 11 frames more)
   */
  l_progress_step =  1.0 / (gdouble)(gvahand->total_frames + 11.0);
  gvahand->frame_counter = 0;
  while(!gvahand->cancel_operation)
  {
     /* read one frame (we use the comressed chunk
      * because this is a dummy read to count frames only)
      * quicktime_read_frame DID not work (always returned 905)
      */
      /* l_rc = quicktime_read_frame(handle
       *                        , dummy_buffer
       *                        , gvahand->vid_track
       *                         );
       */
     l_rc = quicktime_decode_video(handle
                               ,copy_gvahand.row_pointers
                               ,(int)gvahand->vid_track
                               );

     if (gap_debug) printf("quicktime_read_frame: l_rc == %d\n", (int)l_rc);

     if(l_rc != 0)
     {
        break;  /* eof, or fetch error */
     }
     gvahand->frame_counter++;
     gvahand->percentage_done = CLAMP(gvahand->percentage_done + l_progress_step, 0.0, 1.0);
     if(gvahand->fptr_progress_callback)
     {
       gvahand->cancel_operation = (*gvahand->fptr_progress_callback)(gvahand->percentage_done, gvahand->progress_cb_user_data);
     }
  }

  /* close the extra handle (that was opened for counting only) */
  quicktime_close(handle);

  p_drop_frame_cache(&copy_gvahand);

  gvahand->percentage_done = 0.0;

  if(!gvahand->cancel_operation)
  {
     gvahand->total_frames = gvahand->frame_counter;
     gvahand->all_frames_counted = TRUE;
  }
  else
  {
     gvahand->total_frames = MAX(gvahand->total_frames, gvahand->frame_counter);
     gvahand->cancel_operation = FALSE;  /* reset cancel flag */
     return(GVA_RET_ERROR);
  }

  return(GVA_RET_OK);

}  /* end p_wrapper_quicktime_count_frames */


/* -----------------------------
 * p_quicktime_new_dec_elem
 * -----------------------------
 * create a new decoder element and init with
 * functionpointers referencing the QUICKTIME
 * specific Procedures
 */
t_GVA_DecoderElem  *
p_quicktime_new_dec_elem(void)
{
  t_GVA_DecoderElem  *dec_elem;

  dec_elem = g_malloc0(sizeof(t_GVA_DecoderElem));
  if(dec_elem)
  {
    dec_elem->decoder_name         = g_strdup("quicktime4linux");
    dec_elem->decoder_description  = g_strdup("Quicktime Decoder (CODECS: JPEG,PNG,MJPG) (EXT: .mov)");
    dec_elem->fptr_check_sig       = &p_wrapper_quicktime_check_sig;
    dec_elem->fptr_open_read       = &p_wrapper_quicktime_open_read;
    dec_elem->fptr_close           = &p_wrapper_quicktime_close;
    dec_elem->fptr_get_next_frame  = &p_wrapper_quicktime_get_next_frame;
    dec_elem->fptr_seek_frame      = &p_wrapper_quicktime_seek_frame;
    dec_elem->fptr_seek_audio      = &p_wrapper_quicktime_seek_audio;
    dec_elem->fptr_get_audio       = &p_wrapper_quicktime_get_audio;
    dec_elem->fptr_count_frames    = &p_wrapper_quicktime_count_frames;
    dec_elem->fptr_get_video_chunk = NULL; /* &p_wrapper_quicktime_get_video_chunk; */
    dec_elem->next = NULL;
  }

  return (dec_elem);
}  /* end p_quicktime_new_dec_elem */


#endif  /* ENABLE_GVA_LIBQUICKTIME */

