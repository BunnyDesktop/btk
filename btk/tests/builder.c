/* buildertest.c
 * Copyright (C) 2006-2007 Async Open Source
 * Authors: Johan Dahlin
 *          Henrique Romano
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

#include <string.h>
#include <libintl.h>
#include <locale.h>
#include <math.h>

#include <btk/btk.h>
#include <bdk/bdkkeysyms.h>

/* Copied from btkiconfactory.c; keep in sync! */
struct _BtkIconSet
{
  buint ref_count;
  GSList *sources;
  GSList *cache;
  buint cache_size;
  buint cache_serial;
};


static BtkBuilder *
builder_new_from_string (const bchar *buffer,
                         bsize length,
                         const bchar *domain)
{
  BtkBuilder *builder;
  GError *error = NULL;

  builder = btk_builder_new ();
  if (domain)
    btk_builder_set_translation_domain (builder, domain);
  btk_builder_add_from_string (builder, buffer, length, &error);
  if (error)
    {
      g_print ("ERROR: %s", error->message);
      g_error_free (error);
    }

  return builder;
}

static void
test_parser (void)
{
  BtkBuilder *builder;
  GError *error;
  
  builder = btk_builder_new ();

  error = NULL;
  btk_builder_add_from_string (builder, "<xxx/>", -1, &error);
  g_assert (g_error_matches (error, 
                             BTK_BUILDER_ERROR,
                             BTK_BUILDER_ERROR_UNHANDLED_TAG));
  g_error_free (error);
  
  error = NULL;
  btk_builder_add_from_string (builder, "<interface invalid=\"X\"/>", -1, &error);
  g_assert (g_error_matches (error,
                             BTK_BUILDER_ERROR,
                             BTK_BUILDER_ERROR_INVALID_ATTRIBUTE));
  g_error_free (error);

  error = NULL;
  btk_builder_add_from_string (builder, "<interface><child/></interface>", -1, &error);
  g_assert (g_error_matches (error,
                             BTK_BUILDER_ERROR, 
                             BTK_BUILDER_ERROR_INVALID_TAG));
  g_error_free (error);

  error = NULL;
  btk_builder_add_from_string (builder, "<interface><object class=\"BtkVBox\" id=\"a\"><object class=\"BtkHBox\" id=\"b\"/></object></interface>", -1, &error);
  g_assert (g_error_matches (error,
                             BTK_BUILDER_ERROR,
                             BTK_BUILDER_ERROR_INVALID_TAG));
  g_error_free (error);

  error = NULL;
  btk_builder_add_from_string (builder, "<interface><object class=\"Unknown\" id=\"a\"></object></interface>", -1, &error);
  g_assert (g_error_matches (error,
                             BTK_BUILDER_ERROR,
                             BTK_BUILDER_ERROR_INVALID_VALUE));
  g_error_free (error);

  error = NULL;
  btk_builder_add_from_string (builder, "<interface><object class=\"BtkWidget\" id=\"a\" constructor=\"none\"></object></interface>", -1, &error);
  g_assert (g_error_matches (error,
                             BTK_BUILDER_ERROR,
                             BTK_BUILDER_ERROR_INVALID_VALUE));
  g_error_free (error);

  error = NULL;
  btk_builder_add_from_string (builder, "<interface><object class=\"BtkButton\" id=\"a\"><child internal-child=\"foobar\"><object class=\"BtkButton\" id=\"int\"/></child></object></interface>", -1, &error);
  g_assert (g_error_matches (error,
                             BTK_BUILDER_ERROR,
                             BTK_BUILDER_ERROR_INVALID_VALUE));
  g_error_free (error);

  error = NULL;
  btk_builder_add_from_string (builder, "<interface><object class=\"BtkButton\" id=\"a\"></object><object class=\"BtkButton\" id=\"a\"/></object></interface>", -1, &error);
  g_assert (g_error_matches (error,
                             BTK_BUILDER_ERROR,
                             BTK_BUILDER_ERROR_DUPLICATE_ID));
  g_error_free (error);

  g_object_unref (builder);
}

static int normal = 0;
static int after = 0;
static int object = 0;
static int object_after = 0;

void /* exported for BtkBuilder */
signal_normal (BtkWindow *window, BParamSpec spec)
{
  g_assert (BTK_IS_WINDOW (window));
  g_assert (normal == 0);
  g_assert (after == 0);

  normal++;
}

void /* exported for BtkBuilder */
signal_after (BtkWindow *window, BParamSpec spec)
{
  g_assert (BTK_IS_WINDOW (window));
  g_assert (normal == 1);
  g_assert (after == 0);
  
  after++;
}

void /* exported for BtkBuilder */
signal_object (BtkButton *button, BParamSpec spec)
{
  g_assert (BTK_IS_BUTTON (button));
  g_assert (object == 0);
  g_assert (object_after == 0);

  object++;
}

void /* exported for BtkBuilder */
signal_object_after (BtkButton *button, BParamSpec spec)
{
  g_assert (BTK_IS_BUTTON (button));
  g_assert (object == 1);
  g_assert (object_after == 0);

  object_after++;
}

void /* exported for BtkBuilder */
signal_first (BtkButton *button, BParamSpec spec)
{
  g_assert (normal == 0);
  normal = 10;
}

void /* exported for BtkBuilder */
signal_second (BtkButton *button, BParamSpec spec)
{
  g_assert (normal == 10);
  normal = 20;
}

void /* exported for BtkBuilder */
signal_extra (BtkButton *button, BParamSpec spec)
{
  g_assert (normal == 20);
  normal = 30;
}

void /* exported for BtkBuilder */
signal_extra2 (BtkButton *button, BParamSpec spec)
{
  g_assert (normal == 30);
  normal = 40;
}

static void
test_connect_signals (void)
{
  BtkBuilder *builder;
  BObject *window;
  const bchar buffer[] =
    "<interface>"
    "  <object class=\"BtkButton\" id=\"button\"/>"
    "  <object class=\"BtkWindow\" id=\"window1\">"
    "    <signal name=\"notify::title\" handler=\"signal_normal\"/>"
    "    <signal name=\"notify::title\" handler=\"signal_after\" after=\"yes\"/>"
    "    <signal name=\"notify::title\" handler=\"signal_object\""
    "            object=\"button\"/>"
    "    <signal name=\"notify::title\" handler=\"signal_object_after\""
    "            object=\"button\" after=\"yes\"/>"
    "  </object>"
    "</interface>";
  const bchar buffer_order[] =
    "<interface>"
    "  <object class=\"BtkWindow\" id=\"window1\">"
    "    <signal name=\"notify::title\" handler=\"signal_first\"/>"
    "    <signal name=\"notify::title\" handler=\"signal_second\"/>"
    "  </object>"
    "</interface>";
  const bchar buffer_extra[] =
    "<interface>"
    "  <object class=\"BtkWindow\" id=\"window2\">"
    "    <signal name=\"notify::title\" handler=\"signal_extra\"/>"
    "  </object>"
    "</interface>";
  const bchar buffer_extra2[] =
    "<interface>"
    "  <object class=\"BtkWindow\" id=\"window3\">"
    "    <signal name=\"notify::title\" handler=\"signal_extra2\"/>"
    "  </object>"
    "</interface>";
  const bchar buffer_after_child[] =
    "<interface>"
    "  <object class=\"BtkWindow\" id=\"window1\">"
    "    <child>"
    "      <object class=\"BtkButton\" id=\"button1\"/>"
    "    </child>"
    "    <signal name=\"notify::title\" handler=\"signal_normal\"/>"
    "  </object>"
    "</interface>";

  builder = builder_new_from_string (buffer, -1, NULL);
  btk_builder_connect_signals (builder, NULL);

  window = btk_builder_get_object (builder, "window1");
  btk_window_set_title (BTK_WINDOW (window), "test");

  g_assert_cmpint (normal, ==, 1);
  g_assert_cmpint (after, ==, 1);
  g_assert_cmpint (object, ==, 1);
  g_assert_cmpint (object_after, ==, 1);

  btk_widget_destroy (BTK_WIDGET (window));
  g_object_unref (builder);
  
  builder = builder_new_from_string (buffer_order, -1, NULL);
  btk_builder_connect_signals (builder, NULL);
  window = btk_builder_get_object (builder, "window1");
  normal = 0;
  btk_window_set_title (BTK_WINDOW (window), "test");
  g_assert (normal == 20);

  btk_widget_destroy (BTK_WIDGET (window));

  btk_builder_add_from_string (builder, buffer_extra,
			       strlen (buffer_extra), NULL);
  btk_builder_add_from_string (builder, buffer_extra2,
			       strlen (buffer_extra2), NULL);
  btk_builder_connect_signals (builder, NULL);
  window = btk_builder_get_object (builder, "window2");
  btk_window_set_title (BTK_WINDOW (window), "test");
  g_assert (normal == 30);

  btk_widget_destroy (BTK_WIDGET (window));
  window = btk_builder_get_object (builder, "window3");
  btk_window_set_title (BTK_WINDOW (window), "test");
  g_assert (normal == 40);
  btk_widget_destroy (BTK_WIDGET (window));
  
  g_object_unref (builder);

  /* new test, reset globals */
  after = 0;
  normal = 0;
  
  builder = builder_new_from_string (buffer_after_child, -1, NULL);
  window = btk_builder_get_object (builder, "window1");
  btk_builder_connect_signals (builder, NULL);
  btk_window_set_title (BTK_WINDOW (window), "test");

  g_assert (normal == 1);
  btk_widget_destroy (BTK_WIDGET (window));
  g_object_unref (builder);
}

static void
test_uimanager_simple (void)
{
  BtkBuilder *builder;
  BObject *window, *uimgr, *menubar;
  BObject *menu, *label;
  GList *children;
  const bchar buffer[] =
    "<interface>"
    "  <object class=\"BtkUIManager\" id=\"uimgr1\"/>"
    "</interface>";
    
  const bchar buffer2[] =
    "<interface>"
    "  <object class=\"BtkUIManager\" id=\"uimgr1\">"
    "    <child>"
    "      <object class=\"BtkActionGroup\" id=\"ag1\">"
    "        <child>"
    "          <object class=\"BtkAction\" id=\"file\">"
    "            <property name=\"label\">_File</property>"
    "          </object>"
    "          <accelerator key=\"n\" modifiers=\"BDK_CONTROL_MASK\"/>"
    "        </child>"
    "      </object>"
    "    </child>"
    "    <ui>"
    "      <menubar name=\"menubar1\">"
    "        <menu action=\"file\">"
    "        </menu>"
    "      </menubar>"
    "    </ui>"
    "  </object>"
    "  <object class=\"BtkWindow\" id=\"window1\">"
    "    <child>"
    "      <object class=\"BtkMenuBar\" id=\"menubar1\" constructor=\"uimgr1\"/>"
    "    </child>"
    "  </object>"
    "</interface>";

  builder = builder_new_from_string (buffer, -1, NULL);

  uimgr = btk_builder_get_object (builder, "uimgr1");
  g_assert (BTK_IS_UI_MANAGER (uimgr));
  g_object_unref (builder);
  
  builder = builder_new_from_string (buffer2, -1, NULL);

  menubar = btk_builder_get_object (builder, "menubar1");
  g_assert (BTK_IS_MENU_BAR (menubar));

  children = btk_container_get_children (BTK_CONTAINER (menubar));
  menu = children->data;
  g_assert (BTK_IS_MENU_ITEM (menu));
  g_assert (strcmp (BTK_WIDGET (menu)->name, "file") == 0);
  g_list_free (children);
  
  label = B_OBJECT (BTK_BIN (menu)->child);
  g_assert (BTK_IS_LABEL (label));
  g_assert (strcmp (btk_label_get_text (BTK_LABEL (label)), "File") == 0);

  window = btk_builder_get_object (builder, "window1");
  btk_widget_destroy (BTK_WIDGET (window));
  g_object_unref (builder);
}

static void
test_domain (void)
{
  BtkBuilder *builder;
  const bchar buffer1[] = "<interface/>";
  const bchar buffer2[] = "<interface domain=\"domain\"/>";
  const bchar *domain;
  
  builder = builder_new_from_string (buffer1, -1, NULL);
  domain = btk_builder_get_translation_domain (builder);
  g_assert (domain == NULL);
  g_object_unref (builder);
  
  builder = builder_new_from_string (buffer1, -1, "domain-1");
  domain = btk_builder_get_translation_domain (builder);
  g_assert (domain);
  g_assert (strcmp (domain, "domain-1") == 0);
  g_object_unref (builder);

  builder = builder_new_from_string (buffer2, -1, NULL);
  domain = btk_builder_get_translation_domain (builder);
  g_assert (domain == NULL);
  g_object_unref (builder);
}

#if 0
static void
test_translation (void)
{
  BtkBuilder *builder;
  const bchar buffer[] =
    "<interface>"
    "  <object class=\"BtkWindow\" id=\"window1\">"
    "    <child>"
    "      <object class=\"BtkLabel\" id=\"label\">"
    "        <property name=\"label\" translatable=\"yes\">File</property>"
    "      </object>"
    "    </child>"
    "  </object>"
    "</interface>";
  BtkLabel *window, *label;

  setlocale (LC_ALL, "sv_SE");
  textdomain ("builder");
  bindtextdomain ("builder", "tests");

  builder = builder_new_from_string (buffer, -1, NULL);
  label = BTK_LABEL (btk_builder_get_object (builder, "label"));
  g_assert (strcmp (btk_label_get_text (label), "Arkiv") == 0);

  window = btk_builder_get_object (builder, "window1");
  btk_widget_destroy (BTK_WIDGET (window));
  g_object_unref (builder);
}
#endif

static void
test_sizegroup (void)
{
  BtkBuilder * builder;
  const bchar buffer1[] =
    "<interface domain=\"test\">"
    "  <object class=\"BtkSizeGroup\" id=\"sizegroup1\">"
    "    <property name=\"mode\">BTK_SIZE_GROUP_HORIZONTAL</property>"
    "    <widgets>"
    "      <widget name=\"radio1\"/>"
    "      <widget name=\"radio2\"/>"
    "    </widgets>"
    "  </object>"
    "  <object class=\"BtkWindow\" id=\"window1\">"
    "    <child>"
    "      <object class=\"BtkVBox\" id=\"vbox1\">"
    "        <child>"
    "          <object class=\"BtkRadioButton\" id=\"radio1\"/>"
    "        </child>"
    "        <child>"
    "          <object class=\"BtkRadioButton\" id=\"radio2\"/>"
    "        </child>"
    "      </object>"
    "    </child>"
    "  </object>"
    "</interface>";
  const bchar buffer2[] =
    "<interface domain=\"test\">"
    "  <object class=\"BtkSizeGroup\" id=\"sizegroup1\">"
    "    <property name=\"mode\">BTK_SIZE_GROUP_HORIZONTAL</property>"
    "    <widgets>"
    "    </widgets>"
    "   </object>"
    "</interface>";
  const bchar buffer3[] =
    "<interface domain=\"test\">"
    "  <object class=\"BtkSizeGroup\" id=\"sizegroup1\">"
    "    <property name=\"mode\">BTK_SIZE_GROUP_HORIZONTAL</property>"
    "    <widgets>"
    "      <widget name=\"radio1\"/>"
    "      <widget name=\"radio2\"/>"
    "    </widgets>"
    "  </object>"
    "  <object class=\"BtkSizeGroup\" id=\"sizegroup2\">"
    "    <property name=\"mode\">BTK_SIZE_GROUP_HORIZONTAL</property>"
    "    <widgets>"
    "      <widget name=\"radio1\"/>"
    "      <widget name=\"radio2\"/>"
    "    </widgets>"
    "  </object>"
    "  <object class=\"BtkWindow\" id=\"window1\">"
    "    <child>"
    "      <object class=\"BtkVBox\" id=\"vbox1\">"
    "        <child>"
    "          <object class=\"BtkRadioButton\" id=\"radio1\"/>"
    "        </child>"
    "        <child>"
    "          <object class=\"BtkRadioButton\" id=\"radio2\"/>"
    "        </child>"
    "      </object>"
    "    </child>"
    "  </object>"
    "</interface>";
  BObject *sizegroup;
  GSList *widgets;

  builder = builder_new_from_string (buffer1, -1, NULL);
  sizegroup = btk_builder_get_object (builder, "sizegroup1");
  widgets = btk_size_group_get_widgets (BTK_SIZE_GROUP (sizegroup));
  g_assert (b_slist_length (widgets) == 2);
  b_slist_free (widgets);
  g_object_unref (builder);

  builder = builder_new_from_string (buffer2, -1, NULL);
  sizegroup = btk_builder_get_object (builder, "sizegroup1");
  widgets = btk_size_group_get_widgets (BTK_SIZE_GROUP (sizegroup));
  g_assert (b_slist_length (widgets) == 0);
  b_slist_free (widgets);
  g_object_unref (builder);

  builder = builder_new_from_string (buffer3, -1, NULL);
  sizegroup = btk_builder_get_object (builder, "sizegroup1");
  widgets = btk_size_group_get_widgets (BTK_SIZE_GROUP (sizegroup));
  g_assert (b_slist_length (widgets) == 2);
  b_slist_free (widgets);
  sizegroup = btk_builder_get_object (builder, "sizegroup2");
  widgets = btk_size_group_get_widgets (BTK_SIZE_GROUP (sizegroup));
  g_assert (b_slist_length (widgets) == 2);
  b_slist_free (widgets);

#if 0
  {
    BObject *window;
    window = btk_builder_get_object (builder, "window1");
    btk_widget_destroy (BTK_WIDGET (window));
  }
#endif  
  g_object_unref (builder);
}

static void
test_list_store (void)
{
  const bchar buffer1[] =
    "<interface>"
    "  <object class=\"BtkListStore\" id=\"liststore1\">"
    "    <columns>"
    "      <column type=\"bchararray\"/>"
    "      <column type=\"buint\"/>"
    "    </columns>"
    "  </object>"
    "</interface>";
  const char buffer2[] = 
    "<interface>"
    "  <object class=\"BtkListStore\" id=\"liststore1\">"
    "    <columns>"
    "      <column type=\"bchararray\"/>"
    "      <column type=\"bchararray\"/>"
    "      <column type=\"bint\"/>"
    "    </columns>"
    "    <data>"
    "      <row>"
    "        <col id=\"0\" translatable=\"yes\">John</col>"
    "        <col id=\"1\" context=\"foo\">Doe</col>"
    "        <col id=\"2\" comment=\"foobar\">25</col>"
    "      </row>"
    "      <row>"
    "        <col id=\"0\">Johan</col>"
    "        <col id=\"1\">Dole</col>"
    "        <col id=\"2\">50</col>"
    "      </row>"
    "    </data>"
    "  </object>"
    "</interface>";
  const char buffer3[] = 
    "<interface>"
    "  <object class=\"BtkListStore\" id=\"liststore1\">"
    "    <columns>"
    "      <column type=\"bchararray\"/>"
    "      <column type=\"bchararray\"/>"
    "      <column type=\"bint\"/>"
    "    </columns>"
    "    <data>"
    "      <row>"
    "        <col id=\"1\" context=\"foo\">Doe</col>"
    "        <col id=\"0\" translatable=\"yes\">John</col>"
    "        <col id=\"2\" comment=\"foobar\">25</col>"
    "      </row>"
    "      <row>"
    "        <col id=\"2\">50</col>"
    "        <col id=\"1\">Dole</col>"
    "        <col id=\"0\">Johan</col>"
    "      </row>"
    "      <row>"
    "        <col id=\"2\">19</col>"
    "      </row>"
    "    </data>"
    "  </object>"
    "</interface>";
  BtkBuilder *builder;
  BObject *store;
  BtkTreeIter iter;
  bchar *surname, *lastname;
  int age;
  
  builder = builder_new_from_string (buffer1, -1, NULL);
  store = btk_builder_get_object (builder, "liststore1");
  g_assert (btk_tree_model_get_n_columns (BTK_TREE_MODEL (store)) == 2);
  g_assert (btk_tree_model_get_column_type (BTK_TREE_MODEL (store), 0) == B_TYPE_STRING);
  g_assert (btk_tree_model_get_column_type (BTK_TREE_MODEL (store), 1) == B_TYPE_UINT);
  g_object_unref (builder);
  
  builder = builder_new_from_string (buffer2, -1, NULL);
  store = btk_builder_get_object (builder, "liststore1");
  g_assert (btk_tree_model_get_n_columns (BTK_TREE_MODEL (store)) == 3);
  g_assert (btk_tree_model_get_column_type (BTK_TREE_MODEL (store), 0) == B_TYPE_STRING);
  g_assert (btk_tree_model_get_column_type (BTK_TREE_MODEL (store), 1) == B_TYPE_STRING);
  g_assert (btk_tree_model_get_column_type (BTK_TREE_MODEL (store), 2) == B_TYPE_INT);
  
  g_assert (btk_tree_model_get_iter_first (BTK_TREE_MODEL (store), &iter) == TRUE);
  btk_tree_model_get (BTK_TREE_MODEL (store), &iter,
                      0, &surname,
                      1, &lastname,
                      2, &age,
                      -1);
  g_assert (surname != NULL);
  g_assert (strcmp (surname, "John") == 0);
  g_free (surname);
  g_assert (lastname != NULL);
  g_assert (strcmp (lastname, "Doe") == 0);
  g_free (lastname);
  g_assert (age == 25);
  g_assert (btk_tree_model_iter_next (BTK_TREE_MODEL (store), &iter) == TRUE);
  
  btk_tree_model_get (BTK_TREE_MODEL (store), &iter,
                      0, &surname,
                      1, &lastname,
                      2, &age,
                      -1);
  g_assert (surname != NULL);
  g_assert (strcmp (surname, "Johan") == 0);
  g_free (surname);
  g_assert (lastname != NULL);
  g_assert (strcmp (lastname, "Dole") == 0);
  g_free (lastname);
  g_assert (age == 50);
  g_assert (btk_tree_model_iter_next (BTK_TREE_MODEL (store), &iter) == FALSE);

  g_object_unref (builder);  

  builder = builder_new_from_string (buffer3, -1, NULL);
  store = btk_builder_get_object (builder, "liststore1");
  g_assert (btk_tree_model_get_n_columns (BTK_TREE_MODEL (store)) == 3);
  g_assert (btk_tree_model_get_column_type (BTK_TREE_MODEL (store), 0) == B_TYPE_STRING);
  g_assert (btk_tree_model_get_column_type (BTK_TREE_MODEL (store), 1) == B_TYPE_STRING);
  g_assert (btk_tree_model_get_column_type (BTK_TREE_MODEL (store), 2) == B_TYPE_INT);
  
  g_assert (btk_tree_model_get_iter_first (BTK_TREE_MODEL (store), &iter) == TRUE);
  btk_tree_model_get (BTK_TREE_MODEL (store), &iter,
                      0, &surname,
                      1, &lastname,
                      2, &age,
                      -1);
  g_assert (surname != NULL);
  g_assert (strcmp (surname, "John") == 0);
  g_free (surname);
  g_assert (lastname != NULL);
  g_assert (strcmp (lastname, "Doe") == 0);
  g_free (lastname);
  g_assert (age == 25);
  g_assert (btk_tree_model_iter_next (BTK_TREE_MODEL (store), &iter) == TRUE);
  
  btk_tree_model_get (BTK_TREE_MODEL (store), &iter,
                      0, &surname,
                      1, &lastname,
                      2, &age,
                      -1);
  g_assert (surname != NULL);
  g_assert (strcmp (surname, "Johan") == 0);
  g_free (surname);
  g_assert (lastname != NULL);
  g_assert (strcmp (lastname, "Dole") == 0);
  g_free (lastname);
  g_assert (age == 50);
  g_assert (btk_tree_model_iter_next (BTK_TREE_MODEL (store), &iter) == TRUE);
  
  btk_tree_model_get (BTK_TREE_MODEL (store), &iter,
                      0, &surname,
                      1, &lastname,
                      2, &age,
                      -1);
  g_assert (surname == NULL);
  g_assert (lastname == NULL);
  g_assert (age == 19);
  g_assert (btk_tree_model_iter_next (BTK_TREE_MODEL (store), &iter) == FALSE);

  g_object_unref (builder);
}

static void
test_tree_store (void)
{
  const bchar buffer[] =
    "<interface domain=\"test\">"
    "  <object class=\"BtkTreeStore\" id=\"treestore1\">"
    "    <columns>"
    "      <column type=\"bchararray\"/>"
    "      <column type=\"buint\"/>"
    "    </columns>"
    "  </object>"
    "</interface>";
  BtkBuilder *builder;
  BObject *store;
  
  builder = builder_new_from_string (buffer, -1, NULL);
  store = btk_builder_get_object (builder, "treestore1");
  g_assert (btk_tree_model_get_n_columns (BTK_TREE_MODEL (store)) == 2);
  g_assert (btk_tree_model_get_column_type (BTK_TREE_MODEL (store), 0) == B_TYPE_STRING);
  g_assert (btk_tree_model_get_column_type (BTK_TREE_MODEL (store), 1) == B_TYPE_UINT);
  
  g_object_unref (builder);
}

static void
test_types (void)
{
  const bchar buffer[] = 
    "<interface>"
    "  <object class=\"BtkAction\" id=\"action\"/>"
    "  <object class=\"BtkActionGroup\" id=\"actiongroup\"/>"
    "  <object class=\"BtkAlignment\" id=\"alignment\"/>"
    "  <object class=\"BtkArrow\" id=\"arrow\"/>"
    "  <object class=\"BtkButton\" id=\"button\"/>"
    "  <object class=\"BtkCheckButton\" id=\"checkbutton\"/>"
    "  <object class=\"BtkDialog\" id=\"dialog\"/>"
    "  <object class=\"BtkDrawingArea\" id=\"drawingarea\"/>"
    "  <object class=\"BtkEventBox\" id=\"eventbox\"/>"
    "  <object class=\"BtkEntry\" id=\"entry\"/>"
    "  <object class=\"BtkFontButton\" id=\"fontbutton\"/>"
    "  <object class=\"BtkHButtonBox\" id=\"hbuttonbox\"/>"
    "  <object class=\"BtkHBox\" id=\"hbox\"/>"
    "  <object class=\"BtkHPaned\" id=\"hpaned\"/>"
    "  <object class=\"BtkHRuler\" id=\"hruler\"/>"
    "  <object class=\"BtkHScale\" id=\"hscale\"/>"
    "  <object class=\"BtkHScrollbar\" id=\"hscrollbar\"/>"
    "  <object class=\"BtkHSeparator\" id=\"hseparator\"/>"
    "  <object class=\"BtkImage\" id=\"image\"/>"
    "  <object class=\"BtkLabel\" id=\"label\"/>"
    "  <object class=\"BtkListStore\" id=\"liststore\"/>"
    "  <object class=\"BtkMenuBar\" id=\"menubar\"/>"
    "  <object class=\"BtkNotebook\" id=\"notebook\"/>"
    "  <object class=\"BtkProgressBar\" id=\"progressbar\"/>"
    "  <object class=\"BtkRadioButton\" id=\"radiobutton\"/>"
    "  <object class=\"BtkSizeGroup\" id=\"sizegroup\"/>"
    "  <object class=\"BtkScrolledWindow\" id=\"scrolledwindow\"/>"
    "  <object class=\"BtkSpinButton\" id=\"spinbutton\"/>"
    "  <object class=\"BtkStatusbar\" id=\"statusbar\"/>"
    "  <object class=\"BtkTextView\" id=\"textview\"/>"
    "  <object class=\"BtkToggleAction\" id=\"toggleaction\"/>"
    "  <object class=\"BtkToggleButton\" id=\"togglebutton\"/>"
    "  <object class=\"BtkToolbar\" id=\"toolbar\"/>"
    "  <object class=\"BtkTreeStore\" id=\"treestore\"/>"
    "  <object class=\"BtkTreeView\" id=\"treeview\"/>"
    "  <object class=\"BtkTable\" id=\"table\"/>"
    "  <object class=\"BtkVBox\" id=\"vbox\"/>"
    "  <object class=\"BtkVButtonBox\" id=\"vbuttonbox\"/>"
    "  <object class=\"BtkVScrollbar\" id=\"vscrollbar\"/>"
    "  <object class=\"BtkVSeparator\" id=\"vseparator\"/>"
    "  <object class=\"BtkViewport\" id=\"viewport\"/>"
    "  <object class=\"BtkVRuler\" id=\"vruler\"/>"
    "  <object class=\"BtkVPaned\" id=\"vpaned\"/>"
    "  <object class=\"BtkVScale\" id=\"vscale\"/>"
    "  <object class=\"BtkWindow\" id=\"window\"/>"
    "  <object class=\"BtkUIManager\" id=\"uimanager\"/>"
    "</interface>";
  const bchar buffer2[] = 
    "<interface>"
    "  <object type-func=\"btk_window_get_type\" id=\"window\"/>"
    "</interface>";
  const bchar buffer3[] = 
    "<interface>"
    "  <object type-func=\"xxx_invalid_get_type_function\" id=\"window\"/>"
    "</interface>";
  BtkBuilder *builder;
  BObject *window;
  GError *error;

  builder = builder_new_from_string (buffer, -1, NULL);
  btk_widget_destroy (BTK_WIDGET (btk_builder_get_object (builder, "dialog")));
  btk_widget_destroy (BTK_WIDGET (btk_builder_get_object (builder, "window")));
  g_object_unref (builder);

  builder = builder_new_from_string (buffer2, -1, NULL);
  window = btk_builder_get_object (builder, "window");
  g_assert (BTK_IS_WINDOW (window));
  btk_widget_destroy (BTK_WIDGET (window));
  g_object_unref (builder);
  
  error = NULL;
  builder = btk_builder_new ();
  btk_builder_add_from_string (builder, buffer3, -1, &error);
  g_assert (g_error_matches (error,
                             BTK_BUILDER_ERROR,
                             BTK_BUILDER_ERROR_INVALID_TYPE_FUNCTION));
  g_error_free (error);
  g_object_unref (builder);
}

static void
test_spin_button (void)
{
  BtkBuilder *builder;
  const bchar buffer[] =
    "<interface>"
    "<object class=\"BtkAdjustment\" id=\"adjustment1\">"
    "<property name=\"lower\">0</property>"
    "<property name=\"upper\">10</property>"
    "<property name=\"step-increment\">2</property>"
    "<property name=\"page-increment\">3</property>"
    "<property name=\"page-size\">0</property>"
    "<property name=\"value\">1</property>"
    "</object>"
    "<object class=\"BtkSpinButton\" id=\"spinbutton1\">"
    "<property name=\"visible\">True</property>"
    "<property name=\"adjustment\">adjustment1</property>"
    "</object>"
    "</interface>";
  BObject *obj;
  BtkAdjustment *adjustment;
  bdouble value;
  
  builder = builder_new_from_string (buffer, -1, NULL);
  obj = btk_builder_get_object (builder, "spinbutton1");
  g_assert (BTK_IS_SPIN_BUTTON (obj));
  adjustment = btk_spin_button_get_adjustment (BTK_SPIN_BUTTON (obj));
  g_assert (BTK_IS_ADJUSTMENT (adjustment));
  g_object_get (adjustment, "value", &value, NULL);
  g_assert (value == 1);
  g_object_get (adjustment, "lower", &value, NULL);
  g_assert (value == 0);
  g_object_get (adjustment, "upper", &value, NULL);
  g_assert (value == 10);
  g_object_get (adjustment, "step-increment", &value, NULL);
  g_assert (value == 2);
  g_object_get (adjustment, "page-increment", &value, NULL);
  g_assert (value == 3);
  g_object_get (adjustment, "page-size", &value, NULL);
  g_assert (value == 0);
  
  g_object_unref (builder);
}

static void
test_notebook (void)
{
  BtkBuilder *builder;
  const bchar buffer[] =
    "<interface>"
    "  <object class=\"BtkNotebook\" id=\"notebook1\">"
    "    <child>"
    "      <object class=\"BtkLabel\" id=\"label1\">"
    "        <property name=\"label\">label1</property>"
    "      </object>"
    "    </child>"
    "    <child type=\"tab\">"
    "      <object class=\"BtkLabel\" id=\"tablabel1\">"
    "        <property name=\"label\">tab_label1</property>"
    "      </object>"
    "    </child>"
    "    <child>"
    "      <object class=\"BtkLabel\" id=\"label2\">"
    "        <property name=\"label\">label2</property>"
    "      </object>"
    "    </child>"
    "    <child type=\"tab\">"
    "      <object class=\"BtkLabel\" id=\"tablabel2\">"
    "        <property name=\"label\">tab_label2</property>"
    "      </object>"
    "    </child>"
    "  </object>"
    "</interface>";
  BObject *notebook;
  BtkWidget *label;

  builder = builder_new_from_string (buffer, -1, NULL);
  notebook = btk_builder_get_object (builder, "notebook1");
  g_assert (notebook != NULL);
  g_assert (btk_notebook_get_n_pages (BTK_NOTEBOOK (notebook)) == 2);

  label = btk_notebook_get_nth_page (BTK_NOTEBOOK (notebook), 0);
  g_assert (BTK_IS_LABEL (label));
  g_assert (strcmp (btk_label_get_label (BTK_LABEL (label)), "label1") == 0);
  label = btk_notebook_get_tab_label (BTK_NOTEBOOK (notebook), label);
  g_assert (BTK_IS_LABEL (label));
  g_assert (strcmp (btk_label_get_label (BTK_LABEL (label)), "tab_label1") == 0);

  label = btk_notebook_get_nth_page (BTK_NOTEBOOK (notebook), 1);
  g_assert (BTK_IS_LABEL (label));
  g_assert (strcmp (btk_label_get_label (BTK_LABEL (label)), "label2") == 0);
  label = btk_notebook_get_tab_label (BTK_NOTEBOOK (notebook), label);
  g_assert (BTK_IS_LABEL (label));
  g_assert (strcmp (btk_label_get_label (BTK_LABEL (label)), "tab_label2") == 0);

  g_object_unref (builder);
}

static void
test_construct_only_property (void)
{
  BtkBuilder *builder;
  const bchar buffer[] =
    "<interface>"
    "  <object class=\"BtkWindow\" id=\"window1\">"
    "    <property name=\"type\">BTK_WINDOW_POPUP</property>"
    "  </object>"
    "</interface>";
  const bchar buffer2[] =
    "<interface>"
    "  <object class=\"BtkTextTagTable\" id=\"tagtable1\"/>"
    "  <object class=\"BtkTextBuffer\" id=\"textbuffer1\">"
    "    <property name=\"tag-table\">tagtable1</property>"
    "  </object>"
    "</interface>";
  BObject *widget, *tagtable, *textbuffer;
  BtkWindowType type;
  
  builder = builder_new_from_string (buffer, -1, NULL);
  widget = btk_builder_get_object (builder, "window1");
  g_object_get (widget, "type", &type, NULL);
  g_assert (type == BTK_WINDOW_POPUP);

  btk_widget_destroy (BTK_WIDGET (widget));
  g_object_unref (builder);

  builder = builder_new_from_string (buffer2, -1, NULL);
  textbuffer = btk_builder_get_object (builder, "textbuffer1");
  g_assert (textbuffer != NULL);
  g_object_get (textbuffer, "tag-table", &tagtable, NULL);
  g_assert (tagtable == btk_builder_get_object (builder, "tagtable1"));
  g_object_unref (tagtable);
  g_object_unref (builder);
}

static void
test_object_properties (void)
{
  BtkBuilder *builder;
  const bchar buffer[] =
    "<interface>"
    "  <object class=\"BtkWindow\" id=\"window1\">"
    "    <child>"
    "      <object class=\"BtkVBox\" id=\"vbox\">"
    "        <property name=\"border-width\">10</property>"
    "        <child>"
    "          <object class=\"BtkLabel\" id=\"label1\">"
    "            <property name=\"mnemonic-widget\">spinbutton1</property>"
    "          </object>"
    "        </child>"
    "        <child>"
    "          <object class=\"BtkSpinButton\" id=\"spinbutton1\"/>"
    "        </child>"
    "      </object>"
    "    </child>"
    "  </object>"
    "</interface>";
  const bchar buffer2[] =
    "<interface>"
    "  <object class=\"BtkWindow\" id=\"window2\"/>"
    "</interface>";
  BObject *label, *spinbutton, *window;
  
  builder = builder_new_from_string (buffer, -1, NULL);
  label = btk_builder_get_object (builder, "label1");
  g_assert (label != NULL);
  spinbutton = btk_builder_get_object (builder, "spinbutton1");
  g_assert (spinbutton != NULL);
  g_assert (spinbutton == (BObject*)btk_label_get_mnemonic_widget (BTK_LABEL (label)));

  btk_builder_add_from_string (builder, buffer2, -1, NULL);
  window = btk_builder_get_object (builder, "window2");
  g_assert (window != NULL);
  btk_widget_destroy (BTK_WIDGET (window));

  g_object_unref (builder);
}

static void
test_children (void)
{
  BtkBuilder * builder;
  GList *children;
  const bchar buffer1[] =
    "<interface>"
    "  <object class=\"BtkWindow\" id=\"window1\">"
    "    <child>"
    "      <object class=\"BtkButton\" id=\"button1\">"
    "        <property name=\"label\">Hello</property>"
    "      </object>"
    "    </child>"
    "  </object>"
    "</interface>";
  const bchar buffer2[] =
    "<interface>"
    "  <object class=\"BtkDialog\" id=\"dialog1\">"
    "    <child internal-child=\"vbox\">"
    "      <object class=\"BtkVBox\" id=\"dialog1-vbox\">"
    "        <property name=\"border-width\">10</property>"
    "          <child internal-child=\"action_area\">"
    "            <object class=\"BtkHButtonBox\" id=\"dialog1-action_area\">"
    "              <property name=\"border-width\">20</property>"
    "            </object>"
    "          </child>"
    "      </object>"
    "    </child>"
    "  </object>"
    "</interface>";

  BObject *window, *button;
  BObject *dialog, *vbox, *action_area;
  
  builder = builder_new_from_string (buffer1, -1, NULL);
  window = btk_builder_get_object (builder, "window1");
  g_assert (window != NULL);
  g_assert (BTK_IS_WINDOW (window));

  button = btk_builder_get_object (builder, "button1");
  g_assert (button != NULL);
  g_assert (BTK_IS_BUTTON (button));
  g_assert (BTK_WIDGET(button)->parent != NULL);
  g_assert (strcmp (btk_buildable_get_name (BTK_BUILDABLE (BTK_WIDGET (button)->parent)), "window1") == 0);

  btk_widget_destroy (BTK_WIDGET (window));
  g_object_unref (builder);
  
  builder = builder_new_from_string (buffer2, -1, NULL);
  dialog = btk_builder_get_object (builder, "dialog1");
  g_assert (dialog != NULL);
  g_assert (BTK_IS_DIALOG (dialog));
  children = btk_container_get_children (BTK_CONTAINER (dialog));
  g_assert (g_list_length (children) == 1);
  g_list_free (children);
  
  vbox = btk_builder_get_object (builder, "dialog1-vbox");
  g_assert (vbox != NULL);
  g_assert (BTK_IS_VBOX (vbox));
  g_assert (strcmp (btk_buildable_get_name (BTK_BUILDABLE (BTK_WIDGET (vbox)->parent)), "dialog1") == 0);
  g_assert (BTK_CONTAINER (vbox)->border_width == 10);
  g_assert (strcmp (btk_buildable_get_name (BTK_BUILDABLE (BTK_DIALOG (dialog)->vbox)), "dialog1-vbox") == 0);

  action_area = btk_builder_get_object (builder, "dialog1-action_area");
  g_assert (action_area != NULL);
  g_assert (BTK_IS_HBUTTON_BOX (action_area));
  g_assert (BTK_WIDGET (action_area)->parent != NULL);
  g_assert (BTK_CONTAINER (action_area)->border_width == 20);
  g_assert (BTK_DIALOG (dialog)->action_area != NULL);
  g_assert (btk_buildable_get_name (BTK_BUILDABLE (BTK_DIALOG (dialog)->action_area)) != NULL);
  g_assert (strcmp (btk_buildable_get_name (BTK_BUILDABLE (BTK_DIALOG (dialog)->action_area)), "dialog1-action_area") == 0);
  btk_widget_destroy (BTK_WIDGET (dialog));
  g_object_unref (builder);
}

static void
test_child_properties (void)
{
  BtkBuilder * builder;
  const bchar buffer1[] =
    "<interface>"
    "  <object class=\"BtkVBox\" id=\"vbox1\">"
    "    <child>"
    "      <object class=\"BtkLabel\" id=\"label1\"/>"
    "      <packing>"
    "        <property name=\"pack-type\">start</property>"
    "      </packing>"
    "    </child>"
    "    <child>"
    "      <object class=\"BtkLabel\" id=\"label2\"/>"
    "      <packing>"
    "        <property name=\"pack-type\">end</property>"
    "      </packing>"
    "    </child>"
    "  </object>"
    "</interface>";

  BObject *label, *vbox;
  BtkPackType pack_type;
  
  builder = builder_new_from_string (buffer1, -1, NULL);
  vbox = btk_builder_get_object (builder, "vbox1");
  g_assert (BTK_IS_VBOX (vbox));

  label = btk_builder_get_object (builder, "label1");
  g_assert (BTK_IS_LABEL (label));
  btk_container_child_get (BTK_CONTAINER (vbox),
                           BTK_WIDGET (label),
                           "pack-type",
                           &pack_type,
                           NULL);
  g_assert (pack_type == BTK_PACK_START);
  
  label = btk_builder_get_object (builder, "label2");
  g_assert (BTK_IS_LABEL (label));
  btk_container_child_get (BTK_CONTAINER (vbox),
                           BTK_WIDGET (label),
                           "pack-type",
                           &pack_type,
                           NULL);
  g_assert (pack_type == BTK_PACK_END);

  g_object_unref (builder);
}

static void
test_treeview_column (void)
{
  BtkBuilder *builder;
  const bchar buffer[] =
    "<interface>"
    "<object class=\"BtkListStore\" id=\"liststore1\">"
    "  <columns>"
    "    <column type=\"bchararray\"/>"
    "    <column type=\"buint\"/>"
    "  </columns>"
    "  <data>"
    "    <row>"
    "      <col id=\"0\">John</col>"
    "      <col id=\"1\">25</col>"
    "    </row>"
    "  </data>"
    "</object>"
    "<object class=\"BtkWindow\" id=\"window1\">"
    "  <child>"
    "    <object class=\"BtkTreeView\" id=\"treeview1\">"
    "      <property name=\"visible\">True</property>"
    "      <property name=\"model\">liststore1</property>"
    "      <child>"
    "        <object class=\"BtkTreeViewColumn\" id=\"column1\">"
    "          <property name=\"title\">Test</property>"
    "          <child>"
    "            <object class=\"BtkCellRendererText\" id=\"renderer1\"/>"
    "            <attributes>"
    "              <attribute name=\"text\">1</attribute>"
    "            </attributes>"
    "          </child>"
    "        </object>"
    "      </child>"
    "      <child>"
    "        <object class=\"BtkTreeViewColumn\" id=\"column2\">"
    "          <property name=\"title\">Number</property>"
    "          <child>"
    "            <object class=\"BtkCellRendererText\" id=\"renderer2\"/>"
    "            <attributes>"
    "              <attribute name=\"text\">0</attribute>"
    "            </attributes>"
    "          </child>"
    "        </object>"
    "      </child>"
    "    </object>"
    "  </child>"
    "</object>"
    "</interface>";
  BObject *window, *treeview;
  BtkTreeViewColumn *column;
  GList *renderers;
  BObject *renderer;
  bchar *text;

  builder = builder_new_from_string (buffer, -1, NULL);
  treeview = btk_builder_get_object (builder, "treeview1");
  g_assert (treeview);
  g_assert (BTK_IS_TREE_VIEW (treeview));
  column = btk_tree_view_get_column (BTK_TREE_VIEW (treeview), 0);
  g_assert (BTK_IS_TREE_VIEW_COLUMN (column));
  g_assert (strcmp (btk_tree_view_column_get_title (column), "Test") == 0);

  renderers = btk_cell_layout_get_cells (BTK_CELL_LAYOUT (column));
  g_assert (g_list_length (renderers) == 1);
  renderer = g_list_nth_data (renderers, 0);
  g_assert (renderer);
  g_assert (BTK_IS_CELL_RENDERER_TEXT (renderer));
  g_list_free (renderers);

  btk_widget_realize (BTK_WIDGET (treeview));

  renderer = btk_builder_get_object (builder, "renderer1");
  g_object_get (renderer, "text", &text, NULL);
  g_assert (text);
  g_assert (strcmp (text, "25") == 0);
  g_free (text);
  
  renderer = btk_builder_get_object (builder, "renderer2");
  g_object_get (renderer, "text", &text, NULL);
  g_assert (text);
  g_assert (strcmp (text, "John") == 0);
  g_free (text);

  btk_widget_unrealize (BTK_WIDGET (treeview));

  window = btk_builder_get_object (builder, "window1");
  btk_widget_destroy (BTK_WIDGET (window));
  
  g_object_unref (builder);
}

static void
test_icon_view (void)
{
  BtkBuilder *builder;
  const bchar buffer[] =
    "<interface>"
    "  <object class=\"BtkListStore\" id=\"liststore1\">"
    "    <columns>"
    "      <column type=\"bchararray\"/>"
    "      <column type=\"BdkPixbuf\"/>"
    "    </columns>"
    "    <data>"
    "      <row>"
    "        <col id=\"0\">test</col>"
    "      </row>"
    "    </data>"
    "  </object>"
    "  <object class=\"BtkWindow\" id=\"window1\">"
    "    <child>"
    "      <object class=\"BtkIconView\" id=\"iconview1\">"
    "        <property name=\"model\">liststore1</property>"
    "        <property name=\"text-column\">0</property>"
    "        <property name=\"pixbuf-column\">1</property>"
    "        <property name=\"visible\">True</property>"
    "        <child>"
    "          <object class=\"BtkCellRendererText\" id=\"renderer1\"/>"
    "          <attributes>"
    "            <attribute name=\"text\">0</attribute>"
    "          </attributes>"
    "        </child>"
    "      </object>"
    "    </child>"
    "  </object>"
    "</interface>";
  BObject *window, *iconview, *renderer;
  bchar *text;
  
  builder = builder_new_from_string (buffer, -1, NULL);
  iconview = btk_builder_get_object (builder, "iconview1");
  g_assert (iconview);
  g_assert (BTK_IS_ICON_VIEW (iconview));

  btk_widget_realize (BTK_WIDGET (iconview));

  renderer = btk_builder_get_object (builder, "renderer1");
  g_object_get (renderer, "text", &text, NULL);
  g_assert (text);
  g_assert (strcmp (text, "test") == 0);
  g_free (text);
    
  window = btk_builder_get_object (builder, "window1");
  btk_widget_destroy (BTK_WIDGET (window));
  g_object_unref (builder);
}

static void
test_combo_box (void)
{
  BtkBuilder *builder;
  const bchar buffer[] =
    "<interface>"
    "  <object class=\"BtkListStore\" id=\"liststore1\">"
    "    <columns>"
    "      <column type=\"buint\"/>"
    "      <column type=\"bchararray\"/>"
    "    </columns>"
    "    <data>"
    "      <row>"
    "        <col id=\"0\">1</col>"
    "        <col id=\"1\">Foo</col>"
    "      </row>"
    "      <row>"
    "        <col id=\"0\">2</col>"
    "        <col id=\"1\">Bar</col>"
    "      </row>"
    "    </data>"
    "  </object>"
    "  <object class=\"BtkWindow\" id=\"window1\">"
    "    <child>"
    "      <object class=\"BtkComboBox\" id=\"combobox1\">"
    "        <property name=\"model\">liststore1</property>"
    "        <property name=\"visible\">True</property>"
    "        <child>"
    "          <object class=\"BtkCellRendererText\" id=\"renderer1\"/>"
    "          <attributes>"
    "            <attribute name=\"text\">0</attribute>"
    "          </attributes>"
    "        </child>"
    "        <child>"
    "          <object class=\"BtkCellRendererText\" id=\"renderer2\"/>"
    "          <attributes>"
    "            <attribute name=\"text\">1</attribute>"
    "          </attributes>"
    "        </child>"
    "      </object>"
    "    </child>"
    "  </object>"
    "</interface>";
  BObject *window, *combobox, *renderer;
  bchar *text;

  builder = builder_new_from_string (buffer, -1, NULL);
  combobox = btk_builder_get_object (builder, "combobox1");
  g_assert (combobox);
  btk_widget_realize (BTK_WIDGET (combobox));

  renderer = btk_builder_get_object (builder, "renderer2");
  g_assert (renderer);
  g_object_get (renderer, "text", &text, NULL);
  g_assert (text);
  g_assert (strcmp (text, "Bar") == 0);
  g_free (text);

  renderer = btk_builder_get_object (builder, "renderer1");
  g_assert (renderer);
  g_object_get (renderer, "text", &text, NULL);
  g_assert (text);
  g_assert (strcmp (text, "2") == 0);
  g_free (text);

  window = btk_builder_get_object (builder, "window1");
  btk_widget_destroy (BTK_WIDGET (window));

  g_object_unref (builder);
}

#if 0
static void
test_combo_box_entry (void)
{
  BtkBuilder *builder;
  const bchar buffer[] =
    "<interface>"
    "  <object class=\"BtkListStore\" id=\"liststore1\">"
    "    <columns>"
    "      <column type=\"buint\"/>"
    "      <column type=\"bchararray\"/>"
    "    </columns>"
    "    <data>"
    "      <row>"
    "        <col id=\"0\">1</col>"
    "        <col id=\"1\">Foo</col>"
    "      </row>"
    "      <row>"
    "        <col id=\"0\">2</col>"
    "        <col id=\"1\">Bar</col>"
    "      </row>"
    "    </data>"
    "  </object>"
    "  <object class=\"BtkWindow\" id=\"window1\">"
    "    <child>"
    "      <object class=\"BtkComboBox\" id=\"comboboxentry1\">"
    "        <property name=\"model\">liststore1</property>"
    "        <property name=\"has-entry\">True</property>"
    "        <property name=\"visible\">True</property>"
    "        <child>"
    "          <object class=\"BtkCellRendererText\" id=\"renderer1\"/>"
    "            <attributes>"
    "              <attribute name=\"text\">0</attribute>"
    "            </attributes>"
    "        </child>"
    "        <child>"
    "          <object class=\"BtkCellRendererText\" id=\"renderer2\"/>"
    "            <attributes>"
    "              <attribute name=\"text\">1</attribute>"
    "            </attributes>"
    "        </child>"
    "      </object>"
    "    </child>"
    "  </object>"
    "</interface>";
  BObject *window, *combobox, *renderer;
  bchar *text;

  builder = builder_new_from_string (buffer, -1, NULL);
  combobox = btk_builder_get_object (builder, "comboboxentry1");
  g_assert (combobox);

  renderer = btk_builder_get_object (builder, "renderer2");
  g_assert (renderer);
  g_object_get (renderer, "text", &text, NULL);
  g_assert (text);
  g_assert (strcmp (text, "Bar") == 0);
  g_free (text);

  renderer = btk_builder_get_object (builder, "renderer1");
  g_assert (renderer);
  g_object_get (renderer, "text", &text, NULL);
  g_assert (text);
  g_assert (strcmp (text, "2") == 0);
  g_free (text);

  window = btk_builder_get_object (builder, "window1");
  btk_widget_destroy (BTK_WIDGET (window));

  g_object_unref (builder);
}
#endif

static void
test_cell_view (void)
{
  BtkBuilder *builder;
  const bchar *buffer =
    "<interface>"
    "  <object class=\"BtkListStore\" id=\"liststore1\">"
    "    <columns>"
    "      <column type=\"bchararray\"/>"
    "    </columns>"
    "    <data>"
    "      <row>"
    "        <col id=\"0\">test</col>"
    "      </row>"
    "    </data>"
    "  </object>"
    "  <object class=\"BtkWindow\" id=\"window1\">"
    "    <child>"
    "      <object class=\"BtkCellView\" id=\"cellview1\">"
    "        <property name=\"visible\">True</property>"
    "        <property name=\"model\">liststore1</property>"
    "        <accelerator key=\"f\" modifiers=\"BDK_CONTROL_MASK\" signal=\"grab_focus\"/>"
    "        <child>"
    "          <object class=\"BtkCellRendererText\" id=\"renderer1\"/>"
    "          <attributes>"
    "            <attribute name=\"text\">0</attribute>"
    "          </attributes>"
    "        </child>"
    "      </object>"
    "    </child>"
    "  </object>"
    "</interface>";
  BObject *cellview;
  BObject *model, *window;
  BtkTreePath *path;
  GList *renderers;
  BObject *renderer;
  bchar *text;
  
  builder = builder_new_from_string (buffer, -1, NULL);
  cellview = btk_builder_get_object (builder, "cellview1");
  g_assert (builder);
  g_assert (cellview);
  g_assert (BTK_IS_CELL_VIEW (cellview));
  g_object_get (cellview, "model", &model, NULL);
  g_assert (model);
  g_assert (BTK_IS_TREE_MODEL (model));
  g_object_unref (model);
  path = btk_tree_path_new_first ();
  btk_cell_view_set_displayed_row (BTK_CELL_VIEW (cellview), path);
  
  renderers = btk_cell_layout_get_cells (BTK_CELL_LAYOUT (cellview));
  g_assert (renderers);
  g_assert (g_list_length (renderers) == 1);
  
  btk_widget_realize (BTK_WIDGET (cellview));

  renderer = g_list_nth_data (renderers, 0);
  g_list_free (renderers);
  g_assert (renderer);
  g_object_get (renderer, "text", &text, NULL);
  g_assert (strcmp (text, "test") == 0);
  g_free (text);
  btk_tree_path_free (path);

  window = btk_builder_get_object (builder, "window1");
  btk_widget_destroy (BTK_WIDGET (window));
  
  g_object_unref (builder);
}

static void
test_dialog (void)
{
  BtkBuilder * builder;
  const bchar buffer1[] =
    "<interface>"
    "  <object class=\"BtkDialog\" id=\"dialog1\">"
    "    <child internal-child=\"vbox\">"
    "      <object class=\"BtkVBox\" id=\"dialog1-vbox\">"
    "          <child internal-child=\"action_area\">"
    "            <object class=\"BtkHButtonBox\" id=\"dialog1-action_area\">"
    "              <child>"
    "                <object class=\"BtkButton\" id=\"button_cancel\"/>"
    "              </child>"
    "              <child>"
    "                <object class=\"BtkButton\" id=\"button_ok\"/>"
    "              </child>"
    "            </object>"
    "          </child>"
    "      </object>"
    "    </child>"
    "    <action-widgets>"
    "      <action-widget response=\"3\">button_ok</action-widget>"
    "      <action-widget response=\"-5\">button_cancel</action-widget>"
    "    </action-widgets>"
    "  </object>"
    "</interface>";

  BObject *dialog1;
  BObject *button_ok;
  BObject *button_cancel;
  
  builder = builder_new_from_string (buffer1, -1, NULL);
  dialog1 = btk_builder_get_object (builder, "dialog1");
  button_ok = btk_builder_get_object (builder, "button_ok");
  g_assert (btk_dialog_get_response_for_widget (BTK_DIALOG (dialog1), BTK_WIDGET (button_ok)) == 3);
  button_cancel = btk_builder_get_object (builder, "button_cancel");
  g_assert (btk_dialog_get_response_for_widget (BTK_DIALOG (dialog1), BTK_WIDGET (button_cancel)) == -5);
  
  btk_widget_destroy (BTK_WIDGET (dialog1));
  g_object_unref (builder);
}

static void
test_message_dialog (void)
{
  BtkBuilder * builder;
  const bchar buffer1[] =
    "<interface>"
    "  <object class=\"BtkMessageDialog\" id=\"dialog1\">"
    "    <child internal-child=\"message_area\">"
    "      <object class=\"BtkVBox\" id=\"dialog-message-area\">"
    "        <child>"
    "          <object class=\"BtkExpander\" id=\"expander\"/>"
    "        </child>"
    "      </object>"
    "    </child>"
    "  </object>"
    "</interface>";

  BObject *dialog1;
  BObject *expander;

  builder = builder_new_from_string (buffer1, -1, NULL);
  dialog1 = btk_builder_get_object (builder, "dialog1");
  expander = btk_builder_get_object (builder, "expander");
  g_assert (BTK_IS_EXPANDER (expander));
  g_assert (btk_widget_get_parent (BTK_WIDGET (expander)) == btk_message_dialog_get_message_area (BTK_MESSAGE_DIALOG (dialog1)));

  btk_widget_destroy (BTK_WIDGET (dialog1));
  g_object_unref (builder);
}

static void
test_accelerators (void)
{
  BtkBuilder *builder;
  const bchar *buffer =
    "<interface>"
    "  <object class=\"BtkWindow\" id=\"window1\">"
    "    <child>"
    "      <object class=\"BtkButton\" id=\"button1\">"
    "        <accelerator key=\"q\" modifiers=\"BDK_CONTROL_MASK\" signal=\"clicked\"/>"
    "      </object>"
    "    </child>"
    "  </object>"
    "</interface>";
  const bchar *buffer2 =
    "<interface>"
    "  <object class=\"BtkWindow\" id=\"window1\">"
    "    <child>"
    "      <object class=\"BtkTreeView\" id=\"treeview1\">"
    "        <signal name=\"cursor-changed\" handler=\"btk_main_quit\"/>"
    "        <accelerator key=\"f\" modifiers=\"BDK_CONTROL_MASK\" signal=\"grab_focus\"/>"
    "      </object>"
    "    </child>"
    "  </object>"
    "</interface>";
  BObject *window1;
  GSList *accel_groups;
  BObject *accel_group;
  
  builder = builder_new_from_string (buffer, -1, NULL);
  window1 = btk_builder_get_object (builder, "window1");
  g_assert (window1);
  g_assert (BTK_IS_WINDOW (window1));

  accel_groups = btk_accel_groups_from_object (window1);
  g_assert (b_slist_length (accel_groups) == 1);
  accel_group = b_slist_nth_data (accel_groups, 0);
  g_assert (accel_group);

  btk_widget_destroy (BTK_WIDGET (window1));
  g_object_unref (builder);

  builder = builder_new_from_string (buffer2, -1, NULL);
  window1 = btk_builder_get_object (builder, "window1");
  g_assert (window1);
  g_assert (BTK_IS_WINDOW (window1));

  accel_groups = btk_accel_groups_from_object (window1);
  g_assert (b_slist_length (accel_groups) == 1);
  accel_group = b_slist_nth_data (accel_groups, 0);
  g_assert (accel_group);

  btk_widget_destroy (BTK_WIDGET (window1));
  g_object_unref (builder);
}

static void
test_widget (void)
{
  const bchar *buffer =
    "<interface>"
    "  <object class=\"BtkWindow\" id=\"window1\">"
    "    <child>"
    "      <object class=\"BtkButton\" id=\"button1\">"
    "         <property name=\"can-focus\">True</property>"
    "         <property name=\"has-focus\">True</property>"
    "      </object>"
    "    </child>"
    "  </object>"
   "</interface>";
  const bchar *buffer2 =
    "<interface>"
    "  <object class=\"BtkWindow\" id=\"window1\">"
    "    <child>"
    "      <object class=\"BtkButton\" id=\"button1\">"
    "         <property name=\"can-default\">True</property>"
    "         <property name=\"has-default\">True</property>"
    "      </object>"
    "    </child>"
    "  </object>"
   "</interface>";
  const bchar *buffer3 =
    "<interface>"
    "  <object class=\"BtkWindow\" id=\"window1\">"
    "    <child>"
    "      <object class=\"BtkVBox\" id=\"vbox1\">"
    "        <child>"
    "          <object class=\"BtkLabel\" id=\"label1\">"
    "            <child internal-child=\"accessible\">"
    "              <object class=\"BatkObject\" id=\"a11y-label1\">"
    "                <property name=\"BatkObject::accessible-name\">A Label</property>"
    "              </object>"
    "            </child>"
    "            <accessibility>"
    "              <relation target=\"button1\" type=\"label-for\"/>"
    "            </accessibility>"
    "          </object>"
    "        </child>"
    "        <child>"
    "          <object class=\"BtkButton\" id=\"button1\">"
    "            <accessibility>"
    "              <action action_name=\"click\" description=\"Sliff\"/>"
    "              <action action_name=\"clack\" translatable=\"yes\">Sniff</action>"
    "            </accessibility>"
    "          </object>"
    "        </child>"
    "      </object>"
    "    </child>"
    "  </object>"
    "</interface>";
  BtkBuilder *builder;
  BObject *window1, *button1, *label1;
  BatkObject *accessible;
  BatkRelationSet *relation_set;
  BatkRelation *relation;
  char *name;
  
  builder = builder_new_from_string (buffer, -1, NULL);
  button1 = btk_builder_get_object (builder, "button1");

#if 0
  g_assert (btk_widget_has_focus (BTK_WIDGET (button1)));
#endif
  window1 = btk_builder_get_object (builder, "window1");
  btk_widget_destroy (BTK_WIDGET (window1));
  
  g_object_unref (builder);
  
  builder = builder_new_from_string (buffer2, -1, NULL);
  button1 = btk_builder_get_object (builder, "button1");

  g_assert (btk_widget_get_receives_default (BTK_WIDGET (button1)));
  
  g_object_unref (builder);
  
  builder = builder_new_from_string (buffer3, -1, NULL);

  window1 = btk_builder_get_object (builder, "window1");
  label1 = btk_builder_get_object (builder, "label1");

  accessible = btk_widget_get_accessible (BTK_WIDGET (label1));
  relation_set = batk_object_ref_relation_set (accessible);
  g_return_if_fail (batk_relation_set_get_n_relations (relation_set) == 1);
  relation = batk_relation_set_get_relation (relation_set, 0);
  g_return_if_fail (relation != NULL);
  g_return_if_fail (BATK_IS_RELATION (relation));
  g_return_if_fail (batk_relation_get_relation_type (relation) != BATK_RELATION_LABELLED_BY);
  g_object_unref (relation_set);

  g_object_get (B_OBJECT (accessible), "accessible-name", &name, NULL);
  g_return_if_fail (strcmp (name, "A Label") == 0);
  g_free (name);
  
  btk_widget_destroy (BTK_WIDGET (window1));
  g_object_unref (builder);
}

static void
test_window (void)
{
  const bchar *buffer1 =
    "<interface>"
    "  <object class=\"BtkWindow\" id=\"window1\">"
    "     <property name=\"title\"></property>"
    "  </object>"
   "</interface>";
  const bchar *buffer2 =
    "<interface>"
    "  <object class=\"BtkWindow\" id=\"window1\">"
    "  </object>"
   "</interface>";
  BtkBuilder *builder;
  BObject *window1;
  bchar *title;
  
  builder = builder_new_from_string (buffer1, -1, NULL);
  window1 = btk_builder_get_object (builder, "window1");
  g_object_get (window1, "title", &title, NULL);
  g_assert (strcmp (title, "") == 0);
  g_free (title);
  btk_widget_destroy (BTK_WIDGET (window1));
  g_object_unref (builder);

  builder = builder_new_from_string (buffer2, -1, NULL);
  window1 = btk_builder_get_object (builder, "window1");
  btk_widget_destroy (BTK_WIDGET (window1));
  g_object_unref (builder);
}

static void
test_value_from_string (void)
{
  BValue value = { 0 };
  GError *error = NULL;
  BtkBuilder *builder;

  builder = btk_builder_new ();
  
  g_assert (btk_builder_value_from_string_type (builder, B_TYPE_STRING, "test", &value, &error));
  g_assert (G_VALUE_HOLDS_STRING (&value));
  g_assert (strcmp (b_value_get_string (&value), "test") == 0);
  b_value_unset (&value);

  g_assert (btk_builder_value_from_string_type (builder, B_TYPE_BOOLEAN, "true", &value, &error));
  g_assert (G_VALUE_HOLDS_BOOLEAN (&value));
  g_assert (b_value_get_boolean (&value) == TRUE);
  b_value_unset (&value);

  g_assert (btk_builder_value_from_string_type (builder, B_TYPE_BOOLEAN, "false", &value, &error));
  g_assert (G_VALUE_HOLDS_BOOLEAN (&value));
  g_assert (b_value_get_boolean (&value) == FALSE);
  b_value_unset (&value);

  g_assert (btk_builder_value_from_string_type (builder, B_TYPE_BOOLEAN, "yes", &value, &error));
  g_assert (G_VALUE_HOLDS_BOOLEAN (&value));
  g_assert (b_value_get_boolean (&value) == TRUE);
  b_value_unset (&value);

  g_assert (btk_builder_value_from_string_type (builder, B_TYPE_BOOLEAN, "no", &value, &error));
  g_assert (G_VALUE_HOLDS_BOOLEAN (&value));
  g_assert (b_value_get_boolean (&value) == FALSE);
  b_value_unset (&value);

  g_assert (btk_builder_value_from_string_type (builder, B_TYPE_BOOLEAN, "0", &value, &error));
  g_assert (G_VALUE_HOLDS_BOOLEAN (&value));
  g_assert (b_value_get_boolean (&value) == FALSE);
  b_value_unset (&value);

  g_assert (btk_builder_value_from_string_type (builder, B_TYPE_BOOLEAN, "1", &value, &error));
  g_assert (G_VALUE_HOLDS_BOOLEAN (&value));
  g_assert (b_value_get_boolean (&value) == TRUE);
  b_value_unset (&value);

  g_assert (btk_builder_value_from_string_type (builder, B_TYPE_BOOLEAN, "tRuE", &value, &error));
  g_assert (G_VALUE_HOLDS_BOOLEAN (&value));
  g_assert (b_value_get_boolean (&value) == TRUE);
  b_value_unset (&value);
  
  g_assert (btk_builder_value_from_string_type (builder, B_TYPE_BOOLEAN, "blaurgh", &value, &error) == FALSE);
  b_value_unset (&value);
  g_assert (g_error_matches (error,
                             BTK_BUILDER_ERROR,
                             BTK_BUILDER_ERROR_INVALID_VALUE));
  g_error_free (error);
  error = NULL;

  g_assert (btk_builder_value_from_string_type (builder, B_TYPE_BOOLEAN, "yess", &value, &error) == FALSE);
  b_value_unset (&value);
  g_assert (g_error_matches (error,
                             BTK_BUILDER_ERROR,
                             BTK_BUILDER_ERROR_INVALID_VALUE));
  g_error_free (error);
  error = NULL;
  
  g_assert (btk_builder_value_from_string_type (builder, B_TYPE_BOOLEAN, "trueee", &value, &error) == FALSE);
  b_value_unset (&value);
  g_assert (g_error_matches (error,
                             BTK_BUILDER_ERROR,
                             BTK_BUILDER_ERROR_INVALID_VALUE));
  g_error_free (error);
  error = NULL;
  
  g_assert (btk_builder_value_from_string_type (builder, B_TYPE_BOOLEAN, "", &value, &error) == FALSE);
  b_value_unset (&value);
  g_assert (g_error_matches (error,
                             BTK_BUILDER_ERROR,
                             BTK_BUILDER_ERROR_INVALID_VALUE));
  g_error_free (error);
  error = NULL;
  
  g_assert (btk_builder_value_from_string_type (builder, B_TYPE_INT, "12345", &value, &error));
  g_assert (G_VALUE_HOLDS_INT (&value));
  g_assert (b_value_get_int (&value) == 12345);
  b_value_unset (&value);

  g_assert (btk_builder_value_from_string_type (builder, B_TYPE_LONG, "9912345", &value, &error));
  g_assert (G_VALUE_HOLDS_LONG (&value));
  g_assert (b_value_get_long (&value) == 9912345);
  b_value_unset (&value);

  g_assert (btk_builder_value_from_string_type (builder, B_TYPE_UINT, "2345", &value, &error));
  g_assert (G_VALUE_HOLDS_UINT (&value));
  g_assert (b_value_get_uint (&value) == 2345);
  b_value_unset (&value);

  g_assert (btk_builder_value_from_string_type (builder, B_TYPE_FLOAT, "1.454", &value, &error));
  g_assert (G_VALUE_HOLDS_FLOAT (&value));
  g_assert (fabs (b_value_get_float (&value) - 1.454) < 0.00001);
  b_value_unset (&value);

  g_assert (btk_builder_value_from_string_type (builder, B_TYPE_FLOAT, "abc", &value, &error) == FALSE);
  b_value_unset (&value);
  g_assert (g_error_matches (error,
                             BTK_BUILDER_ERROR,
                             BTK_BUILDER_ERROR_INVALID_VALUE));
  g_error_free (error);
  error = NULL;

  g_assert (btk_builder_value_from_string_type (builder, B_TYPE_INT, "/-+,abc", &value, &error) == FALSE);
  b_value_unset (&value);
  g_assert (g_error_matches (error,
                             BTK_BUILDER_ERROR,
                             BTK_BUILDER_ERROR_INVALID_VALUE));
  g_error_free (error);
  error = NULL;

  g_assert (btk_builder_value_from_string_type (builder, BTK_TYPE_WINDOW_TYPE, "toplevel", &value, &error) == TRUE);
  g_assert (G_VALUE_HOLDS_ENUM (&value));
  g_assert (b_value_get_enum (&value) == BTK_WINDOW_TOPLEVEL);
  b_value_unset (&value);

  g_assert (btk_builder_value_from_string_type (builder, BTK_TYPE_WINDOW_TYPE, "sliff", &value, &error) == FALSE);
  b_value_unset (&value);
  g_assert (g_error_matches (error,
                             BTK_BUILDER_ERROR,
                             BTK_BUILDER_ERROR_INVALID_VALUE));
  g_error_free (error);
  error = NULL;
  
  g_assert (btk_builder_value_from_string_type (builder, BTK_TYPE_WIDGET_FLAGS, "mapped", &value, &error) == TRUE);
  g_assert (G_VALUE_HOLDS_FLAGS (&value));
  g_assert (b_value_get_flags (&value) == BTK_MAPPED);
  b_value_unset (&value);

  g_assert (btk_builder_value_from_string_type (builder, BTK_TYPE_WIDGET_FLAGS, "BTK_VISIBLE | BTK_REALIZED", &value, &error) == TRUE);
  g_assert (G_VALUE_HOLDS_FLAGS (&value));
  g_assert (b_value_get_flags (&value) == (BTK_VISIBLE | BTK_REALIZED));
  b_value_unset (&value);
  
  g_assert (btk_builder_value_from_string_type (builder, BTK_TYPE_WINDOW_TYPE, "foobar", &value, &error) == FALSE);
  b_value_unset (&value);
  g_assert (g_error_matches (error,
                             BTK_BUILDER_ERROR,
                             BTK_BUILDER_ERROR_INVALID_VALUE));
  g_error_free (error);
  error = NULL;
  
  g_object_unref (builder);
}

static bboolean model_freed = FALSE;

static void
model_weakref (bpointer data,
               BObject *model)
{
  model_freed = TRUE;
}

static void
test_reference_counting (void)
{
  BtkBuilder *builder;
  const bchar buffer1[] =
    "<interface>"
    "  <object class=\"BtkListStore\" id=\"liststore1\"/>"
    "  <object class=\"BtkListStore\" id=\"liststore2\"/>"
    "  <object class=\"BtkWindow\" id=\"window1\">"
    "    <child>"
    "      <object class=\"BtkTreeView\" id=\"treeview1\">"
    "        <property name=\"model\">liststore1</property>"
    "      </object>"
    "    </child>"
    "  </object>"
    "</interface>";
  const bchar buffer2[] =
    "<interface>"
    "  <object class=\"BtkVBox\" id=\"vbox1\">"
    "    <child>"
    "      <object class=\"BtkLabel\" id=\"label1\"/>"
    "      <packing>"
    "        <property name=\"pack-type\">start</property>"
    "      </packing>"
    "    </child>"
    "  </object>"
    "</interface>";
  BObject *window, *treeview, *model;
  
  builder = builder_new_from_string (buffer1, -1, NULL);
  window = btk_builder_get_object (builder, "window1");
  treeview = btk_builder_get_object (builder, "treeview1");
  model = btk_builder_get_object (builder, "liststore1");
  g_object_unref (builder);

  g_object_weak_ref (model, (GWeakNotify)model_weakref, NULL);

  g_assert (model_freed == FALSE);
  btk_tree_view_set_model (BTK_TREE_VIEW (treeview), NULL);
  g_assert (model_freed == TRUE);
  
  btk_widget_destroy (BTK_WIDGET (window));

  builder = builder_new_from_string (buffer2, -1, NULL);
  g_object_unref (builder);
}

static void
test_icon_factory (void)
{
  BtkBuilder *builder;
  const bchar buffer1[] =
    "<interface>"
    "  <object class=\"BtkIconFactory\" id=\"iconfactory1\">"
    "    <sources>"
    "      <source stock-id=\"apple-red\" filename=\"apple-red.png\"/>"
    "    </sources>"
    "  </object>"
    "</interface>";
  const bchar buffer2[] =
    "<interface>"
    "  <object class=\"BtkIconFactory\" id=\"iconfactory1\">"
    "    <sources>"
    "      <source stock-id=\"sliff\" direction=\"rtl\" state=\"active\""
    "              size=\"menu\" filename=\"sloff.png\"/>"
    "      <source stock-id=\"sliff\" direction=\"ltr\" state=\"selected\""
    "              size=\"dnd\" filename=\"slurf.png\"/>"
    "    </sources>"
    "  </object>"
    "</interface>";
#if 0
  const bchar buffer3[] =
    "<interface>"
    "  <object class=\"BtkIconFactory\" id=\"iconfactory1\">"
    "    <invalid/>"
    "  </object>"
    "</interface>";
  const bchar buffer4[] =
    "<interface>"
    "  <object class=\"BtkIconFactory\" id=\"iconfactory1\">"
    "    <sources>"
    "      <invalid/>"
    "    </sources>"
    "  </object>"
    "</interface>";
  const bchar buffer5[] =
    "<interface>"
    "  <object class=\"BtkIconFactory\" id=\"iconfactory1\">"
    "    <sources>"
    "      <source/>"
    "    </sources>"
    "  </object>"
    "</interface>";
  GError *error = NULL;
#endif  
  BObject *factory;
  BtkIconSet *icon_set;
  BtkIconSource *icon_source;
  BtkWidget *image;
  
  builder = builder_new_from_string (buffer1, -1, NULL);
  factory = btk_builder_get_object (builder, "iconfactory1");
  g_assert (factory != NULL);

  icon_set = btk_icon_factory_lookup (BTK_ICON_FACTORY (factory), "apple-red");
  g_assert (icon_set != NULL);
  btk_icon_factory_add_default (BTK_ICON_FACTORY (factory));
  image = btk_image_new_from_stock ("apple-red", BTK_ICON_SIZE_BUTTON);
  g_assert (image != NULL);
  g_object_ref_sink (image);
  g_object_unref (image);

  g_object_unref (builder);

  builder = builder_new_from_string (buffer2, -1, NULL);
  factory = btk_builder_get_object (builder, "iconfactory1");
  g_assert (factory != NULL);

  icon_set = btk_icon_factory_lookup (BTK_ICON_FACTORY (factory), "sliff");
  g_assert (icon_set != NULL);
  g_assert (b_slist_length (icon_set->sources) == 2);

  icon_source = icon_set->sources->data;
  g_assert (btk_icon_source_get_direction (icon_source) == BTK_TEXT_DIR_RTL);
  g_assert (btk_icon_source_get_state (icon_source) == BTK_STATE_ACTIVE);
  g_assert (btk_icon_source_get_size (icon_source) == BTK_ICON_SIZE_MENU);
  g_assert (g_str_has_suffix (btk_icon_source_get_filename (icon_source), "sloff.png"));
  
  icon_source = icon_set->sources->next->data;
  g_assert (btk_icon_source_get_direction (icon_source) == BTK_TEXT_DIR_LTR);
  g_assert (btk_icon_source_get_state (icon_source) == BTK_STATE_SELECTED);
  g_assert (btk_icon_source_get_size (icon_source) == BTK_ICON_SIZE_DND);
  g_assert (g_str_has_suffix (btk_icon_source_get_filename (icon_source), "slurf.png"));

  g_object_unref (builder);

#if 0
  error = NULL;
  btk_builder_add_from_string (builder, buffer3, -1, &error);
  g_assert (g_error_matches (error,
                             BTK_BUILDER_ERROR,
                             BTK_BUILDER_ERROR_INVALID_TAG));
  g_error_free (error);

  error = NULL;
  btk_builder_add_from_string (builder, buffer4, -1, &error);
  g_assert (g_error_matches (error,
                             BTK_BUILDER_ERROR,
                             BTK_BUILDER_ERROR_INVALID_TAG));
  g_error_free (error);

  error = NULL;
  btk_builder_add_from_string (builder, buffer5, -1, &error);
  g_assert (g_error_matches (error,
                             BTK_BUILDER_ERROR,
                             BTK_BUILDER_ERROR_INVALID_ATTRIBUTE));
  g_error_free (error);
#endif

}

typedef struct {
  bboolean weight;
  bboolean foreground;
  bboolean underline;
  bboolean size;
  bboolean font_desc;
  bboolean language;
} FoundAttrs;

static bboolean 
filter_bango_attrs (BangoAttribute *attr, 
		    bpointer        data)
{
  FoundAttrs *found = (FoundAttrs *)data;

  if (attr->klass->type == BANGO_ATTR_WEIGHT)
    found->weight = TRUE;
  else if (attr->klass->type == BANGO_ATTR_FOREGROUND)
    found->foreground = TRUE;
  else if (attr->klass->type == BANGO_ATTR_UNDERLINE)
    found->underline = TRUE;
  /* Make sure optional start/end properties are working */
  else if (attr->klass->type == BANGO_ATTR_SIZE && 
	   attr->start_index == 5 &&
	   attr->end_index   == 10)
    found->size = TRUE;
  else if (attr->klass->type == BANGO_ATTR_FONT_DESC)
    found->font_desc = TRUE;
  else if (attr->klass->type == BANGO_ATTR_LANGUAGE)
    found->language = TRUE;

  return TRUE;
}

static void
test_bango_attributes (void)
{
  BtkBuilder *builder;
  FoundAttrs found = { 0, };
  const bchar buffer[] =
    "<interface>"
    "  <object class=\"BtkLabel\" id=\"label1\">"
    "    <attributes>"
    "      <attribute name=\"weight\" value=\"BANGO_WEIGHT_BOLD\"/>"
    "      <attribute name=\"foreground\" value=\"DarkSlateGray\"/>"
    "      <attribute name=\"underline\" value=\"True\"/>"
    "      <attribute name=\"size\" value=\"4\" start=\"5\" end=\"10\"/>"
    "      <attribute name=\"font-desc\" value=\"Sans Italic 22\"/>"
    "      <attribute name=\"language\" value=\"pt_BR\"/>"
    "    </attributes>"
    "  </object>"
    "</interface>";
  const bchar err_buffer1[] =
    "<interface>"
    "  <object class=\"BtkLabel\" id=\"label1\">"
    "    <attributes>"
    "      <attribute name=\"weight\"/>"
    "    </attributes>"
    "  </object>"
    "</interface>";
  const bchar err_buffer2[] =
    "<interface>"
    "  <object class=\"BtkLabel\" id=\"label1\">"
    "    <attributes>"
    "      <attribute name=\"weight\" value=\"BANGO_WEIGHT_BOLD\" unrecognized=\"True\"/>"
    "    </attributes>"
    "  </object>"
    "</interface>";

  BObject *label;
  GError  *error = NULL;
  BangoAttrList *attrs, *filtered;
  
  /* Test attributes are set */
  builder = builder_new_from_string (buffer, -1, NULL);
  label = btk_builder_get_object (builder, "label1");
  g_assert (label != NULL);

  attrs = btk_label_get_attributes (BTK_LABEL (label));
  g_assert (attrs != NULL);

  filtered = bango_attr_list_filter (attrs, filter_bango_attrs, &found);
  g_assert (filtered);
  bango_attr_list_unref (filtered);

  g_assert (found.weight);
  g_assert (found.foreground);
  g_assert (found.underline);
  g_assert (found.size);
  g_assert (found.language);
  g_assert (found.font_desc);

  g_object_unref (builder);

  /* Test errors are set */
  builder = btk_builder_new ();
  btk_builder_add_from_string (builder, err_buffer1, -1, &error);
  label = btk_builder_get_object (builder, "label1");
  g_assert (g_error_matches (error,
                             BTK_BUILDER_ERROR,
                             BTK_BUILDER_ERROR_MISSING_ATTRIBUTE));
  g_object_unref (builder);
  g_error_free (error);
  error = NULL;

  builder = btk_builder_new ();
  btk_builder_add_from_string (builder, err_buffer2, -1, &error);
  label = btk_builder_get_object (builder, "label1");

  g_assert (g_error_matches (error,
                             BTK_BUILDER_ERROR,
                             BTK_BUILDER_ERROR_INVALID_ATTRIBUTE));
  g_object_unref (builder);
  g_error_free (error);
}

static void
test_requires (void)
{
  BtkBuilder *builder;
  GError     *error = NULL;
  bchar      *buffer;
  const bchar buffer_fmt[] =
    "<interface>"
    "  <requires lib=\"btk+\" version=\"%d.%d\"/>"
    "</interface>";

  buffer = g_strdup_printf (buffer_fmt, BTK_MAJOR_VERSION, BTK_MINOR_VERSION + 1);
  builder = btk_builder_new ();
  btk_builder_add_from_string (builder, buffer, -1, &error);
  g_assert (g_error_matches (error,
                             BTK_BUILDER_ERROR,
                             BTK_BUILDER_ERROR_VERSION_MISMATCH));
  g_object_unref (builder);
  g_error_free (error);
  g_free (buffer);
}

static void
test_add_objects (void)
{
  BtkBuilder *builder;
  GError *error;
  bint ret;
  BObject *obj;
  BtkUIManager *manager;
  BtkWidget *menubar;
  BObject *menu, *label;
  GList *children;
  bchar *objects[2] = {"mainbox", NULL};
  bchar *objects2[3] = {"mainbox", "window2", NULL};
  bchar *objects3[3] = {"uimgr1", "menubar1"};
  bchar *objects4[2] = {"uimgr1", NULL};
  const bchar buffer[] =
    "<interface>"
    "  <object class=\"BtkWindow\" id=\"window\">"
    "    <child>"
    "      <object class=\"BtkVBox\" id=\"mainbox\">"
    "        <property name=\"visible\">True</property>"
    "        <child>"
    "          <object class=\"BtkLabel\" id=\"label1\">"
    "            <property name=\"visible\">True</property>"
    "            <property name=\"label\" translatable=\"no\">first label</property>"
    "          </object>"
    "        </child>"
    "        <child>"
    "          <object class=\"BtkLabel\" id=\"label2\">"
    "            <property name=\"visible\">True</property>"
    "            <property name=\"label\" translatable=\"no\">second label</property>"
    "          </object>"
    "          <packing>"
    "            <property name=\"position\">1</property>"
    "          </packing>"
    "        </child>"
    "      </object>"
    "    </child>"
    "  </object>"
    "  <object class=\"BtkWindow\" id=\"window2\">"
    "    <child>"
    "      <object class=\"BtkLabel\" id=\"label3\">"
    "        <property name=\"label\" translatable=\"no\">second label</property>"
    "      </object>"
    "    </child>"
    "  </object>"
    "<interface/>";
  const bchar buffer2[] =
    "<interface>"
    "  <object class=\"BtkUIManager\" id=\"uimgr1\">"
    "    <child>"
    "      <object class=\"BtkActionGroup\" id=\"ag1\">"
    "        <child>"
    "          <object class=\"BtkAction\" id=\"file\">"
    "            <property name=\"label\">_File</property>"
    "          </object>"
    "          <accelerator key=\"n\" modifiers=\"BDK_CONTROL_MASK\"/>"
    "        </child>"
    "      </object>"
    "    </child>"
    "    <ui>"
    "      <menubar name=\"menubar1\">"
    "        <menu action=\"file\">"
    "        </menu>"
    "      </menubar>"
    "    </ui>"
    "  </object>"
    "  <object class=\"BtkWindow\" id=\"window1\">"
    "    <child>"
    "      <object class=\"BtkMenuBar\" id=\"menubar1\" constructor=\"uimgr1\"/>"
    "    </child>"
    "  </object>"
    "</interface>";

  error = NULL;
  builder = btk_builder_new ();
  ret = btk_builder_add_objects_from_string (builder, buffer, -1, objects, &error);
  g_assert (ret);
  g_assert (error == NULL);
  obj = btk_builder_get_object (builder, "window");
  g_assert (obj == NULL);
  obj = btk_builder_get_object (builder, "window2");
  g_assert (obj == NULL);
  obj = btk_builder_get_object (builder, "mainbox");  
  g_assert (BTK_IS_WIDGET (obj));
  g_object_unref (builder);

  error = NULL;
  builder = btk_builder_new ();
  ret = btk_builder_add_objects_from_string (builder, buffer, -1, objects2, &error);
  g_assert (ret);
  g_assert (error == NULL);
  obj = btk_builder_get_object (builder, "window");
  g_assert (obj == NULL);
  obj = btk_builder_get_object (builder, "window2");
  g_assert (BTK_IS_WINDOW (obj));
  btk_widget_destroy (BTK_WIDGET (obj));
  obj = btk_builder_get_object (builder, "mainbox");  
  g_assert (BTK_IS_WIDGET (obj));
  g_object_unref (builder);

  /* test cherry picking a ui manager and menubar that depends on it */
  error = NULL;
  builder = btk_builder_new ();
  ret = btk_builder_add_objects_from_string (builder, buffer2, -1, objects3, &error);
  g_assert (ret);
  obj = btk_builder_get_object (builder, "uimgr1");
  g_assert (BTK_IS_UI_MANAGER (obj));
  obj = btk_builder_get_object (builder, "file");
  g_assert (BTK_IS_ACTION (obj));
  obj = btk_builder_get_object (builder, "menubar1");
  g_assert (BTK_IS_MENU_BAR (obj));
  menubar = BTK_WIDGET (obj);

  children = btk_container_get_children (BTK_CONTAINER (menubar));
  menu = children->data;
  g_assert (menu != NULL);
  g_assert (BTK_IS_MENU_ITEM (menu));
  g_assert (strcmp (BTK_WIDGET (menu)->name, "file") == 0);
  g_list_free (children);
 
  label = B_OBJECT (BTK_BIN (menu)->child);
  g_assert (label != NULL);
  g_assert (BTK_IS_LABEL (label));
  g_assert (strcmp (btk_label_get_text (BTK_LABEL (label)), "File") == 0);

  g_object_unref (builder);

  /* test cherry picking just the ui manager */
  error = NULL;
  builder = btk_builder_new ();
  ret = btk_builder_add_objects_from_string (builder, buffer2, -1, objects4, &error);
  g_assert (ret);
  obj = btk_builder_get_object (builder, "uimgr1");
  g_assert (BTK_IS_UI_MANAGER (obj));
  manager = BTK_UI_MANAGER (obj);
  obj = btk_builder_get_object (builder, "file");
  g_assert (BTK_IS_ACTION (obj));
  menubar = btk_ui_manager_get_widget (manager, "/menubar1");
  g_assert (BTK_IS_MENU_BAR (menubar));

  children = btk_container_get_children (BTK_CONTAINER (menubar));
  menu = children->data;
  g_assert (menu != NULL);
  g_assert (BTK_IS_MENU_ITEM (menu));
  g_assert (strcmp (BTK_WIDGET (menu)->name, "file") == 0);
  g_list_free (children);
 
  label = B_OBJECT (BTK_BIN (menu)->child);
  g_assert (label != NULL);
  g_assert (BTK_IS_LABEL (label));
  g_assert (strcmp (btk_label_get_text (BTK_LABEL (label)), "File") == 0);

  g_object_unref (builder);
}

static BtkWidget *
get_parent_menubar (BtkWidget *menuitem)
{
  BtkMenuShell *menu_shell = (BtkMenuShell *)menuitem->parent;
  BtkWidget *attach = NULL;

  g_assert (BTK_IS_MENU_SHELL (menu_shell));

  while (menu_shell && !BTK_IS_MENU_BAR (menu_shell))
    {
      if (BTK_IS_MENU (menu_shell) && 
	  (attach = btk_menu_get_attach_widget (BTK_MENU (menu_shell))) != NULL)
	menu_shell = (BtkMenuShell *)attach->parent;
      else
	menu_shell = NULL;
    }

  return menu_shell ? BTK_WIDGET (menu_shell) : NULL;
}

static void
test_menus (void)
{
  const bchar *buffer =
    "<interface>"
    "  <object class=\"BtkWindow\" id=\"window1\">"
    "    <accel-groups>"
    "      <group name=\"accelgroup1\"/>"
    "    </accel-groups>"
    "    <child>"
    "      <object class=\"BtkVBox\" id=\"vbox1\">"
    "        <property name=\"visible\">True</property>"
    "        <property name=\"orientation\">vertical</property>"
    "        <child>"
    "          <object class=\"BtkMenuBar\" id=\"menubar1\">"
    "            <property name=\"visible\">True</property>"
    "            <child>"
    "              <object class=\"BtkMenuItem\" id=\"menuitem1\">"
    "                <property name=\"visible\">True</property>"
    "                <property name=\"label\" translatable=\"yes\">_File</property>"
    "                <property name=\"use_underline\">True</property>"
    "                <child type=\"submenu\">"
    "                  <object class=\"BtkMenu\" id=\"menu1\">"
    "                    <property name=\"visible\">True</property>"
    "                    <child>"
    "                      <object class=\"BtkImageMenuItem\" id=\"imagemenuitem1\">"
    "                        <property name=\"label\">btk-new</property>"
    "                        <property name=\"visible\">True</property>"
    "                        <property name=\"use_stock\">True</property>"
    "                        <property name=\"accel_group\">accelgroup1</property>"
    "                      </object>"
    "                    </child>"
    "                  </object>"
    "                </child>"
    "              </object>"
    "            </child>"
    "          </object>"
    "        </child>"
    "      </object>"
    "    </child>"
    "  </object>"
    "<object class=\"BtkAccelGroup\" id=\"accelgroup1\"/>"
    "</interface>";

  const bchar *buffer1 =
    "<interface>"
    "  <object class=\"BtkWindow\" id=\"window1\">"
    "    <accel-groups>"
    "      <group name=\"accelgroup1\"/>"
    "    </accel-groups>"
    "    <child>"
    "      <object class=\"BtkVBox\" id=\"vbox1\">"
    "        <property name=\"visible\">True</property>"
    "        <property name=\"orientation\">vertical</property>"
    "        <child>"
    "          <object class=\"BtkMenuBar\" id=\"menubar1\">"
    "            <property name=\"visible\">True</property>"
    "            <child>"
    "              <object class=\"BtkImageMenuItem\" id=\"imagemenuitem1\">"
    "                <property name=\"visible\">True</property>"
    "                <child>"
    "                  <object class=\"BtkLabel\" id=\"custom1\">"
    "                    <property name=\"visible\">True</property>"
    "                    <property name=\"label\">a label</property>"
    "                  </object>"
    "                </child>"
    "              </object>"
    "            </child>"
    "          </object>"
    "        </child>"
    "      </object>"
    "    </child>"
    "  </object>"
    "<object class=\"BtkAccelGroup\" id=\"accelgroup1\"/>"
    "</interface>";
  BtkBuilder *builder;
  BtkWidget *window, *item;
  BtkAccelGroup *accel_group;
  BtkWidget *item_accel_label, *sample_accel_label, *sample_menu_item, *custom;

  /* Check that the item has the correct accel label string set
   */
  builder = builder_new_from_string (buffer, -1, NULL);
  window = (BtkWidget *)btk_builder_get_object (builder, "window1");
  item = (BtkWidget *)btk_builder_get_object (builder, "imagemenuitem1");
  accel_group = (BtkAccelGroup *)btk_builder_get_object (builder, "accelgroup1");

  btk_widget_show_all (window);

  sample_menu_item = btk_image_menu_item_new_from_stock (BTK_STOCK_NEW, accel_group);

  g_assert (BTK_BIN (sample_menu_item)->child);
  g_assert (BTK_IS_ACCEL_LABEL (BTK_BIN (sample_menu_item)->child));
  sample_accel_label = BTK_WIDGET (BTK_BIN (sample_menu_item)->child);
  btk_widget_show (sample_accel_label);

  g_assert (BTK_BIN (item)->child);
  g_assert (BTK_IS_ACCEL_LABEL (BTK_BIN (item)->child));
  item_accel_label = BTK_WIDGET (BTK_BIN (item)->child);

  btk_accel_label_refetch (BTK_ACCEL_LABEL (sample_accel_label));
  btk_accel_label_refetch (BTK_ACCEL_LABEL (item_accel_label));

  g_assert (BTK_ACCEL_LABEL (sample_accel_label)->accel_string != NULL);
  g_assert (BTK_ACCEL_LABEL (item_accel_label)->accel_string != NULL);
  g_assert (strcmp (BTK_ACCEL_LABEL (item_accel_label)->accel_string, 
		    BTK_ACCEL_LABEL (sample_accel_label)->accel_string) == 0);

  /* Check the menu hierarchy worked here  */
  g_assert (get_parent_menubar (item));

  btk_widget_destroy (BTK_WIDGET (window));
  btk_widget_destroy (sample_menu_item);
  g_object_unref (builder);


  /* Check that we can add alien children to menu items via normal
   * BtkContainer apis.
   */
  builder = builder_new_from_string (buffer1, -1, NULL);
  window = (BtkWidget *)btk_builder_get_object (builder, "window1");
  item = (BtkWidget *)btk_builder_get_object (builder, "imagemenuitem1");
  custom = (BtkWidget *)btk_builder_get_object (builder, "custom1");

  g_assert (custom->parent == item);

  btk_widget_destroy (BTK_WIDGET (window));
  g_object_unref (builder);

}


static void 
test_file (const bchar *filename)
{
  BtkBuilder *builder;
  GError *error = NULL;
  GSList *l, *objects;

  builder = btk_builder_new ();

  if (!btk_builder_add_from_file (builder, filename, &error))
    {
      g_error ("%s", error->message);
      g_error_free (error);
      return;
    }

  objects = btk_builder_get_objects (builder);
  for (l = objects; l; l = l->next)
    {
      BObject *obj = (BObject*)l->data;

      if (BTK_IS_DIALOG (obj))
	{
	  int response;

	  g_print ("Running dialog %s.\n",
		   btk_widget_get_name (BTK_WIDGET (obj)));
	  response = btk_dialog_run (BTK_DIALOG (obj));
	}
      else if (BTK_IS_WINDOW (obj))
	{
	  g_signal_connect (obj, "destroy", G_CALLBACK (btk_main_quit), NULL);
	  g_print ("Showing %s.\n",
		   btk_widget_get_name (BTK_WIDGET (obj)));
	  btk_widget_show_all (BTK_WIDGET (obj));
	}
    }

  btk_main ();

  g_object_unref (builder);
  builder = NULL;
}

static void
test_message_area (void)
{
  BtkBuilder *builder;
  GError *error;
  BObject *obj, *obj1;
  const bchar buffer[] =
    "<interface>"
    "  <object class=\"BtkInfoBar\" id=\"infobar1\">"
    "    <child internal-child=\"content_area\">"
    "      <object class=\"BtkHBox\" id=\"contentarea1\">"
    "        <child>"
    "          <object class=\"BtkLabel\" id=\"content\">"
    "            <property name=\"label\" translatable=\"yes\">Message</property>"
    "          </object>"
    "        </child>"
    "      </object>"
    "    </child>"
    "    <child internal-child=\"action_area\">"
    "      <object class=\"BtkVButtonBox\" id=\"actionarea1\">"
    "        <child>"
    "          <object class=\"BtkButton\" id=\"button_ok\">"
    "            <property name=\"label\">btk-ok</property>"
    "            <property name=\"use-stock\">yes</property>"
    "          </object>"
    "        </child>"
    "      </object>"
    "    </child>"
    "    <action-widgets>"
    "      <action-widget response=\"1\">button_ok</action-widget>"
    "    </action-widgets>"
    "  </object>"
    "</interface>";

  error = NULL;
  builder = builder_new_from_string (buffer, -1, NULL);
  g_assert (error == NULL);
  obj = btk_builder_get_object (builder, "infobar1");
  g_assert (BTK_IS_INFO_BAR (obj));
  obj1 = btk_builder_get_object (builder, "content");
  g_assert (BTK_IS_LABEL (obj1));
  g_assert (btk_widget_get_parent (btk_widget_get_parent (BTK_WIDGET (obj1))) == BTK_WIDGET (obj));

  obj1 = btk_builder_get_object (builder, "button_ok");
  g_assert (BTK_IS_BUTTON (obj1));
  g_assert (btk_widget_get_parent (btk_widget_get_parent (BTK_WIDGET (obj1))) == BTK_WIDGET (obj));

  g_object_unref (builder);
}

int
main (int argc, char **argv)
{
  /* initialize test program */
  btk_test_init (&argc, &argv);

  if (argc > 1)
    {
      test_file (argv[1]);
      return 0;
    }

  g_test_add_func ("/Builder/Parser", test_parser);
  g_test_add_func ("/Builder/Types", test_types);
  g_test_add_func ("/Builder/Construct-Only Properties", test_construct_only_property);
  g_test_add_func ("/Builder/Children", test_children);
  g_test_add_func ("/Builder/Child Properties", test_child_properties);
  g_test_add_func ("/Builder/Object Properties", test_object_properties);
  g_test_add_func ("/Builder/Notebook", test_notebook);
  g_test_add_func ("/Builder/Domain", test_domain);
  g_test_add_func ("/Builder/Signal Autoconnect", test_connect_signals);
  g_test_add_func ("/Builder/UIManager Simple", test_uimanager_simple);
  g_test_add_func ("/Builder/Spin Button", test_spin_button);
  g_test_add_func ("/Builder/SizeGroup", test_sizegroup);
  g_test_add_func ("/Builder/ListStore", test_list_store);
  g_test_add_func ("/Builder/TreeStore", test_tree_store);
  g_test_add_func ("/Builder/TreeView Column", test_treeview_column);
  g_test_add_func ("/Builder/IconView", test_icon_view);
  g_test_add_func ("/Builder/ComboBox", test_combo_box);
#if 0
  g_test_add_func ("/Builder/ComboBox Entry", test_combo_box_entry);
#endif
  g_test_add_func ("/Builder/CellView", test_cell_view);
  g_test_add_func ("/Builder/Dialog", test_dialog);
  g_test_add_func ("/Builder/Accelerators", test_accelerators);
  g_test_add_func ("/Builder/Widget", test_widget);
  g_test_add_func ("/Builder/Value From String", test_value_from_string);
  g_test_add_func ("/Builder/Reference Counting", test_reference_counting);
  g_test_add_func ("/Builder/Window", test_window);
  g_test_add_func ("/Builder/IconFactory", test_icon_factory);
  g_test_add_func ("/Builder/BangoAttributes", test_bango_attributes);
  g_test_add_func ("/Builder/Requires", test_requires);
  g_test_add_func ("/Builder/AddObjects", test_add_objects);
  g_test_add_func ("/Builder/Menus", test_menus);
  g_test_add_func ("/Builder/MessageArea", test_message_area);
  g_test_add_func ("/Builder/MessageDialog", test_message_dialog);

  return g_test_run();
}
