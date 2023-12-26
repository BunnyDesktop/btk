/* bdkquartz.h
 *
 * Copyright (C) 2005-2007 Imendio AB
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

#ifndef __BDK_QUARTZ_H__
#define __BDK_QUARTZ_H__

#include <AppKit/AppKit.h>
#include <bdk/bdkprivate.h>

B_BEGIN_DECLS

/* NSInteger only exists in Leopard and newer.  This check has to be
 * done after inclusion of the system headers.  If NSInteger has not
 * been defined, we know for sure that we are on 32-bit.
 */
#ifndef NSINTEGER_DEFINED
typedef int NSInteger;
typedef unsigned int NSUInteger;
#endif

#ifndef CGFLOAT_DEFINED
typedef float CGFloat;
#endif

typedef enum
{
  BDK_OSX_UNSUPPORTED = 0,
  BDK_OSX_MIN = 4,
  BDK_OSX_TIGER = 4,
  BDK_OSX_LEOPARD = 5,
  BDK_OSX_SNOW_LEOPARD = 6,
  BDK_OSX_LION = 7,
  BDK_OSX_MOUNTAIN_LION = 8,
  BDK_OSX_MAVERICKS = 9,
  BDK_OSX_YOSEMITE = 10,
  BDK_OSX_EL_CAPITAN = 11,
  BDK_OSX_SIERRA = 12,
  BDK_OSX_HIGH_SIERRA = 13,
  BDK_OSX_MOJAVE = 14,
  BDK_OSX_CURRENT = 14,
  BDK_OSX_NEW = 99
} BdkOSXVersion;

gboolean  bdk_quartz_window_is_quartz                           (BdkWindow      *window);
NSWindow *bdk_quartz_window_get_nswindow                        (BdkWindow      *window);
NSView   *bdk_quartz_window_get_nsview                          (BdkWindow      *window);
NSImage  *bdk_quartz_pixbuf_to_ns_image_libbtk_only             (BdkPixbuf      *pixbuf);
id        bdk_quartz_drag_context_get_dragging_info_libbtk_only (BdkDragContext *context);
NSEvent  *bdk_quartz_event_get_nsevent                          (BdkEvent       *event);
BdkOSXVersion bdk_quartz_osx_version                            (void);

BdkAtom   bdk_quartz_pasteboard_type_to_atom_libbtk_only        (NSString       *type);
NSString *bdk_quartz_target_to_pasteboard_type_libbtk_only      (const gchar    *target);
NSString *bdk_quartz_atom_to_pasteboard_type_libbtk_only        (BdkAtom         atom);

B_END_DECLS

#endif /* __BDK_QUARTZ_H__ */
