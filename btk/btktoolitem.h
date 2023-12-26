/* btktoolitem.c
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

#ifndef __BTK_TOOL_ITEM_H__
#define __BTK_TOOL_ITEM_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkbin.h>
#include <btk/btktooltips.h>
#include <btk/btkmenuitem.h>
#include <btk/btksizegroup.h>

G_BEGIN_DECLS

#define BTK_TYPE_TOOL_ITEM            (btk_tool_item_get_type ())
#define BTK_TOOL_ITEM(o)              (G_TYPE_CHECK_INSTANCE_CAST ((o), BTK_TYPE_TOOL_ITEM, BtkToolItem))
#define BTK_TOOL_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_TOOL_ITEM, BtkToolItemClass))
#define BTK_IS_TOOL_ITEM(o)           (G_TYPE_CHECK_INSTANCE_TYPE ((o), BTK_TYPE_TOOL_ITEM))
#define BTK_IS_TOOL_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_TOOL_ITEM))
#define BTK_TOOL_ITEM_GET_CLASS(o)    (G_TYPE_INSTANCE_GET_CLASS((o), BTK_TYPE_TOOL_ITEM, BtkToolItemClass))

typedef struct _BtkToolItem        BtkToolItem;
typedef struct _BtkToolItemClass   BtkToolItemClass;
typedef struct _BtkToolItemPrivate BtkToolItemPrivate;

struct _BtkToolItem
{
  BtkBin parent;

  /*< private >*/
  BtkToolItemPrivate *GSEAL (priv);
};

struct _BtkToolItemClass
{
  BtkBinClass parent_class;

  /* signals */
  gboolean   (* create_menu_proxy)    (BtkToolItem *tool_item);
  void       (* toolbar_reconfigured) (BtkToolItem *tool_item);
#ifndef BTK_DISABLE_DEPRECATED
  gboolean   (* set_tooltip)	      (BtkToolItem *tool_item,
				       BtkTooltips *tooltips,
				       const gchar *tip_text,
				       const gchar *tip_private);
#else
  gpointer _set_tooltip;
#endif

  /* Padding for future expansion */
  void (* _btk_reserved1) (void);
  void (* _btk_reserved2) (void);
  void (* _btk_reserved3) (void);
  void (* _btk_reserved4) (void);
};

GType        btk_tool_item_get_type (void) G_GNUC_CONST;
BtkToolItem *btk_tool_item_new      (void);

void            btk_tool_item_set_homogeneous          (BtkToolItem *tool_item,
							gboolean     homogeneous);
gboolean        btk_tool_item_get_homogeneous          (BtkToolItem *tool_item);

void            btk_tool_item_set_expand               (BtkToolItem *tool_item,
							gboolean     expand);
gboolean        btk_tool_item_get_expand               (BtkToolItem *tool_item);

#ifndef BTK_DISABLE_DEPRECATED
void            btk_tool_item_set_tooltip              (BtkToolItem *tool_item,
							BtkTooltips *tooltips,
							const gchar *tip_text,
							const gchar *tip_private);
#endif /* BTK_DISABLE_DEPRECATED */
void            btk_tool_item_set_tooltip_text         (BtkToolItem *tool_item,
							const gchar *text);
void            btk_tool_item_set_tooltip_markup       (BtkToolItem *tool_item,
							const gchar *markup);

void            btk_tool_item_set_use_drag_window      (BtkToolItem *tool_item,
							gboolean     use_drag_window);
gboolean        btk_tool_item_get_use_drag_window      (BtkToolItem *tool_item);

void            btk_tool_item_set_visible_horizontal   (BtkToolItem *tool_item,
							gboolean     visible_horizontal);
gboolean        btk_tool_item_get_visible_horizontal   (BtkToolItem *tool_item);

void            btk_tool_item_set_visible_vertical     (BtkToolItem *tool_item,
							gboolean     visible_vertical);
gboolean        btk_tool_item_get_visible_vertical     (BtkToolItem *tool_item);

gboolean        btk_tool_item_get_is_important         (BtkToolItem *tool_item);
void            btk_tool_item_set_is_important         (BtkToolItem *tool_item,
							gboolean     is_important);

BangoEllipsizeMode btk_tool_item_get_ellipsize_mode    (BtkToolItem *tool_item);
BtkIconSize     btk_tool_item_get_icon_size            (BtkToolItem *tool_item);
BtkOrientation  btk_tool_item_get_orientation          (BtkToolItem *tool_item);
BtkToolbarStyle btk_tool_item_get_toolbar_style        (BtkToolItem *tool_item);
BtkReliefStyle  btk_tool_item_get_relief_style         (BtkToolItem *tool_item);
gfloat          btk_tool_item_get_text_alignment       (BtkToolItem *tool_item);
BtkOrientation  btk_tool_item_get_text_orientation     (BtkToolItem *tool_item);
BtkSizeGroup *  btk_tool_item_get_text_size_group      (BtkToolItem *tool_item);

BtkWidget *     btk_tool_item_retrieve_proxy_menu_item (BtkToolItem *tool_item);
BtkWidget *     btk_tool_item_get_proxy_menu_item      (BtkToolItem *tool_item,
							const gchar *menu_item_id);
void            btk_tool_item_set_proxy_menu_item      (BtkToolItem *tool_item,
							const gchar *menu_item_id,
							BtkWidget   *menu_item);
void		btk_tool_item_rebuild_menu	       (BtkToolItem *tool_item);

void            btk_tool_item_toolbar_reconfigured     (BtkToolItem *tool_item);

/* private */

gboolean       _btk_tool_item_create_menu_proxy        (BtkToolItem *tool_item);

G_END_DECLS

#endif /* __BTK_TOOL_ITEM_H__ */
