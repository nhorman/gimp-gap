/* gap_decode_xanim.c
 * 1999.11.22 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * This Module contains:
 *
 *        GIMP/GAP-frontend interface for XANIM exporting edition from loki entertainmaint
 *         Call xanim exporting edition (the loki version)
 *         To split any xanim supported video into
 *         video frames (single images on disk)
 *         Audio can also be extracted.
 *
 *        xanim  exporting edition is available at:
 *            Web:        http://heroine.linuxbox.com/toys.html
 *                        http://www.lokigames.com/development/smjpeg.php3
 *            download:   http://heroine.linuxbox.com/xanim_exporting_edition.tar.gz
 *                        http://www.lokigames.com/development/download/smjpeg/xanim2801-loki090899.tar.gz
 *            Send comments or questions to smjpeg@lokigames.com.
 *
 * Warning: This Module needs UNIX environment to run.
 *   It uses programs and commands that are NOT available
 *   on other Operating Systems (Win95, NT, XP ...)
 *
 *     - xanim              2.80 exporting edition with extensions from loki entertainment.
 *                          set environment GAP_XANIM_PROG to configure where to find xanim
 *                          (default: search xanim in your PATH)
 *     - cd                 (UNIX command)
 *     - grep               (UNIX command)
 *     - rm                 (UNIX command is used to delete by wildcard (expanded by /bin/sh)
 *                                and to delete a directory with all files
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

/* revision history
 * gimp    2.1.0;   2004/12/06  hof: use g_file_test(dir, G_FILE_TEST_IS_DIR) to check for directory
 * gimp    1.3.26c; 2004/03/09  hof: bugfix (gimp_destroy_params)
 * gimp    1.3.20a; 2003/09/15  hof: should fix compile problems on WIN32 (#122220)
 *                                   (but will not run at WIN OS)
 * gimp    1.3.17b; 2003/07/31  hof: message text fixes for translators (# 118392)
 * gimp    1.3.16a; 2003/06/25  hof: no textsplitting across multiple lables (for translation)
 * gimp    1.3.15a; 2003/06/21  hof: checked textspacing
 * gimp    1.3.12a; 2003/05/02  hof: merge into CVS-gimp-gap project, 6digit framenumbers
 * 1.3.4b    2002/03723  hof: merged in bugfix from stable branch 1.2.2c; 2002/03/19 xanim call fails if . not in PATH or no write permission for current dir
 *                           (reported by Guido Socher)
 * 1.1.29b;  2000/11/30  hof: used g_snprintf
 * 1.1.17b;  2000/02/26  hof: bugfixes
 * 1.1.14a;  1999/11/22  hof: fixed gcc warning (too many arguments for format)
 * 1.1.13a;  1999/11/22  hof: first release
 */
 
/* SYTEM (UNIX) includes */ 
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>

#include <glib/gstdio.h>

/* GIMP includes */
#include "gtk/gtk.h"
#include "config.h"
#include "gap-intl.h"
#include "libgimp/gimp.h"

#ifdef G_OS_WIN32
#include <io.h>
#  ifndef S_ISDIR
#    define S_ISDIR(m) ((m) & _S_IFDIR)
#  endif
#  ifndef S_ISREG
#    define S_ISREG(m) ((m) & _S_IFREG)
#  endif
#endif


#define GAP_XANIM_HELP_ID  "plug-in-gap-xanim-decode"

/* GAP includes */
#include "gap_libgapbase.h"
#include "gap_lib.h"
#include "gap_arr_dialog.h"
#include "gap_decode_xanim.h"

extern      int gap_debug; /* ==0  ... dont print debug infos */

static char *global_xanim_input_dir = NULL;
static char *global_xanim_working_dir = NULL;

static gchar global_xanim_prog[500];
static gchar *global_errlist = NULL;

gint32  global_delete_number;
/* fileformats supported by gap_decode_xanim */

typedef enum
{ 
   XAENC_XCF     /* no direct support by xanim, have to convert */
 , XAENC_PPMRAW
 , XAENC_JPEG
} GapXAnimFormats;


/* ============================================================================
 * p_xanim_info
 * ============================================================================
 */
static int
p_xanim_info(char *errlist)
{
  GapArrArg  argv[22];
  GapArrButtonArg  b_argv[2];

  int        l_idx;
  int        l_rc;
  

  l_idx = 0;
  gap_arr_arg_init(&argv[l_idx], GAP_ARR_WGT_LABEL_LEFT);
  argv[l_idx].label_txt = _("Requirements to run the xanim based video split");

  l_idx++;
  gap_arr_arg_init(&argv[l_idx], GAP_ARR_WGT_LABEL_LEFT);
  argv[l_idx].label_txt = "";

  l_idx++;
  gap_arr_arg_init(&argv[l_idx], GAP_ARR_WGT_LABEL_LEFT);
  argv[l_idx].label_txt = "1.)";

  l_idx++;
  gap_arr_arg_init(&argv[l_idx], GAP_ARR_WGT_LABEL_LEFT);
  argv[l_idx].label_txt = _("xanim 2.80.0 exporting edition (the loki version)\n"
                            "must be installed somewhere in your PATH\n"
                            "you can get xanim exporting edition at:\n"
                           );

  l_idx++;
  gap_arr_arg_init(&argv[l_idx], GAP_ARR_WGT_LABEL_LEFT);
  argv[l_idx].label_txt = "http://heroine.linuxbox.com/toys.html";

  l_idx++;
  gap_arr_arg_init(&argv[l_idx], GAP_ARR_WGT_LABEL_LEFT);
  argv[l_idx].label_txt = "http://www.lokigames.com/development/download/smjpeg/xanim2801-loki090899.tar.gz";

  l_idx++;
  gap_arr_arg_init(&argv[l_idx], GAP_ARR_WGT_LABEL_LEFT);
  argv[l_idx].label_txt = "";

  l_idx++;
  gap_arr_arg_init(&argv[l_idx], GAP_ARR_WGT_LABEL_LEFT);
  argv[l_idx].label_txt = "2.)";

  l_idx++;
  gap_arr_arg_init(&argv[l_idx], GAP_ARR_WGT_LABEL_LEFT);
  argv[l_idx].label_txt = _("if your xanim exporting edition is not in your PATH or is not named xanim\n"
                            "you have to set environment variable GAP_XANIM_PROG\n"
                            "to your xanim exporting program and restart gimp"
                           );

  l_idx++;
  gap_arr_arg_init(&argv[l_idx], GAP_ARR_WGT_LABEL_LEFT);
  argv[l_idx].label_txt = "";

  l_idx++;
  gap_arr_arg_init(&argv[l_idx], GAP_ARR_WGT_LABEL_LEFT);
  argv[l_idx].label_txt = _("An error occurred while trying to call xanim:");  

  l_idx++;
  gap_arr_arg_init(&argv[l_idx], GAP_ARR_WGT_LABEL_LEFT);
  argv[l_idx].label_txt = "--------------------------------------------";  


  l_idx++;
  gap_arr_arg_init(&argv[l_idx], GAP_ARR_WGT_LABEL_LEFT);
  argv[l_idx].label_txt = errlist;


  l_idx++;

  /* the  Action Button */
    b_argv[0].but_txt  = GTK_STOCK_CANCEL;
    b_argv[0].but_val  = -1;
    b_argv[1].but_txt  = GTK_STOCK_OK;
    b_argv[1].but_val  = 0;
  
  l_rc = gap_arr_std_dialog(_("XANIM Information"),
                             "",
                              l_idx,   argv,      /* widget array */
                              1,       b_argv,    /* button array */
                              -1);

  return (l_rc); 
}       /* end p_xanim_info */


/* ============================================================================
 * p_xanim_dialog
 * ============================================================================
 */
static int
p_xanim_dialog   (gint32 *first_frame,
                  gint32 *last_frame,
                  char   *filename,
                  gint32 len_filename,
                  char   *basename,
                  gint32 len_basename,
                  GapXAnimFormats *Format,
                  gint32  *extract_video,
                  gint32  *extract_audio,
                  gint32  *jpeg_quality,
                  gint32  *autoload,
                  gint32  *run_xanim_asynchron)
{
#define XADIALOG_NUM_ARGS 13
  static GapArrArg  argv[XADIALOG_NUM_ARGS];
  static char *radio_args[3]  = { "XCF", "PPM", "JPEG" };

  gap_arr_arg_init(&argv[0], GAP_ARR_WGT_FILESEL);
  argv[0].label_txt = _("Video:");
  argv[0].help_txt  = _("Name of a videofile to read by xanim. "
                        "Frames are extracted from the videofile "
                        "and written to separate diskfiles. "
                        "xanim exporting edition is required.");
  argv[0].text_buf_len = len_filename;
  argv[0].text_buf_ret = filename;
  argv[0].entry_width = 250;

  gap_arr_arg_init(&argv[1], GAP_ARR_WGT_INT_PAIR);
  argv[1].label_txt = _("From Frame:");
  argv[1].help_txt  = _("Framenumber of 1st frame to extract");
  argv[1].constraint = FALSE;
  argv[1].int_min    = 0;
  argv[1].int_max    = 9999;
  argv[1].int_ret    = 0;
  argv[1].umin       = 0;
  argv[1].entry_width = 80;
  
  gap_arr_arg_init(&argv[2], GAP_ARR_WGT_INT_PAIR);
  argv[2].label_txt = _("To Frame:");
  argv[2].help_txt  = _("Framenumber of last frame to extract");
  argv[2].constraint = FALSE;
  argv[2].int_min    = 0;
  argv[2].int_max    = 9999;
  argv[2].int_ret    = 9999;
  argv[2].umin       = 0;
  argv[2].entry_width = 80;
  
  gap_arr_arg_init(&argv[3], GAP_ARR_WGT_FILESEL);
  argv[3].label_txt = _("Framenames:");
  argv[3].help_txt  = _("Basename for the video frames to write on disk. "
                        "Framenumber and extension is added.");
  argv[3].text_buf_len = len_basename;
  argv[3].text_buf_ret = basename;
  argv[3].entry_width = 250;
  
  gap_arr_arg_init(&argv[4], GAP_ARR_WGT_OPTIONMENU);
  argv[4].label_txt = _("Format");
  argv[4].help_txt  = _("Fileformat for the extracted video frames. "
                       "(xcf is extracted as ppm and converted to xcf)");
  argv[4].radio_argc  = 3;
  argv[4].radio_argv = radio_args;
  argv[4].radio_ret  = 0;

  gap_arr_arg_init(&argv[5], GAP_ARR_WGT_TOGGLE);
  argv[5].label_txt = _("Extract Frames");
  argv[5].help_txt  = _("Enable extraction of frames");
  argv[5].int_ret   = 1;

  gap_arr_arg_init(&argv[6], GAP_ARR_WGT_TOGGLE);
  argv[6].label_txt = _("Extract Audio");
  argv[6].help_txt  = _("Enable extraction of audio to raw audiofile. "
                        "(frame range limits are ignored for audio)");
  argv[6].int_ret   = 0;
  
  gap_arr_arg_init(&argv[7], GAP_ARR_WGT_INT_PAIR);
  argv[7].label_txt = _("Jpeg Quality:");
  argv[7].help_txt  = _("Quality for resulting jpeg frames. "
                        "(is ignored when other formats are used)");
  argv[7].constraint = TRUE;
  argv[7].int_min    = 0;
  argv[7].int_max    = 100;
  argv[7].int_ret    = 90;

  gap_arr_arg_init(&argv[8], GAP_ARR_WGT_LABEL);
  argv[8].label_txt = "";

  gap_arr_arg_init(&argv[9], GAP_ARR_WGT_TOGGLE);
  argv[9].label_txt = _("Open");
  argv[9].help_txt  = _("Open the 1st one of the extracted frames");
  argv[9].int_ret   = 1;

  gap_arr_arg_init(&argv[10], GAP_ARR_WGT_TOGGLE);
  argv[10].label_txt = _("Run asynchronously");
  argv[10].help_txt  = _("Run xanim asynchronously and delete unwanted frames "
                        "(out of the specified range) while xanim is still running");
  argv[10].int_ret   = 1;

  gap_arr_arg_init(&argv[11], GAP_ARR_WGT_LABEL_LEFT);
  argv[11].label_txt = _("\nWarning: xanim 2.80 is old unmaintained software\n"
                         "and has only limited MPEG support.\n"
                         "Most of the frames (type P and B) will be skipped.");

  gap_arr_arg_init(&argv[12], GAP_ARR_WGT_HELP_BUTTON);
  argv[12].help_id = GAP_XANIM_HELP_ID;

   
  if(TRUE == gap_arr_ok_cancel_dialog(_("Xanim based extraction (DEPRECATED)"), 
                            _("Select Frame Range"), XADIALOG_NUM_ARGS, argv))
  {
     if(argv[1].int_ret < argv[2].int_ret )
     {
       *first_frame = (long)(argv[1].int_ret);
       *last_frame  = (long)(argv[2].int_ret);
     }
     else
     {
       *first_frame = (long)(argv[2].int_ret);
       *last_frame  = (long)(argv[1].int_ret);
     }
     *Format = (GapXAnimFormats)(argv[4].int_ret);
     *extract_video  = (long)(argv[5].int_ret);
     *extract_audio  = (long)(argv[6].int_ret);
     *jpeg_quality   = (long)(argv[7].int_ret);
     *autoload       = (long)(argv[9].int_ret);
     *run_xanim_asynchron   = (long)(argv[10].int_ret);
     return (0);    /* OK */
  }
  else
  {
     return -1;     /* Cancel */
  }
   

}       /* end p_xanim_dialog */


static gint
p_overwrite_dialog(char *filename, gint overwrite_mode)
{
  static  GapArrButtonArg  l_argv[3];
  static  GapArrArg  argv[1];

  if(gap_lib_file_exists(filename))
  {
    if (overwrite_mode < 1)
    {
       l_argv[0].but_txt  = _("Overwrite Frame");
       l_argv[0].but_val  = 0;
       l_argv[1].but_txt  = _("Overwrite All");
       l_argv[1].but_val  = 1;
       l_argv[2].but_txt  = GTK_STOCK_CANCEL;
       l_argv[2].but_val  = -1;

       gap_arr_arg_init(&argv[0], GAP_ARR_WGT_LABEL);
       argv[0].label_txt = filename;
    
       return(gap_arr_std_dialog ( _("GAP Question"),
                                   _("File already exists"),
                                   1, argv,
                                   3, l_argv, -1));
    }
  }
  return (overwrite_mode);
}




static void
p_build_xanim_framename(char *framename, gint32 sizeof_framename, gint32 frame_nr, char *ext)
{
   g_snprintf(framename, sizeof_framename, "%s%sframe%d.%s",
                global_xanim_input_dir,
                G_DIR_SEPARATOR_S,
                (int)frame_nr,
                ext);
}

static void
p_build_gap_framename(char *framename, gint32 sizeof_framename, gint32 frame_nr, char *basename, char *ext)
{
   g_snprintf(framename, sizeof_framename, "%s%06d.%s", basename, (int)frame_nr, ext);
}


static void
p_dirname(char *fname)
{
  int l_idx;
  
  l_idx = strlen(fname) -1;
  while(l_idx > 0)
  {
     if(fname[l_idx] == G_DIR_SEPARATOR)
     {
        fname[l_idx] = '\0';
        return;
     }
     l_idx--;
  }
  *fname = '\0';
}

static void
p_init_xanim_global_name()
{
  const gchar *l_env;
  gchar       *l_ptr;
  
  l_env = g_getenv("GAP_XANIM_PROG");

  /* use gimp_temp_name to find a writeable tempdir (used as xanim workingdir) */
  if(global_xanim_working_dir)
    g_free(global_xanim_working_dir);
    
  /* cut off the filenamepart */
  global_xanim_working_dir = gimp_temp_name(".txt");
  for(l_ptr = global_xanim_working_dir + strlen(global_xanim_working_dir);
      l_ptr >= global_xanim_working_dir;
      l_ptr--)
  {
    if(*l_ptr == G_DIR_SEPARATOR) 
    { 
      *l_ptr = '\0';
      break;
    }
  }

  if(global_xanim_input_dir)
    g_free(global_xanim_input_dir);
  
  global_xanim_input_dir = g_strdup_printf("%s%sinput", global_xanim_working_dir, G_DIR_SEPARATOR_S);
  
  if(l_env != NULL)
  {
     strcpy(global_xanim_prog, l_env);     
     return;
  }
  strcpy(global_xanim_prog, "xanim");  /* default name */
}

static int
p_convert_frames(gint32 frame_from, gint32 frame_to, char *basename, char *ext, char *ext2)
{
   GimpParam          *return_vals;
   int              nreturn_vals;
   gint32           l_tmp_image_id;
   char             l_first_xa_frame[200];
   int              l_rc;

   l_rc = -1;

  /* load 1st one of those frames generated by xanim  */
   p_build_xanim_framename(l_first_xa_frame, sizeof(l_first_xa_frame), frame_from, ext);
   l_tmp_image_id = gap_lib_load_image(l_first_xa_frame);

   /* convert the xanim frames (from ppm) to xcf fileformat
    * (the gap module for range convert is not linked to the frontends
    *  main program, therefore i call the convert procedure by PDB-interface)
    */   
   return_vals = gimp_run_procedure ("plug_in_gap_range_convert2",
                                    &nreturn_vals,
                                    GIMP_PDB_INT32, GIMP_RUN_NONINTERACTIVE,     /* runmode  */
                                    GIMP_PDB_IMAGE, l_tmp_image_id,
                                    GIMP_PDB_DRAWABLE, 0,         /* (unused)  */
                                    GIMP_PDB_INT32, frame_from,
                                    GIMP_PDB_INT32, frame_to,
                                    GIMP_PDB_INT32, 0,            /* dont flatten */
                                    GIMP_PDB_INT32, 4444,         /* dest type (keep type) */
                                    GIMP_PDB_INT32, 256,          /* colors (unused)  */
                                    GIMP_PDB_INT32, 0,            /* no dither (unused)  */
                                    GIMP_PDB_STRING, ext2,        /* extension for dest. filetype  */
                                    GIMP_PDB_STRING, basename,    /* basename for dest. filetype  */
                                    GIMP_PDB_INT32, 0,            /* (unused)  */
                                    GIMP_PDB_INT32, 0,            /* (unused)  */
                                    GIMP_PDB_INT32, 0,            /* (unused)  */
                                    GIMP_PDB_STRING, "none",      /* (unused) palettename */
                                    GIMP_PDB_END);

   /* destroy the tmp image */
   gimp_image_delete(l_tmp_image_id);

   if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
   {
      l_rc = 0;   /* OK */
   }
   gimp_destroy_params (return_vals, nreturn_vals);

   return(l_rc);
}


static gint32
p_find_max_xanim_frame(gint32 from_nr, char *ext)
{
  gint32 l_high;
  gint32 l_max_found;
  gint32 l_nr;
  gint32 l_delta;
  char   l_frame[500];
  
  l_nr = from_nr;
  l_max_found = 0;
  l_high = 100000;
  
  while(1 == 1)
  {
     p_build_xanim_framename(l_frame, sizeof(l_frame), l_nr, ext);

     if(gap_debug) printf("DEBUG find_MAX :%s\n", l_frame);
     
     if(gap_lib_file_exists(l_frame))
     {
        l_max_found = l_nr;
        l_delta = (l_high - l_nr) / 2;
        if(l_delta == 0)
        { 
           l_delta = 1;
        }
        l_nr = l_max_found + l_delta;
     }
     else
     {  
        if(l_nr == from_nr) { return (-1); }  /* no frames found */

        if(l_nr < l_high)
        {
           l_high = l_nr;
           l_nr = l_max_found + 1;
        }
        else
        {
           return(l_max_found);
        }
     }
  } 
}       /* end p_find_max_xanim_frame */

static int
gap_lib_rename_frames(gint32 frame_from, gint32 frame_to, char *basename, char *ext)
{
  gint32 l_use_mv;
  gint32 l_frame_nr;
  gint32 l_max_found;
  char   l_src_frame[500];
  char   l_dst_frame[500];
  gint    l_overwrite_mode;


  if(gap_debug) printf("gap_lib_rename_frames:\n");
  l_use_mv = TRUE;
  l_overwrite_mode = 0;

  
  l_max_found = p_find_max_xanim_frame (frame_from, ext);
  if(l_max_found < 0)
  {
       global_errlist = g_strdup_printf(
               _("can't find any extracted frames,\n%s\nmaybe xanim has failed or was cancelled"),
               l_src_frame);
       return(-1);
  }
 
  
  l_frame_nr = frame_from;
   
  while (l_frame_nr <= frame_to)
  {
     p_build_xanim_framename(l_src_frame, sizeof(l_src_frame), l_frame_nr, ext);
     p_build_gap_framename(l_dst_frame, sizeof(l_dst_frame), l_frame_nr, basename, ext);
     
     if(!gap_lib_file_exists(l_src_frame))
     {
        break;  /* srcfile not found, stop */
     }
     
     if (strcmp(l_src_frame, l_dst_frame) != 0)
     {
        /* check overwrite if Destination frame already exsts */
        l_overwrite_mode = p_overwrite_dialog(l_dst_frame, l_overwrite_mode);
        if (l_overwrite_mode < 0)
        {
             global_errlist = g_strdup_printf(
                     _("frames are not extracted, because overwrite of %s was cancelled"),
                     l_dst_frame);
             return(-1);
        }
        else
        {
           g_remove(l_dst_frame);
           if (gap_lib_file_exists(l_dst_frame))
           {
             global_errlist = g_strdup_printf(
                     _("failed to overwrite %s (check permissions ?)"),
                     l_dst_frame);
             return(-1);
           }
        }

        if (l_use_mv)
        {       
           g_rename(l_src_frame, l_dst_frame);
        }
        
        if (!gap_lib_file_exists(l_dst_frame)) 
        {
           gap_lib_file_copy(l_src_frame, l_dst_frame);
           if (gap_lib_file_exists(l_dst_frame))
           {
              l_use_mv = FALSE; /* if destination is on another device use copy-remove strategy */
              g_remove(l_src_frame);
           }
           else
           {
              global_errlist = g_strdup_printf(
                     _("failed to write %s (check permissions ?)"),
                     l_dst_frame);
              return(-1);
           }
        }
     }
     l_frame_nr++;
     if(l_max_found > 0) gimp_progress_update ((gdouble)l_frame_nr / (gdouble)l_max_found);
  }
  return(0);
}       /* end gap_lib_rename_frames */

static void
gap_lib_delete_frames(gint32 max_tries, gint32 frame_from, gint32 frame_to, char *ext)
{
  /* this procedure is performed repeatedly while polling the xanim process
   * and after xanim process has (or was) terminated to clean up unwanted frames.
   */
  gint32 l_tries;
  gint32 l_next_number;
  char   l_framename[500];
  
  if(gap_debug) printf("gap_lib_delete_frames: cleaning up unwanted frames (max=%d)\n", (int)max_tries);
  
  l_tries = 0;

  while ((global_delete_number < frame_from) && (l_tries < max_tries))
  {
     l_next_number = global_delete_number + 1;
     p_build_xanim_framename(l_framename, sizeof(l_framename), l_next_number, ext);
     
     if (gap_lib_file_exists(l_framename))
     {
        /* if xanim has already written the next frame
         * we can delete the previous (unwanted) frame now
         */
        p_build_xanim_framename(l_framename, sizeof(l_framename), global_delete_number, ext);
        if(gap_debug) printf("delete frame: %s\n", l_framename);
        g_remove(l_framename);

        global_delete_number = l_next_number;
     }
     l_tries++;
  }
}       /* end gap_lib_delete_frames */


static void
p_poll(pid_t xanim_pid, char *one_past_last_frame, gint32 frame_from, gint32 frame_to, char *ext)
{
  /* loop as long as the Xanim Process is alive */

  if(gap_debug) printf("poll started on xanim pid: %d\n", (int)xanim_pid);


  /* kill  with signal 0 checks only if the process is alive (no signal is sent)
   *       returns 0 if alive, 1 if no process with given pid found.
   */
  while (0 == kill(xanim_pid, 0))
  {
    usleep(100000);  /* sleep 1 second, and let xanim write some frames */
    if (gap_lib_file_exists(one_past_last_frame))
    {
       /* if the last desired frame is written
        * we can stop (kill with signal 9) the xanim process.
        */
       kill(xanim_pid, 9);
       break;
    }

    /* check for max unwanted frames and delete (upto 20 of them) */
    gap_lib_delete_frames(20, frame_from, frame_to, ext);
  }   
              
  if(gap_debug) printf("poll ended on xanim pid: %d\n", (int)xanim_pid);
}       /* end p_poll */


static int
p_grep(char *pattern, char *file)
{
  gint    l_rc;
  gchar  *l_cmd;

  l_cmd = g_strdup_printf("grep -c '%s' \"%s\" >/dev/null", pattern, file);
  l_rc =  system(l_cmd);
  g_free(l_cmd);
  if (l_rc == 0)
  {
     return(0); /* pattern found */
  }  
  return(1);    /* pattern NOT found */
}

static gint
p_check_xanim()
{
  gint l_rc;
  gint l_grep_counter1;
  gint l_grep_counter2;
  gint l_grep_counter3;
  gchar *l_cmd;
  gchar *l_xanim_help_output;
  FILE *l_fp;

  l_xanim_help_output = gimp_temp_name(".xanim_help.stdout");
  l_fp = g_fopen(l_xanim_help_output, "w+");
  if (l_fp == NULL)
  {
    global_errlist = g_strdup_printf("no write permission for tempfile %s", l_xanim_help_output);
    return(10);
  }
  fprintf(l_fp, "dummy");
  fclose(l_fp);

  /* execute xanim with -h option and 
   * store its output in a file.
   */
  l_cmd = g_strdup_printf("%s -h 2>&1 >>%s", global_xanim_prog, l_xanim_help_output);
  l_rc =  system(l_cmd);

  if(gap_debug) printf("DEBUG: executed :%s\n  Retcode: %d\n", l_cmd, (int)l_rc);
  g_free(l_cmd);
  
  if ((l_rc == 127) || (l_rc == (127 << 8)))
  {
        global_errlist = g_strdup_printf(
                _("could not execute %s (check if xanim is installed)"),
                global_xanim_prog  );
        return(10);
  }

  if(!gap_lib_file_exists(l_xanim_help_output)) 
  {
    global_errlist = g_strdup_printf(
            _("%s does not look like xanim"),
            global_xanim_prog  );
    return(10);
  }
  
  /* check the help output of xanim (using grep) */
  l_grep_counter1 = 0;
  /* l_grep_counter1 += p_grep("anim", l_xanim_help_output); */

  /* check for the exporting options */     
  l_grep_counter2 = 0;
  l_grep_counter2 += p_grep("Ea", l_xanim_help_output);
  l_grep_counter2 += p_grep("Ee", l_xanim_help_output);
  l_grep_counter2 += p_grep("Eq", l_xanim_help_output);

  /* check for the loki version that is able to write single frames */     
  l_grep_counter3 = 0;
  l_grep_counter3 += p_grep("Write video to input/frameN.EXT", l_xanim_help_output);

  g_remove(l_xanim_help_output);
  g_free(l_xanim_help_output);

  if(l_grep_counter2 != 0)
  {
     global_errlist = g_strdup_printf(
             _("The xanim program on your system \"%s\"\ndoes not support the exporting options Ea, Ee, Eq"),
             global_xanim_prog  );
     return(10);
  }
  if(l_grep_counter3 != 0)
  {
     global_errlist = g_strdup_printf(
             _("The xanim program on your system \"%s\"\ndoes not support exporting of single frames"),
             global_xanim_prog  );
     return(10);
  }
  return (0);  /* OK, xanim output looks like expected */
}       /* end p_check_xanim */

static pid_t
p_start_xanim_process(gint32 first_frame, gint32 last_frame,
                  char   *filename,
                  GapXAnimFormats Format,
                  gint32  extract_video,
                  gint32  extract_audio,
                  gint32  jpeg_quality , 
                  char *one_past_last_frame,
                  gint32  run_xanim_asynchron)
{
   gchar  l_cmd[500];
   gchar  l_buf[40];
   pid_t l_xanim_pid;
   int   l_rc;
   FILE  *l_fp;
   
   l_xanim_pid = -1;
   
   /* allocate and prepare args for the xanim call */
   g_snprintf(l_cmd, sizeof(l_cmd), "cd %s;%s +f ", global_xanim_working_dir, global_xanim_prog);  /* programname */
   
   if (extract_audio)
   {
     strcat(l_cmd, "+Ea ");
   }

   if (extract_video)
   {
     strcat(l_cmd, "+v ");    /* +v is verbose mode */

     switch(Format)
     { 
       case XAENC_PPMRAW:
          strcat(l_cmd, "+Ee ");
          break;
       case XAENC_JPEG:
          g_snprintf(l_buf, sizeof(l_buf), "+Eq%d ", (int)jpeg_quality);
          strcat(l_cmd, l_buf);
          break;
       default:
          strcat(l_cmd, "+Ee ");
          break;
      }

     /* additional option "Pause after N Frames" is used,
      * to stop xanim exporting frames beyond the requested limit
      */
     if (run_xanim_asynchron)
     {
       g_snprintf(l_buf, sizeof(l_buf), "+Zp%d ", (int)(last_frame +1));
       strcat(l_cmd, l_buf);
     }
      
   }
   
   /* add the videofilename as last parameter */
   strcat(l_cmd, filename);

   if (run_xanim_asynchron)
   {
     gchar *l_xanim_startscript;
     gchar *l_xanim_pidfile;

     l_xanim_startscript = gimp_temp_name(".xanim_start.sh");
     l_xanim_pidfile = gimp_temp_name(".xanim_pidfile.txt");

     /* asynchron start */
     g_remove(l_xanim_pidfile);   
     /* generate a shellscript */
     l_fp = g_fopen(l_xanim_startscript, "w+");
     if (l_fp != NULL)
     {
         fprintf(l_fp, "#!/bin/sh\n");
         /* fprintf(l_fp, "(%s ; touch %s) &\n" */
         fprintf(l_fp, "%s & # ; touch %s) &\n"
                       , l_cmd                 /* start xanim as background process */
                       , one_past_last_frame   /* and create a dummy frame when xanim is done */
                 );
         fprintf(l_fp, "XANIM_PID=$!\n");
         fprintf(l_fp, "echo \"$XANIM_PID # XANIM_PID\"\n");
         fprintf(l_fp, "echo \"$XANIM_PID # XANIM_PID\" > \"%s\"\n", l_xanim_pidfile);

         /* we pass the xanim pid in a file, 
          * exitcodes are truncated to 8 bit
          * by the system call
          */
         /* fprintf(l_fp, "exit $XANIM_PID\n"); */
         fclose(l_fp);

         gap_file_chmod(l_xanim_startscript, GAP_FILE_MKDIR_MODE);
     }

     l_rc = system(l_xanim_startscript);

     l_fp = g_fopen(l_xanim_pidfile, "r");
     if (l_fp != NULL)
     {
        fscanf(l_fp, "%d", &l_rc);
        fclose(l_fp);
        l_xanim_pid = (pid_t)l_rc;
     }

     g_remove(l_xanim_startscript);
     g_remove(l_xanim_pidfile);
     g_free(l_xanim_startscript);
     g_free(l_xanim_pidfile);

     if(gap_debug) printf("ASYNCHRON CALL: %s\nl_xanim_pid:%d\n", l_cmd, (int)l_xanim_pid);
   }
   else
   {
     /* synchron start (blocks until xanim process has finished */
     l_rc = system(l_cmd);
     if ((l_rc & 0xff) == 0) l_xanim_pid = 0;
     else                    l_xanim_pid = -1;

     if(gap_debug) printf("SYNCHRON CALL: %s\nretcode:%d (%d)\n", l_cmd, (int)l_rc, (int)l_xanim_pid);
   }
  
   return(l_xanim_pid);   
}       /* end p_start_xanim_process */


#ifdef THIS_IS_A_COMMENT_EXEC_DID_NOT_WORK_AND_LEAVES_A_ZOMBIE_PROCESS
static pid_t
p_start_xanim_process_exec(gint32 first_frame, gint32 last_frame,
                  char   *filename,
                  GapXAnimFormats Format,
                  gint32  extract_video,
                  gint32  extract_audio,
                  gint32  jpeg_quality
                  )
{
   char *args[20];
   char  l_buf[40];
   int   l_idx;
   pid_t l_xanim_pid;

   /* allocate and prepare args for the xanim call */
   l_idx = 0;
   args[l_idx] = g_strdup(global_xanim_prog);  /* programname */
   
   l_idx++;
   args[l_idx] = g_strdup("+f");

   if (extract_audio)
   {
     l_idx++;
     args[l_idx] = g_strdup("+Ea");
   }

   if (extract_video)
   {
     l_idx++;
     args[l_idx] = g_strdup("+v");    /* +v is verbose mode */

     l_idx++;
     switch(Format)
     { 
       case XAENC_PPMRAW:
          args[l_idx] = g_strdup("+Ee");
          break;
       case XAENC_JPEG:
          g_snprintf(l_buf, sizeof(l_buf), "+Eq%d", (int)jpeg_quality);
          args[l_idx] = g_strdup(l_buf);
          break;
       default:
          args[l_idx] = g_strdup("+Ee");
          break;
      }

     /* additional option "Pause after N Frames" is used,
      * to stop xanim exporting frames beyond the requested limit
      */      
     l_idx++;
     g_snprintf(l_buf, sizeof(l_buf), "+Zp%d", (int)(last_frame +1));
     args[l_idx] = g_strdup(l_buf);
      
   }
   
   /* add the videofilename as last parameter */
   l_idx++;
   args[l_idx] = g_strdup(filename);
   
   l_idx++;
   args[l_idx] = NULL;  /* terminate args list with a NULL pointer */
   
   l_xanim_pid = fork();

   if(l_xanim_pid == 0)
   {
      /* here we are in the forked child process
       * execute xanim
       */
      execvp(args[0], args);
      
      /* this point should never be reached */
      _exit (1);
   }
   
   return(l_xanim_pid);   
}       /* end p_start_xanim_process */
#endif


/* ============================================================================
 * gap_xanim_decode
 * ============================================================================
 */

gint32
gap_xanim_decode(GimpRunMode run_mode)
{
  gint32 l_rc;
  gint32 first_frame;
  gint32 last_frame;
  char   filename[200];
  char   basename[200];
  char   extension[20];
  char   extension2[20];
  GapXAnimFormats Format;
  gint32 extract_audio;
  gint32 extract_video; 
  gint32 jpeg_quality;
  gint32 autoload;
  gint32 run_xanim_asynchron;
  char   l_cmd[300];
  char   l_one_past_last_frame[200];
  char   l_first_to_laod[200];
  char  *l_dst_dir;
  pid_t  l_xanim_pid;
  int    l_input_dir_created_by_myself;
  
  l_rc = 0;
  l_input_dir_created_by_myself = FALSE;
  global_errlist = NULL;
  p_init_xanim_global_name();
  
  filename[0] = '\0';
  strcpy(&basename[0], "frame_");

  l_rc = p_xanim_dialog (&first_frame,
                 &last_frame,
                 filename, sizeof(filename),
                 basename, sizeof(basename),
                 &Format,
                 &extract_video,
                 &extract_audio,
                 &jpeg_quality,
                 &autoload,
                 &run_xanim_asynchron);


  if(l_rc != 0)
  {
    return(l_rc);
  }
  
  if(!gap_lib_file_exists(filename))
  {
     global_errlist = g_strdup_printf(
            _("videofile %s not existent or empty\n"),
            filename);
            l_rc = 10;
  }
  else
  {
     l_rc = p_check_xanim();
  }
  
  
  if (l_rc == 0)
  {
    switch(Format)
    { 
      case XAENC_PPMRAW:
         strcpy(extension,  "ppm");
         strcpy(extension2, ".ppm");
         break;
      case XAENC_JPEG:
         strcpy(extension,  "jpg");
         strcpy(extension2, ".jpg");
         break;
      default:
         strcpy(extension,  "ppm");
         strcpy(extension2, ".xcf");
         break;

     }
     p_build_xanim_framename(l_one_past_last_frame, sizeof(l_one_past_last_frame), last_frame +1 , extension);

    if (extract_video)
    {
         /* for the frames we need a directory named "input" */
         if (g_file_test(global_xanim_input_dir, G_FILE_TEST_IS_DIR))
         {
           /* the input directory already exists,
            * remove frames
            */
           g_snprintf(l_cmd, sizeof(l_cmd), "rm -f %s%s*.%s", global_xanim_input_dir, G_DIR_SEPARATOR_S, extension);
           system(l_cmd);
         }
         else
         {
            /* create input directory (needed by xanim to store the frames) */
            gap_file_mkdir(global_xanim_input_dir, GAP_FILE_MKDIR_MODE);

            if (g_file_test(global_xanim_input_dir, G_FILE_TEST_IS_DIR))
            {
              l_input_dir_created_by_myself = TRUE;
            }
            else
            {
               global_errlist = g_strdup_printf(
                      _("could not create %s directory\n"
                       "(that is required for xanim frame export)"),
                       global_xanim_input_dir);
               l_rc = 10;
            }
         }
     }
  }
   
  if(l_rc == 0)
  {
     gimp_progress_init (_("Extracting frames..."));
     gimp_progress_update (0.1);  /* fake some progress */
     /* note:
      *  we can't show realistic progress for the extracting process
      *  because we know nothing about videofileformat and how much frames
      *  are realy stored in the videofile.
      *
      *  one guess could assume, that xanim will write 0 upto last_frame
      *  to disk, and check for the frames that the xanim process creates.
      *  The periodically checking can be done in the poll procedure for asynchron
      *  startmode only.
      */

     l_xanim_pid = p_start_xanim_process(first_frame, last_frame,
                                        filename,
                                        Format,
                                        extract_video,
                                        extract_audio,
                                        jpeg_quality,
                                        l_one_past_last_frame,
                                        run_xanim_asynchron);

     if (l_xanim_pid == -1 )
     {
        global_errlist = g_strdup_printf(
           _("could not start xanim process\n(program=%s)"),
           global_xanim_prog  );
        l_rc = -1;
     }
  }

  if(l_rc == 0)
  {
     if(run_xanim_asynchron)
     {
       p_poll(l_xanim_pid, l_one_past_last_frame, first_frame, last_frame, extension);
     }
     
     gap_lib_delete_frames(99999, first_frame, last_frame, extension);
     g_remove(l_one_past_last_frame);

     gimp_progress_update (1.0);
     
     if (p_find_max_xanim_frame (first_frame, extension) < first_frame)
     {
        global_errlist = g_strdup_printf(
               _("can't find any extracted frames,\n"
                 "xanim has failed or was cancelled"));
        l_rc = -1;
     }
     else
     {
       /* if destination directorypart does not exist, try to create it */
       l_dst_dir = g_strdup(basename);
       p_dirname(l_dst_dir);
       if (*l_dst_dir != '\0')
       {
         if ( !g_file_test(l_dst_dir, G_FILE_TEST_IS_DIR) )
         {
            gap_file_mkdir (l_dst_dir, GAP_FILE_MKDIR_MODE);
         }
       }

       if(strcmp(extension, &extension2[1]) == 0)
       {
          gimp_progress_init (_("Renaming frames..."));
          l_rc = gap_lib_rename_frames(first_frame, last_frame, basename, extension);
       }
       else
       {
          gimp_progress_init (_("Converting frames..."));
          l_rc = p_convert_frames(first_frame, last_frame, basename, extension, extension2);
       }

       if (l_input_dir_created_by_myself)
       {
         if (strcmp(l_dst_dir, global_xanim_input_dir) != 0)
         {
            /* remove input dir with all files */
            g_snprintf(l_cmd, sizeof(l_cmd), "rm -rf \"%s\"", global_xanim_input_dir);
            system(l_cmd);         
         }
       }
       g_free(l_dst_dir);
       gimp_progress_update (1.0);
     }
     
   }

   if(l_rc != 0)
   {
      if(global_errlist == NULL)
      {
         p_xanim_info("ERROR: could not execute xanim");
      }
      else
      {
         p_xanim_info(global_errlist);
      }
      l_rc = -1;
   }
   else
   {
     if(autoload)
     {
        /* load first frame and add a display */
        p_build_gap_framename(l_first_to_laod, sizeof(l_first_to_laod), first_frame, basename, &extension2[1]);
        l_rc = gap_lib_load_image(l_first_to_laod);

        if(l_rc >= 0) gimp_display_new(l_rc);
     }
   }

   return(l_rc);    
}       /* end  gap_xanim_decode */
