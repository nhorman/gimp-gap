/* $Header$
 * Warren W. Gay VE3WWG		Wed Feb 26 22:01:36 1997
 *
 * CLIENT.C HEADER FILE:
 *
 * 	X LessTif WAV Play :
 * 
 * 	Copyright (C) 1997  Warren W. Gay VE3WWG
 * 
 * This  program is free software; you can redistribute it and/or modify it
 * under the  terms  of  the GNU General Public License as published by the
 * Free Software Foundation version 2 of the License.
 * 
 * This  program  is  distributed  in  the hope that it will be useful, but
 * WITHOUT   ANY   WARRANTY;   without   even  the   implied   warranty  of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details (see enclosed file COPYING).
 * 
 * You  should have received a copy of the GNU General Public License along
 * with this  program; if not, write to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 * Send correspondance to:
 * 
 * 	Warren W. Gay VE3WWG
 * 	5536 Montevideo Road #17
 *	Mississauga, Ontario L5N 2P4
 * 
 * Email:
 * 	wwg@ica.net			(current ISP of the month :-) )
 * 	bx249@freenet.toronto.on.ca	(backup)
 *
 * $Log$
 * Revision 1.1  2003/09/08 08:23:25  neo
 * 2003-09-08  Sven Neumann  <sven@gimp.org>
 *
 * 	* gap/gap_player_main.c
 * 	* gap/gap_name2layer_main.c: fixed harmless but annoying compiler
 * 	warnings.
 *
 * 	* libwavplayclient/client.c
 * 	* libwavplayclient/wavfile.c: include <glib.h> for g_strerror(),
 * 	removed redefinition of TRUE/FALSE.
 *
 * 2003-09-07  Wolfgang Hofer <hof@gimp.org>
 *
 * 	Playback Module:
 *  	  added ctrl/alt modifiers for callback on_go_button_clicked
 *  	  added optional audiosupport based on wavplay (Linux only)
 *  	    (Dont know how to make/install libwavplayclient as library
 *  	     the gap/wpc_* files are just a workaround to compile without
 * 	     libwavplayclient)
 *
 *  	VCR-Navigator:
 *  	 bugfix: show waiting cursor at long running ops
 *  		 to set range begin/end frame
 *  	 bugfix: frame timing labels did always show time at 24.0 fps
 *  		 now use the current framerate as it should be.
 *  	* configure.in	      new parameter: --disable-audiosupport
 *  	* gap/Makefile.am     variable GAP_AUDIOSUPPORT (for conditional audiosupport)
 *  	* gap/gap_player_dialog.c
 *  	* gap/gap_vin.c (force timezoom value >= 1 in case .vin has 0 value)
 *  	* gap/gap_navigator_dialog.c
 *  	* gap/gap_mpege.c	     (bugfix for #121145 translation text)
 *  	* gap/gap_lib.c [.h]
 *  	* gap/gap_timeconv.c [.h]
 *  	new files ()
 *  	* gap/wpc_client.c
 *  	* gap/wpc_lib.h
 *  	* gap/wpc_msg.c
 *  	* gap/wpc_procterm.c
 *  	* gap/wpc_wavfile.c
 *  	* libwavplayclient/wpc_client.c
 *  	* libwavplayclient/wpc_lib.h
 *  	* libwavplayclient/wpc_msg.c
 *  	* libwavplayclient/wpc_procterm.c
 *  	* libwavplayclient/wpc_wavfile.c
 *  	* libwavplayclient/COPYING
 *  	* libwavplayclient/Makefile.am       /* Not finished and currently not OK */
 *  	* libwavplayclient/README
 *  	* libwavplayclient/client.c
 *  	* libwavplayclient/client.h
 *  	* libwavplayclient/msg.c
 *  	* libwavplayclient/procterm.c
 *  	* libwavplayclient/wavfile.c
 *  	* libwavplayclient/wavfile.h
 *  	* libwavplayclient/wavplay.h
 *
 * Revision 1.2  1999/12/04 00:01:20  wwg
 * Implement wavplay-1.4 release changes
 *
 * Revision 1.1.1.1  1999/11/21 19:50:56  wwg
 * Import wavplay-1.3 into CVS
 *
 * Revision 1.1  1997/04/14 01:01:16  wwg
 * Initial revision
 *
 * Revision 1.1  1997/04/14 00:11:03  wwg
 * Initial revision
 *
 * $Log$
 * Revision 1.1  2003/09/08 08:23:25  neo
 * 2003-09-08  Sven Neumann  <sven@gimp.org>
 *
 * 	* gap/gap_player_main.c
 * 	* gap/gap_name2layer_main.c: fixed harmless but annoying compiler
 * 	warnings.
 *
 * 	* libwavplayclient/client.c
 * 	* libwavplayclient/wavfile.c: include <glib.h> for g_strerror(),
 * 	removed redefinition of TRUE/FALSE.
 *
 * 2003-09-07  Wolfgang Hofer <hof@gimp.org>
 *
 * 	Playback Module:
 *  	  added ctrl/alt modifiers for callback on_go_button_clicked
 *  	  added optional audiosupport based on wavplay (Linux only)
 *  	    (Dont know how to make/install libwavplayclient as library
 *  	     the gap/wpc_* files are just a workaround to compile without
 * 	     libwavplayclient)
 *
 *  	VCR-Navigator:
 *  	 bugfix: show waiting cursor at long running ops
 *  		 to set range begin/end frame
 *  	 bugfix: frame timing labels did always show time at 24.0 fps
 *  		 now use the current framerate as it should be.
 *  	* configure.in	      new parameter: --disable-audiosupport
 *  	* gap/Makefile.am     variable GAP_AUDIOSUPPORT (for conditional audiosupport)
 *  	* gap/gap_player_dialog.c
 *  	* gap/gap_vin.c (force timezoom value >= 1 in case .vin has 0 value)
 *  	* gap/gap_navigator_dialog.c
 *  	* gap/gap_mpege.c	     (bugfix for #121145 translation text)
 *  	* gap/gap_lib.c [.h]
 *  	* gap/gap_timeconv.c [.h]
 *  	new files ()
 *  	* gap/wpc_client.c
 *  	* gap/wpc_lib.h
 *  	* gap/wpc_msg.c
 *  	* gap/wpc_procterm.c
 *  	* gap/wpc_wavfile.c
 *  	* libwavplayclient/wpc_client.c
 *  	* libwavplayclient/wpc_lib.h
 *  	* libwavplayclient/wpc_msg.c
 *  	* libwavplayclient/wpc_procterm.c
 *  	* libwavplayclient/wpc_wavfile.c
 *  	* libwavplayclient/COPYING
 *  	* libwavplayclient/Makefile.am       /* Not finished and currently not OK */
 *  	* libwavplayclient/README
 *  	* libwavplayclient/client.c
 *  	* libwavplayclient/client.h
 *  	* libwavplayclient/msg.c
 *  	* libwavplayclient/procterm.c
 *  	* libwavplayclient/wavfile.c
 *  	* libwavplayclient/wavfile.h
 *  	* libwavplayclient/wavplay.h
 *
 * Revision 1.2  1999/12/04 00:01:20  wwg
 * Implement wavplay-1.4 release changes
 *
 * Revision 1.1.1.1  1999/11/21 19:50:56  wwg
 * Import wavplay-1.3 into CVS
 *
 * Revision 1.1  1997/04/14 01:01:16  wwg
 * Initial revision
 *
 */
#ifndef _client_h_
#define _client_h_ "@(#)client.h $Revision$"

extern int tosvr_cmd(MSGTYP cmd,int flags,ErrFunc erf);		/* Simple server command */
extern int tosvr_start(ErrFunc erf);				/* Start server */
extern int tosvr_bye(int flags,ErrFunc erf);			/* Tell server to exit */

#define tosvr_bye(flags,erf) tosvr_cmd(ToSvr_Bye,flags,erf)	/* Tell server to exit */
#define tosvr_play(flags,erf) tosvr_cmd(ToSvr_Play,flags,erf)	/* Tell server to play */
#define tosvr_pause(flags,erf) tosvr_cmd(ToSvr_Pause,flags,erf) /* Tell server to pause */
#define tosvr_stop(flags,erf) tosvr_cmd(ToSvr_Stop,flags,erf)	/* Tell server to stop */
#define tosvr_restore(flags,erf) tosvr_cmd(ToSvr_Restore,flags,erf) /* Tell server to restore settings */
#define tosvr_semreset(flags,erf) tosvr_cmd(ToSvr_SemReset,flags,erf) /* Tell server to reset semaphores */

extern int tosvr_path(const char *path,int flags,ErrFunc erf);	/* Tell server a pathname */
extern int tosvr_bits(int flags,ErrFunc erf,int bits);		/* Tell server bits override */
extern int tosvr_start_sample(int flags, ErrFunc eft, UInt32 sample); /* Tell server to start at sample */
extern int tosvr_sampling_rate(int flags,ErrFunc erf,UInt32 sampling_rate);
extern int tosvr_chan(int flags,ErrFunc erf,Chan chan);		/* Override Mono/Stereo */
extern int tosvr_record(int flags,ErrFunc erf,
	Chan chan_mode,UInt32 sampling_rate,UInt16 data_bits);	/* Start recording */
extern int tosvr_debug(int flags,ErrFunc erf,int bDebugMode);	/* Set debug mode in server */

extern pid_t svrPID;						/* Forked process ID of server */
extern int svrIPC;						/* IPC ID of message queue */

#endif /* _client_h_ */

/* $Source$ */
