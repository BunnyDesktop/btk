/* btkstatusicon.h:
 *
 * Copyright (C) 2003 Sun Microsystems, Inc.
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
 *
 * Authors:
 *      Mark McLoughlin <mark@skynet.ie>
 */

#ifndef __BTK_STATUS_ICON_H__
#define __BTK_STATUS_ICON_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkimage.h>
#include <btk/btkmenu.h>

B_BEGIN_DECLS

#define BTK_TYPE_STATUS_ICON         (btk_status_icon_get_type ())
#define BTK_STATUS_ICON(o)           (B_TYPE_CHECK_INSTANCE_CAST ((o), BTK_TYPE_STATUS_ICON, BtkStatusIcon))
#define BTK_STATUS_ICON_CLASS(k)     (B_TYPE_CHECK_CLASS_CAST ((k), BTK_TYPE_STATUS_ICON, BtkStatusIconClass))
#define BTK_IS_STATUS_ICON(o)        (B_TYPE_CHECK_INSTANCE_TYPE ((o), BTK_TYPE_STATUS_ICON))
#define BTK_IS_STATUS_ICON_CLASS(k)  (B_TYPE_CHECK_CLASS_TYPE ((k), BTK_TYPE_STATUS_ICON))
#define BTK_STATUS_ICON_GET_CLASS(o) (B_TYPE_INSTANCE_GET_CLASS ((o), BTK_TYPE_STATUS_ICON, BtkStatusIconClass))

typedef struct _BtkStatusIcon	     BtkStatusIcon;
typedef struct _BtkStatusIconClass   BtkStatusIconClass;
typedef struct _BtkStatusIconPrivate BtkStatusIconPrivate;

struct _BtkStatusIcon
{
  BObject               parent_instance;

  BtkStatusIconPrivate *GSEAL (priv);
};

struct _BtkStatusIconClass
{
  BObjectClass parent_class;

  void     (* activate)             (BtkStatusIcon  *status_icon);
  void     (* popup_menu)           (BtkStatusIcon  *status_icon,
                                     buint           button,
                                     buint32         activate_time);
  bboolean (* size_changed)         (BtkStatusIcon  *status_icon,
                                     bint            size);
  bboolean (* button_press_event)   (BtkStatusIcon  *status_icon,
                                     BdkEventButton *event);
  bboolean (* button_release_event) (BtkStatusIcon  *status_icon,
                                     BdkEventButton *event);
  bboolean (* scroll_event)         (BtkStatusIcon  *status_icon,
                                     BdkEventScroll *event);
  bboolean (* query_tooltip)        (BtkStatusIcon  *status_icon,
                                     bint            x,
                                     bint            y,
                                     bboolean        keyboard_mode,
                                     BtkTooltip     *tooltip);

  void *__btk_reserved1;
  void *__btk_reserved2;
};

GType                 btk_status_icon_get_type           (void) B_GNUC_CONST;

BtkStatusIcon        *btk_status_icon_new                (void);
BtkStatusIcon        *btk_status_icon_new_from_pixbuf    (BdkPixbuf          *pixbuf);
BtkStatusIcon        *btk_status_icon_new_from_file      (const bchar        *filename);
BtkStatusIcon        *btk_status_icon_new_from_stock     (const bchar        *stock_id);
BtkStatusIcon        *btk_status_icon_new_from_icon_name (const bchar        *icon_name);
BtkStatusIcon        *btk_status_icon_new_from_gicon     (GIcon              *icon);

void                  btk_status_icon_set_from_pixbuf    (BtkStatusIcon      *status_icon,
							  BdkPixbuf          *pixbuf);
void                  btk_status_icon_set_from_file      (BtkStatusIcon      *status_icon,
							  const bchar        *filename);
void                  btk_status_icon_set_from_stock     (BtkStatusIcon      *status_icon,
							  const bchar        *stock_id);
void                  btk_status_icon_set_from_icon_name (BtkStatusIcon      *status_icon,
							  const bchar        *icon_name);
void                  btk_status_icon_set_from_gicon     (BtkStatusIcon      *status_icon,
                                                          GIcon              *icon);

BtkImageType          btk_status_icon_get_storage_type   (BtkStatusIcon      *status_icon);

BdkPixbuf            *btk_status_icon_get_pixbuf         (BtkStatusIcon      *status_icon);
const bchar *         btk_status_icon_get_stock          (BtkStatusIcon      *status_icon);
const bchar *         btk_status_icon_get_icon_name      (BtkStatusIcon      *status_icon);
GIcon                *btk_status_icon_get_gicon          (BtkStatusIcon      *status_icon);

bint                  btk_status_icon_get_size           (BtkStatusIcon      *status_icon);

void                  btk_status_icon_set_screen         (BtkStatusIcon      *status_icon,
                                                          BdkScreen          *screen);
BdkScreen            *btk_status_icon_get_screen         (BtkStatusIcon      *status_icon);

#ifndef BTK_DISABLE_DEPRECATED
void                  btk_status_icon_set_tooltip        (BtkStatusIcon      *status_icon,
                                                          const bchar        *tooltip_text);
#endif
void                  btk_status_icon_set_has_tooltip    (BtkStatusIcon      *status_icon,
                                                          bboolean            has_tooltip);
void                  btk_status_icon_set_tooltip_text   (BtkStatusIcon      *status_icon,
                                                          const bchar        *text);
void                  btk_status_icon_set_tooltip_markup (BtkStatusIcon      *status_icon,
                                                          const bchar        *markup);
void                  btk_status_icon_set_title          (BtkStatusIcon      *status_icon,
                                                          const bchar        *title);
const bchar *         btk_status_icon_get_title          (BtkStatusIcon      *status_icon);
void                  btk_status_icon_set_name           (BtkStatusIcon      *status_icon,
                                                          const bchar        *name);
void                  btk_status_icon_set_visible        (BtkStatusIcon      *status_icon,
							  bboolean            visible);
bboolean              btk_status_icon_get_visible        (BtkStatusIcon      *status_icon);

#if !defined (BTK_DISABLE_DEPRECATED) || defined (BTK_COMPILATION)
void                  btk_status_icon_set_blinking       (BtkStatusIcon      *status_icon,
							  bboolean            blinking);
bboolean              btk_status_icon_get_blinking       (BtkStatusIcon      *status_icon);
#endif

bboolean              btk_status_icon_is_embedded        (BtkStatusIcon      *status_icon);

void                  btk_status_icon_position_menu      (BtkMenu            *menu,
							  bint               *x,
							  bint               *y,
							  bboolean           *push_in,
							  bpointer            user_data);
bboolean              btk_status_icon_get_geometry       (BtkStatusIcon      *status_icon,
							  BdkScreen         **screen,
							  BdkRectangle       *area,
							  BtkOrientation     *orientation);
bboolean              btk_status_icon_get_has_tooltip    (BtkStatusIcon      *status_icon);
bchar                *btk_status_icon_get_tooltip_text   (BtkStatusIcon      *status_icon);
bchar                *btk_status_icon_get_tooltip_markup (BtkStatusIcon      *status_icon);

buint32               btk_status_icon_get_x11_window_id  (BtkStatusIcon      *status_icon);

B_END_DECLS

#endif /* __BTK_STATUS_ICON_H__ */
