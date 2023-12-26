/* BTK - The GIMP Toolkit
 *
 * Copyright (C) 2009 Matthias Clasen <mclasen@redhat.com>
 * Copyright (C) 2008 Richard Hughes <richard@hughsie.com>
 * Copyright (C) 2009 Bastien Nocera <hadess@hadess.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.         See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 */

/*
 * Modified by the BTK+ Team and others 2007.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/.
 */

#include "config.h"

#include "btkcellrendererspinner.h"
#include "btkiconfactory.h"
#include "btkicontheme.h"
#include "btkintl.h"
#include "btkalias.h"


/**
 * SECTION:btkcellrendererspinner
 * @Short_description: Renders a spinning animation in a cell
 * @Title: BtkCellRendererSpinner
 * @See_also: #BtkSpinner, #BtkCellRendererProgress
 *
 * BtkCellRendererSpinner renders a spinning animation in a cell, very
 * similar to #BtkSpinner. It can often be used as an alternative
 * to a #BtkCellRendererProgress for displaying indefinite activity,
 * instead of actual progress.
 *
 * To start the animation in a cell, set the #BtkCellRendererSpinner:active
 * property to %TRUE and increment the #BtkCellRendererSpinner:pulse property
 * at regular intervals. The usual way to set the cell renderer properties
 * for each cell is to bind them to columns in your tree model using e.g.
 * btk_tree_view_column_add_attribute().
 */


enum {
  PROP_0,
  PROP_ACTIVE,
  PROP_PULSE,
  PROP_SIZE
};

struct _BtkCellRendererSpinnerPrivate
{
  bboolean active;
  buint pulse;
  BtkIconSize icon_size, old_icon_size;
  bint size;
};

#define BTK_CELL_RENDERER_SPINNER_GET_PRIVATE(object)        \
                (B_TYPE_INSTANCE_GET_PRIVATE ((object),        \
                        BTK_TYPE_CELL_RENDERER_SPINNER, \
                        BtkCellRendererSpinnerPrivate))

static void btk_cell_renderer_spinner_get_property (BObject         *object,
                                                    buint            param_id,
                                                    BValue          *value,
                                                    BParamSpec      *pspec);
static void btk_cell_renderer_spinner_set_property (BObject         *object,
                                                    buint            param_id,
                                                    const BValue    *value,
                                                    BParamSpec      *pspec);
static void btk_cell_renderer_spinner_get_size     (BtkCellRenderer *cell,
                                                    BtkWidget       *widget,
                                                    BdkRectangle    *cell_area,
                                                    bint            *x_offset,
                                                    bint            *y_offset,
                                                    bint            *width,
                                                    bint            *height);
static void btk_cell_renderer_spinner_render       (BtkCellRenderer *cell,
                                                    BdkWindow       *window,
                                                    BtkWidget       *widget,
                                                    BdkRectangle    *background_area,
                                                    BdkRectangle    *cell_area,
                                                    BdkRectangle    *expose_area,
                                                    buint            flags);

G_DEFINE_TYPE (BtkCellRendererSpinner, btk_cell_renderer_spinner, BTK_TYPE_CELL_RENDERER)

static void
btk_cell_renderer_spinner_class_init (BtkCellRendererSpinnerClass *klass)
{
  BObjectClass *object_class = B_OBJECT_CLASS (klass);
  BtkCellRendererClass *cell_class = BTK_CELL_RENDERER_CLASS (klass);

  object_class->get_property = btk_cell_renderer_spinner_get_property;
  object_class->set_property = btk_cell_renderer_spinner_set_property;

  cell_class->get_size = btk_cell_renderer_spinner_get_size;
  cell_class->render = btk_cell_renderer_spinner_render;

  /* BtkCellRendererSpinner:active:
   *
   * Whether the spinner is active (ie. shown) in the cell
   *
   * Since: 2.20
   */
  g_object_class_install_property (object_class,
                                   PROP_ACTIVE,
                                   g_param_spec_boolean ("active",
                                                         P_("Active"),
                                                         P_("Whether the spinner is active (ie. shown) in the cell"),
                                                         FALSE,
                                                         G_PARAM_READWRITE));
  /**
   * BtkCellRendererSpinner:pulse:
   *
   * Pulse of the spinner. Increment this value to draw the next frame of the
   * spinner animation. Usually, you would update this value in a timeout.
   *
   * The #BtkSpinner widget draws one full cycle of the animation per second by default.
   * You can learn about the number of frames used by the theme
   * by looking at the #BtkSpinner:num-steps style property and the duration
   * of the cycle by looking at #BtkSpinner:cycle-duration.
   *
   * Since: 2.20
   */
  g_object_class_install_property (object_class,
                                   PROP_PULSE,
                                   g_param_spec_uint ("pulse",
                                                      P_("Pulse"),
                                                      P_("Pulse of the spinner"),
                                                      0, B_MAXUINT, 0,
                                                      G_PARAM_READWRITE));
  /**
   * BtkCellRendererSpinner:size:
   *
   * The #BtkIconSize value that specifies the size of the rendered spinner.
   *
   * Since: 2.20
   */
  g_object_class_install_property (object_class,
                                   PROP_SIZE,
                                   g_param_spec_enum ("size",
                                                      P_("Size"),
                                                      P_("The BtkIconSize value that specifies the size of the rendered spinner"),
                                                      BTK_TYPE_ICON_SIZE, BTK_ICON_SIZE_MENU,
                                                      G_PARAM_READWRITE));


  g_type_class_add_private (object_class, sizeof (BtkCellRendererSpinnerPrivate));
}

static void
btk_cell_renderer_spinner_init (BtkCellRendererSpinner *cell)
{
  cell->priv = BTK_CELL_RENDERER_SPINNER_GET_PRIVATE (cell);
  cell->priv->pulse = 0;
  cell->priv->old_icon_size = BTK_ICON_SIZE_INVALID;
  cell->priv->icon_size = BTK_ICON_SIZE_MENU;
}

/**
 * btk_cell_renderer_spinner_new
 *
 * Returns a new cell renderer which will show a spinner to indicate
 * activity.
 *
 * Return value: a new #BtkCellRenderer
 *
 * Since: 2.20
 */
BtkCellRenderer *
btk_cell_renderer_spinner_new (void)
{
  return g_object_new (BTK_TYPE_CELL_RENDERER_SPINNER, NULL);
}

static void
btk_cell_renderer_spinner_update_size (BtkCellRendererSpinner *cell,
                                       BtkWidget              *widget)
{
  BtkCellRendererSpinnerPrivate *priv = cell->priv;
  BdkScreen *screen;
  BtkIconTheme *icon_theme;
  BtkSettings *settings;

  if (cell->priv->old_icon_size == cell->priv->icon_size)
    return;

  screen = btk_widget_get_screen (BTK_WIDGET (widget));
  icon_theme = btk_icon_theme_get_for_screen (screen);
  settings = btk_settings_get_for_screen (screen);

  if (!btk_icon_size_lookup_for_settings (settings, priv->icon_size, &priv->size, NULL))
    {
      g_warning ("Invalid icon size %u\n", priv->icon_size);
      priv->size = 24;
    }
}

static void
btk_cell_renderer_spinner_get_property (BObject    *object,
                                        buint       param_id,
                                        BValue     *value,
                                        BParamSpec *pspec)
{
  BtkCellRendererSpinner *cell = BTK_CELL_RENDERER_SPINNER (object);
  BtkCellRendererSpinnerPrivate *priv = cell->priv;

  switch (param_id)
    {
      case PROP_ACTIVE:
        b_value_set_boolean (value, priv->active);
        break;
      case PROP_PULSE:
        b_value_set_uint (value, priv->pulse);
        break;
      case PROP_SIZE:
        b_value_set_enum (value, priv->icon_size);
        break;
      default:
        B_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
    }
}

static void
btk_cell_renderer_spinner_set_property (BObject      *object,
                                        buint         param_id,
                                        const BValue *value,
                                        BParamSpec   *pspec)
{
  BtkCellRendererSpinner *cell = BTK_CELL_RENDERER_SPINNER (object);
  BtkCellRendererSpinnerPrivate *priv = cell->priv;

  switch (param_id)
    {
      case PROP_ACTIVE:
        priv->active = b_value_get_boolean (value);
        break;
      case PROP_PULSE:
        priv->pulse = b_value_get_uint (value);
        break;
      case PROP_SIZE:
        priv->old_icon_size = priv->icon_size;
        priv->icon_size = b_value_get_enum (value);
        break;
      default:
        B_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
    }
}

static void
btk_cell_renderer_spinner_get_size (BtkCellRenderer *cellr,
                                    BtkWidget       *widget,
                                    BdkRectangle    *cell_area,
                                    bint            *x_offset,
                                    bint            *y_offset,
                                    bint            *width,
                                    bint            *height)
{
  BtkCellRendererSpinner *cell = BTK_CELL_RENDERER_SPINNER (cellr);
  BtkCellRendererSpinnerPrivate *priv = cell->priv;
  bdouble align;
  bint w, h;
  bint xpad, ypad;
  bfloat xalign, yalign;
  bboolean rtl;

  rtl = btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL;

  btk_cell_renderer_spinner_update_size (cell, widget);

  g_object_get (cellr,
                "xpad", &xpad,
                "ypad", &ypad,
                "xalign", &xalign,
                "yalign", &yalign,
                NULL);
  w = h = priv->size;

  if (cell_area)
    {
      if (x_offset)
        {
          align = rtl ? 1.0 - xalign : xalign;
          *x_offset = align * (cell_area->width - w);
          *x_offset = MAX (*x_offset, 0);
        }
      if (y_offset)
        {
          align = rtl ? 1.0 - yalign : yalign;
          *y_offset = align * (cell_area->height - h);
          *y_offset = MAX (*y_offset, 0);
        }
    }
  else
    {
      if (x_offset)
        *x_offset = 0;
      if (y_offset)
        *y_offset = 0;
    }

  if (width)
    *width = w;
  if (height)
    *height = h;
}

static void
btk_cell_renderer_spinner_render (BtkCellRenderer *cellr,
                                  BdkWindow       *window,
                                  BtkWidget       *widget,
                                  BdkRectangle    *background_area,
                                  BdkRectangle    *cell_area,
                                  BdkRectangle    *expose_area,
                                  buint            flags)
{
  BtkCellRendererSpinner *cell = BTK_CELL_RENDERER_SPINNER (cellr);
  BtkCellRendererSpinnerPrivate *priv = cell->priv;
  BtkStateType state;
  BdkRectangle pix_rect;
  BdkRectangle draw_rect;
  bint xpad, ypad;

  if (!priv->active)
    return;

  btk_cell_renderer_spinner_get_size (cellr, widget, cell_area,
                                      &pix_rect.x, &pix_rect.y,
                                      &pix_rect.width, &pix_rect.height);

  g_object_get (cellr,
                "xpad", &xpad,
                "ypad", &ypad,
                NULL);
  pix_rect.x += cell_area->x + xpad;
  pix_rect.y += cell_area->y + ypad;
  pix_rect.width -= xpad * 2;
  pix_rect.height -= ypad * 2;

  if (!bdk_rectangle_intersect (cell_area, &pix_rect, &draw_rect) ||
      !bdk_rectangle_intersect (expose_area, &pix_rect, &draw_rect))
    {
      return;
    }

  state = BTK_STATE_NORMAL;
  if (btk_widget_get_state (widget) == BTK_STATE_INSENSITIVE || !cellr->sensitive)
    {
      state = BTK_STATE_INSENSITIVE;
    }
  else
    {
      if ((flags & BTK_CELL_RENDERER_SELECTED) != 0)
        {
          if (btk_widget_has_focus (widget))
            state = BTK_STATE_SELECTED;
          else
            state = BTK_STATE_ACTIVE;
        }
      else
        state = BTK_STATE_PRELIGHT;
    }

  btk_paint_spinner (widget->style,
                     window,
                     state,
                     expose_area,
                     widget,
                     "cell",
                     priv->pulse,
                     draw_rect.x, draw_rect.y,
                     draw_rect.width, draw_rect.height);
}

#define __BTK_CELL_RENDERER_SPINNER_C__
#include "btkaliasdef.c"
