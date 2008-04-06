## Process this file with automake to produce Makefile.in

STOCK_IMAGES = \
	gap-add-point.png			\
	gap-anim-preview.png			\
	gap-delete-all-points.png		\
	gap-delete-point.png			\
	gap-first-point.png			\
	gap-grab-points.png			\
	gap-insert-point.png			\
	gap-last-point.png			\
	gap-next-point.png			\
	gap-pause.png				\
	gap-play.png				\
	gap-play-reverse.png			\
	gap-prev-point.png			\
	gap-reset-all-points.png		\
	gap-reset-point.png			\
	gap-rotate-follow.png			\
	gap-source-image.png			\
	gap-stepmode.png			\
	gap-update.png				\
	gap-story-icon-movie.png		\
	gap-story-icon-frames.png		\
	gap-story-icon-animimage.png		\
	gap-story-icon-image.png		\
	gap-story-icon-mask-movie.png		\
	gap-story-icon-mask-frames.png		\
	gap-story-icon-mask-animimage.png	\
	gap-story-icon-mask-image.png		\
	gap-story-icon-mask-undefined.png	\
	gap-story-icon-color.png		\
	gap-story-icon-transition-attr.png	\
	gap-story-icon-blacksection.png		\
	gap-story-icon-section.png		\
	gap-story-icon-section-main.png		\
	gap-story-icon-section-mask.png		\
	gap-story-icon-default.png		\
	gap-range-end.png			\
	gap-range-start.png			\
	gap-set-range-end.png			\
	gap-set-range-start.png			\
	gap-speed.png


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
