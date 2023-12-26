/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 1998 Elliot Lee
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

#include "config.h"
#include <stdlib.h>
#include "btkhandlebox.h"
#include "btkinvisible.h"
#include "btkmain.h"
#include "btkmarshalers.h"
#include "btkwindow.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkalias.h"

typedef struct _BtkHandleBoxPrivate BtkHandleBoxPrivate;

struct _BtkHandleBoxPrivate
{
  gint orig_x;
  gint orig_y;
};

enum {
  PROP_0,
  PROP_SHADOW,
  PROP_SHADOW_TYPE,
  PROP_HANDLE_POSITION,
  PROP_SNAP_EDGE,
  PROP_SNAP_EDGE_SET,
  PROP_CHILD_DETACHED
};

#define DRAG_HANDLE_SIZE 10
#define CHILDLESS_SIZE	25
#define GHOST_HEIGHT 3
#define TOLERANCE 5

enum {
  SIGNAL_CHILD_ATTACHED,
  SIGNAL_CHILD_DETACHED,
  SIGNAL_LAST
};

/* The algorithm for docking and redocking implemented here
 * has a couple of nice properties:
 *
 * 1) During a single drag, docking always occurs at the
 *    the same cursor position. This means that the users
 *    motions are reversible, and that you won't
 *    undock/dock oscillations.
 *
 * 2) Docking generally occurs at user-visible features.
 *    The user, once they figure out to redock, will
 *    have useful information about doing it again in
 *    the future.
 *
 * Please try to preserve these properties if you
 * change the algorithm. (And the current algorithm
 * is far from ideal). Briefly, the current algorithm
 * for deciding whether the handlebox is docked or not:
 *
 * 1) The decision is done by comparing two rectangles - the
 *    allocation if the widget at the start of the drag,
 *    and the boundary of hb->bin_window at the start of
 *    of the drag offset by the distance that the cursor
 *    has moved.
 *
 * 2) These rectangles must have one edge, the "snap_edge"
 *    of the handlebox, aligned within TOLERANCE.
 * 
 * 3) On the other dimension, the extents of one rectangle
 *    must be contained in the extents of the other,
 *    extended by tolerance. That is, either we can have:
 *
 * <-TOLERANCE-|--------bin_window--------------|-TOLERANCE->
 *         <--------float_window-------------------->
 *
 * or we can have:
 *
 * <-TOLERANCE-|------float_window--------------|-TOLERANCE->
 *          <--------bin_window-------------------->
 */

static void     btk_handle_box_set_property  (BObject        *object,
                                              guint           param_id,
                                              const BValue   *value,
                                              BParamSpec     *pspec);
static void     btk_handle_box_get_property  (BObject        *object,
                                              guint           param_id,
                                              BValue         *value,
                                              BParamSpec     *pspec);
static void     btk_handle_box_map           (BtkWidget      *widget);
static void     btk_handle_box_unmap         (BtkWidget      *widget);
static void     btk_handle_box_realize       (BtkWidget      *widget);
static void     btk_handle_box_unrealize     (BtkWidget      *widget);
static void     btk_handle_box_style_set     (BtkWidget      *widget,
                                              BtkStyle       *previous_style);
static void     btk_handle_box_size_request  (BtkWidget      *widget,
                                              BtkRequisition *requisition);
static void     btk_handle_box_size_allocate (BtkWidget      *widget,
                                              BtkAllocation  *real_allocation);
static void     btk_handle_box_add           (BtkContainer   *container,
                                              BtkWidget      *widget);
static void     btk_handle_box_remove        (BtkContainer   *container,
                                              BtkWidget      *widget);
static void     btk_handle_box_draw_ghost    (BtkHandleBox   *hb);
static void     btk_handle_box_paint         (BtkWidget      *widget,
                                              BdkEventExpose *event,
                                              BdkRectangle   *area);
static gboolean btk_handle_box_expose        (BtkWidget      *widget,
                                              BdkEventExpose *event);
static gboolean btk_handle_box_button_press  (BtkWidget      *widget,
                                              BdkEventButton *event);
static gboolean btk_handle_box_motion        (BtkWidget      *widget,
                                              BdkEventMotion *event);
static gboolean btk_handle_box_delete_event  (BtkWidget      *widget,
                                              BdkEventAny    *event);
static void     btk_handle_box_reattach      (BtkHandleBox   *hb);
static void     btk_handle_box_end_drag      (BtkHandleBox   *hb,
                                              guint32         time);

static guint handle_box_signals[SIGNAL_LAST] = { 0 };

G_DEFINE_TYPE (BtkHandleBox, btk_handle_box, BTK_TYPE_BIN)

static void
btk_handle_box_class_init (BtkHandleBoxClass *class)
{
  BObjectClass *bobject_class;
  BtkWidgetClass *widget_class;
  BtkContainerClass *container_class;

  bobject_class = (BObjectClass *) class;
  widget_class = (BtkWidgetClass *) class;
  container_class = (BtkContainerClass *) class;

  bobject_class->set_property = btk_handle_box_set_property;
  bobject_class->get_property = btk_handle_box_get_property;
  
  g_object_class_install_property (bobject_class,
                                   PROP_SHADOW,
                                   g_param_spec_enum ("shadow", NULL,
                                                      P_("Deprecated property, use shadow_type instead"),
						      BTK_TYPE_SHADOW_TYPE,
						      BTK_SHADOW_OUT,
                                                      BTK_PARAM_READWRITE | G_PARAM_DEPRECATED));
  g_object_class_install_property (bobject_class,
                                   PROP_SHADOW_TYPE,
                                   g_param_spec_enum ("shadow-type",
                                                      P_("Shadow type"),
                                                      P_("Appearance of the shadow that surrounds the container"),
						      BTK_TYPE_SHADOW_TYPE,
						      BTK_SHADOW_OUT,
                                                      BTK_PARAM_READWRITE));
  
  g_object_class_install_property (bobject_class,
                                   PROP_HANDLE_POSITION,
                                   g_param_spec_enum ("handle-position",
                                                      P_("Handle position"),
                                                      P_("Position of the handle relative to the child widget"),
						      BTK_TYPE_POSITION_TYPE,
						      BTK_POS_LEFT,
                                                      BTK_PARAM_READWRITE));
  
  g_object_class_install_property (bobject_class,
                                   PROP_SNAP_EDGE,
                                   g_param_spec_enum ("snap-edge",
                                                      P_("Snap edge"),
                                                      P_("Side of the handlebox that's lined up with the docking point to dock the handlebox"),
						      BTK_TYPE_POSITION_TYPE,
						      BTK_POS_TOP,
                                                      BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_SNAP_EDGE_SET,
                                   g_param_spec_boolean ("snap-edge-set",
							 P_("Snap edge set"),
							 P_("Whether to use the value from the snap_edge property or a value derived from handle_position"),
							 FALSE,
							 BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_CHILD_DETACHED,
                                   g_param_spec_boolean ("child-detached",
							 P_("Child Detached"),
							 P_("A boolean value indicating whether the handlebox's child is attached or detached."),
							 FALSE,
							 BTK_PARAM_READABLE));

  widget_class->map = btk_handle_box_map;
  widget_class->unmap = btk_handle_box_unmap;
  widget_class->realize = btk_handle_box_realize;
  widget_class->unrealize = btk_handle_box_unrealize;
  widget_class->style_set = btk_handle_box_style_set;
  widget_class->size_request = btk_handle_box_size_request;
  widget_class->size_allocate = btk_handle_box_size_allocate;
  widget_class->expose_event = btk_handle_box_expose;
  widget_class->button_press_event = btk_handle_box_button_press;
  widget_class->delete_event = btk_handle_box_delete_event;

  container_class->add = btk_handle_box_add;
  container_class->remove = btk_handle_box_remove;

  class->child_attached = NULL;
  class->child_detached = NULL;

  handle_box_signals[SIGNAL_CHILD_ATTACHED] =
    g_signal_new (I_("child-attached"),
		  B_OBJECT_CLASS_TYPE (bobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkHandleBoxClass, child_attached),
		  NULL, NULL,
		  _btk_marshal_VOID__OBJECT,
		  B_TYPE_NONE, 1,
		  BTK_TYPE_WIDGET);
  handle_box_signals[SIGNAL_CHILD_DETACHED] =
    g_signal_new (I_("child-detached"),
		  B_OBJECT_CLASS_TYPE (bobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkHandleBoxClass, child_detached),
		  NULL, NULL,
		  _btk_marshal_VOID__OBJECT,
		  B_TYPE_NONE, 1,
		  BTK_TYPE_WIDGET);

  g_type_class_add_private (bobject_class, sizeof (BtkHandleBoxPrivate));    
}

static BtkHandleBoxPrivate *
btk_handle_box_get_private (BtkHandleBox *hb)
{
  return B_TYPE_INSTANCE_GET_PRIVATE (hb, BTK_TYPE_HANDLE_BOX, BtkHandleBoxPrivate);
}

static void
btk_handle_box_init (BtkHandleBox *handle_box)
{
  btk_widget_set_has_window (BTK_WIDGET (handle_box), TRUE);

  handle_box->bin_window = NULL;
  handle_box->float_window = NULL;
  handle_box->shadow_type = BTK_SHADOW_OUT;
  handle_box->handle_position = BTK_POS_LEFT;
  handle_box->float_window_mapped = FALSE;
  handle_box->child_detached = FALSE;
  handle_box->in_drag = FALSE;
  handle_box->shrink_on_detach = TRUE;
  handle_box->snap_edge = -1;
}

static void 
btk_handle_box_set_property (BObject         *object,
			     guint            prop_id,
			     const BValue    *value,
			     BParamSpec      *pspec)
{
  BtkHandleBox *handle_box = BTK_HANDLE_BOX (object);

  switch (prop_id)
    {
    case PROP_SHADOW:
    case PROP_SHADOW_TYPE:
      btk_handle_box_set_shadow_type (handle_box, b_value_get_enum (value));
      break;
    case PROP_HANDLE_POSITION:
      btk_handle_box_set_handle_position (handle_box, b_value_get_enum (value));
      break;
    case PROP_SNAP_EDGE:
      btk_handle_box_set_snap_edge (handle_box, b_value_get_enum (value));
      break;
    case PROP_SNAP_EDGE_SET:
      if (!b_value_get_boolean (value))
	btk_handle_box_set_snap_edge (handle_box, (BtkPositionType)-1);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void 
btk_handle_box_get_property (BObject         *object,
			     guint            prop_id,
			     BValue          *value,
			     BParamSpec      *pspec)
{
  BtkHandleBox *handle_box = BTK_HANDLE_BOX (object);
  
  switch (prop_id)
    {
    case PROP_SHADOW:
    case PROP_SHADOW_TYPE:
      b_value_set_enum (value, handle_box->shadow_type);
      break;
    case PROP_HANDLE_POSITION:
      b_value_set_enum (value, handle_box->handle_position);
      break;
    case PROP_SNAP_EDGE:
      b_value_set_enum (value,
			(handle_box->snap_edge == -1 ?
			 BTK_POS_TOP : handle_box->snap_edge));
      break;
    case PROP_SNAP_EDGE_SET:
      b_value_set_boolean (value, handle_box->snap_edge != -1);
      break;
    case PROP_CHILD_DETACHED:
      b_value_set_boolean (value, handle_box->child_detached);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}
 
BtkWidget*
btk_handle_box_new (void)
{
  return g_object_new (BTK_TYPE_HANDLE_BOX, NULL);
}

static void
btk_handle_box_map (BtkWidget *widget)
{
  BtkBin *bin;
  BtkHandleBox *hb;

  btk_widget_set_mapped (widget, TRUE);

  bin = BTK_BIN (widget);
  hb = BTK_HANDLE_BOX (widget);

  if (bin->child &&
      btk_widget_get_visible (bin->child) &&
      !btk_widget_get_mapped (bin->child))
    btk_widget_map (bin->child);

  if (hb->child_detached && !hb->float_window_mapped)
    {
      bdk_window_show (hb->float_window);
      hb->float_window_mapped = TRUE;
    }

  bdk_window_show (hb->bin_window);
  bdk_window_show (widget->window);
}

static void
btk_handle_box_unmap (BtkWidget *widget)
{
  BtkHandleBox *hb;

  btk_widget_set_mapped (widget, FALSE);

  hb = BTK_HANDLE_BOX (widget);

  bdk_window_hide (widget->window);
  if (hb->float_window_mapped)
    {
      bdk_window_hide (hb->float_window);
      hb->float_window_mapped = FALSE;
    }
}

static void
btk_handle_box_realize (BtkWidget *widget)
{
  BdkWindowAttr attributes;
  gint attributes_mask;
  BtkHandleBox *hb;

  hb = BTK_HANDLE_BOX (widget);

  btk_widget_set_realized (widget, TRUE);

  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.window_type = BDK_WINDOW_CHILD;
  attributes.wclass = BDK_INPUT_OUTPUT;
  attributes.visual = btk_widget_get_visual (widget);
  attributes.colormap = btk_widget_get_colormap (widget);
  attributes.event_mask = (btk_widget_get_events (widget)
			   | BDK_EXPOSURE_MASK);
  attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_VISUAL | BDK_WA_COLORMAP;
  widget->window = bdk_window_new (btk_widget_get_parent_window (widget), &attributes, attributes_mask);
  bdk_window_set_user_data (widget->window, widget);

  attributes.x = 0;
  attributes.y = 0;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.window_type = BDK_WINDOW_CHILD;
  attributes.event_mask = (btk_widget_get_events (widget) |
			   BDK_EXPOSURE_MASK |
			   BDK_BUTTON1_MOTION_MASK |
			   BDK_POINTER_MOTION_HINT_MASK |
			   BDK_BUTTON_PRESS_MASK |
			    BDK_BUTTON_RELEASE_MASK);
  attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_VISUAL | BDK_WA_COLORMAP;
  hb->bin_window = bdk_window_new (widget->window, &attributes, attributes_mask);
  bdk_window_set_user_data (hb->bin_window, widget);
  if (BTK_BIN (hb)->child)
    btk_widget_set_parent_window (BTK_BIN (hb)->child, hb->bin_window);
  
  attributes.x = 0;
  attributes.y = 0;
  attributes.width = widget->requisition.width;
  attributes.height = widget->requisition.height;
  attributes.window_type = BDK_WINDOW_TOPLEVEL;
  attributes.wclass = BDK_INPUT_OUTPUT;
  attributes.visual = btk_widget_get_visual (widget);
  attributes.colormap = btk_widget_get_colormap (widget);
  attributes.event_mask = (btk_widget_get_events (widget) |
			   BDK_KEY_PRESS_MASK |
			   BDK_ENTER_NOTIFY_MASK |
			   BDK_LEAVE_NOTIFY_MASK |
			   BDK_FOCUS_CHANGE_MASK |
			   BDK_STRUCTURE_MASK);
  attributes.type_hint = BDK_WINDOW_TYPE_HINT_TOOLBAR;
  attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_VISUAL | BDK_WA_COLORMAP | BDK_WA_TYPE_HINT;
  hb->float_window = bdk_window_new (btk_widget_get_root_window (widget),
				     &attributes, attributes_mask);
  bdk_window_set_user_data (hb->float_window, widget);
  bdk_window_set_decorations (hb->float_window, 0);
  bdk_window_set_type_hint (hb->float_window, BDK_WINDOW_TYPE_HINT_TOOLBAR);
  
  widget->style = btk_style_attach (widget->style, widget->window);
  btk_style_set_background (widget->style, widget->window, btk_widget_get_state (widget));
  btk_style_set_background (widget->style, hb->bin_window, btk_widget_get_state (widget));
  btk_style_set_background (widget->style, hb->float_window, btk_widget_get_state (widget));
  bdk_window_set_back_pixmap (widget->window, NULL, TRUE);
}

static void
btk_handle_box_unrealize (BtkWidget *widget)
{
  BtkHandleBox *hb = BTK_HANDLE_BOX (widget);

  bdk_window_set_user_data (hb->bin_window, NULL);
  bdk_window_destroy (hb->bin_window);
  hb->bin_window = NULL;
  bdk_window_set_user_data (hb->float_window, NULL);
  bdk_window_destroy (hb->float_window);
  hb->float_window = NULL;

  BTK_WIDGET_CLASS (btk_handle_box_parent_class)->unrealize (widget);
}

static void
btk_handle_box_style_set (BtkWidget *widget,
			  BtkStyle  *previous_style)
{
  BtkHandleBox *hb = BTK_HANDLE_BOX (widget);

  if (btk_widget_get_realized (widget) &&
      btk_widget_get_has_window (widget))
    {
      btk_style_set_background (widget->style, widget->window,
				widget->state);
      btk_style_set_background (widget->style, hb->bin_window, widget->state);
      btk_style_set_background (widget->style, hb->float_window, widget->state);
    }
}

static int
effective_handle_position (BtkHandleBox *hb)
{
  int handle_position;

  if (btk_widget_get_direction (BTK_WIDGET (hb)) == BTK_TEXT_DIR_LTR)
    handle_position = hb->handle_position;
  else
    {
      switch (hb->handle_position) 
	{
	case BTK_POS_LEFT:
	  handle_position = BTK_POS_RIGHT;
	  break;
	case BTK_POS_RIGHT:
	  handle_position = BTK_POS_LEFT;
	  break;
	default:
	  handle_position = hb->handle_position;
	  break;
	}
    }

  return handle_position;
}

static void
btk_handle_box_size_request (BtkWidget      *widget,
			     BtkRequisition *requisition)
{
  BtkBin *bin;
  BtkHandleBox *hb;
  BtkRequisition child_requisition;
  gint handle_position;

  bin = BTK_BIN (widget);
  hb = BTK_HANDLE_BOX (widget);

  handle_position = effective_handle_position (hb);

  if (handle_position == BTK_POS_LEFT ||
      handle_position == BTK_POS_RIGHT)
    {
      requisition->width = DRAG_HANDLE_SIZE;
      requisition->height = 0;
    }
  else
    {
      requisition->width = 0;
      requisition->height = DRAG_HANDLE_SIZE;
    }

  /* if our child is not visible, we still request its size, since we
   * won't have any useful hint for our size otherwise.
   */
  if (bin->child)
    btk_widget_size_request (bin->child, &child_requisition);
  else
    {
      child_requisition.width = 0;
      child_requisition.height = 0;
    }      

  if (hb->child_detached)
    {
      /* FIXME: This doesn't work currently */
      if (!hb->shrink_on_detach)
	{
	  if (handle_position == BTK_POS_LEFT ||
	      handle_position == BTK_POS_RIGHT)
	    requisition->height += child_requisition.height;
	  else
	    requisition->width += child_requisition.width;
	}
      else
	{
	  if (handle_position == BTK_POS_LEFT ||
	      handle_position == BTK_POS_RIGHT)
	    requisition->height += widget->style->ythickness;
	  else
	    requisition->width += widget->style->xthickness;
	}
    }
  else
    {
      requisition->width += BTK_CONTAINER (widget)->border_width * 2;
      requisition->height += BTK_CONTAINER (widget)->border_width * 2;
      
      if (bin->child)
	{
	  requisition->width += child_requisition.width;
	  requisition->height += child_requisition.height;
	}
      else
	{
	  requisition->width += CHILDLESS_SIZE;
	  requisition->height += CHILDLESS_SIZE;
	}
    }
}

static void
btk_handle_box_size_allocate (BtkWidget     *widget,
			      BtkAllocation *allocation)
{
  BtkBin *bin;
  BtkHandleBox *hb;
  BtkRequisition child_requisition;
  gint handle_position;
  
  bin = BTK_BIN (widget);
  hb = BTK_HANDLE_BOX (widget);
  
  handle_position = effective_handle_position (hb);

  if (bin->child)
    btk_widget_get_child_requisition (bin->child, &child_requisition);
  else
    {
      child_requisition.width = 0;
      child_requisition.height = 0;
    }      
      
  widget->allocation = *allocation;

  if (btk_widget_get_realized (widget))
    bdk_window_move_resize (widget->window,
			    widget->allocation.x,
			    widget->allocation.y,
			    widget->allocation.width,
			    widget->allocation.height);


  if (bin->child && btk_widget_get_visible (bin->child))
    {
      BtkAllocation child_allocation;
      guint border_width;

      border_width = BTK_CONTAINER (widget)->border_width;

      child_allocation.x = border_width;
      child_allocation.y = border_width;
      if (handle_position == BTK_POS_LEFT)
	child_allocation.x += DRAG_HANDLE_SIZE;
      else if (handle_position == BTK_POS_TOP)
	child_allocation.y += DRAG_HANDLE_SIZE;

      if (hb->child_detached)
	{
	  guint float_width;
	  guint float_height;
	  
	  child_allocation.width = child_requisition.width;
	  child_allocation.height = child_requisition.height;
	  
	  float_width = child_allocation.width + 2 * border_width;
	  float_height = child_allocation.height + 2 * border_width;
	  
	  if (handle_position == BTK_POS_LEFT ||
	      handle_position == BTK_POS_RIGHT)
	    float_width += DRAG_HANDLE_SIZE;
	  else
	    float_height += DRAG_HANDLE_SIZE;

	  if (btk_widget_get_realized (widget))
	    {
	      bdk_window_resize (hb->float_window,
				 float_width,
				 float_height);
	      bdk_window_move_resize (hb->bin_window,
				      0,
				      0,
				      float_width,
				      float_height);
	    }
	}
      else
	{
	  child_allocation.width = MAX (1, (gint)widget->allocation.width - 2 * border_width);
	  child_allocation.height = MAX (1, (gint)widget->allocation.height - 2 * border_width);

	  if (handle_position == BTK_POS_LEFT ||
	      handle_position == BTK_POS_RIGHT)
	    child_allocation.width -= DRAG_HANDLE_SIZE;
	  else
	    child_allocation.height -= DRAG_HANDLE_SIZE;
	  
	  if (btk_widget_get_realized (widget))
	    bdk_window_move_resize (hb->bin_window,
				    0,
				    0,
				    widget->allocation.width,
				    widget->allocation.height);
	}

      btk_widget_size_allocate (bin->child, &child_allocation);
    }
}

static void
btk_handle_box_draw_ghost (BtkHandleBox *hb)
{
  BtkWidget *widget;
  guint x;
  guint y;
  guint width;
  guint height;
  gint handle_position;

  widget = BTK_WIDGET (hb);
  
  handle_position = effective_handle_position (hb);
  if (handle_position == BTK_POS_LEFT ||
      handle_position == BTK_POS_RIGHT)
    {
      x = handle_position == BTK_POS_LEFT ? 0 : widget->allocation.width - DRAG_HANDLE_SIZE;
      y = 0;
      width = DRAG_HANDLE_SIZE;
      height = widget->allocation.height;
    }
  else
    {
      x = 0;
      y = handle_position == BTK_POS_TOP ? 0 : widget->allocation.height - DRAG_HANDLE_SIZE;
      width = widget->allocation.width;
      height = DRAG_HANDLE_SIZE;
    }
  btk_paint_shadow (widget->style,
		    widget->window,
		    btk_widget_get_state (widget),
		    BTK_SHADOW_ETCHED_IN,
		    NULL, widget, "handle",
		    x,
		    y,
		    width,
		    height);
   if (handle_position == BTK_POS_LEFT ||
       handle_position == BTK_POS_RIGHT)
     btk_paint_hline (widget->style,
		      widget->window,
		      btk_widget_get_state (widget),
		      NULL, widget, "handlebox",
		      handle_position == BTK_POS_LEFT ? DRAG_HANDLE_SIZE : 0,
		      handle_position == BTK_POS_LEFT ? widget->allocation.width : widget->allocation.width - DRAG_HANDLE_SIZE,
		      widget->allocation.height / 2);
   else
     btk_paint_vline (widget->style,
		      widget->window,
		      btk_widget_get_state (widget),
		      NULL, widget, "handlebox",
		      handle_position == BTK_POS_TOP ? DRAG_HANDLE_SIZE : 0,
		      handle_position == BTK_POS_TOP ? widget->allocation.height : widget->allocation.height - DRAG_HANDLE_SIZE,
		      widget->allocation.width / 2);
}

static void
draw_textured_frame (BtkWidget *widget, BdkWindow *window, BdkRectangle *rect, BtkShadowType shadow,
		     BdkRectangle *clip, BtkOrientation orientation)
{
   btk_paint_handle (widget->style, window, BTK_STATE_NORMAL, shadow,
		     clip, widget, "handlebox",
		     rect->x, rect->y, rect->width, rect->height, 
		     orientation);
}

void
btk_handle_box_set_shadow_type (BtkHandleBox  *handle_box,
				BtkShadowType  type)
{
  g_return_if_fail (BTK_IS_HANDLE_BOX (handle_box));

  if ((BtkShadowType) handle_box->shadow_type != type)
    {
      handle_box->shadow_type = type;
      g_object_notify (B_OBJECT (handle_box), "shadow-type");
      btk_widget_queue_resize (BTK_WIDGET (handle_box));
    }
}

/**
 * btk_handle_box_get_shadow_type:
 * @handle_box: a #BtkHandleBox
 * 
 * Gets the type of shadow drawn around the handle box. See
 * btk_handle_box_set_shadow_type().
 *
 * Return value: the type of shadow currently drawn around the handle box.
 **/
BtkShadowType
btk_handle_box_get_shadow_type (BtkHandleBox *handle_box)
{
  g_return_val_if_fail (BTK_IS_HANDLE_BOX (handle_box), BTK_SHADOW_ETCHED_OUT);

  return handle_box->shadow_type;
}

void        
btk_handle_box_set_handle_position  (BtkHandleBox    *handle_box,
				     BtkPositionType  position)
{
  g_return_if_fail (BTK_IS_HANDLE_BOX (handle_box));

  if ((BtkPositionType) handle_box->handle_position != position)
    {
      handle_box->handle_position = position;
      g_object_notify (B_OBJECT (handle_box), "handle-position");
      btk_widget_queue_resize (BTK_WIDGET (handle_box));
    }
}

/**
 * btk_handle_box_get_handle_position:
 * @handle_box: a #BtkHandleBox
 *
 * Gets the handle position of the handle box. See
 * btk_handle_box_set_handle_position().
 *
 * Return value: the current handle position.
 **/
BtkPositionType
btk_handle_box_get_handle_position (BtkHandleBox *handle_box)
{
  g_return_val_if_fail (BTK_IS_HANDLE_BOX (handle_box), BTK_POS_LEFT);

  return handle_box->handle_position;
}

void        
btk_handle_box_set_snap_edge        (BtkHandleBox    *handle_box,
				     BtkPositionType  edge)
{
  g_return_if_fail (BTK_IS_HANDLE_BOX (handle_box));

  if (handle_box->snap_edge != edge)
    {
      handle_box->snap_edge = edge;
      
      g_object_freeze_notify (B_OBJECT (handle_box));
      g_object_notify (B_OBJECT (handle_box), "snap-edge");
      g_object_notify (B_OBJECT (handle_box), "snap-edge-set");
      g_object_thaw_notify (B_OBJECT (handle_box));
    }
}

/**
 * btk_handle_box_get_snap_edge:
 * @handle_box: a #BtkHandleBox
 * 
 * Gets the edge used for determining reattachment of the handle box. See
 * btk_handle_box_set_snap_edge().
 *
 * Return value: the edge used for determining reattachment, or (BtkPositionType)-1 if this
 *               is determined (as per default) from the handle position. 
 **/
BtkPositionType
btk_handle_box_get_snap_edge (BtkHandleBox *handle_box)
{
  g_return_val_if_fail (BTK_IS_HANDLE_BOX (handle_box), (BtkPositionType)-1);

  return handle_box->snap_edge;
}

/**
 * btk_handle_box_get_child_detached:
 * @handle_box: a #BtkHandleBox
 *
 * Whether the handlebox's child is currently detached.
 *
 * Return value: %TRUE if the child is currently detached, otherwise %FALSE
 *
 * Since: 2.14
 **/
gboolean
btk_handle_box_get_child_detached (BtkHandleBox *handle_box)
{
  g_return_val_if_fail (BTK_IS_HANDLE_BOX (handle_box), FALSE);

  return handle_box->child_detached;
}

static void
btk_handle_box_paint (BtkWidget      *widget,
                      BdkEventExpose *event,
		      BdkRectangle   *area)
{
  BtkBin *bin;
  BtkHandleBox *hb;
  gint width, height;
  BdkRectangle rect;
  BdkRectangle dest;
  gint handle_position;
  BtkOrientation handle_orientation;

  bin = BTK_BIN (widget);
  hb = BTK_HANDLE_BOX (widget);

  handle_position = effective_handle_position (hb);

  width = bdk_window_get_width (hb->bin_window);
  height = bdk_window_get_height (hb->bin_window);
  
  if (!event)
    btk_paint_box (widget->style,
		   hb->bin_window,
		   btk_widget_get_state (widget),
		   hb->shadow_type,
		   area, widget, "handlebox_bin",
		   0, 0, -1, -1);
  else
   btk_paint_box (widget->style,
		  hb->bin_window,
		  btk_widget_get_state (widget),
		  hb->shadow_type,
		  &event->area, widget, "handlebox_bin",
		  0, 0, -1, -1);

/* We currently draw the handle _above_ the relief of the handlebox.
 * it could also be drawn on the same level...

		 hb->handle_position == BTK_POS_LEFT ? DRAG_HANDLE_SIZE : 0,
		 hb->handle_position == BTK_POS_TOP ? DRAG_HANDLE_SIZE : 0,
		 width,
		 height);*/

  switch (handle_position)
    {
    case BTK_POS_LEFT:
      rect.x = 0;
      rect.y = 0; 
      rect.width = DRAG_HANDLE_SIZE;
      rect.height = height;
      handle_orientation = BTK_ORIENTATION_VERTICAL;
      break;
    case BTK_POS_RIGHT:
      rect.x = width - DRAG_HANDLE_SIZE; 
      rect.y = 0;
      rect.width = DRAG_HANDLE_SIZE;
      rect.height = height;
      handle_orientation = BTK_ORIENTATION_VERTICAL;
      break;
    case BTK_POS_TOP:
      rect.x = 0;
      rect.y = 0; 
      rect.width = width;
      rect.height = DRAG_HANDLE_SIZE;
      handle_orientation = BTK_ORIENTATION_HORIZONTAL;
      break;
    case BTK_POS_BOTTOM:
      rect.x = 0;
      rect.y = height - DRAG_HANDLE_SIZE;
      rect.width = width;
      rect.height = DRAG_HANDLE_SIZE;
      handle_orientation = BTK_ORIENTATION_HORIZONTAL;
      break;
    default: 
      g_assert_not_reached ();
      break;
    }

  if (bdk_rectangle_intersect (event ? &event->area : area, &rect, &dest))
    draw_textured_frame (widget, hb->bin_window, &rect,
			 BTK_SHADOW_OUT,
			 event ? &event->area : area,
			 handle_orientation);

  if (bin->child && btk_widget_get_visible (bin->child))
    BTK_WIDGET_CLASS (btk_handle_box_parent_class)->expose_event (widget, event);
}

static gboolean
btk_handle_box_expose (BtkWidget      *widget,
		       BdkEventExpose *event)
{
  BtkHandleBox *hb;

  if (btk_widget_is_drawable (widget))
    {
      hb = BTK_HANDLE_BOX (widget);

      if (event->window == widget->window)
	{
	  if (hb->child_detached)
	    btk_handle_box_draw_ghost (hb);
	}
      else
	btk_handle_box_paint (widget, event, NULL);
    }
  
  return FALSE;
}

static BtkWidget *
btk_handle_box_get_invisible (void)
{
  static BtkWidget *handle_box_invisible = NULL;

  if (!handle_box_invisible)
    {
      handle_box_invisible = btk_invisible_new ();
      btk_widget_show (handle_box_invisible);
    }
  
  return handle_box_invisible;
}

static gboolean
btk_handle_box_grab_event (BtkWidget    *widget,
			   BdkEvent     *event,
			   BtkHandleBox *hb)
{
  switch (event->type)
    {
    case BDK_BUTTON_RELEASE:
      if (hb->in_drag)		/* sanity check */
	{
	  btk_handle_box_end_drag (hb, event->button.time);
	  return TRUE;
	}
      break;

    case BDK_MOTION_NOTIFY:
      return btk_handle_box_motion (BTK_WIDGET (hb), (BdkEventMotion *)event);
      break;

    default:
      break;
    }

  return FALSE;
}

static gboolean
btk_handle_box_button_press (BtkWidget      *widget,
                             BdkEventButton *event)
{
  BtkHandleBox *hb;
  gboolean event_handled;
  BdkCursor *fleur;
  gint handle_position;

  hb = BTK_HANDLE_BOX (widget);

  handle_position = effective_handle_position (hb);

  event_handled = FALSE;
  if ((event->button == 1) && 
      (event->type == BDK_BUTTON_PRESS || event->type == BDK_2BUTTON_PRESS))
    {
      BtkWidget *child;
      gboolean in_handle;
      
      if (event->window != hb->bin_window)
	return FALSE;

      child = BTK_BIN (hb)->child;

      if (child)
	{
	  switch (handle_position)
	    {
	    case BTK_POS_LEFT:
	      in_handle = event->x < DRAG_HANDLE_SIZE;
	      break;
	    case BTK_POS_TOP:
	      in_handle = event->y < DRAG_HANDLE_SIZE;
	      break;
	    case BTK_POS_RIGHT:
	      in_handle = event->x > 2 * BTK_CONTAINER (hb)->border_width + child->allocation.width;
	      break;
	    case BTK_POS_BOTTOM:
	      in_handle = event->y > 2 * BTK_CONTAINER (hb)->border_width + child->allocation.height;
	      break;
	    default:
	      in_handle = FALSE;
	      break;
	    }
	}
      else
	{
	  in_handle = FALSE;
	  event_handled = TRUE;
	}
      
      if (in_handle)
	{
	  if (event->type == BDK_BUTTON_PRESS) /* Start a drag */
	    {
	      BtkHandleBoxPrivate *private = btk_handle_box_get_private (hb);
	      BtkWidget *invisible = btk_handle_box_get_invisible ();
	      gint desk_x, desk_y;
	      gint root_x, root_y;
	      gint width, height;

              btk_invisible_set_screen (BTK_INVISIBLE (invisible),
                                        btk_widget_get_screen (BTK_WIDGET (hb)));
	      bdk_window_get_deskrelative_origin (hb->bin_window, &desk_x, &desk_y);
	      bdk_window_get_origin (hb->bin_window, &root_x, &root_y);
	      width = bdk_window_get_width (hb->bin_window);
	      height = bdk_window_get_height (hb->bin_window);
		  
	      private->orig_x = event->x_root;
	      private->orig_y = event->y_root;
		  
	      hb->float_allocation.x = root_x - event->x_root;
	      hb->float_allocation.y = root_y - event->y_root;
	      hb->float_allocation.width = width;
	      hb->float_allocation.height = height;
	      
	      hb->deskoff_x = desk_x - root_x;
	      hb->deskoff_y = desk_y - root_y;
	      
	      if (bdk_window_is_viewable (widget->window))
		{
		  bdk_window_get_origin (widget->window, &root_x, &root_y);
		  width = bdk_window_get_width (widget->window);
		  height = bdk_window_get_height (widget->window);
	      
		  hb->attach_allocation.x = root_x;
		  hb->attach_allocation.y = root_y;
		  hb->attach_allocation.width = width;
		  hb->attach_allocation.height = height;
		}
	      else
		{
		  hb->attach_allocation.x = -1;
		  hb->attach_allocation.y = -1;
		  hb->attach_allocation.width = 0;
		  hb->attach_allocation.height = 0;
		}
	      hb->in_drag = TRUE;
	      fleur = bdk_cursor_new_for_display (btk_widget_get_display (widget),
						  BDK_FLEUR);
	      if (bdk_pointer_grab (invisible->window,
				    FALSE,
				    (BDK_BUTTON1_MOTION_MASK |
				     BDK_POINTER_MOTION_HINT_MASK |
				     BDK_BUTTON_RELEASE_MASK),
				    NULL,
				    fleur,
				    event->time) != 0)
		{
		  hb->in_drag = FALSE;
		}
	      else
		{
		  btk_grab_add (invisible);
		  g_signal_connect (invisible, "event",
				    G_CALLBACK (btk_handle_box_grab_event), hb);
		}
	      
	      bdk_cursor_unref (fleur);
	      event_handled = TRUE;
	    }
	  else if (hb->child_detached) /* Double click */
	    {
	      btk_handle_box_reattach (hb);
	    }
	}
    }
  
  return event_handled;
}

static gboolean
btk_handle_box_motion (BtkWidget      *widget,
		       BdkEventMotion *event)
{
  BtkHandleBox *hb = BTK_HANDLE_BOX (widget);
  gint new_x, new_y;
  gint snap_edge;
  gboolean is_snapped = FALSE;
  gint handle_position;
  BdkGeometry geometry;
  BdkScreen *screen, *pointer_screen;

  if (!hb->in_drag)
    return FALSE;
  handle_position = effective_handle_position (hb);

  /* Calculate the attachment point on the float, if the float
   * were detached
   */
  new_x = 0;
  new_y = 0;
  screen = btk_widget_get_screen (widget);
  bdk_display_get_pointer (bdk_screen_get_display (screen),
			   &pointer_screen, 
			   &new_x, &new_y, NULL);
  if (pointer_screen != screen)
    {
      BtkHandleBoxPrivate *private = btk_handle_box_get_private (hb);

      new_x = private->orig_x;
      new_y = private->orig_y;
    }
  
  new_x += hb->float_allocation.x;
  new_y += hb->float_allocation.y;

  snap_edge = hb->snap_edge;
  if (snap_edge == -1)
    snap_edge = (handle_position == BTK_POS_LEFT ||
		 handle_position == BTK_POS_RIGHT) ?
      BTK_POS_TOP : BTK_POS_LEFT;

  if (btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL) 
    switch (snap_edge) 
      {
      case BTK_POS_LEFT:
	snap_edge = BTK_POS_RIGHT;
	break;
      case BTK_POS_RIGHT:
	snap_edge = BTK_POS_LEFT;
	break;
      default:
	break;
      }

  /* First, check if the snapped edge is aligned
   */
  switch (snap_edge)
    {
    case BTK_POS_TOP:
      is_snapped = abs (hb->attach_allocation.y - new_y) < TOLERANCE;
      break;
    case BTK_POS_BOTTOM:
      is_snapped = abs (hb->attach_allocation.y + (gint)hb->attach_allocation.height -
			new_y - (gint)hb->float_allocation.height) < TOLERANCE;
      break;
    case BTK_POS_LEFT:
      is_snapped = abs (hb->attach_allocation.x - new_x) < TOLERANCE;
      break;
    case BTK_POS_RIGHT:
      is_snapped = abs (hb->attach_allocation.x + (gint)hb->attach_allocation.width -
			new_x - (gint)hb->float_allocation.width) < TOLERANCE;
      break;
    }

  /* Next, check if coordinates in the other direction are sufficiently
   * aligned
   */
  if (is_snapped)
    {
      gint float_pos1 = 0;	/* Initialize to suppress warnings */
      gint float_pos2 = 0;
      gint attach_pos1 = 0;
      gint attach_pos2 = 0;
      
      switch (snap_edge)
	{
	case BTK_POS_TOP:
	case BTK_POS_BOTTOM:
	  attach_pos1 = hb->attach_allocation.x;
	  attach_pos2 = hb->attach_allocation.x + hb->attach_allocation.width;
	  float_pos1 = new_x;
	  float_pos2 = new_x + hb->float_allocation.width;
	  break;
	case BTK_POS_LEFT:
	case BTK_POS_RIGHT:
	  attach_pos1 = hb->attach_allocation.y;
	  attach_pos2 = hb->attach_allocation.y + hb->attach_allocation.height;
	  float_pos1 = new_y;
	  float_pos2 = new_y + hb->float_allocation.height;
	  break;
	}

      is_snapped = ((attach_pos1 - TOLERANCE < float_pos1) && 
		    (attach_pos2 + TOLERANCE > float_pos2)) ||
	           ((float_pos1 - TOLERANCE < attach_pos1) &&
		    (float_pos2 + TOLERANCE > attach_pos2));
    }

  if (is_snapped)
    {
      if (hb->child_detached)
	{
	  hb->child_detached = FALSE;
	  bdk_window_hide (hb->float_window);
	  bdk_window_reparent (hb->bin_window, widget->window, 0, 0);
	  hb->float_window_mapped = FALSE;
	  g_signal_emit (hb,
			 handle_box_signals[SIGNAL_CHILD_ATTACHED],
			 0,
			 BTK_BIN (hb)->child);
	  
	  btk_widget_queue_resize (widget);
	}
    }
  else
    {
      gint width, height;

      width = bdk_window_get_width (hb->float_window);
      height = bdk_window_get_height (hb->float_window);
      new_x += hb->deskoff_x;
      new_y += hb->deskoff_y;

      switch (handle_position)
	{
	case BTK_POS_LEFT:
	  new_y += ((gint)hb->float_allocation.height - height) / 2;
	  break;
	case BTK_POS_RIGHT:
	  new_x += (gint)hb->float_allocation.width - width;
	  new_y += ((gint)hb->float_allocation.height - height) / 2;
	  break;
	case BTK_POS_TOP:
	  new_x += ((gint)hb->float_allocation.width - width) / 2;
	  break;
	case BTK_POS_BOTTOM:
	  new_x += ((gint)hb->float_allocation.width - width) / 2;
	  new_y += (gint)hb->float_allocation.height - height;
	  break;
	}

      if (hb->child_detached)
	{
	  bdk_window_move (hb->float_window, new_x, new_y);
	  bdk_window_raise (hb->float_window);
	}
      else
	{
	  gint width;
	  gint height;
	  BtkRequisition child_requisition;

	  hb->child_detached = TRUE;

	  if (BTK_BIN (hb)->child)
	    btk_widget_get_child_requisition (BTK_BIN (hb)->child, &child_requisition);
	  else
	    {
	      child_requisition.width = 0;
	      child_requisition.height = 0;
	    }      

	  width = child_requisition.width + 2 * BTK_CONTAINER (hb)->border_width;
	  height = child_requisition.height + 2 * BTK_CONTAINER (hb)->border_width;

	  if (handle_position == BTK_POS_LEFT || handle_position == BTK_POS_RIGHT)
	    width += DRAG_HANDLE_SIZE;
	  else
	    height += DRAG_HANDLE_SIZE;
	  
	  bdk_window_move_resize (hb->float_window, new_x, new_y, width, height);
	  bdk_window_reparent (hb->bin_window, hb->float_window, 0, 0);
	  bdk_window_set_geometry_hints (hb->float_window, &geometry, BDK_HINT_POS);
	  bdk_window_show (hb->float_window);
	  hb->float_window_mapped = TRUE;
#if	0
	  /* this extra move is necessary if we use decorations, or our
	   * window manager insists on decorations.
	   */
	  bdk_display_sync (btk_widget_get_display (widget));
	  bdk_window_move (hb->float_window, new_x, new_y);
	  bdk_display_sync (btk_widget_get_display (widget));
#endif	/* 0 */
	  g_signal_emit (hb,
			 handle_box_signals[SIGNAL_CHILD_DETACHED],
			 0,
			 BTK_BIN (hb)->child);
	  btk_handle_box_draw_ghost (hb);
	  
	  btk_widget_queue_resize (widget);
	}
    }

  return TRUE;
}

static void
btk_handle_box_add (BtkContainer *container,
		    BtkWidget    *widget)
{
  btk_widget_set_parent_window (widget, BTK_HANDLE_BOX (container)->bin_window);
  BTK_CONTAINER_CLASS (btk_handle_box_parent_class)->add (container, widget);
}

static void
btk_handle_box_remove (BtkContainer *container,
		       BtkWidget    *widget)
{
  BTK_CONTAINER_CLASS (btk_handle_box_parent_class)->remove (container, widget);

  btk_handle_box_reattach (BTK_HANDLE_BOX (container));
}

static gint
btk_handle_box_delete_event (BtkWidget *widget,
			     BdkEventAny  *event)
{
  BtkHandleBox *hb = BTK_HANDLE_BOX (widget);

  if (event->window == hb->float_window)
    {
      btk_handle_box_reattach (hb);
      
      return TRUE;
    }

  return FALSE;
}

static void
btk_handle_box_reattach (BtkHandleBox *hb)
{
  BtkWidget *widget = BTK_WIDGET (hb);
  
  if (hb->child_detached)
    {
      hb->child_detached = FALSE;
      if (btk_widget_get_realized (widget))
	{
	  bdk_window_hide (hb->float_window);
	  bdk_window_reparent (hb->bin_window, widget->window, 0, 0);

	  if (BTK_BIN (hb)->child)
	    g_signal_emit (hb,
			   handle_box_signals[SIGNAL_CHILD_ATTACHED],
			   0,
			   BTK_BIN (hb)->child);

	}
      hb->float_window_mapped = FALSE;
    }
  if (hb->in_drag)
    btk_handle_box_end_drag (hb, BDK_CURRENT_TIME);

  btk_widget_queue_resize (BTK_WIDGET (hb));
}

static void
btk_handle_box_end_drag (BtkHandleBox *hb,
			 guint32       time)
{
  BtkWidget *invisible = btk_handle_box_get_invisible ();
		
  hb->in_drag = FALSE;

  btk_grab_remove (invisible);
  bdk_pointer_ungrab (time);
  g_signal_handlers_disconnect_by_func (invisible,
					G_CALLBACK (btk_handle_box_grab_event),
					hb);
}

#define __BTK_HANDLE_BOX_C__
#include "btkaliasdef.c"
