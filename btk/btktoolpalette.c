/* BtkToolPalette -- A tool palette with categories and DnD support
 * Copyright (C) 2008  Openismus GmbH
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Authors:
 *      Mathias Hasselmann
 */

#include "config.h"

#include <string.h>
#include <btk/btk.h>

#include "btktoolpaletteprivate.h"
#include "btkmarshalers.h"

#include "btkprivate.h"
#include "btkintl.h"
#include "btkalias.h"

#define DEFAULT_ICON_SIZE       BTK_ICON_SIZE_SMALL_TOOLBAR
#define DEFAULT_ORIENTATION     BTK_ORIENTATION_VERTICAL
#define DEFAULT_TOOLBAR_STYLE   BTK_TOOLBAR_ICONS

#define DEFAULT_CHILD_EXCLUSIVE FALSE
#define DEFAULT_CHILD_EXPAND    FALSE

/**
 * SECTION:btktoolpalette
 * @Short_description: A tool palette with categories
 * @Title: BtkToolPalette
 *
 * A #BtkToolPalette allows you to add #BtkToolItem<!-- -->s to a palette-like
 * container with different categories and drag and drop support.
 *
 * A #BtkToolPalette is created with a call to btk_tool_palette_new().
 *
 * #BtkToolItem<!-- -->s cannot be added directly to a #BtkToolPalette - 
 * instead they are added to a #BtkToolItemGroup which can than be added
 * to a #BtkToolPalette. To add a #BtkToolItemGroup to a #BtkToolPalette,
 * use btk_container_add().
 *
 * |[
 * BtkWidget *palette, *group;
 * BtkToolItem *item;
 *
 * palette = btk_tool_palette_new ();
 * group = btk_tool_item_group_new (_("Test Category"));
 * btk_container_add (BTK_CONTAINER (palette), group);
 *
 * item = btk_tool_button_new_from_stock (BTK_STOCK_OK);
 * btk_tool_item_group_insert (BTK_TOOL_ITEM_GROUP (group), item, -1);
 * ]|
 *
 * The easiest way to use drag and drop with #BtkToolPalette is to call
 * btk_tool_palette_add_drag_dest() with the desired drag source @palette
 * and the desired drag target @widget. Then btk_tool_palette_get_drag_item()
 * can be used to get the dragged item in the #BtkWidget::drag-data-received
 * signal handler of the drag target.
 *
 * |[
 * static void
 * passive_canvas_drag_data_received (BtkWidget        *widget,
 *                                    BdkDragContext   *context,
 *                                    gint              x,
 *                                    gint              y,
 *                                    BtkSelectionData *selection,
 *                                    guint             info,
 *                                    guint             time,
 *                                    gpointer          data)
 * {
 *   BtkWidget *palette;
 *   BtkWidget *item;
 *
 *   /<!-- -->* Get the dragged item *<!-- -->/
 *   palette = btk_widget_get_ancestor (btk_drag_get_source_widget (context),
 *                                      BTK_TYPE_TOOL_PALETTE);
 *   if (palette != NULL)
 *     item = btk_tool_palette_get_drag_item (BTK_TOOL_PALETTE (palette),
 *                                            selection);
 *
 *   /<!-- -->* Do something with item *<!-- -->/
 * }
 *
 * BtkWidget *target, palette;
 *
 * palette = btk_tool_palette_new ();
 * target = btk_drawing_area_new ();
 *
 * g_signal_connect (G_OBJECT (target), "drag-data-received",
 *                   G_CALLBACK (passive_canvas_drag_data_received), NULL);
 * btk_tool_palette_add_drag_dest (BTK_TOOL_PALETTE (palette), target,
 *                                 BTK_DEST_DEFAULT_ALL,
 *                                 BTK_TOOL_PALETTE_DRAG_ITEMS,
 *                                 BDK_ACTION_COPY);
 * ]|
 *
 * Since: 2.20
 */

typedef struct _BtkToolItemGroupInfo   BtkToolItemGroupInfo;
typedef struct _BtkToolPaletteDragData BtkToolPaletteDragData;

enum
{
  PROP_NONE,
  PROP_ICON_SIZE,
  PROP_ICON_SIZE_SET,
  PROP_ORIENTATION,
  PROP_TOOLBAR_STYLE,
};

enum
{
  CHILD_PROP_NONE,
  CHILD_PROP_EXCLUSIVE,
  CHILD_PROP_EXPAND,
};

struct _BtkToolItemGroupInfo
{
  BtkToolItemGroup *widget;

  guint             notify_collapsed;
  guint             pos;
  guint             exclusive : 1;
  guint             expand : 1;
};

struct _BtkToolPalettePrivate
{
  GPtrArray* groups;

  BtkAdjustment        *hadjustment;
  BtkAdjustment        *vadjustment;

  BtkIconSize           icon_size;
  gboolean              icon_size_set;
  BtkOrientation        orientation;
  BtkToolbarStyle       style;
  gboolean              style_set;

  BtkWidget            *expanding_child;

  BtkSizeGroup         *text_size_group;

  BtkSettings       *settings;
  gulong             settings_connection;

  guint                 drag_source : 2;
};

struct _BtkToolPaletteDragData
{
  BtkToolPalette *palette;
  BtkWidget      *item;
};

static BdkAtom dnd_target_atom_item = BDK_NONE;
static BdkAtom dnd_target_atom_group = BDK_NONE;

static const BtkTargetEntry dnd_targets[] =
{
  { "application/x-btk-tool-palette-item", BTK_TARGET_SAME_APP, 0 },
  { "application/x-btk-tool-palette-group", BTK_TARGET_SAME_APP, 0 },
};

G_DEFINE_TYPE_WITH_CODE (BtkToolPalette,
               btk_tool_palette,
               BTK_TYPE_CONTAINER,
               G_IMPLEMENT_INTERFACE (BTK_TYPE_ORIENTABLE, NULL));

static void
btk_tool_palette_init (BtkToolPalette *palette)
{
  palette->priv = G_TYPE_INSTANCE_GET_PRIVATE (palette,
                                               BTK_TYPE_TOOL_PALETTE,
                                               BtkToolPalettePrivate);

  palette->priv->groups = g_ptr_array_sized_new (4);
  g_ptr_array_set_free_func (palette->priv->groups, g_free);

  palette->priv->icon_size = DEFAULT_ICON_SIZE;
  palette->priv->icon_size_set = FALSE;
  palette->priv->orientation = DEFAULT_ORIENTATION;
  palette->priv->style = DEFAULT_TOOLBAR_STYLE;
  palette->priv->style_set = FALSE;

  palette->priv->text_size_group = btk_size_group_new (BTK_SIZE_GROUP_BOTH);
}

static void
btk_tool_palette_reconfigured (BtkToolPalette *palette)
{
  guint i;

  for (i = 0; i < palette->priv->groups->len; ++i)
    {
      BtkToolItemGroupInfo *info = g_ptr_array_index (palette->priv->groups, i);
      if (info->widget)
        _btk_tool_item_group_palette_reconfigured (info->widget);
    }

  btk_widget_queue_resize_no_redraw (BTK_WIDGET (palette));
}

static void
btk_tool_palette_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  BtkToolPalette *palette = BTK_TOOL_PALETTE (object);

  switch (prop_id)
    {
      case PROP_ICON_SIZE:
        if ((guint) g_value_get_enum (value) != palette->priv->icon_size)
          {
            palette->priv->icon_size = g_value_get_enum (value);
            btk_tool_palette_reconfigured (palette);
          }
        break;

      case PROP_ICON_SIZE_SET:
        if ((guint) g_value_get_enum (value) != palette->priv->icon_size)
          {
            palette->priv->icon_size_set = g_value_get_enum (value);
            btk_tool_palette_reconfigured (palette);
          }
        break;

      case PROP_ORIENTATION:
        if ((guint) g_value_get_enum (value) != palette->priv->orientation)
          {
            palette->priv->orientation = g_value_get_enum (value);
            btk_tool_palette_reconfigured (palette);
          }
        break;

      case PROP_TOOLBAR_STYLE:
        if ((guint) g_value_get_enum (value) != palette->priv->style)
          {
            palette->priv->style = g_value_get_enum (value);
            btk_tool_palette_reconfigured (palette);
          }
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
btk_tool_palette_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  BtkToolPalette *palette = BTK_TOOL_PALETTE (object);

  switch (prop_id)
    {
      case PROP_ICON_SIZE:
        g_value_set_enum (value, btk_tool_palette_get_icon_size (palette));
        break;

      case PROP_ICON_SIZE_SET:
        g_value_set_boolean (value, palette->priv->icon_size_set);
        break;

      case PROP_ORIENTATION:
        g_value_set_enum (value, palette->priv->orientation);
        break;

      case PROP_TOOLBAR_STYLE:
        g_value_set_enum (value, btk_tool_palette_get_style (palette));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
btk_tool_palette_dispose (GObject *object)
{
  BtkToolPalette *palette = BTK_TOOL_PALETTE (object);
  guint i;

  if (palette->priv->hadjustment)
    {
      g_object_unref (palette->priv->hadjustment);
      palette->priv->hadjustment = NULL;
    }

  if (palette->priv->vadjustment)
    {
      g_object_unref (palette->priv->vadjustment);
      palette->priv->vadjustment = NULL;
    }

  for (i = 0; i < palette->priv->groups->len; ++i)
    {
      BtkToolItemGroupInfo *group = g_ptr_array_index (palette->priv->groups, i);

      if (group->notify_collapsed)
        {
          g_signal_handler_disconnect (group->widget, group->notify_collapsed);
          group->notify_collapsed = 0;
        }
    }

  if (palette->priv->text_size_group)
    {
      g_object_unref (palette->priv->text_size_group);
      palette->priv->text_size_group = NULL;
    }

  G_OBJECT_CLASS (btk_tool_palette_parent_class)->dispose (object);
}

static void
btk_tool_palette_finalize (GObject *object)
{
  BtkToolPalette *palette = BTK_TOOL_PALETTE (object);

  g_ptr_array_free (palette->priv->groups, TRUE);

  G_OBJECT_CLASS (btk_tool_palette_parent_class)->finalize (object);
}

static void
btk_tool_palette_size_request (BtkWidget      *widget,
                               BtkRequisition *requisition)
{
  const gint border_width = BTK_CONTAINER (widget)->border_width;
  BtkToolPalette *palette = BTK_TOOL_PALETTE (widget);
  BtkRequisition child_requisition;
  guint i;

  requisition->width = 0;
  requisition->height = 0;

  for (i = 0; i < palette->priv->groups->len; ++i)
    {
      BtkToolItemGroupInfo *group = g_ptr_array_index (palette->priv->groups, i);

      if (!group->widget)
        continue;

      btk_widget_size_request (BTK_WIDGET (group->widget), &child_requisition);

      if (BTK_ORIENTATION_VERTICAL == palette->priv->orientation)
        {
          requisition->width = MAX (requisition->width, child_requisition.width);
          requisition->height += child_requisition.height;
        }
      else
        {
          requisition->width += child_requisition.width;
          requisition->height = MAX (requisition->height, child_requisition.height);
        }
    }

  requisition->width += border_width * 2;
  requisition->height += border_width * 2;
}

static void
btk_tool_palette_size_allocate (BtkWidget     *widget,
                                BtkAllocation *allocation)
{
  const gint border_width = BTK_CONTAINER (widget)->border_width;
  BtkToolPalette *palette = BTK_TOOL_PALETTE (widget);
  BtkAdjustment *adjustment = NULL;
  BtkAllocation child_allocation;

  gint n_expand_groups = 0;
  gint remaining_space = 0;
  gint expand_space = 0;

  gint page_start, page_size = 0;
  gint offset = 0;
  guint i;

  gint min_offset = -1, max_offset = -1;

  gint x;

  gint *group_sizes = g_newa (gint, palette->priv->groups->len);

  BtkTextDirection direction = btk_widget_get_direction (widget);

  BTK_WIDGET_CLASS (btk_tool_palette_parent_class)->size_allocate (widget, allocation);

  if (BTK_ORIENTATION_VERTICAL == palette->priv->orientation)
    {
      adjustment = palette->priv->vadjustment;
      page_size = allocation->height;
    }
  else
    {
      adjustment = palette->priv->hadjustment;
      page_size = allocation->width;
    }

  if (adjustment)
    offset = btk_adjustment_get_value (adjustment);
  if (BTK_ORIENTATION_HORIZONTAL == palette->priv->orientation &&
      BTK_TEXT_DIR_RTL == direction)
    offset = -offset;

  if (BTK_ORIENTATION_VERTICAL == palette->priv->orientation)
    child_allocation.width = allocation->width - border_width * 2;
  else
    child_allocation.height = allocation->height - border_width * 2;

  if (BTK_ORIENTATION_VERTICAL == palette->priv->orientation)
    remaining_space = allocation->height;
  else
    remaining_space = allocation->width;

  /* figure out the required size of all groups to be able to distribute the
   * remaining space on allocation
   */
  for (i = 0; i < palette->priv->groups->len; ++i)
    {
      BtkToolItemGroupInfo *group = g_ptr_array_index (palette->priv->groups, i);
      gint size;

      if (!group->widget)
        continue;

      widget = BTK_WIDGET (group->widget);

      if (btk_tool_item_group_get_n_items (group->widget))
        {
          if (BTK_ORIENTATION_VERTICAL == palette->priv->orientation)
            size = _btk_tool_item_group_get_height_for_width (group->widget, child_allocation.width);
          else
            size = _btk_tool_item_group_get_width_for_height (group->widget, child_allocation.height);

          if (group->expand && !btk_tool_item_group_get_collapsed (group->widget))
            n_expand_groups += 1;
        }
      else
        size = 0;

      remaining_space -= size;
      group_sizes[i] = size;

      /* if the widget is currently expanding an offset which allows to
       * display as much of the widget as possible is calculated
       */
      if (widget == palette->priv->expanding_child)
        {
          gint limit =
            BTK_ORIENTATION_VERTICAL == palette->priv->orientation ?
            child_allocation.width : child_allocation.height;

          gint real_size;
          guint j;

          min_offset = 0;

          for (j = 0; j < i; ++j)
            min_offset += group_sizes[j];

          max_offset = min_offset + group_sizes[i];

          real_size = _btk_tool_item_group_get_size_for_limit
            (BTK_TOOL_ITEM_GROUP (widget), limit,
             BTK_ORIENTATION_VERTICAL == palette->priv->orientation,
             FALSE);

          if (size == real_size)
            palette->priv->expanding_child = NULL;
        }
    }

  if (n_expand_groups > 0)
    {
      remaining_space = MAX (0, remaining_space);
      expand_space = remaining_space / n_expand_groups;
    }

  if (max_offset != -1)
    {
      gint limit =
        BTK_ORIENTATION_VERTICAL == palette->priv->orientation ?
        allocation->height : allocation->width;

      offset = MIN (MAX (offset, max_offset - limit), min_offset);
    }

  if (remaining_space > 0)
    offset = 0;

  x = border_width;
  child_allocation.y = border_width;

  if (BTK_ORIENTATION_VERTICAL == palette->priv->orientation)
    child_allocation.y -= offset;
  else
    x -= offset;

  /* allocate all groups at the calculated positions */
  for (i = 0; i < palette->priv->groups->len; ++i)
    {
      BtkToolItemGroupInfo *group = g_ptr_array_index (palette->priv->groups, i);
      BtkWidget *widget;

      if (!group->widget)
        continue;

      widget = BTK_WIDGET (group->widget);

      if (btk_tool_item_group_get_n_items (group->widget))
        {
          gint size = group_sizes[i];

          if (group->expand && !btk_tool_item_group_get_collapsed (group->widget))
            {
              size += MIN (expand_space, remaining_space);
              remaining_space -= expand_space;
            }

          if (BTK_ORIENTATION_VERTICAL == palette->priv->orientation)
            child_allocation.height = size;
          else
            child_allocation.width = size;

          if (BTK_ORIENTATION_HORIZONTAL == palette->priv->orientation &&
              BTK_TEXT_DIR_RTL == direction)
            child_allocation.x = allocation->width - x - child_allocation.width;
          else
            child_allocation.x = x;

          btk_widget_size_allocate (widget, &child_allocation);
          btk_widget_show (widget);

          if (BTK_ORIENTATION_VERTICAL == palette->priv->orientation)
            child_allocation.y += child_allocation.height;
          else
            x += child_allocation.width;
        }
      else
        btk_widget_hide (widget);
    }

  if (BTK_ORIENTATION_VERTICAL == palette->priv->orientation)
    {
      child_allocation.y += border_width;
      child_allocation.y += offset;

      page_start = child_allocation.y;
    }
  else
    {
      x += border_width;
      x += offset;

      page_start = x;
    }

  /* update the scrollbar to match the displayed adjustment */
  if (adjustment)
    {
      gdouble value;

      adjustment->page_increment = page_size * 0.9;
      adjustment->step_increment = page_size * 0.1;
      adjustment->page_size = page_size;

      if (BTK_ORIENTATION_VERTICAL == palette->priv->orientation ||
          BTK_TEXT_DIR_LTR == direction)
        {
          adjustment->lower = 0;
          adjustment->upper = MAX (0, page_start);

          value = MIN (offset, adjustment->upper - adjustment->page_size);
          btk_adjustment_clamp_page (adjustment, value, offset + page_size);
        }
      else
        {
          adjustment->lower = page_size - MAX (0, page_start);
          adjustment->upper = page_size;

          offset = -offset;

          value = MAX (offset, adjustment->lower);
          btk_adjustment_clamp_page (adjustment, offset, value + page_size);
        }

      btk_adjustment_changed (adjustment);
    }
}

static gboolean
btk_tool_palette_expose_event (BtkWidget      *widget,
                               BdkEventExpose *event)
{
  BtkToolPalette *palette = BTK_TOOL_PALETTE (widget);
  BdkDisplay *display;
  bairo_t *cr;
  guint i;

  display = bdk_window_get_display (widget->window);

  if (!bdk_display_supports_composite (display))
    return FALSE;

  cr = bdk_bairo_create (widget->window);
  bdk_bairo_rebunnyion (cr, event->rebunnyion);
  bairo_clip (cr);

  bairo_push_group (cr);

  for (i = 0; i < palette->priv->groups->len; ++i)
  {
    BtkToolItemGroupInfo *info = g_ptr_array_index (palette->priv->groups, i);
    if (info->widget)
      _btk_tool_item_group_paint (info->widget, cr);
  }

  bairo_pop_group_to_source (cr);

  bairo_paint (cr);
  bairo_destroy (cr);

  return FALSE;
}

static void
btk_tool_palette_realize (BtkWidget *widget)
{
  const gint border_width = BTK_CONTAINER (widget)->border_width;
  gint attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_VISUAL | BDK_WA_COLORMAP;
  BdkWindowAttr attributes;

  attributes.window_type = BDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x + border_width;
  attributes.y = widget->allocation.y + border_width;
  attributes.width = widget->allocation.width - border_width * 2;
  attributes.height = widget->allocation.height - border_width * 2;
  attributes.wclass = BDK_INPUT_OUTPUT;
  attributes.visual = btk_widget_get_visual (widget);
  attributes.colormap = btk_widget_get_colormap (widget);
  attributes.event_mask = btk_widget_get_events (widget)
                         | BDK_VISIBILITY_NOTIFY_MASK | BDK_EXPOSURE_MASK
                         | BDK_BUTTON_PRESS_MASK | BDK_BUTTON_RELEASE_MASK
                         | BDK_BUTTON_MOTION_MASK;

  widget->window = bdk_window_new (btk_widget_get_parent_window (widget),
                                   &attributes, attributes_mask);

  bdk_window_set_user_data (widget->window, widget);
  widget->style = btk_style_attach (widget->style, widget->window);
  btk_style_set_background (widget->style, widget->window, BTK_STATE_NORMAL);
  btk_widget_set_realized (widget, TRUE);

  btk_container_forall (BTK_CONTAINER (widget),
                        (BtkCallback) btk_widget_set_parent_window,
                        widget->window);

  btk_widget_queue_resize_no_redraw (widget);
}

static void
btk_tool_palette_adjustment_value_changed (BtkAdjustment *adjustment,
                                           gpointer       data)
{
  BtkWidget *widget = BTK_WIDGET (data);
  btk_tool_palette_size_allocate (widget, &widget->allocation);
}

static void
btk_tool_palette_set_scroll_adjustments (BtkWidget     *widget,
                                         BtkAdjustment *hadjustment,
                                         BtkAdjustment *vadjustment)
{
  BtkToolPalette *palette = BTK_TOOL_PALETTE (widget);

  if (hadjustment)
    g_object_ref_sink (hadjustment);
  if (vadjustment)
    g_object_ref_sink (vadjustment);

  if (palette->priv->hadjustment)
    g_object_unref (palette->priv->hadjustment);
  if (palette->priv->vadjustment)
    g_object_unref (palette->priv->vadjustment);

  palette->priv->hadjustment = hadjustment;
  palette->priv->vadjustment = vadjustment;

  if (palette->priv->hadjustment)
    g_signal_connect (palette->priv->hadjustment, "value-changed",
                      G_CALLBACK (btk_tool_palette_adjustment_value_changed),
                      palette);
  if (palette->priv->vadjustment)
    g_signal_connect (palette->priv->vadjustment, "value-changed",
                      G_CALLBACK (btk_tool_palette_adjustment_value_changed),
                      palette);
}

static void
btk_tool_palette_add (BtkContainer *container,
                      BtkWidget    *child)
{
  BtkToolPalette *palette;
  BtkToolItemGroupInfo *info = g_new0(BtkToolItemGroupInfo, 1);

  g_return_if_fail (BTK_IS_TOOL_PALETTE (container));
  g_return_if_fail (BTK_IS_TOOL_ITEM_GROUP (child));

  palette = BTK_TOOL_PALETTE (container);

  g_ptr_array_add (palette->priv->groups, info);
  info->pos = palette->priv->groups->len - 1;
  info->widget = g_object_ref_sink (child);

  btk_widget_set_parent (child, BTK_WIDGET (palette));
}

static void
btk_tool_palette_remove (BtkContainer *container,
                         BtkWidget    *child)
{
  BtkToolPalette *palette;
  guint i;

  g_return_if_fail (BTK_IS_TOOL_PALETTE (container));
  palette = BTK_TOOL_PALETTE (container);

  for (i = 0; i < palette->priv->groups->len; ++i)
    {
      BtkToolItemGroupInfo *info = g_ptr_array_index (palette->priv->groups, i);
      if (BTK_WIDGET(info->widget) == child)
        {
          g_object_unref (child);
          btk_widget_unparent (child);

          g_ptr_array_remove_index (palette->priv->groups, i);
        }
    }
}

static void
btk_tool_palette_forall (BtkContainer *container,
                         gboolean      internals,
                         BtkCallback   callback,
                         gpointer      callback_data)
{
  BtkToolPalette *palette = BTK_TOOL_PALETTE (container);
  guint i;


  for (i = 0; i < palette->priv->groups->len; ++i)
    {
      BtkToolItemGroupInfo *info = g_ptr_array_index (palette->priv->groups, i);
      if (info->widget)
        callback (BTK_WIDGET (info->widget),
                  callback_data);
    }
}

static GType
btk_tool_palette_child_type (BtkContainer *container)
{
  return BTK_TYPE_TOOL_ITEM_GROUP;
}

static void
btk_tool_palette_set_child_property (BtkContainer *container,
                                     BtkWidget    *child,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  BtkToolPalette *palette = BTK_TOOL_PALETTE (container);

  switch (prop_id)
    {
      case CHILD_PROP_EXCLUSIVE:
        btk_tool_palette_set_exclusive (palette, BTK_TOOL_ITEM_GROUP (child), 
          g_value_get_boolean (value));
        break;

      case CHILD_PROP_EXPAND:
        btk_tool_palette_set_expand (palette, BTK_TOOL_ITEM_GROUP (child), 
          g_value_get_boolean (value));
        break;

      default:
        BTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, prop_id, pspec);
        break;
    }
}

static void
btk_tool_palette_get_child_property (BtkContainer *container,
                                     BtkWidget    *child,
                                     guint         prop_id,
                                     GValue       *value,
                                     GParamSpec   *pspec)
{
  BtkToolPalette *palette = BTK_TOOL_PALETTE (container);

  switch (prop_id)
    {
      case CHILD_PROP_EXCLUSIVE:
        g_value_set_boolean (value, 
          btk_tool_palette_get_exclusive (palette, BTK_TOOL_ITEM_GROUP (child)));
        break;

      case CHILD_PROP_EXPAND:
        g_value_set_boolean (value, 
          btk_tool_palette_get_expand (palette, BTK_TOOL_ITEM_GROUP (child)));
        break;

      default:
        BTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, prop_id, pspec);
        break;
    }
}

static void
style_change_notify (BtkToolPalette *palette)
{
  BtkToolPalettePrivate* priv = palette->priv;

  if (!priv->style_set)
    {
      /* pretend it was set, then unset, thus reverting to new default */
      priv->style_set = TRUE;
      btk_tool_palette_unset_style (palette);
    }
}

static void
icon_size_change_notify (BtkToolPalette *palette)
{
  BtkToolPalettePrivate* priv = palette->priv;

  if (!priv->icon_size_set)
    {
      /* pretend it was set, then unset, thus reverting to new default */
      priv->icon_size_set = TRUE;
      btk_tool_palette_unset_icon_size (palette);
    }
}

static void
btk_tool_palette_settings_change_notify (BtkSettings      *settings,
                                         const GParamSpec *pspec,
                                         BtkToolPalette   *palette)
{
  if (strcmp (pspec->name, "btk-toolbar-style") == 0)
    style_change_notify (palette);
  else if (strcmp (pspec->name, "btk-toolbar-icon-size") == 0)
    icon_size_change_notify (palette);
}

static void
btk_tool_palette_screen_changed (BtkWidget *widget,
                                 BdkScreen *previous_screen)
{
  BtkToolPalette *palette = BTK_TOOL_PALETTE (widget);
  BtkToolPalettePrivate* priv = palette->priv;
  BtkSettings *old_settings = priv->settings;
  BtkSettings *settings;

  if (btk_widget_has_screen (BTK_WIDGET (palette)))
    settings = btk_widget_get_settings (BTK_WIDGET (palette));
  else
    settings = NULL;

  if (settings == old_settings)
    return;

  if (old_settings)
  {
    g_signal_handler_disconnect (old_settings, priv->settings_connection);
    g_object_unref (old_settings);
  }

  if (settings)
  {
    priv->settings_connection =
      g_signal_connect (settings, "notify",
                        G_CALLBACK (btk_tool_palette_settings_change_notify),
                        palette);
    priv->settings = g_object_ref (settings);
  }
  else
    priv->settings = NULL;

  btk_tool_palette_reconfigured (palette);
}


static void
btk_tool_palette_class_init (BtkToolPaletteClass *cls)
{
  GObjectClass      *oclass   = G_OBJECT_CLASS (cls);
  BtkWidgetClass    *wclass   = BTK_WIDGET_CLASS (cls);
  BtkContainerClass *cclass   = BTK_CONTAINER_CLASS (cls);

  oclass->set_property        = btk_tool_palette_set_property;
  oclass->get_property        = btk_tool_palette_get_property;
  oclass->dispose             = btk_tool_palette_dispose;
  oclass->finalize            = btk_tool_palette_finalize;

  wclass->size_request        = btk_tool_palette_size_request;
  wclass->size_allocate       = btk_tool_palette_size_allocate;
  wclass->expose_event        = btk_tool_palette_expose_event;
  wclass->realize             = btk_tool_palette_realize;

  cclass->add                 = btk_tool_palette_add;
  cclass->remove              = btk_tool_palette_remove;
  cclass->forall              = btk_tool_palette_forall;
  cclass->child_type          = btk_tool_palette_child_type;
  cclass->set_child_property  = btk_tool_palette_set_child_property;
  cclass->get_child_property  = btk_tool_palette_get_child_property;

  cls->set_scroll_adjustments = btk_tool_palette_set_scroll_adjustments;

  /* Handle screen-changed so we can update our BtkSettings.
   */
  wclass->screen_changed      = btk_tool_palette_screen_changed;

  /**
   * BtkToolPalette::set-scroll-adjustments:
   * @widget: the BtkToolPalette that received the signal
   * @hadjustment: The horizontal adjustment
   * @vadjustment: The vertical adjustment
   *
   * Set the scroll adjustments for the viewport.
   * Usually scrolled containers like BtkScrolledWindow will emit this
   * signal to connect two instances of BtkScrollbar to the scroll
   * directions of the BtkToolpalette.
   *
   * Since: 2.20
   */
  wclass->set_scroll_adjustments_signal =
    g_signal_new ("set-scroll-adjustments",
                  G_TYPE_FROM_CLASS (oclass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BtkToolPaletteClass, set_scroll_adjustments),
                  NULL, NULL,
                  _btk_marshal_VOID__OBJECT_OBJECT,
                  G_TYPE_NONE, 2,
                  BTK_TYPE_ADJUSTMENT,
                  BTK_TYPE_ADJUSTMENT);

  g_object_class_override_property (oclass, PROP_ORIENTATION, "orientation");

  /**
   * BtkToolPalette:icon-size:
   *
   * The size of the icons in a tool palette is normally determined by
   * the #BtkSettings:toolbar-icon-size setting. When this property is set,
   * it overrides the setting.
   *
   * This should only be used for special-purpose tool palettes, normal
   * application tool palettes should respect the user preferences for the
   * size of icons.
   *
   * Since: 2.20
   */
  g_object_class_install_property (oclass,
                                   PROP_ICON_SIZE,
                                   g_param_spec_enum ("icon-size",
                                                      P_("Icon size"),
                                                      P_("Size of icons in this tool palette"),
                                                      BTK_TYPE_ICON_SIZE,
                                                      DEFAULT_ICON_SIZE,
                                                      BTK_PARAM_READWRITE));

  /**
   * BtkToolPalette:icon-size-set:
   *
   * Is %TRUE if the #BtkToolPalette:icon-size property has been set.
   *
   * Since: 2.20
   */
  g_object_class_install_property (oclass,
                                   PROP_ICON_SIZE_SET,
                                   g_param_spec_boolean ("icon-size-set",
                                                      P_("Icon size set"),
                                                      P_("Whether the icon-size property has been set"),
                                                      FALSE,
                                                      BTK_PARAM_READWRITE));

  /**
   * BtkToolPalette:toolbar-style:
   *
   * The style of items in the tool palette.
   *
   * Since: 2.20
   */
  g_object_class_install_property (oclass, PROP_TOOLBAR_STYLE,
                                   g_param_spec_enum ("toolbar-style",
                                                      P_("Toolbar Style"),
                                                      P_("Style of items in the tool palette"),
                                                      BTK_TYPE_TOOLBAR_STYLE,
                                                      DEFAULT_TOOLBAR_STYLE,
                                                      BTK_PARAM_READWRITE));


  /**
   * BtkToolPalette:exclusive:
   *
   * Whether the item group should be the only one that is expanded
   * at a given time.
   *
   * Since: 2.20
   */
  btk_container_class_install_child_property (cclass, CHILD_PROP_EXCLUSIVE,
                                              g_param_spec_boolean ("exclusive",
                                                                    P_("Exclusive"),
                                                                    P_("Whether the item group should be the only expanded at a given time"),
                                                                    DEFAULT_CHILD_EXCLUSIVE,
                                                                    BTK_PARAM_READWRITE));

  /**
   * BtkToolPalette:expand:
   *
   * Whether the item group should receive extra space when the palette grows.
   * at a given time.
   *
   * Since: 2.20
   */
  btk_container_class_install_child_property (cclass, CHILD_PROP_EXPAND,
                                              g_param_spec_boolean ("expand",
                                                                    P_("Expand"),
                                                                    P_("Whether the item group should receive extra space when the palette grows"),
                                                                    DEFAULT_CHILD_EXPAND,
                                                                    BTK_PARAM_READWRITE));

  g_type_class_add_private (cls, sizeof (BtkToolPalettePrivate));

  dnd_target_atom_item = bdk_atom_intern_static_string (dnd_targets[0].target);
  dnd_target_atom_group = bdk_atom_intern_static_string (dnd_targets[1].target);
}

/**
 * btk_tool_palette_new:
 *
 * Creates a new tool palette.
 *
 * Returns: a new #BtkToolPalette
 *
 * Since: 2.20
 */
BtkWidget*
btk_tool_palette_new (void)
{
  return g_object_new (BTK_TYPE_TOOL_PALETTE, NULL);
}

/**
 * btk_tool_palette_set_icon_size:
 * @palette: a #BtkToolPalette
 * @icon_size: (type int): the #BtkIconSize that icons in the tool
 *     palette shall have
 *
 * Sets the size of icons in the tool palette.
 *
 * Since: 2.20
 */
void
btk_tool_palette_set_icon_size (BtkToolPalette *palette,
                                BtkIconSize     icon_size)
{
  BtkToolPalettePrivate *priv;

  g_return_if_fail (BTK_IS_TOOL_PALETTE (palette));
  g_return_if_fail (icon_size != BTK_ICON_SIZE_INVALID);

  priv = palette->priv;

  if (!priv->icon_size_set)
    {
      priv->icon_size_set = TRUE;
      g_object_notify (G_OBJECT (palette), "icon-size-set");
    }

  if (priv->icon_size == icon_size)
    return;

  priv->icon_size = icon_size;
  g_object_notify (G_OBJECT (palette), "icon-size");

  btk_tool_palette_reconfigured (palette);

  btk_widget_queue_resize (BTK_WIDGET (palette));
}

static BtkSettings *
toolpalette_get_settings (BtkToolPalette *palette)
{
  BtkToolPalettePrivate *priv = palette->priv;
  return priv->settings;
}

/**
 * btk_tool_palette_unset_icon_size:
 * @palette: a #BtkToolPalette
 *
 * Unsets the tool palette icon size set with btk_tool_palette_set_icon_size(),
 * so that user preferences will be used to determine the icon size.
 *
 * Since: 2.20
 */
void
btk_tool_palette_unset_icon_size (BtkToolPalette *palette)
{
  BtkToolPalettePrivate* priv = palette->priv;
  BtkIconSize size;

  g_return_if_fail (BTK_IS_TOOL_PALETTE (palette));

  if (palette->priv->icon_size_set)
    {
      BtkSettings *settings = toolpalette_get_settings (palette);

      if (settings)
        {
          g_object_get (settings,
            "btk-toolbar-icon-size", &size,
            NULL);
        }
      else
        size = DEFAULT_ICON_SIZE;

      if (size != palette->priv->icon_size)
      {
        btk_tool_palette_set_icon_size (palette, size);
        g_object_notify (G_OBJECT (palette), "icon-size");
	    }

      priv->icon_size_set = FALSE;
      g_object_notify (G_OBJECT (palette), "icon-size-set");
    }
}

/* Set the "toolbar-style" property and do appropriate things.
 * BtkToolbar does this by emitting a signal instead of just
 * calling a function...
 */
static void
btk_tool_palette_change_style (BtkToolPalette  *palette,
                               BtkToolbarStyle  style)
{
  BtkToolPalettePrivate* priv = palette->priv;

  if (priv->style != style)
    {
      priv->style = style;

      btk_tool_palette_reconfigured (palette);

      btk_widget_queue_resize (BTK_WIDGET (palette));
      g_object_notify (G_OBJECT (palette), "toolbar-style");
    }
}


/**
 * btk_tool_palette_set_style:
 * @palette: a #BtkToolPalette
 * @style: the #BtkToolbarStyle that items in the tool palette shall have
 *
 * Sets the style (text, icons or both) of items in the tool palette.
 *
 * Since: 2.20
 */
void
btk_tool_palette_set_style (BtkToolPalette  *palette,
                            BtkToolbarStyle  style)
{
  g_return_if_fail (BTK_IS_TOOL_PALETTE (palette));

  palette->priv->style_set = TRUE;
  btk_tool_palette_change_style (palette, style);
}


/**
 * btk_tool_palette_unset_style:
 * @palette: a #BtkToolPalette
 *
 * Unsets a toolbar style set with btk_tool_palette_set_style(),
 * so that user preferences will be used to determine the toolbar style.
 *
 * Since: 2.20
 */
void
btk_tool_palette_unset_style (BtkToolPalette *palette)
{
  BtkToolPalettePrivate* priv = palette->priv;
  BtkToolbarStyle style;

  g_return_if_fail (BTK_IS_TOOL_PALETTE (palette));

  if (priv->style_set)
    {
      BtkSettings *settings = toolpalette_get_settings (palette);

      if (settings)
        g_object_get (settings,
                      "btk-toolbar-style", &style,
                      NULL);
      else
        style = DEFAULT_TOOLBAR_STYLE;

      if (style != priv->style)
        btk_tool_palette_change_style (palette, style);

      priv->style_set = FALSE;
    }
}

/**
 * btk_tool_palette_get_icon_size:
 * @palette: a #BtkToolPalette
 *
 * Gets the size of icons in the tool palette.
 * See btk_tool_palette_set_icon_size().
 *
 * Returns: (type int): the #BtkIconSize of icons in the tool palette
 *
 * Since: 2.20
 */
BtkIconSize
btk_tool_palette_get_icon_size (BtkToolPalette *palette)
{
  g_return_val_if_fail (BTK_IS_TOOL_PALETTE (palette), DEFAULT_ICON_SIZE);

  return palette->priv->icon_size;
}

/**
 * btk_tool_palette_get_style:
 * @palette: a #BtkToolPalette
 *
 * Gets the style (icons, text or both) of items in the tool palette.
 *
 * Returns: the #BtkToolbarStyle of items in the tool palette.
 *
 * Since: 2.20
 */
BtkToolbarStyle
btk_tool_palette_get_style (BtkToolPalette *palette)
{
  g_return_val_if_fail (BTK_IS_TOOL_PALETTE (palette), DEFAULT_TOOLBAR_STYLE);

  return palette->priv->style;
}

gint
_btk_tool_palette_compare_groups (gconstpointer a,
                                  gconstpointer b)
{
  const BtkToolItemGroupInfo *group_a = a;
  const BtkToolItemGroupInfo *group_b = b;

  return group_a->pos - group_b->pos;
}

/**
 * btk_tool_palette_set_group_position:
 * @palette: a #BtkToolPalette
 * @group: a #BtkToolItemGroup which is a child of palette
 * @position: a new index for group
 *
 * Sets the position of the group as an index of the tool palette.
 * If position is 0 the group will become the first child, if position is
 * -1 it will become the last child.
 *
 * Since: 2.20
 */
void
btk_tool_palette_set_group_position (BtkToolPalette   *palette,
                                     BtkToolItemGroup *group,
                                     gint             position)
{
  BtkToolItemGroupInfo *group_new;
  BtkToolItemGroupInfo *group_old;
  gint old_position;

  g_return_if_fail (BTK_IS_TOOL_PALETTE (palette));
  g_return_if_fail (BTK_IS_TOOL_ITEM_GROUP (group));
  g_return_if_fail (position >= -1);

  if (-1 == position)
    position = palette->priv->groups->len - 1;

  g_return_if_fail ((guint) position < palette->priv->groups->len);

  group_new = g_ptr_array_index (palette->priv->groups, position);

  if (BTK_TOOL_ITEM_GROUP (group) == group_new->widget)
    return;

  old_position = btk_tool_palette_get_group_position (palette, group);
  g_return_if_fail (old_position >= 0);

  group_old = g_ptr_array_index (palette->priv->groups, old_position);

  group_new->pos = position;
  group_old->pos = old_position;

  g_ptr_array_sort (palette->priv->groups, _btk_tool_palette_compare_groups);

  btk_widget_queue_resize (BTK_WIDGET (palette));
}

static void
btk_tool_palette_group_notify_collapsed (BtkToolItemGroup *group,
                                         GParamSpec       *pspec,
                                         gpointer          data)
{
  BtkToolPalette *palette = BTK_TOOL_PALETTE (data);
  guint i;

  if (btk_tool_item_group_get_collapsed (group))
    return;

  for (i = 0; i < palette->priv->groups->len; ++i)
    {
      BtkToolItemGroupInfo *info = g_ptr_array_index (palette->priv->groups, i);
      BtkToolItemGroup *current_group = info->widget;

      if (current_group && current_group != group)
        btk_tool_item_group_set_collapsed (current_group, TRUE);
    }
}

/**
 * btk_tool_palette_set_exclusive:
 * @palette: a #BtkToolPalette
 * @group: a #BtkToolItemGroup which is a child of palette
 * @exclusive: whether the group should be exclusive or not
 *
 * Sets whether the group should be exclusive or not.
 * If an exclusive group is expanded all other groups are collapsed.
 *
 * Since: 2.20
 */
void
btk_tool_palette_set_exclusive (BtkToolPalette   *palette,
                                BtkToolItemGroup *group,
                                gboolean          exclusive)
{
  BtkToolItemGroupInfo *group_info;
  gint position;

  g_return_if_fail (BTK_IS_TOOL_PALETTE (palette));
  g_return_if_fail (BTK_IS_TOOL_ITEM_GROUP (group));

  position = btk_tool_palette_get_group_position (palette, group);
  g_return_if_fail (position >= 0);

  group_info = g_ptr_array_index (palette->priv->groups, position);

  if (exclusive == group_info->exclusive)
    return;

  group_info->exclusive = exclusive;

  if (group_info->exclusive != (0 != group_info->notify_collapsed))
    {
      if (group_info->exclusive)
        {
          group_info->notify_collapsed =
            g_signal_connect (group, "notify::collapsed",
                              G_CALLBACK (btk_tool_palette_group_notify_collapsed),
                              palette);
        }
      else
        {
          g_signal_handler_disconnect (group, group_info->notify_collapsed);
          group_info->notify_collapsed = 0;
        }
    }

  btk_tool_palette_group_notify_collapsed (group_info->widget, NULL, palette);
  btk_widget_child_notify (BTK_WIDGET (group), "exclusive");
}

/**
 * btk_tool_palette_set_expand:
 * @palette: a #BtkToolPalette
 * @group: a #BtkToolItemGroup which is a child of palette
 * @expand: whether the group should be given extra space
 *
 * Sets whether the group should be given extra space.
 *
 * Since: 2.20
 */
void
btk_tool_palette_set_expand (BtkToolPalette   *palette,
                             BtkToolItemGroup *group,
                             gboolean        expand)
{
  BtkToolItemGroupInfo *group_info;
  gint position;

  g_return_if_fail (BTK_IS_TOOL_PALETTE (palette));
  g_return_if_fail (BTK_IS_TOOL_ITEM_GROUP (group));

  position = btk_tool_palette_get_group_position (palette, group);
  g_return_if_fail (position >= 0);

  group_info = g_ptr_array_index (palette->priv->groups, position);

  if (expand != group_info->expand)
    {
      group_info->expand = expand;
      btk_widget_queue_resize (BTK_WIDGET (palette));
      btk_widget_child_notify (BTK_WIDGET (group), "expand");
    }
}

/**
 * btk_tool_palette_get_group_position:
 * @palette: a #BtkToolPalette
 * @group: a #BtkToolItemGroup
 *
 * Gets the position of @group in @palette as index.
 * See btk_tool_palette_set_group_position().
 *
 * Returns: the index of group or -1 if @group is not a child of @palette
 *
 * Since: 2.20
 */
gint
btk_tool_palette_get_group_position (BtkToolPalette   *palette,
                                     BtkToolItemGroup *group)
{
  guint i;

  g_return_val_if_fail (BTK_IS_TOOL_PALETTE (palette), -1);
  g_return_val_if_fail (BTK_IS_TOOL_ITEM_GROUP (group), -1);

  for (i = 0; i < palette->priv->groups->len; ++i)
    {
      BtkToolItemGroupInfo *info = g_ptr_array_index (palette->priv->groups, i);
      if ((gpointer) group == info->widget)
        return i;
    }

  return -1;
}

/**
 * btk_tool_palette_get_exclusive:
 * @palette: a #BtkToolPalette
 * @group: a #BtkToolItemGroup which is a child of palette
 *
 * Gets whether @group is exclusive or not.
 * See btk_tool_palette_set_exclusive().
 *
 * Returns: %TRUE if @group is exclusive
 *
 * Since: 2.20
 */
gboolean
btk_tool_palette_get_exclusive (BtkToolPalette   *palette,
                                BtkToolItemGroup *group)
{
  gint position;
  BtkToolItemGroupInfo *info;

  g_return_val_if_fail (BTK_IS_TOOL_PALETTE (palette), DEFAULT_CHILD_EXCLUSIVE);
  g_return_val_if_fail (BTK_IS_TOOL_ITEM_GROUP (group), DEFAULT_CHILD_EXCLUSIVE);

  position = btk_tool_palette_get_group_position (palette, group);
  g_return_val_if_fail (position >= 0, DEFAULT_CHILD_EXCLUSIVE);

  info = g_ptr_array_index (palette->priv->groups, position);

  return info->exclusive;
}

/**
 * btk_tool_palette_get_expand:
 * @palette: a #BtkToolPalette
 * @group: a #BtkToolItemGroup which is a child of palette
 *
 * Gets whether group should be given extra space.
 * See btk_tool_palette_set_expand().
 *
 * Returns: %TRUE if group should be given extra space, %FALSE otherwise
 *
 * Since: 2.20
 */
gboolean
btk_tool_palette_get_expand (BtkToolPalette   *palette,
                             BtkToolItemGroup *group)
{
  gint position;
  BtkToolItemGroupInfo *info;

  g_return_val_if_fail (BTK_IS_TOOL_PALETTE (palette), DEFAULT_CHILD_EXPAND);
  g_return_val_if_fail (BTK_IS_TOOL_ITEM_GROUP (group), DEFAULT_CHILD_EXPAND);

  position = btk_tool_palette_get_group_position (palette, group);
  g_return_val_if_fail (position >= 0, DEFAULT_CHILD_EXPAND);

  info = g_ptr_array_index (palette->priv->groups, position);

  return info->expand;
}

/**
 * btk_tool_palette_get_drop_item:
 * @palette: a #BtkToolPalette
 * @x: the x position
 * @y: the y position
 *
 * Gets the item at position (x, y).
 * See btk_tool_palette_get_drop_group().
 *
 * Returns: (transfer none): the #BtkToolItem at position or %NULL if there is no such item
 *
 * Since: 2.20
 */
BtkToolItem*
btk_tool_palette_get_drop_item (BtkToolPalette *palette,
                                gint            x,
                                gint            y)
{
  BtkToolItemGroup *group = btk_tool_palette_get_drop_group (palette, x, y);
  BtkWidget *widget = BTK_WIDGET (group);

  if (group)
    return btk_tool_item_group_get_drop_item (group,
                                              x - widget->allocation.x,
                                              y - widget->allocation.y);

  return NULL;
}

/**
 * btk_tool_palette_get_drop_group:
 * @palette: a #BtkToolPalette
 * @x: the x position
 * @y: the y position
 *
 * Gets the group at position (x, y).
 *
 * Returns: (transfer none): the #BtkToolItemGroup at position or %NULL
 *     if there is no such group
 *
 * Since: 2.20
 */
BtkToolItemGroup*
btk_tool_palette_get_drop_group (BtkToolPalette *palette,
                                 gint            x,
                                 gint            y)
{
  BtkAllocation *allocation;
  guint i;

  g_return_val_if_fail (BTK_IS_TOOL_PALETTE (palette), NULL);

  allocation = &BTK_WIDGET (palette)->allocation;

  g_return_val_if_fail (x >= 0 && x < allocation->width, NULL);
  g_return_val_if_fail (y >= 0 && y < allocation->height, NULL);

  for (i = 0; i < palette->priv->groups->len; ++i)
    {
      BtkToolItemGroupInfo *group = g_ptr_array_index (palette->priv->groups, i);
      BtkWidget *widget;
      gint x0, y0;

      if (!group->widget)
        continue;

      widget = BTK_WIDGET (group->widget);

      x0 = x - widget->allocation.x;
      y0 = y - widget->allocation.y;

      if (x0 >= 0 && x0 < widget->allocation.width &&
          y0 >= 0 && y0 < widget->allocation.height)
        return BTK_TOOL_ITEM_GROUP (widget);
    }

  return NULL;
}

/**
 * btk_tool_palette_get_drag_item:
 * @palette: a #BtkToolPalette
 * @selection: a #BtkSelectionData
 *
 * Get the dragged item from the selection.
 * This could be a #BtkToolItem or a #BtkToolItemGroup.
 *
 * Returns: (transfer none): the dragged item in selection
 *
 * Since: 2.20
 */
BtkWidget*
btk_tool_palette_get_drag_item (BtkToolPalette         *palette,
                                const BtkSelectionData *selection)
{
  BtkToolPaletteDragData *data;

  g_return_val_if_fail (BTK_IS_TOOL_PALETTE (palette), NULL);
  g_return_val_if_fail (NULL != selection, NULL);

  g_return_val_if_fail (selection->format == 8, NULL);
  g_return_val_if_fail (selection->length == sizeof (BtkToolPaletteDragData), NULL);
  g_return_val_if_fail (selection->target == dnd_target_atom_item ||
                        selection->target == dnd_target_atom_group,
                        NULL);

  data = (BtkToolPaletteDragData*) selection->data;

  g_return_val_if_fail (data->palette == palette, NULL);

  if (dnd_target_atom_item == selection->target)
    g_return_val_if_fail (BTK_IS_TOOL_ITEM (data->item), NULL);
  else if (dnd_target_atom_group == selection->target)
    g_return_val_if_fail (BTK_IS_TOOL_ITEM_GROUP (data->item), NULL);

  return data->item;
}

/**
 * btk_tool_palette_set_drag_source:
 * @palette: a #BtkToolPalette
 * @targets: the #BtkToolPaletteDragTarget<!-- -->s
 *     which the widget should support
 *
 * Sets the tool palette as a drag source.
 * Enables all groups and items in the tool palette as drag sources
 * on button 1 and button 3 press with copy and move actions.
 * See btk_drag_source_set().
 *
 * Since: 2.20
 */
void
btk_tool_palette_set_drag_source (BtkToolPalette            *palette,
                                  BtkToolPaletteDragTargets  targets)
{
  guint i;

  g_return_if_fail (BTK_IS_TOOL_PALETTE (palette));

  if ((palette->priv->drag_source & targets) == targets)
    return;

  palette->priv->drag_source |= targets;

  for (i = 0; i < palette->priv->groups->len; ++i)
    {
      BtkToolItemGroupInfo *info = g_ptr_array_index (palette->priv->groups, i);
      if (info->widget)
        btk_container_forall (BTK_CONTAINER (info->widget),
                              _btk_tool_palette_child_set_drag_source,
                              palette);
    }
}

/**
 * btk_tool_palette_add_drag_dest:
 * @palette: a #BtkToolPalette
 * @widget: a #BtkWidget which should be a drag destination for @palette
 * @flags: the flags that specify what actions BTK+ should take for drops
 *     on that widget
 * @targets: the #BtkToolPaletteDragTarget<!-- -->s which the widget
 *     should support
 * @actions: the #BdkDragAction<!-- -->s which the widget should suppport
 *
 * Sets @palette as drag source (see btk_tool_palette_set_drag_source())
 * and sets @widget as a drag destination for drags from @palette.
 * See btk_drag_dest_set().
 *
 * Since: 2.20
 */
void
btk_tool_palette_add_drag_dest (BtkToolPalette            *palette,
                                BtkWidget                 *widget,
                                BtkDestDefaults            flags,
                                BtkToolPaletteDragTargets  targets,
                                BdkDragAction              actions)
{
  BtkTargetEntry entries[G_N_ELEMENTS (dnd_targets)];
  gint n_entries = 0;

  g_return_if_fail (BTK_IS_TOOL_PALETTE (palette));
  g_return_if_fail (BTK_IS_WIDGET (widget));

  btk_tool_palette_set_drag_source (palette,
                                    targets);

  if (targets & BTK_TOOL_PALETTE_DRAG_ITEMS)
    entries[n_entries++] = dnd_targets[0];
  if (targets & BTK_TOOL_PALETTE_DRAG_GROUPS)
    entries[n_entries++] = dnd_targets[1];

  btk_drag_dest_set (widget, flags, entries, n_entries, actions);
}

void
_btk_tool_palette_get_item_size (BtkToolPalette *palette,
                                 BtkRequisition *item_size,
                                 gboolean        homogeneous_only,
                                 gint           *requested_rows)
{
  BtkRequisition max_requisition;
  gint max_rows;
  guint i;

  g_return_if_fail (BTK_IS_TOOL_PALETTE (palette));
  g_return_if_fail (NULL != item_size);

  max_requisition.width = 0;
  max_requisition.height = 0;
  max_rows = 0;

  /* iterate over all groups and calculate the max item_size and max row request */
  for (i = 0; i < palette->priv->groups->len; ++i)
    {
      BtkRequisition requisition;
      gint rows;
      BtkToolItemGroupInfo *group = g_ptr_array_index (palette->priv->groups, i);

      if (!group->widget)
        continue;

      _btk_tool_item_group_item_size_request (group->widget, &requisition, homogeneous_only, &rows);

      max_requisition.width = MAX (max_requisition.width, requisition.width);
      max_requisition.height = MAX (max_requisition.height, requisition.height);
      max_rows = MAX (max_rows, rows);
    }

  *item_size = max_requisition;
  if (requested_rows)
    *requested_rows = max_rows;
}

static void
btk_tool_palette_item_drag_data_get (BtkWidget        *widget,
                                     BdkDragContext   *context,
                                     BtkSelectionData *selection,
                                     guint             info,
                                     guint             time,
                                     gpointer          data)
{
  BtkToolPaletteDragData drag_data = { BTK_TOOL_PALETTE (data), NULL };

  if (selection->target == dnd_target_atom_item)
    drag_data.item = btk_widget_get_ancestor (widget, BTK_TYPE_TOOL_ITEM);

  if (drag_data.item)
    btk_selection_data_set (selection, selection->target, 8,
                            (guchar*) &drag_data, sizeof (drag_data));
}

static void
btk_tool_palette_child_drag_data_get (BtkWidget        *widget,
                                      BdkDragContext   *context,
                                      BtkSelectionData *selection,
                                      guint             info,
                                      guint             time,
                                      gpointer          data)
{
  BtkToolPaletteDragData drag_data = { BTK_TOOL_PALETTE (data), NULL };

  if (selection->target == dnd_target_atom_group)
    drag_data.item = btk_widget_get_ancestor (widget, BTK_TYPE_TOOL_ITEM_GROUP);

  if (drag_data.item)
    btk_selection_data_set (selection, selection->target, 8,
                            (guchar*) &drag_data, sizeof (drag_data));
}

void
_btk_tool_palette_child_set_drag_source (BtkWidget *child,
                                         gpointer   data)
{
  BtkToolPalette *palette = BTK_TOOL_PALETTE (data);

  /* Check drag_source,
   * to work properly when called from btk_tool_item_group_insert().
   */
  if (!palette->priv->drag_source)
    return;

  if (BTK_IS_TOOL_ITEM (child) &&
      (palette->priv->drag_source & BTK_TOOL_PALETTE_DRAG_ITEMS))
    {
      /* Connect to child instead of the item itself,
       * to work arround bug 510377.
       */
      if (BTK_IS_TOOL_BUTTON (child))
        child = btk_bin_get_child (BTK_BIN (child));

      if (!child)
        return;

      btk_drag_source_set (child, BDK_BUTTON1_MASK | BDK_BUTTON3_MASK,
                           &dnd_targets[0], 1, BDK_ACTION_COPY | BDK_ACTION_MOVE);

      g_signal_connect (child, "drag-data-get",
                        G_CALLBACK (btk_tool_palette_item_drag_data_get),
                        palette);
    }
  else if (BTK_IS_BUTTON (child) &&
           (palette->priv->drag_source & BTK_TOOL_PALETTE_DRAG_GROUPS))
    {
      btk_drag_source_set (child, BDK_BUTTON1_MASK | BDK_BUTTON3_MASK,
                           &dnd_targets[1], 1, BDK_ACTION_COPY | BDK_ACTION_MOVE);

      g_signal_connect (child, "drag-data-get",
                        G_CALLBACK (btk_tool_palette_child_drag_data_get),
                        palette);
    }
}

/**
 * btk_tool_palette_get_drag_target_item:
 *
 * Gets the target entry for a dragged #BtkToolItem.
 *
 * Returns: (transfer none): the #BtkTargetEntry for a dragged item.
 *
 * Since: 2.20
 */
const BtkTargetEntry*
btk_tool_palette_get_drag_target_item (void)
{
  return &dnd_targets[0];
}

/**
 * btk_tool_palette_get_drag_target_group:
 *
 * Get the target entry for a dragged #BtkToolItemGroup.
 *
 * Returns: (transfer none): the #BtkTargetEntry for a dragged group
 *
 * Since: 2.20
 */
const BtkTargetEntry*
btk_tool_palette_get_drag_target_group (void)
{
  return &dnd_targets[1];
}

void
_btk_tool_palette_set_expanding_child (BtkToolPalette *palette,
                                       BtkWidget      *widget)
{
  g_return_if_fail (BTK_IS_TOOL_PALETTE (palette));
  palette->priv->expanding_child = widget;
}

/**
 * btk_tool_palette_get_hadjustment:
 * @palette: a #BtkToolPalette
 *
 * Gets the horizontal adjustment of the tool palette.
 *
 * Returns: (transfer none): the horizontal adjustment of @palette
 *
 * Since: 2.20
 */
BtkAdjustment*
btk_tool_palette_get_hadjustment (BtkToolPalette *palette)
{
  g_return_val_if_fail (BTK_IS_TOOL_PALETTE (palette), NULL);

  return palette->priv->hadjustment;
}

/**
 * btk_tool_palette_get_vadjustment:
 * @palette: a #BtkToolPalette
 *
 * Gets the vertical adjustment of the tool palette.
 *
 * Returns: (transfer none): the vertical adjustment of @palette
 *
 * Since: 2.20
 */
BtkAdjustment*
btk_tool_palette_get_vadjustment (BtkToolPalette *palette)
{
  g_return_val_if_fail (BTK_IS_TOOL_PALETTE (palette), NULL);

  return palette->priv->vadjustment;
}

BtkSizeGroup *
_btk_tool_palette_get_size_group (BtkToolPalette *palette)
{
  g_return_val_if_fail (BTK_IS_TOOL_PALETTE (palette), NULL);

  return palette->priv->text_size_group;
}


#define __BTK_TOOL_PALETTE_C__
#include "btkaliasdef.c"
