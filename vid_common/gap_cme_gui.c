/* gap_cme_gui.c
 * 2001.04.08 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * This Module contains GUI Procedures for common Video Encoder Dialog
 * it calls the galde GTK GUI modules and aditional GUI related stuff
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
 * version 2.1.0a;  2004.05.06   hof: integration into gimp-gap project
 * version 1.2.2b;  2003.03.08   hof: thread for storyboard_file processing
 * version 1.2.2b;  2002.11.23   hof: added filtermacro_file, storyboard_file
 * version 1.2.1a;  2001.06.30   hof: created
 */

#define GAP_USE_PTHREAD


#include <config.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#include <gtk/gtk.h>

#include "gap-intl.h"

#include "gap_cme_main.h"
#include "gap_cme_gui.h"
#include "gap_cme_callbacks.h"

#include "gap_gve_sox.h"
#include "gap_gve_story.h"
#include "gap_arr_dialog.h"
#include "gap_audio_wav.h"
#include "gap_stock.h"


typedef struct t_global_stb
{
  gdouble framerate;
  gdouble aud_total_sec;
  gchar *composite_audio;
  gchar *errtext;
  gchar *errline;
  gchar *warntext;
  gchar *warnline;
  gint32 errline_nr;
  gint32 warnline_nr;
  gboolean vidhand_open_ok;
  gint32 first_frame_limit;
  gint32 last_frame_nr;
  gint32 master_width;
  gint32 master_height;

  gdouble progress;
  gchar   status_msg[65];


  gint32 poll_timertag;
  gboolean stb_job_done;
  GapGveStoryVidHandle *vidhand;
} t_global_stb;

static t_global_stb  global_stb;

static void            p_gap_message(const char *msg);
static gint            p_overwrite_dialog(GapCmeGlobalParams *gpp, gchar *filename, gint overwrite_mode);
static GapGveEncList*  pdb_find_video_encoders(void);
static void            p_replace_optionmenu_encodername(GapCmeGlobalParams *gpp);
static void            p_get_range_and_type (GapCmeGlobalParams *gpp
                           , gint32 *lower
                           , gint32 *upper
                           , GapGveTypeInputRange *range_type);
static void            p_print_storyboard_text_label(GapCmeGlobalParams *gpp, char *msg);
static void            p_print_time_label( GtkLabel *lbl, gint32   tmsec);
static gint32          p_update_aud_info (GapCmeGlobalParams *gpp
                           , GtkLabel *lbl_info
                           , GtkLabel *lbl_time
                           , GtkLabel *lbl_time0
                           , char *audioname);
static void            p_range_widgets_set_limits(GapCmeGlobalParams *gpp
                           , gint32 lower_limit
                           , gint32 upper_limit
                           , GapGveTypeInputRange range_type);
static void            p_init_shell_window_widgets (GapCmeGlobalParams *gpp);
static char *          p_dialog_use_storyboard_audio(char *storyboard_file);
static void            p_status_progress(GapCmeGlobalParams *gpp, t_global_stb *gstb);
static void            p_storybord_job_finished(GapCmeGlobalParams *gpp, t_global_stb *gstb);
static void            on_timer_poll(gpointer   user_data);
static void            p_thread_storyboard_file(void);

/* procedures to create the dialog windows */
static GtkWidget*      create_ow__dialog (GapCmeGlobalParams *gpp);
static GtkWidget*      create_shell_window (GapCmeGlobalParams *gpp);

static gint            p_overwrite_dialog(GapCmeGlobalParams *gpp, gchar *filename, gint overwrite_mode);

/* ----------------------------------------
 * p_gap_message
 * ----------------------------------------
 */
static void
p_gap_message(const char *msg)
{
  static GapArrButtonArg  l_argv[1];
  int               l_argc;  
  
  l_argv[0].but_txt  = GTK_STOCK_OK;
  l_argv[0].but_val  = 0;
  l_argc             = 1;

  if(msg)
  {
    if(*msg)
    {
       printf("%s\n", msg);
       gap_arr_buttons_dialog  (_("GAP Message"), msg, l_argc, l_argv, -1);
    }
  }
}  /* end p_gap_message */

/* ----------------------------------------
 * gap_cme_gui_check_gui_thread_is_active
 * ----------------------------------------
 */
gboolean
gap_cme_gui_check_gui_thread_is_active(GapCmeGlobalParams *gpp)
{
   static gboolean l_gap_message_open = FALSE;

   if(gpp->val.gui_proc_tid)
   {
     /* only one of the threads (Master or GUI thread) can use the PDB Interface (or call gimp_xxx procedures)
      * If 2 threads are talking to the gimp main app parallel it comes to crash.
      */
     /*if(gap_debug)*/ printf("MASTER: GUI thread %d is already active\n", (int)gpp->val.gui_proc_tid);
     if(l_gap_message_open == FALSE)
     {
       l_gap_message_open = TRUE;
       p_gap_message(_("Encoder specific Parameter Window still open"));
       l_gap_message_open = FALSE;
     }
     return (TRUE);
   }
   return (FALSE);
}  /* end gap_cme_gui_check_gui_thread_is_active */

/* ----------------------------------------
 * gap_cme_gui_pdb_call_encoder_gui_plugin
 * ----------------------------------------
 */
gint
gap_cme_gui_pdb_call_encoder_gui_plugin(GapCmeGlobalParams *gpp)
{
  if(gpp->ecp == NULL)
  {
    return -1;
  }

#ifdef GAP_USE_PTHREAD
  if(gap_cme_gui_check_gui_thread_is_active(gpp)) return -1;

  /* start a thread for asynchron PDB call of the gui_ procedure
   */
  /*if(gap_debug)*/ printf("MASTER: Before pthread_create\n");

  pthread_create(&gpp->val.gui_proc_tid, NULL, (void*)gap_cme_gui_pthread_async_pdb_call, NULL);

  /*if(gap_debug)*/ printf("MASTER: After pthread_create\n");
#else
  /* if threads are not used simply call the procedure
   * (the common GUI window is not refreshed until the called gui_proc ends)
   */
  gap_cme_gui_pthread_async_pdb_call();

#endif

  return 0;
}  /* end gap_cme_gui_pdb_call_encoder_gui_plugin */


/* ----------------------------------------
 * gap_cme_gui_pthread_async_pdb_call
 * ----------------------------------------
 */
void
gap_cme_gui_pthread_async_pdb_call(void)
{
   GapCmeGlobalParams *gpp;
   GimpParam       *l_ret_params;
   GimpParam       *l_argv;
   gint             l_retvals;
   gint             l_idx;
   gint             l_nparams;
   gint             l_nreturn_vals;
   GimpPDBProcType  l_proc_type;
   gchar           *l_proc_blurb;
   gchar           *l_proc_help;
   gchar           *l_proc_author;
   gchar           *l_proc_copyright;
   gchar           *l_proc_date;
   GimpParamDef    *l_params;
   GimpParamDef    *l_return_vals;
   gchar           *plugin_name;


  gpp = gap_cme_main_get_global_params();

  if(gap_debug) printf("THREAD: gap_cme_gui_pthread_async_pdb_call &gpp: %d\n", (int)gpp);

  plugin_name = gpp->val.ecp_sel.gui_proc;

  /* query for plugin_name to get its argument types */
  if (!gimp_procedural_db_proc_info (plugin_name,
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
    printf("ERROR: Plugin not available, Name was %s\n", plugin_name);

    if(gap_debug) printf("THREAD gui_proc err TERMINATING: %d\n", (int)gpp->val.gui_proc_tid);

    gpp->val.gui_proc_tid = 0;
    return;
  }

  /* construct the procedures arguments */
  l_argv = g_new (GimpParam, l_nparams);
  memset (l_argv, 0, (sizeof (GimpParam) * l_nparams));

  /* initialize the argument types */
  for (l_idx = 0; l_idx < l_nparams; l_idx++)
  {
    l_argv[l_idx].type = l_params[l_idx].type;
    switch(l_params[l_idx].type)
    {
      case GIMP_PDB_DISPLAY:
        l_argv[l_idx].data.d_display  = -1;
        break;
      case GIMP_PDB_DRAWABLE:
      case GIMP_PDB_LAYER:
      case GIMP_PDB_CHANNEL:
        l_argv[l_idx].data.d_drawable  = -1;
        break;
      case GIMP_PDB_IMAGE:
        l_argv[l_idx].data.d_image  = -1;
        break;
      case GIMP_PDB_INT32:
      case GIMP_PDB_INT16:
      case GIMP_PDB_INT8:
        l_argv[l_idx].data.d_int32  = 0;
        break;
      case GIMP_PDB_FLOAT:
        l_argv[l_idx].data.d_float  = 0.0;
        break;
      case GIMP_PDB_STRING:
        l_argv[l_idx].data.d_string  =  g_strdup("\0");
        break;
      default:
        l_argv[l_idx].data.d_int32  = 0;
        break;

    }
  }


  /* init the standard parameters, that should be common to all plugins */
  l_argv[0].data.d_int32     =  GIMP_RUN_INTERACTIVE;  /* run_mode */
  l_argv[1].data.d_string    =  g_strdup_printf("GAP_KEY_VIDENC_STDPAR_%d", (int)gpp->val.image_ID);

  gimp_set_data(l_argv[1].data.d_string, &gpp->val, sizeof(GapGveCommonValues));


  if(gap_debug)
  {
      printf("THREAD Common GUI key: %s\n", l_argv[1].data.d_string);
      printf("THREAD Common GUI rate: %f  w:%d h:%d\n", (float)gpp->val.framerate, (int)gpp->val.vid_width, (int)gpp->val.vid_height);
  }


  /* run the plug-in procedure */
  l_ret_params = gimp_run_procedure2 (plugin_name, &l_retvals, l_nparams, l_argv);
  /*  free up arguments and values  */
  g_free (l_argv);


  /*  free the query information  */
  g_free (l_proc_blurb);
  g_free (l_proc_help);
  g_free (l_proc_author);
  g_free (l_proc_copyright);
  g_free (l_proc_date);
  g_free (l_params);
  g_free (l_return_vals);



  if (l_ret_params[0].data.d_status != GIMP_PDB_SUCCESS)
  {
    printf("THREAD ERROR: p_call_plugin %s failed.\n", plugin_name);
  }
  else
  {
    if(gap_debug) printf("THREAD DEBUG: p_call_plugin: %s successful.\n", plugin_name);
  }

  /* the GUI of the encoder plugin might have changed the current video filename extension
   * (therefore we repeat the query for extension here)
   */
  gap_cme_gui_requery_vid_extension(gpp);

  if(gap_debug) printf("THREAD gui_proc TERMINATING: %d\n", (int)gpp->val.gui_proc_tid);

  gpp->val.gui_proc_tid = 0;
}  /* end gap_cme_gui_pthread_async_pdb_call */



/* ----------------------------------------
 * pdb_find_video_encoders
 * ----------------------------------------
 */
static GapGveEncList*
pdb_find_video_encoders(void)
{
  GapGveEncList *l_ecp;
  GapGveEncList *list_ecp;
  char **proc_list;
  int num_procs;
  int i;
  int j;
  gint             l_nparams;
  gint             l_nreturn_vals;
  GimpPDBProcType   l_proc_type;
  gchar            *l_proc_blurb;
  gchar            *l_proc_help;
  gchar            *l_proc_author;
  gchar            *l_proc_copyright;
  gchar            *l_proc_date;
  GimpParamDef    *l_paramdef;
  GimpParamDef    *l_return_vals;


  if(gap_debug) printf("pdb_find_video_encoders: START\n");

  list_ecp = NULL;
  l_ecp = NULL;

  gimp_procedural_db_query (GAP_WILDCARD_VIDEO_ENCODERS, ".*", ".*", ".*", ".*", ".*", ".*",
                           &num_procs, &proc_list);
  for (j = 0; j < num_procs; j++)
  {
     i = (num_procs -1) - j;
     if(TRUE == gimp_procedural_db_proc_info (proc_list[i],
                             &l_proc_blurb,
                             &l_proc_help,
                             &l_proc_author,
                             &l_proc_copyright,
                             &l_proc_date,
                             &l_proc_type,
                             &l_nparams,
                             &l_nreturn_vals,
                             &l_paramdef,
                             &l_return_vals))
     {
        char *ecp_infoproc;

        if (l_nparams != GAP_VENC_NUM_STANDARD_PARAM)
        {
           if(gap_debug) printf("pdb_find_video_encoders: procedure %s is skipped (nparams %d != %d)\n"
                               , proc_list[i], (int)l_nparams, (int)GAP_VENC_NUM_STANDARD_PARAM );
           continue;
        }

        l_ecp = g_malloc(sizeof(GapGveEncList));

        /* pepare default values for menu_name short_description and video_extensions */
        g_snprintf(l_ecp->menu_name, sizeof(l_ecp->menu_name), "%s", proc_list[i]);
        g_snprintf(l_ecp->video_extension, sizeof(l_ecp->video_extension), ".%d", (int)i);
        g_snprintf(l_ecp->short_description, sizeof(l_ecp->short_description), "%s", _("no description available"));

        /* get Menuname and video_extension via ecp_infoproc */
        ecp_infoproc = g_strdup_printf("%s%s", GAP_QUERY_PREFIX_VIDEO_ENCODERS, proc_list[i]);

        if(TRUE == gimp_procedural_db_proc_info (ecp_infoproc,
                             &l_proc_blurb,
                             &l_proc_help,
                             &l_proc_author,
                             &l_proc_copyright,
                             &l_proc_date,
                             &l_proc_type,
                             &l_nparams,
                             &l_nreturn_vals,
                             &l_paramdef,
                             &l_return_vals))
        {
           if(gap_debug)
           {
             printf("pdb_find_video_encoders: run procedure %s nparams:%d nretrun_vals:%d\n"
                     , ecp_infoproc, (int)l_nparams, (int)l_nreturn_vals);
           }

           if(l_nreturn_vals >= 1)
           {
              GimpParam* l_params;
              gint       l_retvals;

              /* query for menu_name */
              l_params = gimp_run_procedure (ecp_infoproc,
                          &l_retvals,
                          GIMP_PDB_STRING, GAP_VENC_PAR_MENU_NAME,
                          GIMP_PDB_END);

              if((l_params[0].data.d_status == GIMP_PDB_SUCCESS) && (l_retvals >= 2))
              {
                if((l_params[1].data.d_string != NULL) && (l_params[1].type == GIMP_PDB_STRING))
                {
                  if(gap_debug) printf("[1].d_string %s\n", l_params[1].data.d_string);
                  g_snprintf(l_ecp->menu_name, sizeof(l_ecp->menu_name), "%s", l_params[1].data.d_string);
                }
              }
              g_free(l_params);

              /* query for video_extension */
              l_params = gimp_run_procedure (ecp_infoproc,
                          &l_retvals,
                          GIMP_PDB_STRING, GAP_VENC_PAR_VID_EXTENSION,
                          GIMP_PDB_END);

              if((l_params[0].data.d_status == GIMP_PDB_SUCCESS) && (l_retvals >= 2))
              {
                if((l_params[1].data.d_string) && (l_params[1].type == GIMP_PDB_STRING))
                {
                  if(gap_debug) printf("[1].d_string %s\n", l_params[1].data.d_string);
                  g_snprintf(l_ecp->video_extension, sizeof(l_ecp->video_extension), "%s",  l_params[1].data.d_string);
                }
              }
              g_free(l_params);

              /* query for short_description */
              l_params = gimp_run_procedure (ecp_infoproc,
                          &l_retvals,
                          GIMP_PDB_STRING, GAP_VENC_PAR_SHORT_DESCRIPTION,
                          GIMP_PDB_END);

              if((l_params[0].data.d_status == GIMP_PDB_SUCCESS) && (l_retvals >= 2))
              {
                if((l_params[1].data.d_string) && (l_params[1].type == GIMP_PDB_STRING))
                {
                  if(gap_debug) printf("[1].d_string %s\n", l_params[1].data.d_string);
                  g_snprintf(l_ecp->short_description, sizeof(l_ecp->short_description), "%s",  l_params[1].data.d_string);
                }
              }
              g_free(l_params);


              /* query for (optional) gui_proc (to set encoder soecific params) */
              l_params = gimp_run_procedure (ecp_infoproc,
                          &l_retvals,
                          GIMP_PDB_STRING, GAP_VENC_PAR_GUI_PROC,
                          GIMP_PDB_END);

              if((l_params[0].data.d_status == GIMP_PDB_SUCCESS) && (l_retvals >= 2))
              {
                if((l_params[1].data.d_string) && (l_params[1].type == GIMP_PDB_STRING))
                {
                  if(gap_debug) printf("[1].d_string %s\n", l_params[1].data.d_string);
                  g_snprintf(l_ecp->gui_proc, sizeof(l_ecp->gui_proc), "%s",  l_params[1].data.d_string);
                }
              }
              g_free(l_params);
           }
        }

        g_snprintf(l_ecp->vid_enc_plugin, sizeof(l_ecp->vid_enc_plugin), "%s", proc_list[i]);
        l_ecp->menu_nr = i+1;
        l_ecp->next = list_ecp;

        list_ecp = l_ecp;
     }
  }
  if(list_ecp == NULL)
  {
    list_ecp = g_malloc(sizeof(GapGveEncList));
    list_ecp->menu_nr = 0;
    list_ecp->vid_enc_plugin[0] = '\0';
    g_snprintf(list_ecp->menu_name, sizeof(list_ecp->menu_name), "none");
    g_snprintf(list_ecp->video_extension, sizeof(l_ecp->video_extension), ".0");
    list_ecp->next = NULL;
  }

  return(list_ecp);
}  /* end pdb_find_video_encoders */


/* ----------------------------------------
 * gap_cme_gui_requery_vid_extension
 * ----------------------------------------
 * some encoders do support more than 1 extension
 * therefore we do repeat the query for extension
 * to get the current one
 */
void
gap_cme_gui_requery_vid_extension(GapCmeGlobalParams *gpp)
{
  GapGveEncList *l_ecp;

  if(gap_debug) printf("gap_cme_gui_requery_vid_extension START\n");

  for(l_ecp = gpp->ecp; l_ecp != NULL; l_ecp = (GapGveEncList *)l_ecp->next)
  {
    if(strcmp(l_ecp->vid_enc_plugin , gpp->val.ecp_sel.vid_enc_plugin) == 0)
    {
        char      *ecp_infoproc;
        GimpParam *l_params;
        gint       l_retvals;

        /* get video_extension via ecp_infoproc */
        ecp_infoproc = g_strdup_printf("%s%s", GAP_QUERY_PREFIX_VIDEO_ENCODERS, l_ecp->vid_enc_plugin);


        /* (re) query for video_extension */
        l_params = gimp_run_procedure (ecp_infoproc,
                    &l_retvals,
                    GIMP_PDB_STRING, GAP_VENC_PAR_VID_EXTENSION,
                    GIMP_PDB_END);

        if((l_params[0].data.d_status == GIMP_PDB_SUCCESS) && (l_retvals >= 2))
        {
          if((l_params[1].data.d_string) && (l_params[1].type == GIMP_PDB_STRING))
          {
            if(gap_debug) printf("gap_cme_gui_requery_vid_extension: [1].d_string %s\n", l_params[1].data.d_string);
            g_snprintf(l_ecp->video_extension, sizeof(l_ecp->video_extension), "%s",  l_params[1].data.d_string);
            g_snprintf(gpp->val.ecp_sel.video_extension, sizeof(gpp->val.ecp_sel.video_extension), "%s",  l_params[1].data.d_string);
          }
        }
        g_free(l_params);
        g_free(ecp_infoproc);
        gap_cme_gui_upd_vid_extension(gpp);

        return;
    }
  }
}  /* end gap_cme_gui_requery_vid_extension */


/* ----------------------------------------
 * p_replace_optionmenu_encodername
 * ----------------------------------------
 * replace encodername optionmenu by dynamic menu we got from the PDB
 */
static void
p_replace_optionmenu_encodername(GapCmeGlobalParams *gpp)
{
  GtkWidget *wgt;
  GtkWidget *menu_item;
  GtkWidget *menu;
  GapGveEncList *l_ecp;
  gint  l_active_idx;
  gint  l_idx;

  if(gap_debug) printf("p_replace_optionmenu_encodername: START\n");

  wgt = gpp->cme__optionmenu_encodername;
  menu =  gtk_option_menu_get_menu(GTK_OPTION_MENU(wgt));
  if(menu == NULL)
  {
    printf("p_replace_optionmenu_encodername: widget is no GTK_OPTION_MENU\n");
    return;
  }

  menu = gtk_menu_new ();

  l_idx = 0;
  l_active_idx = 0;
  l_ecp = gpp->ecp;
  while(l_ecp)
  {
     if(gap_debug)
     {
        printf("p_replace_optionmenu_encodername: %d, %s %s\n"
              , (int)l_ecp->menu_nr
              , l_ecp->menu_name
              , l_ecp->vid_enc_plugin);
     }

     menu_item = gtk_menu_item_new_with_label(l_ecp->menu_name);

     gtk_widget_show (menu_item);
     gtk_menu_append (GTK_MENU (menu), menu_item);

     gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
                              (GtkSignalFunc) on_cme__optionmenu_enocder,
                              (gpointer)gpp);
     g_object_set_data (G_OBJECT (menu_item), GAP_CME_GUI_ENC_MENU_ITEM_ENC_PTR, (gpointer)l_ecp);

     /* set FFMPEG as default encoder (if this encoder is installed) */
     if(strcmp(l_ecp->vid_enc_plugin, GAP_PLUGIN_NAME_FFMPEG_ENCODE) == 0)
     {
         gtk_menu_item_activate(GTK_MENU_ITEM (menu_item));
         l_active_idx = l_idx;
     }
     l_ecp =  (GapGveEncList *)l_ecp->next;
     l_idx++;
  }

  gtk_option_menu_set_menu (GTK_OPTION_MENU (wgt), menu);
  gtk_option_menu_set_history (GTK_OPTION_MENU (wgt), l_active_idx);
}  /* end p_replace_optionmenu_encodername */


/* ----------------------------------------
 * p_get_range_and_type
 * ----------------------------------------
 */
static void
p_get_range_and_type (GapCmeGlobalParams *gpp, gint32 *lower, gint32 *upper, GapGveTypeInputRange *range_type)

{
 /* Range limits for widgets "cme__spinbutton_from" and "cme__spinbutton_to"
  * If there is just one frame, we operate on layers
  */
 if(gpp->ainfo.last_frame_nr - gpp->ainfo.first_frame_nr == 0)
 {
   gint          l_nlayers;
   gint32       *l_layers_list;


   l_layers_list = gimp_image_get_layers(gpp->val.image_ID, &l_nlayers);
   g_free(l_layers_list);
   *lower = 0;
   *upper = l_nlayers -1;
   *range_type = GAP_RNGTYPE_LAYER;
 }
 else
 {
   *lower = gpp->ainfo.first_frame_nr;
   *upper = gpp->ainfo.last_frame_nr;
   *range_type = GAP_RNGTYPE_FRAMES;
 }
}   /* end p_get_range_and_type */


/* ----------------------------------------
 * p_print_storyboard_text_label
 * ----------------------------------------
 */
static void
p_print_storyboard_text_label(GapCmeGlobalParams *gpp, char *msg)
{
  GtkWidget  *lbl;

  lbl = gpp->cme__label_storyboard_helptext;
  if(lbl==NULL) { return; }


  if(msg)
  {
     gtk_label_set_text(GTK_LABEL(lbl), msg);
  }
  else
  {
     gtk_label_set_text(GTK_LABEL(lbl),
      _("Storyboardfiles are textfiles that are used to\n"
        "assemble a video from a list of single images,\n"
        "frameranges, videoclips, gif animations or audiofiles.\n"
        "the frames are organized in tracks,\n"
        "and allow fadeing, scale and move\n"
        "operations between the tracks.\n"
        "(see STORYBOARD_FILE_DOC.txt for details)")
        );
  }
}  /* end p_print_storyboard_text_label */


/* ----------------------------------------
 * p_print_time_label
 * ----------------------------------------
 */
static void
p_print_time_label( GtkLabel *lbl, gint32   tmsec)
{
  gint32   tms;
  gint32   tsec;
  gint32   tmin;
  gchar    txt[20];


  tms = tmsec % 1000;
  tsec = (tmsec / 1000) % 60;
  tmin = tmsec / 60000;

  g_snprintf(txt, sizeof(txt), "%02d:%02d:%03d"
                               , (int)tmin
                               , (int)tsec
                               , (int)tms
                               );
  gtk_label_set_text(lbl, txt);
}  /* end p_print_time_label */



/* ----------------------------------------
 * p_update_aud_info
 * ----------------------------------------
 */
static gint32
p_update_aud_info (GapCmeGlobalParams *gpp
                  , GtkLabel *lbl_info
                  , GtkLabel *lbl_time
                  , GtkLabel *lbl_time0
                  , char *audioname)
{
  gchar       txt[200];
  long        samplerate;
  long        disp_samplerate;   /* samplerate to display (rate after resampling by sox) */
  long        channels;
  long        bytes_per_sample;
  long        bits;
  long        samples;
  int         l_rc;
  gint32      tmsec;        /* audioplaytime in milli secs */

  if(*audioname == '\0')
  {
     p_print_time_label(lbl_time, 0);
     p_print_time_label(lbl_time0, 0);
     return 0;
  }
  g_snprintf(txt, sizeof(txt), "??:??:???");
  gtk_label_set_text(lbl_time, txt);
  gtk_label_set_text(lbl_time0, txt);

  samplerate = 0;
  disp_samplerate = gpp->val.samplerate;
  g_snprintf(txt, sizeof(txt), " ");
  if(g_file_test(audioname, G_FILE_TEST_EXISTS))
  {
     tmsec = 0;
     /* check for WAV file, and get audio informations */
     l_rc = gap_audio_wav_file_check(audioname
                     , &samplerate, &channels
                     , &bytes_per_sample, &bits, &samples);

     if(l_rc == 0)
     {
       if((samplerate > 0) && (channels > 0))
       {
         disp_samplerate = samplerate;
         /* have to calculate in gdouble, because samples * 1000
          * may not fit into gint32 datasize and brings negative results
          */
         tmsec = (gint32)((((gdouble)samples / (gdouble)channels) * 1000.0) / (gdouble)disp_samplerate);
       }

       g_snprintf(txt, sizeof(txt), _("%s, Bit:%d Chan:%d Rate:%d")
                                  , "WAV"
                                  , (int)bits
                                  , (int)channels
                                  , (int)samplerate
                                  );
       p_print_time_label(lbl_time, tmsec);
       p_print_time_label(lbl_time0, tmsec);
     }
     else
     {
       g_snprintf(txt, sizeof(txt), _("UNKNOWN (using sox)"));
     }
  }

  gtk_label_set_text(lbl_info, txt);
  return (samplerate);
}  /* end p_update_aud_info */

/* ----------------------------------------
 * gap_cme_gui_upd_vid_extension
 * ----------------------------------------
 */
void
gap_cme_gui_upd_vid_extension (GapCmeGlobalParams *gpp)
{
  gchar *l_vid;
  gint   l_idx;
  char *videoname;

 l_vid = g_strdup(gpp->val.videoname);
 for(l_idx = strlen(l_vid) -1;  l_idx > 0; l_idx--)
 {
   if(l_vid[l_idx] == '.')
   {
      l_vid[l_idx] = '\0';
      break;
   }
 }
 videoname = g_strdup_printf("%s%s", l_vid, &gpp->val.ecp_sel.video_extension[0]);
 g_snprintf(gpp->val.videoname, sizeof(gpp->val.videoname), "%s", videoname);

 gtk_entry_set_text(GTK_ENTRY(gpp->cme__entry_video), videoname);
 gtk_label_set_text(GTK_LABEL(gpp->cme__short_description)
                  , &gpp->val.ecp_sel.short_description[0]);

 g_free(l_vid);
 g_free(videoname);
}  /* end gap_cme_gui_upd_vid_extension */

/* ----------------------------------------
 * gap_cme_gui_upd_wgt_sensitivity
 * ----------------------------------------
 */
void
gap_cme_gui_upd_wgt_sensitivity (GapCmeGlobalParams *gpp)
{
  gboolean   sensitive;

  sensitive = FALSE;
  if(gpp->val.audioname1[0] != '\0')
  {
    if(g_file_test(gpp->val.audioname1, G_FILE_TEST_EXISTS))
    {
      sensitive = TRUE;
    }
  }

  gtk_widget_set_sensitive(gpp->cme__spinbutton_samplerate, sensitive);
  gtk_widget_set_sensitive(gpp->cme__button_gen_tmp_audfile, sensitive);

  sensitive = FALSE;
  if(gpp->val.ecp_sel.gui_proc[0] != '\0')
  {
      sensitive = TRUE;
  }

  gtk_widget_set_sensitive(gpp->cme__button_params, sensitive);
}  /* end gap_cme_gui_upd_wgt_sensitivity */

/* ----------------------------------------
 * gap_cme_gui_update_aud_labels
 * ----------------------------------------
 */
void
gap_cme_gui_update_aud_labels (GapCmeGlobalParams *gpp)
{
  GtkLabel *lbl_tmp_audfile;
  GtkLabel *lbl_info;
  GtkLabel *lbl_info_tmp;
  GtkLabel *lbl_time;
  GtkLabel *lbl_time0;
  GtkLabel *lbl_time_tmp;

  lbl_info  = GTK_LABEL(gpp->cme__label_aud1_info);
  lbl_time  = GTK_LABEL(gpp->cme__label_aud1_time);
  lbl_time0 = GTK_LABEL(gpp->cme__label_aud0_time);
  lbl_time_tmp = GTK_LABEL(gpp->cme__label_aud_tmp_time);
  lbl_info_tmp = GTK_LABEL(gpp->cme__label_aud_tmp_info);
  lbl_tmp_audfile = GTK_LABEL(gpp->cme__label_tmp_audfile);

  gtk_label_set_text(lbl_info, " ");

  gpp->val.wav_samplerate1  =
  p_update_aud_info(gpp, lbl_info, lbl_time, lbl_time0, gpp->val.audioname1);


  gtk_label_set_text(lbl_time_tmp, " ");
  gtk_label_set_text(lbl_info_tmp, " ");
  gtk_label_set_text(lbl_tmp_audfile, " ");

  if(gpp->val.tmp_audfile[0] != '\0')
  {
    if(g_file_test(gpp->val.tmp_audfile, G_FILE_TEST_EXISTS))
    {
       gpp->val.wav_samplerate_tmp =
       p_update_aud_info(gpp, lbl_info_tmp, lbl_time_tmp, lbl_time0, gpp->val.tmp_audfile);
       gtk_label_set_text(lbl_tmp_audfile, gpp->val.tmp_audfile);
   }
  }

  gap_cme_gui_upd_wgt_sensitivity (gpp);
}  /* end gap_cme_gui_update_aud_labels */


/* ----------------------------------------
 * gap_cme_gui_update_vid_labels
 * ----------------------------------------
 */
void
gap_cme_gui_update_vid_labels (GapCmeGlobalParams *gpp)
{
  GtkLabel   *lbl;
  gint32      tmsec;        /* audioplaytime in milli secs */
  gdouble     msec_per_frame;

  if(gpp->val.framerate > 0)
  {
    msec_per_frame = 1000.0 / gpp->val.framerate;
  }
  else
  {
    msec_per_frame = 1000.0;
  }



  lbl = GTK_LABEL(gpp->cme__label_fromtime);
  tmsec = (gpp->val.range_from - gpp->ainfo.first_frame_nr) * msec_per_frame;
  p_print_time_label(lbl, tmsec);

  lbl = GTK_LABEL(gpp->cme__label_totime);
  tmsec = (gpp->val.range_to - gpp->ainfo.first_frame_nr) * msec_per_frame;
  p_print_time_label(lbl, tmsec);

  lbl = GTK_LABEL(gpp->cme__label_totaltime);
  tmsec = abs(gpp->val.range_to - gpp->val.range_from) * msec_per_frame;
  p_print_time_label(lbl, tmsec);
}  /* end gap_cme_gui_update_vid_labels */


/* ----------------------------------------
 * gap_cme_gui_util_sox_widgets
 * ----------------------------------------
 */
void
gap_cme_gui_util_sox_widgets (GapCmeGlobalParams *gpp)
{
  GtkEntry *entry;

  if(gap_debug) printf("gap_cme_gui_util_sox_widgets\n");

  entry = GTK_ENTRY(gpp->cme__entry_sox);
  gtk_entry_set_text(entry, gpp->val.util_sox);

  entry = GTK_ENTRY(gpp->cme__entry_sox_options);
  gtk_entry_set_text(entry, gpp->val.util_sox_options);
}  /* end gap_cme_gui_util_sox_widgets */


/* ----------------------------------------
 * p_range_widgets_set_limits
 * ----------------------------------------
 */
static void
p_range_widgets_set_limits(GapCmeGlobalParams *gpp
                          , gint32 lower_limit
                          , gint32 upper_limit
                          , GapGveTypeInputRange range_type)
{
  GtkAdjustment *adj;
  gchar *lbl_text;
  gchar *range_text;


  /* constraint range into lower_limit / upper_limit */
  /*
   * gpp->val.range_from = MAX(lower_limit, gpp->val.range_from);
   * gpp->val.range_from = MIN(upper_limit, gpp->val.range_from);
   * gpp->val.range_to = MAX(lower_limit, gpp->val.range_to);
   * gpp->val.range_to = MIN(upper_limit, gpp->val.range_to);
   */
  if(range_type == GAP_RNGTYPE_LAYER)
  {
    /* invers range for encoding Multilayer images
     * (top layer is usually the last frame but has index 0 in gimp images)
     */
    gpp->val.range_from = upper_limit;
    gpp->val.range_to = lower_limit;
  }
  else
  {
    gpp->val.range_from = lower_limit;
    gpp->val.range_to = upper_limit;
  }

  /* update spinbutton limits and values */
  adj = GTK_ADJUSTMENT(gpp->cme__spinbutton_from_adj);
  adj->lower = (gfloat) lower_limit;
  adj->upper = (gfloat) upper_limit;
  gtk_adjustment_set_value(adj, (gfloat)gpp->val.range_from);

  adj = GTK_ADJUSTMENT(gpp->cme__spinbutton_to_adj);
  adj->lower = (gfloat) lower_limit;
  adj->upper = (gfloat) upper_limit;
  gtk_adjustment_set_value(adj, (gfloat)gpp->val.range_to);

  switch(range_type)
  {
    case GAP_RNGTYPE_STORYBOARD:
      range_text = g_strdup(_("Storyframe"));
      break;
    case GAP_RNGTYPE_LAYER:
      range_text = g_strdup(_("Layer"));
      break;
    default:
      range_text = g_strdup(_("Frame"));
      break;
  }

  lbl_text = g_strdup_printf(_("From %s:"),  range_text);
  gtk_label_set_text(GTK_LABEL(gpp->cme__label_from), lbl_text);
  g_free(lbl_text);

  lbl_text = g_strdup_printf(_("To %s:"),  range_text);
  gtk_label_set_text(GTK_LABEL(gpp->cme__label_to), lbl_text);
  g_free(lbl_text);

  g_free(range_text);

  gap_cme_gui_update_vid_labels (gpp);
  gap_cme_gui_update_aud_labels (gpp);
}       /* end p_range_widgets_set_new_limits */

/* ----------------------------------------
 * p_init_shell_window_widgets
 * ----------------------------------------
 */
static void
p_init_shell_window_widgets (GapCmeGlobalParams *gpp)
{
 if(gap_debug) printf("p_init_shell_window_widgets: Start INIT\n");

 /* put initial values to the widgets */

 /* widgets "cme__spinbutton_from" and "cme__spinbutton_to" */
 {
   GapGveTypeInputRange l_rangetype;
   gint32  l_first_frame_limit;
   gint32  l_last_frame_nr;

   p_get_range_and_type (gpp, &l_first_frame_limit, &l_last_frame_nr, &l_rangetype);
   gpp->val.input_mode     = l_rangetype;

   p_range_widgets_set_limits(gpp
                             , l_first_frame_limit
                             , l_last_frame_nr
                             , l_rangetype
                             );
 }

 gtk_adjustment_set_value(GTK_ADJUSTMENT(gpp->cme__spinbutton_framerate_adj)
                         , (gfloat)gpp->val.framerate);
 gtk_adjustment_set_value(GTK_ADJUSTMENT(gpp->cme__spinbutton_samplerate_adj)
                         , (gfloat)gpp->val.samplerate);
 gtk_adjustment_set_value(GTK_ADJUSTMENT(gpp->cme__spinbutton_width_adj)
                         , (gfloat)gpp->val.vid_width);
 gtk_adjustment_set_value(GTK_ADJUSTMENT(gpp->cme__spinbutton_height_adj)
                         , (gfloat)gpp->val.vid_height);

 gtk_entry_set_text(GTK_ENTRY(gpp->cme__entry_video), gpp->val.videoname);
 gtk_entry_set_text(GTK_ENTRY(gpp->cme__entry_audio1), gpp->val.audioname1);
 gtk_entry_set_text(GTK_ENTRY(gpp->cme__entry_mac), gpp->val.filtermacro_file);

 gap_cme_gui_update_aud_labels (gpp);
 gap_cme_gui_update_vid_labels (gpp);

 /* dynamic optionmenu for all encoders that are registered at runtime */
 p_replace_optionmenu_encodername(gpp);
 gap_cme_gui_util_sox_widgets(gpp);
}    /* end p_init_shell_window_widgets */


/* ----------------------------------------
 * p_dialog_use_storyboard_audio
 * ----------------------------------------
 * p_dialog_use_storyboard_audio
 *   Dialog Requester Window to let user decide
 *   if to create and use the composite audiotrack (return Audifilename)
 *   or not (return NULL)
 */
static char *
p_dialog_use_storyboard_audio(char *storyboard_file)
{
  static GapArrArg  argv[1];
  int               l_argc;
  int               l_rc;


  l_argc             = 1;

  if(*storyboard_file)
  {
     char *l_msg;
     char *l_audio_filename;

     l_audio_filename = g_strdup_printf("%s_composite_audio.wav", storyboard_file);
     l_msg = g_strdup_printf("Audio track(s) found.\nCreate composite audiotrack as file:\n\n%s\n\nand use that file for encoding?"
                            , l_audio_filename);

     gap_arr_arg_init(&argv[0], GAP_ARR_WGT_LABEL);
     argv[0].label_txt = l_msg;

     l_rc = gap_arr_ok_cancel_dialog(_("Storyboardfile has Audio")
                          , _("Create Audio")
                          , l_argc
                          , argv
                          );
     g_free(l_msg);
     if(l_rc == TRUE)
     {
        if(gap_debug) printf("** START create request for COMPOSITE AUDIO: %s\n", l_audio_filename);

        return(l_audio_filename);
     }
     g_free(l_audio_filename);
  }
  if(gap_debug) printf("** Cancel  do not crate COMPOSITE AUDIO !\n");
  return (NULL);
}   /* p_dialog_use_storyboard_audio */


/* ------------------
 * p_status_progress
 * ------------------
 */
static void
p_status_progress(GapCmeGlobalParams *gpp, t_global_stb *gstb)
{
  GtkWidget  *pbar;
  GtkWidget  *status_lbl;
  /* storyboard thread job still running
   * update status, and restart poll timer
   */
  status_lbl = gpp->cme__label_status;
  if(status_lbl) gtk_label_set_text(GTK_LABEL(status_lbl), gstb->status_msg);

  pbar = gpp->cme__progressbar_status;
  if(pbar) gtk_progress_bar_update (GTK_PROGRESS_BAR (pbar), gstb->progress);

  if(gap_debug) printf("progress: %f\n", (float)gstb->progress );
}  /* end p_status_progress */


/* ------------------------
 * p_storybord_job_finished
 * ------------------------
 * This is called from polling_timer
 * when the storyboard thread has finished
 *
 *  print storyboard parse report, update widgets
 *
 *   check if storyboard open was successful,
 *   if successful, set lower and upper limit of the range spinbutton widgets
 *   to 1 and the total number of frames found in the storyboard_file.
 *
 *   if no storyboard file exists then use the first/last frame_nr from the ainfo structure
 *   that describes the animation frames from the invoking image_frame.
 */
static void
p_storybord_job_finished(GapCmeGlobalParams *gpp, t_global_stb *gstb)
{
  char *l_msg;
  char *l_framerate_info;
  char *l_size_info;
  char *l_playtime_info;
  gdouble l_vid_total_sec;



  if(gap_debug) printf("p_storybord_job_finished: START\n");

  gstb->progress = 1.0;
  g_snprintf(gstb->status_msg, sizeof(gstb->status_msg), _("ready"));
  p_status_progress(gpp, gstb);

#ifdef GAP_USE_PTHREAD
   /* is the encoder specific gui_thread still open ? */
   if(gpp->val.gui_proc_tid != 0)
   {
      /* wait until thread exits */
      printf("p_storybord_job_finished: before pthread_join\n");
      pthread_join(gpp->val.gui_proc_tid, 0);
      printf("p_storybord_job_finished: after pthread_join\n");
      gpp->val.gui_proc_tid = 0;
   }
#endif

  l_vid_total_sec = 0.0;
  if(gstb->vidhand_open_ok)
  {
    if(gstb->framerate != 0.0)
    {
       l_framerate_info = g_strdup_printf(_("using master_framerate %2.2f found in file")
                                         ,(float)gstb->framerate);
       gtk_adjustment_set_value(GTK_ADJUSTMENT(gpp->cme__spinbutton_framerate_adj)
                                , (gfloat)gstb->framerate);
       gpp->val.framerate = gstb->framerate;

    }
    else
    {
       l_framerate_info = g_strdup_printf(_("file has no master_framerate setting"));
    }
    if(gpp->val.framerate > 0)
    {
      l_vid_total_sec = gpp->val.storyboard_total_frames / gpp->val.framerate;
    }

    if(gstb->aud_total_sec > 0.0)
    {
      if(gstb->composite_audio)
      {
         if(g_file_test(gstb->composite_audio, G_FILE_TEST_EXISTS))
         {
            GtkEntry *entry;

            g_snprintf(gpp->val.audioname1, sizeof(gpp->val.audioname1), "%s"
                      , gstb->composite_audio);
            gpp->val.audioname_ptr = &gpp->val.audioname1[0];
            entry = GTK_ENTRY(gpp->cme__entry_audio1);
            if(entry)
            {
               gtk_entry_set_text(entry, gpp->val.audioname_ptr);
            }

            g_free(gstb->composite_audio);
            gstb->composite_audio = NULL;
         }
      }

      l_playtime_info = g_strdup_printf(_("composite video track playtime %.3fsec (%d frames)\ncomposite audiotrack playtime %.3f secs")
                                           , (float)l_vid_total_sec
                                           , (int) gpp->val.storyboard_total_frames
                                           , (float)gstb->aud_total_sec
                                           );
    }
    else
    {
      l_playtime_info = g_strdup_printf(_("composite video track playtime %.3fsec (%d frames)\nhas NO audiotracks")
                                           , (float)l_vid_total_sec
                                           , (int) gpp->val.storyboard_total_frames
                                           );
    }

    if(gstb->master_width != 0)
    {
       l_size_info = g_strdup_printf(_("using master_size %d x %d found in file")
                                         ,(int)gstb->master_width
                                         ,(int)gstb->master_height);
       gtk_adjustment_set_value(GTK_ADJUSTMENT(gpp->cme__spinbutton_width_adj)
                               , (gfloat)gstb->master_width);

       gtk_adjustment_set_value(GTK_ADJUSTMENT(gpp->cme__spinbutton_height_adj)
                               , (gfloat)gstb->master_height);
       gpp->val.vid_width = gstb->master_width;
       gpp->val.vid_height = gstb->master_height;
    }
    else
    {
       l_size_info = g_strdup_printf(_("file has no master_size setting"));
    }

   if(gstb->errtext)
   {
       if(gstb->warntext)
       {
         l_msg = g_strdup_printf(_("Storyboard file %s checkreport:\n\n%s\n%s\n%s\n\n%s\n[%d:] %s\n\n%s\n[%d:] %s")
                                 , " " /* gpp->val.storyboard_file */
                                 , l_playtime_info
                                 , l_size_info
                                 , l_framerate_info
                                 , gstb->errtext
                                 , (int)gstb->errline_nr
                                 , gstb->errline
                                 , gstb->warntext
                                 , (int)gstb->warnline_nr
                                 , gstb->warnline
                                 );
       }
       else
       {
         l_msg = g_strdup_printf(_("Storyboard file %s checkreport:\n\n%s\n%s\n%s\n\n%s\n[%d:] %s")
                                 , " " /* gpp->val.storyboard_file */
                                 , l_playtime_info
                                 , l_size_info
                                 , l_framerate_info
                                 , gstb->errtext
                                 , (int)gstb->errline_nr
                                 , gstb->errline
                                 );
       }
    }
    else
    {
       if(gstb->warntext)
       {
         l_msg = g_strdup_printf(_("Storyboard file %s checkreport:\n\n%s\n%s\n%s\n\n%s\n[%d:] %s")
                                 , " " /* gpp->val.storyboard_file */
                                 , l_playtime_info
                                 , l_size_info
                                 , l_framerate_info
                                 , gstb->warntext
                                 , (int)gstb->warnline_nr
                                 , gstb->warnline
                                 );
       }
       else
       {
         l_msg = g_strdup_printf(_("Storyboard file %s checkreport:\n\n%s\n%s\n%s\n\nno errors found, file is OK")
                                 ,  " " /* gpp->val.storyboard_file */
                                 , l_playtime_info
                                 , l_size_info
                                 , l_framerate_info
                                );
       }
    }
    g_free(l_size_info);
    g_free(l_framerate_info);
  }
  else
  {
     l_msg = g_strdup_printf(_("Storyboard file %s checkreport:\n\nSYNTAX check failed (internal error occured)")
                               , gpp->val.storyboard_file
                                 );
  }

  /* info window of storyboard parsing report */
  /* g_message(l_msg); */
  p_print_storyboard_text_label(gpp, l_msg);

  gstb->progress = 0.0;
  g_snprintf(gstb->status_msg, sizeof(gstb->status_msg), _("ready"));
  p_status_progress(gpp, gstb);

  if(gap_debug) printf("p_storybord_job_finished:\nMSG: %s", l_msg);

  g_free(l_msg);

  p_range_widgets_set_limits(gpp
                             , gstb->first_frame_limit
                             , gstb->last_frame_nr
                             , gpp->val.input_mode
                             );


}  /* end p_storybord_job_finished */

/* -----------------------------
 * gap_cme_gui_remove_poll_timer
 * -----------------------------
 */
void
gap_cme_gui_remove_poll_timer(GapCmeGlobalParams *gpp)
{
  t_global_stb *gstb;

  gstb = &global_stb;

  if(gstb->poll_timertag >= 0)
  {
    gtk_timeout_remove(gstb->poll_timertag);
    gstb->poll_timertag = -1;
  }
}  /* end gap_cme_gui_remove_poll_timer */


/* ------------------
 * on_timer_poll
 * ------------------
 * This timer callback is called periodical
 * (The poll timer is restarted at each call with intervall of 1/4 sec)
 */
static void
on_timer_poll(gpointer   user_data)
{
  t_global_stb *gstb;
  GapCmeGlobalParams *gpp;

  if(gap_debug) printf("\non_timer_poll: START\n");

  gstb = (t_global_stb *)user_data;
  gpp = gap_cme_main_get_global_params();

  if(gstb)
  {
     gtk_timeout_remove(gstb->poll_timertag);
     if(gstb->stb_job_done)
     {
       gstb->poll_timertag = -1;
       gstb->stb_job_done = FALSE;
       p_storybord_job_finished(gpp, gstb);
     }
     else
     {
       if(gstb->poll_timertag >= 0)
       {
          /* storyboard thread job still running
          * update status, and restart poll timer
          */
          p_status_progress(gpp, gstb);

          gstb->poll_timertag = (gint32) gtk_timeout_add(500,
                                     (GtkFunction)on_timer_poll, gstb);
       }
     }
  }
}  /* end on_timer_poll */



/* ----------------------------------------
 * p_thread_storyboard_file
 * ----------------------------------------
 * p_thread_storyboard_file
 *   check if storyboard exists,
 *   if exists, set lower and upper limit of the range spinbutton widgets
 *   to 1 and the total number of frames found in the storyboard_file.
 *
 *   if no storyboard file exists then use the first/last frame_nr from the ainfo structure
 *   that describes the animation frames from the invoking image_frame.
 */
static void
p_thread_storyboard_file(void)
{
   GapCmeGlobalParams *gpp;
   t_global_stb    *gstb;
  GapGveStoryVidHandle *vidhand;
  gint32 l_first_frame_limit;
  gint32 l_last_frame_nr;
  GapGveTypeInputRange l_rangetype;

  gdouble l_aud_total_sec;

  gpp = gap_cme_main_get_global_params();
  if(gap_debug) printf("THREAD: p_thread_storyboard_file &gpp: %d\n", (int)gpp);

  gstb = &global_stb;

  p_get_range_and_type (gpp, &l_first_frame_limit, &l_last_frame_nr, &l_rangetype);



  vidhand = gap_gve_story_open_extended_video_handle
           ( FALSE   /* dont ignore video */
           , FALSE   /* dont ignore audio */
           , &gstb->progress
           , &gstb->status_msg[0]
           , sizeof(gstb->status_msg)
	   , GAP_RNGTYPE_STORYBOARD
           , NULL      /* use no imagename */
           , gpp->val.storyboard_file
           , NULL      /* use no basename */
           , NULL      /* use no extension */
           , -1        /* frame_from */
           , 999999    /* frame_to */
           , &gpp->val.storyboard_total_frames
           );

  if(vidhand)
  {
    gstb->vidhand_open_ok = TRUE;
    gap_gve_story_set_audio_resampling_program(vidhand
                       , gpp->val.util_sox
                       , gpp->val.util_sox_options
                       );

    if(vidhand->master_framerate != 0.0)
    {
        gpp->val.framerate = vidhand->master_framerate;
        gstb->framerate = vidhand->master_framerate;

    }


    gap_gve_story_calc_audio_playtime(vidhand, &l_aud_total_sec);
    gstb->aud_total_sec = l_aud_total_sec;

    if(l_aud_total_sec > 0.0)
    {
      char *l_composite_audio;

      /*if(gap_debug)*/ gap_gve_story_debug_print_audiorange_list(vidhand->aud_list, -1);


      /* l_composite_audio = p_dialog_use_storyboard_audio(gpp->val.storyboard_file); */  /* no dialog in the thread !!! */
      /* always set audio if storyboard has audio */
      l_composite_audio = g_strdup_printf("%s_composite_audio.wav", gpp->val.storyboard_file);



      gstb->composite_audio = NULL;
      if(l_composite_audio)
      {
         gstb->composite_audio = g_strdup(l_composite_audio);
         gap_gve_story_create_composite_audiofile(vidhand, l_composite_audio);
         if(g_file_test(l_composite_audio, G_FILE_TEST_EXISTS))
         {
            g_snprintf(gpp->val.audioname1, sizeof(gpp->val.audioname1), "%s"
                      , l_composite_audio);
            gpp->val.audioname_ptr = &gpp->val.audioname1[0];

            if(/* 1==1 */ gap_debug) gap_gve_story_debug_print_audiorange_list(vidhand->aud_list, -1);
            gap_gve_story_drop_audio_cache();
         }
         g_free(l_composite_audio);
      }
      gap_gve_story_remove_tmp_audiofiles(vidhand);

    }

    gstb->master_width = vidhand->master_width;
    gstb->master_height = vidhand->master_height;


    gstb->errtext = NULL;
    gstb->errline = NULL;
    gstb->warntext = NULL;
    gstb->warnline = NULL;

    if(vidhand->sterr->errtext)
    {
      gstb->errline_nr = vidhand->sterr->errline_nr;
      gstb->errtext = g_strdup(vidhand->sterr->errtext);
      gstb->errline = g_strdup(vidhand->sterr->errline);
    }
    if(vidhand->sterr->warntext)
    {
      gstb->warnline_nr = vidhand->sterr->warnline_nr;
      gstb->warntext = g_strdup(vidhand->sterr->warntext);
      gstb->warnline = g_strdup(vidhand->sterr->warnline);
    }

    if(vidhand->frn_list)
    {
        l_first_frame_limit = 1;
        l_last_frame_nr = gpp->val.storyboard_total_frames;
        l_rangetype = GAP_RNGTYPE_STORYBOARD;
    }
    gap_gve_story_close_vid_handle(vidhand);
  }

  gstb->first_frame_limit = l_first_frame_limit;
  gstb->last_frame_nr     = l_last_frame_nr;
  gpp->val.input_mode     = l_rangetype;

  if(gap_debug) printf("THREAD storyboard TERMINATING: %d\n", (int)gpp->val.gui_proc_tid);

  gstb->stb_job_done = TRUE;
  /* gpp->val.gui_proc_tid = 0; */

} /* end p_thread_storyboard_file */



/* ----------------------------------------
 * gap_cme_gui_check_storyboard_file
 * ----------------------------------------
 */
void
gap_cme_gui_check_storyboard_file(GapCmeGlobalParams *gpp)
{
   t_global_stb    *gstb;
  gint32 l_first_frame_limit;
  gint32 l_last_frame_nr;
  GapGveTypeInputRange l_rangetype;

  gstb = &global_stb;
  p_get_range_and_type (gpp, &l_first_frame_limit, &l_last_frame_nr, &l_rangetype);

  gstb->first_frame_limit = l_first_frame_limit;
  gstb->last_frame_nr     = l_last_frame_nr;
  gpp->val.input_mode     = l_rangetype;
  gstb->composite_audio   = NULL;

  p_range_widgets_set_limits(gpp
                             , l_first_frame_limit
                             , l_last_frame_nr
                             , l_rangetype
                             );

  if(gpp->ecp == NULL)
  {
    return;
  }


  if(gpp->val.storyboard_file[0] == '\0')
  {
    p_print_storyboard_text_label(gpp, NULL);
    return;
  }
  if(!g_file_test(gpp->val.storyboard_file, G_FILE_TEST_EXISTS))
  {
    p_print_storyboard_text_label(gpp, NULL);
     return;
  }



#ifdef GAP_USE_PTHREAD
  if(gap_cme_gui_check_gui_thread_is_active(gpp))  { return; }
  if(gstb->poll_timertag >= 0)           { return; }

  /* g_message(_("Go for checking storyboard file")); */
  p_print_storyboard_text_label(gpp, _("Checking Storyboard File") );

  gstb->progress = 0.0;
  g_snprintf(gstb->status_msg, sizeof(gstb->status_msg), _("Parsing Storyboardfile"));

  gstb->stb_job_done  = FALSE;

  /* start a thread for asynchron PDB call of the gui_ procedure
   */
  if(gap_debug) printf("MASTER: Before storyborad pthread_create\n");

  pthread_create(&gpp->val.gui_proc_tid, NULL, (void*)p_thread_storyboard_file, NULL);

  if(gap_debug) printf("MASTER: After storyborad pthread_create\n");

  /* start poll timer to update progress and notify when storyboard job finished */
  gstb->poll_timertag = (gint32) gtk_timeout_add(500,   /* 500 millisec */
                                  (GtkFunction)on_timer_poll, gstb);


#else
  /* if threads are not used simply call the procedure
   * (the common GUI window is not refreshed until the called gui_proc ends)
   */
  {
    static GapArrArg  argv[1];
    
    gap_arr_arg_init(&argv[0], GAP_ARR_WGT_LABEL);
    argv[0].label_txt = _("Go for checking storyboard file");
    
    if(gap_arr_ok_cancel_dialog(_("Storyboardfile Check")
                               ,_("Storyboardfile Check")
			       ,1
			       ,argv
                               ))
    {			       

      p_thread_storyboard_file();
      p_storybord_job_finished(gpp, gstb);
    }
  }
#endif


}  /* end gap_cme_gui_check_storyboard_file */



/* ----------------------------------------
 * gap_cme_gui_check_encode_OK
 * ----------------------------------------
 *
 *   check the (dialog) parameters if everything is OK
 *   this is done before we can start encoding
 *
 *   return  TRUE  .. OK
 *           FALSE .. in case of Error
 */
gboolean
gap_cme_gui_check_encode_OK (GapCmeGlobalParams *gpp)
{
  gchar       *l_msg;
  long        samplerate, samplerate2;
  long        channels;
  long        bytes_per_sample;
  long        bits, bits2;
  long        samples;
  gint         l_rc;
  gint32      tmsec;        /* audioplaytime in milli secs */

  samplerate = 0;
  samplerate2 = 0;
  bits = 16;
  bits2 = 16;

  if(gap_debug) printf("gap_cme_gui_check_encode_OK: Start\n");

  if(gpp->val.gui_proc_tid != 0 )
  {
     p_gap_message(_("Encoder specific Parameter Window is still open" ));
     return (FALSE);
  }

  if ((strcmp(gpp->val.ecp_sel.vid_enc_plugin, GAP_PLUGIN_NAME_MPG2_ENCODE) == 0)
  ||  (strcmp(gpp->val.ecp_sel.vid_enc_plugin, GAP_PLUGIN_NAME_MPG1_ENCODE) == 0))
  {
    if(((gpp->val.vid_width % 16)  != 0)
    || ((gpp->val.vid_height % 16)  != 0))
    {
      l_msg = g_strdup_printf(_("Error:\nfor MPEG width and height must be a multiple of 16\n"
                         "set Width to %d\n"
                         "set Height to %d")
                         , (int)(gpp->val.vid_width / 16) * 16
                         , (int)(gpp->val.vid_height / 16) * 16
                         );
      g_message(l_msg);
      g_free(l_msg);
      return (FALSE);
    }
  }

  if(g_file_test(gpp->val.audioname1, G_FILE_TEST_EXISTS))
  {
     tmsec = 0;
     l_rc = gap_audio_wav_file_check(gpp->val.audioname1
                     , &samplerate, &channels
                     , &bytes_per_sample, &bits, &samples);

     if(l_rc == 0)
     {
       if((bits != 8) && (bits != 16))
       {
         l_msg = g_strdup_printf(_("Error: Unsupported Bit per Sample %d\n"
                            "file: %s\n"
                            "supported are 8 or 16 Bit")
                            , (int)bits
                            , gpp->val.audioname1
                            );
         g_message(l_msg);
         g_free(l_msg);
         return (FALSE);
       }

       if((samplerate > 0) && (channels > 0))
       {
         tmsec = ((samples / channels) * 1000) / samplerate;
       }
     }
  }
  else
  {
    if(gpp->val.audioname1[0] != '\0')
    {
         l_msg = g_strdup_printf(_("Error: Audiofile not found\n"
                            "file: %s\n")
                            , gpp->val.audioname1
                            );
         g_message(l_msg);
         g_free(l_msg);
         return (FALSE);
    }
  }

  if(gpp->val.audioname1[0] != '\0')
  {
    if (strcmp(gpp->val.ecp_sel.vid_enc_plugin, GAP_PLUGIN_NAME_MPG1_ENCODE) == 0)
    {
      switch(gpp->val.samplerate)
      {
         case 22050:
         case 24000:
         case 32000:
         case 44100:
         case 48000:
             break;
         default:
             l_msg = g_strdup_printf(_("Error: Unsupported Samplerate for MPEG1 Layer2 Audio Encoding\n"
                                "current rate: %d\n"
                                "supported rates: \n"
                                " 22050, 24000, 32000, 44100, 48000")
                                , (int)gpp->val.samplerate);
             g_message(l_msg);
             g_free(l_msg);
             return (FALSE);
             break;
      }
    }


    if (strcmp(gpp->val.ecp_sel.vid_enc_plugin, GAP_PLUGIN_NAME_MPG2_ENCODE) == 0)
    {
      switch(gpp->val.samplerate)
      {
         case 8000:
         case 11025:
         case 12000:
         case 16000:
         case 22050:
         case 24000:
         case 32000:
         case 44100:
         case 48000:
             break;
         default:
             l_msg = g_strdup_printf(_("Error: Unsupported Samplerate for MPEG2 Layer3 Audio Encoding\n"
                                "current rate: %d\n"
                                "supported rates:\n"
                                " 8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000")
                                , (int)gpp->val.samplerate);
             g_message(l_msg);
             g_free(l_msg);
             return (FALSE);
             break;
      }
    }
  }

  if(g_file_test(gpp->val.videoname, G_FILE_TEST_EXISTS))
  {
    if(p_overwrite_dialog(gpp, gpp->val.videoname, 0) < 0)
    {
      return (FALSE);
    }
  }

  if(0 != gap_gve_sox_chk_and_resample(&gpp->val))
  {
     return (FALSE);
  }

  if(gap_debug) printf("gap_cme_gui_check_encode_OK: End OK\n");
  return (TRUE); /* OK */
}  /* end gap_cme_gui_check_encode_OK */


/* ----------------------------------------
 * gap_cme_gui_create_fss__fileselection
 * ----------------------------------------
 */
GtkWidget*
gap_cme_gui_create_fss__fileselection (GapCmeGlobalParams *gpp)
{
  GtkWidget *fss__fileselection;
  GtkWidget *fss__button_OK;
  GtkWidget *fss__button_cancel;

  fss__fileselection = gtk_file_selection_new (_("Select Storyboardfile"));
  gtk_container_set_border_width (GTK_CONTAINER (fss__fileselection), 10);

  fss__button_OK = GTK_FILE_SELECTION (fss__fileselection)->ok_button;
  gtk_widget_show (fss__button_OK);
  GTK_WIDGET_SET_FLAGS (fss__button_OK, GTK_CAN_DEFAULT);

  fss__button_cancel = GTK_FILE_SELECTION (fss__fileselection)->cancel_button;
  gtk_widget_show (fss__button_cancel);
  GTK_WIDGET_SET_FLAGS (fss__button_cancel, GTK_CAN_DEFAULT);

  g_signal_connect (G_OBJECT (fss__fileselection), "destroy",
                      G_CALLBACK (on_fss__fileselection_destroy),
                      gpp);
  g_signal_connect (G_OBJECT (fss__button_OK), "clicked",
                      G_CALLBACK (on_fss__button_OK_clicked),
                      gpp);
  g_signal_connect (G_OBJECT (fss__button_cancel), "clicked",
                      G_CALLBACK (on_fss__button_cancel_clicked),
                      gpp);

  return fss__fileselection;
}  /* end gap_cme_gui_create_fss__fileselection */


/* ----------------------------------------
 * gap_cme_gui_create_fsv__fileselection
 * ----------------------------------------
 */
GtkWidget*
gap_cme_gui_create_fsv__fileselection (GapCmeGlobalParams *gpp)
{
  GtkWidget *fsv__fileselection;
  GtkWidget *fsv__button_OK;
  GtkWidget *fsv__button_cancel;

  fsv__fileselection = gtk_file_selection_new (_("Select Videofile"));
  gtk_container_set_border_width (GTK_CONTAINER (fsv__fileselection), 10);

  fsv__button_OK = GTK_FILE_SELECTION (fsv__fileselection)->ok_button;
  gtk_widget_show (fsv__button_OK);
  GTK_WIDGET_SET_FLAGS (fsv__button_OK, GTK_CAN_DEFAULT);

  fsv__button_cancel = GTK_FILE_SELECTION (fsv__fileselection)->cancel_button;
  gtk_widget_show (fsv__button_cancel);
  GTK_WIDGET_SET_FLAGS (fsv__button_cancel, GTK_CAN_DEFAULT);

  g_signal_connect (G_OBJECT (fsv__fileselection), "destroy",
                      G_CALLBACK (on_fsv__fileselection_destroy),
                      gpp);
  g_signal_connect (G_OBJECT (fsv__button_OK), "clicked",
                      G_CALLBACK (on_fsv__button_OK_clicked),
                      gpp);
  g_signal_connect (G_OBJECT (fsv__button_cancel), "clicked",
                      G_CALLBACK (on_fsv__button_cancel_clicked),
                      gpp);

  gtk_widget_grab_default (fsv__button_cancel);
  return fsv__fileselection;
}  /* end gap_cme_gui_create_fsv__fileselection */


/* ----------------------------------------
 * gap_cme_gui_create_fsb__fileselection
 * ----------------------------------------
 */
GtkWidget*
gap_cme_gui_create_fsb__fileselection (GapCmeGlobalParams *gpp)
{
  GtkWidget *fsb__fileselection;
  GtkWidget *fsb__button_OK;
  GtkWidget *fsb__button_cancel;

  fsb__fileselection = gtk_file_selection_new (_("Select Macrofile"));
  gtk_container_set_border_width (GTK_CONTAINER (fsb__fileselection), 10);

  fsb__button_OK = GTK_FILE_SELECTION (fsb__fileselection)->ok_button;
  gtk_widget_show (fsb__button_OK);
  GTK_WIDGET_SET_FLAGS (fsb__button_OK, GTK_CAN_DEFAULT);

  fsb__button_cancel = GTK_FILE_SELECTION (fsb__fileselection)->cancel_button;
  gtk_widget_show (fsb__button_cancel);
  GTK_WIDGET_SET_FLAGS (fsb__button_cancel, GTK_CAN_DEFAULT);

  g_signal_connect (G_OBJECT (fsb__fileselection), "destroy",
                      G_CALLBACK (on_fsb__fileselection_destroy),
                      gpp);
  g_signal_connect (G_OBJECT (fsb__button_OK), "clicked",
                      G_CALLBACK (on_fsb__button_OK_clicked),
                      gpp);
  g_signal_connect (G_OBJECT (fsb__button_cancel), "clicked",
                      G_CALLBACK (on_fsb__button_cancel_clicked),
                      gpp);

  gtk_widget_grab_default (fsb__button_cancel);
  return fsb__fileselection;
}  /* end gap_cme_gui_create_fsb__fileselection */

/* ----------------------------------------
 * gap_cme_gui_create_fsa__fileselection
 * ----------------------------------------
 */
GtkWidget*
gap_cme_gui_create_fsa__fileselection (GapCmeGlobalParams *gpp)
{
  GtkWidget *fsa__fileselection;
  GtkWidget *fsa__button_OK;
  GtkWidget *fsa__button_cancel;

  fsa__fileselection = gtk_file_selection_new (_("Select Audiofilename"));
  gtk_container_set_border_width (GTK_CONTAINER (fsa__fileselection), 10);

  fsa__button_OK = GTK_FILE_SELECTION (fsa__fileselection)->ok_button;
  gtk_widget_show (fsa__button_OK);
  GTK_WIDGET_SET_FLAGS (fsa__button_OK, GTK_CAN_DEFAULT);

  fsa__button_cancel = GTK_FILE_SELECTION (fsa__fileselection)->cancel_button;
  gtk_widget_show (fsa__button_cancel);
  GTK_WIDGET_SET_FLAGS (fsa__button_cancel, GTK_CAN_DEFAULT);

  g_signal_connect (G_OBJECT (fsa__fileselection), "destroy",
                      G_CALLBACK (on_fsa__fileselection_destroy),
                      gpp);
  g_signal_connect (G_OBJECT (fsa__button_OK), "clicked",
                      G_CALLBACK (on_fsa__button_OK_clicked),
                      gpp);
  g_signal_connect (G_OBJECT (fsa__button_cancel), "clicked",
                      G_CALLBACK (on_fsa__button_cancel_clicked),
                      gpp);

  gtk_widget_grab_default (fsa__button_cancel);
  return fsa__fileselection;
}  /* end gap_cme_gui_create_fsa__fileselection */


/* ----------------------------------------
 * create_ow__dialog
 * ----------------------------------------
 */
static GtkWidget*
create_ow__dialog (GapCmeGlobalParams *gpp)
{
  GtkWidget *ow__dialog;
  GtkWidget *ow__dialog_vbox0;
  GtkWidget *ow__frame1;
  GtkWidget *ow__vbox1;
  GtkWidget *ow__hbox2;
  GtkWidget *ow__label1;
  GtkWidget *ow__hbox3;
  GtkWidget *ow__filename;
  GtkWidget *ow__action_area1;
  GtkWidget *ow__hbox1;
  GtkWidget *ow__label_dummy22;
  GtkWidget *ow__button_one;
  GtkWidget *ow__button_cancel;

  ow__dialog = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (ow__dialog), _("Overwrite warning"));

  ow__dialog_vbox0 = GTK_DIALOG (ow__dialog)->vbox;
  gtk_widget_show (ow__dialog_vbox0);

  ow__frame1 = gimp_frame_new (NULL);
  gtk_widget_show (ow__frame1);
  gtk_box_pack_start (GTK_BOX (ow__dialog_vbox0), ow__frame1, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (ow__frame1), 5);

  ow__vbox1 = gtk_vbox_new (FALSE, 1);
  gtk_widget_show (ow__vbox1);
  gtk_container_add (GTK_CONTAINER (ow__frame1), ow__vbox1);
  gtk_container_set_border_width (GTK_CONTAINER (ow__vbox1), 5);

  ow__hbox2 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (ow__hbox2);
  gtk_box_pack_start (GTK_BOX (ow__vbox1), ow__hbox2, FALSE, FALSE, 0);

  ow__label1 = gtk_label_new (_("File already exists:"));
  gtk_widget_show (ow__label1);
  gtk_box_pack_start (GTK_BOX (ow__hbox2), ow__label1, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (ow__label1), GTK_JUSTIFY_LEFT);

  ow__hbox3 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (ow__hbox3);
  gtk_box_pack_start (GTK_BOX (ow__vbox1), ow__hbox3, FALSE, FALSE, 0);

  ow__filename = gtk_label_new (_("filename"));
  gtk_widget_show (ow__filename);
  gtk_box_pack_start (GTK_BOX (ow__hbox3), ow__filename, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (ow__filename), GTK_JUSTIFY_LEFT);

  ow__action_area1 = GTK_DIALOG (ow__dialog)->action_area;
  gtk_widget_show (ow__action_area1);
  gtk_container_set_border_width (GTK_CONTAINER (ow__action_area1), 10);

  ow__hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (ow__hbox1);
  gtk_box_pack_start (GTK_BOX (ow__action_area1), ow__hbox1, TRUE, TRUE, 0);

  ow__label_dummy22 = gtk_label_new (_("  "));
  gtk_widget_show (ow__label_dummy22);
  gtk_box_pack_start (GTK_BOX (ow__hbox1), ow__label_dummy22, TRUE, FALSE, 0);

  ow__button_one = gtk_button_new_with_label (_("Overwrite"));
  gtk_widget_show (ow__button_one);
  gtk_box_pack_start (GTK_BOX (ow__hbox1), ow__button_one, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (ow__button_one), 2);
  GTK_WIDGET_SET_FLAGS (ow__button_one, GTK_CAN_DEFAULT);

  ow__button_cancel = gtk_button_new_with_label (_("Cancel"));
  gtk_widget_show (ow__button_cancel);
  gtk_box_pack_start (GTK_BOX (ow__hbox1), ow__button_cancel, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (ow__button_cancel), 2);
  GTK_WIDGET_SET_FLAGS (ow__button_cancel, GTK_CAN_DEFAULT);

  g_signal_connect (G_OBJECT (ow__dialog), "destroy",
                      G_CALLBACK (on_ow__dialog_destroy),
                      gpp);
  g_signal_connect (G_OBJECT (ow__button_one), "clicked",
                      G_CALLBACK (on_ow__button_one_clicked),
                      gpp);
  g_signal_connect (G_OBJECT (ow__button_cancel), "clicked",
                      G_CALLBACK (on_ow__button_cancel_clicked),
                      gpp);

  gtk_widget_grab_default (ow__button_cancel);

  gpp->ow__filename = ow__filename;

  return ow__dialog;
}  /* end create_ow__dialog */


/* ----------------------------------------
 * create_shell_window
 * ----------------------------------------
 */
static GtkWidget*
create_shell_window (GapCmeGlobalParams *gpp)
{
  GtkWidget *shell_window;
  GtkWidget *cme__dialog_vbox1;
  GtkWidget *cme__vbox_main;
  GtkWidget *cme__notebook_main;
  GtkWidget *cme__frame1;
  GtkWidget *cme__table1;
  GtkWidget *cme__label_from;
  GtkWidget *cme__label_to;
  GtkWidget *cme__label_width;
  GtkWidget *cme__label_height;
  GtkObject *cme__spinbutton_width_adj;
  GtkWidget *cme__spinbutton_width;
  GtkObject *cme__spinbutton_height_adj;
  GtkWidget *cme__spinbutton_height;
  GtkWidget *cme__optionmenu_scale;
  GtkWidget *cme__optionmenu_scale_menu;
  GtkWidget *glade_menuitem;
  GtkWidget *cme__label_framerate;
  GtkObject *cme__spinbutton_from_adj;
  GtkWidget *cme__spinbutton_from;
  GtkObject *cme__spinbutton_to_adj;
  GtkWidget *cme__spinbutton_to;
  GtkObject *cme__spinbutton_framerate_adj;
  GtkWidget *cme__spinbutton_framerate;
  GtkWidget *cme__optionmenu_framerate;
  GtkWidget *cme__optionmenu_framerate_menu;
  GtkWidget *cme__table_times;
  GtkWidget *cme__label_fromtime;
  GtkWidget *cme__label_vid_tot_text;
  GtkWidget *cme__label_totaltime;
  GtkWidget *cme__label_totime;
  GtkWidget *cme__label_aud_tot_text;
  GtkWidget *cme__label_aud0_time;
  GtkWidget *cme__label_vidnorm;
  GtkWidget *cme__optionmenu_vidnorm;
  GtkWidget *cme__optionmenu_vidnorm_menu;
  GtkWidget *cme__label_encodername;
  GtkWidget *cme__optionmenu_encodername;
  GtkWidget *cme__optionmenu_encodername_menu;
  GtkWidget *cme__button_params;
  GtkWidget *cme__short_description;
  GtkWidget *cme__label_nb1;
  GtkWidget *cme__frame2;
  GtkWidget *cme__table2;
  GtkWidget *cme__label_audio1;
  GtkWidget *cme__entry_audio1;
  GtkWidget *cme__label_aud1_time;
  GtkWidget *cme__label_aud1_info;
  GtkWidget *cme__label_samplerate;
  GtkObject *cme__spinbutton_samplerate_adj;
  GtkWidget *cme__spinbutton_samplerate;
  GtkWidget *cme__optionmenu_outsamplerate;
  GtkWidget *cme__optionmenu_outsamplerate_menu;
  GtkWidget *cme__label_aud_opt_info;
  GtkWidget *cme__label_tmp_audfile;
  GtkWidget *cme__label_tmp;
  GtkWidget *cme__label_aud_tmp_info;
  GtkWidget *cme__label_aud_tmp_time;
  GtkWidget *cme__button_gen_tmp_audfile;
  GtkWidget *cme__button_audio1;
  GtkWidget *cme__label_nb2;
  GtkWidget *cme__frame_cfg4;
  GtkWidget *qte_table_cfg4;
  GtkWidget *cme__label_sox;
  GtkWidget *cme__label_sox_options;
  GtkWidget *cme__hbox_cfg4;
  GtkWidget *cme__button_sox_save;
  GtkWidget *cme__button_sox_load;
  GtkWidget *cme__button_sox_default;
  GtkWidget *cme__entry_sox;
  GtkWidget *cme__entry_sox_options;
  GtkWidget *cme__label_sox_info;
  GtkWidget *cme__label_nb3;
  GtkWidget *cme__frame_nb4;
  GtkWidget *table1;
  GtkWidget *cme__mac_label;
  GtkWidget *cme__entry_mac;
  GtkWidget *cme__button_mac;
  GtkWidget *cme__label_stb;
  GtkWidget *cme__entry_stb;
  GtkWidget *cme__button_stb;
  GtkWidget *cme__label_debug_flat;
  GtkWidget *cme__entry_debug_flat;
  GtkWidget *cme__label_debug_multi;
  GtkWidget *cme__entry_debug_multi;
  GtkWidget *cme__label_debug_monitor;
  GtkWidget *cme__checkbutton_enc_monitor;
  GtkWidget *cme__label_storyboard_helptext;
  GtkWidget *cme__label_nb4;
  GtkWidget *cme__frame3;
  GtkWidget *cme__hbox2;
  GtkWidget *cme__label_video;
  GtkWidget *cme__entry_video;
  GtkWidget *cme__button_video;
  GtkWidget *cme__frame_status;
  GtkWidget *cme__table_status;
  GtkWidget *cme__label_status;
  GtkWidget *cme__progressbar_status;


  shell_window = gimp_dialog_new (_("Master Videoencoder"),
                         GAP_CME_PLUGIN_NAME_VID_ENCODE_MASTER,
                         NULL, 0,
                         gimp_standard_help_func, GAP_CME_PLUGIN_NAME_VID_ENCODE_MASTER ".html",

                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                         GTK_STOCK_OK,     GTK_RESPONSE_OK,
                         NULL);

  g_signal_connect (G_OBJECT (shell_window), "response",
                    G_CALLBACK (on_cme__response),
                    gpp);


  cme__dialog_vbox1 = GTK_DIALOG (shell_window)->vbox;
  gtk_widget_show (cme__dialog_vbox1);

  cme__vbox_main = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (cme__vbox_main);
  gtk_box_pack_start (GTK_BOX (cme__dialog_vbox1), cme__vbox_main, TRUE, TRUE, 0);

  cme__notebook_main = gtk_notebook_new ();
  gtk_widget_show (cme__notebook_main);
  gtk_box_pack_start (GTK_BOX (cme__vbox_main), cme__notebook_main, TRUE, TRUE, 0);

  cme__frame1 = gimp_frame_new (_("Video Encode Options"));
  gtk_widget_show (cme__frame1);
  gtk_container_add (GTK_CONTAINER (cme__notebook_main), cme__frame1);
  gtk_container_set_border_width (GTK_CONTAINER (cme__frame1), 4);

  cme__table1 = gtk_table_new (9, 3, FALSE);
  gtk_widget_show (cme__table1);
  gtk_container_add (GTK_CONTAINER (cme__frame1), cme__table1);
  gtk_container_set_border_width (GTK_CONTAINER (cme__table1), 5);
  gtk_table_set_row_spacings (GTK_TABLE (cme__table1), 2);
  gtk_table_set_col_spacings (GTK_TABLE (cme__table1), 2);

  cme__label_from = gtk_label_new (_("From Frame:"));
  gtk_widget_show (cme__label_from);
  gtk_misc_set_alignment (GTK_MISC (cme__label_from), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (cme__table1), cme__label_from, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  cme__label_to = gtk_label_new (_("To Frame:"));
  gtk_widget_show (cme__label_to);
  gtk_misc_set_alignment (GTK_MISC (cme__label_to), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (cme__table1), cme__label_to, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  cme__label_width = gtk_label_new (_("Width:"));
  gtk_widget_show (cme__label_width);
  gtk_misc_set_alignment (GTK_MISC (cme__label_width), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (cme__table1), cme__label_width, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  cme__label_height = gtk_label_new (_("Height:"));
  gtk_widget_show (cme__label_height);
  gtk_misc_set_alignment (GTK_MISC (cme__label_height), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (cme__table1), cme__label_height, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  cme__spinbutton_width_adj = gtk_adjustment_new (10, 10, 10000, 1, 10, 10);
  cme__spinbutton_width = gtk_spin_button_new (GTK_ADJUSTMENT (cme__spinbutton_width_adj), 1, 0);
  gtk_widget_show (cme__spinbutton_width);
  gtk_table_attach (GTK_TABLE (cme__table1), cme__spinbutton_width, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (cme__spinbutton_width, _("Width of the output video (pixels)"), NULL);

  cme__spinbutton_height_adj = gtk_adjustment_new (10, 10, 10000, 1, 10, 10);
  cme__spinbutton_height = gtk_spin_button_new (GTK_ADJUSTMENT (cme__spinbutton_height_adj), 1, 0);
  gtk_widget_show (cme__spinbutton_height);
  gtk_table_attach (GTK_TABLE (cme__table1), cme__spinbutton_height, 1, 2, 3, 4,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (cme__spinbutton_height, _("Height of the output video (pixels)"), NULL);

  cme__optionmenu_scale = gtk_option_menu_new ();
  gtk_widget_show (cme__optionmenu_scale);
  gtk_table_attach (GTK_TABLE (cme__table1), cme__optionmenu_scale, 2, 3, 2, 3,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_widget_set_usize (cme__optionmenu_scale, 160, -2);
  gimp_help_set_help_data (cme__optionmenu_scale, _("Scale width/height to common size"), NULL);
  cme__optionmenu_scale_menu = gtk_menu_new ();
  glade_menuitem = gtk_menu_item_new_with_label (_("Framesize (1:1)"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_cme__optionmenu_scale),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_CME_STANDARD_SIZE_IMAGE);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (cme__optionmenu_scale_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("320x240 NTSC"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_cme__optionmenu_scale),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_CME_STANDARD_SIZE_320x240);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (cme__optionmenu_scale_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("320x288 PAL"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_cme__optionmenu_scale),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_CME_STANDARD_SIZE_320x288);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (cme__optionmenu_scale_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("640x480"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_cme__optionmenu_scale),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_CME_STANDARD_SIZE_640x480);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (cme__optionmenu_scale_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("720x480 NTSC"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_cme__optionmenu_scale),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_CME_STANDARD_SIZE_720x480);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (cme__optionmenu_scale_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("720x576 PAL"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_cme__optionmenu_scale),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_CME_STANDARD_SIZE_720x576);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (cme__optionmenu_scale_menu), glade_menuitem);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (cme__optionmenu_scale), cme__optionmenu_scale_menu);

  cme__label_framerate = gtk_label_new (_("Framerate:"));
  gtk_widget_show (cme__label_framerate);
  gtk_misc_set_alignment (GTK_MISC (cme__label_framerate), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (cme__table1), cme__label_framerate, 0, 1, 4, 5,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 0);

  cme__spinbutton_from_adj = gtk_adjustment_new (1, 0, 100000, 1, 10, 10);
  cme__spinbutton_from = gtk_spin_button_new (GTK_ADJUSTMENT (cme__spinbutton_from_adj), 1, 0);
  gtk_widget_show (cme__spinbutton_from);
  gtk_table_attach (GTK_TABLE (cme__table1), cme__spinbutton_from, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (cme__spinbutton_from, _("Start encoding at this frame"), NULL);

  cme__spinbutton_to_adj = gtk_adjustment_new (1, 0, 100000, 1, 10, 10);
  cme__spinbutton_to = gtk_spin_button_new (GTK_ADJUSTMENT (cme__spinbutton_to_adj), 1, 0);
  gtk_widget_show (cme__spinbutton_to);
  gtk_table_attach (GTK_TABLE (cme__table1), cme__spinbutton_to, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (cme__spinbutton_to, _("Stop encoding at this frame"), NULL);

  cme__spinbutton_framerate_adj = gtk_adjustment_new (24, 1, 100, 0.1, 1, 10);
  cme__spinbutton_framerate = gtk_spin_button_new (GTK_ADJUSTMENT (cme__spinbutton_framerate_adj), 1, 2);
  gtk_widget_show (cme__spinbutton_framerate);
  gtk_table_attach (GTK_TABLE (cme__table1), cme__spinbutton_framerate, 1, 2, 4, 5,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (cme__spinbutton_framerate, _("Framerate of the output video (frames/sec)"), NULL);

  cme__optionmenu_framerate = gtk_option_menu_new ();
  gtk_widget_show (cme__optionmenu_framerate);
  gtk_table_attach (GTK_TABLE (cme__table1), cme__optionmenu_framerate, 2, 3, 4, 5,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_widget_set_usize (cme__optionmenu_framerate, 160, -2);
  gimp_help_set_help_data (cme__optionmenu_framerate, _("Set framerate"), NULL);
  cme__optionmenu_framerate_menu = gtk_menu_new ();
  glade_menuitem = gtk_menu_item_new_with_label (_("unchanged"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_cme__optionmenu_framerate),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_CME_STANDARD_FRAMERATE_00_UNCHANGED);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (cme__optionmenu_framerate_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("23.98"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_cme__optionmenu_framerate),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_CME_STANDARD_FRAMERATE_01_23_98);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (cme__optionmenu_framerate_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("24"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_cme__optionmenu_framerate),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_CME_STANDARD_FRAMERATE_02_24);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (cme__optionmenu_framerate_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("25"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_cme__optionmenu_framerate),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_CME_STANDARD_FRAMERATE_03_25);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (cme__optionmenu_framerate_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("29.97"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_cme__optionmenu_framerate),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_CME_STANDARD_FRAMERATE_04_29_97);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (cme__optionmenu_framerate_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("30"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_cme__optionmenu_framerate),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_CME_STANDARD_FRAMERATE_05_30);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (cme__optionmenu_framerate_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("50"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_cme__optionmenu_framerate),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_CME_STANDARD_FRAMERATE_06_50);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (cme__optionmenu_framerate_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("59.94"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_cme__optionmenu_framerate),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_CME_STANDARD_FRAMERATE_07_59_94);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (cme__optionmenu_framerate_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("60"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_cme__optionmenu_framerate),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_CME_STANDARD_FRAMERATE_08_60);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (cme__optionmenu_framerate_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("1"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_cme__optionmenu_framerate),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_CME_STANDARD_FRAMERATE_09_1);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (cme__optionmenu_framerate_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("5"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_cme__optionmenu_framerate),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_CME_STANDARD_FRAMERATE_10_5);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (cme__optionmenu_framerate_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("10"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_cme__optionmenu_framerate),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_CME_STANDARD_FRAMERATE_11_10);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (cme__optionmenu_framerate_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("12"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_cme__optionmenu_framerate),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_CME_STANDARD_FRAMERATE_12_12);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (cme__optionmenu_framerate_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("15"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_cme__optionmenu_framerate),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_CME_STANDARD_FRAMERATE_13_15);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (cme__optionmenu_framerate_menu), glade_menuitem);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (cme__optionmenu_framerate), cme__optionmenu_framerate_menu);

  cme__table_times = gtk_table_new (2, 3, FALSE);
  gtk_widget_show (cme__table_times);
  gtk_table_attach (GTK_TABLE (cme__table1), cme__table_times, 2, 3, 0, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);
  gtk_table_set_row_spacings (GTK_TABLE (cme__table_times), 10);

  cme__label_fromtime = gtk_label_new (_("00:00:000"));
  gtk_widget_show (cme__label_fromtime);
  gtk_table_attach (GTK_TABLE (cme__table_times), cme__label_fromtime, 0, 1, 0, 1,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 0);

  cme__label_vid_tot_text = gtk_label_new (_("Vidtime:"));
  gtk_widget_show (cme__label_vid_tot_text);
  gtk_table_attach (GTK_TABLE (cme__table_times), cme__label_vid_tot_text, 1, 2, 0, 1,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_padding (GTK_MISC (cme__label_vid_tot_text), 7, 0);

  cme__label_totaltime = gtk_label_new (_("00:00:000"));
  gtk_widget_show (cme__label_totaltime);
  gtk_table_attach (GTK_TABLE (cme__table_times), cme__label_totaltime, 2, 3, 0, 1,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 0);

  cme__label_totime = gtk_label_new (_("00:00:000"));
  gtk_widget_show (cme__label_totime);
  gtk_table_attach (GTK_TABLE (cme__table_times), cme__label_totime, 0, 1, 1, 2,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 0);

  cme__label_aud_tot_text = gtk_label_new (_("Audtime:"));
  gtk_widget_show (cme__label_aud_tot_text);
  gtk_table_attach (GTK_TABLE (cme__table_times), cme__label_aud_tot_text, 1, 2, 1, 2,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 0);

  cme__label_aud0_time = gtk_label_new (_("00:00:000"));
  gtk_widget_show (cme__label_aud0_time);
  gtk_table_attach (GTK_TABLE (cme__table_times), cme__label_aud0_time, 2, 3, 1, 2,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 0);

  cme__label_vidnorm = gtk_label_new (_("Videonorm:"));
  gtk_widget_show (cme__label_vidnorm);
  gtk_misc_set_alignment (GTK_MISC (cme__label_vidnorm), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (cme__table1), cme__label_vidnorm, 0, 1, 5, 6,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  cme__optionmenu_vidnorm = gtk_option_menu_new ();
  gtk_widget_show (cme__optionmenu_vidnorm);
  gtk_table_attach (GTK_TABLE (cme__table1), cme__optionmenu_vidnorm, 2, 3, 5, 6,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_widget_set_usize (cme__optionmenu_vidnorm, 160, -2);
  gimp_help_set_help_data (cme__optionmenu_vidnorm, _("Select videonorm"), NULL);
  cme__optionmenu_vidnorm_menu = gtk_menu_new ();
  glade_menuitem = gtk_menu_item_new_with_label (_("NTSC"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_cme__optionmenu_vid_norm),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_CME_STANDARD_VIDNORM_00_NTSC);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (cme__optionmenu_vidnorm_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("PAL"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_cme__optionmenu_vid_norm),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_CME_STANDARD_VIDNORM_01_PAL);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (cme__optionmenu_vidnorm_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("SECAM"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_cme__optionmenu_vid_norm),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_CME_STANDARD_VIDNORM_02_SECAM);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (cme__optionmenu_vidnorm_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("MAC"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_cme__optionmenu_vid_norm),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_CME_STANDARD_VIDNORM_03_MAC);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (cme__optionmenu_vidnorm_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("COMP"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_cme__optionmenu_vid_norm),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_CME_STANDARD_VIDNORM_04_COMP);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (cme__optionmenu_vidnorm_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("undefined"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_cme__optionmenu_vid_norm),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_CME_STANDARD_VIDNORM_05_UNDEF);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (cme__optionmenu_vidnorm_menu), glade_menuitem);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (cme__optionmenu_vidnorm), cme__optionmenu_vidnorm_menu);

  cme__label_encodername = gtk_label_new (_("Encoder:"));
  gtk_widget_show (cme__label_encodername);
  gtk_misc_set_alignment (GTK_MISC (cme__label_encodername), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (cme__table1), cme__label_encodername, 0, 1, 6, 7,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  cme__optionmenu_encodername = gtk_option_menu_new ();
  gtk_widget_show (cme__optionmenu_encodername);
  gtk_table_attach (GTK_TABLE (cme__table1), cme__optionmenu_encodername, 2, 3, 6, 7,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_widget_set_usize (cme__optionmenu_encodername, 160, -2);
  gimp_help_set_help_data (cme__optionmenu_encodername, _("Select video encoder plugin"), NULL);
  cme__optionmenu_encodername_menu = gtk_menu_new ();
  
  /* the menuitems are set later by query the PDB
   * for all registered video encoder plug-ins
   */
  glade_menuitem = gtk_menu_item_new_with_label ("DUMMY");
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (cme__optionmenu_encodername_menu), glade_menuitem);

  gtk_option_menu_set_menu (GTK_OPTION_MENU (cme__optionmenu_encodername), cme__optionmenu_encodername_menu);

  cme__button_params = gtk_button_new_with_label (_("Parameters"));
  gtk_widget_show (cme__button_params);
  gtk_table_attach (GTK_TABLE (cme__table1), cme__button_params, 1, 2, 6, 7,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (cme__button_params, _("Edit encoder specific parameters"), NULL);

  cme__short_description = gtk_label_new ("");
  gtk_widget_show (cme__short_description);
  gtk_table_attach (GTK_TABLE (cme__table1), cme__short_description, 1, 3, 7, 8,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_label_set_line_wrap (GTK_LABEL (cme__short_description), TRUE);
  gtk_misc_set_alignment (GTK_MISC (cme__short_description), 0.0, 0.5);

  cme__label_nb1 = gtk_label_new (_("Video Options"));
  gtk_widget_show (cme__label_nb1);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (cme__notebook_main), gtk_notebook_get_nth_page (GTK_NOTEBOOK (cme__notebook_main), 0), cme__label_nb1);

  cme__frame2 = gimp_frame_new (_("Audio Input"));
  gtk_widget_show (cme__frame2);
  gtk_container_add (GTK_CONTAINER (cme__notebook_main), cme__frame2);
  gtk_container_set_border_width (GTK_CONTAINER (cme__frame2), 4);

  cme__table2 = gtk_table_new (6, 3, FALSE);
  gtk_widget_show (cme__table2);
  gtk_container_add (GTK_CONTAINER (cme__frame2), cme__table2);
  gtk_table_set_row_spacings (GTK_TABLE (cme__table2), 2);
  gtk_table_set_col_spacings (GTK_TABLE (cme__table2), 2);

  cme__label_audio1 = gtk_label_new (_("Audiofile:"));
  gtk_widget_show (cme__label_audio1);
  gtk_misc_set_alignment (GTK_MISC (cme__label_audio1), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (cme__table2), cme__label_audio1, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  cme__entry_audio1 = gtk_entry_new ();
  gtk_widget_show (cme__entry_audio1);
  gtk_table_attach (GTK_TABLE (cme__table2), cme__entry_audio1, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (cme__entry_audio1, _("Name of audiofile (.wav 16 bit mono or stereo samples preferred)"), NULL);

  cme__label_aud1_time = gtk_label_new (_("00:00:000"));
  gtk_widget_show (cme__label_aud1_time);
  gtk_table_attach (GTK_TABLE (cme__table2), cme__label_aud1_time, 2, 3, 1, 2,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 0);

  cme__label_aud1_info = gtk_label_new (_("WAV, 16 Bit stereo, rate: 44100"));
  gtk_widget_show (cme__label_aud1_info);
  gtk_misc_set_alignment (GTK_MISC (cme__label_aud1_info), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (cme__table2), cme__label_aud1_info, 1, 2, 1, 2,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 0);

  cme__label_samplerate = gtk_label_new (_("Samplerate:"));
  gtk_widget_show (cme__label_samplerate);
  gtk_misc_set_alignment (GTK_MISC (cme__label_samplerate), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (cme__table2), cme__label_samplerate, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  cme__spinbutton_samplerate_adj = gtk_adjustment_new (44100, 1000, 100000, 10, 100, 1000);
  cme__spinbutton_samplerate = gtk_spin_button_new (GTK_ADJUSTMENT (cme__spinbutton_samplerate_adj), 1, 0);
  gtk_widget_show (cme__spinbutton_samplerate);
  gtk_table_attach (GTK_TABLE (cme__table2), cme__spinbutton_samplerate, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (cme__spinbutton_samplerate, _("Output samplerate in samples/sec"), NULL);

  cme__optionmenu_outsamplerate = gtk_option_menu_new ();
  gtk_widget_show (cme__optionmenu_outsamplerate);
  gtk_table_attach (GTK_TABLE (cme__table2), cme__optionmenu_outsamplerate, 2, 3, 2, 3,
                    (GtkAttachOptions) (GTK_EXPAND),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (cme__optionmenu_outsamplerate, _("Select a common used samplerate"), NULL);
  cme__optionmenu_outsamplerate_menu = gtk_menu_new ();
  glade_menuitem = gtk_menu_item_new_with_label (_(" 8k Phone"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_cme__optionmenu_outsamplerate),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_CME_STANDARD_SAMPLERATE_00_8000);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (cme__optionmenu_outsamplerate_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("11.025k"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_cme__optionmenu_outsamplerate),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_CME_STANDARD_SAMPLERATE_01_11025);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (cme__optionmenu_outsamplerate_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("12k Voice"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_cme__optionmenu_outsamplerate),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_CME_STANDARD_SAMPLERATE_02_12000);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (cme__optionmenu_outsamplerate_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("16k FM"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_cme__optionmenu_outsamplerate),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_CME_STANDARD_SAMPLERATE_03_16000);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (cme__optionmenu_outsamplerate_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("22.05k"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_cme__optionmenu_outsamplerate),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_CME_STANDARD_SAMPLERATE_04_22050);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (cme__optionmenu_outsamplerate_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("24k Tape"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_cme__optionmenu_outsamplerate),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_CME_STANDARD_SAMPLERATE_05_24000);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (cme__optionmenu_outsamplerate_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("32k HiFi"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_cme__optionmenu_outsamplerate),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_CME_STANDARD_SAMPLERATE_05_32000);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (cme__optionmenu_outsamplerate_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("44.1k CD"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_cme__optionmenu_outsamplerate),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_CME_STANDARD_SAMPLERATE_05_44100);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (cme__optionmenu_outsamplerate_menu), glade_menuitem);
  glade_menuitem = gtk_menu_item_new_with_label (_("48 k Studio"));
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_cme__optionmenu_outsamplerate),
                          (gpointer)gpp);
        g_object_set_data (G_OBJECT (glade_menuitem), GAP_GVE_MENU_ITEM_INDEX_KEY
                          , (gpointer)GAP_CME_STANDARD_SAMPLERATE_05_48000);
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (cme__optionmenu_outsamplerate_menu), glade_menuitem);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (cme__optionmenu_outsamplerate), cme__optionmenu_outsamplerate_menu);
  gtk_option_menu_set_history (GTK_OPTION_MENU (cme__optionmenu_outsamplerate), 7);

  cme__label_aud_opt_info = gtk_label_new (_("\nNote:\nif you set samplerate lower than\nrate of the WAV file, you loose sound quality,\nbut higher samplerates can not improve the\nquality of the original sound. "));
  gtk_widget_show (cme__label_aud_opt_info);
  gtk_table_attach (GTK_TABLE (cme__table2), cme__label_aud_opt_info, 1, 2, 5, 6,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_label_set_line_wrap (GTK_LABEL (cme__label_aud_opt_info), TRUE);

  cme__label_tmp_audfile = gtk_label_new ("");
  gtk_widget_show (cme__label_tmp_audfile);
  gtk_misc_set_alignment (GTK_MISC (cme__label_tmp_audfile), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (cme__table2), cme__label_tmp_audfile, 1, 2, 3, 4,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 0);

  cme__label_tmp = gtk_label_new (_("Tmpfile:"));
  gtk_widget_show (cme__label_tmp);
  gtk_misc_set_alignment (GTK_MISC (cme__label_tmp), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (cme__table2), cme__label_tmp, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 4);

  cme__label_aud_tmp_info = gtk_label_new (_("WAV, 16 Bit stereo, rate: 44100"));
  gtk_widget_show (cme__label_aud_tmp_info);
  gtk_table_attach (GTK_TABLE (cme__table2), cme__label_aud_tmp_info, 1, 2, 4, 5,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 0);

  cme__label_aud_tmp_time = gtk_label_new (_("00:00:000"));
  gtk_widget_show (cme__label_aud_tmp_time);
  gtk_table_attach (GTK_TABLE (cme__table2), cme__label_aud_tmp_time, 2, 3, 4, 5,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 0);

  cme__button_gen_tmp_audfile = gtk_button_new_with_label (_("Audioconvert"));
  gtk_widget_show (cme__button_gen_tmp_audfile);
  gtk_table_attach (GTK_TABLE (cme__table2), cme__button_gen_tmp_audfile, 2, 3, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 2, 0);
  gimp_help_set_help_data (cme__button_gen_tmp_audfile, _("Convert audiofile to tmpfile"), NULL);

  cme__button_audio1 = gtk_button_new_with_label (_("..."));
  gtk_widget_show (cme__button_audio1);
  gtk_table_attach (GTK_TABLE (cme__table2), cme__button_audio1, 2, 3, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_widget_set_usize (cme__button_audio1, 80, -2);
  gimp_help_set_help_data (cme__button_audio1, _("Select input audiofile via browser"), NULL);

  cme__label_nb2 = gtk_label_new (_("Audio Options"));
  gtk_widget_show (cme__label_nb2);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (cme__notebook_main), gtk_notebook_get_nth_page (GTK_NOTEBOOK (cme__notebook_main), 1), cme__label_nb2);

  cme__frame_cfg4 = gimp_frame_new (_("Configuration of external audiotool program"));
  gtk_widget_show (cme__frame_cfg4);
  gtk_container_add (GTK_CONTAINER (cme__notebook_main), cme__frame_cfg4);
  gtk_container_set_border_width (GTK_CONTAINER (cme__frame_cfg4), 4);

  qte_table_cfg4 = gtk_table_new (4, 2, FALSE);
  gtk_widget_show (qte_table_cfg4);
  gtk_container_add (GTK_CONTAINER (cme__frame_cfg4), qte_table_cfg4);

  cme__label_sox = gtk_label_new (_("Audiotool:"));
  gtk_widget_show (cme__label_sox);
  gtk_misc_set_alignment (GTK_MISC (cme__label_sox), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (qte_table_cfg4), cme__label_sox, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  cme__label_sox_options = gtk_label_new (_("Options:"));
  gtk_widget_show (cme__label_sox_options);
  gtk_misc_set_alignment (GTK_MISC (cme__label_sox_options), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (qte_table_cfg4), cme__label_sox_options, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  cme__hbox_cfg4 = gtk_hbox_new (TRUE, 0);
  gtk_widget_show (cme__hbox_cfg4);
  gtk_table_attach (GTK_TABLE (qte_table_cfg4), cme__hbox_cfg4, 0, 2, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND), 0, 2);

  cme__button_sox_save = gtk_button_new_with_label (_("Save"));
  gtk_widget_show (cme__button_sox_save);
  gtk_box_pack_start (GTK_BOX (cme__hbox_cfg4), cme__button_sox_save, FALSE, TRUE, 2);
  gimp_help_set_help_data (cme__button_sox_save, _("Save audiotool configuration to gimprc"), NULL);

  cme__button_sox_load = gtk_button_new_with_label (_("Load"));
  gtk_widget_show (cme__button_sox_load);
  gtk_box_pack_start (GTK_BOX (cme__hbox_cfg4), cme__button_sox_load, FALSE, TRUE, 2);
  gimp_help_set_help_data (cme__button_sox_load, _("Load audiotool configuration from gimprc"), NULL);

  cme__button_sox_default = gtk_button_new_with_label (_("Default"));
  gtk_widget_show (cme__button_sox_default);
  gtk_box_pack_start (GTK_BOX (cme__hbox_cfg4), cme__button_sox_default, FALSE, TRUE, 2);
  gimp_help_set_help_data (cme__button_sox_default, _("Set default audiotool configuration "), NULL);

  cme__entry_sox = gtk_entry_new ();
  gtk_widget_show (cme__entry_sox);
  gtk_table_attach (GTK_TABLE (qte_table_cfg4), cme__entry_sox, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 2, 2);
  gimp_help_set_help_data (cme__entry_sox, _("name of audiotool (something like sox with or without path)"), NULL);

  cme__entry_sox_options = gtk_entry_new ();
  gtk_widget_show (cme__entry_sox_options);
  gtk_table_attach (GTK_TABLE (qte_table_cfg4), cme__entry_sox_options, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 2, 2);
  gimp_help_set_help_data (cme__entry_sox_options, _("Options to call the audiotool ($IN, $OUT $RATE are replaced)"), NULL);

  cme__label_sox_info = gtk_label_new ("");
  gtk_label_parse_uline (GTK_LABEL (cme__label_sox_info),
                         _("Configuration of an audiotool (like sox on UNIX).\n\n$IN .. is replaced by the input audiofile\n$OUT .. is replaced by audiofile with suffix  _tmp.wav\n$RATE .. is replaced by samplerate in byte/sec\n\nThis tool is called for audio conversions  if\na) the input audiofile is not WAV 16Bit format\nb) Desired Samplerate does not match the\n     rate in the .wav file"));
  gtk_widget_show (cme__label_sox_info);
  gtk_table_attach (GTK_TABLE (qte_table_cfg4), cme__label_sox_info, 0, 2, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 2, 2);
  gtk_widget_set_usize (cme__label_sox_info, 300, -2);
  gtk_label_set_line_wrap (GTK_LABEL (cme__label_sox_info), TRUE);
  gtk_misc_set_alignment (GTK_MISC (cme__label_sox_info), 0.5, 0.5);

  cme__label_nb3 = gtk_label_new (_("Audio Tool Configuration"));
  gtk_widget_show (cme__label_nb3);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (cme__notebook_main), gtk_notebook_get_nth_page (GTK_NOTEBOOK (cme__notebook_main), 2), cme__label_nb3);

  cme__frame_nb4 = gimp_frame_new (_("Encoding Extras"));
  gtk_widget_show (cme__frame_nb4);
  gtk_container_add (GTK_CONTAINER (cme__notebook_main), cme__frame_nb4);
  gtk_container_set_border_width (GTK_CONTAINER (cme__frame_nb4), 4);

  table1 = gtk_table_new (6, 3, FALSE);
  gtk_widget_show (table1);
  gtk_container_add (GTK_CONTAINER (cme__frame_nb4), table1);

  cme__mac_label = gtk_label_new (_("Macrofile:"));
  gtk_widget_show (cme__mac_label);
  gtk_table_attach (GTK_TABLE (table1), cme__mac_label, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (cme__mac_label), 0.0, 0.5);

  cme__entry_mac = gtk_entry_new ();
  gtk_widget_show (cme__entry_mac);
  gtk_table_attach (GTK_TABLE (table1), cme__entry_mac, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (cme__entry_mac, _("optional filtermacro file to be performed on each handled frame "), NULL);

  cme__button_mac = gtk_button_new_with_label (_("..."));
  gtk_widget_show (cme__button_mac);
  gtk_table_attach (GTK_TABLE (table1), cme__button_mac, 2, 3, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_widget_set_usize (cme__button_mac, 70, -2);
  gimp_help_set_help_data (cme__button_mac, _("select macrofile via browser"), NULL);

  cme__label_stb = gtk_label_new (_("Storyboard File:"));
  gtk_widget_show (cme__label_stb);
  gtk_table_attach (GTK_TABLE (table1), cme__label_stb, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (cme__label_stb), 0.0, 0.5);

  cme__entry_stb = gtk_entry_new ();
  gtk_widget_show (cme__entry_stb);
  gtk_table_attach (GTK_TABLE (table1), cme__entry_stb, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (cme__entry_stb, _("optional use a storyboard file to feed the encoder"), NULL);

  cme__button_stb = gtk_button_new_with_label (_("..."));
  gtk_widget_show (cme__button_stb);
  gtk_table_attach (GTK_TABLE (table1), cme__button_stb, 2, 3, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (cme__button_stb, _("select storyboard file via browser"), NULL);

  cme__label_debug_flat = gtk_label_new (_("Debug\nFlat File:"));
  gtk_widget_show (cme__label_debug_flat);
  gtk_table_attach (GTK_TABLE (table1), cme__label_debug_flat, 0, 1, 5, 6,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (cme__label_debug_flat), 0.0, 0.5);

  cme__entry_debug_flat = gtk_entry_new ();
  gtk_widget_show (cme__entry_debug_flat);
  gtk_table_attach (GTK_TABLE (table1), cme__entry_debug_flat, 1, 2, 5, 6,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (cme__entry_debug_flat, _("optional Save each composite frame to JPEG file, before it is passed to the encoder"), NULL);

  cme__label_debug_multi = gtk_label_new (_("Debug\nMultilayer File:"));
  gtk_widget_show (cme__label_debug_multi);
  gtk_table_attach (GTK_TABLE (table1), cme__label_debug_multi, 0, 1, 4, 5,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (cme__label_debug_multi), 0.0, 0.5);

  cme__entry_debug_multi = gtk_entry_new ();
  gtk_widget_show (cme__entry_debug_multi);
  gtk_table_attach (GTK_TABLE (table1), cme__entry_debug_multi, 1, 2, 4, 5,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (cme__entry_debug_multi, _("optional save each composite multilayer frame to XJT file, before flattening and executing macro"), NULL);

  cme__label_debug_monitor = gtk_label_new (_("Monitor"));
  gtk_widget_hide (cme__label_debug_monitor);
  gtk_table_attach (GTK_TABLE (table1), cme__label_debug_monitor, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (cme__label_debug_monitor), 0.0, 0.5);

  cme__checkbutton_enc_monitor = gtk_check_button_new_with_label (_("Monitor Frames while Encoding"));
  gtk_widget_show (cme__checkbutton_enc_monitor);
  gtk_table_attach (GTK_TABLE (table1), cme__checkbutton_enc_monitor, 1, 2, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (cme__checkbutton_enc_monitor, _("Show each frame before passed to encoder"), NULL);

  cme__label_storyboard_helptext = gtk_label_new (_("Storyboardfiles are textfiles that are used to\n"
			   "assemble a video from a list of single images,\n"
			   "frameranges, videoclips, gif animations or audiofiles.\n"
			   "(see STORYBOARD_FILE_DOC.txt for details)"));
  gtk_widget_show (cme__label_storyboard_helptext);
  gtk_table_attach (GTK_TABLE (table1), cme__label_storyboard_helptext, 0, 3, 2, 3,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 2, 6);
  gtk_label_set_justify (GTK_LABEL (cme__label_storyboard_helptext), GTK_JUSTIFY_FILL);
  gtk_misc_set_alignment (GTK_MISC (cme__label_storyboard_helptext), 0.0, 0.5);

  cme__label_nb4 = gtk_label_new (_("Extras"));
  gtk_widget_show (cme__label_nb4);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (cme__notebook_main), gtk_notebook_get_nth_page (GTK_NOTEBOOK (cme__notebook_main), 3), cme__label_nb4);

  cme__frame3 = gimp_frame_new (_("Output"));
  gtk_widget_show (cme__frame3);
  gtk_box_pack_start (GTK_BOX (cme__vbox_main), cme__frame3, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (cme__frame3), 4);

  cme__hbox2 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (cme__hbox2);
  gtk_container_add (GTK_CONTAINER (cme__frame3), cme__hbox2);

  cme__label_video = gtk_label_new (_("Video :"));
  gtk_widget_show (cme__label_video);
  gtk_box_pack_start (GTK_BOX (cme__hbox2), cme__label_video, FALSE, FALSE, 2);

  cme__entry_video = gtk_entry_new ();
  gtk_widget_show (cme__entry_video);
  gtk_box_pack_start (GTK_BOX (cme__hbox2), cme__entry_video, TRUE, TRUE, 0);
  gimp_help_set_help_data (cme__entry_video, _("Name of output videofile"), NULL);

  cme__button_video = gtk_button_new_with_label (_("..."));
  gtk_widget_show (cme__button_video);
  gtk_box_pack_start (GTK_BOX (cme__hbox2), cme__button_video, FALSE, FALSE, 0);
  gtk_widget_set_usize (cme__button_video, 80, -2);
  gimp_help_set_help_data (cme__button_video, _("Select output videofile via browser"), NULL);

  cme__frame_status = gimp_frame_new (_("Status"));
  gtk_widget_show (cme__frame_status);
  gtk_box_pack_start (GTK_BOX (cme__vbox_main), cme__frame_status, TRUE, TRUE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (cme__frame_status), 4);

  cme__table_status = gtk_table_new (2, 2, FALSE);
  gtk_widget_show (cme__table_status);
  gtk_container_add (GTK_CONTAINER (cme__frame_status), cme__table_status);

  cme__label_status = gtk_label_new (_("READY"));
  gtk_widget_show (cme__label_status);
  gtk_table_attach (GTK_TABLE (cme__table_status), cme__label_status, 0, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (cme__label_status), 0.0, 0.5);
  gtk_misc_set_padding (GTK_MISC (cme__label_status), 2, 0);

  cme__progressbar_status = gtk_progress_bar_new ();
  gtk_widget_show (cme__progressbar_status);
  gtk_table_attach (GTK_TABLE (cme__table_status), cme__progressbar_status, 0, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 2, 0);

  g_signal_connect (G_OBJECT (cme__spinbutton_width), "changed",
                      G_CALLBACK (on_cme__spinbutton_width_changed),
                      gpp);
  g_signal_connect (G_OBJECT (cme__spinbutton_height), "changed",
                      G_CALLBACK (on_cme__spinbutton_height_changed),
                      gpp);
  g_signal_connect (G_OBJECT (cme__spinbutton_from), "changed",
                      G_CALLBACK (on_cme__spinbutton_from_changed),
                      gpp);
  g_signal_connect (G_OBJECT (cme__spinbutton_to), "changed",
                      G_CALLBACK (on_cme__spinbutton_to_changed),
                      gpp);
  g_signal_connect (G_OBJECT (cme__spinbutton_framerate), "changed",
                      G_CALLBACK (on_cme__spinbutton_framerate_changed),
                      gpp);
  g_signal_connect (G_OBJECT (cme__button_params), "clicked",
                      G_CALLBACK (on_cme__button_params_clicked),
                      gpp);
  g_signal_connect (G_OBJECT (cme__entry_audio1), "changed",
                      G_CALLBACK (on_cme__entry_audio1_changed),
                      gpp);
  g_signal_connect (G_OBJECT (cme__spinbutton_samplerate), "changed",
                      G_CALLBACK (on_cme__spinbutton_samplerate_changed),
                      gpp);
  g_signal_connect (G_OBJECT (cme__button_gen_tmp_audfile), "clicked",
                      G_CALLBACK (on_cme__button_gen_tmp_audfile_clicked),
                      gpp);
  g_signal_connect (G_OBJECT (cme__button_audio1), "clicked",
                      G_CALLBACK (on_cme__button_audio1_clicked),
                      gpp);
  g_signal_connect (G_OBJECT (cme__button_sox_save), "clicked",
                      G_CALLBACK (on_cme__button_sox_save_clicked),
                      gpp);
  g_signal_connect (G_OBJECT (cme__button_sox_load), "clicked",
                      G_CALLBACK (on_cme__button_sox_load_clicked),
                      gpp);
  g_signal_connect (G_OBJECT (cme__button_sox_default), "clicked",
                      G_CALLBACK (on_cme__button_sox_default_clicked),
                      gpp);
  g_signal_connect (G_OBJECT (cme__entry_sox), "changed",
                      G_CALLBACK (on_cme__entry_sox_changed),
                      gpp);
  g_signal_connect (G_OBJECT (cme__entry_sox_options), "changed",
                      G_CALLBACK (on_cme__entry_sox_options_changed),
                      gpp);
  g_signal_connect (G_OBJECT (cme__entry_mac), "changed",
                      G_CALLBACK (on_cme__entry_mac_changed),
                      gpp);
  g_signal_connect (G_OBJECT (cme__button_mac), "clicked",
                      G_CALLBACK (on_cme__button_mac_clicked),
                      gpp);
  g_signal_connect (G_OBJECT (cme__entry_stb), "changed",
                      G_CALLBACK (on_cme__entry_stb_changed),
                      gpp);
  g_signal_connect (G_OBJECT (cme__button_stb), "clicked",
                      G_CALLBACK (on_cme__button_stb_clicked),
                      gpp);
  g_signal_connect (G_OBJECT (cme__entry_debug_flat), "changed",
                      G_CALLBACK (on_cme__entry_debug_flat_changed),
                      gpp);
  g_signal_connect (G_OBJECT (cme__entry_debug_multi), "changed",
                      G_CALLBACK (on_cme__entry_debug_multi_changed),
                      gpp);
  g_signal_connect (G_OBJECT (cme__checkbutton_enc_monitor), "toggled",
                      G_CALLBACK (on_cme__checkbutton_enc_monitor_toggled),
                      gpp);
  g_signal_connect (G_OBJECT (cme__entry_video), "changed",
                      G_CALLBACK (on_cme__entry_video_changed),
                      gpp);
  g_signal_connect (G_OBJECT (cme__button_video), "clicked",
                      G_CALLBACK (on_cme__button_video_clicked),
                      gpp);

  gpp->cme__button_gen_tmp_audfile      = cme__button_gen_tmp_audfile;
  gpp->cme__button_params               = cme__button_params;
  gpp->cme__entry_audio1                = cme__entry_audio1;
  gpp->cme__entry_debug_flat            = cme__entry_debug_flat;
  gpp->cme__entry_debug_multi           = cme__entry_debug_multi;
  gpp->cme__entry_mac                   = cme__entry_mac;
  gpp->cme__entry_sox                   = cme__entry_sox;
  gpp->cme__entry_sox_options           = cme__entry_sox_options;
  gpp->cme__entry_stb                   = cme__entry_stb;
  gpp->cme__entry_video                 = cme__entry_video;
  gpp->cme__label_aud0_time             = cme__label_aud0_time;
  gpp->cme__label_aud1_info             = cme__label_aud1_info;
  gpp->cme__label_aud1_time             = cme__label_aud1_time;
  gpp->cme__label_aud_tmp_info          = cme__label_aud_tmp_info;
  gpp->cme__label_from                  = cme__label_from;
  gpp->cme__label_fromtime              = cme__label_fromtime;
  gpp->cme__label_status                = cme__label_status;
  gpp->cme__label_storyboard_helptext   = cme__label_storyboard_helptext;
  gpp->cme__label_to                    = cme__label_to;
  gpp->cme__label_totaltime             = cme__label_totaltime;
  gpp->cme__label_totime                = cme__label_totime;
  gpp->cme__optionmenu_encodername      = cme__optionmenu_encodername;
  gpp->cme__optionmenu_framerate        = cme__optionmenu_framerate;
  gpp->cme__optionmenu_outsamplerate    = cme__optionmenu_outsamplerate;
  gpp->cme__optionmenu_scale            = cme__optionmenu_scale;
  gpp->cme__progressbar_status          = cme__progressbar_status;
  gpp->cme__short_description           = cme__short_description;
  gpp->cme__spinbutton_framerate        = cme__spinbutton_framerate;
  gpp->cme__spinbutton_from             = cme__spinbutton_from;
  gpp->cme__spinbutton_height           = cme__spinbutton_height;
  gpp->cme__spinbutton_samplerate       = cme__spinbutton_samplerate;
  gpp->cme__spinbutton_to               = cme__spinbutton_to;
  gpp->cme__spinbutton_width            = cme__spinbutton_width;
  gpp->cme__label_aud_tmp_time          = cme__label_aud_tmp_time;
  gpp->cme__label_tmp_audfile           = cme__label_tmp_audfile;

  gpp->cme__spinbutton_width_adj        = cme__spinbutton_width_adj;
  gpp->cme__spinbutton_height_adj       = cme__spinbutton_height_adj;
  gpp->cme__spinbutton_from_adj         = cme__spinbutton_from_adj;
  gpp->cme__spinbutton_to_adj           = cme__spinbutton_to_adj;
  gpp->cme__spinbutton_framerate_adj    = cme__spinbutton_framerate_adj;
  gpp->cme__spinbutton_samplerate_adj   = cme__spinbutton_samplerate_adj;

  return shell_window;
}  /* end create_shell_window */


/* ----------------------------------------
 * p_overwrite_dialog
 * ----------------------------------------
 */
static gint
p_overwrite_dialog(GapCmeGlobalParams *gpp, gchar *filename, gint overwrite_mode)
{
  if(g_file_test(filename, G_FILE_TEST_EXISTS))
  {
    if (overwrite_mode < 1)
    {
       if(gpp->val.run_mode != GIMP_RUN_NONINTERACTIVE)
       {
         gpp->val.ow_mode = -1; /* prepare Cancel as default */
         gpp->ow__dialog_window = create_ow__dialog (gpp);

         gtk_label_set_text(GTK_LABEL(gpp->ow__filename), filename);
         gtk_widget_show (gpp->ow__dialog_window);
         gtk_main ();
         /* set ow_mode 0: Overwrite One
          *             1: Overwrite All
          *            -1: Cancel
          */
         gpp->ow__dialog_window = NULL;
       }
       return(gpp->val.ow_mode);
    }
  }
  return (overwrite_mode);
}  /* end p_overwrite_dialog */


/* ----------------------------------------
 * gap_cme_gui_master_encoder_dialog
 * ----------------------------------------
 * common GUI dialog
 *
 *   return  0 .. OK
 *          -1 .. in case of Error or cancel
 */
gint32
gap_cme_gui_master_encoder_dialog(GapCmeGlobalParams *gpp)
{
  t_global_stb    *gstb;

  if(gap_debug) printf("gap_cme_gui_master_encoder_dialog: Start\n");

  gstb = &global_stb;
  gstb->stb_job_done = FALSE;
  gstb->poll_timertag = -1;

  gimp_ui_init ("gap_video_extract", FALSE);
  gap_stock_init();


  /* debug: disable debug save of encoded frames */
  gimp_set_data( GAP_VID_ENC_SAVE_MULTILAYER, "\0\0", 2);
  gimp_set_data( GAP_VID_ENC_SAVE_FLAT, "\0\0", 2);
  gimp_set_data( GAP_VID_ENC_MONITOR, "FALSE", strlen("FALSE") +1);


  /* ---------- dialog ----------*/
  gpp->fsv__fileselection = NULL;
  gpp->fsb__fileselection = NULL;
  gpp->fsa__fileselection = NULL;
  gpp->fss__fileselection = NULL;
  gpp->ow__dialog_window = NULL;
  gpp->ecp = pdb_find_video_encoders();

  if(gap_debug) printf("gap_cme_gui_master_encoder_dialog: Before create_shell_window\n");

  gpp->shell_window = create_shell_window (gpp);

  if(gap_debug) printf("gap_cme_gui_master_encoder_dialog: After create_shell_window\n");

  p_init_shell_window_widgets(gpp);
  gtk_widget_show (gpp->shell_window);

  gpp->val.run = 0;
  gtk_main ();

  if(gap_debug) printf("A F T E R gtk_main run:%d\n", (int)gpp->val.run);

  gpp->shell_window = NULL;

  if(gpp->val.run)
  {
    return 0;
  }
  return -1;
}  /* end gap_cme_gui_master_encoder_dialog */
