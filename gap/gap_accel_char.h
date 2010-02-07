/*  gap_accel_char.h
 *
 *  This module handles GAP acceleration characteristics calculation.
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* revision history:
 * version 2.7.0; 2010/02/06  hof: created
 */

#ifndef _GAP_ACCEL_CHAR_H
#define _GAP_ACCEL_CHAR_H

#include <gtk/gtk.h>
#include <libgimp/gimp.h>


/* ---------------------------------------
 * gap_accelMixFactor
 * ---------------------------------------
 * this proecdure implements hardcoded acceleration characteristics.
 *     
 * accelCharacteristic: 0 and 1 for linear, positive values for acceleration, negative values for deceleration
 *
 * orig_factor: a positive gdouble in the range 0.0 to 1.0
 * returns modified mix_factor in the range 0.0 to 1.0 according to specified accelCharacteristic
 */
gdouble 
gap_accelMixFactor(gdouble orig_factor, gint accelCharacteristic);


#endif
