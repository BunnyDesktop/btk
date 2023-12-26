/*
 * Copyright (C) 2003 Sun Microsystems Inc.
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
 *
 * Authors: Mark McLoughlin <mark@skynet.ie>
 */

#include "config.h"
#include <string.h>
#include <stdlib.h>

#include "bdkspawn.h"

#include <bunnylib.h>
#include <bdk/bdk.h>
#include "bdkalias.h"
  
typedef struct {
  char *display;
  GSpawnChildSetupFunc child_setup;
  bpointer user_data;
} UserChildSetup;

/*
 * Set the DISPLAY variable, and then call the user-specified child setup
 * function.  This is required so that applications can use bdk_spawn_* and 
 * call putenv() in their child_setup functions.
 */
static void
set_environment (bpointer user_data)
{
  UserChildSetup *setup = user_data;
  
  g_setenv ("DISPLAY", setup->display, TRUE);
  
  if (setup->child_setup)
    setup->child_setup (setup->user_data);
}

/**
 * bdk_spawn_on_screen:
 * @screen: a #BdkScreen
 * @working_directory: child's current working directory, or %NULL to 
 *   inherit parent's
 * @argv: child's argument vector
 * @envp: child's environment, or %NULL to inherit parent's
 * @flags: flags from #GSpawnFlags
 * @child_setup: function to run in the child just before exec()
 * @user_data: user data for @child_setup
 * @child_pid: return location for child process ID, or %NULL
 * @error: return location for error
 *
 * Like g_spawn_async(), except the child process is spawned in such
 * an environment that on calling bdk_display_open() it would be
 * returned a #BdkDisplay with @screen as the default screen.
 *
 * This is useful for applications which wish to launch an application
 * on a specific screen.
 *
 * Return value: %TRUE on success, %FALSE if error is set
 *
 * Since: 2.4
 *
 * Deprecated: 2.24: This function is being removed in 3.0. Use
 *     either g_spawn_sync(), g_spawn_async(), or #BdkAppLaunchContext instead.
 **/
bboolean
bdk_spawn_on_screen (BdkScreen             *screen,
		     const bchar           *working_directory,
		     bchar                **argv,
		     bchar                **envp,
		     GSpawnFlags            flags,
		     GSpawnChildSetupFunc   child_setup,
		     bpointer               user_data,
		     bint                  *child_pid,
		     GError               **error)
{
  UserChildSetup setup_data;

  g_return_val_if_fail (BDK_IS_SCREEN (screen), FALSE);

  setup_data.display = bdk_screen_make_display_name (screen);
  setup_data.child_setup = child_setup;
  setup_data.user_data = user_data;

  return g_spawn_async (working_directory,
			  argv,
			  envp,
			  flags,
			  set_environment,
			  &setup_data,
			  child_pid,
			  error);
}

/**
 * bdk_spawn_on_screen_with_pipes:
 * @screen: a #BdkScreen
 * @working_directory: child's current working directory, or %NULL to 
 *   inherit parent's
 * @argv: child's argument vector
 * @envp: child's environment, or %NULL to inherit parent's
 * @flags: flags from #GSpawnFlags
 * @child_setup: function to run in the child just before exec()
 * @user_data: user data for @child_setup
 * @child_pid: return location for child process ID, or %NULL
 * @standard_input: return location for file descriptor to write to 
 *   child's stdin, or %NULL
 * @standard_output: return location for file descriptor to read child's 
 *   stdout, or %NULL
 * @standard_error: return location for file descriptor to read child's 
 *   stderr, or %NULL
 * @error: return location for error
 *
 * Like g_spawn_async_with_pipes(), except the child process is
 * spawned in such an environment that on calling bdk_display_open()
 * it would be returned a #BdkDisplay with @screen as the default
 * screen.
 *
 * This is useful for applications which wish to launch an application
 * on a specific screen.
 *
 * Return value: %TRUE on success, %FALSE if an error was set
 *
 * Since: 2.4
 *
 * Deprecated: 2.24: This function is being removed in 3.0. Use
 *     either g_spawn_async_with_pipes() or #BdkAppLaunchContext instead.
 **/
bboolean
bdk_spawn_on_screen_with_pipes (BdkScreen            *screen,
				const bchar          *working_directory,
				bchar               **argv,
				bchar               **envp,
				GSpawnFlags           flags,
				GSpawnChildSetupFunc  child_setup,
				bpointer              user_data,
				bint                 *child_pid,
				bint                 *standard_input,
				bint                 *standard_output,
				bint                 *standard_error,
				GError              **error)
{
  UserChildSetup setup_data;

  g_return_val_if_fail (BDK_IS_SCREEN (screen), FALSE);

  setup_data.display = bdk_screen_make_display_name (screen);
  setup_data.child_setup = child_setup;
  setup_data.user_data = user_data;

  return g_spawn_async_with_pipes (working_directory,
				     argv,
				     envp,
				     flags,
				     set_environment,
				     &setup_data,
				     child_pid,
				     standard_input,
				     standard_output,
				     standard_error,
				     error);

}

/**
 * bdk_spawn_command_line_on_screen:
 * @screen: a #BdkScreen
 * @command_line: a command line
 * @error: return location for errors
 *
 * Like g_spawn_command_line_async(), except the child process is
 * spawned in such an environment that on calling bdk_display_open()
 * it would be returned a #BdkDisplay with @screen as the default
 * screen.
 *
 * This is useful for applications which wish to launch an application
 * on a specific screen.
 *
 * Return value: %TRUE on success, %FALSE if error is set.
 *
 * Since: 2.4
 *
 * Deprecated: 2.24: This function is being removed in 3.0. Use
 *     either g_spawn_command_line_sync(), g_spawn_command_line_async() or 
 *     #BdkAppLaunchContext instead.
 **/
bboolean
bdk_spawn_command_line_on_screen (BdkScreen    *screen,
				  const bchar  *command_line,
				  GError      **error)
{
  bchar    **argv = NULL;
  bboolean   retval;

  g_return_val_if_fail (command_line != NULL, FALSE);

  if (!g_shell_parse_argv (command_line,
			   NULL, &argv,
			   error))
    return FALSE;

  retval = bdk_spawn_on_screen (screen,
				NULL, argv, NULL,
				G_SPAWN_SEARCH_PATH,
				NULL, NULL, NULL,
				error);
  g_strfreev (argv);

  return retval;
}

#define __BDK_SPAWN_X11_C__
#include "bdkaliasdef.c"
