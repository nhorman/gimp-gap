#include <gtk/gtk.h>
#include <client.c>

#define USE_AUDIOCLIENT_WAVPLAY
#ifdef USE_AUDIOCLIENT_WAVPLAY

/* AudioPlayerCLient Wrapper Procedures
 * Implementation for wavplay client
 */


#define APCL_ErrFunc ErrFunc

int apcl_volume(double volume, int flags,APCL_ErrFunc erf)
{
  static double old_volume = -1.0;
  
  /* todo: the wavplay server has NO COMMAND to adjust the audio volume
   *       (for now the user can call gnome-volume-control utility
   *        but this should be also pssible from here too.
   */
  if(old_volume != volume)
  {
    printf("** Adjusting the Audio Volume is not implemented yet! vol=%.2f\n", (float)volume);
    old_volume = volume;
  }
  return(TRUE);
}

pid_t 
apcl_get_serverpid(void)
{
  return(svrPID);
}

#endif
