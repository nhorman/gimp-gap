/* gap_onion_worker.h
 * 2003.05.22 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * This Module contains ONION Skin Layers worker Procedures
 *
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
 * 1.3.14a; 2003/05/22   hof: integration into gimp-gap-1.3.14
 * 1.3.12a; 2003/05/03   hof: started port to gimp-1.3  /gtk+2.2
 * version 1.2.2a;  2001.12.10   hof: created
 */

#ifndef _GAP_ONION_WORKER_H
#define _GAP_ONION_WORKER_H

#include <gap_onion_main.h>

/* onion_worker procedures */
gint    p_set_data_onion_cfg(t_global_params *gpp, char *key);
gint    p_get_data_onion_cfg(t_global_params *gpp);
gint    p_onion_range(t_global_params *gpp);
gint    p_onion_apply(t_global_params *gpp, gboolean use_cache);
gint    p_onion_delete(t_global_params *gpp);
gint    p_onion_visibility(t_global_params *gpp, gint visi_mode);

void    p_plug_in_gap_get_animinfo(gint32 image_ID, t_ainfo *ainfo);

#endif

