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
 * version 2.1.0a;  2005/03/24  hof: created (based on procedures of the gap_vin module)
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
 * p_set_master_keywords
 * --------------------------
 */
static void
p_set_master_keywords(GapVinKeyList *keylist, GapGveFFMpegValues *ffpar_ptr)
{

   gap_vin_set_keyword(keylist, "(format_name ",   &ffpar_ptr->format_name[0], GAP_VIN_STRING, sizeof(ffpar_ptr->format_name), "\0");
   gap_vin_set_keyword(keylist, "(vcodec_name ",   &ffpar_ptr->vcodec_name[0], GAP_VIN_STRING, sizeof(ffpar_ptr->vcodec_name), "\0");
   gap_vin_set_keyword(keylist, "(acodec_name ",   &ffpar_ptr->acodec_name[0], GAP_VIN_STRING, sizeof(ffpar_ptr->acodec_name), "\0");
   gap_vin_set_keyword(keylist, "(title ",         &ffpar_ptr->title[0],       GAP_VIN_STRING, sizeof(ffpar_ptr->title), "\0");
   gap_vin_set_keyword(keylist, "(author ",        &ffpar_ptr->author[0],      GAP_VIN_STRING, sizeof(ffpar_ptr->author), "\0");
   gap_vin_set_keyword(keylist, "(copyright ",     &ffpar_ptr->copyright[0],   GAP_VIN_STRING, sizeof(ffpar_ptr->copyright), "\0");
   gap_vin_set_keyword(keylist, "(comment ",       &ffpar_ptr->comment[0],     GAP_VIN_STRING, sizeof(ffpar_ptr->comment), "\0");
   gap_vin_set_keyword(keylist, "(passlogfile ",   &ffpar_ptr->passlogfile[0], GAP_VIN_STRING, sizeof(ffpar_ptr->passlogfile), "\0");

   gap_vin_set_keyword(keylist, "(pass ",          &ffpar_ptr->pass,           GAP_VIN_GINT32, 0, "\0");
   gap_vin_set_keyword(keylist, "(audio_bitrate ", &ffpar_ptr->audio_bitrate,  GAP_VIN_GINT32, 0, "# audio bitrate in kBit/sec");
   gap_vin_set_keyword(keylist, "(video_bitrate ", &ffpar_ptr->video_bitrate,  GAP_VIN_GINT32, 0, "# video bitrate in kBit/sec\0");
   gap_vin_set_keyword(keylist, "(gop_size ",      &ffpar_ptr->gop_size,       GAP_VIN_GINT32, 0, "# group of picture size");
   gap_vin_set_keyword(keylist, "(intra ",         &ffpar_ptr->intra,          GAP_VIN_G32BOOLEAN, 0, "# use only intra frames");
   gap_vin_set_keyword(keylist, "(qscale ",        &ffpar_ptr->qscale,         GAP_VIN_GDOUBLE, 0, "# use fixed video quantiser scale (VBR). 0=const bitrate");
   gap_vin_set_keyword(keylist, "(qmin ",          &ffpar_ptr->qmin,           GAP_VIN_GINT32, 0, "# min video quantiser scale (VBR)");
   gap_vin_set_keyword(keylist, "(qmax ",          &ffpar_ptr->qmax,           GAP_VIN_GINT32, 0, "# max video quantiser scale (VBR)");
   gap_vin_set_keyword(keylist, "(qdiff ",         &ffpar_ptr->qdiff,          GAP_VIN_GINT32, 0, "# max difference between the quantiser scale (VBR)");
   gap_vin_set_keyword(keylist, "(qblur ",         &ffpar_ptr->qblur,          GAP_VIN_GDOUBLE, 0, "# video quantiser scale blur (VBR)");
   gap_vin_set_keyword(keylist, "(qcomp ",         &ffpar_ptr->qcomp,          GAP_VIN_GDOUBLE, 0, "# video quantiser scale compression (VBR)");
   gap_vin_set_keyword(keylist, "(rc_init_cplx ",  &ffpar_ptr->rc_init_cplx,   GAP_VIN_GDOUBLE, 0, "# initial complexity for 1-pass encoding");
   gap_vin_set_keyword(keylist, "(b_qfactor ",     &ffpar_ptr->b_qfactor,      GAP_VIN_GDOUBLE, 0, "# qp factor between p and b frames");
   gap_vin_set_keyword(keylist, "(i_qfactor ",     &ffpar_ptr->i_qfactor,      GAP_VIN_GDOUBLE, 0, "# qp factor between p and i frames");
   gap_vin_set_keyword(keylist, "(b_qoffset ",     &ffpar_ptr->b_qoffset,      GAP_VIN_GDOUBLE, 0, "# qp offset between p and b frames");
   gap_vin_set_keyword(keylist, "(i_qoffset ",     &ffpar_ptr->i_qoffset,      GAP_VIN_GDOUBLE, 0, "# qp offset between p and i frames");
   gap_vin_set_keyword(keylist, "(bitrate_tol ",   &ffpar_ptr->bitrate_tol,    GAP_VIN_GINT32, 0, "# set video bitrate tolerance (in kbit/s)");
   gap_vin_set_keyword(keylist, "(maxrate_tol ",   &ffpar_ptr->maxrate_tol,    GAP_VIN_GINT32, 0, "# set max video bitrate tolerance (in kbit/s)");
   gap_vin_set_keyword(keylist, "(minrate_tol ",   &ffpar_ptr->minrate_tol,    GAP_VIN_GINT32, 0, "# set min video bitrate tolerance (in kbit/s)");
   gap_vin_set_keyword(keylist, "(bufsize ",       &ffpar_ptr->bufsize,        GAP_VIN_GINT32, 0, "# set ratecontrol buffere size (in kbit)");
   gap_vin_set_keyword(keylist, "(motion_estimation ", &ffpar_ptr->motion_estimation,           GAP_VIN_GINT32, 0, "# algorithm for motion estimation (1-6)");
   gap_vin_set_keyword(keylist, "(dct_algo ",      &ffpar_ptr->dct_algo,       GAP_VIN_GINT32, 0, "# algorithm for DCT (0-6)");
   gap_vin_set_keyword(keylist, "(idct_algo ",     &ffpar_ptr->idct_algo,      GAP_VIN_GINT32, 0, "# algorithm for IDCT (0-11)");
   gap_vin_set_keyword(keylist, "(strict ",        &ffpar_ptr->strict,         GAP_VIN_GINT32, 0, "# how strictly to follow the standards");
   gap_vin_set_keyword(keylist, "(mb_qmin ",       &ffpar_ptr->mb_qmin,        GAP_VIN_GINT32, 0, "# min macroblock quantiser scale (VBR)");
   gap_vin_set_keyword(keylist, "(mb_qmax ",       &ffpar_ptr->mb_qmax,        GAP_VIN_GINT32, 0, "# max macroblock quantiser scale (VBR)");
   gap_vin_set_keyword(keylist, "(mb_decision ",   &ffpar_ptr->mb_decision,    GAP_VIN_GINT32, 0, "# algorithm for macroblock decision (0-2)");
   gap_vin_set_keyword(keylist, "(aic ",           &ffpar_ptr->aic,            GAP_VIN_G32BOOLEAN, 0, "# activate intra frame coding (only h263+ CODEC)");
   gap_vin_set_keyword(keylist, "(umv ",           &ffpar_ptr->umv,            GAP_VIN_G32BOOLEAN, 0, "# enable unlimited motion vector (only h263+ CODEC)");
   gap_vin_set_keyword(keylist, "(b_frames ",      &ffpar_ptr->b_frames,       GAP_VIN_GINT32, 0, "# max number of B-frames in sequence");
   gap_vin_set_keyword(keylist, "(mv4 ",           &ffpar_ptr->mv4,            GAP_VIN_G32BOOLEAN, 0, "# use four motion vector by macroblock (only MPEG-4 CODEC)");
   gap_vin_set_keyword(keylist, "(partitioning ",  &ffpar_ptr->partitioning,   GAP_VIN_G32BOOLEAN, 0, "# use data partitioning (only MPEG-4 CODEC)");
   gap_vin_set_keyword(keylist, "(packet_size ",   &ffpar_ptr->packet_size,    GAP_VIN_GINT32, 0, "\0");

   /* debug options */
   /* gint32  benchmark; */
   /* gint32  hex_dump; */
   /* gint32  psnr; */
   /* gint32  vstats; */
   gap_vin_set_keyword(keylist, "(bitexact ",      &ffpar_ptr->bitexact,       GAP_VIN_G32BOOLEAN, 0, "\0");
   gap_vin_set_keyword(keylist, "(set_aspect_ratio ",     &ffpar_ptr->set_aspect_ratio,    GAP_VIN_G32BOOLEAN, 0, "# store aspectratio information (width/height) in the output video");
   gap_vin_set_keyword(keylist, "(factor_aspect_ratio ",  &ffpar_ptr->factor_aspect_ratio, GAP_VIN_GDOUBLE, 0, "# the aspect ratio (width/height). 0.0 = auto ratio assuming sqaure pixels");
   gap_vin_set_keyword(keylist, "(dont_recode_flag ",     &ffpar_ptr->dont_recode_flag,    GAP_VIN_GBOOLEAN, 0, "# bypass the video encoder where frames can be copied 1:1");

   /* new params (introduced with ffmpeg 0.4.9pre1) */
   gap_vin_set_keyword(keylist, "(thread_count ",         &ffpar_ptr->thread_count,        GAP_VIN_GINT32, 0, "\0");
   gap_vin_set_keyword(keylist, "(mb_cmp ",               &ffpar_ptr->mb_cmp,              GAP_VIN_GINT32, 0, "# macroblock compare function (0-13,255)");
   gap_vin_set_keyword(keylist, "(ildct_cmp ",            &ffpar_ptr->ildct_cmp,           GAP_VIN_GINT32, 0, "# ildct compare function (0-13,255)");
   gap_vin_set_keyword(keylist, "(sub_cmp ",              &ffpar_ptr->sub_cmp,             GAP_VIN_GINT32, 0, "# subpel compare function (0-13,255)");
   gap_vin_set_keyword(keylist, "(cmp ",                  &ffpar_ptr->cmp,                 GAP_VIN_GINT32, 0, "# fullpel compare function (0-13,255)");
   gap_vin_set_keyword(keylist, "(pre_cmp ",              &ffpar_ptr->pre_cmp,             GAP_VIN_GINT32, 0, "# pre motion estimation compare function (0-13,255)");
   gap_vin_set_keyword(keylist, "(pre_me ",               &ffpar_ptr->pre_me,              GAP_VIN_GINT32, 0, "# pre motion estimation");
   gap_vin_set_keyword(keylist, "(lumi_mask ",            &ffpar_ptr->lumi_mask,           GAP_VIN_GDOUBLE, 0, "# luminance masking");
   gap_vin_set_keyword(keylist, "(dark_mask ",            &ffpar_ptr->dark_mask,           GAP_VIN_GDOUBLE, 0, "# darkness masking");
   gap_vin_set_keyword(keylist, "(scplx_mask ",           &ffpar_ptr->scplx_mask,          GAP_VIN_GDOUBLE, 0, "# spatial complexity masking");
   gap_vin_set_keyword(keylist, "(tcplx_mask ",           &ffpar_ptr->tcplx_mask,          GAP_VIN_GDOUBLE, 0, "# temporary complexity masking");
   gap_vin_set_keyword(keylist, "(p_mask ",               &ffpar_ptr->p_mask,              GAP_VIN_GDOUBLE, 0, "# p block masking");
   gap_vin_set_keyword(keylist, "(qns ",                  &ffpar_ptr->qns,                 GAP_VIN_GINT32, 0, "# quantization noise shaping");
   gap_vin_set_keyword(keylist, "(use_ss ",               &ffpar_ptr->use_ss,              GAP_VIN_G32BOOLEAN, 0, "# enable Slice Structured mode (h263+)");
   gap_vin_set_keyword(keylist, "(use_aiv ",              &ffpar_ptr->use_aiv,             GAP_VIN_G32BOOLEAN, 0, "# enable Alternative inter vlc (h263+)");
   gap_vin_set_keyword(keylist, "(use_obmc ",             &ffpar_ptr->use_obmc,            GAP_VIN_G32BOOLEAN, 0, "# use overlapped block motion compensation (h263+)");
   gap_vin_set_keyword(keylist, "(use_loop ",             &ffpar_ptr->use_loop,            GAP_VIN_G32BOOLEAN, 0, "# use loop filter (h263+)");
   gap_vin_set_keyword(keylist, "(use_alt_scan ",         &ffpar_ptr->use_alt_scan,        GAP_VIN_G32BOOLEAN, 0, "# enable alternate scantable (MPEG2/MPEG4)");
   gap_vin_set_keyword(keylist, "(use_trell ",            &ffpar_ptr->use_trell,           GAP_VIN_G32BOOLEAN, 0, "# enable trellis quantization");
   gap_vin_set_keyword(keylist, "(use_mv0 ",              &ffpar_ptr->use_mv0,             GAP_VIN_G32BOOLEAN, 0, "# try to encode each MB with MV=<0,0> and choose the better one (has no effect if mbd=0)");
   gap_vin_set_keyword(keylist, "(do_normalize_aqp ",     &ffpar_ptr->do_normalize_aqp,    GAP_VIN_G32BOOLEAN, 0, "# normalize adaptive quantization");
   gap_vin_set_keyword(keylist, "(use_scan_offset ",      &ffpar_ptr->use_scan_offset,     GAP_VIN_G32BOOLEAN, 0, "# enable SVCD Scan Offset placeholder");
   gap_vin_set_keyword(keylist, "(closed_gop ",           &ffpar_ptr->closed_gop,          GAP_VIN_G32BOOLEAN, 0, "# closed gop");
   gap_vin_set_keyword(keylist, "(use_qpel ",             &ffpar_ptr->use_qpel,            GAP_VIN_G32BOOLEAN, 0, "# enable 1/4-pel");
   gap_vin_set_keyword(keylist, "(use_qprd ",             &ffpar_ptr->use_qprd,            GAP_VIN_G32BOOLEAN, 0, "# use rate distortion optimization for qp selection");
   gap_vin_set_keyword(keylist, "(use_cbprd ",            &ffpar_ptr->use_cbprd,           GAP_VIN_G32BOOLEAN, 0, "# use rate distortion optimization for cbp");
   gap_vin_set_keyword(keylist, "(do_interlace_dct ",     &ffpar_ptr->do_interlace_dct,    GAP_VIN_G32BOOLEAN, 0, "# use interlaced dct");
   gap_vin_set_keyword(keylist, "(do_interlace_me ",      &ffpar_ptr->do_interlace_me,     GAP_VIN_G32BOOLEAN, 0, "# interlaced motion estimation");

   gap_vin_set_keyword(keylist, "(video_lmin ",           &ffpar_ptr->video_lmin,          GAP_VIN_GINT32, 0, "# min video lagrange factor (VBR) 0-3658");
   gap_vin_set_keyword(keylist, "(video_lmax ",           &ffpar_ptr->video_lmax,          GAP_VIN_GINT32, 0, "# max video lagrange factor (VBR) 0-3658");
   gap_vin_set_keyword(keylist, "(video_lelim ",          &ffpar_ptr->video_lelim,         GAP_VIN_GINT32, 0, "# luma elimination threshold");
   gap_vin_set_keyword(keylist, "(video_celim ",          &ffpar_ptr->video_celim,         GAP_VIN_GINT32, 0, "# chroma elimination threshold");
   gap_vin_set_keyword(keylist, "(video_intra_quant_bias ", &ffpar_ptr->video_intra_quant_bias, GAP_VIN_GINT32, 0, "# intra quant bias");
   gap_vin_set_keyword(keylist, "(video_inter_quant_bias ", &ffpar_ptr->video_inter_quant_bias, GAP_VIN_GINT32, 0, "# inter quant bias");
   gap_vin_set_keyword(keylist, "(me_threshold ",         &ffpar_ptr->me_threshold,        GAP_VIN_GINT32, 0, "# motion estimaton threshold");
   gap_vin_set_keyword(keylist, "(mb_threshold ",         &ffpar_ptr->mb_threshold,        GAP_VIN_GINT32, 0, "# macroblock threshold");
   gap_vin_set_keyword(keylist, "(intra_dc_precision ",   &ffpar_ptr->intra_dc_precision,  GAP_VIN_GINT32, 0, "# precision of the intra dc coefficient - 8");
   gap_vin_set_keyword(keylist, "(error_rate ",           &ffpar_ptr->error_rate,          GAP_VIN_GINT32, 0, "# simulates errors in the bitstream to test error concealment");
   gap_vin_set_keyword(keylist, "(noise_reduction ",      &ffpar_ptr->noise_reduction,     GAP_VIN_GINT32, 0, "# noise reduction strength");
   gap_vin_set_keyword(keylist, "(sc_threshold ",         &ffpar_ptr->sc_threshold,        GAP_VIN_GINT32, 0, "# scene change detection threshold");
   gap_vin_set_keyword(keylist, "(me_range ",             &ffpar_ptr->me_range,            GAP_VIN_GINT32, 0, "# limit motion vectors range (1023 for DivX player, 0 = no limit)");
   gap_vin_set_keyword(keylist, "(coder ",                &ffpar_ptr->coder,               GAP_VIN_GINT32, 0, "# coder type 0=VLC 1=AC");
   gap_vin_set_keyword(keylist, "(context ",              &ffpar_ptr->context,             GAP_VIN_GINT32, 0, "# context model");
   gap_vin_set_keyword(keylist, "(predictor ",            &ffpar_ptr->predictor,           GAP_VIN_GINT32, 0, "# prediction method (0 LEFT, 1 PLANE, 2 MEDIAN");
   gap_vin_set_keyword(keylist, "(nsse_weight ",          &ffpar_ptr->nsse_weight,         GAP_VIN_GINT32, 0, "# noise vs. sse weight for the nsse comparsion function.");
   gap_vin_set_keyword(keylist, "(subpel_quality ",       &ffpar_ptr->subpel_quality,      GAP_VIN_GINT32, 0, "# subpel ME quality");

   /* new parms (found CVS version 2005.03.02 == LIBAVCODEC_BUILD 4744 ) */
   gap_vin_set_keyword(keylist, "(strict_gop ",           &ffpar_ptr->strict_gop,          GAP_VIN_GINT32, 0, "# strictly enforce GOP size");
   gap_vin_set_keyword(keylist, "(no_output ",            &ffpar_ptr->no_output,           GAP_VIN_GINT32, 0, "# skip bitstream encoding");
   gap_vin_set_keyword(keylist, "(video_mb_lmin ",        &ffpar_ptr->video_mb_lmin,       GAP_VIN_GINT32, 0, "# min macroblock quantiser scale (VBR) 0-3658");
   gap_vin_set_keyword(keylist, "(video_mb_lmax ",        &ffpar_ptr->video_mb_lmax,       GAP_VIN_GINT32, 0, "# max macroblock quantiser scale (VBR) 0-3658");
   gap_vin_set_keyword(keylist, "(video_profile ",        &ffpar_ptr->video_profile,       GAP_VIN_GINT32, 0, "\0");
   gap_vin_set_keyword(keylist, "(video_level ",          &ffpar_ptr->video_level,         GAP_VIN_GINT32, 0, "\0");
   gap_vin_set_keyword(keylist, "(frame_skip_threshold ", &ffpar_ptr->frame_skip_threshold,GAP_VIN_GINT32, 0, "# frame skip threshold");
   gap_vin_set_keyword(keylist, "(frame_skip_factor ",    &ffpar_ptr->frame_skip_factor,   GAP_VIN_GINT32, 0, "# frame skip factor");
   gap_vin_set_keyword(keylist, "(frame_skip_exp ",       &ffpar_ptr->frame_skip_exp,      GAP_VIN_GINT32, 0, "# frame skip exponent");
   gap_vin_set_keyword(keylist, "(frame_skip_cmp ",       &ffpar_ptr->frame_skip_cmp,      GAP_VIN_GINT32, 0, "# frame skip comparission function");
   gap_vin_set_keyword(keylist, "(mux_rate ",             &ffpar_ptr->mux_rate,            GAP_VIN_GINT32, 0, "\0");
   gap_vin_set_keyword(keylist, "(mux_packet_size ",      &ffpar_ptr->mux_packet_size,     GAP_VIN_GINT32, 0, "\0");

   gap_vin_set_keyword(keylist, "(mux_preload ",          &ffpar_ptr->mux_preload,         GAP_VIN_GDOUBLE, 0, "# set the initial demux-decode delay (secs)");
   gap_vin_set_keyword(keylist, "(mux_max_delay ",        &ffpar_ptr->mux_max_delay,       GAP_VIN_GDOUBLE, 0, "# set the maximum demux-decode delay (secs)");

}  /* end p_set_master_keywords */


/* --------------------------
 * gap_ffpar_get
 * --------------------------
 * get parameters from ffmpeg videoencoder parameter file.
 * get does always fetch all values for all known keywords
 * and delivers default values for all params, that are not
 * found in the file.
 */
void
gap_ffpar_get(const char *filename, GapGveFFMpegValues *ffpar_ptr)
{
  GapVinKeyList    *keylist;
  gint              cnt_scanned_items;

  /* init with defaults the case where no video_info file available
   * (use preset index 0 for the default settings)
   */
  gap_enc_ffmpeg_main_init_preset_params(ffpar_ptr, 0);

  keylist = gap_vin_new_keylist();
  p_set_master_keywords(keylist, ffpar_ptr);

  cnt_scanned_items = gap_vin_scann_filevalues(keylist, filename);
  gap_vin_free_keylist(keylist);

  if(cnt_scanned_items <= 0)
  {
    g_message(_("Could not read ffmpeg video encoder parameters from file:\n%s"), filename);
  }


  if(gap_debug)
  {
    printf("gap_ffpar_get: params loaded: ffpar_ptr:%d\n", (int)ffpar_ptr);
    printf("format_name  %s\n", ffpar_ptr->format_name);
    printf("vcodec_name  %s\n", ffpar_ptr->vcodec_name);
    printf("acodec_name  %s\n", ffpar_ptr->acodec_name);
    printf("title  %s\n", ffpar_ptr->title);
    printf("author  %s\n", ffpar_ptr->author);
    printf("copyright  %s\n", ffpar_ptr->copyright);
    printf("comment  %s\n", ffpar_ptr->comment);
    printf("passlogfile  %s\n", ffpar_ptr->passlogfile);
    
    printf("pass  %d\n", (int)ffpar_ptr->pass);
    printf("audio_bitrate  %d\n", (int)ffpar_ptr->audio_bitrate);
    printf("video_bitrate  %d\n", (int)ffpar_ptr->video_bitrate);
    printf("gop_size  %d\n", (int)ffpar_ptr->gop_size);
    printf("intra  %d\n", (int)ffpar_ptr->intra);
    printf("qscale  %f\n", (float)ffpar_ptr->qscale);
    printf("qmin  %d\n", (int)ffpar_ptr->qmin);
    printf("qmax  %d\n", (int)ffpar_ptr->qmax);
    printf("qdiff  %d\n", (int)ffpar_ptr->qdiff);
    printf("qblur  %f\n", (float)ffpar_ptr->qblur);
    printf("qcomp  %f\n", (float)ffpar_ptr->qcomp);
    printf("rc_init_cplx  %f\n", (float)ffpar_ptr->rc_init_cplx);
    printf("b_qfactor  %f\n", (float)ffpar_ptr->b_qfactor);
    printf("i_qfactor  %f\n", (float)ffpar_ptr->i_qfactor);
    printf("b_qoffset  %f\n", (float)ffpar_ptr->b_qoffset);
    printf("i_qoffset  %f\n", (float)ffpar_ptr->i_qoffset);
    printf("bitrate_tol  %d\n", (int)ffpar_ptr->bitrate_tol);
    printf("maxrate_tol  %d\n", (int)ffpar_ptr->maxrate_tol);
    printf("minrate_tol  %d\n", (int)ffpar_ptr->minrate_tol);
    printf("bufsize  %d\n", (int)ffpar_ptr->bufsize);
    printf("motion_estimation  %d\n", (int)ffpar_ptr->motion_estimation);
    printf("dct_algo  %d\n", (int)ffpar_ptr->dct_algo);
    printf("idct_algo  %d\n", (int)ffpar_ptr->idct_algo);
    printf("strict  %d\n", (int)ffpar_ptr->strict);
    printf("mb_qmin  %d\n", (int)ffpar_ptr->mb_qmin);
    printf("mb_qmax  %d\n", (int)ffpar_ptr->mb_qmax);
    printf("mb_decision  %d\n", (int)ffpar_ptr->mb_decision);
    printf("aic  %d\n", (int)ffpar_ptr->aic);
    printf("umv  %d\n", (int)ffpar_ptr->umv);
    printf("b_frames  %d\n", (int)ffpar_ptr->b_frames);
    printf("mv4  %d\n", (int)ffpar_ptr->mv4);
    printf("partitioning  %d\n", (int)ffpar_ptr->partitioning);
    printf("packet_size  %d\n", (int)ffpar_ptr->packet_size);
    printf("bitexact  %d\n", (int)ffpar_ptr->bitexact);
    printf("set_aspect_ratio  %d\n", (int)ffpar_ptr->set_aspect_ratio);
    printf("dont_recode_flag  %d\n", (int)ffpar_ptr->dont_recode_flag);
    printf("factor_aspect_ratio  %f\n", (float)ffpar_ptr->factor_aspect_ratio);
  }
}  /* end gap_ffpar_get */


/* --------------------------
 * gap_ffpar_set
 * --------------------------
 * (re)write the specified ffmpeg videoencode parameter file
 */
int
gap_ffpar_set(const char *filename, GapGveFFMpegValues *ffpar_ptr)
{
  GapVinKeyList    *keylist;
  int          l_rc;

  keylist = gap_vin_new_keylist();

  p_set_master_keywords(keylist, ffpar_ptr);
  l_rc = gap_vin_rewrite_file(keylist
                          ,filename
			  ,"# GIMP / GAP FFMPEG videoencoder parameter file"   /*  hdr_text */
			  ,")"                                                 /* terminate char */
			  );

  gap_vin_free_keylist(keylist);
  
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
