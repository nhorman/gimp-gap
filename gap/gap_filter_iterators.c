/* gap_filter_iterators.c
 *
 * 1998.01.29 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * This Module contains:
 * - Implementation of c common Iterator Procedure
 *     for those Plugins
 *     - that can operate on a single drawable,
 *     - and have paramters for varying. (that are documented in the LASTVALUES Decription file)
 *
 * If New plugins were added to the gimp, or existing ones were updated,
 * the Authors should supply original _Iterator Procedures
 * to modify the settings for the plugin when called step by step
 * on animated multilayer Images.
 * Another simpler way is to register the structure description of the LAST_VALUES Buffer.
 * (so that the common iterator can be used)
 *
 *
 * Common things to all Iteratur Plugins:
 * Interface:   run_mode        # is always GIMP_RUN_NONINTERACTIVE
 *              total_steps     # total number of handled layers (drawables)
 *              current_step    # current layer (beginning wit 0)
 *                               has type gdouble for later extensions
 *                               to non-linear iterations.
 *                               the iterator always computes linear inbetween steps,
 *                               but the (central) caller may fake step 1.2345 in the future
 *                               for logaritmic iterations or userdefined curves.
 *
 * Naming Convention:
 *    Iterators must have the name of the plugin (PDB proc_name), whose values
 *    are iterated, with Suffix
 *      "_Iterator"
 *      "_Iterator_ALT"    (if not provided within the original Plugin's sources)
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

/* Change Log:
 * gimp     1.3.20b; 2003/09/20  hof: include gap_dbbrowser_utils.h, datatype support for guint, guint32
 *                                    replaced t_GckVector3 and t_GckColor by GimpVector3 and GimpRGB
 * gimp     1.3.17a; 2003/07/29  hof: fixed signed/unsigned comparison warnings
 * gimp     1.3.12a; 2003/05/02  hof: merge into CVS-gimp-gap project, added iter_ALT Procedures again
 * gimp   1.3.8a;    2002/09/21  hof: gap_lastvaldesc
 * gimp   1.3.5a;    2002/04/14  hof: removed Stuctured Iterators (Material, Light ..)
 * gimp   1.3.4b;    2002/03/24  hof: support COMMON_ITERATOR, removed iter_ALT Procedures
 * version gimp 1.1.17b  2000.02.22  hof: - removed limit PLUGIN_DATA_SIZE
 * 1999.11.16 hof: added p_delta_gintdrawable
 * 1999.06.21 hof: removed Colorify iterator
 * 1999.03.14 hof: added iterators for gimp 1.1.3 prerelease
 *                 iterator code reorganized in _iter_ALT.inc Files
 * 1998.06.12 hof: added p_delta_drawable (Iterate layers in the layerstack)
 *                 this enables to apply an animated bumpmap.
 * 1998.01.29 hof: 1st release
 */
#include "config.h"

/* SYTEM (UNIX) includes */
#include <stdio.h>
#include <string.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* GIMP includes */
#include "gtk/gtk.h"
#include "libgimp/gimp.h"


/* GAP includes */
#include "gap_lastvaldesc.h"
#include "gap_filter.h"
#include "gap_filter_iterators.h"
#include "gap_dbbrowser_utils.h"


static gchar *g_plugin_data_from = NULL;
static gchar *g_plugin_data_to = NULL;

extern int gap_debug;

/* Funtion type for use in the common iterator
 */
typedef void (*GapDeltaFunctionType) (void *val, void *val_from, void *val_to, gint32 total_steps, gdouble current_step);


typedef struct JmpTableEntryType
{
  GapDeltaFunctionType func_ptr;
  gint32               item_size;
} JmpTableEntryType;

typedef  struct IterStackItemType
{
   GimpLastvalType lastval_type;   /* only ARRAY and STRUCT are pushed on the stack */
   gint32       elem_size;  /* only for ARRAY and STRUCT */
   gint32       arr_count;
   gint         idx;
   gint32       iter_flag;
}IterStackItemType;

typedef  struct IterStackType
{
   IterStackItemType *stack_arr;   /* only ARRAY and STRUCT are pushed on the stack */
   gint32       stack_max_elem;    /* only for ARRAY and STRUCT */
   gint32       stack_top;         /* current stack top index (-1 for empty stack */
   gint         idx;
}IterStackType;


static JmpTableEntryType jmp_table[GIMP_LASTVAL_END];

static gchar *
p_alloc_plugin_data(char *key)
{
   int l_len;
   gchar *l_plugin_data;

   l_len = gimp_get_data_size (key);
   if(l_len < 1)
   {
      fprintf(stderr, "ERROR: no stored data found for Key %s\n", key);
      return NULL;
   }
   l_plugin_data = g_malloc0(l_len+1);

   if(gap_debug) printf("DEBUG  Key:%s  plugin_data length %d\n", key, (int)l_len);
   return (l_plugin_data);
}

typedef struct {
	guchar color[3];
} t_color;

typedef struct {
	gint color[3];
} t_gint_color;


/* ----------------------------------------------------------------------
 * iterator functions for basic datatypes
 * (were called from the generated procedures)
 * ----------------------------------------------------------------------
 */

static void p_delta_long(long *val, long val_from, long val_to, gint32 total_steps, gdouble current_step)
{
    double     delta;

    if(total_steps < 1) return;

    delta = ((double)(val_to - val_from) / (double)total_steps) * ((double)total_steps - current_step);
    *val  = val_from + delta;

    if(gap_debug) printf("DEBUG: p_delta_long from: %ld to: %ld curr: %ld    delta: %f\n",
                                  val_from, val_to, *val, delta);
}
static void p_delta_short(short *val, short val_from, short val_to, gint32 total_steps, gdouble current_step)
{
    double     delta;

    if(total_steps < 1) return;

    delta = ((double)(val_to - val_from) / (double)total_steps) * ((double)total_steps - current_step);
    *val  = val_from + delta;
}
static void p_delta_int(int *val, int val_from, int val_to, gint32 total_steps, gdouble current_step)
{
     double     delta;

     if(total_steps < 1) return;

     delta = ((double)(val_to - val_from) / (double)total_steps) * ((double)total_steps - current_step);
     *val  = val_from + delta;
}
static void p_delta_gint(gint *val, gint val_from, gint val_to, gint32 total_steps, gdouble current_step)
{
    double     delta;

    if(total_steps < 1) return;

    delta = ((double)(val_to - val_from) / (double)total_steps) * ((double)total_steps - current_step);
    *val  = val_from + delta;
}
static void p_delta_guint(guint *val, guint val_from, guint val_to, gint32 total_steps, gdouble current_step)
{
    double     delta;

    if(total_steps < 1) return;

    delta = ((double)(val_to - val_from) / (double)total_steps) * ((double)total_steps - current_step);
    *val  = val_from + delta;
}
static void p_delta_gint32(gint32 *val, gint32 val_from, gint32 val_to, gint32 total_steps, gdouble current_step)
{
    double     delta;

    if(total_steps < 1) return;

    delta = ((double)(val_to - val_from) / (double)total_steps) * ((double)total_steps - current_step);
    *val  = val_from + delta;
}
static void p_delta_guint32(guint32 *val, guint32 val_from, guint32 val_to, gint32 total_steps, gdouble current_step)
{
    double     delta;

    if(total_steps < 1) return;

    delta = ((double)(val_to - val_from) / (double)total_steps) * ((double)total_steps - current_step);
    *val  = val_from + delta;
}
static void p_delta_char(char *val, char val_from, char val_to, gint32 total_steps, gdouble current_step)
{
    double     delta;

    if(total_steps < 1) return;

    delta = ((double)(val_to - val_from) / (double)total_steps) * ((double)total_steps - current_step);
    *val  = val_from + delta;
}
static void p_delta_guchar(guchar *val, char val_from, char val_to, gint32 total_steps, gdouble current_step)
{
    double     delta;

    if(total_steps < 1) return;

    delta = ((double)(val_to - val_from) / (double)total_steps) * ((double)total_steps - current_step);
    *val  = val_from + delta;
}
static void p_delta_gdouble(double *val, double val_from, double val_to, gint32 total_steps, gdouble current_step)
{
   double     delta;

   if(total_steps < 1) return;

   delta = ((double)(val_to - val_from) / (double)total_steps) * ((double)total_steps - current_step);
   *val  = val_from + delta;

   if(gap_debug) printf("DEBUG: p_delta_gdouble total: %d  from: %f to: %f curr: %f    delta: %f\n",
                                  (int)total_steps, val_from, val_to, *val, delta);
}
static void p_delta_gfloat(gfloat *val, gfloat val_from, gfloat val_to, gint32 total_steps, gdouble current_step)
{
    double     delta;

    if(total_steps < 1) return;

    delta = ((double)(val_to - val_from) / (double)total_steps) * ((double)total_steps - current_step);
    *val  = val_from + delta;

    if(gap_debug) printf("DEBUG: p_delta_gfloat total: %d  from: %f to: %f curr: %f    delta: %f\n",
                                   (int)total_steps, val_from, val_to, *val, delta);
}

static void p_delta_float(float *val, float val_from, float val_to, gint32 total_steps, gdouble current_step)
{
    double     delta;

    if(total_steps < 1) return;

    delta = ((double)(val_to - val_from) / (double)total_steps) * ((double)total_steps - current_step);
    *val  = val_from + delta;

    if(gap_debug) printf("DEBUG: p_delta_gdouble total: %d  from: %f to: %f curr: %f    delta: %f\n",
                                  (int)total_steps, val_from, val_to, *val, delta);
}
static void p_delta_color(t_color *val, t_color *val_from, t_color *val_to, gint32 total_steps, gdouble current_step)
{
    double     delta;
    guint l_idx;

    if(total_steps < 1) return;

    for(l_idx = 0; l_idx < 3; l_idx++)
    {
       delta = ((double)(val_to->color[l_idx] - val_from->color[l_idx]) / (double)total_steps) * ((double)total_steps - current_step);
       val->color[l_idx]  = val_from->color[l_idx] + delta;

       if(gap_debug) printf("DEBUG: p_delta_color[%d] total: %d  from: %d to: %d curr: %d    delta: %f  current_step: %f\n",
                                  (int)l_idx, (int)total_steps,
                                  (int)val_from->color[l_idx], (int)val_to->color[l_idx], (int)val->color[l_idx],
                                  delta, current_step);
    }
}
static void p_delta_gint_color(t_gint_color *val, t_gint_color *val_from, t_gint_color *val_to, gint32 total_steps, gdouble current_step)
{
    double     delta;
    guint l_idx;

    if(total_steps < 1) return;

    for(l_idx = 0; l_idx < 3; l_idx++)
    {
       delta = ((double)(val_to->color[l_idx] - val_from->color[l_idx]) / (double)total_steps) * ((double)total_steps - current_step);
       val->color[l_idx]  = val_from->color[l_idx] + delta;
    }
}
static void p_delta_drawable(gint32 *val, gint32 val_from, gint32 val_to, gint32 total_steps, gdouble current_step)
{
    gint    l_nlayers;
    gint32 *l_layers_list;
    gint32  l_tmp_image_id;
    gint    l_idx, l_idx_from, l_idx_to;

    if((val_from < 0) || (val_to < 0))
    {
      return;
    }

    l_tmp_image_id = gimp_drawable_get_image(val_from);

    /* check if from and to values are both valid drawables within the same image */
    if ((l_tmp_image_id > 0)
    &&  (l_tmp_image_id = gimp_drawable_get_image(val_to)))
    {
       l_idx_from = -1;
       l_idx_to   = -1;

       /* check the layerstack index of from and to drawable */
       l_layers_list = gimp_image_get_layers(l_tmp_image_id, &l_nlayers);
       for (l_idx = l_nlayers -1; l_idx >= 0; l_idx--)
       {
          if( l_layers_list[l_idx] == val_from ) l_idx_from = l_idx;
          if( l_layers_list[l_idx] == val_to )   l_idx_to   = l_idx;

          if((l_idx_from != -1) && (l_idx_to != -1))
          {
            /* OK found both index values, iterate the index (proceed to next layer) */
            p_delta_gint(&l_idx, l_idx_from, l_idx_to, total_steps, current_step);
            *val = l_layers_list[l_idx];
            break;
          }
       }
       g_free (l_layers_list);
    }
}
static void
p_delta_gintdrawable(gint *val, gint val_from, gint val_to, gint32 total_steps, gdouble current_step)
{
  gint32 l_val, l_from, l_to;
  l_val = *val;
  l_from = val_from;
  l_to   = val_to;
  p_delta_drawable(&l_val, l_from, l_to, total_steps, current_step);
  *val = l_val;
}


/* -----------------------------
 * stuff for the common iterator
 * -----------------------------
 */
/*
static void p_delta_long(long *val, long val_from, long val_to, gint32 total_steps, gdouble current_step)
static void p_delta_short(short *val, short val_from, short val_to, gint32 total_steps, gdouble current_step)
static void p_delta_int(int *val, int val_from, int val_to, gint32 total_steps, gdouble current_step)
static void p_delta_gint(gint *val, gint val_from, gint val_to, gint32 total_steps, gdouble current_step)
static void p_delta_gint32(gint32 *val, gint32 val_from, gint32 val_to, gint32 total_steps, gdouble current_step)
static void p_delta_char(char *val, char val_from, char val_to, gint32 total_steps, gdouble current_step)
static void p_delta_guchar(guchar *val, char val_from, char val_to, gint32 total_steps, gdouble current_step)
static void p_delta_gdouble(double *val, double val_from, double val_to, gint32 total_steps, gdouble current_step)
static void p_delta_gfloat(gfloat *val, gfloat val_from, gfloat val_to, gint32 total_steps, gdouble current_step)
static void p_delta_float(float *val, float val_from, float val_to, gint32 total_steps, gdouble current_step)
static void p_delta_drawable(gint32 *val, gint32 val_from, gint32 val_to, gint32 total_steps, gdouble current_step)
static void p_delta_gintdrawable(gint *val, gint val_from, gint val_to, gint32 total_steps, gdouble current_step)

*/

/* wrapper calls with pointers to value transform
 * (for those delta procedures that do not
    already use pointers for val_from and val-to parameters)
 */
static void gp_delta_long (long *val, long *val_from, long *val_to, gint32 total_steps, gdouble current_step)
{ p_delta_long(val, *val_from, *val_to, total_steps, current_step); }

static void gp_delta_short (short *val, short *val_from, short *val_to, gint32 total_steps, gdouble current_step)
{ p_delta_short(val, *val_from, *val_to, total_steps, current_step); }

static void gp_delta_int (int *val, int *val_from, int *val_to, gint32 total_steps, gdouble current_step)
{ p_delta_int(val, *val_from, *val_to, total_steps, current_step); }

static void gp_delta_gint (gint *val, gint *val_from, gint *val_to, gint32 total_steps, gdouble current_step)
{ p_delta_gint(val, *val_from, *val_to, total_steps, current_step); }

static void gp_delta_guint (guint *val, guint *val_from, guint *val_to, gint32 total_steps, gdouble current_step)
{ p_delta_guint(val, *val_from, *val_to, total_steps, current_step); }

static void gp_delta_gint32 (gint32 *val, gint32 *val_from, gint32 *val_to, gint32 total_steps, gdouble current_step)
{ p_delta_gint32(val, *val_from, *val_to, total_steps, current_step); }

static void gp_delta_guint32 (guint32 *val, guint32 *val_from, guint32 *val_to, gint32 total_steps, gdouble current_step)
{ p_delta_guint32(val, *val_from, *val_to, total_steps, current_step); }

static void gp_delta_char (char *val, char *val_from, char *val_to, gint32 total_steps, gdouble current_step)
{ p_delta_char(val, *val_from, *val_to, total_steps, current_step); }

static void gp_delta_guchar (guchar *val, char *val_from, char *val_to, gint32 total_steps, gdouble current_step)
{ p_delta_guchar(val, *val_from, *val_to, total_steps, current_step); }

static void gp_delta_gdouble (double *val, double *val_from, double *val_to, gint32 total_steps, gdouble current_step)
{ p_delta_gdouble(val, *val_from, *val_to, total_steps, current_step); }

static void gp_delta_gfloat (gfloat *val, gfloat *val_from, gfloat *val_to, gint32 total_steps, gdouble current_step)
{ p_delta_gfloat(val, *val_from, *val_to, total_steps, current_step); }

static void gp_delta_float (float *val, float *val_from, float *val_to, gint32 total_steps, gdouble current_step)
{ p_delta_float(val, *val_from, *val_to, total_steps, current_step); }

static void gp_delta_drawable (gint32 *val, gint32 *val_from, gint32 *val_to, gint32 total_steps, gdouble current_step)
{ p_delta_drawable(val, *val_from, *val_to, total_steps, current_step); }

static void gp_delta_gintdrawable (gint *val, gint *val_from, gint *val_to, gint32 total_steps, gdouble current_step)
{ p_delta_gintdrawable(val, *val_from, *val_to, total_steps, current_step); }


/* wrapper calls of type specific procedure */

void gap_delta_none (void *val, void *val_from, void *val_to, gint32 total_steps, gdouble current_step)
{  return; /* this is just a dummy delta procedure for non-taerable members in a LAST_VALUES buffer */ }

void gap_delta_long (void *val, void *val_from, void *val_to, gint32 total_steps, gdouble current_step)
{  gp_delta_long (val, val_from, val_to, total_steps, current_step); }

void gap_delta_short (void *val, void *val_from, void *val_to, gint32 total_steps, gdouble current_step)
{  gp_delta_short (val, val_from, val_to, total_steps, current_step); }

void gap_delta_int (void *val, void *val_from, void *val_to, gint32 total_steps, gdouble current_step)
{  gp_delta_int (val, val_from, val_to, total_steps, current_step); }

void gap_delta_gint (void *val, void *val_from, void *val_to, gint32 total_steps, gdouble current_step)
{  gp_delta_gint (val, val_from, val_to, total_steps, current_step); }

void gap_delta_guint (void *val, void *val_from, void *val_to, gint32 total_steps, gdouble current_step)
{  gp_delta_guint (val, val_from, val_to, total_steps, current_step); }

void gap_delta_gint32 (void *val, void *val_from, void *val_to, gint32 total_steps, gdouble current_step)
{  gp_delta_gint32 (val, val_from, val_to, total_steps, current_step); }

void gap_delta_guint32 (void *val, void *val_from, void *val_to, gint32 total_steps, gdouble current_step)
{  gp_delta_guint32 (val, val_from, val_to, total_steps, current_step); }

void gap_delta_char (void *val, void *val_from, void *val_to, gint32 total_steps, gdouble current_step)
{  gp_delta_char (val, val_from, val_to, total_steps, current_step); }

void gap_delta_guchar (void *val, void *val_from, void *val_to, gint32 total_steps, gdouble current_step)
{  gp_delta_guchar (val, val_from, val_to, total_steps, current_step); }

void gap_delta_gdouble (void *val, void *val_from, void *val_to, gint32 total_steps, gdouble current_step)
{  gp_delta_gdouble (val, val_from, val_to, total_steps, current_step); }

void gap_delta_gfloat (void *val, void *val_from, void *val_to, gint32 total_steps, gdouble current_step)
{  gp_delta_gfloat (val, val_from, val_to, total_steps, current_step); }

void gap_delta_float (void *val, void *val_from, void *val_to, gint32 total_steps, gdouble current_step)
{  gp_delta_float (val, val_from, val_to, total_steps, current_step); }

void gap_delta_drawable (void *val, void *val_from, void *val_to, gint32 total_steps, gdouble current_step)
{  gp_delta_drawable (val, val_from, val_to, total_steps, current_step); }

void gap_delta_gintdrawable (void *val, void *val_from, void *val_to, gint32 total_steps, gdouble current_step)
{  gp_delta_gintdrawable (val, val_from, val_to, total_steps, current_step); }




void
p_init_iter_jump_table(void)
{
  static gboolean  jmp_table_initialized = FALSE;

  if(jmp_table_initialized != TRUE)
  {
    /* fuction pointers for typespecific delta procedures */
    jmp_table[GIMP_LASTVAL_NONE].func_ptr = gap_delta_none;
    jmp_table[GIMP_LASTVAL_ARRAY].func_ptr = gap_delta_none;
    jmp_table[GIMP_LASTVAL_STRUCT_BEGIN].func_ptr = gap_delta_none;
    jmp_table[GIMP_LASTVAL_STRUCT_END].func_ptr = gap_delta_none;

    jmp_table[GIMP_LASTVAL_LONG].func_ptr = gap_delta_long;
    jmp_table[GIMP_LASTVAL_SHORT].func_ptr = gap_delta_short;
    jmp_table[GIMP_LASTVAL_INT].func_ptr = gap_delta_int;
    jmp_table[GIMP_LASTVAL_GINT].func_ptr = gap_delta_gint;
    jmp_table[GIMP_LASTVAL_GINT32].func_ptr = gap_delta_gint32;
    jmp_table[GIMP_LASTVAL_CHAR].func_ptr = gap_delta_char;
    jmp_table[GIMP_LASTVAL_GCHAR].func_ptr = gap_delta_char;           /* gchar uses same as  char */
    jmp_table[GIMP_LASTVAL_GUCHAR].func_ptr = gap_delta_guchar;
    jmp_table[GIMP_LASTVAL_GDOUBLE].func_ptr = gap_delta_gdouble;
    jmp_table[GIMP_LASTVAL_GFLOAT].func_ptr = gap_delta_gfloat;
    jmp_table[GIMP_LASTVAL_DOUBLE].func_ptr = gap_delta_gdouble;     /* double same as gdouble */
    jmp_table[GIMP_LASTVAL_FLOAT].func_ptr = gap_delta_float;
    jmp_table[GIMP_LASTVAL_DRAWABLE].func_ptr = gap_delta_drawable;
    jmp_table[GIMP_LASTVAL_GINTDRAWABLE].func_ptr = gap_delta_gintdrawable;
    jmp_table[GIMP_LASTVAL_GBOOLEAN].func_ptr = gap_delta_none;
    jmp_table[GIMP_LASTVAL_ENUM].func_ptr = gap_delta_none;
    jmp_table[GIMP_LASTVAL_GUINT].func_ptr = gap_delta_guint;
    jmp_table[GIMP_LASTVAL_GUINT32].func_ptr = gap_delta_guint32;

     /* size of one element for the supported basetypes and predefined structs */
    jmp_table[GIMP_LASTVAL_NONE].item_size = 0;
    jmp_table[GIMP_LASTVAL_ARRAY].item_size = 0;
    jmp_table[GIMP_LASTVAL_STRUCT_BEGIN].item_size = 0;
    jmp_table[GIMP_LASTVAL_STRUCT_END].item_size = 0;

    jmp_table[GIMP_LASTVAL_LONG].item_size = sizeof(long);
    jmp_table[GIMP_LASTVAL_SHORT].item_size = sizeof(short);
    jmp_table[GIMP_LASTVAL_INT].item_size = sizeof(int);
    jmp_table[GIMP_LASTVAL_GINT].item_size = sizeof(gint);
    jmp_table[GIMP_LASTVAL_CHAR].item_size = sizeof(char);
    jmp_table[GIMP_LASTVAL_GCHAR].item_size = sizeof(gchar);
    jmp_table[GIMP_LASTVAL_GUCHAR].item_size = sizeof(guchar);
    jmp_table[GIMP_LASTVAL_GDOUBLE].item_size = sizeof(gdouble);
    jmp_table[GIMP_LASTVAL_GFLOAT].item_size = sizeof(gfloat);
    jmp_table[GIMP_LASTVAL_FLOAT].item_size = sizeof(float);
    jmp_table[GIMP_LASTVAL_DOUBLE].item_size = sizeof(double);
    jmp_table[GIMP_LASTVAL_DRAWABLE].item_size = sizeof(gint32);
    jmp_table[GIMP_LASTVAL_GINTDRAWABLE].item_size = sizeof(gint);
    jmp_table[GIMP_LASTVAL_GBOOLEAN].item_size = sizeof(gboolean);
    jmp_table[GIMP_LASTVAL_ENUM].item_size = sizeof(gint);
    jmp_table[GIMP_LASTVAL_GUINT].item_size = sizeof(guint);
    jmp_table[GIMP_LASTVAL_GUINT32].item_size = sizeof(guint32);

    jmp_table_initialized = TRUE;
  }
}



static gint32
p_stack_offsetsum(IterStackType *iter_stack)
{
  gint32 l_arr_fact;
  gint32 l_offset_sum;
  gint32 l_idx;

  l_arr_fact = 0;
  l_offset_sum = 0;

  for(l_idx=0; l_idx <= iter_stack->stack_top; l_idx++)
  {
    if(iter_stack->stack_arr[l_idx].lastval_type == GIMP_LASTVAL_ARRAY)
    {
       if(l_arr_fact == 0)
       {
         l_arr_fact = iter_stack->stack_arr[l_idx].arr_count;
       }
       else
       {
         l_arr_fact = l_arr_fact * iter_stack->stack_arr[l_idx].arr_count;
       }
    }
    else
    {
       /* GIMP_LASTVAL_STRUCT_BEGIN */
       l_offset_sum += (l_arr_fact * iter_stack->stack_arr[l_idx].elem_size);
       l_arr_fact = 0;
    }
  }
  return(l_offset_sum);
}


static void
p_debug_stackprint(IterStackType *iter_stack)
{
  gint l_idx;

  printf("  p_debug_stackprint stack_top: %d  max_elem: %d\n", (int)iter_stack->stack_top, (int)iter_stack->stack_max_elem);
  for(l_idx=0; l_idx <= iter_stack->stack_top; l_idx++)
  {
    printf("  stack[%02d] lastval_type: %d elem_size: %d arr_count: %d idx: %d\n"
          ,(int)l_idx
          ,(int)iter_stack->stack_arr[l_idx].lastval_type
          ,(int)iter_stack->stack_arr[l_idx].elem_size
          ,(int)iter_stack->stack_arr[l_idx].arr_count
          ,(int)iter_stack->stack_arr[l_idx].idx
          );
  }
}

static void
p_push_iter(IterStackType *iter_stack, IterStackItemType *stack_item)
{
  iter_stack->stack_top++;
  if(iter_stack->stack_top < iter_stack->stack_max_elem)
  {
    iter_stack->stack_arr[iter_stack->stack_top].lastval_type = stack_item->lastval_type;
    iter_stack->stack_arr[iter_stack->stack_top].elem_size = stack_item->elem_size;
    iter_stack->stack_arr[iter_stack->stack_top].arr_count = stack_item->arr_count;
    iter_stack->stack_arr[iter_stack->stack_top].idx       = stack_item->idx;
  }
  else
  {
    printf("p_push_iter: STACK overflow\n");
  }
  if(1==0 /*gap_debug*/ )
  {
     printf("p_push_iter START\n");
     p_debug_stackprint(iter_stack);
  }
}


static void
p_pop_iter(IterStackType *iter_stack, IterStackItemType *stack_item)
{
  if(1==0 /*gap_debug*/ )
  {
     printf("p_pop_iter START\n");
     p_debug_stackprint(iter_stack);
  }
  if(iter_stack->stack_top >= 0)
  {
    stack_item->lastval_type = iter_stack->stack_arr[iter_stack->stack_top].lastval_type;
    stack_item->elem_size = iter_stack->stack_arr[iter_stack->stack_top].elem_size;
    stack_item->arr_count = iter_stack->stack_arr[iter_stack->stack_top].arr_count;
    stack_item->idx       = iter_stack->stack_arr[iter_stack->stack_top].idx;
    iter_stack->stack_top--;
  }
  else
  {
    printf("p_pop_iter: STACK underrun\n");
  }
}


void
p_debug_print_iter_desc (GimpLastvalDescType *lastval_desc_arr, gint32 arg_cnt)
{
  gint32 l_idx;

  printf ("p_debug_print_iter_desc START\n");
  for(l_idx = 0; l_idx < arg_cnt; l_idx++)
  {
     printf("[%03d] lastval_type: %d, offset: %d, elem_size: %d\n"
           , (int)l_idx
           , (int)lastval_desc_arr[l_idx].lastval_type
           , (int)lastval_desc_arr[l_idx].offset
           , (int)lastval_desc_arr[l_idx].elem_size
           );
  }
  printf ("p_debug_print_iter_desc END\n\n");
}


/* ----------------------------------------------------------------------
 * run common iterator
 *  PDB: registered as "plug_in_gap_COMMON_ITERATOR"
 * ----------------------------------------------------------------------
 */

gint
gap_common_iterator(const char *c_keyname, GimpRunMode run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct)
{
   gchar *keyname;
   gchar *key_description;
   gchar *key_from;
   gchar *key_to;
   int buf_size;
   int desc_size;

   keyname = g_strdup(c_keyname);
   if(gap_debug) printf("\ngap_common_iterator START: keyname: %s  total_steps: %d,  curent_step: %f\n", keyname, (int)total_steps, (float)current_step );

   key_description = gimp_lastval_desc_keyname (keyname);
   key_from = g_strdup_printf("%s_ITER_FROM", keyname);
   key_to   = g_strdup_printf("%s_ITER_TO", keyname);

   buf_size = gimp_get_data_size(keyname);
   desc_size = gimp_get_data_size(key_description);
   if (buf_size > 0 && desc_size > 0)
   {
      GimpLastvalDescType *lastval_desc_arr;
      gint         arg_cnt;
      gint         l_idx;
      gint         l_idx_next;
      void  *buffer;
      void  *buffer_from;
      void  *buffer_to;

      IterStackType      *stack_iter;
      IterStackItemType  l_stack_item;
      IterStackItemType  l_stack_2_item;

      lastval_desc_arr = g_malloc(desc_size);
      arg_cnt = desc_size / sizeof(GimpLastvalDescType);

      buffer      = g_malloc(buf_size);
      buffer_from = g_malloc(buf_size);
      buffer_to   = g_malloc(buf_size);

      gimp_get_data(key_description, lastval_desc_arr);
      gimp_get_data(keyname,  buffer);
      gimp_get_data(key_from, buffer_from);
      gimp_get_data(key_to,   buffer_to);

      if(gap_debug)
      {
         p_debug_print_iter_desc(lastval_desc_arr, arg_cnt);
      }


      if ((lastval_desc_arr[0].elem_size  != buf_size)
      ||  (lastval_desc_arr[0].elem_size  != len_struct))
      {
         printf("ERROR: %s stored Data missmatch in size %d != %d\n"
                       , keyname
                       , (int)buf_size
                       , (int)lastval_desc_arr[0].elem_size );
         return -1 ; /* NOT OK */
      }

      p_init_iter_jump_table();

      /* init IterStack */
      stack_iter = g_new(IterStackType, 1);
      stack_iter->stack_arr = g_new(IterStackItemType, arg_cnt);
      stack_iter->stack_top = -1;  /* stack is empty */
      stack_iter->stack_max_elem = arg_cnt;  /* stack is empty */

      l_idx=0;
      while(l_idx < arg_cnt)
      {
         void  *buf_ptr;
         void  *buf_ptr_from;
         void  *buf_ptr_to;
         gint   l_iter_idx;
         gint32 l_iter_flag;
         gint32 l_offset;
         gint32 l_ntimes_arraysize;

         if(lastval_desc_arr[l_idx].lastval_type == GIMP_LASTVAL_END)
         {
           break;
         }

         l_iter_idx = (int)lastval_desc_arr[l_idx].lastval_type;
         l_iter_flag = lastval_desc_arr[l_idx].iter_flag;
         l_idx_next = l_idx +1;

         l_stack_item.lastval_type = lastval_desc_arr[l_idx].lastval_type;
         l_stack_item.iter_flag = lastval_desc_arr[l_idx].iter_flag;
         l_stack_item.elem_size = lastval_desc_arr[l_idx].elem_size;
         l_stack_item.arr_count = 0;
         l_stack_item.idx = l_idx_next;

         switch(lastval_desc_arr[l_idx].lastval_type)
         {
           case GIMP_LASTVAL_ARRAY:
             p_push_iter(stack_iter, &l_stack_item);
             break;
           case GIMP_LASTVAL_STRUCT_BEGIN:
             p_push_iter(stack_iter, &l_stack_item);
             break;
           case GIMP_LASTVAL_STRUCT_END:
             l_stack_2_item.lastval_type = GIMP_LASTVAL_NONE;
             if(stack_iter->stack_top >= 0)
             {
               /* 1.st pop should always retrieve the STUCTURE BEGIN */
               p_pop_iter(stack_iter, &l_stack_2_item);
             }
             if(l_stack_2_item.lastval_type != GIMP_LASTVAL_STRUCT_BEGIN)
             {
               printf("stack out of balance, STRUCTURE END without begin\n");
               return -1; /* ERROR */
             }
             if (stack_iter->stack_top >= 0)
             {
                /* 2.nd pop */
                p_pop_iter(stack_iter, &l_stack_item);
                if(l_stack_item.lastval_type == GIMP_LASTVAL_ARRAY)
                {
                  l_stack_item.elem_size--;
                  if(l_stack_item.elem_size > 0)
                  {
                    p_push_iter(stack_iter, &l_stack_item);
                    p_push_iter(stack_iter, &l_stack_2_item);
                    /*  go back to stored index (1st elem of the structure) */
                    l_idx_next = l_stack_2_item.idx;
                  }
                }
                else
                {
                  p_push_iter(stack_iter, &l_stack_item);
                }
             }
             break;
           default:
             l_ntimes_arraysize = 1;
             while(1==1)
             {
               if (stack_iter->stack_top >= 0)
               {
                  p_pop_iter(stack_iter, &l_stack_item);
                  if(l_stack_item.lastval_type == GIMP_LASTVAL_ARRAY)
                  {
                    l_ntimes_arraysize = l_ntimes_arraysize * l_stack_item.elem_size;
                  }
                  else
                  {
                    p_push_iter(stack_iter, &l_stack_item);
                    break;
                  }
               }
             }

             l_offset = p_stack_offsetsum(stack_iter);            /* offest for current array position */
             l_offset += lastval_desc_arr[l_idx].offset;   /* local offest */

             buf_ptr      = buffer      + l_offset;
             buf_ptr_from = buffer_from + l_offset;
             buf_ptr_to   = buffer_to   + l_offset;


             if(gap_debug) printf("Using JumpTable jmp index:%d iter_flag:%d\n", (int)l_iter_idx, (int)l_iter_flag);

             if (l_iter_idx >= 0
             &&  l_iter_idx < (gint)GIMP_LASTVAL_END
             &&  (l_iter_flag & GIMP_ITER_TRUE) == GIMP_ITER_TRUE  )
             {
               gint32 l_cnt;

               for(l_cnt=0; l_cnt < l_ntimes_arraysize; l_cnt++)
               {
                 /* call type specific iterator delta procedure using jmp_table indexed by lastval_type */
                 jmp_table[l_iter_idx].func_ptr(buf_ptr, buf_ptr_from, buf_ptr_to, total_steps, current_step);
                 buf_ptr      += jmp_table[l_iter_idx].item_size;
                 buf_ptr_from += jmp_table[l_iter_idx].item_size;
                 buf_ptr_to   += jmp_table[l_iter_idx].item_size;
               }
             }
             break;
         }

         l_idx = l_idx_next;
      }

      gimp_set_data(keyname, buffer, buf_size);
      g_free(stack_iter->stack_arr);
      g_free(stack_iter);
   }
   else
   {
         printf("ERROR: %s No stored Data/Description found. Datasize: %d, Descriptionsize: %d\n",
                       keyname, (int)buf_size, (int)desc_size );
         return -1 ; /* NOT OK */
   }
   g_free(keyname);
   return 0; /* OK */
}  /* end gap_common_iterator */










/* ----------------------------------------------------------------------
 * iterator UTILITIES for GimpRGB, GimpVector3, Material and Light Sewttings
 * ----------------------------------------------------------------------
 */

    typedef enum {
      POINT_LIGHT,
      DIRECTIONAL_LIGHT,
      SPOT_LIGHT,
      NO_LIGHT
    } t_LightType;

    typedef enum {
      IMAGE_BUMP,
      WAVES_BUMP
    } t_MapType;

    typedef struct
    {
      gdouble ambient_int;
      gdouble diffuse_int;
      gdouble diffuse_ref;
      gdouble specular_ref;
      gdouble highlight;
      GimpRGB  color;
    } t_MaterialSettings;

    typedef struct
    {
      t_LightType  type;
      GimpVector3  position;
      GimpVector3  direction;
      GimpRGB     color;
      gdouble    intensity;
    } t_LightSettings;



static void p_delta_GimpRGB(GimpRGB *val, GimpRGB *val_from, GimpRGB *val_to, gint32 total_steps, gdouble current_step)
{
    double     delta;

    if(total_steps < 1) return;

    delta = ((double)(val_to->r - val_from->r) / (double)total_steps) * ((double)total_steps - current_step);
    val->r = val_from->r + delta;

    delta = ((double)(val_to->g - val_from->g) / (double)total_steps) * ((double)total_steps - current_step);
    val->g = val_from->g + delta;

    delta = ((double)(val_to->b - val_from->b) / (double)total_steps) * ((double)total_steps - current_step);
    val->b = val_from->b + delta;

    delta = ((double)(val_to->a - val_from->a) / (double)total_steps) * ((double)total_steps - current_step);
    val->a = val_from->a + delta;
}
static void p_delta_GimpVector3(GimpVector3 *val, GimpVector3 *val_from, GimpVector3 *val_to, gint32 total_steps, gdouble current_step)
{
    double     delta;

    if(total_steps < 1) return;

    delta = ((double)(val_to->x - val_from->x) / (double)total_steps) * ((double)total_steps - current_step);
    val->x  = val_from->x + delta;

    delta = ((double)(val_to->y - val_from->y) / (double)total_steps) * ((double)total_steps - current_step);
    val->y  = val_from->y + delta;

    delta = ((double)(val_to->z - val_from->z) / (double)total_steps) * ((double)total_steps - current_step);
    val->z  = val_from->z + delta;
}

static void p_delta_MaterialSettings(t_MaterialSettings *val, t_MaterialSettings *val_from, t_MaterialSettings *val_to, gint32 total_steps, gdouble current_step)
{
    p_delta_gdouble(&val->ambient_int, val_from->ambient_int, val_to->ambient_int, total_steps, current_step);
    p_delta_gdouble(&val->diffuse_int, val_from->diffuse_int, val_to->diffuse_int, total_steps, current_step);
    p_delta_gdouble(&val->diffuse_ref, val_from->diffuse_ref, val_to->diffuse_ref, total_steps, current_step);
    p_delta_gdouble(&val->specular_ref, val_from->specular_ref, val_to->specular_ref, total_steps, current_step);
    p_delta_gdouble(&val->highlight, val_from->highlight, val_to->highlight, total_steps, current_step);
    p_delta_GimpRGB(&val->color, &val_from->color, &val_to->color, total_steps, current_step);

}

static void p_delta_LightSettings(t_LightSettings *val, t_LightSettings *val_from, t_LightSettings *val_to, gint32 total_steps, gdouble current_step)
{
    /* no delta is done for LightType */
    p_delta_GimpVector3(&val->position, &val_from->position, &val_to->position, total_steps, current_step);
    p_delta_GimpVector3(&val->direction, &val_from->direction, &val_to->direction, total_steps, current_step);
    p_delta_GimpRGB(&val->color, &val_from->color, &val_to->color, total_steps, current_step);
    p_delta_gdouble(&val->intensity, val_from->intensity, val_to->intensity, total_steps, current_step);
}



/* ---------------------------------------- 2.nd Section
 * ----------------------------------------
 * INCLUDE the generated p_XXX_iter_ALT procedures
 * ----------------------------------------
 * ----------------------------------------
 */

#include "iter_ALT/mod/plug_in_Twist_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_alienmap_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_applylens_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_blur_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_bump_map_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_convmatrix_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_depth_merge_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_despeckle_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_emboss_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_exchange_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_flame_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_lighting_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_map_object_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_maze_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_nlfilt_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_nova_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_oilify_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_pagecurl_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_papertile_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_plasma_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_polar_coords_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_sample_colorize_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_sinus_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_solid_noise_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_sparkle_iter_ALT.inc"

#include "iter_ALT/mod/plug_in_alienmap2_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_apply_canvas_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_colortoalpha_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_deinterlace_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_illusion_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_lic_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_make_seamless_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_max_rgb_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_normalize_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_sel_gauss_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_small_tiles_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_sobel_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_unsharp_mask_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_vinvert_iter_ALT.inc"




#include "iter_ALT/old/plug_in_CentralReflection_iter_ALT.inc"
#include "iter_ALT/old/plug_in_anamorphose_iter_ALT.inc"
#include "iter_ALT/old/plug_in_blur2_iter_ALT.inc"
#include "iter_ALT/old/plug_in_encript_iter_ALT.inc"
#include "iter_ALT/old/plug_in_figures_iter_ALT.inc"
#include "iter_ALT/old/plug_in_gflare_iter_ALT.inc"
#include "iter_ALT/old/plug_in_holes_iter_ALT.inc"
#include "iter_ALT/old/plug_in_julia_iter_ALT.inc"
#include "iter_ALT/old/plug_in_magic_eye_iter_ALT.inc"
#include "iter_ALT/old/plug_in_mandelbrot_iter_ALT.inc"
#include "iter_ALT/old/plug_in_randomize_iter_ALT.inc"
#include "iter_ALT/old/plug_in_refract_iter_ALT.inc"
#include "iter_ALT/old/plug_in_struc_iter_ALT.inc"
#include "iter_ALT/old/plug_in_tileit_iter_ALT.inc"
#include "iter_ALT/old/plug_in_universal_filter_iter_ALT.inc"
#include "iter_ALT/old/plug_in_warp_iter_ALT.inc"


#include "iter_ALT/gen/plug_in_CML_explorer_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_alpha2color_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_blinds_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_borderaverage_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_checkerboard_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_color_map_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_colorify_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_cubism_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_destripe_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_diffraction_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_displace_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_edge_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_engrave_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_flarefx_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_fractal_trace_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_gauss_iir_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_gauss_iir2_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_gauss_rle_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_gauss_rle2_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_gfig_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_glasstile_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_grid_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_jigsaw_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_mblur_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_mosaic_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_newsprint_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_noisify_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_pixelize_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_randomize_hurl_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_randomize_pick_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_randomize_slur_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_ripple_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_rotate_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_scatter_hsv_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_sharpen_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_shift_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_spread_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_video_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_vpropagate_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_waves_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_whirl_pinch_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_wind_iter_ALT.inc"

/* table of proc_names and funtion pointers to iter_ALT procedures */
/* del ... Deleted (does not make sense to animate)
 * +   ... generated code did not work (changed manually)
 */
static t_iter_ALT_tab   g_iter_ALT_tab[] =
{
/*  { "Colorify",  p_Colorify_iter_ALT }                                          */
/*, { "perl_fu_blowinout",  p_perl_fu_blowinout_iter_ALT }                        */
/*, { "perl_fu_feedback",  p_perl_fu_feedback_iter_ALT }                          */
/*, { "perl_fu_prep4gif",  p_perl_fu_prep4gif_iter_ALT }                          */
/*, { "perl_fu_scratches",  p_perl_fu_scratches_iter_ALT }                        */
/*, { "perl_fu_terraltext",  p_perl_fu_terraltext_iter_ALT }                      */
/*, { "perl_fu_tex_string_to_float",  p_perl_fu_tex_string_to_float_iter_ALT }    */
/*, { "perl_fu_webify",  p_perl_fu_webify_iter_ALT }                              */
/*, { "perl_fu_windify",  p_perl_fu_windify_iter_ALT }                            */
/*, { "perl_fu_xach_blocks",  p_perl_fu_xach_blocks_iter_ALT }                    */
/*, { "perl_fu_xach_shadows",  p_perl_fu_xach_shadows_iter_ALT }                  */
/*, { "perl_fu_xachvision",  p_perl_fu_xachvision_iter_ALT }                      */
    { "plug_in_CML_explorer",  p_plug_in_CML_explorer_iter_ALT }
  , { "plug_in_CentralReflection",  p_plug_in_CentralReflection_iter_ALT }
  , { "plug_in_Twist",  p_plug_in_Twist_iter_ALT }
  , { "plug_in_alienmap",  p_plug_in_alienmap_iter_ALT }
/*, { "plug_in_align_layers",  p_plug_in_align_layers_iter_ALT }                  */
  , { "plug_in_alpha2color",  p_plug_in_alpha2color_iter_ALT }
  , { "plug_in_anamorphose",  p_plug_in_anamorphose_iter_ALT }
/*, { "plug_in_animationoptimize",  p_plug_in_animationoptimize_iter_ALT }        */
/*, { "plug_in_animationplay",  p_plug_in_animationplay_iter_ALT }                */
/*, { "plug_in_animationunoptimize",  p_plug_in_animationunoptimize_iter_ALT }    */
/*, { "plug_in_apply_canvas",  p_plug_in_apply_canvas_iter_ALT }                  */
  , { "plug_in_applylens",  p_plug_in_applylens_iter_ALT }
/*, { "plug_in_autocrop",  p_plug_in_autocrop_iter_ALT }                          */
/*, { "plug_in_autostretch_hsv",  p_plug_in_autostretch_hsv_iter_ALT }            */
  , { "plug_in_blinds",  p_plug_in_blinds_iter_ALT }
  , { "plug_in_blur",  p_plug_in_blur_iter_ALT }
  , { "plug_in_blur2",  p_plug_in_blur2_iter_ALT }
/*, { "plug_in_blur_randomize",  p_plug_in_blur_randomize_iter_ALT }              */
  , { "plug_in_borderaverage",  p_plug_in_borderaverage_iter_ALT }
  , { "plug_in_bump_map",  p_plug_in_bump_map_iter_ALT }
/*, { "plug_in_c_astretch",  p_plug_in_c_astretch_iter_ALT }                      */
  , { "plug_in_checkerboard",  p_plug_in_checkerboard_iter_ALT }
/*, { "plug_in_color_adjust",  p_plug_in_color_adjust_iter_ALT }                  */
  , { "plug_in_color_map",  p_plug_in_color_map_iter_ALT }
  , { "plug_in_colorify",  p_plug_in_colorify_iter_ALT }
/*, { "plug_in_compose",  p_plug_in_compose_iter_ALT }                            */
  , { "plug_in_convmatrix",  p_plug_in_convmatrix_iter_ALT }
  , { "plug_in_cubism",  p_plug_in_cubism_iter_ALT }
/*, { "plug_in_decompose",  p_plug_in_decompose_iter_ALT }                        */
/*, { "plug_in_deinterlace",  p_plug_in_deinterlace_iter_ALT }                    */
  , { "plug_in_depth_merge",  p_plug_in_depth_merge_iter_ALT }
  , { "plug_in_despeckle",  p_plug_in_despeckle_iter_ALT }
  , { "plug_in_destripe",  p_plug_in_destripe_iter_ALT }
  , { "plug_in_diffraction",  p_plug_in_diffraction_iter_ALT }
  , { "plug_in_displace",  p_plug_in_displace_iter_ALT }
/*, { "plug_in_ditherize",  p_plug_in_ditherize_iter_ALT }                        */
  , { "plug_in_edge",  p_plug_in_edge_iter_ALT }
  , { "plug_in_emboss",  p_plug_in_emboss_iter_ALT }
  , { "plug_in_encript",  p_plug_in_encript_iter_ALT }
  , { "plug_in_engrave",  p_plug_in_engrave_iter_ALT }
  , { "plug_in_exchange",  p_plug_in_exchange_iter_ALT }
/*, { "plug_in_export_palette",  p_plug_in_export_palette_iter_ALT }              */
  , { "plug_in_figures",  p_plug_in_figures_iter_ALT }
/*, { "plug_in_film",  p_plug_in_film_iter_ALT }                                  */
/*, { "plug_in_filter_pack",  p_plug_in_filter_pack_iter_ALT }                    */
  , { "plug_in_flame",  p_plug_in_flame_iter_ALT }
  , { "plug_in_flarefx",  p_plug_in_flarefx_iter_ALT }
  , { "plug_in_fractal_trace",  p_plug_in_fractal_trace_iter_ALT }
  , { "plug_in_gauss_iir",  p_plug_in_gauss_iir_iter_ALT }
  , { "plug_in_gauss_iir2",  p_plug_in_gauss_iir2_iter_ALT }
  , { "plug_in_gauss_rle",  p_plug_in_gauss_rle_iter_ALT }
  , { "plug_in_gauss_rle2",  p_plug_in_gauss_rle2_iter_ALT }
  , { "plug_in_gfig",  p_plug_in_gfig_iter_ALT }
  , { "plug_in_gflare",  p_plug_in_gflare_iter_ALT }
  , { "plug_in_glasstile",  p_plug_in_glasstile_iter_ALT }
/*, { "plug_in_gradmap",  p_plug_in_gradmap_iter_ALT }                            */
  , { "plug_in_grid",  p_plug_in_grid_iter_ALT }
/*, { "plug_in_guillotine",  p_plug_in_guillotine_iter_ALT }                      */
  , { "plug_in_holes",  p_plug_in_holes_iter_ALT }
/*, { "plug_in_hot",  p_plug_in_hot_iter_ALT }                                    */
/*, { "plug_in_ifs_compose",  p_plug_in_ifs_compose_iter_ALT }                    */
/*, { "plug_in_illusion",  p_plug_in_illusion_iter_ALT }                          */
/*, { "plug_in_image_rot270",  p_plug_in_image_rot270_iter_ALT }                  */
/*, { "plug_in_image_rot90",  p_plug_in_image_rot90_iter_ALT }                    */
/*, { "plug_in_iwarp",  p_plug_in_iwarp_iter_ALT }                                */
  , { "plug_in_jigsaw",  p_plug_in_jigsaw_iter_ALT }
  , { "plug_in_julia",  p_plug_in_julia_iter_ALT }
/*, { "plug_in_laplace",  p_plug_in_laplace_iter_ALT }                            */
/*, { "plug_in_layer_rot270",  p_plug_in_layer_rot270_iter_ALT }                  */
/*, { "plug_in_layer_rot90",  p_plug_in_layer_rot90_iter_ALT }                    */
/*, { "plug_in_layers_import",  p_plug_in_layers_import_iter_ALT }                */
/*, { "plug_in_lic",  p_plug_in_lic_iter_ALT }                                    */
  , { "plug_in_lighting",  p_plug_in_lighting_iter_ALT }
  , { "plug_in_magic_eye",  p_plug_in_magic_eye_iter_ALT }
/*, { "plug_in_mail_image",  p_plug_in_mail_image_iter_ALT }                      */
/*, { "plug_in_make_seamless",  p_plug_in_make_seamless_iter_ALT }                */
  , { "plug_in_mandelbrot",  p_plug_in_mandelbrot_iter_ALT }
  , { "plug_in_map_object",  p_plug_in_map_object_iter_ALT }
/*, { "plug_in_max_rgb",  p_plug_in_max_rgb_iter_ALT }                            */
  , { "plug_in_maze",  p_plug_in_maze_iter_ALT }
  , { "plug_in_mblur",  p_plug_in_mblur_iter_ALT }
  , { "plug_in_mosaic",  p_plug_in_mosaic_iter_ALT }
  , { "plug_in_newsprint",  p_plug_in_newsprint_iter_ALT }
  , { "plug_in_nlfilt",  p_plug_in_nlfilt_iter_ALT }
  , { "plug_in_noisify",  p_plug_in_noisify_iter_ALT }
/*, { "plug_in_normalize",  p_plug_in_normalize_iter_ALT }                        */
  , { "plug_in_nova",  p_plug_in_nova_iter_ALT }
  , { "plug_in_oilify",  p_plug_in_oilify_iter_ALT }
  , { "plug_in_pagecurl",  p_plug_in_pagecurl_iter_ALT }
  , { "plug_in_papertile",  p_plug_in_papertile_iter_ALT }
  , { "plug_in_pixelize",  p_plug_in_pixelize_iter_ALT }
  , { "plug_in_plasma",  p_plug_in_plasma_iter_ALT }
  , { "plug_in_polar_coords",  p_plug_in_polar_coords_iter_ALT }
/*, { "plug_in_qbist",  p_plug_in_qbist_iter_ALT }                                */
  , { "plug_in_randomize",  p_plug_in_randomize_iter_ALT }
  , { "plug_in_randomize_hurl",  p_plug_in_randomize_hurl_iter_ALT }
  , { "plug_in_randomize_pick",  p_plug_in_randomize_pick_iter_ALT }
  , { "plug_in_randomize_slur",  p_plug_in_randomize_slur_iter_ALT }
  , { "plug_in_refract",  p_plug_in_refract_iter_ALT }
  , { "plug_in_ripple",  p_plug_in_ripple_iter_ALT }
  , { "plug_in_rotate",  p_plug_in_rotate_iter_ALT }
  , { "plug_in_sample_colorize",  p_plug_in_sample_colorize_iter_ALT }
  , { "plug_in_scatter_hsv",  p_plug_in_scatter_hsv_iter_ALT }
/*, { "plug_in_semiflatten",  p_plug_in_semiflatten_iter_ALT }                    */
  , { "plug_in_sharpen",  p_plug_in_sharpen_iter_ALT }
  , { "plug_in_shift",  p_plug_in_shift_iter_ALT }
  , { "plug_in_sinus",  p_plug_in_sinus_iter_ALT }
/*, { "plug_in_small_tiles",  p_plug_in_small_tiles_iter_ALT }                    */
/*, { "plug_in_smooth_palette",  p_plug_in_smooth_palette_iter_ALT }              */
/*, { "plug_in_sobel",  p_plug_in_sobel_iter_ALT }                                */
  , { "plug_in_solid_noise",  p_plug_in_solid_noise_iter_ALT }
  , { "plug_in_sparkle",  p_plug_in_sparkle_iter_ALT }
  , { "plug_in_spread",  p_plug_in_spread_iter_ALT }
  , { "plug_in_struc",  p_plug_in_struc_iter_ALT }
/*, { "plug_in_the_egg",  p_plug_in_the_egg_iter_ALT }                            */
/*, { "plug_in_threshold_alpha",  p_plug_in_threshold_alpha_iter_ALT }            */
/*, { "plug_in_tile",  p_plug_in_tile_iter_ALT }                                  */
  , { "plug_in_tileit",  p_plug_in_tileit_iter_ALT }
  , { "plug_in_universal_filter",  p_plug_in_universal_filter_iter_ALT }
  , { "plug_in_video",  p_plug_in_video_iter_ALT }
/*, { "plug_in_vinvert",  p_plug_in_vinvert_iter_ALT }                            */
  , { "plug_in_vpropagate",  p_plug_in_vpropagate_iter_ALT }
  , { "plug_in_warp",  p_plug_in_warp_iter_ALT }
  , { "plug_in_waves",  p_plug_in_waves_iter_ALT }
  , { "plug_in_whirl_pinch",  p_plug_in_whirl_pinch_iter_ALT }
  , { "plug_in_wind",  p_plug_in_wind_iter_ALT }
/*, { "plug_in_zealouscrop",  p_plug_in_zealouscrop_iter_ALT }                    */

  , { "plug_in_alienmap2", p_plug_in_alienmap2_iter_ALT }
  , { "plug_in_apply_canvas", p_plug_in_apply_canvas_iter_ALT }
  , { "plug_in_colortoalpha", p_plug_in_colortoalpha_iter_ALT }
  , { "plug_in_deinterlace", p_plug_in_deinterlace_iter_ALT }
  , { "plug_in_illusion", p_plug_in_illusion_iter_ALT }
  , { "plug_in_lic", p_plug_in_lic_iter_ALT }
  , { "plug_in_make_seamless", p_plug_in_make_seamless_iter_ALT }
  , { "plug_in_max_rgb", p_plug_in_max_rgb_iter_ALT }
  , { "plug_in_normalize", p_plug_in_normalize_iter_ALT }
  , { "plug_in_sel_gauss", p_plug_in_sel_gauss_iter_ALT }
  , { "plug_in_small_tiles", p_plug_in_small_tiles_iter_ALT }
  , { "plug_in_sobel", p_plug_in_sobel_iter_ALT }
  , { "plug_in_unsharp_mask", p_plug_in_unsharp_mask_iter_ALT }
  , { "plug_in_vinvert", p_plug_in_vinvert_iter_ALT }
};  /* end g_iter_ALT_tab */


#define MAX_ITER_ALT ( sizeof(g_iter_ALT_tab) / sizeof(t_iter_ALT_tab) )


/* ----------------------------------------------------------------------
 * install (query) iterators_ALT
 * ----------------------------------------------------------------------
 */

static void p_install_proc_iter_ALT(char *name)
{
  gchar *l_iter_proc_name;
  gchar *l_blurb_text;

  static GimpParamDef args_iter[] =
  {
    {GIMP_PDB_INT32, "run_mode", "non-interactive"},
    {GIMP_PDB_INT32, "total_steps", "total number of steps (# of layers-1 to apply the related plug-in)"},
    {GIMP_PDB_FLOAT, "current_step", "current (for linear iterations this is the layerstack position, otherwise some value inbetween)"},
    {GIMP_PDB_INT32, "len_struct", "length of stored data structure with id is equal to the plug_in  proc_name"},
  };

  static GimpParamDef *return_vals = NULL;
  static int nreturn_vals = 0;

  l_iter_proc_name = g_strdup_printf("%s_Iterator_ALT", name);
  l_blurb_text = g_strdup_printf("This procedure calculates the modified values for one iterationstep for the call of %s", name);

  gimp_install_procedure(l_iter_proc_name,
			 l_blurb_text,
			 "",
			 "Wolfgang Hofer",
			 "Wolfgang Hofer",
			 "Feb. 2000",
			 NULL,    /* do not appear in menus */
			 NULL,
			 GIMP_PLUGIN,
			 G_N_ELEMENTS (args_iter), nreturn_vals,
			 args_iter, return_vals);

  g_free(l_iter_proc_name);
  g_free(l_blurb_text);
}


void
gap_query_iterators_ALT()
{
  guint l_idx;
  for(l_idx = 0; l_idx < MAX_ITER_ALT; l_idx++)
  {
     p_install_proc_iter_ALT (g_iter_ALT_tab[l_idx].proc_name);
  }
}

/* ----------------------------------------------------------------------
 * run iterators_ALT
 * ----------------------------------------------------------------------
 */

gint
gap_run_iterators_ALT(const char *name, GimpRunMode run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct)
{
  gint l_rc;
  guint l_idx;
  char *l_name;
  int   l_cut;

  l_name = g_strdup(name);
  l_cut  = strlen(l_name) - strlen("_Iterator_ALT");
  if(l_cut < 1)
  {
     fprintf(stderr, "ERROR: gap_run_iterators_ALT: proc_name ending _Iterator_ALT missing%s\n", name);
     return -1;
  }
  if(strcmp(&l_name[l_cut], "_Iterator_ALT") != 0)
  {
     fprintf(stderr, "ERROR: gap_run_iterators_ALT: proc_name ending _Iterator_ALT missing%s\n", name);
     return -1;
  }


  l_rc = -1;
  l_name[l_cut] = '\0';  /* cut off "_Iterator_ALT" from l_name end */

  /* allocate from/to plugin_data buffers
   * as big as needed for the current plugin named l_name
   */
  g_plugin_data_from = p_alloc_plugin_data(l_name);
  g_plugin_data_to = p_alloc_plugin_data(l_name);

  if((g_plugin_data_from != NULL)
  && (g_plugin_data_to != NULL))
  {
    for(l_idx = 0; l_idx < MAX_ITER_ALT; l_idx++)
    {
        if (strcmp (l_name, g_iter_ALT_tab[l_idx].proc_name) == 0)
        {
          if(gap_debug) printf("DEBUG: gap_run_iterators_ALT: FOUND %s\n", l_name);
          l_rc =  (g_iter_ALT_tab[l_idx].proc_func)(run_mode, total_steps, current_step, len_struct);
        }
    }
  }

  if(l_rc < 0) fprintf(stderr, "ERROR: gap_run_iterators_ALT: NOT FOUND proc_name=%s (%s)\n", name, l_name);

  /* free from/to plugin_data buffers */
  if(g_plugin_data_from) g_free(g_plugin_data_from);
  if(g_plugin_data_to)   g_free(g_plugin_data_to);

  return l_rc;
}  /* end gap_run_iterators_ALT */

