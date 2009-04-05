/*  gap_morph_one_tween_dialog.c
 *
 *  This module handles the GAP morph tween dialog
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

#include "config.h"

#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gap_morph_main.h"
#include "gap_morph_tween_dialog.h"
#include "gap-intl.h"

extern int gap_debug;  /* 1 == print debug infos , 0 dont print debug infos */

#define SCALE_WIDTH  200
#define SPIN_BUTTON_WIDTH 60

typedef struct MorphTweenGui { /* nickname: mtg */
     GapMorphGlobalParams *mgpp;
     GtkWidget *morph_filesel;
     GtkWidget *morph_entry;
  } MorphTweenGui;
  
  
/* ----------------------------
 * p_selectionConstraintFunc
 * ----------------------------
 *
 */
static gint
p_selectionConstraintFunc (gint32   image_id,
               gint32   drawable_id,
               gpointer data)
{
  if (image_id < 0)
    return FALSE;

  /* dont accept layers from indexed images */
  if (gimp_drawable_is_indexed (drawable_id))
    return FALSE;

  return TRUE;
}  /* end p_selectionConstraintFunc */

/* -----------------------------
 * p_layerSelectionComboCallback
 * -----------------------------
 *
 */
static void
p_layerSelectionComboCallback (GtkWidget *widget)
{
  gint idValue;
  gint32 *layerIdPtr;

  if(gap_debug)
  {
    printf("p_layerSelectionComboCallback START\n");
  }

  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget), &idValue);
  layerIdPtr = g_object_get_data (G_OBJECT (widget), "layerIdPtr");

  if(gap_debug)
  {
    printf("p_layerSelectionComboCallback idValue:%d\n", idValue);
  }
  
  if (layerIdPtr)
  {
    *layerIdPtr = idValue;
  }

}  /* end p_layerSelectionComboCallback */




/* --------------------------
 * FILESEL
 * --------------------------
 */
static void
p_filesel_close_cb(GtkWidget *widget, MorphTweenGui *mtg)
{
  if(mtg->morph_filesel == NULL) return;  /* filesel is already closed */

  gtk_widget_destroy(GTK_WIDGET(mtg->morph_filesel));
  mtg->morph_filesel = NULL;
}

static void
p_filesel_ok_cb(GtkWidget *widget, MorphTweenGui *mtg)
{
  const gchar *filename;

  if(mtg->morph_filesel == NULL) return;  /* filesel is already closed */

  filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (mtg->morph_filesel));

  gtk_entry_set_text(GTK_ENTRY(mtg->morph_entry), filename);

  p_filesel_close_cb(widget, mtg);
}

/* --------------------------------------
 * p_filesel_open_cb
 * --------------------------------------
 */
static void
p_filesel_open_cb(GtkWidget *widget, MorphTweenGui *mtg)
{
  GtkWidget *filesel;
  GapMorphGlobalParams *mgpp;

  if(mtg->morph_filesel != NULL)
  {
    gtk_window_present(GTK_WINDOW(mtg->morph_filesel));
    return;  /* filesel is already open */
  }
  
  filesel = gtk_file_selection_new (_("Enter Morph Workpoint filename"));
  mtg->morph_filesel = filesel;
  mgpp =  mtg->mgpp;

  gtk_window_set_position (GTK_WINDOW (filesel), GTK_WIN_POS_MOUSE);

  gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesel),
                                   &mgpp->workpoint_file_lower[0]);
  gtk_widget_show (filesel);

  g_signal_connect (G_OBJECT (filesel), "destroy",
                    G_CALLBACK (p_filesel_close_cb),
                    mtg);

  g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button),
                   "clicked",
                    G_CALLBACK (p_filesel_ok_cb),
                    mtg);
  g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (filesel)->cancel_button),
                   "clicked",
                    G_CALLBACK (p_filesel_close_cb),
                    mtg);
}

/* -----------------------------------------------------
 * p_check_workpoint_file_and_use_single_fade_if_missing
 * -----------------------------------------------------
 */
static void
p_check_workpoint_file_and_use_single_fade_if_missing(GapMorphGlobalParams *mgpp)
{
   if(g_file_test(&mgpp->workpoint_file_lower[0], G_FILE_TEST_EXISTS))
   {
     mgpp->do_simple_fade     = FALSE;  /* use morph algortihm when workpoint file is available */
   }
   else
   {
     mgpp->do_simple_fade     = TRUE;  /* use simple fade because we have no workpoint file */
   }
}

/* --------------------------------------
 * p_morph_workpoint_entry_update_cb
 * --------------------------------------
 */
static void
p_morph_workpoint_entry_update_cb(GtkWidget *widget, GapMorphGlobalParams *mgpp)
{
   g_snprintf(&mgpp->workpoint_file_lower[0]
           , sizeof(mgpp->workpoint_file_lower)
           , "%s"
           , gtk_entry_get_text(GTK_ENTRY(widget))
           );
   g_snprintf(&mgpp->workpoint_file_upper[0]
           , sizeof(mgpp->workpoint_file_upper)
           , "%s"
           , gtk_entry_get_text(GTK_ENTRY(widget))
           );
   if(gap_debug)
   {
     printf("p_morph_workpoint_entry_update_cb  workpoint_file_lower: %s\n"
       , &mgpp->workpoint_file_lower[0]);
   }

   p_check_workpoint_file_and_use_single_fade_if_missing(mgpp);
   
}  /* end p_morph_workpoint_entry_update_cb */



/* --------------------------------------
 * p_create_morph_workpoint_entry
 * --------------------------------------
 */
static void
p_create_morph_workpoint_entry(GapMorphGlobalParams *mgpp, MorphTweenGui *mtg, gint row, GtkWidget *table)
{
  GtkWidget *entry;
  GtkWidget *label;
  GtkWidget *button;

  /* morph workpoint entry */
  label = gtk_label_new (_("Morph Workpoint file:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, row, row + 1,
                    GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show (label);

  entry = gtk_entry_new();
  mtg->morph_entry = entry;
  gtk_widget_set_size_request(entry, SCALE_WIDTH, -1);
  if(mgpp->workpoint_file_lower[0] != '\0')
  {
    gtk_entry_set_text(GTK_ENTRY(entry), &mgpp->workpoint_file_lower[0]);
  }
  gtk_table_attach(GTK_TABLE(table), entry, 1, 2, row, row + 1, GTK_FILL, GTK_FILL | GTK_EXPAND, 4, 0);
  gimp_help_set_help_data(entry, _("Name of a Workpointfile created with the Morph feature\n"
                                   "(note that tweens are created via simple fade operations when no workpointfile is available))"), NULL);
  gtk_widget_show(entry);

  g_signal_connect(G_OBJECT(entry), "changed",
                   G_CALLBACK (p_morph_workpoint_entry_update_cb),
                   mgpp);
    
    
  /* Button  to invoke filebrowser */  
  button = gtk_button_new_with_label ( "..." );
  gtk_widget_set_size_request(button, SPIN_BUTTON_WIDTH, -1);
  g_object_set_data (G_OBJECT (button), "mgpp", (gpointer)mgpp);
  gtk_table_attach( GTK_TABLE(table), button, 2, 3, row, row +1,
                    0, 0, 0, 0 );
  gtk_widget_show (button);
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (p_filesel_open_cb),
                    mtg);
}  /* end p_create_morph_workpoint_entry */

/* --------------------------------------
 * gap_morph_one_tween_dialog
 * --------------------------------------
 */
gboolean
gap_morph_one_tween_dialog(GapMorphGlobalParams *mgpp)
{
  MorphTweenGui  morphTweenGui;
  MorphTweenGui  *mtg;
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *label;
  GtkWidget *table;
  GtkWidget *combo;
  GtkObject *adj;
  gint       row;
  gboolean   run;

  mtg = &morphTweenGui;
  mtg->mgpp = mgpp;
  mtg->morph_filesel = NULL;
  p_check_workpoint_file_and_use_single_fade_if_missing(mgpp);

  gimp_ui_init (GAP_MORPH_TWEEN_PLUGIN_NAME, TRUE);

  dialog = gimp_dialog_new (_("Create one tween"), GAP_MORPH_TWEEN_PLUGIN_NAME,
                            NULL, 0,
                            gimp_standard_help_func, GAP_MORPH_TWEEN_HELP_ID,

                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_OK,     GTK_RESPONSE_OK,

                            NULL);


  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), main_vbox);
  gtk_widget_show (main_vbox);


  /* Controls */
  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  row = 0;

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row,
                              _("tween mix:"), SCALE_WIDTH, 7,
                              mgpp->tween_mix_factor, 0.0, 1.0, 0.1, 0.1, 3,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &mgpp->tween_mix_factor);

  row++;

  /* layer combo_box (source) */
  label = gtk_label_new (_("Source Layer:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, row, row + 1,
                    GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show (label);

  /* layer combo_box (Sample from where to pick the alternative selection */
  combo = gimp_layer_combo_box_new (p_selectionConstraintFunc, NULL);

  g_object_set_data (G_OBJECT (combo), "layerIdPtr", &mgpp->osrc_layer_id);
  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo), mgpp->osrc_layer_id,
                              G_CALLBACK (p_layerSelectionComboCallback),
                              NULL);

  gtk_table_attach (GTK_TABLE (table), combo, 1, 3, row, row + 1,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (combo);

  row++;

  /* layer combo_box (source) */
  label = gtk_label_new (_("Destination Layer:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, row, row + 1,
                    GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show (label);

  /* layer combo_box (Sample from where to pick the alternative selection */
  combo = gimp_layer_combo_box_new (p_selectionConstraintFunc, NULL);

  g_object_set_data (G_OBJECT (combo), "layerIdPtr", &mgpp->fdst_layer_id);
  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo), mgpp->fdst_layer_id,
                              G_CALLBACK (p_layerSelectionComboCallback),
                              NULL);

  gtk_table_attach (GTK_TABLE (table), combo, 1, 3, row, row + 1,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (combo);

  row++;

  p_create_morph_workpoint_entry(mgpp, mtg, row, table);

  /* Done */

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
} /* end gap_morph_one_tween_dialog */


/* --------------------------------------
 * gap_morph_frame_tweens_dialog
 * --------------------------------------
 */
gboolean
gap_morph_frame_tweens_dialog(GapAnimInfo *ainfo_ptr, GapMorphGlobalParams *mgpp)
{
  MorphTweenGui  morphTweenGui;
  MorphTweenGui  *mtg;
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *table;
  GtkObject *adj;
  GtkWidget *scale;
  GtkWidget *spinbutton;
  GtkWidget *label;
  gint       row;
  gboolean   run;
  gboolean   isFrameMissing;

  mgpp->range_from = ainfo_ptr->curr_frame_nr;
  mgpp->range_to = ainfo_ptr->frame_nr_after_curr_frame_nr;
  mtg = &morphTweenGui;
  mtg->mgpp = mgpp;
  mtg->morph_filesel = NULL;
  isFrameMissing = FALSE;
  p_check_workpoint_file_and_use_single_fade_if_missing(mgpp);


  if(gap_debug)
  {
    printf("gap_morph_frame_tweens_dialog: mgpp->range_from%d  mgpp->range_to:%d  first:%d, curr:%d, last:%d after_curr:%d\n"
      , (int)mgpp->range_from
      , (int)mgpp->range_to
      , (int)ainfo_ptr->first_frame_nr
      , (int)ainfo_ptr->curr_frame_nr
      , (int)ainfo_ptr->last_frame_nr
      , (int)ainfo_ptr->frame_nr_after_curr_frame_nr
      );
  }

  isFrameMissing = (ainfo_ptr->curr_frame_nr +1) != ainfo_ptr->frame_nr_after_curr_frame_nr;

  gimp_ui_init (GAP_MORPH_TWEEN_PLUGIN_NAME, TRUE);

  dialog = gimp_dialog_new (_("Create Tween Frames"), GAP_MORPH_TWEEN_PLUGIN_NAME,
                            NULL, 0,
                            gimp_standard_help_func, GAP_MORPH_TWEEN_HELP_ID,

                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_OK,     GTK_RESPONSE_OK,

                            NULL);


  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), main_vbox);
  gtk_widget_show (main_vbox);


  /* Controls */
  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  row = 0;
  /* morph workpoint entry */
  label = gtk_label_new (_("Information:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, row, row + 1,
                    GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show (label);

  if (isFrameMissing)
  {
    char *msg;
    msg = g_strdup_printf(_("this operation creates %d mising frames between frame %d and %d")
                         ,(int)(mgpp->range_to - mgpp->range_from) -1
                         ,(int)mgpp->range_from
                         ,(int)mgpp->range_to
                         );
    label = gtk_label_new (msg);
    g_free(msg);
    mgpp->overwrite_flag = FALSE;
  }
  else
  {
    label = gtk_label_new (_("WARNING this operation will overwrite all frames between the specified frame range"));
    mgpp->overwrite_flag = TRUE;
  }
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 1, 3, row, row + 1,
                    GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show (label);

  row++;

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row,
                              _("From:"), SCALE_WIDTH, 7,
                              mgpp->range_from, ainfo_ptr->first_frame_nr, ainfo_ptr->last_frame_nr, 1.0, 10.0, 0,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &mgpp->range_from);

  scale = g_object_get_data(G_OBJECT (adj), "scale");
  spinbutton = g_object_get_data(G_OBJECT (adj), "spinbutton");
  gtk_widget_set_sensitive(scale, FALSE);
  gtk_widget_set_sensitive(spinbutton, FALSE);

  row++;

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row,
                              _("To:"), SCALE_WIDTH, 7,
                              mgpp->range_to, mgpp->range_from +1, ainfo_ptr->last_frame_nr, 1.0, 10.0, 0,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &mgpp->range_to);
  if (isFrameMissing)
  {
    scale = g_object_get_data(G_OBJECT (adj), "scale");
    spinbutton = g_object_get_data(G_OBJECT (adj), "spinbutton");
    gtk_widget_set_sensitive(scale, FALSE);
    gtk_widget_set_sensitive(spinbutton, FALSE);
  }

  row++;

  p_create_morph_workpoint_entry(mgpp, mtg, row, table);
  
  /* Done */

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
} /* end gap_morph_frame_tweens_dialog */

