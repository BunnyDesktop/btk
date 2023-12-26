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

G_BEGIN_DECLS

BatkAttributeSet* bail_misc_add_attribute          (BatkAttributeSet   *attrib_set,
                                                   BatkTextAttribute   attr,
                                                   gchar             *value);
BatkAttributeSet* bail_misc_layout_get_run_attributes
                                                  (BatkAttributeSet   *attrib_set,
                                                   BangoLayout       *layout,
                                                   gchar             *text,
                                                   gint              offset,
                                                   gint              *start_offset,
                                                   gint              *end_offset);

BatkAttributeSet* bail_misc_get_default_attributes (BatkAttributeSet   *attrib_set,
                                                   BangoLayout       *layout,
                                                   BtkWidget         *widget);

void             bail_misc_get_extents_from_bango_rectangle
                                                  (BtkWidget         *widget,
                                                   BangoRectangle    *char_rect,
                                                   gint              x_layout,
                                                   gint              y_layout,
                                                   gint              *x,
                    		                   gint              *y,
                                                   gint              *width,
                                                   gint              *height,
                                                   BatkCoordType      coords);

gint             bail_misc_get_index_at_point_in_layout
                                                  (BtkWidget         *widget,
                                                   BangoLayout       *layout, 
                                                   gint              x_layout,
                                                   gint              y_layout,
                                                   gint              x,
                                                   gint              y,
                                                   BatkCoordType      coords);

void		 bail_misc_get_origins            (BtkWidget         *widget,
                                                   gint              *x_window,
					           gint              *y_window,
					           gint              *x_toplevel,
					           gint              *y_toplevel);

BatkAttributeSet* bail_misc_add_to_attr_set        (BatkAttributeSet   *attrib_set,
			                           BtkTextAttributes *attrs,
			                           BatkTextAttribute  attr);

BatkAttributeSet* bail_misc_buffer_get_run_attributes
                                                  (BtkTextBuffer     *buffer,
                                                   gint              offset,
                                                   gint              *start_offset,
                                                   gint              *end_offset);

G_END_DECLS

#endif /*__BAIL_MISC_H__ */
