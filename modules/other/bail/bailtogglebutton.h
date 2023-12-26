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

#ifndef __BAIL_TOGGLE_BUTTON_H__
#define __BAIL_TOGGLE_BUTTON_H__

#include <bail/bailbutton.h>

B_BEGIN_DECLS

#define BAIL_TYPE_TOGGLE_BUTTON              (bail_toggle_button_get_type ())
#define BAIL_TOGGLE_BUTTON(obj)              (B_TYPE_CHECK_INSTANCE_CAST ((obj), BAIL_TYPE_TOGGLE_BUTTON, BailToggleButton))
#define BAIL_TOGGLE_BUTTON_CLASS(klass)      (B_TYPE_CHECK_CLASS_CAST ((klass), BAIL_TYPE_TOGGLE_BUTTON, BailToggleButtonClass))
#define BAIL_IS_TOGGLE_BUTTON(obj)           (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BAIL_TYPE_TOGGLE_BUTTON))
#define BAIL_IS_TOGGLE_BUTTON_CLASS(klass)   (B_TYPE_CHECK_CLASS_TYPE ((klass), BAIL_TYPE_TOGGLE_BUTTON))
#define BAIL_TOGGLE_BUTTON_GET_CLASS(obj)    (B_TYPE_INSTANCE_GET_CLASS ((obj), BAIL_TYPE_TOGGLE_BUTTON, BailToggleButtonClass))

typedef struct _BailToggleButton              BailToggleButton;
typedef struct _BailToggleButtonClass         BailToggleButtonClass;

struct _BailToggleButton
{
  BailButton parent;
};

GType bail_toggle_button_get_type (void);

struct _BailToggleButtonClass
{
  BailButtonClass parent_class;
};

B_END_DECLS

#endif /* __BAIL_TOGGLE_BUTTON_H__ */
