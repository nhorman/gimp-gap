/* gap_filter_iterators.c
 *
 * 1998.01.29 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * This Module contains:
 * - Implementation of XXX_Iterator_ALT Procedures 
 *     for those Plugins  of the gimp.0.99.17 release
 *     - that can operate on a single drawable,
 *     - and have paramters for varying.
 *
 * for now i made some Iterator Plugins using the ending _ALT,
 * If New plugins were added to the gimp, or existing ones were updated,
 * the Authors should supply original _Iterator Procedures
 * (without the _ALT ending)
 * This Procedures are then used instead of my (Hacked _ALT) versions.
 * to modify the settings for the plugin when called step by step
 * on animated multilayer Images.
 * without name conflicts.
 *
 * The 2.nd section of this file was generated by gap_filter_codegen.c:p_gen_code_iter_ALT
 * using the PDB at version gimp 0.99.18 as base.
 * Unforunately, some of the plugins are using datastructures
 * to store their "Last Values"
 * that are differnt from its calling parameters (as described in the PDB)
 * Therfore I had to adjust (edit by hand) the generted structures to fit the
 * plugins internal settings.
 *
 * Common things to all Iteratur Plugins:
 * Interface:   run_mode        # is always RUN_NONINTERACTIVE
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
 *      "_Iterator"        or
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
 * 1999.03.14 hof: added iterators for gimp 1.1.3 prerelease
 *                 iterator code reorganized in _iter_ALT.inc Files
 * 1998.06.12 hof: added p_delta_drawable (Iterate layers in the layerstack)
 *                 this enables to apply an animated bumpmap.
 * 1998.01.29 hof: 1st release
 */
 
/* SYTEM (UNIX) includes */ 
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/* GIMP includes */
#include "gtk/gtk.h"
#include "libgimp/gimp.h"

/* GAP includes */
#include "gap_filter.h"
#include "gap_filter_iterators.h"


static char g_plugin_data_from[PLUGIN_DATA_SIZE + 1];
static char g_plugin_data_to[PLUGIN_DATA_SIZE + 1];




extern int gap_debug;

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

    if(gap_debug) fprintf(stderr, "DEBUG: p_delta_long from: %ld to: %ld curr: %ld    delta: %f\n",
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
    
   if(gap_debug) fprintf(stderr, "DEBUG: p_delta_gdouble total: %d  from: %f to: %f curr: %f    delta: %f\n",
                                  (int)total_steps, val_from, val_to, *val, delta);
}
static void p_delta_gfloat(gfloat *val, gfloat val_from, gfloat val_to, gint32 total_steps, gdouble current_step)
{
    double     delta;
 
    if(total_steps < 1) return;
 
    delta = ((double)(val_to - val_from) / (double)total_steps) * ((double)total_steps - current_step);
    *val  = val_from + delta;
     
    if(gap_debug) fprintf(stderr, "DEBUG: p_delta_gfloat total: %d  from: %f to: %f curr: %f    delta: %f\n",
                                   (int)total_steps, val_from, val_to, *val, delta);
}

static void p_delta_float(float *val, float val_from, float val_to, gint32 total_steps, gdouble current_step)
{
    double     delta;

    if(total_steps < 1) return;

    delta = ((double)(val_to - val_from) / (double)total_steps) * ((double)total_steps - current_step);
    *val  = val_from + delta;
    
    if(gap_debug) fprintf(stderr, "DEBUG: p_delta_gdouble total: %d  from: %f to: %f curr: %f    delta: %f\n",
                                  (int)total_steps, val_from, val_to, *val, delta);
}
static void p_delta_color(t_color *val, t_color *val_from, t_color *val_to, gint32 total_steps, gdouble current_step)
{
    double     delta;
    int l_idx;

    if(total_steps < 1) return;
    
    for(l_idx = 0; l_idx < 3; l_idx++)
    {
       delta = ((double)(val_to->color[l_idx] - val_from->color[l_idx]) / (double)total_steps) * ((double)total_steps - current_step);
       val->color[l_idx]  = val_from->color[l_idx] + delta; 
    }
}
static void p_delta_gint_color(t_gint_color *val, t_gint_color *val_from, t_gint_color *val_to, gint32 total_steps, gdouble current_step)
{
    double     delta;
    int l_idx;

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

    l_tmp_image_id = gimp_drawable_image_id(val_from);

    /* check if from and to values are both valid drawables within the same image */
    if ((l_tmp_image_id > 0) 
    &&  (l_tmp_image_id = gimp_drawable_image_id(val_to)))
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


/* ----------------------------------------------------------------------
 * iterator UTILITIES for Gck Vectors, Material and Light Sewttings
 * ----------------------------------------------------------------------
 */

    typedef struct
    {
      double color[4];  /* r,g,b,a */
    } t_GckRGB;
    typedef struct
    {
      double  coord[3]; /* x,y,z; */
    } t_GckVector3;

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
      t_GckRGB  color;
    } t_MaterialSettings;

    typedef struct
    {
      t_LightType  type;
      t_GckVector3 position;
      t_GckVector3 direction;
      t_GckRGB     color;
      gdouble    intensity;
    } t_LightSettings;
    

static void p_delta_GckRGB(t_GckRGB *val, t_GckRGB *val_from, t_GckRGB *val_to, gint32 total_steps, gdouble current_step)
{
    double     delta;
    int l_idx;

    if(total_steps < 1) return;
    
    for(l_idx = 0; l_idx < 4; l_idx++)
    {
       delta = ((double)(val_to->color[l_idx] - val_from->color[l_idx]) / (double)total_steps) * ((double)total_steps - current_step);
       val->color[l_idx]  = val_from->color[l_idx] + delta; 
    }
}
static void p_delta_GckVector3(t_GckVector3 *val, t_GckVector3 *val_from, t_GckVector3 *val_to, gint32 total_steps, gdouble current_step)
{
    double     delta;
    int l_idx;

    if(total_steps < 1) return;
    
    for(l_idx = 0; l_idx < 3; l_idx++)
    {
       delta = ((double)(val_to->coord[l_idx] - val_from->coord[l_idx]) / (double)total_steps) * ((double)total_steps - current_step);
       val->coord[l_idx]  = val_from->coord[l_idx] + delta; 
    }
}

static void p_delta_MaterialSettings(t_MaterialSettings *val, t_MaterialSettings *val_from, t_MaterialSettings *val_to, gint32 total_steps, gdouble current_step)
{
    p_delta_gdouble(&val->ambient_int, val_from->ambient_int, val_to->ambient_int, total_steps, current_step);
    p_delta_gdouble(&val->diffuse_int, val_from->diffuse_int, val_to->diffuse_int, total_steps, current_step);
    p_delta_gdouble(&val->diffuse_ref, val_from->diffuse_ref, val_to->diffuse_ref, total_steps, current_step);
    p_delta_gdouble(&val->specular_ref, val_from->specular_ref, val_to->specular_ref, total_steps, current_step);
    p_delta_gdouble(&val->highlight, val_from->highlight, val_to->highlight, total_steps, current_step);
    p_delta_GckRGB(&val->color, &val_from->color, &val_to->color, total_steps, current_step);

}

static void p_delta_LightSettings(t_LightSettings *val, t_LightSettings *val_from, t_LightSettings *val_to, gint32 total_steps, gdouble current_step)
{
    /* no delta is done for LightType */
    p_delta_GckVector3(&val->position, &val_from->position, &val_to->position, total_steps, current_step);
    p_delta_GckVector3(&val->direction, &val_from->direction, &val_to->direction, total_steps, current_step);
    p_delta_GckRGB(&val->color, &val_from->color, &val_to->color, total_steps, current_step);
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
#include "iter_ALT/mod/plug_in_convmatrix_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_depth_merge_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_despeckle_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_emboss_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_exchange_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_flame_iter_ALT.inc
#include "iter_ALT/mod/plug_in_lighting_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_map_object_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_maze_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_nlfilt_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_nova_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_oilify_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_pagecurl_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_plasma_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_polar_coords_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_sample_colorize_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_sinus_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_solid_noise_iter_ALT.inc"
#include "iter_ALT/mod/plug_in_sparkle_iter_ALT.inc"

#include "iter_ALT/old/Colorify_iter_ALT.inc"
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
#include "iter_ALT/gen/plug_in_bump_map_iter_ALT.inc"
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
#include "iter_ALT/gen/plug_in_gauss_rle_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_gfig_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_glasstile_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_grid_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_jigsaw_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_mblur_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_mosaic_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_newsprint_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_noisify_iter_ALT.inc"
#include "iter_ALT/gen/plug_in_paper_tile_iter_ALT.inc"
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
    { "Colorify",  p_Colorify_iter_ALT }
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
  , { "plug_in_CML_explorer",  p_plug_in_CML_explorer_iter_ALT }
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
/*, { "plug_in_colorify",  p_plug_in_colorify_iter_ALT }                          */
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
  , { "plug_in_gauss_rle",  p_plug_in_gauss_rle_iter_ALT }
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
  , { "plug_in_paper_tile",  p_plug_in_paper_tile_iter_ALT }
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
};  /* end g_iter_ALT_tab */


#define MAX_ITER_ALT ( sizeof(g_iter_ALT_tab) / sizeof(t_iter_ALT_tab) )


/* ---------------------------------------------------------------------- 
 * install (query) iterators_ALT
 * ---------------------------------------------------------------------- 
 */

static void p_install_proc_iter_ALT(char *name)
{
  char l_iter_proc_name[256];
  char l_blurb_text[300];

  static GParamDef args_iter[] =
  {
    {PARAM_INT32, "run_mode", "non-interactive"},
    {PARAM_INT32, "total_steps", "total number of steps (# of layers-1 to apply the related plug-in)"},
    {PARAM_FLOAT, "current_step", "current (for linear iterations this is the layerstack position, otherwise some value inbetween)"},
    {PARAM_INT32, "len_struct", "length of stored data structure with id is equal to the plug_in  proc_name"},
  };
  static int nargs_iter = sizeof(args_iter) / sizeof(args_iter[0]);

  static GParamDef *return_vals = NULL;
  static int nreturn_vals = 0;

  sprintf(l_iter_proc_name, "%s_Iterator_ALT", name);
  sprintf(l_blurb_text, "This extension calculates the modified values for one iterationstep for the call of %s", name);
  
  gimp_install_procedure(l_iter_proc_name,
			 l_blurb_text,
			 "",
			 "Wolfgang Hofer",
			 "Wolfgang Hofer",
			 "Dec. 1997",
			 NULL,    /* do not appear in menus */
			 NULL,
			 PROC_EXTENSION,
			 nargs_iter, nreturn_vals,
			 args_iter, return_vals);
  
}

void gap_query_iterators_ALT()
{
  int l_idx;
  for(l_idx = 0; l_idx < MAX_ITER_ALT; l_idx++)
  {
     p_install_proc_iter_ALT (g_iter_ALT_tab[l_idx].proc_name);
  }
}

/* ---------------------------------------------------------------------- 
 * run iterators_ALT
 * ---------------------------------------------------------------------- 
 */

gint gap_run_iterators_ALT(char *name, GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct)
{
  gint l_rc;
  int l_idx;
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


  l_name[l_cut] = '\0';  /* cut off "_Iterator_ALT" from l_name end */
  l_rc = -1;
  for(l_idx = 0; l_idx < MAX_ITER_ALT; l_idx++)
  {
      if (strcmp (l_name, g_iter_ALT_tab[l_idx].proc_name) == 0)
      {
        if(gap_debug) fprintf(stderr, "DEBUG: gap_run_iterators_ALT: FOUND %s\n", l_name);
        l_rc =  (g_iter_ALT_tab[l_idx].proc_func)(run_mode, total_steps, current_step, len_struct);
      }
  }
  
  if(l_rc < 0) fprintf(stderr, "ERROR: gap_run_iterators_ALT: NOT FOUND proc_name=%s (%s)\n", name, l_name);

  return l_rc;
}
