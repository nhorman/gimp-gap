/* $Header$
 * Warren W. Gay VE3WWG		Wed Feb 26 22:43:33 1997
 *
 * INTERPRET PROCESS TERMINATION:
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
 * 
 * Email:
 *	ve3wwg@yahoo.com
 *	wgay@mackenziefinancial.com
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
 * Revision 1.1.1.1  1999/11/21 19:50:56  wwg
 * Import wavplay-1.3 into CVS
 *
 * Revision 1.1  1997/04/14 00:07:16  wwg
 * Initial revision
 *
 */
static const char rcsid[] = "@(#)procterm.c $Revision$";

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

/*
 * Return an interpreted text description of a process termination event.
 */
char *
ProcTerm(int ProcStat) {
	int ExitCode, Signal;
	static char buf[80];

	if ( ProcStat == 0 || WIFEXITED(ProcStat) ) {
		if ( !ProcStat )
			ExitCode = 0;					/* exit(0) */
		else	ExitCode = WEXITSTATUS(ProcStat);		/* exit(code) */
		sprintf(buf,"exit(%d)",ExitCode);

	} else if ( WIFSIGNALED(ProcStat) ) {
		Signal = WTERMSIG(ProcStat);				/* Signalled or aborted */
		sprintf(buf,"signal(%d)%s",Signal,WCOREDUMP(ProcStat)
				? ", core file written"
				: "");

	} else if ( WIFSTOPPED(ProcStat) ) {
		Signal = WSTOPSIG(ProcStat);				/* Stopped process */
		sprintf(buf,"stopsig(%d)",Signal);

	} else	sprintf(buf,"Weird termination 0x%04X",ProcStat);	/* Something weird */

	return buf;
}

/* $Source$ */
