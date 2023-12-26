/* BtkToolPalette -- A tool palette with categories and DnD support
 * Copyright (C) 2008  Openismus GmbH
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Authors:
 *      Mathias Hasselmann
 */

#ifndef __BTK_TOOL_ITEM_GROUP_H__
#define __BTK_TOOL_ITEM_GROUP_H__

#if !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkcontainer.h>
#include <btk/btktoolitem.h>

B_BEGIN_DECLS

#define BTK_TYPE_TOOL_ITEM_GROUP           (btk_tool_item_group_get_type ())
#define BTK_TOOL_ITEM_GROUP(obj)           (B_TYPE_CHECK_INSTANCE_CAST (obj, BTK_TYPE_TOOL_ITEM_GROUP, BtkToolItemGroup))
#define BTK_TOOL_ITEM_GROUP_CLASS(cls)     (B_TYPE_CHECK_CLASS_CAST (cls, BTK_TYPE_TOOL_ITEM_GROUP, BtkToolItemGroupClass))
#define BTK_IS_TOOL_ITEM_GROUP(obj)        (B_TYPE_CHECK_INSTANCE_TYPE (obj, BTK_TYPE_TOOL_ITEM_GROUP))
#define BTK_IS_TOOL_ITEM_GROUP_CLASS(obj)  (B_TYPE_CHECK_CLASS_TYPE (obj, BTK_TYPE_TOOL_ITEM_GROUP))
#define BTK_TOOL_ITEM_GROUP_GET_CLASS(obj) (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_TOOL_ITEM_GROUP, BtkToolItemGroupClass))

typedef struct _BtkToolItemGroup        BtkToolItemGroup;
typedef struct _BtkToolItemGroupClass   BtkToolItemGroupClass;
typedef struct _BtkToolItemGroupPrivate BtkToolItemGroupPrivate;

/**
 * BtkToolItemGroup:
 *
 * This should not be accessed directly. Use the accessor functions below.
 */
struct _BtkToolItemGroup
{
  BtkContainer parent_instance;
  BtkToolItemGroupPrivate *priv;
};

struct _BtkToolItemGroupClass
{
  BtkContainerClass parent_class;
};

GType                 btk_tool_item_group_get_type          (void) B_GNUC_CONST;
BtkWidget*            btk_tool_item_group_new               (const gchar        *label);

void                  btk_tool_item_group_set_label         (BtkToolItemGroup   *group,
                                                             const gchar        *label);
void                  btk_tool_item_group_set_label_widget  (BtkToolItemGroup   *group,
                                                             BtkWidget          *label_widget);
void                  btk_tool_item_group_set_collapsed      (BtkToolItemGroup  *group,
                                                             gboolean            collapsed);
void                  btk_tool_item_group_set_ellipsize     (BtkToolItemGroup   *group,
                                                             BangoEllipsizeMode  ellipsize);
void                  btk_tool_item_group_set_header_relief (BtkToolItemGroup   *group,
                                                             BtkReliefStyle      style);

const gchar *         btk_tool_item_group_get_label         (BtkToolItemGroup   *group);
BtkWidget            *btk_tool_item_group_get_label_widget  (BtkToolItemGroup   *group);
gboolean              btk_tool_item_group_get_collapsed     (BtkToolItemGroup   *group);
BangoEllipsizeMode    btk_tool_item_group_get_ellipsize     (BtkToolItemGroup   *group);
BtkReliefStyle        btk_tool_item_group_get_header_relief (BtkToolItemGroup   *group);

void                  btk_tool_item_group_insert            (BtkToolItemGroup   *group,
                                                             BtkToolItem        *item,
                                                             gint                position);
void                  btk_tool_item_group_set_item_position (BtkToolItemGroup   *group,
                                                             BtkToolItem        *item,
                                                             gint                position);
gint                  btk_tool_item_group_get_item_position (BtkToolItemGroup   *group,
                                                             BtkToolItem        *item);

guint                 btk_tool_item_group_get_n_items       (BtkToolItemGroup   *group);
BtkToolItem*          btk_tool_item_group_get_nth_item      (BtkToolItemGroup   *group,
                                                             guint               index);
BtkToolItem*          btk_tool_item_group_get_drop_item     (BtkToolItemGroup   *group,
                                                             gint                x,
                                                             gint                y);

B_END_DECLS

#endif /* __BTK_TOOL_ITEM_GROUP_H__ */
