
#define BTK_ENABLE_BROKEN
#include "config.h"
#include <stdio.h>
#include <btk/btk.h>

void text_toggle_editable (BtkWidget *checkbutton,
			   BtkWidget *text)
{
  btk_text_set_editable (BTK_TEXT (text),
			 BTK_TOGGLE_BUTTON (checkbutton)->active);
}

void text_toggle_word_wrap (BtkWidget *checkbutton,
			    BtkWidget *text)
{
  btk_text_set_word_wrap (BTK_TEXT (text),
			  BTK_TOGGLE_BUTTON (checkbutton)->active);
}

void close_application( BtkWidget *widget,
                        gpointer   data )
{
       btk_main_quit ();
}

int main( int argc,
          char *argv[] )
{
  BtkWidget *window;
  BtkWidget *box1;
  BtkWidget *box2;
  BtkWidget *hbox;
  BtkWidget *button;
  BtkWidget *check;
  BtkWidget *separator;
  BtkWidget *table;
  BtkWidget *vscrollbar;
  BtkWidget *text;
  BdkColormap *cmap;
  BdkColor color;
  BdkFont *fixed_font;

  FILE *infile;

  btk_init (&argc, &argv);
 
  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  btk_widget_set_size_request (window, 600, 500);
  btk_window_set_policy (BTK_WINDOW (window), TRUE, TRUE, FALSE);  
  g_signal_connect (B_OBJECT (window), "destroy",
                    G_CALLBACK (close_application),
                    NULL);
  btk_window_set_title (BTK_WINDOW (window), "Text Widget Example");
  btk_container_set_border_width (BTK_CONTAINER (window), 0);
  
  
  box1 = btk_vbox_new (FALSE, 0);
  btk_container_add (BTK_CONTAINER (window), box1);
  btk_widget_show (box1);
  
  
  box2 = btk_vbox_new (FALSE, 10);
  btk_container_set_border_width (BTK_CONTAINER (box2), 10);
  btk_box_pack_start (BTK_BOX (box1), box2, TRUE, TRUE, 0);
  btk_widget_show (box2);
  
  
  table = btk_table_new (2, 2, FALSE);
  btk_table_set_row_spacing (BTK_TABLE (table), 0, 2);
  btk_table_set_col_spacing (BTK_TABLE (table), 0, 2);
  btk_box_pack_start (BTK_BOX (box2), table, TRUE, TRUE, 0);
  btk_widget_show (table);
  
  /* Create the BtkText widget */
  text = btk_text_new (NULL, NULL);
  btk_text_set_editable (BTK_TEXT (text), TRUE);
  btk_table_attach (BTK_TABLE (table), text, 0, 1, 0, 1,
		    BTK_EXPAND | BTK_SHRINK | BTK_FILL,
		    BTK_EXPAND | BTK_SHRINK | BTK_FILL, 0, 0);
  btk_widget_show (text);

  /* Add a vertical scrollbar to the BtkText widget */
  vscrollbar = btk_vscrollbar_new (BTK_TEXT (text)->vadj);
  btk_table_attach (BTK_TABLE (table), vscrollbar, 1, 2, 0, 1,
		    BTK_FILL, BTK_EXPAND | BTK_SHRINK | BTK_FILL, 0, 0);
  btk_widget_show (vscrollbar);

  /* Get the system color map and allocate the color red */
  cmap = bdk_colormap_get_system ();
  color.red = 0xffff;
  color.green = 0;
  color.blue = 0;
  if (!bdk_color_alloc (cmap, &color)) {
    g_error ("couldn't allocate color");
  }

  /* Load a fixed font */
  fixed_font = bdk_font_load ("-misc-fixed-medium-r-*-*-*-140-*-*-*-*-*-*");

  /* Realizing a widget creates a window for it,
   * ready for us to insert some text */
  btk_widget_realize (text);

  /* Freeze the text widget, ready for multiple updates */
  btk_text_freeze (BTK_TEXT (text));
  
  /* Insert some colored text */
  btk_text_insert (BTK_TEXT (text), NULL, &text->style->black, NULL,
		   "Supports ", -1);
  btk_text_insert (BTK_TEXT (text), NULL, &color, NULL,
		   "colored ", -1);
  btk_text_insert (BTK_TEXT (text), NULL, &text->style->black, NULL,
		   "text and different ", -1);
  btk_text_insert (BTK_TEXT (text), fixed_font, &text->style->black, NULL,
		   "fonts\n\n", -1);
  
  /* Load the file text.c into the text window */

  infile = fopen ("text.c", "r");
  
  if (infile) {
    char buffer[1024];
    int nchars;
    
    while (1)
      {
	nchars = fread (buffer, 1, 1024, infile);
	btk_text_insert (BTK_TEXT (text), fixed_font, NULL,
			 NULL, buffer, nchars);
	
	if (nchars < 1024)
	  break;
      }
    
    fclose (infile);
  }

  /* Thaw the text widget, allowing the updates to become visible */  
  btk_text_thaw (BTK_TEXT (text));
  
  hbox = btk_hbutton_box_new ();
  btk_box_pack_start (BTK_BOX (box2), hbox, FALSE, FALSE, 0);
  btk_widget_show (hbox);

  check = btk_check_button_new_with_label ("Editable");
  btk_box_pack_start (BTK_BOX (hbox), check, FALSE, FALSE, 0);
  g_signal_connect (B_OBJECT (check), "toggled",
                    G_CALLBACK (text_toggle_editable), text);
  btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (check), TRUE);
  btk_widget_show (check);
  check = btk_check_button_new_with_label ("Wrap Words");
  btk_box_pack_start (BTK_BOX (hbox), check, FALSE, TRUE, 0);
  g_signal_connect (B_OBJECT (check), "toggled",
                    G_CALLBACK (text_toggle_word_wrap), text);
  btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (check), FALSE);
  btk_widget_show (check);

  separator = btk_hseparator_new ();
  btk_box_pack_start (BTK_BOX (box1), separator, FALSE, TRUE, 0);
  btk_widget_show (separator);

  box2 = btk_vbox_new (FALSE, 10);
  btk_container_set_border_width (BTK_CONTAINER (box2), 10);
  btk_box_pack_start (BTK_BOX (box1), box2, FALSE, TRUE, 0);
  btk_widget_show (box2);
  
  button = btk_button_new_with_label ("close");
  g_signal_connect (B_OBJECT (button), "clicked",
	            G_CALLBACK (close_application),
	            NULL);
  btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);
  btk_widget_set_can_default (button, TRUE);
  btk_widget_grab_default (button);
  btk_widget_show (button);

  btk_widget_show (window);

  btk_main ();
  
  return 0;       
}
