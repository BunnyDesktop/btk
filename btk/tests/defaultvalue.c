/* Btk+ default value tests
 * Copyright (C) 2007 Christian Persch
 *               2007 Johan Dahlin
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

#undef BTK_DISABLE_DEPRECATED
#define BTK_ENABLE_BROKEN
#include <string.h>
#include <btk/btk.h>
#include <btk/btkunixprint.h>

static void
check_property (const char *output,
	        BParamSpec *pspec,
		BValue *value)
{
  BValue default_value = { 0, };
  char *v, *dv, *msg;

  if (g_param_value_defaults (pspec, value))
      return;

  b_value_init (&default_value, G_PARAM_SPEC_VALUE_TYPE (pspec));
  g_param_value_set_default (pspec, &default_value);
      
  v = g_strdup_value_contents (value);
  dv = g_strdup_value_contents (&default_value);
  
  msg = g_strdup_printf ("%s %s.%s: %s != %s\n",
			 output,
			 g_type_name (pspec->owner_type),
			 pspec->name,
			 dv, v);
  g_assertion_message (G_LOG_DOMAIN, __FILE__, __LINE__,
		       B_STRFUNC, msg);
  g_free (msg);
  
  g_free (v);
  g_free (dv);
  b_value_unset (&default_value);
}

static void
test_type (gconstpointer data)
{
  BObjectClass *klass;
  BObject *instance;
  BParamSpec **pspecs;
  buint n_pspecs, i;
  GType type;

  type = * (GType *) data;

  if (!B_TYPE_IS_CLASSED (type))
    return;

  if (B_TYPE_IS_ABSTRACT (type))
    return;

  if (!g_type_is_a (type, B_TYPE_OBJECT))
    return;

  /* These can't be freely constructed/destroyed */
  if (g_type_is_a (type, BTK_TYPE_PRINT_JOB) ||
      g_type_is_a (type, BDK_TYPE_PIXBUF_LOADER) ||
      g_type_is_a (type, bdk_pixbuf_simple_anim_iter_get_type ()))
    return;

  /* The btk_arg compat wrappers can't set up default values */
  if (g_type_is_a (type, BTK_TYPE_CLIST) ||
      g_type_is_a (type, BTK_TYPE_CTREE) ||
      g_type_is_a (type, BTK_TYPE_LIST) ||
      g_type_is_a (type, BTK_TYPE_TIPS_QUERY)) 
    return;

  klass = g_type_class_ref (type);
  
  if (g_type_is_a (type, BTK_TYPE_SETTINGS))
    instance = g_object_ref (btk_settings_get_default ());
  else if (g_type_is_a (type, BDK_TYPE_BANGO_RENDERER))
    instance = g_object_ref (bdk_bango_renderer_get_default (bdk_screen_get_default ()));
  else if (g_type_is_a (type, BDK_TYPE_PIXMAP))
    instance = g_object_ref (bdk_pixmap_new (NULL, 1, 1, 1));
  else if (g_type_is_a (type, BDK_TYPE_COLORMAP))
    instance = g_object_ref (bdk_colormap_new (bdk_visual_get_best (), TRUE));
  else if (g_type_is_a (type, BDK_TYPE_WINDOW))
    {
      BdkWindowAttr attributes;
      attributes.window_type = BDK_WINDOW_TEMP;
      attributes.event_mask = 0;
      attributes.width = 100;
      attributes.height = 100;
      instance = g_object_ref (bdk_window_new (NULL, &attributes, 0));
    }
  else
    instance = g_object_new (type, NULL);

  if (g_type_is_a (type, B_TYPE_INITIALLY_UNOWNED))
    g_object_ref_sink (instance);

  pspecs = g_object_class_list_properties (klass, &n_pspecs);
  for (i = 0; i < n_pspecs; ++i)
    {
      BParamSpec *pspec = pspecs[i];
      BValue value = { 0, };
      
      if (pspec->owner_type != type)
	continue;

      if ((pspec->flags & G_PARAM_READABLE) == 0)
	continue;

      if (g_type_is_a (type, BDK_TYPE_DISPLAY_MANAGER) &&
	  (strcmp (pspec->name, "default-display") == 0))
	continue;

      if (g_type_is_a (type, BDK_TYPE_BANGO_RENDERER) &&
	  (strcmp (pspec->name, "screen") == 0))
	continue;

      if (g_type_is_a (type, BTK_TYPE_ABOUT_DIALOG) &&
	  (strcmp (pspec->name, "program-name") == 0))
	continue;
      
      /* These are set to the current date */
      if (g_type_is_a (type, BTK_TYPE_CALENDAR) &&
	  (strcmp (pspec->name, "year") == 0 ||
	   strcmp (pspec->name, "month") == 0 ||
	   strcmp (pspec->name, "day") == 0))
	continue;

      if (g_type_is_a (type, BTK_TYPE_CELL_RENDERER_TEXT) &&
	  (strcmp (pspec->name, "background-bdk") == 0 ||
	   strcmp (pspec->name, "foreground-bdk") == 0 ||
	   strcmp (pspec->name, "font") == 0 ||
	   strcmp (pspec->name, "font-desc") == 0))
	continue;

      if (g_type_is_a (type, BTK_TYPE_CELL_VIEW) &&
	  (strcmp (pspec->name, "background-bdk") == 0 ||
	   strcmp (pspec->name, "foreground-bdk") == 0))
	continue;

      if (g_type_is_a (type, BTK_TYPE_COLOR_BUTTON) &&
	  strcmp (pspec->name, "color") == 0)
	continue;

      if (g_type_is_a (type, BTK_TYPE_COLOR_SELECTION) &&
	  strcmp (pspec->name, "current-color") == 0)
	continue;

      if (g_type_is_a (type, BTK_TYPE_COLOR_SELECTION_DIALOG) &&
	  (strcmp (pspec->name, "color-selection") == 0 ||
	   strcmp (pspec->name, "ok-button") == 0 ||
	   strcmp (pspec->name, "help-button") == 0 ||
	   strcmp (pspec->name, "cancel-button") == 0))
	continue;

      /* Default invisible char is determined at runtime */
      if (g_type_is_a (type, BTK_TYPE_ENTRY) &&
	  (strcmp (pspec->name, "invisible-char") == 0 ||
           strcmp (pspec->name, "buffer") == 0))
	continue;

      /* Gets set to the cwd */
      if (g_type_is_a (type, BTK_TYPE_FILE_SELECTION) &&
	  strcmp (pspec->name, "filename") == 0)
	continue;

      if (g_type_is_a (type, BTK_TYPE_FONT_SELECTION) &&
	  strcmp (pspec->name, "font") == 0)
	continue;

      if (g_type_is_a (type, BTK_TYPE_LAYOUT) &&
	  (strcmp (pspec->name, "hadjustment") == 0 ||
           strcmp (pspec->name, "vadjustment") == 0))
	continue;

      if (g_type_is_a (type, BTK_TYPE_MESSAGE_DIALOG) &&
          (strcmp (pspec->name, "image") == 0 ||
           strcmp (pspec->name, "message-area") == 0))

	continue;

      if (g_type_is_a (type, BTK_TYPE_PRINT_OPERATION) &&
	  strcmp (pspec->name, "job-name") == 0)
	continue;

      if (g_type_is_a (type, BTK_TYPE_PRINT_UNIX_DIALOG) &&
	  (strcmp (pspec->name, "page-setup") == 0 ||
	   strcmp (pspec->name, "print-settings") == 0))
	continue;

      if (g_type_is_a (type, BTK_TYPE_PROGRESS_BAR) &&
          strcmp (pspec->name, "adjustment") == 0)
        continue;

      /* filename value depends on $HOME */
      if (g_type_is_a (type, BTK_TYPE_RECENT_MANAGER) &&
          (strcmp (pspec->name, "filename") == 0 ||
	   strcmp (pspec->name, "size") == 0))
        continue;

      if (g_type_is_a (type, BTK_TYPE_SCALE_BUTTON) &&
          strcmp (pspec->name, "adjustment") == 0)
        continue;

      if (g_type_is_a (type, BTK_TYPE_SCROLLED_WINDOW) &&
	  (strcmp (pspec->name, "hadjustment") == 0 ||
           strcmp (pspec->name, "vadjustment") == 0))
	continue;

      /* these defaults come from XResources */
      if (g_type_is_a (type, BTK_TYPE_SETTINGS) &&
          strncmp (pspec->name, "btk-xft-", 8) == 0)
        continue;

      if (g_type_is_a (type, BTK_TYPE_SETTINGS) &&
          (strcmp (pspec->name, "color-hash") == 0 ||
	   strcmp (pspec->name, "btk-cursor-theme-name") == 0 ||
	   strcmp (pspec->name, "btk-cursor-theme-size") == 0 ||
	   strcmp (pspec->name, "btk-dnd-drag-threshold") == 0 ||
	   strcmp (pspec->name, "btk-double-click-time") == 0 ||
	   strcmp (pspec->name, "btk-fallback-icon-theme") == 0 ||
	   strcmp (pspec->name, "btk-file-chooser-backend") == 0 ||
	   strcmp (pspec->name, "btk-icon-theme-name") == 0 ||
	   strcmp (pspec->name, "btk-im-module") == 0 ||
	   strcmp (pspec->name, "btk-key-theme-name") == 0 ||
	   strcmp (pspec->name, "btk-theme-name") == 0 ||
           strcmp (pspec->name, "btk-sound-theme-name") == 0 ||
           strcmp (pspec->name, "btk-enable-input-feedback-sounds") == 0 ||
           strcmp (pspec->name, "btk-enable-event-sounds") == 0))
        continue;

      if (g_type_is_a (type, BTK_TYPE_SPIN_BUTTON) &&
          (strcmp (pspec->name, "adjustment") == 0))
        continue;

      if (g_type_is_a (type, BTK_TYPE_STATUS_ICON) &&
          (strcmp (pspec->name, "size") == 0 ||
           strcmp (pspec->name, "screen") == 0))
        continue;

      if (g_type_is_a (type, BTK_TYPE_TEXT_BUFFER) &&
          (strcmp (pspec->name, "tag-table") == 0 ||
           strcmp (pspec->name, "copy-target-list") == 0 ||
           strcmp (pspec->name, "paste-target-list") == 0))
        continue;

      /* language depends on the current locale */
      if (g_type_is_a (type, BTK_TYPE_TEXT_TAG) &&
          (strcmp (pspec->name, "background-bdk") == 0 ||
           strcmp (pspec->name, "foreground-bdk") == 0 ||
	   strcmp (pspec->name, "language") == 0 ||
	   strcmp (pspec->name, "font") == 0 ||
	   strcmp (pspec->name, "font-desc") == 0))
        continue;

      if (g_type_is_a (type, BTK_TYPE_TEXT) &&
	  (strcmp (pspec->name, "hadjustment") == 0 ||
           strcmp (pspec->name, "vadjustment") == 0))
        continue;

      if (g_type_is_a (type, BTK_TYPE_TEXT_VIEW) &&
          strcmp (pspec->name, "buffer") == 0)
        continue;

      if (g_type_is_a (type, BTK_TYPE_TOOL_ITEM_GROUP) &&
          strcmp (pspec->name, "label-widget") == 0)
        continue;

      if (g_type_is_a (type, BTK_TYPE_TREE_VIEW) &&
	  (strcmp (pspec->name, "hadjustment") == 0 ||
           strcmp (pspec->name, "vadjustment") == 0))
	continue;

      if (g_type_is_a (type, BTK_TYPE_VIEWPORT) &&
	  (strcmp (pspec->name, "hadjustment") == 0 ||
           strcmp (pspec->name, "vadjustment") == 0))
	continue;

      if (g_type_is_a (type, BTK_TYPE_WIDGET) &&
	  (strcmp (pspec->name, "name") == 0 ||
	   strcmp (pspec->name, "screen") == 0 ||
	   strcmp (pspec->name, "style") == 0))
	continue;

      if (g_test_verbose ())
      g_print ("Property %s.%s\n", 
	     g_type_name (pspec->owner_type),
	     pspec->name);
      b_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      g_object_get_property (instance, pspec->name, &value);
      check_property ("Property", pspec, &value);
      b_value_unset (&value);
    }
  g_free (pspecs);

  if (g_type_is_a (type, BTK_TYPE_WIDGET))
    {
      pspecs = btk_widget_class_list_style_properties (BTK_WIDGET_CLASS (klass), &n_pspecs);
      
      for (i = 0; i < n_pspecs; ++i)
	{
	  BParamSpec *pspec = pspecs[i];
	  BValue value = { 0, };
	  
	  if (pspec->owner_type != type)
	    continue;

	  if ((pspec->flags & G_PARAM_READABLE) == 0)
	    continue;
	  
	  b_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
	  btk_widget_style_get_property (BTK_WIDGET (instance), pspec->name, &value);
	  check_property ("Style property", pspec, &value);
	  b_value_unset (&value);
	}

      g_free (pspecs);
    }
  
  if (g_type_is_a (type, BDK_TYPE_WINDOW))
    bdk_window_destroy (BDK_WINDOW (instance));
  else
    g_object_unref (instance);
  
  g_type_class_unref (klass);
}

extern void pixbuf_init (void);

int
main (int argc, char **argv)
{
  const GType *otypes;
  buint i;

  btk_test_init (&argc, &argv);
  pixbuf_init ();
  btk_test_register_all_types();
  
  otypes = btk_test_list_all_types (NULL);
  for (i = 0; otypes[i]; i++)
    {
      bchar *testname;
      
      testname = g_strdup_printf ("/Default Values/%s",
				  g_type_name (otypes[i]));
      g_test_add_data_func (testname,
                            &otypes[i],
			    test_type);
      g_free (testname);
    }
  
  return g_test_run();
}
