/* bdkmain-quartz.c
 *
 * Copyright (C) 2005 Imendio AB
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
#include <dlfcn.h>

#include "bdk.h"
#include <ApplicationServices/ApplicationServices.h>

const GOptionEntry _bdk_windowing_args[] = {
  { NULL }
};

void
_bdk_windowing_init (void)
{
  ProcessSerialNumber psn = { 0, kCurrentProcess };
  void (*_btk_quartz_framework_init_ptr) (void);

  /* Make the current process a foreground application, i.e. an app
   * with a user interface, in case we're not running from a .app bundle
   */
  TransformProcessType (&psn, kProcessTransformToForegroundApplication);

  /* Initialize BTK+ framework if there is one. */
  _btk_quartz_framework_init_ptr = dlsym (RTLD_DEFAULT, "_btk_quartz_framework_init");
  if (_btk_quartz_framework_init_ptr)
    _btk_quartz_framework_init_ptr ();
}

void
bdk_error_trap_push (void)
{
}

gint
bdk_error_trap_pop (void)
{
  return 0;
}

gchar *
bdk_get_display (void)
{
  return g_strdup (bdk_display_get_name (bdk_display_get_default ()));
}

void
bdk_notify_startup_complete (void)
{
  /* FIXME: Implement? */
}

void
bdk_notify_startup_complete_with_id (const gchar* startup_id)
{
  /* FIXME: Implement? */
}

void          
bdk_window_set_startup_id (BdkWindow   *window,
			   const gchar *startup_id)
{
  /* FIXME: Implement? */
}

void
_bdk_windowing_display_set_sm_client_id (BdkDisplay  *display,
					 const gchar *sm_client_id)
{
}

void
bdk_set_use_xshm (gboolean use_xshm)
{
  /* Always on, since we're always on the local machine */
}

gboolean
bdk_get_use_xshm (void)
{
  return TRUE;
}

