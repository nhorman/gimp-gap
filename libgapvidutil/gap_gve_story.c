/* gap_gve_story.c
 *
 *  GAP video encoder tool procedures for STORYBOARD file based video encoding
 *
 */

/*
 * 2004.09.25  hof  - bugfix: made gap_gve_story_fetch_composite_image_or_chunk
 *                    basically work when dont_recode_flag is set (for lossles MPEG cut)
 * 2004.09.12  hof  - replaced parser by the newer one from libgapstory.a
 *                    (the new parser supports named parameters)
 * 2004.07.24  hof  - added step_density parameter
 * 2004.05.17  hof  - integration into gimp-gap project
 *                    Info: gimp-gap has already another storyboard parser module
 *                     gap/gap_story_file.c, but that module can not handle
 *                     level 2 features (multiple tracks, and transition effects,
 *                     and audio processing)
 *                    In the future there should be only one common parser ...
 *
 * 2004.02.25  hof  - extended Storyboard Syntax  "VID_PREFERRED_DECODER"
 * 2003.05.29  hof  - gap_gve_story_fetch_composite_image_or_chunk (for dont_recode_flag)
 * 2003.04.20  hof  - Support deinterlace and exact_seek features for VID_PLAY_MOVIE
 * 2003.04.15  hof  - minor bugfixes
 * 2003.04.05  hof  - use GVA framecahe
 * 2003.03.08  hof  - buffered audio processing
 * 2003.03.04  hof  - added Videofile (MOVIE) audio support
 * 2003.02.04  hof  - added Videofile (MOVIE) frames support
 * 2003.01.09  hof  - support for video encoding of single multilayer images
 * 2002.11.16  hof  - created procedures for storyboard file processing
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

#include <config.h>

/* SYTEM (UNIX) includes */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <errno.h>

#include <dirent.h>




/* GIMP includes */
#include "gtk/gtk.h"
#include "gap-intl.h"
#include "libgimp/gimp.h"


#include "gap_libgimpgap.h"
#include "gap_file_util.h"
#include "gap_audio_util.h"
#include "gap_audio_wav.h"
#include "gap_gve_sox.h"
#include "gap_vid_api.h"
#include "gap_story_file.h"
#include "gap_gve_story.h"

/*************************************************************
 *         STORYBOARD FUNCTIONS                              *
 *************************************************************
 */
/* if G_BYTE_ORDER == G_BIG_ENDIAN */
#if G_BYTE_ORDER == G_LITTLE_ENDIAN

typedef union t_wav_16bit_int
{
  gint16  value;
  guint16 uvalue;
  struct
  {
    guchar lsb;
    guchar msb;
  } bytes;
} t_wav_16bit_int;
#else

typedef union t_wav_16bit_int
{
  gint16 value;
  struct
  {
    guchar msb;
    guchar lsb;
  } bytes;
} t_wav_16bit_int;

#endif

#define AUDIO_SEGMENT_SIZE 4000000

#define MAX_IMG_CACHE_ELEMENTS 6
#define MAX_AUD_CACHE_ELEMENTS 999
#define VID_FRAMES_TO_KEEP_CACHED 36




static GapGveStoryImageCache *global_imcache = NULL;
static GapGveStoryAudioCache *global_audcache = NULL;
static gint32 global_monitor_image_id = -1;
static gint32 global_monitor_display_id = -1;


static void     p_frame_backup_save(  char *key
                  , gint32 image_id
                  , gint32 layer_id
                  , gint32  master_frame_nr
                  , gboolean multilayer
                  );
static void     p_debug_dup_image(gint32 image_id);
static void     p_encoding_monitor(  char *key
                  , gint32 image_id
                  , gint32 layer_id
                  , gint32  master_frame_nr
                  );
static GapGveStoryErrors * p_new_stb_error(void);
static void     p_init_stb_error(GapGveStoryErrors *sterr);
static void     p_free_stb_error(GapGveStoryErrors *sterr);
static void     p_set_stb_error(GapGveStoryErrors *sterr, char *errtext);
static void     p_set_stb_warning(GapGveStoryErrors *sterr, char *warntext);
static void     p_drop_image_cache_elem1(GapGveStoryImageCache *imcache);
static gint32   p_load_cache_image( char* filename);
static void     p_drop_audio_cache_elem1(GapGveStoryAudioCache *audcache);
static GapGveStoryAudioCacheElem  *p_load_cache_audio( char* filename, gint32 *audio_id, gint32 *aud_bytelength, gint32 seek_idx);
static void     p_clear_layer(gint32 layer_id
                             ,guchar red
                             ,guchar green
                             ,guchar blue
                             ,guchar alpha
                             );
static void     p_find_min_max_vid_tracknumbers(GapGveStoryFrameRangeElem *frn_list
                             , gint32 *lowest_tracknr
                             , gint32 *highest_tracknr
                             );
static void     p_find_min_max_aud_tracknumbers(GapGveStoryAudioRangeElem *aud_list
                              , gint32 *lowest_tracknr
                              , gint32 *highest_tracknr);
static void     p_get_audio_sample(GapGveStoryVidHandle *vidhand        /* IN  */
                  ,gint32 track                 /* IN  */
                  ,gint32 master_sample_idx     /* IN  */
                  ,gdouble *sample_left    /* OUT (prescaled at local volume) */
                  ,gdouble *sample_right   /* OUT  (prescaled at local volume)*/
                  );
static void     p_mix_audio(FILE *fp                  /* IN: NULL: dont write to file */
                  ,gint16 *bufferl           /* IN: NULL: dont write to buffer */
                  ,gint16 *bufferr           /* IN: NULL: dont write to buffer */
                  ,GapGveStoryVidHandle *vidhand
                  ,gdouble aud_total_sec
                  ,gdouble *mix_scale         /* OUT */
                  );
static void     p_check_audio_peaks(GapGveStoryVidHandle *vidhand
                 ,gdouble aud_total_sec
                 ,gdouble *mix_scale         /* OUT */
                 );
static void     p_step_attribute(gint32 frames_handled
                 ,gdouble *from_val
                 ,gdouble *to_val
                 ,gint32  *dur
                 );
static gdouble  p_step_attribute_read(gint32 frame_step
                 ,gdouble from_val
                 ,gdouble to_val
                 ,gint32  dur
                 );
static gdouble  p_step_attribute_read(gint32 frame_step
                 ,gdouble from_val
                 ,gdouble to_val
                 ,gint32  dur
                 );
static char *   p_fetch_framename   (GapGveStoryFrameRangeElem *frn_list
                            , gint32 master_frame_nr                   /* starts at 1 */
                            , gint32 track
                            , GapGveStoryFrameType *frn_type
                            , char **filtermacro_file
                            , gint32   *localframe_index  /* used only for ANIMIMAGE and videoclip, -1 for all other types */
                            , gboolean *keep_proportions
                            , gboolean *fit_width
                            , gboolean *fit_height
                            , guchar  *red
                            , guchar  *green
                            , guchar  *blue
                            , guchar  *alpha
                            , gdouble *opacity       /* output opacity 0.0 upto 1.0 */
                            , gdouble *scale_x       /* output 0.0 upto 10.0 where 1.0 is 1:1 */
                            , gdouble *scale_y       /* output 0.0 upto 10.0 where 1.0 is 1:1 */
                            , gdouble *move_x        /* output -1.0 upto 1.0 where 0.0 is centered */
                            , gdouble *move_y        /* output -1.0 upto 1.0 where 0.0 is centered */
                            , GapGveStoryFrameRangeElem **frn_elem_ptr  /* OUT pointer to the relevant framerange element */
                           );
static char *   p_make_abspath_filename(const char *filename, const char *storyboard_file);
static void     p_extract_audiopart(t_GVA_Handle *gvahand, char *wavfilename, long samples_to_read);
static GapGveStoryAudioRangeElem * p_new_audiorange_element(GapGveStoryAudioType  aud_type
                      ,gint32 track
                      ,const char *audiofile
                      ,gint32  master_samplerate
                      ,gdouble play_from_sec
                      ,gdouble play_to_sec
                      ,gdouble volume_start
                      ,gdouble volume
                      ,gdouble volume_end
                      ,gdouble fade_in_sec
                      ,gdouble fade_out_sec
                      ,char *util_sox
                      ,char *util_sox_options
                      ,const char *storyboard_file  /* IN: NULL if no storyboard file is used */
                      ,const char *preferred_decoder
                      ,GapGveStoryAudioRangeElem *known_aud_list /* NULL or list of already known ranges */
                      ,GapGveStoryErrors *sterr           /* element to store Error/Warning report */
                      ,gint32 seltrack      /* IN: select audiotrack number 1 upto 99 for GAP_AUT_MOVIE */
                      );
static GapGveStoryFrameRangeElem *  p_new_framerange_element(
                           GapGveStoryFrameType  frn_type
                          ,gint32 track
                          ,const char *basename       /* basename or full imagename  for frn_type GAP_FRN_IMAGE */
                          ,const char *ext            /* NULL for frn_type GAP_FRN_IMAGE */
                          ,gint32  frame_from   /* IN: use -1 for first, 99999999 for last */
                          ,gint32  frame_to     /* IN: use -1 for first, 99999999 for last */
                          ,const char *storyboard_file  /* IN: NULL if no storyboard file is used */
                          ,const char *preferred_decoder  /* IN: NULL if no preferred_decoder is specified */
                          ,const char *filtermacro_file  /* IN: NULL, or name of the macro file */
                          ,GapGveStoryFrameRangeElem *frn_list /* NULL or list of already known ranges */
                          ,GapGveStoryErrors *sterr          /* element to store Error/Warning report */
                          ,gint32 seltrack      /* IN: select videotrack number 1 upto 99 for GAP_FRN_MOVIE */
                          ,gint32 exact_seek    /* IN: 0 fast seek, 1 exact seek (only for GVA Videoreads) */
                          ,gdouble delace    /* IN: 0.0 no deinterlace, 1.0-1.99 odd 2.0-2.99 even rows (only for GVA Videoreads) */
                          ,gdouble step_density    /* IN:  1==normal stepsize 1:1   0.5 == each frame twice, 2.0 only every 2nd frame */
                           );
static void       p_add_frn_list(GapGveStoryVidHandle *vidhand, GapGveStoryFrameRangeElem *frn_elem);
static void       p_add_aud_list(GapGveStoryVidHandle *vidhand, GapGveStoryAudioRangeElem *aud_elem);
static void       p_set_vtrack_attributes(GapGveStoryFrameRangeElem *frn_elem
                       ,GapGveStoryVTrackArray *vtarr
                      );
static void       p_storyboard_analyze(GapStoryBoard *stb
                      , GapGveStoryVTrackArray *vtarr
                      , GapGveStoryFrameRangeElem *frn_known_list
                      , GapGveStoryVidHandle *vidhand
                      );
static GapGveStoryFrameRangeElem *  p_framerange_list_from_storyboard(
                           const char *storyboard_file
                          ,gint32 *frame_count
                          ,GapGveStoryVidHandle *vidhand
                          );
static void  p_free_framerange_list(GapGveStoryFrameRangeElem * frn_list);
static GapGveStoryVidHandle * p_open_video_handle_private(    gboolean ignore_audio
                      , gboolean ignore_video
                      , gdouble  *progress_ptr
                      , char *status_msg
                      , gint32 status_msg_len
                      , const char *storyboard_file
                      , const char *basename
                      , const char *ext
                      , gint32  frame_from
                      , gint32  frame_to
                      , gint32 *frame_count   /* output total frame_count , or 0 on failure */
                      , gboolean do_gimp_progress
		      , GapGveTypeInputRange input_mode
		      , const char *imagename
                      );
static gint32     p_exec_filtermacro(gint32 image_id, gint32 layer_id, char *filtermacro_file);
static gint32     p_transform_and_add_layer( gint32 comp_image_id
                         , gint32 tmp_image_id
                         , gint32 layer_id
                         , gboolean keep_proportions
                         , gboolean fit_width
                         , gboolean fit_height
                         , gdouble opacity    /* 0.0 upto 1.0 */
                         , gdouble scale_x    /* 0.0 upto 10.0 where 1.0 = 1:1 */
                         , gdouble scale_y    /* 0.0 upto 10.0 where 1.0 = 1:1 */
                         , gdouble move_x     /* -1.0 upto +1.0 where 0 = no move, -1 is left outside */
                         , gdouble move_y     /* -1.0 upto +1.0 where 0 = no move, -1 is top outside */
                         , char *filtermacro_file
                         );
static gint32     p_create_unicolor_image(gint32 *layer_id, gint32 width , gint32 height
                       , guchar r, guchar g, guchar b, guchar a);
static void       p_layer_resize_to_imagesize(gint32 layer_id);
static gint32     p_prepare_RGB_image(gint32 image_id);
static t_GVA_Handle * p_try_to_steal_gvahand(GapGveStoryVidHandle *vidhand
                      , gint32 master_frame_nr
                      , char *basename             /* the videofile name */
                      , gint32 exact_seek
                      );




/* ----------------------------------------------------
 * gap_gve_story_debug_print_framerange_list
 * ----------------------------------------------------
 * print all List elements for the given track
 * (negative track number will print all elements)
 */
void
gap_gve_story_debug_print_framerange_list(GapGveStoryFrameRangeElem *frn_list
                             , gint32 track                    /* -1 show all tracks */
                             )
{
  GapGveStoryFrameRangeElem *frn_elem;
  gint                 l_idx;

  printf("\ngap_gve_story_debug_print_framerange_list: START\n");

  l_idx = 0;
  for(frn_elem = frn_list; frn_elem != NULL; frn_elem = (GapGveStoryFrameRangeElem *)frn_elem->next)
  {
    if((frn_elem->track == track) || (track < 0))
    {
      printf("\n  [%d] frn_type         : %d\n", (int)l_idx, (int)frn_elem->frn_type );
      printf("  [%d] track             : %d\n", (int)l_idx, (int)frn_elem->track );
      printf("  [%d] basename          : ", (int)l_idx); if(frn_elem->basename) { printf("%s\n", frn_elem->basename );} else { printf ("(null)\n"); }
      printf("  [%d] ext               : ", (int)l_idx); if(frn_elem->ext) { printf("%s\n", frn_elem->ext );} else { printf ("(null)\n"); }
      printf("  [%d] gvahand           : %d\n", (int)l_idx, (int)frn_elem->gvahand );
      printf("  [%d] seltrack          : %d\n", (int)l_idx, (int)frn_elem->seltrack );
      printf("  [%d] exact_seek        : %d\n", (int)l_idx, (int)frn_elem->exact_seek );
      printf("  [%d] delace            : %.2f\n", (int)l_idx, (float)frn_elem->delace );
      printf("  [%d] filtermacro_file  : ", (int)l_idx); if(frn_elem->filtermacro_file) { printf("%s\n", frn_elem->filtermacro_file );} else { printf ("(null)\n"); }

      printf("  [%d] frame_from        : %d\n", (int)l_idx, (int)frn_elem->frame_from );
      printf("  [%d] frame_to          : %d\n", (int)l_idx, (int)frn_elem->frame_to );
      printf("  [%d] frame_first       : %d\n", (int)l_idx, (int)frn_elem->frame_first );
      printf("  [%d] frame_last        : %d\n", (int)l_idx, (int)frn_elem->frame_last );
      printf("  [%d] frames_to_handle  : %d\n", (int)l_idx, (int)frn_elem->frames_to_handle);
      printf("  [%d] frame_cnt         : %d\n", (int)l_idx, (int)frn_elem->frame_cnt );
      printf("  [%d] delta             : %d\n", (int)l_idx, (int)frn_elem->delta );
      printf("  [%d] step_density      : %.4f\n", (int)l_idx, (float)frn_elem->step_density );

      if(frn_elem->keep_proportions)
      {printf("  [%d] keep_proportions  : TRUE\n", (int)l_idx );}
      else
      {printf("  [%d] keep_proportions  : FALSE\n", (int)l_idx );}

      if(frn_elem->fit_width)
      {printf("  [%d] fit_width         : TRUE\n", (int)l_idx );}
      else
      {printf("  [%d] fit_width         : FALSE\n", (int)l_idx );}

      if(frn_elem->fit_height)
      {printf("  [%d] fit_height        : TRUE\n", (int)l_idx );}
      else
      {printf("  [%d] fit_height        : FALSE\n", (int)l_idx );}

      printf("  [%d] wait_untiltime_sec : %f\n", (int)l_idx, (float)frn_elem->wait_untiltime_sec );
      printf("  [%d] wait_untilframes   : %d\n", (int)l_idx, (int)frn_elem->wait_untilframes );


      printf("  [%d] opacity_from      : %f\n", (int)l_idx, (float)frn_elem->opacity_from );
      printf("  [%d] opacity_to        : %f\n", (int)l_idx, (float)frn_elem->opacity_to );
      printf("  [%d] opacity_dur       : %d\n", (int)l_idx, (int)frn_elem->opacity_dur );

      printf("  [%d] scale_x_from      : %f\n", (int)l_idx, (float)frn_elem->scale_x_from );
      printf("  [%d] scale_x_to        : %f\n", (int)l_idx, (float)frn_elem->scale_x_to );
      printf("  [%d] scale_x_dur       : %d\n", (int)l_idx, frn_elem->scale_x_dur );

      printf("  [%d] scale_y_from      : %f\n", (int)l_idx, (float)frn_elem->scale_y_from );
      printf("  [%d] scale_y_to        : %f\n", (int)l_idx, (float)frn_elem->scale_y_to );
      printf("  [%d] scale_y_dur       : %d\n", (int)l_idx, (int)frn_elem->scale_y_dur );

      printf("  [%d] move_x_from       : %f\n", (int)l_idx, (float)frn_elem->move_x_from );
      printf("  [%d] move_x_to         : %f\n", (int)l_idx, (float)frn_elem->move_x_to );
      printf("  [%d] move_x_dur        : %d\n", (int)l_idx, (int)frn_elem->move_x_dur );

      printf("  [%d] move_y_from       : %f\n", (int)l_idx, (float)frn_elem->move_y_from );
      printf("  [%d] move_y_to         : %f\n", (int)l_idx, (float)frn_elem->move_y_to );
      printf("  [%d] move_y_dur        : %d\n", (int)l_idx, (int)frn_elem->move_y_dur );
    }
    l_idx++;
  }

  printf("gap_gve_story_debug_print_framerange_list: END\n");

}  /* end gap_gve_story_debug_print_framerange_list */


/* ----------------------------------------------------
 * gap_gve_story_debug_print_audiorange_list
 * ----------------------------------------------------
 * print all List elements for the given track
 * (negative track number will print all elements)
 */
void
gap_gve_story_debug_print_audiorange_list(GapGveStoryAudioRangeElem *aud_list
                             , gint32 track                    /* -1 show all tracks */
                             )
{
  GapGveStoryAudioRangeElem *aud_elem;
  gint                 l_idx;

  printf("\ngap_gve_story_debug_print_audiorange_list: START\n");

  l_idx = 0;
  for(aud_elem = aud_list; aud_elem != NULL; aud_elem = (GapGveStoryAudioRangeElem *)aud_elem->next)
  {
    if((aud_elem->track == track) || (track < 0))
    {
      printf("\n  [%d] aud_type         : %d\n", (int)l_idx, (int)aud_elem->aud_type );
      printf("  [%d] track             : %d\n", (int)l_idx, (int)aud_elem->track );
      printf("  [%d] audiofile         : ", (int)l_idx); if(aud_elem->audiofile) { printf("%s\n", aud_elem->audiofile );} else { printf ("(null)\n"); }
      printf("  [%d] tmp_audiofile     : ", (int)l_idx); if(aud_elem->tmp_audiofile) { printf("%s\n", aud_elem->tmp_audiofile );} else { printf ("(null)\n"); }
      printf("  [%d] gvahand           : %d\n", (int)l_idx, (int)aud_elem->gvahand );
      printf("  [%d] seltrack          : %d\n", (int)l_idx, (int)aud_elem->seltrack );

      printf("  [%d] samplerate        : %d\n", (int)l_idx, (int)aud_elem->samplerate );
      printf("  [%d] channels          : %d\n", (int)l_idx, (int)aud_elem->channels );
      printf("  [%d] bytes_per_sample  : %d\n", (int)l_idx, (int)aud_elem->bytes_per_sample );
      printf("  [%d] samples           : %d\n", (int)l_idx, (int)aud_elem->samples );


      printf("  [%d] audio_id          : %d\n", (int)l_idx, (int)aud_elem->audio_id);
      printf("  [%d] aud_data          : %d\n", (int)l_idx, (int)aud_elem->aud_data);
      printf("  [%d] aud_bytelength    : %d\n", (int)l_idx, (int)aud_elem->aud_bytelength);

      printf("  [%d] range_samples     : %d\n", (int)l_idx, (int)aud_elem->range_samples );
      printf("  [%d] fade_in_samples   : %d\n", (int)l_idx, (int)aud_elem->fade_in_samples );
      printf("  [%d] fade_out_samples  : %d\n", (int)l_idx, (int)aud_elem->fade_out_samples );
      printf("  [%d] offset_rangestart : %d\n", (int)l_idx, (int)aud_elem->byteoffset_rangestart );
      printf("  [%d] byteoffset_data   : %d\n", (int)l_idx, (int)aud_elem->byteoffset_data );


      printf("  [%d] wait_untiltime_sec: %f\n", (int)l_idx, (float)aud_elem->wait_untiltime_sec );
      printf("  [%d] wait_until_samples: %d\n", (int)l_idx, (int)aud_elem->wait_until_samples );

      printf("  [%d] max_playtime_sec  : %f\n", (int)l_idx, (float)aud_elem->max_playtime_sec );
      printf("  [%d] range_playtime_sec: %f\n", (int)l_idx, (float)aud_elem->range_playtime_sec );
      printf("  [%d] play_from_sec     : %f\n", (int)l_idx, (float)aud_elem->play_from_sec );
      printf("  [%d] play_to_sec       : %f\n", (int)l_idx, (float)aud_elem->play_to_sec );
      printf("  [%d] volume_start      : %f\n", (int)l_idx, (float)aud_elem->volume_start );
      printf("  [%d] volume            : %f\n", (int)l_idx, (float)aud_elem->volume );
      printf("  [%d] volume_end        : %f\n", (int)l_idx, (float)aud_elem->volume_end );
      printf("  [%d] fade_in_sec       : %f\n", (int)l_idx, (float)aud_elem->fade_in_sec );
      printf("  [%d] fade_out_sec      : %f\n", (int)l_idx, (float)aud_elem->fade_out_sec );
    }
    l_idx++;
  }

  printf("gap_gve_story_debug_print_audiorange_list: END\n");

}  /* end gap_gve_story_debug_print_audiorange_list */



/* ----------------------------------------------------
 * p_frame_backup_save
 * ----------------------------------------------------
 * set layer to unique color
 */
static void
p_frame_backup_save(  char *key
              , gint32 image_id
              , gint32 layer_id
              , gint32  master_frame_nr
              , gboolean multilayer
             )
{
  gint32  l_len;
  char *l_framename;
  char *l_basename;

  l_len = gimp_get_data_size(key);
  if(l_len <= 0)
  {
    return;
  }

  l_basename  = g_malloc0(l_len);
  gimp_get_data(key, l_basename);
  if(*l_basename != '\0')
  {
    if(multilayer)
    {
      l_framename = gap_lib_alloc_fname(l_basename, master_frame_nr, ".xcf");
    }
    else
    {
      l_framename = gap_lib_alloc_fname(l_basename, master_frame_nr, ".jpg");
    }

    if(gap_debug) printf("Debug: Saving frame to  file: %s\n", l_framename);

    gimp_file_save(GIMP_RUN_WITH_LAST_VALS, image_id, layer_id, l_framename, l_framename);
    g_free(l_framename);
  }
  g_free(l_basename);
}  /* end p_frame_backup_save */


/* ----------------------------------------------------
 * p_debug_dup_image
 * ----------------------------------------------------
 * Duplicate image, and open a display for the duplicate
 * (Procedure is used for debug only
 */
static void
p_debug_dup_image(gint32 image_id)
{
  gint32 l_dup_image_id;

  l_dup_image_id = gimp_image_duplicate(image_id);
  gimp_display_new(l_dup_image_id);
}  /* end p_debug_dup_image */


/* ----------------------------------------------------
 * p_encoding_monitor
 * ----------------------------------------------------
 * monitor the image be fore passed to the encoder.
 * - at 1.st call open global_monitor_image_id
 *       and add a display.
 * - on all further calls copy the composite layer
 *      to the global_monitor_image_id
 */
static void
p_encoding_monitor(  char *key
              , gint32 image_id
              , gint32 layer_id
              , gint32  master_frame_nr
             )
{
  gint32  l_len;
  char *l_true_or_false;

  l_len = gimp_get_data_size(key);
  if(l_len <= 0)
  {
    return;
  }

  l_true_or_false  = g_malloc0(l_len);
  gimp_get_data(key, l_true_or_false);
  if(*l_true_or_false == 'T')
  {
     char *l_imagename;

     printf("Monitoring image_id: %d, layer_id:%d  master_frame:%d\n", (int)image_id, (int)layer_id ,(int)master_frame_nr );

     l_imagename = g_strdup_printf(_("encoding_video_frame_%06d"), (int)master_frame_nr);
     if(global_monitor_image_id < 0)
     {
       global_monitor_image_id = gimp_image_duplicate(image_id);
       global_monitor_display_id = gimp_display_new(global_monitor_image_id);
       gimp_image_set_filename(global_monitor_image_id, l_imagename);
     }
     else
     {
       gint          l_nlayers;
       gint32       *l_layers_list;
       gint32        l_fsel_layer_id;

       l_layers_list = gimp_image_get_layers(global_monitor_image_id, &l_nlayers);
       if(l_layers_list != NULL)
       {
         gimp_selection_none(image_id);  /* if there is no selection, copy the complete layer */
         gimp_selection_none(global_monitor_image_id);  /* if there is no selection, copy the complete layer */
         gimp_edit_copy(layer_id);
         l_fsel_layer_id = gimp_edit_paste(l_layers_list[0], FALSE);  /* FALSE paste clear selection */
         gimp_floating_sel_anchor(l_fsel_layer_id);
         g_free (l_layers_list);
         gimp_image_set_filename(global_monitor_image_id, l_imagename);
         gimp_displays_flush();
       }
       else
       {
          printf("no more MONITORING, (user has closed monitor image)\n");
       }
     }
     g_free(l_imagename);
  }
  g_free(l_true_or_false);
}  /* end p_encoding_monitor */



/* --------------------------------
 * p_init_stb_error
 * --------------------------------
 */
static void
p_init_stb_error(GapGveStoryErrors *sterr)
{
  if(sterr->errtext)   { g_free(sterr->errtext); }
  if(sterr->errline)   { g_free(sterr->errline); }
  if(sterr->warntext)  { g_free(sterr->warntext); }
  if(sterr->warnline)  { g_free(sterr->warnline); }

  sterr->errtext     = NULL;
  sterr->errline     = NULL;
  sterr->warntext    = NULL;
  sterr->warnline    = NULL;
  sterr->currline    = NULL;
  sterr->errline_nr  = 0;
  sterr->warnline_nr = 0;
  sterr->curr_nr     = 0;
}  /* end p_init_stb_error */

/* --------------------------------
 * p_new_stb_error
 * --------------------------------
 */
static GapGveStoryErrors *
p_new_stb_error(void)
{
  GapGveStoryErrors *sterr;

  sterr = g_malloc0(sizeof(GapGveStoryErrors));
  p_init_stb_error(sterr);
  return(sterr);
}  /* end p_new_stb_error */

/* --------------------------------
 * p_free_stb_error
 * --------------------------------
 */
static void
p_free_stb_error(GapGveStoryErrors *sterr)
{
  p_init_stb_error(sterr);
  g_free(sterr);
}  /* end p_free_stb_error */

/* --------------------------------
 * p_set_stb_error
 * --------------------------------
 */
static void
p_set_stb_error(GapGveStoryErrors *sterr, char *errtext)
{
  printf("** error: %s\n   [at line:%d] %s\n"
        , errtext
        , (int)sterr->curr_nr
        , sterr->currline
        );
  if(sterr->errtext == NULL)
  {
     sterr->errtext     = g_strdup(errtext);
     sterr->errline_nr  = sterr->curr_nr;
     sterr->errline     = g_strdup(sterr->currline);
  }
}  /* end p_set_stb_error */

/* --------------------------------
 * p_set_stb_warning
 * --------------------------------
 */
static void
p_set_stb_warning(GapGveStoryErrors *sterr, char *warntext)
{
  printf("** warning: %s\n   [at line:%d] %s\n"
        , warntext
        , (int)sterr->curr_nr
        , sterr->currline
        );
  if(sterr->warntext == NULL)
  {
     sterr->warntext     = g_strdup(warntext);
     sterr->warnline_nr  = sterr->curr_nr;
     sterr->warnline     = g_strdup(sterr->currline);
  }
}  /* end p_set_stb_warning */


/* ----------------------------------------------------
 * p_drop_image_cache_elem1
 * ----------------------------------------------------
 */
static void
p_drop_image_cache_elem1(GapGveStoryImageCache *imcache)
{
  GapGveStoryImageCacheElem  *ic_ptr;

  if(imcache)
  {
    ic_ptr = imcache->ic_list;
    if(ic_ptr)
    {
      if(gap_debug) printf("p_drop_image_cache_elem1 delete:%s (image_id:%d)\n", ic_ptr->filename, (int)ic_ptr->image_id);
      gap_image_delete_immediate(ic_ptr->image_id);
      g_free(ic_ptr->filename);
      imcache->ic_list = (GapGveStoryImageCacheElem  *)ic_ptr->next;
      g_free(ic_ptr);
    }
  }
}  /* end p_drop_image_cache_elem1 */


/* ----------------------------------------------------
 * gap_gve_story_drop_image_cache
 * ----------------------------------------------------
 */
void
gap_gve_story_drop_image_cache(void)
{
  GapGveStoryImageCache *imcache;

  if(gap_debug)  printf("gap_gve_story_drop_image_cache START\n");
  imcache = global_imcache;
  if(imcache)
  {
    while(imcache->ic_list)
    {
      p_drop_image_cache_elem1(imcache);
    }
  }
  if(gap_debug) printf("gap_gve_story_drop_image_cache END\n");

}  /* end gap_gve_story_drop_image_cache */


/* ----------------------------------------------------
 * p_load_cache_image
 * ----------------------------------------------------
 */
static gint32
p_load_cache_image( char* filename)
{
  gint32 l_idx;
  gint32 l_image_id;
  GapGveStoryImageCacheElem  *ic_ptr;
  GapGveStoryImageCacheElem  *ic_last;
  GapGveStoryImageCacheElem  *ic_new;
  GapGveStoryImageCache  *imcache;

  if(filename == NULL)
  {
    printf("p_load_cache_image: ** ERROR cant load filename == NULL!\n");
    return -1;
  }

  if(global_imcache == NULL)
  {
    /* init the global_mage cache */
    global_imcache = g_malloc0(sizeof(GapGveStoryImageCache));
    global_imcache->ic_list = NULL;
    global_imcache->max_img_cache = MAX_IMG_CACHE_ELEMENTS;
  }

  imcache = global_imcache;
  ic_last = imcache->ic_list;

  l_idx = 0;
  for(ic_ptr = imcache->ic_list; ic_ptr != NULL; ic_ptr = (GapGveStoryImageCacheElem *)ic_ptr->next)
  {
    l_idx++;
    if(strcmp(filename, ic_ptr->filename) == 0)
    {
      /* image found in cache, can skip load */
      return(ic_ptr->image_id);
    }
    ic_last = ic_ptr;
  }

  l_image_id = gap_lib_load_image(filename);
  if(l_image_id >= 0)
  {
    ic_new = g_malloc0(sizeof(GapGveStoryImageCacheElem));
    ic_new->filename = g_strdup(filename);
    ic_new->image_id = l_image_id;

    if(imcache->ic_list == NULL)
    {
      imcache->ic_list = ic_new;   /* 1.st elem starts the list */
    }
    else
    {
      ic_last->next = (GapGveStoryImageCacheElem *)ic_new;  /* add new elem at end of the cache list */
    }

    if(l_idx > imcache->max_img_cache)
    {
      /* chache list has more elements than desired,
       * drop the 1.st (oldest) entry in the chache list
       */
      p_drop_image_cache_elem1(imcache);
    }
  }
  return(l_image_id);
}  /* end p_load_cache_image */




/* ----------------------------------------------------
 * p_drop_audio_cache_elem1
 * ----------------------------------------------------
 */
static void
p_drop_audio_cache_elem1(GapGveStoryAudioCache *audcache)
{
  GapGveStoryAudioCacheElem  *ac_ptr;

  if(audcache)
  {
    ac_ptr = audcache->ac_list;
    if(ac_ptr)
    {
      if(gap_debug) printf("p_drop_audio_cache_elem1 delete:%s (audio_id:%d)\n", ac_ptr->filename, (int)ac_ptr->audio_id);
      g_free(ac_ptr->aud_data);
      g_free(ac_ptr->filename);
      audcache->ac_list = (GapGveStoryAudioCacheElem  *)ac_ptr->next;
      g_free(ac_ptr);
    }
  }
}  /* end p_drop_audio_cache_elem1 */


/* ----------------------------------------------------
 * gap_gve_story_drop_audio_cache
 * ----------------------------------------------------
 */
void
gap_gve_story_drop_audio_cache(void)
{
  GapGveStoryAudioCache *audcache;

  if(gap_debug)  printf("gap_gve_story_drop_audio_cache START\n");
  audcache = global_audcache;
  if(audcache)
  {
    while(audcache->ac_list)
    {
      p_drop_audio_cache_elem1(audcache);
    }
  }
  if(gap_debug) printf("gap_gve_story_drop_audio_cache END\n");

}  /* end gap_gve_story_drop_audio_cache */


/* ----------------------------------------------------
 * gap_gve_story_remove_tmp_audiofiles
 * ----------------------------------------------------
 */
void
gap_gve_story_remove_tmp_audiofiles(GapGveStoryVidHandle *vidhand)
{
  GapGveStoryAudioRangeElem *aud_elem;

  if(gap_debug)  printf("gap_gve_story_remove_tmp_audiofiles START\n");

  for(aud_elem = vidhand->aud_list; aud_elem != NULL; aud_elem = (GapGveStoryAudioRangeElem *)aud_elem->next)
  {
    if(aud_elem->tmp_audiofile)
    {
      if(gap_debug) printf("gap_gve_story_remove_tmp_audiofiles removing: %s\n", aud_elem->tmp_audiofile);
      remove(aud_elem->tmp_audiofile);
      g_free(aud_elem->tmp_audiofile);
      aud_elem->tmp_audiofile = NULL;
    }
  }

}  /* end gap_gve_story_remove_tmp_audiofiles */

/* ----------------------------------------------------
 * p_load_cache_audio
 * ----------------------------------------------------
 */
static GapGveStoryAudioCacheElem  *
p_load_cache_audio( char* filename, gint32 *audio_id, gint32 *aud_bytelength, gint32 seek_idx)
{
  gint32 l_idx;
  gint32 l_audio_id;
  GapGveStoryAudioCacheElem  *ac_ptr;
  GapGveStoryAudioCacheElem  *ac_last;
  GapGveStoryAudioCacheElem  *ac_new;
  GapGveStoryAudioCache  *audcache;
  guchar        *aud_data;


  if(filename == NULL)
  {
    printf("p_load_cache_audio: ** ERROR cant load filename == NULL!\n");
    return NULL;
  }


  if(global_audcache == NULL)
  {
    /* init the global_mage cache */
    global_audcache = g_malloc0(sizeof(GapGveStoryAudioCache));
    global_audcache->ac_list = NULL;
    global_audcache->nextval_audio_id = 0;
    global_audcache->max_aud_cache = MAX_AUD_CACHE_ELEMENTS;
  }

  audcache = global_audcache;
  ac_last = audcache->ac_list;

  l_idx = 0;
  for(ac_ptr = audcache->ac_list; ac_ptr != NULL; ac_ptr = (GapGveStoryAudioCacheElem *)ac_ptr->next)
  {
    l_idx++;
    if(strcmp(filename, ac_ptr->filename) == 0)
    {
      /* audio found in cache, can skip load */
      *audio_id       = ac_ptr->audio_id;
      *aud_bytelength = ac_ptr->aud_bytelength;

      return(ac_ptr);
    }
    ac_last = ac_ptr;
  }

  aud_data = g_malloc(AUDIO_SEGMENT_SIZE);
  *aud_bytelength = gap_file_get_filesize(filename);

  ac_new = NULL;
  if(aud_data != NULL)
  {
    l_audio_id = global_audcache->nextval_audio_id;
    global_audcache->nextval_audio_id++;

    *audio_id = l_audio_id;
    ac_new = g_malloc0(sizeof(GapGveStoryAudioCacheElem));
    ac_new->filename = g_strdup(filename);
    ac_new->audio_id = l_audio_id;
    ac_new->aud_data = aud_data;
    ac_new->aud_bytelength = *aud_bytelength;

    ac_new->segment_startoffset = seek_idx;
    ac_new->segment_bytelength = gap_file_load_file_segment(filename
                                                    ,ac_new->aud_data
                                                    ,seek_idx
                                                    ,AUDIO_SEGMENT_SIZE
                                                    );



    if(audcache->ac_list == NULL)
    {
      audcache->ac_list = ac_new;   /* 1.st elem starts the list */
    }
    else
    {
      ac_last->next = (GapGveStoryAudioCacheElem *)ac_new;  /* add new elem at end of the cache list */
    }

    if(l_idx > audcache->max_aud_cache)
    {
      /* chache list has more elements than desired,
       * drop the 1.st (oldest) entry in the chache list
       */
      /* p_drop_audio_cache_elem1(audcache); */  /* keep all audio files cached */
    }
  }

  return(ac_new);
}  /* end p_load_cache_audio */



/* ----------------------------------------------------
 * p_clear_layer
 * ----------------------------------------------------
 * set layer to unique color
 */
static void
p_clear_layer(gint32 layer_id
                             ,guchar red
                             ,guchar green
                             ,guchar blue
                             ,guchar alpha
                             )
{
  gint32 image_id;

  image_id = gimp_drawable_get_image(layer_id);

  if(alpha==0)
  {
    gimp_selection_none(image_id);
    gimp_edit_clear(layer_id);
  }
  else
  {
    GimpRGB  color;

    color.r = red;
    color.g = green;
    color.b = blue;
    color.a = alpha;
    gimp_selection_all(image_id);
    gimp_context_set_background(&color);
    gimp_drawable_fill(layer_id, GIMP_BACKGROUND_FILL);
    gimp_selection_none(image_id);
  }

}  /* end p_clear_layer */


/* ----------------------------------------------------
 * p_find_min_max_vid_tracknumbers
 * ----------------------------------------------------
 * findout the lowest and highest track number used
 * in the framerange list
 */
static void
p_find_min_max_vid_tracknumbers(GapGveStoryFrameRangeElem *frn_list
                             , gint32 *lowest_tracknr
                             , gint32 *highest_tracknr
                             )
{
  GapGveStoryFrameRangeElem *frn_elem;

  *lowest_tracknr = GAP_STB_MAX_VID_TRACKS;
  *highest_tracknr = -1;

  for(frn_elem = frn_list; frn_elem != NULL; frn_elem = (GapGveStoryFrameRangeElem *)frn_elem->next)
  {
    if (frn_elem->track > *highest_tracknr)
    {
      *highest_tracknr = frn_elem->track;
    }
    if (frn_elem->track < *lowest_tracknr)
    {
      *lowest_tracknr = frn_elem->track;
    }

  }

  if(gap_debug) printf("p_find_min_max_vid_tracknumbers: min:%d max:%d\n", (int)*lowest_tracknr, (int)*highest_tracknr);

}  /* end p_find_min_max_vid_tracknumbers */



/* ----------------------------------------------------
 * p_find_min_max_aud_tracknumbers
 * ----------------------------------------------------
 * findout the lowest and highest audio track number used
 * in the audiorange list
 */
static void
p_find_min_max_aud_tracknumbers(GapGveStoryAudioRangeElem *aud_list
                              , gint32 *lowest_tracknr
                              , gint32 *highest_tracknr)
{
  GapGveStoryAudioRangeElem *aud_elem;

  *lowest_tracknr = GAP_STB_MAX_VID_TRACKS;
  *highest_tracknr = -1;

  for(aud_elem = aud_list; aud_elem != NULL; aud_elem = (GapGveStoryAudioRangeElem *)aud_elem->next)
  {
    if (aud_elem->track > *highest_tracknr)
    {
      *highest_tracknr = aud_elem->track;
    }
    if (aud_elem->track < *lowest_tracknr)
    {
      *lowest_tracknr = aud_elem->track;
    }
  }

  if(gap_debug) printf("p_find_min_aud_vid_tracknumbers: min:%d max:%d\n", (int)*lowest_tracknr, (int)*highest_tracknr);

}  /* end p_find_min_max_aud_tracknumbers */


/* ---------------------------------
 * gap_gve_story_calc_audio_playtime
 * ---------------------------------
 * - check all audio tracks for audio playtime
 *   set *aud_total_sec to the playtime of the
 *   logest audio track playtime.
 */
void
gap_gve_story_calc_audio_playtime(GapGveStoryVidHandle *vidhand, gdouble *aud_total_sec)
{
  GapGveStoryAudioRangeElem *aud_elem;
  gint32 l_track;
  gint32 l_min_track;
  gint32 l_max_track;
  gdouble l_tracktime;
  gdouble l_aud_total_sec;

  l_aud_total_sec = 0;
  p_find_min_max_aud_tracknumbers(vidhand->aud_list, &l_min_track, &l_max_track);

  for(l_track=l_min_track; l_track <= l_max_track; l_track++)
  {
    l_tracktime = 0;
    for(aud_elem = vidhand->aud_list; aud_elem != NULL; aud_elem = aud_elem->next)
    {
      if(aud_elem->track == l_track)
      {
        if(aud_elem->wait_untiltime_sec > 0)
        {
          l_tracktime = MAX(l_tracktime, aud_elem->wait_untiltime_sec);
        }
        l_tracktime += aud_elem->range_playtime_sec;
      }
    }
    l_aud_total_sec = MAX(l_aud_total_sec, l_tracktime);
  }
  *aud_total_sec = l_aud_total_sec;
}  /* gap_gve_story_calc_audio_playtime */


/* ---------------------------------
 * p_get_audio_sample
 * ---------------------------------
 * - scann the aud_list to findout
 *   audiofile that is played at desired track
 *   at desired sample_index position.
 * - return the sample (amplitude) at desired sample_index position
 *   both for left and reight stereo channel.
 *   the sample is already scaled to local input track
 *   volume settings (and loacl fade_in, fade_out effects)
 */
static void
p_get_audio_sample(GapGveStoryVidHandle *vidhand        /* IN  */
                  ,gint32 track                 /* IN  */
                  ,gint32 master_sample_idx     /* IN  */
                  ,gdouble *sample_left    /* OUT (prescaled at local volume) */
                  ,gdouble *sample_right   /* OUT  (prescaled at local volume)*/
                  )
{
  GapGveStoryAudioRangeElem *aud_elem;
  GapGveStoryAudioCacheElem     *ac_elem;
  gint32  l_group_samples;
  gint32  l_byte_idx;
  gint32  l_samp_idx;               /* local track sepcific sample index */

  t_wav_16bit_int l_left;
  t_wav_16bit_int l_right;
  gdouble l_vol;
  gint32  l_range_samples;




  l_group_samples = 0;

  /* we are using master_sample_idx to access all other samples.
   * (this is faster than using time specificaton in secs (gdouble position)
   *  but requires that all samples are using the same samplerate.)
   */

  for(aud_elem = vidhand->aud_list; aud_elem != NULL; aud_elem = (GapGveStoryAudioRangeElem *)aud_elem->next)
  {
    if(aud_elem->track == track)
    {
      l_range_samples = aud_elem->range_samples
                      + MAX(0, aud_elem->wait_until_samples - l_group_samples);

      if (master_sample_idx < l_group_samples + l_range_samples)
      {
        if(aud_elem->aud_type == GAP_AUT_SILENCE)
        {
          *sample_left = 0.0;
          *sample_right = 0.0;
          return;
        }
        /* found the range that contains the sample at the requested index and track */
        l_samp_idx = master_sample_idx - l_group_samples;



        /* fetch sample (and convert to 16bit stereo) */
        l_byte_idx = aud_elem->byteoffset_rangestart + (l_samp_idx * aud_elem->bytes_per_sample);

        /* check for audio data (if not already there then load to memory) */
        if(aud_elem->aud_data == NULL)
        {
           char *l_audiofile;
           gint32  l_seek_idx;

           /* make sure segment start is header offset + a multiple of 4 */
           l_seek_idx = (l_byte_idx - aud_elem->byteoffset_data) / 4;
           l_seek_idx = aud_elem->byteoffset_data + (l_seek_idx * 4);

           l_audiofile = aud_elem->audiofile;
           if(aud_elem->tmp_audiofile)
           {
              l_audiofile = aud_elem->tmp_audiofile;
           }

           if(gap_debug) printf("BEFORE p_load_cache_audio  %s\n", l_audiofile);
           ac_elem = p_load_cache_audio(l_audiofile
                                       , &aud_elem->audio_id
                                       , &aud_elem->aud_bytelength
                                       , l_seek_idx
                                       );

           aud_elem->ac_elem = ac_elem;
           aud_elem->aud_data = ac_elem->aud_data;

           if(aud_elem->aud_data == NULL)
           {
              char *l_errtxt;

              l_errtxt = g_strdup_printf(_("cant load:  %s to memory"), l_audiofile);
              p_set_stb_error(vidhand->sterr, l_errtxt);
              g_free(l_errtxt);

              /* ERROR , audiofile was not loaded ! */
              *sample_left = 0.0;
              *sample_right = 0.0;
              return;
           }
        }

        /* check if byte_index is in the current segment */
        ac_elem = aud_elem->ac_elem;
        if(ac_elem == NULL)
        {
          printf("p_get_audio_sample: ERROR no audiosegement loaded for %s\n", aud_elem->audiofile);
          return;
        }

        if((l_byte_idx < ac_elem->segment_startoffset)
        || (l_byte_idx >= ac_elem->segment_startoffset + ac_elem->segment_bytelength))
        {
          /* the requested byte_index is outside of the currently loaded segment
           * we have to load the matching audio segment of the file
           */

          gint32  l_seek_idx;

          if(gap_debug) printf("SEGM_RELOAD l_byte_idx:%d   startoffset:%d  segm_size:%d\n", (int)l_byte_idx, (int)ac_elem->segment_startoffset ,(int)ac_elem->segment_bytelength );


          /* make sure segment start is header offset + a multiple of 4 */
          l_seek_idx = (l_byte_idx - aud_elem->byteoffset_data) / 4;
          l_seek_idx = aud_elem->byteoffset_data + (l_seek_idx * 4);

          ac_elem->segment_startoffset = l_seek_idx;
          ac_elem->segment_bytelength = gap_file_load_file_segment(ac_elem->filename
                                                           ,ac_elem->aud_data
                                                           ,l_seek_idx
                                                           ,AUDIO_SEGMENT_SIZE
                                                           );
        }


        /* reduce byte_index before accessing audio segmentdata */
        l_byte_idx -= ac_elem->segment_startoffset;

        if((l_byte_idx < 0) || (l_byte_idx >= ac_elem->segment_bytelength  /*aud_elem->aud_bytelength */ ))
        {
          printf("p_get_audio_sample: **ERROR INDEX OVERFLOW: %d (segment_bytelength: %d aud_bytelength: %d)\n"
                 , (int)l_byte_idx
                 , (int)ac_elem->segment_bytelength
                 , (int)aud_elem->aud_bytelength );
          return;
        }


        if(aud_elem->channels == 2)  /* STEREO */
        {
          if(aud_elem->bytes_per_sample == 4)
          {
            /* fetch 16bit stereosample  (byteorder lLrR lLrR) */
            l_left.bytes.lsb = aud_elem->aud_data[l_byte_idx];
            l_left.bytes.msb = aud_elem->aud_data[l_byte_idx +1];
            l_right.bytes.lsb = aud_elem->aud_data[l_byte_idx +2];
            l_right.bytes.msb = aud_elem->aud_data[l_byte_idx +3];
          }
          else
          {
            /* fetch 8bit stereosample  (byteorder LR LR) */
            gap_audio_util_dbl_sample_8_to_16(aud_elem->aud_data[l_byte_idx]
                                ,&l_left.bytes.lsb
                                ,&l_left.bytes.msb
                                );
            gap_audio_util_dbl_sample_8_to_16(aud_elem->aud_data[l_byte_idx +1]
                                ,&l_right.bytes.lsb
                                ,&l_right.bytes.msb
                                );
          }
        }
        else                         /* MONO */
        {
          if(aud_elem->bytes_per_sample == 2)
          {

            /* printf("l_byte_idx: %d   %02x %02x\n", (int)l_byte_idx
             *             ,(int)aud_elem->aud_data[l_byte_idx]
             *             ,(int)aud_elem->aud_data[l_byte_idx +1]);
             */

            /* fetch 16bit momosample  (byteorder lL lL) */
            l_left.bytes.lsb = aud_elem->aud_data[l_byte_idx];
            l_left.bytes.msb = aud_elem->aud_data[l_byte_idx +1];
          }
          else
          {
            /* fetch 8bit monosample */
            gap_audio_util_dbl_sample_8_to_16(aud_elem->aud_data[l_byte_idx]
                                ,&l_left.bytes.lsb
                                ,&l_left.bytes.msb
                                );
          }
          l_right.value = l_left.value;
        }

        /* set volume, respecting fade effects */
        l_vol = aud_elem->volume;

        if( aud_elem->fade_in_samples > 0)
        {
          if(l_samp_idx < aud_elem->fade_in_samples)
          {
            l_vol = ((l_vol - aud_elem->volume_start) * ((gdouble)l_samp_idx / (gdouble)aud_elem->fade_in_samples))
                  + aud_elem->volume_start;
          }
        }

        if( aud_elem->fade_out_samples > 0)
        {
          if(l_samp_idx > (l_range_samples - aud_elem->fade_out_samples))
          {
            l_vol = ((l_vol - aud_elem->volume_end) * ((gdouble)(l_range_samples - l_samp_idx) / (gdouble)aud_elem->fade_out_samples))
                  + aud_elem->volume_end;
          }
        }

        *sample_left = l_left.value * l_vol;
        *sample_right = l_right.value * l_vol;
        return;
      }
      l_group_samples += l_range_samples;
    }
  }

  /* the requested position is larger than the length of the requested track.
   * return silence in this case.
   */
  *sample_left = 0.0;
  *sample_right = 0.0;
  return;
}   /* end p_get_audio_sample */


/* ---------------------------------
 * p_mix_audio
 * ---------------------------------
 * mix all audiotracks to one
 * composite audio track,
 * and calculate mix scale (to fit into 16bit int)
 * optional write the mixed audio to wav file
 *  (bytesrquence LLRRLLRR)
 * or write to seperate buffers for left and right stereo channel
 * or write nothing at all.
 */
static void
p_mix_audio(FILE *fp                  /* IN: NULL: dont write to file */
           ,gint16 *bufferl           /* IN: NULL: dont write to buffer */
           ,gint16 *bufferr           /* IN: NULL: dont write to buffer */
           ,GapGveStoryVidHandle *vidhand
           ,gdouble aud_total_sec
           ,gdouble *mix_scale         /* OUT */
           )
{
  gint32 l_track;
  gint32 l_min_track;
  gint32 l_max_track;

  gdouble l_max_peak;
  gdouble l_peak_posl;
  gdouble l_peak_posr;
  gdouble l_peak_negl;
  gdouble l_peak_negr;
  gdouble l_mixl;
  gdouble l_mixr;
  gdouble l_sample_left;
  gdouble l_sample_right;
  gdouble l_master_scale;
  gint32  l_master_sample_idx;
  gint32  l_max_sample_idx;


  l_peak_posl = 0.0;
  l_peak_posr = 0.0;
  l_peak_negl = 0.0;
  l_peak_negr = 0.0;

  l_master_scale = vidhand->master_volume;

  if((fp) || (bufferl) || (bufferr))
  {
    l_master_scale = *mix_scale * vidhand->master_volume;
  }

  l_max_sample_idx = aud_total_sec * vidhand->master_samplerate;

  p_find_min_max_aud_tracknumbers(vidhand->aud_list, &l_min_track, &l_max_track);

  for(l_master_sample_idx = 0; l_master_sample_idx < l_max_sample_idx; l_master_sample_idx++)
  {
    l_mixl = 0.0;
    l_mixr = 0.0;

    *vidhand->progress = (gdouble)l_master_sample_idx / (gdouble)l_max_sample_idx;

    /* mix samples of all tracks at current index
     * (track specific volume scaling is already done in p_get_audio_sample)
     */
    for(l_track=l_min_track; l_track <= l_max_track; l_track++)
    {
      p_get_audio_sample(vidhand
                        ,l_track
                        ,l_master_sample_idx
                        ,&l_sample_left
                        ,&l_sample_right
                        );
      l_mixl += l_sample_left;
      l_mixr += l_sample_right;
    }
    l_mixl *= l_master_scale;
    l_mixr *= l_master_scale;

    if(fp)
    {
       /* wav databyte sequence is LLRR LLRR LLRR ... */
       gap_audio_wav_write_gint16(fp, (gint16)l_mixl);
       gap_audio_wav_write_gint16(fp, (gint16)l_mixr);
    }

    if(bufferl)
    {
       bufferl[l_master_sample_idx] = l_mixl;
    }
    if(bufferr)
    {
       bufferr[l_master_sample_idx] = l_mixr;
    }


    l_peak_posl = MAX(l_mixl,l_peak_posl);
    l_peak_posr = MAX(l_mixr,l_peak_posr);
    l_peak_negl = MIN(l_mixl,l_peak_negl);
    l_peak_negr = MIN(l_mixr,l_peak_negr);
  }

  l_max_peak = MAX(l_peak_posl, l_peak_posr);
  l_max_peak = MAX(l_max_peak, (-1.0 * l_peak_negl));
  l_max_peak = MAX(l_max_peak, (-1.0 * l_peak_negr));

  if (l_max_peak <= 32767)
  {
    /* max peak fits into 16 bit integer,
     * (we dont need scale down)
     */
    *mix_scale = 1.0;
  }
  else
  {
    /* must scale down ALL sample amplitudes
     * to fit into 16 bit integer
     */
    *mix_scale = 32767 / l_max_peak;
  }

}   /* end p_mix_audio */

/* ---------------------------------
 * p_check_audio_peaks
 * ---------------------------------
 */
static void
p_check_audio_peaks(GapGveStoryVidHandle *vidhand
           ,gdouble aud_total_sec
           ,gdouble *mix_scale         /* OUT */
           )
{
  /* perform mix without any output,
   * to findout peaks and calculate
   * mix_scale (how to scale to fit the result into 16 bit int)
   */
  p_mix_audio(NULL, NULL, NULL, vidhand, aud_total_sec, mix_scale);
}  /* end p_check_audio_peaks */




/* ----------------------------------------
 * gap_gve_story_create_composite_audiofile
 * ----------------------------------------
 */
gboolean
gap_gve_story_create_composite_audiofile(GapGveStoryVidHandle *vidhand
                            , char *comp_audiofile
                            )
{
  gdouble l_mix_scale;
  gdouble l_aud_total_sec;
  FILE   *l_fp;
  gboolean  l_retval;

  l_retval = FALSE;
  gap_gve_story_calc_audio_playtime(vidhand, &l_aud_total_sec);

  if(vidhand->status_msg)
  {
    g_snprintf(vidhand->status_msg, vidhand->status_msg_len, _("checking audio peaks"));
  }
  *vidhand->progress = 0.0;


  p_check_audio_peaks(vidhand, l_aud_total_sec, &l_mix_scale);
  l_fp = fopen(comp_audiofile, "wb");
  if (l_fp)
  {
    gdouble l_nsamples;

    l_nsamples = l_aud_total_sec * (gdouble)vidhand->master_samplerate;
    gap_audio_wav_write_header(l_fp
                            , (gint32)l_nsamples
                            , 2                           /* cannels */
                            , vidhand->master_samplerate
                            , 4                           /* 4bytes for 16bit stereo */
                            , 16                          /* 16 bit sample resolution */
                            );

    if(vidhand->status_msg)
    {
      g_snprintf(vidhand->status_msg, vidhand->status_msg_len, _("writing composite audiofile"));
    }
    *vidhand->progress = 0.0;


    p_mix_audio(l_fp, NULL, NULL, vidhand, l_aud_total_sec, &l_mix_scale);

    fclose(l_fp);
    l_retval = TRUE;
  }
  else
  {
    char *l_errtext;
    l_errtext = g_strdup_printf(_("cant write audio to file: %s "), comp_audiofile);
    p_set_stb_error(vidhand->sterr, l_errtext);
    g_free(l_errtext);
  }

  if(vidhand->status_msg)
  {
    g_snprintf(vidhand->status_msg, vidhand->status_msg_len, _("ready"));
  }
  return(l_retval);

}   /* end gap_gve_story_create_composite_audiofile */




/* ---------------------------------------
 * p_step_attribute
 * ---------------------------------------
 * calculate remaining attribute value after N frames_handled
 * and change *from_val and *dur accordingly
 */
static void
p_step_attribute(gint32 frames_handled
                ,gdouble *from_val
                ,gdouble *to_val
                ,gint32  *dur
                )
{
  gint32 l_rest_steps;
  gdouble step_delta_val;

  l_rest_steps =  *dur  - frames_handled;
  if((l_rest_steps <= 0) || (*dur <= 0))
  {
    *from_val = *to_val;
    *dur = 0;
  }
  else
  {
    step_delta_val =  (*to_val - *from_val) / (gdouble)(*dur);

    *from_val = *to_val - (step_delta_val * l_rest_steps) ;
    *dur =l_rest_steps;
  }
}  /* end p_step_attribute */



/* --------------------------------
 * p_step_attribute_read
 * --------------------------------
 * return attribute value after N frame steps
 */
static gdouble
p_step_attribute_read(gint32 frame_step
                ,gdouble from_val
                ,gdouble to_val
                ,gint32  dur
                )
{
  gdouble l_from_val;
  gdouble l_to_val;
  gint32  l_dur;

  l_from_val    = from_val;
  l_to_val      = to_val;
  l_dur         = dur;

  p_step_attribute( frame_step
                  , &l_from_val
                  , &l_to_val
                  , &l_dur
                  );

  return(l_from_val);

}  /* end p_step_attribute */


/* ----------------------------------------------------
 * p_fetch_framename
 * ----------------------------------------------------
 * fetch framename for a given master_frame_nr in the given video track
 * within a storyboard framerange list.
 * (simple animations without a storyboard file
 *  are represented by a short storyboard framerange list that has
 *  just one element entry at track 1).
 *
 * output gduoble attribute values (opacity, scale, move) for the frame
 * at track and master_frame_nr position.
 *
 * return the name of the frame (that is to be displayed at position master_frame_nr).
 * return NULL if there is no frame at desired track and master_frame_nr
 */
static char *
p_fetch_framename(GapGveStoryFrameRangeElem *frn_list
                 , gint32 master_frame_nr /* starts at 1 */
                 , gint32 track
                 , GapGveStoryFrameType *frn_type
                 , char **filtermacro_file
                 , gint32   *localframe_index  /* used for ANIMIMAGE and VIDEOFILES, -1 for all other types */
                 , gboolean *keep_proportions
                 , gboolean *fit_width
                 , gboolean *fit_height
                 , guchar  *red
                 , guchar  *green
                 , guchar  *blue
                 , guchar  *alpha
                 , gdouble *opacity       /* output opacity 0.0 upto 1.0 */
                 , gdouble *scale_x       /* output 0.0 upto 10.0 where 1.0 is 1:1 */
                 , gdouble *scale_y       /* output 0.0 upto 10.0 where 1.0 is 1:1 */
                 , gdouble *move_x        /* output -1.0 upto 1.0 where 0.0 is centered */
                 , gdouble *move_y        /* output -1.0 upto 1.0 where 0.0 is centered */
                 , GapGveStoryFrameRangeElem **frn_elem_ptr  /* OUT pointer to the relevant framerange element */
                 )
{
  GapGveStoryFrameRangeElem *frn_elem;
  char   *l_framename;
  gint32  l_frame_group_count;
  gint32  l_fnr;
  gint32  l_step;
  gint32  l_found_at_idx;
  gint32  l_frames_to_handle;

  l_frame_group_count = 0;
  l_framename = NULL;

  /* default attributes (used if storyboard does not define settings) */
  *opacity = 1.0;
  *scale_x = 1.0;
  *scale_y = 1.0;
  *move_x  = 0.0;
  *move_y  = 0.0;
  *localframe_index = -1;
  *frn_type = GAP_FRN_SILENCE;
  *keep_proportions = FALSE;
  *fit_width        = TRUE;
  *fit_height       = TRUE;
  *red              = 0;
  *green            = 0;
  *blue             = 0;
  *alpha            = 255;
  *filtermacro_file = NULL;
  *frn_elem_ptr      = NULL;

  l_found_at_idx=0;
  for (frn_elem = frn_list; frn_elem != NULL; frn_elem = (GapGveStoryFrameRangeElem *)frn_elem->next)
  {
    if(frn_elem->track == track)
    {
      l_frames_to_handle = frn_elem->frames_to_handle;
      if (frn_elem->wait_untiltime_sec > 0)
      {
        l_frames_to_handle += MAX(0, frn_elem->wait_untilframes - l_frame_group_count);
      }
      if (master_frame_nr <= l_frame_group_count + l_frames_to_handle)
      {
        gdouble fnr;

	fnr = (gdouble)(frn_elem->delta * (master_frame_nr - (l_frame_group_count +1 )))
	      * frn_elem->step_density;
        l_fnr = frn_elem->frame_from + (gint32)fnr;

        if((frn_elem->frn_type == GAP_FRN_SILENCE)
        || (frn_elem->frn_type == GAP_FRN_COLOR))
        {
          l_framename = NULL;   /* there is no filename for video silence or unicolor */
        }
        if(frn_elem->frn_type == GAP_FRN_IMAGE)
        {
          l_framename = g_strdup(frn_elem->basename);   /* use 1:1 basename for single images */
        }
        if(frn_elem->frn_type == GAP_FRN_ANIMIMAGE )
        {
          l_framename = g_strdup(frn_elem->basename);   /* use 1:1 basename for ainimated single images */
          *localframe_index = l_fnr;                    /* local frame number is index in the layerstack */
        }
        if(frn_elem->frn_type == GAP_FRN_MOVIE )
        {
          l_framename = g_strdup(frn_elem->basename);   /* use 1:1 basename for videofiles */
          *localframe_index = l_fnr;                    /* local frame number is the wanted video frame number */
        }
        if(frn_elem->frn_type == GAP_FRN_FRAMES)
        {
          l_framename = gap_lib_alloc_fname(frn_elem->basename
                                   ,l_fnr
                                   ,frn_elem->ext
                                   );
        }
         /* return values for current fixed attribute settings
          */
         *frn_type         = frn_elem->frn_type;
         *keep_proportions = frn_elem->keep_proportions;
         *fit_width        = frn_elem->fit_width;
         *fit_height       = frn_elem->fit_height;
         *red              = frn_elem->red;
         *green            = frn_elem->green;
         *blue             = frn_elem->blue;
         *alpha            = frn_elem->alpha;
         *filtermacro_file = frn_elem->filtermacro_file;

         frn_elem->last_master_frame_access = master_frame_nr;

         *frn_elem_ptr     = frn_elem;  /* deliver pointer to the current frn_elem */


         /* calculate effect attributes for the current step
          * where l_step = 0 at the 1.st frame of the local range
          */
         l_step = (master_frame_nr - 1) - l_frame_group_count;



         *opacity = p_step_attribute_read(l_step
                                    , frn_elem->opacity_from
                                    , frn_elem->opacity_to
                                    , frn_elem->opacity_dur
                                    );
         *scale_x = p_step_attribute_read(l_step
                                    , frn_elem->scale_x_from
                                    , frn_elem->scale_x_to
                                    , frn_elem->scale_x_dur
                                    );
         *scale_y = p_step_attribute_read(l_step
                                    , frn_elem->scale_y_from
                                    , frn_elem->scale_y_to
                                    , frn_elem->scale_y_dur
                                    );
         *move_x  = p_step_attribute_read(l_step
                                    , frn_elem->move_x_from
                                    , frn_elem->move_x_to
                                    , frn_elem->move_x_dur
                                    );
         *move_y  = p_step_attribute_read(l_step
                                    , frn_elem->move_y_from
                                    , frn_elem->move_y_to
                                    , frn_elem->move_y_dur
                                    );
        break;
      }
      l_frame_group_count += l_frames_to_handle;
    }
    l_found_at_idx++;
  }

  if( /* 1==1 */ gap_debug)
  {
   printf("p_fetch_framename: track:%d master_frame_nr:%d framename:%s: found_at_idx:%d opa:%f scale:%f %f move:%f %f layerstack_idx:%d\n"
       ,(int)track
       ,(int)master_frame_nr
       , l_framename
       ,(int)l_found_at_idx
       , (float)*opacity
       , (float)*scale_x
       , (float)*scale_y
       , (float)*move_x
       , (float)*move_y
       , (int)*localframe_index
       );
  }

  return l_framename;
}       /* end p_fetch_framename */



/* ----------------------------------------------------
 * p_make_abspath_filename
 * ----------------------------------------------------
 * check if filename is specified with absolute path,
 * if true: return 1:1 copy of filename
 * if false: prefix filename with path from storyboard file.
 */
static char *
p_make_abspath_filename(const char *filename, const char *storyboard_file)
{
    gboolean l_path_is_relative;
    char    *l_storyboard_path;
    char    *l_ptr;
    char    *l_abs_name;

    l_abs_name = NULL;
    l_path_is_relative = TRUE;
    if(filename[0] == G_DIR_SEPARATOR)
    {
      l_path_is_relative = FALSE;
    }
#ifdef G_OS_WIN32
    /* check for WIN/DOS styled abs pathname "Drive:\dir\file" */
    for(l_idx=0; filename[l_idx] != '\0'; l_idx++)
    {
      if(filename[l_idx] == DIR_ROOT)
      {
        l_path_is_relative = FALSE;
        break;
      }
    }
#endif

    if((l_path_is_relative) && (storyboard_file != NULL))
    {
      l_storyboard_path = g_strdup(storyboard_file);
      l_ptr = &l_storyboard_path[strlen(l_storyboard_path)];
      while(l_ptr != l_storyboard_path)
      {
        if((*l_ptr == G_DIR_SEPARATOR) || (*l_ptr == DIR_ROOT))
        {
          l_ptr++;
          *l_ptr = '\0';   /* terminate the string after the directorypath */
          break;
        }
        l_ptr--;
      }
      if(l_ptr != l_storyboard_path)
      {
        /* prefix the filename with the storyboard_path */
        l_abs_name  = g_strdup_printf("%s%s", l_storyboard_path, filename);
      }
      else
      {
        /* storyboard_file has no path at all (and must be in the current dir)
         * (therefore we cant expand absolute path name)
         */
        l_abs_name  = g_strdup(filename);
      }
      g_free(l_storyboard_path);
    }
    else
    {
      /* use absolute filename as it is */
      l_abs_name  = g_strdup(filename);
    }

    return(l_abs_name);

}  /* end p_make_abspath_filename */



/* ----------------------------------------------------
 * p_extract_audiopart
 * ----------------------------------------------------
 * extract the audiopart of a videofile as RIFF .WAV file
 *  call this procedure with t_GVA_Handle that is NOT NULL (after successful open)
 */
static void
p_extract_audiopart(t_GVA_Handle *gvahand, char *wavfilename, long samples_to_read)
{
  static int l_audio_channels;
  static int l_sample_rate;
  static unsigned short *left_ptr;
  static unsigned short *right_ptr;


  if(gap_debug) printf("p_extract_audiopart: START\n");

  GVA_seek_audio(gvahand, 0.0, GVA_UPOS_SECS);

  l_audio_channels = gvahand->audio_cannels;
  l_sample_rate    = gvahand->samplerate;

  if(l_audio_channels > 0)
  {
    unsigned short *l_lptr;
    unsigned short *l_rptr;
    long   l_to_read;
    long   l_left_to_read;
    long   l_block_read;
    FILE  *fp_wav;

    fp_wav = fopen(wavfilename, "wb");
    if(fp_wav)
    {
       gint32 l_bytes_per_sample;
       gint32 l_ii;

       if(l_audio_channels == 1) { l_bytes_per_sample = 2;}  /* mono */
       else                      { l_bytes_per_sample = 4;}  /* stereo */

       /* write the header */
       gap_audio_wav_write_header(fp_wav
                      , (gint32)samples_to_read
                      , l_audio_channels            /* cannels 1 or 2 */
                      , l_sample_rate
                      , l_bytes_per_sample
                      , 16                          /* 16 bit sample resolution */
                      );

      if(gap_debug) printf("samples_to_read:%d\n", (int)samples_to_read);

      /* audio block read (blocksize covers playbacktime for 250 frames */
      l_left_to_read = samples_to_read;
      l_block_read = (double)(250.0) / (double)gvahand->framerate * (double)l_sample_rate;
      l_to_read = MIN(l_left_to_read, l_block_read);

      /* allocate audio buffers */
      left_ptr = g_malloc0((sizeof(short) * l_block_read) + 16);
      right_ptr = g_malloc0((sizeof(short) * l_block_read) + 16);


      while(l_to_read > 0)
      {
         l_lptr = left_ptr;
         l_rptr = right_ptr;
         /* read the audio data of channel 0 (left or mono) */
         GVA_get_audio(gvahand
                   ,l_lptr                /* Pointer to pre-allocated buffer if int16's */
                   ,1                     /* Channel to decode */
                   ,(gdouble)l_to_read    /* Number of samples to decode */
                   ,GVA_AMOD_CUR_AUDIO    /* read from current audio position (and advance) */
                   );
        if(l_audio_channels > 1)
        {
           /* read the audio data of channel 2 (right)
            * NOTE: GVA_get_audio has advanced the stream position,
            *       so we have to set GVA_AMOD_REREAD to read from
            *       the same startposition as for channel 1 (left).
            */
           GVA_get_audio(gvahand
                     ,l_rptr                /* Pointer to pre-allocated buffer if int16's */
                     ,2                     /* Channel to decode */
                     ,l_to_read             /* Number of samples to decode */
                     ,GVA_AMOD_REREAD       /* read from */
                   );
        }
        l_left_to_read -= l_to_read;

        /* write 16 bit wave datasamples
         * sequence mono:    (lo, hi)
         * sequence stereo:  (lo_left, hi_left, lo_right, hi_right)
         */
        for(l_ii=0; l_ii < l_to_read; l_ii++)
        {
           gap_audio_wav_write_gint16(fp_wav, *l_lptr);
           l_lptr++;
           if(l_audio_channels > 1)
           {
             gap_audio_wav_write_gint16(fp_wav, *l_rptr);
             l_rptr++;
           }
         }

        l_to_read = MIN(l_left_to_read, l_block_read);

      }
      /* close wavfile */
      fclose(fp_wav);

      /* free audio buffers */
      g_free(left_ptr);
      g_free(right_ptr);
    }
  }

  if(gap_debug) printf("p_extract_audiopart: END\n");

}  /* end p_extract_audiopart */


/* ----------------------------------------------------
 * p_new_audiorange_element
 * ----------------------------------------------------
 * allocate a new GapGveStoryAudioRangeElem for storyboard processing
 * - check audiofile
 */
static GapGveStoryAudioRangeElem *
p_new_audiorange_element(GapGveStoryAudioType  aud_type
                      ,gint32 track
                      ,const char *audiofile
                      ,gint32  master_samplerate
                      ,gdouble play_from_sec
                      ,gdouble play_to_sec
                      ,gdouble volume_start
                      ,gdouble volume
                      ,gdouble volume_end
                      ,gdouble fade_in_sec
                      ,gdouble fade_out_sec
                      ,char *util_sox
                      ,char *util_sox_options
                      ,const char *storyboard_file  /* IN: NULL if no storyboard file is used */
                      ,const char *preferred_decoder
                      ,GapGveStoryAudioRangeElem *known_aud_list /* NULL or list of already known ranges */
                      ,GapGveStoryErrors *sterr           /* element to store Error/Warning report */
                      ,gint32 seltrack      /* IN: select audiotrack number 1 upto 99 for GAP_AUT_MOVIE */
                      )
{
  GapGveStoryAudioRangeElem *aud_elem;
  GapGveStoryAudioRangeElem *aud_known;
  gboolean l_audscan_required;

  //if(gap_debug)
  {
     printf("\np_new_audiorange_element: START aud_type:%d\n", (int)aud_type);
     printf("  track:%d:\n", (int)track);
     printf("  play_from_sec:%.4f:\n", (float)play_from_sec);
     printf("  play_to_sec:%.4f:\n", (float)play_to_sec);
     printf("  volume_start:%.4f:\n", (float)volume_start);
     printf("  volume:%.4f:\n", (float)volume);
     printf("  volume_end:%.4f:\n", (float)volume_end);
     printf("  fade_in_sec:%.4f:\n", (float)fade_in_sec);
     printf("  fade_out_sec:%.4f:\n", (float)fade_out_sec);

     if(audiofile)         printf("  audiofile:%s:\n", audiofile);
     if(storyboard_file)   printf("  storyboard_file:%s:\n", storyboard_file);
     if(preferred_decoder) printf("  preferred_decoder:%s:\n", preferred_decoder);
  }

  l_audscan_required = TRUE;

  aud_elem = g_malloc0(sizeof(GapGveStoryAudioRangeElem));
  aud_elem->aud_type           = aud_type;
  aud_elem->track              = track;
  aud_elem->tmp_audiofile      = NULL;
  aud_elem->audiofile          = NULL;
  aud_elem->samplerate         = master_samplerate;
  aud_elem->channels           = 2;
  aud_elem->bytes_per_sample   = 4;
  aud_elem->samples            = master_samplerate * 9999;
  aud_elem->max_playtime_sec   = 0.0;
  aud_elem->wait_untiltime_sec = 0.0;
  aud_elem->wait_until_samples = 0.0;
  aud_elem->range_playtime_sec = 0.0;
  aud_elem->play_from_sec      = play_from_sec;
  aud_elem->play_to_sec        = play_to_sec;
  aud_elem->volume_start       = volume_start;
  aud_elem->volume             = volume;
  aud_elem->volume_end         = volume_end;
  aud_elem->fade_in_sec        = fade_in_sec;
  aud_elem->fade_out_sec       = fade_out_sec;

  aud_elem->audio_id           = -1;
  aud_elem->aud_data           = NULL;
  aud_elem->ac_elem            = NULL;
  aud_elem->aud_bytelength        = 0;
  aud_elem->range_samples         = 0;
  aud_elem->fade_in_samples       = 0;
  aud_elem->fade_out_samples      = 0;
  aud_elem->byteoffset_rangestart = 0;
  aud_elem->byteoffset_data       = 44;   /* default value, will fit for most WAV files */
  aud_elem->gvahand            = NULL;
  aud_elem->seltrack           = seltrack;

  aud_elem->next               = NULL;

  if(audiofile)
  {
     aud_elem->audiofile = p_make_abspath_filename(audiofile, storyboard_file);

     if(!g_file_test(aud_elem->audiofile, G_FILE_TEST_EXISTS)) /* check for regular file */
     {
        char *l_errtxt;

        l_errtxt = g_strdup_printf(_("file not found:  %s for audioinput"), aud_elem->audiofile);
        p_set_stb_error(sterr, l_errtxt);
        g_free(l_errtxt);
        return(NULL);
     }


     /* check the list for known audioranges
      * if audiofile is already known, we can skip the file scan
      * (that can save lot of time when the file must be converted or resampled)
      */
     for(aud_known = known_aud_list; aud_known != NULL; aud_known = (GapGveStoryAudioRangeElem *)aud_known->next)
     {
        if(aud_known->audiofile)
        {
          if(gap_debug) printf("p_new_audiorange_element: check known audiofile:%s\n", aud_known->audiofile);

          if((strcmp(aud_elem->audiofile, aud_known->audiofile) == 0)
          && (aud_elem->aud_type == aud_type))
          {
            if(gap_debug)  printf("p_new_audiorange_element: Range is already known audiofile:%s\n", aud_known->audiofile);
            aud_elem->tmp_audiofile     = aud_known->tmp_audiofile;
            aud_elem->samplerate        = aud_known->samplerate;
            aud_elem->channels          = aud_known->channels;
            aud_elem->bytes_per_sample  = aud_known->bytes_per_sample;
            aud_elem->samples           = aud_known->samples;
            aud_elem->max_playtime_sec  = aud_known->max_playtime_sec;
            aud_elem->byteoffset_data   = aud_known->byteoffset_data;

            l_audscan_required = FALSE;
            break;
          }
        }
     }
     if(l_audscan_required)
     {
        if(aud_type == GAP_AUT_MOVIE)
        {
           t_GVA_Handle *gvahand;

           if(preferred_decoder)
           {
             gvahand = GVA_open_read_pref(aud_elem->audiofile
                                         , 1 /*videotrack */
                                         , seltrack
                                         , preferred_decoder
                                         , FALSE  /* use MMX if available (disable_mmx == FALSE) */
                                         );
           }
           else
           {
             gvahand = GVA_open_read(aud_elem->audiofile, 1 /*videotrack */, seltrack);
           }
           if(gvahand)
           {
             GVA_set_fcache_size(gvahand, VID_FRAMES_TO_KEEP_CACHED);
             aud_elem->samplerate        = gvahand->samplerate;
             aud_elem->channels          = gvahand->audio_cannels;
             aud_elem->bytes_per_sample  = gvahand->audio_cannels * 2;  /* API operates with 16 bit per sample */
             aud_elem->samples           = gvahand->total_aud_samples;  /* sometimes this is not an exact value, but just a guess */
             aud_elem->byteoffset_data   = 0;                           /* no headeroffset for audiopart loaded from videofiles */
             if(gap_debug)
             {
               printf("AFTER GVA_open_read: %s\n", aud_elem->audiofile);
               printf("   samplerate:      %d\n", (int)aud_elem->samplerate );
               printf("   channels:        %d\n", (int)aud_elem->channels );
               printf("   bytes_per_sample:%d\n", (int)aud_elem->bytes_per_sample );
               printf("   samples:         %d\n", (int)aud_elem->samples );
             }

             if ((aud_elem->samplerate == master_samplerate)
             && ((aud_elem->channels == 2) || (aud_elem->channels == 1)))
             {
               /* audio part of the video is OK, extract 1:1 as wavfile */
               aud_elem->tmp_audiofile = gimp_temp_name("tmp.wav");

               if(gap_debug) printf("Extracting Audiopart as file:%s\n", aud_elem->tmp_audiofile);
               p_extract_audiopart(gvahand, aud_elem->tmp_audiofile, gvahand->total_aud_samples);
               GVA_close(gvahand);
             }
             else
             {
               char *l_wavfilename;
               long samplerate;
               long channels;
               long bytes_per_sample;
               long bits;
               long samples;
               int      l_rc;

               /* extract the audiopart for resample */
               l_wavfilename = gimp_temp_name("tmp.wav");
               aud_elem->tmp_audiofile = gimp_temp_name("tmp.wav");

               if(gap_debug) printf("Resample Audiopart in file:%s outfile: %s\n", l_wavfilename, aud_elem->tmp_audiofile);
               p_extract_audiopart(gvahand, l_wavfilename, gvahand->total_aud_samples);
               GVA_close(gvahand);

               gap_gve_sox_exec_resample(l_wavfilename
                               ,aud_elem->tmp_audiofile
                               ,master_samplerate
                               ,util_sox               /* the extern converter program */
                               ,util_sox_options       /* options for the converter */
                               );

                /* check again (with the resampled copy in tmp_audiofile
                 * that should have been created by gap_gve_sox_exec_resample
                 * (if convert was OK)
                 */
                l_rc = gap_audio_wav_file_check(aud_elem->tmp_audiofile
                        , &samplerate, &channels
                        , &bytes_per_sample, &bits, &samples);

                /* delete the extracted wavfile after resampling (keep just the resampled variante) */
                remove(l_wavfilename);
                g_free(l_wavfilename);

                if((l_rc == 0)
                && ((bits == 16)    || (bits == 8))
                && ((channels == 2) || (channels == 1))
                && (samplerate == master_samplerate))
                {
                  /* audio file is OK */
                   aud_elem->samplerate        = samplerate;
                   aud_elem->channels          = channels;
                   aud_elem->bytes_per_sample  = bytes_per_sample;
                   aud_elem->samples           = samples;
                   aud_elem->byteoffset_data   = 44;
                   /* TODO: gap_audio_wav_file_check should return the real offset
                    * of the 1.st audio_sample databyte in the wav file.
                    */
                }
                else
                {
                  char *l_errtxt;
                  /* conversion failed, cant use that file, delete tmp_audiofile now */

                  if(aud_elem->tmp_audiofile)
                  {
                     remove(aud_elem->tmp_audiofile);
                     g_free(aud_elem->tmp_audiofile);
                     aud_elem->tmp_audiofile = NULL;
                  }

                  l_errtxt = g_strdup_printf(_("cant use file:  %s as audioinput"), aud_elem->audiofile);
                  p_set_stb_error(sterr, l_errtxt);
                  g_free(l_errtxt);
                  return(NULL);
                }

             }

           }
           else
           {
               char *l_errtxt;

               l_errtxt = g_strdup_printf(_("ERROR file: %s is not a supported videoformat"), aud_elem->audiofile);
               p_set_stb_error(sterr, l_errtxt);
               g_free(l_errtxt);
           }
        }
        if(aud_type == GAP_AUT_AUDIOFILE)
        {
          if(g_file_test(aud_elem->audiofile, G_FILE_TEST_EXISTS))
          {
             long samplerate;
             long channels;
             long bytes_per_sample;
             long bits;
             long samples;
             int      l_rc;

             l_rc = gap_audio_wav_file_check(aud_elem->audiofile
                     , &samplerate, &channels
                     , &bytes_per_sample, &bits, &samples);

             if(gap_debug)
             {
               printf("AFTER gap_audio_wav_file_check: %s\n", aud_elem->audiofile);
               printf("   samplerate:      %d\n", (int)samplerate );
               printf("   channels:        %d\n", (int)channels );
               printf("   bytes_per_sample:%d\n", (int)bytes_per_sample );
               printf("   bits:            %d\n", (int) bits);
               printf("   samples:         %d\n", (int)samples );
             }

             if(l_rc == 0)
             {
                aud_elem->samplerate        = samplerate;
                aud_elem->channels          = channels;
                aud_elem->bytes_per_sample  = bytes_per_sample;
                aud_elem->samples           = samples;
                aud_elem->byteoffset_data   = 44;
             }

             if((l_rc == 0)
             && ((bits == 16)    || (bits == 8))
             && ((channels == 2) || (channels == 1))
             && (samplerate == master_samplerate))
             {
               /* audio file is OK */
                aud_elem->samplerate        = samplerate;
                aud_elem->channels          = channels;
                aud_elem->bytes_per_sample  = bytes_per_sample;
                aud_elem->samples           = samples;
                aud_elem->byteoffset_data   = 44;
                /* TODO: gap_audio_wav_file_check should return the real offset
                 * of the 1.st audio_sample databyte in the wav file.
                 */
             }
             else
             {
                /* aud_elem->tmp_audiofile = g_strdup_printf("%s.tmp.wav", aud_elem->audiofile); */
                aud_elem->tmp_audiofile = gimp_temp_name("tmp.wav");
                gap_gve_sox_exec_resample(aud_elem->audiofile
                               ,aud_elem->tmp_audiofile
                               ,master_samplerate
                               ,util_sox               /* the extern converter program */
                               ,util_sox_options       /* options for the converter */
                               );
                /* check again (with the resampled copy in tmp_audiofile
                 * that should have been created by gap_gve_sox_exec_resample
                 * (if convert was OK)
                 */
                l_rc = gap_audio_wav_file_check(aud_elem->tmp_audiofile
                        , &samplerate, &channels
                        , &bytes_per_sample, &bits, &samples);

                if((l_rc == 0)
                && ((bits == 16)    || (bits == 8))
                && ((channels == 2) || (channels == 1))
                && (samplerate == master_samplerate))
                {
                  /* audio file is OK */
                   aud_elem->samplerate        = samplerate;
                   aud_elem->channels          = channels;
                   aud_elem->bytes_per_sample  = bytes_per_sample;
                   aud_elem->samples           = samples;
                   aud_elem->byteoffset_data   = 44;
                   /* TODO: gap_audio_wav_file_check should return the real offset
                    * of the 1.st audio_sample databyte in the wav file.
                    */
                }
                else
                {
                  char *l_errtxt;
                  /* conversion failed, cant use that file, delete tmp_audiofile now */

                  if(aud_elem->tmp_audiofile)
                  {
                     remove(aud_elem->tmp_audiofile);
                     g_free(aud_elem->tmp_audiofile);
                     aud_elem->tmp_audiofile = NULL;
                  }

                  l_errtxt = g_strdup_printf(_("cant use file:  %s as audioinput"), aud_elem->audiofile);
                  p_set_stb_error(sterr, l_errtxt);
                  g_free(l_errtxt);
                  return(NULL);
                }
             }

          }
        }
     }
  }

  if((aud_elem->samplerate > 0) && (aud_elem->channels > 0))
  {
    /* calculate playtime of the full audiofile */
    aud_elem->max_playtime_sec = (((gdouble)aud_elem->samples / (gdouble)aud_elem->channels) / (gdouble)aud_elem->samplerate);
  }
  else
  {
    aud_elem->max_playtime_sec = 0.0;
  }

  /* constraint range times to max_playtime_sec (full length of the audiofile in secs) */
  aud_elem->play_to_sec   = CLAMP(aud_elem->play_to_sec,    0.0, aud_elem->max_playtime_sec);
  aud_elem->play_from_sec = CLAMP(aud_elem->play_from_sec,  0.0, aud_elem->play_to_sec);

  aud_elem->range_playtime_sec = aud_elem->play_to_sec - aud_elem->play_from_sec;

  aud_elem->fade_in_sec   = CLAMP(aud_elem->fade_in_sec,    0.0, aud_elem->range_playtime_sec);
  aud_elem->fade_out_sec  = CLAMP(aud_elem->fade_out_sec,   0.0, aud_elem->range_playtime_sec);


  aud_elem->range_samples         = aud_elem->range_playtime_sec * aud_elem->samplerate;
  aud_elem->fade_in_samples       = aud_elem->fade_in_sec        * aud_elem->samplerate;
  aud_elem->fade_out_samples      = aud_elem->fade_out_sec       * aud_elem->samplerate;

  {
    gint32 boff;

    /* byte offset truncated to bytes_per_sample boundary */
    boff = (aud_elem->play_from_sec  * aud_elem->samplerate * aud_elem->bytes_per_sample) / aud_elem->bytes_per_sample;

    aud_elem->byteoffset_rangestart = (boff * aud_elem->bytes_per_sample)
                                    + aud_elem->byteoffset_data;
  }
  return(aud_elem);

}  /* end p_new_audiorange_element */



/* ----------------------------------------------------
 * p_new_framerange_element
 * ----------------------------------------------------
 * allocate a new GapGveStoryFrameRangeElem for storyboard processing
 * - read directory to check for first and last available frame Number
 *   for the given filename and extension.
 * - constraint range to available frames
 * - findout processing order (1 ascending, -1 descending)
 * - findout frames_to_handle in the given range.
 *
 * Single images are added with frn_type == GAP_FRN_IMAGE
 *  and frame_from = 1
 *  and frame_to   = N   (10 if image is to display for duration of 10 frames)
 *
 * return GapGveStoryFrameRangeElem with allocated copies of basename and ext strings.
 */
static GapGveStoryFrameRangeElem *
p_new_framerange_element(GapGveStoryFrameType  frn_type
                      ,gint32 track
                      ,const char *basename       /* basename or full imagename  for frn_type GAP_FRN_IMAGE */
                      ,const char *ext            /* NULL for frn_type GAP_FRN_IMAGE and GAP_FRN_MOVIE */
                      ,gint32  frame_from   /* IN: use -1 for first, 99999999 for last */
                      ,gint32  frame_to     /* IN: use -1 for first, 99999999 for last */
                      ,const char *storyboard_file  /* IN: NULL if no storyboard file is used */
                      ,const char *preferred_decoder  /* IN: NULL if no preferred_decoder is specified */
                      ,const char *filtermacro_file  /* IN: NULL, or name of the macro file */
                      ,GapGveStoryFrameRangeElem *frn_list /* NULL or list of already known ranges */
                      ,GapGveStoryErrors *sterr           /* element to store Error/Warning report */
                      ,gint32 seltrack      /* IN: select videotrack number 1 upto 99 for GAP_FRN_MOVIE */
                      ,gint32 exact_seek    /* IN: 0 fast seek, 1 exact seek (only for GVA Videoreads) */
                      ,gdouble delace    /* IN: 0.0 no deinterlace, 1.0-1.99 odd 2.0-2.99 even rows (only for GVA Videoreads) */
                      ,gdouble step_density    /* IN:  1==normal stepsize 1:1   0.5 == each frame twice, 2.0 only every 2nd frame */
                      )
{
  GapGveStoryFrameRangeElem *frn_elem;
  GapGveStoryFrameRangeElem *frn_known;
   char          *l_dirname;
   char          *l_dirname_ptr;
   char          *l_ptr;
   char          *l_exptr;
   char          *l_dummy;
   DIR           *l_dirp;
   struct dirent *l_dp;
   long           l_nr;
   long           l_maxnr;
   long           l_minnr;
   short          l_dirflag;
   char           dirname_buff[1024];
   gboolean       l_dirscan_required;


  if(gap_debug)
  {
     printf("\np_new_framerange_element: START frn_type:%d\n", (int)frn_type);
     printf("  track:%d:\n", (int)track);
     printf("  frame_from:%d:\n", (int)frame_from);
     printf("  frame_to:%d:\n", (int)frame_to);
     printf("  step_density:%f:\n", (float)step_density);
     if(basename)          printf("  basename:%s:\n", basename);
     if(ext)               printf("  ext:%s:\n", ext);
     if(storyboard_file)   printf("  storyboard_file:%s:\n", storyboard_file);
     if(preferred_decoder) printf("  preferred_decoder:%s:\n", preferred_decoder);
  }


  frn_elem = g_malloc0(sizeof(GapGveStoryFrameRangeElem));
  frn_elem->frn_type         = frn_type;
  frn_elem->track            = track;
  frn_elem->frame_from       = frame_from;
  frn_elem->frame_to         = frame_to;
  frn_elem->frame_first      = -1;
  frn_elem->frame_last       = -1;
  frn_elem->frames_to_handle = 0;
  frn_elem->frame_cnt        = 0;
  frn_elem->delta            = 1;
  frn_elem->step_density     = step_density;
  frn_elem->last_master_frame_access = -1;   /* -1 indicate that there was no access */

  /* default attributes (used if storyboard does not define settings) */
  frn_elem->red                = 0.0;
  frn_elem->green              = 0.0;
  frn_elem->blue               = 0.0;
  frn_elem->alpha              = 1.0;
  frn_elem->alpha_proportional = 1.0;
  frn_elem->keep_proportions   = FALSE;
  frn_elem->fit_width          = TRUE;
  frn_elem->fit_height         = TRUE;
  frn_elem->wait_untiltime_sec = 0.0;
  frn_elem->wait_untilframes   = 0;
  frn_elem->opacity_from       = 1.0;
  frn_elem->opacity_to         = 1.0;
  frn_elem->opacity_dur        = 0;
  frn_elem->scale_x_from       = 1.0;
  frn_elem->scale_x_to         = 1.0;
  frn_elem->scale_x_dur        = 0;
  frn_elem->scale_y_from       = 1.0;
  frn_elem->scale_y_to         = 1.0;
  frn_elem->scale_y_dur        = 0;
  frn_elem->move_x_from        = 0.0;
  frn_elem->move_x_to          = 0.0;
  frn_elem->move_x_dur         = 0;
  frn_elem->move_y_from        = 0.0;
  frn_elem->move_y_to          = 0.0;
  frn_elem->move_y_dur         = 0;
  frn_elem->filtermacro_file   = NULL;
  frn_elem->gvahand            = NULL;
  frn_elem->seltrack           = seltrack;
  frn_elem->exact_seek         = exact_seek;
  frn_elem->delace             = delace;
  frn_elem->next               = NULL;

  l_dirscan_required = TRUE;
  l_minnr = 99999999;
  l_maxnr = -44;         /* value -44 is indicator for file not found */

  if(ext)
  {
    if(*ext == '.')
    {
      frn_elem->ext              = g_strdup(ext);
    }
    else
    {
      frn_elem->ext              = g_strdup_printf(".%s", ext);
    }
  }

  /* check basename for absolute pathname */
  if(basename)
  {
     frn_elem->basename = p_make_abspath_filename(basename, storyboard_file);

     /* check the list for known frameranges
      * if basename is already known, we can skip the directory scan
      * (that can save lot of time when there are many files in a directory)
      */
     for(frn_known = frn_list; frn_known != NULL; frn_known = (GapGveStoryFrameRangeElem *)frn_known->next)
     {
        if(frn_known->basename)
        {
          if(gap_debug) printf("p_new_framerange_element: check known basename:%s\n", frn_known->basename);

          if((strcmp(frn_elem->basename, frn_known->basename) == 0)
          && (frn_elem->frn_type == frn_type))
          {
            if(gap_debug) printf("p_new_framerange_element: Range is already known basename:%s\n", frn_known->basename);

            l_minnr = frn_known->frame_first;
            l_maxnr = frn_known->frame_last;
            l_dirscan_required = FALSE;
            break;
          }
        }
     }

  } /* end check for absolute pathname */

  /* check filtermacro_file for absolute pathname */
  frn_elem->filtermacro_file      = NULL;
  if(filtermacro_file)
  {
    if(*filtermacro_file != '\0')
    {
      frn_elem->filtermacro_file = p_make_abspath_filename(filtermacro_file, storyboard_file);

      if(!g_file_test(frn_elem->filtermacro_file, G_FILE_TEST_EXISTS))
      {
         char *l_errtxt;

         l_errtxt = g_strdup_printf("filtermacro_file not found:  %s", frn_elem->filtermacro_file);
         p_set_stb_error(sterr, l_errtxt);
         g_free(l_errtxt);
         g_free(frn_elem->filtermacro_file);
         frn_elem->filtermacro_file = NULL;
      }
    }
  }

  if((frn_type == GAP_FRN_SILENCE)
  || (frn_type == GAP_FRN_COLOR))
  {
       l_minnr = -1;
       l_maxnr = 999999;
  }

  if((frn_type == GAP_FRN_IMAGE)
  || (frn_type == GAP_FRN_ANIMIMAGE)
  || (frn_type == GAP_FRN_MOVIE))
  {
     if(l_dirscan_required)
     {
       /* check if image file exists */
       if(g_file_test(frn_elem->basename, G_FILE_TEST_EXISTS)) /* check for regular file */
       {
         l_minnr = 1;
         l_maxnr = frame_to;

         if(frn_type == GAP_FRN_MOVIE)
         {
           t_GVA_Handle *gvahand;

           if(preferred_decoder)
           {
             gvahand = GVA_open_read_pref(frn_elem->basename
                                    , seltrack
                                    , 1 /* aud_track */
                                    , preferred_decoder
                                    , FALSE  /* use MMX if available (disable_mmx == FALSE) */
                                    );
           }
           else
           {
             gvahand = GVA_open_read(frn_elem->basename, seltrack, 1 /* aud_track */);
           }
           if(gvahand)
           {
             l_minnr = 1;
             l_maxnr = gvahand->total_frames;

             /* immediate close the video for now.
              * when we want to fetch frames (later)
              * we have to open a seperate handle for each range element
              * regardless if the range is within the same videofile or not.
              */
             GVA_close(gvahand);
           }
           else
           {
               char *l_errtxt;

               l_errtxt = g_strdup_printf(_("ERROR file: %s is not a supported videoformat"), frn_elem->basename);
               p_set_stb_error(sterr, l_errtxt);
               g_free(l_errtxt);
           }
         }
         if(frn_type == GAP_FRN_ANIMIMAGE)
         {
            gint32        l_anim_image_id;
            gint          l_nlayers;
            gint32       *l_layers_list;

            /* must load animated image to check number of layers */
            l_anim_image_id = p_load_cache_image(frn_elem->basename);
            if(l_anim_image_id >= 0)
            {
              l_layers_list = gimp_image_get_layers(l_anim_image_id, &l_nlayers);
              if(l_layers_list != NULL)
              {
                 g_free (l_layers_list);
              }
            }
            else
            {
               char *l_errtxt;

               l_errtxt = g_strdup_printf(_("loading error on animimage:  %s"), frn_elem->basename);
               p_set_stb_error(sterr, l_errtxt);
               g_free(l_errtxt);
            }
            l_minnr = 0;
            l_maxnr = l_nlayers-1;

            /* image is kept in memory (cached) and delete is deferred
             * until too many images were loaded,
             * or until explicite calls of gap_gve_story_drop_image_cache !
             */
            /* not needed !! gap_image_delete_immediate(l_anim_image_id); */
         }
       }
       else
       {
          char *l_errtxt;

          l_errtxt = g_strdup_printf(_("imagefile not found:  %s"), frn_elem->basename);
          p_set_stb_error(sterr, l_errtxt);
          g_free(l_errtxt);
       }
     }
  }

  if(frn_type == GAP_FRN_FRAMES)
  {
     if(l_dirscan_required)
     {
       l_dirname = g_strdup(frn_elem->basename);
       l_dirp = NULL;

       l_ptr = &l_dirname[strlen(l_dirname)];
       while(l_ptr != l_dirname)
       {
          if ((*l_ptr == G_DIR_SEPARATOR) || (*l_ptr == DIR_ROOT))
          {
            *l_ptr = '\0';   /* split the string into dirpath 0 basename */
            l_ptr++;
            break;           /* stop at char behind the slash */
          }
          l_ptr--;
       }

       if(gap_debug) printf("p_new_framerange_element: BASENAME:%s\n", l_ptr);

       if      (l_ptr == l_dirname)   { l_dirname_ptr = ".";  l_dirflag = 0; }
       else if (*l_dirname == '\0')   { l_dirname_ptr = G_DIR_SEPARATOR_S ; l_dirflag = 1; }
       else                           { l_dirname_ptr = l_dirname; l_dirflag = 2; }

       if(gap_debug) printf("p_new_framerange_element: DIRNAME:%s\n", l_dirname_ptr);
       l_dirp = opendir( l_dirname_ptr );

       if(!l_dirp) printf("p_new_framerange_element: can't read directory %s\n", l_dirname_ptr);
       else
       {
         while ( (l_dp = readdir( l_dirp )) != NULL )
         {

           if(gap_debug) printf("p_new_framerange_element: l_dp->d_name:%s\n", l_dp->d_name);


           /* findout extension of the directory entry name */
           l_exptr = &l_dp->d_name[strlen(l_dp->d_name)];
           while(l_exptr != l_dp->d_name)
           {
             if(*l_exptr == G_DIR_SEPARATOR) { break; }                 /* dont run into dir part */
             if(*l_exptr == '.')       { break; }
             l_exptr--;
           }
           /* l_exptr now points to the "." of the direntry (or to its begin if has no ext) */
           /* now check for equal extension */
           if((*l_exptr == '.') && (0 == strcmp(l_exptr, frn_elem->ext)))
           {
             /* build full pathname (to check if file exists) */
             switch(l_dirflag)
             {
               case 0:
                g_snprintf(dirname_buff, sizeof(dirname_buff), "%s", l_dp->d_name);
                break;
               case 1:
                g_snprintf(dirname_buff, sizeof(dirname_buff), "%c%s",  G_DIR_SEPARATOR, l_dp->d_name);
                break;
               default:
                /* UNIX:  "/dir/file"
                 * DOS:   "drv:\dir\file"
                 */
                g_snprintf(dirname_buff, sizeof(dirname_buff), "%s%c%s", l_dirname_ptr,  G_DIR_SEPARATOR,  l_dp->d_name);
                break;
             }

             if(g_file_test(dirname_buff, G_FILE_TEST_EXISTS)) /* check for regular file */
             {
               /* get basename and frame number of the directory entry */
               l_dummy = gap_lib_alloc_basename(l_dp->d_name, &l_nr);
               if(l_dummy != NULL)
               {
                   /* check for files, with equal basename (frames)
                    * (length must be greater than basepart+extension
                    * because of the frame_nr part "0000")
                    */
                   if((0 == strcmp(l_ptr, l_dummy))
                   && ( strlen(l_dp->d_name) > strlen(l_dummy) + strlen(l_exptr)  ))
                   {
                     frn_elem->frame_cnt++;


                     if(gap_debug) printf("p_new_framerange_element:  %s NR=%ld\n", l_dp->d_name, l_nr);

                     if (l_nr > l_maxnr)
                        l_maxnr = l_nr;
                     if (l_nr < l_minnr)
                        l_minnr = l_nr;
                   }

                   g_free(l_dummy);
               }
             }
           }
         }
         closedir( l_dirp );
       }

       g_free(l_dirname);

       if(l_maxnr <= -1)
       {
          char *l_errtxt;

          l_errtxt = g_strdup_printf(_("no framefile(s) found at:  %s"), frn_elem->basename);
          p_set_stb_error(sterr, l_errtxt);
          g_free(l_errtxt);
       }

     }  /* end l_dirscan_required */
  } /* end if frn_type == GAP_FRN_FRAMES */


  /* set first_frame_nr and last_frame_nr (found as "_0099" in diskfile namepart) */
  frn_elem->frame_last = l_maxnr;
  frn_elem->frame_first = MIN(l_minnr, l_maxnr);

  if(gap_debug) printf("p_new_framerange_element: l_minnr:%d l_maxnr:%d\n", (int)l_minnr, (int)l_maxnr );

  if (l_maxnr <= -44)
  {
    if(gap_debug) printf("p_new_framerange_element: ERROR no file(s) found\n");
    p_free_framerange_list(frn_elem);
    return (NULL);
  }


  /* constraint Rangevalues from */
  if(frn_elem->frame_from > frn_elem->frame_last)
  {
    frn_elem->frame_from = frn_elem->frame_last;
  }
  if(frn_elem->frame_from < frn_elem->frame_first)
  {
    frn_elem->frame_from = frn_elem->frame_first;
  }

  /* constraint Rangevalues to */
  if(frn_elem->frame_to > frn_elem->frame_last)
  {
    frn_elem->frame_to = frn_elem->frame_last;
  }
  if(frn_elem->frame_to < frn_elem->frame_first)
  {
    frn_elem->frame_to = frn_elem->frame_first;
  }

  /* check processing order delta */
  if (frn_elem->frame_from > frn_elem->frame_to)
  {
    frn_elem->delta            = -1;   /* -1 is inverse (descending) order */
  }
  else
  {
    frn_elem->delta            = 1;    /* +1 is normal (ascending) order */
  }
  
  /* calculate frames_to_handle respecting step_density */ 
  {
    gdouble fnr;

    if(frn_elem->step_density < 0)
    {
      /* force legal value */
      frn_elem->step_density = 1.0;
    }
    
    fnr = (gdouble)(ABS(frn_elem->frame_from - frn_elem->frame_to) +1);
    fnr = fnr / frn_elem->step_density;
    frn_elem->frames_to_handle = MAX((gint32)(fnr + 0.5), 1);
  }

  if(gap_debug)
  {
    if(frn_elem->basename) printf("\np_new_framerange_element: frn_elem->basename:%s:\n", frn_elem->basename);
    if(frn_elem->ext)      printf("p_new_framerange_element: frn_elem->ext:%s:\n",frn_elem->ext);
    printf("p_new_framerange_element: frn_elem->frame_from:%d:\n", (int)frn_elem->frame_from);
    printf("p_new_framerange_element: frn_elem->frame_to:%d:\n", (int)frn_elem->frame_to);
    printf("p_new_framerange_element: frn_elem->frames_to_handle:%d:\n", (int)frn_elem->frames_to_handle);
    printf("\np_new_framerange_element: END\n");
  }


  return(frn_elem);
}       /* end p_new_framerange_element */



/* --------------------------------
 * p_add_frn_list
 * --------------------------------
 */
static void
p_add_frn_list(GapGveStoryVidHandle *vidhand, GapGveStoryFrameRangeElem *frn_elem)
{
  GapGveStoryFrameRangeElem *frn_listend;


  if((vidhand)  && (frn_elem))
   {
     frn_listend = vidhand->frn_list;
     if (vidhand->frn_list == NULL)
     {
       /* 1. element (or returned list) starts frn_list */
       vidhand->frn_list = frn_elem;
     }
     else
     {
       /* link frn_elem (that can be a single ement or list) to the end of frn_list */
       frn_listend = vidhand->frn_list;
       while(frn_listend->next != NULL)
       {
          frn_listend = (GapGveStoryFrameRangeElem *)frn_listend->next;
       }
       frn_listend->next = (GapGveStoryFrameRangeElem *)frn_elem;
     }
   }
}  /* end p_add_frn_list */


/* --------------------------------
 * p_add_aud_list
 * --------------------------------
 */
static void
p_add_aud_list(GapGveStoryVidHandle *vidhand, GapGveStoryAudioRangeElem *aud_elem)
{
  GapGveStoryAudioRangeElem *aud_listend;


  if((vidhand)  && (aud_elem))
   {
     aud_listend = vidhand->aud_list;
     if (vidhand->aud_list == NULL)
     {
       /* 1. element (or returned list) starts aud_list */
       vidhand->aud_list = aud_elem;
     }
     else
     {
       /* link aud_elem (that can be a single ement or list) to the end of aud_list */
       aud_listend = vidhand->aud_list;
       while(aud_listend->next != NULL)
       {
          aud_listend = (GapGveStoryAudioRangeElem *)aud_listend->next;
       }
       aud_listend->next = (GapGveStoryAudioRangeElem *)aud_elem;
     }
   }
}  /* end p_add_aud_list */




/* ----------------------------------------------------
 * p_set_vtrack_attributes
 * ----------------------------------------------------
 * set current video track attributes for the
 * Framerange_Element. from the current attribute array.
 *
 * attribute array is changed to the state after
 * the playback of the Framerange_element.
 *
 * Example for fade_in Attribute settings:
 *    IN:    opacity_from: 0.0   opacity_to: 1.0  opacity_dur: 50 frames
 *    if the range has 50 or more frames
 *    the fade_in can be handled fully within the range,
 *    the result will be:
 *       OUT:  opacity_from: 1.0   opacity_to: 1.0  opacity_dur: 0 frames
 *
 *    if the range is for example 25 frames (smaller than the duration 50)
 *    the result will be:
 *       OUT:  opacity_from: 0.5   opacity_to: 1.0  opacity_dur: 25 frames
 *       because there are 25 rest frames to complete the fade in action
 *       in the next range (if there will be any)
 *
 */
static void
p_set_vtrack_attributes(GapGveStoryFrameRangeElem *frn_elem
                       ,GapGveStoryVTrackArray *vtarr
                      )
{
  gint l_idx;

  l_idx = frn_elem->track;

  frn_elem->alpha_proportional    = vtarr->attr[l_idx].alpha_proportional;
  frn_elem->keep_proportions      = vtarr->attr[l_idx].keep_proportions;
  frn_elem->fit_width             = vtarr->attr[l_idx].fit_width;
  frn_elem->fit_height            = vtarr->attr[l_idx].fit_height;


  frn_elem->opacity_from     = vtarr->attr[l_idx].opacity_from;
  frn_elem->opacity_to       = vtarr->attr[l_idx].opacity_to;
  frn_elem->opacity_dur      = vtarr->attr[l_idx].opacity_dur;
  frn_elem->scale_x_from     = vtarr->attr[l_idx].scale_x_from;
  frn_elem->scale_x_to       = vtarr->attr[l_idx].scale_x_to;
  frn_elem->scale_x_dur      = vtarr->attr[l_idx].scale_x_dur;
  frn_elem->scale_y_from     = vtarr->attr[l_idx].scale_y_from;
  frn_elem->scale_y_to       = vtarr->attr[l_idx].scale_y_to;
  frn_elem->scale_y_dur      = vtarr->attr[l_idx].scale_y_dur;
  frn_elem->move_x_from      = vtarr->attr[l_idx].move_x_from;
  frn_elem->move_x_to        = vtarr->attr[l_idx].move_x_to;
  frn_elem->move_x_dur       = vtarr->attr[l_idx].move_x_dur;
  frn_elem->move_y_from      = vtarr->attr[l_idx].move_y_from;
  frn_elem->move_y_to        = vtarr->attr[l_idx].move_y_to;
  frn_elem->move_y_dur       = vtarr->attr[l_idx].move_y_dur;

  p_step_attribute( frn_elem->frames_to_handle
                  , &vtarr->attr[l_idx].opacity_from
                  , &vtarr->attr[l_idx].opacity_to
                  , &vtarr->attr[l_idx].opacity_dur
                  );

  p_step_attribute( frn_elem->frames_to_handle
                  , &vtarr->attr[l_idx].scale_x_from
                  , &vtarr->attr[l_idx].scale_x_to
                  , &vtarr->attr[l_idx].scale_x_dur
                  );

  p_step_attribute( frn_elem->frames_to_handle
                  , &vtarr->attr[l_idx].scale_y_from
                  , &vtarr->attr[l_idx].scale_y_to
                  , &vtarr->attr[l_idx].scale_y_dur
                  );

  p_step_attribute( frn_elem->frames_to_handle
                  , &vtarr->attr[l_idx].move_x_from
                  , &vtarr->attr[l_idx].move_x_to
                  , &vtarr->attr[l_idx].move_x_dur
                  );

  p_step_attribute( frn_elem->frames_to_handle
                  , &vtarr->attr[l_idx].move_y_from
                  , &vtarr->attr[l_idx].move_y_to
                  , &vtarr->attr[l_idx].move_y_dur
                  );

}  /* end p_set_vtrack_attributes  */


/* ----------------------------------------------------
 * p_storyboard_analyze
 * ----------------------------------------------------
 * this procedure checks the storyboard in RAM (GapStoryBoard *stb)
 * and converts all elements to the
 * corresponding rangelist structures (audio or frames or attr tables)
 */
static void
p_storyboard_analyze(GapStoryBoard *stb
                      , GapGveStoryVTrackArray *vtarr
                      , GapGveStoryFrameRangeElem *frn_known_list
                      , GapGveStoryVidHandle *vidhand
                      )
{
  GapStoryElem *stb_elem;
  char *storyboard_file;

  GapGveStoryAudioRangeElem *aud_elem;
  GapGveStoryFrameRangeElem *frn_elem;
  GapGveStoryFrameRangeElem *frn_list;
  GapGveStoryFrameRangeElem *frn_pinglist;
  GapGveStoryErrors         *sterr;

  gint32 l_track;
  gint   l_idx;

  sterr=vidhand->sterr;
  frn_list = NULL;

  storyboard_file = stb->storyboardfile;

  /* copy Master informations: */
  if(stb->master_width > 0)
  {
    vidhand->master_width     = stb->master_width;
    vidhand->master_height    = stb->master_height;
  }
  if(stb->master_framerate > 0)
  {
    vidhand->master_framerate = stb->master_framerate;
  }
  vidhand->preferred_decoder = NULL;
  if(stb->preferred_decoder)
  {
    vidhand->preferred_decoder = g_strdup(stb->preferred_decoder);
  }

  if(stb->master_volume >= 0)
  {
    vidhand->master_volume = stb->master_volume;
  }
  if(stb->master_samplerate > 0)
  {
    vidhand->master_samplerate = stb->master_samplerate;
  }

  /* Loop foreach Element in the STB */
  for(stb_elem = stb->stb_elem; stb_elem != NULL;  stb_elem = stb_elem->next)
  {
    l_track    = stb_elem->track;
    
    switch(stb_elem->record_type)
    {
      case GAP_STBREC_ATT_OPACITY:
        vtarr->attr[l_track].opacity_from = stb_elem->att_value_from;
        vtarr->attr[l_track].opacity_to   = stb_elem->att_value_to;
        vtarr->attr[l_track].opacity_dur  = stb_elem->att_value_dur;
        break;
      case GAP_STBREC_ATT_ZOOM_X:
        vtarr->attr[l_track].scale_x_from = stb_elem->att_value_from;
        vtarr->attr[l_track].scale_x_to   = stb_elem->att_value_to;
        vtarr->attr[l_track].scale_x_dur  = stb_elem->att_value_dur;
        break;
      case GAP_STBREC_ATT_ZOOM_Y:
        vtarr->attr[l_track].scale_y_from = stb_elem->att_value_from;
        vtarr->attr[l_track].scale_y_to   = stb_elem->att_value_to;
        vtarr->attr[l_track].scale_y_dur  = stb_elem->att_value_dur;
        break;
      case GAP_STBREC_ATT_MOVE_X:
        vtarr->attr[l_track].move_x_from  = stb_elem->att_value_from;
        vtarr->attr[l_track].move_x_to    = stb_elem->att_value_to;
        vtarr->attr[l_track].move_x_dur   = stb_elem->att_value_dur;
        break;
      case GAP_STBREC_ATT_MOVE_Y:
        vtarr->attr[l_track].move_y_from  = stb_elem->att_value_from;
        vtarr->attr[l_track].move_y_to    = stb_elem->att_value_to;
        vtarr->attr[l_track].move_y_dur   = stb_elem->att_value_dur;
        break;
      case GAP_STBREC_ATT_FIT_SIZE:
        vtarr->attr[l_track].fit_width        = stb_elem->att_fit_width;
        vtarr->attr[l_track].fit_height       = stb_elem->att_fit_height;
        vtarr->attr[l_track].keep_proportions = stb_elem->att_keep_proportions;
        break;
      case GAP_STBREC_VID_SILENCE:
        if(!vidhand->ignore_video)
        {
          /* add framerange element for the current storyboard_file line */
          frn_elem = p_new_framerange_element(GAP_FRN_SILENCE
                                             , l_track
                                             , NULL            /* basename   */
                                             , NULL            /* extension  */
                                             , 1               /* frame_from */
                                             , stb_elem->nloop /* frame_to   */
                                             , storyboard_file
                                             , NULL            /* preferred_decoder */
                                             , NULL
                                             , frn_known_list
                                             , sterr
                                             , 1              /* seltrack */
                                             , 1              /* exact_seek*/
                                             , 0.0            /* delace */
					     , 1.0            /* step_density */
                                             );
          if(frn_elem)
          {
            vtarr->attr[l_track].frame_count += frn_elem->frames_to_handle;
            p_set_vtrack_attributes(frn_elem, vtarr);
            frn_elem->wait_untiltime_sec = stb_elem->vid_wait_untiltime_sec;
            frn_elem->wait_untilframes = stb_elem->vid_wait_untiltime_sec * vidhand->master_framerate;
            p_add_frn_list(vidhand, frn_elem);
          }
        }
        break;
      case GAP_STBREC_VID_COLOR:
        if(!vidhand->ignore_video)
        {
          /* add framerange element for the current storyboard_file line */
          frn_elem = p_new_framerange_element(GAP_FRN_COLOR
                                             , l_track
                                             , NULL
                                             , NULL            /* extension  */
                                             , 1               /* frame_from */
                                             , stb_elem->nloop /* frame_to   */
                                             , storyboard_file
                                             , NULL            /* referred_decoder */
                                             , NULL
                                             , frn_known_list
                                             , sterr
                                             , 1              /* seltrack */
                                             , 1              /* exact_seek*/
                                             , 0.0            /* delace */
					     , 1.0            /* step_density */
                                             );
          if(frn_elem)
          {
            frn_elem->red                = CLAMP(stb_elem->color_red   * 255, 0 ,255);
            frn_elem->green              = CLAMP(stb_elem->color_green * 255, 0 ,255);
            frn_elem->blue               = CLAMP(stb_elem->color_blue  * 255, 0 ,255);
            frn_elem->alpha              = CLAMP(stb_elem->color_alpha * 255, 0 ,255);
            vtarr->attr[l_track].frame_count += frn_elem->frames_to_handle;
            p_set_vtrack_attributes(frn_elem, vtarr);
            p_add_frn_list(vidhand, frn_elem);
          }
        }
        break;
      case GAP_STBREC_VID_IMAGE:
        if(!vidhand->ignore_video)
        {
          /* add framerange element for the current storyboard_file line */
          frn_elem = p_new_framerange_element(GAP_FRN_IMAGE
                                             , l_track
                                             , stb_elem->orig_filename
                                             , NULL            /* extension  */
                                             , 1               /* frame_from */
                                             , stb_elem->nloop /* frame_to   */
                                             , storyboard_file
                                             , vidhand->preferred_decoder
                                             , stb_elem->filtermacro_file
                                             , frn_known_list
                                             , sterr
                                             , 1              /* seltrack */
                                             , 1              /* exact_seek*/
                                             , 0.0            /* delace */
					     , 1.0            /* step_density */
                                             );
          if(frn_elem)
          {
            vtarr->attr[l_track].frame_count += frn_elem->frames_to_handle;
            p_set_vtrack_attributes(frn_elem, vtarr);
            p_add_frn_list(vidhand, frn_elem);
          }
        }
        break;
      case GAP_STBREC_VID_ANIMIMAGE:
      case GAP_STBREC_VID_FRAMES:
      case GAP_STBREC_VID_MOVIE:
        if(!vidhand->ignore_video)
        {
           GapGveStoryFrameType        l_frn_type;
           gint32   l_repcnt;
           gint32   l_sub_from;
           gint32   l_sub_to;
           gint     l_pingpong;
           char    *l_file_or_basename;
           char    *l_ext_ptr;
          
           l_ext_ptr = NULL;
           l_file_or_basename = stb_elem->orig_filename;
           l_pingpong = 1;
           if(stb_elem->playmode == GAP_STB_PM_PINGPONG)
           {
             l_pingpong = 2;
           }
           
           l_frn_type = GAP_FRN_ANIMIMAGE;
           switch(stb_elem->record_type)
           {
             case GAP_STBREC_VID_ANIMIMAGE:
               l_frn_type = GAP_FRN_ANIMIMAGE;
               l_file_or_basename = stb_elem->orig_filename;
               break;
             case GAP_STBREC_VID_FRAMES:
               l_frn_type = GAP_FRN_FRAMES;
               l_ext_ptr = stb_elem->ext;
               l_file_or_basename = stb_elem->basename;
               break;
             case GAP_STBREC_VID_MOVIE:
               l_frn_type = GAP_FRN_MOVIE;
               l_file_or_basename = stb_elem->orig_filename;
               break;
	     default:
	       break;  /* should never be reached */
           }

           frn_pinglist = frn_known_list;
           /* expand element according to pingpong and nloop settings  */
           for(l_repcnt=0; l_repcnt < stb_elem->nloop; l_repcnt++)
           {
             l_sub_from = stb_elem->from_frame;
             l_sub_to   = stb_elem->to_frame;
             for(l_idx=0; l_idx < l_pingpong; l_idx++)
             {
               /* add framerange element for the current storyboard_file line */
               frn_elem = p_new_framerange_element( l_frn_type
                                                  , l_track
                                                  , l_file_or_basename
                                                  , l_ext_ptr
                                                  , l_sub_from
                                                  , l_sub_to
                                                  , storyboard_file
                                                  , gap_story_get_preferred_decoder(stb, stb_elem)
                                                  , stb_elem->filtermacro_file
                                                  , frn_pinglist
                                                  , sterr
                                                  , stb_elem->seltrack
                                                  , stb_elem->exact_seek
                                                  , stb_elem->delace
						  , stb_elem->step_density
                                                  );
               if(frn_elem)
               {
                 vtarr->attr[l_track].frame_count += frn_elem->frames_to_handle;
                 p_set_vtrack_attributes(frn_elem, vtarr);
                 p_add_frn_list(vidhand, frn_elem);

                 frn_pinglist = frn_list;

                 /* prepare for pingpong mode with inverted range, omitting the start/end frames */
                 l_sub_from = frn_elem->frame_to   - frn_elem->delta;
                 l_sub_to   = frn_elem->frame_from + frn_elem->delta;
               }
             }
           }


        }
        break;
      case GAP_STBREC_AUD_SILENCE:
        if(!vidhand->ignore_audio)
        {
          /* add audiorange element for the current storyboard_file line */
          aud_elem = p_new_audiorange_element(GAP_AUT_SILENCE
                                             , l_track
                                             , NULL
                                             , vidhand->master_samplerate
                                             , 0              /* from_sec     */
                                             , stb_elem->aud_play_to_sec      /* to_sec       */
                                             , 0.0            /* vol_start    */
                                             , 0.0            /* volume       */
                                             , 0.0            /* vol_end      */
                                             , 0.0            /* fade_in_sec  */
                                             , 0.0            /* fade_out_sec */
                                             , vidhand->util_sox
                                             , vidhand->util_sox_options
                                             , storyboard_file
                                             , vidhand->preferred_decoder
                                             , vidhand->aud_list  /* known audio range elements */
                                             , sterr
                                             , 1              /* seltrack */
                                             );
          if(aud_elem)
          {
             aud_elem->wait_untiltime_sec = stb_elem->aud_wait_untiltime_sec;
             aud_elem->wait_until_samples = stb_elem->aud_wait_untiltime_sec * aud_elem->samplerate;
             p_add_aud_list(vidhand, aud_elem);
          }
        }
        break;
      case GAP_STBREC_AUD_SOUND:
      case GAP_STBREC_AUD_MOVIE:
        if(!vidhand->ignore_audio)
        {
          GapGveStoryAudioType  l_aud_type;
          gint     l_rix;

          if(stb_elem->record_type == GAP_STBREC_AUD_SOUND)
          {
            l_aud_type = GAP_AUT_AUDIOFILE;
          }
          else
          {
            l_aud_type = GAP_AUT_MOVIE;
          }

          for(l_rix=0; l_rix < stb_elem->nloop; l_rix++)
          {
            /* add audiorange element for the current storyboard_file line */
            aud_elem = p_new_audiorange_element(l_aud_type  /* GAP_AUT_MOVIE or GAP_AUT_AUDIOFILE */
                                           , l_track
                                           , stb_elem->aud_filename
                                           , vidhand->master_samplerate
                                           , stb_elem->aud_play_from_sec
                                           , stb_elem->aud_play_to_sec
                                           , stb_elem->aud_volume_start
                                           , stb_elem->aud_volume
                                           , stb_elem->aud_volume_end
                                           , stb_elem->aud_fade_in_sec
                                           , stb_elem->aud_fade_out_sec
                                           , vidhand->util_sox
                                           , vidhand->util_sox_options
                                           , storyboard_file
                                           , gap_story_get_preferred_decoder(stb, stb_elem)
                                           , vidhand->aud_list  /* known audio range elements */
                                           , sterr
                                           , stb_elem->aud_seltrack
                                           );
            if(aud_elem)
            {
              p_add_aud_list(vidhand, aud_elem);
            }
	  }
        }
        break;
      default:
        break;
    }
  }  /* END Loop foreach Element in the STB */

}       /* end p_storyboard_analyze */


/* ----------------------------------------------------
 * p_framerange_list_from_storyboard
 * ----------------------------------------------------
 * this procedure builds up a framerange list
 * from the  given storyboard_file.
 *
 * return the framerange_list (scanned from storyboard_file)
 */
static GapGveStoryFrameRangeElem *
p_framerange_list_from_storyboard(const char *storyboard_file
                      ,gint32 *frame_count
                      ,GapGveStoryVidHandle *vidhand
                      )
{
  GapGveStoryVTrackArray        vtarray;
  GapGveStoryVTrackArray       *vtarr;
  GapGveStoryErrors            *sterr;
  gint   l_idx;

  sterr=vidhand->sterr;
  *frame_count = 0;
  vtarr = &vtarray;
  vtarr->max_tracknum = GAP_STB_MAX_VID_TRACKS -1;

  /* clear video track attribute settings for all tracks */
  for(l_idx=0; l_idx <= vtarr->max_tracknum; l_idx++)
  {
    vtarr->attr[l_idx].keep_proportions   = FALSE;
    vtarr->attr[l_idx].fit_width          = TRUE;
    vtarr->attr[l_idx].fit_height         = TRUE;
    vtarr->attr[l_idx].alpha_proportional = 0.0;
    vtarr->attr[l_idx].frame_count   = 0;
    vtarr->attr[l_idx].opacity_from  = 1.0;
    vtarr->attr[l_idx].opacity_to    = 1.0;
    vtarr->attr[l_idx].opacity_dur   = 0;
    vtarr->attr[l_idx].scale_x_from  = 1.0;
    vtarr->attr[l_idx].scale_x_to    = 1.0;
    vtarr->attr[l_idx].scale_x_dur   = 0;
    vtarr->attr[l_idx].scale_y_from  = 1.0;
    vtarr->attr[l_idx].scale_y_to    = 1.0;
    vtarr->attr[l_idx].scale_y_dur   = 0;
    vtarr->attr[l_idx].move_x_from   = 0.0;
    vtarr->attr[l_idx].move_x_to     = 0.0;
    vtarr->attr[l_idx].move_x_dur    = 0;
    vtarr->attr[l_idx].move_y_from   = 0.0;
    vtarr->attr[l_idx].move_y_to     = 0.0;
    vtarr->attr[l_idx].move_y_dur    = 0;
  }



  /* PARSE the storyboard file, loading its elements into
   * a list of type GapStoryBoard into RAM
   */
  {
    GapStoryBoard *stb;
    
    /* load the storyboard structure from file */
    stb = gap_story_parse(storyboard_file);
    if(stb)
    {
      if(stb->errtext != NULL)
      {
	/* report the 1.st error */
	sterr->curr_nr  = stb->errline_nr;
	sterr->currline = stb->errline;
        p_set_stb_error(sterr, stb->errtext);
      }
      else
      {
        /* analyze the stb list and transform this list
	 * to vidhand->frn_list and vidhand->aud_list
	 */
        p_storyboard_analyze(stb
                            ,vtarr
                            ,vidhand->frn_list
                            ,vidhand
                            );
      }
      gap_story_free_storyboard(&stb);
    }
  }


  if(vidhand->frn_list)
  {
    /* findout total frame_count (is the max frame_count of all tracks) */
    for(l_idx=0; l_idx <= vtarr->max_tracknum; l_idx++)
    {
      if (*frame_count < vtarr->attr[l_idx].frame_count)
      {
        *frame_count = vtarr->attr[l_idx].frame_count;
      }
    }
  }
  else
  {
    *frame_count = 0;
    sterr->currline = "(eof)";

    p_set_stb_error(sterr, _("No Frames or Images found ...."));
  }

  return (vidhand->frn_list);
}       /* end p_framerange_list_from_storyboard */


/* ----------------------------------------------------
 * p_free_framerange_list
 * ----------------------------------------------------
 */
static void
p_free_framerange_list(GapGveStoryFrameRangeElem * frn_list)
{
  GapGveStoryFrameRangeElem *frn_elem;
  GapGveStoryFrameRangeElem *frn_next;

  for(frn_elem = frn_list; frn_elem != NULL; frn_elem = frn_next)
  {
    if(frn_elem->basename)          { g_free(frn_elem->basename);}
    if(frn_elem->ext)               { g_free(frn_elem->ext);}
    if(frn_elem->filtermacro_file)  { g_free(frn_elem->filtermacro_file);}

    if(frn_elem->gvahand)           { GVA_close(frn_elem->gvahand);}

    frn_next = (GapGveStoryFrameRangeElem *)frn_elem->next;
    g_free(frn_elem);
  }
}       /* end p_free_framerange_list */



/* ----------------------------------------------------
 * gap_gve_story_close_vid_handle
 * ----------------------------------------------------
 * close video handle (free framelist and errors)
 */
void
gap_gve_story_close_vid_handle(GapGveStoryVidHandle *vidhand)
{
   p_free_framerange_list(vidhand->frn_list);
   p_free_stb_error(vidhand->sterr);

   vidhand->frn_list = NULL;
   vidhand->sterr = NULL;
}  /* end gap_gve_story_close_vid_handle */


/* ----------------------------------------------------
 * gap_gve_story_set_audio_resampling_program
 * ----------------------------------------------------
 */
void
gap_gve_story_set_audio_resampling_program(GapGveStoryVidHandle *vidhand
                           , char *util_sox
                           , char *util_sox_options
                           )
{
  if(vidhand)
  {
     if(vidhand->util_sox)
     {
       g_free(vidhand->util_sox);
       vidhand->util_sox = NULL;
     }
     if(vidhand->util_sox_options)
     {
       g_free(vidhand->util_sox_options);
       vidhand->util_sox_options = NULL;
     }

     if(util_sox)
     {
       vidhand->util_sox = g_strdup(util_sox);
     }
     if(util_sox_options)
     {
       vidhand->util_sox_options = g_strdup(util_sox_options);
     }

  }
}  /* end gap_gve_story_set_audio_resampling_program */


/* ----------------------------------------------------
 * p_open_video_handle_private
 * ----------------------------------------------------
 * this procedure builds a framerange list from
 * the given storyboard file.
 * if NULL is passed as storyboard_file
 * the list is built with just one entry.
 * from basename and ext parameters.
 *
 * return framerange list
 */
static GapGveStoryVidHandle *
p_open_video_handle_private(    gboolean ignore_audio
                      , gboolean ignore_video
                      , gdouble  *progress_ptr
                      , char *status_msg
                      , gint32 status_msg_len
                      , const char *storyboard_file
                      , const char *basename
                      , const char *ext
                      , gint32  frame_from
                      , gint32  frame_to
                      , gint32 *frame_count   /* output total frame_count , or 0 on failure */
                      , gboolean do_gimp_progress
		      , GapGveTypeInputRange input_mode
		      , const char *imagename
                      )
{
  GapGveStoryVidHandle *vidhand;
  GapGveStoryFrameRangeElem *frn_elem;
  gdouble dummy_progress;

  vidhand = g_malloc0(sizeof(GapGveStoryVidHandle));

  if(progress_ptr) { vidhand->progress = progress_ptr; }
  else             { vidhand->progress = &dummy_progress; }

  if(status_msg)
  {
    vidhand->status_msg = status_msg;
    vidhand->status_msg_len = status_msg_len;
  }
  else
  {
    vidhand->status_msg = NULL;
    vidhand->status_msg_len = 0;
  }

  vidhand->frn_list = NULL;
  vidhand->preferred_decoder = NULL;
  vidhand->do_gimp_progress = do_gimp_progress;
  *vidhand->progress = 0.0;
  vidhand->sterr = p_new_stb_error();
  vidhand->master_framerate = 25.0;
  vidhand->master_width = 0;
  vidhand->master_height = 0;
  vidhand->master_samplerate = 44100;    /* 44.1 kHZ CD standard Quality */
  vidhand->master_volume     = 1.0;
  vidhand->util_sox          = NULL;     /* use DEFAULT resample program (sox), where needed */
  vidhand->util_sox_options  = NULL;     /* use DEFAULT options */
  vidhand->ignore_audio      = ignore_audio;
  vidhand->ignore_video      = ignore_video;

  global_monitor_image_id = -1;
  *frame_count = 0;

  if((storyboard_file) && (input_mode == GAP_RNGTYPE_STORYBOARD))
  {
    if(*storyboard_file != '\0')
    {
      vidhand->frn_list = p_framerange_list_from_storyboard(storyboard_file
                                                  , frame_count
                                                  , vidhand);
    }
  }

  if((vidhand->frn_list == NULL)
  && (input_mode == GAP_RNGTYPE_LAYER)
  && (imagename))
  {
      /* add element for animimage (one multilayer image) */
      frn_elem = p_new_framerange_element(GAP_FRN_ANIMIMAGE
                                         , 1           /* track */
                                         , imagename
                                         , NULL
                                         , MIN(frame_from, frame_to)
                                         , MAX(frame_to,   frame_from)
                                         , NULL       /* storyboard_file */
                                         , NULL       /* preferred_decoder */
                                         , NULL       /* filtermacro_file */
                                         , NULL       /* frn_list */
                                         , vidhand->sterr
                                         , 1          /* seltrack */
                                         , 1              /* exact_seek*/
                                         , 0.0            /* delace */
					 , 1.0            /* step_density */
                                         );
      if(frn_elem)
      {
        *frame_count = frn_elem->frames_to_handle;
        /* layerindex starts with 0, but master_index should start wit 1
         * increment by 1 to compensate. (only needed for single multilayer image encoding)
         */
        frn_elem->frame_from++;
        frn_elem->frame_to++;
      }
      vidhand->frn_list = frn_elem;
  }
  
  if((vidhand->frn_list == NULL)
  && (basename)
  && (ext))
  {
    if(input_mode == GAP_RNGTYPE_FRAMES)
    {
      /* element for framerange */
      frn_elem = p_new_framerange_element(GAP_FRN_FRAMES
                                         , 1           /* track */
                                         , basename
                                         , ext
                                         , MIN(frame_from, frame_to)
                                         , MAX(frame_to,   frame_from)
                                         , NULL       /* storyboard_file */
                                         , NULL       /* preferred_decoder */
                                         , NULL       /* filtermacro_file */
                                         , NULL       /* frn_list */
                                         , vidhand->sterr
                                         , 1          /* seltrack */
                                         , 1              /* exact_seek*/
                                         , 0.0            /* delace */
					 , 1.0            /* step_density */
                                         );
      if(frn_elem) *frame_count = frn_elem->frames_to_handle;

      /* add a SILENCE dummy (as 1.st listelement)
       * to compensate framenumbers if the range does not start with 1
       */
      vidhand->frn_list = p_new_framerange_element(GAP_FRN_SILENCE
                                         , 1               /* track */
                                         , NULL
                                         , NULL
                                         , 0               /* frame_from */
                                         , frn_elem->frame_first -1      /* frame_to */
                                         , NULL            /* storyboard_file */
                                         , NULL            /* preferred_decoder */
                                         , NULL            /* filtermacro_file */
                                         , NULL            /* frn_list */
                                         , vidhand->sterr
                                         , 1               /* seltrack */
                                         , 1              /* exact_seek*/
                                         , 0.0            /* delace */
					 , 1.0            /* step_density */
                                         );
      vidhand->frn_list->frames_to_handle = frn_elem->frame_first -1;
      vidhand->frn_list->next = frn_elem;
    }
  }

  /* p_free_stb_error(vidhand->sterr); */

  if(gap_debug) gap_gve_story_debug_print_framerange_list(vidhand->frn_list, -1);
  return(vidhand);

} /* end p_open_video_handle_private */


/* ----------------------------------------------------
 * gap_gve_story_open_extended_video_handle
 * ----------------------------------------------------
 * this procedure builds a framerange list from
 * the given storyboard file.
 * if NULL is passed as storyboard_file
 * the list is built with just one entry.
 * from basename and ext parameters.
 *
 * return framerange list
 */
GapGveStoryVidHandle *
gap_gve_story_open_extended_video_handle(    gboolean ignore_audio
                      , gboolean ignore_video
                      , gdouble  *progress_ptr
                      , char *status_msg
                      , gint32 status_msg_len
		      , GapGveTypeInputRange input_mode
		      , const char *imagename
                      , const char *storyboard_file
                      , const char *basename
                      , const char *ext
                      , gint32  frame_from
                      , gint32  frame_to
                      , gint32 *frame_count   /* output total frame_count , or 0 on failure */
                      )
{
  return(p_open_video_handle_private( ignore_audio  /* ignore_audio */
                            , ignore_video          /* dont ignore_video */
                            , progress_ptr          /* progress_ptr */
                            , status_msg            /* status_msg */
                            , status_msg_len        /* status_msg_len */
                            ,storyboard_file
                            ,basename
                            ,ext
                            ,frame_from
                            ,frame_to
                            ,frame_count
                            ,FALSE          /* DONT do_gimp_progress */
			    ,input_mode
			    ,imagename
                            )

         );

} /* end gap_gve_story_open_extended_video_handle */

/* ----------------------------------------------------
 * gap_gve_story_open_vid_handle
 * ----------------------------------------------------
 * this procedure builds a framerange list from
 * the given storyboard file.
 * if NULL is passed as storyboard_file
 * the list is built with just one entry
 * from basename and ext parameters.
 *
 * this wrapper to p_open_video_handle
 * ignores AUDIO informations in the storyboard file
 * (The encoders use this variant
 *  because the common GUI usually creates
 *  the composite audio track, and passes
 *  the resulting WAV file ready to use)
 *
 * return framerange list
 */
GapGveStoryVidHandle *
gap_gve_story_open_vid_handle(GapGveTypeInputRange input_mode
                      , gint32 image_id
		      , const char *storyboard_file
                      , const char *basename
                      , const char *ext
                      , gint32  frame_from
                      , gint32  frame_to
                      , gint32 *frame_count   /* output total frame_count , or 0 on failure */
                      )
{
  GapGveStoryVidHandle *l_vidhand;
  char *imagename;

  imagename = NULL;
  if(image_id >= 0)
  {
    imagename = gimp_image_get_filename(image_id);
  }
  l_vidhand = p_open_video_handle_private( TRUE         /* ignore_audio */
                            , FALSE        /* dont ignore_video */
                            , NULL         /* progress_ptr */
                            , NULL         /* status_msg */
                            , 0            /* status_msg_len */
                            ,storyboard_file
                            ,basename
                            ,ext
                            ,frame_from
                            ,frame_to
                            ,frame_count
                            ,TRUE          /* do_gimp_progress */
			    ,input_mode
			    ,imagename
                            );

  if(imagename)
  {
    g_free(imagename);
  }
  return(l_vidhand);
}


/* ----------------------------------------------------
 * p_exec_filtermacro
 * ----------------------------------------------------
 * - execute the (optional) filtermacro_file if not NULL
 *   (filtermacro_file is a set of one or more gimp_filter procedures
 *    with predefined parameter values)
 *   NOTE: filtermacros are not supported by gimp-1.2.2
 *         you need a PATCH to use that feature.
 */
static gint32
p_exec_filtermacro(gint32 image_id, gint32 layer_id, char *filtermacro_file)
{
  GimpParam* l_params;
  gint   l_retvals;
  gint32 l_rc_layer_id;
  gint          l_nlayers;
  gint32       *l_layers_list;

  l_rc_layer_id = layer_id;
  if (filtermacro_file)
  {
    if(*filtermacro_file != '\0')
    {
       if(gap_debug) printf("DEBUG: before execute filtermacro_file:%s image_id:%d layer_id:%d\n"
                              , filtermacro_file
                              , (int)image_id
                              , (int)layer_id
                              );

       /* execute GAP Filtermacro_file */
       l_params = gimp_run_procedure ("plug_in_filter_macro",
                     &l_retvals,
                     GIMP_PDB_INT32,    GIMP_RUN_NONINTERACTIVE,
                     GIMP_PDB_IMAGE,    image_id,
                     GIMP_PDB_DRAWABLE, layer_id,
                     GIMP_PDB_STRING,   filtermacro_file,
                     GIMP_PDB_END);

       if(l_params[0].data.d_status != GIMP_PDB_SUCCESS)
       {
         printf("ERROR: filtermacro_file:%s failed\n", filtermacro_file);
         l_rc_layer_id = -1;
       }
       g_free(l_params);

       l_layers_list = gimp_image_get_layers(image_id, &l_nlayers);
       if(l_layers_list != NULL)
       {
         g_free (l_layers_list);
       }
       if(l_nlayers > 1 )
       {
         /* merge visible layers (reduce to single layer) */
         l_rc_layer_id = gap_image_merge_visible_layers(image_id, GIMP_CLIP_TO_IMAGE);
       }

    }
  }
  return(l_rc_layer_id);
} /* end p_exec_filtermacro */

/* ----------------------------------------------------
 * p_transform_and_add_layer
 * ----------------------------------------------------
 * - copy the layer (layer_id) to the composite image
 *   using copy/paste
 * - the source Layer (layer_id) must be part of tmp_image_id
 * - set opacity, scale and move layer_id according to video attributes
 *
 * return the new created layer id in the comp_image_id
 *   (that is already added to the composte image on top of layerstack)
 */
static gint32
p_transform_and_add_layer( gint32 comp_image_id
                         , gint32 tmp_image_id
                         , gint32 layer_id
                         , gboolean keep_proportions
                         , gboolean fit_width
                         , gboolean fit_height
                         , gdouble opacity    /* 0.0 upto 1.0 */
                         , gdouble scale_x    /* 0.0 upto 10.0 where 1.0 = 1:1 */
                         , gdouble scale_y    /* 0.0 upto 10.0 where 1.0 = 1:1 */
                         , gdouble move_x     /* -1.0 upto +1.0 where 0 = no move, -1 is left outside */
                         , gdouble move_y     /* -1.0 upto +1.0 where 0 = no move, -1 is top outside */
                         , char *filtermacro_file
                         )
{
  gint32 vid_width;
  gint32 vid_height;
  gint32 l_width;
  gint32 l_height;
  gint32 l_ofsx;
  gint32 l_ofsy;
  gdouble l_opacity;
  gint32 l_new_layer_id;
  gint32 l_fsel_layer_id;

  gint32        l_tmp_width;
  gint32        l_tmp_height;
  gdouble       l_vid_prop;
  gdouble       l_img_prop;
  gint32        l_vid_width;
  gint32        l_vid_height;
  gint          l_fsel_ofsx;
  gint          l_fsel_ofsy;


  if(gap_debug) printf("p_transform_and_add_layer: called at layer_id: %d, tmp_image_id:%d\n", (int)layer_id ,(int)tmp_image_id );

  /* execute input track specific  filtermacro (optional if supplied) */
  p_exec_filtermacro(tmp_image_id
                    , layer_id
                    , filtermacro_file
                    );



  vid_width  = gimp_image_width(comp_image_id);
  vid_height = gimp_image_height(comp_image_id);

  l_tmp_width = gimp_image_width(tmp_image_id);
  l_tmp_height = gimp_image_height(tmp_image_id);

  if(keep_proportions)
  {
    l_vid_width  = vid_width;
    l_vid_height = vid_height;

    l_vid_prop = (gdouble)l_vid_width / (gdouble)l_vid_height;
    l_img_prop = (gdouble)l_tmp_width / (gdouble)l_tmp_height;

    if((fit_width) && (fit_height))
    {
      if (l_img_prop < l_vid_prop)
      {
        /* keep height, adjust width */
        l_tmp_width = l_tmp_height * l_vid_prop;
      }
      if (l_img_prop > l_vid_prop)
      {
        /* keep width, adjust height */
        l_tmp_height = l_tmp_width / l_vid_prop;
      }

      /* resize tmp_image to vid proportions */
      if ((l_tmp_width  != gimp_image_width(tmp_image_id))
      ||  (l_tmp_height != gimp_image_height(tmp_image_id)))
      {
        l_ofsx = (l_tmp_width - gimp_image_width(tmp_image_id)) / 2;
        l_ofsy = (l_tmp_height - gimp_image_height(tmp_image_id)) / 2;

        gimp_image_resize(tmp_image_id, l_tmp_width, l_tmp_height, l_ofsx, l_ofsy);

        p_layer_resize_to_imagesize(layer_id);
      }

    }
    else
    {
      if(fit_height)
      {
         l_tmp_width = vid_height * l_img_prop;
      }
      if(fit_width)
      {
         l_tmp_height = vid_width / l_img_prop;
      }
    }

  }


  if(fit_width)
  {
    l_width  = MAX(1, (vid_width  * scale_x));
  }
  else
  {
    l_width  = MAX(1, (l_tmp_width  * scale_x));
  }

  if(fit_height)
  {
    l_height = MAX(1, (vid_height * scale_y));
  }
  else
  {
    l_height = MAX(1, (l_tmp_height * scale_y));
  }


  /* check size and scale source layer_id to desired size
   * (is the Videosize * scale attributes)
   */
  if ((gimp_image_width(tmp_image_id) != l_width)
  ||  (gimp_image_height(tmp_image_id) != l_height) )
  {
    if(gap_debug) printf("DEBUG: p_transform_and_add_layer scaling layer from %dx%d to %dx%d\n"
                            , (int)gimp_image_width(tmp_image_id)
                            , (int)gimp_image_height(tmp_image_id)
                            , (int)l_width
                            , (int)l_height
                            );

    gimp_layer_scale(layer_id, l_width, l_height
                    , FALSE  /* FALSE: centered at image TRUE: centered local on layer */
                    );

  }

  /* add a new layer to composite image */
  l_new_layer_id = gimp_layer_new(comp_image_id
                              , "pasted_track"
                              , vid_width
                              , vid_height
                              , GIMP_RGBA_IMAGE
                              , 0.0         /* Opacity full transparent */
                              ,GIMP_NORMAL_MODE);
  gimp_image_add_layer(comp_image_id, l_new_layer_id, 0);
  p_clear_layer(l_new_layer_id, 0, 0, 0, 0);

  /* copy from tmp_image and paste to composite_image
   * (the prescaled layer is cliped to video
   */
  gimp_selection_none(tmp_image_id);  /* if there is no selection, copy the complete layer */
  gimp_edit_copy(layer_id);
  l_fsel_layer_id = gimp_edit_paste(l_new_layer_id, FALSE);  /* FALSE paste clear selection */


  l_ofsx  = ((vid_width/2)  + (l_width/2))  * move_x;
  l_ofsy  = ((vid_height/2) + (l_height/2)) * move_y;

  gimp_drawable_offsets(l_fsel_layer_id, &l_fsel_ofsx, &l_fsel_ofsy);


  if(gap_debug)
  {
    printf (" move_x:%.3f move_y:%.3f\n", (float)move_x , (float) move_y);
    printf (" l_width:%d l_height:%d\n", (int)l_width , (int) l_height);
    printf ("\n FSEL_OFSX:%d FSEL:OFSY:%d\n", (int)l_fsel_ofsx , (int) l_fsel_ofsy);
    printf (" OFSX:%d OFSY:%d\n", (int)l_ofsx , (int) l_ofsy);

    if((!keep_proportions) && (1==0))   /* debug code to check move operations */
    {
      p_debug_dup_image(tmp_image_id);
    }
  }

  if ((l_ofsx != 0) || (l_ofsy != 0))
  {
    gimp_layer_set_offsets(l_fsel_layer_id
                          , l_ofsx + l_fsel_ofsx
                          , l_ofsy + l_fsel_ofsy);
  }


  gimp_floating_sel_anchor(l_fsel_layer_id);

  l_opacity = CLAMP((opacity * 100.0), 0.0, 100.0);
  gimp_layer_set_opacity(l_new_layer_id, l_opacity);

  return(l_new_layer_id);
}   /* end p_transform_and_add_layer */


/* ----------------------------------------------------
 * p_create_unicolor_image
 * ----------------------------------------------------
 * - create a new image with one black filled layer
 *   (both have the requested size)
 *
 * return the new created image_id
 *   and the layer_id of the black_layer
 */
static gint32
p_create_unicolor_image(gint32 *layer_id, gint32 width , gint32 height
                       , guchar r, guchar g, guchar b, guchar a)
{
  gint32 l_empty_layer_id;
  gint32 l_image_id;

  *layer_id = -1;
  l_image_id = gimp_image_new(width, height, GIMP_RGB);
  if(l_image_id >= 0)
  {
    l_empty_layer_id = gimp_layer_new(l_image_id, "black_background",
                          width, height,
                          GIMP_RGBA_IMAGE,
                          100.0,     /* Opacity full opaque */
                          GIMP_NORMAL_MODE);
    gimp_image_add_layer(l_image_id, l_empty_layer_id, 0);

    /* clear layer to unique color */
    p_clear_layer(l_empty_layer_id, r, g, b, a);

    *layer_id = l_empty_layer_id;
  }
  return(l_image_id);
}       /* end p_create_unicolor_image */


/* ----------------------------------------------------
 * p_layer_resize_to_imagesize
 * ----------------------------------------------------
 * enlarge layer with transparent border to fit exactly
 * in the image boundaries.
 * and clip to image size if layer was bigger.
 */
static void
p_layer_resize_to_imagesize(gint32 layer_id)
{
  gint32        l_image_id;
  gint32        l_width;
  gint32        l_height;
  gint          l_ofsx;
  gint          l_ofsy;

  l_image_id = gimp_drawable_get_image(layer_id);

  /* check if resulting layer is exactly the image size
   * and resize to fit imagesize if it is not
   */
  gimp_drawable_offsets(layer_id, &l_ofsx, &l_ofsy);
  l_width = gimp_image_width(l_image_id);
  l_height = gimp_image_height(l_image_id);

  if ((l_width  != gimp_drawable_width(layer_id))
  ||  (l_height != gimp_drawable_height(layer_id))
  ||  (l_ofsx   != 0)
  ||  (l_ofsy   != 0))
  {
    gimp_layer_resize(layer_id
                     ,l_width
                     ,l_height
                     ,l_ofsx
                     ,l_ofsy);
  }
}  /* end p_layer_resize_to_imagesize */


/* ----------------------------------------------------
 * p_prepare_RGB_image
 * ----------------------------------------------------
 * prepare image for encoding
 * - clear undo stack
 * - convert to RGB
 * - merge all visible layer to one layer that
 *   fits the image size.
 *
 * return the resulting layer_id.
 */
static gint32
p_prepare_RGB_image(gint32 image_id)
{
  gint          l_nlayers;
  gint32       *l_layers_list;
  gint32 l_layer_id;

  l_layer_id = -1;
 /* dont waste time and memory for undo in noninteracive processing
  * of the frames
  */
  /*  gimp_image_undo_enable(image_id); */ /* clear undo stack */
  /* no more gimp_image_undo_enable, because this results in Warnings since gimp-2.1.6
   * Gimp-Core-CRITICAL **: file gimpimage.c: line 1708 (gimp_image_undo_thaw): assertion `gimage->undo_freeze_count > 0' failed
   */
  gimp_image_undo_disable(image_id); /*  NO Undo */

  l_layers_list = gimp_image_get_layers(image_id, &l_nlayers);
  if(l_layers_list != NULL)
  {
    l_layer_id = l_layers_list[0];
    g_free (l_layers_list);
  }

  if(l_nlayers > 1 )
  {
     if(gap_debug) printf("DEBUG: p_prepare_image merge layers tmp image\n");

     /* merge visible layers (reduce to single layer) */
     l_layer_id = gap_image_merge_visible_layers(image_id, GIMP_CLIP_TO_IMAGE);
  }

  /* convert TO RGB if needed */
  if(gimp_image_base_type(image_id) != GIMP_RGB)
  {
     gimp_image_convert_rgb(image_id);
  }

  if(l_layer_id >= 0)
  {
    p_layer_resize_to_imagesize(l_layer_id);
  }

  return(l_layer_id);
} /* end p_prepare_RGB_image */

/* ----------------------------------------------------
 * p_try_to_steal_gvahand
 * ----------------------------------------------------
 * try to steal an alread open GVA handle for video read from another
 * element.
 * conditions: must use same videofile, and exact_seek mode
 * but steal only handles that are not in current access
 * (where the last accessed master_frame_nr is lower
 * than the current one)
 */
static t_GVA_Handle *
p_try_to_steal_gvahand(GapGveStoryVidHandle *vidhand
                      , gint32 master_frame_nr
                      , char *basename             /* the videofile name */
                      , gint32 exact_seek
                      )
{
  GapGveStoryFrameRangeElem *frn_elem;
  t_GVA_Handle *gvahand;

  for (frn_elem = vidhand->frn_list; frn_elem != NULL; frn_elem = (GapGveStoryFrameRangeElem *)frn_elem->next)
  {
    if((frn_elem->exact_seek == exact_seek)
    && (frn_elem->last_master_frame_access < master_frame_nr)
    && (frn_elem->gvahand != NULL))
    {
      if(strcmp(frn_elem->basename, basename) == 0)
      {
         if(gap_debug) printf("(RE)using an already open GVA handle for video read access %s\n", frn_elem->basename);
         gvahand = frn_elem->gvahand;
         frn_elem->gvahand = NULL;   /* steal from this element */
         return(gvahand);
      }
    }
  }
  return(NULL);  /* nothing found to steal from, return NULL */

} /* end p_try_to_steal_gvahand */


/* ----------------------------------------------------
 * gap_gve_story_fetch_composite_image
 * ----------------------------------------------------
 * fetch composite VIDEO Image at a given master_frame_nr
 * within a storyboard framerange list.
 *
 * the returned image is flattend RGB and scaled to
 * desired video framesize.
 *
 *  it is a merged result of all video tracks,
 *
 *  frames at master_frame_nr were loaded
 *  for all video tracks and added to the composite image
 *   (track 0 on top, track N on bottom
 *    of the layerstack)
 *  opacity, scaling and move (decenter) attributes
 *  were set to according to current Video Attributes.
 *
 * an (optional) filtermacro_file is performed on the
 * composite image.
 *  (filtermacros are not supported by the official GIMP-1.2.2
 *   you may need patched GIMP/GAP Sourcecode to use that feature)
 *
 *
 * (simple animations without a storyboard file
 *  are represented by a short storyboard framerange list that has
 *  just one element entry at track 1).
 *
 * return image_id of resulting image and the flattened resulting layer_id
 */
gint32
gap_gve_story_fetch_composite_image(GapGveStoryVidHandle *vidhand
                    , gint32 master_frame_nr  /* starts at 1 */
                    , gint32  vid_width       /* desired Video Width in pixels */
                    , gint32  vid_height      /* desired Video Height in pixels */
                    , char *filtermacro_file  /* NULL if no filtermacro is used */
                    , gint32 *layer_id        /* output: Id of the only layer in the composite image */
                 )
{
  gint    l_track;
  gint32    l_track_min;
  gint32    l_track_max;
  gchar  *l_framename;
  gdouble l_opacity;
  gdouble l_scale_x;
  gdouble l_scale_y;
  gdouble l_move_x;
  gdouble l_move_y;
  GapGveStoryFrameRangeElem *l_frn_elem;

  gint32        l_comp_image_id;
  gint32        l_tmp_image_id;
  gint32        l_layer_id;
  gint          l_nlayers;
  gint32       *l_layers_list;
  gint32        l_localframe_index;
  gboolean      l_keep_proportions;
  gboolean      l_fit_width;
  gboolean      l_fit_height;
  GapGveStoryFrameType   l_frn_type;
  char            *l_trak_filtermacro_file;
   guchar l_red;
   guchar l_green;
   guchar l_blue;
   guchar l_alpha;


  l_comp_image_id   = -1;
  l_tmp_image_id    = -1;
  l_layer_id        = -1;
  *layer_id         = -1;

  if(gap_debug) printf("gap_gve_story_fetch_composite_image START  master_frame_nr:%d  %dx%d\n", (int)master_frame_nr, (int)vid_width, (int)vid_height );

   p_find_min_max_vid_tracknumbers(vidhand->frn_list, &l_track_min, &l_track_max);


  /* reverse order, has the effect, that track 0 is processed as last track
   * and will be put on top of the layerstack
   */
  for(l_track = MIN(GAP_STB_MAX_VID_TRACKS, l_track_max); l_track >= MAX(0, l_track_min); l_track--)
  {
    l_framename = p_fetch_framename(vidhand->frn_list
                 , master_frame_nr /* starts at 1 */
                 , l_track
                 , &l_frn_type
                 , &l_trak_filtermacro_file
                 , &l_localframe_index   /* used only for ANIMIMAGE and Videoframe Number, -1 for all other types */
                 , &l_keep_proportions
                 , &l_fit_width
                 , &l_fit_height
                 , &l_red
                 , &l_green
                 , &l_blue
                 , &l_alpha
                 , &l_opacity       /* output opacity 0.0 upto 1.0 */
                 , &l_scale_x       /* output 0.0 upto 10.0 where 1.0 is 1:1 */
                 , &l_scale_y       /* output 0.0 upto 10.0 where 1.0 is 1:1 */
                 , &l_move_x        /* output -1.0 upto 1.0 where 0.0 is centered */
                 , &l_move_y        /* output -1.0 upto 1.0 where 0.0 is centered */
                 , &l_frn_elem      /* output selected to the relevant framerange element */
                 );

     if((l_framename) || (l_frn_type == GAP_FRN_COLOR))
     {
       if(l_frn_type == GAP_FRN_COLOR)
       {
           l_tmp_image_id = p_create_unicolor_image(&l_layer_id
                                                , vid_width
                                                , vid_height
                                                , l_red
                                                , l_green
                                                , l_blue
                                                , l_alpha
                                                );

       }
       else
       {
         if(l_framename)
         {
           if((l_frn_type == GAP_FRN_ANIMIMAGE)
           || (l_frn_type == GAP_FRN_IMAGE))
           {
             gint32 l_orig_image_id;

             l_orig_image_id = p_load_cache_image(l_framename);
             gimp_selection_none(l_orig_image_id);
             if(l_frn_type == GAP_FRN_IMAGE)
             {
               l_layer_id = p_prepare_RGB_image(l_orig_image_id);
               l_tmp_image_id = gimp_image_duplicate(l_orig_image_id);
             }
             else
             {
               /* GAP_FRN_ANIMIMAGE */
               l_tmp_image_id = p_create_unicolor_image(&l_layer_id
                                                       , gimp_image_width(l_orig_image_id)
                                                       , gimp_image_height(l_orig_image_id)
                                                       , 0,0,0,0);
               gimp_layer_add_alpha(l_layer_id);
               l_layers_list = gimp_image_get_layers(l_orig_image_id, &l_nlayers);
               if(l_layers_list != NULL)
               {
                  if(l_localframe_index < l_nlayers)
                  {
                     gint32 l_fsel_layer_id;

                     gimp_drawable_set_visible(l_layers_list[l_localframe_index], TRUE);
                     p_layer_resize_to_imagesize(l_layers_list[l_localframe_index]);
                     gimp_edit_copy(l_layers_list[l_localframe_index]);
                     l_fsel_layer_id = gimp_edit_paste(l_layer_id, FALSE);  /* FALSE paste clear selection */
                     gimp_floating_sel_anchor(l_fsel_layer_id);
                  }
                  g_free (l_layers_list);
               }
             }
           }
           else
           {
             if(l_frn_type == GAP_FRN_MOVIE)
             {
               l_tmp_image_id = -1;

               /* fetch frame from a videofile (l_framename contains the videofile name) */
               if(l_frn_elem->gvahand == NULL)
               {
                  /* before we open a new GVA videohandle, lets check
                   * if another element has already opened this videofile,
                   * and reuse the already open gvahand handle if possible
                   */
                  l_frn_elem->gvahand = p_try_to_steal_gvahand(vidhand
                                                              , master_frame_nr
                                                              , l_frn_elem->basename
                                                              , l_frn_elem->exact_seek
                                                              );
                  if(l_frn_elem->gvahand == NULL)
                  {
                    if(vidhand->preferred_decoder)
                    {
                      l_frn_elem->gvahand = GVA_open_read_pref(l_framename
                                             , l_frn_elem->seltrack
                                             , 1 /* aud_track */
                                             , vidhand->preferred_decoder
                                             , FALSE  /* use MMX if available (disable_mmx == FALSE) */
                                             );
                    }
                    else
                    {
                      l_frn_elem->gvahand = GVA_open_read(l_framename
                                                        ,l_frn_elem->seltrack
                                                        ,1 /* aud_track */
                                                        );
                    }

                    if(l_frn_elem->gvahand)
                    {
                      GVA_set_fcache_size(l_frn_elem->gvahand, VID_FRAMES_TO_KEEP_CACHED);

                      l_frn_elem->gvahand->do_gimp_progress = vidhand->do_gimp_progress;
                      if(l_frn_elem->exact_seek == 1)
                      {
                        /* configure the GVA Procedures for exact (but slow) seek emulaion */
                        l_frn_elem->gvahand->emulate_seek = TRUE;
                      }
                      else
                      {
                        /* alow faster (but not not exact) seek ops */
                        l_frn_elem->gvahand->emulate_seek = FALSE;
                      }
                    }
                  }


               }

               if(l_frn_elem->gvahand)
               {
                  gint32 l_deinterlace;
                  gdouble l_threshold;
                  t_GVA_RetCode  l_fcr;

                  /* split delace value: integer part is deinterlace mode, rest is threshold */
                  l_deinterlace = l_frn_elem->delace;
                  l_threshold = l_frn_elem->delace - (gdouble)l_deinterlace;

                  /* set image and layer in the gvahand structure invalid,
                   * to force creation of a new image in the following call of  GVA_frame_to_gimp_layer
                   */
                  l_frn_elem->gvahand->image_id = -1;
                  l_frn_elem->gvahand->layer_id = -1;


                  /* attempt to read frame from the GVA API internal framecache */

                  /* printf("\nST: before  GVA_debug_print_fcache (2) #:%d\n", (int)l_localframe_index );
                   * GVA_debug_print_fcache(l_frn_elem->gvahand);
                   * printf("ST: before  GVA_frame_to_gimp_layer (2) attempt cache read  #:%d\n", (int)l_localframe_index );
                   */

                  l_fcr = GVA_frame_to_gimp_layer(l_frn_elem->gvahand
                                    , TRUE                 /* delete_mode */
                                    , l_localframe_index   /* framenumber */
                                    , l_deinterlace
                                    , l_threshold
                                    );

                  if (l_fcr != GVA_RET_OK)
                  {
                    /* if no success, we try explicite read that frame  */
                    if(l_frn_elem->gvahand->current_seek_nr != l_localframe_index)
                    {
                      if(((l_frn_elem->gvahand->current_seek_nr + VID_FRAMES_TO_KEEP_CACHED) > l_localframe_index)
                      &&  (l_frn_elem->gvahand->current_seek_nr < l_localframe_index ) )
                      {
                        /* near forward seek is performed by dummyreads to fill up the framecache
                         */
                        while(l_frn_elem->gvahand->current_seek_nr < l_localframe_index)
                        {
                          GVA_get_next_frame(l_frn_elem->gvahand);
                        }
                      }
                      else
                      {
                        if(vidhand->do_gimp_progress)
                        {
                           gimp_progress_init(_("Seek Inputvideoframe..."));
                        }
                        GVA_seek_frame(l_frn_elem->gvahand, (gdouble)l_localframe_index, GVA_UPOS_FRAMES);
                        if(vidhand->do_gimp_progress)
                        {
                           gimp_progress_init(_("Continue Encoding..."));
                        }
                     }
                    }

                    if(GVA_get_next_frame(l_frn_elem->gvahand) == GVA_RET_OK)
                    {
                      GVA_frame_to_gimp_layer(l_frn_elem->gvahand
                                      , TRUE   /* delete_mode */
                                      , l_localframe_index   /* framenumber */
                                      , l_deinterlace
                                      , l_threshold
                                      );
                    }
                  }
                  /* take the newly created image from gvahand stucture */
                  l_tmp_image_id = l_frn_elem->gvahand->image_id;
                  l_frn_elem->gvahand->image_id = -1;
                  l_frn_elem->gvahand->layer_id = -1;
               }
             }
             else
             {
               /* GAP_FRN_FRAMES
                * (l_framename  is one single imagefile out of a series of numbered imagefiles)
                */
               l_tmp_image_id = gap_lib_load_image(l_framename);
             }
           }
           g_free(l_framename);
           if(l_tmp_image_id < 0)
           {
              return -1;
           }
           l_layer_id = p_prepare_RGB_image(l_tmp_image_id);
         }
       }


       if(gap_debug) printf("p_prepare_RGB_image returned layer_id: %d, tmp_image_id:%d\n", (int)l_layer_id, (int)l_tmp_image_id);

       if(l_comp_image_id  < 0)
       {
         if((l_opacity == 1.0)
         && (l_scale_x == 1.0)
         && (l_scale_y == 1.0)
         && (l_move_x == 0.0)
         && (l_move_y == 0.0)
         && (l_fit_width)
         && (l_fit_height)
         && (!l_keep_proportions)
         && (l_trak_filtermacro_file == NULL)
         && (l_frn_type != GAP_FRN_ANIMIMAGE)
         )
         {
           /* because there are no transformations in the first handled track,
            * we can save time and directly use the loaded tmp image as base for the composite image
            */
           l_comp_image_id = l_tmp_image_id;


           /* scale image to desired Videosize */
           if ((gimp_image_width(l_comp_image_id) != vid_width)
           ||  (gimp_image_height(l_comp_image_id) != vid_height) )
           {
              if(gap_debug) printf("DEBUG: gap_gve_story_fetch_composite_image scaling composite image\n");
              gimp_image_scale(l_comp_image_id, vid_width, vid_height);
           }
         }
         else
         {
           /* create empty backgound */
           gint32 l_empty_layer_id;
           l_comp_image_id = p_create_unicolor_image(&l_empty_layer_id, vid_width, vid_height, 0,0,0,255);
         }
       }

       if(l_tmp_image_id != l_comp_image_id)
       {
         p_transform_and_add_layer(l_comp_image_id, l_tmp_image_id, l_layer_id
                                  ,l_keep_proportions
                                  ,l_fit_width
                                  ,l_fit_height
                                  ,l_opacity
                                  ,l_scale_x
                                  ,l_scale_y
                                  ,l_move_x
                                  ,l_move_y
                                  ,l_trak_filtermacro_file
                                   );
         gap_image_delete_immediate(l_tmp_image_id);
       }

     }
  }       /* end for loop over all video tracks */

  if(l_comp_image_id  < 0)
  {
    /* none of the tracks had a frame image on this master_frame_nr position
     * create a blank image (VID_SILENNCE)
     */
    l_comp_image_id = p_create_unicolor_image(&l_layer_id, vid_width, vid_height, 0,0,0,255);
  }

  /* p_debug_dup_image(l_comp_image_id); */ /* debug: display a copy of the image */


  /* check the layerstack
   */
  l_layers_list = gimp_image_get_layers(l_comp_image_id, &l_nlayers);
  if(l_layers_list != NULL)
  {
    l_layer_id = l_layers_list[0];
    g_free (l_layers_list);
  }

  /* debug feature: save the multilayer composite frame  before it is passed to the filtermacro */
  p_frame_backup_save(  GAP_VID_ENC_SAVE_MULTILAYER
                      , l_comp_image_id
                      , l_layer_id
                      , master_frame_nr
                      , TRUE               /* can be multilayer */
                      );


  if((l_nlayers > 1 )
  || (gimp_drawable_has_alpha(l_layer_id)))
  {
     if(gap_debug) printf("DEBUG: gap_gve_story_fetch_composite_image flatten Composite image\n");

     /* flatten current frame image (reduce to single layer) */
     l_layer_id = gimp_image_flatten (l_comp_image_id);
  }

  /* execute filtermacro (optional if supplied) */
  p_exec_filtermacro(l_comp_image_id
                    , l_layer_id
                    , filtermacro_file
                    );
  /* check again size and scale image to desired Videosize if needed */
  if ((gimp_image_width(l_comp_image_id) != vid_width)
  ||  (gimp_image_height(l_comp_image_id) != vid_height) )
  {
     if(gap_debug) printf("DEBUG: gap_gve_story_fetch_composite_image: scaling tmp image\n");

     gimp_image_scale(l_comp_image_id, vid_width, vid_height);
  }

  /* check again for layerstack (macro could have add more layers)
   * or there might be an alpha channel
   */
  l_layers_list = gimp_image_get_layers(l_comp_image_id, &l_nlayers);
  if(l_layers_list != NULL)
  {
    l_layer_id = l_layers_list[0];
    g_free (l_layers_list);
  }
  if((l_nlayers > 1 )
  || (gimp_drawable_has_alpha(l_layer_id)))
  {
     if(gap_debug) printf("DEBUG: gap_gve_story_fetch_composite_image  FINAL flatten Composite image\n");

      /* flatten current frame image (reduce to single layer) */
      l_layer_id = gimp_image_flatten (l_comp_image_id);
  }


  *layer_id =l_layer_id;

  /* debug feature: save the flattened composite as jpg frame  before it is passed to the encoder */
  p_frame_backup_save(  GAP_VID_ENC_SAVE_FLAT
                      , l_comp_image_id
                      , l_layer_id
                      , master_frame_nr
                      , FALSE               /* is no multilayer, use jpeg */
                      );

  /* debug feature: monitor composite image while encoding */

  p_encoding_monitor(GAP_VID_ENC_MONITOR
                    , l_comp_image_id
                    , l_layer_id
                    , master_frame_nr
                    );

  if(gap_debug) printf("gap_gve_story_fetch_composite_image END  master_frame_nr:%d  image_id:%d layer_id:%d\n", (int)master_frame_nr, (int)l_comp_image_id, (int)*layer_id );

  return(l_comp_image_id);

} /* end gap_gve_story_fetch_composite_image */


/* ----------------------------------------------------
 * p_check_chunk_fetch_possible
 * ----------------------------------------------------
 * This procedure checks the preconditions for a possible
 * fetch of already compresses MPEG chunks.
 * - there is only 1 videoinput track at this master_frame_nr
 * - the videoframe must match 1:1 in size
 * - there are no transformations (opacity, offsets ....)
 *
 * return the name of the input videofile if preconditions are OK,
 *        or NULL if not.
 */
static char *
p_check_chunk_fetch_possible(GapGveStoryVidHandle *vidhand
                    , gint32 master_frame_nr  /* starts at 1 */
                    , gint32  vid_width       /* desired Video Width in pixels */
                    , gint32  vid_height      /* desired Video Height in pixels */
                    , gint32 *video_frame_nr  /* OUT: corresponding frame number in the input video */
		    , GapGveStoryFrameRangeElem **frn_elem  /* OUT: pointer to relevant frame range element */
                    )
{
  gint    l_track;
  gint32    l_track_min;
  gint32    l_track_max;
  gchar  *l_framename;
  gchar  *l_videofile;
  gdouble l_opacity;
  gdouble l_scale_x;
  gdouble l_scale_y;
  gdouble l_move_x;
  gdouble l_move_y;
  GapGveStoryFrameRangeElem *l_frn_elem_2;

  gint32        l_localframe_index;
  gboolean      l_keep_proportions;
  gboolean      l_fit_width;
  gboolean      l_fit_height;
  GapGveStoryFrameType   l_frn_type;
  char            *l_trak_filtermacro_file;
   guchar l_red;
   guchar l_green;
   guchar l_blue;
   guchar l_alpha;


  *video_frame_nr   = -1;
  *frn_elem = NULL;

  l_videofile = NULL;
  
  p_find_min_max_vid_tracknumbers(vidhand->frn_list, &l_track_min, &l_track_max);

  /* findout if there is just one input track from type videofile
   * (that possibly could be fetched as comressed videoframe_chunk
   *  and passed 1:1 to the calling encoder)
   */
  for(l_track = MIN(GAP_STB_MAX_VID_TRACKS, l_track_max); l_track >= MAX(0, l_track_min); l_track--)
  {
    l_framename = p_fetch_framename(vidhand->frn_list
                 , master_frame_nr /* starts at 1 */
                 , l_track
                 , &l_frn_type
                 , &l_trak_filtermacro_file
                 , &l_localframe_index   /* used for ANIMIMAGE and Videoframe Number, -1 for all other types */
                 , &l_keep_proportions
                 , &l_fit_width
                 , &l_fit_height
                 , &l_red
                 , &l_green
                 , &l_blue
                 , &l_alpha
                 , &l_opacity       /* output opacity 0.0 upto 1.0 */
                 , &l_scale_x       /* output 0.0 upto 10.0 where 1.0 is 1:1 */
                 , &l_scale_y       /* output 0.0 upto 10.0 where 1.0 is 1:1 */
                 , &l_move_x        /* output -1.0 upto 1.0 where 0.0 is centered */
                 , &l_move_y        /* output -1.0 upto 1.0 where 0.0 is centered */
                 , &l_frn_elem_2    /* output selected to the relevant framerange element */
                 );

     if(gap_debug) printf("l_track:%d  l_frn_type:%d\n", (int)l_track, (int)l_frn_type);


     if((l_framename) || (l_frn_type == GAP_FRN_COLOR))
     {
       if(l_framename)
       {
         if(l_frn_type == GAP_FRN_MOVIE)
         {
           if(l_videofile == NULL)
           {
             /* check for transformations */
             if((l_opacity == 1.0)
             && (l_scale_x == 1.0)
             && (l_scale_y == 1.0)
             && (l_move_x == 0.0)
             && (l_move_y == 0.0)
             && (l_fit_width)
             && (l_fit_height)
             && (!l_keep_proportions)
             && (l_trak_filtermacro_file == NULL))
             {
               if(gap_debug) printf("gap_gve_story_fetch_composite_image_or_chunk:  video:%s\n", l_framename);
               l_videofile = g_strdup(l_framename);
               *video_frame_nr = l_localframe_index;
               *frn_elem = l_frn_elem_2;
             }
             else
             {
               if(gap_debug) printf("gap_gve_story_fetch_composite_image_or_chunk:  there are transformations\n");
               /* there are transformations, cant use compressed frame */
               l_videofile = NULL;
               break;
             }
           }
           else
           {
             if(gap_debug) printf("gap_gve_story_fetch_composite_image_or_chunk:  2 or more videotracks found\n");
             l_videofile = NULL;
             break;
           }
         }
         else
         {
             l_videofile = NULL;
             break;
         }

         g_free(l_framename);
       }
       else
       {
             l_videofile = NULL;
             break;
       }
     }
     /* else: (vid track not used) continue  */

  }       /* end for loop over all video tracks */

  return(l_videofile);  
}  /* end p_check_chunk_fetch_possible */


/* ----------------------------------------------------
 * gap_gve_story_fetch_composite_image_or_chunk
 * ----------------------------------------------------
 *
 * fetch composite VIDEO Image at a given master_frame_nr
 * within a storyboard framerange list
 *
 * if desired (and possible) try directly fetch the (already compressed) Frame chunk from
 * an input videofile for the master_frame_nr.
 *
 * the compressed fetch depends on following conditions:
 * - dont_recode_flag == TRUE
 * - there is only 1 videoinput track at this master_frame_nr
 * - the videodecoder must support a read_video_chunk procedure
 *   (currently only libmpeg3 has this support
 *    TODO: for future releases should also check for the same vcodec_name)
 * - the videoframe must match 1:1 in size
 * - there are no transformations (opacity, offsets ....)
 * - there are no filtermacros to perform on the fetched frame
 *
 * RETURN TRUE on success, FALSE on ERRORS
 *    if an already compressed video_frame_chunk was fetched then return the size of the chunk
 *        in the *video_frame_chunk_size OUT Parameter.
 *        (both *image_id an *layer_id will deliver -1 in that case)
 *    if a composite image was fetched deliver its id in the *image_id OUT parameter
 *        and the id of the only layer in the *layer_id OUT Parameter
 *        the *force_keyframe OUT parameter tells the calling encoder to write an I-Frame
 *        (*video_frame_chunk_size will deliver 0 in that case)
 */
gboolean
gap_gve_story_fetch_composite_image_or_chunk(GapGveStoryVidHandle *vidhand
                    , gint32 master_frame_nr  /* starts at 1 */
                    , gint32  vid_width       /* desired Video Width in pixels */
                    , gint32  vid_height      /* desired Video Height in pixels */
                    , char *filtermacro_file  /* NULL if no filtermacro is used */
                    , gint32 *layer_id        /* output: Id of the only layer in the composite image */

                    , gint32 *image_id        /* output: Id of the only layer in the composite image */
                    , gboolean dont_recode_flag                /* IN: TRUE try to fetch comressed chunk if possible */
                    , char *vcodec_name                        /* IN: video_codec used in the calling encoder program */
                    , gboolean *force_keyframe                 /* OUT: the calling encoder should encode an I-Frame */
                    , unsigned char *video_frame_chunk_data    /* OUT: */
                    , gint32 *video_frame_chunk_size             /* OUT: */
                    , gint32 video_frame_chunk_maxsize           /* IN: */
                    , gdouble master_framerate
                    , gint32  max_master_frame_nr   /* the number of frames that will be encode in total */
                 )
{
#define GAP_MPEG_ASSUMED_REFERENCE_DISTANCE 3
  static gint32     last_video_frame_nr = -1;
  static char      *last_videofile = NULL;
  static gboolean   last_fetch_was_compressed_chunk = FALSE;
  static gboolean   last_intra_frame_fetched = FALSE;

  gchar  *l_framename;
  gchar  *l_videofile_name;
  gchar  *l_videofile;
  GapGveStoryFrameRangeElem *l_frn_elem;
  GapGveStoryFrameRangeElem *l_frn_elem_2;

  gint32        l_video_frame_nr;
  GapGveStoryFrameType   l_frn_type;


  *image_id         = -1;
  *layer_id         = -1;
  *force_keyframe   = FALSE;
  l_frn_elem        = NULL;
  *video_frame_chunk_size = 0;

  if(gap_debug)printf("gap_gve_story_fetch_composite_image_or_chunk START  master_frame_nr:%d  %dx%d dont_recode:%d\n"
                       , (int)master_frame_nr
                       , (int)vid_width
                       , (int)vid_height
                       , (int)dont_recode_flag
                       );

  l_videofile = NULL;     /* NULL: also used as flag for "MUST fetch regular uncompressed frame" */
  l_videofile_name = NULL;
  l_framename = NULL;
  l_video_frame_nr = 1;

  if(filtermacro_file)
  {
     if(*filtermacro_file != '\0')
     {
       dont_recode_flag = FALSE;  /* if a filtermacro_file used we must force recode */
     }
  }

  /* first check if recode is forced by the calling program */
  if (dont_recode_flag)
  {
    l_videofile = p_check_chunk_fetch_possible(vidhand
                      , master_frame_nr
                      , vid_width 
                      , vid_height
                      , &l_video_frame_nr
		      , &l_frn_elem
                      );
    if(l_videofile)
    {
      l_videofile_name = g_strdup(l_videofile);
    }


    if((l_videofile) && (l_frn_elem))
    {
      if(gap_debug) printf("gap_gve_story_fetch_composite_image_or_chunk: ATTEMPT access l_videofile :%s \n", l_videofile);

       /* check if we can FETCH compressed video chunk */
      if(l_frn_elem->gvahand == NULL)
      {
         /* before we open a new GVA videohandle, lets check
          * if another element has already opened this videofile,
          * and reuse the already open gvahand handle if possible
          */
         l_frn_elem->gvahand = p_try_to_steal_gvahand(vidhand
                                                     , master_frame_nr
                                                     , l_frn_elem->basename
                                                     , l_frn_elem->exact_seek
                                                     );
         if(l_frn_elem->gvahand == NULL)
         {
           if(vidhand->preferred_decoder)
           {
             l_frn_elem->gvahand = GVA_open_read_pref(l_videofile
                                    , l_frn_elem->seltrack
                                    , 1 /* aud_track */
                                    , vidhand->preferred_decoder
                                    , FALSE  /* use MMX if available (disable_mmx == FALSE) */
                                    );
           }
           else
           {
             l_frn_elem->gvahand = GVA_open_read(l_videofile
                                               ,l_frn_elem->seltrack
                                               ,1 /* aud_track */
                                               );
           }
           if(l_frn_elem->gvahand)
           {
             GVA_set_fcache_size(l_frn_elem->gvahand, VID_FRAMES_TO_KEEP_CACHED);

             l_frn_elem->gvahand->do_gimp_progress = vidhand->do_gimp_progress;
             if(l_frn_elem->exact_seek == 1)
             {
               /* configure the GVA Procedures for exact (but slow) seek emulaion */
               l_frn_elem->gvahand->emulate_seek = TRUE;
             }
             else
             {
               /* alow faster (but not not exact) seek ops */
               l_frn_elem->gvahand->emulate_seek = FALSE;
             }
           }
         }
      }


      if(l_frn_elem->gvahand)
      {
        /* check if framesize matches 1:1 to output video size
         * and if the videodecoder does support a read procedure for compressed vodeo chunks
         * TODO: should also check for compatible vcodec_name
	 *       (cannot check that, because libmpeg3 does not deliver vcodec_name information
	 *        and there is no implementation to fetch uncompressed chunks in other decoders)
         */
        if((l_frn_elem->gvahand->width != vid_width)
        || (l_frn_elem->gvahand->height != vid_height)
        || (GVA_has_video_chunk_proc(l_frn_elem->gvahand) != TRUE) )
        {
          l_videofile = NULL;
        }
        else
        {
          t_GVA_RetCode  l_fcr;

          if(gap_debug) printf("gap_gve_story_fetch_composite_image_or_chunk:  performing CHUNK fetch\n");

          /* FETCH compressed video chunk */
          l_fcr = GVA_get_video_chunk(l_frn_elem->gvahand
                                  , l_video_frame_nr
                                  , video_frame_chunk_data
                                  , video_frame_chunk_size
                                  , video_frame_chunk_maxsize
                                  );

          if(gap_debug) printf("gap_gve_story_fetch_composite_image_or_chunk:  AFTER CHUNK fetch max:%d chunk_data:%d  chunk_size:%d\n"
                        ,(int)video_frame_chunk_maxsize
                        ,(int)video_frame_chunk_data
                        ,(int)*video_frame_chunk_size
                        );
          if(l_videofile_name)
          {
            g_free(l_videofile_name);
	    l_videofile_name = NULL;
          }

          if(l_fcr == GVA_RET_OK)
          {
	    gint l_frame_type;

	    l_frame_type = GVA_util_check_mpg_frame_type(video_frame_chunk_data
	                                                ,*video_frame_chunk_size
							);
	    GVA_util_fix_mpg_timecode(video_frame_chunk_data
	                             ,*video_frame_chunk_size
				     ,master_framerate
				     ,master_frame_nr
				     );			
            if ((1==0)
	    &&  (master_frame_nr < 10))  /* debug code: dump fist 9 video chunks to file(s) */
            {
               FILE *fp;
               char *fname;

               fname = g_strdup_printf("zz_chunk_data_%06d.dmp", (int)l_video_frame_nr);
               fp = fopen(fname, "wb");
               if(fp)
               {
                  fwrite(video_frame_chunk_data, *video_frame_chunk_size, 1, fp);
                  fclose(fp);
               }

               g_free(fname);
            }

	    if (l_frame_type == GVA_MPGFRAME_I_TYPE)
	    {
	      /* intra frame has no dependencies to other frames
	       * can use that frame type at any place in the stram 
	       */
              last_video_frame_nr = l_video_frame_nr;
              last_intra_frame_fetched = TRUE;
	      if(last_videofile)
	      {
		g_free(last_videofile);
	      }
	      last_videofile = g_strdup(l_videofile);
	      last_fetch_was_compressed_chunk = TRUE;
	      
	      /*if(gap_debug)*/
	      {
	        printf(" Reuse I-FRAME Chunk  at %06d\n", (int)master_frame_nr);
	      }
              return(TRUE);
	    }

	    if (l_frame_type == GVA_MPGFRAME_P_TYPE)
	    {
	      /* predicted frame has dependencies to the previous intra frame
	       * can use that frame if fetch sequence contains previous i frame
	       */
	      if(last_videofile)
	      {
		if((strcmp(l_videofile, last_videofile) == 0)
		&& (l_video_frame_nr = last_video_frame_nr +1))
		{
                  last_video_frame_nr = l_video_frame_nr;
		  last_fetch_was_compressed_chunk = TRUE;
		  /*if(gap_debug)*/
		  {
	            printf(" Reuse P-FRAME Chunk  at %06d\n", (int)master_frame_nr);
		  }
                  return(TRUE);
		}
	      }
	    }

	    if (l_frame_type == GVA_MPGFRAME_B_TYPE)
	    {
	      /* bi-directional predicted frame has dependencies both to 
	       * the previous intra frame or p-frame and to the following i or p-frame.
	       *
	       * can use that frame if fetch sequence contains previous i frame
	       * and fetch will continue until the next i or p frame.
	       *
	       * we do a simplified check if the next few (say 3) frames in storyboard sequence
	       * will fetch the next (3) frames in videofile sequence from the same videofile.
	       * this is just a guess, but maybe sufficient in most cases.
	       */
	      if(last_videofile)
	      {
                gboolean l_bframe_ok;
		
                l_bframe_ok = TRUE;  /* assume that B-frame can be used */
		
		if((strcmp(l_videofile, last_videofile) == 0)
		&& (l_video_frame_nr = last_video_frame_nr +1))
		{
		  if(master_frame_nr + GAP_MPEG_ASSUMED_REFERENCE_DISTANCE > max_master_frame_nr)
		  {
		    /* never deliver B-frame at the last few frames in the output video.
		     * (unresolved references to following p or i frames of the
		     *  input video could be the result)
		     */
		    l_bframe_ok = FALSE;
		  }
		  else
		  {
		    gint ii;
		    gint32 l_next_video_frame_nr;
		    char  *l_next_videofile;

		    /* look ahead if the next few fetches in storyboard sequence
		     * will deliver the next frames from the same inputvideo
		     * in ascending input_video sequence at stepsize 1
		     * (it is assumed that the referenced P or I frame
		     *  will be fetched in later calls then)
		     */
		    for(ii=1; ii <= GAP_MPEG_ASSUMED_REFERENCE_DISTANCE; ii++)
		    {
		      l_next_videofile = p_check_chunk_fetch_possible(vidhand
                		    , (master_frame_nr + ii)
                		    , vid_width
                		    , vid_height
                		    , &l_next_video_frame_nr
		                    , &l_frn_elem_2
                		    );
		      if(l_next_videofile)
		      {
        		if((strcmp(l_next_videofile, l_videofile) != 0)
        		|| (l_next_video_frame_nr != l_video_frame_nr +ii))
        		{
        		  l_bframe_ok = FALSE;
        		}
			g_free(l_next_videofile);
		      }
		      else
		      {
        		l_bframe_ok = FALSE;
 		      }
		      if(!l_bframe_ok)
		      {
		        break;
		      }

		    }  /* end for loop (look ahed next few frames in storyboard sequence) */
		  }
		    
		  /*if(gap_debug)*/
		  {
		    if(l_bframe_ok) printf("Look Ahead B-FRAME OK to copy\n");
		    else            printf("Look Ahead B-FRAME dont USE\n");
		  }
		
		  if(l_bframe_ok)
		  {
                    last_video_frame_nr = l_video_frame_nr;
		    last_fetch_was_compressed_chunk = TRUE;
		    /*if(gap_debug)*/
		    {
	              printf(" Reuse B-FRAME Chunk  at %06d\n", (int)master_frame_nr);
		    }
                    return(TRUE);
		  }
		}
	      }
	    }
    
            if(gap_debug) printf("gap_gve_story_fetch_composite_image_or_chunk:  sorry, cant use the fetched CHUNK\n");
	   
	    last_fetch_was_compressed_chunk = FALSE;
	    last_intra_frame_fetched = FALSE;
            l_videofile = NULL;

          }
	  else
	  {
	    last_fetch_was_compressed_chunk = FALSE;
	    last_intra_frame_fetched = FALSE;
            *video_frame_chunk_size = 0;
            return(FALSE);
	  }
        }
      }
    }

  }   /* end IF dont_recode_flag */

  if(last_fetch_was_compressed_chunk)
  {
    *force_keyframe = TRUE;
  }
  
  if(l_videofile_name)
  {
    g_free(l_videofile_name);
  }

  if(l_videofile == NULL)
  {
    last_video_frame_nr = -1;
    last_intra_frame_fetched = FALSE;
    last_fetch_was_compressed_chunk = FALSE;
    if(last_videofile)
    {
      g_free(last_videofile);
    }
    last_videofile = l_videofile;


    if(gap_debug) printf("gap_gve_story_fetch_composite_image_or_chunk:  CHUNKfetch not possible (doing frame fetch instead)\n");

    *video_frame_chunk_size = 0;
    *image_id = gap_gve_story_fetch_composite_image(vidhand
                    , master_frame_nr  /* starts at 1 */
                    , vid_width       /* desired Video Width in pixels */
                    , vid_height      /* desired Video Height in pixels */
                    , filtermacro_file  /* NULL if no filtermacro is used */
                    , layer_id        /* output: Id of the only layer in the composite image */
                    );
    if(*image_id >= 0)
    {
      return(TRUE);
    }
  }

  return(FALSE);

} /* end gap_gve_story_fetch_composite_image_or_chunk */
