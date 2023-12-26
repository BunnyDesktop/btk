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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * BtkLayout: Widget for scrolling of arbitrary-sized areas.
 *
 * Copyright Owen Taylor, 1998
 */

/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#include "config.h"
#include "bdkconfig.h"

#include "btklayout.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkmarshalers.h"
#include "btkalias.h"

typedef struct _BtkLayoutChild   BtkLayoutChild;

struct _BtkLayoutChild {
  BtkWidget *widget;
  gint x;
  gint y;
};

enum {
   PROP_0,
   PROP_HADJUSTMENT,
   PROP_VADJUSTMENT,
   PROP_WIDTH,
   PROP_HEIGHT
};

enum {
  CHILD_PROP_0,
  CHILD_PROP_X,
  CHILD_PROP_Y
};

static void btk_layout_get_property       (GObject        *object,
                                           guint           prop_id,
                                           GValue         *value,
                                           GParamSpec     *pspec);
static void btk_layout_set_property       (GObject        *object,
                                           guint           prop_id,
                                           const GValue   *value,
                                           GParamSpec     *pspec);
static GObject *btk_layout_constructor    (GType                  type,
					   guint                  n_properties,
					   GObjectConstructParam *properties);
static void btk_layout_finalize           (GObject        *object);
static void btk_layout_realize            (BtkWidget      *widget);
static void btk_layout_unrealize          (BtkWidget      *widget);
static void btk_layout_map                (BtkWidget      *widget);
static void btk_layout_size_request       (BtkWidget      *widget,
                                           BtkRequisition *requisition);
static void btk_layout_size_allocate      (BtkWidget      *widget,
                                           BtkAllocation  *allocation);
static gint btk_layout_expose             (BtkWidget      *widget,
                                           BdkEventExpose *event);
static void btk_layout_add                (BtkContainer   *container,
					   BtkWidget      *widget);
static void btk_layout_remove             (BtkContainer   *container,
                                           BtkWidget      *widget);
static void btk_layout_forall             (BtkContainer   *container,
                                           gboolean        include_internals,
                                           BtkCallback     callback,
                                           gpointer        callback_data);
static void btk_layout_set_adjustments    (BtkLayout      *layout,
                                           BtkAdjustment  *hadj,
                                           BtkAdjustment  *vadj);
static void btk_layout_set_child_property (BtkContainer   *container,
                                           BtkWidget      *child,
                                           guint           property_id,
                                           const GValue   *value,
                                           GParamSpec     *pspec);
static void btk_layout_get_child_property (BtkContainer   *container,
                                           BtkWidget      *child,
                                           guint           property_id,
                                           GValue         *value,
                                           GParamSpec     *pspec);
static void btk_layout_allocate_child     (BtkLayout      *layout,
                                           BtkLayoutChild *child);
static void btk_layout_adjustment_changed (BtkAdjustment  *adjustment,
                                           BtkLayout      *layout);
static void btk_layout_style_set          (BtkWidget      *widget,
					   BtkStyle       *old_style);

static void btk_layout_set_adjustment_upper (BtkAdjustment *adj,
					     gdouble        upper,
					     gboolean       always_emit_changed);

G_DEFINE_TYPE (BtkLayout, btk_layout, BTK_TYPE_CONTAINER)

/* Public interface
 */
/**
 * btk_layout_new:
 * @hadjustment: (allow-none): horizontal scroll adjustment, or %NULL
 * @vadjustment: (allow-none): vertical scroll adjustment, or %NULL
 * 
 * Creates a new #BtkLayout. Unless you have a specific adjustment
 * you'd like the layout to use for scrolling, pass %NULL for
 * @hadjustment and @vadjustment.
 * 
 * Return value: a new #BtkLayout
 **/
  
BtkWidget*    
btk_layout_new (BtkAdjustment *hadjustment,
		BtkAdjustment *vadjustment)
{
  BtkLayout *layout;

  layout = g_object_new (BTK_TYPE_LAYOUT,
			 "hadjustment", hadjustment,
			 "vadjustment", vadjustment,
			 NULL);

  return BTK_WIDGET (layout);
}

/**
 * btk_layout_get_bin_window:
 * @layout: a #BtkLayout
 *
 * Retrieve the bin window of the layout used for drawing operations.
 *
 * Return value: (transfer none): a #BdkWindow
 *
 * Since: 2.14
 **/
BdkWindow*
btk_layout_get_bin_window (BtkLayout *layout)
{
  g_return_val_if_fail (BTK_IS_LAYOUT (layout), NULL);

  return layout->bin_window;
}

/**
 * btk_layout_get_hadjustment:
 * @layout: a #BtkLayout
 *
 * This function should only be called after the layout has been
 * placed in a #BtkScrolledWindow or otherwise configured for
 * scrolling. It returns the #BtkAdjustment used for communication
 * between the horizontal scrollbar and @layout.
 *
 * See #BtkScrolledWindow, #BtkScrollbar, #BtkAdjustment for details.
 *
 * Return value: (transfer none): horizontal scroll adjustment
 **/
BtkAdjustment*
btk_layout_get_hadjustment (BtkLayout *layout)
{
  g_return_val_if_fail (BTK_IS_LAYOUT (layout), NULL);

  return layout->hadjustment;
}
/**
 * btk_layout_get_vadjustment:
 * @layout: a #BtkLayout
 *
 * This function should only be called after the layout has been
 * placed in a #BtkScrolledWindow or otherwise configured for
 * scrolling. It returns the #BtkAdjustment used for communication
 * between the vertical scrollbar and @layout.
 *
 * See #BtkScrolledWindow, #BtkScrollbar, #BtkAdjustment for details.
 *
 * Return value: (transfer none): vertical scroll adjustment
 **/
BtkAdjustment*
btk_layout_get_vadjustment (BtkLayout *layout)
{
  g_return_val_if_fail (BTK_IS_LAYOUT (layout), NULL);

  return layout->vadjustment;
}

static BtkAdjustment *
new_default_adjustment (void)
{
  return BTK_ADJUSTMENT (btk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
}

static void           
btk_layout_set_adjustments (BtkLayout     *layout,
			    BtkAdjustment *hadj,
			    BtkAdjustment *vadj)
{
  gboolean need_adjust = FALSE;

  g_return_if_fail (BTK_IS_LAYOUT (layout));

  if (hadj)
    g_return_if_fail (BTK_IS_ADJUSTMENT (hadj));
  else if (layout->hadjustment)
    hadj = new_default_adjustment ();
  if (vadj)
    g_return_if_fail (BTK_IS_ADJUSTMENT (vadj));
  else if (layout->vadjustment)
    vadj = new_default_adjustment ();
  
  if (layout->hadjustment && (layout->hadjustment != hadj))
    {
      g_signal_handlers_disconnect_by_func (layout->hadjustment,
					    btk_layout_adjustment_changed,
					    layout);
      g_object_unref (layout->hadjustment);
    }
  
  if (layout->vadjustment && (layout->vadjustment != vadj))
    {
      g_signal_handlers_disconnect_by_func (layout->vadjustment,
					    btk_layout_adjustment_changed,
					    layout);
      g_object_unref (layout->vadjustment);
    }
  
  if (layout->hadjustment != hadj)
    {
      layout->hadjustment = hadj;
      g_object_ref_sink (layout->hadjustment);
      btk_layout_set_adjustment_upper (layout->hadjustment, layout->width, FALSE);
      
      g_signal_connect (layout->hadjustment, "value-changed",
			G_CALLBACK (btk_layout_adjustment_changed),
			layout);
      need_adjust = TRUE;
    }
  
  if (layout->vadjustment != vadj)
    {
      layout->vadjustment = vadj;
      g_object_ref_sink (layout->vadjustment);
      btk_layout_set_adjustment_upper (layout->vadjustment, layout->height, FALSE);
      
      g_signal_connect (layout->vadjustment, "value-changed",
			G_CALLBACK (btk_layout_adjustment_changed),
			layout);
      need_adjust = TRUE;
    }

  /* vadj or hadj can be NULL while constructing; don't emit a signal
     then */
  if (need_adjust && vadj && hadj)
    btk_layout_adjustment_changed (NULL, layout);
}

static void
btk_layout_finalize (GObject *object)
{
  BtkLayout *layout = BTK_LAYOUT (object);

  g_object_unref (layout->hadjustment);
  g_object_unref (layout->vadjustment);

  G_OBJECT_CLASS (btk_layout_parent_class)->finalize (object);
}

/**
 * btk_layout_set_hadjustment:
 * @layout: a #BtkLayout
 * @adjustment: (allow-none): new scroll adjustment
 *
 * Sets the horizontal scroll adjustment for the layout.
 *
 * See #BtkScrolledWindow, #BtkScrollbar, #BtkAdjustment for details.
 * 
 **/
void           
btk_layout_set_hadjustment (BtkLayout     *layout,
			    BtkAdjustment *adjustment)
{
  g_return_if_fail (BTK_IS_LAYOUT (layout));

  btk_layout_set_adjustments (layout, adjustment, layout->vadjustment);
  g_object_notify (G_OBJECT (layout), "hadjustment");
}
 
/**
 * btk_layout_set_vadjustment:
 * @layout: a #BtkLayout
 * @adjustment: (allow-none): new scroll adjustment
 *
 * Sets the vertical scroll adjustment for the layout.
 *
 * See #BtkScrolledWindow, #BtkScrollbar, #BtkAdjustment for details.
 * 
 **/
void           
btk_layout_set_vadjustment (BtkLayout     *layout,
			    BtkAdjustment *adjustment)
{
  g_return_if_fail (BTK_IS_LAYOUT (layout));
  
  btk_layout_set_adjustments (layout, layout->hadjustment, adjustment);
  g_object_notify (G_OBJECT (layout), "vadjustment");
}

static BtkLayoutChild*
get_child (BtkLayout  *layout,
           BtkWidget  *widget)
{
  GList *children;
  
  children = layout->children;
  while (children)
    {
      BtkLayoutChild *child;
      
      child = children->data;
      children = children->next;

      if (child->widget == widget)
        return child;
    }

  return NULL;
}

/**
 * btk_layout_put:
 * @layout: a #BtkLayout
 * @child_widget: child widget
 * @x: X position of child widget
 * @y: Y position of child widget
 *
 * Adds @child_widget to @layout, at position (@x,@y).
 * @layout becomes the new parent container of @child_widget.
 * 
 **/
void           
btk_layout_put (BtkLayout     *layout, 
		BtkWidget     *child_widget, 
		gint           x, 
		gint           y)
{
  BtkLayoutChild *child;

  g_return_if_fail (BTK_IS_LAYOUT (layout));
  g_return_if_fail (BTK_IS_WIDGET (child_widget));
  
  child = g_new (BtkLayoutChild, 1);

  child->widget = child_widget;
  child->x = x;
  child->y = y;

  layout->children = g_list_append (layout->children, child);
  
  if (btk_widget_get_realized (BTK_WIDGET (layout)))
    btk_widget_set_parent_window (child->widget, layout->bin_window);

  btk_widget_set_parent (child_widget, BTK_WIDGET (layout));
}

static void
btk_layout_move_internal (BtkLayout       *layout,
                          BtkWidget       *widget,
                          gboolean         change_x,
                          gint             x,
                          gboolean         change_y,
                          gint             y)
{
  BtkLayoutChild *child;

  child = get_child (layout, widget);

  g_assert (child);

  btk_widget_freeze_child_notify (widget);
  
  if (change_x)
    {
      child->x = x;
      btk_widget_child_notify (widget, "x");
    }

  if (change_y)
    {
      child->y = y;
      btk_widget_child_notify (widget, "y");
    }

  btk_widget_thaw_child_notify (widget);
  
  if (btk_widget_get_visible (widget) &&
      btk_widget_get_visible (BTK_WIDGET (layout)))
    btk_widget_queue_resize (widget);
}

/**
 * btk_layout_move:
 * @layout: a #BtkLayout
 * @child_widget: a current child of @layout
 * @x: X position to move to
 * @y: Y position to move to
 *
 * Moves a current child of @layout to a new position.
 * 
 **/
void           
btk_layout_move (BtkLayout     *layout, 
		 BtkWidget     *child_widget, 
		 gint           x, 
		 gint           y)
{
  g_return_if_fail (BTK_IS_LAYOUT (layout));
  g_return_if_fail (BTK_IS_WIDGET (child_widget));
  g_return_if_fail (child_widget->parent == BTK_WIDGET (layout));  

  btk_layout_move_internal (layout, child_widget, TRUE, x, TRUE, y);
}

static void
btk_layout_set_adjustment_upper (BtkAdjustment *adj,
				 gdouble        upper,
				 gboolean       always_emit_changed)
{
  gboolean changed = FALSE;
  gboolean value_changed = FALSE;
  
  gdouble min = MAX (0., upper - adj->page_size);

  if (upper != adj->upper)
    {
      adj->upper = upper;
      changed = TRUE;
    }
      
  if (adj->value > min)
    {
      adj->value = min;
      value_changed = TRUE;
    }
  
  if (changed || always_emit_changed)
    btk_adjustment_changed (adj);
  if (value_changed)
    btk_adjustment_value_changed (adj);
}

/**
 * btk_layout_set_size:
 * @layout: a #BtkLayout
 * @width: width of entire scrollable area
 * @height: height of entire scrollable area
 *
 * Sets the size of the scrollable area of the layout.
 * 
 **/
void
btk_layout_set_size (BtkLayout     *layout, 
		     guint          width,
		     guint          height)
{
  BtkWidget *widget;
  
  g_return_if_fail (BTK_IS_LAYOUT (layout));
  
  widget = BTK_WIDGET (layout);
  
  g_object_freeze_notify (G_OBJECT (layout));
  if (width != layout->width)
     {
	layout->width = width;
	g_object_notify (G_OBJECT (layout), "width");
     }
  if (height != layout->height)
     {
	layout->height = height;
	g_object_notify (G_OBJECT (layout), "height");
     }
  g_object_thaw_notify (G_OBJECT (layout));

  if (layout->hadjustment)
    btk_layout_set_adjustment_upper (layout->hadjustment, layout->width, FALSE);
  if (layout->vadjustment)
    btk_layout_set_adjustment_upper (layout->vadjustment, layout->height, FALSE);

  if (btk_widget_get_realized (widget))
    {
      width = MAX (width, widget->allocation.width);
      height = MAX (height, widget->allocation.height);
      bdk_window_resize (layout->bin_window, width, height);
    }
}

/**
 * btk_layout_get_size:
 * @layout: a #BtkLayout
 * @width: (out) (allow-none): location to store the width set on
 *     @layout, or %NULL
 * @height: (out) (allow-none): location to store the height set on
 *     @layout, or %NULL
 *
 * Gets the size that has been set on the layout, and that determines
 * the total extents of the layout's scrollbar area. See
 * btk_layout_set_size ().
 **/
void
btk_layout_get_size (BtkLayout *layout,
		     guint     *width,
		     guint     *height)
{
  g_return_if_fail (BTK_IS_LAYOUT (layout));

  if (width)
    *width = layout->width;
  if (height)
    *height = layout->height;
}

/**
 * btk_layout_freeze:
 * @layout: a #BtkLayout
 * 
 * This is a deprecated function, it doesn't do anything useful.
 **/
void
btk_layout_freeze (BtkLayout *layout)
{
  g_return_if_fail (BTK_IS_LAYOUT (layout));

  layout->freeze_count++;
}

/**
 * btk_layout_thaw:
 * @layout: a #BtkLayout
 * 
 * This is a deprecated function, it doesn't do anything useful.
 **/
void
btk_layout_thaw (BtkLayout *layout)
{
  g_return_if_fail (BTK_IS_LAYOUT (layout));

  if (layout->freeze_count)
    {
      if (!(--layout->freeze_count))
	{
	  btk_widget_queue_draw (BTK_WIDGET (layout));
	  bdk_window_process_updates (BTK_WIDGET (layout)->window, TRUE);
	}
    }

}

/* Basic Object handling procedures
 */
static void
btk_layout_class_init (BtkLayoutClass *class)
{
  GObjectClass *bobject_class;
  BtkWidgetClass *widget_class;
  BtkContainerClass *container_class;

  bobject_class = (GObjectClass*) class;
  widget_class = (BtkWidgetClass*) class;
  container_class = (BtkContainerClass*) class;

  bobject_class->set_property = btk_layout_set_property;
  bobject_class->get_property = btk_layout_get_property;
  bobject_class->finalize = btk_layout_finalize;
  bobject_class->constructor = btk_layout_constructor;

  container_class->set_child_property = btk_layout_set_child_property;
  container_class->get_child_property = btk_layout_get_child_property;

  btk_container_class_install_child_property (container_class,
					      CHILD_PROP_X,
					      g_param_spec_int ("x",
                                                                P_("X position"),
                                                                P_("X position of child widget"),
                                                                G_MININT,
                                                                G_MAXINT,
                                                                0,
                                                                BTK_PARAM_READWRITE));

  btk_container_class_install_child_property (container_class,
					      CHILD_PROP_Y,
					      g_param_spec_int ("y",
                                                                P_("Y position"),
                                                                P_("Y position of child widget"),
                                                                G_MININT,
                                                                G_MAXINT,
                                                                0,
                                                                BTK_PARAM_READWRITE));
  
  g_object_class_install_property (bobject_class,
				   PROP_HADJUSTMENT,
				   g_param_spec_object ("hadjustment",
							P_("Horizontal adjustment"),
							P_("The BtkAdjustment for the horizontal position"),
							BTK_TYPE_ADJUSTMENT,
							BTK_PARAM_READWRITE));
  
  g_object_class_install_property (bobject_class,
				   PROP_VADJUSTMENT,
				   g_param_spec_object ("vadjustment",
							P_("Vertical adjustment"),
							P_("The BtkAdjustment for the vertical position"),
							BTK_TYPE_ADJUSTMENT,
							BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
				   PROP_WIDTH,
				   g_param_spec_uint ("width",
						     P_("Width"),
						     P_("The width of the layout"),
						     0,
						     G_MAXINT,
						     100,
						     BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
				   PROP_HEIGHT,
				   g_param_spec_uint ("height",
						     P_("Height"),
						     P_("The height of the layout"),
						     0,
						     G_MAXINT,
						     100,
						     BTK_PARAM_READWRITE));
  widget_class->realize = btk_layout_realize;
  widget_class->unrealize = btk_layout_unrealize;
  widget_class->map = btk_layout_map;
  widget_class->size_request = btk_layout_size_request;
  widget_class->size_allocate = btk_layout_size_allocate;
  widget_class->expose_event = btk_layout_expose;
  widget_class->style_set = btk_layout_style_set;

  container_class->add = btk_layout_add;
  container_class->remove = btk_layout_remove;
  container_class->forall = btk_layout_forall;

  class->set_scroll_adjustments = btk_layout_set_adjustments;

  /**
   * BtkLayout::set-scroll-adjustments
   * @horizontal: the horizontal #BtkAdjustment
   * @vertical: the vertical #BtkAdjustment
   *
   * Set the scroll adjustments for the layout. Usually scrolled containers
   * like #BtkScrolledWindow will emit this signal to connect two instances
   * of #BtkScrollbar to the scroll directions of the #BtkLayout.
   */
  widget_class->set_scroll_adjustments_signal =
    g_signal_new (I_("set-scroll-adjustments"),
		  G_OBJECT_CLASS_TYPE (bobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkLayoutClass, set_scroll_adjustments),
		  NULL, NULL,
		  _btk_marshal_VOID__OBJECT_OBJECT,
		  G_TYPE_NONE, 2,
		  BTK_TYPE_ADJUSTMENT,
		  BTK_TYPE_ADJUSTMENT);
}

static void
btk_layout_get_property (GObject     *object,
			 guint        prop_id,
			 GValue      *value,
			 GParamSpec  *pspec)
{
  BtkLayout *layout = BTK_LAYOUT (object);
  
  switch (prop_id)
    {
    case PROP_HADJUSTMENT:
      g_value_set_object (value, layout->hadjustment);
      break;
    case PROP_VADJUSTMENT:
      g_value_set_object (value, layout->vadjustment);
      break;
    case PROP_WIDTH:
      g_value_set_uint (value, layout->width);
      break;
    case PROP_HEIGHT:
      g_value_set_uint (value, layout->height);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_layout_set_property (GObject      *object,
			 guint         prop_id,
			 const GValue *value,
			 GParamSpec   *pspec)
{
  BtkLayout *layout = BTK_LAYOUT (object);
  
  switch (prop_id)
    {
    case PROP_HADJUSTMENT:
      btk_layout_set_hadjustment (layout, 
				  (BtkAdjustment*) g_value_get_object (value));
      break;
    case PROP_VADJUSTMENT:
      btk_layout_set_vadjustment (layout, 
				  (BtkAdjustment*) g_value_get_object (value));
      break;
    case PROP_WIDTH:
      btk_layout_set_size (layout, g_value_get_uint (value),
			   layout->height);
      break;
    case PROP_HEIGHT:
      btk_layout_set_size (layout, layout->width,
			   g_value_get_uint (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_layout_set_child_property (BtkContainer    *container,
                               BtkWidget       *child,
                               guint            property_id,
                               const GValue    *value,
                               GParamSpec      *pspec)
{
  switch (property_id)
    {
    case CHILD_PROP_X:
      btk_layout_move_internal (BTK_LAYOUT (container),
                                child,
                                TRUE, g_value_get_int (value),
                                FALSE, 0);
      break;
    case CHILD_PROP_Y:
      btk_layout_move_internal (BTK_LAYOUT (container),
                                child,
                                FALSE, 0,
                                TRUE, g_value_get_int (value));
      break;
    default:
      BTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

static void
btk_layout_get_child_property (BtkContainer *container,
                               BtkWidget    *child,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtkLayoutChild *layout_child;

  layout_child = get_child (BTK_LAYOUT (container), child);
  
  switch (property_id)
    {
    case CHILD_PROP_X:
      g_value_set_int (value, layout_child->x);
      break;
    case CHILD_PROP_Y:
      g_value_set_int (value, layout_child->y);
      break;
    default:
      BTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

static void
btk_layout_init (BtkLayout *layout)
{
  layout->children = NULL;

  layout->width = 100;
  layout->height = 100;

  layout->hadjustment = NULL;
  layout->vadjustment = NULL;

  layout->bin_window = NULL;

  layout->scroll_x = 0;
  layout->scroll_y = 0;
  layout->visibility = BDK_VISIBILITY_PARTIAL;

  layout->freeze_count = 0;
}

static GObject *
btk_layout_constructor (GType                  type,
			guint                  n_properties,
			GObjectConstructParam *properties)
{
  BtkLayout *layout;
  GObject *object;
  BtkAdjustment *hadj, *vadj;
  
  object = G_OBJECT_CLASS (btk_layout_parent_class)->constructor (type,
								  n_properties,
								  properties);

  layout = BTK_LAYOUT (object);

  hadj = layout->hadjustment ? layout->hadjustment : new_default_adjustment ();
  vadj = layout->vadjustment ? layout->vadjustment : new_default_adjustment ();

  if (!layout->hadjustment || !layout->vadjustment)
    btk_layout_set_adjustments (layout, hadj, vadj);

  return object;
}

/* Widget methods
 */

static void 
btk_layout_realize (BtkWidget *widget)
{
  BtkLayout *layout = BTK_LAYOUT (widget);
  GList *tmp_list;
  BdkWindowAttr attributes;
  gint attributes_mask;

  btk_widget_set_realized (widget, TRUE);

  attributes.window_type = BDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = BDK_INPUT_OUTPUT;
  attributes.visual = btk_widget_get_visual (widget);
  attributes.colormap = btk_widget_get_colormap (widget);
  attributes.event_mask = BDK_VISIBILITY_NOTIFY_MASK;

  attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_VISUAL | BDK_WA_COLORMAP;

  widget->window = bdk_window_new (btk_widget_get_parent_window (widget),
				   &attributes, attributes_mask);
  bdk_window_set_back_pixmap (widget->window, NULL, FALSE);
  bdk_window_set_user_data (widget->window, widget);

  attributes.x = - layout->hadjustment->value,
  attributes.y = - layout->vadjustment->value;
  attributes.width = MAX (layout->width, widget->allocation.width);
  attributes.height = MAX (layout->height, widget->allocation.height);
  attributes.event_mask = BDK_EXPOSURE_MASK | BDK_SCROLL_MASK | 
                          btk_widget_get_events (widget);

  layout->bin_window = bdk_window_new (widget->window,
					&attributes, attributes_mask);
  bdk_window_set_user_data (layout->bin_window, widget);

  widget->style = btk_style_attach (widget->style, widget->window);
  btk_style_set_background (widget->style, layout->bin_window, BTK_STATE_NORMAL);

  tmp_list = layout->children;
  while (tmp_list)
    {
      BtkLayoutChild *child = tmp_list->data;
      tmp_list = tmp_list->next;

      btk_widget_set_parent_window (child->widget, layout->bin_window);
    }
}

static void
btk_layout_style_set (BtkWidget *widget,
                      BtkStyle  *old_style)
{
  BTK_WIDGET_CLASS (btk_layout_parent_class)->style_set (widget, old_style);

  if (btk_widget_get_realized (widget))
    {
      btk_style_set_background (widget->style, BTK_LAYOUT (widget)->bin_window, BTK_STATE_NORMAL);
    }
}

static void
btk_layout_map (BtkWidget *widget)
{
  BtkLayout *layout = BTK_LAYOUT (widget);
  GList *tmp_list;

  btk_widget_set_mapped (widget, TRUE);

  tmp_list = layout->children;
  while (tmp_list)
    {
      BtkLayoutChild *child = tmp_list->data;
      tmp_list = tmp_list->next;

      if (btk_widget_get_visible (child->widget))
	{
	  if (!btk_widget_get_mapped (child->widget))
	    btk_widget_map (child->widget);
	}
    }

  bdk_window_show (layout->bin_window);
  bdk_window_show (widget->window);
}

static void 
btk_layout_unrealize (BtkWidget *widget)
{
  BtkLayout *layout = BTK_LAYOUT (widget);

  bdk_window_set_user_data (layout->bin_window, NULL);
  bdk_window_destroy (layout->bin_window);
  layout->bin_window = NULL;

  BTK_WIDGET_CLASS (btk_layout_parent_class)->unrealize (widget);
}

static void     
btk_layout_size_request (BtkWidget     *widget,
			 BtkRequisition *requisition)
{
  BtkLayout *layout = BTK_LAYOUT (widget);
  GList *tmp_list;

  requisition->width = 0;
  requisition->height = 0;

  tmp_list = layout->children;

  while (tmp_list)
    {
      BtkLayoutChild *child = tmp_list->data;
      BtkRequisition child_requisition;
      
      tmp_list = tmp_list->next;

      btk_widget_size_request (child->widget, &child_requisition);
    }
}

static void     
btk_layout_size_allocate (BtkWidget     *widget,
			  BtkAllocation *allocation)
{
  BtkLayout *layout = BTK_LAYOUT (widget);
  GList *tmp_list;

  widget->allocation = *allocation;

  tmp_list = layout->children;

  while (tmp_list)
    {
      BtkLayoutChild *child = tmp_list->data;
      tmp_list = tmp_list->next;

      btk_layout_allocate_child (layout, child);
    }

  if (btk_widget_get_realized (widget))
    {
      bdk_window_move_resize (widget->window,
			      allocation->x, allocation->y,
			      allocation->width, allocation->height);

      bdk_window_resize (layout->bin_window,
			 MAX (layout->width, allocation->width),
			 MAX (layout->height, allocation->height));
    }

  layout->hadjustment->page_size = allocation->width;
  layout->hadjustment->page_increment = allocation->width * 0.9;
  layout->hadjustment->lower = 0;
  /* set_adjustment_upper() emits ::changed */
  btk_layout_set_adjustment_upper (layout->hadjustment, MAX (allocation->width, layout->width), TRUE);

  layout->vadjustment->page_size = allocation->height;
  layout->vadjustment->page_increment = allocation->height * 0.9;
  layout->vadjustment->lower = 0;
  layout->vadjustment->upper = MAX (allocation->height, layout->height);
  btk_layout_set_adjustment_upper (layout->vadjustment, MAX (allocation->height, layout->height), TRUE);
}

static gint 
btk_layout_expose (BtkWidget      *widget,
                   BdkEventExpose *event)
{
  BtkLayout *layout = BTK_LAYOUT (widget);

  if (event->window != layout->bin_window)
    return FALSE;

  BTK_WIDGET_CLASS (btk_layout_parent_class)->expose_event (widget, event);

  return FALSE;
}

/* Container methods
 */
static void
btk_layout_add (BtkContainer *container,
		BtkWidget    *widget)
{
  btk_layout_put (BTK_LAYOUT (container), widget, 0, 0);
}

static void
btk_layout_remove (BtkContainer *container, 
		   BtkWidget    *widget)
{
  BtkLayout *layout = BTK_LAYOUT (container);
  GList *tmp_list;
  BtkLayoutChild *child = NULL;

  tmp_list = layout->children;
  while (tmp_list)
    {
      child = tmp_list->data;
      if (child->widget == widget)
	break;
      tmp_list = tmp_list->next;
    }

  if (tmp_list)
    {
      btk_widget_unparent (widget);

      layout->children = g_list_remove_link (layout->children, tmp_list);
      g_list_free_1 (tmp_list);
      g_free (child);
    }
}

static void
btk_layout_forall (BtkContainer *container,
		   gboolean      include_internals,
		   BtkCallback   callback,
		   gpointer      callback_data)
{
  BtkLayout *layout = BTK_LAYOUT (container);
  BtkLayoutChild *child;
  GList *tmp_list;

  tmp_list = layout->children;
  while (tmp_list)
    {
      child = tmp_list->data;
      tmp_list = tmp_list->next;

      (* callback) (child->widget, callback_data);
    }
}

/* Operations on children
 */

static void
btk_layout_allocate_child (BtkLayout      *layout,
			   BtkLayoutChild *child)
{
  BtkAllocation allocation;
  BtkRequisition requisition;

  allocation.x = child->x;
  allocation.y = child->y;
  btk_widget_get_child_requisition (child->widget, &requisition);
  allocation.width = requisition.width;
  allocation.height = requisition.height;
  
  btk_widget_size_allocate (child->widget, &allocation);
}

/* Callbacks */

static void
btk_layout_adjustment_changed (BtkAdjustment *adjustment,
			       BtkLayout     *layout)
{
  if (layout->freeze_count)
    return;

  if (btk_widget_get_realized (BTK_WIDGET (layout)))
    {
      bdk_window_move (layout->bin_window,
		       - layout->hadjustment->value,
		       - layout->vadjustment->value);
      
      bdk_window_process_updates (layout->bin_window, TRUE);
    }
}

#define __BTK_LAYOUT_C__
#include "btkaliasdef.c"
