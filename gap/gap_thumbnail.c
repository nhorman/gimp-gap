/* gap_thumbnail.c
 * 2003.05.27 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * basic GAP thumbnail handling utility procedures
 *
 * the gap Thumbnail handling acts as wrapper to libgimpthumb procedures
 * and adds some thumbnail related stuff to satisfy the needs of the gimp-gap project.
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
 * 1.3.24a  2004/01/16   hof: make use of libgimpthumb (see also feature request #113033) 
 *                            to replace old private thumnail handling code
 * 1.3.17a  2003/07/28   hof: G_N_ELEMENTS is unsigned
 * 1.3.14b  2003/06/03   hof: check only for thumbnail-size "none" (suggested by sven see #113033)
 *                            removed p_gimp_file_has_valid_thumbnail
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
#include "libgimpthumb/gimpthumb.h"


/* GAP includes */
#include "gap_lib.h"
#include "gap_pdb_calls.h"
#include "gap_thumbnail.h"


extern      int gap_debug; /* ==0  ... dont print debug infos */

static gboolean gap_thumb_initialized = FALSE;
static char    *global_thumbnail_mode = NULL;  /* NULL or pointer to "none", "normal", "large" */
static gchar   *global_creator_software = NULL;       /* gimp-1.3 */

static void            p_gap_thumb_init(void);
static gchar *         p_gap_filename_to_uri(const char *filename);

static char *          p_alloc_xvpics_thumbname(char *name);
static void            p_copy_png_thumb(char *filename_src, char *filename_dst);

#define MIX_CHANNEL(b, a, m)  (((a * m) + (b * (255 - m))) / 255)
#define THUMB_BG_GRAY1 80
#define THUMB_BG_GRAY2 180
#define THUMB_BG_CECK_SIZE 8


/* --------------------------------
 * p_gap_thumb_init
 * --------------------------------
 */
static void
p_gap_thumb_init(void)
{
  gchar *thumb_basedir;

  thumb_basedir = NULL;
  if(global_creator_software == NULL)
  {
    global_creator_software = g_strdup_printf ("gap%d.%d"
                                       , GAP_MAJOR_VERSION
				       , GAP_MINOR_VERSION);
  }

  if (gimp_thumb_init(global_creator_software, thumb_basedir))
  {
    /* init was successful */
    gap_thumb_initialized = TRUE;
  }
  else
  {
    printf("p_gap_thumb_init: call of gimp_thumb_init FAILED!\n");
    gap_thumb_initialized = FALSE;
  }
  

}  /* end p_gap_thumb_init */


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
 *
 * load src thumbnail as pixbuf for all thumnailsizes ( in the subdirs normal and large)
 * and save the pixbuf for the new filename_dst.
 *
 * The following Information Tags in the resulting copy are updated
 * to match filename_dst. (this is done implicite by the gimp_thumbnail_* procedures)
 *   tEXt::Thumb::URI
 *   tEXt::Thumb::MTime
 *   tEXt::Description
 *   tEXt::Thumb::Size
 *
 * The following Tags are copied explicite from the source thumbnail:
 *   tEXt::Thumb::Image::Width
 *   tEXt::Thumb::Image::Height
 *   tEXt::Thumb::X-GIMP::Type
 *   tEXt::Thumb::X-GIMP::Layers
 *
 * The Tag
 *   tEXt::Software   
 * is set to "gap-<MAJOR_VERSION>.<MINOR_VERSION>"
 *
 * Please note that the image (filename_dst) must have been already created
 * as copy of filename_src before you can call p_copy_png_thumb
 */
static void
p_copy_png_thumb(char *filename_src, char *filename_dst)
{
  gchar       *uri_src;
  gchar       *uri_dst;
  GEnumClass *enum_class;
  GEnumValue *enum_value;
  guint       ii;
  gchar              *src_png_thumb_full;
  gchar              *dst_png_thumb_full;
  GdkPixbuf          *pixbuf     = NULL;
  GError             *error = NULL;


  if(gap_debug) printf("p_copy_png_thumb: START S:%s D:%s\n",filename_src , filename_dst);

  if(!gap_thumb_initialized)
  {
    p_gap_thumb_init();
  }

  uri_src = p_gap_filename_to_uri(filename_src);
  if(uri_src == NULL)
  {
    return;
  }

  uri_dst = p_gap_filename_to_uri(filename_dst);
  if(uri_dst == NULL)
  {
    return;
  }


  /* copy thumbnail files in the normal and large subdirs */
  enum_class = g_type_class_ref (GIMP_TYPE_THUMB_SIZE);
  for (ii = 0, enum_value = enum_class->values;
       ii < enum_class->n_values;
       ii++, enum_value++)
  {
    if(enum_value->value == GIMP_THUMB_SIZE_FAIL)
    {
      continue;  /* skip the .fail dir (makes no sense to copy this) */
    }
    src_png_thumb_full = gimp_thumb_name_from_uri (uri_src, enum_value->value);
    if(src_png_thumb_full)
    {
      if(gap_debug) printf("p_copy_png_thumb: SRC: %s\n", src_png_thumb_full);

      if(gap_lib_file_exists(src_png_thumb_full) == 1 )
      {
        dst_png_thumb_full = gimp_thumb_name_from_uri (uri_dst, enum_value->value);
        if(dst_png_thumb_full)
        {
          GimpThumbnail *thumbnail;

          if(gap_debug) printf("p_copy_png_thumb: DST: %s\n", dst_png_thumb_full);

          thumbnail = gimp_thumbnail_new();
          if(thumbnail)
	  {
	    gimp_thumbnail_set_filename(thumbnail, filename_src, &error);
            pixbuf = gimp_thumbnail_load_thumb(thumbnail, enum_value->value, &error);

            if(pixbuf)
            {
	      gint  l_width, l_height, l_num_layers;
	      gchar *l_type_str;
	      
              if(gap_debug) printf("p_copy_png_thumb: pixbuf: %d\n", (int)pixbuf);

	      l_width = 1;
	      l_height = 1;
	      l_num_layers = 1;
	      l_type_str = NULL;
              g_object_get (thumbnail
	                   , "image-width",      &l_width
			   , "image-height",     &l_height
			   , "image-type",       &l_type_str
			   , "image-num-layers", &l_num_layers
			   , NULL);

              /* set new filename (does also set uri and reset all other TAGs */
	      gimp_thumbnail_set_filename(thumbnail, filename_dst, &error);
	      gimp_thumbnail_peek_image(thumbnail);

              /* copy TAGs for width,height,type,num_layers */
              if(l_type_str)
	      {
		g_object_set (thumbnail,
        		      "image-width",      l_width,
        		      "image-height",     l_height,
        		      "image-type",       l_type_str,
        		      "image-num-layers", l_num_layers,
        		      NULL);
                g_free(l_type_str);
	      }
	      else
	      {
		g_object_set (thumbnail,
        		      "image-width",      l_width,
        		      "image-height",     l_height,
        		      "image-num-layers", l_num_layers,
        		      NULL);
	      }
	      
	    
              gimp_thumbnail_save_thumb(thumbnail
		                         , pixbuf
					 , global_creator_software
                                         , &error
					 );
              g_object_unref(pixbuf);
	    }
	    g_free(thumbnail);
	  }
          g_free(dst_png_thumb_full);
        }

      }
      g_free(src_png_thumb_full);
    }
  }

  g_free(uri_src);
  g_free(uri_dst);

} /* end p_copy_png_thumb  */


/* ============= external procedures ============== */



/* ------------------------------------
 * gap_thumb_gimprc_query_thumbnailsave
 * ------------------------------------
 * return the gimprc value_string
 * responsible for video thumbnail configuration
 * the caller should g_free the retuned string
 *
 * checking gimprc for "thumbnail-size" (GIMP native, values:  "none", "normal", "large"
 *
 */
char *   
gap_thumb_gimprc_query_thumbnailsave(void)
{
  if(global_thumbnail_mode)
  {
    g_free(global_thumbnail_mode);
  }

  global_thumbnail_mode = gimp_gimprc_query("thumbnail-size");
  if(global_thumbnail_mode)
  {
    return g_strdup(global_thumbnail_mode);
  }
  return(NULL);
}  /* end gap_thumb_gimprc_query_thumbnailsave */



/* -------------------------------
 * gap_thumb_thumbnailsave_is_on
 * -------------------------------
 * checking gimprc if thumnail saving is enabled.
 * keyword "thumbnail-size" values:  "none"
 * is checked.
 */
gboolean   
gap_thumb_thumbnailsave_is_on(void)
{
  if(global_thumbnail_mode == NULL)
  {
    global_thumbnail_mode = gimp_gimprc_query("thumbnail-size");
  }

  if(global_thumbnail_mode)
  {
     if(gap_debug) printf("gap_thumb_thumbnailsave_is_on: global_thumbnail_mode = %s\n", global_thumbnail_mode);
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
    if(gap_debug) printf("gap_thumb_thumbnailsave_is_on: global_thumbnail_mode = <NULL>\n");
  }

  return TRUE;
}  /* end gap_thumb_thumbnailsave_is_on */



/* ---------------------------------------
 * gap_thumb_cond_gimp_file_save_thumbnail
 * ---------------------------------------
 *
 * Conditional Thubnail save Procedure
 */
gboolean   
gap_thumb_cond_gimp_file_save_thumbnail(gint32 image_id, char* filename)
{
  if (!gap_thumb_thumbnailsave_is_on())
  {
    /* Thumbnails are turned off via (gimprc) Preferences
     * return without saving any thumbnails
     */
    return TRUE;  /* OK */
  }

  return (gap_pdb_gimp_file_save_thumbnail(image_id, filename));
}  /* end gap_thumb_cond_gimp_file_save_thumbnail */


/* ------------------------------------
 * gap_thumb_file_delete_thumbnail
 * ------------------------------------
 * this procedure is usual called immediate after
 * an imagefile was deleted on disc.
 * it removes all thumbnail files for filename
 * both in the old .xvpics standard and in the new PNG standard.
 */
void
gap_thumb_file_delete_thumbnail(char *filename)
{
  char        *xvpics_thumb;
  gchar       *uri;
  gchar       *png_thumb_full;

  if(gap_debug) printf("gap_thumb_file_delete_thumbnail: START :%s\n",filename);

  /* check and remove thumbnail file for old .xvpics standard */  
  xvpics_thumb = p_alloc_xvpics_thumbname(filename);
  if(xvpics_thumb)
  {
    if(gap_lib_file_exists(xvpics_thumb) == 1) 
    {  
       if(gap_debug) fprintf(stderr, "\nDEBUG gap_lib_delete_frame: %s\n", xvpics_thumb);
       remove(xvpics_thumb);
    }
    g_free(xvpics_thumb);
  }

  if(!gap_thumb_initialized)
  {
    p_gap_thumb_init();
  }
  
  uri = p_gap_filename_to_uri(filename);
  
  if(uri)
  {
    GEnumClass *enum_class;
    GEnumValue *enum_value;
    guint       ii;

    /* check and remove thumbnail files for new thumbnail standard 
     * (for all 3 directories
     *   ~/.thumbnails/.normal
     *   ~/.thumbnails/.large
     *   ~/.thumbnails/.fail
     */

    enum_class = g_type_class_ref (GIMP_TYPE_THUMB_SIZE);

    for (ii = 0, enum_value = enum_class->values;
       ii < enum_class->n_values;
       ii++, enum_value++)
    {
      if (gap_debug) printf ("gap_thumb_file_delete_thumbnail: enum_value: %d, value_nick:%s\n", (int)enum_value->value, enum_value->value_nick);

      png_thumb_full = gimp_thumb_name_from_uri (uri, enum_value->value);
      if(png_thumb_full)
      {
        if(gap_lib_file_exists(png_thumb_full) == 1) 
        {  
          if (gap_debug) printf ("gap_thumb_file_delete_thumbnail: png_thumb_full: %s\n", png_thumb_full);
          remove(png_thumb_full);
        }
        g_free(png_thumb_full);
      }
    }
    
    g_free(uri);
  }
}  /* end gap_thumb_file_delete_thumbnail */


/* ------------------------------
 * gap_thumb_file_copy_thumbnail
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
gap_thumb_file_copy_thumbnail(char *filename_src, char *filename_dst)
{
  char          *l_src_xvpics_thumb;
  char          *l_dst_xvpics_thumb;

  if (!gap_thumb_thumbnailsave_is_on())
  {
    return;
  }

  /* check for thumbnail file for old .xvpics standard */  
  l_src_xvpics_thumb = p_alloc_xvpics_thumbname(filename_src);
  if(l_src_xvpics_thumb)
  {
    if(gap_lib_file_exists(l_src_xvpics_thumb) == 1) 
    {  
       l_dst_xvpics_thumb = p_alloc_xvpics_thumbname(filename_dst);
       if(l_dst_xvpics_thumb)
       {
         /* copy the .xvpics thumbnail file */
         gap_lib_file_copy(l_src_xvpics_thumb, l_dst_xvpics_thumb);
         g_free(l_dst_xvpics_thumb);
       }
    }
    g_free(l_src_xvpics_thumb);
  }

 /* copy (and update) the PNG thumbnail file(s) */
  p_copy_png_thumb(filename_src, filename_dst);
      
}  /* end gap_thumb_file_copy_thumbnail */


/* ------------------------------------
 * gap_thumb_file_rename_thumbnail
 * ------------------------------------
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
gap_thumb_file_rename_thumbnail(char *filename_src, char *filename_dst)
{
  char          *l_src_xvpics_thumb;
  char          *l_dst_xvpics_thumb;

  if (gap_thumb_thumbnailsave_is_on())
  {
    /* check for thumbnail file for old .xvpics standard */  
    l_src_xvpics_thumb = p_alloc_xvpics_thumbname(filename_src);
    if(l_src_xvpics_thumb)
    {
      if(gap_lib_file_exists(l_src_xvpics_thumb) == 1) 
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

  gap_thumb_file_delete_thumbnail(filename_src);
  
}  /* end gap_thumb_file_rename_thumbnail */


/* -----------------------------
 * gap_thumb_file_load_thumbnail
 * -----------------------------
 * load thumbnail data as RGB data
 * 1.) try direct load PNG thumbnail files as GDK pixbuf data via libgimpthumb
 * 2.) try the GIMP PDB procedure gimp_file_load_thumbnail
 *       just for old .xvpics thumbnail support
 *       the PDB Procedure always delivers RGB data (th_bpp == 3)
 *
 * IN: filename of the image (whos thumbnail is to load)
 *
 * I/O: th_width, th_height  as hint what thumbnail size to load 
 *                            (normal == 128 or large == 256)
 *                            for the case that both sizes are available.
 *                            The values for width and height of the thumbnaildata
 *                            are returned in th_width, th_height
 * I/O: th_bpp               If a PNG Thumbnail is available the returned th_data
 *                            can have an alpha channel. (th_bpp == 4)
 *                            The caller can set th_bpp to a value of 3
 *                            to force th_data to RGB. (in that case the alpha
 *                            channel is removed, and transparent parts are replaced
 *                            by the GIMP typical check board backgrond).
 *                            if the caller sets *th_bpp to a value of 4 the returned
 *                            data may have alpha (depending on the thumbnail filedata)
 *                            The returned value in th_bpp tells the caller
 *                            how many bytes per pixels are used in the returned th_data buffer. 
 * OUT: th_data_count       The total number of bytes in the returned th_data buffer. 
 * OUT: th_data             The returned pixeldata buffer.
 *
 * allow to recieve 
 *                            and the caller allows the returned allow the caller must supply a                            
 */

gboolean
gap_thumb_file_load_thumbnail(char* filename, gint32 *th_width, gint32 *th_height,
                           gint32 *th_data_count, gint32 *th_bpp, unsigned char **th_data)
{
  static char     *l_called_proc = "gimp_file_load_thumbnail";


  GimpThumbnail *thumbnail;
  GdkPixbuf          *pixbuf     = NULL;
  GError             *error = NULL;
  GimpThumbSize       wanted_size;
  gboolean            rc;

  if(gap_debug) printf("gap_thumb_file_load_thumbnail:  %s\n", filename);

  *th_data = NULL;
  rc = FALSE;

  if(!gap_thumb_initialized)
  {
    p_gap_thumb_init();
  }
   
  thumbnail = gimp_thumbnail_new();
  if(thumbnail)
  {
    gimp_thumbnail_set_filename(thumbnail, filename, &error);

    wanted_size = GIMP_THUMB_SIZE_NORMAL;
    if(MAX(*th_width, *th_height) > 128)
    {
      wanted_size = GIMP_THUMB_SIZE_LARGE;
    }

    pixbuf = gimp_thumbnail_load_thumb(thumbnail, wanted_size, &error);
    if(pixbuf)
    {
      int nchannels;
      int rowstride;
      int width;
      int height;
      guchar *pix_data;
      gboolean has_alpha;

      width = gdk_pixbuf_get_width(pixbuf);
      height = gdk_pixbuf_get_height(pixbuf);
      nchannels = gdk_pixbuf_get_n_channels(pixbuf);
      pix_data = gdk_pixbuf_get_pixels(pixbuf);
      has_alpha = gdk_pixbuf_get_has_alpha(pixbuf);
      rowstride = gdk_pixbuf_get_rowstride(pixbuf);

      if(gap_debug)
      {
	printf("gap_thumb_file_load_thumbnail:\n");
	printf(" width: %d\n", (int)width );
	printf(" height: %d\n", (int)height );
	printf(" nchannels: %d\n", (int)nchannels );
	printf(" pix_data: %d\n", (int)pix_data );
	printf(" has_alpha: %d\n", (int)has_alpha );
	printf(" rowstride: %d\n", (int)rowstride );
      }

      *th_width = width;
      *th_height = height;

      /* check if we have RGB or RGBA pixeldata,
       * (Note: GIMP-2.0 uses always RGBA for the PNG thumbnails, even for imagetype GRAY)
       */
      if(pix_data)
      {
        if ((nchannels == 4) && (*th_bpp == 3))
	{
	  register guchar *src;
	  register guchar *dest;
	  register guchar  alpha;
	  gint   row, col;
	  gint     ii;
	  

          /* Flatten alpha channel, bacause the caller wants just RGB thumbnail data */
          *th_bpp = 3;
          *th_data_count =  *th_bpp * width * height;
	  
          *th_data = g_malloc(*th_data_count);
	  dest = *th_data;
          src = pix_data;
	  for(row=0; row < height; row++)
	  {
            if ((row / THUMB_BG_CECK_SIZE) & 1) { ii = 0;}
            else                            { ii = THUMB_BG_CECK_SIZE; }
	    
	    for(col=0; col < width; col++)
	    {
	      alpha = src[3];
	      if(alpha == 255)
	      {
        	    *(dest++) = src[0];
        	    *(dest++) = src[1];
        	    *(dest++) = src[2];
	      }
	      else
	      {
                if(((col+ii) / THUMB_BG_CECK_SIZE) & 1)
		{
		  if(alpha ==0)
		  {
        	    *(dest++) = THUMB_BG_GRAY1;
        	    *(dest++) = THUMB_BG_GRAY1;
        	    *(dest++) = THUMB_BG_GRAY1;
		  }
		  else
		  {
        	    *(dest++) = MIX_CHANNEL (THUMB_BG_GRAY1, src[0], alpha);
        	    *(dest++) = MIX_CHANNEL (THUMB_BG_GRAY1, src[1], alpha);
        	    *(dest++) = MIX_CHANNEL (THUMB_BG_GRAY1, src[2], alpha);
		  }
		}
		else
		{
		  if(alpha ==0)
		  {
        	    *(dest++) = THUMB_BG_GRAY2;
        	    *(dest++) = THUMB_BG_GRAY2;
        	    *(dest++) = THUMB_BG_GRAY2;
		  }
		  else
		  {
        	    *(dest++) = MIX_CHANNEL (THUMB_BG_GRAY2, src[0], alpha);
        	    *(dest++) = MIX_CHANNEL (THUMB_BG_GRAY2, src[1], alpha);
        	    *(dest++) = MIX_CHANNEL (THUMB_BG_GRAY2, src[2], alpha);
		  }
		}
		
		
	      }
	      src += nchannels;
	      
	    }
	  }
	  rc = TRUE; /* OK */
 	}
	else
	{
	  if ((nchannels == 3) || (nchannels == 4))
	  {
            *th_bpp = nchannels;
            *th_data_count =  *th_bpp * width * height;
            *th_data = g_malloc(*th_data_count);
	    memcpy(*th_data, pix_data, *th_data_count);
	    rc = TRUE; /* OK */
	  }
	}
      }

      g_object_unref(pixbuf);       
    }
    g_free(thumbnail);
  }

  if(!rc)
  {
    GimpParam       *return_vals;
    int              nreturn_vals;

    /* start a 2.nd try to load thumbnail via GIMP PDB
     * this is slower, but also supports the old .xvpics
     * thumbnail style
     */

    /*if(gap_debug)*/
    {
       printf("gap_thumb_file_load_thumbnail: 2-nd TRY via PDB\n");
    }

    return_vals = gimp_run_procedure (l_called_proc,
                                  &nreturn_vals,
 				  GIMP_PDB_STRING,    filename,
                                  GIMP_PDB_END);

    if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
    {
       *th_width = return_vals[1].data.d_int32;
       *th_height = return_vals[2].data.d_int32;
       *th_data_count = return_vals[3].data.d_int32;
       *th_data = (unsigned char *)return_vals[4].data.d_int8array;
       *th_bpp = 3;  /* always fixed to 3 when using the PDB call */
       rc = TRUE; /* OK */
    }
    else
    {
      if(gap_debug) printf("GAP: Error: PDB call of %s failed\n", l_called_proc);
    }
  }

  return (rc);

}	/* end gap_thumb_file_load_thumbnail */

