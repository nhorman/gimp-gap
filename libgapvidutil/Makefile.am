## Process this file with automake to produce Makefile.in

if GAP_VIDEOAPI_SUPPORT
GAPVIDEOAPI = $(top_builddir)/libgapvidapi/libgapvidapi.a  $(GAPVIDEOAPI_EXTLIBS)
INC_GAPVIDEOAPI = -I$(top_srcdir)/libgapvidapi $(GAPVIDEOAPI_EXTINCS)
endif

INC_LIBGAPBASE = -I$(top_srcdir)/libgapbase


noinst_LIBRARIES = libgapvidutil.a

AM_CPPFLAGS = \
	-DLOCALEDIR=\""$(LOCALEDIR)"\"

INCLUDES = \
	-I$(top_srcdir)	\
	-I$(top_srcdir)/gap	\
	$(INC_LIBGAPBASE)	\
	$(GLIB_CFLAGS)	\
	$(GIMP_CFLAGS)	\
	$(INC_GAPVIDEOAPI)	\
	-I$(includedir)


libgapvidutil_a_SOURCES = \
	gap_gve_jpeg.c		\
	gap_gve_jpeg.h		\
	gap_gve_png.c		\
	gap_gve_png.h		\
	gap_gve_misc_util.c	\
	gap_gve_misc_util.h	\
	gap_gve_raw.c		\
	gap_gve_raw.h		\
	gap_gve_sox.c		\
	gap_gve_sox.h		\
	gap_gve_story.c		\
	gap_gve_story.h		\
	gap_gve_xvid.c		\
	gap_gve_xvid.h		\
	gap_gvetypes.h		\
	gap_libgapvidutil.h

