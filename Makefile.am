## Process this file with automake to produce Makefile.in

if GAP_AUDIO_SUPPORT
LIBWAVCLIENT=libwavplayclient
endif

if GAP_VIDEOAPI_SUPPORT
LIBGAPVIDAPI=libgapvidapi
endif

# todo: videoencoders depend on different libs
#       but all of them use the GAP_VIDEOAPI_SUPPORT
#       for storyboard based encoding
#       compilation should depend primary on those other libs
if GAP_VIDEOAPI_SUPPORT
LIBGAPVIDUTIL=libgapvidutil
VID_COMMON=vid_common
VID_ENC_FFMPEG=vid_enc_ffmpeg
VID_ENC_SINGLE=vid_enc_single
VID_ENC_AVI=vid_enc_avi
endif


SUBDIRS = images $(LIBWAVCLIENT) $(LIBGAPVIDAPI) gap po \
	$(LIBGAPVIDUTIL) 	\
	$(VID_COMMON)		\
	$(VID_ENC_AVI) 		\
	$(VID_ENC_FFMPEG) 	\
	$(VID_ENC_SINGLE)

EXTRA_DIST = \
	AUTHORS			\
	COPYING			\
	INSTALL			\
	README			\
	ChangeLog		\
	ChangeLog.pre-1-2	\
	intltool-extract.in	\
	intltool-merge.in	\
	intltool-update.in

