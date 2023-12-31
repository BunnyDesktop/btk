/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#include "config.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <bunnylib/gstdio.h>
#include <bmodule.h>
#include "btkimmodule.h"
#include "btkimcontextsimple.h"
#include "btksettings.h"
#include "btkmain.h"
#include "btkrc.h"
#include "btkintl.h"
#include "btkalias.h"

/* Do *not* include "btkprivate.h" in this file. If you do, the
 * correct_libdir_prefix() and correct_localedir_prefix() functions
 * below will have to move somewhere else.
 */

#ifdef __BTK_PRIVATE_H__
#error btkprivate.h should not be included in this file
#endif

#define SIMPLE_ID "btk-im-context-simple"

/**
 * BtkIMContextInfo:
 * @context_id: The unique identification string of the input method.
 * @context_name: The human-readable name of the input method.
 * @domain: Translation domain to be used with dgettext()
 * @domain_dirname: Name of locale directory for use with bindtextdomain()
 * @default_locales: A colon-separated list of locales where this input method
 *   should be the default. The asterisk "*" sets the default for all locales.
 *
 * Bookkeeping information about a loadable input method.
 */

typedef struct _BtkIMModule      BtkIMModule;
typedef struct _BtkIMModuleClass BtkIMModuleClass;

#define BTK_TYPE_IM_MODULE          (btk_im_module_get_type ())
#define BTK_IM_MODULE(im_module)    (B_TYPE_CHECK_INSTANCE_CAST ((im_module), BTK_TYPE_IM_MODULE, BtkIMModule))
#define BTK_IS_IM_MODULE(im_module) (B_TYPE_CHECK_INSTANCE_TYPE ((im_module), BTK_TYPE_IM_MODULE))

struct _BtkIMModule
{
  GTypeModule parent_instance;
  
  bboolean builtin;

  GModule *library;

  void          (*list)   (const BtkIMContextInfo ***contexts,
 		           buint                    *n_contexts);
  void          (*init)   (GTypeModule              *module);
  void          (*exit)   (void);
  BtkIMContext *(*create) (const bchar              *context_id);

  BtkIMContextInfo **contexts;
  buint n_contexts;

  bchar *path;
};

struct _BtkIMModuleClass 
{
  GTypeModuleClass parent_class;
};

static GType btk_im_module_get_type (void);

static bint n_loaded_contexts = 0;
static GHashTable *contexts_hash = NULL;
static GSList *modules_list = NULL;

static BObjectClass *parent_class = NULL;

static bboolean
btk_im_module_load (GTypeModule *module)
{
  BtkIMModule *im_module = BTK_IM_MODULE (module);
  
  if (!im_module->builtin)
    {
      im_module->library = g_module_open (im_module->path, G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL);
      if (!im_module->library)
	{
	  g_warning ("%s", g_module_error());
	  return FALSE;
	}
  
      /* extract symbols from the lib */
      if (!g_module_symbol (im_module->library, "im_module_init",
			    (bpointer *)&im_module->init) ||
	  !g_module_symbol (im_module->library, "im_module_exit", 
			    (bpointer *)&im_module->exit) ||
	  !g_module_symbol (im_module->library, "im_module_list", 
			    (bpointer *)&im_module->list) ||
	  !g_module_symbol (im_module->library, "im_module_create", 
			    (bpointer *)&im_module->create))
	{
	  g_warning ("%s", g_module_error());
	  g_module_close (im_module->library);
	  
	  return FALSE;
	}
    }
	    
  /* call the module's init function to let it */
  /* setup anything it needs to set up. */
  im_module->init (module);

  return TRUE;
}

static void
btk_im_module_unload (GTypeModule *module)
{
  BtkIMModule *im_module = BTK_IM_MODULE (module);
  
  im_module->exit();

  if (!im_module->builtin)
    {
      g_module_close (im_module->library);
      im_module->library = NULL;

      im_module->init = NULL;
      im_module->exit = NULL;
      im_module->list = NULL;
      im_module->create = NULL;
    }
}

/* This only will ever be called if an error occurs during
 * initialization
 */
static void
btk_im_module_finalize (BObject *object)
{
  BtkIMModule *module = BTK_IM_MODULE (object);

  g_free (module->path);

  parent_class->finalize (object);
}

G_DEFINE_TYPE (BtkIMModule, btk_im_module, B_TYPE_TYPE_MODULE)

static void
btk_im_module_class_init (BtkIMModuleClass *class)
{
  GTypeModuleClass *module_class = B_TYPE_MODULE_CLASS (class);
  BObjectClass *bobject_class = B_OBJECT_CLASS (class);

  parent_class = B_OBJECT_CLASS (g_type_class_peek_parent (class));
  
  module_class->load = btk_im_module_load;
  module_class->unload = btk_im_module_unload;

  bobject_class->finalize = btk_im_module_finalize;
}

static void 
btk_im_module_init (BtkIMModule* object)
{
}

static void
free_info (BtkIMContextInfo *info)
{
  g_free ((char *)info->context_id);
  g_free ((char *)info->context_name);
  g_free ((char *)info->domain);
  g_free ((char *)info->domain_dirname);
  g_free ((char *)info->default_locales);
  g_free (info);
}

static void
add_module (BtkIMModule *module, GSList *infos)
{
  GSList *tmp_list = infos;
  bint i = 0;
  bint n = b_slist_length (infos);
  module->contexts = g_new (BtkIMContextInfo *, n);

  while (tmp_list)
    {
      BtkIMContextInfo *info = tmp_list->data;
  
      if (g_hash_table_lookup (contexts_hash, info->context_id))
	{
	  free_info (info);	/* Duplicate */
	}
      else
	{
	  g_hash_table_insert (contexts_hash, (char *)info->context_id, module);
	  module->contexts[i++] = tmp_list->data;
	  n_loaded_contexts++;
	}
      
      tmp_list = tmp_list->next;
    }
  b_slist_free (infos);
  module->n_contexts = i;

  modules_list = b_slist_prepend (modules_list, module);
}

#ifdef G_OS_WIN32

static void
correct_libdir_prefix (bchar **path)
{
  /* BTK_LIBDIR here is supposed to still have the definition from
   * Makefile.am, i.e. the build-time value. Do *not* include btkprivate.h
   * in this file.
   */
  if (strncmp (*path, BTK_LIBDIR, strlen (BTK_LIBDIR)) == 0)
    {
      /* This is an entry put there by make install on the
       * packager's system. On Windows a prebuilt BTK+
       * package can be installed in a random
       * location. The immodules.cache file distributed in
       * such a package contains paths from the package
       * builder's machine. Replace the path with the real
       * one on this machine.
       */
      extern const bchar *_btk_get_libdir ();
      bchar *tem = *path;
      *path = g_strconcat (_btk_get_libdir (), tem + strlen (BTK_LIBDIR), NULL);
      g_free (tem);
    }
}

static void
correct_localedir_prefix (bchar **path)
{
  /* As above, but for BTK_LOCALEDIR. Use separate function in case
   * BTK_LOCALEDIR isn't a subfolder of BTK_LIBDIR.
   */
  if (strncmp (*path, BTK_LOCALEDIR, strlen (BTK_LOCALEDIR)) == 0)
    {
      extern const bchar *_btk_get_localedir ();
      bchar *tem = *path;
      *path = g_strconcat (_btk_get_localedir (), tem + strlen (BTK_LOCALEDIR), NULL);
      g_free (tem);
    }
}
#endif


B_GNUC_UNUSED static BtkIMModule *
add_builtin_module (const bchar             *module_name,
		    const BtkIMContextInfo **contexts,
		    int                      n_contexts)
{
  BtkIMModule *module = g_object_new (BTK_TYPE_IM_MODULE, NULL);
  GSList *infos = NULL;
  int i;

  for (i = 0; i < n_contexts; i++)
    {
      BtkIMContextInfo *info = g_new (BtkIMContextInfo, 1);
      info->context_id = g_strdup (contexts[i]->context_id);
      info->context_name = g_strdup (contexts[i]->context_name);
      info->domain = g_strdup (contexts[i]->domain);
      info->domain_dirname = g_strdup (contexts[i]->domain_dirname);
#ifdef G_OS_WIN32
      correct_localedir_prefix ((char **) &info->domain_dirname);
#endif
      info->default_locales = g_strdup (contexts[i]->default_locales);
      infos = b_slist_prepend (infos, info);
    }

  module->builtin = TRUE;
  g_type_module_set_name (B_TYPE_MODULE (module), module_name);
  add_module (module, infos);

  return module;
}

static void
btk_im_module_initialize (void)
{
  GString *line_buf = g_string_new (NULL);
  GString *tmp_buf = g_string_new (NULL);
  bchar *filename = btk_rc_get_im_module_file();
  FILE *file;
  bboolean have_error = FALSE;

  BtkIMModule *module = NULL;
  GSList *infos = NULL;

  contexts_hash = g_hash_table_new (g_str_hash, g_str_equal);

#define do_builtin(m)							\
  {									\
    const BtkIMContextInfo **contexts;					\
    int n_contexts;							\
    extern void _btk_immodule_ ## m ## _list (const BtkIMContextInfo ***contexts, \
					      buint                    *n_contexts); \
    extern void _btk_immodule_ ## m ## _init (GTypeModule *module);	\
    extern void _btk_immodule_ ## m ## _exit (void);			\
    extern BtkIMContext *_btk_immodule_ ## m ## _create (const bchar *context_id); \
									\
    _btk_immodule_ ## m ## _list (&contexts, &n_contexts);		\
    module = add_builtin_module (#m, contexts, n_contexts);		\
    module->init = _btk_immodule_ ## m ## _init;			\
    module->exit = _btk_immodule_ ## m ## _exit;			\
    module->create = _btk_immodule_ ## m ## _create;			\
    module = NULL;							\
  }

#ifdef INCLUDE_IM_am_et
  do_builtin (am_et);
#endif
#ifdef INCLUDE_IM_cedilla
  do_builtin (cedilla);
#endif
#ifdef INCLUDE_IM_cyrillic_translit
  do_builtin (cyrillic_translit);
#endif
#ifdef INCLUDE_IM_ime
  do_builtin (ime);
#endif
#ifdef INCLUDE_IM_inuktitut
  do_builtin (inuktitut);
#endif
#ifdef INCLUDE_IM_ipa
  do_builtin (ipa);
#endif
#ifdef INCLUDE_IM_multipress
  do_builtin (multipress);
#endif
#ifdef INCLUDE_IM_thai
  do_builtin (thai);
#endif
#ifdef INCLUDE_IM_ti_er
  do_builtin (ti_er);
#endif
#ifdef INCLUDE_IM_ti_et
  do_builtin (ti_et);
#endif
#ifdef INCLUDE_IM_viqr
  do_builtin (viqr);
#endif
#ifdef INCLUDE_IM_xim
  do_builtin (xim);
#endif

#undef do_builtin

  file = g_fopen (filename, "r");
  if (!file)
    {
      /* In case someone wants only the default input method,
       * we allow no file at all.
       */
      g_string_free (line_buf, TRUE);
      g_string_free (tmp_buf, TRUE);
      g_free (filename);
      return;
    }

  while (!have_error && bango_read_line (file, line_buf))
    {
      const char *p;
      
      p = line_buf->str;

      if (!bango_skip_space (&p))
	{
	  /* Blank line marking the end of a module
	   */
	  if (module && *p != '#')
	    {
	      add_module (module, infos);
	      module = NULL;
	      infos = NULL;
	    }
	  
	  continue;
	}

      if (!module)
	{
	  /* Read a module location
	   */
	  module = g_object_new (BTK_TYPE_IM_MODULE, NULL);

	  if (!bango_scan_string (&p, tmp_buf) ||
	      bango_skip_space (&p))
	    {
	      g_warning ("Error parsing context info in '%s'\n  %s", 
			 filename, line_buf->str);
	      have_error = TRUE;
	    }

	  module->path = g_strdup (tmp_buf->str);
#ifdef G_OS_WIN32
	  correct_libdir_prefix (&module->path);
#endif
	  g_type_module_set_name (B_TYPE_MODULE (module), module->path);
	}
      else
	{
	  BtkIMContextInfo *info = g_new0 (BtkIMContextInfo, 1);
	  
	  /* Read information about a context type
	   */
	  if (!bango_scan_string (&p, tmp_buf))
	    goto context_error;
	  info->context_id = g_strdup (tmp_buf->str);

	  if (!bango_scan_string (&p, tmp_buf))
	    goto context_error;
	  info->context_name = g_strdup (tmp_buf->str);

	  if (!bango_scan_string (&p, tmp_buf))
	    goto context_error;
	  info->domain = g_strdup (tmp_buf->str);

	  if (!bango_scan_string (&p, tmp_buf))
	    goto context_error;
	  info->domain_dirname = g_strdup (tmp_buf->str);
#ifdef G_OS_WIN32
	  correct_localedir_prefix ((char **) &info->domain_dirname);
#endif

	  if (!bango_scan_string (&p, tmp_buf))
	    goto context_error;
	  info->default_locales = g_strdup (tmp_buf->str);

	  if (bango_skip_space (&p))
	    goto context_error;

	  infos = b_slist_prepend (infos, info);
	  continue;

	context_error:
	  g_warning ("Error parsing context info in '%s'\n  %s", 
		     filename, line_buf->str);
	  have_error = TRUE;
	}
    }

  if (have_error)
    {
      GSList *tmp_list = infos;
      while (tmp_list)
	{
	  free_info (tmp_list->data);
	  tmp_list = tmp_list->next;
	}
      b_slist_free (infos);

      g_object_unref (module);
    }
  else if (module)
    add_module (module, infos);

  fclose (file);
  g_string_free (line_buf, TRUE);
  g_string_free (tmp_buf, TRUE);
  g_free (filename);
}

static bint
compare_btkimcontextinfo_name(const BtkIMContextInfo **a,
			      const BtkIMContextInfo **b)
{
  return g_utf8_collate ((*a)->context_name, (*b)->context_name);
}

/**
 * _btk_im_module_list:
 * @contexts: location to store an array of pointers to #BtkIMContextInfo
 *            this array should be freed with g_free() when you are finished.
 *            The structures it points are statically allocated and should
 *            not be modified or freed.
 * @n_contexts: the length of the array stored in @contexts
 * 
 * List all available types of input method context
 */
void
_btk_im_module_list (const BtkIMContextInfo ***contexts,
		     buint                    *n_contexts)
{
  int n = 0;

  static
#ifndef G_OS_WIN32
	  const
#endif
		BtkIMContextInfo simple_context_info = {
    SIMPLE_ID,
    N_("Simple"),
    GETTEXT_PACKAGE,
#ifdef BTK_LOCALEDIR
    BTK_LOCALEDIR,
#else
    "",
#endif
    ""
  };

#ifdef G_OS_WIN32
  static bboolean beenhere = FALSE;
#endif

  if (!contexts_hash)
    btk_im_module_initialize ();

#ifdef G_OS_WIN32
  if (!beenhere)
    {
      beenhere = TRUE;
      /* correct_localedir_prefix() requires its parameter to be a
       * malloced string
       */
      simple_context_info.domain_dirname = g_strdup (simple_context_info.domain_dirname);
      correct_localedir_prefix ((char **) &simple_context_info.domain_dirname);
    }
#endif

  if (n_contexts)
    *n_contexts = (n_loaded_contexts + 1);

  if (contexts)
    {
      GSList *tmp_list;
      int i;
      
      *contexts = g_new (const BtkIMContextInfo *, n_loaded_contexts + 1);

      (*contexts)[n++] = &simple_context_info;

      tmp_list = modules_list;
      while (tmp_list)
	{
	  BtkIMModule *module = tmp_list->data;

	  for (i=0; i<module->n_contexts; i++)
	    (*contexts)[n++] = module->contexts[i];
	  
	  tmp_list = tmp_list->next;
	}

      /* fisrt element (Default) should always be at top */
      qsort ((*contexts)+1, n-1, sizeof (BtkIMContextInfo *), (GCompareFunc)compare_btkimcontextinfo_name);
    }
}

/**
 * _btk_im_module_create:
 * @context_id: the context ID for the context type to create
 * 
 * Create an IM context of a type specified by the string
 * ID @context_id.
 * 
 * Return value: a newly created input context of or @context_id, or
 *     if that could not be created, a newly created BtkIMContextSimple.
 */
BtkIMContext *
_btk_im_module_create (const bchar *context_id)
{
  BtkIMModule *im_module;
  BtkIMContext *context = NULL;
  
  if (!contexts_hash)
    btk_im_module_initialize ();

  if (strcmp (context_id, SIMPLE_ID) != 0)
    {
      im_module = g_hash_table_lookup (contexts_hash, context_id);
      if (!im_module)
	{
	  g_warning ("Attempt to load unknown IM context type '%s'", context_id);
	}
      else
	{
	  if (g_type_module_use (B_TYPE_MODULE (im_module)))
	    {
	      context = im_module->create (context_id);
	      g_type_module_unuse (B_TYPE_MODULE (im_module));
	    }
	  
	  if (!context)
	    g_warning ("Loading IM context type '%s' failed", context_id);
	}
    }
  
  if (!context)
     return btk_im_context_simple_new ();
  else
    return context;
}

/* Match @locale against @against.
 * 
 * 'en_US' against 'en_US'       => 4
 * 'en_US' against 'en'          => 3
 * 'en', 'en_UK' against 'en_US' => 2
 *  all locales, against '*' 	 => 1
 */
static bint
match_locale (const bchar *locale,
	      const bchar *against,
	      bint         against_len)
{
  if (strcmp (against, "*") == 0)
    return 1;

  if (g_ascii_strcasecmp (locale, against) == 0)
    return 4;

  if (g_ascii_strncasecmp (locale, against, 2) == 0)
    return (against_len == 2) ? 3 : 2;

  return 0;
}

static const bchar *
lookup_immodule (bchar **immodules_list)
{
  while (immodules_list && *immodules_list)
    {
      if (g_strcmp0 (*immodules_list, SIMPLE_ID) == 0)
        return SIMPLE_ID;
      else
	{
	  bboolean found;
	  bchar *context_id;
	  found = g_hash_table_lookup_extended (contexts_hash, *immodules_list,
						&context_id, NULL);
	  if (found)
	    return context_id;
	}
      immodules_list++;
    }

  return NULL;
}

/**
 * _btk_im_module_get_default_context_id:
 * @client_window: a window
 * 
 * Return the context_id of the best IM context type 
 * for the given window.
 * 
 * Return value: the context ID (will never be %NULL)
 */
const bchar *
_btk_im_module_get_default_context_id (BdkWindow *client_window)
{
  GSList *tmp_list;
  const bchar *context_id = NULL;
  bint best_goodness = 0;
  bint i;
  bchar *tmp_locale, *tmp, **immodules;
  const bchar *envvar;
  BdkScreen *screen;
  BtkSettings *settings;
      
  if (!contexts_hash)
    btk_im_module_initialize ();

  envvar = g_getenv("BTK_IM_MODULE");
  if (envvar)
    {
        immodules = g_strsplit(envvar, ":", 0);
        context_id = lookup_immodule(immodules);
        g_strfreev(immodules);

        if (context_id)
          return context_id;
    }

  /* Check if the certain immodule is set in XSETTINGS.
   */
  if (BDK_IS_DRAWABLE (client_window))
    {
      screen = bdk_window_get_screen (client_window);
      settings = btk_settings_get_for_screen (screen);
      g_object_get (B_OBJECT (settings), "btk-im-module", &tmp, NULL);
      if (tmp)
        {
          immodules = g_strsplit(tmp, ":", 0);
          context_id = lookup_immodule(immodules);
          g_strfreev(immodules);
          g_free (tmp);

       	  if (context_id)
            return context_id;
        }
    }

  /* Strip the locale code down to the essentials
   */
  tmp_locale = _btk_get_lc_ctype ();
  tmp = strchr (tmp_locale, '.');
  if (tmp)
    *tmp = '\0';
  tmp = strchr (tmp_locale, '@');
  if (tmp)
    *tmp = '\0';
  
  tmp_list = modules_list;
  while (tmp_list)
    {
      BtkIMModule *module = tmp_list->data;
     
      for (i = 0; i < module->n_contexts; i++)
	{
	  const bchar *p = module->contexts[i]->default_locales;
	  while (p)
	    {
	      const bchar *q = strchr (p, ':');
	      bint goodness = match_locale (tmp_locale, p, q ? q - p : strlen (p));

	      if (goodness > best_goodness)
		{
		  context_id = module->contexts[i]->context_id;
		  best_goodness = goodness;
		}

	      p = q ? q + 1 : NULL;
	    }
	}
      
      tmp_list = tmp_list->next;
    }

  g_free (tmp_locale);
  
  return context_id ? context_id : SIMPLE_ID;
}
