/* gap_vid_api_ffmpeg.c
 *
 * GAP Video read API implementation of libavformat/lbavcodec (also known as FFMPEG)
 * based wrappers to read various videofile formats
 *
 * 2004.04.12   vindex bugfix seek high framnumbers sometimes used wrong (last index)
 * 2004.03.07   vindex
 * 2004.02.29   hof fptr_progress_callback
 * 2003.05.09   hof created
 *
 */


/* ================================================ FFMPEG
 * FFMPEG (libavformat libavcodec)                  FFMPEG
 * ================================================ FFMPEG
 * ================================================ FFMPEG
 */
#ifdef ENABLE_GVA_LIBAVFORMAT
#include "avformat.h"


/* -------------------------
 * API READ extension FFMPEG
 * -------------------------
 */


/* samples buffer for ca. 500 Audioframes at 48000 Hz Samplerate and 30 fps
 * if someone tries to get a bigger audio segment than this at once
 * the call will fail !!
 */

#define GVA_SAMPLES_BUFFER_SIZE  2400000

#define GVA_FFMPEG_DECODER_NAME "libavformat"

/* structure with FFMPEG specific file and codec handle pointers */
typedef struct t_GVA_ffmpeg
{
 AVFormatContext *vid_input_context;
 AVFormatContext *aud_input_context;

 AVCodecContext  *vid_codec_context;
 AVCodecContext  *aud_codec_context;
 AVCodec         *vcodec;
 AVCodec         *acodec;

 AVStream       *vid_stream;  /* pointer to to vid_input_context->streams[vid_track] */
 AVStream       *aud_stream;  /* pointer to to aud_input_context->streams[aud_track] */

 AVPacket        vid_pkt;         /* the current packet (read from the video stream) */
 AVPacket        aud_pkt;         /* the current packet (read from the audio stream) */

 AVFrame         big_picture_rgb;
 AVFrame         big_picture_yuv;
 AVPicture      *picture_rgb;
 AVPicture      *picture_yuv;
 enum PixelFormat yuv_buff_pix_fmt;
 /*UNIT8*/  guchar        *yuv_buffer;
 /*UNIT8*/  guchar        *inbuf_ptr;
 int             inbuf_len;
 /*UNIT8*/  guchar        *abuf_ptr;
 int             abuf_len;

  guchar        *output_samples_ptr;    /* current write position (points somewhere into samples_buffer) */

  gint           bf_idx;               /* 0 or 1 (index of the active samples_buffer) */

  gint32        samples_buffer_size;
  guchar        *samples_buffer[2];    /* Buffer for decoded samples from samples_base_a until read position */

  gint32        samples_base[2];       /* start offset of the samples_buffers (unit is samples) */

  gint32        bytes_filled[2];       /* how many bytes are filled into samples_buffer(s) */

  gint32        biggest_data_size;     /* the biggest uncompressed audio packet found (min 4096) */

  gdouble        samples_read;         /* how many (decoded) samples already read into samples_buffer
                                        * since (RE)open the audio context
                                        */

 gboolean        dummy_read;      /* FALSE: read YUV + convert to RGB,  TRUE: dont convert RGB */
 gboolean        capture_offset;  /* TRUE: capture url_offsets to vindex while reading next frame  */
 gint32          max_frame_len;
 gint32          frame_len;
 gint32          got_frame_length;
 gint64          prev_url_offset;
 gint64          prev_prev_url_offset;
 gint64          prev_key_url_offset;
 gint32          prev_key_seek_nr;
 gint32          prev_pkt_count;
 gdouble         guess_gop_size;

 int             vid_stream_index;
 int             aud_stream_index;
 int             vid_codec_id;
 int             aud_codec_id;
} t_GVA_ffmpeg;


static void      p_ffmpeg_vid_reopen_read(t_GVA_ffmpeg *handle, t_GVA_Handle *gvahand);
static void      p_ffmpeg_aud_reopen_read(t_GVA_ffmpeg *handle, t_GVA_Handle *gvahand);
static gboolean  p_ff_open_input(char *filename, t_GVA_Handle *gvahand, t_GVA_ffmpeg*  handle, gboolean vid_open);

static gint32    p_pick_channel( t_GVA_ffmpeg *handle
                               , t_GVA_Handle *gvahand
                               , gint16 *output_i
                               , gint32 sample_idx
                               , gint32 samples
                               , gint32 channel
                               );
static int       p_read_audio_packets(t_GVA_ffmpeg *handle
                               , t_GVA_Handle *gvahand
                               , gint32 max_sample_pos);

static void      p_vindex_add_url_offest(t_GVA_Handle *gvahand
                         , t_GVA_ffmpeg *handle
                         , t_GVA_Videoindex *vindex
			 , gint32 seek_nr
			 , gint64 url_offset
			 , guint16 frame_length
			 , guint16 checksum
			 );
static guint16  p_gva_checksum(AVPicture *picture_yuv, gint32 height);

/* -----------------------------
 * p_wrapper_ffmpeg_check_sig
 * -----------------------------
 * TODO: should also check supported codec (for vid_track and aud_track but need this info as calling parameter !!)
 */
gboolean
p_wrapper_ffmpeg_check_sig(char *filename)
{
  AVFormatContext *ic;
  int err, ret;

  if(gap_debug) printf("p_wrapper_ffmpeg_check_sig: START filename:%s\n", filename);

  /* ------------ File Format  -------   */
  /* open the input file with generic libav function
   * (do not force a specific format)
   */
  err = av_open_input_file(&ic, filename, NULL, 0, NULL);
  if (err < 0)
  {
     if(gap_debug) printf("p_wrapper_ffmpeg_check_sig: av_open_input_file FAILED: %d\n", (int)err);
     return(FALSE);
  }

  /* If not enough info to get the stream parameters, we decode the
   * first frames to get it. (used in mpeg case for example)
   */
  ret = av_find_stream_info(ic);
  if (ret < 0)
  {
     if(gap_debug) printf("p_wrapper_ffmpeg_check_sig:%s: could not find codec parameters\n", filename);
     return(FALSE);
  }

  av_close_input_file(ic);

  if(gap_debug) printf("p_wrapper_ffmpeg_check_sig: compatible is TRUE\n");

  return (TRUE);
}

/* -----------------------------
 * p_wrapper_ffmpeg_open_read
 * -----------------------------
 *  Open the fileformat
 *  Open the needed decoder codecs (video and audio)
 *  - set the CodecContext
 *  - do we need AVStream vid_stream, aud_stream and howto init and use that ?
 *  - init picture (rowpointers)
 * TODO:
 *  - howto findout total_frames (and total_samples)
 */
void
p_wrapper_ffmpeg_open_read(char *filename, t_GVA_Handle *gvahand)
{
  t_GVA_ffmpeg*  handle;
  AVInputFormat *iformat;

  if(gap_debug) printf("p_wrapper_ffmpeg_open_read: START filename:%s\n", filename);

  if(gvahand->filename == NULL)
  {
    gvahand->filename = g_strdup(filename);
  }

  gvahand->decoder_handle = (void *)NULL;
  gvahand->vtracks = 0;
  gvahand->atracks = 0;
  gvahand->frame_bpp = 3;              /* RGB pixelformat */

  handle = g_malloc0(sizeof(t_GVA_ffmpeg));
  handle->dummy_read = FALSE;
  handle->capture_offset = FALSE;
  handle->guess_gop_size = 0;
  handle->prev_url_offset = 0;
  handle->prev_prev_url_offset = 0;
  handle->prev_key_url_offset = 0;
  handle->prev_key_seek_nr = 1;
  handle->prev_pkt_count = 0;
  handle->max_frame_len = 0;
  handle->got_frame_length = 0;
  handle->vid_input_context = NULL;     /* set later (if has video) */
  handle->aud_input_context = NULL;     /* set later (if has audio) */
  handle->vid_codec_context = NULL;     /* set later (if has video) */
  handle->aud_codec_context = NULL;     /* set later (if has audio) */
  handle->vcodec = NULL;                /* set later (if has video and vid_track > 0) */
  handle->acodec = NULL;                /* set later (if has audio and aud_track > 0) */
  handle->vid_stream_index = -1;
  handle->aud_stream_index = -1;
  handle->vid_codec_id = 0;
  handle->aud_codec_id = 0;
  handle->vid_pkt.size = 0;             /* start with empty packet */
  handle->vid_pkt.data = NULL;          /* start with empty packet */
  handle->aud_pkt.size = 0;             /* start with empty packet */
  handle->aud_pkt.data = NULL;          /* start with empty packet */
  handle->samples_buffer[0] = NULL;        /* set later (at 1.st audio packet read) */
  handle->samples_buffer[1] = NULL;        /* set later (at 1.st audio packet read) */
  handle->samples_base[0] = 0;
  handle->samples_base[1] = 0;
  handle->bytes_filled[0] = 0;
  handle->bytes_filled[1] = 0;
  handle->biggest_data_size = 4096;
  handle->output_samples_ptr = NULL;    /* current write position (points somewhere into samples_buffer) */
  handle->samples_buffer_size = 0;
  handle->samples_read = 0;

  /* open for the VIDEO part */
  if(FALSE == p_ff_open_input(filename, gvahand, handle, TRUE))
  {
    g_free(handle);
    if(gap_debug) printf("p_wrapper_ffmpeg_open_read: open Videopart FAILED\n");
    return;
  }

  if(gvahand->aud_track >= 0)
  {
    /* open a seperate input context for the AUDIO part */
    if(FALSE == p_ff_open_input(filename, gvahand, handle, FALSE))
    {
      g_free(handle);
      if(gap_debug) printf("p_wrapper_ffmpeg_open_read: open Audiopart FAILED\n");
      return;
    }
  }


  if((handle->vid_stream_index == -1) && (handle->aud_stream_index == -1))
  {
     g_free(handle);
     if(gap_debug) printf("p_wrapper_ffmpeg_open_read: neither video nor audio codec found\n");
     return;    /* give up if both video_codec and audio_codec are not supported */
  }


  /* set detailed decoder description */
  iformat = handle->vid_input_context->iformat;
  if(iformat)
  {
    gchar *vcodec_name;
    gchar *acodec_name;
    t_GVA_DecoderElem  *dec_elem;

    dec_elem = (t_GVA_DecoderElem *)gvahand->dec_elem;

    if(dec_elem)
    {
      if(dec_elem->decoder_description)
      {
        g_free(dec_elem->decoder_description);
      }

      if(handle->vcodec)  { vcodec_name = g_strdup(handle->vcodec->name); }
      else                { vcodec_name = g_strdup("** no video codec **"); }

      if(handle->acodec)  { acodec_name = g_strdup(handle->acodec->name); }
      else                { acodec_name = g_strdup("** no audio codec **"); }

      dec_elem->decoder_description =
        g_strdup_printf("FFMPEG Decoder (Decoder for MPEG1,MPEG4(DivX),RealVideo)\n"
                        " (EXT: .mpg,.vob,.avi,.rm,.mpeg)\n"
                        "active FORMAT: %s %s\n"
                        "active CODEC(S): %s  %s"
                        , iformat->name
                        , iformat->long_name
                        , vcodec_name
                        , acodec_name
                       );

      g_free(vcodec_name);
      g_free(acodec_name);
    }
  }



  handle->inbuf_len = 0;            /* start with empty buffer */
  handle->frame_len = 0;            /* start with empty buffer */
  handle->inbuf_ptr = NULL;         /* start with empty buffer, after 1.st av_read_packet: pointer to pkt.data read pos */

  /* yuv_buffer big enough for all supported PixelFormats
   * (the vid_codec_context->pix_fmt may change later (?)
   *  but i want to use one all purpose yuv_buffer all the time
   *  while the video is open for performance reasons)
   */
  handle->yuv_buffer = g_malloc0(gvahand->width * gvahand->height * 4);


  /* total_frames and total_aud_samples are just a guess, based on framesize and filesize
   * TODO: ??? howto findout exact value ?
   */
  gvahand->total_frames = p_guess_total_frames(gvahand);
  gvahand->total_aud_samples = 0;
  if((gvahand->atracks > 0) && (handle->aud_stream_index >= 0))
  {
    gvahand->total_aud_samples = (gint32)GVA_frame_2_samples(gvahand->framerate, gvahand->samplerate, gvahand->total_frames);
  }

  /* Picture field are filled with 'ptr' addresses */

  handle->picture_rgb = (AVPicture *)&handle->big_picture_rgb;
  handle->picture_yuv = (AVPicture *)&handle->big_picture_yuv;

  /* allocate frame_data and row_pointers for one frame (minimal cachesize 1 == no chaching)
   * (calling apps may request bigger cache after successful open)
   */
  p_build_frame_cache(gvahand, 1);

  avpicture_fill(handle->picture_rgb
                ,gvahand->frame_data
                ,PIX_FMT_RGB24      /* PIX_FMT_RGB24, PIX_FMT_RGBA32, PIX_FMT_BGRA32 */
                ,gvahand->width
                ,gvahand->height
                );


  /* 
   * vid_codec_context->pix_fmt may change later, and is sometimes
   * not recognized until the 1.st frame has been decoded.
   * therefore the follwing avpicture_fill is just a 1.st guess
   * and must be repeated later to be sure to know the codec's native pix_fmt
   * But this does decode the the 1.st frame with the wrong PIX_FMT settings
   *  - most MPEG codecs (VCD, DVD..) use the default PIX_FMT_YUV420P
   *    and work fine without problems
   *  - Quicktime codec (codec_id == 10)  uses PIX_FMT_YUV422P ( Olympus C4040Z Digicam MJPEG .mov files)
   */
  handle->yuv_buff_pix_fmt = handle->vid_codec_context->pix_fmt;   /* PIX_FMT_YUV420P */
  
  avpicture_fill(handle->picture_yuv
                ,handle->yuv_buffer
                ,handle->yuv_buff_pix_fmt
                ,gvahand->width
                ,gvahand->height
                );

  if(gap_debug)
  {
     printf("gvahand->width: %d  gvahand->height: %d\n", (int)gvahand->width , (int)gvahand->height);
     printf("picture_rgb: data[0]: %d linesize[0]: %d\n", (int)handle->picture_rgb->data[0], (int)handle->picture_rgb->linesize[0]);
     printf("picture_rgb: data[1]: %d linesize[1]: %d\n", (int)handle->picture_rgb->data[1], (int)handle->picture_rgb->linesize[1]);
     printf("picture_rgb: data[2]: %d linesize[2]: %d\n", (int)handle->picture_rgb->data[2], (int)handle->picture_rgb->linesize[2]);
     printf("picture_rgb: data[3]: %d linesize[3]: %d\n", (int)handle->picture_rgb->data[3], (int)handle->picture_rgb->linesize[3]);

     printf("picture_yuv: data[0]: %d linesize[0]: %d\n", (int)handle->picture_yuv->data[0], (int)handle->picture_yuv->linesize[0]);
     printf("picture_yuv: data[1]: %d linesize[1]: %d\n", (int)handle->picture_yuv->data[1], (int)handle->picture_yuv->linesize[1]);
     printf("picture_yuv: data[2]: %d linesize[2]: %d\n", (int)handle->picture_yuv->data[2], (int)handle->picture_yuv->linesize[2]);
     printf("picture_yuv: data[3]: %d linesize[3]: %d\n", (int)handle->picture_yuv->data[3], (int)handle->picture_yuv->linesize[3]);
     printf("handle->yuv_buff_pix_fmt: %d\n", (int)handle->yuv_buff_pix_fmt);
  }

  gvahand->decoder_handle = (void *)handle;

  if(gvahand->vindex == NULL)
  {
    gvahand->vindex = GVA_load_videoindex(gvahand->filename, gvahand->vid_track, GVA_FFMPEG_DECODER_NAME);
    if(gvahand->vindex)
    {
      if(gvahand->vindex->total_frames > 1)
      {
        /* we have a complete vindex and can
	 * trust the total_frames in the vindex file
	 */
        gvahand->total_frames = gvahand->vindex->total_frames;
        gvahand->all_frames_counted = TRUE;
      }
    }
  }

  if(gap_debug) printf("p_wrapper_ffmpeg_open_read: END, OK\n");

}  /* end p_wrapper_ffmpeg_open_read */


/* -----------------------------
 * p_wrapper_ffmpeg_close
 * -----------------------------
 * TODO:
 *  - howto close codec, codec ontext ... and whatever ...
 */
void
p_wrapper_ffmpeg_close(t_GVA_Handle *gvahand)
{
  t_GVA_ffmpeg *handle;

  if(gap_debug) printf("p_wrapper_ffmpeg_close: START\n");

  handle = (t_GVA_ffmpeg *)gvahand->decoder_handle;

  if(handle->samples_buffer[0])
  {
    if(gap_debug) printf("p_wrapper_ffmpeg_close: FREE audio samples_buffer\n");
    g_free(handle->samples_buffer[0]);
    handle->samples_buffer[0] = NULL;
    g_free(handle->samples_buffer[1]);
    handle->samples_buffer[1] = NULL;
  }

  if(handle->yuv_buffer)
  {
    if(gap_debug) printf("p_wrapper_ffmpeg_close: FREE YUV_BUFFER\n");
    g_free(handle->yuv_buffer);
    handle->yuv_buffer = NULL;
  }

  if((handle->vid_codec_context) && (handle->vcodec))
  {
    if(gap_debug) printf("p_wrapper_ffmpeg_close: avcodec_close VIDEO_CODEC: %s\n", handle->vcodec->name);

    avcodec_close(handle->vid_codec_context);
    handle->vid_codec_context = NULL;
    /* ?????????? do not attempt to free handle->vid_codec_context (it points to &handle->vid_stream.codec) */
  }

  if((handle->aud_codec_context) && (handle->acodec))
  {
    if(gap_debug) printf("p_wrapper_ffmpeg_close: avcodec_close AUDIO_CODEC: %s\n", handle->acodec->name);

    avcodec_close(handle->aud_codec_context);
    handle->aud_codec_context = NULL;
    /* ??????? do not attempt to free handle->aud_codec_context (it points to &handle->aud_stream.codec) */
  }

  /* Close a media file (but not its codecs) */
  if(gap_debug) printf("p_wrapper_ffmpeg_close: av_close_input_file\n");
  if(handle->vid_input_context) av_close_input_file(handle->vid_input_context);
  if(handle->aud_input_context) av_close_input_file(handle->aud_input_context);

  if(gap_debug) printf("p_wrapper_ffmpeg_close: END\n");

}  /* end p_wrapper_ffmpeg_close */


/* ----------------------------------
 * p_wrapper_ffmpeg_get_next_frame
 * ----------------------------------
 * read one frame from the video_track (stream)
 * and decode the frame
 * TODO: when EOF reached: update total_frames and close the stream)
 *
 */
t_GVA_RetCode
p_wrapper_ffmpeg_get_next_frame(t_GVA_Handle *gvahand)
{
  t_GVA_ffmpeg *handle;
  int       l_got_picture;
  int       l_rc;
  gint64    l_url_offset;
  gint64    l_record_url_offset;
  gint32    l_url_seek_nr;
  gint32    l_len;
  gint32    l_frame_len;
  gint32    l_pkt_size;
  gint32    l_pkt_count;
  gint32    l_prev_pkt_count;
  guint16   l_checksum;

  handle = (t_GVA_ffmpeg *)gvahand->decoder_handle;

  /* redirect picture_rgb pointers to current fcache element */
  avpicture_fill(handle->picture_rgb
                ,gvahand->frame_data
                ,PIX_FMT_RGB24      /* PIX_FMT_RGB24, PIX_FMT_RGBA32, PIX_FMT_BGRA32 */
                ,gvahand->width
                ,gvahand->height
                );
  if(handle->yuv_buff_pix_fmt != handle->vid_codec_context->pix_fmt)
  {
    if(gap_debug)
    {
      printf("## pix_fmt: old:%d new from codec:%d (PIX_FMT_YUV420P:%d PIX_FMT_YUV422P:%d)\n"
	  , (int)handle->yuv_buff_pix_fmt
	  , (int)handle->vid_codec_context->pix_fmt
	  , (int)PIX_FMT_YUV420P
	  , (int)PIX_FMT_YUV422P
	  );
    }

    handle->yuv_buff_pix_fmt = handle->vid_codec_context->pix_fmt;   /* PIX_FMT_YUV420P */
    avpicture_fill(handle->picture_yuv
              ,handle->yuv_buffer
              ,handle->yuv_buff_pix_fmt
              ,gvahand->width
              ,gvahand->height
              );
  } 

  l_got_picture = 0;
  l_rc = 0;
  l_len = 0;
  l_frame_len = 0;
  l_pkt_size = 0;

  l_prev_pkt_count = handle->prev_pkt_count;
  
  /* capture the offest 2 frames ago */
  l_record_url_offset = handle->prev_key_url_offset;
  l_pkt_count = handle->prev_pkt_count;

  l_url_offset = -1;
  l_url_seek_nr = gvahand->current_seek_nr;

  while(l_got_picture == 0)
  {
    int l_pktlen;

    /* if inbuf is empty we must read the next packet */
    if((handle->inbuf_len  < 1) || (handle->inbuf_ptr == NULL))
    {
      if((handle->vid_input_context->packet_buffer == NULL)
      && (l_url_offset < 0))
      {
        /* capture only if packet_buffer is empty
	 * (the following av_read_packet call will read packages
	 * from the packet_buffer if not empty, but reads from stream
	 * on empty packet_buffer)
	 */
        l_url_offset = url_ftell(&handle->vid_input_context->pb);
	if(l_url_offset != handle->prev_url_offset)
	{
	  handle->prev_prev_url_offset = handle->prev_url_offset;
          handle->prev_url_offset = l_url_offset;
          handle->prev_pkt_count = l_pkt_count;
	  l_pkt_count = 0;
	}
      }
    
    
      /* if (gap_debug) printf("p_wrapper_ffmpeg_get_next_frame: before av_read_packet\n"); */
      /**
        * Read a packet from a media file
        * @param s media file handle
        * @param pkt is filled
        * @return 0 if OK. AVERROR_xxx(negative) if error.
        */
      l_pktlen = av_read_packet(handle->vid_input_context, &handle->vid_pkt);
      if(l_pktlen < 0)
      {
         /* EOF reached */
         if (gap_debug) printf("p_wrapper_ffmpeg_get_next_frame: EOF reached (or read ERROR)\n");

         gvahand->total_frames = gvahand->current_frame_nr;
         gvahand->all_frames_counted = TRUE;
         if(handle->aud_stream_index >= 0)
         {
            /* calculate number of audio samples
             * (assuming that audio track has same duration as video track)
             */
            gvahand->total_aud_samples = (gint32)GVA_frame_2_samples(gvahand->framerate, gvahand->samplerate, gvahand->total_frames);
         }

         l_rc = 1;
         break;
      }
      l_pkt_count++;
      
      /* if (gap_debug)  printf("vid_stream:%d pkt.stream_index #%d, pkt.size: %d\n", handle->vid_stream_index, handle->vid_pkt.stream_index, handle->vid_pkt.size); */


      /* check if packet belongs to the selected video stream */
      if(handle->vid_pkt.stream_index != handle->vid_stream_index)
      {
         /* discard packet */
         /* if (gap_debug)  printf("DISCARD Packet\n"); */
         av_free_packet(&handle->vid_pkt);
         continue;
      }


      /* packet is part of the selected video stream, use that packet */
      handle->inbuf_ptr = handle->vid_pkt.data;
      handle->inbuf_len = handle->vid_pkt.size;

      l_pkt_size = handle->vid_pkt.size;
      handle->frame_len += l_pkt_size;
      /*if(gap_debug)   printf("using Packet data:%d size:%d  handle->frame_len:%d\n"
       *                           ,(int)handle->vid_pkt.data
       *			   ,(int)handle->vid_pkt.size
       *			   ,(int)handle->frame_len
       *			   );
       */
       
    }

    /* if (gap_debug) printf("before avcodec_decode_video: inbuf_ptr:%d inbuf_len:%d\n", (int)handle->inbuf_ptr, (int)handle->inbuf_len); */

    /* decode a frame. return -1 if error, otherwise return the number of
     * bytes used. If no frame could be decompressed, *got_picture_ptr is
     * zero. Otherwise, it is non zero.
     */
    l_len = avcodec_decode_video(handle->vid_codec_context  /* AVCodecContext * */
                               ,&handle->big_picture_yuv
                               ,&l_got_picture
                               ,handle->inbuf_ptr
                               ,handle->inbuf_len
                               );
    /*if (gap_debug) printf("after avcodec_decode_video: l_len:%d got_pic:%d\n", (int)l_len, (int)l_got_picture);*/

    if(l_len < 0)
    {
       printf("p_wrapper_ffmpeg_get_next_frame: avcodec_decode_video returned ERROR)\n");
       l_rc = 2;
       break;
    }

    handle->inbuf_ptr += l_len;
    handle->inbuf_len -= l_len;
    if(l_got_picture)
    {
      l_frame_len = handle->frame_len + (l_len - l_pkt_size);
      handle->frame_len = (l_len - l_pkt_size);
    }
   

    /* length of video packet was completely processed, we can free that packet now */
    if(handle->inbuf_len < 1)
    {
      /* if (gap_debug)  printf("FREE Packet\n"); */
      av_free_packet(&handle->vid_pkt);
      handle->vid_pkt.size = 0;             /* set empty packet status */
      handle->vid_pkt.data = NULL;          /* set empty packet status */
    }

  }  /* end while packet_read and decode frame loop */

  if((l_rc == 0)  && (l_got_picture))
  {
    if(gvahand->current_seek_nr > 1)
    {
      /* 1.st frame_len may contain headers (size may be too large)
       */
      handle->max_frame_len = MAX(handle->max_frame_len, l_frame_len);
    }
    
    handle->got_frame_length = l_frame_len;
    
    if (gap_debug) printf("GOT PIC: current_seek_nr:%06d l_frame_len:%d got_pic:%d key:%d\n"
		       , (int)gvahand->current_seek_nr
		       , (int)l_frame_len
		     , (int)l_got_picture
		     , (int)handle->big_picture_yuv.key_frame
		     );

    if(handle->dummy_read == FALSE)
    {
      if (gap_debug) printf("p_wrapper_ffmpeg_get_next_frame: before img_convert\n");

      /* avcodec.img_convert  convert among pixel formats */


      l_rc = img_convert(handle->picture_rgb
                        , PIX_FMT_RGB24                      /* dst */
                        ,handle->picture_yuv
			,handle->yuv_buff_pix_fmt            /* src */
                        ,gvahand->width
                        ,gvahand->height
                        );

      /* if (gap_debug) printf("p_wrapper_ffmpeg_get_next_frame: after img_convert l_rc:%d\n", (int)l_rc); */
    }


    if((gvahand->vindex) 
    && (handle->capture_offset)
    && (handle->big_picture_yuv.key_frame == 1)
    )
    {
#define GVA_LOW_GOP_LIMIT 24
        l_checksum = p_gva_checksum(handle->picture_yuv, gvahand->height);

        /* the automatic GOP detection has a lower LIMIT of 24 frames
	 * GOP values less than the limit can make the videoindex
	 * slow, because the packet reads in libavformat are buffered,
	 * and the index based search starting at the recorded seek offset (in the index)
	 * may not find the wanted keyframe in the first Synchronisation Lopp
	 * if its 1.st packet starts before the recorded seek offset.
	 * 24 frames shold be enough to catch an offest before the start of the wanted
	 * packet.
	 * Increasing the GVA_LOW_GOP_LIMIT would slow down the index based seek
	 * ops.
	 */

        /* printf("GUESS_GOP: prev_key_seek_nr:%d  l_url_seek_nr:%d\n"
	 *       , (int)handle->prev_key_seek_nr
	 *       , (int)l_url_seek_nr); 
	 */
	/* try to guess the GOP size (but use at least  GOPSIZE of 4 frames) */
	if(l_url_seek_nr > handle->prev_key_seek_nr)
	{
	   gdouble l_gopsize;
	   
	   l_gopsize = l_url_seek_nr - handle->prev_key_seek_nr;
	   if(handle->guess_gop_size == 0)
	   {
	     handle->guess_gop_size = l_gopsize;
	   }
	   else
	   {
	     handle->guess_gop_size = MAX(GVA_LOW_GOP_LIMIT, ((handle->guess_gop_size + l_gopsize) / 2));
	   }
	}
        if(l_url_seek_nr >= handle->prev_key_seek_nr + GVA_LOW_GOP_LIMIT)
	{
            handle->prev_key_url_offset = handle->prev_prev_url_offset;
            p_vindex_add_url_offest(gvahand
                         , handle
                         , gvahand->vindex
			 , l_url_seek_nr
			 , l_record_url_offset
			 , (guint16)l_frame_len
			 , l_checksum
			 );
            handle->prev_key_seek_nr = l_url_seek_nr;
	}
    }
    
    gvahand->current_frame_nr = gvahand->current_seek_nr;
    gvahand->current_seek_nr++;
 
    if (gap_debug) printf("p_wrapper_ffmpeg_get_next_frame: current_frame_nr :%d\n"
                         , (int)gvahand->current_frame_nr);

    /* if we found more frames, increase total_frames */
    gvahand->total_frames = MAX(gvahand->total_frames, gvahand->current_frame_nr);
    return(GVA_RET_OK);
  }

  if(l_rc == 1)  { return(GVA_RET_EOF); }

  return(GVA_RET_ERROR);
}  /* end p_wrapper_ffmpeg_get_next_frame */


/* ------------------------------
 * p_wrapper_ffmpeg_seek_frame
 * ------------------------------
 *  TODO:
 *  - howto seek in avformat lib ?
 *    (AVInputFormat provides read_seek function ptr that should be used anyway in future.
 *     but today read_seek is not implemented for most of the formats.
 *     for those input formats with read_seek pointer == NULL we must emulate seek ops !!)
 *  - the current implementation emulates
 *    seek by pos-1 sequential read ops.
 *    seek back: must always reopen to reset stream position to start.
 *    this is very slow, creating a videoindex is recommanded for GUI-based applications
 *
 *  - The videoindex is an external solution outside libavformat
 *    and enables faster seek in the videofile.
 *    a videoindex has stored offsets of the keyframes
 *    that were optional created by the count_frames procedure.
 *    The indexed based seek modifies the libavformat internal straem seek position
 *    by an explicte call of url_fseek.
 *    The index also has information about the (compressed) frame length and a checksum.
 *    In the Sync Loop(s) we try to find exactly the frame that is described in the index.
 *    The postioning to last Group of pictures is done
 *    by sequential read. (it should contain at least one keyframe (I-frame)
 *    that enables clean decoding of the following P and B frames
 *    
 */
#define GVA_IDX_SYNC_STEPSIZE 1
t_GVA_RetCode
p_wrapper_ffmpeg_seek_frame(t_GVA_Handle *gvahand, gdouble pos, t_GVA_PosUnit pos_unit)
{
  t_GVA_ffmpeg *handle;
  t_GVA_RetCode l_rc_rd;
  gint32   l_frame_pos;
  gint32   l_url_frame_pos;
  gint32   l_readsteps;
  int64_t  l_pts_pos_microsec;
  gdouble  l_progress_step;
  t_GVA_Videoindex    *vindex;

  handle = (t_GVA_ffmpeg *)gvahand->decoder_handle;
  vindex = gvahand->vindex;

  switch(pos_unit)
  {
    case GVA_UPOS_FRAMES:
      l_frame_pos = (gint32)pos;
      break;
    case GVA_UPOS_SECS:
      l_frame_pos = (gint32)round(pos * gvahand->framerate);
      l_pts_pos_microsec = (int64_t)(pos * 1000 * 1000);
      break;
    case GVA_UPOS_PRECENTAGE:
      /* is not reliable until all_frames_counted == TRUE */
      l_frame_pos = (gint32)GVA_percent_2_frame(gvahand->total_frames, pos);
      break;
    default:
      l_frame_pos = (gint32)pos;
      break;
  }

/*  if(handle->file_iformat->read_seek == NULL)
 * {
 *    printf("AVFORMAT: %s read_seek not implemented, (emulating SEEK)\n", handle->file_iformat->name);
 * }
 */

  gvahand->percentage_done = 0.0;

  if(gap_debug)
  {
    printf("p_wrapper_ffmpeg_seek_frame: start: l_frame_pos: %d cur_seek:%d cur_frame:%d\n"
                           , (int)l_frame_pos
			   , (int)gvahand->current_seek_nr
			   , (int)gvahand->current_frame_nr
			   );
  }

  handle->dummy_read = TRUE;
  l_url_frame_pos = 0;
  if(vindex)
  {
     gint64  seek_pos;
     gint32  l_idx;
     if(vindex->tabsize_used < 1)
     {
       printf("SEEK: index is >>> EMPTY <<<: vindex->tabsize_used: %d cannot use index!\n", (int)vindex->tabsize_used);
     }
     else
     {
       l_idx = ((l_frame_pos -2) / vindex->stepsize);

       /* make sure that table access limited to used tablesize
        * (this allows usage of incomplete indexes)
	*/
       if(l_idx > vindex->tabsize_used -1)
       {
         if(gap_debug)
	 {
	   printf("SEEK: overflow l_idx: %d limit:%d\n"
	                 , (int)l_idx
			 , (int) vindex->tabsize_used -1
			 );
         }
         l_idx = vindex->tabsize_used -1;
       }
       if(l_idx > 0)
       {
	 while(vindex->ofs_tab[l_idx].seek_nr >= l_frame_pos)
	 {
	   l_idx--;
	   if(l_idx < 1)
	   {
	     break;
	   }
	 }
       }
       
       if(l_idx > 0)
       {
	 l_readsteps = l_frame_pos - gvahand->current_seek_nr;
         if((l_readsteps > vindex->stepsize)
         || (l_readsteps < 0))
	 {
	   gint32  l_synctries;
	   gint32  l_nloops;

           l_nloops = 1;
           while(l_idx >= 0)
	   {
	     if(gap_debug)
	     {
	       printf("SEEK: USING_INDEX: ofs_tab[%d]: ofs64: %d seek_nr:%d flen:%d chk:%d NLOOPS:%d\n"
	       , (int)l_idx
	       , (int)vindex->ofs_tab[l_idx].uni.offset_gint64
	       , (int)vindex->ofs_tab[l_idx].seek_nr
	       , (int)vindex->ofs_tab[l_idx].frame_length
	       , (int)vindex->ofs_tab[l_idx].checksum
	       , (int)l_nloops
	       );
	     }
	       
	     l_synctries = 4 + (vindex->stepsize * GVA_IDX_SYNC_STEPSIZE);
             /* SYNC READ loop
	      * seek to offest found in the index table
	      * then read until we are at the KEYFRAME that matches
	      * in length and checksum with the one from th  index table
	      */
             p_ffmpeg_vid_reopen_read(handle, gvahand);
             seek_pos = vindex->ofs_tab[l_idx].uni.offset_gint64;
             url_fseek(&handle->vid_input_context->pb, seek_pos, SEEK_SET);

             gvahand->current_seek_nr = vindex->ofs_tab[vindex->tabsize_used].uni.offset_gint64;

             l_rc_rd = GVA_RET_OK;
             while((l_rc_rd == GVA_RET_OK)
	     && (l_synctries > 0))
	     {
               l_rc_rd = p_wrapper_ffmpeg_get_next_frame(gvahand);

	      /*
	       *   printf("SEEK_SYNC_LOOP: idx_frame_len:%d  got_frame_length:%d  Key:%d\n"
	       *     , (int)vindex->ofs_tab[l_idx].frame_length
	       *     , (int)handle->got_frame_length
	       *     , (int)handle->big_picture_yuv.key_frame
	       *     );
	       */
	       
 	       if(vindex->ofs_tab[l_idx].frame_length == handle->got_frame_length)
	       {
		 guint16 l_checksum;

		 l_checksum = p_gva_checksum(handle->picture_yuv, gvahand->height);
		 if(l_checksum == vindex->ofs_tab[l_idx].checksum)
		 {
	           /* seems that we have found the wanted key frame */
	           break;
		 }
		 else
		 {
		   if(gap_debug)
		   {
		     /* checksum missmatch is non critical
		      * continue searching for an exactly matching frame
		      */
		     printf("SEEK_SYNC_LOOP: CHKSUM_MISSMATCH: CHECKSUM[%d]:%d  l_checksum:%d\n"
		     , (int)l_idx
		     , (int)vindex->ofs_tab[l_idx].checksum
		     , (int)l_checksum
		     );
		   }
		 }

	       }
	       l_synctries--;
	     }
	     if(l_synctries <= 0)
	     {
	       /* index sync search failed, try with previous index in the table */
	       if(l_idx < GVA_IDX_SYNC_STEPSIZE)
	       {
	         /* index sync search failed, must search slow from start */
                 p_ffmpeg_vid_reopen_read(handle, gvahand);
 	         gvahand->current_seek_nr = 1;
	         l_url_frame_pos = 0;
	       }
	     }
	     else
	     {

               l_url_frame_pos = 1 + vindex->ofs_tab[l_idx].seek_nr;
               if(gap_debug)
	       {
	         printf("SEEK: url_fseek seek_pos: %d  l_idx:%d l_url_frame_pos:%d\n"
	                                , (int)seek_pos
					, (int)l_idx
					, (int)l_url_frame_pos
					);
	       }
               gvahand->current_seek_nr = (gint32)l_url_frame_pos;
               l_readsteps = l_frame_pos - gvahand->current_seek_nr;
               if(gap_debug)
	       {
	         printf("SEEK: l_readsteps: %d\n", (int)l_readsteps);
	       }
  
               break;  /* OK, we are now at read position of the wanted keyframe */
	     }

             /* try another search with previos index table entry */
	     l_idx -= GVA_IDX_SYNC_STEPSIZE;
	     l_nloops++;
	   }
	   
	 }
       }
     }
  }
  
  if(l_url_frame_pos == 0)
  {
    /* emulate seek with N steps of read ops */
    if(l_frame_pos < gvahand->current_seek_nr)
    {
      p_ffmpeg_vid_reopen_read(handle, gvahand);
      l_readsteps = l_frame_pos -1;
    }
    else
    {
      l_readsteps = l_frame_pos - gvahand->current_seek_nr;
    }
  }

  if(gap_debug) printf("p_wrapper_ffmpeg_seek_frame: l_readsteps: %d\n", (int)l_readsteps);

  l_progress_step =  1.0 / MAX((gdouble)l_readsteps, 1.0);
  l_rc_rd = GVA_RET_OK;
  while((l_readsteps > 0)
  &&    (l_rc_rd == GVA_RET_OK))
  {
    l_rc_rd = p_wrapper_ffmpeg_get_next_frame(gvahand);
    gvahand->percentage_done = CLAMP(gvahand->percentage_done + l_progress_step, 0.0, 1.0);

    if(gvahand->do_gimp_progress) {  gimp_progress_update (gvahand->percentage_done); }
    if(gvahand->fptr_progress_callback)
    {
      gvahand->cancel_operation = (*gvahand->fptr_progress_callback)(gvahand->percentage_done, gvahand->progress_cb_user_data);
    }

    l_readsteps--;

    if(gvahand->cancel_operation)
    {
       l_rc_rd = GVA_RET_ERROR;             /* cancel by user request reults in seek error */
       gvahand->cancel_operation = FALSE;   /* reset cancel flag */
    }
  }
  handle->dummy_read = FALSE;

  if(l_rc_rd == GVA_RET_OK)
  {
    if(gap_debug)
    {
       printf("p_wrapper_ffmpeg_seek_frame: SEEK OK: l_frame_pos: %d cur_seek:%d cur_frame:%d\n"
                            , (int)l_frame_pos
			    , (int)gvahand->current_seek_nr
			    , (int)gvahand->current_frame_nr
			    );
    }

    gvahand->current_seek_nr = (gint32)l_frame_pos;

    return(GVA_RET_OK);
  }

  if(l_rc_rd == GVA_RET_EOF)  { return(GVA_RET_EOF); }

  return(GVA_RET_ERROR);
}  /* end p_wrapper_ffmpeg_seek_frame */




/* ------------------------------
 * p_wrapper_ffmpeg_seek_audio
 * ------------------------------
 * this procedure just sets the current_sample position
 * for access of the samples_buffer
 */
t_GVA_RetCode
p_wrapper_ffmpeg_seek_audio(t_GVA_Handle *gvahand, gdouble pos, t_GVA_PosUnit pos_unit)
{
  t_GVA_ffmpeg *handle;
  gint32   l_sample_pos;
  gint32   l_frame_pos;

  handle = (t_GVA_ffmpeg *)gvahand->decoder_handle;

  switch(pos_unit)
  {
    case GVA_UPOS_FRAMES:
      l_sample_pos = (gint32)GVA_frame_2_samples(gvahand->framerate, gvahand->samplerate, pos -1.0);
      break;
    case GVA_UPOS_SECS:
      l_sample_pos = (gint32)round(pos / MAX(gvahand->samplerate, 1.0));
      break;
    case GVA_UPOS_PRECENTAGE:
      l_frame_pos= (gint32)GVA_percent_2_frame(gvahand->total_frames, pos);
      l_sample_pos = (gint32)GVA_frame_2_samples(gvahand->framerate, gvahand->samplerate, l_frame_pos -1);
      break;
    default:
      l_sample_pos = 0;
      break;
  }


  if(gap_debug) printf("p_wrapper_ffmpeg_seek_audio: l_sample_pos:%d\n", (int)l_sample_pos);

  gvahand->current_sample = (gint32)l_sample_pos;
  return(GVA_RET_OK);

}  /* end p_wrapper_ffmpeg_seek_audio */


/* ------------------------------
 * p_wrapper_ffmpeg_get_audio
 * ------------------------------
 */
t_GVA_RetCode
p_wrapper_ffmpeg_get_audio(t_GVA_Handle *gvahand
             ,gint16 *output_i
             ,gint32 channel
             ,gdouble samples
             ,t_GVA_AudPos mode_flag
             )
{
  t_GVA_ffmpeg *handle;
  int      l_rc;
  gint32   l_sample_idx;
  gint32   l_low_sample_idx;
  gint32   l_samples_picked;

  handle = (t_GVA_ffmpeg *)gvahand->decoder_handle;


  if(mode_flag == GVA_AMOD_FRAME)
  {
      p_wrapper_ffmpeg_seek_audio(gvahand
                                    , (gdouble) gvahand->current_frame_nr
                                    , GVA_UPOS_FRAMES
                                    );
  }

  if(mode_flag == GVA_AMOD_REREAD)
  {
    l_sample_idx = gvahand->reread_sample_pos;
  }
  else
  {
    gvahand->reread_sample_pos = gvahand->current_sample;  /* store current position for reread operations */
    l_sample_idx = gvahand->current_sample;
  }

  if(gap_debug) printf("p_wrapper_ffmpeg_get_audio  samples: %d l_sample_idx:%d\n", (int)samples, (int)l_sample_idx);

  l_low_sample_idx = 0;
  if (handle->samples_base[0] > 0)
  {
    l_low_sample_idx = handle->samples_base[0];
  }
  if ((handle->samples_base[1] > 0)
  &&  (handle->samples_base[1] < l_low_sample_idx))
  {
    l_low_sample_idx = handle->samples_base[1];
  }

  if(l_sample_idx < l_low_sample_idx)
  {
    /* the requested sample_idx is to old
     * (is not chached in the sample_buffers any more)
     * we have to reset audio read, and restart reading from the begin
     */
    p_ffmpeg_aud_reopen_read(handle, gvahand);
  }

  /* read and decode audio packets into sample_buffer */
  l_rc = p_read_audio_packets(handle, gvahand, (gdouble)(l_sample_idx + samples));

  /* copy desired range number of samples for the desired channel
   * from sample_buffer to output_i (16bit) sampledata
   */
  l_samples_picked = p_pick_channel( handle
                , gvahand
                , output_i
                , l_sample_idx
                , samples
                , channel
                );

  if(l_rc == 0)
  {
    if(mode_flag != GVA_AMOD_REREAD)
    {
       gvahand->current_sample += l_samples_picked;        /* advance current audio read position */
    }
    return(GVA_RET_OK);
  }
  if(l_rc == 1)  { return(GVA_RET_EOF); }

  return(GVA_RET_ERROR);
}  /* end p_wrapper_ffmpeg_get_audio */


/* ----------------------------------
 * p_wrapper_ffmpeg_count_frames
 * ----------------------------------
 * (re)open a seperated handle for counting
 * to ensure that stream positions are not affected by the count.
 * This procedure optionally creates a videoindex
 * for faster seek access
 */
t_GVA_RetCode
p_wrapper_ffmpeg_count_frames(t_GVA_Handle *gvahand)
{
  t_GVA_ffmpeg*  handle;
  t_GVA_RetCode  l_rc;
  t_GVA_Handle   *copy_gvahand;
  gint32         l_total_frames;
  t_GVA_Videoindex    *vindex;

  if(gap_debug)
  {
    printf("p_wrapper_ffmpeg_count_frames: START\n");
  }
  gvahand->percentage_done = 0.0;

  if(gvahand->vindex == NULL)
  {
    gvahand->vindex = GVA_load_videoindex(gvahand->filename, gvahand->vid_track, GVA_FFMPEG_DECODER_NAME);
  }
  
  if(gvahand->vindex)
  {
     if((gvahand->vindex->total_frames > 0)
     && (gvahand->vindex->track == gvahand->vid_track))
     {
       /* we already have a valid videoindex that contains the real total number */
       if(gap_debug)
       {
	 printf("p_wrapper_ffmpeg_count_frames: have already valid vindex\n");
       }
       gvahand->total_frames = gvahand->vindex->total_frames;
       gvahand->all_frames_counted = TRUE;
       return(GVA_RET_OK);
     }
     /* throw away existing videoindex (because it is unusable) */
     if(gap_debug)
     {
       printf("p_wrapper_ffmpeg_count_frames: throw away existing videoindex\n");
     }
     GVA_free_videoindex(&gvahand->vindex);
  }

  vindex = NULL;
  if(gvahand->create_vindex)
  {
    gvahand->vindex = GVA_new_videoindex();
    vindex = gvahand->vindex;
    if(vindex)
    {
      vindex->tabtype = GVA_IDX_TT_GINT64;
      vindex->ofs_tab = g_new(t_GVA_IndexElem, GVA_VIDINDEXTAB_BLOCK_SIZE);
      vindex->track = gvahand->vid_track;
      if(vindex->ofs_tab)
      {
	vindex->tabsize_allocated = GVA_VIDINDEXTAB_BLOCK_SIZE;
      }
    }
  }

  copy_gvahand = g_malloc0(sizeof(t_GVA_Handle));
  if(gap_debug) printf("p_wrapper_ffmpeg_count_frames: ## before open ## handle:%s\n", gvahand->filename);
  p_wrapper_ffmpeg_open_read(gvahand->filename
                            ,copy_gvahand
                            );
  if(gap_debug) printf("p_wrapper_ffmpeg_count_frames: after p_wrapper_ffmpeg_open_read\n");
  copy_gvahand->current_frame_nr = 0;
  copy_gvahand->current_seek_nr = 1;
  copy_gvahand->vindex = vindex;
  handle = (t_GVA_ffmpeg*)copy_gvahand->decoder_handle;
printf("p_wrapper_ffmpeg_count_frames: ## 10 ## handle:%d\n", (int)handle);
  if(handle)
  {
    if(vindex)
    {
      handle->capture_offset = TRUE;
    }
    handle->dummy_read = TRUE;

    /* percentage_done is just a guess, because we dont know
     * the exact total_frames number before.
     */
    if(gvahand->all_frames_counted)
    {
       l_total_frames = gvahand->total_frames;
    }
    else
    {
       /* we need a guess to update the percentage_done used for progress indication)
        */
       l_total_frames = p_guess_total_frames(gvahand);
    }
    if(gap_debug)
    {
      printf("p_wrapper_ffmpeg_count_frames: begin Counting\n");
    }
    gvahand->frame_counter = 0;
    while(!gvahand->cancel_operation)
    {
       /* READ FRAME */
       l_rc = p_wrapper_ffmpeg_get_next_frame(copy_gvahand);
       if(l_rc != GVA_RET_OK)
       {
          break;  /* eof, or fetch error */
       }
       gvahand->frame_counter++;
       l_total_frames = MAX(l_total_frames, gvahand->frame_counter);

       if(gvahand->all_frames_counted)
       {
         gvahand->percentage_done = (gdouble)l_total_frames / (gdouble)gvahand->frame_counter;
       }
       else
       {
         /* assume we have 5 % frames more to go than total_frames */
         gvahand->percentage_done = (gdouble)gvahand->frame_counter / MAX(((gdouble)l_total_frames * 1.05),1);
         gvahand->total_frames = l_total_frames;
       }

       if(gvahand->do_gimp_progress) {  gimp_progress_update (gvahand->percentage_done); }
       if(gvahand->fptr_progress_callback)
       {
	 gvahand->cancel_operation = (*gvahand->fptr_progress_callback)(gvahand->percentage_done, gvahand->progress_cb_user_data);
       }

    }
    if(gap_debug)
    {
      printf("p_wrapper_ffmpeg_count_frames: stop Counting\n");
    }

    vindex->stepsize = handle->guess_gop_size;
    handle->capture_offset = FALSE;

    /* the copy_gvahand has used reference to the orinal gvahand->vindex
     * this reference must be set NULL before the close is done
     * (otherwise p_wrapper_ffmpeg_close would free the vindex)
     */
    copy_gvahand->vindex = NULL;

    /* close the extra handle (that was opened for counting only) */
    p_wrapper_ffmpeg_close(copy_gvahand);
  }

  g_free(copy_gvahand);

  gvahand->percentage_done = 0.0;

  if(vindex)
  {
    if(gap_debug)
    {
      printf("p_wrapper_ffmpeg_count_frames: before GVA_save_videoindex\n");
      printf("p_wrapper_ffmpeg_count_frames: gvahand->filename:%s\n", gvahand->filename);
    }
    vindex->total_frames = gvahand->frame_counter;
    if(gvahand->cancel_operation)
    {
      /* because of cancel the total_frames is still unknown
       * (vindex->total_frames == 0 is the indicator for incomplete index) 
       */
      vindex->total_frames = 0;
    }
    if(gap_debug)
    {
      printf("p_wrapper_ffmpeg_count_frames: before GVA_save_videoindex\n");
    }
    GVA_save_videoindex(vindex, gvahand->filename, GVA_FFMPEG_DECODER_NAME);
  }

  if(!gvahand->cancel_operation)
  {
     gvahand->total_frames = gvahand->frame_counter;
     gvahand->all_frames_counted = TRUE;
  }
  else
  {
     gvahand->total_frames = MAX(gvahand->total_frames, gvahand->frame_counter);
     gvahand->cancel_operation = FALSE;  /* reset cancel flag */
     if(gap_debug)
     {
       printf("p_wrapper_ffmpeg_count_frames: RETURN ERROR\n");
     }
     return(GVA_RET_ERROR);
  }

  if(gap_debug)
  {
    printf("p_wrapper_ffmpeg_count_frames: RETURN OK\n");
  }

  return(GVA_RET_OK);

}  /* end p_wrapper_ffmpeg_count_frames */



/* -----------------------------
 * p_ffmpeg_new_dec_elem
 * -----------------------------
 * create a new decoder element and init with
 * functionpointers referencing the QUICKTIME
 * specific Procedures
 */
t_GVA_DecoderElem  *
p_ffmpeg_new_dec_elem(void)
{
  t_GVA_DecoderElem  *dec_elem;

  dec_elem = g_malloc0(sizeof(t_GVA_DecoderElem));
  if(dec_elem)
  {
    dec_elem->decoder_name         = g_strdup(GVA_FFMPEG_DECODER_NAME);
    dec_elem->decoder_description  = g_strdup("FFMPEG Decoder (Decoder for MPEG1,MPEG4(DivX),RealVideo) (EXT: .mpg,.vob,.avi,.rm,.mpeg)");
    dec_elem->fptr_check_sig       = &p_wrapper_ffmpeg_check_sig;
    dec_elem->fptr_open_read       = &p_wrapper_ffmpeg_open_read;
    dec_elem->fptr_close           = &p_wrapper_ffmpeg_close;
    dec_elem->fptr_get_next_frame  = &p_wrapper_ffmpeg_get_next_frame;
    dec_elem->fptr_seek_frame      = &p_wrapper_ffmpeg_seek_frame;
    dec_elem->fptr_seek_audio      = &p_wrapper_ffmpeg_seek_audio;
    dec_elem->fptr_get_audio       = &p_wrapper_ffmpeg_get_audio;
    dec_elem->fptr_count_frames    = &p_wrapper_ffmpeg_count_frames;
    dec_elem->fptr_get_video_chunk = NULL;  /* &p_wrapper_ffmpeg_get_video_chunk */
    dec_elem->next = NULL;
  }

  return (dec_elem);
}  /* end p_ffmpeg_new_dec_elem */




/* ===================================
 * private (static) helper  PROCEDURES
 * ===================================
 * ===================================
 */


static void
p_debug_codec_list(void)
{
  AVCodec *codec;

  for(codec = first_avcodec; codec != NULL; codec = codec->next)
  {
     printf("\n-------------------------\n");
     printf("name: %s\n", codec->name);
     printf("type: %d\n", codec->type);
     printf("id: %d\n",   codec->id);
     printf("priv_data_size: %d\n",   codec->priv_data_size);
     printf("capabilities: %d\n",   codec->capabilities);
     printf("init fptr: %d\n",   (int)codec->init);
     printf("encode fptr: %d\n",   (int)codec->encode);
     printf("close fptr: %d\n",   (int)codec->close);
     printf("decode fptr: %d\n",   (int)codec->decode);
  }
}

/* -----------------------------
 * p_ffmpeg_init
 * -----------------------------
 */
static void
p_ffmpeg_init(void)
{
  if(gap_debug) printf("p_ffmpeg_init: before av_register_all \n");


 /**
  * Initialize libavcodec and register all the codecs and formats.
  * (does  avcodec_init(),  avcodec_register_all, protocols etc....
  */
  av_register_all();

  if(gap_debug)
  {
    p_debug_codec_list();
  }
}  /* end p_ffmpeg_init */


/* -----------------------------
 * p_ff_open_input
 * -----------------------------
 * open input context and codec for video or audio
 * depending on the input parameter vid_open.
 *
 * IN: filename (videofile to open for read)
 * INOUT: gvahand
 * INOUT: handle
 * IN: vid_open TRUE: open stream for video only
 *               FALSE: open stream for audio only
 */
static gboolean
p_ff_open_input(char *filename, t_GVA_Handle *gvahand, t_GVA_ffmpeg*  handle, gboolean vid_open)
{
  AVCodecContext *acc;
  AVFormatContext *ic;
  AVInputFormat *iformat;
  int err, ii, ret, rfps;

  if(gap_debug) printf("p_ff_open_input: START  vid_open:%d\n", (int)vid_open);


  /* open the input file with generic libav function
   * Opens a media file as input. The codec are not opened.
   * Only the file header (if present) is read.
   */
  err = av_open_input_file(&ic, filename, NULL, 0, NULL);
  if (err < 0)
  {
     if(gap_debug) printf("p_ff_open_input: av_open_input_file FAILED: %d\n", (int)err);
     return(FALSE);
  }

  iformat = ic->iformat;

  if(gap_debug)
  {
    printf("ic: iformat: %d\n", (int)iformat);
    if(iformat)
    {
      printf("iformat name: %s\n", iformat->name);
      printf("iformat long_name: %s\n", iformat->long_name);
      printf("iformat FPTR read_seek: %d\n", (int)iformat->read_seek);
    }
  }


  /* If not enough info to get the stream parameters, we decode the
   * first frames to get it. (used in mpeg case for example)
   */
  ret = av_find_stream_info(ic);
  if (ret < 0)
  {
     if(gap_debug) printf("p_ff_open_input:%s: could not find codec parameters\n", filename);
     return(FALSE);
  }



  /* --------------------------
   * check all input streams
   * and pick up infos of the desired streams
   * (specified by vid_track aud_track)
   */
  gvahand->vtracks = 0;
  gvahand->atracks = 0;
  for(ii=0; ii < ic->nb_streams; ii++)
  {
    acc = &ic->streams[ii]->codec;

    switch(acc->codec_type)
    {
        case CODEC_TYPE_AUDIO:
            gvahand->atracks++;  /* count all audiostraems as audiotrack */
            if(gap_debug) printf("\nInput Audio channels: %d\n", acc->channels);
            if((gvahand->atracks == gvahand->aud_track)
            || (gvahand->atracks == 1))
            {
              if(!vid_open)
              {
                handle->aud_stream_index = ii;
                handle->aud_codec_id = acc->codec_id;
                handle->aud_codec_context = acc;

                handle->aud_stream = ic->streams[ii];
              }
              gvahand->audio_cannels = acc->channels;
              gvahand->samplerate = acc->sample_rate;
            }
            break;
        case CODEC_TYPE_VIDEO:
            gvahand->vtracks++; /* count all videostraems as videotrack */
            if((gvahand->vtracks == gvahand->vid_track)
            || (gvahand->vtracks == 1))
            {
              if(vid_open)
              {
                handle->vid_stream_index = ii;
                handle->vid_codec_id = acc->codec_id;
                handle->vid_codec_context = acc;

                handle->vid_stream = ic->streams[ii];
              }
              gvahand->height = acc->height;
              gvahand->width = acc->width;
              rfps = ic->streams[ii]->r_frame_rate;
              acc->workaround_bugs = FF_BUG_AUTODETECT;
              acc->error_resilience = 2;
              acc->error_concealment = 3;
              acc->idct_algo= 0;
              /*
               * if(acc->codec->capabilities & CODEC_CAP_TRUNCATED)
               *   acc->flags|= CODEC_FLAG_TRUNCATED; 
	       */
              if(/*acc->codec_id==CODEC_ID_MPEG4 || */acc->codec_id==CODEC_ID_MPEG1VIDEO)
              {
                  acc->flags|= CODEC_FLAG_TRUNCATED;
              }
              if (acc->frame_rate != rfps)
              {
                  printf("\nSeems that stream %d comes from film source: %2.2f->%2.2f\n",
                      ii, (float)acc->frame_rate / (float)acc->frame_rate_base,
                      (float)rfps / (float)acc->frame_rate_base);
              }

              /* update the current frame rate to match the stream frame rate */
              gvahand->framerate = (gdouble)rfps / (gdouble)acc->frame_rate_base;
            }
            break;
        default:
            break;  /* av_abort(); */
    }
  }



  /* open video codec (if needed) */
  if((gvahand->vtracks > 0)
  && (vid_open))
  {
    if(handle->vid_codec_context)
    {
      handle->vcodec = avcodec_find_decoder(handle->vid_codec_id);
    }

    if(handle->vcodec)
    {
      /* open codec  */
      if (avcodec_open(handle->vid_codec_context, handle->vcodec) < 0)
      {
         printf("Error while opening video codec %s\n", handle->vcodec->name);
         return(FALSE);
      }
      if(gap_debug) printf("p_wrapper_ffmpeg_open_read: open video codec : %s OK\n", handle->vcodec->name);
    }
    else
    {
      handle->vid_stream_index = -1;
      printf("cant decode video (no compatible codec found)\n");
      /* return(FALSE); */
    }

    if(gap_debug)
    {
      if(handle->vid_codec_context) printf("(2) CodecPointer  AVCodecContext->codec : %d\n", (int)handle->vid_codec_context->codec);
      if(handle->vid_codec_context->codec) printf("(2) Codec FunctionPointer  AVCodecContext->codec->decode : %d\n", (int)handle->vid_codec_context->codec->decode);
    }
  }



  /* open audio codec (if needed) */
  if((gvahand->atracks > 0)
  && (!vid_open))
  {
    if(handle->aud_codec_context)
    {
      handle->acodec = avcodec_find_decoder(handle->aud_codec_id);
    }
    if(handle->acodec)
    {
      if (avcodec_open(handle->aud_codec_context, handle->acodec) < 0)
      {
         printf("** Error while opening audio codec %s\n", handle->acodec->name);
         return(FALSE);
      }
      if(gap_debug) printf("p_wrapper_ffmpeg_open_read: open audio codec : %s OK\n", handle->acodec->name);
    }
    else
    {
      printf("cant decode audio (no compatible codec found)\n");
      handle->aud_stream_index = -1;
      /* return(FALSE); */
    }

    if(gap_debug)
    {
      if(handle->aud_codec_context) printf("(3) CodecPointer  AVCodecContext->codec : %d\n", (int)handle->aud_codec_context->codec);
      if(handle->aud_codec_context->codec) printf("(3) Codec FunctionPointer  AVCodecContext->codec->decode : %d\n", (int)handle->aud_codec_context->codec->decode);
    }
  }


  if(vid_open)
  {
    handle->vid_input_context = ic;

    if(gap_debug) printf("p_ff_open_input: END  vid_input_context:%d\n", (int)handle->vid_input_context);
  }
  else
  {
    handle->aud_input_context = ic;

    if(gap_debug) printf("p_ff_open_input: END  aud_input_context:%d\n", (int)handle->aud_input_context);
  }

  return(TRUE);
}  /* end p_ff_open_input */



/* ------------------------------
 * p_ffmpeg_vid_reopen_read
 * ------------------------------
 * internal procedure to reset read position
 * reopen the video context of a videofile for read
 * (that is already open for read)
 * the audio context (if there is one) is not affected !
 */
static void
p_ffmpeg_vid_reopen_read(t_GVA_ffmpeg *handle, t_GVA_Handle *gvahand)
{
    if(gap_debug) printf("p_ffmpeg_vid_reopen_read: REOPEN\n");

    /* CLOSE the video codec */
    if((handle->vid_codec_context) && (handle->vcodec))
    {
      avcodec_close(handle->vid_codec_context);
      handle->vid_codec_context = NULL;
    }

    /* Close a media file (just the video context) */
    if(handle->vid_input_context)
    {
      av_close_input_file(handle->vid_input_context);
      handle->vid_input_context = NULL;
    }

    handle->vid_pkt.size = 0;             /* REstart with empty packet */
    handle->vid_pkt.data = NULL;          /* REstart with empty packet */
    handle->inbuf_len = 0;            /* start with empty buffer */
    handle->inbuf_ptr = NULL;         /* start with empty buffer, after 1.st av_read_packet: pointer to pkt.data read pos */

    /* RE-open for the VIDEO part */
    p_ff_open_input(gvahand->filename, gvahand, handle, TRUE);

}  /* end p_ffmpeg_vid_reopen_read */


/* ------------------------------
 * p_ffmpeg_aud_reopen_read
 * ------------------------------
 * internal procedure to reset read position
 * reopen the audio context of a videofile for read
 * (that is already open for read)
 * the video context (if there is one) is not affected !
 */
static void
p_ffmpeg_aud_reopen_read(t_GVA_ffmpeg *handle, t_GVA_Handle *gvahand)
{
    if(gap_debug) printf("p_ffmpeg_aud_reopen_read: REOPEN\n");


    /* CLOSE the audio codec */
    if((handle->aud_codec_context) && (handle->acodec))
    {
      avcodec_close(handle->aud_codec_context);
      handle->aud_codec_context = NULL;
    }

    /* Close a media file (just the video context) */
    if(handle->aud_input_context)
    {
      av_close_input_file(handle->aud_input_context);
      handle->aud_input_context = NULL;
    }

    handle->vid_pkt.size = 0;             /* REstart with empty packet */
    handle->vid_pkt.data = NULL;          /* REstart with empty packet */
    handle->inbuf_len = 0;            /* start with empty buffer */
    handle->inbuf_ptr = NULL;         /* start with empty buffer, after 1.st av_read_packet: pointer to pkt.data read pos */

    handle->samples_read = 0.0;
    handle->bf_idx = 0;
    handle->samples_base[0] = 0;
    handle->samples_base[1] = 0;
    handle->bytes_filled[0] = 0;
    handle->bytes_filled[1] = 0;
    handle->output_samples_ptr = handle->samples_buffer[0];

    /* RE-open for the AUDIO part */
    p_ff_open_input(gvahand->filename, gvahand, handle, FALSE);

}  /* end p_ffmpeg_aud_reopen_read */




/* ------------------------------
 * p_pick_channel
 * ------------------------------
 * copy 16bit samples from samples_buffer[0/1] to output_i buffer
 * only the samples from the requested channel are copied.
 * copy starts at sample_idx and affects the given number of samples.
 * NOTE: the samples_buffer must have been filled up
 *       at least until the needed position (sample_idx + samples) before
 *       this routine is called.
 *  there will be no size checks !!
 *      output_i must be large enogh to hold samples * 2 bytes.
 */
static gint32
p_pick_channel( t_GVA_ffmpeg *handle
              , t_GVA_Handle *gvahand
              , gint16 *output_i
              , gint32 sample_idx
              , gint32 samples
              , gint32 channel
              )
{
  guchar *this_peek_ptr;
  guchar *prev_peek_ptr;
  guchar *poke_ptr;
  gint    bytes_per_sample;
  gint32  ii;
  gint32  l_samples;
  gint32  l_this_samples;
  gint32  l_prev_samples;
  gint32  peek_idx;
  gint32  l_samples_picked;
  gint32  this_idx;
  gint32  prev_idx;

  l_samples_picked = 0;
  bytes_per_sample =  2 * gvahand->audio_cannels;

  this_idx = handle->bf_idx ;
  prev_idx = (this_idx + 1) & 1;
  this_peek_ptr = NULL;
  prev_peek_ptr = NULL;
  l_prev_samples = 0;
  l_this_samples = 0;
  l_samples  = samples;

  if((samples + sample_idx) > handle->samples_read)
  {
      /* copy only as much as was read before.
       * (normally we dont get in here,
       * but if we did not read enough (? at EOF) l_samples gets 0 or negative
       * and prevents copying uninitiated data
       */
      l_this_samples = (gint32)  (handle->samples_read - sample_idx);

      if(gap_debug) printf("p_pick_channel(2): l_this_samples:%d\n", (int)l_this_samples);
  }

  if(sample_idx >= handle->samples_base[this_idx])
  {
    /* start sample is available in the current samples_buffer
     * (there is no need to check the prev buffer)
     */

    peek_idx = ((sample_idx - handle->samples_base[this_idx])  * bytes_per_sample) + (2* (channel -1));

    this_peek_ptr = handle->samples_buffer[this_idx];
    this_peek_ptr += peek_idx;
    l_this_samples = l_samples;
  }
  else
  {
    if(sample_idx >= handle->samples_base[prev_idx])
    {
      gint32 l_avail_samples;

      /* start sample is available in the prev samples_buffer
       * but the requested number of samples may be spread
       * from the other buffer to this (current) buffer
       * (there may be a need to read from both buffers)
       */
      peek_idx = ((sample_idx - handle->samples_base[prev_idx])  * bytes_per_sample) + (2* (channel -1));

      prev_peek_ptr = handle->samples_buffer[prev_idx];
      prev_peek_ptr += peek_idx;

      l_avail_samples = (handle->bytes_filled[prev_idx] - peek_idx) / bytes_per_sample;

      if(l_samples <= l_avail_samples)
      {
        l_prev_samples = l_samples;     /* peek everything from the prev buffer */
      }
      else
      {
        l_prev_samples = l_avail_samples;
        l_this_samples  = l_samples - l_avail_samples;  /* peek the rest from this buffer */
        this_peek_ptr = handle->samples_buffer[this_idx];
      }

    }
    else
    {
       printf("** ERROR could not fetch %d samples at once (max Buffersize is: %d)\n"
              , (int)samples
              , (int)GVA_SAMPLES_BUFFER_SIZE / 2
              );
       return(0);
    }
  }

  poke_ptr = (guchar *)output_i;

  if(gap_debug)
  {
    printf("p_pick_channel(1): sample_idx:%d ch:%d bytes_p_s:%d sam_read:%d peek_idx:%d prev_peek_ptr:%d  this_peek_ptr:%d poke_ptr:%d\n"
                        ,(int)sample_idx
                        ,(int)channel
                        ,(int)bytes_per_sample
                        ,(int)handle->samples_read
                        ,(int)peek_idx
                        ,(int)prev_peek_ptr
                        ,(int)this_peek_ptr
                        ,(int)poke_ptr
                        );
    printf("p_pick_channel(2): samples:%d   prev_samples:%d this_samples:%d\n"
                        ,(int)samples
                        ,(int)l_prev_samples
                        ,(int)l_this_samples
                        );
  }

  /* we may peek the start from the prev buffer */
  for(ii=0; ii < l_prev_samples; ii++)
  {
     /* if(ii%60 == 0) printf("\n%12d:", (int)peek_ptr);  */
     /* printf("%02x%02x ", (int)peek_ptr[0] ,(int)peek_ptr[1]); */

     *poke_ptr = prev_peek_ptr[0];
     poke_ptr++;
     *poke_ptr = prev_peek_ptr[1];
     poke_ptr++;
     prev_peek_ptr += bytes_per_sample;
     l_samples_picked++;
  }

  /* then we can peek the rest from the this (current) buffer */
  for(ii=0; ii < l_this_samples; ii++)
  {
     /* if(ii%60 == 0) printf("\n%12d:", (int)peek_ptr);  */
     /* printf("%02x%02x ", (int)peek_ptr[0] ,(int)peek_ptr[1]); */

     *poke_ptr = this_peek_ptr[0];
     poke_ptr++;
     *poke_ptr = this_peek_ptr[1];
     poke_ptr++;
     this_peek_ptr += bytes_per_sample;
     l_samples_picked++;
  }



  if(gap_debug)
  {
    printf("p_pick_channel END: l_samples_picked:%d l_samples:%d prev_peek_ptr:%d this_peek_ptr:%d poke_ptr%d\n"
                      ,(int)l_samples_picked
                      ,(int)l_samples
                      ,(int)prev_peek_ptr
                      ,(int)this_peek_ptr
                      ,(int)poke_ptr
                      );
  }

  return(l_samples_picked);

}  /* end p_pick_channel */


/* ------------------------------
 * p_read_audio_packets
 * ------------------------------
 * IN: max_sample_pos is the position of the last audiosample
 *     that is needed.
 *     this procedure does write to samples_buffer[0] and/or samples_buffer[1]
 *     by reading and decoding audio packets as long as the samples_buffer [0/1]
 *     are filled up to max_sample_pos (or eof is reached)
 */
static int
p_read_audio_packets( t_GVA_ffmpeg *handle, t_GVA_Handle *gvahand, gint32 max_sample_pos)
{
  int       data_size;
  int       l_rc;

  l_rc = 0;

   if(handle->samples_buffer[0] == NULL)
   {
      /* allocate 2 buffers, each is large enough to hold at least one uncompressed
       * audiopacket. those buffers are then filled with uncomressed 16bit audiosamples.
       * each time one of the buffers is full, the write pointer output_samples_ptr
       * switches to the other buffer
       */
      handle->samples_buffer_size = GVA_SAMPLES_BUFFER_SIZE * gvahand->audio_cannels * 2;
      handle->samples_buffer[0] = g_malloc0(handle->samples_buffer_size);
      handle->samples_buffer[1] = g_malloc0(handle->samples_buffer_size);
      handle->samples_read = 0.0;
      handle->bf_idx = 0;
      handle->samples_base[0] = 0;
      handle->samples_base[1] = 0;
      handle->bytes_filled[0] = 0;
      handle->bytes_filled[1] = 0;
      handle->biggest_data_size = 4096;

      gvahand->current_sample = 0.0;
      gvahand->reread_sample_pos = 0.0;
      handle->output_samples_ptr = handle->samples_buffer[0];
   }

   if (gap_debug)
   {
     printf("p_read_audio_packets: before WHILE max_sample_pos: %d\n", (int)max_sample_pos);
     printf("p_read_audio_packets: before WHILE samples_read: %d\n", (int)handle->samples_read);
   }

   while(handle->samples_read < max_sample_pos)
   {
     int l_len;
     int l_pktlen;

    /* if abuf is empty we must read the next packet */
    if((handle->abuf_len  < 1) || (handle->abuf_ptr == NULL))
    {
      /*if (gap_debug) printf("p_read_audio_packets: before av_read_packet aud_input_context:%d\n", (int)handle->aud_input_context);*/
      /**
        * Read a packet from a media file
        * @param s media file handle
        * @param pkt is filled
        * @return 0 if OK. AVERROR_xxx if error.
        */
      l_pktlen = av_read_packet(handle->aud_input_context, &handle->aud_pkt);
      if(l_pktlen < 0)
      {
         /* EOF reached */
         if (gap_debug) printf("p_read_audio_packets: EOF reached (or read ERROR)\n");

         gvahand->total_aud_samples = handle->samples_read;
         gvahand->all_samples_counted = TRUE;

         l_rc = 1;
         break;
      }

      /*if (gap_debug)  printf("aud_stream:%d pkt.stream_index #%d, pkt.size: %d samples_read:%d\n", handle->aud_stream_index, handle->aud_pkt.stream_index, handle->aud_pkt.size, (int)handle->samples_read);*/


      /* check if packet belongs to the selected audio stream */
      if(handle->aud_pkt.stream_index != handle->aud_stream_index)
      {
         /* discard packet */
         /* if (gap_debug)  printf("DISCARD Packet\n"); */
         av_free_packet(&handle->aud_pkt);
         continue;
      }

      /* if (gap_debug)  printf("using Packet\n"); */

      /* packet is part of the selected video stream, use that packet */
      handle->abuf_ptr = handle->aud_pkt.data;
      handle->abuf_len = handle->aud_pkt.size;
    }

    /* if (gap_debug) printf("before avcodec_decode_audio: abuf_ptr:%d abuf_len:%d\n", (int)handle->abuf_ptr, (int)handle->abuf_len); */

    /* decode a frame. return -1 if error, otherwise return the number of
     * bytes used. If no audio frame could be decompressed, data_size is
     * zero or lnegative.
     */
    l_len = avcodec_decode_audio(handle->aud_codec_context  /* AVCodecContext * */
                               ,(int16_t *)handle->output_samples_ptr
                               ,&data_size
                               ,handle->abuf_ptr
                               ,handle->abuf_len
                               );

    if (gap_debug)
    {
      printf("after avcodec_decode_audio: l_len:%d data_size:%d samples_read:%d\n"
                         , (int)l_len
                         , (int)data_size
                         , (int)handle->samples_read
                         );
    }

    if(l_len < 0)
    {
       printf("p_read_audio_packets: avcodec_decode_audio returned ERROR)\n");
       l_rc = 2;
       break;
    }

    /* Some bug in mpeg audio decoder gives */
    /* data_size < 0, it seems they are overflows */
    if (data_size > 0)
    {
       gint    bytes_per_sample;

       /* debug code block: count not null bytes in the decoded data block */
       if(gap_debug)
       {
          gint32   ii;
          gint32   l_sum;

          l_sum = 0;
          for(ii=0; ii < data_size; ii++)
          {
            /* if(ii%60 == 0) printf("\n%012d:", (int)&handle->output_samples_ptr[ii]); */
            /* printf("%02x", (int)handle->output_samples_ptr[ii]); */

            if(handle->output_samples_ptr[ii] != 0)
            {
              l_sum++;
            }
          }
          printf("\nSUM of NOT NULL bytes: %d output_samples_ptr:%d\n", (int)l_sum, (int)handle->output_samples_ptr);
       }

       /* check for the biggest uncompressed packet size */
       if (data_size  > handle->biggest_data_size)
       {
         handle->biggest_data_size = data_size;
       }
       /* add the decoded packet size to one of the samples_buffers
        * and advance write position
        */
       bytes_per_sample =  MAX((2 * gvahand->audio_cannels), 1);
       handle->bytes_filled[handle->bf_idx] += data_size;
       handle->samples_read += (data_size  / bytes_per_sample);

       /* check if we have enough space in the current samples_buffer (for the next packet) */
       if(handle->bytes_filled[handle->bf_idx] + handle->biggest_data_size  > GVA_SAMPLES_BUFFER_SIZE)
       {
         /* no more space in the current samples_buffer,
          * switch write pointer to the other buffer
          * (reset this other buffer now, and overwrite at next packet read)
          */
         handle->bf_idx = (handle->bf_idx +1) & 1;

         if(gap_debug) printf("WRITE SWITCH samples_buffer to %d\n", (int)handle->bf_idx);

         handle->output_samples_ptr = handle->samples_buffer[handle->bf_idx];
         handle->samples_base[handle->bf_idx] = handle->samples_read;
         handle->bytes_filled[handle->bf_idx] = 0;
       }
       else
       {
         /* enouch space, continue writing to the same buffer */
         handle->output_samples_ptr += data_size;
       }

       /* if more samples found then update total_aud_samples (that was just a guess) */
       gvahand->total_aud_samples = MAX(handle->samples_read, gvahand->total_aud_samples);
    }

    handle->abuf_ptr += l_len;
    handle->abuf_len -= l_len;

    /* length of audio packet was completely processed, we can free that packet now */
    if(handle->abuf_len < 1)
    {
      /* if (gap_debug)  printf("FREE Packet\n"); */
      av_free_packet(&handle->aud_pkt);
      handle->aud_pkt.size = 0;             /* set empty packet status */
      handle->aud_pkt.data = NULL;          /* set empty packet status */
    }

  }  /* end while packet_read and decode audio frame loop */

  return(l_rc);
}  /* end p_read_audio_packets */



/* ----------------------------------
 * p_vindex_add_url_offest
 * ----------------------------------
 */
static void
p_vindex_add_url_offest(t_GVA_Handle *gvahand
                         , t_GVA_ffmpeg *handle
                         , t_GVA_Videoindex *vindex
			 , gint32 seek_nr
			 , gint64 url_offset
			 , guint16 frame_length
			 , guint16 checksum
			 )
{
  static gint64 s_last_seek_nr;
  
  if(vindex->tabsize_used > 0)
  {
    if(seek_nr <= s_last_seek_nr)
    {
      return;
    }
  }
  s_last_seek_nr = seek_nr;
  
  if(vindex->tabsize_used >= vindex->tabsize_allocated -1)
  {
    t_GVA_IndexElem *redim_ofs_tab;
    t_GVA_IndexElem *old_ofs_tab;
    gint32  new_size;

    /* on overflow redim the table by adding GVA_VIDINDEXTAB_BLOCK_SIZE elements */
    new_size = vindex->tabsize_allocated + GVA_VIDINDEXTAB_BLOCK_SIZE;
    redim_ofs_tab = g_new(t_GVA_IndexElem, new_size);
    if(redim_ofs_tab)
    {
      if(gap_debug)
      {
        printf("p_vindex_add_url_offest: REDIM:vindex->tabsize_allocated %d NEW:%d\n"
                          , (int)vindex->tabsize_allocated
			  , (int)new_size
			  );
      }
      old_ofs_tab = vindex->ofs_tab;
      memcpy(redim_ofs_tab, old_ofs_tab, (vindex->tabsize_allocated * sizeof(t_GVA_IndexElem)));
      vindex->ofs_tab = redim_ofs_tab;
      g_free(old_ofs_tab);
      vindex->tabsize_allocated = new_size;
    }
  }

  if(vindex->tabsize_used < vindex->tabsize_allocated -1)
  {
    vindex->ofs_tab[vindex->tabsize_used].uni.offset_gint64 = url_offset;
    vindex->ofs_tab[vindex->tabsize_used].seek_nr = seek_nr;
    vindex->ofs_tab[vindex->tabsize_used].frame_length = frame_length;
    vindex->ofs_tab[vindex->tabsize_used].checksum = checksum;

    if(gap_debug)
    {
      printf("p_vindex_add_url_offest: ofs_tab[%d]: ofs64: %d seek_nr:%d flen:%d chk:%d\n"
		       , (int)vindex->tabsize_used
		       , (int)vindex->ofs_tab[vindex->tabsize_used].uni.offset_gint64
		       , (int)vindex->ofs_tab[vindex->tabsize_used].seek_nr
		       , (int)vindex->ofs_tab[vindex->tabsize_used].frame_length
		       , (int)vindex->ofs_tab[vindex->tabsize_used].checksum
		       );
    }
    vindex->tabsize_used++;
  }
           
}  /* end p_vindex_add_url_offest */



/* -----------------------------
 * p_gva_checksum
 * -----------------------------
 * for performance reasons the checksum
 * counts only odd rows of the middle column
 */
static guint16
p_gva_checksum(AVPicture *picture_yuv, gint height)
{
  guint16 l_checksum;
  gint ii;
  
  l_checksum = 0;
  for(ii=0;ii<4;ii++)
  {
    if(picture_yuv->linesize[ii] > 0)
    {
      guchar *buf;
      gint row;
     
      buf = picture_yuv->data[ii];
      buf += picture_yuv->linesize[ii] / 2;
      for(row=0; row < height; row+=2)
      {
        l_checksum += *buf;
        buf += picture_yuv->linesize[ii];
      }
    }
  }
  return (l_checksum);
}  /* end p_gva_checksum */


#endif  /* ENABLE_GVA_LIBAVFORMAT */
