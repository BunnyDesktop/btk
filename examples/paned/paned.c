
#include <stdio.h>
#include <btk/btk.h>
   
/* Create the list of "messages" */
static BtkWidget *create_list( void )
{

    BtkWidget *scrolled_window;
    BtkWidget *tree_view;
    BtkListStore *model;
    BtkTreeIter iter;
    BtkCellRenderer *cell;
    BtkTreeViewColumn *column;

    int i;
   
    /* Create a new scrolled window, with scrollbars only if needed */
    scrolled_window = btk_scrolled_window_new (NULL, NULL);
    btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (scrolled_window),
				    BTK_POLICY_AUTOMATIC, 
				    BTK_POLICY_AUTOMATIC);
   
    model = btk_list_store_new (1, G_TYPE_STRING);
    tree_view = btk_tree_view_new ();
    btk_scrolled_window_add_with_viewport (BTK_SCROLLED_WINDOW (scrolled_window), 
                                           tree_view);
    btk_tree_view_set_model (BTK_TREE_VIEW (tree_view), BTK_TREE_MODEL (model));
    btk_widget_show (tree_view);
   
    /* Add some messages to the window */
    for (i = 0; i < 10; i++) {
        gchar *msg = g_strdup_printf ("Message #%d", i);
        btk_list_store_append (BTK_LIST_STORE (model), &iter);
        btk_list_store_set (BTK_LIST_STORE (model), 
	                    &iter,
                            0, msg,
	                    -1);
	g_free (msg);
    }
   
    cell = btk_cell_renderer_text_new ();

    column = btk_tree_view_column_new_with_attributes ("Messages",
                                                       cell,
                                                       "text", 0,
                                                       NULL);
  
    btk_tree_view_append_column (BTK_TREE_VIEW (tree_view),
	  		         BTK_TREE_VIEW_COLUMN (column));

    return scrolled_window;
}
   
/* Add some text to our text widget - this is a callback that is invoked
when our window is realized. We could also force our window to be
realized with btk_widget_realize, but it would have to be part of
a hierarchy first */

static void insert_text( BtkTextBuffer *buffer )
{
   BtkTextIter iter;
 
   btk_text_buffer_get_iter_at_offset (buffer, &iter, 0);

   btk_text_buffer_insert (buffer, &iter,   
    "From: pathfinder@nasa.gov\n"
    "To: mom@nasa.gov\n"
    "Subject: Made it!\n"
    "\n"
    "We just got in this morning. The weather has been\n"
    "great - clear but cold, and there are lots of fun sights.\n"
    "Sojourner says hi. See you soon.\n"
    " -Path\n", -1);
}
   
/* Create a scrolled text area that displays a "message" */
static BtkWidget *create_text( void )
{
   BtkWidget *scrolled_window;
   BtkWidget *view;
   BtkTextBuffer *buffer;

   view = btk_text_view_new ();
   buffer = btk_text_view_get_buffer (BTK_TEXT_VIEW (view));

   scrolled_window = btk_scrolled_window_new (NULL, NULL);
   btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (scrolled_window),
		   	           BTK_POLICY_AUTOMATIC,
				   BTK_POLICY_AUTOMATIC);

   btk_container_add (BTK_CONTAINER (scrolled_window), view);
   insert_text (buffer);

   btk_widget_show_all (scrolled_window);

   return scrolled_window;
}
   
int main( int   argc,
          char *argv[] )
{
    BtkWidget *window;
    BtkWidget *vpaned;
    BtkWidget *list;
    BtkWidget *text;

    btk_init (&argc, &argv);
   
    window = btk_window_new (BTK_WINDOW_TOPLEVEL);
    btk_window_set_title (BTK_WINDOW (window), "Paned Windows");
    g_signal_connect (G_OBJECT (window), "destroy",
	              G_CALLBACK (btk_main_quit), NULL);
    btk_container_set_border_width (BTK_CONTAINER (window), 10);
    btk_widget_set_size_request (BTK_WIDGET (window), 450, 400);

    /* create a vpaned widget and add it to our toplevel window */
   
    vpaned = btk_vpaned_new ();
    btk_container_add (BTK_CONTAINER (window), vpaned);
    btk_widget_show (vpaned);
   
    /* Now create the contents of the two halves of the window */
   
    list = create_list ();
    btk_paned_add1 (BTK_PANED (vpaned), list);
    btk_widget_show (list);
   
    text = create_text ();
    btk_paned_add2 (BTK_PANED (vpaned), text);
    btk_widget_show (text);
    btk_widget_show (window);

    btk_main ();

    return 0;
}
