/* bdkapplaunchcontext.h - Btk+ implementation for GAppLaunchContext
 *
 * Copyright (C) 2007 Red Hat, Inc.
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
 *
 * Author: Alexander Larsson <alexl@redhat.com>
 */

#ifndef __BDK_APP_LAUNCH_CONTEXT_H__
#define __BDK_APP_LAUNCH_CONTEXT_H__

#if !defined (__BDK_H_INSIDE__) && !defined (BDK_COMPILATION)
#error "Only <bdk/bdk.h> can be included directly."
#endif

#include <bunnyio/bunnyio.h>
#include <bdk/bdkscreen.h>

B_BEGIN_DECLS

#define BDK_TYPE_APP_LAUNCH_CONTEXT         (bdk_app_launch_context_get_type ())
#define BDK_APP_LAUNCH_CONTEXT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BDK_TYPE_APP_LAUNCH_CONTEXT, BdkAppLaunchContext))
#define BDK_APP_LAUNCH_CONTEXT_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), BDK_TYPE_APP_LAUNCH_CONTEXT, BdkAppLaunchContextClass))
#define BDK_IS_APP_LAUNCH_CONTEXT(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BDK_TYPE_APP_LAUNCH_CONTEXT))
#define BDK_IS_APP_LAUNCH_CONTEXT_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BDK_TYPE_APP_LAUNCH_CONTEXT))
#define BDK_APP_LAUNCH_CONTEXT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BDK_TYPE_APP_LAUNCH_CONTEXT, BdkAppLaunchContextClass))

typedef struct BdkAppLaunchContext	      BdkAppLaunchContext;
typedef struct BdkAppLaunchContextClass       BdkAppLaunchContextClass;
typedef struct BdkAppLaunchContextPrivate     BdkAppLaunchContextPrivate;

struct BdkAppLaunchContext
{
  GAppLaunchContext parent_instance;

  BdkAppLaunchContextPrivate *priv;
};

struct BdkAppLaunchContextClass
{
  GAppLaunchContextClass parent_class;
};

GType                bdk_app_launch_context_get_type      (void);

BdkAppLaunchContext *bdk_app_launch_context_new           (void);
void                 bdk_app_launch_context_set_display   (BdkAppLaunchContext *context,
							   BdkDisplay          *display);
void                 bdk_app_launch_context_set_screen    (BdkAppLaunchContext *context,
							   BdkScreen           *screen);
void                 bdk_app_launch_context_set_desktop   (BdkAppLaunchContext *context,
							   gint                 desktop);
void                 bdk_app_launch_context_set_timestamp (BdkAppLaunchContext *context,
							   guint32              timestamp);
void                 bdk_app_launch_context_set_icon      (BdkAppLaunchContext *context,
							   GIcon               *icon);
void                 bdk_app_launch_context_set_icon_name (BdkAppLaunchContext *context,
							   const char          *icon_name);

B_END_DECLS

#endif /* __BDK_APP_LAUNCH_CONTEXT_H__ */
