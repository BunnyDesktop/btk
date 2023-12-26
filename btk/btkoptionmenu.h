/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#ifndef BTK_DISABLE_DEPRECATED

#ifndef __BTK_OPTION_MENU_H__
#define __BTK_OPTION_MENU_H__

#include <btk/btk.h>


B_BEGIN_DECLS

#define BTK_TYPE_OPTION_MENU              (btk_option_menu_get_type ())
#define BTK_OPTION_MENU(obj)              (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_OPTION_MENU, BtkOptionMenu))
#define BTK_OPTION_MENU_CLASS(klass)      (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_OPTION_MENU, BtkOptionMenuClass))
#define BTK_IS_OPTION_MENU(obj)           (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_OPTION_MENU))
#define BTK_IS_OPTION_MENU_CLASS(klass)   (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_OPTION_MENU))
#define BTK_OPTION_MENU_GET_CLASS(obj)    (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_OPTION_MENU, BtkOptionMenuClass))


typedef struct _BtkOptionMenu       BtkOptionMenu;
typedef struct _BtkOptionMenuClass  BtkOptionMenuClass;

struct _BtkOptionMenu
{
  BtkButton button;
  
  BtkWidget *menu;
  BtkWidget *menu_item;
  
  buint16 width;
  buint16 height;
};

struct _BtkOptionMenuClass
{
  BtkButtonClass parent_class;

  void (*changed) (BtkOptionMenu *option_menu);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};


GType      btk_option_menu_get_type    (void) B_GNUC_CONST;
BtkWidget* btk_option_menu_new         (void);
BtkWidget* btk_option_menu_get_menu    (BtkOptionMenu *option_menu);
void       btk_option_menu_set_menu    (BtkOptionMenu *option_menu,
					BtkWidget     *menu);
void       btk_option_menu_remove_menu (BtkOptionMenu *option_menu);
bint       btk_option_menu_get_history (BtkOptionMenu *option_menu);
void       btk_option_menu_set_history (BtkOptionMenu *option_menu,
					buint          index_);


B_END_DECLS

#endif /* __BTK_OPTION_MENU_H__ */

#endif /* BTK_DISABLE_DEPRECATED */
