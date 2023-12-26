/* BTK - The GIMP Toolkit
 *
 * Copyright (C) 2003 Ricardo Fernandez Pascual
 * Copyright (C) 2004 Paolo Borelli
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

#ifndef __BTK_MENU_TOOL_BUTTON_H__
#define __BTK_MENU_TOOL_BUTTON_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkmenu.h>
#include <btk/btktoolbutton.h>

B_BEGIN_DECLS

#define BTK_TYPE_MENU_TOOL_BUTTON         (btk_menu_tool_button_get_type ())
#define BTK_MENU_TOOL_BUTTON(o)           (B_TYPE_CHECK_INSTANCE_CAST ((o), BTK_TYPE_MENU_TOOL_BUTTON, BtkMenuToolButton))
#define BTK_MENU_TOOL_BUTTON_CLASS(k)     (B_TYPE_CHECK_CLASS_CAST((k), BTK_TYPE_MENU_TOOL_BUTTON, BtkMenuToolButtonClass))
#define BTK_IS_MENU_TOOL_BUTTON(o)        (B_TYPE_CHECK_INSTANCE_TYPE ((o), BTK_TYPE_MENU_TOOL_BUTTON))
#define BTK_IS_MENU_TOOL_BUTTON_CLASS(k)  (B_TYPE_CHECK_CLASS_TYPE ((k), BTK_TYPE_MENU_TOOL_BUTTON))
#define BTK_MENU_TOOL_BUTTON_GET_CLASS(o) (B_TYPE_INSTANCE_GET_CLASS ((o), BTK_TYPE_MENU_TOOL_BUTTON, BtkMenuToolButtonClass))

typedef struct _BtkMenuToolButtonClass   BtkMenuToolButtonClass;
typedef struct _BtkMenuToolButton        BtkMenuToolButton;
typedef struct _BtkMenuToolButtonPrivate BtkMenuToolButtonPrivate;

struct _BtkMenuToolButton
{
  BtkToolButton parent;

  /*< private >*/
  BtkMenuToolButtonPrivate *GSEAL (priv);
};

struct _BtkMenuToolButtonClass
{
  BtkToolButtonClass parent_class;

  void (*show_menu) (BtkMenuToolButton *button);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};

GType         btk_menu_tool_button_get_type       (void) B_GNUC_CONST;
BtkToolItem  *btk_menu_tool_button_new            (BtkWidget   *icon_widget,
                                                   const gchar *label);
BtkToolItem  *btk_menu_tool_button_new_from_stock (const gchar *stock_id);

void          btk_menu_tool_button_set_menu       (BtkMenuToolButton *button,
                                                   BtkWidget         *menu);
BtkWidget    *btk_menu_tool_button_get_menu       (BtkMenuToolButton *button);

#ifndef BTK_DISABLE_DEPRECATED
void          btk_menu_tool_button_set_arrow_tooltip (BtkMenuToolButton *button,
                                                      BtkTooltips       *tooltips,
                                                      const gchar       *tip_text,
                                                      const gchar       *tip_private);
#endif /* BTK_DISABLE_DEPRECATED */

void          btk_menu_tool_button_set_arrow_tooltip_text   (BtkMenuToolButton *button,
							     const gchar       *text);
void          btk_menu_tool_button_set_arrow_tooltip_markup (BtkMenuToolButton *button,
							     const gchar       *markup);

B_END_DECLS

#endif /* __BTK_MENU_TOOL_BUTTON_H__ */
