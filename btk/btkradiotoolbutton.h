/* btkradiotoolbutton.h
 *
 * Copyright (C) 2002 Anders Carlsson <andersca@bunny.org>
 * Copyright (C) 2002 James Henstridge <james@daa.com.au>
 * Copyright (C) 2003 Soeren Sandmann <sandmann@daimi.au.dk>
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

#ifndef __BTK_RADIO_TOOL_BUTTON_H__
#define __BTK_RADIO_TOOL_BUTTON_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btktoggletoolbutton.h>

B_BEGIN_DECLS

#define BTK_TYPE_RADIO_TOOL_BUTTON            (btk_radio_tool_button_get_type ())
#define BTK_RADIO_TOOL_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_RADIO_TOOL_BUTTON, BtkRadioToolButton))
#define BTK_RADIO_TOOL_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_RADIO_TOOL_BUTTON, BtkRadioToolButtonClass))
#define BTK_IS_RADIO_TOOL_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_RADIO_TOOL_BUTTON))
#define BTK_IS_RADIO_TOOL_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_RADIO_TOOL_BUTTON))
#define BTK_RADIO_TOOL_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), BTK_TYPE_RADIO_TOOL_BUTTON, BtkRadioToolButtonClass))

typedef struct _BtkRadioToolButton      BtkRadioToolButton;
typedef struct _BtkRadioToolButtonClass BtkRadioToolButtonClass;

struct _BtkRadioToolButton
{
  BtkToggleToolButton parent;
};

struct _BtkRadioToolButtonClass
{
  BtkToggleToolButtonClass parent_class;

  /* Padding for future expansion */
  void (* _btk_reserved1) (void);
  void (* _btk_reserved2) (void);
  void (* _btk_reserved3) (void);
  void (* _btk_reserved4) (void);
};

GType        btk_radio_tool_button_get_type       (void) B_GNUC_CONST;

BtkToolItem *btk_radio_tool_button_new                        (GSList             *group);
BtkToolItem *btk_radio_tool_button_new_from_stock             (GSList             *group,
							       const gchar        *stock_id);
BtkToolItem *btk_radio_tool_button_new_from_widget            (BtkRadioToolButton *group);
BtkToolItem *btk_radio_tool_button_new_with_stock_from_widget (BtkRadioToolButton *group,
							       const gchar        *stock_id);
GSList *     btk_radio_tool_button_get_group                  (BtkRadioToolButton *button);
void         btk_radio_tool_button_set_group                  (BtkRadioToolButton *button,
							       GSList             *group);

B_END_DECLS

#endif /* __BTK_RADIO_TOOL_BUTTON_H__ */
