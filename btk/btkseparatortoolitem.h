/* btktoggletoolbutton.h
 *
 * Copyright (C) 2002 Anders Carlsson <andersca@bunny.org>
 * Copyright (C) 2002 James Henstridge <james@daa.com.au>
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

#ifndef __BTK_SEPARATOR_TOOL_ITEM_H__
#define __BTK_SEPARATOR_TOOL_ITEM_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btktoolitem.h>

B_BEGIN_DECLS

#define BTK_TYPE_SEPARATOR_TOOL_ITEM            (btk_separator_tool_item_get_type ())
#define BTK_SEPARATOR_TOOL_ITEM(obj)            (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_SEPARATOR_TOOL_ITEM, BtkSeparatorToolItem))
#define BTK_SEPARATOR_TOOL_ITEM_CLASS(klass)    (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_SEPARATOR_TOOL_ITEM, BtkSeparatorToolItemClass))
#define BTK_IS_SEPARATOR_TOOL_ITEM(obj)         (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_SEPARATOR_TOOL_ITEM))
#define BTK_IS_SEPARATOR_TOOL_ITEM_CLASS(klass) (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_SEPARATOR_TOOL_ITEM))
#define BTK_SEPARATOR_TOOL_ITEM_GET_CLASS(obj)  (B_TYPE_INSTANCE_GET_CLASS((obj), BTK_TYPE_SEPARATOR_TOOL_ITEM, BtkSeparatorToolItemClass))

typedef struct _BtkSeparatorToolItem        BtkSeparatorToolItem;
typedef struct _BtkSeparatorToolItemClass   BtkSeparatorToolItemClass;
typedef struct _BtkSeparatorToolItemPrivate BtkSeparatorToolItemPrivate;

struct _BtkSeparatorToolItem
{
  BtkToolItem parent;

  /*< private >*/
  BtkSeparatorToolItemPrivate *GSEAL (priv);
};

struct _BtkSeparatorToolItemClass
{
  BtkToolItemClass parent_class;

  /* Padding for future expansion */
  void (* _btk_reserved1) (void);
  void (* _btk_reserved2) (void);
  void (* _btk_reserved3) (void);
  void (* _btk_reserved4) (void);
};

GType        btk_separator_tool_item_get_type (void) B_GNUC_CONST;
BtkToolItem *btk_separator_tool_item_new      (void);

gboolean     btk_separator_tool_item_get_draw (BtkSeparatorToolItem *item);
void         btk_separator_tool_item_set_draw (BtkSeparatorToolItem *item,
					       gboolean              draw);

B_END_DECLS

#endif /* __BTK_SEPARATOR_TOOL_ITEM_H__ */
