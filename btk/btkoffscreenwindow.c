/*
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
 *
 * Authors: Cody Russell <crussell@canonical.com>
 *          Alexander Larsson <alexl@redhat.com>
 */

#undef BDK_DISABLE_DEPRECATED /* We need bdk_drawable_get_size() */

#include "btkoffscreenwindow.h"
#include "btkalias.h"

/**
 * SECTION:btkoffscreenwindow
 * @short_description: A toplevel container widget used to manage offscreen
 *    rendering of child widgets.
 * @title: BtkOffscreenWindow
 *
 * #BtkOffscreenWindow is strictly intended to be used for obtaining
 * snapshots of widgets that are not part of a normal widget hierarchy.
 * It differs from btk_widget_get_snapshot() in that the widget you
 * want to get a snapshot of need not be displayed on the user's screen
 * as a part of a widget hierarchy.  However, since #BtkOffscreenWindow
 * is a toplevel widget you cannot obtain snapshots of a full window
 * with it since you cannot pack a toplevel widget in another toplevel.
 *
 * The idea is to take a widget and manually set the state of it,
 * add it to a #BtkOffscreenWindow and then retrieve the snapshot
 * as a #BdkPixmap or #BdkPixbuf.
 *
 * #BtkOffscreenWindow derives from #BtkWindow only as an implementation
 * detail.  Applications should not use any API specific to #BtkWindow
 * to operate on this object.  It should be treated as a #BtkBin that
 * has no parent widget.
 *
 * When contained offscreen widgets are redrawn, #BtkOffscreenWindow
 * will emit a #BtkWidget::damage-event signal.
 */

G_DEFINE_TYPE (BtkOffscreenWindow, btk_offscreen_window, BTK_TYPE_WINDOW);

static void
btk_offscreen_window_size_request (BtkWidget *widget,
                                   BtkRequisition *requisition)
{
  BtkBin *bin = BTK_BIN (widget);
  bint border_width;
  bint default_width, default_height;

  border_width = btk_container_get_border_width (BTK_CONTAINER (widget));

  requisition->width = border_width * 2;
  requisition->height = border_width * 2;

  if (bin->child && btk_widget_get_visible (bin->child))
    {
      BtkRequisition child_req;

      btk_widget_size_request (bin->child, &child_req);

      requisition->width += child_req.width;
      requisition->height += child_req.height;
    }

  btk_window_get_default_size (BTK_WINDOW (widget),
                               &default_width, &default_height);
  if (default_width > 0)
    requisition->width = default_width;

  if (default_height > 0)
    requisition->height = default_height;
}

static void
btk_offscreen_window_size_allocate (BtkWidget *widget,
                                    BtkAllocation *allocation)
{
  BtkBin *bin = BTK_BIN (widget);
  bint border_width;

  widget->allocation = *allocation;

  border_width = btk_container_get_border_width (BTK_CONTAINER (widget));

  if (btk_widget_get_realized (widget))
    bdk_window_move_resize (widget->window,
                            allocation->x,
                            allocation->y,
                            allocation->width,
                            allocation->height);

  if (bin->child && btk_widget_get_visible (bin->child))
    {
      BtkAllocation  child_alloc;

      child_alloc.x = border_width;
      child_alloc.y = border_width;
      child_alloc.width = allocation->width - 2 * border_width;
      child_alloc.height = allocation->height - 2 * border_width;

      btk_widget_size_allocate (bin->child, &child_alloc);
    }

  btk_widget_queue_draw (widget);
}

static void
btk_offscreen_window_realize (BtkWidget *widget)
{
  BtkBin *bin;
  BdkWindowAttr attributes;
  bint attributes_mask;
  bint border_width;

  bin = BTK_BIN (widget);

  btk_widget_set_realized (widget, TRUE);

  border_width = btk_container_get_border_width (BTK_CONTAINER (widget));

  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.window_type = BDK_WINDOW_OFFSCREEN;
  attributes.event_mask = btk_widget_get_events (widget) | BDK_EXPOSURE_MASK;
  attributes.visual = btk_widget_get_visual (widget);
  attributes.colormap = btk_widget_get_colormap (widget);
  attributes.wclass = BDK_INPUT_OUTPUT;

  attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_VISUAL | BDK_WA_COLORMAP;

  widget->window = bdk_window_new (btk_widget_get_parent_window (widget),
                                   &attributes, attributes_mask);
  bdk_window_set_user_data (widget->window, widget);

  if (bin->child)
    btk_widget_set_parent_window (bin->child, widget->window);

  widget->style = btk_style_attach (widget->style, widget->window);

  btk_style_set_background (widget->style, widget->window, BTK_STATE_NORMAL);
}

static void
btk_offscreen_window_resize (BtkWidget *widget)
{
  BtkAllocation allocation = { 0, 0 };
  BtkRequisition requisition;

  btk_widget_size_request (widget, &requisition);

  allocation.width  = requisition.width;
  allocation.height = requisition.height;
  btk_widget_size_allocate (widget, &allocation);
}

static void
move_focus (BtkWidget       *widget,
            BtkDirectionType dir)
{
  btk_widget_child_focus (widget, dir);

  if (!BTK_CONTAINER (widget)->focus_child)
    btk_window_set_focus (BTK_WINDOW (widget), NULL);
}

static void
btk_offscreen_window_show (BtkWidget *widget)
{
  bboolean need_resize;
  BtkContainer *container;

  BTK_WIDGET_SET_FLAGS (widget, BTK_VISIBLE);

  container = BTK_CONTAINER (widget);
  need_resize = container->need_resize || !btk_widget_get_realized (widget);
  container->need_resize = FALSE;

  if (need_resize)
    btk_offscreen_window_resize (widget);

  btk_widget_map (widget);

  /* Try to make sure that we have some focused widget */
  if (!btk_window_get_focus (BTK_WINDOW (widget)))
    move_focus (widget, BTK_DIR_TAB_FORWARD);
}

static void
btk_offscreen_window_hide (BtkWidget *widget)
{
  BTK_WIDGET_UNSET_FLAGS (widget, BTK_VISIBLE);
  btk_widget_unmap (widget);
}

static void
btk_offscreen_window_check_resize (BtkContainer *container)
{
  BtkWidget *widget = BTK_WIDGET (container);

  if (btk_widget_get_visible (widget))
    btk_offscreen_window_resize (widget);
}

static void
btk_offscreen_window_class_init (BtkOffscreenWindowClass *class)
{
  BtkWidgetClass *widget_class;
  BtkContainerClass *container_class;

  widget_class = BTK_WIDGET_CLASS (class);
  container_class = BTK_CONTAINER_CLASS (class);

  widget_class->realize = btk_offscreen_window_realize;
  widget_class->show = btk_offscreen_window_show;
  widget_class->hide = btk_offscreen_window_hide;
  widget_class->size_request = btk_offscreen_window_size_request;
  widget_class->size_allocate = btk_offscreen_window_size_allocate;

  container_class->check_resize = btk_offscreen_window_check_resize;
}

static void
btk_offscreen_window_init (BtkOffscreenWindow *window)
{
}

/* --- functions --- */
/**
 * btk_offscreen_window_new:
 *
 * Creates a toplevel container widget that is used to retrieve
 * snapshots of widgets without showing them on the screen.  For
 * widgets that are on the screen and part of a normal widget
 * hierarchy, btk_widget_get_snapshot() can be used instead.
 *
 * Return value: A pointer to a #BtkWidget
 *
 * Since: 2.20
 */
BtkWidget *
btk_offscreen_window_new (void)
{
  return g_object_new (btk_offscreen_window_get_type (), NULL);
}

/**
 * btk_offscreen_window_get_pixmap:
 * @offscreen: the #BtkOffscreenWindow contained widget.
 *
 * Retrieves a snapshot of the contained widget in the form of
 * a #BdkPixmap.  If you need to keep this around over window
 * resizes then you should add a reference to it.
 *
 * Returns: (transfer none): A #BdkPixmap pointer to the offscreen pixmap,
 *     or %NULL.
 *
 * Since: 2.20
 */
BdkPixmap *
btk_offscreen_window_get_pixmap (BtkOffscreenWindow *offscreen)
{
  g_return_val_if_fail (BTK_IS_OFFSCREEN_WINDOW (offscreen), NULL);

  return bdk_offscreen_window_get_pixmap (BTK_WIDGET (offscreen)->window);
}

/**
 * btk_offscreen_window_get_pixbuf:
 * @offscreen: the #BtkOffscreenWindow contained widget.
 *
 * Retrieves a snapshot of the contained widget in the form of
 * a #BdkPixbuf.  This is a new pixbuf with a reference count of 1,
 * and the application should unreference it once it is no longer
 * needed.
 *
 * Returns: (transfer full): A #BdkPixbuf pointer, or %NULL.
 *
 * Since: 2.20
 */
BdkPixbuf *
btk_offscreen_window_get_pixbuf (BtkOffscreenWindow *offscreen)
{
  BdkPixmap *pixmap = NULL;
  BdkPixbuf *pixbuf = NULL;

  g_return_val_if_fail (BTK_IS_OFFSCREEN_WINDOW (offscreen), NULL);

  pixmap = bdk_offscreen_window_get_pixmap (BTK_WIDGET (offscreen)->window);

  if (pixmap != NULL)
    {
      bint width, height;

      bdk_drawable_get_size (pixmap, &width, &height);

      pixbuf = bdk_pixbuf_get_from_drawable (NULL, pixmap, NULL,
                                             0, 0, 0, 0,
                                             width, height);
    }

  return pixbuf;
}

#define __BTK_OFFSCREEN_WINDOW_C__
#include "btkaliasdef.c"
