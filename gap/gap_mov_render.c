/* gap_mov_render.c
 * 1997.11.06 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * Render utility Procedures for GAP MovePath
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
 * gimp    1.3.20a; 2003/09/14  hof: fixed compiler warnings
 * gimp    1.3.14a; 2003/05/24  hof: created (splitted off from gap_mov_dialog)
 */

#include "config.h"

/* SYTEM (UNIX) includes */ 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

/* GIMP includes */
#include "gtk/gtk.h"
#include "gap-intl.h"
#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

/* GAP includes */
#include "gap_layer_copy.h"
#include "gap_lib.h"
#include "gap_mov_exec.h"
#include "gap_mov_dialog.h"
#include "gap_mov_render.h"
#include "gap_pdb_calls.h"
#include "gap_vin.h"
#include "gap_arr_dialog.h"

extern      int gap_debug; /* ==0  ... dont print debug infos */


#define BOUNDS(a,x,y)  ((a < x) ? x : ((a > y) ? y : a))

/* ============================================================================
 * p_mov_render
 * insert current source layer into image
 *    at current settings (position, size opacity ...)
 * ============================================================================
 */

  /*  why dont use: l_cp_layer_id = gimp_layer_copy(src_id);
   *  ==> Sorry this procedure works only for layers within the same image !!
   *  Workaround:
   *    use my 'private' version of layercopy
   */

gint
p_mov_render(gint32 image_id, t_mov_values *val_ptr, t_mov_current *cur_ptr)
{
  gint32       l_cp_layer_id;
  gint32       l_cp_layer_mask_id;
  gint         l_offset_x, l_offset_y;            /* new offsets within dest. image */
  gint         l_src_offset_x, l_src_offset_y;    /* layeroffsets as they were in src_image */
  guint        l_new_width;
  guint        l_new_height;
  guint        l_orig_width;
  guint        l_orig_height;
  gint         l_resized_flag;
  gboolean     l_interpolation;   
  gint          lx1, ly1, lx2, ly2;
  guint        l_image_width;
  guint        l_image_height;
 
  if(gap_debug) printf("p_mov_render: frame/layer: %ld/%ld  X=%f, Y=%f\n"
                "       Width=%f Height=%f\n"
                "       Opacity=%f  Rotate=%f  clip_to_img = %d force_visibility = %d\n"
                "       src_stepmode = %d\n",
                     cur_ptr->dst_frame_nr, cur_ptr->src_layer_idx,
                     cur_ptr->currX, cur_ptr->currY,
                     cur_ptr->currWidth,
                     cur_ptr->currHeight,
                     cur_ptr->currOpacity,
                     cur_ptr->currRotation,
                     val_ptr->clip_to_img,
                     val_ptr->src_force_visible,
		     val_ptr->src_stepmode);

  if(val_ptr->src_stepmode < GAP_STEP_FRAME)
  {
    if(gap_debug) 
    {
      printf("p_mov_render: Before p_my_layer_copy image_id:%d src_layer_id:%d\n"
              ,(int)image_id, (int)cur_ptr->src_layers[cur_ptr->src_layer_idx]);
    }
    /* make a copy of the current source layer
     * (using current opacity  & paintmode values)
     */
     l_cp_layer_id = p_my_layer_copy(image_id,
                                   cur_ptr->src_layers[cur_ptr->src_layer_idx],
                                   cur_ptr->currOpacity,
                                   val_ptr->src_paintmode,
                                   &l_src_offset_x,
                                   &l_src_offset_y);
  }
  else
  {
    if(gap_debug) 
    {
      printf("p_mov_render: Before p_my_layer_copy image_id:%d cache_tmp_layer_id:%d\n"
              ,(int)image_id, (int)val_ptr->cache_tmp_layer_id);
    }
     /* for FRAME based stepmodes use the flattened layer in the cahed frame image */
     l_cp_layer_id = p_my_layer_copy(image_id,
                                   val_ptr->cache_tmp_layer_id,
                                   cur_ptr->currOpacity,
                                   val_ptr->src_paintmode,
                                   &l_src_offset_x,
                                   &l_src_offset_y);
  }


  /* add the copied layer to current destination image */
  if(gap_debug) printf("p_mov_render: after layer copy layer_id=%d\n", (int)l_cp_layer_id);
  if(l_cp_layer_id < 0)
  {
     return -1;
  }  

  gimp_image_add_layer(image_id, l_cp_layer_id, 
                       val_ptr->dst_layerstack);

  if(gap_debug) printf("p_mov_render: after add layer\n");

  if(val_ptr->src_force_visible)
  {
     gimp_layer_set_visible(l_cp_layer_id, TRUE);
  }

  /* check for layermask */
  l_cp_layer_mask_id = gimp_layer_get_mask_id(l_cp_layer_id);
  if(l_cp_layer_mask_id >= 0)
  {
     /* apply the layermask
      *   some transitions (especially rotate) can't operate proper on
      *   layers with masks !
      *   (tests with gimp_rotate resulted in trashed images,
      *    even if the mask was rotated too)
      */
      gimp_image_remove_layer_mask(image_id, l_cp_layer_id, 0 /* 0==APPLY */ );
  }

  /* remove selection (if there is one)
   *   if there is a selection transitions (gimp_rotate)
   *   will create new layer and do not operate on l_cp_layer_id
   */
  gimp_selection_none(image_id);

  l_resized_flag = 0;
  l_orig_width  = gimp_drawable_width(l_cp_layer_id);
  l_orig_height = gimp_drawable_height(l_cp_layer_id);
  l_new_width  = l_orig_width;
  l_new_height = l_orig_height;
  
  if((cur_ptr->currWidth  > 100.01) || (cur_ptr->currWidth < 99.99)
  || (cur_ptr->currHeight > 100.01) || (cur_ptr->currHeight < 99.99))
  {
     /* have to scale layer */
     l_resized_flag = 1;

     l_new_width  = (l_orig_width  * cur_ptr->currWidth) / 100;
     l_new_height = (l_orig_height * cur_ptr->currHeight) / 100;
     gimp_layer_scale(l_cp_layer_id, l_new_width, l_new_height, 0);
     
  }

  if((cur_ptr->currRotation  > 0.5) || (cur_ptr->currRotation < -0.5))
  {
    l_resized_flag = 1;
    l_interpolation = TRUE;  /* rotate always with smoothing option turned on */

    /* have to rotate the layer (rotation also changes size as needed) */
    p_gimp_rotate_degree(l_cp_layer_id, l_interpolation, cur_ptr->currRotation);

    
    l_new_width  = gimp_drawable_width(l_cp_layer_id);
    l_new_height = gimp_drawable_height(l_cp_layer_id);
  }
  
  if(l_resized_flag == 1)
  {
     /* adjust offsets according to handle and change of size */
     switch(val_ptr->src_handle)
     {
        case GAP_HANDLE_LEFT_BOT:
           l_src_offset_y += ((gint)l_orig_height - (gint)l_new_height);
           break;
        case GAP_HANDLE_RIGHT_TOP:
           l_src_offset_x += ((gint)l_orig_width - (gint)l_new_width);
           break;
        case GAP_HANDLE_RIGHT_BOT:
           l_src_offset_x += ((gint)l_orig_width - (gint)l_new_width);
           l_src_offset_y += ((gint)l_orig_height - (gint)l_new_height);
           break;
        case GAP_HANDLE_CENTER:
           l_src_offset_x += (((gint)l_orig_width - (gint)l_new_width) / 2);
           l_src_offset_y += (((gint)l_orig_height - (gint)l_new_height) / 2);
           break;
        case GAP_HANDLE_LEFT_TOP:
        default:
           break;
     }
  }
  
  /* calculate offsets in destination image */
  l_offset_x = (cur_ptr->currX - cur_ptr->l_handleX) + l_src_offset_x;
  l_offset_y = (cur_ptr->currY - cur_ptr->l_handleY) + l_src_offset_y;
  
  /* modify coordinate offsets of the copied layer within dest. image */
  gimp_layer_set_offsets(l_cp_layer_id, l_offset_x, l_offset_y);

  /* clip the handled layer to image size if desired */
  if(val_ptr->clip_to_img != 0)
  {
     l_image_width = gimp_image_width(image_id);
     l_image_height = gimp_image_height(image_id);
     
     lx1 = BOUNDS (l_offset_x, 0, (gint32)l_image_width);
     ly1 = BOUNDS (l_offset_y, 0, (gint32)l_image_height);
     lx2 = BOUNDS ((gint32)(l_new_width + l_offset_x), 0, (gint32)l_image_width);
     ly2 = BOUNDS ((gint32)(l_new_height + l_offset_y), 0, (gint32)l_image_height);

     l_new_width = lx2 - lx1;
     l_new_height = ly2 - ly1;
     
     if (l_new_width && l_new_height)
     {
       gimp_layer_resize(l_cp_layer_id, l_new_width, l_new_height,
			-(lx1 - l_offset_x),
			-(ly1 - l_offset_y));
     }
     else
     {
       /* no part of the layer is inside of the current frame (this image)
        * instead of removing we make the layer small and move him outside
        * the image.
        * (that helps to keep the layerstack position of the inserted layer(s)
        * constant in all handled anim_frames) 
        */
       gimp_layer_resize(l_cp_layer_id, 2, 2, -3, -3);
     }
  }

  if(gap_debug) printf("GAP p_mov_render: exit OK\n");
  
  return 0;
}	/* end p_mov_render */


/* ============================================================================
 * p_get_flattened_layer
 *   flatten the given image and return pointer to the
 *   (only) remaining drawable. 
 * ============================================================================
 */
gint32
p_get_flattened_layer(gint32 image_id, GimpMergeType mergemode)
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
}	/* end p_get_flattened_layer */


/* ============================================================================
 * p_mov_fetch_src_frame
 *   fetch the requested AnimFrame SourceImage into cache_tmp_image_id
 *   and
 *    - reduce all visible layer to one layer (cache_tmp_layer_id)
 *    - (scale to animated preview size if called for AnimPreview )
 *    - reuse cached image (for subsequent calls for the same framenumber
 *      of the same source image -- for  GAP_STEP_FRAME_NONE
 *    - never load current frame number from diskfile (use duplicate of the src_image)
 *  returns 0 (OK) or -1 (on Errors)
 * ============================================================================
 */
gint
p_mov_fetch_src_frame(t_mov_values *pvals,  gint32 wanted_frame_nr)
{  
  t_anim_info  *l_ainfo_ptr;
  t_anim_info  *l_old_ainfo_ptr;

  if(gap_debug)
  {
     printf("p_mov_fetch_src_frame: START src_image_id: %d wanted_frame_nr:%d"
            " cache_src_image_id:%d cache_frame_number:%d\n"
            , (int)pvals->src_image_id
            , (int)wanted_frame_nr
            , (int)pvals->cache_src_image_id
            , (int)pvals->cache_frame_number
            );
  }
  
  if(pvals->src_image_id < 0)
  {
     return -1;  
  }
 
  if((pvals->src_image_id != pvals->cache_src_image_id)
  || (wanted_frame_nr != pvals->cache_frame_number))
  {
     if(pvals->cache_tmp_image_id >= 0)
     {
        if(gap_debug)
	{
	   printf("p_mov_fetch_src_frame: DELETE cache_tmp_image_id:%d\n",
	            (int)pvals->cache_tmp_image_id);
        }
        /* destroy the cached frame image */
        gimp_image_delete(pvals->cache_tmp_image_id);
	pvals->cache_tmp_image_id = -1;
     }

     l_ainfo_ptr =  p_alloc_ainfo(pvals->src_image_id, GIMP_RUN_NONINTERACTIVE);

     if(pvals->cache_ainfo_ptr == NULL)
     {
       pvals->cache_ainfo_ptr =  l_ainfo_ptr;
     }
     else
     {
        if ((pvals->src_image_id == pvals->cache_src_image_id)
	&&  (strcmp(pvals->cache_ainfo_ptr->basename, l_ainfo_ptr->basename) == 0))
	{
           pvals->cache_ainfo_ptr->curr_frame_nr =  l_ainfo_ptr->curr_frame_nr;
           p_free_ainfo(&l_ainfo_ptr);
	}
	else
	{           
           /* update cached ainfo  if source image has changed
	    * (either by id or by its basename)
	    */
           l_old_ainfo_ptr = pvals->cache_ainfo_ptr;
           pvals->cache_ainfo_ptr = l_ainfo_ptr;
           p_free_ainfo(&l_old_ainfo_ptr);
	}
     }
     
     if ((wanted_frame_nr == pvals->cache_ainfo_ptr->curr_frame_nr)
     ||  (wanted_frame_nr < 0))
     {
        /* always take the current source frame from the already opened image
	 * not only for speedup reasons. (the diskfile may contain non actual imagedata)
	 */
        pvals->cache_tmp_image_id = gimp_image_duplicate(pvals->src_image_id);
        wanted_frame_nr = pvals->cache_ainfo_ptr->curr_frame_nr;
     }
     else
     {
       /* build the source framename */
       if(pvals->cache_ainfo_ptr->new_filename != NULL)
       {
         g_free(pvals->cache_ainfo_ptr->new_filename);
       }
       pvals->cache_ainfo_ptr->new_filename = p_alloc_fname(pvals->cache_ainfo_ptr->basename,
                                	wanted_frame_nr,
                                	pvals->cache_ainfo_ptr->extension);
       if(pvals->cache_ainfo_ptr->new_filename == NULL)
       {
          printf("gap: error got no source frame filename\n");
          return -1;
       }

       /* load the wanted source frame */
       pvals->cache_tmp_image_id =  p_load_image(pvals->cache_ainfo_ptr->new_filename);
       if(pvals->cache_tmp_image_id < 0)
       {
          printf("gap: load error on src image %s\n", pvals->cache_ainfo_ptr->new_filename);
          return -1;
       }

     }
     
     pvals->cache_tmp_layer_id = p_get_flattened_layer(pvals->cache_tmp_image_id, GIMP_EXPAND_AS_NECESSARY);
     

     /* check if we are generating an anim preview
      * where we must Scale (down) the src image to preview size
      */
     if ((pvals->apv_mlayer_image >= 0)
     &&  ((pvals->apv_scalex != 100.0) || (pvals->apv_scaley != 100.0)))
     {
       gint32      l_size_x, l_size_y;

       if(gap_debug)
       {
          printf("p_mov_fetch_src_frame: Scale for Animpreview apv_scalex %f apv_scaley %f\n"
                 , (float)pvals->apv_scalex, (float)pvals->apv_scaley );
       }

       l_size_x = (gimp_image_width(pvals->cache_tmp_image_id) * pvals->apv_scalex) / 100;
       l_size_y = (gimp_image_height(pvals->cache_tmp_image_id) * pvals->apv_scaley) / 100;
       gimp_image_scale(pvals->cache_tmp_image_id, l_size_x, l_size_y);
     }
         
     pvals->cache_src_image_id = pvals->src_image_id;
     pvals->cache_frame_number = wanted_frame_nr;
  }

  return 0; /* OK */    
}	/* end p_mov_fetch_src_frame */
