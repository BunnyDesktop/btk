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
 * Modified by the BTK+ Team and others 1997-2001.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/.
 */

#ifndef __BTK_CHECK_MENU_ITEM_H__
#define __BTK_CHECK_MENU_ITEM_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkmenuitem.h>


G_BEGIN_DECLS

#define BTK_TYPE_CHECK_MENU_ITEM            (btk_check_menu_item_get_type ())
#define BTK_CHECK_MENU_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_CHECK_MENU_ITEM, BtkCheckMenuItem))
#define BTK_CHECK_MENU_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_CHECK_MENU_ITEM, BtkCheckMenuItemClass))
#define BTK_IS_CHECK_MENU_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_CHECK_MENU_ITEM))
#define BTK_IS_CHECK_MENU_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_CHECK_MENU_ITEM))
#define BTK_CHECK_MENU_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_CHECK_MENU_ITEM, BtkCheckMenuItemClass))


typedef struct _BtkCheckMenuItem       BtkCheckMenuItem;
typedef struct _BtkCheckMenuItemClass  BtkCheckMenuItemClass;

struct _BtkCheckMenuItem
{
  BtkMenuItem menu_item;

  guint GSEAL (active) : 1;
  guint GSEAL (always_show_toggle) : 1;
  guint GSEAL (inconsistent) : 1;
  guint GSEAL (draw_as_radio) : 1;
};

struct _BtkCheckMenuItemClass
{
  BtkMenuItemClass parent_class;

  void (* toggled)	  (BtkCheckMenuItem *check_menu_item);
  void (* draw_indicator) (BtkCheckMenuItem *check_menu_item,
			   BdkRectangle	    *area);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};


GType	   btk_check_menu_item_get_type	         (void) G_GNUC_CONST;

BtkWidget* btk_check_menu_item_new               (void);
BtkWidget* btk_check_menu_item_new_with_label    (const gchar      *label);
BtkWidget* btk_check_menu_item_new_with_mnemonic (const gchar      *label);
void       btk_check_menu_item_set_active        (BtkCheckMenuItem *check_menu_item,
						  gboolean          is_active);
gboolean   btk_check_menu_item_get_active        (BtkCheckMenuItem *check_menu_item);
void       btk_check_menu_item_toggled           (BtkCheckMenuItem *check_menu_item);
void       btk_check_menu_item_set_inconsistent  (BtkCheckMenuItem *check_menu_item,
						  gboolean          setting);
gboolean   btk_check_menu_item_get_inconsistent  (BtkCheckMenuItem *check_menu_item);
void       btk_check_menu_item_set_draw_as_radio (BtkCheckMenuItem *check_menu_item,
						  gboolean          draw_as_radio);
gboolean   btk_check_menu_item_get_draw_as_radio (BtkCheckMenuItem *check_menu_item);


#ifndef BTK_DISABLE_DEPRECATED
void	   btk_check_menu_item_set_show_toggle (BtkCheckMenuItem *menu_item,
						gboolean	  always);
#define	btk_check_menu_item_set_state		btk_check_menu_item_set_active
#endif

G_END_DECLS

#endif /* __BTK_CHECK_MENU_ITEM_H__ */
