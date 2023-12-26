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

#ifndef __BAIL_OPTION_MENU_H__
#define __BAIL_OPTION_MENU_H__

#include <bail/bailbutton.h>

G_BEGIN_DECLS

#define BAIL_TYPE_OPTION_MENU                (bail_option_menu_get_type ())
#define BAIL_OPTION_MENU(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAIL_TYPE_OPTION_MENU, BailOptionMenu))
#define BAIL_OPTION_MENU_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), BAIL_TYPE_OPTION_MENU, BailOptionMenuClass))
#define BAIL_IS_OPTION_MENU(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAIL_TYPE_OPTION_MENU))
#define BAIL_IS_OPTION_MENU_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass), BAIL_TYPE_OPTION_MENU))
#define BAIL_OPTION_MENU_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), BAIL_TYPE_OPTION_MENU, BailOptionMenuClass))

typedef struct _BailOptionMenu                   BailOptionMenu;
typedef struct _BailOptionMenuClass              BailOptionMenuClass;

struct _BailOptionMenu
{
  BailButton parent;
};

GType bail_option_menu_get_type (void);

struct _BailOptionMenuClass
{
  BailButtonClass parent_class;
};

G_END_DECLS

#endif /* __BAIL_OPTION_MENU_H__ */
