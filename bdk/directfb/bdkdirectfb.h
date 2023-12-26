/* BDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.
 */

/*
 * BTK+ DirectFB backend
 * Copyright (C) 2001-2002  convergence integrated media GmbH
 * Copyright (C) 2002       convergence GmbH
 * Written by Denis Oliver Kropp <dok@convergence.de> and
 *            Sven Neumann <sven@convergence.de>
 */

#ifndef __BDK_DIRECTFB_H__
#define __BDK_DIRECTFB_H__

/* This define disables some experimental code
 */
#define BDK_DIRECTFB_NO_EXPERIMENTS

#include <bairo.h>
#include <directfb.h>
#include "bdk/bdkprivate.h"


extern BdkWindow * _bdk_parent_root;

G_BEGIN_DECLS

#define BDK_ROOT_WINDOW()      _bdk_parent_root

#define BDK_WINDOW_DFB_ID(win) (BDK_WINDOW_IMPL_DIRECTFB (BDK_WINDOW_OBJECT (win)->impl)->dfb_id)


/* used for the --transparent-unfocused hack */
extern gboolean            bdk_directfb_apply_focus_opacity;

/* used for the --enable-color-keying hack */
extern gboolean            bdk_directfb_enable_color_keying;
extern DFBColor            bdk_directfb_bg_color;
extern DFBColor            bdk_directfb_bg_color_key;

/* to disable antialiasing */
extern gboolean            bdk_directfb_monochrome_fonts;


/* BTK+-DirectFB specific functions */

void        bdk_directfb_window_set_opacity (BdkWindow *window,
                                             guchar     opacity);

#ifndef BDK_DISABLE_DEPRECATED
BdkWindow * bdk_directfb_window_new         (BdkWindow             *parent,
                                             BdkWindowAttr         *attributes,
                                             gint                   attributes_mask,
                                             DFBWindowCapabilities  window_caps,
                                             DFBWindowOptions       window_options,
                                             DFBSurfaceCapabilities surface_caps);
#endif /* BDK_DISABLE_DEPRECATED */

BdkVisual * bdk_directfb_visual_by_format   (DFBSurfacePixelFormat  pixel_format);

IDirectFBWindow  *bdk_directfb_window_lookup (BdkWindow *window);
IDirectFBSurface *bdk_directfb_surface_lookup (BdkWindow *window);

BdkWindow *bdk_directfb_create_child_window (BdkWindow        *parent,
                                             IDirectFBSurface *subsurface);


G_END_DECLS

#endif /* __BDK_DIRECTFB_H__ */
