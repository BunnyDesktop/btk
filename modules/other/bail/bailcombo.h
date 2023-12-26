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

#ifndef __BAIL_COMBO_H__
#define __BAIL_COMBO_H__

#include <bail/bailcontainer.h>

B_BEGIN_DECLS

#define BAIL_TYPE_COMBO                      (bail_combo_get_type ())
#define BAIL_COMBO(obj)                      (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAIL_TYPE_COMBO, BailCombo))
#define BAIL_COMBO_CLASS(klass)              (G_TYPE_CHECK_CLASS_CAST ((klass), BAIL_TYPE_COMBO, BailComboClass))
#define BAIL_IS_COMBO(obj)                   (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAIL_TYPE_COMBO))
#define BAIL_IS_COMBO_CLASS(klass)           (G_TYPE_CHECK_CLASS_TYPE ((klass), BAIL_TYPE_COMBO))
#define BAIL_COMBO_GET_CLASS(obj)            (G_TYPE_INSTANCE_GET_CLASS ((obj), BAIL_TYPE_COMBO, BailComboClass))

typedef struct _BailCombo              BailCombo;
typedef struct _BailComboClass         BailComboClass;

struct _BailCombo
{
  BailContainer parent;

  gpointer      old_selection;
  gchar         *press_description;

  guint         action_idle_handler;
  guint         select_idle_handler;
  guint         deselect_idle_handler;
};

GType bail_combo_get_type (void);

struct _BailComboClass
{
  BailContainerClass parent_class;
};

B_END_DECLS

#endif /* __BAIL_COMBO_H__ */
