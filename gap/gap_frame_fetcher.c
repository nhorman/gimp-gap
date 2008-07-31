/* gap_frame_fetcher.c
 *
 *
 *  The FrameFetcher provides access to frames both from imagefiles and videofiles.
 *  
 *  It holds a global image cache of temporary gimp images intended for
 *  read only access in various gimp-gap render processings.
 *  
 *  There are methods to get the temporary image 
 *  or to get a duplicate that has only one layer at imagesize.
 *  (merged or picked via desired stackposition)
 *
 *  For videofiles it holds a cache of open videofile handles.
 *  (note that caching of videoframes is already available in the videohandle)
 *  
 *  The current implementation of the frame fetcher is NOT multithred save !
 *  (the procedures may drop cached images that are still in use by a concurrent thread
 *  further the cache lists can be messe up if they are modified by concurrent threads
 *  at the same time.
 *
 *  Currently there is no support to keep track of cached images during the full length
 *  of a gimp session. This simple version of the frame fetcher is limited 
 *  to one main program (such as the storyboard or the filtermacro plug-in)
 *  and loses its information on exit of the main program.
 *  (If there are still registrated users at exit time, the cached images are still
 *  loaded in the gimp session)
 *  TODO: a more sophisticated version of the frame fetcher may keep its information
 *        using the gimp_det_data feature or mark the cached images with a tattoo.
 *

 *
 *
 * Copyright (C) 2008 Wolfgang Hofer <hof@gimp.org>
 *
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

/*
 * 2008.08.20  hof  - created (moved image cache stuff from gap_story_render_processing modules to this  new module)
 *                  - new feature:  caching of videohandles.
 *
 */

#include <config.h>

/* SYTEM (UNIX) includes */
#include <stdlib.h>
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <math.h>
//#include <errno.h>

//#include <dirent.h>


#include <glib/gstdio.h>



/* GIMP includes */
#include "gtk/gtk.h"
#include "gap-intl.h"
#include "libgimp/gimp.h"


#include "gap_libgimpgap.h"
#include "gap_lib_common_defs.h"
#include "gap_file_util.h"
#include "gap_vid_api.h"
#include "gap_layer_copy.h"
//#include "gap_fmac_name.h"

#include "gap_frame_fetcher.h"

#define GAP_FFETCH_MAX_IMG_CACHE_ELEMENTS 12
#define GAP_FFETCH_MAX_GVC_CACHE_ELEMENTS 6
#define GAP_FFETCH_GVA_FRAMES_TO_KEEP_CACHED 36



typedef struct GapFFetchResourceUserElem
{
   gint32  ffetch_user_id;
   void *next;
} GapFFetchResourceUserElem;



typedef struct GapFFetchDuplicatedImagesElem
{
   gint32  ffetch_user_id;
   gint32  image_id;
   void *next;
} GapFFetchDuplicatedImagesElem;


/* -------- types for the image cache  ------- */

typedef struct GapFFetchImageCacheElem
{
   gint32  image_id;
   char   *filename;
   void *next;
} GapFFetchImageCacheElem;

typedef struct GapFFetchImageCache
{
  GapFFetchImageCacheElem *ic_list;
  gint32            max_img_cache;  /* number of images to hold in the cache */
} GapFFetchImageCache;


/* -------- types for the video handle cache  ------- */

typedef struct GapFFetchGvahandCacheElem
{
   t_GVA_Handle *gvahand;
   gint32        mtime;
   gint32        seltrack;
   char         *filename;
   void *next;
} GapFFetchGvahandCacheElem;


typedef struct GapFFetchGvahandCache
{
  GapFFetchGvahandCacheElem *gvc_list;
  gint32            max_vid_cache;  /* number of videohandles to hold in the cache */
} GapFFetchGvahandCache;



extern int gap_debug;  /* 1 == print debug infos , 0 dont print debug infos */

/*************************************************************
 *         STATIC varaibles                                  *
 *************************************************************
 */

static GapFFetchDuplicatedImagesElem *global_duplicated_images = NULL;

static GapFFetchImageCache *global_imcache = NULL;

static GapFFetchGvahandCache *global_gvcache = NULL;

static GapFFetchResourceUserElem *global_rsource_users = NULL;

/*************************************************************
 *         FRAME FETCHER procedures                          *
 *************************************************************
 */
static gint32         p_load_cache_image(const char* filename, gboolean addToCache);
static void           p_drop_image_cache_elem1(GapFFetchImageCache *imcache);
static void           p_drop_image_cache(void);
#ifdef GAP_ENABLE_VIDEOAPI_SUPPORT
static void           p_drop_gvahand_cache_elem1(GapFFetchGvahandCache *gvcache);
static void           p_drop_vidhandle_cache(void);

static t_GVA_Handle*  p_ffetch_get_open_gvahand(const char* filename, gint32 seltrack
                          , const char *preferred_decoder);
#endif

static void           p_add_image_to_list_of_duplicated_images(gint32 image_id, gint32 ffetch_user_id);


/* ----------------------------------------------------
 * p_find_img_cache_by_image_id
 * ----------------------------------------------------
 */
static GapFFetchImageCacheElem  *
p_find_img_cache_by_image_id(gint32 image_id)
{
  GapFFetchImageCacheElem  *ic_ptr;

  if((global_imcache == NULL) || (image_id < 0))
  {
    return (NULL);
  }

  for(ic_ptr = global_imcache->ic_list; ic_ptr != NULL; ic_ptr = (GapFFetchImageCacheElem *)ic_ptr->next)
  {
    if(ic_ptr->image_id == image_id)
    {
      /* image found in cache */
      return(ic_ptr);
    }
  }

  return (NULL);
}  /* end  p_find_img_cache_by_image_id */


/* ----------------------------------------------------
 * p_load_cache_image
 * ----------------------------------------------------
 */
static gint32
p_load_cache_image(const char* filename, gboolean addToCache)
{
  gint32 l_idx;
  gint32 l_image_id;
  GapFFetchImageCacheElem  *ic_ptr;
  GapFFetchImageCacheElem  *ic_last;
  GapFFetchImageCacheElem  *ic_new;
  GapFFetchImageCache  *imcache;
  char *l_filename;

  if(filename == NULL)
  {
    printf("p_load_cache_image: ** ERROR cant load filename == NULL!\n");
    return -1;
  }

  if(global_imcache == NULL)
  {
    /* init the global_image cache */
    global_imcache = g_malloc0(sizeof(GapFFetchImageCache));
    global_imcache->ic_list = NULL;
    global_imcache->max_img_cache = GAP_FFETCH_MAX_IMG_CACHE_ELEMENTS;
  }

  imcache = global_imcache;
  ic_last = imcache->ic_list;

  l_idx = 0;
  for(ic_ptr = imcache->ic_list; ic_ptr != NULL; ic_ptr = (GapFFetchImageCacheElem *)ic_ptr->next)
  {
    l_idx++;
    if(strcmp(filename, ic_ptr->filename) == 0)
    {
      if(gap_debug)
      {
        printf("FrameFetcher: p_load_cache_image CACHE-HIT :%s (image_id:%d)\n"
          , ic_ptr->filename, (int)ic_ptr->image_id);
      }
      /* image found in cache, can skip load */
      return(ic_ptr->image_id);
    }
    ic_last = ic_ptr;
  }

  l_filename = g_strdup(filename);
  l_image_id = gap_lib_load_image(l_filename);
  if(gap_debug)
  {
    printf("FrameFetcher: loaded imafe from disk:%s (image_id:%d)\n"
      , l_filename, (int)l_image_id);
  }

  if((l_image_id >= 0) && (addToCache == TRUE))
  {
    ic_new = g_malloc0(sizeof(GapFFetchImageCacheElem));
    ic_new->filename = l_filename;
    ic_new->image_id = l_image_id;

    if(imcache->ic_list == NULL)
    {
      imcache->ic_list = ic_new;   /* 1.st elem starts the list */
    }
    else
    {
      ic_last->next = (GapFFetchImageCacheElem *)ic_new;  /* add new elem at end of the cache list */
    }

    if(l_idx > imcache->max_img_cache)
    {
      /* chache list has more elements than desired,
       * drop the 1.st (oldest) entry in the chache list
       */
      p_drop_image_cache_elem1(imcache);
    }
  }
  else
  {
    g_free(l_filename);
  }
  return(l_image_id);
}  /* end p_load_cache_image */


/* ----------------------------------------------------
 * p_drop_image_cache_elem1
 * ----------------------------------------------------
 */
static void
p_drop_image_cache_elem1(GapFFetchImageCache *imcache)
{
  GapFFetchImageCacheElem  *ic_ptr;

  if(imcache)
  {
    ic_ptr = imcache->ic_list;
    if(ic_ptr)
    {
      if(gap_debug)
      {
        printf("p_drop_image_cache_elem1 delete:%s (image_id:%d)\n"
          , ic_ptr->filename, (int)ic_ptr->image_id);
      }
      gap_image_delete_immediate(ic_ptr->image_id);
      g_free(ic_ptr->filename);
      imcache->ic_list = (GapFFetchImageCacheElem  *)ic_ptr->next;
      g_free(ic_ptr);
    }
  }
}  /* end p_drop_image_cache_elem1 */



/* ----------------------------------------------------
 * p_drop_image_cache
 * ----------------------------------------------------
 * drop the image cache.
 */
static void
p_drop_image_cache(void)
{
  GapFFetchImageCache *imcache;

  if(gap_debug)
  {
    printf("p_drop_image_cache START\n");
  }
  imcache = global_imcache;
  if(imcache)
  {
    while(imcache->ic_list)
    {
      p_drop_image_cache_elem1(imcache);
    }
  }
  global_imcache = NULL;

  if(gap_debug)
  {
    printf("p_drop_image_cache END\n");
  }

}  /* end p_drop_image_cache */



#ifdef GAP_ENABLE_VIDEOAPI_SUPPORT


/* ----------------------------------------------------
 * p_drop_gvahand_cache_elem1
 * ----------------------------------------------------
 */
static void
p_drop_gvahand_cache_elem1(GapFFetchGvahandCache *gvcache)
{
  GapFFetchGvahandCacheElem  *gvc_ptr;

  if(gvcache)
  {
    gvc_ptr = gvcache->gvc_list;
    if(gvc_ptr)
    {
      if(gap_debug)
      {
        printf("p_drop_gvahand_cache_elem1 delete:%s (gvahand:%d seltrack:%d mtime:%ld)\n"
               , gvc_ptr->filename, (int)gvc_ptr->gvahand
               , (int)gvc_ptr->seltrack, (long)gvc_ptr->mtime);
      }
      GVA_close(gvc_ptr->gvahand);
      g_free(gvc_ptr->filename);
      gvcache->gvc_list = (GapFFetchGvahandCacheElem  *)gvc_ptr->next;
      g_free(gvc_ptr);
    }
  }
}  /* end p_drop_gvahand_cache_elem1 */


/* ----------------------------------------------------
 * p_drop_vidhandle_cache
 * ----------------------------------------------------
 */
static void
p_drop_vidhandle_cache(void)
{
  GapFFetchGvahandCache *gvc_cache;

  if(gap_debug)
  {
    printf("p_drop_vidhandle_cache START\n");
  }
  gvc_cache = global_gvcache;
  if(gvc_cache)
  {
    while(gvc_cache->gvc_list)
    {
      p_drop_gvahand_cache_elem1(gvc_cache);
    }
  }
  global_gvcache = NULL;

  if(gap_debug)
  {
    printf("p_drop_vidhandle_cache END\n");
  }
  

}  /* end p_drop_vidhandle_cache */




/* ----------------------------------------------------
 * p_ffetch_get_open_gvahand
 * ----------------------------------------------------
 * get videohandle from the cache of already open handles.
 * opens a new handle if there is no cache hit.
 * if too many handles are alredy open, the oldest one is closed.
 * note that the cached handles must not be used outside the
 * frame fetcher module. Therefore the closing old handles
 * can be done.
 */
static t_GVA_Handle*
p_ffetch_get_open_gvahand(const char* filename, gint32 seltrack, const char *preferred_decoder)
{
  gint32 l_idx;
  t_GVA_Handle *l_gvahand;
  GapFFetchGvahandCacheElem  *gvc_ptr;
  GapFFetchGvahandCacheElem  *gvc_last;
  GapFFetchGvahandCacheElem  *gvc_new;
  GapFFetchGvahandCache      *gvcache;

  if(filename == NULL)
  {
    printf("p_ffetch_get_open_gvahand: ** ERROR cant load video filename == NULL!\n");
    return (NULL);
  }

  if(global_gvcache == NULL)
  {
    /* init the global_image cache */
    global_gvcache = g_malloc0(sizeof(GapFFetchGvahandCache));
    global_gvcache->gvc_list = NULL;
    global_gvcache->max_vid_cache = GAP_FFETCH_MAX_GVC_CACHE_ELEMENTS;
  }

  gvcache = global_gvcache;
  gvc_last = gvcache->gvc_list;

  l_idx = 0;
  for(gvc_ptr = gvcache->gvc_list; gvc_ptr != NULL; gvc_ptr = (GapFFetchGvahandCacheElem *)gvc_ptr->next)
  {
    l_idx++;
    if((strcmp(filename, gvc_ptr->filename) == 0) && (seltrack == gvc_ptr->seltrack))
    {
      /* videohandle found in cache, can skip opening a new handle */
      return(gvc_ptr->gvahand);
    }
    gvc_last = gvc_ptr;
  }

  if(preferred_decoder)
  {
    l_gvahand = GVA_open_read_pref(filename
                                  , seltrack
                                  , 1 /* aud_track */
                                  , preferred_decoder
                                  , FALSE  /* use MMX if available (disable_mmx == FALSE) */
                                  );
  }
  else
  {
    l_gvahand = GVA_open_read(filename
                             ,seltrack
                             ,1 /* aud_track */
                             );
  }
  
  if(l_gvahand)
  {
    GVA_set_fcache_size(l_gvahand, GAP_FFETCH_GVA_FRAMES_TO_KEEP_CACHED);

    gvc_new = g_malloc0(sizeof(GapFFetchGvahandCacheElem));
    gvc_new->filename = g_strdup(filename);
    gvc_new->seltrack = seltrack;
    gvc_new->mtime = GVA_file_get_mtime(filename);
    gvc_new->gvahand = l_gvahand;

    if(gvcache->gvc_list == NULL)
    {
      gvcache->gvc_list = gvc_new;   /* 1.st elem starts the list */
    }
    else
    {
      gvc_last->next = (GapFFetchGvahandCacheElem *)gvc_new;  /* add new elem at end of the cache list */
    }

    if(l_idx > gvcache->max_vid_cache)
    {
      /* chache list has more elements than desired,
       * drop the 1.st (oldest) entry in the chache list
       * (this closes the droped handle)
       */
      p_drop_gvahand_cache_elem1(gvcache);
    }
  }
  return(l_gvahand);
}  /* end p_ffetch_get_open_gvahand */

#endif

/* -----------------------------------------
 * p_add_image_to_list_of_duplicated_images
 * -----------------------------------------
 * add specified image to the list of duplicated images.
 * this list contains temporary images of both fetched video frames
 * and merged duplicates of the cached original images.
 */
static void
p_add_image_to_list_of_duplicated_images(gint32 image_id, gint32 ffetch_user_id)
{
  GapFFetchDuplicatedImagesElem *dupElem;
  
  dupElem = g_new(GapFFetchDuplicatedImagesElem, 1);
  dupElem->next = global_duplicated_images;
  dupElem->image_id = image_id;
  dupElem->ffetch_user_id = ffetch_user_id;
  global_duplicated_images = dupElem;
}   /* end p_add_image_to_list_of_duplicated_images */


/* -------------------------------------------------
 * gap_frame_fetch_delete_list_of_duplicated_images
 * -------------------------------------------------
 * deletes all duplicate imageas that wre created by the specified ffetch_user_id
 * (if ffetch_user_id -1 is specified delte all duplicated images)
 */
void
gap_frame_fetch_delete_list_of_duplicated_images(gint32 ffetch_user_id)
{
  GapFFetchDuplicatedImagesElem *dupElem;
  GapFFetchDuplicatedImagesElem *nextDupElem;
  
  dupElem = global_duplicated_images;
  while(dupElem)
  {
    nextDupElem = dupElem->next;

    if (((ffetch_user_id == dupElem->ffetch_user_id) || (ffetch_user_id < 0))
    && (dupElem->image_id >= 0))
    {
      gap_image_delete_immediate(dupElem->image_id);
      dupElem->image_id = -1;  /* set image invalid */
    }

    if (ffetch_user_id < 0)
    {
      g_free(dupElem);
    }

    dupElem = nextDupElem;
  }
  if (ffetch_user_id < 0)
  {
    global_duplicated_images = NULL;
  }
}  /* end gap_frame_fetch_delete_list_of_duplicated_images */






/* ----------------------------
 * gap_frame_fetch_orig_image
 * ----------------------------
 * returns image_id of the original cached image.
 */
gint32
gap_frame_fetch_orig_image(gint32 ffetch_user_id
    ,const char *filename            /* full filename of the image */
    ,gboolean addToCache             /* enable caching */
    )
{
  return (p_load_cache_image(filename, addToCache));
}  /* end gap_frame_fetch_orig_image */



/* ----------------------------
 * gap_frame_fetch_dup_image
 * ----------------------------
 * returns merged or selected layer_id 
 *        (that is the only visible layer in temporary created scratch image)
 *        the caller is resonsible to delete the scratch image when processing is done.
 *         this can be done by calling gap_frame_fetch_delete_list_of_duplicated_images()
 */
gint32
gap_frame_fetch_dup_image(gint32 ffetch_user_id
    ,const char *filename            /* full filename of the image (already contains framenr) */
    ,gint32      stackpos            /* 0 pick layer on top of stack, -1 merge visible layers */
    ,gboolean addToCache             /* enable caching */
    )
{
  gint32 resulting_layer;
  gint32 image_id;
  gint32 dup_image_id;

  resulting_layer = -1;
  image_id = p_load_cache_image(filename, addToCache);
  if (image_id < 0)
  {
    return(-1);
  }
  
  if (stackpos < 0)
  {
    dup_image_id = gimp_image_duplicate(image_id);
    resulting_layer = gap_image_merge_visible_layers(dup_image_id, GIMP_CLIP_TO_IMAGE);
  }
  else
  {
    gint          l_nlayers;
    gint32       *l_layers_list;
     

    l_layers_list = gimp_image_get_layers(image_id, &l_nlayers);
    if(l_layers_list != NULL)
    {
      if (stackpos < l_nlayers)
      {
        gint32 src_layer_id;

        src_layer_id = l_layers_list[stackpos];
        dup_image_id = gimp_image_new (gimp_image_width(image_id)
                                     , gimp_image_height(image_id)
                                     , gimp_image_base_type(image_id)
                                     );
        resulting_layer = gap_layer_copy_to_image (dup_image_id, src_layer_id);
      }
      
      g_free (l_layers_list);
    }
  }

  p_add_image_to_list_of_duplicated_images(dup_image_id, ffetch_user_id);


  if (addToCache != TRUE)
  {
    GapFFetchImageCacheElem *ic_elem;
    
    ic_elem = p_find_img_cache_by_image_id(image_id);
    
    if (ic_elem == NULL)
    {
      /* the original image is not cached
       * (delete it because the caller gets the preprocessed duplicate)
       */
      gap_image_delete_immediate(image_id);
    }
  }

  return(resulting_layer);

}  /* end gap_frame_fetch_dup_image */



/* ----------------------------
 * gap_frame_fetch_dup_video
 * ----------------------------
 * returns the fetched video frame as gimp layer_id.
 *         the returned layer id is (the only layer) in a temporary image.
 *         note the caller is responsible to delete that temporary image after processing is done.
 *         this can be done by calling gap_frame_fetch_delete_list_of_duplicated_images()
 */
gint32
gap_frame_fetch_dup_video(gint32 ffetch_user_id
    ,const char *filename            /* full filename of a video */
    ,gint32      framenr             /* frame within the video (starting at 1) */
    ,gint32      seltrack            /* videotrack */
    ,const char *preferred_decoder)
{
  gint32         l_layer_id = -1;
#ifdef GAP_ENABLE_VIDEOAPI_SUPPORT
  t_GVA_Handle *gvahand;
  t_GVA_RetCode  l_fcr;
  
  gvahand = p_ffetch_get_open_gvahand(filename, seltrack, preferred_decoder);
  if (gvahand == NULL)
  {
    return(-1);
  }

  /* attempt to get frame from the handles internal cache */
  l_fcr = GVA_frame_to_gimp_layer(gvahand
                                    , TRUE      /* delete_mode */
                                    , framenr   /* framenumber */
                                    , 0         /* deinterlace */
                                    , 0.0       /* threshold */
                                    );

  if (l_fcr != GVA_RET_OK)
  {
    /* if no success, we try explicite read that frame  */
    if(gvahand->current_seek_nr != framenr)
    {
      if(((gvahand->current_seek_nr + GAP_FFETCH_GVA_FRAMES_TO_KEEP_CACHED) > framenr)
      &&  (gvahand->current_seek_nr < framenr ) )
      {
        /* near forward seek is performed by dummyreads to fill up the 
         * handles internal framecache
         */
        while(gvahand->current_seek_nr < framenr)
        {
          GVA_get_next_frame(gvahand);
        }
      }
      else
      {
        GVA_seek_frame(gvahand, (gdouble)framenr, GVA_UPOS_FRAMES);
     }
    }

    if(GVA_get_next_frame(gvahand) == GVA_RET_OK)
    {
      GVA_frame_to_gimp_layer(gvahand
                      , TRUE      /* delete_mode */
                      , framenr   /* framenumber */
                      , 0         /* deinterlace */
                      , 0.0       /* threshold */
                      );
    }
  }

  /* return the newly created layer from the temporary image in the gvahand stucture.
   */
  l_layer_id = gvahand->layer_id;
  
  p_add_image_to_list_of_duplicated_images(gvahand->image_id, ffetch_user_id);
  gvahand->image_id = -1;
  gvahand->layer_id = -1;

#endif  
  return (l_layer_id);

}  /* end gap_frame_fetch_dup_video */    


/* -------------------------------------------------
 * gap_frame_fetch_drop_resources
 * -------------------------------------------------
 * drop all cached resources and all working copies (in the list of duplicated images).
 * (regardless if there are still resource users registrated)
 */
void
gap_frame_fetch_drop_resources()
{
  gap_frame_fetch_delete_list_of_duplicated_images(-1);

  p_drop_image_cache();
  p_drop_vidhandle_cache();
}  /* end gap_frame_fetch_drop_resources */


/* -------------------------------------------------
 * gap_frame_fetch_register_user
 * -------------------------------------------------
 * register for using the frame fetcher resource.
 * returns a unique resource user id.
 */
gint32
gap_frame_fetch_register_user(const char *caller_name)
{
  gint32 max_ffetch_user_id;
  GapFFetchResourceUserElem *usr_ptr;
  GapFFetchResourceUserElem *new_usr_ptr;
  
  max_ffetch_user_id = 0;
  new_usr_ptr = NULL;
  
  for(usr_ptr = global_rsource_users; usr_ptr != NULL; usr_ptr = (GapFFetchResourceUserElem *)usr_ptr->next)
  {
    printf("usr_ptr->ffetch_user_id: %d  usr_ptr:%d\n", usr_ptr->ffetch_user_id, usr_ptr);
  
    if (usr_ptr->ffetch_user_id >= 0)
    {
      max_ffetch_user_id = MAX(max_ffetch_user_id, usr_ptr->ffetch_user_id);
    }
    else
    {
      new_usr_ptr = usr_ptr;  /* reuse inactive element */
    }
  }
  
  max_ffetch_user_id++;
  if (new_usr_ptr == NULL)
  {
    new_usr_ptr = g_new(GapFFetchResourceUserElem, 1);
    new_usr_ptr->next = global_rsource_users;
    global_rsource_users = new_usr_ptr;
  }
  new_usr_ptr->ffetch_user_id = max_ffetch_user_id;
  
  if(gap_debug)
  {
    printf("gap_frame_fetch_register_user: REGISTRATED ffetch_user_id:%d  caller_name:%s  new_usr_ptr:%d\n"
          , new_usr_ptr->ffetch_user_id
          , caller_name
          , new_usr_ptr
          );
  }
  return (max_ffetch_user_id);
}  /* end  gap_frame_fetch_register_user*/


/* -------------------------------------------------
 * gap_frame_fetch_unregister_user
 * -------------------------------------------------
 * unregister the specified resource user id.
 + (if there are still registered resource users
 *  cached images and videohandles are kept.
 *  until the last resource user calls this procedure.
 *  if there are no more registered users all
 *  cached resources and duplicates are dropped)
 */
void
gap_frame_fetch_unregister_user(gint32 ffetch_user_id)
{
  gint32 count_active_users;
  GapFFetchResourceUserElem *usr_ptr;

  if(gap_debug)
  {
    printf("gap_frame_fetch_unregister_user: UNREGISTER ffetch_user_id:%d\n"
          , ffetch_user_id
          );
  }

  count_active_users = 0;
  for(usr_ptr = global_rsource_users; usr_ptr != NULL; usr_ptr = (GapFFetchResourceUserElem *)usr_ptr->next)
  {
    if (ffetch_user_id == usr_ptr->ffetch_user_id)
    {
      usr_ptr->ffetch_user_id = -1;
    }
    else if (usr_ptr->ffetch_user_id >= 0)
    {
      count_active_users++;
    }
  }
  
  if(count_active_users == 0)
  {
    if(gap_debug)
    {
      printf("gap_frame_fetch_unregister_user: no more resource users, DROP cached resources\n");
    }
    gap_frame_fetch_drop_resources();    
  }

}  /* end gap_frame_fetch_unregister_user */
