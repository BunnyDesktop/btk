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

#ifndef __BAIL_MENU_ITEM_H__
#define __BAIL_MENU_ITEM_H__

#include <bail/bailitem.h>

B_BEGIN_DECLS

#define BAIL_TYPE_MENU_ITEM                     (bail_menu_item_get_type ())
#define BAIL_MENU_ITEM(obj)                     (B_TYPE_CHECK_INSTANCE_CAST ((obj), BAIL_TYPE_MENU_ITEM, BailMenuItem))
#define BAIL_MENU_ITEM_CLASS(klass)             (B_TYPE_CHECK_CLASS_CAST ((klass), BAIL_TYPE_MENU_ITEM, BailMenuItemClass))
#define BAIL_IS_MENU_ITEM(obj)                  (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BAIL_TYPE_MENU_ITEM))
#define BAIL_IS_MENU_ITEM_CLASS(klass)          (B_TYPE_CHECK_CLASS_TYPE ((klass), BAIL_TYPE_MENU_ITEM))
#define BAIL_MENU_ITEM_GET_CLASS(obj)           (B_TYPE_INSTANCE_GET_CLASS ((obj), BAIL_TYPE_MENU_ITEM, BailMenuItemClass))

typedef struct _BailMenuItem                   BailMenuItem;
typedef struct _BailMenuItemClass              BailMenuItemClass;

struct _BailMenuItem
{
  BailItem parent;

  gchar    *click_keybinding;
  gchar    *click_description;
  guint    action_idle_handler;
};

GType bail_menu_item_get_type (void);

struct _BailMenuItemClass
{
  BailItemClass parent_class;
};

BatkObject* bail_menu_item_new (BtkWidget *widget);

B_END_DECLS

#endif /* __BAIL_MENU_ITEM_H__ */
