/* Size Groups
 *
 * BtkSizeGroup provides a mechanism for grouping a number of
 * widgets together so they all request the same amount of space.
 * This is typically useful when you want a column of widgets to 
 * have the same size, but you can't use a BtkTable widget.
 * 
 * Note that size groups only affect the amount of space requested,
 * not the size that the widgets finally receive. If you want the
 * widgets in a BtkSizeGroup to actually be the same size, you need
 * to pack them in such a way that they get the size they request
 * and not more. For example, if you are packing your widgets
 * into a table, you would not include the BTK_FILL flag.
 */

#include <btk/btk.h>

static BtkWidget *window = NULL;

/* Convenience function to create a combo box holding a number of strings
 */
BtkWidget *
create_combo_box (const char **strings)
{
  BtkWidget *combo_box;
  const char **str;

  combo_box = btk_combo_box_text_new ();
  
  for (str = strings; *str; str++)
    btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (combo_box), *str);

  btk_combo_box_set_active (BTK_COMBO_BOX (combo_box), 0);

  return combo_box;
}

static void
add_row (BtkTable     *table,
	 int           row,
	 BtkSizeGroup *size_group,
	 const char   *label_text,
	 const char  **options)
{
  BtkWidget *combo_box;
  BtkWidget *label;

  label = btk_label_new_with_mnemonic (label_text);
  btk_misc_set_alignment (BTK_MISC (label), 0, 1);
  btk_table_attach (BTK_TABLE (table), label,
		    0, 1,                  row, row + 1,
		    BTK_EXPAND | BTK_FILL, 0,
		    0,                     0);
  
  combo_box = create_combo_box (options);
  btk_label_set_mnemonic_widget (BTK_LABEL (label), combo_box);
  btk_size_group_add_widget (size_group, combo_box);
  btk_table_attach (BTK_TABLE (table), combo_box,
		    1, 2,                  row, row + 1,
		    0,                     0,
		    0,                     0);
}

static void
toggle_grouping (BtkToggleButton *check_button,
		 BtkSizeGroup    *size_group)
{
  BtkSizeGroupMode new_mode;

  /* BTK_SIZE_GROUP_NONE is not generally useful, but is useful
   * here to show the effect of BTK_SIZE_GROUP_HORIZONTAL by
   * contrast.
   */
  if (btk_toggle_button_get_active (check_button))
    new_mode = BTK_SIZE_GROUP_HORIZONTAL;
  else
    new_mode = BTK_SIZE_GROUP_NONE;
  
  btk_size_group_set_mode (size_group, new_mode);
}

BtkWidget *
do_sizegroup (BtkWidget *do_widget)
{
  BtkWidget *table;
  BtkWidget *frame;
  BtkWidget *vbox;
  BtkWidget *check_button;
  BtkSizeGroup *size_group;

  static const char *color_options[] = {
    "Red", "Green", "Blue", NULL
  };
  
  static const char *dash_options[] = {
    "Solid", "Dashed", "Dotted", NULL
  };
  
  static const char *end_options[] = {
    "Square", "Round", "Arrow", NULL
  };
  
  if (!window)
    {
      window = btk_dialog_new_with_buttons ("BtkSizeGroup",
					    BTK_WINDOW (do_widget),
					    0,
					    BTK_STOCK_CLOSE,
					    BTK_RESPONSE_NONE,
					    NULL);
      btk_window_set_resizable (BTK_WINDOW (window), FALSE);
      
      g_signal_connect (window, "response",
			G_CALLBACK (btk_widget_destroy), NULL);
      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed), &window);

      vbox = btk_vbox_new (FALSE, 5);
      btk_box_pack_start (BTK_BOX (btk_dialog_get_content_area (BTK_DIALOG (window))), vbox, TRUE, TRUE, 0);
      btk_container_set_border_width (BTK_CONTAINER (vbox), 5);

      size_group = btk_size_group_new (BTK_SIZE_GROUP_HORIZONTAL);
      
      /* Create one frame holding color options
       */
      frame = btk_frame_new ("Color Options");
      btk_box_pack_start (BTK_BOX (vbox), frame, TRUE, TRUE, 0);

      table = btk_table_new (2, 2, FALSE);
      btk_container_set_border_width (BTK_CONTAINER (table), 5);
      btk_table_set_row_spacings (BTK_TABLE (table), 5);
      btk_table_set_col_spacings (BTK_TABLE (table), 10);
      btk_container_add (BTK_CONTAINER (frame), table);

      add_row (BTK_TABLE (table), 0, size_group, "_Foreground", color_options);
      add_row (BTK_TABLE (table), 1, size_group, "_Background", color_options);

      /* And another frame holding line style options
       */
      frame = btk_frame_new ("Line Options");
      btk_box_pack_start (BTK_BOX (vbox), frame, FALSE, FALSE, 0);

      table = btk_table_new (2, 2, FALSE);
      btk_container_set_border_width (BTK_CONTAINER (table), 5);
      btk_table_set_row_spacings (BTK_TABLE (table), 5);
      btk_table_set_col_spacings (BTK_TABLE (table), 10);
      btk_container_add (BTK_CONTAINER (frame), table);

      add_row (BTK_TABLE (table), 0, size_group, "_Dashing", dash_options);
      add_row (BTK_TABLE (table), 1, size_group, "_Line ends", end_options);

      /* And a check button to turn grouping on and off */
      check_button = btk_check_button_new_with_mnemonic ("_Enable grouping");
      btk_box_pack_start (BTK_BOX (vbox), check_button, FALSE, FALSE, 0);
      
      btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (check_button), TRUE);
      g_signal_connect (check_button, "toggled",
			G_CALLBACK (toggle_grouping), size_group);
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    btk_widget_destroy (window);

  return window;
}
