/* gap_onion_worker.c   procedures
 * 2003.05.22 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * This Module contains GAP Onionskin Worker Procedures
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
 * version 1.3.14a; 2003.05.24   hof: integration into gimp-gap-1.3.14
 * version 1.2.2a;  2001.11.24   hof: created
 */

#include <gap_onion_main.h>
extern int gap_debug;


/* ============================================================================
 * p_mark_as_onionlayer
 *   set onion layer parasite (store timestamp and tattoo)
 *   tattoos are unique identifiers within an image
 *   and remain unique over sessions -- if saved in xcf format.
 * ============================================================================
 */
static void
p_mark_as_onionlayer(gint32 layer_id)
{
  t_onion_parasite_data *l_parasite_data;
  GimpParasite          *l_parasite;

  if(gap_debug) printf("p_mark_as_onionlayer: START\n");

  l_parasite_data = g_malloc(sizeof(t_onion_parasite_data));
  l_parasite_data->timestamp = time(0);
  l_parasite_data->tattoo = gimp_layer_get_tattoo(layer_id);
  if(gap_debug) printf("p_mark_as_onionlayer: tattoo is: %d\n", (int)l_parasite_data->tattoo);

  l_parasite = gimp_parasite_new(GAP_ONION_PARASITE_NAME,
                                 GIMP_PARASITE_PERSISTENT,
                                 sizeof(t_onion_parasite_data),
                                 l_parasite_data);
  if(l_parasite)
  {
    gimp_drawable_parasite_attach(layer_id, l_parasite);
    gimp_parasite_free(l_parasite);
  }
}       /* end p_mark_as_onionlayer */


/* ============================================================================
 * p_check_is_onion_layer
 *   check for onion layer parasite and tattoo.
 *   (if the user made a copy of an onion layer, the copy has an onionparasite
 *    but the originals parasitedata.tattoo will not match with the layers tattoo.
 *    therefore the copy is not identified as onion layer.)
 * returns TRUE if layer is an original onion layer.
 * ============================================================================
 */
static gint32
p_check_is_onion_layer(gint32 layer_id)
{
   gint           l_found;
   t_onion_parasite_data *l_parasite_data;
   GimpParasite  *l_parasite;

   if(gap_debug) printf("p_check_is_onion_layer: START layer_id %d\n", (int)layer_id);

   l_found = FALSE;
   l_parasite = gimp_drawable_parasite_find(layer_id, GAP_ONION_PARASITE_NAME);
   if (l_parasite)
   {
      l_parasite_data = (t_onion_parasite_data *)l_parasite->data;
      if(gap_debug) printf("p_check_is_onion_layer: tattoo is: %d\n", (int)l_parasite_data->tattoo);

      if (l_parasite_data->tattoo == gimp_layer_get_tattoo(layer_id))
      {
        l_found = TRUE;
        if(gap_debug) printf("p_check_is_onion_layer: ONION_LAYER_FOUND layer_id %d\n", (int)layer_id);
      }
      gimp_parasite_free(l_parasite);
   }

   return l_found;
}       /* end p_check_is_onion_layer */



/* ============================================================================
 * p_delete_image_immediate
 *    delete image (with workaround to ensure that most of the
 *    allocatd memory is freed)
 * ============================================================================
 */
void
p_delete_image_immediate (gint32 image_id)
{
    if(gap_debug) printf("p_delete_image_immediate: SCALED down to 2x2 id = %d (workaround for gimp_image-delete problem)\n", (int)image_id);

    gimp_image_scale(image_id, 2, 2);

    gimp_image_undo_enable(image_id); /* clear undo stack */
    gimp_image_delete(image_id);
}       /* end  p_delete_image_immediate */



/* ============================================================================
 * p_find_frame_in_img_cache
 *    return delete image (with workaround to ensure that most of the
 *    return -1 if nothing was found.
 * ============================================================================
 */
gint32
p_find_frame_in_img_cache(t_global_params *gpp
                         , gint32 framenr, gint32 *image_id, gint32 *layer_id)
{
  gint32 l_idx;
  *image_id = -1;
  *layer_id = -1;

  for(l_idx = 0; l_idx < MIN(gpp->cache.count, GAP_ONION_CACHE_SIZE); l_idx++)
  {
    if(framenr == gpp->cache.framenr[l_idx])
    {
      *image_id = gpp->cache.image_id[l_idx];
      *layer_id = gpp->cache.layer_id[l_idx];
      return (l_idx);
    }
  }
  return (-1);
}       /* end p_find_frame_in_img_cache */

/* ============================================================================
 * p_add_img_to_cache
 *    add image_id and layer_id to the image cache.
 *    the image cache is a list of temporary images (without display)
 *    and contains frames that were processed in the previous steps
 *    of the onionskin layer processing.
 *    - a layer_id of -1 is used, if the image was loaded, but not
 *      merged.
 *    - If the framenr is already in the cache
 *      ??
 *    - If the cache is full, the oldest image_id is deleted
 *      and removed from the cache
 *    - Do not add the current image to the cache !!
 * ============================================================================
 */
void
p_add_img_to_cache (t_global_params *gpp, gint32 framenr, gint32 image_id, gint32 layer_id)
{
  gint32 l_idx;

  if(gap_debug)
  {
     printf("p_add_img_to_cache: cache.count: %d)\n", (int)gpp->cache.count);
     printf("PARAMS framenr: %d image_id: %d layer_id: %d\n"
           , (int)framenr
           , (int)image_id
           , (int)layer_id
           );
  }
  for(l_idx = 0; l_idx < MIN(gpp->cache.count, GAP_ONION_CACHE_SIZE); l_idx++)
  {
    if(framenr == gpp->cache.framenr[l_idx])
    {
      /* an image with this framenr is already in the cache */
      if(gap_debug) printf("p_add_img_to_cache: framenr already in cache at index: %d)\n", (int)l_idx);

      if( gpp->cache.layer_id[l_idx] >= 0)
      {
        /* the cached image has aready merged layers */
        return;
      }
      if(layer_id >= 0)
      {
         /* update the chache with the id of the merged layer */
         gpp->cache.image_id[l_idx] = image_id;
         gpp->cache.layer_id[l_idx] = layer_id;
      }
      return;
    }
  }
  if(l_idx < GAP_ONION_CACHE_SIZE)
  {
    gpp->cache.count++;
  }
  else
  {
    if(gap_debug)
    {
      printf("p_add_img_to_cache: FULL CACHE delete oldest entry\n");
    }
    /* cache is full, so delete 1.st (oldest) entry */
    p_delete_image_immediate(gpp->cache.image_id[0]);

    /* shift all entries down one index step */
    for(l_idx = 0; l_idx <  GAP_ONION_CACHE_SIZE -1; l_idx++)
    {
      gpp->cache.framenr[l_idx] = gpp->cache.framenr[l_idx +1];
      gpp->cache.image_id[l_idx] = gpp->cache.image_id[l_idx +1];
      gpp->cache.layer_id[l_idx] = gpp->cache.layer_id[l_idx +1];
    }
    /* l_idx = GAP_ONION_CACHE_SIZE -1; */
  }

  if(gap_debug)
  {
    printf("p_add_img_to_cache: new entry ADDED at IMAGECACHE index: %d\n", (int)l_idx);
  }
  gpp->cache.framenr[l_idx] = framenr;
  gpp->cache.image_id[l_idx] = image_id;
  gpp->cache.layer_id[l_idx] = layer_id;
}       /* end p_add_img_to_cache */



/* ============================================================================
 * p_delete_img_cache
 *    delete all images in the cache, and set the cache empty.
 * ============================================================================
 */
static void
p_delete_img_cache (t_global_params *gpp)
{
  gint32 l_idx;

  if(gap_debug) printf("p_delete_image_immediate: cache.count: %d)\n", (int)gpp->cache.count);
  for(l_idx = 0; l_idx < MIN(gpp->cache.count, GAP_ONION_CACHE_SIZE); l_idx++)
  {
    p_delete_image_immediate(gpp->cache.image_id[l_idx]);
  }
  gpp->cache.count = 0;
}       /* end p_delete_img_cache */


/* ============================================================================
 * p_plug_in_gap_get_animinfo
 *      get informations about the animation
 * ============================================================================
 */
void
p_plug_in_gap_get_animinfo(gint32 image_ID, t_ainfo *ainfo)
{
   static char     *l_called_proc = "plug_in_gap_get_animinfo";
   GimpParam       *return_vals;
   int              nreturn_vals;
   gint32           l_gap_feature_level;

   l_gap_feature_level = 0;
   return_vals = gimp_run_procedure (l_called_proc,
                                 &nreturn_vals,
                                 GIMP_PDB_INT32, GIMP_RUN_NONINTERACTIVE,
                                 GIMP_PDB_IMAGE, image_ID,
                                 GIMP_PDB_DRAWABLE, -1,
                                 GIMP_PDB_END);

   if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
   {
      ainfo->first_frame_nr = return_vals[1].data.d_int32;
      ainfo->last_frame_nr =  return_vals[2].data.d_int32;
      ainfo->curr_frame_nr =  return_vals[3].data.d_int32;
      ainfo->frame_cnt =      return_vals[4].data.d_int32;
      g_snprintf(ainfo->basename, sizeof(ainfo->basename), "%s", return_vals[5].data.d_string);
      g_snprintf(ainfo->extension, sizeof(ainfo->extension), "%s", return_vals[6].data.d_string);
      ainfo->framerate = return_vals[7].data.d_float;

      return;
   }

   printf("Error: PDB call of %s failed, image_ID: %d\n", l_called_proc, (int)image_ID);

}       /* end p_plug_in_gap_get_animinfo */


/* ============================================================================
 * p_set_data_onion_cfg
 *    Set onion config params (for one gimp session).
 * ============================================================================
 */
gint
p_set_data_onion_cfg(t_global_params *gpp, char *key)
{
  if(gap_debug) printf("p_set_data_onion_cfg: START\n");

  gimp_set_data(key, gpp, sizeof(t_global_params));

  return 0;
}       /* end p_set_onion_cfg */


/* ============================================================================
 * p_get_data_onion_cfg
 *    get onion config params (if there are any set in the current gimp session).
 * ============================================================================
 */
gint
p_get_data_onion_cfg(t_global_params *gpp)
{
  gint l_par_size;
  if(gap_debug) printf("p_get_data_onion_cfg: START\n");

  /* try to read configuration params */
  l_par_size = gimp_get_data_size(GAP_PLUGIN_NAME_ONION_CFG);
  if(l_par_size == sizeof(t_global_params))
  {
    if(gap_debug) printf("get config params: gimp_get_data_size %s OK\n", GAP_PLUGIN_NAME_ONION_CFG);

    gimp_get_data(GAP_PLUGIN_NAME_ONION_CFG, gpp);
  }
  else
  {
    if(gap_debug) printf("gimp_get_data_size FAILED for %s size:%d (using default params)\n"
                        , GAP_PLUGIN_NAME_ONION_CFG
                        , (int)l_par_size);
  }
  return 0;
}       /* end p_get_data_onion_cfg */

/* ============================================================================
 * p_get_merged_layer
 *    merge visible layer an return layer_id of the resulting merged layer.
 *    (with workaround, for empty images return transparent layer)
 * ============================================================================
 */
gint32
p_get_merged_layer(gint32 image_id, GimpMergeType mergemode)
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
}       /* end p_get_merged_layer */


/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
 */

/* ============================================================================
 * p_onion_visibility
 *    set visibility of all onion layer(s) in the current image.
 * ============================================================================
 */
gint
p_onion_visibility(t_global_params *gpp, gint visi_mode)
{
#define VISIBILTY_UNSET  -4444
  gint32     *l_layers_list;
  gint        l_nlayers;
  gint        l_idx;
  gint        l_is_onion;
  gint        l_visible;
  gint32      l_layer_id;

  if(gap_debug)
  {
     printf("p_onion_visibility: START visi_mode: %d\n", (int)visi_mode);
     printf("  image_ID: %d\n", (int)gpp->image_ID);
  }

  l_visible = VISIBILTY_UNSET;
  if(visi_mode == GAP_ONION_VISI_TRUE)
  {
    l_visible = TRUE;
  }
  if(visi_mode == GAP_ONION_VISI_FALSE)
  {
    l_visible = TRUE;
  }

  l_layers_list = gimp_image_get_layers(gpp->image_ID, &l_nlayers);
  if(l_layers_list)
  {
    for(l_idx=0;l_idx < l_nlayers;l_idx++)
    {
      l_layer_id = l_layers_list[l_idx];
      l_is_onion = p_check_is_onion_layer(l_layer_id);

      if(l_is_onion)
      {
        if (l_visible == VISIBILTY_UNSET)
        {
           if(gimp_layer_get_visible(l_layer_id)) { l_visible = FALSE; }
           else                                   { l_visible = TRUE; }
        }

        /* set visibility  */
        if(gap_debug) printf("layer_id %d  visibility: %d\n", (int)l_layer_id ,(int)l_visible);
        gimp_layer_set_visible(l_layer_id, l_visible);
      }
    }
    g_free(l_layers_list);
  }
  return 0;
}       /* end p_onion_visibility */



/* ============================================================================
 * p_onion_delete
 *    remove onion layer(s) from the current image.
 * ============================================================================
 */
gint
p_onion_delete(t_global_params *gpp)
{
  gint32     *l_layers_list;
  gint        l_nlayers;
  gint        l_idx;
  gint        l_is_onion;

  if(gap_debug)
  {
     printf("p_onion_delete: START\n");
     printf("  image_ID: %d\n", (int)gpp->image_ID);
  }

  l_layers_list = gimp_image_get_layers(gpp->image_ID, &l_nlayers);
  if(gap_debug) printf("p_onion_delete: l_nlayers = %d\n", (int)l_nlayers);
  if(l_layers_list)
  {
    for(l_idx=0;l_idx < l_nlayers;l_idx++)
    {
      if(gap_debug) printf("p_onion_delete: l_idx = %d\n", (int)l_idx);
      l_is_onion = p_check_is_onion_layer(l_layers_list[l_idx]);

      if(l_is_onion)
      {
        /* remove onion layer from source */
        gimp_image_remove_layer(gpp->image_ID, l_layers_list[l_idx]);
      }
    }
    g_free(l_layers_list);
  }
  if(gap_debug) printf("p_onion_delete: END\n");
  return 0;
}       /* end p_onion_delete */


/* ============================================================================
 * p_onion_apply
 *    create or replace onion layer(s) in the current image.
 *    Onion layers do show one (or more) merged copies of previos (or next)
 *    videoframe(s).
 *    This procedure first removes onion layers (if there are any)
 *    then reads the other videoframes, merges them (according to config params)
 *    and imports the merged layer(s) as onion layer(s).
 *    Onion Layers are marked by tattoo and parasite
 *    use_chache TRUE:
 *       check if desired frame is already in the image cache we can skip loading
 *       if the cached image is already merged we can simply copy the layer.
 *       Further we update the cache with merged layer.
 *       use_cache is for processing multiple frames (speeds up remarkable)
 *    use_cache FALSE:
 *       always load frames and perform merge,
 *       but we can steal layer from the tmp_image.
 *       (faster than copy when processing a single frame only)
 *
 *
 * returns   value >= 0 if all is ok
 *           (or -1 on error)
 * ============================================================================
 */
gint
p_onion_apply(t_global_params *gpp, gboolean use_cache)
{
  gint32        l_onr;
  gint32        l_ign;
  gint32        l_idx;
  gint32        l_frame_nr;
  gint32        l_tmp_image_id;
  gint32        l_is_onion;
  gint32        l_layerstack;
  char         *l_new_filename;
  char         *l_name;
  gint32        l_layer_id;
  gint32        l_new_layer_id;
  gint32     *l_layers_list;
  gint        l_nlayers;
  gdouble     l_opacity;
  char       *l_layername;


  if(gap_debug)
  {
     printf("p_onion_apply: START\n");
     printf("  num_olayers: %d\n",   (int)gpp->val.num_olayers);
     printf("  ref_delta: %d\n",     (int)gpp->val.ref_delta);
     printf("  ref_cycle: %d\n",     (int)gpp->val.ref_cycle);
     printf("  stack_pos: %d\n",     (int)gpp->val.stack_pos);
     printf("  stack_top: %d\n",     (int)gpp->val.stack_top);
     printf("  opacity: %f\n",       (float)gpp->val.opacity);
     printf("  opacity_delta: %f\n", (float)gpp->val.opacity_delta);
     printf("  ignore_botlayers: %d\n", (int)gpp->val.ignore_botlayers);
     printf("  image_ID: %d\n",      (int)gpp->image_ID);
     printf("  use_cache: %d\n",     (int)use_cache);
  }

  /* delete onion layers (if there are old ones) */
  p_onion_delete(gpp);

  l_layers_list = gimp_image_get_layers(gpp->image_ID, &l_nlayers);
  if(gpp->val.stack_top)
  {
    l_layerstack = CLAMP(gpp->val.stack_pos, 0, l_nlayers);
  }
  else
  {
    l_layerstack = CLAMP((l_nlayers - gpp->val.stack_pos),0 ,l_nlayers);
  }
  if(l_layers_list) { g_free(l_layers_list); l_layers_list = NULL;}

  /* create new onion layer(s) */
  l_opacity = gpp->val.opacity;
  l_new_filename = NULL;
  l_frame_nr = gpp->ainfo.curr_frame_nr;
  for(l_onr=0; l_onr < gpp->val.num_olayers; l_onr++)
  {
    /* find out reference frame number */
    l_frame_nr += gpp->val.ref_delta;
    if(!gpp->val.ref_cycle)
    {
      if((l_frame_nr < gpp->ainfo.first_frame_nr)
      || (l_frame_nr > gpp->ainfo.last_frame_nr))
      {
         break;  /* fold back cycle turned off */
      }
    }
    if (l_frame_nr < gpp->ainfo.first_frame_nr)
    {
      l_frame_nr = gpp->ainfo.last_frame_nr +1 - (gpp->ainfo.first_frame_nr - l_frame_nr);
      if (l_frame_nr < gpp->ainfo.first_frame_nr)
      {
        break;  /* stop on multiple fold back cycle */
      }
    }
    if (l_frame_nr > gpp->ainfo.last_frame_nr)
    {
      l_frame_nr = gpp->ainfo.first_frame_nr -1 + (l_frame_nr - gpp->ainfo.last_frame_nr);
      if (l_frame_nr > gpp->ainfo.last_frame_nr)
      {
         break;  /* stop on multiple fold back cycle */
      }
    }

    l_tmp_image_id = -1;
    l_layer_id = -1;
    if(use_cache)
    {
      p_find_frame_in_img_cache(gpp, l_frame_nr, &l_tmp_image_id, &l_layer_id);
      if (l_tmp_image_id == gpp->image_ID)
      {
        /* never use the same image as source and destination,
         * if references point to same image, always force
         * loading a 2.nd copy for the merge
         */
        l_tmp_image_id = -1;
      }
    }

    if(l_tmp_image_id < 0)
    {
      /* frame is not available in the cache */
      if(gap_debug) printf("p_onion_apply: frame is NOT available in the CACHE\n");
      /* build the frame name */
      if(l_new_filename != NULL) g_free(l_new_filename);
      l_new_filename = p_alloc_fname(gpp->ainfo.basename,
                                          l_frame_nr,
                                          gpp->ainfo.extension);
      /* load referenced frame */
      l_tmp_image_id = p_load_image(l_new_filename);
      if(l_tmp_image_id < 0)
         return -1;

      /* dont waste time and memory for undo in noninteracive processing
       * of the src frames
       */
      gimp_image_undo_enable(l_tmp_image_id); /* clear undo stack */
      gimp_image_undo_disable(l_tmp_image_id); /*  NO Undo */
    }
    else
    {
      if(gap_debug) printf("p_onion_apply: frame is AVAILABLE in the CACHE l_tmp_image_id: %d\n", (int)l_tmp_image_id);
    }


    if(l_layer_id < 0)
    {
      /* there was no merged layer in the cache, so we must
       * merge layers now
       */
      if(gap_debug) printf("p_onion_apply: layer is NOT available in the CACHE\n");

      /* set some layers invisible
       * a) ignored bottomlayer(s)
       * b) select_mode dependent: layers where layername does not match select-string
       * c) all onion layers
       */
      l_layers_list = gimp_image_get_layers(l_tmp_image_id, &l_nlayers);
      for(l_ign=0, l_idx=l_nlayers -1; l_idx >= 0;l_idx--)
      {
        l_layer_id = l_layers_list[l_idx];
        l_layername = gimp_layer_get_name(l_layer_id);


        l_is_onion = p_check_is_onion_layer(l_layer_id);

        if((l_ign <  gpp->val.ignore_botlayers)
        || (FALSE == p_match_layer( l_idx
                                  , l_layername
                                  , &gpp->val.select_string[0]
                                  , gpp->val.select_mode
                                  , gpp->val.select_case
                                  , gpp->val.select_invert
                                  , l_nlayers
                                  , l_layer_id)
           )
        || (l_is_onion))
        {
          gimp_layer_set_visible(l_layer_id, FALSE);
        }

        g_free (l_layername);

        if(!l_is_onion)
        {
          /* exclude other onion layers from counting ignored layers
           */
          l_ign++;
        }
      }

      /* merge visible layers (clip at image size) */
      l_layer_id = p_get_merged_layer(l_tmp_image_id, GIMP_CLIP_TO_IMAGE);
    }
    else
    {
      if(gap_debug) printf("p_onion_apply: layer is AVAILABLE in the CACHE l_layer_id: %d\n", (int)l_layer_id);
    }

    /* COPY or MOVE the resulting merged layer to current image gpp->image_ID */
    if(use_cache)
    {
      /* when using image cache we must copy the merged layer
       * but first update the cache, so we can reuse the merged layer
       * when processing the next frame
       */
      p_add_img_to_cache(gpp, l_frame_nr, l_tmp_image_id, l_layer_id);
    }

    if(l_layer_id >= 0)
    {
      gint         l_src_offset_x;    /* layeroffsets as they were in src_image */
      gint         l_src_offset_y;

      /* copy the layer to dest image gpp->image_ID */
      l_new_layer_id = p_my_layer_copy (gpp->image_ID
                                     ,l_layer_id
                                     ,CLAMP(l_opacity, 0.0, 100.0)
                                     ,0           /* NORMAL GimpLayerModeEffects */
                                     ,&l_src_offset_x
                                     ,&l_src_offset_y
                                     );

      if(! gimp_drawable_has_alpha(l_new_layer_id))
      {
         /* have to add alpha channel */
         gimp_layer_add_alpha(l_new_layer_id);
      }

      /* add the layer to current frame at desired stackposition  */
      gimp_image_add_layer (gpp->image_ID, l_new_layer_id, l_layerstack);
      gimp_layer_set_offsets(l_new_layer_id, l_src_offset_x, l_src_offset_y);

      /* set layername */
      l_name = g_strdup_printf(_("onionskin_%04d"), (int) l_frame_nr);
      gimp_layer_set_name(l_new_layer_id, l_name);
      g_free(l_name);


      /* Set parasite or tattoo */
      p_mark_as_onionlayer(l_new_layer_id);
      /* gimp_layer_set_opacity(l_new_layer_id, l_opacity); */
    }


    if(l_layers_list != NULL)  { g_free (l_layers_list); l_layers_list = NULL;}

    if(!use_cache)
    {
      /* destroy the tmp image */
      p_delete_image_immediate(l_tmp_image_id);
    }

    /* opacitiy for next onion layer (reduced by delta percentage) */
    l_opacity = CLAMP((l_opacity * gpp->val.opacity_delta / 100.0), 0.0 , 100.0);

    /* stackposition for next onion layer */
    l_layerstack++;
    if(l_new_filename) { g_free(l_new_filename); l_new_filename = NULL; };

  }
  if(gap_debug) printf("p_onion_apply: END\n\n");

  return 0;
}       /* end p_onion_apply */



/* ============================================================================
 * p_onion_range
 *    Apply or delete onionskin layers in selected framerange
 * ============================================================================
 */
gint
p_onion_range(t_global_params *gpp)
{
  int    l_rc;
  gint32 l_frame_nr;
  gint32 l_step, l_begin, l_end;
  gint32 l_current_image_id;
  gint32 l_curr_frame_nr;
  gdouble    l_percentage, l_percentage_step;
  gchar  *l_new_filename;


  l_percentage = 0.0;
  l_current_image_id = gpp->image_ID;
  if(gpp->val.ref_delta < 0)
  {
    /* for references to previous frames we step up (from low to high frame numbers) */
    l_begin = MIN(gpp->range_from, gpp->range_to);
    l_end = MAX(gpp->range_from, gpp->range_to);
    l_step = 1;
    l_percentage_step = 1.0 / ((1.0 + l_end) - l_begin);
  }
  else
  {
    /* for references to next frames we step down (from high to low frame numbers) */
    l_begin = MAX(gpp->range_from, gpp->range_to);
    l_end = MIN(gpp->range_from, gpp->range_to);
    l_step = -1;
    l_percentage_step = 1.0 / ((1.0 + l_begin) - l_end);
  }


  if(gpp->run_mode == GIMP_RUN_INTERACTIVE)
  {
    if(gpp->val.run == GAP_ONION_RUN_APPLY)
    {
      gimp_progress_init( _("Creating Onionskin Layers..."));
    }
    else
    {
      gimp_progress_init( _("Removing Onionskin Layers..."));
    }
  }

  /* save current image */
  l_new_filename = gimp_image_get_filename(l_current_image_id);
  l_rc = p_save_named_frame(gpp->image_ID, l_new_filename);
  if(l_rc < 0)
     return -1;

  l_curr_frame_nr = gpp->ainfo.curr_frame_nr;
  l_frame_nr = l_begin;
  while(1)
  {
    if(gap_debug) printf("p_onion_range processing frame %d\n", (int)l_frame_nr);
    gpp->ainfo.curr_frame_nr = l_frame_nr;
    if(l_new_filename != NULL) { g_free(l_new_filename); l_new_filename = NULL; }
    if (l_curr_frame_nr == l_frame_nr)
    {
       l_new_filename = gimp_image_get_filename(l_current_image_id);
       gpp->image_ID = l_current_image_id;

       /* never add the current image to the cache
        * (to prevent it from merging !!
        */
    }
    else
    {
      /* build the frame name */
      l_new_filename = p_alloc_fname(gpp->ainfo.basename,
                                          l_frame_nr,
                                          gpp->ainfo.extension);
      /* load frame */
      gpp->image_ID =  p_load_image(l_new_filename);
      if(gpp->image_ID < 0)
         return -1;

      /* add image to cache */
      p_add_img_to_cache(gpp, l_frame_nr, gpp->image_ID, -1);
    }

    if(gpp->val.run == GAP_ONION_RUN_APPLY)
    {
      p_onion_apply(gpp, TRUE /* use_cache */);
    }
    else if(gpp->val.run == GAP_ONION_RUN_DELETE)
    {
      p_onion_delete(gpp);
    }
    else
    {
      printf("operation not implemented\n");
    }

    l_rc = p_save_named_frame(gpp->image_ID, l_new_filename);
    if(l_rc < 0)
       return -1;

    if(gpp->run_mode == GIMP_RUN_INTERACTIVE)
    {
      l_percentage += l_percentage_step;
      gimp_progress_update (l_percentage);
    }

    if(l_frame_nr == l_end)
    {
      break;
    }
    l_frame_nr += l_step;
  }

  /* clean up the cache (delete all cached temp images) */
  p_delete_img_cache(gpp);

  return 0; /* OK */
}       /* end p_onion_range */
