/* gap_story_undo.c
 *  This module handles GAP storyboard undo and redo features.
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
 * version 1.3.25a; 2007/10/18  hof: created
 */

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <gdk/gdkkeysyms.h>

#include "gap_story_main.h"
#include "gap_story_undo_types.h"
#include "gap_story_undo.h"
#include "gap_story_dialog.h"
#include "gap_story_file.h"


extern int gap_debug;  /* 1 == print debug infos , 0 dont print debug infos */

static void             p_free_undo_elem(GapStoryUndoElem    *undo_elem);
static void             p_delete_redo_stack_area(GapStbTabWidgets *tabw);

/* ----------------------------------------------------
 * gap_stb_undo_debug_print_stack
 * ----------------------------------------------------
 */
void
gap_stb_undo_debug_print_stack(GapStbTabWidgets *tabw)
{
  GapStoryUndoElem    *undo_elem;

  if(tabw == NULL)
  {
    printf("\nStack NOT present (NULL POINTER)\n");
    fflush(stdout);
    return;
  }
  printf("\n-------------------------------- Top of STACK ---\n\n");
  printf("gap_stb_undo_debug_print_stack: tabw:%d stack_list:%d stack_ptr:%d group_counter:%.2f\n"
    , (int)tabw
    , (int)tabw->undo_stack_list
    , (int)tabw->undo_stack_ptr
    , (float)tabw->undo_stack_group_counter
    );
  fflush(stdout);

  for(undo_elem = tabw->undo_stack_list; undo_elem != NULL; undo_elem = undo_elem->next)
  {
    printf("  %d %s"
          , (int)undo_elem
          , gap_stb_undo_feature_to_string(undo_elem->feature_id)
          );
    if(undo_elem == tabw->undo_stack_ptr)
    {
      printf(" <-- stack_ptr");
    }
    printf("\n");
    fflush(stdout);
  }

  printf("\n-------------------------------- End of STACK ---\n\n");
  fflush(stdout);

}  /* end gap_stb_undo_debug_print_stack */


/* ----------------------------------------------------
 * gap_stb_undo_feature_to_string
 * ----------------------------------------------------
 */
const char *
gap_stb_undo_feature_to_string(GapStoryFeatureEnum feature_id)
{
  switch(feature_id)
  {
    case GAP_STB_FEATURE_LATEST:                  return("latest"); break;
    case GAP_STB_FEATURE_EDIT_CUT:                return("edit cut"); break;
    case GAP_STB_FEATURE_EDIT_PASTE:              return("edit paste"); break;
    case GAP_STB_FEATURE_DND_CUT:                 return("dnd cut"); break;
    case GAP_STB_FEATURE_DND_PASTE:               return("dnd paste"); break;
    case GAP_STB_FEATURE_CREATE_CLIP:             return("create clip"); break;
    case GAP_STB_FEATURE_CREATE_TRANSITION:       return("create transition"); break;
    case GAP_STB_FEATURE_CREATE_SECTION_CLIP:     return("create section clip"); break;
    case GAP_STB_FEATURE_CREATE_SECTION:          return("create section"); break;
    case GAP_STB_FEATURE_DELETE_SECTION:          return("delete section"); break;
    case GAP_STB_FEATURE_PROPERTIES_CLIP:         return("properties clip"); break;
    case GAP_STB_FEATURE_PROPERTIES_TRANSITION:   return("properties transition"); break;
    case GAP_STB_FEATURE_PROPERTIES_SECTION:      return("properties section"); break;
    case GAP_STB_FEATURE_PROPERTIES_MASTER:       return("properties master"); break;
    case GAP_STB_FEATURE_SCENE_SPLITTING:         return("scene split"); break;
    case GAP_STB_FEATURE_GENERATE_OTONE:          return("generate otone"); break;
  }
  return("unknown");
}  /* end gap_stb_undo_feature_to_string */



/* ---------------------------------------
 * gap_stb_undo_pop
 * ---------------------------------------
 * this procedure is typically called for UNDO storyboard changes
 * made by the storyboard features.
 *
 * undo the feature that is refered by the undo stackpointer
 * move the stackpointer to next element. (keep the element
 * on the stack for redo purpose)
 * returns a duplicate of the storyboard backup that is attached to the
 * undo stackpointer (position as it was before this call)
 * this refers to the backup version at the time BEFORE the
 * feature (that now shall be undone) was applied.
 *
 * example1:
 *                                                   latest
 * stack_list --> BBBB <-- stack_ptr before pop      BBBB 
 *                AAAA                               AAAA <--- stack_ptr after pop returns (BBBB)
 * --------------------------------------------------------------------------------
 *
 * if the stack_ptr is on top (e.g. == stack_list root)
 * a duplicate of the current storyboard is pushed as feature_id "latest" onto the stack.
 * this backup reprents the version AFTER feature BBBB of example1 to be re-done
 *
 *           
 * example2:
 *
 * stack_list -->latest                             latest 
 *               DDDD                               DDDD 
 *               CCCC  <-- stack_ptr before pop     CCCC
 *               BBBB                               BBBB <--- stack_ptr after pop returns (CCCC)
 *               AAAA                               AAAA
 * --------------------------------------------------------------------------------
 */
GapStoryBoard *
gap_stb_undo_pop(GapStbTabWidgets *tabw)
{
  GapStoryBoard       *stb;

  stb = NULL;
  if(tabw == NULL)
  {
    return (NULL);
  }

  if (tabw->undo_stack_ptr == NULL)
  {
    return (NULL);
  }

  if (tabw->undo_stack_ptr == tabw->undo_stack_list)
  {
    /* at undo push the lastest available 
     */
    gap_stb_undo_push_clip(tabw, GAP_STB_FEATURE_LATEST, -1);
    
    /* advance stack_ptr 
     * (to compensate the extra push above)
     */
    tabw->undo_stack_ptr = tabw->undo_stack_ptr->next;
  }

  stb = gap_story_duplicate_full(tabw->undo_stack_ptr->stb);
  
  if(gap_debug)
  {
    printf("gap_stb_undo_pop returning feature_id:%d %s grp_count:%.2f next:%d\n"
      ,(int)tabw->undo_stack_ptr->feature_id
      ,gap_stb_undo_feature_to_string(tabw->undo_stack_ptr->feature_id)
      ,(float)tabw->undo_stack_group_counter
      ,(int)tabw->undo_stack_ptr->next
      );
    fflush(stdout);
  }
  
  tabw->undo_stack_ptr = tabw->undo_stack_ptr->next;

  gap_story_dlg_tabw_undo_redo_sensitivity(tabw);
  
  return (stb);
  
}  /* end gap_stb_undo_pop */


/* ---------------------------------------
 * gap_stb_undo_redo
 * ---------------------------------------
 * redo the feature that is above (e.g before) the undo stackpointer
 * returns a duplicate of the storyboard backup that is attached to the
 * element 2 above the undo stackpointer.
 * this refers to the backup version at the time BEFORE the
 * feature (that now shall be undone) was applied.
 *
 *           
 * stack_list -->latest                             latest 
 *               EEEE                               EEEE
 *               DDDD                               DDDD <--- stack_ptr after redo
 *               CCCC  <-- stack_ptr before redo    CCCC        returns (EEEE)
 *               BBBB                               BBBB 
 *               AAAA                               AAAA
 * --------------------------------------------------------------------------------
 */
GapStoryBoard *
gap_stb_undo_redo(GapStbTabWidgets *tabw)
{
  GapStoryBoard       *stb;
  GapStoryUndoElem    *undo_elem;
  GapStoryUndoElem    *prev_elem;
  GapStoryUndoElem    *redo_elem;

  stb = NULL;
  if(tabw == NULL)
  {
    return (NULL);
  }

  prev_elem = NULL;
  redo_elem = NULL;
  for(undo_elem = tabw->undo_stack_list; undo_elem != NULL; undo_elem = undo_elem->next)
  {
    if(undo_elem->next == tabw->undo_stack_ptr)
    {
      redo_elem = prev_elem;
      tabw->undo_stack_ptr = undo_elem;
      break;
    }
    prev_elem = undo_elem;
  }

  if (redo_elem != NULL)
  {
    stb = gap_story_duplicate_full(redo_elem->stb);
    gap_story_dlg_tabw_undo_redo_sensitivity(tabw);

    if(gap_debug)
    {
      printf("gap_stb_undo_redo returning feature_id:%d %s grp_count:%.2f resulting stack_ptr:%d\n"
        ,(int)redo_elem->feature_id
        ,gap_stb_undo_feature_to_string(redo_elem->feature_id)
        ,(float)tabw->undo_stack_group_counter
        ,(int)tabw->undo_stack_ptr
        );
      fflush(stdout);
    }
  }
  
  return (stb);
  
}  /* end gap_stb_undo_redo */


/* ---------------------------------------
 * p_free_undo_elem
 * ---------------------------------------
 */
static void
p_free_undo_elem(GapStoryUndoElem    *undo_elem)
{
  if(undo_elem == NULL)
  {
    return;
  }

  if(gap_debug)
  {
    printf("p_free_undo_elem: %d %s\n"
          , (int)undo_elem
          , gap_stb_undo_feature_to_string(undo_elem->feature_id)
          );
    fflush(stdout);
  }

  
  if(undo_elem->stb)
  {
    gap_story_free_storyboard(&undo_elem->stb);
  }
  g_free(undo_elem);
  
}  /* end p_free_undo_elem */


/* ---------------------------------------
 * p_delete_redo_stack_area
 * ---------------------------------------
 * delete all undo elements beginning from stack_list root
 * up to exclusive stackpointer.
 * if the stackpointer points to an element with feature_id 
 *   GAP_STB_FEATURE_LATEST
 * then delete inclusive this element.
 *
 * after this procedure the stack_list root and stack_ptr
 * point to the same element (or both are NULL)
 */
static void
p_delete_redo_stack_area(GapStbTabWidgets *tabw)
{
  GapStoryUndoElem    *undo_elem;
  GapStoryUndoElem    *next_elem;
  GapStoryUndoElem    *new_root_elem;

  if(tabw == NULL)
  {
    return;
  }

  if(gap_debug)
  {
    printf("p_delete_redo_stack_area\n");
  }

  /* make sure that the stack_ptr does not point to an element
   * with feature_id GAP_STB_FEATURE_LATEST
   * (advance if necessary)
   */
  for(undo_elem = tabw->undo_stack_ptr; undo_elem != NULL; undo_elem = undo_elem->next)
  {
    if(undo_elem->feature_id != GAP_STB_FEATURE_LATEST)
    {
      break;
    }
    tabw->undo_stack_ptr = undo_elem->next;
  }
  
  new_root_elem = NULL;
  next_elem = NULL;
  for(undo_elem = tabw->undo_stack_list; undo_elem != NULL; undo_elem = next_elem)
  {
    if(undo_elem == tabw->undo_stack_ptr)
    {
      new_root_elem = undo_elem;
      break;
    }
    next_elem = undo_elem->next;
    p_free_undo_elem(undo_elem);
  }

  tabw->undo_stack_list = new_root_elem;
  return;
  
}  /* end p_delete_redo_stack_area */


/* ---------------------------------------
 * gap_stb_undo_destroy_undo_stack
 * ---------------------------------------
 * delete all undo elements beginning from stack_list root
 * up to exclusive stackpointer.
 * after this procedure the stack_list root and stack_ptr
 * point to the same element (or both are NULL)
 */
void
gap_stb_undo_destroy_undo_stack(GapStbTabWidgets *tabw)
{
  if(tabw == NULL)
  {
    return;
  }

  if(gap_debug)
  {
    printf("gap_stb_undo_destroy_undo_stack\n");
    fflush(stdout);
  }

  tabw->undo_stack_ptr = NULL;
  p_delete_redo_stack_area(tabw);

  gap_story_dlg_tabw_undo_redo_sensitivity(tabw);
  
}  /* end gap_stb_undo_destroy_undo_stack */


/* ---------------------------------------
 * gap_stb_undo_push_clip
 * ---------------------------------------
 * create a new undo element (according to specified parameters)
 * and place it at top (first) of the und stack.
 * if the stackpointer is NOT equal to the stack_list root,
 * the delete all undo elements from stack_list up to stackpointer.
 *
 * move the stackpointer to next element. (keep the element
 * on the stack for redo purpose)
 *           
 * stack_list -->DDDD                               
 *               CCCC                                   EEEE <--- stack_ptr after push (EEEE)
 *               BBBB  <-- stack_ptr before push(EEEE)  BBBB      (deletes CCCC, DDDD)
 *               AAAA                                   AAAA
 * --------------------------------------------------------------------------------
 */
void
gap_stb_undo_push_clip(GapStbTabWidgets *tabw, GapStoryFeatureEnum feature_id, gint32 story_id)
{
  GapStoryBoard       *stb;
  GapStoryUndoElem    *new_undo_elem;


  if(tabw == NULL)
  {
    return;
  }

  if(gap_debug)
  {
    printf("gap_stb_undo_push_clip feature_id:%d %s grp_count:%.2f\n"
      ,(int)feature_id
      ,gap_stb_undo_feature_to_string(feature_id)
      ,(float)tabw->undo_stack_group_counter
      );
    fflush(stdout);
  }


  if(tabw->undo_stack_group_counter > 1)
  {
    return;
  }

  if(tabw->undo_stack_group_counter == 1)
  {
    tabw->undo_stack_group_counter = 1.5;
  }

  p_delete_redo_stack_area(tabw);

  stb = gap_story_dlg_tabw_get_stb_ptr(tabw);

  if(story_id >= 0)
  {
    switch (feature_id)
    {
      case GAP_STB_FEATURE_PROPERTIES_CLIP:
      case GAP_STB_FEATURE_PROPERTIES_TRANSITION:
        if(tabw->undo_stack_ptr != NULL)
        {
          if((tabw->undo_stack_ptr->feature_id == feature_id)
          && (tabw->undo_stack_ptr->clip_story_id == story_id))
          {
            /* multiple push of the same feature on the same clip object
             * are supressed. this collects multiple property changes
             * in sequence into one single undo step.
             */
            return;
          }
        }
        break;
      default:
        break;
    }
  }

  new_undo_elem = g_new(GapStoryUndoElem, 1);
  new_undo_elem->clip_story_id = story_id;
  new_undo_elem->feature_id = feature_id;
  
  new_undo_elem->next = tabw->undo_stack_list;

  tabw->undo_stack_list = new_undo_elem;
  tabw->undo_stack_ptr = new_undo_elem;

  gap_story_dlg_update_edit_settings(stb, tabw);

  new_undo_elem->stb = gap_story_duplicate_full(stb);

  gap_story_dlg_tabw_undo_redo_sensitivity(tabw);
  
}  /* end gap_stb_undo_push_clip */


/* ---------------------------------------
 * gap_stb_undo_push
 * ---------------------------------------
 */
void
gap_stb_undo_push(GapStbTabWidgets *tabw, GapStoryFeatureEnum feature_id)
{
  gap_stb_undo_push_clip(tabw, feature_id, -1 /* story_id */);
}  /* end gap_stb_undo_push */



/* ---------------------------------------
 * gap_stb_undo_group_begin
 * ---------------------------------------
 * 
 */
void
gap_stb_undo_group_begin(GapStbTabWidgets *tabw)
{
  if(tabw == NULL)
  {
    return;
  }
  tabw->undo_stack_group_counter += 1.0;
}  /* end gap_stb_undo_group_begin */


/* ---------------------------------------
 * gap_stb_undo_group_end
 * ---------------------------------------
 */
void
gap_stb_undo_group_end(GapStbTabWidgets *tabw)
{
  if(tabw == NULL)
  {
    return;
  }
  tabw->undo_stack_group_counter -= 1.0;
  if (tabw->undo_stack_group_counter < 1.0)
  {
    tabw->undo_stack_group_counter = 0.0;
  }
  
}  /* end gap_stb_undo_group_end */


/* ---------------------------------------
 * gap_stb_undo_get_undo_feature
 * ---------------------------------------
 * return NULL if undo not available
 *        or feature name that is ready to be un-done
 *        (dont g_free the returned string)
 */
const char *
gap_stb_undo_get_undo_feature(GapStbTabWidgets *tabw)
{
  const char *feature_name;
  
  if (tabw == NULL)
  {
    return (NULL);
  }

  if (tabw->undo_stack_ptr == NULL)
  {
    return (NULL);
  }

  feature_name = gap_stb_undo_feature_to_string(tabw->undo_stack_ptr->feature_id);
  return (feature_name);

}  /* end gap_stb_undo_get_undo_feature */


/* ---------------------------------------
 * gap_stb_undo_get_redo_feature
 * ---------------------------------------
 * return NULL if redo not available
 *        or feature name that is ready to be re-done
 *        (dont g_free the returned string)
 */
const char *
gap_stb_undo_get_redo_feature(GapStbTabWidgets *tabw)
{
  const char *feature_name;
  GapStoryUndoElem    *undo_elem;
  GapStoryUndoElem    *redo_elem;
  
  if (tabw == NULL)
  {
    return (NULL);
  }

  redo_elem = NULL;
  for(undo_elem = tabw->undo_stack_list; undo_elem != NULL; undo_elem = undo_elem->next)
  {
    if(undo_elem->next == tabw->undo_stack_ptr)
    {
      if(undo_elem->feature_id == GAP_STB_FEATURE_LATEST)
      {
        return (NULL);
      }
      redo_elem = undo_elem;
      break;
    }
  }

  if (redo_elem != NULL)
  {
    feature_name = gap_stb_undo_feature_to_string(redo_elem->feature_id);
    return (feature_name);
  }

  return (NULL);

}  /* end gap_stb_undo_get_redo_feature */

/* ---------------------------------------
 * gap_stb_undo_stack_set_unsaved_changes
 * ---------------------------------------
 * set the unsaved_changes flag = TRUE in
 * all storyboard backups in the undo stack.
 * this is typicall called on save.
 * (because the unsaved_changes in the back ups
 * refers to the state at capture time and gets invalid
 * when the storyboard is saved to file).
 */
void
gap_stb_undo_stack_set_unsaved_changes(GapStbTabWidgets *tabw)
{
  GapStoryUndoElem    *undo_elem;
  for(undo_elem = tabw->undo_stack_list; undo_elem != NULL; undo_elem = undo_elem->next)
  {
    if(undo_elem->stb != NULL)
    {
      undo_elem->stb->unsaved_changes = TRUE;
    }
  }
}  /* end gap_stb_undo_stack_set_unsaved_changes */
