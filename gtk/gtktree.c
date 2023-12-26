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

#undef BTK_DISABLE_DEPRECATED

#include "config.h"
#include "btkmain.h"
#include "btkmarshalers.h"
#include "btksignal.h"
#include "btklist.h"

#define BTK_ENABLE_BROKEN
#include "btktree.h"
#include "btktreeitem.h"
#include "btkintl.h"

#include "btkalias.h"

enum {
  SELECTION_CHANGED,
  SELECT_CHILD,
  UNSELECT_CHILD,
  LAST_SIGNAL
};

static void btk_tree_class_init      (BtkTreeClass   *klass);
static void btk_tree_init            (BtkTree        *tree);
static void btk_tree_destroy         (BtkObject      *object);
static void btk_tree_map             (BtkWidget      *widget);
static void btk_tree_parent_set      (BtkWidget      *widget,
				      BtkWidget      *previous_parent);
static void btk_tree_unmap           (BtkWidget      *widget);
static void btk_tree_realize         (BtkWidget      *widget);
static gint btk_tree_motion_notify   (BtkWidget      *widget,
				      BdkEventMotion *event);
static gint btk_tree_button_press    (BtkWidget      *widget,
				      BdkEventButton *event);
static gint btk_tree_button_release  (BtkWidget      *widget,
				      BdkEventButton *event);
static void btk_tree_size_request    (BtkWidget      *widget,
				      BtkRequisition *requisition);
static void btk_tree_size_allocate   (BtkWidget      *widget,
				      BtkAllocation  *allocation);
static void btk_tree_add             (BtkContainer   *container,
				      BtkWidget      *widget);
static void btk_tree_forall          (BtkContainer   *container,
				      gboolean	      include_internals,
				      BtkCallback     callback,
				      gpointer        callback_data);

static void btk_real_tree_select_child   (BtkTree       *tree,
					  BtkWidget     *child);
static void btk_real_tree_unselect_child (BtkTree       *tree,
					  BtkWidget     *child);

static BtkType btk_tree_child_type  (BtkContainer   *container);

static BtkContainerClass *parent_class = NULL;
static guint tree_signals[LAST_SIGNAL] = { 0 };

BtkType
btk_tree_get_type (void)
{
  static BtkType tree_type = 0;
  
  if (!tree_type)
    {
      static const BtkTypeInfo tree_info =
      {
	"BtkTree",
	sizeof (BtkTree),
	sizeof (BtkTreeClass),
	(BtkClassInitFunc) btk_tree_class_init,
	(BtkObjectInitFunc) btk_tree_init,
	/* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (BtkClassInitFunc) NULL,
      };
      
      I_("BtkTree");
      tree_type = btk_type_unique (btk_container_get_type (), &tree_info);
    }
  
  return tree_type;
}

static void
btk_tree_class_init (BtkTreeClass *class)
{
  BtkObjectClass *object_class;
  BtkWidgetClass *widget_class;
  BtkContainerClass *container_class;
  
  object_class = (BtkObjectClass*) class;
  widget_class = (BtkWidgetClass*) class;
  container_class = (BtkContainerClass*) class;
  
  parent_class = btk_type_class (btk_container_get_type ());
  
  
  object_class->destroy = btk_tree_destroy;
  
  widget_class->map = btk_tree_map;
  widget_class->unmap = btk_tree_unmap;
  widget_class->parent_set = btk_tree_parent_set;
  widget_class->realize = btk_tree_realize;
  widget_class->motion_notify_event = btk_tree_motion_notify;
  widget_class->button_press_event = btk_tree_button_press;
  widget_class->button_release_event = btk_tree_button_release;
  widget_class->size_request = btk_tree_size_request;
  widget_class->size_allocate = btk_tree_size_allocate;
  
  container_class->add = btk_tree_add;
  container_class->remove = 
    (void (*)(BtkContainer *, BtkWidget *)) btk_tree_remove_item;
  container_class->forall = btk_tree_forall;
  container_class->child_type = btk_tree_child_type;
  
  class->selection_changed = NULL;
  class->select_child = btk_real_tree_select_child;
  class->unselect_child = btk_real_tree_unselect_child;

  tree_signals[SELECTION_CHANGED] =
    btk_signal_new (I_("selection-changed"),
		    BTK_RUN_FIRST,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkTreeClass, selection_changed),
		    _btk_marshal_VOID__VOID,
		    BTK_TYPE_NONE, 0);
  tree_signals[SELECT_CHILD] =
    btk_signal_new (I_("select-child"),
		    BTK_RUN_FIRST,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkTreeClass, select_child),
		    _btk_marshal_VOID__OBJECT,
		    BTK_TYPE_NONE, 1,
		    BTK_TYPE_WIDGET);
  tree_signals[UNSELECT_CHILD] =
    btk_signal_new (I_("unselect-child"),
		    BTK_RUN_FIRST,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkTreeClass, unselect_child),
		    _btk_marshal_VOID__OBJECT,
		    BTK_TYPE_NONE, 1,
		    BTK_TYPE_WIDGET);
}

static BtkType
btk_tree_child_type (BtkContainer     *container)
{
  return BTK_TYPE_TREE_ITEM;
}

static void
btk_tree_init (BtkTree *tree)
{
  tree->children = NULL;
  tree->root_tree = tree;
  tree->selection = NULL;
  tree->tree_owner = NULL;
  tree->selection_mode = BTK_SELECTION_SINGLE;
  tree->indent_value = 9;
  tree->current_indent = 0;
  tree->level = 0;
  tree->view_mode = BTK_TREE_VIEW_LINE;
  tree->view_line = TRUE;
}

BtkWidget*
btk_tree_new (void)
{
  return BTK_WIDGET (btk_type_new (btk_tree_get_type ()));
}

void
btk_tree_append (BtkTree   *tree,
		 BtkWidget *tree_item)
{
  g_return_if_fail (BTK_IS_TREE (tree));
  g_return_if_fail (BTK_IS_TREE_ITEM (tree_item));
  
  btk_tree_insert (tree, tree_item, -1);
}

void
btk_tree_prepend (BtkTree   *tree,
		  BtkWidget *tree_item)
{
  g_return_if_fail (BTK_IS_TREE (tree));
  g_return_if_fail (BTK_IS_TREE_ITEM (tree_item));
  
  btk_tree_insert (tree, tree_item, 0);
}

void
btk_tree_insert (BtkTree   *tree,
		 BtkWidget *tree_item,
		 gint       position)
{
  gint nchildren;
  
  g_return_if_fail (BTK_IS_TREE (tree));
  g_return_if_fail (BTK_IS_TREE_ITEM (tree_item));
  
  nchildren = g_list_length (tree->children);
  
  if ((position < 0) || (position > nchildren))
    position = nchildren;
  
  if (position == nchildren)
    tree->children = g_list_append (tree->children, tree_item);
  else
    tree->children = g_list_insert (tree->children, tree_item, position);
  
  btk_widget_set_parent (tree_item, BTK_WIDGET (tree));
}

static void
btk_tree_add (BtkContainer *container,
	      BtkWidget    *child)
{
  BtkTree *tree;
  
  g_return_if_fail (BTK_IS_TREE (container));
  g_return_if_fail (BTK_IS_TREE_ITEM (child));
  
  tree = BTK_TREE (container);
  
  tree->children = g_list_append (tree->children, child);
  
  btk_widget_set_parent (child, BTK_WIDGET (container));
  
  if (!tree->selection && (tree->selection_mode == BTK_SELECTION_BROWSE))
    btk_tree_select_child (tree, child);
}

static gint
btk_tree_button_press (BtkWidget      *widget,
		       BdkEventButton *event)
{
  BtkTree *tree;
  BtkWidget *item;
  
  g_return_val_if_fail (BTK_IS_TREE (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);
  
  tree = BTK_TREE (widget);
  item = btk_get_event_widget ((BdkEvent*) event);
  
  while (item && !BTK_IS_TREE_ITEM (item))
    item = item->parent;
  
  if (!item || (item->parent != widget))
    return FALSE;
  
  switch(event->button) 
    {
    case 1:
      btk_tree_select_child (tree, item);
      break;
    case 2:
      if(BTK_TREE_ITEM(item)->subtree) btk_tree_item_expand(BTK_TREE_ITEM(item));
      break;
    case 3:
      if(BTK_TREE_ITEM(item)->subtree) btk_tree_item_collapse(BTK_TREE_ITEM(item));
      break;
    }
  
  return TRUE;
}

static gint
btk_tree_button_release (BtkWidget      *widget,
			 BdkEventButton *event)
{
  BtkTree *tree;
  BtkWidget *item;
  
  g_return_val_if_fail (BTK_IS_TREE (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);
  
  tree = BTK_TREE (widget);
  item = btk_get_event_widget ((BdkEvent*) event);
  
  return TRUE;
}

gint
btk_tree_child_position (BtkTree   *tree,
			 BtkWidget *child)
{
  GList *children;
  gint pos;
  
  
  g_return_val_if_fail (BTK_IS_TREE (tree), -1);
  g_return_val_if_fail (child != NULL, -1);
  
  pos = 0;
  children = tree->children;
  
  while (children)
    {
      if (child == BTK_WIDGET (children->data)) 
	return pos;
      
      pos += 1;
      children = children->next;
    }
  
  
  return -1;
}

void
btk_tree_clear_items (BtkTree *tree,
		      gint     start,
		      gint     end)
{
  BtkWidget *widget;
  GList *clear_list;
  GList *tmp_list;
  guint nchildren;
  guint index;
  
  g_return_if_fail (BTK_IS_TREE (tree));
  
  nchildren = g_list_length (tree->children);
  
  if (nchildren > 0)
    {
      if ((end < 0) || (end > nchildren))
	end = nchildren;
      
      if (start >= end)
	return;
      
      tmp_list = g_list_nth (tree->children, start);
      clear_list = NULL;
      index = start;
      while (tmp_list && index <= end)
	{
	  widget = tmp_list->data;
	  tmp_list = tmp_list->next;
	  index++;
	  
	  clear_list = g_list_prepend (clear_list, widget);
	}
      
      btk_tree_remove_items (tree, clear_list);
    }
}

static void
btk_tree_destroy (BtkObject *object)
{
  BtkTree *tree;
  BtkWidget *child;
  GList *children;
  
  g_return_if_fail (BTK_IS_TREE (object));
  
  tree = BTK_TREE (object);
  
  children = tree->children;
  while (children)
    {
      child = children->data;
      children = children->next;
      
      g_object_ref (child);
      btk_widget_unparent (child);
      btk_widget_destroy (child);
      g_object_unref (child);
    }
  
  g_list_free (tree->children);
  tree->children = NULL;
  
  if (tree->root_tree == tree)
    {
      GList *node;
      for (node = tree->selection; node; node = node->next)
	g_object_unref (node->data);
      g_list_free (tree->selection);
      tree->selection = NULL;
    }

  BTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
btk_tree_forall (BtkContainer *container,
		 gboolean      include_internals,
		 BtkCallback   callback,
		 gpointer      callback_data)
{
  BtkTree *tree;
  BtkWidget *child;
  GList *children;
  
  
  g_return_if_fail (BTK_IS_TREE (container));
  g_return_if_fail (callback != NULL);
  
  tree = BTK_TREE (container);
  children = tree->children;
  
  while (children)
    {
      child = children->data;
      children = children->next;
      
      (* callback) (child, callback_data);
    }
}

static void
btk_tree_unselect_all (BtkTree *tree)
{
  GList *tmp_list, *selection;
  BtkWidget *tmp_item;
      
  selection = tree->selection;
  tree->selection = NULL;

  tmp_list = selection;
  while (tmp_list)
    {
      tmp_item = selection->data;

      if (tmp_item->parent &&
	  BTK_IS_TREE (tmp_item->parent) &&
	  BTK_TREE (tmp_item->parent)->root_tree == tree)
	btk_tree_item_deselect (BTK_TREE_ITEM (tmp_item));

      g_object_unref (tmp_item);

      tmp_list = tmp_list->next;
    }

  g_list_free (selection);
}

static void
btk_tree_parent_set (BtkWidget *widget,
		     BtkWidget *previous_parent)
{
  BtkTree *tree = BTK_TREE (widget);
  BtkWidget *child;
  GList *children;
  
  if (BTK_IS_TREE (widget->parent))
    {
      btk_tree_unselect_all (tree);
      
      /* set root tree for this tree */
      tree->root_tree = BTK_TREE(widget->parent)->root_tree;
      
      tree->level = BTK_TREE(BTK_WIDGET(tree)->parent)->level+1;
      tree->indent_value = BTK_TREE(BTK_WIDGET(tree)->parent)->indent_value;
      tree->current_indent = BTK_TREE(BTK_WIDGET(tree)->parent)->current_indent + 
	tree->indent_value;
      tree->view_mode = BTK_TREE(BTK_WIDGET(tree)->parent)->view_mode;
      tree->view_line = BTK_TREE(BTK_WIDGET(tree)->parent)->view_line;
    }
  else
    {
      tree->root_tree = tree;
      
      tree->level = 0;
      tree->current_indent = 0;
    }

  children = tree->children;
  while (children)
    {
      child = children->data;
      children = children->next;
      
      if (BTK_TREE_ITEM (child)->subtree)
	btk_tree_parent_set (BTK_TREE_ITEM (child)->subtree, child);
    }
}

static void
btk_tree_map (BtkWidget *widget)
{
  BtkTree *tree = BTK_TREE (widget);
  BtkWidget *child;
  GList *children;
  
  btk_widget_set_mapped (widget, TRUE);
  
  children = tree->children;
  while (children)
    {
      child = children->data;
      children = children->next;
      
      if (btk_widget_get_visible (child) &&
	  !btk_widget_get_mapped (child))
	btk_widget_map (child);
      
      if (BTK_TREE_ITEM (child)->subtree)
	{
	  child = BTK_WIDGET (BTK_TREE_ITEM (child)->subtree);
	  
	  if (btk_widget_get_visible (child) && !btk_widget_get_mapped (child))
	    btk_widget_map (child);
	}
    }

  bdk_window_show (widget->window);
}

static gint
btk_tree_motion_notify (BtkWidget      *widget,
			BdkEventMotion *event)
{
  g_return_val_if_fail (BTK_IS_TREE (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);
  
#ifdef TREE_DEBUG
  g_message("btk_tree_motion_notify\n");
#endif /* TREE_DEBUG */
  
  return FALSE;
}

static void
btk_tree_realize (BtkWidget *widget)
{
  BdkWindowAttr attributes;
  gint attributes_mask;
  
  
  g_return_if_fail (BTK_IS_TREE (widget));
  
  btk_widget_set_realized (widget, TRUE);
  
  attributes.window_type = BDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = BDK_INPUT_OUTPUT;
  attributes.visual = btk_widget_get_visual (widget);
  attributes.colormap = btk_widget_get_colormap (widget);
  attributes.event_mask = btk_widget_get_events (widget) | BDK_EXPOSURE_MASK;
  
  attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_VISUAL | BDK_WA_COLORMAP;
  
  widget->window = bdk_window_new (btk_widget_get_parent_window (widget), &attributes, attributes_mask);
  bdk_window_set_user_data (widget->window, widget);
  
  widget->style = btk_style_attach (widget->style, widget->window);
  bdk_window_set_background (widget->window, 
			     &widget->style->base[BTK_STATE_NORMAL]);
}

void
btk_tree_remove_item (BtkTree      *container,
		      BtkWidget    *widget)
{
  GList *item_list;
  
  g_return_if_fail (BTK_IS_TREE (container));
  g_return_if_fail (widget != NULL);
  g_return_if_fail (container == BTK_TREE (widget->parent));
  
  item_list = g_list_append (NULL, widget);
  
  btk_tree_remove_items (BTK_TREE (container), item_list);
  
  g_list_free (item_list);
}

/* used by btk_tree_remove_items to make the function independent of
   order in list of items to remove.
   Sort item by depth in tree */
static gint 
btk_tree_sort_item_by_depth(BtkWidget* a, BtkWidget* b)
{
  if((BTK_TREE(a->parent)->level) < (BTK_TREE(b->parent)->level))
    return 1;
  if((BTK_TREE(a->parent)->level) > (BTK_TREE(b->parent)->level))
    return -1;
  
  return 0;
}

void
btk_tree_remove_items (BtkTree *tree,
		       GList   *items)
{
  BtkWidget *widget;
  GList *selected_widgets;
  GList *tmp_list;
  GList *sorted_list;
  BtkTree *real_tree;
  BtkTree *root_tree;
  
  g_return_if_fail (BTK_IS_TREE (tree));
  
#ifdef TREE_DEBUG
  g_message("+ btk_tree_remove_items [ tree %#x items list %#x ]\n", (int)tree, (int)items);
#endif /* TREE_DEBUG */
  
  /* We may not yet be mapped, so we actively have to find our
   * root tree
   */
  if (tree->root_tree)
    root_tree = tree->root_tree;
  else
    {
      BtkWidget *tmp = BTK_WIDGET (tree);
      while (BTK_IS_TREE (tmp->parent))
	tmp = tmp->parent;
      
      root_tree = BTK_TREE (tmp);
    }
  
  tmp_list = items;
  selected_widgets = NULL;
  sorted_list = NULL;
  widget = NULL;
  
#ifdef TREE_DEBUG
  g_message("* sort list by depth\n");
#endif /* TREE_DEBUG */
  
  while (tmp_list)
    {
      
#ifdef TREE_DEBUG
      g_message ("* item [%#x] depth [%d]\n", 
		 (int)tmp_list->data,
		 (int)BTK_TREE(BTK_WIDGET(tmp_list->data)->parent)->level);
#endif /* TREE_DEBUG */
      
      sorted_list = g_list_insert_sorted(sorted_list,
					 tmp_list->data,
					 (GCompareFunc)btk_tree_sort_item_by_depth);
      tmp_list = g_list_next(tmp_list);
    }
  
#ifdef TREE_DEBUG
  /* print sorted list */
  g_message("* sorted list result\n");
  tmp_list = sorted_list;
  while(tmp_list)
    {
      g_message("* item [%#x] depth [%d]\n", 
		(int)tmp_list->data,
		(int)BTK_TREE(BTK_WIDGET(tmp_list->data)->parent)->level);
      tmp_list = g_list_next(tmp_list);
    }
#endif /* TREE_DEBUG */
  
#ifdef TREE_DEBUG
  g_message("* scan sorted list\n");
#endif /* TREE_DEBUG */
  
  tmp_list = sorted_list;
  while (tmp_list)
    {
      widget = tmp_list->data;
      tmp_list = tmp_list->next;
      
#ifdef TREE_DEBUG
      g_message("* item [%#x] subtree [%#x]\n", 
		(int)widget, (int)BTK_TREE_ITEM_SUBTREE(widget));
#endif /* TREE_DEBUG */
      
      /* get real owner of this widget */
      real_tree = BTK_TREE(widget->parent);
#ifdef TREE_DEBUG
      g_message("* subtree having this widget [%#x]\n", (int)real_tree);
#endif /* TREE_DEBUG */
      
      
      if (widget->state == BTK_STATE_SELECTED)
	{
	  selected_widgets = g_list_prepend (selected_widgets, widget);
#ifdef TREE_DEBUG
	  g_message("* selected widget - adding it in selected list [%#x]\n",
		    (int)selected_widgets);
#endif /* TREE_DEBUG */
	}
      
      /* remove this item from its real parent */
#ifdef TREE_DEBUG
      g_message("* remove widget from its owner tree\n");
#endif /* TREE_DEBUG */
      real_tree->children = g_list_remove (real_tree->children, widget);
      
      /* remove subtree associate at this item if it exist */      
      if(BTK_TREE_ITEM(widget)->subtree) 
	{
#ifdef TREE_DEBUG
	  g_message("* remove subtree associate at this item [%#x]\n",
		    (int) BTK_TREE_ITEM(widget)->subtree);
#endif /* TREE_DEBUG */
	  if (btk_widget_get_mapped (BTK_TREE_ITEM(widget)->subtree))
	    btk_widget_unmap (BTK_TREE_ITEM(widget)->subtree);
	  
	  btk_widget_unparent (BTK_TREE_ITEM(widget)->subtree);
	  BTK_TREE_ITEM(widget)->subtree = NULL;
	}
      
      /* really remove widget for this item */
#ifdef TREE_DEBUG
      g_message("* unmap and unparent widget [%#x]\n", (int)widget);
#endif /* TREE_DEBUG */
      if (btk_widget_get_mapped (widget))
	btk_widget_unmap (widget);
      
      btk_widget_unparent (widget);
      
      /* delete subtree if there is no children in it */
      if(real_tree->children == NULL && 
	 real_tree != root_tree)
	{
#ifdef TREE_DEBUG
	  g_message("* owner tree don't have children ... destroy it\n");
#endif /* TREE_DEBUG */
	  btk_tree_item_remove_subtree(BTK_TREE_ITEM(real_tree->tree_owner));
	}
      
#ifdef TREE_DEBUG
      g_message("* next item in list\n");
#endif /* TREE_DEBUG */
    }
  
  if (selected_widgets)
    {
#ifdef TREE_DEBUG
      g_message("* scan selected item list\n");
#endif /* TREE_DEBUG */
      tmp_list = selected_widgets;
      while (tmp_list)
	{
	  widget = tmp_list->data;
	  tmp_list = tmp_list->next;
	  
#ifdef TREE_DEBUG
	  g_message("* widget [%#x] subtree [%#x]\n", 
		    (int)widget, (int)BTK_TREE_ITEM_SUBTREE(widget));
#endif /* TREE_DEBUG */
	  
	  /* remove widget of selection */
	  root_tree->selection = g_list_remove (root_tree->selection, widget);
	  
	  /* unref it to authorize is destruction */
	  g_object_unref (widget);
	}
      
      /* emit only one selection_changed signal */
      btk_signal_emit (BTK_OBJECT (root_tree), 
		       tree_signals[SELECTION_CHANGED]);
    }
  
#ifdef TREE_DEBUG
  g_message("* free selected_widgets list\n");
#endif /* TREE_DEBUG */
  g_list_free (selected_widgets);
  g_list_free (sorted_list);
  
  if (root_tree->children && !root_tree->selection &&
      (root_tree->selection_mode == BTK_SELECTION_BROWSE))
    {
#ifdef TREE_DEBUG
      g_message("* BROWSE mode, select another item\n");
#endif /* TREE_DEBUG */
      widget = root_tree->children->data;
      btk_tree_select_child (root_tree, widget);
    }
  
  if (btk_widget_get_visible (BTK_WIDGET (root_tree)))
    {
#ifdef TREE_DEBUG
      g_message("* query queue resizing for root_tree\n");
#endif /* TREE_DEBUG */      
      btk_widget_queue_resize (BTK_WIDGET (root_tree));
    }
}

void
btk_tree_select_child (BtkTree   *tree,
		       BtkWidget *tree_item)
{
  g_return_if_fail (BTK_IS_TREE (tree));
  g_return_if_fail (BTK_IS_TREE_ITEM (tree_item));
  
  btk_signal_emit (BTK_OBJECT (tree), tree_signals[SELECT_CHILD], tree_item);
}

void
btk_tree_select_item (BtkTree   *tree,
		      gint       item)
{
  GList *tmp_list;
  
  g_return_if_fail (BTK_IS_TREE (tree));
  
  tmp_list = g_list_nth (tree->children, item);
  if (tmp_list)
    btk_tree_select_child (tree, BTK_WIDGET (tmp_list->data));
  
}

static void
btk_tree_size_allocate (BtkWidget     *widget,
			BtkAllocation *allocation)
{
  BtkTree *tree;
  BtkWidget *child, *subtree;
  BtkAllocation child_allocation;
  GList *children;
  
  
  g_return_if_fail (BTK_IS_TREE (widget));
  g_return_if_fail (allocation != NULL);
  
  tree = BTK_TREE (widget);
  
  widget->allocation = *allocation;
  if (btk_widget_get_realized (widget))
    bdk_window_move_resize (widget->window,
			    allocation->x, allocation->y,
			    allocation->width, allocation->height);
  
  if (tree->children)
    {
      child_allocation.x = BTK_CONTAINER (tree)->border_width;
      child_allocation.y = BTK_CONTAINER (tree)->border_width;
      child_allocation.width = MAX (1, (gint)allocation->width - child_allocation.x * 2);
      
      children = tree->children;
      
      while (children)
	{
	  child = children->data;
	  children = children->next;
	  
	  if (btk_widget_get_visible (child))
	    {
	      BtkRequisition child_requisition;
	      btk_widget_get_child_requisition (child, &child_requisition);
	      
	      child_allocation.height = child_requisition.height;
	      
	      btk_widget_size_allocate (child, &child_allocation);
	      
	      child_allocation.y += child_allocation.height;
	      
	      if((subtree = BTK_TREE_ITEM(child)->subtree))
		if(btk_widget_get_visible (subtree))
		  {
		    child_allocation.height = subtree->requisition.height;
		    btk_widget_size_allocate (subtree, &child_allocation);
		    child_allocation.y += child_allocation.height;
		  }
	    }
	}
    }
  
}

static void
btk_tree_size_request (BtkWidget      *widget,
		       BtkRequisition *requisition)
{
  BtkTree *tree;
  BtkWidget *child, *subtree;
  GList *children;
  BtkRequisition child_requisition;
  
  
  g_return_if_fail (BTK_IS_TREE (widget));
  g_return_if_fail (requisition != NULL);
  
  tree = BTK_TREE (widget);
  requisition->width = 0;
  requisition->height = 0;
  
  children = tree->children;
  while (children)
    {
      child = children->data;
      children = children->next;
      
      if (btk_widget_get_visible (child))
	{
	  btk_widget_size_request (child, &child_requisition);
	  
	  requisition->width = MAX (requisition->width, child_requisition.width);
	  requisition->height += child_requisition.height;
	  
	  if((subtree = BTK_TREE_ITEM(child)->subtree) &&
	     btk_widget_get_visible (subtree))
	    {
	      btk_widget_size_request (subtree, &child_requisition);
	      
	      requisition->width = MAX (requisition->width, 
					child_requisition.width);
	      
	      requisition->height += child_requisition.height;
	    }
	}
    }
  
  requisition->width += BTK_CONTAINER (tree)->border_width * 2;
  requisition->height += BTK_CONTAINER (tree)->border_width * 2;
  
  requisition->width = MAX (requisition->width, 1);
  requisition->height = MAX (requisition->height, 1);
  
}

static void
btk_tree_unmap (BtkWidget *widget)
{
  
  g_return_if_fail (BTK_IS_TREE (widget));
  
  btk_widget_set_mapped (widget, FALSE);
  bdk_window_hide (widget->window);
  
}

void
btk_tree_unselect_child (BtkTree   *tree,
			 BtkWidget *tree_item)
{
  g_return_if_fail (BTK_IS_TREE (tree));
  g_return_if_fail (BTK_IS_TREE_ITEM (tree_item));
  
  btk_signal_emit (BTK_OBJECT (tree), tree_signals[UNSELECT_CHILD], tree_item);
}

void
btk_tree_unselect_item (BtkTree *tree,
			gint     item)
{
  GList *tmp_list;
  
  g_return_if_fail (BTK_IS_TREE (tree));
  
  tmp_list = g_list_nth (tree->children, item);
  if (tmp_list)
    btk_tree_unselect_child (tree, BTK_WIDGET (tmp_list->data));
  
}

static void
btk_real_tree_select_child (BtkTree   *tree,
			    BtkWidget *child)
{
  GList *selection, *root_selection;
  GList *tmp_list;
  BtkWidget *tmp_item;
  
  g_return_if_fail (BTK_IS_TREE (tree));
  g_return_if_fail (BTK_IS_TREE_ITEM (child));

  root_selection = tree->root_tree->selection;
  
  switch (tree->root_tree->selection_mode)
    {
    case BTK_SELECTION_SINGLE:
      
      selection = root_selection;
      
      /* remove old selection list */
      while (selection)
	{
	  tmp_item = selection->data;
	  
	  if (tmp_item != child)
	    {
	      btk_tree_item_deselect (BTK_TREE_ITEM (tmp_item));
	      
	      tmp_list = selection;
	      selection = selection->next;
	      
	      root_selection = g_list_remove_link (root_selection, tmp_list);
	      g_object_unref (tmp_item);
	      
	      g_list_free (tmp_list);
	    }
	  else
	    selection = selection->next;
	}
      
      if (child->state == BTK_STATE_NORMAL)
	{
	  btk_tree_item_select (BTK_TREE_ITEM (child));
	  root_selection = g_list_prepend (root_selection, child);
	  g_object_ref (child);
	}
      else if (child->state == BTK_STATE_SELECTED)
	{
	  btk_tree_item_deselect (BTK_TREE_ITEM (child));
	  root_selection = g_list_remove (root_selection, child);
	  g_object_unref (child);
	}
      
      tree->root_tree->selection = root_selection;
      
      btk_signal_emit (BTK_OBJECT (tree->root_tree), 
		       tree_signals[SELECTION_CHANGED]);
      break;
      
      
    case BTK_SELECTION_BROWSE:
      selection = root_selection;
      
      while (selection)
	{
	  tmp_item = selection->data;
	  
	  if (tmp_item != child)
	    {
	      btk_tree_item_deselect (BTK_TREE_ITEM (tmp_item));
	      
	      tmp_list = selection;
	      selection = selection->next;
	      
	      root_selection = g_list_remove_link (root_selection, tmp_list);
	      g_object_unref (tmp_item);
	      
	      g_list_free (tmp_list);
	    }
	  else
	    selection = selection->next;
	}
      
      tree->root_tree->selection = root_selection;
      
      if (child->state == BTK_STATE_NORMAL)
	{
	  btk_tree_item_select (BTK_TREE_ITEM (child));
	  root_selection = g_list_prepend (root_selection, child);
	  g_object_ref (child);
	  tree->root_tree->selection = root_selection;
	  btk_signal_emit (BTK_OBJECT (tree->root_tree), 
			   tree_signals[SELECTION_CHANGED]);
	}
      break;
      
    case BTK_SELECTION_MULTIPLE:
      break;
    }
}

static void
btk_real_tree_unselect_child (BtkTree   *tree,
			      BtkWidget *child)
{
  g_return_if_fail (BTK_IS_TREE (tree));
  g_return_if_fail (BTK_IS_TREE_ITEM (child));
  
  switch (tree->selection_mode)
    {
    case BTK_SELECTION_SINGLE:
    case BTK_SELECTION_BROWSE:
      if (child->state == BTK_STATE_SELECTED)
	{
	  BtkTree* root_tree = BTK_TREE_ROOT_TREE(tree);
	  btk_tree_item_deselect (BTK_TREE_ITEM (child));
	  root_tree->selection = g_list_remove (root_tree->selection, child);
	  g_object_unref (child);
	  btk_signal_emit (BTK_OBJECT (tree->root_tree), 
			   tree_signals[SELECTION_CHANGED]);
	}
      break;
      
    case BTK_SELECTION_MULTIPLE:
      break;
    }
}

void
btk_tree_set_selection_mode (BtkTree       *tree,
			     BtkSelectionMode mode) 
{
  g_return_if_fail (BTK_IS_TREE (tree));
  
  tree->selection_mode = mode;
}

void
btk_tree_set_view_mode (BtkTree       *tree,
			BtkTreeViewMode mode) 
{
  g_return_if_fail (BTK_IS_TREE (tree));
  
  tree->view_mode = mode;
}

void
btk_tree_set_view_lines (BtkTree       *tree,
			 gboolean	flag) 
{
  g_return_if_fail (BTK_IS_TREE (tree));
  
  tree->view_line = flag;
}

#define __BTK_TREE_C__
#include "btkaliasdef.c"
