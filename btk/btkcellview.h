/* btkcellview.h
 * Copyright (C) 2002, 2003  Kristian Rietveld <kris@btk.org>
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

#ifndef __BTK_CELL_VIEW_H__
#define __BTK_CELL_VIEW_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkwidget.h>
#include <btk/btkcellrenderer.h>
#include <btk/btktreemodel.h>

B_BEGIN_DECLS

#define BTK_TYPE_CELL_VIEW                (btk_cell_view_get_type ())
#define BTK_CELL_VIEW(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_CELL_VIEW, BtkCellView))
#define BTK_CELL_VIEW_CLASS(vtable)       (G_TYPE_CHECK_CLASS_CAST ((vtable), BTK_TYPE_CELL_VIEW, BtkCellViewClass))
#define BTK_IS_CELL_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_CELL_VIEW))
#define BTK_IS_CELL_VIEW_CLASS(vtable)    (G_TYPE_CHECK_CLASS_TYPE ((vtable), BTK_TYPE_CELL_VIEW))
#define BTK_CELL_VIEW_GET_CLASS(inst)     (G_TYPE_INSTANCE_GET_CLASS ((inst), BTK_TYPE_CELL_VIEW, BtkCellViewClass))

typedef struct _BtkCellView             BtkCellView;
typedef struct _BtkCellViewClass        BtkCellViewClass;
typedef struct _BtkCellViewPrivate      BtkCellViewPrivate;

struct _BtkCellView
{
  BtkWidget parent_instance;

  /*< private >*/
  BtkCellViewPrivate *GSEAL (priv);
};

struct _BtkCellViewClass
{
  BtkWidgetClass parent_class;
};

GType             btk_cell_view_get_type               (void) B_GNUC_CONST;
BtkWidget        *btk_cell_view_new                    (void);
BtkWidget        *btk_cell_view_new_with_text          (const gchar     *text);
BtkWidget        *btk_cell_view_new_with_markup        (const gchar     *markup);
BtkWidget        *btk_cell_view_new_with_pixbuf        (BdkPixbuf       *pixbuf);

void              btk_cell_view_set_model               (BtkCellView     *cell_view,
                                                         BtkTreeModel    *model);
BtkTreeModel     *btk_cell_view_get_model               (BtkCellView     *cell_view);
void              btk_cell_view_set_displayed_row       (BtkCellView     *cell_view,
                                                         BtkTreePath     *path);
BtkTreePath      *btk_cell_view_get_displayed_row       (BtkCellView     *cell_view);
gboolean          btk_cell_view_get_size_of_row         (BtkCellView     *cell_view,
                                                         BtkTreePath     *path,
                                                         BtkRequisition  *requisition);

void              btk_cell_view_set_background_color    (BtkCellView     *cell_view,
                                                         const BdkColor  *color);
#ifndef BTK_DISABLE_DEPRECATED
GList            *btk_cell_view_get_cell_renderers      (BtkCellView     *cell_view);
#endif

B_END_DECLS

#endif /* __BTK_CELL_VIEW_H__ */
