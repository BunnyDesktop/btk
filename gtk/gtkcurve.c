/* BTK - The GIMP Toolkit
 * Copyright (C) 1997 David Mosberger
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
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "btkcurve.h"
#include "btkdrawingarea.h"
#include "btkmain.h"
#include "btkmarshalers.h"
#include "btkradiobutton.h"
#include "btktable.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkalias.h"

#define RADIUS		3	/* radius of the control points */
#define MIN_DISTANCE	8	/* min distance between control points */

#define GRAPH_MASK	(BDK_EXPOSURE_MASK |		\
			 BDK_POINTER_MOTION_MASK |	\
			 BDK_POINTER_MOTION_HINT_MASK |	\
			 BDK_ENTER_NOTIFY_MASK |	\
			 BDK_BUTTON_PRESS_MASK |	\
			 BDK_BUTTON_RELEASE_MASK |	\
			 BDK_BUTTON1_MOTION_MASK)

enum {
  PROP_0,
  PROP_CURVE_TYPE,
  PROP_MIN_X,
  PROP_MAX_X,
  PROP_MIN_Y,
  PROP_MAX_Y
};

static BtkDrawingAreaClass *parent_class = NULL;
static guint curve_type_changed_signal = 0;


/* forward declarations: */
static void btk_curve_class_init   (BtkCurveClass *class);
static void btk_curve_init         (BtkCurve      *curve);
static void btk_curve_get_property  (GObject              *object,
				     guint                 param_id,
				     GValue               *value,
				     GParamSpec           *pspec);
static void btk_curve_set_property  (GObject              *object,
				     guint                 param_id,
				     const GValue         *value,
				     GParamSpec           *pspec);
static void btk_curve_finalize     (GObject       *object);
static gint btk_curve_graph_events (BtkWidget     *widget, 
				    BdkEvent      *event, 
				    BtkCurve      *c);
static void btk_curve_size_graph   (BtkCurve      *curve);

GType
btk_curve_get_type (void)
{
  static GType curve_type = 0;

  if (!curve_type)
    {
      const GTypeInfo curve_info =
      {
	sizeof (BtkCurveClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) btk_curve_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (BtkCurve),
	0,		/* n_preallocs */
	(GInstanceInitFunc) btk_curve_init,
      };

      curve_type = g_type_register_static (BTK_TYPE_DRAWING_AREA, I_("BtkCurve"),
					   &curve_info, 0);
    }
  return curve_type;
}

static void
btk_curve_class_init (BtkCurveClass *class)
{
  GObjectClass *bobject_class = G_OBJECT_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  bobject_class->finalize = btk_curve_finalize;

  bobject_class->set_property = btk_curve_set_property;
  bobject_class->get_property = btk_curve_get_property;
  
  g_object_class_install_property (bobject_class,
				   PROP_CURVE_TYPE,
				   g_param_spec_enum ("curve-type",
						      P_("Curve type"),
						      P_("Is this curve linear, spline interpolated, or free-form"),
						      BTK_TYPE_CURVE_TYPE,
						      BTK_CURVE_TYPE_SPLINE,
						      BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
				   PROP_MIN_X,
				   g_param_spec_float ("min-x",
						       P_("Minimum X"),
						       P_("Minimum possible value for X"),
						       -G_MAXFLOAT,
						       G_MAXFLOAT,
						       0.0,
						       BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
				   PROP_MAX_X,
				   g_param_spec_float ("max-x",
						       P_("Maximum X"),
						       P_("Maximum possible X value"),
						       -G_MAXFLOAT,
						       G_MAXFLOAT,
                                                       1.0,
						       BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
				   PROP_MIN_Y,
				   g_param_spec_float ("min-y",
						       P_("Minimum Y"),
						       P_("Minimum possible value for Y"),
                                                       -G_MAXFLOAT,
						       G_MAXFLOAT,
						       0.0,
						       BTK_PARAM_READWRITE));  
  g_object_class_install_property (bobject_class,
				   PROP_MAX_Y,
				   g_param_spec_float ("max-y",
						       P_("Maximum Y"),
						       P_("Maximum possible value for Y"),
						       -G_MAXFLOAT,
						       G_MAXFLOAT,
						       1.0,
						       BTK_PARAM_READWRITE));

  curve_type_changed_signal =
    g_signal_new (I_("curve-type-changed"),
		   G_OBJECT_CLASS_TYPE (bobject_class),
		   G_SIGNAL_RUN_FIRST,
		   G_STRUCT_OFFSET (BtkCurveClass, curve_type_changed),
		   NULL, NULL,
		   _btk_marshal_VOID__VOID,
		   G_TYPE_NONE, 0);
}

static void
btk_curve_init (BtkCurve *curve)
{
  gint old_mask;

  curve->cursor_type = BDK_TOP_LEFT_ARROW;
  curve->pixmap = NULL;
  curve->curve_type = BTK_CURVE_TYPE_SPLINE;
  curve->height = 0;
  curve->grab_point = -1;

  curve->num_points = 0;
  curve->point = NULL;

  curve->num_ctlpoints = 0;
  curve->ctlpoint = NULL;

  curve->min_x = 0.0;
  curve->max_x = 1.0;
  curve->min_y = 0.0;
  curve->max_y = 1.0;

  old_mask = btk_widget_get_events (BTK_WIDGET (curve));
  btk_widget_set_events (BTK_WIDGET (curve), old_mask | GRAPH_MASK);
  g_signal_connect (curve, "event",
		    G_CALLBACK (btk_curve_graph_events), curve);
  btk_curve_size_graph (curve);
}

static void
btk_curve_set_property (GObject              *object,
			guint                 prop_id,
			const GValue         *value,
			GParamSpec           *pspec)
{
  BtkCurve *curve = BTK_CURVE (object);
  
  switch (prop_id)
    {
    case PROP_CURVE_TYPE:
      btk_curve_set_curve_type (curve, g_value_get_enum (value));
      break;
    case PROP_MIN_X:
      btk_curve_set_range (curve, g_value_get_float (value), curve->max_x,
			   curve->min_y, curve->max_y);
      break;
    case PROP_MAX_X:
      btk_curve_set_range (curve, curve->min_x, g_value_get_float (value),
			   curve->min_y, curve->max_y);
      break;	
    case PROP_MIN_Y:
      btk_curve_set_range (curve, curve->min_x, curve->max_x,
			   g_value_get_float (value), curve->max_y);
      break;
    case PROP_MAX_Y:
      btk_curve_set_range (curve, curve->min_x, curve->max_x,
			   curve->min_y, g_value_get_float (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_curve_get_property (GObject              *object,
			guint                 prop_id,
			GValue               *value,
			GParamSpec           *pspec)
{
  BtkCurve *curve = BTK_CURVE (object);

  switch (prop_id)
    {
    case PROP_CURVE_TYPE:
      g_value_set_enum (value, curve->curve_type);
      break;
    case PROP_MIN_X:
      g_value_set_float (value, curve->min_x);
      break;
    case PROP_MAX_X:
      g_value_set_float (value, curve->max_x);
      break;
    case PROP_MIN_Y:
      g_value_set_float (value, curve->min_y);
      break;
    case PROP_MAX_Y:
      g_value_set_float (value, curve->max_y);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static int
project (gfloat value, gfloat min, gfloat max, int norm)
{
  return (norm - 1) * ((value - min) / (max - min)) + 0.5;
}

static gfloat
unproject (gint value, gfloat min, gfloat max, int norm)
{
  return value / (gfloat) (norm - 1) * (max - min) + min;
}

/* Solve the tridiagonal equation system that determines the second
   derivatives for the interpolation points.  (Based on Numerical
   Recipies 2nd Edition.) */
static void
spline_solve (int n, gfloat x[], gfloat y[], gfloat y2[])
{
  gfloat p, sig, *u;
  gint i, k;

  u = g_malloc ((n - 1) * sizeof (u[0]));

  y2[0] = u[0] = 0.0;	/* set lower boundary condition to "natural" */

  for (i = 1; i < n - 1; ++i)
    {
      sig = (x[i] - x[i - 1]) / (x[i + 1] - x[i - 1]);
      p = sig * y2[i - 1] + 2.0;
      y2[i] = (sig - 1.0) / p;
      u[i] = ((y[i + 1] - y[i])
	      / (x[i + 1] - x[i]) - (y[i] - y[i - 1]) / (x[i] - x[i - 1]));
      u[i] = (6.0 * u[i] / (x[i + 1] - x[i - 1]) - sig * u[i - 1]) / p;
    }

  y2[n - 1] = 0.0;
  for (k = n - 2; k >= 0; --k)
    y2[k] = y2[k] * y2[k + 1] + u[k];

  g_free (u);
}

static gfloat
spline_eval (int n, gfloat x[], gfloat y[], gfloat y2[], gfloat val)
{
  gint k_lo, k_hi, k;
  gfloat h, b, a;

  /* do a binary search for the right interval: */
  k_lo = 0; k_hi = n - 1;
  while (k_hi - k_lo > 1)
    {
      k = (k_hi + k_lo) / 2;
      if (x[k] > val)
	k_hi = k;
      else
	k_lo = k;
    }

  h = x[k_hi] - x[k_lo];
  g_assert (h > 0.0);

  a = (x[k_hi] - val) / h;
  b = (val - x[k_lo]) / h;
  return a*y[k_lo] + b*y[k_hi] +
    ((a*a*a - a)*y2[k_lo] + (b*b*b - b)*y2[k_hi]) * (h*h)/6.0;
}

static void
btk_curve_interpolate (BtkCurve *c, gint width, gint height)
{
  gfloat *vector;
  int i;

  vector = g_malloc (width * sizeof (vector[0]));

  btk_curve_get_vector (c, width, vector);

  c->height = height;
  if (c->num_points != width)
    {
      c->num_points = width;
      g_free (c->point);
      c->point = g_malloc (c->num_points * sizeof (c->point[0]));
    }

  for (i = 0; i < width; ++i)
    {
      c->point[i].x = RADIUS + i;
      c->point[i].y = RADIUS + height
	- project (vector[i], c->min_y, c->max_y, height);
    }

  g_free (vector);
}

static void
btk_curve_draw (BtkCurve *c, gint width, gint height)
{
  BtkStateType state;
  BtkStyle *style;
  gint i;

  if (!c->pixmap)
    return;

  if (c->height != height || c->num_points != width)
    btk_curve_interpolate (c, width, height);

  state = BTK_STATE_NORMAL;
  if (!btk_widget_is_sensitive (BTK_WIDGET (c)))
    state = BTK_STATE_INSENSITIVE;

  style = BTK_WIDGET (c)->style;

  /* clear the pixmap: */
  btk_paint_flat_box (style, c->pixmap, BTK_STATE_NORMAL, BTK_SHADOW_NONE,
		      NULL, BTK_WIDGET (c), "curve_bg",
		      0, 0, width + RADIUS * 2, height + RADIUS * 2);
  /* draw the grid lines: (XXX make more meaningful) */
  for (i = 0; i < 5; i++)
    {
      bdk_draw_line (c->pixmap, style->dark_gc[state],
		     RADIUS, i * (height / 4.0) + RADIUS,
		     width + RADIUS, i * (height / 4.0) + RADIUS);
      bdk_draw_line (c->pixmap, style->dark_gc[state],
		     i * (width / 4.0) + RADIUS, RADIUS,
		     i * (width / 4.0) + RADIUS, height + RADIUS);
    }

  bdk_draw_points (c->pixmap, style->fg_gc[state], c->point, c->num_points);
  if (c->curve_type != BTK_CURVE_TYPE_FREE)
    for (i = 0; i < c->num_ctlpoints; ++i)
      {
	gint x, y;

	if (c->ctlpoint[i][0] < c->min_x)
	  continue;

	x = project (c->ctlpoint[i][0], c->min_x, c->max_x,
		     width);
	y = height -
	  project (c->ctlpoint[i][1], c->min_y, c->max_y,
		   height);

	/* draw a bullet: */
	bdk_draw_arc (c->pixmap, style->fg_gc[state], TRUE, x, y,
		      RADIUS * 2, RADIUS*2, 0, 360*64);
      }
  bdk_draw_drawable (BTK_WIDGET (c)->window, style->fg_gc[state], c->pixmap,
		     0, 0, 0, 0, width + RADIUS * 2, height + RADIUS * 2);
}

static gint
btk_curve_graph_events (BtkWidget *widget, BdkEvent *event, BtkCurve *c)
{
  BdkCursorType new_type = c->cursor_type;
  gint i, src, dst, leftbound, rightbound;
  BdkEventMotion *mevent;
  BtkWidget *w;
  gint tx, ty;
  gint cx, x, y, width, height;
  gint closest_point = 0;
  gfloat rx, ry, min_x;
  guint distance;
  gint x1, x2, y1, y2;
  gint retval = FALSE;

  w = BTK_WIDGET (c);
  width = w->allocation.width - RADIUS * 2;
  height = w->allocation.height - RADIUS * 2;

  if ((width < 0) || (height < 0))
    return FALSE;

  /*  get the pointer position  */
  bdk_window_get_pointer (w->window, &tx, &ty, NULL);
  x = CLAMP ((tx - RADIUS), 0, width-1);
  y = CLAMP ((ty - RADIUS), 0, height-1);

  min_x = c->min_x;

  distance = ~0U;
  for (i = 0; i < c->num_ctlpoints; ++i)
    {
      cx = project (c->ctlpoint[i][0], min_x, c->max_x, width);
      if ((guint) abs (x - cx) < distance)
	{
	  distance = abs (x - cx);
	  closest_point = i;
	}
    }

  switch (event->type)
    {
    case BDK_CONFIGURE:
      if (c->pixmap)
	g_object_unref (c->pixmap);
      c->pixmap = NULL;
      /* fall through */
    case BDK_EXPOSE:
      if (!c->pixmap)
	c->pixmap = bdk_pixmap_new (w->window,
				    w->allocation.width,
				    w->allocation.height, -1);
      btk_curve_draw (c, width, height);
      break;

    case BDK_BUTTON_PRESS:
      btk_grab_add (widget);

      new_type = BDK_TCROSS;

      switch (c->curve_type)
	{
	case BTK_CURVE_TYPE_LINEAR:
	case BTK_CURVE_TYPE_SPLINE:
	  if (distance > MIN_DISTANCE)
	    {
	      /* insert a new control point */
	      if (c->num_ctlpoints > 0)
		{
		  cx = project (c->ctlpoint[closest_point][0], min_x,
				c->max_x, width);
		  if (x > cx)
		    ++closest_point;
		}
	      ++c->num_ctlpoints;
	      c->ctlpoint =
		g_realloc (c->ctlpoint,
			   c->num_ctlpoints * sizeof (*c->ctlpoint));
	      for (i = c->num_ctlpoints - 1; i > closest_point; --i)
		memcpy (c->ctlpoint + i, c->ctlpoint + i - 1,
			sizeof (*c->ctlpoint));
	    }
	  c->grab_point = closest_point;
	  c->ctlpoint[c->grab_point][0] =
	    unproject (x, min_x, c->max_x, width);
	  c->ctlpoint[c->grab_point][1] =
	    unproject (height - y, c->min_y, c->max_y, height);

	  btk_curve_interpolate (c, width, height);
	  break;

	case BTK_CURVE_TYPE_FREE:
	  c->point[x].x = RADIUS + x;
	  c->point[x].y = RADIUS + y;
	  c->grab_point = x;
	  c->last = y;
	  break;
	}
      btk_curve_draw (c, width, height);
      retval = TRUE;
      break;

    case BDK_BUTTON_RELEASE:
      btk_grab_remove (widget);

      /* delete inactive points: */
      if (c->curve_type != BTK_CURVE_TYPE_FREE)
	{
	  for (src = dst = 0; src < c->num_ctlpoints; ++src)
	    {
	      if (c->ctlpoint[src][0] >= min_x)
		{
		  memcpy (c->ctlpoint + dst, c->ctlpoint + src,
			  sizeof (*c->ctlpoint));
		  ++dst;
		}
	    }
	  if (dst < src)
	    {
	      c->num_ctlpoints -= (src - dst);
	      if (c->num_ctlpoints <= 0)
		{
		  c->num_ctlpoints = 1;
		  c->ctlpoint[0][0] = min_x;
		  c->ctlpoint[0][1] = c->min_y;
		  btk_curve_interpolate (c, width, height);
		  btk_curve_draw (c, width, height);
		}
	      c->ctlpoint =
		g_realloc (c->ctlpoint,
			   c->num_ctlpoints * sizeof (*c->ctlpoint));
	    }
	}
      new_type = BDK_FLEUR;
      c->grab_point = -1;
      retval = TRUE;
      break;

    case BDK_MOTION_NOTIFY:
      mevent = (BdkEventMotion *) event;

      switch (c->curve_type)
	{
	case BTK_CURVE_TYPE_LINEAR:
	case BTK_CURVE_TYPE_SPLINE:
	  if (c->grab_point == -1)
	    {
	      /* if no point is grabbed...  */
	      if (distance <= MIN_DISTANCE)
		new_type = BDK_FLEUR;
	      else
		new_type = BDK_TCROSS;
	    }
	  else
	    {
	      /* drag the grabbed point  */
	      new_type = BDK_TCROSS;

	      leftbound = -MIN_DISTANCE;
	      if (c->grab_point > 0)
		leftbound = project (c->ctlpoint[c->grab_point - 1][0],
				     min_x, c->max_x, width);

	      rightbound = width + RADIUS * 2 + MIN_DISTANCE;
	      if (c->grab_point + 1 < c->num_ctlpoints)
		rightbound = project (c->ctlpoint[c->grab_point + 1][0],
				      min_x, c->max_x, width);

	      if (tx <= leftbound || tx >= rightbound
		  || ty > height + RADIUS * 2 + MIN_DISTANCE
		  || ty < -MIN_DISTANCE)
		c->ctlpoint[c->grab_point][0] = min_x - 1.0;
	      else
		{
		  rx = unproject (x, min_x, c->max_x, width);
		  ry = unproject (height - y, c->min_y, c->max_y, height);
		  c->ctlpoint[c->grab_point][0] = rx;
		  c->ctlpoint[c->grab_point][1] = ry;
		}
	      btk_curve_interpolate (c, width, height);
	      btk_curve_draw (c, width, height);
	    }
	  break;

	case BTK_CURVE_TYPE_FREE:
	  if (c->grab_point != -1)
	    {
	      if (c->grab_point > x)
		{
		  x1 = x;
		  x2 = c->grab_point;
		  y1 = y;
		  y2 = c->last;
		}
	      else
		{
		  x1 = c->grab_point;
		  x2 = x;
		  y1 = c->last;
		  y2 = y;
		}

	      if (x2 != x1)
		for (i = x1; i <= x2; i++)
		  {
		    c->point[i].x = RADIUS + i;
		    c->point[i].y = RADIUS +
		      (y1 + ((y2 - y1) * (i - x1)) / (x2 - x1));
		  }
	      else
		{
		  c->point[x].x = RADIUS + x;
		  c->point[x].y = RADIUS + y;
		}
	      c->grab_point = x;
	      c->last = y;
	      btk_curve_draw (c, width, height);
	    }
	  if (mevent->state & BDK_BUTTON1_MASK)
	    new_type = BDK_TCROSS;
	  else
	    new_type = BDK_PENCIL;
	  break;
	}
      if (new_type != (BdkCursorType) c->cursor_type)
	{
	  BdkCursor *cursor;

	  c->cursor_type = new_type;

	  cursor = bdk_cursor_new_for_display (btk_widget_get_display (w),
					      c->cursor_type);
	  bdk_window_set_cursor (w->window, cursor);
	  bdk_cursor_unref (cursor);
	}
      retval = TRUE;
      break;

    default:
      break;
    }

  return retval;
}

void
btk_curve_set_curve_type (BtkCurve *c, BtkCurveType new_type)
{
  gfloat rx, dx;
  gint x, i;

  if (new_type != c->curve_type)
    {
      gint width, height;

      width  = BTK_WIDGET (c)->allocation.width - RADIUS * 2;
      height = BTK_WIDGET (c)->allocation.height - RADIUS * 2;

      if (new_type == BTK_CURVE_TYPE_FREE)
	{
	  btk_curve_interpolate (c, width, height);
	  c->curve_type = new_type;
	}
      else if (c->curve_type == BTK_CURVE_TYPE_FREE)
	{
	  g_free (c->ctlpoint);
	  c->num_ctlpoints = 9;
	  c->ctlpoint = g_malloc (c->num_ctlpoints * sizeof (*c->ctlpoint));

	  rx = 0.0;
	  dx = (width - 1) / (gfloat) (c->num_ctlpoints - 1);

	  for (i = 0; i < c->num_ctlpoints; ++i, rx += dx)
	    {
	      x = (int) (rx + 0.5);
	      c->ctlpoint[i][0] =
		unproject (x, c->min_x, c->max_x, width);
	      c->ctlpoint[i][1] =
		unproject (RADIUS + height - c->point[x].y,
			   c->min_y, c->max_y, height);
	    }
	  c->curve_type = new_type;
	  btk_curve_interpolate (c, width, height);
	}
      else
	{
	  c->curve_type = new_type;
	  btk_curve_interpolate (c, width, height);
	}
      g_signal_emit (c, curve_type_changed_signal, 0);
      g_object_notify (G_OBJECT (c), "curve-type");
      btk_curve_draw (c, width, height);
    }
}

static void
btk_curve_size_graph (BtkCurve *curve)
{
  gint width, height;
  gfloat aspect;
  BdkScreen *screen = btk_widget_get_screen (BTK_WIDGET (curve)); 

  width  = (curve->max_x - curve->min_x) + 1;
  height = (curve->max_y - curve->min_y) + 1;
  aspect = width / (gfloat) height;
  if (width > bdk_screen_get_width (screen) / 4)
    width  = bdk_screen_get_width (screen) / 4;
  if (height > bdk_screen_get_height (screen) / 4)
    height = bdk_screen_get_height (screen) / 4;

  if (aspect < 1.0)
    width  = height * aspect;
  else
    height = width / aspect;

  btk_widget_set_size_request (BTK_WIDGET (curve),
			       width + RADIUS * 2,
			       height + RADIUS * 2);
}

static void
btk_curve_reset_vector (BtkCurve *curve)
{
  g_free (curve->ctlpoint);

  curve->num_ctlpoints = 2;
  curve->ctlpoint = g_malloc (2 * sizeof (curve->ctlpoint[0]));
  curve->ctlpoint[0][0] = curve->min_x;
  curve->ctlpoint[0][1] = curve->min_y;
  curve->ctlpoint[1][0] = curve->max_x;
  curve->ctlpoint[1][1] = curve->max_y;

  if (curve->pixmap)
    {
      gint width, height;

      width = BTK_WIDGET (curve)->allocation.width - RADIUS * 2;
      height = BTK_WIDGET (curve)->allocation.height - RADIUS * 2;

      if (curve->curve_type == BTK_CURVE_TYPE_FREE)
	{
	  curve->curve_type = BTK_CURVE_TYPE_LINEAR;
	  btk_curve_interpolate (curve, width, height);
	  curve->curve_type = BTK_CURVE_TYPE_FREE;
	}
      else
	btk_curve_interpolate (curve, width, height);
      btk_curve_draw (curve, width, height);
    }
}

void
btk_curve_reset (BtkCurve *c)
{
  BtkCurveType old_type;

  old_type = c->curve_type;
  c->curve_type = BTK_CURVE_TYPE_SPLINE;
  btk_curve_reset_vector (c);

  if (old_type != BTK_CURVE_TYPE_SPLINE)
    {
       g_signal_emit (c, curve_type_changed_signal, 0);
       g_object_notify (G_OBJECT (c), "curve-type");
    }
}

void
btk_curve_set_gamma (BtkCurve *c, gfloat gamma)
{
  gfloat x, one_over_gamma, height;
  BtkCurveType old_type;
  gint i;

  if (c->num_points < 2)
    return;

  old_type = c->curve_type;
  c->curve_type = BTK_CURVE_TYPE_FREE;

  if (gamma <= 0)
    one_over_gamma = 1.0;
  else
    one_over_gamma = 1.0 / gamma;
  height = c->height;
  for (i = 0; i < c->num_points; ++i)
    {
      x = (gfloat) i / (c->num_points - 1);
      c->point[i].x = RADIUS + i;
      c->point[i].y =
	RADIUS + (height * (1.0 - pow (x, one_over_gamma)) + 0.5);
    }

  if (old_type != BTK_CURVE_TYPE_FREE)
    g_signal_emit (c, curve_type_changed_signal, 0);

  btk_curve_draw (c, c->num_points, c->height);
}

void
btk_curve_set_range (BtkCurve *curve,
		     gfloat    min_x,
                     gfloat    max_x,
                     gfloat    min_y,
                     gfloat    max_y)
{
  g_object_freeze_notify (G_OBJECT (curve));
  if (curve->min_x != min_x) {
     curve->min_x = min_x;
     g_object_notify (G_OBJECT (curve), "min-x");
  }
  if (curve->max_x != max_x) {
     curve->max_x = max_x;
     g_object_notify (G_OBJECT (curve), "max-x");
  }
  if (curve->min_y != min_y) {
     curve->min_y = min_y;
     g_object_notify (G_OBJECT (curve), "min-y");
  }
  if (curve->max_y != max_y) {
     curve->max_y = max_y;
     g_object_notify (G_OBJECT (curve), "max-y");
  }
  g_object_thaw_notify (G_OBJECT (curve));

  btk_curve_size_graph (curve);
  btk_curve_reset_vector (curve);
}

void
btk_curve_set_vector (BtkCurve *c, int veclen, gfloat vector[])
{
  BtkCurveType old_type;
  gfloat rx, dx, ry;
  gint i, height;
  BdkScreen *screen = btk_widget_get_screen (BTK_WIDGET (c));

  old_type = c->curve_type;
  c->curve_type = BTK_CURVE_TYPE_FREE;

  if (c->point)
    height = BTK_WIDGET (c)->allocation.height - RADIUS * 2;
  else
    {
      height = (c->max_y - c->min_y);
      if (height > bdk_screen_get_height (screen) / 4)
	height = bdk_screen_get_height (screen) / 4;

      c->height = height;
      c->num_points = veclen;
      c->point = g_malloc (c->num_points * sizeof (c->point[0]));
    }
  rx = 0;
  dx = (veclen - 1.0) / (c->num_points - 1.0);

  for (i = 0; i < c->num_points; ++i, rx += dx)
    {
      ry = vector[(int) (rx + 0.5)];
      if (ry > c->max_y) ry = c->max_y;
      if (ry < c->min_y) ry = c->min_y;
      c->point[i].x = RADIUS + i;
      c->point[i].y =
	RADIUS + height - project (ry, c->min_y, c->max_y, height);
    }
  if (old_type != BTK_CURVE_TYPE_FREE)
    {
       g_signal_emit (c, curve_type_changed_signal, 0);
       g_object_notify (G_OBJECT (c), "curve-type");
    }

  btk_curve_draw (c, c->num_points, height);
}

void
btk_curve_get_vector (BtkCurve *c, int veclen, gfloat vector[])
{
  gfloat rx, ry, dx, dy, min_x, delta_x, *mem, *xv, *yv, *y2v, prev;
  gint dst, i, x, next, num_active_ctlpoints = 0, first_active = -1;

  min_x = c->min_x;

  if (c->curve_type != BTK_CURVE_TYPE_FREE)
    {
      /* count active points: */
      prev = min_x - 1.0;
      for (i = num_active_ctlpoints = 0; i < c->num_ctlpoints; ++i)
	if (c->ctlpoint[i][0] > prev)
	  {
	    if (first_active < 0)
	      first_active = i;
	    prev = c->ctlpoint[i][0];
	    ++num_active_ctlpoints;
	  }

      /* handle degenerate case: */
      if (num_active_ctlpoints < 2)
	{
	  if (num_active_ctlpoints > 0)
	    ry = c->ctlpoint[first_active][1];
	  else
	    ry = c->min_y;
	  if (ry < c->min_y) ry = c->min_y;
	  if (ry > c->max_y) ry = c->max_y;
	  for (x = 0; x < veclen; ++x)
	    vector[x] = ry;
	  return;
	}
    }

  switch (c->curve_type)
    {
    case BTK_CURVE_TYPE_SPLINE:
      mem = g_malloc (3 * num_active_ctlpoints * sizeof (gfloat));
      xv  = mem;
      yv  = mem + num_active_ctlpoints;
      y2v = mem + 2*num_active_ctlpoints;

      prev = min_x - 1.0;
      for (i = dst = 0; i < c->num_ctlpoints; ++i)
	if (c->ctlpoint[i][0] > prev)
	  {
	    prev    = c->ctlpoint[i][0];
	    xv[dst] = c->ctlpoint[i][0];
	    yv[dst] = c->ctlpoint[i][1];
	    ++dst;
	  }

      spline_solve (num_active_ctlpoints, xv, yv, y2v);

      rx = min_x;
      dx = (c->max_x - min_x) / (veclen - 1);
      for (x = 0; x < veclen; ++x, rx += dx)
	{
	  ry = spline_eval (num_active_ctlpoints, xv, yv, y2v, rx);
	  if (ry < c->min_y) ry = c->min_y;
	  if (ry > c->max_y) ry = c->max_y;
	  vector[x] = ry;
	}

      g_free (mem);
      break;

    case BTK_CURVE_TYPE_LINEAR:
      dx = (c->max_x - min_x) / (veclen - 1);
      rx = min_x;
      ry = c->min_y;
      dy = 0.0;
      i  = first_active;
      for (x = 0; x < veclen; ++x, rx += dx)
	{
	  if (rx >= c->ctlpoint[i][0])
	    {
	      if (rx > c->ctlpoint[i][0])
		ry = c->min_y;
	      dy = 0.0;
	      next = i + 1;
	      while (next < c->num_ctlpoints
		     && c->ctlpoint[next][0] <= c->ctlpoint[i][0])
		++next;
	      if (next < c->num_ctlpoints)
		{
		  delta_x = c->ctlpoint[next][0] - c->ctlpoint[i][0];
		  dy = ((c->ctlpoint[next][1] - c->ctlpoint[i][1])
			/ delta_x);
		  dy *= dx;
		  ry = c->ctlpoint[i][1];
		  i = next;
		}
	    }
	  vector[x] = ry;
	  ry += dy;
	}
      break;

    case BTK_CURVE_TYPE_FREE:
      if (c->point)
	{
	  rx = 0.0;
	  dx = c->num_points / (double) veclen;
	  for (x = 0; x < veclen; ++x, rx += dx)
	    vector[x] = unproject (RADIUS + c->height - c->point[(int) rx].y,
				   c->min_y, c->max_y,
				   c->height);
	}
      else
	memset (vector, 0, veclen * sizeof (vector[0]));
      break;
    }
}

BtkWidget*
btk_curve_new (void)
{
  return g_object_new (BTK_TYPE_CURVE, NULL);
}

static void
btk_curve_finalize (GObject *object)
{
  BtkCurve *curve;

  g_return_if_fail (BTK_IS_CURVE (object));

  curve = BTK_CURVE (object);
  if (curve->pixmap)
    g_object_unref (curve->pixmap);
  g_free (curve->point);
  g_free (curve->ctlpoint);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

#define __BTK_CURVE_C__
#include "btkaliasdef.c"
