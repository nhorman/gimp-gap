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
 * version 2.1.0a;  2004/06/03  hof: added onionskin setting ref_mode
 * version 1.3.25b; 2004/01/23  hof: bugfix: gap_vin_load_textfile set correct line_nr
 * version 1.3.18b; 2003/08/23  hof: gap_vin_get_all: force timezoom value >= 1 (0 results in divison by zero)
 * version 1.3.18a; 2003/08/23  hof: bugfix: gap_vin_get_all must clear (g_malloc0) vin_ptr struct at creation
 * version 1.3.16c; 2003/07/12  hof: support onionskin settings in video_info files
 *                                   key/value/datatype is now managed by t_keylist
 * version 1.3.14b; 2003/06/03  hof: using setlocale independent float conversion procedures
 *                                   g_ascii_strtod() and g_ascii_dtostr()
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


typedef enum
{
  GAP_VIN_GINT32
, GAP_VIN_GDOUBLE
, GAP_VIN_STRING
, GAP_VIN_GBOOLEAN
, GAP_VIN_DUMMY
} t_vin_datatype;


typedef struct t_keylist
{
  char            keyword[50];
  char            comment[80];
  gint32          len;
  gpointer        val_ptr;
  t_vin_datatype  dataype;
  gboolean        done_flag;
  void *next;
} t_keylist;


/* --------------------------
 * p_free_keylist
 * --------------------------
 */
static void
p_free_keylist(t_keylist *keylist)
{
  t_keylist *keyptr;
  
  while(keylist)
  {
    keyptr = (t_keylist *)keylist->next;
    g_free(keylist);
    keylist = keyptr;
  }

}  /* end p_free_keylist */


/* --------------------------
 * p_new_keylist
 * --------------------------
 */
static t_keylist*
p_new_keylist(void)
{
  t_keylist *keyptr;
  
  keyptr = g_malloc0(sizeof(t_keylist));
  keyptr->keyword[0] = '\0';
  keyptr->comment[0] = '\0';
  keyptr->len = 0;
  keyptr->val_ptr = NULL;
  keyptr->dataype = GAP_VIN_DUMMY;
  keyptr->done_flag = FALSE;
  keyptr->next = NULL;
  
  return(keyptr);

}  /* end p_new_keylist */


/* --------------------------
 * p_set_keyword
 * --------------------------
 * create a new element, init with the passed args
 * and add to end of the passed keylist.
 *
 * keylist must contain at least one element.
 * (if this is a dummy it will be replaced)
 */
static void
p_set_keyword(t_keylist *keylist
             , const gchar *keyword
             , gpointer val_ptr
             , t_vin_datatype dataype
             , gint32 len
             , const gchar *comment
             )
{
  t_keylist *keyptr;
  
  if(keylist == NULL)
  {
    printf ("** INTERNAL ERROR p_set_keyword was called with keylist == NULL\n");
    return;
  }
  
  if(keyword == NULL)
  {
    printf ("** INTERNAL ERROR p_set_keyword was called with keyword == NULL\n");
    return;
  }

  if(keylist->dataype == GAP_VIN_DUMMY)
  {
    /* if the 1.st element is a dummy
     * we replace the content rather than creating a new element
     */
    keyptr = keylist;
  }
  else
  {
    keyptr = p_new_keylist();
    
    /* add new element to end of keylist */  
    while(keylist->next)
    {
      keylist = (t_keylist *)keylist->next;
    }
    keylist->next = keyptr;
  }
  
  /* copy the passed args into keyptr element */
  g_snprintf(&keyptr->keyword[0], sizeof(keyptr->keyword), "%s", keyword);
  if(comment)
  {
    g_snprintf(&keyptr->comment[0], sizeof(keyptr->comment), "%s", comment);
  }
  keyptr->len = len;
  keyptr->val_ptr = val_ptr;
  keyptr->dataype = dataype;
  
}  /* end p_set_keyword */


/* --------------------------
 * p_set_master_keywords
 * --------------------------
 */
static void
p_set_master_keywords(t_keylist *keylist, GapVinVideoInfo *vin_ptr)
{
   p_set_keyword(keylist, "(framerate ", &vin_ptr->framerate, GAP_VIN_GDOUBLE, 0, "# 1.0 upto 100.0 frames per sec");
   p_set_keyword(keylist, "(timezoom ", &vin_ptr->timezoom,   GAP_VIN_GINT32, 0, "# 1 upto 100 frames");
}  /* end p_set_master_keywords */

/* --------------------------
 * p_set_onion_keywords
 * --------------------------
 */
static void
p_set_onion_keywords(t_keylist *keylist, GapVinVideoInfo *vin_ptr)
{
   p_set_keyword(keylist, "(onion_auto_enable ", &vin_ptr->onionskin_auto_enable, GAP_VIN_GBOOLEAN, 0, "\0");
   p_set_keyword(keylist, "(onion_auto_replace_after_load ", &vin_ptr->auto_replace_after_load, GAP_VIN_GBOOLEAN, 0, "\0");
   p_set_keyword(keylist, "(onion_auto_delete_before_save ", &vin_ptr->auto_delete_before_save, GAP_VIN_GBOOLEAN, 0, "\0");
   p_set_keyword(keylist, "(onion_number_of_layers ", &vin_ptr->num_olayers, GAP_VIN_GINT32, 0, "\0");
   p_set_keyword(keylist, "(onion_ref_mode ", &vin_ptr->ref_mode, GAP_VIN_GINT32, 0, "\0");
   p_set_keyword(keylist, "(onion_ref_delta ", &vin_ptr->ref_delta, GAP_VIN_GINT32, 0, "\0");
   p_set_keyword(keylist, "(onion_ref_cycle ", &vin_ptr->ref_cycle, GAP_VIN_GBOOLEAN, 0, "\0");
   p_set_keyword(keylist, "(onion_stack_pos ", &vin_ptr->stack_pos, GAP_VIN_GINT32, 0, "\0");
   p_set_keyword(keylist, "(onion_stack_top ", &vin_ptr->stack_top, GAP_VIN_GBOOLEAN, 0, "\0");
   p_set_keyword(keylist, "(onion_opacity_initial ", &vin_ptr->opacity, GAP_VIN_GDOUBLE, 0, "\0");
   p_set_keyword(keylist, "(onion_opacity_delta ", &vin_ptr->opacity_delta, GAP_VIN_GDOUBLE, 0, "\0");
   p_set_keyword(keylist, "(onion_ignore_botlayers ", &vin_ptr->ignore_botlayers, GAP_VIN_GINT32, 0, "\0");
   p_set_keyword(keylist, "(onion_select_mode ", &vin_ptr->select_mode, GAP_VIN_GINT32, 0, "\0");
   p_set_keyword(keylist, "(onion_select_casesensitive ", &vin_ptr->select_case, GAP_VIN_GBOOLEAN, 0, "\0");
   p_set_keyword(keylist, "(onion_select_invert ", &vin_ptr->select_invert, GAP_VIN_GBOOLEAN, 0, "\0");
   p_set_keyword(keylist, "(onion_select_string ", &vin_ptr->select_string[0], GAP_VIN_STRING, sizeof(vin_ptr->select_string), "\0");
   p_set_keyword(keylist, "(onion_ascending_opacity ", &vin_ptr->asc_opacity, GAP_VIN_GBOOLEAN, 0, "\0");
}  /* end p_set_onion_keywords */


/* --------------------------
 * gap_vin_alloc_name
 * --------------------------
 * to get the name of the the video_info_file
 * (the caller should g_free the returned name after use)
 */
char *
gap_vin_alloc_name(char *basename)
{
  char *l_str;

  if(basename == NULL)
  {
    return(NULL);
  }

  l_str = g_strdup_printf("%svin.gap", basename);
  return(l_str);
}  /* end gap_vin_alloc_name */


/* ---------------------------
 * gap_vin_free_textfile_lines
 * ---------------------------
 */
void
gap_vin_free_textfile_lines(GapVinTextFileLines *txf_ptr_root)
{
  GapVinTextFileLines *txf_ptr;
  GapVinTextFileLines *txf_ptr_next;


  txf_ptr_next = NULL;
  for(txf_ptr = txf_ptr_root; txf_ptr != NULL; txf_ptr = txf_ptr_next)
  {
     txf_ptr_next = (GapVinTextFileLines *) txf_ptr->next;
     g_free(txf_ptr->line);
     g_free(txf_ptr);
  }
}  /* end gap_vin_free_textfile_lines */


/* --------------------------
 * gap_vin_load_textfile
 * --------------------------
 * load all lines from a textfile
 * into a GapVinTextFileLines list structure 
 * and return root pointer of this list.
 * return NULL if file not found or empty.
 */
GapVinTextFileLines *
gap_vin_load_textfile(const char *filename)
{
  FILE *l_fp;
  GapVinTextFileLines *txf_ptr;
  GapVinTextFileLines *txf_ptr_prev;
  GapVinTextFileLines *txf_ptr_root;
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
      txf_ptr = g_malloc0(sizeof(GapVinTextFileLines));
      txf_ptr->line = g_strdup(l_buf);
      txf_ptr->line_nr=line_nr;
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
}  /* end gap_vin_load_textfile */



/* --------------------------
 * p_write_keylist_value
 * --------------------------
 * write Key/value entry (one line) to file,
 * value format depends on datatype
 *   (keyname value) #comment
 */
static void
p_write_keylist_value(FILE *fp, t_keylist *keyptr)
{
  switch(keyptr->dataype)
  {
    case GAP_VIN_GINT32:
      {
        gint32 *val_ptr;
      
        val_ptr = (gint32 *)keyptr->val_ptr;
        fprintf(fp, "%s%d) %s\n"
               , keyptr->keyword   /* "(keyword " */
               , (int)*val_ptr     /* value */
               , keyptr->comment   /*  "# 1.0 upto 100.0 frames per sec\n" */
               );
      }
      break;
    case GAP_VIN_GDOUBLE:
      {
        gdouble *val_ptr;
        gchar l_dbl_str[G_ASCII_DTOSTR_BUF_SIZE];
      
        val_ptr = (gdouble *)keyptr->val_ptr;
        /* setlocale independent float string */
        g_ascii_dtostr(&l_dbl_str[0]
                     ,G_ASCII_DTOSTR_BUF_SIZE
                     ,*val_ptr
                     );
     
        fprintf(fp, "%s%s) %s\n"
               , keyptr->keyword   /* "(keyword " */
               , l_dbl_str         /* value */
               , keyptr->comment   /*  "# 1.0 upto 100.0 frames per sec\n" */
               );
      }
      break;
    case GAP_VIN_GBOOLEAN:
      {
        gboolean *val_ptr;

        val_ptr = (gboolean *)keyptr->val_ptr;
        if(*val_ptr)
        {
          fprintf(fp, "%syes) %s\n"
               , keyptr->keyword   /* "(keyword " */
               , keyptr->comment   /*  "# 1.0 upto 100.0 frames per sec\n" */
               );
        }
        else
        {
          fprintf(fp, "%sno) %s\n"
               , keyptr->keyword   /* "(keyword " */
               , keyptr->comment   /*  "# 1.0 upto 100.0 frames per sec\n" */
               );
        }
      }
      break;
    case GAP_VIN_STRING:
      {
        gchar  *val_ptr;
        gint   idx;

        val_ptr = (gchar *)keyptr->val_ptr;
        fprintf(fp, "%s \""
               , keyptr->keyword   /* "(keyword " */
               );

        for(idx=0; idx < keyptr->len; idx++)
        {
          if((val_ptr[idx] == '\0')
          || (val_ptr[idx] == '\n'))
          {
            break;
          }
          if((val_ptr[idx] == '\\')
          || (val_ptr[idx] == '"'))
          {
            fprintf(fp, "\\%c", val_ptr[idx]);
          }
          else
          {
            fprintf(fp, "%c", val_ptr[idx]);
          }
        }
               
        fprintf(fp, "\") %s\n"
               , keyptr->comment   /*  "# 1.0 upto 100.0 frames per sec\n" */
               );
        
      }
      break;
    default:
      break;
  }  /* end switch */

}  /* end p_write_keylist_value */



/* --------------------------
 * gap_vin_set_common_keylist
 * --------------------------
 * (re)write the current video info to .vin file
 * only the values for the keywords in the passed keylist
 * are created (or replaced) in the file.
 * all other lines are left unchanged.
 * if the file is empty a header is added.
 */
static int
gap_vin_set_common_keylist(t_keylist *keylist, GapVinVideoInfo *vin_ptr, char *basename)
{
  FILE *l_fp;
  GapVinTextFileLines *txf_ptr_root;
  GapVinTextFileLines *txf_ptr;
  t_keylist *keyptr;
  char  *l_vin_filename;
  int   l_rc;
  int   l_cnt_framerate;
  int   l_cnt_timezoom;
  int   l_len;
   
  l_rc = -1;
  l_cnt_framerate = 0;
  l_cnt_timezoom = 0;
  l_vin_filename = gap_vin_alloc_name(basename);

  if(l_vin_filename)
  {
      txf_ptr_root = gap_vin_load_textfile(l_vin_filename);
  
      l_fp = fopen(l_vin_filename, "w");
      if(l_fp)
      {
         for(txf_ptr = txf_ptr_root; txf_ptr != NULL; txf_ptr = (GapVinTextFileLines *) txf_ptr->next)
         {
           gboolean line_done;
           
           line_done = FALSE;
           for(keyptr=keylist; keyptr != NULL; keyptr = (t_keylist*)keyptr->next)
           {
             l_len = strlen(keyptr->keyword);
             if(strncmp(txf_ptr->line, keyptr->keyword, l_len) == 0)
             {
               /* replace the existing line (same key, new value) */
               p_write_keylist_value(l_fp, keyptr);
               keyptr->done_flag = TRUE;
               line_done = TRUE;
               break;
             }
           }   /* end for keylist loop */
           
           if(line_done == FALSE)
           {
              /* 1:1 copy of lines with unhandled keywords
               * (and add newline if required)
               * for keeping comment lines
               * and for compatibility with future keywords
               */
              l_len = strlen(txf_ptr->line);
              if(txf_ptr->line[MAX(0,l_len-1)] == '\n')
              {
                 fprintf(l_fp, "%s", txf_ptr->line);
              }
              else
              {
                 fprintf(l_fp, "%s\n", txf_ptr->line);
              }
           }
           
         }  /* end loop for each textline */
         
         if(txf_ptr_root)
         {
           gap_vin_free_textfile_lines(txf_ptr_root);
         }
         else
         {
           /* write header if file was empty or not existent */
           fprintf(l_fp, "# GIMP / GAP Videoinfo file\n");
         }
         
         /* write the unhandled key/values (where key was not found in the file before) */
         for(keyptr=keylist; keyptr != NULL; keyptr = (t_keylist*)keyptr->next)
         {
           if(keyptr->done_flag == FALSE)
           {
             p_write_keylist_value(l_fp, keyptr);
             keyptr->done_flag = TRUE;
           }
         }   /* end for keylist loop */

         fclose(l_fp);
         l_rc = 0;
       }
       g_free(l_vin_filename);
  }

  return(l_rc);
}  /* end gap_vin_set_common_keylist */



/* --------------------------
 * gap_vin_get_all_keylist
 * --------------------------
 * get video info from .vin file
 */
static void
gap_vin_get_all_keylist(t_keylist *keylist, GapVinVideoInfo *vin_ptr, char *basename)
{
  char  *l_vin_filename;
  t_keylist *keyptr;
  GapVinTextFileLines *txf_ptr_root;
  GapVinTextFileLines *txf_ptr;
  GapVinVideoInfo *l_vin_ptr;
  int   l_len;
  
  l_vin_ptr = g_malloc(sizeof(GapVinVideoInfo));
  /* init wit defaults (for the case where no video_info file available) */
  l_vin_ptr->timezoom = 1;
  l_vin_ptr->framerate = 24.0;
  
  l_vin_ptr->onionskin_auto_enable = TRUE;
  l_vin_ptr->auto_replace_after_load = FALSE;
  l_vin_ptr->auto_delete_before_save = FALSE;

  l_vin_ptr->num_olayers        = 2;
  l_vin_ptr->ref_delta          = -1;
  l_vin_ptr->ref_cycle          = FALSE;
  l_vin_ptr->stack_pos          = 1;
  l_vin_ptr->stack_top          = FALSE;
  l_vin_ptr->asc_opacity        = FALSE;
  l_vin_ptr->opacity            = 80.0;
  l_vin_ptr->opacity_delta      = 80.0;
  l_vin_ptr->ignore_botlayers   = 1;
  l_vin_ptr->select_mode        = 6;     /* GAP_MTCH_ALL_VISIBLE */
  l_vin_ptr->select_case        = 0;     /* 0 .. ignore case, 1..case sensitve */
  l_vin_ptr->select_invert      = 0;     /* 0 .. no invert, 1 ..invert */
  l_vin_ptr->select_string[0] = '\0';
  
  l_vin_filename = gap_vin_alloc_name(basename);
  if(l_vin_filename)
  {
      txf_ptr_root = gap_vin_load_textfile(l_vin_filename);

      for(txf_ptr = txf_ptr_root; txf_ptr != NULL; txf_ptr = (GapVinTextFileLines *) txf_ptr->next)
      {
          for(keyptr=keylist; keyptr != NULL; keyptr = (t_keylist*)keyptr->next)
          {
             l_len = strlen(keyptr->keyword);
             if(strncmp(txf_ptr->line, keyptr->keyword, l_len) == 0)
             {
               switch(keyptr->dataype)
               {
                 case GAP_VIN_GINT32:
                   {
                      gint32 *val_ptr;
                      
                      val_ptr = (gint32 *)keyptr->val_ptr;
                      *val_ptr = atol(&txf_ptr->line[l_len]);
                   }
                   break;
                 case GAP_VIN_GDOUBLE:
                   {
                      gdouble *val_ptr;
                    
                      val_ptr = (gdouble *)keyptr->val_ptr;
                      /* setlocale independent string to double converion */
                      *val_ptr = g_ascii_strtod(&txf_ptr->line[l_len], NULL);
                   }
                   break;
                 case GAP_VIN_GBOOLEAN:
                   {
                      gboolean *val_ptr;
                      
                      val_ptr = (gboolean *)keyptr->val_ptr;
                      if((txf_ptr->line[l_len] == 'N')
                      || (txf_ptr->line[l_len] == 'n'))
                      {
                        *val_ptr = FALSE;
                      }
                      else
                      {
                        *val_ptr = TRUE;
                      }
                   }
                   break;
                 case GAP_VIN_STRING:
                   {
                      gchar *val_ptr;
                      gboolean esc_flag;
                      gint32   l_idx;
                      
                      val_ptr = (gchar *)keyptr->val_ptr;
                      if(txf_ptr->line[l_len] == '"')
                      {
                        esc_flag = FALSE;
                        for(l_idx=0; l_idx < MIN(4000-l_len, keyptr->len); l_idx++)
                        {
                          if((txf_ptr->line[l_len+l_idx] == '\n')
                          || (txf_ptr->line[l_len+l_idx] == '\0'))
                          {
                            break;
                          }
                          if((txf_ptr->line[l_len+l_idx] == '\\')
                          && (esc_flag == FALSE))
                          {
                            esc_flag = TRUE;
                            continue;
                          }
                          if((txf_ptr->line[l_len+l_idx] == '"')
                          && (esc_flag == FALSE))
                          {
                            break;
                          }
                          *(val_ptr++) = txf_ptr->line[l_len+l_idx];
                          esc_flag = FALSE;
                          
                        }
                      }
                      *val_ptr = '\0';
                      
                   }
                   break;
                 default:
                   break;
               }
             
               break;
             }
          }  /* end for keylist loop */
      } /* end for text lines scann loop */
      if(txf_ptr_root)
      {
        gap_vin_free_textfile_lines(txf_ptr_root);
      }

      g_free(l_vin_filename);
  }
}  /* end gap_vin_get_all_keylist */


/* --------------------------
 * gap_vin_get_all
 * --------------------------
 * get video info from .vin file
 * get does always fetch all values for all known keywords
 */
GapVinVideoInfo *
gap_vin_get_all(char *basename)
{
  GapVinVideoInfo *vin_ptr;
  t_keylist    *keylist;

  keylist = p_new_keylist();
  vin_ptr = g_new0 (GapVinVideoInfo, 1);
  vin_ptr->timezoom = 1;
  p_set_master_keywords(keylist, vin_ptr);
  p_set_onion_keywords(keylist, vin_ptr);
  
  gap_vin_get_all_keylist(keylist, vin_ptr, basename);

  vin_ptr->timezoom = MAX(1,vin_ptr->timezoom);
  
  p_free_keylist(keylist);

  if(gap_debug)
  {
     printf("gap_vin_get_all: RETURN with vin_ptr content:\n");
     printf("  num_olayers: %d\n",   (int)vin_ptr->num_olayers);
     printf("  ref_delta: %d\n",     (int)vin_ptr->ref_delta);
     printf("  ref_cycle: %d\n",     (int)vin_ptr->ref_cycle);
     printf("  stack_pos: %d\n",     (int)vin_ptr->stack_pos);
     printf("  stack_top: %d\n",     (int)vin_ptr->stack_top);
     printf("  opacity: %f\n",       (float)vin_ptr->opacity);
     printf("  opacity_delta: %f\n", (float)vin_ptr->opacity_delta);
     printf("  ignore_botlayers: %d\n", (int)vin_ptr->ignore_botlayers);
     printf("  asc_opacity: %d\n",   (int)vin_ptr->asc_opacity);
     printf("  onionskin_auto_enable: %d\n",   (int)vin_ptr->onionskin_auto_enable);
     printf("  auto_replace_after_load: %d\n",   (int)vin_ptr->auto_replace_after_load);
     printf("  auto_delete_before_save: %d\n",   (int)vin_ptr->auto_delete_before_save);
  }
  return(vin_ptr);
}  /* end gap_vin_get_all */


/* --------------------------
 * gap_vin_set_common
 * --------------------------
 * (re)write the current video info file  (_vin.gap file)
 * IMPORTANT: this Procedure affects only
 * the master values for framerate and timezoom.
 * (other settings like the onionskin specific Settings.
 *  are left unchanged)
 */
int
gap_vin_set_common(GapVinVideoInfo *vin_ptr, char *basename)
{
  t_keylist    *keylist;
  int          l_rc;

  keylist = p_new_keylist();

  p_set_master_keywords(keylist, vin_ptr);
  l_rc = gap_vin_set_common_keylist(keylist, vin_ptr, basename);
  
  p_free_keylist(keylist);

  return(l_rc);
}  /* end gap_vin_set_common */


/* --------------------------
 * gap_vin_set_common_onion
 * --------------------------
 * (re)write the onionskin specific values
 * to the video info file
 */
int
gap_vin_set_common_onion(GapVinVideoInfo *vin_ptr, char *basename)
{
  t_keylist    *keylist;
  int          l_rc;

  keylist = p_new_keylist();

  p_set_onion_keywords(keylist, vin_ptr);
  l_rc = gap_vin_set_common_keylist(keylist, vin_ptr, basename);
  
  p_free_keylist(keylist);

  return(l_rc);
}  /* end gap_vin_set_common_onion */
