## Process this file with automake to produce Makefile.in

libexecdir = $(GIMP_PLUGIN_DIR)/plug-ins

scriptdatadir = $(GIMP_DATA_DIR)/scripts


if GAP_VIDEOAPI_SUPPORT
GAPVIDEOAPI = $(top_builddir)/libgapvidapi/libgapvidapi.a  $(GAPVIDEOAPI_EXTLIBS)
INC_GAPVIDEOAPI = -I$(top_builddir)/libgapvidapi $(GAPVIDEOAPI_EXTINCS)
endif

LIBGIMPGAP = $(top_builddir)/gap/libgimpgap.a
INC_LIBGIMPGAP = -I$(top_builddir)/gap

LIBGAPVIDUTIL  = $(top_builddir)/libgapvidutil/libgapvidutil.a
INC_LIBGAPVIDUTIL = -I$(top_builddir)/libgapvidutil

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
	$(INC_LIBGIMPGAP)	\
	$(INC_LIBGAPVIDUTIL)	\
	$(GIMP_CFLAGS)	\
	$(GAP_VINCS_XVIDCORE)	\
	-I$(includedir)


LDADD = $(GIMP_LIBS)


# note: sequence of libs matters because LIBGAPVIDUTIL uses both LIBGIMPGAP and GAPVIDEOAPI
#       (if those libs appear before LIBGAPVIDUTIL the linker can not resolve those references.

gap_vid_enc_avi_LDADD =  $(GIMP_LIBS) $(LIBGAPVIDUTIL) $(LIBGIMPGAP) $(GAPVIDEOAPI) $(GAP_VLIBS_XVIDCORE) -ljpeg



EXTRA_DIST = README.avilib
