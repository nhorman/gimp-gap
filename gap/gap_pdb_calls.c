/* gap_pdb_calls.c
 *
 * this module contains wraper calls of procedures in the GIMPs Procedural Database
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
 * version 1.3.14c; 2003/06/15  hof: take care of gimp_image_thumbnail 128x128 sizelimit
 * version 1.3.14b; 2003/06/03  hof: gboolean retcode for thumbnail procedures
 * version 1.3.14a; 2003/05/24  hof: moved vin Procedures to gap_vin module
 * version 1.3.5a;  2002/04/20  hof: p_gimp_layer_new_from_drawable. (removed set_drabale)
 * version 1.3.4a;  2002/03/12  hof: removed duplicate wrappers that are available in libgimp too.
 * version 1.2.2b;  2001/12/09  hof: wrappers for tattoo procedures
 * version 1.1.16a; 2000/02/05  hof: path lockedstaus
 * version 1.1.15b; 2000/01/30  hof: image parasites
 * version 1.1.15a; 2000/01/26  hof: pathes
 *                                   removed old gimp 1.0.x PDB Interfaces
 * version 1.1.14a; 2000/01/09  hof: thumbnail save/load,
 *                              Procedures for video_info file
 * version 0.98.00; 1998/11/28  hof: 1.st (pre) release (GAP port to GIMP 1.1)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* GIMP includes */
#include "libgimp/gimp.h"

/* GAP includes */
#include "gap_pdb_calls.h"

extern int gap_debug;

/* ============================================================================
 * p_pdb_procedure_available
 *   if requested procedure is available in the PDB return the number of args
 *      (0 upto n) that are needed to call the procedure.
 *   if not available return -1
 * ============================================================================
 */

gint 
p_pdb_procedure_available(char *proc_name)
{
   /* Note: It would be nice to call "gimp_layer_get_linked" direct,
    *       but there is not such an Interface in gimp 0.99.16
    * Workaround:
    *   I did a patch to implement the "gimp_layer_get_linked"
    *   procedure, and call it via PDB call if available.
    *   if not available FALSE is returned.
    */
    
  gint             l_nparams;
  gint             l_nreturn_vals;
  GimpPDBProcType   l_proc_type;
  gchar            *l_proc_blurb;
  gchar            *l_proc_help;
  gchar            *l_proc_author;
  gchar            *l_proc_copyright;
  gchar            *l_proc_date;
  GimpParamDef    *l_params;
  GimpParamDef    *l_return_vals;
  gint             l_rc;

  l_rc = 0;
  
  /* Query the gimp application's procedural database
   *  regarding a particular procedure.
   */
  if (gimp_procedural_db_proc_info (proc_name,
				    &l_proc_blurb,
				    &l_proc_help,
				    &l_proc_author,
				    &l_proc_copyright,
				    &l_proc_date,
				    &l_proc_type,
				    &l_nparams,
				    &l_nreturn_vals,
				    &l_params,
				    &l_return_vals))
    {
      /* procedure found in PDB */
      return (l_nparams);
    }

  printf("Warning: Procedure %s not found.\n", proc_name);
  return -1;
}	/* end p_pdb_procedure_available */

/* ---------------------- PDB procedure calls  -------------------------- */






/* ============================================================================
 * p_gimp_rotate_degree
 *  PDB call of 'gimp_rotate'
 * ============================================================================
 */

gint32
p_gimp_rotate_degree(gint32 drawable_id, gboolean interpolation, gdouble angle_deg)
{
   gdouble          l_angle_rad;

   l_angle_rad = (angle_deg * 3.14159) / 180.0;
   return(gimp_rotate(drawable_id, interpolation, l_angle_rad));
   
}  /* end p_gimp_rotate_degree */



/* ============================================================================
 * p_gimp_displays_reconnect
 *   
 * ============================================================================
 */

gboolean
p_gimp_displays_reconnect(gint32 old_image_id, gint32 new_image_id)
{
   static char     *l_called_proc = "gimp_displays_reconnect";
   GimpParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_called_proc,
                                 &nreturn_vals,
                                 GIMP_PDB_IMAGE,  old_image_id,
                                 GIMP_PDB_IMAGE,  new_image_id,
                                 GIMP_PDB_END);

   if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
   {
      return (TRUE);   /* OK */
   }
   printf("GAP: Error: PDB call of %s failed\n", l_called_proc);
   return(FALSE);
}	/* end p_gimp_displays_reconnect */



/* ============================================================================
 * p_gimp_layer_new_from_drawable
 *   
 * ============================================================================
 */

gint32
p_gimp_layer_new_from_drawable(gint32 drawable_id, gint32 dst_image_id)
{
   static char     *l_called_proc = "gimp_layer_new_from_drawable";
   GimpParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_called_proc,
                                 &nreturn_vals,
                                 GIMP_PDB_DRAWABLE,  drawable_id,
                                 GIMP_PDB_IMAGE,     dst_image_id,
                                 GIMP_PDB_END);

   if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
   {
      return (return_vals[1].data.d_int32);   /* return the resulting layer_id */
   }
   printf("GAP: Error: PDB call of %s failed\n", l_called_proc);
   return(-1);
}	/* end p_gimp_layer_new_from_drawable */

/* ============================================================================
 * p_gimp_file_save_thumbnail
 *   
 * ============================================================================
 */

gboolean
p_gimp_file_save_thumbnail(gint32 image_id, char* filename)
{
   static char     *l_called_proc = "gimp_file_save_thumbnail";
   GimpParam          *return_vals;
   int              nreturn_vals;

   /*if(gap_debug) printf("p_gimp_file_save_thumbnail: image_id:%d  %s\n", (int)image_id, filename); */

   return_vals = gimp_run_procedure (l_called_proc,
                                 &nreturn_vals,
                                 GIMP_PDB_IMAGE,     image_id,
				 GIMP_PDB_STRING,    filename,
                                 GIMP_PDB_END);

   if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
   {
      return (TRUE);
   }
   printf("GAP: Error: PDB call of %s failed\n", l_called_proc);
   return(FALSE);
}	/* end p_gimp_file_save_thumbnail */

/* ============================================================================
 * p_gimp_file_load_thumbnail
 *   
 * ============================================================================
 */

gboolean
p_gimp_file_load_thumbnail(char* filename, gint32 *th_width, gint32 *th_height,
                           gint32 *th_data_count,  unsigned char **th_data)
{
   static char     *l_called_proc = "gimp_file_load_thumbnail";
   GimpParam          *return_vals;
   int              nreturn_vals;

   if(gap_debug) printf("p_gimp_file_load_thumbnail:  %s\n", filename);

   *th_data = NULL;
   return_vals = gimp_run_procedure (l_called_proc,
                                 &nreturn_vals,
 				 GIMP_PDB_STRING,    filename,
                                 GIMP_PDB_END);

   if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
   {
      *th_width = return_vals[1].data.d_int32;
      *th_height = return_vals[2].data.d_int32;
      *th_data_count = return_vals[3].data.d_int32;
      *th_data = (unsigned char *)return_vals[4].data.d_int8array;
      return (TRUE); /* OK */
   }
   if(gap_debug) printf("GAP: Error: PDB call of %s failed\n", l_called_proc);
   return(FALSE);
}	/* end p_gimp_file_load_thumbnail */



gboolean
p_gimp_image_thumbnail(gint32 image_id, gint32 width, gint32 height,
                              gint32 *th_width, gint32 *th_height, gint32 *th_bpp,
			      gint32 *th_data_count, unsigned char **th_data)
{
   static char     *l_called_proc = "gimp_image_thumbnail";
   GimpParam          *return_vals;
   int              nreturn_vals;

   *th_data = NULL;

    /* gimp_image_thumbnail 
     *   has a limit of maximal 128x128 pixels. (in gimp-1.3.14)
     *   On bigger sizes it returns success, along with a th_data == NULL.
     * THIS workaround makes a 2nd try with reduced size.
     * TODO:
     *  - if gimp keeps the size limit until the stable 1.4 release
     *       i suggest to check for sizs < 128 before the 1.st attemp.
     *       (for better performance on bigger thumbnail sizes)
     *  - if there will be no size limit in the future,
     *       the 2.nd try cold be removed (just for cleanup reasons)
     * hof, 2003.06.17
     */
workaround:
   return_vals = gimp_run_procedure (l_called_proc,
                                 &nreturn_vals,
 				 GIMP_PDB_IMAGE,    image_id,
 				 GIMP_PDB_INT32,    width,
 				 GIMP_PDB_INT32,    height,
                                 GIMP_PDB_END);

   if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
   {
      *th_width  = return_vals[1].data.d_int32;
      *th_height = return_vals[2].data.d_int32;
      *th_bpp    = return_vals[3].data.d_int32;
      *th_data_count = return_vals[4].data.d_int32;
      *th_data = (unsigned char *)return_vals[5].data.d_int8array;

      if (*th_data == NULL)
      {
         if(gap_debug)
         {
           printf("(PDB_WRAPPER workaround for gimp_image_thumbnail GIMP_PDB_SUCCESS, th_data:%d  (%d x %d) \n"
            , (int)return_vals[5].data.d_int8array
            , (int)return_vals[1].data.d_int32
            , (int)return_vals[2].data.d_int32
            );
         }
         if(MAX(width, height) > 128)
         {
           if(width > height)
           {
              height = (128 * height) / width;
              width = 128;
           }
           else
           {
              width = (128 * width) / height;
              height = 128;
           }

           goto workaround;
         }
         return(FALSE);  /* this is no success */
      }
      return(TRUE); /* OK */
   }
   printf("GAP: Error: PDB call of %s failed\n", l_called_proc);
   return(FALSE);
}	/* end p_gimp_image_thumbnail */

