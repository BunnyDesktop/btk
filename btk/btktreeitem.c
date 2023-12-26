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

#undef BDK_DISABLE_DEPRECATED
#undef BTK_DISABLE_DEPRECATED

#include "config.h"

#include "btklabel.h"
#include "btkeventbox.h"
#include "btkpixmap.h"
#include "btkmain.h"
#include "btkmarshalers.h"
#include "btksignal.h"
#define BTK_ENABLE_BROKEN
#include "btktree.h"
#include "btktreeitem.h"
#include "btkintl.h"

#include "btkalias.h"

#include "tree_plus.xpm"
#include "tree_minus.xpm"

#define DEFAULT_DELTA 9

enum {
  COLLAPSE_TREE,
  EXPAND_TREE,
  LAST_SIGNAL
};

typedef struct _BtkTreePixmaps BtkTreePixmaps;

struct _BtkTreePixmaps {
  bint refcount;
  BdkColormap *colormap;
  
  BdkPixmap *pixmap_plus;
  BdkPixmap *pixmap_minus;
  BdkBitmap *mask_plus;
  BdkBitmap *mask_minus;
};

static GList *pixmaps = NULL;

static void btk_tree_item_class_init (BtkTreeItemClass *klass);
static void btk_tree_item_init       (BtkTreeItem      *tree_item);
static void btk_tree_item_realize       (BtkWidget        *widget);
static void btk_tree_item_size_request  (BtkWidget        *widget,
					 BtkRequisition   *requisition);
static void btk_tree_item_size_allocate (BtkWidget        *widget,
					 BtkAllocation    *allocation);
static void btk_tree_item_paint         (BtkWidget        *widget,
					 BdkRectangle     *area);
static bint btk_tree_item_button_press  (BtkWidget        *widget,
					 BdkEventButton   *event);
static bint btk_tree_item_expose        (BtkWidget        *widget,
					 BdkEventExpose   *event);
static void btk_tree_item_forall        (BtkContainer    *container,
					 bboolean         include_internals,
					 BtkCallback      callback,
					 bpointer         callback_data);

static void btk_real_tree_item_select   (BtkItem          *item);
static void btk_real_tree_item_deselect (BtkItem          *item);
static void btk_real_tree_item_toggle   (BtkItem          *item);
static void btk_real_tree_item_expand   (BtkTreeItem      *item);
static void btk_real_tree_item_collapse (BtkTreeItem      *item);
static void btk_tree_item_destroy        (BtkObject *object);
static bint btk_tree_item_subtree_button_click (BtkWidget *widget);
static void btk_tree_item_subtree_button_changed_state (BtkWidget *widget);

static void btk_tree_item_map(BtkWidget*);
static void btk_tree_item_unmap(BtkWidget*);

static void btk_tree_item_add_pixmaps    (BtkTreeItem       *tree_item);
static void btk_tree_item_remove_pixmaps (BtkTreeItem       *tree_item);

static BtkItemClass *parent_class = NULL;
static buint tree_item_signals[LAST_SIGNAL] = { 0 };

BtkType
btk_tree_item_get_type (void)
{
  static BtkType tree_item_type = 0;

  if (!tree_item_type)
    {
      static const BtkTypeInfo tree_item_info =
      {
	"BtkTreeItem",
	sizeof (BtkTreeItem),
	sizeof (BtkTreeItemClass),
	(BtkClassInitFunc) btk_tree_item_class_init,
	(BtkObjectInitFunc) btk_tree_item_init,
	/* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (BtkClassInitFunc) NULL,
      };

      I_("BtkTreeItem");
      tree_item_type = btk_type_unique (btk_item_get_type (), &tree_item_info);
    }

  return tree_item_type;
}

static void
btk_tree_item_class_init (BtkTreeItemClass *class)
{
  BtkObjectClass *object_class;
  BtkWidgetClass *widget_class;
  BtkContainerClass *container_class;
  BtkItemClass *item_class;

  parent_class = btk_type_class (BTK_TYPE_ITEM);
  
  object_class = (BtkObjectClass*) class;
  widget_class = (BtkWidgetClass*) class;
  item_class = (BtkItemClass*) class;
  container_class = (BtkContainerClass*) class;

  object_class->destroy = btk_tree_item_destroy;

  widget_class->realize = btk_tree_item_realize;
  widget_class->size_request = btk_tree_item_size_request;
  widget_class->size_allocate = btk_tree_item_size_allocate;
  widget_class->button_press_event = btk_tree_item_button_press;
  widget_class->expose_event = btk_tree_item_expose;
  widget_class->map = btk_tree_item_map;
  widget_class->unmap = btk_tree_item_unmap;

  container_class->forall = btk_tree_item_forall;

  item_class->select = btk_real_tree_item_select;
  item_class->deselect = btk_real_tree_item_deselect;
  item_class->toggle = btk_real_tree_item_toggle;

  class->expand = btk_real_tree_item_expand;
  class->collapse = btk_real_tree_item_collapse;

  tree_item_signals[EXPAND_TREE] =
    btk_signal_new (I_("expand"),
		    BTK_RUN_FIRST,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkTreeItemClass, expand),
		    _btk_marshal_VOID__VOID,
		    BTK_TYPE_NONE, 0);
  tree_item_signals[COLLAPSE_TREE] =
    btk_signal_new (I_("collapse"),
		    BTK_RUN_FIRST,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkTreeItemClass, collapse),
		    _btk_marshal_VOID__VOID,
		    BTK_TYPE_NONE, 0);
}

/* callback for event box mouse event */
static bint
btk_tree_item_subtree_button_click (BtkWidget *widget)
{
  BtkTreeItem* item;
  
  g_return_val_if_fail (BTK_IS_EVENT_BOX (widget), FALSE);
  
  item = (BtkTreeItem*) btk_object_get_user_data (BTK_OBJECT (widget));
  if (!btk_widget_is_sensitive (BTK_WIDGET (item)))
    return FALSE;
  
  if (item->expanded)
    btk_tree_item_collapse (item);
  else
    btk_tree_item_expand (item);

  return TRUE;
}

/* callback for event box state changed */
static void
btk_tree_item_subtree_button_changed_state (BtkWidget *widget)
{
  g_return_if_fail (BTK_IS_EVENT_BOX (widget));
  
  if (btk_widget_get_visible (widget))
    {
      
      if (widget->state == BTK_STATE_NORMAL)
	bdk_window_set_background (widget->window, &widget->style->base[widget->state]);
      else
	bdk_window_set_background (widget->window, &widget->style->bg[widget->state]);
      
      if (btk_widget_is_drawable (widget))
	bdk_window_clear_area (widget->window, 0, 0, 
			       widget->allocation.width, widget->allocation.height);
    }
}

static void
btk_tree_item_init (BtkTreeItem *tree_item)
{
  BtkWidget *eventbox, *pixmapwid;

  tree_item->expanded = FALSE;
  tree_item->subtree = NULL;
  btk_widget_set_can_focus (BTK_WIDGET (tree_item), TRUE);
  
  /* create an event box containing one pixmaps */
  eventbox = btk_event_box_new();
  btk_widget_set_events (eventbox, BDK_BUTTON_PRESS_MASK);
  btk_signal_connect(BTK_OBJECT(eventbox), "state-changed",
		     G_CALLBACK (btk_tree_item_subtree_button_changed_state),
		     (bpointer)NULL);
  btk_signal_connect(BTK_OBJECT(eventbox), "realize",
		     G_CALLBACK (btk_tree_item_subtree_button_changed_state),
		     (bpointer)NULL);
  btk_signal_connect(BTK_OBJECT(eventbox), "button-press-event",
		     G_CALLBACK (btk_tree_item_subtree_button_click),
		     (bpointer)NULL);
  btk_object_set_user_data(BTK_OBJECT(eventbox), tree_item);
  tree_item->pixmaps_box = eventbox;

  /* create pixmap for button '+' */
  pixmapwid = btk_type_new (btk_pixmap_get_type ());
  if (!tree_item->expanded) 
    btk_container_add (BTK_CONTAINER (eventbox), pixmapwid);
  btk_widget_show (pixmapwid);
  tree_item->plus_pix_widget = pixmapwid;
  g_object_ref_sink (tree_item->plus_pix_widget);
  
  /* create pixmap for button '-' */
  pixmapwid = btk_type_new (btk_pixmap_get_type ());
  if (tree_item->expanded) 
    btk_container_add (BTK_CONTAINER (eventbox), pixmapwid);
  btk_widget_show (pixmapwid);
  tree_item->minus_pix_widget = pixmapwid;
  g_object_ref_sink (tree_item->minus_pix_widget);
  
  btk_widget_set_parent (eventbox, BTK_WIDGET (tree_item));
}


BtkWidget*
btk_tree_item_new (void)
{
  BtkWidget *tree_item;

  tree_item = BTK_WIDGET (btk_type_new (btk_tree_item_get_type ()));

  return tree_item;
}

BtkWidget*
btk_tree_item_new_with_label (const bchar *label)
{
  BtkWidget *tree_item;
  BtkWidget *label_widget;

  tree_item = btk_tree_item_new ();
  label_widget = btk_label_new (label);
  btk_misc_set_alignment (BTK_MISC (label_widget), 0.0, 0.5);

  btk_container_add (BTK_CONTAINER (tree_item), label_widget);
  btk_widget_show (label_widget);


  return tree_item;
}

void
btk_tree_item_set_subtree (BtkTreeItem *tree_item,
			   BtkWidget   *subtree)
{
  g_return_if_fail (BTK_IS_TREE_ITEM (tree_item));
  g_return_if_fail (BTK_IS_TREE (subtree));

  if (tree_item->subtree)
    {
      g_warning("there is already a subtree for this tree item\n");
      return;
    }

  tree_item->subtree = subtree; 
  BTK_TREE (subtree)->tree_owner = BTK_WIDGET (tree_item);

  /* show subtree button */
  if (tree_item->pixmaps_box)
    btk_widget_show (tree_item->pixmaps_box);

  if (tree_item->expanded)
    btk_widget_show (subtree);
  else
    btk_widget_hide (subtree);

  btk_widget_set_parent (subtree, BTK_WIDGET (tree_item)->parent);
}

void
btk_tree_item_select (BtkTreeItem *tree_item)
{
  g_return_if_fail (BTK_IS_TREE_ITEM (tree_item));

  btk_item_select (BTK_ITEM (tree_item));
}

void
btk_tree_item_deselect (BtkTreeItem *tree_item)
{
  g_return_if_fail (BTK_IS_TREE_ITEM (tree_item));

  btk_item_deselect (BTK_ITEM (tree_item));
}

void
btk_tree_item_expand (BtkTreeItem *tree_item)
{
  g_return_if_fail (BTK_IS_TREE_ITEM (tree_item));

  btk_signal_emit (BTK_OBJECT (tree_item), tree_item_signals[EXPAND_TREE], NULL);
}

void
btk_tree_item_collapse (BtkTreeItem *tree_item)
{
  g_return_if_fail (BTK_IS_TREE_ITEM (tree_item));

  btk_signal_emit (BTK_OBJECT (tree_item), tree_item_signals[COLLAPSE_TREE], NULL);
}

static void
btk_tree_item_add_pixmaps (BtkTreeItem *tree_item)
{
  GList *tmp_list;
  BdkColormap *colormap;
  BtkTreePixmaps *pixmap_node = NULL;

  g_return_if_fail (BTK_IS_TREE_ITEM (tree_item));

  if (tree_item->pixmaps)
    return;

  colormap = btk_widget_get_colormap (BTK_WIDGET (tree_item));

  tmp_list = pixmaps;
  while (tmp_list)
    {
      pixmap_node = (BtkTreePixmaps *)tmp_list->data;

      if (pixmap_node->colormap == colormap)
	break;
      
      tmp_list = tmp_list->next;
    }

  if (tmp_list)
    {
      pixmap_node->refcount++;
      tree_item->pixmaps = tmp_list;
    }
  else
    {
      pixmap_node = g_new (BtkTreePixmaps, 1);

      pixmap_node->colormap = colormap;
      g_object_ref (colormap);

      pixmap_node->refcount = 1;

      /* create pixmaps for plus icon */
      pixmap_node->pixmap_plus = 
	bdk_pixmap_create_from_xpm_d (BTK_WIDGET (tree_item)->window,
				      &pixmap_node->mask_plus,
				      NULL,
				      (bchar **)tree_plus);
      
      /* create pixmaps for minus icon */
      pixmap_node->pixmap_minus = 
	bdk_pixmap_create_from_xpm_d (BTK_WIDGET (tree_item)->window,
				      &pixmap_node->mask_minus,
				      NULL,
				      (bchar **)tree_minus);

      tree_item->pixmaps = pixmaps = g_list_prepend (pixmaps, pixmap_node);
    }
  
  btk_pixmap_set (BTK_PIXMAP (tree_item->plus_pix_widget), 
		  pixmap_node->pixmap_plus, pixmap_node->mask_plus);
  btk_pixmap_set (BTK_PIXMAP (tree_item->minus_pix_widget), 
		  pixmap_node->pixmap_minus, pixmap_node->mask_minus);
}

static void
btk_tree_item_remove_pixmaps (BtkTreeItem *tree_item)
{
  g_return_if_fail (BTK_IS_TREE_ITEM (tree_item));

  if (tree_item->pixmaps)
    {
      BtkTreePixmaps *pixmap_node = (BtkTreePixmaps *)tree_item->pixmaps->data;
      
      g_assert (pixmap_node->refcount > 0);
      
      if (--pixmap_node->refcount == 0)
	{
	  g_object_unref (pixmap_node->colormap);
	  g_object_unref (pixmap_node->pixmap_plus);
	  g_object_unref (pixmap_node->mask_plus);
	  g_object_unref (pixmap_node->pixmap_minus);
	  g_object_unref (pixmap_node->mask_minus);
	  
	  pixmaps = g_list_remove_link (pixmaps, tree_item->pixmaps);
	  g_list_free_1 (tree_item->pixmaps);
	  g_free (pixmap_node);
	}

      tree_item->pixmaps = NULL;
    }
}

static void
btk_tree_item_realize (BtkWidget *widget)
{
  BTK_WIDGET_CLASS (parent_class)->realize (widget);

  bdk_window_set_background (widget->window, 
			     &widget->style->base[BTK_STATE_NORMAL]);

  btk_tree_item_add_pixmaps (BTK_TREE_ITEM (widget));
}

static void
btk_tree_item_size_request (BtkWidget      *widget,
			    BtkRequisition *requisition)
{
  BtkBin *bin = BTK_BIN (widget);
  BtkTreeItem *item = BTK_TREE_ITEM (widget);
  BtkRequisition child_requisition;

  requisition->width = (BTK_CONTAINER (widget)->border_width +
			widget->style->xthickness) * 2;
  requisition->height = BTK_CONTAINER (widget)->border_width * 2;

  if (bin->child && btk_widget_get_visible (bin->child))
    {
      BtkRequisition pix_requisition;
      
      btk_widget_size_request (bin->child, &child_requisition);

      requisition->width += child_requisition.width;

      btk_widget_size_request (item->pixmaps_box, 
			       &pix_requisition);
      requisition->width += pix_requisition.width + DEFAULT_DELTA + 
	BTK_TREE (widget->parent)->current_indent;

      requisition->height += MAX (child_requisition.height,
				  pix_requisition.height);
    }
}

static void
btk_tree_item_size_allocate (BtkWidget     *widget,
			     BtkAllocation *allocation)
{
  BtkBin *bin = BTK_BIN (widget);
  BtkTreeItem *item = BTK_TREE_ITEM (widget);
  BtkAllocation child_allocation;
  bint border_width;
  int temp;

  widget->allocation = *allocation;
  if (btk_widget_get_realized (widget))
    bdk_window_move_resize (widget->window,
			    allocation->x, allocation->y,
			    allocation->width, allocation->height);

  if (bin->child)
    {
      border_width = (BTK_CONTAINER (widget)->border_width +
		      widget->style->xthickness);

      child_allocation.x = border_width + BTK_TREE(widget->parent)->current_indent;
      child_allocation.y = BTK_CONTAINER (widget)->border_width;

      child_allocation.width = item->pixmaps_box->requisition.width;
      child_allocation.height = item->pixmaps_box->requisition.height;
      
      temp = allocation->height - child_allocation.height;
      child_allocation.y += ( temp / 2 ) + ( temp % 2 );

      btk_widget_size_allocate (item->pixmaps_box, &child_allocation);

      child_allocation.y = BTK_CONTAINER (widget)->border_width;
      child_allocation.height = MAX (1, (bint)allocation->height - child_allocation.y * 2);
      child_allocation.x += item->pixmaps_box->requisition.width+DEFAULT_DELTA;

      child_allocation.width = 
	MAX (1, (bint)allocation->width - ((bint)child_allocation.x + border_width));

      btk_widget_size_allocate (bin->child, &child_allocation);
    }
}

static void 
btk_tree_item_draw_lines (BtkWidget *widget) 
{
  BtkTreeItem* item;
  BtkTree* tree;
  buint lx1, ly1, lx2, ly2;
  BdkGC* gc;

  g_return_if_fail (BTK_IS_TREE_ITEM (widget));

  item = BTK_TREE_ITEM(widget);
  tree = BTK_TREE(widget->parent);

  if (!tree->view_line)
    return;

  gc = widget->style->text_gc[BTK_STATE_NORMAL];

  /* draw vertical line */
  lx1 = item->pixmaps_box->allocation.width;
  lx1 = lx2 = ((lx1 / 2) + (lx1 % 2) + 
	       BTK_CONTAINER (widget)->border_width + 1 + tree->current_indent);
  ly1 = 0;
  ly2 = widget->allocation.height;

  if (g_list_last (tree->children)->data == widget)
    ly2 = (ly2 / 2) + (ly2 % 2);

  if (tree != tree->root_tree)
    bdk_draw_line (widget->window, gc, lx1, ly1, lx2, ly2);

  /* draw vertical line for subtree connecting */
  if(g_list_last(tree->children)->data != (bpointer)widget)
    ly2 = (ly2 / 2) + (ly2 % 2);
  
  lx2 += DEFAULT_DELTA;

  if (item->subtree && item->expanded)
    bdk_draw_line (widget->window, gc,
		   lx2, ly2, lx2, widget->allocation.height);

  /* draw horizontal line */
  ly1 = ly2;
  lx2 += 2;

  bdk_draw_line (widget->window, gc, lx1, ly1, lx2, ly2);

  lx2 -= DEFAULT_DELTA+2;
  ly1 = 0;
  ly2 = widget->allocation.height;

  if (tree != tree->root_tree)
    {
      item = BTK_TREE_ITEM (tree->tree_owner);
      tree = BTK_TREE (BTK_WIDGET (tree)->parent);
      while (tree != tree->root_tree)
	{
	  lx1 = lx2 -= tree->indent_value;
	  
	  if (g_list_last (tree->children)->data != item)
	    bdk_draw_line (widget->window, gc, lx1, ly1, lx2, ly2);
	  item = BTK_TREE_ITEM (tree->tree_owner);
	  tree = BTK_TREE (BTK_WIDGET (tree)->parent);
	} 
    }
}

static void
btk_tree_item_paint (BtkWidget    *widget,
		     BdkRectangle *area)
{
  BdkRectangle child_area, item_area;
  BtkTreeItem* tree_item;

  /* FIXME: We should honor tree->view_mode, here - I think
   * the desired effect is that when the mode is VIEW_ITEM,
   * only the subitem is drawn as selected, not the entire
   * line. (Like the way that the tree in Windows Explorer
   * works).
   */
  if (btk_widget_is_drawable (widget))
    {
      tree_item = BTK_TREE_ITEM(widget);

      if (widget->state == BTK_STATE_NORMAL)
	{
	  bdk_window_set_back_pixmap (widget->window, NULL, TRUE);
	  bdk_window_clear_area (widget->window, area->x, area->y, area->width, area->height);
	}
      else 
	{
	  if (!btk_widget_is_sensitive (widget))
	    btk_paint_flat_box(widget->style, widget->window,
			       widget->state, BTK_SHADOW_NONE,
			       area, widget, "treeitem",
			       0, 0, -1, -1);
	  else
	    btk_paint_flat_box(widget->style, widget->window,
			       widget->state, BTK_SHADOW_ETCHED_OUT,
			       area, widget, "treeitem",
			       0, 0, -1, -1);
	}

      /* draw left size of tree item */
      item_area.x = 0;
      item_area.y = 0;
      item_area.width = (tree_item->pixmaps_box->allocation.width + DEFAULT_DELTA +
			 BTK_TREE (widget->parent)->current_indent + 2);
      item_area.height = widget->allocation.height;


      if (bdk_rectangle_intersect(&item_area, area, &child_area)) 
	{
	  
	  btk_tree_item_draw_lines(widget);

	  if (tree_item->pixmaps_box && 
	      btk_widget_get_visible(tree_item->pixmaps_box) &&
	      btk_widget_intersect (tree_item->pixmaps_box, area, &child_area))
            {
              btk_widget_queue_draw_area (tree_item->pixmaps_box,
                                          child_area.x, child_area.y,
                                          child_area.width, child_area.height);
              bdk_window_process_updates (tree_item->pixmaps_box->window, TRUE);
            }
	}

      if (btk_widget_has_focus (widget))
	btk_paint_focus (widget->style, widget->window, btk_widget_get_state (widget),
			 NULL, widget, "treeitem",
			 0, 0,
			 widget->allocation.width,
			 widget->allocation.height);
      
    }
}

static bint
btk_tree_item_button_press (BtkWidget      *widget,
			    BdkEventButton *event)
{
  if (event->type == BDK_BUTTON_PRESS
      && btk_widget_is_sensitive(widget)
      && !btk_widget_has_focus (widget))
    btk_widget_grab_focus (widget);

  return (event->type == BDK_BUTTON_PRESS && btk_widget_is_sensitive(widget));
}

static void
btk_tree_item_expose_child (BtkWidget *child,
                            bpointer   client_data)
{
  struct {
    BtkWidget *container;
    BdkEventExpose *event;
  } *data = client_data;

  if (btk_widget_is_drawable (child) &&
      !btk_widget_get_has_window (child) &&
      (child->window == data->event->window))
    {
      BdkEvent *child_event = bdk_event_new (BDK_EXPOSE);
      child_event->expose = *data->event;
      g_object_ref (child_event->expose.window);

      child_event->expose.rebunnyion = btk_widget_rebunnyion_intersect (child,
								data->event->rebunnyion);
      if (!bdk_rebunnyion_empty (child_event->expose.rebunnyion))
        {
          bdk_rebunnyion_get_clipbox (child_event->expose.rebunnyion, &child_event->expose.area);
          btk_widget_send_expose (child, child_event);
	}
      bdk_event_free (child_event);
    }
}

static bint
btk_tree_item_expose (BtkWidget      *widget,
		      BdkEventExpose *event)
{
  struct {
    BtkWidget *container;
    BdkEventExpose *event;
  } data;

  if (btk_widget_is_drawable (widget))
    {
      btk_tree_item_paint (widget, &event->area);

      data.container = widget;
      data.event = event;

      btk_container_forall (BTK_CONTAINER (widget),
                            btk_tree_item_expose_child,
                            &data);
   }

  return FALSE;
}

static void
btk_real_tree_item_select (BtkItem *item)
{
  BtkWidget *widget;

  g_return_if_fail (BTK_IS_TREE_ITEM (item));

  widget = BTK_WIDGET (item);

  btk_widget_set_state (BTK_WIDGET (item), BTK_STATE_SELECTED);

  if (!widget->parent || BTK_TREE (widget->parent)->view_mode == BTK_TREE_VIEW_LINE)
    btk_widget_set_state (BTK_TREE_ITEM (item)->pixmaps_box, BTK_STATE_SELECTED);
}

static void
btk_real_tree_item_deselect (BtkItem *item)
{
  BtkTreeItem *tree_item;
  BtkWidget *widget;

  g_return_if_fail (BTK_IS_TREE_ITEM (item));

  tree_item = BTK_TREE_ITEM (item);
  widget = BTK_WIDGET (item);

  btk_widget_set_state (widget, BTK_STATE_NORMAL);

  if (!widget->parent || BTK_TREE (widget->parent)->view_mode == BTK_TREE_VIEW_LINE)
    btk_widget_set_state (tree_item->pixmaps_box, BTK_STATE_NORMAL);
}

static void
btk_real_tree_item_toggle (BtkItem *item)
{
  g_return_if_fail (BTK_IS_TREE_ITEM (item));

  if(!btk_widget_is_sensitive(BTK_WIDGET (item)))
    return;

  if (BTK_IS_TREE (BTK_WIDGET (item)->parent))
    btk_tree_select_child (BTK_TREE (BTK_WIDGET (item)->parent),
			   BTK_WIDGET (item));
  else
    {
      /* Should we really bother with this bit? A listitem not in a list?
       * -Johannes Keukelaar
       * yes, always be on the safe side!
       * -timj
       */
      if (BTK_WIDGET (item)->state == BTK_STATE_SELECTED)
	btk_widget_set_state (BTK_WIDGET (item), BTK_STATE_NORMAL);
      else
	btk_widget_set_state (BTK_WIDGET (item), BTK_STATE_SELECTED);
    }
}

static void
btk_real_tree_item_expand (BtkTreeItem *tree_item)
{
  BtkTree* tree;
  
  g_return_if_fail (BTK_IS_TREE_ITEM (tree_item));
  
  if (tree_item->subtree && !tree_item->expanded)
    {
      tree = BTK_TREE (BTK_WIDGET (tree_item)->parent); 
      
      /* hide subtree widget */
      btk_widget_show (tree_item->subtree);
      
      /* hide button '+' and show button '-' */
      if (tree_item->pixmaps_box)
	{
	  btk_container_remove (BTK_CONTAINER (tree_item->pixmaps_box), 
				tree_item->plus_pix_widget);
	  btk_container_add (BTK_CONTAINER (tree_item->pixmaps_box), 
			     tree_item->minus_pix_widget);
	}
      if (tree->root_tree)
	btk_widget_queue_resize (BTK_WIDGET (tree->root_tree));
      tree_item->expanded = TRUE;
    }
}

static void
btk_real_tree_item_collapse (BtkTreeItem *tree_item)
{
  BtkTree* tree;
  
  g_return_if_fail (BTK_IS_TREE_ITEM (tree_item));
  
  if (tree_item->subtree && tree_item->expanded) 
    {
      tree = BTK_TREE (BTK_WIDGET (tree_item)->parent);
      
      /* hide subtree widget */
      btk_widget_hide (tree_item->subtree);
      
      /* hide button '-' and show button '+' */
      if (tree_item->pixmaps_box)
	{
	  btk_container_remove (BTK_CONTAINER (tree_item->pixmaps_box), 
				tree_item->minus_pix_widget);
	  btk_container_add (BTK_CONTAINER (tree_item->pixmaps_box), 
			     tree_item->plus_pix_widget);
	}
      if (tree->root_tree)
	btk_widget_queue_resize (BTK_WIDGET (tree->root_tree));
      tree_item->expanded = FALSE;
    }
}

static void
btk_tree_item_destroy (BtkObject *object)
{
  BtkTreeItem* item = BTK_TREE_ITEM(object);
  BtkWidget* child;

#ifdef TREE_DEBUG
  g_message("+ btk_tree_item_destroy [object %#x]\n", (int)object);
#endif /* TREE_DEBUG */

  /* free sub tree if it exist */
  child = item->subtree;
  if (child)
    {
      g_object_ref (child);
      btk_widget_unparent (child);
      btk_widget_destroy (child);
      g_object_unref (child);
      item->subtree = NULL;
    }
  
  /* free pixmaps box */
  child = item->pixmaps_box;
  if (child)
    {
      g_object_ref (child);
      btk_widget_unparent (child);
      btk_widget_destroy (child);
      g_object_unref (child);
      item->pixmaps_box = NULL;
    }
  
  
  /* destroy plus pixmap */
  if (item->plus_pix_widget)
    {
      btk_widget_destroy (item->plus_pix_widget);
      g_object_unref (item->plus_pix_widget);
      item->plus_pix_widget = NULL;
    }
  
  /* destroy minus pixmap */
  if (item->minus_pix_widget)
    {
      btk_widget_destroy (item->minus_pix_widget);
      g_object_unref (item->minus_pix_widget);
      item->minus_pix_widget = NULL;
    }
  
  /* By removing the pixmaps here, and not in unrealize, we depend on
   * the fact that a widget can never change colormap or visual.
   */
  btk_tree_item_remove_pixmaps (item);
  
  BTK_OBJECT_CLASS (parent_class)->destroy (object);
  
#ifdef TREE_DEBUG
  g_message("- btk_tree_item_destroy\n");
#endif /* TREE_DEBUG */
}

void
btk_tree_item_remove_subtree (BtkTreeItem* item) 
{
  g_return_if_fail (BTK_IS_TREE_ITEM(item));
  g_return_if_fail (item->subtree != NULL);
  
  if (BTK_TREE (item->subtree)->children)
    {
      /* The following call will remove the children and call
       * btk_tree_item_remove_subtree() again. So we are done.
       */
      btk_tree_remove_items (BTK_TREE (item->subtree), 
			     BTK_TREE (item->subtree)->children);
      return;
    }

  if (btk_widget_get_mapped (item->subtree))
    btk_widget_unmap (item->subtree);
      
  btk_widget_unparent (item->subtree);
  
  if (item->pixmaps_box)
    btk_widget_hide (item->pixmaps_box);
  
  item->subtree = NULL;

  if (item->expanded)
    {
      item->expanded = FALSE;
      if (item->pixmaps_box)
	{
	  btk_container_remove (BTK_CONTAINER (item->pixmaps_box), 
				item->minus_pix_widget);
	  btk_container_add (BTK_CONTAINER (item->pixmaps_box), 
			     item->plus_pix_widget);
	}
    }
}

static void
btk_tree_item_map (BtkWidget *widget)
{
  BtkBin *bin = BTK_BIN (widget);
  BtkTreeItem* item = BTK_TREE_ITEM(widget);

  btk_widget_set_mapped (widget, TRUE);

  if(item->pixmaps_box &&
     btk_widget_get_visible (item->pixmaps_box) &&
     !btk_widget_get_mapped (item->pixmaps_box))
    btk_widget_map (item->pixmaps_box);

  if (bin->child &&
      btk_widget_get_visible (bin->child) &&
      !btk_widget_get_mapped (bin->child))
    btk_widget_map (bin->child);

  bdk_window_show (widget->window);
}

static void
btk_tree_item_unmap (BtkWidget *widget)
{
  BtkBin *bin = BTK_BIN (widget);
  BtkTreeItem* item = BTK_TREE_ITEM(widget);

  btk_widget_set_mapped (widget, FALSE);

  bdk_window_hide (widget->window);

  if(item->pixmaps_box &&
     btk_widget_get_visible (item->pixmaps_box) &&
     btk_widget_get_mapped (item->pixmaps_box))
    btk_widget_unmap (bin->child);

  if (bin->child &&
      btk_widget_get_visible (bin->child) &&
      btk_widget_get_mapped (bin->child))
    btk_widget_unmap (bin->child);
}

static void
btk_tree_item_forall (BtkContainer *container,
		      bboolean      include_internals,
		      BtkCallback   callback,
		      bpointer      callback_data)
{
  BtkBin *bin = BTK_BIN (container);
  BtkTreeItem *tree_item = BTK_TREE_ITEM (container);

  if (bin->child)
    (* callback) (bin->child, callback_data);
  if (include_internals && tree_item->subtree)
    (* callback) (tree_item->subtree, callback_data);
  if (include_internals && tree_item->pixmaps_box)
    (* callback) (tree_item->pixmaps_box, callback_data);
}

#define __BTK_TREE_ITEM_C__
#include "btkaliasdef.c"
