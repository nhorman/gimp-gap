/* gap_lib.c
 * 1997.11.18 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * basic GAP types and utility procedures
 * 
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
 * 1.3.18a  2003/08/23   hof: - all gap_debug messages to stdout (do not mix with stderr)
 *                            - do not save thumbnails in p_decide_save_as because it saves
 *                              to a temp filename that is renamed later after successful save
 * 1.3.16c  2003/07/12   hof: - triggers for automatic onionskinlayer create and remove
 *                              bugfix gap_vid_edit_paste
 * 1.3.14a  2003/05/27   hof: - moved basic gap operations to new module gap_base_ops
 *                            - moved procedures for thumbnail handling to gap_thumbnail
 *                            - conditional save now depends on
 *                               GIMP standard gimprc keyword "trust-dirty-flag"
 *                               (removed support for old GAP private keyword "video-unconditional-frame-save")
 * 1.3.12a; 2003/05/03   hof: merge into CVS-gimp-gap project, added gap_renumber, p_alloc_fname 6 digit support
 * 1.3.11a; 2003/01/19   hof: conditional SAVE  (based on gimp_image_is_dirty), use gimp_directory
 * 1.3.9a;  2002/10/28   hof: minor cleanup (replace strcpy by g_strdup)
 * 1.3.5a;  2002/04/21   hof: gimp_palette_refresh changed name to: gimp_palettes_refresh
 *                            gap_locking (now moved to gap_lock.c)
 * 1.3.4b;  2002/03/15   hof: p_load_named_frame now uses gimp_displays_reconnect (removed gap_exchange_image.h)
 * 1.3.4a;  2002/03/11   hof: port to gimp-1.3.4
 * 1.2.2a;  2001/10/21   hof: bufix # 61677 (error in duplicate frames GUI) 
 *                            and disable duplicate for Unsaved/untitled Images.
 *                            (creating frames from such images with a default name may cause problems
 *                             as unexpected overwriting frames or mixing animations with different sized frames)
 * 1.2.1a;  2001/07/07   hof: p_file_copy use binary modes in fopen (hope that fixes bug #52890 in video/duplicate)
 * 1.1.29a; 2000/11/23   hof: gap locking (changed to procedures and placed here)
 * 1.1.28a; 2000/11/05   hof: check for GIMP_PDB_SUCCESS (not for FALSE)
 * 1.1.20a; 2000/04/25   hof: new: p_get_video_paste_name p_vid_edit_clear
 * 1.1.17b; 2000/02/27   hof: bug/style fixes
 * 1.1.14a; 1999/12/18   hof: handle .xvpics on fileops (copy, rename and delete)
 *                            new: p_get_frame_nr,
 * 1.1.9a;  1999/09/14   hof: handle frame filenames with framenumbers
 *                            that are not the 4digit style. (like frame1.xcf)
 * 1.1.8a;  1999/08/31   hof: for AnimFrame Filtypes != XCF:
 *                            p_decide_save_as does save INTERACTIVE at 1.st time
 *                            and uses GIMP_RUN_WITH_LAST_VALS for subsequent calls
 *                            (this enables to set Fileformat specific save-Parameters
 *                            at least at the 1.st call, using the save dialog
 *                            of the selected (by gimp_file_save) file_save procedure.
 *                            in NONINTERACTIVE mode we have no access to
 *                            the Fileformat specific save-Parameters
 *          1999/07/22   hof: accept anim framenames without underscore '_'
 *                            (suggested by Samuel Meder)
 * 0.99.00; 1999/03/15   hof: prepared for win/dos filename conventions
 * 0.98.00; 1998/11/30   hof: started Port to GIMP 1.1:
 *                               exchange of Images (by next frame) is now handled in the
 *                               new module: gap_exchange_image.c
 * 0.96.02; 1998/07/30   hof: extended gap_dup (duplicate range instead of singele frame)
 *                            added gap_shift
 * 0.96.00               hof: - now using gap_arr_dialog.h
 * 0.95.00               hof:  increased duplicate frames limit from 50 to 99
 * 0.93.01               hof: fixup bug when frames are not in the current directory
 * 0.90.00;              hof: 1.st (pre) release
 */
#include "config.h"
 
/* SYSTEM (UNIX) includes */ 
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <signal.h>           /* for kill */
#ifdef HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* GIMP includes */
#include "gtk/gtk.h"
#include "gap-intl.h"
#include "libgimp/gimp.h"

#ifdef G_OS_WIN32
#include <io.h>
#  ifndef S_ISDIR
#    define S_ISDIR(m) ((m) & _S_IFDIR)
#  endif
#  ifndef S_ISREG
#    define S_ISREG(m) ((m) & _S_IFREG)
#  endif
#endif

#ifdef G_OS_WIN32
#include <direct.h>		/* For _mkdir() */
#define mkdir(path,mode) _mkdir(path)
#endif

#ifdef G_OS_WIN32
#include <process.h>		/* For _getpid() */
#endif

/* GAP includes */
#include "gap_layer_copy.h"
#include "gap_lib.h"
#include "gap_pdb_calls.h"
#include "gap_arr_dialog.h"
#include "gap_lock.h"
#include "gap_navi_activtable.h"
#include "gap_thumbnail.h"
#include "gap_vin.h"
#include "gap_onion_base.h"


extern      int gap_debug; /* ==0  ... dont print debug infos */
 
/* ------------------------------------------ */
/* forward  working procedures */
/* ------------------------------------------ */

static int          p_save_old_frame(t_anim_info *ainfo_ptr, t_video_info *vin_ptr);
static int          p_decide_save_as(gint32 image_id, char *sav_name);
static gint32       p_save_named_image2(gint32 image_id, char *sav_name, GimpRunMode run_mode, gboolean enable_thumbnailsave);


/* ============================================================================
 * p_strdup_*_underscore
 *   duplicate string and if last char is no underscore add one at end.
 *   duplicate string and delete last char if it is the underscore
 * ============================================================================
 */
char *
p_strdup_add_underscore(char *name)
{
  int   l_len;
  char *l_str;
  if(name == NULL)
  {
    return(g_strdup("\0"));
  }

  l_len = strlen(name);
  l_str = g_malloc(l_len+1);
  strcpy(l_str, name);
  if(l_len > 0)
  {
    if (name[l_len-1] != '_')
    {
       l_str[l_len    ] = '_';
       l_str[l_len +1 ] = '\0';
    }
      
  }
  return(l_str);
}

char *
p_strdup_del_underscore(char *name)
{
  int   l_len;
  char *l_str;
  if(name == NULL)
  {
    return(g_strdup("\0"));
  }

  l_len = strlen(name);
  l_str = g_strdup(name);
  if(l_len > 0)
  {
    if (l_str[l_len-1] == '_')
    {
       l_str[l_len -1 ] = '\0';
    }
      
  }
  return(l_str);
}

/* ============================================================================
 * p_msg_win
 *   print a message both to stderr
 *   and to a gimp info window with OK button (only when run INTERACTIVE)
 * ============================================================================
 */

void
p_msg_win(GimpRunMode run_mode, char *msg)
{
  static t_but_arg  l_argv[1];
  int               l_argc;  
  
  l_argv[0].but_txt  = GTK_STOCK_OK;
  l_argv[0].but_val  = 0;
  l_argc             = 1;

  if(msg)
  {
    if(*msg)
    {
       fwrite(msg, 1, strlen(msg), stderr);
       fputc('\n', stderr);

       if(run_mode == GIMP_RUN_INTERACTIVE) p_buttons_dialog  (_("GAP Message"), msg, l_argc, l_argv, -1);
    }
  }
}    /* end  p_msg_win */

/* ============================================================================
 * p_file_exists
 *
 * return 0  ... file does not exist, or is not accessable by user,
 *               or is empty,
 *               or is not a regular file
 *        1  ... file exists
 * ============================================================================
 */
int
p_file_exists(char *fname)
{
  struct stat  l_stat_buf;
  long         l_len;

  /* File Laenge ermitteln */
  if (0 != stat(fname, &l_stat_buf))
  {
    /* stat error (file does not exist) */
    return(0);
  }
  
  if(!S_ISREG(l_stat_buf.st_mode))
  {
    return(0);
  }
  
  l_len = (long)l_stat_buf.st_size;
  if(l_len < 1)
  {
    /* File is empty*/
    return(0);
  }
  
  return(1);
}	/* end p_file_exists */

/* ============================================================================
 * p_image_file_copy
 *    (copy the imagefile and its thumbnail)
 * ============================================================================
 */
int
p_image_file_copy(char *fname, char *fname_copy)
{
   int            l_rc;

   l_rc = p_file_copy(fname, fname_copy);  
   p_gimp_file_copy_thumbnail(fname, fname_copy);
   return(l_rc);
}

/* ============================================================================
 * p_file_copy
 * ============================================================================
 */
int
p_file_copy(char *fname, char *fname_copy)
{
  FILE	      *l_fp;
  char                     *l_buffer;
  struct stat               l_stat_buf;
  long	       l_len;

  if(gap_debug) printf("p_file_copy src:%s dst:%s\n", fname, fname_copy);

  /* File Laenge ermitteln */
  if (0 != stat(fname, &l_stat_buf))
  {
    fprintf (stderr, "stat error on '%s'\n", fname);
    return -1;
  }
  l_len = (long)l_stat_buf.st_size;

  /* Buffer allocate */
  l_buffer=(char *)g_malloc0((size_t)l_len+1);
  if(l_buffer == NULL)
  {
    fprintf(stderr, "file_copy: calloc error (%ld Bytes not available)\n", l_len);
    return -1;
  }

  /* load File into Buffer */
  l_fp = fopen(fname, "rb");		    /* open read */
  if(l_fp == NULL)
  {
    fprintf (stderr, "open(read) error on '%s'\n", fname);
    g_free(l_buffer);
    return -1;
  }
  fread(l_buffer, 1, (size_t)l_len, l_fp);
  fclose(l_fp);
  
  l_fp = fopen(fname_copy, "wb");		    /* open write */
  if(l_fp == NULL)
  {
    fprintf (stderr, "file_copy: open(write) error on '%s' \n", fname_copy);
    g_free(l_buffer);
    return -1;
  }

  if(l_len > 0)
  {
     fwrite(l_buffer,  l_len, 1, l_fp);
  }

  fclose(l_fp);
  g_free(l_buffer);
  return 0;           /* all done OK */
}	/* end p_file_copy */


/* ============================================================================
 * p_delete_frame
 * ============================================================================
 */
int
p_delete_frame(t_anim_info *ainfo_ptr, long nr)
{
   char          *l_fname;
   int            l_rc;
   
   l_fname = p_alloc_fname(ainfo_ptr->basename, nr, ainfo_ptr->extension);
   if(l_fname == NULL) { return(1); }

   if(gap_debug) printf("\nDEBUG p_delete_frame: %s\n", l_fname);
   l_rc = remove(l_fname);
   
   p_gimp_file_delete_thumbnail(l_fname);
   
   g_free(l_fname);
   
   return(l_rc);
   
}    /* end p_delete_frame */

/* ============================================================================
 * p_rename_frame
 * ============================================================================
 */
int
p_rename_frame(t_anim_info *ainfo_ptr, long from_nr, long to_nr)
{
   char          *l_from_fname;
   char          *l_to_fname;
   int            l_rc;
   
   l_from_fname = p_alloc_fname(ainfo_ptr->basename, from_nr, ainfo_ptr->extension);
   if(l_from_fname == NULL) { return(1); }
   
   l_to_fname = p_alloc_fname(ainfo_ptr->basename, to_nr, ainfo_ptr->extension);
   if(l_to_fname == NULL) { g_free(l_from_fname); return(1); }
   
     
   if(gap_debug) printf("\nDEBUG p_rename_frame: %s ..to.. %s\n", l_from_fname, l_to_fname);
   l_rc = rename(l_from_fname, l_to_fname);

   p_gimp_file_rename_thumbnail(l_from_fname, l_to_fname);

   
   g_free(l_from_fname);
   g_free(l_to_fname);
   
   return(l_rc);
   
}    /* end p_rename_frame */


/* ============================================================================
 * p_alloc_basename
 *
 * build the basename from an images name
 * Extension and trailing digits ("0000.xcf") are cut off.
 * return name or NULL (if malloc fails)
 * Output: number contains the integer representation of the stripped off
 *         number String. (or 0 if there was none)
 * ============================================================================
 */
char *
p_alloc_basename(const char *imagename, long *number)
{
  char *l_fname;
  char *l_ptr;
  long  l_idx;

  *number = 0;
  if(imagename == NULL) return (NULL);

  /* copy from imagename */
  l_fname = g_strdup(imagename);

  if(gap_debug) printf("DEBUG p_alloc_basename  source: '%s'\n", l_fname);
  /* cut off extension */
  l_ptr = &l_fname[strlen(l_fname)];
  while(l_ptr != l_fname)
  {
    if((*l_ptr == G_DIR_SEPARATOR) || (*l_ptr == DIR_ROOT))  { break; }  /* dont run into dir part */
    if(*l_ptr == '.')                                        { *l_ptr = '\0'; break; }
    l_ptr--;
  }
  if(gap_debug) printf("DEBUG p_alloc_basename (ext_off): '%s'\n", l_fname);

  /* cut off trailing digits (0000) */
  l_ptr = &l_fname[strlen(l_fname)];
  if(l_ptr != l_fname) l_ptr--;
  l_idx = 1;
  while(l_ptr != l_fname)
  {
    if((*l_ptr >= '0') && (*l_ptr <= '9'))
    { 
      *number += ((*l_ptr - '0') * l_idx);
       l_idx = l_idx * 10;
      *l_ptr = '\0'; 
       l_ptr--; 
    }
    else
    {
      /* do not cut off underscore any longer */
      /*
       * if(*l_ptr == '_')
       * { 
       *    *l_ptr = '\0';
       * }
       */
       break;
    }
  }
  
  if(gap_debug) printf("DEBUG p_alloc_basename  result:'%s'\n", l_fname);

  return(l_fname);
   
}    /* end p_alloc_basename */



/* ============================================================================
 * p_alloc_extension
 *
 * input: a filename
 * returns: a copy of the filename extension (incl "." )
 *          if the filename has no extension, a pointer to
 *          an empty string is returned ("\0")
 *          NULL if allocate mem for extension failed.
 * ============================================================================
 */
char *
p_alloc_extension(char *imagename)
{
  int   l_exlen;
  char *l_ext;
  char *l_ptr;
  
  l_exlen = 0;
  l_ptr = &imagename[strlen(imagename)];
  while(l_ptr != imagename)
  {
    if((*l_ptr == G_DIR_SEPARATOR) || (*l_ptr == DIR_ROOT))  { break; }     /* dont run into dir part */
    if(*l_ptr == '.')                                        { l_exlen = strlen(l_ptr); break; }
    l_ptr--;
  }
  
  if(l_exlen > 0)
  {
    l_ext = g_strdup(l_ptr);
  }
  else
  {
    l_ext = g_strdup("\0");
  }
     
  return(l_ext);
}


/* ----------------------------------
 * p_alloc_fname_fixed_digits
 * ----------------------------------
 * build the framname by concatenating basename, nr and extension.
 * the Number part has leading zeroes, depending
 * on the number of digits specified.
 */
char*  
p_alloc_fname_fixed_digits(char *basename, long nr, char *extension, long digits)
{
  gchar *l_fname;
  gint   l_len;

  if(basename == NULL) return (NULL);
  l_len = (strlen(basename)  + strlen(extension) + 10);
  l_fname = (char *)g_malloc(l_len);

  switch(digits)
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
}    /* end p_alloc_fname_fixed_digits */


/* ============================================================================
 * p_alloc_fname
 * ============================================================================
 * at 1st call check environment
 * to findout how many digits (leading zeroes) to use in framename numbers
 * per default.
 */
char*  
p_alloc_fname(char *basename, long nr, char *extension)
{
  static long default_digits = -1;
  
  if (default_digits < 0)
  {
    const char   *l_env;

    default_digits = 6;

    l_env = g_getenv("GAP_FRAME_DIGITS");
    if(l_env != NULL)
    {
      default_digits = atol(l_env);
      default_digits = CLAMP(default_digits, 1 , 6);
    }
  }

  return (p_alloc_fname6(basename, nr, extension, default_digits));
}    /* end p_alloc_fname */

/* ----------------------------------
 * p_alloc_fname6
 * ----------------------------------
 * build the framname by concatenating basename, nr and extension.
 * the Number part has leading zeroes, depending
 * on filenames with the same basename and extension on disc.
 *
 * if no similar discfiles were found default_digits (with leading zeroes)
 * are used per default.
 *
 * if a similar discfilename is found, the number of digits/leading zeroes
 * is set equal to the discfile found.
 * example:
 *   basename == "frame_", nr == 5, ext == ".xcf"
 *   - discfile was found with name:  "frame_00001.xcf"
 *     return ("frame_00005.xcf");
 *
 *   - discfile was found with name:  "frame_001.xcf"
 *     return ("frame_005.xcf"); 
 *
 * return the resulting framename string
 *   (the calling program should g_free this string
 *    after use)
 */
char*  
p_alloc_fname6(char *basename, long nr, char *extension, long default_digits)
{
  gchar *l_fname;
  gint   l_digits_used;
  gint   l_len;
  long   l_nr_chk;

  if(basename == NULL) return (NULL);
  l_len = (strlen(basename)  + strlen(extension) + 10);
  l_fname = (char *)g_malloc(l_len);

    l_digits_used = default_digits;
    if(nr < 100000)
    {
       /* try to figure out if the frame numbers are in
        * 6-digit style, with leading zeroes  "frame_000001.xcf"
        * 4-digit style, with leading zeroes  "frame_0001.xcf"
        * or not                              "frame_1.xcf"
        */
       l_nr_chk = nr;

       while(l_nr_chk >= 0)
       {
         /* check if frame is on disk with 6-digit style framenumber */
         g_snprintf(l_fname, l_len, "%s%06ld%s", basename, l_nr_chk, extension);
         if (p_file_exists(l_fname))
         {
            l_digits_used = 6;
            break;
         }

         /* check if frame is on disk with 4-digit style framenumber */
         g_snprintf(l_fname, l_len, "%s%04ld%s", basename, l_nr_chk, extension);
         if (p_file_exists(l_fname))
         {
            l_digits_used = 4;
            break;
         }

         /* check if frame is on disk with 5-digit style framenumber */
         g_snprintf(l_fname, l_len, "%s%05ld%s", basename, l_nr_chk, extension);
         if (p_file_exists(l_fname))
         {
            l_digits_used = 5;
            break;
         }

         /* check if frame is on disk with 3-digit style framenumber */
         g_snprintf(l_fname, l_len, "%s%03ld%s", basename, l_nr_chk, extension);
         if (p_file_exists(l_fname))
         {
            l_digits_used = 3;
            break;
         }

         /* check if frame is on disk with 2-digit style framenumber */
         g_snprintf(l_fname, l_len, "%s%02ld%s", basename, l_nr_chk, extension);
         if (p_file_exists(l_fname))
         {
            l_digits_used = 2;
            break;
         }



         /* now check for filename without leading zeroes in the framenumber */
         g_snprintf(l_fname, l_len, "%s%ld%s", basename, l_nr_chk, extension);
         if (p_file_exists(l_fname))
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
}    /* end p_alloc_fname6 */


/* ----------------------------------
 * p_exists_frame_nr
 * ----------------------------------
 * check if frame with nr does exist
 * and find out how much digits are used for the number part
 */
gboolean  
p_exists_frame_nr(t_anim_info *ainfo_ptr, long nr, long *l_has_digits)
{
  gchar *l_fname;
  gint   l_digits_used;
  gint   l_len;
  long   l_nr_chk;
  gboolean l_exists;

  l_exists = FALSE;
  
  if(ainfo_ptr->basename == NULL) return (FALSE);
  l_len = (strlen(ainfo_ptr->basename)  + strlen(ainfo_ptr->extension) + 10);
  l_fname = (char *)g_malloc(l_len);

  l_digits_used = 6;
  l_nr_chk = nr;

  while(l_nr_chk >= 0)
  {
     /* check if frame is on disk with 6-digit style framenumber */
     g_snprintf(l_fname, l_len, "%s%06ld%s", ainfo_ptr->basename, l_nr_chk, ainfo_ptr->extension);
     if (p_file_exists(l_fname))
     {
        l_digits_used = 6;
        if(l_nr_chk == nr)
        {
          l_exists = TRUE;
        }
        break;
     }

     /* check if frame is on disk with 4-digit style framenumber */
     g_snprintf(l_fname, l_len, "%s%04ld%s", ainfo_ptr->basename, l_nr_chk, ainfo_ptr->extension);
     if (p_file_exists(l_fname))
     {
        l_digits_used = 4;
        if(l_nr_chk == nr)
        {
          l_exists = TRUE;
        }
        break;
     }

     /* check if frame is on disk with 5-digit style framenumber */
     g_snprintf(l_fname, l_len, "%s%05ld%s", ainfo_ptr->basename, l_nr_chk, ainfo_ptr->extension);
     if (p_file_exists(l_fname))
     {
        l_digits_used = 5;
        if(l_nr_chk == nr)
        {
          l_exists = TRUE;
        }
        break;
     }

     /* check if frame is on disk with 3-digit style framenumber */
     g_snprintf(l_fname, l_len, "%s%03ld%s", ainfo_ptr->basename, l_nr_chk, ainfo_ptr->extension);
     if (p_file_exists(l_fname))
     {
        l_digits_used = 3;
        if(l_nr_chk == nr)
        {
          l_exists = TRUE;
        }
        break;
     }

     /* check if frame is on disk with 2-digit style framenumber */
     g_snprintf(l_fname, l_len, "%s%02ld%s", ainfo_ptr->basename, l_nr_chk, ainfo_ptr->extension);
     if (p_file_exists(l_fname))
     {
        l_digits_used = 2;
        if(l_nr_chk == nr)
        {
          l_exists = TRUE;
        }
        break;
     }


     /* now check for filename without leading zeroes in the framenumber */
     g_snprintf(l_fname, l_len, "%s%ld%s", ainfo_ptr->basename, l_nr_chk, ainfo_ptr->extension);
     if (p_file_exists(l_fname))
     {
        l_digits_used = 1;
        if(l_nr_chk == nr)
        {
          l_exists = TRUE;
        }
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

  g_free(l_fname);

  *l_has_digits = l_digits_used;

  return(l_exists);
}    /* end p_exists_frame_nr */


/* ============================================================================
 * p_alloc_ainfo
 *
 * allocate and init an ainfo structure from the given image.
 * ============================================================================
 */
t_anim_info *
p_alloc_ainfo(gint32 image_id, GimpRunMode run_mode)
{
   t_anim_info   *l_ainfo_ptr;

   l_ainfo_ptr = (t_anim_info*)g_malloc(sizeof(t_anim_info));
   if(l_ainfo_ptr == NULL) return(NULL);
   
   l_ainfo_ptr->basename = NULL;
   l_ainfo_ptr->new_filename = NULL;
   l_ainfo_ptr->extension = NULL;
   l_ainfo_ptr->image_id = image_id;
 
   /* get current gimp images name  */   
   l_ainfo_ptr->old_filename = gimp_image_get_filename(image_id);
   if(l_ainfo_ptr->old_filename == NULL)
   {
     /* note: some gimp versions > 1.2  and < 1.3.x had default filenames for new created images
      * gimp-1.3.14 delivers NULL for unnamed images again
      */
     l_ainfo_ptr->old_filename = p_alloc_fname("frame_", 1, ".xcf");    /* assign a defaultname */
     gimp_image_set_filename (image_id, l_ainfo_ptr->old_filename);
   }

   l_ainfo_ptr->basename = p_alloc_basename(l_ainfo_ptr->old_filename, &l_ainfo_ptr->frame_nr);
   if(NULL == l_ainfo_ptr->basename)
   {
       p_free_ainfo(&l_ainfo_ptr);
       return(NULL);
   }

   l_ainfo_ptr->extension = p_alloc_extension(l_ainfo_ptr->old_filename);

   l_ainfo_ptr->curr_frame_nr = l_ainfo_ptr->frame_nr;
   l_ainfo_ptr->first_frame_nr = -1;
   l_ainfo_ptr->last_frame_nr = -1;
   l_ainfo_ptr->frame_cnt = 0;
   l_ainfo_ptr->run_mode = run_mode;


   return(l_ainfo_ptr);
 

}    /* end p_init_ainfo */

/* ============================================================================
 * p_dir_ainfo
 *
 * fill in more items into the given ainfo structure.
 * - first_frame_nr
 * - last_frame_nr
 * - frame_cnt
 *
 * to get this information, the directory entries have to be checked
 * ============================================================================
 */
int
p_dir_ainfo(t_anim_info *ainfo_ptr)
{
   char          *l_dirname;
   char          *l_dirname_ptr;
   char          *l_ptr;
   const char    *l_exptr;
   char          *l_dummy;
   GDir          *l_dirp;
   const gchar   *l_entry;
   long           l_nr;
   long           l_maxnr;
   long           l_minnr;
   short          l_dirflag;
   char           dirname_buff[1024];

   ainfo_ptr->frame_cnt = 0;
   l_dirp = NULL;
   l_minnr = 99999999;
   l_maxnr = 0;
   l_dirname = g_strdup(ainfo_ptr->basename);

   l_ptr = &l_dirname[strlen(l_dirname)];
   while(l_ptr != l_dirname)
   {
      if ((*l_ptr == G_DIR_SEPARATOR) || (*l_ptr == DIR_ROOT))
      { 
        *l_ptr = '\0';   /* split the string into dirpath 0 basename */
        l_ptr++;
        break;           /* stop at char behind the slash */
      }
      l_ptr--;
   }

   if(gap_debug) printf("DEBUG p_dir_ainfo: BASENAME:%s\n", l_ptr);
   
   if      (l_ptr == l_dirname)   { l_dirname_ptr = ".";  l_dirflag = 0; }
   else if (*l_dirname == '\0')   { l_dirname_ptr = G_DIR_SEPARATOR_S ; l_dirflag = 1; }
   else                           { l_dirname_ptr = l_dirname; l_dirflag = 2; }

   if(gap_debug) printf("DEBUG p_dir_ainfo: DIRNAME:%s\n", l_dirname_ptr);
   l_dirp = g_dir_open( l_dirname_ptr, 0, NULL );  
   
   if(!l_dirp) fprintf(stderr, "ERROR p_dir_ainfo: can't read directory %s\n", l_dirname_ptr);
   else
   {
     while ( (l_entry = g_dir_read_name( l_dirp )) != NULL )
     {

       /* if(gap_debug) printf("DEBUG p_dir_ainfo: l_entry:%s\n", l_entry); */
       
       
       /* findout extension of the directory entry name */
       l_exptr = &l_entry[strlen(l_entry)];
       while(l_exptr != l_entry)
       {
         if(*l_exptr == G_DIR_SEPARATOR) { break; }                 /* dont run into dir part */
         if(*l_exptr == '.')       { break; }
         l_exptr--;
       }
       /* l_exptr now points to the "." of the direntry (or to its begin if has no ext) */
       /* now check for equal extension */
       if((*l_exptr == '.') && (0 == strcmp(l_exptr, ainfo_ptr->extension)))
       {
         /* build full pathname (to check if file exists) */
         switch(l_dirflag)
         {
           case 0:
            g_snprintf(dirname_buff, sizeof(dirname_buff), "%s", l_entry);
            break;
           case 1:
            g_snprintf(dirname_buff, sizeof(dirname_buff), "%c%s",  G_DIR_SEPARATOR, l_entry);
            break;
           default:
            /* UNIX:  "/dir/file"
             * DOS:   "drv:\dir\file"
             */
            g_snprintf(dirname_buff, sizeof(dirname_buff), "%s%c%s", l_dirname_ptr,  G_DIR_SEPARATOR,  l_entry);
            break;
         }
         
         if(1 == p_file_exists(dirname_buff)) /* check for regular file */
         {
           /* get basename and frame number of the directory entry */
           l_dummy = p_alloc_basename(l_entry, &l_nr);
           if(l_dummy != NULL)
           { 
               /* check for files, with equal basename (frames)
                * (length must be greater than basepart+extension
                * because of the frame_nr part "0000")
                */
               if((0 == strcmp(l_ptr, l_dummy))
               && ( strlen(l_entry) > strlen(l_dummy) + strlen(l_exptr)  ))
               {
                 ainfo_ptr->frame_cnt++;


                 if(gap_debug) printf("DEBUG p_dir_ainfo:  %s NR=%ld\n", l_entry, l_nr);

                 if (l_nr > l_maxnr)
                    l_maxnr = l_nr;
                 if (l_nr < l_minnr)
                    l_minnr = l_nr;
               }

               g_free(l_dummy);
           }
         }
       }
     }
     g_dir_close( l_dirp );
   }

  g_free(l_dirname);

  /* set first_frame_nr and last_frame_nr (found as "_0099" in diskfile namepart) */
  ainfo_ptr->last_frame_nr = l_maxnr;
  ainfo_ptr->first_frame_nr = MIN(l_minnr, l_maxnr);

  return 0;           /* OK */
}	/* end p_dir_ainfo */
 

/* ============================================================================
 * p_free_ainfo
 *
 * free ainfo and its allocated stuff
 * ============================================================================
 */

void
p_free_ainfo(t_anim_info **ainfo)
{
  t_anim_info *aptr;
  
  if((aptr = *ainfo) == NULL)
     return;
  
  if(aptr->basename)
     g_free(aptr->basename);
  
  if(aptr->extension)
     g_free(aptr->extension);
   
  if(aptr->new_filename)
     g_free(aptr->new_filename);

  if(aptr->old_filename)
     g_free(aptr->old_filename);
     
  g_free(aptr);
}


/* ============================================================================
 * p_get_frame_nr
 * ============================================================================
 */
long
p_get_frame_nr_from_name(char *fname)
{
  long number;
  int  len;
  char *basename;
  if(fname == NULL) return(-1);
  
  basename = p_alloc_basename(fname, &number);
  if(basename == NULL) return(-1);
  
  len = strlen(basename);
  g_free(basename);
  
  if(number > 0)  return(number);

  if(fname[len]  == '0') return(number);
/*
 *   if(fname[len]  == '_')
 *   {
 *     if(fname[len+1]  == '0') return(TRUE);
 *   }
 */
  return(-1);
}

long
p_get_frame_nr(gint32 image_id)
{
  char *fname;
  long  number;
  
  number = -1;
  fname = gimp_image_get_filename(image_id);
  if(fname)
  {
     number = p_get_frame_nr_from_name(fname);
     g_free(fname);
  }
  return (number);
}

/* ============================================================================
 * p_chk_framechange
 *
 * check if the current frame nr has changed. 
 * useful after dialogs, because the user may have renamed the current image
 * (using save as)
 * while the gap-plugin's dialog was open.
 * return: 0 .. OK
 *        -1 .. Changed (or error occured)
 * ============================================================================
 */
int 
p_chk_framechange(t_anim_info *ainfo_ptr)
{
  int l_rc;
  t_anim_info *l_ainfo_ptr;
 
  l_rc = -1;
  l_ainfo_ptr = p_alloc_ainfo(ainfo_ptr->image_id, ainfo_ptr->run_mode);
  if(l_ainfo_ptr != NULL)
  {
    if(ainfo_ptr->curr_frame_nr == l_ainfo_ptr->curr_frame_nr )
    { 
       l_rc = 0;
    }
    else
    {
       p_msg_win(ainfo_ptr->run_mode,
         _("OPERATION CANCELLED.\n"
	   "Current frame changed while dialog was open."));
    }
    p_free_ainfo(&l_ainfo_ptr);
  }

  return l_rc;   
}	/* end p_chk_framechange */

/* ============================================================================
 * p_chk_framerange
 *
 * check ainfo struct if there is a framerange (of at least one frame)
 * return: 0 .. OK
 *        -1 .. No frames there (print error)
 * ============================================================================
 */
int 
p_chk_framerange(t_anim_info *ainfo_ptr)
{
  if(ainfo_ptr->frame_cnt == 0)
  {
     p_msg_win(ainfo_ptr->run_mode,
	       _("OPERATION CANCELLED.\n"
                 "GAP plug-ins only work with filenames\n"
                 "that end in numbers like _000001.xcf.\n"
                 "==> Rename your image, then try again."));
     return -1;
  }
  return 0;
}	/* end p_chk_framerange */

/* ============================================================================
 * p_gzip
 *   gzip or gunzip the file to a temporary file.
 *   zip == "zip"    compress
 *   zip == "unzip"  decompress
 *   return a pointer to the temporary created (by gzip) file.
 *          NULL  in case of errors
 * ============================================================================
 */
char * 
p_gzip (char *orig_name, char *new_name, char *zip)
{
  gchar*   l_cmd;
  gchar*   l_tmpname;
  gint     l_rc, l_rc2;

  if(zip == NULL) return NULL;
  
  l_cmd = NULL;
  l_tmpname = new_name;
  
  /* used gzip options:
   *     -c --stdout --to-stdout
   *          Write  output  on  standard  output
   *     -d --decompress --uncompress
   *          Decompress.
   *     -f --force
   *           Force compression or decompression even if the file
   */

  if(*zip == 'u')
  {
    l_cmd = g_strdup_printf("gzip -cfd <\"%s\"  >\"%s\"", orig_name, l_tmpname);
  }
  else
  {
    l_cmd = g_strdup_printf("gzip -cf  <\"%s\"  >\"%s\"", orig_name, l_tmpname);
  }

  if(gap_debug) printf("system: %s\n", l_cmd);
  
  l_rc = system(l_cmd);
  if(l_rc != 0)
  {
     /* Shift 8 Bits gets Retcode of the executed Program */
     l_rc2 = l_rc >> 8;
     fprintf(stderr, "ERROR system: %s\nreturncodes %d %d", l_cmd, l_rc, l_rc2);
     l_tmpname = NULL;
  }
  g_free(l_cmd);
  return l_tmpname;
  
}	/* end p_gzip */

/* ============================================================================
 * p_decide_save_as
 *   decide what to to, when attempt to save a frame in any image format 
 *  (other than xcf)
 *   Let the user decide if the frame is to save "as it is" or "flattened"
 *   ("as it is" will save only the backround layer in most fileformat types)
 *   The SAVE_AS_MODE is stored , and reused
 *   (without displaying the dialog) on subsequent calls.
 *
 *   return -1  ... CANCEL (do not save)
 *           0  ... save the image (may be flattened)
 * ============================================================================
 */
int
p_decide_save_as(gint32 image_id, char *sav_name)
{
  static char *l_msg;


  static char l_save_as_name[80];
  
  static t_but_arg  l_argv[3];
  int               l_argc;  
  int               l_save_as_mode;
  GimpRunMode      l_run_mode;  

  l_msg = _("You are using a file format != xcf\n"
	    "Save Operations may result\n"
	    "in loss of layer information.");
  /* check if there are SAVE_AS_MODE settings (from privious calls within one gimp session) */
  l_save_as_mode = -1;
  /* g_snprintf(l_save_as_name, sizeof(l_save_as_name), "plug_in_gap_plugins_SAVE_AS_MODE_%d", (int)image_id);*/
  g_snprintf(l_save_as_name, sizeof(l_save_as_name), "plug_in_gap_plugins_SAVE_AS_MODE");
  gimp_get_data (l_save_as_name, &l_save_as_mode);

  if(l_save_as_mode == -1)
  {
    /* no defined value found (this is the 1.st call for this image_id)
     * ask what to do with a 3 Button dialog
     */
    l_argv[0].but_txt  = GTK_STOCK_CANCEL;
    l_argv[0].but_val  = -1;
    l_argv[1].but_txt  = _("Save Flattened");
    l_argv[1].but_val  = 1;
    l_argv[2].but_txt  = _("Save As Is");
    l_argv[2].but_val  = 0;
    l_argc             = 3;

    l_save_as_mode =  p_buttons_dialog  (_("GAP Question"), l_msg, l_argc, l_argv, -1);
    
    if(gap_debug) printf("DEBUG: decide SAVE_AS_MODE %d\n", (int)l_save_as_mode);
    
    if(l_save_as_mode < 0) return -1;
    l_run_mode = GIMP_RUN_INTERACTIVE;
  }
  else
  {
    l_run_mode = GIMP_RUN_WITH_LAST_VALS;
  }

  gimp_set_data (l_save_as_name, &l_save_as_mode, sizeof(l_save_as_mode));
   
  
  if(l_save_as_mode == 1)
  {
      gimp_image_flatten (image_id);
  }

  return(p_save_named_image2(image_id
                           , sav_name
			   , l_run_mode
			   , FALSE      /* do not enable_thumbnailsave */
			   ));
}	/* end p_decide_save_as */

/* ============================================================================
 * p_gap_check_save_needed
 * ============================================================================
 */
gint32
p_gap_check_save_needed(gint32 image_id)
{
  const char *value_string;

  if (gimp_image_is_dirty(image_id))
  {
    if(gap_debug) printf("p_gap_check_save_needed: GAP need save, caused by dirty image id: %d\n", (int)image_id);
    return(TRUE);
  }
  else 
  {
    value_string = gimp_gimprc_query("trust-dirty-flag");
    if(value_string != NULL)
    {
      if(gap_debug) printf("p_gap_check_save_needed:GAP gimprc FOUND relevant LINE: trust-dirty-flag  %s\n", value_string);
      {
        if((*value_string != 'y') && (*value_string != 'Y'))
        {
           if(gap_debug) printf("p_gap_check_save_needed: GAP need save, forced by gimprc trust-dirty-flag %s\n", value_string);
           return(TRUE);
        }
      }
    }
  }

  if(gap_debug) printf("p_gap_check_save_needed: GAP can SKIP save\n");
  return(FALSE);
}  /* end p_gap_check_save_needed */


/* ============================================================================
 * p_save_named_image / 2
 * ============================================================================
 */
gint32
p_save_named_image2(gint32 image_id, char *sav_name, GimpRunMode run_mode, gboolean enable_thumbnailsave)
{
  GimpDrawable  *l_drawable;
  gint        l_nlayers;
  gint32     *l_layers_list;
  gboolean    l_rc;

  if(gap_debug) printf("DEBUG: before   p_save_named_image2: '%s'\n", sav_name);

  l_layers_list = gimp_image_get_layers(image_id, &l_nlayers);
  if(l_layers_list == NULL)
     return -1;

  l_drawable =  gimp_drawable_get(l_layers_list[l_nlayers -1]);  /* use the background layer */
  if(l_drawable == NULL)
  {
     fprintf(stderr, "ERROR: p_save_named_image2 gimp_drawable_get failed '%s' nlayers=%d\n",
                     sav_name, (int)l_nlayers);
     g_free (l_layers_list);
     return -1;
  }
  
  l_rc = gimp_file_save(run_mode, 
                 image_id,
		 l_drawable->drawable_id,
		 sav_name,
		 sav_name /* raw name ? */
		 );

  
  if(gap_debug) printf("DEBUG: after    p_save_named_image2: '%s' nlayers=%d image=%d drw=%d run_mode=%d\n", sav_name, (int)l_nlayers, (int)image_id, (int)l_drawable->drawable_id, (int)run_mode);

  if(enable_thumbnailsave)
  {
    p_cond_gimp_file_save_thumbnail(image_id, sav_name);
  }

  if(gap_debug) printf("DEBUG: after thumbmail save\n");

  g_free (l_layers_list);
  g_free (l_drawable);


  if (l_rc != TRUE)
  {
    fprintf(stderr, "ERROR: p_save_named_image2  gimp_file_save failed '%s'\n", sav_name);
    return -1;
  }
  return image_id;

}	/* end p_save_named_image2 */

gint32
p_save_named_image(gint32 image_id, char *sav_name, GimpRunMode run_mode)
{
  return(p_save_named_image2(image_id
                            , sav_name
			    , run_mode
			    , TRUE      /* enable_thumbnailsave */
			    ));
}


/* ============================================================================
 * p_save_named_frame
 *  save frame into temporary image,
 *  on success rename it to desired framename.
 *  (this is done, to avoid corrupted frames on disk in case of
 *   crash in one of the save procedures)
 * ============================================================================
 */
int
p_save_named_frame(gint32 image_id, char *sav_name)
{
  GimpParam *l_params;
  gchar     *l_ext;
  char      *l_tmpname;
  gint       l_retvals;
  int        l_gzip;
  int        l_xcf;
  int        l_rc;

  l_tmpname = NULL;
  l_rc   = -1;
  l_gzip = 0;          /* dont zip */
  l_xcf  = 0;          /* assume no xcf format */
  
  /* check extension to decide if savd file will be zipped */      
  l_ext = p_alloc_extension(sav_name);
  if(l_ext == NULL)  return -1;
  
  if(0 == strcmp(l_ext, ".xcf"))
  { 
    l_xcf  = 1;
  }
  else if(0 == strcmp(l_ext, ".xcfgz"))
  { 
    l_gzip = 1;          /* zip it */
    l_xcf  = 1;
  }
  else if(0 == strcmp(l_ext, ".gz"))
  { 
    l_gzip = 1;          /* zip it */
  }

  /* find a temp name 
   * that resides on the same filesystem as sav_name
   * and has the same extension as the original sav_name 
   */
  l_tmpname = g_strdup_printf("%s.gtmp%s", sav_name, l_ext);
  if(1 == p_file_exists(l_tmpname))
  {
      /* FILE exists: let gimp find another temp name */
      l_tmpname = gimp_temp_name(&l_ext[1]);
  }

  g_free(l_ext);


   if(gap_debug)
   {
     l_ext = (gchar *) g_getenv("GAP_NO_SAVE");
     if(l_ext != NULL)
     {
       fprintf(stderr, "DEBUG: GAP_NO_SAVE is set: save is skipped: '%s'\n", l_tmpname);
       g_free(l_tmpname);  /* free if it was a temporary name */
       return 0;
     }
   }

  if(gap_debug) printf("DEBUG: before   p_save_named_frame: '%s'\n", l_tmpname);

  if(l_xcf != 0)
  {
    /* save current frame as xcf image
     * xcf_save does operate on the complete image,
     * the drawable is ignored. (we can supply a dummy value)
     */
    l_params = gimp_run_procedure ("gimp_xcf_save",
			         &l_retvals,
			         GIMP_PDB_INT32,    GIMP_RUN_NONINTERACTIVE,
			         GIMP_PDB_IMAGE,    image_id,
			         GIMP_PDB_DRAWABLE, 0,
			         GIMP_PDB_STRING, l_tmpname,
			         GIMP_PDB_STRING, l_tmpname, /* raw name ? */
			         GIMP_PDB_END);
    if(gap_debug) printf("DEBUG: after   xcf  p_save_named_frame: '%s'\n", l_tmpname);

    if (l_params[0].data.d_status == GIMP_PDB_SUCCESS)
    {
       l_rc = image_id;
    }
    g_free(l_params);
  }
  else
  {
     /* let gimp try to save (and detect filetype by extension)
      * Note: the most imagefileformats do not support multilayer
      *       images, and extra channels
      *       the result may not contain the whole imagedata
      *
      * To Do: Should warn the user at 1.st attempt to do this.
      */
      
     l_rc = p_decide_save_as(image_id, l_tmpname);
  } 

  if(l_rc < 0)
  {
     remove(l_tmpname);
     g_free(l_tmpname);  /* free temporary name */
     return l_rc;
  }

  if(l_gzip == 0)
  {
     /* remove sav_name, then rename tmpname ==> sav_name */
     remove(sav_name);
     if (0 != rename(l_tmpname, sav_name))
     {
        /* if tempname is located on another filesystem (errno == EXDEV)
         * rename will not work.
         * so lets try a  copy ; remove sequence
         */
         if(gap_debug) printf("DEBUG: p_save_named_frame: RENAME 2nd try\n");
         if(0 == p_file_copy(l_tmpname, sav_name))
	 {
	    remove(l_tmpname);
	 }
         else
         {
            fprintf(stderr, "ERROR in p_save_named_frame: can't rename %s to %s\n",
                            l_tmpname, sav_name);
            return -1;
         }
     }
  }
  else
  {
    /* ZIP tmpname ==> sav_name */
    if(NULL != p_gzip(l_tmpname, sav_name, "zip"))
    {
       /* OK zip created compressed file named sav_name
        * now delete the uncompressed l_tempname
        */
       remove(l_tmpname);
    }
  }

  p_cond_gimp_file_save_thumbnail(image_id, sav_name);

  g_free(l_tmpname);  /* free temporary name */

  return l_rc;
   
}	/* end p_save_named_frame */


/* ============================================================================
 * p_save_old_frame
 * ============================================================================
 */
int
p_save_old_frame(t_anim_info *ainfo_ptr, t_video_info *vin_ptr)
{
  /* SAVE of old image image if it has unsaved changes
   * (or if Unconditional frame save is forced by gimprc setting)
   */ 
  if(p_gap_check_save_needed(ainfo_ptr->image_id))
  {
    /* check and peroform automatic onionskinlayer remove */
    if(vin_ptr)
    {
      if((vin_ptr->auto_delete_before_save) && (vin_ptr->onionskin_auto_enable))
      {
        p_onionskin_delete(ainfo_ptr->image_id);
      }
    }
    return (p_save_named_frame(ainfo_ptr->image_id, ainfo_ptr->old_filename));
  }
  return 0;
}	/* end p_save_old_frame */



/* ============================================================================
 * p_load_image
 * load image of any type by filename, and return its image id
 * (or -1 in case of errors)
 * ============================================================================
 */
gint32
p_load_image (char *lod_name)
{
  char  *l_ext;
  char  *l_tmpname;
  gint32 l_tmp_image_id;
  int    l_rc;
  
  l_rc = 0;
  l_tmpname = NULL;
  l_ext = p_alloc_extension(lod_name);
  if(l_ext != NULL)
  {
    if((0 == strcmp(l_ext, ".xcfgz"))
    || (0 == strcmp(l_ext, ".gz")))
    { 

      /* find a temp name and */
      /* try to unzip file, before loading it */
      l_tmpname = p_gzip(lod_name, gimp_temp_name(&l_ext[1]), "unzip");
    }
    else l_tmpname = lod_name;
    g_free(l_ext);
  }

  if(l_tmpname == NULL)
  {
    return -1;
  }


  if(gap_debug) printf("DEBUG: before   p_load_image: '%s'\n", l_tmpname);

  l_tmp_image_id = gimp_file_load(GIMP_RUN_NONINTERACTIVE,
		l_tmpname,
		l_tmpname  /* raw name ? */
		);
  
  if(gap_debug) printf("DEBUG: after    p_load_image: '%s' id=%d\n\n",
                               l_tmpname, (int)l_tmp_image_id);

  if(l_tmpname != lod_name)
  {
    remove(l_tmpname);
    g_free(l_tmpname);  /* free if it was a temporary name */
  }
  

  return l_tmp_image_id;

}	/* end p_load_image */



/* ============================================================================
 * p_load_named_frame
 *   load new frame,
 *   reconnect displays from old existing image to the new loaded frame
 *   and delete the old image.
 *  returns: new_image_id (or -1 on errors)
 * ============================================================================
 */
gint32 
p_load_named_frame (gint32 old_image_id, char *lod_name)
{
  gint32 l_new_image_id;

  l_new_image_id = p_load_image(lod_name);
  
  if(gap_debug) printf("DEBUG: after    p_load_named_frame: '%s' old_id=%d  new_id=%d\n\n",
                               lod_name, (int)old_image_id, (int)l_new_image_id);

  if(l_new_image_id < 0)
      return -1;

  if (p_gimp_displays_reconnect(old_image_id, l_new_image_id))
  {
      /* delete the old image */
      gimp_image_delete(old_image_id);

      /* use the original lod_name */
      gimp_image_set_filename (l_new_image_id, lod_name);

      /* dont consider image dirty after load */
      gimp_image_clean_all(l_new_image_id);

      /* Update the active_image table for the Navigator Dialog
       * (TODO: replace the active-image table by real Event handling)
       */
      p_update_active_image(old_image_id, l_new_image_id);


      return l_new_image_id;
   }

   printf("GAP: Error: PDB call of gimp_displays_reconnect failed\n");
   return (-1);
}	/* end p_load_named_frame */



/* ============================================================================
 * p_replace_image
 *
 * make new_filename of next file to load, check if that file does exist on disk
 * then save current image and replace it, by loading the new_filename
 *
 * return image_id (of the new loaded frame) on success
 *        or -1 on errors
 * ============================================================================
 */
gint32
p_replace_image(t_anim_info *ainfo_ptr)
{
  gint32 image_id;
  t_video_info *vin_ptr;
  gboolean  do_onionskin_crate;
  
  do_onionskin_crate  = FALSE;
  if(ainfo_ptr->new_filename != NULL) g_free(ainfo_ptr->new_filename);
  ainfo_ptr->new_filename = p_alloc_fname(ainfo_ptr->basename,
                                      ainfo_ptr->frame_nr,
                                      ainfo_ptr->extension);
  if(ainfo_ptr->new_filename == NULL)
     return -1;

  if(0 == p_file_exists(ainfo_ptr->new_filename ))
     return -1;

  vin_ptr = p_get_video_info(ainfo_ptr->basename);
  if(p_save_old_frame(ainfo_ptr, vin_ptr) < 0)
  {
    if(vin_ptr)
    {
      g_free(vin_ptr);
    }
    return -1;
  }
 
  if((vin_ptr->auto_replace_after_load) && (vin_ptr->onionskin_auto_enable))
  {
    do_onionskin_crate  = TRUE;
    
    /* check if directoryscan is needed */
    if((ainfo_ptr->first_frame_nr < 0)
    || (ainfo_ptr->last_frame_nr < 0))
    {
       /* perform directoryscan to findout first_frame_nr and last_frame_nr
        * that is needed for onionskin creation
        */
       p_dir_ainfo(ainfo_ptr);
    }
  }
 
  image_id = p_load_named_frame(ainfo_ptr->image_id, ainfo_ptr->new_filename);

  /* check and peroform automatic onionskinlayer creation */
  if(vin_ptr)
  {
    if(do_onionskin_crate)
    {
       /* create onionskinlayers without keeping the handled images cached
        * (passing NULL pointers for the chaching structures and functions)
        */
       p_onionskin_apply(NULL         /* dummy pointer gpp */
             , image_id               /* apply on the newly loaded image_id */
             , vin_ptr
             , ainfo_ptr->frame_nr        /* the new current frame_nr */
             , ainfo_ptr->first_frame_nr
             , ainfo_ptr->last_frame_nr
             , ainfo_ptr->basename
             , ainfo_ptr->extension
             , NULL                    /* fptr_add_img_to_cache */
             , NULL                    /* fptr_find_frame_in_img_cache */
             , FALSE                   /* use_cache */
             );
    }
    g_free(vin_ptr);
  }

  return(image_id);
}	/* end p_replace_image */


/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */ 
/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */ 
/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */ 

/* ============================================================================
 * gap_video_paste Buffer procedures
 * ============================================================================
 */

gchar *
p_get_video_paste_basename(void)
{
  gchar *l_basename;
  
  l_basename = gimp_gimprc_query("video-paste-basename");
  if(l_basename == NULL)
  {
     l_basename = g_strdup("gap_video_pastebuffer_");
  }
  return(l_basename);
}

gchar *
p_get_video_paste_dir(void)
{
  gchar *l_dir;
  gint   l_len;
  
  l_dir = gimp_gimprc_query("video-paste-dir");
  if(l_dir == NULL)
  {
     l_dir = g_strdup("/tmp");
  }
  
  /* if dir is configured with trailing dir seprator slash
   * then cut it off
   */
  l_len = strlen(l_dir);
  if((l_dir[l_len -1] == G_DIR_SEPARATOR) && (l_len > 1))
  {
     l_dir[l_len -1] = '\0';
  }
  return(l_dir);
}

gchar *
p_get_video_paste_name(void)
{
  gchar *l_dir;
  gchar *l_basename;
  gchar *l_video_name;
  gchar *l_dir_thumb;
         
  l_dir = p_get_video_paste_dir();
  l_basename = p_get_video_paste_basename();
  l_video_name = g_strdup_printf("%s%s%s", l_dir, G_DIR_SEPARATOR_S, l_basename);
  l_dir_thumb = g_strdup_printf("%s%s%s", l_dir, G_DIR_SEPARATOR_S, ".xvpics");

  mkdir (l_dir_thumb, 0755);

  g_free(l_dir);
  g_free(l_basename);
  g_free(l_dir_thumb);

  if(gap_debug) printf("p_get_video_paste_name: %s\n", l_video_name);
  return(l_video_name); 
}

static gint32
p_clear_or_count_video_paste(gint delete_flag)
{
  gchar *l_dir;
  gchar *l_basename;
  gchar *l_filename;
  gint   l_len;
  gint32 l_framecount;
  GDir          *l_dirp;
  const char    *l_entry;

  l_dir = p_get_video_paste_dir();
  l_dirp = g_dir_open(l_dir, 0, NULL);  
  l_framecount = 0;
  
  if(!l_dirp)
  {
    printf("ERROR p_vid_edit_clear: can't read directory %s\n", l_dir);
    l_framecount = -1;
  }
  else
  {
     l_basename = p_get_video_paste_basename();
     
     l_len = strlen(l_basename);
     while ( (l_entry = g_dir_read_name( l_dirp )) != NULL )
     {
       if(strncmp(l_basename, l_entry, l_len) == 0)
       {
          l_filename = g_strdup_printf("%s%s%s", l_dir, G_DIR_SEPARATOR_S, l_entry);
          if(1 == p_file_exists(l_filename)) /* check for regular file */
          {
             /* delete all files in the video paste directory
              * with names matching the basename part
              */
             l_framecount++;
             if(delete_flag)
             {
                if(gap_debug) printf("p_vid_edit_clear: remove file %s\n", l_filename);
                remove(l_filename);

                /* also delete thumbnail */
                p_gimp_file_delete_thumbnail(l_filename);
             }
          }
          g_free(l_filename);
       }
     }
     g_dir_close( l_dirp );
     g_free(l_basename);
   }
   g_free(l_dir);
   return(l_framecount);
}

gint32
p_vid_edit_clear(void)
{
  return(p_clear_or_count_video_paste(TRUE)); /* delete frames */
}

gint32
p_vid_edit_framecount()
{
  return (p_clear_or_count_video_paste(FALSE)); /* delete_flag is off, just count frames */
}


/* ============================================================================
 * gap_vid_edit_copy
 * ============================================================================
 * copy frames to the vid paste directory 
 * (always using 6 digit framenumbers in the vid paste dir)
 */
gint
gap_vid_edit_copy(GimpRunMode run_mode, gint32 image_id, long range_from, long range_to)
{
  int rc;
  t_anim_info *ainfo_ptr;
  
  gchar *l_curr_name;
  gchar *l_fname ;
  gchar *l_fname_copy;
  gchar *l_basename;
  gint32 l_frame_nr;
  gint32 l_cnt_range;
  gint32 l_cnt2;
  gint32 l_idx;
  gint32 l_tmp_image_id;

  ainfo_ptr = p_alloc_ainfo(image_id, run_mode);
  if(ainfo_ptr == NULL)
  {
     return (-1);
  }
  rc = 0;

  if((ainfo_ptr->curr_frame_nr >= MIN(range_to, range_from))
  && (ainfo_ptr->curr_frame_nr <= MAX(range_to, range_from)))
  {
    /* current frame is in the affected range
     * so we have to save current frame to file
     */   
    l_curr_name = p_alloc_fname(ainfo_ptr->basename, ainfo_ptr->curr_frame_nr, ainfo_ptr->extension);
    p_save_named_frame(ainfo_ptr->image_id, l_curr_name);
    g_free(l_curr_name);
  }
  
  l_basename = p_get_video_paste_name();
  l_cnt2 = p_vid_edit_framecount();  /* count frames in the video paste buffer */
  l_frame_nr = 1 + l_cnt2;           /* start at one, or continue (append) at end +1 */
  
  l_cnt_range = 1 + MAX(range_to, range_from) - MIN(range_to, range_from);
  for(l_idx = 0; l_idx < l_cnt_range;  l_idx++)
  {
     if(rc < 0)
     {
       break;
     }
     l_fname = p_alloc_fname(ainfo_ptr->basename,
                             MIN(range_to, range_from) + l_idx,
                             ainfo_ptr->extension);
     l_fname_copy = g_strdup_printf("%s%06ld.xcf", l_basename, (long)l_frame_nr);
     
     if(strcmp(ainfo_ptr->extension, ".xcf") == 0)
     {
        rc = p_image_file_copy(l_fname, l_fname_copy);
     }
     else
     {
        /* convert other fileformats to xcf before saving to video paste buffer */
        l_tmp_image_id = p_load_image(l_fname);
        rc = p_save_named_frame(l_tmp_image_id, l_fname_copy);
        gimp_image_delete(l_tmp_image_id);
     }
     g_free(l_fname);
     g_free(l_fname_copy);
     l_frame_nr++;
  }
  p_free_ainfo(&ainfo_ptr);
  return(rc);
}       /* end gap_vid_edit_copy */

/* ============================================================================
 * p_custom_palette_file
 *   write a gimp palette file
 * ============================================================================
 */

static gint
p_custom_palette_file(char *filename, guchar *rgb, gint count)
{
  FILE *l_fp;

  l_fp= fopen(filename, "w");
  if (l_fp == NULL)
  {
    return -1;
  }
  
  fprintf(l_fp, "GIMP Palette\n");
  fprintf(l_fp, "# this file will be overwritten each time when video frames are converted to INDEXED\n");

  while (count > 0)
  {
    fprintf(l_fp, "%d %d %d\tUnknown\n", rgb[0], rgb[1], rgb[2]);
    rgb+= 3;
    --count;
  }


  fclose (l_fp);
  return 0;
}       /* end p_custom_palette_file */


/* ============================================================================
 * gap_vid_edit_paste
 *
 * return image_id (of the new loaded current frame) on success
 *        or -1 on errors
 * ============================================================================
 * copy frames from the vid paste directory 
 * (always using 6 digit framenumbers in the vid paste dir)
 */
gint32
gap_vid_edit_paste(GimpRunMode run_mode, gint32 image_id, long paste_mode)
{
#define CUSTOM_PALETTE_NAME "gap_cmap"
  gint32 rc;
  t_anim_info *ainfo_ptr;
  
  gchar *l_curr_name;
  gchar *l_fname ;
  gchar *l_fname_copy;
  gchar *l_basename;
  gint32 l_frame_nr;
  gint32 l_dst_frame_nr;
  gint32 l_cnt2;
  gint32 l_lo, l_hi;
  gint32 l_insert_frame_nr;
  gint32 l_tmp_image_id;
  gint       l_rc;
  GimpImageBaseType  l_orig_basetype;

  l_cnt2 = p_vid_edit_framecount();
  if(gap_debug)
  {
    printf("gap_vid_edit_paste: paste_mode %d found %d frames to paste, image_id: %d\n"
           , (int)paste_mode
           , (int)l_cnt2
           , (int)image_id
           );
  }
  if (l_cnt2 < 1)
  {
    return(0);  /* video paste buffer is empty */
  }


  ainfo_ptr = p_alloc_ainfo(image_id, run_mode);
  if(ainfo_ptr == NULL)
  {
     return (-1);
  }
  if (0 != p_dir_ainfo(ainfo_ptr))
  {
     return (-1);
  }

  if(gap_debug)
  {
    printf("gap_vid_edit_paste: ainfo: basename: %s, extension:%s  curr:%d first:%d last: %d\n"
           , ainfo_ptr->basename
           , ainfo_ptr->extension
           , (int) ainfo_ptr->curr_frame_nr
           , (int) ainfo_ptr->first_frame_nr
           , (int) ainfo_ptr->last_frame_nr
           );
  }

  rc = 0;

  l_insert_frame_nr = ainfo_ptr->curr_frame_nr;

  if(paste_mode != VID_PASTE_INSERT_AFTER)
  {
    /* we have to save current frame to file */   
    l_curr_name = p_alloc_fname(ainfo_ptr->basename, ainfo_ptr->curr_frame_nr, ainfo_ptr->extension);
    p_save_named_frame(ainfo_ptr->image_id, l_curr_name);
    g_free(l_curr_name);
  }
  
  if(paste_mode != VID_PASTE_REPLACE)
  {
     if(paste_mode == VID_PASTE_INSERT_AFTER)
     {
       l_insert_frame_nr = ainfo_ptr->curr_frame_nr +1;
     }
    
     /* rename (renumber) all frames with number greater (or greater equal)  than current
      */
     l_lo   = ainfo_ptr->last_frame_nr;
     l_hi   = l_lo + l_cnt2;

     if(gap_debug)
     {
       printf("gap_vid_edit_paste: l_insert_frame_nr %d l_lo:%d l_hi:%d\n"
           , (int)l_insert_frame_nr, (int)l_lo, (int)l_hi);
     }
     
     while(l_lo >= l_insert_frame_nr)
     {     
       if(0 != p_rename_frame(ainfo_ptr, l_lo, l_hi))
       {
          gchar *tmp_errtxt;
          tmp_errtxt = g_strdup_printf(_("Error: could not rename frame %ld to %ld"), (long int)l_lo, (long int)l_hi);
          p_msg_win(ainfo_ptr->run_mode, tmp_errtxt);
          g_free(tmp_errtxt);
          return -1;
       }
       l_lo--;
       l_hi--;
     }
  }

  l_basename = p_get_video_paste_name();
  l_dst_frame_nr = l_insert_frame_nr;
  for(l_frame_nr = 1; l_frame_nr <= l_cnt2; l_frame_nr++)
  {
     l_fname = p_alloc_fname(ainfo_ptr->basename,
                             l_dst_frame_nr,
                             ainfo_ptr->extension);
     l_fname_copy = g_strdup_printf("%s%06ld.xcf", l_basename, (long)l_frame_nr);

     l_tmp_image_id = p_load_image(l_fname_copy);
     
     /* delete onionskin layers (if there are any) before paste */
     p_onionskin_delete(l_tmp_image_id);
     
     /* check size and resize if needed */
     if((gimp_image_width(l_tmp_image_id) != gimp_image_width(image_id))
     || (gimp_image_height(l_tmp_image_id) != gimp_image_height(image_id)))
     {
         gint32      l_size_x, l_size_y;

         l_size_x = gimp_image_width(image_id);
         l_size_y = gimp_image_height(image_id);
         if(gap_debug) printf("DEBUG: scale to size %d %d\n", (int)l_size_x, (int)l_size_y);
 
         gimp_image_scale(l_tmp_image_id, l_size_x, l_size_y);
     }
     
     /* check basetype and convert if needed */
     l_orig_basetype = gimp_image_base_type(image_id);
     if(gimp_image_base_type(l_tmp_image_id) != l_orig_basetype)
     {
       switch(l_orig_basetype)
       {
           gchar      *l_palette_filename;
           guchar     *l_cmap;
           gint        l_ncolors;
 
           /* convert tmp image to dest type */
           case GIMP_INDEXED:
             l_cmap = gimp_image_get_cmap(image_id, &l_ncolors);
             if(gap_debug) printf("DEBUG: convert to INDEXED %d colors\n", (int)l_ncolors);

             l_palette_filename = g_strdup_printf("%s%spalettes%s%s"
                                                 , gimp_directory()
                                                 , G_DIR_SEPARATOR_S
                                                 , G_DIR_SEPARATOR_S
                                                 , CUSTOM_PALETTE_NAME);
 
             l_rc = p_custom_palette_file(l_palette_filename, l_cmap, l_ncolors);
             if(l_rc == 0)
             {
               gimp_palettes_refresh();
               gimp_convert_indexed(l_tmp_image_id,
                                    GIMP_FS_DITHER,           /* dither floyd-steinberg */
                                    GIMP_CUSTOM_PALETTE,
                                    l_ncolors,                /* number of colors */
                                    FALSE,                    /* No alpha_dither */
                                    FALSE,                    /* dont remove_unused */
                                    CUSTOM_PALETTE_NAME
                                    );
             }
             else
             {
               printf("ERROR: gap_vid_edit_paste: could not save custom palette %s\n", l_palette_filename);
             }
             g_free(l_cmap);
             g_free(l_palette_filename);
             break;
           case GIMP_GRAY:
             if(gap_debug) printf("DEBUG: convert to GRAY'\n");
             gimp_convert_grayscale(l_tmp_image_id);
             break;
           case GIMP_RGB:
             if(gap_debug) printf("DEBUG: convert to RGB'\n");
             gimp_convert_rgb(l_tmp_image_id);
             break;
           default:
             printf( "DEBUG: unknown image type\n");
             return -1;
             break;
        }
     }

     if(gap_debug) printf("DEBUG: before p_save_named_frame l_fname:%s l_tmp_image_id:%d'\n"
                      , l_fname
                      , (int) l_tmp_image_id);

     gimp_image_set_filename (l_tmp_image_id, l_fname);
     rc = p_save_named_frame(l_tmp_image_id, l_fname);

     gimp_image_delete(l_tmp_image_id);

     g_free(l_fname);
     g_free(l_fname_copy);

     l_dst_frame_nr++;
  }
  
  if(paste_mode == VID_PASTE_INSERT_AFTER)
  {
    /* we pasted successful after the current image,
     * keep the calling image_id as active image_id return value
     */
    if(rc >= 0)
    {
      rc = image_id;
    }
  }
  else
  {
    if(rc >= 0)
    {
      /* load from the "new" current frame */   
      if(ainfo_ptr->new_filename != NULL) g_free(ainfo_ptr->new_filename);

      ainfo_ptr->new_filename = p_alloc_fname(ainfo_ptr->basename,
                                      ainfo_ptr->curr_frame_nr,
                                      ainfo_ptr->extension);

      if(gap_debug)  printf("gap_vid_edit_paste: before load: %s basename: %s, extension:%s  curr:%d first:%d last: %d\n"
           , ainfo_ptr->new_filename
           , ainfo_ptr->basename
           , ainfo_ptr->extension
           , (int) ainfo_ptr->curr_frame_nr
           , (int) ainfo_ptr->first_frame_nr
           , (int) ainfo_ptr->last_frame_nr
           );

      rc = p_load_named_frame(ainfo_ptr->image_id, ainfo_ptr->new_filename);
    }
  }

  if(rc < 0)
  {
      rc = -1;
  }

  if(gap_debug)  printf("gap_vid_edit_paste: rc: %d\n", (int)rc);

  p_free_ainfo(&ainfo_ptr);
  
  return(rc);
}       /* end gap_vid_edit_paste */
