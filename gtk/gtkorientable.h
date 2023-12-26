/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * btkorientable.h
 * Copyright (C) 2008 Imendio AB
 * Contact: Michael Natterer <mitch@imendio.com>
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

#ifndef __BTK_ORIENTABLE_H__
#define __BTK_ORIENTABLE_H__

#if !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkwidget.h>

G_BEGIN_DECLS

#define BTK_TYPE_ORIENTABLE             (btk_orientable_get_type ())
#define BTK_ORIENTABLE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_ORIENTABLE, BtkOrientable))
#define BTK_ORIENTABLE_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), BTK_TYPE_ORIENTABLE, BtkOrientableIface))
#define BTK_IS_ORIENTABLE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_ORIENTABLE))
#define BTK_IS_ORIENTABLE_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), BTK_TYPE_ORIENTABLE))
#define BTK_ORIENTABLE_GET_IFACE(inst)  (G_TYPE_INSTANCE_GET_INTERFACE ((inst), BTK_TYPE_ORIENTABLE, BtkOrientableIface))


typedef struct _BtkOrientable       BtkOrientable;         /* Dummy typedef */
typedef struct _BtkOrientableIface  BtkOrientableIface;

struct _BtkOrientableIface
{
  GTypeInterface base_iface;
};


GType          btk_orientable_get_type        (void) G_GNUC_CONST;

void           btk_orientable_set_orientation (BtkOrientable  *orientable,
                                               BtkOrientation  orientation);
BtkOrientation btk_orientable_get_orientation (BtkOrientable  *orientable);

G_END_DECLS

#endif /* __BTK_ORIENTABLE_H__ */
