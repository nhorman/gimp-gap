#include <wavplay.h>
#include <wavfile.h>
#include <client.h>


#define USE_AUDIOCLIENT_WAVPLAY
#ifdef USE_AUDIOCLIENT_WAVPLAY

/* AudioPlayerCLient Wrapper Procedures
 * Implementation for wavplay client
 * can use MACRO definitions for most Procedures
 */


#define APCL_ErrFunc ErrFunc

/* apcl_volume: volume must be a value between 0.0 and 1.0 */
extern int   apcl_volume(double volume, int flags,APCL_ErrFunc erf);
extern pid_t apcl_get_serverpid(void);

#define apcl_bye(flags,erf) tosvr_cmd(ToSvr_Bye,flags,erf)	/* Tell server to exit */
#define apcl_play(flags,erf) tosvr_cmd(ToSvr_Play,flags,erf)	/* Tell server to play */
#define apcl_pause(flags,erf) tosvr_cmd(ToSvr_Pause,flags,erf) /* Tell server to pause */
#define apcl_stop(flags,erf) tosvr_cmd(ToSvr_Stop,flags,erf)	/* Tell server to stop */
#define apcl_restore(flags,erf) tosvr_cmd(ToSvr_Restore,flags,erf) /* Tell server to restore settings */
#define apcl_semreset(flags,erf) tosvr_cmd(ToSvr_SemReset,flags,erf) /* Tell server to reset semaphores */

#define apcl_start(erf) tosvr_start(erf)
#define apcl_path(path,flags,erf) tosvr_path(path,flags,erf)	/* Tell server a pathname */
#define apcl_sampling_rate(rate,flags,erf) tosvr_sampling_rate(flags,erf,rate)	/* Tell server a pathname */
#define apcl_start_sample(offs,flags,erf) tosvr_start_sample(flags,erf,offs)	/* Tell server a pathname */

#endif
