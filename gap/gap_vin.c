/*  gap_vin.c
 *
 *  This module handles GAP video info files (_vin.gap)
 *  _vin.gap files store global informations about an animation
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
 * version 1.3.14a; 2003/05/24  hof: created (splitted off from gap_pdb_calls module)
 *                              write now keeps unkown keyword lines in _vin.gap files
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

/* GIMP includes */
#include "libgimp/gimp.h"

/* GAP includes */
#include "gap_vin.h"
extern int gap_debug;



/* --------------------------
 * p_alloc_video_info_name
 * --------------------------
 * to get the name of the the video_info_file
 * (the caller should g_free the returned name after use)
 */
char *
p_alloc_video_info_name(char *basename)
{
  char *l_str;

  if(basename == NULL)
  {
    return(NULL);
  }

  l_str = g_strdup_printf("%svin.gap", basename);
  return(l_str);
}  /* end p_alloc_video_info_name */


/* --------------------------
 * p_free_textfile_lines
 * --------------------------
 */
void
p_free_textfile_lines(t_textfile_lines *txf_ptr_root)
{
  t_textfile_lines *txf_ptr;
  t_textfile_lines *txf_ptr_next;


  txf_ptr_next = NULL;
  for(txf_ptr = txf_ptr_root; txf_ptr != NULL; txf_ptr = txf_ptr_next)
  {
     txf_ptr_next = (t_textfile_lines *) txf_ptr->next;
     g_free(txf_ptr->line);
     g_free(txf_ptr);
  }
}  /* end p_free_textfile_lines */


/* --------------------------
 * p_load_textfile
 * --------------------------
 * load all lines from a textfile
 * into a t_textfile_lines list structure 
 * and return root pointer of this list.
 * return NULL if file not found or empty.
 */
t_textfile_lines *
p_load_textfile(char *filename)
{
  FILE *l_fp;
  t_textfile_lines *txf_ptr;
  t_textfile_lines *txf_ptr_prev;
  t_textfile_lines *txf_ptr_root;
  char         l_buf[4000];
  int   l_len;
  int   line_nr;
  
  line_nr = 0;
  txf_ptr_prev = NULL;
  txf_ptr_root = NULL;
  l_fp = fopen(filename, "r");
  if(l_fp)
  {
    while(NULL != fgets(l_buf, 4000-1, l_fp))
    {
      line_nr++;
      l_len = strlen("(framerate ");
      txf_ptr = g_malloc0(sizeof(t_textfile_lines));
      txf_ptr->line = g_strdup(l_buf);
      txf_ptr->line_nr++;
      txf_ptr->next = NULL;
      
      if(txf_ptr_prev == NULL)
      {
        txf_ptr_root = txf_ptr;
      }
      else
      {
        txf_ptr_prev->next = txf_ptr;
      }
      txf_ptr_prev = txf_ptr;
    }
    fclose(l_fp);
  }

  return(txf_ptr_root);
}  /* end p_load_textfile */


/* --------------------------
 * p_set_video_info
 * --------------------------
 * (re)write the current video info to .vin file
 */
int
p_set_video_info(t_video_info *vin_ptr, char *basename)
{
  FILE *l_fp;
  t_textfile_lines *txf_ptr_root;
  t_textfile_lines *txf_ptr;
  char  *l_vin_filename;
  int   l_rc;
  int   l_cnt_framerate;
  int   l_cnt_timezoom;
  int   l_len;
   
  l_rc = -1;
  l_cnt_framerate = 0;
  l_cnt_timezoom = 0;
  l_vin_filename = p_alloc_video_info_name(basename);

  if(l_vin_filename)
  {
      /* use "." as decimalpoint (in the fprintf statements) */
      /* setlocale (LC_NUMERIC, "C"); */
      
      
      txf_ptr_root = p_load_textfile(l_vin_filename);
  
      l_fp = fopen(l_vin_filename, "w");
      if(l_fp)
      {
         for(txf_ptr = txf_ptr_root; txf_ptr != NULL; txf_ptr = (t_textfile_lines *) txf_ptr->next)
         {
            l_len = strlen("(framerate ");
            if(strncmp(txf_ptr->line, "(framerate ", l_len) == 0)
            {
                fprintf(l_fp, "(framerate %f) # 1.0 upto 100.0 frames per sec\n"
                       , (float)vin_ptr->framerate);
                l_cnt_framerate++;
            }
            else
            {
              l_len = strlen("(timezoom ");
              if (strncmp(txf_ptr->line, "(timezoom ", l_len) == 0)
              {
                 fprintf(l_fp, "(timezoom %d)  # 1 upto 100 frames\n"
                        , (int)vin_ptr->timezoom );
                 l_cnt_timezoom++;
              }
              else
              {
                /* copy unknow lines 1:1
                 * (and add newline if required)
                 * For compatibility with future keywords
                 */
                l_len = strlen(txf_ptr->line);
                if(txf_ptr->line[l_len-1] == '\n')
                {
                   fprintf(l_fp, "%s", txf_ptr->line);
                }
                else
                {
                   fprintf(l_fp, "%s\n", txf_ptr->line);
                }
              }
            }
         }
         
         if(txf_ptr_root)
         {
           p_free_textfile_lines(txf_ptr_root);
         }
         else
         {
           /* write header if file was empty or not existent */
           fprintf(l_fp, "# GIMP / GAP Videoinfo file\n");
         }
         
         if(l_cnt_framerate == 0)
         {
           fprintf(l_fp, "(framerate %f) # 1.0 upto 100.0 frames per sec\n", (float)vin_ptr->framerate);
         }
         if(l_cnt_timezoom == 0) 
         {
           fprintf(l_fp, "(timezoom %d)  # 1 upto 100 frames\n", (int)vin_ptr->timezoom );
         }

         fclose(l_fp);
         l_rc = 0;
       }
       g_free(l_vin_filename);
       
  }

  return(l_rc);
}  /* end p_set_video_info */


/* --------------------------
 * p_get_video_info
 * --------------------------
 * get video info from .vin file
 */
t_video_info *
p_get_video_info(char *basename)
{
  FILE *l_fp;
  char  *l_vin_filename;
  t_textfile_lines *txf_ptr_root;
  t_textfile_lines *txf_ptr;
  t_video_info *l_vin_ptr;
  char         l_buf[4000];
  int   l_len;
  
  l_vin_ptr = g_malloc(sizeof(t_video_info));
  l_vin_ptr->timezoom = 1;
  l_vin_ptr->framerate = 24.0;
  
  l_vin_filename = p_alloc_video_info_name(basename);
  if(l_vin_filename)
  {
      /* use "." as decimalpoint (in the atof statements) */
      /* setlocale (LC_NUMERIC, "C"); */
      
      txf_ptr_root = p_load_textfile(l_vin_filename);

      for(txf_ptr = txf_ptr_root; txf_ptr != NULL; txf_ptr = (t_textfile_lines *) txf_ptr->next)
      {
          /* printf("p_get_video_info:%s\n", txf_ptr->line); */

          l_len = strlen("(framerate ");
          if(strncmp(txf_ptr->line, "(framerate ", l_len) == 0)
          {
             l_vin_ptr->framerate = atof(&txf_ptr->line[l_len]);
          }
          
          l_len = strlen("(timezoom ");
          if (strncmp(txf_ptr->line, "(timezoom ", l_len ) == 0)
          {
             l_vin_ptr->timezoom = atol(&txf_ptr->line[l_len]);
          }

      }
      if(txf_ptr_root)
      {
        p_free_textfile_lines(txf_ptr_root);
      }

      g_free(l_vin_filename);
  }
  return(l_vin_ptr);
}  /* end p_get_video_info */

