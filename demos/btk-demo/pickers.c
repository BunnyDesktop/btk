/* Pickers 
 *
 * These widgets are mainly intended for use in preference dialogs.
 * They allow to select colors, fonts, files and directories.
 */

#include <btk/btk.h>

BtkWidget *
do_pickers (BtkWidget *do_widget)
{
  static BtkWidget *window = NULL;
  BtkWidget *table, *label, *picker;

  if (!window)
  {
    window = btk_window_new (BTK_WINDOW_TOPLEVEL);
    btk_window_set_screen (BTK_WINDOW (window),
                           btk_widget_get_screen (do_widget));
    btk_window_set_title (BTK_WINDOW (window), "Pickers");
   
    g_signal_connect (window, "destroy",
                      G_CALLBACK (btk_widget_destroyed),
                      &window);
    
    btk_container_set_border_width (BTK_CONTAINER (window), 10);

    table = btk_table_new (4, 2, FALSE);    
    btk_table_set_col_spacing (BTK_TABLE (table), 0, 10);
    btk_table_set_row_spacings (BTK_TABLE (table), 3);
    btk_container_add (BTK_CONTAINER (window), table);

    btk_container_set_border_width (BTK_CONTAINER (table), 10);

    label = btk_label_new ("Color:");
    btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
    picker = btk_color_button_new ();
    btk_table_attach_defaults (BTK_TABLE (table), label, 0, 1, 0, 1);
    btk_table_attach_defaults (BTK_TABLE (table), picker, 1, 2, 0, 1);

    label = btk_label_new ("Font:");
    btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
    picker = btk_font_button_new ();
    btk_table_attach_defaults (BTK_TABLE (table), label, 0, 1, 1, 2);
    btk_table_attach_defaults (BTK_TABLE (table), picker, 1, 2, 1, 2);

    label = btk_label_new ("File:");
    btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
    picker = btk_file_chooser_button_new ("Pick a File", 
                                          BTK_FILE_CHOOSER_ACTION_OPEN);
    btk_table_attach_defaults (BTK_TABLE (table), label, 0, 1, 2, 3);
    btk_table_attach_defaults (BTK_TABLE (table), picker, 1, 2, 2, 3);

    label = btk_label_new ("Folder:");
    btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
    picker = btk_file_chooser_button_new ("Pick a Folder", 
                                          BTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
    btk_table_attach_defaults (BTK_TABLE (table), label, 0, 1, 3, 4);
    btk_table_attach_defaults (BTK_TABLE (table), picker, 1, 2, 3, 4);
  }

  if (!btk_widget_get_visible (window))
    {
      btk_widget_show_all (window);
    }
  else
    {    
      btk_widget_destroy (window);
      window = NULL;
    }

  return window;
}
