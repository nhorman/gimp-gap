/* vid_api_util.c
 *
 * GAP Video read API implementation of utility procedures.
 *
 * 2004.03.14   hof created
 *
 */


/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX END fcache procedures */


/* CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC START copies gap_lib procedures */

/* --------------------
 * p_get_filesize
 * --------------------
 *   get the filesize of a file.
 * in: fname: The filename of the file to check.
 * returns: The filesize.
 */
static gint32
p_get_filesize(char *fname)
{
  struct stat  stat_buf;

  if (0 != stat(fname, &stat_buf))
  {
    printf ("stat error on '%s'\n", fname);
    return(0);
  }

  return((gint32)(stat_buf.st_size));
}  /* end p_get_filesize */


/* ----------------------------------
 * p_alloc_fname
 * ----------------------------------
 * build the framname by concatenating basename, nr and extension.
 * the Number part has leading zeroes, depending
 * on filenames with the same basename and extension on disc.
 *
 * if no similar discfiles were found 6 digits (with leading zeroes)
 * are used per default.
 *
 * if a similar discfilename is found, the number of digits/leading zeroes
 * is set equal to the discfile found.
 * example:
 *   basename == "frame_", nr == 5, ext == ".xcf"
 *   - discfile was found with name:  "frame_00001.xcf"
 *     return ("frame_00001.xcf");
 *
 *   - discfile was found with name:  "frame_001.xcf"
 *     return ("frame_001.xcf");
 *
 * return the resulting framename string
 *   (the calling program should g_free this string
 *    after use)
 */
static char*
p_alloc_fname(char *basename, long nr, char *extension)
{
  gchar *l_fname;
  gint   l_digits_used;
  gint   l_len;
  long   l_nr_chk;

  if(basename == NULL) return (NULL);
  l_len = (strlen(basename)  + strlen(extension) + 10);
  l_fname = (char *)g_malloc(l_len);

    l_digits_used = 6;
    if(nr < 100000)
    {
       /* try to figure out if the frame numbers are in
        * 4-digit style, with leading zeroes  "frame_0001.xcf"
        * or not                              "frame_1.xcf"
        */
       l_nr_chk = nr;

       while(l_nr_chk >= 0)
       {
         /* check if frame is on disk with 6-digit style framenumber */
         g_snprintf(l_fname, l_len, "%s%06ld%s", basename, l_nr_chk, extension);
         if (g_file_test(l_fname, G_FILE_TEST_EXISTS))
         {
            l_digits_used = 6;
            break;
         }

         /* check if frame is on disk with 5-digit style framenumber */
         g_snprintf(l_fname, l_len, "%s%05ld%s", basename, l_nr_chk, extension);
         if (g_file_test(l_fname, G_FILE_TEST_EXISTS))
         {
            l_digits_used = 5;
            break;
         }

         /* check if frame is on disk with 4-digit style framenumber */
         g_snprintf(l_fname, l_len, "%s%04ld%s", basename, l_nr_chk, extension);
         if (g_file_test(l_fname, G_FILE_TEST_EXISTS))
         {
            l_digits_used = 4;
            break;
         }

         /* check if frame is on disk with 3-digit style framenumber */
         g_snprintf(l_fname, l_len, "%s%03ld%s", basename, l_nr_chk, extension);
         if (g_file_test(l_fname, G_FILE_TEST_EXISTS))
         {
            l_digits_used = 3;
            break;
         }

         /* check if frame is on disk with 2-digit style framenumber */
         g_snprintf(l_fname, l_len, "%s%02ld%s", basename, l_nr_chk, extension);
         if (g_file_test(l_fname, G_FILE_TEST_EXISTS))
         {
            l_digits_used = 2;
            break;
         }



         /* now check for filename without leading zeroes in the framenumber */
         g_snprintf(l_fname, l_len, "%s%ld%s", basename, l_nr_chk, extension);
         if (g_file_test(l_fname, G_FILE_TEST_EXISTS))
         {
            l_digits_used = 1;
            break;
         }
         l_nr_chk--;

         /* if the frames nr and nr-1  were not found
          * try to check frames 1 and 0
          * to limit down the loop to max 4 cycles
          */
         if((l_nr_chk == nr -2) && (l_nr_chk > 1))
         {
           l_nr_chk = 1;
         }
      }
    }
    else
    {
      /* numbers > 100000 have 6 digits or more */
      l_digits_used = 0;
    }

  g_free(l_fname);

  switch(l_digits_used)
  {
    case 6:  l_fname = g_strdup_printf("%s%06ld%s", basename, nr, extension);
             break;
    case 5:  l_fname = g_strdup_printf("%s%05ld%s", basename, nr, extension);
             break;
    case 4:  l_fname = g_strdup_printf("%s%04ld%s", basename, nr, extension);
             break;
    case 3:  l_fname = g_strdup_printf("%s%03ld%s", basename, nr, extension);
             break;
    case 2:  l_fname = g_strdup_printf("%s%02ld%s", basename, nr, extension);
             break;
    default: l_fname = g_strdup_printf("%s%ld%s", basename, nr, extension);
             break;
  }
  return(l_fname);
}    /* end p_alloc_fname */


/* ----------------------------------------------------
 * DELETE image
 * ----------------------------------------------------
 * this procedure deletes a frame image
 * (only from the GIMP allocated Memory)
 *
 * further it makes a Workaround for a memory leak Problem in gimp-1.2.2
 */
static void
p_gimp_image_delete(gint32 image_id)
{
    gimp_image_scale(image_id, 2, 2);
    if(gap_debug) printf("SCALED down to 2x2 id = %d (workaround for gimp_image-delete problem)\n", (int)image_id);

    gimp_image_undo_enable(image_id); /* clear undo stack */
    gimp_image_delete(image_id);
}

/* CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC END copies of gap_lib procedures */



/* --------------------------------
 * GVA_md5_string
 * --------------------------------
 * name must point to a buffer of at least 32 Bytes
 * tht will be filled with the MD5 hashstring.
 */
void
GVA_md5_string(char *name, const char *uri)
{
  if(uri)
  {
    guchar       digest[16];
    guchar       n;
    gint         i;
    
    gimp_md5_get_digest (uri, -1, digest);

    for (i = 0; i < 16; i++)
    {
	n = (digest[i] >> 4) & 0xF;
	name[i * 2]     = (n > 9) ? 'a' + n - 10 : '0' + n;

	n = digest[i] & 0xF;
	name[i * 2 + 1] = (n > 9) ? 'a' + n - 10 : '0' + n;
    }

    name[32] = '\0';
  }  
}  /* end GVA_md5_string */

/* --------------------------------
 * GVA_file_get_mtime
 * --------------------------------
 */
gint32
GVA_file_get_mtime(const char *filename)
{
  struct stat  l_stat;

  if (0 == stat(filename, &l_stat))
  {
    return(l_stat.st_mtime);
  }
  return(0);
  
}  /* end GVA_file_get_mtime */

/* --------------------------------
 * GVA_filename_to_uri
 * --------------------------------
 */
gchar*
GVA_filename_to_uri(const char *filename)
{
  gchar *uri;
  gchar *absolute;

  if (! g_path_is_absolute (filename))
  {
      gchar *current;

      current = g_get_current_dir ();
      absolute = g_build_filename (current, filename, NULL);
      g_free (current);
  }
  else
  {
      absolute = g_strdup (filename);
  }

  uri = g_filename_to_uri (absolute, NULL, NULL);

  g_free (absolute);

  return uri;
}  /* end GVA_filename_to_uri */

