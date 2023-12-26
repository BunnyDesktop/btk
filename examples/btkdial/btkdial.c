
/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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
#include <math.h>
#include <stdio.h>
#include <btk/btkmain.h>
#include <btk/btksignal.h>

#include "btkdial.h"

#define SCROLL_DELAY_LENGTH  300
#define DIAL_DEFAULT_SIZE 100

/* Forward declarations */

static void btk_dial_class_init               (BtkDialClass     *klass);
static void btk_dial_init                     (BtkDial          *dial);
static void btk_dial_destroy                  (BtkObject        *object);
static void btk_dial_realize                  (BtkWidget        *widget);
static void btk_dial_size_request             (BtkWidget        *widget,
                                               BtkRequisition   *requisition);
static void btk_dial_size_allocate            (BtkWidget        *widget,
                                               BtkAllocation    *allocation);
static gboolean btk_dial_expose               (BtkWidget        *widget,
                                               BdkEventExpose   *event);
static gboolean btk_dial_button_press         (BtkWidget        *widget,
                                               BdkEventButton   *event);
static gboolean btk_dial_button_release       (BtkWidget        *widget,
                                               BdkEventButton   *event);
static gboolean btk_dial_motion_notify        (BtkWidget        *widget,
                                               BdkEventMotion   *event);
static gboolean btk_dial_timer                (BtkDial          *dial);

static void btk_dial_update_mouse             (BtkDial *dial, gint x, gint y);
static void btk_dial_update                   (BtkDial *dial);
static void btk_dial_adjustment_changed       (BtkAdjustment    *adjustment,
						gpointer          data);
static void btk_dial_adjustment_value_changed (BtkAdjustment    *adjustment,
						gpointer          data);

/* Local data */

static BtkWidgetClass *parent_class = NULL;

GType
btk_dial_get_type ()
{
  static GType dial_type = 0;

  if (!dial_type)
    {
      const GTypeInfo dial_info =
      {
	sizeof (BtkDialClass),
	NULL,
	NULL,
	(GClassInitFunc) btk_dial_class_init,
	NULL,
	NULL,
	sizeof (BtkDial),
        0,
	(GInstanceInitFunc) btk_dial_init,
      };

      dial_type = g_type_register_static (BTK_TYPE_WIDGET, "BtkDial", &dial_info, 0);
    }

  return dial_type;
}

static void
btk_dial_class_init (BtkDialClass *class)
{
  BtkObjectClass *object_class;
  BtkWidgetClass *widget_class;

  object_class = (BtkObjectClass*) class;
  widget_class = (BtkWidgetClass*) class;

  parent_class = g_type_class_peek_parent (class);

  object_class->destroy = btk_dial_destroy;

  widget_class->realize = btk_dial_realize;
  widget_class->expose_event = btk_dial_expose;
  widget_class->size_request = btk_dial_size_request;
  widget_class->size_allocate = btk_dial_size_allocate;
  widget_class->button_press_event = btk_dial_button_press;
  widget_class->button_release_event = btk_dial_button_release;
  widget_class->motion_notify_event = btk_dial_motion_notify;
}

static void
btk_dial_init (BtkDial *dial)
{
  dial->button = 0;
  dial->policy = BTK_UPDATE_CONTINUOUS;
  dial->timer = 0;
  dial->radius = 0;
  dial->pointer_width = 0;
  dial->angle = 0.0;
  dial->old_value = 0.0;
  dial->old_lower = 0.0;
  dial->old_upper = 0.0;
  dial->adjustment = NULL;
}

BtkWidget*
btk_dial_new (BtkAdjustment *adjustment)
{
  BtkDial *dial;

  dial = g_object_new (btk_dial_get_type (), NULL);

  if (!adjustment)
    adjustment = (BtkAdjustment*) btk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

  btk_dial_set_adjustment (dial, adjustment);

  return BTK_WIDGET (dial);
}

static void
btk_dial_destroy (BtkObject *object)
{
  BtkDial *dial;

  g_return_if_fail (object != NULL);
  g_return_if_fail (BTK_IS_DIAL (object));

  dial = BTK_DIAL (object);

  if (dial->adjustment)
    {
      g_object_unref (BTK_OBJECT (dial->adjustment));
      dial->adjustment = NULL;
    }

  BTK_OBJECT_CLASS (parent_class)->destroy (object);
}

BtkAdjustment*
btk_dial_get_adjustment (BtkDial *dial)
{
  g_return_val_if_fail (dial != NULL, NULL);
  g_return_val_if_fail (BTK_IS_DIAL (dial), NULL);

  return dial->adjustment;
}

void
btk_dial_set_update_policy (BtkDial      *dial,
			     BtkUpdateType  policy)
{
  g_return_if_fail (dial != NULL);
  g_return_if_fail (BTK_IS_DIAL (dial));

  dial->policy = policy;
}

void
btk_dial_set_adjustment (BtkDial      *dial,
			  BtkAdjustment *adjustment)
{
  g_return_if_fail (dial != NULL);
  g_return_if_fail (BTK_IS_DIAL (dial));

  if (dial->adjustment)
    {
      g_signal_handlers_disconnect_by_func (BTK_OBJECT (dial->adjustment), NULL, (gpointer) dial);
      g_object_unref (BTK_OBJECT (dial->adjustment));
    }

  dial->adjustment = adjustment;
  g_object_ref (BTK_OBJECT (dial->adjustment));

  g_signal_connect (B_OBJECT (adjustment), "changed",
		    G_CALLBACK (btk_dial_adjustment_changed),
		    (gpointer) dial);
  g_signal_connect (B_OBJECT (adjustment), "value_changed",
		    G_CALLBACK (btk_dial_adjustment_value_changed),
		    (gpointer) dial);

  dial->old_value = adjustment->value;
  dial->old_lower = adjustment->lower;
  dial->old_upper = adjustment->upper;

  btk_dial_update (dial);
}

static void
btk_dial_realize (BtkWidget *widget)
{
  BtkDial *dial;
  BdkWindowAttr attributes;
  gint attributes_mask;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (BTK_IS_DIAL (widget));

  btk_widget_set_realized (widget, TRUE);
  dial = BTK_DIAL (widget);

  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = BDK_INPUT_OUTPUT;
  attributes.window_type = BDK_WINDOW_CHILD;
  attributes.event_mask = btk_widget_get_events (widget) | 
    BDK_EXPOSURE_MASK | BDK_BUTTON_PRESS_MASK | 
    BDK_BUTTON_RELEASE_MASK | BDK_POINTER_MOTION_MASK |
    BDK_POINTER_MOTION_HINT_MASK;
  attributes.visual = btk_widget_get_visual (widget);
  attributes.colormap = btk_widget_get_colormap (widget);

  attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_VISUAL | BDK_WA_COLORMAP;
  widget->window = bdk_window_new (widget->parent->window, &attributes, attributes_mask);

  widget->style = btk_style_attach (widget->style, widget->window);

  bdk_window_set_user_data (widget->window, widget);

  btk_style_set_background (widget->style, widget->window, BTK_STATE_ACTIVE);
}

static void 
btk_dial_size_request (BtkWidget      *widget,
		       BtkRequisition *requisition)
{
  requisition->width = DIAL_DEFAULT_SIZE;
  requisition->height = DIAL_DEFAULT_SIZE;
}

static void
btk_dial_size_allocate (BtkWidget     *widget,
			BtkAllocation *allocation)
{
  BtkDial *dial;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (BTK_IS_DIAL (widget));
  g_return_if_fail (allocation != NULL);

  widget->allocation = *allocation;
  dial = BTK_DIAL (widget);

  if (btk_widget_get_realized (widget))
    {

      bdk_window_move_resize (widget->window,
			      allocation->x, allocation->y,
			      allocation->width, allocation->height);

    }
  dial->radius = MIN (allocation->width, allocation->height) * 0.45;
  dial->pointer_width = dial->radius / 5;
}

static gboolean
btk_dial_expose( BtkWidget      *widget,
		 BdkEventExpose *event )
{
  BtkDial *dial;
  BdkPoint points[6];
  gdouble s,c;
  gdouble theta, last, increment;
  BtkStyle      *blankstyle;
  gint xc, yc;
  gint upper, lower;
  gint tick_length;
  gint i, inc;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (BTK_IS_DIAL (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  if (event->count > 0)
    return FALSE;
  
  dial = BTK_DIAL (widget);

/*  bdk_window_clear_area (widget->window,
			 0, 0,
			 widget->allocation.width,
			 widget->allocation.height);
*/
  xc = widget->allocation.width / 2;
  yc = widget->allocation.height / 2;

  upper = dial->adjustment->upper;
  lower = dial->adjustment->lower;

  /* Erase old pointer */

  s = sin (dial->last_angle);
  c = cos (dial->last_angle);
  dial->last_angle = dial->angle;

  points[0].x = xc + s*dial->pointer_width/2;
  points[0].y = yc + c*dial->pointer_width/2;
  points[1].x = xc + c*dial->radius;
  points[1].y = yc - s*dial->radius;
  points[2].x = xc - s*dial->pointer_width/2;
  points[2].y = yc - c*dial->pointer_width/2;
  points[3].x = xc - c*dial->radius/10;
  points[3].y = yc + s*dial->radius/10;
  points[4].x = points[0].x;
  points[4].y = points[0].y;

  blankstyle = btk_style_new ();
  blankstyle->bg_gc[BTK_STATE_NORMAL] =
                widget->style->bg_gc[BTK_STATE_NORMAL];
  blankstyle->dark_gc[BTK_STATE_NORMAL] =
                widget->style->bg_gc[BTK_STATE_NORMAL];
  blankstyle->light_gc[BTK_STATE_NORMAL] =
                widget->style->bg_gc[BTK_STATE_NORMAL];
  blankstyle->black_gc =
                widget->style->bg_gc[BTK_STATE_NORMAL];
  blankstyle->depth =
                bdk_drawable_get_depth( BDK_DRAWABLE (widget->window));

  btk_paint_polygon (blankstyle,
                    widget->window,
                    BTK_STATE_NORMAL,
                    BTK_SHADOW_OUT,
	            NULL,
                    widget,
                    NULL,
                    points, 5,
                    FALSE);

  g_object_unref (blankstyle);


  /* Draw ticks */

  if ((upper - lower) == 0)
    return FALSE;

  increment = (100*M_PI) / (dial->radius*dial->radius);

  inc = (upper - lower);

  while (inc < 100) inc *= 10;
  while (inc >= 1000) inc /= 10;
  last = -1;

  for (i = 0; i <= inc; i++)
    {
      theta = ((gfloat)i*M_PI / (18*inc/24.) - M_PI/6.);

      if ((theta - last) < (increment))
	continue;     
      last = theta;

      s = sin (theta);
      c = cos (theta);

      tick_length = (i%(inc/10) == 0) ? dial->pointer_width : dial->pointer_width / 2;

      bdk_draw_line (widget->window,
                     widget->style->fg_gc[widget->state],
                     xc + c*(dial->radius - tick_length),
                     yc - s*(dial->radius - tick_length),
                     xc + c*dial->radius,
                     yc - s*dial->radius);
    }

  /* Draw pointer */

  s = sin (dial->angle);
  c = cos (dial->angle);
  dial->last_angle = dial->angle;

  points[0].x = xc + s*dial->pointer_width/2;
  points[0].y = yc + c*dial->pointer_width/2;
  points[1].x = xc + c*dial->radius;
  points[1].y = yc - s*dial->radius;
  points[2].x = xc - s*dial->pointer_width/2;
  points[2].y = yc - c*dial->pointer_width/2;
  points[3].x = xc - c*dial->radius/10;
  points[3].y = yc + s*dial->radius/10;
  points[4].x = points[0].x;
  points[4].y = points[0].y;


  btk_paint_polygon (widget->style,
		    widget->window,
		    BTK_STATE_NORMAL,
		    BTK_SHADOW_OUT,
	            NULL,
                    widget,
                    NULL,
		    points, 5,
		    TRUE);

  return FALSE;
}

static gboolean
btk_dial_button_press( BtkWidget      *widget,
		       BdkEventButton *event )
{
  BtkDial *dial;
  gint dx, dy;
  double s, c;
  double d_parallel;
  double d_perpendicular;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (BTK_IS_DIAL (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  dial = BTK_DIAL (widget);

  /* Determine if button press was within pointer rebunnyion - we 
     do this by computing the parallel and perpendicular distance of
     the point where the mouse was pressed from the line passing through
     the pointer */
  
  dx = event->x - widget->allocation.width / 2;
  dy = widget->allocation.height / 2 - event->y;
  
  s = sin (dial->angle);
  c = cos (dial->angle);
  
  d_parallel = s*dy + c*dx;
  d_perpendicular = fabs (s*dx - c*dy);
  
  if (!dial->button &&
      (d_perpendicular < dial->pointer_width/2) &&
      (d_parallel > - dial->pointer_width))
    {
      btk_grab_add (widget);

      dial->button = event->button;

      btk_dial_update_mouse (dial, event->x, event->y);
    }

  return FALSE;
}

static gboolean
btk_dial_button_release( BtkWidget      *widget,
                         BdkEventButton *event )
{
  BtkDial *dial;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (BTK_IS_DIAL (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  dial = BTK_DIAL (widget);

  if (dial->button == event->button)
    {
      btk_grab_remove (widget);

      dial->button = 0;

      if (dial->policy == BTK_UPDATE_DELAYED)
	g_source_remove (dial->timer);
      
      if ((dial->policy != BTK_UPDATE_CONTINUOUS) &&
	  (dial->old_value != dial->adjustment->value))
	g_signal_emit_by_name (BTK_OBJECT (dial->adjustment), "value_changed");
    }

  return FALSE;
}

static gboolean
btk_dial_motion_notify( BtkWidget      *widget,
                        BdkEventMotion *event )
{
  BtkDial *dial;
  BdkModifierType mods;
  gint x, y, mask;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (BTK_IS_DIAL (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  dial = BTK_DIAL (widget);

  if (dial->button != 0)
    {
      x = event->x;
      y = event->y;

      if (event->is_hint || (event->window != widget->window))
	bdk_window_get_pointer (widget->window, &x, &y, &mods);

      switch (dial->button)
	{
	case 1:
	  mask = BDK_BUTTON1_MASK;
	  break;
	case 2:
	  mask = BDK_BUTTON2_MASK;
	  break;
	case 3:
	  mask = BDK_BUTTON3_MASK;
	  break;
	default:
	  mask = 0;
	  break;
	}

      if (mods & mask)
	btk_dial_update_mouse (dial, x,y);
    }

  return FALSE;
}

static gboolean
btk_dial_timer( BtkDial *dial )
{
  g_return_val_if_fail (dial != NULL, FALSE);
  g_return_val_if_fail (BTK_IS_DIAL (dial), FALSE);

  if (dial->policy == BTK_UPDATE_DELAYED)
    g_signal_emit_by_name (BTK_OBJECT (dial->adjustment), "value_changed");

  return FALSE;
}

static void
btk_dial_update_mouse( BtkDial *dial, gint x, gint y )
{
  gint xc, yc;
  gfloat old_value;

  g_return_if_fail (dial != NULL);
  g_return_if_fail (BTK_IS_DIAL (dial));

  xc = BTK_WIDGET(dial)->allocation.width / 2;
  yc = BTK_WIDGET(dial)->allocation.height / 2;

  old_value = dial->adjustment->value;
  dial->angle = atan2(yc-y, x-xc);

  if (dial->angle < -M_PI/2.)
    dial->angle += 2*M_PI;

  if (dial->angle < -M_PI/6)
    dial->angle = -M_PI/6;

  if (dial->angle > 7.*M_PI/6.)
    dial->angle = 7.*M_PI/6.;

  dial->adjustment->value = dial->adjustment->lower + (7.*M_PI/6 - dial->angle) *
    (dial->adjustment->upper - dial->adjustment->lower) / (4.*M_PI/3.);

  if (dial->adjustment->value != old_value)
    {
      if (dial->policy == BTK_UPDATE_CONTINUOUS)
	{
	  g_signal_emit_by_name (BTK_OBJECT (dial->adjustment), "value_changed");
	}
      else
	{
	  btk_widget_queue_draw (BTK_WIDGET (dial));

	  if (dial->policy == BTK_UPDATE_DELAYED)
	    {
	      if (dial->timer)
		g_source_remove (dial->timer);

	      dial->timer = bdk_threads_add_timeout (SCROLL_DELAY_LENGTH,
					   (GSourceFunc) btk_dial_timer,
					   (gpointer) dial);
	    }
	}
    }
}

static void
btk_dial_update (BtkDial *dial)
{
  gfloat new_value;
  
  g_return_if_fail (dial != NULL);
  g_return_if_fail (BTK_IS_DIAL (dial));

  new_value = dial->adjustment->value;
  
  if (new_value < dial->adjustment->lower)
    new_value = dial->adjustment->lower;

  if (new_value > dial->adjustment->upper)
    new_value = dial->adjustment->upper;

  if (new_value != dial->adjustment->value)
    {
      dial->adjustment->value = new_value;
      g_signal_emit_by_name (BTK_OBJECT (dial->adjustment), "value_changed");
    }

  dial->angle = 7.*M_PI/6. - (new_value - dial->adjustment->lower) * 4.*M_PI/3. /
    (dial->adjustment->upper - dial->adjustment->lower);

  btk_widget_queue_draw (BTK_WIDGET (dial));
}

static void
btk_dial_adjustment_changed (BtkAdjustment *adjustment,
			      gpointer       data)
{
  BtkDial *dial;

  g_return_if_fail (adjustment != NULL);
  g_return_if_fail (data != NULL);

  dial = BTK_DIAL (data);

  if ((dial->old_value != adjustment->value) ||
      (dial->old_lower != adjustment->lower) ||
      (dial->old_upper != adjustment->upper))
    {
      btk_dial_update (dial);

      dial->old_value = adjustment->value;
      dial->old_lower = adjustment->lower;
      dial->old_upper = adjustment->upper;
    }
}

static void
btk_dial_adjustment_value_changed (BtkAdjustment *adjustment,
				    gpointer       data)
{
  BtkDial *dial;

  g_return_if_fail (adjustment != NULL);
  g_return_if_fail (data != NULL);

  dial = BTK_DIAL (data);

  if (dial->old_value != adjustment->value)
    {
      btk_dial_update (dial);

      dial->old_value = adjustment->value;
    }
}
