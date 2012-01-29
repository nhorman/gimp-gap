/*  gap_detail_align_exec.c
 *    This filter locates the position of a small 
 *    outside the selction into the selected area.
 *    It was implemented for fixing small pixel defects of my video camera sensor
 *    and is intended to be used as filter when processing video frames
 *    that are shot by such faulty cameras and typically runs
 *    as filtermacro. Therefore the selection can be provided via an external image
 *    or as path vectors via an extrernal SVG file.
 *
 *  2011/12/01
 */
/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* Revision history
 *  (2011/12/01)  2.7.0       hof: created
 */
extern int gap_debug;

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <glib/gstdio.h>

#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gap_base.h"
#include "gap_libgapbase.h"
#include "gap_detail_align_exec.h"

#include "gap-intl.h"


typedef struct PixelCoords
{
  gboolean  valid;
  gint32  px;
  gint32  py;
} PixelCoords;


typedef struct AlingCoords
{
  PixelCoords  currCoords;   /* 1st coords in current frame */
  PixelCoords  currCoords2;  /* 2nd detail coords in current frame */
  PixelCoords  startCoords;  /* 1st coords of first processed (reference) frame */
  PixelCoords  startCoords2; /* 2nd detail coords of first processed frame */
} AlingCoords;


typedef struct ParseContext {
  char  *parsePtr;
  gint32 frameNr;
  AlingCoords *alingCoords;
} ParseContext;



#define DEFAULT_framePhase 1


/* --------------------------------------
 * p_parse_value_gint32
 * --------------------------------------
 */
static gboolean
p_parse_value_gint32(ParseContext *parseCtx, gint32 *valDestPtr, gint *itemCount)
{
  gint64 value64;
  gchar *endptr;
  
  value64 = g_ascii_strtoll(parseCtx->parsePtr, &endptr, 10);
  if(parseCtx->parsePtr == endptr)
  {
    /* pointer was not advanced (no int number could be scanned */
    return (FALSE);
  }
  *valDestPtr = value64;
  parseCtx->parsePtr = endptr;
  *itemCount +=1;
  return (TRUE);

}  /* end p_parse_value_gint32 */


/* --------------------------------
 * p_parse_coords_p1_and_p2
 * --------------------------------
 * parse p1x, p1y, p2x, p2y values into p1 (mandatory) and p2 (ptional) coordinates
 * and parse keyframe_abs value int *frameNrPtr (optional if present)
 * multiple occurances are not tolerated.
 * return TRUE on success.
 */
static gboolean
p_parse_coords_p1_and_p2(ParseContext *parseCtx, PixelCoords *p1, PixelCoords *p2, gint32 *frameNrPtr)
{
  gboolean ok;
  gint     px1Count;
  gint     py1Count;
  gint     px2Count;
  gint     py2Count;
  gint     frCount;
  
  ok = TRUE;
  px1Count = 0;
  py1Count = 0;
  px2Count = 0;
  py2Count = 0;
  frCount  = 0;
  while(*parseCtx->parsePtr != '\0')
  {
    if (strncmp(parseCtx->parsePtr, "keyframe_abs=\"", strlen("keyframe_abs=\"")) == 0)
    {
      parseCtx->parsePtr += strlen("keyframe_abs=\"");
      ok = p_parse_value_gint32(parseCtx, frameNrPtr, &frCount);
    }
    else if (strncmp(parseCtx->parsePtr, "p1x=\"", strlen("p1x=\"")) == 0)
    {
      parseCtx->parsePtr += strlen("p1x=\"");
      ok = p_parse_value_gint32(parseCtx, &p1->px, &px1Count);
    }
    else if (strncmp(parseCtx->parsePtr, "p1y=\"", strlen("p1y=\"")) == 0)
    {
      parseCtx->parsePtr += strlen("p1y=\"");
      ok = p_parse_value_gint32(parseCtx, &p1->py, &py1Count);
    }
    else if (strncmp(parseCtx->parsePtr, "p2x=\"", strlen("p2x=\"")) == 0)
    {
      parseCtx->parsePtr += strlen("p2x=\"");
      ok = p_parse_value_gint32(parseCtx, &p2->px, &px2Count);
    }
    else if (strncmp(parseCtx->parsePtr, "p2y=\"", strlen("p2y=\"")) == 0)
    {
      parseCtx->parsePtr += strlen("p2y=\"");
      ok = p_parse_value_gint32(parseCtx, &p2->py, &py2Count);
    }
    else if (strncmp(parseCtx->parsePtr, "/>", strlen("/>")) == 0)
    {
      /* stop evaluate when current controlpoint ends */
      parseCtx->parsePtr += strlen("/>");
      break;
    }
    else if (strncmp(parseCtx->parsePtr, "<controlpoint ", strlen("<controlpoint ")) == 0)
    {
      /* stop evaluate when when we run into the next controlpoint */
      break;
    }
    else
    {
      parseCtx->parsePtr++;
    }
    
    if(ok != TRUE)
    {
     break;
    }
  }

  if ((ok == TRUE)
  && (px1Count == 1)
  && (py1Count == 1)
  && (frCount <= 1))
  {
    p1->valid = TRUE;
    if ((px2Count == 1)
    &&  (py2Count == 1))
    {
      p2->valid = TRUE;
    }

    return (TRUE);
  }


  
  if(gap_debug)
  {
    printf("p_parse_coords_p1_and_p2 ok:%d px1Count:%d py1Count:%d px2Count:%d py2Count:%d frCount:%d\n"
           " parsePtr:%.200s\n" 
      ,(int)ok
      ,(int)px1Count
      ,(int)py1Count
      ,(int)px2Count
      ,(int)py2Count
      ,(int)frCount
      ,parseCtx->parsePtr 
      );
  }
  return (FALSE);
  
}  /* end p_parse_coords_p1_and_p2 */


static void
p_find_attribute_start(ParseContext *parseCtx)
{
  while(*parseCtx->parsePtr != '\0')
  {
    if(*parseCtx->parsePtr == '<')
    {
      break;
    }
    parseCtx->parsePtr++;
  }
  
  if(gap_debug)
  {
    printf("\nSTRING at parsePtr:%.200s\n"
      , parseCtx->parsePtr
      );
  }
}   /* end p_find_attribute_start */


/* --------------------------------
 * p_parse_xml_controlpoint_coords
 * --------------------------------
 *
 */
static gboolean
p_parse_xml_controlpoint_coords(ParseContext *parseCtx)
{
  gboolean     ok;
  gint32       frameNr;

  ok = TRUE;
  frameNr = -1;
  
  while(*parseCtx->parsePtr != '\0')
  {
    p_find_attribute_start(parseCtx);

    if (strncmp(parseCtx->parsePtr, "<controlpoint ", strlen("<controlpoint ")) == 0)
    {
      parseCtx->parsePtr += strlen("<controlpoint ");
      
      if(parseCtx->alingCoords->startCoords.valid == FALSE)
      {
        ok = p_parse_coords_p1_and_p2(parseCtx
                               , &parseCtx->alingCoords->startCoords
                               , &parseCtx->alingCoords->startCoords2
                               , &frameNr
                               );
      }
      else
      {
        ok = p_parse_coords_p1_and_p2(parseCtx
                               , &parseCtx->alingCoords->currCoords
                               , &parseCtx->alingCoords->currCoords2
                               , &frameNr
                               );
      }

      if(gap_debug)
      {
        printf("p_parse_xml_controlpoint_coords: ok:%d, frameNr:%d parseCtx->frameNr:%d\n"
          ,(int)ok
          ,(int)frameNr
          ,(int)parseCtx->frameNr
          );
      }

      if(ok != TRUE)
      {
        return (FALSE);
      }

      if ((frameNr == parseCtx->frameNr)
      &&  (parseCtx->alingCoords->currCoords2.valid == TRUE))
      {
        return(TRUE);
      }
    }
    parseCtx->parsePtr++;
  }


  if ((ok == TRUE)
  &&  (parseCtx->alingCoords->currCoords2.valid == TRUE))
  {
    /* accept the last controlpoint when no matching frameNr was found */
    return(TRUE);
  }
  
  return(FALSE);

}  /* end p_parse_xml_controlpoint_coords */


/* -----------------------------------------
 * p_parse_xml_controlpoint_coords_from_file
 * -----------------------------------------
 * parse coords of 1st controlpoint and the controlpoint at frameNr
 * from the xml file.
 * return TRUE on success.
 */
static gboolean
p_parse_xml_controlpoint_coords_from_file(const char *filename, gint32 frameNr, AlingCoords *alingCoords)
{
  char *textBuffer;
  gsize lengthTextBuffer;
  
  ParseContext parseCtx;
  gboolean     parseOk;
  
  textBuffer = NULL;
  parseOk = FALSE;
  if ((g_file_get_contents (filename, &textBuffer, &lengthTextBuffer, NULL) != TRUE) 
  || (textBuffer == NULL))
  {
    printf("Couldn't load XML file:%s\n", filename);
    return(FALSE);
  }

  parseCtx.parsePtr = textBuffer;
  parseCtx.alingCoords = alingCoords;
  parseCtx.frameNr = frameNr;

  parseCtx.alingCoords->startCoords.valid  = FALSE;
  parseCtx.alingCoords->startCoords2.valid = FALSE;
  parseCtx.alingCoords->currCoords.valid   = FALSE;
  parseCtx.alingCoords->currCoords2.valid  = FALSE;
  
  parseOk = p_parse_xml_controlpoint_coords(&parseCtx);
    
    
  g_free(textBuffer);
  
  return (parseOk);

}  /* end p_parse_xml_controlpoint_coords_from_file */


/* -----------------------------------
 * p_set_drawable_offsets
 * -----------------------------------
 * simple 2-point align via offsets (without rotate and scale)
 */
static gint32
p_set_drawable_offsets(gint32 activeDrawableId, AlingCoords *alingCoords)
{
  gdouble px1, py1, px2, py2;
  gdouble dx, dy;
  gint    offset_x;
  gint    offset_y;
  

  px1 = alingCoords->startCoords.px;
  py1 = alingCoords->startCoords.py;
  px2 = alingCoords->currCoords.px;
  py2 = alingCoords->currCoords.py;

  dx = px2 - px1;
  dy = py2 - py1;

  /* findout the offsets of the original layer within the source Image */
  gimp_drawable_offsets(activeDrawableId, &offset_x, &offset_y );
  gimp_layer_set_offsets(activeDrawableId, offset_x - dx, offset_y - dy);
  
  return (activeDrawableId);

}  /* end p_set_drawable_offsets */


/* -----------------------------------
 * p_exact_align_drawable
 * -----------------------------------
 * 4-point alignment including necessary scale and rotate transformation
 * to match 2 pairs of corresponding coordonates.
 */
static gint32
p_exact_align_drawable(gint32 activeDrawableId, AlingCoords *alingCoords)
{
  gdouble px1, py1, px2, py2;
  gdouble px3, py3, px4, py4;
  gdouble dx1, dy1, dx2, dy2;
  gdouble angle1Rad, angle2Rad, angleRad;
  gdouble len1, len2;
  gdouble scaleXY;
  gint32  transformedDrawableId;
  
  px1 = alingCoords->startCoords.px;
  py1 = alingCoords->startCoords.py;
  px2 = alingCoords->startCoords2.px;
  py2 = alingCoords->startCoords2.py;
  
  px3 = alingCoords->currCoords.px;
  py3 = alingCoords->currCoords.py;
  px4 = alingCoords->currCoords2.px;
  py4 = alingCoords->currCoords2.py;

  dx1 = px2 - px1;
  dy1 = py2 - py1;
  dx2 = px4 - px3;
  dy2 = py4 - py3;
  
  /* the angle between the two lines. i.e., the angle layer2 must be clockwise rotatet
   * in order to overlap with initial start layer1
   */
  angle1Rad = 0;
  angle2Rad = 0;
  if (dx1 != 0.0)
  {
    angle1Rad = atan(dy1 / dx1);
  }
  if (dx2 != 0.0)
  {
    angle2Rad = atan(dy2 / dx2);
  }
  angleRad = angle1Rad - angle2Rad;
  
  /* the scale factors current layer must be mulitplied by,
   * in order to fit onto reference start layer.
   * this is simply the ratio of the two line lenths from the path we created with the 4 points
   */
  
  len1 = sqrt((dx1 * dx1) + (dy1 * dy1));
  len2 = sqrt((dx2 * dx2) + (dy2 * dy2));

  scaleXY = 1.0;
  if ((len1 != len2)
  &&  (len2 != 0.0))
  {
    scaleXY = len1 / len2;
  }

  transformedDrawableId = gimp_drawable_transform_2d(activeDrawableId
                            , px3
                            , py3
                            , scaleXY 
                            , scaleXY
                            , angleRad
                            , px1
                            , py1
                            , 0      /* FORWARD (0), TRANSFORM-BACKWARD (1) */
                            , 2      /* INTERPOLATION-CUBIC (2) */
                            , TRUE   /* supersample */
                            , 1      /* Maximum recursion level used for supersampling */
                            , 1      /* TRANSFORM-RESIZE-CLIP (1) */
                            );

  if(gap_debug)
  {
    printf("p_exact_align_drawable: activeDrawableId:%d transformedDrawableId:%d\n"
           "    p1: %f %f\n"
           "    p2: %f %f\n"
           "    p3: %f %f\n"
           "    p4: %f %f\n"
           "    scale:%f  angleRad:%f (degree:%f)\n"
      ,(int)activeDrawableId
      ,(int)transformedDrawableId
      ,(float)px1
      ,(float)py1
      ,(float)px2
      ,(float)py2
      ,(float)px3
      ,(float)py3
      ,(float)px4
      ,(float)py4
      ,(float)scaleXY
      ,(float)angleRad
      ,(float)(angleRad * 180.0) / G_PI
      );
  }
  return (transformedDrawableId);  
  
}  /* end p_exact_align_drawable */

/* -----------------------------------
 * gap_detail_xml_align
 * -----------------------------------
 * This procedure transforms the specified drawable
 * according controlpoints in an XML file that was recorded via detail tracking.
 * it picks the relevant controlpoint at framePhase from the XML file
 * and scales, rotates and aligns the specified drawableId (shall be a layer)
 * in a way that it exactly matches with the 1st (reference) controlpoint in the XML file.
 *
 * returns the drawable id of the resulting transformed layer (or -1 on errors)s
 */
gint32
gap_detail_xml_align(gint32 drawableId, XmlAlignValues *xaVals)
{
  gint32 newDrawableId;
  
  newDrawableId = drawableId;
  if(gap_debug)
  {
      printf("gap_detail_xml_align: START\n"
             "  framePhase:%d  moveLogFile:%s\n"
            , (int)xaVals->framePhase
             , xaVals->moveLogFile
            );
  }
  
  if(xaVals->framePhase > 1)
  {
    gboolean  parseOk;
    AlingCoords alingCoords;
    
    parseOk =
      p_parse_xml_controlpoint_coords_from_file(xaVals->moveLogFile
         , xaVals->framePhase, &alingCoords);

    if(parseOk)
    {
      if ((alingCoords.startCoords2.valid == TRUE)
      &&  (alingCoords.currCoords2.valid == TRUE))
      {
        /* exact align transformation with 2 point pairs including rotation and scaling */
        newDrawableId = p_exact_align_drawable(drawableId, &alingCoords);
      }
      else
      {
        /* simple move (to match current recorded point to recorded start point) */
        newDrawableId = p_set_drawable_offsets(drawableId, &alingCoords);
      }
    }
    else
    {
      newDrawableId = -1;
    }

  }
 
  return(newDrawableId);
  
}  /* end gap_detail_xml_align */




/* -----------------------------------
 * gap_detail_xml_align_get_values
 * -----------------------------------
 * This procedure is typically called
 * on the snapshot image created by the Player.
 * This image has one layer at the first snapshot
 * and each further snapshot adds one layer on top of the layerstack.
 *
 * The start is detected when the image has only one layer.
 * optionally the numer of layers can be limted
 * to 2 (or more) layers.
 */
void
gap_detail_xml_align_get_values(XmlAlignValues *xaVals)
{
  int l_len;

  /* init default values */
  xaVals->framePhase                 = DEFAULT_framePhase;
  xaVals->moveLogFile[0]             = '\0';

  l_len = gimp_get_data_size (GAP_DETAIL_TRACKING_XML_ALIGNER_PLUG_IN_NAME);
  if (l_len == sizeof(XmlAlignValues))
  {
    /* Possibly retrieve data from a previous interactive run */
    gimp_get_data (GAP_DETAIL_TRACKING_XML_ALIGNER_PLUG_IN_NAME, xaVals);
    
    if(gap_debug)
    {
      printf("gap_detail_xml_align_get_values FOUND data for key:%s\n"
        , GAP_DETAIL_TRACKING_XML_ALIGNER_PLUG_IN_NAME
        );
    }
  }

  if(gap_debug)
  {
    printf("gap_detail_xml_align_get_values:\n"
           "  framePhase:%d  moveLogFile:%s\n"
            , (int)xaVals->framePhase
            , xaVals->moveLogFile
            );
  }
  
}  /* end gap_detail_xml_align_get_values */



/* ---------------------------
 * gap_detail_xml_align_dialog
 * ---------------------------
 *   return  TRUE.. OK
 *           FALSE.. in case of Error or cancel
 */
gboolean
gap_detail_xml_align_dialog(XmlAlignValues *xaVals)
{
#define SPINBUTTON_ENTRY_WIDTH 70
#define DETAIL_ALIGN_XML_DIALOG_ARGC 3

  static GapArrArg  argv[DETAIL_ALIGN_XML_DIALOG_ARGC];
  gint ii;
  gint ii_framePhase;

  ii=0; gap_arr_arg_init(&argv[ii], GAP_ARR_WGT_INT_PAIR); ii_framePhase = ii;
  argv[ii].label_txt = _("Frame Phase:");
  argv[ii].help_txt  = _("Frame number (phase) to be rendered.");
  argv[ii].constraint = FALSE;
  argv[ii].int_min   = 1;
  argv[ii].int_max   = 9999;
  argv[ii].umin      = 1;
  argv[ii].umax      = 999999;
  argv[ii].int_ret   = xaVals->framePhase;
  argv[ii].entry_width = SPINBUTTON_ENTRY_WIDTH;
  argv[ii].has_default = TRUE;
  argv[ii].int_default = DEFAULT_framePhase;


  ii++; gap_arr_arg_init(&argv[ii], GAP_ARR_WGT_FILESEL);
  argv[ii].label_txt = _("XML file:");
  argv[ii].help_txt  = _("Name of the xml file that contains the tracked detail coordinates. "
                        " (recorded with the detail tracking feature).");
  argv[ii].text_buf_len = sizeof(xaVals->moveLogFile);
  argv[ii].text_buf_ret = &xaVals->moveLogFile[0];
  argv[ii].entry_width = 400;



  ii++; gap_arr_arg_init(&argv[ii], GAP_ARR_WGT_DEFAULT_BUTTON);
  argv[ii].label_txt = _("Default");
  argv[ii].help_txt  = _("Reset all parameters to default values");

  if(TRUE == gap_arr_ok_cancel_dialog(_("Detail Align via XML"),
                            _("Settings :"),
                            DETAIL_ALIGN_XML_DIALOG_ARGC, argv))
  {
      xaVals->framePhase               = (gint32)(argv[ii_framePhase].int_ret);
      
      gimp_set_data (GAP_DETAIL_TRACKING_XML_ALIGNER_PLUG_IN_NAME
                    , xaVals, sizeof (XmlAlignValues));
      return TRUE;
  }
  else
  {
      return FALSE;
  }
}               /* end gap_detail_xml_align_dialog */



/* ---------------------------------- */
/* ---------------------------------- */
/* ---------------------------------- */
/* ---------------------------------- */
/* ---------------------------------- */
/* ---------------------------------- */



/* ------------------------------------------
 * p_capture_4_vector_points
 * ------------------------------------------
 * capture the first 4 points of the 1st stroke in the active path vectors
 */
static gint
p_capture_4_vector_points(gint32 imageId, AlingCoords *alingCoords)
{
  gint32  activeVectorsId;
  PixelCoords *coordPtr[4];
  gint         ii;
  gint         countVaildPoints;
  
  
  coordPtr[0] = &alingCoords->startCoords;
  coordPtr[1] = &alingCoords->startCoords2;
  coordPtr[2] = &alingCoords->currCoords;
  coordPtr[3] = &alingCoords->currCoords2;
  
  countVaildPoints = 0;
  for(ii=0; ii < 4; ii++)
  {
    coordPtr[ii]->px = -1;
    coordPtr[ii]->py = -1;
    coordPtr[ii]->valid = FALSE;

  }

  activeVectorsId = gimp_image_get_active_vectors(imageId);
  if(activeVectorsId >= 0)
  {
    gint num_strokes;
    gint *strokes;
 
    strokes = gimp_vectors_get_strokes (activeVectorsId, &num_strokes);
    if(strokes)
    {
      if(num_strokes > 0)
      {
        gdouble  *points;
        gint      num_points;
        gboolean  closed;
        GimpVectorsStrokeType type;
       
        points = NULL;
        type = gimp_vectors_stroke_get_points(activeVectorsId, strokes[0],
                                   &num_points, &points, &closed);

        if(gap_debug)
        {
          gint ii;
          for(ii=0; ii < MIN(24, num_points); ii++)
          {
            printf ("point[%d] = %.3f\n", ii, points[ii]);
          }
        }
        
        if (type == GIMP_VECTORS_STROKE_TYPE_BEZIER)
        {
          if(num_points >= 6)
          {
            coordPtr[0]->px = points[0];
            coordPtr[0]->py = points[1];
            coordPtr[0]->valid = TRUE;
            countVaildPoints++;
          }
          if(num_points >= 12)
          {
            coordPtr[1]->px = points[6];
            coordPtr[1]->py = points[7];
            coordPtr[1]->valid = TRUE;
            countVaildPoints++;
          }
          if(num_points >= 18)
          {
            coordPtr[2]->px = points[12];
            coordPtr[2]->py = points[13];
            coordPtr[2]->valid = TRUE;
            countVaildPoints++;
          }
          if(num_points >= 24)
          {
            coordPtr[3]->px = points[18];
            coordPtr[3]->py = points[19];
            coordPtr[3]->valid = TRUE;
            countVaildPoints++;
          }
        }
        if(points)
        {
          g_free(points);
        }
      
      }
      g_free(strokes);
    }

  }
 
  return(countVaildPoints);
  
}  /* end p_capture_4_vector_points */



/* ------------------------------------------
 * gap_detail_exact_align_via_4point_path
 * ------------------------------------------
 *
 */
gint32
gap_detail_exact_align_via_4point_path(gint32 image_id, gint32 activeDrawableId)
{
  AlingCoords  alingCoordinates;
  AlingCoords *alingCoords;
  gint         countVaildPoints;
  gint32       ret;
  
  alingCoords = &alingCoordinates;
  ret = -1;
  
  countVaildPoints = p_capture_4_vector_points(image_id, alingCoords);
  if(countVaildPoints == 4)
  {
    ret = p_exact_align_drawable(activeDrawableId, alingCoords);

  }
  else if(countVaildPoints == 2)
  {
    alingCoords->currCoords.px = alingCoords->startCoords2.px;
    alingCoords->currCoords.py = alingCoords->startCoords2.py;
    alingCoords->currCoords.valid = alingCoords->startCoords2.valid;
    ret = p_set_drawable_offsets(activeDrawableId, alingCoords);
  }
  else
  {
    g_message(_("This filter requires a current path with 4 points,"
                "where point 1 and 2 mark reference positions "
                "and point 3 and 4 mark postions in the target layer."
                "It transforms the target layer in a way that "
                "point3 is moved to point1 and point4 moves to point2."
                "(this may include rotate an scale transforamtion).\n"
                "A path with 2 points can be used to move point2 to point1."
                "(via simple move operation without rotate and scale)"));
  }
  
  return(ret);
  
  
}  /* end  gap_detail_exact_align_via_4point_path */

