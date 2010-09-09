/* gap_gve_raw.c by Wolfgang Hofer (hof@gimp.org)
 *
 * (GAP ... GIMP Animation Plugins, now also known as GIMP Video)
 *    Routines for RAW video encoding
 *    useable for uncompressed AVI video.
 *    (GimpDrawable <--> Raw Bitmap Data)
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
 * version 1.2.2; 2003.04.18  hof: gap_gve_raw_YUV420P_drawable_encode get all ros from gimp pixel region with one call
 *                                 (need full buffersize but is faster than get row by row)
 * version 1.2.2; 2002.12.15  hof: created
 */




/* SYSTEM (UNIX) includes */
#include <stdio.h>
#include <string.h>

/* GIMP includes */
#include "gtk/gtk.h"
#include "libgimp/gimp.h"

/* GAP includes */
#include "gap_gve_raw.h"


/* the raw CODEC needs no extra LIB includes */



/*************************************************************
 *          TOOL FUNCTIONS                                   *
 *************************************************************/


/* ------------------------------------
 * gap_gve_raw_BGR_drawable_encode
 * ------------------------------------
 * Encode drawable to RAW Buffer (Bytesequence BGR)
 *
 */
guchar *
gap_gve_raw_BGR_drawable_encode(GimpDrawable *drawable, gint32 *RAW_size, gboolean vflip
                        ,guchar *app0_buffer, gint32 app0_length)
{
  GimpPixelRgn pixel_rgn;
  GimpImageType drawable_type;
  guchar *RAW_data;
  guchar *RAW_ptr;
  guchar *pixelrow_data;
  guint   l_row;
  gint32  l_idx;
  gint32  l_blue;
  gint32  l_green;
  gint32  l_red;
  gint32  l_rowstride;

  drawable_type = gimp_drawable_type (drawable->drawable_id);
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, drawable->width, drawable->height, FALSE, FALSE);

  l_rowstride = drawable->width * drawable->bpp;
  pixelrow_data = (guchar *)g_malloc0(l_rowstride);
  *RAW_size = drawable->width * drawable->height * 3;

  RAW_data = (guchar *)g_malloc0((drawable->width * drawable->height * 3)
           + app0_length);
  if(app0_buffer)
  {
    memcpy(RAW_data, app0_buffer, app0_length);
    *RAW_size += app0_length;
  }

  RAW_ptr = RAW_data + app0_length;
  l_red   = 0;
  l_green = 1;
  l_blue  = 2;
  if((drawable_type == GIMP_GRAY_IMAGE)
  || (drawable_type == GIMP_GRAYA_IMAGE))
  {
    l_green = 0;
    l_blue  = 0;
  }

  for(l_row = 0; l_row < drawable->height; l_row++)
  {
     gint32 l_src_row;

     if(vflip)  { l_src_row = (drawable->height - 1) - l_row; }
     else       { l_src_row = l_row;}

     gimp_pixel_rgn_get_rect (&pixel_rgn, pixelrow_data
                              , 0
                              , l_src_row
                              , drawable->width
                              , 1);
     for(l_idx=0;l_idx < l_rowstride; l_idx += drawable->bpp)
     {
       *(RAW_ptr++) = pixelrow_data[l_idx + l_blue];
       *(RAW_ptr++) = pixelrow_data[l_idx + l_green];
       *(RAW_ptr++) = pixelrow_data[l_idx + l_red];
     }
  }
  g_free(pixelrow_data);
  return(RAW_data);
}    /* end gap_gve_raw_BGR_drawable_encode */


/* ------------------------------------
 * gap_gve_raw_RGB_drawable_encode
 * ------------------------------------
 * Encode drawable to RAW Buffer (Bytesequence RGB)
 *
 */
guchar *
gap_gve_raw_RGB_drawable_encode(GimpDrawable *drawable, gint32 *RAW_size, gboolean vflip
                        ,guchar *app0_buffer, gint32 app0_length)
{
  GimpPixelRgn pixel_rgn;
  GimpImageType drawable_type;
  guchar *RAW_data;
  guchar *RAW_ptr;
  guchar *pixelrow_data;
  guint   l_row;
  gint32  l_idx;
  gint32  l_blue;
  gint32  l_green;
  gint32  l_red;
  gint32  l_rowstride;

  drawable_type = gimp_drawable_type (drawable->drawable_id);
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, drawable->width, drawable->height, FALSE, FALSE);

  l_rowstride = drawable->width * drawable->bpp;
  pixelrow_data = (guchar *)g_malloc0(l_rowstride);
  *RAW_size = drawable->width * drawable->height * 3;

  RAW_data = (guchar *)g_malloc0((drawable->width * drawable->height * 3)
           + app0_length);
  if(app0_buffer)
  {
    memcpy(RAW_data, app0_buffer, app0_length);
    *RAW_size += app0_length;
  }

  RAW_ptr = RAW_data + app0_length;
  l_red   = 0;
  l_green = 1;
  l_blue  = 2;
  if((drawable_type == GIMP_GRAY_IMAGE)
  || (drawable_type == GIMP_GRAYA_IMAGE))
  {
    l_green = 0;
    l_blue  = 0;
  }

  for(l_row = 0; l_row < drawable->height; l_row++)
  {
     gint32 l_src_row;

     if(vflip)  { l_src_row = (drawable->height - 1) - l_row; }
     else       { l_src_row = l_row;}

     gimp_pixel_rgn_get_rect (&pixel_rgn, pixelrow_data
                              , 0
                              , l_src_row
                              , drawable->width
                              , 1);
     for(l_idx=0;l_idx < l_rowstride; l_idx += drawable->bpp)
     {
       *(RAW_ptr++) = pixelrow_data[l_idx + l_red];
       *(RAW_ptr++) = pixelrow_data[l_idx + l_green];
       *(RAW_ptr++) = pixelrow_data[l_idx + l_blue];
     }
  }
  g_free(pixelrow_data);
  return(RAW_data);
}    /* end gap_gve_raw_RGB_drawable_encode */



/* ------------------------------------
 * gap_gve_raw_YUV444_drawable_encode
 * ------------------------------------
 * Encode drawable to RAW Buffer (YUV 4:4:4 colormodel)
 *
 * for image width =5 , height = 3 the returned buffer is filled
 * in the folloing Byte order:
 * YYYYY YYYYY YYYYY UUUUU UUUUU UUUUU VVVVV VVVVV VVVVV
 */
guchar *
gap_gve_raw_YUV444_drawable_encode(GimpDrawable *drawable, gint32 *RAW_size, gboolean vflip
                        ,guchar *app0_buffer, gint32 app0_length
                        ,gint32 matrix_coefficients
                        )
{
  guint j;
  int i;
  int r, g, b;
  double y, u, v;
  double cr, cg, cb, cu, cv;
  unsigned char *yp, *up, *vp;
  static unsigned char *y444, *u444, *v444;
  static double coef[7][3] = {
    {0.2125,0.7154,0.0721}, /* ITU-R Rec. 709 (1990) */
    {0.299, 0.587, 0.114},  /* unspecified */
    {0.299, 0.587, 0.114},  /* reserved */
    {0.30,  0.59,  0.11},   /* FCC */
    {0.299, 0.587, 0.114},  /* ITU-R Rec. 624-4 System B, G */
    {0.299, 0.587, 0.114},  /* SMPTE 170M */
    {0.212, 0.701, 0.087}}; /* SMPTE 240M (1987) */
  GimpPixelRgn srcPR;

  gint32  l_rowstride;
  guchar *pixelrow_data;
  guchar *RAW_data;
  guchar *RAW_ptr;
  GimpImageType drawable_type;
  gint32  l_blue;
  gint32  l_green;
  gint32  l_red;
  guint   l_row;


  drawable_type = gimp_drawable_type (drawable->drawable_id);
  l_rowstride = drawable->width * drawable->bpp;
  pixelrow_data = (guchar *)g_malloc0(l_rowstride);
  *RAW_size = drawable->width * drawable->height * 3;

  RAW_data = (guchar *)g_malloc0((drawable->width * drawable->height * 3)
           + app0_length);
  if(app0_buffer)
  {
    memcpy(RAW_data, app0_buffer, app0_length);
    *RAW_size += app0_length;
  }

  RAW_ptr = RAW_data + app0_length;
  l_red   = 0;
  l_green = 1;
  l_blue  = 2;
  if((drawable_type == GIMP_GRAY_IMAGE)
  || (drawable_type == GIMP_GRAYA_IMAGE))
  {
    l_green = 0;
    l_blue  = 0;
  }


  i = matrix_coefficients;
  if ((i>8) || (i<0))
  {
    i = 3;
  }

  cr = coef[i-1][0];
  cg = coef[i-1][1];
  cb = coef[i-1][2];
  cu = 0.5/(1.0-cb);
  cv = 0.5/(1.0-cr);

  y444 = RAW_ptr;
  u444 = RAW_ptr + (drawable->width * drawable->height);
  v444 = RAW_ptr + ((drawable->width * drawable->height) * 2 );


  /* get drawable */
  gimp_pixel_rgn_init (&srcPR, drawable, 0, 0, drawable->width, drawable->height,
                         FALSE, FALSE);

  /* copy imagedata to yuv rowpointers */
  for(l_row = 0; l_row < drawable->height; l_row++)
  {
     gint32   l_row_x_width;
     guchar *l_bptr;
     gint32 l_src_row;

     if(vflip)  { l_src_row = (drawable->height - 1) - l_row; }
     else       { l_src_row = l_row;}

     l_row_x_width = l_row * drawable->width;
     yp = y444 + l_row_x_width;
     up = u444 + l_row_x_width;
     vp = v444 + l_row_x_width;

    gimp_pixel_rgn_get_row (&srcPR, pixelrow_data, 0, l_src_row, drawable->width);

    l_bptr = pixelrow_data;
    for (j=0; j < drawable->width; j++)
    {
      /* peek RGB (for grey images: R==G==B) */
      r=l_bptr[l_red];
      g=l_bptr[l_green];
      b=l_bptr[l_blue];

      /* convert to YUV */
      y = cr*r + cg*g + cb*b;
      u = cu*(b-y);
      v = cv*(r-y);
      yp[j] = (219.0/256.0)*y + 16.5;  /* nominal range: 16..235 */
      up[j] = (224.0/256.0)*u + 128.5; /* 16..240 */
      vp[j] = (224.0/256.0)*v + 128.5; /* 16..240 */

      l_bptr += drawable->bpp;   /* advance read pointer */
    }
  }

  g_free(pixelrow_data);
  return(RAW_data);
}    /* end gap_gve_raw_YUV444_drawable_encode */



/* ------------------------------
 * gap_gve_raw_YUV420P_drawable_encode
 * ------------------------------
 * - convert Gimp drawable to YUV420P colormodel
 *   suitable for FFMEG Codec input.
 * - both width and height must be a multiple of 2
 * - output is written to yuv420_buffer (without size checks)
 *   the caller must provide this buffer large enough
 *   (width * height * 1.5)
 *
 *   ouput byte sequence example for image width = 6, height = 4
 *   YYYYYY YYYYYY YYYYYY YYYYYY UUU UUU VVV VVV
 *
 */

#define SCALEBITS 8
#define ONE_HALF  (1 << (SCALEBITS - 1))
#define FIX(x)          ((int) ((x) * (1L<<SCALEBITS) + 0.5))

void
gap_gve_raw_YUV420P_drawable_encode(GimpDrawable *drawable, guchar *yuv420_buffer)
{
  guint x, y;
  int wrap, wrap3;
  int r, g, b, r1, g1, b1;
  guchar *p;
  guchar *lum;
  guchar *cb;
  guchar *cr;
  GimpImageType drawable_type;
  GimpPixelRgn srcPR;
  gint32  l_blue;
  gint32  l_green;
  gint32  l_red;
  guchar *all_pixelrows;
  int     size;

  drawable_type = gimp_drawable_type (drawable->drawable_id);
  l_red   = 0;
  l_green = 1;
  l_blue  = 2;
  if((drawable_type == GIMP_GRAY_IMAGE)
  || (drawable_type == GIMP_GRAYA_IMAGE))
  {
    l_red   = 0;
    l_green = 0;
    l_blue  = 0;
  }

  /* init Pixel Region */
  gimp_pixel_rgn_init (&srcPR, drawable, 0, 0, drawable->width, drawable->height,
                         FALSE, FALSE);

  wrap = drawable->width;
  wrap3 = drawable->width * drawable->bpp;
  size = drawable->width * drawable->height;
  lum = yuv420_buffer;
  cb = yuv420_buffer + size;
  cr = cb + (size / 4);

    /* buffer for 2 pixelrows (RGB, RGBA, GREY, or GRAYA Gimp Colormodel) */
    all_pixelrows = g_malloc(wrap3 * drawable->height);

    gimp_pixel_rgn_get_rect (&srcPR, all_pixelrows
                              , 0
                              , 0
                              , drawable->width
                              , drawable->height               /* get all rows */
                              );

    for(y=0;y < drawable->height;y+=2)
    {
        p = all_pixelrows + (y * wrap3);
        for(x=0;x < drawable->width;x+=2)
        {
            r = p[l_red];
            g = p[l_green];
            b = p[l_blue];
            r1 = r;
            g1 = g;
            b1 = b;
            lum[0] = (FIX(0.29900) * r + FIX(0.58700) * g +
                      FIX(0.11400) * b + ONE_HALF) >> SCALEBITS;
            r = p[drawable->bpp + l_red];
            g = p[drawable->bpp + l_green];
            b = p[drawable->bpp + l_blue];
            r1 += r;
            g1 += g;
            b1 += b;
            lum[1] = (FIX(0.29900) * r + FIX(0.58700) * g +
                      FIX(0.11400) * b + ONE_HALF) >> SCALEBITS;

            /* step to next row (same column) */
            p += wrap3;
            lum += wrap;

            r = p[l_red];
            g = p[l_green];
            b = p[l_blue];
            r1 += r;
            g1 += g;
            b1 += b;
            lum[0] = (FIX(0.29900) * r + FIX(0.58700) * g +
                      FIX(0.11400) * b + ONE_HALF) >> SCALEBITS;
            r = p[drawable->bpp + l_red];
            g = p[drawable->bpp + l_green];
            b = p[drawable->bpp + l_blue];
            r1 += r;
            g1 += g;
            b1 += b;
            lum[1] = (FIX(0.29900) * r + FIX(0.58700) * g +
                      FIX(0.11400) * b + ONE_HALF) >> SCALEBITS;

            cb[0] = ((- FIX(0.16874) * r1 - FIX(0.33126) * g1 +
                      FIX(0.50000) * b1 + 4 * ONE_HALF - 1) >> (SCALEBITS + 2)) + 128;
            cr[0] = ((FIX(0.50000) * r1 - FIX(0.41869) * g1 -
                     FIX(0.08131) * b1 + 4 * ONE_HALF - 1) >> (SCALEBITS + 2)) + 128;

            cb++;
            cr++;

            /* backstep to current row and advance 2 column */
            p += -wrap3 + 2 * drawable->bpp;
            lum += -wrap + 2;
        }
        lum += wrap;
    }

    g_free(all_pixelrows);

}   /* end gap_gve_raw_YUV420P_drawable_encode */

/* ------------------------------------
 * gap_gve_raw_YUV420_drawable_encode
 * ------------------------------------
 * Encode drawable to RAW Buffer (YUV420 encoded)
 *
 */
guchar *
gap_gve_raw_YUV420_drawable_encode(GimpDrawable *drawable, gint32 *RAW_size, gboolean vflip
                        ,guchar *app0_buffer, gint32 app0_length)
{
  guchar *RAW_data;
  guchar *RAW_ptr;


  *RAW_size = drawable->width * drawable->height * 3;

  RAW_data = (guchar *)g_malloc0((drawable->width * drawable->height * 3)
           + app0_length);
  if(app0_buffer)
  {
    memcpy(RAW_data, app0_buffer, app0_length);
    *RAW_size += app0_length;
  }
  RAW_ptr = RAW_data + app0_length;

  gap_gve_raw_YUV420P_drawable_encode(drawable, RAW_ptr);

  return(RAW_data);
}    /* end gap_gve_raw_YUV420_drawable_encode */
