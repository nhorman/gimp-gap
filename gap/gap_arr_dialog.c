/* gap_arr_dialog.c
 * 1998.June.29 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * This is the common GTK Dialog for most of the GAP Functions.
 * (it replaces the older gap_sld_dialog module)
 *
 * - p_array_dialog   Dialog Window with one or more rows
 *                    each row can contain one of the following GAP widgets:
 *                       - float pair widget
 *                         (horizontal slidebar combined with a float input field)
 *                       - int pair widget
 *                         (horizontal slidebar combined with a int input field)
 *                       - Toggle Button widget
 *                       - Textentry widget
 *                       - Float entry widget
 *                       - Int entry widget
 *
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
 * gimp    1.3.17b; 2003/07/31  hof: message text fixes for translators (# 118392)
 * gimp    1.3.17a; 2003/07/28  hof: gimp_interactive_selection_font was renamed to:  gimp_font_select_new
 * gimp    1.3.16c; 2003/07/12  hof: new p_confirm_dialog
 * gimp    1.3.15a; 2003/06/21  hof: textspacing
 * gimp    1.3.14b; 2003/06/03  hof: added gap_stock_init
 * gimp    1.3.14a; 2003/05/19  hof: GUI standard (OK ist rightmost button)
 *                                   changed WGT_INT, and WGT_FLT from entry to spinbutton
 *                                   added WGT_FONTSEL
 * gimp    1.3.12a; 2003/05/03  hof: merge into CVS-gimp-gap project, replace gimp_help_init by _gimp_help_init
 * gimp    1.3.11a; 2003/01/18  hof: merged in changes of the gap_vid_enc project
 *                                   - added WGT_OPT_ENTRY (entry comined with Optionmenu) and WGT_DEFAULT_BUTTON
 * gimp    1.3.4a;  2002/03/12  hof: ported to gtk+-2.0.0
 *                                   radio_create_value (prevent fire callback to early, needed in new gtk)
 *                                   bugfix: optionmenu_create_value active menu entry now can have any position
 *                                   (dont change menuorder any longer, where active item was placed 1st)
 * gimp    1.1.17b; 2000/01/26  hof: bugfix gimp_help_init
 *                                   use gimp_scale_entry_new for WGT_FLT_PAIR, WGT_INT_PAIR
 * gimp    1.1.17a; 2000/02/20  hof: use gimp_help_set_help_data for tooltips
 * gimp    1.1.13b; 1999/12/04  hof: some cosmetic gtk fixes
 *                                   changed border_width spacing and Buttons in action area
 *                                   to same style as used in dialogs of the gimp 1.1.13 main dialogs
 * gimp    1.1.11b; 1999/11/20  hof: some cosmetic gtk fixes:
 *                                   - allow X-expansion (useful for the scale widgets)
 *                                   - use a hbox on WGT_INT_PAIR and WGT_FLT_PAIR
 *                                     (reduces the waste of horizontal space
 *                                      when used together with other widget types in the table)
 * gimp    1.1.5.1; 1999/05/08  hof: call fileselect in gtk+1.2 style 
 * version 0.96.03; 1998/08/15  hof: p_arr_gtk_init 
 * version 0.96.01; 1998/07/09  hof: Bugfix: gtk_init should be called only
 *                                           once in a plugin process 
 * version 0.96.00; 1998/07/09  hof: 1.st release 
 *                                   (re-implementation of gap_sld_dialog.c)
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

/* GIMP includes */
#include "gtk/gtk.h"
#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

/* private includes */
#include "gap_arr_dialog.h"
#include "gap_stock.h"
#include "gap-intl.h"

typedef void (*t_entry_cb_func) (GtkWidget *widget, t_arr_arg *arr_ptr);

typedef struct
{
  t_arr_arg *arr_ptr;
  gint       radio_index;
} t_radio_arg;

typedef struct {
  GtkWidget *dlg;
  gint run;
} t_arr_interface;

typedef struct
{
  t_arr_arg *argv;
  gint       argc;
}   t_all_arr_args;

static t_arr_interface g_arrint =
{
  NULL,     /*  dlg  */
  FALSE     /*  run  */
};


extern      int gap_debug; /* ==0  ... dont print debug infos */

/* Declare local functions.
 */

static void   arr_close_callback        (GtkWidget *widget, gpointer data);
static void   but_array_callback           (GtkWidget *widget, gpointer data);

static void   entry_create_value        (char *title, GtkTable *table, int row, t_arr_arg *arr_ptr,
                                         t_entry_cb_func entry_update_cb, char *init_txt);
static void   label_create_value         (char *title, GtkTable *table, int row, t_arr_arg *arr_ptr,
                                          gfloat align);
static void   text_entry_update_cb       (GtkWidget *widget, t_arr_arg *arr_ptr);
static void   text_create_value          (char *title, GtkTable *table, int row, t_arr_arg *arr_ptr);
static void   filesel_close_cb           (GtkWidget *widget, t_arr_arg *arr_ptr);
static void   filesel_ok_cb              (GtkWidget *widget, t_arr_arg *arr_ptr);
static void   filesel_open_cb            (GtkWidget *widget, t_arr_arg *arr_ptr);
static void   filesel_create_value       (char *title, GtkTable *table, int row, t_arr_arg *arr_ptr);

static void   int_create_value           (char *title, GtkTable *table, int row, t_arr_arg *arr_ptr);
static void   flt_create_value           (char *title, GtkTable *table, int row, t_arr_arg *arr_ptr);

static void   toggle_update_cb           (GtkWidget *widget, t_arr_arg *arr_ptr);
static void   toggle_create_value        (char *title, GtkTable *table, int row, t_arr_arg *arr_ptr);

static void   radio_update_cb            (GtkWidget *widget, t_radio_arg *radio_ptr);
static void   radio_create_value         (char *title, GtkTable *table, int row, t_arr_arg *arr_ptr);
static void   optionmenu_create_value    (char *title, GtkTable *table, int row, t_arr_arg *arr_ptr);

static void   pair_int_create_value     (gchar *title, GtkTable *table, gint row, t_arr_arg *arr_ptr);
static void   pair_flt_create_value     (gchar *title, GtkTable *table, gint row, t_arr_arg *arr_ptr);

static void   default_button_cb           (GtkWidget *widget, t_all_arr_args *arr_all);
static void   default_button_create_value (char *title, GtkWidget *hbox, t_arr_arg *arr_ptr, t_all_arr_args *arr_all);



static void
arr_close_callback (GtkWidget *widget,
		       gpointer   data)
{
  gtk_widget_destroy (GTK_WIDGET (g_arrint.dlg));  /* close & destroy dialog window */
  gtk_main_quit ();
}

static void
but_array_callback (GtkWidget *widget,
		    gpointer   data)
{
  g_arrint.run = *((gint *)data);                  /* set returnvalue according to button */
  gtk_widget_destroy (GTK_WIDGET (g_arrint.dlg));  /* close & destroy dialog window */
  gtk_main_quit ();
}


static void
entry_create_value(char *title, GtkTable *table, int row, t_arr_arg *arr_ptr,
                   t_entry_cb_func entry_update_cb, char *init_txt)
{
    GtkWidget *entry;
    GtkWidget *label;


    label = gtk_label_new(title);
    gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
    gtk_table_attach(table, label, 0, 1, row, row + 1, GTK_FILL, GTK_FILL, 0, 0);
    gtk_widget_show(label);

    entry = gtk_entry_new();
    gtk_widget_set_size_request(entry, arr_ptr->entry_width, -1);
    gtk_entry_set_text(GTK_ENTRY(entry), init_txt);
    gtk_table_attach(GTK_TABLE(table), entry, 1, 2, row, row + 1, GTK_FILL, GTK_FILL | GTK_EXPAND, 4, 0);
    if(arr_ptr->help_txt != NULL)
    { 
       gimp_help_set_help_data(entry, arr_ptr->help_txt,NULL);
    }
    gtk_widget_show(entry);
    g_signal_connect(G_OBJECT(entry), "changed",
		     G_CALLBACK (entry_update_cb),
		     arr_ptr);
    
    arr_ptr->text_entry = entry;
}

/* --------------------------
 * LABEL
 * --------------------------
 */

static void
label_create_value(char *title, GtkTable *table, int row, t_arr_arg *arr_ptr, gfloat align)
{
    GtkWidget *label;
    GtkWidget *hbox;

    label = gtk_label_new(title);
    gtk_misc_set_alignment(GTK_MISC(label), align, 0.5);

    if(align != 0.5)
    {
      hbox = gtk_hbox_new (FALSE, 2);
      gtk_widget_show (hbox);
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
      gtk_table_attach(table, hbox, 0, 3, row, row + 1, GTK_FILL, GTK_FILL, 4, 0);
    }
    else
    {
      gtk_table_attach(table, label, 0, 3, row, row + 1, GTK_FILL, GTK_FILL, 4, 0);
    }

    gtk_widget_show(label);
}

/* --------------------------
 * DEFAULT_BUTTON
 * --------------------------
 */

static void
default_button_create_value(char *title, GtkWidget *hbox, t_arr_arg *arr_ptr, t_all_arr_args *arr_all)
{
    GtkWidget *button;

    button = gtk_button_new_from_stock (GIMP_STOCK_RESET);    /*button = gtk_button_new_with_label (title);*/
    
    gtk_widget_show(button);
    g_signal_connect (G_OBJECT (button), "clicked",
		      G_CALLBACK(default_button_cb),
		      arr_all);
 
    gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);

    /* gtk_table_attach(table, button, 0, 3, row, row + 1, GTK_FILL, 0, 0, 0); */
    if(arr_ptr->help_txt != NULL)
    { 
       gimp_help_set_help_data(button, arr_ptr->help_txt,NULL);
    }
}  /* end default_button_create_value */

static void
default_button_cb(GtkWidget *widget,  t_all_arr_args *arr_all)
{
  gint l_idx;
  t_arr_arg  *arr_ptr;
  char       *buf;
  char       *fmt;

  if(arr_all == NULL)
  {
    return;
  }
  
  for(l_idx = 0; l_idx < arr_all->argc; l_idx++)
  {
     arr_ptr = &arr_all->argv[l_idx];
     buf = NULL;
 
     if(!arr_ptr->has_default)
     {
       continue;
     }
     
     switch(arr_ptr->widget_type)
     {
        case WGT_TEXT:
        case WGT_FONTSEL:
        case WGT_OPT_ENTRY:
          if((arr_ptr->text_buf_ret != NULL) && (arr_ptr->text_buf_default))
          {
             strncpy(arr_ptr->text_buf_ret, arr_ptr->text_buf_default, arr_ptr->text_buf_len -1);
             buf = g_strdup(arr_ptr->text_buf_default);
          }
	  if(arr_ptr->widget_type == WGT_OPT_ENTRY)
	  {
            arr_ptr->radio_ret = arr_ptr->radio_default;
            gtk_option_menu_set_history (GTK_OPTION_MENU (arr_ptr->option_menu), arr_ptr->radio_ret);
	  }
	  break;
        case WGT_FLT_PAIR:
        case WGT_FLT:  
          arr_ptr->flt_ret = arr_ptr->flt_default;
	  fmt = g_strdup_printf("%%.%df", arr_ptr->flt_digits);
          buf = g_strdup_printf(fmt, arr_ptr->flt_ret);
          g_free(fmt);  
          if(arr_ptr->adjustment)
          {
            gtk_adjustment_set_value (GTK_ADJUSTMENT (arr_ptr->adjustment), (gfloat)arr_ptr->flt_default);
          }
	  break;
        case WGT_INT_PAIR:
        case WGT_INT:
          arr_ptr->int_ret = arr_ptr->int_default;
          buf = g_strdup_printf("%d", arr_ptr->int_ret);
          if(arr_ptr->adjustment)
          {
            gtk_adjustment_set_value (GTK_ADJUSTMENT (arr_ptr->adjustment), (gfloat)arr_ptr->int_default);
          }
 	  break;
        case WGT_TOGGLE:
          arr_ptr->int_ret = arr_ptr->int_default;
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (arr_ptr->check_button),
				arr_ptr->int_default);
 	  break;
        case WGT_RADIO:
          arr_ptr->radio_ret = arr_ptr->radio_default;
	  if (gap_debug) printf("ERROR: default for WGT_RADIO not impemented\n");
 	  break;
        case WGT_OPTIONMENU:
         arr_ptr->radio_ret = arr_ptr->radio_default;
         gtk_option_menu_set_history (GTK_OPTION_MENU (arr_ptr->option_menu), arr_ptr->radio_ret);
         break;
        default :
         break;
     }   /* end switch */

     if(buf != NULL)
     {
       if(arr_ptr->text_entry != NULL)
       {
         gtk_entry_set_text(GTK_ENTRY(arr_ptr->text_entry), buf);
       }
       g_free(buf);
     }
  }      /* end for */
}  /* end default_button_cb */


/* --------------------------
 * FONTSEL
 * --------------------------
 */

static void
p_font_callback   (gchar    *name
		  ,gboolean  dialog_closing
		  ,gpointer  user_data)
{
  t_arr_arg *arr_ptr;
  
  if(gap_debug) printf("p_font_callback: %s  closing:%d\n", name, (int)dialog_closing);

  arr_ptr = (t_arr_arg *)user_data;
  if(arr_ptr)
  {
    g_snprintf(arr_ptr->text_buf_ret, arr_ptr->text_buf_len, "%s", name);
    gtk_entry_set_text(GTK_ENTRY(arr_ptr->text_entry), arr_ptr->text_buf_ret);
  }
  
  if(dialog_closing)
  {
    arr_ptr->text_fontsel = NULL;
  }
}
                  

static void
fontsel_open_cb(GtkWidget *widget, t_arr_arg *arr_ptr)
{
  const gchar *fontsel;

  if(arr_ptr->text_fontsel != NULL) return;  /* fontsel is already open */

  fontsel = gimp_font_select_new (arr_ptr->label_txt       /*    *dialogname */
                                 , arr_ptr->text_buf_ret   /*    *font_name */
                                 , (GimpRunFontCallback) p_font_callback
                                 , arr_ptr                 /* gpointer   data */
                                 );

  arr_ptr->text_fontsel = fontsel;
}


static void
fontsel_create_value(char *title, GtkTable *table, int row, t_arr_arg *arr_ptr)
{
  GtkWidget *button;

  entry_create_value(title, table, row, arr_ptr, text_entry_update_cb, arr_ptr->text_buf_ret);
  gtk_widget_set_sensitive(arr_ptr->text_entry ,FALSE);
  arr_ptr->text_fontsel = NULL;
    
  /* Button  to invoke fontbrowser */  
  button = gtk_button_new_with_label ( _("Font Browser"));
  gtk_table_attach( GTK_TABLE(table), button, 2, 3, row, row +1,
		    0, 0, 0, 0 );
  if(arr_ptr->help_txt != NULL)
  { 
       gimp_help_set_help_data(button, arr_ptr->help_txt,NULL);
  }
  gtk_widget_show (button);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (fontsel_open_cb),
		    arr_ptr);   
}


/* --------------------------
 * FILESEL
 * --------------------------
 */
static void
filesel_close_cb(GtkWidget *widget, t_arr_arg *arr_ptr)
{
  if(arr_ptr->text_filesel == NULL) return;  /* filesel is already closed */

  gtk_widget_destroy(GTK_WIDGET(arr_ptr->text_filesel));
  arr_ptr->text_filesel = NULL;
}
static void
filesel_ok_cb(GtkWidget *widget, t_arr_arg *arr_ptr)
{
  const gchar *filename;

  if(arr_ptr->text_filesel == NULL) return;  /* filesel is already closed */

  filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (arr_ptr->text_filesel));
  strncpy(arr_ptr->text_buf_ret, filename, arr_ptr->text_buf_len -1);

  gtk_entry_set_text(GTK_ENTRY(arr_ptr->text_entry), filename);

  filesel_close_cb(widget, arr_ptr);
}

static void
filesel_open_cb(GtkWidget *widget, t_arr_arg *arr_ptr)
{
  GtkWidget *filesel;

  if(arr_ptr->text_filesel != NULL) return;  /* filesel is already open */

  filesel = gtk_file_selection_new (arr_ptr->label_txt);
  arr_ptr->text_filesel = filesel;

  gtk_window_set_position (GTK_WINDOW (filesel), GTK_WIN_POS_MOUSE);

  gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesel),
				   arr_ptr->text_buf_ret);
  gtk_widget_show (filesel);

  g_signal_connect (G_OBJECT (filesel), "destroy",
		    G_CALLBACK (filesel_close_cb),
		    arr_ptr);

  g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button),
		   "clicked",
                    G_CALLBACK (filesel_ok_cb),
		    arr_ptr);
  g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (filesel)->cancel_button),
		   "clicked",
                    G_CALLBACK (filesel_close_cb),
		    arr_ptr);
}


static void
filesel_create_value(char *title, GtkTable *table, int row, t_arr_arg *arr_ptr)
{
  GtkWidget *button;

  entry_create_value(title, table, row, arr_ptr, text_entry_update_cb, arr_ptr->text_buf_ret);
  arr_ptr->text_filesel = NULL;
    
  /* Button  to invoke filebrowser */  
  button = gtk_button_new_with_label ( _("File Browser"));
  gtk_table_attach( GTK_TABLE(table), button, 2, 3, row, row +1,
		    0, 0, 0, 0 );
  gtk_widget_show (button);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (filesel_open_cb),
		    arr_ptr);
    
}

/* --------------------------
 * TEXT
 * --------------------------
 */
static void
text_entry_update_cb(GtkWidget *widget, t_arr_arg *arr_ptr)
{
  if((arr_ptr->widget_type != WGT_TEXT)
  && (arr_ptr->widget_type != WGT_FONTSEL)
  && (arr_ptr->widget_type != WGT_FILESEL))
  {
    return;
  }
  
  strncpy(arr_ptr->text_buf_ret,
          gtk_entry_get_text(GTK_ENTRY(widget)), 
          arr_ptr->text_buf_len -1);
  arr_ptr->text_buf_ret[arr_ptr->text_buf_len -1] = '\0';
}

static void
text_create_value(char *title, GtkTable *table, int row, t_arr_arg *arr_ptr)
{
    entry_create_value(title, table, row, arr_ptr, text_entry_update_cb, arr_ptr->text_buf_ret);
}


/* --------------------------
 * SPIN  (INT and FLT)
 * --------------------------
 */

static void
spin_create_value(char *title, GtkTable *table, int row, t_arr_arg *arr_ptr
                , gboolean int_flag
                , gdouble un_min
                , gdouble un_max
                )
{
  GtkWidget *label;
  GtkWidget *spinbutton;
  GtkObject *adj;
  GtkWidget *hbox1;
  GtkWidget *label2;

  gdouble     umin, umax;
  gdouble     sstep;
  gdouble     initial_val;
  gint        l_digits;

  label = gtk_label_new(title);
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(table, label, 0, 1, row, row + 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);

  if(int_flag)
  {
    sstep = arr_ptr->int_step;
    initial_val = (gdouble)   arr_ptr->int_ret;
    l_digits = 0;
  }
  else
  {
    sstep = arr_ptr->flt_step;
    initial_val = (gdouble)   arr_ptr->flt_ret;
    l_digits = arr_ptr->flt_digits;
  }
  
  if(arr_ptr->constraint)
  {
    umin = un_min;
    umax = un_max;
  }
  else
  {
    umin = arr_ptr->umin;
    umax = arr_ptr->umax;
  }

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox1);

  spinbutton = gimp_spin_button_new (&adj,  /* return value */
		      initial_val,
		      umin,
		      umax,
		      sstep,
		      (gdouble)   arr_ptr->pagestep,
		      0.0,                 /* page_size */
		      1.0,                 /* climb_rate */
		      l_digits              /* digits */
                      );
 
  gtk_widget_set_size_request(spinbutton, arr_ptr->entry_width, -1);
  gtk_widget_show (spinbutton);

  label2 = gtk_label_new(" ");
  gtk_widget_show(label2);

  gtk_box_pack_start (GTK_BOX (hbox1), spinbutton, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox1), label2, TRUE, TRUE, 0);

  gtk_table_attach(GTK_TABLE(table), hbox1, 1, 2, row, row + 1, GTK_FILL, GTK_FILL, 4, 0);
  if(arr_ptr->help_txt != NULL)
  { 
     gimp_help_set_help_data(spinbutton, arr_ptr->help_txt,NULL);
  }
  gtk_widget_show(spinbutton);

  if(int_flag)
  {
    g_signal_connect
	(G_OBJECT (adj), "value_changed",
	 G_CALLBACK (gimp_int_adjustment_update),
	 &arr_ptr->int_ret);
  }
  else
  {
    g_signal_connect
	(G_OBJECT (adj), "value_changed",
	 G_CALLBACK (gimp_double_adjustment_update),
	 &arr_ptr->flt_ret);
  }
  
    
  arr_ptr->adjustment = (GtkWidget *)adj;                
}  /* end spin_create_value */


/* --------------------------
 * INT
 * --------------------------
 */
static void
int_create_value(char *title, GtkTable *table, int row, t_arr_arg *arr_ptr)
{
  spin_create_value(title, table, row, arr_ptr
                   , TRUE  /* int_flag */
                   , (gdouble) arr_ptr->int_min
                   , (gdouble) arr_ptr->int_max
                   );
}
/* --------------------------
 * FLOAT
 * --------------------------
 */

static void
flt_create_value(char *title, GtkTable *table, int row, t_arr_arg *arr_ptr)
{
  spin_create_value(title, table, row, arr_ptr
                   , FALSE  /* int_flag */
                   , (gdouble) arr_ptr->flt_min
                   , (gdouble) arr_ptr->flt_max
                   );
}


/* --------------------------
 * TOGGLE
 * --------------------------
 */

static void
toggle_update_cb (GtkWidget *widget, t_arr_arg *arr_ptr)
{
  if(arr_ptr->widget_type !=WGT_TOGGLE) return;

  if (GTK_TOGGLE_BUTTON (widget)->active)
  {
    arr_ptr->int_ret = 1;
  }
  else
  {
    arr_ptr->int_ret = 0;
  }
}

static void
toggle_create_value(char *title, GtkTable *table, int row, t_arr_arg *arr_ptr)
{
  GtkWidget *check_button;
  GtkWidget *label;
  char      *l_togg_txt;


  label = gtk_label_new(title);
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach(table, label, 0, 1, row, row + 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);

  /* (make sure there is only 0 or 1) */
  if(arr_ptr->int_ret != 0) arr_ptr->int_ret = 1;

  if(arr_ptr->togg_label == NULL) l_togg_txt = " ";
  else                            l_togg_txt = arr_ptr->togg_label;
  /* check button */
  check_button = gtk_check_button_new_with_label (l_togg_txt);
  gtk_table_attach ( GTK_TABLE (table), check_button, 1, 3, row, row+1, GTK_FILL, 0, 0, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button),
				arr_ptr->int_ret);
  if(arr_ptr->help_txt != NULL)
  { 
     gimp_help_set_help_data(check_button, arr_ptr->help_txt,NULL);
  }
  gtk_widget_show (check_button);

  arr_ptr->check_button = check_button;

  g_signal_connect (G_OBJECT (check_button), "toggled",
                    G_CALLBACK (toggle_update_cb),
                    arr_ptr);
}


/* --------------------------
 * RADIO
 * --------------------------
 */

static void
radio_update_cb (GtkWidget *widget, t_radio_arg *radio_ptr)
{
  if(radio_ptr->arr_ptr == NULL) return;
  if(radio_ptr->arr_ptr->widget_locked) return;
  if((radio_ptr->arr_ptr->widget_type != WGT_RADIO)
  && (radio_ptr->arr_ptr->widget_type != WGT_OPTIONMENU)
  && (radio_ptr->arr_ptr->widget_type != WGT_OPT_ENTRY))
  {
    return;
  }
  
  radio_ptr->arr_ptr->int_ret = radio_ptr->radio_index;
  radio_ptr->arr_ptr->radio_ret = radio_ptr->radio_index;
  if(gap_debug) printf("radio_update_cb: radio_ret %d\n", (int)radio_ptr->arr_ptr->radio_ret );

  if((radio_ptr->arr_ptr->widget_type == WGT_OPT_ENTRY)
  && (radio_ptr->arr_ptr->radio_argv != NULL))
  {
    gtk_entry_set_text(GTK_ENTRY(radio_ptr->arr_ptr->text_entry), radio_ptr->arr_ptr->radio_argv[radio_ptr->radio_index]);

    strncpy(radio_ptr->arr_ptr->text_buf_ret,
          radio_ptr->arr_ptr->radio_argv[radio_ptr->radio_index],
          radio_ptr->arr_ptr->text_buf_len -1);
    radio_ptr->arr_ptr->text_buf_ret[radio_ptr->arr_ptr->text_buf_len -1] = '\0';

  }
}

static void
radio_create_value(char *title, GtkTable *table, int row, t_arr_arg *arr_ptr)
{
  GtkWidget *label;
  GtkWidget *radio_table;
  GtkWidget *radio_button;
  GSList    *radio_group = NULL;
  gint       l_idy;
  char      *l_radio_txt;
  char      *l_radio_help_txt;
  gint       l_radio_pressed;
  gint       l_int_ret_initial_value;


  t_radio_arg   *radio_ptr;

  l_int_ret_initial_value = arr_ptr->radio_ret;
  
  label = gtk_label_new(title);
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.0);
  gtk_table_attach( GTK_TABLE (table), label, 0, 1, row, row+1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);

  /* radio_table */
  radio_table = gtk_table_new (arr_ptr->radio_argc, 2, FALSE);

  for(l_idy=0; l_idy < arr_ptr->radio_argc; l_idy++)
  {
     radio_ptr          = g_malloc0(sizeof(t_radio_arg));
     radio_ptr->arr_ptr = arr_ptr;
     radio_ptr->radio_index = l_idy;
     
     if(arr_ptr->radio_ret == l_idy) l_radio_pressed  = TRUE;
     else                            l_radio_pressed  = FALSE;

     l_radio_txt = "null";
     if (arr_ptr->radio_argv != NULL)
     {
        if (arr_ptr->radio_argv[l_idy] != NULL)
            l_radio_txt = arr_ptr->radio_argv[l_idy];
     }

     l_radio_help_txt = arr_ptr->help_txt;
     if (arr_ptr->radio_help_argv != NULL)
     {
         if (arr_ptr->radio_help_argv[l_idy] != NULL)
            l_radio_help_txt = arr_ptr->radio_help_argv[l_idy];
     }
 
     if(gap_debug) printf("radio_create_value: %02d %s\n", l_idy, l_radio_txt);
    
     radio_button = gtk_radio_button_new_with_label ( radio_group, l_radio_txt );
     radio_group = gtk_radio_button_get_group ( GTK_RADIO_BUTTON (radio_button) );  
     gtk_table_attach ( GTK_TABLE (radio_table), radio_button, 0, 2, l_idy, l_idy+1, GTK_FILL | GTK_EXPAND, 0, 0, 0);

     gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_button), 
				   l_radio_pressed);
     if(l_radio_help_txt != NULL)
     { 
       gimp_help_set_help_data(radio_button, l_radio_help_txt, NULL);
     }
     gtk_widget_show (radio_button);

     g_signal_connect ( G_OBJECT (radio_button), "toggled",
		        G_CALLBACK (radio_update_cb),
		        radio_ptr);
  }

  
  /* attach radio_table */
  gtk_table_attach ( GTK_TABLE (table), radio_table, 1, 3, row, row+1, GTK_FILL | GTK_EXPAND, 0, 0, 0);
  gtk_widget_show (radio_table);

  /* the new gtk always calls the callback procedure of the 1st radio item,
   * if the 1.st item (with index 0) is not the selected one,
   * and the user makes no further selection
   * 0 would be returned as result (but this is wrong and not the expected value)
   * WORKAROUND:
   * copy initial value to int_ret 
   */
  arr_ptr->int_ret = l_int_ret_initial_value;
  arr_ptr->radio_ret = l_int_ret_initial_value;
}


/* --------------------------
 * OPTIONMENU
 * --------------------------
 */

/* optionmenus share callback and data structure with WGT_RADIO */
static void
optionmenu_create_value(char *title, GtkTable *table, int row, t_arr_arg *arr_ptr)
{
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget  *option_menu;
  GtkWidget  *menu;
  GtkWidget  *menu_item;
  gint       l_idx;
  gint       l_idy;
  char      *l_radio_txt;
  gint       l_col;

  t_radio_arg *l_menu_ptr;
  
  /* label */
  l_col = 0;
  label = gtk_label_new(title);
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_table_attach( GTK_TABLE (table), label, l_col, l_col+1, row, row+1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);

  /* entry */
  if(arr_ptr->widget_type == WGT_OPT_ENTRY)
  {
    l_col++;
    entry = gtk_entry_new();
    gtk_widget_set_size_request(entry, arr_ptr->entry_width, 0);
    gtk_entry_set_text(GTK_ENTRY(entry), arr_ptr->text_buf_ret);
    gtk_table_attach(GTK_TABLE(table), entry, l_col, l_col+1, row, row + 1, GTK_FILL, GTK_FILL | GTK_EXPAND, 4, 0);
    if(arr_ptr->help_txt != NULL)
    { 
       gimp_help_set_help_data(entry, arr_ptr->help_txt,NULL);
    }
    gtk_widget_show(entry);
    g_signal_connect(G_OBJECT(entry), "changed",
		     G_CALLBACK(text_entry_update_cb),
                     arr_ptr);
    
    arr_ptr->text_entry = entry;
  }

  /* optionmenu */
  l_col++;
  option_menu = gtk_option_menu_new();
  gtk_table_attach(GTK_TABLE(table), option_menu, l_col, l_col+1, row, row +1,
		   GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(option_menu);

  if(arr_ptr->help_txt != NULL)
  { 
       gimp_help_set_help_data(option_menu, arr_ptr->help_txt, NULL);
  }

  /* menu (filled with items in loop) */
  menu = gtk_menu_new ();

  l_idx=0;
  for(l_idy=0; l_idy < arr_ptr->radio_argc; l_idy++)
  {
     if(arr_ptr->radio_ret == l_idy) 
     {
       l_idx = l_idy; 
     }

     l_radio_txt = "null";
     if (arr_ptr->radio_argv != NULL)
     {
        if (arr_ptr->radio_argv[l_idy] != NULL)
            l_radio_txt = arr_ptr->radio_argv[l_idy];
     }
     l_menu_ptr   = g_malloc0(sizeof(t_radio_arg));
     l_menu_ptr->radio_index  = l_idy;
     l_menu_ptr->arr_ptr  = arr_ptr;


     menu_item = gtk_menu_item_new_with_label (l_radio_txt);
     gtk_container_add (GTK_CONTAINER (menu), menu_item);

     gtk_widget_show (menu_item);

     g_signal_connect (G_OBJECT (menu_item), "activate",
                       G_CALLBACK (radio_update_cb),
                       l_menu_ptr);
  }

  gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
  gtk_widget_show(option_menu);
  gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), l_idx);
  
  arr_ptr->option_menu = option_menu;

}


/* --------------------------
 * FLT_PAIR
 * --------------------------
 */

static void
pair_flt_create_value(gchar *title, GtkTable *table, gint row, t_arr_arg *arr_ptr)
{
  GtkObject *adj;
  gfloat     umin, umax;
  
  if(arr_ptr->constraint)
  {
    umin = arr_ptr->flt_min;
    umax = arr_ptr->flt_max;
  }
  else
  {
    umin = arr_ptr->umin;
    umax = arr_ptr->umax;
  }
  

  adj = 
  gimp_scale_entry_new( GTK_TABLE (table), 0, row,        /* table col, row */
		        title,                            /* label text */
		        arr_ptr->scale_width,             /* scalesize */
			arr_ptr->entry_width,             /* entrysize */
		       (gfloat)arr_ptr->flt_ret,          /* init value */
		       (gfloat)arr_ptr->flt_min,          /* lower,  */
		       (gfloat)arr_ptr->flt_max,          /* upper */
		        arr_ptr->flt_step,                /* step */
			arr_ptr->pagestep,                /* pagestep */
		        arr_ptr->flt_digits,              /* digits */
		        arr_ptr->constraint,              /* constrain */
		        umin, umax,                       /* lower, upper (unconstrained) */
		        arr_ptr->help_txt,                /* tooltip */
		        NULL);                            /* privatetip */

  g_signal_connect (G_OBJECT (adj), "value_changed",
		    G_CALLBACK (gimp_double_adjustment_update),
		    &arr_ptr->flt_ret);
  arr_ptr->adjustment = (GtkWidget *)adj;                
}

/* --------------------------
 * INT_PAIR
 * --------------------------
 */

static void
pair_int_create_value(gchar *title, GtkTable *table, gint row, t_arr_arg *arr_ptr)
{
  GtkObject *adj;
  gfloat     umin, umax;
  
  if(arr_ptr->constraint)
  {
    umin = (gfloat)arr_ptr->int_min;
    umax = (gfloat)arr_ptr->int_max;
  }
  else
  {
    umin = arr_ptr->umin;
    umax = arr_ptr->umax;
  }

  adj = 
  gimp_scale_entry_new( GTK_TABLE (table), 0, row,        /* table col, row */
		        title,                            /* label text */
		        arr_ptr->scale_width,             /* scalesize */
			arr_ptr->entry_width,             /* entrysize */
		       (gfloat)arr_ptr->int_ret,          /* init value */
		       (gfloat)arr_ptr->int_min,          /* lower,  */
		       (gfloat)arr_ptr->int_max,          /* upper */
		        arr_ptr->int_step,                /* step */
			arr_ptr->pagestep,                /* pagestep */
		        0,                                /* digits */
		        arr_ptr->constraint,              /* constrain */
		        umin, umax,                       /* lower, upper (unconstrained) */
		        arr_ptr->help_txt,                /* tooltip */
		        NULL);                            /* privatetip */

  g_signal_connect (G_OBJECT (adj), "value_changed",
		    G_CALLBACK (gimp_int_adjustment_update),
		    &arr_ptr->int_ret);

  arr_ptr->adjustment = (GtkWidget *)adj;                
}


/* ============================================================================
 * p_array_std_dialog
 *
 *   GTK dialog window that has argc rows.
 *   each row contains the widget(s) as defined in argv[row]
 *
 *   The Dialog has an Action Area with OK and CANCEL Buttons.
 * ============================================================================
 */
gint p_array_std_dialog(char *title_txt,
                        char *frame_txt,
                        int        argc,
                        t_arr_arg  argv[],
                        int        b_argc,
                        t_but_arg  b_argv[],
                        gint       b_def_val)
{
  GtkWidget *hbbox;
  GtkWidget *button;
  GtkWidget *frame;
  GtkWidget *table;
  gint    l_idx;
  gint    l_ok_value;
  char   *l_label_txt;
  t_arr_arg  *arr_ptr;
    
  g_arrint.run = b_def_val;           /* prepare default retcode (if window is closed without button) */
  l_ok_value = 0;
  table = NULL;
  
  if((argc > 0) && (argv == NULL))
  {
    printf("p_array_std_dialog: calling error (widget array == NULL)\n");
    return (g_arrint.run);
  }
  if((b_argc > 0) && (b_argv == NULL))
  {
    printf("p_array_std_dialog: calling error (button array == NULL)\n");
    return (g_arrint.run);
  }

  gimp_ui_init ("gap_std_dialog", FALSE);
  gap_stock_init();
  
  /* dialog */
  g_arrint.dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (g_arrint.dlg), title_txt);
  gtk_window_set_position (GTK_WINDOW (g_arrint.dlg), GTK_WIN_POS_MOUSE);
  g_signal_connect (G_OBJECT (g_arrint.dlg), "destroy",
		    G_CALLBACK (arr_close_callback),
		    NULL);

  gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (g_arrint.dlg)->action_area), 2);
  gtk_box_set_homogeneous (GTK_BOX (GTK_DIALOG (g_arrint.dlg)->action_area), FALSE);
  hbbox = gtk_hbutton_box_new ();
  gtk_box_set_spacing (GTK_BOX (hbbox), 4);
  gtk_box_pack_end (GTK_BOX (GTK_DIALOG (g_arrint.dlg)->action_area), hbbox, FALSE, FALSE, 0);
  gtk_widget_show (hbbox);


  /*  parameter settings  */
  if (frame_txt == NULL)   frame = gtk_frame_new ( _("Enter Values"));
  else                     frame = gtk_frame_new (frame_txt);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (g_arrint.dlg)->vbox), frame, TRUE, TRUE, 0);

  if(argc > 0)
  {
    /* table (one row per argv) */
    table = gtk_table_new (argc +1, 3, FALSE);
    gtk_table_set_col_spacings (GTK_TABLE (table), 4);
    gtk_table_set_row_spacings (GTK_TABLE (table), 2);
    gtk_container_set_border_width (GTK_CONTAINER (table), 4);
    gtk_container_add (GTK_CONTAINER (frame), table);

    for(l_idx = 0; l_idx < argc; l_idx++)
    {
       arr_ptr = &argv[l_idx];
       arr_ptr->widget_locked = TRUE;

       if(arr_ptr->label_txt == NULL)  l_label_txt = _("Value:");
       else                            l_label_txt = arr_ptr->label_txt;

       switch(arr_ptr->widget_type)
       {
         case WGT_FLT_PAIR:
            pair_flt_create_value(l_label_txt, GTK_TABLE(table), (l_idx + 1), arr_ptr);
            break;
         case WGT_INT_PAIR:
            pair_int_create_value(l_label_txt, GTK_TABLE(table), (l_idx + 1), arr_ptr);
            break;
         case WGT_TOGGLE:
            toggle_create_value(l_label_txt, GTK_TABLE(table), (l_idx + 1),  arr_ptr);
            break;
         case WGT_RADIO:
            radio_create_value(l_label_txt, GTK_TABLE(table), (l_idx + 1),  arr_ptr);
            break;
         case WGT_OPTIONMENU:
         case WGT_OPT_ENTRY:
            optionmenu_create_value(l_label_txt, GTK_TABLE(table), (l_idx + 1),  arr_ptr);
            break;
         case WGT_FILESEL:
            filesel_create_value(l_label_txt, GTK_TABLE(table), (l_idx + 1),  arr_ptr);
            break;
         case WGT_FONTSEL:
            fontsel_create_value(l_label_txt, GTK_TABLE(table), (l_idx + 1),  arr_ptr);
            break;
         case WGT_TEXT:
            text_create_value(l_label_txt, GTK_TABLE(table), (l_idx + 1),  arr_ptr);
            break;
         case WGT_INT:
            int_create_value(l_label_txt, GTK_TABLE(table), (l_idx + 1),  arr_ptr);
            break;
         case WGT_FLT:
            flt_create_value(l_label_txt, GTK_TABLE(table), (l_idx + 1),  arr_ptr);
            break;
         case WGT_LABEL:
            label_create_value(l_label_txt, GTK_TABLE(table), (l_idx + 1),  arr_ptr, 0.5);
            break;
         case WGT_LABEL_LEFT:
            label_create_value(l_label_txt, GTK_TABLE(table), (l_idx + 1),  arr_ptr, 0.0);
            break;
         case WGT_LABEL_RIGHT:
            label_create_value(l_label_txt, GTK_TABLE(table), (l_idx + 1),  arr_ptr, 1.0);
            break;
         case WGT_ACT_BUTTON:
            printf ("WGT_ACT_BUTTON not implemented yet, widget type ignored\n");
            break;
         case WGT_DEFAULT_BUTTON:
	    {
	      t_all_arr_args arr_all;
	    
	      arr_all.argc = argc;
	      arr_all.argv = argv;
              default_button_create_value(l_label_txt, hbbox, arr_ptr, &arr_all);
	    }
            break;
         default:     /* undefined widget type */
            printf ("Unknown widget type %d ignored\n", arr_ptr->widget_type);
            break;

       }   /* end switch */
       arr_ptr->widget_locked = FALSE;
    }      /* end for */
  }


  /*  Action area  */
  for(l_idx = 0; l_idx < b_argc; l_idx++)
  {

     if(b_argv[l_idx].but_txt == NULL)  
     {
       button = gtk_button_new_from_stock (GTK_STOCK_OK);
     }
     else
     {
       button = gtk_button_new_from_stock (b_argv[l_idx].but_txt);
     }
     GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
     gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
     if( b_argv[l_idx].but_val == b_def_val ) gtk_widget_grab_default (button);
     gtk_widget_show (button);
     g_signal_connect (G_OBJECT (button), "clicked",
		       G_CALLBACK (but_array_callback),
		       &b_argv[l_idx].but_val);
     
  }

  if(b_argc < 1)
  {
     /* if no buttons are specified use one CLOSE button per default */
     button = gtk_button_new_from_stock ( GTK_STOCK_CLOSE);
     GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
     gtk_box_pack_start (GTK_BOX (hbbox), button, TRUE, TRUE, 0);
     gtk_widget_grab_default (button);
     gtk_widget_show (button);
     g_signal_connect (G_OBJECT (button), "clicked",
                       G_CALLBACK (but_array_callback),
                       &l_ok_value);
  }


  gtk_widget_show (frame);
  if(argc > 0)  {  gtk_widget_show (table); }
  gtk_widget_show (g_arrint.dlg);

  gtk_main ();
  gdk_flush ();

  if(gap_debug)
  {
     /* for debugging: print results to stdout */
     for(l_idx = 0; l_idx < argc; l_idx++)
     {
        arr_ptr = &argv[l_idx];

        if(arr_ptr->label_txt == NULL)  l_label_txt = _("Value:");
        else                            l_label_txt = arr_ptr->label_txt;
        arr_ptr = &argv[l_idx];
        
        printf("%02d  ", l_idx);

        switch(arr_ptr->widget_type)
        {
          case WGT_FLT_PAIR:
          case WGT_FLT:
             printf("FLT  %s : %f\n",  l_label_txt, arr_ptr->flt_ret);
             break;
          case WGT_INT_PAIR:
          case WGT_INT:
          case WGT_TOGGLE:
             printf("INT  %s : %d\n",  l_label_txt, arr_ptr->int_ret);
             break;
          case WGT_TEXT:
          case WGT_OPT_ENTRY:
          case WGT_FILESEL:
          case WGT_FONTSEL:
             printf("TEXT  %s : %s\n",  l_label_txt, arr_ptr->text_buf_ret);
             break;
          case WGT_RADIO:
          case WGT_OPTIONMENU:
             printf("RADIO/OPTIONMENU  %s : %d\n",  l_label_txt, arr_ptr->radio_ret);
             break;
          default:
             printf("\n");
             break;

        }

     }
  }
  
  return (g_arrint.run);
}	/* end p_array_std_dialog */



void     p_init_arr_arg  (t_arr_arg *arr_ptr,
                          gint       widget_type)
{
   arr_ptr->label_txt   = NULL;
   arr_ptr->help_txt    = NULL;
   arr_ptr->togg_label  = NULL;
   arr_ptr->entry_width = 60;
   arr_ptr->scale_width = 200;
   arr_ptr->constraint  = TRUE;
   arr_ptr->has_default = FALSE;
   arr_ptr->text_entry  = NULL;
   arr_ptr->text_buf_len     = 0;
   arr_ptr->text_buf_default = NULL;
   arr_ptr->text_buf_ret     = NULL;

   arr_ptr->text_fontsel = NULL;
   arr_ptr->text_filesel = NULL;
   arr_ptr->text_entry = NULL;
   arr_ptr->check_button = NULL;
   arr_ptr->option_menu = NULL;
   arr_ptr->adjustment = NULL;

   switch(widget_type)
   {
     case WGT_LABEL:
     case WGT_LABEL_LEFT:
     case WGT_LABEL_RIGHT:
        arr_ptr->widget_type = widget_type;
        break;
     case WGT_INT_PAIR:
     case WGT_INT:
        arr_ptr->widget_type = widget_type;
        arr_ptr->umin        = (gfloat)G_MININT;
        arr_ptr->umax        = (gfloat)G_MAXINT;
        arr_ptr->int_min     = 0;
        arr_ptr->int_max     = 100;
        arr_ptr->int_step    = 1;
        arr_ptr->pagestep    = 10.0;
        arr_ptr->int_default = 0;
        arr_ptr->int_ret     = 0;
        break;
     case WGT_FLT_PAIR:
     case WGT_FLT:
        arr_ptr->widget_type = widget_type;
        arr_ptr->flt_digits  = 2;
        arr_ptr->umin        = G_MINFLOAT;
        arr_ptr->umax        = G_MAXFLOAT;
        arr_ptr->flt_min     = 0.0;
        arr_ptr->flt_max     = 100.0;
        arr_ptr->flt_step    = 0.1;
        arr_ptr->pagestep    = 10.0;
        arr_ptr->flt_default = 0.0;
        arr_ptr->flt_ret     = 0.0;
        break;
     case WGT_TOGGLE:
        arr_ptr->widget_type = widget_type;
        arr_ptr->int_default = 0;
        arr_ptr->int_ret     = 0;
        break;
     case WGT_RADIO:
     case WGT_OPTIONMENU:
     case WGT_OPT_ENTRY:
        arr_ptr->widget_type = widget_type;
        arr_ptr->radio_argc    = 0;
        arr_ptr->radio_default = 0;
        arr_ptr->radio_ret     = 0;
        arr_ptr->radio_argv    = NULL;
        arr_ptr->radio_help_argv = NULL;
        break;
     case WGT_TEXT:
     case WGT_FONTSEL:
     case WGT_FILESEL:
        arr_ptr->widget_type = widget_type;
        arr_ptr->text_buf_len     = 0;
        arr_ptr->text_buf_default = NULL;
        arr_ptr->text_buf_ret     = NULL;
        arr_ptr->text_fontsel     = NULL;
        arr_ptr->text_filesel     = NULL;
        break;
     case WGT_ACT_BUTTON:
        arr_ptr->widget_type = widget_type;
        arr_ptr->action_functon = NULL;
        arr_ptr->action_data    = NULL;
        break;
     case WGT_DEFAULT_BUTTON:
        arr_ptr->widget_type = widget_type;
        break;
     default:     /* Calling error: undefined widget type */
        arr_ptr->widget_type = WGT_LABEL;
        break;

   }
  
}	/* end p_init_arr_arg */

/* ============================================================================
 *   simplified calls of p_array_std_dialog
 * ============================================================================
 */


gint p_array_dialog(char *title_txt,
                    char *frame_txt,
                    int        argc,
                    t_arr_arg  argv[])
{
    static t_but_arg  b_argv[2];

    b_argv[1].but_txt  = GTK_STOCK_OK;
    b_argv[1].but_val  = TRUE;
    b_argv[0].but_txt  = GTK_STOCK_CANCEL;
    b_argv[0].but_val  = FALSE;
  
    return( p_array_std_dialog(title_txt,
                       frame_txt,
                       argc, argv,      /* widget array */
                       2,    b_argv,    /* button array */
                       FALSE)
           );          /* ret value for window close */
}

/* ============================================================================
 * p_buttons_dialog
 *   dialog window wit 1 upto n buttons
 *   return: the value aassigned with the pressed button.
 *           (If window closed by windowmanager return b_def_val)
 * ============================================================================
 */


gint p_buttons_dialog(char *title_txt,
                         char   *msg_txt,
                         int        b_argc,
                         t_but_arg  b_argv[],
                         gint       b_def_val)
{
  static t_arr_arg  argv[1];
  char   *frame_txt;

  if(b_argc == 1) frame_txt = _("Press Button");
  else            frame_txt = _("Select");
  
  p_init_arr_arg(&argv[0], WGT_LABEL);
  argv[0].label_txt = msg_txt;

  return( p_array_std_dialog(title_txt,
                     frame_txt,
                     1, argv,
                     b_argc, b_argv,
                     b_def_val)
           );          /* ret value for window close */
                       
}	/* end p_buttons_dialog */



/* ============================================================================
 * p_slider_dialog
 *   simplified call of p_array_dialog, using an array with one value.
 *
 *   return  the value of the (only) entryfield 
 *          or  -1 in case of Error or cancel
 * ============================================================================
 */

long p_slider_dialog(char *title, char *frame, char *label, char *tooltip,
                     long min, long max, long curr, long constraint)
{
  static t_arr_arg  argv[1];
  
  p_init_arr_arg(&argv[0], WGT_INT_PAIR);
  argv[0].label_txt = label;
  argv[0].help_txt = tooltip;
  argv[0].constraint = constraint;
  argv[0].entry_width = 45;
  argv[0].scale_width = 130;
  argv[0].int_min    = (gint)min;
  argv[0].int_max    = (gint)max;
  argv[0].int_step   = 1;
  argv[0].int_ret    = (gint)curr;
  
  if(TRUE == p_array_dialog(title, frame, 1, argv))
  {    return (long)(argv[0].int_ret);
  }
  else return -1;
}	/* end p_slider_dialog */


/* ------------------------
 * p_confirm_dialog
 * ------------------------
 */
gboolean
p_confirm_dialog(char *msg_txt, char *title_txt, char *frame_txt)
{
  static t_but_arg  l_but_argv[2];
  static t_arr_arg  l_argv[1];
  gboolean          l_continue;
  gboolean          l_confirm;
  char *value_string;

  l_confirm = TRUE;
  value_string = gimp_gimprc_query("video-confirm-frame-delete");
  if(value_string)
  {

     if(gap_debug) printf("video-confirm-frame-delete: %s\n", value_string);

     /* check if gimprc preferences value no disables confirmation dialog */
     if((*value_string == 'n') || (*value_string == 'N'))
     {
       l_confirm = FALSE;
     }
     g_free(value_string);
  }
  else
  {
     if(gap_debug) printf("NOT found in gimprc: video-confirm-frame-delete:\n (CONFIRM default yes is used)");
  }

  if(!l_confirm)
  {
    /* automatic confirmed OK (dialog is disabled) */
    return TRUE;  
  }

  
  l_but_argv[0].but_txt  = GTK_STOCK_CANCEL;
  l_but_argv[0].but_val  = -1;
  l_but_argv[1].but_txt  = GTK_STOCK_OK;
  l_but_argv[1].but_val  = 0;


  p_init_arr_arg(&l_argv[0], WGT_LABEL);
  l_argv[0].label_txt = msg_txt;

  l_continue = p_array_std_dialog (title_txt
                                  ,frame_txt
                                  ,G_N_ELEMENTS(l_argv),     l_argv
                                  ,G_N_ELEMENTS(l_but_argv), l_but_argv
                                  ,-1  /* default value is Cancel */
                                  );
      
  if(l_continue == 0)
  {
    return TRUE;  /* OK pressed */
  }
  
  return FALSE;

}  /* end p_confirm_dialog */
