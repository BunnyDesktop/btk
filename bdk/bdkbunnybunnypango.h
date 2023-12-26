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

#ifndef __BDK_BANGO_H__
#define __BDK_BANGO_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BDK_H_INSIDE__) && !defined (BDK_COMPILATION)
#error "Only <bdk/bdk.h> can be included directly."
#endif

#include <bdk/bdktypes.h>

B_BEGIN_DECLS

/* Bango interaction */

typedef struct _BdkBangoRenderer        BdkBangoRenderer;
typedef struct _BdkBangoRendererClass   BdkBangoRendererClass;
typedef struct _BdkBangoRendererPrivate BdkBangoRendererPrivate;

#define BDK_TYPE_BANGO_RENDERER            (bdk_bango_renderer_get_type())
#define BDK_BANGO_RENDERER(object)         (B_TYPE_CHECK_INSTANCE_CAST ((object), BDK_TYPE_BANGO_RENDERER, BdkBangoRenderer))
#define BDK_IS_BANGO_RENDERER(object)      (B_TYPE_CHECK_INSTANCE_TYPE ((object), BDK_TYPE_BANGO_RENDERER))
#define BDK_BANGO_RENDERER_CLASS(klass)    (B_TYPE_CHECK_CLASS_CAST ((klass), BDK_TYPE_BANGO_RENDERER, BdkBangoRendererClass))
#define BDK_IS_BANGO_RENDERER_CLASS(klass) (B_TYPE_CHECK_CLASS_TYPE ((klass), BDK_TYPE_BANGO_RENDERER))
#define BDK_BANGO_RENDERER_GET_CLASS(obj)  (B_TYPE_INSTANCE_GET_CLASS ((obj), BDK_TYPE_BANGO_RENDERER, BdkBangoRendererClass))

/**
 * BdkBangoRenderer:
 *
 * #BdkBangoRenderer is a subclass of #BangoRenderer used for rendering
 * Bango objects into BDK drawables. The default renderer for a particular
 * screen is obtained with bdk_bango_renderer_get_default(); Bango
 * functions like bango_renderer_draw_layout() and
 * bango_renderer_draw_layout_line() are then used to draw objects with
 * the renderer.
 *
 * In most simple cases, applications can just use bdk_draw_layout(), and
 * don't need to directly use #BdkBangoRenderer at all. Using the
 * #BdkBangoRenderer directly is most useful when working with a
 * transformation such as a rotation, because the Bango drawing functions
 * take user space coordinates (coordinates before the transformation)
 * instead of device coordinates.
 *
 * In certain cases it can be useful to subclass #BdkBangoRenderer. Examples
 * of reasons to do this are to add handling of custom attributes by
 * overriding 'prepare_run' or to do custom drawing of embedded objects
 * by overriding 'draw_shape'.
 *
 * Since: 2.6
 **/
struct _BdkBangoRenderer
{
  /*< private >*/
  BangoRenderer parent_instance;

  BdkBangoRendererPrivate *priv;
};

/**
 * BdkBangoRendererClass:
 *
 * #BdkBangoRenderer is the class structure for #BdkBangoRenderer.
 *
 * Since: 2.6
 **/
struct _BdkBangoRendererClass
{
  /*< private >*/
  BangoRendererClass parent_class;
};

GType bdk_bango_renderer_get_type (void) B_GNUC_CONST;

BangoRenderer *bdk_bango_renderer_new         (BdkScreen *screen);
BangoRenderer *bdk_bango_renderer_get_default (BdkScreen *screen);

void bdk_bango_renderer_set_drawable       (BdkBangoRenderer *bdk_renderer,
					    BdkDrawable      *drawable);
void bdk_bango_renderer_set_gc             (BdkBangoRenderer *bdk_renderer,
					    BdkGC            *gc);
void bdk_bango_renderer_set_stipple        (BdkBangoRenderer *bdk_renderer,
					    BangoRenderPart   part,
					    BdkBitmap        *stipple);
void bdk_bango_renderer_set_override_color (BdkBangoRenderer *bdk_renderer,
					    BangoRenderPart   part,
					    const BdkColor   *color);

/************************************************************************/

BangoContext *bdk_bango_context_get_for_screen (BdkScreen    *screen);
#ifndef BDK_MULTIHEAD_SAFE
BangoContext *bdk_bango_context_get            (void);
#endif
#ifndef BDK_DISABLE_DEPRECATED
void          bdk_bango_context_set_colormap   (BangoContext *context,
                                                BdkColormap  *colormap);
#endif 


/* Get a clip rebunnyion to draw only part of a layout or
 * line. index_ranges contains alternating range starts/stops. The
 * rebunnyion is the rebunnyion which contains the given ranges, i.e. if you
 * draw with the rebunnyion as clip, only the given ranges are drawn.
 */

BdkRebunnyion    *bdk_bango_layout_line_get_clip_rebunnyion (BangoLayoutLine *line,
                                                     gint             x_origin,
                                                     gint             y_origin,
                                                     const gint      *index_ranges,
                                                     gint             n_ranges);
BdkRebunnyion    *bdk_bango_layout_get_clip_rebunnyion      (BangoLayout     *layout,
                                                     gint             x_origin,
                                                     gint             y_origin,
                                                     const gint      *index_ranges,
                                                     gint             n_ranges);



/* Attributes use to render insensitive text in BTK+. */

typedef struct _BdkBangoAttrStipple BdkBangoAttrStipple;
typedef struct _BdkBangoAttrEmbossed BdkBangoAttrEmbossed;
typedef struct _BdkBangoAttrEmbossColor BdkBangoAttrEmbossColor;

struct _BdkBangoAttrStipple
{
  BangoAttribute attr;
  BdkBitmap *stipple;
};

struct _BdkBangoAttrEmbossed
{
  BangoAttribute attr;
  gboolean embossed;
};

struct _BdkBangoAttrEmbossColor
{
  BangoAttribute attr;
  BangoColor color;
};

BangoAttribute *bdk_bango_attr_stipple_new  (BdkBitmap *stipple);
BangoAttribute *bdk_bango_attr_embossed_new (gboolean embossed);
BangoAttribute *bdk_bango_attr_emboss_color_new (const BdkColor *color);

B_END_DECLS

#endif /* __BDK_FONT_H__ */
