/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 2 -*- */

/* BTK+: btkfilechooserbutton.c
 * 
 * Copyright (c) 2004 James M. Cape <jcape@ignore-your.tv>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

#include <string.h>

#include <btk/btk.h>

#include "prop-editor.h"

static gchar *backend = "btk+";
static gboolean rtl = FALSE;
static GOptionEntry entries[] = {
  { "backend", 'b', 0, G_OPTION_ARG_STRING, &backend, "The filesystem backend to use.", "btk+" },
  { "right-to-left", 'r', 0, G_OPTION_ARG_NONE, &rtl, "Force right-to-left layout.", NULL },
  { NULL }
};

static gchar *btk_src_dir = NULL;


static void
win_style_set_cb (BtkWidget *win)
{
  btk_container_set_border_width (BTK_CONTAINER (BTK_DIALOG (win)->vbox), 12);
  btk_box_set_spacing (BTK_BOX (BTK_DIALOG (win)->vbox), 24);
  btk_container_set_border_width (BTK_CONTAINER (BTK_DIALOG (win)->action_area), 0);
  btk_box_set_spacing (BTK_BOX (BTK_DIALOG (win)->action_area), 6);
}

static gboolean
delete_event_cb (BtkWidget *editor,
		 gint       response,
		 gpointer   user_data)
{
  btk_widget_hide (editor);

  return TRUE;
}


static void
properties_button_clicked_cb (BtkWidget *button,
			      GObject   *entry)
{
  BtkWidget *editor;

  editor = g_object_get_data (entry, "properties-dialog");

  if (editor == NULL)
    {
      editor = create_prop_editor (G_OBJECT (entry), G_TYPE_INVALID);
      btk_container_set_border_width (BTK_CONTAINER (editor), 12);
      btk_window_set_transient_for (BTK_WINDOW (editor),
				    BTK_WINDOW (btk_widget_get_toplevel (button)));
      g_signal_connect (editor, "delete-event", G_CALLBACK (delete_event_cb), NULL);
      g_object_set_data (entry, "properties-dialog", editor);
    }

  btk_window_present (BTK_WINDOW (editor));
}


static void
print_selected_path_clicked_cb (BtkWidget *button,
				gpointer   user_data)
{
  gchar *folder, *filename;

  folder = btk_file_chooser_get_current_folder (user_data);
  filename = btk_file_chooser_get_filename (user_data);
  g_message ("Currently Selected:\n\tFolder: `%s'\n\tFilename: `%s'\nDone.\n",
	     folder, filename);
  g_free (folder);
  g_free (filename);
}

static void
add_pwds_parent_as_shortcut_clicked_cb (BtkWidget *button,
					gpointer   user_data)
{
  GError *err = NULL;

  if (!btk_file_chooser_add_shortcut_folder (user_data, btk_src_dir, &err))
    {
      g_message ("Couldn't add `%s' as shortcut folder: %s", btk_src_dir,
		 err->message);
      g_error_free (err);
    }
  else
    {
      g_message ("Added `%s' as shortcut folder.", btk_src_dir);
    }
}

static void
del_pwds_parent_as_shortcut_clicked_cb (BtkWidget *button,
					gpointer   user_data)
{
  GError *err = NULL;

  if (!btk_file_chooser_remove_shortcut_folder (user_data, btk_src_dir, &err))
    {
      g_message ("Couldn't remove `%s' as shortcut folder: %s", btk_src_dir,
		 err->message);
      g_error_free (err);
    }
  else
    {
      g_message ("Removed `%s' as shortcut folder.", btk_src_dir);
    }
}

static void
unselect_all_clicked_cb (BtkWidget *button,
                         gpointer   user_data)
{
  btk_file_chooser_unselect_all (user_data);
}

static void
tests_button_clicked_cb (BtkButton *real_button,
			 gpointer   user_data)
{
  BtkWidget *tests;

  tests = g_object_get_data (user_data, "tests-dialog");

  if (tests == NULL)
    {
      BtkWidget *box, *button;

      tests = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_title (BTK_WINDOW (tests),
			    "Tests - TestFileChooserButton");
      btk_container_set_border_width (BTK_CONTAINER (tests), 12);
      btk_window_set_transient_for (BTK_WINDOW (tests),
				    BTK_WINDOW (btk_widget_get_toplevel (user_data)));

      box = btk_vbox_new (FALSE, 0);
      btk_container_add (BTK_CONTAINER (tests), box);
      btk_widget_show (box);

      button = btk_button_new_with_label ("Print Selected Path");
      g_signal_connect (button, "clicked",
			G_CALLBACK (print_selected_path_clicked_cb), user_data);
      btk_box_pack_start (BTK_BOX (box), button, FALSE, FALSE, 0);
      btk_widget_show (button);

      button = btk_button_new_with_label ("Add $PWD's Parent as Shortcut");
      g_signal_connect (button, "clicked",
			G_CALLBACK (add_pwds_parent_as_shortcut_clicked_cb), user_data);
      btk_box_pack_start (BTK_BOX (box), button, FALSE, FALSE, 0);
      btk_widget_show (button);

      button = btk_button_new_with_label ("Remove $PWD's Parent as Shortcut");
      g_signal_connect (button, "clicked",
			G_CALLBACK (del_pwds_parent_as_shortcut_clicked_cb), user_data);
      btk_box_pack_start (BTK_BOX (box), button, FALSE, FALSE, 0);
      btk_widget_show (button);

      button = btk_button_new_with_label ("Unselect all");
      g_signal_connect (button, "clicked",
			G_CALLBACK (unselect_all_clicked_cb), user_data);
      btk_box_pack_start (BTK_BOX (box), button, FALSE, FALSE, 0);
      btk_widget_show (button);

      g_signal_connect (tests, "delete-event", G_CALLBACK (delete_event_cb), NULL);
      g_object_set_data (user_data, "tests-dialog", tests);
    }

  btk_window_present (BTK_WINDOW (tests));
}

static void
chooser_current_folder_changed_cb (BtkFileChooser *chooser,
				   gpointer        user_data)
{
  gchar *folder, *filename;

  folder = btk_file_chooser_get_current_folder_uri (chooser);
  filename = btk_file_chooser_get_uri (chooser);
  g_message ("%s::current-folder-changed\n\tFolder: `%s'\n\tFilename: `%s'\nDone.\n",
	     G_OBJECT_TYPE_NAME (chooser), folder, filename);
  g_free (folder);
  g_free (filename);
}

static void
chooser_selection_changed_cb (BtkFileChooser *chooser,
			      gpointer        user_data)
{
  gchar *filename;

  filename = btk_file_chooser_get_uri (chooser);
  g_message ("%s::selection-changed\n\tSelection:`%s'\nDone.\n",
	     G_OBJECT_TYPE_NAME (chooser), filename);
  g_free (filename);
}

static void
chooser_file_activated_cb (BtkFileChooser *chooser,
			   gpointer        user_data)
{
  gchar *folder, *filename;

  folder = btk_file_chooser_get_current_folder_uri (chooser);
  filename = btk_file_chooser_get_uri (chooser);
  g_message ("%s::file-activated\n\tFolder: `%s'\n\tFilename: `%s'\nDone.\n",
	     G_OBJECT_TYPE_NAME (chooser), folder, filename);
  g_free (folder);
  g_free (filename);
}

static void
chooser_update_preview_cb (BtkFileChooser *chooser,
			   gpointer        user_data)
{
  gchar *filename;

  filename = btk_file_chooser_get_preview_uri (chooser);
  if (filename != NULL)
    {
      g_message ("%s::update-preview\n\tPreview Filename: `%s'\nDone.\n",
		 G_OBJECT_TYPE_NAME (chooser), filename);
      g_free (filename);
    }
}


int
main (int   argc,
      char *argv[])
{
  BtkWidget *win, *vbox, *frame, *alignment, *group_box;
  BtkWidget *hbox, *label, *chooser, *button;
  BtkSizeGroup *label_group;
  GOptionContext *context;
  gchar *cwd;

  context = g_option_context_new ("- test BtkFileChooserButton widget");
  g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
  g_option_context_add_group (context, btk_get_option_group (TRUE));
  g_option_context_parse (context, &argc, &argv, NULL);
  g_option_context_free (context);

  btk_init (&argc, &argv);

  /* to test rtl layout, use "--right-to-left" */
  if (rtl)
    btk_widget_set_default_direction (BTK_TEXT_DIR_RTL);

  cwd = g_get_current_dir();
  btk_src_dir = g_path_get_dirname (cwd);
  g_free (cwd);

  win = btk_dialog_new_with_buttons ("TestFileChooserButton", NULL, BTK_DIALOG_NO_SEPARATOR,
				     BTK_STOCK_QUIT, BTK_RESPONSE_CLOSE, NULL);
  g_signal_connect (win, "style-set", G_CALLBACK (win_style_set_cb), NULL);
  g_signal_connect (win, "response", G_CALLBACK (btk_main_quit), NULL);

  vbox = btk_vbox_new (FALSE, 18);
  btk_container_add (BTK_CONTAINER (BTK_DIALOG (win)->vbox), vbox);

  frame = btk_frame_new ("<b>BtkFileChooserButton</b>");
  btk_frame_set_shadow_type (BTK_FRAME (frame), BTK_SHADOW_NONE);
  btk_label_set_use_markup (BTK_LABEL (btk_frame_get_label_widget (BTK_FRAME (frame))), TRUE);
  btk_box_pack_start (BTK_BOX (vbox), frame, FALSE, FALSE, 0);

  alignment = btk_alignment_new (0.0, 0.0, 1.0, 1.0);
  btk_alignment_set_padding (BTK_ALIGNMENT (alignment), 6, 0, 12, 0);
  btk_container_add (BTK_CONTAINER (frame), alignment);
  
  label_group = btk_size_group_new (BTK_SIZE_GROUP_HORIZONTAL);
  
  group_box = btk_vbox_new (FALSE, 6);
  btk_container_add (BTK_CONTAINER (alignment), group_box);

  /* OPEN */
  hbox = btk_hbox_new (FALSE, 12);
  btk_box_pack_start (BTK_BOX (group_box), hbox, FALSE, FALSE, 0);

  label = btk_label_new_with_mnemonic ("_Open:");
  btk_size_group_add_widget (BTK_SIZE_GROUP (label_group), label);
  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
  btk_box_pack_start (BTK_BOX (hbox), label, FALSE, FALSE, 0);

  chooser = btk_file_chooser_button_new ("Select A File - testfilechooserbutton",
                                         BTK_FILE_CHOOSER_ACTION_OPEN);
  btk_file_chooser_add_shortcut_folder (BTK_FILE_CHOOSER (chooser), btk_src_dir, NULL);
  btk_file_chooser_remove_shortcut_folder (BTK_FILE_CHOOSER (chooser), btk_src_dir, NULL);
  btk_label_set_mnemonic_widget (BTK_LABEL (label), chooser);
  g_signal_connect (chooser, "current-folder-changed",
		    G_CALLBACK (chooser_current_folder_changed_cb), NULL);
  g_signal_connect (chooser, "selection-changed", G_CALLBACK (chooser_selection_changed_cb), NULL);
  g_signal_connect (chooser, "file-activated", G_CALLBACK (chooser_file_activated_cb), NULL);
  g_signal_connect (chooser, "update-preview", G_CALLBACK (chooser_update_preview_cb), NULL);
  btk_container_add (BTK_CONTAINER (hbox), chooser);

  button = btk_button_new_from_stock (BTK_STOCK_PROPERTIES);
  g_signal_connect (button, "clicked", G_CALLBACK (properties_button_clicked_cb), chooser);
  btk_box_pack_start (BTK_BOX (hbox), button, FALSE, FALSE, 0);

  button = btk_button_new_with_label ("Tests");
  g_signal_connect (button, "clicked", G_CALLBACK (tests_button_clicked_cb), chooser);
  btk_box_pack_start (BTK_BOX (hbox), button, FALSE, FALSE, 0);

  /* SELECT_FOLDER */
  hbox = btk_hbox_new (FALSE, 12);
  btk_box_pack_start (BTK_BOX (group_box), hbox, FALSE, FALSE, 0);

  label = btk_label_new_with_mnemonic ("Select _Folder:");
  btk_size_group_add_widget (BTK_SIZE_GROUP (label_group), label);
  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
  btk_box_pack_start (BTK_BOX (hbox), label, FALSE, FALSE, 0);

  chooser = btk_file_chooser_button_new ("Select A Folder - testfilechooserbutton",
                                         BTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
  btk_file_chooser_add_shortcut_folder (BTK_FILE_CHOOSER (chooser), btk_src_dir, NULL);
  btk_file_chooser_remove_shortcut_folder (BTK_FILE_CHOOSER (chooser), btk_src_dir, NULL);
  btk_file_chooser_add_shortcut_folder (BTK_FILE_CHOOSER (chooser), btk_src_dir, NULL);

  btk_label_set_mnemonic_widget (BTK_LABEL (label), chooser);
  g_signal_connect (chooser, "current-folder-changed",
		    G_CALLBACK (chooser_current_folder_changed_cb), NULL);
  g_signal_connect (chooser, "selection-changed", G_CALLBACK (chooser_selection_changed_cb), NULL);
  g_signal_connect (chooser, "file-activated", G_CALLBACK (chooser_file_activated_cb), NULL);
  g_signal_connect (chooser, "update-preview", G_CALLBACK (chooser_update_preview_cb), NULL);
  btk_container_add (BTK_CONTAINER (hbox), chooser);

  button = btk_button_new_from_stock (BTK_STOCK_PROPERTIES);
  g_signal_connect (button, "clicked", G_CALLBACK (properties_button_clicked_cb), chooser);
  btk_box_pack_start (BTK_BOX (hbox), button, FALSE, FALSE, 0);

  button = btk_button_new_with_label ("Tests");
  g_signal_connect (button, "clicked", G_CALLBACK (tests_button_clicked_cb), chooser);
  btk_box_pack_start (BTK_BOX (hbox), button, FALSE, FALSE, 0);

  g_object_unref (label_group);

  btk_widget_show_all (win);
  btk_window_present (BTK_WINDOW (win));

  btk_main ();

  return 0;
}
