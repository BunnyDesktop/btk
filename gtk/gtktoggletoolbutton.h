/* btktoggletoolbutton.h
 *
 * Copyright (C) 2002 Anders Carlsson <andersca@gnome.org>
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

#ifndef __BTK_TOGGLE_TOOL_BUTTON_H__
#define __BTK_TOGGLE_TOOL_BUTTON_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btktoolbutton.h>

G_BEGIN_DECLS

#define BTK_TYPE_TOGGLE_TOOL_BUTTON             (btk_toggle_tool_button_get_type ())
#define BTK_TOGGLE_TOOL_BUTTON(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_TOGGLE_TOOL_BUTTON, BtkToggleToolButton))
#define BTK_TOGGLE_TOOL_BUTTON_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_TOGGLE_TOOL_BUTTON, BtkToggleToolButtonClass))
#define BTK_IS_TOGGLE_TOOL_BUTTON(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_TOGGLE_TOOL_BUTTON))
#define BTK_IS_TOGGLE_TOOL_BUTTON_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_TOGGLE_TOOL_BUTTON))
#define BTK_TOGGLE_TOOL_BUTTON_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), BTK_TYPE_TOGGLE_TOOL_BUTTON, BtkToggleToolButtonClass))

typedef struct _BtkToggleToolButton        BtkToggleToolButton;
typedef struct _BtkToggleToolButtonClass   BtkToggleToolButtonClass;
typedef struct _BtkToggleToolButtonPrivate BtkToggleToolButtonPrivate;

struct _BtkToggleToolButton
{
  BtkToolButton parent;

  /*< private >*/
  BtkToggleToolButtonPrivate *GSEAL (priv);
};

struct _BtkToggleToolButtonClass
{
  BtkToolButtonClass parent_class;

  /* signal */
  void (* toggled) (BtkToggleToolButton *button);

  /* Padding for future expansion */
  void (* _btk_reserved1) (void);
  void (* _btk_reserved2) (void);
  void (* _btk_reserved3) (void);
  void (* _btk_reserved4) (void);
};

GType        btk_toggle_tool_button_get_type       (void) G_GNUC_CONST;
BtkToolItem *btk_toggle_tool_button_new            (void);
BtkToolItem *btk_toggle_tool_button_new_from_stock (const gchar *stock_id);

void         btk_toggle_tool_button_set_active     (BtkToggleToolButton *button,
						    gboolean             is_active);
gboolean     btk_toggle_tool_button_get_active     (BtkToggleToolButton *button);

G_END_DECLS

#endif /* __BTK_TOGGLE_TOOL_BUTTON_H__ */
