/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */
#include "config.h"
#include <string.h>
#include <bunnylib.h>
#include "btkcolorseldialog.h"
#include "btkframe.h"
#include "btkhbbox.h"
#include "btkbutton.h"
#include "btkstock.h"
#include "btkintl.h"
#include "btkbuildable.h"
#include "btkalias.h"

enum {
  PROP_0,
  PROP_COLOR_SELECTION,
  PROP_OK_BUTTON,
  PROP_CANCEL_BUTTON,
  PROP_HELP_BUTTON
};


/***************************/
/* BtkColorSelectionDialog */
/***************************/

static void btk_color_selection_dialog_buildable_interface_init     (BtkBuildableIface *iface);
static GObject * btk_color_selection_dialog_buildable_get_internal_child (BtkBuildable *buildable,
									  BtkBuilder   *builder,
									  const gchar  *childname);

G_DEFINE_TYPE_WITH_CODE (BtkColorSelectionDialog, btk_color_selection_dialog,
           BTK_TYPE_DIALOG,
           G_IMPLEMENT_INTERFACE (BTK_TYPE_BUILDABLE,
                      btk_color_selection_dialog_buildable_interface_init))

static BtkBuildableIface *parent_buildable_iface;

static void
btk_color_selection_dialog_get_property (GObject         *object,
					 guint            prop_id,
					 GValue          *value,
					 GParamSpec      *pspec)
{
  BtkColorSelectionDialog *colorsel;

  colorsel = BTK_COLOR_SELECTION_DIALOG (object);

  switch (prop_id)
    {
    case PROP_COLOR_SELECTION:
      g_value_set_object (value, colorsel->colorsel);
      break;
    case PROP_OK_BUTTON:
      g_value_set_object (value, colorsel->ok_button);
      break;
    case PROP_CANCEL_BUTTON:
      g_value_set_object (value, colorsel->cancel_button);
      break;
    case PROP_HELP_BUTTON:
      g_value_set_object (value, colorsel->help_button);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_color_selection_dialog_class_init (BtkColorSelectionDialogClass *klass)
{
  GObjectClass   *bobject_class = G_OBJECT_CLASS (klass);
  bobject_class->get_property = btk_color_selection_dialog_get_property;

  g_object_class_install_property (bobject_class,
				   PROP_COLOR_SELECTION,
				   g_param_spec_object ("color-selection",
						     P_("Color Selection"),
						     P_("The color selection embedded in the dialog."),
						     BTK_TYPE_WIDGET,
						     G_PARAM_READABLE));
  g_object_class_install_property (bobject_class,
				   PROP_OK_BUTTON,
				   g_param_spec_object ("ok-button",
						     P_("OK Button"),
						     P_("The OK button of the dialog."),
						     BTK_TYPE_WIDGET,
						     G_PARAM_READABLE));
  g_object_class_install_property (bobject_class,
				   PROP_CANCEL_BUTTON,
				   g_param_spec_object ("cancel-button",
						     P_("Cancel Button"),
						     P_("The cancel button of the dialog."),
						     BTK_TYPE_WIDGET,
						     G_PARAM_READABLE));
  g_object_class_install_property (bobject_class,
				   PROP_HELP_BUTTON,
				   g_param_spec_object ("help-button",
						     P_("Help Button"),
						     P_("The help button of the dialog."),
						     BTK_TYPE_WIDGET,
						     G_PARAM_READABLE));
}

static void
btk_color_selection_dialog_init (BtkColorSelectionDialog *colorseldiag)
{
  BtkDialog *dialog = BTK_DIALOG (colorseldiag);

  btk_dialog_set_has_separator (dialog, FALSE);
  btk_container_set_border_width (BTK_CONTAINER (dialog), 5);
  btk_box_set_spacing (BTK_BOX (dialog->vbox), 2); /* 2 * 5 + 2 = 12 */
  btk_container_set_border_width (BTK_CONTAINER (dialog->action_area), 5);
  btk_box_set_spacing (BTK_BOX (dialog->action_area), 6);

  colorseldiag->colorsel = btk_color_selection_new ();
  btk_container_set_border_width (BTK_CONTAINER (colorseldiag->colorsel), 5);
  btk_color_selection_set_has_palette (BTK_COLOR_SELECTION(colorseldiag->colorsel), FALSE); 
  btk_color_selection_set_has_opacity_control (BTK_COLOR_SELECTION(colorseldiag->colorsel), FALSE);
  btk_container_add (BTK_CONTAINER (BTK_DIALOG (colorseldiag)->vbox), colorseldiag->colorsel);
  btk_widget_show (colorseldiag->colorsel);
  
  colorseldiag->cancel_button = btk_dialog_add_button (BTK_DIALOG (colorseldiag),
                                                       BTK_STOCK_CANCEL,
                                                       BTK_RESPONSE_CANCEL);

  colorseldiag->ok_button = btk_dialog_add_button (BTK_DIALOG (colorseldiag),
                                                   BTK_STOCK_OK,
                                                   BTK_RESPONSE_OK);
                                                   
  btk_widget_grab_default (colorseldiag->ok_button);
  
  colorseldiag->help_button = btk_dialog_add_button (BTK_DIALOG (colorseldiag),
                                                     BTK_STOCK_HELP,
                                                     BTK_RESPONSE_HELP);

  btk_widget_hide (colorseldiag->help_button);

  btk_dialog_set_alternative_button_order (BTK_DIALOG (colorseldiag),
					   BTK_RESPONSE_OK,
					   BTK_RESPONSE_CANCEL,
					   BTK_RESPONSE_HELP,
					   -1);

  btk_window_set_title (BTK_WINDOW (colorseldiag),
                        _("Color Selection"));

  _btk_dialog_set_ignore_separator (dialog, TRUE);
}

BtkWidget*
btk_color_selection_dialog_new (const gchar *title)
{
  BtkColorSelectionDialog *colorseldiag;
  
  colorseldiag = g_object_new (BTK_TYPE_COLOR_SELECTION_DIALOG, NULL);

  if (title)
    btk_window_set_title (BTK_WINDOW (colorseldiag), title);

  btk_window_set_resizable (BTK_WINDOW (colorseldiag), FALSE);
  
  return BTK_WIDGET (colorseldiag);
}

/**
 * btk_color_selection_dialog_get_color_selection:
 * @colorsel: a #BtkColorSelectionDialog
 *
 * Retrieves the #BtkColorSelection widget embedded in the dialog.
 *
 * Returns: (transfer none): the embedded #BtkColorSelection
 *
 * Since: 2.14
 **/
BtkWidget*
btk_color_selection_dialog_get_color_selection (BtkColorSelectionDialog *colorsel)
{
  g_return_val_if_fail (BTK_IS_COLOR_SELECTION_DIALOG (colorsel), NULL);

  return colorsel->colorsel;
}

static void
btk_color_selection_dialog_buildable_interface_init (BtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->get_internal_child = btk_color_selection_dialog_buildable_get_internal_child;
}

static GObject *
btk_color_selection_dialog_buildable_get_internal_child (BtkBuildable *buildable,
							 BtkBuilder   *builder,
							 const gchar  *childname)
{
    if (strcmp(childname, "ok_button") == 0)
	return G_OBJECT (BTK_COLOR_SELECTION_DIALOG (buildable)->ok_button);
    else if (strcmp(childname, "cancel_button") == 0)
	return G_OBJECT (BTK_COLOR_SELECTION_DIALOG (buildable)->cancel_button);
    else if (strcmp(childname, "help_button") == 0)
	return G_OBJECT (BTK_COLOR_SELECTION_DIALOG(buildable)->help_button);
    else if (strcmp(childname, "color_selection") == 0)
	return G_OBJECT (BTK_COLOR_SELECTION_DIALOG(buildable)->colorsel);

    return parent_buildable_iface->get_internal_child (buildable, builder, childname);
}


#define __BTK_COLOR_SELECTION_DIALOG_C__
#include "btkaliasdef.c"
