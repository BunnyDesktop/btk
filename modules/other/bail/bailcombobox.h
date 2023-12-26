/* BAIL - The BUNNY Accessibility Implementation Library
 * Copyright 2004 Sun Microsystems Inc.
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

#ifndef __BAIL_COMBO_BOX_H__
#define __BAIL_COMBO_BOX_H__

#include <bail/bailcontainer.h>

B_BEGIN_DECLS

#define BAIL_TYPE_COMBO_BOX                      (bail_combo_box_get_type ())
#define BAIL_COMBO_BOX(obj)                      (B_TYPE_CHECK_INSTANCE_CAST ((obj), BAIL_TYPE_COMBO_BOX, BailComboBox))
#define BAIL_COMBO_BOX_CLASS(klass)              (B_TYPE_CHECK_CLASS_CAST ((klass), BAIL_TYPE_COMBO_BOX, BailComboBoxClass))
#define BAIL_IS_COMBO_BOX(obj)                   (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BAIL_TYPE_COMBO_BOX))
#define BAIL_IS_COMBO_BOX_CLASS(klass)           (B_TYPE_CHECK_CLASS_TYPE ((klass), BAIL_TYPE_COMBO_BOX))
#define BAIL_COMBO_BOX_GET_CLASS(obj)            (B_TYPE_INSTANCE_GET_CLASS ((obj), BAIL_TYPE_COMBO_BOX, BailComboBoxClass))

typedef struct _BailComboBox              BailComboBox;
typedef struct _BailComboBoxClass         BailComboBoxClass;

struct _BailComboBox
{
  BailContainer parent;

  bchar         *press_keybinding;
  bchar         *press_description;
  buint         action_idle_handler;

  bchar         *name;
  bint          old_selection;
  bboolean      popup_set;
};

GType bail_combo_box_get_type (void);

struct _BailComboBoxClass
{
  BailContainerClass parent_class;
};

B_END_DECLS

#endif /* __BAIL_COMBO_BOX_H__ */
