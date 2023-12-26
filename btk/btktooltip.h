/* btktooltip.h
 *
 * Copyright (C) 2006-2007 Imendio AB
 * Contact: Kristian Rietveld <kris@imendio.com>
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

#ifndef __BTK_TOOLTIP_H__
#define __BTK_TOOLTIP_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkwindow.h>

G_BEGIN_DECLS

#define BTK_TYPE_TOOLTIP                 (btk_tooltip_get_type ())
#define BTK_TOOLTIP(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_TOOLTIP, BtkTooltip))
#define BTK_IS_TOOLTIP(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_TOOLTIP))

GType btk_tooltip_get_type (void);

void btk_tooltip_set_markup              (BtkTooltip         *tooltip,
                                          const gchar        *markup);
void btk_tooltip_set_text                (BtkTooltip         *tooltip,
                                          const gchar        *text);
void btk_tooltip_set_icon                (BtkTooltip         *tooltip,
                                          BdkPixbuf          *pixbuf);
void btk_tooltip_set_icon_from_stock     (BtkTooltip         *tooltip,
                                          const gchar        *stock_id,
                                          BtkIconSize         size);
void btk_tooltip_set_icon_from_icon_name (BtkTooltip         *tooltip,
				          const gchar        *icon_name,
				          BtkIconSize         size);
void btk_tooltip_set_icon_from_gicon     (BtkTooltip         *tooltip,
					  GIcon              *gicon,
					  BtkIconSize         size);
void btk_tooltip_set_custom	         (BtkTooltip         *tooltip,
                                          BtkWidget          *custom_widget);

void btk_tooltip_set_tip_area            (BtkTooltip         *tooltip,
                                          const BdkRectangle *rect);

void btk_tooltip_trigger_tooltip_query   (BdkDisplay         *display);


void _btk_tooltip_focus_in               (BtkWidget          *widget);
void _btk_tooltip_focus_out              (BtkWidget          *widget);
void _btk_tooltip_toggle_keyboard_mode   (BtkWidget          *widget);
void _btk_tooltip_handle_event           (BdkEvent           *event);
void _btk_tooltip_hide                   (BtkWidget          *widget);

BtkWidget * _btk_widget_find_at_coords   (BdkWindow          *window,
                                          gint                window_x,
                                          gint                window_y,
                                          gint               *widget_x,
                                          gint               *widget_y);

G_END_DECLS

#endif /* __BTK_TOOLTIP_H__ */
