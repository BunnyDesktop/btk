/* Btk+ testing utilities
 * Copyright (C) 2007 Imendio AB
 * Authors: Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* need to get the prototypes of all get_type functions */
#define BTK_ENABLE_BROKEN
#undef BTK_DISABLE_DEPRECATED
/* Need to get BDK_WINDOW_OBJECT */
#undef BDK_DISABLE_DEPRECATED

#include "config.h"

#include <btk/btk.h>
#include "btkalias.h"

#include <locale.h>
#include <string.h>
#include <math.h>


/**
 * SECTION:btktesting
 * @Short_description: Utilities for testing BTK+ applications
 * @Title: Testing
 */

/**
 * btk_test_init:
 * @argcp: Address of the <parameter>argc</parameter> parameter of the
 *        main() function. Changed if any arguments were handled.
 * @argvp: (inout) (array length=argcp): Address of the 
 *        <parameter>argv</parameter> parameter of main().
 *        Any parameters understood by g_test_init() or btk_init() are
 *        stripped before return.
 * @Varargs: currently unused
 *
 * This function is used to initialize a BTK+ test program.
 *
 * It will in turn call g_test_init() and btk_init() to properly
 * initialize the testing framework and graphical toolkit. It'll 
 * also set the program's locale to "C" and prevent loading of rc 
 * files and Btk+ modules. This is done to make tets program
 * environments as deterministic as possible.
 *
 * Like btk_init() and g_test_init(), any known arguments will be
 * processed and stripped from @argc and @argv.
 *
 * Since: 2.14
 **/
void
btk_test_init (int    *argcp,
               char ***argvp,
               ...)
{
  g_test_init (argcp, argvp, NULL);
  /* - enter C locale
   * - call g_test_init();
   * - call btk_init();
   * - prevent RC files from loading;
   * - prevent Btk modules from loading;
   * - supply mock object for BtkSettings
   * FUTURE TODO:
   * - this function could install a mock object around BtkSettings
   */
  g_setenv ("BTK_MODULES", "", TRUE);
  g_setenv ("BTK2_RC_FILES", "/dev/null", TRUE);
  btk_disable_setlocale();
  setlocale (LC_ALL, "C");
  g_test_bug_base ("http://bugzilla.bunny.org/show_bug.cgi?id=%s");
  btk_init (argcp, argvp);
}

static GSList*
test_find_widget_input_windows (BtkWidget *widget,
                                gboolean   input_only)
{
  GList *node, *children;
  GSList *matches = NULL;
  gpointer udata;
  bdk_window_get_user_data (widget->window, &udata);
  if (udata == widget && (!input_only || (BDK_IS_WINDOW (widget->window) && BDK_WINDOW_OBJECT (widget->window)->input_only)))
    matches = g_slist_prepend (matches, widget->window);
  children = bdk_window_get_children (btk_widget_get_parent_window (widget));
  for (node = children; node; node = node->next)
    {
      bdk_window_get_user_data (node->data, &udata);
      if (udata == widget && (!input_only || (BDK_IS_WINDOW (node->data) && BDK_WINDOW_OBJECT (node->data)->input_only)))
        matches = g_slist_prepend (matches, node->data);
    }
  return g_slist_reverse (matches);
}

/**
 * btk_test_widget_send_key
 * @widget: Widget to generate a key press and release on.
 * @keyval: A Bdk keyboard value.
 * @modifiers: Keyboard modifiers the event is setup with.
 *
 * This function will generate keyboard press and release events in
 * the middle of the first BdkWindow found that belongs to @widget.
 * For %BTK_NO_WINDOW widgets like BtkButton, this will often be an
 * input-only event window. For other widgets, this is usually widget->window.
 * Certain caveats should be considered when using this function, in
 * particular because the mouse pointer is warped to the key press
 * location, see bdk_test_simulate_key() for details.
 *
 * Returns: wether all actions neccessary for the key event simulation were carried out successfully.
 *
 * Since: 2.14
 **/
gboolean
btk_test_widget_send_key (BtkWidget      *widget,
                          guint           keyval,
                          BdkModifierType modifiers)
{
  gboolean k1res, k2res;
  GSList *iwindows = test_find_widget_input_windows (widget, FALSE);
  if (!iwindows)
    iwindows = test_find_widget_input_windows (widget, TRUE);
  if (!iwindows)
    return FALSE;
  k1res = bdk_test_simulate_key (iwindows->data, -1, -1, keyval, modifiers, BDK_KEY_PRESS);
  k2res = bdk_test_simulate_key (iwindows->data, -1, -1, keyval, modifiers, BDK_KEY_RELEASE);
  g_slist_free (iwindows);
  return k1res && k2res;
}

/**
 * btk_test_widget_click
 * @widget: Widget to generate a button click on.
 * @button: Number of the pointer button for the event, usually 1, 2 or 3.
 * @modifiers: Keyboard modifiers the event is setup with.
 *
 * This function will generate a @button click (button press and button
 * release event) in the middle of the first BdkWindow found that belongs
 * to @widget.
 * For %BTK_NO_WINDOW widgets like BtkButton, this will often be an
 * input-only event window. For other widgets, this is usually widget->window.
 * Certain caveats should be considered when using this function, in
 * particular because the mouse pointer is warped to the button click
 * location, see bdk_test_simulate_button() for details.
 *
 * Returns: wether all actions neccessary for the button click simulation were carried out successfully.
 *
 * Since: 2.14
 **/
gboolean
btk_test_widget_click (BtkWidget      *widget,
                       guint           button,
                       BdkModifierType modifiers)
{
  gboolean b1res, b2res;
  GSList *iwindows = test_find_widget_input_windows (widget, FALSE);
  if (!iwindows)
    iwindows = test_find_widget_input_windows (widget, TRUE);
  if (!iwindows)
    return FALSE;
  b1res = bdk_test_simulate_button (iwindows->data, -1, -1, button, modifiers, BDK_BUTTON_PRESS);
  b2res = bdk_test_simulate_button (iwindows->data, -1, -1, button, modifiers, BDK_BUTTON_RELEASE);
  g_slist_free (iwindows);
  return b1res && b2res;
}

/**
 * btk_test_spin_button_click
 * @spinner: valid BtkSpinButton widget.
 * @button:  Number of the pointer button for the event, usually 1, 2 or 3.
 * @upwards: %TRUE for upwards arrow click, %FALSE for downwards arrow click.
 *
 * This function will generate a @button click in the upwards or downwards
 * spin button arrow areas, usually leading to an increase or decrease of
 * spin button's value.
 *
 * Returns: wether all actions neccessary for the button click simulation were carried out successfully.
 *
 * Since: 2.14
 **/
gboolean
btk_test_spin_button_click (BtkSpinButton  *spinner,
                            guint           button,
                            gboolean        upwards)
{
  gboolean b1res = FALSE, b2res = FALSE;
  if (spinner->panel)
    {
      gint width, height, pos;
      bdk_drawable_get_size (spinner->panel, &width, &height);
      pos = upwards ? 0 : height - 1;
      b1res = bdk_test_simulate_button (spinner->panel, width - 1, pos, button, 0, BDK_BUTTON_PRESS);
      b2res = bdk_test_simulate_button (spinner->panel, width - 1, pos, button, 0, BDK_BUTTON_RELEASE);
    }
  return b1res && b2res;
}

/**
 * btk_test_find_label
 * @widget:        Valid label or container widget.
 * @label_pattern: Shell-glob pattern to match a label string.
 *
 * This function will search @widget and all its descendants for a BtkLabel
 * widget with a text string matching @label_pattern.
 * The @label_pattern may contain asterisks '*' and question marks '?' as
 * placeholders, g_pattern_match() is used for the matching.
 * Note that locales other than "C" tend to alter (translate" label strings,
 * so this function is genrally only useful in test programs with
 * predetermined locales, see btk_test_init() for more details.
 *
 * Returns: (transfer none): a BtkLabel widget if any is found.
 *
 * Since: 2.14
 **/
BtkWidget*
btk_test_find_label (BtkWidget    *widget,
                     const gchar  *label_pattern)
{
  if (BTK_IS_LABEL (widget))
    {
      const gchar *text = btk_label_get_text (BTK_LABEL (widget));
      if (g_pattern_match_simple (label_pattern, text))
        return widget;
    }
  if (BTK_IS_CONTAINER (widget))
    {
      GList *node, *list = btk_container_get_children (BTK_CONTAINER (widget));
      for (node = list; node; node = node->next)
        {
          BtkWidget *label = btk_test_find_label (node->data, label_pattern);
          if (label)
            return label;
        }
      g_list_free (list);
    }
  return NULL;
}

static GList*
test_list_descendants (BtkWidget *widget,
                       GType      widget_type)
{
  GList *results = NULL;
  if (BTK_IS_CONTAINER (widget))
    {
      GList *node, *list = btk_container_get_children (BTK_CONTAINER (widget));
      for (node = list; node; node = node->next)
        {
          if (!widget_type || g_type_is_a (G_OBJECT_TYPE (node->data), widget_type))
            results = g_list_prepend (results, node->data);
          else
            results = g_list_concat (results, test_list_descendants (node->data, widget_type));
        }
      g_list_free (list);
    }
  return results;
}

static int
widget_geo_dist (BtkWidget *a,
                 BtkWidget *b,
                 BtkWidget *base)
{
  int ax0, ay0, ax1, ay1, bx0, by0, bx1, by1, xdist = 0, ydist = 0;
  if (!btk_widget_translate_coordinates (a, base, 0, 0, &ax0, &ay0) ||
      !btk_widget_translate_coordinates (a, base, a->allocation.width, a->allocation.height, &ax1, &ay1))
    return -G_MAXINT;
  if (!btk_widget_translate_coordinates (b, base, 0, 0, &bx0, &by0) ||
      !btk_widget_translate_coordinates (b, base, b->allocation.width, b->allocation.height, &bx1, &by1))
    return +G_MAXINT;
  if (bx0 >= ax1)
    xdist = bx0 - ax1;
  else if (ax0 >= bx1)
    xdist = ax0 - bx1;
  if (by0 >= ay1)
    ydist = by0 - ay1;
  else if (ay0 >= by1)
    ydist = ay0 - by1;
  return xdist + ydist;
}

static int
widget_geo_cmp (gconstpointer a,
                gconstpointer b,
                gpointer      user_data)
{
  gpointer *data = user_data;
  BtkWidget *wa = (void*) a, *wb = (void*) b, *toplevel = data[0], *base_widget = data[1];
  int adist = widget_geo_dist (wa, base_widget, toplevel);
  int bdist = widget_geo_dist (wb, base_widget, toplevel);
  return adist > bdist ? +1 : adist == bdist ? 0 : -1;
}

/**
 * btk_test_find_sibling
 * @base_widget:        Valid widget, part of a widget hierarchy
 * @widget_type:        Type of a aearched for sibling widget
 *
 * This function will search siblings of @base_widget and siblings of its
 * ancestors for all widgets matching @widget_type.
 * Of the matching widgets, the one that is geometrically closest to
 * @base_widget will be returned.
 * The general purpose of this function is to find the most likely "action"
 * widget, relative to another labeling widget. Such as finding a
 * button or text entry widget, given it's corresponding label widget.
 *
 * Returns: (transfer none): a widget of type @widget_type if any is found.
 *
 * Since: 2.14
 **/
BtkWidget*
btk_test_find_sibling (BtkWidget *base_widget,
                       GType      widget_type)
{
  GList *siblings = NULL;
  BtkWidget *tmpwidget = base_widget;
  gpointer data[2];
  /* find all sibling candidates */
  while (tmpwidget)
    {
      tmpwidget = tmpwidget->parent;
      siblings = g_list_concat (siblings, test_list_descendants (tmpwidget, widget_type));
    }
  /* sort them by distance to base_widget */
  data[0] = btk_widget_get_toplevel (base_widget);
  data[1] = base_widget;
  siblings = g_list_sort_with_data (siblings, widget_geo_cmp, data);
  /* pick nearest != base_widget */
  siblings = g_list_remove (siblings, base_widget);
  tmpwidget = siblings ? siblings->data : NULL;
  g_list_free (siblings);
  return tmpwidget;
}

/**
 * btk_test_find_widget
 * @widget:        Container widget, usually a BtkWindow.
 * @label_pattern: Shell-glob pattern to match a label string.
 * @widget_type:   Type of a aearched for label sibling widget.
 *
 * This function will search the descendants of @widget for a widget
 * of type @widget_type that has a label matching @label_pattern next
 * to it. This is most useful for automated GUI testing, e.g. to find
 * the "OK" button in a dialog and synthesize clicks on it.
 * However see btk_test_find_label(), btk_test_find_sibling() and
 * btk_test_widget_click() for possible caveats involving the search of
 * such widgets and synthesizing widget events.
 *
 * Returns: (transfer none): a valid widget if any is found or %NULL.
 *
 * Since: 2.14
 **/
BtkWidget*
btk_test_find_widget (BtkWidget    *widget,
                      const gchar  *label_pattern,
                      GType         widget_type)
{
  BtkWidget *label = btk_test_find_label (widget, label_pattern);
  if (!label)
    label = btk_test_find_label (btk_widget_get_toplevel (widget), label_pattern);
  if (label)
    return btk_test_find_sibling (label, widget_type);
  return NULL;
}

/**
 * btk_test_slider_set_perc
 * @widget:     valid widget pointer.
 * @percentage: value between 0 and 100.
 *
 * This function will adjust the slider position of all BtkRange
 * based widgets, such as scrollbars or scales, it'll also adjust
 * spin buttons. The adjustment value of these widgets is set to
 * a value between the lower and upper limits, according to the
 * @percentage argument.
 *
 * Since: 2.14
 **/
void
btk_test_slider_set_perc (BtkWidget      *widget,
                          double          percentage)
{
  BtkAdjustment *adjustment = NULL;
  if (BTK_IS_RANGE (widget))
    adjustment = btk_range_get_adjustment (BTK_RANGE (widget));
  else if (BTK_IS_SPIN_BUTTON (widget))
    adjustment = btk_spin_button_get_adjustment (BTK_SPIN_BUTTON (widget));
  if (adjustment)
    btk_adjustment_set_value (adjustment, adjustment->lower + (adjustment->upper - adjustment->lower - adjustment->page_size) * percentage * 0.01);
}

/**
 * btk_test_slider_get_value
 * @widget:     valid widget pointer.
 *
 * Retrive the literal adjustment value for BtkRange based
 * widgets and spin buttons. Note that the value returned by
 * this function is anything between the lower and upper bounds
 * of the adjustment belonging to @widget, and is not a percentage
 * as passed in to btk_test_slider_set_perc().
 *
 * Returns: adjustment->value for an adjustment belonging to @widget.
 *
 * Since: 2.14
 **/
double
btk_test_slider_get_value (BtkWidget *widget)
{
  BtkAdjustment *adjustment = NULL;
  if (BTK_IS_RANGE (widget))
    adjustment = btk_range_get_adjustment (BTK_RANGE (widget));
  else if (BTK_IS_SPIN_BUTTON (widget))
    adjustment = btk_spin_button_get_adjustment (BTK_SPIN_BUTTON (widget));
  return adjustment ? adjustment->value : 0;
}

/**
 * btk_test_text_set
 * @widget:     valid widget pointer.
 * @string:     a 0-terminated C string
 *
 * Set the text string of @widget to @string if it is a BtkLabel,
 * BtkEditable (entry and text widgets) or BtkTextView.
 *
 * Since: 2.14
 **/
void
btk_test_text_set (BtkWidget   *widget,
                   const gchar *string)
{
  if (BTK_IS_LABEL (widget))
    btk_label_set_text (BTK_LABEL (widget), string);
  else if (BTK_IS_EDITABLE (widget))
    {
      int pos = 0;
      btk_editable_delete_text (BTK_EDITABLE (widget), 0, -1);
      btk_editable_insert_text (BTK_EDITABLE (widget), string, -1, &pos);
    }
  else if (BTK_IS_TEXT_VIEW (widget))
    {
      BtkTextBuffer *tbuffer = btk_text_view_get_buffer (BTK_TEXT_VIEW (widget));
      btk_text_buffer_set_text (tbuffer, string, -1);
    }
}

/**
 * btk_test_text_get
 * @widget:     valid widget pointer.
 *
 * Retrive the text string of @widget if it is a BtkLabel,
 * BtkEditable (entry and text widgets) or BtkTextView.
 *
 * Returns: new 0-terminated C string, needs to be released with g_free().
 *
 * Since: 2.14
 **/
gchar*
btk_test_text_get (BtkWidget *widget)
{
  if (BTK_IS_LABEL (widget))
    return g_strdup (btk_label_get_text (BTK_LABEL (widget)));
  else if (BTK_IS_EDITABLE (widget))
    {
      return g_strdup (btk_editable_get_chars (BTK_EDITABLE (widget), 0, -1));
    }
  else if (BTK_IS_TEXT_VIEW (widget))
    {
      BtkTextBuffer *tbuffer = btk_text_view_get_buffer (BTK_TEXT_VIEW (widget));
      BtkTextIter start, end;
      btk_text_buffer_get_start_iter (tbuffer, &start);
      btk_text_buffer_get_end_iter (tbuffer, &end);
      return btk_text_buffer_get_text (tbuffer, &start, &end, FALSE);
    }
  return NULL;
}

/**
 * btk_test_create_widget
 * @widget_type: a valid widget type.
 * @first_property_name: (allow-none): Name of first property to set or %NULL
 * @Varargs: value to set the first property to, followed by more
 *    name-value pairs, terminated by %NULL
 *
 * This function wraps g_object_new() for widget types.
 * It'll automatically show all created non window widgets, also
 * g_object_ref_sink() them (to keep them alive across a running test)
 * and set them up for destruction during the next test teardown phase.
 *
 * Returns: a newly created widget.
 *
 * Since: 2.14
 */
BtkWidget*
btk_test_create_widget (GType        widget_type,
                        const gchar *first_property_name,
                        ...)
{
  BtkWidget *widget;
  va_list var_args;
  g_return_val_if_fail (g_type_is_a (widget_type, BTK_TYPE_WIDGET), NULL);
  va_start (var_args, first_property_name);
  widget = (BtkWidget*) g_object_new_valist (widget_type, first_property_name, var_args);
  va_end (var_args);
  if (widget)
    {
      if (!BTK_IS_WINDOW (widget))
        btk_widget_show (widget);
      g_object_ref_sink (widget);
      g_test_queue_unref (widget);
      g_test_queue_destroy ((GDestroyNotify) btk_widget_destroy, widget);
    }
  return widget;
}

static void
try_main_quit (void)
{
  if (btk_main_level())
    btk_main_quit();
}

static int
test_increment_intp (int *intp)
{
  if (intp != NULL)
    *intp += 1;
  return 1; /* TRUE in case we're connected to event signals */
}

/**
 * btk_test_display_button_window
 * @window_title:       Title of the window to be displayed.
 * @dialog_text:        Text inside the window to be displayed.
 * @...:                %NULL terminated list of (const char *label, int *nump) pairs.
 *
 * Create a window with window title @window_title, text contents @dialog_text,
 * and a number of buttons, according to the paired argument list given
 * as @... parameters.
 * Each button is created with a @label and a ::clicked signal handler that
 * incremrents the integer stored in @nump.
 * The window will be automatically shown with btk_widget_show_now() after
 * creation, so when this function returns it has already been mapped,
 * resized and positioned on screen.
 * The window will quit any running btk_main()-loop when destroyed, and it
 * will automatically be destroyed upon test function teardown.
 *
 * Returns: a widget pointer to the newly created BtkWindow.
 *
 * Since: 2.14
 **/
BtkWidget*
btk_test_display_button_window (const gchar *window_title,
                                const gchar *dialog_text,
                                ...) /* NULL terminated list of (label, &int) pairs */
{
  va_list var_args;
  BtkWidget *window = btk_test_create_widget (BTK_TYPE_WINDOW, "title", window_title, NULL);
  BtkWidget *vbox = btk_test_create_widget (BTK_TYPE_VBOX, "parent", window, NULL);
  const char *arg1;
  btk_test_create_widget (BTK_TYPE_LABEL, "label", dialog_text, "parent", vbox, NULL);
  g_signal_connect (window, "destroy", G_CALLBACK (try_main_quit), NULL);
  va_start (var_args, dialog_text);
  arg1 = va_arg (var_args, const char*);
  while (arg1)
    {
      int *arg2 = va_arg (var_args, int*);
      BtkWidget *button = btk_test_create_widget (BTK_TYPE_BUTTON, "label", arg1, "parent", vbox, NULL);
      g_signal_connect_swapped (button, "clicked", G_CALLBACK (test_increment_intp), arg2);
      arg1 = va_arg (var_args, const char*);
    }
  va_end (var_args);
  btk_widget_show_all (vbox);
  btk_widget_show_now (window);
  while (btk_events_pending ())
    btk_main_iteration ();
  return window;
}

/**
 * btk_test_create_simple_window
 * @window_title:       Title of the window to be displayed.
 * @dialog_text:        Text inside the window to be displayed.
 *
 * Create a simple window with window title @window_title and
 * text contents @dialog_text.
 * The window will quit any running btk_main()-loop when destroyed, and it
 * will automatically be destroyed upon test function teardown.
 *
 * Returns: (transfer none): a widget pointer to the newly created BtkWindow.
 *
 * Since: 2.14
 **/
BtkWidget*
btk_test_create_simple_window (const gchar *window_title,
                               const gchar *dialog_text)
{
  BtkWidget *window = btk_test_create_widget (BTK_TYPE_WINDOW, "title", window_title, NULL);
  BtkWidget *vbox = btk_test_create_widget (BTK_TYPE_VBOX, "parent", window, NULL);
  btk_test_create_widget (BTK_TYPE_LABEL, "label", dialog_text, "parent", vbox, NULL);
  g_signal_connect (window, "destroy", G_CALLBACK (try_main_quit), NULL);
  btk_widget_show_all (vbox);
  return window;
}

static GType *all_registered_types = NULL;
static guint  n_all_registered_types = 0;

/**
 * btk_test_list_all_types
 * @n_types: location to store number of types
 * @returns: (array length=n_types zero-terminated=1) (transfer none):
 *    0-terminated array of type ids
 *
 * Return the type ids that have been registered after
 * calling btk_test_register_all_types().
 *
 * Since: 2.14
 **/
const GType*
btk_test_list_all_types (guint *n_types)
{
  if (n_types)
    *n_types = n_all_registered_types;
  return all_registered_types;
}

/**
 * btk_test_register_all_types
 *
 * Force registration of all core Btk+ and Bdk object types.
 * This allowes to refer to any of those object types via
 * g_type_from_name() after calling this function.
 *
 * Since: 2.14
 **/
void
btk_test_register_all_types (void)
{
  if (!all_registered_types)
    {
      const guint max_btk_types = 999;
      GType *tp;
      all_registered_types = g_new0 (GType, max_btk_types);
      tp = all_registered_types;
#include "btktypefuncs.c"
      n_all_registered_types = tp - all_registered_types;
      g_assert (n_all_registered_types + 1 < max_btk_types);
      *tp = 0;
    }
}

#define __BTK_TEST_UTILS_C__
#include "btkaliasdef.c"
