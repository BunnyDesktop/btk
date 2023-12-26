/* BAIL - The BUNNY Accessibility Implementation Library
 * Copyright 2001 Sun Microsystems Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __BAIL_MISC_H__
#define __BAIL_MISC_H__

#include <bunnylib-object.h>
#include <btk/btk.h>
#include <bango/bango.h>

B_BEGIN_DECLS

BatkAttributeSet* bail_misc_add_attribute          (BatkAttributeSet   *attrib_set,
                                                   BatkTextAttribute   attr,
                                                   bchar             *value);
BatkAttributeSet* bail_misc_layout_get_run_attributes
                                                  (BatkAttributeSet   *attrib_set,
                                                   BangoLayout       *layout,
                                                   bchar             *text,
                                                   bint              offset,
                                                   bint              *start_offset,
                                                   bint              *end_offset);

BatkAttributeSet* bail_misc_get_default_attributes (BatkAttributeSet   *attrib_set,
                                                   BangoLayout       *layout,
                                                   BtkWidget         *widget);

void             bail_misc_get_extents_from_bango_rectangle
                                                  (BtkWidget         *widget,
                                                   BangoRectangle    *char_rect,
                                                   bint              x_layout,
                                                   bint              y_layout,
                                                   bint              *x,
                    		                   bint              *y,
                                                   bint              *width,
                                                   bint              *height,
                                                   BatkCoordType      coords);

bint             bail_misc_get_index_at_point_in_layout
                                                  (BtkWidget         *widget,
                                                   BangoLayout       *layout, 
                                                   bint              x_layout,
                                                   bint              y_layout,
                                                   bint              x,
                                                   bint              y,
                                                   BatkCoordType      coords);

void		 bail_misc_get_origins            (BtkWidget         *widget,
                                                   bint              *x_window,
					           bint              *y_window,
					           bint              *x_toplevel,
					           bint              *y_toplevel);

BatkAttributeSet* bail_misc_add_to_attr_set        (BatkAttributeSet   *attrib_set,
			                           BtkTextAttributes *attrs,
			                           BatkTextAttribute  attr);

BatkAttributeSet* bail_misc_buffer_get_run_attributes
                                                  (BtkTextBuffer     *buffer,
                                                   bint              offset,
                                                   bint              *start_offset,
                                                   bint              *end_offset);

B_END_DECLS

#endif /*__BAIL_MISC_H__ */
