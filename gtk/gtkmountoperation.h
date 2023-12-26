/* BTK - The GIMP Toolkit
 * Copyright (C) Christian Kellner <gicmo@bunny.org>
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

#ifndef __BTK_MOUNT_OPERATION_H__
#define __BTK_MOUNT_OPERATION_H__

#if !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

G_BEGIN_DECLS

#define BTK_TYPE_MOUNT_OPERATION         (btk_mount_operation_get_type ())
#define BTK_MOUNT_OPERATION(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BTK_TYPE_MOUNT_OPERATION, BtkMountOperation))
#define BTK_MOUNT_OPERATION_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), BTK_TYPE_MOUNT_OPERATION, BtkMountOperationClass))
#define BTK_IS_MOUNT_OPERATION(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BTK_TYPE_MOUNT_OPERATION))
#define BTK_IS_MOUNT_OPERATION_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BTK_TYPE_MOUNT_OPERATION))
#define BTK_MOUNT_OPERATION_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BTK_TYPE_MOUNT_OPERATION, BtkMountOperationClass))

typedef struct _BtkMountOperation         BtkMountOperation;
typedef struct _BtkMountOperationClass    BtkMountOperationClass;
typedef struct _BtkMountOperationPrivate  BtkMountOperationPrivate;

/**
 * BtkMountOperation:
 *
 * This should not be accessed directly. Use the accessor functions below.
 */
struct _BtkMountOperation
{
  GMountOperation parent_instance;

  BtkMountOperationPrivate *priv;
};

struct _BtkMountOperationClass
{
  GMountOperationClass parent_class;

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};


GType            btk_mount_operation_get_type   (void);
GMountOperation *btk_mount_operation_new        (BtkWindow         *parent);
gboolean         btk_mount_operation_is_showing (BtkMountOperation *op);
void             btk_mount_operation_set_parent (BtkMountOperation *op,
                                                 BtkWindow         *parent);
BtkWindow *      btk_mount_operation_get_parent (BtkMountOperation *op);
void             btk_mount_operation_set_screen (BtkMountOperation *op,
                                                 BdkScreen         *screen);
BdkScreen       *btk_mount_operation_get_screen (BtkMountOperation *op);

G_END_DECLS

#endif /* __BTK_MOUNT_OPERATION_H__ */
