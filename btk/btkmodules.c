/* BTK - The GIMP Toolkit
 * Copyright 1998-2002 Tim Janik, Red Hat, Inc., and others.
 * Copyright (C) 2003 Alex Graveley
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

#include "config.h"

#include <string.h>

#include "btkmodules.h"
#include "btkmain.h"
#include "btksettings.h"
#include "btkdebug.h"
#include "btkprivate.h" /* BTK_LIBDIR */
#include "btkintl.h" 
#include "btkalias.h"

#include <bmodule.h>

typedef struct _BtkModuleInfo BtkModuleInfo;
struct _BtkModuleInfo
{
  GModule                 *module;
  bint                     ref_count;
  BtkModuleInitFunc        init_func;
  BtkModuleDisplayInitFunc display_init_func;
  GSList                  *names;
};

static GSList *btk_modules = NULL;

static bboolean default_display_opened = FALSE;

/* Saved argc, argv for delayed module initialization
 */
static bint    btk_argc = 0;
static bchar **btk_argv = NULL;

static bchar **
get_module_path (void)
{
  const bchar *module_path_env;
  const bchar *exe_prefix;
  const bchar *home_dir;
  bchar *home_btk_dir = NULL;
  bchar *module_path;
  bchar *default_dir;
  static bchar **result = NULL;

  if (result)
    return result;

  home_dir = g_get_home_dir();
  if (home_dir)
    home_btk_dir = g_build_filename (home_dir, ".btk-2.0", NULL);

  module_path_env = g_getenv ("BTK_PATH");
  exe_prefix = g_getenv ("BTK_EXE_PREFIX");

  if (exe_prefix)
    default_dir = g_build_filename (exe_prefix, "lib", "btk-2.0", NULL);
  else
    default_dir = g_build_filename (BTK_LIBDIR, "btk-2.0", NULL);

  if (module_path_env && home_btk_dir)
    module_path = g_build_path (G_SEARCHPATH_SEPARATOR_S,
				module_path_env, home_btk_dir, default_dir, NULL);
  else if (module_path_env)
    module_path = g_build_path (G_SEARCHPATH_SEPARATOR_S,
				module_path_env, default_dir, NULL);
  else if (home_btk_dir)
    module_path = g_build_path (G_SEARCHPATH_SEPARATOR_S,
				home_btk_dir, default_dir, NULL);
  else
    module_path = g_build_path (G_SEARCHPATH_SEPARATOR_S,
				default_dir, NULL);

  g_free (home_btk_dir);
  g_free (default_dir);

  result = bango_split_file_list (module_path);
  g_free (module_path);

  return result;
}

/**
 * _btk_get_module_path:
 * @type: the type of the module, for instance 'modules', 'engines', immodules'
 * 
 * Determines the search path for a particular type of module.
 * 
 * Return value: the search path for the module type. Free with g_strfreev().
 **/
bchar **
_btk_get_module_path (const bchar *type)
{
  bchar **paths = get_module_path();
  bchar **path;
  bchar **result;
  bint count = 0;

  for (path = paths; *path; path++)
    count++;

  result = g_new (bchar *, count * 4 + 1);

  count = 0;
  for (path = get_module_path (); *path; path++)
    {
      bint use_version, use_host;
      
      for (use_version = TRUE; use_version >= FALSE; use_version--)
	for (use_host = TRUE; use_host >= FALSE; use_host--)
	  {
	    bchar *tmp_dir;
	    
	    if (use_version && use_host)
	      tmp_dir = g_build_filename (*path, BTK_BINARY_VERSION, BTK_HOST, type, NULL);
	    else if (use_version)
	      tmp_dir = g_build_filename (*path, BTK_BINARY_VERSION, type, NULL);
	    else if (use_host)
	      tmp_dir = g_build_filename (*path, BTK_HOST, type, NULL);
	    else
	      tmp_dir = g_build_filename (*path, type, NULL);

	    result[count++] = tmp_dir;
	  }
    }

  result[count++] = NULL;

  return result;
}

/* Like g_module_path, but use .la as the suffix
 */
static bchar*
module_build_la_path (const bchar *directory,
		      const bchar *module_name)
{
  bchar *filename;
  bchar *result;
	
  if (strncmp (module_name, "lib", 3) == 0)
    filename = (bchar *)module_name;
  else
    filename =  g_strconcat ("lib", module_name, ".la", NULL);

  if (directory && *directory)
    result = g_build_filename (directory, filename, NULL);
  else
    result = g_strdup (filename);

  if (filename != module_name)
    g_free (filename);

  return result;
}

/**
 * _btk_find_module:
 * @name: the name of the module
 * @type: the type of the module, for instance 'modules', 'engines', immodules'
 * 
 * Looks for a dynamically module named @name of type @type in the standard BTK+
 *  module search path.
 * 
 * Return value: the pathname to the found module, or %NULL if it wasn't found.
 *  Free with g_free().
 **/
bchar *
_btk_find_module (const bchar *name,
		  const bchar *type)
{
  bchar **paths;
  bchar **path;
  bchar *module_name = NULL;

  if (g_path_is_absolute (name))
    return g_strdup (name);

  paths = _btk_get_module_path (type);
  for (path = paths; *path; path++)
    {
      bchar *tmp_name;

      tmp_name = g_module_build_path (*path, name);
      if (g_file_test (tmp_name, G_FILE_TEST_EXISTS))
	{
	  module_name = tmp_name;
	  goto found;
	}
      g_free(tmp_name);

      tmp_name = module_build_la_path (*path, name);
      if (g_file_test (tmp_name, G_FILE_TEST_EXISTS))
	{
	  module_name = tmp_name;
	  goto found;
	}
      g_free(tmp_name);
    }

 found:
  g_strfreev (paths);
  return module_name;
}

static GModule *
find_module (const bchar *name)
{
  GModule *module;
  bchar *module_name;

  module_name = _btk_find_module (name, "modules");
  if (!module_name)
    {
      /* As last resort, try loading without an absolute path (using system
       * library path)
       */
      module_name = g_module_build_path (NULL, name);
    }

  module = g_module_open (module_name, G_MODULE_BIND_LOCAL | G_MODULE_BIND_LAZY);

  if (_btk_module_has_mixed_deps (module))
    {
      g_warning ("BTK+ module %s cannot be loaded.\n"
                 "BTK+ 2.x symbols detected. Using BTK+ 2.x and BTK+ 3 in the same process is not supported.", module_name);
      g_module_close (module);
      module = NULL;
    }

  g_free (module_name);

  return module;
}

static bint
cmp_module (BtkModuleInfo *info,
	    GModule       *module)
{
  return info->module != module;
}

static GSList *
load_module (GSList      *module_list,
	     const bchar *name)
{
  BtkModuleInitFunc modinit_func;
  bpointer modinit_func_ptr;
  BtkModuleInfo *info = NULL;
  GModule *module = NULL;
  GSList *l;
  bboolean success = FALSE;
  
  if (g_module_supported ())
    {
      for (l = btk_modules; l; l = l->next)
	{
	  info = l->data;
	  if (b_slist_find_custom (info->names, name, 
				   (GCompareFunc)strcmp))
	    {
	      info->ref_count++;
	      
	      success = TRUE;
              break;
	    }
	}

      if (!success) 
	{
	  module = find_module (name);

	  if (module)
	    {
	      if (g_module_symbol (module, "btk_module_init", &modinit_func_ptr))
		modinit_func = modinit_func_ptr;
	      else
		modinit_func = NULL;

	      if (!modinit_func)
		g_module_close (module);
	      else
		{
		  GSList *temp;

		  success = TRUE;
		  info = NULL;

		  temp = b_slist_find_custom (btk_modules, module,
			(GCompareFunc)cmp_module);
		  if (temp != NULL)
			info = temp->data;

		  if (!info)
		    {
		      info = g_new0 (BtkModuleInfo, 1);
		      
		      info->names = b_slist_prepend (info->names, g_strdup (name));
		      info->module = module;
		      info->ref_count = 1;
		      info->init_func = modinit_func;
		      g_module_symbol (module, "btk_module_display_init",
				       (bpointer *) &info->display_init_func);
		      
		      btk_modules = b_slist_append (btk_modules, info);
		      
		      /* display_init == NULL indicates a non-multihead aware module.
		       * For these, we delay the call to init_func until first display is 
		       * opened, see default_display_notify_cb().
		       * For multihead aware modules, we call init_func immediately,
		       * and also call display_init_func on all opened displays.
		       */
		      if (default_display_opened || info->display_init_func)
			(* info->init_func) (&btk_argc, &btk_argv);
		      
		      if (info->display_init_func) 
			{
			  GSList *displays, *iter; 		  
			  displays = bdk_display_manager_list_displays (bdk_display_manager_get ());
			  for (iter = displays; iter; iter = iter->next)
			    {
			      BdkDisplay *display = iter->data;
			  (* info->display_init_func) (display);
			    }
			  b_slist_free (displays);
			}
		    }
		  else
		    {
		      BTK_NOTE (MODULES, g_print ("Module already loaded, ignoring: %s\n", name));
		      info->names = b_slist_prepend (info->names, g_strdup (name));
		      info->ref_count++;
		      /* remove new reference count on module, we already have one */
		      g_module_close (module);
		    }
		}
	    }
	}
    }

  if (success)
    {
      if (!b_slist_find (module_list, info))
	{
	  module_list = b_slist_prepend (module_list, info);
	}
      else
        info->ref_count--;
    }
  else
   {
      const bchar *error = g_module_error ();

      g_message ("Failed to load module \"%s\"%s%s",
                 name, error ? ": " : "", error ? error : "");
    }

  return module_list;
}


static void
btk_module_info_unref (BtkModuleInfo *info)
{
  GSList *l;

  info->ref_count--;

  if (info->ref_count == 0) 
    {
      BTK_NOTE (MODULES, 
		g_print ("Unloading module: %s\n", g_module_name (info->module)));

      btk_modules = b_slist_remove (btk_modules, info);
      g_module_close (info->module);
      for (l = info->names; l; l = l->next)
	g_free (l->data);
      b_slist_free (info->names);
      g_free (info);
    }
}

static GSList *
load_modules (const char *module_str)
{
  bchar **module_names;
  GSList *module_list = NULL;
  bint i;

  BTK_NOTE (MODULES, g_print ("Loading module list: %s\n", module_str));

  module_names = bango_split_file_list (module_str);
  for (i = 0; module_names[i]; i++) 
    module_list = load_module (module_list, module_names[i]);

  module_list = b_slist_reverse (module_list);
  g_strfreev (module_names);

  return module_list;
}

static void
default_display_notify_cb (BdkDisplayManager *display_manager)
{
  GSList *slist;

  /* Initialize non-multihead-aware modules when the
   * default display is first set to a non-NULL value.
   */

  if (!bdk_display_get_default () || default_display_opened)
    return;

  default_display_opened = TRUE;

  for (slist = btk_modules; slist; slist = slist->next)
    {
      if (slist->data)
	{
	  BtkModuleInfo *info = slist->data;

	  if (!info->display_init_func)
	    (* info->init_func) (&btk_argc, &btk_argv);
	}
    }
}

static void
display_closed_cb (BdkDisplay *display,
		   bboolean    is_error)
{
  BdkScreen *screen;
  BtkSettings *settings;
  bint i;

  for (i = 0; i < bdk_display_get_n_screens (display); i++)
    {
      screen = bdk_display_get_screen (display, i);

      settings = btk_settings_get_for_screen (screen);

      g_object_set_data_full (B_OBJECT (settings),
			      I_("btk-modules"),
			      NULL, NULL);
    }  
}
		   

static void
display_opened_cb (BdkDisplayManager *display_manager,
		   BdkDisplay        *display)
{
  GSList *slist;
  BdkScreen *screen;
  BtkSettings *settings;
  bint i;

  for (slist = btk_modules; slist; slist = slist->next)
    {
      if (slist->data)
	{
	  BtkModuleInfo *info = slist->data;

	  if (info->display_init_func)
	    (* info->display_init_func) (display);
	}
    }
  
  for (i = 0; i < bdk_display_get_n_screens (display); i++)
    {
      BValue value = { 0, };

      b_value_init (&value, B_TYPE_STRING);

      screen = bdk_display_get_screen (display, i);

      if (bdk_screen_get_setting (screen, "btk-modules", &value))
	{
	  settings = btk_settings_get_for_screen (screen);
	  _btk_modules_settings_changed (settings, b_value_get_string (&value));
	  b_value_unset (&value);
	}
    }

  /* Since closing display doesn't actually release the resources yet,
   * we have to connect to the ::closed signal.
   */
  g_signal_connect (display, "closed", G_CALLBACK (display_closed_cb), NULL);
}

void
_btk_modules_init (bint        *argc, 
		   bchar     ***argv, 
		   const bchar *btk_modules_args)
{
  BdkDisplayManager *display_manager;
  bint i;

  g_assert (btk_argv == NULL);

  if (argc && argv) 
    {
      /* store argc and argv for later use in mod initialization */
      btk_argc = *argc;
      btk_argv = g_new (bchar *, *argc + 1);
      for (i = 0; i < btk_argc; i++)
	btk_argv [i] = g_strdup ((*argv) [i]);
      btk_argv [*argc] = NULL;
    }

  display_manager = bdk_display_manager_get ();
  default_display_opened = bdk_display_get_default () != NULL;
  g_signal_connect (display_manager, "notify::default-display",
		    G_CALLBACK (default_display_notify_cb), 
		    NULL);
  g_signal_connect (display_manager, "display-opened",
		    G_CALLBACK (display_opened_cb), 
		    NULL);

  if (btk_modules_args) {
    /* Modules specified in the BTK_MODULES environment variable
     * or on the command line are always loaded, so we'll just leak 
     * the refcounts.
     */
    b_slist_free (load_modules (btk_modules_args));
  }
}

static void
settings_destroy_notify (bpointer data)
{
  GSList *iter, *modules = data;

  for (iter = modules; iter; iter = iter->next) 
    {
      BtkModuleInfo *info = iter->data;
      btk_module_info_unref (info);
    }
  b_slist_free (modules);
}

void
_btk_modules_settings_changed (BtkSettings *settings, 
			       const bchar *modules)
{
  GSList *new_modules = NULL;

  BTK_NOTE (MODULES, g_print ("btk-modules setting changed to: %s\n", modules));

  /* load/ref before unreffing existing */
  if (modules && modules[0])
    new_modules = load_modules (modules);

  g_object_set_data_full (B_OBJECT (settings),
			  I_("btk-modules"),
			  new_modules,
			  settings_destroy_notify);
}
