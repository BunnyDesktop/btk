/* BTK - The GIMP Toolkit
 * btkrecentchoosermenu.h - Recently used items menu widget
 * Copyright (C) 2006, Emmanuele Bassi
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

#ifndef __BTK_RECENT_CHOOSER_MENU_H__
#define __BTK_RECENT_CHOOSER_MENU_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkmenu.h>
#include <btk/btkrecentchooser.h>

B_BEGIN_DECLS

#define BTK_TYPE_RECENT_CHOOSER_MENU		(btk_recent_chooser_menu_get_type ())
#define BTK_RECENT_CHOOSER_MENU(obj)		(B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_RECENT_CHOOSER_MENU, BtkRecentChooserMenu))
#define BTK_IS_RECENT_CHOOSER_MENU(obj)		(B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_RECENT_CHOOSER_MENU))
#define BTK_RECENT_CHOOSER_MENU_CLASS(klass)	(B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_RECENT_CHOOSER_MENU, BtkRecentChooserMenuClass))
#define BTK_IS_RECENT_CHOOSER_MENU_CLASS(klass)	(B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_RECENT_CHOOSER_MENU))
#define BTK_RECENT_CHOOSER_MENU_GET_CLASS(obj)	(B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_RECENT_CHOOSER_MENU, BtkRecentChooserMenuClass))

typedef struct _BtkRecentChooserMenu		BtkRecentChooserMenu;
typedef struct _BtkRecentChooserMenuClass	BtkRecentChooserMenuClass;
typedef struct _BtkRecentChooserMenuPrivate	BtkRecentChooserMenuPrivate;

struct _BtkRecentChooserMenu
{
  /*< private >*/
  BtkMenu parent_instance;

  BtkRecentChooserMenuPrivate *GSEAL (priv);
};

struct _BtkRecentChooserMenuClass
{
  BtkMenuClass parent_class;

  /* padding for future expansion */
  void (* btk_recent1) (void);
  void (* btk_recent2) (void);
  void (* btk_recent3) (void);
  void (* btk_recent4) (void);
};

GType      btk_recent_chooser_menu_get_type         (void) B_GNUC_CONST;

BtkWidget *btk_recent_chooser_menu_new              (void);
BtkWidget *btk_recent_chooser_menu_new_for_manager  (BtkRecentManager     *manager);

gboolean   btk_recent_chooser_menu_get_show_numbers (BtkRecentChooserMenu *menu);
void       btk_recent_chooser_menu_set_show_numbers (BtkRecentChooserMenu *menu,
						     gboolean              show_numbers);

B_END_DECLS

#endif /* ! __BTK_RECENT_CHOOSER_MENU_H__ */
