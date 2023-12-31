/*
 * Btktextbtree.c --
 *
 *      This file contains code that manages the B-tree representation
 *      of text for the text buffer and implements character and
 *      toggle segment types.
 *
 * Copyright (c) 1992-1994 The Regents of the University of California.
 * Copyright (c) 1994-1995 Sun Microsystems, Inc.
 * Copyright (c) 2000      Red Hat, Inc.
 * Tk -> Btk port by Havoc Pennington <hp@redhat.com>
 *
 * This software is copyrighted by the Regents of the University of
 * California, Sun Microsystems, Inc., and other parties.  The
 * following terms apply to all files associated with the software
 * unless explicitly disclaimed in individual files.
 *
 * The authors hereby grant permission to use, copy, modify,
 * distribute, and license this software and its documentation for any
 * purpose, provided that existing copyright notices are retained in
 * all copies and that this notice is included verbatim in any
 * distributions. No written agreement, license, or royalty fee is
 * required for any of the authorized uses.  Modifications to this
 * software may be copyrighted by their authors and need not follow
 * the licensing terms described here, provided that the new terms are
 * clearly indicated on the first page of each file where they apply.
 *
 * IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY
 * PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
 * DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION,
 * OR ANY DERIVATIVES THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND
 * NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS,
 * AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
 * MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * GOVERNMENT USE: If you are acquiring this software on behalf of the
 * U.S. government, the Government shall have only "Restricted Rights"
 * in the software and related documentation as defined in the Federal
 * Acquisition Regulations (FARs) in Clause 52.227.19 (c) (2).  If you
 * are acquiring the software on behalf of the Department of Defense,
 * the software shall be classified as "Commercial Computer Software"
 * and the Government shall have only "Restricted Rights" as defined
 * in Clause 252.227-7013 (c) (1) of DFARs.  Notwithstanding the
 * foregoing, the authors grant the U.S. Government and others acting
 * in its behalf permission to use and distribute the software in
 * accordance with the terms specified in this license.
 *
 */

#define BTK_TEXT_USE_INTERNAL_UNSUPPORTED_API
#include "config.h"
#include "btktextbtree.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "btktexttag.h"
#include "btktexttagtable.h"
#include "btktextlayout.h"
#include "btktextiterprivate.h"
#include "btkdebug.h"
#include "btktextmarkprivate.h"
#include "btkalias.h"

/*
 * Types
 */


/*
 * The structure below is used to pass information between
 * _btk_text_btree_get_tags and inc_count:
 */

typedef struct TagInfo {
  int numTags;                  /* Number of tags for which there
                                 * is currently information in
                                 * tags and counts. */
  int arraySize;                        /* Number of entries allocated for
                                         * tags and counts. */
  BtkTextTag **tags;           /* Array of tags seen so far.
                                * Malloc-ed. */
  int *counts;                  /* Toggle count (so far) for each
                                 * entry in tags.  Malloc-ed. */
} TagInfo;


/*
 * This is used to store per-view width/height info at the tree nodes.
 */

typedef struct _NodeData NodeData;

struct _NodeData {
  bpointer view_id;
  NodeData *next;

  /* Height and width of this node */
  bint height;
  signed int width : 24;

  /* boolean indicating whether the lines below this node are in need of validation.
   * However, width/height should always represent the current total width and
   * max height for lines below this node; the valid flag indicates whether the
   * width/height on the lines needs recomputing, not whether the totals
   * need recomputing.
   */
  buint valid : 8;		/* Actually a boolean */
};


/*
 * The data structure below keeps summary information about one tag as part
 * of the tag information in a node.
 */

typedef struct Summary {
  BtkTextTagInfo *info;                     /* Handle for tag. */
  int toggle_count;                     /* Number of transitions into or
                                         * out of this tag that occur in
                                         * the subtree rooted at this node. */
  struct Summary *next;         /* Next in list of all tags for same
                                 * node, or NULL if at end of list. */
} Summary;

/*
 * The data structure below defines a node in the B-tree.
 */

struct _BtkTextBTreeNode {
  BtkTextBTreeNode *parent;         /* Pointer to parent node, or NULL if
                                     * this is the root. */
  BtkTextBTreeNode *next;           /* Next in list of siblings with the
                                     * same parent node, or NULL for end
                                     * of list. */
  Summary *summary;             /* First in malloc-ed list of info
                                 * about tags in this subtree (NULL if
                                 * no tag info in the subtree). */
  int level;                            /* Level of this node in the B-tree.
                                         * 0 refers to the bottom of the tree
                                         * (children are lines, not nodes). */
  union {                               /* First in linked list of children. */
    struct _BtkTextBTreeNode *node;         /* Used if level > 0. */
    BtkTextLine *line;         /* Used if level == 0. */
  } children;
  int num_children;                     /* Number of children of this node. */
  int num_lines;                        /* Total number of lines (leaves) in
                                         * the subtree rooted here. */
  int num_chars;                        /* Number of chars below here */

  NodeData *node_data;
};


/*
 * Used to store the list of views in our btree
 */

typedef struct _BTreeView BTreeView;

struct _BTreeView {
  bpointer view_id;
  BtkTextLayout *layout;
  BTreeView *next;
  BTreeView *prev;
};

/*
 * And the tree itself
 */

struct _BtkTextBTree {
  BtkTextBTreeNode *root_node;          /* Pointer to root of B-tree. */
  BtkTextTagTable *table;
  GHashTable *mark_table;
  buint refcount;
  BtkTextMark *insert_mark;
  BtkTextMark *selection_bound_mark;
  BtkTextBuffer *buffer;
  BTreeView *views;
  GSList *tag_infos;
  bulong tag_changed_handler;

  /* Incremented when a segment with a byte size > 0
   * is added to or removed from the tree (i.e. the
   * length of a line may have changed, and lines may
   * have been added or removed). This invalidates
   * all outstanding iterators.
   */
  buint chars_changed_stamp;
  /* Incremented when any segments are added or deleted;
   * this makes outstanding iterators recalculate their
   * pointed-to segment and segment offset.
   */
  buint segments_changed_stamp;

  /* Cache the last line in the buffer */
  BtkTextLine *last_line;
  buint last_line_stamp;

  /* Cache the next-to-last line in the buffer,
   * containing the end iterator
   */
  BtkTextLine *end_iter_line;
  BtkTextLineSegment *end_iter_segment;
  int end_iter_segment_byte_index;
  int end_iter_segment_char_offset;
  buint end_iter_line_stamp;
  buint end_iter_segment_stamp;
  
  GHashTable *child_anchor_table;
};


/*
 * Upper and lower bounds on how many children a node may have:
 * rebalance when either of these limits is exceeded.  MAX_CHILDREN
 * should be twice MIN_CHILDREN and MIN_CHILDREN must be >= 2.
 */

/* Tk used MAX of 12 and MIN of 6. This makes the tree wide and
   shallow. It appears to be faster to locate a particular line number
   if the tree is narrow and deep, since it is more finely sorted.  I
   guess this may increase memory use though, and make it slower to
   walk the tree in order, or locate a particular byte index (which
   is done by walking the tree in order).

   There's basically a tradeoff here. However I'm thinking we want to
   add pixels, byte counts, and char counts to the tree nodes,
   at that point narrow and deep should speed up all operations,
   not just the line number searches.
*/

#if 1
#define MAX_CHILDREN 12
#define MIN_CHILDREN 6
#else
#define MAX_CHILDREN 6
#define MIN_CHILDREN 3
#endif

/*
 * Prototypes
 */

static BTreeView        *btk_text_btree_get_view                 (BtkTextBTree     *tree,
                                                                  bpointer          view_id);
static void              btk_text_btree_rebalance                (BtkTextBTree     *tree,
                                                                  BtkTextBTreeNode *node);
static BtkTextLine     * get_last_line                           (BtkTextBTree     *tree);
static void              post_insert_fixup                       (BtkTextBTree     *tree,
                                                                  BtkTextLine      *insert_line,
                                                                  bint              char_count_delta,
                                                                  bint              line_count_delta);
static void              btk_text_btree_node_adjust_toggle_count (BtkTextBTreeNode *node,
                                                                  BtkTextTagInfo   *info,
                                                                  bint              adjust);
static bboolean          btk_text_btree_node_has_tag             (BtkTextBTreeNode *node,
                                                                  BtkTextTag       *tag);

static void             segments_changed                (BtkTextBTree     *tree);
static void             chars_changed                   (BtkTextBTree     *tree);
static void             summary_list_destroy            (Summary          *summary);
static BtkTextLine     *btk_text_line_new               (void);
static void             btk_text_line_destroy           (BtkTextBTree     *tree,
                                                         BtkTextLine      *line);
static void             btk_text_line_set_parent        (BtkTextLine      *line,
                                                         BtkTextBTreeNode *node);
static void             btk_text_btree_node_remove_data (BtkTextBTreeNode *node,
                                                         bpointer          view_id);


static NodeData         *node_data_new          (bpointer  view_id);
static void              node_data_destroy      (NodeData *nd);
static void              node_data_list_destroy (NodeData *nd);
static NodeData         *node_data_find         (NodeData *nd,
                                                 bpointer  view_id);

static BtkTextBTreeNode     *btk_text_btree_node_new                  (void);
#if 0
static void                  btk_text_btree_node_invalidate_downward  (BtkTextBTreeNode *node);
#endif
static void                  btk_text_btree_node_invalidate_upward    (BtkTextBTreeNode *node,
                                                                       bpointer          view_id);
static NodeData *            btk_text_btree_node_check_valid          (BtkTextBTreeNode *node,
                                                                       bpointer          view_id);
static NodeData *            btk_text_btree_node_check_valid_downward (BtkTextBTreeNode *node,
                                                                       bpointer          view_id);
static void                  btk_text_btree_node_check_valid_upward   (BtkTextBTreeNode *node,
                                                                       bpointer          view_id);

static void                  btk_text_btree_node_remove_view         (BTreeView        *view,
                                                                      BtkTextBTreeNode *node,
                                                                      bpointer          view_id);
static void                  btk_text_btree_node_destroy             (BtkTextBTree     *tree,
                                                                      BtkTextBTreeNode *node);
static void                  btk_text_btree_node_free_empty          (BtkTextBTree *tree,
                                                                      BtkTextBTreeNode *node);
static NodeData         *    btk_text_btree_node_ensure_data         (BtkTextBTreeNode *node,
                                                                      bpointer          view_id);
static void                  btk_text_btree_node_remove_data         (BtkTextBTreeNode *node,
                                                                      bpointer          view_id);
static void                  btk_text_btree_node_get_size            (BtkTextBTreeNode *node,
                                                                      bpointer          view_id,
                                                                      bint             *width,
                                                                      bint             *height);
static BtkTextBTreeNode *    btk_text_btree_node_common_parent       (BtkTextBTreeNode *node1,
                                                                      BtkTextBTreeNode *node2);
static void get_tree_bounds       (BtkTextBTree     *tree,
                                   BtkTextIter      *start,
                                   BtkTextIter      *end);
static void tag_changed_cb        (BtkTextTagTable  *table,
                                   BtkTextTag       *tag,
                                   bboolean          size_changed,
                                   BtkTextBTree     *tree);
static void cleanup_line          (BtkTextLine      *line);
static void recompute_node_counts (BtkTextBTree     *tree,
                                   BtkTextBTreeNode *node);
static void inc_count             (BtkTextTag       *tag,
                                   int               inc,
                                   TagInfo          *tagInfoPtr);

static void summary_destroy       (Summary          *summary);

static void btk_text_btree_link_segment   (BtkTextLineSegment *seg,
                                           const BtkTextIter  *iter);
static void btk_text_btree_unlink_segment (BtkTextBTree       *tree,
                                           BtkTextLineSegment *seg,
                                           BtkTextLine        *line);


static BtkTextTagInfo *btk_text_btree_get_tag_info          (BtkTextBTree   *tree,
                                                             BtkTextTag     *tag);
static BtkTextTagInfo *btk_text_btree_get_existing_tag_info (BtkTextBTree   *tree,
                                                             BtkTextTag     *tag);
static void            btk_text_btree_remove_tag_info       (BtkTextBTree   *tree,
                                                             BtkTextTag     *tag);

static void redisplay_rebunnyion (BtkTextBTree      *tree,
                              const BtkTextIter *start,
                              const BtkTextIter *end,
                              bboolean           cursors_only);

/* Inline thingies */

static inline void
segments_changed (BtkTextBTree *tree)
{
  tree->segments_changed_stamp += 1;
}

static inline void
chars_changed (BtkTextBTree *tree)
{
  tree->chars_changed_stamp += 1;
}

/*
 * BTree operations
 */

BtkTextBTree*
_btk_text_btree_new (BtkTextTagTable *table,
                     BtkTextBuffer *buffer)
{
  BtkTextBTree *tree;
  BtkTextBTreeNode *root_node;
  BtkTextLine *line, *line2;

  g_return_val_if_fail (BTK_IS_TEXT_TAG_TABLE (table), NULL);
  g_return_val_if_fail (BTK_IS_TEXT_BUFFER (buffer), NULL);

  /*
   * The tree will initially have two empty lines.  The second line
   * isn't actually part of the tree's contents, but its presence
   * makes several operations easier.  The tree will have one BtkTextBTreeNode,
   * which is also the root of the tree.
   */

  /* Create the root node. */

  root_node = btk_text_btree_node_new ();

  line = btk_text_line_new ();
  line2 = btk_text_line_new ();

  root_node->parent = NULL;
  root_node->next = NULL;
  root_node->summary = NULL;
  root_node->level = 0;
  root_node->children.line = line;
  root_node->num_children = 2;
  root_node->num_lines = 2;
  root_node->num_chars = 2;

  line->parent = root_node;
  line->next = line2;

  line->segments = _btk_char_segment_new ("\n", 1);

  line2->parent = root_node;
  line2->next = NULL;
  line2->segments = _btk_char_segment_new ("\n", 1);

  /* Create the tree itself */

  tree = g_new0(BtkTextBTree, 1);
  tree->root_node = root_node;
  tree->table = table;
  tree->views = NULL;

  /* Set these to values that are unlikely to be found
   * in random memory garbage, and also avoid
   * duplicates between tree instances.
   */
  tree->chars_changed_stamp = g_random_int ();
  tree->segments_changed_stamp = g_random_int ();

  tree->last_line_stamp = tree->chars_changed_stamp - 1;
  tree->last_line = NULL;

  tree->end_iter_line_stamp = tree->chars_changed_stamp - 1;
  tree->end_iter_segment_stamp = tree->segments_changed_stamp - 1;
  tree->end_iter_line = NULL;
  tree->end_iter_segment_byte_index = 0;
  tree->end_iter_segment_char_offset = 0;
  
  g_object_ref (tree->table);

  tree->tag_changed_handler = g_signal_connect (tree->table,
						"tag-changed",
						G_CALLBACK (tag_changed_cb),
						tree);

  tree->mark_table = g_hash_table_new (g_str_hash, g_str_equal);
  tree->child_anchor_table = NULL;
  
  /* We don't ref the buffer, since the buffer owns us;
   * we'd have some circularity issues. The buffer always
   * lasts longer than the BTree
   */
  tree->buffer = buffer;

  {
    BtkTextIter start;
    BtkTextLineSegment *seg;

    _btk_text_btree_get_iter_at_line_char (tree, &start, 0, 0);


    tree->insert_mark = _btk_text_btree_set_mark (tree,
                                                 NULL,
                                                 "insert",
                                                 FALSE,
                                                 &start,
                                                 FALSE);

    seg = tree->insert_mark->segment;

    seg->body.mark.not_deleteable = TRUE;
    seg->body.mark.visible = TRUE;

    tree->selection_bound_mark = _btk_text_btree_set_mark (tree,
                                                          NULL,
                                                          "selection_bound",
                                                          FALSE,
                                                          &start,
                                                          FALSE);

    seg = tree->selection_bound_mark->segment;

    seg->body.mark.not_deleteable = TRUE;

    g_object_ref (tree->insert_mark);
    g_object_ref (tree->selection_bound_mark);
  }

  tree->refcount = 1;

  return tree;
}

void
_btk_text_btree_ref (BtkTextBTree *tree)
{
  g_return_if_fail (tree != NULL);
  g_return_if_fail (tree->refcount > 0);

  tree->refcount += 1;
}

void
_btk_text_btree_unref (BtkTextBTree *tree)
{
  g_return_if_fail (tree != NULL);
  g_return_if_fail (tree->refcount > 0);

  tree->refcount -= 1;

  if (tree->refcount == 0)
    {      
      g_signal_handler_disconnect (tree->table,
                                   tree->tag_changed_handler);

      g_object_unref (tree->table);
      tree->table = NULL;
      
      btk_text_btree_node_destroy (tree, tree->root_node);
      tree->root_node = NULL;
      
      g_assert (g_hash_table_size (tree->mark_table) == 0);
      g_hash_table_destroy (tree->mark_table);
      tree->mark_table = NULL;
      if (tree->child_anchor_table != NULL) 
	{
	  g_hash_table_destroy (tree->child_anchor_table);
	  tree->child_anchor_table = NULL;
	}

      g_object_unref (tree->insert_mark);
      tree->insert_mark = NULL;
      g_object_unref (tree->selection_bound_mark);
      tree->selection_bound_mark = NULL;

      g_free (tree);
    }
}

BtkTextBuffer*
_btk_text_btree_get_buffer (BtkTextBTree *tree)
{
  return tree->buffer;
}

buint
_btk_text_btree_get_chars_changed_stamp (BtkTextBTree *tree)
{
  return tree->chars_changed_stamp;
}

buint
_btk_text_btree_get_segments_changed_stamp (BtkTextBTree *tree)
{
  return tree->segments_changed_stamp;
}

void
_btk_text_btree_segments_changed (BtkTextBTree *tree)
{
  g_return_if_fail (tree != NULL);
  segments_changed (tree);
}

/*
 * Indexable segment mutation
 */

/*
 *  The following function is responsible for resolving the bidi direction
 *  for the lines between start and end. But it also calculates any
 *  dependent bidi direction for surrounding lines that change as a result
 *  of the bidi direction decisions within the range. The function is
 *  trying to do as little propagation as is needed.
 */
static void
btk_text_btree_resolve_bidi (BtkTextIter *start,
			     BtkTextIter *end)
{
  BtkTextBTree *tree = _btk_text_iter_get_btree (start);
  BtkTextLine *start_line, *end_line, *start_line_prev, *end_line_next, *line;
  BangoDirection last_strong, dir_above_propagated, dir_below_propagated;

  /* Resolve the strong bidi direction for all lines between
   * start and end.
  */
  start_line = _btk_text_iter_get_text_line (start);
  start_line_prev = _btk_text_line_previous (start_line);
  end_line = _btk_text_iter_get_text_line (end);
  end_line_next = _btk_text_line_next (end_line);
  
  line = start_line;
  while (line && line != end_line_next)
    {
      /* Loop through the segments and search for a strong character
       */
      BtkTextLineSegment *seg = line->segments;
      line->dir_strong = BANGO_DIRECTION_NEUTRAL;
      
      while (seg)
        {
          if (seg->type == &btk_text_char_type && seg->byte_count > 0)
            {
	      BangoDirection bango_dir;

              bango_dir = bango_find_base_dir (seg->body.chars,
					       seg->byte_count);
	      
              if (bango_dir != BANGO_DIRECTION_NEUTRAL)
                {
                  line->dir_strong = bango_dir;
                  break;
                }
            }
          seg = seg->next;
        }

      line = _btk_text_line_next (line);
    }

  /* Sweep forward */

  /* The variable dir_above_propagated contains the forward propagated
   * direction before start. It is neutral if start is in the beginning
   * of the buffer.
   */
  dir_above_propagated = BANGO_DIRECTION_NEUTRAL;
  if (start_line_prev)
    dir_above_propagated = start_line_prev->dir_propagated_forward;

  /* Loop forward and propagate the direction of each paragraph 
   * to all neutral lines.
   */
  line = start_line;
  last_strong = dir_above_propagated;
  while (line != end_line_next)
    {
      if (line->dir_strong != BANGO_DIRECTION_NEUTRAL)
        last_strong = line->dir_strong;
      
      line->dir_propagated_forward = last_strong;
      
      line = _btk_text_line_next (line);
    }

  /* Continue propagating as long as the previous resolved forward
   * is different from last_strong.
   */
  {
    BtkTextIter end_propagate;
    
    while (line &&
	   line->dir_strong == BANGO_DIRECTION_NEUTRAL &&
	   line->dir_propagated_forward != last_strong)
      {
        BtkTextLine *prev = line;
        line->dir_propagated_forward = last_strong;
        
        line = _btk_text_line_next(line);
        if (!line)
          {
            line = prev;
            break;
          }
      }

    /* The last line to invalidate is the last line before the
     * line with the strong character. Or in case of the end of the
     * buffer, the last line of the buffer. (There seems to be an
     * extra "virtual" last line in the buffer that must not be used
     * calling _btk_text_btree_get_iter_at_line (causes crash). Thus the
     * _btk_text_line_previous is ok in that case as well.)
     */
    line = _btk_text_line_previous (line);
    _btk_text_btree_get_iter_at_line (tree, &end_propagate, line, 0);
    _btk_text_btree_invalidate_rebunnyion (tree, end, &end_propagate, FALSE);
  }
  
  /* Sweep backward */

  /* The variable dir_below_propagated contains the backward propagated
   * direction after end. It is neutral if end is at the end of
   * the buffer.
  */
  dir_below_propagated = BANGO_DIRECTION_NEUTRAL;
  if (end_line_next)
    dir_below_propagated = end_line_next->dir_propagated_back;

  /* Loop backward and propagate the direction of each paragraph 
   * to all neutral lines.
   */
  line = end_line;
  last_strong = dir_below_propagated;
  while (line != start_line_prev)
    {
      if (line->dir_strong != BANGO_DIRECTION_NEUTRAL)
        last_strong = line->dir_strong;

      line->dir_propagated_back = last_strong;

      line = _btk_text_line_previous (line);
    }

  /* Continue propagating as long as the resolved backward dir
   * is different from last_strong.
   */
  {
    BtkTextIter start_propagate;

    while (line &&
	   line->dir_strong == BANGO_DIRECTION_NEUTRAL &&
	   line->dir_propagated_back != last_strong)
      {
        BtkTextLine *prev = line;
        line->dir_propagated_back = last_strong;

        line = _btk_text_line_previous (line);
        if (!line)
          {
            line = prev;
            break;
          }
      }

    /* We only need to invalidate for backwards propagation if the
     * line we ended up on didn't get a direction from forwards
     * propagation.
     */
    if (line && line->dir_propagated_forward == BANGO_DIRECTION_NEUTRAL)
      {
        _btk_text_btree_get_iter_at_line (tree, &start_propagate, line, 0);
        _btk_text_btree_invalidate_rebunnyion (tree, &start_propagate, start, FALSE);
      }
  }
}

void
_btk_text_btree_delete (BtkTextIter *start,
                        BtkTextIter *end)
{
  BtkTextLineSegment *prev_seg;             /* The segment just before the start
                                             * of the deletion range. */
  BtkTextLineSegment *last_seg;             /* The segment just after the end
                                             * of the deletion range. */
  BtkTextLineSegment *seg, *next, *next2;
  BtkTextLine *curline;
  BtkTextBTreeNode *curnode, *node;
  BtkTextBTree *tree;
  BtkTextLine *start_line;
  BtkTextLine *end_line;
  BtkTextLine *line;
  BtkTextLine *deleted_lines = NULL;        /* List of lines we've deleted */
  bint start_byte_offset;

  g_return_if_fail (start != NULL);
  g_return_if_fail (end != NULL);
  g_return_if_fail (_btk_text_iter_get_btree (start) ==
                    _btk_text_iter_get_btree (end));

  btk_text_iter_order (start, end);

  tree = _btk_text_iter_get_btree (start);
 
  if (btk_debug_flags & BTK_DEBUG_TEXT)
    _btk_text_btree_check (tree);
  
  /* Broadcast the need for redisplay before we break the iterators */
  DV (g_print ("invalidating due to deleting some text (%s)\n", B_STRLOC));
  _btk_text_btree_invalidate_rebunnyion (tree, start, end, FALSE);

  /* Save the byte offset so we can reset the iterators */
  start_byte_offset = btk_text_iter_get_line_index (start);

  start_line = _btk_text_iter_get_text_line (start);
  end_line = _btk_text_iter_get_text_line (end);

  /*
   * Split the start and end segments, so we have a place
   * to insert our new text.
   *
   * Tricky point:  split at end first;  otherwise the split
   * at end may invalidate seg and/or prev_seg. This allows
   * us to avoid invalidating segments for start.
   */

  last_seg = btk_text_line_segment_split (end);
  if (last_seg != NULL)
    last_seg = last_seg->next;
  else
    last_seg = end_line->segments;

  prev_seg = btk_text_line_segment_split (start);
  if (prev_seg != NULL)
    {
      seg = prev_seg->next;
      prev_seg->next = last_seg;
    }
  else
    {
      seg = start_line->segments;
      start_line->segments = last_seg;
    }

  /* notify iterators that their segments need recomputation,
     just for robustness. */
  segments_changed (tree);

  /*
   * Delete all of the segments between prev_seg and last_seg.
   */

  curline = start_line;
  curnode = curline->parent;
  while (seg != last_seg)
    {
      bint char_count = 0;

      if (seg == NULL)
        {
          BtkTextLine *nextline;

          /*
           * We just ran off the end of a line.  First find the
           * next line, then go back to the old line and delete it
           * (unless it's the starting line for the range).
           */

          nextline = _btk_text_line_next (curline);
          if (curline != start_line)
            {
              if (curnode == start_line->parent)
                start_line->next = curline->next;
              else
                curnode->children.line = curline->next;

              for (node = curnode; node != NULL;
                   node = node->parent)
                {
                  /* Don't update node->num_chars, because
                   * that was done when we deleted the segments.
                   */
                  node->num_lines -= 1;
                }

              curnode->num_children -= 1;
              curline->next = deleted_lines;
              deleted_lines = curline;
            }

          curline = nextline;
          seg = curline->segments;

          /*
           * If the BtkTextBTreeNode is empty then delete it and its parents,
           * recursively upwards until a non-empty BtkTextBTreeNode is found.
           */

          while (curnode->num_children == 0)
            {
              BtkTextBTreeNode *parent;

              parent = curnode->parent;
              if (parent->children.node == curnode)
                {
                  parent->children.node = curnode->next;
                }
              else
                {
                  BtkTextBTreeNode *prevnode = parent->children.node;
                  while (prevnode->next != curnode)
                    {
                      prevnode = prevnode->next;
                    }
                  prevnode->next = curnode->next;
                }
              parent->num_children--;
              btk_text_btree_node_free_empty (tree, curnode);
              curnode = parent;
            }
          curnode = curline->parent;
          continue;
        }

      next = seg->next;
      char_count = seg->char_count;

      if ((*seg->type->deleteFunc)(seg, curline, FALSE) != 0)
        {
          /*
           * This segment refuses to die.  Move it to prev_seg and
           * advance prev_seg if the segment has left gravity.
           */

          if (prev_seg == NULL)
            {
              seg->next = start_line->segments;
              start_line->segments = seg;
            }
          else if (prev_seg->next &&
		   prev_seg->next != last_seg &&
		   seg->type == &btk_text_toggle_off_type &&
		   prev_seg->next->type == &btk_text_toggle_on_type &&
		   seg->body.toggle.info == prev_seg->next->body.toggle.info)
	    {
	      /* Try to match an off toggle with the matching on toggle
	       * if it immediately follows. This is a common case, and
	       * handling it here prevents quadratic blowup in
	       * cleanup_line() below. See bug 317125.
	       */
	      next2 = prev_seg->next->next;
	      g_free ((char *)prev_seg->next);
	      prev_seg->next = next2;
	      g_free ((char *)seg);
	      seg = NULL;
	    }
	  else
	    {
              seg->next = prev_seg->next;
              prev_seg->next = seg;
            }

          if (seg && seg->type->leftGravity)
            {
              prev_seg = seg;
            }
        }
      else
        {
          /* Segment is gone. Decrement the char count of the node and
             all its parents. */
          for (node = curnode; node != NULL;
               node = node->parent)
            {
              node->num_chars -= char_count;
            }
        }

      seg = next;
    }

  /*
   * If the beginning and end of the deletion range are in different
   * lines, join the two lines together and discard the ending line.
   */

  if (start_line != end_line)
    {
      BTreeView *view;
      BtkTextBTreeNode *ancestor_node;
      BtkTextLine *prevline;
      int chars_moved;      

      /* last_seg was appended to start_line up at the top of this function */
      chars_moved = 0;
      for (seg = last_seg; seg != NULL;
           seg = seg->next)
        {
          chars_moved += seg->char_count;
          if (seg->type->lineChangeFunc != NULL)
            {
              (*seg->type->lineChangeFunc)(seg, end_line);
            }
        }

      for (node = start_line->parent; node != NULL;
           node = node->parent)
        {
          node->num_chars += chars_moved;
        }
      
      curnode = end_line->parent;
      for (node = curnode; node != NULL;
           node = node->parent)
        {
          node->num_chars -= chars_moved;
          node->num_lines--;
        }
      curnode->num_children--;
      prevline = curnode->children.line;
      if (prevline == end_line)
        {
          curnode->children.line = end_line->next;
        }
      else
        {
          while (prevline->next != end_line)
            {
              prevline = prevline->next;
            }
          prevline->next = end_line->next;
        }
      end_line->next = deleted_lines;
      deleted_lines = end_line;

      /* We now fix up the per-view aggregates. We add all the height and
       * width for the deleted lines to the start line, so that when revalidation
       * occurs, the correct change in size is seen.
       */
      ancestor_node = btk_text_btree_node_common_parent (curnode, start_line->parent);
      view = tree->views;
      while (view)
        {
          BtkTextLineData *ld;

          bint deleted_width = 0;
          bint deleted_height = 0;

          line = deleted_lines;
          while (line)
            {
              BtkTextLine *next_line = line->next;
              ld = _btk_text_line_get_data (line, view->view_id);

              if (ld)
                {
                  deleted_width = MAX (deleted_width, ld->width);
                  deleted_height += ld->height;
                }

              line = next_line;
            }

          if (deleted_width > 0 || deleted_height > 0)
            {
              ld = _btk_text_line_get_data (start_line, view->view_id);
              
              if (ld == NULL)
                {
                  /* This means that start_line has never been validated.
                   * We don't really want to do the validation here but
                   * we do need to store our temporary sizes. So we
                   * create the line data and assume a line w/h of 0.
                   */
                  ld = _btk_text_line_data_new (view->layout, start_line);
                  _btk_text_line_add_data (start_line, ld);
                  ld->width = 0;
                  ld->height = 0;
                  ld->valid = FALSE;
                }
              
              ld->width = MAX (deleted_width, ld->width);
              ld->height += deleted_height;
              ld->valid = FALSE;
            }

          btk_text_btree_node_check_valid_downward (ancestor_node, view->view_id);
          if (ancestor_node->parent)
            btk_text_btree_node_check_valid_upward (ancestor_node->parent, view->view_id);

          view = view->next;
        }

      line = deleted_lines;
      while (line)
        {
          BtkTextLine *next_line = line->next;

          btk_text_line_destroy (tree, line);

          line = next_line;
        }

      /* avoid dangling pointer */
      deleted_lines = NULL;
      
      btk_text_btree_rebalance (tree, curnode);
    }

  /*
   * Cleanup the segments in the new line.
   */

  cleanup_line (start_line);

  /*
   * Lastly, rebalance the first BtkTextBTreeNode of the range.
   */

  btk_text_btree_rebalance (tree, start_line->parent);

  /* Notify outstanding iterators that they
     are now hosed */
  chars_changed (tree);
  segments_changed (tree);

  if (btk_debug_flags & BTK_DEBUG_TEXT)
    _btk_text_btree_check (tree);

  /* Re-initialize our iterators */
  _btk_text_btree_get_iter_at_line (tree, start, start_line, start_byte_offset);
  *end = *start;

  btk_text_btree_resolve_bidi (start, end);
}

void
_btk_text_btree_insert (BtkTextIter *iter,
                        const bchar *text,
                        bint         len)
{
  BtkTextLineSegment *prev_seg;     /* The segment just before the first
                                     * new segment (NULL means new segment
                                     * is at beginning of line). */
  BtkTextLineSegment *cur_seg;              /* Current segment;  new characters
                                             * are inserted just after this one.
                                             * NULL means insert at beginning of
                                             * line. */
  BtkTextLine *line;           /* Current line (new segments are
                                * added to this line). */
  BtkTextLineSegment *seg;
  BtkTextLine *newline;
  int chunk_len;                        /* # characters in current chunk. */
  bint sol;                           /* start of line */
  bint eol;                           /* Pointer to character just after last
                                       * one in current chunk.
                                       */
  bint delim;                          /* index of paragraph delimiter */
  int line_count_delta;                /* Counts change to total number of
                                        * lines in file.
                                        */

  int char_count_delta;                /* change to number of chars */
  BtkTextBTree *tree;
  bint start_byte_index;
  BtkTextLine *start_line;

  g_return_if_fail (text != NULL);
  g_return_if_fail (iter != NULL);

  if (len < 0)
    len = strlen (text);

  /* extract iterator info */
  tree = _btk_text_iter_get_btree (iter);
  line = _btk_text_iter_get_text_line (iter);
  
  start_line = line;
  start_byte_index = btk_text_iter_get_line_index (iter);

  /* Get our insertion segment split. Note this assumes line allows
   * char insertions, which isn't true of the "last" line. But iter
   * should not be on that line, as we assert here.
   */
  g_assert (!_btk_text_line_is_last (line, tree));
  prev_seg = btk_text_line_segment_split (iter);
  cur_seg = prev_seg;

  /* Invalidate all iterators */
  chars_changed (tree);
  segments_changed (tree);
  
  /*
   * Chop the text up into lines and create a new segment for
   * each line, plus a new line for the leftovers from the
   * previous line.
   */

  eol = 0;
  sol = 0;
  line_count_delta = 0;
  char_count_delta = 0;
  while (eol < len)
    {
      sol = eol;
      
      bango_find_paragraph_boundary (text + sol,
                                     len - sol,
                                     &delim,
                                     &eol);      

      /* make these relative to the start of the text */
      delim += sol;
      eol += sol;

      g_assert (eol >= sol);
      g_assert (delim >= sol);
      g_assert (eol >= delim);
      g_assert (sol >= 0);
      g_assert (eol <= len);
      
      chunk_len = eol - sol;

      g_assert (g_utf8_validate (&text[sol], chunk_len, NULL));
      seg = _btk_char_segment_new (&text[sol], chunk_len);

      char_count_delta += seg->char_count;

      if (cur_seg == NULL)
        {
          seg->next = line->segments;
          line->segments = seg;
        }
      else
        {
          seg->next = cur_seg->next;
          cur_seg->next = seg;
        }

      if (delim == eol)
        {
          /* chunk didn't end with a paragraph separator */
          g_assert (eol == len);
          break;
        }

      /*
       * The chunk ended with a newline, so create a new BtkTextLine
       * and move the remainder of the old line to it.
       */

      newline = btk_text_line_new ();
      btk_text_line_set_parent (newline, line->parent);
      newline->next = line->next;
      line->next = newline;
      newline->segments = seg->next;
      seg->next = NULL;
      line = newline;
      cur_seg = NULL;
      line_count_delta++;
    }

  /*
   * Cleanup the starting line for the insertion, plus the ending
   * line if it's different.
   */

  cleanup_line (start_line);
  if (line != start_line)
    {
      cleanup_line (line);
    }

  post_insert_fixup (tree, line, line_count_delta, char_count_delta);

  /* Invalidate our rebunnyion, and reset the iterator the user
     passed in to point to the end of the inserted text. */
  {
    BtkTextIter start;
    BtkTextIter end;


    _btk_text_btree_get_iter_at_line (tree,
                                      &start,
                                      start_line,
                                      start_byte_index);
    end = start;

    /* We could almost certainly be more efficient here
       by saving the information from the insertion loop
       above. FIXME */
    btk_text_iter_forward_chars (&end, char_count_delta);

    DV (g_print ("invalidating due to inserting some text (%s)\n", B_STRLOC));
    _btk_text_btree_invalidate_rebunnyion (tree, &start, &end, FALSE);


    /* Convenience for the user */
    *iter = end;

    btk_text_btree_resolve_bidi (&start, &end);
  }
}

static void
insert_pixbuf_or_widget_segment (BtkTextIter        *iter,
                                 BtkTextLineSegment *seg)

{
  BtkTextIter start;
  BtkTextLineSegment *prevPtr;
  BtkTextLine *line;
  BtkTextBTree *tree;
  bint start_byte_offset;

  line = _btk_text_iter_get_text_line (iter);
  tree = _btk_text_iter_get_btree (iter);
  start_byte_offset = btk_text_iter_get_line_index (iter);

  prevPtr = btk_text_line_segment_split (iter);
  if (prevPtr == NULL)
    {
      seg->next = line->segments;
      line->segments = seg;
    }
  else
    {
      seg->next = prevPtr->next;
      prevPtr->next = seg;
    }

  post_insert_fixup (tree, line, 0, seg->char_count);

  chars_changed (tree);
  segments_changed (tree);

  /* reset *iter for the user, and invalidate tree nodes */

  _btk_text_btree_get_iter_at_line (tree, &start, line, start_byte_offset);

  *iter = start;
  btk_text_iter_forward_char (iter); /* skip forward past the segment */

  DV (g_print ("invalidating due to inserting pixbuf/widget (%s)\n", B_STRLOC));
  _btk_text_btree_invalidate_rebunnyion (tree, &start, iter, FALSE);
}
     
void
_btk_text_btree_insert_pixbuf (BtkTextIter *iter,
                              BdkPixbuf   *pixbuf)
{
  BtkTextLineSegment *seg;
  
  seg = _btk_pixbuf_segment_new (pixbuf);

  insert_pixbuf_or_widget_segment (iter, seg);
}

void
_btk_text_btree_insert_child_anchor (BtkTextIter        *iter,
                                     BtkTextChildAnchor *anchor)
{
  BtkTextLineSegment *seg;
  BtkTextBTree *tree;

  if (anchor->segment != NULL)
    {
      g_warning (B_STRLOC": Same child anchor can't be inserted twice");
      return;
    }
  
  seg = _btk_widget_segment_new (anchor);

  tree = seg->body.child.tree = _btk_text_iter_get_btree (iter);
  seg->body.child.line = _btk_text_iter_get_text_line (iter);
  
  insert_pixbuf_or_widget_segment (iter, seg);

  if (tree->child_anchor_table == NULL)
    tree->child_anchor_table = g_hash_table_new (NULL, NULL);

  g_hash_table_insert (tree->child_anchor_table,
                       seg->body.child.obj,
                       seg->body.child.obj);
}

void
_btk_text_btree_unregister_child_anchor (BtkTextChildAnchor *anchor)
{
  BtkTextLineSegment *seg;

  seg = anchor->segment;
  
  g_hash_table_remove (seg->body.child.tree->child_anchor_table,
                       anchor);
}

/*
 * View stuff
 */

static BtkTextLine*
find_line_by_y (BtkTextBTree *tree, BTreeView *view,
                BtkTextBTreeNode *node, bint y, bint *line_top,
                BtkTextLine *last_line)
{
  bint current_y = 0;

  if (btk_debug_flags & BTK_DEBUG_TEXT)
    _btk_text_btree_check (tree);

  if (node->level == 0)
    {
      BtkTextLine *line;

      line = node->children.line;

      while (line != NULL && line != last_line)
        {
          BtkTextLineData *ld;

          ld = _btk_text_line_get_data (line, view->view_id);

          if (ld)
            {
              if (y < (current_y + (ld ? ld->height : 0)))
                return line;

              current_y += ld->height;
              *line_top += ld->height;
            }

          line = line->next;
        }
      return NULL;
    }
  else
    {
      BtkTextBTreeNode *child;

      child = node->children.node;

      while (child != NULL)
        {
          bint width;
          bint height;

          btk_text_btree_node_get_size (child, view->view_id,
                                        &width, &height);

          if (y < (current_y + height))
            return find_line_by_y (tree, view, child,
                                   y - current_y, line_top,
                                   last_line);

          current_y += height;
          *line_top += height;

          child = child->next;
        }

      return NULL;
    }
}

BtkTextLine *
_btk_text_btree_find_line_by_y (BtkTextBTree *tree,
                                bpointer      view_id,
                                bint          ypixel,
                                bint         *line_top_out)
{
  BtkTextLine *line;
  BTreeView *view;
  BtkTextLine *last_line;
  bint line_top = 0;

  view = btk_text_btree_get_view (tree, view_id);
  g_return_val_if_fail (view != NULL, NULL);

  last_line = get_last_line (tree);

  line = find_line_by_y (tree, view, tree->root_node, ypixel, &line_top,
                         last_line);

  if (line_top_out)
    *line_top_out = line_top;

  return line;
}

static bint
find_line_top_in_line_list (BtkTextBTree *tree,
                            BTreeView *view,
                            BtkTextLine *line,
                            BtkTextLine *target_line,
                            bint y)
{
  while (line != NULL)
    {
      BtkTextLineData *ld;

      if (line == target_line)
        return y;

      ld = _btk_text_line_get_data (line, view->view_id);
      if (ld)
        y += ld->height;

      line = line->next;
    }

  g_assert_not_reached (); /* If we get here, our
                              target line didn't exist
                              under its parent node */
  return 0;
}

bint
_btk_text_btree_find_line_top (BtkTextBTree *tree,
                              BtkTextLine *target_line,
                              bpointer view_id)
{
  bint y = 0;
  BTreeView *view;
  GSList *nodes;
  GSList *iter;
  BtkTextBTreeNode *node;

  view = btk_text_btree_get_view (tree, view_id);

  g_return_val_if_fail (view != NULL, 0);

  nodes = NULL;
  node = target_line->parent;
  while (node != NULL)
    {
      nodes = b_slist_prepend (nodes, node);
      node = node->parent;
    }

  iter = nodes;
  while (iter != NULL)
    {
      node = iter->data;

      if (node->level == 0)
        {
          b_slist_free (nodes);
          return find_line_top_in_line_list (tree, view,
                                             node->children.line,
                                             target_line, y);
        }
      else
        {
          BtkTextBTreeNode *child;
          BtkTextBTreeNode *target_node;

          g_assert (iter->next != NULL); /* not at level 0 */
          target_node = iter->next->data;

          child = node->children.node;

          while (child != NULL)
            {
              bint width;
              bint height;

              if (child == target_node)
                break;
              else
                {
                  btk_text_btree_node_get_size (child, view->view_id,
                                                &width, &height);
                  y += height;
                }
              child = child->next;
            }
          g_assert (child != NULL); /* should have broken out before we
                                       ran out of nodes */
        }

      iter = b_slist_next (iter);
    }

  g_assert_not_reached (); /* we return when we find the target line */
  return 0;
}

void
_btk_text_btree_add_view (BtkTextBTree *tree,
                         BtkTextLayout *layout)
{
  BTreeView *view;
  BtkTextLine *last_line;
  BtkTextLineData *line_data;

  g_return_if_fail (tree != NULL);
  
  view = g_new (BTreeView, 1);

  view->view_id = layout;
  view->layout = layout;

  view->next = tree->views;
  view->prev = NULL;

  if (tree->views)
    {
      g_assert (tree->views->prev == NULL);
      tree->views->prev = view;
    }
  
  tree->views = view;

  /* The last line in the buffer has identity values for the per-view
   * data so that we can avoid special case checks for it in a large
   * number of loops
   */
  last_line = get_last_line (tree);

  line_data = g_new (BtkTextLineData, 1);
  line_data->view_id = layout;
  line_data->next = NULL;
  line_data->width = 0;
  line_data->height = 0;
  line_data->valid = TRUE;

  _btk_text_line_add_data (last_line, line_data);
}

void
_btk_text_btree_remove_view (BtkTextBTree *tree,
                             bpointer      view_id)
{
  BTreeView *view;
  BtkTextLine *last_line;
  BtkTextLineData *line_data;

  g_return_if_fail (tree != NULL);
  
  view = tree->views;

  while (view != NULL)
    {
      if (view->view_id == view_id)
        break;

      view = view->next;
    }

  g_return_if_fail (view != NULL);

  if (view->next)
    view->next->prev = view->prev;

  if (view->prev)
    view->prev->next = view->next;

  if (view == tree->views)
    tree->views = view->next;

  /* Remove the line data for the last line which we added ourselves.
   * (Do this first, so that we don't try to call the view's line data destructor on it.)
   */
  last_line = get_last_line (tree);
  line_data = _btk_text_line_remove_data (last_line, view_id);
  g_free (line_data);

  btk_text_btree_node_remove_view (view, tree->root_node, view_id);

  view->layout = (bpointer) 0xdeadbeef;
  view->view_id = (bpointer) 0xdeadbeef;
  
  g_free (view);
}

void
_btk_text_btree_invalidate_rebunnyion (BtkTextBTree      *tree,
                                   const BtkTextIter *start,
                                   const BtkTextIter *end,
                                   bboolean           cursors_only)
{
  BTreeView *view;

  view = tree->views;

  while (view != NULL)
    {
      if (cursors_only)
	btk_text_layout_invalidate_cursors (view->layout, start, end);
      else
	btk_text_layout_invalidate (view->layout, start, end);

      view = view->next;
    }
}

void
_btk_text_btree_get_view_size (BtkTextBTree *tree,
                              bpointer view_id,
                              bint *width,
                              bint *height)
{
  g_return_if_fail (tree != NULL);
  g_return_if_fail (view_id != NULL);

  btk_text_btree_node_get_size (tree->root_node, view_id,
                                width, height);
}

/*
 * Tag
 */

typedef struct {
  BtkTextIter *iters;
  buint count;
  buint alloced;
} IterStack;

static IterStack*
iter_stack_new (void)
{
  IterStack *stack;
  stack = g_slice_new (IterStack);
  stack->iters = NULL;
  stack->count = 0;
  stack->alloced = 0;
  return stack;
}

static void
iter_stack_push (IterStack         *stack, 
		 const BtkTextIter *iter)
{
  stack->count += 1;
  if (stack->count > stack->alloced)
    {
      stack->alloced = stack->count*2;
      stack->iters = g_realloc (stack->iters,
                                stack->alloced * sizeof (BtkTextIter));
    }
  stack->iters[stack->count-1] = *iter;
}

static bboolean
iter_stack_pop (IterStack   *stack, 
		BtkTextIter *iter)
{
  if (stack->count == 0)
    return FALSE;
  else
    {
      stack->count -= 1;
      *iter = stack->iters[stack->count];
      return TRUE;
    }
}

static void
iter_stack_free (IterStack *stack)
{
  g_free (stack->iters);
  g_slice_free (IterStack, stack);
}

static void
iter_stack_invert (IterStack *stack)
{
  if (stack->count > 0)
    {
      buint i = 0;
      buint j = stack->count - 1;
      while (i < j)
        {
          BtkTextIter tmp;

          tmp = stack->iters[i];
          stack->iters[i] = stack->iters[j];
          stack->iters[j] = tmp;

          ++i;
          --j;
        }
    }
}

static void
queue_tag_redisplay (BtkTextBTree      *tree,
                     BtkTextTag        *tag,
                     const BtkTextIter *start,
                     const BtkTextIter *end)
{
  if (_btk_text_tag_affects_size (tag))
    {
      DV (g_print ("invalidating due to size-affecting tag (%s)\n", B_STRLOC));
      _btk_text_btree_invalidate_rebunnyion (tree, start, end, FALSE);
    }
  else if (_btk_text_tag_affects_nonsize_appearance (tag))
    {
      /* We only need to queue a redraw, not a relayout */
      redisplay_rebunnyion (tree, start, end, FALSE);
    }

  /* We don't need to do anything if the tag doesn't affect display */
}

void
_btk_text_btree_tag (const BtkTextIter *start_orig,
                     const BtkTextIter *end_orig,
                     BtkTextTag        *tag,
                     bboolean           add)
{
  BtkTextLineSegment *seg, *prev;
  BtkTextLine *cleanupline;
  bboolean toggled_on;
  BtkTextLine *start_line;
  BtkTextLine *end_line;
  BtkTextIter iter;
  BtkTextIter start, end;
  BtkTextBTree *tree;
  IterStack *stack;
  BtkTextTagInfo *info;

  g_return_if_fail (start_orig != NULL);
  g_return_if_fail (end_orig != NULL);
  g_return_if_fail (BTK_IS_TEXT_TAG (tag));
  g_return_if_fail (_btk_text_iter_get_btree (start_orig) ==
                    _btk_text_iter_get_btree (end_orig));
  g_return_if_fail (tag->table == _btk_text_iter_get_btree (start_orig)->table);
  
#if 0
  printf ("%s tag %s from %d to %d\n",
          add ? "Adding" : "Removing",
          tag->name,
          btk_text_buffer_get_offset (start_orig),
          btk_text_buffer_get_offset (end_orig));
#endif

  if (btk_text_iter_equal (start_orig, end_orig))
    return;

  start = *start_orig;
  end = *end_orig;

  btk_text_iter_order (&start, &end);

  tree = _btk_text_iter_get_btree (&start);

  queue_tag_redisplay (tree, tag, &start, &end);

  info = btk_text_btree_get_tag_info (tree, tag);

  start_line = _btk_text_iter_get_text_line (&start);
  end_line = _btk_text_iter_get_text_line (&end);

  /* Find all tag toggles in the rebunnyion; we are going to delete them.
     We need to find them in advance, because
     forward_find_tag_toggle () won't work once we start playing around
     with the tree. */
  stack = iter_stack_new ();
  iter = start;

  /* forward_to_tag_toggle() skips a toggle at the start iterator,
   * which is deliberate - we don't want to delete a toggle at the
   * start.
   */
  while (btk_text_iter_forward_to_tag_toggle (&iter, tag))
    {
      if (btk_text_iter_compare (&iter, &end) >= 0)
        break;
      else
        iter_stack_push (stack, &iter);
    }

  /* We need to traverse the toggles in order. */
  iter_stack_invert (stack);

  /*
   * See whether the tag is present at the start of the range.  If
   * the state doesn't already match what we want then add a toggle
   * there.
   */

  toggled_on = btk_text_iter_has_tag (&start, tag);
  if ( (add && !toggled_on) ||
       (!add && toggled_on) )
    {
      /* This could create a second toggle at the start position;
         cleanup_line () will remove it if so. */
      seg = _btk_toggle_segment_new (info, add);

      prev = btk_text_line_segment_split (&start);
      if (prev == NULL)
        {
          seg->next = start_line->segments;
          start_line->segments = seg;
        }
      else
        {
          seg->next = prev->next;
          prev->next = seg;
        }

      /* cleanup_line adds the new toggle to the node counts. */
#if 0
      printf ("added toggle at start\n");
#endif
      /* we should probably call segments_changed, but in theory
         any still-cached segments in the iters we are about to
         use are still valid, since they're in front
         of this spot. */
    }

  /*
   *
   * Scan the range of characters and delete any internal tag
   * transitions.  Keep track of what the old state was at the end
   * of the range, and add a toggle there if it's needed.
   *
   */

  cleanupline = start_line;
  while (iter_stack_pop (stack, &iter))
    {
      BtkTextLineSegment *indexable_seg;
      BtkTextLine *line;

      line = _btk_text_iter_get_text_line (&iter);
      seg = _btk_text_iter_get_any_segment (&iter);
      indexable_seg = _btk_text_iter_get_indexable_segment (&iter);

      g_assert (seg != NULL);
      g_assert (indexable_seg != NULL);
      g_assert (seg != indexable_seg);

      prev = line->segments;

      /* Find the segment that actually toggles this tag. */
      while (seg != indexable_seg)
        {
          g_assert (seg != NULL);
          g_assert (indexable_seg != NULL);
          g_assert (seg != indexable_seg);
          
          if ( (seg->type == &btk_text_toggle_on_type ||
                seg->type == &btk_text_toggle_off_type) &&
               (seg->body.toggle.info == info) )
            break;
          else
            seg = seg->next;
        }

      g_assert (seg != NULL);
      g_assert (indexable_seg != NULL);

      g_assert (seg != indexable_seg); /* If this happens, then
                                          forward_to_tag_toggle was
                                          full of shit. */
      g_assert (seg->body.toggle.info->tag == tag);

      /* If this happens, when previously tagging we didn't merge
         overlapping tags. */
      g_assert ( (toggled_on && seg->type == &btk_text_toggle_off_type) ||
                 (!toggled_on && seg->type == &btk_text_toggle_on_type) );

      toggled_on = !toggled_on;

#if 0
      printf ("deleting %s toggle\n",
              seg->type == &btk_text_toggle_on_type ? "on" : "off");
#endif
      /* Remove toggle segment from the list. */
      if (prev == seg)
        {
          line->segments = seg->next;
        }
      else
        {
          while (prev->next != seg)
            {
              prev = prev->next;
            }
          prev->next = seg->next;
        }

      /* Inform iterators we've hosed them. This actually reflects a
         bit of inefficiency; if you have the same tag toggled on and
         off a lot in a single line, we keep having the rescan from
         the front of the line. Of course we have to do that to get
         "prev" anyway, but here we are doing it an additional
         time. FIXME */
      segments_changed (tree);

      /* Update node counts */
      if (seg->body.toggle.inNodeCounts)
        {
          _btk_change_node_toggle_count (line->parent,
                                         info, -1);
          seg->body.toggle.inNodeCounts = FALSE;
        }

      g_free (seg);

      /* We only clean up lines when we're done with them, saves some
         gratuitous line-segment-traversals */

      if (cleanupline != line)
        {
          cleanup_line (cleanupline);
          cleanupline = line;
        }
    }

  iter_stack_free (stack);

  /* toggled_on now reflects the toggle state _just before_ the
     end iterator. The end iterator could already have a toggle
     on or a toggle off. */
  if ( (add && !toggled_on) ||
       (!add && toggled_on) )
    {
      /* This could create a second toggle at the start position;
         cleanup_line () will remove it if so. */

      seg = _btk_toggle_segment_new (info, !add);

      prev = btk_text_line_segment_split (&end);
      if (prev == NULL)
        {
          seg->next = end_line->segments;
          end_line->segments = seg;
        }
      else
        {
          seg->next = prev->next;
          prev->next = seg;
        }
      /* cleanup_line adds the new toggle to the node counts. */
      g_assert (seg->body.toggle.inNodeCounts == FALSE);
#if 0
      printf ("added toggle at end\n");
#endif
    }

  /*
   * Cleanup cleanupline and the last line of the range, if
   * these are different.
   */

  cleanup_line (cleanupline);
  if (cleanupline != end_line)
    {
      cleanup_line (end_line);
    }

  segments_changed (tree);

  queue_tag_redisplay (tree, tag, &start, &end);

  if (btk_debug_flags & BTK_DEBUG_TEXT)
    _btk_text_btree_check (tree);
}


/*
 * "Getters"
 */

static BtkTextLine*
get_line_internal (BtkTextBTree *tree,
                   bint          line_number,
                   bint         *real_line_number,
                   bboolean      include_last)
{
  BtkTextBTreeNode *node;
  BtkTextLine *line;
  int lines_left;
  int line_count;

  line_count = _btk_text_btree_line_count (tree);
  if (!include_last)
    line_count -= 1;
  
  if (line_number < 0)
    {
      line_number = line_count;
    }
  else if (line_number > line_count)
    {
      line_number = line_count;
    }

  if (real_line_number)
    *real_line_number = line_number;

  node = tree->root_node;
  lines_left = line_number;

  /*
   * Work down through levels of the tree until a BtkTextBTreeNode is found at
   * level 0.
   */

  while (node->level != 0)
    {
      for (node = node->children.node;
           node->num_lines <= lines_left;
           node = node->next)
        {
#if 0
          if (node == NULL)
            {
              g_error ("btk_text_btree_find_line ran out of BtkTextBTreeNodes");
            }
#endif
          lines_left -= node->num_lines;
        }
    }

  /*
   * Work through the lines attached to the level-0 BtkTextBTreeNode.
   */

  for (line = node->children.line; lines_left > 0;
       line = line->next)
    {
#if 0
      if (line == NULL)
        {
          g_error ("btk_text_btree_find_line ran out of lines");
        }
#endif
      lines_left -= 1;
    }
  return line;
}

BtkTextLine*
_btk_text_btree_get_end_iter_line (BtkTextBTree *tree)
{
  return
    _btk_text_btree_get_line (tree,
                              _btk_text_btree_line_count (tree) - 1,
                              NULL);
}

BtkTextLine*
_btk_text_btree_get_line (BtkTextBTree *tree,
                          bint          line_number,
                          bint         *real_line_number)
{
  return get_line_internal (tree, line_number, real_line_number, TRUE);
}

BtkTextLine*
_btk_text_btree_get_line_no_last (BtkTextBTree      *tree,
                                  bint               line_number,
                                  bint              *real_line_number)
{
  return get_line_internal (tree, line_number, real_line_number, FALSE);
}

BtkTextLine*
_btk_text_btree_get_line_at_char (BtkTextBTree      *tree,
                                  bint               char_index,
                                  bint              *line_start_index,
                                  bint              *real_char_index)
{
  BtkTextBTreeNode *node;
  BtkTextLine *line;
  BtkTextLineSegment *seg;
  int chars_left;
  int chars_in_line;

  node = tree->root_node;

  /* Clamp to valid indexes (-1 is magic for "highest index"),
   * node->num_chars includes the two newlines that aren't really
   * in the buffer.
   */
  if (char_index < 0 || char_index >= (node->num_chars - 1))
    {
      char_index = node->num_chars - 2;
    }

  *real_char_index = char_index;

  /*
   * Work down through levels of the tree until a BtkTextBTreeNode is found at
   * level 0.
   */

  chars_left = char_index;
  while (node->level != 0)
    {
      for (node = node->children.node;
           chars_left >= node->num_chars;
           node = node->next)
        {
          chars_left -= node->num_chars;

          g_assert (chars_left >= 0);
        }
    }

  if (chars_left == 0)
    {
      /* Start of a line */

      *line_start_index = char_index;
      return node->children.line;
    }

  /*
   * Work through the lines attached to the level-0 BtkTextBTreeNode.
   */

  chars_in_line = 0;
  seg = NULL;
  for (line = node->children.line; line != NULL; line = line->next)
    {
      seg = line->segments;
      while (seg != NULL)
        {
          if (chars_in_line + seg->char_count > chars_left)
            goto found; /* found our line/segment */

          chars_in_line += seg->char_count;

          seg = seg->next;
        }

      chars_left -= chars_in_line;

      chars_in_line = 0;
      seg = NULL;
    }

 found:

  g_assert (line != NULL); /* hosage, ran out of lines */
  g_assert (seg != NULL);

  *line_start_index = char_index - chars_left;
  return line;
}

/* It returns an array sorted by tags priority, ready to pass to
 * _btk_text_attributes_fill_from_tags() */
BtkTextTag**
_btk_text_btree_get_tags (const BtkTextIter *iter,
                         bint *num_tags)
{
  BtkTextBTreeNode *node;
  BtkTextLine *siblingline;
  BtkTextLineSegment *seg;
  int src, dst, index;
  TagInfo tagInfo;
  BtkTextLine *line;
  bint byte_index;

#define NUM_TAG_INFOS 10

  line = _btk_text_iter_get_text_line (iter);
  byte_index = btk_text_iter_get_line_index (iter);

  tagInfo.numTags = 0;
  tagInfo.arraySize = NUM_TAG_INFOS;
  tagInfo.tags = g_new (BtkTextTag*, NUM_TAG_INFOS);
  tagInfo.counts = g_new (int, NUM_TAG_INFOS);

  /*
   * Record tag toggles within the line of indexPtr but preceding
   * indexPtr. Note that if this loop segfaults, your
   * byte_index probably points past the sum of all
   * seg->byte_count */

  for (index = 0, seg = line->segments;
       (index + seg->byte_count) <= byte_index;
       index += seg->byte_count, seg = seg->next)
    {
      if ((seg->type == &btk_text_toggle_on_type)
          || (seg->type == &btk_text_toggle_off_type))
        {
          inc_count (seg->body.toggle.info->tag, 1, &tagInfo);
        }
    }

  /*
   * Record toggles for tags in lines that are predecessors of
   * line but under the same level-0 BtkTextBTreeNode.
   */

  for (siblingline = line->parent->children.line;
       siblingline != line;
       siblingline = siblingline->next)
    {
      for (seg = siblingline->segments; seg != NULL;
           seg = seg->next)
        {
          if ((seg->type == &btk_text_toggle_on_type)
              || (seg->type == &btk_text_toggle_off_type))
            {
              inc_count (seg->body.toggle.info->tag, 1, &tagInfo);
            }
        }
    }

  /*
   * For each BtkTextBTreeNode in the ancestry of this line, record tag
   * toggles for all siblings that precede that BtkTextBTreeNode.
   */

  for (node = line->parent; node->parent != NULL;
       node = node->parent)
    {
      BtkTextBTreeNode *siblingPtr;
      Summary *summary;

      for (siblingPtr = node->parent->children.node;
           siblingPtr != node; siblingPtr = siblingPtr->next)
        {
          for (summary = siblingPtr->summary; summary != NULL;
               summary = summary->next)
            {
              if (summary->toggle_count & 1)
                {
                  inc_count (summary->info->tag, summary->toggle_count,
                             &tagInfo);
                }
            }
        }
    }

  /*
   * Go through the tag information and squash out all of the tags
   * that have even toggle counts (these tags exist before the point
   * of interest, but not at the desired character itself).
   */

  for (src = 0, dst = 0; src < tagInfo.numTags; src++)
    {
      if (tagInfo.counts[src] & 1)
        {
          g_assert (BTK_IS_TEXT_TAG (tagInfo.tags[src]));
          tagInfo.tags[dst] = tagInfo.tags[src];
          dst++;
        }
    }

  *num_tags = dst;
  g_free (tagInfo.counts);
  if (dst == 0)
    {
      g_free (tagInfo.tags);
      return NULL;
    }

  /* Sort tags in ascending order of priority */
  _btk_text_tag_array_sort (tagInfo.tags, dst);

  return tagInfo.tags;
}

static void
copy_segment (GString *string,
              bboolean include_hidden,
              bboolean include_nonchars,
              const BtkTextIter *start,
              const BtkTextIter *end)
{
  BtkTextLineSegment *end_seg;
  BtkTextLineSegment *seg;

  if (btk_text_iter_equal (start, end))
    return;

  seg = _btk_text_iter_get_indexable_segment (start);
  end_seg = _btk_text_iter_get_indexable_segment (end);

  if (seg->type == &btk_text_char_type)
    {
      bboolean copy = TRUE;
      bint copy_bytes = 0;
      bint copy_start = 0;

      /* Don't copy if we're invisible; segments are invisible/not
         as a whole, no need to check each char */
      if (!include_hidden &&
          _btk_text_btree_char_is_invisible (start))
        {
          copy = FALSE;
          /* printf (" <invisible>\n"); */
        }

      copy_start = _btk_text_iter_get_segment_byte (start);

      if (seg == end_seg)
        {
          /* End is in the same segment; need to copy fewer bytes. */
          bint end_byte = _btk_text_iter_get_segment_byte (end);

          copy_bytes = end_byte - copy_start;
        }
      else
        copy_bytes = seg->byte_count - copy_start;

      g_assert (copy_bytes != 0); /* Due to iter equality check at
                                     front of this function. */

      if (copy)
        {
          g_assert ((copy_start + copy_bytes) <= seg->byte_count);

          g_string_append_len (string,
                               seg->body.chars + copy_start,
                               copy_bytes);
        }

      /* printf ("  :%s\n", string->str); */
    }
  else if (seg->type == &btk_text_pixbuf_type ||
           seg->type == &btk_text_child_type)
    {
      bboolean copy = TRUE;

      if (!include_nonchars)
        {
          copy = FALSE;
        }
      else if (!include_hidden &&
               _btk_text_btree_char_is_invisible (start))
        {
          copy = FALSE;
        }

      if (copy)
        {
          g_string_append_len (string,
                               btk_text_unknown_char_utf8,
                               3);

        }
    }
}

bchar*
_btk_text_btree_get_text (const BtkTextIter *start_orig,
                         const BtkTextIter *end_orig,
                         bboolean include_hidden,
                         bboolean include_nonchars)
{
  BtkTextLineSegment *seg;
  BtkTextLineSegment *end_seg;
  GString *retval;
  bchar *str;
  BtkTextIter iter;
  BtkTextIter start;
  BtkTextIter end;

  g_return_val_if_fail (start_orig != NULL, NULL);
  g_return_val_if_fail (end_orig != NULL, NULL);
  g_return_val_if_fail (_btk_text_iter_get_btree (start_orig) ==
                        _btk_text_iter_get_btree (end_orig), NULL);

  start = *start_orig;
  end = *end_orig;

  btk_text_iter_order (&start, &end);

  retval = g_string_new (NULL);

  end_seg = _btk_text_iter_get_indexable_segment (&end);
  iter = start;
  seg = _btk_text_iter_get_indexable_segment (&iter);
  while (seg != end_seg)
    {
      copy_segment (retval, include_hidden, include_nonchars,
                    &iter, &end);

      _btk_text_iter_forward_indexable_segment (&iter);

      seg = _btk_text_iter_get_indexable_segment (&iter);
    }

  copy_segment (retval, include_hidden, include_nonchars, &iter, &end);

  str = retval->str;
  g_string_free (retval, FALSE);
  return str;
}

bint
_btk_text_btree_line_count (BtkTextBTree *tree)
{
  /* Subtract bogus line at the end; we return a count
     of usable lines. */
  return tree->root_node->num_lines - 1;
}

bint
_btk_text_btree_char_count (BtkTextBTree *tree)
{
  /* Exclude newline in bogus last line and the
   * one in the last line that is after the end iterator
   */
  return tree->root_node->num_chars - 2;
}

#define LOTSA_TAGS 1000
bboolean
_btk_text_btree_char_is_invisible (const BtkTextIter *iter)
{
  bboolean invisible = FALSE;  /* if nobody says otherwise, it's visible */

  int deftagCnts[LOTSA_TAGS] = { 0, };
  int *tagCnts = deftagCnts;
  BtkTextTag *deftags[LOTSA_TAGS];
  BtkTextTag **tags = deftags;
  int numTags;
  BtkTextBTreeNode *node;
  BtkTextLine *siblingline;
  BtkTextLineSegment *seg;
  BtkTextTag *tag;
  int i, index;
  BtkTextLine *line;
  BtkTextBTree *tree;
  bint byte_index;

  line = _btk_text_iter_get_text_line (iter);
  tree = _btk_text_iter_get_btree (iter);
  byte_index = btk_text_iter_get_line_index (iter);

  numTags = btk_text_tag_table_get_size (tree->table);

  /* almost always avoid malloc, so stay out of system calls */
  if (LOTSA_TAGS < numTags)
    {
      tagCnts = g_new0 (int, numTags);
      tags = g_new (BtkTextTag*, numTags);
    }

  /*
   * Record tag toggles within the line of indexPtr but preceding
   * indexPtr.
   */

  for (index = 0, seg = line->segments;
       (index + seg->byte_count) <= byte_index; /* segfault here means invalid index */
       index += seg->byte_count, seg = seg->next)
    {
      if ((seg->type == &btk_text_toggle_on_type)
          || (seg->type == &btk_text_toggle_off_type))
        {
          tag = seg->body.toggle.info->tag;
          if (tag->invisible_set)
            {
              tags[tag->priority] = tag;
              tagCnts[tag->priority]++;
            }
        }
    }

  /*
   * Record toggles for tags in lines that are predecessors of
   * line but under the same level-0 BtkTextBTreeNode.
   */

  for (siblingline = line->parent->children.line;
       siblingline != line;
       siblingline = siblingline->next)
    {
      for (seg = siblingline->segments; seg != NULL;
           seg = seg->next)
        {
          if ((seg->type == &btk_text_toggle_on_type)
              || (seg->type == &btk_text_toggle_off_type))
            {
              tag = seg->body.toggle.info->tag;
              if (tag->invisible_set)
                {
                  tags[tag->priority] = tag;
                  tagCnts[tag->priority]++;
                }
            }
        }
    }

  /*
   * For each BtkTextBTreeNode in the ancestry of this line, record tag toggles
   * for all siblings that precede that BtkTextBTreeNode.
   */

  for (node = line->parent; node->parent != NULL;
       node = node->parent)
    {
      BtkTextBTreeNode *siblingPtr;
      Summary *summary;

      for (siblingPtr = node->parent->children.node;
           siblingPtr != node; siblingPtr = siblingPtr->next)
        {
          for (summary = siblingPtr->summary; summary != NULL;
               summary = summary->next)
            {
              if (summary->toggle_count & 1)
                {
                  tag = summary->info->tag;
                  if (tag->invisible_set)
                    {
                      tags[tag->priority] = tag;
                      tagCnts[tag->priority] += summary->toggle_count;
                    }
                }
            }
        }
    }

  /*
   * Now traverse from highest priority to lowest,
   * take invisible value from first odd count (= on)
   */

  for (i = numTags-1; i >=0; i--)
    {
      if (tagCnts[i] & 1)
        {
          /* FIXME not sure this should be if 0 */
#if 0
#ifndef ALWAYS_SHOW_SELECTION
          /* who would make the selection invisible? */
          if ((tag == tkxt->seltag)
              && !(tkxt->flags & GOT_FOCUS))
            {
              continue;
            }
#endif
#endif
          invisible = tags[i]->values->invisible;
          break;
        }
    }

  if (LOTSA_TAGS < numTags)
    {
      g_free (tagCnts);
      g_free (tags);
    }

  return invisible;
}


/*
 * Manipulate marks
 */

static void
redisplay_rebunnyion (BtkTextBTree      *tree,
                  const BtkTextIter *start,
                  const BtkTextIter *end,
                  bboolean           cursors_only)
{
  BTreeView *view;
  BtkTextLine *start_line, *end_line;

  if (btk_text_iter_compare (start, end) > 0)
    {
      const BtkTextIter *tmp = start;
      start = end;
      end = tmp;
    }

  start_line = _btk_text_iter_get_text_line (start);
  end_line = _btk_text_iter_get_text_line (end);

  view = tree->views;
  while (view != NULL)
    {
      bint start_y, end_y;
      BtkTextLineData *ld;

      start_y = _btk_text_btree_find_line_top (tree, start_line, view->view_id);

      if (end_line == start_line)
        end_y = start_y;
      else
        end_y = _btk_text_btree_find_line_top (tree, end_line, view->view_id);

      ld = _btk_text_line_get_data (end_line, view->view_id);
      if (ld)
        end_y += ld->height;

      if (cursors_only)
	btk_text_layout_cursors_changed (view->layout, start_y,
					 end_y - start_y,
					  end_y - start_y);
      else
	btk_text_layout_changed (view->layout, start_y,
				 end_y - start_y,
				 end_y - start_y);

      view = view->next;
    }
}

static void
redisplay_mark (BtkTextLineSegment *mark)
{
  BtkTextIter iter;
  BtkTextIter end;
  bboolean cursor_only;

  _btk_text_btree_get_iter_at_mark (mark->body.mark.tree,
                                   &iter,
                                   mark->body.mark.obj);

  end = iter;
  btk_text_iter_forward_char (&end);

  DV (g_print ("invalidating due to moving visible mark (%s)\n", B_STRLOC));
  cursor_only = mark == mark->body.mark.tree->insert_mark->segment;
  _btk_text_btree_invalidate_rebunnyion (mark->body.mark.tree, &iter, &end, cursor_only);
}

static void
redisplay_mark_if_visible (BtkTextLineSegment *mark)
{
  if (!mark->body.mark.visible)
    return;
  else
    redisplay_mark (mark);
}

static void
ensure_not_off_end (BtkTextBTree *tree,
                    BtkTextLineSegment *mark,
                    BtkTextIter *iter)
{
  if (btk_text_iter_get_line (iter) == _btk_text_btree_line_count (tree))
    btk_text_iter_backward_char (iter);
}

static BtkTextLineSegment*
real_set_mark (BtkTextBTree      *tree,
               BtkTextMark       *existing_mark,
               const bchar       *name,
               bboolean           left_gravity,
               const BtkTextIter *where,
               bboolean           should_exist,
               bboolean           redraw_selections)
{
  BtkTextLineSegment *mark;
  BtkTextIter iter;

  g_return_val_if_fail (tree != NULL, NULL);
  g_return_val_if_fail (where != NULL, NULL);
  g_return_val_if_fail (_btk_text_iter_get_btree (where) == tree, NULL);

  if (existing_mark)
    {
      if (btk_text_mark_get_buffer (existing_mark) != NULL)
	mark = existing_mark->segment;
      else
	mark = NULL;
    }
  else if (name != NULL)
    mark = g_hash_table_lookup (tree->mark_table,
                                name);
  else
    mark = NULL;

  if (should_exist && mark == NULL)
    {
      g_warning ("No mark `%s' exists!", name);
      return NULL;
    }

  /* OK if !should_exist and it does already exist, in that case
   * we just move it.
   */
  
  iter = *where;

  if (btk_debug_flags & BTK_DEBUG_TEXT)
    _btk_text_iter_check (&iter);
  
  if (mark != NULL)
    {
      if (redraw_selections &&
          (mark == tree->insert_mark->segment ||
           mark == tree->selection_bound_mark->segment))
        {
          BtkTextIter old_pos;

          _btk_text_btree_get_iter_at_mark (tree, &old_pos,
                                           mark->body.mark.obj);
          redisplay_rebunnyion (tree, &old_pos, where, TRUE);
        }

      /*
       * don't let visible marks be after the final newline of the
       *  file.
       */

      if (mark->body.mark.visible)
        {
          ensure_not_off_end (tree, mark, &iter);
        }

      /* Redraw the mark's old location. */
      redisplay_mark_if_visible (mark);

      /* Unlink mark from its current location.
         This could hose our iterator... */
      btk_text_btree_unlink_segment (tree, mark,
                                     mark->body.mark.line);
      mark->body.mark.line = _btk_text_iter_get_text_line (&iter);
      g_assert (mark->body.mark.line == _btk_text_iter_get_text_line (&iter));

      segments_changed (tree); /* make sure the iterator recomputes its
                                  segment */
    }
  else
    {
      if (existing_mark)
	g_object_ref (existing_mark);
      else
	existing_mark = btk_text_mark_new (name, left_gravity);

      mark = existing_mark->segment;
      _btk_mark_segment_set_tree (mark, tree);

      mark->body.mark.line = _btk_text_iter_get_text_line (&iter);

      if (mark->body.mark.name)
        g_hash_table_insert (tree->mark_table,
                             mark->body.mark.name,
                             mark);
    }

  if (btk_debug_flags & BTK_DEBUG_TEXT)
    _btk_text_iter_check (&iter);
  
  /* Link mark into new location */
  btk_text_btree_link_segment (mark, &iter);

  /* Invalidate some iterators. */
  segments_changed (tree);

  /*
   * update the screen at the mark's new location.
   */

  redisplay_mark_if_visible (mark);

  if (btk_debug_flags & BTK_DEBUG_TEXT)
    _btk_text_iter_check (&iter);

  if (btk_debug_flags & BTK_DEBUG_TEXT)
    _btk_text_btree_check (tree);
  
  return mark;
}


BtkTextMark*
_btk_text_btree_set_mark (BtkTextBTree *tree,
                         BtkTextMark  *existing_mark,
                         const bchar *name,
                         bboolean left_gravity,
                         const BtkTextIter *iter,
                         bboolean should_exist)
{
  BtkTextLineSegment *seg;

  seg = real_set_mark (tree, existing_mark,
                       name, left_gravity, iter, should_exist,
                       TRUE);

  return seg ? seg->body.mark.obj : NULL;
}

bboolean
_btk_text_btree_get_selection_bounds (BtkTextBTree *tree,
                                     BtkTextIter  *start,
                                     BtkTextIter  *end)
{
  BtkTextIter tmp_start, tmp_end;

  _btk_text_btree_get_iter_at_mark (tree, &tmp_start,
                                   tree->insert_mark);
  _btk_text_btree_get_iter_at_mark (tree, &tmp_end,
                                   tree->selection_bound_mark);

  if (btk_text_iter_equal (&tmp_start, &tmp_end))
    {
      if (start)
        *start = tmp_start;

      if (end)
        *end = tmp_end;

      return FALSE;
    }
  else
    {
      btk_text_iter_order (&tmp_start, &tmp_end);

      if (start)
        *start = tmp_start;

      if (end)
        *end = tmp_end;

      return TRUE;
    }
}

void
_btk_text_btree_place_cursor (BtkTextBTree      *tree,
                             const BtkTextIter *iter)
{
  _btk_text_btree_select_range (tree, iter, iter);
}

void
_btk_text_btree_select_range (BtkTextBTree      *tree,
			      const BtkTextIter *ins,
                              const BtkTextIter *bound)
{
  BtkTextIter old_ins, old_bound;

  _btk_text_btree_get_iter_at_mark (tree, &old_ins,
                                    tree->insert_mark);
  _btk_text_btree_get_iter_at_mark (tree, &old_bound,
                                    tree->selection_bound_mark);

  /* Check if it's no-op since btk_text_buffer_place_cursor()
   * also calls this, and this will redraw the cursor line. */
  if (!btk_text_iter_equal (&old_ins, ins) ||
      !btk_text_iter_equal (&old_bound, bound))
    {
      redisplay_rebunnyion (tree, &old_ins, &old_bound, TRUE);

      /* Move insert AND selection_bound before we redisplay */
      real_set_mark (tree, tree->insert_mark,
		     "insert", FALSE, ins, TRUE, FALSE);
      real_set_mark (tree, tree->selection_bound_mark,
		     "selection_bound", FALSE, bound, TRUE, FALSE);

      redisplay_rebunnyion (tree, ins, bound, TRUE);
    }
}


void
_btk_text_btree_remove_mark_by_name (BtkTextBTree *tree,
                                    const bchar *name)
{
  BtkTextMark *mark;

  g_return_if_fail (tree != NULL);
  g_return_if_fail (name != NULL);

  mark = g_hash_table_lookup (tree->mark_table,
                              name);

  _btk_text_btree_remove_mark (tree, mark);
}

void
_btk_text_btree_release_mark_segment (BtkTextBTree       *tree,
                                      BtkTextLineSegment *segment)
{

  if (segment->body.mark.name)
    g_hash_table_remove (tree->mark_table, segment->body.mark.name);

  segment->body.mark.tree = NULL;
  segment->body.mark.line = NULL;
  
  /* Remove the ref on the mark, which frees segment as a side effect
   * if this is the last reference.
   */
  g_object_unref (segment->body.mark.obj);
}

void
_btk_text_btree_remove_mark (BtkTextBTree *tree,
                             BtkTextMark *mark)
{
  BtkTextLineSegment *segment;

  g_return_if_fail (mark != NULL);
  g_return_if_fail (tree != NULL);

  segment = mark->segment;

  if (segment->body.mark.not_deleteable)
    {
      g_warning ("Can't delete special mark `%s'", segment->body.mark.name);
      return;
    }

  /* This calls cleanup_line and segments_changed */
  btk_text_btree_unlink_segment (tree, segment, segment->body.mark.line);
  
  _btk_text_btree_release_mark_segment (tree, segment);
}

bboolean
_btk_text_btree_mark_is_insert (BtkTextBTree *tree,
                                BtkTextMark *segment)
{
  return segment == tree->insert_mark;
}

bboolean
_btk_text_btree_mark_is_selection_bound (BtkTextBTree *tree,
                                         BtkTextMark *segment)
{
  return segment == tree->selection_bound_mark;
}

BtkTextMark *
_btk_text_btree_get_insert (BtkTextBTree *tree)
{
  return tree->insert_mark;
}

BtkTextMark *
_btk_text_btree_get_selection_bound (BtkTextBTree *tree)
{
  return tree->selection_bound_mark;
}

BtkTextMark*
_btk_text_btree_get_mark_by_name (BtkTextBTree *tree,
                                  const bchar *name)
{
  BtkTextLineSegment *seg;

  g_return_val_if_fail (tree != NULL, NULL);
  g_return_val_if_fail (name != NULL, NULL);

  seg = g_hash_table_lookup (tree->mark_table, name);

  return seg ? seg->body.mark.obj : NULL;
}

/**
 * btk_text_mark_set_visible:
 * @mark: a #BtkTextMark
 * @setting: visibility of mark
 * 
 * Sets the visibility of @mark; the insertion point is normally
 * visible, i.e. you can see it as a vertical bar. Also, the text
 * widget uses a visible mark to indicate where a drop will occur when
 * dragging-and-dropping text. Most other marks are not visible.
 * Marks are not visible by default.
 * 
 **/
void
btk_text_mark_set_visible (BtkTextMark       *mark,
                           bboolean           setting)
{
  BtkTextLineSegment *seg;

  g_return_if_fail (mark != NULL);

  seg = mark->segment;

  if (seg->body.mark.visible == setting)
    return;
  else
    {
      seg->body.mark.visible = setting;

      if (seg->body.mark.tree)
	redisplay_mark (seg);
    }
}

BtkTextLine*
_btk_text_btree_first_could_contain_tag (BtkTextBTree *tree,
                                        BtkTextTag *tag)
{
  BtkTextBTreeNode *node;
  BtkTextTagInfo *info;

  g_return_val_if_fail (tree != NULL, NULL);

  if (tag != NULL)
    {
      info = btk_text_btree_get_existing_tag_info (tree, tag);

      if (info == NULL)
        return NULL;

      if (info->tag_root == NULL)
        return NULL;

      node = info->tag_root;

      /* We know the tag root has instances of the given
         tag below it */

    continue_outer_loop:
      g_assert (node != NULL);
      while (node->level > 0)
        {
          g_assert (node != NULL); /* Failure probably means bad tag summaries. */
          node = node->children.node;
          while (node != NULL)
            {
              if (btk_text_btree_node_has_tag (node, tag))
                goto continue_outer_loop;

              node = node->next;
            }
          g_assert (node != NULL);
        }

      g_assert (node != NULL); /* The tag summaries said some node had
                                  tag toggles... */

      g_assert (node->level == 0);

      return node->children.line;
    }
  else
    {
      /* Looking for any tag at all (tag == NULL).
         Unfortunately this can't be done in a simple and efficient way
         right now; so I'm just going to return the
         first line in the btree. FIXME */
      return _btk_text_btree_get_line (tree, 0, NULL);
    }
}

BtkTextLine*
_btk_text_btree_last_could_contain_tag (BtkTextBTree *tree,
                                       BtkTextTag *tag)
{
  BtkTextBTreeNode *node;
  BtkTextBTreeNode *last_node;
  BtkTextLine *line;
  BtkTextTagInfo *info;

  g_return_val_if_fail (tree != NULL, NULL);

  if (tag != NULL)
    {
      info = btk_text_btree_get_existing_tag_info (tree, tag);

      if (info->tag_root == NULL)
        return NULL;

      node = info->tag_root;
      /* We know the tag root has instances of the given
         tag below it */

      while (node->level > 0)
        {
          g_assert (node != NULL); /* Failure probably means bad tag summaries. */
          last_node = NULL;
          node = node->children.node;
          while (node != NULL)
            {
              if (btk_text_btree_node_has_tag (node, tag))
                last_node = node;
              node = node->next;
            }

          node = last_node;
        }

      g_assert (node != NULL); /* The tag summaries said some node had
                                  tag toggles... */

      g_assert (node->level == 0);

      /* Find the last line in this node */
      line = node->children.line;
      while (line->next != NULL)
        line = line->next;

      return line;
    }
  else
    {
      /* This search can't be done efficiently at the moment,
         at least not without complexity.
         So, we just return the last line.
      */
      return _btk_text_btree_get_end_iter_line (tree);
    }
}


/*
 * Lines
 */

bint
_btk_text_line_get_number (BtkTextLine *line)
{
  BtkTextLine *line2;
  BtkTextBTreeNode *node, *parent, *node2;
  int index;

  /*
   * First count how many lines precede this one in its level-0
   * BtkTextBTreeNode.
   */

  node = line->parent;
  index = 0;
  for (line2 = node->children.line; line2 != line;
       line2 = line2->next)
    {
      if (line2 == NULL)
        {
          g_error ("btk_text_btree_line_number couldn't find line");
        }
      index += 1;
    }

  /*
   * Now work up through the levels of the tree one at a time,
   * counting how many lines are in BtkTextBTreeNodes preceding the current
   * BtkTextBTreeNode.
   */

  for (parent = node->parent ; parent != NULL;
       node = parent, parent = parent->parent)
    {
      for (node2 = parent->children.node; node2 != node;
           node2 = node2->next)
        {
          if (node2 == NULL)
            {
              g_error ("btk_text_btree_line_number couldn't find BtkTextBTreeNode");
            }
          index += node2->num_lines;
        }
    }
  return index;
}

static BtkTextLineSegment*
find_toggle_segment_before_char (BtkTextLine *line,
                                 bint char_in_line,
                                 BtkTextTag *tag)
{
  BtkTextLineSegment *seg;
  BtkTextLineSegment *toggle_seg;
  int index;

  toggle_seg = NULL;
  index = 0;
  seg = line->segments;
  while ( (index + seg->char_count) <= char_in_line )
    {
      if (((seg->type == &btk_text_toggle_on_type)
           || (seg->type == &btk_text_toggle_off_type))
          && (seg->body.toggle.info->tag == tag))
        toggle_seg = seg;

      index += seg->char_count;
      seg = seg->next;
    }

  return toggle_seg;
}

static BtkTextLineSegment*
find_toggle_segment_before_byte (BtkTextLine *line,
                                 bint byte_in_line,
                                 BtkTextTag *tag)
{
  BtkTextLineSegment *seg;
  BtkTextLineSegment *toggle_seg;
  int index;

  toggle_seg = NULL;
  index = 0;
  seg = line->segments;
  while ( (index + seg->byte_count) <= byte_in_line )
    {
      if (((seg->type == &btk_text_toggle_on_type)
           || (seg->type == &btk_text_toggle_off_type))
          && (seg->body.toggle.info->tag == tag))
        toggle_seg = seg;

      index += seg->byte_count;
      seg = seg->next;
    }

  return toggle_seg;
}

static bboolean
find_toggle_outside_current_line (BtkTextLine *line,
                                  BtkTextBTree *tree,
                                  BtkTextTag *tag)
{
  BtkTextBTreeNode *node;
  BtkTextLine *sibling_line;
  BtkTextLineSegment *seg;
  BtkTextLineSegment *toggle_seg;
  int toggles;
  BtkTextTagInfo *info = NULL;

  /*
   * No toggle in this line.  Look for toggles for the tag in lines
   * that are predecessors of line but under the same
   * level-0 BtkTextBTreeNode.
   */
  toggle_seg = NULL;
  sibling_line = line->parent->children.line;
  while (sibling_line != line)
    {
      seg = sibling_line->segments;
      while (seg != NULL)
        {
          if (((seg->type == &btk_text_toggle_on_type)
               || (seg->type == &btk_text_toggle_off_type))
              && (seg->body.toggle.info->tag == tag))
            toggle_seg = seg;

          seg = seg->next;
        }

      sibling_line = sibling_line->next;
    }

  if (toggle_seg != NULL)
    return (toggle_seg->type == &btk_text_toggle_on_type);

  /*
   * No toggle in this BtkTextBTreeNode.  Scan upwards through the ancestors of
   * this BtkTextBTreeNode, counting the number of toggles of the given tag in
   * siblings that precede that BtkTextBTreeNode.
   */

  info = btk_text_btree_get_existing_tag_info (tree, tag);

  if (info == NULL)
    return FALSE;

  toggles = 0;
  node = line->parent;
  while (node->parent != NULL)
    {
      BtkTextBTreeNode *sibling_node;

      sibling_node = node->parent->children.node;
      while (sibling_node != node)
        {
          Summary *summary;

          summary = sibling_node->summary;
          while (summary != NULL)
            {
              if (summary->info == info)
                toggles += summary->toggle_count;

              summary = summary->next;
            }

          sibling_node = sibling_node->next;
        }

      if (node == info->tag_root)
        break;

      node = node->parent;
    }

  /*
   * An odd number of toggles means that the tag is present at the
   * given point.
   */

  return (toggles & 1) != 0;
}

/* FIXME this function is far too slow, for no good reason. */
bboolean
_btk_text_line_char_has_tag (BtkTextLine *line,
                             BtkTextBTree *tree,
                             bint char_in_line,
                             BtkTextTag *tag)
{
  BtkTextLineSegment *toggle_seg;

  g_return_val_if_fail (line != NULL, FALSE);

  /*
   * Check for toggles for the tag in the line but before
   * the char.  If there is one, its type indicates whether or
   * not the character is tagged.
   */

  toggle_seg = find_toggle_segment_before_char (line, char_in_line, tag);

  if (toggle_seg != NULL)
    return (toggle_seg->type == &btk_text_toggle_on_type);
  else
    return find_toggle_outside_current_line (line, tree, tag);
}

bboolean
_btk_text_line_byte_has_tag (BtkTextLine *line,
                             BtkTextBTree *tree,
                             bint byte_in_line,
                             BtkTextTag *tag)
{
  BtkTextLineSegment *toggle_seg;

  g_return_val_if_fail (line != NULL, FALSE);

  /*
   * Check for toggles for the tag in the line but before
   * the char.  If there is one, its type indicates whether or
   * not the character is tagged.
   */

  toggle_seg = find_toggle_segment_before_byte (line, byte_in_line, tag);

  if (toggle_seg != NULL)
    return (toggle_seg->type == &btk_text_toggle_on_type);
  else
    return find_toggle_outside_current_line (line, tree, tag);
}

bboolean
_btk_text_line_is_last (BtkTextLine *line,
                        BtkTextBTree *tree)
{
  return line == get_last_line (tree);
}

static void
ensure_end_iter_line (BtkTextBTree *tree)
{
  if (tree->end_iter_line_stamp != tree->chars_changed_stamp)
    {
      bint real_line;
	
       /* n_lines is without the magic line at the end */
      g_assert (_btk_text_btree_line_count (tree) >= 1);

      tree->end_iter_line = _btk_text_btree_get_line_no_last (tree, -1, &real_line);
      
      tree->end_iter_line_stamp = tree->chars_changed_stamp;
    }
}

static void
ensure_end_iter_segment (BtkTextBTree *tree)
{
  if (tree->end_iter_segment_stamp != tree->segments_changed_stamp)
    {
      BtkTextLineSegment *seg;
      BtkTextLineSegment *last_with_chars;

      ensure_end_iter_line (tree);

      last_with_chars = NULL;
      
      seg = tree->end_iter_line->segments;
      while (seg != NULL)
        {
          if (seg->char_count > 0)
            last_with_chars = seg;
          seg = seg->next;
        }

      tree->end_iter_segment = last_with_chars;

      /* We know the last char in the last line is '\n' */
      tree->end_iter_segment_byte_index = last_with_chars->byte_count - 1;
      tree->end_iter_segment_char_offset = last_with_chars->char_count - 1;
      
      tree->end_iter_segment_stamp = tree->segments_changed_stamp;

      g_assert (tree->end_iter_segment->type == &btk_text_char_type);
      g_assert (tree->end_iter_segment->body.chars[tree->end_iter_segment_byte_index] == '\n');
    }
}

bboolean
_btk_text_line_contains_end_iter (BtkTextLine  *line,
                                  BtkTextBTree *tree)
{
  ensure_end_iter_line (tree);

  return line == tree->end_iter_line;
}

bboolean
_btk_text_btree_is_end (BtkTextBTree       *tree,
                        BtkTextLine        *line,
                        BtkTextLineSegment *seg,
                        int                 byte_index,
                        int                 char_offset)
{
  g_return_val_if_fail (byte_index >= 0 || char_offset >= 0, FALSE);
  
  /* Do this first to avoid walking segments in most cases */
  if (!_btk_text_line_contains_end_iter (line, tree))
    return FALSE;

  ensure_end_iter_segment (tree);

  if (seg != tree->end_iter_segment)
    return FALSE;

  if (byte_index >= 0)
    return byte_index == tree->end_iter_segment_byte_index;
  else
    return char_offset == tree->end_iter_segment_char_offset;
}

BtkTextLine*
_btk_text_line_next (BtkTextLine *line)
{
  BtkTextBTreeNode *node;

  if (line->next != NULL)
    return line->next;
  else
    {
      /*
       * This was the last line associated with the particular parent
       * BtkTextBTreeNode.  Search up the tree for the next BtkTextBTreeNode,
       * then search down from that BtkTextBTreeNode to find the first
       * line.
       */

      node = line->parent;
      while (node != NULL && node->next == NULL)
        node = node->parent;

      if (node == NULL)
        return NULL;

      node = node->next;
      while (node->level > 0)
        {
          node = node->children.node;
        }

      g_assert (node->children.line != line);

      return node->children.line;
    }
}

BtkTextLine*
_btk_text_line_next_excluding_last (BtkTextLine *line)
{
  BtkTextLine *next;
  
  next = _btk_text_line_next (line);

  /* If we were on the end iter line, we can't go to
   * the last line
   */
  if (next && next->next == NULL && /* these checks are optimization only */
      _btk_text_line_next (next) == NULL)
    return NULL;

  return next;
}

BtkTextLine*
_btk_text_line_previous (BtkTextLine *line)
{
  BtkTextBTreeNode *node;
  BtkTextBTreeNode *node2;
  BtkTextLine *prev;

  /*
   * Find the line under this BtkTextBTreeNode just before the starting line.
   */
  prev = line->parent->children.line;        /* First line at leaf */
  while (prev != line)
    {
      if (prev->next == line)
        return prev;

      prev = prev->next;

      if (prev == NULL)
        g_error ("btk_text_btree_previous_line ran out of lines");
    }

  /*
   * This was the first line associated with the particular parent
   * BtkTextBTreeNode.  Search up the tree for the previous BtkTextBTreeNode,
   * then search down from that BtkTextBTreeNode to find its last line.
   */
  for (node = line->parent; ; node = node->parent)
    {
      if (node == NULL || node->parent == NULL)
        return NULL;
      else if (node != node->parent->children.node)
        break;
    }

  for (node2 = node->parent->children.node; ;
       node2 = node2->children.node)
    {
      while (node2->next != node)
        node2 = node2->next;

      if (node2->level == 0)
        break;

      node = NULL;
    }

  for (prev = node2->children.line ; ; prev = prev->next)
    {
      if (prev->next == NULL)
        return prev;
    }

  g_assert_not_reached ();
  return NULL;
}


BtkTextLineData*
_btk_text_line_data_new (BtkTextLayout *layout,
                         BtkTextLine   *line)
{
  BtkTextLineData *line_data;

  line_data = g_new (BtkTextLineData, 1);

  line_data->view_id = layout;
  line_data->next = NULL;
  line_data->width = 0;
  line_data->height = 0;
  line_data->valid = FALSE;

  return line_data;
}

void
_btk_text_line_add_data (BtkTextLine     *line,
                         BtkTextLineData *data)
{
  g_return_if_fail (line != NULL);
  g_return_if_fail (data != NULL);
  g_return_if_fail (data->view_id != NULL);

  if (line->views)
    {
      data->next = line->views;
      line->views = data;
    }
  else
    {
      line->views = data;
    }
}

bpointer
_btk_text_line_remove_data (BtkTextLine *line,
                           bpointer view_id)
{
  BtkTextLineData *prev;
  BtkTextLineData *iter;

  g_return_val_if_fail (line != NULL, NULL);
  g_return_val_if_fail (view_id != NULL, NULL);

  prev = NULL;
  iter = line->views;
  while (iter != NULL)
    {
      if (iter->view_id == view_id)
        break;
      prev = iter;
      iter = iter->next;
    }

  if (iter)
    {
      if (prev)
        prev->next = iter->next;
      else
        line->views = iter->next;

      return iter;
    }
  else
    return NULL;
}

bpointer
_btk_text_line_get_data (BtkTextLine *line,
                         bpointer view_id)
{
  BtkTextLineData *iter;

  g_return_val_if_fail (line != NULL, NULL);
  g_return_val_if_fail (view_id != NULL, NULL);

  iter = line->views;
  while (iter != NULL)
    {
      if (iter->view_id == view_id)
        break;
      iter = iter->next;
    }

  return iter;
}

void
_btk_text_line_invalidate_wrap (BtkTextLine *line,
                                BtkTextLineData *ld)
{
  /* For now this is totally unoptimized. FIXME?

     We could probably optimize the case where the width removed
     is less than the max width for the parent node,
     and the case where the height is unchanged when we re-wrap.
  */
  
  g_return_if_fail (ld != NULL);
  
  ld->valid = FALSE;
  btk_text_btree_node_invalidate_upward (line->parent, ld->view_id);
}

bint
_btk_text_line_char_count (BtkTextLine *line)
{
  BtkTextLineSegment *seg;
  bint size;

  size = 0;
  seg = line->segments;
  while (seg != NULL)
    {
      size += seg->char_count;
      seg = seg->next;
    }
  return size;
}

bint
_btk_text_line_byte_count (BtkTextLine *line)
{
  BtkTextLineSegment *seg;
  bint size;

  size = 0;
  seg = line->segments;
  while (seg != NULL)
    {
      size += seg->byte_count;
      seg = seg->next;
    }

  return size;
}

bint
_btk_text_line_char_index (BtkTextLine *target_line)
{
  GSList *node_stack = NULL;
  BtkTextBTreeNode *iter;
  BtkTextLine *line;
  bint num_chars;

  /* Push all our parent nodes onto a stack */
  iter = target_line->parent;

  g_assert (iter != NULL);

  while (iter != NULL)
    {
      node_stack = b_slist_prepend (node_stack, iter);

      iter = iter->parent;
    }

  /* Check that we have the root node on top of the stack. */
  g_assert (node_stack != NULL &&
            node_stack->data != NULL &&
            ((BtkTextBTreeNode*)node_stack->data)->parent == NULL);

  /* Add up chars in all nodes before the nodes in our stack.
   */

  num_chars = 0;
  iter = node_stack->data;
  while (iter != NULL)
    {
      BtkTextBTreeNode *child_iter;
      BtkTextBTreeNode *next_node;

      next_node = node_stack->next ?
        node_stack->next->data : NULL;
      node_stack = b_slist_remove (node_stack, node_stack->data);

      if (iter->level == 0)
        {
          /* stack should be empty when we're on the last node */
          g_assert (node_stack == NULL);
          break; /* Our children are now lines */
        }

      g_assert (next_node != NULL);
      g_assert (iter != NULL);
      g_assert (next_node->parent == iter);

      /* Add up chars before us in the tree */
      child_iter = iter->children.node;
      while (child_iter != next_node)
        {
          g_assert (child_iter != NULL);

          num_chars += child_iter->num_chars;

          child_iter = child_iter->next;
        }

      iter = next_node;
    }

  g_assert (iter != NULL);
  g_assert (iter == target_line->parent);

  /* Since we don't store char counts in lines, only in segments, we
     have to iterate over the lines adding up segment char counts
     until we find our line.  */
  line = iter->children.line;
  while (line != target_line)
    {
      g_assert (line != NULL);

      num_chars += _btk_text_line_char_count (line);

      line = line->next;
    }

  g_assert (line == target_line);

  return num_chars;
}

BtkTextLineSegment*
_btk_text_line_byte_to_segment (BtkTextLine *line,
                               bint byte_offset,
                               bint *seg_offset)
{
  BtkTextLineSegment *seg;
  int offset;

  g_return_val_if_fail (line != NULL, NULL);

  offset = byte_offset;
  seg = line->segments;

  while (offset >= seg->byte_count)
    {
      offset -= seg->byte_count;
      seg = seg->next;
      g_assert (seg != NULL); /* means an invalid byte index */
    }

  if (seg_offset)
    *seg_offset = offset;

  return seg;
}

BtkTextLineSegment*
_btk_text_line_char_to_segment (BtkTextLine *line,
                               bint char_offset,
                               bint *seg_offset)
{
  BtkTextLineSegment *seg;
  int offset;

  g_return_val_if_fail (line != NULL, NULL);

  offset = char_offset;
  seg = line->segments;

  while (offset >= seg->char_count)
    {
      offset -= seg->char_count;
      seg = seg->next;
      g_assert (seg != NULL); /* means an invalid char index */
    }

  if (seg_offset)
    *seg_offset = offset;

  return seg;
}

BtkTextLineSegment*
_btk_text_line_byte_to_any_segment (BtkTextLine *line,
                                   bint byte_offset,
                                   bint *seg_offset)
{
  BtkTextLineSegment *seg;
  int offset;

  g_return_val_if_fail (line != NULL, NULL);

  offset = byte_offset;
  seg = line->segments;

  while (offset > 0 && offset >= seg->byte_count)
    {
      offset -= seg->byte_count;
      seg = seg->next;
      g_assert (seg != NULL); /* means an invalid byte index */
    }

  if (seg_offset)
    *seg_offset = offset;

  return seg;
}

BtkTextLineSegment*
_btk_text_line_char_to_any_segment (BtkTextLine *line,
                                   bint char_offset,
                                   bint *seg_offset)
{
  BtkTextLineSegment *seg;
  int offset;

  g_return_val_if_fail (line != NULL, NULL);

  offset = char_offset;
  seg = line->segments;

  while (offset > 0 && offset >= seg->char_count)
    {
      offset -= seg->char_count;
      seg = seg->next;
      g_assert (seg != NULL); /* means an invalid byte index */
    }

  if (seg_offset)
    *seg_offset = offset;

  return seg;
}

bint
_btk_text_line_byte_to_char (BtkTextLine *line,
                            bint byte_offset)
{
  bint char_offset;
  BtkTextLineSegment *seg;

  g_return_val_if_fail (line != NULL, 0);
  g_return_val_if_fail (byte_offset >= 0, 0);

  char_offset = 0;
  seg = line->segments;
  while (byte_offset >= seg->byte_count) /* while (we need to go farther than
                                            the next segment) */
    {
      byte_offset -= seg->byte_count;
      char_offset += seg->char_count;
      seg = seg->next;
      g_assert (seg != NULL); /* our byte_index was bogus if this happens */
    }

  g_assert (seg != NULL);

  /* Now byte_offset is the offset into the current segment,
     and char_offset is the start of the current segment.
     Optimize the case where no chars use > 1 byte */
  if (seg->byte_count == seg->char_count)
    return char_offset + byte_offset;
  else
    {
      if (seg->type == &btk_text_char_type)
        return char_offset + g_utf8_strlen (seg->body.chars, byte_offset);
      else
        {
          g_assert (seg->char_count == 1);
          g_assert (byte_offset == 0);

          return char_offset;
        }
    }
}

bint
_btk_text_line_char_to_byte (BtkTextLine *line,
                            bint         char_offset)
{
  g_warning ("FIXME not implemented");

  return 0;
}

/* FIXME sync with char_locate (or figure out a clean
   way to merge the two functions) */
bboolean
_btk_text_line_byte_locate (BtkTextLine *line,
                            bint byte_offset,
                            BtkTextLineSegment **segment,
                            BtkTextLineSegment **any_segment,
                            bint *seg_byte_offset,
                            bint *line_byte_offset)
{
  BtkTextLineSegment *seg;
  BtkTextLineSegment *after_last_indexable;
  BtkTextLineSegment *last_indexable;
  bint offset;
  bint bytes_in_line;

  g_return_val_if_fail (line != NULL, FALSE);
  g_return_val_if_fail (byte_offset >= 0, FALSE);

  *segment = NULL;
  *any_segment = NULL;
  bytes_in_line = 0;

  offset = byte_offset;

  last_indexable = NULL;
  after_last_indexable = line->segments;
  seg = line->segments;

  /* The loop ends when we're inside a segment;
     last_indexable refers to the last segment
     we passed entirely. */
  while (seg && offset >= seg->byte_count)
    {
      if (seg->char_count > 0)
        {
          offset -= seg->byte_count;
          bytes_in_line += seg->byte_count;
          last_indexable = seg;
          after_last_indexable = last_indexable->next;
        }

      seg = seg->next;
    }

  if (seg == NULL)
    {
      /* We went off the end of the line */
      if (offset != 0)
        g_warning ("%s: byte index off the end of the line", B_STRLOC);

      return FALSE;
    }
  else
    {
      *segment = seg;
      if (after_last_indexable != NULL)
        *any_segment = after_last_indexable;
      else
        *any_segment = *segment;
    }

  /* Override any_segment if we're in the middle of a segment. */
  if (offset > 0)
    *any_segment = *segment;

  *seg_byte_offset = offset;

  g_assert (*segment != NULL);
  g_assert (*any_segment != NULL);
  g_assert (*seg_byte_offset < (*segment)->byte_count);

  *line_byte_offset = bytes_in_line + *seg_byte_offset;

  return TRUE;
}

/* FIXME sync with byte_locate (or figure out a clean
   way to merge the two functions) */
bboolean
_btk_text_line_char_locate     (BtkTextLine     *line,
                                bint              char_offset,
                                BtkTextLineSegment **segment,
                                BtkTextLineSegment **any_segment,
                                bint             *seg_char_offset,
                                bint             *line_char_offset)
{
  BtkTextLineSegment *seg;
  BtkTextLineSegment *after_last_indexable;
  BtkTextLineSegment *last_indexable;
  bint offset;
  bint chars_in_line;

  g_return_val_if_fail (line != NULL, FALSE);
  g_return_val_if_fail (char_offset >= 0, FALSE);
  
  *segment = NULL;
  *any_segment = NULL;
  chars_in_line = 0;

  offset = char_offset;

  last_indexable = NULL;
  after_last_indexable = line->segments;
  seg = line->segments;

  /* The loop ends when we're inside a segment;
     last_indexable refers to the last segment
     we passed entirely. */
  while (seg && offset >= seg->char_count)
    {
      if (seg->char_count > 0)
        {
          offset -= seg->char_count;
          chars_in_line += seg->char_count;
          last_indexable = seg;
          after_last_indexable = last_indexable->next;
        }

      seg = seg->next;
    }

  if (seg == NULL)
    {
      /* end of the line */
      if (offset != 0)
        g_warning ("%s: char offset off the end of the line", B_STRLOC);

      return FALSE;
    }
  else
    {
      *segment = seg;
      if (after_last_indexable != NULL)
        *any_segment = after_last_indexable;
      else
        *any_segment = *segment;
    }

  /* Override any_segment if we're in the middle of a segment. */
  if (offset > 0)
    *any_segment = *segment;

  *seg_char_offset = offset;

  g_assert (*segment != NULL);
  g_assert (*any_segment != NULL);
  g_assert (*seg_char_offset < (*segment)->char_count);

  *line_char_offset = chars_in_line + *seg_char_offset;

  return TRUE;
}

void
_btk_text_line_byte_to_char_offsets (BtkTextLine *line,
                                    bint byte_offset,
                                    bint *line_char_offset,
                                    bint *seg_char_offset)
{
  BtkTextLineSegment *seg;
  int offset;

  g_return_if_fail (line != NULL);
  g_return_if_fail (byte_offset >= 0);

  *line_char_offset = 0;

  offset = byte_offset;
  seg = line->segments;

  while (offset >= seg->byte_count)
    {
      offset -= seg->byte_count;
      *line_char_offset += seg->char_count;
      seg = seg->next;
      g_assert (seg != NULL); /* means an invalid char offset */
    }

  g_assert (seg->char_count > 0); /* indexable. */

  /* offset is now the number of bytes into the current segment we
   * want to go. Count chars into the current segment.
   */

  if (seg->type == &btk_text_char_type)
    {
      *seg_char_offset = g_utf8_strlen (seg->body.chars, offset);

      g_assert (*seg_char_offset < seg->char_count);

      *line_char_offset += *seg_char_offset;
    }
  else
    {
      g_assert (offset == 0);
      *seg_char_offset = 0;
    }
}

void
_btk_text_line_char_to_byte_offsets (BtkTextLine *line,
                                    bint char_offset,
                                    bint *line_byte_offset,
                                    bint *seg_byte_offset)
{
  BtkTextLineSegment *seg;
  int offset;

  g_return_if_fail (line != NULL);
  g_return_if_fail (char_offset >= 0);

  *line_byte_offset = 0;

  offset = char_offset;
  seg = line->segments;

  while (offset >= seg->char_count)
    {
      offset -= seg->char_count;
      *line_byte_offset += seg->byte_count;
      seg = seg->next;
      g_assert (seg != NULL); /* means an invalid char offset */
    }

  g_assert (seg->char_count > 0); /* indexable. */

  /* offset is now the number of chars into the current segment we
     want to go. Count bytes into the current segment. */

  if (seg->type == &btk_text_char_type)
    {
      const char *p;

      /* if in the last fourth of the segment walk backwards */
      if (seg->char_count - offset < seg->char_count / 4)
        p = g_utf8_offset_to_pointer (seg->body.chars + seg->byte_count, 
                                      offset - seg->char_count);
      else
        p = g_utf8_offset_to_pointer (seg->body.chars, offset);

      *seg_byte_offset = p - seg->body.chars;

      g_assert (*seg_byte_offset < seg->byte_count);

      *line_byte_offset += *seg_byte_offset;
    }
  else
    {
      g_assert (offset == 0);
      *seg_byte_offset = 0;
    }
}

static bint
node_compare (BtkTextBTreeNode *lhs,
              BtkTextBTreeNode *rhs)
{
  BtkTextBTreeNode *iter;
  BtkTextBTreeNode *node;
  BtkTextBTreeNode *common_parent;
  BtkTextBTreeNode *parent_of_lower;
  BtkTextBTreeNode *parent_of_higher;
  bboolean lhs_is_lower;
  BtkTextBTreeNode *lower;
  BtkTextBTreeNode *higher;

  /* This function assumes that lhs and rhs are not underneath each
   * other.
   */

  if (lhs == rhs)
    return 0;

  if (lhs->level < rhs->level)
    {
      lhs_is_lower = TRUE;
      lower = lhs;
      higher = rhs;
    }
  else
    {
      lhs_is_lower = FALSE;
      lower = rhs;
      higher = lhs;
    }

  /* Algorithm: find common parent of lhs/rhs. Save the child nodes
   * of the common parent we used to reach the common parent; the
   * ordering of these child nodes in the child list is the ordering
   * of lhs and rhs.
   */

  /* Get on the same level (may be on same level already) */
  node = lower;
  while (node->level < higher->level)
    node = node->parent;

  g_assert (node->level == higher->level);

  g_assert (node != higher); /* Happens if lower is underneath higher */

  /* Go up until we have two children with a common parent.
   */
  parent_of_lower = node;
  parent_of_higher = higher;

  while (parent_of_lower->parent != parent_of_higher->parent)
    {
      parent_of_lower = parent_of_lower->parent;
      parent_of_higher = parent_of_higher->parent;
    }

  g_assert (parent_of_lower->parent == parent_of_higher->parent);

  common_parent = parent_of_lower->parent;

  g_assert (common_parent != NULL);

  /* See which is first in the list of common_parent's children */
  iter = common_parent->children.node;
  while (iter != NULL)
    {
      if (iter == parent_of_higher)
        {
          /* higher is less than lower */

          if (lhs_is_lower)
            return 1; /* lhs > rhs */
          else
            return -1;
        }
      else if (iter == parent_of_lower)
        {
          /* lower is less than higher */

          if (lhs_is_lower)
            return -1; /* lhs < rhs */
          else
            return 1;
        }

      iter = iter->next;
    }

  g_assert_not_reached ();
  return 0;
}

/* remember that tag == NULL means "any tag" */
BtkTextLine*
_btk_text_line_next_could_contain_tag (BtkTextLine  *line,
                                       BtkTextBTree *tree,
                                       BtkTextTag   *tag)
{
  BtkTextBTreeNode *node;
  BtkTextTagInfo *info;
  bboolean below_tag_root;

  g_return_val_if_fail (line != NULL, NULL);

  if (btk_debug_flags & BTK_DEBUG_TEXT)
    _btk_text_btree_check (tree);

  if (tag == NULL)
    {
      /* Right now we can only offer linear-search if the user wants
       * to know about any tag toggle at all.
       */
      return _btk_text_line_next_excluding_last (line);
    }

  /* Our tag summaries only have node precision, not line
   * precision. This means that if any line under a node could contain a
   * tag, then any of the others could also contain a tag.
   * 
   * In the future we could have some mechanism to keep track of how
   * many toggles we've found under a node so far, since we have a
   * count of toggles under the node. But for now I'm going with KISS.
   */

  /* return same-node line, if any. */
  if (line->next)
    return line->next;

  info = btk_text_btree_get_existing_tag_info (tree, tag);
  if (info == NULL)
    return NULL;

  if (info->tag_root == NULL)
    return NULL;

  if (info->tag_root == line->parent)
    return NULL; /* we were at the last line under the tag root */

  /* We need to go up out of this node, and on to the next one with
     toggles for the target tag. If we're below the tag root, we need to
     find the next node below the tag root that has tag summaries. If
     we're not below the tag root, we need to see if the tag root is
     after us in the tree, and if so, return the first line underneath
     the tag root. */

  node = line->parent;
  below_tag_root = FALSE;
  while (node != NULL)
    {
      if (node == info->tag_root)
        {
          below_tag_root = TRUE;
          break;
        }

      node = node->parent;
    }

  if (below_tag_root)
    {
      node = line->parent;
      while (node != info->tag_root)
        {
          if (node->next == NULL)
            node = node->parent;
          else
            {
              node = node->next;

              if (btk_text_btree_node_has_tag (node, tag))
                goto found;
            }
        }
      return NULL;
    }
  else
    {
      bint ordering;

      ordering = node_compare (line->parent, info->tag_root);

      if (ordering < 0)
        {
          /* Tag root is ahead of us, so search there. */
          node = info->tag_root;
          goto found;
        }
      else
        {
          /* Tag root is after us, so no more lines that
           * could contain the tag.
           */
          return NULL;
        }

      g_assert_not_reached ();
    }

 found:

  g_assert (node != NULL);

  /* We have to find the first sub-node of this node that contains
   * the target tag.
   */

  while (node->level > 0)
    {
      g_assert (node != NULL); /* If this fails, it likely means an
                                  incorrect tag summary led us on a
                                  wild goose chase down this branch of
                                  the tree. */
      node = node->children.node;
      while (node != NULL)
        {
          if (btk_text_btree_node_has_tag (node, tag))
            break;
          node = node->next;
        }
    }

  g_assert (node != NULL);
  g_assert (node->level == 0);

  return node->children.line;
}

static BtkTextLine*
prev_line_under_node (BtkTextBTreeNode *node,
                      BtkTextLine      *line)
{
  BtkTextLine *prev;

  prev = node->children.line;

  g_assert (prev);

  if (prev != line)
    {
      while (prev->next != line)
        prev = prev->next;

      return prev;
    }

  return NULL;
}

BtkTextLine*
_btk_text_line_previous_could_contain_tag (BtkTextLine  *line,
                                          BtkTextBTree *tree,
                                          BtkTextTag   *tag)
{
  BtkTextBTreeNode *node;
  BtkTextBTreeNode *found_node = NULL;
  BtkTextTagInfo *info;
  bboolean below_tag_root;
  BtkTextLine *prev;
  BtkTextBTreeNode *line_ancestor;
  BtkTextBTreeNode *line_ancestor_parent;

  /* See next_could_contain_tag () for more extensive comments
   * on what's going on here.
   */

  g_return_val_if_fail (line != NULL, NULL);

  if (btk_debug_flags & BTK_DEBUG_TEXT)
    _btk_text_btree_check (tree);

  if (tag == NULL)
    {
      /* Right now we can only offer linear-search if the user wants
       * to know about any tag toggle at all.
       */
      return _btk_text_line_previous (line);
    }

  /* Return same-node line, if any. */
  prev = prev_line_under_node (line->parent, line);
  if (prev)
    return prev;

  info = btk_text_btree_get_existing_tag_info (tree, tag);
  if (info == NULL)
    return NULL;

  if (info->tag_root == NULL)
    return NULL;

  if (info->tag_root == line->parent)
    return NULL; /* we were at the first line under the tag root */

  /* Are we below the tag root */
  node = line->parent;
  below_tag_root = FALSE;
  while (node != NULL)
    {
      if (node == info->tag_root)
        {
          below_tag_root = TRUE;
          break;
        }

      node = node->parent;
    }

  if (below_tag_root)
    {
      /* Look for a previous node under this tag root that has our
       * tag.
       */

      /* this assertion holds because line->parent is not the
       * tag root, we are below the tag root, and the tag
       * root exists.
       */
      g_assert (line->parent->parent != NULL);

      line_ancestor = line->parent;
      line_ancestor_parent = line->parent->parent;

      while (line_ancestor != info->tag_root)
        {
          GSList *child_nodes = NULL;
          GSList *tmp;

          /* Create reverse-order list of nodes before
           * line_ancestor
           */
          if (line_ancestor_parent != NULL)
	    node = line_ancestor_parent->children.node;
	  else
	    node = line_ancestor;

          while (node != line_ancestor && node != NULL)
            {
              child_nodes = b_slist_prepend (child_nodes, node);

              node = node->next;
            }

          /* Try to find a node with our tag on it in the list */
          tmp = child_nodes;
          while (tmp != NULL)
            {
              BtkTextBTreeNode *this_node = tmp->data;

              g_assert (this_node != line_ancestor);

              if (btk_text_btree_node_has_tag (this_node, tag))
                {
                  found_node = this_node;
                  b_slist_free (child_nodes);
                  goto found;
                }

              tmp = b_slist_next (tmp);
            }

          b_slist_free (child_nodes);

          /* Didn't find anything on this level; go up one level. */
          line_ancestor = line_ancestor_parent;
          line_ancestor_parent = line_ancestor->parent;
        }

      /* No dice. */
      return NULL;
    }
  else
    {
      bint ordering;

      ordering = node_compare (line->parent, info->tag_root);

      if (ordering < 0)
        {
          /* Tag root is ahead of us, so no more lines
           * with this tag.
           */
          return NULL;
        }
      else
        {
          /* Tag root is after us, so grab last tagged
           * line underneath the tag root.
           */
          found_node = info->tag_root;
          goto found;
        }

      g_assert_not_reached ();
    }

 found:

  g_assert (found_node != NULL);

  /* We have to find the last sub-node of this node that contains
   * the target tag.
   */
  node = found_node;

  while (node->level > 0)
    {
      GSList *child_nodes = NULL;
      GSList *iter;
      g_assert (node != NULL); /* If this fails, it likely means an
                                  incorrect tag summary led us on a
                                  wild goose chase down this branch of
                                  the tree. */

      node = node->children.node;
      while (node != NULL)
        {
          child_nodes = b_slist_prepend (child_nodes, node);
          node = node->next;
        }

      node = NULL; /* detect failure to find a child node. */

      iter = child_nodes;
      while (iter != NULL)
        {
          if (btk_text_btree_node_has_tag (iter->data, tag))
            {
              /* recurse into this node. */
              node = iter->data;
              break;
            }

          iter = b_slist_next (iter);
        }

      b_slist_free (child_nodes);

      g_assert (node != NULL);
    }

  g_assert (node != NULL);
  g_assert (node->level == 0);

  /* this assertion is correct, but slow. */
  /* g_assert (node_compare (node, line->parent) < 0); */

  /* Return last line in this node. */

  prev = node->children.line;
  while (prev->next)
    prev = prev->next;

  return prev;
}

/*
 * Non-public function implementations
 */

static void
summary_list_destroy (Summary *summary)
{
  g_slice_free_chain (Summary, summary, next);
}

static BtkTextLine*
get_last_line (BtkTextBTree *tree)
{
  if (tree->last_line_stamp != tree->chars_changed_stamp)
    {
      bint n_lines;
      BtkTextLine *line;
      bint real_line;

      n_lines = _btk_text_btree_line_count (tree);

      g_assert (n_lines >= 1); /* num_lines doesn't return bogus last line. */

      line = _btk_text_btree_get_line (tree, n_lines, &real_line);

      tree->last_line_stamp = tree->chars_changed_stamp;
      tree->last_line = line;
    }

  return tree->last_line;
}

/*
 * Lines
 */

static BtkTextLine*
btk_text_line_new (void)
{
  BtkTextLine *line;

  line = g_new0(BtkTextLine, 1);
  line->dir_strong = BANGO_DIRECTION_NEUTRAL;
  line->dir_propagated_forward = BANGO_DIRECTION_NEUTRAL;
  line->dir_propagated_back = BANGO_DIRECTION_NEUTRAL;

  return line;
}

static void
btk_text_line_destroy (BtkTextBTree *tree, BtkTextLine *line)
{
  BtkTextLineData *ld;
  BtkTextLineData *next;

  g_return_if_fail (line != NULL);

  ld = line->views;
  while (ld != NULL)
    {
      BTreeView *view;

      view = btk_text_btree_get_view (tree, ld->view_id);

      g_assert (view != NULL);

      next = ld->next;
      btk_text_layout_free_line_data (view->layout, line, ld);

      ld = next;
    }

  g_free (line);
}

static void
btk_text_line_set_parent (BtkTextLine *line,
                          BtkTextBTreeNode *node)
{
  if (line->parent == node)
    return;
  line->parent = node;
  btk_text_btree_node_invalidate_upward (node, NULL);
}

static void
cleanup_line (BtkTextLine *line)
{
  BtkTextLineSegment *seg, **prev_p;
  bboolean changed;

  /*
   * Make a pass over all of the segments in the line, giving each
   * a chance to clean itself up.  This could potentially change
   * the structure of the line, e.g. by merging two segments
   * together or having two segments cancel themselves;  if so,
   * then repeat the whole process again, since the first structure
   * change might make other structure changes possible.  Repeat
   * until eventually there are no changes.
   */

  changed = TRUE;
  while (changed)
    {
      changed = FALSE;
      prev_p = &line->segments;
      for (seg = *prev_p; seg != NULL; seg = *prev_p)
        {
          if (seg->type->cleanupFunc != NULL)
            {
              *prev_p = (*seg->type->cleanupFunc)(seg, line);
              if (seg != *prev_p)
		{
		  changed = TRUE;
		  continue;
		}
            }

	  prev_p = &(*prev_p)->next;
        }
    }
}

/*
 * Nodes
 */

static NodeData*
node_data_new (bpointer view_id)
{
  NodeData *nd;
  
  nd = g_slice_new (NodeData);

  nd->view_id = view_id;
  nd->next = NULL;
  nd->width = 0;
  nd->height = 0;
  nd->valid = FALSE;

  return nd;
}

static void
node_data_destroy (NodeData *nd)
{
  g_slice_free (NodeData, nd);
}

static void
node_data_list_destroy (NodeData *nd)
{
  g_slice_free_chain (NodeData, nd, next);
}

static NodeData*
node_data_find (NodeData *nd, 
		bpointer  view_id)
{
  while (nd != NULL)
    {
      if (nd->view_id == view_id)
        break;
      nd = nd->next;
    }
  return nd;
}

static void
summary_destroy (Summary *summary)
{
  /* Fill with error-triggering garbage */
  summary->info = (void*)0x1;
  summary->toggle_count = 567;
  summary->next = (void*)0x1;
  g_slice_free (Summary, summary);
}

static BtkTextBTreeNode*
btk_text_btree_node_new (void)
{
  BtkTextBTreeNode *node;

  node = g_new (BtkTextBTreeNode, 1);

  node->node_data = NULL;

  return node;
}

static void
btk_text_btree_node_adjust_toggle_count (BtkTextBTreeNode  *node,
                                         BtkTextTagInfo  *info,
                                         bint adjust)
{
  Summary *summary;

  summary = node->summary;
  while (summary != NULL)
    {
      if (summary->info == info)
        {
          summary->toggle_count += adjust;
          break;
        }

      summary = summary->next;
    }

  if (summary == NULL)
    {
      /* didn't find a summary for our tag. */
      g_return_if_fail (adjust > 0);
      summary = g_slice_new (Summary);
      summary->info = info;
      summary->toggle_count = adjust;
      summary->next = node->summary;
      node->summary = summary;
    }
}

/* Note that the tag root and above do not have summaries
   for the tag; only nodes below the tag root have
   the summaries. */
static bboolean
btk_text_btree_node_has_tag (BtkTextBTreeNode *node, BtkTextTag *tag)
{
  Summary *summary;

  summary = node->summary;
  while (summary != NULL)
    {
      if (tag == NULL ||
          summary->info->tag == tag)
        return TRUE;

      summary = summary->next;
    }

  return FALSE;
}

/* Add node and all children to the damage rebunnyion. */
#if 0
static void
btk_text_btree_node_invalidate_downward (BtkTextBTreeNode *node)
{
  NodeData *nd;

  nd = node->node_data;
  while (nd != NULL)
    {
      nd->valid = FALSE;
      nd = nd->next;
    }

  if (node->level == 0)
    {
      BtkTextLine *line;

      line = node->children.line;
      while (line != NULL)
        {
          BtkTextLineData *ld;

          ld = line->views;
          while (ld != NULL)
            {
              ld->valid = FALSE;
              ld = ld->next;
            }

          line = line->next;
        }
    }
  else
    {
      BtkTextBTreeNode *child;

      child = node->children.node;

      while (child != NULL)
        {
          btk_text_btree_node_invalidate_downward (child);

          child = child->next;
        }
    }
}
#endif

static void
btk_text_btree_node_invalidate_upward (BtkTextBTreeNode *node, bpointer view_id)
{
  BtkTextBTreeNode *iter;

  iter = node;
  while (iter != NULL)
    {
      NodeData *nd;

      if (view_id)
        {
          nd = node_data_find (iter->node_data, view_id);

          if (nd == NULL || !nd->valid)
            break; /* Once a node is invalid, we know its parents are as well. */

          nd->valid = FALSE;
        }
      else
        {
          bboolean should_continue = FALSE;

          nd = iter->node_data;
          while (nd != NULL)
            {
              if (nd->valid)
                {
                  should_continue = TRUE;
                  nd->valid = FALSE;
                }

              nd = nd->next;
            }

          if (!should_continue)
            break; /* This node was totally invalidated, so are its
                      parents */
        }

      iter = iter->parent;
    }
}


/**
 * _btk_text_btree_is_valid:
 * @tree: a #BtkTextBTree
 * @view_id: ID for the view
 *
 * Check to see if the entire #BtkTextBTree is valid or not for
 * the given view.
 *
 * Return value: %TRUE if the entire #BtkTextBTree is valid
 **/
bboolean
_btk_text_btree_is_valid (BtkTextBTree *tree,
                         bpointer      view_id)
{
  NodeData *nd;
  g_return_val_if_fail (tree != NULL, FALSE);

  nd = node_data_find (tree->root_node->node_data, view_id);
  return (nd && nd->valid);
}

typedef struct _ValidateState ValidateState;

struct _ValidateState
{
  bint remaining_pixels;
  bboolean in_validation;
  bint y;
  bint old_height;
  bint new_height;
};

static void
btk_text_btree_node_validate (BTreeView         *view,
                              BtkTextBTreeNode  *node,
                              bpointer           view_id,
                              ValidateState     *state)
{
  bint node_valid = TRUE;
  bint node_width = 0;
  bint node_height = 0;

  NodeData *nd = btk_text_btree_node_ensure_data (node, view_id);
  g_return_if_fail (!nd->valid);

  if (node->level == 0)
    {
      BtkTextLine *line = node->children.line;
      BtkTextLineData *ld;

      /* Iterate over leading valid lines */
      while (line != NULL)
        {
          ld = _btk_text_line_get_data (line, view_id);

          if (!ld || !ld->valid)
            break;
          else if (state->in_validation)
            {
              state->in_validation = FALSE;
              return;
            }
          else
            {
              state->y += ld->height;
              node_width = MAX (ld->width, node_width);
              node_height += ld->height;
            }

          line = line->next;
        }

      state->in_validation = TRUE;

      /* Iterate over invalid lines */
      while (line != NULL)
        {
          ld = _btk_text_line_get_data (line, view_id);

          if (ld && ld->valid)
            break;
          else
            {
              if (ld)
                state->old_height += ld->height;
              ld = btk_text_layout_wrap (view->layout, line, ld);
              state->new_height += ld->height;

              node_width = MAX (ld->width, node_width);
              node_height += ld->height;

              state->remaining_pixels -= ld->height;
              if (state->remaining_pixels <= 0)
                {
                  line = line->next;
                  break;
                }
            }

          line = line->next;
        }

      /* Iterate over the remaining lines */
      while (line != NULL)
        {
          ld = _btk_text_line_get_data (line, view_id);
          state->in_validation = FALSE;

          if (!ld || !ld->valid)
            node_valid = FALSE;

          if (ld)
            {
              node_width = MAX (ld->width, node_width);
              node_height += ld->height;
            }

          line = line->next;
        }
    }
  else
    {
      BtkTextBTreeNode *child;
      NodeData *child_nd;

      child = node->children.node;

      /* Iterate over leading valid nodes */
      while (child)
        {
          child_nd = btk_text_btree_node_ensure_data (child, view_id);

          if (!child_nd->valid)
            break;
          else if (state->in_validation)
            {
              state->in_validation = FALSE;
              return;
            }
          else
            {
              state->y += child_nd->height;
              node_width = MAX (node_width, child_nd->width);
              node_height += child_nd->height;
            }

          child = child->next;
        }

      /* Iterate over invalid nodes */
      while (child)
        {
          child_nd = btk_text_btree_node_ensure_data (child, view_id);

          if (child_nd->valid)
            break;
          else
            {
              btk_text_btree_node_validate (view, child, view_id, state);

              if (!child_nd->valid)
                node_valid = FALSE;
              node_width = MAX (node_width, child_nd->width);
              node_height += child_nd->height;

              if (!state->in_validation || state->remaining_pixels <= 0)
                {
                  child = child->next;
                  break;
                }
            }

          child = child->next;
        }

      /* Iterate over the remaining lines */
      while (child)
        {
          child_nd = btk_text_btree_node_ensure_data (child, view_id);
          state->in_validation = FALSE;

          if (!child_nd->valid)
            node_valid = FALSE;

          node_width = MAX (child_nd->width, node_width);
          node_height += child_nd->height;

          child = child->next;
        }
    }

  nd->width = node_width;
  nd->height = node_height;
  nd->valid = node_valid;
}

/**
 * _btk_text_btree_validate:
 * @tree: a #BtkTextBTree
 * @view_id: view id
 * @max_pixels: the maximum number of pixels to validate. (No more
 *              than one paragraph beyond this limit will be validated)
 * @y: location to store starting y coordinate of validated rebunnyion
 * @old_height: location to store old height of validated rebunnyion
 * @new_height: location to store new height of validated rebunnyion
 *
 * Validate a single contiguous invalid rebunnyion of a #BtkTextBTree for
 * a given view.
 *
 * Return value: %TRUE if a rebunnyion has been validated, %FALSE if the
 * entire tree was already valid.
 **/
bboolean
_btk_text_btree_validate (BtkTextBTree *tree,
                         bpointer      view_id,
                         bint          max_pixels,
                         bint         *y,
                         bint         *old_height,
                         bint         *new_height)
{
  BTreeView *view;

  g_return_val_if_fail (tree != NULL, FALSE);

  view = btk_text_btree_get_view (tree, view_id);
  g_return_val_if_fail (view != NULL, FALSE);

  if (!_btk_text_btree_is_valid (tree, view_id))
    {
      ValidateState state;

      state.remaining_pixels = max_pixels;
      state.in_validation = FALSE;
      state.y = 0;
      state.old_height = 0;
      state.new_height = 0;

      btk_text_btree_node_validate (view,
                                    tree->root_node,
                                    view_id, &state);

      if (y)
        *y = state.y;
      if (old_height)
        *old_height = state.old_height;
      if (new_height)
        *new_height = state.new_height;

      if (btk_debug_flags & BTK_DEBUG_TEXT)
        _btk_text_btree_check (tree);

      return TRUE;
    }
  else
    return FALSE;
}

static void
btk_text_btree_node_compute_view_aggregates (BtkTextBTreeNode *node,
                                             bpointer          view_id,
                                             bint             *width_out,
                                             bint             *height_out,
                                             bboolean         *valid_out)
{
  bint width = 0;
  bint height = 0;
  bboolean valid = TRUE;

  if (node->level == 0)
    {
      BtkTextLine *line = node->children.line;

      while (line != NULL)
        {
          BtkTextLineData *ld = _btk_text_line_get_data (line, view_id);

          if (!ld || !ld->valid)
            valid = FALSE;

          if (ld)
            {
              width = MAX (ld->width, width);
              height += ld->height;
            }

          line = line->next;
        }
    }
  else
    {
      BtkTextBTreeNode *child = node->children.node;

      while (child)
        {
          NodeData *child_nd = node_data_find (child->node_data, view_id);

          if (!child_nd || !child_nd->valid)
            valid = FALSE;

          if (child_nd)
            {
              width = MAX (child_nd->width, width);
              height += child_nd->height;
            }

          child = child->next;
        }
    }

  *width_out = width;
  *height_out = height;
  *valid_out = valid;
}


/* Recompute the validity and size of the view data for a given
 * view at this node from the immediate children of the node
 */
static NodeData *
btk_text_btree_node_check_valid (BtkTextBTreeNode *node,
                                 bpointer          view_id)
{
  NodeData *nd = btk_text_btree_node_ensure_data (node, view_id);
  bboolean valid;
  bint width;
  bint height;

  btk_text_btree_node_compute_view_aggregates (node, view_id,
                                               &width, &height, &valid);
  nd->width = width;
  nd->height = height;
  nd->valid = valid;

  return nd;
}

static void
btk_text_btree_node_check_valid_upward (BtkTextBTreeNode *node,
                                        bpointer          view_id)
{
  while (node)
    {
      btk_text_btree_node_check_valid (node, view_id);
      node = node->parent;
    }
}

static NodeData *
btk_text_btree_node_check_valid_downward (BtkTextBTreeNode *node,
                                          bpointer          view_id)
{
  if (node->level == 0)
    {
      return btk_text_btree_node_check_valid (node, view_id);
    }
  else
    {
      BtkTextBTreeNode *child = node->children.node;

      NodeData *nd = btk_text_btree_node_ensure_data (node, view_id);

      nd->valid = TRUE;
      nd->width = 0;
      nd->height = 0;

      while (child)
        {
          NodeData *child_nd = btk_text_btree_node_check_valid_downward (child, view_id);

          if (!child_nd->valid)
            nd->valid = FALSE;
          nd->width = MAX (child_nd->width, nd->width);
          nd->height += child_nd->height;

          child = child->next;
        }
      return nd;
    }
}



/**
 * _btk_text_btree_validate_line:
 * @tree: a #BtkTextBTree
 * @line: line to validate
 * @view_id: view ID for the view to validate
 *
 * Revalidate a single line of the btree for the given view, propagate
 * results up through the entire tree.
 **/
void
_btk_text_btree_validate_line (BtkTextBTree     *tree,
                               BtkTextLine      *line,
                               bpointer          view_id)
{
  BtkTextLineData *ld;
  BTreeView *view;

  g_return_if_fail (tree != NULL);
  g_return_if_fail (line != NULL);

  view = btk_text_btree_get_view (tree, view_id);
  g_return_if_fail (view != NULL);
  
  ld = _btk_text_line_get_data (line, view_id);
  if (!ld || !ld->valid)
    {
      ld = btk_text_layout_wrap (view->layout, line, ld);
      
      btk_text_btree_node_check_valid_upward (line->parent, view_id);
    }
}

static void
btk_text_btree_node_remove_view (BTreeView *view, BtkTextBTreeNode *node, bpointer view_id)
{
  if (node->level == 0)
    {
      BtkTextLine *line;

      line = node->children.line;
      while (line != NULL)
        {
          BtkTextLineData *ld;

          ld = _btk_text_line_remove_data (line, view_id);

          if (ld)
            btk_text_layout_free_line_data (view->layout, line, ld);

          line = line->next;
        }
    }
  else
    {
      BtkTextBTreeNode *child;

      child = node->children.node;

      while (child != NULL)
        {
          /* recurse */
          btk_text_btree_node_remove_view (view, child, view_id);

          child = child->next;
        }
    }

  btk_text_btree_node_remove_data (node, view_id);
}

static void
btk_text_btree_node_destroy (BtkTextBTree *tree, BtkTextBTreeNode *node)
{
  if (node->level == 0)
    {
      BtkTextLine *line;
      BtkTextLineSegment *seg;

      while (node->children.line != NULL)
        {
          line = node->children.line;
          node->children.line = line->next;
          while (line->segments != NULL)
            {
              seg = line->segments;
              line->segments = seg->next;

              (*seg->type->deleteFunc) (seg, line, TRUE);
            }
          btk_text_line_destroy (tree, line);
        }
    }
  else
    {
      BtkTextBTreeNode *childPtr;

      while (node->children.node != NULL)
        {
          childPtr = node->children.node;
          node->children.node = childPtr->next;
          btk_text_btree_node_destroy (tree, childPtr);
        }
    }

  btk_text_btree_node_free_empty (tree, node);
}

static void
btk_text_btree_node_free_empty (BtkTextBTree *tree,
                                BtkTextBTreeNode *node)
{
  g_return_if_fail ((node->level > 0 && node->children.node == NULL) ||
                    (node->level == 0 && node->children.line == NULL));

  summary_list_destroy (node->summary);
  node_data_list_destroy (node->node_data);
  g_free (node);
}

static NodeData*
btk_text_btree_node_ensure_data (BtkTextBTreeNode *node, bpointer view_id)
{
  NodeData *nd;

  nd = node->node_data;
  while (nd != NULL)
    {
      if (nd->view_id == view_id)
        break;

      nd = nd->next;
    }

  if (nd == NULL)
    {
      nd = node_data_new (view_id);
      
      if (node->node_data)
        nd->next = node->node_data;
      
      node->node_data = nd;
    }

  return nd;
}

static void
btk_text_btree_node_remove_data (BtkTextBTreeNode *node, bpointer view_id)
{
  NodeData *nd;
  NodeData *prev;

  prev = NULL;
  nd = node->node_data;
  while (nd != NULL)
    {
      if (nd->view_id == view_id)
        break;

      prev = nd;
      nd = nd->next;
    }

  if (nd == NULL)
    return;

  if (prev != NULL)
    prev->next = nd->next;

  if (node->node_data == nd)
    node->node_data = nd->next;

  nd->next = NULL;

  node_data_destroy (nd);
}

static void
btk_text_btree_node_get_size (BtkTextBTreeNode *node, bpointer view_id,
                              bint *width, bint *height)
{
  NodeData *nd;

  g_return_if_fail (width != NULL);
  g_return_if_fail (height != NULL);

  nd = btk_text_btree_node_ensure_data (node, view_id);

  if (width)
    *width = nd->width;
  if (height)
    *height = nd->height;
}

/* Find the closest common ancestor of the two nodes. FIXME: The interface
 * here isn't quite right, since for a lot of operations we want to
 * know which children of the common parent correspond to the two nodes
 * (e.g., when computing the order of two iters)
 */
static BtkTextBTreeNode *
btk_text_btree_node_common_parent (BtkTextBTreeNode *node1,
                                   BtkTextBTreeNode *node2)
{
  while (node1->level < node2->level)
    node1 = node1->parent;
  while (node2->level < node1->level)
    node2 = node2->parent;
  while (node1 != node2)
    {
      node1 = node1->parent;
      node2 = node2->parent;
    }

  return node1;
}

/*
 * BTree
 */

static BTreeView*
btk_text_btree_get_view (BtkTextBTree *tree, bpointer view_id)
{
  BTreeView *view;

  view = tree->views;
  while (view != NULL)
    {
      if (view->view_id == view_id)
        break;
      view = view->next;
    }

  return view;
}

static void
get_tree_bounds (BtkTextBTree *tree,
                 BtkTextIter *start,
                 BtkTextIter *end)
{
  _btk_text_btree_get_iter_at_line_char (tree, start, 0, 0);
  _btk_text_btree_get_end_iter (tree, end);
}

static void
tag_changed_cb (BtkTextTagTable *table,
                BtkTextTag      *tag,
                bboolean         size_changed,
                BtkTextBTree    *tree)
{
  if (size_changed)
    {
      /* We need to queue a relayout on all rebunnyions that are tagged with
       * this tag.
       */

      BtkTextIter start;
      BtkTextIter end;

      if (_btk_text_btree_get_iter_at_first_toggle (tree, &start, tag))
        {
          /* Must be a last toggle if there was a first one. */
          _btk_text_btree_get_iter_at_last_toggle (tree, &end, tag);
          DV (g_print ("invalidating due to tag change (%s)\n", B_STRLOC));
          _btk_text_btree_invalidate_rebunnyion (tree, &start, &end, FALSE);

        }
    }
  else
    {
      /* We only need to queue a redraw, not a relayout */
      BTreeView *view;

      view = tree->views;

      while (view != NULL)
        {
          bint width, height;

          _btk_text_btree_get_view_size (tree, view->view_id, &width, &height);
          btk_text_layout_changed (view->layout, 0, height, height);

          view = view->next;
        }
    }
}

void
_btk_text_btree_notify_will_remove_tag (BtkTextBTree    *tree,
                                        BtkTextTag      *tag)
{
  /* Remove the tag from the tree */

  BtkTextIter start;
  BtkTextIter end;

  get_tree_bounds (tree, &start, &end);

  _btk_text_btree_tag (&start, &end, tag, FALSE);
  btk_text_btree_remove_tag_info (tree, tag);
}


/* Rebalance the out-of-whack node "node" */
static void
btk_text_btree_rebalance (BtkTextBTree *tree,
                          BtkTextBTreeNode *node)
{
  /*
   * Loop over the entire ancestral chain of the BtkTextBTreeNode, working
   * up through the tree one BtkTextBTreeNode at a time until the root
   * BtkTextBTreeNode has been processed.
   */

  while (node != NULL)
    {
      BtkTextBTreeNode *new_node, *child;
      BtkTextLine *line;
      int i;

      /*
       * Check to see if the BtkTextBTreeNode has too many children.  If it does,
       * then split off all but the first MIN_CHILDREN into a separate
       * BtkTextBTreeNode following the original one.  Then repeat until the
       * BtkTextBTreeNode has a decent size.
       */

      if (node->num_children > MAX_CHILDREN)
        {
          while (1)
            {
              /*
               * If the BtkTextBTreeNode being split is the root
               * BtkTextBTreeNode, then make a new root BtkTextBTreeNode above
               * it first.
               */

              if (node->parent == NULL)
                {
                  new_node = btk_text_btree_node_new ();
                  new_node->parent = NULL;
                  new_node->next = NULL;
                  new_node->summary = NULL;
                  new_node->level = node->level + 1;
                  new_node->children.node = node;
                  recompute_node_counts (tree, new_node);
                  tree->root_node = new_node;
                }
              new_node = btk_text_btree_node_new ();
              new_node->parent = node->parent;
              new_node->next = node->next;
              node->next = new_node;
              new_node->summary = NULL;
              new_node->level = node->level;
              new_node->num_children = node->num_children - MIN_CHILDREN;
              if (node->level == 0)
                {
                  for (i = MIN_CHILDREN-1,
                         line = node->children.line;
                       i > 0; i--, line = line->next)
                    {
                      /* Empty loop body. */
                    }
                  new_node->children.line = line->next;
                  line->next = NULL;
                }
              else
                {
                  for (i = MIN_CHILDREN-1,
                         child = node->children.node;
                       i > 0; i--, child = child->next)
                    {
                      /* Empty loop body. */
                    }
                  new_node->children.node = child->next;
                  child->next = NULL;
                }
              recompute_node_counts (tree, node);
              node->parent->num_children++;
              node = new_node;
              if (node->num_children <= MAX_CHILDREN)
                {
                  recompute_node_counts (tree, node);
                  break;
                }
            }
        }

      while (node->num_children < MIN_CHILDREN)
        {
          BtkTextBTreeNode *other;
          BtkTextBTreeNode *halfwaynode = NULL; /* Initialization needed only */
          BtkTextLine *halfwayline = NULL; /* to prevent cc warnings. */
          int total_children, first_children, i;

          /*
           * Too few children for this BtkTextBTreeNode.  If this is the root then,
           * it's OK for it to have less than MIN_CHILDREN children
           * as long as it's got at least two.  If it has only one
           * (and isn't at level 0), then chop the root BtkTextBTreeNode out of
           * the tree and use its child as the new root.
           */

          if (node->parent == NULL)
            {
              if ((node->num_children == 1) && (node->level > 0))
                {
                  tree->root_node = node->children.node;
                  tree->root_node->parent = NULL;

                  node->children.node = NULL;
                  btk_text_btree_node_free_empty (tree, node);
                }
              return;
            }

          /*
           * Not the root.  Make sure that there are siblings to
           * balance with.
           */

          if (node->parent->num_children < 2)
            {
              btk_text_btree_rebalance (tree, node->parent);
              continue;
            }

          /*
           * Find a sibling neighbor to borrow from, and arrange for
           * node to be the earlier of the pair.
           */

          if (node->next == NULL)
            {
              for (other = node->parent->children.node;
                   other->next != node;
                   other = other->next)
                {
                  /* Empty loop body. */
                }
              node = other;
            }
          other = node->next;

          /*
           * We're going to either merge the two siblings together
           * into one BtkTextBTreeNode or redivide the children among them to
           * balance their loads.  As preparation, join their two
           * child lists into a single list and remember the half-way
           * point in the list.
           */

          total_children = node->num_children + other->num_children;
          first_children = total_children/2;
          if (node->children.node == NULL)
            {
              node->children = other->children;
              other->children.node = NULL;
              other->children.line = NULL;
            }
          if (node->level == 0)
            {
              BtkTextLine *line;

              for (line = node->children.line, i = 1;
                   line->next != NULL;
                   line = line->next, i++)
                {
                  if (i == first_children)
                    {
                      halfwayline = line;
                    }
                }
              line->next = other->children.line;
              while (i <= first_children)
                {
                  halfwayline = line;
                  line = line->next;
                  i++;
                }
            }
          else
            {
              BtkTextBTreeNode *child;

              for (child = node->children.node, i = 1;
                   child->next != NULL;
                   child = child->next, i++)
                {
                  if (i <= first_children)
                    {
                      if (i == first_children)
                        {
                          halfwaynode = child;
                        }
                    }
                }
              child->next = other->children.node;
              while (i <= first_children)
                {
                  halfwaynode = child;
                  child = child->next;
                  i++;
                }
            }

          /*
           * If the two siblings can simply be merged together, do it.
           */

          if (total_children <= MAX_CHILDREN)
            {
              recompute_node_counts (tree, node);
              node->next = other->next;
              node->parent->num_children--;

              other->children.node = NULL;
              other->children.line = NULL;
              btk_text_btree_node_free_empty (tree, other);
              continue;
            }

          /*
           * The siblings can't be merged, so just divide their
           * children evenly between them.
           */

          if (node->level == 0)
            {
              other->children.line = halfwayline->next;
              halfwayline->next = NULL;
            }
          else
            {
              other->children.node = halfwaynode->next;
              halfwaynode->next = NULL;
            }

          recompute_node_counts (tree, node);
          recompute_node_counts (tree, other);
        }

      node = node->parent;
    }
}

static void
post_insert_fixup (BtkTextBTree *tree,
                   BtkTextLine *line,
                   bint line_count_delta,
                   bint char_count_delta)

{
  BtkTextBTreeNode *node;

  /*
   * Increment the line counts in all the parent BtkTextBTreeNodes of the insertion
   * point, then rebalance the tree if necessary.
   */

  for (node = line->parent ; node != NULL;
       node = node->parent)
    {
      node->num_lines += line_count_delta;
      node->num_chars += char_count_delta;
    }
  node = line->parent;
  node->num_children += line_count_delta;

  if (node->num_children > MAX_CHILDREN)
    {
      btk_text_btree_rebalance (tree, node);
    }

  if (btk_debug_flags & BTK_DEBUG_TEXT)
    _btk_text_btree_check (tree);
}

static BtkTextTagInfo*
btk_text_btree_get_existing_tag_info (BtkTextBTree *tree,
                                      BtkTextTag   *tag)
{
  BtkTextTagInfo *info;
  GSList *list;


  list = tree->tag_infos;
  while (list != NULL)
    {
      info = list->data;
      if (info->tag == tag)
        return info;

      list = b_slist_next (list);
    }

  return NULL;
}

static BtkTextTagInfo*
btk_text_btree_get_tag_info (BtkTextBTree *tree,
                             BtkTextTag   *tag)
{
  BtkTextTagInfo *info;

  info = btk_text_btree_get_existing_tag_info (tree, tag);

  if (info == NULL)
    {
      /* didn't find it, create. */

      info = g_slice_new (BtkTextTagInfo);

      info->tag = tag;
      g_object_ref (tag);
      info->tag_root = NULL;
      info->toggle_count = 0;

      tree->tag_infos = b_slist_prepend (tree->tag_infos, info);

#if 0
      g_print ("Created tag info %p for tag %s(%p)\n",
               info, info->tag->name ? info->tag->name : "anon",
               info->tag);
#endif
    }

  return info;
}

static void
btk_text_btree_remove_tag_info (BtkTextBTree *tree,
                                BtkTextTag   *tag)
{
  BtkTextTagInfo *info;
  GSList *list;
  GSList *prev;

  prev = NULL;
  list = tree->tag_infos;
  while (list != NULL)
    {
      info = list->data;
      if (info->tag == tag)
        {
#if 0
          g_print ("Removing tag info %p for tag %s(%p)\n",
                   info, info->tag->name ? info->tag->name : "anon",
                   info->tag);
#endif
          
          if (prev != NULL)
            {
              prev->next = list->next;
            }
          else
            {
              tree->tag_infos = list->next;
            }
          list->next = NULL;
          b_slist_free (list);

          g_object_unref (info->tag);

          g_slice_free (BtkTextTagInfo, info);
          return;
        }

      prev = list;
      list = b_slist_next (list);
    }
}

static void
recompute_level_zero_counts (BtkTextBTreeNode *node)
{
  BtkTextLine *line;
  BtkTextLineSegment *seg;

  g_assert (node->level == 0);

  line = node->children.line;
  while (line != NULL)
    {
      node->num_children++;
      node->num_lines++;

      if (line->parent != node)
        btk_text_line_set_parent (line, node);

      seg = line->segments;
      while (seg != NULL)
        {

          node->num_chars += seg->char_count;

          if (((seg->type != &btk_text_toggle_on_type)
               && (seg->type != &btk_text_toggle_off_type))
              || !(seg->body.toggle.inNodeCounts))
            {
              ; /* nothing */
            }
          else
            {
              BtkTextTagInfo *info;

              info = seg->body.toggle.info;

              btk_text_btree_node_adjust_toggle_count (node, info, 1);
            }

          seg = seg->next;
        }

      line = line->next;
    }
}

static void
recompute_level_nonzero_counts (BtkTextBTreeNode *node)
{
  Summary *summary;
  BtkTextBTreeNode *child;

  g_assert (node->level > 0);

  child = node->children.node;
  while (child != NULL)
    {
      node->num_children += 1;
      node->num_lines += child->num_lines;
      node->num_chars += child->num_chars;

      if (child->parent != node)
        {
          child->parent = node;
          btk_text_btree_node_invalidate_upward (node, NULL);
        }

      summary = child->summary;
      while (summary != NULL)
        {
          btk_text_btree_node_adjust_toggle_count (node,
                                                   summary->info,
                                                   summary->toggle_count);

          summary = summary->next;
        }

      child = child->next;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * recompute_node_counts --
 *
 *      This procedure is called to recompute all the counts in a BtkTextBTreeNode
 *      (tags, child information, etc.) by scanning the information in
 *      its descendants.  This procedure is called during rebalancing
 *      when a BtkTextBTreeNode's child structure has changed.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The tag counts for node are modified to reflect its current
 *      child structure, as are its num_children, num_lines, num_chars fields.
 *      Also, all of the childrens' parent fields are made to point
 *      to node.
 *
 *----------------------------------------------------------------------
 */

static void
recompute_node_counts (BtkTextBTree *tree, BtkTextBTreeNode *node)
{
  BTreeView *view;
  Summary *summary, *summary2;

  /*
   * Zero out all the existing counts for the BtkTextBTreeNode, but don't delete
   * the existing Summary records (most of them will probably be reused).
   */

  summary = node->summary;
  while (summary != NULL)
    {
      summary->toggle_count = 0;
      summary = summary->next;
    }

  node->num_children = 0;
  node->num_lines = 0;
  node->num_chars = 0;

  /*
   * Scan through the children, adding the childrens' tag counts into
   * the BtkTextBTreeNode's tag counts and adding new Summary structures if
   * necessary.
   */

  if (node->level == 0)
    recompute_level_zero_counts (node);
  else
    recompute_level_nonzero_counts (node);

  view = tree->views;
  while (view)
    {
      btk_text_btree_node_check_valid (node, view->view_id);
      view = view->next;
    }
  
  /*
   * Scan through the BtkTextBTreeNode's tag records again and delete any Summary
   * records that still have a zero count, or that have all the toggles.
   * The BtkTextBTreeNode with the children that account for all the tags toggles
   * have no summary information, and they become the tag_root for the tag.
   */

  summary2 = NULL;
  for (summary = node->summary; summary != NULL; )
    {
      if (summary->toggle_count > 0 &&
          summary->toggle_count < summary->info->toggle_count)
        {
          if (node->level == summary->info->tag_root->level)
            {
              /*
               * The tag's root BtkTextBTreeNode split and some toggles left.
               * The tag root must move up a level.
               */
              summary->info->tag_root = node->parent;
            }
          summary2 = summary;
          summary = summary->next;
          continue;
        }
      if (summary->toggle_count == summary->info->toggle_count)
        {
          /*
           * A BtkTextBTreeNode merge has collected all the toggles under
           * one BtkTextBTreeNode.  Push the root down to this level.
           */
          summary->info->tag_root = node;
        }
      if (summary2 != NULL)
        {
          summary2->next = summary->next;
          summary_destroy (summary);
          summary = summary2->next;
        }
      else
        {
          node->summary = summary->next;
          summary_destroy (summary);
          summary = node->summary;
        }
    }
}

void
_btk_change_node_toggle_count (BtkTextBTreeNode *node,
                               BtkTextTagInfo   *info,
                               bint              delta) /* may be negative */
{
  Summary *summary, *prevPtr;
  BtkTextBTreeNode *node2Ptr;
  int rootLevel;                        /* Level of original tag root */

  info->toggle_count += delta;

  if (info->tag_root == (BtkTextBTreeNode *) NULL)
    {
      info->tag_root = node;
      return;
    }

  /*
   * Note the level of the existing root for the tag so we can detect
   * if it needs to be moved because of the toggle count change.
   */

  rootLevel = info->tag_root->level;

  /*
   * Iterate over the BtkTextBTreeNode and its ancestors up to the tag root, adjusting
   * summary counts at each BtkTextBTreeNode and moving the tag's root upwards if
   * necessary.
   */

  for ( ; node != info->tag_root; node = node->parent)
    {
      /*
       * See if there's already an entry for this tag for this BtkTextBTreeNode.  If so,
       * perhaps all we have to do is adjust its count.
       */

      for (prevPtr = NULL, summary = node->summary;
           summary != NULL;
           prevPtr = summary, summary = summary->next)
        {
          if (summary->info == info)
            {
              break;
            }
        }
      if (summary != NULL)
        {
          summary->toggle_count += delta;
          if (summary->toggle_count > 0 &&
              summary->toggle_count < info->toggle_count)
            {
              continue;
            }
          if (summary->toggle_count != 0)
            {
              /*
               * Should never find a BtkTextBTreeNode with max toggle count at this
               * point (there shouldn't have been a summary entry in the
               * first place).
               */

              g_error ("%s: bad toggle count (%d) max (%d)",
                       B_STRLOC, summary->toggle_count, info->toggle_count);
            }

          /*
           * Zero toggle count;  must remove this tag from the list.
           */

          if (prevPtr == NULL)
            {
              node->summary = summary->next;
            }
          else
            {
              prevPtr->next = summary->next;
            }
          summary_destroy (summary);
        }
      else
        {
          /*
           * This tag isn't currently in the summary information list.
           */

          if (rootLevel == node->level)
            {

              /*
               * The old tag root is at the same level in the tree as this
               * BtkTextBTreeNode, but it isn't at this BtkTextBTreeNode.  Move the tag root up
               * a level, in the hopes that it will now cover this BtkTextBTreeNode
               * as well as the old root (if not, we'll move it up again
               * the next time through the loop).  To push it up one level
               * we copy the original toggle count into the summary
               * information at the old root and change the root to its
               * parent BtkTextBTreeNode.
               */

              BtkTextBTreeNode *rootnode = info->tag_root;
              summary = g_slice_new (Summary);
              summary->info = info;
              summary->toggle_count = info->toggle_count - delta;
              summary->next = rootnode->summary;
              rootnode->summary = summary;
              rootnode = rootnode->parent;
              rootLevel = rootnode->level;
              info->tag_root = rootnode;
            }
          summary = g_slice_new (Summary);
          summary->info = info;
          summary->toggle_count = delta;
          summary->next = node->summary;
          node->summary = summary;
        }
    }

  /*
   * If we've decremented the toggle count, then it may be necessary
   * to push the tag root down one or more levels.
   */

  if (delta >= 0)
    {
      return;
    }
  if (info->toggle_count == 0)
    {
      info->tag_root = (BtkTextBTreeNode *) NULL;
      return;
    }
  node = info->tag_root;
  while (node->level > 0)
    {
      /*
       * See if a single child BtkTextBTreeNode accounts for all of the tag's
       * toggles.  If so, push the root down one level.
       */

      for (node2Ptr = node->children.node;
           node2Ptr != (BtkTextBTreeNode *)NULL ;
           node2Ptr = node2Ptr->next)
        {
          for (prevPtr = NULL, summary = node2Ptr->summary;
               summary != NULL;
               prevPtr = summary, summary = summary->next)
            {
              if (summary->info == info)
                {
                  break;
                }
            }
          if (summary == NULL)
            {
              continue;
            }
          if (summary->toggle_count != info->toggle_count)
            {
              /*
               * No BtkTextBTreeNode has all toggles, so the root is still valid.
               */

              return;
            }

          /*
           * This BtkTextBTreeNode has all the toggles, so push down the root.
           */

          if (prevPtr == NULL)
            {
              node2Ptr->summary = summary->next;
            }
          else
            {
              prevPtr->next = summary->next;
            }
          summary_destroy (summary);
          info->tag_root = node2Ptr;
          break;
        }
      node = info->tag_root;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * inc_count --
 *
 *      This is a utility procedure used by _btk_text_btree_get_tags.  It
 *      increments the count for a particular tag, adding a new
 *      entry for that tag if there wasn't one previously.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The information at *tagInfoPtr may be modified, and the arrays
 *      may be reallocated to make them larger.
 *
 *----------------------------------------------------------------------
 */

static void
inc_count (BtkTextTag *tag, int inc, TagInfo *tagInfoPtr)
{
  BtkTextTag **tag_p;
  int count;

  for (tag_p = tagInfoPtr->tags, count = tagInfoPtr->numTags;
       count > 0; tag_p++, count--)
    {
      if (*tag_p == tag)
        {
          tagInfoPtr->counts[tagInfoPtr->numTags-count] += inc;
          return;
        }
    }

  /*
   * There isn't currently an entry for this tag, so we have to
   * make a new one.  If the arrays are full, then enlarge the
   * arrays first.
   */

  if (tagInfoPtr->numTags == tagInfoPtr->arraySize)
    {
      BtkTextTag **newTags;
      int *newCounts, newSize;

      newSize = 2*tagInfoPtr->arraySize;
      newTags = (BtkTextTag **) g_malloc ((unsigned)
                                          (newSize*sizeof (BtkTextTag *)));
      memcpy ((void *) newTags, (void *) tagInfoPtr->tags,
              tagInfoPtr->arraySize  *sizeof (BtkTextTag *));
      g_free ((char *) tagInfoPtr->tags);
      tagInfoPtr->tags = newTags;
      newCounts = (int *) g_malloc ((unsigned) (newSize*sizeof (int)));
      memcpy ((void *) newCounts, (void *) tagInfoPtr->counts,
              tagInfoPtr->arraySize  *sizeof (int));
      g_free ((char *) tagInfoPtr->counts);
      tagInfoPtr->counts = newCounts;
      tagInfoPtr->arraySize = newSize;
    }

  tagInfoPtr->tags[tagInfoPtr->numTags] = tag;
  tagInfoPtr->counts[tagInfoPtr->numTags] = inc;
  tagInfoPtr->numTags++;
}

static void
btk_text_btree_link_segment (BtkTextLineSegment *seg,
                             const BtkTextIter *iter)
{
  BtkTextLineSegment *prev;
  BtkTextLine *line;
  BtkTextBTree *tree;

  line = _btk_text_iter_get_text_line (iter);
  tree = _btk_text_iter_get_btree (iter);

  prev = btk_text_line_segment_split (iter);
  if (prev == NULL)
    {
      seg->next = line->segments;
      line->segments = seg;
    }
  else
    {
      seg->next = prev->next;
      prev->next = seg;
    }
  cleanup_line (line);
  segments_changed (tree);

  if (btk_debug_flags & BTK_DEBUG_TEXT)
    _btk_text_btree_check (tree);
}

static void
btk_text_btree_unlink_segment (BtkTextBTree *tree,
                               BtkTextLineSegment *seg,
                               BtkTextLine *line)
{
  BtkTextLineSegment *prev;

  if (line->segments == seg)
    {
      line->segments = seg->next;
    }
  else
    {
      for (prev = line->segments; prev->next != seg;
           prev = prev->next)
        {
          /* Empty loop body. */
        }
      prev->next = seg->next;
    }
  cleanup_line (line);
  segments_changed (tree);
}

/*
 * This is here because it requires BTree internals, it logically
 * belongs in btktextsegment.c
 */


/*
 *--------------------------------------------------------------
 *
 * _btk_toggle_segment_check_func --
 *
 *      This procedure is invoked to perform consistency checks
 *      on toggle segments.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      If a consistency problem is found the procedure g_errors.
 *
 *--------------------------------------------------------------
 */

void
_btk_toggle_segment_check_func (BtkTextLineSegment *segPtr,
                                BtkTextLine *line)
{
  Summary *summary;
  int needSummary;

  if (segPtr->byte_count != 0)
    {
      g_error ("toggle_segment_check_func: segment had non-zero size");
    }
  if (!segPtr->body.toggle.inNodeCounts)
    {
      g_error ("toggle_segment_check_func: toggle counts not updated in BtkTextBTreeNodes");
    }
  needSummary = (segPtr->body.toggle.info->tag_root != line->parent);
  for (summary = line->parent->summary; ;
       summary = summary->next)
    {
      if (summary == NULL)
        {
          if (needSummary)
            {
              g_error ("toggle_segment_check_func: tag not present in BtkTextBTreeNode");
            }
          else
            {
              break;
            }
        }
      if (summary->info == segPtr->body.toggle.info)
        {
          if (!needSummary)
            {
              g_error ("toggle_segment_check_func: tag present in root BtkTextBTreeNode summary");
            }
          break;
        }
    }
}

/*
 * Debug
 */

static void
btk_text_btree_node_view_check_consistency (BtkTextBTree     *tree,
                                            BtkTextBTreeNode *node,
                                            NodeData         *nd)
{
  bint width;
  bint height;
  bboolean valid;
  BTreeView *view;
  
  view = tree->views;

  while (view != NULL)
    {
      if (view->view_id == nd->view_id)
        break;

      view = view->next;
    }

  if (view == NULL)
    g_error ("Node has data for a view %p no longer attached to the tree",
             nd->view_id);
  
  btk_text_btree_node_compute_view_aggregates (node, nd->view_id,
                                               &width, &height, &valid);

  /* valid aggregate not checked the same as width/height, because on
   * btree rebalance we can have invalid nodes where all lines below
   * them are actually valid, due to moving lines around between
   * nodes.
   *
   * The guarantee is that if there are invalid lines the node is
   * invalid - we don't guarantee that if the node is invalid there
   * are invalid lines.
   */
  
  if (nd->width != width ||
      nd->height != height ||
      (nd->valid && !valid))
    {
      g_error ("Node aggregates for view %p are invalid:\n"
               "Are (%d,%d,%s), should be (%d,%d,%s)",
               nd->view_id,
               nd->width, nd->height, nd->valid ? "TRUE" : "FALSE",
               width, height, valid ? "TRUE" : "FALSE");
    }
}

static void
btk_text_btree_node_check_consistency (BtkTextBTree     *tree,
                                       BtkTextBTreeNode *node)
{
  BtkTextBTreeNode *childnode;
  Summary *summary, *summary2;
  BtkTextLine *line;
  BtkTextLineSegment *segPtr;
  int num_children, num_lines, num_chars, toggle_count, min_children;
  BtkTextLineData *ld;
  NodeData *nd;

  if (node->parent != NULL)
    {
      min_children = MIN_CHILDREN;
    }
  else if (node->level > 0)
    {
      min_children = 2;
    }
  else  {
    min_children = 1;
  }
  if ((node->num_children < min_children)
      || (node->num_children > MAX_CHILDREN))
    {
      g_error ("btk_text_btree_node_check_consistency: bad child count (%d)",
               node->num_children);
    }

  nd = node->node_data;
  while (nd != NULL)
    {
      btk_text_btree_node_view_check_consistency (tree, node, nd);
      nd = nd->next;
    }

  num_children = 0;
  num_lines = 0;
  num_chars = 0;
  if (node->level == 0)
    {
      for (line = node->children.line; line != NULL;
           line = line->next)
        {
          if (line->parent != node)
            {
              g_error ("btk_text_btree_node_check_consistency: line doesn't point to parent");
            }
          if (line->segments == NULL)
            {
              g_error ("btk_text_btree_node_check_consistency: line has no segments");
            }

          ld = line->views;
          while (ld != NULL)
            {
              /* Just ensuring we don't segv while doing this loop */

              ld = ld->next;
            }

          for (segPtr = line->segments; segPtr != NULL; segPtr = segPtr->next)
            {
              if (segPtr->type->checkFunc != NULL)
                {
                  (*segPtr->type->checkFunc)(segPtr, line);
                }
              if ((segPtr->byte_count == 0) && (!segPtr->type->leftGravity)
                  && (segPtr->next != NULL)
                  && (segPtr->next->byte_count == 0)
                  && (segPtr->next->type->leftGravity))
                {
                  g_error ("btk_text_btree_node_check_consistency: wrong segment order for gravity");
                }
              if ((segPtr->next == NULL)
                  && (segPtr->type != &btk_text_char_type))
                {
                  g_error ("btk_text_btree_node_check_consistency: line ended with wrong type");
                }

              num_chars += segPtr->char_count;
            }

          num_children++;
          num_lines++;
        }
    }
  else
    {
      for (childnode = node->children.node; childnode != NULL;
           childnode = childnode->next)
        {
          if (childnode->parent != node)
            {
              g_error ("btk_text_btree_node_check_consistency: BtkTextBTreeNode doesn't point to parent");
            }
          if (childnode->level != (node->level-1))
            {
              g_error ("btk_text_btree_node_check_consistency: level mismatch (%d %d)",
                       node->level, childnode->level);
            }
          btk_text_btree_node_check_consistency (tree, childnode);
          for (summary = childnode->summary; summary != NULL;
               summary = summary->next)
            {
              for (summary2 = node->summary; ;
                   summary2 = summary2->next)
                {
                  if (summary2 == NULL)
                    {
                      if (summary->info->tag_root == node)
                        {
                          break;
                        }
                      g_error ("btk_text_btree_node_check_consistency: BtkTextBTreeNode tag \"%s\" not %s",
                               summary->info->tag->name,
                               "present in parent summaries");
                    }
                  if (summary->info == summary2->info)
                    {
                      break;
                    }
                }
            }
          num_children++;
          num_lines += childnode->num_lines;
          num_chars += childnode->num_chars;
        }
    }
  if (num_children != node->num_children)
    {
      g_error ("btk_text_btree_node_check_consistency: mismatch in num_children (%d %d)",
               num_children, node->num_children);
    }
  if (num_lines != node->num_lines)
    {
      g_error ("btk_text_btree_node_check_consistency: mismatch in num_lines (%d %d)",
               num_lines, node->num_lines);
    }
  if (num_chars != node->num_chars)
    {
      g_error ("btk_text_btree_node_check_consistency: mismatch in num_chars (%d %d)",
               num_chars, node->num_chars);
    }

  for (summary = node->summary; summary != NULL;
       summary = summary->next)
    {
      if (summary->info->toggle_count == summary->toggle_count)
        {
          g_error ("btk_text_btree_node_check_consistency: found unpruned root for \"%s\"",
                   summary->info->tag->name);
        }
      toggle_count = 0;
      if (node->level == 0)
        {
          for (line = node->children.line; line != NULL;
               line = line->next)
            {
              for (segPtr = line->segments; segPtr != NULL;
                   segPtr = segPtr->next)
                {
                  if ((segPtr->type != &btk_text_toggle_on_type)
                      && (segPtr->type != &btk_text_toggle_off_type))
                    {
                      continue;
                    }
                  if (segPtr->body.toggle.info == summary->info)
                    {
                      if (!segPtr->body.toggle.inNodeCounts)
                        g_error ("Toggle segment not in the node counts");

                      toggle_count ++;
                    }
                }
            }
        }
      else
        {
          for (childnode = node->children.node;
               childnode != NULL;
               childnode = childnode->next)
            {
              for (summary2 = childnode->summary;
                   summary2 != NULL;
                   summary2 = summary2->next)
                {
                  if (summary2->info == summary->info)
                    {
                      toggle_count += summary2->toggle_count;
                    }
                }
            }
        }
      if (toggle_count != summary->toggle_count)
        {
          g_error ("btk_text_btree_node_check_consistency: mismatch in toggle_count (%d %d)",
                   toggle_count, summary->toggle_count);
        }
      for (summary2 = summary->next; summary2 != NULL;
           summary2 = summary2->next)
        {
          if (summary2->info == summary->info)
            {
              g_error ("btk_text_btree_node_check_consistency: duplicated BtkTextBTreeNode tag: %s",
                       summary->info->tag->name);
            }
        }
    }
}

static void
listify_foreach (BtkTextTag *tag, bpointer user_data)
{
  GSList** listp = user_data;

  *listp = b_slist_prepend (*listp, tag);
}

static GSList*
list_of_tags (BtkTextTagTable *table)
{
  GSList *list = NULL;

  btk_text_tag_table_foreach (table, listify_foreach, &list);

  return list;
}

void
_btk_text_btree_check (BtkTextBTree *tree)
{
  Summary *summary;
  BtkTextBTreeNode *node;
  BtkTextLine *line;
  BtkTextLineSegment *seg;
  BtkTextTag *tag;
  GSList *all_tags, *taglist = NULL;
  int count;
  BtkTextTagInfo *info;

  /*
   * Make sure that the tag toggle counts and the tag root pointers are OK.
   */
  all_tags = list_of_tags (tree->table);
  for (taglist = all_tags; taglist != NULL ; taglist = taglist->next)
    {
      tag = taglist->data;
      info = btk_text_btree_get_existing_tag_info (tree, tag);
      if (info != NULL)
        {
          node = info->tag_root;
          if (node == NULL)
            {
              if (info->toggle_count != 0)
                {
                  g_error ("_btk_text_btree_check found \"%s\" with toggles (%d) but no root",
                           tag->name, info->toggle_count);
                }
              continue;         /* no ranges for the tag */
            }
          else if (info->toggle_count == 0)
            {
              g_error ("_btk_text_btree_check found root for \"%s\" with no toggles",
                       tag->name);
            }
          else if (info->toggle_count & 1)
            {
              g_error ("_btk_text_btree_check found odd toggle count for \"%s\" (%d)",
                       tag->name, info->toggle_count);
            }
          for (summary = node->summary; summary != NULL;
               summary = summary->next)
            {
              if (summary->info->tag == tag)
                {
                  g_error ("_btk_text_btree_check found root BtkTextBTreeNode with summary info");
                }
            }
          count = 0;
          if (node->level > 0)
            {
              for (node = node->children.node ; node != NULL ;
                   node = node->next)
                {
                  for (summary = node->summary; summary != NULL;
                       summary = summary->next)
                    {
                      if (summary->info->tag == tag)
                        {
                          count += summary->toggle_count;
                        }
                    }
                }
            }
          else
            {
              const BtkTextLineSegmentClass *last = NULL;

              for (line = node->children.line ; line != NULL ;
                   line = line->next)
                {
                  for (seg = line->segments; seg != NULL;
                       seg = seg->next)
                    {
                      if ((seg->type == &btk_text_toggle_on_type ||
                           seg->type == &btk_text_toggle_off_type) &&
                          seg->body.toggle.info->tag == tag)
                        {
                          if (last == seg->type)
                            g_error ("Two consecutive toggles on or off weren't merged");
                          if (!seg->body.toggle.inNodeCounts)
                            g_error ("Toggle segment not in the node counts");

                          last = seg->type;

                          count++;
                        }
                    }
                }
            }
          if (count != info->toggle_count)
            {
              g_error ("_btk_text_btree_check toggle_count (%d) wrong for \"%s\" should be (%d)",
                       info->toggle_count, tag->name, count);
            }
        }
    }

  b_slist_free (all_tags);

  /*
   * Call a recursive procedure to do the main body of checks.
   */

  node = tree->root_node;
  btk_text_btree_node_check_consistency (tree, tree->root_node);

  /*
   * Make sure that there are at least two lines in the text and
   * that the last line has no characters except a newline.
   */

  if (node->num_lines < 2)
    {
      g_error ("_btk_text_btree_check: less than 2 lines in tree");
    }
  if (node->num_chars < 2)
    {
      g_error ("_btk_text_btree_check: less than 2 chars in tree");
    }
  while (node->level > 0)
    {
      node = node->children.node;
      while (node->next != NULL)
        {
          node = node->next;
        }
    }
  line = node->children.line;
  while (line->next != NULL)
    {
      line = line->next;
    }
  seg = line->segments;
  while ((seg->type == &btk_text_toggle_off_type)
         || (seg->type == &btk_text_right_mark_type)
         || (seg->type == &btk_text_left_mark_type))
    {
      /*
       * It's OK to toggle a tag off in the last line, but
       * not to start a new range.  It's also OK to have marks
       * in the last line.
       */

      seg = seg->next;
    }
  if (seg->type != &btk_text_char_type)
    {
      g_error ("_btk_text_btree_check: last line has bogus segment type");
    }
  if (seg->next != NULL)
    {
      g_error ("_btk_text_btree_check: last line has too many segments");
    }
  if (seg->byte_count != 1)
    {
      g_error ("_btk_text_btree_check: last line has wrong # characters: %d",
               seg->byte_count);
    }
  if ((seg->body.chars[0] != '\n') || (seg->body.chars[1] != 0))
    {
      g_error ("_btk_text_btree_check: last line had bad value: %s",
               seg->body.chars);
    }
}

void _btk_text_btree_spew_line (BtkTextBTree* tree, BtkTextLine* line);
void _btk_text_btree_spew_segment (BtkTextBTree* tree, BtkTextLineSegment* seg);
void _btk_text_btree_spew_node (BtkTextBTreeNode *node, int indent);
void _btk_text_btree_spew_line_short (BtkTextLine *line, int indent);

void
_btk_text_btree_spew (BtkTextBTree *tree)
{
  BtkTextLine * line;
  int real_line;

  printf ("%d lines in tree %p\n",
          _btk_text_btree_line_count (tree), tree);

  line = _btk_text_btree_get_line (tree, 0, &real_line);

  while (line != NULL)
    {
      _btk_text_btree_spew_line (tree, line);
      line = _btk_text_line_next (line);
    }

  printf ("=================== Tag information\n");

  {
    GSList * list;

    list = tree->tag_infos;

    while (list != NULL)
      {
        BtkTextTagInfo *info;

        info = list->data;

        printf ("  tag `%s': root at %p, toggle count %d\n",
                info->tag->name, info->tag_root, info->toggle_count);

        list = b_slist_next (list);
      }

    if (tree->tag_infos == NULL)
      {
        printf ("  (no tags in the tree)\n");
      }
  }

  printf ("=================== Tree nodes\n");

  {
    _btk_text_btree_spew_node (tree->root_node, 0);
  }
}

void
_btk_text_btree_spew_line_short (BtkTextLine *line, int indent)
{
  bchar * spaces;
  BtkTextLineSegment *seg;

  spaces = g_strnfill (indent, ' ');

  printf ("%sline %p chars %d bytes %d\n",
          spaces, line,
          _btk_text_line_char_count (line),
          _btk_text_line_byte_count (line));

  seg = line->segments;
  while (seg != NULL)
    {
      if (seg->type == &btk_text_char_type)
        {
          bchar* str = g_strndup (seg->body.chars, MIN (seg->byte_count, 10));
          bchar* s;
          s = str;
          while (*s)
            {
              if (*s == '\n' || *s == '\r')
                *s = '\\';
              ++s;
            }
          printf ("%s chars `%s'...\n", spaces, str);
          g_free (str);
        }
      else if (seg->type == &btk_text_right_mark_type)
        {
          printf ("%s right mark `%s' visible: %d\n",
                  spaces,
                  seg->body.mark.name,
                  seg->body.mark.visible);
        }
      else if (seg->type == &btk_text_left_mark_type)
        {
          printf ("%s left mark `%s' visible: %d\n",
                  spaces,
                  seg->body.mark.name,
                  seg->body.mark.visible);
        }
      else if (seg->type == &btk_text_toggle_on_type ||
               seg->type == &btk_text_toggle_off_type)
        {
          printf ("%s tag `%s' %s\n",
                  spaces, seg->body.toggle.info->tag->name,
                  seg->type == &btk_text_toggle_off_type ? "off" : "on");
        }

      seg = seg->next;
    }

  g_free (spaces);
}

void
_btk_text_btree_spew_node (BtkTextBTreeNode *node, int indent)
{
  bchar * spaces;
  BtkTextBTreeNode *iter;
  Summary *s;

  spaces = g_strnfill (indent, ' ');

  printf ("%snode %p level %d children %d lines %d chars %d\n",
          spaces, node, node->level,
          node->num_children, node->num_lines, node->num_chars);

  s = node->summary;
  while (s)
    {
      printf ("%s %d toggles of `%s' below this node\n",
              spaces, s->toggle_count, s->info->tag->name);
      s = s->next;
    }

  g_free (spaces);

  if (node->level > 0)
    {
      iter = node->children.node;
      while (iter != NULL)
        {
          _btk_text_btree_spew_node (iter, indent + 2);

          iter = iter->next;
        }
    }
  else
    {
      BtkTextLine *line = node->children.line;
      while (line != NULL)
        {
          _btk_text_btree_spew_line_short (line, indent + 2);

          line = line->next;
        }
    }
}

void
_btk_text_btree_spew_line (BtkTextBTree* tree, BtkTextLine* line)
{
  BtkTextLineSegment * seg;

  printf ("%4d| line: %p parent: %p next: %p\n",
          _btk_text_line_get_number (line), line, line->parent, line->next);

  seg = line->segments;

  while (seg != NULL)
    {
      _btk_text_btree_spew_segment (tree, seg);
      seg = seg->next;
    }
}

void
_btk_text_btree_spew_segment (BtkTextBTree* tree, BtkTextLineSegment * seg)
{
  printf ("     segment: %p type: %s bytes: %d chars: %d\n",
          seg, seg->type->name, seg->byte_count, seg->char_count);

  if (seg->type == &btk_text_char_type)
    {
      bchar* str = g_strndup (seg->body.chars, seg->byte_count);
      printf ("       `%s'\n", str);
      g_free (str);
    }
  else if (seg->type == &btk_text_right_mark_type)
    {
      printf ("       right mark `%s' visible: %d not_deleteable: %d\n",
              seg->body.mark.name,
              seg->body.mark.visible,
              seg->body.mark.not_deleteable);
    }
  else if (seg->type == &btk_text_left_mark_type)
    {
      printf ("       left mark `%s' visible: %d not_deleteable: %d\n",
              seg->body.mark.name,
              seg->body.mark.visible,
              seg->body.mark.not_deleteable);
    }
  else if (seg->type == &btk_text_toggle_on_type ||
           seg->type == &btk_text_toggle_off_type)
    {
      printf ("       tag `%s' priority %d\n",
              seg->body.toggle.info->tag->name,
              seg->body.toggle.info->tag->priority);
    }
}

#define __BTK_TEXT_BTREE_C__
#include "btkaliasdef.c"
