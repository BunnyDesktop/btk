
#include <btk/btk.h>

int main( int   argc,
          char *argv[] )
{
  static BtkWidget *window = NULL;
  BtkWidget *hbox;
  BtkWidget *vbox;
  BtkWidget *frame;
  BtkWidget *label;

  /* Initialise BTK */
  btk_init (&argc, &argv);

  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  g_signal_connect (G_OBJECT (window), "destroy",
		    G_CALLBACK (btk_main_quit),
		    NULL);

  btk_window_set_title (BTK_WINDOW (window), "Label");
  vbox = btk_vbox_new (FALSE, 5);
  hbox = btk_hbox_new (FALSE, 5);
  btk_container_add (BTK_CONTAINER (window), hbox);
  btk_box_pack_start (BTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  btk_container_set_border_width (BTK_CONTAINER (window), 5);
  
  frame = btk_frame_new ("Normal Label");
  label = btk_label_new ("This is a Normal label");
  btk_container_add (BTK_CONTAINER (frame), label);
  btk_box_pack_start (BTK_BOX (vbox), frame, FALSE, FALSE, 0);
  
  frame = btk_frame_new ("Multi-line Label");
  label = btk_label_new ("This is a Multi-line label.\nSecond line\n" \
			 "Third line");
  btk_container_add (BTK_CONTAINER (frame), label);
  btk_box_pack_start (BTK_BOX (vbox), frame, FALSE, FALSE, 0);
  
  frame = btk_frame_new ("Left Justified Label");
  label = btk_label_new ("This is a Left-Justified\n" \
			 "Multi-line label.\nThird      line");
  btk_label_set_justify (BTK_LABEL (label), BTK_JUSTIFY_LEFT);
  btk_container_add (BTK_CONTAINER (frame), label);
  btk_box_pack_start (BTK_BOX (vbox), frame, FALSE, FALSE, 0);
  
  frame = btk_frame_new ("Right Justified Label");
  label = btk_label_new ("This is a Right-Justified\nMulti-line label.\n" \
			 "Fourth line, (j/k)");
  btk_label_set_justify (BTK_LABEL (label), BTK_JUSTIFY_RIGHT);
  btk_container_add (BTK_CONTAINER (frame), label);
  btk_box_pack_start (BTK_BOX (vbox), frame, FALSE, FALSE, 0);

  vbox = btk_vbox_new (FALSE, 5);
  btk_box_pack_start (BTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  frame = btk_frame_new ("Line wrapped label");
  label = btk_label_new ("This is an example of a line-wrapped label.  It " \
			 "should not be taking up the entire             " /* big space to test spacing */\
			 "width allocated to it, but automatically " \
			 "wraps the words to fit.  " \
			 "The time has come, for all good men, to come to " \
			 "the aid of their party.  " \
			 "The sixth sheik's six sheep's sick.\n" \
			 "     It supports multiple paragraphs correctly, " \
			 "and  correctly   adds "\
			 "many          extra  spaces. ");
  btk_label_set_line_wrap (BTK_LABEL (label), TRUE);
  btk_container_add (BTK_CONTAINER (frame), label);
  btk_box_pack_start (BTK_BOX (vbox), frame, FALSE, FALSE, 0);
  
  frame = btk_frame_new ("Filled, wrapped label");
  label = btk_label_new ("This is an example of a line-wrapped, filled label.  " \
			 "It should be taking "\
			 "up the entire              width allocated to it.  " \
			 "Here is a sentence to prove "\
			 "my point.  Here is another sentence. "\
			 "Here comes the sun, do de do de do.\n"\
			 "    This is a new paragraph.\n"\
			 "    This is another newer, longer, better " \
			 "paragraph.  It is coming to an end, "\
			 "unfortunately.");
  btk_label_set_justify (BTK_LABEL (label), BTK_JUSTIFY_FILL);
  btk_label_set_line_wrap (BTK_LABEL (label), TRUE);
  btk_container_add (BTK_CONTAINER (frame), label);
  btk_box_pack_start (BTK_BOX (vbox), frame, FALSE, FALSE, 0);
  
  frame = btk_frame_new ("Underlined label");
  label = btk_label_new ("This label is underlined!\n"
			 "This one is underlined in quite a funky fashion");
  btk_label_set_justify (BTK_LABEL (label), BTK_JUSTIFY_LEFT);
  btk_label_set_pattern (BTK_LABEL (label),
			 "_________________________ _ _________ _ ______     __ _______ ___");
  btk_container_add (BTK_CONTAINER (frame), label);
  btk_box_pack_start (BTK_BOX (vbox), frame, FALSE, FALSE, 0);
  
  btk_widget_show_all (window);

  btk_main ();
  
  return 0;
}
