/* 
 *  gap_dbbrowser_utils.c (most parts of the code are copied from 
 *  GIMP DB-Browser Code by Thomas NOEL <thomas@minet.net>
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
 * gimp    1.3.20b; 2003/09/20  hof: gap_db_browser_dialog and constraint procedures added new param image_id
 *                                   DEBUG MODE: made iterator code generation working again.
 * gimp    1.3.20a; 2003/09/14  hof: merged in changes of the file
 *                                   gimp-1.3.20/plug-ins/dbbrowser/dbbrowser_utils.c
 *                                   now uses GtkTreeView and gtk+2.2 compatible coding
 *                                   added menu path and Search by Menu Path Button
 * 27. jan .1999   hof: update for GIMP 1.1.1 (show help)
 * 09. dec .1998   hof: update for GIMP 1.1
 * 12. jan .1998   hof: added "Gen Code" button
 *  
 * 23. dec .1997   hof: created GAP specific variant of DBbrowser
 *                       removed apply_callback
 *                       added constraint_procedure, 
 *                       added 2 buttons
 *                       added return type
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>


/* GAP includes */
#include "gap_filter.h"
#include "gap_match.h"
#include "gap_pdb_calls.h"
#include "gap_dbbrowser_utils.h"
#include "gap-intl.h"


#define DBL_LIST_WIDTH 220
#define DBL_WIDTH      (DBL_LIST_WIDTH + 500)
#define DBL_HEIGHT     250


extern int gap_debug;

typedef struct
{
  GtkWidget        *dialog;

  GtkWidget        *search_entry;
  GtkWidget        *name_button;
  GtkWidget        *blurb_button;

  GtkWidget        *descr_vbox;
  GtkWidget        *description;

  GtkListStore     *store;
  GtkWidget        *tv;
  GtkTreeSelection *sel;

  /* the currently selected procedure */
  gchar            *selected_proc_name;
  gchar            *selected_scheme_proc_name;
  gchar            *selected_proc_blurb;
  gchar            *selected_proc_help;
  gchar            *selected_proc_author;
  gchar            *selected_proc_copyright;
  gchar            *selected_proc_date;
  GimpPDBProcType   selected_proc_type;
  gint              selected_nparams;
  gint              selected_nreturn_vals;
  GimpParamDef     *selected_params;
  GimpParamDef     *selected_return_vals; 

  /* GAP DB-Browser specific items */
  gchar            *selected_menu_path;
  GtkWidget* app_const_button;
  GtkWidget* app_vary_button;
  GtkWidget* menupath_button;

  t_constraint_func      constraint_func;
  t_constraint_func      constraint_func_sel1;
  t_constraint_func      constraint_func_sel2;
  t_gap_db_browse_result *result;
  
  gint                   codegen_flag;
  gint32                 current_image_id;
} dbbrowser_t;

/* local functions */

static void         procedure_select_callback    (GtkTreeSelection  *sel,
						  dbbrowser_t       *dbbrowser);
static void         dialog_search_callback       (GtkWidget         *widget, 
						  dbbrowser_t       *dbbrowser);
static void         dialog_select                (dbbrowser_t       *dbbrowser, 
						  gchar             *proc_name);
static void         dialog_close_callback        (GtkWidget         *widget, 
						  dbbrowser_t       *dbbrowser);
static void         convert_string               (gchar             *str);
static const gchar *GParamType2char              (GimpPDBArgType     t);

/* GAP specific extra callbacks */
static void         dialog_num_button_callback   (dbbrowser_t* dbbrowser, 
		                                  gint button_nr);
static void         dialog_button_1_callback     (GtkWidget *widget,
                                                  dbbrowser_t* dbbrowser);
static void         dialog_button_2_callback     (GtkWidget *widget,
                                                  dbbrowser_t* dbbrowser);
static void         dialog_button_3_callback     (GtkWidget *widget,
                                                  dbbrowser_t* dbbrowser);
static gchar*       p_get_plugin_menupath        (const gchar *name);
static void         p_create_action_area_buttons (dbbrowser_t *dbbrowser,
                				  char *button_1_txt,
                				  char *button_2_txt
                                                );


/* create and perform the dialog */
int 
gap_db_browser_dialog(char *title_txt,
                      char *button_1_txt,
                      char *button_2_txt,
                      t_constraint_func        constraint_func,
                      t_constraint_func        constraint_func_sel1,
                      t_constraint_func        constraint_func_sel2,
                      t_gap_db_browse_result  *result,
		      gint32                   image_id)
{
  dbbrowser_t     *dbbrowser;
  GtkWidget       *hpaned;
  GtkWidget       *searchhbox;
  GtkWidget       *vbox;
  GtkWidget       *label;
  GtkWidget       *scrolled_window;
  GtkCellRenderer *renderer;

  gimp_ui_init ("gap-animated-filter-apply", FALSE);

  dbbrowser = g_new0 (dbbrowser_t, 1);
  
  /* store pointers to gap constraint procedures */  
  dbbrowser->constraint_func      = constraint_func;
  dbbrowser->constraint_func_sel1 = constraint_func_sel1;
  dbbrowser->constraint_func_sel2 = constraint_func_sel2;
  dbbrowser->result  = result;
  dbbrowser->codegen_flag  = 0;   /* default: no code generation */
  dbbrowser->current_image_id = image_id;
  
  /* the dialog box */

  dbbrowser->dialog = gtk_dialog_new ();
  
  gtk_window_set_title (GTK_WINDOW (dbbrowser->dialog), _("Animated Filter apply"));
  gtk_window_set_position (GTK_WINDOW (dbbrowser->dialog), GTK_WIN_POS_MOUSE);
  g_signal_connect (dbbrowser->dialog, "destroy",
                    G_CALLBACK (dialog_close_callback), dbbrowser);


  /* hpaned : left=list ; right=description */

  hpaned = gtk_hpaned_new ();
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dbbrowser->dialog)->vbox), 
		      hpaned, TRUE, TRUE, 0);
  gtk_widget_show (hpaned);

  /* left = vbox : the list and the search entry */
  
  vbox = gtk_vbox_new (FALSE, 4);
  gtk_paned_pack1 (GTK_PANED (hpaned), vbox, FALSE, TRUE);
  gtk_widget_show (vbox);

  /* list : list in a scrolled_win */
  
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
				       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_ALWAYS);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_window);

  dbbrowser->tv = gtk_tree_view_new ();

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_renderer_text_set_fixed_height_from_font
    (GTK_CELL_RENDERER_TEXT (renderer), 1);

  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (dbbrowser->tv),
					       -1, NULL,
					       renderer,
					       "text", 0,
					       NULL);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (dbbrowser->tv), FALSE);

  gtk_widget_set_size_request (dbbrowser->tv, DBL_LIST_WIDTH, DBL_HEIGHT);
  gtk_container_add (GTK_CONTAINER (scrolled_window), dbbrowser->tv);
  gtk_widget_show (dbbrowser->tv);

  dbbrowser->sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (dbbrowser->tv));
  g_signal_connect (dbbrowser->sel, "changed",
		    G_CALLBACK (procedure_select_callback), dbbrowser);

  /* search entry */

  searchhbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), searchhbox, FALSE, FALSE, 2);
  gtk_widget_show (searchhbox);

  label = gtk_label_new_with_mnemonic (_("_Search:"));
  gtk_box_pack_start (GTK_BOX (searchhbox), label, FALSE, FALSE, 2);
  gtk_widget_show (label);

  dbbrowser->search_entry = gtk_entry_new ();
  gtk_entry_set_activates_default (GTK_ENTRY (dbbrowser->search_entry), TRUE);
  gtk_box_pack_start (GTK_BOX (searchhbox), dbbrowser->search_entry,
		      TRUE, TRUE, 0);
  gtk_widget_show (dbbrowser->search_entry);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), dbbrowser->search_entry);

  /* right = description */

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_size_request (scrolled_window, DBL_WIDTH - DBL_LIST_WIDTH, -1);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				  GTK_POLICY_AUTOMATIC, 
				  GTK_POLICY_ALWAYS);
  gtk_paned_pack2 (GTK_PANED (hpaned), scrolled_window, TRUE, TRUE);
  gtk_widget_show (scrolled_window);

  dbbrowser->descr_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (dbbrowser->descr_vbox), 4);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window),
					 dbbrowser->descr_vbox);
  gtk_widget_show (dbbrowser->descr_vbox);





  /* GAP specific buttons in dialog->action_aera */
  p_create_action_area_buttons(dbbrowser, button_1_txt, button_2_txt);


  /* now build the list */

  gtk_widget_show (dbbrowser->dialog);

  /* initialize the "return" value (for "apply") */

  dbbrowser->description               = NULL;
  dbbrowser->selected_proc_name        = NULL;
  dbbrowser->selected_scheme_proc_name = NULL;
  dbbrowser->selected_proc_blurb       = NULL;
  dbbrowser->selected_proc_help        = NULL;
  dbbrowser->selected_proc_author      = NULL;
  dbbrowser->selected_proc_copyright   = NULL;
  dbbrowser->selected_proc_date        = NULL;
  dbbrowser->selected_proc_type        = 0;
  dbbrowser->selected_nparams          = 0;
  dbbrowser->selected_nreturn_vals     = 0;
  dbbrowser->selected_params           = NULL;
  dbbrowser->selected_return_vals      = NULL;
  dbbrowser->selected_menu_path        = NULL;

  dbbrowser->result->selected_proc_name[0] = '\0';
  dbbrowser->result->button_nr = -1;

  /* first search (all procedures) */
  dialog_search_callback (NULL, dbbrowser);

  /* GTK Main Loop */
  gtk_main ();
  gdk_flush ();

  return  (dbbrowser->result->button_nr);
} /* end gap_db_browser_dialog */


static void
p_create_action_area_buttons(dbbrowser_t *dbbrowser,
                	     char *button_1_txt,
                	     char *button_2_txt
                            )
{
  GtkWidget       *table;
  GtkWidget       *button;
  gint             row;

  table = gtk_table_new(3,3,FALSE);

  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
  gtk_widget_show (table);

  gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG(dbbrowser->dialog)->action_area), 0);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dbbrowser->dialog)->action_area), 
			table, TRUE, TRUE, 0);

  row = 0;
  /* Button GenCode (conditional in DEBUG mode only for developers) */
  if (gap_debug) {
    button = gtk_button_new_with_label ( _("Gen Code by name"));
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    g_signal_connect (G_OBJECT (button), "clicked",
			G_CALLBACK (dialog_button_3_callback), dbbrowser );

    gtk_table_attach (GTK_TABLE (table), button,
		    0, 1, row, row + 1, GTK_FILL, GTK_FILL, 0, 6);
    gtk_widget_show (button);
  }

  row = 1;
  /* Button Search by Name */
  dbbrowser->name_button = gtk_button_new_with_label ( _("Search by Name"));
  GTK_WIDGET_SET_FLAGS (dbbrowser->name_button, GTK_CAN_DEFAULT);
  g_signal_connect (G_OBJECT (dbbrowser->name_button), "clicked",
                      G_CALLBACK (dialog_search_callback), dbbrowser);
  gtk_table_attach (GTK_TABLE (table), dbbrowser->name_button,
		    0, 1, row, row + 1, GTK_FILL, GTK_FILL, 0, 6);
  gtk_widget_show(dbbrowser->name_button);

  /* Button Search by Blurb */
  dbbrowser->blurb_button = gtk_button_new_with_label ( _("Search by Blurb"));
  GTK_WIDGET_SET_FLAGS (dbbrowser->blurb_button, GTK_CAN_DEFAULT);
  g_signal_connect (G_OBJECT (dbbrowser->blurb_button), "clicked",
                      G_CALLBACK (dialog_search_callback), dbbrowser);
  gtk_table_attach (GTK_TABLE (table), dbbrowser->blurb_button,
		    1, 2, row, row + 1, GTK_FILL, GTK_FILL, 0, 6);
  gtk_widget_show(dbbrowser->blurb_button);

  /* Button Search by Menupath */
  dbbrowser->menupath_button = gtk_button_new_with_label ( _("Search by Menu Path"));
  GTK_WIDGET_SET_FLAGS (dbbrowser->menupath_button, GTK_CAN_DEFAULT);
  g_signal_connect (G_OBJECT (dbbrowser->menupath_button), "clicked",
                      G_CALLBACK (dialog_search_callback), dbbrowser);
  gtk_table_attach (GTK_TABLE (table), dbbrowser->menupath_button,
		    2, 3, row, row + 1, GTK_FILL, GTK_FILL, 0, 6);
  gtk_widget_show(dbbrowser->menupath_button);


  row = 2;
  /* Button1 (Apply Constant) */
  if (button_1_txt) {
    dbbrowser->app_const_button = gtk_button_new_with_label (button_1_txt);
    GTK_WIDGET_SET_FLAGS (dbbrowser->app_const_button, GTK_CAN_DEFAULT);
    g_signal_connect (G_OBJECT (dbbrowser->app_const_button), "clicked",
			G_CALLBACK (dialog_button_1_callback), dbbrowser );
    gtk_table_attach (GTK_TABLE (table), dbbrowser->app_const_button,
		    0, 1, row, row + 1, GTK_FILL, GTK_FILL, 0, 6);
    gtk_widget_show (dbbrowser->app_const_button);
  } else dbbrowser->app_const_button = NULL;

  /* Button1 (Apply Varying) */
  if (button_2_txt) {
    dbbrowser->app_vary_button = gtk_button_new_with_label (button_2_txt);
    GTK_WIDGET_SET_FLAGS (dbbrowser->app_vary_button, GTK_CAN_DEFAULT);
    g_signal_connect (G_OBJECT (dbbrowser->app_vary_button), "clicked",
			G_CALLBACK (dialog_button_2_callback), dbbrowser );
    gtk_table_attach (GTK_TABLE (table), dbbrowser->app_vary_button,
		    1, 2, row, row + 1, GTK_FILL, GTK_FILL, 0, 6);
    gtk_widget_show (dbbrowser->app_vary_button);
  } else dbbrowser->app_vary_button = NULL;

  /* Button Cancel */
  button = gtk_button_new_from_stock ( GTK_STOCK_CANCEL);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  g_signal_connect (G_OBJECT (button), "clicked",
		      G_CALLBACK (dialog_close_callback), dbbrowser);
  gtk_table_attach (GTK_TABLE (table), button,
		    2, 3, row, row + 1, GTK_FILL, GTK_FILL, 0, 6);
  gtk_widget_show (button);

}  /* end p_create_action_area_buttons */



static void
procedure_select_callback (GtkTreeSelection *sel,
			   dbbrowser_t      *dbbrowser)
{
  GtkTreeIter  iter;
  gchar       *func;

  g_return_if_fail (sel != NULL);
  g_return_if_fail (dbbrowser != NULL);

  if (gtk_tree_selection_get_selected (sel, NULL, &iter))
    {
      gtk_tree_model_get (GTK_TREE_MODEL (dbbrowser->store), &iter,
			  1, &func,
			  -1);
      dialog_select (dbbrowser, func);
      g_free (func);
    }
}

/* update the description box (right) */
static void 
dialog_select (dbbrowser_t *dbbrowser, 
	       gchar       *proc_name)
{
  GtkWidget   *old_description;
  GtkWidget   *label;
  GtkWidget   *sep;
  gint         i;
  gint         row = 0;
  const gchar *type;

  g_free (dbbrowser->selected_proc_name);
  dbbrowser->selected_proc_name = g_strdup (proc_name);

  g_free (dbbrowser->selected_scheme_proc_name);
  dbbrowser->selected_scheme_proc_name = g_strdup (proc_name);
  convert_string (dbbrowser->selected_scheme_proc_name);

  g_free (dbbrowser->selected_proc_blurb);
  g_free (dbbrowser->selected_proc_help);
  g_free (dbbrowser->selected_proc_author);
  g_free (dbbrowser->selected_proc_copyright);
  g_free (dbbrowser->selected_proc_date);
  g_free (dbbrowser->selected_params);
  g_free (dbbrowser->selected_return_vals);

  g_free (dbbrowser->selected_menu_path); 
  dbbrowser->selected_menu_path = p_get_plugin_menupath(proc_name);
  if(dbbrowser->selected_menu_path == NULL)
    dbbrowser->selected_menu_path = g_strdup(_("** not available **"));
    

  gimp_procedural_db_proc_info (proc_name, 
				&dbbrowser->selected_proc_blurb,
				&dbbrowser->selected_proc_help,
				&dbbrowser->selected_proc_author,
				&dbbrowser->selected_proc_copyright,
				&dbbrowser->selected_proc_date,
				&dbbrowser->selected_proc_type,
				&dbbrowser->selected_nparams,
				&dbbrowser->selected_nreturn_vals,
				&dbbrowser->selected_params,
				&dbbrowser->selected_return_vals);

  /* save the "old" table */
  old_description = dbbrowser->description;

  dbbrowser->description = gtk_table_new (11 +
					  dbbrowser->selected_nparams +
					  dbbrowser->selected_nreturn_vals,
					  5, FALSE);

  gtk_table_set_col_spacings (GTK_TABLE (dbbrowser->description), 6);
  gtk_table_set_row_spacing (GTK_TABLE (dbbrowser->description), 0, 2);

  /* show the name */

  label = gtk_label_new (dbbrowser->selected_scheme_proc_name);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);

  gimp_table_attach_aligned (GTK_TABLE (dbbrowser->description), 0, row++,
			     _("Name:"), 1.0, 0.5,
			     label, 3, FALSE);
  /* show the description */

  label = gtk_label_new (dbbrowser->selected_proc_blurb);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  gimp_table_attach_aligned (GTK_TABLE (dbbrowser->description), 0, row++,
			     _("Blurb:"), 1.0, 0.5,
			     label, 3, FALSE);


  /* GAP extension: show the Menu Path */

  label = gtk_label_new (dbbrowser->selected_menu_path);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);

  gimp_table_attach_aligned (GTK_TABLE (dbbrowser->description), 0, row++,
			     _("Menu Path:"), 1.0, 0.5,
			     label, 3, FALSE);



  sep = gtk_hseparator_new ();
  gtk_table_attach (GTK_TABLE (dbbrowser->description), sep,
		    0, 4, row, row + 1, GTK_FILL, GTK_FILL, 0, 6);
  gtk_widget_show (sep);
  row++;

  /* in parameters */
  if (dbbrowser->selected_nparams) 
    {
      label = gtk_label_new (_("In:"));
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5); 
      gtk_table_attach (GTK_TABLE (dbbrowser->description), label,
			0, 1, row, row + (dbbrowser->selected_nparams), 
			GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (label);

      for (i = 0; i < dbbrowser->selected_nparams; i++) 
	{
	  /* name */
	  label = gtk_label_new ((dbbrowser->selected_params[i]).name);
	  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5); 
	  gtk_table_attach (GTK_TABLE (dbbrowser->description), label,
			    1, 2, row, row + 1, GTK_FILL, GTK_FILL, 0, 0);
	  gtk_widget_show (label);

	  /* type */
	  type = GParamType2char ((dbbrowser->selected_params[i]).type);
	  label = gtk_label_new (type);
	  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	  gtk_table_attach (GTK_TABLE (dbbrowser->description), label,
			    2, 3, row, row + 1, GTK_FILL, GTK_FILL, 0, 0);
	  gtk_widget_show (label);

	  /* description */
	  label = gtk_label_new ((dbbrowser->selected_params[i]).description);
	  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
	  gtk_table_attach (GTK_TABLE (dbbrowser->description), label,
			    3, 4, row, row + 1, GTK_FILL, GTK_FILL, 0, 0);
	  gtk_widget_show (label);

	  row++;
	}
    }

  if ((dbbrowser->selected_nparams) && 
      (dbbrowser->selected_nreturn_vals))
    {
      sep = gtk_hseparator_new ();
      gtk_table_attach (GTK_TABLE (dbbrowser->description), sep,
			0, 4, row, row + 1, GTK_FILL, GTK_FILL, 0, 6);
      gtk_widget_show (sep);
      row++;
    }

  /* out parameters */
  if (dbbrowser->selected_nreturn_vals)
    {
      label = gtk_label_new (_("Out:"));
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5); 
      gtk_table_attach (GTK_TABLE (dbbrowser->description), label,
			0, 1, row, row + (dbbrowser->selected_nreturn_vals), 
			GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (label);

      for (i = 0; i < dbbrowser->selected_nreturn_vals; i++) 
	{
	  /* name */
	  label = gtk_label_new ((dbbrowser->selected_return_vals[i]).name);
	  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5); 
	  gtk_table_attach (GTK_TABLE (dbbrowser->description), label,
			    1, 2, row, row + 1, GTK_FILL, GTK_FILL, 0, 0);
	  gtk_widget_show (label);

	  /* type */
	  type = GParamType2char (dbbrowser->selected_return_vals[i].type);
	  label = gtk_label_new (type);
	  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	  gtk_table_attach (GTK_TABLE (dbbrowser->description), label,
			    2, 3, row, row + 1, GTK_FILL, GTK_FILL, 0, 0);
	  gtk_widget_show (label);

	  /* description */
	  label = gtk_label_new (dbbrowser->selected_return_vals[i].description);
	  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5); 
	  gtk_table_attach (GTK_TABLE (dbbrowser->description), label,
			    3, 4, row, row + 1, GTK_FILL, GTK_FILL, 0, 0);
	  gtk_widget_show (label);

	  row++;
	}
    }

  if (dbbrowser->selected_nparams || 
      dbbrowser->selected_nreturn_vals)
    {
      sep = gtk_hseparator_new ();
      gtk_table_attach (GTK_TABLE (dbbrowser->description), sep,
			0, 4, row, row + 1, GTK_FILL, GTK_FILL, 0, 6);
      gtk_widget_show (sep);
      row++;
    }

  /* show the help */
  if (dbbrowser->selected_proc_help &&
      (strlen (dbbrowser->selected_proc_help) > 1))
    {
      label = gtk_label_new (dbbrowser->selected_proc_help);
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);

      gimp_table_attach_aligned (GTK_TABLE (dbbrowser->description), 0, row++,
				 _("Help:"), 1.0, 0.5,
				 label, 3, FALSE);

      sep = gtk_hseparator_new ();
      gtk_table_attach (GTK_TABLE (dbbrowser->description), sep,
			0, 4, row, row + 1, GTK_FILL, GTK_FILL, 0, 6);
      gtk_widget_show (sep);

      row++;
    }

  /* show the author & the copyright */

  label = gtk_label_new (dbbrowser->selected_proc_author);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  gtk_table_set_row_spacing (GTK_TABLE (dbbrowser->description), row, 2);
  gimp_table_attach_aligned (GTK_TABLE (dbbrowser->description), 0, row++,
			     _("Author:"), 1.0, 0.5,
			     label, 3, FALSE);

  label = gtk_label_new (dbbrowser->selected_proc_date);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  gtk_table_set_row_spacing (GTK_TABLE (dbbrowser->description), row, 2);
  gimp_table_attach_aligned (GTK_TABLE (dbbrowser->description), 0, row++,
			     _("Date:"), 1.0, 0.5,
			     label, 3, FALSE);

  label = gtk_label_new (dbbrowser->selected_proc_copyright);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  gtk_table_set_row_spacing (GTK_TABLE (dbbrowser->description), row, 2);
  gimp_table_attach_aligned (GTK_TABLE (dbbrowser->description), 0, row++,
			     _("Copyright:"), 1.0, 0.5,
			     label, 3, FALSE);

  if (old_description)
    gtk_container_remove (GTK_CONTAINER (dbbrowser->descr_vbox),
                          old_description);

  gtk_box_pack_start (GTK_BOX (dbbrowser->descr_vbox),
                      dbbrowser->description, FALSE, FALSE, 0);
  gtk_widget_show (dbbrowser->description);
  

  /* call GAP constraint functions to check sensibility for the apply buttons */ 
  if(dbbrowser->app_const_button != NULL)
  {
     if(0 != (dbbrowser->constraint_func_sel1)(dbbrowser->selected_proc_name, dbbrowser->current_image_id))
     { 
       gtk_widget_set_sensitive (dbbrowser->app_const_button, TRUE);
     }
     else 
     {
       gtk_widget_set_sensitive (dbbrowser->app_const_button, FALSE);
     }
  }
  if(dbbrowser->app_vary_button != NULL)
  {
     if(0 != (dbbrowser->constraint_func_sel2)(dbbrowser->selected_proc_name, dbbrowser->current_image_id))
     { 
        gtk_widget_set_sensitive (dbbrowser->app_vary_button, TRUE);
     }
     else
     {
        gtk_widget_set_sensitive (dbbrowser->app_vary_button, FALSE);
     }
  }
  
}

static void
dialog_show_message (dbbrowser_t *dbbrowser,
                     const gchar *message)
{
  if (dbbrowser->description && GTK_IS_LABEL (dbbrowser->description))
    {
      gtk_label_set_text (GTK_LABEL (dbbrowser->description), message);
    }
  else
    {
      if (dbbrowser->description)
        gtk_container_remove (GTK_CONTAINER (dbbrowser->descr_vbox),
                              dbbrowser->description);

      dbbrowser->description = gtk_label_new (message);
      gtk_box_pack_start (GTK_BOX (dbbrowser->descr_vbox),
                          dbbrowser->description, FALSE, FALSE, 0);
      gtk_widget_show (dbbrowser->description);
    }

  while (gtk_events_pending ())
    gtk_main_iteration ();
}

/* end of the dialog */
static void
dialog_close_callback (GtkWidget   *widget, 
		       dbbrowser_t *dbbrowser)
{
      /* we are in the plug_in : kill the gtk application */
      gtk_main_quit ();
}

/* GAP dialog_num_button_callback (end of the dialog) */
static void 
dialog_num_button_callback (dbbrowser_t* dbbrowser, 
		            gint button_nr)
{
  if (dbbrowser->selected_proc_name==NULL) return;

  strcpy(dbbrowser->result->selected_proc_name, dbbrowser->selected_proc_name);
  dbbrowser->result->button_nr = button_nr;
  
  gtk_widget_hide(dbbrowser->dialog);
  gtk_widget_destroy(dbbrowser->dialog);
  gtk_main_quit ();
}  /* end dialog_num_button_callback */


/* GAP APPLY const callback */
static void 
dialog_button_1_callback (GtkWidget *widget, dbbrowser_t* dbbrowser)
{
  dialog_num_button_callback(dbbrowser, 0);
}

/* GAP APPLY varying callback */
static void 
dialog_button_2_callback (GtkWidget *widget, dbbrowser_t* dbbrowser)
{
  dialog_num_button_callback(dbbrowser, 1);
}

/* CODEGEN callback (does not close the dialog) */
static void 
dialog_button_3_callback (GtkWidget *widget, dbbrowser_t* dbbrowser)
{
  p_remove_codegen_files();      /* remove old versions of generated CODE */
  dbbrowser->codegen_flag = 1;   /* let dialog_search_callback generate */
  dialog_search_callback(dbbrowser->name_button, (gpointer)dbbrowser );
  dbbrowser->codegen_flag = 0;
}  /* end dialog_button_3_callback */


/* search in the whole db */
static void 
dialog_search_callback (GtkWidget   *widget, 
		        dbbrowser_t *dbbrowser)
{
  gchar       **proc_list;
  gint          num_procs;
  gint          i;
  gint          i_added;
  gchar        *label;
  const gchar  *query_text;
  GString      *query;
  GtkTreeIter   iter;
  const char  *pattern;

  gtk_tree_view_set_model (GTK_TREE_VIEW (dbbrowser->tv), NULL);

  /* search */

  if (widget == dbbrowser->name_button)
    {
      dialog_show_message (dbbrowser, _("Searching by name - please wait"));

      query = g_string_new ("");
      query_text = gtk_entry_get_text (GTK_ENTRY (dbbrowser->search_entry));

      while (*query_text)
	{
	  if ((*query_text == '_') || (*query_text == '-'))
	    g_string_append (query, "[-_]");
	  else
	    g_string_append_c (query, *query_text);

	  query_text++;
	}

      gimp_procedural_db_query (query->str,
			        ".*", ".*", ".*", ".*", ".*", ".*", 
			        &num_procs, &proc_list);

      g_string_free (query, TRUE);
    }
  else if (widget == dbbrowser->blurb_button)
    {
      dialog_show_message (dbbrowser, _("Searching by blurb - please wait"));

      gimp_procedural_db_query (".*", 
			        (gchar *) gtk_entry_get_text
				  (GTK_ENTRY (dbbrowser->search_entry)),
			        ".*", ".*", ".*", ".*", ".*",
			        &num_procs, &proc_list);
    }
  else
    {
      if (widget == dbbrowser->menupath_button)
        {
          dialog_show_message (dbbrowser, _("Searching by menupath - please wait"));
	}
	else
        {
          dialog_show_message (dbbrowser, _("Searching - please wait"));
	}

      gimp_procedural_db_query (".*", ".*", ".*", ".*", ".*", ".*", ".*", 
			        &num_procs, &proc_list);
    }

  dbbrowser->store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
  gtk_tree_view_set_model (GTK_TREE_VIEW (dbbrowser->tv),
                           GTK_TREE_MODEL (dbbrowser->store));
  g_object_unref (dbbrowser->store);

 
  pattern = NULL;
  query_text = gtk_entry_get_text (GTK_ENTRY (dbbrowser->search_entry));
  if (query_text && strlen (query_text))
    pattern = query_text;

  i_added = 0;
  for (i = 0; i < num_procs; i++)
    {
      gboolean pre_selection;
      
      pre_selection = FALSE;
      if (widget == dbbrowser->menupath_button)
        {
	  gchar *menu_path;
	  
 	  menu_path = p_get_plugin_menupath(proc_list[i]);
	  if(menu_path)
	    {
	      if(pattern)
	        {
	          if(p_match_name(menu_path
		                 , pattern
		                 , MTCH_ANYWHERE  /* gint32 mode */
			         , FALSE          /* NOT case_sensitive */
	            ))
		    {
	              pre_selection = TRUE;  /* select matching menupath */
		    }
		}
		else
		{
	          pre_selection = TRUE;  /* if we dont have a pattern select all */
		}
	    }
	  g_free(menu_path);
	}
	else
	{
	  /* for all other cases than menupath_button 
	   * the pre_selection was already done at gimp_procedural_db_query 
	   */
	  pre_selection = TRUE;  
	}
	
      if (pre_selection)
        {
	  /* the filter constraint_function checks if the
	   * PDB-proc has a typical interface to operate on a single drawable.
	   * all other PDB-procedures are not listed in the GAP-dbbrowser
	   */
	  if (0 != (dbbrowser->constraint_func)((char *)proc_list[i], dbbrowser->current_image_id))
            {
              i_added++;
	      
	      if((dbbrowser->codegen_flag != 0) && (gap_debug))
	        {
                  p_gen_forward_iter_ALT(proc_list[i]);
                  p_gen_tab_iter_ALT(proc_list[i]);
                  p_gen_code_iter_ALT(proc_list[i]);
		}
              label = g_strdup (proc_list[i]);
              convert_string (label);
              gtk_list_store_append (dbbrowser->store, &iter);
              gtk_list_store_set (dbbrowser->store, &iter,
    			      0, label,
			      1, proc_list[i],
			      -1);
              g_free (label);
            }
	}
      g_free (proc_list[i]);
    }

  g_free (proc_list);

  /* now sort the store */ 
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (dbbrowser->store),
                                        0, GTK_SORT_ASCENDING);
  
  if (i_added > 0)
    {
      gtk_tree_model_get_iter_root (GTK_TREE_MODEL (dbbrowser->store), &iter);
      gtk_tree_selection_select_iter (dbbrowser->sel, &iter);
    }
  else
    {
      dialog_show_message (dbbrowser, _("No matches"));
    }
}

/* utils ... */

static void 
convert_string (gchar *str)
{
  while (*str)
    {
      if (*str == '_')
	*str = '-';

      str++;
    }
}

static const gchar * 
GParamType2char (GimpPDBArgType t)
{
  switch (t)
    {
    case GIMP_PDB_INT32:       return "INT32";
    case GIMP_PDB_INT16:       return "INT16";
    case GIMP_PDB_INT8:        return "INT8";
    case GIMP_PDB_FLOAT:       return "FLOAT";
    case GIMP_PDB_STRING:      return "STRING";
    case GIMP_PDB_INT32ARRAY:  return "INT32ARRAY";
    case GIMP_PDB_INT16ARRAY:  return "INT16ARRAY";
    case GIMP_PDB_INT8ARRAY:   return "INT8ARRAY";
    case GIMP_PDB_FLOATARRAY:  return "FLOATARRAY";
    case GIMP_PDB_STRINGARRAY: return "STRINGARRAY";
    case GIMP_PDB_COLOR:       return "COLOR";
    case GIMP_PDB_REGION:      return "REGION";
    case GIMP_PDB_DISPLAY:     return "DISPLAY";
    case GIMP_PDB_IMAGE:       return "IMAGE";
    case GIMP_PDB_LAYER:       return "LAYER";
    case GIMP_PDB_CHANNEL:     return "CHANNEL";
    case GIMP_PDB_DRAWABLE:    return "DRAWABLE";
    case GIMP_PDB_SELECTION:   return "SELECTION";
    case GIMP_PDB_BOUNDARY:    return "BOUNDARY";
    case GIMP_PDB_PATH:        return "PATH";
    case GIMP_PDB_PARASITE:    return "PARASITE";
    case GIMP_PDB_STATUS:      return "STATUS";
    case GIMP_PDB_END:         return "END";
    default:                   return "UNKNOWN?";
    }
}



/* search for menupath
 *  return NULL if menupath for plugin name was not found.
 *  return menupath if name was found
 *  the caller should g_free the rturned string.
 */
static gchar*
p_get_plugin_menupath (const gchar *search_text)
{
  static GimpParam *return_vals = NULL;
  static gboolean initialized = FALSE;
  gint nreturn_vals;
  gchar **menu_strs;
  gchar **accel_strs;
  gchar **prog_strs;
  gchar **types_strs;
  gchar **realname_strs;
  gint  *time_ints;
  
  gchar *menu_path;

  menu_path = NULL;
 
  if (!initialized)
    {
      /* query for all elements (only at 1.st call in this process)
       *  if we apply the search string to gimp_plugins_query
       *  we get no result, because the search will query MenuPath
       *  and not for the realname of the plug-in)
       */
      return_vals = gimp_run_procedure ("gimp_plugins_query",
                                    &nreturn_vals,
				    GIMP_PDB_STRING,  "", /* search all elements*/
                                    GIMP_PDB_END);

      if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
        {
	  initialized = TRUE;
	}
     
    }

  if (initialized)
    {
      int loop;
      int num_plugins;
      
      num_plugins        = return_vals[1].data.d_int32;
      menu_strs          = return_vals[2].data.d_stringarray;
      accel_strs         = return_vals[4].data.d_stringarray;
      prog_strs          = return_vals[6].data.d_stringarray;
      types_strs         = return_vals[8].data.d_stringarray;
      time_ints          = return_vals[10].data.d_int32array;
      realname_strs      = return_vals[12].data.d_stringarray;


      for (loop = 0; loop < return_vals[1].data.d_int32; loop++)
	{
	  if(strcmp(search_text, realname_strs[loop]) == 0)
	    {
	      menu_path     = g_strdup (menu_strs[loop]);
	      break;  /* stop at 1st match */
	    }
	}
    }

  /* Dont Destroy, but Keep the Returned Parameters for the lifetime of this process */
  /* gimp_destroy_params (return_vals, nreturn_vals); */

  return (menu_path);
}  /* end p_get_plugin_menupath */

