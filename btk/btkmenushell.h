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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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

#ifndef __BTK_MENU_SHELL_H__
#define __BTK_MENU_SHELL_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkcontainer.h>


B_BEGIN_DECLS

#define	BTK_TYPE_MENU_SHELL		(btk_menu_shell_get_type ())
#define BTK_MENU_SHELL(obj)		(B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_MENU_SHELL, BtkMenuShell))
#define BTK_MENU_SHELL_CLASS(klass)	(B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_MENU_SHELL, BtkMenuShellClass))
#define BTK_IS_MENU_SHELL(obj)		(B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_MENU_SHELL))
#define BTK_IS_MENU_SHELL_CLASS(klass)	(B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_MENU_SHELL))
#define BTK_MENU_SHELL_GET_CLASS(obj)   (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_MENU_SHELL, BtkMenuShellClass))


typedef struct _BtkMenuShell	   BtkMenuShell;
typedef struct _BtkMenuShellClass  BtkMenuShellClass;

struct _BtkMenuShell
{
  BtkContainer container;

  GList *GSEAL (children);
  BtkWidget *GSEAL (active_menu_item);
  BtkWidget *GSEAL (parent_menu_shell);

  buint GSEAL (button);
  buint32 GSEAL (activate_time);

  buint GSEAL (active) : 1;
  buint GSEAL (have_grab) : 1;
  buint GSEAL (have_xgrab) : 1;
  buint GSEAL (ignore_leave) : 1; /* unused */
  buint GSEAL (menu_flag) : 1;    /* unused */
  buint GSEAL (ignore_enter) : 1;
  buint GSEAL (keyboard_mode) : 1;
};

struct _BtkMenuShellClass
{
  BtkContainerClass parent_class;
  
  buint submenu_placement : 1;
  
  void (*deactivate)     (BtkMenuShell *menu_shell);
  void (*selection_done) (BtkMenuShell *menu_shell);

  void (*move_current)     (BtkMenuShell        *menu_shell,
			    BtkMenuDirectionType direction);
  void (*activate_current) (BtkMenuShell *menu_shell,
			    bboolean      force_hide);
  void (*cancel)           (BtkMenuShell *menu_shell);
  void (*select_item)      (BtkMenuShell *menu_shell,
			    BtkWidget    *menu_item);
  void (*insert)           (BtkMenuShell *menu_shell,
			    BtkWidget    *child,
			    bint          position);
  bint (*get_popup_delay)  (BtkMenuShell *menu_shell);
  bboolean (*move_selected) (BtkMenuShell *menu_shell,
			     bint          distance);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
};


GType btk_menu_shell_get_type          (void) B_GNUC_CONST;
void  btk_menu_shell_append            (BtkMenuShell *menu_shell,
					BtkWidget    *child);
void  btk_menu_shell_prepend           (BtkMenuShell *menu_shell,
					BtkWidget    *child);
void  btk_menu_shell_insert            (BtkMenuShell *menu_shell,
					BtkWidget    *child,
					bint          position);
void  btk_menu_shell_deactivate        (BtkMenuShell *menu_shell);
void  btk_menu_shell_select_item       (BtkMenuShell *menu_shell,
					BtkWidget    *menu_item);
void  btk_menu_shell_deselect          (BtkMenuShell *menu_shell);
void  btk_menu_shell_activate_item     (BtkMenuShell *menu_shell,
					BtkWidget    *menu_item,
					bboolean      force_deactivate);
void  btk_menu_shell_select_first      (BtkMenuShell *menu_shell,
					bboolean      search_sensitive);
void _btk_menu_shell_select_last       (BtkMenuShell *menu_shell,
					bboolean      search_sensitive);
bint  _btk_menu_shell_get_popup_delay  (BtkMenuShell *menu_shell);
void  btk_menu_shell_cancel            (BtkMenuShell *menu_shell);

void  _btk_menu_shell_add_mnemonic     (BtkMenuShell *menu_shell,
                                        buint         keyval,
                                        BtkWidget    *target);
void  _btk_menu_shell_remove_mnemonic  (BtkMenuShell *menu_shell,
                                        buint         keyval,
                                        BtkWidget    *target);

bboolean btk_menu_shell_get_take_focus (BtkMenuShell *menu_shell);
void     btk_menu_shell_set_take_focus (BtkMenuShell *menu_shell,
                                        bboolean      take_focus);

void     _btk_menu_shell_update_mnemonics  (BtkMenuShell *menu_shell);
void     _btk_menu_shell_set_keyboard_mode (BtkMenuShell *menu_shell,
                                            bboolean      keyboard_mode);
bboolean _btk_menu_shell_get_keyboard_mode (BtkMenuShell *menu_shell);

B_END_DECLS

#endif /* __BTK_MENU_SHELL_H__ */
