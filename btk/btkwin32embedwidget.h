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
 * Modified by the BTK+ Team and others 1997-2006.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/.
 */

#ifndef __BTK_WIN32_EMBED_WIDGET_H__
#define __BTK_WIN32_EMBED_WIDGET_H__


#include <btk/btkwindow.h>
#include "win32/bdkwin32.h"


B_BEGIN_DECLS

#define BTK_TYPE_WIN32_EMBED_WIDGET            (btk_win32_embed_widget_get_type ())
#define BTK_WIN32_EMBED_WIDGET(obj)            (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_WIN32_EMBED_WIDGET, BtkWin32EmbedWidget))
#define BTK_WIN32_EMBED_WIDGET_CLASS(klass)    (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_WIN32_EMBED_WIDGET, BtkWin32EmbedWidgetClass))
#define BTK_IS_WIN32_EMBED_WIDGET(obj)         (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_WIN32_EMBED_WIDGET))
#define BTK_IS_WIN32_EMBED_WIDGET_CLASS(klass) (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_WIN32_EMBED_WIDGET))
#define BTK_WIN32_EMBED_WIDGET_GET_CLASS(obj)  (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_WIN32_EMBED_WIDGET, BtkWin32EmbedWidgetClass))


typedef struct _BtkWin32EmbedWidget        BtkWin32EmbedWidget;
typedef struct _BtkWin32EmbedWidgetClass   BtkWin32EmbedWidgetClass;


struct _BtkWin32EmbedWidget
{
  BtkWindow window;

  BdkWindow *parent_window;
  bpointer old_window_procedure;
};

struct _BtkWin32EmbedWidgetClass
{
  BtkWindowClass parent_class;

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};


GType      btk_win32_embed_widget_get_type (void) B_GNUC_CONST;
BtkWidget* _btk_win32_embed_widget_new              (BdkNativeWindow  parent_id);
BOOL       _btk_win32_embed_widget_dialog_procedure (BtkWin32EmbedWidget *embed_widget,
						     HWND wnd, UINT message, WPARAM wparam, LPARAM lparam);


B_END_DECLS

#endif /* __BTK_WIN32_EMBED_WIDGET_H__ */
