/*
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
 *
 * Authors: Cody Russell <crussell@canonical.com>
 *          Alexander Larsson <alexl@redhat.com>
 */

#ifndef __BTK_OFFSCREEN_WINDOW_H__
#define __BTK_OFFSCREEN_WINDOW_H__

#if !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkwindow.h>

G_BEGIN_DECLS

#define BTK_TYPE_OFFSCREEN_WINDOW         (btk_offscreen_window_get_type ())
#define BTK_OFFSCREEN_WINDOW(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BTK_TYPE_OFFSCREEN_WINDOW, BtkOffscreenWindow))
#define BTK_OFFSCREEN_WINDOW_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), BTK_TYPE_OFFSCREEN_WINDOW, BtkOffscreenWindowClass))
#define BTK_IS_OFFSCREEN_WINDOW(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BTK_TYPE_OFFSCREEN_WINDOW))
#define BTK_IS_OFFSCREEN_WINDOW_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BTK_TYPE_OFFSCREEN_WINDOW))
#define BTK_OFFSCREEN_WINDOW_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BTK_TYPE_OFFSCREEN_WINDOW, BtkOffscreenWindowClass))

typedef struct _BtkOffscreenWindow      BtkOffscreenWindow;
typedef struct _BtkOffscreenWindowClass BtkOffscreenWindowClass;

struct _BtkOffscreenWindow
{
  BtkWindow parent_object;
};

struct _BtkOffscreenWindowClass
{
  BtkWindowClass parent_class;
};

GType      btk_offscreen_window_get_type   (void) G_GNUC_CONST;

BtkWidget *btk_offscreen_window_new        (void);
BdkPixmap *btk_offscreen_window_get_pixmap (BtkOffscreenWindow *offscreen);
BdkPixbuf *btk_offscreen_window_get_pixbuf (BtkOffscreenWindow *offscreen);

G_END_DECLS

#endif /* __BTK_OFFSCREEN_WINDOW_H__ */
