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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#include "config.h"

#include "btkmain.h"

#include <bunnylib.h>
#include "bdkconfig.h"

#include <locale.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>		/* For uid_t, gid_t */

#ifdef G_OS_WIN32
#define STRICT
#include <windows.h>
#undef STRICT
#endif

#include "btkintl.h"

#include "btkaccelmap.h"
#include "btkbox.h"
#include "btkclipboard.h"
#include "btkdnd.h"
#include "btkversion.h"
#include "btkmodules.h"
#include "btkrc.h"
#include "btkrecentmanager.h"
#include "btkselection.h"
#include "btksettings.h"
#include "btkwidget.h"
#include "btkwindow.h"
#include "btktooltip.h"
#include "btkdebug.h"
#include "btkalias.h"
#include "btkmenu.h"
#include "bdk/bdkkeysyms.h"

#include "bdk/bdkprivate.h" /* for BDK_WINDOW_DESTROYED */

#ifdef G_OS_WIN32

static HMODULE btk_dll;

BOOL WINAPI
DllMain (HINSTANCE hinstDLL,
	 DWORD     fdwReason,
	 LPVOID    lpvReserved)
{
  switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
      btk_dll = (HMODULE) hinstDLL;
      break;
    }

  return TRUE;
}

/* These here before inclusion of btkprivate.h so that the original
 * BTK_LIBDIR and BTK_LOCALEDIR definitions are seen. Yeah, this is a
 * bit sucky.
 */
const bchar *
_btk_get_libdir (void)
{
  static char *btk_libdir = NULL;
  if (btk_libdir == NULL)
    {
      bchar *root = g_win32_get_package_installation_directory_of_module (btk_dll);
      bchar *slash = root ? strrchr (root, '\\') : NULL;
      if (slash != NULL &&
          g_ascii_strcasecmp (slash + 1, ".libs") == 0)
	btk_libdir = BTK_LIBDIR;
      else
	btk_libdir = g_build_filename (root, "lib", NULL);
      g_free (root);
    }

  return btk_libdir;
}

const bchar *
_btk_get_localedir (void)
{
  static char *btk_localedir = NULL;
  if (btk_localedir == NULL)
    {
      const bchar *p;
      bchar *root, *temp;
      
      /* BTK_LOCALEDIR ends in either /lib/locale or
       * /share/locale. Scan for that slash.
       */
      p = BTK_LOCALEDIR + strlen (BTK_LOCALEDIR);
      while (*--p != '/')
	;
      while (*--p != '/')
	;

      root = g_win32_get_package_installation_directory_of_module (btk_dll);
      temp = g_build_filename (root, p, NULL);
      g_free (root);

      /* btk_localedir is passed to bindtextdomain() which isn't
       * UTF-8-aware.
       */
      btk_localedir = g_win32_locale_filename_from_utf8 (temp);
      g_free (temp);
    }
  return btk_localedir;
}

#endif

#include "btkprivate.h"

/* Private type definitions
 */
typedef struct _BtkInitFunction		 BtkInitFunction;
typedef struct _BtkQuitFunction		 BtkQuitFunction;
typedef struct _BtkClosure	         BtkClosure;
typedef struct _BtkKeySnooperData	 BtkKeySnooperData;

struct _BtkInitFunction
{
  BtkFunction function;
  bpointer data;
};

struct _BtkQuitFunction
{
  buint id;
  buint main_level;
  BtkCallbackMarshal marshal;
  BtkFunction function;
  bpointer data;
  GDestroyNotify destroy;
};

struct _BtkClosure
{
  BtkCallbackMarshal marshal;
  bpointer data;
  GDestroyNotify destroy;
};

struct _BtkKeySnooperData
{
  BtkKeySnoopFunc func;
  bpointer func_data;
  buint id;
};

static bint  btk_quit_invoke_function	 (BtkQuitFunction    *quitf);
static void  btk_quit_destroy		 (BtkQuitFunction    *quitf);
static bint  btk_invoke_key_snoopers	 (BtkWidget	     *grab_widget,
					  BdkEvent	     *event);

static void     btk_destroy_closure      (bpointer            data);
static bboolean btk_invoke_idle_timeout  (bpointer            data);
static void     btk_invoke_input         (bpointer            data,
					  bint                source,
					  BdkInputCondition   condition);

#if 0
static void  btk_error			 (bchar		     *str);
static void  btk_warning		 (bchar		     *str);
static void  btk_message		 (bchar		     *str);
static void  btk_print			 (bchar		     *str);
#endif

static BtkWindowGroup *btk_main_get_window_group (BtkWidget   *widget);

const buint btk_major_version = BTK_MAJOR_VERSION;
const buint btk_minor_version = BTK_MINOR_VERSION;
const buint btk_micro_version = BTK_MICRO_VERSION;
const buint btk_binary_age = BTK_BINARY_AGE;
const buint btk_interface_age = BTK_INTERFACE_AGE;

static buint btk_main_loop_level = 0;
static bint pre_initialized = FALSE;
static bint btk_initialized = FALSE;
static GList *current_events = NULL;

static GSList *main_loops = NULL;      /* stack of currently executing main loops */

static GList *init_functions = NULL;	   /* A list of init functions.
					    */
static GList *quit_functions = NULL;	   /* A list of quit functions.
					    */
static GSList *key_snoopers = NULL;

buint btk_debug_flags = 0;		   /* Global BTK debug flag */

#ifdef G_ENABLE_DEBUG
static const GDebugKey btk_debug_keys[] = {
  {"misc", BTK_DEBUG_MISC},
  {"plugsocket", BTK_DEBUG_PLUGSOCKET},
  {"text", BTK_DEBUG_TEXT},
  {"tree", BTK_DEBUG_TREE},
  {"updates", BTK_DEBUG_UPDATES},
  {"keybindings", BTK_DEBUG_KEYBINDINGS},
  {"multihead", BTK_DEBUG_MULTIHEAD},
  {"modules", BTK_DEBUG_MODULES},
  {"geometry", BTK_DEBUG_GEOMETRY},
  {"icontheme", BTK_DEBUG_ICONTHEME},
  {"printing", BTK_DEBUG_PRINTING},
  {"builder", BTK_DEBUG_BUILDER}
};
#endif /* G_ENABLE_DEBUG */

/**
 * btk_check_version:
 * @required_major: the required major version.
 * @required_minor: the required minor version.
 * @required_micro: the required micro version.
 * 
 * Checks that the BTK+ library in use is compatible with the
 * given version. Generally you would pass in the constants
 * #BTK_MAJOR_VERSION, #BTK_MINOR_VERSION, #BTK_MICRO_VERSION
 * as the three arguments to this function; that produces
 * a check that the library in use is compatible with
 * the version of BTK+ the application or module was compiled
 * against.
 *
 * Compatibility is defined by two things: first the version
 * of the running library is newer than the version
 * @required_major.required_minor.@required_micro. Second
 * the running library must be binary compatible with the
 * version @required_major.required_minor.@required_micro
 * (same major version.)
 *
 * This function is primarily for BTK+ modules; the module
 * can call this function to check that it wasn't loaded
 * into an incompatible version of BTK+. However, such a
 * a check isn't completely reliable, since the module may be
 * linked against an old version of BTK+ and calling the
 * old version of btk_check_version(), but still get loaded
 * into an application using a newer version of BTK+.
 *
 * Return value: %NULL if the BTK+ library is compatible with the
 *   given version, or a string describing the version mismatch.
 *   The returned string is owned by BTK+ and should not be modified
 *   or freed.
 **/
const bchar*
btk_check_version (buint required_major,
		   buint required_minor,
		   buint required_micro)
{
  bint btk_effective_micro = 100 * BTK_MINOR_VERSION + BTK_MICRO_VERSION;
  bint required_effective_micro = 100 * required_minor + required_micro;

  if (required_major > BTK_MAJOR_VERSION)
    return "Btk+ version too old (major mismatch)";
  if (required_major < BTK_MAJOR_VERSION)
    return "Btk+ version too new (major mismatch)";
  if (required_effective_micro < btk_effective_micro - BTK_BINARY_AGE)
    return "Btk+ version too new (micro mismatch)";
  if (required_effective_micro > btk_effective_micro)
    return "Btk+ version too old (micro mismatch)";
  return NULL;
}

/* This checks to see if the process is running suid or sgid
 * at the current time. If so, we don't allow BTK+ to be initialized.
 * This is meant to be a mild check - we only error out if we
 * can prove the programmer is doing something wrong, not if
 * they could be doing something wrong. For this reason, we
 * don't use issetugid() on BSD or prctl (PR_GET_DUMPABLE).
 */
static bboolean
check_setugid (void)
{
/* this isn't at all relevant on MS Windows and doesn't compile ... --hb */
#ifndef G_OS_WIN32
  uid_t ruid, euid, suid; /* Real, effective and saved user ID's */
  gid_t rgid, egid, sgid; /* Real, effective and saved group ID's */
  
#ifdef HAVE_GETRESUID
  /* These aren't in the header files, so we prototype them here.
   */
  int getresuid(uid_t *ruid, uid_t *euid, uid_t *suid);
  int getresgid(gid_t *rgid, gid_t *egid, gid_t *sgid);

  if (getresuid (&ruid, &euid, &suid) != 0 ||
      getresgid (&rgid, &egid, &sgid) != 0)
#endif /* HAVE_GETRESUID */
    {
      suid = ruid = getuid ();
      sgid = rgid = getgid ();
      euid = geteuid ();
      egid = getegid ();
    }

  if (ruid != euid || ruid != suid ||
      rgid != egid || rgid != sgid)
    {
      g_warning ("This process is currently running setuid or setgid.\n"
		 "This is not a supported use of BTK+. You must create a helper\n"
		 "program instead. For further details, see:\n\n"
		 "    http://www.btk.org/setuid.html\n\n"
		 "Refusing to initialize BTK+.");
      exit (1);
    }
#endif
  return TRUE;
}

#ifdef G_OS_WIN32

const bchar *
_btk_get_datadir (void)
{
  static char *btk_datadir = NULL;
  if (btk_datadir == NULL)
    {
      bchar *root = g_win32_get_package_installation_directory_of_module (btk_dll);
      btk_datadir = g_build_filename (root, "share", NULL);
      g_free (root);
    }

  return btk_datadir;
}

const bchar *
_btk_get_sysconfdir (void)
{
  static char *btk_sysconfdir = NULL;
  if (btk_sysconfdir == NULL)
    {
      bchar *root = g_win32_get_package_installation_directory_of_module (btk_dll);
      btk_sysconfdir = g_build_filename (root, "etc", NULL);
      g_free (root);
    }

  return btk_sysconfdir;
}

const bchar *
_btk_get_data_prefix (void)
{
  static char *btk_data_prefix = NULL;
  if (btk_data_prefix == NULL)
    btk_data_prefix = g_win32_get_package_installation_directory_of_module (btk_dll);

  return btk_data_prefix;
}

#endif /* G_OS_WIN32 */

static bboolean do_setlocale = TRUE;

/**
 * btk_disable_setlocale:
 * 
 * Prevents btk_init(), btk_init_check(), btk_init_with_args() and
 * btk_parse_args() from automatically
 * calling <literal>setlocale (LC_ALL, "")</literal>. You would
 * want to use this function if you wanted to set the locale for
 * your program to something other than the user's locale, or if
 * you wanted to set different values for different locale categories.
 *
 * Most programs should not need to call this function.
 **/
void
btk_disable_setlocale (void)
{
  if (pre_initialized)
    g_warning ("btk_disable_setlocale() must be called before btk_init()");
    
  do_setlocale = FALSE;
}

#ifdef G_PLATFORM_WIN32
#undef btk_init_check
#endif

static GString *btk_modules_string = NULL;
static bboolean g_fatal_warnings = FALSE;

#ifdef G_ENABLE_DEBUG
static bboolean
btk_arg_debug_cb (const char *key, const char *value, bpointer user_data)
{
  btk_debug_flags |= g_parse_debug_string (value,
					   btk_debug_keys,
					   G_N_ELEMENTS (btk_debug_keys));

  return TRUE;
}

static bboolean
btk_arg_no_debug_cb (const char *key, const char *value, bpointer user_data)
{
  btk_debug_flags &= ~g_parse_debug_string (value,
					    btk_debug_keys,
					    G_N_ELEMENTS (btk_debug_keys));

  return TRUE;
}
#endif /* G_ENABLE_DEBUG */

static bboolean
btk_arg_module_cb (const char *key, const char *value, bpointer user_data)
{
  if (value && *value)
    {
      if (btk_modules_string)
	g_string_append_c (btk_modules_string, G_SEARCHPATH_SEPARATOR);
      else
	btk_modules_string = g_string_new (NULL);
      
      g_string_append (btk_modules_string, value);
    }

  return TRUE;
}

static const GOptionEntry btk_args[] = {
  { "btk-module",       0, 0, G_OPTION_ARG_CALLBACK, btk_arg_module_cb,   
    /* Description of --btk-module=MODULES in --help output */ N_("Load additional BTK+ modules"), 
    /* Placeholder in --btk-module=MODULES in --help output */ N_("MODULES") },
  { "g-fatal-warnings", 0, 0, G_OPTION_ARG_NONE, &g_fatal_warnings, 
    /* Description of --g-fatal-warnings in --help output */   N_("Make all warnings fatal"), NULL },
#ifdef G_ENABLE_DEBUG
  { "btk-debug",        0, 0, G_OPTION_ARG_CALLBACK, btk_arg_debug_cb,    
    /* Description of --btk-debug=FLAGS in --help output */    N_("BTK+ debugging flags to set"), 
    /* Placeholder in --btk-debug=FLAGS in --help output */    N_("FLAGS") },
  { "btk-no-debug",     0, 0, G_OPTION_ARG_CALLBACK, btk_arg_no_debug_cb, 
    /* Description of --btk-no-debug=FLAGS in --help output */ N_("BTK+ debugging flags to unset"), 
    /* Placeholder in --btk-no-debug=FLAGS in --help output */ N_("FLAGS") },
#endif 
  { NULL }
};

#ifdef G_OS_WIN32

static char *iso639_to_check = NULL;
static char *iso3166_to_check = NULL;
static char *script_to_check = NULL;
static bboolean setlocale_called = FALSE;

static BOOL CALLBACK
enum_locale_proc (LPTSTR locale)
{
  LCID lcid;
  char iso639[10];
  char iso3166[10];
  char *endptr;


  lcid = strtoul (locale, &endptr, 16);
  if (*endptr == '\0' &&
      GetLocaleInfo (lcid, LOCALE_SISO639LANGNAME, iso639, sizeof (iso639)) &&
      GetLocaleInfo (lcid, LOCALE_SISO3166CTRYNAME, iso3166, sizeof (iso3166)))
    {
      if (strcmp (iso639, iso639_to_check) == 0 &&
	  ((iso3166_to_check != NULL &&
	    strcmp (iso3166, iso3166_to_check) == 0) ||
	   (iso3166_to_check == NULL &&
	    SUBLANGID (LANGIDFROMLCID (lcid)) == SUBLANG_DEFAULT)))
	{
	  char language[100], country[100];
	  char locale[300];

	  if (script_to_check != NULL)
	    {
	      /* If lcid is the "other" script for this language,
	       * return TRUE, i.e. continue looking.
	       */
	      if (strcmp (script_to_check, "Latn") == 0)
		{
		  switch (LANGIDFROMLCID (lcid))
		    {
		    case MAKELANGID (LANG_AZERI, SUBLANG_AZERI_CYRILLIC):
		      return TRUE;
		    case MAKELANGID (LANG_UZBEK, SUBLANG_UZBEK_CYRILLIC):
		      return TRUE;
		    case MAKELANGID (LANG_SERBIAN, SUBLANG_SERBIAN_CYRILLIC):
		      return TRUE;
		    case MAKELANGID (LANG_SERBIAN, 0x07):
		      /* Serbian in Bosnia and Herzegovina, Cyrillic */
		      return TRUE;
		    }
		}
	      else if (strcmp (script_to_check, "Cyrl") == 0)
		{
		  switch (LANGIDFROMLCID (lcid))
		    {
		    case MAKELANGID (LANG_AZERI, SUBLANG_AZERI_LATIN):
		      return TRUE;
		    case MAKELANGID (LANG_UZBEK, SUBLANG_UZBEK_LATIN):
		      return TRUE;
		    case MAKELANGID (LANG_SERBIAN, SUBLANG_SERBIAN_LATIN):
		      return TRUE;
		    case MAKELANGID (LANG_SERBIAN, 0x06):
		      /* Serbian in Bosnia and Herzegovina, Latin */
		      return TRUE;
		    }
		}
	    }

	  SetThreadLocale (lcid);

	  if (GetLocaleInfo (lcid, LOCALE_SENGLANGUAGE, language, sizeof (language)) &&
	      GetLocaleInfo (lcid, LOCALE_SENGCOUNTRY, country, sizeof (country)))
	    {
	      strcpy (locale, language);
	      strcat (locale, "_");
	      strcat (locale, country);

	      if (setlocale (LC_ALL, locale) != NULL)
		setlocale_called = TRUE;
	    }

	  return FALSE;
	}
    }

  return TRUE;
}
  
#endif

static void
setlocale_initialization (void)
{
  static bboolean initialized = FALSE;

  if (initialized)
    return;
  initialized = TRUE;

  if (do_setlocale)
    {
#ifdef G_OS_WIN32
      /* If some of the POSIXish environment variables are set, set
       * the Win32 thread locale correspondingly.
       */ 
      char *p = getenv ("LC_ALL");
      if (p == NULL)
	p = getenv ("LANG");

      if (p != NULL)
	{
	  p = g_strdup (p);
	  if (strcmp (p, "C") == 0)
	    SetThreadLocale (LOCALE_SYSTEM_DEFAULT);
	  else
	    {
	      /* Check if one of the supported locales match the
	       * environment variable. If so, use that locale.
	       */
	      iso639_to_check = p;
	      iso3166_to_check = strchr (iso639_to_check, '_');
	      if (iso3166_to_check != NULL)
		{
		  *iso3166_to_check++ = '\0';

		  script_to_check = strchr (iso3166_to_check, '@');
		  if (script_to_check != NULL)
		    *script_to_check++ = '\0';

		  /* Handle special cases. */
		  
		  /* The standard code for Serbia and Montenegro was
		   * "CS", but MSFT uses for some reason "SP". By now
		   * (October 2006), SP has split into two, "RS" and
		   * "ME", but don't bother trying to handle those
		   * yet. Do handle the even older "YU", though.
		   */
		  if (strcmp (iso3166_to_check, "CS") == 0 ||
		      strcmp (iso3166_to_check, "YU") == 0)
		    iso3166_to_check = "SP";
		}
	      else
		{
		  script_to_check = strchr (iso639_to_check, '@');
		  if (script_to_check != NULL)
		    *script_to_check++ = '\0';
		  /* LANG_SERBIAN == LANG_CROATIAN, recognize just "sr" */
		  if (strcmp (iso639_to_check, "sr") == 0)
		    iso3166_to_check = "SP";
		}

	      EnumSystemLocales (enum_locale_proc, LCID_SUPPORTED);
	    }
	  g_free (p);
	}
      if (!setlocale_called)
	setlocale (LC_ALL, "");
#else
      if (!setlocale (LC_ALL, ""))
	g_warning ("Locale not supported by C library.\n\tUsing the fallback 'C' locale.");
#endif
    }
}

/* Return TRUE if module_to_check causes version conflicts.
 * If module_to_check is NULL, check the main module.
 */
bboolean
_btk_module_has_mixed_deps (GModule *module_to_check)
{
  GModule *module;
  bpointer func;
  bboolean result;

  if (!module_to_check)
    module = g_module_open (NULL, 0);
  else
    module = module_to_check;

  if (g_module_symbol (module, "btk_widget_device_is_shadowed", &func))
    result = TRUE;
  else
    result = FALSE;

  if (!module_to_check)
    g_module_close (module);

  return result;
}

static void
do_pre_parse_initialization (int    *argc,
			     char ***argv)
{
  const bchar *env_string;
  
#if	0
  g_set_error_handler (btk_error);
  g_set_warning_handler (btk_warning);
  g_set_message_handler (btk_message);
  g_set_print_handler (btk_print);
#endif

  if (pre_initialized)
    return;

  pre_initialized = TRUE;

  if (_btk_module_has_mixed_deps (NULL))
    g_error ("BTK+ 2.x symbols detected. Using BTK+ 2.x and BTK+ 3 in the same process is not supported");

  bdk_pre_parse_libbtk_only ();
  bdk_event_handler_set ((BdkEventFunc)btk_main_do_event, NULL, NULL);
  
#ifdef G_ENABLE_DEBUG
  env_string = g_getenv ("BTK_DEBUG");
  if (env_string != NULL)
    {
      btk_debug_flags = g_parse_debug_string (env_string,
					      btk_debug_keys,
					      G_N_ELEMENTS (btk_debug_keys));
      env_string = NULL;
    }
#endif	/* G_ENABLE_DEBUG */

  env_string = g_getenv ("BTK2_MODULES");
  if (env_string)
    btk_modules_string = g_string_new (env_string);

  env_string = g_getenv ("BTK_MODULES");
  if (env_string)
    {
      if (btk_modules_string)
        g_string_append_c (btk_modules_string, G_SEARCHPATH_SEPARATOR);
      else
        btk_modules_string = g_string_new (NULL);

      g_string_append (btk_modules_string, env_string);
    }
}

static void
gettext_initialization (void)
{
  setlocale_initialization ();

#ifdef ENABLE_NLS
  bindtextdomain (GETTEXT_PACKAGE, BTK_LOCALEDIR);
  bindtextdomain (GETTEXT_PACKAGE "-properties", BTK_LOCALEDIR);
#    ifdef HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  bind_textdomain_codeset (GETTEXT_PACKAGE "-properties", "UTF-8");
#    endif
#endif  
}

static void
do_post_parse_initialization (int    *argc,
			      char ***argv)
{
  if (btk_initialized)
    return;

  gettext_initialization ();

#ifdef SIGPIPE
  signal (SIGPIPE, SIG_IGN);
#endif

  if (g_fatal_warnings)
    {
      GLogLevelFlags fatal_mask;

      fatal_mask = g_log_set_always_fatal (G_LOG_FATAL_MASK);
      fatal_mask |= G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL;
      g_log_set_always_fatal (fatal_mask);
    }

  if (btk_debug_flags & BTK_DEBUG_UPDATES)
    bdk_window_set_debug_updates (TRUE);

  {
  /* Translate to default:RTL if you want your widgets
   * to be RTL, otherwise translate to default:LTR.
   * Do *not* translate it to "predefinito:LTR", if it
   * it isn't default:LTR or default:RTL it will not work 
   */
    char *e = _("default:LTR");
    if (strcmp (e, "default:RTL")==0) 
      btk_widget_set_default_direction (BTK_TEXT_DIR_RTL);
    else if (strcmp (e, "default:LTR"))
      g_warning ("Whoever translated default:LTR did so wrongly.\n");
  }

  /* do what the call to btk_type_init() used to do */
  g_type_init ();

  _btk_accel_map_init ();
  _btk_rc_init ();

  /* Set the 'initialized' flag.
   */
  btk_initialized = TRUE;

  /* load btk modules */
  if (btk_modules_string)
    {
      _btk_modules_init (argc, argv, btk_modules_string->str);
      g_string_free (btk_modules_string, TRUE);
    }
  else
    {
      _btk_modules_init (argc, argv, NULL);
    }
}


typedef struct
{
  bboolean open_default_display;
} OptionGroupInfo;

static bboolean
pre_parse_hook (GOptionContext *context,
		GOptionGroup   *group,
		bpointer	data,
		GError        **error)
{
  do_pre_parse_initialization (NULL, NULL);
  
  return TRUE;
}

static bboolean
post_parse_hook (GOptionContext *context,
		 GOptionGroup   *group,
		 bpointer	data,
		 GError        **error)
{
  OptionGroupInfo *info = data;

  
  do_post_parse_initialization (NULL, NULL);
  
  if (info->open_default_display)
    {
      if (bdk_display_open_default_libbtk_only () == NULL)
	{
	  const char *display_name = bdk_get_display_arg_name ();
	  g_set_error (error, 
		       G_OPTION_ERROR, 
		       G_OPTION_ERROR_FAILED,
		       _("Cannot open display: %s"),
		       display_name ? display_name : "" );
	  
	  return FALSE;
	}
    }

  return TRUE;
}


/**
 * btk_get_option_group:
 * @open_default_display: whether to open the default display 
 *    when parsing the commandline arguments
 * 
 * Returns a #GOptionGroup for the commandline arguments recognized
 * by BTK+ and BDK. You should add this group to your #GOptionContext 
 * with g_option_context_add_group(), if you are using 
 * g_option_context_parse() to parse your commandline arguments.
 *
 * Returns: a #GOptionGroup for the commandline arguments recognized
 *   by BTK+
 *
 * Since: 2.6
 */
GOptionGroup *
btk_get_option_group (bboolean open_default_display)
{
  GOptionGroup *group;
  OptionGroupInfo *info;

  gettext_initialization ();

  info = g_new0 (OptionGroupInfo, 1);
  info->open_default_display = open_default_display;
  
  group = g_option_group_new ("btk", _("BTK+ Options"), _("Show BTK+ Options"), info, g_free);
  g_option_group_set_parse_hooks (group, pre_parse_hook, post_parse_hook);

  bdk_add_option_entries_libbtk_only (group);
  g_option_group_add_entries (group, btk_args);
  g_option_group_set_translation_domain (group, GETTEXT_PACKAGE);
  
  return group;
}

/**
 * btk_init_with_args:
 * @argc: a pointer to the number of command line arguments.
 * @argv: (inout) (array length=argc): a pointer to the array of
 *    command line arguments.
 * @parameter_string: a string which is displayed in
 *    the first line of <option>--help</option> output, after 
 *    <literal><replaceable>programname</replaceable> [OPTION...]</literal>
 * @entries: (array zero-terminated=1):  a %NULL-terminated array
 *    of #GOptionEntry<!-- -->s describing the options of your program
 * @translation_domain: a translation domain to use for translating
 *    the <option>--help</option> output for the options in @entries
 *    and the @parameter_string with gettext(), or %NULL
 * @error: a return location for errors 
 *
 * This function does the same work as btk_init_check(). 
 * Additionally, it allows you to add your own commandline options, 
 * and it automatically generates nicely formatted 
 * <option>--help</option> output. Note that your program will
 * be terminated after writing out the help output.
 *
 * Returns: %TRUE if the GUI has been successfully initialized, 
 *               %FALSE otherwise.
 * 
 * Since: 2.6
 */
bboolean
btk_init_with_args (int            *argc,
		    char         ***argv,
		    const char     *parameter_string,
		    GOptionEntry   *entries,
		    const char     *translation_domain,
		    GError        **error)
{
  GOptionContext *context;
  GOptionGroup *btk_group;
  bboolean retval;

  if (btk_initialized)
    return bdk_display_open_default_libbtk_only () != NULL;

  gettext_initialization ();

  if (!check_setugid ())
    return FALSE;

  btk_group = btk_get_option_group (TRUE);
  
  context = g_option_context_new (parameter_string);
  g_option_context_add_group (context, btk_group);
  
  g_option_context_set_translation_domain (context, translation_domain);

  if (entries)
    g_option_context_add_main_entries (context, entries, translation_domain);
  retval = g_option_context_parse (context, argc, argv, error);
  
  g_option_context_free (context);

  return retval;
}


/**
 * btk_parse_args:
 * @argc: (inout): a pointer to the number of command line arguments
 * @argv: (array length=argc) (inout): a pointer to the array of
 *     command line arguments
 *
 * Parses command line arguments, and initializes global
 * attributes of BTK+, but does not actually open a connection
 * to a display. (See bdk_display_open(), bdk_get_display_arg_name())
 *
 * Any arguments used by BTK+ or BDK are removed from the array and
 * @argc and @argv are updated accordingly.
 *
 * There is no need to call this function explicitely if you are using
 * btk_init(), or btk_init_check().
 *
 * Return value: %TRUE if initialization succeeded, otherwise %FALSE.
 **/
bboolean
btk_parse_args (int    *argc,
		char ***argv)
{
  GOptionContext *option_context;
  GOptionGroup *btk_group;
  GError *error = NULL;
  
  if (btk_initialized)
    return TRUE;

  gettext_initialization ();

  if (!check_setugid ())
    return FALSE;

  option_context = g_option_context_new (NULL);
  g_option_context_set_ignore_unknown_options (option_context, TRUE);
  g_option_context_set_help_enabled (option_context, FALSE);
  btk_group = btk_get_option_group (FALSE);
  g_option_context_set_main_group (option_context, btk_group);
  if (!g_option_context_parse (option_context, argc, argv, &error))
    {
      g_warning ("%s", error->message);
      g_error_free (error);
    }

  g_option_context_free (option_context);

  return TRUE;
}

#ifdef G_PLATFORM_WIN32
#undef btk_init_check
#endif

/**
 * btk_init_check:
 * @argc: (inout): Address of the <parameter>argc</parameter> parameter of your
 *   main() function. Changed if any arguments were handled.
 * @argv: (array length=argc) (inout) (allow-none): Address of the <parameter>argv</parameter> parameter of main().
 *   Any parameters understood by btk_init() are stripped before return.
 *
 * This function does the same work as btk_init() with only
 * a single change: It does not terminate the program if the GUI can't be
 * initialized. Instead it returns %FALSE on failure.
 *
 * This way the application can fall back to some other means of communication 
 * with the user - for example a curses or command line interface.
 * 
 * Return value: %TRUE if the GUI has been successfully initialized, 
 *               %FALSE otherwise.
 **/
bboolean
btk_init_check (int	 *argc,
		char   ***argv)
{
  if (!btk_parse_args (argc, argv))
    return FALSE;

  return bdk_display_open_default_libbtk_only () != NULL;
}

#ifdef G_PLATFORM_WIN32
#undef btk_init
#endif

/**
 * btk_init:
 * @argc: (inout): Address of the <parameter>argc</parameter> parameter of
 *     your main() function. Changed if any arguments were handled
 * @argv: (array length=argc) (inout) (allow-none): Address of the
 *     <parameter>argv</parameter> parameter of main(). Any options
 *     understood by BTK+ are stripped before return.
 *
 * Call this function before using any other BTK+ functions in your GUI
 * applications.  It will initialize everything needed to operate the
 * toolkit and parses some standard command line options.
 *
 * @argc and @argv are adjusted accordingly so your own code will
 * never see those standard arguments.
 *
 * Note that there are some alternative ways to initialize BTK+:
 * if you are calling btk_parse_args(), btk_init_check(),
 * btk_init_with_args() or g_option_context_parse() with
 * the option group returned by btk_get_option_group(),
 * you <emphasis>don't</emphasis> have to call btk_init().
 *
 * <note><para>
 * This function will terminate your program if it was unable to
 * initialize the windowing system for some reason. If you want
 * your program to fall back to a textual interface you want to
 * call btk_init_check() instead.
 * </para></note>
 *
 * <note><para>
 * Since 2.18, BTK+ calls <literal>signal (SIGPIPE, SIG_IGN)</literal>
 * during initialization, to ignore SIGPIPE signals, since these are
 * almost never wanted in graphical applications. If you do need to
 * handle SIGPIPE for some reason, reset the handler after btk_init(),
 * but notice that other libraries (e.g. libdbus or gvfs) might do
 * similar things.
 * </para></note>
 */
void
btk_init (int *argc, char ***argv)
{
  if (!btk_init_check (argc, argv))
    {
      const char *display_name_arg = bdk_get_display_arg_name ();
      if (display_name_arg == NULL)
        display_name_arg = getenv("DISPLAY");
      g_warning ("cannot open display: %s", display_name_arg ? display_name_arg : "");
      exit (1);
    }
}

#ifdef G_PLATFORM_WIN32

static void
check_sizeof_BtkWindow (size_t sizeof_BtkWindow)
{
  if (sizeof_BtkWindow != sizeof (BtkWindow))
    g_error ("Incompatible build!\n"
	     "The code using BTK+ thinks BtkWindow is of different\n"
             "size than it actually is in this build of BTK+.\n"
	     "On Windows, this probably means that you have compiled\n"
	     "your code with gcc without the -mms-bitfields switch,\n"
	     "or that you are using an unsupported compiler.");
}

/* In BTK+ 2.0 the BtkWindow struct actually is the same size in
 * gcc-compiled code on Win32 whether compiled with -fnative-struct or
 * not. Unfortunately this wan't noticed until after BTK+ 2.0.1. So,
 * from BTK+ 2.0.2 on, check some other struct, too, where the use of
 * -fnative-struct still matters. BtkBox is one such.
 */
static void
check_sizeof_BtkBox (size_t sizeof_BtkBox)
{
  if (sizeof_BtkBox != sizeof (BtkBox))
    g_error ("Incompatible build!\n"
	     "The code using BTK+ thinks BtkBox is of different\n"
             "size than it actually is in this build of BTK+.\n"
	     "On Windows, this probably means that you have compiled\n"
	     "your code with gcc without the -mms-bitfields switch,\n"
	     "or that you are using an unsupported compiler.");
}

/* These two functions might get more checks added later, thus pass
 * in the number of extra args.
 */
void
btk_init_abi_check (int *argc, char ***argv, int num_checks, size_t sizeof_BtkWindow, size_t sizeof_BtkBox)
{
  check_sizeof_BtkWindow (sizeof_BtkWindow);
  if (num_checks >= 2)
    check_sizeof_BtkBox (sizeof_BtkBox);
  btk_init (argc, argv);
}

bboolean
btk_init_check_abi_check (int *argc, char ***argv, int num_checks, size_t sizeof_BtkWindow, size_t sizeof_BtkBox)
{
  check_sizeof_BtkWindow (sizeof_BtkWindow);
  if (num_checks >= 2)
    check_sizeof_BtkBox (sizeof_BtkBox);
  return btk_init_check (argc, argv);
}

#endif

void
btk_exit (bint errorcode)
{
  exit (errorcode);
}


/**
 * btk_set_locale:
 *
 * Initializes internationalization support for BTK+. btk_init()
 * automatically does this, so there is typically no point
 * in calling this function.
 *
 * If you are calling this function because you changed the locale
 * after BTK+ is was initialized, then calling this function
 * may help a bit. (Note, however, that changing the locale
 * after BTK+ is initialized may produce inconsistent results and
 * is not really supported.)
 * 
 * In detail - sets the current locale according to the
 * program environment. This is the same as calling the C library function
 * <literal>setlocale (LC_ALL, "")</literal> but also takes care of the 
 * locale specific setup of the windowing system used by BDK.
 * 
 * Returns: a string corresponding to the locale set, typically in the
 * form lang_COUNTRY, where lang is an ISO-639 language code, and
 * COUNTRY is an ISO-3166 country code. On Unix, this form matches the
 * result of the setlocale(); it is also used on other machines, such as 
 * Windows, where the C library returns a different result. The string is 
 * owned by BTK+ and should not be modified or freed.
 *
 * Deprecated: 2.24: Use setlocale() directly
 **/
bchar *
btk_set_locale (void)
{
  return bdk_set_locale ();
}

/**
 * _btk_get_lc_ctype:
 *
 * Return the Unix-style locale string for the language currently in
 * effect. On Unix systems, this is the return value from
 * <literal>setlocale(LC_CTYPE, NULL)</literal>, and the user can
 * affect this through the environment variables LC_ALL, LC_CTYPE or
 * LANG (checked in that order). The locale strings typically is in
 * the form lang_COUNTRY, where lang is an ISO-639 language code, and
 * COUNTRY is an ISO-3166 country code. For instance, sv_FI for
 * Swedish as written in Finland or pt_BR for Portuguese as written in
 * Brazil.
 * 
 * On Windows, the C library doesn't use any such environment
 * variables, and setting them won't affect the behaviour of functions
 * like ctime(). The user sets the locale through the Rebunnyional Options 
 * in the Control Panel. The C library (in the setlocale() function) 
 * does not use country and language codes, but country and language 
 * names spelled out in English. 
 * However, this function does check the above environment
 * variables, and does return a Unix-style locale string based on
 * either said environment variables or the thread's current locale.
 *
 * Return value: a dynamically allocated string, free with g_free().
 */

bchar *
_btk_get_lc_ctype (void)
{
#ifdef G_OS_WIN32
  /* Somebody might try to set the locale for this process using the
   * LANG or LC_ environment variables. The Microsoft C library
   * doesn't know anything about them. You set the locale in the
   * Control Panel. Setting these env vars won't have any affect on
   * locale-dependent C library functions like ctime(). But just for
   * kicks, do obey LC_ALL, LC_CTYPE and LANG in BTK. (This also makes
   * it easier to test BTK and Bango in various default languages, you
   * don't have to clickety-click in the Control Panel, you can simply
   * start the program with LC_ALL=something on the command line.)
   */
  bchar *p;

  p = getenv ("LC_ALL");
  if (p != NULL)
    return g_strdup (p);

  p = getenv ("LC_CTYPE");
  if (p != NULL)
    return g_strdup (p);

  p = getenv ("LANG");
  if (p != NULL)
    return g_strdup (p);

  return g_win32_getlocale ();
#else
  return g_strdup (setlocale (LC_CTYPE, NULL));
#endif
}

/**
 * btk_get_default_language:
 *
 * Returns the #BangoLanguage for the default language currently in
 * effect. (Note that this can change over the life of an
 * application.)  The default language is derived from the current
 * locale. It determines, for example, whether BTK+ uses the
 * right-to-left or left-to-right text direction.
 *
 * This function is equivalent to bango_language_get_default().  See
 * that function for details.
 * 
 * Return value: the default language as a #BangoLanguage, must not be
 * freed
 **/
BangoLanguage *
btk_get_default_language (void)
{
  return bango_language_get_default ();
}

void
btk_main (void)
{
  GList *tmp_list;
  GList *functions;
  BtkInitFunction *init;
  GMainLoop *loop;

  btk_main_loop_level++;
  
  loop = g_main_loop_new (NULL, TRUE);
  main_loops = b_slist_prepend (main_loops, loop);

  tmp_list = functions = init_functions;
  init_functions = NULL;
  
  while (tmp_list)
    {
      init = tmp_list->data;
      tmp_list = tmp_list->next;
      
      (* init->function) (init->data);
      g_free (init);
    }
  g_list_free (functions);

  if (g_main_loop_is_running (main_loops->data))
    {
      BDK_THREADS_LEAVE ();
      g_main_loop_run (loop);
      BDK_THREADS_ENTER ();
      bdk_flush ();
    }

  if (quit_functions)
    {
      GList *reinvoke_list = NULL;
      BtkQuitFunction *quitf;

      while (quit_functions)
	{
	  quitf = quit_functions->data;

	  tmp_list = quit_functions;
	  quit_functions = g_list_remove_link (quit_functions, quit_functions);
	  g_list_free_1 (tmp_list);

	  if ((quitf->main_level && quitf->main_level != btk_main_loop_level) ||
	      btk_quit_invoke_function (quitf))
	    {
	      reinvoke_list = g_list_prepend (reinvoke_list, quitf);
	    }
	  else
	    {
	      btk_quit_destroy (quitf);
	    }
	}
      if (reinvoke_list)
	{
	  GList *work;
	  
	  work = g_list_last (reinvoke_list);
	  if (quit_functions)
	    quit_functions->prev = work;
	  work->next = quit_functions;
	  quit_functions = work;
	}

      bdk_flush ();
    }
    
  main_loops = b_slist_remove (main_loops, loop);

  g_main_loop_unref (loop);

  btk_main_loop_level--;

  if (btk_main_loop_level == 0)
    {
      /* Try storing all clipboard data we have */
      _btk_clipboard_store_all ();

      /* Synchronize the recent manager singleton */
      _btk_recent_manager_sync ();
    }
}

buint
btk_main_level (void)
{
  return btk_main_loop_level;
}

void
btk_main_quit (void)
{
  g_return_if_fail (main_loops != NULL);

  g_main_loop_quit (main_loops->data);
}

bboolean
btk_events_pending (void)
{
  bboolean result;
  
  BDK_THREADS_LEAVE ();  
  result = g_main_context_pending (NULL);
  BDK_THREADS_ENTER ();

  return result;
}

bboolean
btk_main_iteration (void)
{
  BDK_THREADS_LEAVE ();
  g_main_context_iteration (NULL, TRUE);
  BDK_THREADS_ENTER ();

  if (main_loops)
    return !g_main_loop_is_running (main_loops->data);
  else
    return TRUE;
}

bboolean
btk_main_iteration_do (bboolean blocking)
{
  BDK_THREADS_LEAVE ();
  g_main_context_iteration (NULL, blocking);
  BDK_THREADS_ENTER ();

  if (main_loops)
    return !g_main_loop_is_running (main_loops->data);
  else
    return TRUE;
}

/* private libbtk to libbdk interfaces
 */
bboolean bdk_pointer_grab_info_libbtk_only  (BdkDisplay *display,
					     BdkWindow **grab_window,
					     bboolean   *owner_events);
bboolean bdk_keyboard_grab_info_libbtk_only (BdkDisplay *display,
					     BdkWindow **grab_window,
					     bboolean   *owner_events);

static void
rewrite_events_translate (BdkWindow *old_window,
			  BdkWindow *new_window,
			  bdouble   *x,
			  bdouble   *y)
{
  bint old_origin_x, old_origin_y;
  bint new_origin_x, new_origin_y;

  bdk_window_get_origin	(old_window, &old_origin_x, &old_origin_y);
  bdk_window_get_origin	(new_window, &new_origin_x, &new_origin_y);

  *x += old_origin_x - new_origin_x;
  *y += old_origin_y - new_origin_y;
}

static BdkEvent *
rewrite_event_for_window (BdkEvent  *event,
			  BdkWindow *new_window)
{
  event = bdk_event_copy (event);

  switch (event->type)
    {
    case BDK_SCROLL:
      rewrite_events_translate (event->any.window,
				new_window,
				&event->scroll.x, &event->scroll.y);
      break;
    case BDK_BUTTON_PRESS:
    case BDK_2BUTTON_PRESS:
    case BDK_3BUTTON_PRESS:
    case BDK_BUTTON_RELEASE:
      rewrite_events_translate (event->any.window,
				new_window,
				&event->button.x, &event->button.y);
      break;
    case BDK_MOTION_NOTIFY:
      rewrite_events_translate (event->any.window,
				new_window,
				&event->motion.x, &event->motion.y);
      break;
    case BDK_KEY_PRESS:
    case BDK_KEY_RELEASE:
    case BDK_PROXIMITY_IN:
    case BDK_PROXIMITY_OUT:
      break;

    default:
      return event;
    }

  g_object_unref (event->any.window);
  event->any.window = g_object_ref (new_window);

  return event;
}

/* If there is a pointer or keyboard grab in effect with owner_events = TRUE,
 * then what X11 does is deliver the event normally if it was going to this
 * client, otherwise, delivers it in terms of the grab window. This function
 * rewrites events to the effect that events going to the same window group
 * are delivered normally, otherwise, the event is delivered in terms of the
 * grab window.
 */
static BdkEvent *
rewrite_event_for_grabs (BdkEvent *event)
{
  BdkWindow *grab_window;
  BtkWidget *event_widget, *grab_widget;
  bpointer grab_widget_ptr;
  bboolean owner_events;
  BdkDisplay *display;

  switch (event->type)
    {
    case BDK_SCROLL:
    case BDK_BUTTON_PRESS:
    case BDK_2BUTTON_PRESS:
    case BDK_3BUTTON_PRESS:
    case BDK_BUTTON_RELEASE:
    case BDK_MOTION_NOTIFY:
    case BDK_PROXIMITY_IN:
    case BDK_PROXIMITY_OUT:
      display = bdk_window_get_display (event->proximity.window);
      if (!bdk_pointer_grab_info_libbtk_only (display, &grab_window, &owner_events) ||
	  !owner_events)
	return NULL;
      break;

    case BDK_KEY_PRESS:
    case BDK_KEY_RELEASE:
      display = bdk_window_get_display (event->key.window);
      if (!bdk_keyboard_grab_info_libbtk_only (display, &grab_window, &owner_events) ||
	  !owner_events)
	return NULL;
      break;

    default:
      return NULL;
    }

  event_widget = btk_get_event_widget (event);
  bdk_window_get_user_data (grab_window, &grab_widget_ptr);
  grab_widget = grab_widget_ptr;

  if (grab_widget &&
      btk_main_get_window_group (grab_widget) != btk_main_get_window_group (event_widget))
    return rewrite_event_for_window (event, grab_window);
  else
    return NULL;
}

void 
btk_main_do_event (BdkEvent *event)
{
  BtkWidget *event_widget;
  BtkWidget *grab_widget;
  BtkWindowGroup *window_group;
  BdkEvent *rewritten_event = NULL;
  GList *tmp_list;

  if (event->type == BDK_SETTING)
    {
      _btk_settings_handle_event (&event->setting);
      return;
    }

  if (event->type == BDK_OWNER_CHANGE)
    {
      _btk_clipboard_handle_event (&event->owner_change);
      return;
    }

  /* Find the widget which got the event. We store the widget
   *  in the user_data field of BdkWindow's.
   *  Ignore the event if we don't have a widget for it, except
   *  for BDK_PROPERTY_NOTIFY events which are handled specialy.
   *  Though this happens rarely, bogus events can occour
   *  for e.g. destroyed BdkWindows. 
   */
  event_widget = btk_get_event_widget (event);
  if (!event_widget)
    {
      /* To handle selection INCR transactions, we select
       * PropertyNotify events on the requestor window and create
       * a corresponding (fake) BdkWindow so that events get
       * here. There won't be a widget though, so we have to handle
	   * them specially
	   */
      if (event->type == BDK_PROPERTY_NOTIFY)
	_btk_selection_incr_event (event->any.window,
				   &event->property);

      return;
    }

  /* If pointer or keyboard grabs are in effect, munge the events
   * so that each window group looks like a separate app.
   */
  rewritten_event = rewrite_event_for_grabs (event);
  if (rewritten_event)
    {
      event = rewritten_event;
      event_widget = btk_get_event_widget (event);
    }
  
  window_group = btk_main_get_window_group (event_widget);

  /* Push the event onto a stack of current events for
   * btk_current_event_get().
   */
  current_events = g_list_prepend (current_events, event);

  /* If there is a grab in effect...
   */
  if (window_group->grabs)
    {
      grab_widget = window_group->grabs->data;
      
      /* If the grab widget is an ancestor of the event widget
       *  then we send the event to the original event widget.
       *  This is the key to implementing modality.
       */
      if ((btk_widget_is_sensitive (event_widget) || event->type == BDK_SCROLL) &&
	  btk_widget_is_ancestor (event_widget, grab_widget))
	grab_widget = event_widget;
    }
  else
    {
      grab_widget = event_widget;
    }

  /* Not all events get sent to the grabbing widget.
   * The delete, destroy, expose, focus change and resize
   *  events still get sent to the event widget because
   *  1) these events have no meaning for the grabbing widget
   *  and 2) redirecting these events to the grabbing widget
   *  could cause the display to be messed up.
   * 
   * Drag events are also not redirected, since it isn't
   *  clear what the semantics of that would be.
   */
  switch (event->type)
    {
    case BDK_NOTHING:
      break;
      
    case BDK_DELETE:
      g_object_ref (event_widget);
      if ((!window_group->grabs || btk_widget_get_toplevel (window_group->grabs->data) == event_widget) &&
	  !btk_widget_event (event_widget, event))
	btk_widget_destroy (event_widget);
      g_object_unref (event_widget);
      break;
      
    case BDK_DESTROY:
      /* Unexpected BDK_DESTROY from the outside, ignore for
       * child windows, handle like a BDK_DELETE for toplevels
       */
      if (!event_widget->parent)
	{
	  g_object_ref (event_widget);
	  if (!btk_widget_event (event_widget, event) &&
	      btk_widget_get_realized (event_widget))
	    btk_widget_destroy (event_widget);
	  g_object_unref (event_widget);
	}
      break;
      
    case BDK_EXPOSE:
      if (event->any.window && btk_widget_get_double_buffered (event_widget))
	{
	  bdk_window_begin_paint_rebunnyion (event->any.window, event->expose.rebunnyion);
	  btk_widget_send_expose (event_widget, event);
	  bdk_window_end_paint (event->any.window);
	}
      else
	{
	  /* The app may paint with a previously allocated bairo_t,
	     which will draw directly to the window. We can't catch bairo
	     drap operatoins to automatically flush the window, thus we
	     need to explicitly flush any outstanding moves or double
	     buffering */
	  bdk_window_flush (event->any.window);
	  btk_widget_send_expose (event_widget, event);
	}
      break;

    case BDK_PROPERTY_NOTIFY:
    case BDK_NO_EXPOSE:
    case BDK_FOCUS_CHANGE:
    case BDK_CONFIGURE:
    case BDK_MAP:
    case BDK_UNMAP:
    case BDK_SELECTION_CLEAR:
    case BDK_SELECTION_REQUEST:
    case BDK_SELECTION_NOTIFY:
    case BDK_CLIENT_EVENT:
    case BDK_VISIBILITY_NOTIFY:
    case BDK_WINDOW_STATE:
    case BDK_GRAB_BROKEN:
    case BDK_DAMAGE:
      btk_widget_event (event_widget, event);
      break;

    case BDK_SCROLL:
    case BDK_BUTTON_PRESS:
    case BDK_2BUTTON_PRESS:
    case BDK_3BUTTON_PRESS:
      btk_propagate_event (grab_widget, event);
      break;

    case BDK_KEY_PRESS:
    case BDK_KEY_RELEASE:
      if (key_snoopers)
	{
	  if (btk_invoke_key_snoopers (grab_widget, event))
	    break;
	}
      /* Catch alt press to enable auto-mnemonics;
       * menus are handled elsewhere
       */
      if ((event->key.keyval == BDK_Alt_L || event->key.keyval == BDK_Alt_R) &&
          !BTK_IS_MENU_SHELL (grab_widget))
        {
          bboolean auto_mnemonics;

          g_object_get (btk_widget_get_settings (grab_widget),
                        "btk-auto-mnemonics", &auto_mnemonics, NULL);

          if (auto_mnemonics)
            {
              bboolean mnemonics_visible;
              BtkWidget *window;

              mnemonics_visible = (event->type == BDK_KEY_PRESS);

              window = btk_widget_get_toplevel (grab_widget);

              if (BTK_IS_WINDOW (window))
                btk_window_set_mnemonics_visible (BTK_WINDOW (window), mnemonics_visible);
            }
        }
      /* else fall through */
    case BDK_MOTION_NOTIFY:
    case BDK_BUTTON_RELEASE:
    case BDK_PROXIMITY_IN:
    case BDK_PROXIMITY_OUT:
      btk_propagate_event (grab_widget, event);
      break;
      
    case BDK_ENTER_NOTIFY:
      BTK_PRIVATE_SET_FLAG (event_widget, BTK_HAS_POINTER);
      _btk_widget_set_pointer_window (event_widget, event->any.window);
      if (btk_widget_is_sensitive (grab_widget))
	btk_widget_event (grab_widget, event);
      break;
      
    case BDK_LEAVE_NOTIFY:
      BTK_PRIVATE_UNSET_FLAG (event_widget, BTK_HAS_POINTER);
      if (btk_widget_is_sensitive (grab_widget))
	btk_widget_event (grab_widget, event);
      break;
      
    case BDK_DRAG_STATUS:
    case BDK_DROP_FINISHED:
      _btk_drag_source_handle_event (event_widget, event);
      break;
    case BDK_DRAG_ENTER:
    case BDK_DRAG_LEAVE:
    case BDK_DRAG_MOTION:
    case BDK_DROP_START:
      _btk_drag_dest_handle_event (event_widget, event);
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  if (event->type == BDK_ENTER_NOTIFY
      || event->type == BDK_LEAVE_NOTIFY
      || event->type == BDK_BUTTON_PRESS
      || event->type == BDK_2BUTTON_PRESS
      || event->type == BDK_3BUTTON_PRESS
      || event->type == BDK_KEY_PRESS
      || event->type == BDK_DRAG_ENTER
      || event->type == BDK_GRAB_BROKEN
      || event->type == BDK_MOTION_NOTIFY
      || event->type == BDK_SCROLL)
    {
      _btk_tooltip_handle_event (event);
    }
  
  tmp_list = current_events;
  current_events = g_list_remove_link (current_events, tmp_list);
  g_list_free_1 (tmp_list);

  if (rewritten_event)
    bdk_event_free (rewritten_event);
}

bboolean
btk_true (void)
{
  return TRUE;
}

bboolean
btk_false (void)
{
  return FALSE;
}

static BtkWindowGroup *
btk_main_get_window_group (BtkWidget   *widget)
{
  BtkWidget *toplevel = NULL;

  if (widget)
    toplevel = btk_widget_get_toplevel (widget);

  if (BTK_IS_WINDOW (toplevel))
    return btk_window_get_group (BTK_WINDOW (toplevel));
  else
    return btk_window_get_group (NULL);
}

typedef struct
{
  BtkWidget *old_grab_widget;
  BtkWidget *new_grab_widget;
  bboolean   was_grabbed;
  bboolean   is_grabbed;
  bboolean   from_grab;
} GrabNotifyInfo;

static void
btk_grab_notify_foreach (BtkWidget *child,
			 bpointer   data)
                        
{
  GrabNotifyInfo *info = data;
 
  bboolean was_grabbed, is_grabbed, was_shadowed, is_shadowed;

  was_grabbed = info->was_grabbed;
  is_grabbed = info->is_grabbed;

  info->was_grabbed = info->was_grabbed || (child == info->old_grab_widget);
  info->is_grabbed = info->is_grabbed || (child == info->new_grab_widget);

  was_shadowed = info->old_grab_widget && !info->was_grabbed;
  is_shadowed = info->new_grab_widget && !info->is_grabbed;

  g_object_ref (child);

  if ((was_shadowed || is_shadowed) && BTK_IS_CONTAINER (child))
    btk_container_forall (BTK_CONTAINER (child), btk_grab_notify_foreach, info);
  
  if (is_shadowed)
    {
      BTK_PRIVATE_SET_FLAG (child, BTK_SHADOWED);
      if (!was_shadowed && BTK_WIDGET_HAS_POINTER (child)
	  && btk_widget_is_sensitive (child))
	_btk_widget_synthesize_crossing (child, info->new_grab_widget,
					 BDK_CROSSING_BTK_GRAB);
    }
  else
    {
      BTK_PRIVATE_UNSET_FLAG (child, BTK_SHADOWED);
      if (was_shadowed && BTK_WIDGET_HAS_POINTER (child)
	  && btk_widget_is_sensitive (child))
	_btk_widget_synthesize_crossing (info->old_grab_widget, child,
					 info->from_grab ? BDK_CROSSING_BTK_GRAB
					 : BDK_CROSSING_BTK_UNGRAB);
    }

  if (was_shadowed != is_shadowed)
    _btk_widget_grab_notify (child, was_shadowed);
  
  g_object_unref (child);
  
  info->was_grabbed = was_grabbed;
  info->is_grabbed = is_grabbed;
}

static void
btk_grab_notify (BtkWindowGroup *group,
		 BtkWidget      *old_grab_widget,
		 BtkWidget      *new_grab_widget,
		 bboolean        from_grab)
{
  GList *toplevels;
  GrabNotifyInfo info;

  if (old_grab_widget == new_grab_widget)
    return;

  info.old_grab_widget = old_grab_widget;
  info.new_grab_widget = new_grab_widget;
  info.from_grab = from_grab;

  g_object_ref (group);

  toplevels = btk_window_list_toplevels ();
  g_list_foreach (toplevels, (GFunc)g_object_ref, NULL);
			    
  while (toplevels)
    {
      BtkWindow *toplevel = toplevels->data;
      toplevels = g_list_delete_link (toplevels, toplevels);

      info.was_grabbed = FALSE;
      info.is_grabbed = FALSE;

      if (group == btk_window_get_group (toplevel))
	btk_grab_notify_foreach (BTK_WIDGET (toplevel), &info);
      g_object_unref (toplevel);
    }

  g_object_unref (group);
}

void
btk_grab_add (BtkWidget *widget)
{
  BtkWindowGroup *group;
  BtkWidget *old_grab_widget;
  
  g_return_if_fail (widget != NULL);
  
  if (!btk_widget_has_grab (widget) && btk_widget_is_sensitive (widget))
    {
      _btk_widget_set_has_grab (widget, TRUE);
      
      group = btk_main_get_window_group (widget);

      if (group->grabs)
	old_grab_widget = (BtkWidget *)group->grabs->data;
      else
	old_grab_widget = NULL;

      g_object_ref (widget);
      group->grabs = b_slist_prepend (group->grabs, widget);

      btk_grab_notify (group, old_grab_widget, widget, TRUE);
    }
}

/**
 * btk_grab_get_current:
 *
 * Queries the current grab of the default window group.
 *
 * Return value: (transfer none): The widget which currently
 *     has the grab or %NULL if no grab is active
 */
BtkWidget*
btk_grab_get_current (void)
{
  BtkWindowGroup *group;

  group = btk_main_get_window_group (NULL);

  if (group->grabs)
    return BTK_WIDGET (group->grabs->data);
  return NULL;
}

void
btk_grab_remove (BtkWidget *widget)
{
  BtkWindowGroup *group;
  BtkWidget *new_grab_widget;
  
  g_return_if_fail (widget != NULL);
  
  if (btk_widget_has_grab (widget))
    {
      _btk_widget_set_has_grab (widget, FALSE);

      group = btk_main_get_window_group (widget);
      group->grabs = b_slist_remove (group->grabs, widget);
      
      if (group->grabs)
	new_grab_widget = (BtkWidget *)group->grabs->data;
      else
	new_grab_widget = NULL;

      btk_grab_notify (group, widget, new_grab_widget, FALSE);
      
      g_object_unref (widget);
    }
}

void
btk_init_add (BtkFunction function,
	      bpointer	  data)
{
  BtkInitFunction *init;
  
  init = g_new (BtkInitFunction, 1);
  init->function = function;
  init->data = data;
  
  init_functions = g_list_prepend (init_functions, init);
}

buint
btk_key_snooper_install (BtkKeySnoopFunc snooper,
			 bpointer	 func_data)
{
  BtkKeySnooperData *data;
  static buint snooper_id = 1;

  g_return_val_if_fail (snooper != NULL, 0);

  data = g_new (BtkKeySnooperData, 1);
  data->func = snooper;
  data->func_data = func_data;
  data->id = snooper_id++;
  key_snoopers = b_slist_prepend (key_snoopers, data);

  return data->id;
}

void
btk_key_snooper_remove (buint snooper_id)
{
  BtkKeySnooperData *data = NULL;
  GSList *slist;

  slist = key_snoopers;
  while (slist)
    {
      data = slist->data;
      if (data->id == snooper_id)
	break;

      slist = slist->next;
      data = NULL;
    }
  if (data)
    {
      key_snoopers = b_slist_remove (key_snoopers, data);
      g_free (data);
    }
}

static bint
btk_invoke_key_snoopers (BtkWidget *grab_widget,
			 BdkEvent  *event)
{
  GSList *slist;
  bint return_val = FALSE;

  slist = key_snoopers;
  while (slist && !return_val)
    {
      BtkKeySnooperData *data;

      data = slist->data;
      slist = slist->next;
      return_val = (*data->func) (grab_widget, (BdkEventKey*) event, data->func_data);
    }

  return return_val;
}

buint
btk_quit_add_full (buint		main_level,
		   BtkFunction		function,
		   BtkCallbackMarshal	marshal,
		   bpointer		data,
		   GDestroyNotify	destroy)
{
  static buint quit_id = 1;
  BtkQuitFunction *quitf;
  
  g_return_val_if_fail ((function != NULL) || (marshal != NULL), 0);

  quitf = g_slice_new (BtkQuitFunction);
  
  quitf->id = quit_id++;
  quitf->main_level = main_level;
  quitf->function = function;
  quitf->marshal = marshal;
  quitf->data = data;
  quitf->destroy = destroy;

  quit_functions = g_list_prepend (quit_functions, quitf);
  
  return quitf->id;
}

static void
btk_quit_destroy (BtkQuitFunction *quitf)
{
  if (quitf->destroy)
    quitf->destroy (quitf->data);
  g_slice_free (BtkQuitFunction, quitf);
}

static bint
btk_quit_destructor (BtkObject **object_p)
{
  if (*object_p)
    btk_object_destroy (*object_p);
  g_free (object_p);

  return FALSE;
}

void
btk_quit_add_destroy (buint              main_level,
		      BtkObject         *object)
{
  BtkObject **object_p;

  g_return_if_fail (main_level > 0);
  g_return_if_fail (BTK_IS_OBJECT (object));

  object_p = g_new (BtkObject*, 1);
  *object_p = object;
  g_signal_connect (object,
		    "destroy",
		    G_CALLBACK (btk_widget_destroyed),
		    object_p);
  btk_quit_add (main_level, (BtkFunction) btk_quit_destructor, object_p);
}

buint
btk_quit_add (buint	  main_level,
	      BtkFunction function,
	      bpointer	  data)
{
  return btk_quit_add_full (main_level, function, NULL, data, NULL);
}

void
btk_quit_remove (buint id)
{
  BtkQuitFunction *quitf;
  GList *tmp_list;
  
  tmp_list = quit_functions;
  while (tmp_list)
    {
      quitf = tmp_list->data;
      
      if (quitf->id == id)
	{
	  quit_functions = g_list_remove_link (quit_functions, tmp_list);
	  g_list_free (tmp_list);
	  btk_quit_destroy (quitf);
	  
	  return;
	}
      
      tmp_list = tmp_list->next;
    }
}

void
btk_quit_remove_by_data (bpointer data)
{
  BtkQuitFunction *quitf;
  GList *tmp_list;
  
  tmp_list = quit_functions;
  while (tmp_list)
    {
      quitf = tmp_list->data;
      
      if (quitf->data == data)
	{
	  quit_functions = g_list_remove_link (quit_functions, tmp_list);
	  g_list_free (tmp_list);
	  btk_quit_destroy (quitf);

	  return;
	}
      
      tmp_list = tmp_list->next;
    }
}

buint
btk_timeout_add_full (buint32		 interval,
		      BtkFunction	 function,
		      BtkCallbackMarshal marshal,
		      bpointer		 data,
		      GDestroyNotify	 destroy)
{
  if (marshal)
    {
      BtkClosure *closure;

      closure = g_new (BtkClosure, 1);
      closure->marshal = marshal;
      closure->data = data;
      closure->destroy = destroy;

      return g_timeout_add_full (0, interval, 
				 btk_invoke_idle_timeout,
				 closure,
				 btk_destroy_closure);
    }
  else
    return g_timeout_add_full (0, interval, function, data, destroy);
}

buint
btk_timeout_add (buint32     interval,
		 BtkFunction function,
		 bpointer    data)
{
  return g_timeout_add_full (0, interval, function, data, NULL);
}

void
btk_timeout_remove (buint tag)
{
  g_source_remove (tag);
}

buint
btk_idle_add_full (bint			priority,
		   BtkFunction		function,
		   BtkCallbackMarshal	marshal,
		   bpointer		data,
		   GDestroyNotify	destroy)
{
  if (marshal)
    {
      BtkClosure *closure;

      closure = g_new (BtkClosure, 1);
      closure->marshal = marshal;
      closure->data = data;
      closure->destroy = destroy;

      return g_idle_add_full (priority,
			      btk_invoke_idle_timeout,
			      closure,
			      btk_destroy_closure);
    }
  else
    return g_idle_add_full (priority, function, data, destroy);
}

buint
btk_idle_add (BtkFunction function,
	      bpointer	  data)
{
  return g_idle_add_full (G_PRIORITY_DEFAULT_IDLE, function, data, NULL);
}

buint	    
btk_idle_add_priority (bint        priority,
		       BtkFunction function,
		       bpointer	   data)
{
  return g_idle_add_full (priority, function, data, NULL);
}

void
btk_idle_remove (buint tag)
{
  g_source_remove (tag);
}

void
btk_idle_remove_by_data (bpointer data)
{
  if (!g_idle_remove_by_data (data))
    g_warning ("btk_idle_remove_by_data(%p): no such idle", data);
}

buint
btk_input_add_full (bint		source,
		    BdkInputCondition	condition,
		    BdkInputFunction	function,
		    BtkCallbackMarshal	marshal,
		    bpointer		data,
		    GDestroyNotify	destroy)
{
  if (marshal)
    {
      BtkClosure *closure;

      closure = g_new (BtkClosure, 1);
      closure->marshal = marshal;
      closure->data = data;
      closure->destroy = destroy;

      return bdk_input_add_full (source,
				 condition,
				 (BdkInputFunction) btk_invoke_input,
				 closure,
				 (GDestroyNotify) btk_destroy_closure);
    }
  else
    return bdk_input_add_full (source, condition, function, data, destroy);
}

void
btk_input_remove (buint tag)
{
  g_source_remove (tag);
}

static void
btk_destroy_closure (bpointer data)
{
  BtkClosure *closure = data;

  if (closure->destroy)
    (closure->destroy) (closure->data);
  g_free (closure);
}

static bboolean
btk_invoke_idle_timeout (bpointer data)
{
  BtkClosure *closure = data;

  BtkArg args[1];
  bint ret_val = FALSE;
  args[0].name = NULL;
  args[0].type = B_TYPE_BOOLEAN;
  args[0].d.pointer_data = &ret_val;
  closure->marshal (NULL, closure->data,  0, args);
  return ret_val;
}

static void
btk_invoke_input (bpointer	    data,
		  bint		    source,
		  BdkInputCondition condition)
{
  BtkClosure *closure = data;

  BtkArg args[3];
  args[0].type = B_TYPE_INT;
  args[0].name = NULL;
  BTK_VALUE_INT (args[0]) = source;
  args[1].type = BDK_TYPE_INPUT_CONDITION;
  args[1].name = NULL;
  BTK_VALUE_FLAGS (args[1]) = condition;
  args[2].type = B_TYPE_NONE;
  args[2].name = NULL;

  closure->marshal (NULL, closure->data, 2, args);
}

/**
 * btk_get_current_event:
 * 
 * Obtains a copy of the event currently being processed by BTK+.  For
 * example, if you get a "clicked" signal from #BtkButton, the current
 * event will be the #BdkEventButton that triggered the "clicked"
 * signal. The returned event must be freed with bdk_event_free().
 * If there is no current event, the function returns %NULL.
 * 
 * Return value: (transfer full): a copy of the current event, or %NULL if no
 *     current event.
 **/
BdkEvent*
btk_get_current_event (void)
{
  if (current_events)
    return bdk_event_copy (current_events->data);
  else
    return NULL;
}

/**
 * btk_get_current_event_time:
 * 
 * If there is a current event and it has a timestamp, return that
 * timestamp, otherwise return %BDK_CURRENT_TIME.
 * 
 * Return value: the timestamp from the current event, or %BDK_CURRENT_TIME.
 **/
buint32
btk_get_current_event_time (void)
{
  if (current_events)
    return bdk_event_get_time (current_events->data);
  else
    return BDK_CURRENT_TIME;
}

/**
 * btk_get_current_event_state:
 * @state: (out): a location to store the state of the current event
 * 
 * If there is a current event and it has a state field, place
 * that state field in @state and return %TRUE, otherwise return
 * %FALSE.
 * 
 * Return value: %TRUE if there was a current event and it had a state field
 **/
bboolean
btk_get_current_event_state (BdkModifierType *state)
{
  g_return_val_if_fail (state != NULL, FALSE);
  
  if (current_events)
    return bdk_event_get_state (current_events->data, state);
  else
    {
      *state = 0;
      return FALSE;
    }
}

/**
 * btk_get_event_widget:
 * @event: a #BdkEvent
 *
 * If @event is %NULL or the event was not associated with any widget,
 * returns %NULL, otherwise returns the widget that received the event
 * originally.
 * 
 * Return value: (transfer none): the widget that originally
 *     received @event, or %NULL
 **/
BtkWidget*
btk_get_event_widget (BdkEvent *event)
{
  BtkWidget *widget;
  bpointer widget_ptr;

  widget = NULL;
  if (event && event->any.window && 
      (event->type == BDK_DESTROY || !BDK_WINDOW_DESTROYED (event->any.window)))
    {
      bdk_window_get_user_data (event->any.window, &widget_ptr);
      widget = widget_ptr;
    }
  
  return widget;
}

static bint
btk_quit_invoke_function (BtkQuitFunction *quitf)
{
  if (!quitf->marshal)
    return quitf->function (quitf->data);
  else
    {
      BtkArg args[1];
      bint ret_val = FALSE;

      args[0].name = NULL;
      args[0].type = B_TYPE_BOOLEAN;
      args[0].d.pointer_data = &ret_val;
      ((BtkCallbackMarshal) quitf->marshal) (NULL,
					     quitf->data,
					     0, args);
      return ret_val;
    }
}

/**
 * btk_propagate_event:
 * @widget: a #BtkWidget
 * @event: an event
 *
 * Sends an event to a widget, propagating the event to parent widgets
 * if the event remains unhandled. Events received by BTK+ from BDK
 * normally begin in btk_main_do_event(). Depending on the type of
 * event, existence of modal dialogs, grabs, etc., the event may be
 * propagated; if so, this function is used. btk_propagate_event()
 * calls btk_widget_event() on each widget it decides to send the
 * event to.  So btk_widget_event() is the lowest-level function; it
 * simply emits the "event" and possibly an event-specific signal on a
 * widget.  btk_propagate_event() is a bit higher-level, and
 * btk_main_do_event() is the highest level.
 *
 * All that said, you most likely don't want to use any of these
 * functions; synthesizing events is rarely needed. Consider asking on
 * the mailing list for better ways to achieve your goals. For
 * example, use bdk_window_invalidate_rect() or
 * btk_widget_queue_draw() instead of making up expose events.
 * 
 **/
void
btk_propagate_event (BtkWidget *widget,
		     BdkEvent  *event)
{
  bint handled_event;
  
  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (event != NULL);
  
  handled_event = FALSE;

  g_object_ref (widget);
      
  if ((event->type == BDK_KEY_PRESS) ||
      (event->type == BDK_KEY_RELEASE))
    {
      /* Only send key events within Window widgets to the Window
       *  The Window widget will in turn pass the
       *  key event on to the currently focused widget
       *  for that window.
       */
      BtkWidget *window;

      window = btk_widget_get_toplevel (widget);
      if (BTK_IS_WINDOW (window))
	{
	  /* If there is a grab within the window, give the grab widget
	   * a first crack at the key event
	   */
	  if (widget != window && btk_widget_has_grab (widget))
	    handled_event = btk_widget_event (widget, event);
	  
	  if (!handled_event)
	    {
	      window = btk_widget_get_toplevel (widget);
	      if (BTK_IS_WINDOW (window))
		{
		  if (btk_widget_is_sensitive (window))
		    btk_widget_event (window, event);
		}
	    }
		  
	  handled_event = TRUE; /* don't send to widget */
	}
    }
  
  /* Other events get propagated up the widget tree
   *  so that parents can see the button and motion
   *  events of the children.
   */
  if (!handled_event)
    {
      while (TRUE)
	{
	  BtkWidget *tmp;

	  /* Scroll events are special cased here because it
	   * feels wrong when scrolling a BtkViewport, say,
	   * to have children of the viewport eat the scroll
	   * event
	   */
	  if (!btk_widget_is_sensitive (widget))
	    handled_event = event->type != BDK_SCROLL;
	  else
	    handled_event = btk_widget_event (widget, event);
	      
	  tmp = widget->parent;
	  g_object_unref (widget);

	  widget = tmp;
	  
	  if (!handled_event && widget)
	    g_object_ref (widget);
	  else
	    break;
	}
    }
  else
    g_object_unref (widget);
}

#if 0
static void
btk_error (bchar *str)
{
  btk_print (str);
}

static void
btk_warning (bchar *str)
{
  btk_print (str);
}

static void
btk_message (bchar *str)
{
  btk_print (str);
}

static void
btk_print (bchar *str)
{
  static BtkWidget *window = NULL;
  static BtkWidget *text;
  static int level = 0;
  BtkWidget *box1;
  BtkWidget *box2;
  BtkWidget *table;
  BtkWidget *hscrollbar;
  BtkWidget *vscrollbar;
  BtkWidget *separator;
  BtkWidget *button;
  
  if (level > 0)
    {
      fputs (str, stdout);
      fflush (stdout);
      return;
    }
  
  if (!window)
    {
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      
      btk_signal_connect (BTK_OBJECT (window), "destroy",
			  G_CALLBACK (btk_widget_destroyed),
			  &window);
      
      btk_window_set_title (BTK_WINDOW (window), "Messages");
      
      box1 = btk_vbox_new (FALSE, 0);
      btk_container_add (BTK_CONTAINER (window), box1);
      btk_widget_show (box1);
      
      
      box2 = btk_vbox_new (FALSE, 10);
      btk_container_set_border_width (BTK_CONTAINER (box2), 10);
      btk_box_pack_start (BTK_BOX (box1), box2, TRUE, TRUE, 0);
      btk_widget_show (box2);
      
      
      table = btk_table_new (2, 2, FALSE);
      btk_table_set_row_spacing (BTK_TABLE (table), 0, 2);
      btk_table_set_col_spacing (BTK_TABLE (table), 0, 2);
      btk_box_pack_start (BTK_BOX (box2), table, TRUE, TRUE, 0);
      btk_widget_show (table);
      
      text = btk_text_new (NULL, NULL);
      btk_text_set_editable (BTK_TEXT (text), FALSE);
      btk_table_attach_defaults (BTK_TABLE (table), text, 0, 1, 0, 1);
      btk_widget_show (text);
      btk_widget_realize (text);
      
      hscrollbar = btk_hscrollbar_new (BTK_TEXT (text)->hadj);
      btk_table_attach (BTK_TABLE (table), hscrollbar, 0, 1, 1, 2,
			BTK_EXPAND | BTK_FILL, BTK_FILL, 0, 0);
      btk_widget_show (hscrollbar);
      
      vscrollbar = btk_vscrollbar_new (BTK_TEXT (text)->vadj);
      btk_table_attach (BTK_TABLE (table), vscrollbar, 1, 2, 0, 1,
			BTK_FILL, BTK_EXPAND | BTK_FILL, 0, 0);
      btk_widget_show (vscrollbar);
      
      separator = btk_hseparator_new ();
      btk_box_pack_start (BTK_BOX (box1), separator, FALSE, TRUE, 0);
      btk_widget_show (separator);
      
      
      box2 = btk_vbox_new (FALSE, 10);
      btk_container_set_border_width (BTK_CONTAINER (box2), 10);
      btk_box_pack_start (BTK_BOX (box1), box2, FALSE, TRUE, 0);
      btk_widget_show (box2);
      
      
      button = btk_button_new_with_label ("close");
      btk_signal_connect_object (BTK_OBJECT (button), "clicked",
				 G_CALLBACK (btk_widget_hide),
				 BTK_OBJECT (window));
      btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);
      btk_widget_set_can_default (button, TRUE);
      btk_widget_grab_default (button);
      btk_widget_show (button);
    }
  
  level += 1;
  btk_text_insert (BTK_TEXT (text), NULL, NULL, NULL, str, -1);
  level -= 1;
  
  if (!btk_widget_get_visible (window))
    btk_widget_show (window);
}
#endif

bboolean
_btk_boolean_handled_accumulator (GSignalInvocationHint *ihint,
				  BValue                *return_accu,
				  const BValue          *handler_return,
				  bpointer               dummy)
{
  bboolean continue_emission;
  bboolean signal_handled;
  
  signal_handled = b_value_get_boolean (handler_return);
  b_value_set_boolean (return_accu, signal_handled);
  continue_emission = !signal_handled;
  
  return continue_emission;
}

bboolean
_btk_button_event_triggers_context_menu (BdkEventButton *event)
{
  if (event->type == BDK_BUTTON_PRESS)
    {
      if (event->button == 3 &&
          ! (event->state & (BDK_BUTTON1_MASK | BDK_BUTTON2_MASK)))
        return TRUE;

#ifdef BDK_WINDOWING_QUARTZ
      if (event->button == 1 &&
          ! (event->state & (BDK_BUTTON2_MASK | BDK_BUTTON3_MASK)) &&
          (event->state & BDK_CONTROL_MASK))
        return TRUE;
#endif
    }

  return FALSE;
}

bboolean
_btk_translate_keyboard_accel_state (BdkKeymap       *keymap,
                                     buint            hardware_keycode,
                                     BdkModifierType  state,
                                     BdkModifierType  accel_mask,
                                     bint             group,
                                     buint           *keyval,
                                     bint            *effective_group,
                                     bint            *level,
                                     BdkModifierType *consumed_modifiers)
{
  bboolean group_mask_disabled = FALSE;
  bboolean retval;

  /* if the group-toggling modifier is part of the accel mod mask, and
   * it is active, disable it for matching
   */
  if (accel_mask & state & BTK_TOGGLE_GROUP_MOD_MASK)
    {
      state &= ~BTK_TOGGLE_GROUP_MOD_MASK;
      group = 0;
      group_mask_disabled = TRUE;
    }

  retval = bdk_keymap_translate_keyboard_state (keymap,
                                                hardware_keycode, state, group,
                                                keyval,
                                                effective_group, level,
                                                consumed_modifiers);

  /* add back the group mask, we want to match against the modifier,
   * but not against the keyval from its group
   */
  if (group_mask_disabled)
    {
      if (effective_group)
        *effective_group = 1;

      if (consumed_modifiers)
        *consumed_modifiers &= ~BTK_TOGGLE_GROUP_MOD_MASK;
    }

  return retval;
}

#define __BTK_MAIN_C__
#include "btkaliasdef.c"
