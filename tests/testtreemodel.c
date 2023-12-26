/* testtreemodel.c
 * Copyright (C) 2004  Red Hat, Inc.,  Matthias Clasen <mclasen@redhat.com>
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

#include <string.h>

#ifdef HAVE_MALLINFO
#include <malloc.h>
#endif

#include <btk/btk.h>

static bint repeats = 2;
static bint max_size = 8;

static GOptionEntry entries[] = {
  { "repeats", 'r', 0, G_OPTION_ARG_INT, &repeats, "Average over N repetitions", "N" },
  { "max-size", 'm', 0, G_OPTION_ARG_INT, &max_size, "Test up to 2^M items", "M" },
  { NULL }
};


typedef void (ClearFunc)(BtkTreeModel *model);
typedef void (InsertFunc)(BtkTreeModel *model,
			  bint          items,
			  bint          i);

static void
list_store_append (BtkTreeModel *model,
		   bint          items,
		   bint          i)
{
  BtkListStore *store = BTK_LIST_STORE (model);
  BtkTreeIter iter;
  bchar *text;

  text = g_strdup_printf ("row %d", i);
  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter, 0, i, 1, text, -1);
  g_free (text);
}

static void
list_store_prepend (BtkTreeModel *model,
		    bint          items,
		    bint          i)
{
  BtkListStore *store = BTK_LIST_STORE (model);
  BtkTreeIter iter;
  bchar *text;

  text = g_strdup_printf ("row %d", i);
  btk_list_store_prepend (store, &iter);
  btk_list_store_set (store, &iter, 0, i, 1, text, -1);
  g_free (text);
}

static void
list_store_insert (BtkTreeModel *model,
		   bint          items,
		   bint          i)
{
  BtkListStore *store = BTK_LIST_STORE (model);
  BtkTreeIter iter;
  bchar *text;
  bint n;

  text = g_strdup_printf ("row %d", i);
  n = g_random_int_range (0, i + 1);
  btk_list_store_insert (store, &iter, n);
  btk_list_store_set (store, &iter, 0, i, 1, text, -1);
  g_free (text);
}

static bint
compare (BtkTreeModel *model,
	 BtkTreeIter  *a,
	 BtkTreeIter  *b,
	 bpointer      data)
{
  bchar *str_a, *str_b;
  bint result;

  btk_tree_model_get (model, a, 1, &str_a, -1);
  btk_tree_model_get (model, b, 1, &str_b, -1);
  
  result = strcmp (str_a, str_b);

  g_free (str_a);
  g_free (str_b);

  return result;
}

static void
tree_store_append (BtkTreeModel *model,
		   bint          items,
		   bint          i)
{
  BtkTreeStore *store = BTK_TREE_STORE (model);
  BtkTreeIter iter;
  bchar *text;

  text = g_strdup_printf ("row %d", i);
  btk_tree_store_append (store, &iter, NULL);
  btk_tree_store_set (store, &iter, 0, i, 1, text, -1);
  g_free (text);
}

static void
tree_store_prepend (BtkTreeModel *model,
		    bint          items,
		    bint          i)
{
  BtkTreeStore *store = BTK_TREE_STORE (model);
  BtkTreeIter iter;
  bchar *text;

  text = g_strdup_printf ("row %d", i);
  btk_tree_store_prepend (store, &iter, NULL);
  btk_tree_store_set (store, &iter, 0, i, 1, text, -1);
  g_free (text);
}

static void
tree_store_insert_flat (BtkTreeModel *model,
			bint          items,
			bint          i)
{
  BtkTreeStore *store = BTK_TREE_STORE (model);
  BtkTreeIter iter;
  bchar *text;
  bint n;

  text = g_strdup_printf ("row %d", i);
  n = g_random_int_range (0, i + 1);
  btk_tree_store_insert (store, &iter, NULL, n);
  btk_tree_store_set (store, &iter, 0, i, 1, text, -1);
  g_free (text);
}

typedef struct {
  bint i;
  bint n;
  bboolean found;
  BtkTreeIter iter;
} FindData;

static bboolean
find_nth (BtkTreeModel *model,
	  BtkTreePath  *path,
	  BtkTreeIter  *iter,
	  bpointer      data)
{
  FindData *fdata = (FindData *)data; 

  if (fdata->i >= fdata->n)
    {
      fdata->iter = *iter;
      fdata->found = TRUE;
      return TRUE;
    }

  fdata->i++;

  return FALSE;
}

static void
tree_store_insert_deep (BtkTreeModel *model,
			bint          items,
			bint          i)
{
  BtkTreeStore *store = BTK_TREE_STORE (model);
  BtkTreeIter iter;
  bchar *text;
  FindData data;

  text = g_strdup_printf ("row %d", i);
  data.n = g_random_int_range (0, items);
  data.i = 0;
  data.found = FALSE;
  if (data.n < i)
    btk_tree_model_foreach (model, find_nth, &data);
  btk_tree_store_insert (store, &iter, data.found ? &(data.iter) : NULL, data.n);
  btk_tree_store_set (store, &iter, 0, i, 1, text, -1);
  g_free (text);
}


static void
test_run (bchar        *title,
	  BtkTreeModel *store,
	  ClearFunc    *clear,
	  InsertFunc   *insert)
{
  bint i, k, d, items;
  GTimer *timer;
  bdouble elapsed;
  int uordblks_before = 0, memused;

  g_print ("%s (average over %d runs, time in milliseconds)\n"
	   "items \ttime      \ttime/item \tused memory\n", title, repeats);

  timer = g_timer_new ();

  for (k = 0; k < max_size; k++)
    {
      items = 1 << k;
      elapsed = 0.0;
      for (d = 0; d < repeats; d++)
	{
	  (*clear)(store);
#ifdef HAVE_MALLINFO
	  /* Peculiar location of this, btw.  -- MW.  */
	  uordblks_before = mallinfo().uordblks;
#endif
	  g_timer_reset (timer);
	  g_timer_start (timer);
	  for (i = 0; i < items; i++)
	    (*insert) (store, items, i);
	  g_timer_stop (timer);
	  elapsed += g_timer_elapsed (timer, NULL);
	}
      
      elapsed = elapsed * 1000 / repeats;
#ifdef HAVE_MALLINFO
      memused = (mallinfo().uordblks - uordblks_before) / 1024;
#else
      memused = 0;
#endif
      g_print ("%d \t%f \t%f  \t%dk\n", 
	       items, elapsed, elapsed/items, memused);
    }  
}

int
main (int argc, char *argv[])
{
  BtkTreeModel *model;
  
  btk_init_with_args (&argc, &argv, NULL, entries, NULL, NULL);

  model = BTK_TREE_MODEL (btk_list_store_new (2, B_TYPE_INT, B_TYPE_STRING));
  
  test_run ("list store append", 
	    model, 
	    (ClearFunc*)btk_list_store_clear, 
	    (InsertFunc*)list_store_append);

  test_run ("list store prepend", 
	    model, 
	    (ClearFunc*)btk_list_store_clear, 
	    (InsertFunc*)list_store_prepend);

  test_run ("list store insert", 
	    model, 
	    (ClearFunc*)btk_list_store_clear, 
	    (InsertFunc*)list_store_insert);

  btk_tree_sortable_set_default_sort_func (BTK_TREE_SORTABLE (model), 
					   compare, NULL, NULL);
  btk_tree_sortable_set_sort_column_id (BTK_TREE_SORTABLE (model), 
					BTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
					BTK_SORT_ASCENDING);

  test_run ("list store insert (sorted)", 
	    model, 
	    (ClearFunc*)btk_list_store_clear, 
	    (InsertFunc*)list_store_insert);

  g_object_unref (model);
  
  model = BTK_TREE_MODEL (btk_tree_store_new (2, B_TYPE_INT, B_TYPE_STRING));

  test_run ("tree store append", 
	    model, 
	    (ClearFunc*)btk_tree_store_clear, 
	    (InsertFunc*)tree_store_append);

  test_run ("tree store prepend", 
	    model, 
	    (ClearFunc*)btk_tree_store_clear, 
	    (InsertFunc*)tree_store_prepend);

  test_run ("tree store insert (flat)", 
	    model, 
	    (ClearFunc*)btk_tree_store_clear, 
	    (InsertFunc*)tree_store_insert_flat);

  test_run ("tree store insert (deep)", 
	    model, 
	    (ClearFunc*)btk_tree_store_clear, 
	    (InsertFunc*)tree_store_insert_deep);

  btk_tree_sortable_set_default_sort_func (BTK_TREE_SORTABLE (model), 
					   compare, NULL, NULL);
  btk_tree_sortable_set_sort_column_id (BTK_TREE_SORTABLE (model), 
					BTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
					BTK_SORT_ASCENDING);

  test_run ("tree store insert (flat, sorted)", 
	    model, 
	    (ClearFunc*)btk_tree_store_clear, 
	    (InsertFunc*)tree_store_insert_flat);

  test_run ("tree store insert (deep, sorted)", 
	    model, 
	    (ClearFunc*)btk_tree_store_clear, 
	    (InsertFunc*)tree_store_insert_deep);

  return 0;
}
