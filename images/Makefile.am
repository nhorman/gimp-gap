## Process this file with automake to produce Makefile.in

STOCK_IMAGES = \
	gap-add-point.png		\
	gap-anim-preview.png		\
	gap-delete-all-points.png	\
	gap-delete-point.png		\
	gap-first-point.png		\
	gap-insert-point.png		\
	gap-last-point.png		\
	gap-next-point.png		\
	gap-pause.png			\
	gap-play.png			\
	gap-play-reverse.png		\
	gap-prev-point.png		\
	gap-reset-all-points.png	\
	gap-reset-point.png		\
	gap-rotate-follow.png		\
	gap-source-image.png		\
	gap-stepmode.png		\
	gap-update.png

EXTRA_DIST = $(STOCK_IMAGES)

noinst_DATA = gap-stock-pixbufs.h
CLEANFILES = $(noinst_DATA) stock-icons.list

stock-icons.list: $(STOCK_IMAGES) Makefile.am
	( rm -f $@; \
          for image in $(STOCK_IMAGES); do \
            echo $$image | \
              sed -e 's|.*/||' -e 's|-|_|g' -e 's|\.png$$||' >> $@; \
	    echo "  $(srcdir)/$$image" >> $@; \
	  done )

$(srcdir)/gap-stock-pixbufs.h: stock-icons.list
	gdk-pixbuf-csource --raw --build-list `cat stock-icons.list` > $(@F)
