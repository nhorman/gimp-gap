/* gap_gve_story.h
 *
 *  GAP common encoder tool procedures
 *  for storyboard_file handling.
 *
 */

/*
 * 2003.05.29  hof  - gap_gve_story_fetch_composite_image_or_chunk (for dont_recode_flag)
 * 2003.04.16  hof  - Support deinterlace and exact_seek features for VID_PLAY_MOVIE
 * 2003.03.08  hof  - buffered audio processing
 * 2003.02.04  hof  - added Videofile (MOVIE) support
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


#ifndef GAP_GVE_STORY_H
#define GAP_GVE_STORY_H

#include "libgimp/gimp.h"
#include "gap_vid_api.h"

/* G_DIR_SEPARATOR (is defined in glib.h if you have glib-1.2.0 or later) */
#ifdef G_OS_WIN32

/* Filenames in WIN/DOS Style */
#ifndef G_DIR_SEPARATOR
#define G_DIR_SEPARATOR '\\'
#endif
#define DIR_ROOT ':'

#else  /* !G_OS_WIN32 */

/* Filenames in UNIX Style */
#ifndef G_DIR_SEPARATOR
#define G_DIR_SEPARATOR '/'
#endif
#define DIR_ROOT '/'

#endif /* !G_OS_WIN32 */


#define STORYBOARD_MAX_VID_TRACKS 20

/* record keynames in Storyboard files */
#define STORYBOARD_HEADER                "STORYBOARDFILE"
#define CLIPLISTFILE_HEADER              "CLIPLISTFILE"
#define STORYBOARD_VID_MASTER_SIZE       "VID_MASTER_SIZE"
#define STORYBOARD_VID_MASTER_FRAMERATE  "VID_MASTER_FRAMERATE"
#define STORYBOARD_VID_PREFERRED_DECODER "VID_PREFERRED_DECODER"

#define STORYBOARD_VID_PLAY_MOVIE        "VID_PLAY_MOVIE"
#define STORYBOARD_VID_PLAY_FRAMES       "VID_PLAY_FRAMES"
#define STORYBOARD_VID_PLAY_ANIMIMAGE    "VID_PLAY_ANIMIMAGE"
#define STORYBOARD_VID_PLAY_IMAGE        "VID_PLAY_IMAGE"
#define STORYBOARD_VID_PLAY_COLOR        "VID_PLAY_COLOR"
#define STORYBOARD_VID_SILENCE           "VID_SILENCE"
#define STORYBOARD_VID_OPACITY           "VID_OPACITY"
#define STORYBOARD_VID_ZOOM_X            "VID_ZOOM_X"
#define STORYBOARD_VID_ZOOM_Y            "VID_ZOOM_Y"
#define STORYBOARD_VID_MOVE_X            "VID_MOVE_X"
#define STORYBOARD_VID_MOVE_Y            "VID_MOVE_Y"
#define STORYBOARD_VID_FIT_SIZE          "VID_FIT_SIZE"

#define STORYBOARD_AUD_MASTER_VOLUME     "AUD_MASTER_VOLUME"
#define STORYBOARD_AUD_MASTER_SAMPLERATE "AUD_MASTER_SAMPLERATE"
#define STORYBOARD_AUD_PLAY_SOUND        "AUD_PLAY_SOUND"
#define STORYBOARD_AUD_PLAY_MOVIE        "AUD_PLAY_MOVIE"
#define STORYBOARD_AUD_SILENCE           "AUD_SILENCE"


#define GAP_VID_ENC_SAVE_MULTILAYER   "GAP_VID_ENC_SAVE_MULTILAYER"
#define GAP_VID_ENC_SAVE_FLAT         "GAP_VID_ENC_SAVE_FLAT"
#define GAP_VID_ENC_MONITOR           "GAP_VID_ENC_MONITOR"



typedef enum
{
   GAP_FRN_SILENCE
  ,GAP_FRN_COLOR
  ,GAP_FRN_IMAGE
  ,GAP_FRN_ANIMIMAGE
  ,GAP_FRN_FRAMES
  ,GAP_FRN_MOVIE
} GapGveStoryFrameType;

typedef enum
{
   GAP_AUT_SILENCE
  ,GAP_AUT_AUDIOFILE
  ,GAP_AUT_MOVIE
} GapGveStoryAudioType;


typedef struct GapGveStoryImageCacheElem
{
   gint32  image_id;
   char   *filename;
   void *next;
} GapGveStoryImageCacheElem;

typedef struct GapGveStoryImageCache
{
  GapGveStoryImageCacheElem *ic_list;
  gint32            max_img_cache;  /* number of images to hold in the cache */
} GapGveStoryImageCache;



typedef struct GapGveStoryAudioCacheElem
{
   gint32  audio_id;
   char   *filename;
   guchar *aud_data;   /* full audiodata (including header) loaded in memory */
   gint32 aud_bytelength;

   gint32 segment_startoffset;
   gint32 segment_bytelength;

   void *next;
} GapGveStoryAudioCacheElem;

typedef struct GapGveStoryAudioCache
{
  GapGveStoryAudioCacheElem *ac_list;
  gint32 nextval_audio_id;
  gint32            max_aud_cache;  /* number of images to hold in the cache */
} GapGveStoryAudioCache;



typedef struct GapGveStoryFrameRangeElem
{
   GapGveStoryFrameType  frn_type;
   gint32  track;
   gint32  last_master_frame_access;
   char   *basename;
   char   *ext;
   t_GVA_Handle *gvahand;     /* API handle for videofile (for GAP_FRN_MOVIE) */
   gint32        seltrack;    /* selected videotrack in a videofile (for GAP_FRN_MOVIE) */
   gint32        exact_seek;  /* 0 fast seek, 1 exact seek (for GAP_FRN_MOVIE) */
   gdouble       delace;      /* 0.0 no deinterlace, 1.0-1.99 odd 2.0-2.99 even rows  (for GAP_FRN_MOVIE) */
   char   *filtermacro_file;
   gint32  frame_from;
   gint32  frame_to;
   gint32  frame_first;
   gint32  frame_last;
   gint32  frames_to_handle;
   gint32  frame_cnt;
   gint32  delta;               /* +1 or -1 */
   guchar red;
   guchar green;
   guchar blue;
   guchar alpha;

   gdouble  alpha_proportional; /* 0.0 upto 1.0 */
   gboolean keep_proportions;
   gboolean fit_width;
   gboolean fit_height;

   gdouble  wait_untiltime_sec;
   gint32   wait_untilframes;

   gdouble opacity_from;       /* 0.0 upto 1.0 */
   gdouble opacity_to;         /* 0.0 upto 1.0 */
   gint32  opacity_dur;        /* number of frames to change from -> to value */

   gdouble scale_x_from;       /* 0.0 upto 10.0  where 1.0 is fit video size */
   gdouble scale_x_to;         /* 0.0 upto 10.0  where 1.0 is fit video size */
   gint32  scale_x_dur;        /* number of frames to change from -> to value */

   gdouble scale_y_from;       /* 0.0 upto 10.0  where 1.0 is fit video size */
   gdouble scale_y_to;         /* 0.0 upto 10.0  where 1.0 is fit video size */
   gint32  scale_y_dur;        /* number of frames to change from -> to value */

   gdouble move_x_from;        /* -1.0 upto 1.0 where 0 is center and -1.0 left outside */
   gdouble move_x_to;          /* -1.0 upto 1.0 where 0 is center and -1.0 left outside */
   gint32  move_x_dur;         /* number of frames to change from -> to value */

   gdouble move_y_from;        /* -1.0 upto 1.0 where 0 is center and -1.0 up outside */
   gdouble move_y_to;          /* -1.0 upto 1.0 where 0 is center and -1.0 up outside */
   gint32  move_y_dur;         /* number of frames to change from -> to value */

   void   *next;
} GapGveStoryFrameRangeElem;  /* used for storyboard processing */


typedef struct GapGveStoryAudioRangeElem
{
   GapGveStoryAudioType  aud_type;
   gint32    track;
   char     *audiofile;
   char     *tmp_audiofile;
   t_GVA_Handle *gvahand;           /* API handle for videofile (for GAP_AUT_MOVIE) */
   gint32        seltrack;          /* selected audiotrack in a videofile (for GAP_AUT_MOVIE) */
   gint32   samplerate;             /* samples per sec */
   gint32   channels;               /* 1 mono, 2 stereo */
   gint32   bytes_per_sample;
   gint32   samples;
   gdouble  max_playtime_sec;

   gdouble  wait_untiltime_sec;
   gint32   wait_until_samples;

   gdouble  range_playtime_sec;
   gdouble  play_from_sec;
   gdouble  play_to_sec;
   gdouble  volume_start;
   gdouble  volume;
   gdouble  volume_end;
   gdouble  fade_in_sec;
   gdouble  fade_out_sec;

   gint32 audio_id;               /* audio cache id */
   GapGveStoryAudioCacheElem *ac_elem;
   guchar *aud_data;
   gint32 aud_bytelength;
   gint32 range_samples;          /* number of samples in the selected range (sample has upto 4 bytes) */
   gint32 fade_in_samples;
   gint32 fade_out_samples;
   gint32 byteoffset_rangestart;
   gint32 byteoffset_data;

   void   *next;
} GapGveStoryAudioRangeElem;  /* used for storyboard processing */


typedef struct GapGveStoryVTrackAttrElem
{
   gint32  frame_count;

   gdouble  alpha_proportional; /* 0.0 upto 1.0 */
   gboolean keep_proportions;
   gboolean fit_width;
   gboolean fit_height;

   gdouble opacity_from;       /* 0.0 upto 1.0 */
   gdouble opacity_to;         /* 0.0 upto 1.0 */
   gint32  opacity_dur;        /* number of frames to change from -> to value */

   gdouble scale_x_from;       /* 0.0 upto 10.0  where 1.0 is fit video size */
   gdouble scale_x_to;         /* 0.0 upto 10.0  where 1.0 is fit video size */
   gint32  scale_x_dur;        /* number of frames to change from -> to value */

   gdouble scale_y_from;       /* 0.0 upto 10.0  where 1.0 is fit video size */
   gdouble scale_y_to;         /* 0.0 upto 10.0  where 1.0 is fit video size */
   gint32  scale_y_dur;        /* number of frames to change from -> to value */

   gdouble move_x_from;        /* -1.0 upto 1.0 where 0 is center and -1.0 left outside */
   gdouble move_x_to;          /* -1.0 upto 1.0 where 0 is center and -1.0 left outside */
   gint32  move_x_dur;         /* number of frames to change from -> to value */

   gdouble move_y_from;        /* -1.0 upto 1.0 where 0 is center and -1.0 up outside */
   gdouble move_y_to;          /* -1.0 upto 1.0 where 0 is center and -1.0 up outside */
   gint32  move_y_dur;         /* number of frames to change from -> to value */

} GapGveStoryVTrackAttrElem;  /* Video track attributes used for storyboard processing */

typedef struct GapGveStoryVTrackArray
{
  GapGveStoryVTrackAttrElem attr[STORYBOARD_MAX_VID_TRACKS];
  gint32 max_tracknum;
} GapGveStoryVTrackArray;  /* used for storyboard processing */


typedef struct GapGveStoryErrors
{
  char   *errtext;       /* NULL==no error */
  char   *errline;       /* NULL, or copy of the line that has the 1. error */
  gint32  errline_nr;    /* line number where 1.st error occured */
  char   *warntext;      /* NULL==no error */
  char   *warnline;      /* NULL, or copy of the line that has the 1. error */
  gint32  warnline_nr;   /* line number where 1.st error occured */
  gint32  curr_nr;       /* current line nr while parsing */
  char   *currline;      /* pointer to currently parsed line (do not free this) */
} GapGveStoryErrors;  /* used for storyboard processing */

typedef struct GapGveStoryVidHandle
{
  GapGveStoryFrameRangeElem    *frn_list;
  GapGveStoryAudioRangeElem    *aud_list;
  GapGveStoryErrors            *sterr;
  char                         *preferred_decoder;

  /* master video settings found in the storyboard file */
  gdouble  master_framerate;
  gint32   master_width;
  gint32   master_height;
  gint32   master_samplerate;
  gdouble  master_volume;

  char    *util_sox;
  char    *util_sox_options;

  gboolean ignore_audio;
  gboolean ignore_video;

  gboolean   cancel_operation;   /* not supported yet */
  gdouble    *progress;
  gboolean   do_gimp_progress;   /* pass this to GVA gvahand->do_gimp_progress (to show video seek progress) */
  gchar      *status_msg;
  gint32     status_msg_len;

} GapGveStoryVidHandle;  /* used for storyboard processing */


/* --------------------------*/
/* PROCEDURE DECLARATIONS    */
/* --------------------------*/

void     gap_gve_story_debug_print_framerange_list(GapGveStoryFrameRangeElem *frn_list
                             , gint32 track                    /* -1 show all tracks */
                             );
void     gap_gve_story_debug_print_audiorange_list(GapGveStoryAudioRangeElem *aud_list
                             , gint32 track                    /* -1 show all tracks */
                             );


void     gap_gve_story_drop_image_cache(void);

void     gap_gve_story_remove_tmp_audiofiles(GapGveStoryVidHandle *vidhand);
void     gap_gve_story_drop_audio_cache(void);



gint32   gap_gve_story_fetch_composite_image(GapGveStoryVidHandle *vidhand
                    , gint32 master_frame_nr  /* starts at 1 */
                    , gint32  vid_width       /* desired Video Width in pixels */
                    , gint32  vid_height      /* desired Video Height in pixels */
                    , char *filtermacro_file  /* NULL if no filtermacro is used */
                    , gint32 *layer_id        /* output: Id of the only layer in the composite image */
                 );

gboolean gap_gve_story_fetch_composite_image_or_chunk(GapGveStoryVidHandle *vidhand
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
                    , gint32 *video_frame_chunk_size           /* OUT: */
                    , gint32 video_frame_chunk_maxsize         /* IN: */
                 );

GapGveStoryVidHandle *  gap_gve_story_open_vid_handle(
                           char *storyboard_file
                          ,char *basename
                          ,char *ext
                          ,gint32  frame_from
                          ,gint32  frame_to
                          ,gint32 *frame_count   /* output total frame_count , or 0 on failure */
                          );

GapGveStoryVidHandle *  gap_gve_story_open_extended_video_handle(
                           gboolean ignore_audio
                          ,gboolean igore_video
                          ,gdouble  *progress_ptr
                          ,char *status_msg
                          ,gint32 status_msg_len
                          ,char *storyboard_file
                          ,char *basename
                          ,char *ext
                          ,gint32  frame_from
                          ,gint32  frame_to
                          ,gint32 *frame_count   /* output total frame_count , or 0 on failure */
                          );

void    gap_gve_story_close_vid_handle(GapGveStoryVidHandle *vidhand);
void    gap_gve_story_set_audio_resampling_program(
                           GapGveStoryVidHandle *vidhand
                           , char *util_sox
                           , char *util_sox_options
                           );
void      gap_gve_story_calc_audio_playtime(GapGveStoryVidHandle *vidhand, gdouble *aud_total_sec);
gboolean  gap_gve_story_create_composite_audiofile(GapGveStoryVidHandle *vidhand
                            , char *comp_audiofile
                            );


#endif        /* GAP_GVE_STORY_H */
