/* HSV color selector for BTK+
 *
 * Copyright (C) 1999 The Free Software Foundation
 *
 * Authors: Simon Budig <Simon.Budig@unix-ag.org> (original code)
 *          Federico Mena-Quintero <federico@gimp.org> (cleanup for BTK+)
 *          Jonathan Blandford <jrb@redhat.com> (cleanup for BTK+)
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

#include <math.h>
#include <string.h>

#include "bdk/bdkkeysyms.h"

#include "btkhsv.h"
#include "btkbindings.h"
#include "btkmarshalers.h"
#include "btkintl.h"
#include "btkalias.h"

/* Default width/height */
#define DEFAULT_SIZE 100

/* Default ring width */
#define DEFAULT_RING_WIDTH 10


/* Dragging modes */
typedef enum {
  DRAG_NONE,
  DRAG_H,
  DRAG_SV
} DragMode;

/* Private part of the BtkHSV structure */
typedef struct {
  /* Color value */
  double h;
  double s;
  double v;
  
  /* Size and ring width */
  int size;
  int ring_width;
  
  /* Window for capturing events */
  BdkWindow *window;
  
  /* Dragging mode */
  DragMode mode;

  buint focus_on_ring : 1;
  
} HSVPrivate;



/* Signal IDs */

enum {
  CHANGED,
  MOVE,
  LAST_SIGNAL
};

static void     btk_hsv_destroy        (BtkObject        *object);
static void     btk_hsv_map            (BtkWidget        *widget);
static void     btk_hsv_unmap          (BtkWidget        *widget);
static void     btk_hsv_realize        (BtkWidget        *widget);
static void     btk_hsv_unrealize      (BtkWidget        *widget);
static void     btk_hsv_size_request   (BtkWidget        *widget,
					BtkRequisition   *requisition);
static void     btk_hsv_size_allocate  (BtkWidget        *widget,
					BtkAllocation    *allocation);
static bint     btk_hsv_button_press   (BtkWidget        *widget,
					BdkEventButton   *event);
static bint     btk_hsv_button_release (BtkWidget        *widget,
					BdkEventButton   *event);
static bint     btk_hsv_motion         (BtkWidget        *widget,
					BdkEventMotion   *event);
static bint     btk_hsv_expose         (BtkWidget        *widget,
					BdkEventExpose   *event);
static bboolean btk_hsv_grab_broken    (BtkWidget          *widget,
					BdkEventGrabBroken *event);
static bboolean btk_hsv_focus          (BtkWidget        *widget,
					BtkDirectionType  direction);
static void     btk_hsv_move           (BtkHSV           *hsv,
					BtkDirectionType  dir);

static buint hsv_signals[LAST_SIGNAL];

G_DEFINE_TYPE (BtkHSV, btk_hsv, BTK_TYPE_WIDGET)

/* Class initialization function for the HSV color selector */
static void
btk_hsv_class_init (BtkHSVClass *class)
{
  BObjectClass   *bobject_class;
  BtkObjectClass *object_class;
  BtkWidgetClass *widget_class;
  BtkHSVClass    *hsv_class;
  BtkBindingSet  *binding_set;
  
  bobject_class = (BObjectClass *) class;
  object_class = (BtkObjectClass *) class;
  widget_class = (BtkWidgetClass *) class;
  hsv_class = BTK_HSV_CLASS (class);
  
  object_class->destroy = btk_hsv_destroy;
  
  widget_class->map = btk_hsv_map;
  widget_class->unmap = btk_hsv_unmap;                                      
  widget_class->realize = btk_hsv_realize;
  widget_class->unrealize = btk_hsv_unrealize;
  widget_class->size_request = btk_hsv_size_request;
  widget_class->size_allocate = btk_hsv_size_allocate;
  widget_class->button_press_event = btk_hsv_button_press;
  widget_class->button_release_event = btk_hsv_button_release;
  widget_class->motion_notify_event = btk_hsv_motion;
  widget_class->expose_event = btk_hsv_expose;
  widget_class->focus = btk_hsv_focus;
  widget_class->grab_broken_event = btk_hsv_grab_broken;
  
  hsv_class->move = btk_hsv_move;
  
  hsv_signals[CHANGED] =
    g_signal_new (I_("changed"),
		  B_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkHSVClass, changed),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  B_TYPE_NONE, 0);

  hsv_signals[MOVE] =
    g_signal_new (I_("move"),
		  B_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkHSVClass, move),
		  NULL, NULL,
		  _btk_marshal_VOID__ENUM,
		  B_TYPE_NONE, 1,
		  BTK_TYPE_DIRECTION_TYPE);

  binding_set = btk_binding_set_by_class (class);

  btk_binding_entry_add_signal (binding_set, BDK_Up, 0,
                                "move", 1,
                                B_TYPE_ENUM, BTK_DIR_UP);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Up, 0,
                                "move", 1,
                                B_TYPE_ENUM, BTK_DIR_UP);
  
  btk_binding_entry_add_signal (binding_set, BDK_Down, 0,
                                "move", 1,
                                B_TYPE_ENUM, BTK_DIR_DOWN);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Down, 0,
                                "move", 1,
                                B_TYPE_ENUM, BTK_DIR_DOWN);

  
  btk_binding_entry_add_signal (binding_set, BDK_Right, 0,
                                "move", 1,
                                B_TYPE_ENUM, BTK_DIR_RIGHT);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Right, 0,
                                "move", 1,
                                B_TYPE_ENUM, BTK_DIR_RIGHT);
  
  btk_binding_entry_add_signal (binding_set, BDK_Left, 0,
                                "move", 1,
                                B_TYPE_ENUM, BTK_DIR_LEFT);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Left, 0,
                                "move", 1,
                                B_TYPE_ENUM, BTK_DIR_LEFT);

  g_type_class_add_private (bobject_class, sizeof (HSVPrivate));   
}

/* Object initialization function for the HSV color selector */
static void
btk_hsv_init (BtkHSV *hsv)
{
  HSVPrivate *priv;

  priv = B_TYPE_INSTANCE_GET_PRIVATE (hsv, BTK_TYPE_HSV, HSVPrivate);
  
  hsv->priv = priv;

  btk_widget_set_has_window (BTK_WIDGET (hsv), FALSE);
  btk_widget_set_can_focus (BTK_WIDGET (hsv), TRUE);
  
  priv->h = 0.0;
  priv->s = 0.0;
  priv->v = 0.0;
  
  priv->size = DEFAULT_SIZE;
  priv->ring_width = DEFAULT_RING_WIDTH;
}

/* Destroy handler for the HSV color selector */
static void
btk_hsv_destroy (BtkObject *object)
{
  BTK_OBJECT_CLASS (btk_hsv_parent_class)->destroy (object);
}

/* Default signal handlers */

    
/* Map handler for the HSV color selector */

static void
btk_hsv_map (BtkWidget *widget)
{
  BtkHSV *hsv;
  HSVPrivate *priv;

  hsv = BTK_HSV (widget);
  priv = hsv->priv;

  BTK_WIDGET_CLASS (btk_hsv_parent_class)->map (widget);

  bdk_window_show (priv->window);
}

/* Unmap handler for the HSV color selector */

static void
btk_hsv_unmap (BtkWidget *widget)
{
  BtkHSV *hsv;
  HSVPrivate *priv;

  hsv = BTK_HSV (widget);
  priv = hsv->priv;

  bdk_window_hide (priv->window);

  BTK_WIDGET_CLASS (btk_hsv_parent_class)->unmap (widget);
}                                                                           
                                      
/* Realize handler for the HSV color selector */
static void
btk_hsv_realize (BtkWidget *widget)
{
  BtkHSV *hsv;
  HSVPrivate *priv;
  BdkWindowAttr attr;
  int attr_mask;
  BdkWindow *parent_window;
  
  hsv = BTK_HSV (widget);
  priv = hsv->priv;
  
  btk_widget_set_realized (widget, TRUE);
  
  /* Create window */
  
  attr.window_type = BDK_WINDOW_CHILD;
  attr.x = widget->allocation.x;
  attr.y = widget->allocation.y;
  attr.width = widget->allocation.width;
  attr.height = widget->allocation.height;
  attr.wclass = BDK_INPUT_ONLY;
  attr.event_mask = btk_widget_get_events (widget);
  attr.event_mask |= (BDK_KEY_PRESS_MASK
                      | BDK_BUTTON_PRESS_MASK
		      | BDK_BUTTON_RELEASE_MASK
		      | BDK_POINTER_MOTION_MASK
                      | BDK_ENTER_NOTIFY_MASK
                      | BDK_LEAVE_NOTIFY_MASK);
  
  attr_mask = BDK_WA_X | BDK_WA_Y;
  
  parent_window = btk_widget_get_parent_window (widget);
  
  widget->window = parent_window;
  g_object_ref (widget->window);
  
  priv->window = bdk_window_new (parent_window, &attr, attr_mask);
  bdk_window_set_user_data (priv->window, hsv);
  
  widget->style = btk_style_attach (widget->style, widget->window);
}

/* Unrealize handler for the HSV color selector */
static void
btk_hsv_unrealize (BtkWidget *widget)
{
  BtkHSV *hsv;
  HSVPrivate *priv;
  
  hsv = BTK_HSV (widget);
  priv = hsv->priv;
  
  bdk_window_set_user_data (priv->window, NULL);
  bdk_window_destroy (priv->window);
  priv->window = NULL;
  
  BTK_WIDGET_CLASS (btk_hsv_parent_class)->unrealize (widget);
}

/* Size_request handler for the HSV color selector */
static void
btk_hsv_size_request (BtkWidget      *widget,
		      BtkRequisition *requisition)
{
  BtkHSV *hsv = BTK_HSV (widget);
  HSVPrivate *priv = hsv->priv;
  bint focus_width;
  bint focus_pad;

  btk_widget_style_get (widget,
			"focus-line-width", &focus_width,
			"focus-padding", &focus_pad,
			NULL);
  
  requisition->width = priv->size + 2 * (focus_width + focus_pad);
  requisition->height = priv->size + 2 * (focus_width + focus_pad);
}

/* Size_allocate handler for the HSV color selector */
static void
btk_hsv_size_allocate (BtkWidget     *widget,
		       BtkAllocation *allocation)
{
  BtkHSV *hsv;
  HSVPrivate *priv;
  
  hsv = BTK_HSV (widget);
  priv = hsv->priv;
  
  widget->allocation = *allocation;
  
  if (btk_widget_get_realized (widget))
    bdk_window_move_resize (priv->window,
			    allocation->x,
			    allocation->y,
			    allocation->width,
			    allocation->height);
}


/* Utility functions */

#define INTENSITY(r, g, b) ((r) * 0.30 + (g) * 0.59 + (b) * 0.11)

/* Converts from HSV to RGB */
static void
hsv_to_rgb (bdouble *h,
	    bdouble *s,
	    bdouble *v)
{
  bdouble hue, saturation, value;
  bdouble f, p, q, t;
  
  if (*s == 0.0)
    {
      *h = *v;
      *s = *v;
      *v = *v; /* heh */
    }
  else
    {
      hue = *h * 6.0;
      saturation = *s;
      value = *v;
      
      if (hue == 6.0)
	hue = 0.0;
      
      f = hue - (int) hue;
      p = value * (1.0 - saturation);
      q = value * (1.0 - saturation * f);
      t = value * (1.0 - saturation * (1.0 - f));
      
      switch ((int) hue)
	{
	case 0:
	  *h = value;
	  *s = t;
	  *v = p;
	  break;
	  
	case 1:
	  *h = q;
	  *s = value;
	  *v = p;
	  break;
	  
	case 2:
	  *h = p;
	  *s = value;
	  *v = t;
	  break;
	  
	case 3:
	  *h = p;
	  *s = q;
	  *v = value;
	  break;
	  
	case 4:
	  *h = t;
	  *s = p;
	  *v = value;
	  break;
	  
	case 5:
	  *h = value;
	  *s = p;
	  *v = q;
	  break;
	  
	default:
	  g_assert_not_reached ();
	}
    }
}

/* Converts from RGB to HSV */
static void
rgb_to_hsv (bdouble *r,
	    bdouble *g,
	    bdouble *b)
{
  bdouble red, green, blue;
  bdouble h, s, v;
  bdouble min, max;
  bdouble delta;
  
  red = *r;
  green = *g;
  blue = *b;
  
  h = 0.0;
  
  if (red > green)
    {
      if (red > blue)
	max = red;
      else
	max = blue;
      
      if (green < blue)
	min = green;
      else
	min = blue;
    }
  else
    {
      if (green > blue)
	max = green;
      else
	max = blue;
      
      if (red < blue)
	min = red;
      else
	min = blue;
    }
  
  v = max;
  
  if (max != 0.0)
    s = (max - min) / max;
  else
    s = 0.0;
  
  if (s == 0.0)
    h = 0.0;
  else
    {
      delta = max - min;
      
      if (red == max)
	h = (green - blue) / delta;
      else if (green == max)
	h = 2 + (blue - red) / delta;
      else if (blue == max)
	h = 4 + (red - green) / delta;
      
      h /= 6.0;
      
      if (h < 0.0)
	h += 1.0;
      else if (h > 1.0)
	h -= 1.0;
    }
  
  *r = h;
  *g = s;
  *b = v;
}

/* Computes the vertices of the saturation/value triangle */
static void
compute_triangle (BtkHSV *hsv,
		  bint   *hx,
		  bint   *hy,
		  bint   *sx,
		  bint   *sy,
		  bint   *vx,
		  bint   *vy)
{
  HSVPrivate *priv;
  bdouble center_x;
  bdouble center_y;
  bdouble inner, outer;
  bdouble angle;

  priv = hsv->priv;

  center_x = BTK_WIDGET (hsv)->allocation.width / 2.0;
  center_y = BTK_WIDGET (hsv)->allocation.height / 2.0;
  outer = priv->size / 2.0;
  inner = outer - priv->ring_width;
  angle = priv->h * 2.0 * G_PI;

  *hx = floor (center_x + cos (angle) * inner + 0.5);
  *hy = floor (center_y - sin (angle) * inner + 0.5);
  *sx = floor (center_x + cos (angle + 2.0 * G_PI / 3.0) * inner + 0.5);
  *sy = floor (center_y - sin (angle + 2.0 * G_PI / 3.0) * inner + 0.5);
  *vx = floor (center_x + cos (angle + 4.0 * G_PI / 3.0) * inner + 0.5);
  *vy = floor (center_y - sin (angle + 4.0 * G_PI / 3.0) * inner + 0.5);
}

/* Computes whether a point is inside the hue ring */
static bboolean
is_in_ring (BtkHSV *hsv,
	    bdouble x,
	    bdouble y)
{
  HSVPrivate *priv;
  bdouble dx, dy, dist;
  bdouble center_x;
  bdouble center_y;
  bdouble inner, outer;

  priv = hsv->priv;

  center_x = BTK_WIDGET (hsv)->allocation.width / 2.0;
  center_y = BTK_WIDGET (hsv)->allocation.height / 2.0;
  outer = priv->size / 2.0;
  inner = outer - priv->ring_width;

  dx = x - center_x;
  dy = center_y - y;
  dist = dx * dx + dy * dy;

  return (dist >= inner * inner && dist <= outer * outer);
}

/* Computes a saturation/value pair based on the mouse coordinates */
static void
compute_sv (BtkHSV  *hsv,
	    bdouble  x,
	    bdouble  y,
	    bdouble *s,
	    bdouble *v)
{
  int ihx, ihy, isx, isy, ivx, ivy;
  double hx, hy, sx, sy, vx, vy;
  double center_x;
  double center_y;

  compute_triangle (hsv, &ihx, &ihy, &isx, &isy, &ivx, &ivy);
  center_x = BTK_WIDGET (hsv)->allocation.width / 2.0;
  center_y = BTK_WIDGET (hsv)->allocation.height / 2.0;
  hx = ihx - center_x;
  hy = center_y - ihy;
  sx = isx - center_x;
  sy = center_y - isy;
  vx = ivx - center_x;
  vy = center_y - ivy;
  x -= center_x;
  y = center_y - y;
  
  if (vx * (x - sx) + vy * (y - sy) < 0.0)
    {
      *s = 1.0;
      *v = (((x - sx) * (hx - sx) + (y - sy) * (hy-sy))
	    / ((hx - sx) * (hx - sx) + (hy - sy) * (hy - sy)));
      
      if (*v < 0.0)
	*v = 0.0;
      else if (*v > 1.0)
	*v = 1.0;
    }
  else if (hx * (x - sx) + hy * (y - sy) < 0.0)
    {
      *s = 0.0;
      *v = (((x - sx) * (vx - sx) + (y - sy) * (vy - sy))
	    / ((vx - sx) * (vx - sx) + (vy - sy) * (vy - sy)));
      
      if (*v < 0.0)
	*v = 0.0;
      else if (*v > 1.0)
	*v = 1.0;
    }
  else if (sx * (x - hx) + sy * (y - hy) < 0.0)
    {
      *v = 1.0;
      *s = (((x - vx) * (hx - vx) + (y - vy) * (hy - vy)) /
	    ((hx - vx) * (hx - vx) + (hy - vy) * (hy - vy)));
      
      if (*s < 0.0)
	*s = 0.0;
      else if (*s > 1.0)
	*s = 1.0;
    }
  else
    {
      *v = (((x - sx) * (hy - vy) - (y - sy) * (hx - vx))
	    / ((vx - sx) * (hy - vy) - (vy - sy) * (hx - vx)));
      
      if (*v<= 0.0)
	{
	  *v = 0.0;
	  *s = 0.0;
	}
      else
	{
	  if (*v > 1.0)
	    *v = 1.0;

	  if (fabs (hy - vy) < fabs (hx - vx))
	    *s = (x - sx - *v * (vx - sx)) / (*v * (hx - vx));
	  else
	    *s = (y - sy - *v * (vy - sy)) / (*v * (hy - vy));
	    
	  if (*s < 0.0)
	    *s = 0.0;
	  else if (*s > 1.0)
	    *s = 1.0;
	}
    }
}

/* Computes whether a point is inside the saturation/value triangle */
static bboolean
is_in_triangle (BtkHSV *hsv,
		bdouble x,
		bdouble y)
{
  int hx, hy, sx, sy, vx, vy;
  double det, s, v;
  
  compute_triangle (hsv, &hx, &hy, &sx, &sy, &vx, &vy);
  
  det = (vx - sx) * (hy - sy) - (vy - sy) * (hx - sx);
  
  s = ((x - sx) * (hy - sy) - (y - sy) * (hx - sx)) / det;
  v = ((vx - sx) * (y - sy) - (vy - sy) * (x - sx)) / det;
  
  return (s >= 0.0 && v >= 0.0 && s + v <= 1.0);
}

/* Computes a value based on the mouse coordinates */
static double
compute_v (BtkHSV *hsv,
	   bdouble x,
	   bdouble y)
{
  double center_x;
  double center_y;
  double dx, dy;
  double angle;

  center_x = BTK_WIDGET (hsv)->allocation.width / 2.0;
  center_y = BTK_WIDGET (hsv)->allocation.height / 2.0;
  dx = x - center_x;
  dy = center_y - y;

  angle = atan2 (dy, dx);
  if (angle < 0.0)
    angle += 2.0 * G_PI;

  return angle / (2.0 * G_PI);
}

/* Event handlers */

static void
set_cross_grab (BtkHSV *hsv,
		buint32 time)
{
  HSVPrivate *priv;
  BdkCursor *cursor;
  
  priv = hsv->priv;
  
  cursor = bdk_cursor_new_for_display (btk_widget_get_display (BTK_WIDGET (hsv)),
				       BDK_CROSSHAIR);
  bdk_pointer_grab (priv->window, FALSE,
		    (BDK_POINTER_MOTION_MASK
		     | BDK_POINTER_MOTION_HINT_MASK
		     | BDK_BUTTON_RELEASE_MASK),
		    NULL,
		    cursor,
		    time);
  bdk_cursor_unref (cursor);
}

static bboolean 
btk_hsv_grab_broken (BtkWidget          *widget,
		     BdkEventGrabBroken *event)
{
  BtkHSV *hsv = BTK_HSV (widget);
  HSVPrivate *priv;
  
  priv = hsv->priv;
  
  priv->mode = DRAG_NONE;
  
  return TRUE;
}

/* Button_press_event handler for the HSV color selector */
static bint
btk_hsv_button_press (BtkWidget      *widget,
		      BdkEventButton *event)
{
  BtkHSV *hsv;
  HSVPrivate *priv;
  double x, y;
  
  hsv = BTK_HSV (widget);
  priv = hsv->priv;
  
  if (priv->mode != DRAG_NONE || event->button != 1)
    return FALSE;
  
  x = event->x;
  y = event->y;
  
  if (is_in_ring (hsv, x, y))
    {
      priv->mode = DRAG_H;
      set_cross_grab (hsv, event->time);
      
      btk_hsv_set_color (hsv,
			 compute_v (hsv, x, y),
			 priv->s,
			 priv->v);

      btk_widget_grab_focus (widget);
      priv->focus_on_ring = TRUE;
      
      return TRUE;
    }
  
  if (is_in_triangle (hsv, x, y))
    {
      bdouble s, v;
      
      priv->mode = DRAG_SV;
      set_cross_grab (hsv, event->time);
      
      compute_sv (hsv, x, y, &s, &v);
      btk_hsv_set_color (hsv, priv->h, s, v);

      btk_widget_grab_focus (widget);
      priv->focus_on_ring = FALSE;
      
      return TRUE;
    }
  
  return FALSE;
}

/* Button_release_event handler for the HSV color selector */
static bint
btk_hsv_button_release (BtkWidget      *widget,
			BdkEventButton *event)
{
  BtkHSV *hsv;
  HSVPrivate *priv;
  DragMode mode;
  bdouble x, y;
  
  hsv = BTK_HSV (widget);
  priv = hsv->priv;
  
  if (priv->mode == DRAG_NONE || event->button != 1)
    return FALSE;
  
  /* Set the drag mode to DRAG_NONE so that signal handlers for "catched"
   * can see that this is the final color state.
   */
  
  mode = priv->mode;
  priv->mode = DRAG_NONE;
  
  x = event->x;
  y = event->y;
  
  if (mode == DRAG_H)
    btk_hsv_set_color (hsv, compute_v (hsv, x, y), priv->s, priv->v);
  else if (mode == DRAG_SV) {
    double s, v;
    
    compute_sv (hsv, x, y, &s, &v);
    btk_hsv_set_color (hsv, priv->h, s, v);
  } else
    g_assert_not_reached ();
  
  bdk_display_pointer_ungrab (bdk_window_get_display (event->window),
			      event->time);
  return TRUE;
}

/* Motion_notify_event handler for the HSV color selector */
static bint
btk_hsv_motion (BtkWidget      *widget,
		BdkEventMotion *event)
{
  BtkHSV *hsv;
  HSVPrivate *priv;
  double x, y;
  BdkModifierType mods;
  
  hsv = BTK_HSV (widget);
  priv = hsv->priv;
  
  if (priv->mode == DRAG_NONE)
    return FALSE;
  
  bdk_event_request_motions (event);
  x = event->x;
  y = event->y;
  mods = event->state;

  if (priv->mode == DRAG_H)
    {
      btk_hsv_set_color (hsv, compute_v (hsv, x, y), priv->s, priv->v);
      return TRUE;
    }
  else if (priv->mode == DRAG_SV)
    {
      double s, v;
      
      compute_sv (hsv, x, y, &s, &v);
      btk_hsv_set_color (hsv, priv->h, s, v);
      return TRUE;
    }
  
  g_assert_not_reached ();
  return FALSE;
}


/* Redrawing */

/* Paints the hue ring */
static void
paint_ring (BtkHSV      *hsv,
	    bairo_t     *cr,
	    bint         x,
	    bint         y,
	    bint         width,
	    bint         height)
{
  BtkWidget *widget = BTK_WIDGET (hsv);
  HSVPrivate *priv;
  int xx, yy;
  bdouble dx, dy, dist;
  bdouble center_x;
  bdouble center_y;
  bdouble inner, outer;
  buint32 *buf, *p;
  bdouble angle;
  bdouble hue;
  bdouble r, g, b;
  bairo_surface_t *source;
  bairo_t *source_cr;
  bint stride;
  bint focus_width;
  bint focus_pad;

  btk_widget_style_get (widget,
			"focus-line-width", &focus_width,
			"focus-padding", &focus_pad,
			NULL);
  
  priv = hsv->priv;

  center_x = widget->allocation.width / 2.0;
  center_y = widget->allocation.height / 2.0;

  outer = priv->size / 2.0;
  inner = outer - priv->ring_width;
  
  /* Create an image initialized with the ring colors */
  
  stride = bairo_format_stride_for_width (BAIRO_FORMAT_RGB24, width);
  buf = g_new (buint32, height * stride / 4);
  
  for (yy = 0; yy < height; yy++)
    {
      p = buf + yy * width;
      
      dy = -(yy + y - center_y);
      
      for (xx = 0; xx < width; xx++)
	{
	  dx = xx + x - center_x;
	  
	  dist = dx * dx + dy * dy;
	  if (dist < ((inner-1) * (inner-1)) || dist > ((outer+1) * (outer+1)))
	    {
	      *p++ = 0;
	      continue;
	    }
	  
	  angle = atan2 (dy, dx);
	  if (angle < 0.0)
	    angle += 2.0 * G_PI;
	  
	  hue = angle / (2.0 * G_PI);
	  
	  r = hue;
	  g = 1.0;
	  b = 1.0;
	  hsv_to_rgb (&r, &g, &b);
	  
	  *p++ = (((int)floor (r * 255 + 0.5) << 16) |
		  ((int)floor (g * 255 + 0.5) << 8) |
		  (int)floor (b * 255 + 0.5));
	}
    }

  source = bairo_image_surface_create_for_data ((unsigned char *)buf,
						BAIRO_FORMAT_RGB24,
						width, height, stride);

  /* Now draw the value marker onto the source image, so that it
   * will get properly clipped at the edges of the ring
   */
  source_cr = bairo_create (source);
  
  r = priv->h;
  g = 1.0;
  b = 1.0;
  hsv_to_rgb (&r, &g, &b);
  
  if (INTENSITY (r, g, b) > 0.5)
    bairo_set_source_rgb (source_cr, 0., 0., 0.);
  else
    bairo_set_source_rgb (source_cr, 1., 1., 1.);

  bairo_move_to (source_cr, -x + center_x, - y + center_y);
  bairo_line_to (source_cr,
		 -x + center_x + cos (priv->h * 2.0 * G_PI) * priv->size / 2,
		 -y + center_y - sin (priv->h * 2.0 * G_PI) * priv->size / 2);
  bairo_stroke (source_cr);
  bairo_destroy (source_cr);

  /* Draw the ring using the source image */

  bairo_save (cr);
    
  bairo_set_source_surface (cr, source, x, y);
  bairo_surface_destroy (source);

  bairo_set_line_width (cr, priv->ring_width);
  bairo_new_path (cr);
  bairo_arc (cr,
	     center_x, center_y,
	     priv->size / 2. - priv->ring_width / 2.,
	     0, 2 * G_PI);
  bairo_stroke (cr);
  
  bairo_restore (cr);
  
  g_free (buf);
}

/* Converts an HSV triplet to an integer RGB triplet */
static void
get_color (bdouble h,
	   bdouble s,
	   bdouble v,
	   bint   *r,
	   bint   *g,
	   bint   *b)
{
  hsv_to_rgb (&h, &s, &v);
  
  *r = floor (h * 255 + 0.5);
  *g = floor (s * 255 + 0.5);
  *b = floor (v * 255 + 0.5);
}

#define SWAP(a, b, t) ((t) = (a), (a) = (b), (b) = (t))

#define LERP(a, b, v1, v2, i) (((v2) - (v1) != 0)					\
			       ? ((a) + ((b) - (a)) * ((i) - (v1)) / ((v2) - (v1)))	\
			       : (a))

/* Number of pixels we extend out from the edges when creating
 * color source to avoid artifacts
 */
#define PAD 3

/* Paints the HSV triangle */
static void
paint_triangle (BtkHSV      *hsv,
		bairo_t     *cr,
		bint         x,
		bint         y,
		bint         width,
		bint         height)
{
  BtkWidget *widget = BTK_WIDGET (hsv);
  HSVPrivate *priv;
  bint hx, hy, sx, sy, vx, vy; /* HSV vertices */
  bint x1, y1, r1, g1, b1; /* First vertex in scanline order */
  bint x2, y2, r2, g2, b2; /* Second vertex */
  bint x3, y3, r3, g3, b3; /* Third vertex */
  bint t;
  buint32 *buf, *p, c;
  bint xl, xr, rl, rr, gl, gr, bl, br; /* Scanline data */
  bint xx, yy;
  bint x_interp, y_interp;
  bint x_start, x_end;
  bairo_surface_t *source;
  bdouble r, g, b;
  bchar *detail;
  bint stride;
  
  priv = hsv->priv;
  
  /* Compute triangle's vertices */
  
  compute_triangle (hsv, &hx, &hy, &sx, &sy, &vx, &vy);
  
  x1 = hx;
  y1 = hy;
  get_color (priv->h, 1.0, 1.0, &r1, &g1, &b1);
  
  x2 = sx;
  y2 = sy;
  get_color (priv->h, 1.0, 0.0, &r2, &g2, &b2);
  
  x3 = vx;
  y3 = vy;
  get_color (priv->h, 0.0, 1.0, &r3, &g3, &b3);
  
  if (y2 > y3)
    {
      SWAP (x2, x3, t);
      SWAP (y2, y3, t);
      SWAP (r2, r3, t);
      SWAP (g2, g3, t);
      SWAP (b2, b3, t);
    }
  
  if (y1 > y3)
    {
      SWAP (x1, x3, t);
      SWAP (y1, y3, t);
      SWAP (r1, r3, t);
      SWAP (g1, g3, t);
      SWAP (b1, b3, t);
    }
  
  if (y1 > y2)
    {
      SWAP (x1, x2, t);
      SWAP (y1, y2, t);
      SWAP (r1, r2, t);
      SWAP (g1, g2, t);
      SWAP (b1, b2, t);
    }
  
  /* Shade the triangle */

  stride = bairo_format_stride_for_width (BAIRO_FORMAT_RGB24, width);
  buf = g_new (buint32, height * stride / 4);
  
  for (yy = 0; yy < height; yy++)
    {
      p = buf + yy * width;
      
      if (yy + y >= y1 - PAD && yy + y < y3 + PAD) {
	y_interp = CLAMP (yy + y, y1, y3);
	
	if (y_interp < y2)
	  {
	    xl = LERP (x1, x2, y1, y2, y_interp);
	    
	    rl = LERP (r1, r2, y1, y2, y_interp);
	    gl = LERP (g1, g2, y1, y2, y_interp);
	    bl = LERP (b1, b2, y1, y2, y_interp);
	  }
	else
	  {
	    xl = LERP (x2, x3, y2, y3, y_interp);
	    
	    rl = LERP (r2, r3, y2, y3, y_interp);
	    gl = LERP (g2, g3, y2, y3, y_interp);
	    bl = LERP (b2, b3, y2, y3, y_interp);
	  }
	
	xr = LERP (x1, x3, y1, y3, y_interp);
	
	rr = LERP (r1, r3, y1, y3, y_interp);
	gr = LERP (g1, g3, y1, y3, y_interp);
	br = LERP (b1, b3, y1, y3, y_interp);
	
	if (xl > xr)
	  {
	    SWAP (xl, xr, t);
	    SWAP (rl, rr, t);
	    SWAP (gl, gr, t);
	    SWAP (bl, br, t);
	  }

	x_start = MAX (xl - PAD, x);
	x_end = MIN (xr + PAD, x + width);
	x_start = MIN (x_start, x_end);

	c = (rl << 16) | (gl << 8) | bl;

	for (xx = x; xx < x_start; xx++)
	  *p++ = c;
	  
	for (; xx < x_end; xx++)
	  {
	    x_interp = CLAMP (xx, xl, xr);
		
	    *p++ = ((LERP (rl, rr, xl, xr, x_interp) << 16) |
		    (LERP (gl, gr, xl, xr, x_interp) << 8) |
		    LERP (bl, br, xl, xr, x_interp));
	  }

	c = (rr << 16) | (gr << 8) | br;

	for (; xx < x + width; xx++)
	  *p++ = c;
      }
    }

  source = bairo_image_surface_create_for_data ((unsigned char *)buf,
						BAIRO_FORMAT_RGB24,
						width, height, stride);
  
  /* Draw a triangle with the image as a source */

  bairo_set_source_surface (cr, source, x, y);
  bairo_surface_destroy (source);
  
  bairo_move_to (cr, x1, y1);
  bairo_line_to (cr, x2, y2);
  bairo_line_to (cr, x3, y3);
  bairo_close_path (cr);
  bairo_fill (cr);
  
  g_free (buf);
  
  /* Draw value marker */
  
  xx = floor (sx + (vx - sx) * priv->v + (hx - vx) * priv->s * priv->v + 0.5);
  yy = floor (sy + (vy - sy) * priv->v + (hy - vy) * priv->s * priv->v + 0.5);
  
  r = priv->h;
  g = priv->s;
  b = priv->v;
  hsv_to_rgb (&r, &g, &b);

  if (INTENSITY (r, g, b) > 0.5)
    {
      detail = "colorwheel_light";
      bairo_set_source_rgb (cr, 0., 0., 0.);
    }
  else
    {
      detail = "colorwheel_dark";
      bairo_set_source_rgb (cr, 1., 1., 1.);
    }

#define RADIUS 4
#define FOCUS_RADIUS 6

  bairo_new_path (cr);
  bairo_arc (cr, xx, yy, RADIUS, 0, 2 * G_PI);
  bairo_stroke (cr);
  
  /* Draw focus outline */

  if (btk_widget_has_focus (widget) &&
      !priv->focus_on_ring)
    {
      bint focus_width;
      bint focus_pad;

      btk_widget_style_get (widget,
			    "focus-line-width", &focus_width,
			    "focus-padding", &focus_pad,
			    NULL);
  
      btk_paint_focus (widget->style, widget->window,
		       btk_widget_get_state (widget),
		       NULL, widget, detail,
		       widget->allocation.x + xx - FOCUS_RADIUS - focus_width - focus_pad, 
		       widget->allocation.y + yy - FOCUS_RADIUS - focus_width - focus_pad, 
		       2 * (FOCUS_RADIUS + focus_width + focus_pad), 
		       2 * (FOCUS_RADIUS + focus_width + focus_pad));
    }
  
}

/* Paints the contents of the HSV color selector */
static void
paint (BtkHSV      *hsv,
       bairo_t     *cr,
       bint         x,
       bint         y,
       bint         width,
       bint         height)
{
  paint_ring (hsv, cr, x, y, width, height);
  paint_triangle (hsv, cr, x, y, width, height);
}

/* Expose_event handler for the HSV color selector */
static bint
btk_hsv_expose (BtkWidget      *widget,
		BdkEventExpose *event)
{
  BtkHSV *hsv;
  HSVPrivate *priv;
  BdkRectangle rect, dest;
  bairo_t *cr;
  
  hsv = BTK_HSV (widget);
  priv = hsv->priv;
  
  if (!(event->window == widget->window && btk_widget_is_drawable (widget)))
    return FALSE;

  rect.x = widget->allocation.x;
  rect.y = widget->allocation.y;
  rect.width = widget->allocation.width;
  rect.height = widget->allocation.height;
  
  if (!bdk_rectangle_intersect (&event->area, &rect, &dest))
    return FALSE;
  
  cr = bdk_bairo_create (widget->window);

  bairo_translate (cr, widget->allocation.x, widget->allocation.y);
  paint (hsv, cr,
	 dest.x - widget->allocation.x,
	 dest.y - widget->allocation.y,
	 dest.width, dest.height);
  bairo_destroy (cr);

  if (btk_widget_has_focus (widget) && priv->focus_on_ring)
    btk_paint_focus (widget->style, widget->window,
		     btk_widget_get_state (widget),
		     &event->area, widget, NULL,
		     widget->allocation.x,
		     widget->allocation.y, 
		     widget->allocation.width, 
		     widget->allocation.height);

  return FALSE;
}

static bboolean
btk_hsv_focus (BtkWidget       *widget,
               BtkDirectionType dir)
{
  BtkHSV *hsv;
  HSVPrivate *priv;

  hsv = BTK_HSV (widget);
  priv = hsv->priv;

  if (!btk_widget_has_focus (widget))
    {
      if (dir == BTK_DIR_TAB_BACKWARD)
        priv->focus_on_ring = FALSE;
      else
        priv->focus_on_ring = TRUE;

      btk_widget_grab_focus (BTK_WIDGET (hsv));
      return TRUE;
    }
  
  switch (dir)
    {
    case BTK_DIR_UP:
      if (priv->focus_on_ring)
        return FALSE;
      else
        priv->focus_on_ring = TRUE;
      break;

    case BTK_DIR_DOWN:
      if (priv->focus_on_ring)
        priv->focus_on_ring = FALSE;
      else
        return FALSE;
      break;

    case BTK_DIR_LEFT:
    case BTK_DIR_TAB_BACKWARD:
      if (priv->focus_on_ring)
        return FALSE;
      else
        priv->focus_on_ring = TRUE;
      break;

    case BTK_DIR_RIGHT:
    case BTK_DIR_TAB_FORWARD:
      if (priv->focus_on_ring)
        priv->focus_on_ring = FALSE;
      else
        return FALSE;
      break;
    }

  btk_widget_queue_draw (BTK_WIDGET (hsv));

  return TRUE;
}

/**
 * btk_hsv_new:
 *
 * Creates a new HSV color selector.
 *
 * Return value: A newly-created HSV color selector.
 *
 * Since: 2.14
 */
BtkWidget*
btk_hsv_new (void)
{
  return g_object_new (BTK_TYPE_HSV, NULL);
}

/**
 * btk_hsv_set_color:
 * @hsv: An HSV color selector
 * @h: Hue
 * @s: Saturation
 * @v: Value
 *
 * Sets the current color in an HSV color selector.
 * Color component values must be in the [0.0, 1.0] range.
 *
 * Since: 2.14
 */
void
btk_hsv_set_color (BtkHSV *hsv,
		   bdouble h,
		   bdouble s,
		   bdouble v)
{
  HSVPrivate *priv;
  
  g_return_if_fail (BTK_IS_HSV (hsv));
  g_return_if_fail (h >= 0.0 && h <= 1.0);
  g_return_if_fail (s >= 0.0 && s <= 1.0);
  g_return_if_fail (v >= 0.0 && v <= 1.0);
  
  priv = hsv->priv;
  
  priv->h = h;
  priv->s = s;
  priv->v = v;
  
  g_signal_emit (hsv, hsv_signals[CHANGED], 0);
  
  btk_widget_queue_draw (BTK_WIDGET (hsv));
}

/**
 * btk_hsv_get_color:
 * @hsv: An HSV color selector
 * @h: (out): Return value for the hue
 * @s: (out): Return value for the saturation
 * @v: (out): Return value for the value
 *
 * Queries the current color in an HSV color selector.
 * Returned values will be in the [0.0, 1.0] range.
 *
 * Since: 2.14
 */
void
btk_hsv_get_color (BtkHSV *hsv,
                   double *h,
                   double *s,
                   double *v)
{
  HSVPrivate *priv;
  
  g_return_if_fail (BTK_IS_HSV (hsv));
  
  priv = hsv->priv;
  
  if (h)
    *h = priv->h;
  
  if (s)
    *s = priv->s;
  
  if (v)
    *v = priv->v;
}

/**
 * btk_hsv_set_metrics:
 * @hsv: An HSV color selector
 * @size: Diameter for the hue ring
 * @ring_width: Width of the hue ring
 *
 * Sets the size and ring width of an HSV color selector.
 *
 * Since: 2.14
 */
void
btk_hsv_set_metrics (BtkHSV *hsv,
		     bint    size,
		     bint    ring_width)
{
  HSVPrivate *priv;
  int same_size;
  
  g_return_if_fail (BTK_IS_HSV (hsv));
  g_return_if_fail (size > 0);
  g_return_if_fail (ring_width > 0);
  g_return_if_fail (2 * ring_width + 1 <= size);
  
  priv = hsv->priv;
  
  same_size = (priv->size == size);
  
  priv->size = size;
  priv->ring_width = ring_width;
  
  if (same_size)
    btk_widget_queue_draw (BTK_WIDGET (hsv));
  else
    btk_widget_queue_resize (BTK_WIDGET (hsv));
}

/**
 * btk_hsv_get_metrics:
 * @hsv: An HSV color selector
 * @size: (out): Return value for the diameter of the hue ring
 * @ring_width: (out): Return value for the width of the hue ring
 *
 * Queries the size and ring width of an HSV color selector.
 *
 * Since: 2.14
 */
void
btk_hsv_get_metrics (BtkHSV *hsv,
		     bint   *size,
		     bint   *ring_width)
{
  HSVPrivate *priv;
  
  g_return_if_fail (BTK_IS_HSV (hsv));
  
  priv = hsv->priv;
  
  if (size)
    *size = priv->size;
  
  if (ring_width)
    *ring_width = priv->ring_width;
}

/**
 * btk_hsv_is_adjusting:
 * @hsv: A #BtkHSV 
 *
 * An HSV color selector can be said to be adjusting if multiple rapid
 * changes are being made to its value, for example, when the user is 
 * adjusting the value with the mouse. This function queries whether 
 * the HSV color selector is being adjusted or not.
 *
 * Return value: %TRUE if clients can ignore changes to the color value,
 *     since they may be transitory, or %FALSE if they should consider
 *     the color value status to be final.
 *
 * Since: 2.14
 */
bboolean
btk_hsv_is_adjusting (BtkHSV *hsv)
{
  HSVPrivate *priv;
  
  g_return_val_if_fail (BTK_IS_HSV (hsv), FALSE);
  
  priv = hsv->priv;

  return priv->mode != DRAG_NONE;
}

/**
 * btk_hsv_to_rgb:
 * @h: Hue
 * @s: Saturation
 * @v: Value
 * @r: (out): Return value for the red component
 * @g: (out): Return value for the green component
 * @b: (out): Return value for the blue component
 *
 * Converts a color from HSV space to RGB.
 * Input values must be in the [0.0, 1.0] range; 
 * output values will be in the same range.
 *
 * Since: 2.14
 */
void
btk_hsv_to_rgb (bdouble  h,
		bdouble  s,
		bdouble  v,
		bdouble *r,
		bdouble *g,
		bdouble *b)
{
  g_return_if_fail (h >= 0.0 && h <= 1.0);
  g_return_if_fail (s >= 0.0 && s <= 1.0);
  g_return_if_fail (v >= 0.0 && v <= 1.0);
  
  hsv_to_rgb (&h, &s, &v);
  
  if (r)
    *r = h;
  
  if (g)
    *g = s;
  
  if (b)
    *b = v;
}

/**
 * btk_rgb_to_hsv:
 * @r: Red
 * @g: Green
 * @b: Blue
 * @h: (out): Return value for the hue component
 * @s: (out): Return value for the saturation component
 * @v: (out): Return value for the value component
 *
 * Converts a color from RGB space to HSV.
 * Input values must be in the [0.0, 1.0] range;
 * output values will be in the same range.
 *
 * Since: 2.14
 */
void
btk_rgb_to_hsv (bdouble  r,
		bdouble  g,
		bdouble  b,
		bdouble *h,
		bdouble *s,
		bdouble *v)
{
  g_return_if_fail (r >= 0.0 && r <= 1.0);
  g_return_if_fail (g >= 0.0 && g <= 1.0);
  g_return_if_fail (b >= 0.0 && b <= 1.0);
  
  rgb_to_hsv (&r, &g, &b);
  
  if (h)
    *h = r;
  
  if (s)
    *s = g;
  
  if (v)
    *v = b;
}

static void
btk_hsv_move (BtkHSV          *hsv,
              BtkDirectionType dir)
{
  HSVPrivate *priv;
  bdouble hue, sat, val;
  bint hx, hy, sx, sy, vx, vy; /* HSV vertices */
  bint x, y; /* position in triangle */
  
  priv = hsv->priv;

  hue = priv->h;
  sat = priv->s;
  val = priv->v;

  compute_triangle (hsv, &hx, &hy, &sx, &sy, &vx, &vy);

  x = floor (sx + (vx - sx) * priv->v + (hx - vx) * priv->s * priv->v + 0.5);
  y = floor (sy + (vy - sy) * priv->v + (hy - vy) * priv->s * priv->v + 0.5);

#define HUE_DELTA 0.002
  switch (dir)
    {
    case BTK_DIR_UP:
      if (priv->focus_on_ring)
        hue += HUE_DELTA;
      else
        {
          y -= 1;
          compute_sv (hsv, x, y, &sat, &val);
        }
      break;

    case BTK_DIR_DOWN:
      if (priv->focus_on_ring)
        hue -= HUE_DELTA;
      else
        {
          y += 1;
          compute_sv (hsv, x, y, &sat, &val);
        }
      break;

    case BTK_DIR_LEFT:
      if (priv->focus_on_ring)
        hue += HUE_DELTA;
      else
        {
          x -= 1;
          compute_sv (hsv, x, y, &sat, &val);
        }
      break;

    case BTK_DIR_RIGHT:
      if (priv->focus_on_ring)
        hue -= HUE_DELTA
          ;
      else
        {
          x += 1;
          compute_sv (hsv, x, y, &sat, &val);
        }
      break;

    default:
      /* we don't care about the tab directions */
      break;
    }

  /* Wrap */
  if (hue < 0.0)
    hue = 1.0;
  else if (hue > 1.0)
    hue = 0.0;
  
  btk_hsv_set_color (hsv, hue, sat, val);
}

#define __BTK_HSV_C__
#include "btkaliasdef.c"
