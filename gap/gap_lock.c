/* gap_lock.c
 * 2002.04.21 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins  LOCKING
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
 * 1.3.12a; 2003/05/01  hof: use gap-intl.h
 * 1.3.5a; 2002/04/21   hof: created (new gap locking procedures with locktable)
 */

#include "config.h"

/* SYSTEM (UNIX) includes */ 
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <signal.h>           /* for kill */
#ifdef HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* GIMP includes */
#include "gtk/gtk.h"
#include "gap-intl.h"
#include "libgimp/gimp.h"

/* GAP includes */
#include "gap_lock.h"

extern      int gap_debug; /* ==0  ... dont print debug infos */

#define GAP_LOCKTABLE_KEY "plug_in_gap_plugins_LOCKTABLE"

typedef struct t_gap_lockdata
{
    gint32   image_id;  /* -1 marks an empty record */
    gint32   pid;
} t_gap_lockdata;

static gint32
p_getpid(void)
{
#ifndef G_OS_WIN32
  /* for UNIX */
  return ((gint32)getpid());
#else
  /* hof: dont know how to getpid on windows */
  return 0;
#endif
}

static gint 
p_pid_is_alive(gint32 pid)
{
#ifndef G_OS_WIN32
  /* for UNIX */

  /* kill  with signal 0 checks only if the process is alive (no signal is sent)
   *       returns 0 if alive, 1 if no process with given pid found.
   */
  if (0 == kill(pid, 0))
  {
    return(TRUE);
  }
  return (FALSE);
#else
  /* hof: dont know how to check on Windows
   *      assume that process is always alive
   *      (therefore on Windows locks will not be cleared 
   *       automatically after crashes of the locking process)
   */
  return(TRUE);
#endif
}


/* ============================================================================
 * p_gap_lock_is_locked
 *   check if image_id is in the LOCKTABLE and if the locking Process is still
 *   alive. return TRUE if both is TRUE
 *   return FALSE if no valid lock is found.
 * ============================================================================
 */

gboolean
p_gap_lock_is_locked(gint32 image_id, GimpRunMode run_mode)
{
  gint32           l_idx;
  gint32           l_locksize;
  gint32           l_nlocks;
  t_gap_lockdata  *l_locktab;
  
  /* check locktable for locks */
  l_locksize = gimp_get_data_size (GAP_LOCKTABLE_KEY);
  if (l_locksize > 0)
  {
    l_nlocks = l_locksize / (sizeof(t_gap_lockdata));
    l_locktab = g_new(t_gap_lockdata, l_nlocks);
    gimp_get_data (GAP_LOCKTABLE_KEY, l_locktab);
    
    for(l_idx=0; l_idx < l_nlocks; l_idx++)
    {
      if(l_locktab[l_idx].image_id == image_id)
      {
         if(p_pid_is_alive(l_locktab[l_idx].pid))
         {
           if(run_mode == GIMP_RUN_INTERACTIVE)
           {
              gchar *l_lockmsg;
           
              l_lockmsg = g_strdup_printf(_("Can't execute more than 1 Video Function\n"
                                            "on the same AnimFrame Image at the same time\n"
                                            "Locking IMAGE_ID:%d\n")
                                         ,  (int)image_id);
              gimp_message(l_lockmsg);
              g_free(l_lockmsg);
           }
           printf("GAP plug-in is LOCKED  IMAGE_ID:%d PID:%d\n", 
                  (int)l_locktab[l_idx].image_id,
                  (int)l_locktab[l_idx].pid);
       
           return(TRUE);
         }
         break;
      }
    }
    g_free(l_locktab);
  }
  return(FALSE);
}	/* end p_gap_lock_is_locked */


/* ============================================================================
 * p_gap_lock_set
 *   add a lock for image_id and current Process to the LOCKTABLE.
 *   (overwrite empty record or add a new record if no empty one is found)
 * ============================================================================
 */

void
p_gap_lock_set(gint32 image_id)
{
  gint32           l_idx;
  gint32           l_locksize;
  gint32           l_nlocks;
  gint32           l_nlocks_old;
  t_gap_lockdata  *l_locktab;
  
  /* check locktable for empty records */
  l_locksize = gimp_get_data_size (GAP_LOCKTABLE_KEY);
  if (l_locksize > 0)
  {
    l_nlocks_old = l_locksize / (sizeof(t_gap_lockdata));
    /* l_nlocks +  one extra record in case there is no empty record in the table
     * for overwrite 
     */
    l_nlocks = l_nlocks_old + 1;
    l_locktab = g_new(t_gap_lockdata, l_nlocks);
    gimp_get_data (GAP_LOCKTABLE_KEY, l_locktab);
    
    for(l_idx=0; l_idx < l_nlocks_old; l_idx++)
    {
      if((l_locktab[l_idx].image_id < 0)
      || (!p_pid_is_alive(l_locktab[l_idx].pid)))
      {
         l_nlocks--;      /* empty record found, dont need the extra record */
         break; 
      }
    }
  }
  else
  {
    /* create a new locktab with 1 entry */
    l_nlocks = 1;
    l_idx = 0;
    l_locktab = g_new(t_gap_lockdata, l_nlocks);
  }

  l_locktab[l_idx].image_id = image_id;
  l_locktab[l_idx].pid      = p_getpid();

  gimp_set_data(GAP_LOCKTABLE_KEY, l_locktab, l_nlocks * sizeof(t_gap_lockdata));

  if (gap_debug)
  {
    for(l_idx=0; l_idx < l_nlocks; l_idx++)
    {
      printf("p_gap_lock_set: LOCKTAB[%d] IMAGE_ID:%d PID:%d\n",
              (int)l_idx,
              (int)l_locktab[l_idx].image_id,
              (int)l_locktab[l_idx].pid );
    }
  }
  g_free(l_locktab);
  
}	/* end p_gap_lock_set */


/* ============================================================================
 * p_gap_lock_remove
 *   remove lock for image_id from the LOCKTABLE.
 *   (by setting the corresponding record empty)
 * ============================================================================
 */
 
void
p_gap_lock_remove(gint32 image_id)
{
  gint32           l_idx;
  gint32           l_locksize;
  gint32           l_nlocks;
  t_gap_lockdata  *l_locktab;
  
  /* check locktable for empty records */
  l_locksize = gimp_get_data_size (GAP_LOCKTABLE_KEY);
  if (l_locksize > 0)
  {
    l_nlocks = l_locksize / (sizeof(t_gap_lockdata));
    l_locktab = g_new(t_gap_lockdata, l_nlocks);
    gimp_get_data (GAP_LOCKTABLE_KEY, l_locktab);
    
    for(l_idx=0; l_idx < l_nlocks; l_idx++)
    {
      if(l_locktab[l_idx].image_id == image_id)
      {
         /* mark this record as empty (image_id = -1) */
        l_locktab[l_idx].image_id = -1;
        l_locktab[l_idx].pid      = 0;         
        gimp_set_data(GAP_LOCKTABLE_KEY, l_locktab, l_nlocks * sizeof(t_gap_lockdata));
        break; 
      }
    }
    g_free(l_locktab);
  }
}	/* end p_gap_lock_remove */
