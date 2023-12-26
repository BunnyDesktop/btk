/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/.
 */

#ifndef __BTK_BUTTON_BOX_H__
#define __BTK_BUTTON_BOX_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkbox.h>


B_BEGIN_DECLS  

#define BTK_TYPE_BUTTON_BOX             (btk_button_box_get_type ())
#define BTK_BUTTON_BOX(obj)             (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_BUTTON_BOX, BtkButtonBox))
#define BTK_BUTTON_BOX_CLASS(klass)     (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_BUTTON_BOX, BtkButtonBoxClass))
#define BTK_IS_BUTTON_BOX(obj)          (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_BUTTON_BOX))
#define BTK_IS_BUTTON_BOX_CLASS(klass)  (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_BUTTON_BOX))
#define BTK_BUTTON_BOX_GET_CLASS(obj)   (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_BUTTON_BOX, BtkButtonBoxClass))
  

#define BTK_BUTTONBOX_DEFAULT -1
 
typedef struct _BtkButtonBox       BtkButtonBox;
typedef struct _BtkButtonBoxClass  BtkButtonBoxClass;

struct _BtkButtonBox
{
  BtkBox box;
  bint GSEAL (child_min_width);
  bint GSEAL (child_min_height);
  bint GSEAL (child_ipad_x);
  bint GSEAL (child_ipad_y);
  BtkButtonBoxStyle GSEAL (layout_style);
};

struct _BtkButtonBoxClass
{
  BtkBoxClass parent_class;
};


GType btk_button_box_get_type (void) B_GNUC_CONST;

BtkButtonBoxStyle btk_button_box_get_layout          (BtkButtonBox      *widget);
void              btk_button_box_set_layout          (BtkButtonBox      *widget,
						      BtkButtonBoxStyle  layout_style);
bboolean          btk_button_box_get_child_secondary (BtkButtonBox      *widget,
						      BtkWidget         *child);
void              btk_button_box_set_child_secondary (BtkButtonBox      *widget,
						      BtkWidget         *child,
						      bboolean           is_secondary);

#ifndef BTK_DISABLE_DEPRECATED
#define btk_button_box_set_spacing(b,s) btk_box_set_spacing (BTK_BOX (b), s)
#define btk_button_box_get_spacing(b)   btk_box_get_spacing (BTK_BOX (b))

void btk_button_box_set_child_size     (BtkButtonBox *widget,
					bint          min_width,
					bint          min_height);
void btk_button_box_set_child_ipadding (BtkButtonBox *widget,
					bint          ipad_x,
					bint          ipad_y);
void btk_button_box_get_child_size     (BtkButtonBox *widget,
					bint         *min_width,
					bint         *min_height);
void btk_button_box_get_child_ipadding (BtkButtonBox *widget,
					bint         *ipad_x,
					bint         *ipad_y);
#endif

/* Internal method - do not use. */
void _btk_button_box_child_requisition (BtkWidget *widget,
					int       *nvis_children,
					int       *nvis_secondaries,
					int       *width,
					int       *height);
B_END_DECLS

#endif /* __BTK_BUTTON_BOX_H__ */
