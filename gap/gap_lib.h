/* gap_lib.h
 * 1997.11.01 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * basic GAP types and utility procedures
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
 * 1.3.19a  2003/09/06   hof: added p_searchpath_for_exefile
 * 1.3.16c  2003/07/07   hof: extend ainfo  onion_triggers
 * 1.3.14a  2003/05/27   hof: moved basic gap operations to new module gap_base_ops
 * 1.3.12a  2003/05/02   hof: merge into CVS-gimp-gap project, added gap_renumber, upto 6digit framenumber support
 * 1.3.11a  2003/01/18   hof: added p_gap_check_save_needed
 * 1.1.35a; 2002/04/21   hof: gap locking (moved to gap_lock.h)
 * 1.1.29a; 2000/11/23   hof: gap locking (changed to procedures and placed here)
 * 1.1.20a; 2000/04/25   hof: new: p_get_video_paste_name p_clear_video_paste
 * 1.1.14a; 2000/01/02   hof: new: p_get_frame_nr
 * 1.1.8a;  1999/08/31   hof: new: p_strdup_del_underscore and p_strdup_add_underscore
 * 0.99.00; 1999/03/15   hof: prepared for win/dos filename conventions
 * 0.96.02; 1998/08/05   hof: extended gap_dup (duplicate range instead of singele frame)
 *                            added gap_shift (framesequence shift)
 * 0.96.00; 1998/06/27   hof: added gap animation sizechange plugins
 *                            (moved range_ops to seperate .h file)
 * 0.94.01; 1998/04/27   hof: added flatten_mode to plugin: gap_range_to_multilayer
 * 0.90.00;              hof: 1.st (pre) release
 */

#ifndef _GAP_LIB_H
#define _GAP_LIB_H

#include "libgimp/gimp.h"

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

typedef struct t_anim_info {
   gint32      image_id;
   char        *basename;    /* may include path */
   long         frame_nr; 
   char        *extension;
   char        *new_filename;
   char        *old_filename;
   GimpRunMode run_mode;
   long         width;       
   long         height;      
   long         type;    
   long         frame_cnt;   
   long         curr_frame_nr; 
   long         first_frame_nr; 
   long         last_frame_nr;
} t_anim_info;

/* procedures used in other gap*.c files */
int          p_file_exists(char *fname);
char*        p_searchpath_for_exefile(const char *exefile, const char *path);
int          p_file_copy(char *fname, char *fname_copy);
void         p_free_ainfo(t_anim_info **ainfo);
char*        p_alloc_basename(const char *imagename, long *number);
char*        p_alloc_extension(char *imagename);
t_anim_info* p_alloc_ainfo(gint32 image_id, GimpRunMode run_mode);
int          p_dir_ainfo(t_anim_info *ainfo_ptr);
int          p_chk_framerange(t_anim_info *ainfo_ptr);
int          p_chk_framechange(t_anim_info *ainfo_ptr);

int    p_save_named_frame (gint32 image_id, char *sav_name);
int    p_load_named_frame (gint32 image_id, char *lod_name);
gint32 p_load_image (char *lod_name);
gint32 p_save_named_image(gint32 image_id, char *sav_name, GimpRunMode run_mode);
char*  p_alloc_fname_fixed_digits(char *basename, long nr, char *extension, long digits);
char*  p_alloc_fname(char *basename, long nr, char *extension);
char*  p_alloc_fname6(char *basename, long nr, char *extension, long default_digits);
gboolean p_exists_frame_nr(t_anim_info *ainfo_ptr, long nr, long *l_has_digits);
char*  p_gzip (char *orig_name, char *new_name, char *zip);
char*  p_strdup_add_underscore(char *name);
char*  p_strdup_del_underscore(char *name);

long  p_get_frame_nr(gint32 image_id);
long  p_get_frame_nr_from_name(char *fname);
int   p_image_file_copy(char *fname, char *fname_copy);


void p_msg_win(GimpRunMode run_mode, char *msg);
gchar *p_get_video_paste_name(void);
gint32 p_vid_edit_clear(void);
gint32 p_vid_edit_framecount(void);
gint   gap_vid_edit_copy(GimpRunMode run_mode, gint32 image_id, long range_from, long range_to);
gint32 gap_vid_edit_paste(GimpRunMode run_mode, gint32 image_id, long paste_mode);
gint32 p_getpid(void);
gint   p_pid_is_alive(gint32 pid);

gboolean p_gap_lock_is_locked(gint32 image_id, GimpRunMode run_mode);
void     p_gap_lock_set(gint32 image_id);
void     p_gap_lock_remove(gint32 image_id);

gboolean p_gap_check_save_needed(gint32 image_id);

int      p_rename_frame(t_anim_info *ainfo_ptr, long from_nr, long to_nr);
int      p_delete_frame(t_anim_info *ainfo_ptr, long nr);
gint32   p_replace_image(t_anim_info *ainfo_ptr);


#define  VID_PASTE_REPLACE         0
#define  VID_PASTE_INSERT_BEFORE   1
#define  VID_PASTE_INSERT_AFTER    2

#endif


