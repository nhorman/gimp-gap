## Process this file with automake to produce Makefile.in

libexecdir = $(GIMP_PLUGIN_DIR)/plug-ins

scriptdatadir = $(GIMP_DATA_DIR)/scripts

scriptdata_DATA = sel-to-anim-img.scm

libexec_PROGRAMS = \
	gap_plugins		\
	gap_filter		\
	gap_frontends		\
	gap_name2layer		\
	gap_navigator_dialog	\
        gap_onion


gap_plugins_SOURCES = \
	gap-intl.h		\
	gap_arr_dialog.c	\
	gap_arr_dialog.h	\
	gap_base_ops.c	\
	gap_base_ops.h	\
	gap_dbbrowser_utils.c	\
	gap_dbbrowser_utils.h	\
	gap_filter_codegen.c	\
	gap_filter_pdb.c	\
	gap_filter_pdb.h	\
	gap_layer_copy.c	\
	gap_layer_copy.h 	\
	gap_lastvaldesc.c	\
	gap_lastvaldesc.h	\
	gap_lib.c		\
	gap_lib.h		\
	gap_lock.c		\
	gap_lock.h		\
	gap_main.c		\
	gap_match.c		\
	gap_match.h		\
	gap_mod_layer.c		\
	gap_mod_layer.h		\
	gap_mov_dialog.c	\
	gap_mov_dialog.h	\
	gap_mov_exec.c		\
	gap_mov_exec.h		\
	gap_mov_render.c	\
	gap_mov_render.h	\
	gap_navi_activtable.c	\
	gap_navi_activtable.h	\
	gap_pdb_calls.c		\
	gap_pdb_calls.h		\
	gap_range_ops.c		\
	gap_range_ops.h		\
	gap_resi_dialog.c	\
	gap_resi_dialog.h	\
	gap_split.c		\
	gap_split.h		\
	gap_thumbnail.c		\
	gap_thumbnail.h		\
	gap_vin.c		\
	gap_vin.h

gap_filter_SOURCES = \
	gap_arr_dialog.c	\
	gap_arr_dialog.h	\
	gap_dbbrowser_utils.c	\
	gap_dbbrowser_utils.h	\
	gap_filter.h		\
	gap_filter_codegen.c	\
	gap_filter_foreach.c	\
	gap_filter_iterators.c	\
	gap_filter_iterators.h	\
	gap_filter_main.c	\
	gap_filter_pdb.c	\
	gap_filter_pdb.h	\
	gap_lastvaldesc.c	\
	gap_lastvaldesc.h	\
	gap_pdb_calls.c		\
	gap_pdb_calls.h		\
	gap_vin.c		\
	gap_vin.h

gap_frontends_SOURCES = \
	gap_arr_dialog.c	\
	gap_arr_dialog.h	\
	gap_decode_xanim.c	\
	gap_decode_xanim.h	\
	gap_frontends_main.c	\
	gap_layer_copy.c	\
	gap_layer_copy.h 	\
	gap_lib.c		\
	gap_lib.h		\
	gap_lock.c		\
	gap_lock.h		\
	gap_match.c		\
	gap_match.h		\
	gap_mpege.c		\
	gap_mpege.h		\
	gap_navi_activtable.c	\
	gap_navi_activtable.h	\
	gap_pdb_calls.c		\
	gap_pdb_calls.h		\
	gap_thumbnail.c		\
	gap_thumbnail.h		\
	gap_vin.c		\
	gap_vin.h

gap_name2layer_SOURCES = \
	gap_arr_dialog.c	\
	gap_arr_dialog.h	\
	gap_lastvaldesc.c	\
	gap_lastvaldesc.h	\
	gap_name2layer_main.c

gap_navigator_dialog_SOURCES = \
	gap_arr_dialog.c	\
	gap_arr_dialog.h	\
	gap_layer_copy.c        \
	gap_layer_copy.h        \
	gap_lib.c		\
	gap_lib.h		\
	gap_navi_activtable.c	\
	gap_navi_activtable.h	\
	gap_navigator_dialog.c	\
	gap_pdb_calls.c		\
	gap_pdb_calls.h		\
	gap_thumbnail.c		\
	gap_thumbnail.h		\
	gap_vin.c		\
	gap_vin.h

gap_onion_SOURCES = \
	gap_arr_dialog.c	\
	gap_arr_dialog.h	\
	gap_onion_main.c	\
	gap_onion_main.h	\
	gap_onion_dialog.c	\
	gap_onion_dialog.h	\
	gap_onion_worker.c	\
	gap_onion_worker.h	\
	gap_layer_copy.c        \
	gap_layer_copy.h        \
	gap_lib.c		\
	gap_lib.h		\
	gap_lock.c		\
	gap_lock.h		\
	gap_navi_activtable.c	\
	gap_navi_activtable.h	\
	gap_match.c		\
	gap_match.h		\
	gap_pdb_calls.c		\
	gap_pdb_calls.h		\
	gap_thumbnail.c		\
	gap_thumbnail.h		\
	gap_vin.c		\
	gap_vin.h

AM_CPPFLAGS = \
	-DLOCALEDIR=\""$(LOCALEDIR)"\"

INCLUDES = \
	-I$(top_srcdir)	\
	$(GIMP_CFLAGS)	\
	-I$(includedir)

LDADD = \
	$(GIMP_LIBS)


EXTRA_DIST = \
	README							\
	README_developers					\
	TESTPROT_iter_ALT					\
	iter_ALT/README_iter_subdirs				\
	iter_ALT/gen/plug_in_CML_explorer_iter_ALT.inc		\
	iter_ALT/gen/plug_in_alpha2color_iter_ALT.inc		\
	iter_ALT/gen/plug_in_blinds_iter_ALT.inc		\
	iter_ALT/gen/plug_in_borderaverage_iter_ALT.inc		\
	iter_ALT/gen/plug_in_checkerboard_iter_ALT.inc		\
	iter_ALT/gen/plug_in_color_map_iter_ALT.inc		\
	iter_ALT/gen/plug_in_colorify_iter_ALT.inc		\
	iter_ALT/gen/plug_in_cubism_iter_ALT.inc		\
	iter_ALT/gen/plug_in_destripe_iter_ALT.inc		\
	iter_ALT/gen/plug_in_diffraction_iter_ALT.inc		\
	iter_ALT/gen/plug_in_displace_iter_ALT.inc		\
	iter_ALT/gen/plug_in_edge_iter_ALT.inc			\
	iter_ALT/gen/plug_in_engrave_iter_ALT.inc		\
	iter_ALT/gen/plug_in_flarefx_iter_ALT.inc		\
	iter_ALT/gen/plug_in_fractal_trace_iter_ALT.inc		\
	iter_ALT/gen/plug_in_gauss_iir_iter_ALT.inc		\
	iter_ALT/gen/plug_in_gauss_iir2_iter_ALT.inc		\
	iter_ALT/gen/plug_in_gauss_rle_iter_ALT.inc		\
	iter_ALT/gen/plug_in_gauss_rle2_iter_ALT.inc		\
	iter_ALT/gen/plug_in_gfig_iter_ALT.inc			\
	iter_ALT/gen/plug_in_glasstile_iter_ALT.inc		\
	iter_ALT/gen/plug_in_grid_iter_ALT.inc			\
	iter_ALT/gen/plug_in_jigsaw_iter_ALT.inc		\
	iter_ALT/gen/plug_in_mblur_iter_ALT.inc			\
	iter_ALT/gen/plug_in_mosaic_iter_ALT.inc		\
	iter_ALT/gen/plug_in_newsprint_iter_ALT.inc		\
	iter_ALT/gen/plug_in_noisify_iter_ALT.inc		\
	iter_ALT/gen/plug_in_pixelize_iter_ALT.inc		\
	iter_ALT/gen/plug_in_randomize_hurl_iter_ALT.inc	\
	iter_ALT/gen/plug_in_randomize_pick_iter_ALT.inc	\
	iter_ALT/gen/plug_in_randomize_slur_iter_ALT.inc	\
	iter_ALT/gen/plug_in_ripple_iter_ALT.inc		\
	iter_ALT/gen/plug_in_rotate_iter_ALT.inc		\
	iter_ALT/gen/plug_in_scatter_hsv_iter_ALT.inc		\
	iter_ALT/gen/plug_in_sharpen_iter_ALT.inc		\
	iter_ALT/gen/plug_in_shift_iter_ALT.inc			\
	iter_ALT/gen/plug_in_spread_iter_ALT.inc		\
	iter_ALT/gen/plug_in_video_iter_ALT.inc			\
	iter_ALT/gen/plug_in_vpropagate_iter_ALT.inc		\
	iter_ALT/gen/plug_in_waves_iter_ALT.inc			\
	iter_ALT/gen/plug_in_whirl_pinch_iter_ALT.inc		\
	iter_ALT/gen/plug_in_wind_iter_ALT.inc			\
	iter_ALT/mod/plug_in_Twist_iter_ALT.inc			\
	iter_ALT/mod/plug_in_alienmap_iter_ALT.inc		\
	iter_ALT/mod/plug_in_applylens_iter_ALT.inc		\
	iter_ALT/mod/plug_in_blur_iter_ALT.inc			\
	iter_ALT/mod/plug_in_bump_map_iter_ALT.inc		\
	iter_ALT/mod/plug_in_convmatrix_iter_ALT.inc		\
	iter_ALT/mod/plug_in_depth_merge_iter_ALT.inc		\
	iter_ALT/mod/plug_in_despeckle_iter_ALT.inc		\
	iter_ALT/mod/plug_in_emboss_iter_ALT.inc		\
	iter_ALT/mod/plug_in_exchange_iter_ALT.inc		\
	iter_ALT/mod/plug_in_flame_iter_ALT.inc			\
	iter_ALT/mod/plug_in_lighting_iter_ALT.inc		\
	iter_ALT/mod/plug_in_map_object_iter_ALT.inc		\
	iter_ALT/mod/plug_in_maze_iter_ALT.inc			\
	iter_ALT/mod/plug_in_nlfilt_iter_ALT.inc		\
	iter_ALT/mod/plug_in_nova_iter_ALT.inc			\
	iter_ALT/mod/plug_in_oilify_iter_ALT.inc		\
	iter_ALT/mod/plug_in_pagecurl_iter_ALT.inc		\
	iter_ALT/mod/plug_in_papertile_iter_ALT.inc		\
	iter_ALT/mod/plug_in_plasma_iter_ALT.inc		\
	iter_ALT/mod/plug_in_polar_coords_iter_ALT.inc		\
	iter_ALT/mod/plug_in_sample_colorize_iter_ALT.inc	\
	iter_ALT/mod/plug_in_sinus_iter_ALT.inc			\
	iter_ALT/mod/plug_in_solid_noise_iter_ALT.inc		\
	iter_ALT/mod/plug_in_sparkle_iter_ALT.inc		\
	iter_ALT/old/plug_in_CentralReflection_iter_ALT.inc	\
	iter_ALT/old/plug_in_anamorphose_iter_ALT.inc		\
	iter_ALT/old/plug_in_blur2_iter_ALT.inc			\
	iter_ALT/old/plug_in_colorify_iter_ALT.inc		\
	iter_ALT/old/plug_in_encript_iter_ALT.inc		\
	iter_ALT/old/plug_in_figures_iter_ALT.inc		\
	iter_ALT/old/plug_in_gflare_iter_ALT.inc		\
	iter_ALT/old/plug_in_holes_iter_ALT.inc			\
	iter_ALT/old/plug_in_julia_iter_ALT.inc			\
	iter_ALT/old/plug_in_magic_eye_iter_ALT.inc		\
	iter_ALT/old/plug_in_mandelbrot_iter_ALT.inc		\
	iter_ALT/old/plug_in_randomize_iter_ALT.inc		\
	iter_ALT/old/plug_in_refract_iter_ALT.inc		\
	iter_ALT/old/plug_in_struc_iter_ALT.inc			\
	iter_ALT/old/plug_in_tileit_iter_ALT.inc		\
	iter_ALT/old/plug_in_universal_filter_iter_ALT.inc	\
	iter_ALT/old/plug_in_warp_iter_ALT.inc			\
	$(scriptdata_DATA)
