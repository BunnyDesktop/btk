/* testentrycompletion.c
 * Copyright (C) 2004  Red Hat, Inc.
 * Author: Matthias Clasen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <btk/btk.h>

#include "prop-editor.h"

/* Don't copy this bad example; inline RGB data is always a better
 * idea than inline XPMs.
 */
static char  *book_closed_xpm[] = {
"16 16 6 1",
"       c None s None",
".      c black",
"X      c red",
"o      c yellow",
"O      c #808080",
"#      c white",
"                ",
"       ..       ",
"     ..XX.      ",
"   ..XXXXX.     ",
" ..XXXXXXXX.    ",
".ooXXXXXXXXX.   ",
"..ooXXXXXXXXX.  ",
".X.ooXXXXXXXXX. ",
".XX.ooXXXXXX..  ",
" .XX.ooXXX..#O  ",
"  .XX.oo..##OO. ",
"   .XX..##OO..  ",
"    .X.#OO..    ",
"     ..O..      ",
"      ..        ",
"                "
};

static BtkWidget *window = NULL;


/* Creates a tree model containing the completions */
BtkTreeModel *
create_simple_completion_model (void)
{
  BtkListStore *store;
  BtkTreeIter iter;
  
  store = btk_list_store_new (1, B_TYPE_STRING);

  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, "BUNNY", -1);
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, "gnominious", -1);
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, "Gnomonic projection", -1);

  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, "total", -1);
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, "totally", -1);
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, "toto", -1);
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, "tottery", -1);
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, "totterer", -1);
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, "Totten trust", -1);
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, "totipotent", -1);
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, "totipotency", -1);
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, "totemism", -1);
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, "totem pole", -1);
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, "Totara", -1);
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, "totalizer", -1);
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, "totalizator", -1);
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, "totalitarianism", -1);
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, "total parenteral nutrition", -1);
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, "total hysterectomy", -1);
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, "total eclipse", -1);
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, "Totipresence", -1);
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, "Totipalmi", -1);
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, "zombie", -1);
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, "a\303\246x", -1);
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, "a\303\246y", -1);
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, "a\303\246z", -1);
 
  return BTK_TREE_MODEL (store);
}

/* Creates a tree model containing the completions */
BtkTreeModel *
create_completion_model (void)
{
  BtkListStore *store;
  BtkTreeIter iter;
  BdkPixbuf *pixbuf;

  pixbuf = bdk_pixbuf_new_from_xpm_data ((const char **)book_closed_xpm);

  store = btk_list_store_new (2, BDK_TYPE_PIXBUF, B_TYPE_STRING);

  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, pixbuf, 1, "ambient", -1);
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, pixbuf, 1, "ambidextrously", -1);
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, pixbuf, 1, "ambidexter", -1);
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, pixbuf, 1, "ambiguity", -1);
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, pixbuf, 1, "American Party", -1);
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, pixbuf, 1, "American mountain ash", -1);
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, pixbuf, 1, "amelioration", -1);
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, pixbuf, 1, "Amelia Earhart", -1);
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, pixbuf, 1, "Totten trust", -1);
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, pixbuf, 1, "Laminated arch", -1);
 
  return BTK_TREE_MODEL (store);
}

static gboolean
match_func (BtkEntryCompletion *completion,
	    const gchar        *key,
	    BtkTreeIter        *iter,
	    gpointer            user_data)
{
  gchar *item = NULL;
  BtkTreeModel *model;

  gboolean ret = FALSE;

  model = btk_entry_completion_get_model (completion);

  btk_tree_model_get (model, iter, 1, &item, -1);

  if (item != NULL)
    {
      g_print ("compare %s %s\n", key, item);
      if (strncmp (key, item, strlen (key)) == 0)
	ret = TRUE;

      g_free (item);
    }

  return ret;
}

static void
activated_cb (BtkEntryCompletion *completion, 
	      gint                index,
	      gpointer            user_data)
{
  g_print ("action activated: %d\n", index);
}

static gint timer_count = 0;

static gchar *dynamic_completions[] = {
  "BUNNY",
  "gnominious",
  "Gnomonic projection",
  "total",
  "totally",
  "toto",
  "tottery",
  "totterer",
  "Totten trust",
  "totipotent",
  "totipotency",
  "totemism",
  "totem pole",
  "Totara",
  "totalizer",
  "totalizator",
  "totalitarianism",
  "total parenteral nutrition",
  "total hysterectomy",
  "total eclipse",
  "Totipresence",
  "Totipalmi",
  "zombie"
};

static gint
animation_timer (BtkEntryCompletion *completion)
{
  BtkTreeIter iter;
  gint n_completions = G_N_ELEMENTS (dynamic_completions);
  gint n;
  static BtkListStore *old_store = NULL;
  BtkListStore *store = BTK_LIST_STORE (btk_entry_completion_get_model (completion));

  if (timer_count % 10 == 0)
    {
      if (!old_store)
	{
	  g_print ("removing model!\n");

	  old_store = g_object_ref (btk_entry_completion_get_model (completion));
	  btk_entry_completion_set_model (completion, NULL);
	}
      else
	{
	  g_print ("readding model!\n");
	  
	  btk_entry_completion_set_model (completion, BTK_TREE_MODEL (old_store));
	  g_object_unref (old_store);
	  old_store = NULL;
	}

      timer_count ++;
      return TRUE;
    }

  if (!old_store)
    {
      if ((timer_count / n_completions) % 2 == 0)
	{
	  n = timer_count % n_completions;
	  btk_list_store_append (store, &iter);
	  btk_list_store_set (store, &iter, 0, dynamic_completions[n], -1);
	  
	}
      else
	{
	  if (btk_tree_model_get_iter_first (BTK_TREE_MODEL (store), &iter))
	    btk_list_store_remove (store, &iter);
	}
    }
  
  timer_count++;
  return TRUE;
}

gboolean 
match_selected_cb (BtkEntryCompletion *completion,
		   BtkTreeModel       *model,
		   BtkTreeIter        *iter)
{
  gchar *str;
  BtkWidget *entry;

  entry = btk_entry_completion_get_entry (completion);
  btk_tree_model_get (BTK_TREE_MODEL (model), iter, 1, &str, -1);
  btk_entry_set_text (BTK_ENTRY (entry), str);
  btk_editable_set_position (BTK_EDITABLE (entry), -1);
  g_free (str);

  return TRUE;
}

static void
new_prop_editor (BObject *object)
{
	btk_widget_show (create_prop_editor (object, B_OBJECT_TYPE (object)));
}

static void
add_with_prop_edit_button (BtkWidget *vbox, BtkWidget *entry, BtkEntryCompletion *completion)
{
	BtkWidget *hbox, *button;

	hbox = btk_hbox_new (FALSE, 12);
	btk_box_pack_start (BTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	btk_box_pack_start (BTK_BOX (hbox), entry, TRUE, TRUE, 0);

	button = btk_button_new_with_label ("Properties");
	g_signal_connect_swapped (button, "clicked", G_CALLBACK (new_prop_editor), completion);
	btk_box_pack_start (BTK_BOX (hbox), button, FALSE, FALSE, 0);
}

int 
main (int argc, char *argv[])
{
  BtkWidget *vbox;
  BtkWidget *label;
  BtkWidget *entry;
  BtkEntryCompletion *completion;
  BtkTreeModel *completion_model;
  BtkCellRenderer *cell;

  btk_init (&argc, &argv);

  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  btk_container_set_border_width (BTK_CONTAINER (window), 5);
  g_signal_connect (window, "delete_event", btk_main_quit, NULL);
  
  vbox = btk_vbox_new (FALSE, 2);
  btk_container_add (BTK_CONTAINER (window), vbox);
    
  btk_container_set_border_width (BTK_CONTAINER (vbox), 5);
  
  label = btk_label_new (NULL);

  btk_label_set_markup (BTK_LABEL (label), "Completion demo, try writing <b>total</b> or <b>bunny</b> for example.");
  btk_box_pack_start (BTK_BOX (vbox), label, FALSE, FALSE, 0);

  /* Create our first entry */
  entry = btk_entry_new ();
  
  /* Create the completion object */
  completion = btk_entry_completion_new ();
  btk_entry_completion_set_inline_completion (completion, TRUE);
  
  /* Assign the completion to the entry */
  btk_entry_set_completion (BTK_ENTRY (entry), completion);
  g_object_unref (completion);
  
  add_with_prop_edit_button (vbox, entry, completion);

  /* Create a tree model and use it as the completion model */
  completion_model = create_simple_completion_model ();
  btk_entry_completion_set_model (completion, completion_model);
  g_object_unref (completion_model);
  
  /* Use model column 0 as the text column */
  btk_entry_completion_set_text_column (completion, 0);

  /* Create our second entry */
  entry = btk_entry_new ();

  /* Create the completion object */
  completion = btk_entry_completion_new ();
  
  /* Assign the completion to the entry */
  btk_entry_set_completion (BTK_ENTRY (entry), completion);
  g_object_unref (completion);
  
  add_with_prop_edit_button (vbox, entry, completion);

  /* Create a tree model and use it as the completion model */
  completion_model = create_completion_model ();
  btk_entry_completion_set_model (completion, completion_model);
  btk_entry_completion_set_minimum_key_length (completion, 2);
  g_object_unref (completion_model);
  
  /* Use model column 1 as the text column */
  cell = btk_cell_renderer_pixbuf_new ();
  btk_cell_layout_pack_start (BTK_CELL_LAYOUT (completion), cell, FALSE);
  btk_cell_layout_set_attributes (BTK_CELL_LAYOUT (completion), cell, 
				  "pixbuf", 0, NULL); 

  cell = btk_cell_renderer_text_new ();
  btk_cell_layout_pack_start (BTK_CELL_LAYOUT (completion), cell, FALSE);
  btk_cell_layout_set_attributes (BTK_CELL_LAYOUT (completion), cell, 
				  "text", 1, NULL); 
  
  btk_entry_completion_set_match_func (completion, match_func, NULL, NULL);
  g_signal_connect (completion, "match-selected", 
		    G_CALLBACK (match_selected_cb), NULL);

  btk_entry_completion_insert_action_text (completion, 100, "action!");
  btk_entry_completion_insert_action_text (completion, 101, "'nother action!");
  g_signal_connect (completion, "action_activated", G_CALLBACK (activated_cb), NULL);

  /* Create our third entry */
  entry = btk_entry_new ();

  /* Create the completion object */
  completion = btk_entry_completion_new ();
  
  /* Assign the completion to the entry */
  btk_entry_set_completion (BTK_ENTRY (entry), completion);
  g_object_unref (completion);
  
  add_with_prop_edit_button (vbox, entry, completion);

  /* Create a tree model and use it as the completion model */
  completion_model = BTK_TREE_MODEL (btk_list_store_new (1, B_TYPE_STRING));

  btk_entry_completion_set_model (completion, completion_model);
  g_object_unref (completion_model);

  /* Use model column 0 as the text column */
  btk_entry_completion_set_text_column (completion, 0);

  /* Fill the completion dynamically */
  bdk_threads_add_timeout (1000, (GSourceFunc) animation_timer, completion);

  /* Fourth entry */
  btk_box_pack_start (BTK_BOX (vbox), btk_label_new ("Model-less entry completion"), FALSE, FALSE, 0);

  entry = btk_entry_new ();

  /* Create the completion object */
  completion = btk_entry_completion_new ();
  
  /* Assign the completion to the entry */
  btk_entry_set_completion (BTK_ENTRY (entry), completion);
  g_object_unref (completion);
  
  add_with_prop_edit_button (vbox, entry, completion);

  btk_widget_show_all (window);

  btk_main ();
  
  return 0;
}


