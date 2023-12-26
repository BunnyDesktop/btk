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

#ifndef __BTK_TOOL_PALETTE_PRIVATE_H__
#define __BTK_TOOL_PALETTE_PRIVATE_H__

#include <btk/btk.h>

void _btk_tool_palette_get_item_size           (BtkToolPalette   *palette,
                                                BtkRequisition   *item_size,
                                                bboolean          homogeneous_only,
                                                bint             *requested_rows);
void _btk_tool_palette_child_set_drag_source   (BtkWidget        *widget,
                                                bpointer          data);
void _btk_tool_palette_set_expanding_child     (BtkToolPalette   *palette,
                                                BtkWidget        *widget);

void _btk_tool_item_group_palette_reconfigured (BtkToolItemGroup *group);
void _btk_tool_item_group_item_size_request    (BtkToolItemGroup *group,
                                                BtkRequisition   *item_size,
                                                bboolean          homogeneous_only,
                                                bint             *requested_rows);
bint _btk_tool_item_group_get_height_for_width (BtkToolItemGroup *group,
                                                bint              width);
bint _btk_tool_item_group_get_width_for_height (BtkToolItemGroup *group,
                                                bint              height);
void _btk_tool_item_group_paint                (BtkToolItemGroup *group,
                                                bairo_t          *cr);
bint _btk_tool_item_group_get_size_for_limit   (BtkToolItemGroup *group,
                                                bint              limit,
                                                bboolean          vertical,
                                                bboolean          animation);


BtkSizeGroup *_btk_tool_palette_get_size_group (BtkToolPalette   *palette);

#endif /* __BTK_TOOL_PALETTE_PRIVATE_H__ */
