## Process this file with automake to produce Makefile.in

# FIXME: there is no support for PLATFORM_WIN32 in this release
# the general idea is, to add clientcode variants in the future
#  (for WIN32 and maybe others)
# 
# All that stuff should use the same simple wavplay API on clientside
# but should talk to different audioserver programs.
# (using other server api than wavplayer's API)  
#

libexecdir = $(GAPLIBDIR)
libexec_SCRIPTS = audioconvert_to_wav.sh

noinst_LIBRARIES = libwavplayclient.a

AM_CPPFLAGS = \
	-DLOCALEDIR=\""$(LOCALEDIR)"\"

INCLUDES = \
	-I$(top_srcdir)	\
	$(GLIB_CFLAGS)	\
	$(GIMP_CFLAGS)	\
	-I$(includedir)


libwavplayclient_a_SOURCES = \
	wpc_client.c	\
	wpc_lib.h	\
	wpc_msg.c	\
	wpc_procterm.c	\
	wpc_wavfile.c

EXTRA_DIST = \
	README		\
	client.h	\
	client.c	\
	msg.c		\
	procterm.c	\
	wavfile.c	\
	wavfile.h	\
	wavplay.h	\
	$(libexec_SCRIPTS)
