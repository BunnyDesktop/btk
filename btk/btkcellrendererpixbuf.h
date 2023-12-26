/* btkcellrendererpixbuf.h
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

#ifndef __BTK_CELL_RENDERER_PIXBUF_H__
#define __BTK_CELL_RENDERER_PIXBUF_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkcellrenderer.h>


B_BEGIN_DECLS


#define BTK_TYPE_CELL_RENDERER_PIXBUF			(btk_cell_renderer_pixbuf_get_type ())
#define BTK_CELL_RENDERER_PIXBUF(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_CELL_RENDERER_PIXBUF, BtkCellRendererPixbuf))
#define BTK_CELL_RENDERER_PIXBUF_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_CELL_RENDERER_PIXBUF, BtkCellRendererPixbufClass))
#define BTK_IS_CELL_RENDERER_PIXBUF(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_CELL_RENDERER_PIXBUF))
#define BTK_IS_CELL_RENDERER_PIXBUF_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_CELL_RENDERER_PIXBUF))
#define BTK_CELL_RENDERER_PIXBUF_GET_CLASS(obj)         (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_CELL_RENDERER_PIXBUF, BtkCellRendererPixbufClass))

typedef struct _BtkCellRendererPixbuf BtkCellRendererPixbuf;
typedef struct _BtkCellRendererPixbufClass BtkCellRendererPixbufClass;

struct _BtkCellRendererPixbuf
{
  BtkCellRenderer parent;

  /*< private >*/
  BdkPixbuf *GSEAL (pixbuf);
  BdkPixbuf *GSEAL (pixbuf_expander_open);
  BdkPixbuf *GSEAL (pixbuf_expander_closed);
};

struct _BtkCellRendererPixbufClass
{
  BtkCellRendererClass parent_class;

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};

GType            btk_cell_renderer_pixbuf_get_type (void) B_GNUC_CONST;
BtkCellRenderer *btk_cell_renderer_pixbuf_new      (void);


B_END_DECLS


#endif /* __BTK_CELL_RENDERER_PIXBUF_H__ */
