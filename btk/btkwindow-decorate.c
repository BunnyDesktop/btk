/* BTK - The GIMP Toolkit
 * Copyright (C) 2001 Red Hat, Inc.
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
 * Authors: Alexander Larsson <alexl@redhat.com>
 */

#include "config.h"
#include "btkprivate.h"
#include "btkwindow.h"
#include "btkmain.h"
#include "btkwindow-decorate.h"
#include "btkintl.h"
#include "btkalias.h"


#ifdef DECORATE_WINDOWS

typedef enum
{
  BTK_WINDOW_REBUNNYION_TITLE,
  BTK_WINDOW_REBUNNYION_MAXIMIZE,
  BTK_WINDOW_REBUNNYION_CLOSE,
  BTK_WINDOW_REBUNNYION_BR_RESIZE
} BtkWindowRebunnyionType;

typedef struct _BtkWindowRebunnyion BtkWindowRebunnyion;
typedef struct _BtkWindowDecoration BtkWindowDecoration;

struct _BtkWindowRebunnyion
{
  BdkRectangle rect;
  BtkWindowRebunnyionType type;
};

typedef enum
{
  RESIZE_TOP_LEFT,
  RESIZE_TOP,
  RESIZE_TOP_RIGHT,
  RESIZE_RIGHT,
  RESIZE_BOTTOM_RIGHT,
  RESIZE_BOTTOM,
  RESIZE_BOTTOM_LEFT,
  RESIZE_LEFT,
  RESIZE_NONE,
} BtkWindowResizeType;

struct _BtkWindowDecoration
{
  gint n_rebunnyions;
  BtkWindowRebunnyion *rebunnyions;

  gint last_x, last_y;
  gint last_w, last_h;
  
  BangoLayout *title_layout;

  BtkWindowResizeType resize;
  
  guint moving : 1;
  guint closing : 1;
  guint maximizing : 1;
  guint maximized : 1;
  guint maximizable : 1;
  guint decorated : 1;
  guint real_inner_move : 1;
  guint focused : 1;
};

#define DECORATION_BORDER_TOP 15
#define DECORATION_BORDER_LEFT 3
#define DECORATION_BORDER_RIGHT 3
#define DECORATION_BORDER_BOTTOM 3
#define DECORATION_BORDER_TOT_X (DECORATION_BORDER_LEFT + DECORATION_BORDER_RIGHT)
#define DECORATION_BORDER_TOT_Y (DECORATION_BORDER_TOP + DECORATION_BORDER_BOTTOM)
#define DECORATION_BUTTON_SIZE 9
#define DECORATION_BUTTON_Y_OFFSET 2
#define DECORATION_TITLE_FONT "Sans 9"

static void btk_decorated_window_recalculate_rebunnyions      (BtkWindow      *window);
static BtkWindowRebunnyionType btk_decorated_window_rebunnyion_type    (BtkWindow      *window,
						 gint            x,
						 gint            y);
static gint btk_decorated_window_frame_event    (BtkWindow *window,
						 BdkEvent *event);
static gint btk_decorated_window_button_press   (BtkWidget      *widget,
						 BdkEventButton *event);
static gint btk_decorated_window_button_release (BtkWidget      *widget,
						 BdkEventButton *event);
static gint btk_decorated_window_motion_notify  (BtkWidget      *widget,
						 BdkEventMotion *event);
static gint btk_decorated_window_window_state   (BtkWidget           *widget,
						 BdkEventWindowState *event);
static void btk_decorated_window_paint          (BtkWidget      *widget,
						 BdkRectangle   *area);
static gint btk_decorated_window_focus_change   (BtkWidget         *widget,
						 BdkEventFocus     *event);
static void btk_decorated_window_realize        (BtkWindow   *window);
static void btk_decorated_window_unrealize      (BtkWindow   *window);

static void
btk_decoration_free (BtkWindowDecoration *deco)
{
  g_free (deco->rebunnyions);
  deco->rebunnyions = NULL;
  deco->n_rebunnyions = 0;

  g_free (deco);
}

void
btk_decorated_window_init (BtkWindow   *window)
{
  BtkWindowDecoration *deco;

  deco = g_new (BtkWindowDecoration, 1);

  deco->n_rebunnyions = 0;
  deco->rebunnyions = NULL;
  deco->title_layout = NULL;
  deco->resize = RESIZE_NONE;
  deco->moving = FALSE;
  deco->decorated = TRUE;
  deco->closing = FALSE;
  deco->maximizing = FALSE;
  deco->maximized = FALSE;
  deco->maximizable = FALSE;
  deco->real_inner_move = FALSE;
 
  g_object_set_data_full (B_OBJECT (window), I_("btk-window-decoration"), deco,
			  (GDestroyNotify) btk_decoration_free);
  
  btk_window_set_has_frame (window, TRUE);

  g_signal_connect (window,
		    "frame-event",
		    G_CALLBACK (btk_decorated_window_frame_event),
		    window);
  g_signal_connect (window,
		    "focus-in-event",
		    G_CALLBACK (btk_decorated_window_focus_change),
		    window);
  g_signal_connect (window,
		    "focus-out-event",
		    G_CALLBACK (btk_decorated_window_focus_change),
		    window);
  g_signal_connect (window,
		    "realize",
		    G_CALLBACK (btk_decorated_window_realize),
		    window);
  g_signal_connect (window,
		    "unrealize",
		    G_CALLBACK (btk_decorated_window_unrealize),
		    window);
}

static inline BtkWindowDecoration *
get_decoration (BtkWindow *window)
{
  return (BtkWindowDecoration *)g_object_get_data (B_OBJECT (window), "btk-window-decoration");
}

void
btk_decorated_window_set_title (BtkWindow   *window,
				const gchar *title)
{
  BtkWindowDecoration *deco = get_decoration (window);
  
  if (deco->title_layout)
    bango_layout_set_text (deco->title_layout, title, -1);
}

void 
btk_decorated_window_calculate_frame_size (BtkWindow *window)
{
  BdkWMDecoration decorations;
  BtkWindowDecoration *deco = get_decoration (window);
  
  if (bdk_window_get_decorations (BTK_WIDGET (window)->window,
				  &decorations))
    {
      if ((decorations & BDK_DECOR_BORDER) &&
	  (decorations & BDK_DECOR_TITLE))
	{
	  deco->decorated = TRUE;
	  if ((decorations & BDK_DECOR_MAXIMIZE) &&
	      (btk_window_get_type_hint (window) == BDK_WINDOW_TYPE_HINT_NORMAL))
	    deco->maximizable = TRUE;
	}
      else
	deco->decorated = FALSE;
    }
  else
    {
      deco->decorated = (window->type != BTK_WINDOW_POPUP);
      deco->maximizable = (btk_window_get_type_hint (window) == BDK_WINDOW_TYPE_HINT_NORMAL);
    }

  if (deco->decorated)
    btk_window_set_frame_dimensions (window,
				     DECORATION_BORDER_LEFT,
				     DECORATION_BORDER_TOP,
				     DECORATION_BORDER_RIGHT,
				     DECORATION_BORDER_BOTTOM);
  else
    btk_window_set_frame_dimensions (window, 0, 0, 0, 0);

  btk_decorated_window_recalculate_rebunnyions (window);
}

static gboolean
btk_decorated_window_inner_change (BdkWindow *win,
				   gint x, gint y,
				   gint width, gint height,
				   gpointer user_data)
{
  BtkWindow *window = (BtkWindow *)user_data;
  BtkWidget *widget = BTK_WIDGET (window);
  BtkWindowDecoration *deco = get_decoration (window);

  if (deco->real_inner_move)
    {
      deco->real_inner_move = FALSE;
      return FALSE;
    }

  deco->real_inner_move = TRUE;
  bdk_window_move_resize (widget->window,
			  window->frame_left, window->frame_top,
			  width, height);

  bdk_window_move_resize (window->frame,
			  x - window->frame_left, y - window->frame_top,
			  width + window->frame_left + window->frame_right,
			  height + window->frame_top + window->frame_bottom);
  return TRUE;
}

static void
btk_decorated_window_inner_get_pos (BdkWindow *win,
				    gint *x, gint *y,
				    gpointer user_data)
{
  BtkWindow *window = (BtkWindow *)user_data;

  bdk_window_get_position (window->frame, x, y);
  
  *x += window->frame_left;
  *y += window->frame_top;
}

static void
btk_decorated_window_realize (BtkWindow   *window)
{
  BtkWindowDecoration *deco = get_decoration (window);
  BtkWidget *widget = BTK_WIDGET (window);
  BangoFontDescription *font_desc;

  deco->title_layout = btk_widget_create_bango_layout (widget,
						       (window->title)?window->title:"");

  font_desc = bango_font_description_from_string(DECORATION_TITLE_FONT);
  bango_layout_set_font_description (deco->title_layout, font_desc);
  bango_font_description_free (font_desc);

#if 0
  /* What is this code exactly doing? I remember we were using the
     decorated windows with the DirectFB port and it did just work,
     and there was definitely no code in linux-fb involved. */
  bdk_fb_window_set_child_handler (window->frame,
				   btk_decorated_window_inner_change,
				   btk_decorated_window_inner_get_pos,
				   window);

  /* This is a huge hack to make frames have the same shape as
     the window they wrap */
  bdk_window_shape_combine_mask (window->frame, BDK_FB_USE_CHILD_SHAPE, 0, 0);
#endif
}


static void
btk_decorated_window_unrealize (BtkWindow   *window)
{
  BtkWindowDecoration *deco = get_decoration (window);

  if (deco->title_layout)
    {
      g_object_unref (deco->title_layout);
      deco->title_layout = NULL;
    }
}

static gint
btk_decorated_window_frame_event (BtkWindow *window, BdkEvent *event)
{
  BtkWindowDecoration *deco = get_decoration (window);
  BtkWidget *widget = BTK_WIDGET (window);
  BdkEventExpose *expose_event;

  switch (event->type)
    {
    case BDK_EXPOSE:
      expose_event = (BdkEventExpose *)event;
      if (deco->decorated)
	btk_decorated_window_paint (widget, &expose_event->area);
      return TRUE;
      break;
    case BDK_CONFIGURE:
      btk_decorated_window_recalculate_rebunnyions (window);
      break;
    case BDK_MOTION_NOTIFY:
      return btk_decorated_window_motion_notify (widget, (BdkEventMotion *)event);
      break;
    case BDK_BUTTON_PRESS:
      return btk_decorated_window_button_press (widget, (BdkEventButton *)event);
      break;
    case BDK_BUTTON_RELEASE:
      return btk_decorated_window_button_release (widget, (BdkEventButton *)event);
    case BDK_WINDOW_STATE:
      return btk_decorated_window_window_state (widget, (BdkEventWindowState *)event);
    default:
      break;
    }
  return FALSE;
}

static gint
btk_decorated_window_focus_change (BtkWidget         *widget,
				   BdkEventFocus     *event)
{
  BtkWindow *window = BTK_WINDOW(widget);
  BtkWindowDecoration *deco = get_decoration (window);
  deco->focused = event->in;
  bdk_window_invalidate_rect (window->frame, NULL, FALSE);
  return FALSE;
}

static gint
btk_decorated_window_motion_notify (BtkWidget       *widget,
				    BdkEventMotion  *event)
{
  BtkWindow *window;
  BtkWindowDecoration *deco;
  BdkModifierType mask;
  BdkWindow *win;
  gint x, y;
  gint win_x, win_y, win_w, win_h;
  
  window = BTK_WINDOW (widget);
  deco = get_decoration (window);
  
  if (!deco->decorated)
    return TRUE;
  
  win = widget->window;
  bdk_window_get_pointer (window->frame, &x, &y, &mask);
  
  bdk_window_get_position (window->frame, &win_x, &win_y);
  win_x += DECORATION_BORDER_LEFT;
  win_y += DECORATION_BORDER_TOP;
  
  bdk_window_get_geometry (win, NULL, NULL, &win_w, &win_h, NULL);

  if (deco->moving)
    {
      int dx, dy;
      dx = x - deco->last_x;
      dy = y - deco->last_y;

      _btk_window_reposition (window, win_x + dx, win_y + dy);
    }

  if (deco->resize != RESIZE_NONE)
    {
      int w, h;
      
      w = win_w;
      h = win_h;
      
      switch(deco->resize) {
      case RESIZE_BOTTOM_RIGHT:
	w = x - DECORATION_BORDER_TOT_X;
	h = y - DECORATION_BORDER_TOT_Y;
	break;
      case RESIZE_RIGHT:
	w = x - DECORATION_BORDER_TOT_X;
	break;
      case RESIZE_BOTTOM:
	h = y - DECORATION_BORDER_TOT_Y;
	break;
      case RESIZE_TOP_LEFT:
      case RESIZE_TOP:
      case RESIZE_TOP_RIGHT:
      case RESIZE_BOTTOM_LEFT:
      case RESIZE_LEFT:
      default:
	g_warning ("Resize mode %d not handled yet.\n", deco->resize);
	break;
      }
      
      if ((w > 0) && (h > 0))
	{
	  _btk_window_constrain_size (window, w,h, &w, &h);
	  
	  if ((w != win_w) || (h != win_h))
	    bdk_window_resize (widget->window, w, h);
	}
    }

  return TRUE;
}

static BtkWindowRebunnyionType
btk_decorated_window_rebunnyion_type (BtkWindow *window, gint x, gint y)
{
  BtkWindowDecoration *deco = get_decoration (window);
  int i;

  for (i=0;i<deco->n_rebunnyions;i++)
    {
      if ((x > deco->rebunnyions[i].rect.x) &&
	  (x - deco->rebunnyions[i].rect.x < deco->rebunnyions[i].rect.width) &&
	  (y > deco->rebunnyions[i].rect.y) &&
	  (y - deco->rebunnyions[i].rect.y < deco->rebunnyions[i].rect.height))
	return deco->rebunnyions[i].type;
    }
  return -1;
}

static gint
btk_decorated_window_button_press (BtkWidget       *widget,
				   BdkEventButton  *event)
{
  BtkWindow *window;
  BtkWindowRebunnyionType type;
  BtkWindowDecoration *deco;
  gint x, y; 

  window = BTK_WINDOW (widget);
  deco = get_decoration (window);

  if (!deco->decorated)
    return TRUE;

  x = event->x;
  y = event->y;
  
  type = btk_decorated_window_rebunnyion_type (window, x, y);

  switch (type)
    {
    case BTK_WINDOW_REBUNNYION_TITLE:
      if (!deco->maximized && event->state & BDK_BUTTON1_MASK)
	{
	  deco->last_x = x;
	  deco->last_y = y;
	  deco->moving = TRUE;
	}
      break;
    case BTK_WINDOW_REBUNNYION_MAXIMIZE:
      if (event->state & BDK_BUTTON1_MASK)
	deco->maximizing = TRUE;
      break;
    case BTK_WINDOW_REBUNNYION_CLOSE:
      if (event->state & BDK_BUTTON1_MASK)
	deco->closing = TRUE;
      break;
    case BTK_WINDOW_REBUNNYION_BR_RESIZE:
      if (!deco->maximized)
	{
	  if (event->state & BDK_BUTTON1_MASK)
	    deco->resize = RESIZE_BOTTOM_RIGHT;
	  deco->last_x = x;
	  deco->last_y = y;
	}
      break;
    default:
      break;
    }
  
  return TRUE;
}

static gint
btk_decorated_window_button_release (BtkWidget	    *widget,
				     BdkEventButton *event)
{
  BtkWindow *window;
  BtkWindowRebunnyionType type;
  BtkWindowDecoration *deco;
      
  window = BTK_WINDOW (widget);
  deco = get_decoration (window);

  if (deco->closing)
    {
      type = btk_decorated_window_rebunnyion_type (window, event->x, event->y);
      if (type == BTK_WINDOW_REBUNNYION_CLOSE)
	{
	  BdkEvent *event = bdk_event_new (BDK_DELETE);

	  event->any.type = BDK_DELETE;
	  event->any.window = g_object_ref (widget->window);
	  event->any.send_event = TRUE;

	  btk_main_do_event (event);
	  bdk_event_free (event);
	}
    }
  else if (deco->maximizing)
    {
      type = btk_decorated_window_rebunnyion_type (window, event->x, event->y);
      if (type == BTK_WINDOW_REBUNNYION_MAXIMIZE)
        {
	  if (deco->maximized)
	    btk_window_unmaximize (window);
	  else
	    btk_window_maximize (window);
	}
    }
  
  deco->closing = FALSE;
  deco->maximizing = FALSE;
  deco->moving = FALSE;
  deco->resize = RESIZE_NONE;
  return TRUE;
}

static gint
btk_decorated_window_window_state (BtkWidget	       *widget,
				   BdkEventWindowState *event)
{
  BtkWindow *window;
  BtkWindowDecoration *deco;
  BdkWindowObject *priv;
      
  window = BTK_WINDOW (widget);
  deco = get_decoration (window);
  priv = BDK_WINDOW_OBJECT (window->frame);

  if (event->changed_mask & BDK_WINDOW_STATE_MAXIMIZED)
    {
      if (event->new_window_state & BDK_WINDOW_STATE_MAXIMIZED)
	{
	  int w, h;
	  bdk_window_get_geometry (widget->window, NULL, NULL,
				   &deco->last_w, &deco->last_h, NULL);
	  bdk_window_get_origin (widget->window, &deco->last_x, &deco->last_y);
	  w = bdk_screen_get_width(bdk_screen_get_default()) - DECORATION_BORDER_TOT_X;
	  h = bdk_screen_get_height(bdk_screen_get_default()) - DECORATION_BORDER_TOT_Y;
	  _btk_window_constrain_size (window, w, h, &w, &h);
	  if (w != deco->last_w || h != deco->last_h)
	    {
	      _btk_window_reposition (window, DECORATION_BORDER_LEFT, DECORATION_BORDER_TOP);
	      bdk_window_resize (widget->window, w, h);
	      deco->maximized = TRUE;
	    }
	}
      else
	{
	  _btk_window_reposition (window, deco->last_x, deco->last_y);
	  _btk_window_constrain_size (window, deco->last_w, deco->last_h,
				      &deco->last_w, &deco->last_h);
	  bdk_window_resize (widget->window, deco->last_w, deco->last_h);
	  deco->maximized = FALSE;
	}
    }
  return TRUE;
}

static void
btk_decorated_window_paint (BtkWidget    *widget,
			    BdkRectangle *area)
{
  BtkWindow *window = BTK_WINDOW (widget);
  BtkWindowDecoration *deco = get_decoration (window);
  gint x1, y1, x2, y2;
  BtkStateType border_state;

  if (deco->decorated)
    {
      BdkWindow *frame;
      gint width, height;

      frame = window->frame;
      bdk_drawable_get_size (frame, &width, &height);

      /* Top */
      btk_paint_flat_box (widget->style, frame, BTK_STATE_NORMAL,
			  BTK_SHADOW_NONE, area, widget, "base",
			  0, 0,
			  width, DECORATION_BORDER_TOP);
      /* Bottom */
      btk_paint_flat_box (widget->style, frame, BTK_STATE_NORMAL,
			  BTK_SHADOW_NONE, area, widget, "base",
			  0, height - DECORATION_BORDER_BOTTOM,
			  width, DECORATION_BORDER_BOTTOM);
      /* Left */
      btk_paint_flat_box (widget->style, frame, BTK_STATE_NORMAL,
			  BTK_SHADOW_NONE, area, widget, "base",
			  0, DECORATION_BORDER_TOP,
			  DECORATION_BORDER_LEFT, height - DECORATION_BORDER_TOT_Y);
      /* Right */
      btk_paint_flat_box (widget->style, frame, BTK_STATE_NORMAL,
			  BTK_SHADOW_NONE, area, widget, "base",
			  width - DECORATION_BORDER_RIGHT, DECORATION_BORDER_TOP,
			  DECORATION_BORDER_RIGHT, height - DECORATION_BORDER_TOT_Y);
      
      /* Border: */
      if (deco->focused)
	border_state = BTK_STATE_SELECTED;
      else 
	border_state = BTK_STATE_PRELIGHT;

      btk_paint_box (widget->style, frame, border_state, 
		     BTK_SHADOW_OUT, area, widget, "base",
		     0, 0, width, height);
      
      btk_paint_box (widget->style, frame, border_state, 
		     BTK_SHADOW_IN, area, widget, "base",
		     DECORATION_BORDER_LEFT - 2, DECORATION_BORDER_TOP - 2,
		     width - (DECORATION_BORDER_LEFT + DECORATION_BORDER_RIGHT) + 3,
		     height - (DECORATION_BORDER_TOP + DECORATION_BORDER_BOTTOM) + 3);

      if (deco->maximizable)
	{
	  /* Maximize button: */

	  x1 = width - (DECORATION_BORDER_LEFT * 2) - (DECORATION_BUTTON_SIZE * 2);
	  y1 = DECORATION_BUTTON_Y_OFFSET;
	  x2 = x1 + DECORATION_BUTTON_SIZE;
	  y2 = y1 + DECORATION_BUTTON_SIZE;

	  if (area)
	    bdk_gc_set_clip_rectangle (widget->style->bg_gc[widget->state], area);

	  bdk_draw_rectangle (frame, widget->style->bg_gc[widget->state], TRUE,
			      x1, y1, x2 - x1, y2 - y1);

	  bdk_draw_line (frame, widget->style->black_gc, x1 + 1, y1 + 1, x2 - 2, y1 + 1);

	  bdk_draw_rectangle (frame, widget->style->black_gc, FALSE,
			      x1 + 1, y1 + 2,
			      DECORATION_BUTTON_SIZE - 3, DECORATION_BUTTON_SIZE - 4);

	  if (area)
	    bdk_gc_set_clip_rectangle (widget->style->black_gc, NULL);
	}
      
      /* Close button: */
      
      x1 = width - DECORATION_BORDER_LEFT - DECORATION_BUTTON_SIZE;
      y1 = DECORATION_BUTTON_Y_OFFSET;
      x2 = width - DECORATION_BORDER_LEFT;
      y2 = DECORATION_BUTTON_Y_OFFSET + DECORATION_BUTTON_SIZE;

      if (area)
	bdk_gc_set_clip_rectangle (widget->style->bg_gc[widget->state], area);

      bdk_draw_rectangle (frame, widget->style->bg_gc[widget->state], TRUE,
			  x1, y1, x2 - x1, y2 - y1);

      if (area)
	bdk_gc_set_clip_rectangle (widget->style->bg_gc[widget->state], NULL);
      
      if (area)
	bdk_gc_set_clip_rectangle (widget->style->black_gc, area);

      bdk_draw_line (frame, widget->style->black_gc, x1, y1, x2-1, y2-1);

      bdk_draw_line (frame, widget->style->black_gc, x1, y2-1, x2-1, y1);

      if (area)
	bdk_gc_set_clip_rectangle (widget->style->black_gc, NULL);
      
      

      /* Title */
      if (deco->title_layout)
	{
	  if (area)
	    bdk_gc_set_clip_rectangle (widget->style->fg_gc [border_state], area);

	  bdk_draw_layout (frame,
			   widget->style->fg_gc [border_state],
			   DECORATION_BORDER_LEFT, 1,
			   deco->title_layout);
	  if (area)
	    bdk_gc_set_clip_rectangle (widget->style->fg_gc [border_state], NULL);
	}
      
    }
}


static void
btk_decorated_window_recalculate_rebunnyions (BtkWindow *window)
{
  gint n_rebunnyions;
  gint width, height;
  BtkWindowRebunnyion *rebunnyion;
  BtkWindowDecoration *deco = get_decoration (window);
      
  n_rebunnyions = 0;

  if (!deco->decorated)
    return;
  
  n_rebunnyions += 2; /* close, Title */
  if (deco->maximizable)
    n_rebunnyions += 1;
  if (window->allow_shrink || window->allow_grow)
    n_rebunnyions += 2;

  if (deco->n_rebunnyions != n_rebunnyions)
    {
      g_free (deco->rebunnyions);
      deco->rebunnyions = g_new (BtkWindowRebunnyion, n_rebunnyions);
      deco->n_rebunnyions = n_rebunnyions;
    }

  width = BTK_WIDGET (window)->allocation.width + DECORATION_BORDER_TOT_X;
  height = BTK_WIDGET (window)->allocation.height + DECORATION_BORDER_TOT_Y;

  rebunnyion = deco->rebunnyions;

  /* Maximize button */
  if (deco->maximizable)
    {
      rebunnyion->rect.x = width - (DECORATION_BORDER_LEFT * 2) - (DECORATION_BUTTON_SIZE * 2);
      rebunnyion->rect.y = DECORATION_BUTTON_Y_OFFSET;
      rebunnyion->rect.width = DECORATION_BUTTON_SIZE;
      rebunnyion->rect.height = DECORATION_BUTTON_SIZE;
      rebunnyion->type = BTK_WINDOW_REBUNNYION_MAXIMIZE;
      rebunnyion++;
    }

  /* Close button */
  rebunnyion->rect.x = width - DECORATION_BORDER_LEFT - DECORATION_BUTTON_SIZE;
  rebunnyion->rect.y = DECORATION_BUTTON_Y_OFFSET;
  rebunnyion->rect.width = DECORATION_BUTTON_SIZE;
  rebunnyion->rect.height = DECORATION_BUTTON_SIZE;
  rebunnyion->type = BTK_WINDOW_REBUNNYION_CLOSE;
  rebunnyion++;
    
  /* title bar */
  rebunnyion->rect.x = 0;
  rebunnyion->rect.y = 0;
  rebunnyion->rect.width = width;
  rebunnyion->rect.height = DECORATION_BORDER_TOP;
  rebunnyion->type = BTK_WINDOW_REBUNNYION_TITLE;
  rebunnyion++;
  
  if (window->allow_shrink || window->allow_grow)
    {
      rebunnyion->rect.x = width - (DECORATION_BORDER_RIGHT + 10);
      rebunnyion->rect.y = height - DECORATION_BORDER_BOTTOM;
      rebunnyion->rect.width = DECORATION_BORDER_RIGHT + 10;
      rebunnyion->rect.height = DECORATION_BORDER_BOTTOM;
      rebunnyion->type = BTK_WINDOW_REBUNNYION_BR_RESIZE;
      rebunnyion++;

      rebunnyion->rect.x = width - DECORATION_BORDER_RIGHT;
      rebunnyion->rect.y = height - (DECORATION_BORDER_BOTTOM + 10);
      rebunnyion->rect.width = DECORATION_BORDER_RIGHT;
      rebunnyion->rect.height = DECORATION_BORDER_BOTTOM + 10;
      rebunnyion->type = BTK_WINDOW_REBUNNYION_BR_RESIZE;
      rebunnyion++;
    }
}

void
btk_decorated_window_move_resize_window (BtkWindow   *window,
					 gint         x,
					 gint         y,
					 gint         width,
					 gint         height)
{
  BtkWidget *widget = BTK_WIDGET (window);
  BtkWindowDecoration *deco = get_decoration (window);
  
  deco->real_inner_move = TRUE;
  bdk_window_move_resize (widget->window,
			  x, y, width, height);
}
#else

void
btk_decorated_window_init (BtkWindow  *window)
{
}

void 
btk_decorated_window_calculate_frame_size (BtkWindow *window)
{
}

void
btk_decorated_window_set_title (BtkWindow   *window,
				const gchar *title)
{
}

void
btk_decorated_window_move_resize_window (BtkWindow   *window,
					 gint         x,
					 gint         y,
					 gint         width,
					 gint         height)
{
  bdk_window_move_resize (BTK_WIDGET (window)->window,
			  x, y, width, height);
}
#endif


#define __BTK_WINDOW_DECORATE_C__
#include "btkaliasdef.c"
