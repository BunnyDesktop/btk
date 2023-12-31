/* btkcellrenderertext.h
 * Copyright (C) 2000  Red Hat, Inc.,  Jonathan Blandford <jrb@redhat.com>
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

#ifndef __BTK_CELL_RENDERER_TEXT_H__
#define __BTK_CELL_RENDERER_TEXT_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkcellrenderer.h>


B_BEGIN_DECLS


#define BTK_TYPE_CELL_RENDERER_TEXT		(btk_cell_renderer_text_get_type ())
#define BTK_CELL_RENDERER_TEXT(obj)		(B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_CELL_RENDERER_TEXT, BtkCellRendererText))
#define BTK_CELL_RENDERER_TEXT_CLASS(klass)	(B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_CELL_RENDERER_TEXT, BtkCellRendererTextClass))
#define BTK_IS_CELL_RENDERER_TEXT(obj)		(B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_CELL_RENDERER_TEXT))
#define BTK_IS_CELL_RENDERER_TEXT_CLASS(klass)	(B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_CELL_RENDERER_TEXT))
#define BTK_CELL_RENDERER_TEXT_GET_CLASS(obj)   (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_CELL_RENDERER_TEXT, BtkCellRendererTextClass))

typedef struct _BtkCellRendererText      BtkCellRendererText;
typedef struct _BtkCellRendererTextClass BtkCellRendererTextClass;

struct _BtkCellRendererText
{
  BtkCellRenderer parent;

  /*< private >*/
  bchar *GSEAL (text);
  BangoFontDescription *GSEAL (font);
  bdouble GSEAL (font_scale);
  BangoColor GSEAL (foreground);
  BangoColor GSEAL (background);

  BangoAttrList *GSEAL (extra_attrs);

  BangoUnderline GSEAL (underline_style);

  bint GSEAL (rise);
  bint GSEAL (fixed_height_rows);

  buint GSEAL (strikethrough) : 1;

  buint GSEAL (editable)  : 1;

  buint GSEAL (scale_set) : 1;

  buint GSEAL (foreground_set) : 1;
  buint GSEAL (background_set) : 1;

  buint GSEAL (underline_set) : 1;

  buint GSEAL (rise_set) : 1;

  buint GSEAL (strikethrough_set) : 1;

  buint GSEAL (editable_set) : 1;
  buint GSEAL (calc_fixed_height) : 1;
};

struct _BtkCellRendererTextClass
{
  BtkCellRendererClass parent_class;

  void (* edited) (BtkCellRendererText *cell_renderer_text,
		   const bchar         *path,
		   const bchar         *new_text);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};

GType            btk_cell_renderer_text_get_type (void) B_GNUC_CONST;
BtkCellRenderer *btk_cell_renderer_text_new      (void);

void             btk_cell_renderer_text_set_fixed_height_from_font (BtkCellRendererText *renderer,
								    bint                 number_of_rows);


B_END_DECLS

#endif /* __BTK_CELL_RENDERER_TEXT_H__ */
