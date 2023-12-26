
#define BTK_ENABLE_BROKEN
#include "config.h"
#include <btk/btk.h>

/* for all the BtkItem:: and BtkTreeItem:: signals */
static void cb_itemsignal( BtkWidget *item,
                           bchar     *signame )
{
  bchar *name;
  BtkLabel *label;

  /* It's a Bin, so it has one child, which we know to be a
     label, so get that */
  label = BTK_LABEL (BTK_BIN (item)->child);
  /* Get the text of the label */
  btk_label_get (label, &name);
  /* Get the level of the tree which the item is in */
  g_print ("%s called for item %s->%p, level %d\n", signame, name,
	   item, BTK_TREE (item->parent)->level);
}

/* Note that this is never called */
static void cb_unselect_child( BtkWidget *root_tree,
                               BtkWidget *child,
                               BtkWidget *subtree )
{
  g_print ("unselect_child called for root tree %p, subtree %p, child %p\n",
	   root_tree, subtree, child);
}

/* Note that this is called every time the user clicks on an item,
   whether it is already selected or not. */
static void cb_select_child (BtkWidget *root_tree, BtkWidget *child,
			     BtkWidget *subtree)
{
  g_print ("select_child called for root tree %p, subtree %p, child %p\n",
	   root_tree, subtree, child);
}

static void cb_selection_changed( BtkWidget *tree )
{
  GList *i;
  
  g_print ("selection_change called for tree %p\n", tree);
  g_print ("selected objects are:\n");

  i = BTK_TREE_SELECTION_OLD (tree);
  while (i) {
    bchar *name;
    BtkLabel *label;
    BtkWidget *item;

    /* Get a BtkWidget pointer from the list node */
    item = BTK_WIDGET (i->data);
    label = BTK_LABEL (BTK_BIN (item)->child);
    btk_label_get (label, &name);
    g_print ("\t%s on level %d\n", name, BTK_TREE
	     (item->parent)->level);
    i = i->next;
  }
}

int main( int   argc,
          char *argv[] )
{
  BtkWidget *window, *scrolled_win, *tree;
  static bchar *itemnames[] = {"Foo", "Bar", "Baz", "Quux",
			       "Maurice"};
  bint i;

  btk_init (&argc, &argv);

  /* a generic toplevel window */
  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  g_signal_connect (B_OBJECT (window), "delete_event",
                    G_CALLBACK (btk_main_quit), NULL);
  btk_container_set_border_width (BTK_CONTAINER (window), 5);

  /* A generic scrolled window */
  scrolled_win = btk_scrolled_window_new (NULL, NULL);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (scrolled_win),
				  BTK_POLICY_AUTOMATIC,
				  BTK_POLICY_AUTOMATIC);
  btk_widget_set_size_request (scrolled_win, 150, 200);
  btk_container_add (BTK_CONTAINER (window), scrolled_win);
  btk_widget_show (scrolled_win);
  
  /* Create the root tree */
  tree = btk_tree_new ();
  g_print ("root tree is %p\n", tree);
  /* connect all BtkTree:: signals */
  g_signal_connect (B_OBJECT (tree), "select_child",
                    G_CALLBACK (cb_select_child), tree);
  g_signal_connect (B_OBJECT (tree), "unselect_child",
                    G_CALLBACK (cb_unselect_child), tree);
  g_signal_connect (B_OBJECT(tree), "selection_changed",
                    G_CALLBACK(cb_selection_changed), tree);
  /* Add it to the scrolled window */
  btk_scrolled_window_add_with_viewport (BTK_SCROLLED_WINDOW (scrolled_win),
                                         tree);
  /* Set the selection mode */
  btk_tree_set_selection_mode (BTK_TREE (tree),
			       BTK_SELECTION_MULTIPLE);
  /* Show it */
  btk_widget_show (tree);

  for (i = 0; i < 5; i++){
    BtkWidget *subtree, *item;
    bint j;

    /* Create a tree item */
    item = btk_tree_item_new_with_label (itemnames[i]);
    /* Connect all BtkItem:: and BtkTreeItem:: signals */
    g_signal_connect (B_OBJECT (item), "select",
                      G_CALLBACK (cb_itemsignal), "select");
    g_signal_connect (B_OBJECT (item), "deselect",
                      G_CALLBACK (cb_itemsignal), "deselect");
    g_signal_connect (B_OBJECT (item), "toggle",
                      G_CALLBACK (cb_itemsignal), "toggle");
    g_signal_connect (B_OBJECT (item), "expand",
                      G_CALLBACK (cb_itemsignal), "expand");
    g_signal_connect (B_OBJECT (item), "collapse",
                      G_CALLBACK (cb_itemsignal), "collapse");
    /* Add it to the parent tree */
    btk_tree_append (BTK_TREE (tree), item);
    /* Show it - this can be done at any time */
    btk_widget_show (item);
    /* Create this item's subtree */
    subtree = btk_tree_new ();
    g_print ("-> item %s->%p, subtree %p\n", itemnames[i], item,
	     subtree);

    /* This is still necessary if you want these signals to be called
       for the subtree's children.  Note that selection_change will be 
       signalled for the root tree regardless. */
    g_signal_connect (B_OBJECT (subtree), "select_child",
			G_CALLBACK (cb_select_child), subtree);
    g_signal_connect (B_OBJECT (subtree), "unselect_child",
			G_CALLBACK (cb_unselect_child), subtree);
    /* This has absolutely no effect, because it is completely ignored 
       in subtrees */
    btk_tree_set_selection_mode (BTK_TREE (subtree),
				 BTK_SELECTION_SINGLE);
    /* Neither does this, but for a rather different reason - the
       view_mode and view_line values of a tree are propagated to
       subtrees when they are mapped.  So, setting it later on would
       actually have a (somewhat unpredictable) effect */
    btk_tree_set_view_mode (BTK_TREE (subtree), BTK_TREE_VIEW_ITEM);
    /* Set this item's subtree - note that you cannot do this until
       AFTER the item has been added to its parent tree! */
    btk_tree_item_set_subtree (BTK_TREE_ITEM (item), subtree);

    for (j = 0; j < 5; j++){
      BtkWidget *subitem;

      /* Create a subtree item, in much the same way */
      subitem = btk_tree_item_new_with_label (itemnames[j]);
      /* Connect all BtkItem:: and BtkTreeItem:: signals */
      g_signal_connect (B_OBJECT (subitem), "select",
			  G_CALLBACK (cb_itemsignal), "select");
      g_signal_connect (B_OBJECT (subitem), "deselect",
			  G_CALLBACK (cb_itemsignal), "deselect");
      g_signal_connect (B_OBJECT (subitem), "toggle",
			  G_CALLBACK (cb_itemsignal), "toggle");
      g_signal_connect (B_OBJECT (subitem), "expand",
			  G_CALLBACK (cb_itemsignal), "expand");
      g_signal_connect (B_OBJECT (subitem), "collapse",
			  G_CALLBACK (cb_itemsignal), "collapse");
      g_print ("-> -> item %s->%p\n", itemnames[j], subitem);
      /* Add it to its parent tree */
      btk_tree_append (BTK_TREE (subtree), subitem);
      /* Show it */
      btk_widget_show (subitem);
    }
  }

  /* Show the window and loop endlessly */
  btk_widget_show (window);

  btk_main();

  return 0;
}
