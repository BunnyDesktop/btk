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

#ifndef __BTK_PLUG_H__
#define __BTK_PLUG_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btksocket.h>
#include <btk/btkwindow.h>


G_BEGIN_DECLS

#define BTK_TYPE_PLUG            (btk_plug_get_type ())
#define BTK_PLUG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_PLUG, BtkPlug))
#define BTK_PLUG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_PLUG, BtkPlugClass))
#define BTK_IS_PLUG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_PLUG))
#define BTK_IS_PLUG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_PLUG))
#define BTK_PLUG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_PLUG, BtkPlugClass))


typedef struct _BtkPlug        BtkPlug;
typedef struct _BtkPlugClass   BtkPlugClass;


struct _BtkPlug
{
  BtkWindow window;

  BdkWindow *GSEAL (socket_window);
  BtkWidget *GSEAL (modality_window);
  BtkWindowGroup *GSEAL (modality_group);
  GHashTable *GSEAL (grabbed_keys);

  guint GSEAL (same_app) : 1;
};

struct _BtkPlugClass
{
  BtkWindowClass parent_class;

  void (*embedded) (BtkPlug *plug);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};


GType      btk_plug_get_type  (void) G_GNUC_CONST;

#ifndef BDK_MULTIHEAD_SAFE
void       btk_plug_construct (BtkPlug         *plug,
			       BdkNativeWindow  socket_id);
BtkWidget* btk_plug_new       (BdkNativeWindow  socket_id);
#endif

void       btk_plug_construct_for_display (BtkPlug         *plug,
					   BdkDisplay      *display,
					   BdkNativeWindow  socket_id);
BtkWidget* btk_plug_new_for_display       (BdkDisplay      *display,
					   BdkNativeWindow  socket_id);

BdkNativeWindow btk_plug_get_id (BtkPlug         *plug);

gboolean  btk_plug_get_embedded (BtkPlug         *plug);

BdkWindow *btk_plug_get_socket_window (BtkPlug   *plug);

void _btk_plug_add_to_socket      (BtkPlug   *plug,
				   BtkSocket *socket_);
void _btk_plug_remove_from_socket (BtkPlug   *plug,
				   BtkSocket *socket_);

G_END_DECLS

#endif /* __BTK_PLUG_H__ */
