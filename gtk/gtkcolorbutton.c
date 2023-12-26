/*
 * BTK - The GIMP Toolkit
 * Copyright (C) 1998, 1999 Red Hat, Inc.
 * All rights reserved.
 *
 * This Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/* Color picker button for GNOME
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 *
 * Modified by the BTK+ Team and others 2003.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#include "config.h"

#include "btkcolorbutton.h"
#include "bdk/bdkkeysyms.h"
#include "bdk-pixbuf/bdk-pixbuf.h"
#include "btkbutton.h"
#include "btkmain.h"
#include "btkalignment.h"
#include "btkcolorsel.h"
#include "btkcolorseldialog.h"
#include "btkdnd.h"
#include "btkdrawingarea.h"
#include "btkframe.h"
#include "btkmarshalers.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkalias.h"

/* Size of checks and gray levels for alpha compositing checkerboard */
#define CHECK_SIZE  4
#define CHECK_DARK  (1.0 / 3.0)
#define CHECK_LIGHT (2.0 / 3.0)

#define BTK_COLOR_BUTTON_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), BTK_TYPE_COLOR_BUTTON, BtkColorButtonPrivate))

struct _BtkColorButtonPrivate 
{
  BtkWidget *draw_area; /* Widget where we draw the color sample */
  BtkWidget *cs_dialog; /* Color selection dialog */
  
  gchar *title;         /* Title for the color selection window */
  
  BdkColor color;
  guint16 alpha;
  
  guint use_alpha : 1;  /* Use alpha or not */
};

/* Properties */
enum 
{
  PROP_0,
  PROP_USE_ALPHA,
  PROP_TITLE,
  PROP_COLOR,
  PROP_ALPHA
};

/* Signals */
enum 
{
  COLOR_SET,
  LAST_SIGNAL
};

/* bobject signals */
static void btk_color_button_finalize      (GObject             *object);
static void btk_color_button_set_property  (GObject        *object,
					    guint           param_id,
					    const GValue   *value,
					    GParamSpec     *pspec);
static void btk_color_button_get_property  (GObject        *object,
					    guint           param_id,
					    GValue         *value,
					    GParamSpec     *pspec);

/* btkwidget signals */
static void btk_color_button_state_changed (BtkWidget           *widget, 
					    BtkStateType         previous_state);

/* btkbutton signals */
static void btk_color_button_clicked       (BtkButton           *button);

/* source side drag signals */
static void btk_color_button_drag_begin (BtkWidget        *widget,
					 BdkDragContext   *context,
					 gpointer          data);
static void btk_color_button_drag_data_get (BtkWidget        *widget,
                                            BdkDragContext   *context,
                                            BtkSelectionData *selection_data,
                                            guint             info,
                                            guint             time,
                                            BtkColorButton   *color_button);

/* target side drag signals */
static void btk_color_button_drag_data_received (BtkWidget        *widget,
						 BdkDragContext   *context,
						 gint              x,
						 gint              y,
						 BtkSelectionData *selection_data,
						 guint             info,
						 guint32           time,
						 BtkColorButton   *color_button);


static guint color_button_signals[LAST_SIGNAL] = { 0 };

static const BtkTargetEntry drop_types[] = { { "application/x-color", 0, 0 } };

G_DEFINE_TYPE (BtkColorButton, btk_color_button, BTK_TYPE_BUTTON)

static void
btk_color_button_class_init (BtkColorButtonClass *klass)
{
  GObjectClass *bobject_class;
  BtkWidgetClass *widget_class;
  BtkButtonClass *button_class;

  bobject_class = G_OBJECT_CLASS (klass);
  widget_class = BTK_WIDGET_CLASS (klass);
  button_class = BTK_BUTTON_CLASS (klass);

  bobject_class->get_property = btk_color_button_get_property;
  bobject_class->set_property = btk_color_button_set_property;
  bobject_class->finalize = btk_color_button_finalize;
  widget_class->state_changed = btk_color_button_state_changed;
  button_class->clicked = btk_color_button_clicked;
  klass->color_set = NULL;

  /**
   * BtkColorButton:use-alpha:
   *
   * If this property is set to %TRUE, the color swatch on the button is rendered against a 
   * checkerboard background to show its opacity and the opacity slider is displayed in the 
   * color selection dialog. 
   *
   * Since: 2.4
   */
  g_object_class_install_property (bobject_class,
                                   PROP_USE_ALPHA,
                                   g_param_spec_boolean ("use-alpha", P_("Use alpha"), 
                                                         P_("Whether or not to give the color an alpha value"),
                                                         FALSE,
                                                         BTK_PARAM_READWRITE));

  /**
   * BtkColorButton:title:
   *
   * The title of the color selection dialog
   *
   * Since: 2.4
   */
  g_object_class_install_property (bobject_class,
                                   PROP_TITLE,
                                   g_param_spec_string ("title", 
							P_("Title"), 
                                                        P_("The title of the color selection dialog"),
                                                        _("Pick a Color"),
                                                        BTK_PARAM_READWRITE));

  /**
   * BtkColorButton:color:
   *
   * The selected color.
   *
   * Since: 2.4
   */
  g_object_class_install_property (bobject_class,
                                   PROP_COLOR,
                                   g_param_spec_boxed ("color",
                                                       P_("Current Color"),
                                                       P_("The selected color"),
                                                       BDK_TYPE_COLOR,
                                                       BTK_PARAM_READWRITE));

  /**
   * BtkColorButton:alpha:
   *
   * The selected opacity value (0 fully transparent, 65535 fully opaque). 
   *
   * Since: 2.4
   */
  g_object_class_install_property (bobject_class,
                                   PROP_ALPHA,
                                   g_param_spec_uint ("alpha",
                                                      P_("Current Alpha"),
                                                      P_("The selected opacity value (0 fully transparent, 65535 fully opaque)"),
                                                      0, 65535, 65535,
                                                      BTK_PARAM_READWRITE));
        
  /**
   * BtkColorButton::color-set:
   * @widget: the object which received the signal.
   * 
   * The ::color-set signal is emitted when the user selects a color. 
   * When handling this signal, use btk_color_button_get_color() and 
   * btk_color_button_get_alpha() to find out which color was just selected.
   *
   * Note that this signal is only emitted when the <emphasis>user</emphasis>
   * changes the color. If you need to react to programmatic color changes
   * as well, use the notify::color signal.
   *
   * Since: 2.4
   */
  color_button_signals[COLOR_SET] = g_signal_new (I_("color-set"),
						  G_TYPE_FROM_CLASS (bobject_class),
						  G_SIGNAL_RUN_FIRST,
						  G_STRUCT_OFFSET (BtkColorButtonClass, color_set),
						  NULL, NULL,
						  _btk_marshal_VOID__VOID,
						  G_TYPE_NONE, 0);

  g_type_class_add_private (bobject_class, sizeof (BtkColorButtonPrivate));
}

static gboolean
btk_color_button_has_alpha (BtkColorButton *color_button)
{
  return color_button->priv->use_alpha &&
      color_button->priv->alpha < 65535;
}

static bairo_pattern_t *
btk_color_button_get_checkered (void)
{
  /* need to respect pixman's stride being a multiple of 4 */
  static unsigned char data[8] = { 0xFF, 0x00, 0x00, 0x00,
                                   0x00, 0xFF, 0x00, 0x00 };
  static bairo_surface_t *checkered = NULL;
  bairo_pattern_t *pattern;

  if (checkered == NULL)
    {
      checkered = bairo_image_surface_create_for_data (data,
                                                       BAIRO_FORMAT_A8,
                                                       2, 2, 4);
    }

  pattern = bairo_pattern_create_for_surface (checkered);
  bairo_pattern_set_extend (pattern, BAIRO_EXTEND_REPEAT);
  bairo_pattern_set_filter (pattern, BAIRO_FILTER_NEAREST);

  return pattern;
}

/* Handle exposure events for the color picker's drawing area */
static gint
expose_event (BtkWidget      *widget, 
              BdkEventExpose *event, 
              gpointer        data)
{
  BtkColorButton *color_button = BTK_COLOR_BUTTON (data);
  BtkAllocation allocation;
  bairo_pattern_t *checkered;
  bairo_t *cr;

  cr = bdk_bairo_create (event->window);

  btk_widget_get_allocation (widget, &allocation);
  bdk_bairo_rectangle (cr, &allocation);
  bairo_clip (cr);

  if (btk_color_button_has_alpha (color_button))
    {
      bairo_save (cr);

      bairo_set_source_rgb (cr, CHECK_DARK, CHECK_DARK, CHECK_DARK);
      bairo_paint (cr);

      bairo_set_source_rgb (cr, CHECK_LIGHT, CHECK_LIGHT, CHECK_LIGHT);
      bairo_scale (cr, CHECK_SIZE, CHECK_SIZE);

      checkered = btk_color_button_get_checkered ();
      bairo_mask (cr, checkered);
      bairo_pattern_destroy (checkered);

      bairo_restore (cr);

      bairo_set_source_rgba (cr,
                             color_button->priv->color.red / 65535.,
                             color_button->priv->color.green / 65535.,
                             color_button->priv->color.blue / 65535.,
                             color_button->priv->alpha / 65535.);
    }
  else
    {
      bdk_bairo_set_source_color (cr, &color_button->priv->color);
    }

  bairo_paint (cr);

  if (!btk_widget_is_sensitive (BTK_WIDGET (color_button)))
    {
      bdk_bairo_set_source_color (cr, &BTK_WIDGET(color_button)->style->bg[BTK_STATE_INSENSITIVE]);
      checkered = btk_color_button_get_checkered ();
      bairo_mask (cr, checkered);
      bairo_pattern_destroy (checkered);
    }

  bairo_destroy (cr);

  return FALSE;
}

static void
btk_color_button_state_changed (BtkWidget   *widget, 
                                BtkStateType previous_state)
{
  btk_widget_queue_draw (widget);
}

static void
btk_color_button_drag_data_received (BtkWidget        *widget,
				     BdkDragContext   *context,
				     gint              x,
				     gint              y,
				     BtkSelectionData *selection_data,
				     guint             info,
				     guint32           time,
				     BtkColorButton   *color_button)
{
  guint16 *dropped;

  if (selection_data->length < 0)
    return;

  /* We accept drops with the wrong format, since the KDE color
   * chooser incorrectly drops application/x-color with format 8.
   */
  if (selection_data->length != 8) 
    {
      g_warning (_("Received invalid color data\n"));
      return;
    }


  dropped = (guint16 *)selection_data->data;

  color_button->priv->color.red = dropped[0];
  color_button->priv->color.green = dropped[1];
  color_button->priv->color.blue = dropped[2];
  color_button->priv->alpha = dropped[3];

  btk_widget_queue_draw (color_button->priv->draw_area);

  g_signal_emit (color_button, color_button_signals[COLOR_SET], 0);

  g_object_freeze_notify (G_OBJECT (color_button));
  g_object_notify (G_OBJECT (color_button), "color");
  g_object_notify (G_OBJECT (color_button), "alpha");
  g_object_thaw_notify (G_OBJECT (color_button));
}

static void
set_color_icon (BdkDragContext *context,
		BdkColor       *color)
{
  BdkPixbuf *pixbuf;
  guint32 pixel;

  pixbuf = bdk_pixbuf_new (BDK_COLORSPACE_RGB, FALSE,
			   8, 48, 32);

  pixel = ((color->red & 0xff00) << 16) | 
          ((color->green & 0xff00) << 8) | 
           (color->blue & 0xff00);

  bdk_pixbuf_fill (pixbuf, pixel);
  
  btk_drag_set_icon_pixbuf (context, pixbuf, -2, -2);
  g_object_unref (pixbuf);
}

static void
btk_color_button_drag_begin (BtkWidget      *widget,
			     BdkDragContext *context,
			     gpointer        data)
{
  BtkColorButton *color_button = data;
  
  set_color_icon (context, &color_button->priv->color);
}

static void
btk_color_button_drag_data_get (BtkWidget        *widget,
				BdkDragContext   *context,
				BtkSelectionData *selection_data,
				guint             info,
				guint             time,
				BtkColorButton   *color_button)
{
  guint16 dropped[4];

  dropped[0] = color_button->priv->color.red;
  dropped[1] = color_button->priv->color.green;
  dropped[2] = color_button->priv->color.blue;
  dropped[3] = color_button->priv->alpha;

  btk_selection_data_set (selection_data, selection_data->target,
			  16, (guchar *)dropped, 8);
}

static void
btk_color_button_init (BtkColorButton *color_button)
{
  BtkWidget *alignment;
  BtkWidget *frame;
  BangoLayout *layout;
  BangoRectangle rect;

  /* Create the widgets */
  color_button->priv = BTK_COLOR_BUTTON_GET_PRIVATE (color_button);

  btk_widget_push_composite_child ();

  alignment = btk_alignment_new (0.5, 0.5, 0.5, 1.0);
  btk_container_set_border_width (BTK_CONTAINER (alignment), 1);
  btk_container_add (BTK_CONTAINER (color_button), alignment);
  btk_widget_show (alignment);

  frame = btk_frame_new (NULL);
  btk_frame_set_shadow_type (BTK_FRAME (frame), BTK_SHADOW_ETCHED_OUT);
  btk_container_add (BTK_CONTAINER (alignment), frame);
  btk_widget_show (frame);

  /* Just some widget we can hook to expose-event on */
  color_button->priv->draw_area = btk_alignment_new (0.5, 0.5, 0.0, 0.0);

  layout = btk_widget_create_bango_layout (BTK_WIDGET (color_button), "Black");
  bango_layout_get_pixel_extents (layout, NULL, &rect);
  g_object_unref (layout);

  btk_widget_set_size_request (color_button->priv->draw_area, rect.width - 2, rect.height - 2);
  g_signal_connect (color_button->priv->draw_area, "expose-event",
                    G_CALLBACK (expose_event), color_button);
  btk_container_add (BTK_CONTAINER (frame), color_button->priv->draw_area);
  btk_widget_show (color_button->priv->draw_area);

  color_button->priv->title = g_strdup (_("Pick a Color")); /* default title */

  /* Start with opaque black, alpha disabled */

  color_button->priv->color.red = 0;
  color_button->priv->color.green = 0;
  color_button->priv->color.blue = 0;
  color_button->priv->alpha = 65535;
  color_button->priv->use_alpha = FALSE;

  btk_drag_dest_set (BTK_WIDGET (color_button),
                     BTK_DEST_DEFAULT_MOTION |
                     BTK_DEST_DEFAULT_HIGHLIGHT |
                     BTK_DEST_DEFAULT_DROP,
                     drop_types, 1, BDK_ACTION_COPY);
  btk_drag_source_set (BTK_WIDGET(color_button),
                       BDK_BUTTON1_MASK|BDK_BUTTON3_MASK,
                       drop_types, 1,
                       BDK_ACTION_COPY);
  g_signal_connect (color_button, "drag-begin",
		    G_CALLBACK (btk_color_button_drag_begin), color_button);
  g_signal_connect (color_button, "drag-data-received",
                    G_CALLBACK (btk_color_button_drag_data_received), color_button);
  g_signal_connect (color_button, "drag-data-get",
                    G_CALLBACK (btk_color_button_drag_data_get), color_button);

  btk_widget_pop_composite_child ();
}

static void
btk_color_button_finalize (GObject *object)
{
  BtkColorButton *color_button = BTK_COLOR_BUTTON (object);

  if (color_button->priv->cs_dialog != NULL)
    btk_widget_destroy (color_button->priv->cs_dialog);
  color_button->priv->cs_dialog = NULL;

  g_free (color_button->priv->title);
  color_button->priv->title = NULL;

  G_OBJECT_CLASS (btk_color_button_parent_class)->finalize (object);
}


/**
 * btk_color_button_new:
 *
 * Creates a new color button. This returns a widget in the form of
 * a small button containing a swatch representing the current selected 
 * color. When the button is clicked, a color-selection dialog will open, 
 * allowing the user to select a color. The swatch will be updated to reflect 
 * the new color when the user finishes.
 *
 * Returns: a new color button.
 *
 * Since: 2.4
 */
BtkWidget *
btk_color_button_new (void)
{
  return g_object_new (BTK_TYPE_COLOR_BUTTON, NULL);
}

/**
 * btk_color_button_new_with_color:
 * @color: A #BdkColor to set the current color with.
 *
 * Creates a new color button. 
 *
 * Returns: a new color button.
 *
 * Since: 2.4
 */
BtkWidget *
btk_color_button_new_with_color (const BdkColor *color)
{
  return g_object_new (BTK_TYPE_COLOR_BUTTON, "color", color, NULL);
}

static void
dialog_ok_clicked (BtkWidget *widget, 
		   gpointer   data)
{
  BtkColorButton *color_button = BTK_COLOR_BUTTON (data);
  BtkColorSelection *color_selection;

  color_selection = BTK_COLOR_SELECTION (BTK_COLOR_SELECTION_DIALOG (color_button->priv->cs_dialog)->colorsel);

  btk_color_selection_get_current_color (color_selection, &color_button->priv->color);
  color_button->priv->alpha = btk_color_selection_get_current_alpha (color_selection);

  btk_widget_hide (color_button->priv->cs_dialog);

  btk_widget_queue_draw (color_button->priv->draw_area);

  g_signal_emit (color_button, color_button_signals[COLOR_SET], 0);

  g_object_freeze_notify (G_OBJECT (color_button));
  g_object_notify (G_OBJECT (color_button), "color");
  g_object_notify (G_OBJECT (color_button), "alpha");
  g_object_thaw_notify (G_OBJECT (color_button));
}

static gboolean
dialog_destroy (BtkWidget *widget, 
		gpointer   data)
{
  BtkColorButton *color_button = BTK_COLOR_BUTTON (data);
  
  color_button->priv->cs_dialog = NULL;

  return FALSE;
}

static void
dialog_cancel_clicked (BtkWidget *widget,
		       gpointer   data)
{
  BtkColorButton *color_button = BTK_COLOR_BUTTON (data);
  
  btk_widget_hide (color_button->priv->cs_dialog);  
}

static void
btk_color_button_clicked (BtkButton *button)
{
  BtkColorButton *color_button = BTK_COLOR_BUTTON (button);
  BtkColorSelectionDialog *color_dialog;

  /* if dialog already exists, make sure it's shown and raised */
  if (!color_button->priv->cs_dialog) 
    {
      /* Create the dialog and connects its buttons */
      BtkWidget *parent;
      
      parent = btk_widget_get_toplevel (BTK_WIDGET (color_button));
      
      color_button->priv->cs_dialog = btk_color_selection_dialog_new (color_button->priv->title);
      
      color_dialog = BTK_COLOR_SELECTION_DIALOG (color_button->priv->cs_dialog);

      if (btk_widget_is_toplevel (parent) && BTK_IS_WINDOW (parent))
        {
          if (BTK_WINDOW (parent) != btk_window_get_transient_for (BTK_WINDOW (color_dialog)))
 	    btk_window_set_transient_for (BTK_WINDOW (color_dialog), BTK_WINDOW (parent));
	       
	  btk_window_set_modal (BTK_WINDOW (color_dialog),
				btk_window_get_modal (BTK_WINDOW (parent)));
	}
      
      g_signal_connect (color_dialog->ok_button, "clicked",
                        G_CALLBACK (dialog_ok_clicked), color_button);
      g_signal_connect (color_dialog->cancel_button, "clicked",
			G_CALLBACK (dialog_cancel_clicked), color_button);
      g_signal_connect (color_dialog, "destroy",
                        G_CALLBACK (dialog_destroy), color_button);
    }

  color_dialog = BTK_COLOR_SELECTION_DIALOG (color_button->priv->cs_dialog);

  btk_color_selection_set_has_opacity_control (BTK_COLOR_SELECTION (color_dialog->colorsel),
                                               color_button->priv->use_alpha);
  
  btk_color_selection_set_previous_color (BTK_COLOR_SELECTION (color_dialog->colorsel), 
					  &color_button->priv->color);
  btk_color_selection_set_previous_alpha (BTK_COLOR_SELECTION (color_dialog->colorsel), 
					  color_button->priv->alpha);

  btk_color_selection_set_current_color (BTK_COLOR_SELECTION (color_dialog->colorsel), 
					 &color_button->priv->color);
  btk_color_selection_set_current_alpha (BTK_COLOR_SELECTION (color_dialog->colorsel), 
					 color_button->priv->alpha);

  btk_window_present (BTK_WINDOW (color_button->priv->cs_dialog));
}

/**
 * btk_color_button_set_color:
 * @color_button: a #BtkColorButton.
 * @color: A #BdkColor to set the current color with.
 *
 * Sets the current color to be @color.
 *
 * Since: 2.4
 **/
void
btk_color_button_set_color (BtkColorButton *color_button,
			    const BdkColor *color)
{
  g_return_if_fail (BTK_IS_COLOR_BUTTON (color_button));
  g_return_if_fail (color != NULL);

  color_button->priv->color.red = color->red;
  color_button->priv->color.green = color->green;
  color_button->priv->color.blue = color->blue;

  btk_widget_queue_draw (color_button->priv->draw_area);
  
  g_object_notify (G_OBJECT (color_button), "color");
}


/**
 * btk_color_button_set_alpha:
 * @color_button: a #BtkColorButton.
 * @alpha: an integer between 0 and 65535.
 *
 * Sets the current opacity to be @alpha. 
 *
 * Since: 2.4
 **/
void
btk_color_button_set_alpha (BtkColorButton *color_button,
			    guint16         alpha)
{
  g_return_if_fail (BTK_IS_COLOR_BUTTON (color_button));

  color_button->priv->alpha = alpha;

  btk_widget_queue_draw (color_button->priv->draw_area);

  g_object_notify (G_OBJECT (color_button), "alpha");
}

/**
 * btk_color_button_get_color:
 * @color_button: a #BtkColorButton.
 * @color: (out): a #BdkColor to fill in with the current color.
 *
 * Sets @color to be the current color in the #BtkColorButton widget.
 *
 * Since: 2.4
 **/
void
btk_color_button_get_color (BtkColorButton *color_button,
			    BdkColor       *color)
{
  g_return_if_fail (BTK_IS_COLOR_BUTTON (color_button));
  
  color->red = color_button->priv->color.red;
  color->green = color_button->priv->color.green;
  color->blue = color_button->priv->color.blue;
}

/**
 * btk_color_button_get_alpha:
 * @color_button: a #BtkColorButton.
 *
 * Returns the current alpha value. 
 *
 * Return value: an integer between 0 and 65535.
 *
 * Since: 2.4
 **/
guint16
btk_color_button_get_alpha (BtkColorButton *color_button)
{
  g_return_val_if_fail (BTK_IS_COLOR_BUTTON (color_button), 0);
  
  return color_button->priv->alpha;
}

/**
 * btk_color_button_set_use_alpha:
 * @color_button: a #BtkColorButton.
 * @use_alpha: %TRUE if color button should use alpha channel, %FALSE if not.
 *
 * Sets whether or not the color button should use the alpha channel.
 *
 * Since: 2.4
 */
void
btk_color_button_set_use_alpha (BtkColorButton *color_button, 
				gboolean        use_alpha)
{
  g_return_if_fail (BTK_IS_COLOR_BUTTON (color_button));

  use_alpha = (use_alpha != FALSE);

  if (color_button->priv->use_alpha != use_alpha) 
    {
      color_button->priv->use_alpha = use_alpha;

      btk_widget_queue_draw (color_button->priv->draw_area);

      g_object_notify (G_OBJECT (color_button), "use-alpha");
    }
}

/**
 * btk_color_button_get_use_alpha:
 * @color_button: a #BtkColorButton.
 *
 * Does the color selection dialog use the alpha channel?
 *
 * Returns: %TRUE if the color sample uses alpha channel, %FALSE if not.
 *
 * Since: 2.4
 */
gboolean
btk_color_button_get_use_alpha (BtkColorButton *color_button)
{
  g_return_val_if_fail (BTK_IS_COLOR_BUTTON (color_button), FALSE);

  return color_button->priv->use_alpha;
}


/**
 * btk_color_button_set_title:
 * @color_button: a #BtkColorButton
 * @title: String containing new window title.
 *
 * Sets the title for the color selection dialog.
 *
 * Since: 2.4
 */
void
btk_color_button_set_title (BtkColorButton *color_button, 
			    const gchar    *title)
{
  gchar *old_title;

  g_return_if_fail (BTK_IS_COLOR_BUTTON (color_button));

  old_title = color_button->priv->title;
  color_button->priv->title = g_strdup (title);
  g_free (old_title);

  if (color_button->priv->cs_dialog)
    btk_window_set_title (BTK_WINDOW (color_button->priv->cs_dialog), 
			  color_button->priv->title);
  
  g_object_notify (G_OBJECT (color_button), "title");
}

/**
 * btk_color_button_get_title:
 * @color_button: a #BtkColorButton
 *
 * Gets the title of the color selection dialog.
 *
 * Returns: An internal string, do not free the return value
 *
 * Since: 2.4
 */
const gchar *
btk_color_button_get_title (BtkColorButton *color_button)
{
  g_return_val_if_fail (BTK_IS_COLOR_BUTTON (color_button), NULL);

  return color_button->priv->title;
}

static void
btk_color_button_set_property (GObject      *object,
			       guint         param_id,
			       const GValue *value,
			       GParamSpec   *pspec)
{
  BtkColorButton *color_button = BTK_COLOR_BUTTON (object);

  switch (param_id) 
    {
    case PROP_USE_ALPHA:
      btk_color_button_set_use_alpha (color_button, g_value_get_boolean (value));
      break;
    case PROP_TITLE:
      btk_color_button_set_title (color_button, g_value_get_string (value));
      break;
    case PROP_COLOR:
      btk_color_button_set_color (color_button, g_value_get_boxed (value));
      break;
    case PROP_ALPHA:
      btk_color_button_set_alpha (color_button, g_value_get_uint (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
btk_color_button_get_property (GObject    *object,
			       guint       param_id,
			       GValue     *value,
			       GParamSpec *pspec)
{
  BtkColorButton *color_button = BTK_COLOR_BUTTON (object);
  BdkColor color;

  switch (param_id) 
    {
    case PROP_USE_ALPHA:
      g_value_set_boolean (value, btk_color_button_get_use_alpha (color_button));
      break;
    case PROP_TITLE:
      g_value_set_string (value, btk_color_button_get_title (color_button));
      break;
    case PROP_COLOR:
      btk_color_button_get_color (color_button, &color);
      g_value_set_boxed (value, &color);
      break;
    case PROP_ALPHA:
      g_value_set_uint (value, btk_color_button_get_alpha (color_button));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

#define __BTK_COLOR_BUTTON_C__
#include "btkaliasdef.c"
