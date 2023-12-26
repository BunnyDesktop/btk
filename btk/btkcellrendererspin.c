/* BtkCellRendererSpin
 * Copyright (C) 2004 Lorenzo Gil Sanchez
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
 *
 * Authors: Lorenzo Gil Sanchez    <lgs@sicem.biz>
 *          Carlos Garnacho Parro  <carlosg@bunny.org>
 */

#include "config.h"
#include <bdk/bdkkeysyms.h>
#include "btkintl.h"
#include "btkprivate.h"
#include "btkspinbutton.h"
#include "btkcellrendererspin.h"
#include "btkalias.h"

#define BTK_CELL_RENDERER_SPIN_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), BTK_TYPE_CELL_RENDERER_SPIN, BtkCellRendererSpinPrivate))

struct _BtkCellRendererSpinPrivate
{
  BtkAdjustment *adjustment;
  gdouble climb_rate;
  guint   digits;
};

static void btk_cell_renderer_spin_finalize   (GObject                  *object);

static void btk_cell_renderer_spin_get_property (GObject      *object,
						 guint         prop_id,
						 GValue       *value,
						 GParamSpec   *spec);
static void btk_cell_renderer_spin_set_property (GObject      *object,
						 guint         prop_id,
						 const GValue *value,
						 GParamSpec   *spec);

static BtkCellEditable * btk_cell_renderer_spin_start_editing (BtkCellRenderer     *cell,
							       BdkEvent            *event,
							       BtkWidget           *widget,
							       const gchar         *path,
							       BdkRectangle        *background_area,
							       BdkRectangle        *cell_area,
							       BtkCellRendererState flags);
enum {
  PROP_0,
  PROP_ADJUSTMENT,
  PROP_CLIMB_RATE,
  PROP_DIGITS
};

#define BTK_CELL_RENDERER_SPIN_PATH "btk-cell-renderer-spin-path"

G_DEFINE_TYPE (BtkCellRendererSpin, btk_cell_renderer_spin, BTK_TYPE_CELL_RENDERER_TEXT)


static void
btk_cell_renderer_spin_class_init (BtkCellRendererSpinClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BtkCellRendererClass *cell_class = BTK_CELL_RENDERER_CLASS (klass);

  object_class->finalize     = btk_cell_renderer_spin_finalize;
  object_class->get_property = btk_cell_renderer_spin_get_property;
  object_class->set_property = btk_cell_renderer_spin_set_property;

  cell_class->start_editing  = btk_cell_renderer_spin_start_editing;

  /**
   * BtkCellRendererSpin:adjustment:
   *
   * The adjustment that holds the value of the spinbutton. 
   * This must be non-%NULL for the cell renderer to be editable.
   *
   * Since: 2.10
   */
  g_object_class_install_property (object_class,
				   PROP_ADJUSTMENT,
				   g_param_spec_object ("adjustment",
							P_("Adjustment"),
							P_("The adjustment that holds the value of the spinbutton."),
							BTK_TYPE_ADJUSTMENT,
							BTK_PARAM_READWRITE));


  /**
   * BtkCellRendererSpin:climb-rate:
   *
   * The acceleration rate when you hold down a button.
   *
   * Since: 2.10
   */
  g_object_class_install_property (object_class,
				   PROP_CLIMB_RATE,
				   g_param_spec_double ("climb-rate",
							P_("Climb rate"),
							P_("The acceleration rate when you hold down a button"),
							0.0, G_MAXDOUBLE, 0.0,
							BTK_PARAM_READWRITE));  
  /**
   * BtkCellRendererSpin:digits:
   *
   * The number of decimal places to display.
   *
   * Since: 2.10
   */
  g_object_class_install_property (object_class,
				   PROP_DIGITS,
				   g_param_spec_uint ("digits",
						      P_("Digits"),
						      P_("The number of decimal places to display"),
						      0, 20, 0,
						      BTK_PARAM_READWRITE));  

  g_type_class_add_private (object_class, sizeof (BtkCellRendererSpinPrivate));
}

static void
btk_cell_renderer_spin_init (BtkCellRendererSpin *self)
{
  BtkCellRendererSpinPrivate *priv;

  priv = BTK_CELL_RENDERER_SPIN_GET_PRIVATE (self);

  priv->adjustment = NULL;
  priv->climb_rate = 0.0;
  priv->digits = 0;
}

static void
btk_cell_renderer_spin_finalize (GObject *object)
{
  BtkCellRendererSpinPrivate *priv;

  priv = BTK_CELL_RENDERER_SPIN_GET_PRIVATE (object);

  if (priv && priv->adjustment)
    g_object_unref (priv->adjustment);

  G_OBJECT_CLASS (btk_cell_renderer_spin_parent_class)->finalize (object);
}

static void
btk_cell_renderer_spin_get_property (GObject      *object,
				     guint         prop_id,
				     GValue       *value,
				     GParamSpec   *pspec)
{
  BtkCellRendererSpin *renderer;
  BtkCellRendererSpinPrivate *priv;

  renderer = BTK_CELL_RENDERER_SPIN (object);
  priv = BTK_CELL_RENDERER_SPIN_GET_PRIVATE (renderer);

  switch (prop_id)
    {
    case PROP_ADJUSTMENT:
      g_value_set_object (value, priv->adjustment);
      break;
    case PROP_CLIMB_RATE:
      g_value_set_double (value, priv->climb_rate);
      break;
    case PROP_DIGITS:
      g_value_set_uint (value, priv->digits);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_cell_renderer_spin_set_property (GObject      *object,
				     guint         prop_id,
				     const GValue *value,
				     GParamSpec   *pspec)
{
  BtkCellRendererSpin *renderer;
  BtkCellRendererSpinPrivate *priv;
  GObject *obj;

  renderer = BTK_CELL_RENDERER_SPIN (object);
  priv = BTK_CELL_RENDERER_SPIN_GET_PRIVATE (renderer);

  switch (prop_id)
    {
    case PROP_ADJUSTMENT:
      obj = g_value_get_object (value);

      if (priv->adjustment)
	{
	  g_object_unref (priv->adjustment);
	  priv->adjustment = NULL;
	}

      if (obj)
	priv->adjustment = g_object_ref_sink (obj);
      break;
    case PROP_CLIMB_RATE:
      priv->climb_rate = g_value_get_double (value);
      break;
    case PROP_DIGITS:
      priv->digits = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static gboolean
btk_cell_renderer_spin_focus_out_event (BtkWidget *widget,
					BdkEvent  *event,
					gpointer   data)
{
  const gchar *path;
  const gchar *new_text;
  gboolean canceled;

  g_object_get (widget,
                "editing-canceled", &canceled,
                NULL);

  g_signal_handlers_disconnect_by_func (widget,
					btk_cell_renderer_spin_focus_out_event,
					data);

  btk_cell_renderer_stop_editing (BTK_CELL_RENDERER (data), canceled);

  if (!canceled)
    {
      path = g_object_get_data (G_OBJECT (widget), BTK_CELL_RENDERER_SPIN_PATH);

      new_text = btk_entry_get_text (BTK_ENTRY (widget));
      g_signal_emit_by_name (data, "edited", path, new_text);
    }
  
  return FALSE;
}

static gboolean
btk_cell_renderer_spin_key_press_event (BtkWidget   *widget,
					BdkEventKey *event,
					gpointer     data)
{
  if (event->state == 0)
    {
      if (event->keyval == BDK_Up)
	{
	  btk_spin_button_spin (BTK_SPIN_BUTTON (widget), BTK_SPIN_STEP_FORWARD, 1);
	  return TRUE;
	}
      else if (event->keyval == BDK_Down)
	{
	  btk_spin_button_spin (BTK_SPIN_BUTTON (widget), BTK_SPIN_STEP_BACKWARD, 1);
	  return TRUE;
	}
    }

  return FALSE;
}

static gboolean
btk_cell_renderer_spin_button_press_event (BtkWidget      *widget,
                                           BdkEventButton *event,
                                           gpointer        user_data)
{
  /* Block 2BUTTON and 3BUTTON here, so that they won't be eaten
   * by tree view.
   */
  if (event->type == BDK_2BUTTON_PRESS
      || event->type == BDK_3BUTTON_PRESS)
    return TRUE;

  return FALSE;
}

static BtkCellEditable *
btk_cell_renderer_spin_start_editing (BtkCellRenderer     *cell,
				      BdkEvent            *event,
				      BtkWidget           *widget,
				      const gchar         *path,
				      BdkRectangle        *background_area,
				      BdkRectangle        *cell_area,
				      BtkCellRendererState flags)
{
  BtkCellRendererSpinPrivate *priv;
  BtkCellRendererText *cell_text;
  BtkWidget *spin;

  cell_text = BTK_CELL_RENDERER_TEXT (cell);
  priv = BTK_CELL_RENDERER_SPIN_GET_PRIVATE (cell);

  if (!cell_text->editable)
    return NULL;

  if (!priv->adjustment)
    return NULL;

  spin = btk_spin_button_new (priv->adjustment,
			      priv->climb_rate, priv->digits);

  g_signal_connect (spin, "button-press-event",
                    G_CALLBACK (btk_cell_renderer_spin_button_press_event),
                    NULL);

  if (cell_text->text)
    btk_spin_button_set_value (BTK_SPIN_BUTTON (spin),
			       g_ascii_strtod (cell_text->text, NULL));

  g_object_set_data_full (G_OBJECT (spin), BTK_CELL_RENDERER_SPIN_PATH,
			  g_strdup (path), g_free);

  g_signal_connect (G_OBJECT (spin), "focus-out-event",
		    G_CALLBACK (btk_cell_renderer_spin_focus_out_event),
		    cell);
  g_signal_connect (G_OBJECT (spin), "key-press-event",
		    G_CALLBACK (btk_cell_renderer_spin_key_press_event),
		    cell);

  btk_widget_show (spin);

  return BTK_CELL_EDITABLE (spin);
}

/**
 * btk_cell_renderer_spin_new:
 *
 * Creates a new #BtkCellRendererSpin. 
 *
 * Returns: a new #BtkCellRendererSpin
 *
 * Since: 2.10
 */
BtkCellRenderer *
btk_cell_renderer_spin_new (void)
{
  return g_object_new (BTK_TYPE_CELL_RENDERER_SPIN, NULL);
}


#define __BTK_CELL_RENDERER_SPIN_C__
#include  "btkaliasdef.c"
