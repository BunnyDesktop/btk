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

#ifndef __BTK_DRAWING_AREA_H__
#define __BTK_DRAWING_AREA_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkwidget.h>


B_BEGIN_DECLS

#define BTK_TYPE_DRAWING_AREA            (btk_drawing_area_get_type ())
#define BTK_DRAWING_AREA(obj)            (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_DRAWING_AREA, BtkDrawingArea))
#define BTK_DRAWING_AREA_CLASS(klass)    (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_DRAWING_AREA, BtkDrawingAreaClass))
#define BTK_IS_DRAWING_AREA(obj)         (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_DRAWING_AREA))
#define BTK_IS_DRAWING_AREA_CLASS(klass) (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_DRAWING_AREA))
#define BTK_DRAWING_AREA_GET_CLASS(obj)  (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_DRAWING_AREA, BtkDrawingAreaClass))


typedef struct _BtkDrawingArea       BtkDrawingArea;
typedef struct _BtkDrawingAreaClass  BtkDrawingAreaClass;

struct _BtkDrawingArea
{
  BtkWidget widget;

  bpointer GSEAL (draw_data);
};

struct _BtkDrawingAreaClass
{
  BtkWidgetClass parent_class;

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};


GType      btk_drawing_area_get_type   (void) B_GNUC_CONST;
BtkWidget* btk_drawing_area_new        (void);

#ifndef BTK_DISABLE_DEPRECATED
void       btk_drawing_area_size       (BtkDrawingArea      *darea,
					bint                 width,
					bint                 height);
#endif /* BTK_DISABLE_DEPRECATED */


B_END_DECLS

#endif /* __BTK_DRAWING_AREA_H__ */
