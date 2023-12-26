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

#ifndef __BTK_MOUNT_OPERATION_PRIVATE_H__
#define __BTK_MOUNT_OPERATION_PRIVATE_H__

#include <bunnyio/bunnyio.h>
#include <bdk/bdk.h>
#include <bdk-pixbuf/bdk-pixbuf.h>

struct _BtkMountOperationLookupContext;
typedef struct _BtkMountOperationLookupContext BtkMountOperationLookupContext;

BtkMountOperationLookupContext *_btk_mount_operation_lookup_context_get  (BdkDisplay *display);

bboolean _btk_mount_operation_lookup_info         (BtkMountOperationLookupContext *context,
                                                   GPid                            pid,
                                                   bint                            size_pixels,
                                                   bchar                         **out_name,
                                                   bchar                         **out_command_line,
                                                   BdkPixbuf                     **out_pixbuf);

void     _btk_mount_operation_lookup_context_free (BtkMountOperationLookupContext *context);

/* throw G_IO_ERROR_FAILED_HANDLED if a helper already reported the error to the user */
bboolean _btk_mount_operation_kill_process (GPid      pid,
                                            GError  **error);

#endif /* __BTK_MOUNT_OPERATION_PRIVATE_H__ */
