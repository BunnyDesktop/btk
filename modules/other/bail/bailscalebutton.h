/* BAIL - The BUNNY Accessibility Implementation Library
 * Copyright 2008 Jan Arne Petersen <jap@bunny.org>
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

#ifndef __BAIL_SCALE_BUTTON_H__
#define __BAIL_SCALE_BUTTON_H__

#include <btk/btk.h>
#include <bail/bailbutton.h>

B_BEGIN_DECLS

#define BAIL_TYPE_SCALE_BUTTON                     (bail_scale_button_get_type ())
#define BAIL_SCALE_BUTTON(obj)                     (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAIL_TYPE_SCALE_BUTTON, BailScaleButton))
#define BAIL_SCALE_BUTTON_CLASS(klass)             (G_TYPE_CHECK_CLASS_CAST ((klass), BAIL_TYPE_SCALE_BUTTON, BailScaleButtonClass))
#define BAIL_IS_SCALE_BUTTON(obj)                  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAIL_TYPE_SCALE_BUTTON))
#define BAIL_IS_SCALE_BUTTON_CLASS(klass)          (G_TYPE_CHECK_CLASS_TYPE ((klass), BAIL_TYPE_SCALE_BUTTON))
#define BAIL_SCALE_BUTTON_GET_CLASS(obj)           (G_TYPE_INSTANCE_GET_CLASS ((obj), BAIL_TYPE_SCALE_BUTTON, BailScaleButtonClass))

typedef struct _BailScaleButton                   BailScaleButton;
typedef struct _BailScaleButtonClass              BailScaleButtonClass;

struct _BailScaleButton
{
  BailButton parent;
};

struct _BailScaleButtonClass
{
  BailButtonClass parent_class;
};

GType bail_scale_button_get_type (void);

B_END_DECLS

#endif /* __BAIL_SCALE_BUTTON_H__ */
