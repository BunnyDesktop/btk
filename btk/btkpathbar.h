/* btkpathbar.h
 * Copyright (C) 2004  Red Hat, Inc.,  Jonathan Blandford <jrb@bunny.org>
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
 */

#ifndef __BTK_PATH_BAR_H__
#define __BTK_PATH_BAR_H__

#include "btkcontainer.h"
#include "btkfilesystem.h"

G_BEGIN_DECLS

typedef struct _BtkPathBar      BtkPathBar;
typedef struct _BtkPathBarClass BtkPathBarClass;


#define BTK_TYPE_PATH_BAR                 (btk_path_bar_get_type ())
#define BTK_PATH_BAR(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_PATH_BAR, BtkPathBar))
#define BTK_PATH_BAR_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_PATH_BAR, BtkPathBarClass))
#define BTK_IS_PATH_BAR(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_PATH_BAR))
#define BTK_IS_PATH_BAR_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_PATH_BAR))
#define BTK_PATH_BAR_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_PATH_BAR, BtkPathBarClass))

struct _BtkPathBar
{
  BtkContainer parent;

  BtkFileSystem *file_system;
  GFile *root_file;
  GFile *home_file;
  GFile *desktop_file;

  GCancellable *get_info_cancellable;

  BdkPixbuf *root_icon;
  BdkPixbuf *home_icon;
  BdkPixbuf *desktop_icon;

  BdkWindow *event_window;

  GList *button_list;
  GList *first_scrolled_button;
  GList *fake_root;
  BtkWidget *up_slider_button;
  BtkWidget *down_slider_button;
  guint settings_signal_id;
  gint icon_size;
  gint16 slider_width;
  gint16 spacing;
  gint16 button_offset;
  guint timer;
  guint slider_visible : 1;
  guint need_timer     : 1;
  guint ignore_click   : 1;
  guint scrolling_up   : 1;
  guint scrolling_down : 1;
};

struct _BtkPathBarClass
{
  BtkContainerClass parent_class;

  void (* path_clicked) (BtkPathBar  *path_bar,
			 GFile       *file,
			 GFile       *child_file,
			 gboolean     child_is_hidden);
};

GType    btk_path_bar_get_type (void) G_GNUC_CONST;
void     _btk_path_bar_set_file_system (BtkPathBar         *path_bar,
					BtkFileSystem      *file_system);
gboolean _btk_path_bar_set_file        (BtkPathBar         *path_bar,
					GFile              *file,
					gboolean            keep_trail,
					GError            **error);
void     _btk_path_bar_up              (BtkPathBar *path_bar);
void     _btk_path_bar_down            (BtkPathBar *path_bar);

G_END_DECLS

#endif /* __BTK_PATH_BAR_H__ */
