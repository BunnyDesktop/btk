/* btkcellrenderertoggle.c
 * Copyright (C) 2000  Red Hat, Inc.,  Jonathan Blandford <jrb@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include <stdlib.h>
#include "btkcellrenderertoggle.h"
#include "btkintl.h"
#include "btkmarshalers.h"
#include "btkprivate.h"
#include "btktreeprivate.h"
#include "btkalias.h"

static void btk_cell_renderer_toggle_get_property  (BObject                    *object,
						    buint                       param_id,
						    BValue                     *value,
						    BParamSpec                 *pspec);
static void btk_cell_renderer_toggle_set_property  (BObject                    *object,
						    buint                       param_id,
						    const BValue               *value,
						    BParamSpec                 *pspec);
static void btk_cell_renderer_toggle_get_size   (BtkCellRenderer            *cell,
						 BtkWidget                  *widget,
 						 BdkRectangle               *cell_area,
						 bint                       *x_offset,
						 bint                       *y_offset,
						 bint                       *width,
						 bint                       *height);
static void btk_cell_renderer_toggle_render     (BtkCellRenderer            *cell,
						 BdkWindow                  *window,
						 BtkWidget                  *widget,
						 BdkRectangle               *background_area,
						 BdkRectangle               *cell_area,
						 BdkRectangle               *expose_area,
						 BtkCellRendererState        flags);
static bboolean btk_cell_renderer_toggle_activate  (BtkCellRenderer            *cell,
						    BdkEvent                   *event,
						    BtkWidget                  *widget,
						    const bchar                *path,
						    BdkRectangle               *background_area,
						    BdkRectangle               *cell_area,
						    BtkCellRendererState        flags);


enum {
  TOGGLED,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_ACTIVATABLE,
  PROP_ACTIVE,
  PROP_RADIO,
  PROP_INCONSISTENT,
  PROP_INDICATOR_SIZE
};

#define TOGGLE_WIDTH 13

static buint toggle_cell_signals[LAST_SIGNAL] = { 0 };

#define BTK_CELL_RENDERER_TOGGLE_GET_PRIVATE(obj) (B_TYPE_INSTANCE_GET_PRIVATE ((obj), BTK_TYPE_CELL_RENDERER_TOGGLE, BtkCellRendererTogglePrivate))

typedef struct _BtkCellRendererTogglePrivate BtkCellRendererTogglePrivate;
struct _BtkCellRendererTogglePrivate
{
  bint indicator_size;

  buint inconsistent : 1;
};


G_DEFINE_TYPE (BtkCellRendererToggle, btk_cell_renderer_toggle, BTK_TYPE_CELL_RENDERER)

static void
btk_cell_renderer_toggle_init (BtkCellRendererToggle *celltoggle)
{
  BtkCellRendererTogglePrivate *priv;

  priv = BTK_CELL_RENDERER_TOGGLE_GET_PRIVATE (celltoggle);

  celltoggle->activatable = TRUE;
  celltoggle->active = FALSE;
  celltoggle->radio = FALSE;

  BTK_CELL_RENDERER (celltoggle)->mode = BTK_CELL_RENDERER_MODE_ACTIVATABLE;
  BTK_CELL_RENDERER (celltoggle)->xpad = 2;
  BTK_CELL_RENDERER (celltoggle)->ypad = 2;

  priv->indicator_size = TOGGLE_WIDTH;
  priv->inconsistent = FALSE;
}

static void
btk_cell_renderer_toggle_class_init (BtkCellRendererToggleClass *class)
{
  BObjectClass *object_class = B_OBJECT_CLASS (class);
  BtkCellRendererClass *cell_class = BTK_CELL_RENDERER_CLASS (class);

  object_class->get_property = btk_cell_renderer_toggle_get_property;
  object_class->set_property = btk_cell_renderer_toggle_set_property;

  cell_class->get_size = btk_cell_renderer_toggle_get_size;
  cell_class->render = btk_cell_renderer_toggle_render;
  cell_class->activate = btk_cell_renderer_toggle_activate;
  
  g_object_class_install_property (object_class,
				   PROP_ACTIVE,
				   g_param_spec_boolean ("active",
							 P_("Toggle state"),
							 P_("The toggle state of the button"),
							 FALSE,
							 BTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
		                   PROP_INCONSISTENT,
				   g_param_spec_boolean ("inconsistent",
					                 P_("Inconsistent state"),
							 P_("The inconsistent state of the button"),
							 FALSE,
							 BTK_PARAM_READWRITE));
  
  g_object_class_install_property (object_class,
				   PROP_ACTIVATABLE,
				   g_param_spec_boolean ("activatable",
							 P_("Activatable"),
							 P_("The toggle button can be activated"),
							 TRUE,
							 BTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_RADIO,
				   g_param_spec_boolean ("radio",
							 P_("Radio state"),
							 P_("Draw the toggle button as a radio button"),
							 FALSE,
							 BTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_INDICATOR_SIZE,
				   g_param_spec_int ("indicator-size",
						     P_("Indicator size"),
						     P_("Size of check or radio indicator"),
						     0,
						     B_MAXINT,
						     TOGGLE_WIDTH,
						     BTK_PARAM_READWRITE));

  
  /**
   * BtkCellRendererToggle::toggled:
   * @cell_renderer: the object which received the signal
   * @path: string representation of #BtkTreePath describing the 
   *        event location
   *
   * The ::toggled signal is emitted when the cell is toggled. 
   **/
  toggle_cell_signals[TOGGLED] =
    g_signal_new (I_("toggled"),
		  B_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkCellRendererToggleClass, toggled),
		  NULL, NULL,
		  _btk_marshal_VOID__STRING,
		  B_TYPE_NONE, 1,
		  B_TYPE_STRING);

  g_type_class_add_private (object_class, sizeof (BtkCellRendererTogglePrivate));
}

static void
btk_cell_renderer_toggle_get_property (BObject     *object,
				       buint        param_id,
				       BValue      *value,
				       BParamSpec  *pspec)
{
  BtkCellRendererToggle *celltoggle = BTK_CELL_RENDERER_TOGGLE (object);
  BtkCellRendererTogglePrivate *priv;

  priv = BTK_CELL_RENDERER_TOGGLE_GET_PRIVATE (object);
  
  switch (param_id)
    {
    case PROP_ACTIVE:
      b_value_set_boolean (value, celltoggle->active);
      break;
    case PROP_INCONSISTENT:
      b_value_set_boolean (value, priv->inconsistent);
      break;
    case PROP_ACTIVATABLE:
      b_value_set_boolean (value, celltoggle->activatable);
      break;
    case PROP_RADIO:
      b_value_set_boolean (value, celltoggle->radio);
      break;
    case PROP_INDICATOR_SIZE:
      b_value_set_int (value, priv->indicator_size);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}


static void
btk_cell_renderer_toggle_set_property (BObject      *object,
				       buint         param_id,
				       const BValue *value,
				       BParamSpec   *pspec)
{
  BtkCellRendererToggle *celltoggle = BTK_CELL_RENDERER_TOGGLE (object);
  BtkCellRendererTogglePrivate *priv;

  priv = BTK_CELL_RENDERER_TOGGLE_GET_PRIVATE (object);

  switch (param_id)
    {
    case PROP_ACTIVE:
      celltoggle->active = b_value_get_boolean (value);
      break;
    case PROP_INCONSISTENT:
      priv->inconsistent = b_value_get_boolean (value);
      break;
    case PROP_ACTIVATABLE:
      celltoggle->activatable = b_value_get_boolean (value);
      break;
    case PROP_RADIO:
      celltoggle->radio = b_value_get_boolean (value);
      break;
    case PROP_INDICATOR_SIZE:
      priv->indicator_size = b_value_get_int (value);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

/**
 * btk_cell_renderer_toggle_new:
 *
 * Creates a new #BtkCellRendererToggle. Adjust rendering
 * parameters using object properties. Object properties can be set
 * globally (with g_object_set()). Also, with #BtkTreeViewColumn, you
 * can bind a property to a value in a #BtkTreeModel. For example, you
 * can bind the "active" property on the cell renderer to a boolean value
 * in the model, thus causing the check button to reflect the state of
 * the model.
 *
 * Return value: the new cell renderer
 **/
BtkCellRenderer *
btk_cell_renderer_toggle_new (void)
{
  return g_object_new (BTK_TYPE_CELL_RENDERER_TOGGLE, NULL);
}

static void
btk_cell_renderer_toggle_get_size (BtkCellRenderer *cell,
				   BtkWidget       *widget,
				   BdkRectangle    *cell_area,
				   bint            *x_offset,
				   bint            *y_offset,
				   bint            *width,
				   bint            *height)
{
  bint calc_width;
  bint calc_height;
  BtkCellRendererTogglePrivate *priv;

  priv = BTK_CELL_RENDERER_TOGGLE_GET_PRIVATE (cell);

  calc_width = (bint) cell->xpad * 2 + priv->indicator_size;
  calc_height = (bint) cell->ypad * 2 + priv->indicator_size;

  if (width)
    *width = calc_width;

  if (height)
    *height = calc_height;

  if (cell_area)
    {
      if (x_offset)
	{
	  *x_offset = ((btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL) ?
		       (1.0 - cell->xalign) : cell->xalign) * (cell_area->width - calc_width);
	  *x_offset = MAX (*x_offset, 0);
	}
      if (y_offset)
	{
	  *y_offset = cell->yalign * (cell_area->height - calc_height);
	  *y_offset = MAX (*y_offset, 0);
	}
    }
  else
    {
      if (x_offset) *x_offset = 0;
      if (y_offset) *y_offset = 0;
    }
}

static void
btk_cell_renderer_toggle_render (BtkCellRenderer      *cell,
				 BdkDrawable          *window,
				 BtkWidget            *widget,
				 BdkRectangle         *background_area,
				 BdkRectangle         *cell_area,
				 BdkRectangle         *expose_area,
				 BtkCellRendererState  flags)
{
  BtkCellRendererToggle *celltoggle = (BtkCellRendererToggle *) cell;
  BtkCellRendererTogglePrivate *priv;
  bint width, height;
  bint x_offset, y_offset;
  BtkShadowType shadow;
  BtkStateType state = 0;

  priv = BTK_CELL_RENDERER_TOGGLE_GET_PRIVATE (cell);

  btk_cell_renderer_toggle_get_size (cell, widget, cell_area,
				     &x_offset, &y_offset,
				     &width, &height);
  width -= cell->xpad*2;
  height -= cell->ypad*2;

  if (width <= 0 || height <= 0)
    return;

  if (priv->inconsistent)
    shadow = BTK_SHADOW_ETCHED_IN;
  else
    shadow = celltoggle->active ? BTK_SHADOW_IN : BTK_SHADOW_OUT;

  if (btk_widget_get_state (widget) == BTK_STATE_INSENSITIVE || !cell->sensitive)
    {
      state = BTK_STATE_INSENSITIVE;
    }
  else if ((flags & BTK_CELL_RENDERER_SELECTED) == BTK_CELL_RENDERER_SELECTED)
    {
      if (btk_widget_has_focus (widget))
	state = BTK_STATE_SELECTED;
      else
	state = BTK_STATE_ACTIVE;
    }
  else
    {
      if (celltoggle->activatable)
        state = BTK_STATE_NORMAL;
      else
        state = BTK_STATE_INSENSITIVE;
    }

  if (celltoggle->radio)
    {
      btk_paint_option (widget->style,
                        window,
                        state, shadow,
                        expose_area, widget, "cellradio",
                        cell_area->x + x_offset + cell->xpad,
                        cell_area->y + y_offset + cell->ypad,
                        width, height);
    }
  else
    {
      btk_paint_check (widget->style,
                       window,
                       state, shadow,
                       expose_area, widget, "cellcheck",
                       cell_area->x + x_offset + cell->xpad,
                       cell_area->y + y_offset + cell->ypad,
                       width, height);
    }
}

static bint
btk_cell_renderer_toggle_activate (BtkCellRenderer      *cell,
				   BdkEvent             *event,
				   BtkWidget            *widget,
				   const bchar          *path,
				   BdkRectangle         *background_area,
				   BdkRectangle         *cell_area,
				   BtkCellRendererState  flags)
{
  BtkCellRendererToggle *celltoggle;
  
  celltoggle = BTK_CELL_RENDERER_TOGGLE (cell);
  if (celltoggle->activatable)
    {
      g_signal_emit (cell, toggle_cell_signals[TOGGLED], 0, path);
      return TRUE;
    }

  return FALSE;
}

/**
 * btk_cell_renderer_toggle_set_radio:
 * @toggle: a #BtkCellRendererToggle
 * @radio: %TRUE to make the toggle look like a radio button
 * 
 * If @radio is %TRUE, the cell renderer renders a radio toggle
 * (i.e. a toggle in a group of mutually-exclusive toggles).
 * If %FALSE, it renders a check toggle (a standalone boolean option).
 * This can be set globally for the cell renderer, or changed just
 * before rendering each cell in the model (for #BtkTreeView, you set
 * up a per-row setting using #BtkTreeViewColumn to associate model
 * columns with cell renderer properties).
 **/
void
btk_cell_renderer_toggle_set_radio (BtkCellRendererToggle *toggle,
				    bboolean               radio)
{
  g_return_if_fail (BTK_IS_CELL_RENDERER_TOGGLE (toggle));

  toggle->radio = radio;
}

/**
 * btk_cell_renderer_toggle_get_radio:
 * @toggle: a #BtkCellRendererToggle
 *
 * Returns whether we're rendering radio toggles rather than checkboxes. 
 * 
 * Return value: %TRUE if we're rendering radio toggles rather than checkboxes
 **/
bboolean
btk_cell_renderer_toggle_get_radio (BtkCellRendererToggle *toggle)
{
  g_return_val_if_fail (BTK_IS_CELL_RENDERER_TOGGLE (toggle), FALSE);

  return toggle->radio;
}

/**
 * btk_cell_renderer_toggle_get_active:
 * @toggle: a #BtkCellRendererToggle
 *
 * Returns whether the cell renderer is active. See
 * btk_cell_renderer_toggle_set_active().
 *
 * Return value: %TRUE if the cell renderer is active.
 **/
bboolean
btk_cell_renderer_toggle_get_active (BtkCellRendererToggle *toggle)
{
  g_return_val_if_fail (BTK_IS_CELL_RENDERER_TOGGLE (toggle), FALSE);

  return toggle->active;
}

/**
 * btk_cell_renderer_toggle_set_active:
 * @toggle: a #BtkCellRendererToggle.
 * @setting: the value to set.
 *
 * Activates or deactivates a cell renderer.
 **/
void
btk_cell_renderer_toggle_set_active (BtkCellRendererToggle *toggle,
				     bboolean               setting)
{
  g_return_if_fail (BTK_IS_CELL_RENDERER_TOGGLE (toggle));

  g_object_set (toggle, "active", setting ? TRUE : FALSE, NULL);
}

/**
 * btk_cell_renderer_toggle_get_activatable:
 * @toggle: a #BtkCellRendererToggle
 *
 * Returns whether the cell renderer is activatable. See
 * btk_cell_renderer_toggle_set_activatable().
 *
 * Return value: %TRUE if the cell renderer is activatable.
 *
 * Since: 2.18
 **/
bboolean
btk_cell_renderer_toggle_get_activatable (BtkCellRendererToggle *toggle)
{
  g_return_val_if_fail (BTK_IS_CELL_RENDERER_TOGGLE (toggle), FALSE);

  return toggle->activatable;
}

/**
 * btk_cell_renderer_toggle_set_activatable:
 * @toggle: a #BtkCellRendererToggle.
 * @setting: the value to set.
 *
 * Makes the cell renderer activatable.
 *
 * Since: 2.18
 **/
void
btk_cell_renderer_toggle_set_activatable (BtkCellRendererToggle *toggle,
                                          bboolean               setting)
{
  g_return_if_fail (BTK_IS_CELL_RENDERER_TOGGLE (toggle));

  if (toggle->activatable != setting)
    {
      toggle->activatable = setting ? TRUE : FALSE;
      g_object_notify (B_OBJECT (toggle), "activatable");
    }
}

#define __BTK_CELL_RENDERER_TOGGLE_C__
#include "btkaliasdef.c"
