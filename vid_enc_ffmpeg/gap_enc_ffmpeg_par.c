/*  gap_enc_ffmpeg_par.c
 *
 *  This module handles GAP ffmpeg videoencoder parameter files
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
 * version 2.1.0a;  2005/03/24  hof: created (based on procedures of the gap_val_file module)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <errno.h>

/* GIMP includes */
#include "libgimp/gimp.h"

/* GAP includes */
#include "config.h"
#include "gap_libgimpgap.h"
#include "gap_enc_ffmpeg_main.h"
#include "gap_enc_ffmpeg_par.h"

extern int gap_debug;


/* --------------------------
 * p_set_keyword_bool32
 * --------------------------
 * helper procedure to set keyword for flags of type GAP_VAL_G32BOOLEAN.
 */
static void
p_set_keyword_bool32(GapValKeyList *keylist, const char *key, void *addr, const char *comment)
{
   gap_val_set_keyword(keylist, key, addr,  GAP_VAL_G32BOOLEAN, 0, comment);
}


/* --------------------------
 * p_set_master_keywords
 * --------------------------
 * set keywords for the ffmpeg encoder preset fileformat and assign
 * adresspointers to the relevant parameter, datatype (how to scan value when parameter are read from file)
 * and comment string (that is added to the parameter when saved to file)
 */
static void
p_set_master_keywords(GapValKeyList *keylist, GapGveFFMpegValues *epp)
{

   gap_val_set_keyword(keylist, "(format_name ",   &epp->format_name[0], GAP_VAL_STRING, sizeof(epp->format_name), "\0");
   gap_val_set_keyword(keylist, "(vcodec_name ",   &epp->vcodec_name[0], GAP_VAL_STRING, sizeof(epp->vcodec_name), "\0");
   gap_val_set_keyword(keylist, "(acodec_name ",   &epp->acodec_name[0], GAP_VAL_STRING, sizeof(epp->acodec_name), "\0");
   gap_val_set_keyword(keylist, "(title ",         &epp->title[0],       GAP_VAL_STRING, sizeof(epp->title), "\0");
   gap_val_set_keyword(keylist, "(author ",        &epp->author[0],      GAP_VAL_STRING, sizeof(epp->author), "\0");
   gap_val_set_keyword(keylist, "(copyright ",     &epp->copyright[0],   GAP_VAL_STRING, sizeof(epp->copyright), "\0");
   gap_val_set_keyword(keylist, "(comment ",       &epp->comment[0],     GAP_VAL_STRING, sizeof(epp->comment), "\0");
   gap_val_set_keyword(keylist, "(passlogfile ",   &epp->passlogfile[0], GAP_VAL_STRING, sizeof(epp->passlogfile), "\0");

   gap_val_set_keyword(keylist, "(pass ",          &epp->pass_nr,        GAP_VAL_GINT32, 0, "# 1: one pass encoding, 2: for two-pass encoding");
   gap_val_set_keyword(keylist, "(audio_bitrate ", &epp->audio_bitrate,  GAP_VAL_GINT32, 0, "# audio bitrate in kBit/sec");
   gap_val_set_keyword(keylist, "(video_bitrate ", &epp->video_bitrate,  GAP_VAL_GINT32, 0, "# video bitrate in kBit/sec\0");
   gap_val_set_keyword(keylist, "(gop_size ",      &epp->gop_size,       GAP_VAL_GINT32, 0, "# group of picture size");
   gap_val_set_keyword(keylist, "(qscale ",        &epp->qscale,         GAP_VAL_GDOUBLE, 0, "# use fixed video quantiser scale (VBR). 0=const bitrate");
   gap_val_set_keyword(keylist, "(qmin ",          &epp->qmin,           GAP_VAL_GINT32, 0, "# min video quantiser scale (VBR)");
   gap_val_set_keyword(keylist, "(qmax ",          &epp->qmax,           GAP_VAL_GINT32, 0, "# max video quantiser scale (VBR)");
   gap_val_set_keyword(keylist, "(qdiff ",         &epp->qdiff,          GAP_VAL_GINT32, 0, "# max difference between the quantiser scale (VBR)");
   gap_val_set_keyword(keylist, "(qblur ",         &epp->qblur,          GAP_VAL_GDOUBLE, 0, "# video quantiser scale blur (VBR)");
   gap_val_set_keyword(keylist, "(qcomp ",         &epp->qcomp,          GAP_VAL_GDOUBLE, 0, "# video quantiser scale compression (VBR)");
   gap_val_set_keyword(keylist, "(rc_init_cplx ",  &epp->rc_init_cplx,   GAP_VAL_GDOUBLE, 0, "# initial complexity for 1-pass encoding");
   gap_val_set_keyword(keylist, "(b_qfactor ",     &epp->b_qfactor,      GAP_VAL_GDOUBLE, 0, "# qp factor between p and b frames");
   gap_val_set_keyword(keylist, "(i_qfactor ",     &epp->i_qfactor,      GAP_VAL_GDOUBLE, 0, "# qp factor between p and i frames");
   gap_val_set_keyword(keylist, "(b_qoffset ",     &epp->b_qoffset,      GAP_VAL_GDOUBLE, 0, "# qp offset between p and b frames");
   gap_val_set_keyword(keylist, "(i_qoffset ",     &epp->i_qoffset,      GAP_VAL_GDOUBLE, 0, "# qp offset between p and i frames");
   gap_val_set_keyword(keylist, "(bitrate_tol ",   &epp->bitrate_tol,    GAP_VAL_GINT32, 0, "# set video bitrate tolerance (in kbit/s)");
   gap_val_set_keyword(keylist, "(maxrate_tol ",   &epp->maxrate_tol,    GAP_VAL_GINT32, 0, "# set max video bitrate tolerance (in kbit/s)");
   gap_val_set_keyword(keylist, "(minrate_tol ",   &epp->minrate_tol,    GAP_VAL_GINT32, 0, "# set min video bitrate tolerance (in kbit/s)");
   gap_val_set_keyword(keylist, "(bufsize ",       &epp->bufsize,        GAP_VAL_GINT32, 0, "# set ratecontrol buffere size (in kbit)");
   gap_val_set_keyword(keylist, "(motion_estimation ", &epp->motion_estimation,           GAP_VAL_GINT32, 0, "# algorithm for motion estimation (1-6)");
   gap_val_set_keyword(keylist, "(dct_algo ",      &epp->dct_algo,       GAP_VAL_GINT32, 0, "# algorithm for DCT (0-6)");
   gap_val_set_keyword(keylist, "(idct_algo ",     &epp->idct_algo,      GAP_VAL_GINT32, 0, "# algorithm for IDCT (0-11)");
   gap_val_set_keyword(keylist, "(strict ",        &epp->strict,         GAP_VAL_GINT32, 0, "# how strictly to follow the standards");
   gap_val_set_keyword(keylist, "(mb_qmin ",       &epp->mb_qmin,        GAP_VAL_GINT32, 0, "# min macroblock quantiser scale (VBR)");
   gap_val_set_keyword(keylist, "(mb_qmax ",       &epp->mb_qmax,        GAP_VAL_GINT32, 0, "# max macroblock quantiser scale (VBR)");
   gap_val_set_keyword(keylist, "(mb_decision ",   &epp->mb_decision,    GAP_VAL_GINT32, 0, "# algorithm for macroblock decision (0-2)");
   gap_val_set_keyword(keylist, "(b_frames ",      &epp->b_frames,       GAP_VAL_GINT32, 0, "# max number of B-frames in sequence");
   gap_val_set_keyword(keylist, "(packet_size ",   &epp->packet_size,    GAP_VAL_GINT32, 0, "\0");

   gap_val_set_keyword(keylist, "(intra ",                &epp->intra,               GAP_VAL_G32BOOLEAN, 0, "# use only intra frames");
   gap_val_set_keyword(keylist, "(set_aspect_ratio ",     &epp->set_aspect_ratio,    GAP_VAL_G32BOOLEAN, 0, "# store aspectratio information (width/height) in the output video");
   gap_val_set_keyword(keylist, "(factor_aspect_ratio ",  &epp->factor_aspect_ratio, GAP_VAL_GDOUBLE, 0, "# the aspect ratio (width/height). 0.0 = auto ratio assuming sqaure pixels");
   gap_val_set_keyword(keylist, "(dont_recode_flag ",     &epp->dont_recode_flag,    GAP_VAL_GBOOLEAN, 0, "# bypass the video encoder where frames can be copied 1:1");

   /* new params (introduced with ffmpeg 0.4.9pre1) */
   gap_val_set_keyword(keylist, "(thread_count ",         &epp->thread_count,        GAP_VAL_GINT32, 0, "\0");
   gap_val_set_keyword(keylist, "(mb_cmp ",               &epp->mb_cmp,              GAP_VAL_GINT32, 0, "# macroblock compare function (0-13,255)");
   gap_val_set_keyword(keylist, "(ildct_cmp ",            &epp->ildct_cmp,           GAP_VAL_GINT32, 0, "# ildct compare function (0-13,255)");
   gap_val_set_keyword(keylist, "(sub_cmp ",              &epp->sub_cmp,             GAP_VAL_GINT32, 0, "# subpel compare function (0-13,255)");
   gap_val_set_keyword(keylist, "(cmp ",                  &epp->cmp,                 GAP_VAL_GINT32, 0, "# fullpel compare function (0-13,255)");
   gap_val_set_keyword(keylist, "(pre_cmp ",              &epp->pre_cmp,             GAP_VAL_GINT32, 0, "# pre motion estimation compare function (0-13,255)");
   gap_val_set_keyword(keylist, "(pre_me ",               &epp->pre_me,              GAP_VAL_GINT32, 0, "# pre motion estimation");
   gap_val_set_keyword(keylist, "(lumi_mask ",            &epp->lumi_mask,           GAP_VAL_GDOUBLE, 0, "# luminance masking");
   gap_val_set_keyword(keylist, "(dark_mask ",            &epp->dark_mask,           GAP_VAL_GDOUBLE, 0, "# darkness masking");
   gap_val_set_keyword(keylist, "(scplx_mask ",           &epp->scplx_mask,          GAP_VAL_GDOUBLE, 0, "# spatial complexity masking");
   gap_val_set_keyword(keylist, "(tcplx_mask ",           &epp->tcplx_mask,          GAP_VAL_GDOUBLE, 0, "# temporary complexity masking");
   gap_val_set_keyword(keylist, "(p_mask ",               &epp->p_mask,              GAP_VAL_GDOUBLE, 0, "# p block masking");
   gap_val_set_keyword(keylist, "(qns ",                  &epp->qns,                 GAP_VAL_GINT32, 0, "# quantization noise shaping");

   gap_val_set_keyword(keylist, "(video_lmin ",           &epp->video_lmin,          GAP_VAL_GINT32, 0, "# min video lagrange factor (VBR) 0-3658");
   gap_val_set_keyword(keylist, "(video_lmax ",           &epp->video_lmax,          GAP_VAL_GINT32, 0, "# max video lagrange factor (VBR) 0-3658");
   gap_val_set_keyword(keylist, "(video_lelim ",          &epp->video_lelim,         GAP_VAL_GINT32, 0, "# luma elimination threshold");
   gap_val_set_keyword(keylist, "(video_celim ",          &epp->video_celim,         GAP_VAL_GINT32, 0, "# chroma elimination threshold");
   gap_val_set_keyword(keylist, "(video_intra_quant_bias ", &epp->video_intra_quant_bias, GAP_VAL_GINT32, 0, "# intra quant bias");
   gap_val_set_keyword(keylist, "(video_inter_quant_bias ", &epp->video_inter_quant_bias, GAP_VAL_GINT32, 0, "# inter quant bias");
   gap_val_set_keyword(keylist, "(me_threshold ",         &epp->me_threshold,        GAP_VAL_GINT32, 0, "# motion estimaton threshold");
   gap_val_set_keyword(keylist, "(mb_threshold ",         &epp->mb_threshold,        GAP_VAL_GINT32, 0, "# macroblock threshold");
   gap_val_set_keyword(keylist, "(intra_dc_precision ",   &epp->intra_dc_precision,  GAP_VAL_GINT32, 0, "# precision of the intra dc coefficient - 8");
   gap_val_set_keyword(keylist, "(error_rate ",           &epp->error_rate,          GAP_VAL_GINT32, 0, "# simulates errors in the bitstream to test error concealment");
   gap_val_set_keyword(keylist, "(noise_reduction ",      &epp->noise_reduction,     GAP_VAL_GINT32, 0, "# noise reduction strength");
   gap_val_set_keyword(keylist, "(sc_threshold ",         &epp->sc_threshold,        GAP_VAL_GINT32, 0, "# scene change detection threshold");
   gap_val_set_keyword(keylist, "(me_range ",             &epp->me_range,            GAP_VAL_GINT32, 0, "# limit motion vectors range (1023 for DivX player, 0 = no limit)");
   gap_val_set_keyword(keylist, "(coder ",                &epp->coder,               GAP_VAL_GINT32, 0, "# coder type 0=VLC 1=AC");
   gap_val_set_keyword(keylist, "(context ",              &epp->context,             GAP_VAL_GINT32, 0, "# context model");
   gap_val_set_keyword(keylist, "(predictor ",            &epp->predictor,           GAP_VAL_GINT32, 0, "# prediction method (0 LEFT, 1 PLANE, 2 MEDIAN");
   gap_val_set_keyword(keylist, "(nsse_weight ",          &epp->nsse_weight,         GAP_VAL_GINT32, 0, "# noise vs. sse weight for the nsse comparsion function.");
   gap_val_set_keyword(keylist, "(subpel_quality ",       &epp->subpel_quality,      GAP_VAL_GINT32, 0, "# subpel ME quality");

   /* new parms (found CVS version 2005.03.02 == LIBAVCODEC_BUILD 4744 ) */
   gap_val_set_keyword(keylist, "(strict_gop ",           &epp->strict_gop,          GAP_VAL_GINT32, 0, "# strictly enforce GOP size");
   gap_val_set_keyword(keylist, "(no_output ",            &epp->no_output,           GAP_VAL_GINT32, 0, "# skip bitstream encoding");
   gap_val_set_keyword(keylist, "(video_mb_lmin ",        &epp->video_mb_lmin,       GAP_VAL_GINT32, 0, "# min macroblock quantiser scale (VBR) 0-3658");
   gap_val_set_keyword(keylist, "(video_mb_lmax ",        &epp->video_mb_lmax,       GAP_VAL_GINT32, 0, "# max macroblock quantiser scale (VBR) 0-3658");
   gap_val_set_keyword(keylist, "(video_profile ",        &epp->video_profile,       GAP_VAL_GINT32, 0, "\0");
   gap_val_set_keyword(keylist, "(video_level ",          &epp->video_level,         GAP_VAL_GINT32, 0, "\0");
   gap_val_set_keyword(keylist, "(frame_skip_threshold ", &epp->frame_skip_threshold,GAP_VAL_GINT32, 0, "# frame skip threshold");
   gap_val_set_keyword(keylist, "(frame_skip_factor ",    &epp->frame_skip_factor,   GAP_VAL_GINT32, 0, "# frame skip factor");
   gap_val_set_keyword(keylist, "(frame_skip_exp ",       &epp->frame_skip_exp,      GAP_VAL_GINT32, 0, "# frame skip exponent");
   gap_val_set_keyword(keylist, "(frame_skip_cmp ",       &epp->frame_skip_cmp,      GAP_VAL_GINT32, 0, "# frame skip comparission function");
   gap_val_set_keyword(keylist, "(mux_rate ",             &epp->mux_rate,            GAP_VAL_GINT32, 0, "\0");
   gap_val_set_keyword(keylist, "(mux_packet_size ",      &epp->mux_packet_size,     GAP_VAL_GINT32, 0, "\0");

   gap_val_set_keyword(keylist, "(mux_preload ",          &epp->mux_preload,         GAP_VAL_GDOUBLE, 0, "# set the initial demux-decode delay (secs)");
   gap_val_set_keyword(keylist, "(mux_max_delay ",        &epp->mux_max_delay,       GAP_VAL_GDOUBLE, 0, "# set the maximum demux-decode delay (secs)");


   /* new parms SVN snaphot 2009.01.31 */
   gap_val_set_keyword(keylist, "(b_frame_strategy ",          &epp->b_frame_strategy,           GAP_VAL_GINT32, 0, "# strategy to choose between I/P/B-frames");
   gap_val_set_keyword(keylist, "(workaround_bugs ",           &epp->workaround_bugs,            GAP_VAL_GINT32, 0, "# 1:FF_BUG_AUTODETECT, 2:FF_BUG_OLD_MSMPEG4 4:FF_BUG_XVID_ILACE 8:FF_BUG_UMP4 16:FF_BUG_NO_PADDING 32:FF_BUG_AMV 8192:FF_BUG_MS (Work around various bugs in Microsoft's broken decoders)");
   gap_val_set_keyword(keylist, "(error_recognition ",         &epp->error_recognition,          GAP_VAL_GINT32, 0, "# 1:FF_ER_CAREFUL  2:FF_ER_COMPLIANT 3:FF_ER_AGGRESSIVE 4:FF_ER_VERY_AGGRESSIVE");
   gap_val_set_keyword(keylist, "(mpeg_quant ",                &epp->mpeg_quant,                 GAP_VAL_GINT32, 0, "# 0-> h263 quant 1-> mpeg quant");
   gap_val_set_keyword(keylist, "(rc_qsquish ",                &epp->rc_qsquish,                 GAP_VAL_GDOUBLE, 0, "# how to keep quantizer between qmin and qmax (0 = clip, 1 = use differentiable function)");
   gap_val_set_keyword(keylist, "(rc_qmod_amp ",               &epp->rc_qmod_amp,                GAP_VAL_GDOUBLE, 0, "# experimental quantizer modulation");
   gap_val_set_keyword(keylist, "(rc_qmod_freq ",              &epp->rc_qmod_freq,               GAP_VAL_GINT32, 0, "# experimental quantizer modulation");
   gap_val_set_keyword(keylist, "(rc_eq[60] ",                 &epp->rc_eq[60],                  GAP_VAL_STRING,   sizeof(epp->rc_eq), "# rate control equation (tex^qComp)");
   gap_val_set_keyword(keylist, "(rc_buffer_aggressivity ",    &epp->rc_buffer_aggressivity,     GAP_VAL_GDOUBLE, 0, "# currently useless");
   gap_val_set_keyword(keylist, "(dia_size ",                  &epp->dia_size,                   GAP_VAL_GINT32, 0, "# ME diamond size & shape");
   gap_val_set_keyword(keylist, "(last_predictor_count ",      &epp->last_predictor_count,       GAP_VAL_GINT32, 0, "# amount of previous MV predictors (2a+1 x 2a+1 square)");
   gap_val_set_keyword(keylist, "(pre_dia_size ",              &epp->pre_dia_size,               GAP_VAL_GINT32, 0, "# ME prepass diamond size & shape");
   gap_val_set_keyword(keylist, "(inter_threshold ",           &epp->inter_threshold,            GAP_VAL_GINT32, 0, "\0");
   gap_val_set_keyword(keylist, "(border_masking ",            &epp->border_masking,             GAP_VAL_GDOUBLE, 0, "# increases the quantizer for macroblocks close to borders");
   gap_val_set_keyword(keylist, "(me_penalty_compensation ",   &epp->me_penalty_compensation,    GAP_VAL_GINT32, 0, "# motion estimation bitrate penalty compensation (1.0 = 256)");
   gap_val_set_keyword(keylist, "(bidir_refine ",              &epp->bidir_refine,               GAP_VAL_GINT32, 0, "# refine the two motion vectors used in bidirectional macroblocks");
   gap_val_set_keyword(keylist, "(brd_scale ",                 &epp->brd_scale,                  GAP_VAL_GINT32, 0, "# downscales frames for dynamic B-frame decision");
   gap_val_set_keyword(keylist, "(crf ",                       &epp->crf,                        GAP_VAL_GDOUBLE, 0, "# constant rate factor - quality-based VBR - values ~correspond to qps");
   gap_val_set_keyword(keylist, "(cqp ",                       &epp->cqp,                        GAP_VAL_GINT32, 0, "# constant quantization parameter rate control method");
   gap_val_set_keyword(keylist, "(keyint_min ",                &epp->keyint_min,                 GAP_VAL_GINT32, 0, "# minimum GOP size");
   gap_val_set_keyword(keylist, "(refs ",                      &epp->refs,                       GAP_VAL_GINT32, 0, "# number of reference frames");
   gap_val_set_keyword(keylist, "(chromaoffset ",              &epp->chromaoffset,               GAP_VAL_GINT32, 0, "# chroma qp offset from luma");
   gap_val_set_keyword(keylist, "(bframebias ",                &epp->bframebias,                 GAP_VAL_GINT32, 0, "# Influences how often B-frames are used");
   gap_val_set_keyword(keylist, "(trellis ",                   &epp->trellis,                    GAP_VAL_GINT32, 0, "# trellis RD quantization");
   gap_val_set_keyword(keylist, "(complexityblur ",            &epp->complexityblur,             GAP_VAL_GDOUBLE, 0, "# Reduce fluctuations in qp (before curve compression)");
   gap_val_set_keyword(keylist, "(deblockalpha ",              &epp->deblockalpha,               GAP_VAL_GINT32, 0, "# in-loop deblocking filter alphac0 parameter. alpha is in the range -6...6");
   gap_val_set_keyword(keylist, "(deblockbeta ",               &epp->deblockbeta,                GAP_VAL_GINT32, 0, "# in-loop deblocking filter beta parameter. beta is in the range -6...6");
   gap_val_set_keyword(keylist, "(partitions ",                &epp->partitions,                 GAP_VAL_GINT32, 0, "# macroblock subpartition sizes to consider - p8x8, p4x4, b8x8, i8x8, i4x4");
   gap_val_set_keyword(keylist, "(directpred ",                &epp->directpred,                 GAP_VAL_GINT32, 0, "# direct MV prediction mode - 0 (none), 1 (spatial), 2 (temporal), 3 (auto)");
   gap_val_set_keyword(keylist, "(cutoff ",                    &epp->cutoff,                     GAP_VAL_GINT32, 0, "# Audio cutoff bandwidth (0 means automatic)");
   gap_val_set_keyword(keylist, "(scenechange_factor ",        &epp->scenechange_factor,         GAP_VAL_GINT32, 0, "#  Multiplied by qscale for each frame and added to scene_change_score.");
   gap_val_set_keyword(keylist, "(mv0_threshold ",             &epp->mv0_threshold,              GAP_VAL_GINT32, 0, "# Value depends upon the compare function used for fullpel ME");
   gap_val_set_keyword(keylist, "(b_sensitivity ",             &epp->b_sensitivity,              GAP_VAL_GINT32, 0, "# Adjusts sensitivity of b_frame_strategy 1");
   gap_val_set_keyword(keylist, "(compression_level ",         &epp->compression_level,          GAP_VAL_GINT32, 0, "\0");
   gap_val_set_keyword(keylist, "(use_lpc ",                   &epp->use_lpc,                    GAP_VAL_GINT32, 0, "# Sets whether to use LPC mode - used by FLAC encoder");
   gap_val_set_keyword(keylist, "(lpc_coeff_precision ",       &epp->lpc_coeff_precision,        GAP_VAL_GINT32, 0, "# LPC coefficient precision - used by FLAC encoder");
   gap_val_set_keyword(keylist, "(min_prediction_order ",      &epp->min_prediction_order,       GAP_VAL_GINT32, 0, "\0");
   gap_val_set_keyword(keylist, "(max_prediction_order ",      &epp->max_prediction_order,       GAP_VAL_GINT32, 0, "\0");
   gap_val_set_keyword(keylist, "(prediction_order_method ",   &epp->prediction_order_method,    GAP_VAL_GINT32, 0, "# search method for selecting prediction order");
   gap_val_set_keyword(keylist, "(min_partition_order ",       &epp->min_partition_order,        GAP_VAL_GINT32, 0, "\0");
   gap_val_set_keyword(keylist, "(timecode_frame_start ",      &epp->timecode_frame_start,       GAP_VAL_GINT64, 0, "# GOP timecode frame start number, in non drop frame format");
   gap_val_set_keyword(keylist, "(bits_per_raw_sample ",       &epp->bits_per_raw_sample,        GAP_VAL_GINT64, 0, "# This field is applicable only when sample_fmt is SAMPLE_FMT_S32");
   gap_val_set_keyword(keylist, "(channel_layout ",            &epp->channel_layout,             GAP_VAL_GINT64, 0, "# Audio channel layout");
   gap_val_set_keyword(keylist, "(rc_max_available_vbv_use ",  &epp->rc_max_available_vbv_use,   GAP_VAL_GDOUBLE, 0, "# Ratecontrol attempt to use, at maximum, <value> of what can be used without an underflow");
   gap_val_set_keyword(keylist, "(rc_min_vbv_overflow_use ",   &epp->rc_min_vbv_overflow_use,    GAP_VAL_GDOUBLE, 0, "# Ratecontrol attempt to use, at least, <value> times the amount needed to prevent a vbv overflow");

   /* new params (introduced with ffmpeg 0.6) */
   gap_val_set_keyword(keylist, "(color_primaries ",          &epp->color_primaries,          GAP_VAL_GINT32, 0, "# Chromaticity coordinates of the source primaries.");
   gap_val_set_keyword(keylist, "(color_trc ",                &epp->color_trc,                GAP_VAL_GINT32, 0, "# Color Transfer Characteristic.");
   gap_val_set_keyword(keylist, "(colorspace ",               &epp->colorspace,               GAP_VAL_GINT32, 0, "# YUV colorspace type.");
   gap_val_set_keyword(keylist, "(color_range ",              &epp->color_range,              GAP_VAL_GINT32, 0, "# MPEG vs JPEG YUV range.");
   gap_val_set_keyword(keylist, "(chroma_sample_location ",   &epp->chroma_sample_location,   GAP_VAL_GINT32, 0, "# This defines the location of chroma samples.");
   gap_val_set_keyword(keylist, "(weighted_p_pred ",          &epp->weighted_p_pred,          GAP_VAL_GINT32, 0, "# explicit P-frame weighted prediction analysis method 0:off, 1: fast blind weighting, 2:smart weighting (full fade detection analysis)");
   gap_val_set_keyword(keylist, "(aq_mode ",                  &epp->aq_mode,                  GAP_VAL_GINT32, 0, "# AQ mode");
   gap_val_set_keyword(keylist, "(aq_strength ",              &epp->aq_strength,              GAP_VAL_GDOUBLE, 0, "# AQ strength, Reduces blocking and blurring in flat and textured areas.");
   gap_val_set_keyword(keylist, "(psy_rd ",                   &epp->psy_rd,                   GAP_VAL_GDOUBLE, 0, "# PSY RD Strength of psychovisual optimization ");
   gap_val_set_keyword(keylist, "(psy_trellis ",              &epp->psy_trellis,              GAP_VAL_GDOUBLE, 0, "# PSY trellis Strength of psychovisual optimization");
   gap_val_set_keyword(keylist, "(rc_lookahead ",             &epp->rc_lookahead,             GAP_VAL_GINT32, 0,  "# Number of frames for frametype and ratecontrol lookahead");

   /* codec flags */

   p_set_keyword_bool32(keylist, "(bitexact ",                &epp->bitexact,            "# CODEC_FLAG_BITEXACT (for testing, unused in productive version)");
   p_set_keyword_bool32(keylist, "(use_ss ",                  &epp->use_ss,              "# CODEC_FLAG_H263P_SLICE_STRUCT enable Slice Structured mode (h263+)");
   p_set_keyword_bool32(keylist, "(use_aiv ",                 &epp->use_aiv,             "# CODEC_FLAG_H263P_AIV  enable Alternative inter vlc (h263+)");
   p_set_keyword_bool32(keylist, "(use_obmc ",                &epp->use_obmc,            "# CODEC_FLAG_OBMC use overlapped block motion compensation (h263+)");
   p_set_keyword_bool32(keylist, "(use_loop ",                &epp->use_loop,            "# CODEC_FLAG_LOOP_FILTER use loop filter (h263+)");
   p_set_keyword_bool32(keylist, "(use_alt_scan ",            &epp->use_alt_scan,        "# CODEC_FLAG_ALT_SCAN enable alternate scantable (MPEG2/MPEG4)");
   p_set_keyword_bool32(keylist, "(use_mv0 ",                 &epp->use_mv0,             "# CODEC_FLAG_MV0 try to encode each MB with MV=<0,0> and choose the better one (has no effect if mbd=0)");
   p_set_keyword_bool32(keylist, "(do_normalize_aqp ",        &epp->do_normalize_aqp,    "# CODEC_FLAG_NORMALIZE_AQP normalize adaptive quantization");
   p_set_keyword_bool32(keylist, "(use_scan_offset ",         &epp->use_scan_offset,     "# CODEC_FLAG_SVCD_SCAN_OFFSET enable SVCD Scan Offset placeholder");
   p_set_keyword_bool32(keylist, "(closed_gop ",              &epp->closed_gop,          "# CODEC_FLAG_CLOSED_GOP closed gop");
   p_set_keyword_bool32(keylist, "(use_qpel ",                &epp->use_qpel,            "# CODEC_FLAG_QPEL enable 1/4-pel");
   p_set_keyword_bool32(keylist, "(use_qprd ",                &epp->use_qprd,            "# CODEC_FLAG_QP_RD use rate distortion optimization for qp selection");
   p_set_keyword_bool32(keylist, "(use_cbprd ",               &epp->use_cbprd,           "# CODEC_FLAG_CBP_RD use rate distortion optimization for cbp");
   p_set_keyword_bool32(keylist, "(do_interlace_dct ",        &epp->do_interlace_dct,    "# CODEC_FLAG_INTERLACED_DCT use interlaced dct");
   p_set_keyword_bool32(keylist, "(do_interlace_me ",         &epp->do_interlace_me,     "# CODEC_FLAG_INTERLACED_ME interlaced motion estimation");
   p_set_keyword_bool32(keylist, "(aic ",                     &epp->aic,                 "# CODEC_FLAG_AC_PRED activate intra frame coding (only h263+ CODEC) ");
   p_set_keyword_bool32(keylist, "(umv ",                     &epp->umv,                 "# CODEC_FLAG_H263P_UMV enable unlimited motion vector (only h263+ CODEC)");
   p_set_keyword_bool32(keylist, "(mv4 ",                     &epp->mv4,                 "# CODEC_FLAG_4MV use four motion vector by macroblock (only MPEG-4 CODEC)");
   p_set_keyword_bool32(keylist, "(partitioning ",            &epp->partitioning,        "# CODEC_FLAG_PART use data partitioning (only MPEG-4 CODEC)");

   /* support for new codec flags */
   p_set_keyword_bool32(keylist, "(use_gmc ",                 &epp->codec_FLAG_GMC,                     "# CODEC_FLAG_GMC                  Use GMC");
   p_set_keyword_bool32(keylist, "(input_preserved ",         &epp->codec_FLAG_INPUT_PRESERVED,         "# CODEC_FLAG_INPUT_PRESERVED      ");
   p_set_keyword_bool32(keylist, "(use_gray ",                &epp->codec_FLAG_GRAY,                    "# CODEC_FLAG_GRAY                 Only encode grayscale");
   p_set_keyword_bool32(keylist, "(use_emu_edge ",            &epp->codec_FLAG_EMU_EDGE,                "# CODEC_FLAG_EMU_EDGE             Dont draw edges");
   p_set_keyword_bool32(keylist, "(use_truncated ",           &epp->codec_FLAG_TRUNCATED,               "# CODEC_FLAG_TRUNCATED            Input bitstream might be truncated at a random location instead of only at frame boundaries");

   p_set_keyword_bool32(keylist, "(use_fast ",                &epp->codec_FLAG2_FAST,                   "# CODEC_FLAG2_FAST                Allow non spec compliant speedup tricks");
   p_set_keyword_bool32(keylist, "(use_local_header ",        &epp->codec_FLAG2_LOCAL_HEADER,           "# CODEC_FLAG2_LOCAL_HEADER        Place global headers at every keyframe instead of in extradata.");
   p_set_keyword_bool32(keylist, "(use_bpyramid ",            &epp->codec_FLAG2_BPYRAMID,               "# CODEC_FLAG2_BPYRAMID            H.264 allow B-frames to be used as references for predicting.");
   p_set_keyword_bool32(keylist, "(use_wpred ",               &epp->codec_FLAG2_WPRED,                  "# CODEC_FLAG2_WPRED               H.264 weighted biprediction for B-frames");
   p_set_keyword_bool32(keylist, "(use_mixed_refs ",          &epp->codec_FLAG2_MIXED_REFS,             "# CODEC_FLAG2_MIXED_REFS          H.264 one reference per partition, as opposed to one reference per macroblock");
   p_set_keyword_bool32(keylist, "(use_dct8x8 ",              &epp->codec_FLAG2_8X8DCT,                 "# CODEC_FLAG2_8X8DCT              H.264 high profile 8x8 transform");
   p_set_keyword_bool32(keylist, "(use_fastpskip ",           &epp->codec_FLAG2_FASTPSKIP,              "# CODEC_FLAG2_FASTPSKIP           H.264 fast pskip");
   p_set_keyword_bool32(keylist, "(use_aud ",                 &epp->codec_FLAG2_AUD,                    "# CODEC_FLAG2_AUD                 H.264 access unit delimiters");
   p_set_keyword_bool32(keylist, "(use_brdo ",                &epp->codec_FLAG2_BRDO,                   "# CODEC_FLAG2_BRDO                B-frame rate-distortion optimization");
   p_set_keyword_bool32(keylist, "(use_ivlc ",                &epp->codec_FLAG2_INTRA_VLC,              "# CODEC_FLAG2_INTRA_VLC           Use MPEG-2 intra VLC table.");
   p_set_keyword_bool32(keylist, "(use_memc_only ",           &epp->codec_FLAG2_MEMC_ONLY,              "# CODEC_FLAG2_MEMC_ONLY           Only do ME/MC (I frames -> ref, P frame -> ME+MC)");
   p_set_keyword_bool32(keylist, "(use_drop_frame_timecode ", &epp->codec_FLAG2_DROP_FRAME_TIMECODE,    "# CODEC_FLAG2_DROP_FRAME_TIMECODE timecode is in drop frame format");
   p_set_keyword_bool32(keylist, "(use_skip_rd ",             &epp->codec_FLAG2_SKIP_RD,                "# CODEC_FLAG2_SKIP_RD             RD optimal MB level residual skipping");
   p_set_keyword_bool32(keylist, "(use_chunks ",              &epp->codec_FLAG2_CHUNKS,                 "# CODEC_FLAG2_CHUNKS              Input bitstream might be truncated at a packet boundaries instead of only at frame boundaries");
   p_set_keyword_bool32(keylist, "(use_non_linear_quant ",    &epp->codec_FLAG2_NON_LINEAR_QUANT,       "# CODEC_FLAG2_NON_LINEAR_QUANT    Use MPEG-2 nonlinear quantizer");
   p_set_keyword_bool32(keylist, "(use_bit_reservoir ",       &epp->codec_FLAG2_BIT_RESERVOIR,          "# CODEC_FLAG2_BIT_RESERVOIR       Use a bit reservoir when encoding if possible");

   /* codec flags new in ffmpeg-0.6 */

   p_set_keyword_bool32(keylist, "(use_bit_reservoir ",       &epp->codec_FLAG2_BIT_RESERVOIR,          "# CODEC_FLAG2_BIT_RESERVOIR       Use a bit reservoir when encoding if possible");
   p_set_keyword_bool32(keylist, "(use_bit_mbtree ",          &epp->codec_FLAG2_MBTREE,                 "# CODEC_FLAG2_MBTREE              Use macroblock tree ratecontrol (x264 only)");
   p_set_keyword_bool32(keylist, "(use_bit_psy ",             &epp->codec_FLAG2_PSY,                    "# CODEC_FLAG2_PSY                 Use psycho visual optimizations.");
   p_set_keyword_bool32(keylist, "(use_bit_ssim ",            &epp->codec_FLAG2_SSIM,                   "# CODEC_FLAG2_SSIM                Compute SSIM during encoding, error[] values are undefined");


}  /* end p_set_master_keywords */


/* --------------------------
 * p_debug_printf_parameters
 * --------------------------
 */
static void
p_debug_printf_parameters(GapGveFFMpegValues *epp)
{
  printf("format_name  %s\n", epp->format_name);
  printf("vcodec_name  %s\n", epp->vcodec_name);
  printf("acodec_name  %s\n", epp->acodec_name);
  printf("title  %s\n", epp->title);
  printf("author  %s\n", epp->author);
  printf("copyright  %s\n", epp->copyright);
  printf("comment  %s\n", epp->comment);
  printf("passlogfile  %s\n", epp->passlogfile);

  printf("pass_nr  %d\n", (int)epp->pass_nr);
  printf("audio_bitrate  %d\n", (int)epp->audio_bitrate);
  printf("video_bitrate  %d\n", (int)epp->video_bitrate);
  printf("gop_size  %d\n", (int)epp->gop_size);
  printf("intra  %d\n", (int)epp->intra);
  printf("qscale  %f\n", (float)epp->qscale);
  printf("qmin  %d\n", (int)epp->qmin);
  printf("qmax  %d\n", (int)epp->qmax);
  printf("qdiff  %d\n", (int)epp->qdiff);
  printf("qblur  %f\n", (float)epp->qblur);
  printf("qcomp  %f\n", (float)epp->qcomp);
  printf("rc_init_cplx  %f\n", (float)epp->rc_init_cplx);
  printf("b_qfactor  %f\n", (float)epp->b_qfactor);
  printf("i_qfactor  %f\n", (float)epp->i_qfactor);
  printf("b_qoffset  %f\n", (float)epp->b_qoffset);
  printf("i_qoffset  %f\n", (float)epp->i_qoffset);
  printf("bitrate_tol  %d\n", (int)epp->bitrate_tol);
  printf("maxrate_tol  %d\n", (int)epp->maxrate_tol);
  printf("minrate_tol  %d\n", (int)epp->minrate_tol);
  printf("bufsize  %d\n", (int)epp->bufsize);
  printf("motion_estimation  %d\n", (int)epp->motion_estimation);
  printf("dct_algo  %d\n", (int)epp->dct_algo);
  printf("idct_algo  %d\n", (int)epp->idct_algo);
  printf("strict  %d\n", (int)epp->strict);
  printf("mb_qmin  %d\n", (int)epp->mb_qmin);
  printf("mb_qmax  %d\n", (int)epp->mb_qmax);
  printf("mb_decision  %d\n", (int)epp->mb_decision);
  printf("aic  %d\n", (int)epp->aic);
  printf("umv  %d\n", (int)epp->umv);
  printf("b_frames  %d\n", (int)epp->b_frames);
  printf("mv4  %d\n", (int)epp->mv4);
  printf("partitioning  %d\n", (int)epp->partitioning);
  printf("packet_size  %d\n", (int)epp->packet_size);
  printf("bitexact  %d\n", (int)epp->bitexact);
  printf("set_aspect_ratio  %d\n", (int)epp->set_aspect_ratio);
  printf("dont_recode_flag  %d\n", (int)epp->dont_recode_flag);
  printf("factor_aspect_ratio  %f\n", (float)epp->factor_aspect_ratio);

}  /* end  p_debug_printf_parameters */


/* --------------------------
 * gap_ffpar_get
 * --------------------------
 * get parameters from ffmpeg videoencoder parameter file.
 * get does always fetch all values for all known keywords
 * and delivers default values for all params, that are not
 * found in the file.
 */
void
gap_ffpar_get(const char *filename, GapGveFFMpegValues *epp)
{
  GapValKeyList    *keylist;
  gint              cnt_scanned_items;

  /* init with defaults the case where no video_info file available
   * (use preset index 0 for the default settings)
   */
  gap_enc_ffmpeg_main_init_preset_params(epp, 0);

  keylist = gap_val_new_keylist();
  p_set_master_keywords(keylist, epp);

  cnt_scanned_items = gap_val_scann_filevalues(keylist, filename);
  gap_val_free_keylist(keylist);

  if(cnt_scanned_items <= 0)
  {
    g_message(_("Could not read ffmpeg video encoder parameters from file:\n%s"), filename);
  }

  epp->twoPassFlag = FALSE;
  if (epp->pass_nr == 2)
  {
    epp->twoPassFlag = TRUE;
  }

  if(gap_debug)
  {
    printf("gap_ffpar_get: params loaded: epp:%d\n", (int)epp);
    p_debug_printf_parameters(epp);
  }
}  /* end gap_ffpar_get */


/* --------------------------
 * gap_ffpar_set
 * --------------------------
 * (re)write the specified ffmpeg videoencode parameter file
 */
int
gap_ffpar_set(const char *filename, GapGveFFMpegValues *epp)
{
  GapValKeyList    *keylist;
  int          l_rc;

  epp->pass_nr = 1;
  if (epp->twoPassFlag == TRUE)
  {
    epp->pass_nr = 2;
  }
  keylist = gap_val_new_keylist();

  if(gap_debug)
  {
    printf("gap_ffpar_set: now saving parameters: epp:%d to file:%s\n", (int)epp, filename);
    p_debug_printf_parameters(epp);
  }

  p_set_master_keywords(keylist, epp);
  l_rc = gap_val_rewrite_file(keylist
                          ,filename
                          ,"# GIMP / GAP FFMPEG videoencoder parameter file"   /*  hdr_text */
                          ,")"                                                 /* terminate char */
                          );

  gap_val_free_keylist(keylist);

  if(l_rc != 0)
  {
    gint l_errno;

    l_errno = errno;
    g_message(_("Could not save ffmpeg video encoder parameterfile:"
               "'%s'"
               "%s")
               ,filename
               ,g_strerror (l_errno) );
  }

  return(l_rc);
}  /* end gap_ffpar_set */
