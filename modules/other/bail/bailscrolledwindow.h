/* BAIL - The BUNNY Accessibility Implementation Library
 * Copyright 2001 Sun Microsystems Inc.
 *
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

#ifndef __BAIL_SCROLLED_WINDOW_H__
#define __BAIL_SCROLLED_WINDOW_H__

#include <bail/bailcontainer.h>

G_BEGIN_DECLS

#define BAIL_TYPE_SCROLLED_WINDOW            (bail_scrolled_window_get_type ())
#define BAIL_SCROLLED_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAIL_TYPE_SCROLLED_WINDOW, BailScrolledWindow))
#define BAIL_SCROLLED_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BAIL_TYPE_SCROLLED_WINDOW, BailScrolledWindowClass))
#define BAIL_IS_SCROLLED_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAIL_TYPE_SCROLLED_WINDOW))
#define BAIL_IS_SCROLLED_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BAIL_TYPE_SCROLLED_WINDOW))
#define BAIL_SCROLLED_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BAIL_TYPE_SCROLLED_WINDOW, BailScrolledWindowClass))

typedef struct _BailScrolledWindow              BailScrolledWindow;
typedef struct _BailScrolledWindowClass         BailScrolledWindowClass;

struct _BailScrolledWindow
{
  BailContainer parent;
};

GType bail_scrolled_window_get_type (void);

struct _BailScrolledWindowClass
{
  BailContainerClass parent_class;
};

G_END_DECLS

#endif /* __BAIL_SCROLLED_WINDOW_H__ */
