/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 2 -*- */
/* BTK - The GIMP Toolkit
 * Copyright (C) 2000 Red Hat, Inc.
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

/*
 * Modified by the BTK+ Team and others 1997-2003.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#include "config.h"
#include <string.h>

#include "btkmessagedialog.h"
#include "btkaccessible.h"
#include "btkbuildable.h"
#include "btklabel.h"
#include "btkhbox.h"
#include "btkvbox.h"
#include "btkimage.h"
#include "btkstock.h"
#include "btkiconfactory.h"
#include "btkintl.h"
#include "btkprivate.h"
#include "btkalias.h"

/**
 * SECTION:btkmessagedialog
 * @Short_description: A convenient message window
 * @Title: BtkMessageDialog
 * @See_also:#BtkDialog
 *
 * #BtkMessageDialog presents a dialog with an image representing the type of
 * message (Error, Question, etc.) alongside some message text. It's simply a
 * convenience widget; you could construct the equivalent of #BtkMessageDialog
 * from #BtkDialog without too much effort, but #BtkMessageDialog saves typing.
 *
 * The easiest way to do a modal message dialog is to use btk_dialog_run(), though
 * you can also pass in the %BTK_DIALOG_MODAL flag, btk_dialog_run() automatically
 * makes the dialog modal and waits for the user to respond to it. btk_dialog_run()
 * returns when any dialog button is clicked.
 * <example>
 * <title>A modal dialog.</title>
 * <programlisting>
 *  dialog = btk_message_dialog_new (main_application_window,
 *                                   BTK_DIALOG_DESTROY_WITH_PARENT,
 *                                   BTK_MESSAGE_ERROR,
 *                                   BTK_BUTTONS_CLOSE,
 *                                   "Error loading file '&percnt;s': &percnt;s",
 *                                   filename, g_strerror (errno));
 *  btk_dialog_run (BTK_DIALOG (dialog));
 *  btk_widget_destroy (dialog);
 * </programlisting>
 * </example>
 * You might do a non-modal #BtkMessageDialog as follows:
 * <example>
 * <title>A non-modal dialog.</title>
 * <programlisting>
 *  dialog = btk_message_dialog_new (main_application_window,
 *                                   BTK_DIALOG_DESTROY_WITH_PARENT,
 *                                   BTK_MESSAGE_ERROR,
 *                                   BTK_BUTTONS_CLOSE,
 *                                   "Error loading file '&percnt;s': &percnt;s",
 *                                   filename, g_strerror (errno));
 *
 *  /&ast; Destroy the dialog when the user responds to it (e.g. clicks a button) &ast;/
 *  g_signal_connect_swapped (dialog, "response",
 *                            G_CALLBACK (btk_widget_destroy),
 *                            dialog);
 * </programlisting>
 * </example>
 *
 * <refsect2 id="BtkMessageDialog-BUILDER-UI">
 * <title>BtkMessageDialog as BtkBuildable</title>
 * <para>
 * The BtkMessageDialog implementation of the BtkBuildable interface exposes
 * the message area as an internal child with the name "message_area".
 * </para>
 * </refsect2>
 */

#define BTK_MESSAGE_DIALOG_GET_PRIVATE(obj) (B_TYPE_INSTANCE_GET_PRIVATE ((obj), BTK_TYPE_MESSAGE_DIALOG, BtkMessageDialogPrivate))

typedef struct _BtkMessageDialogPrivate BtkMessageDialogPrivate;

struct _BtkMessageDialogPrivate
{
  BtkWidget *message_area; /* vbox for the primary and secondary labels, and any extra content from the caller */
  BtkWidget *secondary_label;
  guint message_type : 3;
  guint has_primary_markup : 1;
  guint has_secondary_text : 1;
};

static void btk_message_dialog_style_set  (BtkWidget             *widget,
                                           BtkStyle              *prev_style);

static void btk_message_dialog_set_property (BObject          *object,
					     guint             prop_id,
					     const BValue     *value,
					     BParamSpec       *pspec);
static void btk_message_dialog_get_property (BObject          *object,
					     guint             prop_id,
					     BValue           *value,
					     BParamSpec       *pspec);
static void btk_message_dialog_add_buttons  (BtkMessageDialog *message_dialog,
					     BtkButtonsType    buttons);
static void      btk_message_dialog_buildable_interface_init     (BtkBuildableIface *iface);
static BObject * btk_message_dialog_buildable_get_internal_child (BtkBuildable  *buildable,
                                                                  BtkBuilder    *builder,
                                                                  const gchar   *childname);


enum {
  PROP_0,
  PROP_MESSAGE_TYPE,
  PROP_BUTTONS,
  PROP_TEXT,
  PROP_USE_MARKUP,
  PROP_SECONDARY_TEXT,
  PROP_SECONDARY_USE_MARKUP,
  PROP_IMAGE,
  PROP_MESSAGE_AREA
};

G_DEFINE_TYPE_WITH_CODE (BtkMessageDialog, btk_message_dialog, BTK_TYPE_DIALOG,
                         G_IMPLEMENT_INTERFACE (BTK_TYPE_BUILDABLE,
                                                btk_message_dialog_buildable_interface_init))

static BtkBuildableIface *parent_buildable_iface;

static void
btk_message_dialog_buildable_interface_init (BtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->get_internal_child = btk_message_dialog_buildable_get_internal_child;
  iface->custom_tag_start = parent_buildable_iface->custom_tag_start;
  iface->custom_finished = parent_buildable_iface->custom_finished;
}

static BObject *
btk_message_dialog_buildable_get_internal_child (BtkBuildable *buildable,
                                                 BtkBuilder   *builder,
                                                 const gchar  *childname)
{
  if (strcmp (childname, "message_area") == 0)
    return B_OBJECT (btk_message_dialog_get_message_area (BTK_MESSAGE_DIALOG (buildable)));

  return parent_buildable_iface->get_internal_child (buildable, builder, childname);
}


static void
btk_message_dialog_class_init (BtkMessageDialogClass *class)
{
  BtkWidgetClass *widget_class;
  BObjectClass *bobject_class;

  widget_class = BTK_WIDGET_CLASS (class);
  bobject_class = B_OBJECT_CLASS (class);
  
  widget_class->style_set = btk_message_dialog_style_set;

  bobject_class->set_property = btk_message_dialog_set_property;
  bobject_class->get_property = btk_message_dialog_get_property;
  
  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("message-border",
                                                             P_("Image/label border"),
                                                             P_("Width of border around the label and image in the message dialog"),
                                                             0,
                                                             G_MAXINT,
                                                             12,
                                                             BTK_PARAM_READABLE));
  /**
   * BtkMessageDialog:use-separator:
   *
   * Whether to draw a separator line between the message label and the buttons
   * in the dialog.
   *
   * Since: 2.4
   *
   * Deprecated: 2.22: This style property will be removed in BTK+ 3
   */
  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_boolean ("use-separator",
								 P_("Use separator"),
								 P_("Whether to put a separator between the message dialog's text and the buttons"),
								 FALSE,
								 BTK_PARAM_READABLE));
  /**
   * BtkMessageDialog:message-type:
   *
   * The type of the message. The type is used to determine
   * the image that is shown in the dialog, unless the image is 
   * explicitly set by the ::image property.
   */
  g_object_class_install_property (bobject_class,
                                   PROP_MESSAGE_TYPE,
                                   g_param_spec_enum ("message-type",
						      P_("Message Type"),
						      P_("The type of message"),
						      BTK_TYPE_MESSAGE_TYPE,
                                                      BTK_MESSAGE_INFO,
                                                      BTK_PARAM_READWRITE | G_PARAM_CONSTRUCT));
  g_object_class_install_property (bobject_class,
                                   PROP_BUTTONS,
                                   g_param_spec_enum ("buttons",
						      P_("Message Buttons"),
						      P_("The buttons shown in the message dialog"),
						      BTK_TYPE_BUTTONS_TYPE,
                                                      BTK_BUTTONS_NONE,
                                                      BTK_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

  /**
   * BtkMessageDialog:text:
   * 
   * The primary text of the message dialog. If the dialog has 
   * a secondary text, this will appear as the title.
   *
   * Since: 2.10
   */
  g_object_class_install_property (bobject_class,
                                   PROP_TEXT,
                                   g_param_spec_string ("text",
                                                        P_("Text"),
                                                        P_("The primary text of the message dialog"),
                                                        "",
                                                        BTK_PARAM_READWRITE));

  /**
   * BtkMessageDialog:use-markup:
   * 
   * %TRUE if the primary text of the dialog includes Bango markup. 
   * See bango_parse_markup(). 
   *
   * Since: 2.10
   */
  g_object_class_install_property (bobject_class,
				   PROP_USE_MARKUP,
				   g_param_spec_boolean ("use-markup",
							 P_("Use Markup"),
							 P_("The primary text of the title includes Bango markup."),
							 FALSE,
							 BTK_PARAM_READWRITE));
  
  /**
   * BtkMessageDialog:secondary-text:
   * 
   * The secondary text of the message dialog. 
   *
   * Since: 2.10
   */
  g_object_class_install_property (bobject_class,
                                   PROP_SECONDARY_TEXT,
                                   g_param_spec_string ("secondary-text",
                                                        P_("Secondary Text"),
                                                        P_("The secondary text of the message dialog"),
                                                        NULL,
                                                        BTK_PARAM_READWRITE));

  /**
   * BtkMessageDialog:secondary-use-markup:
   * 
   * %TRUE if the secondary text of the dialog includes Bango markup. 
   * See bango_parse_markup(). 
   *
   * Since: 2.10
   */
  g_object_class_install_property (bobject_class,
				   PROP_SECONDARY_USE_MARKUP,
				   g_param_spec_boolean ("secondary-use-markup",
							 P_("Use Markup in secondary"),
							 P_("The secondary text includes Bango markup."),
							 FALSE,
							 BTK_PARAM_READWRITE));

  /**
   * BtkMessageDialog:image:
   * 
   * The image for this dialog.
   *
   * Since: 2.10
   */
  g_object_class_install_property (bobject_class,
                                   PROP_IMAGE,
                                   g_param_spec_object ("image",
                                                        P_("Image"),
                                                        P_("The image"),
                                                        BTK_TYPE_WIDGET,
                                                        BTK_PARAM_READWRITE));

  /**
   * BtkMessageDialog:message-area
   *
   * The #BtkVBox that corresponds to the message area of this dialog.  See
   * btk_message_dialog_get_message_area() for a detailed description of this
   * area.
   *
   * Since: 2.22
   */
  g_object_class_install_property (bobject_class,
				   PROP_MESSAGE_AREA,
				   g_param_spec_object ("message-area",
							P_("Message area"),
							P_("BtkVBox that holds the dialog's primary and secondary labels"),
							BTK_TYPE_WIDGET,
							BTK_PARAM_READABLE));

  g_type_class_add_private (bobject_class,
			    sizeof (BtkMessageDialogPrivate));
}

static void
btk_message_dialog_init (BtkMessageDialog *dialog)
{
  BtkWidget *hbox;
  BtkMessageDialogPrivate *priv;

  priv = BTK_MESSAGE_DIALOG_GET_PRIVATE (dialog);

  btk_window_set_resizable (BTK_WINDOW (dialog), FALSE);
  btk_window_set_title (BTK_WINDOW (dialog), "");
  btk_window_set_skip_taskbar_hint (BTK_WINDOW (dialog), TRUE);

  priv->has_primary_markup = FALSE;
  priv->has_secondary_text = FALSE;
  priv->secondary_label = btk_label_new (NULL);
  btk_widget_set_no_show_all (priv->secondary_label, TRUE);
  
  dialog->label = btk_label_new (NULL);
  dialog->image = btk_image_new_from_stock (NULL, BTK_ICON_SIZE_DIALOG);
  btk_misc_set_alignment (BTK_MISC (dialog->image), 0.5, 0.0);
  
  btk_label_set_line_wrap  (BTK_LABEL (dialog->label), TRUE);
  btk_label_set_selectable (BTK_LABEL (dialog->label), TRUE);
  btk_misc_set_alignment   (BTK_MISC  (dialog->label), 0.0, 0.0);
  
  btk_label_set_line_wrap  (BTK_LABEL (priv->secondary_label), TRUE);
  btk_label_set_selectable (BTK_LABEL (priv->secondary_label), TRUE);
  btk_misc_set_alignment   (BTK_MISC  (priv->secondary_label), 0.0, 0.0);

  hbox = btk_hbox_new (FALSE, 12);
  priv->message_area = btk_vbox_new (FALSE, 12);

  btk_box_pack_start (BTK_BOX (priv->message_area), dialog->label,
                      FALSE, FALSE, 0);

  btk_box_pack_start (BTK_BOX (priv->message_area), priv->secondary_label,
                      TRUE, TRUE, 0);

  btk_box_pack_start (BTK_BOX (hbox), dialog->image,
                      FALSE, FALSE, 0);

  btk_box_pack_start (BTK_BOX (hbox), priv->message_area,
                      TRUE, TRUE, 0);

  btk_box_pack_start (BTK_BOX (BTK_DIALOG (dialog)->vbox),
                      hbox,
                      FALSE, FALSE, 0);

  btk_container_set_border_width (BTK_CONTAINER (dialog), 5);
  btk_container_set_border_width (BTK_CONTAINER (hbox), 5);
  btk_box_set_spacing (BTK_BOX (BTK_DIALOG (dialog)->vbox), 14); /* 14 + 2 * 5 = 24 */
  btk_container_set_border_width (BTK_CONTAINER (BTK_DIALOG (dialog)->action_area), 5);
  btk_box_set_spacing (BTK_BOX (BTK_DIALOG (dialog)->action_area), 6);

  btk_widget_show_all (hbox);

  _btk_dialog_set_ignore_separator (BTK_DIALOG (dialog), TRUE);
}

static void
setup_primary_label_font (BtkMessageDialog *dialog)
{
  gint size;
  BangoFontDescription *font_desc;
  BtkMessageDialogPrivate *priv;

  priv = BTK_MESSAGE_DIALOG_GET_PRIVATE (dialog);

  /* unset the font settings */
  btk_widget_modify_font (dialog->label, NULL);

  if (priv->has_secondary_text && !priv->has_primary_markup)
    {
      size = bango_font_description_get_size (dialog->label->style->font_desc);
      font_desc = bango_font_description_new ();
      bango_font_description_set_weight (font_desc, BANGO_WEIGHT_BOLD);
      bango_font_description_set_size (font_desc, size * BANGO_SCALE_LARGE);
      btk_widget_modify_font (dialog->label, font_desc);
      bango_font_description_free (font_desc);
    }
}

static void
setup_type (BtkMessageDialog *dialog,
	    BtkMessageType    type)
{
  BtkMessageDialogPrivate *priv = BTK_MESSAGE_DIALOG_GET_PRIVATE (dialog);
  const gchar *stock_id = NULL;
  BatkObject *batk_obj;
 
  priv->message_type = type;

  switch (type)
    {
    case BTK_MESSAGE_INFO:
      stock_id = BTK_STOCK_DIALOG_INFO;
      break;

    case BTK_MESSAGE_QUESTION:
      stock_id = BTK_STOCK_DIALOG_QUESTION;
      break;

    case BTK_MESSAGE_WARNING:
      stock_id = BTK_STOCK_DIALOG_WARNING;
      break;
      
    case BTK_MESSAGE_ERROR:
      stock_id = BTK_STOCK_DIALOG_ERROR;
      break;

    case BTK_MESSAGE_OTHER:
      break;

    default:
      g_warning ("Unknown BtkMessageType %u", type);
      break;
    }

  if (stock_id)
    btk_image_set_from_stock (BTK_IMAGE (dialog->image), stock_id,
                              BTK_ICON_SIZE_DIALOG);
      
  batk_obj = btk_widget_get_accessible (BTK_WIDGET (dialog));
  if (BTK_IS_ACCESSIBLE (batk_obj))
    {
      batk_object_set_role (batk_obj, BATK_ROLE_ALERT);
      if (stock_id)
        {
          BtkStockItem item;

          btk_stock_lookup (stock_id, &item);
          batk_object_set_name (batk_obj, item.label);
        }
    }
}

static void 
btk_message_dialog_set_property (BObject      *object,
				 guint         prop_id,
				 const BValue *value,
				 BParamSpec   *pspec)
{
  BtkMessageDialog *dialog;
  BtkMessageDialogPrivate *priv;

  dialog = BTK_MESSAGE_DIALOG (object);
  priv = BTK_MESSAGE_DIALOG_GET_PRIVATE (dialog);
  
  switch (prop_id)
    {
    case PROP_MESSAGE_TYPE:
      setup_type (dialog, b_value_get_enum (value));
      break;
    case PROP_BUTTONS:
      btk_message_dialog_add_buttons (dialog, b_value_get_enum (value));
      break;
    case PROP_TEXT:
      if (priv->has_primary_markup)
	btk_label_set_markup (BTK_LABEL (dialog->label), 
			      b_value_get_string (value));
      else
	btk_label_set_text (BTK_LABEL (dialog->label), 
			    b_value_get_string (value));
      break;
    case PROP_USE_MARKUP:
      priv->has_primary_markup = b_value_get_boolean (value) != FALSE;
      btk_label_set_use_markup (BTK_LABEL (dialog->label), 
				priv->has_primary_markup);
      setup_primary_label_font (dialog);
      break;
    case PROP_SECONDARY_TEXT:
      {
	const gchar *txt = b_value_get_string (value);
	
	if (btk_label_get_use_markup (BTK_LABEL (priv->secondary_label)))
	  btk_label_set_markup (BTK_LABEL (priv->secondary_label), txt);
	else
	  btk_label_set_text (BTK_LABEL (priv->secondary_label), txt);

	if (txt)
	  {
	    priv->has_secondary_text = TRUE;
	    btk_widget_show (priv->secondary_label);
	  }
	else
	  {
	    priv->has_secondary_text = FALSE;
	    btk_widget_hide (priv->secondary_label);
	  }
	setup_primary_label_font (dialog);
      }
      break;
    case PROP_SECONDARY_USE_MARKUP:
      btk_label_set_use_markup (BTK_LABEL (priv->secondary_label), 
				b_value_get_boolean (value));
      break;
    case PROP_IMAGE:
      btk_message_dialog_set_image (dialog, b_value_get_object (value));
      break;

    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void 
btk_message_dialog_get_property (BObject     *object,
				 guint        prop_id,
				 BValue      *value,
				 BParamSpec  *pspec)
{
  BtkMessageDialog *dialog;
  BtkMessageDialogPrivate *priv;

  dialog = BTK_MESSAGE_DIALOG (object);
  priv = BTK_MESSAGE_DIALOG_GET_PRIVATE (dialog);
    
  switch (prop_id)
    {
    case PROP_MESSAGE_TYPE:
      b_value_set_enum (value, (BtkMessageType) priv->message_type);
      break;
    case PROP_TEXT:
      b_value_set_string (value, btk_label_get_label (BTK_LABEL (dialog->label)));
      break;
    case PROP_USE_MARKUP:
      b_value_set_boolean (value, priv->has_primary_markup);
      break;
    case PROP_SECONDARY_TEXT:
      if (priv->has_secondary_text)
      b_value_set_string (value, 
			  btk_label_get_label (BTK_LABEL (priv->secondary_label)));
      else
	b_value_set_string (value, NULL);
      break;
    case PROP_SECONDARY_USE_MARKUP:
      if (priv->has_secondary_text)
	b_value_set_boolean (value, 
			     btk_label_get_use_markup (BTK_LABEL (priv->secondary_label)));
      else
	b_value_set_boolean (value, FALSE);
      break;
    case PROP_IMAGE:
      b_value_set_object (value, dialog->image);
      break;
    case PROP_MESSAGE_AREA:
      b_value_set_object (value, priv->message_area);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/**
 * btk_message_dialog_new:
 * @parent: (allow-none): transient parent, or %NULL for none
 * @flags: flags
 * @type: type of message
 * @buttons: set of buttons to use
 * @message_format: (allow-none): printf()-style format string, or %NULL
 * @Varargs: arguments for @message_format
 *
 * Creates a new message dialog, which is a simple dialog with an icon
 * indicating the dialog type (error, warning, etc.) and some text the
 * user may want to see. When the user clicks a button a "response"
 * signal is emitted with response IDs from #BtkResponseType. See
 * #BtkDialog for more details.
 *
 * Return value: (transfer none): a new #BtkMessageDialog
 **/
BtkWidget*
btk_message_dialog_new (BtkWindow     *parent,
                        BtkDialogFlags flags,
                        BtkMessageType type,
                        BtkButtonsType buttons,
                        const gchar   *message_format,
                        ...)
{
  BtkWidget *widget;
  BtkDialog *dialog;
  gchar* msg = NULL;
  va_list args;

  g_return_val_if_fail (parent == NULL || BTK_IS_WINDOW (parent), NULL);

  widget = g_object_new (BTK_TYPE_MESSAGE_DIALOG,
			 "message-type", type,
			 "buttons", buttons,
			 NULL);
  dialog = BTK_DIALOG (widget);

  if (flags & BTK_DIALOG_NO_SEPARATOR)
    {
      g_warning ("The BTK_DIALOG_NO_SEPARATOR flag cannot be used for BtkMessageDialog");
      flags &= ~BTK_DIALOG_NO_SEPARATOR;
    }

  if (message_format)
    {
      va_start (args, message_format);
      msg = g_strdup_vprintf (message_format, args);
      va_end (args);

      btk_label_set_text (BTK_LABEL (BTK_MESSAGE_DIALOG (widget)->label),
                          msg);

      g_free (msg);
    }

  if (parent != NULL)
    btk_window_set_transient_for (BTK_WINDOW (widget),
                                  BTK_WINDOW (parent));
  
  if (flags & BTK_DIALOG_MODAL)
    btk_window_set_modal (BTK_WINDOW (dialog), TRUE);

  if (flags & BTK_DIALOG_DESTROY_WITH_PARENT)
    btk_window_set_destroy_with_parent (BTK_WINDOW (dialog), TRUE);

  return widget;
}

/**
 * btk_message_dialog_new_with_markup:
 * @parent: (allow-none): transient parent, or %NULL for none 
 * @flags: flags
 * @type: type of message
 * @buttons: set of buttons to use
 * @message_format: (allow-none): printf()-style format string, or %NULL
 * @Varargs: arguments for @message_format
 * 
 * Creates a new message dialog, which is a simple dialog with an icon
 * indicating the dialog type (error, warning, etc.) and some text which
 * is marked up with the <link linkend="BangoMarkupFormat">Bango text markup language</link>.
 * When the user clicks a button a "response" signal is emitted with
 * response IDs from #BtkResponseType. See #BtkDialog for more details.
 *
 * Special XML characters in the printf() arguments passed to this
 * function will automatically be escaped as necessary.
 * (See g_markup_printf_escaped() for how this is implemented.)
 * Usually this is what you want, but if you have an existing
 * Bango markup string that you want to use literally as the
 * label, then you need to use btk_message_dialog_set_markup()
 * instead, since you can't pass the markup string either
 * as the format (it might contain '%' characters) or as a string
 * argument.
 * |[
 *  BtkWidget *dialog;
 *  dialog = btk_message_dialog_new (main_application_window,
 *                                   BTK_DIALOG_DESTROY_WITH_PARENT,
 *                                   BTK_MESSAGE_ERROR,
 *                                   BTK_BUTTONS_CLOSE,
 *                                   NULL);
 *  btk_message_dialog_set_markup (BTK_MESSAGE_DIALOG (dialog),
 *                                 markup);
 * ]|
 * 
 * Return value: a new #BtkMessageDialog
 *
 * Since: 2.4
 **/
BtkWidget*
btk_message_dialog_new_with_markup (BtkWindow     *parent,
                                    BtkDialogFlags flags,
                                    BtkMessageType type,
                                    BtkButtonsType buttons,
                                    const gchar   *message_format,
                                    ...)
{
  BtkWidget *widget;
  va_list args;
  gchar *msg = NULL;

  g_return_val_if_fail (parent == NULL || BTK_IS_WINDOW (parent), NULL);

  widget = btk_message_dialog_new (parent, flags, type, buttons, NULL);

  if (message_format)
    {
      va_start (args, message_format);
      msg = g_markup_vprintf_escaped (message_format, args);
      va_end (args);

      btk_message_dialog_set_markup (BTK_MESSAGE_DIALOG (widget), msg);

      g_free (msg);
    }

  return widget;
}

/**
 * btk_message_dialog_set_image:
 * @dialog: a #BtkMessageDialog
 * @image: the image
 * 
 * Sets the dialog's image to @image.
 *
 * Since: 2.10
 **/
void
btk_message_dialog_set_image (BtkMessageDialog *dialog,
			      BtkWidget        *image)
{
  BtkMessageDialogPrivate *priv;
  BtkWidget *parent;

  g_return_if_fail (BTK_IS_MESSAGE_DIALOG (dialog));
  g_return_if_fail (image == NULL || BTK_IS_WIDGET (image));

  if (image == NULL)
    {
      image = btk_image_new_from_stock (NULL, BTK_ICON_SIZE_DIALOG);
      btk_misc_set_alignment (BTK_MISC (image), 0.5, 0.0);
    }

  priv = BTK_MESSAGE_DIALOG_GET_PRIVATE (dialog);

  priv->message_type = BTK_MESSAGE_OTHER;
  
  parent = dialog->image->parent;
  btk_container_add (BTK_CONTAINER (parent), image);
  btk_container_remove (BTK_CONTAINER (parent), dialog->image);
  btk_box_reorder_child (BTK_BOX (parent), image, 0);

  dialog->image = image;

  g_object_notify (B_OBJECT (dialog), "image");
}

/**
 * btk_message_dialog_get_image:
 * @dialog: a #BtkMessageDialog
 *
 * Gets the dialog's image.
 *
 * Return value: (transfer none): the dialog's image
 *
 * Since: 2.14
 **/
BtkWidget *
btk_message_dialog_get_image (BtkMessageDialog *dialog)
{
  g_return_val_if_fail (BTK_IS_MESSAGE_DIALOG (dialog), NULL);

  return dialog->image;
}

/**
 * btk_message_dialog_set_markup:
 * @message_dialog: a #BtkMessageDialog
 * @str: markup string (see <link linkend="BangoMarkupFormat">Bango markup format</link>)
 * 
 * Sets the text of the message dialog to be @str, which is marked
 * up with the <link linkend="BangoMarkupFormat">Bango text markup
 * language</link>.
 *
 * Since: 2.4
 **/
void
btk_message_dialog_set_markup (BtkMessageDialog *message_dialog,
                               const gchar      *str)
{
  BtkMessageDialogPrivate *priv;

  g_return_if_fail (BTK_IS_MESSAGE_DIALOG (message_dialog));

  priv = BTK_MESSAGE_DIALOG_GET_PRIVATE (message_dialog);
  priv->has_primary_markup = TRUE;
  btk_label_set_markup (BTK_LABEL (message_dialog->label), str);
}

/**
 * btk_message_dialog_format_secondary_text:
 * @message_dialog: a #BtkMessageDialog
 * @message_format: (allow-none): printf()-style format string, or %NULL
 * @Varargs: arguments for @message_format
 * 
 * Sets the secondary text of the message dialog to be @message_format 
 * (with printf()-style).
 *
 * Note that setting a secondary text makes the primary text become
 * bold, unless you have provided explicit markup.
 *
 * Since: 2.6
 **/
void
btk_message_dialog_format_secondary_text (BtkMessageDialog *message_dialog,
                                          const gchar      *message_format,
                                          ...)
{
  va_list args;
  gchar *msg = NULL;
  BtkMessageDialogPrivate *priv;

  g_return_if_fail (BTK_IS_MESSAGE_DIALOG (message_dialog));

  priv = BTK_MESSAGE_DIALOG_GET_PRIVATE (message_dialog);

  if (message_format)
    {
      priv->has_secondary_text = TRUE;

      va_start (args, message_format);
      msg = g_strdup_vprintf (message_format, args);
      va_end (args);

      btk_widget_show (priv->secondary_label);
      btk_label_set_text (BTK_LABEL (priv->secondary_label), msg);

      g_free (msg);
    }
  else
    {
      priv->has_secondary_text = FALSE;
      btk_widget_hide (priv->secondary_label);
    }

  setup_primary_label_font (message_dialog);
}

/**
 * btk_message_dialog_format_secondary_markup:
 * @message_dialog: a #BtkMessageDialog
 * @message_format: printf()-style markup string (see 
     <link linkend="BangoMarkupFormat">Bango markup format</link>), or %NULL
 * @Varargs: arguments for @message_format
 * 
 * Sets the secondary text of the message dialog to be @message_format (with 
 * printf()-style), which is marked up with the 
 * <link linkend="BangoMarkupFormat">Bango text markup language</link>.
 *
 * Note that setting a secondary text makes the primary text become
 * bold, unless you have provided explicit markup.
 *
 * Due to an oversight, this function does not escape special XML characters
 * like btk_message_dialog_new_with_markup() does. Thus, if the arguments 
 * may contain special XML characters, you should use g_markup_printf_escaped()
 * to escape it.

 * <informalexample><programlisting>
 * gchar *msg;
 *  
 * msg = g_markup_printf_escaped (message_format, ...);
 * btk_message_dialog_format_secondary_markup (message_dialog, "&percnt;s", msg);
 * g_free (msg);
 * </programlisting></informalexample>
 *
 * Since: 2.6
 **/
void
btk_message_dialog_format_secondary_markup (BtkMessageDialog *message_dialog,
                                            const gchar      *message_format,
                                            ...)
{
  va_list args;
  gchar *msg = NULL;
  BtkMessageDialogPrivate *priv;

  g_return_if_fail (BTK_IS_MESSAGE_DIALOG (message_dialog));

  priv = BTK_MESSAGE_DIALOG_GET_PRIVATE (message_dialog);

  if (message_format)
    {
      priv->has_secondary_text = TRUE;

      va_start (args, message_format);
      msg = g_strdup_vprintf (message_format, args);
      va_end (args);

      btk_widget_show (priv->secondary_label);
      btk_label_set_markup (BTK_LABEL (priv->secondary_label), msg);

      g_free (msg);
    }
  else
    {
      priv->has_secondary_text = FALSE;
      btk_widget_hide (priv->secondary_label);
    }

  setup_primary_label_font (message_dialog);
}

/**
 * btk_message_dialog_get_message_area:
 * @message_dialog: a #BtkMessageDialog
 *
 * Returns the message area of the dialog. This is the box where the
 * dialog's primary and secondary labels are packed. You can add your
 * own extra content to that box and it will appear below those labels,
 * on the right side of the dialog's image (or on the left for right-to-left
 * languages).  See btk_dialog_get_content_area() for the corresponding
 * function in the parent #BtkDialog.
 *
 * Return value: (transfer none): A #BtkVBox corresponding to the
 *     "message area" in the @message_dialog.
 *
 * Since: 2.22
 **/
BtkWidget *
btk_message_dialog_get_message_area (BtkMessageDialog *message_dialog)
{
  BtkMessageDialogPrivate *priv;

  g_return_val_if_fail (BTK_IS_MESSAGE_DIALOG (message_dialog), NULL);

  priv = BTK_MESSAGE_DIALOG_GET_PRIVATE (message_dialog);

  return priv->message_area;
}

static void
btk_message_dialog_add_buttons (BtkMessageDialog* message_dialog,
				BtkButtonsType buttons)
{
  BtkDialog* dialog = BTK_DIALOG (message_dialog);

  switch (buttons)
    {
    case BTK_BUTTONS_NONE:
      /* nothing */
      break;

    case BTK_BUTTONS_OK:
      btk_dialog_add_button (dialog,
                             BTK_STOCK_OK,
                             BTK_RESPONSE_OK);
      break;

    case BTK_BUTTONS_CLOSE:
      btk_dialog_add_button (dialog,
                             BTK_STOCK_CLOSE,
                             BTK_RESPONSE_CLOSE);
      break;

    case BTK_BUTTONS_CANCEL:
      btk_dialog_add_button (dialog,
                             BTK_STOCK_CANCEL,
                             BTK_RESPONSE_CANCEL);
      break;

    case BTK_BUTTONS_YES_NO:
      btk_dialog_add_button (dialog,
                             BTK_STOCK_NO,
                             BTK_RESPONSE_NO);
      btk_dialog_add_button (dialog,
                             BTK_STOCK_YES,
                             BTK_RESPONSE_YES);
      btk_dialog_set_alternative_button_order (BTK_DIALOG (dialog),
					       BTK_RESPONSE_YES,
					       BTK_RESPONSE_NO,
					       -1);
      break;

    case BTK_BUTTONS_OK_CANCEL:
      btk_dialog_add_button (dialog,
                             BTK_STOCK_CANCEL,
                             BTK_RESPONSE_CANCEL);
      btk_dialog_add_button (dialog,
                             BTK_STOCK_OK,
                             BTK_RESPONSE_OK);
      btk_dialog_set_alternative_button_order (BTK_DIALOG (dialog),
					       BTK_RESPONSE_OK,
					       BTK_RESPONSE_CANCEL,
					       -1);
      break;
      
    default:
      g_warning ("Unknown BtkButtonsType");
      break;
    } 

  g_object_notify (B_OBJECT (message_dialog), "buttons");
}

static void
btk_message_dialog_style_set (BtkWidget *widget,
                              BtkStyle  *prev_style)
{
  BtkMessageDialog *dialog = BTK_MESSAGE_DIALOG (widget);
  gboolean use_separator;
  BtkWidget *parent;
  gint border_width;

  parent = BTK_WIDGET (BTK_MESSAGE_DIALOG (widget)->image->parent);

  if (parent)
    {
      btk_widget_style_get (widget, "message-border",
                            &border_width, NULL);
      
      btk_container_set_border_width (BTK_CONTAINER (parent),
                                      MAX (0, border_width - 7));
    }

  btk_widget_style_get (widget,
			"use-separator", &use_separator,
			NULL);

  _btk_dialog_set_ignore_separator (BTK_DIALOG (widget), FALSE);
  btk_dialog_set_has_separator (BTK_DIALOG (widget), use_separator);
  _btk_dialog_set_ignore_separator (BTK_DIALOG (widget), TRUE);

  setup_primary_label_font (dialog);

  BTK_WIDGET_CLASS (btk_message_dialog_parent_class)->style_set (widget, prev_style);
}

#define __BTK_MESSAGE_DIALOG_C__
#include "btkaliasdef.c"
