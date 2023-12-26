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

#ifndef __BAIL_ADJUSTMENT_H__
#define __BAIL_ADJUSTMENT_H__

#include <batk/batk.h>

G_BEGIN_DECLS

#define BAIL_TYPE_ADJUSTMENT                     (bail_adjustment_get_type ())
#define BAIL_ADJUSTMENT(obj)                     (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAIL_TYPE_ADJUSTMENT, BailAdjustment))
#define BAIL_ADJUSTMENT_CLASS(klass)             (G_TYPE_CHECK_CLASS_CAST ((klass), BAIL_TYPE_ADJUSTMENT, BailAdjustmentClass))
#define BAIL_IS_ADJUSTMENT(obj)                  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAIL_TYPE_ADJUSTMENT))
#define BAIL_IS_ADJUSTMENT_CLASS(klass)          (G_TYPE_CHECK_CLASS_TYPE ((klass), BAIL_TYPE_ADJUSTMENT))
#define BAIL_ADJUSTMENT_GET_CLASS(obj)           (G_TYPE_INSTANCE_GET_CLASS ((obj), BAIL_TYPE_ADJUSTMENT, BailAdjustmentClass))

typedef struct _BailAdjustment                  BailAdjustment;
typedef struct _BailAdjustmentClass		BailAdjustmentClass;

struct _BailAdjustment
{
  BatkObject parent;

  BtkAdjustment *adjustment;
};

GType bail_adjustment_get_type (void);

struct _BailAdjustmentClass
{
  BatkObjectClass parent_class;
};

BatkObject *bail_adjustment_new (BtkAdjustment *adjustment);

G_END_DECLS

#endif /* __BAIL_ADJUSTMENT_H__ */
