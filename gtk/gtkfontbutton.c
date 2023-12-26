/* 
 * BTK - The GIMP Toolkit
 * Copyright (C) 1998 David Abilleira Freijeiro <odaf@nbexo.es>
 * All rights reserved.
 *
 * Based on gnome-color-picker by Federico Mena <federico@nuclecu.unam.mx>
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Cambridge, MA 02139, USA.
 */
/*
 * Modified by the BTK+ Team and others 2003.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#include "config.h"

#include "btkfontbutton.h"

#include "btkmain.h"
#include "btkalignment.h"
#include "btkhbox.h"
#include "btklabel.h"
#include "btkvseparator.h"
#include "btkfontsel.h"
#include "btkimage.h"
#include "btkmarshalers.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkalias.h"

#include <string.h>
#include <stdio.h>

#define BTK_FONT_BUTTON_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), BTK_TYPE_FONT_BUTTON, BtkFontButtonPrivate))

struct _BtkFontButtonPrivate 
{
  gchar         *title;
  
  gchar         *fontname;
  
  guint         use_font : 1;
  guint         use_size : 1;
  guint         show_style : 1;
  guint         show_size : 1;
   
  BtkWidget     *font_dialog;
  BtkWidget     *inside;
  BtkWidget     *font_label;
  BtkWidget     *size_label;
};

/* Signals */
enum 
{
  FONT_SET,
  LAST_SIGNAL
};

enum 
{
  PROP_0,
  PROP_TITLE,
  PROP_FONT_NAME,
  PROP_USE_FONT,
  PROP_USE_SIZE,
  PROP_SHOW_STYLE,
  PROP_SHOW_SIZE
};

/* Prototypes */
static void btk_font_button_finalize               (GObject            *object);
static void btk_font_button_get_property           (GObject            *object,
                                                    guint               param_id,
                                                    GValue             *value,
                                                    GParamSpec         *pspec);
static void btk_font_button_set_property           (GObject            *object,
                                                    guint               param_id,
                                                    const GValue       *value,
                                                    GParamSpec         *pspec);

static void btk_font_button_clicked                 (BtkButton         *button);

/* Dialog response functions */
static void dialog_ok_clicked                       (BtkWidget         *widget,
                                                     gpointer           data);
static void dialog_cancel_clicked                   (BtkWidget         *widget,
                                                     gpointer           data);
static void dialog_destroy                          (BtkWidget         *widget,
                                                     gpointer           data);

/* Auxiliary functions */
static BtkWidget *btk_font_button_create_inside     (BtkFontButton     *gfs);
static void btk_font_button_label_use_font          (BtkFontButton     *gfs);
static void btk_font_button_update_font_info        (BtkFontButton     *gfs);

static guint font_button_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (BtkFontButton, btk_font_button, BTK_TYPE_BUTTON)

static void
btk_font_button_class_init (BtkFontButtonClass *klass)
{
  GObjectClass *bobject_class;
  BtkButtonClass *button_class;
  
  bobject_class = (GObjectClass *) klass;
  button_class = (BtkButtonClass *) klass;

  bobject_class->finalize = btk_font_button_finalize;
  bobject_class->set_property = btk_font_button_set_property;
  bobject_class->get_property = btk_font_button_get_property;
  
  button_class->clicked = btk_font_button_clicked;
  
  klass->font_set = NULL;

  /**
   * BtkFontButton:title:
   * 
   * The title of the font selection dialog.
   *
   * Since: 2.4
   */
  g_object_class_install_property (bobject_class,
                                   PROP_TITLE,
                                   g_param_spec_string ("title",
                                                        P_("Title"),
                                                        P_("The title of the font selection dialog"),
                                                        _("Pick a Font"),
                                                        (BTK_PARAM_READABLE |
                                                         BTK_PARAM_WRITABLE)));

  /**
   * BtkFontButton:font-name:
   * 
   * The name of the currently selected font.
   *
   * Since: 2.4
   */
  g_object_class_install_property (bobject_class,
                                   PROP_FONT_NAME,
                                   g_param_spec_string ("font-name",
                                                        P_("Font name"),
                                                        P_("The name of the selected font"),
                                                        P_("Sans 12"),
                                                        (BTK_PARAM_READABLE |
                                                         BTK_PARAM_WRITABLE)));

  /**
   * BtkFontButton:use-font:
   * 
   * If this property is set to %TRUE, the label will be drawn 
   * in the selected font.
   *
   * Since: 2.4
   */
  g_object_class_install_property (bobject_class,
                                   PROP_USE_FONT,
                                   g_param_spec_boolean ("use-font",
                                                         P_("Use font in label"),
                                                         P_("Whether the label is drawn in the selected font"),
                                                         FALSE,
                                                         BTK_PARAM_READWRITE));

  /**
   * BtkFontButton:use-size:
   * 
   * If this property is set to %TRUE, the label will be drawn 
   * with the selected font size.
   *
   * Since: 2.4
   */
  g_object_class_install_property (bobject_class,
                                   PROP_USE_SIZE,
                                   g_param_spec_boolean ("use-size",
                                                         P_("Use size in label"),
                                                         P_("Whether the label is drawn with the selected font size"),
                                                         FALSE,
                                                         BTK_PARAM_READWRITE));

  /**
   * BtkFontButton:show-style:
   * 
   * If this property is set to %TRUE, the name of the selected font style 
   * will be shown in the label. For a more WYSIWYG way to show the selected 
   * style, see the ::use-font property. 
   *
   * Since: 2.4
   */
  g_object_class_install_property (bobject_class,
                                   PROP_SHOW_STYLE,
                                   g_param_spec_boolean ("show-style",
                                                         P_("Show style"),
                                                         P_("Whether the selected font style is shown in the label"),
                                                         TRUE,
                                                         BTK_PARAM_READWRITE));
  /**
   * BtkFontButton:show-size:
   * 
   * If this property is set to %TRUE, the selected font size will be shown 
   * in the label. For a more WYSIWYG way to show the selected size, see the 
   * ::use-size property. 
   *
   * Since: 2.4
   */
  g_object_class_install_property (bobject_class,
                                   PROP_SHOW_SIZE,
                                   g_param_spec_boolean ("show-size",
                                                         P_("Show size"),
                                                         P_("Whether selected font size is shown in the label"),
                                                         TRUE,
                                                         BTK_PARAM_READWRITE));

  /**
   * BtkFontButton::font-set:
   * @widget: the object which received the signal.
   * 
   * The ::font-set signal is emitted when the user selects a font. 
   * When handling this signal, use btk_font_button_get_font_name() 
   * to find out which font was just selected.
   *
   * Note that this signal is only emitted when the <emphasis>user</emphasis>
   * changes the font. If you need to react to programmatic font changes
   * as well, use the notify::font-name signal.
   *
   * Since: 2.4
   */
  font_button_signals[FONT_SET] = g_signal_new (I_("font-set"),
                                                G_TYPE_FROM_CLASS (bobject_class),
                                                G_SIGNAL_RUN_FIRST,
                                                G_STRUCT_OFFSET (BtkFontButtonClass, font_set),
                                                NULL, NULL,
                                                g_cclosure_marshal_VOID__VOID,
                                                G_TYPE_NONE, 0);
  
  g_type_class_add_private (bobject_class, sizeof (BtkFontButtonPrivate));
}

static void
btk_font_button_init (BtkFontButton *font_button)
{
  font_button->priv = BTK_FONT_BUTTON_GET_PRIVATE (font_button);

  /* Initialize fields */
  font_button->priv->fontname = g_strdup (_("Sans 12"));
  font_button->priv->use_font = FALSE;
  font_button->priv->use_size = FALSE;
  font_button->priv->show_style = TRUE;
  font_button->priv->show_size = TRUE;
  font_button->priv->font_dialog = NULL;
  font_button->priv->title = g_strdup (_("Pick a Font"));

  font_button->priv->inside = btk_font_button_create_inside (font_button);
  btk_container_add (BTK_CONTAINER (font_button), font_button->priv->inside);

  btk_font_button_update_font_info (font_button);  
}


static void
btk_font_button_finalize (GObject *object)
{
  BtkFontButton *font_button = BTK_FONT_BUTTON (object);

  if (font_button->priv->font_dialog != NULL) 
    btk_widget_destroy (font_button->priv->font_dialog);
  font_button->priv->font_dialog = NULL;

  g_free (font_button->priv->fontname);
  font_button->priv->fontname = NULL;
  
  g_free (font_button->priv->title);
  font_button->priv->title = NULL;
  
  G_OBJECT_CLASS (btk_font_button_parent_class)->finalize (object);
}

static void
btk_font_button_set_property (GObject      *object,
                              guint         param_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtkFontButton *font_button = BTK_FONT_BUTTON (object);
  
  switch (param_id) 
    {
    case PROP_TITLE:
      btk_font_button_set_title (font_button, g_value_get_string (value));
      break;
    case PROP_FONT_NAME:
      btk_font_button_set_font_name (font_button, g_value_get_string (value));
      break;
    case PROP_USE_FONT:
      btk_font_button_set_use_font (font_button, g_value_get_boolean (value));
      break;
    case PROP_USE_SIZE:
      btk_font_button_set_use_size (font_button, g_value_get_boolean (value));
      break;
    case PROP_SHOW_STYLE:
      btk_font_button_set_show_style (font_button, g_value_get_boolean (value));
      break;
    case PROP_SHOW_SIZE:
      btk_font_button_set_show_size (font_button, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
btk_font_button_get_property (GObject    *object,
                              guint       param_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  BtkFontButton *font_button = BTK_FONT_BUTTON (object);
  
  switch (param_id) 
    {
    case PROP_TITLE:
      g_value_set_string (value, btk_font_button_get_title (font_button));
      break;
    case PROP_FONT_NAME:
      g_value_set_string (value, btk_font_button_get_font_name (font_button));
      break;
    case PROP_USE_FONT:
      g_value_set_boolean (value, btk_font_button_get_use_font (font_button));
      break;
    case PROP_USE_SIZE:
      g_value_set_boolean (value, btk_font_button_get_use_size (font_button));
      break;
    case PROP_SHOW_STYLE:
      g_value_set_boolean (value, btk_font_button_get_show_style (font_button));
      break;
    case PROP_SHOW_SIZE:
      g_value_set_boolean (value, btk_font_button_get_show_size (font_button));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
} 


/**
 * btk_font_button_new:
 *
 * Creates a new font picker widget.
 *
 * Returns: a new font picker widget.
 *
 * Since: 2.4
 */
BtkWidget *
btk_font_button_new (void)
{
  return g_object_new (BTK_TYPE_FONT_BUTTON, NULL);
} 

/**
 * btk_font_button_new_with_font:
 * @fontname: Name of font to display in font selection dialog
 *
 * Creates a new font picker widget.
 *
 * Returns: a new font picker widget.
 *
 * Since: 2.4
 */
BtkWidget *
btk_font_button_new_with_font (const gchar *fontname)
{
  return g_object_new (BTK_TYPE_FONT_BUTTON, "font-name", fontname, NULL);
} 

/**
 * btk_font_button_set_title:
 * @font_button: a #BtkFontButton
 * @title: a string containing the font selection dialog title
 *
 * Sets the title for the font selection dialog.  
 *
 * Since: 2.4
 */
void
btk_font_button_set_title (BtkFontButton *font_button, 
                           const gchar   *title)
{
  gchar *old_title;
  g_return_if_fail (BTK_IS_FONT_BUTTON (font_button));
  
  old_title = font_button->priv->title;
  font_button->priv->title = g_strdup (title);
  g_free (old_title);
  
  if (font_button->priv->font_dialog)
    btk_window_set_title (BTK_WINDOW (font_button->priv->font_dialog),
                          font_button->priv->title);

  g_object_notify (G_OBJECT (font_button), "title");
} 

/**
 * btk_font_button_get_title:
 * @font_button: a #BtkFontButton
 *
 * Retrieves the title of the font selection dialog.
 *
 * Returns: an internal copy of the title string which must not be freed.
 *
 * Since: 2.4
 */
const gchar*
btk_font_button_get_title (BtkFontButton *font_button)
{
  g_return_val_if_fail (BTK_IS_FONT_BUTTON (font_button), NULL);

  return font_button->priv->title;
} 

/**
 * btk_font_button_get_use_font:
 * @font_button: a #BtkFontButton
 *
 * Returns whether the selected font is used in the label.
 *
 * Returns: whether the selected font is used in the label.
 *
 * Since: 2.4
 */
gboolean
btk_font_button_get_use_font (BtkFontButton *font_button)
{
  g_return_val_if_fail (BTK_IS_FONT_BUTTON (font_button), FALSE);

  return font_button->priv->use_font;
} 

/**
 * btk_font_button_set_use_font:
 * @font_button: a #BtkFontButton
 * @use_font: If %TRUE, font name will be written using font chosen.
 *
 * If @use_font is %TRUE, the font name will be written using the selected font.  
 *
 * Since: 2.4
 */
void  
btk_font_button_set_use_font (BtkFontButton *font_button,
			      gboolean       use_font)
{
  g_return_if_fail (BTK_IS_FONT_BUTTON (font_button));
  
  use_font = (use_font != FALSE);
  
  if (font_button->priv->use_font != use_font) 
    {
      font_button->priv->use_font = use_font;

      if (use_font)
        btk_font_button_label_use_font (font_button);
      else
	btk_widget_set_style (font_button->priv->font_label, NULL);
 
     g_object_notify (G_OBJECT (font_button), "use-font");
    }
} 


/**
 * btk_font_button_get_use_size:
 * @font_button: a #BtkFontButton
 *
 * Returns whether the selected size is used in the label.
 *
 * Returns: whether the selected size is used in the label.
 *
 * Since: 2.4
 */
gboolean
btk_font_button_get_use_size (BtkFontButton *font_button)
{
  g_return_val_if_fail (BTK_IS_FONT_BUTTON (font_button), FALSE);

  return font_button->priv->use_size;
} 

/**
 * btk_font_button_set_use_size:
 * @font_button: a #BtkFontButton
 * @use_size: If %TRUE, font name will be written using the selected size.
 *
 * If @use_size is %TRUE, the font name will be written using the selected size.
 *
 * Since: 2.4
 */
void  
btk_font_button_set_use_size (BtkFontButton *font_button,
			      gboolean       use_size)
{
  g_return_if_fail (BTK_IS_FONT_BUTTON (font_button));
  
  use_size = (use_size != FALSE);
  if (font_button->priv->use_size != use_size) 
    {
      font_button->priv->use_size = use_size;

      if (font_button->priv->use_font)
        btk_font_button_label_use_font (font_button);

      g_object_notify (G_OBJECT (font_button), "use-size");
    }
} 

/**
 * btk_font_button_get_show_style:
 * @font_button: a #BtkFontButton
 * 
 * Returns whether the name of the font style will be shown in the label.
 * 
 * Return value: whether the font style will be shown in the label.
 *
 * Since: 2.4
 **/
gboolean 
btk_font_button_get_show_style (BtkFontButton *font_button)
{
  g_return_val_if_fail (BTK_IS_FONT_BUTTON (font_button), FALSE);

  return font_button->priv->show_style;
}

/**
 * btk_font_button_set_show_style:
 * @font_button: a #BtkFontButton
 * @show_style: %TRUE if font style should be displayed in label.
 *
 * If @show_style is %TRUE, the font style will be displayed along with name of the selected font.
 *
 * Since: 2.4
 */
void
btk_font_button_set_show_style (BtkFontButton *font_button,
                                gboolean       show_style)
{
  g_return_if_fail (BTK_IS_FONT_BUTTON (font_button));
  
  show_style = (show_style != FALSE);
  if (font_button->priv->show_style != show_style) 
    {
      font_button->priv->show_style = show_style;
      
      btk_font_button_update_font_info (font_button);
  
      g_object_notify (G_OBJECT (font_button), "show-style");
    }
} 


/**
 * btk_font_button_get_show_size:
 * @font_button: a #BtkFontButton
 * 
 * Returns whether the font size will be shown in the label.
 * 
 * Return value: whether the font size will be shown in the label.
 *
 * Since: 2.4
 **/
gboolean 
btk_font_button_get_show_size (BtkFontButton *font_button)
{
  g_return_val_if_fail (BTK_IS_FONT_BUTTON (font_button), FALSE);

  return font_button->priv->show_size;
}

/**
 * btk_font_button_set_show_size:
 * @font_button: a #BtkFontButton
 * @show_size: %TRUE if font size should be displayed in dialog.
 *
 * If @show_size is %TRUE, the font size will be displayed along with the name of the selected font.
 *
 * Since: 2.4
 */
void
btk_font_button_set_show_size (BtkFontButton *font_button,
                               gboolean       show_size)
{
  g_return_if_fail (BTK_IS_FONT_BUTTON (font_button));
  
  show_size = (show_size != FALSE);

  if (font_button->priv->show_size != show_size) 
    {
      font_button->priv->show_size = show_size;

      btk_container_remove (BTK_CONTAINER (font_button), font_button->priv->inside);
      font_button->priv->inside = btk_font_button_create_inside (font_button);
      btk_container_add (BTK_CONTAINER (font_button), font_button->priv->inside);
      
      btk_font_button_update_font_info (font_button);

      g_object_notify (G_OBJECT (font_button), "show-size");
    }
} 


/**
 * btk_font_button_get_font_name:
 * @font_button: a #BtkFontButton
 *
 * Retrieves the name of the currently selected font. This name includes
 * style and size information as well. If you want to render something
 * with the font, use this string with bango_font_description_from_string() .
 * If you're interested in peeking certain values (family name,
 * style, size, weight) just query these properties from the
 * #BangoFontDescription object.
 *
 * Returns: an internal copy of the font name which must not be freed.
 *
 * Since: 2.4
 */
const gchar *
btk_font_button_get_font_name (BtkFontButton *font_button)
{
  g_return_val_if_fail (BTK_IS_FONT_BUTTON (font_button), NULL);
  
  return font_button->priv->fontname;
}

/**
 * btk_font_button_set_font_name:
 * @font_button: a #BtkFontButton
 * @fontname: Name of font to display in font selection dialog
 *
 * Sets or updates the currently-displayed font in font picker dialog.
 *
 * Returns: Return value of btk_font_selection_dialog_set_font_name() if the
 * font selection dialog exists, otherwise %FALSE.
 *
 * Since: 2.4
 */
gboolean 
btk_font_button_set_font_name (BtkFontButton *font_button,
                               const gchar    *fontname)
{
  gboolean result;
  gchar *old_fontname;

  g_return_val_if_fail (BTK_IS_FONT_BUTTON (font_button), FALSE);
  g_return_val_if_fail (fontname != NULL, FALSE);
  
  if (g_ascii_strcasecmp (font_button->priv->fontname, fontname)) 
    {
      old_fontname = font_button->priv->fontname;
      font_button->priv->fontname = g_strdup (fontname);
      g_free (old_fontname);
    }
  
  btk_font_button_update_font_info (font_button);
  
  if (font_button->priv->font_dialog)
    result = btk_font_selection_dialog_set_font_name (BTK_FONT_SELECTION_DIALOG (font_button->priv->font_dialog), 
                                                      font_button->priv->fontname);
  else
    result = FALSE;

  g_object_notify (G_OBJECT (font_button), "font-name");

  return result;
}

static void
btk_font_button_clicked (BtkButton *button)
{
  BtkFontSelectionDialog *font_dialog;
  BtkFontButton    *font_button = BTK_FONT_BUTTON (button);
  
  if (!font_button->priv->font_dialog) 
    {
      BtkWidget *parent;
      
      parent = btk_widget_get_toplevel (BTK_WIDGET (font_button));
      
      font_button->priv->font_dialog = btk_font_selection_dialog_new (font_button->priv->title);
      
      font_dialog = BTK_FONT_SELECTION_DIALOG (font_button->priv->font_dialog);
      
      if (btk_widget_is_toplevel (parent) && BTK_IS_WINDOW (parent))
        {
          if (BTK_WINDOW (parent) != btk_window_get_transient_for (BTK_WINDOW (font_dialog)))
 	    btk_window_set_transient_for (BTK_WINDOW (font_dialog), BTK_WINDOW (parent));
	       
	  btk_window_set_modal (BTK_WINDOW (font_dialog),
				btk_window_get_modal (BTK_WINDOW (parent)));
	}

      g_signal_connect (font_dialog->ok_button, "clicked",
                        G_CALLBACK (dialog_ok_clicked), font_button);
      g_signal_connect (font_dialog->cancel_button, "clicked",
			G_CALLBACK (dialog_cancel_clicked), font_button);
      g_signal_connect (font_dialog, "destroy",
                        G_CALLBACK (dialog_destroy), font_button);
    }
  
  if (!btk_widget_get_visible (font_button->priv->font_dialog))
    {
      font_dialog = BTK_FONT_SELECTION_DIALOG (font_button->priv->font_dialog);
      
      btk_font_selection_dialog_set_font_name (font_dialog, font_button->priv->fontname);
      
    } 

  btk_window_present (BTK_WINDOW (font_button->priv->font_dialog));
}

static void
dialog_ok_clicked (BtkWidget *widget,
		   gpointer   data)
{
  BtkFontButton *font_button = BTK_FONT_BUTTON (data);
  
  btk_widget_hide (font_button->priv->font_dialog);
  
  g_free (font_button->priv->fontname);
  font_button->priv->fontname = btk_font_selection_dialog_get_font_name (BTK_FONT_SELECTION_DIALOG (font_button->priv->font_dialog));
  
  /* Set label font */
  btk_font_button_update_font_info (font_button);

  g_object_notify (G_OBJECT (font_button), "font-name");
  
  /* Emit font_set signal */
  g_signal_emit (font_button, font_button_signals[FONT_SET], 0);
}


static void
dialog_cancel_clicked (BtkWidget *widget,
		       gpointer   data)
{
  BtkFontButton *font_button = BTK_FONT_BUTTON (data);
  
  btk_widget_hide (font_button->priv->font_dialog);  
}

static void
dialog_destroy (BtkWidget *widget,
		gpointer   data)
{
  BtkFontButton *font_button = BTK_FONT_BUTTON (data);
    
  /* Dialog will get destroyed so reference is not valid now */
  font_button->priv->font_dialog = NULL;
} 

static BtkWidget *
btk_font_button_create_inside (BtkFontButton *font_button)
{
  BtkWidget *widget;
  
  btk_widget_push_composite_child ();

  widget = btk_hbox_new (FALSE, 0);
  
  font_button->priv->font_label = btk_label_new (_("Font"));
  
  btk_label_set_justify (BTK_LABEL (font_button->priv->font_label), BTK_JUSTIFY_LEFT);
  btk_box_pack_start (BTK_BOX (widget), font_button->priv->font_label, TRUE, TRUE, 5);

  if (font_button->priv->show_size) 
    {
      btk_box_pack_start (BTK_BOX (widget), btk_vseparator_new (), FALSE, FALSE, 0);
      font_button->priv->size_label = btk_label_new ("14");
      btk_box_pack_start (BTK_BOX (widget), font_button->priv->size_label, FALSE, FALSE, 5);
    }

  btk_widget_show_all (widget);

  btk_widget_pop_composite_child ();

  return widget;
} 

static void
btk_font_button_label_use_font (BtkFontButton *font_button)
{
  BangoFontDescription *desc;

  if (!font_button->priv->use_font)
    return;

  desc = bango_font_description_from_string (font_button->priv->fontname);
  
  if (!font_button->priv->use_size)
    bango_font_description_unset_fields (desc, BANGO_FONT_MASK_SIZE);

  btk_widget_modify_font (font_button->priv->font_label, desc);

  bango_font_description_free (desc);
}

static gboolean
font_description_style_equal (const BangoFontDescription *a,
                              const BangoFontDescription *b)
{
  return (bango_font_description_get_weight (a) == bango_font_description_get_weight (b) &&
          bango_font_description_get_style (a) == bango_font_description_get_style (b) &&
          bango_font_description_get_stretch (a) == bango_font_description_get_stretch (b) &&
          bango_font_description_get_variant (a) == bango_font_description_get_variant (b));
}

static void
btk_font_button_update_font_info (BtkFontButton *font_button)
{
  BangoFontDescription *desc;
  const gchar *family;
  gchar *style;
  gchar *family_style;
  
  desc = bango_font_description_from_string (font_button->priv->fontname);
  family = bango_font_description_get_family (desc);
  
#if 0
  /* This gives the wrong names, e.g. Italic when the font selection
   * dialog displayed Oblique.
   */
  bango_font_description_unset_fields (desc, BANGO_FONT_MASK_FAMILY | BANGO_FONT_MASK_SIZE);
  style = bango_font_description_to_string (desc);
  btk_label_set_text (BTK_LABEL (font_button->priv->style_label), style);      
#endif

  style = NULL;
  if (font_button->priv->show_style && family) 
    {
      BangoFontFamily **families;
      BangoFontFace **faces;
      gint n_families, n_faces, i;

      n_families = 0;
      families = NULL;
      bango_context_list_families (btk_widget_get_bango_context (BTK_WIDGET (font_button)),
                                   &families, &n_families);
      n_faces = 0;
      faces = NULL;
      for (i = 0; i < n_families; i++) 
        {
          const gchar *name = bango_font_family_get_name (families[i]);
          
          if (!g_ascii_strcasecmp (name, family)) 
            {
              bango_font_family_list_faces (families[i], &faces, &n_faces);
              break;
            }
        }
      g_free (families);
      
      for (i = 0; i < n_faces; i++) 
        {
          BangoFontDescription *tmp_desc = bango_font_face_describe (faces[i]);
          
          if (font_description_style_equal (tmp_desc, desc)) 
            {
              style = g_strdup (bango_font_face_get_face_name (faces[i]));
              bango_font_description_free (tmp_desc);
              break;
            }
          else
            bango_font_description_free (tmp_desc);
        }
      g_free (faces);
    }

  if (style == NULL || !g_ascii_strcasecmp (style, "Regular"))
    family_style = g_strdup (family);
  else
    family_style = g_strdup_printf ("%s %s", family, style);
  
  btk_label_set_text (BTK_LABEL (font_button->priv->font_label), family_style);
  
  g_free (style);
  g_free (family_style);

  if (font_button->priv->show_size) 
    {
      gchar *size = g_strdup_printf ("%g",
                                     bango_font_description_get_size (desc) / (double)BANGO_SCALE);
      
      btk_label_set_text (BTK_LABEL (font_button->priv->size_label), size);
      
      g_free (size);
    }

  btk_font_button_label_use_font (font_button);
  
  bango_font_description_free (desc);
} 

#define __BTK_FONT_BUTTON_C__
#include "btkaliasdef.c"
