/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* BTK - The GIMP Toolkit
 * Copyright (C) David Zeuthen <davidz@redhat.com>
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

/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/.
 */

#include "config.h"

#include <bunnyio/bunnyio.h>
#include "btkintl.h"

#include "btkmountoperationprivate.h"

BtkMountOperationLookupContext *
_btk_mount_operation_lookup_context_get (BdkDisplay *display)
{
  return NULL;
}

void
_btk_mount_operation_lookup_context_free (BtkMountOperationLookupContext *context)
{
}

bboolean
_btk_mount_operation_lookup_info (BtkMountOperationLookupContext *context,
                                  GPid                            pid,
                                  bint                            size_pixels,
                                  bchar                         **out_name,
                                  bchar                         **out_command_line,
                                  BdkPixbuf                     **out_pixbuf)
{
  return FALSE;
}

bboolean
_btk_mount_operation_kill_process (GPid      pid,
                                   GError  **error)
{
  g_set_error (error,
               G_IO_ERROR,
               G_IO_ERROR_NOT_SUPPORTED,
               _("Cannot kill process with PID %d. Operation is not implemented."),
               pid);
  return FALSE;
}

