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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __BAIL_TOPLEVEL_H__
#define __BAIL_TOPLEVEL_H__

#include <batk/batk.h>

B_BEGIN_DECLS

#define BAIL_TYPE_TOPLEVEL               (bail_toplevel_get_type ())
#define BAIL_TOPLEVEL(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAIL_TYPE_TOPLEVEL, BailToplevel))
#define BAIL_TOPLEVEL_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), BAIL_TYPE_TOPLEVEL, BailToplevelClass))
#define BAIL_IS_TOPLEVEL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAIL_TYPE_TOPLEVEL))
#define BAIL_IS_TOPLEVEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), BAIL_TYPE_TOPLEVEL))
#define BAIL_TOPLEVEL_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), BAIL_TYPE_TOPLEVEL, BailToplevelClass))

typedef struct _BailToplevel             BailToplevel;
typedef struct _BailToplevelClass        BailToplevelClass;
  
struct _BailToplevel
{
  BatkObject parent;
  GList *window_list;
};

GType bail_toplevel_get_type (void);

struct _BailToplevelClass
{
  BatkObjectClass parent_class;
};

B_END_DECLS

#endif /* __BAIL_TOPLEVEL_H__ */
