/* BTK - The GIMP Toolkit
 *
 * Copyright (C) 2009 Matthias Clasen <mclasen@redhat.com>
 * Copyright (C) 2008 Richard Hughes <richard@hughsie.com>
 * Copyright (C) 2009 Bastien Nocera <hadess@hadess.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.         See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 */

#ifndef __BTK_CELL_RENDERER_SPINNER_H__
#define __BTK_CELL_RENDERER_SPINNER_H__

#if !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkcellrenderer.h>

B_BEGIN_DECLS

#define BTK_TYPE_CELL_RENDERER_SPINNER            (btk_cell_renderer_spinner_get_type ())
#define BTK_CELL_RENDERER_SPINNER(obj)            (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_CELL_RENDERER_SPINNER, BtkCellRendererSpinner))
#define BTK_CELL_RENDERER_SPINNER_CLASS(klass)    (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_CELL_RENDERER_SPINNER, BtkCellRendererSpinnerClass))
#define BTK_IS_CELL_RENDERER_SPINNER(obj)         (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_CELL_RENDERER_SPINNER))
#define BTK_IS_CELL_RENDERER_SPINNER_CLASS(klass) (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_CELL_RENDERER_SPINNER))
#define BTK_CELL_RENDERER_SPINNER_GET_CLASS(obj)  (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_CELL_RENDERER_SPINNER, BtkCellRendererSpinnerClass))

typedef struct _BtkCellRendererSpinner        BtkCellRendererSpinner;
typedef struct _BtkCellRendererSpinnerClass   BtkCellRendererSpinnerClass;
typedef struct _BtkCellRendererSpinnerPrivate BtkCellRendererSpinnerPrivate;

struct _BtkCellRendererSpinner
{
  BtkCellRenderer                parent;
  BtkCellRendererSpinnerPrivate *priv;
};

struct _BtkCellRendererSpinnerClass
{
  BtkCellRendererClass parent_class;

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};

GType            btk_cell_renderer_spinner_get_type (void) B_GNUC_CONST;
BtkCellRenderer *btk_cell_renderer_spinner_new      (void);

B_END_DECLS

#endif /* __BTK_CELL_RENDERER_SPINNER_H__ */
