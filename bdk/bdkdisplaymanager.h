/* BDK - The GIMP Drawing Kit
 * Copyright (C) 2000 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/.
 */

#ifndef __BDK_DISPLAY_MANAGER_H__
#define __BDK_DISPLAY_MANAGER_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BDK_H_INSIDE__) && !defined (BDK_COMPILATION)
#error "Only <bdk/bdk.h> can be included directly."
#endif

#include <bdk/bdktypes.h>
#include <bdk/bdkdisplay.h>

B_BEGIN_DECLS

typedef struct _BdkDisplayManager      BdkDisplayManager;
typedef struct _BdkDisplayManagerClass BdkDisplayManagerClass;

#define BDK_TYPE_DISPLAY_MANAGER              (bdk_display_manager_get_type ())
#define BDK_DISPLAY_MANAGER(object)           (B_TYPE_CHECK_INSTANCE_CAST ((object), BDK_TYPE_DISPLAY_MANAGER, BdkDisplayManager))
#define BDK_DISPLAY_MANAGER_CLASS(klass)      (B_TYPE_CHECK_CLASS_CAST ((klass), BDK_TYPE_DISPLAY_MANAGER, BdkDisplayManagerClass))
#define BDK_IS_DISPLAY_MANAGER(object)        (B_TYPE_CHECK_INSTANCE_TYPE ((object), BDK_TYPE_DISPLAY_MANAGER))
#define BDK_IS_DISPLAY_MANAGER_CLASS(klass)   (B_TYPE_CHECK_CLASS_TYPE ((klass), BDK_TYPE_DISPLAY_MANAGER))
#define BDK_DISPLAY_MANAGER_GET_CLASS(obj)    (B_TYPE_INSTANCE_GET_CLASS ((obj), BDK_TYPE_DISPLAY_MANAGER, BdkDisplayManagerClass))

struct _BdkDisplayManagerClass
{
  BObjectClass parent_class;

  void (*display_opened) (BdkDisplayManager *display_manager,
			  BdkDisplay *display);
};

GType bdk_display_manager_get_type (void) B_GNUC_CONST;

BdkDisplayManager *bdk_display_manager_get                 (void);
BdkDisplay *       bdk_display_manager_get_default_display (BdkDisplayManager *display_manager);
void               bdk_display_manager_set_default_display (BdkDisplayManager *display_manager,
							    BdkDisplay        *display);
GSList     *       bdk_display_manager_list_displays       (BdkDisplayManager *display_manager);

B_END_DECLS

#endif /* __BDK_DISPLAY_MANAGER_H__ */
