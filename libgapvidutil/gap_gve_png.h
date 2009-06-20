/* gap_gve_png.h by Wolfgang Hofer (hof@gimp.org)
 *
 * (GAP ... GIMP Animation Plugins, now also known as GIMP Video)
 *
 */


/* revision history (see svn)
 * 2008.06.22   hof: created
 */

#ifndef GAP_GVE_PNG_H
#define GAP_GVE_PNG_H


/* ------------------------------------
 *  gap_gve_png_drawable_encode_png
 * ------------------------------------
 * in: drawable: Describes the picture to be compressed in GIMP terms.
       png_interlaced: TRUE: Generate two PNGs (one for odd/even lines each) into one buffer.
       png_compression: The compression of the generated PNG (0-9, where 9 is best 0 fastest).
       app0_buffer: if != NULL, the content of the APP0-marker to write.
       app0_length: the length of the APP0-marker.
   out:PNG_size: The size of the buffer that is returned.
   returns: guchar *: A buffer, allocated by this routines, which contains
                      the compressed PNG, NULL on error.
 */

guchar *gap_gve_png_drawable_encode_png(GimpDrawable *drawable, gint32 png_interlaced, gint32 *PNG_size,
                               gint32 png_compression,
                               void *app0_buffer, gint32 app0_length);



#endif
