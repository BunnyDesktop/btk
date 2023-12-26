/* btktoolbutton.h
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

#ifndef __BTK_TOOL_BUTTON_H__
#define __BTK_TOOL_BUTTON_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btktoolitem.h>

B_BEGIN_DECLS

#define BTK_TYPE_TOOL_BUTTON            (btk_tool_button_get_type ())
#define BTK_TOOL_BUTTON(obj)            (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_TOOL_BUTTON, BtkToolButton))
#define BTK_TOOL_BUTTON_CLASS(klass)    (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_TOOL_BUTTON, BtkToolButtonClass))
#define BTK_IS_TOOL_BUTTON(obj)         (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_TOOL_BUTTON))
#define BTK_IS_TOOL_BUTTON_CLASS(klass) (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_TOOL_BUTTON))
#define BTK_TOOL_BUTTON_GET_CLASS(obj)  (B_TYPE_INSTANCE_GET_CLASS((obj), BTK_TYPE_TOOL_BUTTON, BtkToolButtonClass))

typedef struct _BtkToolButton        BtkToolButton;
typedef struct _BtkToolButtonClass   BtkToolButtonClass;
typedef struct _BtkToolButtonPrivate BtkToolButtonPrivate;

struct _BtkToolButton
{
  BtkToolItem parent;

  /*< private >*/
  BtkToolButtonPrivate *GSEAL (priv);
};

struct _BtkToolButtonClass
{
  BtkToolItemClass parent_class;

  GType button_type;

  /* signal */
  void       (* clicked)             (BtkToolButton    *tool_item);

  /* Padding for future expansion */
  void (* _btk_reserved1) (void);
  void (* _btk_reserved2) (void);
  void (* _btk_reserved3) (void);
  void (* _btk_reserved4) (void);
};

GType        btk_tool_button_get_type       (void) B_GNUC_CONST;
BtkToolItem *btk_tool_button_new            (BtkWidget   *icon_widget,
					     const gchar *label);
BtkToolItem *btk_tool_button_new_from_stock (const gchar *stock_id);

void                  btk_tool_button_set_label         (BtkToolButton *button,
							 const gchar   *label);
const gchar *         btk_tool_button_get_label         (BtkToolButton *button);
void                  btk_tool_button_set_use_underline (BtkToolButton *button,
							 gboolean       use_underline);
gboolean              btk_tool_button_get_use_underline (BtkToolButton *button);
void                  btk_tool_button_set_stock_id      (BtkToolButton *button,
							 const gchar   *stock_id);
const gchar *         btk_tool_button_get_stock_id      (BtkToolButton *button);
void                  btk_tool_button_set_icon_name     (BtkToolButton *button,
							 const gchar   *icon_name);
const gchar *         btk_tool_button_get_icon_name     (BtkToolButton *button);
void                  btk_tool_button_set_icon_widget   (BtkToolButton *button,
							 BtkWidget     *icon_widget);
BtkWidget *           btk_tool_button_get_icon_widget   (BtkToolButton *button);
void                  btk_tool_button_set_label_widget  (BtkToolButton *button,
							 BtkWidget     *label_widget);
BtkWidget *           btk_tool_button_get_label_widget  (BtkToolButton *button);


/* internal function */
BtkWidget *_btk_tool_button_get_button (BtkToolButton *button);

B_END_DECLS

#endif /* __BTK_TOOL_BUTTON_H__ */
