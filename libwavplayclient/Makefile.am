## Process this file with automake to produce Makefile.in

# FIXME: this Makefile.am is not finished 
#        (dont know howto create a library using autoconf/automake)
# FIXME: there is no support for PLATFORM_WIN32
#

libwavplayclientincludedir = $(includedir)/gimp-$(LT_RELEASE)/libwavplayclient

if PLATFORM_WIN32
AM_CPPFLAGS = \
	-DGAP_DISABLE_WAV_AUDIOSUPPORT
else
AM_CPPFLAGS = \
	-DGAP_ENABLE_WAV_AUDIOSUPPORT
endif

INCLUDES = \
	-I$(top_srcdir)	\
	$(GLIB_CFLAGS) \
	$(GMODULE_CFLAGS) \
	-I$(includedir)


lib_LTLIBRARIES = libwavplayclient-1.4.la

libwavplayclient_1_4_la_SOURCES = \
	client.h		\
	client.c		\
	msg.c			\
	procterm.c		\
	wavfile.c		\
	wavfile.h		\
	wavplay.h		\
	wpc_client.c		\
	wpc_lib.h		\
	wpc_msg.c		\
	wpc_procterm.c		\
	wpc_wavfile.c

libwavplayclientinclude_HEADERS = \
	wpc_lib.h		\
	client.h		\
	client.c		\
	msg.c			\
	procterm.c		\
	wavfile.c		\
	wavfile.h		\
	wavplay.h

EXTRA_HEADERS =

libwavplayclient_1_4_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)	\
	$(no_undefined) 					\
	$(libwavplayclient_export_symbols)

libwavplayclient_1_4_la_DEPENDENCIES = $(gimpmodule_def)

libwavplayclient_1_4_la_LIBADD = \
	$(top_builddir)/libgimpbase/libgimpbase-$(LT_RELEASE).la	\
	$(GLIB_LIBS)							\
	$(GMODULE_LIBS)

install-data-local: install-ms-lib install-libtool-import-lib

uninstall-local: uninstall-ms-lib uninstall-libtool-import-lib
