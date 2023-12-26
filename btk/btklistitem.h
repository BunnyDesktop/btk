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

#if !defined (BTK_DISABLE_DEPRECATED) || defined (__BTK_LIST_ITEM_C__)

#ifndef __BTK_LIST_ITEM_H__
#define __BTK_LIST_ITEM_H__

#include <btk/btk.h>


G_BEGIN_DECLS


#define BTK_TYPE_LIST_ITEM              (btk_list_item_get_type ())
#define BTK_LIST_ITEM(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_LIST_ITEM, BtkListItem))
#define BTK_LIST_ITEM_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_LIST_ITEM, BtkListItemClass))
#define BTK_IS_LIST_ITEM(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_LIST_ITEM))
#define BTK_IS_LIST_ITEM_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_LIST_ITEM))
#define BTK_LIST_ITEM_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_LIST_ITEM, BtkListItemClass))


typedef struct _BtkListItem       BtkListItem;
typedef struct _BtkListItemClass  BtkListItemClass;

struct _BtkListItem
{
  BtkItem item;
};

struct _BtkListItemClass
{
  BtkItemClass parent_class;

  void (*toggle_focus_row)  (BtkListItem   *list_item);
  void (*select_all)        (BtkListItem   *list_item);
  void (*unselect_all)      (BtkListItem   *list_item);
  void (*undo_selection)    (BtkListItem   *list_item);
  void (*start_selection)   (BtkListItem   *list_item);
  void (*end_selection)     (BtkListItem   *list_item);
  void (*extend_selection)  (BtkListItem   *list_item,
			     BtkScrollType  scroll_type,
			     gfloat         position,
			     gboolean       auto_start_selection);
  void (*scroll_horizontal) (BtkListItem   *list_item,
			     BtkScrollType  scroll_type,
			     gfloat         position);
  void (*scroll_vertical)   (BtkListItem   *list_item,
			     BtkScrollType  scroll_type,
			     gfloat         position);
  void (*toggle_add_mode)   (BtkListItem   *list_item);
};


GType      btk_list_item_get_type       (void) G_GNUC_CONST;
BtkWidget* btk_list_item_new            (void);
BtkWidget* btk_list_item_new_with_label (const gchar      *label);
void       btk_list_item_select         (BtkListItem      *list_item);
void       btk_list_item_deselect       (BtkListItem      *list_item);



G_END_DECLS


#endif /* __BTK_LIST_ITEM_H__ */

#endif /* BTK_DISABLE_DEPRECATED */
