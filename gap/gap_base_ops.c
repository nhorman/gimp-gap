/* gap_lib.c
 * 1997.11.18 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * basic Videomenu functions:
 *   Delete, Duplicate, Exchange, Shift 
 *   Next, Prev, First, Last, Goto
 * 
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
 * 1.3.14a  2003/05/27   hof: created (module was splitted off from gap_lib)
 */

#include "config.h"
 
/* SYSTEM (UNIX) includes */ 
#include <stdio.h>
#include <stdlib.h>

/* GIMP includes */
#include "gtk/gtk.h"
#include "gap-intl.h"
#include "libgimp/gimp.h"


/* GAP includes */
#include "gap_layer_copy.h"
#include "gap_lib.h"
#include "gap_base_ops.h"
#include "gap_pdb_calls.h"
#include "gap_arr_dialog.h"
#include "gap_lock.h"
#include "gap_thumbnail.h"


extern      int gap_debug; /* ==0  ... dont print debug infos */
 


/* ============================================================================
 * p_del
 *
 * delete cnt frames starting at current
 * all following frames are renamed (renumbered down by cnt) 
 *
 * return image_id (of the new loaded frame) on success
 *        or -1 on errors
 * ============================================================================
 */
static gint32
p_del(t_anim_info *ainfo_ptr, long cnt)
{
   long  l_lo, l_hi, l_curr, l_idx;

   if(gap_debug) fprintf(stderr, "DEBUG  p_del\n");

   if(cnt < 1) return -1;

   l_curr =  ainfo_ptr->curr_frame_nr;
   if((1 + ainfo_ptr->last_frame_nr - l_curr) < cnt)
   {
      /* limt cnt to last existing frame */
      cnt = 1 + ainfo_ptr->frame_cnt - l_curr;
   }

   if(cnt >= ainfo_ptr->frame_cnt)
   {
      /* dont want to delete all frames
       * so we have to leave a rest of one frame
       */
      cnt--;
   }

   
   l_idx   = l_curr;
   while(l_idx < (l_curr + cnt))
   {
      p_delete_frame(ainfo_ptr, l_idx);
      l_idx++;
   }
   
   /* rename (renumber) all frames with number greater than current
    */
   l_lo   = l_curr;
   l_hi   = l_curr + cnt;
   while(l_hi <= ainfo_ptr->last_frame_nr)
   {
     if(0 != p_rename_frame(ainfo_ptr, l_hi, l_lo))
     {
        gchar *tmp_errtxt;
	
        tmp_errtxt = g_strdup_printf(_("Error: could not rename frame %ld to %ld") ,l_hi, l_lo);
        p_msg_win(ainfo_ptr->run_mode, tmp_errtxt);
	g_free(tmp_errtxt);
        return -1;
     }
     l_lo++;
     l_hi++;
   }

   /* calculate how much frames are left */
   ainfo_ptr->frame_cnt -= cnt;
   ainfo_ptr->last_frame_nr = ainfo_ptr->first_frame_nr + ainfo_ptr->frame_cnt -1;
   
   /* set current position to previous frame (if there is one) */
   if(l_curr > ainfo_ptr->first_frame_nr) 
   { 
     ainfo_ptr->frame_nr = l_curr -1;
   }
   else
   { 
      ainfo_ptr->frame_nr = ainfo_ptr->first_frame_nr;
   }

   /* make filename, then load the new current frame */
   if(ainfo_ptr->new_filename != NULL) g_free(ainfo_ptr->new_filename);
   ainfo_ptr->new_filename = p_alloc_fname(ainfo_ptr->basename,
                                      ainfo_ptr->frame_nr,
                                      ainfo_ptr->extension);

   if(ainfo_ptr->new_filename == NULL)
      return -1;

   return (p_load_named_frame(ainfo_ptr->image_id, ainfo_ptr->new_filename));

}        /* end p_del */

/* ============================================================================
 * p_dup
 *
 * all following frames are renamed (renumbered up by cnt) 
 * current frame is duplicated (cnt) times
 *
 * return image_id (of the new loaded frame) on success
 *        or -1 on errors
 * ============================================================================
 */
gint32
p_dup(t_anim_info *ainfo_ptr, long cnt, long range_from, long range_to)
{
   long  l_lo, l_hi;
   long  l_cnt2;
   long  l_step;
   long  l_src_nr, l_src_nr_min, l_src_nr_max;
   char  *l_dup_name;
   char  *l_curr_name;
   gdouble    l_percentage, l_percentage_step;
 
   if(gap_debug) fprintf(stderr, "DEBUG  p_dup fr:%d to:%d cnt:%d extension:%s: basename:%s frame_cnt:%d\n",
                         (int)range_from, (int)range_to, (int)cnt, ainfo_ptr->extension, ainfo_ptr->basename, (int)ainfo_ptr->frame_cnt);

   if(cnt < 1) return -1;

   l_curr_name = p_alloc_fname(ainfo_ptr->basename, ainfo_ptr->curr_frame_nr, ainfo_ptr->extension);
   
   if(p_gap_check_save_needed(ainfo_ptr->image_id))
   {
     /* save current frame  */   
     if(p_save_named_frame(ainfo_ptr->image_id, l_curr_name) < 0)
     {
       gchar *tmp_errtxt;
       tmp_errtxt = g_strdup_printf(_("Error: could not save frame %s"), l_curr_name);
       p_msg_win(ainfo_ptr->run_mode, tmp_errtxt);
       g_free(tmp_errtxt);
       return -1;
     }
   }

   /* use a new name (000001.xcf Konvention) */ 
   gimp_image_set_filename (ainfo_ptr->image_id, l_curr_name);
   g_free(l_curr_name);


   if((range_from <0 ) && (range_to < 0 ))
   {
      /* set range to one single current frame
       * (used for the old non_interactive PDB-interface without range params)
       */
      range_from = ainfo_ptr->curr_frame_nr;
      range_to   = ainfo_ptr->curr_frame_nr;
   }

   /* clip range */
   if(range_from > ainfo_ptr->last_frame_nr)  range_from = ainfo_ptr->last_frame_nr;
   if(range_to   > ainfo_ptr->last_frame_nr)  range_to   = ainfo_ptr->last_frame_nr;
   if(range_from < ainfo_ptr->first_frame_nr) range_from = ainfo_ptr->first_frame_nr;
   if(range_to   < ainfo_ptr->first_frame_nr) range_to   = ainfo_ptr->first_frame_nr;

   if(range_to < range_from)
   {
       /* invers range */
      l_cnt2 = ((range_from - range_to ) + 1) * cnt;
      l_step = -1;
      l_src_nr_max = range_from;
      l_src_nr_min = range_to;
   }
   else
   {
      l_cnt2 = ((range_to - range_from ) + 1) * cnt;  
      l_step = 1;
      l_src_nr_max = range_to;
      l_src_nr_min = range_from;
   }    

   if(gap_debug) fprintf(stderr, "DEBUG  p_dup fr:%d to:%d cnt2:%d l_src_nr_max:%d\n",
                         (int)range_from, (int)range_to, (int)l_cnt2, (int)l_src_nr_max);
   

   l_percentage = 0.0;  
   if(ainfo_ptr->run_mode == GIMP_RUN_INTERACTIVE)
   { 
     gimp_progress_init( _("Duplicating frames..."));
   }

   /* rename (renumber) all frames with number greater than current
    */
   l_lo   = ainfo_ptr->last_frame_nr;
   l_hi   = l_lo + l_cnt2;
   while(l_lo > l_src_nr_max)
   {     
     if(0 != p_rename_frame(ainfo_ptr, l_lo, l_hi))
     {
        gchar *tmp_errtxt;
        tmp_errtxt = g_strdup_printf(_("Error: could not rename frame %ld to %ld"), l_lo, l_hi);
        p_msg_win(ainfo_ptr->run_mode, tmp_errtxt);
	g_free(tmp_errtxt);
        return -1;
     }
     l_lo--;
     l_hi--;
   }


   l_percentage_step = 1.0 / ((1.0 + l_hi) - l_src_nr_max);
   if(ainfo_ptr->run_mode == GIMP_RUN_INTERACTIVE)
   { 
      l_percentage += l_percentage_step;
      gimp_progress_update (l_percentage);
   }

   /* copy cnt duplicates */   
   l_src_nr = range_to;
   while(l_hi > l_src_nr_max)
   {
      l_curr_name = p_alloc_fname(ainfo_ptr->basename, l_src_nr, ainfo_ptr->extension);  
      l_dup_name = p_alloc_fname(ainfo_ptr->basename, l_hi, ainfo_ptr->extension);
      if((l_dup_name != NULL) && (l_curr_name != NULL))
      {
         p_image_file_copy(l_curr_name, l_dup_name);
         g_free(l_dup_name);
         g_free(l_curr_name);
      }
      if(ainfo_ptr->run_mode == GIMP_RUN_INTERACTIVE)
      { 
         l_percentage += l_percentage_step;
         gimp_progress_update (l_percentage);
      }
      
      
      l_src_nr -= l_step;
      if(l_src_nr < l_src_nr_min) l_src_nr = l_src_nr_max;
      if(l_src_nr > l_src_nr_max) l_src_nr = l_src_nr_min;
      
      l_hi--;
   }

   /* restore current position */
   ainfo_ptr->frame_cnt += l_cnt2;
   ainfo_ptr->last_frame_nr = ainfo_ptr->first_frame_nr + ainfo_ptr->frame_cnt -1;

   /* load from the "new" current frame */   
   if(ainfo_ptr->new_filename != NULL) g_free(ainfo_ptr->new_filename);
   ainfo_ptr->new_filename = p_alloc_fname(ainfo_ptr->basename,
                                      ainfo_ptr->curr_frame_nr,
                                      ainfo_ptr->extension);
   return (p_load_named_frame(ainfo_ptr->image_id, ainfo_ptr->new_filename));
}        /* end p_dup */

/* ============================================================================
 * p_exchg
 *
 * save current frame, exchange its name with destination frame on disk 
 * and reload current frame (now has contents of dest. and vice versa)
 *
 * return image_id (of the new loaded frame) on success
 *        or -1 on errors
 * ============================================================================
 */
gint32
p_exchg(t_anim_info *ainfo_ptr, long dest)
{
   long  l_tmp_nr;
   gchar *tmp_errtxt;

   l_tmp_nr = ainfo_ptr->last_frame_nr + 4;  /* use a free frame_nr for temp name */

   if((dest < 1) || (dest == ainfo_ptr->curr_frame_nr)) 
      return -1;

   if(p_gap_check_save_needed(ainfo_ptr->image_id))
   {
     if(p_save_named_frame(ainfo_ptr->image_id, ainfo_ptr->old_filename) < 0)
        return -1;
   }

   /* rename (renumber) frames */
   if(0 != p_rename_frame(ainfo_ptr, dest, l_tmp_nr))
   {
        tmp_errtxt = g_strdup_printf(_("Error: could not rename frame %ld to %ld"), dest, l_tmp_nr);
        p_msg_win(ainfo_ptr->run_mode, tmp_errtxt);
	g_free(tmp_errtxt);
        return -1;
   }
   if(0 != p_rename_frame(ainfo_ptr, ainfo_ptr->curr_frame_nr, dest))
   {
        tmp_errtxt = g_strdup_printf(_("Error: could not rename frame %ld to %ld"), ainfo_ptr->curr_frame_nr, dest);
        p_msg_win(ainfo_ptr->run_mode, tmp_errtxt);
	g_free(tmp_errtxt);
        return -1;
   }
   if(0 != p_rename_frame(ainfo_ptr, l_tmp_nr, ainfo_ptr->curr_frame_nr))
   {
        tmp_errtxt = g_strdup_printf(_("Error: could not rename frame %ld to %ld"), l_tmp_nr, ainfo_ptr->curr_frame_nr);
        p_msg_win(ainfo_ptr->run_mode, tmp_errtxt);
	g_free(tmp_errtxt);
        return -1;
   }

   /* load from the "new" current frame */   
   if(ainfo_ptr->new_filename != NULL) g_free(ainfo_ptr->new_filename);
   ainfo_ptr->new_filename = p_alloc_fname(ainfo_ptr->basename,
                                      ainfo_ptr->curr_frame_nr,
                                      ainfo_ptr->extension);
   return(p_load_named_frame(ainfo_ptr->image_id, ainfo_ptr->new_filename));
}        /* end p_exchg */

/* ============================================================================
 * p_shift
 *
 * all frmaes in the given range are renumbered (shifted)
 * according to cnt:
 *  example:  cnt == 1 :  range before 3, 4, 5, 6, 7
 *                        range after  4, 5, 6, 7, 3
 *
 * return image_id (of the new loaded frame) on success
 *        or -1 on errors
 * ============================================================================
 */
static gint32
p_shift(t_anim_info *ainfo_ptr, long cnt, long range_from, long range_to)
{
   long  l_lo, l_hi, l_curr, l_dst;
   long  l_upper;
   long  l_shift;
   gchar *l_curr_name;
   gchar *tmp_errtxt;
	
   gdouble    l_percentage, l_percentage_step;  

   if(gap_debug) fprintf(stderr, "DEBUG  p_shift fr:%d to:%d cnt:%d\n",
                         (int)range_from, (int)range_to, (int)cnt);

   if(range_from == range_to) return -1;

   /* clip range */
   if(range_from > ainfo_ptr->last_frame_nr)  range_from = ainfo_ptr->last_frame_nr;
   if(range_to   > ainfo_ptr->last_frame_nr)  range_to   = ainfo_ptr->last_frame_nr;
   if(range_from < ainfo_ptr->first_frame_nr) range_from = ainfo_ptr->first_frame_nr;
   if(range_to   < ainfo_ptr->first_frame_nr) range_to   = ainfo_ptr->first_frame_nr;

   if(range_to < range_from)
   {
      l_lo = range_to;
      l_hi = range_from;
   }
   else
   {
      l_lo = range_from;
      l_hi = range_to;
   }
   
   /* limit shift  amount to number of frames in range */
   l_shift = cnt % (l_hi - l_lo);
   if(gap_debug) fprintf(stderr, "DEBUG  p_shift shift:%d\n",
                         (int)l_shift);
   if(l_shift == 0) return -1;

   l_curr_name = p_alloc_fname(ainfo_ptr->basename, ainfo_ptr->curr_frame_nr, ainfo_ptr->extension);
   if(p_gap_check_save_needed(ainfo_ptr->image_id))
   {
     /* save current frame  */   
     p_save_named_frame(ainfo_ptr->image_id, l_curr_name);
   }
   g_free(l_curr_name);

   l_percentage = 0.0;  
   if(ainfo_ptr->run_mode == GIMP_RUN_INTERACTIVE)
   { 
     gimp_progress_init( _("Renumber Framesequence..."));
   }

   /* rename (renumber) all frames (using high numbers)
    */

   l_upper = ainfo_ptr->last_frame_nr +100;
   l_percentage_step = 0.5 / ((1.0 + l_lo) - l_hi);
   for(l_curr = l_lo; l_curr <= l_hi; l_curr++)
   {
     if(0 != p_rename_frame(ainfo_ptr, l_curr, l_curr + l_upper))
     {
        tmp_errtxt = g_strdup_printf(_("Error: could not rename frame %ld to %ld"), l_lo, l_hi);
        p_msg_win(ainfo_ptr->run_mode, tmp_errtxt);
	g_free(tmp_errtxt);
        return -1;
     }
     if(ainfo_ptr->run_mode == GIMP_RUN_INTERACTIVE)
     { 
       l_percentage += l_percentage_step;
       gimp_progress_update (l_percentage);
     }
   }

   /* rename (renumber) all frames (using desied destination numbers)
    */
   l_dst = l_lo + l_shift;
   if (l_dst > l_hi) { l_dst -= (l_lo -1); }
   if (l_dst < l_lo) { l_dst += ((l_hi - l_lo) +1); }
   for(l_curr = l_upper + l_lo; l_curr <= l_upper + l_hi; l_curr++)
   {
     if (l_dst > l_hi) { l_dst = l_lo; }
     if(0 != p_rename_frame(ainfo_ptr, l_curr, l_dst))
     {
        tmp_errtxt = g_strdup_printf(_("Error: could not rename frame %ld to %ld"), l_lo, l_hi);
        p_msg_win(ainfo_ptr->run_mode, tmp_errtxt);
	g_free(tmp_errtxt);
        return -1;
     }
     if(ainfo_ptr->run_mode == GIMP_RUN_INTERACTIVE)
     { 
       l_percentage += l_percentage_step;
       gimp_progress_update (l_percentage);
     }

     l_dst ++;     
   }


   /* load from the "new" current frame */   
   if(ainfo_ptr->new_filename != NULL) g_free(ainfo_ptr->new_filename);
   ainfo_ptr->new_filename = p_alloc_fname(ainfo_ptr->basename,
                                      ainfo_ptr->curr_frame_nr,
                                      ainfo_ptr->extension);
   return(p_load_named_frame(ainfo_ptr->image_id, ainfo_ptr->new_filename));
}  /* end p_shift */








/* ============================================================================
 * gap_next gap_prev
 *
 * store the current Gimp Image to the current anim Frame
 * and load it from the next/prev anim Frame on disk.
 *
 * return image_id (of the new loaded frame) on success
 *        or -1 on errors
 * ============================================================================
 */
gint32
gap_next(GimpRunMode run_mode, gint32 image_id)
{
  int rc;
  t_anim_info *ainfo_ptr;

  rc = -1;
  ainfo_ptr = p_alloc_ainfo(image_id, run_mode);
  if(ainfo_ptr != NULL)
  {
    ainfo_ptr->frame_nr = ainfo_ptr->curr_frame_nr + 1;
    rc = p_replace_image(ainfo_ptr);
  
    p_free_ainfo(&ainfo_ptr);
  }
  
  return(rc);    
}	/* end gap_next */

gint32
gap_prev(GimpRunMode run_mode, gint32 image_id)
{
  int rc;
  t_anim_info *ainfo_ptr;

  rc = -1;
  ainfo_ptr = p_alloc_ainfo(image_id, run_mode);
  if(ainfo_ptr != NULL)
  {
    ainfo_ptr->frame_nr = ainfo_ptr->curr_frame_nr - 1;
    rc = p_replace_image(ainfo_ptr);
  
    p_free_ainfo(&ainfo_ptr);
  }
  
  return(rc);    
}	/* end gap_prev */

/* ============================================================================
 * gap_first  gap_last
 *
 * store the current Gimp Image to the current anim Frame
 * and load it from the first/last anim Frame on disk.
 * ============================================================================
 */

gint32
gap_first(GimpRunMode run_mode, gint32 image_id)
{
  int rc;
  t_anim_info *ainfo_ptr;

  rc = -1;
  ainfo_ptr = p_alloc_ainfo(image_id, run_mode);
  if(ainfo_ptr != NULL)
  {
    if (0 == p_dir_ainfo(ainfo_ptr))
    {
      ainfo_ptr->frame_nr = ainfo_ptr->first_frame_nr;
      rc = p_replace_image(ainfo_ptr);
    }
    p_free_ainfo(&ainfo_ptr);
  }
  
  return(rc);    
}	/* end gap_first */

gint32
gap_last(GimpRunMode run_mode, gint32 image_id)
{
  int rc;
  t_anim_info *ainfo_ptr;

  rc = -1;
  ainfo_ptr = p_alloc_ainfo(image_id, run_mode);
  if(ainfo_ptr != NULL)
  {
    if (0 == p_dir_ainfo(ainfo_ptr))
    {
      ainfo_ptr->frame_nr = ainfo_ptr->last_frame_nr;
      rc = p_replace_image(ainfo_ptr);
    }
    p_free_ainfo(&ainfo_ptr);
  }
  
  return(rc);    
}	/* end gap_last */

/* ============================================================================
 * gap_goto
 * 
 * store the current Gimp Image to disk
 * and load it from the anim Frame on disk that has the specified frame Nr.
 * GIMP_RUN_INTERACTIVE:
 *    show dialogwindow where user can enter the destination frame Nr.
 * ============================================================================
 */

gint32
gap_goto(GimpRunMode run_mode, gint32 image_id, int nr)
{
  int rc;
  t_anim_info *ainfo_ptr;

  long            l_dest;
  gchar          *l_hline;
  gchar          *l_title;

  rc = -1;
  ainfo_ptr = p_alloc_ainfo(image_id, run_mode);
  if(ainfo_ptr != NULL)
  {
    if (0 == p_dir_ainfo(ainfo_ptr))
    {
      if(0 != p_chk_framerange(ainfo_ptr))   return -1;

      if(run_mode == GIMP_RUN_INTERACTIVE)
      {
        l_title = g_strdup_printf (_("Goto Frame (%ld/%ld)")
				   , ainfo_ptr->curr_frame_nr
				   , ainfo_ptr->frame_cnt);
        l_hline =  g_strdup_printf (_("Destination Frame Number (%ld  - %ld)")
				    , ainfo_ptr->first_frame_nr
				    , ainfo_ptr->last_frame_nr);

        l_dest = p_slider_dialog(l_title, l_hline, _("Number:"), NULL
                , ainfo_ptr->first_frame_nr
                , ainfo_ptr->last_frame_nr
                , ainfo_ptr->curr_frame_nr
                , TRUE);

	g_free (l_title);
	g_free (l_hline);
                
        if(l_dest < 0)
        {
           /* Cancel button: go back to current frame */
           l_dest = ainfo_ptr->curr_frame_nr;
        }  
        if(0 != p_chk_framechange(ainfo_ptr))
        {
           l_dest = -1;
        }
      }
      else
      {
        l_dest = nr;
      }

      if(l_dest >= 0)
      {
        ainfo_ptr->frame_nr = l_dest;
        rc = p_replace_image(ainfo_ptr);
      }

    }
    p_free_ainfo(&ainfo_ptr);
  }
  
  return(rc);    
}	/* end gap_goto */

/* ============================================================================
 * gap_del
 * ============================================================================
 */
gint32
gap_del(GimpRunMode run_mode, gint32 image_id, int nr)
{
  int rc;
  t_anim_info *ainfo_ptr;

  long           l_cnt;
  long           l_max;
  gchar         *l_hline;
  gchar         *l_title;

  rc = -1;
  ainfo_ptr = p_alloc_ainfo(image_id, run_mode);
  if(ainfo_ptr != NULL)
  {
    if (0 == p_dir_ainfo(ainfo_ptr))
    {
      if(0 != p_chk_framerange(ainfo_ptr))   return -1;
      
      if(run_mode == GIMP_RUN_INTERACTIVE)
      {
        l_title = g_strdup_printf (_("Delete Frames (%ld/%ld)")
				   , ainfo_ptr->curr_frame_nr
				   , ainfo_ptr->frame_cnt);
        l_hline = g_strdup_printf (_("Delete Frames from %ld to (number)")
				   , ainfo_ptr->curr_frame_nr);

        l_max = ainfo_ptr->last_frame_nr;
        if(l_max == ainfo_ptr->curr_frame_nr)
        { 
          /* bugfix: the slider needs a maximum > minimum
           *         (if not an error is printed, and
           *          a default range 0 to 100 is displayed)
           */
          l_max++;
        }
        
        l_cnt = p_slider_dialog(l_title, l_hline, _("Number:"), NULL
              , ainfo_ptr->curr_frame_nr
              , l_max
              , ainfo_ptr->curr_frame_nr
              , TRUE);
                
	g_free (l_title);
	g_free (l_hline);
	
        if(l_cnt >= 0)
        {
           l_cnt = 1 + l_cnt - ainfo_ptr->curr_frame_nr;
        } 
        if(0 != p_chk_framechange(ainfo_ptr))
        {
           l_cnt = -1;
        }
      }
      else
      {
        l_cnt = nr;
      }
 
      if(l_cnt >= 0)
      {
         /* delete l_cnt number of frames (-1 CANCEL button) */

         rc = p_del(ainfo_ptr, l_cnt);
      }


    }
    p_free_ainfo(&ainfo_ptr);
  }
  
  return(rc);    

}	/* end gap_del */


/* ============================================================================
 * p_dup_dialog
 *
 * ============================================================================
 */
gint32
p_dup_dialog(t_anim_info *ainfo_ptr, long *range_from, long *range_to)
{
  static t_arr_arg  argv[3];
  gchar            *l_title;

  l_title = g_strdup_printf (_("Duplicate Frames (%ld/%ld)")
			     , ainfo_ptr->curr_frame_nr
			     , ainfo_ptr->frame_cnt);

  p_init_arr_arg(&argv[0], WGT_INT_PAIR);
  argv[0].label_txt = _("From:");
  argv[0].constraint = TRUE;
  argv[0].int_min   = (gint)ainfo_ptr->first_frame_nr;
  argv[0].int_max   = (gint)ainfo_ptr->last_frame_nr;
  argv[0].int_ret   = (gint)ainfo_ptr->curr_frame_nr;
  argv[0].help_txt  = _("Source Range starts at this framenumber");

  p_init_arr_arg(&argv[1], WGT_INT_PAIR);
  argv[1].label_txt = _("To:");
  argv[1].constraint = TRUE;
  argv[1].int_min   = (gint)ainfo_ptr->first_frame_nr;
  argv[1].int_max   = (gint)ainfo_ptr->last_frame_nr;
  argv[1].int_ret   = (gint)ainfo_ptr->curr_frame_nr;
  argv[1].help_txt  = _("Source Range ends at this framenumber");
    
  p_init_arr_arg(&argv[2], WGT_INT_PAIR);
  argv[2].label_txt = _("N times:");
  argv[2].constraint = FALSE;
  argv[2].int_min   = 1;
  argv[2].int_max   = 99;
  argv[2].int_ret   = 1;
  argv[2].umin      = 1;
  argv[2].umax      = 9999;
  argv[2].help_txt  = _("Copy selected Range n-times  \n(you may type in Values > 99)");


  if(TRUE == p_array_dialog(l_title, _("Duplicate Frame Range"),  3, argv))
  { 
    g_free (l_title);
    *range_from = (long)(argv[0].int_ret);
    *range_to   = (long)(argv[1].int_ret);
       return (int)(argv[2].int_ret);
  }
  else
  {
    g_free (l_title);
    return -1;
  }
   

}	/* end p_dup_dialog */


/* ============================================================================
 * gap_dup
 * ============================================================================
 */
gint32
gap_dup(GimpRunMode run_mode, gint32 image_id, int nr,
            long range_from, long range_to)
{
  int rc;
  t_anim_info *ainfo_ptr;

  long           l_cnt, l_from, l_to;

  rc = -1;
  ainfo_ptr = p_alloc_ainfo(image_id, run_mode);
  if(ainfo_ptr != NULL)
  {
    if (0 == p_dir_ainfo(ainfo_ptr))
    {
      if(run_mode == GIMP_RUN_INTERACTIVE)
      {
         if(0 != p_chk_framechange(ainfo_ptr)) { l_cnt = -1; }
         else
	 {
	   if(*ainfo_ptr->extension == '\0' && ainfo_ptr->frame_cnt == 0)
	   {
	     /* duplicate was called on a frame without extension and without framenumer in its name
	      * (typical for new created images named like 'Untitled' (or 'Unbenannt' for german GUI or .. in other languages)
	      */
	       p_msg_win(ainfo_ptr->run_mode,
		       _("OPERATION CANCELLED.\n"
			 "GAP plug-ins only work with filenames\n"
			 "that end in numbers like _000001.xcf.\n"
			 "==> Rename your image, then try again."));
	       return -1;
	   }
	   l_cnt = p_dup_dialog(ainfo_ptr, &l_from, &l_to);
	 }

         if((0 != p_chk_framechange(ainfo_ptr)) || (l_cnt < 1))
         {
            l_cnt = -1;
         }
                
      }
      else
      {
        l_cnt  = nr;
        l_from = range_from;
        l_to   = range_to;
      }
 
      if(l_cnt > 0)
      {
         /* make l_cnt duplicate frames (on disk) */
         rc = p_dup(ainfo_ptr, l_cnt, l_from, l_to);
      }


    }
    p_free_ainfo(&ainfo_ptr);
  }
  
  return(rc);    

 
}	/* end gap_dup */


/* ============================================================================
 * gap_exchg
 * ============================================================================
 */

gint32
gap_exchg(GimpRunMode run_mode, gint32 image_id, int nr)
{
  int rc;
  t_anim_info *ainfo_ptr;

  long           l_dest;
  long           l_initial;
  gchar         *l_title;

  rc = -1;
  l_initial = 1;
  ainfo_ptr = p_alloc_ainfo(image_id, run_mode);
  if(ainfo_ptr != NULL)
  {
    if (0 == p_dir_ainfo(ainfo_ptr))
    {
      if(0 != p_chk_framerange(ainfo_ptr))   return -1;
      
      if(run_mode == GIMP_RUN_INTERACTIVE)
      {
         if(ainfo_ptr->curr_frame_nr < ainfo_ptr->last_frame_nr)
         {
           l_initial = ainfo_ptr->curr_frame_nr + 1;
         }
         else
         {
           l_initial = ainfo_ptr->last_frame_nr; 
         }
         l_title = g_strdup_printf (_("Exchange current Frame (%ld)")
				    , ainfo_ptr->curr_frame_nr);

         l_dest = p_slider_dialog(l_title, 
				  _("With Frame (number)"), 
				  _("Number:"), NULL
				  , ainfo_ptr->first_frame_nr 
				  , ainfo_ptr->last_frame_nr
				  , l_initial
				  , TRUE);
	 g_free (l_title);
				  
         if(0 != p_chk_framechange(ainfo_ptr))
         {
            l_dest = -1;
         }
      }
      else
      {
        l_dest = nr;
      }
 
      if((l_dest >= ainfo_ptr->first_frame_nr ) && (l_dest <= ainfo_ptr->last_frame_nr ))
      {
         /* excange current frames with destination frame (on disk) */
         rc = p_exchg(ainfo_ptr, l_dest);
      }


    }
    p_free_ainfo(&ainfo_ptr);
  }
  
  return(rc);    
}	/* end gap_exchg */

/* ============================================================================
 * p_shift_dialog
 *
 * ============================================================================
 */
gint32
p_shift_dialog(t_anim_info *ainfo_ptr, long *range_from, long *range_to)
{
  static t_arr_arg  argv[3];
  gchar            *l_title;

  l_title = g_strdup_printf (_("Framesequence Shift (%ld/%ld)")
			     , ainfo_ptr->curr_frame_nr
			     , ainfo_ptr->frame_cnt);

  p_init_arr_arg(&argv[0], WGT_INT_PAIR);
  argv[0].label_txt = _("From:");
  argv[0].constraint = TRUE;
  argv[0].int_min   = (gint)ainfo_ptr->first_frame_nr;
  argv[0].int_max   = (gint)ainfo_ptr->last_frame_nr;
  argv[0].int_ret   = (gint)ainfo_ptr->curr_frame_nr;
  argv[0].help_txt  = _("Affected Range starts at this framenumber");

  p_init_arr_arg(&argv[1], WGT_INT_PAIR);
  argv[1].label_txt = _("To:");
  argv[1].constraint = TRUE;
  argv[1].int_min   = (gint)ainfo_ptr->first_frame_nr;
  argv[1].int_max   = (gint)ainfo_ptr->last_frame_nr;
  argv[1].int_ret   = (gint)ainfo_ptr->last_frame_nr;
  argv[1].help_txt  = _("Affected Range ends at this framenumber");
    
  p_init_arr_arg(&argv[2], WGT_INT_PAIR);
  argv[2].label_txt = _("N-Shift:");
  argv[2].constraint = TRUE;
  argv[2].int_min   = -1 * (gint)ainfo_ptr->last_frame_nr;
  argv[2].int_max   = (gint)ainfo_ptr->last_frame_nr;
  argv[2].int_ret   = 1;
  argv[2].help_txt  = _("Renumber the affected framesequence     \n(numbers are shifted in circle by N)");
  
  if(TRUE == p_array_dialog(l_title, _("Framesequence shift"),  3, argv))
  { 
    g_free (l_title);
    *range_from = (long)(argv[0].int_ret);
    *range_to   = (long)(argv[1].int_ret);
    return (int)(argv[2].int_ret);
  }
  else
  {
    g_free (l_title);
    return 0;
  }
   

}	/* end p_shift_dialog */


/* ============================================================================
 * gap_shift
 * ============================================================================
 */
gint32
gap_shift(GimpRunMode run_mode, gint32 image_id, int nr,
            long range_from, long range_to)
{
  int rc;
  t_anim_info *ainfo_ptr;

  long           l_cnt, l_from, l_to;

  rc = -1;
  ainfo_ptr = p_alloc_ainfo(image_id, run_mode);
  if(ainfo_ptr != NULL)
  {
    if (0 == p_dir_ainfo(ainfo_ptr))
    {
      if(run_mode == GIMP_RUN_INTERACTIVE)
      {
         l_cnt = 1;
         if(0 != p_chk_framechange(ainfo_ptr)) { l_cnt = 0; }
         else { l_cnt = p_shift_dialog(ainfo_ptr, &l_from, &l_to); }

         if((0 != p_chk_framechange(ainfo_ptr)) || (l_cnt == 0))
         {
            l_cnt = 0;
         }
                
      }
      else
      {
        l_cnt  = nr;
        l_from = range_from;
        l_to   = range_to;
      }
 
      if(l_cnt != 0)
      {
         /* shift framesquence by l_cnt frames 
          * (rename all frames in the given range on disk)
          */
         rc = p_shift(ainfo_ptr, l_cnt, l_from, l_to);
      }


    }
    p_free_ainfo(&ainfo_ptr);
  }
  
  return(rc);    

 
}	/* end gap_shift */



/* --------------------------------
 * p_renumber_dialog
 * --------------------------------
 */
static int 
p_renumber_dialog(t_anim_info *ainfo_ptr, long *start_frame_nr, long *digits)
{
  static t_arr_arg  argv[2];
  gchar            *l_title;

  l_title = g_strdup_printf (_("Renumber Frames (%ld)")
			     , ainfo_ptr->frame_cnt);

  p_init_arr_arg(&argv[0], WGT_INT_PAIR);
  argv[0].label_txt = _("Start Frame Number:");
  argv[0].constraint = TRUE;
  argv[0].int_min   = (gint)0;
  argv[0].int_max   = (gint)999999;
  argv[0].int_ret   = (gint)1;
  argv[0].help_txt  = _("New Framenumber for the 1.st frame");

  p_init_arr_arg(&argv[1], WGT_INT_PAIR);
  argv[1].label_txt = _("Digits:");
  argv[1].constraint = TRUE;
  argv[1].int_min   = 1;
  argv[1].int_max   = 6;
  argv[1].int_ret   = 6;
  argv[1].help_txt  = _("How many digits to use for the framenumber filenamepart");
    


  if(TRUE == p_array_dialog(l_title, _("Renumber Frames"),  2, argv))
  { 
    g_free (l_title);
    *start_frame_nr = (long)(argv[0].int_ret);
    *digits   = (long)(argv[1].int_ret);
    return (0);
  }
  else
  {
    g_free (l_title);
    return -1;
  }

}	/* end p_renumber_dialog */


/* ------------------------
 * p_rename_frame_digits
 * ------------------------
 * rename framename. the source framename number part must match with from_nr
 * the resulting name has a number part with to_nr, filled up with leading zeroes 
 * to the specified number of digits.
 * if the current frame is renumbered, we also set the image filename,
 * and keep track of the new current number
 */
int 
p_rename_frame_digits(t_anim_info *ainfo_ptr, long from_nr, long to_nr, long from_digits, long to_digits)
{
   char          *l_from_fname;
   char          *l_to_fname;
   int            l_rc;
   
   l_from_fname = p_alloc_fname_fixed_digits(ainfo_ptr->basename, from_nr, ainfo_ptr->extension, from_digits);
   if(l_from_fname == NULL) { return(1); }
   
   l_to_fname = p_alloc_fname_fixed_digits(ainfo_ptr->basename, to_nr, ainfo_ptr->extension, to_digits);
   if(l_to_fname == NULL) { g_free(l_from_fname); return(1); }
   
   if(gap_debug) printf("DEBUG p_rename_frame_digits: %s ..to.. %s\n", l_from_fname, l_to_fname);
   l_rc = rename(l_from_fname, l_to_fname);

   p_gimp_file_rename_thumbnail(l_from_fname, l_to_fname);


   if (from_nr == ainfo_ptr->curr_frame_nr)
   {
      ainfo_ptr->curr_frame_nr = to_nr;
      gimp_image_set_filename(ainfo_ptr->image_id, l_to_fname);
   }
   
   g_free(l_from_fname);
   g_free(l_to_fname);
   
   return(l_rc);
   
}    /* end p_rename_frame_digits */


/* ----------------------------------
 * p_renumber_frames
 * ----------------------------------
 * rename all frames with framenumbers starting at start_frame_nr
 * using frame numbers with at least n digits (leading zeroes)
 * and make sure that resulting framenumbers are continous
 *  example1:  digits == 6, start_frame_nr == 2
 *
 *     Old filenames               New Filenames
 *   -----------------------------------------------
 *     frame_7.xcf                 frame_000002.xcf
 *     frame_8.xcf                 frame_000003.xcf
 *     frame_14.xcf                frame_000004.xcf
 *     frame_16.xcf                frame_000005.xcf
 *
 *  example2:  digits == 4, start_frame_nr == 8
 *
 *     Old filenames               New Filenames
 *   -----------------------------------------------
 *     frame_7.xcf                 frame_0008.xcf
 *     frame_8.xcf                 frame_0009.xcf
 *     frame_14.xcf                frame_0010.xcf
 *     frame_16.xcf                frame_0011.xcf
 *
 * the 2nd example is processed in 2 loops, where the 1.st loop
 * steps down through all frames and makes continous numbers  0017, 0016, 0015, 0014.
 * the 2nd (up stepping) pass makes the final numbers.
 *
 *  example3:  digits == 4, start_frame_nr == 8
 *
 *     Old filenames               New Filenames
 *   -----------------------------------------------
 *     frame_2.xcf                 frame_0008.xcf
 *     frame_3.xcf                 frame_0009.xcf
 *     frame_4.xcf                 frame_0010.xcf
 *     frame_5.xcf                 frame_0011.xcf
 *
 * the 3nd example does not need a 2nd pass, because the 1.st downstepping loop
 * can make the requested numbers.
 */
static gint32
p_renumber_frames(t_anim_info *ainfo_ptr, long start_frame_nr, long digits)
{
  long l_from;
  long l_to;
  long l_has_digits;
  gboolean two_pass;


  if(ainfo_ptr->run_mode == GIMP_RUN_INTERACTIVE)
  {
    gimp_progress_init(_("renumber frames"));
  }
  two_pass = FALSE;
  if(start_frame_nr > ainfo_ptr->first_frame_nr)
  {
    long l_cnt;

    l_from = ainfo_ptr->last_frame_nr;
    l_to = ainfo_ptr->last_frame_nr + (start_frame_nr - ainfo_ptr->first_frame_nr);
    
    l_cnt=0; 
    while (l_cnt < ainfo_ptr->frame_cnt)
    {

      if (gap_debug) printf("p_renumber_frames: DOWNSTEP l_cnt:%d l_from:%d l_to:%d\n", (int)l_cnt, (int)l_to, (int)l_from);
      if( p_exists_frame_nr(ainfo_ptr, l_from, &l_has_digits) )
      {
        if((l_from != l_to)
        || (l_has_digits != digits))
        {
          if (0 != p_rename_frame_digits(ainfo_ptr, l_from, l_to, l_has_digits, digits))
          { 
            return -1;
          }
        }      
        l_to--;
        l_cnt++;
      }
      l_from--;
      if(ainfo_ptr->run_mode == GIMP_RUN_INTERACTIVE)
      {
        gimp_progress_update( (gdouble)(l_cnt)
                            / (gdouble)(ainfo_ptr->frame_cnt) );
      }
    }
    if(start_frame_nr !=  (l_to +1))
    {
      two_pass = TRUE;
      if(ainfo_ptr->run_mode == GIMP_RUN_INTERACTIVE)
      {
        gimp_progress_init(_("renumber frames 2nd pass"));
      }
    }
  }

  if((start_frame_nr <= ainfo_ptr->first_frame_nr)
  || (two_pass))
  {
    l_from = ainfo_ptr->first_frame_nr;
    l_to = start_frame_nr; 
    while (l_to < start_frame_nr + ainfo_ptr->frame_cnt)
    {

      if (gap_debug) printf("p_renumber_frames: UPSTEP l_from:%d l_to:%d\n", (int)l_to, (int)l_from);

      if( p_exists_frame_nr(ainfo_ptr, l_from, &l_has_digits) )
      {
        if((l_from != l_to)
        || (l_has_digits != digits))
        {
          if(0 != p_rename_frame_digits(ainfo_ptr, l_from, l_to, l_has_digits, digits))
          { 
            return -1;
          }
        }      
        l_to++;
      }
      l_from++;
      
      if(ainfo_ptr->run_mode == GIMP_RUN_INTERACTIVE)
      {
        gimp_progress_update( (gdouble)(l_from - ainfo_ptr->first_frame_nr)
                            / (gdouble)(1+ (ainfo_ptr->last_frame_nr - ainfo_ptr->first_frame_nr)) );
      }
    }
  }

  return 0; /* OK */
}  /* end p_renumber_frames */


/* ============================================================================
 * gap_renumber
 * ============================================================================
 */
gint32
gap_renumber(GimpRunMode run_mode, gint32 image_id,
            long start_frame_nr, long digits)
{
  gint32 rc;
  t_anim_info *ainfo_ptr;

  long           l_cnt, l_start_frame_nr, l_digits;

  
  rc = -1;
  l_cnt = 0;
  ainfo_ptr = p_alloc_ainfo(image_id, run_mode);
  if(ainfo_ptr != NULL)
  {
    if (0 == p_dir_ainfo(ainfo_ptr))
    {
      if(run_mode == GIMP_RUN_INTERACTIVE)
      {
         if(0 != p_chk_framechange(ainfo_ptr)) { l_cnt = -1; }
         else
	 {
	   l_cnt = p_renumber_dialog(ainfo_ptr, &l_start_frame_nr, &l_digits);
	 }

         if(0 != p_chk_framechange(ainfo_ptr))
         {
            l_cnt = -1;
         }
                
      }
      else
      {
        l_start_frame_nr = start_frame_nr;
        l_digits   = digits;
      }

      if(gap_debug) printf("gap_renumber: l_cnt:%d l_start_frame_nr:%d l_digits:%d\n", (int)l_cnt, (int)l_start_frame_nr, (int)l_digits);
 
      if(l_cnt >= 0)
      {
         /* rename all frames (on disk) */
         rc = p_renumber_frames(ainfo_ptr, l_start_frame_nr, l_digits);
         if(rc >= 0)
         {
           rc = image_id;  /* if OK, return current image id */
         }
      }

    }
    p_free_ainfo(&ainfo_ptr);
  }
  
  return(rc); 
}	/* end gap_renumber */
