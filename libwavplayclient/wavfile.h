/* $Header$
 * Copyright:	wavfile.h (c) Erik de Castro Lopo  erikd@zip.com.au
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
 * Revision 1.2  1997/04/14 01:03:06  wwg
 * Fixed copyright, to reflect Erik's ownership of this file.
 *
 * Revision 1.1  1997/04/14 00:58:10  wwg
 * Initial revision
 */
#ifndef _wavfile_h
#define _wavfile_h "@(#)wavfile.h $Revision$"

#define WW_BADOUTPUTFILE	1
#define WW_BADWRITEHEADER	2

#define WR_BADALLOC		3
#define WR_BADSEEK		4
#define WR_BADRIFF		5
#define WR_BADWAVE		6
#define WR_BADFORMAT		7
#define WR_BADFORMATSIZE	8

#define WR_NOTPCMFORMAT		9
#define WR_NODATACHUNK		10
#define WR_BADFORMATDATA	11

extern int WaveWriteHeader(int wavefile,int channels,u_long samplerate,int sampbits,u_long samples,ErrFunc erf);
extern int WaveReadHeader(int wavefile,int *channels,u_long *samplerate,int *samplebits,u_long *samples,u_long *datastart,ErrFunc erf);

#if 0
extern char *WaveFileError(int error);
#endif

#endif /* _wavfile_h_ */

/* $Source$ */
