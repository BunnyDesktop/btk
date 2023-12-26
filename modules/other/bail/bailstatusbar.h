/* BAIL - The BUNNY Accessibility Implementation Library
 * Copyright 2001, 2002, 2003 Sun Microsystems Inc.
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

#ifndef __BAIL_STATUSBAR_H__
#define __BAIL_STATUSBAR_H__

#include <bail/bailcontainer.h>
#include <libbail-util/bailtextutil.h>

B_BEGIN_DECLS

#define BAIL_TYPE_STATUSBAR                  (bail_statusbar_get_type ())
#define BAIL_STATUSBAR(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAIL_TYPE_STATUSBAR, BailStatusbar))
#define BAIL_STATUSBAR_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), BAIL_TYPE_STATUSBAR, BailStatusbarClass))
#define BAIL_IS_STATUSBAR(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAIL_TYPE_STATUSBAR))
#define BAIL_IS_STATUSBAR_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), BAIL_TYPE_STATUSBAR))
#define BAIL_STATUSBAR_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), BAIL_TYPE_STATUSBAR, BailStatusbarClass))

typedef struct _BailStatusbar              BailStatusbar;
typedef struct _BailStatusbarClass         BailStatusbarClass;

struct _BailStatusbar
{
  BailContainer parent;

  BailTextUtil *textutil;
};

GType bail_statusbar_get_type (void);

struct _BailStatusbarClass
{
  BailContainerClass parent_class;
};

B_END_DECLS

#endif /* __BAIL_STATUSBAR_H__ */
