/* gap_enc_ffmpeg_callbacks.h
 * 2004.05.12 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * This Module contains FFMPEG specific Video Encoder GUI Callback Procedures
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
 * version 2.1.0a;  2004.06.05   hof: update params from ffmpeg 0.4.6 to 0.4.8
 * version 2.1.0a;  2004.05.12 : created
 */


#ifndef GAP_ENC_FFMPEG_CALLBACKS_H
#define GAP_ENC_FFMPEG_CALLBACKS_H

#include <config.h>
#include "gap_libgapvidutil.h"
#include "gap_libgimpgap.h"

#include "gap_enc_ffmpeg_main.h"

#include <gtk/gtk.h>


void
on_ff_response (GtkWidget *widget,
                 gint       response_id,
                 GapGveFFMpegGlobalParams *gpp);

void
on_ff_fileformat_optionmenu  (GtkWidget     *wgt_item,
                              GapGveFFMpegGlobalParams *gpp);

void
on_ff_vid_codec_optionmenu  (GtkWidget     *wgt_item,
                             GapGveFFMpegGlobalParams *gpp);

void
on_ff_aud_codec_optionmenu  (GtkWidget     *wgt_item,
                             GapGveFFMpegGlobalParams *gpp);

void
on_ff_aud_bitrate_optionmenu  (GtkWidget     *wgt_item,
                               GapGveFFMpegGlobalParams *gpp);

void
on_ff_motion_estimation_optionmenu  (GtkWidget     *wgt_item,
                                     GapGveFFMpegGlobalParams *gpp);

void
on_ff_dct_algo_optionmenu  (GtkWidget     *wgt_item,
                           GapGveFFMpegGlobalParams *gpp);

void
on_ff_idct_algo_optionmenu  (GtkWidget     *wgt_item,
                           GapGveFFMpegGlobalParams *gpp);

void
on_ff_mb_decision_optionmenu  (GtkWidget     *wgt_item,
                           GapGveFFMpegGlobalParams *gpp);

void
on_ff_presets_optionmenu  (GtkWidget     *wgt_item,
                           GapGveFFMpegGlobalParams *gpp);


void
on_ff_aud_bitrate_spinbutton_changed   (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp);

void
on_ff_vid_bitrate_spinbutton_changed   (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp);

void
on_ff_qscale_spinbutton_changed        (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp);

void
on_ff_qmin_spinbutton_changed          (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp);

void
on_ff_qmax_spinbutton_changed          (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp);

void
on_ff_qdiff_spinbutton_changed         (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp);

void
on_ff_gop_size_spinbutton_changed      (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp);

void
on_ff_intra_checkbutton_toggled        (GtkToggleButton *checkbutton,
                                        GapGveFFMpegGlobalParams *gpp);

void
on_ff_aic_checkbutton_toggled          (GtkToggleButton *checkbutton,
                                        GapGveFFMpegGlobalParams *gpp);
void
on_ff_umv_checkbutton_toggled          (GtkToggleButton *checkbutton,
                                        GapGveFFMpegGlobalParams *gpp);
					

void
on_ff_bitexact_checkbutton_toggled     (GtkToggleButton *checkbutton,
                                        GapGveFFMpegGlobalParams *gpp);

void
on_ff_aspect_checkbutton_toggled       (GtkToggleButton *checkbutton,
                                        GapGveFFMpegGlobalParams *gpp);

void
on_ff_b_frames_checkbutton_toggled     (GtkToggleButton *checkbutton,
                                        GapGveFFMpegGlobalParams *gpp);

void
on_ff_mv4_checkbutton_toggled          (GtkToggleButton *checkbutton,
                                        GapGveFFMpegGlobalParams *gpp);

void
on_ff_partitioning_checkbutton_toggled
                                        (GtkToggleButton *checkbutton,
                                        GapGveFFMpegGlobalParams *gpp);

void
on_ff_qblur_spinbutton_changed         (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp);

void
on_ff_qcomp_spinbutton_changed         (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp);

void
on_ff_rc_init_cplx_spinbutton_changed  (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp);

void
on_ff_b_qfactor_spinbutton_changed     (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp);

void
on_ff_i_qfactor_spinbutton_changed     (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp);

void
on_ff_b_qoffset_spinbutton_changed     (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp);

void
on_ff_i_qoffset_spinbutton_changed     (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp);

void
on_ff_bitrate_tol_spinbutton_changed   (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp);

void
on_ff_maxrate_tol_spinbutton_changed   (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp);

void
on_ff_minrate_tol_spinbutton_changed   (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp);

void
on_ff_bufsize_spinbutton_changed       (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp);

void
on_ff_passlogfile_entry_changed        (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp);

void
on_ff_passlogfile_filesel_button_clicked (GtkButton       *button,
                                        GapGveFFMpegGlobalParams *gpp);

void
on_ff_pass_checkbutton_toggled         (GtkToggleButton *checkbutton,
                                        GapGveFFMpegGlobalParams *gpp);

void
on_ff_title_entry_changed              (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp);

void
on_ff_author_entry_changed             (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp);

void
on_ff_copyright_entry_changed          (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp);

void
on_ff_comment_entry_changed            (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp);

void
on_fsb__fileselection_destroy          (GtkObject       *object,
                                        GapGveFFMpegGlobalParams *gpp);

void
on_fsb__ok_button_clicked              (GtkButton       *button,
                                        GapGveFFMpegGlobalParams *gpp);

void
on_fsb__cancel_button_clicked          (GtkButton       *button,
                                        GapGveFFMpegGlobalParams *gpp);

void
on_ff_strict_spinbutton_changed        (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp);

void
on_ff_mb_qmin_spinbutton_changed       (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp);

void
on_ff_mb_qmax_spinbutton_changed       (GtkEditable     *editable,
                                        GapGveFFMpegGlobalParams *gpp);

void
on_ff_dont_recodecheckbutton_toggled   (GtkToggleButton *checkbutton,
                                        GapGveFFMpegGlobalParams *gpp);

void
on_ff_dont_recode_checkbutton_toggled  (GtkToggleButton *checkbutton,
                                        GapGveFFMpegGlobalParams *gpp);

#endif
