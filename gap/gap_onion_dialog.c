/* gap_onion_dialog.c
 * 2003.05.22 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * This Module contains GAP Onionskin GUI Procedures
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
 * version 1.3.14a; 2003.05.24   hof: created
 */ 

#include "config.h"
 
/* SYTEM (UNIX) includes */ 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* GIMP includes */
#include "gtk/gtk.h"
#include "libgimp/gimp.h"

/* GAP includes */
#include "gap_lib.h"
#include "gap_match.h"
#include "gap_onion_main.h"
#include "gap_onion_dialog.h"
#include "gap_onion_worker.h"

#include "gap-intl.h"


#define ENC_MENU_ITEM_INDEX_KEY "gap_enc_menu_item_index"
#define MAX_SELECT_MODE_ARRAY_ELEMENTS 7


extern int gap_debug;

gint  gtab_select_modes[MAX_SELECT_MODE_ARRAY_ELEMENTS] =  { 0, 1, 2, 3, 4, 5, 6 };


static void 
p_init_main_dialog_widgets(t_global_params *gpp);


/* ----------------------------
 * Declare CALLBACK Procedures
 * ----------------------------
 */
static void
on_oni__spinbutton_range_from_changed  (GtkEditable     *editable,
                                        gpointer         user_data);

static void
on_oni__spinbutton_range_to_changed    (GtkEditable     *editable,
                                        gpointer         user_data);

static void
on_oni__spinbutton_num_olayers_changed (GtkEditable     *editable,
                                        gpointer         user_data);

static void
on_oni__spinbutton_ref_delta_changed   (GtkEditable     *editable,
                                        gpointer         user_data);

static void
on_oni__checkbutton_ref_cycle_toggled (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

static void
on_oni__spinbutton_stack_pos_changed   (GtkEditable     *editable,
                                        gpointer         user_data);

static void
on_oni__checkbutton_stack_top_toggled  (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

static void
on_oni__spinbutton_opacity_changed     (GtkEditable     *editable,
                                        gpointer         user_data);

static void
on_oni__spinbutton_opacity_delta_changed
                                        (GtkEditable     *editable,
                                        gpointer         user_data);

static void
on_oni__spinbutton_ignore_botlayers_changed
                                        (GtkEditable     *editable,
                                        gpointer         user_data);

static void
on_oni__entry_select_string_changed    (GtkEditable     *editable,
                                        gpointer         user_data);

static void
on_oni__checkbutton_select_case_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

static void
on_oni__checkbutton_select_invert_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

static void
on_oni__button_default_clicked         (GtkButton       *button,
                                        gpointer         user_data);

static void
on_oni__button_cancel_clicked          (GtkButton       *button,
                                        gpointer         user_data);

static void
on_oni__button_set_clicked             (GtkButton       *button,
                                        gpointer         user_data);

static void
on_oni__button_delete_clicked          (GtkButton       *button,
                                        gpointer         user_data);

static void
on_oni__button_apply_clicked           (GtkButton       *button,
                                        gpointer         user_data);

static void
on_oni__dialog_destroy                 (GtkObject       *object,
                                        gpointer         user_data);






/* ------------------------------------------------------
 * CALLBACK Procedures Implementation 
 * ------------------------------------------------------
 */


static void
p_init_sensitive(t_global_params *gpp)
{
  GtkWidget *wgt;
  gboolean  l_sensitive;

  l_sensitive = TRUE;
  if(gpp->val.select_mode > 3)
  {
    /* insensitive for select modes (4,5,6) that are lists of stacknumbers or all_visible layers */
    l_sensitive = FALSE;
  }
  wgt = gpp->oni__checkbutton_select_case;
  gtk_widget_set_sensitive(wgt, l_sensitive);
  wgt = gpp->oni__checkbutton_select_invert;
  gtk_widget_set_sensitive(wgt, l_sensitive);

  l_sensitive = TRUE;
  if(gpp->val.select_mode == 6)
  {
     /* the pattern entry is insensitive if all_visible layers (6) is selected */
     l_sensitive = FALSE;
  }
  wgt = gpp->oni__entry_select_string;
  gtk_widget_set_sensitive(wgt, l_sensitive);
}


static void
on_oni__optionmenu_select_mode (GtkWidget     *wgt_item,
                           gpointer       user_data)
{
  t_global_params *gpp;
  gint       l_idx;

 gpp = (t_global_params*)user_data;
 if(gap_debug) printf("CB: on_oni__optionmenu_select_mode\n");

 if(gpp == NULL) return;

 l_idx = (gint) gtk_object_get_data (GTK_OBJECT (wgt_item), ENC_MENU_ITEM_INDEX_KEY);

 if(gap_debug) printf("CB: on_oni__optionmenu_select_mode index: %d\n", (int)l_idx);
 if((l_idx >= MAX_SELECT_MODE_ARRAY_ELEMENTS) || (l_idx < 1))
 {
    l_idx = 0;
 }
 gpp->val.select_mode = gtab_select_modes[l_idx];
 p_init_sensitive(gpp);
}



static void
on_oni__spinbutton_range_from_changed  (GtkEditable     *editable,
                                        gpointer         user_data)
{
 t_global_params *gpp;
  GtkWidget *wgt;
  GtkAdjustment *adj;

 gpp = (t_global_params*)user_data;
 if(gap_debug) printf("CB: on_oni__spinbutton_range_from_changed\n");

 if(gpp == NULL) return;
 wgt = gpp->oni__spinbutton_range_from;
 if(wgt)
 {
   adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(wgt));

   if(gap_debug) printf("spin value: %f\n", (float)adj->value );

   if((gdouble)adj->value != gpp->range_from)
   {
     gpp->range_from = (gdouble)adj->value;
   }
 }
}


static void
on_oni__spinbutton_range_to_changed    (GtkEditable     *editable,
                                        gpointer         user_data)
{
 t_global_params *gpp;
  GtkWidget *wgt;
  GtkAdjustment *adj;

 gpp = (t_global_params*)user_data;
 if(gap_debug) printf("CB: on_oni__spinbutton_range_to_changed\n");

 if(gpp == NULL) return;
 wgt = gpp->oni__spinbutton_range_to;
 if(wgt)
 {
   adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(wgt));

   if(gap_debug) printf("spin value: %f\n", (float)adj->value );

   if((gdouble)adj->value != gpp->range_to)
   {
     gpp->range_to = (gdouble)adj->value;
   }
 }
}


static void
on_oni__spinbutton_num_olayers_changed (GtkEditable     *editable,
                                        gpointer         user_data)
{
 t_global_params *gpp;
  GtkWidget *wgt;
  GtkAdjustment *adj;

 gpp = (t_global_params*)user_data;
 if(gap_debug) printf("CB: on_oni__spinbutton_num_olayers_changed\n");

 if(gpp == NULL) return;
 wgt = gpp->oni__spinbutton_num_olayers;
 if(wgt)
 {
   adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(wgt));

   if(gap_debug) printf("spin value: %f\n", (float)adj->value );

   if((gdouble)adj->value != gpp->val.num_olayers)
   {
     gpp->val.num_olayers = (gdouble)adj->value;
   }
 }
}


static void
on_oni__spinbutton_ref_delta_changed   (GtkEditable     *editable,
                                        gpointer         user_data)
{
 t_global_params *gpp;
  GtkWidget *wgt;
  GtkAdjustment *adj;

 gpp = (t_global_params*)user_data;
 if(gap_debug) printf("CB: on_oni__spinbutton_ref_delta_changed\n");

 if(gpp == NULL) return;
 wgt = gpp->oni__spinbutton_ref_delta;
 if(wgt)
 {
   adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(wgt));

   if(gap_debug) printf("spin value: %f\n", (float)adj->value );

   if((gdouble)adj->value != gpp->val.ref_delta)
   {
     gpp->val.ref_delta = (gdouble)adj->value;
   }
 }
}


static void
on_oni__checkbutton_ref_cycle_toggled (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
 t_global_params *gpp;
 gpp = (t_global_params*)user_data;
 if(gap_debug) printf("CB: on_oni__checkbutton_ref_cycle_toggled\n");

 if(gpp)
 {
    if (togglebutton->active)
    {
       gpp->val.ref_cycle = TRUE;
    }
    else
    {
       gpp->val.ref_cycle = FALSE;
    }
 }
}


static void
on_oni__spinbutton_stack_pos_changed   (GtkEditable     *editable,
                                        gpointer         user_data)
{
 t_global_params *gpp;
  GtkWidget *wgt;
  GtkAdjustment *adj;

 gpp = (t_global_params*)user_data;
 if(gap_debug) printf("CB: on_oni__spinbutton_stack_pos_changed\n");

 if(gpp == NULL) return;
 wgt = gpp->oni__spinbutton_stack_pos;
 if(wgt)
 {
   adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(wgt));

   if(gap_debug) printf("spin value: %f\n", (float)adj->value );

   if((gdouble)adj->value != gpp->val.stack_pos)
   {
     gpp->val.stack_pos = (gdouble)adj->value;
   }
 }
}


static void
on_oni__checkbutton_stack_top_toggled  (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
 t_global_params *gpp;
 gpp = (t_global_params*)user_data;
 if(gap_debug) printf("CB: on_oni__checkbutton_stack_top_toggled\n");

 if(gpp)
 {
    if (togglebutton->active)
    {
       gpp->val.stack_top = TRUE;
    }
    else
    {
       gpp->val.stack_top = FALSE;
    }
 }
}


static void
on_oni__spinbutton_opacity_changed     (GtkEditable     *editable,
                                        gpointer         user_data)
{
 t_global_params *gpp;
  GtkWidget *wgt;
  GtkAdjustment *adj;

 gpp = (t_global_params*)user_data;
 if(gap_debug) printf("CB: on_oni__spinbutton_opacity_changed\n");

 if(gpp == NULL) return;
 wgt = gpp->oni__spinbutton_opacity;
 if(wgt)
 {
   adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(wgt));

   if(gap_debug) printf("spin value: %f\n", (float)adj->value );

   if((gdouble)adj->value != gpp->val.opacity)
   {
     gpp->val.opacity = (gdouble)adj->value;
   }
 }
}


static void
on_oni__spinbutton_opacity_delta_changed
                                        (GtkEditable     *editable,
                                        gpointer         user_data)
{
 t_global_params *gpp;
  GtkWidget *wgt;
  GtkAdjustment *adj;

 gpp = (t_global_params*)user_data;
 if(gap_debug) printf("CB: on_oni__spinbutton_opacity_delta_changed\n");

 if(gpp == NULL) return;
 wgt = gpp->oni__spinbutton_opacity_delta;
 if(wgt)
 {
   adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(wgt));

   if(gap_debug) printf("spin value: %f\n", (float)adj->value );

   if((gdouble)adj->value != gpp->val.opacity_delta)
   {
     gpp->val.opacity_delta = (gdouble)adj->value;
   }
 }
}


static void
on_oni__spinbutton_ignore_botlayers_changed
                                        (GtkEditable     *editable,
                                        gpointer         user_data)
{
 t_global_params *gpp;
  GtkWidget *wgt;
  GtkAdjustment *adj;

 gpp = (t_global_params*)user_data;
 if(gap_debug) printf("CB: on_oni__spinbutton_ignore_botlayers_changed\n");

 if(gpp == NULL) return;
 wgt = gpp->oni__spinbutton_ignore_botlayers;
 if(wgt)
 {
   adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(wgt));

   if(gap_debug) printf("spin value: %f\n", (float)adj->value );

   if((gdouble)adj->value != gpp->val.ignore_botlayers)
   {
     gpp->val.ignore_botlayers = (gdouble)adj->value;
   }
 }
}


static void
on_oni__entry_select_string_changed    (GtkEditable     *editable,
                                        gpointer         user_data)
{
 t_global_params *gpp;
 gpp = (t_global_params*)user_data;
 if(gap_debug) printf("CB: on_oni__entry_select_string_changed\n");

 if(gpp)
 {
    g_snprintf(gpp->val.select_string, sizeof(gpp->val.select_string), "%s"
              , gtk_entry_get_text(GTK_ENTRY(editable)));
 }
}


static void
on_oni__checkbutton_select_case_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
 t_global_params *gpp;
 gpp = (t_global_params*)user_data;
 if(gap_debug) printf("CB: on_oni__checkbutton_select_case_toggled\n");

 if(gpp)
 {
    if (togglebutton->active)
    {
       gpp->val.select_case = TRUE;
    }
    else
    {
       gpp->val.select_case = FALSE;
    }
 }
}


static void
on_oni__checkbutton_select_invert_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
 t_global_params *gpp;
 gpp = (t_global_params*)user_data;
 if(gap_debug) printf("CB: on_oni__checkbutton_select_invert_toggled\n");

 if(gpp)
 {
    if (togglebutton->active)
    {
       gpp->val.select_invert = TRUE;
    }
    else
    {
       gpp->val.select_invert = FALSE;
    }
 }
}


static void
on_oni__button_default_clicked         (GtkButton       *button,
                                        gpointer         user_data)
{
 t_global_params *gpp;
 gpp = (t_global_params*)user_data;
 if(gap_debug) printf("CB: on_oni__button_default_clicked gpp: %d\n", (int)gpp);

 if(gpp)
 {
   p_init_default_values(gpp);
   p_init_main_dialog_widgets(gpp);
 }
}


static void
on_oni__button_cancel_clicked          (GtkButton       *button,
                                        gpointer         user_data)
{
 t_global_params *gpp;
 gpp = (t_global_params*)user_data;
 if(gap_debug) printf("CB: on_oni__button_cancel_clicked\n");

 if(gpp)
 {
   gpp->val.run = GAP_ONION_RUN_CANCEL;
   gtk_widget_destroy (gpp->main_dialog);
 }
 gtk_main_quit ();
}


static void
on_oni__button_set_clicked             (GtkButton       *button,
                                        gpointer         user_data)
{
 t_global_params *gpp;
 gpp = (t_global_params*)user_data;
 if(gap_debug) printf("CB: on_oni__button_set_clicked\n");

 if(gpp)
 {
   /* gpp->val.run = GAP_ONION_RUN_SET; */
   /* gtk_widget_destroy (gpp->main_dialog); */
   
   /* set does not close the dialog and is processed immediate now */
   
   p_set_data_onion_cfg(gpp, GAP_PLUGIN_NAME_ONION_CFG);
 }
 /* gtk_main_quit (); */
}


static void
on_oni__button_delete_clicked          (GtkButton       *button,
                                        gpointer         user_data)
{
 t_global_params *gpp;
 gpp = (t_global_params*)user_data;
 if(gap_debug) printf("CB: on_oni__button_delete_clicked\n");

 if(gpp)
 {
   gpp->val.run = GAP_ONION_RUN_DELETE;
   gtk_widget_destroy (gpp->main_dialog);
 }
 gtk_main_quit ();
}


static void
on_oni__button_apply_clicked           (GtkButton       *button,
                                        gpointer         user_data)
{
 t_global_params *gpp;
 gpp = (t_global_params*)user_data;
 if(gap_debug) printf("CB: on_oni__button_apply_clicked\n");

 if(gpp)
 {
   gpp->val.run = GAP_ONION_RUN_APPLY;
   gtk_widget_destroy (gpp->main_dialog);
 }
 gtk_main_quit ();
}


static void
on_oni__dialog_destroy                 (GtkObject       *object,
                                        gpointer         user_data)
{
 t_global_params *gpp;
 gpp = (t_global_params*)user_data;
 if(gap_debug) printf("CB: on_oni__dialog_destroy\n");

 gtk_main_quit ();

}


/* ----------------------------
 * 
 * ----------------------------
 */


///* add callback functions for optionmenu widgets */
//
//static void
//p_set_menu_item_callbacks(GtkWidget *w, GtkSignalFunc cb_fun, gpointer data)
//{
//  GList     *wgt_children;
//  GtkWidget *opt_menu;
//  GtkWidget *wgt_child;
//  gint       l_idx;
//
//  opt_menu =  gtk_option_menu_get_menu(GTK_OPTION_MENU(w));
//  if(opt_menu == NULL)
//  {
//    if(gap_debug) printf("p_set_menu_item_callbacks: widget is no GTK_OPTION_MENU\n");
//    return;
//  }
//
//  l_idx = 0;
//  wgt_children = gtk_container_children(GTK_CONTAINER (opt_menu));
//  while(wgt_children)
//  {
//     wgt_child = (GtkWidget *)wgt_children->data;
//     if(wgt_child != NULL)
//     {
//        /* if(gap_debug) printf("p_set_menu_item_callbacks: wgt_child found %x\n", (int)wgt_child); */
//     }
//     if(GTK_IS_MENU_ITEM(wgt_child))
//     {
//
//        /* if(gap_debug) printf("p_set_menu_item_callbacks: wgt_child IS_MENU_ITEM\n"); */
//
//        g_signal_connect (G_OBJECT (wgt_child), "activate",
//                        G_CALLBACK (cb_fun),
//                        data);
//        gtk_object_set_data (GTK_OBJECT (wgt_child), ENC_MENU_ITEM_INDEX_KEY, (gpointer) l_idx);
//
//      l_idx++;
//     }
//     wgt_children = (GList *) g_slist_next (wgt_children);
//   }
//}


//static void
//p_set_option_menu_callbacks(t_global_params *gpp)
//{
//  GtkWidget *wgt;
//
//  wgt = gpp->oni__optionmenu_select_mode;
//  p_set_menu_item_callbacks(wgt, (GtkSignalFunc) on_oni__optionmenu_select_mode, (gpointer)gpp);
//}

static void
p_init_optionmenu_actual_idx(t_global_params *gpp, GtkWidget *wgt, gint *gtab_ptr, gint val, gint maxidx)
{
  gint l_idx;

  for(l_idx = 0; l_idx < maxidx; l_idx++)
  {
    if(val == gtab_ptr[l_idx])
    {
      gtk_option_menu_set_history (GTK_OPTION_MENU (wgt), l_idx);
      return;
    }
  }
}


static void
p_init_optionmenus(t_global_params *gpp)
{
 p_init_optionmenu_actual_idx( gpp
                             , gpp->oni__optionmenu_select_mode
                             , gtab_select_modes
                             , gpp->val.select_mode
                             , MAX_SELECT_MODE_ARRAY_ELEMENTS
                             );
}


static void
p_init_entries(t_global_params *gpp)
{
  GtkEntry *entry;

  if(gap_debug) printf("p_init_utl_widgets\n");

  entry = GTK_ENTRY(gpp->oni__entry_select_string);
  gtk_entry_set_text(entry, gpp->val.select_string);
}


static void
p_init_spinbuttons(t_global_params *gpp)
{
  GtkWidget *wgt;
  GtkAdjustment *adj;

  if(gap_debug) printf("p_init_spinbuttons\n");

  wgt = gpp->oni__spinbutton_range_from;
  adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(wgt));
  adj->lower = (gfloat) gpp->ainfo.first_frame_nr;
  adj->upper = (gfloat) gpp->ainfo.last_frame_nr;
  gtk_adjustment_set_value(adj, (gfloat)gpp->range_from);

  wgt = gpp->oni__spinbutton_range_to;
  adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(wgt));
  adj->lower = (gfloat) gpp->ainfo.first_frame_nr;
  adj->upper = (gfloat) gpp->ainfo.last_frame_nr;
  gtk_adjustment_set_value(adj, (gfloat)gpp->range_to);


  wgt = gpp->oni__spinbutton_num_olayers;
  adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(wgt));
  gtk_adjustment_set_value(adj, (gfloat)gpp->val.num_olayers);

  wgt = gpp->oni__spinbutton_ref_delta;
  adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(wgt));
  gtk_adjustment_set_value(adj, (gfloat)gpp->val.ref_delta);

  wgt = gpp->oni__spinbutton_stack_pos;
  adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(wgt));
  gtk_adjustment_set_value(adj, (gfloat)gpp->val.stack_pos);

  wgt = gpp->oni__spinbutton_opacity;
  adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(wgt));
  gtk_adjustment_set_value(adj, (gfloat)gpp->val.opacity);

  wgt = gpp->oni__spinbutton_opacity_delta;
  adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(wgt));
  gtk_adjustment_set_value(adj, (gfloat)gpp->val.opacity_delta);

  wgt = gpp->oni__spinbutton_ignore_botlayers;
  adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(wgt));
  gtk_adjustment_set_value(adj, (gfloat)gpp->val.ignore_botlayers);
}

static void
p_init_togglebuttons(t_global_params *gpp)
{
  GtkWidget *wgt;

  wgt = gpp->oni__checkbutton_ref_cycle;
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (wgt), gpp->val.ref_cycle);

  wgt = gpp->oni__checkbutton_stack_top;
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (wgt), gpp->val.stack_top);

  wgt = gpp->oni__checkbutton_select_case;
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (wgt), gpp->val.select_case);

  wgt = gpp->oni__checkbutton_select_invert;
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (wgt), gpp->val.select_invert);
}

static void
p_init_main_dialog_widgets(t_global_params *gpp)
{
  if(gap_debug) printf("p_init_main_dialog_widgets: Start INIT\n");

  /* put initial values to the widgets */

  p_init_spinbuttons(gpp);
  p_init_entries(gpp);
  p_init_optionmenus(gpp);
  p_init_togglebuttons(gpp);
  p_init_sensitive(gpp);

}       /* end p_init_main_dialog_widgets */


/* ----------------------------
 * create_oni__dialog
 * ----------------------------
 */
static GtkWidget*
create_oni__dialog (t_global_params *gpp)
{
  GtkWidget *oni__dialog;
  GtkWidget *dialog_vbox1;
  GtkWidget *frame1;
  GtkWidget *frame2;
  GtkWidget *frame3;

  GtkWidget *hbox1;
  GtkWidget *hbox2;

  GtkWidget *table1;
  GtkWidget *table2;
  GtkWidget *table3;

  GtkWidget *label1;
  GtkWidget *label2;
  GtkWidget *label3;
  GtkWidget *label4;
  GtkWidget *label5;
  GtkWidget *label6;
  GtkWidget *label7;
  GtkWidget *label8;
  GtkWidget *label9;
  GtkWidget *label10;

  GtkWidget *glade_menuitem;
  GtkWidget *oni__optionmenu_select_mode;
  GtkWidget *oni__optionmenu_select_mode_menu;
  
  GtkObject *oni__spinbutton_range_from_adj;
  GtkWidget *oni__spinbutton_range_from;
  GtkObject *oni__spinbutton_range_to_adj;
  GtkWidget *oni__spinbutton_range_to;
  GtkObject *oni__spinbutton_num_olayers_adj;
  GtkWidget *oni__spinbutton_num_olayers;
  GtkObject *oni__spinbutton_ref_delta_adj;
  GtkWidget *oni__spinbutton_ref_delta;
  GtkWidget *oni__checkbutton_ref_cycle;
  GtkObject *oni__spinbutton_stack_pos_adj;
  GtkWidget *oni__spinbutton_stack_pos;
  GtkWidget *oni__checkbutton_stack_top;
  GtkObject *oni__spinbutton_opacity_adj;
  GtkWidget *oni__spinbutton_opacity;
  GtkObject *oni__spinbutton_opacity_delta_adj;
  GtkWidget *oni__spinbutton_opacity_delta;
  GtkObject *oni__spinbutton_ignore_botlayers_adj;
  GtkWidget *oni__spinbutton_ignore_botlayers;
  GtkWidget *oni__checkbutton_select_case;
  GtkWidget *oni__checkbutton_select_invert;
  GtkWidget *oni__entry_select_string;
  GtkWidget *oni__button_default;
  GtkWidget *dialog_action_area1;
  GtkWidget *oni__button_cancel;
  GtkWidget *oni__button_set;
  GtkWidget *oni__button_delete;
  GtkWidget *oni__button_apply;



  oni__dialog = gtk_dialog_new ();
  gtk_container_set_border_width (GTK_CONTAINER (oni__dialog), 2);
  gtk_window_set_title (GTK_WINDOW (oni__dialog), _("Onionskin Configuration"));

  dialog_vbox1 = GTK_DIALOG (oni__dialog)->vbox;
  gtk_widget_show (dialog_vbox1);

  frame3 = gtk_frame_new (NULL);
  gtk_widget_show (frame3);
  gtk_box_pack_start (GTK_BOX (dialog_vbox1), frame3, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame3), 2);

  table3 = gtk_table_new (2, 2, FALSE);
  gtk_widget_show (table3);
  gtk_container_add (GTK_CONTAINER (frame3), table3);
  gtk_container_set_border_width (GTK_CONTAINER (table3), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table3), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table3), 2);

  label9 = gtk_label_new (_("From Frame : "));
  gtk_widget_show (label9);
  gtk_table_attach (GTK_TABLE (table3), label9, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label9), 0, 0.5);

  label10 = gtk_label_new (_("To Frame :"));
  gtk_widget_show (label10);
  gtk_table_attach (GTK_TABLE (table3), label10, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label10), 0, 0.5);

  oni__spinbutton_range_from_adj = gtk_adjustment_new (1, 0, 9999, 1, 10, 10);
  oni__spinbutton_range_from = gtk_spin_button_new (GTK_ADJUSTMENT (oni__spinbutton_range_from_adj), 1, 0);
  gimp_help_set_help_data(oni__spinbutton_range_from
                         , _("First handled frame")
                         , NULL);
  gtk_widget_show (oni__spinbutton_range_from);
  gtk_table_attach (GTK_TABLE (table3), oni__spinbutton_range_from, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 2, 0);

  oni__spinbutton_range_to_adj = gtk_adjustment_new (1, 0, 9999, 1, 10, 10);
  oni__spinbutton_range_to = gtk_spin_button_new (GTK_ADJUSTMENT (oni__spinbutton_range_to_adj), 1, 0);
  gimp_help_set_help_data(oni__spinbutton_range_to
                         , _("Last handled frame")
                         , NULL);
  gtk_widget_show (oni__spinbutton_range_to);
  gtk_table_attach (GTK_TABLE (table3), oni__spinbutton_range_to, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 2, 0);

  frame1 = gtk_frame_new (NULL);
  gtk_widget_show (frame1);
  gtk_box_pack_start (GTK_BOX (dialog_vbox1), frame1, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame1), 2);

  table1 = gtk_table_new (6, 3, FALSE);
  gtk_widget_show (table1);
  gtk_container_add (GTK_CONTAINER (frame1), table1);
  gtk_container_set_border_width (GTK_CONTAINER (table1), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table1), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table1), 2);

  label1 = gtk_label_new (_("Onionskin Layers :"));
  gtk_widget_show (label1);
  gtk_table_attach (GTK_TABLE (table1), label1, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label1), 0, 0.5);

  oni__spinbutton_num_olayers_adj = gtk_adjustment_new (2, 1, 20, 1, 10, 10);
  oni__spinbutton_num_olayers = gtk_spin_button_new (GTK_ADJUSTMENT (oni__spinbutton_num_olayers_adj), 1, 0);
  gtk_widget_show (oni__spinbutton_num_olayers);
  gtk_table_attach (GTK_TABLE (table1), oni__spinbutton_num_olayers, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data(oni__spinbutton_num_olayers
                         , _("number of onionskin layers to create in handled frame.")
                         , NULL);

  label2 = gtk_label_new (_("Frame Reference :"));
  gtk_widget_show (label2);
  gtk_table_attach (GTK_TABLE (table1), label2, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);

  oni__spinbutton_ref_delta_adj = gtk_adjustment_new (-1, -100, 100, 1, 10, 10);
  oni__spinbutton_ref_delta = gtk_spin_button_new (GTK_ADJUSTMENT (oni__spinbutton_ref_delta_adj), 1, 0);
  gtk_widget_show (oni__spinbutton_ref_delta);
  gtk_table_attach (GTK_TABLE (table1), oni__spinbutton_ref_delta, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data(oni__spinbutton_ref_delta
                         , _("-1 is previous frame, +1 is next frame")
                         , NULL);

  oni__checkbutton_ref_cycle = gtk_check_button_new_with_label (_("Cyclic"));
  gtk_widget_show (oni__checkbutton_ref_cycle);
  gtk_table_attach (GTK_TABLE (table1), oni__checkbutton_ref_cycle, 2, 3, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data(oni__checkbutton_ref_cycle
                         , _("ON: foldback next frame of last is first")
                         , NULL);

  label3 = gtk_label_new (_("Stackposition :"));
  gtk_widget_show (label3);
  gtk_table_attach (GTK_TABLE (table1), label3, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label3), 0, 0.5);

  oni__spinbutton_stack_pos_adj = gtk_adjustment_new (1, 0, 100, 1, 10, 10);
  oni__spinbutton_stack_pos = gtk_spin_button_new (GTK_ADJUSTMENT (oni__spinbutton_stack_pos_adj), 1, 0);
  gtk_widget_show (oni__spinbutton_stack_pos);
  gtk_table_attach (GTK_TABLE (table1), oni__spinbutton_stack_pos, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data(oni__spinbutton_stack_pos
                         , _("Stackposition where to place onion layers")
                         , NULL);

  oni__checkbutton_stack_top = gtk_check_button_new_with_label (_("From Top"));
  gtk_widget_show (oni__checkbutton_stack_top);
  gtk_table_attach (GTK_TABLE (table1), oni__checkbutton_stack_top, 2, 3, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data(oni__checkbutton_stack_top
                         , _("ON: 0 is on top, OFF: 0 is on bottom")
                         , NULL);

  label4 = gtk_label_new (_("Opacity :"));
  gtk_widget_show (label4);
  gtk_table_attach (GTK_TABLE (table1), label4, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label4), 0, 0.5);

  oni__spinbutton_opacity_adj = gtk_adjustment_new (80, 0, 100, 1, 10, 10);
  oni__spinbutton_opacity = gtk_spin_button_new (GTK_ADJUSTMENT (oni__spinbutton_opacity_adj), 1, 1);
  gtk_widget_show (oni__spinbutton_opacity);
  gtk_table_attach (GTK_TABLE (table1), oni__spinbutton_opacity, 1, 2, 3, 4,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data(oni__spinbutton_opacity
                         , _("Opacity of 1.st onionskin layer (0 is transparent, 100 full opaque)")
                         , NULL);

  oni__spinbutton_opacity_delta_adj = gtk_adjustment_new (80, 0, 100, 1, 10, 10);
  oni__spinbutton_opacity_delta = gtk_spin_button_new (GTK_ADJUSTMENT (oni__spinbutton_opacity_delta_adj), 1, 1);
  gtk_widget_show (oni__spinbutton_opacity_delta);
  gtk_table_attach (GTK_TABLE (table1), oni__spinbutton_opacity_delta, 2, 3, 3, 4,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data(oni__spinbutton_opacity_delta
                         , _("Descending Opacity for 2.nd onionskin layer")
                         , NULL);

  frame2 = gtk_frame_new (_("Layer Selection "));
  gtk_widget_show (frame2);
  gtk_table_attach (GTK_TABLE (table1), frame2, 0, 3, 4, 5,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame2), 2);

  table2 = gtk_table_new (5, 2, FALSE);
  gtk_widget_show (table2);
  gtk_container_add (GTK_CONTAINER (frame2), table2);
  gtk_container_set_border_width (GTK_CONTAINER (table2), 4);

  label5 = gtk_label_new (_("Ignore BG-layer(s) : "));
  gtk_widget_show (label5);
  gtk_table_attach (GTK_TABLE (table2), label5, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label5), 0, 0.5);

  oni__spinbutton_ignore_botlayers_adj = gtk_adjustment_new (1, 0, 100, 1, 10, 10);
  oni__spinbutton_ignore_botlayers = gtk_spin_button_new (GTK_ADJUSTMENT (oni__spinbutton_ignore_botlayers_adj), 1, 0);
  gtk_widget_show (oni__spinbutton_ignore_botlayers);
  gtk_table_attach (GTK_TABLE (table2), oni__spinbutton_ignore_botlayers, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data(oni__spinbutton_ignore_botlayers
                         , _("exclude N BG-Layers (0 ")
                         , NULL);

  label7 = gtk_label_new (_("Select Mode :"));
  gtk_widget_show (label7);
  gtk_table_attach (GTK_TABLE (table2), label7, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label7), 0, 0.5);

  oni__optionmenu_select_mode = gtk_option_menu_new ();
  gtk_widget_show (oni__optionmenu_select_mode);
  gtk_table_attach (GTK_TABLE (table2), oni__optionmenu_select_mode, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data(oni__optionmenu_select_mode
                         , _("Modes how to use Select Pattern")
                         , NULL);
  oni__optionmenu_select_mode_menu = gtk_menu_new ();
  glade_menuitem = gtk_menu_item_new_with_label (_("Pattern is equal to LayerName"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (oni__optionmenu_select_mode_menu), glade_menuitem);
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_oni__optionmenu_select_mode),
                          (gpointer)gpp);
        gtk_object_set_data (GTK_OBJECT (glade_menuitem), ENC_MENU_ITEM_INDEX_KEY, (gpointer)0);

  glade_menuitem = gtk_menu_item_new_with_label (_("Pattern is Start of LayerName"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (oni__optionmenu_select_mode_menu), glade_menuitem);
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_oni__optionmenu_select_mode),
                          (gpointer)gpp);
        gtk_object_set_data (GTK_OBJECT (glade_menuitem), ENC_MENU_ITEM_INDEX_KEY, (gpointer)1);

  glade_menuitem = gtk_menu_item_new_with_label (_("Pattern is End of Layername"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (oni__optionmenu_select_mode_menu), glade_menuitem);
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_oni__optionmenu_select_mode),
                          (gpointer)gpp);
        gtk_object_set_data (GTK_OBJECT (glade_menuitem), ENC_MENU_ITEM_INDEX_KEY, (gpointer)2);

  glade_menuitem = gtk_menu_item_new_with_label (_("Pattern is a Part of LayerName"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (oni__optionmenu_select_mode_menu), glade_menuitem);
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_oni__optionmenu_select_mode),
                          (gpointer)gpp);
        gtk_object_set_data (GTK_OBJECT (glade_menuitem), ENC_MENU_ITEM_INDEX_KEY, (gpointer)3);

  glade_menuitem = gtk_menu_item_new_with_label (_("Pattern is LayerstackNumber List"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (oni__optionmenu_select_mode_menu), glade_menuitem);
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_oni__optionmenu_select_mode),
                          (gpointer)gpp);
        gtk_object_set_data (GTK_OBJECT (glade_menuitem), ENC_MENU_ITEM_INDEX_KEY, (gpointer)4);

  glade_menuitem = gtk_menu_item_new_with_label (_("Pattern is REVERSE-stack List"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (oni__optionmenu_select_mode_menu), glade_menuitem);
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_oni__optionmenu_select_mode),
                          (gpointer)gpp);
        gtk_object_set_data (GTK_OBJECT (glade_menuitem), ENC_MENU_ITEM_INDEX_KEY, (gpointer)5);

  glade_menuitem = gtk_menu_item_new_with_label (_("All Visible (ignore Pattern)"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (oni__optionmenu_select_mode_menu), glade_menuitem);
        g_signal_connect (G_OBJECT (glade_menuitem), "activate",
                          G_CALLBACK (on_oni__optionmenu_select_mode),
                          (gpointer)gpp);
        gtk_object_set_data (GTK_OBJECT (glade_menuitem), ENC_MENU_ITEM_INDEX_KEY, (gpointer)6);

  gtk_option_menu_set_menu (GTK_OPTION_MENU (oni__optionmenu_select_mode), oni__optionmenu_select_mode_menu);
  gtk_option_menu_set_history (GTK_OPTION_MENU (oni__optionmenu_select_mode), 6);

  label8 = gtk_label_new (_("Select Options :"));
  gtk_widget_show (label8);
  gtk_table_attach (GTK_TABLE (table2), label8, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label8), 0, 0.5);

  hbox2 = gtk_hbox_new (TRUE, 0);
  gtk_widget_show (hbox2);
  gtk_table_attach (GTK_TABLE (table2), hbox2, 1, 2, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  oni__checkbutton_select_case = gtk_check_button_new_with_label (_("Case sensitve"));
  gtk_widget_show (oni__checkbutton_select_case);
  gtk_box_pack_start (GTK_BOX (hbox2), oni__checkbutton_select_case, FALSE, TRUE, 0);
  gimp_help_set_help_data(oni__checkbutton_select_case
                         , _("ON: case sensitive pattern, OFF ignore case")
                         , NULL);

  oni__checkbutton_select_invert = gtk_check_button_new_with_label (_("Invert Selection"));
  gtk_widget_show (oni__checkbutton_select_invert);
  gtk_box_pack_start (GTK_BOX (hbox2), oni__checkbutton_select_invert, FALSE, TRUE, 0);
  gimp_help_set_help_data(oni__checkbutton_select_invert
                         , _("ON: Select NON-matching layers, OFF: Select matching layers")
                         , NULL);

  label6 = gtk_label_new (_("Select Pattern :"));
  gtk_widget_show (label6);
  gtk_table_attach (GTK_TABLE (table2), label6, 0, 1, 4, 5,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label6), 0, 0.5);

  oni__entry_select_string = gtk_entry_new ();
  gtk_widget_show (oni__entry_select_string);
  gtk_table_attach (GTK_TABLE (table2), oni__entry_select_string, 1, 2, 4, 5,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data(oni__entry_select_string
                         , _("Select Layernames by Pattern (depends on Mode and Options)")
                         , NULL);


  oni__button_set = gtk_button_new_with_label (_("Set"));
  gtk_widget_show (oni__button_set);
  gtk_table_attach (GTK_TABLE (table1), oni__button_set, 0, 3, 5, 6,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 2, 0);
  gimp_help_set_help_data(oni__button_set
                         , _("Set Onionskin parameters for the current gimp session")
                         , NULL);

  dialog_action_area1 = GTK_DIALOG (oni__dialog)->action_area;
  gtk_widget_show (dialog_action_area1);
  gtk_container_set_border_width (GTK_CONTAINER (dialog_action_area1), 10);

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox1);
  gtk_box_pack_end (GTK_BOX (dialog_action_area1), hbox1, FALSE, TRUE, 0);



  oni__button_apply = gtk_button_new_from_stock (GTK_STOCK_OK);
  gtk_widget_set_usize (oni__button_apply, 100, -2);
  gtk_widget_show (oni__button_apply);
  gtk_box_pack_end (GTK_BOX (hbox1), oni__button_apply, FALSE, FALSE, 2);
  gimp_help_set_help_data(oni__button_apply
                         , _("Create or replace onionskin layer in all frames of the selected framerange")
                         , NULL);

  oni__button_delete = gtk_button_new_from_stock (GTK_STOCK_DELETE);
  gtk_widget_show (oni__button_delete);
  gtk_box_pack_end (GTK_BOX (hbox1), oni__button_delete, FALSE, FALSE, 2);
  gimp_help_set_help_data(oni__button_delete
                         , _("remove all onionlayers in all frames of the the selected framerange")
                         , NULL);


  oni__button_cancel = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  gtk_widget_show (oni__button_cancel);
  gtk_box_pack_end (GTK_BOX (hbox1), oni__button_cancel, FALSE, FALSE, 2);
  gimp_help_set_help_data(oni__button_cancel
                         , _("Exit without any action")
                         , NULL);

  oni__button_default = gtk_button_new_from_stock (GIMP_STOCK_RESET);
  gtk_widget_show (oni__button_default);
  gtk_box_pack_end (GTK_BOX (hbox1), oni__button_default, FALSE, FALSE, 2);
  gimp_help_set_help_data(oni__button_default
                         , _("Reset to Default Settings")
                         , NULL);


  g_signal_connect (G_OBJECT (oni__dialog), "destroy",
                      G_CALLBACK (on_oni__dialog_destroy),
                      gpp);
  g_signal_connect (G_OBJECT (oni__spinbutton_range_from), "changed",
                      G_CALLBACK (on_oni__spinbutton_range_from_changed),
                      gpp);
  g_signal_connect (G_OBJECT (oni__spinbutton_range_to), "changed",
                      G_CALLBACK (on_oni__spinbutton_range_to_changed),
                      gpp);
  g_signal_connect (G_OBJECT (oni__spinbutton_num_olayers), "changed",
                      G_CALLBACK (on_oni__spinbutton_num_olayers_changed),
                      gpp);
  g_signal_connect (G_OBJECT (oni__spinbutton_ref_delta), "changed",
                      G_CALLBACK (on_oni__spinbutton_ref_delta_changed),
                      gpp);
  g_signal_connect (G_OBJECT (oni__checkbutton_ref_cycle), "toggled",
                      G_CALLBACK (on_oni__checkbutton_ref_cycle_toggled),
                      gpp);
  g_signal_connect (G_OBJECT (oni__spinbutton_stack_pos), "changed",
                      G_CALLBACK (on_oni__spinbutton_stack_pos_changed),
                      gpp);
  g_signal_connect (G_OBJECT (oni__checkbutton_stack_top), "toggled",
                      G_CALLBACK (on_oni__checkbutton_stack_top_toggled),
                      gpp);
  g_signal_connect (G_OBJECT (oni__spinbutton_opacity), "changed",
                      G_CALLBACK (on_oni__spinbutton_opacity_changed),
                      gpp);
  g_signal_connect (G_OBJECT (oni__spinbutton_opacity_delta), "changed",
                      G_CALLBACK (on_oni__spinbutton_opacity_delta_changed),
                      gpp);
  g_signal_connect (G_OBJECT (oni__spinbutton_ignore_botlayers), "changed",
                      G_CALLBACK (on_oni__spinbutton_ignore_botlayers_changed),
                      gpp);
  g_signal_connect (G_OBJECT (oni__checkbutton_select_case), "toggled",
                      G_CALLBACK (on_oni__checkbutton_select_case_toggled),
                      gpp);
  g_signal_connect (G_OBJECT (oni__checkbutton_select_invert), "toggled",
                      G_CALLBACK (on_oni__checkbutton_select_invert_toggled),
                      gpp);
  g_signal_connect (G_OBJECT (oni__entry_select_string), "changed",
                      G_CALLBACK (on_oni__entry_select_string_changed),
                      gpp);
  g_signal_connect (G_OBJECT (oni__button_default), "clicked",
                      G_CALLBACK (on_oni__button_default_clicked),
                      gpp);
  g_signal_connect (G_OBJECT (oni__button_cancel), "clicked",
                      G_CALLBACK (on_oni__button_cancel_clicked),
                      gpp);
  g_signal_connect (G_OBJECT (oni__button_set), "clicked",
                      G_CALLBACK (on_oni__button_set_clicked),
                      gpp);
  g_signal_connect (G_OBJECT (oni__button_delete), "clicked",
                      G_CALLBACK (on_oni__button_delete_clicked),
                      gpp);
  g_signal_connect (G_OBJECT (oni__button_apply), "clicked",
                      G_CALLBACK (on_oni__button_apply_clicked),
                      gpp);




  /* copy widget pointers to global parameter
   * (for use in callbacks outside of this procedure)
   */
  gpp->oni__entry_select_string         = oni__entry_select_string;
  gpp->oni__optionmenu_select_mode      = oni__optionmenu_select_mode;
  gpp->oni__spinbutton_ignore_botlayers = oni__spinbutton_ignore_botlayers;
  gpp->oni__spinbutton_num_olayers      = oni__spinbutton_num_olayers;
  gpp->oni__spinbutton_opacity          = oni__spinbutton_opacity;
  gpp->oni__spinbutton_opacity_delta    = oni__spinbutton_opacity_delta;
  gpp->oni__spinbutton_range_from       = oni__spinbutton_range_from;
  gpp->oni__spinbutton_range_to         = oni__spinbutton_range_to;
  gpp->oni__spinbutton_ref_delta        = oni__spinbutton_ref_delta;
  gpp->oni__spinbutton_stack_pos        = oni__spinbutton_stack_pos;
  gpp->oni__checkbutton_ref_cycle       = oni__checkbutton_ref_cycle;
  gpp->oni__checkbutton_select_case     = oni__checkbutton_select_case;
  gpp->oni__checkbutton_select_invert   = oni__checkbutton_select_invert;
  gpp->oni__checkbutton_stack_top       = oni__checkbutton_stack_top;


  return oni__dialog;
}  /* end create_oni__dialog */


/* ============================================================================
 * p_init_default_values
 *    Set onion config default params.
 * ============================================================================
 */
void
p_init_default_values(t_global_params *gpp)
{
          gpp->val.num_olayers        = 2;
          gpp->val.ref_delta          = -1;
          gpp->val.ref_cycle          = FALSE;
          gpp->val.stack_pos          = 1;
          gpp->val.stack_top          = FALSE;
          gpp->val.opacity            = 80.0;
          gpp->val.opacity_delta      = 80.0;
          gpp->val.ignore_botlayers   = 1;
          gpp->val.select_mode        = MTCH_ALL_VISIBLE;  /* MTCH_ANYWHERE, MTCH_ALL_VISIBLE */
          gpp->val.select_case        = 0;     /* 0 .. ignore case, 1..case sensitve */
          gpp->val.select_invert      = 0;     /* 0 .. no invert, 1 ..invert */
          gpp->val.select_string[0] = '\0';
/*
          g_snprintf(&gpp->val.select_string[0]
                    , sizeof(gpp->val.select_string)
                    ,"pasted"
                    );
*/
}       /* end p_init_default_values */


/* ============================================================================
 * p_onion_cfg_dialog
 *
 *   return  0 .. OK
 *          -1 .. in case of Error or cancel
 * ============================================================================
 */

gint
p_onion_cfg_dialog(t_global_params *gpp)
{
  //gint argc = 1;
  //gchar **argv = g_new (gchar *, 1);

  if(gap_debug) printf("p_onion_cfg_dialog: Start\n");

  //argv[0] = g_strdup (_("GAP_ONION__DIALOG"));
  //gtk_set_locale ();
  //setlocale (LC_NUMERIC, "C");  /* is needed when after gtk_set_locale ()
  //                               * to make sure PASSING FLOAT PDB_PARAMETERS works
  //                               * (thanks to sven for the tip)
  //                               */
  //
  //gtk_init (&argc, &argv);
  gimp_ui_init ("gap_onion_dialog", FALSE);

  /* ---------- dialog ----------*/

  if(gap_debug) printf("p_onion_cfg_dialog: Before create_oni__dialog\n");

  gpp->main_dialog = create_oni__dialog(gpp);

  if(gap_debug) printf("p_onion_cfg_dialog: After create_oni__dialog\n");

  //p_set_option_menu_callbacks(gpp);
  p_init_main_dialog_widgets(gpp);
  gtk_widget_show (gpp->main_dialog);

  gpp->val.run = 0; /* GAP_ONION_RUN_CANCEL */
  gtk_main ();

  if(gap_debug) printf("p_onion_cfg_dialog: A F T E R gtk_main run:%d\n", (int)gpp->val.run);

  gpp->main_dialog = NULL;

  if(gpp->val.run)
  {
    return 0;
  }
  return -1;
}               /* end p_onion_cfg_dialog */
