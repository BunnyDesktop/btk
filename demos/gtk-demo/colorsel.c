/* Color Selector
 *
 * BtkColorSelection lets the user choose a color. BtkColorSelectionDialog is
 * a prebuilt dialog containing a BtkColorSelection.
 *
 */

#include <btk/btk.h>

static BtkWidget *window = NULL;
static BtkWidget *da;
static BdkColor color;
static BtkWidget *frame;

/* Expose callback for the drawing area
 */
static gboolean
expose_event_callback (BtkWidget      *widget, 
                       BdkEventExpose *event, 
                       gpointer        data)
{
  BdkWindow *window = btk_widget_get_window (widget);
  
  if (window)
    {
      BtkStyle *style;
      bairo_t *cr;

      style = btk_widget_get_style (widget);

      cr = bdk_bairo_create (window);

      bdk_bairo_set_source_color (cr, &style->bg[BTK_STATE_NORMAL]);
      bdk_bairo_rectangle (cr, &event->area);
      bairo_fill (cr);

      bairo_destroy (cr);
    }

  return TRUE;
}

static void
change_color_callback (BtkWidget *button,
		       gpointer	  data)
{
  BtkWidget *dialog;
  BtkColorSelection *colorsel;
  gint response;
  
  dialog = btk_color_selection_dialog_new ("Changing color");

  btk_window_set_transient_for (BTK_WINDOW (dialog), BTK_WINDOW (window));
  
  colorsel = 
    BTK_COLOR_SELECTION (btk_color_selection_dialog_get_color_selection (BTK_COLOR_SELECTION_DIALOG (dialog)));

  btk_color_selection_set_previous_color (colorsel, &color);
  btk_color_selection_set_current_color (colorsel, &color);
  btk_color_selection_set_has_palette (colorsel, TRUE);
  
  response = btk_dialog_run (BTK_DIALOG (dialog));

  if (response == BTK_RESPONSE_OK)
    {
      btk_color_selection_get_current_color (colorsel,
					     &color);
      
      btk_widget_modify_bg (da, BTK_STATE_NORMAL, &color);
    }
  
  btk_widget_destroy (dialog);
}

BtkWidget *
do_colorsel (BtkWidget *do_widget)
{
  BtkWidget *vbox;
  BtkWidget *button;
  BtkWidget *alignment;
  
  if (!window)
    {
      color.red = 0;
      color.blue = 65535;
      color.green = 0;
      
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (do_widget));
      btk_window_set_title (BTK_WINDOW (window), "Color Selection");

      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed), &window);

      btk_container_set_border_width (BTK_CONTAINER (window), 8);

      vbox = btk_vbox_new (FALSE, 8);
      btk_container_set_border_width (BTK_CONTAINER (vbox), 8);
      btk_container_add (BTK_CONTAINER (window), vbox);

      /*
       * Create the color swatch area
       */
      
      
      frame = btk_frame_new (NULL);
      btk_frame_set_shadow_type (BTK_FRAME (frame), BTK_SHADOW_IN);
      btk_box_pack_start (BTK_BOX (vbox), frame, TRUE, TRUE, 0);

      da = btk_drawing_area_new ();

      g_signal_connect (da, "expose_event",
			G_CALLBACK (expose_event_callback), NULL);

      /* set a minimum size */
      btk_widget_set_size_request (da, 200, 200);
      /* set the color */
      btk_widget_modify_bg (da, BTK_STATE_NORMAL, &color);
      
      btk_container_add (BTK_CONTAINER (frame), da);

      alignment = btk_alignment_new (1.0, 0.5, 0.0, 0.0);
      
      button = btk_button_new_with_mnemonic ("_Change the above color");
      btk_container_add (BTK_CONTAINER (alignment), button);
      
      btk_box_pack_start (BTK_BOX (vbox), alignment, FALSE, FALSE, 0);
      
      g_signal_connect (button, "clicked",
			G_CALLBACK (change_color_callback), NULL);
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
