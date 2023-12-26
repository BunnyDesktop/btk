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
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/.
 */

#ifndef __BDK_SELECTION_H__
#define __BDK_SELECTION_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BDK_H_INSIDE__) && !defined (BDK_COMPILATION)
#error "Only <bdk/bdk.h> can be included directly."
#endif

#include <bdk/bdktypes.h>

B_BEGIN_DECLS

/* Predefined atoms relating to selections. In general, one will need to use
 * bdk_intern_atom
 */
#define BDK_SELECTION_PRIMARY 		_BDK_MAKE_ATOM (1)
#define BDK_SELECTION_SECONDARY 	_BDK_MAKE_ATOM (2)
#define BDK_SELECTION_CLIPBOARD 	_BDK_MAKE_ATOM (69)
#define BDK_TARGET_BITMAP 		_BDK_MAKE_ATOM (5)
#define BDK_TARGET_COLORMAP 		_BDK_MAKE_ATOM (7)
#define BDK_TARGET_DRAWABLE 		_BDK_MAKE_ATOM (17)
#define BDK_TARGET_PIXMAP 		_BDK_MAKE_ATOM (20)
#define BDK_TARGET_STRING 		_BDK_MAKE_ATOM (31)
#define BDK_SELECTION_TYPE_ATOM 	_BDK_MAKE_ATOM (4)
#define BDK_SELECTION_TYPE_BITMAP 	_BDK_MAKE_ATOM (5)
#define BDK_SELECTION_TYPE_COLORMAP 	_BDK_MAKE_ATOM (7)
#define BDK_SELECTION_TYPE_DRAWABLE 	_BDK_MAKE_ATOM (17)
#define BDK_SELECTION_TYPE_INTEGER 	_BDK_MAKE_ATOM (19)
#define BDK_SELECTION_TYPE_PIXMAP 	_BDK_MAKE_ATOM (20)
#define BDK_SELECTION_TYPE_WINDOW 	_BDK_MAKE_ATOM (33)
#define BDK_SELECTION_TYPE_STRING 	_BDK_MAKE_ATOM (31)

#ifndef BDK_DISABLE_DEPRECATED

typedef BdkAtom BdkSelection;
typedef BdkAtom BdkTarget;
typedef BdkAtom BdkSelectionType;

#endif /* BDK_DISABLE_DEPRECATED */

/* Selections
 */

#ifndef BDK_MULTIHEAD_SAFE
bboolean   bdk_selection_owner_set (BdkWindow	 *owner,
				    BdkAtom	  selection,
				    buint32	  time_,
				    bboolean      send_event);
BdkWindow* bdk_selection_owner_get (BdkAtom	  selection);
#endif/* BDK_MULTIHEAD_SAFE */

bboolean   bdk_selection_owner_set_for_display (BdkDisplay *display,
						BdkWindow  *owner,
						BdkAtom     selection,
						buint32     time_,
						bboolean    send_event);
BdkWindow *bdk_selection_owner_get_for_display (BdkDisplay *display,
						BdkAtom     selection);

void	   bdk_selection_convert   (BdkWindow	 *requestor,
				    BdkAtom	  selection,
				    BdkAtom	  target,
				    buint32	  time_);
bint       bdk_selection_property_get (BdkWindow  *requestor,
				       buchar	 **data,
				       BdkAtom	  *prop_type,
				       bint	  *prop_format);

#ifndef BDK_MULTIHEAD_SAFE
void	   bdk_selection_send_notify (BdkNativeWindow requestor,
				      BdkAtom	      selection,
				      BdkAtom	      target,
				      BdkAtom	      property,
				      buint32	      time_);
#endif /* BDK_MULTIHEAD_SAFE */

void       bdk_selection_send_notify_for_display (BdkDisplay      *display,
						  BdkNativeWindow  requestor,
						  BdkAtom     	   selection,
						  BdkAtom     	   target,
						  BdkAtom     	   property,
						  buint32     	   time_);

B_END_DECLS

#endif /* __BDK_SELECTION_H__ */
