/* gap_audio_wav.h
 *
 *  GAP tool procedures
 *  for RIFF WAVE format audifile handling.
 *
 */

/*
 * 2004.04.10  hof  - created
 *
 */

#ifndef GAP_AUDIO_WAV_H
#define GAP_AUDIO_WAV_H

#include "stdio.h"
#include "libgimp/gimp.h"


void        gap_audio_wav_write_gint16(FILE *fp, gint16 val);
void        gap_audio_wav_write_gint32(FILE *fp, gint32 val);
void        gap_audio_wav_write_header(FILE *fp
                         , gint32 nsamples
                         , gint32 channels
                         , gint32 samplerate
                         , gint32 bytes_per_sample
                         , gint32 bits
                         );
int         gap_audio_wav_file_check(const char *audfile
                     , long *sample_rate
                     , long *channels
                     , long *bytes_per_sample
                     , long *bits
                     , long *samples
                     );

FILE *      gap_audio_wav_open_seek_data(const char *filename);

int         gap_audio_wav_16bit_save(const char *wavfile
                    , int channels
                    , unsigned short *left_ptr
                    , unsigned short *right_ptr
                    , int samplerate
                    , long total_samples);

int         gap_audio_playlist_wav_file_check(const char *audfile, long *sample_rate, long *channels
                     , long *bytes_per_sample, long *bits, long *samples
		     , long *all_playlist_references
		     , long *valid_playlist_references
		     , long desired_samplerate
		     );

#endif
