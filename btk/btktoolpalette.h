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

#ifndef __BTK_TOOL_PALETTE_H__
#define __BTK_TOOL_PALETTE_H__

#if !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkcontainer.h>
#include <btk/btkdnd.h>
#include <btk/btktoolitem.h>

B_BEGIN_DECLS

#define BTK_TYPE_TOOL_PALETTE           (btk_tool_palette_get_type ())
#define BTK_TOOL_PALETTE(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, BTK_TYPE_TOOL_PALETTE, BtkToolPalette))
#define BTK_TOOL_PALETTE_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, BTK_TYPE_TOOL_PALETTE, BtkToolPaletteClass))
#define BTK_IS_TOOL_PALETTE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, BTK_TYPE_TOOL_PALETTE))
#define BTK_IS_TOOL_PALETTE_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, BTK_TYPE_TOOL_PALETTE))
#define BTK_TOOL_PALETTE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_TOOL_PALETTE, BtkToolPaletteClass))

typedef struct _BtkToolPalette           BtkToolPalette;
typedef struct _BtkToolPaletteClass      BtkToolPaletteClass;
typedef struct _BtkToolPalettePrivate    BtkToolPalettePrivate;

/**
 * BtkToolPaletteDragTargets:
 * @BTK_TOOL_PALETTE_DRAG_ITEMS: Support drag of items.
 * @BTK_TOOL_PALETTE_DRAG_GROUPS: Support drag of groups.
 *
 * Flags used to specify the supported drag targets.
 */
typedef enum /*< flags >*/
{
  BTK_TOOL_PALETTE_DRAG_ITEMS = (1 << 0),
  BTK_TOOL_PALETTE_DRAG_GROUPS = (1 << 1)
}
BtkToolPaletteDragTargets;

/**
 * BtkToolPalette:
 *
 * This should not be accessed directly. Use the accessor functions below.
 */
struct _BtkToolPalette
{
  BtkContainer parent_instance;
  BtkToolPalettePrivate *priv;
};

struct _BtkToolPaletteClass
{
  BtkContainerClass parent_class;

  void (*set_scroll_adjustments) (BtkWidget     *widget,
                                  BtkAdjustment *hadjustment,
                                  BtkAdjustment *vadjustment);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
  void (*_btk_reserved5) (void);
  void (*_btk_reserved6) (void);
};

GType                          btk_tool_palette_get_type              (void) B_GNUC_CONST;
BtkWidget*                     btk_tool_palette_new                   (void);

void                           btk_tool_palette_set_group_position    (BtkToolPalette            *palette,
                                                                       BtkToolItemGroup          *group,
                                                                       gint                       position);
void                           btk_tool_palette_set_exclusive         (BtkToolPalette            *palette,
                                                                       BtkToolItemGroup          *group,
                                                                       gboolean                   exclusive);
void                           btk_tool_palette_set_expand            (BtkToolPalette            *palette,
                                                                       BtkToolItemGroup          *group,
                                                                       gboolean                   expand);

gint                           btk_tool_palette_get_group_position    (BtkToolPalette            *palette,
                                                                       BtkToolItemGroup          *group);
gboolean                       btk_tool_palette_get_exclusive         (BtkToolPalette            *palette,
                                                                       BtkToolItemGroup          *group);
gboolean                       btk_tool_palette_get_expand            (BtkToolPalette            *palette,
                                                                       BtkToolItemGroup          *group);

void                           btk_tool_palette_set_icon_size         (BtkToolPalette            *palette,
                                                                       BtkIconSize                icon_size);
void                           btk_tool_palette_unset_icon_size       (BtkToolPalette            *palette);
void                           btk_tool_palette_set_style             (BtkToolPalette            *palette,
                                                                       BtkToolbarStyle            style);
void                           btk_tool_palette_unset_style           (BtkToolPalette            *palette);

BtkIconSize                    btk_tool_palette_get_icon_size         (BtkToolPalette            *palette);
BtkToolbarStyle                btk_tool_palette_get_style             (BtkToolPalette            *palette);

BtkToolItem*                   btk_tool_palette_get_drop_item         (BtkToolPalette            *palette,
                                                                       gint                       x,
                                                                       gint                       y);
BtkToolItemGroup*              btk_tool_palette_get_drop_group        (BtkToolPalette            *palette,
                                                                       gint                       x,
                                                                       gint                       y);
BtkWidget*                     btk_tool_palette_get_drag_item         (BtkToolPalette            *palette,
                                                                       const BtkSelectionData    *selection);

void                           btk_tool_palette_set_drag_source       (BtkToolPalette            *palette,
                                                                       BtkToolPaletteDragTargets  targets);
void                           btk_tool_palette_add_drag_dest         (BtkToolPalette            *palette,
                                                                       BtkWidget                 *widget,
                                                                       BtkDestDefaults            flags,
                                                                       BtkToolPaletteDragTargets  targets,
                                                                       BdkDragAction              actions);

BtkAdjustment*                 btk_tool_palette_get_hadjustment       (BtkToolPalette            *palette);
BtkAdjustment*                 btk_tool_palette_get_vadjustment       (BtkToolPalette            *palette);

const BtkTargetEntry *         btk_tool_palette_get_drag_target_item  (void) B_GNUC_CONST;
const BtkTargetEntry *         btk_tool_palette_get_drag_target_group (void) B_GNUC_CONST;


B_END_DECLS

#endif /* __BTK_TOOL_PALETTE_H__ */
