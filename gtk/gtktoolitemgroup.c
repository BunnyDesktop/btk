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
 *      Jan Arne Petersen
 */

#include "config.h"

#include "btktoolpaletteprivate.h"

#include <btk/btk.h>
#include <math.h>
#include <string.h>
#include "btkprivate.h"
#include "btkintl.h"
#include "btkalias.h"

#define ANIMATION_TIMEOUT        50
#define ANIMATION_DURATION      (ANIMATION_TIMEOUT * 4)
#define DEFAULT_ANIMATION_STATE  TRUE
#define DEFAULT_EXPANDER_SIZE    16
#define DEFAULT_HEADER_SPACING   2

#define DEFAULT_LABEL            ""
#define DEFAULT_COLLAPSED        FALSE
#define DEFAULT_ELLIPSIZE        BANGO_ELLIPSIZE_NONE

/**
 * SECTION:btktoolitemgroup
 * @Short_description: A sub container used in a tool palette
 * @Title: BtkToolItemGroup
 *
 * A #BtkToolItemGroup is used together with #BtkToolPalette to add
 * #BtkToolItem<!-- -->s to a palette like container with different
 * categories and drag and drop support.
 *
 * Since: 2.20
 */

enum
{
  PROP_NONE,
  PROP_LABEL,
  PROP_LABEL_WIDGET,
  PROP_COLLAPSED,
  PROP_ELLIPSIZE,
  PROP_RELIEF
};

enum
{
  CHILD_PROP_NONE,
  CHILD_PROP_HOMOGENEOUS,
  CHILD_PROP_EXPAND,
  CHILD_PROP_FILL,
  CHILD_PROP_NEW_ROW,
  CHILD_PROP_POSITION,
};

typedef struct _BtkToolItemGroupChild BtkToolItemGroupChild;

struct _BtkToolItemGroupPrivate
{
  BtkWidget         *header;
  BtkWidget         *label_widget;

  GList             *children;

  gboolean           animation;
  gint64             animation_start;
  GSource           *animation_timeout;
  BtkExpanderStyle   expander_style;
  gint               expander_size;
  gint               header_spacing;
  BangoEllipsizeMode ellipsize;

  gulong             focus_set_id;
  BtkWidget         *toplevel;

  BtkSettings       *settings;
  gulong             settings_connection;

  guint              collapsed : 1;
};

struct _BtkToolItemGroupChild
{
  BtkToolItem *item;

  guint        homogeneous : 1;
  guint        expand : 1;
  guint        fill : 1;
  guint        new_row : 1;
};

static void btk_tool_item_group_tool_shell_init (BtkToolShellIface *iface);

G_DEFINE_TYPE_WITH_CODE (BtkToolItemGroup, btk_tool_item_group, BTK_TYPE_CONTAINER,
G_IMPLEMENT_INTERFACE (BTK_TYPE_TOOL_SHELL, btk_tool_item_group_tool_shell_init));

static BtkWidget*
btk_tool_item_group_get_alignment (BtkToolItemGroup *group)
{
  return btk_bin_get_child (BTK_BIN (group->priv->header));
}

static BtkOrientation
btk_tool_item_group_get_orientation (BtkToolShell *shell)
{
  BtkWidget *parent = btk_widget_get_parent (BTK_WIDGET (shell));

  if (BTK_IS_TOOL_PALETTE (parent))
    return btk_orientable_get_orientation (BTK_ORIENTABLE (parent));

  return BTK_ORIENTATION_VERTICAL;
}

static BtkToolbarStyle
btk_tool_item_group_get_style (BtkToolShell *shell)
{
  BtkWidget *parent = btk_widget_get_parent (BTK_WIDGET (shell));

  if (BTK_IS_TOOL_PALETTE (parent))
    return btk_tool_palette_get_style (BTK_TOOL_PALETTE (parent));

  return BTK_TOOLBAR_ICONS;
}

static BtkIconSize
btk_tool_item_group_get_icon_size (BtkToolShell *shell)
{
  BtkWidget *parent = btk_widget_get_parent (BTK_WIDGET (shell));

  if (BTK_IS_TOOL_PALETTE (parent))
    return btk_tool_palette_get_icon_size (BTK_TOOL_PALETTE (parent));

  return BTK_ICON_SIZE_SMALL_TOOLBAR;
}

static BangoEllipsizeMode
btk_tool_item_group_get_ellipsize_mode (BtkToolShell *shell)
{
  return BTK_TOOL_ITEM_GROUP (shell)->priv->ellipsize;
}

static gfloat
btk_tool_item_group_get_text_alignment (BtkToolShell *shell)
{
  if (BTK_TOOLBAR_TEXT == btk_tool_item_group_get_style (shell) ||
      BTK_TOOLBAR_BOTH_HORIZ == btk_tool_item_group_get_style (shell))
    return 0.0;

  return 0.5;
}

static BtkOrientation
btk_tool_item_group_get_text_orientation (BtkToolShell *shell)
{
  return BTK_ORIENTATION_HORIZONTAL;
}

static BtkSizeGroup *
btk_tool_item_group_get_text_size_group (BtkToolShell *shell)
{
  BtkWidget *parent = btk_widget_get_parent (BTK_WIDGET (shell));

  if (BTK_IS_TOOL_PALETTE (parent))
    return _btk_tool_palette_get_size_group (BTK_TOOL_PALETTE (parent));

  return NULL;
}

static void
animation_change_notify (BtkToolItemGroup *group)
{
  BtkSettings *settings = group->priv->settings;
  gboolean animation;

  if (settings)
    g_object_get (settings,
                  "btk-enable-animations", &animation,
                  NULL);
  else
    animation = DEFAULT_ANIMATION_STATE;

  group->priv->animation = animation;
}

static void
btk_tool_item_group_settings_change_notify (BtkSettings      *settings,
                                            const GParamSpec *pspec,
                                            BtkToolItemGroup *group)
{
  if (strcmp (pspec->name, "btk-enable-animations") == 0)
    animation_change_notify (group);
}

static void
btk_tool_item_group_screen_changed (BtkWidget *widget,
                                    BdkScreen *previous_screen)
{
  BtkToolItemGroup *group = BTK_TOOL_ITEM_GROUP (widget);
  BtkToolItemGroupPrivate* priv = group->priv;
  BtkSettings *old_settings = priv->settings;
  BtkSettings *settings;

  if (btk_widget_has_screen (BTK_WIDGET (group)))
    settings = btk_widget_get_settings (BTK_WIDGET (group));
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
                        G_CALLBACK (btk_tool_item_group_settings_change_notify),
                        group);
    priv->settings = g_object_ref (settings);
  }
  else
    priv->settings = NULL;

  animation_change_notify (group);
}

static void
btk_tool_item_group_tool_shell_init (BtkToolShellIface *iface)
{
  iface->get_icon_size = btk_tool_item_group_get_icon_size;
  iface->get_orientation = btk_tool_item_group_get_orientation;
  iface->get_style = btk_tool_item_group_get_style;
  iface->get_text_alignment = btk_tool_item_group_get_text_alignment;
  iface->get_text_orientation = btk_tool_item_group_get_text_orientation;
  iface->get_text_size_group = btk_tool_item_group_get_text_size_group;
  iface->get_ellipsize_mode = btk_tool_item_group_get_ellipsize_mode;
}

static gboolean
btk_tool_item_group_header_expose_event_cb (BtkWidget      *widget,
                                            BdkEventExpose *event,
                                            gpointer        data)
{
  BtkToolItemGroup *group = BTK_TOOL_ITEM_GROUP (data);
  BtkToolItemGroupPrivate* priv = group->priv;
  BtkExpanderStyle expander_style;
  BtkOrientation orientation;
  gint x, y;
  BtkTextDirection direction;

  orientation = btk_tool_shell_get_orientation (BTK_TOOL_SHELL (group));
  expander_style = priv->expander_style;
  direction = btk_widget_get_direction (widget);

  if (BTK_ORIENTATION_VERTICAL == orientation)
    {
      if (BTK_TEXT_DIR_RTL == direction)
        x = widget->allocation.x + widget->allocation.width - priv->expander_size / 2;
      else
        x = widget->allocation.x + priv->expander_size / 2;
      y = widget->allocation.y + widget->allocation.height / 2;
    }
  else
    {
      x = widget->allocation.x + widget->allocation.width / 2;
      y = widget->allocation.y + priv->expander_size / 2;

      /* Unfortunatly btk_paint_expander() doesn't support rotated drawing
       * modes. Luckily the following shady arithmetics produce the desired
       * result. */
      expander_style = BTK_EXPANDER_EXPANDED - expander_style;
    }

  btk_paint_expander (widget->style, widget->window,
                      priv->header->state,
                      &event->area, BTK_WIDGET (group),
                      "tool-palette-header", x, y,
                      expander_style);

  return FALSE;
}

static void
btk_tool_item_group_header_size_request_cb (BtkWidget      *widget,
                                            BtkRequisition *requisition,
                                            gpointer        data)
{
  BtkToolItemGroup *group = BTK_TOOL_ITEM_GROUP (data);
  requisition->height = MAX (requisition->height, group->priv->expander_size);
}

static void
btk_tool_item_group_header_clicked_cb (BtkButton *button,
                                       gpointer   data)
{
  BtkToolItemGroup *group = BTK_TOOL_ITEM_GROUP (data);
  BtkToolItemGroupPrivate* priv = group->priv;
  BtkWidget *parent = btk_widget_get_parent (data);

  if (priv->collapsed ||
      !BTK_IS_TOOL_PALETTE (parent) ||
      !btk_tool_palette_get_exclusive (BTK_TOOL_PALETTE (parent), data))
    btk_tool_item_group_set_collapsed (group, !priv->collapsed);
}

static void
btk_tool_item_group_header_adjust_style (BtkToolItemGroup *group)
{
  BtkWidget *alignment = btk_tool_item_group_get_alignment (group);
  BtkWidget *label_widget = btk_bin_get_child (BTK_BIN (alignment));
  BtkWidget *widget = BTK_WIDGET (group);
  BtkToolItemGroupPrivate* priv = group->priv;
  gint dx = 0, dy = 0;
  BtkTextDirection direction = btk_widget_get_direction (widget);

  btk_widget_style_get (widget,
                        "header-spacing", &(priv->header_spacing),
                        "expander-size", &(priv->expander_size),
                        NULL);

  switch (btk_tool_shell_get_orientation (BTK_TOOL_SHELL (group)))
    {
      case BTK_ORIENTATION_HORIZONTAL:
        dy = priv->header_spacing + priv->expander_size;

        if (BTK_IS_LABEL (label_widget))
          {
            btk_label_set_ellipsize (BTK_LABEL (label_widget), BANGO_ELLIPSIZE_NONE);
            if (BTK_TEXT_DIR_RTL == direction)
              btk_label_set_angle (BTK_LABEL (label_widget), -90);
            else
              btk_label_set_angle (BTK_LABEL (label_widget), 90);
          }
       break;

      case BTK_ORIENTATION_VERTICAL:
        dx = priv->header_spacing + priv->expander_size;

        if (BTK_IS_LABEL (label_widget))
          {
            btk_label_set_ellipsize (BTK_LABEL (label_widget), priv->ellipsize);
            btk_label_set_angle (BTK_LABEL (label_widget), 0);
          }
        break;
    }

  btk_alignment_set_padding (BTK_ALIGNMENT (alignment), dy, 0, dx, 0);
}

static void
btk_tool_item_group_init (BtkToolItemGroup *group)
{
  BtkWidget *alignment;
  BtkToolItemGroupPrivate* priv;

  btk_widget_set_redraw_on_allocate (BTK_WIDGET (group), FALSE);

  group->priv = priv = G_TYPE_INSTANCE_GET_PRIVATE (group,
                                             BTK_TYPE_TOOL_ITEM_GROUP,
                                             BtkToolItemGroupPrivate);

  priv->children = NULL;
  priv->header_spacing = DEFAULT_HEADER_SPACING;
  priv->expander_size = DEFAULT_EXPANDER_SIZE;
  priv->expander_style = BTK_EXPANDER_EXPANDED;

  priv->label_widget = btk_label_new (NULL);
  btk_misc_set_alignment (BTK_MISC (priv->label_widget), 0.0, 0.5);
  alignment = btk_alignment_new (0.5, 0.5, 1.0, 1.0);
  btk_container_add (BTK_CONTAINER (alignment), priv->label_widget);
  btk_widget_show_all (alignment);

  btk_widget_push_composite_child ();
  priv->header = btk_button_new ();
  btk_widget_set_composite_name (priv->header, "header");
  btk_widget_pop_composite_child ();

  g_object_ref_sink (priv->header);
  btk_button_set_focus_on_click (BTK_BUTTON (priv->header), FALSE);
  btk_container_add (BTK_CONTAINER (priv->header), alignment);
  btk_widget_set_parent (priv->header, BTK_WIDGET (group));

  btk_tool_item_group_header_adjust_style (group);

  g_signal_connect_after (alignment, "expose-event",
                          G_CALLBACK (btk_tool_item_group_header_expose_event_cb),
                          group);
  g_signal_connect_after (alignment, "size-request",
                          G_CALLBACK (btk_tool_item_group_header_size_request_cb),
                          group);

  g_signal_connect (priv->header, "clicked",
                    G_CALLBACK (btk_tool_item_group_header_clicked_cb),
                    group);
}

static void
btk_tool_item_group_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  BtkToolItemGroup *group = BTK_TOOL_ITEM_GROUP (object);

  switch (prop_id)
    {
      case PROP_LABEL:
        btk_tool_item_group_set_label (group, g_value_get_string (value));
        break;

      case PROP_LABEL_WIDGET:
        btk_tool_item_group_set_label_widget (group, g_value_get_object (value));
	break;

      case PROP_COLLAPSED:
        btk_tool_item_group_set_collapsed (group, g_value_get_boolean (value));
        break;

      case PROP_ELLIPSIZE:
        btk_tool_item_group_set_ellipsize (group, g_value_get_enum (value));
        break;

      case PROP_RELIEF:
        btk_tool_item_group_set_header_relief (group, g_value_get_enum(value));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
btk_tool_item_group_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  BtkToolItemGroup *group = BTK_TOOL_ITEM_GROUP (object);

  switch (prop_id)
    {
      case PROP_LABEL:
        g_value_set_string (value, btk_tool_item_group_get_label (group));
        break;

      case PROP_LABEL_WIDGET:
        g_value_set_object (value,
			    btk_tool_item_group_get_label_widget (group));
        break;

      case PROP_COLLAPSED:
        g_value_set_boolean (value, btk_tool_item_group_get_collapsed (group));
        break;

      case PROP_ELLIPSIZE:
        g_value_set_enum (value, btk_tool_item_group_get_ellipsize (group));
        break;

      case PROP_RELIEF:
        g_value_set_enum (value, btk_tool_item_group_get_header_relief (group));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
btk_tool_item_group_finalize (GObject *object)
{
  BtkToolItemGroup *group = BTK_TOOL_ITEM_GROUP (object);

  if (group->priv->children)
    {
      g_list_free (group->priv->children);
      group->priv->children = NULL;
    }

  G_OBJECT_CLASS (btk_tool_item_group_parent_class)->finalize (object);
}

static void
btk_tool_item_group_dispose (GObject *object)
{
  BtkToolItemGroup *group = BTK_TOOL_ITEM_GROUP (object);
  BtkToolItemGroupPrivate* priv = group->priv;

  if (priv->toplevel)
    {
      /* disconnect focus tracking handler */
      g_signal_handler_disconnect (priv->toplevel,
                                   priv->focus_set_id);

      priv->focus_set_id = 0;
      priv->toplevel = NULL;
    }

  G_OBJECT_CLASS (btk_tool_item_group_parent_class)->dispose (object);
}

static void
btk_tool_item_group_get_item_size (BtkToolItemGroup *group,
                                   BtkRequisition   *item_size,
                                   gboolean          homogeneous_only,
                                   gint             *requested_rows)
{
  BtkWidget *parent = btk_widget_get_parent (BTK_WIDGET (group));

  if (BTK_IS_TOOL_PALETTE (parent))
    _btk_tool_palette_get_item_size (BTK_TOOL_PALETTE (parent), item_size, homogeneous_only, requested_rows);
  else
    _btk_tool_item_group_item_size_request (group, item_size, homogeneous_only, requested_rows);
}

static void
btk_tool_item_group_size_request (BtkWidget      *widget,
                                  BtkRequisition *requisition)
{
  const gint border_width = BTK_CONTAINER (widget)->border_width;
  BtkToolItemGroup *group = BTK_TOOL_ITEM_GROUP (widget);
  BtkToolItemGroupPrivate* priv = group->priv;
  BtkOrientation orientation;
  BtkRequisition item_size;
  gint requested_rows;

  if (priv->children && btk_tool_item_group_get_label_widget (group))
    {
      btk_widget_size_request (priv->header, requisition);
      btk_widget_show (priv->header);
    }
  else
    {
      requisition->width = requisition->height = 0;
      btk_widget_hide (priv->header);
    }

  btk_tool_item_group_get_item_size (group, &item_size, FALSE, &requested_rows);

  orientation = btk_tool_shell_get_orientation (BTK_TOOL_SHELL (group));

  if (BTK_ORIENTATION_VERTICAL == orientation)
    requisition->width = MAX (requisition->width, item_size.width);
  else
    requisition->height = MAX (requisition->height, item_size.height * requested_rows);

  requisition->width += border_width * 2;
  requisition->height += border_width * 2;
}

static gboolean
btk_tool_item_group_is_item_visible (BtkToolItemGroup      *group,
                                     BtkToolItemGroupChild *child)
{
  BtkToolbarStyle style;
  BtkOrientation orientation;

  orientation = btk_tool_shell_get_orientation (BTK_TOOL_SHELL (group));
  style = btk_tool_shell_get_style (BTK_TOOL_SHELL (group));

  /* horizontal tool palettes with text style support only homogeneous items */
  if (!child->homogeneous &&
      BTK_ORIENTATION_HORIZONTAL == orientation &&
      BTK_TOOLBAR_TEXT == style)
    return FALSE;

  return
    (btk_widget_get_visible (BTK_WIDGET (child->item))) &&
    (BTK_ORIENTATION_VERTICAL == orientation ?
     btk_tool_item_get_visible_vertical (child->item) :
     btk_tool_item_get_visible_horizontal (child->item));
}

static inline unsigned
udiv (unsigned x,
      unsigned y)
{
  return (x + y - 1) / y;
}

static void
btk_tool_item_group_real_size_query (BtkWidget      *widget,
                                     BtkAllocation  *allocation,
                                     BtkRequisition *inquery)
{
  const gint border_width = BTK_CONTAINER (widget)->border_width;
  BtkToolItemGroup *group = BTK_TOOL_ITEM_GROUP (widget);
  BtkToolItemGroupPrivate* priv = group->priv;

  BtkRequisition item_size;
  BtkAllocation item_area;

  BtkOrientation orientation;
  BtkToolbarStyle style;

  gint min_rows;

  orientation = btk_tool_shell_get_orientation (BTK_TOOL_SHELL (group));
  style = btk_tool_shell_get_style (BTK_TOOL_SHELL (group));

  /* figure out the size of homogeneous items */
  btk_tool_item_group_get_item_size (group, &item_size, TRUE, &min_rows);

  if (BTK_ORIENTATION_VERTICAL == orientation)
    item_size.width = MIN (item_size.width, allocation->width);
  else
    item_size.height = MIN (item_size.height, allocation->height);

  item_size.width  = MAX (item_size.width, 1);
  item_size.height = MAX (item_size.height, 1);

  item_area.width = 0;
  item_area.height = 0;

  /* figure out the required columns (n_columns) and rows (n_rows) to place all items */
  if (!priv->collapsed || !priv->animation || priv->animation_timeout)
    {
      guint n_columns;
      gint n_rows;
      GList *it;

      if (BTK_ORIENTATION_VERTICAL == orientation)
        {
          gboolean new_row = FALSE;
          gint row = -1;
          guint col = 0;

          item_area.width = allocation->width - 2 * border_width;
          n_columns = MAX (item_area.width / item_size.width, 1);

          /* calculate required rows for n_columns columns */
          for (it = priv->children; it != NULL; it = it->next)
            {
              BtkToolItemGroupChild *child = it->data;

              if (!btk_tool_item_group_is_item_visible (group, child))
                continue;

              if (new_row || child->new_row)
                {
                  new_row = FALSE;
                  row++;
                  col = 0;
                }

              if (child->expand)
                new_row = TRUE;

              if (child->homogeneous)
                {
                  col++;
                  if (col >= n_columns)
                    new_row = TRUE;
                }
              else
                {
                  BtkRequisition req = {0, 0};
                  guint width;

                  btk_widget_size_request (BTK_WIDGET (child->item), &req);

                  width = udiv (req.width, item_size.width);
                  col += width;

                  if (col > n_columns)
                    row++;

                  col = width;

                  if (col >= n_columns)
                    new_row = TRUE;
                }
            }
          n_rows = row + 2;
        }
      else
        {
          guint *row_min_width;
          gint row = -1;
          gboolean new_row = TRUE;
          guint col = 0, min_col, max_col = 0, all_items = 0;
          gint i;

          item_area.height = allocation->height - 2 * border_width;
          n_rows = MAX (item_area.height / item_size.height, min_rows);

          row_min_width = g_new0 (guint, n_rows);

          /* calculate minimal and maximal required cols and minimal required rows */
          for (it = priv->children; it != NULL; it = it->next)
            {
              BtkToolItemGroupChild *child = it->data;

              if (!btk_tool_item_group_is_item_visible (group, child))
                continue;

              if (new_row || child->new_row)
                {
                  new_row = FALSE;
                  row++;
                  col = 0;
                  row_min_width[row] = 1;
                }

              if (child->expand)
                new_row = TRUE;

              if (child->homogeneous)
                {
                  col++;
                  all_items++;
                }
              else
                {
                  BtkRequisition req = {0, 0};
                  guint width;

                  btk_widget_size_request (BTK_WIDGET (child->item), &req);

                  width = udiv (req.width, item_size.width);

                  col += width;
                  all_items += width;

                  row_min_width[row] = MAX (row_min_width[row], width);
                }

              max_col = MAX (max_col, col);
            }

          /* calculate minimal required cols */
          min_col = udiv (all_items, n_rows);

          for (i = 0; i <= row; i++)
            {
              min_col = MAX (min_col, row_min_width[i]);
            }

          /* simple linear search for minimal required columns for the given maximal number of rows (n_rows) */
          for (n_columns = min_col; n_columns < max_col; n_columns ++)
            {
              new_row = TRUE;
              row = -1;
              /* calculate required rows for n_columns columns */
              for (it = priv->children; it != NULL; it = it->next)
                {
                  BtkToolItemGroupChild *child = it->data;

                  if (!btk_tool_item_group_is_item_visible (group, child))
                    continue;

                  if (new_row || child->new_row)
                    {
                      new_row = FALSE;
                      row++;
                      col = 0;
                    }

                  if (child->expand)
                    new_row = TRUE;

                  if (child->homogeneous)
                    {
                      col++;
                      if (col >= n_columns)
                        new_row = TRUE;
                    }
                  else
                    {
                      BtkRequisition req = {0, 0};
                      guint width;

                      btk_widget_size_request (BTK_WIDGET (child->item), &req);

                      width = udiv (req.width, item_size.width);
                      col += width;

                      if (col > n_columns)
                        row++;

                      col = width;

                      if (col >= n_columns)
                        new_row = TRUE;
                    }
                }

              if (row < n_rows)
                break;
            }
        }

      item_area.width = item_size.width * n_columns;
      item_area.height = item_size.height * n_rows;
    }

  inquery->width = 0;
  inquery->height = 0;

  /* figure out header widget size */
  if (btk_widget_get_visible (priv->header))
    {
      BtkRequisition child_requisition;

      btk_widget_size_request (priv->header, &child_requisition);

      if (BTK_ORIENTATION_VERTICAL == orientation)
        inquery->height += child_requisition.height;
      else
        inquery->width += child_requisition.width;
    }

  /* report effective widget size */
  inquery->width += item_area.width + 2 * border_width;
  inquery->height += item_area.height + 2 * border_width;
}

static void
btk_tool_item_group_real_size_allocate (BtkWidget     *widget,
                                        BtkAllocation *allocation)
{
  const gint border_width = BTK_CONTAINER (widget)->border_width;
  BtkToolItemGroup *group = BTK_TOOL_ITEM_GROUP (widget);
  BtkToolItemGroupPrivate* priv = group->priv;
  BtkRequisition child_requisition;
  BtkAllocation child_allocation;

  BtkRequisition item_size;
  BtkAllocation item_area;

  BtkOrientation orientation;
  BtkToolbarStyle style;

  GList *it;

  gint n_columns, n_rows = 1;
  gint min_rows;

  BtkTextDirection direction = btk_widget_get_direction (widget);

  orientation = btk_tool_shell_get_orientation (BTK_TOOL_SHELL (group));
  style = btk_tool_shell_get_style (BTK_TOOL_SHELL (group));

  /* chain up */
  BTK_WIDGET_CLASS (btk_tool_item_group_parent_class)->size_allocate (widget, allocation);

  child_allocation.x = border_width;
  child_allocation.y = border_width;

  /* place the header widget */
  if (btk_widget_get_visible (priv->header))
    {
      btk_widget_size_request (priv->header, &child_requisition);

      if (BTK_ORIENTATION_VERTICAL == orientation)
        {
          child_allocation.width = allocation->width;
          child_allocation.height = child_requisition.height;
        }
      else
        {
          child_allocation.width = child_requisition.width;
          child_allocation.height = allocation->height;

          if (BTK_TEXT_DIR_RTL == direction)
            child_allocation.x = allocation->width - border_width - child_allocation.width;
        }

      btk_widget_size_allocate (priv->header, &child_allocation);

      if (BTK_ORIENTATION_VERTICAL == orientation)
        child_allocation.y += child_allocation.height;
      else if (BTK_TEXT_DIR_RTL != direction)
        child_allocation.x += child_allocation.width;
      else
        child_allocation.x = border_width;
    }
  else
    child_requisition.width = child_requisition.height = 0;

  /* figure out the size of homogeneous items */
  btk_tool_item_group_get_item_size (group, &item_size, TRUE, &min_rows);

  item_size.width  = MAX (item_size.width, 1);
  item_size.height = MAX (item_size.height, 1);

  /* figure out the available columns and size of item_area */
  if (BTK_ORIENTATION_VERTICAL == orientation)
    {
      item_size.width = MIN (item_size.width, allocation->width);

      item_area.width = allocation->width - 2 * border_width;
      item_area.height = allocation->height - 2 * border_width - child_requisition.height;

      n_columns = MAX (item_area.width / item_size.width, 1);

      item_size.width = item_area.width / n_columns;
    }
  else
    {
      item_size.height = MIN (item_size.height, allocation->height);

      item_area.width = allocation->width - 2 * border_width - child_requisition.width;
      item_area.height = allocation->height - 2 * border_width;

      n_columns = MAX (item_area.width / item_size.width, 1);
      n_rows = MAX (item_area.height / item_size.height, min_rows);

      item_size.height = item_area.height / n_rows;
    }

  item_area.x = child_allocation.x;
  item_area.y = child_allocation.y;

  /* when expanded or in transition, place the tool items in a grid like layout */
  if (!priv->collapsed || !priv->animation || priv->animation_timeout)
    {
      gint col = 0, row = 0;

      for (it = priv->children; it != NULL; it = it->next)
        {
          BtkToolItemGroupChild *child = it->data;
          gint col_child;

          if (!btk_tool_item_group_is_item_visible (group, child))
            {
              btk_widget_set_child_visible (BTK_WIDGET (child->item), FALSE);

              continue;
            }

          /* for non homogeneous widgets request the required size */
          child_requisition.width = 0;

          if (!child->homogeneous)
            {
              btk_widget_size_request (BTK_WIDGET (child->item), &child_requisition);
              child_requisition.width = MIN (child_requisition.width, item_area.width);
            }

          /* select next row if at end of row */
          if (col > 0 && (child->new_row || (col * item_size.width) + MAX (child_requisition.width, item_size.width) > item_area.width))
            {
              row++;
              col = 0;
              child_allocation.y += child_allocation.height;
            }

          col_child = col;

          /* calculate the position and size of the item */
          if (!child->homogeneous)
            {
              gint col_width;
              gint width;

              if (!child->expand)
                col_width = udiv (child_requisition.width, item_size.width);
              else
                col_width = n_columns - col;

              width = col_width * item_size.width;

              if (BTK_TEXT_DIR_RTL == direction)
                col_child = (n_columns - col - col_width);

              if (child->fill)
                {
                  child_allocation.x = item_area.x + col_child * item_size.width;
                  child_allocation.width = width;
                }
              else
                {
                  child_allocation.x =
                    (item_area.x + col_child * item_size.width +
                    (width - child_requisition.width) / 2);
                  child_allocation.width = child_requisition.width;
                }

              col += col_width;
            }
          else
            {
              if (BTK_TEXT_DIR_RTL == direction)
                col_child = (n_columns - col - 1);

              child_allocation.x = item_area.x + col_child * item_size.width;
              child_allocation.width = item_size.width;

              col++;
            }

          child_allocation.height = item_size.height;

          btk_widget_size_allocate (BTK_WIDGET (child->item), &child_allocation);
          btk_widget_set_child_visible (BTK_WIDGET (child->item), TRUE);
        }

      child_allocation.y += item_size.height;
    }

  /* or just hide all items, when collapsed */

  else
    {
      for (it = priv->children; it != NULL; it = it->next)
        {
          BtkToolItemGroupChild *child = it->data;

          btk_widget_set_child_visible (BTK_WIDGET (child->item), FALSE);
        }
    }
}

static void
btk_tool_item_group_size_allocate (BtkWidget     *widget,
                                   BtkAllocation *allocation)
{
  btk_tool_item_group_real_size_allocate (widget, allocation);

  if (btk_widget_get_mapped (widget))
    bdk_window_invalidate_rect (widget->window, NULL, FALSE);
}

static void
btk_tool_item_group_set_focus_cb (BtkWidget *window,
                                  BtkWidget *widget,
                                  gpointer   user_data)
{
  BtkAdjustment *adjustment;
  BtkWidget *p;

  /* Find this group's parent widget in the focused widget's anchestry. */
  for (p = widget; p; p = btk_widget_get_parent (p))
    if (p == user_data)
      {
        p = btk_widget_get_parent (p);
        break;
      }

  if (BTK_IS_TOOL_PALETTE (p))
    {
      /* Check that the focused widgets is fully visible within
       * the group's parent widget and make it visible otherwise. */

      adjustment = btk_tool_palette_get_hadjustment (BTK_TOOL_PALETTE (p));
      adjustment = btk_tool_palette_get_vadjustment (BTK_TOOL_PALETTE (p));

      if (adjustment)
        {
          int y;

          /* Handle vertical adjustment. */
          if (btk_widget_translate_coordinates
                (widget, p, 0, 0, NULL, &y) && y < 0)
            {
              y += adjustment->value;
              btk_adjustment_clamp_page (adjustment, y, y + widget->allocation.height);
            }
          else if (btk_widget_translate_coordinates
                      (widget, p, 0, widget->allocation.height, NULL, &y) &&
                   y > p->allocation.height)
            {
              y += adjustment->value;
              btk_adjustment_clamp_page (adjustment, y - widget->allocation.height, y);
            }
        }

      adjustment = btk_tool_palette_get_hadjustment (BTK_TOOL_PALETTE (p));

      if (adjustment)
        {
          int x;

          /* Handle horizontal adjustment. */
          if (btk_widget_translate_coordinates
                (widget, p, 0, 0, &x, NULL) && x < 0)
            {
              x += adjustment->value;
              btk_adjustment_clamp_page (adjustment, x, x + widget->allocation.width);
            }
          else if (btk_widget_translate_coordinates
                      (widget, p, widget->allocation.width, 0, &x, NULL) &&
                   x > p->allocation.width)
            {
              x += adjustment->value;
              btk_adjustment_clamp_page (adjustment, x - widget->allocation.width, x);
            }

          return;
        }
    }
}

static void
btk_tool_item_group_set_toplevel_window (BtkToolItemGroup *group,
                                         BtkWidget        *toplevel)
{
  BtkToolItemGroupPrivate* priv = group->priv;

  if (toplevel != priv->toplevel)
    {
      if (priv->toplevel)
        {
          /* Disconnect focus tracking handler. */
          g_signal_handler_disconnect (priv->toplevel,
                                       priv->focus_set_id);

          priv->focus_set_id = 0;
          priv->toplevel = NULL;
        }

      if (toplevel)
        {
          /* Install focus tracking handler. We connect to the window's
           * set-focus signal instead of connecting to the focus signal of
           * each child to:
           *
           * 1) Reduce the number of signal handlers used.
           * 2) Avoid special handling for group headers.
           * 3) Catch focus grabs not only for direct children,
           *    but also for nested widgets.
           */
          priv->focus_set_id =
            g_signal_connect (toplevel, "set-focus",
                              G_CALLBACK (btk_tool_item_group_set_focus_cb),
                              group);

          priv->toplevel = toplevel;
        }
    }
}

static void
btk_tool_item_group_realize (BtkWidget *widget)
{
  BtkWidget *toplevel_window;
  const gint border_width = BTK_CONTAINER (widget)->border_width;
  gint attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_VISUAL | BDK_WA_COLORMAP;
  BdkWindowAttr attributes;
  BdkDisplay *display;

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

  display = bdk_window_get_display (widget->window);

  if (bdk_display_supports_composite (display))
    bdk_window_set_composited (widget->window, TRUE);

  bdk_window_set_user_data (widget->window, widget);
  widget->style = btk_style_attach (widget->style, widget->window);
  btk_style_set_background (widget->style, widget->window, BTK_STATE_NORMAL);
  btk_widget_set_realized (widget, TRUE);

  btk_container_forall (BTK_CONTAINER (widget),
                        (BtkCallback) btk_widget_set_parent_window,
                        widget->window);

  btk_widget_queue_resize_no_redraw (widget);

  toplevel_window = btk_widget_get_ancestor (widget, BTK_TYPE_WINDOW);
  btk_tool_item_group_set_toplevel_window (BTK_TOOL_ITEM_GROUP (widget),
                                           toplevel_window);
}

static void
btk_tool_item_group_unrealize (BtkWidget *widget)
{
  btk_tool_item_group_set_toplevel_window (BTK_TOOL_ITEM_GROUP (widget), NULL);
  BTK_WIDGET_CLASS (btk_tool_item_group_parent_class)->unrealize (widget);
}

static void
btk_tool_item_group_style_set (BtkWidget *widget,
                               BtkStyle  *previous_style)
{
  btk_tool_item_group_header_adjust_style (BTK_TOOL_ITEM_GROUP (widget));
  BTK_WIDGET_CLASS (btk_tool_item_group_parent_class)->style_set (widget, previous_style);
}

static void
btk_tool_item_group_add (BtkContainer *container,
                         BtkWidget    *widget)
{
  g_return_if_fail (BTK_IS_TOOL_ITEM_GROUP (container));
  g_return_if_fail (BTK_IS_TOOL_ITEM (widget));

  btk_tool_item_group_insert (BTK_TOOL_ITEM_GROUP (container),
                              BTK_TOOL_ITEM (widget), -1);
}

static void
btk_tool_item_group_remove (BtkContainer *container,
                            BtkWidget    *child)
{
  BtkToolItemGroup *group;
  BtkToolItemGroupPrivate* priv;
  GList *it;

  g_return_if_fail (BTK_IS_TOOL_ITEM_GROUP (container));
  group = BTK_TOOL_ITEM_GROUP (container);
  priv = group->priv;

  for (it = priv->children; it != NULL; it = it->next)
    {
      BtkToolItemGroupChild *child_info = it->data;

      if ((BtkWidget *)child_info->item == child)
        {
          g_object_unref (child);
          btk_widget_unparent (child);

          g_free (child_info);
          priv->children = g_list_delete_link (priv->children, it);

          btk_widget_queue_resize (BTK_WIDGET (container));
          break;
        }
    }
}

static void
btk_tool_item_group_forall (BtkContainer *container,
                            gboolean      internals,
                            BtkCallback   callback,
                            gpointer      callback_data)
{
  BtkToolItemGroup *group = BTK_TOOL_ITEM_GROUP (container);
  BtkToolItemGroupPrivate* priv = group->priv;
  GList *children;

  if (internals && priv->header)
    callback (priv->header, callback_data);

  children = priv->children;
  while (children)
    {
      BtkToolItemGroupChild *child = children->data;
      children = children->next; /* store pointer before call to callback
				    because the child pointer is invalid if the
				    child->item is removed from the item group
				    in callback */

      callback (BTK_WIDGET (child->item), callback_data);
    }
}

static GType
btk_tool_item_group_child_type (BtkContainer *container)
{
  return BTK_TYPE_TOOL_ITEM;
}

static BtkToolItemGroupChild *
btk_tool_item_group_get_child (BtkToolItemGroup  *group,
                               BtkToolItem       *item,
                               gint              *position,
                               GList            **link)
{
  guint i;
  GList *it;

  g_return_val_if_fail (BTK_IS_TOOL_ITEM_GROUP (group), NULL);
  g_return_val_if_fail (BTK_IS_TOOL_ITEM (item), NULL);

  for (it = group->priv->children, i = 0; it != NULL; it = it->next, ++i)
    {
      BtkToolItemGroupChild *child = it->data;

      if (child->item == item)
        {
          if (position)
            *position = i;

          if (link)
            *link = it;

          return child;
        }
    }

  return NULL;
}

static void
btk_tool_item_group_get_item_packing (BtkToolItemGroup *group,
                                      BtkToolItem      *item,
                                      gboolean         *homogeneous,
                                      gboolean         *expand,
                                      gboolean         *fill,
                                      gboolean         *new_row)
{
  BtkToolItemGroupChild *child;

  g_return_if_fail (BTK_IS_TOOL_ITEM_GROUP (group));
  g_return_if_fail (BTK_IS_TOOL_ITEM (item));

  child = btk_tool_item_group_get_child (group, item, NULL, NULL);
  if (!child)
    return;

  if (expand)
    *expand = child->expand;

  if (homogeneous)
    *homogeneous = child->homogeneous;

  if (fill)
    *fill = child->fill;

  if (new_row)
    *new_row = child->new_row;
}

static void
btk_tool_item_group_set_item_packing (BtkToolItemGroup *group,
                                      BtkToolItem      *item,
                                      gboolean          homogeneous,
                                      gboolean          expand,
                                      gboolean          fill,
                                      gboolean          new_row)
{
  BtkToolItemGroupChild *child;
  gboolean changed = FALSE;

  g_return_if_fail (BTK_IS_TOOL_ITEM_GROUP (group));
  g_return_if_fail (BTK_IS_TOOL_ITEM (item));

  child = btk_tool_item_group_get_child (group, item, NULL, NULL);
  if (!child)
    return;

  btk_widget_freeze_child_notify (BTK_WIDGET (item));

  if (child->homogeneous != homogeneous)
    {
      child->homogeneous = homogeneous;
      changed = TRUE;
      btk_widget_child_notify (BTK_WIDGET (item), "homogeneous");
    }
  if (child->expand != expand)
    {
      child->expand = expand;
      changed = TRUE;
      btk_widget_child_notify (BTK_WIDGET (item), "expand");
    }
  if (child->fill != fill)
    {
      child->fill = fill;
      changed = TRUE;
      btk_widget_child_notify (BTK_WIDGET (item), "fill");
    }
  if (child->new_row != new_row)
    {
      child->new_row = new_row;
      changed = TRUE;
      btk_widget_child_notify (BTK_WIDGET (item), "new-row");
    }

  btk_widget_thaw_child_notify (BTK_WIDGET (item));

  if (changed
      && btk_widget_get_visible (BTK_WIDGET (group))
      && btk_widget_get_visible (BTK_WIDGET (item)))
    btk_widget_queue_resize (BTK_WIDGET (group));
}

static void
btk_tool_item_group_set_child_property (BtkContainer *container,
                                        BtkWidget    *child,
                                        guint         prop_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  BtkToolItemGroup *group = BTK_TOOL_ITEM_GROUP (container);
  BtkToolItem *item = BTK_TOOL_ITEM (child);
  gboolean homogeneous, expand, fill, new_row;

  if (prop_id != CHILD_PROP_POSITION)
    btk_tool_item_group_get_item_packing (group, item,
                                          &homogeneous,
                                          &expand,
                                          &fill,
                                          &new_row);

  switch (prop_id)
    {
      case CHILD_PROP_HOMOGENEOUS:
        btk_tool_item_group_set_item_packing (group, item,
                                              g_value_get_boolean (value),
                                              expand,
                                              fill,
                                              new_row);
        break;

      case CHILD_PROP_EXPAND:
        btk_tool_item_group_set_item_packing (group, item,
                                              homogeneous,
                                              g_value_get_boolean (value),
                                              fill,
                                              new_row);
        break;

      case CHILD_PROP_FILL:
        btk_tool_item_group_set_item_packing (group, item,
                                              homogeneous,
                                              expand,
                                              g_value_get_boolean (value),
                                              new_row);
        break;

      case CHILD_PROP_NEW_ROW:
        btk_tool_item_group_set_item_packing (group, item,
                                              homogeneous,
                                              expand,
                                              fill,
                                              g_value_get_boolean (value));
        break;

      case CHILD_PROP_POSITION:
        btk_tool_item_group_set_item_position (group, item, g_value_get_int (value));
        break;

      default:
        BTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, prop_id, pspec);
        break;
    }
}

static void
btk_tool_item_group_get_child_property (BtkContainer *container,
                                        BtkWidget    *child,
                                        guint         prop_id,
                                        GValue       *value,
                                        GParamSpec   *pspec)
{
  BtkToolItemGroup *group = BTK_TOOL_ITEM_GROUP (container);
  BtkToolItem *item = BTK_TOOL_ITEM (child);
  gboolean homogeneous, expand, fill, new_row;

  if (prop_id != CHILD_PROP_POSITION)
    btk_tool_item_group_get_item_packing (group, item,
                                          &homogeneous,
                                          &expand,
                                          &fill,
                                          &new_row);

  switch (prop_id)
    {
      case CHILD_PROP_HOMOGENEOUS:
        g_value_set_boolean (value, homogeneous);
        break;

       case CHILD_PROP_EXPAND:
        g_value_set_boolean (value, expand);
        break;

       case CHILD_PROP_FILL:
        g_value_set_boolean (value, fill);
        break;

       case CHILD_PROP_NEW_ROW:
        g_value_set_boolean (value, new_row);
        break;

     case CHILD_PROP_POSITION:
        g_value_set_int (value, btk_tool_item_group_get_item_position (group, item));
        break;

      default:
        BTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, prop_id, pspec);
        break;
    }
}

static void
btk_tool_item_group_class_init (BtkToolItemGroupClass *cls)
{
  GObjectClass       *oclass = G_OBJECT_CLASS (cls);
  BtkWidgetClass     *wclass = BTK_WIDGET_CLASS (cls);
  BtkContainerClass  *cclass = BTK_CONTAINER_CLASS (cls);

  oclass->set_property       = btk_tool_item_group_set_property;
  oclass->get_property       = btk_tool_item_group_get_property;
  oclass->finalize           = btk_tool_item_group_finalize;
  oclass->dispose            = btk_tool_item_group_dispose;

  wclass->size_request       = btk_tool_item_group_size_request;
  wclass->size_allocate      = btk_tool_item_group_size_allocate;
  wclass->realize            = btk_tool_item_group_realize;
  wclass->unrealize          = btk_tool_item_group_unrealize;
  wclass->style_set          = btk_tool_item_group_style_set;
  wclass->screen_changed     = btk_tool_item_group_screen_changed;

  cclass->add                = btk_tool_item_group_add;
  cclass->remove             = btk_tool_item_group_remove;
  cclass->forall             = btk_tool_item_group_forall;
  cclass->child_type         = btk_tool_item_group_child_type;
  cclass->set_child_property = btk_tool_item_group_set_child_property;
  cclass->get_child_property = btk_tool_item_group_get_child_property;

  g_object_class_install_property (oclass, PROP_LABEL,
                                   g_param_spec_string ("label",
                                                        P_("Label"),
                                                        P_("The human-readable title of this item group"),
                                                        DEFAULT_LABEL,
                                                        BTK_PARAM_READWRITE));

  g_object_class_install_property (oclass, PROP_LABEL_WIDGET,
                                   g_param_spec_object  ("label-widget",
                                                        P_("Label widget"),
                                                        P_("A widget to display in place of the usual label"),
                                                        BTK_TYPE_WIDGET,
							BTK_PARAM_READWRITE));

  g_object_class_install_property (oclass, PROP_COLLAPSED,
                                   g_param_spec_boolean ("collapsed",
                                                         P_("Collapsed"),
                                                         P_("Whether the group has been collapsed and items are hidden"),
                                                         DEFAULT_COLLAPSED,
                                                         BTK_PARAM_READWRITE));

  g_object_class_install_property (oclass, PROP_ELLIPSIZE,
                                   g_param_spec_enum ("ellipsize",
                                                      P_("ellipsize"),
                                                      P_("Ellipsize for item group headers"),
                                                      BANGO_TYPE_ELLIPSIZE_MODE, DEFAULT_ELLIPSIZE,
                                                      BTK_PARAM_READWRITE));

  g_object_class_install_property (oclass, PROP_RELIEF,
                                   g_param_spec_enum ("header-relief",
                                                      P_("Header Relief"),
                                                      P_("Relief of the group header button"),
                                                      BTK_TYPE_RELIEF_STYLE, BTK_RELIEF_NORMAL,
                                                      BTK_PARAM_READWRITE));

  btk_widget_class_install_style_property (wclass,
                                           g_param_spec_int ("expander-size",
                                                             P_("Expander Size"),
                                                             P_("Size of the expander arrow"),
                                                             0,
                                                             G_MAXINT,
                                                             DEFAULT_EXPANDER_SIZE,
                                                             BTK_PARAM_READABLE));

  btk_widget_class_install_style_property (wclass,
                                           g_param_spec_int ("header-spacing",
                                                             P_("Header Spacing"),
                                                             P_("Spacing between expander arrow and caption"),
                                                             0,
                                                             G_MAXINT,
                                                             DEFAULT_HEADER_SPACING,
                                                             BTK_PARAM_READABLE));

  btk_container_class_install_child_property (cclass, CHILD_PROP_HOMOGENEOUS,
                                              g_param_spec_boolean ("homogeneous",
                                                                    P_("Homogeneous"),
                                                                    P_("Whether the item should be the same size as other homogeneous items"),
                                                                    TRUE,
                                                                    BTK_PARAM_READWRITE));

  btk_container_class_install_child_property (cclass, CHILD_PROP_EXPAND,
                                              g_param_spec_boolean ("expand",
                                                                    P_("Expand"),
                                                                    P_("Whether the item should receive extra space when the group grows"),
                                                                    FALSE,
                                                                    BTK_PARAM_READWRITE)); 

  btk_container_class_install_child_property (cclass, CHILD_PROP_FILL,
                                              g_param_spec_boolean ("fill",
                                                                    P_("Fill"),
                                                                    P_("Whether the item should fill the available space"),
                                                                    TRUE,
                                                                    BTK_PARAM_READWRITE));

  btk_container_class_install_child_property (cclass, CHILD_PROP_NEW_ROW,
                                              g_param_spec_boolean ("new-row",
                                                                    P_("New Row"),
                                                                    P_("Whether the item should start a new row"),
                                                                    FALSE,
                                                                    BTK_PARAM_READWRITE));

  btk_container_class_install_child_property (cclass, CHILD_PROP_POSITION,
                                              g_param_spec_int ("position",
                                                                P_("Position"),
                                                                P_("Position of the item within this group"),
                                                                0,
                                                                G_MAXINT,
                                                                0,
                                                                BTK_PARAM_READWRITE));

  g_type_class_add_private (cls, sizeof (BtkToolItemGroupPrivate));
}

/**
 * btk_tool_item_group_new:
 * @label: the label of the new group
 *
 * Creates a new tool item group with label @label.
 *
 * Returns: a new #BtkToolItemGroup.
 *
 * Since: 2.20
 */
BtkWidget*
btk_tool_item_group_new (const gchar *label)
{
  return g_object_new (BTK_TYPE_TOOL_ITEM_GROUP, "label", label, NULL);
}

/**
 * btk_tool_item_group_set_label:
 * @group: a #BtkToolItemGroup
 * @label: the new human-readable label of of the group
 *
 * Sets the label of the tool item group. The label is displayed in the header
 * of the group.
 *
 * Since: 2.20
 */
void
btk_tool_item_group_set_label (BtkToolItemGroup *group,
                               const gchar      *label)
{
  g_return_if_fail (BTK_IS_TOOL_ITEM_GROUP (group));

  if (!label)
    btk_tool_item_group_set_label_widget (group, NULL);
  else
    {
      BtkWidget *child = btk_label_new (label);
      btk_widget_show (child);

      btk_tool_item_group_set_label_widget (group, child);
    }

  g_object_notify (G_OBJECT (group), "label");
}

/**
 * btk_tool_item_group_set_label_widget:
 * @group: a #BtkToolItemGroup
 * @label_widget: the widget to be displayed in place of the usual label
 *
 * Sets the label of the tool item group.
 * The label widget is displayed in the header of the group, in place
 * of the usual label.
 *
 * Since: 2.20
 */
void
btk_tool_item_group_set_label_widget (BtkToolItemGroup *group,
                                      BtkWidget        *label_widget)
{
  BtkToolItemGroupPrivate* priv;
  BtkWidget *alignment;

  g_return_if_fail (BTK_IS_TOOL_ITEM_GROUP (group));
  g_return_if_fail (label_widget == NULL || BTK_IS_WIDGET (label_widget));
  g_return_if_fail (label_widget == NULL || label_widget->parent == NULL);

  priv = group->priv;

  if (priv->label_widget == label_widget)
    return;

  alignment = btk_tool_item_group_get_alignment (group);

  if (priv->label_widget)
    {
      btk_widget_set_state (priv->label_widget, BTK_STATE_NORMAL);
      btk_container_remove (BTK_CONTAINER (alignment), priv->label_widget);
    }


  if (label_widget)
      btk_container_add (BTK_CONTAINER (alignment), label_widget);

  priv->label_widget = label_widget;

  if (btk_widget_get_visible (BTK_WIDGET (group)))
    btk_widget_queue_resize (BTK_WIDGET (group));

  /* Only show the header widget if the group has children: */
  if (label_widget && priv->children)
    btk_widget_show (priv->header);
  else
    btk_widget_hide (priv->header);

  g_object_freeze_notify (G_OBJECT (group));
  g_object_notify (G_OBJECT (group), "label-widget");
  g_object_notify (G_OBJECT (group), "label");
  g_object_thaw_notify (G_OBJECT (group));
}

/**
 * btk_tool_item_group_set_header_relief:
 * @group: a #BtkToolItemGroup
 * @style: the #BtkReliefStyle
 *
 * Set the button relief of the group header.
 * See btk_button_set_relief() for details.
 *
 * Since: 2.20
 */
void
btk_tool_item_group_set_header_relief (BtkToolItemGroup *group,
                                       BtkReliefStyle    style)
{
  g_return_if_fail (BTK_IS_TOOL_ITEM_GROUP (group));

  btk_button_set_relief (BTK_BUTTON (group->priv->header), style);
}

static gint64
btk_tool_item_group_get_animation_timestamp (BtkToolItemGroup *group)
{
  return (g_source_get_time (group->priv->animation_timeout) -
          group->priv->animation_start) / 1000;
}

static void
btk_tool_item_group_force_expose (BtkToolItemGroup *group)
{
  BtkToolItemGroupPrivate* priv = group->priv;
  BtkWidget *widget = BTK_WIDGET (group);

  if (btk_widget_get_realized (priv->header))
    {
      BtkWidget *alignment = btk_tool_item_group_get_alignment (group);
      BdkRectangle area;

      /* Find the header button's arrow area... */
      area.x = alignment->allocation.x;
      area.y = alignment->allocation.y + (alignment->allocation.height - priv->expander_size) / 2;
      area.height = priv->expander_size;
      area.width = priv->expander_size;

      /* ... and invalidated it to get it animated. */
      bdk_window_invalidate_rect (priv->header->window, &area, TRUE);
    }

  if (btk_widget_get_realized (widget))
    {
      BtkWidget *parent = btk_widget_get_parent (widget);
      int x, y, width, height;

      /* Find the tool item area button's arrow area... */
      width = widget->allocation.width;
      height = widget->allocation.height;

      btk_widget_translate_coordinates (widget, parent, 0, 0, &x, &y);

      if (btk_widget_get_visible (priv->header))
        {
          height -= priv->header->allocation.height;
          y += priv->header->allocation.height;
        }

      /* ... and invalidated it to get it animated. */
      btk_widget_queue_draw_area (parent, x, y, width, height);
    }
}

static gboolean
btk_tool_item_group_animation_cb (gpointer data)
{
  BtkToolItemGroup *group = BTK_TOOL_ITEM_GROUP (data);
  BtkToolItemGroupPrivate* priv = group->priv;
  gint64 timestamp = btk_tool_item_group_get_animation_timestamp (group);
  gboolean retval;

  BDK_THREADS_ENTER ();

  /* Enque this early to reduce number of expose events. */
  btk_widget_queue_resize_no_redraw (BTK_WIDGET (group));

  /* Figure out current style of the expander arrow. */
  if (priv->collapsed)
    {
      if (priv->expander_style == BTK_EXPANDER_EXPANDED)
        priv->expander_style = BTK_EXPANDER_SEMI_COLLAPSED;
      else
        priv->expander_style = BTK_EXPANDER_COLLAPSED;
    }
  else
    {
      if (priv->expander_style == BTK_EXPANDER_COLLAPSED)
        priv->expander_style = BTK_EXPANDER_SEMI_EXPANDED;
      else
        priv->expander_style = BTK_EXPANDER_EXPANDED;
    }

  btk_tool_item_group_force_expose (group);

  /* Finish animation when done. */
  if (timestamp >= ANIMATION_DURATION)
    priv->animation_timeout = NULL;

  retval = (priv->animation_timeout != NULL);

  BDK_THREADS_LEAVE ();

  return retval;
}

/**
 * btk_tool_item_group_set_collapsed:
 * @group: a #BtkToolItemGroup
 * @collapsed: whether the @group should be collapsed or expanded
 *
 * Sets whether the @group should be collapsed or expanded.
 *
 * Since: 2.20
 */
void
btk_tool_item_group_set_collapsed (BtkToolItemGroup *group,
                                   gboolean          collapsed)
{
  BtkWidget *parent;
  BtkToolItemGroupPrivate* priv;

  g_return_if_fail (BTK_IS_TOOL_ITEM_GROUP (group));

  priv = group->priv;

  parent = btk_widget_get_parent (BTK_WIDGET (group));
  if (BTK_IS_TOOL_PALETTE (parent) && !collapsed)
    _btk_tool_palette_set_expanding_child (BTK_TOOL_PALETTE (parent),
                                           BTK_WIDGET (group));
  if (collapsed != priv->collapsed)
    {
      if (priv->animation)
        {
          if (priv->animation_timeout)
            g_source_destroy (priv->animation_timeout);

          priv->animation_start = g_get_monotonic_time ();
          priv->animation_timeout = g_timeout_source_new (ANIMATION_TIMEOUT);

          g_source_set_callback (priv->animation_timeout,
                                 btk_tool_item_group_animation_cb,
                                 group, NULL);

          g_source_attach (priv->animation_timeout, NULL);
        }
        else
        {
          priv->expander_style = BTK_EXPANDER_COLLAPSED;
          btk_tool_item_group_force_expose (group);
        }

      priv->collapsed = collapsed;
      g_object_notify (G_OBJECT (group), "collapsed");
    }
}

/**
 * btk_tool_item_group_set_ellipsize:
 * @group: a #BtkToolItemGroup
 * @ellipsize: the #BangoEllipsizeMode labels in @group should use
 *
 * Sets the ellipsization mode which should be used by labels in @group.
 *
 * Since: 2.20
 */
void
btk_tool_item_group_set_ellipsize (BtkToolItemGroup   *group,
                                   BangoEllipsizeMode  ellipsize)
{
  g_return_if_fail (BTK_IS_TOOL_ITEM_GROUP (group));

  if (ellipsize != group->priv->ellipsize)
    {
      group->priv->ellipsize = ellipsize;
      btk_tool_item_group_header_adjust_style (group);
      g_object_notify (G_OBJECT (group), "ellipsize");
      _btk_tool_item_group_palette_reconfigured (group);
    }
}

/**
 * btk_tool_item_group_get_label:
 * @group: a #BtkToolItemGroup
 *
 * Gets the label of @group.
 *
 * Returns: the label of @group. The label is an internal string of @group
 *     and must not be modified. Note that %NULL is returned if a custom
 *     label has been set with btk_tool_item_group_set_label_widget()
 *
 * Since: 2.20
 */
const gchar*
btk_tool_item_group_get_label (BtkToolItemGroup *group)
{
  BtkToolItemGroupPrivate *priv;

  g_return_val_if_fail (BTK_IS_TOOL_ITEM_GROUP (group), NULL);

  priv = group->priv;

  if (BTK_IS_LABEL (priv->label_widget))
    return btk_label_get_label (BTK_LABEL (priv->label_widget));
  else
    return NULL;
}

/**
 * btk_tool_item_group_get_label_widget:
 * @group: a #BtkToolItemGroup
 *
 * Gets the label widget of @group.
 * See btk_tool_item_group_set_label_widget().
 *
 * Returns: (transfer none): the label widget of @group
 *
 * Since: 2.20
 */
BtkWidget*
btk_tool_item_group_get_label_widget (BtkToolItemGroup *group)
{
  BtkWidget *alignment = btk_tool_item_group_get_alignment (group);

  return btk_bin_get_child (BTK_BIN (alignment));
}

/**
 * btk_tool_item_group_get_collapsed:
 * @group: a BtkToolItemGroup
 *
 * Gets whether @group is collapsed or expanded.
 *
 * Returns: %TRUE if @group is collapsed, %FALSE if it is expanded
 *
 * Since: 2.20
 */
gboolean
btk_tool_item_group_get_collapsed (BtkToolItemGroup *group)
{
  g_return_val_if_fail (BTK_IS_TOOL_ITEM_GROUP (group), DEFAULT_COLLAPSED);

  return group->priv->collapsed;
}

/**
 * btk_tool_item_group_get_ellipsize:
 * @group: a #BtkToolItemGroup
 *
 * Gets the ellipsization mode of @group.
 *
 * Returns: the #BangoEllipsizeMode of @group
 *
 * Since: 2.20
 */
BangoEllipsizeMode
btk_tool_item_group_get_ellipsize (BtkToolItemGroup *group)
{
  g_return_val_if_fail (BTK_IS_TOOL_ITEM_GROUP (group), DEFAULT_ELLIPSIZE);

  return group->priv->ellipsize;
}

/**
 * btk_tool_item_group_get_header_relief:
 * @group: a #BtkToolItemGroup
 *
 * Gets the relief mode of the header button of @group.
 *
 * Returns: the #BtkReliefStyle
 *
 * Since: 2.20
 */
BtkReliefStyle
btk_tool_item_group_get_header_relief (BtkToolItemGroup   *group)
{
  g_return_val_if_fail (BTK_IS_TOOL_ITEM_GROUP (group), BTK_RELIEF_NORMAL);

  return btk_button_get_relief (BTK_BUTTON (group->priv->header));
}

/**
 * btk_tool_item_group_insert:
 * @group: a #BtkToolItemGroup
 * @item: the #BtkToolItem to insert into @group
 * @position: the position of @item in @group, starting with 0.
 *     The position -1 means end of list.
 *
 * Inserts @item at @position in the list of children of @group.
 *
 * Since: 2.20
 */
void
btk_tool_item_group_insert (BtkToolItemGroup *group,
                            BtkToolItem      *item,
                            gint              position)
{
  BtkWidget *parent, *child_widget;
  BtkToolItemGroupChild *child;

  g_return_if_fail (BTK_IS_TOOL_ITEM_GROUP (group));
  g_return_if_fail (BTK_IS_TOOL_ITEM (item));
  g_return_if_fail (position >= -1);

  parent = btk_widget_get_parent (BTK_WIDGET (group));

  child = g_new (BtkToolItemGroupChild, 1);
  child->item = g_object_ref_sink (item);
  child->homogeneous = TRUE;
  child->expand = FALSE;
  child->fill = TRUE;
  child->new_row = FALSE;

  group->priv->children = g_list_insert (group->priv->children, child, position);

  if (BTK_IS_TOOL_PALETTE (parent))
    _btk_tool_palette_child_set_drag_source (BTK_WIDGET (item), parent);

  child_widget = btk_bin_get_child (BTK_BIN (item));

  if (BTK_IS_BUTTON (child_widget))
    btk_button_set_focus_on_click (BTK_BUTTON (child_widget), TRUE);

  btk_widget_set_parent (BTK_WIDGET (item), BTK_WIDGET (group));
}

/**
 * btk_tool_item_group_set_item_position:
 * @group: a #BtkToolItemGroup
 * @item: the #BtkToolItem to move to a new position, should
 *     be a child of @group.
 * @position: the new position of @item in @group, starting with 0.
 *     The position -1 means end of list.
 *
 * Sets the position of @item in the list of children of @group.
 *
 * Since: 2.20
 */
void
btk_tool_item_group_set_item_position (BtkToolItemGroup *group,
                                       BtkToolItem      *item,
                                       gint              position)
{
  gint old_position;
  GList *link;
  BtkToolItemGroupChild *child;
  BtkToolItemGroupPrivate* priv;

  g_return_if_fail (BTK_IS_TOOL_ITEM_GROUP (group));
  g_return_if_fail (BTK_IS_TOOL_ITEM (item));
  g_return_if_fail (position >= -1);

  child = btk_tool_item_group_get_child (group, item, &old_position, &link);
  priv = group->priv;

  g_return_if_fail (child != NULL);

  if (position == old_position)
    return;

  priv->children = g_list_delete_link (priv->children, link);
  priv->children = g_list_insert (priv->children, child, position);

  btk_widget_child_notify (BTK_WIDGET (item), "position");
  if (btk_widget_get_visible (BTK_WIDGET (group)) &&
      btk_widget_get_visible (BTK_WIDGET (item)))
    btk_widget_queue_resize (BTK_WIDGET (group));
}

/**
 * btk_tool_item_group_get_item_position:
 * @group: a #BtkToolItemGroup
 * @item: a #BtkToolItem
 *
 * Gets the position of @item in @group as index.
 *
 * Returns: the index of @item in @group or -1 if @item is no child of @group
 *
 * Since: 2.20
 */
gint
btk_tool_item_group_get_item_position (BtkToolItemGroup *group,
                                       BtkToolItem      *item)
{
  gint position;

  g_return_val_if_fail (BTK_IS_TOOL_ITEM_GROUP (group), -1);
  g_return_val_if_fail (BTK_IS_TOOL_ITEM (item), -1);

  if (btk_tool_item_group_get_child (group, item, &position, NULL))
    return position;

  return -1;
}

/**
 * btk_tool_item_group_get_n_items:
 * @group: a #BtkToolItemGroup
 *
 * Gets the number of tool items in @group.
 *
 * Returns: the number of tool items in @group
 *
 * Since: 2.20
 */
guint
btk_tool_item_group_get_n_items (BtkToolItemGroup *group)
{
  g_return_val_if_fail (BTK_IS_TOOL_ITEM_GROUP (group), 0);

  return g_list_length (group->priv->children);
}

/**
 * btk_tool_item_group_get_nth_item:
 * @group: a #BtkToolItemGroup
 * @index: the index
 *
 * Gets the tool item at @index in group.
 *
 * Returns: (transfer none): the #BtkToolItem at index
 *
 * Since: 2.20
 */
BtkToolItem*
btk_tool_item_group_get_nth_item (BtkToolItemGroup *group,
                                  guint             index)
{
  BtkToolItemGroupChild *child;

  g_return_val_if_fail (BTK_IS_TOOL_ITEM_GROUP (group), NULL);

  child = g_list_nth_data (group->priv->children, index);

  return child != NULL ? child->item : NULL;
}

/**
 * btk_tool_item_group_get_drop_item:
 * @group: a #BtkToolItemGroup
 * @x: the x position
 * @y: the y position
 *
 * Gets the tool item at position (x, y).
 *
 * Returns: (transfer none): the #BtkToolItem at position (x, y)
 *
 * Since: 2.20
 */
BtkToolItem*
btk_tool_item_group_get_drop_item (BtkToolItemGroup *group,
                                   gint              x,
                                   gint              y)
{
  BtkAllocation *allocation;
  BtkOrientation orientation;
  GList *it;

  g_return_val_if_fail (BTK_IS_TOOL_ITEM_GROUP (group), NULL);

  allocation = &BTK_WIDGET (group)->allocation;
  orientation = btk_tool_shell_get_orientation (BTK_TOOL_SHELL (group));

  g_return_val_if_fail (x >= 0 && x < allocation->width, NULL);
  g_return_val_if_fail (y >= 0 && y < allocation->height, NULL);

  for (it = group->priv->children; it != NULL; it = it->next)
    {
      BtkToolItemGroupChild *child = it->data;
      BtkToolItem *item = child->item;
      gint x0, y0;

      if (!item || !btk_tool_item_group_is_item_visible (group, child))
        continue;

      allocation = &BTK_WIDGET (item)->allocation;

      x0 = x - allocation->x;
      y0 = y - allocation->y;

      if (x0 >= 0 && x0 < allocation->width &&
          y0 >= 0 && y0 < allocation->height)
        return item;
    }

  return NULL;
}

void
_btk_tool_item_group_item_size_request (BtkToolItemGroup *group,
                                        BtkRequisition   *item_size,
                                        gboolean          homogeneous_only,
                                        gint             *requested_rows)
{
  BtkRequisition child_requisition;
  GList *it;
  gint rows = 0;
  gboolean new_row = TRUE;
  BtkOrientation orientation;
  BtkToolbarStyle style;

  g_return_if_fail (BTK_IS_TOOL_ITEM_GROUP (group));
  g_return_if_fail (NULL != item_size);

  orientation = btk_tool_shell_get_orientation (BTK_TOOL_SHELL (group));
  style = btk_tool_shell_get_style (BTK_TOOL_SHELL (group));

  item_size->width = item_size->height = 0;

  for (it = group->priv->children; it != NULL; it = it->next)
    {
      BtkToolItemGroupChild *child = it->data;

      if (!btk_tool_item_group_is_item_visible (group, child))
        continue;

      if (child->new_row || new_row)
        {
          rows++;
          new_row = FALSE;
        }

      if (!child->homogeneous && child->expand)
          new_row = TRUE;

      btk_widget_size_request (BTK_WIDGET (child->item), &child_requisition);

      if (!homogeneous_only || child->homogeneous)
        item_size->width = MAX (item_size->width, child_requisition.width);
      item_size->height = MAX (item_size->height, child_requisition.height);
    }

  if (requested_rows)
    *requested_rows = rows;
}

void
_btk_tool_item_group_paint (BtkToolItemGroup *group,
                            bairo_t          *cr)
{
  BtkWidget *widget = BTK_WIDGET (group);
  BtkToolItemGroupPrivate* priv = group->priv;

  bdk_bairo_set_source_pixmap (cr, widget->window,
                               widget->allocation.x,
                               widget->allocation.y);

  if (priv->animation_timeout)
    {
      BtkOrientation orientation = btk_tool_item_group_get_orientation (BTK_TOOL_SHELL (group));
      bairo_pattern_t *mask;
      gdouble v0, v1;

      if (BTK_ORIENTATION_VERTICAL == orientation)
        v1 = widget->allocation.height;
      else
        v1 = widget->allocation.width;

      v0 = v1 - 256;

      if (!btk_widget_get_visible (priv->header))
        v0 = MAX (v0, 0);
      else if (BTK_ORIENTATION_VERTICAL == orientation)
        v0 = MAX (v0, priv->header->allocation.height);
      else
        v0 = MAX (v0, priv->header->allocation.width);

      v1 = MIN (v0 + 256, v1);

      if (BTK_ORIENTATION_VERTICAL == orientation)
        {
          v0 += widget->allocation.y;
          v1 += widget->allocation.y;

          mask = bairo_pattern_create_linear (0.0, v0, 0.0, v1);
        }
      else
        {
          v0 += widget->allocation.x;
          v1 += widget->allocation.x;

          mask = bairo_pattern_create_linear (v0, 0.0, v1, 0.0);
        }

      bairo_pattern_add_color_stop_rgba (mask, 0.00, 0.0, 0.0, 0.0, 1.00);
      bairo_pattern_add_color_stop_rgba (mask, 0.25, 0.0, 0.0, 0.0, 0.25);
      bairo_pattern_add_color_stop_rgba (mask, 0.50, 0.0, 0.0, 0.0, 0.10);
      bairo_pattern_add_color_stop_rgba (mask, 0.75, 0.0, 0.0, 0.0, 0.01);
      bairo_pattern_add_color_stop_rgba (mask, 1.00, 0.0, 0.0, 0.0, 0.00);

      bairo_mask (cr, mask);
      bairo_pattern_destroy (mask);
    }
  else
    bairo_paint (cr);
}

gint
_btk_tool_item_group_get_size_for_limit (BtkToolItemGroup *group,
                                         gint              limit,
                                         gboolean          vertical,
                                         gboolean          animation)
{
  BtkRequisition requisition;
  BtkToolItemGroupPrivate* priv = group->priv;

  btk_widget_size_request (BTK_WIDGET (group), &requisition);

  if (!priv->collapsed || priv->animation_timeout)
    {
      BtkAllocation allocation = { 0, 0, requisition.width, requisition.height };
      BtkRequisition inquery;

      if (vertical)
        allocation.width = limit;
      else
        allocation.height = limit;

      btk_tool_item_group_real_size_query (BTK_WIDGET (group),
                                           &allocation, &inquery);

      if (vertical)
        inquery.height -= requisition.height;
      else
        inquery.width -= requisition.width;

      if (priv->animation_timeout && animation)
        {
          gint64 timestamp = btk_tool_item_group_get_animation_timestamp (group);

          timestamp = MIN (timestamp, ANIMATION_DURATION);

          if (priv->collapsed)
            timestamp = ANIMATION_DURATION - timestamp;

          if (vertical)
            {
              inquery.height *= timestamp;
              inquery.height /= ANIMATION_DURATION;
            }
          else
            {
              inquery.width *= timestamp;
              inquery.width /= ANIMATION_DURATION;
            }
        }

      if (vertical)
        requisition.height += inquery.height;
      else
        requisition.width += inquery.width;
    }

  return (vertical ? requisition.height : requisition.width);
}

gint
_btk_tool_item_group_get_height_for_width (BtkToolItemGroup *group,
                                           gint              width)
{
  return _btk_tool_item_group_get_size_for_limit (group, width, TRUE, group->priv->animation);
}

gint
_btk_tool_item_group_get_width_for_height (BtkToolItemGroup *group,
                                           gint              height)
{
  return _btk_tool_item_group_get_size_for_limit (group, height, FALSE, TRUE);
}

static void
btk_tool_palette_reconfigured_foreach_item (BtkWidget *child,
                                            gpointer   data)
{
  if (BTK_IS_TOOL_ITEM (child))
    btk_tool_item_toolbar_reconfigured (BTK_TOOL_ITEM (child));
}


void
_btk_tool_item_group_palette_reconfigured (BtkToolItemGroup *group)
{
  btk_container_foreach (BTK_CONTAINER (group),
                         btk_tool_palette_reconfigured_foreach_item,
                         NULL);

  btk_tool_item_group_header_adjust_style (group);
}


#define __BTK_TOOL_ITEM_GROUP_C__
#include "btkaliasdef.c"
