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

#ifndef __BTK_MENU_ITEM_H__
#define __BTK_MENU_ITEM_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkitem.h>


B_BEGIN_DECLS

#define	BTK_TYPE_MENU_ITEM		(btk_menu_item_get_type ())
#define BTK_MENU_ITEM(obj)		(B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_MENU_ITEM, BtkMenuItem))
#define BTK_MENU_ITEM_CLASS(klass)	(B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_MENU_ITEM, BtkMenuItemClass))
#define BTK_IS_MENU_ITEM(obj)		(B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_MENU_ITEM))
#define BTK_IS_MENU_ITEM_CLASS(klass)	(B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_MENU_ITEM))
#define BTK_MENU_ITEM_GET_CLASS(obj)    (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_MENU_ITEM, BtkMenuItemClass))


typedef struct _BtkMenuItem	  BtkMenuItem;
typedef struct _BtkMenuItemClass  BtkMenuItemClass;

struct _BtkMenuItem
{
  BtkItem item;

  BtkWidget *GSEAL (submenu);
  BdkWindow *GSEAL (event_window);

  buint16 GSEAL (toggle_size);
  buint16 GSEAL (accelerator_width);
  bchar  *GSEAL (accel_path);

  buint GSEAL (show_submenu_indicator) : 1;
  buint GSEAL (submenu_placement) : 1;
  buint GSEAL (submenu_direction) : 1;
  buint GSEAL (right_justify): 1;
  buint GSEAL (timer_from_keypress) : 1;
  buint GSEAL (from_menubar) : 1;
  buint GSEAL (timer);
};

struct _BtkMenuItemClass
{
  BtkItemClass parent_class;
  
  /* If the following flag is true, then we should always hide
   * the menu when the MenuItem is activated. Otherwise, the 
   * it is up to the caller. For instance, when navigating
   * a menu with the keyboard, <Space> doesn't hide, but
   * <Return> does.
   */
  buint hide_on_activate : 1;
  
  void (* activate)             (BtkMenuItem *menu_item);
  void (* activate_item)        (BtkMenuItem *menu_item);
  void (* toggle_size_request)  (BtkMenuItem *menu_item,
				 bint        *requisition);
  void (* toggle_size_allocate) (BtkMenuItem *menu_item,
				 bint         allocation);
  void (* set_label)            (BtkMenuItem *menu_item,
				 const bchar *label);
  const bchar *(* get_label) (BtkMenuItem *menu_item);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
};


GType	   btk_menu_item_get_type	      (void) B_GNUC_CONST;
BtkWidget* btk_menu_item_new                  (void);
BtkWidget* btk_menu_item_new_with_label       (const bchar         *label);
BtkWidget* btk_menu_item_new_with_mnemonic    (const bchar         *label);
void       btk_menu_item_set_submenu          (BtkMenuItem         *menu_item,
					       BtkWidget           *submenu);
BtkWidget* btk_menu_item_get_submenu          (BtkMenuItem         *menu_item);
void       btk_menu_item_select               (BtkMenuItem         *menu_item);
void       btk_menu_item_deselect             (BtkMenuItem         *menu_item);
void       btk_menu_item_activate             (BtkMenuItem         *menu_item);
void       btk_menu_item_toggle_size_request  (BtkMenuItem         *menu_item,
					       bint                *requisition);
void       btk_menu_item_toggle_size_allocate (BtkMenuItem         *menu_item,
					       bint                 allocation);
void       btk_menu_item_set_right_justified  (BtkMenuItem         *menu_item,
					       bboolean             right_justified);
bboolean   btk_menu_item_get_right_justified  (BtkMenuItem         *menu_item);
void	   btk_menu_item_set_accel_path	      (BtkMenuItem	   *menu_item,
					       const bchar	   *accel_path);
const bchar* btk_menu_item_get_accel_path     (BtkMenuItem    *menu_item);

void       btk_menu_item_set_label            (BtkMenuItem         *menu_item,
 					       const bchar         *label);
const bchar *btk_menu_item_get_label          (BtkMenuItem         *menu_item);

void       btk_menu_item_set_use_underline    (BtkMenuItem         *menu_item,
 					       bboolean             setting);
bboolean   btk_menu_item_get_use_underline    (BtkMenuItem         *menu_item);

/* private */
void	  _btk_menu_item_refresh_accel_path   (BtkMenuItem	   *menu_item,
					       const bchar	   *prefix,
					       BtkAccelGroup	   *accel_group,
					       bboolean		    group_changed);
bboolean  _btk_menu_item_is_selectable        (BtkWidget           *menu_item);
void      _btk_menu_item_popup_submenu        (BtkWidget           *menu_item,
                                               bboolean             with_delay);
void      _btk_menu_item_popdown_submenu      (BtkWidget           *menu_item);

#ifndef BTK_DISABLE_DEPRECATED
void       btk_menu_item_remove_submenu       (BtkMenuItem         *menu_item);
#define btk_menu_item_right_justify(menu_item) btk_menu_item_set_right_justified ((menu_item), TRUE)
#endif /* BTK_DISABLE_DEPRECATED */

B_END_DECLS

#endif /* __BTK_MENU_ITEM_H__ */
