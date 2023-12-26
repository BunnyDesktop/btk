/* btktrayicon.h
 * Copyright (C) 2002 Anders Carlsson <andersca@gnu.org>
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

#ifndef __BTK_TRAY_ICON_H__
#define __BTK_TRAY_ICON_H__

#include "btkplug.h"

B_BEGIN_DECLS

#define BTK_TYPE_TRAY_ICON		(btk_tray_icon_get_type ())
#define BTK_TRAY_ICON(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_TRAY_ICON, BtkTrayIcon))
#define BTK_TRAY_ICON_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_TRAY_ICON, BtkTrayIconClass))
#define BTK_IS_TRAY_ICON(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_TRAY_ICON))
#define BTK_IS_TRAY_ICON_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_TRAY_ICON))
#define BTK_TRAY_ICON_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_TRAY_ICON, BtkTrayIconClass))
	
typedef struct _BtkTrayIcon	   BtkTrayIcon;
typedef struct _BtkTrayIconPrivate BtkTrayIconPrivate;
typedef struct _BtkTrayIconClass   BtkTrayIconClass;

struct _BtkTrayIcon
{
  BtkPlug parent_instance;

  BtkTrayIconPrivate *priv;
};

struct _BtkTrayIconClass
{
  BtkPlugClass parent_class;

  void (*__btk_reserved1);
  void (*__btk_reserved2);
  void (*__btk_reserved3);
  void (*__btk_reserved4);
  void (*__btk_reserved5);
  void (*__btk_reserved6);
};

GType          btk_tray_icon_get_type         (void) B_GNUC_CONST;

BtkTrayIcon   *_btk_tray_icon_new_for_screen  (BdkScreen   *screen,
					       const gchar *name);

BtkTrayIcon   *_btk_tray_icon_new             (const gchar *name);

guint          _btk_tray_icon_send_message    (BtkTrayIcon *icon,
					       gint         timeout,
					       const gchar *message,
					       gint         len);
void           _btk_tray_icon_cancel_message  (BtkTrayIcon *icon,
					       guint        id);

BtkOrientation _btk_tray_icon_get_orientation (BtkTrayIcon *icon);
					    
B_END_DECLS

#endif /* __BTK_TRAY_ICON_H__ */
