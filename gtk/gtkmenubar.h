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

#ifndef __BTK_MENU_BAR_H__
#define __BTK_MENU_BAR_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkmenushell.h>


G_BEGIN_DECLS


#define	BTK_TYPE_MENU_BAR               (btk_menu_bar_get_type ())
#define BTK_MENU_BAR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_MENU_BAR, BtkMenuBar))
#define BTK_MENU_BAR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_MENU_BAR, BtkMenuBarClass))
#define BTK_IS_MENU_BAR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_MENU_BAR))
#define BTK_IS_MENU_BAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_MENU_BAR))
#define BTK_MENU_BAR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_MENU_BAR, BtkMenuBarClass))

typedef struct _BtkMenuBar       BtkMenuBar;
typedef struct _BtkMenuBarClass  BtkMenuBarClass;

struct _BtkMenuBar
{
  BtkMenuShell menu_shell;
};

struct _BtkMenuBarClass
{
  BtkMenuShellClass parent_class;

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};


GType      btk_menu_bar_get_type        (void) G_GNUC_CONST;
BtkWidget* btk_menu_bar_new             (void);

BtkPackDirection btk_menu_bar_get_pack_direction (BtkMenuBar       *menubar);
void             btk_menu_bar_set_pack_direction (BtkMenuBar       *menubar,
						  BtkPackDirection  pack_dir);
BtkPackDirection btk_menu_bar_get_child_pack_direction (BtkMenuBar       *menubar);
void             btk_menu_bar_set_child_pack_direction (BtkMenuBar       *menubar,
							BtkPackDirection  child_pack_dir);

#ifndef BTK_DISABLE_DEPRECATED
#define btk_menu_bar_append(menu,child)	    btk_menu_shell_append  ((BtkMenuShell *)(menu),(child))
#define btk_menu_bar_prepend(menu,child)    btk_menu_shell_prepend ((BtkMenuShell *)(menu),(child))
#define btk_menu_bar_insert(menu,child,pos) btk_menu_shell_insert ((BtkMenuShell *)(menu),(child),(pos))
#endif /* BTK_DISABLE_DEPRECATED */

/* Private functions */
void _btk_menu_bar_cycle_focus (BtkMenuBar       *menubar,
				BtkDirectionType  dir);


G_END_DECLS


#endif /* __BTK_MENU_BAR_H__ */
