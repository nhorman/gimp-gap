/* gap_audio_wav.c
 *
 *  GAP tool procedures
 *  for RIFF WAVE format audiofile handling.
 *
 */

/*
 * 2004.04.10  hof  - created
 *
 */
#include "sys/types.h"
#include "sys/stat.h"
#include "unistd.h"

#include "libgimp/gimp.h"
#include "gap_audio_wav.h"

extern int gap_debug;

/* if G_BYTE_ORDER == G_BIG_ENDIAN */
#if G_BYTE_ORDER == G_LITTLE_ENDIAN

typedef union t_wav_16bit_int
{
  gint16  value;
  guint16 uvalue;
  struct
  {
    guchar lsb;
    guchar msb;
  } bytes;
} t_wav_16bit_int;

typedef union t_wav_32bit_int
{
  gint32  value;
  guint32 uvalue;
  struct
  {
    guchar b0;  /* lsb */
    guchar b1;
    guchar b2;
    guchar b3;  /* msb */
  } bytes;
} t_wav_32bit_int;

#else

typedef union t_wav_16bit_int
{
  gint16 value;
  struct
  {
    guchar msb;
    guchar lsb;
  } bytes;
} t_wav_16bit_int;

typedef union t_wav_32bit_int
{
  gint32 value;
  struct
  {
    guchar b3; /* msb */
    guchar b2;
    guchar b1;
    guchar b0; /* lsb */
  } bytes;
} t_wav_32bit_int;

#endif



void
gap_audio_wav_write_gint16(FILE *fp, gint16 val)
{
   t_wav_16bit_int l_val;
   
   l_val.value = val;

   /* write 16bit sample to file (filedata has always lsb first) */   
   fputc((int)l_val.bytes.lsb, fp);
   fputc((int)l_val.bytes.msb, fp);   
}

void
gap_audio_wav_write_gint32(FILE *fp, gint32 val)
{
  t_wav_32bit_int l_val;
  
  l_val.value = val;
  
  /* write 32bit integer to file (lsb first) */   
  fputc((int)l_val.bytes.b0, fp);
  fputc((int)l_val.bytes.b1, fp);
  fputc((int)l_val.bytes.b2, fp);
  fputc((int)l_val.bytes.b3, fp);   
}


/* ---------------------------------
 * gap_audio_wav_write_header
 * ---------------------------------
 */
void
gap_audio_wav_write_header(FILE *fp
                         , gint32 nsamples
                         , gint32 channels
                         , gint32 samplerate
                         , gint32 bytes_per_sample
                         , gint32 bits
                         )
{
  gint32 l_total_len;  /* is the total filesize - 8 bytes (for RIFF keyword and 4 bytes lengthfield) */
  gint32 l_data_len;
  gint32 l_fmt_len;
  gint16 l_format_tag;
  gint32 l_avg_bytes_per_sec;


  l_data_len  = nsamples * bytes_per_sample;
  l_total_len = l_data_len + 36;
  l_fmt_len = 16;      /* length of 'fmt ' chunk has fix length of 16 byte */
  
  l_format_tag = 1;     /* 1 is tag for PCM_WAVE_FORMAT   */
  l_avg_bytes_per_sec = samplerate * bytes_per_sample;

  /* 00 - 03 */  fprintf(fp, "RIFF");
  /* 04 - 07 */  gap_audio_wav_write_gint32(fp, l_total_len);
  /* 08 - 15 */  fprintf(fp, "WAVEfmt ");

  /* 16 - 19 */  gap_audio_wav_write_gint32(fp, (gint32)l_fmt_len);
  /* 20 - 21 */  gap_audio_wav_write_gint16(fp, (gint16)l_format_tag);
  /* 22 - 23 */  gap_audio_wav_write_gint16(fp, (gint16)channels);
  /* 24 - 27 */  gap_audio_wav_write_gint32(fp, samplerate);
  /* 28 - 31 */  gap_audio_wav_write_gint32(fp, l_avg_bytes_per_sec);
  /* 32 - 33 */  gap_audio_wav_write_gint16(fp, (gint16)bytes_per_sample);
  /* 34 - 35 */  gap_audio_wav_write_gint16(fp, (gint16)bits);

  /* 36 - 39 */  fprintf(fp, "data");
  /* 40 - 43 */  gap_audio_wav_write_gint32(fp, l_data_len);
}  /* end gap_audio_wav_write_header */



/* ----------------------------
 * p_wav_open_seek_data_private
 * ----------------------------
 * Open a RIFF WAVE fmt file
 * and seek position to the 1.st data byte.
 *
 * return NULL   on ERROR
 *        FILE*  filepointer to the opened audiofile
 */
static FILE *
p_wav_open_seek_data_private(const char *filename, unsigned char *audata)
{
  FILE        *fp;
  struct stat  l_stat_buf;
  size_t       l_len_to_read;
  size_t       l_len_read;
  

  /* get File Length */
  if (0 != stat(filename, &l_stat_buf))
  {
    /* stat error (file does not exist) */
    return(NULL);
  }

  if (l_stat_buf.st_size < 48)
  {
    return(NULL);  /* too short for WAVE file */
  }

  fp = fopen(filename, "rb");
  if(fp == NULL)
  {
    return(NULL);
  }

  l_len_to_read = 44;
  l_len_read = fread(&audata[0], 1, l_len_to_read, fp);

  /* searching for data chunk */
  while (l_len_to_read == l_len_read)
  {
    if(gap_debug)
    {
      printf("chunkID: %c%c%c%c\n", audata[36], audata[37], audata[38], audata[39]);
      printf("chunkID(hex): %x %x %x %x\n", audata[36], audata[37], audata[38], audata[39]);
    }
    if((audata[36] == 'd')
    && (audata[37] == 'a')
    && (audata[38] == 't')
    && (audata[39] == 'a'))
    {
      break;  /* data chunk found, stop searching */
    }
    /* skip unknown chunk(s) */
    l_len_to_read = audata[40] + (256 * (long)audata[41]) + (65536 * (long)audata[42]) + (16777216 * (long)audata[43]);
    if(gap_debug) printf("skip length: %d\n", (int)l_len_to_read);
    if(l_len_to_read <= 0 || l_len_to_read > l_stat_buf.st_size)
    {
      break;  /* chunk length contains nonsens, give up searching for data chunk */
    }
    fseek(fp, l_len_to_read, SEEK_CUR);

    /* read next chunk header and length (4+4 bytes) */
    l_len_to_read = 8;
    l_len_read = fread(&audata[36], 1, l_len_to_read, fp);
  }
  
  return(fp);
}   /* end p_wav_open_seek_data_private */

/* ----------------------------
 * gap_audio_wav_open_seek_data
 * ----------------------------
 */
FILE *
gap_audio_wav_open_seek_data(const char *filename)
{
  FILE *fp;
  unsigned char audata[60];

  fp = p_wav_open_seek_data_private(filename, &audata[0]);
  return(fp);
}  /* end gap_audio_wav_open_seek_data */


/* -----------------------
 * gap_audio_wav_file_check
 * -----------------------
 * check file if it is a RIFF WAVE fmt file
 * and obtain the header informatains
 * return -1 on ERROR
 *         0 if OK
 */
int
gap_audio_wav_file_check(const char *audfile, long *sample_rate, long *channels
                     , long *bytes_per_sample, long *bits, long *samples)
{
  /* note: this code is a quick hack and was inspired by a look at some
   *       .wav files. It will work on many .wav files.
   *       but it also may fail to identify some .wav files.
   */
  FILE        *fp;
  unsigned char audata[60];
  long  l_data_len;
  int          l_rc;

  l_rc = -1;  /* prepare retcode NOT OK */

  fp = p_wav_open_seek_data_private(audfile, &audata[0]);
  if(fp == NULL)
  {
    if(gap_debug) printf ("p_audio_wav_file_check: open(read) error on '%s'\n", audfile);
    return(l_rc);
  }
  fclose(fp);

  /* check if audata is a RIFF WAVE format file */
  if((audata[0]  == 'R')      /* check for RIFF identifier */
  && (audata[1]  == 'I')
  && (audata[2]  == 'F')
  && (audata[3]  == 'F')
  && (audata[8]  == 'W')      /* check for "WAVEfmt " chunk identifier */
  && (audata[9]  == 'A')
  && (audata[10] == 'V')
  && (audata[11] == 'E')
  && (audata[12] == 'f')
  && (audata[13] == 'm')
  && (audata[14] == 't')
  && (audata[15] == ' ')
  && (audata[36] == 'd')     /* check for "data " chunk identifier */
  && (audata[37] == 'a')
  && (audata[38] == 't')
  && (audata[39] == 'a'))
  {
     *sample_rate = audata[24] + (256 * (long)audata[25])+ (65536 * (long)audata[26]) + (16777216 * (long)audata[27]);
     *channels = audata[22] + (256 * (long)audata[23]);
     *bytes_per_sample = audata[32] + (256 * (long)audata[33]);
     *bits = audata[34] + (256 * (long)audata[35]);
     l_data_len = audata[40] + (256 * (long)audata[41]) + (65536 * (long)audata[42]) + (16777216 * (long)audata[43]);

     *samples = l_data_len / (*bits / 8);
     l_rc = 0; /* OK seems to be a WAVE file */

     if(gap_debug)
     {
       printf ("p_audio_wav_file_check: WAVE FILE: %s\n", audfile);
       printf (" sample_rate: %d\n", (int)*sample_rate );
       printf (" channels: %d\n", (int)*channels);
       printf (" bytes_per_sample: %d\n", (int)*bytes_per_sample );
       printf (" bits: %d\n", (int) *bits);
       printf (" summary samples (all channels): %d\n", (int)*samples);
       printf (" l_data_len: %d\n", (int)l_data_len);
     }
  }

  return l_rc;
}   /* end gap_audio_wav_file_check */



/* -----------------------
 * gap_audio_wav_16bit_save
 * -----------------------
 * check file if it is a RIFF WAVE fmt file
 * and obtain the header informatains
 * return -1 on ERROR
 *         0 if OK
 */
int
gap_audio_wav_16bit_save(const char *wavfile
                    , int channels
                    , unsigned short *left_ptr
                    , unsigned short *right_ptr
                    , int samplerate
                    , long total_samples
                    )
{
  FILE *fp;
  gint32 l_bytes_per_sample;
  gint32 l_ii;
  
  if(channels == 1) { l_bytes_per_sample = 2;}  /* mono */
  else              { l_bytes_per_sample = 4; channels = 2; }  /* stereo */
  

  fp = fopen(wavfile, "wb");
  if (fp)
  {
    /* write the header */
    gap_audio_wav_write_header(fp
                            , (gint32)total_samples
                            , channels                           /* cannels 1 or 2 */
                            , samplerate
                            , l_bytes_per_sample
                            , 16                          /* 16 bit sample resolution */
                            );
 
    /* write 16 bit wave datasamples 
     * sequence mono:    (lo, hi)
     * sequence stereo:  (lo_left, hi_left, lo_right, hi_right)
     */
    for(l_ii=0; l_ii < total_samples; l_ii++)
    {
       gap_audio_wav_write_gint16(fp, *left_ptr);
       left_ptr++;
       if(channels > 1)
       {
         gap_audio_wav_write_gint16(fp, *right_ptr);
         right_ptr++;
       }
     }
    
    fclose(fp);
  }
  return(0); /* OK */
}  /* end gap_audio_wav_16bit_save */                     
