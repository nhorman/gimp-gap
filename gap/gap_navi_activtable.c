/* gap_navi_activtable.c
 * 2002.04.21 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 */

#include "libgimp/gimp.h"
#include "gap_navi_activtable.h"

typedef struct t_gap_activdata {
   gint32      pid;
   gint32      old_image_id;
   gint32      new_image_id;
} t_gap_activdata;


extern      int gap_debug; /* ==0  ... dont print debug infos */

#define GAP_NAVID_ACTIVE_IMAGE "plug_in_gap_navid_ACTIVE_IMAGETABLE"

/* ============================================================================
 * p_update_active_image
 *   update activ_image table.
 *   - is usually called in gap_lib at change of the current frame.
 *   - The activ_image table contains all the active_images for all
 *     open navigator dialogs. (currently 1 or 0 entries)
 * ============================================================================
 */

/* revision history:
 * 1.3.5a; 2002/04/21   hof: created (Handle Event: active_image changed id)
 */
 
void
p_update_active_image(gint32 old_image_id, gint32 new_image_id)
{
  gint32           l_idx;
  gint32           l_activsize;
  gint32           l_nactivs;
  t_gap_activdata  *l_activtab;
  
  /* update activtable (all records that match old_image_id) */
  l_activsize = gimp_get_data_size (GAP_NAVID_ACTIVE_IMAGE);
  if (l_activsize > 0)
  {
    l_nactivs = l_activsize / (sizeof(t_gap_activdata));
    l_activtab = g_new(t_gap_activdata, l_nactivs);
    gimp_get_data (GAP_NAVID_ACTIVE_IMAGE, l_activtab);
    
    for(l_idx=0; l_idx < l_nactivs; l_idx++)
    {
      if(l_activtab[l_idx].old_image_id == old_image_id)
      {
        l_activtab[l_idx].new_image_id = new_image_id;
        gimp_set_data(GAP_NAVID_ACTIVE_IMAGE, l_activtab, l_nactivs * sizeof(t_gap_activdata));
      }
    }
    g_free(l_activtab);
  }
}	/* end p_update_active_image */

/* ============================================================================
 * p_set_active_image
 *   set active image for one (Navigator) Process in the activ_image table.
 * ============================================================================
 */
void
p_set_active_image(gint32 image_id, gint32 pid)
{
  gint32           l_idx;
  gint32           l_activsize;
  gint32           l_nactivs;
  gint32           l_nactivs_old;
  t_gap_activdata  *l_activtab;
  
  /* check activtable for empty records */
  l_activsize = gimp_get_data_size (GAP_NAVID_ACTIVE_IMAGE);
  if (l_activsize > 0)
  {
    l_nactivs_old = l_activsize / (sizeof(t_gap_activdata));
    l_nactivs = l_nactivs_old + 1;
    l_activtab = g_new(t_gap_activdata, l_nactivs);
    gimp_get_data (GAP_NAVID_ACTIVE_IMAGE, l_activtab);
    
    for(l_idx=0; l_idx < l_nactivs_old; l_idx++)
    {
      if (l_activtab[l_idx].pid == pid)
      {
        l_nactivs--;
        break;
      }
    }
  }
  else
  {
    /* create a new tab with 1 entry */
    l_nactivs = 1;
    l_idx = 0;
    l_activtab = g_new(t_gap_activdata, l_nactivs);
  }

  l_activtab[l_idx].old_image_id = image_id;
  l_activtab[l_idx].new_image_id = image_id;
  l_activtab[l_idx].pid = pid;
  
  gimp_set_data(GAP_NAVID_ACTIVE_IMAGE, l_activtab, l_nactivs * sizeof(t_gap_activdata));

  g_free(l_activtab);
}	/* end p_set_active_image */




/* ============================================================================
 * p_get_active_image
 *   get new_image_id for the active image (image_id)
 *    for one (Navigator) Process in the activ_image table.
 *   - If the table has information about a change of the old active image_id,
 *     the new_image_id is returned
 *   - If the old active image_id is found in the table, but still is
 *     the same (as new_image_id) then -1 is returned.
 *   - If the active_image table is empty
 *     the input image_id is retuned.
 * ============================================================================
 */
gint32
p_get_active_image(gint32 image_id, gint32 pid)
{
  gint32           l_idx;
  gint32           l_activsize;
  gint32           l_nactivs;
  gint32           l_new_image_id;
  t_gap_activdata  *l_activtab;
  
  l_new_image_id = image_id;
  
  /* check activtable for empty records */
  l_activsize = gimp_get_data_size (GAP_NAVID_ACTIVE_IMAGE);
  if (l_activsize > 0)
  {
    l_nactivs = l_activsize / (sizeof(t_gap_activdata));
    l_activtab = g_new(t_gap_activdata, l_nactivs);
    gimp_get_data (GAP_NAVID_ACTIVE_IMAGE, l_activtab);
    
    for(l_idx=0; l_idx < l_nactivs; l_idx++)
    {
      if ((l_activtab[l_idx].pid == pid)
      &&  (l_activtab[l_idx].old_image_id == image_id))
      {
        if(l_activtab[l_idx].new_image_id == image_id)
        {
          l_new_image_id = -1;
        }
        else
        {
          l_new_image_id = l_activtab[l_idx].new_image_id;
        }
        break;
      }
    }
    g_free(l_activtab);
  }

  return (l_new_image_id);
  
}	/* end p_get_active_image */

