/* btktextdisplay.c - display layed-out text
 *
 * Copyright (c) 2010 Red Hat, Inc.
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
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/.
 */

#include "config.h"
#include <bango/bangobairo.h>
#include "btkintl.h"

#define BTK_TYPE_FILL_LAYOUT_RENDERER            (_btk_fill_layout_renderer_get_type())
#define BTK_FILL_LAYOUT_RENDERER(object)         (B_TYPE_CHECK_INSTANCE_CAST ((object), BTK_TYPE_FILL_LAYOUT_RENDERER, BtkFillLayoutRenderer))
#define BTK_IS_FILL_LAYOUT_RENDERER(object)      (B_TYPE_CHECK_INSTANCE_TYPE ((object), BTK_TYPE_FILL_LAYOUT_RENDERER))
#define BTK_FILL_LAYOUT_RENDERER_CLASS(klass)    (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_FILL_LAYOUT_RENDERER, BtkFillLayoutRendererClass))
#define BTK_IS_FILL_LAYOUT_RENDERER_CLASS(klass) (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_FILL_LAYOUT_RENDERER))
#define BTK_FILL_LAYOUT_RENDERER_GET_CLASS(obj)  (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_FILL_LAYOUT_RENDERER, BtkFillLayoutRendererClass))

typedef struct _BtkFillLayoutRenderer      BtkFillLayoutRenderer;
typedef struct _BtkFillLayoutRendererClass BtkFillLayoutRendererClass;

struct _BtkFillLayoutRenderer
{
  BangoRenderer parent_instance;

  bairo_t *cr;
};

struct _BtkFillLayoutRendererClass
{
  BangoRendererClass parent_class;
};

G_DEFINE_TYPE (BtkFillLayoutRenderer, _btk_fill_layout_renderer, BANGO_TYPE_RENDERER)

static void
btk_fill_layout_renderer_draw_glyphs (BangoRenderer     *renderer,
                                      BangoFont         *font,
                                      BangoGlyphString  *glyphs,
                                      int                x,
                                      int                y)
{
  BtkFillLayoutRenderer *text_renderer = BTK_FILL_LAYOUT_RENDERER (renderer);

  bairo_move_to (text_renderer->cr, (double)x / BANGO_SCALE, (double)y / BANGO_SCALE);
  bango_bairo_show_glyph_string (text_renderer->cr, font, glyphs);
}

static void
btk_fill_layout_renderer_draw_glyph_item (BangoRenderer     *renderer,
                                          const char        *text,
                                          BangoGlyphItem    *glyph_item,
                                          int                x,
                                          int                y)
{
  BtkFillLayoutRenderer *text_renderer = BTK_FILL_LAYOUT_RENDERER (renderer);

  bairo_move_to (text_renderer->cr, (double)x / BANGO_SCALE, (double)y / BANGO_SCALE);
  bango_bairo_show_glyph_item (text_renderer->cr, text, glyph_item);
}

static void
btk_fill_layout_renderer_draw_rectangle (BangoRenderer     *renderer,
                                         BangoRenderPart    part,
                                         int                x,
                                         int                y,
                                         int                width,
                                         int                height)
{
  BtkFillLayoutRenderer *text_renderer = BTK_FILL_LAYOUT_RENDERER (renderer);

  if (part == BANGO_RENDER_PART_BACKGROUND)
    return;

  bairo_rectangle (text_renderer->cr,
                   (double)x / BANGO_SCALE, (double)y / BANGO_SCALE,
		   (double)width / BANGO_SCALE, (double)height / BANGO_SCALE);
  bairo_fill (text_renderer->cr);
}

static void
btk_fill_layout_renderer_draw_trapezoid (BangoRenderer     *renderer,
                                         BangoRenderPart    part,
                                         double             y1_,
                                         double             x11,
                                         double             x21,
                                         double             y2,
                                         double             x12,
                                         double             x22)
{
  BtkFillLayoutRenderer *text_renderer = BTK_FILL_LAYOUT_RENDERER (renderer);
  bairo_matrix_t matrix;
  bairo_t *cr;

  cr = text_renderer->cr;

  bairo_save (cr);

  /* use identity scale, but keep translation */
  bairo_get_matrix (cr, &matrix);
  matrix.xx = matrix.yy = 1;
  matrix.xy = matrix.yx = 0;
  bairo_set_matrix (cr, &matrix);

  bairo_move_to (cr, x11, y1_);
  bairo_line_to (cr, x21, y1_);
  bairo_line_to (cr, x22, y2);
  bairo_line_to (cr, x12, y2);
  bairo_close_path (cr);

  bairo_fill (cr);

  bairo_restore (cr);
}

static void
btk_fill_layout_renderer_draw_error_underline (BangoRenderer *renderer,
                                               int            x,
                                               int            y,
                                               int            width,
                                               int            height)
{
  BtkFillLayoutRenderer *text_renderer = BTK_FILL_LAYOUT_RENDERER (renderer);

  bango_bairo_show_error_underline (text_renderer->cr,
                                    (double)x / BANGO_SCALE, (double)y / BANGO_SCALE,
                                    (double)width / BANGO_SCALE, (double)height / BANGO_SCALE);
}

static void
btk_fill_layout_renderer_draw_shape (BangoRenderer   *renderer,
                                     BangoAttrShape  *attr,
                                     int              x,
                                     int              y)
{
  BtkFillLayoutRenderer *text_renderer = BTK_FILL_LAYOUT_RENDERER (renderer);
  bairo_t *cr = text_renderer->cr;
  BangoLayout *layout;
  BangoBairoShapeRendererFunc shape_renderer;
  gpointer                    shape_renderer_data;

  layout = bango_renderer_get_layout (renderer);

  if (!layout)
  	return;

  shape_renderer = bango_bairo_context_get_shape_renderer (bango_layout_get_context (layout),
							   &shape_renderer_data);

  if (!shape_renderer)
    return;

  bairo_save (cr);

  bairo_move_to (cr, (double)x / BANGO_SCALE, (double)y / BANGO_SCALE);

  shape_renderer (cr, attr, FALSE, shape_renderer_data);

  bairo_restore (cr);
}

static void
btk_fill_layout_renderer_finalize (BObject *object)
{
  B_OBJECT_CLASS (_btk_fill_layout_renderer_parent_class)->finalize (object);
}

static void
_btk_fill_layout_renderer_init (BtkFillLayoutRenderer *renderer)
{
}

static void
_btk_fill_layout_renderer_class_init (BtkFillLayoutRendererClass *klass)
{
  BObjectClass *object_class = B_OBJECT_CLASS (klass);
  
  BangoRendererClass *renderer_class = BANGO_RENDERER_CLASS (klass);
  
  renderer_class->draw_glyphs = btk_fill_layout_renderer_draw_glyphs;
  renderer_class->draw_glyph_item = btk_fill_layout_renderer_draw_glyph_item;
  renderer_class->draw_rectangle = btk_fill_layout_renderer_draw_rectangle;
  renderer_class->draw_trapezoid = btk_fill_layout_renderer_draw_trapezoid;
  renderer_class->draw_error_underline = btk_fill_layout_renderer_draw_error_underline;
  renderer_class->draw_shape = btk_fill_layout_renderer_draw_shape;

  object_class->finalize = btk_fill_layout_renderer_finalize;
}

void
_btk_bango_fill_layout (bairo_t     *cr,
                        BangoLayout *layout)
{
  static BtkFillLayoutRenderer *renderer = NULL;
  gboolean has_current_point;
  double current_x, current_y;

  has_current_point = bairo_has_current_point (cr);
  bairo_get_current_point (cr, &current_x, &current_y);

  if (renderer == NULL)
    renderer = g_object_new (BTK_TYPE_FILL_LAYOUT_RENDERER, NULL);

  bairo_save (cr);
  bairo_translate (cr, current_x, current_y);

  renderer->cr = cr;
  bango_renderer_draw_layout (BANGO_RENDERER (renderer), layout, 0, 0);

  bairo_restore (cr);

  if (has_current_point)
    bairo_move_to (cr, current_x, current_y);
}

