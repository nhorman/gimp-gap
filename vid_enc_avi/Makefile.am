## Process this file with automake to produce Makefile.in

libexecdir = $(GIMP_PLUGIN_DIR)/plug-ins

scriptdatadir = $(GIMP_DATA_DIR)/scripts


if GAP_VIDEOAPI_SUPPORT
GAPVIDEOAPI = $(top_builddir)/libgapvidapi/libgapvidapi.a  $(GAPVIDEOAPI_EXTLIBS)
INC_GAPVIDEOAPI = -I$(top_srcdir)/libgapvidapi $(GAPVIDEOAPI_EXTINCS)
endif

LIBGAPSTORY =  -L$(top_builddir)/gap  -lgapstory
INC_LIBGAPSTORY = -I$(top_srcdir)/gap

LIBGAPVIDUTIL  = $(top_builddir)/libgapvidutil/libgapvidutil.a
INC_LIBGAPVIDUTIL = -I$(top_srcdir)/libgapvidutil

libexec_PROGRAMS = gap_vid_enc_avi

gap_vid_enc_avi_SOURCES = \
	gap_enc_avi_main.c	\
	gap_enc_avi_main.h	\
	gap_enc_avi_gui.c	\
	gap_enc_avi_gui.h	\
	avilib.h \
	avilib.c


AM_CPPFLAGS = \
	-DGAPLIBDIR=\""$(GAPLIBDIR)"\"	\
	-DLOCALEDIR=\""$(LOCALEDIR)"\"

INCLUDES = \
	-I$(top_srcdir)	\
	-I$(top_srcdir)/libwavplayclient	\
	$(INC_GAPVIDEOAPI)	\
	$(INC_LIBGAPSTORY)	\
	$(INC_LIBGAPVIDUTIL)	\
	$(GIMP_CFLAGS)	\
	$(GAP_VINCS_XVIDCORE)	\
	-I$(includedir)


LDADD = $(GIMP_LIBS)


# note: sequence of libs matters because LIBGAPVIDUTIL uses both LIBGAPSTORY and GAPVIDEOAPI
#       (if those libs appear before LIBGAPVIDUTIL the linker can not resolve those references.

gap_vid_enc_avi_LDADD =  $(LIBGAPVIDUTIL) $(LIBGAPSTORY) $(GAPVIDEOAPI) $(GAP_VLIBS_XVIDCORE) -ljpeg $(GIMP_LIBS)



EXTRA_DIST = README.avilib
