/* BAIL - The GNOME Accessibility Implementation Library
 * Copyright 2002 Sun Microsystems Inc.
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

#ifndef __BAIL_RADIO_SUB_MENU_ITEM_H__
#define __BAIL_RADIO_SUB_MENU_ITEM_H__

#include <bail/bailchecksubmenuitem.h>

G_BEGIN_DECLS

#define BAIL_TYPE_RADIO_SUB_MENU_ITEM               (bail_radio_sub_menu_item_get_type ())
#define BAIL_RADIO_SUB_MENU_ITEM(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAIL_TYPE_RADIO_SUB_MENU_ITEM, BailRadioSubMenuItem))
#define BAIL_RADIO_SUB_MENU_ITEM_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), BAIL_TYPE_RADIO_SUB_MENU_ITEM, BailRadioSubMenuItemClass))
#define BAIL_IS_RADIO_SUB_MENU_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAIL_TYPE_RADIO_SUB_MENU_ITEM))
#define BAIL_IS_RADIO_SUB_MENU_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), BAIL_TYPE_RADIO_SUB_MENU_ITEM))
#define BAIL_RADIO_SUB_MENU_ITEM_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), BAIL_TYPE_RADIO_SUB_MENU_ITEM, BailRadioSubMenuItemClass))

typedef struct _BailRadioSubMenuItem              BailRadioSubMenuItem;
typedef struct _BailRadioSubMenuItemClass         BailRadioSubMenuItemClass;

struct _BailRadioSubMenuItem
{
  BailCheckSubMenuItem parent;

  GSList *old_group;
};

GType bail_radio_sub_menu_item_get_type (void);

struct _BailRadioSubMenuItemClass
{
  BailCheckSubMenuItemClass parent_class;
};

BatkObject* bail_radio_sub_menu_item_new (BtkWidget *widget);

G_END_DECLS

#endif /* __BAIL_RADIO_SUB_MENU_ITEM_H__ */
