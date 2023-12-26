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

#ifndef __BTK_TREE_H__
#define __BTK_TREE_H__


#include <btk/btkcontainer.h>


B_BEGIN_DECLS

/* set this flag to enable tree debugging output */
/* #define TREE_DEBUG */

#define BTK_TYPE_TREE                  (btk_tree_get_type ())
#define BTK_TREE(obj)                  (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_TREE, BtkTree))
#define BTK_TREE_CLASS(klass)          (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_TREE, BtkTreeClass))
#define BTK_IS_TREE(obj)               (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_TREE))
#define BTK_IS_TREE_CLASS(klass)       (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_TREE))
#define BTK_TREE_GET_CLASS(obj)        (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_TREE, BtkTreeClass))


#define BTK_IS_ROOT_TREE(obj)   ((BtkObject*) BTK_TREE(obj)->root_tree == (BtkObject*)obj)
#define BTK_TREE_ROOT_TREE(obj) (BTK_TREE(obj)->root_tree ? BTK_TREE(obj)->root_tree : BTK_TREE(obj))
#define BTK_TREE_SELECTION_OLD(obj) (BTK_TREE_ROOT_TREE(obj)->selection)

typedef enum 
{
  BTK_TREE_VIEW_LINE,  /* default view mode */
  BTK_TREE_VIEW_ITEM
} BtkTreeViewMode;

typedef struct _BtkTree       BtkTree;
typedef struct _BtkTreeClass  BtkTreeClass;

struct _BtkTree
{
  BtkContainer container;
  
  GList *children;
  
  BtkTree* root_tree; /* owner of selection list */
  BtkWidget* tree_owner;
  GList *selection;
  buint level;
  buint indent_value;
  buint current_indent;
  buint selection_mode : 2;
  buint view_mode : 1;
  buint view_line : 1;
};

struct _BtkTreeClass
{
  BtkContainerClass parent_class;
  
  void (* selection_changed) (BtkTree   *tree);
  void (* select_child)      (BtkTree   *tree,
			      BtkWidget *child);
  void (* unselect_child)    (BtkTree   *tree,
			      BtkWidget *child);
};


GType      btk_tree_get_type           (void) B_GNUC_CONST;
BtkWidget* btk_tree_new                (void);
void       btk_tree_append             (BtkTree          *tree,
				        BtkWidget        *tree_item);
void       btk_tree_prepend            (BtkTree          *tree,
				        BtkWidget        *tree_item);
void       btk_tree_insert             (BtkTree          *tree,
				        BtkWidget        *tree_item,
				        bint              position);
void       btk_tree_remove_items       (BtkTree          *tree,
				        GList            *items);
void       btk_tree_clear_items        (BtkTree          *tree,
				        bint              start,
				        bint              end);
void       btk_tree_select_item        (BtkTree          *tree,
				        bint              item);
void       btk_tree_unselect_item      (BtkTree          *tree,
				        bint              item);
void       btk_tree_select_child       (BtkTree          *tree,
				        BtkWidget        *tree_item);
void       btk_tree_unselect_child     (BtkTree          *tree,
				        BtkWidget        *tree_item);
bint       btk_tree_child_position     (BtkTree          *tree,
				        BtkWidget        *child);
void       btk_tree_set_selection_mode (BtkTree          *tree,
				        BtkSelectionMode  mode);
void       btk_tree_set_view_mode      (BtkTree          *tree,
				        BtkTreeViewMode   mode); 
void       btk_tree_set_view_lines     (BtkTree          *tree,
					bboolean	  flag);

/* deprecated function, use btk_container_remove instead.
 */
void       btk_tree_remove_item        (BtkTree          *tree,
				        BtkWidget        *child);


B_END_DECLS

#endif /* __BTK_TREE_H__ */

#endif /* BTK_ENABLE_BROKEN */
