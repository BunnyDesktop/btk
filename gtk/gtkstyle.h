/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/.
 */

#ifndef __BTK_STYLE_H__
#define __BTK_STYLE_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <bdk/bdk.h>
#include <btk/btkenums.h>


G_BEGIN_DECLS

#define BTK_TYPE_STYLE              (btk_style_get_type ())
#define BTK_STYLE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BTK_TYPE_STYLE, BtkStyle))
#define BTK_STYLE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_STYLE, BtkStyleClass))
#define BTK_IS_STYLE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BTK_TYPE_STYLE))
#define BTK_IS_STYLE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_STYLE))
#define BTK_STYLE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_STYLE, BtkStyleClass))

#define BTK_TYPE_BORDER             (btk_border_get_type ())

/* Some forward declarations needed to rationalize the header
 * files.
 */
typedef struct _BtkBorder      BtkBorder;
typedef struct _BtkStyle       BtkStyle;
typedef struct _BtkStyleClass  BtkStyleClass;
typedef struct _BtkThemeEngine BtkThemeEngine;
typedef struct _BtkRcStyle     BtkRcStyle;
typedef struct _BtkIconSet     BtkIconSet;
typedef struct _BtkIconSource  BtkIconSource;
typedef struct _BtkRcProperty  BtkRcProperty;
typedef struct _BtkSettings    BtkSettings;
typedef gboolean (*BtkRcPropertyParser) (const GParamSpec *pspec,
					 const GString    *rc_string,
					 GValue           *property_value);

/* We make this forward declaration here, since we pass
 * BtkWidget's to the draw functions.
 */
typedef struct _BtkWidget      BtkWidget;

#define BTK_STYLE_ATTACHED(style)	(BTK_STYLE (style)->attach_count > 0)

struct _BtkStyle
{
  GObject parent_instance;

  /*< public >*/

  BdkColor fg[5];
  BdkColor bg[5];
  BdkColor light[5];
  BdkColor dark[5];
  BdkColor mid[5];
  BdkColor text[5];
  BdkColor base[5];
  BdkColor text_aa[5];		/* Halfway between text/base */

  BdkColor black;
  BdkColor white;
  BangoFontDescription *font_desc;

  gint xthickness;
  gint ythickness;

  BdkGC *fg_gc[5];
  BdkGC *bg_gc[5];
  BdkGC *light_gc[5];
  BdkGC *dark_gc[5];
  BdkGC *mid_gc[5];
  BdkGC *text_gc[5];
  BdkGC *base_gc[5];
  BdkGC *text_aa_gc[5];
  BdkGC *black_gc;
  BdkGC *white_gc;

  BdkPixmap *bg_pixmap[5];

  /*< private >*/

  gint attach_count;

  gint depth;
  BdkColormap *colormap;
  BdkFont *private_font;
  BangoFontDescription *private_font_desc; /* Font description for style->private_font or %NULL */

  /* the RcStyle from which this style was created */
  BtkRcStyle	 *rc_style;

  GSList	 *styles;	  /* of type BtkStyle* */
  GArray	 *property_cache;
  GSList         *icon_factories; /* of type BtkIconFactory* */
};

struct _BtkStyleClass
{
  GObjectClass parent_class;

  /* Initialize for a particular colormap/depth
   * combination. style->colormap/style->depth will have
   * been set at this point. Will typically chain to parent.
   */
  void (*realize)               (BtkStyle               *style);

  /* Clean up for a particular colormap/depth combination. Will
   * typically chain to parent.
   */
  void (*unrealize)             (BtkStyle               *style);

  /* Make style an exact duplicate of src.
   */
  void (*copy)                  (BtkStyle               *style,
				 BtkStyle               *src);

  /* Create an empty style of the same type as this style.
   * The default implementation, which does
   * g_object_new (G_OBJECT_TYPE (style), NULL);
   * should work in most cases.
   */
  BtkStyle *(*clone)             (BtkStyle               *style);

  /* Initialize the BtkStyle with the values in the BtkRcStyle.
   * should chain to the parent implementation.
   */
  void     (*init_from_rc)      (BtkStyle               *style,
				 BtkRcStyle             *rc_style);

  void (*set_background)        (BtkStyle               *style,
				 BdkWindow              *window,
				 BtkStateType            state_type);


  BdkPixbuf * (* render_icon)   (BtkStyle               *style,
                                 const BtkIconSource    *source,
                                 BtkTextDirection        direction,
                                 BtkStateType            state,
                                 BtkIconSize             size,
                                 BtkWidget              *widget,
                                 const gchar            *detail);

  /* Drawing functions
   */

  void (*draw_hline)		(BtkStyle		*style,
				 BdkWindow		*window,
				 BtkStateType		 state_type,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const gchar		*detail,
				 gint			 x1,
				 gint			 x2,
				 gint			 y);
  void (*draw_vline)		(BtkStyle		*style,
				 BdkWindow		*window,
				 BtkStateType		 state_type,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const gchar		*detail,
				 gint			 y1_,
				 gint			 y2_,
				 gint			 x);
  void (*draw_shadow)		(BtkStyle		*style,
				 BdkWindow		*window,
				 BtkStateType		 state_type,
				 BtkShadowType		 shadow_type,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const gchar		*detail,
				 gint			 x,
				 gint			 y,
				 gint			 width,
				 gint			 height);
  void (*draw_polygon)		(BtkStyle		*style,
				 BdkWindow		*window,
				 BtkStateType		 state_type,
				 BtkShadowType		 shadow_type,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const gchar		*detail,
				 BdkPoint		*point,
				 gint			 npoints,
				 gboolean		 fill);
  void (*draw_arrow)		(BtkStyle		*style,
				 BdkWindow		*window,
				 BtkStateType		 state_type,
				 BtkShadowType		 shadow_type,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const gchar		*detail,
				 BtkArrowType		 arrow_type,
				 gboolean		 fill,
				 gint			 x,
				 gint			 y,
				 gint			 width,
				 gint			 height);
  void (*draw_diamond)		(BtkStyle		*style,
				 BdkWindow		*window,
				 BtkStateType		 state_type,
				 BtkShadowType		 shadow_type,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const gchar		*detail,
				 gint			 x,
				 gint			 y,
				 gint			 width,
				 gint			 height);
  void (*draw_string)		(BtkStyle		*style,
				 BdkWindow		*window,
				 BtkStateType		 state_type,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const gchar		*detail,
				 gint			 x,
				 gint			 y,
				 const gchar		*string);
  void (*draw_box)		(BtkStyle		*style,
				 BdkWindow		*window,
				 BtkStateType		 state_type,
				 BtkShadowType		 shadow_type,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const gchar		*detail,
				 gint			 x,
				 gint			 y,
				 gint			 width,
				 gint			 height);
  void (*draw_flat_box)		(BtkStyle		*style,
				 BdkWindow		*window,
				 BtkStateType		 state_type,
				 BtkShadowType		 shadow_type,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const gchar		*detail,
				 gint			 x,
				 gint			 y,
				 gint			 width,
				 gint			 height);
  void (*draw_check)		(BtkStyle		*style,
				 BdkWindow		*window,
				 BtkStateType		 state_type,
				 BtkShadowType		 shadow_type,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const gchar		*detail,
				 gint			 x,
				 gint			 y,
				 gint			 width,
				 gint			 height);
  void (*draw_option)		(BtkStyle		*style,
				 BdkWindow		*window,
				 BtkStateType		 state_type,
				 BtkShadowType		 shadow_type,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const gchar		*detail,
				 gint			 x,
				 gint			 y,
				 gint			 width,
				 gint			 height);
  void (*draw_tab)		(BtkStyle		*style,
				 BdkWindow		*window,
				 BtkStateType		 state_type,
				 BtkShadowType		 shadow_type,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const gchar		*detail,
				 gint			 x,
				 gint			 y,
				 gint			 width,
				 gint			 height);
  void (*draw_shadow_gap)	(BtkStyle		*style,
				 BdkWindow		*window,
				 BtkStateType		 state_type,
				 BtkShadowType		 shadow_type,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const gchar		*detail,
				 gint			 x,
				 gint			 y,
				 gint			 width,
				 gint			 height,
				 BtkPositionType	 gap_side,
				 gint			 gap_x,
				 gint			 gap_width);
  void (*draw_box_gap)		(BtkStyle		*style,
				 BdkWindow		*window,
				 BtkStateType		 state_type,
				 BtkShadowType		 shadow_type,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const gchar		*detail,
				 gint			 x,
				 gint			 y,
				 gint			 width,
				 gint			 height,
				 BtkPositionType	 gap_side,
				 gint			 gap_x,
				 gint			 gap_width);
  void (*draw_extension)	(BtkStyle		*style,
				 BdkWindow		*window,
				 BtkStateType		 state_type,
				 BtkShadowType		 shadow_type,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const gchar		*detail,
				 gint			 x,
				 gint			 y,
				 gint			 width,
				 gint			 height,
				 BtkPositionType	 gap_side);
  void (*draw_focus)		(BtkStyle		*style,
				 BdkWindow		*window,
                                 BtkStateType            state_type,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const gchar		*detail,
				 gint			 x,
				 gint			 y,
				 gint			 width,
				 gint			 height);
  void (*draw_slider)		(BtkStyle		*style,
				 BdkWindow		*window,
				 BtkStateType		 state_type,
				 BtkShadowType		 shadow_type,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const gchar		*detail,
				 gint			 x,
				 gint			 y,
				 gint			 width,
				 gint			 height,
				 BtkOrientation		 orientation);
  void (*draw_handle)		(BtkStyle		*style,
				 BdkWindow		*window,
				 BtkStateType		 state_type,
				 BtkShadowType		 shadow_type,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const gchar		*detail,
				 gint			 x,
				 gint			 y,
				 gint			 width,
				 gint			 height,
				 BtkOrientation		 orientation);

  void (*draw_expander)		(BtkStyle		*style,
				 BdkWindow		*window,
				 BtkStateType		 state_type,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const gchar		*detail,
				 gint			 x,
				 gint			 y,
                                 BtkExpanderStyle        expander_style);
  void (*draw_layout)		(BtkStyle		*style,
				 BdkWindow		*window,
				 BtkStateType		 state_type,
				 gboolean                use_text,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const gchar		*detail,
				 gint			 x,
				 gint			 y,
                                 BangoLayout            *layout);
  void (*draw_resize_grip)      (BtkStyle		*style,
				 BdkWindow		*window,
				 BtkStateType		 state_type,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const gchar		*detail,
                                 BdkWindowEdge           edge,
				 gint			 x,
				 gint			 y,
				 gint			 width,
				 gint			 height);
  void (*draw_spinner)          (BtkStyle		*style,
				 BdkWindow		*window,
				 BtkStateType		 state_type,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const gchar		*detail,
				 guint                   step,
				 gint			 x,
				 gint			 y,
				 gint			 width,
				 gint			 height);

  /* Padding for future expansion */
  void (*_btk_reserved1)  (void);
  void (*_btk_reserved2)  (void);
  void (*_btk_reserved3)  (void);
  void (*_btk_reserved4)  (void);
  void (*_btk_reserved5)  (void);
  void (*_btk_reserved6)  (void);
  void (*_btk_reserved7)  (void);
  void (*_btk_reserved8)  (void);
  void (*_btk_reserved9)  (void);
  void (*_btk_reserved10) (void);
  void (*_btk_reserved11) (void);
};

struct _BtkBorder
{
  gint left;
  gint right;
  gint top;
  gint bottom;
};

GType     btk_style_get_type                 (void) G_GNUC_CONST;
BtkStyle* btk_style_new			     (void);
BtkStyle* btk_style_copy		     (BtkStyle	   *style);
BtkStyle* btk_style_attach		     (BtkStyle	   *style,
					      BdkWindow	   *window) G_GNUC_WARN_UNUSED_RESULT;
void	  btk_style_detach		     (BtkStyle	   *style);

#ifndef BTK_DISABLE_DEPRECATED
BtkStyle* btk_style_ref			     (BtkStyle	   *style);
void	  btk_style_unref		     (BtkStyle	   *style);

BdkFont * btk_style_get_font                 (BtkStyle     *style);
void      btk_style_set_font                 (BtkStyle     *style,
					      BdkFont      *font);
#endif /* BTK_DISABLE_DEPRECATED */

void	  btk_style_set_background	     (BtkStyle	   *style,
					      BdkWindow	   *window,
					      BtkStateType  state_type);
void	  btk_style_apply_default_background (BtkStyle	   *style,
					      BdkWindow	   *window,
					      gboolean	    set_bg,
					      BtkStateType  state_type,
					      const BdkRectangle *area,
					      gint	    x,
					      gint	    y,
					      gint	    width,
					      gint	    height);

BtkIconSet* btk_style_lookup_icon_set        (BtkStyle     *style,
                                              const gchar  *stock_id);
gboolean    btk_style_lookup_color           (BtkStyle     *style,
                                              const gchar  *color_name,
                                              BdkColor     *color);

BdkPixbuf*  btk_style_render_icon     (BtkStyle            *style,
                                       const BtkIconSource *source,
                                       BtkTextDirection     direction,
                                       BtkStateType         state,
                                       BtkIconSize          size,
                                       BtkWidget           *widget,
                                       const gchar         *detail);

#ifndef BTK_DISABLE_DEPRECATED
void btk_draw_hline      (BtkStyle        *style,
			  BdkWindow       *window,
			  BtkStateType     state_type,
			  gint             x1,
			  gint             x2,
			  gint             y);
void btk_draw_vline      (BtkStyle        *style,
			  BdkWindow       *window,
			  BtkStateType     state_type,
			  gint             y1_,
			  gint             y2_,
			  gint             x);
void btk_draw_shadow     (BtkStyle        *style,
			  BdkWindow       *window,
			  BtkStateType     state_type,
			  BtkShadowType    shadow_type,
			  gint             x,
			  gint             y,
			  gint             width,
			  gint             height);
void btk_draw_polygon    (BtkStyle        *style,
			  BdkWindow       *window,
			  BtkStateType     state_type,
			  BtkShadowType    shadow_type,
			  BdkPoint        *points,
			  gint             npoints,
			  gboolean         fill);
void btk_draw_arrow      (BtkStyle        *style,
			  BdkWindow       *window,
			  BtkStateType     state_type,
			  BtkShadowType    shadow_type,
			  BtkArrowType     arrow_type,
			  gboolean         fill,
			  gint             x,
			  gint             y,
			  gint             width,
			  gint             height);
void btk_draw_diamond    (BtkStyle        *style,
			  BdkWindow       *window,
			  BtkStateType     state_type,
			  BtkShadowType    shadow_type,
			  gint             x,
			  gint             y,
			  gint             width,
			  gint             height);
void btk_draw_box        (BtkStyle        *style,
			  BdkWindow       *window,
			  BtkStateType     state_type,
			  BtkShadowType    shadow_type,
			  gint             x,
			  gint             y,
			  gint             width,
			  gint             height);
void btk_draw_flat_box   (BtkStyle        *style,
			  BdkWindow       *window,
			  BtkStateType     state_type,
			  BtkShadowType    shadow_type,
			  gint             x,
			  gint             y,
			  gint             width,
			  gint             height);
void btk_draw_check      (BtkStyle        *style,
			  BdkWindow       *window,
			  BtkStateType     state_type,
			  BtkShadowType    shadow_type,
			  gint             x,
			  gint             y,
			  gint             width,
			  gint             height);
void btk_draw_option     (BtkStyle        *style,
			  BdkWindow       *window,
			  BtkStateType     state_type,
			  BtkShadowType    shadow_type,
			  gint             x,
			  gint             y,
			  gint             width,
			  gint             height);
void btk_draw_tab        (BtkStyle        *style,
			  BdkWindow       *window,
			  BtkStateType     state_type,
			  BtkShadowType    shadow_type,
			  gint             x,
			  gint             y,
			  gint             width,
			  gint             height);
void btk_draw_shadow_gap (BtkStyle        *style,
			  BdkWindow       *window,
			  BtkStateType     state_type,
			  BtkShadowType    shadow_type,
			  gint             x,
			  gint             y,
			  gint             width,
			  gint             height,
			  BtkPositionType  gap_side,
			  gint             gap_x,
			  gint             gap_width);
void btk_draw_box_gap    (BtkStyle        *style,
			  BdkWindow       *window,
			  BtkStateType     state_type,
			  BtkShadowType    shadow_type,
			  gint             x,
			  gint             y,
			  gint             width,
			  gint             height,
			  BtkPositionType  gap_side,
			  gint             gap_x,
			  gint             gap_width);
void btk_draw_extension  (BtkStyle        *style,
			  BdkWindow       *window,
			  BtkStateType     state_type,
			  BtkShadowType    shadow_type,
			  gint             x,
			  gint             y,
			  gint             width,
			  gint             height,
			  BtkPositionType  gap_side);
void btk_draw_focus      (BtkStyle        *style,
			  BdkWindow       *window,
			  gint             x,
			  gint             y,
			  gint             width,
			  gint             height);
void btk_draw_slider     (BtkStyle        *style,
			  BdkWindow       *window,
			  BtkStateType     state_type,
			  BtkShadowType    shadow_type,
			  gint             x,
			  gint             y,
			  gint             width,
			  gint             height,
			  BtkOrientation   orientation);
void btk_draw_handle     (BtkStyle        *style,
			  BdkWindow       *window,
			  BtkStateType     state_type,
			  BtkShadowType    shadow_type,
			  gint             x,
			  gint             y,
			  gint             width,
			  gint             height,
			  BtkOrientation   orientation);
void btk_draw_expander   (BtkStyle        *style,
                          BdkWindow       *window,
                          BtkStateType     state_type,
                          gint             x,
                          gint             y,
			  BtkExpanderStyle expander_style);
void btk_draw_layout     (BtkStyle        *style,
                          BdkWindow       *window,
                          BtkStateType     state_type,
			  gboolean         use_text,
                          gint             x,
                          gint             y,
                          BangoLayout     *layout);
void btk_draw_resize_grip (BtkStyle       *style,
                           BdkWindow      *window,
                           BtkStateType    state_type,
                           BdkWindowEdge   edge,
                           gint            x,
                           gint            y,
                           gint            width,
                           gint            height);
#endif /* BTK_DISABLE_DEPRECATED */

void btk_paint_hline       (BtkStyle           *style,
			    BdkWindow          *window,
			    BtkStateType        state_type,
			    const BdkRectangle *area,
			    BtkWidget          *widget,
			    const gchar        *detail,
			    gint                x1,
			    gint                x2,
			    gint                y);
void btk_paint_vline       (BtkStyle           *style,
			    BdkWindow          *window,
			    BtkStateType        state_type,
			    const BdkRectangle *area,
			    BtkWidget          *widget,
			    const gchar        *detail,
			    gint                y1_,
			    gint                y2_,
			    gint                x);
void btk_paint_shadow      (BtkStyle           *style,
			    BdkWindow          *window,
			    BtkStateType        state_type,
			    BtkShadowType       shadow_type,
			    const BdkRectangle *area,
			    BtkWidget          *widget,
			    const gchar        *detail,
			    gint                x,
			    gint                y,
			    gint                width,
			    gint                height);
void btk_paint_polygon     (BtkStyle           *style,
			    BdkWindow          *window,
			    BtkStateType        state_type,
			    BtkShadowType       shadow_type,
			    const BdkRectangle *area,
			    BtkWidget          *widget,
			    const gchar        *detail,
			    const BdkPoint     *points,
			    gint                n_points,
			    gboolean            fill);
void btk_paint_arrow       (BtkStyle           *style,
			    BdkWindow          *window,
			    BtkStateType        state_type,
			    BtkShadowType       shadow_type,
			    const BdkRectangle *area,
			    BtkWidget          *widget,
			    const gchar        *detail,
			    BtkArrowType        arrow_type,
			    gboolean            fill,
			    gint                x,
			    gint                y,
			    gint                width,
			    gint                height);
void btk_paint_diamond     (BtkStyle           *style,
			    BdkWindow          *window,
			    BtkStateType        state_type,
			    BtkShadowType       shadow_type,
			    const BdkRectangle *area,
			    BtkWidget          *widget,
			    const gchar        *detail,
			    gint                x,
			    gint                y,
			    gint                width,
			    gint                height);
void btk_paint_box         (BtkStyle           *style,
			    BdkWindow          *window,
			    BtkStateType        state_type,
			    BtkShadowType       shadow_type,
			    const BdkRectangle *area,
			    BtkWidget          *widget,
			    const gchar        *detail,
			    gint                x,
			    gint                y,
			    gint                width,
			    gint                height);
void btk_paint_flat_box    (BtkStyle           *style,
			    BdkWindow          *window,
			    BtkStateType        state_type,
			    BtkShadowType       shadow_type,
			    const BdkRectangle *area,
			    BtkWidget          *widget,
			    const gchar        *detail,
			    gint                x,
			    gint                y,
			    gint                width,
			    gint                height);
void btk_paint_check       (BtkStyle           *style,
			    BdkWindow          *window,
			    BtkStateType        state_type,
			    BtkShadowType       shadow_type,
			    const BdkRectangle *area,
			    BtkWidget          *widget,
			    const gchar        *detail,
			    gint                x,
			    gint                y,
			    gint                width,
			    gint                height);
void btk_paint_option      (BtkStyle           *style,
			    BdkWindow          *window,
			    BtkStateType        state_type,
			    BtkShadowType       shadow_type,
			    const BdkRectangle *area,
			    BtkWidget          *widget,
			    const gchar        *detail,
			    gint                x,
			    gint                y,
			    gint                width,
			    gint                height);
void btk_paint_tab         (BtkStyle           *style,
			    BdkWindow          *window,
			    BtkStateType        state_type,
			    BtkShadowType       shadow_type,
			    const BdkRectangle *area,
			    BtkWidget          *widget,
			    const gchar        *detail,
			    gint                x,
			    gint                y,
			    gint                width,
			    gint                height);
void btk_paint_shadow_gap  (BtkStyle           *style,
			    BdkWindow          *window,
			    BtkStateType        state_type,
			    BtkShadowType       shadow_type,
			    const BdkRectangle *area,
			    BtkWidget          *widget,
			    const gchar        *detail,
			    gint                x,
			    gint                y,
			    gint                width,
			    gint                height,
			    BtkPositionType     gap_side,
			    gint                gap_x,
			    gint                gap_width);
void btk_paint_box_gap     (BtkStyle           *style,
			    BdkWindow          *window,
			    BtkStateType        state_type,
			    BtkShadowType       shadow_type,
			    const BdkRectangle *area,
			    BtkWidget          *widget,
			    const gchar        *detail,
			    gint                x,
			    gint                y,
			    gint                width,
			    gint                height,
			    BtkPositionType     gap_side,
			    gint                gap_x,
			    gint                gap_width);
void btk_paint_extension   (BtkStyle           *style,
			    BdkWindow          *window,
			    BtkStateType        state_type,
			    BtkShadowType       shadow_type,
			    const BdkRectangle *area,
			    BtkWidget          *widget,
			    const gchar        *detail,
			    gint                x,
			    gint                y,
			    gint                width,
			    gint                height,
			    BtkPositionType     gap_side);
void btk_paint_focus       (BtkStyle           *style,
			    BdkWindow          *window,
			    BtkStateType        state_type,
			    const BdkRectangle *area,
			    BtkWidget          *widget,
			    const gchar        *detail,
			    gint                x,
			    gint                y,
			    gint                width,
			    gint                height);
void btk_paint_slider      (BtkStyle           *style,
			    BdkWindow          *window,
			    BtkStateType        state_type,
			    BtkShadowType       shadow_type,
			    const BdkRectangle *area,
			    BtkWidget          *widget,
			    const gchar        *detail,
			    gint                x,
			    gint                y,
			    gint                width,
			    gint                height,
			    BtkOrientation      orientation);
void btk_paint_handle      (BtkStyle           *style,
			    BdkWindow          *window,
			    BtkStateType        state_type,
			    BtkShadowType       shadow_type,
			    const BdkRectangle *area,
			    BtkWidget          *widget,
			    const gchar        *detail,
			    gint                x,
			    gint                y,
			    gint                width,
			    gint                height,
			    BtkOrientation      orientation);
void btk_paint_expander    (BtkStyle           *style,
                            BdkWindow          *window,
                            BtkStateType        state_type,
                            const BdkRectangle *area,
                            BtkWidget          *widget,
                            const gchar        *detail,
                            gint                x,
                            gint                y,
			    BtkExpanderStyle    expander_style);
void btk_paint_layout      (BtkStyle           *style,
                            BdkWindow          *window,
                            BtkStateType        state_type,
			    gboolean            use_text,
                            const BdkRectangle *area,
                            BtkWidget          *widget,
                            const gchar        *detail,
                            gint                x,
                            gint                y,
                            BangoLayout        *layout);
void btk_paint_resize_grip (BtkStyle           *style,
                            BdkWindow          *window,
                            BtkStateType        state_type,
                            const BdkRectangle *area,
                            BtkWidget          *widget,
                            const gchar        *detail,
                            BdkWindowEdge       edge,
                            gint                x,
                            gint                y,
                            gint                width,
                            gint                height);
void btk_paint_spinner     (BtkStyle           *style,
			    BdkWindow          *window,
			    BtkStateType        state_type,
                            const BdkRectangle *area,
                            BtkWidget          *widget,
                            const gchar        *detail,
			    guint               step,
			    gint                x,
			    gint                y,
			    gint                width,
			    gint                height);

GType      btk_border_get_type (void) G_GNUC_CONST;
BtkBorder *btk_border_new      (void) G_GNUC_MALLOC;
BtkBorder *btk_border_copy     (const BtkBorder *border_);
void       btk_border_free     (BtkBorder       *border_);

void btk_style_get_style_property (BtkStyle    *style,
                                   GType        widget_type,
                                   const gchar *property_name,
                                   GValue      *value);
void btk_style_get_valist         (BtkStyle    *style,
                                   GType        widget_type,
                                   const gchar *first_property_name,
                                   va_list      var_args);
void btk_style_get                (BtkStyle    *style,
                                   GType        widget_type,
                                   const gchar *first_property_name,
                                   ...) G_GNUC_NULL_TERMINATED;

/* --- private API --- */
const GValue* _btk_style_peek_property_value (BtkStyle           *style,
					      GType               widget_type,
					      GParamSpec         *pspec,
					      BtkRcPropertyParser parser);

void          _btk_style_init_for_settings   (BtkStyle           *style,
                                              BtkSettings        *settings);

void          _btk_style_shade               (const BdkColor     *a,
                                              BdkColor           *b,
                                              gdouble             k);

/* deprecated */
#ifndef BTK_DISABLE_DEPRECATED
#define btk_style_apply_default_pixmap(s,gw,st,a,x,y,w,h) btk_style_apply_default_background (s,gw,1,st,a,x,y,w,h)
void btk_draw_string      (BtkStyle           *style,
			   BdkWindow          *window,
                           BtkStateType        state_type,
                           gint                x,
                           gint                y,
                           const gchar        *string);
void btk_paint_string     (BtkStyle           *style,
			   BdkWindow          *window,
			   BtkStateType        state_type,
			   const BdkRectangle *area,
			   BtkWidget          *widget,
			   const gchar        *detail,
			   gint                x,
			   gint                y,
			   const gchar        *string);
#endif /* BTK_DISABLE_DEPRECATED */

void   btk_draw_insertion_cursor    (BtkWidget          *widget,
                                     BdkDrawable        *drawable,
                                     const BdkRectangle *area,
                                     const BdkRectangle *location,
                                     gboolean            is_primary,
                                     BtkTextDirection    direction,
                                     gboolean            draw_arrow);
BdkGC *_btk_widget_get_cursor_gc    (BtkWidget          *widget);
void   _btk_widget_get_cursor_color (BtkWidget          *widget,
				     BdkColor           *color);

G_END_DECLS

#endif /* __BTK_STYLE_H__ */
