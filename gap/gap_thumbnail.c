/* gap_thumbnail.c
 * 2003.05.27 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * basic GAP thumbnail handling utility procedures
 *
 * TODO:
 * when the feature request #113033 is implemented in the gimp core
 * this gap Thumbnail handling should be updated
 * to act as wrapper.
 * (replace GAP private implementations and call 
 *  the corresponding GIMP_core procedures where available)
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
 * 1.3.14a  2003/05/27   hof: created
 */

#include "config.h"
 
/* SYSTEM (UNIX) includes */ 
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#ifdef HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#ifdef G_OS_WIN32
#include <io.h>
#  ifndef S_ISDIR
#    define S_ISDIR(m) ((m) & _S_IFDIR)
#  endif
#  ifndef S_ISREG
#    define S_ISREG(m) ((m) & _S_IFREG)
#  endif
#endif

/* GIMP includes */
#include "gtk/gtk.h"
#include "gap-intl.h"
#include "libgimp/gimp.h"


/* GAP includes */
#include "gap_lib.h"
#include "gap_pdb_calls.h"
#include "gap_thumbnail.h"


extern      int gap_debug; /* ==0  ... dont print debug infos */



#define TAG_DESCRIPTION        "tEXt::Description"
#define TAG_SOFTWARE           "tEXt::Software"
#define TAG_THUMB_URI          "tEXt::Thumb::URI"
#define TAG_THUMB_MTIME        "tEXt::Thumb::MTime"
#define TAG_THUMB_SIZE         "tEXt::Thumb::Size"
#define TAG_THUMB_IMAGE_WIDTH  "tEXt::Thumb::Image::Width"
#define TAG_THUMB_IMAGE_HEIGHT "tEXt::Thumb::Image::Height"
#define TAG_THUMB_GIMP_TYPE    "tEXt::Thumb::X-GIMP::Type"
#define TAG_THUMB_GIMP_LAYERS  "tEXt::Thumb::X-GIMP::Layers"

#define THUMB_SIZE_FAIL -1

typedef struct
{
  const gchar *dirname;
  gint         size;
} ThumbnailSize;

static const ThumbnailSize thumb_sizes[] =
{
  { "fail",   THUMB_SIZE_FAIL },
  { "normal", 128             },
  { "large",  256             }
};

static gchar *thumb_dir                                 = NULL;
static gchar *thumb_subdirs[G_N_ELEMENTS (thumb_sizes)] = { 0 };
static gchar *thumb_fail_subdir                         = NULL;

static char  *global_thumbnail_mode = NULL;  /* NULL or pointer to "none", "normal", "large" */

static void            p_gap_init_thumb_dirs(void);
static gchar *         p_gap_filename_to_uri(const char *filename);
static const gchar *   p_gimp_imagefile_png_thumb_name (const gchar *uri);
static gchar *         p_gap_filename_to_png_thumb_name (const gchar *filename);

static void            p_copy_png_thumb(char *filename_src, char *filename_dst);

static char *          p_alloc_xvpics_thumbname(char *name);



/* --------------------------------
 * p_gap_init_thumb_dirs
 * --------------------------------
 */
static void
p_gap_init_thumb_dirs(void)
{
  gint ii;

  thumb_dir = g_build_filename (g_get_home_dir(), ".thumbnails", NULL);

  for (ii = 0; ii < G_N_ELEMENTS (thumb_sizes); ii++)
  {
    thumb_subdirs[ii] = g_build_filename (g_get_home_dir(), ".thumbnails",
                                         thumb_sizes[ii].dirname, NULL);
  }

  thumb_fail_subdir = thumb_subdirs[0];
  thumb_subdirs[0] = g_strdup_printf ("%s%cgimp-%d.%d",
                                      thumb_fail_subdir, G_DIR_SEPARATOR,
                                      GIMP_MAJOR_VERSION, GIMP_MINOR_VERSION);

}  /* end p_gap_init_thumb_dirs */



/* --------------------------------
 * p_gap_filename_to_uri
 * --------------------------------
 */
static gchar*
p_gap_filename_to_uri(const char *filename)
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

  uri = g_filename_to_uri (filename, NULL, NULL);

  g_free (absolute);

  return uri;
}  /* end p_gap_filename_to_uri */


/* --------------------------------
 * p_gimp_imagefile_png_thumb_name
 * --------------------------------
 */
static const gchar *
p_gimp_imagefile_png_thumb_name (const gchar *uri)
{
  static gchar name[40];
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

  strncpy (name + 32, ".png", 5);

  return (const gchar *) name;
}  /* end p_gimp_imagefile_png_thumb_name */


/* --------------------------------
 * p_gap_filename_to_png_thumb_name
 * --------------------------------
 */
static gchar *
p_gap_filename_to_png_thumb_name (const gchar *filename)
{
   gchar *uri;
   const gchar *png_thumb_name;
   
   uri = p_gap_filename_to_uri(filename);
   png_thumb_name = p_gimp_imagefile_png_thumb_name(uri);
   
   g_free(uri);

   if(gap_debug) printf("png_thumb_name: %s\n", png_thumb_name);
   return (g_strdup(png_thumb_name));
}  /* end p_gap_filename_to_png_thumb_name */


/* -------------------------------
 * p_gimprc_query_thumbnailsave
 * -------------------------------
 * return the gimprc value_string
 * responsible for video thumbnail configuration
 * the caller should g_free the retuned string
 *
 * checking gimprc both for "thumbnail-size" (GIMP native, values:  "none", "normal", "large"
 * and                  for "video-thumbnails" (GAP values:  "none", "png")
 *
 */
char *   
p_gimprc_query_thumbnailsave(void)
{
  if(global_thumbnail_mode)
  {
    g_free(global_thumbnail_mode);
  }

  global_thumbnail_mode = gimp_gimprc_query("video-thumbnails");
  
  if(global_thumbnail_mode == NULL)
  {
     global_thumbnail_mode = gimp_gimprc_query("thumbnail-size");
  }
  return g_strdup(global_thumbnail_mode);
}  /* end p_gimprc_query_thumbnailsave */


/* -------------------------------
 * p_thumbnailsave_is_on
 * -------------------------------
 * checking gimprc if thumnail saving is enabled.
 * both keywords "video-thumbnails" value:  "none",
 * and           "thumbnail-size" values:  "none"
 * are checked.
 */
gboolean   
p_thumbnailsave_is_on(void)
{
  if(global_thumbnail_mode == NULL)
  {
    global_thumbnail_mode = gimp_gimprc_query("video-thumbnails");
    if(global_thumbnail_mode == NULL)
    {
       global_thumbnail_mode = gimp_gimprc_query("thumbnail-size");
    }
  }

  if(global_thumbnail_mode)
  {
     if(gap_debug) printf("p_thumbnailsave_is_on: global_thumbnail_mode = %s\n", global_thumbnail_mode);
     if (strcmp(global_thumbnail_mode, "none") == 0)
     {
       /* Thumbnails are turned off via (gimprc) Preferences
        * return without saving any thumbnails
        */
       return FALSE;
     }
  }
  else
  {
    if(gap_debug) printf("p_thumbnailsave_is_on: global_thumbnail_mode = <NULL>\n");
  }

  return TRUE;
}  /* end p_thumbnailsave_is_on */


/* -------------------------------
 * p_cond_gimp_file_save_thumbnail
 * -------------------------------
 *
 * Conditional Thubnail save Procedure
 */
gint   
p_cond_gimp_file_save_thumbnail(gint32 image_id, char* filename)
{
  if (!p_thumbnailsave_is_on())
  {
    /* Thumbnails are turned off via (gimprc) Preferences
     * return without saving any thumbnails
     */
    return 0;  /* OK */
  }

  return (p_gimp_file_save_thumbnail(image_id, filename));
}  /* end p_cond_gimp_file_save_thumbnail */

 
/* ------------------------
 * p_alloc_xvpics_thumbname
 * ------------------------
 *   return the thumbnail name (in .xvpics subdir)
 *   for the given filename
 */
char *
p_alloc_xvpics_thumbname(char *name)
{
  int   l_len;
  int   l_len2;
  int   l_idx;
  char *l_str;
  
  if(name == NULL)
  {
    return(g_strdup("\0"));
  }

  l_len = strlen(name);
  l_len2 = l_len + 10;
  l_str = g_malloc(l_len2);
  strcpy(l_str, name);
  if(l_len > 0)
  {
     for(l_idx = l_len -1; l_idx > 0; l_idx--)
     {
        if((name[l_idx] == G_DIR_SEPARATOR) || (name[l_idx] == DIR_ROOT))
        {
           l_idx++;
           break;
        }
     }
     g_snprintf(&l_str[l_idx], l_len2 - l_idx, ".xvpics%s%s", G_DIR_SEPARATOR_S, &name[l_idx]);      
  }
  if(gap_debug) printf("p_alloc_xvpics_thumbname: thumbname=%s:\n", l_str );
  return(l_str);
}  /* end p_alloc_xvpics_thumbname */


/* ------------------------------
 * p_copy_png_thumb
 * ------------------------------
 * copy PNG thumbnail file(s) for filename_src
 * the Thumb::URI Tag in the resulting copy is updated
 * to match filename_dst.
 */
static void
p_copy_png_thumb(char *filename_src, char *filename_dst)
{
  GdkPixbuf          *pixbuf     = NULL;
  GError             *error = NULL;
  gchar              *src_png_thumb;
  gchar              *dst_png_thumb;
  gchar              *src_png_thumb_full;
  gchar              *dst_png_thumb_full;
  gchar              *option_uri;
  gchar              *option_desc;
  const gchar        *option_software;
  const gchar        *option_t_str;
  const gchar        *option_s_str;
  const gchar        *option_w_str;
  const gchar        *option_h_str;
  const gchar        *option_type_str;
  const gchar        *option_l_str;
  gint               ii;

  if(gap_debug) printf("p_copy_png_thumb: START S:%s D:%s\n",filename_src , filename_dst);

  if(thumb_dir == NULL)
  {
    p_gap_init_thumb_dirs();
  }

  src_png_thumb = p_gap_filename_to_png_thumb_name(filename_src);
  if(src_png_thumb == NULL)
  {
    return;
  }
  dst_png_thumb = p_gap_filename_to_png_thumb_name(filename_dst);
  if(dst_png_thumb == NULL)
  {
    return;
  }


  /* copy thumbnail files in the normal and large subdirs 
   * start at index 1 because the .fail subdir is not copied
   */
  for (ii = 1; ii < G_N_ELEMENTS (thumb_subdirs); ii++)
  {
    src_png_thumb_full = g_build_filename (thumb_subdirs[ii], src_png_thumb, NULL);
    if(src_png_thumb_full)
    {
      if(gap_debug) printf("p_copy_png_thumb: SRC: %s\n", src_png_thumb_full);

      if(p_file_exists(src_png_thumb_full) == 1 )
      {
        dst_png_thumb_full = g_build_filename (thumb_subdirs[ii], dst_png_thumb, NULL);
        if(dst_png_thumb_full)
        {
          if(gap_debug) printf("p_copy_png_thumb: DST: %s\n", dst_png_thumb_full);

          pixbuf = gdk_pixbuf_new_from_file (src_png_thumb_full, &error);

          if(gap_debug) printf("pixp_copy_png_thumbbuf: pixbuf: %d\n", (int)pixbuf);
  
          if(pixbuf)
          {

             option_uri = p_gap_filename_to_uri(filename_dst);
             option_desc = g_strdup_printf ("Thumbnail of %s", option_uri);
             
             option_software = gdk_pixbuf_get_option (pixbuf, TAG_SOFTWARE);
             option_t_str = gdk_pixbuf_get_option (pixbuf, TAG_THUMB_MTIME);
             option_s_str = gdk_pixbuf_get_option (pixbuf, TAG_THUMB_SIZE);
             option_w_str = gdk_pixbuf_get_option (pixbuf, TAG_THUMB_IMAGE_WIDTH);
             option_h_str = gdk_pixbuf_get_option (pixbuf, TAG_THUMB_IMAGE_HEIGHT);
             option_type_str = gdk_pixbuf_get_option (pixbuf, TAG_THUMB_GIMP_TYPE);
             option_l_str = gdk_pixbuf_get_option (pixbuf, TAG_THUMB_GIMP_LAYERS);
             
             gdk_pixbuf_save (pixbuf, dst_png_thumb_full, "png", &error,
                                TAG_DESCRIPTION,        option_desc,
                                TAG_SOFTWARE,           option_software,
                                TAG_THUMB_URI,          option_uri,
                                TAG_THUMB_MTIME,        option_t_str,
                                TAG_THUMB_SIZE,         option_s_str,
                                TAG_THUMB_IMAGE_WIDTH,  option_w_str,
                                TAG_THUMB_IMAGE_HEIGHT, option_h_str,
                                TAG_THUMB_GIMP_TYPE,    option_type_str,
                                TAG_THUMB_GIMP_LAYERS,  option_l_str,
                                NULL);
             g_object_unref(pixbuf);
             g_free(option_uri);
             g_free(option_desc);
          }
          g_free(dst_png_thumb_full);
        }

      }
      g_free(src_png_thumb_full);
    }
  }

  g_free(src_png_thumb);
  g_free(dst_png_thumb);

} /* end p_copy_png_thumb  */



/* -------------------------------
 * p_gimp_file_has_valid_thumbnail
 * -------------------------------
 *
 * check if filename has a valid thumbnail
 * 1. check if filename exists
 * 2. check for the new PNG standard (Mtime and URI must match exact)
 * 3. if there is no PNG thumbnail at all then check the old .xvpics standard
 */
gboolean 
p_gimp_file_has_valid_thumbnail(char *filename)
{
  struct stat  l_stat_thumb;
  struct stat  l_stat_image;
  char        *l_xvpics_thumb;

  GdkPixbuf          *pixbuf     = NULL;
  GError             *error = NULL;
  gint               ii;
  const gchar        *option_t_str;
  const gchar        *option_uri;
  gchar              *uri;
  gchar              *src_png_thumb;
  gchar              *src_png_thumb_full;
  long               thumb_mtime;
  gint               png_thumb_is_valid;   /* 0 no png thumb, 1 valid png thumb, 2 invalid png thumb */

  png_thumb_is_valid = 0;

  if (0 != stat(filename, &l_stat_image))
  {
    return(FALSE);  /* no imagefile was found, cant have a valid thumbnail */
  }
  
  /* first we check for the Thumb::MTime Tag in new PNG thumbnail standard
   */
  if(thumb_dir == NULL)
  {
    p_gap_init_thumb_dirs();
  }

  src_png_thumb = p_gap_filename_to_png_thumb_name(filename);
  if(src_png_thumb)
  {
    /* check only the normal and large subdir (but not the fail subdir) */
    for (ii = 1; ii < G_N_ELEMENTS (thumb_subdirs); ii++)
    {
      src_png_thumb_full = g_build_filename (thumb_subdirs[ii], src_png_thumb, NULL);
      if(src_png_thumb_full)
      {
        if(p_file_exists(src_png_thumb_full) == 1 )
        {
            png_thumb_is_valid = 2;  /* assume invalid thumbnail */
            pixbuf = gdk_pixbuf_new_from_file (src_png_thumb_full, &error);

            if(pixbuf)
            {

               uri = p_gap_filename_to_uri(filename);
               option_uri = gdk_pixbuf_get_option (pixbuf, TAG_THUMB_URI);
               option_t_str = gdk_pixbuf_get_option (pixbuf, TAG_THUMB_MTIME);

               if((option_t_str) && (option_uri))
               {
                 sscanf (option_t_str, "%ld", &thumb_mtime);
                  
                 if((strcmp(uri, option_uri) == 0)
                 && (thumb_mtime == l_stat_image.st_mtime))
                 {
                   png_thumb_is_valid = 1;  /* thumbnail is valid */
                 }
               }

               g_object_unref(pixbuf);
            }
        }
        g_free(src_png_thumb_full);
      }
      
      if(png_thumb_is_valid != 0)
      {
        break;
      }
    }

    g_free(src_png_thumb);
  }

  if(png_thumb_is_valid == 1)
  {
    return TRUE;
  }
  if(png_thumb_is_valid == 2)
  {
    return FALSE;
  }

  /* there was no PNG thumnail at all, so we check the old .xvpics standard too */
  l_xvpics_thumb = p_alloc_xvpics_thumbname(filename);
     
  if(l_xvpics_thumb)
  {
    if (0 == stat(l_xvpics_thumb, &l_stat_thumb))
    {
      /* thumbnail filename exists */
      if(S_ISREG(l_stat_thumb.st_mode))
      {
          if(l_stat_image.st_mtime <= l_stat_thumb.st_mtime)
          {
           /* time of last modification of image is older (less or equal)
            * than last modification of the thumbnail
            * So we can assume the thumbnail is valid
            */
            g_free(l_xvpics_thumb);
            return TRUE;
          }
      }
    }
    g_free(l_xvpics_thumb);
  }

  return FALSE;
}  /* end p_gimp_file_has_valid_thumbnail */


/* ------------------------------
 * p_gimp_file_delete_thumbnail
 * ------------------------------
 * this procedure is usual called immediate after
 * an imagefile was deleted on disc.
 * it removes all thumbnail files for filename
 * both in the old .xvpics standard and in the new PNG standard.
 */
void
p_gimp_file_delete_thumbnail(char *filename)
{
  char        *xvpics_thumb;
  gint ii;
  gchar       *png_thumb;
  gchar       *png_thumb_full;

  if(gap_debug) printf("p_gimp_file_delete_thumbnail: START :%s\n",filename);

  /* check and remove thumbnail file for old .xvpics standard */  
  xvpics_thumb = p_alloc_xvpics_thumbname(filename);
  if(xvpics_thumb)
  {
    if(p_file_exists(xvpics_thumb) == 1) 
    {  
       if(gap_debug) fprintf(stderr, "\nDEBUG p_delete_frame: %s\n", xvpics_thumb);
       remove(xvpics_thumb);
    }
    g_free(xvpics_thumb);
  }

  /* check and remove thumbnail files for new thumbnail standard 
   * (for all 3 directories
   *   ~/.thumbnails/.normal
   *   ~/.thumbnails/.large
   *   ~/.thumbnails/.fail
   */
  if(thumb_dir == NULL)
  {
    p_gap_init_thumb_dirs();
  }
  
  png_thumb = p_gap_filename_to_png_thumb_name(filename);
  
  if(png_thumb)
  {
    for (ii = 0; ii < G_N_ELEMENTS (thumb_subdirs); ii++)
    {
       png_thumb_full = g_build_filename (thumb_subdirs[ii], png_thumb, NULL);
       if(png_thumb_full)
       {
         if(p_file_exists(png_thumb_full) == 1) 
         {  
           remove(png_thumb_full);
         }
         g_free(png_thumb_full);
       }
    }
    g_free(png_thumb);
  }
}  /* end p_gimp_file_delete_thumbnail */


/* ------------------------------
 * p_gimp_file_copy_thumbnail
 * ------------------------------
 * this procedure is usual called immediate after
 * an imagefile was copied on disc.
 *
 * if thumbnail saving is turned on (in gimprc)
 * the existing thumbnail(s) are copied or renamed to match
 * the destination file for both the old .xvpics and the new PNG standard.
 *
 */
void
p_gimp_file_copy_thumbnail(char *filename_src, char *filename_dst)
{
  char          *l_src_xvpics_thumb;
  char          *l_dst_xvpics_thumb;


  if (!p_thumbnailsave_is_on())
  {
    return;
  }

  /* check for thumbnail file for old .xvpics standard */  
  l_src_xvpics_thumb = p_alloc_xvpics_thumbname(filename_src);
  if(l_src_xvpics_thumb)
  {
    if(p_file_exists(l_src_xvpics_thumb) == 1) 
    {  
       l_dst_xvpics_thumb = p_alloc_xvpics_thumbname(filename_dst);
       if(l_dst_xvpics_thumb)
       {
         /* copy the .xvpics thumbnail file */
         p_file_copy(l_src_xvpics_thumb, l_dst_xvpics_thumb);
         g_free(l_dst_xvpics_thumb);
       }
    }
    g_free(l_src_xvpics_thumb);
  }

  /* copy (and update) the PNG thumbnail file(s) */
  p_copy_png_thumb(filename_src, filename_dst);
      
}  /* end p_gimp_file_copy_thumbnail */


/* ------------------------------
 * p_gimp_file_rename_thumbnail
 * ------------------------------
 * this procedure is usual called immediate after
 * an imagefile was renamed on disc.
 *
 * if thumbnail saving is turned on (in gimprc)
 * the existing thumbnail(s) are copied or renamed to match
 * the destination file for both the old .xvpics and the new PNG standard.
 *
 * The Old Thumnail(s) are deleted (unconditional)
 */
void
p_gimp_file_rename_thumbnail(char *filename_src, char *filename_dst)
{
  char          *l_src_xvpics_thumb;
  char          *l_dst_xvpics_thumb;

  if (p_thumbnailsave_is_on())
  {
    /* check for thumbnail file for old .xvpics standard */  
    l_src_xvpics_thumb = p_alloc_xvpics_thumbname(filename_src);
    if(l_src_xvpics_thumb)
    {
      if(p_file_exists(l_src_xvpics_thumb) == 1) 
      {  
         l_dst_xvpics_thumb = p_alloc_xvpics_thumbname(filename_dst);
         if(l_dst_xvpics_thumb)
         {
           /* rename the .xvpics thumbnail file */
           rename(l_src_xvpics_thumb, l_dst_xvpics_thumb);
           g_free(l_dst_xvpics_thumb);
         }
      }
      g_free(l_src_xvpics_thumb);
    }
    
    /* copy (and update) the PNG thumbnail file(s) */
    p_copy_png_thumb(filename_src, filename_dst);
  }

  p_gimp_file_delete_thumbnail(filename_src);
  
}  /* end p_gimp_file_rename_thumbnail */
