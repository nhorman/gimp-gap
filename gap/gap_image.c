/* gap_image.c   procedures
 * 2003.10.09 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * This Module contains Image specific Procedures
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
 * version 1.3.20d; 2003.10.14   hof: created
 */

/* SYTEM (UNIX) includes */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include <gap_image.h>

extern int gap_debug;


/* ============================================================================
 * gap_image_delete_immediate
 *    delete image (with workaround to ensure that most of the
 *    allocatd memory is freed)
 * ============================================================================
 */
void
gap_image_delete_immediate (gint32 image_id)
{
    if(gap_debug) printf("gap_image_delete_immediate: SCALED down to 2x2 id = %d (workaround for gimp_image-delete problem)\n", (int)image_id);

    gimp_image_undo_disable(image_id);

    gimp_image_scale(image_id, 2, 2);

    gimp_image_undo_enable(image_id); /* clear undo stack */
    gimp_image_delete(image_id);
}       /* end  gap_image_delete_immediate */


/* ============================================================================
 * gap_image_merge_visible_layers
 *    merge visible layer an return layer_id of the resulting merged layer.
 *    (with workaround, for empty images return transparent layer)
 * ============================================================================
 */
gint32
gap_image_merge_visible_layers(gint32 image_id, GimpMergeType mergemode)
{
  GimpImageBaseType l_type;
  guint   l_width, l_height;
  gint32  l_layer_id;

  /* get info about the image */
  l_width  = gimp_image_width(image_id);
  l_height = gimp_image_height(image_id);
  l_type   = gimp_image_base_type(image_id);

  l_type   = (l_type * 2); /* convert from GimpImageBaseType to GimpImageType */

  /* add 2 full transparent dummy layers at top
   * (because gimp_image_merge_visible_layers complains
   * if there are less than 2 visible layers)
   */
  l_layer_id = gimp_layer_new(image_id, "dummy",
                                 l_width, l_height,  l_type,
                                 0.0,       /* Opacity full transparent */
                                 0);        /* NORMAL */
  gimp_image_add_layer(image_id, l_layer_id, 0);

  l_layer_id = gimp_layer_new(image_id, "dummy",
                                 10, 10,  l_type,
                                 0.0,       /* Opacity full transparent */
                                 0);        /* NORMAL */
  gimp_image_add_layer(image_id, l_layer_id, 0);

  return gimp_image_merge_visible_layers (image_id, mergemode);
}       /* end gap_image_merge_visible_layers */


/* ============================================================================
 * gap_image_prevent_empty_image
 *    check if the resulting image has at least one layer
 *    (gimp 1.0.0 tends to crash on layerless images)
 * ============================================================================
 */

void gap_image_prevent_empty_image(gint32 image_id)
{
  GimpImageBaseType l_type;
  guint   l_width, l_height;
  gint32  l_layer_id;
  gint    l_nlayers;
  gint32 *l_layers_list;

  l_layers_list = gimp_image_get_layers(image_id, &l_nlayers);
  if(l_layers_list != NULL)
  {
     g_free (l_layers_list);
  }
  else l_nlayers = 0;
  
  if(l_nlayers == 0)
  {
     /* the resulting image has no layer, add a transparent dummy layer */

     /* get info about the image */
     l_width  = gimp_image_width(image_id);
     l_height = gimp_image_height(image_id);
     l_type   = gimp_image_base_type(image_id);

     l_type   = (l_type * 2); /* convert from GimpImageBaseType to GimpImageType */

     /* add a transparent dummy layer */
     l_layer_id = gimp_layer_new(image_id, "dummy",
                                    l_width, l_height,  l_type,
                                    0.0,       /* Opacity full transparent */     
                                    0);        /* NORMAL */
     gimp_image_add_layer(image_id, l_layer_id, 0);
  }

}	/* end gap_image_prevent_empty_image */



/* ============================================================================
 * gap_image_new_with_layer_of_samesize
 * ============================================================================
 * create empty image 
 *  if layer_id is NOT NULL then create one full transparent layer at full image size
 *  and return the layer_id
 */
gint32  
gap_image_new_with_layer_of_samesize(gint32 old_image_id, gint32 *layer_id)
{
  GimpImageBaseType  l_type;
  guint       l_width;
  guint       l_height;
  gint32      new_image_id;
  
  /* create empty image  */
  l_width  = gimp_image_width(old_image_id);
  l_height = gimp_image_height(old_image_id);
  l_type   = gimp_image_base_type(old_image_id);

  new_image_id = gimp_image_new(l_width, l_height,l_type);
  
  if(layer_id)
  {
    l_type   = (l_type * 2); /* convert from GimpImageBaseType to GimpImageType */
    *layer_id = gimp_layer_new(new_image_id, "dummy",
                                 l_width, l_height,  l_type,
                                 0.0,       /* Opacity full transparent */     
                                 0);        /* NORMAL */
    gimp_image_add_layer(new_image_id, *layer_id, 0);
  }
  
  return (new_image_id);
  
}  /* end gap_image_new_with_layer_of_samesize */

gint32 
gap_image_new_of_samesize(gint32 old_image_id)
{
  return(gap_image_new_with_layer_of_samesize(old_image_id, NULL));
}


 
/* ------------------------------------
 * gap_image_is_alive
 * ------------------------------------
 * TODO: gimp 1.3.x sometimes keeps a copy of closed images
 *       therefore this proceedure may tell only half the truth
 *
 * return TRUE  if OK (image is still valid)
 * return FALSE if image is NOT valid
 */
gboolean
gap_image_is_alive(gint32 image_id)
{
  gint32 *images;
  gint    nimages;
  gint    l_idi;
  gint    l_found;

  if(image_id < 0)
  {
     return FALSE;
  }

  images = gimp_image_list(&nimages);
  l_idi = nimages -1;
  l_found = FALSE;
  while((l_idi >= 0) && images)
  {
    if(image_id == images[l_idi])
    {
          l_found = TRUE;
          break;
    }
    l_idi--;
  }
  if(images) g_free(images);
  if(l_found)
  {
    return TRUE;  /* OK */
  }

  if(gap_debug) printf("gap_image_is_alive: image_id %d is not VALID\n", (int)image_id);
 
  return FALSE ;   /* INVALID image id */
}  /* end gap_image_is_alive */
 
 
/* ------------------------------------
 * gap_image_get_any_layer
 * ------------------------------------
 * return the id of the active layer 
 *   or the id of the first layer found in the image if there is no active layer
 *   or -1 if the image has no layer at all.
 */
gint32
gap_image_get_any_layer(gint32 image_id)
{
  gint32  l_layer_id;
  gint    l_nlayers;
  gint32 *l_layers_list;

  l_layer_id = gimp_image_get_active_layer(image_id);
  if(l_layer_id < 0)
  {
    l_layers_list = gimp_image_get_layers(image_id, &l_nlayers);
    if(l_layers_list != NULL)
    {
       l_layer_id = l_layers_list[0];
       g_free (l_layers_list);
    }
  }
  return (l_layer_id);
}  /* end gap_image_get_any_layer */
