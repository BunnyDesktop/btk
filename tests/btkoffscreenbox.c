/*
 * btkoffscreenbox.c
 */

#include "config.h"

#include <math.h>
#include <btk/btk.h>

#include "btkoffscreenbox.h"

static void        btk_offscreen_box_realize       (BtkWidget       *widget);
static void        btk_offscreen_box_unrealize     (BtkWidget       *widget);
static void        btk_offscreen_box_size_request  (BtkWidget       *widget,
                                                    BtkRequisition  *requisition);
static void        btk_offscreen_box_size_allocate (BtkWidget       *widget,
                                                    BtkAllocation   *allocation);
static gboolean    btk_offscreen_box_damage        (BtkWidget       *widget,
                                                    BdkEventExpose  *event);
static gboolean    btk_offscreen_box_expose        (BtkWidget       *widget,
                                                    BdkEventExpose  *offscreen);

static void        btk_offscreen_box_add           (BtkContainer    *container,
                                                    BtkWidget       *child);
static void        btk_offscreen_box_remove        (BtkContainer    *container,
                                                    BtkWidget       *widget);
static void        btk_offscreen_box_forall        (BtkContainer    *container,
                                                    gboolean         include_internals,
                                                    BtkCallback      callback,
                                                    gpointer         callback_data);
static GType       btk_offscreen_box_child_type    (BtkContainer    *container);

#define CHILD1_SIZE_SCALE 1.0
#define CHILD2_SIZE_SCALE 1.0

G_DEFINE_TYPE (BtkOffscreenBox, btk_offscreen_box, BTK_TYPE_CONTAINER);

static void
to_child_2 (BtkOffscreenBox *offscreen_box,
	    double widget_x, double widget_y,
	    double *x_out, double *y_out)
{
  BtkAllocation child_area;
  double x, y, xr, yr;
  double cos_angle, sin_angle;

  x = widget_x;
  y = widget_y;

  if (offscreen_box->child1 && btk_widget_get_visible (offscreen_box->child1))
    y -= offscreen_box->child1->allocation.height;

  child_area = offscreen_box->child2->allocation;

  x -= child_area.width / 2;
  y -= child_area.height / 2;

  cos_angle = cos (-offscreen_box->angle);
  sin_angle = sin (-offscreen_box->angle);

  xr = x * cos_angle - y * sin_angle;
  yr = x * sin_angle + y * cos_angle;
  x = xr;
  y = yr;

  x += child_area.width / 2;
  y += child_area.height / 2;

  *x_out = x;
  *y_out = y;
}

static void
to_parent_2 (BtkOffscreenBox *offscreen_box,
	     double offscreen_x, double offscreen_y,
	     double *x_out, double *y_out)
{
  BtkAllocation child_area;
  double x, y, xr, yr;
  double cos_angle, sin_angle;

  child_area = offscreen_box->child2->allocation;

  x = offscreen_x;
  y = offscreen_y;

  x -= child_area.width / 2;
  y -= child_area.height / 2;

  cos_angle = cos (offscreen_box->angle);
  sin_angle = sin (offscreen_box->angle);

  xr = x * cos_angle - y * sin_angle;
  yr = x * sin_angle + y * cos_angle;
  x = xr;
  y = yr;

  x += child_area.width / 2;
  y += child_area.height / 2;

  if (offscreen_box->child1 && btk_widget_get_visible (offscreen_box->child1))
    y += offscreen_box->child1->allocation.height;

  *x_out = x;
  *y_out = y;
}

static void
btk_offscreen_box_class_init (BtkOffscreenBoxClass *klass)
{
  BtkWidgetClass *widget_class = BTK_WIDGET_CLASS (klass);
  BtkContainerClass *container_class = BTK_CONTAINER_CLASS (klass);

  widget_class->realize = btk_offscreen_box_realize;
  widget_class->unrealize = btk_offscreen_box_unrealize;
  widget_class->size_request = btk_offscreen_box_size_request;
  widget_class->size_allocate = btk_offscreen_box_size_allocate;
  widget_class->expose_event = btk_offscreen_box_expose;

  g_signal_override_class_closure (g_signal_lookup ("damage-event", BTK_TYPE_WIDGET),
                                   BTK_TYPE_OFFSCREEN_BOX,
                                   g_cclosure_new (G_CALLBACK (btk_offscreen_box_damage),
                                                   NULL, NULL));

  container_class->add = btk_offscreen_box_add;
  container_class->remove = btk_offscreen_box_remove;
  container_class->forall = btk_offscreen_box_forall;
  container_class->child_type = btk_offscreen_box_child_type;
}

static void
btk_offscreen_box_init (BtkOffscreenBox *offscreen_box)
{
  btk_widget_set_has_window (BTK_WIDGET (offscreen_box), TRUE);
}

BtkWidget *
btk_offscreen_box_new (void)
{
  return g_object_new (BTK_TYPE_OFFSCREEN_BOX, NULL);
}

static BdkWindow *
pick_offscreen_child (BdkWindow *offscreen_window,
		      double widget_x, double widget_y,
		      BtkOffscreenBox *offscreen_box)
{
 BtkAllocation child_area;
 double x, y;

 /* First check for child 2 */
 if (offscreen_box->child2 &&
     btk_widget_get_visible (offscreen_box->child2))
    {
      to_child_2 (offscreen_box,
		  widget_x, widget_y,
		  &x, &y);

      child_area = offscreen_box->child2->allocation;

      if (x >= 0 && x < child_area.width &&
	  y >= 0 && y < child_area.height)
	return offscreen_box->offscreen_window2;
    }

 if (offscreen_box->child1 && btk_widget_get_visible (offscreen_box->child1))
   {
     x = widget_x;
     y = widget_y;

     child_area = offscreen_box->child1->allocation;

     if (x >= 0 && x < child_area.width &&
	 y >= 0 && y < child_area.height)
       return offscreen_box->offscreen_window1;
   }

  return NULL;
}

static void
offscreen_window_to_parent1 (BdkWindow       *offscreen_window,
			     double           offscreen_x,
			     double           offscreen_y,
			     double          *parent_x,
			     double          *parent_y,
			     BtkOffscreenBox *offscreen_box)
{
  *parent_x = offscreen_x;
  *parent_y = offscreen_y;
}

static void
offscreen_window_from_parent1 (BdkWindow       *window,
			       double           parent_x,
			       double           parent_y,
			       double          *offscreen_x,
			       double          *offscreen_y,
			       BtkOffscreenBox *offscreen_box)
{
  *offscreen_x = parent_x;
  *offscreen_y = parent_y;
}

static void
offscreen_window_to_parent2 (BdkWindow       *offscreen_window,
			     double           offscreen_x,
			     double           offscreen_y,
			     double          *parent_x,
			     double          *parent_y,
			     BtkOffscreenBox *offscreen_box)
{
  to_parent_2 (offscreen_box,
	      offscreen_x, offscreen_y,
	      parent_x, parent_y);
}

static void
offscreen_window_from_parent2 (BdkWindow       *window,
			       double           parent_x,
			       double           parent_y,
			       double          *offscreen_x,
			       double          *offscreen_y,
			       BtkOffscreenBox *offscreen_box)
{
  to_child_2 (offscreen_box,
	      parent_x, parent_y,
	      offscreen_x, offscreen_y);
}

static void
btk_offscreen_box_realize (BtkWidget *widget)
{
  BtkOffscreenBox *offscreen_box = BTK_OFFSCREEN_BOX (widget);
  BdkWindowAttr attributes;
  gint attributes_mask;
  gint border_width;
  BtkRequisition child_requisition;
  int start_y = 0;

  btk_widget_set_realized (widget, TRUE);

  border_width = BTK_CONTAINER (widget)->border_width;

  attributes.x = widget->allocation.x + border_width;
  attributes.y = widget->allocation.y + border_width;
  attributes.width = widget->allocation.width - 2 * border_width;
  attributes.height = widget->allocation.height - 2 * border_width;
  attributes.window_type = BDK_WINDOW_CHILD;
  attributes.event_mask = btk_widget_get_events (widget)
			| BDK_EXPOSURE_MASK
			| BDK_POINTER_MOTION_MASK
			| BDK_BUTTON_PRESS_MASK
			| BDK_BUTTON_RELEASE_MASK
			| BDK_SCROLL_MASK
			| BDK_ENTER_NOTIFY_MASK
			| BDK_LEAVE_NOTIFY_MASK;

  attributes.visual = btk_widget_get_visual (widget);
  attributes.colormap = btk_widget_get_colormap (widget);
  attributes.wclass = BDK_INPUT_OUTPUT;

  attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_VISUAL | BDK_WA_COLORMAP;

  widget->window = bdk_window_new (btk_widget_get_parent_window (widget),
				   &attributes, attributes_mask);
  bdk_window_set_user_data (widget->window, widget);

  g_signal_connect (widget->window, "pick-embedded-child",
		    G_CALLBACK (pick_offscreen_child), offscreen_box);

  attributes.window_type = BDK_WINDOW_OFFSCREEN;

  /* Child 1 */
  attributes.x = attributes.y = 0;
  if (offscreen_box->child1 && btk_widget_get_visible (offscreen_box->child1))
    {
      attributes.width = offscreen_box->child1->allocation.width;
      attributes.height = offscreen_box->child1->allocation.height;
      start_y += offscreen_box->child1->allocation.height;
    }
  offscreen_box->offscreen_window1 = bdk_window_new (btk_widget_get_root_window (widget),
						     &attributes, attributes_mask);
  bdk_window_set_user_data (offscreen_box->offscreen_window1, widget);
  if (offscreen_box->child1)
    btk_widget_set_parent_window (offscreen_box->child1, offscreen_box->offscreen_window1);

  bdk_offscreen_window_set_embedder (offscreen_box->offscreen_window1,
				     widget->window);
  
  g_signal_connect (offscreen_box->offscreen_window1, "to-embedder",
		    G_CALLBACK (offscreen_window_to_parent1), offscreen_box);
  g_signal_connect (offscreen_box->offscreen_window1, "from-embedder",
		    G_CALLBACK (offscreen_window_from_parent1), offscreen_box);

  /* Child 2 */
  attributes.y = start_y;
  child_requisition.width = child_requisition.height = 0;
  if (offscreen_box->child2 && btk_widget_get_visible (offscreen_box->child2))
    {
      attributes.width = offscreen_box->child2->allocation.width;
      attributes.height = offscreen_box->child2->allocation.height;
    }
  offscreen_box->offscreen_window2 = bdk_window_new (btk_widget_get_root_window (widget),
						     &attributes, attributes_mask);
  bdk_window_set_user_data (offscreen_box->offscreen_window2, widget);
  if (offscreen_box->child2)
    btk_widget_set_parent_window (offscreen_box->child2, offscreen_box->offscreen_window2);
  bdk_offscreen_window_set_embedder (offscreen_box->offscreen_window2,
				     widget->window);
  g_signal_connect (offscreen_box->offscreen_window2, "to-embedder",
		    G_CALLBACK (offscreen_window_to_parent2), offscreen_box);
  g_signal_connect (offscreen_box->offscreen_window2, "from-embedder",
		    G_CALLBACK (offscreen_window_from_parent2), offscreen_box);

  widget->style = btk_style_attach (widget->style, widget->window);

  btk_style_set_background (widget->style, widget->window, BTK_STATE_NORMAL);
  btk_style_set_background (widget->style, offscreen_box->offscreen_window1, BTK_STATE_NORMAL);
  btk_style_set_background (widget->style, offscreen_box->offscreen_window2, BTK_STATE_NORMAL);

  bdk_window_show (offscreen_box->offscreen_window1);
  bdk_window_show (offscreen_box->offscreen_window2);
}

static void
btk_offscreen_box_unrealize (BtkWidget *widget)
{
  BtkOffscreenBox *offscreen_box = BTK_OFFSCREEN_BOX (widget);

  bdk_window_set_user_data (offscreen_box->offscreen_window1, NULL);
  bdk_window_destroy (offscreen_box->offscreen_window1);
  offscreen_box->offscreen_window1 = NULL;

  bdk_window_set_user_data (offscreen_box->offscreen_window2, NULL);
  bdk_window_destroy (offscreen_box->offscreen_window2);
  offscreen_box->offscreen_window2 = NULL;

  BTK_WIDGET_CLASS (btk_offscreen_box_parent_class)->unrealize (widget);
}

static GType
btk_offscreen_box_child_type (BtkContainer *container)
{
  BtkOffscreenBox *offscreen_box = BTK_OFFSCREEN_BOX (container);

  if (offscreen_box->child1 && offscreen_box->child2)
    return G_TYPE_NONE;

  return BTK_TYPE_WIDGET;
}

static void
btk_offscreen_box_add (BtkContainer *container,
		       BtkWidget    *widget)
{
  BtkOffscreenBox *offscreen_box = BTK_OFFSCREEN_BOX (container);

  if (!offscreen_box->child1)
    btk_offscreen_box_add1 (offscreen_box, widget);
  else if (!offscreen_box->child2)
    btk_offscreen_box_add2 (offscreen_box, widget);
  else
    g_warning ("BtkOffscreenBox cannot have more than 2 children\n");
}

void
btk_offscreen_box_add1 (BtkOffscreenBox *offscreen_box,
			BtkWidget       *child)
{
  g_return_if_fail (BTK_IS_OFFSCREEN_BOX (offscreen_box));
  g_return_if_fail (BTK_IS_WIDGET (child));

  if (offscreen_box->child1 == NULL)
    {
      btk_widget_set_parent_window (child, offscreen_box->offscreen_window1);
      btk_widget_set_parent (child, BTK_WIDGET (offscreen_box));
      offscreen_box->child1 = child;
    }
}

void
btk_offscreen_box_add2 (BtkOffscreenBox  *offscreen_box,
			BtkWidget    *child)
{
  g_return_if_fail (BTK_IS_OFFSCREEN_BOX (offscreen_box));
  g_return_if_fail (BTK_IS_WIDGET (child));

  if (offscreen_box->child2 == NULL)
    {
      btk_widget_set_parent_window (child, offscreen_box->offscreen_window2);
      btk_widget_set_parent (child, BTK_WIDGET (offscreen_box));
      offscreen_box->child2 = child;
    }
}

static void
btk_offscreen_box_remove (BtkContainer *container,
			  BtkWidget    *widget)
{
  BtkOffscreenBox *offscreen_box = BTK_OFFSCREEN_BOX (container);
  gboolean was_visible;

  was_visible = btk_widget_get_visible (widget);

  if (offscreen_box->child1 == widget)
    {
      btk_widget_unparent (widget);

      offscreen_box->child1 = NULL;

      if (was_visible && btk_widget_get_visible (BTK_WIDGET (container)))
	btk_widget_queue_resize (BTK_WIDGET (container));
    }
  else if (offscreen_box->child2 == widget)
    {
      btk_widget_unparent (widget);

      offscreen_box->child2 = NULL;

      if (was_visible && btk_widget_get_visible (BTK_WIDGET (container)))
	btk_widget_queue_resize (BTK_WIDGET (container));
    }
}

static void
btk_offscreen_box_forall (BtkContainer *container,
			  gboolean      include_internals,
			  BtkCallback   callback,
			  gpointer      callback_data)
{
  BtkOffscreenBox *offscreen_box = BTK_OFFSCREEN_BOX (container);

  g_return_if_fail (callback != NULL);

  if (offscreen_box->child1)
    (*callback) (offscreen_box->child1, callback_data);
  if (offscreen_box->child2)
    (*callback) (offscreen_box->child2, callback_data);
}

void
btk_offscreen_box_set_angle (BtkOffscreenBox  *offscreen_box,
			     gdouble           angle)
{
  g_return_if_fail (BTK_IS_OFFSCREEN_BOX (offscreen_box));

  offscreen_box->angle = angle;
  btk_widget_queue_draw (BTK_WIDGET (offscreen_box));

  /* TODO: Really needs to resent pointer events if over the rotated window */
}


static void
btk_offscreen_box_size_request (BtkWidget      *widget,
				BtkRequisition *requisition)
{
  BtkOffscreenBox *offscreen_box = BTK_OFFSCREEN_BOX (widget);
  int w, h;

  w = 0;
  h = 0;

  if (offscreen_box->child1 && btk_widget_get_visible (offscreen_box->child1))
    {
      BtkRequisition child_requisition;

      btk_widget_size_request (offscreen_box->child1, &child_requisition);

      w = MAX (w, CHILD1_SIZE_SCALE * child_requisition.width);
      h += CHILD1_SIZE_SCALE * child_requisition.height;
    }

  if (offscreen_box->child2 && btk_widget_get_visible (offscreen_box->child2))
    {
      BtkRequisition child_requisition;

      btk_widget_size_request (offscreen_box->child2, &child_requisition);

      w = MAX (w, CHILD2_SIZE_SCALE * child_requisition.width);
      h += CHILD2_SIZE_SCALE * child_requisition.height;
    }

  requisition->width = BTK_CONTAINER (widget)->border_width * 2 + w;
  requisition->height = BTK_CONTAINER (widget)->border_width * 2 + h;
}

static void
btk_offscreen_box_size_allocate (BtkWidget     *widget,
				 BtkAllocation *allocation)
{
  BtkOffscreenBox *offscreen_box;
  gint border_width;
  gint start_y;

  widget->allocation = *allocation;
  offscreen_box = BTK_OFFSCREEN_BOX (widget);

  border_width = BTK_CONTAINER (widget)->border_width;

  if (btk_widget_get_realized (widget))
    bdk_window_move_resize (widget->window,
                            allocation->x + border_width,
                            allocation->y + border_width,
                            allocation->width - border_width * 2,
                            allocation->height - border_width * 2);

  start_y = 0;

  if (offscreen_box->child1 && btk_widget_get_visible (offscreen_box->child1))
    {
      BtkRequisition child_requisition;
      BtkAllocation child_allocation;

      btk_widget_get_child_requisition (offscreen_box->child1, &child_requisition);
      child_allocation.x = child_requisition.width * (CHILD1_SIZE_SCALE - 1.0) / 2;
      child_allocation.y = start_y + child_requisition.height * (CHILD1_SIZE_SCALE - 1.0) / 2;
      child_allocation.width = MAX (1, (gint) widget->allocation.width - 2 * border_width);
      child_allocation.height = child_requisition.height;

      start_y += CHILD1_SIZE_SCALE * child_requisition.height;

      if (btk_widget_get_realized (widget))
	bdk_window_move_resize (offscreen_box->offscreen_window1,
				child_allocation.x,
                                child_allocation.y,
				child_allocation.width,
                                child_allocation.height);

      child_allocation.x = child_allocation.y = 0;
      btk_widget_size_allocate (offscreen_box->child1, &child_allocation);
    }

  if (offscreen_box->child2 && btk_widget_get_visible (offscreen_box->child2))
    {
      BtkRequisition child_requisition;
      BtkAllocation child_allocation;

      btk_widget_get_child_requisition (offscreen_box->child2, &child_requisition);
      child_allocation.x = child_requisition.width * (CHILD2_SIZE_SCALE - 1.0) / 2;
      child_allocation.y = start_y + child_requisition.height * (CHILD2_SIZE_SCALE - 1.0) / 2;
      child_allocation.width = MAX (1, (gint) widget->allocation.width - 2 * border_width);
      child_allocation.height = child_requisition.height;

      start_y += CHILD2_SIZE_SCALE * child_requisition.height;

      if (btk_widget_get_realized (widget))
	bdk_window_move_resize (offscreen_box->offscreen_window2,
				child_allocation.x,
                                child_allocation.y,
				child_allocation.width,
                                child_allocation.height);

      child_allocation.x = child_allocation.y = 0;
      btk_widget_size_allocate (offscreen_box->child2, &child_allocation);
    }
}

static gboolean
btk_offscreen_box_damage (BtkWidget      *widget,
                          BdkEventExpose *event)
{
  bdk_window_invalidate_rect (widget->window, NULL, FALSE);

  return TRUE;
}

static gboolean
btk_offscreen_box_expose (BtkWidget      *widget,
			  BdkEventExpose *event)
{
  BtkOffscreenBox *offscreen_box = BTK_OFFSCREEN_BOX (widget);

  if (btk_widget_is_drawable (widget))
    {
      if (event->window == widget->window)
	{
          BdkPixmap *pixmap;
          BtkAllocation child_area;
          bairo_t *cr;
	  int start_y = 0;

	  if (offscreen_box->child1 && btk_widget_get_visible (offscreen_box->child1))
	    {
	      pixmap = bdk_offscreen_window_get_pixmap (offscreen_box->offscreen_window1);
              child_area = offscreen_box->child1->allocation;

	      cr = bdk_bairo_create (widget->window);

              bdk_bairo_set_source_pixmap (cr, pixmap, 0, 0);
              bairo_paint (cr);

              bairo_destroy (cr);

              start_y += child_area.height;
	    }

	  if (offscreen_box->child2 && btk_widget_get_visible (offscreen_box->child2))
	    {
	      pixmap = bdk_offscreen_window_get_pixmap (offscreen_box->offscreen_window2);
              child_area = offscreen_box->child2->allocation;

	      cr = bdk_bairo_create (widget->window);

              /* transform */
	      bairo_translate (cr, 0, start_y);
	      bairo_translate (cr, child_area.width / 2, child_area.height / 2);
	      bairo_rotate (cr, offscreen_box->angle);
	      bairo_translate (cr, -child_area.width / 2, -child_area.height / 2);

              /* paint */
	      bdk_bairo_set_source_pixmap (cr, pixmap, 0, 0);
	      bairo_paint (cr);

              bairo_destroy (cr);
	    }
	}
      else if (event->window == offscreen_box->offscreen_window1)
	{
	  btk_paint_flat_box (widget->style, event->window,
			      BTK_STATE_NORMAL, BTK_SHADOW_NONE,
			      &event->area, widget, "blah",
			      0, 0, -1, -1);

	  if (offscreen_box->child1)
	    btk_container_propagate_expose (BTK_CONTAINER (widget),
					    offscreen_box->child1,
                                            event);
	}
      else if (event->window == offscreen_box->offscreen_window2)
	{
	  btk_paint_flat_box (widget->style, event->window,
			      BTK_STATE_NORMAL, BTK_SHADOW_NONE,
			      &event->area, widget, "blah",
			      0, 0, -1, -1);

	  if (offscreen_box->child2)
	    btk_container_propagate_expose (BTK_CONTAINER (widget),
					    offscreen_box->child2,
                                            event);
	}
    }

  return FALSE;
}
