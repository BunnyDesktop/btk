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
 * btkinputdialog.c
 *
 * Copyright 1997 Owen Taylor <owt1@cornell.edu>
 *
 */

/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/.
 */

#include "config.h"

#include <stdlib.h>

#include "bdk/bdkkeysyms.h"

#undef BTK_DISABLE_DEPRECATED /* BtkOptionMenu */

#include "btkinputdialog.h"
#include "btkbutton.h"
#include "btkentry.h"
#include "btkhbox.h"
#include "btklabel.h"
#include "btkmain.h"
#include "btkmarshalers.h"
#include "btkmenu.h"
#include "btkmenuitem.h"
#include "btknotebook.h"
#include "btkoptionmenu.h"
#include "btkscrolledwindow.h"
#include "btkstock.h"
#include "btktable.h"
#include "btkvbox.h"

#include "btkintl.h"
#include "btkalias.h"

typedef struct _BtkInputDialogPrivate BtkInputDialogPrivate;
typedef struct _BtkInputKeyInfo       BtkInputKeyInfo;

struct _BtkInputDialogPrivate
{
  BtkWidget *device_menu;
  BtkWidget *device_optionmenu;
  BtkWidget *no_devices_label;
  BtkWidget *main_vbox;
};

struct _BtkInputKeyInfo
{
  gint       index;
  BtkWidget *entry;
  BtkInputDialog *inputd;
};

enum
{
  ENABLE_DEVICE,
  DISABLE_DEVICE,
  LAST_SIGNAL
};


#define AXIS_LIST_WIDTH 160
#define AXIS_LIST_HEIGHT 175

#define KEYS_LIST_WIDTH 200
#define KEYS_LIST_HEIGHT 175

/* Forward declarations */

static void btk_input_dialog_screen_changed   (BtkWidget           *widget,
					       BdkScreen           *previous_screen);
static void btk_input_dialog_set_device       (BtkWidget           *widget,
					       gpointer             data);
static void btk_input_dialog_set_mapping_mode (BtkWidget           *w,
					       gpointer             data);
static void btk_input_dialog_set_axis         (BtkWidget           *widget,
					       gpointer             data);
static void btk_input_dialog_fill_axes        (BtkInputDialog      *inputd,
					       BdkDevice           *info);
static void btk_input_dialog_set_key          (BtkInputKeyInfo     *key,
					       guint                keyval,
					       BdkModifierType      modifiers);
static gboolean btk_input_dialog_key_press    (BtkWidget           *widget,
					       BdkEventKey         *event,
					       BtkInputKeyInfo     *key);
static void btk_input_dialog_clear_key        (BtkWidget           *widget,
					       BtkInputKeyInfo     *key);
static void btk_input_dialog_destroy_key      (BtkWidget           *widget,
					       BtkInputKeyInfo     *key);
static void btk_input_dialog_fill_keys        (BtkInputDialog      *inputd,
					       BdkDevice           *info);

static guint input_dialog_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (BtkInputDialog, btk_input_dialog, BTK_TYPE_DIALOG)

static BtkInputDialogPrivate *
btk_input_dialog_get_private (BtkInputDialog *input_dialog)
{
  return B_TYPE_INSTANCE_GET_PRIVATE (input_dialog, 
				      BTK_TYPE_INPUT_DIALOG, 
				      BtkInputDialogPrivate);
}

static BtkInputDialog *
input_dialog_from_widget (BtkWidget *widget)
{
  BtkWidget *toplevel;
  
  if (BTK_IS_MENU_ITEM (widget))
    {
      BtkMenu *menu = BTK_MENU (widget->parent);
      widget = btk_menu_get_attach_widget (menu);
    }

  toplevel = btk_widget_get_toplevel (widget);
  return BTK_INPUT_DIALOG (toplevel);
}

static void
btk_input_dialog_class_init (BtkInputDialogClass *klass)
{
  BObjectClass *object_class = (BObjectClass *) klass;
  BtkWidgetClass *widget_class = (BtkWidgetClass *)klass;
  
  widget_class->screen_changed = btk_input_dialog_screen_changed;
  
  klass->enable_device = NULL;
  klass->disable_device = NULL;

  input_dialog_signals[ENABLE_DEVICE] =
    g_signal_new (I_("enable-device"),
		  B_OBJECT_CLASS_TYPE (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkInputDialogClass, enable_device),
		  NULL, NULL,
		  _btk_marshal_VOID__OBJECT,
		  B_TYPE_NONE, 1,
		  BDK_TYPE_DEVICE);

  input_dialog_signals[DISABLE_DEVICE] =
    g_signal_new (I_("disable-device"),
		  B_OBJECT_CLASS_TYPE (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkInputDialogClass, disable_device),
		  NULL, NULL,
		  _btk_marshal_VOID__OBJECT,
		  B_TYPE_NONE, 1,
		  BDK_TYPE_DEVICE);

  g_type_class_add_private (object_class, sizeof (BtkInputDialogPrivate));
}

static void
btk_input_dialog_init (BtkInputDialog *inputd)
{
  BtkInputDialogPrivate *private = btk_input_dialog_get_private (inputd);
  BtkDialog *dialog = BTK_DIALOG (inputd);
  BtkWidget *util_box;
  BtkWidget *label;
  BtkWidget *mapping_menu;
  BtkWidget *menuitem;
  BtkWidget *notebook;

  btk_widget_push_composite_child ();

  btk_window_set_title (BTK_WINDOW (inputd), _("Input"));

  btk_dialog_set_has_separator (dialog, FALSE);
  btk_container_set_border_width (BTK_CONTAINER (dialog), 5);
  btk_box_set_spacing (BTK_BOX (dialog->vbox), 2); /* 2 * 5 + 2 = 12 */
  btk_container_set_border_width (BTK_CONTAINER (dialog->action_area), 5);
  btk_box_set_spacing (BTK_BOX (dialog->action_area), 6);

  /* main vbox */

  private->main_vbox = btk_vbox_new (FALSE, 12);
  btk_container_set_border_width (BTK_CONTAINER (private->main_vbox), 5);
  btk_box_pack_start (BTK_BOX (BTK_DIALOG (inputd)->vbox), private->main_vbox,
		      TRUE, TRUE, 0);

  private->no_devices_label = btk_label_new (_("No extended input devices"));
  btk_container_set_border_width (BTK_CONTAINER (private->main_vbox), 5);
  btk_box_pack_start (BTK_BOX (BTK_DIALOG (inputd)->vbox),
		      private->no_devices_label,
		      TRUE, TRUE, 0);

  /* menu for selecting device */

  private->device_menu = btk_menu_new ();

  util_box = btk_hbox_new (FALSE, 12);
  btk_box_pack_start (BTK_BOX (private->main_vbox), util_box, FALSE, FALSE, 0);

  label = btk_label_new_with_mnemonic (_("_Device:"));
  btk_box_pack_start (BTK_BOX (util_box), label, FALSE, FALSE, 0);

  private->device_optionmenu = btk_option_menu_new ();
  btk_label_set_mnemonic_widget (BTK_LABEL (label), private->device_optionmenu);
  btk_box_pack_start (BTK_BOX (util_box), private->device_optionmenu, TRUE, TRUE, 0);
  btk_widget_show (private->device_optionmenu);
  btk_option_menu_set_menu (BTK_OPTION_MENU (private->device_optionmenu), private->device_menu);

  btk_widget_show (label);

  /* Device options */

  /* mapping mode option menu */

  mapping_menu = btk_menu_new ();

  menuitem = btk_menu_item_new_with_label(_("Disabled"));
  btk_menu_shell_append (BTK_MENU_SHELL (mapping_menu), menuitem);
  btk_widget_show (menuitem);
  g_signal_connect (menuitem, "activate",
		    G_CALLBACK (btk_input_dialog_set_mapping_mode),
		    GINT_TO_POINTER (BDK_MODE_DISABLED));

  menuitem = btk_menu_item_new_with_label(_("Screen"));
  btk_menu_shell_append (BTK_MENU_SHELL (mapping_menu), menuitem);
  btk_widget_show (menuitem);
  g_signal_connect (menuitem, "activate",
		    G_CALLBACK (btk_input_dialog_set_mapping_mode),
		    GINT_TO_POINTER (BDK_MODE_SCREEN));

  menuitem = btk_menu_item_new_with_label(_("Window"));
  btk_menu_shell_append (BTK_MENU_SHELL (mapping_menu), menuitem);
  btk_widget_show (menuitem);
  g_signal_connect (menuitem, "activate",
		    G_CALLBACK (btk_input_dialog_set_mapping_mode),
		    GINT_TO_POINTER (BDK_MODE_WINDOW));

  label = btk_label_new_with_mnemonic (_("_Mode:"));
  btk_box_pack_start (BTK_BOX (util_box), label, FALSE, FALSE, 0);
  
  inputd->mode_optionmenu = btk_option_menu_new ();
  btk_label_set_mnemonic_widget (BTK_LABEL (label), inputd->mode_optionmenu);
  btk_box_pack_start (BTK_BOX (util_box), inputd->mode_optionmenu, FALSE, FALSE, 0);
  btk_widget_show (inputd->mode_optionmenu);
  btk_option_menu_set_menu (BTK_OPTION_MENU (inputd->mode_optionmenu), mapping_menu);

  btk_widget_show(label);

  btk_widget_show (util_box);

  /* Notebook */

  notebook = btk_notebook_new ();
  btk_box_pack_start (BTK_BOX (private->main_vbox), notebook, TRUE, TRUE, 0);
  btk_widget_show (notebook);
      
  /*  The axis listbox  */

  label = btk_label_new (_("Axes"));

  inputd->axis_listbox = btk_scrolled_window_new (NULL, NULL);
  btk_container_set_border_width (BTK_CONTAINER (inputd->axis_listbox), 12);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW(inputd->axis_listbox),
				  BTK_POLICY_AUTOMATIC, BTK_POLICY_AUTOMATIC);
      
  btk_widget_set_size_request (inputd->axis_listbox,
			       AXIS_LIST_WIDTH, AXIS_LIST_HEIGHT);
  btk_notebook_append_page (BTK_NOTEBOOK(notebook), 
			    inputd->axis_listbox, label);

  btk_widget_show (inputd->axis_listbox);

  inputd->axis_list = NULL;

  /* Keys listbox */

  label = btk_label_new (_("Keys"));

  inputd->keys_listbox = btk_scrolled_window_new (NULL, NULL);
  btk_container_set_border_width (BTK_CONTAINER (inputd->keys_listbox), 12);
  btk_widget_set_size_request (inputd->keys_listbox,
			       KEYS_LIST_WIDTH, KEYS_LIST_HEIGHT);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (inputd->keys_listbox),
				  BTK_POLICY_AUTOMATIC, BTK_POLICY_AUTOMATIC);
  btk_notebook_append_page (BTK_NOTEBOOK (notebook), 
			    inputd->keys_listbox, label);

  btk_widget_show (inputd->keys_listbox);

  inputd->keys_list = NULL;

  inputd->save_button = btk_button_new_from_stock (BTK_STOCK_SAVE);
  btk_widget_set_can_default (inputd->save_button, TRUE);
  btk_box_pack_start (BTK_BOX (BTK_DIALOG(inputd)->action_area),
		      inputd->save_button, TRUE, TRUE, 0);
  btk_widget_show (inputd->save_button);

  inputd->close_button = btk_button_new_from_stock (BTK_STOCK_CLOSE);
  btk_widget_set_can_default (inputd->close_button, TRUE);
  btk_box_pack_start (BTK_BOX (BTK_DIALOG(inputd)->action_area),
		      inputd->close_button, TRUE, TRUE, 0);

  btk_widget_show (inputd->close_button);
  btk_widget_grab_default (inputd->close_button);

  btk_widget_pop_composite_child ();

  btk_input_dialog_screen_changed (BTK_WIDGET (inputd), NULL);

  _btk_dialog_set_ignore_separator (dialog, TRUE);
}

static void
btk_input_dialog_screen_changed (BtkWidget *widget,
				 BdkScreen *previous_screen)
{
  BtkInputDialog *inputd = BTK_INPUT_DIALOG (widget);
  BtkInputDialogPrivate *private = btk_input_dialog_get_private (inputd);
  
  GList *device_info = NULL;
  BdkDevice *core_pointer = NULL;
  GList *tmp_list;

  if (btk_widget_has_screen (widget))
    {
      BdkDisplay *display;
      
      display = btk_widget_get_display (widget);
      device_info = bdk_display_list_devices (display);
      core_pointer = bdk_display_get_core_pointer (display);
    }

  inputd->current_device = NULL;
  btk_container_foreach (BTK_CONTAINER (private->device_menu),
			 (BtkCallback)btk_widget_destroy, NULL);
  
  if (g_list_length(device_info) <= 1) /* only core device */
    {
      btk_widget_hide (private->main_vbox);
      btk_widget_show (private->no_devices_label);
      btk_widget_set_sensitive(inputd->save_button, FALSE);
    }
  else
    {
      btk_widget_show (private->main_vbox);
      btk_widget_hide (private->no_devices_label);
      btk_widget_set_sensitive(inputd->save_button, TRUE);

      for (tmp_list = device_info; tmp_list; tmp_list = tmp_list->next)
	{
	  BdkDevice *info = tmp_list->data;
	  if (info != core_pointer)
	    {
	      BtkWidget *menuitem;
	      
	      menuitem = btk_menu_item_new_with_label (info->name);
	      
	      btk_menu_shell_append (BTK_MENU_SHELL (private->device_menu),
				     menuitem);
	      btk_widget_show (menuitem);
	      g_signal_connect (menuitem, "activate",
				G_CALLBACK (btk_input_dialog_set_device),
				info);
	    }
	}
      
      btk_input_dialog_set_device (widget, device_info->data);
      btk_option_menu_set_history (BTK_OPTION_MENU (private->device_optionmenu), 0);
    }
}
     
BtkWidget*
btk_input_dialog_new (void)
{
  BtkInputDialog *inputd;

  inputd = g_object_new (BTK_TYPE_INPUT_DIALOG, NULL);

  return BTK_WIDGET (inputd);
}

static void
btk_input_dialog_set_device (BtkWidget *w,
			     gpointer   data)
{
  BdkDevice *device = data;
  BtkInputDialog *inputd = input_dialog_from_widget (w);

  inputd->current_device = device;

  btk_input_dialog_fill_axes (inputd, device);
  btk_input_dialog_fill_keys (inputd, device);

  btk_option_menu_set_history (BTK_OPTION_MENU (inputd->mode_optionmenu),
			       device->mode);
}

static void
btk_input_dialog_set_mapping_mode (BtkWidget *w,
				   gpointer   data)
{
  BtkInputDialog *inputd = input_dialog_from_widget (w);
  BdkDevice *info = inputd->current_device;
  BdkInputMode old_mode;
  BdkInputMode mode = GPOINTER_TO_INT (data);

  if (!info)
    return;
  
  old_mode = info->mode;

  if (mode != old_mode)
    {
      if (bdk_device_set_mode (info, mode))
	{
	  if (mode == BDK_MODE_DISABLED)
	    g_signal_emit (inputd,
			   input_dialog_signals[DISABLE_DEVICE],
			   0,
			   info);
	  else
	    g_signal_emit (inputd,
			   input_dialog_signals[ENABLE_DEVICE],
			   0,
			   info);
	}
      else
	btk_option_menu_set_history (BTK_OPTION_MENU (inputd->mode_optionmenu),
				     old_mode);

      /* FIXME: error dialog ? */
    }
}

static void
btk_input_dialog_set_axis (BtkWidget *w,
			   gpointer   data)
{
  BdkAxisUse use = GPOINTER_TO_INT(data) & 0xFFFF;
  BdkAxisUse old_use;
  BdkAxisUse *new_axes;
  BtkInputDialog *inputd = input_dialog_from_widget (w);
  BdkDevice *info = inputd->current_device;

  gint axis = (GPOINTER_TO_INT(data) >> 16) - 1;
  gint old_axis;
  int i;

  if (!info)
    return;

  new_axes = g_new (BdkAxisUse, info->num_axes);
  old_axis = -1;
  for (i=0;i<info->num_axes;i++)
    {
      new_axes[i] = info->axes[i].use;
      if (info->axes[i].use == use)
	old_axis = i;
    }

  if (axis != -1)
    old_use = info->axes[axis].use;
  else
    old_use = BDK_AXIS_IGNORE;

  if (axis == old_axis) {
    g_free (new_axes);
    return;
  }

  /* we must always have an x and a y axis */
  if ((axis == -1 && (use == BDK_AXIS_X || use == BDK_AXIS_Y)) ||
      (old_axis == -1 && (old_use == BDK_AXIS_X || old_use == BDK_AXIS_Y)))
    {
      btk_option_menu_set_history (
	        BTK_OPTION_MENU (inputd->axis_items[use]),
		old_axis + 1);
    }
  else
    {
      if (axis != -1)
	bdk_device_set_axis_use (info, axis, use);

      if (old_axis != -1)
	bdk_device_set_axis_use (info, old_axis, old_use);

      if (old_use != BDK_AXIS_IGNORE)
	{
	  btk_option_menu_set_history (
		BTK_OPTION_MENU (inputd->axis_items[old_use]),
		old_axis + 1);
	}
    }

  g_free (new_axes);
}

static void
btk_input_dialog_fill_axes(BtkInputDialog *inputd, BdkDevice *info)
{
  static const char *const axis_use_strings[BDK_AXIS_LAST] =
  {
    "",
    N_("_X:"),
    N_("_Y:"),
    N_("_Pressure:"),
    N_("X _tilt:"),
    N_("Y t_ilt:"),
    N_("_Wheel:")
  };

  int i,j;
  BtkWidget *menu;
  BtkWidget *option_menu;
  BtkWidget *label;
  BtkWidget *viewport;
  BtkWidget *old_child;

  /* remove all the old items */
  if (inputd->axis_list)
    {
      btk_widget_hide (inputd->axis_list);	/* suppress resizes (or get warnings) */
      btk_widget_destroy (inputd->axis_list);
    }
  inputd->axis_list = btk_table_new (BDK_AXIS_LAST, 2, 0);
  btk_table_set_row_spacings (BTK_TABLE (inputd->axis_list), 6);
  btk_table_set_col_spacings (BTK_TABLE (inputd->axis_list), 12);
  
  viewport = btk_viewport_new (NULL, NULL);
  old_child = btk_bin_get_child (BTK_BIN (inputd->axis_listbox));
  if (old_child != NULL)
    btk_widget_destroy (old_child);
  btk_container_add (BTK_CONTAINER (inputd->axis_listbox), viewport);
  btk_viewport_set_shadow_type (BTK_VIEWPORT (viewport), BTK_SHADOW_NONE);
  btk_widget_show (viewport);
  btk_container_add (BTK_CONTAINER (viewport), inputd->axis_list);
  btk_widget_show (inputd->axis_list);

  btk_widget_realize (inputd->axis_list);
  bdk_window_set_background (inputd->axis_list->window,
			     &inputd->axis_list->style->base[BTK_STATE_NORMAL]);

  for (i=BDK_AXIS_X;i<BDK_AXIS_LAST;i++)
    {
      /* create the label */

      label = btk_label_new_with_mnemonic (_(axis_use_strings[i]));
      btk_misc_set_alignment (BTK_MISC (label), 0, 0.5);
      btk_table_attach (BTK_TABLE (inputd->axis_list), label, 0, 1, i, i+1, 
                        BTK_FILL, 0, 2, 2);

      /* and the use option menu */
      menu = btk_menu_new();

      for (j = -1; j < info->num_axes; j++)
	{
	  char buffer[16];
	  BtkWidget *menu_item;

	  if (j == -1)
	    menu_item = btk_menu_item_new_with_label (_("none"));
	  else
	    {
	      g_snprintf (buffer, sizeof (buffer), "%d", j+1);
	      menu_item = btk_menu_item_new_with_label (buffer);
	    }
	  g_signal_connect (menu_item, "activate",
			    G_CALLBACK (btk_input_dialog_set_axis),
			    GINT_TO_POINTER (0x10000 * (j + 1) + i));
	  btk_widget_show (menu_item);
	  btk_menu_shell_append (BTK_MENU_SHELL (menu), menu_item);
	}

      inputd->axis_items[i] = option_menu = btk_option_menu_new ();
      btk_label_set_mnemonic_widget (BTK_LABEL (label), option_menu);
      btk_table_attach (BTK_TABLE (inputd->axis_list), option_menu, 
			1, 2, i, i+1, BTK_EXPAND | BTK_FILL, 0, 2, 2);

      btk_widget_show (option_menu);
      btk_option_menu_set_menu (BTK_OPTION_MENU (option_menu), menu);
      for (j = 0; j < info->num_axes; j++)
	if (info->axes[j].use == (BdkAxisUse) i)
	  {
	    btk_option_menu_set_history (BTK_OPTION_MENU (option_menu), j+1);
	    break;
	  }

      btk_widget_show (label);
    }
}

static void 
btk_input_dialog_clear_key (BtkWidget *widget, BtkInputKeyInfo *key)
{
  if (!key->inputd->current_device)
    return;
  
  btk_entry_set_text (BTK_ENTRY(key->entry), _("(disabled)"));
  bdk_device_set_key (key->inputd->current_device, key->index, 0, 0);
}

static void 
btk_input_dialog_set_key (BtkInputKeyInfo *key,
			  guint keyval, BdkModifierType modifiers)
{
  GString *str;
  gchar chars[2];

  if (keyval)
    {
      str = g_string_new (NULL);
      
      if (modifiers & BDK_SHIFT_MASK)
	g_string_append (str, "Shift+");
      if (modifiers & BDK_CONTROL_MASK)
	g_string_append (str, "Ctrl+");
      if (modifiers & BDK_MOD1_MASK)
	g_string_append (str, "Alt+");
      
      if ((keyval >= 0x20) && (keyval <= 0xFF))
	{
	  chars[0] = keyval;
	  chars[1] = 0;
	  g_string_append (str, chars);
	}
      else
	g_string_append (str, _("(unknown)"));
      btk_entry_set_text (BTK_ENTRY(key->entry), str->str);

      g_string_free (str, TRUE);
    }
  else
    {
      btk_entry_set_text (BTK_ENTRY(key->entry), _("(disabled)"));
    }
}

static gboolean
btk_input_dialog_key_press (BtkWidget *widget, 
			    BdkEventKey *event,
			    BtkInputKeyInfo *key)
{
  if (!key->inputd->current_device)
    return FALSE;
  
  btk_input_dialog_set_key (key, event->keyval, event->state & 0xFF);
  bdk_device_set_key (key->inputd->current_device, key->index, 
		      event->keyval, event->state & 0xFF);

  g_signal_stop_emission_by_name (widget, "key-press-event");
  
  return TRUE;
}

static void 
btk_input_dialog_destroy_key (BtkWidget *widget, BtkInputKeyInfo *key)
{
  g_free (key);
}

static void
btk_input_dialog_fill_keys(BtkInputDialog *inputd, BdkDevice *info)
{
  int i;
  BtkWidget *label;
  BtkWidget *button;
  BtkWidget *hbox;
  BtkWidget *viewport;
  BtkWidget *old_child;

  char buffer[32];
  
  /* remove all the old items */
  if (inputd->keys_list)
    {
      btk_widget_hide (inputd->keys_list);	/* suppress resizes (or get warnings) */
      btk_widget_destroy (inputd->keys_list);
    }

  inputd->keys_list = btk_table_new (info->num_keys, 2, FALSE);
  btk_table_set_row_spacings (BTK_TABLE (inputd->keys_list), 6);
  btk_table_set_col_spacings (BTK_TABLE (inputd->keys_list), 12);
  
  viewport = btk_viewport_new (NULL, NULL);
  old_child = btk_bin_get_child (BTK_BIN (inputd->keys_listbox));
  if (old_child != NULL)
    btk_widget_destroy (old_child);
  btk_container_add (BTK_CONTAINER (inputd->keys_listbox), viewport);
  btk_viewport_set_shadow_type (BTK_VIEWPORT (viewport), BTK_SHADOW_NONE);
  btk_widget_show (viewport);
  btk_container_add (BTK_CONTAINER (viewport), inputd->keys_list);
  btk_widget_show (inputd->keys_list);

  btk_widget_realize (inputd->keys_list);
  bdk_window_set_background (inputd->keys_list->window,
			     &inputd->keys_list->style->base[BTK_STATE_NORMAL]);

  for (i=0;i<info->num_keys;i++)
    {
      BtkInputKeyInfo *key = g_new (BtkInputKeyInfo, 1);
      key->index = i;
      key->inputd = inputd;

      /* create the label */

      g_snprintf (buffer, sizeof (buffer), "_%d:", i+1);
      label = btk_label_new_with_mnemonic (buffer);
      btk_table_attach (BTK_TABLE (inputd->keys_list), label, 0, 1, i, i+1, 
			BTK_FILL, 0, 2, 2);
      btk_widget_show (label);

      /* the entry */

      hbox = btk_hbox_new (FALSE, 6);
      btk_table_attach (BTK_TABLE (inputd->keys_list), hbox, 1, 2, i, i+1, 
                        BTK_EXPAND | BTK_FILL, 0, 2, 2);
      btk_widget_show (hbox);

      key->entry = btk_entry_new ();
      btk_label_set_mnemonic_widget (BTK_LABEL (label), key->entry);
      btk_box_pack_start (BTK_BOX (hbox), key->entry, TRUE, TRUE, 0);
      btk_widget_show (key->entry);

      g_signal_connect (key->entry, "key-press-event",
			G_CALLBACK (btk_input_dialog_key_press), key);
      g_signal_connect (key->entry, "destroy",
			G_CALLBACK (btk_input_dialog_destroy_key), key);
      
      /* and clear button */

      button = btk_button_new_with_mnemonic (_("Cl_ear"));
      btk_box_pack_start (BTK_BOX (hbox), button, FALSE, TRUE, 0);
      btk_widget_show (button);

      g_signal_connect (button, "clicked",
			G_CALLBACK (btk_input_dialog_clear_key), key);

      btk_input_dialog_set_key (key, info->keys[i].keyval,
				info->keys[i].modifiers);
    }
}

#define __BTK_INPUTDIALOG_C__
#include "btkaliasdef.c"
