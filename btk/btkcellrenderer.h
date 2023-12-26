/* btkcellrenderer.h
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

#ifndef __BTK_CELL_RENDERER_H__
#define __BTK_CELL_RENDERER_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkcelleditable.h>

B_BEGIN_DECLS

typedef enum
{
  BTK_CELL_RENDERER_SELECTED    = 1 << 0,
  BTK_CELL_RENDERER_PRELIT      = 1 << 1,
  BTK_CELL_RENDERER_INSENSITIVE = 1 << 2,
  /* this flag means the cell is in the sort column/row */
  BTK_CELL_RENDERER_SORTED      = 1 << 3,
  BTK_CELL_RENDERER_FOCUSED     = 1 << 4
} BtkCellRendererState;

typedef enum
{
  BTK_CELL_RENDERER_MODE_INERT,
  BTK_CELL_RENDERER_MODE_ACTIVATABLE,
  BTK_CELL_RENDERER_MODE_EDITABLE
} BtkCellRendererMode;

#define BTK_TYPE_CELL_RENDERER		  (btk_cell_renderer_get_type ())
#define BTK_CELL_RENDERER(obj)		  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_CELL_RENDERER, BtkCellRenderer))
#define BTK_CELL_RENDERER_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_CELL_RENDERER, BtkCellRendererClass))
#define BTK_IS_CELL_RENDERER(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_CELL_RENDERER))
#define BTK_IS_CELL_RENDERER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_CELL_RENDERER))
#define BTK_CELL_RENDERER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_CELL_RENDERER, BtkCellRendererClass))

typedef struct _BtkCellRenderer BtkCellRenderer;
typedef struct _BtkCellRendererClass BtkCellRendererClass;

struct _BtkCellRenderer
{
  BtkObject parent;

  gfloat GSEAL (xalign);
  gfloat GSEAL (yalign);

  gint GSEAL (width);
  gint GSEAL (height);

  guint16 GSEAL (xpad);
  guint16 GSEAL (ypad);

  guint GSEAL (mode) : 2;
  guint GSEAL (visible) : 1;
  guint GSEAL (is_expander) : 1;
  guint GSEAL (is_expanded) : 1;
  guint GSEAL (cell_background_set) : 1;
  guint GSEAL (sensitive) : 1;
  guint GSEAL (editing) : 1;
};

struct _BtkCellRendererClass
{
  BtkObjectClass parent_class;

  /* vtable - not signals */
  void             (* get_size)      (BtkCellRenderer      *cell,
				      BtkWidget            *widget,
				      BdkRectangle         *cell_area,
				      gint                 *x_offset,
				      gint                 *y_offset,
				      gint                 *width,
				      gint                 *height);
  void             (* render)        (BtkCellRenderer      *cell,
				      BdkDrawable          *window,
				      BtkWidget            *widget,
				      BdkRectangle         *background_area,
				      BdkRectangle         *cell_area,
				      BdkRectangle         *expose_area,
				      BtkCellRendererState  flags);
  gboolean         (* activate)      (BtkCellRenderer      *cell,
				      BdkEvent             *event,
				      BtkWidget            *widget,
				      const gchar          *path,
				      BdkRectangle         *background_area,
				      BdkRectangle         *cell_area,
				      BtkCellRendererState  flags);
  BtkCellEditable *(* start_editing) (BtkCellRenderer      *cell,
				      BdkEvent             *event,
				      BtkWidget            *widget,
				      const gchar          *path,
				      BdkRectangle         *background_area,
				      BdkRectangle         *cell_area,
				      BtkCellRendererState  flags);

  /* Signals */
  void (* editing_canceled) (BtkCellRenderer *cell);
  void (* editing_started)  (BtkCellRenderer *cell,
			     BtkCellEditable *editable,
			     const gchar     *path);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
};

GType            btk_cell_renderer_get_type       (void) B_GNUC_CONST;

void             btk_cell_renderer_get_size       (BtkCellRenderer      *cell,
						   BtkWidget            *widget,
						   const BdkRectangle   *cell_area,
						   gint                 *x_offset,
						   gint                 *y_offset,
						   gint                 *width,
						   gint                 *height);
void             btk_cell_renderer_render         (BtkCellRenderer      *cell,
						   BdkWindow            *window,
						   BtkWidget            *widget,
						   const BdkRectangle   *background_area,
						   const BdkRectangle   *cell_area,
						   const BdkRectangle   *expose_area,
						   BtkCellRendererState  flags);
gboolean         btk_cell_renderer_activate       (BtkCellRenderer      *cell,
						   BdkEvent             *event,
						   BtkWidget            *widget,
						   const gchar          *path,
						   const BdkRectangle   *background_area,
						   const BdkRectangle   *cell_area,
						   BtkCellRendererState  flags);
BtkCellEditable *btk_cell_renderer_start_editing  (BtkCellRenderer      *cell,
						   BdkEvent             *event,
						   BtkWidget            *widget,
						   const gchar          *path,
						   const BdkRectangle   *background_area,
						   const BdkRectangle   *cell_area,
						   BtkCellRendererState  flags);

void             btk_cell_renderer_set_fixed_size (BtkCellRenderer      *cell,
						   gint                  width,
						   gint                  height);
void             btk_cell_renderer_get_fixed_size (BtkCellRenderer      *cell,
						   gint                 *width,
						   gint                 *height);

void             btk_cell_renderer_set_alignment  (BtkCellRenderer      *cell,
                                                   gfloat                xalign,
                                                   gfloat                yalign);
void             btk_cell_renderer_get_alignment  (BtkCellRenderer      *cell,
                                                   gfloat               *xalign,
                                                   gfloat               *yalign);

void             btk_cell_renderer_set_padding    (BtkCellRenderer      *cell,
                                                   gint                  xpad,
                                                   gint                  ypad);
void             btk_cell_renderer_get_padding    (BtkCellRenderer      *cell,
                                                   gint                 *xpad,
                                                   gint                 *ypad);

void             btk_cell_renderer_set_visible    (BtkCellRenderer      *cell,
                                                   gboolean              visible);
gboolean         btk_cell_renderer_get_visible    (BtkCellRenderer      *cell);

void             btk_cell_renderer_set_sensitive  (BtkCellRenderer      *cell,
                                                   gboolean              sensitive);
gboolean         btk_cell_renderer_get_sensitive  (BtkCellRenderer      *cell);

/* For use by cell renderer implementations only */
#ifndef BTK_DISABLE_DEPRECATED
void btk_cell_renderer_editing_canceled (BtkCellRenderer *cell);
#endif
void btk_cell_renderer_stop_editing     (BtkCellRenderer *cell,
				         gboolean         canceled);


B_END_DECLS

#endif /* __BTK_CELL_RENDERER_H__ */
