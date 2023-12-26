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

#ifdef BTK_ENABLE_BROKEN

#ifndef __BTK_TREE_ITEM_H__
#define __BTK_TREE_ITEM_H__


#include <btk/btkitem.h>


B_BEGIN_DECLS

#define BTK_TYPE_TREE_ITEM              (btk_tree_item_get_type ())
#define BTK_TREE_ITEM(obj)              (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_TREE_ITEM, BtkTreeItem))
#define BTK_TREE_ITEM_CLASS(klass)      (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_TREE_ITEM, BtkTreeItemClass))
#define BTK_IS_TREE_ITEM(obj)           (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_TREE_ITEM))
#define BTK_IS_TREE_ITEM_CLASS(klass)   (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_TREE_ITEM))
#define BTK_TREE_ITEM_GET_CLASS(obj)    (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_TREE_ITEM, BtkTreeItemClass))


#define BTK_TREE_ITEM_SUBTREE(obj)      (BTK_TREE_ITEM(obj)->subtree)


typedef struct _BtkTreeItem       BtkTreeItem;
typedef struct _BtkTreeItemClass  BtkTreeItemClass;

struct _BtkTreeItem
{
  BtkItem item;

  BtkWidget *subtree;
  BtkWidget *pixmaps_box;
  BtkWidget *plus_pix_widget, *minus_pix_widget;

  GList *pixmaps;		/* pixmap node for this items color depth */

  buint expanded : 1;
};

struct _BtkTreeItemClass
{
  BtkItemClass parent_class;

  void (* expand)   (BtkTreeItem *tree_item);
  void (* collapse) (BtkTreeItem *tree_item);
};


GType      btk_tree_item_get_type       (void) B_GNUC_CONST;
BtkWidget* btk_tree_item_new            (void);
BtkWidget* btk_tree_item_new_with_label (const bchar *label);
void       btk_tree_item_set_subtree    (BtkTreeItem *tree_item,
					 BtkWidget   *subtree);
void       btk_tree_item_remove_subtree (BtkTreeItem *tree_item);
void       btk_tree_item_select         (BtkTreeItem *tree_item);
void       btk_tree_item_deselect       (BtkTreeItem *tree_item);
void       btk_tree_item_expand         (BtkTreeItem *tree_item);
void       btk_tree_item_collapse       (BtkTreeItem *tree_item);


B_END_DECLS

#endif /* __BTK_TREE_ITEM_H__ */

#endif /* BTK_ENABLE_BROKEN */
