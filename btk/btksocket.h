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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
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

#ifndef __BTK_SOCKET_H__
#define __BTK_SOCKET_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkcontainer.h>

B_BEGIN_DECLS

#define BTK_TYPE_SOCKET            (btk_socket_get_type ())
#define BTK_SOCKET(obj)            (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_SOCKET, BtkSocket))
#define BTK_SOCKET_CLASS(klass)    (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_SOCKET, BtkSocketClass))
#define BTK_IS_SOCKET(obj)         (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_SOCKET))
#define BTK_IS_SOCKET_CLASS(klass) (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_SOCKET))
#define BTK_SOCKET_GET_CLASS(obj)  (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_SOCKET, BtkSocketClass))


typedef struct _BtkSocket        BtkSocket;
typedef struct _BtkSocketClass   BtkSocketClass;

struct _BtkSocket
{
  BtkContainer container;

  guint16 GSEAL (request_width);
  guint16 GSEAL (request_height);
  guint16 GSEAL (current_width);
  guint16 GSEAL (current_height);

  BdkWindow *GSEAL (plug_window);
  BtkWidget *GSEAL (plug_widget);

  gshort GSEAL (xembed_version); /* -1 == not xembed */
  guint GSEAL (same_app) : 1;
  guint GSEAL (focus_in) : 1;
  guint GSEAL (have_size) : 1;
  guint GSEAL (need_map) : 1;
  guint GSEAL (is_mapped) : 1;
  guint GSEAL (active) : 1;

  BtkAccelGroup *GSEAL (accel_group);
  BtkWidget *GSEAL (toplevel);
};

struct _BtkSocketClass
{
  BtkContainerClass parent_class;

  void     (*plug_added)   (BtkSocket *socket_);
  gboolean (*plug_removed) (BtkSocket *socket_);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};


GType          btk_socket_get_type (void) B_GNUC_CONST;
BtkWidget*     btk_socket_new      (void);

void            btk_socket_add_id (BtkSocket       *socket_,
				   BdkNativeWindow  window_id);
BdkNativeWindow btk_socket_get_id (BtkSocket       *socket_);
BdkWindow*      btk_socket_get_plug_window (BtkSocket       *socket_);

#ifndef BTK_DISABLE_DEPRECATED
void           btk_socket_steal    (BtkSocket      *socket_,
				    BdkNativeWindow wid);
#endif /* BTK_DISABLE_DEPRECATED */

B_END_DECLS

#endif /* __BTK_SOCKET_H__ */
