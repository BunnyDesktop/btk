/* BTK - The GIMP Toolkit
 * Copyright (C) Red Hat, Inc.
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

#ifndef __BTK_IMAGE_MENU_ITEM_H__
#define __BTK_IMAGE_MENU_ITEM_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkmenuitem.h>


B_BEGIN_DECLS

#define BTK_TYPE_IMAGE_MENU_ITEM            (btk_image_menu_item_get_type ())
#define BTK_IMAGE_MENU_ITEM(obj)            (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_IMAGE_MENU_ITEM, BtkImageMenuItem))
#define BTK_IMAGE_MENU_ITEM_CLASS(klass)    (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_IMAGE_MENU_ITEM, BtkImageMenuItemClass))
#define BTK_IS_IMAGE_MENU_ITEM(obj)         (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_IMAGE_MENU_ITEM))
#define BTK_IS_IMAGE_MENU_ITEM_CLASS(klass) (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_IMAGE_MENU_ITEM))
#define BTK_IMAGE_MENU_ITEM_GET_CLASS(obj)  (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_IMAGE_MENU_ITEM, BtkImageMenuItemClass))


typedef struct _BtkImageMenuItem       BtkImageMenuItem;
typedef struct _BtkImageMenuItemClass  BtkImageMenuItemClass;

struct _BtkImageMenuItem
{
  BtkMenuItem menu_item;

  /*< private >*/
  BtkWidget      *GSEAL (image);

};

struct _BtkImageMenuItemClass
{
  BtkMenuItemClass parent_class;
};


GType	   btk_image_menu_item_get_type          (void) B_GNUC_CONST;
BtkWidget* btk_image_menu_item_new               (void);
BtkWidget* btk_image_menu_item_new_with_label    (const bchar      *label);
BtkWidget* btk_image_menu_item_new_with_mnemonic (const bchar      *label);
BtkWidget* btk_image_menu_item_new_from_stock    (const bchar      *stock_id,
                                                  BtkAccelGroup    *accel_group);
void       btk_image_menu_item_set_always_show_image (BtkImageMenuItem *image_menu_item,
                                                      bboolean          always_show);
bboolean   btk_image_menu_item_get_always_show_image (BtkImageMenuItem *image_menu_item);
void       btk_image_menu_item_set_image         (BtkImageMenuItem *image_menu_item,
                                                  BtkWidget        *image);
BtkWidget* btk_image_menu_item_get_image         (BtkImageMenuItem *image_menu_item);
void       btk_image_menu_item_set_use_stock     (BtkImageMenuItem *image_menu_item,
						  bboolean          use_stock);
bboolean   btk_image_menu_item_get_use_stock     (BtkImageMenuItem *image_menu_item);
void       btk_image_menu_item_set_accel_group   (BtkImageMenuItem *image_menu_item, 
						  BtkAccelGroup    *accel_group);

B_END_DECLS

#endif /* __BTK_IMAGE_MENU_ITEM_H__ */
