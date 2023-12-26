/* Offscreen windows/Effects
 *
 * Offscreen windows can be used to render elements multiple times to achieve
 * various effects.
 */
#include <btk/btk.h>

#define BTK_TYPE_MIRROR_BIN              (btk_mirror_bin_get_type ())
#define BTK_MIRROR_BIN(obj)              (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_MIRROR_BIN, BtkMirrorBin))
#define BTK_MIRROR_BIN_CLASS(klass)      (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_MIRROR_BIN, BtkMirrorBinClass))
#define BTK_IS_MIRROR_BIN(obj)           (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_MIRROR_BIN))
#define BTK_IS_MIRROR_BIN_CLASS(klass)   (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_MIRROR_BIN))
#define BTK_MIRROR_BIN_GET_CLASS(obj)    (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_MIRROR_BIN, BtkMirrorBinClass))

typedef struct _BtkMirrorBin   BtkMirrorBin;
typedef struct _BtkMirrorBinClass  BtkMirrorBinClass;

struct _BtkMirrorBin
{
  BtkContainer container;

  BtkWidget *child;
  BdkWindow *offscreen_window;
};

struct _BtkMirrorBinClass
{
  BtkContainerClass parent_class;
};

GType      btk_mirror_bin_get_type  (void) B_GNUC_CONST;
BtkWidget* btk_mirror_bin_new       (void);

/*** implementation ***/

static void     btk_mirror_bin_realize       (BtkWidget       *widget);
static void     btk_mirror_bin_unrealize     (BtkWidget       *widget);
static void     btk_mirror_bin_size_request  (BtkWidget       *widget,
                                               BtkRequisition  *requisition);
static void     btk_mirror_bin_size_allocate (BtkWidget       *widget,
                                               BtkAllocation   *allocation);
static bboolean btk_mirror_bin_damage        (BtkWidget       *widget,
                                               BdkEventExpose  *event);
static bboolean btk_mirror_bin_expose        (BtkWidget       *widget,
                                               BdkEventExpose  *offscreen);

static void     btk_mirror_bin_add           (BtkContainer    *container,
                                               BtkWidget       *child);
static void     btk_mirror_bin_remove        (BtkContainer    *container,
                                               BtkWidget       *widget);
static void     btk_mirror_bin_forall        (BtkContainer    *container,
                                               bboolean         include_internals,
                                               BtkCallback      callback,
                                               bpointer         callback_data);
static GType    btk_mirror_bin_child_type    (BtkContainer    *container);

G_DEFINE_TYPE (BtkMirrorBin, btk_mirror_bin, BTK_TYPE_CONTAINER);

static void
to_child (BtkMirrorBin *bin,
          double         widget_x,
          double         widget_y,
          double        *x_out,
          double        *y_out)
{
  *x_out = widget_x;
  *y_out = widget_y;
}

static void
to_parent (BtkMirrorBin *bin,
           double         offscreen_x,
           double         offscreen_y,
           double        *x_out,
           double        *y_out)
{
  *x_out = offscreen_x;
  *y_out = offscreen_y;
}

static void
btk_mirror_bin_class_init (BtkMirrorBinClass *klass)
{
  BtkWidgetClass *widget_class = BTK_WIDGET_CLASS (klass);
  BtkContainerClass *container_class = BTK_CONTAINER_CLASS (klass);

  widget_class->realize = btk_mirror_bin_realize;
  widget_class->unrealize = btk_mirror_bin_unrealize;
  widget_class->size_request = btk_mirror_bin_size_request;
  widget_class->size_allocate = btk_mirror_bin_size_allocate;
  widget_class->expose_event = btk_mirror_bin_expose;

  g_signal_override_class_closure (g_signal_lookup ("damage-event", BTK_TYPE_WIDGET),
                                   BTK_TYPE_MIRROR_BIN,
                                   g_cclosure_new (G_CALLBACK (btk_mirror_bin_damage),
                                                   NULL, NULL));

  container_class->add = btk_mirror_bin_add;
  container_class->remove = btk_mirror_bin_remove;
  container_class->forall = btk_mirror_bin_forall;
  container_class->child_type = btk_mirror_bin_child_type;
}

static void
btk_mirror_bin_init (BtkMirrorBin *bin)
{
  btk_widget_set_has_window (BTK_WIDGET (bin), TRUE);
}

BtkWidget *
btk_mirror_bin_new (void)
{
  return g_object_new (BTK_TYPE_MIRROR_BIN, NULL);
}

static BdkWindow *
pick_offscreen_child (BdkWindow     *offscreen_window,
                      double         widget_x,
                      double         widget_y,
                      BtkMirrorBin *bin)
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
                            BtkMirrorBin *bin)
{
  to_parent (bin, offscreen_x, offscreen_y, parent_x, parent_y);
}

static void
offscreen_window_from_parent (BdkWindow     *window,
                              double         parent_x,
                              double         parent_y,
                              double        *offscreen_x,
                              double        *offscreen_y,
                              BtkMirrorBin *bin)
{
  to_child (bin, parent_x, parent_y, offscreen_x, offscreen_y);
}

static void
btk_mirror_bin_realize (BtkWidget *widget)
{
  BtkMirrorBin *bin = BTK_MIRROR_BIN (widget);
  BdkWindowAttr attributes;
  BdkWindow *bdk_window;
  bint attributes_mask;
  bint border_width;
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
  
  bdk_window = bdk_window_new (btk_widget_get_parent_window (widget),
                               &attributes, attributes_mask);
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
btk_mirror_bin_unrealize (BtkWidget *widget)
{
  BtkMirrorBin *bin = BTK_MIRROR_BIN (widget);

  bdk_window_set_user_data (bin->offscreen_window, NULL);
  bdk_window_destroy (bin->offscreen_window);
  bin->offscreen_window = NULL;

  BTK_WIDGET_CLASS (btk_mirror_bin_parent_class)->unrealize (widget);
}

static GType
btk_mirror_bin_child_type (BtkContainer *container)
{
  BtkMirrorBin *bin = BTK_MIRROR_BIN (container);

  if (bin->child)
    return B_TYPE_NONE;

  return BTK_TYPE_WIDGET;
}

static void
btk_mirror_bin_add (BtkContainer *container,
                     BtkWidget    *widget)
{
  BtkMirrorBin *bin = BTK_MIRROR_BIN (container);

  if (!bin->child)
    {
      btk_widget_set_parent_window (widget, bin->offscreen_window);
      btk_widget_set_parent (widget, BTK_WIDGET (bin));
      bin->child = widget;
    }
  else
    g_warning ("BtkMirrorBin cannot have more than one child\n");
}

static void
btk_mirror_bin_remove (BtkContainer *container,
                        BtkWidget    *widget)
{
  BtkMirrorBin *bin = BTK_MIRROR_BIN (container);
  bboolean was_visible;

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
btk_mirror_bin_forall (BtkContainer *container,
                        bboolean      include_internals,
                        BtkCallback   callback,
                        bpointer      callback_data)
{
  BtkMirrorBin *bin = BTK_MIRROR_BIN (container);

  g_return_if_fail (callback != NULL);

  if (bin->child)
    (*callback) (bin->child, callback_data);
}

static void
btk_mirror_bin_size_request (BtkWidget      *widget,
                              BtkRequisition *requisition)
{
  BtkMirrorBin *bin = BTK_MIRROR_BIN (widget);
  BtkRequisition child_requisition;
  bint border_width;
  
  border_width = btk_container_get_border_width (BTK_CONTAINER (widget));

  child_requisition.width = 0;
  child_requisition.height = 0;

  if (bin->child && btk_widget_get_visible (bin->child))
    btk_widget_size_request (bin->child, &child_requisition);

  requisition->width = border_width * 2 + child_requisition.width + 10;
  requisition->height = border_width * 2 + child_requisition.height * 2 + 10;
}

static void
btk_mirror_bin_size_allocate (BtkWidget     *widget,
                               BtkAllocation *allocation)
{
  BtkMirrorBin *bin = BTK_MIRROR_BIN (widget);
  bint border_width;
  bint w, h;
  
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

      btk_widget_get_child_requisition (bin->child, &child_requisition);
      child_allocation.x = 0;
      child_allocation.y = 0;
      child_allocation.height = child_requisition.height;
      child_allocation.width = child_requisition.width;

      if (btk_widget_get_realized (widget))
        bdk_window_move_resize (bin->offscreen_window,
                                allocation->x + border_width,
                                allocation->y + border_width,
                                child_allocation.width, child_allocation.height);
      btk_widget_size_allocate (bin->child, &child_allocation);
    }
}

static bboolean
btk_mirror_bin_damage (BtkWidget      *widget,
                        BdkEventExpose *event)
{
  bdk_window_invalidate_rect (btk_widget_get_window (widget), NULL, FALSE);

  return TRUE;
}

static bboolean
btk_mirror_bin_expose (BtkWidget      *widget,
                        BdkEventExpose *event)
{
  BtkMirrorBin *bin = BTK_MIRROR_BIN (widget);
  bint width, height;

  if (btk_widget_is_drawable (widget))
    {
      if (event->window == btk_widget_get_window (widget))
        {
          BdkPixmap *pixmap;
          bairo_t *cr;
          bairo_matrix_t matrix;
          bairo_pattern_t *mask;

          if (bin->child && btk_widget_get_visible (bin->child))
            {
              pixmap = bdk_offscreen_window_get_pixmap (bin->offscreen_window);
              bdk_pixmap_get_size (pixmap, &width, &height);

              cr = bdk_bairo_create (btk_widget_get_window (widget));

              bairo_save (cr);

              bairo_rectangle (cr, 0, 0, width, height);
              bairo_clip (cr);

              /* paint the offscreen child */
              bdk_bairo_set_source_pixmap (cr, pixmap, 0, 0);
              bairo_paint (cr);

              bairo_restore (cr);

              bairo_matrix_init (&matrix, 1.0, 0.0, 0.3, 1.0, 0.0, 0.0);
              bairo_matrix_scale (&matrix, 1.0, -1.0);
              bairo_matrix_translate (&matrix, -10, - 3 * height - 10);
              bairo_transform (cr, &matrix);

              bairo_rectangle (cr, 0, height, width, height);
              bairo_clip (cr);

              bdk_bairo_set_source_pixmap (cr, pixmap, 0, height);

              /* create linear gradient as mask-pattern to fade out the source */
              mask = bairo_pattern_create_linear (0.0, height, 0.0, 2*height);
              bairo_pattern_add_color_stop_rgba (mask, 0.0,  0.0, 0.0, 0.0, 0.0);
              bairo_pattern_add_color_stop_rgba (mask, 0.25, 0.0, 0.0, 0.0, 0.01);
              bairo_pattern_add_color_stop_rgba (mask, 0.5,  0.0, 0.0, 0.0, 0.25);
              bairo_pattern_add_color_stop_rgba (mask, 0.75, 0.0, 0.0, 0.0, 0.5);
              bairo_pattern_add_color_stop_rgba (mask, 1.0,  0.0, 0.0, 0.0, 1.0);

              /* paint the reflection */
              bairo_mask (cr, mask);

              bairo_pattern_destroy (mask);
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

static BtkWidget *window = NULL;

BtkWidget *
do_offscreen_window2 (BtkWidget *do_widget)
{
  if (!window)
    {
      BtkWidget *bin, *vbox;
      BtkWidget *hbox, *entry, *applybutton, *backbutton;
      BtkSizeGroup *group;

      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_screen (BTK_WINDOW (window),
                             btk_widget_get_screen (do_widget));
      btk_window_set_title (BTK_WINDOW (window), "Effects");

      g_signal_connect (window, "destroy",
                        G_CALLBACK (btk_widget_destroyed), &window);

      btk_container_set_border_width (BTK_CONTAINER (window), 10);

      vbox = btk_vbox_new (0, FALSE);

      bin = btk_mirror_bin_new ();

      group = btk_size_group_new (BTK_SIZE_GROUP_VERTICAL);

      hbox = btk_hbox_new (FALSE, 6);
      backbutton = btk_button_new ();
      btk_container_add (BTK_CONTAINER (backbutton),
                         btk_image_new_from_stock (BTK_STOCK_GO_BACK, 4));
      btk_size_group_add_widget (group, backbutton);
      entry = btk_entry_new ();
      btk_size_group_add_widget (group, entry);
      applybutton = btk_button_new ();
      btk_size_group_add_widget (group, applybutton);
      btk_container_add (BTK_CONTAINER (applybutton),
                         btk_image_new_from_stock (BTK_STOCK_APPLY, 4));

      btk_container_add (BTK_CONTAINER (window), vbox);
      btk_box_pack_start (BTK_BOX (vbox), bin, TRUE, TRUE, 0);
      btk_container_add (BTK_CONTAINER (bin), hbox);
      btk_box_pack_start (BTK_BOX (hbox), backbutton, FALSE, FALSE, 0);
      btk_box_pack_start (BTK_BOX (hbox), entry, TRUE, TRUE, 0);
      btk_box_pack_start (BTK_BOX (hbox), applybutton, FALSE, FALSE, 0);
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

