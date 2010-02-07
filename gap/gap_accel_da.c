/*  gap_accel_da.c
 *
 *  This module handles GAP acceleration characteristics drawing area.
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
 * version 2.7.0; 2010/02/06  hof: created
 */

#include "config.h"

#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include "gap_accel_char.h"
#include "gap_accel_da.h"
#include "gap-intl.h"
#include "gap_lib_common_defs.h"


extern int gap_debug;  /* 1 == print debug infos , 0 dont print debug infos */


#define MIX_CHANNEL(b, a, m)  (((a * m) + (b * (255 - m))) / 255)
#define PREVIEW_BG_GRAY1 80
#define PREVIEW_BG_GRAY2 180

#define PREVIEW_BG_GRAY1_GDK 0x505050
#define PREVIEW_BG_GRAY2_GDK 0xb4b4b4


#define GRAPH_RADIUS           3


static gint  gap_accel_repaint (GtkWidget *, GdkEvent *,
                                GapAccelWidget *);

/* ------------------------------
 * gap_pview_new
 * ------------------------------
 * pv_check_size is checkboard size in pixels
 * (for transparent pixels)
 */
GapAccelWidget *
gap_accel_new(gint width, gint height, gint accelerationCharacteristic)
{
  GapAccelWidget *accel_ptr;
  
 
  accel_ptr = g_malloc0(sizeof(GapAccelWidget));

  accel_ptr->accelerationCharacteristic = accelerationCharacteristic;
  accel_ptr->width = width;
  accel_ptr->height = height;
  accel_ptr->pixWidth = accel_ptr->width + GRAPH_RADIUS * 4;
  accel_ptr->pixHeight = accel_ptr->height+ GRAPH_RADIUS * 4;


  accel_ptr->da_widget = gtk_drawing_area_new ();
  gtk_widget_set_size_request (accel_ptr->da_widget
                              , accel_ptr->pixWidth
                              , accel_ptr->pixHeight
                              );

  gtk_widget_set_events (accel_ptr->da_widget, GDK_EXPOSURE_MASK);

/*
  gtk_container_add (GTK_CONTAINER (abox), accel_ptr->da_widget);
  gtk_widget_show (accel_ptr->da_widget);
*/

  g_signal_connect (accel_ptr->da_widget, "event",
                    G_CALLBACK (gap_accel_repaint),
                    accel_ptr);




  return(accel_ptr);
}  /* end gap_pview_new */



/* ------------------------------
 * gap_accel_render
 * ------------------------------
 */
void
gap_accel_render (GapAccelWidget *accel_ptr, gint accelerationCharacteristic)
{
  accel_ptr->accelerationCharacteristic = accelerationCharacteristic;
  gap_accel_repaint(NULL, NULL, accel_ptr);
}

/* ------------------------------
 * gap_accel_repaint
 * ------------------------------
 */
static gint
gap_accel_repaint(GtkWidget *wgt, GdkEvent *evt,
  GapAccelWidget *accel_ptr)
{
  GtkStyle *graph_style;
  gint      i;

  if(accel_ptr->da_widget == NULL)
  {
     if(gap_debug)
     {
       printf("gap_accel_repaint: drawing area graph is NULL (render request ignored\n");
     }
     return (FALSE);
  }
  if(accel_ptr->da_widget->window == NULL)
  {
     if(gap_debug)
     {
       printf("gap_accel_repaint: window pointer is NULL (render request ignored\n");
     }
     return (FALSE);
  }

  
  graph_style = gtk_widget_get_style (accel_ptr->da_widget);
  
  
  /*  Clear the pixmap  */
  gdk_draw_rectangle (accel_ptr->da_widget->window, graph_style->bg_gc[GTK_STATE_NORMAL],
                      TRUE, 0, 0, accel_ptr->pixWidth, accel_ptr->pixHeight);


  /*  Draw the grid lines (or just outline if acceleration is not active */
  for (i = 0; i < 5; i++)
  {
      if ((i == 0) || (i==4) || (accel_ptr->accelerationCharacteristic != 0))
      {
        gdouble xf;
        gdouble yf;
        gint    xi;
        gint    yi;
        
        xf = (gdouble)i * ((gdouble)accel_ptr->width / 4.0);
        yf = (gdouble)i * ((gdouble)accel_ptr->height / 4.0);
        xi = xf;
        yi = yf;
        
        gdk_draw_line (accel_ptr->da_widget->window, graph_style->dark_gc[GTK_STATE_NORMAL],
                     GRAPH_RADIUS, yi + GRAPH_RADIUS,
                     accel_ptr->width + GRAPH_RADIUS, yi + GRAPH_RADIUS);
        gdk_draw_line (accel_ptr->da_widget->window, graph_style->dark_gc[GTK_STATE_NORMAL],
                     xi + GRAPH_RADIUS, GRAPH_RADIUS,
                     xi + GRAPH_RADIUS, accel_ptr->height + GRAPH_RADIUS);

      }
  }

 
  /*  Draw the the acceleration curve according to accelerationCharacteristic
   *  when acceleration is active 
   */
  if(accel_ptr->accelerationCharacteristic != 0)
  {
       gdouble xFactor;
       gdouble yFactor;
       gdouble yF;
       
       /*  Draw the active curve  */
       for (i = 0; i < accel_ptr->width; i++)
       {
           gint cx, cy, px, py;
         
           xFactor = (gdouble)i / (MAX(1.0, (gdouble)accel_ptr->width));
           yFactor = gap_accelMixFactor(xFactor,  accel_ptr->accelerationCharacteristic);
           yF = (gdouble)accel_ptr->height * yFactor;


           cx = i + GRAPH_RADIUS;
           cy = (accel_ptr->height) - yF  + GRAPH_RADIUS;
           
           if(gap_debug)
           {
             printf("point[%03d] x:%04d y:%04d  xf:%f yf:%f\n"
               ,(int)i
               ,(int)cx
               ,(int)cy
               ,(float)xFactor
               ,(float)yFactor
               );
           }
           if(i>0)
           {
              gdk_draw_line (accel_ptr->da_widget->window, graph_style->black_gc,
                     px, py,
                     cx, cy
                     );
           }
           px = cx;
           py = cy;
       }
  }

  return FALSE;

}  /* end gap_accel_repaint */
