## Process this file with automake to produce Makefile.in

libexecdir = $(GIMP_PLUGIN_DIR)/plug-ins

scriptdatadir = $(GIMP_DATA_DIR)/scripts


if GAP_VIDEOAPI_SUPPORT
GAPVIDEOAPI = -L$(top_builddir)/libgapvidapi -lgapvidapi  $(GAPVIDEOAPI_EXTLIBS)
INC_GAPVIDEOAPI = -I$(top_builddir)/libgapvidapi $(GAPVIDEOAPI_EXTINCS)
endif

LIBGIMPGAP = -L$(top_builddir)/gap -lgimpgap
INC_LIBGIMPGAP = -I$(top_builddir)/gap

LIBGAPVIDUTIL  = -L$(top_builddir)/libgapvidutil -lgapvidutil
INC_LIBGAPVIDUTIL = -I$(top_builddir)/libgapvidutil


libexec_PROGRAMS = gap_vid_enc_ffmpeg


gap_vid_enc_ffmpeg_SOURCES = \
	gap_enc_ffmpeg_main.c		\
	gap_enc_ffmpeg_main.h		\
	gap_enc_ffmpeg_gui.c		\
	gap_enc_ffmpeg_gui.h		\
	gap_enc_ffmpeg_callbacks.c	\
	gap_enc_ffmpeg_callbacks.h	


AM_CPPFLAGS = \
	-DGAPLIBDIR=\""$(GAPLIBDIR)"\"	\
	-DLOCALEDIR=\""$(LOCALEDIR)"\"

INCLUDES = \
	-I$(top_srcdir)	\
	-I$(top_srcdir)/libwavplayclient	\
	$(INC_GAPVIDEOAPI)	\
	$(INC_LIBGIMPGAP)	\
	$(INC_LIBGAPVIDUTIL)	\
	$(GIMP_CFLAGS)		\
	-I$(includedir)


LDADD = $(GIMP_LIBS)


# note: sequence of libs matters because LIBGAPVIDUTIL uses both LIBGIMPGAP and GAPVIDEOAPI
#       (if those libs appear before LIBGAPVIDUTIL the linker can not resolve those references.

gap_vid_enc_ffmpeg_LDADD =  $(GIMP_LIBS) $(LIBGAPVIDUTIL) $(LIBGIMPGAP) $(GAPVIDEOAPI)