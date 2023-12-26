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

#ifndef __BTK_HBUTTON_BOX_H__
#define __BTK_HBUTTON_BOX_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkbbox.h>


B_BEGIN_DECLS

#define BTK_TYPE_HBUTTON_BOX                  (btk_hbutton_box_get_type ())
#define BTK_HBUTTON_BOX(obj)                  (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_HBUTTON_BOX, BtkHButtonBox))
#define BTK_HBUTTON_BOX_CLASS(klass)          (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_HBUTTON_BOX, BtkHButtonBoxClass))
#define BTK_IS_HBUTTON_BOX(obj)               (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_HBUTTON_BOX))
#define BTK_IS_HBUTTON_BOX_CLASS(klass)       (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_HBUTTON_BOX))
#define BTK_HBUTTON_BOX_GET_CLASS(obj)        (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_HBUTTON_BOX, BtkHButtonBoxClass))


typedef struct _BtkHButtonBox       BtkHButtonBox;
typedef struct _BtkHButtonBoxClass  BtkHButtonBoxClass;

struct _BtkHButtonBox
{
  BtkButtonBox button_box;
};

struct _BtkHButtonBoxClass
{
  BtkButtonBoxClass parent_class;
};


GType      btk_hbutton_box_get_type (void) B_GNUC_CONST;
BtkWidget* btk_hbutton_box_new      (void);

/* buttons can be added by btk_container_add() */

#ifndef BTK_DISABLE_DEPRECATED
bint btk_hbutton_box_get_spacing_default (void);
BtkButtonBoxStyle btk_hbutton_box_get_layout_default (void);

void btk_hbutton_box_set_spacing_default (bint spacing);
void btk_hbutton_box_set_layout_default (BtkButtonBoxStyle layout);
#endif

/* private API */
BtkButtonBoxStyle _btk_hbutton_box_get_layout_default (void);

B_END_DECLS

#endif /* __BTK_HBUTTON_BOX_H__ */
