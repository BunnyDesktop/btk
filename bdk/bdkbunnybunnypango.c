/* BDK - The GIMP Drawing Kit
 * Copyright (C) 2000 Red Hat, Inc. 
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include <math.h>
#include <bango/bangobairo.h>
#include "bdkbairo.h"
#include "bdkcolor.h"
#include "bdkgc.h"
#include "bdkinternals.h"
#include "bdkbango.h"
#include "bdkrgb.h"
#include "bdkprivate.h"
#include "bdkscreen.h"
#include "bdkintl.h"
#include "bdkalias.h"

#define BDK_INFO_KEY "bdk-info"

/* We have various arrays indexed by render part; if BangoRenderPart
 * is extended, we want to make sure not to overwrite the end of
 * those arrays.
 */
#define MAX_RENDER_PART  BANGO_RENDER_PART_STRIKETHROUGH

struct _BdkBangoRendererPrivate
{
  BdkScreen *screen;

  /* BdkBangoRenderer specific state */
  BangoColor override_color[MAX_RENDER_PART + 1];
  gboolean override_color_set[MAX_RENDER_PART + 1];
  
  BdkBitmap *stipple[MAX_RENDER_PART + 1];
  BangoColor emboss_color;
  gboolean embossed;

  bairo_t *cr;
  BangoRenderPart last_part;

  /* Current target */
  BdkDrawable *drawable;
  BdkGC *base_gc;

  gboolean gc_changed;
};

static BangoAttrType bdk_bango_attr_stipple_type;
static BangoAttrType bdk_bango_attr_embossed_type;
static BangoAttrType bdk_bango_attr_emboss_color_type;

enum {
  PROP_0,
  PROP_SCREEN
};

G_DEFINE_TYPE (BdkBangoRenderer, bdk_bango_renderer, BANGO_TYPE_RENDERER)

static void
bdk_bango_renderer_finalize (BObject *object)
{
  BdkBangoRenderer *bdk_renderer = BDK_BANGO_RENDERER (object);
  BdkBangoRendererPrivate *priv = bdk_renderer->priv;
  int i;

  if (priv->base_gc)
    g_object_unref (priv->base_gc);
  if (priv->drawable)
    g_object_unref (priv->drawable);

  for (i = 0; i <= MAX_RENDER_PART; i++)
    if (priv->stipple[i])
      g_object_unref (priv->stipple[i]);

  B_OBJECT_CLASS (bdk_bango_renderer_parent_class)->finalize (object);
}

static BObject*
bdk_bango_renderer_constructor (GType                  type,
				guint                  n_construct_properties,
				BObjectConstructParam *construct_params)
{
  BObject *object;
  BdkBangoRenderer *bdk_renderer;

  object = B_OBJECT_CLASS (bdk_bango_renderer_parent_class)->constructor (type,
                                                                          n_construct_properties,
                                                                          construct_params);

  bdk_renderer = BDK_BANGO_RENDERER (object);
  
  if (!bdk_renderer->priv->screen)
    {
      g_warning ("Screen must be specified at construct time for BdkBangoRenderer");
      bdk_renderer->priv->screen = bdk_screen_get_default ();
    }

  return object;
}

/* Adjusts matrix and color for the renderer to draw the secondary
 * "shadow" copy for embossed text */
static void
emboss_context (BdkBangoRenderer *renderer, bairo_t *cr)
{
  BdkBangoRendererPrivate *priv = renderer->priv;
  bairo_matrix_t tmp_matrix;
  double red, green, blue;

  /* The gymnastics here to adjust the matrix are because we want
   * to offset by +1,+1 in device-space, not in user-space,
   * so we can't just draw the layout at x + 1, y + 1
   */
  bairo_get_matrix (cr, &tmp_matrix);
  tmp_matrix.x0 += 1.0;
  tmp_matrix.y0 += 1.0;
  bairo_set_matrix (cr, &tmp_matrix);

  red = (double) priv->emboss_color.red / 65535.;
  green = (double) priv->emboss_color.green / 65535.;
  blue = (double) priv->emboss_color.blue / 65535.;

  bairo_set_source_rgb (cr, red, green, blue);
}

static inline gboolean
color_equal (const BangoColor *c1, const BangoColor *c2)
{
  if (!c1 && !c2)
    return TRUE;

  if (c1 && c2 &&
      c1->red == c2->red &&
      c1->green == c2->green &&
      c1->blue == c2->blue)
    return TRUE;

  return FALSE;
}

static bairo_t *
get_bairo_context (BdkBangoRenderer *bdk_renderer,
		   BangoRenderPart   part)
{
  BangoRenderer *renderer = BANGO_RENDERER (bdk_renderer);
  BdkBangoRendererPrivate *priv = bdk_renderer->priv;

  if (!priv->cr)
    {
      const BangoMatrix *matrix;
      
      priv->cr = bdk_bairo_create (priv->drawable);

      matrix = bango_renderer_get_matrix (renderer);
      if (matrix)
	{
	  bairo_matrix_t bairo_matrix;
	  
	  bairo_matrix_init (&bairo_matrix,
			     matrix->xx, matrix->yx,
			     matrix->xy, matrix->yy,
			     matrix->x0, matrix->y0);
	  bairo_set_matrix (priv->cr, &bairo_matrix);
	}
    }

  if (part != priv->last_part)
    {
      BangoColor *bango_color;
      BdkColor *color;
      BdkColor tmp_color;
      gboolean changed;

      bango_color = bango_renderer_get_color (renderer, part);
      
      if (priv->last_part != -1)
	changed = priv->gc_changed ||
	  priv->stipple[priv->last_part] != priv->stipple[part] ||
	  !color_equal (bango_color,
			bango_renderer_get_color (renderer, priv->last_part));
      else
	changed = TRUE;
      
      if (changed)
	{
	  if (bango_color)
	    {
	      tmp_color.red = bango_color->red;
	      tmp_color.green = bango_color->green;
	      tmp_color.blue = bango_color->blue;
	      
	      color = &tmp_color;
	    }
	  else
	    color = NULL;

	  _bdk_gc_update_context (priv->base_gc,
				  priv->cr,
				  color,
				  priv->stipple[part],
				  priv->gc_changed,
				  priv->drawable);
	}

      priv->last_part = part;
      priv->gc_changed = FALSE;
    }

  return priv->cr;
}

static void
bdk_bango_renderer_draw_glyphs (BangoRenderer    *renderer,
				BangoFont        *font,
				BangoGlyphString *glyphs,
				int               x,
				int               y)
{
  BdkBangoRenderer *bdk_renderer = BDK_BANGO_RENDERER (renderer);
  BdkBangoRendererPrivate *priv = bdk_renderer->priv;
  bairo_t *cr;

  cr = get_bairo_context (bdk_renderer, 
			  BANGO_RENDER_PART_FOREGROUND);

  if (priv->embossed)
    {
      bairo_save (cr);
      emboss_context (bdk_renderer, cr);
      bairo_move_to (cr, (double)x / BANGO_SCALE, (double)y / BANGO_SCALE);
      bango_bairo_show_glyph_string (cr, font, glyphs);
      bairo_restore (cr);
    }

  bairo_move_to (cr, (double)x / BANGO_SCALE, (double)y / BANGO_SCALE);
  bango_bairo_show_glyph_string (cr, font, glyphs);
}

static void
bdk_bango_renderer_draw_rectangle (BangoRenderer    *renderer,
				   BangoRenderPart   part,
				   int               x,
				   int               y,
				   int               width,
				   int               height)
{
  BdkBangoRenderer *bdk_renderer = BDK_BANGO_RENDERER (renderer);
  BdkBangoRendererPrivate *priv = bdk_renderer->priv;
  bairo_t *cr;
  
  cr = get_bairo_context (bdk_renderer, part);

  if (priv->embossed && part != BANGO_RENDER_PART_BACKGROUND)
    {
      bairo_save (cr);
      emboss_context (bdk_renderer, cr);
      bairo_rectangle (cr,
		       (double)x / BANGO_SCALE, (double)y / BANGO_SCALE,
		       (double)width / BANGO_SCALE, (double)height / BANGO_SCALE);

      bairo_fill (cr);
      bairo_restore (cr);
    }

  bairo_rectangle (cr,
		   (double)x / BANGO_SCALE, (double)y / BANGO_SCALE,
		   (double)width / BANGO_SCALE, (double)height / BANGO_SCALE);
  bairo_fill (cr);
}

static void
bdk_bango_renderer_draw_error_underline (BangoRenderer    *renderer,
					 int               x,
					 int               y,
					 int               width,
					 int               height)
{
  BdkBangoRenderer *bdk_renderer = BDK_BANGO_RENDERER (renderer);
  BdkBangoRendererPrivate *priv = bdk_renderer->priv;
  bairo_t *cr;
  
  cr = get_bairo_context (bdk_renderer, BANGO_RENDER_PART_UNDERLINE);
  
  if (priv->embossed)
    {
      bairo_save (cr);
      emboss_context (bdk_renderer, cr);
      bango_bairo_show_error_underline (cr,
            (double)x / BANGO_SCALE, (double)y / BANGO_SCALE,
            (double)width / BANGO_SCALE, (double)height / BANGO_SCALE);
      bairo_restore (cr);
    }

  bango_bairo_show_error_underline (cr,
	(double)x / BANGO_SCALE, (double)y / BANGO_SCALE,
	(double)width / BANGO_SCALE, (double)height / BANGO_SCALE);
}

static void
bdk_bango_renderer_draw_shape (BangoRenderer  *renderer,
			       BangoAttrShape *attr,
			       int             x,
			       int             y)
{
  BdkBangoRenderer *bdk_renderer = BDK_BANGO_RENDERER (renderer);
  BdkBangoRendererPrivate *priv = bdk_renderer->priv;
  BangoLayout *layout;
  BangoBairoShapeRendererFunc shape_renderer;
  gpointer                    shape_renderer_data;
  bairo_t *cr;
  double dx = (double)x / BANGO_SCALE, dy = (double)y / BANGO_SCALE;

  layout = bango_renderer_get_layout (renderer);

  if (!layout)
  	return;

  shape_renderer = bango_bairo_context_get_shape_renderer (bango_layout_get_context (layout),
							   &shape_renderer_data);

  if (!shape_renderer)
    return;

  cr = get_bairo_context (bdk_renderer, BANGO_RENDER_PART_FOREGROUND);
  
  bairo_save (cr);

  if (priv->embossed)
    {
      bairo_save (cr);
      emboss_context (bdk_renderer, cr);

      bairo_move_to (cr, dx, dy);
      shape_renderer (cr, attr, FALSE, shape_renderer_data);

      bairo_restore (cr);
    }

  bairo_move_to (cr, dx, dy);
  shape_renderer (cr, attr, FALSE, shape_renderer_data);

  bairo_restore (cr);
}

static void
bdk_bango_renderer_part_changed (BangoRenderer   *renderer,
				 BangoRenderPart  part)
{
  BdkBangoRenderer *bdk_renderer = BDK_BANGO_RENDERER (renderer);

  if (bdk_renderer->priv->last_part == part)
    bdk_renderer->priv->last_part = (BangoRenderPart)-1;
}

static void
bdk_bango_renderer_begin (BangoRenderer *renderer)
{
  BdkBangoRenderer *bdk_renderer = BDK_BANGO_RENDERER (renderer);
  BdkBangoRendererPrivate *priv = bdk_renderer->priv;
  
  if (!priv->drawable || !priv->base_gc)
    {
      g_warning ("bdk_bango_renderer_set_drawable() and bdk_bango_renderer_set_drawable()"
		 "must be used to set the target drawable and GC before using the renderer\n");
    }
}

static void
bdk_bango_renderer_end (BangoRenderer *renderer)
{
  BdkBangoRenderer *bdk_renderer = BDK_BANGO_RENDERER (renderer);
  BdkBangoRendererPrivate *priv = bdk_renderer->priv;

  if (priv->cr)
    {
      bairo_destroy (priv->cr);
      priv->cr = NULL;
    }
  priv->last_part = (BangoRenderPart)-1;
}

static void
bdk_bango_renderer_prepare_run (BangoRenderer  *renderer,
				BangoLayoutRun *run)
{
  BdkBangoRenderer *bdk_renderer = BDK_BANGO_RENDERER (renderer);
  gboolean embossed = FALSE;
  BdkBitmap *stipple = NULL;
  gboolean changed = FALSE;
  BangoColor emboss_color;
  GSList *l;
  int i;

  emboss_color.red = 0xffff;
  emboss_color.green = 0xffff;
  emboss_color.blue = 0xffff;

  for (l = run->item->analysis.extra_attrs; l; l = l->next)
    {
      BangoAttribute *attr = l->data;

      /* stipple_type and embossed_type aren't necessarily
       * initialized, but they are 0, which is an
       * invalid type so won't occur. 
       */
      if (attr->klass->type == bdk_bango_attr_stipple_type)
	{
	  stipple = ((BdkBangoAttrStipple*)attr)->stipple;
	}
      else if (attr->klass->type == bdk_bango_attr_embossed_type)
	{
	  embossed = ((BdkBangoAttrEmbossed*)attr)->embossed;
	}
      else if (attr->klass->type == bdk_bango_attr_emboss_color_type)
	{
	  emboss_color = ((BdkBangoAttrEmbossColor*)attr)->color;
	}
    }

  bdk_bango_renderer_set_stipple (bdk_renderer, BANGO_RENDER_PART_FOREGROUND, stipple);
  bdk_bango_renderer_set_stipple (bdk_renderer, BANGO_RENDER_PART_BACKGROUND, stipple);
  bdk_bango_renderer_set_stipple (bdk_renderer, BANGO_RENDER_PART_UNDERLINE, stipple);
  bdk_bango_renderer_set_stipple (bdk_renderer, BANGO_RENDER_PART_STRIKETHROUGH, stipple);

  if (embossed != bdk_renderer->priv->embossed)
    {
      bdk_renderer->priv->embossed = embossed;
      changed = TRUE;
    }

  if (!color_equal (&bdk_renderer->priv->emboss_color, &emboss_color))
    {
      bdk_renderer->priv->emboss_color = emboss_color;
      changed = TRUE;
    }

  if (changed)
    bango_renderer_part_changed (renderer, BANGO_RENDER_PART_FOREGROUND);

  BANGO_RENDERER_CLASS (bdk_bango_renderer_parent_class)->prepare_run (renderer, run);

  for (i = 0; i <= MAX_RENDER_PART; i++)
    {
      if (bdk_renderer->priv->override_color_set[i])
	bango_renderer_set_color (renderer, i, &bdk_renderer->priv->override_color[i]);
    }
}

static void
bdk_bango_renderer_set_property (BObject         *object,
				 guint            prop_id,
				 const BValue    *value,
				 BParamSpec      *pspec)
{
  BdkBangoRenderer *bdk_renderer = BDK_BANGO_RENDERER (object);

  switch (prop_id)
    {
    case PROP_SCREEN:
      bdk_renderer->priv->screen = b_value_get_object (value);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bdk_bango_renderer_get_property (BObject    *object,
				 guint       prop_id,
				 BValue     *value,
				 BParamSpec *pspec)
{
  BdkBangoRenderer *bdk_renderer = BDK_BANGO_RENDERER (object);

  switch (prop_id)
    {
    case PROP_SCREEN:
      b_value_set_object (value, bdk_renderer->priv->screen);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bdk_bango_renderer_init (BdkBangoRenderer *renderer)
{
  renderer->priv = B_TYPE_INSTANCE_GET_PRIVATE (renderer,
						BDK_TYPE_BANGO_RENDERER,
						BdkBangoRendererPrivate);

  renderer->priv->last_part = (BangoRenderPart)-1;
  renderer->priv->gc_changed = TRUE;
}

static void
bdk_bango_renderer_class_init (BdkBangoRendererClass *klass)
{
  BObjectClass *object_class = B_OBJECT_CLASS (klass);
  
  BangoRendererClass *renderer_class = BANGO_RENDERER_CLASS (klass);
  
  renderer_class->draw_glyphs = bdk_bango_renderer_draw_glyphs;
  renderer_class->draw_rectangle = bdk_bango_renderer_draw_rectangle;
  renderer_class->draw_error_underline = bdk_bango_renderer_draw_error_underline;
  renderer_class->draw_shape = bdk_bango_renderer_draw_shape;
  renderer_class->part_changed = bdk_bango_renderer_part_changed;
  renderer_class->begin = bdk_bango_renderer_begin;
  renderer_class->end = bdk_bango_renderer_end;
  renderer_class->prepare_run = bdk_bango_renderer_prepare_run;

  object_class->finalize = bdk_bango_renderer_finalize;
  object_class->constructor = bdk_bango_renderer_constructor;
  object_class->set_property = bdk_bango_renderer_set_property;
  object_class->get_property = bdk_bango_renderer_get_property;
  
  g_object_class_install_property (object_class,
                                   PROP_SCREEN,
                                   g_param_spec_object ("screen",
                                                        P_("Screen"),
                                                        P_("the BdkScreen for the renderer"),
                                                        BDK_TYPE_SCREEN,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
							G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | 
							G_PARAM_STATIC_BLURB));

  g_type_class_add_private (object_class, sizeof (BdkBangoRendererPrivate));  
}

/**
 * bdk_bango_renderer_new:
 * @screen: a #BdkScreen
 * 
 * Creates a new #BangoRenderer for @screen. Normally you can use the
 * results of bdk_bango_renderer_get_default() rather than creating a new
 * renderer.
 * 
 * Return value: a newly created #BangoRenderer. Free with g_object_unref().
 *
 * Since: 2.6
 **/
BangoRenderer *
bdk_bango_renderer_new (BdkScreen *screen)
{
  g_return_val_if_fail (screen != NULL, NULL);
  
  return g_object_new (BDK_TYPE_BANGO_RENDERER,
		       "screen", screen,
		       NULL);
}

static void
on_renderer_display_closed (BdkDisplay       *display,
                            gboolean          is_error,
			    BdkBangoRenderer *renderer)
{
  g_signal_handlers_disconnect_by_func (display,
					on_renderer_display_closed,
					renderer);
  g_object_set_data (B_OBJECT (renderer->priv->screen),
                     g_intern_static_string ("bdk-bango-renderer"), NULL);
}

/**
 * bdk_bango_renderer_get_default:
 * @screen: a #BdkScreen
 * 
 * Gets the default #BangoRenderer for a screen. This default renderer
 * is shared by all users of the display, so properties such as the color
 * or transformation matrix set for the renderer may be overwritten
 * by functions such as bdk_draw_layout().
 *
 * Before using the renderer, you need to call bdk_bango_renderer_set_drawable()
 * and bdk_bango_renderer_set_gc() to set the drawable and graphics context
 * to use for drawing.
 * 
 * Return value: the default #BangoRenderer for @screen. The
 *  renderer is owned by BTK+ and will be kept around until the
 *  screen is closed.
 *
 * Since: 2.6
 **/
BangoRenderer *
bdk_bango_renderer_get_default (BdkScreen *screen)
{
  BangoRenderer *renderer;

  g_return_val_if_fail (BDK_IS_SCREEN (screen), NULL);
  
  renderer = g_object_get_data (B_OBJECT (screen), "bdk-bango-renderer");
  if (!renderer)
    {
      renderer = bdk_bango_renderer_new (screen);
      g_object_set_data_full (B_OBJECT (screen), 
                              g_intern_static_string ("bdk-bango-renderer"), renderer,
			      (GDestroyNotify)g_object_unref);

      g_signal_connect (bdk_screen_get_display (screen), "closed",
			G_CALLBACK (on_renderer_display_closed), renderer);
    }

  return renderer;
}

/**
 * bdk_bango_renderer_set_drawable:
 * @bdk_renderer: a #BdkBangoRenderer
 * @drawable: (allow-none): the new target drawable, or %NULL
 * 
 * Sets the drawable the renderer draws to.
 *
 * Since: 2.6
 **/
void
bdk_bango_renderer_set_drawable (BdkBangoRenderer *bdk_renderer,
				 BdkDrawable      *drawable)
{
  BdkBangoRendererPrivate *priv;
  
  g_return_if_fail (BDK_IS_BANGO_RENDERER (bdk_renderer));
  g_return_if_fail (drawable == NULL || BDK_IS_DRAWABLE (drawable));

  priv = bdk_renderer->priv;
  
  if (priv->drawable != drawable)
    {
      if (priv->drawable)
	g_object_unref (priv->drawable);
      priv->drawable = drawable;
      if (priv->drawable)
	g_object_ref (priv->drawable);
    }
}

/**
 * bdk_bango_renderer_set_gc:
 * @bdk_renderer: a #BdkBangoRenderer
 * @gc: (allow-none): the new GC to use for drawing, or %NULL
 * 
 * Sets the GC the renderer draws with. Note that the GC must not be
 * modified until it is unset by calling the function again with
 * %NULL for the @gc parameter, since BDK may make internal copies
 * of the GC which won't be updated to follow changes to the
 * original GC.
 *
 * Since: 2.6
 **/
void
bdk_bango_renderer_set_gc (BdkBangoRenderer *bdk_renderer,
			   BdkGC            *gc)
{
  BdkBangoRendererPrivate *priv;
  
  g_return_if_fail (BDK_IS_BANGO_RENDERER (bdk_renderer));
  g_return_if_fail (gc == NULL || BDK_IS_GC (gc));

  priv = bdk_renderer->priv;
  
  if (priv->base_gc != gc)
    {
      if (priv->base_gc)
	g_object_unref (priv->base_gc);
      priv->base_gc = gc;
      if (priv->base_gc)
	g_object_ref (priv->base_gc);

      priv->gc_changed = TRUE;
    }
}


/**
 * bdk_bango_renderer_set_stipple:
 * @bdk_renderer: a #BdkBangoRenderer
 * @part: the part to render with the stipple
 * @stipple: the new stipple value.
 * 
 * Sets the stipple for one render part (foreground, background, underline,
 * etc.) Note that this is overwritten when iterating through the individual
 * styled runs of a #BangoLayout or #BangoLayoutLine. This function is thus
 * only useful when you call low level functions like bango_renderer_draw_glyphs()
 * directly, or in the 'prepare_run' virtual function of a subclass of
 * #BdkBangoRenderer.
 *
 * Since: 2.6
 **/
void
bdk_bango_renderer_set_stipple (BdkBangoRenderer *bdk_renderer,
				BangoRenderPart   part,
				BdkBitmap        *stipple)
{
  g_return_if_fail (BDK_IS_BANGO_RENDERER (bdk_renderer));

  if (part > MAX_RENDER_PART)	/* Silently ignore unknown parts */
    return;

  if (stipple != bdk_renderer->priv->stipple[part])
    {
      if (bdk_renderer->priv->stipple[part])
	g_object_unref (bdk_renderer->priv->stipple[part]);

      bdk_renderer->priv->stipple[part] = stipple;
      
      if (bdk_renderer->priv->stipple[part])
	g_object_ref (bdk_renderer->priv->stipple[part]);

      bango_renderer_part_changed (BANGO_RENDERER (bdk_renderer), part);
    }
}

/**
 * bdk_bango_renderer_set_override_color:
 * @bdk_renderer: a #BdkBangoRenderer
 * @part: the part to render to set the color of
 * @color: (allow-none): the color to use, or %NULL to unset a previously
 *         set override color.
 * 
 * Sets the color for a particular render part (foreground,
 * background, underline, etc.), overriding any attributes on the layouts
 * renderered with this renderer.
 * 
 * Since: 2.6
 **/
void
bdk_bango_renderer_set_override_color (BdkBangoRenderer *bdk_renderer,
				       BangoRenderPart   part,
				       const BdkColor   *color)
{
  BdkBangoRendererPrivate *priv;
  
  g_return_if_fail (BDK_IS_BANGO_RENDERER (bdk_renderer));

  priv = bdk_renderer->priv;
  
  if (part > MAX_RENDER_PART)	/* Silently ignore unknown parts */
    return;

  if (color)
    {
      priv->override_color[part].red = color->red;
      priv->override_color[part].green = color->green;
      priv->override_color[part].blue = color->blue;
      priv->override_color_set[part] = TRUE;
    }
  else
    priv->override_color_set[part] = FALSE;
}

/**
 * bdk_bango_context_set_colormap:
 * @context: a #BangoContext
 * @colormap: a #BdkColormap
 *
 * This function used to set the colormap to be used for drawing with
 * @context. The colormap is now always derived from the graphics
 * context used for drawing, so calling this function is no longer
 * necessary.
 **/
void
bdk_bango_context_set_colormap (BangoContext *context,
				BdkColormap  *colormap)
{
  g_return_if_fail (BANGO_IS_CONTEXT (context));
  g_return_if_fail (colormap == NULL || BDK_IS_COLORMAP (colormap));
}

/* Gets a renderer to draw with, setting the properties of the
 * renderer and activating it. Note that since we activate the
 * renderer here, the implicit setting of the matrix that
 * bango_renderer_draw_layout_[line] normally do when they
 * activate the renderer is suppressed. */
static BangoRenderer *
get_renderer (BdkDrawable     *drawable,
	      BdkGC           *gc,
	      const BdkColor  *foreground,
	      const BdkColor  *background)
{
  BdkScreen *screen = bdk_drawable_get_screen (drawable);
  BangoRenderer *renderer = bdk_bango_renderer_get_default (screen);
  BdkBangoRenderer *bdk_renderer = BDK_BANGO_RENDERER (renderer);

  bdk_bango_renderer_set_drawable (bdk_renderer, drawable);
  bdk_bango_renderer_set_gc (bdk_renderer, gc);  

  bdk_bango_renderer_set_override_color (bdk_renderer,
					 BANGO_RENDER_PART_FOREGROUND,
					 foreground);
  bdk_bango_renderer_set_override_color (bdk_renderer,
					 BANGO_RENDER_PART_UNDERLINE,
					 foreground);
  bdk_bango_renderer_set_override_color (bdk_renderer,
					 BANGO_RENDER_PART_STRIKETHROUGH,
					 foreground);

  bdk_bango_renderer_set_override_color (bdk_renderer,
					 BANGO_RENDER_PART_BACKGROUND,
					 background);

  bango_renderer_activate (renderer);

  return renderer;
}

/* Cleans up the renderer obtained with get_renderer() */
static void
release_renderer (BangoRenderer *renderer)
{
  BdkBangoRenderer *bdk_renderer = BDK_BANGO_RENDERER (renderer);
  
  bango_renderer_deactivate (renderer);
  
  bdk_bango_renderer_set_override_color (bdk_renderer,
					 BANGO_RENDER_PART_FOREGROUND,
					 NULL);
  bdk_bango_renderer_set_override_color (bdk_renderer,
					 BANGO_RENDER_PART_UNDERLINE,
					 NULL);
  bdk_bango_renderer_set_override_color (bdk_renderer,
					 BANGO_RENDER_PART_STRIKETHROUGH,
					 NULL);
  bdk_bango_renderer_set_override_color (bdk_renderer,
					 BANGO_RENDER_PART_BACKGROUND,
					 NULL);
  
  bdk_bango_renderer_set_drawable (bdk_renderer, NULL);
  bdk_bango_renderer_set_gc (bdk_renderer, NULL);
}

/**
 * bdk_draw_layout_line_with_colors:
 * @drawable:  the drawable on which to draw the line
 * @gc:        base graphics to use
 * @x:         the x position of start of string (in pixels)
 * @y:         the y position of baseline (in pixels)
 * @line:      a #BangoLayoutLine
 * @foreground: (allow-none): foreground override color, or %NULL for none
 * @background: (allow-none): background override color, or %NULL for none
 *
 * Render a #BangoLayoutLine onto a #BdkDrawable, overriding the
 * layout's normal colors with @foreground and/or @background.
 * @foreground and @background need not be allocated.
 *
 * If the layout's #BangoContext has a transformation matrix set, then
 * @x and @y specify the position of the left edge of the baseline
 * (left is in before-tranform user coordinates) in after-transform
 * device coordinates.
 */
void 
bdk_draw_layout_line_with_colors (BdkDrawable      *drawable,
                                  BdkGC            *gc,
                                  gint              x, 
                                  gint              y,
                                  BangoLayoutLine  *line,
                                  const BdkColor   *foreground,
                                  const BdkColor   *background)
{
  BangoRenderer *renderer;
  const BangoMatrix *matrix;
  
  g_return_if_fail (BDK_IS_DRAWABLE (drawable));
  g_return_if_fail (BDK_IS_GC (gc));
  g_return_if_fail (line != NULL);

  renderer = get_renderer (drawable, gc, foreground, background);

  /* When we have a matrix, we do positioning by adjusting the matrix, and
   * clamp just pass x=0, y=0 to the lower levels. We don't want to introduce
   * a matrix when the caller didn't provide one, however, since that adds
   * lots of floating point arithmetic for each glyph.
   */
  matrix = bango_context_get_matrix (bango_layout_get_context (line->layout));
  if (matrix)
    {
      BangoMatrix tmp_matrix;
      
      tmp_matrix = *matrix;
      tmp_matrix.x0 += x;
      tmp_matrix.y0 += y;
      bango_renderer_set_matrix (renderer, &tmp_matrix);

      x = 0;
      y = 0;
    }
  /* Fall back to introduce a matrix if the coords would scale out of range.
   * The x and y here will be added to in-layout coordinates.  So we cannot
   * support the entire range here safely.  So, we just accept the middle half
   * and use fallback for the rest. */
  else if (BDK_BANGO_UNITS_OVERFLOWS (x, y))
    {
      BangoMatrix tmp_matrix = BANGO_MATRIX_INIT;
      tmp_matrix.x0 += x;
      tmp_matrix.y0 += y;
      bango_renderer_set_matrix (renderer, &tmp_matrix);

      x = 0;
      y = 0;
    }
  else
    bango_renderer_set_matrix (renderer, NULL);

  bango_renderer_draw_layout_line (renderer, line, x * BANGO_SCALE, y * BANGO_SCALE);

  release_renderer (renderer);
}

/**
 * bdk_draw_layout_with_colors:
 * @drawable:  the drawable on which to draw string
 * @gc:        base graphics context to use
 * @x:         the X position of the left of the layout (in pixels)
 * @y:         the Y position of the top of the layout (in pixels)
 * @layout:    a #BangoLayout
 * @foreground: (allow-none): foreground override color, or %NULL for none
 * @background: (allow-none): background override color, or %NULL for none
 *
 * Render a #BangoLayout onto a #BdkDrawable, overriding the
 * layout's normal colors with @foreground and/or @background.
 * @foreground and @background need not be allocated.
 *
 * If the layout's #BangoContext has a transformation matrix set, then
 * @x and @y specify the position of the top left corner of the
 * bounding box (in device space) of the transformed layout.
 *
 * If you're using BTK+, the ususal way to obtain a #BangoLayout
 * is btk_widget_create_bango_layout().
 */
void 
bdk_draw_layout_with_colors (BdkDrawable     *drawable,
                             BdkGC           *gc,
                             int              x, 
                             int              y,
                             BangoLayout     *layout,
                             const BdkColor  *foreground,
                             const BdkColor  *background)
{
  BangoRenderer *renderer;
  const BangoMatrix *matrix;
  
  g_return_if_fail (BDK_IS_DRAWABLE (drawable));
  g_return_if_fail (BDK_IS_GC (gc));
  g_return_if_fail (BANGO_IS_LAYOUT (layout));

  renderer = get_renderer (drawable, gc, foreground, background);

  /* When we have a matrix, we do positioning by adjusting the matrix, and
   * clamp just pass x=0, y=0 to the lower levels. We don't want to introduce
   * a matrix when the caller didn't provide one, however, since that adds
   * lots of floating point arithmetic for each glyph.
   */
  matrix = bango_context_get_matrix (bango_layout_get_context (layout));
  if (matrix)
    {
      BangoMatrix tmp_matrix;
      BangoRectangle rect;

      bango_layout_get_extents (layout, NULL, &rect);
      bango_matrix_transform_rectangle (matrix, &rect);
      bango_extents_to_pixels (&rect, NULL);
      
      tmp_matrix = *matrix;
      tmp_matrix.x0 += x - rect.x;
      tmp_matrix.y0 += y - rect.y;
      bango_renderer_set_matrix (renderer, &tmp_matrix);
      
      x = 0;
      y = 0;
    }
  else if (BDK_BANGO_UNITS_OVERFLOWS (x, y))
    {
      BangoMatrix tmp_matrix = BANGO_MATRIX_INIT;
      tmp_matrix.x0 = x;
      tmp_matrix.y0 = y;
      bango_renderer_set_matrix (renderer, &tmp_matrix);

      x = 0;
      y = 0;
    }
  else
    bango_renderer_set_matrix (renderer, NULL);

  bango_renderer_draw_layout (renderer, layout, x * BANGO_SCALE, y * BANGO_SCALE);
  
  release_renderer (renderer);
}

/**
 * bdk_draw_layout_line:
 * @drawable:  the drawable on which to draw the line
 * @gc:        base graphics to use
 * @x:         the x position of start of string (in pixels)
 * @y:         the y position of baseline (in pixels)
 * @line:      a #BangoLayoutLine
 *
 * Render a #BangoLayoutLine onto an BDK drawable
 *
 * If the layout's #BangoContext has a transformation matrix set, then
 * @x and @y specify the position of the left edge of the baseline
 * (left is in before-tranform user coordinates) in after-transform
 * device coordinates.
 */
void 
bdk_draw_layout_line (BdkDrawable      *drawable,
		      BdkGC            *gc,
		      gint              x, 
		      gint              y,
		      BangoLayoutLine  *line)
{
  g_return_if_fail (BDK_IS_DRAWABLE (drawable));
  g_return_if_fail (BDK_IS_GC (gc));
  g_return_if_fail (line != NULL);
  
  bdk_draw_layout_line_with_colors (drawable, gc, x, y, line, NULL, NULL);
}

/**
 * bdk_draw_layout:
 * @drawable:  the drawable on which to draw string
 * @gc:        base graphics context to use
 * @x:         the X position of the left of the layout (in pixels)
 * @y:         the Y position of the top of the layout (in pixels)
 * @layout:    a #BangoLayout
 *
 * Render a #BangoLayout onto a BDK drawable
 *
 * If the layout's #BangoContext has a transformation matrix set, then
 * @x and @y specify the position of the top left corner of the
 * bounding box (in device space) of the transformed layout.
 *
 * If you're using BTK+, the usual way to obtain a #BangoLayout
 * is btk_widget_create_bango_layout().
 */
void 
bdk_draw_layout (BdkDrawable     *drawable,
		 BdkGC           *gc,
		 int              x, 
		 int              y,
		 BangoLayout     *layout)
{
  g_return_if_fail (BDK_IS_DRAWABLE (drawable));
  g_return_if_fail (BDK_IS_GC (gc));
  g_return_if_fail (BANGO_IS_LAYOUT (layout));

  bdk_draw_layout_with_colors (drawable, gc, x, y, layout, NULL, NULL);
}

/* BdkBangoAttrStipple */

static BangoAttribute *
bdk_bango_attr_stipple_copy (const BangoAttribute *attr)
{
  const BdkBangoAttrStipple *src = (const BdkBangoAttrStipple*) attr;

  return bdk_bango_attr_stipple_new (src->stipple);
}

static void
bdk_bango_attr_stipple_destroy (BangoAttribute *attr)
{
  BdkBangoAttrStipple *st = (BdkBangoAttrStipple*) attr;

  if (st->stipple)
    g_object_unref (st->stipple);
  
  g_free (attr);
}

static gboolean
bdk_bango_attr_stipple_compare (const BangoAttribute *attr1,
                                    const BangoAttribute *attr2)
{
  const BdkBangoAttrStipple *a = (const BdkBangoAttrStipple*) attr1;
  const BdkBangoAttrStipple *b = (const BdkBangoAttrStipple*) attr2;

  return a->stipple == b->stipple;
}

/**
 * bdk_bango_attr_stipple_new:
 * @stipple: a bitmap to be set as stipple
 *
 * Creates a new attribute containing a stipple bitmap to be used when
 * rendering the text.
 *
 * Return value: new #BangoAttribute
 **/

BangoAttribute *
bdk_bango_attr_stipple_new (BdkBitmap *stipple)
{
  BdkBangoAttrStipple *result;
  
  static BangoAttrClass klass = {
    0,
    bdk_bango_attr_stipple_copy,
    bdk_bango_attr_stipple_destroy,
    bdk_bango_attr_stipple_compare
  };

  if (!klass.type)
    klass.type = bdk_bango_attr_stipple_type =
      bango_attr_type_register ("BdkBangoAttrStipple");

  result = g_new (BdkBangoAttrStipple, 1);
  result->attr.klass = &klass;

  if (stipple)
    g_object_ref (stipple);
  
  result->stipple = stipple;

  return (BangoAttribute *)result;
}

/* BdkBangoAttrEmbossed */

static BangoAttribute *
bdk_bango_attr_embossed_copy (const BangoAttribute *attr)
{
  const BdkBangoAttrEmbossed *e = (const BdkBangoAttrEmbossed*) attr;

  return bdk_bango_attr_embossed_new (e->embossed);
}

static void
bdk_bango_attr_embossed_destroy (BangoAttribute *attr)
{
  g_free (attr);
}

static gboolean
bdk_bango_attr_embossed_compare (const BangoAttribute *attr1,
                                 const BangoAttribute *attr2)
{
  const BdkBangoAttrEmbossed *e1 = (const BdkBangoAttrEmbossed*) attr1;
  const BdkBangoAttrEmbossed *e2 = (const BdkBangoAttrEmbossed*) attr2;

  return e1->embossed == e2->embossed;
}

/**
 * bdk_bango_attr_embossed_new:
 * @embossed: if the rebunnyion should be embossed
 *
 * Creates a new attribute flagging a rebunnyion as embossed or not.
 *
 * Return value: new #BangoAttribute
 **/

BangoAttribute *
bdk_bango_attr_embossed_new (gboolean embossed)
{
  BdkBangoAttrEmbossed *result;
  
  static BangoAttrClass klass = {
    0,
    bdk_bango_attr_embossed_copy,
    bdk_bango_attr_embossed_destroy,
    bdk_bango_attr_embossed_compare
  };

  if (!klass.type)
    klass.type = bdk_bango_attr_embossed_type =
      bango_attr_type_register ("BdkBangoAttrEmbossed");

  result = g_new (BdkBangoAttrEmbossed, 1);
  result->attr.klass = &klass;
  result->embossed = embossed;
  
  return (BangoAttribute *)result;
}

/* BdkBangoAttrEmbossColor */

static BangoAttribute *
bdk_bango_attr_emboss_color_copy (const BangoAttribute *attr)
{
  const BdkBangoAttrEmbossColor *old = (const BdkBangoAttrEmbossColor*) attr;
  BdkBangoAttrEmbossColor *copy;

  copy = g_new (BdkBangoAttrEmbossColor, 1);
  copy->attr.klass = old->attr.klass;
  copy->color = old->color;

  return (BangoAttribute *) copy;
}

static void
bdk_bango_attr_emboss_color_destroy (BangoAttribute *attr)
{
  g_free (attr);
}

static gboolean
bdk_bango_attr_emboss_color_compare (const BangoAttribute *attr1,
                                     const BangoAttribute *attr2)
{
  const BdkBangoAttrEmbossColor *c1 = (const BdkBangoAttrEmbossColor*) attr1;
  const BdkBangoAttrEmbossColor *c2 = (const BdkBangoAttrEmbossColor*) attr2;

  return color_equal (&c1->color, &c2->color);
}

/**
 * bdk_bango_attr_emboss_color_new:
 * @color: a BdkColor representing the color to emboss with
 *
 * Creates a new attribute specifying the color to emboss text with.
 *
 * Return value: new #BangoAttribute
 *
 * Since: 2.12
 **/
BangoAttribute *
bdk_bango_attr_emboss_color_new (const BdkColor *color)
{
  BdkBangoAttrEmbossColor *result;
  
  static BangoAttrClass klass = {
    0,
    bdk_bango_attr_emboss_color_copy,
    bdk_bango_attr_emboss_color_destroy,
    bdk_bango_attr_emboss_color_compare
  };

  if (!klass.type)
    klass.type = bdk_bango_attr_emboss_color_type =
      bango_attr_type_register ("BdkBangoAttrEmbossColor");

  result = g_new (BdkBangoAttrEmbossColor, 1);
  result->attr.klass = &klass;
  result->color.red = color->red;
  result->color.green = color->green;
  result->color.blue = color->blue;

  return (BangoAttribute *) result;
}

/* Get a clip rebunnyion to draw only part of a layout. index_ranges
 * contains alternating range starts/stops. The rebunnyion is the
 * rebunnyion which contains the given ranges, i.e. if you draw with the
 * rebunnyion as clip, only the given ranges are drawn.
 */
static BdkRebunnyion*
layout_iter_get_line_clip_rebunnyion (BangoLayoutIter *iter,
				  gint             x_origin,
				  gint             y_origin,
				  const gint      *index_ranges,
				  gint             n_ranges)
{
  BangoLayoutLine *line;
  BdkRebunnyion *clip_rebunnyion;
  BangoRectangle logical_rect;
  gint baseline;
  gint i;

  line = bango_layout_iter_get_line_readonly (iter);

  clip_rebunnyion = bdk_rebunnyion_new ();

  bango_layout_iter_get_line_extents (iter, NULL, &logical_rect);
  baseline = bango_layout_iter_get_baseline (iter);

  i = 0;
  while (i < n_ranges)
    {  
      gint *pixel_ranges = NULL;
      gint n_pixel_ranges = 0;
      gint j;

      /* Note that get_x_ranges returns layout coordinates
       */
      if (index_ranges[i*2+1] >= line->start_index &&
	  index_ranges[i*2] < line->start_index + line->length)
	bango_layout_line_get_x_ranges (line,
					index_ranges[i*2],
					index_ranges[i*2+1],
					&pixel_ranges, &n_pixel_ranges);
  
      for (j = 0; j < n_pixel_ranges; j++)
        {
          BdkRectangle rect;
	  int x_off, y_off;
          
          x_off = BANGO_PIXELS (pixel_ranges[2*j] - logical_rect.x);
	  y_off = BANGO_PIXELS (baseline - logical_rect.y);

          rect.x = x_origin + x_off;
          rect.y = y_origin - y_off;
          rect.width = BANGO_PIXELS (pixel_ranges[2*j + 1] - logical_rect.x) - x_off;
          rect.height = BANGO_PIXELS (baseline - logical_rect.y + logical_rect.height) - y_off;

          bdk_rebunnyion_union_with_rect (clip_rebunnyion, &rect);
        }

      g_free (pixel_ranges);
      ++i;
    }
  return clip_rebunnyion;
}

/**
 * bdk_bango_layout_line_get_clip_rebunnyion:
 * @line: a #BangoLayoutLine 
 * @x_origin: X pixel where you intend to draw the layout line with this clip
 * @y_origin: baseline pixel where you intend to draw the layout line with this clip
 * @index_ranges: array of byte indexes into the layout, where even members of array are start indexes and odd elements are end indexes
 * @n_ranges: number of ranges in @index_ranges, i.e. half the size of @index_ranges
 * 
 * Obtains a clip rebunnyion which contains the areas where the given
 * ranges of text would be drawn. @x_origin and @y_origin are the same
 * position you would pass to bdk_draw_layout_line(). @index_ranges
 * should contain ranges of bytes in the layout's text. The clip
 * rebunnyion will include space to the left or right of the line (to the
 * layout bounding box) if you have indexes above or below the indexes
 * contained inside the line. This is to draw the selection all the way
 * to the side of the layout. However, the clip rebunnyion is in line coordinates,
 * not layout coordinates.
 *
 * Note that the rebunnyions returned correspond to logical extents of the text
 * ranges, not ink extents. So the drawn line may in fact touch areas out of
 * the clip rebunnyion.  The clip rebunnyion is mainly useful for highlightling parts
 * of text, such as when text is selected.
 * 
 * Return value: a clip rebunnyion containing the given ranges
 **/
BdkRebunnyion*
bdk_bango_layout_line_get_clip_rebunnyion (BangoLayoutLine *line,
                                       gint             x_origin,
                                       gint             y_origin,
                                       const gint      *index_ranges,
                                       gint             n_ranges)
{
  BdkRebunnyion *clip_rebunnyion;
  BangoLayoutIter *iter;
  
  g_return_val_if_fail (line != NULL, NULL);
  g_return_val_if_fail (index_ranges != NULL, NULL);
  
  iter = bango_layout_get_iter (line->layout);
  while (bango_layout_iter_get_line_readonly (iter) != line)
    bango_layout_iter_next_line (iter);
  
  clip_rebunnyion = layout_iter_get_line_clip_rebunnyion(iter, x_origin, y_origin, index_ranges, n_ranges);

  bango_layout_iter_free (iter);

  return clip_rebunnyion;
}

/**
 * bdk_bango_layout_get_clip_rebunnyion:
 * @layout: a #BangoLayout 
 * @x_origin: X pixel where you intend to draw the layout with this clip
 * @y_origin: Y pixel where you intend to draw the layout with this clip
 * @index_ranges: array of byte indexes into the layout, where even members of array are start indexes and odd elements are end indexes
 * @n_ranges: number of ranges in @index_ranges, i.e. half the size of @index_ranges
 * 
 * Obtains a clip rebunnyion which contains the areas where the given ranges
 * of text would be drawn. @x_origin and @y_origin are the same position
 * you would pass to bdk_draw_layout_line(). @index_ranges should contain
 * ranges of bytes in the layout's text.
 * 
 * Note that the rebunnyions returned correspond to logical extents of the text
 * ranges, not ink extents. So the drawn layout may in fact touch areas out of
 * the clip rebunnyion.  The clip rebunnyion is mainly useful for highlightling parts
 * of text, such as when text is selected.
 * 
 * Return value: a clip rebunnyion containing the given ranges
 **/
BdkRebunnyion*
bdk_bango_layout_get_clip_rebunnyion (BangoLayout *layout,
                                  gint         x_origin,
                                  gint         y_origin,
                                  const gint  *index_ranges,
                                  gint         n_ranges)
{
  BangoLayoutIter *iter;  
  BdkRebunnyion *clip_rebunnyion;
  
  g_return_val_if_fail (BANGO_IS_LAYOUT (layout), NULL);
  g_return_val_if_fail (index_ranges != NULL, NULL);
  
  clip_rebunnyion = bdk_rebunnyion_new ();
  
  iter = bango_layout_get_iter (layout);
  
  do
    {
      BangoRectangle logical_rect;
      BdkRebunnyion *line_rebunnyion;
      gint baseline;
      
      bango_layout_iter_get_line_extents (iter, NULL, &logical_rect);
      baseline = bango_layout_iter_get_baseline (iter);      

      line_rebunnyion = layout_iter_get_line_clip_rebunnyion(iter, 
						     x_origin + BANGO_PIXELS (logical_rect.x),
						     y_origin + BANGO_PIXELS (baseline),
						     index_ranges,
						     n_ranges);

      bdk_rebunnyion_union (clip_rebunnyion, line_rebunnyion);
      bdk_rebunnyion_destroy (line_rebunnyion);
    }
  while (bango_layout_iter_next_line (iter));

  bango_layout_iter_free (iter);

  return clip_rebunnyion;
}

/**
 * bdk_bango_context_get:
 * 
 * Creates a #BangoContext for the default BDK screen.
 *
 * The context must be freed when you're finished with it.
 * 
 * When using BTK+, normally you should use btk_widget_get_bango_context()
 * instead of this function, to get the appropriate context for
 * the widget you intend to render text onto.
 * 
 * The newly created context will have the default font options (see
 * #bairo_font_options_t) for the default screen; if these options
 * change it will not be updated. Using btk_widget_get_bango_context()
 * is more convenient if you want to keep a context around and track
 * changes to the screen's font rendering settings.
 *
 * Return value: a new #BangoContext for the default display
 **/
BangoContext *
bdk_bango_context_get (void)
{
  return bdk_bango_context_get_for_screen (bdk_screen_get_default ());
}

/**
 * bdk_bango_context_get_for_screen:
 * @screen: the #BdkScreen for which the context is to be created.
 * 
 * Creates a #BangoContext for @screen.
 *
 * The context must be freed when you're finished with it.
 * 
 * When using BTK+, normally you should use btk_widget_get_bango_context()
 * instead of this function, to get the appropriate context for
 * the widget you intend to render text onto.
 * 
 * The newly created context will have the default font options
 * (see #bairo_font_options_t) for the screen; if these options
 * change it will not be updated. Using btk_widget_get_bango_context()
 * is more convenient if you want to keep a context around and track
 * changes to the screen's font rendering settings.
 * 
 * Return value: a new #BangoContext for @screen
 *
 * Since: 2.2
 **/
BangoContext *
bdk_bango_context_get_for_screen (BdkScreen *screen)
{
  BangoFontMap *fontmap;
  BangoContext *context;
  const bairo_font_options_t *options;
  double dpi;

  g_return_val_if_fail (BDK_IS_SCREEN (screen), NULL);

  fontmap = bango_bairo_font_map_get_default ();
  context = bango_font_map_create_context (fontmap);

  options = bdk_screen_get_font_options (screen);
  bango_bairo_context_set_font_options (context, options);

  dpi = bdk_screen_get_resolution (screen);
  bango_bairo_context_set_resolution (context, dpi);

  return context;
}

#define __BDK_BANGO_C__
#include "bdkaliasdef.c"
