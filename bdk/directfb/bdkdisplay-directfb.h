/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef BDK_DISPLAY_DFB_H
#define BDK_DISPLAY_DFB_H

#include <directfb.h>
#include <bdk/bdkdisplay.h>
#include <bdk/bdkkeys.h>

B_BEGIN_DECLS

typedef struct _BdkDisplayDFB BdkDisplayDFB;
typedef struct _BdkDisplayDFBClass BdkDisplayDFBClass;


#define BDK_TYPE_DISPLAY_DFB              (bdk_display_dfb_get_type())
#define BDK_DISPLAY_DFB(object)           (B_TYPE_CHECK_INSTANCE_CAST ((object), BDK_TYPE_DISPLAY_DFB, BdkDisplayDFB))
#define BDK_DISPLAY_DFB_CLASS(klass)      (B_TYPE_CHECK_CLASS_CAST ((klass), BDK_TYPE_DISPLAY_DFB, BdkDisplayDFBClass))
#define BDK_IS_DISPLAY_DFB(object)        (B_TYPE_CHECK_INSTANCE_TYPE ((object), BDK_TYPE_DISPLAY_DFB))
#define BDK_IS_DISPLAY_DFB_CLASS(klass)   (B_TYPE_CHECK_CLASS_TYPE ((klass), BDK_TYPE_DISPLAY_DFB))
#define BDK_DISPLAY_DFB_GET_CLASS(obj)    (B_TYPE_INSTANCE_GET_CLASS ((obj), BDK_TYPE_DISPLAY_DFB, BdkDisplayDFBClass))

struct _BdkDisplayDFB
{
  BdkDisplay             parent;
  IDirectFB             *directfb;
  IDirectFBDisplayLayer *layer;
  IDirectFBEventBuffer  *buffer;
  IDirectFBInputDevice  *keyboard;
  BdkKeymap             *keymap;
};

struct _BdkDisplayDFBClass
{
  BdkDisplayClass parent;
};

GType      bdk_display_dfb_get_type            (void);

IDirectFBSurface *bdk_display_dfb_create_surface (BdkDisplayDFB *display,
                                                  int format,
                                                  int width, int height);

B_END_DECLS

#endif /* BDK_DISPLAY_DFB_H */
