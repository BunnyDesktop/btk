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


B_BEGIN_DECLS

#define BTK_TYPE_STYLE              (btk_style_get_type ())
#define BTK_STYLE(object)           (B_TYPE_CHECK_INSTANCE_CAST ((object), BTK_TYPE_STYLE, BtkStyle))
#define BTK_STYLE_CLASS(klass)      (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_STYLE, BtkStyleClass))
#define BTK_IS_STYLE(object)        (B_TYPE_CHECK_INSTANCE_TYPE ((object), BTK_TYPE_STYLE))
#define BTK_IS_STYLE_CLASS(klass)   (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_STYLE))
#define BTK_STYLE_GET_CLASS(obj)    (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_STYLE, BtkStyleClass))

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
typedef bboolean (*BtkRcPropertyParser) (const BParamSpec *pspec,
					 const GString    *rc_string,
					 BValue           *property_value);

/* We make this forward declaration here, since we pass
 * BtkWidget's to the draw functions.
 */
typedef struct _BtkWidget      BtkWidget;

#define BTK_STYLE_ATTACHED(style)	(BTK_STYLE (style)->attach_count > 0)

struct _BtkStyle
{
  BObject parent_instance;

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

  bint xthickness;
  bint ythickness;

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

  bint attach_count;

  bint depth;
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
  BObjectClass parent_class;

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
   * g_object_new (B_OBJECT_TYPE (style), NULL);
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
                                 const bchar            *detail);

  /* Drawing functions
   */

  void (*draw_hline)		(BtkStyle		*style,
				 BdkWindow		*window,
				 BtkStateType		 state_type,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const bchar		*detail,
				 bint			 x1,
				 bint			 x2,
				 bint			 y);
  void (*draw_vline)		(BtkStyle		*style,
				 BdkWindow		*window,
				 BtkStateType		 state_type,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const bchar		*detail,
				 bint			 y1_,
				 bint			 y2_,
				 bint			 x);
  void (*draw_shadow)		(BtkStyle		*style,
				 BdkWindow		*window,
				 BtkStateType		 state_type,
				 BtkShadowType		 shadow_type,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const bchar		*detail,
				 bint			 x,
				 bint			 y,
				 bint			 width,
				 bint			 height);
  void (*draw_polygon)		(BtkStyle		*style,
				 BdkWindow		*window,
				 BtkStateType		 state_type,
				 BtkShadowType		 shadow_type,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const bchar		*detail,
				 BdkPoint		*point,
				 bint			 npoints,
				 bboolean		 fill);
  void (*draw_arrow)		(BtkStyle		*style,
				 BdkWindow		*window,
				 BtkStateType		 state_type,
				 BtkShadowType		 shadow_type,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const bchar		*detail,
				 BtkArrowType		 arrow_type,
				 bboolean		 fill,
				 bint			 x,
				 bint			 y,
				 bint			 width,
				 bint			 height);
  void (*draw_diamond)		(BtkStyle		*style,
				 BdkWindow		*window,
				 BtkStateType		 state_type,
				 BtkShadowType		 shadow_type,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const bchar		*detail,
				 bint			 x,
				 bint			 y,
				 bint			 width,
				 bint			 height);
  void (*draw_string)		(BtkStyle		*style,
				 BdkWindow		*window,
				 BtkStateType		 state_type,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const bchar		*detail,
				 bint			 x,
				 bint			 y,
				 const bchar		*string);
  void (*draw_box)		(BtkStyle		*style,
				 BdkWindow		*window,
				 BtkStateType		 state_type,
				 BtkShadowType		 shadow_type,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const bchar		*detail,
				 bint			 x,
				 bint			 y,
				 bint			 width,
				 bint			 height);
  void (*draw_flat_box)		(BtkStyle		*style,
				 BdkWindow		*window,
				 BtkStateType		 state_type,
				 BtkShadowType		 shadow_type,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const bchar		*detail,
				 bint			 x,
				 bint			 y,
				 bint			 width,
				 bint			 height);
  void (*draw_check)		(BtkStyle		*style,
				 BdkWindow		*window,
				 BtkStateType		 state_type,
				 BtkShadowType		 shadow_type,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const bchar		*detail,
				 bint			 x,
				 bint			 y,
				 bint			 width,
				 bint			 height);
  void (*draw_option)		(BtkStyle		*style,
				 BdkWindow		*window,
				 BtkStateType		 state_type,
				 BtkShadowType		 shadow_type,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const bchar		*detail,
				 bint			 x,
				 bint			 y,
				 bint			 width,
				 bint			 height);
  void (*draw_tab)		(BtkStyle		*style,
				 BdkWindow		*window,
				 BtkStateType		 state_type,
				 BtkShadowType		 shadow_type,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const bchar		*detail,
				 bint			 x,
				 bint			 y,
				 bint			 width,
				 bint			 height);
  void (*draw_shadow_gap)	(BtkStyle		*style,
				 BdkWindow		*window,
				 BtkStateType		 state_type,
				 BtkShadowType		 shadow_type,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const bchar		*detail,
				 bint			 x,
				 bint			 y,
				 bint			 width,
				 bint			 height,
				 BtkPositionType	 gap_side,
				 bint			 gap_x,
				 bint			 gap_width);
  void (*draw_box_gap)		(BtkStyle		*style,
				 BdkWindow		*window,
				 BtkStateType		 state_type,
				 BtkShadowType		 shadow_type,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const bchar		*detail,
				 bint			 x,
				 bint			 y,
				 bint			 width,
				 bint			 height,
				 BtkPositionType	 gap_side,
				 bint			 gap_x,
				 bint			 gap_width);
  void (*draw_extension)	(BtkStyle		*style,
				 BdkWindow		*window,
				 BtkStateType		 state_type,
				 BtkShadowType		 shadow_type,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const bchar		*detail,
				 bint			 x,
				 bint			 y,
				 bint			 width,
				 bint			 height,
				 BtkPositionType	 gap_side);
  void (*draw_focus)		(BtkStyle		*style,
				 BdkWindow		*window,
                                 BtkStateType            state_type,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const bchar		*detail,
				 bint			 x,
				 bint			 y,
				 bint			 width,
				 bint			 height);
  void (*draw_slider)		(BtkStyle		*style,
				 BdkWindow		*window,
				 BtkStateType		 state_type,
				 BtkShadowType		 shadow_type,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const bchar		*detail,
				 bint			 x,
				 bint			 y,
				 bint			 width,
				 bint			 height,
				 BtkOrientation		 orientation);
  void (*draw_handle)		(BtkStyle		*style,
				 BdkWindow		*window,
				 BtkStateType		 state_type,
				 BtkShadowType		 shadow_type,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const bchar		*detail,
				 bint			 x,
				 bint			 y,
				 bint			 width,
				 bint			 height,
				 BtkOrientation		 orientation);

  void (*draw_expander)		(BtkStyle		*style,
				 BdkWindow		*window,
				 BtkStateType		 state_type,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const bchar		*detail,
				 bint			 x,
				 bint			 y,
                                 BtkExpanderStyle        expander_style);
  void (*draw_layout)		(BtkStyle		*style,
				 BdkWindow		*window,
				 BtkStateType		 state_type,
				 bboolean                use_text,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const bchar		*detail,
				 bint			 x,
				 bint			 y,
                                 BangoLayout            *layout);
  void (*draw_resize_grip)      (BtkStyle		*style,
				 BdkWindow		*window,
				 BtkStateType		 state_type,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const bchar		*detail,
                                 BdkWindowEdge           edge,
				 bint			 x,
				 bint			 y,
				 bint			 width,
				 bint			 height);
  void (*draw_spinner)          (BtkStyle		*style,
				 BdkWindow		*window,
				 BtkStateType		 state_type,
				 BdkRectangle		*area,
				 BtkWidget		*widget,
				 const bchar		*detail,
				 buint                   step,
				 bint			 x,
				 bint			 y,
				 bint			 width,
				 bint			 height);

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
  bint left;
  bint right;
  bint top;
  bint bottom;
};

GType     btk_style_get_type                 (void) B_GNUC_CONST;
BtkStyle* btk_style_new			     (void);
BtkStyle* btk_style_copy		     (BtkStyle	   *style);
BtkStyle* btk_style_attach		     (BtkStyle	   *style,
					      BdkWindow	   *window) B_GNUC_WARN_UNUSED_RESULT;
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
					      bboolean	    set_bg,
					      BtkStateType  state_type,
					      const BdkRectangle *area,
					      bint	    x,
					      bint	    y,
					      bint	    width,
					      bint	    height);

BtkIconSet* btk_style_lookup_icon_set        (BtkStyle     *style,
                                              const bchar  *stock_id);
bboolean    btk_style_lookup_color           (BtkStyle     *style,
                                              const bchar  *color_name,
                                              BdkColor     *color);

BdkPixbuf*  btk_style_render_icon     (BtkStyle            *style,
                                       const BtkIconSource *source,
                                       BtkTextDirection     direction,
                                       BtkStateType         state,
                                       BtkIconSize          size,
                                       BtkWidget           *widget,
                                       const bchar         *detail);

#ifndef BTK_DISABLE_DEPRECATED
void btk_draw_hline      (BtkStyle        *style,
			  BdkWindow       *window,
			  BtkStateType     state_type,
			  bint             x1,
			  bint             x2,
			  bint             y);
void btk_draw_vline      (BtkStyle        *style,
			  BdkWindow       *window,
			  BtkStateType     state_type,
			  bint             y1_,
			  bint             y2_,
			  bint             x);
void btk_draw_shadow     (BtkStyle        *style,
			  BdkWindow       *window,
			  BtkStateType     state_type,
			  BtkShadowType    shadow_type,
			  bint             x,
			  bint             y,
			  bint             width,
			  bint             height);
void btk_draw_polygon    (BtkStyle        *style,
			  BdkWindow       *window,
			  BtkStateType     state_type,
			  BtkShadowType    shadow_type,
			  BdkPoint        *points,
			  bint             npoints,
			  bboolean         fill);
void btk_draw_arrow      (BtkStyle        *style,
			  BdkWindow       *window,
			  BtkStateType     state_type,
			  BtkShadowType    shadow_type,
			  BtkArrowType     arrow_type,
			  bboolean         fill,
			  bint             x,
			  bint             y,
			  bint             width,
			  bint             height);
void btk_draw_diamond    (BtkStyle        *style,
			  BdkWindow       *window,
			  BtkStateType     state_type,
			  BtkShadowType    shadow_type,
			  bint             x,
			  bint             y,
			  bint             width,
			  bint             height);
void btk_draw_box        (BtkStyle        *style,
			  BdkWindow       *window,
			  BtkStateType     state_type,
			  BtkShadowType    shadow_type,
			  bint             x,
			  bint             y,
			  bint             width,
			  bint             height);
void btk_draw_flat_box   (BtkStyle        *style,
			  BdkWindow       *window,
			  BtkStateType     state_type,
			  BtkShadowType    shadow_type,
			  bint             x,
			  bint             y,
			  bint             width,
			  bint             height);
void btk_draw_check      (BtkStyle        *style,
			  BdkWindow       *window,
			  BtkStateType     state_type,
			  BtkShadowType    shadow_type,
			  bint             x,
			  bint             y,
			  bint             width,
			  bint             height);
void btk_draw_option     (BtkStyle        *style,
			  BdkWindow       *window,
			  BtkStateType     state_type,
			  BtkShadowType    shadow_type,
			  bint             x,
			  bint             y,
			  bint             width,
			  bint             height);
void btk_draw_tab        (BtkStyle        *style,
			  BdkWindow       *window,
			  BtkStateType     state_type,
			  BtkShadowType    shadow_type,
			  bint             x,
			  bint             y,
			  bint             width,
			  bint             height);
void btk_draw_shadow_gap (BtkStyle        *style,
			  BdkWindow       *window,
			  BtkStateType     state_type,
			  BtkShadowType    shadow_type,
			  bint             x,
			  bint             y,
			  bint             width,
			  bint             height,
			  BtkPositionType  gap_side,
			  bint             gap_x,
			  bint             gap_width);
void btk_draw_box_gap    (BtkStyle        *style,
			  BdkWindow       *window,
			  BtkStateType     state_type,
			  BtkShadowType    shadow_type,
			  bint             x,
			  bint             y,
			  bint             width,
			  bint             height,
			  BtkPositionType  gap_side,
			  bint             gap_x,
			  bint             gap_width);
void btk_draw_extension  (BtkStyle        *style,
			  BdkWindow       *window,
			  BtkStateType     state_type,
			  BtkShadowType    shadow_type,
			  bint             x,
			  bint             y,
			  bint             width,
			  bint             height,
			  BtkPositionType  gap_side);
void btk_draw_focus      (BtkStyle        *style,
			  BdkWindow       *window,
			  bint             x,
			  bint             y,
			  bint             width,
			  bint             height);
void btk_draw_slider     (BtkStyle        *style,
			  BdkWindow       *window,
			  BtkStateType     state_type,
			  BtkShadowType    shadow_type,
			  bint             x,
			  bint             y,
			  bint             width,
			  bint             height,
			  BtkOrientation   orientation);
void btk_draw_handle     (BtkStyle        *style,
			  BdkWindow       *window,
			  BtkStateType     state_type,
			  BtkShadowType    shadow_type,
			  bint             x,
			  bint             y,
			  bint             width,
			  bint             height,
			  BtkOrientation   orientation);
void btk_draw_expander   (BtkStyle        *style,
                          BdkWindow       *window,
                          BtkStateType     state_type,
                          bint             x,
                          bint             y,
			  BtkExpanderStyle expander_style);
void btk_draw_layout     (BtkStyle        *style,
                          BdkWindow       *window,
                          BtkStateType     state_type,
			  bboolean         use_text,
                          bint             x,
                          bint             y,
                          BangoLayout     *layout);
void btk_draw_resize_grip (BtkStyle       *style,
                           BdkWindow      *window,
                           BtkStateType    state_type,
                           BdkWindowEdge   edge,
                           bint            x,
                           bint            y,
                           bint            width,
                           bint            height);
#endif /* BTK_DISABLE_DEPRECATED */

void btk_paint_hline       (BtkStyle           *style,
			    BdkWindow          *window,
			    BtkStateType        state_type,
			    const BdkRectangle *area,
			    BtkWidget          *widget,
			    const bchar        *detail,
			    bint                x1,
			    bint                x2,
			    bint                y);
void btk_paint_vline       (BtkStyle           *style,
			    BdkWindow          *window,
			    BtkStateType        state_type,
			    const BdkRectangle *area,
			    BtkWidget          *widget,
			    const bchar        *detail,
			    bint                y1_,
			    bint                y2_,
			    bint                x);
void btk_paint_shadow      (BtkStyle           *style,
			    BdkWindow          *window,
			    BtkStateType        state_type,
			    BtkShadowType       shadow_type,
			    const BdkRectangle *area,
			    BtkWidget          *widget,
			    const bchar        *detail,
			    bint                x,
			    bint                y,
			    bint                width,
			    bint                height);
void btk_paint_polygon     (BtkStyle           *style,
			    BdkWindow          *window,
			    BtkStateType        state_type,
			    BtkShadowType       shadow_type,
			    const BdkRectangle *area,
			    BtkWidget          *widget,
			    const bchar        *detail,
			    const BdkPoint     *points,
			    bint                n_points,
			    bboolean            fill);
void btk_paint_arrow       (BtkStyle           *style,
			    BdkWindow          *window,
			    BtkStateType        state_type,
			    BtkShadowType       shadow_type,
			    const BdkRectangle *area,
			    BtkWidget          *widget,
			    const bchar        *detail,
			    BtkArrowType        arrow_type,
			    bboolean            fill,
			    bint                x,
			    bint                y,
			    bint                width,
			    bint                height);
void btk_paint_diamond     (BtkStyle           *style,
			    BdkWindow          *window,
			    BtkStateType        state_type,
			    BtkShadowType       shadow_type,
			    const BdkRectangle *area,
			    BtkWidget          *widget,
			    const bchar        *detail,
			    bint                x,
			    bint                y,
			    bint                width,
			    bint                height);
void btk_paint_box         (BtkStyle           *style,
			    BdkWindow          *window,
			    BtkStateType        state_type,
			    BtkShadowType       shadow_type,
			    const BdkRectangle *area,
			    BtkWidget          *widget,
			    const bchar        *detail,
			    bint                x,
			    bint                y,
			    bint                width,
			    bint                height);
void btk_paint_flat_box    (BtkStyle           *style,
			    BdkWindow          *window,
			    BtkStateType        state_type,
			    BtkShadowType       shadow_type,
			    const BdkRectangle *area,
			    BtkWidget          *widget,
			    const bchar        *detail,
			    bint                x,
			    bint                y,
			    bint                width,
			    bint                height);
void btk_paint_check       (BtkStyle           *style,
			    BdkWindow          *window,
			    BtkStateType        state_type,
			    BtkShadowType       shadow_type,
			    const BdkRectangle *area,
			    BtkWidget          *widget,
			    const bchar        *detail,
			    bint                x,
			    bint                y,
			    bint                width,
			    bint                height);
void btk_paint_option      (BtkStyle           *style,
			    BdkWindow          *window,
			    BtkStateType        state_type,
			    BtkShadowType       shadow_type,
			    const BdkRectangle *area,
			    BtkWidget          *widget,
			    const bchar        *detail,
			    bint                x,
			    bint                y,
			    bint                width,
			    bint                height);
void btk_paint_tab         (BtkStyle           *style,
			    BdkWindow          *window,
			    BtkStateType        state_type,
			    BtkShadowType       shadow_type,
			    const BdkRectangle *area,
			    BtkWidget          *widget,
			    const bchar        *detail,
			    bint                x,
			    bint                y,
			    bint                width,
			    bint                height);
void btk_paint_shadow_gap  (BtkStyle           *style,
			    BdkWindow          *window,
			    BtkStateType        state_type,
			    BtkShadowType       shadow_type,
			    const BdkRectangle *area,
			    BtkWidget          *widget,
			    const bchar        *detail,
			    bint                x,
			    bint                y,
			    bint                width,
			    bint                height,
			    BtkPositionType     gap_side,
			    bint                gap_x,
			    bint                gap_width);
void btk_paint_box_gap     (BtkStyle           *style,
			    BdkWindow          *window,
			    BtkStateType        state_type,
			    BtkShadowType       shadow_type,
			    const BdkRectangle *area,
			    BtkWidget          *widget,
			    const bchar        *detail,
			    bint                x,
			    bint                y,
			    bint                width,
			    bint                height,
			    BtkPositionType     gap_side,
			    bint                gap_x,
			    bint                gap_width);
void btk_paint_extension   (BtkStyle           *style,
			    BdkWindow          *window,
			    BtkStateType        state_type,
			    BtkShadowType       shadow_type,
			    const BdkRectangle *area,
			    BtkWidget          *widget,
			    const bchar        *detail,
			    bint                x,
			    bint                y,
			    bint                width,
			    bint                height,
			    BtkPositionType     gap_side);
void btk_paint_focus       (BtkStyle           *style,
			    BdkWindow          *window,
			    BtkStateType        state_type,
			    const BdkRectangle *area,
			    BtkWidget          *widget,
			    const bchar        *detail,
			    bint                x,
			    bint                y,
			    bint                width,
			    bint                height);
void btk_paint_slider      (BtkStyle           *style,
			    BdkWindow          *window,
			    BtkStateType        state_type,
			    BtkShadowType       shadow_type,
			    const BdkRectangle *area,
			    BtkWidget          *widget,
			    const bchar        *detail,
			    bint                x,
			    bint                y,
			    bint                width,
			    bint                height,
			    BtkOrientation      orientation);
void btk_paint_handle      (BtkStyle           *style,
			    BdkWindow          *window,
			    BtkStateType        state_type,
			    BtkShadowType       shadow_type,
			    const BdkRectangle *area,
			    BtkWidget          *widget,
			    const bchar        *detail,
			    bint                x,
			    bint                y,
			    bint                width,
			    bint                height,
			    BtkOrientation      orientation);
void btk_paint_expander    (BtkStyle           *style,
                            BdkWindow          *window,
                            BtkStateType        state_type,
                            const BdkRectangle *area,
                            BtkWidget          *widget,
                            const bchar        *detail,
                            bint                x,
                            bint                y,
			    BtkExpanderStyle    expander_style);
void btk_paint_layout      (BtkStyle           *style,
                            BdkWindow          *window,
                            BtkStateType        state_type,
			    bboolean            use_text,
                            const BdkRectangle *area,
                            BtkWidget          *widget,
                            const bchar        *detail,
                            bint                x,
                            bint                y,
                            BangoLayout        *layout);
void btk_paint_resize_grip (BtkStyle           *style,
                            BdkWindow          *window,
                            BtkStateType        state_type,
                            const BdkRectangle *area,
                            BtkWidget          *widget,
                            const bchar        *detail,
                            BdkWindowEdge       edge,
                            bint                x,
                            bint                y,
                            bint                width,
                            bint                height);
void btk_paint_spinner     (BtkStyle           *style,
			    BdkWindow          *window,
			    BtkStateType        state_type,
                            const BdkRectangle *area,
                            BtkWidget          *widget,
                            const bchar        *detail,
			    buint               step,
			    bint                x,
			    bint                y,
			    bint                width,
			    bint                height);

GType      btk_border_get_type (void) B_GNUC_CONST;
BtkBorder *btk_border_new      (void) B_GNUC_MALLOC;
BtkBorder *btk_border_copy     (const BtkBorder *border_);
void       btk_border_free     (BtkBorder       *border_);

void btk_style_get_style_property (BtkStyle    *style,
                                   GType        widget_type,
                                   const bchar *property_name,
                                   BValue      *value);
void btk_style_get_valist         (BtkStyle    *style,
                                   GType        widget_type,
                                   const bchar *first_property_name,
                                   va_list      var_args);
void btk_style_get                (BtkStyle    *style,
                                   GType        widget_type,
                                   const bchar *first_property_name,
                                   ...) B_GNUC_NULL_TERMINATED;

/* --- private API --- */
const BValue* _btk_style_peek_property_value (BtkStyle           *style,
					      GType               widget_type,
					      BParamSpec         *pspec,
					      BtkRcPropertyParser parser);

void          _btk_style_init_for_settings   (BtkStyle           *style,
                                              BtkSettings        *settings);

void          _btk_style_shade               (const BdkColor     *a,
                                              BdkColor           *b,
                                              bdouble             k);

/* deprecated */
#ifndef BTK_DISABLE_DEPRECATED
#define btk_style_apply_default_pixmap(s,gw,st,a,x,y,w,h) btk_style_apply_default_background (s,gw,1,st,a,x,y,w,h)
void btk_draw_string      (BtkStyle           *style,
			   BdkWindow          *window,
                           BtkStateType        state_type,
                           bint                x,
                           bint                y,
                           const bchar        *string);
void btk_paint_string     (BtkStyle           *style,
			   BdkWindow          *window,
			   BtkStateType        state_type,
			   const BdkRectangle *area,
			   BtkWidget          *widget,
			   const bchar        *detail,
			   bint                x,
			   bint                y,
			   const bchar        *string);
#endif /* BTK_DISABLE_DEPRECATED */

void   btk_draw_insertion_cursor    (BtkWidget          *widget,
                                     BdkDrawable        *drawable,
                                     const BdkRectangle *area,
                                     const BdkRectangle *location,
                                     bboolean            is_primary,
                                     BtkTextDirection    direction,
                                     bboolean            draw_arrow);
BdkGC *_btk_widget_get_cursor_gc    (BtkWidget          *widget);
void   _btk_widget_get_cursor_color (BtkWidget          *widget,
				     BdkColor           *color);

B_END_DECLS

#endif /* __BTK_STYLE_H__ */
