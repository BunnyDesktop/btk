/* btkquartz.h: Utility functions used by the Quartz port
 *
 * Copyright (C) 2006 Imendio AB
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

#ifndef __BTK_QUARTZ_H__
#define __BTK_QUARTZ_H__

#import <Cocoa/Cocoa.h>
#include <btk/btkselection.h>

B_BEGIN_DECLS

NSSet   *_btk_quartz_target_list_to_pasteboard_types    (BtkTargetList *target_list);
NSSet   *_btk_quartz_target_entries_to_pasteboard_types (const BtkTargetEntry *targets,
							 guint                 n_targets);

GList   *_btk_quartz_pasteboard_types_to_atom_list (NSArray  *array);

BtkSelectionData *_btk_quartz_get_selection_data_from_pasteboard (NSPasteboard *pasteboard,
								  BdkAtom       target,
								  BdkAtom       selection);

void _btk_quartz_set_selection_data_for_pasteboard (NSPasteboard *pasteboard,
						    BtkSelectionData *selection_data);
			
NSImage *_btk_quartz_create_image_from_pixbuf (BdkPixbuf *pixbuf);
			    
B_END_DECLS

#endif /* __BTK_QUARTZ_H__ */
