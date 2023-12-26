/* btktreemodelfilter.h
 * Copyright (C) 2000,2001  Red Hat, Inc., Jonathan Blandford <jrb@redhat.com>
 * Copyright (C) 2001-2003  Kristian Rietveld <kris@btk.org>
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

#ifndef __BTK_TREE_MODEL_FILTER_H__
#define __BTK_TREE_MODEL_FILTER_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <bdkconfig.h>
#include <btk/btktreemodel.h>

B_BEGIN_DECLS

#define BTK_TYPE_TREE_MODEL_FILTER              (btk_tree_model_filter_get_type ())
#define BTK_TREE_MODEL_FILTER(obj)              (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_TREE_MODEL_FILTER, BtkTreeModelFilter))
#define BTK_TREE_MODEL_FILTER_CLASS(vtable)     (B_TYPE_CHECK_CLASS_CAST ((vtable), BTK_TYPE_TREE_MODEL_FILTER, BtkTreeModelFilterClass))
#define BTK_IS_TREE_MODEL_FILTER(obj)           (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_TREE_MODEL_FILTER))
#define BTK_IS_TREE_MODEL_FILTER_CLASS(vtable)  (B_TYPE_CHECK_CLASS_TYPE ((vtable), BTK_TYPE_TREE_MODEL_FILTER))
#define BTK_TREE_MODEL_FILTER_GET_CLASS(obj)   (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_TREE_MODEL_FILTER, BtkTreeModelFilterClass))

typedef bboolean (* BtkTreeModelFilterVisibleFunc) (BtkTreeModel *model,
                                                    BtkTreeIter  *iter,
                                                    bpointer      data);
typedef void (* BtkTreeModelFilterModifyFunc) (BtkTreeModel *model,
                                               BtkTreeIter  *iter,
                                               BValue       *value,
                                               bint          column,
                                               bpointer      data);

typedef struct _BtkTreeModelFilter          BtkTreeModelFilter;
typedef struct _BtkTreeModelFilterClass     BtkTreeModelFilterClass;
typedef struct _BtkTreeModelFilterPrivate   BtkTreeModelFilterPrivate;

struct _BtkTreeModelFilter
{
  BObject parent;

  /*< private >*/
  BtkTreeModelFilterPrivate *GSEAL (priv);
};

struct _BtkTreeModelFilterClass
{
  BObjectClass parent_class;

  /* Padding for future expansion */
  void (*_btk_reserved0) (void);
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
};

/* base */
GType         btk_tree_model_filter_get_type                   (void) B_GNUC_CONST;
BtkTreeModel *btk_tree_model_filter_new                        (BtkTreeModel                 *child_model,
                                                                BtkTreePath                  *root);
void          btk_tree_model_filter_set_visible_func           (BtkTreeModelFilter           *filter,
                                                                BtkTreeModelFilterVisibleFunc func,
                                                                bpointer                      data,
                                                                GDestroyNotify                destroy);
void          btk_tree_model_filter_set_modify_func            (BtkTreeModelFilter           *filter,
                                                                bint                          n_columns,
                                                                GType                        *types,
                                                                BtkTreeModelFilterModifyFunc  func,
                                                                bpointer                      data,
                                                                GDestroyNotify                destroy);
void          btk_tree_model_filter_set_visible_column         (BtkTreeModelFilter           *filter,
                                                                bint                          column);

BtkTreeModel *btk_tree_model_filter_get_model                  (BtkTreeModelFilter           *filter);

/* conversion */
bboolean      btk_tree_model_filter_convert_child_iter_to_iter (BtkTreeModelFilter           *filter,
                                                                BtkTreeIter                  *filter_iter,
                                                                BtkTreeIter                  *child_iter);
void          btk_tree_model_filter_convert_iter_to_child_iter (BtkTreeModelFilter           *filter,
                                                                BtkTreeIter                  *child_iter,
                                                                BtkTreeIter                  *filter_iter);
BtkTreePath  *btk_tree_model_filter_convert_child_path_to_path (BtkTreeModelFilter           *filter,
                                                                BtkTreePath                  *child_path);
BtkTreePath  *btk_tree_model_filter_convert_path_to_child_path (BtkTreeModelFilter           *filter,
                                                                BtkTreePath                  *filter_path);

/* extras */
void          btk_tree_model_filter_refilter                   (BtkTreeModelFilter           *filter);
void          btk_tree_model_filter_clear_cache                (BtkTreeModelFilter           *filter);

B_END_DECLS

#endif /* __BTK_TREE_MODEL_FILTER_H__ */
