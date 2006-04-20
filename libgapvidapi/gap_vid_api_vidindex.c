/* vid_api_vidindex.c
 *
 * GAP Video read API implementation of vidindex file handling procedures.
 * vidindex files are created optional to speed up
 * frame seek operations.
 * vidindex files are machine and decoder dependent files
 * that store seek offsets (gint64 or gdouble) in an access table where (framenumber / stepsize)
 * is the table index.
 * standard stepsize of 50 reults in one entry for every 50.th frame.
 * usually vidindex is built in the frame_count procedure,
 * but only if the decoder has an implementation for videoindex.
 * (the 1.st decoder with videoindex implementation is libavformat FFMPEG) 
 *
 * 2004.03.06   hof created
 *
 */
static char *   p_build_videoindex_filename(const char *filename, gint32 track, const char *decoder_name);
static gboolean p_equal_mtime(time_t mtime_idx, time_t mtime_file);

/* ----------------------------------
 * GVA_build_videoindex_filename
 * ----------------------------------
 * external variante using track numbers starting at 1
 */
char *
GVA_build_videoindex_filename(const char *filename, gint32 track, const char *decoder_name)
{
  return (p_build_videoindex_filename(filename
         , MIN(track -1, 0)
         ,decoder_name
         ));
}  /* end GVA_build_videoindex_filename */


/* ------------------------------------
 * p_build_videoindex_filename
 * ------------------------------------
 * internal variante using track numbers starting at 0
 */
static char *
p_build_videoindex_filename(const char *filename, gint32 track, const char *decoder_name)
{
  static gchar name[40];
  gchar *vindex_file;
  gchar *filename_part;
  gchar *uri;
  gchar *gvaindexes_dir;
  
  uri = GVA_filename_to_uri(filename);
  if(uri)
  {
    GVA_md5_string(name, uri);
  }
  else
  {
    return (NULL);
  }
  
  filename_part = g_strdup_printf("%s.%d.%s.gvaidx", name, (int)track, decoder_name);
  gvaindexes_dir = gimp_gimprc_query("video-index-dir");
  if(gvaindexes_dir)
  {
    vindex_file = g_build_filename(gvaindexes_dir, filename_part, NULL);
    g_free(gvaindexes_dir);  
  }
  else
  {
    /* nothing configured. in that case we use a default directory */
    //vindex_file = g_build_filename(g_get_home_dir(), "gvaindexes", filename_part, NULL);
    vindex_file = g_build_filename(gimp_directory(), "gvaindexes", filename_part, NULL);
  }


  if(gap_debug)
  {
    printf("VIDINDEX: filename_part: %s\n", filename_part);
    printf("VIDINDEX: vindex_file: %s\n", vindex_file);
  }

  g_free(filename_part);  

  return(vindex_file);
}  /* end p_build_videoindex_filename */



/* ----------------------------------
 * p_equal_mtime
 * ----------------------------------
 * return if bot mtime values are equal
 * 
 * if the difference is exactly 3600 seconds 
 * accept this as equal and return TRUE
 *
 * return FALSE in all other cases.
 *
 * This behaviour compensates the problem were the video index creation
 * was done in normal time (for instance in december)
 * but the query is done after switch to daylight saving time (for instance in may)
 * where the mtime delivered via a query with stat uses 1 hour earlier mtime
 * (assuming that the video file was not modified since december) and won't match
 * with the mtime value stored in the videoindex.
 *
 * therefore we tolerate the difference (with a very little risk to use an old index
 * in case there really was a modififaction with exactly one hour difference)
 * but keeps all videoindexes usable after switch to daylight saving time.
 */
static gboolean 
p_equal_mtime(time_t mtime_idx, time_t mtime_file)
{
  if(mtime_idx == mtime_file)
  {
    return (TRUE);
  }
  
  if (abs(mtime_idx - mtime_file) == 3600)
  {
    if(gap_debug)
    {
      printf("GVA_videoindex faking equal mtimes  MTIME_INDEX:%ld MTIME_FILE:%ld  (3600 sec different)\n"
             , (long)mtime_idx
             , (long)mtime_file);
    }
    return (TRUE);
  }
  
  return (FALSE);
}  /* end p_equal_mtime */



/* ----------------------------------
 * GVA_build_video_toc_filename
 * ----------------------------------
 */
char *
GVA_build_video_toc_filename(const char *filename, const char *decoder_name)
{
  static gchar name[40];
  gchar *toc_file;
  gchar *filename_part;
  gchar *uri;
  gchar *gvaindexes_dir;
  
  uri = GVA_filename_to_uri(filename);
  if(uri)
  {
    GVA_md5_string(name, uri);
  }
  else
  {
    printf("TOC-NAME: uri is NULL filename:%s\n", filename);
    return (NULL);
  }
  
  filename_part = g_strdup_printf("%s.%s.toc", name, decoder_name);
  gvaindexes_dir = gimp_gimprc_query("video-index-dir");
  if(gvaindexes_dir)
  {
    toc_file = g_build_filename(gvaindexes_dir, filename_part, NULL);
    g_free(gvaindexes_dir);  
  }
  else
  {
    /* nothing configured. in that case we use a default directory */
    //toc_file = g_build_filename(g_get_home_dir(), "gvaindexes", filename_part, NULL);
    toc_file = g_build_filename(gimp_directory(), "gvaindexes", filename_part, NULL);
  }


  if(gap_debug)
  {
    printf("TOC-NAME: filename_part: %s\n", filename_part);
    printf("TOC-NAME: toc_file: %s\n", toc_file);
  }
  g_free(filename_part);  

  return(toc_file);
}  /* end GVA_build_video_toc_filename */

/* ----------------------------------
 * GVA_new_videoindex
 * ----------------------------------
 */
t_GVA_Videoindex *
GVA_new_videoindex(void)
{
  t_GVA_Videoindex *vindex;
  
  vindex = g_malloc0(sizeof(t_GVA_Videoindex));
  if(vindex)
  {
    vindex->tabtype = GVA_IDX_TT_GINT64;
    vindex->videoindex_filename = NULL;
    vindex->tocfile = NULL;
    vindex->videofile_uri = NULL;
    vindex->stepsize = GVA_VIDINDEXTAB_DEFAULT_STEPSIZE;
    vindex->tabsize_used = 0;
    vindex->tabsize_allocated = 0;
    vindex->track = 1;
    vindex->mtime = 0;        /* is set later when saved to file */
    vindex->ofs_tab = NULL;
  }
  
  return(vindex);
  
}  /* end GVA_new_videoindex */


/* ----------------------------------
 * GVA_free_videoindex
 * ----------------------------------
 */
void
GVA_free_videoindex(t_GVA_Videoindex **in_vindex)
{
  t_GVA_Videoindex *vindex;
  
  if(in_vindex)
  {
    vindex = *in_vindex;
    if(vindex)
    {
      if(vindex->videoindex_filename) { g_free(vindex->videoindex_filename); }
      if(vindex->videofile_uri)       { g_free(vindex->videofile_uri); }
      if(vindex->ofs_tab)             { g_free(vindex->ofs_tab); }
      if(vindex->tocfile)             { g_free(vindex->tocfile); }
      g_free(vindex);
    }
    *in_vindex = NULL;
  }
  
}  /* end GVA_free_videoindex */


/* ----------------------------------
 * GVA_load_videoindex
 * ----------------------------------
 */
t_GVA_Videoindex *
GVA_load_videoindex(const char *filename, gint32 track, const char *decoder_name)
{
  t_GVA_Videoindex *vindex;
  FILE *fp;
  gboolean success;
  gboolean delete_flag;

  if(gap_debug)
  {
    printf("GVA_load_videoindex  START\n");
  }
  if(filename == NULL)     { return (NULL); }
  if(decoder_name == NULL) { return (NULL); }

  if(gap_debug)
  {
    printf("GVA_load_videoindex  file:%s\n", filename);
  }
  success = FALSE;
  delete_flag = FALSE;
  vindex = GVA_new_videoindex();
  if(vindex)
  {
    vindex->videoindex_filename = p_build_videoindex_filename(filename
                                                               , track
                                                               , decoder_name);
    if(vindex->videoindex_filename)
    {
      if(gap_debug) 
      {
        printf("GVA_load_videoindex  videoindex_filename:%s\n", vindex->videoindex_filename);
      }
      fp = fopen(vindex->videoindex_filename, "rb");
      if(fp)
      {
	gint32 rd_len;
	gint32 rd_size;
	gint   l_flen;
	gint32 l_mtime;

	rd_len = fread(&vindex->hdr, 1, sizeof(vindex->hdr), fp);
	if(rd_len)
	{
          vindex->stepsize = atol(vindex->hdr.val_step);
          vindex->tabsize_used = atol(vindex->hdr.val_size);
          vindex->track = atol(vindex->hdr.val_trak);
          vindex->total_frames = atol(vindex->hdr.val_ftot);
          vindex->tabsize_allocated = atol(vindex->hdr.val_size);
          vindex->mtime = atol(vindex->hdr.val_mtim);

          if(gap_debug) 
          {
            printf("GVA_load_videoindex MTIM:  vindex->hdr.val_mtim:%s\n"
               , vindex->hdr.val_mtim);
          }

	  l_mtime = GVA_file_get_mtime(filename);
	  if(p_equal_mtime(l_mtime, vindex->mtime) == TRUE)
	  {
            l_flen = atol(vindex->hdr.val_flen);
	    if(l_flen > 0)
	    {
	      /* read the videofile_uri of the videofile */
	      vindex->videofile_uri = g_malloc0(l_flen);
	      if(vindex->videofile_uri)
	      {
        	rd_len = fread(vindex->videofile_uri, 1, l_flen, fp);
	      }
	      else
	      {
		fseek(fp, l_flen, SEEK_CUR);
	      }
	    }

            vindex->tabtype = GVA_IDX_TT_UNDEFINED;
            if(strcmp(vindex->hdr.val_type, "gdouble") == 0)
            {
              vindex->tabtype = GVA_IDX_TT_GDOUBLE;
            }
            if(strcmp(vindex->hdr.val_type, "gint64") == 0)
            {
              vindex->tabtype = GVA_IDX_TT_GINT64;
            }


            rd_len = 0;
	    rd_size = -1;
            switch(vindex->tabtype)
            {
              case GVA_IDX_TT_GINT64:
              case GVA_IDX_TT_GDOUBLE:
		rd_size = sizeof(t_GVA_IndexElem) * vindex->tabsize_used;
		if(rd_size > 0)
		{
        	  vindex->ofs_tab = g_new(t_GVA_IndexElem, vindex->tabsize_used);
        	  if(vindex->ofs_tab)
        	  {
        	    rd_len = fread(vindex->ofs_tab, 1, rd_size, fp);
        	  }
		}
        	break;
              default:
        	break;
            }
            if(rd_len == rd_size)
            {
              success = TRUE;
              if(gap_debug) 
              {
                printf("GVA_load_videoindex  SUCCESS\n");
              }
            }
	  }
	  else
	  {
            delete_flag = TRUE;
            if(gap_debug)
	    {
	      printf("\nGVA_load_videoindex  TOO OLD  videoindex_filename:%s\n"
                     , vindex->videoindex_filename);
              printf("GVA_load_videoindex  MTIME_INDEX:%ld FILE:%ld\n"
                     , (long)vindex->mtime
                     , (long)l_mtime);
	    }
	  }

	}
	fclose(fp);
	if(delete_flag)
	{
	  /* delete OLD videoindex
	   * (that has become unusable because mtime does not match with videofile) */
	  remove(vindex->videoindex_filename);
	}
      }
      else
      {
        if(gap_debug)
        {
          printf("GVA_load_videoindex  FILE NOT FOUND %s\n", vindex->videoindex_filename);
        }
      }
    }
    if(!success)
    {
      if(gap_debug)
      {
        printf("GVA_load_videoindex  NO vindex available\n");
      }
      GVA_free_videoindex(&vindex);
    }
   
  }

  return(vindex);
  
}  /* end GVA_load_videoindex */



/* ----------------------------------
 * GVA_save_videoindex
 * ----------------------------------
 */
gboolean
GVA_save_videoindex(t_GVA_Videoindex *vindex, const char *filename, const char *decoder_name)
{
  FILE *fp;
  gint l_flen;
  
  if(vindex == NULL)       { return (FALSE); }
  if(filename == NULL)     { return (FALSE); }
  if(decoder_name == NULL) { return (FALSE); }
  
  if(vindex->videofile_uri)
  {
    g_free(vindex->videofile_uri);
  }
  vindex->videofile_uri = GVA_filename_to_uri(filename);
  if(vindex->videofile_uri == NULL)
  {
    return (FALSE);
  }
  
  vindex->mtime = GVA_file_get_mtime(filename);

  /* use one or 2 extra bytes for one or 2 terminating \0 characters
   * (l_flen must be even number)
   */
  l_flen = 1 + (strlen(vindex->videofile_uri) / 2);
  l_flen *= 2;

  g_snprintf(vindex->hdr.key_identifier, sizeof(vindex->hdr), "GVA_VIDEOINDEX");
  g_snprintf(vindex->hdr.key_type, sizeof(vindex->hdr.key_type), "TYPE:");
  switch(vindex->tabtype)
  {
    case GVA_IDX_TT_GDOUBLE:
      g_snprintf(vindex->hdr.val_type, sizeof(vindex->hdr.val_type), "gdouble");
      break;
    case GVA_IDX_TT_GINT64:
    case GVA_IDX_TT_UNDEFINED:
      g_snprintf(vindex->hdr.val_type, sizeof(vindex->hdr.val_type), "gint64");
      break;
  }
  g_snprintf(vindex->hdr.key_step, sizeof(vindex->hdr.key_step), "STEP");
  g_snprintf(vindex->hdr.val_step, sizeof(vindex->hdr.val_step), "%d", (int)vindex->stepsize);
  g_snprintf(vindex->hdr.key_size, sizeof(vindex->hdr.key_size), "SIZE");
  g_snprintf(vindex->hdr.val_size, sizeof(vindex->hdr.val_size), "%d", (int)vindex->tabsize_used);
  g_snprintf(vindex->hdr.key_trak, sizeof(vindex->hdr.key_trak), "TRAK");
  g_snprintf(vindex->hdr.val_trak, sizeof(vindex->hdr.val_trak), "%d", (int)vindex->track);
  g_snprintf(vindex->hdr.key_ftot, sizeof(vindex->hdr.key_ftot), "FTOT");
  g_snprintf(vindex->hdr.val_ftot, sizeof(vindex->hdr.val_ftot), "%d", (int)vindex->total_frames);
  g_snprintf(vindex->hdr.key_deco, sizeof(vindex->hdr.key_deco), "DECO");
  g_snprintf(vindex->hdr.val_deco, sizeof(vindex->hdr.val_deco), "%s", decoder_name);
  g_snprintf(vindex->hdr.key_mtim, sizeof(vindex->hdr.key_mtim), "MTIM");
  g_snprintf(vindex->hdr.val_mtim, sizeof(vindex->hdr.val_mtim), "%ld", (long)vindex->mtime);
  g_snprintf(vindex->hdr.key_flen, sizeof(vindex->hdr.key_flen), "FLEN");
  g_snprintf(vindex->hdr.val_flen, sizeof(vindex->hdr.val_flen), "%d", l_flen);
  
  vindex->videoindex_filename = p_build_videoindex_filename(filename, vindex->track, decoder_name);
  if(vindex->videoindex_filename)
  {
    fp = fopen(vindex->videoindex_filename, "wb");
    if(fp)
    {
      /* write HEAEDR */
      fwrite(&vindex->hdr, 1, sizeof(vindex->hdr), fp);

      /* write VIDEOFILE_URI + terminating \0 character(s)  */
      {
	gchar *uri_buffer;

	uri_buffer = g_malloc0(l_flen);
	g_snprintf(uri_buffer, l_flen, "%s", vindex->videofile_uri);
	fwrite(uri_buffer, 1, l_flen, fp);
	g_free(uri_buffer);
      }

      /* write offset table */
      fwrite(vindex->ofs_tab, 1, sizeof(t_GVA_IndexElem) * vindex->tabsize_used, fp);
      fclose(fp);
      return(TRUE);
    }
    else
    {
      gint l_errno;
      
      l_errno = errno;
      g_message(_("ERROR: Failed to write videoindex file\n"
		"file: '%s'\n"
		"%s")
		, vindex->videoindex_filename
		, g_strerror (l_errno));
      
    }
  }
 
  return (FALSE);
  
}  /* end GVA_save_videoindex */


/* ----------------------------------
 * GVA_debug_print_videoindex
 * ----------------------------------
 */
void
GVA_debug_print_videoindex(t_GVA_Handle *gvahand)
{
  t_GVA_Videoindex    *vindex;
  
  vindex = gvahand->vindex;
  printf("\n");
  printf("GVA_debug_print_videoindex: START\n");
  if(vindex)
  {
    gint l_idx;
    
    for(l_idx=0; l_idx < vindex->tabsize_used; l_idx++)
    {
      printf("VINDEX: ofs_tab[%d]: ofs64: %d seek_nr:%d flen:%d chk:%d\n"
	       , (int)l_idx
	       , (int)vindex->ofs_tab[l_idx].uni.offset_gint64
	       , (int)vindex->ofs_tab[l_idx].seek_nr
	       , (int)vindex->ofs_tab[l_idx].frame_length
	       , (int)vindex->ofs_tab[l_idx].checksum
	       );
    }
  }
  
  printf("GVA_debug_print_videoindex: END\n\n");
  
}  /* end GVA_debug_print_videoindex */
