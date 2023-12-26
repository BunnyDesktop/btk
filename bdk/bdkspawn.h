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

#ifndef __BDK_SPAWN_H__
#define __BDK_SPAWN_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BDK_H_INSIDE__) && !defined (BDK_COMPILATION)
#error "Only <bdk/bdk.h> can be included directly."
#endif

#include <bdk/bdkscreen.h>

B_BEGIN_DECLS

#ifndef BDK_DISABLE_DEPRECATED
bboolean bdk_spawn_on_screen              (BdkScreen             *screen,
					   const bchar           *working_directory,
					   bchar                **argv,
					   bchar                **envp,
					   GSpawnFlags            flags,
					   GSpawnChildSetupFunc   child_setup,
					   bpointer               user_data,
					   bint                  *child_pid,
					   GError               **error);

bboolean bdk_spawn_on_screen_with_pipes   (BdkScreen             *screen,
					   const bchar           *working_directory,
					   bchar                **argv,
					   bchar                **envp,
					   GSpawnFlags            flags,
					   GSpawnChildSetupFunc   child_setup,
					   bpointer               user_data,
					   bint                  *child_pid,
					   bint                  *standard_input,
					   bint                  *standard_output,
					   bint                  *standard_error,
					   GError               **error);

bboolean bdk_spawn_command_line_on_screen (BdkScreen             *screen,
					   const bchar           *command_line,
					   GError               **error);
#endif

B_END_DECLS

#endif /* __BDK_SPAWN_H__ */
