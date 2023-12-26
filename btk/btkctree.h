/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball, Josh MacDonald
 * Copyright (C) 1997-1998 Jay Painter <jpaint@serv.net><jpaint@gimp.org>
 *
 * BtkCTree widget for BTK+
 * Copyright (C) 1998 Lars Hamann and Stefan Jeske
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

#if !defined (BTK_DISABLE_DEPRECATED) || defined (__BTK_CLIST_C__) || defined (__BTK_CTREE_C__)

#ifndef __BTK_CTREE_H__
#define __BTK_CTREE_H__

#include <btk/btkclist.h>

B_BEGIN_DECLS

#define BTK_TYPE_CTREE            (btk_ctree_get_type ())
#define BTK_CTREE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_CTREE, BtkCTree))
#define BTK_CTREE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_CTREE, BtkCTreeClass))
#define BTK_IS_CTREE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_CTREE))
#define BTK_IS_CTREE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_CTREE))
#define BTK_CTREE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_CTREE, BtkCTreeClass))

#define BTK_CTREE_ROW(_node_) ((BtkCTreeRow *)(((GList *)(_node_))->data))
#define BTK_CTREE_NODE(_node_) ((BtkCTreeNode *)((_node_)))
#define BTK_CTREE_NODE_NEXT(_nnode_) ((BtkCTreeNode *)(((GList *)(_nnode_))->next))
#define BTK_CTREE_NODE_PREV(_pnode_) ((BtkCTreeNode *)(((GList *)(_pnode_))->prev))
#define BTK_CTREE_FUNC(_func_) ((BtkCTreeFunc)(_func_))

#define BTK_TYPE_CTREE_NODE (btk_ctree_node_get_type ())

typedef enum
{
  BTK_CTREE_POS_BEFORE,
  BTK_CTREE_POS_AS_CHILD,
  BTK_CTREE_POS_AFTER
} BtkCTreePos;

typedef enum
{
  BTK_CTREE_LINES_NONE,
  BTK_CTREE_LINES_SOLID,
  BTK_CTREE_LINES_DOTTED,
  BTK_CTREE_LINES_TABBED
} BtkCTreeLineStyle;

typedef enum
{
  BTK_CTREE_EXPANDER_NONE,
  BTK_CTREE_EXPANDER_SQUARE,
  BTK_CTREE_EXPANDER_TRIANGLE,
  BTK_CTREE_EXPANDER_CIRCULAR
} BtkCTreeExpanderStyle;

typedef enum
{
  BTK_CTREE_EXPANSION_EXPAND,
  BTK_CTREE_EXPANSION_EXPAND_RECURSIVE,
  BTK_CTREE_EXPANSION_COLLAPSE,
  BTK_CTREE_EXPANSION_COLLAPSE_RECURSIVE,
  BTK_CTREE_EXPANSION_TOGGLE,
  BTK_CTREE_EXPANSION_TOGGLE_RECURSIVE
} BtkCTreeExpansionType;

typedef struct _BtkCTree      BtkCTree;
typedef struct _BtkCTreeClass BtkCTreeClass;
typedef struct _BtkCTreeRow   BtkCTreeRow;
typedef struct _BtkCTreeNode  BtkCTreeNode;

typedef void (*BtkCTreeFunc) (BtkCTree     *ctree,
			      BtkCTreeNode *node,
			      gpointer      data);

typedef gboolean (*BtkCTreeGNodeFunc) (BtkCTree     *ctree,
                                       guint         depth,
                                       GNode        *gnode,
				       BtkCTreeNode *cnode,
                                       gpointer      data);

typedef gboolean (*BtkCTreeCompareDragFunc) (BtkCTree     *ctree,
                                             BtkCTreeNode *source_node,
                                             BtkCTreeNode *new_parent,
                                             BtkCTreeNode *new_sibling);

struct _BtkCTree
{
  BtkCList clist;
  
  BdkGC *lines_gc;
  
  gint tree_indent;
  gint tree_spacing;
  gint tree_column;

  guint line_style     : 2;
  guint expander_style : 2;
  guint show_stub      : 1;

  BtkCTreeCompareDragFunc drag_compare;
};

struct _BtkCTreeClass
{
  BtkCListClass parent_class;
  
  void (*tree_select_row)   (BtkCTree     *ctree,
			     BtkCTreeNode *row,
			     gint          column);
  void (*tree_unselect_row) (BtkCTree     *ctree,
			     BtkCTreeNode *row,
			     gint          column);
  void (*tree_expand)       (BtkCTree     *ctree,
			     BtkCTreeNode *node);
  void (*tree_collapse)     (BtkCTree     *ctree,
			     BtkCTreeNode *node);
  void (*tree_move)         (BtkCTree     *ctree,
			     BtkCTreeNode *node,
			     BtkCTreeNode *new_parent,
			     BtkCTreeNode *new_sibling);
  void (*change_focus_row_expansion) (BtkCTree *ctree,
				      BtkCTreeExpansionType action);
};

struct _BtkCTreeRow
{
  BtkCListRow row;
  
  BtkCTreeNode *parent;
  BtkCTreeNode *sibling;
  BtkCTreeNode *children;
  
  BdkPixmap *pixmap_closed;
  BdkBitmap *mask_closed;
  BdkPixmap *pixmap_opened;
  BdkBitmap *mask_opened;
  
  guint16 level;
  
  guint is_leaf  : 1;
  guint expanded : 1;
};

struct _BtkCTreeNode {
  GList list;
};


/***********************************************************
 *           Creation, insertion, deletion                 *
 ***********************************************************/

GType   btk_ctree_get_type                       (void) B_GNUC_CONST;
BtkWidget * btk_ctree_new_with_titles            (gint          columns, 
						  gint          tree_column,
						  gchar        *titles[]);
BtkWidget * btk_ctree_new                        (gint          columns, 
						  gint          tree_column);
BtkCTreeNode * btk_ctree_insert_node             (BtkCTree     *ctree,
						  BtkCTreeNode *parent, 
						  BtkCTreeNode *sibling,
						  gchar        *text[],
						  guint8        spacing,
						  BdkPixmap    *pixmap_closed,
						  BdkBitmap    *mask_closed,
						  BdkPixmap    *pixmap_opened,
						  BdkBitmap    *mask_opened,
						  gboolean      is_leaf,
						  gboolean      expanded);
void btk_ctree_remove_node                       (BtkCTree     *ctree, 
						  BtkCTreeNode *node);
BtkCTreeNode * btk_ctree_insert_gnode            (BtkCTree          *ctree,
						  BtkCTreeNode      *parent,
						  BtkCTreeNode      *sibling,
						  GNode             *gnode,
						  BtkCTreeGNodeFunc  func,
						  gpointer           data);
GNode * btk_ctree_export_to_gnode                (BtkCTree          *ctree,
						  GNode             *parent,
						  GNode             *sibling,
						  BtkCTreeNode      *node,
						  BtkCTreeGNodeFunc  func,
						  gpointer           data);

/***********************************************************
 *  Generic recursive functions, querying / finding tree   *
 *  information                                            *
 ***********************************************************/

void btk_ctree_post_recursive                    (BtkCTree     *ctree, 
						  BtkCTreeNode *node,
						  BtkCTreeFunc  func,
						  gpointer      data);
void btk_ctree_post_recursive_to_depth           (BtkCTree     *ctree, 
						  BtkCTreeNode *node,
						  gint          depth,
						  BtkCTreeFunc  func,
						  gpointer      data);
void btk_ctree_pre_recursive                     (BtkCTree     *ctree, 
						  BtkCTreeNode *node,
						  BtkCTreeFunc  func,
						  gpointer      data);
void btk_ctree_pre_recursive_to_depth            (BtkCTree     *ctree, 
						  BtkCTreeNode *node,
						  gint          depth,
						  BtkCTreeFunc  func,
						  gpointer      data);
gboolean btk_ctree_is_viewable                   (BtkCTree     *ctree, 
					          BtkCTreeNode *node);
BtkCTreeNode * btk_ctree_last                    (BtkCTree     *ctree,
					          BtkCTreeNode *node);
BtkCTreeNode * btk_ctree_find_node_ptr           (BtkCTree     *ctree,
					          BtkCTreeRow  *ctree_row);
BtkCTreeNode * btk_ctree_node_nth                (BtkCTree     *ctree,
						  guint         row);
gboolean btk_ctree_find                          (BtkCTree     *ctree,
					          BtkCTreeNode *node,
					          BtkCTreeNode *child);
gboolean btk_ctree_is_ancestor                   (BtkCTree     *ctree,
					          BtkCTreeNode *node,
					          BtkCTreeNode *child);
BtkCTreeNode * btk_ctree_find_by_row_data        (BtkCTree     *ctree,
					          BtkCTreeNode *node,
					          gpointer      data);
/* returns a GList of all BtkCTreeNodes with row->data == data. */
GList * btk_ctree_find_all_by_row_data           (BtkCTree     *ctree,
						  BtkCTreeNode *node,
						  gpointer      data);
BtkCTreeNode * btk_ctree_find_by_row_data_custom (BtkCTree     *ctree,
						  BtkCTreeNode *node,
						  gpointer      data,
						  GCompareFunc  func);
/* returns a GList of all BtkCTreeNodes with row->data == data. */
GList * btk_ctree_find_all_by_row_data_custom    (BtkCTree     *ctree,
						  BtkCTreeNode *node,
						  gpointer      data,
						  GCompareFunc  func);
gboolean btk_ctree_is_hot_spot                   (BtkCTree     *ctree,
					          gint          x,
					          gint          y);

/***********************************************************
 *   Tree signals : move, expand, collapse, (un)select     *
 ***********************************************************/

void btk_ctree_move                              (BtkCTree     *ctree,
						  BtkCTreeNode *node,
						  BtkCTreeNode *new_parent, 
						  BtkCTreeNode *new_sibling);
void btk_ctree_expand                            (BtkCTree     *ctree,
						  BtkCTreeNode *node);
void btk_ctree_expand_recursive                  (BtkCTree     *ctree,
						  BtkCTreeNode *node);
void btk_ctree_expand_to_depth                   (BtkCTree     *ctree,
						  BtkCTreeNode *node,
						  gint          depth);
void btk_ctree_collapse                          (BtkCTree     *ctree,
						  BtkCTreeNode *node);
void btk_ctree_collapse_recursive                (BtkCTree     *ctree,
						  BtkCTreeNode *node);
void btk_ctree_collapse_to_depth                 (BtkCTree     *ctree,
						  BtkCTreeNode *node,
						  gint          depth);
void btk_ctree_toggle_expansion                  (BtkCTree     *ctree,
						  BtkCTreeNode *node);
void btk_ctree_toggle_expansion_recursive        (BtkCTree     *ctree,
						  BtkCTreeNode *node);
void btk_ctree_select                            (BtkCTree     *ctree, 
						  BtkCTreeNode *node);
void btk_ctree_select_recursive                  (BtkCTree     *ctree, 
						  BtkCTreeNode *node);
void btk_ctree_unselect                          (BtkCTree     *ctree, 
						  BtkCTreeNode *node);
void btk_ctree_unselect_recursive                (BtkCTree     *ctree, 
						  BtkCTreeNode *node);
void btk_ctree_real_select_recursive             (BtkCTree     *ctree, 
						  BtkCTreeNode *node, 
						  gint          state);

/***********************************************************
 *           Analogons of BtkCList functions               *
 ***********************************************************/

void btk_ctree_node_set_text                     (BtkCTree     *ctree,
						  BtkCTreeNode *node,
						  gint          column,
						  const gchar  *text);
void btk_ctree_node_set_pixmap                   (BtkCTree     *ctree,
						  BtkCTreeNode *node,
						  gint          column,
						  BdkPixmap    *pixmap,
						  BdkBitmap    *mask);
void btk_ctree_node_set_pixtext                  (BtkCTree     *ctree,
						  BtkCTreeNode *node,
						  gint          column,
						  const gchar  *text,
						  guint8        spacing,
						  BdkPixmap    *pixmap,
						  BdkBitmap    *mask);
void btk_ctree_set_node_info                     (BtkCTree     *ctree,
						  BtkCTreeNode *node,
						  const gchar  *text,
						  guint8        spacing,
						  BdkPixmap    *pixmap_closed,
						  BdkBitmap    *mask_closed,
						  BdkPixmap    *pixmap_opened,
						  BdkBitmap    *mask_opened,
						  gboolean      is_leaf,
						  gboolean      expanded);
void btk_ctree_node_set_shift                    (BtkCTree     *ctree,
						  BtkCTreeNode *node,
						  gint          column,
						  gint          vertical,
						  gint          horizontal);
void btk_ctree_node_set_selectable               (BtkCTree     *ctree,
						  BtkCTreeNode *node,
						  gboolean      selectable);
gboolean btk_ctree_node_get_selectable           (BtkCTree     *ctree,
						  BtkCTreeNode *node);
BtkCellType btk_ctree_node_get_cell_type         (BtkCTree     *ctree,
						  BtkCTreeNode *node,
						  gint          column);
gboolean btk_ctree_node_get_text                 (BtkCTree     *ctree,
						  BtkCTreeNode *node,
						  gint          column,
						  gchar       **text);
gboolean btk_ctree_node_get_pixmap               (BtkCTree     *ctree,
						  BtkCTreeNode *node,
						  gint          column,
						  BdkPixmap   **pixmap,
						  BdkBitmap   **mask);
gboolean btk_ctree_node_get_pixtext              (BtkCTree     *ctree,
						  BtkCTreeNode *node,
						  gint          column,
						  gchar       **text,
						  guint8       *spacing,
						  BdkPixmap   **pixmap,
						  BdkBitmap   **mask);
gboolean btk_ctree_get_node_info                 (BtkCTree     *ctree,
						  BtkCTreeNode *node,
						  gchar       **text,
						  guint8       *spacing,
						  BdkPixmap   **pixmap_closed,
						  BdkBitmap   **mask_closed,
						  BdkPixmap   **pixmap_opened,
						  BdkBitmap   **mask_opened,
						  gboolean     *is_leaf,
						  gboolean     *expanded);
void btk_ctree_node_set_row_style                (BtkCTree     *ctree,
						  BtkCTreeNode *node,
						  BtkStyle     *style);
BtkStyle * btk_ctree_node_get_row_style          (BtkCTree     *ctree,
						  BtkCTreeNode *node);
void btk_ctree_node_set_cell_style               (BtkCTree     *ctree,
						  BtkCTreeNode *node,
						  gint          column,
						  BtkStyle     *style);
BtkStyle * btk_ctree_node_get_cell_style         (BtkCTree     *ctree,
						  BtkCTreeNode *node,
						  gint          column);
void btk_ctree_node_set_foreground               (BtkCTree       *ctree,
						  BtkCTreeNode   *node,
						  const BdkColor *color);
void btk_ctree_node_set_background               (BtkCTree       *ctree,
						  BtkCTreeNode   *node,
						  const BdkColor *color);
void btk_ctree_node_set_row_data                 (BtkCTree     *ctree,
						  BtkCTreeNode *node,
						  gpointer      data);
void btk_ctree_node_set_row_data_full            (BtkCTree     *ctree,
						  BtkCTreeNode *node,
						  gpointer      data,
						  GDestroyNotify destroy);
gpointer btk_ctree_node_get_row_data             (BtkCTree     *ctree,
						  BtkCTreeNode *node);
void btk_ctree_node_moveto                       (BtkCTree     *ctree,
						  BtkCTreeNode *node,
						  gint          column,
						  gfloat        row_align,
						  gfloat        col_align);
BtkVisibility btk_ctree_node_is_visible          (BtkCTree     *ctree,
						  BtkCTreeNode *node);

/***********************************************************
 *             BtkCTree specific functions                 *
 ***********************************************************/

void btk_ctree_set_indent            (BtkCTree                *ctree, 
				      gint                     indent);
void btk_ctree_set_spacing           (BtkCTree                *ctree, 
				      gint                     spacing);
void btk_ctree_set_show_stub         (BtkCTree                *ctree, 
				      gboolean                 show_stub);
void btk_ctree_set_line_style        (BtkCTree                *ctree, 
				      BtkCTreeLineStyle        line_style);
void btk_ctree_set_expander_style    (BtkCTree                *ctree, 
				      BtkCTreeExpanderStyle    expander_style);
void btk_ctree_set_drag_compare_func (BtkCTree     	      *ctree,
				      BtkCTreeCompareDragFunc  cmp_func);

/***********************************************************
 *             Tree sorting functions                      *
 ***********************************************************/

void btk_ctree_sort_node                         (BtkCTree     *ctree, 
						  BtkCTreeNode *node);
void btk_ctree_sort_recursive                    (BtkCTree     *ctree, 
						  BtkCTreeNode *node);


#define btk_ctree_set_reorderable(t,r)                    btk_clist_set_reorderable((BtkCList*) (t),(r))

/* GType for the BtkCTreeNode.  This is a boxed type, although it uses
 * no-op's for the copy and free routines.  It is defined in order to
 * provide type information for the signal arguments
 */
GType   btk_ctree_node_get_type                  (void) B_GNUC_CONST;

B_END_DECLS

#endif				/* __BTK_CTREE_H__ */

#endif /* BTK_DISABLE_DEPRECATED */
