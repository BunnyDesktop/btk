/* Offscreen windows/Rotated button
 *
 * Offscreen windows can be used to transform parts of a widget
 * hierarchy. Note that the rotated button is fully functional.
 */
#include <math.h>
#include <btk/btk.h>

#define BTK_TYPE_ROTATED_BIN              (btk_rotated_bin_get_type ())
#define BTK_ROTATED_BIN(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_ROTATED_BIN, BtkRotatedBin))
#define BTK_ROTATED_BIN_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_ROTATED_BIN, BtkRotatedBinClass))
#define BTK_IS_ROTATED_BIN(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_ROTATED_BIN))
#define BTK_IS_ROTATED_BIN_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_ROTATED_BIN))
#define BTK_ROTATED_BIN_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_ROTATED_BIN, BtkRotatedBinClass))

typedef struct _BtkRotatedBin   BtkRotatedBin;
typedef struct _BtkRotatedBinClass  BtkRotatedBinClass;

struct _BtkRotatedBin
{
  BtkContainer container;

  BtkWidget *child;
  BdkWindow *offscreen_window;
  gdouble angle;
};

struct _BtkRotatedBinClass
{
  BtkContainerClass parent_class;
};

GType      btk_rotated_bin_get_type  (void) G_GNUC_CONST;
BtkWidget* btk_rotated_bin_new       (void);
void       btk_rotated_bin_set_angle (BtkRotatedBin *bin,
                                      gdouble        angle);

/*** implementation ***/

static void     btk_rotated_bin_realize       (BtkWidget       *widget);
static void     btk_rotated_bin_unrealize     (BtkWidget       *widget);
static void     btk_rotated_bin_size_request  (BtkWidget       *widget,
                                               BtkRequisition  *requisition);
static void     btk_rotated_bin_size_allocate (BtkWidget       *widget,
                                               BtkAllocation   *allocation);
static gboolean btk_rotated_bin_damage        (BtkWidget       *widget,
                                               BdkEventExpose  *event);
static gboolean btk_rotated_bin_expose        (BtkWidget       *widget,
                                               BdkEventExpose  *offscreen);

static void     btk_rotated_bin_add           (BtkContainer    *container,
                                               BtkWidget       *child);
static void     btk_rotated_bin_remove        (BtkContainer    *container,
                                               BtkWidget       *widget);
static void     btk_rotated_bin_forall        (BtkContainer    *container,
                                               gboolean         include_internals,
                                               BtkCallback      callback,
                                               gpointer         callback_data);
static GType    btk_rotated_bin_child_type    (BtkContainer    *container);

G_DEFINE_TYPE (BtkRotatedBin, btk_rotated_bin, BTK_TYPE_CONTAINER);

static void
to_child (BtkRotatedBin *bin,
          double         widget_x,
          double         widget_y,
          double        *x_out,
          double        *y_out)
{
  BtkAllocation child_area;
  double x, y, xr, yr;
  double c, s;
  double w, h;

  s = sin (bin->angle);
  c = cos (bin->angle);
  btk_widget_get_allocation (bin->child, &child_area);

  w = c * child_area.width + s * child_area.height;
  h = s * child_area.width + c * child_area.height;

  x = widget_x;
  y = widget_y;

  x -= (w - child_area.width) / 2;
  y -= (h - child_area.height) / 2;

  x -= child_area.width / 2;
  y -= child_area.height / 2;

  xr = x * c + y * s;
  yr = y * c - x * s;
  x = xr;
  y = yr;

  x += child_area.width / 2;
  y += child_area.height / 2;

  *x_out = x;
  *y_out = y;
}

static void
to_parent (BtkRotatedBin *bin,
           double         offscreen_x,
           double         offscreen_y,
           double        *x_out,
           double        *y_out)
{
  BtkAllocation child_area;
  double x, y, xr, yr;
  double c, s;
  double w, h;

  s = sin (bin->angle);
  c = cos (bin->angle);
  btk_widget_get_allocation (bin->child, &child_area);

  w = c * child_area.width + s * child_area.height;
  h = s * child_area.width + c * child_area.height;

  x = offscreen_x;
  y = offscreen_y;

  x -= child_area.width / 2;
  y -= child_area.height / 2;

  xr = x * c - y * s;
  yr = x * s + y * c;
  x = xr;
  y = yr;

  x += child_area.width / 2;
  y += child_area.height / 2;

  x -= (w - child_area.width) / 2;
  y -= (h - child_area.height) / 2;

  *x_out = x;
  *y_out = y;
}

static void
btk_rotated_bin_class_init (BtkRotatedBinClass *klass)
{
  BtkWidgetClass *widget_class = BTK_WIDGET_CLASS (klass);
  BtkContainerClass *container_class = BTK_CONTAINER_CLASS (klass);

  widget_class->realize = btk_rotated_bin_realize;
  widget_class->unrealize = btk_rotated_bin_unrealize;
  widget_class->size_request = btk_rotated_bin_size_request;
  widget_class->size_allocate = btk_rotated_bin_size_allocate;
  widget_class->expose_event = btk_rotated_bin_expose;

  g_signal_override_class_closure (g_signal_lookup ("damage-event", BTK_TYPE_WIDGET),
                                   BTK_TYPE_ROTATED_BIN,
                                   g_cclosure_new (G_CALLBACK (btk_rotated_bin_damage),
                                                   NULL, NULL));

  container_class->add = btk_rotated_bin_add;
  container_class->remove = btk_rotated_bin_remove;
  container_class->forall = btk_rotated_bin_forall;
  container_class->child_type = btk_rotated_bin_child_type;
}

static void
btk_rotated_bin_init (BtkRotatedBin *bin)
{
  btk_widget_set_has_window (BTK_WIDGET (bin), TRUE);
}

BtkWidget *
btk_rotated_bin_new (void)
{
  return g_object_new (BTK_TYPE_ROTATED_BIN, NULL);
}

static BdkWindow *
pick_offscreen_child (BdkWindow     *offscreen_window,
                      double         widget_x,
                      double         widget_y,
                      BtkRotatedBin *bin)
{
 BtkAllocation child_area;
 double x, y;

 if (bin->child && btk_widget_get_visible (bin->child))
    {
      to_child (bin, widget_x, widget_y, &x, &y);

      btk_widget_get_allocation (bin->child, &child_area);

      if (x >= 0 && x < child_area.width &&
          y >= 0 && y < child_area.height)
        return bin->offscreen_window;
    }

  return NULL;
}

static void
offscreen_window_to_parent (BdkWindow     *offscreen_window,
                            double         offscreen_x,
                            double         offscreen_y,
                            double        *parent_x,
                            double        *parent_y,
                            BtkRotatedBin *bin)
{
  to_parent (bin, offscreen_x, offscreen_y, parent_x, parent_y);
}

static void
offscreen_window_from_parent (BdkWindow     *window,
                              double         parent_x,
                              double         parent_y,
                              double        *offscreen_x,
                              double        *offscreen_y,
                              BtkRotatedBin *bin)
{
  to_child (bin, parent_x, parent_y, offscreen_x, offscreen_y);
}

static void
btk_rotated_bin_realize (BtkWidget *widget)
{
  BtkRotatedBin *bin = BTK_ROTATED_BIN (widget);
  BdkWindow *bdk_window;
  BdkWindowAttr attributes;
  gint attributes_mask;
  gint border_width;
  BtkRequisition child_requisition;
  BtkAllocation widget_allocation, bin_child_allocation;
  BtkStyle *style;

  btk_widget_set_realized (widget, TRUE);

  border_width = btk_container_get_border_width (BTK_CONTAINER (widget));
  btk_widget_get_allocation (widget, &widget_allocation);

  attributes.x = widget_allocation.x + border_width;
  attributes.y = widget_allocation.y + border_width;
  attributes.width = widget_allocation.width - 2 * border_width;
  attributes.height = widget_allocation.height - 2 * border_width;
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
  
  bdk_window = bdk_window_new (btk_widget_get_parent_window (widget), &attributes, attributes_mask);
  btk_widget_set_window (widget, bdk_window);
  bdk_window_set_user_data (bdk_window, widget);
  g_signal_connect (bdk_window, "pick-embedded-child",
                    G_CALLBACK (pick_offscreen_child), bin);

  attributes.window_type = BDK_WINDOW_OFFSCREEN;

  child_requisition.width = child_requisition.height = 0;
  if (bin->child && btk_widget_get_visible (bin->child))
    {
      btk_widget_get_allocation (bin->child, &bin_child_allocation);
      attributes.width = bin_child_allocation.width;
      attributes.height = bin_child_allocation.height;
    }
  bin->offscreen_window = bdk_window_new (btk_widget_get_root_window (widget),
                                          &attributes, attributes_mask);
  bdk_window_set_user_data (bin->offscreen_window, widget);
  if (bin->child)
    btk_widget_set_parent_window (bin->child, bin->offscreen_window);
  bdk_offscreen_window_set_embedder (bin->offscreen_window, btk_widget_get_window (widget));
  g_signal_connect (bin->offscreen_window, "to-embedder",
                    G_CALLBACK (offscreen_window_to_parent), bin);
  g_signal_connect (bin->offscreen_window, "from-embedder",
                    G_CALLBACK (offscreen_window_from_parent), bin);

  btk_widget_style_attach (widget);
  style = btk_widget_get_style (widget);
  btk_style_set_background (style, bdk_window, BTK_STATE_NORMAL);
  btk_style_set_background (style, bin->offscreen_window, BTK_STATE_NORMAL);
  bdk_window_show (bin->offscreen_window);
}

static void
btk_rotated_bin_unrealize (BtkWidget *widget)
{
  BtkRotatedBin *bin = BTK_ROTATED_BIN (widget);

  bdk_window_set_user_data (bin->offscreen_window, NULL);
  bdk_window_destroy (bin->offscreen_window);
  bin->offscreen_window = NULL;

  BTK_WIDGET_CLASS (btk_rotated_bin_parent_class)->unrealize (widget);
}

static GType
btk_rotated_bin_child_type (BtkContainer *container)
{
  BtkRotatedBin *bin = BTK_ROTATED_BIN (container);

  if (bin->child)
    return G_TYPE_NONE;

  return BTK_TYPE_WIDGET;
}

static void
btk_rotated_bin_add (BtkContainer *container,
                     BtkWidget    *widget)
{
  BtkRotatedBin *bin = BTK_ROTATED_BIN (container);

  if (!bin->child)
    {
      btk_widget_set_parent_window (widget, bin->offscreen_window);
      btk_widget_set_parent (widget, BTK_WIDGET (bin));
      bin->child = widget;
    }
  else
    g_warning ("BtkRotatedBin cannot have more than one child\n");
}

static void
btk_rotated_bin_remove (BtkContainer *container,
                        BtkWidget    *widget)
{
  BtkRotatedBin *bin = BTK_ROTATED_BIN (container);
  gboolean was_visible;

  was_visible = btk_widget_get_visible (widget);

  if (bin->child == widget)
    {
      btk_widget_unparent (widget);

      bin->child = NULL;

      if (was_visible && btk_widget_get_visible (BTK_WIDGET (container)))
        btk_widget_queue_resize (BTK_WIDGET (container));
    }
}

static void
btk_rotated_bin_forall (BtkContainer *container,
                        gboolean      include_internals,
                        BtkCallback   callback,
                        gpointer      callback_data)
{
  BtkRotatedBin *bin = BTK_ROTATED_BIN (container);

  g_return_if_fail (callback != NULL);

  if (bin->child)
    (*callback) (bin->child, callback_data);
}

void
btk_rotated_bin_set_angle (BtkRotatedBin *bin,
                           gdouble        angle)
{
  g_return_if_fail (BTK_IS_ROTATED_BIN (bin));

  bin->angle = angle;
  btk_widget_queue_resize (BTK_WIDGET (bin));

  bdk_window_geometry_changed (bin->offscreen_window);
}

static void
btk_rotated_bin_size_request (BtkWidget      *widget,
                              BtkRequisition *requisition)
{
  BtkRotatedBin *bin = BTK_ROTATED_BIN (widget);
  BtkRequisition child_requisition;
  gint border_width;
  double s, c;
  double w, h;
  
  border_width = btk_container_get_border_width (BTK_CONTAINER (widget));
  
  child_requisition.width = 0;
  child_requisition.height = 0;

  if (bin->child && btk_widget_get_visible (bin->child))
    btk_widget_size_request (bin->child, &child_requisition);

  s = sin (bin->angle);
  c = cos (bin->angle);
  w = c * child_requisition.width + s * child_requisition.height;
  h = s * child_requisition.width + c * child_requisition.height;

  requisition->width = border_width * 2 + w;
  requisition->height = border_width * 2 + h;
}

static void
btk_rotated_bin_size_allocate (BtkWidget     *widget,
                               BtkAllocation *allocation)
{
  BtkRotatedBin *bin = BTK_ROTATED_BIN (widget);
  gint border_width;
  gint w, h;
  gdouble s, c;

  btk_widget_set_allocation (widget, allocation);

  border_width = btk_container_get_border_width (BTK_CONTAINER (widget));

  w = allocation->width - border_width * 2;
  h = allocation->height - border_width * 2;

  if (btk_widget_get_realized (widget))
    bdk_window_move_resize (btk_widget_get_window (widget),
                            allocation->x + border_width,
                            allocation->y + border_width,
                            w, h);

  if (bin->child && btk_widget_get_visible (bin->child))
    {
      BtkRequisition child_requisition;
      BtkAllocation child_allocation;

      s = sin (bin->angle);
      c = cos (bin->angle);

      btk_widget_get_child_requisition (bin->child, &child_requisition);
      child_allocation.x = 0;
      child_allocation.y = 0;
      child_allocation.height = child_requisition.height;
      if (c == 0.0)
        child_allocation.width = h / s;
      else if (s == 0.0)
        child_allocation.width = w / c;
      else
        child_allocation.width = MIN ((w - s * child_allocation.height) / c,
                                      (h - c * child_allocation.height) / s);

      if (btk_widget_get_realized (widget))
        bdk_window_move_resize (bin->offscreen_window,
                                child_allocation.x,
                                child_allocation.y,
                                child_allocation.width,
                                child_allocation.height);

      child_allocation.x = child_allocation.y = 0;
      btk_widget_size_allocate (bin->child, &child_allocation);
    }
}

static gboolean
btk_rotated_bin_damage (BtkWidget      *widget,
                        BdkEventExpose *event)
{
  bdk_window_invalidate_rect (btk_widget_get_window (widget), NULL, FALSE);

  return TRUE;
}

static gboolean
btk_rotated_bin_expose (BtkWidget      *widget,
                        BdkEventExpose *event)
{
  BtkRotatedBin *bin = BTK_ROTATED_BIN (widget);
  gint width, height;
  gdouble s, c;
  gdouble w, h;

  if (btk_widget_is_drawable (widget))
    {
      if (event->window == btk_widget_get_window (widget))
        {
          BdkPixmap *pixmap;
          BtkAllocation child_area;
          bairo_t *cr;

          if (bin->child && btk_widget_get_visible (bin->child))
            {
              pixmap = bdk_offscreen_window_get_pixmap (bin->offscreen_window);
              btk_widget_get_allocation (bin->child, &child_area);

              cr = bdk_bairo_create (btk_widget_get_window (widget));

              /* transform */
              s = sin (bin->angle);
              c = cos (bin->angle);
              w = c * child_area.width + s * child_area.height;
              h = s * child_area.width + c * child_area.height;

              bairo_translate (cr, (w - child_area.width) / 2, (h - child_area.height) / 2);
              bairo_translate (cr, child_area.width / 2, child_area.height / 2);
              bairo_rotate (cr, bin->angle);
              bairo_translate (cr, -child_area.width / 2, -child_area.height / 2);

              /* clip */
              bdk_pixmap_get_size (pixmap, &width, &height);
              bairo_rectangle (cr, 0, 0, width, height);
              bairo_clip (cr);
              /* paint */
              bdk_bairo_set_source_pixmap (cr, pixmap, 0, 0);
              bairo_paint (cr);

              bairo_destroy (cr);
            }
        }
      else if (event->window == bin->offscreen_window)
        {
          btk_paint_flat_box (btk_widget_get_style (widget), event->window,
                              BTK_STATE_NORMAL, BTK_SHADOW_NONE,
                              &event->area, widget, "blah",
                              0, 0, -1, -1);

          if (bin->child)
            btk_container_propagate_expose (BTK_CONTAINER (widget),
                                            bin->child,
                                            event);
        }
    }

  return FALSE;
}

/*** ***/

static void
scale_changed (BtkRange      *range,
               BtkRotatedBin *bin)
{
  btk_rotated_bin_set_angle (bin, btk_range_get_value (range));
}

static BtkWidget *window = NULL;

BtkWidget *
do_offscreen_window (BtkWidget *do_widget)
{
  if (!window)
    {
      BtkWidget *bin, *vbox, *scale, *button;
      BdkColor black;

      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_screen (BTK_WINDOW (window),
                             btk_widget_get_screen (do_widget));
      btk_window_set_title (BTK_WINDOW (window), "Rotated widget");

      g_signal_connect (window, "destroy",
                        G_CALLBACK (btk_widget_destroyed), &window);

      bdk_color_parse ("black", &black);
      btk_widget_modify_bg (window, BTK_STATE_NORMAL, &black);
      btk_container_set_border_width (BTK_CONTAINER (window), 10);

      vbox = btk_vbox_new (0, FALSE);
      scale = btk_hscale_new_with_range (0, G_PI/2, 0.01);
      btk_scale_set_draw_value (BTK_SCALE (scale), FALSE);

      button = btk_button_new_with_label ("A Button");
      bin = btk_rotated_bin_new ();

      g_signal_connect (scale, "value-changed", G_CALLBACK (scale_changed), bin);

      btk_container_add (BTK_CONTAINER (window), vbox);
      btk_box_pack_start (BTK_BOX (vbox), scale, FALSE, FALSE, 0);
      btk_box_pack_start (BTK_BOX (vbox), bin, TRUE, TRUE, 0);
      btk_container_add (BTK_CONTAINER (bin), button);
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    {
      btk_widget_destroy (window);
      window = NULL;
    }

  return window;
}

