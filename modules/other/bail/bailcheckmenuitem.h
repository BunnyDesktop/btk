/* BAIL - The BUNNY Accessibility Implementation Library
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

#ifndef __BAIL_CHECK_MENU_ITEM_H__
#define __BAIL_CHECK_MENU_ITEM_H__

#include <bail/bailmenuitem.h>

B_BEGIN_DECLS

#define BAIL_TYPE_CHECK_MENU_ITEM              (bail_check_menu_item_get_type ())
#define BAIL_CHECK_MENU_ITEM(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAIL_TYPE_CHECK_MENU_ITEM, BailCheckMenuItem))
#define BAIL_CHECK_MENU_ITEM_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BAIL_TYPE_CHECK_MENU_ITEM, BailCheckMenuItemClass))
#define BAIL_IS_CHECK_MENU_ITEM(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAIL_TYPE_CHECK_MENU_ITEM))
#define BAIL_IS_CHECK_MENU_ITEM_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BAIL_TYPE_CHECK_MENU_ITEM))
#define BAIL_CHECK_MENU_ITEM_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), BAIL_TYPE_CHECK_MENU_ITEM, BailCheckMenuItemClass))

typedef struct _BailCheckMenuItem              BailCheckMenuItem;
typedef struct _BailCheckMenuItemClass         BailCheckMenuItemClass;

struct _BailCheckMenuItem
{
  BailMenuItem parent;
};

GType bail_check_menu_item_get_type (void);

struct _BailCheckMenuItemClass
{
  BailMenuItemClass parent_class;
};

BatkObject* bail_check_menu_item_new (BtkWidget *widget);

B_END_DECLS

#endif /* __BAIL_CHECK_MENU_ITEM_H__ */
