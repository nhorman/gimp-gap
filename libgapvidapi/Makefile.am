## Process this file with automake to produce Makefile.in


libexecdir = $(GAPLIBDIR)

noinst_LIBRARIES = libgapvidapi.a

AM_CPPFLAGS = \
	-DLOCALEDIR=\""$(LOCALEDIR)"\"

INCLUDES = \
	-I$(top_srcdir)	\
	$(GLIB_CFLAGS)	\
	$(GIMP_CFLAGS)	\
	$(GAPVIDEOAPI_EXTINCS)	\
	-I$(includedir)


libgapvidapi_a_SOURCES = 	\
	gap_vid_api.c		\
	gap_vid_api.h

# the current implementation includes this
# .c sourcefiles in gap_vid_api.c (except example.c)
EXTRA_DIST = \
	gap_vid_api_gimp.c	\
	gap_vid_api_ffmpeg.c	\
	gap_vid_api_mpeg3.c	\
	gap_vid_api_mpeg3toc.c	\
	gap_vid_api_quicktime.c	\
	gap_vid_api_util.c	\
	gap_vid_api_vidindex.c	\
	example.c
