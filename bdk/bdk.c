/* BDK - The GIMP Drawing Kit
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

#include <string.h>
#include <stdlib.h>

#include "bdk.h"
#include "bdkinternals.h"
#include "bdkintl.h"

#ifndef HAVE_XCONVERTCASE
#include "bdkkeysyms.h"
#endif
#include "bdkalias.h"

typedef struct _BdkPredicate  BdkPredicate;

struct _BdkPredicate
{
  BdkEventFunc func;
  gpointer data;
};

typedef struct _BdkThreadsDispatch BdkThreadsDispatch;

struct _BdkThreadsDispatch
{
  GSourceFunc func;
  gpointer data;
  GDestroyNotify destroy;
};


/* Private variable declarations
 */
static int bdk_initialized = 0;			    /* 1 if the library is initialized,
						     * 0 otherwise.
						     */

static gchar  *bdk_progclass = NULL;

#ifdef G_ENABLE_DEBUG
static const GDebugKey bdk_debug_keys[] = {
  {"events",	    BDK_DEBUG_EVENTS},
  {"misc",	    BDK_DEBUG_MISC},
  {"dnd",	    BDK_DEBUG_DND},
  {"xim",	    BDK_DEBUG_XIM},
  {"nograbs",       BDK_DEBUG_NOGRABS},
  {"colormap",	    BDK_DEBUG_COLORMAP},
  {"bdkrgb",	    BDK_DEBUG_BDKRGB},
  {"gc",	    BDK_DEBUG_GC},
  {"pixmap",	    BDK_DEBUG_PIXMAP},
  {"image",	    BDK_DEBUG_IMAGE},
  {"input",	    BDK_DEBUG_INPUT},
  {"cursor",	    BDK_DEBUG_CURSOR},
  {"multihead",	    BDK_DEBUG_MULTIHEAD},
  {"xinerama",	    BDK_DEBUG_XINERAMA},
  {"draw",	    BDK_DEBUG_DRAW},
  {"eventloop",	    BDK_DEBUG_EVENTLOOP}
};

static const int bdk_ndebug_keys = G_N_ELEMENTS (bdk_debug_keys);

#endif /* G_ENABLE_DEBUG */

#ifdef G_ENABLE_DEBUG
static gboolean
bdk_arg_debug_cb (const char *key, const char *value, gpointer user_data, GError **error)
{
  guint debug_value = g_parse_debug_string (value,
					    (GDebugKey *) bdk_debug_keys,
					    bdk_ndebug_keys);

  if (debug_value == 0 && value != NULL && strcmp (value, "") != 0)
    {
      g_set_error (error, 
		   G_OPTION_ERROR, G_OPTION_ERROR_FAILED,
		   _("Error parsing option --bdk-debug"));
      return FALSE;
    }

  _bdk_debug_flags |= debug_value;

  return TRUE;
}

static gboolean
bdk_arg_no_debug_cb (const char *key, const char *value, gpointer user_data, GError **error)
{
  guint debug_value = g_parse_debug_string (value,
					    (GDebugKey *) bdk_debug_keys,
					    bdk_ndebug_keys);

  if (debug_value == 0 && value != NULL && strcmp (value, "") != 0)
    {
      g_set_error (error, 
		   G_OPTION_ERROR, G_OPTION_ERROR_FAILED,
		   _("Error parsing option --bdk-no-debug"));
      return FALSE;
    }

  _bdk_debug_flags &= ~debug_value;

  return TRUE;
}
#endif /* G_ENABLE_DEBUG */

static gboolean
bdk_arg_class_cb (const char *key, const char *value, gpointer user_data, GError **error)
{
  bdk_set_program_class (value);

  return TRUE;
}

static gboolean
bdk_arg_name_cb (const char *key, const char *value, gpointer user_data, GError **error)
{
  g_set_prgname (value);

  return TRUE;
}

static const GOptionEntry bdk_args[] = {
  { "class",        0, 0,                     G_OPTION_ARG_CALLBACK, bdk_arg_class_cb,
    /* Description of --class=CLASS in --help output */        N_("Program class as used by the window manager"),
    /* Placeholder in --class=CLASS in --help output */        N_("CLASS") },
  { "name",         0, 0,                     G_OPTION_ARG_CALLBACK, bdk_arg_name_cb,
    /* Description of --name=NAME in --help output */          N_("Program name as used by the window manager"),
    /* Placeholder in --name=NAME in --help output */          N_("NAME") },
  { "display",      0, G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_STRING,   &_bdk_display_name,
    /* Description of --display=DISPLAY in --help output */    N_("X display to use"),
    /* Placeholder in --display=DISPLAY in --help output */    N_("DISPLAY") },
  { "screen",       0, 0, G_OPTION_ARG_INT,      &_bdk_screen_number,
    /* Description of --screen=SCREEN in --help output */      N_("X screen to use"),
    /* Placeholder in --screen=SCREEN in --help output */      N_("SCREEN") },
#ifdef G_ENABLE_DEBUG
  { "bdk-debug",    0, 0, G_OPTION_ARG_CALLBACK, bdk_arg_debug_cb,  
    /* Description of --bdk-debug=FLAGS in --help output */    N_("BDK debugging flags to set"),
    /* Placeholder in --bdk-debug=FLAGS in --help output */    N_("FLAGS") },
  { "bdk-no-debug", 0, 0, G_OPTION_ARG_CALLBACK, bdk_arg_no_debug_cb, 
    /* Description of --bdk-no-debug=FLAGS in --help output */ N_("BDK debugging flags to unset"),
    /* Placeholder in --bdk-no-debug=FLAGS in --help output */ N_("FLAGS") },
#endif 
  { NULL }
};

/**
 * bdk_add_option_entries_libbtk_only:
 * @group: An option group.
 * 
 * Appends bdk option entries to the passed in option group. This is
 * not public API and must not be used by applications.
 **/
void
bdk_add_option_entries_libbtk_only (GOptionGroup *group)
{
  g_option_group_add_entries (group, bdk_args);
  g_option_group_add_entries (group, _bdk_windowing_args);
}

void
bdk_pre_parse_libbtk_only (void)
{
  bdk_initialized = TRUE;

  /* We set the fallback program class here, rather than lazily in
   * bdk_get_program_class, since we don't want -name to override it.
   */
  bdk_progclass = g_strdup (g_get_prgname ());
  if (bdk_progclass && bdk_progclass[0])
    bdk_progclass[0] = g_ascii_toupper (bdk_progclass[0]);
  
#ifdef G_ENABLE_DEBUG
  {
    gchar *debug_string = getenv("BDK_DEBUG");
    if (debug_string != NULL)
      _bdk_debug_flags = g_parse_debug_string (debug_string,
					      (GDebugKey *) bdk_debug_keys,
					      bdk_ndebug_keys);
  }
#endif	/* G_ENABLE_DEBUG */

  if (getenv ("BDK_NATIVE_WINDOWS"))
    {
      _bdk_native_windows = TRUE;
      /* Ensure that this is not propagated
	 to spawned applications */
      g_unsetenv ("BDK_NATIVE_WINDOWS");
    }

  g_type_init ();

  /* Do any setup particular to the windowing system
   */
  _bdk_windowing_init ();  
}

  
/**
 * bdk_parse_args:
 * @argc: the number of command line arguments.
 * @argv: (inout) (array length=argc): the array of command line arguments.
 * 
 * Parse command line arguments, and store for future
 * use by calls to bdk_display_open().
 *
 * Any arguments used by BDK are removed from the array and @argc and @argv are
 * updated accordingly.
 *
 * You shouldn't call this function explicitely if you are using
 * btk_init(), btk_init_check(), bdk_init(), or bdk_init_check().
 *
 * Since: 2.2
 **/
void
bdk_parse_args (int    *argc,
		char ***argv)
{
  GOptionContext *option_context;
  GOptionGroup *option_group;
  GError *error = NULL;

  if (bdk_initialized)
    return;

  bdk_pre_parse_libbtk_only ();
  
  option_context = g_option_context_new (NULL);
  g_option_context_set_ignore_unknown_options (option_context, TRUE);
  g_option_context_set_help_enabled (option_context, FALSE);
  option_group = g_option_group_new (NULL, NULL, NULL, NULL, NULL);
  g_option_context_set_main_group (option_context, option_group);
  
  g_option_group_add_entries (option_group, bdk_args);
  g_option_group_add_entries (option_group, _bdk_windowing_args);

  if (!g_option_context_parse (option_context, argc, argv, &error))
    {
      g_warning ("%s", error->message);
      g_error_free (error);
    }
  g_option_context_free (option_context);

  if (_bdk_debug_flags && BDK_DEBUG_BDKRGB)
    bdk_rgb_set_verbose (TRUE);

  BDK_NOTE (MISC, g_message ("progname: \"%s\"", g_get_prgname ()));
}

/** 
 * bdk_get_display_arg_name:
 *
 * Gets the display name specified in the command line arguments passed
 * to bdk_init() or bdk_parse_args(), if any.
 *
 * Returns: the display name, if specified explicitely, otherwise %NULL
 *   this string is owned by BTK+ and must not be modified or freed.
 *
 * Since: 2.2
 */
const gchar *
bdk_get_display_arg_name (void)
{
  if (!_bdk_display_arg_name)
    {
      if (_bdk_screen_number >= 0)
	_bdk_display_arg_name = _bdk_windowing_substitute_screen_number (_bdk_display_name, _bdk_screen_number);
      else
	_bdk_display_arg_name = g_strdup (_bdk_display_name);
   }

   return _bdk_display_arg_name;
}

/**
 * bdk_display_open_default_libbtk_only:
 * 
 * Opens the default display specified by command line arguments or
 * environment variables, sets it as the default display, and returns
 * it.  bdk_parse_args must have been called first. If the default
 * display has previously been set, simply returns that. An internal
 * function that should not be used by applications.
 * 
 * Return value: the default display, if it could be opened,
 *   otherwise %NULL.
 **/
BdkDisplay *
bdk_display_open_default_libbtk_only (void)
{
  BdkDisplay *display;

  g_return_val_if_fail (bdk_initialized, NULL);
  
  display = bdk_display_get_default ();
  if (display)
    return display;

  display = bdk_display_open (bdk_get_display_arg_name ());

  if (!display && _bdk_screen_number >= 0)
    {
      g_free (_bdk_display_arg_name);
      _bdk_display_arg_name = g_strdup (_bdk_display_name);
      
      display = bdk_display_open (_bdk_display_name);
    }
  
  if (display)
    bdk_display_manager_set_default_display (bdk_display_manager_get (),
					     display);
  
  return display;
}

/**
 * bdk_init_check:
 * @argc: (inout):
 * @argv: (array length=argc) (inout):
 *
 *   Initialize the library for use.
 *
 * Arguments:
 *   "argc" is the number of arguments.
 *   "argv" is an array of strings.
 *
 * Results:
 *   "argc" and "argv" are modified to reflect any arguments
 *   which were not handled. (Such arguments should either
 *   be handled by the application or dismissed). If initialization
 *   fails, returns FALSE, otherwise TRUE.
 *
 * Side effects:
 *   The library is initialized.
 *
 *--------------------------------------------------------------
 */
gboolean
bdk_init_check (int    *argc,
		char ***argv)
{
  bdk_parse_args (argc, argv);

  return bdk_display_open_default_libbtk_only () != NULL;
}


/**
 * bdk_init:
 * @argc: (inout):
 * @argv: (array length=argc) (inout):
 */
void
bdk_init (int *argc, char ***argv)
{
  if (!bdk_init_check (argc, argv))
    {
      const char *display_name = bdk_get_display_arg_name ();
      g_warning ("cannot open display: %s", display_name ? display_name : "");
      exit(1);
    }
}

/*
 *--------------------------------------------------------------
 * bdk_exit
 *
 *   Restores the library to an un-itialized state and exits
 *   the program using the "exit" system call.
 *
 * Arguments:
 *   "errorcode" is the error value to pass to "exit".
 *
 * Results:
 *   Allocated structures are freed and the program exits
 *   cleanly.
 *
 * Side effects:
 *
 *--------------------------------------------------------------
 */

void
bdk_exit (gint errorcode)
{
  exit (errorcode);
}

void
bdk_threads_enter (void)
{
  BDK_THREADS_ENTER ();
}

void
bdk_threads_leave (void)
{
  BDK_THREADS_LEAVE ();
}

static void
bdk_threads_impl_lock (void)
{
  if (bdk_threads_mutex)
    g_mutex_lock (bdk_threads_mutex);
}

static void
bdk_threads_impl_unlock (void)
{
  if (bdk_threads_mutex)
    {
      /* we need a trylock() here because trying to unlock a mutex
       * that hasn't been locked yet is:
       *
       *  a) not portable
       *  b) fail on GLib ≥ 2.41
       *
       * trylock() will either succeed because nothing is holding the
       * BDK mutex, and will be unlocked right afterwards; or it's
       * going to fail because the mutex is locked already, in which
       * case we unlock it as expected.
       *
       * this is needed in the case somebody called bdk_threads_init()
       * without calling bdk_threads_enter() before calling btk_main().
       * in theory, we could just say that this is undefined behaviour,
       * but our documentation has always been *less* than explicit as
       * to what the behaviour should actually be.
       *
       * see bug: https://bugzilla.bunny.org/show_bug.cgi?id=735428
       */
      g_mutex_trylock (bdk_threads_mutex);
      g_mutex_unlock (bdk_threads_mutex);
    }
}

/**
 * bdk_threads_init:
 * 
 * Initializes BDK so that it can be used from multiple threads
 * in conjunction with bdk_threads_enter() and bdk_threads_leave().
 * g_thread_init() must be called previous to this function.
 *
 * This call must be made before any use of the main loop from
 * BTK+; to be safe, call it before btk_init().
 **/
void
bdk_threads_init (void)
{
  if (!g_thread_supported ())
    g_error ("g_thread_init() must be called before bdk_threads_init()");

  bdk_threads_mutex = g_mutex_new ();
  if (!bdk_threads_lock)
    bdk_threads_lock = bdk_threads_impl_lock;
  if (!bdk_threads_unlock)
    bdk_threads_unlock = bdk_threads_impl_unlock;
}

/**
 * bdk_threads_set_lock_functions:
 * @enter_fn:   function called to guard BDK
 * @leave_fn: function called to release the guard
 *
 * Allows the application to replace the standard method that
 * BDK uses to protect its data structures. Normally, BDK
 * creates a single #GMutex that is locked by bdk_threads_enter(),
 * and released by bdk_threads_leave(); using this function an
 * application provides, instead, a function @enter_fn that is
 * called by bdk_threads_enter() and a function @leave_fn that is
 * called by bdk_threads_leave().
 *
 * The functions must provide at least same locking functionality
 * as the default implementation, but can also do extra application
 * specific processing.
 *
 * As an example, consider an application that has its own recursive
 * lock that when held, holds the BTK+ lock as well. When BTK+ unlocks
 * the BTK+ lock when entering a recursive main loop, the application
 * must temporarily release its lock as well.
 *
 * Most threaded BTK+ apps won't need to use this method.
 *
 * This method must be called before bdk_threads_init(), and cannot
 * be called multiple times.
 *
 * Since: 2.4
 **/
void
bdk_threads_set_lock_functions (GCallback enter_fn,
				GCallback leave_fn)
{
  g_return_if_fail (bdk_threads_lock == NULL &&
		    bdk_threads_unlock == NULL);

  bdk_threads_lock = enter_fn;
  bdk_threads_unlock = leave_fn;
}

static gboolean
bdk_threads_dispatch (gpointer data)
{
  BdkThreadsDispatch *dispatch = data;
  gboolean ret = FALSE;

  BDK_THREADS_ENTER ();

  if (!g_source_is_destroyed (g_main_current_source ()))
    ret = dispatch->func (dispatch->data);

  BDK_THREADS_LEAVE ();

  return ret;
}

static void
bdk_threads_dispatch_free (gpointer data)
{
  BdkThreadsDispatch *dispatch = data;

  if (dispatch->destroy && dispatch->data)
    dispatch->destroy (dispatch->data);

  g_slice_free (BdkThreadsDispatch, data);
}


/**
 * bdk_threads_add_idle_full:
 * @priority: the priority of the idle source. Typically this will be in the
 *            range btweeen #G_PRIORITY_DEFAULT_IDLE and #G_PRIORITY_HIGH_IDLE
 * @function: function to call
 * @data:     data to pass to @function
 * @notify: (allow-none):   function to call when the idle is removed, or %NULL
 *
 * Adds a function to be called whenever there are no higher priority
 * events pending.  If the function returns %FALSE it is automatically
 * removed from the list of event sources and will not be called again.
 *
 * This variant of g_idle_add_full() calls @function with the BDK lock
 * held. It can be thought of a MT-safe version for BTK+ widgets for the 
 * following use case, where you have to worry about idle_callback()
 * running in thread A and accessing @self after it has been finalized
 * in thread B:
 *
 * |[
 * static gboolean
 * idle_callback (gpointer data)
 * {
 *    /&ast; bdk_threads_enter(); would be needed for g_idle_add() &ast;/
 *
 *    SomeWidget *self = data;
 *    /&ast; do stuff with self &ast;/
 *
 *    self->idle_id = 0;
 *
 *    /&ast; bdk_threads_leave(); would be needed for g_idle_add() &ast;/
 *    return FALSE;
 * }
 *
 * static void
 * some_widget_do_stuff_later (SomeWidget *self)
 * {
 *    self->idle_id = bdk_threads_add_idle (idle_callback, self)
 *    /&ast; using g_idle_add() here would require thread protection in the callback &ast;/
 * }
 *
 * static void
 * some_widget_finalize (GObject *object)
 * {
 *    SomeWidget *self = SOME_WIDGET (object);
 *    if (self->idle_id)
 *      g_source_remove (self->idle_id);
 *    G_OBJECT_CLASS (parent_class)->finalize (object);
 * }
 * ]|
 *
 * Return value: the ID (greater than 0) of the event source.
 *
 * Since: 2.12
 */
guint
bdk_threads_add_idle_full (gint           priority,
		           GSourceFunc    function,
		           gpointer       data,
		           GDestroyNotify notify)
{
  BdkThreadsDispatch *dispatch;

  g_return_val_if_fail (function != NULL, 0);

  dispatch = g_slice_new (BdkThreadsDispatch);
  dispatch->func = function;
  dispatch->data = data;
  dispatch->destroy = notify;

  return g_idle_add_full (priority,
                          bdk_threads_dispatch,
                          dispatch,
                          bdk_threads_dispatch_free);
}

/**
 * bdk_threads_add_idle:
 * @function: function to call
 * @data:     data to pass to @function
 *
 * A wrapper for the common usage of bdk_threads_add_idle_full() 
 * assigning the default priority, #G_PRIORITY_DEFAULT_IDLE.
 *
 * See bdk_threads_add_idle_full().
 *
 * Return value: the ID (greater than 0) of the event source.
 * 
 * Since: 2.12
 */
guint
bdk_threads_add_idle (GSourceFunc    function,
		      gpointer       data)
{
  return bdk_threads_add_idle_full (G_PRIORITY_DEFAULT_IDLE,
                                    function, data, NULL);
}


/**
 * bdk_threads_add_timeout_full:
 * @priority: the priority of the timeout source. Typically this will be in the
 *            range between #G_PRIORITY_DEFAULT_IDLE and #G_PRIORITY_HIGH_IDLE.
 * @interval: the time between calls to the function, in milliseconds
 *             (1/1000ths of a second)
 * @function: function to call
 * @data:     data to pass to @function
 * @notify: (allow-none):   function to call when the timeout is removed, or %NULL
 *
 * Sets a function to be called at regular intervals holding the BDK lock,
 * with the given priority.  The function is called repeatedly until it 
 * returns %FALSE, at which point the timeout is automatically destroyed 
 * and the function will not be called again.  The @notify function is
 * called when the timeout is destroyed.  The first call to the
 * function will be at the end of the first @interval.
 *
 * Note that timeout functions may be delayed, due to the processing of other
 * event sources. Thus they should not be relied on for precise timing.
 * After each call to the timeout function, the time of the next
 * timeout is recalculated based on the current time and the given interval
 * (it does not try to 'catch up' time lost in delays).
 *
 * This variant of g_timeout_add_full() can be thought of a MT-safe version 
 * for BTK+ widgets for the following use case:
 *
 * |[
 * static gboolean timeout_callback (gpointer data)
 * {
 *    SomeWidget *self = data;
 *    
 *    /&ast; do stuff with self &ast;/
 *    
 *    self->timeout_id = 0;
 *    
 *    return FALSE;
 * }
 *  
 * static void some_widget_do_stuff_later (SomeWidget *self)
 * {
 *    self->timeout_id = g_timeout_add (timeout_callback, self)
 * }
 *  
 * static void some_widget_finalize (GObject *object)
 * {
 *    SomeWidget *self = SOME_WIDGET (object);
 *    
 *    if (self->timeout_id)
 *      g_source_remove (self->timeout_id);
 *    
 *    G_OBJECT_CLASS (parent_class)->finalize (object);
 * }
 * ]|
 *
 * Return value: the ID (greater than 0) of the event source.
 * 
 * Since: 2.12
 */
guint
bdk_threads_add_timeout_full (gint           priority,
                              guint          interval,
                              GSourceFunc    function,
                              gpointer       data,
                              GDestroyNotify notify)
{
  BdkThreadsDispatch *dispatch;

  g_return_val_if_fail (function != NULL, 0);

  dispatch = g_slice_new (BdkThreadsDispatch);
  dispatch->func = function;
  dispatch->data = data;
  dispatch->destroy = notify;

  return g_timeout_add_full (priority, 
                             interval,
                             bdk_threads_dispatch, 
                             dispatch, 
                             bdk_threads_dispatch_free);
}

/**
 * bdk_threads_add_timeout:
 * @interval: the time between calls to the function, in milliseconds
 *             (1/1000ths of a second)
 * @function: function to call
 * @data:     data to pass to @function
 *
 * A wrapper for the common usage of bdk_threads_add_timeout_full() 
 * assigning the default priority, #G_PRIORITY_DEFAULT.
 *
 * See bdk_threads_add_timeout_full().
 * 
 * Return value: the ID (greater than 0) of the event source.
 *
 * Since: 2.12
 */
guint
bdk_threads_add_timeout (guint       interval,
                         GSourceFunc function,
                         gpointer    data)
{
  return bdk_threads_add_timeout_full (G_PRIORITY_DEFAULT,
                                       interval, function, data, NULL);
}


/**
 * bdk_threads_add_timeout_seconds_full:
 * @priority: the priority of the timeout source. Typically this will be in the
 *            range between #G_PRIORITY_DEFAULT_IDLE and #G_PRIORITY_HIGH_IDLE.
 * @interval: the time between calls to the function, in seconds
 * @function: function to call
 * @data:     data to pass to @function
 * @notify: (allow-none):   function to call when the timeout is removed, or %NULL
 *
 * A variant of bdk_threads_add_timout_full() with second-granularity.
 * See g_timeout_add_seconds_full() for a discussion of why it is
 * a good idea to use this function if you don't need finer granularity.
 *
 *  Return value: the ID (greater than 0) of the event source.
 * 
 * Since: 2.14
 */
guint
bdk_threads_add_timeout_seconds_full (gint           priority,
                                      guint          interval,
                                      GSourceFunc    function,
                                      gpointer       data,
                                      GDestroyNotify notify)
{
  BdkThreadsDispatch *dispatch;

  g_return_val_if_fail (function != NULL, 0);

  dispatch = g_slice_new (BdkThreadsDispatch);
  dispatch->func = function;
  dispatch->data = data;
  dispatch->destroy = notify;

  return g_timeout_add_seconds_full (priority, 
                                     interval,
                                     bdk_threads_dispatch, 
                                     dispatch, 
                                     bdk_threads_dispatch_free);
}

/**
 * bdk_threads_add_timeout_seconds:
 * @interval: the time between calls to the function, in seconds
 * @function: function to call
 * @data:     data to pass to @function
 *
 * A wrapper for the common usage of bdk_threads_add_timeout_seconds_full() 
 * assigning the default priority, #G_PRIORITY_DEFAULT.
 *
 * For details, see bdk_threads_add_timeout_full().
 * 
 * Return value: the ID (greater than 0) of the event source.
 *
 * Since: 2.14
 */
guint
bdk_threads_add_timeout_seconds (guint       interval,
                                 GSourceFunc function,
                                 gpointer    data)
{
  return bdk_threads_add_timeout_seconds_full (G_PRIORITY_DEFAULT,
                                               interval, function, data, NULL);
}


const char *
bdk_get_program_class (void)
{
  return bdk_progclass;
}

void
bdk_set_program_class (const char *program_class)
{
  g_free (bdk_progclass);

  bdk_progclass = g_strdup (program_class);
}

#define __BDK_C__
#include "bdkaliasdef.c"
