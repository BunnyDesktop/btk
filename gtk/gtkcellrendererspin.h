/* BtkCellRendererSpin
 * Copyright (C) 2004 Lorenzo Gil Sanchez
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

#ifndef __BTK_CELL_RENDERER_SPIN_H__
#define __BTK_CELL_RENDERER_SPIN_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkcellrenderertext.h>

G_BEGIN_DECLS

#define BTK_TYPE_CELL_RENDERER_SPIN		(btk_cell_renderer_spin_get_type ())
#define BTK_CELL_RENDERER_SPIN(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_CELL_RENDERER_SPIN, BtkCellRendererSpin))
#define BTK_CELL_RENDERER_SPIN_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_CELL_RENDERER_SPIN, BtkCellRendererSpinClass))
#define BTK_IS_CELL_RENDERER_SPIN(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_CELL_RENDERER_SPIN))
#define BTK_IS_CELL_RENDERER_SPIN_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_CELL_RENDERER_SPIN))
#define BTK_CELL_RENDERER_SPIN_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_CELL_RENDERER_SPIN, BtkCellRendererTextClass))

typedef struct _BtkCellRendererSpin        BtkCellRendererSpin;
typedef struct _BtkCellRendererSpinClass   BtkCellRendererSpinClass;
typedef struct _BtkCellRendererSpinPrivate BtkCellRendererSpinPrivate;

struct _BtkCellRendererSpin
{
  BtkCellRendererText parent;
};

struct _BtkCellRendererSpinClass
{
  BtkCellRendererTextClass parent;
};

GType            btk_cell_renderer_spin_get_type (void);
BtkCellRenderer *btk_cell_renderer_spin_new      (void);

G_END_DECLS

#endif  /* __BTK_CELL_RENDERER_SPIN_H__ */
