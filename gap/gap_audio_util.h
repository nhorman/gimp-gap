/* gap_audio_util.h
 *
 *  GAP common encoder audio utility procedures
 *
 */
/* 2002.01.05 hof  removed p_get_wavparams
 */

#ifndef GAP_AUDIO_UTIL_H
#define GAP_AUDIO_UTIL_H


/* GIMP includes */
#include "gtk/gtk.h"
#include "libgimp/gimp.h"


void     gap_audio_util_stereo_split16to16(unsigned char *l_left_ptr,
                  unsigned char *l_right_ptr, unsigned char *l_aud_ptr,
                  long l_data_len);

void     gap_audio_util_dbl_sample_8_to_16(unsigned char insample8,
                  unsigned char *lsb_out,
                  unsigned char *msb_out);

void     gap_audio_util_stereo_split8to16(unsigned char *l_left_ptr,
                  unsigned char *l_right_ptr,
                  unsigned char *l_aud_ptr,
                  long l_data_len);


#endif          /* end  GAP_AUDIO_UTIL_H */
