/* gap_mov_dialog.c
 *   by hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * Dialog Window for Move Path (gap_mov)
 *
 */

/* code was mainly inspired by SuperNova plug-in
 * Copyright (C) 1997 Eiichi Takamori <taka@ma1.seikyou.ne.jp>,
 *		      Spencer Kimball, Federico Mena Quintero
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
 * gimp    1.3.20c; 2003/09/29  hof: new features:  instant_apply, perspective transformation,
 *                                   - tween_layer and trace_layer
 *                                   - changed opacity, rotation and resize from int to gdouble
 * gimp    1.3.17b; 2003/07/31  hof: message text fixes for translators (# 118392)
 * gimp    1.3.16c; 2003/07/12  hof: removed deprecated GtkKPreview widget (replaced by drawing_area based gap_pview_da calls)
 *                                   cursor crosslines ar now switchable (show_cursor flag)
 * gimp    1.3.15a; 2003/06/21  hof: attempt to remove some deprecated calls (no success)
 * gimp    1.3.14b; 2003/06/03  hof: added gap_stock_init
 *                                   replaced mov_gtk_button_new_with_pixmap  by  gtk_button_new_from_stock
 * gimp    1.3.14a; 2003/05/24  hof: moved render procedures to module gap_mov_render
 *                                   placed OK button right.
 *                                   added pixmaps (thanks to Jakub Steiner for providing the pixmaps)
 *                              sven:  replaced _gimp_help_init by gimp_ui_init
 * gimp    1.3.12a; 2003/05/03  hof: merge into CVS-gimp-gap project, replace gimp_help_init by _gimp_help_init
 * gimp    1.3.4b;  2002/03/15  hof: temp. reverted setting of preview widget size.
 * gimp    1.3.4;   2002/03/12  hof: ported to gtk+-2.0.0
 *                                   still needs GTK_DISABLE_DEPRECATED (port is not complete)
 * gimp    1.1.29b; 2000/11/30  hof: new feature: FRAME based Stepmodes, changes for NONINTERACTIVE mode
 * gimp    1.1.23a; 2000/06/04  hof: new button: rotation follow path
 * gimp    1.1.20a; 2000/04/25  hof: support for keyframes, anim_preview (suggested by jakub steiner)
 * gimp    1.1.17b; 2000/02/23  hof: bugfix: dont flatten the preview, just merge visible layers
 *                                   bugfix: for current frame never use diskfile for the preview
 *                                           (to avoid inconsitencies, and to speed up a little)
 *                                   added "Show Path", pick and drag Controlpoints
 * gimp    1.1.17a; 2000/02/20  hof: use gimp_help_set_help_data for tooltips
 *                                   added spinbuttons, and more layout cosmetics.
 * gimp    1.1.15a; 2000/01/26  hof: removed gimp 1.0.x support
 * gimp    1.1.13b; 1999/12/04  hof: some cosmetic gtk fixes
 *                                   changed border_width spacing and Buttons in action area
 *                                   to same style as used in dialogs of the gimp 1.1.13 main dialogs
 * gimp   1.1.8a;   1999/08/31  hof: accept anim framenames without underscore '_'
 * gimp   1.1.5a;   1999/05/08  hof: call fileselect in gtk+1.2 style 
 * version 0.99.00; 1999.03.03  hof: bugfix: update of the preview (did'nt work with gimp1.1.2)
 * version 0.98.00; 1998.11.28  hof: Port to GIMP 1.1: replaced buildmenu.h, apply layermask (before rotate)
 *                                   mov_imglayer_constrain must check for drawable_id -1
 * version 0.97.00; 1998.10.19  hof: Set window title to "Move Path"
 * version 0.96.02; 1998.07.30  hof: added clip to frame option and tooltips
 * version 0.96.00; 1998.07.09  hof: bugfix (filesel did not reopen after cancel)
 * version 0.95.00; 1998.05.12  hof: added rotatation capabilities
 * version 0.94.00; 1998.04.25  hof: use only one point as default
 *                                   bugfix: initial value for src_paintmode
 *                                           fixes the problem reported in p_my_layer_copy (can't get new layer)
 * version 0.90.00; 1997.12.14  hof: 1.st (pre) release
 */


#include "config.h"

/* #define MOVE_PATH_LAYOUT_BIG_PREVIEW */
/* if defined MOVE_PATH_LAYOUT_BIG_PREVIEW use layout with big preview 
 * (maybe enable this on fast machines via configure option later)
 */

/*#define MOVE_PATH_FRAMES_ENABLE */
/* if defined MOVE_PATH_FRAMES_ENABLE do waste some layout space 
 * and add frames in the notebook widgets
 * (maybe ill remove that stuff before stable release)
 */

/* SYTEM (UNIX) includes */ 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <time.h>

/* GIMP includes */
#include "gtk/gtk.h"
#include "gap-intl.h"
#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

/* GAP includes */
#include "gap_layer_copy.h"
#include "gap_lib.h"
#include "gap_mov_exec.h"
#include "gap_mov_dialog.h"
#include "gap_mov_render.h"
#include "gap_pdb_calls.h"
#include "gap_vin.h"
#include "gap_arr_dialog.h"

#include "gap_pview_da.h"
#include "gap_stock.h"


extern      int gap_debug; /* ==0  ... dont print debug infos */

/* Some useful defines and  macros */
#define ENTRY_WIDTH 60
#define SPINBUTTON_WIDTH 60
#define SCALE_WIDTH 125

/* instant apply is implemented via timer, configured to fire 10 times per second (100 msec)
 * this collects instant_apply_requests set by other widget callbacks and events
 * and then update only once.
 * The timer is completely removed, when instant_apply is OFF
 * instant_apply requires much CPU and IO power especially on big images
 * and images with many layers
 */
#define INSTANT_TIMERINTERVAL_MILLISEC  100

#ifdef MOVE_PATH_LAYOUT_BIG_PREVIEW
#define PREVIEW_SIZE 384
#else
#define PREVIEW_SIZE 256
#endif

#define RADIUS           3

#define PREVIEW	      0x1
#define CURSOR        0x2
#define PATH_LINE     0x4
#define ALL	      0xf

#define GAP_MOV_CHECK_SIZE 8

/*  event masks for the preview widget */
#define PREVIEW_MASK   GDK_EXPOSURE_MASK | \
		       GDK_BUTTON_PRESS_MASK |\
		       GDK_BUTTON_RELEASE_MASK |\
		       GDK_BUTTON_MOTION_MASK


#define POINT_INDEX_LABEL_LENGTH 256

typedef struct {
  gint run;
} t_mov_interface;

typedef struct
{
  GimpDrawable	*drawable;
  gint		dwidth, dheight;
  gint		bpp;
  t_pview       *pv_ptr;
  GimpPixelRgn	 src_rgn;
  gint           show_path;
  gint           show_cursor;
  gint           show_grid;
  gint           instant_apply;
  gboolean       instant_apply_request;
  gint32         instant_timertag;
  gint           startup;

  gint		pwidth, pheight;
  gint		curx, cury;		 /* x,y of cursor in preview */

  GtkWidget     *filesel;
  GtkAdjustment *x_adj;
  GtkAdjustment *y_adj;
  GtkAdjustment *wres_adj;
  GtkAdjustment *hres_adj;
  GtkAdjustment *opacity_adj;
  GtkAdjustment *rotation_adj;
  GtkAdjustment *keyframe_adj;

  GtkWidget     *constrain;       /* scale width/height keeps ratio constant */
  GtkAdjustment *ttlx_adj;
  GtkAdjustment *ttly_adj;
  GtkAdjustment *ttrx_adj;
  GtkAdjustment *ttry_adj;
  GtkAdjustment *tblx_adj;
  GtkAdjustment *tbly_adj;
  GtkAdjustment *tbrx_adj;
  GtkAdjustment *tbry_adj;
  GtkAdjustment *step_speed_factor_adj;
  GtkAdjustment *tween_opacity_initail_adj;
  GtkAdjustment *tween_opacity_desc_adj;
  GtkAdjustment *trace_opacity_initial_adj;
  GtkAdjustment *trace_opacity_desc_adj;
  GtkAdjustment *tween_steps_adj;

  gchar          point_index_text[POINT_INDEX_LABEL_LENGTH];
  GtkWidget     *point_index_frame;
  gint           p_x, p_y;
  gdouble        opacity;
  gdouble        w_resize;
  gdouble        h_resize;
  gdouble        rotation;
  gdouble        ttlx;     /* 0.0 upto 10.0 transform x top left */
  gdouble        ttly;     /* 0.0 upto 10.0 transform y top left */
  gdouble        ttrx;     /* 0.0 upto 10.0 transform x top right */
  gdouble        ttry;     /* 0.0 upto 10.0 transform y top right */
  gdouble        tblx;     /* 0.0 upto 10.0 transform x bot left */
  gdouble        tbly;     /* 0.0 upto 10.0 transform y bot left */
  gdouble        tbrx;     /* 0.0 upto 10.0 transform x bot right */
  gdouble        tbry;     /* 0.0 upto 10.0 transform y bot right */

  gint           keyframe_abs;
  gint           max_frame;


  gint           preview_frame_nr;      /* default: current frame */
  gint           old_preview_frame_nr;
  t_anim_info   *ainfo_ptr;
 	
  gint          in_call;
  char         *pointfile_name;

  gint                first_nr;
  gint                last_nr;
  GtkWidget          *shell;
  GtkWidget          *master_vbox;
} t_mov_gui_stuff;


/* p_buildmenu Structures */

typedef struct _MenuItem   MenuItem;

typedef void (*MenuItemCallback) (GtkWidget *widget,
				  gpointer   user_data);
struct _MenuItem
{
  char *label;
  char  unused_accelerator_key;
  gint  unused_accelerator_mods;
  MenuItemCallback callback;
  gpointer user_data;
  MenuItem *unused_subitems;
  GtkWidget *widget;
};


/* Declare a local function.
 */
GtkWidget       *  p_buildmenu (MenuItem *, t_mov_gui_stuff *mgp);

       long        p_move_dialog             (t_mov_data *mov_ptr);
static void        p_update_point_index_text (t_mov_gui_stuff *mgp);
static void        p_points_from_tab         (t_mov_gui_stuff *mgp);
static void        p_points_to_tab           (t_mov_gui_stuff *mgp);
static void        p_point_refresh           (t_mov_gui_stuff *mgp);
static void        p_pick_nearest_point      (gint px, gint py);
static void        p_reset_points            ();
static void        p_clear_one_point         (gint idx);
static void        p_load_points             (char *filename);
static void        p_save_points             (char *filename);

static GimpDrawable * p_get_flattened_drawable (gint32 image_id);
static GimpDrawable * p_get_prevw_drawable (t_mov_gui_stuff *mgp);

static gint	   mov_dialog ( GimpDrawable *drawable, t_mov_gui_stuff *mgp,
                                gint min, gint max);
static GtkWidget * mov_modify_tab_create (t_mov_gui_stuff *mgp);
static GtkWidget * mov_trans_tab_create  (t_mov_gui_stuff *mgp);
static void        mov_path_prevw_create ( GimpDrawable *drawable,
                                           t_mov_gui_stuff *mgp,
					   gboolean vertical_layout);
static GtkWidget * mov_src_sel_create (t_mov_gui_stuff *mgp);
static GtkWidget * mov_advanced_tab_create(t_mov_gui_stuff *mgp);
static GtkWidget * mov_edit_button_box_create (t_mov_gui_stuff *mgp);
static GtkWidget * mov_path_framerange_box_create(t_mov_gui_stuff *mgp,
						  gboolean vertical_layout
						  );
static void	   mov_path_prevw_preview_init ( t_mov_gui_stuff *mgp );
static void	   mov_path_prevw_draw ( t_mov_gui_stuff *mgp, gint update );
static void	   mov_instant_int_adjustment_update (GtkObject *obj, gpointer val);
static void	   mov_instant_double_adjustment_update (GtkObject *obj, gpointer val);
static void	   mov_path_x_adjustment_update ( GtkWidget *widget, gpointer data );
static void	   mov_path_y_adjustment_update ( GtkWidget *widget, gpointer data );
static void        mov_path_tfactor_adjustment_update( GtkWidget *widget, gdouble *val);
static void        mov_path_resize_adjustment_update( GtkObject *obj, gdouble *val);
static void        mov_path_tween_steps_adjustment_update( GtkObject *obj, gint *val);
static void	   mov_path_prevw_cursor_update ( t_mov_gui_stuff *mgp );
static gint	   mov_path_prevw_preview_expose ( GtkWidget *widget, GdkEvent *event );
static gint	   mov_path_prevw_preview_events ( GtkWidget *widget, GdkEvent *event );
static gint        p_chk_keyframes(t_mov_gui_stuff *mgp);

static void	mov_padd_callback        (GtkWidget *widget,gpointer data);
static void	mov_pins_callback        (GtkWidget *widget,gpointer data);
static void	mov_pdel_callback        (GtkWidget *widget,gpointer data);
static void     mov_pnext_callback       (GtkWidget *widget,gpointer data);
static void     mov_pprev_callback       (GtkWidget *widget,gpointer data);
static void     mov_pfirst_callback      (GtkWidget *widget,gpointer data);
static void     mov_plast_callback       (GtkWidget *widget,gpointer data);
static void	mov_pdel_all_callback    (GtkWidget *widget,gpointer data);
static void	mov_pclr_callback        (GtkWidget *widget,gpointer data);
static void	mov_pclr_all_callback    (GtkWidget *widget,gpointer data);
static void	mov_prot_follow_callback (GtkWidget *widget,GdkEventButton *bevent,gpointer data);
static void     mov_pload_callback       (GtkWidget *widget,gpointer data);
static void     mov_psave_callback       (GtkWidget *widget,gpointer data);
static void     p_points_load_from_file  (GtkWidget *widget,t_mov_gui_stuff *mgp);
static void     p_points_save_to_file    (GtkWidget *widget,t_mov_gui_stuff *mgp);

static void	mov_close_callback	 (GtkWidget *widget,gpointer data);
static void	mov_ok_callback	         (GtkWidget *widget,gpointer data);
static void     mov_upvw_callback        (GtkWidget *widget, t_mov_gui_stuff *mgp);
static void     mov_apv_callback         (GtkWidget *widget,gpointer data);
static void	p_filesel_close_cb       (GtkWidget *widget, t_mov_gui_stuff *mgp);

static gint mov_imglayer_constrain      (gint32 image_id, gint32 drawable_id, gpointer data);
static void mov_imglayer_menu_callback  (gint32 id, gpointer data);
static void mov_paintmode_menu_callback (GtkWidget *, gpointer);
static void mov_handmode_menu_callback  (GtkWidget *, gpointer);
static void mov_stepmode_menu_callback  (GtkWidget *, gpointer);
static void mov_tweenlayer_sensitivity(t_mov_gui_stuff *mgp);
static void mov_tracelayer_sensitivity(t_mov_gui_stuff *mgp);
static void mov_gint_toggle_callback    (GtkWidget *, gpointer);
static void mov_force_visibility_toggle_callback ( GtkWidget *widget, gpointer data );
static void mov_tracelayer_callback     (GtkWidget *, gpointer);
static void mov_show_path_or_cursor     (t_mov_gui_stuff *mgp);
static void mov_show_path_callback      (GtkWidget *, t_mov_gui_stuff *mgp);
static void mov_show_cursor_callback    (GtkWidget *, t_mov_gui_stuff *mgp);
static void mov_show_grid_callback      (GtkWidget *, t_mov_gui_stuff *mgp);
static void mov_install_timer           (t_mov_gui_stuff *mgp);
static void mov_remove_timer            (t_mov_gui_stuff *mgp);
static void mov_instant_timer_callback (gpointer   user_data);
static void mov_instant_apply_callback  (GtkWidget *, t_mov_gui_stuff *mgp);
static void mov_set_instant_apply_request(t_mov_gui_stuff *mgp);

GtkObject *
p_mov_spinbutton_new(GtkTable *table
                    ,gint      col
                    ,gint      row
                    ,gchar    *label_text
                    ,gint      scale_width      /* dummy, not used */
                    ,gint      spinbutton_width
                    ,gdouble   initial_val
                    ,gdouble   lower            /* dummy, not used */
                    ,gdouble   upper            /* dummy, not used */
                    ,gdouble   sstep
                    ,gdouble   pagestep
                    ,gint      digits
                    ,gboolean  constrain
                    ,gdouble   umin
                    ,gdouble   umax
                    ,gchar    *tooltip_text
                    ,gchar    *privatetip
                    );


/*  the option menu items -- the paint modes  */
static MenuItem option_paint_items[] =
{
  { N_("Normal"), 0, 0, mov_paintmode_menu_callback, (gpointer) GIMP_NORMAL_MODE, NULL, NULL },
  { N_("Dissolve"), 0, 0, mov_paintmode_menu_callback, (gpointer) GIMP_DISSOLVE_MODE, NULL, NULL },
  { N_("Multiply"), 0, 0, mov_paintmode_menu_callback, (gpointer) GIMP_MULTIPLY_MODE, NULL, NULL },
  { N_("Screen"), 0, 0, mov_paintmode_menu_callback, (gpointer) GIMP_SCREEN_MODE, NULL, NULL },
  { N_("Overlay"), 0, 0, mov_paintmode_menu_callback, (gpointer) GIMP_OVERLAY_MODE, NULL, NULL },
  { N_("Difference"), 0, 0, mov_paintmode_menu_callback, (gpointer) GIMP_DIFFERENCE_MODE, NULL, NULL },
  { N_("Addition"), 0, 0, mov_paintmode_menu_callback, (gpointer) GIMP_ADDITION_MODE, NULL, NULL },
  { N_("Subtract"), 0, 0, mov_paintmode_menu_callback, (gpointer) GIMP_SUBTRACT_MODE, NULL, NULL },
  { N_("Darken Only"), 0, 0, mov_paintmode_menu_callback, (gpointer) GIMP_DARKEN_ONLY_MODE, NULL, NULL },
  { N_("Lighten Only"), 0, 0, mov_paintmode_menu_callback, (gpointer) GIMP_LIGHTEN_ONLY_MODE, NULL, NULL },
  { N_("Hue"), 0, 0, mov_paintmode_menu_callback, (gpointer) GIMP_HUE_MODE, NULL, NULL },
  { N_("Saturation"), 0, 0, mov_paintmode_menu_callback, (gpointer) GIMP_SATURATION_MODE, NULL, NULL },
  { N_("Color"), 0, 0, mov_paintmode_menu_callback, (gpointer) GIMP_COLOR_MODE, NULL, NULL },
  { N_("Value"), 0, 0, mov_paintmode_menu_callback, (gpointer) GIMP_VALUE_MODE, NULL, NULL },
  { NULL, 0, 0, NULL, NULL, NULL, NULL }
};

/*  the option menu items -- the handle modes  */
static MenuItem option_handle_items[] =
{
  { N_("Left  Top"),    0, 0, mov_handmode_menu_callback, (gpointer) GAP_HANDLE_LEFT_TOP, NULL, NULL },
  { N_("Left  Bottom"), 0, 0, mov_handmode_menu_callback, (gpointer) GAP_HANDLE_LEFT_BOT, NULL, NULL },
  { N_("Right Top"),    0, 0, mov_handmode_menu_callback, (gpointer) GAP_HANDLE_RIGHT_TOP, NULL, NULL },
  { N_("Right Bottom"), 0, 0, mov_handmode_menu_callback, (gpointer) GAP_HANDLE_RIGHT_BOT, NULL, NULL },
  { N_("Center"),       0, 0, mov_handmode_menu_callback, (gpointer) GAP_HANDLE_CENTER, NULL, NULL },
  { NULL, 0, 0, NULL, NULL, NULL, NULL }
};


/*  the option menu items -- the loop step modes  */
static MenuItem option_step_items[] =
{
  { N_("Loop"),         0, 0, mov_stepmode_menu_callback, (gpointer) GAP_STEP_LOOP, NULL, NULL },
  { N_("Loop Reverse"), 0, 0, mov_stepmode_menu_callback, (gpointer) GAP_STEP_LOOP_REV, NULL, NULL },
  { N_("Once"),         0, 0, mov_stepmode_menu_callback, (gpointer) GAP_STEP_ONCE, NULL, NULL },
  { N_("OnceReverse"),  0, 0, mov_stepmode_menu_callback, (gpointer) GAP_STEP_ONCE_REV, NULL, NULL },
  { N_("PingPong"),     0, 0, mov_stepmode_menu_callback, (gpointer) GAP_STEP_PING_PONG, NULL, NULL },
  { N_("None"),         0, 0, mov_stepmode_menu_callback, (gpointer) GAP_STEP_NONE, NULL, NULL },
  { N_("Frame Loop"),         0, 0, mov_stepmode_menu_callback, (gpointer) GAP_STEP_FRAME_LOOP, NULL, NULL },
  { N_("Frame Loop Reverse"), 0, 0, mov_stepmode_menu_callback, (gpointer) GAP_STEP_FRAME_LOOP_REV, NULL, NULL },
  { N_("Frame Once"),         0, 0, mov_stepmode_menu_callback, (gpointer) GAP_STEP_FRAME_ONCE, NULL, NULL },
  { N_("Frame OnceReverse"),  0, 0, mov_stepmode_menu_callback, (gpointer) GAP_STEP_FRAME_ONCE_REV, NULL, NULL },
  { N_("Frame PingPong"),     0, 0, mov_stepmode_menu_callback, (gpointer) GAP_STEP_FRAME_PING_PONG, NULL, NULL },
  { N_("Frame None"),         0, 0, mov_stepmode_menu_callback, (gpointer) GAP_STEP_FRAME_NONE, NULL, NULL },
  { NULL, 0, 0, NULL, NULL, NULL, NULL }
};



static t_mov_values *pvals;

static t_mov_interface mov_int =
{
  FALSE	    /* run */
};


/* ============================================================================
 **********************
 *                    *
 *  Dialog interface  *
 *                    *
 **********************
 * ============================================================================
 */

long      p_move_dialog    (t_mov_data *mov_ptr)
{
  GimpDrawable *l_drawable_ptr;
  gint       l_first, l_last;  
  char      *l_str;
  t_mov_gui_stuff *mgp;


  if(gap_debug) printf("GAP-DEBUG: START p_move_dialog\n");

  mgp = g_new( t_mov_gui_stuff, 1 );
  if(mgp == NULL)
  {
    printf("error can't alloc path_preview structure\n");
    return -1;
  }
  mgp->show_path = TRUE;
  mgp->show_cursor = TRUE;
  mgp->show_grid = FALSE;
  mgp->instant_apply = FALSE;
  mgp->instant_apply_request = FALSE;
  mgp->instant_timertag = -1;
  mgp->startup = TRUE;
  mgp->keyframe_adj = NULL;
  mgp->pv_ptr = NULL;
  
  pvals = mov_ptr->val_ptr;

  l_str = p_strdup_del_underscore(mov_ptr->dst_ainfo_ptr->basename);
  mgp->pointfile_name  = g_strdup_printf("%s.path_points", l_str);
  g_free(l_str);

  
  l_first = mov_ptr->dst_ainfo_ptr->first_frame_nr;
  l_last  = mov_ptr->dst_ainfo_ptr->last_frame_nr;

  /* init parameter values */ 
  pvals->dst_image_id = mov_ptr->dst_ainfo_ptr->image_id;
  pvals->tmp_image_id = -1;
  pvals->src_image_id = -1;
  pvals->src_layer_id = -1;
  pvals->src_paintmode = GIMP_NORMAL_MODE;
  pvals->src_handle = GAP_HANDLE_LEFT_TOP;
  pvals->src_stepmode = GAP_STEP_LOOP;
  pvals->src_force_visible  = 1;
  pvals->clip_to_img  = 0;

  pvals->step_speed_factor = 1.0;
  pvals->tracelayer_enable = FALSE;
  pvals->trace_opacity_initial = 100.0;
  pvals->trace_opacity_desc = 80.0;
  pvals->tween_steps = 0;
  pvals->tween_opacity_initail = 80.0;
  pvals->tween_opacity_desc = 80.0;

  pvals->apv_mode  = GAP_APV_QUICK;
  pvals->apv_src_frame  = -1;
  pvals->apv_mlayer_image  = -1;
  pvals->apv_gap_paste_buff  = NULL;
  pvals->apv_scalex  = 40.0;
  pvals->apv_scaley  = 40.0;

  pvals->cache_src_image_id  = -1;  
  pvals->cache_tmp_image_id  = -1;
  pvals->cache_tmp_layer_id  = -1;
  pvals->cache_frame_number  = -1;
  pvals->cache_ainfo_ptr = NULL;

  p_reset_points();
  
  /* pvals->point[1].p_x = 100; */  /* default: move from 0/0 to 100/0 */

  pvals->dst_range_start = mov_ptr->dst_ainfo_ptr->curr_frame_nr;
  pvals->dst_range_end   = l_last;
  pvals->dst_layerstack = 0;   /* 0 ... insert layer on top of stack */

  mgp->filesel = NULL;   /* fileselector is not open */
  mgp->ainfo_ptr            = mov_ptr->dst_ainfo_ptr;
  mgp->preview_frame_nr     = mov_ptr->dst_ainfo_ptr->curr_frame_nr;
  mgp->old_preview_frame_nr = mgp->preview_frame_nr;
  mgp->point_index_frame = NULL;
  
  p_points_from_tab(mgp);
  p_update_point_index_text(mgp);

  /* duplicate the curerent image (for flatten & preview)  */
  if(gap_debug) printf("GAP-DEBUG: p_move_dialog before gimp_image_duplicate\n");
  pvals->tmp_image_id = gimp_image_duplicate(pvals->dst_image_id);
  if(gap_debug) printf("GAP-DEBUG: p_move_dialog before gimp_image_duplicate\n");

  /* flatten image, and get the (only) resulting drawable */
  l_drawable_ptr = p_get_prevw_drawable(mgp);

  /* do DIALOG window */
  mov_dialog(l_drawable_ptr, mgp, l_first, l_last);
  p_points_to_tab(mgp);

  /* destroy the tmp image */
  gimp_image_delete(pvals->tmp_image_id);

  /* remove timer if there is one */
  mov_remove_timer(mgp);

  g_free(mgp);


  if(gap_debug) printf("GAP-DEBUG: END p_move_dialog\n");

  if(mov_int.run == TRUE)  return 0;
  else                     return  -1;
}


/* ============================================================================
 *******************
 *                 *
 *   Main Dialog   *
 *                 *
 *******************
 * ============================================================================
 */

static gint
mov_dialog ( GimpDrawable *drawable, t_mov_gui_stuff *mgp,
             gint first_nr, gint last_nr )
{
  GtkWidget *notebook;
  GtkWidget *vbox;
  GtkWidget *hbbox;
  GtkWidget *dlg;
  GtkWidget *frame;
  GtkWidget *button;
  GtkWidget *label;
  GtkWidget *src_sel_frame;
  GtkWidget *advanced_frame;
  GtkWidget *framerange_table;
  gboolean  vertical_layout;

  if(gap_debug) printf("GAP-DEBUG: START mov_dialog\n");

  gimp_ui_init ("gap_move", FALSE);
  gap_stock_init();

#ifdef MOVE_PATH_LAYOUT_BIG_PREVIEW
  vertical_layout = TRUE;
#else
  vertical_layout = FALSE;
#endif
  /* dialog */
  dlg = gtk_dialog_new ();
  mgp->shell = dlg;
  mgp->first_nr = first_nr;
  mgp->last_nr = last_nr;
  
  gtk_window_set_title (GTK_WINDOW (dlg), _("Move Path"));
  gtk_window_set_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  g_signal_connect (G_OBJECT (dlg), "destroy",
		    G_CALLBACK (mov_close_callback),
		    NULL);

  /*  Action area  */
  gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (dlg)->action_area), 2);
  gtk_box_set_homogeneous (GTK_BOX (GTK_DIALOG (dlg)->action_area), FALSE);

  hbbox = gtk_hbutton_box_new ();
  gtk_box_set_spacing (GTK_BOX (hbbox), 2);
  gtk_box_pack_end (GTK_BOX (GTK_DIALOG (dlg)->action_area), hbbox, FALSE, FALSE, 0);
  gtk_widget_show (hbbox);
      

  button = gtk_button_new_from_stock ( GTK_STOCK_CANCEL);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (hbbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);
  g_signal_connect_swapped (G_OBJECT (button), "clicked",
			   G_CALLBACK (gtk_widget_destroy),
			   dlg);

  /* button = gtk_button_new_with_label ( _("Update Preview")); */
  button = gtk_button_new_from_stock ( GTK_STOCK_REFRESH );
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (hbbox), button, TRUE, TRUE, 0);
  gimp_help_set_help_data(button,
                       _("Show PreviewFrame with Selected SrcLayer at current Controlpoint")
                       , NULL);
  gtk_widget_show (button);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK  (mov_upvw_callback),
		    mgp);

  button = gtk_button_new_from_stock ( GAP_STOCK_ANIM_PREVIEW );
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (hbbox), button, TRUE, TRUE, 0);
  gimp_help_set_help_data(button,
                       _("Generate Animated Preview as multilayer image")
                       , NULL);
  gtk_widget_show (button);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (mov_apv_callback),
		    mgp);


  button = gtk_button_new_from_stock ( GTK_STOCK_OK);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (hbbox), button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK  (mov_ok_callback),
		    mgp);

  /*  parameter settings  */
  frame = gtk_frame_new ( _("Copy moving source-layer(s) into frames"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 4);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);


  /* the vbox */
  vbox = gtk_vbox_new (FALSE, 3);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  /* the notebook */
  notebook = gtk_notebook_new();
  {
    GtkWidget *hbox;

    hbox = gtk_hbox_new (FALSE, 3);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (hbox), notebook, TRUE, TRUE, 4);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
  }
  
  /* the source select frame */  
  src_sel_frame = mov_src_sel_create (mgp);
  gtk_container_add (GTK_CONTAINER (notebook), src_sel_frame);
  label = gtk_label_new(_("Source Select"));
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook)
                             , gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), 0)
			     , label
			     );
  /* the advanced features frame */  
  advanced_frame = mov_advanced_tab_create(mgp);
  gtk_container_add (GTK_CONTAINER (notebook), advanced_frame);
  label = gtk_label_new(_("Advanced Settings"));
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook)
                             , gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), 1)
			     , label
			     );
  gtk_widget_show (notebook);
  
  /* the path preview frame (with all the controlpoint widgets) */
  mgp->max_frame = MAX(first_nr, last_nr);
  mgp->master_vbox = vbox;
  mov_path_prevw_create ( drawable, mgp, vertical_layout);

  if(!vertical_layout)
  {
    /* the box with the framerange selection widgets
     * (if we have vertical_layout using the BIG Preview
     *  the framerange_table was already done in the mov_path_prevw_create procedure,
     *  otherwise we have to create it now)
     */
    framerange_table = mov_path_framerange_box_create(mgp, vertical_layout);
    gtk_box_pack_start (GTK_BOX (vbox), framerange_table, TRUE, TRUE, 0);
  }

  gtk_widget_show (dlg);
  gtk_widget_realize(mgp->shell);

  mgp->startup = FALSE;

  p_pview_set_size(mgp->pv_ptr
                  , mgp->pwidth
                  , mgp->pheight
                  , GAP_MOV_CHECK_SIZE
                  );
  mov_path_prevw_preview_init(mgp);
  mov_show_path_or_cursor(mgp);
  
  gtk_main ();
  gdk_flush ();

  if(gap_debug) printf("GAP-DEBUG: END mov_dialog\n");

  return mov_int.run;
}  /* end mov_dialog */


/* ============================================================================
 * implementation of CALLBACK procedures
 * ============================================================================
 */

static void
mov_close_callback (GtkWidget *widget,
			 gpointer   data)
{
  gtk_main_quit ();
}

static void
mov_ok_callback (GtkWidget *widget,
		      gpointer	 data)
{
  t_mov_gui_stuff   *mgp;

  mgp = data;

  if(pvals != NULL)
  {
    if(pvals->src_layer_id < 0)
    {

       p_msg_win(GIMP_RUN_INTERACTIVE,
                 _("No Source Image was selected\n"
		   "(Please open a 2nd Image of the same type before opening Move Path)"));
       return;
    }
  }

  if(!p_chk_keyframes(mgp))
  {
    return;
  }

  mov_int.run = TRUE;
 
  if(pvals->point_idx_max == 0)
  {
    /* if we have only one point duplicate that point
     * (move algorithm needs at least 2 points)
     */
    mov_padd_callback(NULL, mgp);
  }
  gtk_widget_destroy (mgp->shell);
}  /* end mov_ok_callback */

static void
mov_upvw_callback (GtkWidget *widget,
		  t_mov_gui_stuff *mgp)
{
  char               *l_filename;
  long                l_frame_nr;
  gint32              l_new_tmp_image_id;
  gint32              l_old_tmp_image_id;

  if(gap_debug) printf("mov_upvw_callback nr: %d old_nr: %d\n",
         (int)mgp->preview_frame_nr , (int)mgp->old_preview_frame_nr);

  l_frame_nr = (long)mgp->preview_frame_nr;
  l_filename = p_alloc_fname(mgp->ainfo_ptr->basename,
                             l_frame_nr,
                             mgp->ainfo_ptr->extension);
  if(l_filename != NULL)
  {
     /* replace the temporary image */
     if(mgp->preview_frame_nr  == mgp->ainfo_ptr->curr_frame_nr)
     {
       l_new_tmp_image_id = gimp_image_duplicate(mgp->ainfo_ptr->image_id);
     }
     else
     {
	l_new_tmp_image_id = p_load_image(l_filename);
     }
     g_free(l_filename);
     if (l_new_tmp_image_id >= 0)
     {
	/* use the new loaded temporary image */
	l_old_tmp_image_id  = pvals->tmp_image_id;
	pvals->tmp_image_id = l_new_tmp_image_id;

	/* flatten image, and get the (only) resulting drawable */
	mgp->drawable = p_get_prevw_drawable(mgp);

	/* gimp_display_new(pvals->tmp_image_id); */ /* add a display for debugging only */

	/* re initialize preview image */
	mov_path_prevw_preview_init(mgp);
	p_point_refresh(mgp);

	mgp->old_preview_frame_nr = mgp->preview_frame_nr;

	gtk_widget_queue_draw(mgp->pv_ptr->da_widget);
	mov_path_prevw_draw ( mgp, CURSOR | PATH_LINE );
	gdk_flush();  

	/* destroy the old tmp image */
	gimp_image_delete(l_old_tmp_image_id);
	
	mgp->instant_apply_request = FALSE;
     }

  }
}  /* end mov_upvw_callback */


static void
mov_apv_callback (GtkWidget *widget,
		      gpointer	 data)
{
#define ARGC_APV 4
  t_mov_gui_stuff *mgp;
  t_video_info       *vin_ptr;
  static gint         apv_locked = FALSE;
  gint32              l_new_image_id;
  GimpParam             *return_vals;
  int                 nreturn_vals;

  static t_arr_arg  argv[ARGC_APV];
  static char *radio_apv_mode[3] = { N_("Object on empty frames")
                                   , N_("Object on one frame")
				   , N_("Exact Object on frames")
				   };
  static int gettextize_loop = 0;



  mgp = data;


  if(!p_chk_keyframes(mgp))
  {
    return;
  }

  
  if(apv_locked)
  {
    return;
  }

  apv_locked = TRUE;

  if(gap_debug) printf("mov_apv_callback preview_frame_nr: %d\n",
         (int)mgp->preview_frame_nr);


  for (;gettextize_loop < 3; gettextize_loop++)
    radio_apv_mode[gettextize_loop] = gettext(radio_apv_mode[gettextize_loop]);
  
  p_init_arr_arg(&argv[0], WGT_RADIO);
  argv[0].label_txt = _("Anim Preview Mode");
  argv[0].help_txt  = NULL;
  argv[0].radio_argc  = 3;
  argv[0].radio_argv = radio_apv_mode;
  argv[0].radio_ret  = 0;

  p_init_arr_arg(&argv[1], WGT_FLT_PAIR);
  argv[1].constraint = TRUE;
  argv[1].label_txt = _("Scale Preview");
  argv[1].help_txt  = _("Scale down size of the generated animated preview (in %)");
  argv[1].flt_min   = 5.0;
  argv[1].flt_max   = 100.0;
  argv[1].flt_step  = 1.0;
  argv[1].flt_ret   = pvals->apv_scalex;

  p_init_arr_arg(&argv[2], WGT_FLT_PAIR);
  argv[2].constraint = TRUE;
  argv[2].label_txt = _("Framerate");
  argv[2].help_txt  = _("Framerate to use in the animated preview in frames/sec");
  argv[2].flt_min   = 1.0;
  argv[2].flt_max   = 100.0;
  argv[2].flt_step  = 1.0;
  argv[2].flt_ret   = 24;

  vin_ptr = p_get_video_info(mgp->ainfo_ptr->basename);
  if(vin_ptr)
  {
     if(vin_ptr->framerate > 0) argv[2].flt_ret = vin_ptr->framerate;
     g_free(vin_ptr);
   }	 

  p_init_arr_arg(&argv[3], WGT_TOGGLE);
  argv[3].label_txt = _("Copy to Video Buffer");
  argv[3].help_txt  = _("Save all single frames of animated preview to video buffer\n"
                        "(configured in gimprc by video-paste-dir and video-paste-basename)");
  argv[3].int_ret   = 0;

  if(TRUE == p_array_dialog( _("Move Path Animated Preview"),
                                 _("Options"), 
                                  ARGC_APV, argv))
  {
      switch(argv[0].radio_ret)
      {
        case 2: 
           pvals->apv_mode = GAP_APV_EXACT;
           break;
        case 1: 
           pvals->apv_mode = GAP_APV_ONE_FRAME;
           break;
        default:
           pvals->apv_mode = GAP_APV_QUICK;
           break;
      }
      
      pvals->apv_scalex = argv[1].flt_ret;
      pvals->apv_scaley = argv[1].flt_ret;
      pvals->apv_framerate = argv[2].flt_ret;

      if(argv[3].int_ret)
      {
         pvals->apv_gap_paste_buff = p_get_video_paste_name();
	 p_vid_edit_clear();
      }
      else
      {
         pvals->apv_gap_paste_buff = NULL;
      }
      
      if(gap_debug) printf("Generating Animated Preview\n");

        /* TODO: here we should start a thread for calculate and playback of the anim preview, 
	 * so the move_path main window is not blocked until playback exits
	 */
      p_points_to_tab(mgp);
      if(!p_chk_keyframes(mgp))
      {
	return;
      }

      l_new_image_id = p_mov_anim_preview(pvals, mgp->ainfo_ptr, mgp->preview_frame_nr);
      if(l_new_image_id < 0)
      {
	 p_msg_win(GIMP_RUN_INTERACTIVE,
         _("Generate Animated Preview failed\n"));
      }
      else
      {
         return_vals = gimp_run_procedure ("plug_in_animationplay",
                                 &nreturn_vals,
	                         GIMP_PDB_INT32,    GIMP_RUN_NONINTERACTIVE,
				 GIMP_PDB_IMAGE,    l_new_image_id,
				 GIMP_PDB_DRAWABLE, -1,  /* dummy */
                                 GIMP_PDB_END);
      }
      pvals->apv_mlayer_image = -1;
  }

  apv_locked = FALSE;
}  /* end mov_apv_callback */

static void
p_copy_point(gint to_idx, gint from_idx)
{
    pvals->point[to_idx].p_x = pvals->point[from_idx].p_x;
    pvals->point[to_idx].p_y = pvals->point[from_idx].p_y;
    pvals->point[to_idx].opacity = pvals->point[from_idx].opacity;
    pvals->point[to_idx].w_resize = pvals->point[from_idx].w_resize;
    pvals->point[to_idx].h_resize = pvals->point[from_idx].h_resize;
    pvals->point[to_idx].rotation = pvals->point[from_idx].rotation;
    pvals->point[to_idx].ttlx = pvals->point[from_idx].ttlx;
    pvals->point[to_idx].ttly = pvals->point[from_idx].ttly;
    pvals->point[to_idx].ttrx = pvals->point[from_idx].ttrx;
    pvals->point[to_idx].ttry = pvals->point[from_idx].ttry;
    pvals->point[to_idx].tblx = pvals->point[from_idx].tblx;
    pvals->point[to_idx].tbly = pvals->point[from_idx].tbly;
    pvals->point[to_idx].tbrx = pvals->point[from_idx].tbrx;
    pvals->point[to_idx].tbry = pvals->point[from_idx].tbry;

    /* do not copy keyframe */
    pvals->point[to_idx].keyframe_abs = 0;
    pvals->point[to_idx].keyframe = 0;
}


static void
mov_padd_callback (GtkWidget *widget,
		      gpointer	 data)
{
  t_mov_gui_stuff *mgp = data;
  gint l_idx;
  
  if(gap_debug) printf("mov_padd_callback\n");
  l_idx = pvals->point_idx_max;
  if (l_idx < GAP_MOV_MAX_POINT -2)
  {
    /* advance to next point */
    p_points_to_tab(mgp);
    pvals->point_idx_max++;
    pvals->point_idx = pvals->point_idx_max;
    
    /* copy values from previous point to current (new) point */
    p_copy_point(pvals->point_idx_max, l_idx);
    p_point_refresh(mgp);
  }
}

static void
mov_pins_callback (GtkWidget *widget,
		      gpointer	 data)
{
  t_mov_gui_stuff *mgp = data;
  gint l_idx;
  
  if(gap_debug) printf("mov_pins_callback\n");
  l_idx = pvals->point_idx_max;
  if (l_idx < GAP_MOV_MAX_POINT -2)
  {
    /* advance to next point */
    p_points_to_tab(mgp);
    pvals->point_idx_max++;

    for(l_idx = pvals->point_idx_max; l_idx >  pvals->point_idx; l_idx--)
    {
      /* copy values from prev point */
      p_copy_point(l_idx, l_idx-1);
    }
     
    pvals->point_idx++;
    p_point_refresh(mgp);
  }
}

static void
mov_pdel_callback (GtkWidget *widget,
		      gpointer	 data)
{
  t_mov_gui_stuff *mgp = data;
  gint l_idx;
  
  if(gap_debug) printf("mov_pdel_callback\n");
  
  l_idx = pvals->point_idx_max;
  if(pvals->point_idx_max == 0)
  {
    /* This is the las t point to delete */
    p_reset_points();
  }
  else
  {
    for(l_idx = pvals->point_idx; l_idx <  pvals->point_idx_max; l_idx++)
    {
      /* copy values from next point */
      p_copy_point(l_idx, l_idx+1);
    }
    pvals->point_idx_max--;
    pvals->point_idx = MIN(pvals->point_idx, pvals->point_idx_max);
  }
  p_point_refresh(mgp);
  mov_set_instant_apply_request(mgp);
}

static void
mov_pnext_callback (GtkWidget *widget,
		      gpointer	 data)
{
  t_mov_gui_stuff *mgp = data;
  
  if(gap_debug) printf("mov_pnext_callback\n");
  if (pvals->point_idx < pvals->point_idx_max)
  {
    /* advance to next point */
    p_points_to_tab(mgp);
    pvals->point_idx++;
    p_point_refresh(mgp);
    mov_set_instant_apply_request(mgp);
  }
}


static void
mov_pprev_callback (GtkWidget *widget,
		      gpointer	 data)
{
  t_mov_gui_stuff *mgp = data;
  
  if(gap_debug) printf("mov_pprev_callback\n");
  if (pvals->point_idx > 0)
  {
    /* advance to next point */
    p_points_to_tab(mgp);
    pvals->point_idx--;
    p_point_refresh(mgp);
    mov_set_instant_apply_request(mgp);
  }
}

static void
mov_pfirst_callback (GtkWidget *widget,
		      gpointer	 data)
{
  t_mov_gui_stuff *mgp = data;
  
  if(gap_debug) printf("mov_pfirst_callback\n");

  /* advance to first point */
  p_points_to_tab(mgp);
  pvals->point_idx = 0;
  p_point_refresh(mgp);
  mov_set_instant_apply_request(mgp);
}


static void
mov_plast_callback (GtkWidget *widget,
		      gpointer	 data)
{
  t_mov_gui_stuff *mgp = data;
  
  if(gap_debug) printf("mov_plast_callback\n");

  /* advance to first point */
  p_points_to_tab(mgp);
  pvals->point_idx = pvals->point_idx_max;
  p_point_refresh(mgp);
  mov_set_instant_apply_request(mgp);
}


static void
mov_pclr_callback (GtkWidget *widget,
		      gpointer	 data)
{
  t_mov_gui_stuff *mgp = data;
  
  if(gap_debug) printf("mov_pclr_callback\n");
  p_clear_one_point(pvals->point_idx);         /* clear the current point */
  p_point_refresh(mgp);
  mov_set_instant_apply_request(mgp);
}

static void
mov_pdel_all_callback (GtkWidget *widget,
		      gpointer	 data)
{
  t_mov_gui_stuff *mgp = data;
  
  if(gap_debug) printf("mov_pdel_all_callback\n");
  p_reset_points();
  p_point_refresh(mgp);
  mov_set_instant_apply_request(mgp);
}

static void
mov_pclr_all_callback (GtkWidget *widget,
		      gpointer	 data)
{
  gint l_idx;
  t_mov_gui_stuff *mgp = data;
  
  if(gap_debug) printf("mov_pclr_all_callback\n");
 
  for(l_idx = 0; l_idx <= pvals->point_idx_max; l_idx++)
  {
    p_clear_one_point(l_idx);
  }
  p_point_refresh(mgp);
  mov_set_instant_apply_request(mgp);
}


static void
mov_prot_follow_callback (GtkWidget *widget,
			 GdkEventButton *bevent,
		         gpointer data)
{
  gdouble  l_startangle;
  int     key_modifier;
  
  t_mov_gui_stuff *mgp = data;
  key_modifier = 0;
  
  if(gap_debug) printf("mov_prot_follow_callback\n");

  /* SHIFT: GDK_SHIFT_MASK,
   * CTRL:  GDK_CONTROL_MASK,
   * ALT:   GDK_MOD1_MASK
   */
  key_modifier = 0;
  if (bevent->state & GDK_SHIFT_MASK)
  {
    key_modifier = 1;       /* SHIFT */
  }

  if(gap_debug)
  {  
    printf("key_modifier %d\n", (int)key_modifier);
  }

  if( pvals->point_idx_max > 1)
  {
    l_startangle = 0.0;
    if(key_modifier != 0)
    {
      /* SHIFT */
      p_points_to_tab(mgp);
      l_startangle = pvals->point[0].rotation;
    }
    p_calculate_rotate_follow(pvals, l_startangle);
  }

  p_point_refresh(mgp);
  mov_set_instant_apply_request(mgp);
}

static void
p_filesel_close_cb(GtkWidget *widget,
		   t_mov_gui_stuff *mgp)
{
  if(mgp->filesel == NULL) return;

  gtk_widget_destroy(GTK_WIDGET(mgp->filesel));
  mgp->filesel = NULL;   /* now filesel is closed */
}

static void
mov_pload_callback (GtkWidget *widget,
		      gpointer	 data)
{
  GtkWidget *filesel;
  t_mov_gui_stuff *mgp = data;

  if(mgp->filesel != NULL) return;  /* filesel is already open */
  
  filesel = gtk_file_selection_new ( _("Load Path Points from file"));
  mgp->filesel = filesel;

  gtk_window_set_position (GTK_WINDOW (filesel), GTK_WIN_POS_MOUSE);

  g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button),
		   "clicked",
                    G_CALLBACK (p_points_load_from_file),
		    mgp);

  g_signal_connect(G_OBJECT (GTK_FILE_SELECTION (filesel)->cancel_button),
		  "clicked",
                   G_CALLBACK (p_filesel_close_cb),
	           mgp);
	             
  gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesel),
				   mgp->pointfile_name);

  gtk_widget_show (filesel);
  /* "destroy" has to be the last signal, 
   * (otherwise the other callbacks are never called)
   */
  g_signal_connect (G_OBJECT (filesel), "destroy",
		    G_CALLBACK (p_filesel_close_cb),
		    mgp);
}


static void
mov_psave_callback (GtkWidget *widget,
		      gpointer	 data)
{
  GtkWidget *filesel;
  t_mov_gui_stuff *mgp = data;

  if(mgp->filesel != NULL) return;  /* filesel is already open */
  
  filesel = gtk_file_selection_new ( _("Save Path Points to file"));
  mgp->filesel = filesel;

  gtk_window_set_position (GTK_WINDOW (filesel), GTK_WIN_POS_MOUSE);

  g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button),
		   "clicked",
                   G_CALLBACK (p_points_save_to_file),
		   mgp);

  g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (filesel)->cancel_button),
		   "clicked",
                    G_CALLBACK (p_filesel_close_cb),
		    mgp);

  gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesel),
				   mgp->pointfile_name);

  gtk_widget_show (filesel);
  /* "destroy" has to be the last signal, 
   * (otherwise the other callbacks are never called)
   */
  g_signal_connect (G_OBJECT (filesel), "destroy",
		    G_CALLBACK (p_filesel_close_cb),
		    mgp);
}


static void 
p_points_load_from_file (GtkWidget *widget,
		      t_mov_gui_stuff *mgp)
{
  const gchar        *filename;
  
  if(gap_debug) printf("p_points_load_from_file\n");
  if(mgp->filesel == NULL) return;
  
  filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (mgp->filesel));
  g_free(mgp->pointfile_name);
  mgp->pointfile_name = g_strdup(filename);

  if(gap_debug) printf("p_points_load_from_file %s\n", mgp->pointfile_name);

  gtk_widget_destroy(GTK_WIDGET(mgp->filesel));
  mgp->filesel = NULL;

  p_load_points(mgp->pointfile_name);
  p_point_refresh(mgp);
  mov_set_instant_apply_request(mgp);
}  /* end p_points_load_from_file */


static void
p_points_save_to_file (GtkWidget *widget,
		      t_mov_gui_stuff *mgp)
{
  const gchar        *filename;
  
  if(gap_debug) printf("p_points_save_to_file\n");
  if(mgp->filesel == NULL) return;
  
  filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (mgp->filesel));
  g_free(mgp->pointfile_name);
  mgp->pointfile_name = g_strdup(filename);

  if(gap_debug) printf("p_points_save_to_file %s\n", mgp->pointfile_name);

  gtk_widget_destroy(GTK_WIDGET(mgp->filesel));
  mgp->filesel = NULL;

  p_points_to_tab(mgp);
  p_save_points(mgp->pointfile_name);
  p_point_refresh(mgp);
}  /* end p_points_save_to_file */


static void
p_point_refresh(t_mov_gui_stuff *mgp)
{
  p_points_from_tab(mgp);
  p_update_point_index_text(mgp);

  if(gap_debug) printf("p_point_refresh:newval in_call=%d\n", mgp->in_call );
  if( !mgp->in_call )
  {
      mov_path_prevw_cursor_update( mgp );
      mov_path_prevw_draw ( mgp, CURSOR | PATH_LINE );
  }
  mgp->in_call = TRUE;

  gtk_adjustment_set_value (mgp->x_adj,
			    (gdouble)mgp->p_x);
  gtk_adjustment_set_value (mgp->y_adj,
			    (gdouble)mgp->p_y);    
  gtk_adjustment_set_value (mgp->wres_adj,
			    (gdouble)mgp->w_resize);
  gtk_adjustment_set_value (mgp->hres_adj,
			    (gdouble)mgp->h_resize);
  gtk_adjustment_set_value (mgp->opacity_adj,
			    (gdouble)mgp->opacity);
  gtk_adjustment_set_value (mgp->rotation_adj,
			    (gdouble)mgp->rotation);
  gtk_adjustment_set_value (mgp->keyframe_adj,
			    (gdouble)mgp->keyframe_abs);
  gtk_adjustment_set_value (mgp->ttlx_adj,
			    (gdouble)mgp->ttlx);
  gtk_adjustment_set_value (mgp->ttly_adj,
			    (gdouble)mgp->ttly);
  gtk_adjustment_set_value (mgp->ttrx_adj,
			    (gdouble)mgp->ttrx);
  gtk_adjustment_set_value (mgp->ttry_adj,
			    (gdouble)mgp->ttry);
  gtk_adjustment_set_value (mgp->tblx_adj,
			    (gdouble)mgp->tblx);
  gtk_adjustment_set_value (mgp->tbly_adj,
			    (gdouble)mgp->tbly);
  gtk_adjustment_set_value (mgp->tbrx_adj,
			    (gdouble)mgp->tbrx);
  gtk_adjustment_set_value (mgp->tbry_adj,
			    (gdouble)mgp->tbry);

  mgp->in_call = FALSE;
}	/* end p_point_refresh */

static void
p_pick_nearest_point(gint px, gint py)
{
  gint l_idx;
  gint l_idx_min;
  gdouble l_sq_dist;
  gdouble l_dx, l_dy;
  gdouble l_sq_dist_min;
  
  l_idx_min = 0;
  l_sq_dist_min = G_MAXDOUBLE;

  if(gap_debug) printf("\np_pick_nearest_point: near to %4d %4d\n", (int)px, (int)py);

  for(l_idx = pvals->point_idx_max; l_idx >= 0; l_idx--)
  {
    /* calculate x and y distance */
    l_dx = pvals->point[l_idx].p_x - px;
    l_dy = pvals->point[l_idx].p_y - py;
    
    /* calculate square of the distance */
    l_sq_dist = (l_dx * l_dx) + (l_dy * l_dy);
    if(l_sq_dist < l_sq_dist_min)
    {
      l_sq_dist_min = l_sq_dist;
      l_idx_min = l_idx;
    }

    if(gap_debug)
    {
       printf("  [%2d] %4d %4d %f\n",
             (int)l_idx, 
             (int)pvals->point[l_idx].p_x,
             (int)pvals->point[l_idx].p_y,
             (float)l_sq_dist
             );
    }
  }
  if(gap_debug) printf("p_pick_nearest_point: selected %d\n", (int)l_idx_min);
     
  pvals->point_idx = l_idx_min;
  pvals->point[pvals->point_idx].p_x = px;
  pvals->point[pvals->point_idx].p_y = py;
}	/* end p_pick_nearest_point */



static void
mov_imglayer_menu_callback(gint32 id, gpointer data)
{
  pvals->src_layer_id = id;
  pvals->src_image_id = gimp_layer_get_image_id(id);

  
  if(gap_debug) printf("mov_imglayer_menu_callback: image_id=%ld layer_id=%ld\n",
         (long)pvals->src_image_id, (long)pvals->src_layer_id);

  mov_set_instant_apply_request((t_mov_gui_stuff *) data);
} /* end mov_imglayer_menu_callback */

static gint
mov_imglayer_constrain(gint32 image_id, gint32 drawable_id, gpointer data)
{
  gint32 l_src_image_id;

  if(gap_debug) printf("GAP-DEBUG: mov_imglayer_constrain PROCEDURE\n");

  if(drawable_id < 0)
  {
     /* gimp 1.1 makes a first call of the constraint procedure
      * with drawable_id = -1, and skips the whole image if FALSE is returned
      */
     return(TRUE);
  }
  
  l_src_image_id = gimp_layer_get_image_id(drawable_id);
  
   /* dont accept layers from within the destination image id 
    * or layers within the tmp preview image
    * conversions between different base_types are not supported in this version
    */
   return((l_src_image_id != pvals->dst_image_id) &&
          (l_src_image_id != pvals->tmp_image_id) &&
          (gimp_image_base_type(l_src_image_id) == gimp_image_base_type(pvals->tmp_image_id)) );
} /* end mov_imglayer_constrain */

static void
mov_paintmode_menu_callback (GtkWidget *widget,  gpointer   client_data)
{
  t_mov_gui_stuff *mgp;
  
  pvals->src_paintmode = (gint)client_data;
  mgp = g_object_get_data( G_OBJECT(widget), "mgp" );
  mov_set_instant_apply_request(mgp);
}

static void
mov_handmode_menu_callback (GtkWidget *widget,  gpointer   client_data)
{
  t_mov_gui_stuff *mgp;
  
  pvals->src_handle = (gint)client_data;
  mgp = g_object_get_data( G_OBJECT(widget), "mgp" );
  mov_set_instant_apply_request(mgp);
}

static void
mov_stepmode_menu_callback (GtkWidget *widget, gpointer   client_data)
{
  gboolean l_sensitive;
  GtkWidget *spinbutton;
  t_mov_gui_stuff *mgp;
  
  pvals->src_stepmode = (gint)client_data;

  mgp = g_object_get_data( G_OBJECT(widget), "mgp" );
  if(mgp == NULL) return;

  l_sensitive = TRUE;
  if((pvals->src_stepmode == GAP_STEP_NONE) 
  || (pvals->src_stepmode == GAP_STEP_FRAME_NONE))
  {
    l_sensitive = FALSE;
  }

  
  spinbutton = GTK_WIDGET(g_object_get_data (G_OBJECT (mgp->step_speed_factor_adj), "spinbutton"));
  if(spinbutton)
  {
    gtk_widget_set_sensitive(spinbutton, l_sensitive);
  }
}  /* end mov_stepmode_menu_callback */


static void 
mov_tweenlayer_sensitivity(t_mov_gui_stuff *mgp)
{
  gboolean l_sensitive;
  GtkWidget *spinbutton;
  
  l_sensitive = TRUE;
  if(pvals->tween_steps < 1)
  {
    l_sensitive = FALSE;
  }

  spinbutton = GTK_WIDGET(g_object_get_data (G_OBJECT (mgp->tween_opacity_initail_adj), "spinbutton"));
  if(spinbutton)
  {
    gtk_widget_set_sensitive(spinbutton, l_sensitive);
  }

  spinbutton = GTK_WIDGET(g_object_get_data (G_OBJECT (mgp->tween_opacity_desc_adj), "spinbutton"));
  if(spinbutton)
  {
    gtk_widget_set_sensitive(spinbutton, l_sensitive);
  }
}  /* end mov_tweenlayer_sensitivity */

static void 
mov_tracelayer_sensitivity(t_mov_gui_stuff *mgp)
{
  gboolean l_sensitive;
  GtkWidget *spinbutton;
  
  l_sensitive = FALSE;
  if(pvals->tracelayer_enable)
  {
    l_sensitive = TRUE;
  }

  spinbutton = GTK_WIDGET(g_object_get_data (G_OBJECT (mgp->trace_opacity_initial_adj), "spinbutton"));
  if(spinbutton)
  {
    gtk_widget_set_sensitive(spinbutton, l_sensitive);
  }

  spinbutton = GTK_WIDGET(g_object_get_data (G_OBJECT (mgp->trace_opacity_desc_adj), "spinbutton"));
  if(spinbutton)
  {
    gtk_widget_set_sensitive(spinbutton, l_sensitive);
  }
}  /* end mov_tracelayer_sensitivity */



static void
mov_gint_toggle_callback(GtkWidget *w, gpointer   client_data)
{
  gint *data;
  
  data = (gint*)client_data;
  
  if (GTK_TOGGLE_BUTTON (w)->active)
  {
    *data = 1;
  }
  else
  {
    *data = 0;
  }
}  /* end mov_gint_toggle_callback */

static void 
mov_force_visibility_toggle_callback    (GtkWidget *widget, gpointer client_data)
{
  t_mov_gui_stuff *mgp;

  if((widget == NULL) || (client_data == NULL))
  {
    return;
  }
  mov_gint_toggle_callback(widget, client_data);
  mgp = g_object_get_data( G_OBJECT(widget), "mgp" );
  mov_set_instant_apply_request(mgp);
  
}  /* end mov_force_visibility_toggle_callback */

static void 
mov_tracelayer_callback    (GtkWidget *widget, gpointer client_data)
{
  t_mov_gui_stuff *mgp;

  if((widget == NULL) || (client_data == NULL))
  {
    return;
  }
  mov_gint_toggle_callback(widget, client_data);
  mgp = g_object_get_data( G_OBJECT(widget), "mgp" );

  if(mgp)
  {
    mov_tracelayer_sensitivity(mgp);
  }
  
}  /* end mov_tracelayer_callback */



static void
mov_show_path_or_cursor(t_mov_gui_stuff *mgp)
{
  if(mgp == NULL) return;
  if(mgp->startup) return;
  if(mgp->pv_ptr == NULL) return;
  if(mgp->pv_ptr->da_widget == NULL) return;
  if(mgp->drawable == NULL) return;
  
  p_point_refresh(mgp);
  mov_path_prevw_draw ( mgp, CURSOR | PATH_LINE );
  gtk_widget_queue_draw(mgp->pv_ptr->da_widget);
  gdk_flush();  
}  /* end mov_show_path_or_cursor */

static void
mov_show_path_callback(GtkWidget *widget, t_mov_gui_stuff *mgp)
{
  mov_gint_toggle_callback(widget, &mgp->show_path);
  mov_show_path_or_cursor(mgp);
}  /* end mov_show_path_callback */

static void
mov_show_cursor_callback(GtkWidget *widget, t_mov_gui_stuff *mgp)
{
  mov_gint_toggle_callback(widget, &mgp->show_cursor);
  mov_show_path_or_cursor(mgp);
}  /* end mov_show_cursor_callback */

static void
mov_show_grid_callback(GtkWidget *widget, t_mov_gui_stuff *mgp)
{
  mov_gint_toggle_callback(widget, &mgp->show_grid);
  mov_show_path_or_cursor(mgp);
  //XX grid rendering and picking not implemented yet !!!
}  /* end mov_show_grid_callback */


/* --------------------------
 * install / remove timer
 * --------------------------
 */
static void
mov_install_timer(t_mov_gui_stuff *mgp)
{
  if(mgp->instant_timertag < 0)
  {
    mgp->instant_timertag = (gint32) g_timeout_add(INSTANT_TIMERINTERVAL_MILLISEC,
                                      (GtkFunction)mov_instant_timer_callback
				      , mgp);
  }
}  /* end mov_install_timer */

static void
mov_remove_timer(t_mov_gui_stuff *mgp)
{
  if(mgp->instant_timertag >= 0)
  {
    g_source_remove(mgp->instant_timertag);
    mgp->instant_timertag = -1;
  }
}  /* end mov_remove_timer */

/* --------------------------
 * mov_instant_timer_callback
 * --------------------------
 * This procedure is called periodically via timer
 * when the instant_apply checkbox is ON
 */
static void
mov_instant_timer_callback(gpointer   user_data)
{
  t_mov_gui_stuff *mgp;

  mgp = user_data;
  if(mgp == NULL)
  {
    return;
  }

  mov_remove_timer(mgp);

  if(mgp->instant_apply_request)
  {
    if(gap_debug) printf("FIRE mov_instant_timer_callback >>>> REQUEST is TRUE\n");
    mov_upvw_callback (NULL, mgp);  /* the request is cleared in this procedure */
  }

  /* restart timer for next cycle */
  if(mgp->instant_apply)
  {
     mov_install_timer(mgp);
  }
}  /* end mov_instant_timer_callback */

static void
mov_instant_apply_callback(GtkWidget *widget, t_mov_gui_stuff *mgp)
{
  mov_gint_toggle_callback(widget, &mgp->instant_apply);
  if(mgp->instant_apply)
  {
    mov_set_instant_apply_request(mgp);
    mov_install_timer(mgp);
  }
  else
  {
    mov_remove_timer(mgp);
  }
}  /* end mov_instant_apply_callback */


/* ============================================================================
 * procedures to handle POINTS - TABLE
 * ============================================================================
 */
 
 
static void
p_points_from_tab(t_mov_gui_stuff *mgp)
{
  GtkWidget *scale;
  GtkWidget *spinbutton;

  mgp->p_x      = pvals->point[pvals->point_idx].p_x;
  mgp->p_y      = pvals->point[pvals->point_idx].p_y;
  mgp->opacity  = pvals->point[pvals->point_idx].opacity;
  mgp->w_resize = pvals->point[pvals->point_idx].w_resize;
  mgp->h_resize = pvals->point[pvals->point_idx].h_resize;
  mgp->rotation = pvals->point[pvals->point_idx].rotation;
  mgp->ttlx     = pvals->point[pvals->point_idx].ttlx;
  mgp->ttly     = pvals->point[pvals->point_idx].ttly;
  mgp->ttrx     = pvals->point[pvals->point_idx].ttrx;
  mgp->ttry     = pvals->point[pvals->point_idx].ttry;
  mgp->tblx     = pvals->point[pvals->point_idx].tblx;
  mgp->tbly     = pvals->point[pvals->point_idx].tbly;
  mgp->tbrx     = pvals->point[pvals->point_idx].tbrx;
  mgp->tbry     = pvals->point[pvals->point_idx].tbry;
  mgp->keyframe_abs = pvals->point[pvals->point_idx].keyframe_abs;
  
  if(( mgp->keyframe_adj != NULL) && (mgp->startup != TRUE))
  {
   /*   findout the gtk_widgets (scale and spinbutton) connected
    *   to mgp->keyframe_adj
    *   and set_sensitive to TRUE or FALSE
    */
    scale = GTK_WIDGET(g_object_get_data (G_OBJECT (mgp->keyframe_adj), "scale"));
    spinbutton = GTK_WIDGET(g_object_get_data (G_OBJECT (mgp->keyframe_adj), "spinbutton"));

    if(spinbutton == NULL)
    {
      return;
    }
    if(gap_debug)
    {
      printf("p_points_from_tab: scale %x spinbutton %x\n",
              (int)scale, (int)spinbutton);
    }
    if((pvals->point_idx == 0) || (pvals->point_idx == pvals->point_idx_max))
    {
      gtk_widget_set_sensitive(spinbutton, FALSE);
      if(scale) 
        gtk_widget_set_sensitive(scale, FALSE);
    }
    else
    {
      gtk_widget_set_sensitive(spinbutton, TRUE);
      if(scale) 
        gtk_widget_set_sensitive(scale, TRUE);
    }
  }
}

static void
p_points_to_tab(t_mov_gui_stuff *mgp)
{
  if(gap_debug) printf("p_points_to_tab: idx=%d, rotation=%f\n", (int)pvals->point_idx , (float)mgp->rotation);
  
  pvals->point[pvals->point_idx].p_x       = mgp->p_x;
  pvals->point[pvals->point_idx].p_y       = mgp->p_y;
  pvals->point[pvals->point_idx].opacity   = mgp->opacity;
  pvals->point[pvals->point_idx].w_resize  = mgp->w_resize;
  pvals->point[pvals->point_idx].h_resize  = mgp->h_resize;
  pvals->point[pvals->point_idx].rotation  = mgp->rotation;
  pvals->point[pvals->point_idx].ttlx      = mgp->ttlx;
  pvals->point[pvals->point_idx].ttly      = mgp->ttly;
  pvals->point[pvals->point_idx].ttrx      = mgp->ttrx;
  pvals->point[pvals->point_idx].ttry      = mgp->ttry;
  pvals->point[pvals->point_idx].tblx      = mgp->tblx;
  pvals->point[pvals->point_idx].tbly      = mgp->tbly;
  pvals->point[pvals->point_idx].tbrx      = mgp->tbrx;
  pvals->point[pvals->point_idx].tbry      = mgp->tbry;
  pvals->point[pvals->point_idx].keyframe_abs  = mgp->keyframe_abs;
  if((mgp->keyframe_abs > 0) 
  && (pvals->point_idx != 0)
  && (pvals->point_idx != pvals->point_idx_max))
  {
    pvals->point[pvals->point_idx].keyframe = p_conv_keyframe_to_rel(mgp->keyframe_abs, pvals);
  }
  else
  {
    pvals->point[pvals->point_idx].keyframe  = 0;
  }
}

void
p_update_point_index_text(t_mov_gui_stuff *mgp)
{ 
  g_snprintf (&mgp->point_index_text[0], POINT_INDEX_LABEL_LENGTH,
	      _("Current Point: [ %3d ] of [ %3d ]"), 
	      pvals->point_idx + 1, pvals->point_idx_max +1);

  if (mgp->point_index_frame)
    {
      gtk_frame_set_label (GTK_FRAME (mgp->point_index_frame),
			  &mgp->point_index_text[0]);
    }
}


/* ============================================================================
 * p_clear_one_point
 *   Init point table with identical 2 Points
 * ============================================================================
 */
void 
p_clear_one_point(gint idx)
{
  if((idx >= 0) && (idx <= pvals->point_idx_max))
  {
    pvals->point[idx].opacity  = 100.0; /* 100 percent (no transparecy) */
    pvals->point[idx].w_resize = 100.0; /* 100%  no resizize (1:1) */
    pvals->point[idx].h_resize = 100.0; /* 100%  no resizize (1:1) */
    pvals->point[idx].rotation = 0.0;   /* no rotation (0 degree) */
    /* 1.0 for all perspective transform factors (== no perspective transformation) */
    pvals->point[idx].ttlx      = 1.0;
    pvals->point[idx].ttly      = 1.0;
    pvals->point[idx].ttrx      = 1.0;
    pvals->point[idx].ttry      = 1.0;
    pvals->point[idx].tblx      = 1.0;
    pvals->point[idx].tbly      = 1.0;
    pvals->point[idx].tbrx      = 1.0;
    pvals->point[idx].tbry      = 1.0;
    pvals->point[idx].keyframe = 0;   /* 0: controlpoint is not fixed to keyframe */
    pvals->point[idx].keyframe_abs = 0;   /* 0: controlpoint is not fixed to keyframe */
  }
}	/* end p_clear_one_point */


/* ============================================================================
 * p_reset_points
 *   Init point table with identical 2 Points
 * ============================================================================
 */
void p_reset_points()
{
  pvals->point_idx = 0;        /* 0 == current point */
  pvals->point_idx_max = 0;    /* 0 == there is only one valid point */
  p_clear_one_point(0);
  pvals->point[0].p_x = 0;
  pvals->point[0].p_y = 0;
}	/* end p_reset_points */

/* ============================================================================
 * p_load_points
 *   load point table (from named file into global pvals)
 *   (reset points if load failed)
 * ============================================================================
 */

void 
p_load_points(char *filename)
{
  gint l_rc;
  gint l_errno;
  
  l_rc = p_gap_load_pointfile(filename, pvals);
  l_errno = errno;
  
  if (l_rc < -1)
  {
    p_reset_points();
  }
  if (l_rc != 0)
  {
    if(l_errno != 0)
    {
      g_message(_("ERROR: Could not open Controlpoints\n"
	        "filename: '%s'\n%s")
	     ,filename, g_strerror (errno));
    }
    else
    {
      g_message(_("ERROR: Could not read Controlpoints\n"
	        "filename: '%s'\n(Is not a valid Controlpoint File)")
	     ,filename);
    }
  }
}  /* end p_load_points */

/* ============================================================================
 * p_save_points
 *   save point table (from global pvals into named file)
 * ============================================================================
 */
static void 
p_save_points(char *filename)
{
  gint l_rc;
  gint l_errno;

  if(p_overwrite_file_dialog(filename))
  {
    l_rc = p_gap_save_pointfile(filename, pvals);
    l_errno = errno;

    if(l_rc != 0)
    {
	g_message (_("Failed to write Controlpointfile\n"
                      "filename: '%s':\n%s"),
		   filename, g_strerror (l_errno));
    }
  }
}	/* end p_save_points */


/* ============================================================================
 * Create new source selection table Frame, and return it.
 *   A frame that contains:
 *   - 2x2 menus (src_image/layer, handle, stepmode, paintmode)
 * ============================================================================
 */

static GtkWidget *
mov_src_sel_create(t_mov_gui_stuff *mgp)
{
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *sub_table;
  GtkWidget *option_menu;
  GtkWidget *menu;
  GtkWidget *label;
  GtkObject      *adj;
  gint gettextize_loop;


  /* the frame */
  frame = gtk_frame_new ( _("Source Select") );
  gtk_frame_set_shadow_type (GTK_FRAME (frame) ,GTK_SHADOW_ETCHED_IN);
  
  /* the table */
  table = gtk_table_new (2, 4, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);

  /* Source Layer menu */
  label = gtk_label_new( _("Source Image/Layer:"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show(label);

  option_menu = gtk_option_menu_new();
  gtk_table_attach(GTK_TABLE(table), option_menu, 1, 2, 0, 1,
		   GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  gimp_help_set_help_data(option_menu,
                       _("Source Object to insert into Frame Range")
                       , NULL);

  gtk_widget_show(option_menu);

  menu = gimp_layer_menu_new(mov_imglayer_constrain,
			     mov_imglayer_menu_callback,
			     mgp,
			     pvals->src_layer_id);
  gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
  gtk_widget_show(option_menu);


  /* Paintmode menu */

  label = gtk_label_new( _("Mode:"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 2, 3, 0, 1, GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show(label);

  option_menu = gtk_option_menu_new();
  gtk_table_attach(GTK_TABLE(table), option_menu, 3, 4, 0, 1,
		   GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gimp_help_set_help_data(option_menu,
                       _("Paintmode")
                       , NULL);
  gtk_widget_show(option_menu);

  for (gettextize_loop = 0; option_paint_items[gettextize_loop].label != NULL;
       gettextize_loop++)
    option_paint_items[gettextize_loop].label =
      gettext(option_paint_items[gettextize_loop].label);

  menu = p_buildmenu (option_paint_items, mgp);
  gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
  gtk_widget_show(option_menu);



  /* Loop Stepmode menu (Label) */

  label = gtk_label_new( _("Stepmode:"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show(label);

  /* the sub_table (1 row) */
  sub_table = gtk_table_new (1, 3, FALSE);
  gtk_widget_show(sub_table);
  gtk_container_set_border_width (GTK_CONTAINER (sub_table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (sub_table), 0);
  gtk_table_set_col_spacings (GTK_TABLE (sub_table), 2);

  gtk_table_attach(GTK_TABLE(table), sub_table, 1, 2, 1, 2,
		   GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);


  /* Loop Stepmode menu */

  option_menu = gtk_option_menu_new();
  gtk_table_attach(GTK_TABLE(sub_table), option_menu, 0, 1, 0, 1,
		   GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show(option_menu);

  for (gettextize_loop = 0; option_step_items[gettextize_loop].label != NULL;
       gettextize_loop++)
    option_step_items[gettextize_loop].label =
      gettext(option_step_items[gettextize_loop].label);

  menu = p_buildmenu (option_step_items, mgp);
  gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
  gimp_help_set_help_data(option_menu,
                       _("How to fetch the next SrcLayer at the next handled frame")
                       , NULL);
  gtk_widget_show(option_menu);

  /* StepSpeedFactor */
  adj = p_mov_spinbutton_new( GTK_TABLE (sub_table), 1, 0,    /* table col, row */
			  _("SpeedFactor:"),                  /* label text */
			  SCALE_WIDTH, ENTRY_WIDTH,           /* scalesize spinsize */
			  (gdouble)pvals->step_speed_factor,  /* initial value */
			  (gdouble)0.0, (gdouble)50.0,        /* lower, upper */
			  0.1, 1,                             /* step, page */
			  3,                                  /* digits */
			  FALSE,                              /* constrain */
			  (gdouble)0.0, (gdouble)50.0,        /* lower, upper (unconstrained) */
			  _("Step Speed Factor\n"
			    "Source and Targetframes step synchron at value 1.0\n"
			    "A Value of 0.5 will step the Source half time slower\n"
			    "(1 Sourcestep only at every 2.nd Targetframe)"),
			  NULL);    /* tooltip privatetip */
  g_signal_connect (G_OBJECT (adj), "value_changed",
		    G_CALLBACK (gimp_double_adjustment_update),
		    &pvals->step_speed_factor);
  mgp->step_speed_factor_adj = GTK_ADJUSTMENT(adj);


  /* Source Image Handle menu */

  label = gtk_label_new( _("Handle:"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 2, 3, 1, 2, GTK_FILL, GTK_FILL, 4, 0);
  gtk_widget_show(label);

  option_menu = gtk_option_menu_new();
  gtk_table_attach(GTK_TABLE(table), option_menu, 3, 4, 1, 2,
		   GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show(option_menu);

  for (gettextize_loop = 0; option_handle_items[gettextize_loop].label != NULL;
       gettextize_loop++)
    option_handle_items[gettextize_loop].label =
      gettext(option_handle_items[gettextize_loop].label);

  menu = p_buildmenu (option_handle_items, mgp);
  gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
  gimp_help_set_help_data(option_menu,
                       _("How to place the SrcLayer at Controlpoint Coordinates")
                       , NULL);
  gtk_widget_show(option_menu);

  gtk_widget_show( table );

#ifndef MOVE_PATH_FRAMES_ENABLE
  /* copile without MOVE_PATH_LAYOUT_frame needs less space */  
  return table;
#else
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 2);
  gtk_widget_show(frame);  
  return frame;
#endif  
}	/* end mov_src_sel_create */

 
/* ============================================================================
 * Create set of widgets for the advanced Move Path features
 *   A frame that contains:
 *   in the 1.st row
 *   - 3x spionbutton  for tween_steps, tween_opacity_init, tween_opacity_desc
 *   in the 2.nd row
 *   - checkbutton  make_tracelayer
 *   - 2x spinbutton   for trace_opacity_initial, trace_opacity_desc
 * ============================================================================
 */

static GtkWidget *
mov_advanced_tab_create(t_mov_gui_stuff *mgp)
{
  GtkWidget	 *frame;
  GtkWidget	 *table;
  GtkWidget	 *check_button;
  GtkObject      *adj;


  /* the frame */
  frame = gtk_frame_new ( _("Advanced Settings") );
  gtk_frame_set_shadow_type (GTK_FRAME (frame) ,GTK_SHADOW_ETCHED_IN);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 2);
  
  /* the table (2 rows) */
  table = gtk_table_new (2, 6, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);

  /* toggle Tracelayer */
  check_button = gtk_check_button_new_with_label ( _("Tracelayer"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button),
                		  pvals->tracelayer_enable);
  gimp_help_set_help_data(check_button,
                         _("Create an additional Trace Layer in all handled Frames")
                         , NULL);
  gtk_widget_show (check_button);
  gtk_table_attach(GTK_TABLE(table), check_button, 0, 1, 0, 1
                  ,0 , 0, 0, 0);
  g_object_set_data(G_OBJECT(check_button), "mgp", mgp);
  g_signal_connect (G_OBJECT (check_button), "toggled",
                      G_CALLBACK  (mov_tracelayer_callback),
                      &pvals->tracelayer_enable);

  /* TraceOpacityInitial */
  adj = p_mov_spinbutton_new( GTK_TABLE (table), 2, 0,          /* table col, row */
			  _("TraceOpacity1:"),                  /* label text */
			  SCALE_WIDTH, ENTRY_WIDTH,             /* scalesize spinsize */
			  (gdouble)pvals->trace_opacity_initial,  /* initial value */
			  (gdouble)0.0, (gdouble)100,           /* lower, upper */
			  1, 10,                                /* step, page */
			  1,                                    /* digits */
			  FALSE,                                /* constrain */
			  (gdouble)0.0, (gdouble)100,           /* lower, upper (unconstrained) */
			  _("Initial Opacity of the Tracelayer"),
			  NULL);    /* tooltip privatetip */
  g_signal_connect (G_OBJECT (adj), "value_changed",
		    G_CALLBACK (gimp_double_adjustment_update),
		    &pvals->trace_opacity_initial);
  mgp->trace_opacity_initial_adj = GTK_ADJUSTMENT(adj);


  /* TraceOpacityDescending */
  adj = p_mov_spinbutton_new( GTK_TABLE (table), 4, 0,          /* table col, row */
			  _("TraceOpacity2:"),                  /* label text */
			  SCALE_WIDTH, ENTRY_WIDTH,             /* scalesize spinsize */
			  (gdouble)pvals->trace_opacity_desc,     /* initial value */
			  (gdouble)0.0, (gdouble)100,           /* lower, upper */
			  1, 10,                                /* step, page */
			  1,                                    /* digits */
			  FALSE,                                /* constrain */
			  (gdouble)0.0, (gdouble)100,           /* lower, upper (unconstrained) */
			  _("Descending Opacity of the Tracelayer"),
			  NULL);    /* tooltip privatetip */
  g_signal_connect (G_OBJECT (adj), "value_changed",
		    G_CALLBACK (gimp_double_adjustment_update),
		    &pvals->trace_opacity_desc);
  mgp->trace_opacity_desc_adj = GTK_ADJUSTMENT(adj);


  /* TweenSteps */
  adj = p_mov_spinbutton_new( GTK_TABLE (table), 0, 1,          /* table col, row */
			  _("TweenSteps:"),                     /* label text */
			  SCALE_WIDTH, ENTRY_WIDTH,             /* scalesize spinsize */
			  (gdouble)pvals->tween_steps,          /* initial value */
			  (gdouble)0, (gdouble)50,              /* lower, upper */
			  1, 2,                                 /* step, page */
			  0,                                    /* digits */
			  FALSE,                                /* constrain */
			  (gdouble)0, (gdouble)50,              /* lower, upper (unconstrained) */
			  _("Calculate n Steps between 2 Frames\n"
			    "The Rendered Tweensteps are collected in a Tweenlayer\n"
			    "that will be added to the handeld destination frames\n"
			    "If Value is 0, No tweens are calculated\n"
			    "and no Tweenlayers are created"),
			  NULL);    /* tooltip privatetip */
  mgp->tween_steps_adj = GTK_ADJUSTMENT(adj);
  g_object_set_data(G_OBJECT(adj), "mgp", mgp);
  g_signal_connect (G_OBJECT (adj), "value_changed",
		    G_CALLBACK (mov_path_tween_steps_adjustment_update),
		    &pvals->tween_steps);


  /* TweenOpacityInitial */
  adj = p_mov_spinbutton_new( GTK_TABLE (table), 2, 1,          /* table col, row */
			  _("TweenOpacity1:"),                  /* label text */
			  SCALE_WIDTH, ENTRY_WIDTH,             /* scalesize spinsize */
			  (gdouble)pvals->tween_opacity_initail,  /* initial value */
			  (gdouble)0.0, (gdouble)100,           /* lower, upper */
			  1, 10,                                /* step, page */
			  1,                                    /* digits */
			  FALSE,                                /* constrain */
			  (gdouble)0.0, (gdouble)100,           /* lower, upper (unconstrained) */
			  _("Initial Opacity of the Tweenlayer"),
			  NULL);    /* tooltip privatetip */
  g_signal_connect (G_OBJECT (adj), "value_changed",
		    G_CALLBACK (gimp_double_adjustment_update),
		    &pvals->tween_opacity_initail);
  mgp->tween_opacity_initail_adj = GTK_ADJUSTMENT(adj);

  /* TweenOpacityDescending */
  adj = p_mov_spinbutton_new( GTK_TABLE (table), 4, 1,          /* table col, row */
			  _("TweenOpacity2:"),                  /* label text */
			  SCALE_WIDTH, ENTRY_WIDTH,             /* scalesize spinsize */
			  (gdouble)pvals->tween_opacity_desc,   /* initial value */
			  (gdouble)0.0, (gdouble)100,           /* lower, upper */
			  1, 10,                                /* step, page */
			  1,                                    /* digits */
			  FALSE,                                /* constrain */
			  (gdouble)0.0, (gdouble)100,           /* lower, upper (unconstrained) */
			  _("Descending Opacity of the Tweenlayer"),
			  NULL);    /* tooltip privatetip */
  g_signal_connect (G_OBJECT (adj), "value_changed",
		    G_CALLBACK (gimp_double_adjustment_update),
		    &pvals->tween_opacity_desc);
  mgp->tween_opacity_desc_adj = GTK_ADJUSTMENT(adj);

  /* set initial sensitivity */
  mov_tweenlayer_sensitivity(mgp);
  mov_tracelayer_sensitivity(mgp);
  
  gtk_widget_show( table );
  
#ifndef MOVE_PATH_FRAMES_ENABLE
  /* copile without MOVE_PATH_LAYOUT_frame needs less space */  
  return table;
#else
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show(frame);  
  return frame;
#endif
}	/* end mov_advanced_tab_create */

/* ============================================================================
 * Create new EditCtrlPoint Frame
 * ============================================================================
 */
static GtkWidget *
mov_edit_button_box_create (t_mov_gui_stuff *mgp)
{
  GtkWidget	 *vbox;
  GtkWidget	 *hbox;
  GtkWidget	 *frame;
  GtkWidget      *button_table;
  GtkWidget      *button;
  gint           row;

  /* the vbox */
  vbox = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (vbox);

  /* the frame */
  frame = gtk_frame_new (_("Edit Controlpoints"));
  gtk_frame_set_shadow_type( GTK_FRAME( frame ) ,GTK_SHADOW_ETCHED_IN );
  gtk_container_set_border_width( GTK_CONTAINER( frame ), 2 );
  

  /* button_table 7 rows */
  button_table = gtk_table_new (7, 2, TRUE);
  gtk_table_set_row_spacings (GTK_TABLE (button_table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (button_table), 2);
  gtk_widget_show (button_table);

  row = 0;

  /* Add Controlpoint */
  button = gtk_button_new_from_stock ( GAP_STOCK_ADD_POINT );
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_table_attach( GTK_TABLE(button_table), button, 0, 1, row, row+1,
		    GTK_FILL, 0, 0, 0 );
  gimp_help_set_help_data(button,
                       _("Add Controlpoint at end\n(the last Point is duplicated)")
                       , NULL);
  gtk_widget_show (button);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK  (mov_padd_callback),
		    mgp);

  /* Grab Path (Controlpoints from current GIMP Path)  */
  //XX button = .... 
  //XX  gtk_table_attach(GTK_TABLE(button_table), button, 1, 2, row, row+1,
  //XX                   0, 0, 0, 0);
  row++;

  /* Insert Controlpoint */
  button = gtk_button_new_from_stock ( GAP_STOCK_INSERT_POINT );
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_table_attach( GTK_TABLE(button_table), button, 0, 1, row, row+1,
		    GTK_FILL, 0, 0, 0 );
  gimp_help_set_help_data(button,
                       _("Insert Controlpoint\n(the current Point is duplicated)")
                       , NULL);
  gtk_widget_show (button);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (mov_pins_callback),
		    mgp);

  /* Delete Controlpoint */
  button = gtk_button_new_from_stock ( GAP_STOCK_DELETE_POINT );
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_table_attach( GTK_TABLE(button_table), button, 1, 2, row, row+1,
		    GTK_FILL, 0, 0, 0 );
  gimp_help_set_help_data(button,
                       _("Delete current Controlpoint")
                       , NULL);
  gtk_widget_show (button);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (mov_pdel_callback),
		    mgp);

  row++;

  /* Previous Controlpoint */
  button = gtk_button_new_from_stock ( GAP_STOCK_PREV_POINT );
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_table_attach( GTK_TABLE(button_table), button, 0, 1, row, row+1,
		    GTK_FILL, 0, 0, 0 );
  gimp_help_set_help_data(button,
                       _("Show Previous Controlpoint")
                       , NULL);
  gtk_widget_show (button);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (mov_pprev_callback),
		    mgp);

  /* Next Controlpoint */
  button = gtk_button_new_from_stock ( GAP_STOCK_NEXT_POINT );
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_table_attach( GTK_TABLE(button_table), button, 1, 2, row, row+1,
		    GTK_FILL, 0, 0, 0 );
  gimp_help_set_help_data(button,
                       _("Show Next Controlpoint")
                       , NULL);
  gtk_widget_show (button);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (mov_pnext_callback),
		    mgp);

  row++;

  /* First Controlpoint */
  button = gtk_button_new_from_stock ( GAP_STOCK_FIRST_POINT );
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_table_attach( GTK_TABLE(button_table), button, 0, 1, row, row+1,
		    GTK_FILL, 0, 0, 0 );
  gimp_help_set_help_data(button,
                       _("Show First Controlpoint")
                       , NULL);
  gtk_widget_show (button);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (mov_pfirst_callback),
		    mgp);

  /* Last Controlpoint */
  button = gtk_button_new_from_stock ( GAP_STOCK_LAST_POINT );
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_table_attach( GTK_TABLE(button_table), button, 1, 2, row, row+1,
		    GTK_FILL, 0, 0, 0 );
  gimp_help_set_help_data(button,
                       _("Show Last Controlpoint")
                       , NULL);
  gtk_widget_show (button);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (mov_plast_callback),
		    mgp);

  row++;

  /* Reset Controlpoint */
  button = gtk_button_new_from_stock ( GAP_STOCK_RESET_POINT );
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_table_attach( GTK_TABLE(button_table), button, 0, 1, row, row+1,
		    GTK_FILL, 0, 0, 0 );
  gimp_help_set_help_data(button,
                       _("Reset the current Controlpoint to default Values")
                       , NULL);
  gtk_widget_show (button);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (mov_pclr_callback),
		    mgp);

  /* Reset All Controlpoints */
  button = gtk_button_new_from_stock ( GAP_STOCK_RESET_ALL_POINTS );
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_table_attach( GTK_TABLE(button_table), button, 1, 2, row, row+1,
		    GTK_FILL, 0, 0, 0 );
  gimp_help_set_help_data(button,
                       _("Reset all Controlpoints to default Values "
		         "but dont change the path (X/Y Values)")
                       , NULL);
  gtk_widget_show (button);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (mov_pclr_all_callback),
		    mgp);

  row++;

  /* Rotate Follow */
  button = gtk_button_new_from_stock ( GAP_STOCK_ROTATE_FOLLOW );
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_table_attach( GTK_TABLE(button_table), button, 0, 1, row, row+1,
		    GTK_FILL, 0, 0, 0 );
  gimp_help_set_help_data(button,
                       _("Set Rotation for all Controlpoints "
		         "to follow the shape of the path.\n"
		         "(Shift: use Rotation of contolpoint 1 as offset)")
                       , NULL);
  gtk_widget_show (button);
  g_signal_connect (G_OBJECT (button), "button_press_event",
		    G_CALLBACK (mov_prot_follow_callback),
		    mgp);

  /* Delete All Controlpoints */
  button = gtk_button_new_from_stock ( GAP_STOCK_DELETE_ALL_POINTS );
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_table_attach( GTK_TABLE(button_table), button, 1, 2, row, row+1,
		    GTK_FILL, 0, 0, 0 );
  gimp_help_set_help_data(button,
                       _("Delete all Controlpoints")
                       , NULL);
  gtk_widget_show (button);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (mov_pdel_all_callback),
		    mgp);

  
  row++;

  /* Load Controlpoints */
  button = gtk_button_new_from_stock (GTK_STOCK_OPEN );
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_table_attach( GTK_TABLE(button_table), button, 0, 1, row, row+1,
		    GTK_FILL, 0, 0, 0 );
  gimp_help_set_help_data(button,
                       _("Load Controlpoints from file")
                       , NULL);
  gtk_widget_show (button);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (mov_pload_callback),
		    mgp);

  /* Save Controlpoints */
  button = gtk_button_new_from_stock ( GTK_STOCK_SAVE );
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_table_attach( GTK_TABLE(button_table), button, 1, 2, row, row+1,
		    GTK_FILL, 0, 0, 0 );
  gimp_help_set_help_data(button,
                       _("Save Controlpoints to file")
                       , NULL);
  gtk_widget_show (button);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (mov_psave_callback),
		    mgp);

  row++;

  gtk_widget_show(button_table);
  gtk_widget_show(frame);
 
  /* the hbox */
  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);

  gtk_box_pack_start (GTK_BOX (hbox), button_table, FALSE, FALSE, 4);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 4);
  return vbox;
}  /* end mov_edit_button_box_create */

/* ============================================================================
 * Create set of widgets for the framerange and layerstack widgets
 * ============================================================================
 * vertical_layout == FALSE
 *   +-master_table-------------------------+----------------------+
 *   |  +-----------+--------+------------+ | +-vcbox-----------+  |
 *   |  | FromFrame | scale  | spinbutton | | |                 |  |
 *   |  +-----------+--------+------------+ | | ForceVisibility |  |
 *   |  | ToFrame   | scale  | spinbutton | | |                 |  |
 *   |  +-----------+--------+------------+ | | Clip To Frame   |  |
 *   |  | Layerstack| scale  | spinbutton | | |                 |  |
 *   |  +-----------+--------+------------+ | +-----------------+  |
 *   +--------------------------------------+----------------------+
 *
 * vertical_layout == TRUE
 *
 *   +-master_table-------------------------+
 *   |  +-vcbox---------------------------+ |
 *   |  | ForceVisibility                 | |
 *   |  | Clip To Frame                   | |
 *   |  +---------------------------------+ |
 *   +--------------------------------------+
 *   |  +-----------+--------+------------+ +
 *   |  | FromFrame | scale  | spinbutton | |
 *   |  +-----------+--------+------------+ |
 *   |  | ToFrame   | scale  | spinbutton | |
 *   |  +-----------+--------+------------+ |
 *   |  | Layerstack| scale  | spinbutton | |
 *   |  +-----------+--------+------------+ |
 *   +--------------------------------------+
 */
GtkWidget *
mov_path_framerange_box_create(t_mov_gui_stuff *mgp
			      ,gboolean vertical_layout
			      )
{
  GtkWidget *vcbox;
  GtkWidget *master_table;
  GtkWidget *table;
  GtkObject *adj;
  GtkWidget *check_button;
  gint  master_rows;
  gint  master_cols;
  gint  tabcol, tabrow, boxcol, boxrow;
  
  if(vertical_layout)
  {
    master_rows = 2;
    master_cols = 1;
    tabcol = 0;
    tabrow = 1;
    boxcol = 0;
    boxrow = 0;
  }
  else
  {
    master_rows = 1;
    master_cols = 2;
    tabcol = 0;
    tabrow = 0;
    boxcol = 1;
    boxrow = 0;
  }
  
  /* the master_table (1 row) */
  master_table = gtk_table_new (master_rows, master_cols, FALSE);
  gtk_widget_show (master_table);
  
  /* table with 3 rows */
  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_attach(GTK_TABLE(master_table), table, tabcol, tabcol+1, tabrow, tabrow+1
                  , GTK_FILL|GTK_EXPAND, GTK_FILL, 4, 0);
  gtk_widget_show (table);

  /* the start frame scale_entry */
  adj = gimp_scale_entry_new( GTK_TABLE (table), 0, 0,          /* table col, row */
			  _("From Frame:"),                     /* label text */
			  SCALE_WIDTH, ENTRY_WIDTH,             /* scalesize spinsize */
			  (gdouble)pvals->dst_range_start,      /* value */
			  (gdouble)mgp->first_nr,               /* lower */
			  (gdouble)mgp->last_nr,                /* upper */
			  1, 10,                                /* step, page */
			  0,                                    /* digits */
			  TRUE,                                 /* constrain */
			  (gdouble)mgp->first_nr,               /* lower, (unconstrained) */
			  (gdouble)mgp->last_nr,                /* upper (unconstrained) */
			  _("First handled frame"), NULL);      /* tooltip privatetip */
  g_object_set_data(G_OBJECT(adj), "mgp", mgp);
  g_signal_connect (G_OBJECT (adj), "value_changed",
		    G_CALLBACK (gimp_int_adjustment_update),
		    &pvals->dst_range_start);

  /* the end frame scale_entry */
  adj = gimp_scale_entry_new( GTK_TABLE (table), 0, 1,          /* table col, row */
			  _("To Frame:"),                       /* label text */
			  SCALE_WIDTH, ENTRY_WIDTH,             /* scalesize spinsize */
			  (gdouble)pvals->dst_range_end,        /* value */
			  (gdouble)mgp->first_nr,               /* lower */
			  (gdouble)mgp->last_nr,                /* upper */
			  1, 10,                                /* step, page */
			  0,                                    /* digits */
			  TRUE,                                 /* constrain */
			  (gdouble)mgp->first_nr,               /* lower, (unconstrained) */
			  (gdouble)mgp->last_nr,                /* upper (unconstrained) */
			  _("Last handled frame"), NULL);       /* tooltip privatetip */
  g_object_set_data(G_OBJECT(adj), "mgp", mgp);
  g_signal_connect (G_OBJECT (adj), "value_changed",
		    G_CALLBACK (gimp_int_adjustment_update),
		    &pvals->dst_range_end);
			  
  /* the Layerstack scale_entry */
  adj = gimp_scale_entry_new( GTK_TABLE (table), 0, 2,          /* table col, row */
			  _("Layerstack:"),                     /* label text */
			  SCALE_WIDTH, ENTRY_WIDTH,             /* scalesize spinsize */
			  (gdouble)pvals->dst_layerstack,       /* value */
			  0.0, 99.0,                            /* lower, upper */
			  1, 10,                                /* step, page */
			  0,                                    /* digits */
			  FALSE,                                /* constrain */
			  0.0, 999999.0,                        /* lower, upper (unconstrained) */
			  _("How to insert SrcLayer into the Dst. Frame's Layerstack\n0 means on top i.e. in front"),
			  NULL);                              /* tooltip privatetip */
  g_object_set_data(G_OBJECT(adj), "mgp", mgp);
  g_signal_connect (G_OBJECT (adj), "value_changed",
		    G_CALLBACK (mov_instant_int_adjustment_update),
		    &pvals->dst_layerstack);

  /* the vbox for checkbuttons */
  vcbox = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (vcbox);


  /* toggle force visibility  */
  check_button = gtk_check_button_new_with_label ( _("Force visibility"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button),
				pvals->src_force_visible);
  gimp_help_set_help_data(check_button,
                       _("Force visibility for all copied Src-Layers")
                       , NULL);
  gtk_widget_show (check_button);
  g_object_set_data(G_OBJECT(check_button), "mgp", mgp);
  g_signal_connect (G_OBJECT (check_button), "toggled",
                    G_CALLBACK  (mov_force_visibility_toggle_callback),
                    &pvals->src_force_visible);
  gtk_box_pack_start (GTK_BOX (vcbox), check_button, TRUE, TRUE, 0);

  /* toggle clip_to_image */
  check_button = gtk_check_button_new_with_label ( _("Clip To Frame"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button),
				pvals->clip_to_img);
  gimp_help_set_help_data(check_button,
                       _("Clip all copied Src-Layers at Frame Boundaries")
                       , NULL);
  gtk_widget_show (check_button);
  g_signal_connect (G_OBJECT (check_button), "toggled",
                    G_CALLBACK (mov_gint_toggle_callback),
                    &pvals->clip_to_img);
  gtk_box_pack_start (GTK_BOX (vcbox), check_button, TRUE, TRUE, 0);

  gtk_table_attach(GTK_TABLE(master_table), vcbox, boxcol, boxcol+1, boxrow, boxrow+1
                  , GTK_FILL, GTK_FILL, 4, 0);

  return(master_table);
}  /* end mov_path_framerange_box_create */



/* ============================================================================
 * Create  VBox with 
 *   The VBox contains 
 *   - Resize    2x spinbutton   (for resizing Width + Height in percent)
 *                  chainbutton  (for constrain both resize widgets)
 *   - Opacity      spinbutton   (0.0 upto 100.0 %)
 *   - Rotation     spinbutton   (-360.0 to 360.0 degrees)
 * ============================================================================
 */
static GtkWidget*
mov_modify_tab_create(t_mov_gui_stuff *mgp)
{
  GtkWidget *frame;
  GtkWidget      *vbox;
  GtkWidget      *table;
  GtkObject      *adj;

  /* the frame */
  frame = gtk_frame_new ( _("Modify Size Opacity and Rotation") );
  gtk_frame_set_shadow_type( GTK_FRAME( frame ) ,GTK_SHADOW_ETCHED_IN );

  /* the vbox */
  vbox = gtk_vbox_new (FALSE, 3);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);

  /* the table (2 rows) */
  table = gtk_table_new ( 2, 6, FALSE );
  gtk_container_set_border_width (GTK_CONTAINER (table), 2 );
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);


  /* Width Scale */
  adj = p_mov_spinbutton_new( GTK_TABLE (table), 0, 0,        /* table col, row */
			  _("Width:"),                        /* label text */
			  SCALE_WIDTH, ENTRY_WIDTH,           /* scalesize spinsize */
			  (gdouble)mgp->w_resize,             /* value */
			  (gdouble)1, (gdouble)200,           /* lower, upper */
			  1, 10,                              /* step, page */
			  1,                                  /* digits */
			  FALSE,                              /* constrain */
			  (gdouble)1, (gdouble)1000,          /* lower, upper (unconstrained) */
			  _("Scale Source Layer's Width in percent"),
			  NULL);    /* tooltip privatetip */
  g_object_set_data(G_OBJECT(adj), "mgp", mgp);
  g_signal_connect (G_OBJECT (adj), "value_changed",
		    G_CALLBACK (mov_path_resize_adjustment_update),
		    &mgp->w_resize);
  mgp->wres_adj = GTK_ADJUSTMENT(adj);

  /* Height Scale */		      
  adj = p_mov_spinbutton_new( GTK_TABLE (table), 0, 1,        /* table col, row */
			  _("Height:"),                       /* label text */
			  SCALE_WIDTH, ENTRY_WIDTH,           /* scalesize spinsize */
			  (gdouble)mgp->h_resize,             /* value */
			  (gdouble)1, (gdouble)200,           /* lower, upper */
			  1, 10,                              /* step, page */
			  1,                                  /* digits */
			  FALSE,                              /* constrain */
			  (gdouble)1, (gdouble)1000,          /* lower, upper (unconstrained) */
			  _("Scale SrcLayer's Height in percent"),
			  NULL);    /* tooltip privatetip */
  g_object_set_data(G_OBJECT(adj), "mgp", mgp);
  g_signal_connect (G_OBJECT (adj), "value_changed",
		    G_CALLBACK (mov_path_resize_adjustment_update),
		    &mgp->h_resize);
  mgp->hres_adj = GTK_ADJUSTMENT(adj);


  /*  the constrain ratio chainbutton  */
  mgp->constrain = gimp_chain_button_new (GIMP_CHAIN_RIGHT);
  gimp_chain_button_set_active (GIMP_CHAIN_BUTTON (mgp->constrain), TRUE);
  gtk_table_attach (GTK_TABLE (table), mgp->constrain, 2, 3, 0, 2
                   , 0,0,0,0);
  gtk_widget_show (mgp->constrain);

  gimp_help_set_help_data (GIMP_CHAIN_BUTTON (mgp->constrain)->button,
                           _("Constrain aspect ratio"), NULL);
  
  /* the state of the contrain ratio chainbutton is checked in other callbacks (where needed)
   * there is no need for the chainbutton to have its own callback procedure
   */

		      
  /* Opacity */
  adj = gimp_scale_entry_new( GTK_TABLE (table), 3, 0,        /* table col, row */
			  _("Opacity:"),                      /* label text */
			  SCALE_WIDTH, ENTRY_WIDTH,           /* scalesize spinsize */
			  (gdouble)mgp->opacity,              /* value */
			  (gdouble)0, (gdouble)100,           /* lower, upper */
			  1, 10,                              /* step, page */
			  1,                                  /* digits */
			  TRUE,                               /* constrain */
			  (gdouble)0, (gdouble)100,           /* lower, upper (unconstrained) */
			  _("SrcLayer's Opacity in percent"),
			  NULL);    /* tooltip privatetip */
  g_object_set_data(G_OBJECT(adj), "mgp", mgp);
  g_signal_connect (G_OBJECT (adj), "value_changed",
		    G_CALLBACK (mov_instant_double_adjustment_update),
		    &mgp->opacity);
  mgp->opacity_adj = GTK_ADJUSTMENT(adj);
		      
  /* Rotation */ 
  adj = gimp_scale_entry_new( GTK_TABLE (table), 3, 1,        /* table col, row */
			  _("Rotate:"),                       /* label text */
			  SCALE_WIDTH, ENTRY_WIDTH,           /* scalesize spinsize */
			  (gdouble)mgp->rotation,             /* value */
			  (gdouble)-360, (gdouble)360,        /* lower, upper */
			  1, 45,                              /* step, page */
			  1,                                  /* digits */
			  FALSE,                              /* constrain */
			  (gdouble)-3600, (gdouble)3600,      /* lower, upper (unconstrained) */
			  _("Rotate SrcLayer (in degree)"),
			  NULL);    /* tooltip privatetip */
  g_object_set_data(G_OBJECT(adj), "mgp", mgp);
  g_signal_connect (G_OBJECT (adj), "value_changed",
		    G_CALLBACK (mov_instant_double_adjustment_update),
		    &mgp->rotation);
  mgp->rotation_adj = GTK_ADJUSTMENT(adj);


  gtk_widget_show (table);
  gtk_widget_show (vbox);
  
#ifndef MOVE_PATH_FRAMES_ENABLE
  /* copile without MOVE_PATH_LAYOUT_frame needs less space */  
  return vbox;
#else
  gtk_container_set_border_width( GTK_CONTAINER( frame ), 2 );
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show(frame);
  return frame;
#endif  

}  /* end mov_modify_tab_create */


/* ============================================================================
 * Create  VBox with the 8 transformation factors and return it.
 *   The VBox contains 
 *   - Transform 8x spinbutton   (0.01 upto 10.0) 4-point perspective transformation
 * ============================================================================
 */
static GtkWidget *
mov_trans_tab_create (t_mov_gui_stuff *mgp)
{
  GtkWidget	 *frame;
  GtkWidget	 *vbox;
  GtkWidget	 *table;
  GtkObject      *adj;


  /* the frame */
  frame = gtk_frame_new ( _("Transformfactors X/Y for the 4 corners") );
  gtk_frame_set_shadow_type( GTK_FRAME( frame ) ,GTK_SHADOW_ETCHED_IN );
  gtk_container_set_border_width( GTK_CONTAINER( frame ), 2 );
  

  /* the vbox */
  vbox = gtk_vbox_new (FALSE, 3);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);

  /* the table (2 rows) */
  table = gtk_table_new ( 2, 8, FALSE );
  gtk_container_set_border_width (GTK_CONTAINER (table), 2 );
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);


  /* ttlx transformfactor */
  adj = p_mov_spinbutton_new( GTK_TABLE (table), 0, 0,        /* table col, row */
			  _("x1:"),                           /* label text */
			  SCALE_WIDTH, ENTRY_WIDTH,           /* scalesize spinsize */
			  (gdouble)mgp->ttlx,                 /* initial value */
			  (gdouble)0, (gdouble)10,            /* lower, upper */
			  0.1, 1,                             /* step, page */
			  3,                                  /* digits */
			  FALSE,                              /* constrain */
			  (gdouble)0, (gdouble)10,            /* lower, upper (unconstrained) */
			  _("Transformfactor for upper left corner X coordinate"),
			  NULL);    /* tooltip privatetip */
  g_object_set_data(G_OBJECT(adj), "mgp", mgp);
  g_signal_connect (G_OBJECT (adj), "value_changed",
		    G_CALLBACK (mov_path_tfactor_adjustment_update),
		    &mgp->ttlx);
  mgp->ttlx_adj = GTK_ADJUSTMENT(adj);


  /* ttly transformfactor */
  adj = p_mov_spinbutton_new( GTK_TABLE (table), 2, 0,        /* table col, row */
			  _("y1:"),                           /* label text */
			  SCALE_WIDTH, ENTRY_WIDTH,           /* scalesize spinsize */
			  (gdouble)mgp->ttly,                 /* initial value */
			  (gdouble)0, (gdouble)10,            /* lower, upper */
			  0.1, 1,                             /* step, page */
			  3,                                  /* digits */
			  FALSE,                              /* constrain */
			  (gdouble)0, (gdouble)10,            /* lower, upper (unconstrained) */
			  _("Transformfactor for upper left corner Y coordinate"),
			  NULL);    /* tooltip privatetip */
  g_object_set_data(G_OBJECT(adj), "mgp", mgp);
  g_signal_connect (G_OBJECT (adj), "value_changed",
		    G_CALLBACK (mov_path_tfactor_adjustment_update),
		    &mgp->ttly);
  mgp->ttly_adj = GTK_ADJUSTMENT(adj);


  /* ttrx transformfactor */
  adj = p_mov_spinbutton_new( GTK_TABLE (table), 4, 0,        /* table col, row */
			  _("x2:"),                           /* label text */
			  SCALE_WIDTH, ENTRY_WIDTH,           /* scalesize spinsize */
			  (gdouble)mgp->ttrx,                 /* initial value */
			  (gdouble)0, (gdouble)10,            /* lower, upper */
			  0.1, 1,                             /* step, page */
			  3,                                  /* digits */
			  FALSE,                              /* constrain */
			  (gdouble)0, (gdouble)10,            /* lower, upper (unconstrained) */
			  _("Transformfactor for upper right corner X coordinate"),
			  NULL);    /* tooltip privatetip */
  g_object_set_data(G_OBJECT(adj), "mgp", mgp);
  g_signal_connect (G_OBJECT (adj), "value_changed",
		    G_CALLBACK (mov_path_tfactor_adjustment_update),
		    &mgp->ttrx);
  mgp->ttrx_adj = GTK_ADJUSTMENT(adj);

  /* ttry transformfactor */
  adj = p_mov_spinbutton_new( GTK_TABLE (table), 6, 0,        /* table col, row */
			  _("y2:"),                           /* label text */
			  SCALE_WIDTH, ENTRY_WIDTH,           /* scalesize spinsize */
			  (gdouble)mgp->ttry,                 /* initial value */
			  (gdouble)0, (gdouble)10,            /* lower, upper */
			  0.1, 1,                             /* step, page */
			  3,                                  /* digits */
			  FALSE,                              /* constrain */
			  (gdouble)0, (gdouble)10,            /* lower, upper (unconstrained) */
			  _("Transformfactor for upper right corner Y coordinate"),
			  NULL);    /* tooltip privatetip */
  g_object_set_data(G_OBJECT(adj), "mgp", mgp);
  g_signal_connect (G_OBJECT (adj), "value_changed",
		    G_CALLBACK (mov_path_tfactor_adjustment_update),
		    &mgp->ttry);
  mgp->ttry_adj = GTK_ADJUSTMENT(adj);

  /* tblx transformfactor */
  adj = p_mov_spinbutton_new( GTK_TABLE (table), 0, 1,        /* table col, row */
			  _("x3:"),                           /* label text */
			  SCALE_WIDTH, ENTRY_WIDTH,           /* scalesize spinsize */
			  (gdouble)mgp->tblx,                 /* initial value */
			  (gdouble)0, (gdouble)10,            /* lower, upper */
			  0.1, 1,                             /* step, page */
			  3,                                  /* digits */
			  FALSE,                              /* constrain */
			  (gdouble)0, (gdouble)10,            /* lower, upper (unconstrained) */
			  _("Transformfactor for lower left corner X coordinate"),
			  NULL);    /* tooltip privatetip */
  g_object_set_data(G_OBJECT(adj), "mgp", mgp);
  g_signal_connect (G_OBJECT (adj), "value_changed",
		    G_CALLBACK (mov_path_tfactor_adjustment_update),
		    &mgp->tblx);
  mgp->tblx_adj = GTK_ADJUSTMENT(adj);

  /* tbly transformfactor */
  adj = p_mov_spinbutton_new( GTK_TABLE (table), 2, 1,        /* table col, row */
			  _("y3:"),                           /* label text */
			  SCALE_WIDTH, ENTRY_WIDTH,           /* scalesize spinsize */
			  (gdouble)mgp->tbly,                 /* initial value */
			  (gdouble)0, (gdouble)10,            /* lower, upper */
			  0.1, 1,                             /* step, page */
			  3,                                  /* digits */
			  FALSE,                              /* constrain */
			  (gdouble)0, (gdouble)10,            /* lower, upper (unconstrained) */
			  _("Transformfactor for lower left corner Y coordinate"),
			  NULL);    /* tooltip privatetip */
  g_object_set_data(G_OBJECT(adj), "mgp", mgp);
  g_signal_connect (G_OBJECT (adj), "value_changed",
		    G_CALLBACK (mov_path_tfactor_adjustment_update),
		    &mgp->tbly);
  mgp->tbly_adj = GTK_ADJUSTMENT(adj);

  /* tbrx transformfactor */
  adj = p_mov_spinbutton_new( GTK_TABLE (table), 4, 1,        /* table col, row */
			  _("x4:"),                           /* label text */
			  SCALE_WIDTH, ENTRY_WIDTH,           /* scalesize spinsize */
			  (gdouble)mgp->tbrx,                 /* initial value */
			  (gdouble)0, (gdouble)10,            /* lower, upper */
			  0.1, 1,                             /* step, page */
			  3,                                  /* digits */
			  FALSE,                              /* constrain */
			  (gdouble)0, (gdouble)10,            /* lower, upper (unconstrained) */
			  _("Transformfactor for lower right corner X coordinate"),
			  NULL);    /* tooltip privatetip */
  g_object_set_data(G_OBJECT(adj), "mgp", mgp);
  g_signal_connect (G_OBJECT (adj), "value_changed",
		    G_CALLBACK (mov_path_tfactor_adjustment_update),
		    &mgp->tbrx);
  mgp->tbrx_adj = GTK_ADJUSTMENT(adj);

  /* tbry transformfactor */
  adj = p_mov_spinbutton_new( GTK_TABLE (table), 6, 1,        /* table col, row */
			  _("y3:"),                           /* label text */
			  SCALE_WIDTH, ENTRY_WIDTH,           /* scalesize spinsize */
			  (gdouble)mgp->tbry,                 /* initial value */
			  (gdouble)0, (gdouble)10,            /* lower, upper */
			  0.1, 1,                             /* step, page */
			  3,                                  /* digits */
			  FALSE,                              /* constrain */
			  (gdouble)0, (gdouble)10,            /* lower, upper (unconstrained) */
			  _("Transformfactor for lower right corner Y coordinate"),
			  NULL);    /* tooltip privatetip */
  g_object_set_data(G_OBJECT(adj), "mgp", mgp);
  g_signal_connect (G_OBJECT (adj), "value_changed",
		    G_CALLBACK (mov_path_tfactor_adjustment_update),
		    &mgp->tbry);
  mgp->tbry_adj = GTK_ADJUSTMENT(adj);

  gtk_widget_show(table);
  gtk_widget_show(vbox);

#ifndef MOVE_PATH_FRAMES_ENABLE
  /* copile without MOVE_PATH_LAYOUT_frame needs less space */  
  return vbox;
#else
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show(frame);
  return frame;
#endif  
}  /* end mov_trans_tab_create */


/* ============================================================================
 * Create all Widget Blocks that represent 
 *   - The current contolpoint settings
 *   - The Preview
 *   - The Buttonbox for Editing Controlpoints
 *   and attach those Blocks to the mgp->master_vbox Widget.
 *
 *   One "ControlPoint" has:
 *   - 2 spinbuttons X/Y, used for positioning
 *   - Keyframe     spinbutton integer (0 to max_frame)
 *   - Notebook  with following sub tables:
 *      - transform SubTable  4-point perspective transformation
 *      - modify    SubTable  for Resize(Scaling), Opacity and Rotation
 * ============================================================================
 */
static void
mov_path_prevw_create ( GimpDrawable *drawable, t_mov_gui_stuff *mgp, gboolean vertical_layout)
{
  GtkWidget	 *cpt_frame;
  GtkWidget	 *pv_frame;
  GtkWidget	 *vbox;
  GtkWidget	 *hbox;
  GtkWidget      *notebook;
  GtkWidget	 *table;
  GtkWidget	 *label;
  GtkWidget	 *aspect_frame;
  GtkWidget	 *da_widget;
  GtkWidget      *pv_table;
  GtkWidget      *pv_sub_table;
  GtkWidget      *check_button;
  GtkObject      *adj;
  GtkWidget	 *framerange_table;
  GtkWidget      *edit_buttons;


  mgp->drawable = drawable;
  mgp->dwidth   = gimp_drawable_width(drawable->drawable_id );
  mgp->dheight  = gimp_drawable_height(drawable->drawable_id );
  mgp->bpp	= gimp_drawable_bpp(drawable->drawable_id);
  if ( gimp_drawable_has_alpha(drawable->drawable_id) )
    mgp->bpp--;
  mgp->curx = 0;
  mgp->cury = 0;
  mgp->in_call = TRUE;  /* to avoid side effects while initialization */


  /* the vbox */
  vbox = gtk_vbox_new (FALSE, 3);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_widget_show (vbox);
  gtk_box_pack_start (GTK_BOX (mgp->master_vbox), vbox, TRUE, TRUE, 0);

  /* the cpt_frame */
  cpt_frame = gtk_frame_new (" ");  /* text "Current Point: [ %3d ] of [ %3d ]"
                                 * is set later in procedure p_update_point_index_text
                                 */
  gtk_frame_set_shadow_type( GTK_FRAME( cpt_frame ) ,GTK_SHADOW_ETCHED_IN );
  gtk_container_set_border_width( GTK_CONTAINER( cpt_frame ), 2 );
  mgp->point_index_frame = cpt_frame;
  p_update_point_index_text(mgp);

  gtk_box_pack_start (GTK_BOX (vbox), cpt_frame, TRUE, TRUE, 0);
  gtk_widget_show( mgp->master_vbox );
  gtk_widget_show( cpt_frame );
  


  /* the table (3 rows) for other controlpoint specific settings */
  table = gtk_table_new ( 3, 4, FALSE );
  gtk_container_set_border_width (GTK_CONTAINER (table), 2 );
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_container_add (GTK_CONTAINER (cpt_frame), table);


  /* X */
  adj = gimp_scale_entry_new( GTK_TABLE (table), 0, 0,            /* table col, row */
			  _("X:"),                                /* label text */
			  SCALE_WIDTH, SPINBUTTON_WIDTH,          /* scalesize spinsize */
			  (gdouble)mgp->p_x,                      /* value */
			  (gdouble)0, (gdouble)mgp->dwidth,       /* lower, upper */
			  1, 10,                                  /* step, page */
			  0,                                      /* digits */
			  FALSE,                                  /* constrain */
			  (gdouble)(-GIMP_MAX_IMAGE_SIZE),
			  (gdouble)GIMP_MAX_IMAGE_SIZE,           /* lower, upper (unconstrained) */
			  _("X Coordinate"),
			  NULL);    /* tooltip privatetip */
  g_signal_connect (G_OBJECT (adj), "value_changed",
		    G_CALLBACK (mov_path_x_adjustment_update),
		    mgp);
  mgp->x_adj = GTK_ADJUSTMENT(adj);

  /* Y */
  adj = gimp_scale_entry_new( GTK_TABLE (table), 0, 1,            /* table col, row */
			  _("Y:"),                                /* label text */
			  SCALE_WIDTH, ENTRY_WIDTH,               /* scalesize spinsize */
			  (gdouble)mgp->p_y,                      /* value */
			  (gdouble)0, (gdouble)mgp->dheight,      /* lower, upper */
			  1, 10,                                  /* step, page */
			  0,                                      /* digits */
			  FALSE,                                  /* constrain */
			  (gdouble)(-GIMP_MAX_IMAGE_SIZE),
			  (gdouble)GIMP_MAX_IMAGE_SIZE,           /* lower, upper (unconstrained) */
			  _("Y Coordinate"),
			  NULL);    /* tooltip privatetip */
  g_signal_connect (G_OBJECT (adj), "value_changed",
		    G_CALLBACK (mov_path_y_adjustment_update),
		    mgp);
  mgp->y_adj = GTK_ADJUSTMENT(adj);
  
  /* Keyframe */ 
  adj = p_mov_spinbutton_new( GTK_TABLE (table), 1, 2,          /* table col, row */
			  _("Keyframe:"),                       /* label text */
			  SCALE_WIDTH, ENTRY_WIDTH,             /* scalesize spinsize */
			  (gdouble)mgp->keyframe_abs,           /* value */
			  (gdouble)0, (gdouble)mgp->max_frame,  /* lower, upper */
			  1, 10,                                /* step, page */
			  0,                                    /* digits */
			  TRUE,                                 /* constrain */
			  (gdouble)0, (gdouble)mgp->max_frame,  /* lower, upper (unconstrained) */
			  _("Fix Controlpoint to Keyframe number\n(0 == No Keyframe)"),
			  NULL);    /* tooltip privatetip */
  g_signal_connect (G_OBJECT (adj), "value_changed",
		    G_CALLBACK (gimp_int_adjustment_update),
		    &mgp->keyframe_abs);
  mgp->keyframe_adj = GTK_ADJUSTMENT(adj);
 

  /* the notebook */
  notebook = gtk_notebook_new();
  
  {
    GtkWidget *modify_table;
    GtkWidget *transform_table;

    /* set of modifier widgets for the current controlpoint */
    modify_table = mov_modify_tab_create(mgp);
    
    /* set of perspective transformation widgets for the current controlpoint */
    transform_table = mov_trans_tab_create(mgp);

    gtk_container_add (GTK_CONTAINER (notebook), modify_table);
    label = gtk_label_new(_("Scale and Modify"));
    gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook)
                             , gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), 0)
			     , label
			     );
    gtk_container_add (GTK_CONTAINER (notebook), transform_table);
    label = gtk_label_new(_("Perspective"));
    gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook)
                             , gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), 1)
			     , label
			     );
  }
  gtk_table_attach(GTK_TABLE(table), notebook, 3, 4          /* column */
                                             , 0, 3          /* all rows */
		                             , 0, 0, 0, 0);

  gtk_widget_show (notebook);
  gtk_widget_show( table );



  /* the hbox (for preview table and Edit Controlpoint Frame) */
  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /* the preview frame */
  pv_frame = gtk_frame_new ( _("Preview"));
  gtk_frame_set_shadow_type (GTK_FRAME (pv_frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_set_border_width (GTK_CONTAINER (pv_frame), 2);
  gtk_box_pack_start (GTK_BOX (hbox), pv_frame, TRUE, TRUE, 0);
  gtk_widget_show (pv_frame);

  /* the preview table (3 rows) */
  pv_table = gtk_table_new ( 3, 1, FALSE );
  gtk_container_set_border_width (GTK_CONTAINER (pv_table), 2 );
  gtk_table_set_row_spacings (GTK_TABLE (pv_table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (pv_table), 4);
  gtk_container_add (GTK_CONTAINER (pv_frame), pv_table);
  gtk_widget_show( pv_table );


  /*
   * Resize the greater one of dwidth and dheight to PREVIEW_SIZE
   */
  if ( mgp->dwidth > mgp->dheight ) 
  {
    mgp->pheight = mgp->dheight * PREVIEW_SIZE / mgp->dwidth;
    mgp->pwidth = PREVIEW_SIZE;
  } 
  else 
  {
    mgp->pwidth = mgp->dwidth * PREVIEW_SIZE / mgp->dheight;
    mgp->pheight = PREVIEW_SIZE;
  }

  /* aspect_frame is the CONTAINER for the preview drawing area */
  aspect_frame = gtk_aspect_frame_new (NULL   /* without label */
                                      , 0.5   /* xalign center */
                                      , 0.5   /* yalign center */
                                      , mgp->pwidth / mgp->pheight     /* ratio */
                                      , TRUE  /* obey_child */
                                      );
  gtk_table_attach( GTK_TABLE(pv_table), aspect_frame, 0, 1, 0, 1,
		    0, 0, 0, 0 );



  /* hbox_show block */
  {
    GtkWidget *hbox_show;
    GtkWidget *frame_show;
    
    frame_show = gtk_frame_new ( _("Show"));
    gtk_frame_set_shadow_type (GTK_FRAME (frame_show), GTK_SHADOW_ETCHED_IN);
   
    hbox_show = gtk_hbox_new (FALSE, 3);
    gtk_widget_show (hbox_show);

    /* toggle Show path */
    check_button = gtk_check_button_new_with_label ( _("Path"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button),
				  mgp->show_path);
    gimp_help_set_help_data(check_button,
                         _("Show Path Lines and enable "
                           "pick/drag with left button "
                           "or move with right button")
                         , NULL);
    gtk_widget_show (check_button);
    gtk_box_pack_start (GTK_BOX (hbox_show), check_button, TRUE, TRUE, 0);
    
    g_signal_connect (G_OBJECT (check_button), "toggled",
                      G_CALLBACK  (mov_show_path_callback),
                      mgp);

    /* toggle Show cursor */
    check_button = gtk_check_button_new_with_label ( _("Cursor"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button),
				  mgp->show_cursor);
    gimp_help_set_help_data(check_button,
                         _("Show Cursor Crosslines")
                         , NULL);
    gtk_widget_show (check_button);
    gtk_box_pack_start (GTK_BOX (hbox_show), check_button, TRUE, TRUE, 0);
    
    g_signal_connect (G_OBJECT (check_button), "toggled",
                      G_CALLBACK  (mov_show_cursor_callback),
                      mgp);


    /* toggle Show Grid */
    check_button = gtk_check_button_new_with_label ( _("Grid"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button),
				  mgp->show_grid);
    gimp_help_set_help_data(check_button,
                         _("Show Source Layer as Gridlines")
                         , NULL);
    //XX gtk_widget_show (check_button);
    gtk_box_pack_start (GTK_BOX (hbox_show), check_button, TRUE, TRUE, 0);
    
    g_signal_connect (G_OBJECT (check_button), "toggled",
                      G_CALLBACK  (mov_show_grid_callback),
                      mgp);

    /* toggle Instant Apply */
    check_button = gtk_check_button_new_with_label ( _("Instant Apply"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button),
				  mgp->instant_apply);
    gimp_help_set_help_data(check_button,
                         _("Update the Preview automatically")
                         , NULL);
    gtk_widget_show (check_button);
    gtk_box_pack_start (GTK_BOX (hbox_show), check_button, TRUE, TRUE, 0);
    
    g_signal_connect (G_OBJECT (check_button), "toggled",
                      G_CALLBACK  (mov_instant_apply_callback),
                      mgp);
   
#ifndef MOVE_PATH_FRAMES_ENABLE
    gtk_table_attach(GTK_TABLE(pv_table), hbox_show, 0, 1, 1, 2,
                     0, 0, 16, 0);
#else
    gtk_widget_show (frame_show);
    gtk_container_add (GTK_CONTAINER (frame_show), hbox_show);
    gtk_table_attach(GTK_TABLE(pv_table), frame_show, 0, 1, 1, 2,
                     0, 0, 12, 0);
#endif     
  }

  /* the preview sub table (1 row) */
  pv_sub_table = gtk_table_new ( 1, 3, FALSE );

  /* the Preview Frame Number  */
  adj = gimp_scale_entry_new( GTK_TABLE (pv_sub_table), 0, 1,   /* table col, row */
			  _("Frame:"),                          /* label text */
			  SCALE_WIDTH, ENTRY_WIDTH,             /* scalesize spinsize */
			  (gdouble)mgp->preview_frame_nr,       /* value */
			  (gdouble)mgp->first_nr,               /* lower */
			  (gdouble)mgp->last_nr,                /* upper */
			  1, 10,                                /* step, page */
			  0,                                    /* digits */
			  TRUE,                                 /* constrain */
			  (gdouble)mgp->first_nr,               /* lower (unconstrained)*/
			  (gdouble)mgp->last_nr,                /* upper (unconstrained)*/
			  _("Frame to show when UpdPreview button is pressed"),
			  NULL);                                /* tooltip privatetip */
  g_object_set_data(G_OBJECT(adj), "mgp", mgp);
  g_signal_connect (G_OBJECT (adj), "value_changed",
		    G_CALLBACK (mov_instant_int_adjustment_update),
		    &mgp->preview_frame_nr);


  gtk_table_attach( GTK_TABLE(pv_table), pv_sub_table, 0, 1, 2, 3,
		    GTK_FILL|GTK_EXPAND, 0, 0, 0 );
  gtk_widget_show (pv_sub_table);


  /* PREVIEW DRAWING AREA */
  mgp->pv_ptr = p_pview_new(mgp->pwidth
                                , mgp->pheight
                                , GAP_MOV_CHECK_SIZE
                                , aspect_frame
                                );
  da_widget = mgp->pv_ptr->da_widget;
  
  g_object_set_data( G_OBJECT(da_widget), "mgp", mgp);
  gtk_widget_set_events( GTK_WIDGET(da_widget), PREVIEW_MASK );
  g_signal_connect_after( G_OBJECT(da_widget), "expose_event",
		          G_CALLBACK (mov_path_prevw_preview_expose),
		          mgp );
  g_signal_connect( G_OBJECT(da_widget), "event",
		    G_CALLBACK  (mov_path_prevw_preview_events),
		    mgp );
  gtk_container_add( GTK_CONTAINER( aspect_frame ), da_widget);
  gtk_widget_show(da_widget);
  gtk_widget_show(aspect_frame);


  /* Draw the contents of preview, that is saved in the preview widget */
  mov_path_prevw_preview_init( mgp );


  /* edit buttons table */
  edit_buttons = mov_edit_button_box_create(mgp);
  if(vertical_layout)
  {
    GtkWidget *vvbox;

    vvbox = gtk_vbox_new(FALSE, 3);
    gtk_widget_show (vvbox);
    framerange_table = mov_path_framerange_box_create(mgp, vertical_layout);

    gtk_box_pack_start (GTK_BOX (vvbox), edit_buttons, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vvbox), framerange_table, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), vvbox, TRUE, TRUE, 0);
  }
  else
  {
    gtk_box_pack_start (GTK_BOX (hbox), edit_buttons, TRUE, TRUE, 0);
  }


  mov_path_prevw_cursor_update( mgp );

  mgp->in_call = FALSE;   /* End of initialization */
  if(gap_debug) printf("pvals mgp=%d,%d\n", mgp->p_x, mgp->p_y );
  if(gap_debug) printf("mgp cur=%d,%d\n", mgp->curx, mgp->cury );

}  /* end mov_path_prevw_create */


/* ============================================================================
 *  mov_path_prevw_preview_init
 *    Initialize preview
 *    Draw the contents into the internal buffer of the preview widget
 * ============================================================================
 */

static void
mov_path_prevw_preview_init ( t_mov_gui_stuff *mgp )
{
  gint32  image_id;


  if(gap_debug) printf ("mov_path_prevw_preview_init:  START\n");

  if(mgp->pv_ptr)
  {
    if(gap_debug) printf ("mov_path_prevw_preview_init: before p_pview_render_from_image\n");
    image_id = gimp_drawable_image_id(mgp->drawable->drawable_id);
    p_pview_render_from_image(mgp->pv_ptr, image_id);
  }
}

/* ============================================================================
 * mov_path_prevw_draw
 *   Preview Rendering Util routine End
 *     if update & PATH_LINE, draw the path lines
 *     if update & CURSOR,  draw cross cursor
 * ============================================================================
 */

static void
mov_path_prevw_draw ( t_mov_gui_stuff *mgp, gint update )
{
  gint     l_idx;
  GdkColor fg;
  GdkColormap *cmap;
  GimpRGB  foreground;
  guchar   l_red, l_green, l_blue;

  if(gap_debug) printf("mov_path_prevw_draw: START update:%d\n", (int)update);

  if(mgp->pv_ptr == NULL)
  {
    return;
  }
  if(mgp->pv_ptr->da_widget==NULL)
  {
    return;
  }

  if(gap_debug) printf("mov_path_prevw_draw: p_pview_repaint\n");
  p_pview_repaint(mgp->pv_ptr);

  /* alternate cross cursor OR path graph */

  if((mgp->show_path)
  && ( pvals != NULL )
  && (update & PATH_LINE))
  {
      /* redraw the preview
       * (to clear path lines and cross cursor)
       */
 
     
      gimp_palette_get_foreground (&foreground);
      gimp_rgb_get_uchar (&foreground, &l_red, &l_green, &l_blue);

      cmap = gtk_widget_get_colormap(mgp->pv_ptr->da_widget);
      fg.red   = (l_red   << 8) | l_red;
      fg.green = (l_green << 8) | l_green;
      fg.blue  = (l_blue  << 8) | l_blue;

      /*if(gap_debug) printf ("fg.r/g/b (%d %d %d)\n", (int)fg.red ,(int)fg.green, (int)fg.blue); */

      if(cmap)
      {
         gdk_colormap_alloc_color(cmap
                              , &fg
                              , FALSE   /* writeable */
                              , TRUE   /* best_match */
                              );
      }
      /*if(gap_debug) printf ("fg.pixel (%d)\n", (int)fg.pixel); */

        
      gdk_gc_set_foreground (mgp->pv_ptr->da_widget->style->black_gc, &fg);

      p_points_to_tab(mgp);
      for(l_idx = 0; l_idx < pvals->point_idx_max; l_idx++)
        {
           /* draw the path line(s) */
	   gdk_draw_line (mgp->pv_ptr->da_widget->window,
			  mgp->pv_ptr->da_widget->style->black_gc,
			  (pvals->point[l_idx].p_x    * mgp->pwidth) / mgp->dwidth,
			  (pvals->point[l_idx].p_y    * mgp->pheight) / mgp->dheight, 
			  (pvals->point[l_idx +1].p_x * mgp->pwidth) / mgp->dwidth,
			  (pvals->point[l_idx +1].p_y * mgp->pheight) / mgp->dheight
			  );
           /* draw the path point(s) */
	   gdk_draw_arc (mgp->pv_ptr->da_widget->window, mgp->pv_ptr->da_widget->style->black_gc, TRUE,
			    (pvals->point[l_idx +1].p_x  * mgp->pwidth / mgp->dwidth) -RADIUS,
			    (pvals->point[l_idx +1].p_y  * mgp->pheight / mgp->dheight) -RADIUS,
			    RADIUS * 2, RADIUS * 2, 0, 23040);
        }
        /* draw the start point */
	gdk_draw_arc (mgp->pv_ptr->da_widget->window, mgp->pv_ptr->da_widget->style->black_gc, TRUE,
		     (pvals->point[0].p_x * mgp->pwidth / mgp->dwidth) -RADIUS,
		     (pvals->point[0].p_y * mgp->pheight / mgp->dheight) -RADIUS,
		     RADIUS * 2, RADIUS * 2, 0, 23040);

        /* restore black gc */
        fg.red   = 0;
        fg.green = 0;
        fg.blue  = 0;
        if(cmap)
        {
          gdk_colormap_alloc_color(cmap
                                , &fg
                                , FALSE   /* writeable */
                                , TRUE   /* best_match */
                                );
        }

        gdk_gc_set_foreground (mgp->pv_ptr->da_widget->style->black_gc, &fg);
  }

  /* draw CURSOR */
  if(mgp->show_cursor)
  {
      if(gap_debug) printf("mov_path_prevw_draw: draw-cursor cur=%d,%d\n"
             , mgp->curx
             , mgp->cury
             );
      gdk_gc_set_function ( mgp->pv_ptr->da_widget->style->black_gc, GDK_INVERT);

      gdk_draw_line ( mgp->pv_ptr->da_widget->window,
		      mgp->pv_ptr->da_widget->style->black_gc,
		      mgp->curx, 1, mgp->curx, mgp->pheight-1 );
      gdk_draw_line ( mgp->pv_ptr->da_widget->window,
		      mgp->pv_ptr->da_widget->style->black_gc,
		      1, mgp->cury, mgp->pwidth-1, mgp->cury );
      /* current position of cursor is updated */
      gdk_gc_set_function ( mgp->pv_ptr->da_widget->style->black_gc, GDK_COPY);
  }
}

/* Adjustment Update Callbacks (with instant_update request) */

static void
mov_instant_int_adjustment_update(GtkObject *obj, gpointer val)
{
  t_mov_gui_stuff *mgp;
  
  gimp_int_adjustment_update(GTK_ADJUSTMENT(obj), val);
  
  
  mgp = g_object_get_data( G_OBJECT(obj), "mgp" );
  mov_set_instant_apply_request(mgp);
}  /* end mov_instant_int_adjustment_update */

static void
mov_instant_double_adjustment_update(GtkObject *obj, gpointer val)
{
  t_mov_gui_stuff *mgp;
  
  gimp_double_adjustment_update(GTK_ADJUSTMENT(obj), val);
  
  mgp = g_object_get_data( G_OBJECT(obj), "mgp" );
  mov_set_instant_apply_request(mgp);
}  /* end mov_instant_double_adjustment_update */


/*
 *  mov_path_xy_adjustment_update
 */

static void
mov_path_x_adjustment_update( GtkWidget *widget,
			      gpointer data )
{
  t_mov_gui_stuff *mgp;
  gint old_val;

  mgp = (t_mov_gui_stuff *)data;
  if(mgp == NULL) return;
  
  old_val = mgp->p_x;
  gimp_int_adjustment_update(GTK_ADJUSTMENT(widget), &mgp->p_x);
  if( old_val != mgp->p_x )
  {
      if( !mgp->in_call )
      {
	  mov_path_prevw_cursor_update( mgp );
	  mov_path_prevw_draw ( mgp, CURSOR | PATH_LINE );
          mov_set_instant_apply_request(mgp);
      }
  }
}

static void
mov_path_y_adjustment_update( GtkWidget *widget,
			      gpointer data )
{
  t_mov_gui_stuff *mgp;
  gint old_val;

  mgp = (t_mov_gui_stuff *)data;
  if(mgp == NULL) return;
  
  old_val = mgp->p_y;
  gimp_int_adjustment_update(GTK_ADJUSTMENT(widget), &mgp->p_y);
  if( old_val != mgp->p_y )
  {
      if( !mgp->in_call )
      {
	  mov_path_prevw_cursor_update( mgp );
	  mov_path_prevw_draw ( mgp, CURSOR | PATH_LINE );
          mov_set_instant_apply_request(mgp);
      }
  }
}

static void
mov_path_tfactor_adjustment_update( GtkWidget *widget,
			    gdouble *val )
{
  gdouble old_val;
  t_mov_gui_stuff *mgp;

  mgp = g_object_get_data( G_OBJECT(widget), "mgp" );

  if(mgp == NULL) return;
  old_val = *val;
  gimp_double_adjustment_update(GTK_ADJUSTMENT(widget), (gpointer)val);
  if( old_val != *val )
  {
      mov_set_instant_apply_request(mgp);
      if( !mgp->in_call )
      {
	  mov_path_prevw_cursor_update( mgp );
	  mov_path_prevw_draw ( mgp, CURSOR | PATH_LINE );
	  //XXX check if we need an additional GRID flag for the preview
      }
  }
}  /* end mov_path_tfactor_adjustment_update */


static void
mov_path_resize_adjustment_update( GtkObject *obj, gdouble *val)
{
  gdouble old_val;
  t_mov_gui_stuff *mgp;

  mgp = g_object_get_data( G_OBJECT(obj), "mgp" );

  if(mgp == NULL) return;
  old_val = *val;
  gimp_double_adjustment_update(GTK_ADJUSTMENT(obj), (gpointer)val);
  if( old_val != *val )
  {
    mov_set_instant_apply_request(mgp);
    if (gimp_chain_button_get_active (GIMP_CHAIN_BUTTON (mgp->constrain)))
    {
      /* in the constrain mode we propagate the value
       * for with and height resize factors to the other one
       * this constrains both spinbuttons to the same factor
       * and keeps the original src layer proportions
       */
      if(GTK_ADJUSTMENT(obj) == mgp->wres_adj)
      {
        gtk_adjustment_set_value(mgp->hres_adj, (gfloat)*val);
      }
      else
      {
        gtk_adjustment_set_value(mgp->wres_adj, (gfloat)*val);
      }
    }
  }
}  /* end mov_path_resize_adjustment_update */

static void 
mov_path_tween_steps_adjustment_update ( GtkObject *obj, gint *val)
{
  t_mov_gui_stuff *mgp;

  if((obj == NULL) || (val == NULL))
  {
    return;
  }
  gimp_int_adjustment_update(GTK_ADJUSTMENT(obj), val);
  mgp = g_object_get_data( G_OBJECT(obj), "mgp" );
  if(mgp)
  {
    mov_tweenlayer_sensitivity(mgp);
  }
  
}  /* end mov_path_tween_steps_adjustment_update */


/*
 *  Update the cross cursor's  coordinates accoding to pvals->[xy]path_prevw
 *  but not redraw it
 */

static void
mov_path_prevw_cursor_update ( t_mov_gui_stuff *mgp )
{
  mgp->curx = mgp->p_x * mgp->pwidth / mgp->dwidth;
  mgp->cury = mgp->p_y * mgp->pheight / mgp->dheight;

  if( mgp->curx < 0 )		       mgp->curx = 0;
  else if( mgp->curx >= mgp->pwidth )  mgp->curx = mgp->pwidth-1;
  if( mgp->cury < 0 )		       mgp->cury = 0;
  else if( mgp->cury >= mgp->pheight)  mgp->cury = mgp->pheight-1;

}

/*
 *    Handle the expose event on the preview
 */
static gint
mov_path_prevw_preview_expose( GtkWidget *widget,
			    GdkEvent *event )
{
  t_mov_gui_stuff *mgp;

  mgp = g_object_get_data( G_OBJECT(widget), "mgp" );

  if((mgp->pv_ptr == NULL)
  || (mgp->in_call))
  {
       return FALSE;
  }

  if(mgp->pv_ptr->da_widget == NULL)
  {
       return FALSE;
  }

  mgp->in_call = TRUE;
  mov_path_prevw_draw( mgp, ALL );
  mgp->in_call = FALSE;
  return FALSE;
}


/*
 *    Handle other events on the preview
 */

static gint
mov_path_prevw_preview_events ( GtkWidget *widget,
			     GdkEvent *event )
{
  t_mov_gui_stuff *mgp;
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  gint upd_flag;
  gint mouse_button;

  mgp = g_object_get_data ( G_OBJECT(widget), "mgp" );

 /* HINT:
  * smooth update of both CURSOR and PATH_LINE
  * on every mousemove works fine on machines with 300MHz.
  * for slower machines it once was better to paint just the cross cursor,
  * and refresh the path lines only at
  * button press and release events.
  * 2003.07.12 hof: 
  *    since we use a drawing_area that is repainted at each expose
  *    event, we MUST force painting of the PATH_LINE.
  *    (if we dont, the pathline disappears completely until the mouse
  *     button is released)
  *    The gap_pview_da repaint is faster than the old render_preview procedure
  *    so it may work even for slower machines now. 
  */
  upd_flag = CURSOR | PATH_LINE;
  /* upd_flag = CURSOR; */ 
       
  mouse_button = 0;
  
  switch (event->type)
    {
    case GDK_EXPOSE:
      break;

    case GDK_BUTTON_RELEASE:
      bevent = (GdkEventButton *) event;
      mouse_button = 0 - bevent->button;
      mov_set_instant_apply_request(mgp);
      goto mbuttons;
    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;
      mouse_button = bevent->button;
    mbuttons:
      mgp->curx = bevent->x;
      mgp->cury = bevent->y;
      upd_flag = CURSOR | PATH_LINE;    
      goto mouse;

    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *) event;
      if ( !mevent->state ) break;
      mgp->curx = mevent->x;
      mgp->cury = mevent->y;
      mov_set_instant_apply_request(mgp);
    mouse:
      if((mouse_button == 1)
      && (mgp->show_path))
      {
         /* Picking of pathpoints is done only if
          *   the left mousebutton goes down (mouse_button == 1)
          *   and only if Path is visible
          */
         p_points_to_tab(mgp);
         mgp->p_x = mgp->curx * mgp->dwidth / mgp->pwidth;     
         mgp->p_y = mgp->cury * mgp->dheight / mgp->pheight;
         p_pick_nearest_point(mgp->p_x, mgp->p_y);
         p_point_refresh(mgp);
      }
      else
      {
         mgp->p_x = mgp->curx * mgp->dwidth / mgp->pwidth;     
         mgp->p_y = mgp->cury * mgp->dheight / mgp->pheight;
         p_points_to_tab(mgp);
         mov_path_prevw_cursor_update( mgp );
      }
      mov_path_prevw_draw( mgp, upd_flag);
      mgp->in_call = TRUE;
      gtk_adjustment_set_value (mgp->x_adj,
				(gfloat)mgp->p_x);
      gtk_adjustment_set_value (mgp->y_adj,
				(gfloat)mgp->p_y);    
      
      mgp->in_call = FALSE;
      break;

    default:
      break;
    }

  return FALSE;
}


/* ============================================================================
 * p_chk_keyframes
 *   check if controlpoints and keyframe settings are OK
 *   return TRUE if OK,
 *   Pop Up error Dialog window and return FALSE if NOT.
 * ============================================================================
 */

gint
p_chk_keyframes(t_mov_gui_stuff *mgp)
{
#define ARGC_ERRWINDOW 2

  static t_arr_arg  argv[ARGC_APV];
  gint   l_idx;
  gchar *l_err_lbltext;
  static t_but_arg  b_argv[2];
  static gint  keychk_locked = FALSE;

  p_points_to_tab(mgp);

  l_err_lbltext = p_gap_chk_keyframes(pvals);

  if(*l_err_lbltext != '\0')
  {
    if(!keychk_locked)
    {
      keychk_locked = TRUE;
      p_init_arr_arg(&argv[0], WGT_LABEL);
      argv[0].label_txt = _("Can't operate with current Controlpoint\nor Keyframe settings");

      p_init_arr_arg(&argv[1], WGT_LABEL);
      argv[1].label_txt = l_err_lbltext;

      b_argv[0].but_txt  = _("Reset Keyframes");
      b_argv[0].but_val  = TRUE;
      b_argv[1].but_txt  = GTK_STOCK_CANCEL;
      b_argv[1].but_val  = FALSE;

      if(TRUE == p_array_std_dialog( _("Move Path Controlpointcheck"),
                                   _("Errors:"), 
                                   ARGC_ERRWINDOW, argv,
				   2,    b_argv, TRUE))
      {
	 /* Reset all keyframes */
	 for(l_idx = 0; l_idx <= pvals->point_idx_max; l_idx++ )
	 {
            pvals->point[l_idx].keyframe = 0;
            pvals->point[l_idx].keyframe_abs = 0;
            p_point_refresh(mgp);
	 }
      }
      keychk_locked = FALSE;
    }
    
    g_free(l_err_lbltext);
    return(FALSE);
    
  }
  g_free(l_err_lbltext);
  return(TRUE);
}	/* end p_chk_keyframes */


/* ============================================================================
 * p_get_flattened_drawable
 *   flatten the given image and return pointer to the
 *   (only) remaining drawable. 
 * ============================================================================
 */
GimpDrawable *
p_get_flattened_drawable(gint32 image_id)
{
  GimpDrawable *l_drawable_ptr ;

  l_drawable_ptr = gimp_drawable_get (p_get_flattened_layer(image_id, GIMP_CLIP_TO_IMAGE));
  return l_drawable_ptr;
}	/* end p_get_flattened_drawable */



/* ============================================================================
 *   add the selected source layer to the temp. preview image
 *   (modified accordung to current settings)
 *   then flatten the temporary preview image, 
 *   and return pointer to the (only) remaining drawable. 
 * ============================================================================
 */

GimpDrawable * 
p_get_prevw_drawable (t_mov_gui_stuff *mgp)
{
  t_mov_current l_curr;
  gint      l_nlayers;
 

  /* check if we have a source layer (to add to the preview) */
  if((pvals->src_layer_id >= 0) && (pvals->src_image_id >= 0))
  {
    p_points_to_tab(mgp);
    
    /* calculate current settings */
    l_curr.dst_frame_nr    = 0;

    l_curr.currX         = (gdouble)mgp->p_x;
    l_curr.currY         = (gdouble)mgp->p_y;
    l_curr.currOpacity   = (gdouble)mgp->opacity;
    l_curr.currWidth     = (gdouble)mgp->w_resize;
    l_curr.currHeight    = (gdouble)mgp->h_resize;
    l_curr.currRotation  = (gdouble)mgp->rotation;
    l_curr.currTTLX      = (gdouble)mgp->ttlx; 
    l_curr.currTTLY      = (gdouble)mgp->ttly; 
    l_curr.currTTRX      = (gdouble)mgp->ttrx; 
    l_curr.currTTRY      = (gdouble)mgp->ttry; 
    l_curr.currTBLX      = (gdouble)mgp->tblx; 
    l_curr.currTBLY      = (gdouble)mgp->tbly; 
    l_curr.currTBRX      = (gdouble)mgp->tbrx; 
    l_curr.currTBRY      = (gdouble)mgp->tbry; 

    l_curr.src_layer_idx   = 0;
    l_curr.src_layers      = gimp_image_get_layers (pvals->src_image_id, &l_nlayers);
    if((l_curr.src_layers != NULL) && (l_nlayers > 0))
    {
      l_curr.src_last_layer  = l_nlayers -1;
      /* findout index of src_layer_id */
      for(l_curr.src_layer_idx = 0; 
          l_curr.src_layer_idx  < l_nlayers;
          l_curr.src_layer_idx++)
      {
         if(l_curr.src_layers[l_curr.src_layer_idx] == pvals->src_layer_id)
            break;
      }
    }
    if(pvals->src_stepmode >= GAP_STEP_FRAME)
    {
      p_mov_fetch_src_frame (pvals, -1);  /* negative value fetches the selected frame number */
    }  

    
    /* set offsets (in cur_ptr) 
     *  according to handle_mode and src_img dimension (pvals) 
     */
    p_set_handle_offsets(pvals,  &l_curr);
    
    /* render: add source layer to (temporary) preview image */
    p_mov_render(pvals->tmp_image_id, pvals, &l_curr);

    if(l_curr.src_layers != NULL) g_free(l_curr.src_layers);
    l_curr.src_layers = NULL;    
  }

  /* flatten image, and get the (only) resulting drawable */
  return(p_get_flattened_drawable(pvals->tmp_image_id));

}	/* end p_get_prevw_drawable */


/* ============================================================================
 * p_set_handle_offsets
 *  set handle offsets according to handle mode and src image dimensions
 * ============================================================================
 */
void p_set_handle_offsets(t_mov_values *val_ptr, t_mov_current *cur_ptr)
{
  guint    l_src_width, l_src_height;         /* dimensions of the source image */

   /* get dimensions of source image */
   if((val_ptr->src_stepmode < GAP_STEP_FRAME)
   || (val_ptr->cache_tmp_image_id < 0))
   {
     l_src_width  = gimp_image_width(val_ptr->src_image_id);
     l_src_height = gimp_image_height(val_ptr->src_image_id);
   }
   else
   {
     /* for Frame Based Modes use the cached tmp image */
     l_src_width  = gimp_image_width(val_ptr->cache_tmp_image_id);
     l_src_height = gimp_image_height(val_ptr->cache_tmp_image_id);
   }

   cur_ptr->l_handleX = 0.0;
   cur_ptr->l_handleY = 0.0;
   switch(val_ptr->src_handle)
   {
      case GAP_HANDLE_LEFT_BOT:
         cur_ptr->l_handleY += l_src_height;
         break;
      case GAP_HANDLE_RIGHT_TOP:
         cur_ptr->l_handleX += l_src_width;
         break;
      case GAP_HANDLE_RIGHT_BOT:
         cur_ptr->l_handleX += l_src_width;
         cur_ptr->l_handleY += l_src_height;
         break;
      case GAP_HANDLE_CENTER:
         cur_ptr->l_handleX += (l_src_width  / 2);
         cur_ptr->l_handleY += (l_src_height / 2);
         break;
      case GAP_HANDLE_LEFT_TOP:
      default:
         break;
   }
}	/* end p_set_handle_offsets */

static void
mov_set_instant_apply_request(t_mov_gui_stuff *mgp)
{
  if(mgp)
  {
    mgp->instant_apply_request = TRUE; /* request is handled by timer */
  }
}  /* end mov_set_instant_apply_request */

/* ============================================================================
 * p_buildmenu
 *    build menu widget for all Items passed in the MenuItems Parameter
 *    MenuItems is an array of Pointers to Structure MenuItem.
 *    The End is marked by a Structure Member where the label is a NULL pointer
 *    (simplifyed version of GIMP 1.0.2 bulid_menu procedur)
 * ============================================================================
 */

GtkWidget *
p_buildmenu (MenuItem            *items, t_mov_gui_stuff *mgp)
{
  GtkWidget *menu;
  GtkWidget *menu_item;

  menu = gtk_menu_new ();

  while (items->label)
  {
      menu_item = gtk_menu_item_new_with_label (items->label);
      gtk_container_add (GTK_CONTAINER (menu), menu_item);

      if (items->callback)
      {
        g_object_set_data(G_OBJECT(menu_item), "mgp", mgp);
	g_signal_connect (G_OBJECT (menu_item), "activate",
			  G_CALLBACK (items->callback),
			  items->user_data);
      }
      gtk_widget_show (menu_item);
      items->widget = menu_item;

      items++;
  }

  return menu;
}	/* end p_buildmenu */


/* ----------------------------------
 * p_mov_spinbutton_new
 * ----------------------------------
 * create label and spinbutton and add to table
 * return the adjustment of the spinbutton
 * (for compatible parameters to gimp_scale_entry_new
 *  there are some unused dummy parameters)
 */
GtkObject *
p_mov_spinbutton_new(GtkTable *table
                    ,gint      col
                    ,gint      row
                    ,gchar    *label_text
                    ,gint      scale_width      /* dummy, not used */
                    ,gint      spinbutton_width
                    ,gdouble   initial_val
                    ,gdouble   lower            /* dummy, not used */
                    ,gdouble   upper            /* dummy, not used */
                    ,gdouble   sstep
                    ,gdouble   pagestep
                    ,gint      digits
                    ,gboolean  constrain
                    ,gdouble   umin
                    ,gdouble   umax
                    ,gchar    *tooltip_text
                    ,gchar    *privatetip
                    )
{
  GtkObject      *adj;
  GtkWidget      *spinbutton;
  GtkWidget	 *label;

  label = gtk_label_new (label_text);
  gtk_misc_set_alignment( GTK_MISC(label), 0.0, 0.5 );
  gtk_table_attach( GTK_TABLE(table), label, col, col+1, row, row+1,
		    GTK_FILL, 0, 4, 0 );
  gtk_widget_show(label);

  spinbutton = gimp_spin_button_new (&adj  /* return value */
		, initial_val
		, umin
		, umax
		, sstep
		, pagestep
		, 0.0                 /* page_size */
		, 1.0                 /* climb_rate */
		, digits
                );
  gtk_widget_set_size_request(spinbutton, spinbutton_width, -1);
  gtk_widget_show (spinbutton);
  gtk_table_attach (GTK_TABLE (table), spinbutton, col+1, col+2, row, row+1,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 0, 0);
  gimp_help_set_help_data (spinbutton, tooltip_text, privatetip);

  g_object_set_data (G_OBJECT (adj), "label", label);
  g_object_set_data (G_OBJECT (adj), "spinbutton", spinbutton);

  return(adj);
}  /* end p_mov_spinbutton_new */
