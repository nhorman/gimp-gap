/*  gap_pview_da.c
 *
 *  This module handles GAP drawing_area rendering from thumbnail data buffer
 *  or alternative from a gimp image
 *  (used for video playback preview)
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
 * version 1.3.25a; 2004/01/22  hof: added gap_pview_render_from_pixbuf 
 * version 1.3.24a; 2004/01/17  hof: speed up gap_pview_render_from_buf 
 *                                   faster rendering of fully opaque pixels
 *                                   for buffers with alpha channel
 * version 1.3.16a; 2003/06/26  hof: use aspect_frame instead of simple frame
 *                                   added gap_pview_render_default_icon
 * version 1.3.14c; 2003/06/19  hof: created
 */

#include "config.h"

#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include "gap_pview_da.h"
#include "gap-intl.h"


extern int gap_debug;  /* 1 == print debug infos , 0 dont print debug infos */


#define MIX_CHANNEL(b, a, m)  (((a * m) + (b * (255 - m))) / 255)
#define PREVIEW_BG_GRAY1 80
#define PREVIEW_BG_GRAY2 180

#define PREVIEW_BG_GRAY1_GDK 0x505050
#define PREVIEW_BG_GRAY2_GDK 0xb4b4b4

/* ------------------------------
 * gap_pview_reset
 * ------------------------------
 * reset/free precalculated stuff
 */
void
gap_pview_reset(GapPView *pv_ptr)
{
  if(pv_ptr->src_col) g_free(pv_ptr->src_col);
  if(pv_ptr->pv_area_data)  g_free(pv_ptr->pv_area_data);
  if(pv_ptr->pixmap)        g_object_unref(pv_ptr->pixmap);
  if(pv_ptr->pixbuf)        g_object_unref(pv_ptr->pixbuf);

  pv_ptr->src_col = NULL;
  pv_ptr->pv_area_data = NULL;
  pv_ptr->src_width = 0;
  pv_ptr->src_bpp = 0;
  pv_ptr->src_rowstride = 0;
  pv_ptr->use_pixmap_repaint = FALSE;
  pv_ptr->use_pixbuf_repaint = FALSE;
  pv_ptr->pixmap = NULL;
  pv_ptr->pixbuf = NULL;
  
} /* end gap_pview_reset */


/* ------------------------------
 * gap_pview_set_size
 * ------------------------------
 * set new preview size
 * and allocate buffers for src columns and row at previesize
 */
void
gap_pview_set_size(GapPView *pv_ptr, gint pv_width, gint pv_height, gint pv_check_size)
{
  if(pv_ptr->da_widget == NULL) { return; }

  gap_pview_reset(pv_ptr);

  gtk_widget_set_size_request (pv_ptr->da_widget, pv_width, pv_height);
  if(pv_ptr->aspect_frame)
  { 
    gtk_aspect_frame_set (GTK_ASPECT_FRAME(pv_ptr->aspect_frame)
                         ,0.5   /* align x centered */
                         ,0.5   /* align y centered */
                         , pv_width / pv_height
                         , TRUE  /* obey_child */
                         );
    gtk_widget_set_size_request (pv_ptr->aspect_frame
                                , pv_width+5
                                , (gdouble)(pv_width+5) * ( (gdouble)pv_height / (gdouble)pv_width)                       /* pv_height */
                                );
  }
  pv_ptr->pv_width = pv_width;
  pv_ptr->pv_height = pv_height;
  pv_ptr->pv_check_size = pv_check_size;

  pv_ptr->pv_area_data = g_new ( guchar
                                    , pv_ptr->pv_height * pv_ptr->pv_width * pv_ptr->pv_bpp );
  pv_ptr->src_col = g_new ( gint, pv_ptr->pv_width );                   /* column fetch indexes foreach preview col */
  
}  /* end gap_pview_set_size */


/* ------------------------------
 * gap_pview_new
 * ------------------------------
 * pv_check_size is checkboard size in pixels
 * (for transparent pixels)
 */
GapPView *
gap_pview_new(gint pv_width, gint pv_height, gint pv_check_size, GtkWidget *aspect_frame)
{
  GapPView *pv_ptr;
 
  pv_ptr = g_malloc0(sizeof(GapPView));
  pv_ptr->pv_bpp = 3;
 
  pv_ptr->da_widget = gtk_drawing_area_new ();
  pv_ptr->aspect_frame = aspect_frame;
  gap_pview_set_size(pv_ptr, pv_width, pv_height, pv_check_size);
  pv_ptr->use_pixmap_repaint = FALSE;
  pv_ptr->use_pixbuf_repaint = FALSE;
  pv_ptr->pixmap = NULL;
  pv_ptr->pixbuf = NULL;

  return(pv_ptr);
}  /* end gap_pview_new */


/* ------------------------------
 * gap_pview_repaint
 * ------------------------------
 */
void
gap_pview_repaint(GapPView *pv_ptr)
{
  if(pv_ptr == NULL) { return; }
  if(pv_ptr->da_widget == NULL) { return; }
  if(pv_ptr->da_widget->window == NULL) { return; }

  if((pv_ptr->pixbuf != NULL)
  && (pv_ptr->use_pixbuf_repaint))
  {
    gdk_draw_pixbuf(
                     pv_ptr->da_widget->window
		   , pv_ptr->da_widget->style->white_gc
                   , pv_ptr->pixbuf
                   , 0                         /*  gint src_x  */
                   , 0                         /*  gint src_y  */
                   , 0                         /*  gint dest_x */
                   , 0                         /*  gint dest_y */
                   , pv_ptr->pv_width
                   , pv_ptr->pv_height
                   , GDK_RGB_DITHER_NORMAL
                   , 0                         /* gint x_dither_offset */
                   , 0                         /* gint y_dither_offset */
		   );
    return;
  }
  
  
  if((pv_ptr->pv_area_data != NULL)
  && (!pv_ptr->use_pixmap_repaint))
  {
    gdk_draw_rgb_image ( pv_ptr->da_widget->window
		       , pv_ptr->da_widget->style->white_gc
		       , 0
		       , 0
		       , pv_ptr->pv_width
		       , pv_ptr->pv_height
		       , GDK_RGB_DITHER_NORMAL
		       , pv_ptr->pv_area_data
		       , pv_ptr->pv_width * 3
		       );
    return;
  }
  if(pv_ptr->pixmap != NULL)
  {
    gdk_draw_drawable(pv_ptr->da_widget->window
                   ,pv_ptr->da_widget->style->white_gc
                   ,pv_ptr->pixmap
                   ,0
                   ,0
                   ,0
                   ,0
                   ,pv_ptr->pv_width
                   ,pv_ptr->pv_height
                   );
  }
}  /* end gap_pview_repaint */


/* ------------------------------
 * gap_pview_render_from_buf
 * ------------------------------
 * render drawing_area widget from src_data buffer
 * quick scaling without any interpolation
 * is done to fit into preview size. 
 * the GapPView structure holds
 * precalculations to allow faster scaling
 * in multiple calls when the src_data dimensions
 * do not change.
 *
 * IN: allow_grab_src_data
 *         if TRUE, grant permission to grab the src_data for internal (refresh) usage
 *         this is a performance feature with benefit for the case of matching buffer size
 *         without alpha channel.
 *
 * return TRUE in case the src_data really was grabbed.
 *             in such a case the caller MUST NOT change the src_data after
 *             the call, and MUST NOT free the src_data buffer.
 *        FALSE src_data was not grabbed, the caller is still responsible
 *             to g_free the src_data
 *            
 */
gboolean
gap_pview_render_from_buf (GapPView *pv_ptr
                 , guchar *src_data
                 , gint    src_width
                 , gint    src_height
                 , gint    src_bpp
                 , gboolean allow_grab_src_data
                 )
{
  register guchar  *src, *dest;
  register gint    col;
  gint           row;
  gint           ofs_green, ofs_blue, ofs_alpha;

  if(pv_ptr == NULL) { return FALSE; }
  if(pv_ptr->da_widget == NULL) { return FALSE; }
  if(pv_ptr->da_widget->window == NULL)
  { 
    printf("gap_pview_render_from_buf: drawing_area window pointer is NULL, cant render\n");
    return FALSE;
  }

  /* clear flag to let gap_pview_repaint procedure know
   * to use the pv_area_data rather than the pixmap for refresh
   */
  pv_ptr->use_pixmap_repaint = FALSE;
  pv_ptr->use_pixbuf_repaint = FALSE;

  /* check for col and area_data buffers (allocate if needed) */
  if ((pv_ptr->src_col == NULL)
  ||  (pv_ptr->pv_area_data == NULL))
  {
     gint pv_width, pv_height, pv_check_size;
     
     pv_width = pv_ptr->pv_width;
     pv_height = pv_ptr->pv_height;
     pv_check_size = pv_ptr->pv_check_size;
     
     gap_pview_set_size(pv_ptr, pv_width, pv_height, pv_check_size);
  }
  
  /* init column fetch indexes array (only needed on src size changes) */
  if((src_width != pv_ptr->src_width)
  || (src_bpp != pv_ptr->src_bpp))
  {
     pv_ptr->src_width = src_width;
     pv_ptr->src_bpp   = src_bpp;
     pv_ptr->src_rowstride = src_width * src_bpp;

     /* array of column fetch indexes for quick access
      * to the source pixels. (The source row may be larger or smaller than pv_ptr->pv_width)
      */
     for ( col = 0; col < pv_ptr->pv_width; col++ )
     {
       pv_ptr->src_col[ col ] = ( col * src_width / pv_ptr->pv_width ) * src_bpp;
     }
  }

  if(src_data == NULL)
  { 
    /* if(gap_debug) printf("\n   Paint it Black (NO src_data)\n"); */

    /* Source is empty: draw a black preview */
    memset(pv_ptr->pv_area_data, 0, pv_ptr->pv_height * pv_ptr->pv_width * pv_ptr->pv_bpp);

    gdk_draw_rgb_image ( pv_ptr->da_widget->window
		       , pv_ptr->da_widget->style->white_gc
		       , 0
		       , 0
		       , pv_ptr->pv_width
		       , pv_ptr->pv_height
		       , GDK_RGB_DITHER_NORMAL
		       , pv_ptr->pv_area_data
		       , pv_ptr->pv_width * 3
		       );
    return FALSE;
  }

  if ((src_width == pv_ptr->pv_width)
  &&  (src_height == pv_ptr->pv_height)
  &&  (src_bpp == 3))
  {

    /* if(gap_debug) printf("\n\n   can use QUICK render\n"); */

    /* for RGB src_data with matching size and without Alpha
     * we can render with just one call (fastest)
     */
    
    gdk_draw_rgb_image ( pv_ptr->da_widget->window
		       , pv_ptr->da_widget->style->white_gc
		       , 0
		       , 0
		       , pv_ptr->pv_width
		       , pv_ptr->pv_height
		       , GDK_RGB_DITHER_NORMAL
		       , src_data
		       , pv_ptr->pv_width * 3
		       );

    if( allow_grab_src_data )
    {
       /* the calling procedure has permitted to grab the src_data
        * for internal refresh usage.
        */
       g_free(pv_ptr->pv_area_data);
       pv_ptr->pv_area_data = src_data;
       return TRUE;
    }
    else
    {
      /* here we make a claen copy of src_data to pv_ptr->pv_area_data.
       *  beacause the data might be needed
       *  for refresh of the display (for an "expose" event handler procedure)
       */
       memcpy(pv_ptr->pv_area_data, src_data, pv_ptr->pv_height * pv_ptr->pv_width * pv_ptr->pv_bpp);
    }     
 
    return FALSE;
  }

  
  ofs_green = 1;
  ofs_blue  = 2;
  ofs_alpha = 3;
  if(src_bpp < 3)
  {
    ofs_green = 0;
    ofs_blue  = 0;
    ofs_alpha = 1;
  }


  if((src_bpp ==3) || (src_bpp == 1))
  {
    guchar   *src_row;
 
    /* Source is Opaque (has no alpha) draw preview row by row */
    dest = pv_ptr->pv_area_data;
    for ( row = 0; row < pv_ptr->pv_height; row++ )
    {
        src_row = &src_data[CLAMP(((row * src_height) / pv_ptr->pv_height)
                                  , 0
                                  , src_height)
                                  * pv_ptr->src_rowstride];
        for ( col = 0; col < pv_ptr->pv_width; col++ )
        {
            src = &src_row[ pv_ptr->src_col[col] ];
            *(dest++) = src[0];
            *(dest++) = src[ofs_green];
            *(dest++) = src[ofs_blue];
        }
    }
  }
  else
  {
    guchar   *src_row;
    guchar   alpha;
    gint     ii;

    /* Source has alpha, draw preview row by row
     * show checkboard as background for transparent pixels
     */
    dest = pv_ptr->pv_area_data;
    for ( row = 0; row < pv_ptr->pv_height; row++ )
    {
      
        if ((row / pv_ptr->pv_check_size) & 1) { ii = 0;}
        else                                   { ii = pv_ptr->pv_check_size; }
        
        src_row = &src_data[CLAMP(((row * src_height) / pv_ptr->pv_height)
                                  , 0
                                  , src_height)
                                  * pv_ptr->src_rowstride];
        for ( col = 0; col < pv_ptr->pv_width; col++ )
        {
          src = &src_row[ pv_ptr->src_col[col] ];
          alpha = src[ofs_alpha];
	  if(alpha > 244)
	  {
	    /* copy full (or nearly full opaque) pixels 1:1
	     * without MIXING with checkerboard background.
	     * (speeds up rendering of opaque pixels)
	     */
            *(dest++) = src[0];
            *(dest++) = src[ofs_green];
            *(dest++) = src[ofs_blue];
	  }
	  else
	  {
            if(((col+ii) / pv_ptr->pv_check_size) & 1)
            {
	      if(alpha < 10)
	      {
        	*(dest++) = PREVIEW_BG_GRAY1;
        	*(dest++) = PREVIEW_BG_GRAY1;
        	*(dest++) = PREVIEW_BG_GRAY1;
	      }
	      else
	      {
        	*(dest++) = MIX_CHANNEL (PREVIEW_BG_GRAY1, src[0], alpha);
        	*(dest++) = MIX_CHANNEL (PREVIEW_BG_GRAY1, src[ofs_green], alpha);
        	*(dest++) = MIX_CHANNEL (PREVIEW_BG_GRAY1, src[ofs_blue], alpha);
	      }
            }
            else
            {
	      if(alpha < 10)
	      {
        	*(dest++) = PREVIEW_BG_GRAY2;
        	*(dest++) = PREVIEW_BG_GRAY2;
        	*(dest++) = PREVIEW_BG_GRAY2;
	      }
	      else
	      {
                *(dest++) = MIX_CHANNEL (PREVIEW_BG_GRAY2, src[0], alpha);
                *(dest++) = MIX_CHANNEL (PREVIEW_BG_GRAY2, src[ofs_green], alpha);
                *(dest++) = MIX_CHANNEL (PREVIEW_BG_GRAY2, src[ofs_blue], alpha);
	      }
            }
	  }
        }
    }
  }

  gdk_draw_rgb_image ( pv_ptr->da_widget->window
		       , pv_ptr->da_widget->style->white_gc
		       , 0
		       , 0
		       , pv_ptr->pv_width
		       , pv_ptr->pv_height
		       , GDK_RGB_DITHER_NORMAL
		       , pv_ptr->pv_area_data
		       , pv_ptr->pv_width * 3
		       );

  return FALSE;

}       /* end gap_pview_render_from_buf */


/* ------------------------------
 * gap_pview_render_from_image
 * ------------------------------
 * render preview widget from image.
 * IMPORTANT: the image is scaled to preview size
 *            and the visible layers in the image are merged together !
 *
 * hint: call gtk_widget_queue_draw(pv_ptr->da_widget);
 * after this procedure to make the changes appear on screen.
 */
void
gap_pview_render_from_image (GapPView *pv_ptr, gint32 image_id)
{
  gint32 layer_id;
  guchar *frame_data;
  GimpPixelRgn pixel_rgn;
  GimpDrawable *drawable;
  

  gimp_image_scale(image_id, pv_ptr->pv_width, pv_ptr->pv_height);

  /* workaround: gimp_image_merge_visible_layers
   * needs at least 2 layers to work without complaining
   * therefore add 2 full transparent dummy layers
   */
  {
    gint32  l_layer_id;
    GimpImageBaseType l_type;

    l_type   = gimp_image_base_type(image_id);
    l_type   = (l_type * 2); /* convert from GimpImageBaseType to GimpImageType */

    l_layer_id = gimp_layer_new(image_id, "dummy_01"
                                , 4, 4
                                , l_type
                                , 0.0       /* Opacity full transparent */     
                                , 0         /* NORMAL */
                                );   
    gimp_image_add_layer(image_id, l_layer_id, 0);

    l_layer_id = gimp_layer_new(image_id, "dummy_02"
                                , 4, 4
                                , l_type
                                , 0.0       /* Opacity full transparent */     
                                , 0         /* NORMAL */
                                );   
    gimp_image_add_layer(image_id, l_layer_id, 0);
  }
  
    
  layer_id = gimp_image_merge_visible_layers (image_id, GIMP_CLIP_TO_IMAGE);
  drawable = gimp_drawable_get (layer_id);

  frame_data = g_malloc(drawable->width * drawable->height * drawable->bpp);
  
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0
                      , drawable->width, drawable->height
		      , FALSE     /* dirty */
		      , FALSE     /* shadow */
		       );
  gimp_pixel_rgn_get_rect (&pixel_rgn, frame_data
                          , 0
			  , 0
                          , drawable->width
                          , drawable->height);
                          
 
 {
    gboolean frame_data_was_grabbed;
    frame_data_was_grabbed = gap_pview_render_from_buf (pv_ptr
                   , frame_data
                   , drawable->width
                   , drawable->height
                   , drawable->bpp
                   , TRUE            /* allow_grab_src_data */
                 );
  
    if(!frame_data_was_grabbed) g_free(frame_data);
  }
  
}  /* end gap_pview_render_from_image */


/* ------------------------------
 * gap_pview_render_default_icon
 * ------------------------------
 */
void
gap_pview_render_default_icon(GapPView   *pv_ptr)
{
  GtkWidget *widget;
  int w, h;
  int x1, y1, x2, y2;
  GdkPoint poly[6];
  int foldh, foldw;
  int i;

  if(pv_ptr == NULL) { return; }
  if(pv_ptr->da_widget == NULL) { return; }
  if(pv_ptr->da_widget->window == NULL) { return; }

  widget = pv_ptr->da_widget;  /* the drawing area */

  /* set flag to let gap_pview_repaint procedure know
   * to use the pixmap rather than pv_area_data for refresh
   */
  pv_ptr->use_pixmap_repaint = TRUE;
  pv_ptr->use_pixbuf_repaint = FALSE;
  
  if(pv_ptr->pixmap)
  {
    gdk_drawable_get_size (pv_ptr->pixmap, &w, &h);
    if((w != pv_ptr->pv_width)
    || (h != pv_ptr->pv_height))
    {
       /* drop the old pixmap because of size missmatch */
       g_object_unref(pv_ptr->pixmap);
       pv_ptr->pixmap = NULL;
    }
  }

  w = pv_ptr->pv_width;
  h = pv_ptr->pv_height;

  if(pv_ptr->pixmap == NULL)
  {
       pv_ptr->pixmap = gdk_pixmap_new (widget->window
                          ,pv_ptr->pv_width
                          ,pv_ptr->pv_height
                          ,-1    /* use same depth as widget->window */
                          );

  }

  x1 = 2;
  y1 = h / 8 + 2;
  x2 = w - w / 8 - 2;
  y2 = h - 2;
  gdk_draw_rectangle (pv_ptr->pixmap, widget->style->bg_gc[GTK_STATE_NORMAL], 1,
                      0, 0, w, h);
/*
  gdk_draw_rectangle (pv_ptr->pixmap, widget->style->black_gc, 0,
                      x1, y1, (x2 - x1), (y2 - y1));
*/

  foldw = w / 4;
  foldh = h / 4;
  x1 = w / 8 + 2;
  y1 = 2;
  x2 = w - 2;
  y2 = h - h / 8 - 2;

  poly[0].x = x1 + foldw; poly[0].y = y1;
  poly[1].x = x1 + foldw; poly[1].y = y1 + foldh;
  poly[2].x = x1; poly[2].y = y1 + foldh;
  poly[3].x = x1; poly[3].y = y2;
  poly[4].x = x2; poly[4].y = y2;
  poly[5].x = x2; poly[5].y = y1;
  gdk_draw_polygon (pv_ptr->pixmap, widget->style->white_gc, 1, poly, 6);

  gdk_draw_line (pv_ptr->pixmap, widget->style->black_gc,
                 x1, y1 + foldh, x1, y2);
  gdk_draw_line (pv_ptr->pixmap, widget->style->black_gc,
                 x1, y2, x2, y2);
  gdk_draw_line (pv_ptr->pixmap, widget->style->black_gc,
                 x2, y2, x2, y1);
  gdk_draw_line (pv_ptr->pixmap, widget->style->black_gc,
                 x1 + foldw, y1, x2, y1);

  for (i = 0; i < foldw; i++)
  {
    gdk_draw_line (pv_ptr->pixmap, widget->style->black_gc,
                   x1 + i, y1 + foldh, x1 + i, (foldw == 1) ? y1 :
                   (y1 + (foldh - (foldh * i) / (foldw - 1))));
  }

  gdk_draw_drawable(widget->window
                   ,widget->style->black_gc
                   ,pv_ptr->pixmap
                   ,0
                   ,0
                   ,0
                   ,0
                   ,w
                   ,h
                   );

  
}  /* end gap_pview_render_default_icon */


#ifdef GAP_PVIEW_USE_GDK_PIXBUF_RENDERING

/* ------------------------------
 * gap_pview_render_from_pixbuf (slow)
 * ------------------------------
 * render drawing_area widget from src_pixbuf buffer
 * scaling and flattening against checkerboard background
 * is done implicite using GDK-pixbuf procedures
 *            
 * Thumbnails at size 128 rendered to Widget Size 256x256 pixels
 * at my Pentium IV 2600 MHZ
 * can be Played at Speed of  98 Frames/sec without dropping frames.
 *
 * The other Implementation without GDK-pixbuf procedures
 * is faster (at least on my machine), therefore GAP_PVIEW_USE_GDK_PIXBUF_RENDERING
 * is NOT defined per default.
 */
void
gap_pview_render_from_pixbuf (GapPView *pv_ptr, GdkPixbuf *src_pixbuf)
{
  static int l_checksize_tab[17] = { 2, 4, 8, 8, 16, 16, 16, 32, 32, 32, 32, 32, 32, 64, 64, 64, 64 };
  int l_check_size;

  /* printf("gap_pview_render_from_pixbuf --- USE GDK-PIXBUF procedures\n"); */
  
  if(pv_ptr == NULL) { return; }
  if(pv_ptr->da_widget == NULL) { return; }
  if(pv_ptr->da_widget->window == NULL)
  { 
    printf("gap_pview_render_from_pixbuf: drawing_area window pointer is NULL, cant render\n");
    return ;
  }

  if(src_pixbuf == NULL)
  {
    printf("gap_pview_render_from_pixbuf: src_pixbuf is NULL, cant render\n");
    return ;
  }

  /* clear flag to let gap_pview_repaint procedure know
   * to use the pixbuf rather than pv_area_data or pixmap for refresh
   */
  pv_ptr->use_pixmap_repaint = FALSE;
  pv_ptr->use_pixbuf_repaint = TRUE;

  /* l_check_size must be a power of 2 (using fixed size for 1.st test) */
  l_check_size = l_checksize_tab[MIN((pv_ptr->pv_check_size >> 2), 8)];
  if(pv_ptr->pixbuf)
  {
    /* free old (refresh) pixbuf if there is one */
    g_object_unref(pv_ptr->pixbuf);
  }
  
  /* scale and flatten the pixbuf */
  pv_ptr->pixbuf = gdk_pixbuf_composite_color_simple(
                	 src_pixbuf
                      , (int) pv_ptr->pv_width
                      , (int) pv_ptr->pv_height
                      , GDK_INTERP_NEAREST
                      , 255                          /* overall_alpha */
                      , (int)l_check_size            /* power of 2 required */
                      , PREVIEW_BG_GRAY1_GDK
                      , PREVIEW_BG_GRAY2_GDK
		      );
  if(gap_debug)
  {
    int nchannels;
    int rowstride;
    int width;
    int height;
    guchar *pix_data;
    gboolean has_alpha;

    width = gdk_pixbuf_get_width(pv_ptr->pixbuf);
    height = gdk_pixbuf_get_height(pv_ptr->pixbuf);
    nchannels = gdk_pixbuf_get_n_channels(pv_ptr->pixbuf);
    pix_data = gdk_pixbuf_get_pixels(pv_ptr->pixbuf);
    has_alpha = gdk_pixbuf_get_has_alpha(pv_ptr->pixbuf);
    rowstride = gdk_pixbuf_get_rowstride(pv_ptr->pixbuf);

    printf("gap_pview_render_from_pixbuf (AFTER SCALE/FLATTEN):\n");
    printf(" l_check_size: %d (%d)\n", (int)l_check_size, pv_ptr->pv_check_size);
    printf(" width: %d\n", (int)width );
    printf(" height: %d\n", (int)height );
    printf(" nchannels: %d\n", (int)nchannels );
    printf(" pix_data: %d\n", (int)pix_data );
    printf(" has_alpha: %d\n", (int)has_alpha );
    printf(" rowstride: %d\n", (int)rowstride );
  }

  gdk_draw_pixbuf(
                     pv_ptr->da_widget->window
		   , pv_ptr->da_widget->style->white_gc
                   , pv_ptr->pixbuf
                   , 0                         /*  gint src_x  */
                   , 0                         /*  gint src_y  */
                   , 0                         /*  gint dest_x */
                   , 0                         /*  gint dest_y */
                   , pv_ptr->pv_width
                   , pv_ptr->pv_height
                   , GDK_RGB_DITHER_NORMAL
                   , 0                         /* gint x_dither_offset */
                   , 0                         /* gint y_dither_offset */
		   );
}       /* end gap_pview_render_from_pixbuf */

#else

/* ------------------------------
 * gap_pview_render_from_pixbuf (fast)
 * ------------------------------
 * render drawing_area widget from src_pixbuf buffer.
 * 
 * scaling and flattening against checkerboard background
 * is done by calling gap_pview_render_from_buf.
 *
 * The scaling in gap_pview_render_from_buf is optimized for speed
 * especially when both src and dest sizes are the same as in the
 * previous call.
 *
 * 
 * Thumbnails at size 128 rendered to Widget Size 256x256 pixels
 * at my Pentium IV 2600 MHZ
 * can be Played at Speed of  136 Frames/sec without dropping frames
 *
 */
void
gap_pview_render_from_pixbuf (GapPView *pv_ptr, GdkPixbuf *src_pixbuf)
{
  /* printf("gap_pview_render_from_pixbuf >>NO<< USE OF GDK-PIXBUF procedures\n"); */

  if(src_pixbuf == NULL)
  {
    printf("gap_pview_render_from_pixbuf: src_pixbuf is NULL, cant render\n");
    return ;
  }
  else
  {
    int nchannels;
    int width;
    int height;
    guchar *pix_data;

    width = gdk_pixbuf_get_width(src_pixbuf);
    height = gdk_pixbuf_get_height(src_pixbuf);
    nchannels = gdk_pixbuf_get_n_channels(src_pixbuf);
    pix_data = gdk_pixbuf_get_pixels(src_pixbuf);
     
    gap_pview_render_from_buf(pv_ptr
                             , pix_data
			     , width
			     , height
			     , nchannels
			     , FALSE                             /* DONT allow_grab_src_data */
			     );
  }
  

}       /* end gap_pview_render_from_pixbuf */

#endif

