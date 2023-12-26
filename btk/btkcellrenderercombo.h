/* BtkCellRendererCombo
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

#ifndef __BTK_CELL_RENDERER_COMBO_H__
#define __BTK_CELL_RENDERER_COMBO_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btktreemodel.h>
#include <btk/btkcellrenderertext.h>

B_BEGIN_DECLS

#define BTK_TYPE_CELL_RENDERER_COMBO		(btk_cell_renderer_combo_get_type ())
#define BTK_CELL_RENDERER_COMBO(obj)		(B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_CELL_RENDERER_COMBO, BtkCellRendererCombo))
#define BTK_CELL_RENDERER_COMBO_CLASS(klass)	(B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_CELL_RENDERER_COMBO, BtkCellRendererComboClass))
#define BTK_IS_CELL_RENDERER_COMBO(obj)		(B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_CELL_RENDERER_COMBO))
#define BTK_IS_CELL_RENDERER_COMBO_CLASS(klass)	(B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_CELL_RENDERER_COMBO))
#define BTK_CELL_RENDERER_COMBO_GET_CLASS(obj)   (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_CELL_RENDERER_COMBO, BtkCellRendererTextClass))

typedef struct _BtkCellRendererCombo      BtkCellRendererCombo;
typedef struct _BtkCellRendererComboClass BtkCellRendererComboClass;

struct _BtkCellRendererCombo
{
  BtkCellRendererText parent;

  BtkTreeModel *GSEAL (model);
  gint          GSEAL (text_column);
  gboolean      GSEAL (has_entry);

  /*< private >*/
  guint         GSEAL (focus_out_id);
};

struct _BtkCellRendererComboClass
{
  BtkCellRendererTextClass parent;
};

GType            btk_cell_renderer_combo_get_type (void) B_GNUC_CONST;
BtkCellRenderer *btk_cell_renderer_combo_new      (void);

B_END_DECLS

#endif /* __BTK_CELL_RENDERER_COMBO_H__ */
