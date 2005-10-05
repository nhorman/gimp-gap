/* gap_wr_color_curve.c
 * 2000.Feb.21 hof (Wolfgang Hofer)
 *
 *  Wrapper Plugin for GIMP Curves tool
 *
 * Warning: This is just a QUICK HACK to enable
 *          Animated Filterapply in conjunction with the
 *          GIMP Curve Tool.
 *
 *  It provides a primitive Dialog Interface where you
 *  can load Curves (from Files saved by the Curve Tool)
 *  and Run The Curve Tool with these values as a Plugin on the current Drawable.
 *
 *  Further it has an Interface to 'Run_with_last_values'
 *  and an Iterator Procedure.
 *  (This enables the 'Animated Filter Call' from
 *   the GAP's Menu Filters->Filter all Layers
 *   and You can create Curves based Coloranimations)
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

/* Revision history
 *  (2005/01/29)  v2.1   hof: added Help
 *  (2004/01/15)  v1.3   hof: integrated into GIMP-GAP
 *  (2003/10/30)  v1.3   hof: adapted for gimp-1.3.x and gtk+2.2 API
 *  (2002/10/27)  v1.03  hof: appear in menu (enable filtermacro use)
 *  (2002/01/01)  v1.02  hof: removed GIMP_ENABLE_COMPAT_CRUFT (gimp 1.2.x)
 *  (2000/10/06)  v1.01  hof: - GIMP_ENABLE_COMPAT_CRUFT to compile with gimptool 1.1.26
 *  (2000/02/22)  v1.0   hof: first public release
 *  (2000/02/21)  v0.0   hof: coding started,
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gap_stock.h"

#include "gap-intl.h"


/* Defines */
#define PLUG_IN_NAME        "plug_in_wr_curves"
#define PLUG_IN_VERSION     "v1.3 (2003/12/19)"
#define PLUG_IN_IMAGE_TYPES "RGB*, GRAY*"
#define PLUG_IN_AUTHOR      "Wolfgang Hofer (hof@gimp.org)"
#define PLUG_IN_COPYRIGHT   "Wolfgang Hofer"
#define PLUG_IN_DESCRIPTION "Wrapper for GIMP Curves Tool call based on Curves file"

#define PLUG_IN_ITER_NAME       "plug_in_wr_curves_Iterator"
#define PLUG_IN_DATA_ITER_FROM  "plug_in_wr_curves_ITER_FROM"
#define PLUG_IN_DATA_ITER_TO    "plug_in_wr_curves_ITER_TO"

#define PLUG_IN_HELP_ID         "plug-in-wr-curves"

/* pointval_t
 *  wanted to use gdouble to store plugin data
 *  but in older version of gap_filter_pdb.h
 *  there is a hardcoded limt
 *
 *    PLUGIN_DATA_SIZE       8192
 *
 *  pointval_t guint8  precision will do it for the
 *  curves tool.
 *  But you may set pointval_t to gdouble
 *  if you have a gimp/gap version newer than 1.1.17
 */

typedef guint8 pointval_t;
/* typedef gdouble pointval_t; */

typedef struct
{
  pointval_t val_curve[5][256];
} wr_curves_val_t;


/* The following Stuff for calculating the Curve
 * was taken from gimp-1.1.17/app/curves.c
 * (and reduced for non-ineracive usage)
 */
#define SMOOTH 0
#define GFREE  1

typedef double CRMatrix[4][4];

typedef struct _CurvesDialog CurvesDialog;

struct _CurvesDialog
{
  gint          channel;

  gint          curve_type[5];
  gint          points[5][17][2];
  guchar        curve[5][256];
  gint          col_value[5];

};

typedef struct _WrCurveDialog WrCurveDialog;

struct _WrCurveDialog
{
  gint          run;
  gint          show_progress;
  gchar        *filename;
  GtkWidget *shell;
  GtkWidget *filesel;
  GtkWidget *entry;

};


WrCurveDialog *do_dialog (wr_curves_val_t *);
static void   wr_curve_response (GtkWidget *widget,
                 gint       response_id,
                 WrCurveDialog *gpp);
static void   curves_CR_compose      (CRMatrix, CRMatrix, CRMatrix);
static void   curves_plot_curve      (CurvesDialog *, int, int, int, int);
static void  query (void);
static void  run(const gchar *name
           , gint nparams
           , const GimpParam *param
           , gint *nreturn_vals
           , GimpParam **return_vals);

/* Global Variables */
GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,   /* init_proc  */
  NULL,   /* quit_proc  */
  query,  /* query_proc */
  run     /* run_proc   */
};

static CRMatrix CR_basis =
{
  { -0.5,  1.5, -1.5,  0.5 },
  {  1.0, -2.5,  2.0, -0.5 },
  { -0.5,  0.0,  0.5,  0.0 },
  {  0.0,  1.0,  0.0,  0.0 },
};


/* --------------
 * procedures
 * --------------
 */


static void
curves_CR_compose (CRMatrix a,
		   CRMatrix b,
		   CRMatrix ab)
{
  gint i, j;

  for (i = 0; i < 4; i++)
    {
      for (j = 0; j < 4; j++)
        {
          ab[i][j] = (a[i][0] * b[0][j] +
                      a[i][1] * b[1][j] +
                      a[i][2] * b[2][j] +
                      a[i][3] * b[3][j]);
        }
    }
}


static void
curves_plot_curve (CurvesDialog *cd,
		   gint          p1,
		   gint          p2,
		   gint          p3,
		   gint          p4)
{
  CRMatrix geometry;
  CRMatrix tmp1, tmp2;
  CRMatrix deltas;
  double x, dx, dx2, dx3;
  double y, dy, dy2, dy3;
  double d, d2, d3;
  int lastx, lasty;
  gint32 newx, newy;
  int i;

  /* construct the geometry matrix from the segment */
  for (i = 0; i < 4; i++)
    {
      geometry[i][2] = 0;
      geometry[i][3] = 0;
    }

  for (i = 0; i < 2; i++)
    {
      geometry[0][i] = cd->points[cd->channel][p1][i];
      geometry[1][i] = cd->points[cd->channel][p2][i];
      geometry[2][i] = cd->points[cd->channel][p3][i];
      geometry[3][i] = cd->points[cd->channel][p4][i];
    }

  /* subdivide the curve 1000 times */
  /* n can be adjusted to give a finer or coarser curve */
  d = 1.0 / 1000;
  d2 = d * d;
  d3 = d * d * d;

  /* construct a temporary matrix for determining the forward differencing deltas */
  tmp2[0][0] = 0;     tmp2[0][1] = 0;     tmp2[0][2] = 0;    tmp2[0][3] = 1;
  tmp2[1][0] = d3;    tmp2[1][1] = d2;    tmp2[1][2] = d;    tmp2[1][3] = 0;
  tmp2[2][0] = 6*d3;  tmp2[2][1] = 2*d2;  tmp2[2][2] = 0;    tmp2[2][3] = 0;
  tmp2[3][0] = 6*d3;  tmp2[3][1] = 0;     tmp2[3][2] = 0;    tmp2[3][3] = 0;

  /* compose the basis and geometry matrices */
  curves_CR_compose (CR_basis, geometry, tmp1);

  /* compose the above results to get the deltas matrix */
  curves_CR_compose (tmp2, tmp1, deltas);

  /* extract the x deltas */
  x = deltas[0][0];
  dx = deltas[1][0];
  dx2 = deltas[2][0];
  dx3 = deltas[3][0];

  /* extract the y deltas */
  y = deltas[0][1];
  dy = deltas[1][1];
  dy2 = deltas[2][1];
  dy3 = deltas[3][1];

  lastx = CLAMP (x, 0, 255);
  lasty = CLAMP (y, 0, 255);

  cd->curve[cd->channel][lastx] = lasty;

  /* loop over the curve */
  for (i = 0; i < 1000; i++)
    {
      /* increment the x values */
      x += dx;
      dx += dx2;
      dx2 += dx3;

      /* increment the y values */
      y += dy;
      dy += dy2;
      dy2 += dy3;

      newx = CLAMP0255 (ROUND (x));
      newy = CLAMP0255 (ROUND (y));

      /* if this point is different than the last one...then draw it */
      if ((lastx != newx) || (lasty != newy))
	cd->curve[cd->channel][newx] = newy;

      lastx = newx;
      lasty = newy;
    }
}

void
curves_calculate_curve (CurvesDialog *cd)
{
  int i;
  int points[17];
  int num_pts;
  int p1, p2, p3, p4;

  switch (cd->curve_type[cd->channel])
    {
    case GFREE:
      break;
    case SMOOTH:
      /*  cycle through the curves  */
      num_pts = 0;
      for (i = 0; i < 17; i++)
	if (cd->points[cd->channel][i][0] != -1)
	  points[num_pts++] = i;

      /*  Initialize boundary curve points */
      if (num_pts != 0)
	{
	  for (i = 0; i < cd->points[cd->channel][points[0]][0]; i++)
	    cd->curve[cd->channel][i] = cd->points[cd->channel][points[0]][1];
	  for (i = cd->points[cd->channel][points[num_pts - 1]][0]; i < 256; i++)
	    cd->curve[cd->channel][i] = cd->points[cd->channel][points[num_pts - 1]][1];
	}

      for (i = 0; i < num_pts - 1; i++)
	{
	  p1 = (i == 0) ? points[i] : points[(i - 1)];
	  p2 = points[i];
	  p3 = points[(i + 1)];
	  p4 = (i == (num_pts - 2)) ? points[(num_pts - 1)] : points[(i + 2)];

	  curves_plot_curve (cd, p1, p2, p3, p4);
	}
      break;
    }

}


static gboolean
read_curves_from_file (FILE *fp, wr_curves_val_t *cuvals)
{
  gint i, j, fields;
  gchar buf[50];
  gint index[5][17];
  gint value[5][17];
  gint current_channel;
  CurvesDialog   curves_dialog_struct;
  CurvesDialog   *curves_dialog = &curves_dialog_struct;

  if (!fgets (buf, 50, fp))
    return FALSE;

  if (strcmp (buf, "# GIMP Curves File\n") != 0)
    return FALSE;

  for (i = 0; i < 5; i++)
  {
    for (j = 0; j < 17; j++)
	{
	  fields = fscanf (fp, "%d %d ", &index[i][j], &value[i][j]);
	  if (fields != 2)
	    {
	      g_print ("fields != 2");
	      return FALSE;
	    }
	}
  }

  for (i = 0; i < 5; i++)
  {
    curves_dialog->curve_type[i] = SMOOTH;
    for (j = 0; j < 17; j++)
	{
	  curves_dialog->points[i][j][0] = index[i][j];
	  curves_dialog->points[i][j][1] = value[i][j];
	}
  }

  /* this is ugly, but works ... */
  current_channel = curves_dialog->channel;
  for (i = 0; i < 5; i++)
  {
      curves_dialog->channel = i;
      curves_calculate_curve (curves_dialog);
      for(j = 0; j < 256; j++)
      {
	    cuvals->val_curve[i][j] = (pointval_t)curves_dialog->curve[i][j];
      }
  }
  curves_dialog->channel = current_channel;

  return TRUE;
}


static int
p_load_curve(gchar *filename, wr_curves_val_t *cuvals)
{
  FILE  *fp;

  fp = fopen (filename, "rt");

  if (!fp)
  {
	  g_message (_("Unable to open file %s"), filename);
	  return -1;
  }

  if (!read_curves_from_file (fp, cuvals))
  {
	  g_message (("Error in reading file %s"), filename);
      fclose (fp);
	  return -1;
  }

  fclose (fp);
  return 0;
}


/*
 * Delta Calculations for the iterator
 */
static void
p_delta_pointval_t (pointval_t  *val,
		pointval_t   val_from,
		pointval_t   val_to,
		 gint32   total_steps,
		 gdouble  current_step)
{
    gdouble     delta;

    if(total_steps < 1) return;

    delta = ((gdouble)(val_to - val_from) / (gdouble)total_steps) * ((gdouble)total_steps - current_step);
    *val  = val_from + delta;
}


/* ============================================================================
 * p_gimp_curves_explicit
 *
 * ============================================================================
 */

gint32
p_gimp_curves_explicit (gint32 drawable_id,
		   gint32 channel,
		   gint32 num_bytes,
		   gint8 *curve_points)
{
   static char     *l_procname = "gimp_curves_explicit";
   GimpParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_procname,
                                 &nreturn_vals,
                                 GIMP_PDB_DRAWABLE,  drawable_id,
                                 GIMP_PDB_INT32,     channel,
                                 GIMP_PDB_INT32,     num_bytes,
                                 GIMP_PDB_INT8ARRAY, curve_points,
                                 GIMP_PDB_END);

   if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
   {
      gimp_destroy_params(return_vals, nreturn_vals);
      return (0);
   }
   gimp_destroy_params(return_vals, nreturn_vals);
   printf("Error: PDB call of %s failed status:%d\n", l_procname, (int)return_vals[0].data.d_status);
   return(-1);
}	/* end p_gimp_curves_explicit */

static void
p_run_curves_tool(gint32 drawable_id, wr_curves_val_t *cuvals)
{
  gint32  l_idx;
  gint32  l_channel;
  gint8 curve_points[256];

  for(l_channel=0; l_channel < 5; l_channel++)
  {
     for(l_idx=0; l_idx < 256; l_idx++)
     {
       /* curve_points[l_idx] = (gint8)ROUND(cuvals->val_curve[l_channel][l_idx]); */
       curve_points[l_idx] = (gint8)cuvals->val_curve[l_channel][l_idx];
     }
     p_gimp_curves_explicit(drawable_id, l_channel, 256, curve_points);
  }
}

MAIN ()

static void
query (void)
{
  static GimpParamDef args[] = {
                  { GIMP_PDB_INT32,      "run_mode", "Interactive, non-interactive"},
                  { GIMP_PDB_IMAGE,      "image", "Input image" },
                  { GIMP_PDB_DRAWABLE,   "drawable", "Input drawable (must be a layer)"},
                  { GIMP_PDB_STRING,     "filename", "Name of a #GIMP curves file (saved by the original Curve Tool)"},
  };
  static int nargs = sizeof(args) / sizeof(args[0]);

  static GimpParamDef return_vals[] =
  {
    { GIMP_PDB_DRAWABLE, "the_drawable", "the handled drawable" }
  };
  static int nreturn_vals = sizeof(return_vals) / sizeof(return_vals[0]);


  static GimpParamDef args_iter[] =
  {
    {GIMP_PDB_INT32, "run_mode", "non-interactive"},
    {GIMP_PDB_INT32, "total_steps", "total number of steps (# of layers-1 to apply the related plug-in)"},
    {GIMP_PDB_FLOAT, "current_step", "current (for linear iterations this is the layerstack position, otherwise some value inbetween)"},
    {GIMP_PDB_INT32, "len_struct", "length of stored data structure with id is equal to the plug_in  proc_name"},
  };
  static int nargs_iter = sizeof(args_iter) / sizeof(args_iter[0]);

  static GimpParamDef *return_iter = NULL;
  static int nreturn_iter = 0;

  /* the actual installation of the bend plugin */
  gimp_install_procedure (PLUG_IN_NAME,
                          PLUG_IN_DESCRIPTION,
  			 "This Plugin loads a # GIMP Curves File,"
			 " that was saved by the GIMP 2.0pre1 Curves Tool"
			 " then calculates the curves (256 points foreach channel val,r,g,b,a)"
			 " and calls the Curve Tool via PDB interface with the calculated curve points"
			 " It also stores the points, and offers a GIMP_RUN_WITH_LAST_VALUES"
			 " Interface and an Iterator Procedure for animated calls"
			 " of the Curves Tool with varying values"
                          ,
                          PLUG_IN_AUTHOR,
                          PLUG_IN_COPYRIGHT,
                          PLUG_IN_VERSION,
                          N_("<Image>/Video/Layer/Colors/CurvesFile..."),
                          PLUG_IN_IMAGE_TYPES,
                          GIMP_PLUGIN,
                          nargs,
                          nreturn_vals,
                          args,
                          return_vals);


  /* the installation of the Iterator extension for the bend plugin */
  gimp_install_procedure (PLUG_IN_ITER_NAME,
                          "This extension calculates the modified values for one iterationstep for the call of plug_in_curve_bend",
                          "",
                          PLUG_IN_AUTHOR,
                          PLUG_IN_COPYRIGHT,
                          PLUG_IN_VERSION,
                          NULL,    /* do not appear in menus */
                          NULL,
                          GIMP_PLUGIN,
                          nargs_iter, nreturn_iter,
                          args_iter, return_iter);
}



static void  
run(const gchar *name
   , gint nparams
   , const GimpParam *param
   , gint *nreturn_vals
   , GimpParam **return_vals)
{
  wr_curves_val_t l_cuvals;
  WrCurveDialog   *wcd = NULL;

  gint32    l_image_id = -1;
  gint32    l_drawable_id = -1;
  gint32    l_handled_drawable_id = -1;

  /* Get the runmode from the in-parameters */
  GimpRunMode run_mode = param[0].data.d_int32;

  /* status variable, use it to check for errors in invocation usualy only
     during non-interactive calling */
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;

  /*always return at least the status to the caller. */
  static GimpParam values[2];


  INIT_I18N();

  /* initialize the return of the status */
  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
  values[1].type = GIMP_PDB_DRAWABLE;
  values[1].data.d_int32 = -1;
  *nreturn_vals = 2;
  *return_vals = values;

  /* the Iterator Stuff */
  if (strcmp (name, PLUG_IN_ITER_NAME) == 0)
  {
     gint32  len_struct;
     gint32  total_steps;
     gdouble current_step;
     wr_curves_val_t   cval;                  /* current values while iterating */
     wr_curves_val_t   cval_from, cval_to;    /* start and end values */
     gint  l_idi, l_idx;

     /* Iterator procedure for animated calls is usually called from
      * "plug_in_gap_layers_run_animfilter"
      * (always run noninteractive)
      */
     if ((run_mode == GIMP_RUN_NONINTERACTIVE) && (nparams == 4))
     {
       total_steps  =  param[1].data.d_int32;
       current_step =  param[2].data.d_float;
       len_struct   =  param[3].data.d_int32;

       if(len_struct == sizeof(cval))
       {
         /* get _FROM and _TO data,
          * This data was stored by plug_in_gap_layers_run_animfilter
          */
          gimp_get_data(PLUG_IN_DATA_ITER_FROM, &cval_from);
          gimp_get_data(PLUG_IN_DATA_ITER_TO,   &cval_to);
          memcpy(&cval, &cval_from, sizeof(cval));
          for(l_idi = 0; l_idi < 5; l_idi++)
          {
	     for(l_idx = 0; l_idx < 256; l_idx++)
	     {
               p_delta_pointval_t(&cval.val_curve[l_idi][l_idx],
                    cval_from.val_curve[l_idi][l_idx],
                    cval_to.val_curve[l_idi][l_idx],
                    total_steps, current_step);
             }
	  }
          gimp_set_data(PLUG_IN_NAME, &cval, sizeof(cval));
       }
       else status = GIMP_PDB_CALLING_ERROR;
     }
     else status = GIMP_PDB_CALLING_ERROR;

     values[0].data.d_status = status;
     return;
  }




  /* get image and drawable */
  l_image_id = param[1].data.d_int32;
  l_drawable_id = param[2].data.d_drawable;

  if(status == GIMP_PDB_SUCCESS)
  {
    /* how are we running today? */
    switch (run_mode)
     {
      case GIMP_RUN_INTERACTIVE:
        /* Get information from the dialog */
        wcd = do_dialog(&l_cuvals);
        wcd->show_progress = TRUE;
	if(wcd->filename == NULL)
	{
           status = GIMP_PDB_EXECUTION_ERROR;
	}
	else
	{
	  if(p_load_curve(wcd->filename, &l_cuvals) < 0)
          {
            status = GIMP_PDB_EXECUTION_ERROR;
	  }
        }
        break;

      case GIMP_RUN_NONINTERACTIVE:
        /* check to see if invoked with the correct number of parameters */
        if (nparams >= 5)
        {
           wcd = g_malloc (sizeof (WrCurveDialog));
           wcd->run = TRUE;
           wcd->show_progress = FALSE;

           wcd->filename = g_strdup(param[3].data.d_string);
        }
        else
        {
          status = GIMP_PDB_CALLING_ERROR;
        }

        break;

      case GIMP_RUN_WITH_LAST_VALS:
        wcd = g_malloc (sizeof (WrCurveDialog));
        wcd->run = TRUE;
        wcd->show_progress = TRUE;
        /* Possibly retrieve data from a previous run */
        gimp_get_data (PLUG_IN_NAME, &l_cuvals);
        break;

      default:
        break;
    }
  }

  if (wcd == NULL)
  {
    status = GIMP_PDB_EXECUTION_ERROR;
  }

  if (status == GIMP_PDB_SUCCESS)
  {
     /* Run the main function */
     if(wcd->run)
     {
	gimp_image_undo_group_start(l_image_id);
	p_run_curves_tool(l_drawable_id, &l_cuvals);
        l_handled_drawable_id = l_drawable_id;
	gimp_image_undo_group_end(l_image_id);

	/* Store variable states for next run */
	if (run_mode == GIMP_RUN_INTERACTIVE)
	{
	  gimp_set_data(PLUG_IN_NAME, &l_cuvals, sizeof(l_cuvals));
	}
     }
     else
     {
       status = GIMP_PDB_EXECUTION_ERROR;       /* dialog ended with cancel button */
     }

     /* If run mode is interactive, flush displays, else (script) don't
        do it, as the screen updates would make the scripts slow */
     if (run_mode != GIMP_RUN_NONINTERACTIVE)
     {
       gimp_displays_flush ();
     }
  }
  values[0].data.d_status = status;
  values[1].data.d_int32 = l_handled_drawable_id;   /* return the id of handled layer */

}	/* end run */


/*
 * DIALOG and callback stuff
 */

static void
p_filesel_close_cb (GtkWidget *widget,
		    gpointer   data)
{
  WrCurveDialog *wcd;

  wcd = (WrCurveDialog *) data;

  if(wcd->filesel == NULL) return;

  gtk_widget_destroy(GTK_WIDGET(wcd->filesel));
  wcd->filesel = NULL;   /* now filesel is closed */
}

static void
p_filesel_ok_callback (GtkWidget *widget,
			 gpointer   data)
{
  WrCurveDialog *wcd;
  wr_curves_val_t  cuvals;

  wcd = (WrCurveDialog *) data;

  if(wcd->filesel == NULL) return;

  if(wcd->filename) g_free(wcd->filename);

  wcd->filename = g_strdup(gtk_file_selection_get_filename (GTK_FILE_SELECTION (wcd->filesel)));
  gtk_entry_set_text(GTK_ENTRY(wcd->entry), wcd->filename);

  gtk_widget_destroy(GTK_WIDGET(wcd->filesel));
  wcd->filesel = NULL;

  /* try to read the file,
   * (and make a g_message on errors)
   */
  p_load_curve(wcd->filename, &cuvals);
}

static void
wr_curve_load_callback (GtkWidget *w,
		      WrCurveDialog *wcd)
{
  GtkWidget *filesel;

  if(wcd == NULL)
  {
    return;
  }
  if(wcd->filesel != NULL)
  {
     gtk_window_present(GTK_WINDOW(wcd->filesel));
     return;   /* filesel is already open */
  }
  
  filesel = gtk_file_selection_new ( _("Load color curve from file"));
  wcd->filesel = filesel;

  gtk_window_set_position (GTK_WINDOW (filesel), GTK_WIN_POS_MOUSE);
  g_signal_connect (GTK_FILE_SELECTION (filesel)->ok_button,
                    "clicked", (GtkSignalFunc) p_filesel_ok_callback,
                    wcd);
  g_signal_connect (GTK_FILE_SELECTION (filesel)->cancel_button,
		      "clicked", (GtkSignalFunc) p_filesel_close_cb,
		      wcd);
  g_signal_connect (filesel, "destroy",
                    (GtkSignalFunc) p_filesel_close_cb,
                    wcd);
  if(wcd->filename)
  {
    gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesel),
                                     wcd->filename);
  }
  gtk_widget_show (filesel);

}

static void
text_entry_callback(GtkWidget *widget, WrCurveDialog *wcd)
{
  if(wcd)
  {
    if(wcd->filename) g_free(wcd->filename);
    wcd->filename = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));
  }
}

/* ---------------------------------
 * wr_curve_response
 * ---------------------------------
 */
static void
wr_curve_response (GtkWidget *widget,
                 gint       response_id,
                 WrCurveDialog *wcd)
{
  GtkWidget *dialog;

  switch (response_id)
  {
    case GTK_RESPONSE_OK:
      if(wcd)
      {
	if (GTK_WIDGET_VISIBLE (wcd->shell))
	  gtk_widget_hide (wcd->shell);

	wcd->run = TRUE;
      }

    default:
      dialog = NULL;
      if(wcd)
      {
        dialog = wcd->shell;
	if(dialog)
	{
          wcd->shell = NULL;
          gtk_widget_destroy (dialog);
	}
      }
      gtk_main_quit ();
      break;
  }
}  /* end wr_curve_response */



WrCurveDialog *
do_dialog (wr_curves_val_t *cuvals)
{
  WrCurveDialog *wcd;
  GtkWidget *dialog;
  GtkWidget  *vbox;
  GtkWidget  *hbox;
  GtkWidget  *button;
  GtkWidget  *entry;


  /* Init UI  */
  gimp_ui_init ("wr_curves", FALSE);
  gap_stock_init();


  /*  The curve_bend dialog  */
  wcd = g_malloc (sizeof (WrCurveDialog));
  wcd->run = FALSE;
  wcd->filesel = NULL;

  /*  The dialog and main vbox  */
  dialog = gimp_dialog_new (_("CurvesFile"), "curves_wrapper",
                               NULL, 0,
			       gimp_standard_help_func, PLUG_IN_HELP_ID,

                               GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                               GTK_STOCK_OK,     GTK_RESPONSE_OK,
                               NULL);

  wcd->shell = dialog;
  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (wr_curve_response),
                    wcd);

  /* the vbox */
  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), vbox,
		      TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  /* the hbox */
  hbox = gtk_hbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  /*  The Load button  */
  button = gtk_button_new_with_label (_("Load Curve"));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, FALSE, 0);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (wr_curve_load_callback),
		    wcd);
  gtk_widget_show (button);
  gimp_help_set_help_data (button,
			   _("Load curve from a GIMP curve file "
			     "(that was saved with the GIMP's color curve tool)"),
			   NULL);

  /*  The filename entry */
  entry = gtk_entry_new();
  gtk_widget_set_size_request(entry, 350, 0);
  gtk_entry_set_text(GTK_ENTRY(entry), "");
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, FALSE, 0);
  g_signal_connect(G_OBJECT(entry), "changed",
		   G_CALLBACK (text_entry_callback),
		   wcd);
  gtk_widget_show (entry);
  wcd->entry = entry;

  gtk_widget_show (dialog);

  gtk_main ();
  gdk_flush ();


  return wcd;
}
