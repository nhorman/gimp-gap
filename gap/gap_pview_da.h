/*  gap_pview_da.h
 *
 *  This module handles GAP preview rendering from thumbnail data buffer
 *  used for video playback preview
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
 * version 1.3.16a; 2003/06/27  hof: use aspect_frame instead of simple frame, added p_pview_render_default_icon
 * version 1.3.14c; 2003/06/12  hof: created
 */

#ifndef _GAP_PVIEW_H
#define _GAP_PVIEW_H

#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>


typedef struct t_pview
{
  GtkWidget *da_widget;       /* the preview drawing_area */
  GtkWidget *aspect_frame;
  gint   *src_col;            /* Pointer array[pv_width] used for width scaling */
  gint    src_width;          /* width of last handled src buffer */
  gint    src_bpp;            /* byte per pixel of last handled src buffer */
  gint    src_rowstride;
  gint    pv_width;           /* Preview Width in pixels */
  gint    pv_height;          /* Preview Height in pixels */
  gint    pv_bpp;             /* BPP of the preview currently always 3 */
  gint    pv_check_size;      /* size of the cheks in pixels */
  gboolean use_pixmap_repaint;
  guchar *pv_area_data;       /* buffer to hold RGB image in preview size */
  GdkPixmap *pixmap;          /* alternative pixmap buffer */
} t_pview;


gboolean   p_pview_render_from_buf (t_pview *pv_ptr
                 , guchar *src_data
                 , gint    src_width
                 , gint    src_height
                 , gint    src_bpp
                 , gboolean allow_grab_src_data
                 );
void       p_pview_render_from_image (t_pview *pv_ptr, gint32 image_id);
void       p_pview_repaint(t_pview *pv_ptr);

t_pview   *p_pview_new(gint pv_width, gint pv_height, gint pv_check_size, GtkWidget *frame);
void       p_pview_set_size(t_pview *pv_ptr, gint pv_width, gint pv_height, gint pv_check_size);
void       p_pview_reset(t_pview *pv_ptr);

void       p_pview_render_default_icon(t_pview   *pv_ptr);

#endif
