/* gap_gve_jpeg.h by Gernot Ziegler (gz@lysator.liu.se)
 *
 * (GAP ... GIMP Animation Plugins, now also known as GIMP Video)
 *
 */


/* revision history:
 * version 1.2.2; 2004.05.14   hof: rename from gap_encode_main.c -> gap_gve_jpeg.c
 *                              removed codeparts that does not deal with jpeg
 *                              ported to gimp-1.2 API
 * version 0.0.1; 1999.11.13   gz: 1st release
 */

#ifndef GAP_GVE_JPEG_H
#define GAP_GVE_JPEG_H


/* ------------------------------------
 *  gap_gve_jpeg_drawable_encode_jpeg
 * ------------------------------------
 *  in: drawable: Describes the picture to be compressed in GIMP terms.
 *      jpeg_interlaced: TRUE: Generate two JPEGs (one for odd/even lines each) into one buffer.
 *      jpeg_quality: The quality of the generated JPEG (0-100, where 100 is best).
 *      odd_even (only valid for jpeg_interlaced = TRUE): TRUE: Code the odd lines first.
 *                                                        FALSE: Code the even lines first.
 *      use_YUV411: Use the (standard) YUV411 coding for saving (can't be played back by hardware).
 *      app0_buffer: if != NULL, the content of the APP0-marker to write.
 *      app0_length: the length of the APP0-marker.
 *  out:JPEG_size: The size of the buffer that is returned.
 *  returns: guchar *: A buffer, allocated by this routines, which contains
 *                     the compressed JPEG, NULL on error.
 */

guchar *gap_gve_jpeg_drawable_encode_jpeg(GimpDrawable *drawable, gint32 jpeg_interlaced, gint32 *JPEG_size,
			       gint32 jpeg_quality, gint32 odd_even, gint32 use_YUV411,
			       void *app0_buffer, gint32 app0_length);



#endif
