
void
p_example_read_video(char *videofile, gdouble skip_seconds, gint32 nframes)
{
  t_GVA_Handle* gvahand;
  gint32 vid_track;
  gint32 aud_track;

  vid_track = 1; aud_track = 1;
  gvahand = GVA_open_read(videofile, vid_track, aud_track);
  if(gvahand)
  {
    t_GVA_RetCode  l_rc;
    gint32 framenumber;
    GimpRunMode runmode;

    /* print informations about the videofile */
    printf("Videofile        : %s\n", videofile);
    printf("  framesize      : %d x %d\n", (int)gvahand->width, (int)gvahand->width );
    printf("  # of frames    : %d\n", (int)gvahand->total_frames );
    printf("  framerate      : %f (f/sec)\n", (float)gvahand->framerate );
    printf("Decoder Name     : %s\n", gvahand->dec_elem->decoder_name);

    
    runmode  = GIMP_RUN_INTERACTIVE;
    
    if (skip_seconds > 0.0)
    {
      /* skip the the trailer (time in seconds) */
      l_rc = GVA_seek_frame(gvahand, skip_seconds, GVA_UPOS_SECS);
    }
    
    /* read nframes from the video */
    for(framenumber=1; framenumber <= nframes; framenumber++)
    {
       gboolean delete_mode = TRUE;
       gint32   deinterlace;    /* 0.. NO deinterlace, 1..odd rows, 2..even rows */
       gdouble  threshold;      /* 0.0 <= threshold <= 1.0 */
       char *framename;

       deinterlace = 1;
       /* threshold for interpolated rows (only used if deinterlace != 0)
        * - big thresholds 1.0 do smooth mix interpolation
        * - small thresholds keep hard edges (does not mix different colors)
        * - threshold 0.0 does not interpolate at all and just makes a copy of the previous row
        */
       threshold = 1.0;
       
       /* fetch one frame to buffer gvahand->frame_data
        * (and proceed position to next frame) 
        */
       l_rc = GVA_get_next_frame(gvahand);
       
       if(l_rc == GVA_RET_OK)
       {
         /* convert fetched frame from buffer to gimp image gvahand->image_id
          * by creating a new layer gvahand->layer_id 
          * delete_mode TRUE does first delete gvahand->layer_id
          * (delete_mode FALSE would not delete the layer and the layerstack would
          *  grow upto full nframes Layers in the last turn of the loop)
          */
         l_rc = GVA_frame_to_gimp_layer(gvahand, delete_mode, framenumber, deinterlace, threshold);

         /* save the extracted frames using gimp_file_save 
          * (the fileformat is selected automatically by the extension)
          */       
         framename = g_strdup_printf("frame_%04d.jpg");
         printf ("saving: %s\n", framename);
         gimp_file_save(runmode, gvahand->image_id, gvahand->layer_id, framename,  framename);
         g_free(framename);       
 
         /* foreach following frame use the same save settings (same jpeg quality)
          * as given by the User in the 1.st INTERACTIVE Run
          */
         runmode  = GIMP_RUN_WITH_LAST_VALS;
       }
    }
    GVA_close(gvahand);
  }
}  /* end p_example_read_video */
