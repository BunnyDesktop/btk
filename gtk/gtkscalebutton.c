/* BTK - The GIMP Toolkit
 * Copyright (C) 2005 Ronald S. Bultje
 * Copyright (C) 2006, 2007 Christian Persch
 * Copyright (C) 2006 Jan Arne Petersen
 * Copyright (C) 2005-2007 Red Hat, Inc.
 *
 * Authors:
 * - Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * - Bastien Nocera <bnocera@redhat.com>
 * - Jan Arne Petersen <jpetersen@jpetersen.org>
 * - Christian Persch <chpe@svn.gnome.org>
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
 * Modified by the BTK+ Team and others 2007.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/.
 */

#include "config.h"

#ifndef _WIN32
#define _GNU_SOURCE
#endif
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <bdk-pixbuf/bdk-pixbuf.h>
#include <bdk/bdkkeysyms.h>

#include "btkbindings.h"
#include "btkframe.h"
#include "btkmain.h"
#include "btkmarshalers.h"
#include "btkorientable.h"
#include "btkprivate.h"
#include "btkscale.h"
#include "btkscalebutton.h"
#include "btkstock.h"
#include "btkvbox.h"
#include "btkwindow.h"

#include "btkintl.h"
#include "btkalias.h"

#define SCALE_SIZE 100
#define CLICK_TIMEOUT 250

enum
{
  VALUE_CHANGED,
  POPUP,
  POPDOWN,

  LAST_SIGNAL
};

enum
{
  PROP_0,

  PROP_ORIENTATION,
  PROP_VALUE,
  PROP_SIZE,
  PROP_ADJUSTMENT,
  PROP_ICONS
};

#define GET_PRIVATE(obj)        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), BTK_TYPE_SCALE_BUTTON, BtkScaleButtonPrivate))

struct _BtkScaleButtonPrivate
{
  BtkWidget *dock;
  BtkWidget *box;
  BtkWidget *scale;
  BtkWidget *image;

  BtkIconSize size;
  BtkOrientation orientation;

  guint click_id;
  gint click_timeout;
  guint timeout : 1;
  gdouble direction;
  guint32 pop_time;

  gchar **icon_list;

  BtkAdjustment *adjustment; /* needed because it must be settable in init() */
};

static GObject* btk_scale_button_constructor    (GType                  type,
                                                 guint                  n_construct_properties,
                                                 GObjectConstructParam *construct_params);
static void	btk_scale_button_dispose	(GObject             *object);
static void     btk_scale_button_finalize       (GObject             *object);
static void	btk_scale_button_set_property	(GObject             *object,
						 guint                prop_id,
						 const GValue        *value,
						 GParamSpec          *pspec);
static void	btk_scale_button_get_property	(GObject             *object,
						 guint                prop_id,
						 GValue              *value,
						 GParamSpec          *pspec);
static void btk_scale_button_set_orientation_private (BtkScaleButton *button,
                                                      BtkOrientation  orientation);
static gboolean	btk_scale_button_scroll		(BtkWidget           *widget,
						 BdkEventScroll      *event);
static void btk_scale_button_screen_changed	(BtkWidget           *widget,
						 BdkScreen           *previous_screen);
static gboolean	btk_scale_button_press		(BtkWidget           *widget,
						 BdkEventButton      *event);
static gboolean btk_scale_button_key_release	(BtkWidget           *widget,
    						 BdkEventKey         *event);
static void     btk_scale_button_popup          (BtkWidget           *widget);
static void     btk_scale_button_popdown        (BtkWidget           *widget);
static gboolean cb_dock_button_press		(BtkWidget           *widget,
						 BdkEventButton      *event,
						 gpointer             user_data);
static gboolean cb_dock_key_release		(BtkWidget           *widget,
						 BdkEventKey         *event,
						 gpointer             user_data);
static gboolean cb_button_press			(BtkWidget           *widget,
						 BdkEventButton      *event,
						 gpointer             user_data);
static gboolean cb_button_release		(BtkWidget           *widget,
						 BdkEventButton      *event,
						 gpointer             user_data);
static void cb_dock_grab_notify			(BtkWidget           *widget,
						 gboolean             was_grabbed,
						 gpointer             user_data);
static gboolean cb_dock_grab_broken_event	(BtkWidget           *widget,
						 gboolean             was_grabbed,
						 gpointer             user_data);
static void cb_scale_grab_notify		(BtkWidget           *widget,
						 gboolean             was_grabbed,
						 gpointer             user_data);
static void btk_scale_button_update_icon	(BtkScaleButton      *button);
static void btk_scale_button_scale_value_changed(BtkRange            *range);

/* see below for scale definitions */
static BtkWidget *btk_scale_button_scale_new    (BtkScaleButton      *button);

G_DEFINE_TYPE_WITH_CODE (BtkScaleButton, btk_scale_button, BTK_TYPE_BUTTON,
                         G_IMPLEMENT_INTERFACE (BTK_TYPE_ORIENTABLE,
                                                NULL))

static guint signals[LAST_SIGNAL] = { 0, };

static void
btk_scale_button_class_init (BtkScaleButtonClass *klass)
{
  GObjectClass *bobject_class = G_OBJECT_CLASS (klass);
  BtkWidgetClass *widget_class = BTK_WIDGET_CLASS (klass);
  BtkBindingSet *binding_set;

  g_type_class_add_private (klass, sizeof (BtkScaleButtonPrivate));

  bobject_class->constructor = btk_scale_button_constructor;
  bobject_class->finalize = btk_scale_button_finalize;
  bobject_class->dispose = btk_scale_button_dispose;
  bobject_class->set_property = btk_scale_button_set_property;
  bobject_class->get_property = btk_scale_button_get_property;

  widget_class->button_press_event = btk_scale_button_press;
  widget_class->key_release_event = btk_scale_button_key_release;
  widget_class->scroll_event = btk_scale_button_scroll;
  widget_class->screen_changed = btk_scale_button_screen_changed;

  /**
   * BtkScaleButton:orientation:
   *
   * The orientation of the #BtkScaleButton's popup window.
   *
   * Note that since BTK+ 2.16, #BtkScaleButton implements the
   * #BtkOrientable interface which has its own @orientation
   * property. However we redefine the property here in order to
   * override its default horizontal orientation.
   *
   * Since: 2.14
   **/
  g_object_class_override_property (bobject_class,
				    PROP_ORIENTATION,
				    "orientation");

  g_object_class_install_property (bobject_class,
				   PROP_VALUE,
				   g_param_spec_double ("value",
							P_("Value"),
							P_("The value of the scale"),
							-G_MAXDOUBLE,
							G_MAXDOUBLE,
							0,
							BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
				   PROP_SIZE,
				   g_param_spec_enum ("size",
						      P_("Icon size"),
						      P_("The icon size"),
						      BTK_TYPE_ICON_SIZE,
						      BTK_ICON_SIZE_SMALL_TOOLBAR,
						      BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_ADJUSTMENT,
                                   g_param_spec_object ("adjustment",
							P_("Adjustment"),
							P_("The BtkAdjustment that contains the current value of this scale button object"),
                                                        BTK_TYPE_ADJUSTMENT,
                                                        BTK_PARAM_READWRITE));

  /**
   * BtkScaleButton:icons:
   *
   * The names of the icons to be used by the scale button.
   * The first item in the array will be used in the button
   * when the current value is the lowest value, the second
   * item for the highest value. All the subsequent icons will
   * be used for all the other values, spread evenly over the
   * range of values.
   *
   * If there's only one icon name in the @icons array, it will
   * be used for all the values. If only two icon names are in
   * the @icons array, the first one will be used for the bottom
   * 50% of the scale, and the second one for the top 50%.
   *
   * It is recommended to use at least 3 icons so that the
   * #BtkScaleButton reflects the current value of the scale
   * better for the users.
   *
   * Since: 2.12
   */
  g_object_class_install_property (bobject_class,
                                   PROP_ICONS,
                                   g_param_spec_boxed ("icons",
                                                       P_("Icons"),
                                                       P_("List of icon names"),
                                                       G_TYPE_STRV,
                                                       BTK_PARAM_READWRITE));

  /**
   * BtkScaleButton::value-changed:
   * @button: the object which received the signal
   * @value: the new value
   *
   * The ::value-changed signal is emitted when the value field has
   * changed.
   *
   * Since: 2.12
   */
  signals[VALUE_CHANGED] =
    g_signal_new (I_("value-changed"),
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkScaleButtonClass, value_changed),
		  NULL, NULL,
		  _btk_marshal_VOID__DOUBLE,
		  G_TYPE_NONE, 1, G_TYPE_DOUBLE);

  /**
   * BtkScaleButton::popup:
   * @button: the object which received the signal
   *
   * The ::popup signal is a
   * <link linkend="keybinding-signals">keybinding signal</link>
   * which gets emitted to popup the scale widget.
   *
   * The default bindings for this signal are Space, Enter and Return.
   *
   * Since: 2.12
   */
  signals[POPUP] =
    g_signal_new_class_handler (I_("popup"),
                                G_OBJECT_CLASS_TYPE (klass),
                                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                G_CALLBACK (btk_scale_button_popup),
                                NULL, NULL,
                                g_cclosure_marshal_VOID__VOID,
                                G_TYPE_NONE, 0);

  /**
   * BtkScaleButton::popdown:
   * @button: the object which received the signal
   *
   * The ::popdown signal is a
   * <link linkend="keybinding-signals">keybinding signal</link>
   * which gets emitted to popdown the scale widget.
   *
   * The default binding for this signal is Escape.
   *
   * Since: 2.12
   */
  signals[POPDOWN] =
    g_signal_new_class_handler (I_("popdown"),
                                G_OBJECT_CLASS_TYPE (klass),
                                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                G_CALLBACK (btk_scale_button_popdown),
                                NULL, NULL,
                                g_cclosure_marshal_VOID__VOID,
                                G_TYPE_NONE, 0);

  /* Key bindings */
  binding_set = btk_binding_set_by_class (widget_class);

  btk_binding_entry_add_signal (binding_set, BDK_space, 0,
				"popup", 0);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Space, 0,
				"popup", 0);
  btk_binding_entry_add_signal (binding_set, BDK_Return, 0,
				"popup", 0);
  btk_binding_entry_add_signal (binding_set, BDK_ISO_Enter, 0,
				"popup", 0);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Enter, 0,
				"popup", 0);
  btk_binding_entry_add_signal (binding_set, BDK_Escape, 0,
				"popdown", 0);
}

static void
btk_scale_button_init (BtkScaleButton *button)
{
  BtkScaleButtonPrivate *priv;
  BtkWidget *frame;

  button->priv = priv = GET_PRIVATE (button);

  priv->timeout = FALSE;
  priv->click_id = 0;
  priv->click_timeout = CLICK_TIMEOUT;
  priv->orientation = BTK_ORIENTATION_VERTICAL;

  btk_button_set_relief (BTK_BUTTON (button), BTK_RELIEF_NONE);
  btk_button_set_focus_on_click (BTK_BUTTON (button), FALSE);

  /* image */
  priv->image = btk_image_new ();
  btk_container_add (BTK_CONTAINER (button), priv->image);
  btk_widget_show_all (priv->image);

  /* window */
  priv->dock = btk_window_new (BTK_WINDOW_POPUP);
  btk_widget_set_name (priv->dock, "btk-scalebutton-popup-window");
  g_signal_connect (priv->dock, "button-press-event",
		    G_CALLBACK (cb_dock_button_press), button);
  g_signal_connect (priv->dock, "key-release-event",
		    G_CALLBACK (cb_dock_key_release), button);
  g_signal_connect (priv->dock, "grab-notify",
		    G_CALLBACK (cb_dock_grab_notify), button);
  g_signal_connect (priv->dock, "grab-broken-event",
		    G_CALLBACK (cb_dock_grab_broken_event), button);
  btk_window_set_decorated (BTK_WINDOW (priv->dock), FALSE);

  /* frame */
  frame = btk_frame_new (NULL);
  btk_frame_set_shadow_type (BTK_FRAME (frame), BTK_SHADOW_OUT);
  btk_container_add (BTK_CONTAINER (priv->dock), frame);

  /* box for scale and +/- buttons */
  priv->box = btk_vbox_new (FALSE, 0);
  btk_container_add (BTK_CONTAINER (frame), priv->box);

  /* + */
  button->plus_button = btk_button_new_with_label ("+");
  btk_button_set_relief (BTK_BUTTON (button->plus_button), BTK_RELIEF_NONE);
  g_signal_connect (button->plus_button, "button-press-event",
		    G_CALLBACK (cb_button_press), button);
  g_signal_connect (button->plus_button, "button-release-event",
		    G_CALLBACK (cb_button_release), button);
  btk_box_pack_start (BTK_BOX (priv->box), button->plus_button, FALSE, FALSE, 0);

  /* - */
  button->minus_button = btk_button_new_with_label ("-");
  btk_button_set_relief (BTK_BUTTON (button->minus_button), BTK_RELIEF_NONE);
  g_signal_connect (button->minus_button, "button-press-event",
		   G_CALLBACK (cb_button_press), button);
  g_signal_connect (button->minus_button, "button-release-event",
		    G_CALLBACK (cb_button_release), button);
  btk_box_pack_end (BTK_BOX (priv->box), button->minus_button, FALSE, FALSE, 0);

  priv->adjustment = BTK_ADJUSTMENT (btk_adjustment_new (0.0, 0.0, 100.0, 2, 20, 0));
  g_object_ref_sink (priv->adjustment);

  /* the scale */
  priv->scale = btk_scale_button_scale_new (button);
  btk_container_add (BTK_CONTAINER (priv->box), priv->scale);
}

static GObject *
btk_scale_button_constructor (GType                  type,
                              guint                  n_construct_properties,
                              GObjectConstructParam *construct_params)
{
  GObject *object;
  BtkScaleButton *button;
  BtkScaleButtonPrivate *priv;

  object = G_OBJECT_CLASS (btk_scale_button_parent_class)->constructor (type, n_construct_properties, construct_params);

  button = BTK_SCALE_BUTTON (object);

  priv = button->priv;

  /* set button text and size */
  priv->size = BTK_ICON_SIZE_SMALL_TOOLBAR;
  btk_scale_button_update_icon (button);

  return object;
}

static void
btk_scale_button_set_property (GObject       *object,
			       guint          prop_id,
			       const GValue  *value,
			       GParamSpec    *pspec)
{
  BtkScaleButton *button = BTK_SCALE_BUTTON (object);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      btk_scale_button_set_orientation_private (button, g_value_get_enum (value));
      break;
    case PROP_VALUE:
      btk_scale_button_set_value (button, g_value_get_double (value));
      break;
    case PROP_SIZE:
      {
	BtkIconSize size;
	size = g_value_get_enum (value);
	if (button->priv->size != size)
	  {
	    button->priv->size = size;
	    btk_scale_button_update_icon (button);
	  }
      }
      break;
    case PROP_ADJUSTMENT:
      btk_scale_button_set_adjustment (button, g_value_get_object (value));
      break;
    case PROP_ICONS:
      btk_scale_button_set_icons (button,
                                  (const gchar **)g_value_get_boxed (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_scale_button_get_property (GObject     *object,
			       guint        prop_id,
			       GValue      *value,
			       GParamSpec  *pspec)
{
  BtkScaleButton *button = BTK_SCALE_BUTTON (object);
  BtkScaleButtonPrivate *priv = button->priv;

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, priv->orientation);
      break;
    case PROP_VALUE:
      g_value_set_double (value, btk_scale_button_get_value (button));
      break;
    case PROP_SIZE:
      g_value_set_enum (value, priv->size);
      break;
    case PROP_ADJUSTMENT:
      g_value_set_object (value, btk_scale_button_get_adjustment (button));
      break;
    case PROP_ICONS:
      g_value_set_boxed (value, priv->icon_list);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_scale_button_finalize (GObject *object)
{
  BtkScaleButton *button = BTK_SCALE_BUTTON (object);
  BtkScaleButtonPrivate *priv = button->priv;

  if (priv->icon_list)
    {
      g_strfreev (priv->icon_list);
      priv->icon_list = NULL;
    }

  if (priv->adjustment)
    {
      g_object_unref (priv->adjustment);
      priv->adjustment = NULL;
    }

  G_OBJECT_CLASS (btk_scale_button_parent_class)->finalize (object);
}

static void
btk_scale_button_dispose (GObject *object)
{
  BtkScaleButton *button = BTK_SCALE_BUTTON (object);
  BtkScaleButtonPrivate *priv = button->priv;

  if (priv->dock)
    {
      btk_widget_destroy (priv->dock);
      priv->dock = NULL;
    }

  if (priv->click_id != 0)
    {
      g_source_remove (priv->click_id);
      priv->click_id = 0;
    }

  G_OBJECT_CLASS (btk_scale_button_parent_class)->dispose (object);
}

/**
 * btk_scale_button_new:
 * @size: (int): a stock icon size
 * @min: the minimum value of the scale (usually 0)
 * @max: the maximum value of the scale (usually 100)
 * @step: the stepping of value when a scroll-wheel event,
 *        or up/down arrow event occurs (usually 2)
 * @icons: (allow-none) (array zero-terminated=1): a %NULL-terminated
 *         array of icon names, or %NULL if you want to set the list
 *         later with btk_scale_button_set_icons()
 *
 * Creates a #BtkScaleButton, with a range between @min and @max, with
 * a stepping of @step.
 *
 * Return value: a new #BtkScaleButton
 *
 * Since: 2.12
 */
BtkWidget *
btk_scale_button_new (BtkIconSize   size,
		      gdouble       min,
		      gdouble       max,
		      gdouble       step,
		      const gchar **icons)
{
  BtkScaleButton *button;
  BtkObject *adj;

  adj = btk_adjustment_new (min, min, max, step, 10 * step, 0);

  button = g_object_new (BTK_TYPE_SCALE_BUTTON,
                         "adjustment", adj,
                         "icons", icons,
                         "size", size,
                         NULL);

  return BTK_WIDGET (button);
}

/**
 * btk_scale_button_get_value:
 * @button: a #BtkScaleButton
 *
 * Gets the current value of the scale button.
 *
 * Return value: current value of the scale button
 *
 * Since: 2.12
 */
gdouble
btk_scale_button_get_value (BtkScaleButton * button)
{
  BtkScaleButtonPrivate *priv;

  g_return_val_if_fail (BTK_IS_SCALE_BUTTON (button), 0);

  priv = button->priv;

  return btk_adjustment_get_value (priv->adjustment);
}

/**
 * btk_scale_button_set_value:
 * @button: a #BtkScaleButton
 * @value: new value of the scale button
 *
 * Sets the current value of the scale; if the value is outside
 * the minimum or maximum range values, it will be clamped to fit
 * inside them. The scale button emits the #BtkScaleButton::value-changed
 * signal if the value changes.
 *
 * Since: 2.12
 */
void
btk_scale_button_set_value (BtkScaleButton *button,
			    gdouble         value)
{
  BtkScaleButtonPrivate *priv;

  g_return_if_fail (BTK_IS_SCALE_BUTTON (button));

  priv = button->priv;

  btk_range_set_value (BTK_RANGE (priv->scale), value);
}

/**
 * btk_scale_button_set_icons:
 * @button: a #BtkScaleButton
 * @icons: (array zero-terminated=1): a %NULL-terminated array of icon names
 *
 * Sets the icons to be used by the scale button.
 * For details, see the #BtkScaleButton:icons property.
 *
 * Since: 2.12
 */
void
btk_scale_button_set_icons (BtkScaleButton  *button,
			    const gchar    **icons)
{
  BtkScaleButtonPrivate *priv;
  gchar **tmp;

  g_return_if_fail (BTK_IS_SCALE_BUTTON (button));

  priv = button->priv;

  tmp = priv->icon_list;
  priv->icon_list = g_strdupv ((gchar **) icons);
  g_strfreev (tmp);
  btk_scale_button_update_icon (button);

  g_object_notify (G_OBJECT (button), "icons");
}

/**
 * btk_scale_button_get_adjustment:
 * @button: a #BtkScaleButton
 *
 * Gets the #BtkAdjustment associated with the #BtkScaleButton's scale.
 * See btk_range_get_adjustment() for details.
 *
 * Returns: (transfer none): the adjustment associated with the scale
 *
 * Since: 2.12
 */
BtkAdjustment*
btk_scale_button_get_adjustment	(BtkScaleButton *button)
{
  g_return_val_if_fail (BTK_IS_SCALE_BUTTON (button), NULL);

  return button->priv->adjustment;
}

/**
 * btk_scale_button_set_adjustment:
 * @button: a #BtkScaleButton
 * @adjustment: a #BtkAdjustment
 *
 * Sets the #BtkAdjustment to be used as a model
 * for the #BtkScaleButton's scale.
 * See btk_range_set_adjustment() for details.
 *
 * Since: 2.12
 */
void
btk_scale_button_set_adjustment	(BtkScaleButton *button,
				 BtkAdjustment  *adjustment)
{
  g_return_if_fail (BTK_IS_SCALE_BUTTON (button));
  if (!adjustment)
    adjustment = (BtkAdjustment*) btk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
  else
    g_return_if_fail (BTK_IS_ADJUSTMENT (adjustment));

  if (button->priv->adjustment != adjustment)
    {
      if (button->priv->adjustment)
        g_object_unref (button->priv->adjustment);
      button->priv->adjustment = g_object_ref_sink (adjustment);

      if (button->priv->scale)
        btk_range_set_adjustment (BTK_RANGE (button->priv->scale), adjustment);

      g_object_notify (G_OBJECT (button), "adjustment");
    }
}

/**
 * btk_scale_button_get_orientation:
 * @button: a #BtkScaleButton
 *
 * Gets the orientation of the #BtkScaleButton's popup window.
 *
 * Returns: the #BtkScaleButton's orientation.
 *
 * Since: 2.14
 *
 * Deprecated: 2.16: Use btk_orientable_get_orientation() instead.
 **/
BtkOrientation
btk_scale_button_get_orientation (BtkScaleButton *button)
{
  g_return_val_if_fail (BTK_IS_SCALE_BUTTON (button), BTK_ORIENTATION_VERTICAL);

  return button->priv->orientation;
}

/**
 * btk_scale_button_set_orientation:
 * @button: a #BtkScaleButton
 * @orientation: the new orientation
 *
 * Sets the orientation of the #BtkScaleButton's popup window.
 *
 * Since: 2.14
 *
 * Deprecated: 2.16: Use btk_orientable_set_orientation() instead.
 **/
void
btk_scale_button_set_orientation (BtkScaleButton *button,
                                  BtkOrientation  orientation)
{
  g_return_if_fail (BTK_IS_SCALE_BUTTON (button));

  btk_scale_button_set_orientation_private (button, orientation);
}

/**
 * btk_scale_button_get_plus_button:
 * @button: a #BtkScaleButton
 *
 * Retrieves the plus button of the #BtkScaleButton.
 *
 * Returns: (transfer none): the plus button of the #BtkScaleButton
 *
 * Since: 2.14
 */
BtkWidget *
btk_scale_button_get_plus_button (BtkScaleButton *button)
{
  g_return_val_if_fail (BTK_IS_SCALE_BUTTON (button), NULL);

  return button->plus_button;
}

/**
 * btk_scale_button_get_minus_button:
 * @button: a #BtkScaleButton
 *
 * Retrieves the minus button of the #BtkScaleButton.
 *
 * Returns: (transfer none): the minus button of the #BtkScaleButton
 *
 * Since: 2.14
 */
BtkWidget *
btk_scale_button_get_minus_button (BtkScaleButton *button)
{
  g_return_val_if_fail (BTK_IS_SCALE_BUTTON (button), NULL);

  return button->minus_button;
}

/**
 * btk_scale_button_get_popup:
 * @button: a #BtkScaleButton
 *
 * Retrieves the popup of the #BtkScaleButton.
 *
 * Returns: (transfer none): the popup of the #BtkScaleButton
 *
 * Since: 2.14
 */
BtkWidget *
btk_scale_button_get_popup (BtkScaleButton *button)
{
  g_return_val_if_fail (BTK_IS_SCALE_BUTTON (button), NULL);

  return button->priv->dock;
}

static void
btk_scale_button_set_orientation_private (BtkScaleButton *button,
                                          BtkOrientation  orientation)
{
  BtkScaleButtonPrivate *priv = button->priv;

  if (orientation != priv->orientation)
    {
      priv->orientation = orientation;

      btk_orientable_set_orientation (BTK_ORIENTABLE (priv->box),
                                      orientation);
      btk_container_child_set (BTK_CONTAINER (priv->box),
                               button->plus_button,
                               "pack-type",
                               orientation == BTK_ORIENTATION_VERTICAL ?
                               BTK_PACK_START : BTK_PACK_END,
                               NULL);
      btk_container_child_set (BTK_CONTAINER (priv->box),
                               button->minus_button,
                               "pack-type",
                               orientation == BTK_ORIENTATION_VERTICAL ?
                               BTK_PACK_END : BTK_PACK_START,
                               NULL);

      btk_orientable_set_orientation (BTK_ORIENTABLE (priv->scale),
                                      orientation);

      if (orientation == BTK_ORIENTATION_VERTICAL)
        {
          btk_widget_set_size_request (BTK_WIDGET (priv->scale),
                                       -1, SCALE_SIZE);
          btk_range_set_inverted (BTK_RANGE (priv->scale), TRUE);
        }
      else
        {
          btk_widget_set_size_request (BTK_WIDGET (priv->scale),
                                       SCALE_SIZE, -1);
          btk_range_set_inverted (BTK_RANGE (priv->scale), FALSE);
        }

      /* FIXME: without this, the popup window appears as a square
       * after changing the orientation
       */
      btk_window_resize (BTK_WINDOW (priv->dock), 1, 1);

      g_object_notify (G_OBJECT (button), "orientation");
    }
}

/*
 * button callbacks.
 */

static gboolean
btk_scale_button_scroll (BtkWidget      *widget,
			 BdkEventScroll *event)
{
  BtkScaleButton *button;
  BtkScaleButtonPrivate *priv;
  BtkAdjustment *adj;
  gdouble d;

  button = BTK_SCALE_BUTTON (widget);
  priv = button->priv;
  adj = priv->adjustment;

  if (event->type != BDK_SCROLL)
    return FALSE;

  d = btk_scale_button_get_value (button);
  if (event->direction == BDK_SCROLL_UP)
    {
      d += adj->step_increment;
      if (d > adj->upper)
	d = adj->upper;
    }
  else
    {
      d -= adj->step_increment;
      if (d < adj->lower)
	d = adj->lower;
    }
  btk_scale_button_set_value (button, d);

  return TRUE;
}

static void
btk_scale_button_screen_changed (BtkWidget *widget,
				 BdkScreen *previous_screen)
{
  BtkScaleButton *button = (BtkScaleButton *) widget;
  BtkScaleButtonPrivate *priv;
  BdkScreen *screen;
  GValue value = { 0, };

  if (btk_widget_has_screen (widget) == FALSE)
    return;

  priv = button->priv;

  screen = btk_widget_get_screen (widget);
  g_value_init (&value, G_TYPE_INT);
  if (bdk_screen_get_setting (screen,
			      "btk-double-click-time",
			      &value) == FALSE)
    {
      priv->click_timeout = CLICK_TIMEOUT;
      return;
    }

  priv->click_timeout = g_value_get_int (&value);
}

static gboolean
btk_scale_popup (BtkWidget *widget,
		 BdkEvent  *event,
		 guint32    time)
{
  BtkScaleButton *button;
  BtkScaleButtonPrivate *priv;
  BtkAdjustment *adj;
  gint x, y, m, dx, dy, sx, sy, startoff;
  gdouble v;
  BdkDisplay *display;
  BdkScreen *screen;
  gboolean is_moved;

  is_moved = FALSE;
  button = BTK_SCALE_BUTTON (widget);
  priv = button->priv;
  adj = priv->adjustment;

  display = btk_widget_get_display (widget);
  screen = btk_widget_get_screen (widget);

  /* position roughly */
  btk_window_set_screen (BTK_WINDOW (priv->dock), screen);

  bdk_window_get_origin (widget->window, &x, &y);
  x += widget->allocation.x;
  y += widget->allocation.y;

  if (priv->orientation == BTK_ORIENTATION_VERTICAL)
    btk_window_move (BTK_WINDOW (priv->dock), x, y - (SCALE_SIZE / 2));
  else
    btk_window_move (BTK_WINDOW (priv->dock), x - (SCALE_SIZE / 2), y);

  btk_widget_show_all (priv->dock);

  bdk_window_get_origin (priv->dock->window, &dx, &dy);
  dx += priv->dock->allocation.x;
  dy += priv->dock->allocation.y;

  bdk_window_get_origin (priv->scale->window, &sx, &sy);
  sx += priv->scale->allocation.x;
  sy += priv->scale->allocation.y;

  priv->timeout = TRUE;

  /* position (needs widget to be shown already) */
  v = btk_scale_button_get_value (button) / (adj->upper - adj->lower);

  if (priv->orientation == BTK_ORIENTATION_VERTICAL)
    {
      startoff = sy - dy;

      x += (widget->allocation.width - priv->dock->allocation.width) / 2;
      y -= startoff;
      y -= BTK_RANGE (priv->scale)->min_slider_size / 2;
      m = priv->scale->allocation.height -
          BTK_RANGE (priv->scale)->min_slider_size;
      y -= m * (1.0 - v);
    }
  else
    {
      startoff = sx - dx;

      x -= startoff;
      y += (widget->allocation.height - priv->dock->allocation.height) / 2;
      x -= BTK_RANGE (priv->scale)->min_slider_size / 2;
      m = priv->scale->allocation.width -
          BTK_RANGE (priv->scale)->min_slider_size;
      x -= m * v;
    }

  /* Make sure the dock stays inside the monitor */
  if (event->type == BDK_BUTTON_PRESS)
    {
      int monitor;
      BdkEventButton *button_event = (BdkEventButton *) event;
      BdkRectangle rect;
      BtkWidget *d;

      d = BTK_WIDGET (priv->dock);
      monitor = bdk_screen_get_monitor_at_point (screen,
						 button_event->x_root,
						 button_event->y_root);
      bdk_screen_get_monitor_geometry (screen, monitor, &rect);

      if (priv->orientation == BTK_ORIENTATION_VERTICAL)
        y += button_event->y;
      else
        x += button_event->x;

      /* Move the dock, but set is_moved so we
       * don't forward the first click later on,
       * as it could make the scale go to the bottom */
      if (y < rect.y) {
	y = rect.y;
	is_moved = TRUE;
      } else if (y + d->allocation.height > rect.height + rect.y) {
	y = rect.y + rect.height - d->allocation.height;
	is_moved = TRUE;
      }

      if (x < rect.x) {
	x = rect.x;
	is_moved = TRUE;
      } else if (x + d->allocation.width > rect.width + rect.x) {
	x = rect.x + rect.width - d->allocation.width;
	is_moved = TRUE;
      }
    }

  btk_window_move (BTK_WINDOW (priv->dock), x, y);

  if (event->type == BDK_BUTTON_PRESS)
    BTK_WIDGET_CLASS (btk_scale_button_parent_class)->button_press_event (widget, (BdkEventButton *) event);

  /* grab focus */
  btk_grab_add (priv->dock);

  if (bdk_pointer_grab (priv->dock->window, TRUE,
			BDK_BUTTON_PRESS_MASK | BDK_BUTTON_RELEASE_MASK |
			BDK_POINTER_MOTION_MASK, NULL, NULL, time)
      != BDK_GRAB_SUCCESS)
    {
      btk_grab_remove (priv->dock);
      btk_widget_hide (priv->dock);
      return FALSE;
    }

  if (bdk_keyboard_grab (priv->dock->window, TRUE, time) != BDK_GRAB_SUCCESS)
    {
      bdk_display_pointer_ungrab (display, time);
      btk_grab_remove (priv->dock);
      btk_widget_hide (priv->dock);
      return FALSE;
    }

  btk_widget_grab_focus (priv->dock);

  if (event->type == BDK_BUTTON_PRESS && !is_moved)
    {
      BdkEventButton *e;
      BdkEventButton *button_event = (BdkEventButton *) event;

      /* forward event to the slider */
      e = (BdkEventButton *) bdk_event_copy ((BdkEvent *) event);
      e->window = priv->scale->window;

      /* position: the X position isn't relevant, halfway will work just fine.
       * The vertical position should be *exactly* in the middle of the slider
       * of the scale; if we don't do that correctly, it'll move from its current
       * position, which means a position change on-click, which is bad.
       */
      if (priv->orientation == BTK_ORIENTATION_VERTICAL)
        {
          e->x = priv->scale->allocation.width / 2;
          m = priv->scale->allocation.height -
              BTK_RANGE (priv->scale)->min_slider_size;
          e->y = ((1.0 - v) * m) + BTK_RANGE (priv->scale)->min_slider_size / 2;
        }
      else
        {
          e->y = priv->scale->allocation.height / 2;
          m = priv->scale->allocation.width -
              BTK_RANGE (priv->scale)->min_slider_size;
          e->x = (v * m) + BTK_RANGE (priv->scale)->min_slider_size / 2;
        }

      btk_widget_event (priv->scale, (BdkEvent *) e);
      e->window = button_event->window;
      bdk_event_free ((BdkEvent *) e);
    }

  btk_widget_grab_focus (priv->scale);

  priv->pop_time = time;

  return TRUE;
}

static gboolean
btk_scale_button_press (BtkWidget      *widget,
			BdkEventButton *event)
{
  return btk_scale_popup (widget, (BdkEvent *) event, event->time);
}

static void
btk_scale_button_popup (BtkWidget *widget)
{
  BdkEvent *ev;

  ev = bdk_event_new (BDK_KEY_RELEASE);
  btk_scale_popup (widget, ev, BDK_CURRENT_TIME);
  bdk_event_free (ev);
}

static gboolean
btk_scale_button_key_release (BtkWidget   *widget,
			      BdkEventKey *event)
{
  return btk_bindings_activate_event (BTK_OBJECT (widget), event);
}

/* This is called when the grab is broken for
 * either the dock, or the scale itself */
static void
btk_scale_button_grab_notify (BtkScaleButton *button,
			      gboolean        was_grabbed)
{
  BdkDisplay *display;
  BtkScaleButtonPrivate *priv;

  if (was_grabbed != FALSE)
    return;

  priv = button->priv;

  if (!btk_widget_has_grab (priv->dock))
    return;

  if (btk_widget_is_ancestor (btk_grab_get_current (), priv->dock))
    return;

  display = btk_widget_get_display (priv->dock);
  bdk_display_keyboard_ungrab (display, BDK_CURRENT_TIME);
  bdk_display_pointer_ungrab (display, BDK_CURRENT_TIME);
  btk_grab_remove (priv->dock);

  /* hide again */
  btk_widget_hide (priv->dock);
  priv->timeout = FALSE;
}

/*
 * +/- button callbacks.
 */

static gboolean
cb_button_timeout (gpointer user_data)
{
  BtkScaleButton *button;
  BtkScaleButtonPrivate *priv;
  BtkAdjustment *adj;
  gdouble val;
  gboolean res = TRUE;

  button = BTK_SCALE_BUTTON (user_data);
  priv = button->priv;

  if (priv->click_id == 0)
    return FALSE;

  adj = priv->adjustment;

  val = btk_scale_button_get_value (button);
  val += priv->direction;
  if (val <= adj->lower)
    {
      res = FALSE;
      val = adj->lower;
    }
  else if (val > adj->upper)
    {
      res = FALSE;
      val = adj->upper;
    }
  btk_scale_button_set_value (button, val);

  if (!res)
    {
      g_source_remove (priv->click_id);
      priv->click_id = 0;
    }

  return res;
}

static gboolean
cb_button_press (BtkWidget      *widget,
		 BdkEventButton *event,
		 gpointer        user_data)
{
  BtkScaleButton *button;
  BtkScaleButtonPrivate *priv;
  BtkAdjustment *adj;

  button = BTK_SCALE_BUTTON (user_data);
  priv = button->priv;
  adj = priv->adjustment;

  if (priv->click_id != 0)
    g_source_remove (priv->click_id);

  if (widget == button->plus_button)
    priv->direction = fabs (adj->page_increment);
  else
    priv->direction = - fabs (adj->page_increment);

  priv->click_id = bdk_threads_add_timeout (priv->click_timeout,
                                            cb_button_timeout,
                                            button);
  cb_button_timeout (button);

  return TRUE;
}

static gboolean
cb_button_release (BtkWidget      *widget,
		   BdkEventButton *event,
		   gpointer        user_data)
{
  BtkScaleButton *button;
  BtkScaleButtonPrivate *priv;

  button = BTK_SCALE_BUTTON (user_data);
  priv = button->priv;

  if (priv->click_id != 0)
    {
      g_source_remove (priv->click_id);
      priv->click_id = 0;
    }

  return TRUE;
}

static void
cb_dock_grab_notify (BtkWidget *widget,
		     gboolean   was_grabbed,
		     gpointer   user_data)
{
  BtkScaleButton *button = (BtkScaleButton *) user_data;

  btk_scale_button_grab_notify (button, was_grabbed);
}

static gboolean
cb_dock_grab_broken_event (BtkWidget *widget,
			   gboolean   was_grabbed,
			   gpointer   user_data)
{
  BtkScaleButton *button = (BtkScaleButton *) user_data;

  btk_scale_button_grab_notify (button, FALSE);

  return FALSE;
}

/*
 * Scale callbacks.
 */

static void
btk_scale_button_release_grab (BtkScaleButton *button,
			       BdkEventButton *event)
{
  BdkEventButton *e;
  BdkDisplay *display;
  BtkScaleButtonPrivate *priv;

  priv = button->priv;

  /* ungrab focus */
  display = btk_widget_get_display (BTK_WIDGET (button));
  bdk_display_keyboard_ungrab (display, event->time);
  bdk_display_pointer_ungrab (display, event->time);
  btk_grab_remove (priv->dock);

  /* hide again */
  btk_widget_hide (priv->dock);
  priv->timeout = FALSE;

  e = (BdkEventButton *) bdk_event_copy ((BdkEvent *) event);
  e->window = BTK_WIDGET (button)->window;
  e->type = BDK_BUTTON_RELEASE;
  btk_widget_event (BTK_WIDGET (button), (BdkEvent *) e);
  e->window = event->window;
  bdk_event_free ((BdkEvent *) e);
}

static gboolean
cb_dock_button_press (BtkWidget      *widget,
		      BdkEventButton *event,
		      gpointer        user_data)
{
  BtkScaleButton *button = BTK_SCALE_BUTTON (user_data);

  if (event->type == BDK_BUTTON_PRESS)
    {
      btk_scale_button_release_grab (button, event);
      return TRUE;
    }

  return FALSE;
}

static void
btk_scale_button_popdown (BtkWidget *widget)
{
  BtkScaleButton *button;
  BtkScaleButtonPrivate *priv;
  BdkDisplay *display;

  button = BTK_SCALE_BUTTON (widget);
  priv = button->priv;

  /* ungrab focus */
  display = btk_widget_get_display (widget);
  bdk_display_keyboard_ungrab (display, BDK_CURRENT_TIME);
  bdk_display_pointer_ungrab (display, BDK_CURRENT_TIME);
  btk_grab_remove (priv->dock);

  /* hide again */
  btk_widget_hide (priv->dock);
  priv->timeout = FALSE;
}

static gboolean
cb_dock_key_release (BtkWidget   *widget,
		     BdkEventKey *event,
		     gpointer     user_data)
{
  if (event->keyval == BDK_Escape)
    {
      btk_scale_button_popdown (BTK_WIDGET (user_data));
      return TRUE;
    }

  if (!btk_bindings_activate_event (BTK_OBJECT (widget), event))
    {
      /* The popup hasn't managed the event, pass onto the button */
      btk_bindings_activate_event (BTK_OBJECT (user_data), event);
    }

  return TRUE;
}

static void
cb_scale_grab_notify (BtkWidget *widget,
		      gboolean   was_grabbed,
		      gpointer   user_data)
{
  BtkScaleButton *button = (BtkScaleButton *) user_data;

  btk_scale_button_grab_notify (button, was_grabbed);
}

/*
 * Scale stuff.
 */

#define BTK_TYPE_SCALE_BUTTON_SCALE    (_btk_scale_button_scale_get_type ())
#define BTK_SCALE_BUTTON_SCALE(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_SCALE_BUTTON_SCALE, BtkScaleButtonScale))
#define BTK_IS_SCALE_BUTTON_SCALE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_SCALE_BUTTON_SCALE))

typedef struct _BtkScaleButtonScale
{
  BtkScale parent_instance;
  BtkScaleButton *button;
} BtkScaleButtonScale;

typedef struct _BtkScaleButtonScaleClass
{
  BtkScaleClass parent_class;
} BtkScaleButtonScaleClass;

static gboolean	btk_scale_button_scale_press   (BtkWidget      *widget,
                                                BdkEventButton *event);
static gboolean btk_scale_button_scale_release (BtkWidget      *widget,
                                                BdkEventButton *event);

G_DEFINE_TYPE (BtkScaleButtonScale, _btk_scale_button_scale, BTK_TYPE_SCALE)

static void
_btk_scale_button_scale_class_init (BtkScaleButtonScaleClass *klass)
{
  BtkWidgetClass *btkwidget_class = BTK_WIDGET_CLASS (klass);
  BtkRangeClass *btkrange_class = BTK_RANGE_CLASS (klass);

  btkwidget_class->button_press_event = btk_scale_button_scale_press;
  btkwidget_class->button_release_event = btk_scale_button_scale_release;
  btkrange_class->value_changed = btk_scale_button_scale_value_changed;
}

static void
_btk_scale_button_scale_init (BtkScaleButtonScale *scale)
{
}

static BtkWidget *
btk_scale_button_scale_new (BtkScaleButton *button)
{
  BtkScaleButtonPrivate *priv = button->priv;
  BtkScaleButtonScale *scale;

  scale = g_object_new (BTK_TYPE_SCALE_BUTTON_SCALE,
                        "orientation", priv->orientation,
                        "adjustment",  priv->adjustment,
                        "draw-value",  FALSE,
                        NULL);

  scale->button = button;

  g_signal_connect (scale, "grab-notify",
                    G_CALLBACK (cb_scale_grab_notify), button);

  if (priv->orientation == BTK_ORIENTATION_VERTICAL)
    {
      btk_widget_set_size_request (BTK_WIDGET (scale), -1, SCALE_SIZE);
      btk_range_set_inverted (BTK_RANGE (scale), TRUE);
    }
  else
    {
      btk_widget_set_size_request (BTK_WIDGET (scale), SCALE_SIZE, -1);
      btk_range_set_inverted (BTK_RANGE (scale), FALSE);
    }

  return BTK_WIDGET (scale);
}

static gboolean
btk_scale_button_scale_press (BtkWidget      *widget,
			      BdkEventButton *event)
{
  BtkScaleButtonPrivate *priv = BTK_SCALE_BUTTON_SCALE (widget)->button->priv;

  /* the scale will grab input; if we have input grabbed, all goes
   * horribly wrong, so let's not do that.
   */
  btk_grab_remove (priv->dock);

  return BTK_WIDGET_CLASS (_btk_scale_button_scale_parent_class)->button_press_event (widget, event);
}

static gboolean
btk_scale_button_scale_release (BtkWidget      *widget,
				BdkEventButton *event)
{
  BtkScaleButton *button = BTK_SCALE_BUTTON_SCALE (widget)->button;
  gboolean res;

  if (button->priv->timeout)
    {
      /* if we did a quick click, leave the window open; else, hide it */
      if (event->time > button->priv->pop_time + button->priv->click_timeout)
        {

	  btk_scale_button_release_grab (button, event);
	  BTK_WIDGET_CLASS (_btk_scale_button_scale_parent_class)->button_release_event (widget, event);

	  return TRUE;
	}

      button->priv->timeout = FALSE;
    }

  res = BTK_WIDGET_CLASS (_btk_scale_button_scale_parent_class)->button_release_event (widget, event);

  /* the scale will release input; right after that, we *have to* grab
   * it back so we can catch out-of-scale clicks and hide the popup,
   * so I basically want a g_signal_connect_after_always(), but I can't
   * find that, so we do this complex 'first-call-parent-then-do-actual-
   * action' thingy...
   */
  btk_grab_add (button->priv->dock);

  return res;
}

static void
btk_scale_button_update_icon (BtkScaleButton *button)
{
  BtkScaleButtonPrivate *priv;
  BtkRange *range;
  BtkAdjustment *adj;
  gdouble value;
  const gchar *name;
  guint num_icons;

  priv = button->priv;

  if (!priv->icon_list || priv->icon_list[0] == '\0')
    {
      btk_image_set_from_stock (BTK_IMAGE (priv->image),
				BTK_STOCK_MISSING_IMAGE,
				priv->size);
      return;
    }

  num_icons = g_strv_length (priv->icon_list);

  /* The 1-icon special case */
  if (num_icons == 1)
    {
      btk_image_set_from_icon_name (BTK_IMAGE (priv->image),
				    priv->icon_list[0],
				    priv->size);
      return;
    }

  range = BTK_RANGE (priv->scale);
  adj = priv->adjustment;
  value = btk_scale_button_get_value (button);

  /* The 2-icons special case */
  if (num_icons == 2)
    {
      gdouble limit;
      limit = (adj->upper - adj->lower) / 2 + adj->lower;
      if (value < limit)
	name = priv->icon_list[0];
      else
	name = priv->icon_list[1];

      btk_image_set_from_icon_name (BTK_IMAGE (priv->image),
				    name,
				    priv->size);
      return;
    }

  /* With 3 or more icons */
  if (value == adj->lower)
    {
      name = priv->icon_list[0];
    }
  else if (value == adj->upper)
    {
      name = priv->icon_list[1];
    }
  else
    {
      gdouble step;
      guint i;

      step = (adj->upper - adj->lower) / (num_icons - 2);
      i = (guint) ((value - adj->lower) / step) + 2;
      g_assert (i < num_icons);
      name = priv->icon_list[i];
    }

  btk_image_set_from_icon_name (BTK_IMAGE (priv->image),
				name,
				priv->size);
}

static void
btk_scale_button_scale_value_changed (BtkRange *range)
{
  BtkScaleButton *button = BTK_SCALE_BUTTON_SCALE (range)->button;
  gdouble value;

  value = btk_range_get_value (range);

  btk_scale_button_update_icon (button);

  g_signal_emit (button, signals[VALUE_CHANGED], 0, value);
  g_object_notify (G_OBJECT (button), "value");
}

#define __BTK_SCALE_BUTTON_C__
#include "btkaliasdef.c"
