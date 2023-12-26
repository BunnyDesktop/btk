/* btkrbtree.h
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

/* A Red-Black Tree implementation used specifically by BtkTreeView.
 */
#ifndef __BTK_RBTREE_H__
#define __BTK_RBTREE_H__

#include <bunnylib.h>


B_BEGIN_DECLS


typedef enum
{
  BTK_RBNODE_BLACK = 1 << 0,
  BTK_RBNODE_RED = 1 << 1,
  BTK_RBNODE_IS_PARENT = 1 << 2,
  BTK_RBNODE_IS_SELECTED = 1 << 3,
  BTK_RBNODE_IS_PRELIT = 1 << 4,
  BTK_RBNODE_IS_SEMI_COLLAPSED = 1 << 5,
  BTK_RBNODE_IS_SEMI_EXPANDED = 1 << 6,
  BTK_RBNODE_INVALID = 1 << 7,
  BTK_RBNODE_COLUMN_INVALID = 1 << 8,
  BTK_RBNODE_DESCENDANTS_INVALID = 1 << 9,
  BTK_RBNODE_NON_COLORS = BTK_RBNODE_IS_PARENT |
  			  BTK_RBNODE_IS_SELECTED |
  			  BTK_RBNODE_IS_PRELIT |
                          BTK_RBNODE_IS_SEMI_COLLAPSED |
                          BTK_RBNODE_IS_SEMI_EXPANDED |
                          BTK_RBNODE_INVALID |
                          BTK_RBNODE_COLUMN_INVALID |
                          BTK_RBNODE_DESCENDANTS_INVALID
} BtkRBNodeColor;

typedef struct _BtkRBTree BtkRBTree;
typedef struct _BtkRBNode BtkRBNode;
typedef struct _BtkRBTreeView BtkRBTreeView;

typedef void (*BtkRBTreeTraverseFunc) (BtkRBTree  *tree,
                                       BtkRBNode  *node,
                                       bpointer  data);

struct _BtkRBTree
{
  BtkRBNode *root;
  BtkRBNode *nil;
  BtkRBTree *parent_tree;
  BtkRBNode *parent_node;
};

struct _BtkRBNode
{
  buint flags : 14;

  /* We keep track of whether the aggregate count of children plus 1
   * for the node itself comes to an even number.  The parity flag is
   * the total count of children mod 2, where the total count of
   * children gets computed in the same way that the total offset gets
   * computed. i.e. not the same as the "count" field below which
   * doesn't include children. We could replace parity with a
   * full-size int field here, and then take % 2 to get the parity flag,
   * but that would use extra memory.
   */

  buint parity : 1;
  
  BtkRBNode *left;
  BtkRBNode *right;
  BtkRBNode *parent;

  /* count is the number of nodes beneath us, plus 1 for ourselves.
   * i.e. node->left->count + node->right->count + 1
   */
  bint count;
  
  /* this is the total of sizes of
   * node->left, node->right, our own height, and the height
   * of all trees in ->children, iff children exists because
   * the thing is expanded.
   */
  bint offset;

  /* Child trees */
  BtkRBTree *children;
};


#define BTK_RBNODE_GET_COLOR(node)		(node?(((node->flags&BTK_RBNODE_RED)==BTK_RBNODE_RED)?BTK_RBNODE_RED:BTK_RBNODE_BLACK):BTK_RBNODE_BLACK)
#define BTK_RBNODE_SET_COLOR(node,color) 	if((node->flags&color)!=color)node->flags=node->flags^(BTK_RBNODE_RED|BTK_RBNODE_BLACK)
#define BTK_RBNODE_GET_HEIGHT(node) 		(node->offset-(node->left->offset+node->right->offset+(node->children?node->children->root->offset:0)))
#define BTK_RBNODE_SET_FLAG(node, flag)   	B_STMT_START{ (node->flags|=flag); }B_STMT_END
#define BTK_RBNODE_UNSET_FLAG(node, flag) 	B_STMT_START{ (node->flags&=~(flag)); }B_STMT_END
#define BTK_RBNODE_FLAG_SET(node, flag) 	(node?(((node->flags&flag)==flag)?TRUE:FALSE):FALSE)


BtkRBTree *_btk_rbtree_new              (void);
void       _btk_rbtree_free             (BtkRBTree              *tree);
void       _btk_rbtree_remove           (BtkRBTree              *tree);
void       _btk_rbtree_destroy          (BtkRBTree              *tree);
BtkRBNode *_btk_rbtree_insert_before    (BtkRBTree              *tree,
					 BtkRBNode              *node,
					 bint                    height,
					 bboolean                valid);
BtkRBNode *_btk_rbtree_insert_after     (BtkRBTree              *tree,
					 BtkRBNode              *node,
					 bint                    height,
					 bboolean                valid);
void       _btk_rbtree_remove_node      (BtkRBTree              *tree,
					 BtkRBNode              *node);
void       _btk_rbtree_reorder          (BtkRBTree              *tree,
					 bint                   *new_order,
					 bint                    length);
BtkRBNode *_btk_rbtree_find_count       (BtkRBTree              *tree,
					 bint                    count);
void       _btk_rbtree_node_set_height  (BtkRBTree              *tree,
					 BtkRBNode              *node,
					 bint                    height);
void       _btk_rbtree_node_mark_invalid(BtkRBTree              *tree,
					 BtkRBNode              *node);
void       _btk_rbtree_node_mark_valid  (BtkRBTree              *tree,
					 BtkRBNode              *node);
void       _btk_rbtree_column_invalid   (BtkRBTree              *tree);
void       _btk_rbtree_mark_invalid     (BtkRBTree              *tree);
void       _btk_rbtree_set_fixed_height (BtkRBTree              *tree,
					 bint                    height,
					 bboolean                mark_valid);
bint       _btk_rbtree_node_find_offset (BtkRBTree              *tree,
					 BtkRBNode              *node);
bint       _btk_rbtree_node_find_parity (BtkRBTree              *tree,
					 BtkRBNode              *node);
bint       _btk_rbtree_find_offset      (BtkRBTree              *tree,
					 bint                    offset,
					 BtkRBTree             **new_tree,
					 BtkRBNode             **new_node);
void       _btk_rbtree_traverse         (BtkRBTree              *tree,
					 BtkRBNode              *node,
					 GTraverseType           order,
					 BtkRBTreeTraverseFunc   func,
					 bpointer                data);
BtkRBNode *_btk_rbtree_next             (BtkRBTree              *tree,
					 BtkRBNode              *node);
BtkRBNode *_btk_rbtree_prev             (BtkRBTree              *tree,
					 BtkRBNode              *node);
void       _btk_rbtree_next_full        (BtkRBTree              *tree,
					 BtkRBNode              *node,
					 BtkRBTree             **new_tree,
					 BtkRBNode             **new_node);
void       _btk_rbtree_prev_full        (BtkRBTree              *tree,
					 BtkRBNode              *node,
					 BtkRBTree             **new_tree,
					 BtkRBNode             **new_node);

bint       _btk_rbtree_get_depth        (BtkRBTree              *tree);

/* This func checks the integrity of the tree */
#ifdef G_ENABLE_DEBUG  
void       _btk_rbtree_test             (const bchar            *where,
                                         BtkRBTree              *tree);
void       _btk_rbtree_debug_spew       (BtkRBTree              *tree);
#endif


B_END_DECLS


#endif /* __BTK_RBTREE_H__ */
