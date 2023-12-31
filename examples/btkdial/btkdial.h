/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

#ifndef __BTK_DIAL_H__
#define __BTK_DIAL_H__


#include <bdk/bdk.h>
#include <btk/btkadjustment.h>
#include <btk/btkwidget.h>


B_BEGIN_DECLS


#define BTK_DIAL(obj)          B_TYPE_CHECK_INSTANCE_CAST (obj, btk_dial_get_type (), BtkDial)
#define BTK_DIAL_CLASS(klass)  B_TYPE_CHECK_CLASS_CAST (klass, btk_dial_get_type (), BtkDialClass)
#define BTK_IS_DIAL(obj)       B_TYPE_CHECK_INSTANCE_TYPE (obj, btk_dial_get_type ())


typedef struct _BtkDial        BtkDial;
typedef struct _BtkDialClass   BtkDialClass;

struct _BtkDial
{
  BtkWidget widget;

  /* update policy (BTK_UPDATE_[CONTINUOUS/DELAYED/DISCONTINUOUS]) */
  buint policy : 2;

  /* Button currently pressed or 0 if none */
  buint8 button;

  /* Dimensions of dial components */
  bint radius;
  bint pointer_width;

  /* ID of update timer, or 0 if none */
  buint32 timer;

  /* Current angle */
  bfloat angle;
  bfloat last_angle;

  /* Old values from adjustment stored so we know when something changes */
  bfloat old_value;
  bfloat old_lower;
  bfloat old_upper;

  /* The adjustment object that stores the data for this dial */
  BtkAdjustment *adjustment;
};

struct _BtkDialClass
{
  BtkWidgetClass parent_class;
};


BtkWidget*     btk_dial_new                    (BtkAdjustment *adjustment);
GType          btk_dial_get_type               (void);
BtkAdjustment* btk_dial_get_adjustment         (BtkDial      *dial);
void           btk_dial_set_update_policy      (BtkDial      *dial,
						BtkUpdateType  policy);

void           btk_dial_set_adjustment         (BtkDial      *dial,
						BtkAdjustment *adjustment);


B_END_DECLS


#endif /* __BTK_DIAL_H__ */
