/* btktreeselection.h
 * Copyright (C) 2000  Red Hat, Inc.,  Jonathan Blandford <jrb@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __BTK_TREE_SELECTION_H__
#define __BTK_TREE_SELECTION_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btktreeview.h>

B_BEGIN_DECLS


#define BTK_TYPE_TREE_SELECTION			(btk_tree_selection_get_type ())
#define BTK_TREE_SELECTION(obj)			(B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_TREE_SELECTION, BtkTreeSelection))
#define BTK_TREE_SELECTION_CLASS(klass)		(B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_TREE_SELECTION, BtkTreeSelectionClass))
#define BTK_IS_TREE_SELECTION(obj)		(B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_TREE_SELECTION))
#define BTK_IS_TREE_SELECTION_CLASS(klass)	(B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_TREE_SELECTION))
#define BTK_TREE_SELECTION_GET_CLASS(obj)	(B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_TREE_SELECTION, BtkTreeSelectionClass))

typedef gboolean (* BtkTreeSelectionFunc)    (BtkTreeSelection  *selection,
					      BtkTreeModel      *model,
					      BtkTreePath       *path,
                                              gboolean           path_currently_selected,
					      gpointer           data);
typedef void (* BtkTreeSelectionForeachFunc) (BtkTreeModel      *model,
					      BtkTreePath       *path,
					      BtkTreeIter       *iter,
					      gpointer           data);

struct _BtkTreeSelection
{
  BObject parent;

  /*< private >*/

  BtkTreeView *GSEAL (tree_view);
  BtkSelectionMode GSEAL (type);
  BtkTreeSelectionFunc GSEAL (user_func);
  gpointer GSEAL (user_data);
  GDestroyNotify GSEAL (destroy);
};

struct _BtkTreeSelectionClass
{
  BObjectClass parent_class;

  void (* changed) (BtkTreeSelection *selection);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};


GType            btk_tree_selection_get_type            (void) B_GNUC_CONST;

void             btk_tree_selection_set_mode            (BtkTreeSelection            *selection,
							 BtkSelectionMode             type);
BtkSelectionMode btk_tree_selection_get_mode        (BtkTreeSelection            *selection);
void             btk_tree_selection_set_select_function (BtkTreeSelection            *selection,
							 BtkTreeSelectionFunc         func,
							 gpointer                     data,
							 GDestroyNotify               destroy);
gpointer         btk_tree_selection_get_user_data       (BtkTreeSelection            *selection);
BtkTreeView*     btk_tree_selection_get_tree_view       (BtkTreeSelection            *selection);

BtkTreeSelectionFunc btk_tree_selection_get_select_function (BtkTreeSelection        *selection);

/* Only meaningful if BTK_SELECTION_SINGLE or BTK_SELECTION_BROWSE is set */
/* Use selected_foreach or get_selected_rows for BTK_SELECTION_MULTIPLE */
gboolean         btk_tree_selection_get_selected        (BtkTreeSelection            *selection,
							 BtkTreeModel               **model,
							 BtkTreeIter                 *iter);
GList *          btk_tree_selection_get_selected_rows   (BtkTreeSelection            *selection,
                                                         BtkTreeModel               **model);
gint             btk_tree_selection_count_selected_rows (BtkTreeSelection            *selection);
void             btk_tree_selection_selected_foreach    (BtkTreeSelection            *selection,
							 BtkTreeSelectionForeachFunc  func,
							 gpointer                     data);
void             btk_tree_selection_select_path         (BtkTreeSelection            *selection,
							 BtkTreePath                 *path);
void             btk_tree_selection_unselect_path       (BtkTreeSelection            *selection,
							 BtkTreePath                 *path);
void             btk_tree_selection_select_iter         (BtkTreeSelection            *selection,
							 BtkTreeIter                 *iter);
void             btk_tree_selection_unselect_iter       (BtkTreeSelection            *selection,
							 BtkTreeIter                 *iter);
gboolean         btk_tree_selection_path_is_selected    (BtkTreeSelection            *selection,
							 BtkTreePath                 *path);
gboolean         btk_tree_selection_iter_is_selected    (BtkTreeSelection            *selection,
							 BtkTreeIter                 *iter);
void             btk_tree_selection_select_all          (BtkTreeSelection            *selection);
void             btk_tree_selection_unselect_all        (BtkTreeSelection            *selection);
void             btk_tree_selection_select_range        (BtkTreeSelection            *selection,
							 BtkTreePath                 *start_path,
							 BtkTreePath                 *end_path);
void             btk_tree_selection_unselect_range      (BtkTreeSelection            *selection,
                                                         BtkTreePath                 *start_path,
							 BtkTreePath                 *end_path);


B_END_DECLS

#endif /* __BTK_TREE_SELECTION_H__ */
