/* gap_file_util.c
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
#include "libgimp/gimp.h"


#include "gap_file_util.h"

extern      int gap_debug; /* ==0  ... dont print debug infos */

/*************************************************************
 *          TOOL FUNCTIONS                                   *
 *************************************************************/


/* --------------------
 * gap_file_get_filesize
 * --------------------
 *   get the filesize of a file.
 * in: fname: The filename of the file to check.
 * returns: The filesize.
 */
gint32
gap_file_get_filesize(const char *fname)
{
  struct stat  stat_buf;

  if (0 != stat(fname, &stat_buf))
  {
    printf ("stat error on '%s'\n", fname);
    return(0);
  }

  return((gint32)(stat_buf.st_size));
}


/* -----------------------
 * gap_file_load_file_segment
 * -----------------------
 * load one segment of (audio)data from a file.
 * into the buffer segment_data_ptr (must be allocated by the calling program)
 *
 * return number of bytes read from file
 */
gint32
gap_file_load_file_segment(const char *filename
                  ,guchar *segment_data_ptr /* IN: pointer to memory at least segent_size large, OUT: filled with segment data */
                  ,gint32 seek_index        /* start byte of datasegment in file */
                  ,gint32 segment_size      /* segment size in byets (must be a multiple of 4) */
                  )
{
  FILE *fp;
  gint32      l_bytes_read;


  l_bytes_read = 0;

  fp = fopen(filename, "rb");
  if (fp)
  {

    fseek(fp, seek_index, SEEK_SET);

    /* read full segement size (or smaller rest until EOF) */
    l_bytes_read = fread(segment_data_ptr, 1, (size_t)segment_size, fp);

    fclose(fp);
  }

  if(gap_debug) printf("gap_file_load_file_segment: %s seek_idx:%d  bytes_read:%d\n", filename, (int)seek_index, (int)l_bytes_read);

  return(l_bytes_read);

}  /* end gap_file_load_file_segment */


/* -------------------------
 * gap_file_load_file_len
 * -------------------------
 *    Load a file into a memory buffer
 *  in: fname: The name of the file to load
 *  returns: A pointer to the allocated buffer with the file content.
 */
char *
gap_file_load_file_len(const char *fname, gint32 *filelen)
{
  FILE	      *fp;
  char        *l_buff_ptr;
  long	       len;

  *filelen = 0;

  /* File Laenge ermitteln */
  len = gap_file_get_filesize(fname);
  if (len < 1)
  {
    return(NULL);
  }

  /* Buffer allocate */
  l_buff_ptr=(char *)g_malloc(len+1);
  if(l_buff_ptr == NULL)
  {
    printf ("calloc error (%d Bytes not available)\n", (int)len);
    return(NULL);
  }
  l_buff_ptr[len] = '\0';

  /* File in eine Buffer laden */
  fp = fopen(fname, "r");		    /* open read */
  if(fp == NULL)
  {
    printf ("open(read) error on '%s'\n", fname);
    return(NULL);
  }
  fread(l_buff_ptr, 1, (size_t)len, fp);	    /* read */
  fclose(fp);				    /* close */

  *filelen = len;
  return(l_buff_ptr);
}	/* end gap_file_load_file_len */

/* -------------------------
 * gap_file_load_file
 * -------------------------
 */
char *
gap_file_load_file(const char *fname)
{
  gint32 l_filelen;

  return(gap_file_load_file_len(fname, &l_filelen));
}  /* end gap_file_load_file */

