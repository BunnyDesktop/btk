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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
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

#include "config.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <bobject/gvaluecollector.h>
#undef BDK_DISABLE_DEPRECATED
#include "btkgc.h"
#include "btkmarshalers.h"
#undef BTK_DISABLE_DEPRECATED
#include "btkoptionmenu.h"
#include "btkrc.h"
#include "btkspinbutton.h"
#include "btkstyle.h"
#include "btkwidget.h"
#include "btkthemes.h"
#include "btkiconfactory.h"
#include "btksettings.h"	/* _btk_settings_parse_convert() */
#include "btkintl.h"
#include "btkalias.h"

#define LIGHTNESS_MULT  1.3
#define DARKNESS_MULT   0.7

/* --- typedefs & structures --- */
typedef struct {
  GType       widget_type;
  GParamSpec *pspec;
  GValue      value;
} PropertyValue;

#define BTK_STYLE_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), BTK_TYPE_STYLE, BtkStylePrivate))

typedef struct _BtkStylePrivate BtkStylePrivate;

struct _BtkStylePrivate {
  GSList *color_hashes;
};

/* --- prototypes --- */
static void	 btk_style_finalize		(GObject	*object);
static void	 btk_style_realize		(BtkStyle	*style,
						 BdkColormap	*colormap);
static void      btk_style_real_realize        (BtkStyle	*style);
static void      btk_style_real_unrealize      (BtkStyle	*style);
static void      btk_style_real_copy           (BtkStyle	*style,
						BtkStyle	*src);
static void      btk_style_real_set_background (BtkStyle	*style,
						BdkWindow	*window,
						BtkStateType	 state_type);
static BtkStyle *btk_style_real_clone          (BtkStyle	*style);
static void      btk_style_real_init_from_rc   (BtkStyle	*style,
                                                BtkRcStyle	*rc_style);
static BdkPixbuf *btk_default_render_icon      (BtkStyle            *style,
                                                const BtkIconSource *source,
                                                BtkTextDirection     direction,
                                                BtkStateType         state,
                                                BtkIconSize          size,
                                                BtkWidget           *widget,
                                                const gchar         *detail);
static void btk_default_draw_hline      (BtkStyle        *style,
					 BdkWindow       *window,
					 BtkStateType     state_type,
					 BdkRectangle    *area,
					 BtkWidget       *widget,
					 const gchar     *detail,
					 gint             x1,
					 gint             x2,
					 gint             y);
static void btk_default_draw_vline      (BtkStyle        *style,
					 BdkWindow       *window,
					 BtkStateType     state_type,
					 BdkRectangle    *area,
					 BtkWidget       *widget,
					 const gchar     *detail,
					 gint             y1,
					 gint             y2,
					 gint             x);
static void btk_default_draw_shadow     (BtkStyle        *style,
					 BdkWindow       *window,
					 BtkStateType     state_type,
					 BtkShadowType    shadow_type,
					 BdkRectangle    *area,
					 BtkWidget       *widget,
					 const gchar     *detail,
					 gint             x,
					 gint             y,
					 gint             width,
					 gint             height);
static void btk_default_draw_polygon    (BtkStyle        *style,
					 BdkWindow       *window,
					 BtkStateType     state_type,
					 BtkShadowType    shadow_type,
					 BdkRectangle    *area,
					 BtkWidget       *widget,
					 const gchar     *detail,
					 BdkPoint        *points,
					 gint             npoints,
					 gboolean         fill);
static void btk_default_draw_arrow      (BtkStyle        *style,
					 BdkWindow       *window,
					 BtkStateType     state_type,
					 BtkShadowType    shadow_type,
					 BdkRectangle    *area,
					 BtkWidget       *widget,
					 const gchar     *detail,
					 BtkArrowType     arrow_type,
					 gboolean         fill,
					 gint             x,
					 gint             y,
					 gint             width,
					 gint             height);
static void btk_default_draw_diamond    (BtkStyle        *style,
					 BdkWindow       *window,
					 BtkStateType     state_type,
					 BtkShadowType    shadow_type,
					 BdkRectangle    *area,
					 BtkWidget       *widget,
					 const gchar     *detail,
					 gint             x,
					 gint             y,
					 gint             width,
					 gint             height);
static void btk_default_draw_string     (BtkStyle        *style,
					 BdkWindow       *window,
					 BtkStateType     state_type,
					 BdkRectangle    *area,
					 BtkWidget       *widget,
					 const gchar     *detail,
					 gint             x,
					 gint             y,
					 const gchar     *string);
static void btk_default_draw_box        (BtkStyle        *style,
					 BdkWindow       *window,
					 BtkStateType     state_type,
					 BtkShadowType    shadow_type,
					 BdkRectangle    *area,
					 BtkWidget       *widget,
					 const gchar     *detail,
					 gint             x,
					 gint             y,
					 gint             width,
					 gint             height);
static void btk_default_draw_flat_box   (BtkStyle        *style,
					 BdkWindow       *window,
					 BtkStateType     state_type,
					 BtkShadowType    shadow_type,
					 BdkRectangle    *area,
					 BtkWidget       *widget,
					 const gchar     *detail,
					 gint             x,
					 gint             y,
					 gint             width,
					 gint             height);
static void btk_default_draw_check      (BtkStyle        *style,
					 BdkWindow       *window,
					 BtkStateType     state_type,
					 BtkShadowType    shadow_type,
					 BdkRectangle    *area,
					 BtkWidget       *widget,
					 const gchar     *detail,
					 gint             x,
					 gint             y,
					 gint             width,
					 gint             height);
static void btk_default_draw_option     (BtkStyle        *style,
					 BdkWindow       *window,
					 BtkStateType     state_type,
					 BtkShadowType    shadow_type,
					 BdkRectangle    *area,
					 BtkWidget       *widget,
					 const gchar     *detail,
					 gint             x,
					 gint             y,
					 gint             width,
					 gint             height);
static void btk_default_draw_tab        (BtkStyle        *style,
					 BdkWindow       *window,
					 BtkStateType     state_type,
					 BtkShadowType    shadow_type,
					 BdkRectangle    *area,
					 BtkWidget       *widget,
					 const gchar     *detail,
					 gint             x,
					 gint             y,
					 gint             width,
					 gint             height);
static void btk_default_draw_shadow_gap (BtkStyle        *style,
					 BdkWindow       *window,
					 BtkStateType     state_type,
					 BtkShadowType    shadow_type,
					 BdkRectangle    *area,
					 BtkWidget       *widget,
					 const gchar     *detail,
					 gint             x,
					 gint             y,
					 gint             width,
					 gint             height,
					 BtkPositionType  gap_side,
					 gint             gap_x,
					 gint             gap_width);
static void btk_default_draw_box_gap    (BtkStyle        *style,
					 BdkWindow       *window,
					 BtkStateType     state_type,
					 BtkShadowType    shadow_type,
					 BdkRectangle    *area,
					 BtkWidget       *widget,
					 const gchar     *detail,
					 gint             x,
					 gint             y,
					 gint             width,
					 gint             height,
					 BtkPositionType  gap_side,
					 gint             gap_x,
					 gint             gap_width);
static void btk_default_draw_extension  (BtkStyle        *style,
					 BdkWindow       *window,
					 BtkStateType     state_type,
					 BtkShadowType    shadow_type,
					 BdkRectangle    *area,
					 BtkWidget       *widget,
					 const gchar     *detail,
					 gint             x,
					 gint             y,
					 gint             width,
					 gint             height,
					 BtkPositionType  gap_side);
static void btk_default_draw_focus      (BtkStyle        *style,
					 BdkWindow       *window,
					 BtkStateType     state_type,
					 BdkRectangle    *area,
					 BtkWidget       *widget,
					 const gchar     *detail,
					 gint             x,
					 gint             y,
					 gint             width,
					 gint             height);
static void btk_default_draw_slider     (BtkStyle        *style,
					 BdkWindow       *window,
					 BtkStateType     state_type,
					 BtkShadowType    shadow_type,
					 BdkRectangle    *area,
					 BtkWidget       *widget,
					 const gchar     *detail,
					 gint             x,
					 gint             y,
					 gint             width,
					 gint             height,
					 BtkOrientation   orientation);
static void btk_default_draw_handle     (BtkStyle        *style,
					 BdkWindow       *window,
					 BtkStateType     state_type,
					 BtkShadowType    shadow_type,
					 BdkRectangle    *area,
					 BtkWidget       *widget,
					 const gchar     *detail,
					 gint             x,
					 gint             y,
					 gint             width,
					 gint             height,
					 BtkOrientation   orientation);
static void btk_default_draw_expander   (BtkStyle        *style,
                                         BdkWindow       *window,
                                         BtkStateType     state_type,
                                         BdkRectangle    *area,
                                         BtkWidget       *widget,
                                         const gchar     *detail,
                                         gint             x,
                                         gint             y,
					 BtkExpanderStyle expander_style);
static void btk_default_draw_layout     (BtkStyle        *style,
                                         BdkWindow       *window,
                                         BtkStateType     state_type,
					 gboolean         use_text,
                                         BdkRectangle    *area,
                                         BtkWidget       *widget,
                                         const gchar     *detail,
                                         gint             x,
                                         gint             y,
                                         BangoLayout     *layout);
static void btk_default_draw_resize_grip (BtkStyle       *style,
                                          BdkWindow      *window,
                                          BtkStateType    state_type,
                                          BdkRectangle   *area,
                                          BtkWidget      *widget,
                                          const gchar    *detail,
                                          BdkWindowEdge   edge,
                                          gint            x,
                                          gint            y,
                                          gint            width,
                                          gint            height);
static void btk_default_draw_spinner     (BtkStyle       *style,
					  BdkWindow      *window,
					  BtkStateType    state_type,
                                          BdkRectangle   *area,
                                          BtkWidget      *widget,
                                          const gchar    *detail,
					  guint           step,
					  gint            x,
					  gint            y,
					  gint            width,
					  gint            height);

static void rgb_to_hls			(gdouble	 *r,
					 gdouble	 *g,
					 gdouble	 *b);
static void hls_to_rgb			(gdouble	 *h,
					 gdouble	 *l,
					 gdouble	 *s);

static void style_unrealize_cursor_gcs (BtkStyle *style);

static BdkFont *btk_style_get_font_internal (BtkStyle *style);

/*
 * Data for default check and radio buttons
 */

static const BtkRequisition default_option_indicator_size = { 7, 13 };
static const BtkBorder default_option_indicator_spacing = { 7, 5, 2, 2 };

#define BTK_GRAY		0xdcdc, 0xdada, 0xd5d5
#define BTK_DARK_GRAY		0xc4c4, 0xc2c2, 0xbdbd
#define BTK_LIGHT_GRAY		0xeeee, 0xebeb, 0xe7e7
#define BTK_WHITE		0xffff, 0xffff, 0xffff
#define BTK_BLUE		0x4b4b, 0x6969, 0x8383
#define BTK_VERY_DARK_GRAY	0x9c9c, 0x9a9a, 0x9494
#define BTK_BLACK		0x0000, 0x0000, 0x0000
#define BTK_WEAK_GRAY		0x7530, 0x7530, 0x7530

/* --- variables --- */
static const BdkColor btk_default_normal_fg =      { 0, BTK_BLACK };
static const BdkColor btk_default_active_fg =      { 0, BTK_BLACK };
static const BdkColor btk_default_prelight_fg =    { 0, BTK_BLACK };
static const BdkColor btk_default_selected_fg =    { 0, BTK_WHITE };
static const BdkColor btk_default_insensitive_fg = { 0, BTK_WEAK_GRAY };

static const BdkColor btk_default_normal_bg =      { 0, BTK_GRAY };
static const BdkColor btk_default_active_bg =      { 0, BTK_DARK_GRAY };
static const BdkColor btk_default_prelight_bg =    { 0, BTK_LIGHT_GRAY };
static const BdkColor btk_default_selected_bg =    { 0, BTK_BLUE };
static const BdkColor btk_default_insensitive_bg = { 0, BTK_GRAY };
static const BdkColor btk_default_selected_base =  { 0, BTK_BLUE };
static const BdkColor btk_default_active_base =    { 0, BTK_VERY_DARK_GRAY };

/* --- signals --- */
static guint realize_signal = 0;
static guint unrealize_signal = 0;

G_DEFINE_TYPE (BtkStyle, btk_style, G_TYPE_OBJECT)

/* --- functions --- */

/**
 * _btk_style_init_for_settings:
 * @style: a #BtkStyle
 * @settings: a #BtkSettings
 * 
 * Initializes the font description in @style according to the default
 * font name of @settings. This is called for btk_style_new() with
 * the settings for the default screen (if any); if we are creating
 * a style for a particular screen, we then call it again in a
 * location where we know the correct settings.
 * The reason for this is that btk_rc_style_create_style() doesn't
 * take the screen for an argument.
 **/
void
_btk_style_init_for_settings (BtkStyle    *style,
			      BtkSettings *settings)
{
  const gchar *font_name = _btk_rc_context_get_default_font_name (settings);

  if (style->font_desc)
    bango_font_description_free (style->font_desc);
  
  style->font_desc = bango_font_description_from_string (font_name);
      
  if (!bango_font_description_get_family (style->font_desc))
    {
      g_warning ("Default font does not have a family set");
      bango_font_description_set_family (style->font_desc, "Sans");
    }
  if (bango_font_description_get_size (style->font_desc) <= 0)
    {
      g_warning ("Default font does not have a positive size");
      bango_font_description_set_size (style->font_desc, 10 * BANGO_SCALE);
    }
}

static void
btk_style_init (BtkStyle *style)
{
  gint i;
  
  BtkSettings *settings = btk_settings_get_default ();
  
  if (settings)
    _btk_style_init_for_settings (style, settings);
  else
    style->font_desc = bango_font_description_from_string ("Sans 10");
  
  style->attach_count = 0;
  style->colormap = NULL;
  style->depth = -1;
  
  style->black.red = 0;
  style->black.green = 0;
  style->black.blue = 0;
  
  style->white.red = 65535;
  style->white.green = 65535;
  style->white.blue = 65535;
  
  style->black_gc = NULL;
  style->white_gc = NULL;
  
  style->fg[BTK_STATE_NORMAL] = btk_default_normal_fg;
  style->fg[BTK_STATE_ACTIVE] = btk_default_active_fg;
  style->fg[BTK_STATE_PRELIGHT] = btk_default_prelight_fg;
  style->fg[BTK_STATE_SELECTED] = btk_default_selected_fg;
  style->fg[BTK_STATE_INSENSITIVE] = btk_default_insensitive_fg;
  
  style->bg[BTK_STATE_NORMAL] = btk_default_normal_bg;
  style->bg[BTK_STATE_ACTIVE] = btk_default_active_bg;
  style->bg[BTK_STATE_PRELIGHT] = btk_default_prelight_bg;
  style->bg[BTK_STATE_SELECTED] = btk_default_selected_bg;
  style->bg[BTK_STATE_INSENSITIVE] = btk_default_insensitive_bg;
  
  for (i = 0; i < 4; i++)
    {
      style->text[i] = style->fg[i];
      style->base[i] = style->white;
    }

  style->base[BTK_STATE_SELECTED] = btk_default_selected_base;
  style->text[BTK_STATE_SELECTED] = style->white;
  style->base[BTK_STATE_ACTIVE] = btk_default_active_base;
  style->text[BTK_STATE_ACTIVE] = style->white;
  style->base[BTK_STATE_INSENSITIVE] = btk_default_prelight_bg;
  style->text[BTK_STATE_INSENSITIVE] = btk_default_insensitive_fg;
  
  for (i = 0; i < 5; i++)
    style->bg_pixmap[i] = NULL;
  
  style->rc_style = NULL;
  
  for (i = 0; i < 5; i++)
    {
      style->fg_gc[i] = NULL;
      style->bg_gc[i] = NULL;
      style->light_gc[i] = NULL;
      style->dark_gc[i] = NULL;
      style->mid_gc[i] = NULL;
      style->text_gc[i] = NULL;
      style->base_gc[i] = NULL;
      style->text_aa_gc[i] = NULL;
    }

  style->xthickness = 2;
  style->ythickness = 2;

  style->property_cache = NULL;
}

static void
btk_style_class_init (BtkStyleClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  object_class->finalize = btk_style_finalize;

  klass->clone = btk_style_real_clone;
  klass->copy = btk_style_real_copy;
  klass->init_from_rc = btk_style_real_init_from_rc;
  klass->realize = btk_style_real_realize;
  klass->unrealize = btk_style_real_unrealize;
  klass->set_background = btk_style_real_set_background;
  klass->render_icon = btk_default_render_icon;

  klass->draw_hline = btk_default_draw_hline;
  klass->draw_vline = btk_default_draw_vline;
  klass->draw_shadow = btk_default_draw_shadow;
  klass->draw_polygon = btk_default_draw_polygon;
  klass->draw_arrow = btk_default_draw_arrow;
  klass->draw_diamond = btk_default_draw_diamond;
  klass->draw_string = btk_default_draw_string;
  klass->draw_box = btk_default_draw_box;
  klass->draw_flat_box = btk_default_draw_flat_box;
  klass->draw_check = btk_default_draw_check;
  klass->draw_option = btk_default_draw_option;
  klass->draw_tab = btk_default_draw_tab;
  klass->draw_shadow_gap = btk_default_draw_shadow_gap;
  klass->draw_box_gap = btk_default_draw_box_gap;
  klass->draw_extension = btk_default_draw_extension;
  klass->draw_focus = btk_default_draw_focus;
  klass->draw_slider = btk_default_draw_slider;
  klass->draw_handle = btk_default_draw_handle;
  klass->draw_expander = btk_default_draw_expander;
  klass->draw_layout = btk_default_draw_layout;
  klass->draw_resize_grip = btk_default_draw_resize_grip;
  klass->draw_spinner = btk_default_draw_spinner;

  g_type_class_add_private (object_class, sizeof (BtkStylePrivate));

  /**
   * BtkStyle::realize:
   * @style: the object which received the signal
   *
   * Emitted when the style has been initialized for a particular
   * colormap and depth. Connecting to this signal is probably seldom
   * useful since most of the time applications and widgets only
   * deal with styles that have been already realized.
   *
   * Since: 2.4
   */
  realize_signal = g_signal_new (I_("realize"),
				 G_TYPE_FROM_CLASS (object_class),
				 G_SIGNAL_RUN_FIRST,
				 G_STRUCT_OFFSET (BtkStyleClass, realize),
				 NULL, NULL,
				 _btk_marshal_VOID__VOID,
				 G_TYPE_NONE, 0);
  /**
   * BtkStyle::unrealize:
   * @style: the object which received the signal
   *
   * Emitted when the aspects of the style specific to a particular colormap
   * and depth are being cleaned up. A connection to this signal can be useful
   * if a widget wants to cache objects like a #BdkGC as object data on #BtkStyle.
   * This signal provides a convenient place to free such cached objects.
   *
   * Since: 2.4
   */
  unrealize_signal = g_signal_new (I_("unrealize"),
				   G_TYPE_FROM_CLASS (object_class),
				   G_SIGNAL_RUN_FIRST,
				   G_STRUCT_OFFSET (BtkStyleClass, unrealize),
				   NULL, NULL,
				   _btk_marshal_VOID__VOID,
				   G_TYPE_NONE, 0);
}

static void
clear_property_cache (BtkStyle *style)
{
  if (style->property_cache)
    {
      guint i;

      for (i = 0; i < style->property_cache->len; i++)
	{
	  PropertyValue *node = &g_array_index (style->property_cache, PropertyValue, i);

	  g_param_spec_unref (node->pspec);
	  g_value_unset (&node->value);
	}
      g_array_free (style->property_cache, TRUE);
      style->property_cache = NULL;
    }
}

static void
btk_style_finalize (GObject *object)
{
  BtkStyle *style = BTK_STYLE (object);
  BtkStylePrivate *priv = BTK_STYLE_GET_PRIVATE (style);

  g_return_if_fail (style->attach_count == 0);

  clear_property_cache (style);
  
  /* All the styles in the list have the same 
   * style->styles pointer. If we delete the 
   * *first* style from the list, we need to update
   * the style->styles pointers from all the styles.
   * Otherwise we simply remove the node from
   * the list.
   */
  if (style->styles)
    {
      if (style->styles->data != style)
        style->styles = g_slist_remove (style->styles, style);
      else
        {
          GSList *tmp_list = style->styles->next;
	  
          while (tmp_list)
            {
              BTK_STYLE (tmp_list->data)->styles = style->styles->next;
              tmp_list = tmp_list->next;
            }
          g_slist_free_1 (style->styles);
        }
    }

  g_slist_foreach (style->icon_factories, (GFunc) g_object_unref, NULL);
  g_slist_free (style->icon_factories);

  g_slist_foreach (priv->color_hashes, (GFunc) g_hash_table_unref, NULL);
  g_slist_free (priv->color_hashes);

  bango_font_description_free (style->font_desc);
  
  if (style->private_font)
    bdk_font_unref (style->private_font);

  if (style->private_font_desc)
    bango_font_description_free (style->private_font_desc);

  if (style->rc_style)
    g_object_unref (style->rc_style);

  G_OBJECT_CLASS (btk_style_parent_class)->finalize (object);
}


/**
 * btk_style_copy:
 * @style: a #BtkStyle
 *
 * Creates a copy of the passed in #BtkStyle object.
 *
 * Returns: (transfer full): a copy of @style
 */
BtkStyle*
btk_style_copy (BtkStyle *style)
{
  BtkStyle *new_style;
  
  g_return_val_if_fail (BTK_IS_STYLE (style), NULL);
  
  new_style = BTK_STYLE_GET_CLASS (style)->clone (style);
  BTK_STYLE_GET_CLASS (style)->copy (new_style, style);

  return new_style;
}

static BtkStyle*
btk_style_duplicate (BtkStyle *style)
{
  BtkStyle *new_style;
  
  g_return_val_if_fail (BTK_IS_STYLE (style), NULL);
  
  new_style = btk_style_copy (style);
  
  /* All the styles in the list have the same 
   * style->styles pointer. When we insert a new 
   * style, we append it to the list to avoid having 
   * to update the existing ones. 
   */
  style->styles = g_slist_append (style->styles, new_style);
  new_style->styles = style->styles;  
  
  return new_style;
}

/**
 * btk_style_new:
 * @returns: a new #BtkStyle.
 *
 * Creates a new #BtkStyle.
 **/
BtkStyle*
btk_style_new (void)
{
  BtkStyle *style;
  
  style = g_object_new (BTK_TYPE_STYLE, NULL);
  
  return style;
}

/**
 * btk_style_attach:
 * @style: a #BtkStyle.
 * @window: a #BdkWindow.
 *
 * Attaches a style to a window; this process allocates the
 * colors and creates the GC's for the style - it specializes
 * it to a particular visual and colormap. The process may
 * involve the creation of a new style if the style has already
 * been attached to a window with a different style and colormap.
 *
 * Since this function may return a new object, you have to use it
 * in the following way:
 * <literal>style = btk_style_attach (style, window)</literal>
 *
 * Returns: Either @style, or a newly-created #BtkStyle.
 *   If the style is newly created, the style parameter
 *   will be unref'ed, and the new style will have
 *   a reference count belonging to the caller.
 */
BtkStyle*
btk_style_attach (BtkStyle  *style,
                  BdkWindow *window)
{
  GSList *styles;
  BtkStyle *new_style = NULL;
  BdkColormap *colormap;
  
  g_return_val_if_fail (BTK_IS_STYLE (style), NULL);
  g_return_val_if_fail (window != NULL, NULL);
  
  colormap = bdk_drawable_get_colormap (window);
  
  if (!style->styles)
    style->styles = g_slist_append (NULL, style);
  
  styles = style->styles;
  while (styles)
    {
      new_style = styles->data;
      
      if (new_style->colormap == colormap)
        break;

      new_style = NULL;
      styles = styles->next;
    }

  if (!new_style)
    {
      styles = style->styles;
      
      while (styles)
	{
	  new_style = styles->data;
	  
	  if (new_style->attach_count == 0)
	    {
	      btk_style_realize (new_style, colormap);
	      break;
	    }
	  
	  new_style = NULL;
	  styles = styles->next;
	}
    }
  
  if (!new_style)
    {
      new_style = btk_style_duplicate (style);
      if (bdk_colormap_get_screen (style->colormap) != bdk_colormap_get_screen (colormap) &&
	  new_style->private_font)
	{
	  bdk_font_unref (new_style->private_font);
	  new_style->private_font = NULL;
	}
      btk_style_realize (new_style, colormap);
    }

  /* A style gets a refcount from being attached */
  if (new_style->attach_count == 0)
    g_object_ref (new_style);

  /* Another refcount belongs to the parent */
  if (style != new_style) 
    {
      g_object_unref (style);
      g_object_ref (new_style);
    }
  
  new_style->attach_count++;
  
  return new_style;
}

/**
 * btk_style_detach:
 * @style: a #BtkStyle
 *
 * Detaches a style from a window. If the style is not attached
 * to any windows anymore, it is unrealized. See btk_style_attach().
 * 
 */
void
btk_style_detach (BtkStyle *style)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (style->attach_count > 0);
  
  style->attach_count -= 1;
  if (style->attach_count == 0)
    {
      g_signal_emit (style, unrealize_signal, 0);
      
      g_object_unref (style->colormap);
      style->colormap = NULL;

      if (style->private_font_desc)
	{
	  if (style->private_font)
	    {
	      bdk_font_unref (style->private_font);
	      style->private_font = NULL;
	    }
	  
	  bango_font_description_free (style->private_font_desc);
	  style->private_font_desc = NULL;
	}

      g_object_unref (style);
    }
}

/**
 * btk_style_ref:
 * @style: a #BtkStyle.
 * @returns: @style.
 *
 * Increase the reference count of @style.
 * 
 * Deprecated: 2.0: use g_object_ref() instead.
 */
BtkStyle*
btk_style_ref (BtkStyle *style)
{
  return (BtkStyle *) g_object_ref (style);
}

/**
 * btk_style_unref:
 * @style: a #BtkStyle.
 *
 * Decrease the reference count of @style.
 * 
 * Deprecated: 2.0: use g_object_unref() instead.
 */
void
btk_style_unref (BtkStyle *style)
{
  g_object_unref (style);
}

static void
btk_style_realize (BtkStyle    *style,
                   BdkColormap *colormap)
{
  style->colormap = g_object_ref (colormap);
  style->depth = bdk_colormap_get_visual (colormap)->depth;

  g_signal_emit (style, realize_signal, 0);
}

/**
 * btk_style_lookup_icon_set:
 * @style: a #BtkStyle
 * @stock_id: an icon name
 *
 * Looks up @stock_id in the icon factories associated with @style
 * and the default icon factory, returning an icon set if found,
 * otherwise %NULL.
 *
 * Return value: (transfer none): icon set of @stock_id
 */
BtkIconSet*
btk_style_lookup_icon_set (BtkStyle   *style,
                           const char *stock_id)
{
  GSList *iter;

  g_return_val_if_fail (BTK_IS_STYLE (style), NULL);
  g_return_val_if_fail (stock_id != NULL, NULL);
  
  iter = style->icon_factories;
  while (iter != NULL)
    {
      BtkIconSet *icon_set = btk_icon_factory_lookup (BTK_ICON_FACTORY (iter->data),
						      stock_id);
      if (icon_set)
        return icon_set;
      
      iter = g_slist_next (iter);
    }

  return btk_icon_factory_lookup_default (stock_id);
}

/**
 * btk_style_lookup_color:
 * @style: a #BtkStyle
 * @color_name: the name of the logical color to look up
 * @color: (out): the #BdkColor to fill in
 *
 * Looks up @color_name in the style's logical color mappings,
 * filling in @color and returning %TRUE if found, otherwise
 * returning %FALSE. Do not cache the found mapping, because
 * it depends on the #BtkStyle and might change when a theme
 * switch occurs.
 *
 * Return value: %TRUE if the mapping was found.
 *
 * Since: 2.10
 **/
gboolean
btk_style_lookup_color (BtkStyle   *style,
                        const char *color_name,
                        BdkColor   *color)
{
  BtkStylePrivate *priv;
  GSList *iter;

  g_return_val_if_fail (BTK_IS_STYLE (style), FALSE);
  g_return_val_if_fail (color_name != NULL, FALSE);
  g_return_val_if_fail (color != NULL, FALSE);

  priv = BTK_STYLE_GET_PRIVATE (style);

  for (iter = priv->color_hashes; iter != NULL; iter = iter->next)
    {
      GHashTable *hash    = iter->data;
      BdkColor   *mapping = g_hash_table_lookup (hash, color_name);

      if (mapping)
        {
          color->red = mapping->red;
          color->green = mapping->green;
          color->blue = mapping->blue;
          return TRUE;
        }
    }

  return FALSE;
}

/**
 * btk_draw_hline:
 * @style: a #BtkStyle
 * @window: a #BdkWindow
 * @state_type: a state
 * @x1: the starting x coordinate
 * @x2: the ending x coordinate
 * @y: the y coordinate
 * 
 * Draws a horizontal line from (@x1, @y) to (@x2, @y) in @window
 * using the given style and state.
 * 
 * Deprecated: 2.0: Use btk_paint_hline() instead.
 **/
void
btk_draw_hline (BtkStyle     *style,
                BdkWindow    *window,
                BtkStateType  state_type,
                gint          x1,
                gint          x2,
                gint          y)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (BTK_STYLE_GET_CLASS (style)->draw_hline != NULL);
  
  BTK_STYLE_GET_CLASS (style)->draw_hline (style, window, state_type, NULL, NULL, NULL, x1, x2, y);
}


/**
 * btk_draw_vline:
 * @style: a #BtkStyle
 * @window: a #BdkWindow
 * @state_type: a state
 * @y1_: the starting y coordinate
 * @y2_: the ending y coordinate
 * @x: the x coordinate
 * 
 * Draws a vertical line from (@x, @y1_) to (@x, @y2_) in @window
 * using the given style and state.
 * 
 * Deprecated: 2.0: Use btk_paint_vline() instead.
 **/
void
btk_draw_vline (BtkStyle     *style,
                BdkWindow    *window,
                BtkStateType  state_type,
                gint          y1_,
                gint          y2_,
                gint          x)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (BTK_STYLE_GET_CLASS (style)->draw_vline != NULL);
  
  BTK_STYLE_GET_CLASS (style)->draw_vline (style, window, state_type, NULL, NULL, NULL, y1_, y2_, x);
}

/**
 * btk_draw_shadow:
 * @style: a #BtkStyle
 * @window: a #BdkWindow
 * @state_type: a state
 * @shadow_type: type of shadow to draw
 * @x: x origin of the rectangle
 * @y: y origin of the rectangle
 * @width: width of the rectangle 
 * @height: width of the rectangle 
 *
 * Draws a shadow around the given rectangle in @window 
 * using the given style and state and shadow type.
 * 
 * Deprecated: 2.0: Use btk_paint_shadow() instead.
 */
void
btk_draw_shadow (BtkStyle      *style,
                 BdkWindow     *window,
                 BtkStateType   state_type,
                 BtkShadowType  shadow_type,
                 gint           x,
                 gint           y,
                 gint           width,
                 gint           height)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (BTK_STYLE_GET_CLASS (style)->draw_shadow != NULL);
  
  BTK_STYLE_GET_CLASS (style)->draw_shadow (style, window, state_type, shadow_type, NULL, NULL, NULL, x, y, width, height);
}

/**
 * btk_draw_polygon:
 * @style: a #BtkStyle
 * @window: a #BdkWindow
 * @state_type: a state
 * @shadow_type: type of shadow to draw
 * @points: an array of #BdkPoint<!-- -->s
 * @npoints: length of @points
 * @fill: %TRUE if the polygon should be filled
 * 
 * Draws a polygon on @window with the given parameters.
 *
 * Deprecated: 2.0: Use btk_paint_polygon() instead.
 */ 
void
btk_draw_polygon (BtkStyle      *style,
                  BdkWindow     *window,
                  BtkStateType   state_type,
                  BtkShadowType  shadow_type,
                  BdkPoint      *points,
                  gint           npoints,
                  gboolean       fill)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (BTK_STYLE_GET_CLASS (style)->draw_polygon != NULL);
  
  BTK_STYLE_GET_CLASS (style)->draw_polygon (style, window, state_type, shadow_type, NULL, NULL, NULL, points, npoints, fill);
}

/**
 * btk_draw_arrow:
 * @style: a #BtkStyle
 * @window: a #BdkWindow
 * @state_type: a state
 * @shadow_type: the type of shadow to draw
 * @arrow_type: the type of arrow to draw
 * @fill: %TRUE if the arrow tip should be filled
 * @x: x origin of the rectangle to draw the arrow in
 * @y: y origin of the rectangle to draw the arrow in
 * @width: width of the rectangle to draw the arrow in
 * @height: height of the rectangle to draw the arrow in
 * 
 * Draws an arrow in the given rectangle on @window using the given 
 * parameters. @arrow_type determines the direction of the arrow.
 *
 * Deprecated: 2.0: Use btk_paint_arrow() instead.
 */
void
btk_draw_arrow (BtkStyle      *style,
                BdkWindow     *window,
                BtkStateType   state_type,
                BtkShadowType  shadow_type,
                BtkArrowType   arrow_type,
                gboolean       fill,
                gint           x,
                gint           y,
                gint           width,
                gint           height)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (BTK_STYLE_GET_CLASS (style)->draw_arrow != NULL);
  
  BTK_STYLE_GET_CLASS (style)->draw_arrow (style, window, state_type, shadow_type, NULL, NULL, NULL, arrow_type, fill, x, y, width, height);
}

/**
 * btk_draw_diamond:
 * @style: a #BtkStyle
 * @window: a #BdkWindow
 * @state_type: a state
 * @shadow_type: the type of shadow to draw
 * @x: x origin of the rectangle to draw the diamond in
 * @y: y origin of the rectangle to draw the diamond in
 * @width: width of the rectangle to draw the diamond in
 * @height: height of the rectangle to draw the diamond in
 *
 * Draws a diamond in the given rectangle on @window using the given
 * parameters.
 *
 * Deprecated: 2.0: Use btk_paint_diamond() instead.
 */
void
btk_draw_diamond (BtkStyle      *style,
                  BdkWindow     *window,
                  BtkStateType   state_type,
                  BtkShadowType  shadow_type,
                  gint           x,
                  gint           y,
                  gint           width,
                  gint           height)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (BTK_STYLE_GET_CLASS (style)->draw_diamond != NULL);
  
  BTK_STYLE_GET_CLASS (style)->draw_diamond (style, window, state_type, shadow_type, NULL, NULL, NULL, x, y, width, height);
}

/**
 * btk_draw_string:
 * @style: a #BtkStyle
 * @window: a #BdkWindow
 * @state_type: a state
 * @x: x origin
 * @y: y origin
 * @string: the string to draw
 * 
 * Draws a text string on @window with the given parameters.
 *
 * Deprecated: 2.0: Use btk_paint_layout() instead.
 */
void
btk_draw_string (BtkStyle      *style,
                 BdkWindow     *window,
                 BtkStateType   state_type,
                 gint           x,
                 gint           y,
                 const gchar   *string)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (BTK_STYLE_GET_CLASS (style)->draw_string != NULL);
  
  BTK_STYLE_GET_CLASS (style)->draw_string (style, window, state_type, NULL, NULL, NULL, x, y, string);
}

/**
 * btk_draw_box:
 * @style: a #BtkStyle
 * @window: a #BdkWindow
 * @state_type: a state
 * @shadow_type: the type of shadow to draw
 * @x: x origin of the box
 * @y: y origin of the box
 * @width: the width of the box
 * @height: the height of the box
 * 
 * Draws a box on @window with the given parameters.
 *
 * Deprecated: 2.0: Use btk_paint_box() instead.
 */
void
btk_draw_box (BtkStyle      *style,
              BdkWindow     *window,
              BtkStateType   state_type,
              BtkShadowType  shadow_type,
              gint           x,
              gint           y,
              gint           width,
              gint           height)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (BTK_STYLE_GET_CLASS (style)->draw_box != NULL);
  
  BTK_STYLE_GET_CLASS (style)->draw_box (style, window, state_type, shadow_type, NULL, NULL, NULL, x, y, width, height);
}

/**
 * btk_draw_flat_box:
 * @style: a #BtkStyle
 * @window: a #BdkWindow
 * @state_type: a state
 * @shadow_type: the type of shadow to draw
 * @x: x origin of the box
 * @y: y origin of the box
 * @width: the width of the box
 * @height: the height of the box
 * 
 * Draws a flat box on @window with the given parameters.
 *
 * Deprecated: 2.0: Use btk_paint_flat_box() instead.
 */
void
btk_draw_flat_box (BtkStyle      *style,
                   BdkWindow     *window,
                   BtkStateType   state_type,
                   BtkShadowType  shadow_type,
                   gint           x,
                   gint           y,
                   gint           width,
                   gint           height)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (BTK_STYLE_GET_CLASS (style)->draw_flat_box != NULL);
  
  BTK_STYLE_GET_CLASS (style)->draw_flat_box (style, window, state_type, shadow_type, NULL, NULL, NULL, x, y, width, height);
}

/**
 * btk_draw_check:
 * @style: a #BtkStyle
 * @window: a #BdkWindow
 * @state_type: a state
 * @shadow_type: the type of shadow to draw
 * @x: x origin of the rectangle to draw the check in
 * @y: y origin of the rectangle to draw the check in
 * @width: the width of the rectangle to draw the check in
 * @height: the height of the rectangle to draw the check in
 * 
 * Draws a check button indicator in the given rectangle on @window with 
 * the given parameters.
 *
 * Deprecated: 2.0: Use btk_paint_check() instead.
 */
void
btk_draw_check (BtkStyle      *style,
                BdkWindow     *window,
                BtkStateType   state_type,
                BtkShadowType  shadow_type,
                gint           x,
                gint           y,
                gint           width,
                gint           height)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (BTK_STYLE_GET_CLASS (style)->draw_check != NULL);
  
  BTK_STYLE_GET_CLASS (style)->draw_check (style, window, state_type, shadow_type, NULL, NULL, NULL, x, y, width, height);
}

/**
 * btk_draw_option:
 * @style: a #BtkStyle
 * @window: a #BdkWindow
 * @state_type: a state
 * @shadow_type: the type of shadow to draw
 * @x: x origin of the rectangle to draw the option in
 * @y: y origin of the rectangle to draw the option in
 * @width: the width of the rectangle to draw the option in
 * @height: the height of the rectangle to draw the option in
 *
 * Draws a radio button indicator in the given rectangle on @window with 
 * the given parameters.
 *
 * Deprecated: 2.0: Use btk_paint_option() instead.
 */
void
btk_draw_option (BtkStyle      *style,
		 BdkWindow     *window,
		 BtkStateType   state_type,
		 BtkShadowType  shadow_type,
		 gint           x,
		 gint           y,
		 gint           width,
		 gint           height)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (BTK_STYLE_GET_CLASS (style)->draw_option != NULL);
  
  BTK_STYLE_GET_CLASS (style)->draw_option (style, window, state_type, shadow_type, NULL, NULL, NULL, x, y, width, height);
}

/**
 * btk_draw_tab:
 * @style: a #BtkStyle
 * @window: a #BdkWindow
 * @state_type: a state
 * @shadow_type: the type of shadow to draw
 * @x: x origin of the rectangle to draw the tab in
 * @y: y origin of the rectangle to draw the tab in
 * @width: the width of the rectangle to draw the tab in
 * @height: the height of the rectangle to draw the tab in
 *
 * Draws an option menu tab (i.e. the up and down pointing arrows)
 * in the given rectangle on @window using the given parameters.
 * 
 * Deprecated: 2.0: Use btk_paint_tab() instead.
 */ 
void
btk_draw_tab (BtkStyle      *style,
	      BdkWindow     *window,
	      BtkStateType   state_type,
	      BtkShadowType  shadow_type,
	      gint           x,
	      gint           y,
	      gint           width,
	      gint           height)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (BTK_STYLE_GET_CLASS (style)->draw_tab != NULL);
  
  BTK_STYLE_GET_CLASS (style)->draw_tab (style, window, state_type, shadow_type, NULL, NULL, NULL, x, y, width, height);
}

/**
 * btk_draw_shadow_gap:
 * @style: a #BtkStyle
 * @window: a #BdkWindow
 * @state_type: a state
 * @shadow_type: type of shadow to draw
 * @x: x origin of the rectangle
 * @y: y origin of the rectangle
 * @width: width of the rectangle 
 * @height: width of the rectangle 
 * @gap_side: side in which to leave the gap
 * @gap_x: starting position of the gap
 * @gap_width: width of the gap
 *
 * Draws a shadow around the given rectangle in @window 
 * using the given style and state and shadow type, leaving a 
 * gap in one side.
 * 
 * Deprecated: 2.0: Use btk_paint_shadow_gap() instead.
*/
void
btk_draw_shadow_gap (BtkStyle       *style,
                     BdkWindow      *window,
                     BtkStateType    state_type,
                     BtkShadowType   shadow_type,
                     gint            x,
                     gint            y,
                     gint            width,
                     gint            height,
                     BtkPositionType gap_side,
                     gint            gap_x,
                     gint            gap_width)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (BTK_STYLE_GET_CLASS (style)->draw_shadow_gap != NULL);
  
  BTK_STYLE_GET_CLASS (style)->draw_shadow_gap (style, window, state_type, shadow_type, NULL, NULL, NULL, x, y, width, height, gap_side, gap_x, gap_width);
}

/**
 * btk_draw_box_gap:
 * @style: a #BtkStyle
 * @window: a #BdkWindow
 * @state_type: a state
 * @shadow_type: type of shadow to draw
 * @x: x origin of the rectangle
 * @y: y origin of the rectangle
 * @width: width of the rectangle 
 * @height: width of the rectangle 
 * @gap_side: side in which to leave the gap
 * @gap_x: starting position of the gap
 * @gap_width: width of the gap
 *
 * Draws a box in @window using the given style and state and shadow type, 
 * leaving a gap in one side.
 * 
 * Deprecated: 2.0: Use btk_paint_box_gap() instead.
 */
void
btk_draw_box_gap (BtkStyle       *style,
                  BdkWindow      *window,
                  BtkStateType    state_type,
                  BtkShadowType   shadow_type,
                  gint            x,
                  gint            y,
                  gint            width,
                  gint            height,
                  BtkPositionType gap_side,
                  gint            gap_x,
                  gint            gap_width)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (BTK_STYLE_GET_CLASS (style)->draw_box_gap != NULL);
  
  BTK_STYLE_GET_CLASS (style)->draw_box_gap (style, window, state_type, shadow_type, NULL, NULL, NULL, x, y, width, height, gap_side, gap_x, gap_width);
}

/**
 * btk_draw_extension: 
 * @style: a #BtkStyle
 * @window: a #BdkWindow
 * @state_type: a state
 * @shadow_type: type of shadow to draw
 * @x: x origin of the extension
 * @y: y origin of the extension
 * @width: width of the extension 
 * @height: width of the extension 
 * @gap_side: the side on to which the extension is attached
 * 
 * Draws an extension, i.e. a notebook tab.
 *
 * Deprecated: 2.0: Use btk_paint_extension() instead.
 **/
void
btk_draw_extension (BtkStyle       *style,
                    BdkWindow      *window,
                    BtkStateType    state_type,
                    BtkShadowType   shadow_type,
                    gint            x,
                    gint            y,
                    gint            width,
                    gint            height,
                    BtkPositionType gap_side)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (BTK_STYLE_GET_CLASS (style)->draw_extension != NULL);
  
  BTK_STYLE_GET_CLASS (style)->draw_extension (style, window, state_type, shadow_type, NULL, NULL, NULL, x, y, width, height, gap_side);
}

/**
 * btk_draw_focus:
 * @style: a #BtkStyle
 * @window: a #BdkWindow
 * @x: the x origin of the rectangle around which to draw a focus indicator
 * @y: the y origin of the rectangle around which to draw a focus indicator
 * @width: the width of the rectangle around which to draw a focus indicator
 * @height: the height of the rectangle around which to draw a focus indicator
 *
 * Draws a focus indicator around the given rectangle on @window using the
 * given style.
 * 
 * Deprecated: 2.0: Use btk_paint_focus() instead.
 */
void
btk_draw_focus (BtkStyle      *style,
		BdkWindow     *window,
		gint           x,
		gint           y,
		gint           width,
		gint           height)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (BTK_STYLE_GET_CLASS (style)->draw_focus != NULL);
  
  BTK_STYLE_GET_CLASS (style)->draw_focus (style, window, BTK_STATE_NORMAL, NULL, NULL, NULL, x, y, width, height);
}

/**
 * btk_draw_slider:
 * @style: a #BtkStyle
  @window: a #BdkWindow
 * @state_type: a state
 * @shadow_type: a shadow
 * @x: the x origin of the rectangle in which to draw a slider
 * @y: the y origin of the rectangle in which to draw a slider
 * @width: the width of the rectangle in which to draw a slider
 * @height: the height of the rectangle in which to draw a slider
 * @orientation: the orientation to be used
 *
 * Draws a slider in the given rectangle on @window using the
 * given style and orientation.
 */
void 
btk_draw_slider (BtkStyle      *style,
		 BdkWindow     *window,
		 BtkStateType   state_type,
		 BtkShadowType  shadow_type,
		 gint           x,
		 gint           y,
		 gint           width,
		 gint           height,
		 BtkOrientation orientation)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (BTK_STYLE_GET_CLASS (style)->draw_slider != NULL);
  
  BTK_STYLE_GET_CLASS (style)->draw_slider (style, window, state_type, shadow_type, NULL, NULL, NULL, x, y, width, height, orientation);
}

/**
 * btk_draw_handle:
 * @style: a #BtkStyle
 * @window: a #BdkWindow
 * @state_type: a state
 * @shadow_type: type of shadow to draw
 * @x: x origin of the handle
 * @y: y origin of the handle
 * @width: with of the handle
 * @height: height of the handle
 * @orientation: the orientation of the handle
 * 
 * Draws a handle as used in #BtkHandleBox and #BtkPaned.
 * 
 * Deprecated: 2.0: Use btk_paint_handle() instead.
 **/
void 
btk_draw_handle (BtkStyle      *style,
		 BdkWindow     *window,
		 BtkStateType   state_type,
		 BtkShadowType  shadow_type,
		 gint           x,
		 gint           y,
		 gint           width,
		 gint           height,
		 BtkOrientation orientation)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (BTK_STYLE_GET_CLASS (style)->draw_handle != NULL);
  
  BTK_STYLE_GET_CLASS (style)->draw_handle (style, window, state_type, shadow_type, NULL, NULL, NULL, x, y, width, height, orientation);
}

/**
 * btk_draw_expander:
 * @style: a #BtkStyle
 * @window: a #BdkWindow
 * @state_type: a state
 * @x: the x position to draw the expander at
 * @y: the y position to draw the expander at
 * @expander_style: the style to draw the expander in
 * 
 * Draws an expander as used in #BtkTreeView.
 * 
 * Deprecated: 2.0: Use btk_paint_expander() instead.
 **/
void
btk_draw_expander (BtkStyle        *style,
                   BdkWindow       *window,
                   BtkStateType     state_type,
                   gint             x,
                   gint             y,
		   BtkExpanderStyle expander_style)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (BTK_STYLE_GET_CLASS (style)->draw_expander != NULL);
  
  BTK_STYLE_GET_CLASS (style)->draw_expander (style, window, state_type,
                                              NULL, NULL, NULL,
                                              x, y, expander_style);
}

/**
 * btk_draw_layout:
 * @style: a #BtkStyle
 * @window: a #BdkWindow
 * @state_type: a state
 * @use_text: whether to use the text or foreground
 *            graphics context of @style
 * @x: x origin
 * @y: y origin
 * @layout: the layout to draw
 * 
 * Draws a layout on @window using the given parameters.
 */
void
btk_draw_layout (BtkStyle        *style,
                 BdkWindow       *window,
                 BtkStateType     state_type,
		 gboolean         use_text,
                 gint             x,
                 gint             y,
                 BangoLayout     *layout)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (BTK_STYLE_GET_CLASS (style)->draw_layout != NULL);
  
  BTK_STYLE_GET_CLASS (style)->draw_layout (style, window, state_type, use_text,
                                            NULL, NULL, NULL,
                                            x, y, layout);
}

/**
 * btk_draw_resize_grip:
 * @style: a #BtkStyle
 * @window: a #BdkWindow
 * @state_type: a state
 * @edge: the edge in which to draw the resize grip
 * @x: the x origin of the rectangle in which to draw the resize grip
 * @y: the y origin of the rectangle in which to draw the resize grip
 * @width: the width of the rectangle in which to draw the resize grip
 * @height: the height of the rectangle in which to draw the resize grip
 *
 * Draws a resize grip in the given rectangle on @window using the given
 * parameters. 
 * 
 * Deprecated: 2.0: Use btk_paint_resize_grip() instead.
 */
void
btk_draw_resize_grip (BtkStyle     *style,
                      BdkWindow    *window,
                      BtkStateType  state_type,
                      BdkWindowEdge edge,
                      gint          x,
                      gint          y,
                      gint          width,
                      gint          height)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (BTK_STYLE_GET_CLASS (style)->draw_resize_grip != NULL);

  BTK_STYLE_GET_CLASS (style)->draw_resize_grip (style, window, state_type,
                                                 NULL, NULL, NULL,
                                                 edge,
                                                 x, y, width, height);
}


/**
 * btk_style_set_background:
 * @style: a #BtkStyle
 * @window: a #BdkWindow
 * @state_type: a state
 * 
 * Sets the background of @window to the background color or pixmap
 * specified by @style for the given state.
 */
void
btk_style_set_background (BtkStyle    *style,
                          BdkWindow   *window,
                          BtkStateType state_type)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (window != NULL);
  
  BTK_STYLE_GET_CLASS (style)->set_background (style, window, state_type);
}

/* Default functions */
static BtkStyle *
btk_style_real_clone (BtkStyle *style)
{
  return g_object_new (G_OBJECT_TYPE (style), NULL);
}

static void
btk_style_real_copy (BtkStyle *style,
		     BtkStyle *src)
{
  BtkStylePrivate *priv = BTK_STYLE_GET_PRIVATE (style);
  BtkStylePrivate *src_priv = BTK_STYLE_GET_PRIVATE (src);
  gint i;
  
  for (i = 0; i < 5; i++)
    {
      style->fg[i] = src->fg[i];
      style->bg[i] = src->bg[i];
      style->text[i] = src->text[i];
      style->base[i] = src->base[i];

      if (style->bg_pixmap[i])
	g_object_unref (style->bg_pixmap[i]),
      style->bg_pixmap[i] = src->bg_pixmap[i];
      if (style->bg_pixmap[i])
	g_object_ref (style->bg_pixmap[i]);
    }

  if (style->private_font)
    bdk_font_unref (style->private_font);
  style->private_font = src->private_font;
  if (style->private_font)
    bdk_font_ref (style->private_font);

  if (style->font_desc)
    bango_font_description_free (style->font_desc);
  if (src->font_desc)
    style->font_desc = bango_font_description_copy (src->font_desc);
  else
    style->font_desc = NULL;
  
  style->xthickness = src->xthickness;
  style->ythickness = src->ythickness;

  if (style->rc_style)
    g_object_unref (style->rc_style);
  style->rc_style = src->rc_style;
  if (src->rc_style)
    g_object_ref (src->rc_style);

  g_slist_foreach (style->icon_factories, (GFunc) g_object_unref, NULL);
  g_slist_free (style->icon_factories);
  style->icon_factories = g_slist_copy (src->icon_factories);
  g_slist_foreach (style->icon_factories, (GFunc) g_object_ref, NULL);

  g_slist_foreach (priv->color_hashes, (GFunc) g_hash_table_unref, NULL);
  g_slist_free (priv->color_hashes);
  priv->color_hashes = g_slist_copy (src_priv->color_hashes);
  g_slist_foreach (priv->color_hashes, (GFunc) g_hash_table_ref, NULL);

  /* don't copy, just clear cache */
  clear_property_cache (style);
}

static void
btk_style_real_init_from_rc (BtkStyle   *style,
			     BtkRcStyle *rc_style)
{
  BtkStylePrivate *priv = BTK_STYLE_GET_PRIVATE (style);
  gint i;

  /* cache _should_ be still empty */
  clear_property_cache (style);

  if (rc_style->font_desc)
    bango_font_description_merge (style->font_desc, rc_style->font_desc, TRUE);
    
  for (i = 0; i < 5; i++)
    {
      if (rc_style->color_flags[i] & BTK_RC_FG)
	style->fg[i] = rc_style->fg[i];
      if (rc_style->color_flags[i] & BTK_RC_BG)
	style->bg[i] = rc_style->bg[i];
      if (rc_style->color_flags[i] & BTK_RC_TEXT)
	style->text[i] = rc_style->text[i];
      if (rc_style->color_flags[i] & BTK_RC_BASE)
	style->base[i] = rc_style->base[i];
    }

  if (rc_style->xthickness >= 0)
    style->xthickness = rc_style->xthickness;
  if (rc_style->ythickness >= 0)
    style->ythickness = rc_style->ythickness;

  style->icon_factories = g_slist_copy (rc_style->icon_factories);
  g_slist_foreach (style->icon_factories, (GFunc) g_object_ref, NULL);

  priv->color_hashes = g_slist_copy (_btk_rc_style_get_color_hashes (rc_style));
  g_slist_foreach (priv->color_hashes, (GFunc) g_hash_table_ref, NULL);
}

static gint
style_property_values_cmp (gconstpointer bsearch_node1,
			   gconstpointer bsearch_node2)
{
  const PropertyValue *val1 = bsearch_node1;
  const PropertyValue *val2 = bsearch_node2;

  if (val1->widget_type == val2->widget_type)
    return val1->pspec < val2->pspec ? -1 : val1->pspec == val2->pspec ? 0 : 1;
  else
    return val1->widget_type < val2->widget_type ? -1 : 1;
}

/**
 * btk_style_get_style_property:
 * @style: a #BtkStyle
 * @widget_type: the #GType of a descendant of #BtkWidget
 * @property_name: the name of the style property to get
 * @value: a #GValue where the value of the property being
 *     queried will be stored
 *
 * Queries the value of a style property corresponding to a
 * widget class is in the given style.
 *
 * Since: 2.16
 */
void 
btk_style_get_style_property (BtkStyle     *style,
                              GType        widget_type,
                              const gchar *property_name,
                              GValue      *value)
{
  BtkWidgetClass *klass;
  GParamSpec *pspec;
  BtkRcPropertyParser parser;
  const GValue *peek_value;

  klass = g_type_class_ref (widget_type);
  pspec = btk_widget_class_find_style_property (klass, property_name);
  g_type_class_unref (klass);

  if (!pspec)
    {
      g_warning ("%s: widget class `%s' has no property named `%s'",
                 B_STRLOC,
                 g_type_name (widget_type),
                 property_name);
      return;
    }

  parser = g_param_spec_get_qdata (pspec,
                                   g_quark_from_static_string ("btk-rc-property-parser"));

  peek_value = _btk_style_peek_property_value (style, widget_type, pspec, parser);

  if (G_VALUE_TYPE (value) == G_PARAM_SPEC_VALUE_TYPE (pspec))
    g_value_copy (peek_value, value);
  else if (g_value_type_transformable (G_PARAM_SPEC_VALUE_TYPE (pspec), G_VALUE_TYPE (value)))
    g_value_transform (peek_value, value);
  else
    g_warning ("can't retrieve style property `%s' of type `%s' as value of type `%s'",
               pspec->name,
               g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)),
               G_VALUE_TYPE_NAME (value));
}

/**
 * btk_style_get_valist:
 * @style: a #BtkStyle
 * @widget_type: the #GType of a descendant of #BtkWidget
 * @first_property_name: the name of the first style property to get
 * @var_args: a <type>va_list</type> of pairs of property names and
 *     locations to return the property values, starting with the
 *     location for @first_property_name.
 *
 * Non-vararg variant of btk_style_get().
 * Used primarily by language bindings.
 *
 * Since: 2.16
 */
void 
btk_style_get_valist (BtkStyle    *style,
                      GType        widget_type,
                      const gchar *first_property_name,
                      va_list      var_args)
{
  const char *property_name;
  BtkWidgetClass *klass;

  g_return_if_fail (BTK_IS_STYLE (style));

  klass = g_type_class_ref (widget_type);

  property_name = first_property_name;
  while (property_name)
    {
      GParamSpec *pspec;
      BtkRcPropertyParser parser;
      const GValue *peek_value;
      gchar *error;

      pspec = btk_widget_class_find_style_property (klass, property_name);

      if (!pspec)
        {
          g_warning ("%s: widget class `%s' has no property named `%s'",
                     B_STRLOC,
                     g_type_name (widget_type),
                     property_name);
          break;
        }

      parser = g_param_spec_get_qdata (pspec,
                                       g_quark_from_static_string ("btk-rc-property-parser"));

      peek_value = _btk_style_peek_property_value (style, widget_type, pspec, parser);
      G_VALUE_LCOPY (peek_value, var_args, 0, &error);
      if (error)
        {
          g_warning ("%s: %s", B_STRLOC, error);
          g_free (error);
          break;
        }

      property_name = va_arg (var_args, gchar*);
    }

  g_type_class_unref (klass);
}

/**
 * btk_style_get:
 * @style: a #BtkStyle
 * @widget_type: the #GType of a descendant of #BtkWidget
 * @first_property_name: the name of the first style property to get
 * @Varargs: pairs of property names and locations to
 *   return the property values, starting with the location for
 *   @first_property_name, terminated by %NULL.
 *
 * Gets the values of a multiple style properties for @widget_type
 * from @style.
 *
 * Since: 2.16
 */
void
btk_style_get (BtkStyle    *style,
               GType        widget_type,
               const gchar *first_property_name,
               ...)
{
  va_list var_args;

  va_start (var_args, first_property_name);
  btk_style_get_valist (style, widget_type, first_property_name, var_args);
  va_end (var_args);
}

const GValue*
_btk_style_peek_property_value (BtkStyle           *style,
				GType               widget_type,
				GParamSpec         *pspec,
				BtkRcPropertyParser parser)
{
  PropertyValue *pcache, key = { 0, NULL, { 0, } };
  const BtkRcProperty *rcprop = NULL;
  guint i;

  g_return_val_if_fail (BTK_IS_STYLE (style), NULL);
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), NULL);
  g_return_val_if_fail (g_type_is_a (pspec->owner_type, BTK_TYPE_WIDGET), NULL);
  g_return_val_if_fail (g_type_is_a (widget_type, pspec->owner_type), NULL);

  key.widget_type = widget_type;
  key.pspec = pspec;

  /* need value cache array */
  if (!style->property_cache)
    style->property_cache = g_array_new (FALSE, FALSE, sizeof (PropertyValue));
  else
    {
      pcache = bsearch (&key,
			style->property_cache->data, style->property_cache->len,
			sizeof (PropertyValue), style_property_values_cmp);
      if (pcache)
	return &pcache->value;
    }

  i = 0;
  while (i < style->property_cache->len &&
	 style_property_values_cmp (&key, &g_array_index (style->property_cache, PropertyValue, i)) >= 0)
    i++;

  g_array_insert_val (style->property_cache, i, key);
  pcache = &g_array_index (style->property_cache, PropertyValue, i);

  /* cache miss, initialize value type, then set contents */
  g_param_spec_ref (pcache->pspec);
  g_value_init (&pcache->value, G_PARAM_SPEC_VALUE_TYPE (pspec));

  /* value provided by rc style? */
  if (style->rc_style)
    {
      GQuark prop_quark = g_quark_from_string (pspec->name);

      do
	{
	  rcprop = _btk_rc_style_lookup_rc_property (style->rc_style,
						     g_type_qname (widget_type),
						     prop_quark);
	  if (rcprop)
	    break;
	  widget_type = g_type_parent (widget_type);
	}
      while (g_type_is_a (widget_type, pspec->owner_type));
    }

  /* when supplied by rc style, we need to convert */
  if (rcprop && !_btk_settings_parse_convert (parser, &rcprop->value,
					      pspec, &pcache->value))
    {
      gchar *contents = g_strdup_value_contents (&rcprop->value);
      
      g_message ("%s: failed to retrieve property `%s::%s' of type `%s' from rc file value \"%s\" of type `%s'",
		 rcprop->origin ? rcprop->origin : "(for origin information, set BTK_DEBUG)",
		 g_type_name (pspec->owner_type), pspec->name,
		 g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)),
		 contents,
		 G_VALUE_TYPE_NAME (&rcprop->value));
      g_free (contents);
      rcprop = NULL; /* needs default */
    }
  
  /* not supplied by rc style (or conversion failed), revert to default */
  if (!rcprop)
    g_param_value_set_default (pspec, &pcache->value);

  return &pcache->value;
}

static BdkPixmap *
load_bg_image (BdkColormap *colormap,
	       BdkColor    *bg_color,
	       const gchar *filename)
{
  if (strcmp (filename, "<parent>") == 0)
    return (BdkPixmap*) BDK_PARENT_RELATIVE;
  else
    {
      return bdk_pixmap_colormap_create_from_xpm (NULL, colormap, NULL,
						  bg_color,
						  filename);
    }
}

static void
btk_style_real_realize (BtkStyle *style)
{
  BdkGCValues gc_values;
  BdkGCValuesMask gc_values_mask;
  
  gint i;

  for (i = 0; i < 5; i++)
    {
      _btk_style_shade (&style->bg[i], &style->light[i], LIGHTNESS_MULT);
      _btk_style_shade (&style->bg[i], &style->dark[i], DARKNESS_MULT);

      style->mid[i].red = (style->light[i].red + style->dark[i].red) / 2;
      style->mid[i].green = (style->light[i].green + style->dark[i].green) / 2;
      style->mid[i].blue = (style->light[i].blue + style->dark[i].blue) / 2;

      style->text_aa[i].red = (style->text[i].red + style->base[i].red) / 2;
      style->text_aa[i].green = (style->text[i].green + style->base[i].green) / 2;
      style->text_aa[i].blue = (style->text[i].blue + style->base[i].blue) / 2;
    }

  style->black.red = 0x0000;
  style->black.green = 0x0000;
  style->black.blue = 0x0000;
  bdk_colormap_alloc_color (style->colormap, &style->black, FALSE, TRUE);

  style->white.red = 0xffff;
  style->white.green = 0xffff;
  style->white.blue = 0xffff;
  bdk_colormap_alloc_color (style->colormap, &style->white, FALSE, TRUE);

  gc_values_mask = BDK_GC_FOREGROUND | BDK_GC_BACKGROUND;
  
  gc_values.foreground = style->black;
  gc_values.background = style->white;
  style->black_gc = btk_gc_get (style->depth, style->colormap, &gc_values, gc_values_mask);
  
  gc_values.foreground = style->white;
  gc_values.background = style->black;
  style->white_gc = btk_gc_get (style->depth, style->colormap, &gc_values, gc_values_mask);
  
  gc_values_mask = BDK_GC_FOREGROUND;

  for (i = 0; i < 5; i++)
    {
      if (style->rc_style && style->rc_style->bg_pixmap_name[i])
	style->bg_pixmap[i] = load_bg_image (style->colormap,
					     &style->bg[i],
					     style->rc_style->bg_pixmap_name[i]);
      
      if (!bdk_colormap_alloc_color (style->colormap, &style->fg[i], FALSE, TRUE))
        g_warning ("unable to allocate color: ( %d %d %d )",
                   style->fg[i].red, style->fg[i].green, style->fg[i].blue);
      if (!bdk_colormap_alloc_color (style->colormap, &style->bg[i], FALSE, TRUE))
        g_warning ("unable to allocate color: ( %d %d %d )",
                   style->bg[i].red, style->bg[i].green, style->bg[i].blue);
      if (!bdk_colormap_alloc_color (style->colormap, &style->light[i], FALSE, TRUE))
        g_warning ("unable to allocate color: ( %d %d %d )",
                   style->light[i].red, style->light[i].green, style->light[i].blue);
      if (!bdk_colormap_alloc_color (style->colormap, &style->dark[i], FALSE, TRUE))
        g_warning ("unable to allocate color: ( %d %d %d )",
                   style->dark[i].red, style->dark[i].green, style->dark[i].blue);
      if (!bdk_colormap_alloc_color (style->colormap, &style->mid[i], FALSE, TRUE))
        g_warning ("unable to allocate color: ( %d %d %d )",
                   style->mid[i].red, style->mid[i].green, style->mid[i].blue);
      if (!bdk_colormap_alloc_color (style->colormap, &style->text[i], FALSE, TRUE))
        g_warning ("unable to allocate color: ( %d %d %d )",
                   style->text[i].red, style->text[i].green, style->text[i].blue);
      if (!bdk_colormap_alloc_color (style->colormap, &style->base[i], FALSE, TRUE))
        g_warning ("unable to allocate color: ( %d %d %d )",
                   style->base[i].red, style->base[i].green, style->base[i].blue);
      if (!bdk_colormap_alloc_color (style->colormap, &style->text_aa[i], FALSE, TRUE))
        g_warning ("unable to allocate color: ( %d %d %d )",
                   style->text_aa[i].red, style->text_aa[i].green, style->text_aa[i].blue);
      
      gc_values.foreground = style->fg[i];
      style->fg_gc[i] = btk_gc_get (style->depth, style->colormap, &gc_values, gc_values_mask);
      
      gc_values.foreground = style->bg[i];
      style->bg_gc[i] = btk_gc_get (style->depth, style->colormap, &gc_values, gc_values_mask);
      
      gc_values.foreground = style->light[i];
      style->light_gc[i] = btk_gc_get (style->depth, style->colormap, &gc_values, gc_values_mask);
      
      gc_values.foreground = style->dark[i];
      style->dark_gc[i] = btk_gc_get (style->depth, style->colormap, &gc_values, gc_values_mask);
      
      gc_values.foreground = style->mid[i];
      style->mid_gc[i] = btk_gc_get (style->depth, style->colormap, &gc_values, gc_values_mask);
      
      gc_values.foreground = style->text[i];
      style->text_gc[i] = btk_gc_get (style->depth, style->colormap, &gc_values, gc_values_mask);
      
      gc_values.foreground = style->base[i];
      style->base_gc[i] = btk_gc_get (style->depth, style->colormap, &gc_values, gc_values_mask);

      gc_values.foreground = style->text_aa[i];
      style->text_aa_gc[i] = btk_gc_get (style->depth, style->colormap, &gc_values, gc_values_mask);
    }
}

static void
btk_style_real_unrealize (BtkStyle *style)
{
  int i;

  btk_gc_release (style->black_gc);
  btk_gc_release (style->white_gc);
      
  for (i = 0; i < 5; i++)
    {
      btk_gc_release (style->fg_gc[i]);
      btk_gc_release (style->bg_gc[i]);
      btk_gc_release (style->light_gc[i]);
      btk_gc_release (style->dark_gc[i]);
      btk_gc_release (style->mid_gc[i]);
      btk_gc_release (style->text_gc[i]);
      btk_gc_release (style->base_gc[i]);
      btk_gc_release (style->text_aa_gc[i]);

      if (style->bg_pixmap[i] &&  style->bg_pixmap[i] != (BdkPixmap*) BDK_PARENT_RELATIVE)
	{
	  g_object_unref (style->bg_pixmap[i]);
	  style->bg_pixmap[i] = NULL;
	}
      
    }
  
  bdk_colormap_free_colors (style->colormap, style->fg, 5);
  bdk_colormap_free_colors (style->colormap, style->bg, 5);
  bdk_colormap_free_colors (style->colormap, style->light, 5);
  bdk_colormap_free_colors (style->colormap, style->dark, 5);
  bdk_colormap_free_colors (style->colormap, style->mid, 5);
  bdk_colormap_free_colors (style->colormap, style->text, 5);
  bdk_colormap_free_colors (style->colormap, style->base, 5);
  bdk_colormap_free_colors (style->colormap, style->text_aa, 5);

  style_unrealize_cursor_gcs (style);
}

static void
btk_style_real_set_background (BtkStyle    *style,
			       BdkWindow   *window,
			       BtkStateType state_type)
{
  BdkPixmap *pixmap;
  gint parent_relative;
  
  if (style->bg_pixmap[state_type])
    {
      if (style->bg_pixmap[state_type] == (BdkPixmap*) BDK_PARENT_RELATIVE)
        {
          pixmap = NULL;
          parent_relative = TRUE;
        }
      else
        {
          pixmap = style->bg_pixmap[state_type];
          parent_relative = FALSE;
        }
      
      bdk_window_set_back_pixmap (window, pixmap, parent_relative);
    }
  else
    bdk_window_set_background (window, &style->bg[state_type]);
}

/**
 * btk_style_render_icon:
 * @style: a #BtkStyle
 * @source: the #BtkIconSource specifying the icon to render
 * @direction: a text direction
 * @state: a state
 * @size: (type int) the size to render the icon at. A size of
 *     (BtkIconSize)-1 means render at the size of the source and
 *     don't scale.
 * @widget: (allow-none): the widget
 * @detail: (allow-none): a style detail
 *
 * Renders the icon specified by @source at the given @size
 * according to the given parameters and returns the result in a
 * pixbuf.
 *
 * Return value: (transfer full): a newly-created #BdkPixbuf
 *     containing the rendered icon
 */
BdkPixbuf *
btk_style_render_icon (BtkStyle            *style,
                       const BtkIconSource *source,
                       BtkTextDirection     direction,
                       BtkStateType         state,
                       BtkIconSize          size,
                       BtkWidget           *widget,
                       const gchar         *detail)
{
  BdkPixbuf *pixbuf;
  
  g_return_val_if_fail (BTK_IS_STYLE (style), NULL);
  g_return_val_if_fail (BTK_STYLE_GET_CLASS (style)->render_icon != NULL, NULL);
  
  pixbuf = BTK_STYLE_GET_CLASS (style)->render_icon (style, source, direction, state,
                                                     size, widget, detail);

  g_return_val_if_fail (pixbuf != NULL, NULL);

  return pixbuf;
}

/* Default functions */

/**
 * btk_style_apply_default_background:
 * @style:
 * @window:
 * @set_bg:
 * @state_type:
 * @area: (allow-none):
 * @x:
 * @y:
 * @width:
 * @height:
 */
void
btk_style_apply_default_background (BtkStyle          *style,
                                    BdkWindow         *window,
                                    gboolean           set_bg,
                                    BtkStateType        state_type,
                                    const BdkRectangle *area,
                                    gint                x,
                                    gint                y,
                                    gint                width,
                                    gint                height)
{
  BdkRectangle new_rect, old_rect;
  
  if (area)
    {
      old_rect.x = x;
      old_rect.y = y;
      old_rect.width = width;
      old_rect.height = height;
      
      if (!bdk_rectangle_intersect (area, &old_rect, &new_rect))
        return;
    }
  else
    {
      new_rect.x = x;
      new_rect.y = y;
      new_rect.width = width;
      new_rect.height = height;
    }
  
  if (!style->bg_pixmap[state_type] ||
      BDK_IS_PIXMAP (window) ||
      (!set_bg && style->bg_pixmap[state_type] != (BdkPixmap*) BDK_PARENT_RELATIVE))
    {
      BdkGC *gc = style->bg_gc[state_type];
      
      if (style->bg_pixmap[state_type])
        {
          bdk_gc_set_fill (gc, BDK_TILED);
          bdk_gc_set_tile (gc, style->bg_pixmap[state_type]);
        }
      
      bdk_draw_rectangle (window, gc, TRUE, 
                          new_rect.x, new_rect.y, new_rect.width, new_rect.height);
      if (style->bg_pixmap[state_type])
        bdk_gc_set_fill (gc, BDK_SOLID);
    }
  else
    {
      if (set_bg)
        {
          if (style->bg_pixmap[state_type] == (BdkPixmap*) BDK_PARENT_RELATIVE)
            bdk_window_set_back_pixmap (window, NULL, TRUE);
          else
            bdk_window_set_back_pixmap (window, style->bg_pixmap[state_type], FALSE);
        }
      
      bdk_window_clear_area (window, 
                             new_rect.x, new_rect.y, 
                             new_rect.width, new_rect.height);
    }
}

static BdkPixbuf *
scale_or_ref (BdkPixbuf *src,
              gint       width,
              gint       height)
{
  if (width == bdk_pixbuf_get_width (src) &&
      height == bdk_pixbuf_get_height (src))
    {
      return g_object_ref (src);
    }
  else
    {
      return bdk_pixbuf_scale_simple (src,
                                      width, height,
                                      BDK_INTERP_BILINEAR);
    }
}

static gboolean
lookup_icon_size (BtkStyle    *style,
		  BtkWidget   *widget,
		  BtkIconSize  size,
		  gint        *width,
		  gint        *height)
{
  BdkScreen *screen;
  BtkSettings *settings;

  if (widget && btk_widget_has_screen (widget))
    {
      screen = btk_widget_get_screen (widget);
      settings = btk_settings_get_for_screen (screen);
    }
  else if (style && style->colormap)
    {
      screen = bdk_colormap_get_screen (style->colormap);
      settings = btk_settings_get_for_screen (screen);
    }
  else
    {
      settings = btk_settings_get_default ();
      BTK_NOTE (MULTIHEAD,
		g_warning ("Using the default screen for btk_default_render_icon()"));
    }

  return btk_icon_size_lookup_for_settings (settings, size, width, height);
}

static BdkPixbuf *
btk_default_render_icon (BtkStyle            *style,
                         const BtkIconSource *source,
                         BtkTextDirection     direction,
                         BtkStateType         state,
                         BtkIconSize          size,
                         BtkWidget           *widget,
                         const gchar         *detail)
{
  gint width = 1;
  gint height = 1;
  BdkPixbuf *scaled;
  BdkPixbuf *stated;
  BdkPixbuf *base_pixbuf;

  /* Oddly, style can be NULL in this function, because
   * BtkIconSet can be used without a style and if so
   * it uses this function.
   */

  base_pixbuf = btk_icon_source_get_pixbuf (source);

  g_return_val_if_fail (base_pixbuf != NULL, NULL);

  if (size != (BtkIconSize) -1 && !lookup_icon_size(style, widget, size, &width, &height))
    {
      g_warning (B_STRLOC ": invalid icon size '%d'", size);
      return NULL;
    }

  /* If the size was wildcarded, and we're allowed to scale, then scale; otherwise,
   * leave it alone.
   */
  if (size != (BtkIconSize)-1 && btk_icon_source_get_size_wildcarded (source))
    scaled = scale_or_ref (base_pixbuf, width, height);
  else
    scaled = g_object_ref (base_pixbuf);

  /* If the state was wildcarded, then generate a state. */
  if (btk_icon_source_get_state_wildcarded (source))
    {
      if (state == BTK_STATE_INSENSITIVE)
        {
          stated = bdk_pixbuf_copy (scaled);      
          
          bdk_pixbuf_saturate_and_pixelate (scaled, stated,
                                            0.8, TRUE);
          
          g_object_unref (scaled);
        }
      else if (state == BTK_STATE_PRELIGHT)
        {
          stated = bdk_pixbuf_copy (scaled);      
          
          bdk_pixbuf_saturate_and_pixelate (scaled, stated,
                                            1.2, FALSE);
          
          g_object_unref (scaled);
        }
      else
        {
          stated = scaled;
        }
    }
  else
    stated = scaled;
  
  return stated;
}

static void
sanitize_size (BdkWindow *window,
	       gint      *width,
	       gint      *height)
{
  if ((*width == -1) && (*height == -1))
    bdk_drawable_get_size (window, width, height);
  else if (*width == -1)
    bdk_drawable_get_size (window, width, NULL);
  else if (*height == -1)
    bdk_drawable_get_size (window, NULL, height);
}

static void
btk_default_draw_hline (BtkStyle     *style,
                        BdkWindow    *window,
                        BtkStateType  state_type,
                        BdkRectangle  *area,
                        BtkWidget     *widget,
                        const gchar   *detail,
                        gint          x1,
                        gint          x2,
                        gint          y)
{
  gint thickness_light;
  gint thickness_dark;
  gint i;
  
  thickness_light = style->ythickness / 2;
  thickness_dark = style->ythickness - thickness_light;
  
  if (area)
    {
      bdk_gc_set_clip_rectangle (style->light_gc[state_type], area);
      bdk_gc_set_clip_rectangle (style->dark_gc[state_type], area);
    }
  
  if (detail && !strcmp (detail, "label"))
    {
      if (state_type == BTK_STATE_INSENSITIVE)
        bdk_draw_line (window, style->white_gc, x1 + 1, y + 1, x2 + 1, y + 1);   
      bdk_draw_line (window, style->fg_gc[state_type], x1, y, x2, y);     
    }
  else
    {
      for (i = 0; i < thickness_dark; i++)
        {
          bdk_draw_line (window, style->dark_gc[state_type], x1, y + i, x2 - i - 1, y + i);
          bdk_draw_line (window, style->light_gc[state_type], x2 - i, y + i, x2, y + i);
        }
      
      y += thickness_dark;
      for (i = 0; i < thickness_light; i++)
        {
          bdk_draw_line (window, style->dark_gc[state_type], x1, y + i, x1 + thickness_light - i - 1, y + i);
          bdk_draw_line (window, style->light_gc[state_type], x1 + thickness_light - i, y + i, x2, y + i);
        }
    }
  
  if (area)
    {
      bdk_gc_set_clip_rectangle (style->light_gc[state_type], NULL);
      bdk_gc_set_clip_rectangle (style->dark_gc[state_type], NULL);
    }
}


static void
btk_default_draw_vline (BtkStyle     *style,
                        BdkWindow    *window,
                        BtkStateType  state_type,
                        BdkRectangle  *area,
                        BtkWidget     *widget,
                        const gchar   *detail,
                        gint          y1,
                        gint          y2,
                        gint          x)
{
  gint thickness_light;
  gint thickness_dark;
  gint i;
  
  thickness_light = style->xthickness / 2;
  thickness_dark = style->xthickness - thickness_light;
  
  if (area)
    {
      bdk_gc_set_clip_rectangle (style->light_gc[state_type], area);
      bdk_gc_set_clip_rectangle (style->dark_gc[state_type], area);
    }
  for (i = 0; i < thickness_dark; i++)
    { 
      bdk_draw_line (window, style->dark_gc[state_type], x + i, y1, x + i, y2 - i - 1);
      bdk_draw_line (window, style->light_gc[state_type], x + i, y2 - i, x + i, y2);
    }
  
  x += thickness_dark;
  for (i = 0; i < thickness_light; i++)
    {
      bdk_draw_line (window, style->dark_gc[state_type], x + i, y1, x + i, y1 + thickness_light - i - 1);
      bdk_draw_line (window, style->light_gc[state_type], x + i, y1 + thickness_light - i, x + i, y2);
    }
  if (area)
    {
      bdk_gc_set_clip_rectangle (style->light_gc[state_type], NULL);
      bdk_gc_set_clip_rectangle (style->dark_gc[state_type], NULL);
    }
}

static void
draw_thin_shadow (BtkStyle      *style,
		  BdkWindow     *window,
		  BtkStateType   state,
		  BdkRectangle  *area,
		  gint           x,
		  gint           y,
		  gint           width,
		  gint           height)
{
  BdkGC *gc1, *gc2;

  sanitize_size (window, &width, &height);
  
  gc1 = style->light_gc[state];
  gc2 = style->dark_gc[state];
  
  if (area)
    {
      bdk_gc_set_clip_rectangle (gc1, area);
      bdk_gc_set_clip_rectangle (gc2, area);
    }
  
  bdk_draw_line (window, gc1,
		 x, y + height - 1, x + width - 1, y + height - 1);
  bdk_draw_line (window, gc1,
		 x + width - 1, y,  x + width - 1, y + height - 1);
      
  bdk_draw_line (window, gc2,
		 x, y, x + width - 2, y);
  bdk_draw_line (window, gc2,
		 x, y, x, y + height - 2);

  if (area)
    {
      bdk_gc_set_clip_rectangle (gc1, NULL);
      bdk_gc_set_clip_rectangle (gc2, NULL);
    }
}

static void
draw_spinbutton_shadow (BtkStyle        *style,
			BdkWindow       *window,
			BtkStateType     state,
			BtkTextDirection direction,
			BdkRectangle    *area,
			gint             x,
			gint             y,
			gint             width,
			gint             height)
{
  sanitize_size (window, &width, &height);

  if (area)
    {
      bdk_gc_set_clip_rectangle (style->black_gc, area);
      bdk_gc_set_clip_rectangle (style->bg_gc[state], area);
      bdk_gc_set_clip_rectangle (style->dark_gc[state], area);
      bdk_gc_set_clip_rectangle (style->light_gc[state], area);
    }

  if (direction == BTK_TEXT_DIR_LTR)
    {
      bdk_draw_line (window, style->dark_gc[state],
		     x, y, x + width - 1, y);
      bdk_draw_line (window, style->black_gc,
		     x, y + 1, x + width - 2, y + 1);
      bdk_draw_line (window, style->black_gc,
		     x + width - 2, y + 2, x + width - 2, y + height - 3);
      bdk_draw_line (window, style->light_gc[state],
		     x + width - 1, y + 1, x + width - 1, y + height - 2);
      bdk_draw_line (window, style->light_gc[state],
		     x, y + height - 1, x + width - 1, y + height - 1);
      bdk_draw_line (window, style->bg_gc[state],
		     x, y + height - 2, x + width - 2, y + height - 2);
      bdk_draw_line (window, style->black_gc,
		     x, y + 2, x, y + height - 3);
    }
  else
    {
      bdk_draw_line (window, style->dark_gc[state],
		     x, y, x + width - 1, y);
      bdk_draw_line (window, style->dark_gc[state],
		     x, y + 1, x, y + height - 1);
      bdk_draw_line (window, style->black_gc,
		     x + 1, y + 1, x + width - 1, y + 1);
      bdk_draw_line (window, style->black_gc,
		     x + 1, y + 2, x + 1, y + height - 2);
      bdk_draw_line (window, style->black_gc,
		     x + width - 1, y + 2, x + width - 1, y + height - 3);
      bdk_draw_line (window, style->light_gc[state],
		     x + 1, y + height - 1, x + width - 1, y + height - 1);
      bdk_draw_line (window, style->bg_gc[state],
		     x + 2, y + height - 2, x + width - 1, y + height - 2);
    }
  
  if (area)
    {
      bdk_gc_set_clip_rectangle (style->black_gc, NULL);
      bdk_gc_set_clip_rectangle (style->bg_gc[state], NULL);
      bdk_gc_set_clip_rectangle (style->dark_gc[state], NULL);
      bdk_gc_set_clip_rectangle (style->light_gc[state], NULL);
    }
}

static void
draw_menu_shadow (BtkStyle        *style,
		  BdkWindow       *window,
		  BtkStateType     state,
		  BdkRectangle    *area,
		  gint             x,
		  gint             y,
		  gint             width,
		  gint             height)
{
  if (style->ythickness > 0)
    {
      if (style->ythickness > 1)
	{
	  bdk_draw_line (window, style->dark_gc[state],
			 x + 1, y + height - 2, x + width - 2, y + height - 2);
	  bdk_draw_line (window, style->black_gc,
			 x, y + height - 1, x + width - 1, y + height - 1);
	}
      else
	{
	  bdk_draw_line (window, style->dark_gc[state],
			 x + 1, y + height - 1, x + width - 1, y + height - 1);
	}
    }
  
  if (style->xthickness > 0)
    {
      if (style->xthickness > 1)
	{
	  bdk_draw_line (window, style->dark_gc[state],
			 x + width - 2, y + 1, x + width - 2, y + height - 2);
	  
	  bdk_draw_line (window, style->black_gc,
			 x + width - 1, y, x + width - 1, y + height - 1);
	}
      else
	{
	  bdk_draw_line (window, style->dark_gc[state],
			 x + width - 1, y + 1, x + width - 1, y + height - 1);
	}
    }
  
  /* Light around top and left */
  
  if (style->ythickness > 0)
    bdk_draw_line (window, style->black_gc,
		   x, y, x + width - 2, y);
  if (style->xthickness > 0)
    bdk_draw_line (window, style->black_gc,
		   x, y, x, y + height - 2);
  
  if (style->ythickness > 1)
    bdk_draw_line (window, style->light_gc[state],
		   x + 1, y + 1, x + width - 3, y + 1);
  if (style->xthickness > 1)
    bdk_draw_line (window, style->light_gc[state],
		   x + 1, y + 1, x + 1, y + height - 3);
}

static BtkTextDirection
get_direction (BtkWidget *widget)
{
  BtkTextDirection dir;
  
  if (widget)
    dir = btk_widget_get_direction (widget);
  else
    dir = BTK_TEXT_DIR_LTR;
  
  return dir;
}


static void
btk_default_draw_shadow (BtkStyle      *style,
                         BdkWindow     *window,
                         BtkStateType   state_type,
                         BtkShadowType  shadow_type,
                         BdkRectangle  *area,
                         BtkWidget     *widget,
                         const gchar   *detail,
                         gint           x,
                         gint           y,
                         gint           width,
                         gint           height)
{
  BdkGC *gc1 = NULL;
  BdkGC *gc2 = NULL;
  gint thickness_light;
  gint thickness_dark;
  gint i;
  
  if (shadow_type == BTK_SHADOW_IN)
    {
      if (detail && strcmp (detail, "buttondefault") == 0)
	{
	  sanitize_size (window, &width, &height);

	  bdk_draw_rectangle (window, style->black_gc, FALSE,
			      x, y, width - 1, height - 1);
	  
	  return;
	}
      if (detail && strcmp (detail, "trough") == 0)
	{
	  draw_thin_shadow (style, window, state_type, area,
			    x, y, width, height);
	  return;
	}
      if (BTK_IS_SPIN_BUTTON (widget) &&
         detail && strcmp (detail, "spinbutton") == 0)
	{
	  draw_spinbutton_shadow (style, window, state_type, 
				  get_direction (widget), area, x, y, width, height);
	  
	  return;
	}
    }

  if (shadow_type == BTK_SHADOW_OUT && detail && strcmp (detail, "menu") == 0)
    {
      draw_menu_shadow (style, window, state_type, area, x, y, width, height);
      return;
    }
  
  sanitize_size (window, &width, &height);
  
  switch (shadow_type)
    {
    case BTK_SHADOW_NONE:
      return;
    case BTK_SHADOW_IN:
    case BTK_SHADOW_ETCHED_IN:
      gc1 = style->light_gc[state_type];
      gc2 = style->dark_gc[state_type];
      break;
    case BTK_SHADOW_OUT:
    case BTK_SHADOW_ETCHED_OUT:
      gc1 = style->dark_gc[state_type];
      gc2 = style->light_gc[state_type];
      break;
    }
  
  if (area)
    {
      bdk_gc_set_clip_rectangle (gc1, area);
      bdk_gc_set_clip_rectangle (gc2, area);
      if (shadow_type == BTK_SHADOW_IN || 
          shadow_type == BTK_SHADOW_OUT)
        {
          bdk_gc_set_clip_rectangle (style->black_gc, area);
          bdk_gc_set_clip_rectangle (style->bg_gc[state_type], area);
        }
    }
  
  switch (shadow_type)
    {
    case BTK_SHADOW_NONE:
      break;
      
    case BTK_SHADOW_IN:
      /* Light around right and bottom edge */

      if (style->ythickness > 0)
        bdk_draw_line (window, gc1,
                       x, y + height - 1, x + width - 1, y + height - 1);
      if (style->xthickness > 0)
        bdk_draw_line (window, gc1,
                       x + width - 1, y, x + width - 1, y + height - 1);

      if (style->ythickness > 1)
        bdk_draw_line (window, style->bg_gc[state_type],
                       x + 1, y + height - 2, x + width - 2, y + height - 2);
      if (style->xthickness > 1)
        bdk_draw_line (window, style->bg_gc[state_type],
                       x + width - 2, y + 1, x + width - 2, y + height - 2);

      /* Dark around left and top */

      if (style->ythickness > 1)
        bdk_draw_line (window, style->black_gc,
                       x + 1, y + 1, x + width - 2, y + 1);
      if (style->xthickness > 1)
        bdk_draw_line (window, style->black_gc,
                       x + 1, y + 1, x + 1, y + height - 2);

      if (style->ythickness > 0)
        bdk_draw_line (window, gc2,
                       x, y, x + width - 1, y);
      if (style->xthickness > 0)
        bdk_draw_line (window, gc2,
                       x, y, x, y + height - 1);
      break;
      
    case BTK_SHADOW_OUT:
      /* Dark around right and bottom edge */

      if (style->ythickness > 0)
        {
          if (style->ythickness > 1)
            {
              bdk_draw_line (window, gc1,
                             x + 1, y + height - 2, x + width - 2, y + height - 2);
              bdk_draw_line (window, style->black_gc,
                             x, y + height - 1, x + width - 1, y + height - 1);
            }
          else
            {
              bdk_draw_line (window, gc1,
                             x + 1, y + height - 1, x + width - 1, y + height - 1);
            }
        }

      if (style->xthickness > 0)
        {
          if (style->xthickness > 1)
            {
              bdk_draw_line (window, gc1,
                             x + width - 2, y + 1, x + width - 2, y + height - 2);
              
              bdk_draw_line (window, style->black_gc,
                             x + width - 1, y, x + width - 1, y + height - 1);
            }
          else
            {
              bdk_draw_line (window, gc1,
                             x + width - 1, y + 1, x + width - 1, y + height - 1);
            }
        }
      
      /* Light around top and left */

      if (style->ythickness > 0)
        bdk_draw_line (window, gc2,
                       x, y, x + width - 2, y);
      if (style->xthickness > 0)
        bdk_draw_line (window, gc2,
                       x, y, x, y + height - 2);

      if (style->ythickness > 1)
        bdk_draw_line (window, style->bg_gc[state_type],
                       x + 1, y + 1, x + width - 3, y + 1);
      if (style->xthickness > 1)
        bdk_draw_line (window, style->bg_gc[state_type],
                       x + 1, y + 1, x + 1, y + height - 3);
      break;
      
    case BTK_SHADOW_ETCHED_IN:
    case BTK_SHADOW_ETCHED_OUT:
      if (style->xthickness > 0)
        {
          if (style->xthickness > 1)
            {
              thickness_light = 1;
              thickness_dark = 1;
      
              for (i = 0; i < thickness_dark; i++)
                {
                  bdk_draw_line (window, gc1,
                                 x + width - i - 1,
                                 y + i,
                                 x + width - i - 1,
                                 y + height - i - 1);
                  bdk_draw_line (window, gc2,
                                 x + i,
                                 y + i,
                                 x + i,
                                 y + height - i - 2);
                }
      
              for (i = 0; i < thickness_light; i++)
                {
                  bdk_draw_line (window, gc1,
                                 x + thickness_dark + i,
                                 y + thickness_dark + i,
                                 x + thickness_dark + i,
                                 y + height - thickness_dark - i - 1);
                  bdk_draw_line (window, gc2,
                                 x + width - thickness_light - i - 1,
                                 y + thickness_dark + i,
                                 x + width - thickness_light - i - 1,
                                 y + height - thickness_light - 1);
                }
            }
          else
            {
              bdk_draw_line (window, 
                             style->dark_gc[state_type],
                             x, y, x, y + height);                         
              bdk_draw_line (window, 
                             style->dark_gc[state_type],
                             x + width, y, x + width, y + height);
            }
        }

      if (style->ythickness > 0)
        {
          if (style->ythickness > 1)
            {
              thickness_light = 1;
              thickness_dark = 1;
      
              for (i = 0; i < thickness_dark; i++)
                {
                  bdk_draw_line (window, gc1,
                                 x + i,
                                 y + height - i - 1,
                                 x + width - i - 1,
                                 y + height - i - 1);
          
                  bdk_draw_line (window, gc2,
                                 x + i,
                                 y + i,
                                 x + width - i - 2,
                                 y + i);
                }
      
              for (i = 0; i < thickness_light; i++)
                {
                  bdk_draw_line (window, gc1,
                                 x + thickness_dark + i,
                                 y + thickness_dark + i,
                                 x + width - thickness_dark - i - 2,
                                 y + thickness_dark + i);
          
                  bdk_draw_line (window, gc2,
                                 x + thickness_dark + i,
                                 y + height - thickness_light - i - 1,
                                 x + width - thickness_light - 1,
                                 y + height - thickness_light - i - 1);
                }
            }
          else
            {
              bdk_draw_line (window, 
                             style->dark_gc[state_type],
                             x, y, x + width, y);
              bdk_draw_line (window, 
                             style->dark_gc[state_type],
                             x, y + height, x + width, y + height);
            }
        }
      
      break;
    }

  if (shadow_type == BTK_SHADOW_IN &&
      BTK_IS_SPIN_BUTTON (widget) &&
      detail && strcmp (detail, "entry") == 0)
    {
      if (get_direction (widget) == BTK_TEXT_DIR_LTR)
	{
	  bdk_draw_line (window,
			 style->base_gc[state_type],
			 x + width - 1, y + 2,
			 x + width - 1, y + height - 3);
	  bdk_draw_line (window,
			 style->base_gc[state_type],
			 x + width - 2, y + 2,
			 x + width - 2, y + height - 3);
	  bdk_draw_point (window,
			  style->black_gc,
			  x + width - 1, y + 1);
	  bdk_draw_point (window,
			  style->bg_gc[state_type],
			  x + width - 1, y + height - 2);
	}
      else
	{
	  bdk_draw_line (window,
			 style->base_gc[state_type],
			 x, y + 2,
			 x, y + height - 3);
	  bdk_draw_line (window,
			 style->base_gc[state_type],
			 x + 1, y + 2,
			 x + 1, y + height - 3);
	  bdk_draw_point (window,
			  style->black_gc,
			  x, y + 1);
	  bdk_draw_line (window,
			 style->bg_gc[state_type],
			 x, y + height - 2,
			 x + 1, y + height - 2);
	  bdk_draw_point (window,
			  style->light_gc[state_type],
			  x, y + height - 1);
	}
    }


  if (area)
    {
      bdk_gc_set_clip_rectangle (gc1, NULL);
      bdk_gc_set_clip_rectangle (gc2, NULL);
      if (shadow_type == BTK_SHADOW_IN || 
          shadow_type == BTK_SHADOW_OUT)
        {
          bdk_gc_set_clip_rectangle (style->black_gc, NULL);
          bdk_gc_set_clip_rectangle (style->bg_gc[state_type], NULL);
        }
    }
}

static void
btk_default_draw_polygon (BtkStyle      *style,
                          BdkWindow     *window,
                          BtkStateType   state_type,
                          BtkShadowType  shadow_type,
                          BdkRectangle  *area,
                          BtkWidget     *widget,
                          const gchar   *detail,
                          BdkPoint      *points,
                          gint           npoints,
                          gboolean       fill)
{
  static const gdouble pi_over_4 = G_PI_4;
  static const gdouble pi_3_over_4 = G_PI_4 * 3;
  BdkGC *gc1;
  BdkGC *gc2;
  BdkGC *gc3;
  BdkGC *gc4;
  gdouble angle;
  gint xadjust;
  gint yadjust;
  gint i;
  
  switch (shadow_type)
    {
    case BTK_SHADOW_IN:
      gc1 = style->bg_gc[state_type];
      gc2 = style->dark_gc[state_type];
      gc3 = style->light_gc[state_type];
      gc4 = style->black_gc;
      break;
    case BTK_SHADOW_ETCHED_IN:
      gc1 = style->light_gc[state_type];
      gc2 = style->dark_gc[state_type];
      gc3 = style->dark_gc[state_type];
      gc4 = style->light_gc[state_type];
      break;
    case BTK_SHADOW_OUT:
      gc1 = style->dark_gc[state_type];
      gc2 = style->light_gc[state_type];
      gc3 = style->black_gc;
      gc4 = style->bg_gc[state_type];
      break;
    case BTK_SHADOW_ETCHED_OUT:
      gc1 = style->dark_gc[state_type];
      gc2 = style->light_gc[state_type];
      gc3 = style->light_gc[state_type];
      gc4 = style->dark_gc[state_type];
      break;
    default:
      return;
    }
  
  if (area)
    {
      bdk_gc_set_clip_rectangle (gc1, area);
      bdk_gc_set_clip_rectangle (gc2, area);
      bdk_gc_set_clip_rectangle (gc3, area);
      bdk_gc_set_clip_rectangle (gc4, area);
    }
  
  if (fill)
    bdk_draw_polygon (window, style->bg_gc[state_type], TRUE, points, npoints);
  
  npoints--;
  
  for (i = 0; i < npoints; i++)
    {
      if ((points[i].x == points[i+1].x) &&
          (points[i].y == points[i+1].y))
        {
          angle = 0;
        }
      else
        {
          angle = atan2 (points[i+1].y - points[i].y,
                         points[i+1].x - points[i].x);
        }
      
      if ((angle > -pi_3_over_4) && (angle < pi_over_4))
        {
          if (angle > -pi_over_4)
            {
              xadjust = 0;
              yadjust = 1;
            }
          else
            {
              xadjust = 1;
              yadjust = 0;
            }
          
          bdk_draw_line (window, gc1,
                         points[i].x-xadjust, points[i].y-yadjust,
                         points[i+1].x-xadjust, points[i+1].y-yadjust);
          bdk_draw_line (window, gc3,
                         points[i].x, points[i].y,
                         points[i+1].x, points[i+1].y);
        }
      else
        {
          if ((angle < -pi_3_over_4) || (angle > pi_3_over_4))
            {
              xadjust = 0;
              yadjust = 1;
            }
          else
            {
              xadjust = 1;
              yadjust = 0;
            }
          
          bdk_draw_line (window, gc4,
                         points[i].x+xadjust, points[i].y+yadjust,
                         points[i+1].x+xadjust, points[i+1].y+yadjust);
          bdk_draw_line (window, gc2,
                         points[i].x, points[i].y,
                         points[i+1].x, points[i+1].y);
        }
    }

  if (area)
    {
      bdk_gc_set_clip_rectangle (gc1, NULL);
      bdk_gc_set_clip_rectangle (gc2, NULL);
      bdk_gc_set_clip_rectangle (gc3, NULL);
      bdk_gc_set_clip_rectangle (gc4, NULL);
    }
}

static void
draw_arrow (BdkWindow     *window,
	    BdkColor      *color,
	    BdkRectangle  *area,
	    BtkArrowType   arrow_type,
	    gint           x,
	    gint           y,
	    gint           width,
	    gint           height)
{
  bairo_t *cr = bdk_bairo_create (window);
  bdk_bairo_set_source_color (cr, color);
  
  if (area)
    {
      bdk_bairo_rectangle (cr, area);
      bairo_clip (cr);
    }
    
  if (arrow_type == BTK_ARROW_DOWN)
    {
      bairo_move_to (cr, x,              y);
      bairo_line_to (cr, x + width,      y);
      bairo_line_to (cr, x + width / 2., y + height);
    }
  else if (arrow_type == BTK_ARROW_UP)
    {
      bairo_move_to (cr, x,              y + height);
      bairo_line_to (cr, x + width / 2., y);
      bairo_line_to (cr, x + width,      y + height);
    }
  else if (arrow_type == BTK_ARROW_LEFT)
    {
      bairo_move_to (cr, x + width,      y);
      bairo_line_to (cr, x + width,      y + height);
      bairo_line_to (cr, x,              y + height / 2.);
    }
  else if (arrow_type == BTK_ARROW_RIGHT)
    {
      bairo_move_to (cr, x,              y);
      bairo_line_to (cr, x + width,      y + height / 2.);
      bairo_line_to (cr, x,              y + height);
    }

  bairo_close_path (cr);
  bairo_fill (cr);

  bairo_destroy (cr);
}

static void
calculate_arrow_geometry (BtkArrowType  arrow_type,
			  gint         *x,
			  gint         *y,
			  gint         *width,
			  gint         *height)
{
  gint w = *width;
  gint h = *height;
  
  switch (arrow_type)
    {
    case BTK_ARROW_UP:
    case BTK_ARROW_DOWN:
      w += (w % 2) - 1;
      h = (w / 2 + 1);
      
      if (h > *height)
	{
	  h = *height;
	  w = 2 * h - 1;
	}
      
      if (arrow_type == BTK_ARROW_DOWN)
	{
	  if (*height % 2 == 1 || h % 2 == 0)
	    *height += 1;
	}
      else
	{
	  if (*height % 2 == 0 || h % 2 == 0)
	    *height -= 1;
	}
      break;

    case BTK_ARROW_RIGHT:
    case BTK_ARROW_LEFT:
      h += (h % 2) - 1;
      w = (h / 2 + 1);
      
      if (w > *width)
	{
	  w = *width;
	  h = 2 * w - 1;
	}
      
      if (arrow_type == BTK_ARROW_RIGHT)
	{
	  if (*width % 2 == 1 || w % 2 == 0)
	    *width += 1;
	}
      else
	{
	  if (*width % 2 == 0 || w % 2 == 0)
	    *width -= 1;
	}
      break;
      
    default:
      /* should not be reached */
      break;
    }

  *x += (*width - w) / 2;
  *y += (*height - h) / 2;
  *height = h;
  *width = w;
}

static void
btk_default_draw_arrow (BtkStyle      *style,
			BdkWindow     *window,
			BtkStateType   state,
			BtkShadowType  shadow,
			BdkRectangle  *area,
			BtkWidget     *widget,
			const gchar   *detail,
			BtkArrowType   arrow_type,
			gboolean       fill,
			gint           x,
			gint           y,
			gint           width,
			gint           height)
{
  sanitize_size (window, &width, &height);

  calculate_arrow_geometry (arrow_type, &x, &y, &width, &height);

  if (detail && strcmp (detail, "menu_scroll_arrow_up") == 0)
    y++;

  if (state == BTK_STATE_INSENSITIVE)
    draw_arrow (window, &style->white, area, arrow_type,
		x + 1, y + 1, width, height);
  draw_arrow (window, &style->fg[state], area, arrow_type,
	      x, y, width, height);
}

static void
btk_default_draw_diamond (BtkStyle      *style,
                          BdkWindow     *window,
                          BtkStateType   state_type,
                          BtkShadowType  shadow_type,
                          BdkRectangle  *area,
                          BtkWidget     *widget,
                          const gchar   *detail,
                          gint           x,
                          gint           y,
                          gint           width,
                          gint           height)
{
  gint half_width;
  gint half_height;
  BdkGC *outer_nw = NULL;
  BdkGC *outer_ne = NULL;
  BdkGC *outer_sw = NULL;
  BdkGC *outer_se = NULL;
  BdkGC *middle_nw = NULL;
  BdkGC *middle_ne = NULL;
  BdkGC *middle_sw = NULL;
  BdkGC *middle_se = NULL;
  BdkGC *inner_nw = NULL;
  BdkGC *inner_ne = NULL;
  BdkGC *inner_sw = NULL;
  BdkGC *inner_se = NULL;
  
  sanitize_size (window, &width, &height);
  
  half_width = width / 2;
  half_height = height / 2;
  
  if (area)
    {
      bdk_gc_set_clip_rectangle (style->light_gc[state_type], area);
      bdk_gc_set_clip_rectangle (style->bg_gc[state_type], area);
      bdk_gc_set_clip_rectangle (style->dark_gc[state_type], area);
      bdk_gc_set_clip_rectangle (style->black_gc, area);
    }
  
  switch (shadow_type)
    {
    case BTK_SHADOW_IN:
      inner_sw = inner_se = style->bg_gc[state_type];
      middle_sw = middle_se = style->light_gc[state_type];
      outer_sw = outer_se = style->light_gc[state_type];
      inner_nw = inner_ne = style->black_gc;
      middle_nw = middle_ne = style->dark_gc[state_type];
      outer_nw = outer_ne = style->dark_gc[state_type];
      break;
          
    case BTK_SHADOW_OUT:
      inner_sw = inner_se = style->dark_gc[state_type];
      middle_sw = middle_se = style->dark_gc[state_type];
      outer_sw = outer_se = style->black_gc;
      inner_nw = inner_ne = style->bg_gc[state_type];
      middle_nw = middle_ne = style->light_gc[state_type];
      outer_nw = outer_ne = style->light_gc[state_type];
      break;

    case BTK_SHADOW_ETCHED_IN:
      inner_sw = inner_se = style->bg_gc[state_type];
      middle_sw = middle_se = style->dark_gc[state_type];
      outer_sw = outer_se = style->light_gc[state_type];
      inner_nw = inner_ne = style->bg_gc[state_type];
      middle_nw = middle_ne = style->light_gc[state_type];
      outer_nw = outer_ne = style->dark_gc[state_type];
      break;

    case BTK_SHADOW_ETCHED_OUT:
      inner_sw = inner_se = style->bg_gc[state_type];
      middle_sw = middle_se = style->light_gc[state_type];
      outer_sw = outer_se = style->dark_gc[state_type];
      inner_nw = inner_ne = style->bg_gc[state_type];
      middle_nw = middle_ne = style->dark_gc[state_type];
      outer_nw = outer_ne = style->light_gc[state_type];
      break;
      
    default:

      break;
    }

  if (inner_sw)
    {
      bdk_draw_line (window, inner_sw,
                     x + 2, y + half_height,
                     x + half_width, y + height - 2);
      bdk_draw_line (window, inner_se,
                     x + half_width, y + height - 2,
                     x + width - 2, y + half_height);
      bdk_draw_line (window, middle_sw,
                     x + 1, y + half_height,
                     x + half_width, y + height - 1);
      bdk_draw_line (window, middle_se,
                     x + half_width, y + height - 1,
                     x + width - 1, y + half_height);
      bdk_draw_line (window, outer_sw,
                     x, y + half_height,
                     x + half_width, y + height);
      bdk_draw_line (window, outer_se,
                     x + half_width, y + height,
                     x + width, y + half_height);
  
      bdk_draw_line (window, inner_nw,
                     x + 2, y + half_height,
                     x + half_width, y + 2);
      bdk_draw_line (window, inner_ne,
                     x + half_width, y + 2,
                     x + width - 2, y + half_height);
      bdk_draw_line (window, middle_nw,
                     x + 1, y + half_height,
                     x + half_width, y + 1);
      bdk_draw_line (window, middle_ne,
                     x + half_width, y + 1,
                     x + width - 1, y + half_height);
      bdk_draw_line (window, outer_nw,
                     x, y + half_height,
                     x + half_width, y);
      bdk_draw_line (window, outer_ne,
                     x + half_width, y,
                     x + width, y + half_height);
    }
  
  if (area)
    {
      bdk_gc_set_clip_rectangle (style->light_gc[state_type], NULL);
      bdk_gc_set_clip_rectangle (style->bg_gc[state_type], NULL);
      bdk_gc_set_clip_rectangle (style->dark_gc[state_type], NULL);
      bdk_gc_set_clip_rectangle (style->black_gc, NULL);
    }
}

static void
btk_default_draw_string (BtkStyle      *style,
                         BdkWindow     *window,
                         BtkStateType   state_type,
                         BdkRectangle  *area,
                         BtkWidget     *widget,
                         const gchar   *detail,
                         gint           x,
                         gint           y,
                         const gchar   *string)
{
  if (area)
    {
      bdk_gc_set_clip_rectangle (style->white_gc, area);
      bdk_gc_set_clip_rectangle (style->fg_gc[state_type], area);
    }

  if (state_type == BTK_STATE_INSENSITIVE)
    bdk_draw_string (window,
		     btk_style_get_font_internal (style),
		     style->white_gc, x + 1, y + 1, string);

  bdk_draw_string (window,
		   btk_style_get_font_internal (style),
		   style->fg_gc[state_type], x, y, string);

  if (area)
    {
      bdk_gc_set_clip_rectangle (style->white_gc, NULL);
      bdk_gc_set_clip_rectangle (style->fg_gc[state_type], NULL);
    }
}

static void
option_menu_get_props (BtkWidget      *widget,
		       BtkRequisition *indicator_size,
		       BtkBorder      *indicator_spacing)
{
  BtkRequisition *tmp_size = NULL;
  BtkBorder *tmp_spacing = NULL;
  
  if (BTK_IS_OPTION_MENU (widget))
    btk_widget_style_get (widget, 
			  "indicator-size", &tmp_size,
			  "indicator-spacing", &tmp_spacing,
			  NULL);

  if (tmp_size)
    {
      *indicator_size = *tmp_size;
      btk_requisition_free (tmp_size);
    }
  else
    *indicator_size = default_option_indicator_size;

  if (tmp_spacing)
    {
      *indicator_spacing = *tmp_spacing;
      btk_border_free (tmp_spacing);
    }
  else
    *indicator_spacing = default_option_indicator_spacing;
}

static void 
btk_default_draw_box (BtkStyle      *style,
		      BdkWindow     *window,
		      BtkStateType   state_type,
		      BtkShadowType  shadow_type,
		      BdkRectangle  *area,
		      BtkWidget     *widget,
		      const gchar   *detail,
		      gint           x,
		      gint           y,
		      gint           width,
		      gint           height)
{
  gboolean is_spinbutton_box = FALSE;
  
  sanitize_size (window, &width, &height);

  if (BTK_IS_SPIN_BUTTON (widget) && detail)
    {
      if (strcmp (detail, "spinbutton_up") == 0)
	{
	  y += 2;
	  width -= 3;
	  height -= 2;

	  if (get_direction (widget) == BTK_TEXT_DIR_RTL)
	    x += 2;
	  else
	    x += 1;

	  is_spinbutton_box = TRUE;
	}
      else if (strcmp (detail, "spinbutton_down") == 0)
	{
	  width -= 3;
	  height -= 2;

	  if (get_direction (widget) == BTK_TEXT_DIR_RTL)
	    x += 2;
	  else
	    x += 1;

	  is_spinbutton_box = TRUE;
	}
    }
  
  if (!style->bg_pixmap[state_type] || 
      BDK_IS_PIXMAP (window))
    {
      BdkGC *gc = style->bg_gc[state_type];
      
      if (state_type == BTK_STATE_SELECTED && detail && strcmp (detail, "paned") == 0)
	{
	  if (widget && !btk_widget_has_focus (widget))
	    gc = style->base_gc[BTK_STATE_ACTIVE];
	}

      if (area)
	bdk_gc_set_clip_rectangle (gc, area);

      bdk_draw_rectangle (window, gc, TRUE,
                          x, y, width, height);
      if (area)
	bdk_gc_set_clip_rectangle (gc, NULL);
    }
  else
    btk_style_apply_default_background (style, window,
                                        widget && btk_widget_get_has_window (widget),
                                        state_type, area, x, y, width, height);

  if (is_spinbutton_box)
    {
      BdkGC *upper_gc;
      BdkGC *lower_gc;
      
      lower_gc = style->dark_gc[state_type];
      if (shadow_type == BTK_SHADOW_OUT)
	upper_gc = style->light_gc[state_type];
      else
	upper_gc = style->dark_gc[state_type];

      if (area)
	{
	  bdk_gc_set_clip_rectangle (style->dark_gc[state_type], area);
	  bdk_gc_set_clip_rectangle (style->light_gc[state_type], area);
	}
      
      bdk_draw_line (window, upper_gc, x, y, x + width - 1, y);
      bdk_draw_line (window, lower_gc, x, y + height - 1, x + width - 1, y + height - 1);

      if (area)
	{
	  bdk_gc_set_clip_rectangle (style->dark_gc[state_type], NULL);
	  bdk_gc_set_clip_rectangle (style->light_gc[state_type], NULL);
	}
      return;
    }

  btk_paint_shadow (style, window, state_type, shadow_type, area, widget, detail,
                    x, y, width, height);

  if (detail && strcmp (detail, "optionmenu") == 0)
    {
      BtkRequisition indicator_size;
      BtkBorder indicator_spacing;
      gint vline_x;

      option_menu_get_props (widget, &indicator_size, &indicator_spacing);

      sanitize_size (window, &width, &height);

      if (get_direction (widget) == BTK_TEXT_DIR_RTL)
	vline_x = x + indicator_size.width + indicator_spacing.left + indicator_spacing.right;
      else 
	vline_x = x + width - (indicator_size.width + indicator_spacing.left + indicator_spacing.right) - style->xthickness;

      btk_paint_vline (style, window, state_type, area, widget,
		       detail,
		       y + style->ythickness + 1,
		       y + height - style->ythickness - 3,
		       vline_x);
    }
}

static BdkGC *
get_darkened_gc (BdkWindow      *window,
                 const BdkColor *color,
                 gint            darken_count)
{
  BdkColor src = *color;
  BdkColor shaded = *color;
  BdkGC *gc;
  
  gc = bdk_gc_new (window);

  while (darken_count)
    {
      _btk_style_shade (&src, &shaded, 0.93);
      src = shaded;
      --darken_count;
    }
   
  bdk_gc_set_rgb_fg_color (gc, &shaded);

  return gc;
}

static void 
btk_default_draw_flat_box (BtkStyle      *style,
                           BdkWindow     *window,
                           BtkStateType   state_type,
                           BtkShadowType  shadow_type,
                           BdkRectangle  *area,
                           BtkWidget     *widget,
                           const gchar   *detail,
                           gint           x,
                           gint           y,
                           gint           width,
                           gint           height)
{
  BdkGC *gc1;
  BdkGC *freeme = NULL;
  
  sanitize_size (window, &width, &height);
  
  if (detail)
    {
      if (state_type == BTK_STATE_SELECTED)
        {
          if (!strcmp ("text", detail))
            gc1 = style->bg_gc[BTK_STATE_SELECTED];
          else if (!strcmp ("cell_even", detail) ||
                   !strcmp ("cell_odd", detail) ||
                   !strcmp ("cell_even_ruled", detail) ||
		   !strcmp ("cell_even_ruled_sorted", detail))
            {
	      /* This has to be really broken; alex made me do it. -jrb */
	      if (widget && btk_widget_has_focus (widget))
		gc1 = style->base_gc[state_type];
	      else
	        gc1 = style->base_gc[BTK_STATE_ACTIVE];
            }
	  else if (!strcmp ("cell_odd_ruled", detail) ||
		   !strcmp ("cell_odd_ruled_sorted", detail))
	    {
	      if (widget && btk_widget_has_focus (widget))
	        freeme = get_darkened_gc (window, &style->base[state_type], 1);
	      else
	        freeme = get_darkened_gc (window, &style->base[BTK_STATE_ACTIVE], 1);
	      gc1 = freeme;
	    }
          else
            {
              gc1 = style->bg_gc[state_type];
            }
        }
      else
        {
          if (!strcmp ("viewportbin", detail))
            gc1 = style->bg_gc[BTK_STATE_NORMAL];
          else if (!strcmp ("entry_bg", detail))
            gc1 = style->base_gc[state_type];

          /* For trees: even rows are base color, odd rows are a shade of
           * the base color, the sort column is a shade of the original color
           * for that row.
           */

          else if (!strcmp ("cell_even", detail) ||
                   !strcmp ("cell_odd", detail) ||
                   !strcmp ("cell_even_ruled", detail))
            {
	      BdkColor *color = NULL;

	      btk_widget_style_get (widget,
		                    "even-row-color", &color,
				    NULL);

	      if (color)
	        {
		  freeme = get_darkened_gc (window, color, 0);
		  gc1 = freeme;

		  bdk_color_free (color);
		}
	      else
	        gc1 = style->base_gc[state_type];
            }
	  else if (!strcmp ("cell_odd_ruled", detail))
	    {
	      BdkColor *color = NULL;

	      btk_widget_style_get (widget,
		                    "odd-row-color", &color,
				    NULL);

	      if (color)
	        {
		  freeme = get_darkened_gc (window, color, 0);
		  gc1 = freeme;

		  bdk_color_free (color);
		}
	      else
	        {
		  btk_widget_style_get (widget,
		                        "even-row-color", &color,
					NULL);

		  if (color)
		    {
		      freeme = get_darkened_gc (window, color, 1);
		      bdk_color_free (color);
		    }
		  else
		    freeme = get_darkened_gc (window, &style->base[state_type], 1);
		  gc1 = freeme;
		}
	    }
          else if (!strcmp ("cell_even_sorted", detail) ||
                   !strcmp ("cell_odd_sorted", detail) ||
                   !strcmp ("cell_even_ruled_sorted", detail))
            {
	      BdkColor *color = NULL;

	      if (!strcmp ("cell_odd_sorted", detail))
	        btk_widget_style_get (widget,
		                      "odd-row-color", &color,
				      NULL);
	      else
	        btk_widget_style_get (widget,
		                      "even-row-color", &color,
				      NULL);

	      if (color)
	        {
		  freeme = get_darkened_gc (window, color, 1);
		  gc1 = freeme;

		  bdk_color_free (color);
		}
	      else
		{
	          freeme = get_darkened_gc (window, &style->base[state_type], 1);
                  gc1 = freeme;
		}
            }
          else if (!strcmp ("cell_odd_ruled_sorted", detail))
            {
	      BdkColor *color = NULL;

	      btk_widget_style_get (widget,
		                    "odd-row-color", &color,
				    NULL);

	      if (color)
	        {
		  freeme = get_darkened_gc (window, color, 1);
		  gc1 = freeme;

		  bdk_color_free (color);
		}
	      else
	        {
		  btk_widget_style_get (widget,
		                        "even-row-color", &color,
					NULL);

		  if (color)
		    {
		      freeme = get_darkened_gc (window, color, 2);
		      bdk_color_free (color);
		    }
		  else
                    freeme = get_darkened_gc (window, &style->base[state_type], 2);
                  gc1 = freeme;
	        }
            }
          else
            gc1 = style->bg_gc[state_type];
        }
    }
  else
    gc1 = style->bg_gc[state_type];
  
  if (!style->bg_pixmap[state_type] || gc1 != style->bg_gc[state_type] ||
      BDK_IS_PIXMAP (window))
    {
      if (area)
	bdk_gc_set_clip_rectangle (gc1, area);

      bdk_draw_rectangle (window, gc1, TRUE,
                          x, y, width, height);

      if (detail && !strcmp ("tooltip", detail))
        bdk_draw_rectangle (window, style->black_gc, FALSE,
                            x, y, width - 1, height - 1);

      if (area)
	bdk_gc_set_clip_rectangle (gc1, NULL);
    }
  else
    btk_style_apply_default_background (style, window,
                                        widget && btk_widget_get_has_window (widget),
                                        state_type, area, x, y, width, height);


  if (freeme)
    g_object_unref (freeme);
}

static void 
btk_default_draw_check (BtkStyle      *style,
			BdkWindow     *window,
			BtkStateType   state_type,
			BtkShadowType  shadow_type,
			BdkRectangle  *area,
			BtkWidget     *widget,
			const gchar   *detail,
			gint           x,
			gint           y,
			gint           width,
			gint           height)
{
  bairo_t *cr = bdk_bairo_create (window);
  enum { BUTTON, MENU, CELL } type = BUTTON;
  int exterior_size;
  int interior_size;
  int pad;
  
  if (detail)
    {
      if (strcmp (detail, "cellcheck") == 0)
	type = CELL;
      else if (strcmp (detail, "check") == 0)
	type = MENU;
    }
      
  if (area)
    {
      bdk_bairo_rectangle (cr, area);
      bairo_clip (cr);
    }
  
  exterior_size = MIN (width, height);
  if (exterior_size % 2 == 0) /* Ensure odd */
    exterior_size -= 1;

  pad = style->xthickness + MAX (1, (exterior_size - 2 * style->xthickness) / 9);
  interior_size = MAX (1, exterior_size - 2 * pad);

  if (interior_size < 7)
    {
      interior_size = 7;
      pad = MAX (0, (exterior_size - interior_size) / 2);
    }

  x -= (1 + exterior_size - width) / 2;
  y -= (1 + exterior_size - height) / 2;

  switch (type)
    {
    case BUTTON:
    case CELL:
      if (type == BUTTON)
	bdk_bairo_set_source_color (cr, &style->fg[state_type]);
      else
	bdk_bairo_set_source_color (cr, &style->text[state_type]);
	
      bairo_set_line_width (cr, 1.0);
      bairo_rectangle (cr, x + 0.5, y + 0.5, exterior_size - 1, exterior_size - 1);
      bairo_stroke (cr);

      bdk_bairo_set_source_color (cr, &style->base[state_type]);
      bairo_rectangle (cr, x + 1, y + 1, exterior_size - 2, exterior_size - 2);
      bairo_fill (cr);
      break;

    case MENU:
      break;
    }
      
  switch (type)
    {
    case BUTTON:
    case CELL:
      bdk_bairo_set_source_color (cr, &style->text[state_type]);
      break;
    case MENU:
      bdk_bairo_set_source_color (cr, &style->fg[state_type]);
      break;
    }

  if (shadow_type == BTK_SHADOW_IN)
    {
      bairo_translate (cr,
		       x + pad, y + pad);
      
      bairo_scale (cr, interior_size / 7., interior_size / 7.);
      
      bairo_move_to  (cr, 7.0, 0.0);
      bairo_line_to  (cr, 7.5, 1.0);
      bairo_curve_to (cr, 5.3, 2.0,
		      4.3, 4.0,
		      3.5, 7.0);
      bairo_curve_to (cr, 3.0, 5.7,
		      1.3, 4.7,
		      0.0, 4.7);
      bairo_line_to  (cr, 0.2, 3.5);
      bairo_curve_to (cr, 1.1, 3.5,
		      2.3, 4.3,
		      3.0, 5.0);
      bairo_curve_to (cr, 1.0, 3.9,
		      2.4, 4.1,
		      3.2, 4.9);
      bairo_curve_to (cr, 3.5, 3.1,
		      5.2, 2.0,
		      7.0, 0.0);
      
      bairo_fill (cr);
    }
  else if (shadow_type == BTK_SHADOW_ETCHED_IN) /* inconsistent */
    {
      int line_thickness = MAX (1, (3 + interior_size * 2) / 7);

      bairo_rectangle (cr,
		       x + pad,
		       y + pad + (1 + interior_size - line_thickness) / 2,
		       interior_size,
		       line_thickness);
      bairo_fill (cr);
    }
  
  bairo_destroy (cr);
}

static void 
btk_default_draw_option (BtkStyle      *style,
			 BdkWindow     *window,
			 BtkStateType   state_type,
			 BtkShadowType  shadow_type,
			 BdkRectangle  *area,
			 BtkWidget     *widget,
			 const gchar   *detail,
			 gint           x,
			 gint           y,
			 gint           width,
			 gint           height)
{
  bairo_t *cr = bdk_bairo_create (window);
  enum { BUTTON, MENU, CELL } type = BUTTON;
  int exterior_size;
  
  if (detail)
    {
      if (strcmp (detail, "radio") == 0)
	type = CELL;
      else if (strcmp (detail, "option") == 0)
	type = MENU;
    }
      
  if (area)
    {
      bdk_bairo_rectangle (cr, area);
      bairo_clip (cr);
    }
  
  exterior_size = MIN (width, height);
  if (exterior_size % 2 == 0) /* Ensure odd */
    exterior_size -= 1;
  
  x -= (1 + exterior_size - width) / 2;
  y -= (1 + exterior_size - height) / 2;

  switch (type)
    {
    case BUTTON:
    case CELL:
      bdk_bairo_set_source_color (cr, &style->base[state_type]);
      
      bairo_arc (cr,
		 x + exterior_size / 2.,
		 y + exterior_size / 2.,
		 (exterior_size - 1) / 2.,
		 0, 2 * G_PI);

      bairo_fill_preserve (cr);

      if (type == BUTTON)
	bdk_bairo_set_source_color (cr, &style->fg[state_type]);
      else
	bdk_bairo_set_source_color (cr, &style->text[state_type]);
	
      bairo_set_line_width (cr, 1.);
      bairo_stroke (cr);
      break;

    case MENU:
      break;
    }
      
  switch (type)
    {
    case BUTTON:
      bdk_bairo_set_source_color (cr, &style->text[state_type]);
      break;
    case CELL:
      break;
    case MENU:
      bdk_bairo_set_source_color (cr, &style->fg[state_type]);
      break;
    }

  if (shadow_type == BTK_SHADOW_IN)
    {
      int pad = style->xthickness + MAX (1, 2 * (exterior_size - 2 * style->xthickness) / 9);
      int interior_size = MAX (1, exterior_size - 2 * pad);

      if (interior_size < 5)
	{
	  interior_size = 7;
	  pad = MAX (0, (exterior_size - interior_size) / 2);
	}

      bairo_arc (cr,
		 x + pad + interior_size / 2.,
		 y + pad + interior_size / 2.,
		 interior_size / 2.,
		 0, 2 * G_PI);
      bairo_fill (cr);
    }
  else if (shadow_type == BTK_SHADOW_ETCHED_IN) /* inconsistent */
    {
      int pad = style->xthickness + MAX (1, (exterior_size - 2 * style->xthickness) / 9);
      int interior_size = MAX (1, exterior_size - 2 * pad);
      int line_thickness;

      if (interior_size < 7)
	{
	  interior_size = 7;
	  pad = MAX (0, (exterior_size - interior_size) / 2);
	}

      line_thickness = MAX (1, (3 + interior_size * 2) / 7);

      bairo_rectangle (cr,
		       x + pad,
		       y + pad + (interior_size - line_thickness) / 2.,
		       interior_size,
		       line_thickness);
      bairo_fill (cr);
    }
  
  bairo_destroy (cr);
}

static void
btk_default_draw_tab (BtkStyle      *style,
		      BdkWindow     *window,
		      BtkStateType   state_type,
		      BtkShadowType  shadow_type,
		      BdkRectangle  *area,
		      BtkWidget     *widget,
		      const gchar   *detail,
		      gint           x,
		      gint           y,
		      gint           width,
		      gint           height)
{
#define ARROW_SPACE 4

  BtkRequisition indicator_size;
  BtkBorder indicator_spacing;
  gint arrow_height;
  
  option_menu_get_props (widget, &indicator_size, &indicator_spacing);

  indicator_size.width += (indicator_size.width % 2) - 1;
  arrow_height = indicator_size.width / 2 + 1;

  x += (width - indicator_size.width) / 2;
  y += (height - (2 * arrow_height + ARROW_SPACE)) / 2;

  if (state_type == BTK_STATE_INSENSITIVE)
    {
      draw_arrow (window, &style->white, area,
		  BTK_ARROW_UP, x + 1, y + 1,
		  indicator_size.width, arrow_height);
      
      draw_arrow (window, &style->white, area,
		  BTK_ARROW_DOWN, x + 1, y + arrow_height + ARROW_SPACE + 1,
		  indicator_size.width, arrow_height);
    }
  
  draw_arrow (window, &style->fg[state_type], area,
	      BTK_ARROW_UP, x, y,
	      indicator_size.width, arrow_height);
  
  
  draw_arrow (window, &style->fg[state_type], area,
	      BTK_ARROW_DOWN, x, y + arrow_height + ARROW_SPACE,
	      indicator_size.width, arrow_height);
}

static void 
btk_default_draw_shadow_gap (BtkStyle       *style,
                             BdkWindow      *window,
                             BtkStateType    state_type,
                             BtkShadowType   shadow_type,
                             BdkRectangle   *area,
                             BtkWidget      *widget,
                             const gchar    *detail,
                             gint            x,
                             gint            y,
                             gint            width,
                             gint            height,
                             BtkPositionType gap_side,
                             gint            gap_x,
                             gint            gap_width)
{
  BdkGC *gc1 = NULL;
  BdkGC *gc2 = NULL;
  BdkGC *gc3 = NULL;
  BdkGC *gc4 = NULL;
  
  sanitize_size (window, &width, &height);
  
  switch (shadow_type)
    {
    case BTK_SHADOW_NONE:
      return;
    case BTK_SHADOW_IN:
      gc1 = style->dark_gc[state_type];
      gc2 = style->black_gc;
      gc3 = style->bg_gc[state_type];
      gc4 = style->light_gc[state_type];
      break;
    case BTK_SHADOW_ETCHED_IN:
      gc1 = style->dark_gc[state_type];
      gc2 = style->light_gc[state_type];
      gc3 = style->dark_gc[state_type];
      gc4 = style->light_gc[state_type];
      break;
    case BTK_SHADOW_OUT:
      gc1 = style->light_gc[state_type];
      gc2 = style->bg_gc[state_type];
      gc3 = style->dark_gc[state_type];
      gc4 = style->black_gc;
      break;
    case BTK_SHADOW_ETCHED_OUT:
      gc1 = style->light_gc[state_type];
      gc2 = style->dark_gc[state_type];
      gc3 = style->light_gc[state_type];
      gc4 = style->dark_gc[state_type];
      break;
    }
  if (area)
    {
      bdk_gc_set_clip_rectangle (gc1, area);
      bdk_gc_set_clip_rectangle (gc2, area);
      bdk_gc_set_clip_rectangle (gc3, area);
      bdk_gc_set_clip_rectangle (gc4, area);
    }
  
  switch (shadow_type)
    {
    case BTK_SHADOW_NONE:
    case BTK_SHADOW_IN:
    case BTK_SHADOW_OUT:
    case BTK_SHADOW_ETCHED_IN:
    case BTK_SHADOW_ETCHED_OUT:
      switch (gap_side)
        {
        case BTK_POS_TOP:
          bdk_draw_line (window, gc1,
                         x, y, x, y + height - 1);
          bdk_draw_line (window, gc2,
                         x + 1, y, x + 1, y + height - 2);
          
          bdk_draw_line (window, gc3,
                         x + 1, y + height - 2, x + width - 2, y + height - 2);
          bdk_draw_line (window, gc3,
                         x + width - 2, y, x + width - 2, y + height - 2);
          bdk_draw_line (window, gc4,
                         x, y + height - 1, x + width - 1, y + height - 1);
          bdk_draw_line (window, gc4,
                         x + width - 1, y, x + width - 1, y + height - 1);
          if (gap_x > 0)
            {
              bdk_draw_line (window, gc1,
                             x, y, x + gap_x - 1, y);
              bdk_draw_line (window, gc2,
                             x + 1, y + 1, x + gap_x - 1, y + 1);
              bdk_draw_line (window, gc2,
                             x + gap_x, y, x + gap_x, y);
            }
          if ((width - (gap_x + gap_width)) > 0)
            {
              bdk_draw_line (window, gc1,
                             x + gap_x + gap_width, y, x + width - 2, y);
              bdk_draw_line (window, gc2,
                             x + gap_x + gap_width, y + 1, x + width - 3, y + 1);
              bdk_draw_line (window, gc2,
                             x + gap_x + gap_width - 1, y, x + gap_x + gap_width - 1, y);
            }
          break;
        case BTK_POS_BOTTOM:
          bdk_draw_line (window, gc1,
                         x, y, x + width - 1, y);
          bdk_draw_line (window, gc1,
                         x, y, x, y + height - 1);
          bdk_draw_line (window, gc2,
                         x + 1, y + 1, x + width - 2, y + 1);
          bdk_draw_line (window, gc2,
                         x + 1, y + 1, x + 1, y + height - 1);
          
          bdk_draw_line (window, gc3,
                         x + width - 2, y + 1, x + width - 2, y + height - 1);
          bdk_draw_line (window, gc4,
                         x + width - 1, y, x + width - 1, y + height - 1);
          if (gap_x > 0)
            {
              bdk_draw_line (window, gc4,
                             x, y + height - 1, x + gap_x - 1, y + height - 1);
              bdk_draw_line (window, gc3,
                             x + 1, y + height - 2, x + gap_x - 1, y + height - 2);
              bdk_draw_line (window, gc3,
                             x + gap_x, y + height - 1, x + gap_x, y + height - 1);
            }
          if ((width - (gap_x + gap_width)) > 0)
            {
              bdk_draw_line (window, gc4,
                             x + gap_x + gap_width, y + height - 1, x + width - 2, y + height - 1);
              bdk_draw_line (window, gc3,
                             x + gap_x + gap_width, y + height - 2, x + width - 2, y + height - 2);
              bdk_draw_line (window, gc3,
                             x + gap_x + gap_width - 1, y + height - 1, x + gap_x + gap_width - 1, y + height - 1);
            }
          break;
        case BTK_POS_LEFT:
          bdk_draw_line (window, gc1,
                         x, y, x + width - 1, y);
          bdk_draw_line (window, gc2,
                         x, y + 1, x + width - 2, y + 1);
          
          bdk_draw_line (window, gc3,
                         x, y + height - 2, x + width - 2, y + height - 2);
          bdk_draw_line (window, gc3,
                         x + width - 2, y + 1, x + width - 2, y + height - 2);
          bdk_draw_line (window, gc4,
                         x, y + height - 1, x + width - 1, y + height - 1);
          bdk_draw_line (window, gc4,
                         x + width - 1, y, x + width - 1, y + height - 1);
          if (gap_x > 0)
            {
              bdk_draw_line (window, gc1,
                             x, y, x, y + gap_x - 1);
              bdk_draw_line (window, gc2,
                             x + 1, y + 1, x + 1, y + gap_x - 1);
              bdk_draw_line (window, gc2,
                             x, y + gap_x, x, y + gap_x);
            }
          if ((width - (gap_x + gap_width)) > 0)
            {
              bdk_draw_line (window, gc1,
                             x, y + gap_x + gap_width, x, y + height - 2);
              bdk_draw_line (window, gc2,
                             x + 1, y + gap_x + gap_width, x + 1, y + height - 2);
              bdk_draw_line (window, gc2,
                             x, y + gap_x + gap_width - 1, x, y + gap_x + gap_width - 1);
            }
          break;
        case BTK_POS_RIGHT:
          bdk_draw_line (window, gc1,
                         x, y, x + width - 1, y);
          bdk_draw_line (window, gc1,
                         x, y, x, y + height - 1);
          bdk_draw_line (window, gc2,
                         x + 1, y + 1, x + width - 1, y + 1);
          bdk_draw_line (window, gc2,
                         x + 1, y + 1, x + 1, y + height - 2);
          
          bdk_draw_line (window, gc3,
                         x + 1, y + height - 2, x + width - 1, y + height - 2);
          bdk_draw_line (window, gc4,
                         x, y + height - 1, x + width - 1, y + height - 1);
          if (gap_x > 0)
            {
              bdk_draw_line (window, gc4,
                             x + width - 1, y, x + width - 1, y + gap_x - 1);
              bdk_draw_line (window, gc3,
                             x + width - 2, y + 1, x + width - 2, y + gap_x - 1);
              bdk_draw_line (window, gc3,
                             x + width - 1, y + gap_x, x + width - 1, y + gap_x);
            }
          if ((width - (gap_x + gap_width)) > 0)
            {
              bdk_draw_line (window, gc4,
                             x + width - 1, y + gap_x + gap_width, x + width - 1, y + height - 2);
              bdk_draw_line (window, gc3,
                             x + width - 2, y + gap_x + gap_width, x + width - 2, y + height - 2);
              bdk_draw_line (window, gc3,
                             x + width - 1, y + gap_x + gap_width - 1, x + width - 1, y + gap_x + gap_width - 1);
            }
          break;
        }
    }

  if (area)
    {
      bdk_gc_set_clip_rectangle (gc1, NULL);
      bdk_gc_set_clip_rectangle (gc2, NULL);
      bdk_gc_set_clip_rectangle (gc3, NULL);
      bdk_gc_set_clip_rectangle (gc4, NULL);
    }
}

static void 
btk_default_draw_box_gap (BtkStyle       *style,
                          BdkWindow      *window,
                          BtkStateType    state_type,
                          BtkShadowType   shadow_type,
                          BdkRectangle   *area,
                          BtkWidget      *widget,
                          const gchar    *detail,
                          gint            x,
                          gint            y,
                          gint            width,
                          gint            height,
                          BtkPositionType gap_side,
                          gint            gap_x,
                          gint            gap_width)
{
  BdkGC *gc1 = NULL;
  BdkGC *gc2 = NULL;
  BdkGC *gc3 = NULL;
  BdkGC *gc4 = NULL;
  
  btk_style_apply_default_background (style, window,
                                      widget && btk_widget_get_has_window (widget),
                                      state_type, area, x, y, width, height);
  
  sanitize_size (window, &width, &height);
  
  switch (shadow_type)
    {
    case BTK_SHADOW_NONE:
      return;
    case BTK_SHADOW_IN:
      gc1 = style->dark_gc[state_type];
      gc2 = style->black_gc;
      gc3 = style->bg_gc[state_type];
      gc4 = style->light_gc[state_type];
      break;
    case BTK_SHADOW_ETCHED_IN:
      gc1 = style->dark_gc[state_type];
      gc2 = style->light_gc[state_type];
      gc3 = style->dark_gc[state_type];
      gc4 = style->light_gc[state_type];
      break;
    case BTK_SHADOW_OUT:
      gc1 = style->light_gc[state_type];
      gc2 = style->bg_gc[state_type];
      gc3 = style->dark_gc[state_type];
      gc4 = style->black_gc;
      break;
    case BTK_SHADOW_ETCHED_OUT:
      gc1 = style->light_gc[state_type];
      gc2 = style->dark_gc[state_type];
      gc3 = style->light_gc[state_type];
      gc4 = style->dark_gc[state_type];
      break;
    }

  if (area)
    {
      bdk_gc_set_clip_rectangle (gc1, area);
      bdk_gc_set_clip_rectangle (gc2, area);
      bdk_gc_set_clip_rectangle (gc3, area);
      bdk_gc_set_clip_rectangle (gc4, area);
    }
  
  switch (shadow_type)
    {
    case BTK_SHADOW_NONE:
    case BTK_SHADOW_IN:
    case BTK_SHADOW_OUT:
    case BTK_SHADOW_ETCHED_IN:
    case BTK_SHADOW_ETCHED_OUT:
      switch (gap_side)
        {
        case BTK_POS_TOP:
          bdk_draw_line (window, gc1,
                         x, y, x, y + height - 1);
          bdk_draw_line (window, gc2,
                         x + 1, y, x + 1, y + height - 2);
          
          bdk_draw_line (window, gc3,
                         x + 1, y + height - 2, x + width - 2, y + height - 2);
          bdk_draw_line (window, gc3,
                         x + width - 2, y, x + width - 2, y + height - 2);
          bdk_draw_line (window, gc4,
                         x, y + height - 1, x + width - 1, y + height - 1);
          bdk_draw_line (window, gc4,
                         x + width - 1, y, x + width - 1, y + height - 1);
          if (gap_x > 0)
            {
              bdk_draw_line (window, gc1,
                             x, y, x + gap_x - 1, y);
              bdk_draw_line (window, gc2,
                             x + 1, y + 1, x + gap_x - 1, y + 1);
              bdk_draw_line (window, gc2,
                             x + gap_x, y, x + gap_x, y);
            }
          if ((width - (gap_x + gap_width)) > 0)
            {
              bdk_draw_line (window, gc1,
                             x + gap_x + gap_width, y, x + width - 2, y);
              bdk_draw_line (window, gc2,
                             x + gap_x + gap_width, y + 1, x + width - 2, y + 1);
              bdk_draw_line (window, gc2,
                             x + gap_x + gap_width - 1, y, x + gap_x + gap_width - 1, y);
            }
          break;
        case  BTK_POS_BOTTOM:
          bdk_draw_line (window, gc1,
                         x, y, x + width - 1, y);
          bdk_draw_line (window, gc1,
                         x, y, x, y + height - 1);
          bdk_draw_line (window, gc2,
                         x + 1, y + 1, x + width - 2, y + 1);
          bdk_draw_line (window, gc2,
                         x + 1, y + 1, x + 1, y + height - 1);
          
          bdk_draw_line (window, gc3,
                         x + width - 2, y + 1, x + width - 2, y + height - 1);
          bdk_draw_line (window, gc4,
                         x + width - 1, y, x + width - 1, y + height - 1);
          if (gap_x > 0)
            {
              bdk_draw_line (window, gc4,
                             x, y + height - 1, x + gap_x - 1, y + height - 1);
              bdk_draw_line (window, gc3,
                             x + 1, y + height - 2, x + gap_x - 1, y + height - 2);
              bdk_draw_line (window, gc3,
                             x + gap_x, y + height - 1, x + gap_x, y + height - 1);
            }
          if ((width - (gap_x + gap_width)) > 0)
            {
              bdk_draw_line (window, gc4,
                             x + gap_x + gap_width, y + height - 1, x + width - 2, y + height - 1);
              bdk_draw_line (window, gc3,
                             x + gap_x + gap_width, y + height - 2, x + width - 2, y + height - 2);
              bdk_draw_line (window, gc3,
                             x + gap_x + gap_width - 1, y + height - 1, x + gap_x + gap_width - 1, y + height - 1);
            }
          break;
        case BTK_POS_LEFT:
          bdk_draw_line (window, gc1,
                         x, y, x + width - 1, y);
          bdk_draw_line (window, gc2,
                         x, y + 1, x + width - 2, y + 1);
          
          bdk_draw_line (window, gc3,
                         x, y + height - 2, x + width - 2, y + height - 2);
          bdk_draw_line (window, gc3,
                         x + width - 2, y + 1, x + width - 2, y + height - 2);
          bdk_draw_line (window, gc4,
                         x, y + height - 1, x + width - 1, y + height - 1);
          bdk_draw_line (window, gc4,
                         x + width - 1, y, x + width - 1, y + height - 1);
          if (gap_x > 0)
            {
              bdk_draw_line (window, gc1,
                             x, y, x, y + gap_x - 1);
              bdk_draw_line (window, gc2,
                             x + 1, y + 1, x + 1, y + gap_x - 1);
              bdk_draw_line (window, gc2,
                             x, y + gap_x, x, y + gap_x);
            }
          if ((height - (gap_x + gap_width)) > 0)
            {
              bdk_draw_line (window, gc1,
                             x, y + gap_x + gap_width, x, y + height - 2);
              bdk_draw_line (window, gc2,
                             x + 1, y + gap_x + gap_width, x + 1, y + height - 2);
              bdk_draw_line (window, gc2,
                             x, y + gap_x + gap_width - 1, x, y + gap_x + gap_width - 1);
            }
          break;
        case BTK_POS_RIGHT:
          bdk_draw_line (window, gc1,
                         x, y, x + width - 1, y);
          bdk_draw_line (window, gc1,
                         x, y, x, y + height - 1);
          bdk_draw_line (window, gc2,
                         x + 1, y + 1, x + width - 1, y + 1);
          bdk_draw_line (window, gc2,
                         x + 1, y + 1, x + 1, y + height - 2);
          
          bdk_draw_line (window, gc3,
                         x + 1, y + height - 2, x + width - 1, y + height - 2);
          bdk_draw_line (window, gc4,
                         x, y + height - 1, x + width - 1, y + height - 1);
          if (gap_x > 0)
            {
              bdk_draw_line (window, gc4,
                             x + width - 1, y, x + width - 1, y + gap_x - 1);
              bdk_draw_line (window, gc3,
                             x + width - 2, y + 1, x + width - 2, y + gap_x - 1);
              bdk_draw_line (window, gc3,
                             x + width - 1, y + gap_x, x + width - 1, y + gap_x);
            }
          if ((height - (gap_x + gap_width)) > 0)
            {
              bdk_draw_line (window, gc4,
                             x + width - 1, y + gap_x + gap_width, x + width - 1, y + height - 2);
              bdk_draw_line (window, gc3,
                             x + width - 2, y + gap_x + gap_width, x + width - 2, y + height - 2);
              bdk_draw_line (window, gc3,
                             x + width - 1, y + gap_x + gap_width - 1, x + width - 1, y + gap_x + gap_width - 1);
            }
          break;
        }
    }

  if (area)
    {
      bdk_gc_set_clip_rectangle (gc1, NULL);
      bdk_gc_set_clip_rectangle (gc2, NULL);
      bdk_gc_set_clip_rectangle (gc3, NULL);
      bdk_gc_set_clip_rectangle (gc4, NULL);
    }
}

static void 
btk_default_draw_extension (BtkStyle       *style,
                            BdkWindow      *window,
                            BtkStateType    state_type,
                            BtkShadowType   shadow_type,
                            BdkRectangle   *area,
                            BtkWidget      *widget,
                            const gchar    *detail,
                            gint            x,
                            gint            y,
                            gint            width,
                            gint            height,
                            BtkPositionType gap_side)
{
  BdkGC *gc1 = NULL;
  BdkGC *gc2 = NULL;
  BdkGC *gc3 = NULL;
  BdkGC *gc4 = NULL;
  
  btk_style_apply_default_background (style, window,
                                      widget && btk_widget_get_has_window (widget),
                                      BTK_STATE_NORMAL, area, x, y, width, height);
  
  sanitize_size (window, &width, &height);
  
  switch (shadow_type)
    {
    case BTK_SHADOW_NONE:
      return;
    case BTK_SHADOW_IN:
      gc1 = style->dark_gc[state_type];
      gc2 = style->black_gc;
      gc3 = style->bg_gc[state_type];
      gc4 = style->light_gc[state_type];
      break;
    case BTK_SHADOW_ETCHED_IN:
      gc1 = style->dark_gc[state_type];
      gc2 = style->light_gc[state_type];
      gc3 = style->dark_gc[state_type];
      gc4 = style->light_gc[state_type];
      break;
    case BTK_SHADOW_OUT:
      gc1 = style->light_gc[state_type];
      gc2 = style->bg_gc[state_type];
      gc3 = style->dark_gc[state_type];
      gc4 = style->black_gc;
      break;
    case BTK_SHADOW_ETCHED_OUT:
      gc1 = style->light_gc[state_type];
      gc2 = style->dark_gc[state_type];
      gc3 = style->light_gc[state_type];
      gc4 = style->dark_gc[state_type];
      break;
    }

  if (area)
    {
      bdk_gc_set_clip_rectangle (gc1, area);
      bdk_gc_set_clip_rectangle (gc2, area);
      bdk_gc_set_clip_rectangle (gc3, area);
      bdk_gc_set_clip_rectangle (gc4, area);
    }

  switch (shadow_type)
    {
    case BTK_SHADOW_NONE:
    case BTK_SHADOW_IN:
    case BTK_SHADOW_OUT:
    case BTK_SHADOW_ETCHED_IN:
    case BTK_SHADOW_ETCHED_OUT:
      switch (gap_side)
        {
        case BTK_POS_TOP:
          btk_style_apply_default_background (style, window,
                                              widget && btk_widget_get_has_window (widget),
                                              state_type, area,
                                              x + style->xthickness, 
                                              y, 
                                              width - (2 * style->xthickness), 
                                              height - (style->ythickness));
          bdk_draw_line (window, gc1,
                         x, y, x, y + height - 2);
          bdk_draw_line (window, gc2,
                         x + 1, y, x + 1, y + height - 2);
          
          bdk_draw_line (window, gc3,
                         x + 2, y + height - 2, x + width - 2, y + height - 2);
          bdk_draw_line (window, gc3,
                         x + width - 2, y, x + width - 2, y + height - 2);
          bdk_draw_line (window, gc4,
                         x + 1, y + height - 1, x + width - 2, y + height - 1);
          bdk_draw_line (window, gc4,
                         x + width - 1, y, x + width - 1, y + height - 2);
          break;
        case BTK_POS_BOTTOM:
          btk_style_apply_default_background (style, window,
                                              widget && btk_widget_get_has_window (widget),
                                              state_type, area,
                                              x + style->xthickness, 
                                              y + style->ythickness, 
                                              width - (2 * style->xthickness), 
                                              height - (style->ythickness));
          bdk_draw_line (window, gc1,
                         x + 1, y, x + width - 2, y);
          bdk_draw_line (window, gc1,
                         x, y + 1, x, y + height - 1);
          bdk_draw_line (window, gc2,
                         x + 1, y + 1, x + width - 2, y + 1);
          bdk_draw_line (window, gc2,
                         x + 1, y + 1, x + 1, y + height - 1);
          
          bdk_draw_line (window, gc3,
                         x + width - 2, y + 2, x + width - 2, y + height - 1);
          bdk_draw_line (window, gc4,
                         x + width - 1, y + 1, x + width - 1, y + height - 1);
          break;
        case BTK_POS_LEFT:
          btk_style_apply_default_background (style, window,
                                              widget && btk_widget_get_has_window (widget),
                                              state_type, area,
                                              x, 
                                              y + style->ythickness, 
                                              width - (style->xthickness), 
                                              height - (2 * style->ythickness));
          bdk_draw_line (window, gc1,
                         x, y, x + width - 2, y);
          bdk_draw_line (window, gc2,
                         x + 1, y + 1, x + width - 2, y + 1);
          
          bdk_draw_line (window, gc3,
                         x, y + height - 2, x + width - 2, y + height - 2);
          bdk_draw_line (window, gc3,
                         x + width - 2, y + 2, x + width - 2, y + height - 2);
          bdk_draw_line (window, gc4,
                         x, y + height - 1, x + width - 2, y + height - 1);
          bdk_draw_line (window, gc4,
                         x + width - 1, y + 1, x + width - 1, y + height - 2);
          break;
        case BTK_POS_RIGHT:
          btk_style_apply_default_background (style, window,
                                              widget && btk_widget_get_has_window (widget),
                                              state_type, area,
                                              x + style->xthickness, 
                                              y + style->ythickness, 
                                              width - (style->xthickness), 
                                              height - (2 * style->ythickness));
          bdk_draw_line (window, gc1,
                         x + 1, y, x + width - 1, y);
          bdk_draw_line (window, gc1,
                         x, y + 1, x, y + height - 2);
          bdk_draw_line (window, gc2,
                         x + 1, y + 1, x + width - 1, y + 1);
          bdk_draw_line (window, gc2,
                         x + 1, y + 1, x + 1, y + height - 2);
          
          bdk_draw_line (window, gc3,
                         x + 2, y + height - 2, x + width - 1, y + height - 2);
          bdk_draw_line (window, gc4,
                         x + 1, y + height - 1, x + width - 1, y + height - 1);
          break;
        }
    }

  if (area)
    {
      bdk_gc_set_clip_rectangle (gc1, NULL);
      bdk_gc_set_clip_rectangle (gc2, NULL);
      bdk_gc_set_clip_rectangle (gc3, NULL);
      bdk_gc_set_clip_rectangle (gc4, NULL);
    }
}

static void 
btk_default_draw_focus (BtkStyle      *style,
			BdkWindow     *window,
			BtkStateType   state_type,
			BdkRectangle  *area,
			BtkWidget     *widget,
			const gchar   *detail,
			gint           x,
			gint           y,
			gint           width,
			gint           height)
{
  bairo_t *cr;
  gboolean free_dash_list = FALSE;
  gint line_width = 1;
  gint8 *dash_list = (gint8 *) "\1\1";

  if (widget)
    {
      btk_widget_style_get (widget,
			    "focus-line-width", &line_width,
			    "focus-line-pattern", (gchar *)&dash_list,
			    NULL);

      free_dash_list = TRUE;
  }

  if (detail && !strcmp (detail, "add-mode"))
    {
      if (free_dash_list)
	g_free (dash_list);

      dash_list = (gint8 *) "\4\4";
      free_dash_list = FALSE;
    }

  sanitize_size (window, &width, &height);

  cr = bdk_bairo_create (window);

  if (detail && !strcmp (detail, "colorwheel_light"))
    bairo_set_source_rgb (cr, 0., 0., 0.);
  else if (detail && !strcmp (detail, "colorwheel_dark"))
    bairo_set_source_rgb (cr, 1., 1., 1.);
  else
    bdk_bairo_set_source_color (cr, &style->fg[state_type]);

  bairo_set_line_width (cr, line_width);

  if (dash_list[0])
    {
      gint n_dashes = strlen ((const gchar *) dash_list);
      gdouble *dashes = g_new (gdouble, n_dashes);
      gdouble total_length = 0;
      gdouble dash_offset;
      gint i;

      for (i = 0; i < n_dashes; i++)
	{
	  dashes[i] = dash_list[i];
	  total_length += dash_list[i];
	}

      /* The dash offset here aligns the pattern to integer pixels
       * by starting the dash at the right side of the left border
       * Negative dash offsets in bairo don't work
       * (https://bugs.freedesktop.org/show_bug.cgi?id=2729)
       */
      dash_offset = - line_width / 2.;
      while (dash_offset < 0)
	dash_offset += total_length;
      
      bairo_set_dash (cr, dashes, n_dashes, dash_offset);
      g_free (dashes);
    }

  if (area)
    {
      bdk_bairo_rectangle (cr, area);
      bairo_clip (cr);
    }

  bairo_rectangle (cr,
		   x + line_width / 2.,
		   y + line_width / 2.,
		   width - line_width,
		   height - line_width);
  bairo_stroke (cr);
  bairo_destroy (cr);

  if (free_dash_list)
    g_free (dash_list);
}

static void 
btk_default_draw_slider (BtkStyle      *style,
                         BdkWindow     *window,
                         BtkStateType   state_type,
                         BtkShadowType  shadow_type,
                         BdkRectangle  *area,
                         BtkWidget     *widget,
                         const gchar   *detail,
                         gint           x,
                         gint           y,
                         gint           width,
                         gint           height,
                         BtkOrientation orientation)
{
  sanitize_size (window, &width, &height);
  
  btk_paint_box (style, window, state_type, shadow_type,
                 area, widget, detail, x, y, width, height);

  if (detail &&
      (strcmp ("hscale", detail) == 0 ||
       strcmp ("vscale", detail) == 0))
    {
      if (orientation == BTK_ORIENTATION_HORIZONTAL)
        btk_paint_vline (style, window, state_type, area, widget, detail, 
                         y + style->ythickness, 
                         y + height - style->ythickness - 1, x + width / 2);
      else
        btk_paint_hline (style, window, state_type, area, widget, detail, 
                         x + style->xthickness, 
                         x + width - style->xthickness - 1, y + height / 2);
    }
}

static void
draw_dot (BdkWindow    *window,
	  BdkGC        *light_gc,
	  BdkGC        *dark_gc,
	  gint          x,
	  gint          y,
	  gushort       size)
{
  size = CLAMP (size, 2, 3);

  if (size == 2)
    {
      bdk_draw_point (window, light_gc, x, y);
      bdk_draw_point (window, light_gc, x+1, y+1);
    }
  else if (size == 3)
    {
      bdk_draw_point (window, light_gc, x, y);
      bdk_draw_point (window, light_gc, x+1, y);
      bdk_draw_point (window, light_gc, x, y+1);
      bdk_draw_point (window, dark_gc, x+1, y+2);
      bdk_draw_point (window, dark_gc, x+2, y+1);
      bdk_draw_point (window, dark_gc, x+2, y+2);
    }
}

static void 
btk_default_draw_handle (BtkStyle      *style,
			 BdkWindow     *window,
			 BtkStateType   state_type,
			 BtkShadowType  shadow_type,
			 BdkRectangle  *area,
			 BtkWidget     *widget,
			 const gchar   *detail,
			 gint           x,
			 gint           y,
			 gint           width,
			 gint           height,
			 BtkOrientation orientation)
{
  gint xx, yy;
  gint xthick, ythick;
  BdkGC *light_gc, *dark_gc;
  BdkGC *free_me = NULL;
  BdkRectangle rect;
  BdkRectangle dest;
  gint intersect;
  
  sanitize_size (window, &width, &height);
  
  btk_paint_box (style, window, state_type, shadow_type, area, widget, 
                 detail, x, y, width, height);
  
  
  if (detail && !strcmp (detail, "paned"))
    {
      /* we want to ignore the shadow border in paned widgets */
      xthick = 0;
      ythick = 0;

      if (state_type == BTK_STATE_SELECTED && widget && !btk_widget_has_focus (widget))
	{
	  BdkColor unfocused_light;

	  _btk_style_shade (&style->base[BTK_STATE_ACTIVE], &unfocused_light,
                            LIGHTNESS_MULT);

	  light_gc = free_me = bdk_gc_new (window);
	  bdk_gc_set_rgb_fg_color (light_gc, &unfocused_light);
	}
      else
	light_gc = style->light_gc[state_type];

      dark_gc = style->black_gc;
    }
  else
    {
      xthick = style->xthickness;
      ythick = style->ythickness;

      light_gc = style->light_gc[state_type];
      dark_gc = style->dark_gc[state_type];
    }
  
  rect.x = x + xthick;
  rect.y = y + ythick;
  rect.width = width - (xthick * 2);
  rect.height = height - (ythick * 2);

  if (area)
    intersect = bdk_rectangle_intersect (area, &rect, &dest);
  else
    {
      intersect = TRUE;
      dest = rect;
    }

  if (!intersect)
    goto out;

  bdk_gc_set_clip_rectangle (light_gc, &dest);
  bdk_gc_set_clip_rectangle (dark_gc, &dest);

  if (detail && !strcmp (detail, "paned"))
    {
      if (orientation == BTK_ORIENTATION_HORIZONTAL)
	for (xx = x + width/2 - 15; xx <= x + width/2 + 15; xx += 5)
	  draw_dot (window, light_gc, dark_gc, xx, y + height/2 - 1, 3);
      else
	for (yy = y + height/2 - 15; yy <= y + height/2 + 15; yy += 5)
	  draw_dot (window, light_gc, dark_gc, x + width/2 - 1, yy, 3);
    }
  else
    {
      for (yy = y + ythick; yy < (y + height - ythick); yy += 3)
	for (xx = x + xthick; xx < (x + width - xthick); xx += 6)
	  {
	    draw_dot (window, light_gc, dark_gc, xx, yy, 2);
	    draw_dot (window, light_gc, dark_gc, xx + 3, yy + 1, 2);
	  }
    }

  bdk_gc_set_clip_rectangle (light_gc, NULL);
  bdk_gc_set_clip_rectangle (dark_gc, NULL);

 out:
  if (free_me)
    g_object_unref (free_me);
}

static void
btk_default_draw_expander (BtkStyle        *style,
                           BdkWindow       *window,
                           BtkStateType     state_type,
                           BdkRectangle    *area,
                           BtkWidget       *widget,
                           const gchar     *detail,
                           gint             x,
                           gint             y,
			   BtkExpanderStyle expander_style)
{
#define DEFAULT_EXPANDER_SIZE 12

  gint expander_size;
  gint line_width;
  double vertical_overshoot;
  int diameter;
  double radius;
  double interp;		/* interpolation factor for center position */
  double x_double_horz, y_double_horz;
  double x_double_vert, y_double_vert;
  double x_double, y_double;
  gint degrees = 0;

  bairo_t *cr = bdk_bairo_create (window);
  
  if (area)
    {
      bdk_bairo_rectangle (cr, area);
      bairo_clip (cr);
    }

  if (widget &&
      btk_widget_class_find_style_property (BTK_WIDGET_GET_CLASS (widget),
					    "expander-size"))
    {
      btk_widget_style_get (widget,
			    "expander-size", &expander_size,
			    NULL);
    }
  else
    expander_size = DEFAULT_EXPANDER_SIZE;
    
  line_width = MAX (1, expander_size/9);

  switch (expander_style)
    {
    case BTK_EXPANDER_COLLAPSED:
      degrees = (get_direction (widget) == BTK_TEXT_DIR_RTL) ? 180 : 0;
      interp = 0.0;
      break;
    case BTK_EXPANDER_SEMI_COLLAPSED:
      degrees = (get_direction (widget) == BTK_TEXT_DIR_RTL) ? 150 : 30;
      interp = 0.25;
      break;
    case BTK_EXPANDER_SEMI_EXPANDED:
      degrees = (get_direction (widget) == BTK_TEXT_DIR_RTL) ? 120 : 60;
      interp = 0.75;
      break;
    case BTK_EXPANDER_EXPANDED:
      degrees = 90;
      interp = 1.0;
      break;
    default:
      g_assert_not_reached ();
    }

  /* Compute distance that the stroke extends beyonds the end
   * of the triangle we draw.
   */
  vertical_overshoot = line_width / 2.0 * (1. / tan (G_PI / 8));

  /* For odd line widths, we end the vertical line of the triangle
   * at a half pixel, so we round differently.
   */
  if (line_width % 2 == 1)
    vertical_overshoot = ceil (0.5 + vertical_overshoot) - 0.5;
  else
    vertical_overshoot = ceil (vertical_overshoot);

  /* Adjust the size of the triangle we draw so that the entire stroke fits
   */
  diameter = MAX (3, expander_size - 2 * vertical_overshoot);

  /* If the line width is odd, we want the diameter to be even,
   * and vice versa, so force the sum to be odd. This relationship
   * makes the point of the triangle look right.
   */
  diameter -= (1 - (diameter + line_width) % 2);
  
  radius = diameter / 2.;

  /* Adjust the center so that the stroke is properly aligned with
   * the pixel grid. The center adjustment is different for the
   * horizontal and vertical orientations. For intermediate positions
   * we interpolate between the two.
   */
  x_double_vert = floor (x - (radius + line_width) / 2.) + (radius + line_width) / 2.;
  y_double_vert = y - 0.5;

  x_double_horz = x - 0.5;
  y_double_horz = floor (y - (radius + line_width) / 2.) + (radius + line_width) / 2.;

  x_double = x_double_vert * (1 - interp) + x_double_horz * interp;
  y_double = y_double_vert * (1 - interp) + y_double_horz * interp;
  
  bairo_translate (cr, x_double, y_double);
  bairo_rotate (cr, degrees * G_PI / 180);

  bairo_move_to (cr, - radius / 2., - radius);
  bairo_line_to (cr,   radius / 2.,   0);
  bairo_line_to (cr, - radius / 2.,   radius);
  bairo_close_path (cr);
  
  bairo_set_line_width (cr, line_width);

  if (state_type == BTK_STATE_PRELIGHT)
    bdk_bairo_set_source_color (cr,
				&style->fg[BTK_STATE_PRELIGHT]);
  else if (state_type == BTK_STATE_ACTIVE)
    bdk_bairo_set_source_color (cr,
				&style->light[BTK_STATE_ACTIVE]);
  else
    bdk_bairo_set_source_color (cr,
				&style->base[BTK_STATE_NORMAL]);
  
  bairo_fill_preserve (cr);
  
  bdk_bairo_set_source_color (cr, &style->fg[state_type]);
  bairo_stroke (cr);
  
  bairo_destroy (cr);
}

typedef struct _ByteRange ByteRange;

struct _ByteRange
{
  guint start;
  guint end;
};

static ByteRange*
range_new (guint start,
           guint end)
{
  ByteRange *br = g_new (ByteRange, 1);

  br->start = start;
  br->end = end;
  
  return br;
}

static BangoLayout*
get_insensitive_layout (BdkDrawable *drawable,
			BangoLayout *layout)
{
  GSList *embossed_ranges = NULL;
  GSList *stippled_ranges = NULL;
  BangoLayoutIter *iter;
  GSList *tmp_list = NULL;
  BangoLayout *new_layout;
  BangoAttrList *attrs;
  BdkBitmap *stipple = NULL;
  
  iter = bango_layout_get_iter (layout);
  
  do
    {
      BangoLayoutRun *run;
      BangoAttribute *attr;
      gboolean need_stipple = FALSE;
      ByteRange *br;
      
      run = bango_layout_iter_get_run_readonly (iter);

      if (run)
        {
          tmp_list = run->item->analysis.extra_attrs;

          while (tmp_list != NULL)
            {
              attr = tmp_list->data;
              switch (attr->klass->type)
                {
                case BANGO_ATTR_FOREGROUND:
                case BANGO_ATTR_BACKGROUND:
                  need_stipple = TRUE;
                  break;
              
                default:
                  break;
                }

              if (need_stipple)
                break;
          
              tmp_list = g_slist_next (tmp_list);
            }

          br = range_new (run->item->offset, run->item->offset + run->item->length);
      
          if (need_stipple)
            stippled_ranges = g_slist_prepend (stippled_ranges, br);
          else
            embossed_ranges = g_slist_prepend (embossed_ranges, br);
        }
    }
  while (bango_layout_iter_next_run (iter));

  bango_layout_iter_free (iter);

  new_layout = bango_layout_copy (layout);

  attrs = bango_layout_get_attributes (new_layout);

  if (attrs == NULL)
    {
      /* Create attr list if there wasn't one */
      attrs = bango_attr_list_new ();
      bango_layout_set_attributes (new_layout, attrs);
      bango_attr_list_unref (attrs);
    }
  
  tmp_list = embossed_ranges;
  while (tmp_list != NULL)
    {
      BangoAttribute *attr;
      ByteRange *br = tmp_list->data;

      attr = bdk_bango_attr_embossed_new (TRUE);

      attr->start_index = br->start;
      attr->end_index = br->end;
      
      bango_attr_list_change (attrs, attr);

      g_free (br);
      
      tmp_list = g_slist_next (tmp_list);
    }

  g_slist_free (embossed_ranges);
  
  tmp_list = stippled_ranges;
  while (tmp_list != NULL)
    {
      BangoAttribute *attr;
      ByteRange *br = tmp_list->data;

      if (stipple == NULL)
        {
#define gray50_width 2
#define gray50_height 2
          static const char gray50_bits[] = {
            0x02, 0x01
          };

          stipple = bdk_bitmap_create_from_data (drawable,
                                                 gray50_bits, gray50_width,
                                                 gray50_height);
        }
      
      attr = bdk_bango_attr_stipple_new (stipple);

      attr->start_index = br->start;
      attr->end_index = br->end;
      
      bango_attr_list_change (attrs, attr);

      g_free (br);
      
      tmp_list = g_slist_next (tmp_list);
    }

  g_slist_free (stippled_ranges);
  
  if (stipple)
    g_object_unref (stipple);

  return new_layout;
}

static void
btk_default_draw_layout (BtkStyle        *style,
                         BdkWindow       *window,
                         BtkStateType     state_type,
			 gboolean         use_text,
                         BdkRectangle    *area,
                         BtkWidget       *widget,
                         const gchar     *detail,
                         gint             x,
                         gint             y,
                         BangoLayout     *layout)
{
  BdkGC *gc;

  gc = use_text ? style->text_gc[state_type] : style->fg_gc[state_type];
  
  if (area)
    bdk_gc_set_clip_rectangle (gc, area);

  if (state_type == BTK_STATE_INSENSITIVE)
    {
      BangoLayout *ins;

      ins = get_insensitive_layout (window, layout);
      
      bdk_draw_layout (window, gc, x, y, ins);

      g_object_unref (ins);
    }
  else
    {
      bdk_draw_layout (window, gc, x, y, layout);
    }

  if (area)
    bdk_gc_set_clip_rectangle (gc, NULL);
}

static void
btk_default_draw_resize_grip (BtkStyle       *style,
                              BdkWindow      *window,
                              BtkStateType    state_type,
                              BdkRectangle   *area,
                              BtkWidget      *widget,
                              const gchar    *detail,
                              BdkWindowEdge   edge,
                              gint            x,
                              gint            y,
                              gint            width,
                              gint            height)
{
  BdkPoint points[4];
  gint i, j, skip;

  if (area)
    {
      bdk_gc_set_clip_rectangle (style->light_gc[state_type], area);
      bdk_gc_set_clip_rectangle (style->dark_gc[state_type], area);
      bdk_gc_set_clip_rectangle (style->bg_gc[state_type], area);
    }
  
  skip = -1;
  switch (edge)
    {
    case BDK_WINDOW_EDGE_NORTH_WEST:
      /* make it square */
      if (width < height)
	height = width;
      else if (height < width)
	width = height;
      skip = 2;
      break;
    case BDK_WINDOW_EDGE_NORTH:
      if (width < height)
	height = width;
      break;
    case BDK_WINDOW_EDGE_NORTH_EAST:
      /* make it square, aligning to top right */
      if (width < height)
	height = width;
      else if (height < width)
	{
	  x += (width - height);
	  width = height;
	}
      skip = 3;
      break;
    case BDK_WINDOW_EDGE_WEST:
      if (height < width)
	width = height;
      break;
    case BDK_WINDOW_EDGE_EAST:
      /* aligning to right */
      if (height < width)
	{
	  x += (width - height);
	  width = height;
	}
      break;
    case BDK_WINDOW_EDGE_SOUTH_WEST:
      /* make it square, aligning to bottom left */
      if (width < height)
	{
	  y += (height - width);
	  height = width;
	}
      else if (height < width)
	width = height;
      skip = 1;
      break;
    case BDK_WINDOW_EDGE_SOUTH:
      /* align to bottom */
      if (width < height)
	{
	  y += (height - width);
	  height = width;
	}
      break;
    case BDK_WINDOW_EDGE_SOUTH_EAST:
      /* make it square, aligning to bottom right */
      if (width < height)
	{
	  y += (height - width);
	  height = width;
	}
      else if (height < width)
	{
	  x += (width - height);
	  width = height;
	}
      skip = 0;
      break;
    default:
      g_assert_not_reached ();
    }
  /* Clear background */
  j = 0;
  for (i = 0; i < 4; i++)
    {
      if (skip != i)
	{
	  points[j].x = (i == 0 || i == 3) ? x : x + width;
	  points[j].y = (i < 2) ? y : y + height;
	  j++;
	}
    }
  
  bdk_draw_polygon (window, style->bg_gc[state_type], TRUE, 
		    points, skip < 0 ? 4 : 3);
  
  switch (edge)
    {
    case BDK_WINDOW_EDGE_WEST:
    case BDK_WINDOW_EDGE_EAST:
      {
	gint xi;

	xi = x;

	while (xi < x + width)
	  {
	    bdk_draw_line (window,
			   style->light_gc[state_type],
			   xi, y,
			   xi, y + height);

	    xi++;
	    bdk_draw_line (window,
			   style->dark_gc[state_type],
			   xi, y,
			   xi, y + height);

	    xi += 2;
	  }
      }
      break;
    case BDK_WINDOW_EDGE_NORTH:
    case BDK_WINDOW_EDGE_SOUTH:
      {
	gint yi;

	yi = y;

	while (yi < y + height)
	  {
	    bdk_draw_line (window,
			   style->light_gc[state_type],
			   x, yi,
			   x + width, yi);

	    yi++;
	    bdk_draw_line (window,
			   style->dark_gc[state_type],
			   x, yi,
			   x + width, yi);

	    yi+= 2;
	  }
      }
      break;
    case BDK_WINDOW_EDGE_NORTH_WEST:
      {
	gint xi, yi;

	xi = x + width;
	yi = y + height;

	while (xi > x + 3)
	  {
	    bdk_draw_line (window,
			   style->dark_gc[state_type],
			   xi, y,
			   x, yi);

	    --xi;
	    --yi;

	    bdk_draw_line (window,
			   style->dark_gc[state_type],
			   xi, y,
			   x, yi);

	    --xi;
	    --yi;

	    bdk_draw_line (window,
			   style->light_gc[state_type],
			   xi, y,
			   x, yi);

	    xi -= 3;
	    yi -= 3;
	    
	  }
      }
      break;
    case BDK_WINDOW_EDGE_NORTH_EAST:
      {
        gint xi, yi;

        xi = x;
        yi = y + height;

        while (xi < (x + width - 3))
          {
            bdk_draw_line (window,
                           style->light_gc[state_type],
                           xi, y,
                           x + width, yi);                           

            ++xi;
            --yi;
            
            bdk_draw_line (window,
                           style->dark_gc[state_type],
                           xi, y,
                           x + width, yi);                           

            ++xi;
            --yi;
            
            bdk_draw_line (window,
                           style->dark_gc[state_type],
                           xi, y,
                           x + width, yi);

            xi += 3;
            yi -= 3;
          }
      }
      break;
    case BDK_WINDOW_EDGE_SOUTH_WEST:
      {
	gint xi, yi;

	xi = x + width;
	yi = y;

	while (xi > x + 3)
	  {
	    bdk_draw_line (window,
			   style->dark_gc[state_type],
			   x, yi,
			   xi, y + height);

	    --xi;
	    ++yi;

	    bdk_draw_line (window,
			   style->dark_gc[state_type],
			   x, yi,
			   xi, y + height);

	    --xi;
	    ++yi;

	    bdk_draw_line (window,
			   style->light_gc[state_type],
			   x, yi,
			   xi, y + height);

	    xi -= 3;
	    yi += 3;
	    
	  }
      }
      break;
    case BDK_WINDOW_EDGE_SOUTH_EAST:
      {
        gint xi, yi;

        xi = x;
        yi = y;

        while (xi < (x + width - 3))
          {
            bdk_draw_line (window,
                           style->light_gc[state_type],
                           xi, y + height,
                           x + width, yi);                           

            ++xi;
            ++yi;
            
            bdk_draw_line (window,
                           style->dark_gc[state_type],
                           xi, y + height,
                           x + width, yi);                           

            ++xi;
            ++yi;
            
            bdk_draw_line (window,
                           style->dark_gc[state_type],
                           xi, y + height,
                           x + width, yi);

            xi += 3;
            yi += 3;
          }
      }
      break;
    default:
      g_assert_not_reached ();
      break;
    }
  
  if (area)
    {
      bdk_gc_set_clip_rectangle (style->light_gc[state_type], NULL);
      bdk_gc_set_clip_rectangle (style->dark_gc[state_type], NULL);
      bdk_gc_set_clip_rectangle (style->bg_gc[state_type], NULL);
    }
}

static void
btk_default_draw_spinner (BtkStyle     *style,
                          BdkWindow    *window,
                          BtkStateType  state_type,
                          BdkRectangle *area,
                          BtkWidget    *widget,
                          const gchar  *detail,
                          guint         step,
                          gint          x,
                          gint          y,
                          gint          width,
                          gint          height)
{
  BdkColor *color;
  bairo_t *cr;
  guint num_steps;
  gdouble dx, dy;
  gdouble radius;
  gdouble half;
  gint i;
  guint real_step;

  btk_style_get (style, BTK_TYPE_SPINNER,
                 "num-steps", &num_steps,
                 NULL);
  real_step = step % num_steps;

  /* get bairo context */
  cr = bdk_bairo_create (window);

  /* set a clip rebunnyion for the expose event */
  bairo_rectangle (cr, x, y, width, height);
  bairo_clip (cr);

  bairo_translate (cr, x, y);

  /* draw clip rebunnyion */
  bairo_set_operator (cr, BAIRO_OPERATOR_OVER);

  color = &style->fg[state_type];
  dx = width / 2;
  dy = height / 2;
  radius = MIN (width / 2, height / 2);
  half = num_steps / 2;

  for (i = 0; i < num_steps; i++)
    {
      gint inset = 0.7 * radius;

      /* transparency is a function of time and intial value */
      gdouble t = (gdouble) ((i + num_steps - real_step)
                             % num_steps) / num_steps;

      bairo_save (cr);

      bairo_set_source_rgba (cr,
                             color->red / 65535.,
                             color->green / 65535.,
                             color->blue / 65535.,
                             t);

      bairo_set_line_width (cr, 2.0);
      bairo_move_to (cr,
                     dx + (radius - inset) * cos (i * G_PI / half),
                     dy + (radius - inset) * sin (i * G_PI / half));
      bairo_line_to (cr,
                     dx + radius * cos (i * G_PI / half),
                     dy + radius * sin (i * G_PI / half));
      bairo_stroke (cr);

      bairo_restore (cr);
    }

  /* free memory */
  bairo_destroy (cr);
}

void
_btk_style_shade (const BdkColor *a,
                  BdkColor       *b,
                  gdouble         k)
{
  gdouble red;
  gdouble green;
  gdouble blue;
  
  red = (gdouble) a->red / 65535.0;
  green = (gdouble) a->green / 65535.0;
  blue = (gdouble) a->blue / 65535.0;
  
  rgb_to_hls (&red, &green, &blue);
  
  green *= k;
  if (green > 1.0)
    green = 1.0;
  else if (green < 0.0)
    green = 0.0;
  
  blue *= k;
  if (blue > 1.0)
    blue = 1.0;
  else if (blue < 0.0)
    blue = 0.0;
  
  hls_to_rgb (&red, &green, &blue);
  
  b->red = red * 65535.0;
  b->green = green * 65535.0;
  b->blue = blue * 65535.0;
}

static void
rgb_to_hls (gdouble *r,
            gdouble *g,
            gdouble *b)
{
  gdouble min;
  gdouble max;
  gdouble red;
  gdouble green;
  gdouble blue;
  gdouble h, l, s;
  gdouble delta;
  
  red = *r;
  green = *g;
  blue = *b;
  
  if (red > green)
    {
      if (red > blue)
        max = red;
      else
        max = blue;
      
      if (green < blue)
        min = green;
      else
        min = blue;
    }
  else
    {
      if (green > blue)
        max = green;
      else
        max = blue;
      
      if (red < blue)
        min = red;
      else
        min = blue;
    }
  
  l = (max + min) / 2;
  s = 0;
  h = 0;
  
  if (max != min)
    {
      if (l <= 0.5)
        s = (max - min) / (max + min);
      else
        s = (max - min) / (2 - max - min);
      
      delta = max -min;
      if (red == max)
        h = (green - blue) / delta;
      else if (green == max)
        h = 2 + (blue - red) / delta;
      else if (blue == max)
        h = 4 + (red - green) / delta;
      
      h *= 60;
      if (h < 0.0)
        h += 360;
    }
  
  *r = h;
  *g = l;
  *b = s;
}

static void
hls_to_rgb (gdouble *h,
            gdouble *l,
            gdouble *s)
{
  gdouble hue;
  gdouble lightness;
  gdouble saturation;
  gdouble m1, m2;
  gdouble r, g, b;
  
  lightness = *l;
  saturation = *s;
  
  if (lightness <= 0.5)
    m2 = lightness * (1 + saturation);
  else
    m2 = lightness + saturation - lightness * saturation;
  m1 = 2 * lightness - m2;
  
  if (saturation == 0)
    {
      *h = lightness;
      *l = lightness;
      *s = lightness;
    }
  else
    {
      hue = *h + 120;
      while (hue > 360)
        hue -= 360;
      while (hue < 0)
        hue += 360;
      
      if (hue < 60)
        r = m1 + (m2 - m1) * hue / 60;
      else if (hue < 180)
        r = m2;
      else if (hue < 240)
        r = m1 + (m2 - m1) * (240 - hue) / 60;
      else
        r = m1;
      
      hue = *h;
      while (hue > 360)
        hue -= 360;
      while (hue < 0)
        hue += 360;
      
      if (hue < 60)
        g = m1 + (m2 - m1) * hue / 60;
      else if (hue < 180)
        g = m2;
      else if (hue < 240)
        g = m1 + (m2 - m1) * (240 - hue) / 60;
      else
        g = m1;
      
      hue = *h - 120;
      while (hue > 360)
        hue -= 360;
      while (hue < 0)
        hue += 360;
      
      if (hue < 60)
        b = m1 + (m2 - m1) * hue / 60;
      else if (hue < 180)
        b = m2;
      else if (hue < 240)
        b = m1 + (m2 - m1) * (240 - hue) / 60;
      else
        b = m1;
      
      *h = r;
      *l = g;
      *s = b;
    }
}


/**
 * btk_paint_hline:
 * @style: a #BtkStyle
 * @window: a #BdkWindow
 * @state_type: a state
 * @area: (allow-none): rectangle to which the output is clipped, or %NULL if the
 *        output should not be clipped
 * @widget: (allow-none): the widget
 * @detail: (allow-none): a style detail
 * @x1: the starting x coordinate
 * @x2: the ending x coordinate
 * @y: the y coordinate
 *
 * Draws a horizontal line from (@x1, @y) to (@x2, @y) in @window
 * using the given style and state.
 **/ 
void 
btk_paint_hline (BtkStyle           *style,
                 BdkWindow          *window,
                 BtkStateType        state_type,
                 const BdkRectangle *area,
                 BtkWidget          *widget,
                 const gchar        *detail,
                 gint                x1,
                 gint                x2,
                 gint                y)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (BTK_STYLE_GET_CLASS (style)->draw_hline != NULL);
  g_return_if_fail (style->depth == bdk_drawable_get_depth (window));

  BTK_STYLE_GET_CLASS (style)->draw_hline (style, window, state_type,
                                           (BdkRectangle *) area, widget, detail,
                                           x1, x2, y);
}

/**
 * btk_paint_vline:
 * @style: a #BtkStyle
 * @window: a #BdkWindow
 * @state_type: a state
 * @area: (allow-none): rectangle to which the output is clipped, or %NULL if the
 *        output should not be clipped
 * @widget: (allow-none): the widget
 * @detail: (allow-none): a style detail
 * @y1_: the starting y coordinate
 * @y2_: the ending y coordinate
 * @x: the x coordinate
 *
 * Draws a vertical line from (@x, @y1_) to (@x, @y2_) in @window
 * using the given style and state.
 */
void
btk_paint_vline (BtkStyle           *style,
                 BdkWindow          *window,
                 BtkStateType        state_type,
                 const BdkRectangle *area,
                 BtkWidget          *widget,
                 const gchar        *detail,
                 gint                y1_,
                 gint                y2_,
                 gint                x)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (BTK_STYLE_GET_CLASS (style)->draw_vline != NULL);
  g_return_if_fail (style->depth == bdk_drawable_get_depth (window));

  BTK_STYLE_GET_CLASS (style)->draw_vline (style, window, state_type,
                                           (BdkRectangle *) area, widget, detail,
                                           y1_, y2_, x);
}

/**
 * btk_paint_shadow:
 * @style: a #BtkStyle
 * @window: a #BdkWindow
 * @state_type: a state
 * @shadow_type: type of shadow to draw
 * @area: (allow-none): clip rectangle or %NULL if the
 *        output should not be clipped
 * @widget: (allow-none): the widget
 * @detail: (allow-none): a style detail
 * @x: x origin of the rectangle
 * @y: y origin of the rectangle
 * @width: width of the rectangle
 * @height: width of the rectangle
 *
 * Draws a shadow around the given rectangle in @window 
 * using the given style and state and shadow type.
 */
void
btk_paint_shadow (BtkStyle           *style,
                  BdkWindow          *window,
                  BtkStateType        state_type,
                  BtkShadowType       shadow_type,
                  const BdkRectangle *area,
                  BtkWidget          *widget,
                  const gchar        *detail,
                  gint                x,
                  gint                y,
                  gint                width,
                  gint                height)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (BTK_STYLE_GET_CLASS (style)->draw_shadow != NULL);
  g_return_if_fail (style->depth == bdk_drawable_get_depth (window));

  BTK_STYLE_GET_CLASS (style)->draw_shadow (style, window, state_type, shadow_type,
                                            (BdkRectangle *) area, widget, detail,
                                            x, y, width, height);
}

/**
 * btk_paint_polygon:
 * @style: a #BtkStyle
 * @window: a #BdkWindow
 * @state_type: a state
 * @shadow_type: type of shadow to draw
 * @area: (allow-none): clip rectangle, or %NULL if the
 *        output should not be clipped
 * @widget: (allow-none): the widget
 * @detail: (allow-none): a style detail
 * @points: an array of #BdkPoint<!-- -->s
 * @n_points: length of @points
 * @fill: %TRUE if the polygon should be filled
 *
 * Draws a polygon on @window with the given parameters.
 */ 
void
btk_paint_polygon (BtkStyle           *style,
                   BdkWindow          *window,
                   BtkStateType        state_type,
                   BtkShadowType       shadow_type,
                   const BdkRectangle *area,
                   BtkWidget          *widget,
                   const gchar        *detail,
                   const BdkPoint     *points,
                   gint                n_points,
                   gboolean            fill)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (BTK_STYLE_GET_CLASS (style)->draw_polygon != NULL);
  g_return_if_fail (style->depth == bdk_drawable_get_depth (window));

  BTK_STYLE_GET_CLASS (style)->draw_polygon (style, window, state_type, shadow_type,
                                             (BdkRectangle *) area, widget, detail,
                                             (BdkPoint *) points, n_points, fill);
}

/**
 * btk_paint_arrow:
 * @style: a #BtkStyle
 * @window: a #BdkWindow
 * @state_type: a state
 * @shadow_type: the type of shadow to draw
 * @area: (allow-none): clip rectangle, or %NULL if the
 *        output should not be clipped
 * @widget: (allow-none): the widget
 * @detail: (allow-none): a style detail
 * @arrow_type: the type of arrow to draw
 * @fill: %TRUE if the arrow tip should be filled
 * @x: x origin of the rectangle to draw the arrow in
 * @y: y origin of the rectangle to draw the arrow in
 * @width: width of the rectangle to draw the arrow in
 * @height: height of the rectangle to draw the arrow in
 * 
 * Draws an arrow in the given rectangle on @window using the given 
 * parameters. @arrow_type determines the direction of the arrow.
 */
void
btk_paint_arrow (BtkStyle           *style,
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
                 gint                height)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (BTK_STYLE_GET_CLASS (style)->draw_arrow != NULL);
  g_return_if_fail (style->depth == bdk_drawable_get_depth (window));

  BTK_STYLE_GET_CLASS (style)->draw_arrow (style, window, state_type, shadow_type,
                                           (BdkRectangle *) area, widget, detail,
                                           arrow_type, fill, x, y, width, height);
}

/**
 * btk_paint_diamond:
 * @style: a #BtkStyle
 * @window: a #BdkWindow
 * @state_type: a state
 * @shadow_type: the type of shadow to draw
 * @area: (allow-none): clip rectangle, or %NULL if the
 *        output should not be clipped
 * @widget: (allow-none): the widget
 * @detail: (allow-none): a style detail
 * @x: x origin of the rectangle to draw the diamond in
 * @y: y origin of the rectangle to draw the diamond in
 * @width: width of the rectangle to draw the diamond in
 * @height: height of the rectangle to draw the diamond in
 *
 * Draws a diamond in the given rectangle on @window using the given
 * parameters.
 */
void
btk_paint_diamond (BtkStyle           *style,
                   BdkWindow          *window,
                   BtkStateType        state_type,
                   BtkShadowType       shadow_type,
                   const BdkRectangle *area,
                   BtkWidget          *widget,
                   const gchar        *detail,
                   gint                x,
                   gint                y,
                   gint                width,
                   gint                height)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (BTK_STYLE_GET_CLASS (style)->draw_diamond != NULL);
  g_return_if_fail (style->depth == bdk_drawable_get_depth (window));

  BTK_STYLE_GET_CLASS (style)->draw_diamond (style, window, state_type, shadow_type,
                                             (BdkRectangle *) area, widget, detail,
                                             x, y, width, height);
}

/**
 * btk_paint_string:
 * @style: a #BtkStyle
 * @window: a #BdkWindow
 * @state_type: a state
 * @area: (allow-none): clip rectangle, or %NULL if the
 *        output should not be clipped
 * @widget: (allow-none): the widget
 * @detail: (allow-none): a style detail
 * @x: x origin
 * @y: y origin
 * @string: the string to draw
 *
 * Draws a text string on @window with the given parameters.
 *
 * Deprecated: 2.0: Use btk_paint_layout() instead.
 */
void
btk_paint_string (BtkStyle           *style,
                  BdkWindow          *window,
                  BtkStateType        state_type,
                  const BdkRectangle *area,
                  BtkWidget          *widget,
                  const gchar        *detail,
                  gint                x,
                  gint                y,
                  const gchar        *string)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (BTK_STYLE_GET_CLASS (style)->draw_string != NULL);
  g_return_if_fail (style->depth == bdk_drawable_get_depth (window));

  BTK_STYLE_GET_CLASS (style)->draw_string (style, window, state_type,
                                            (BdkRectangle *) area, widget, detail,
                                            x, y, string);
}

/**
 * btk_paint_box:
 * @style: a #BtkStyle
 * @window: a #BdkWindow
 * @state_type: a state
 * @shadow_type: the type of shadow to draw
 * @area: (allow-none): clip rectangle, or %NULL if the
 *        output should not be clipped
 * @widget: (allow-none): the widget
 * @detail: (allow-none): a style detail
 * @x: x origin of the box
 * @y: y origin of the box
 * @width: the width of the box
 * @height: the height of the box
 * 
 * Draws a box on @window with the given parameters.
 */
void
btk_paint_box (BtkStyle           *style,
               BdkWindow          *window,
               BtkStateType        state_type,
               BtkShadowType       shadow_type,
               const BdkRectangle *area,
               BtkWidget          *widget,
               const gchar        *detail,
               gint                x,
               gint                y,
               gint                width,
               gint                height)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (BTK_STYLE_GET_CLASS (style)->draw_box != NULL);
  g_return_if_fail (style->depth == bdk_drawable_get_depth (window));

  BTK_STYLE_GET_CLASS (style)->draw_box (style, window, state_type, shadow_type,
                                         (BdkRectangle *) area, widget, detail,
                                         x, y, width, height);
}

/**
 * btk_paint_flat_box:
 * @style: a #BtkStyle
 * @window: a #BdkWindow
 * @state_type: a state
 * @shadow_type: the type of shadow to draw
 * @area: (allow-none): clip rectangle, or %NULL if the
 *        output should not be clipped
 * @widget: (allow-none): the widget
 * @detail: (allow-none): a style detail
 * @x: x origin of the box
 * @y: y origin of the box
 * @width: the width of the box
 * @height: the height of the box
 * 
 * Draws a flat box on @window with the given parameters.
 */
void
btk_paint_flat_box (BtkStyle           *style,
                    BdkWindow          *window,
                    BtkStateType        state_type,
                    BtkShadowType       shadow_type,
                    const BdkRectangle *area,
                    BtkWidget          *widget,
                    const gchar        *detail,
                    gint                x,
                    gint                y,
                    gint                width,
                    gint                height)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (BTK_STYLE_GET_CLASS (style)->draw_flat_box != NULL);
  g_return_if_fail (style->depth == bdk_drawable_get_depth (window));

  BTK_STYLE_GET_CLASS (style)->draw_flat_box (style, window, state_type, shadow_type,
                                              (BdkRectangle *) area, widget, detail,
                                              x, y, width, height);
}

/**
 * btk_paint_check:
 * @style: a #BtkStyle
 * @window: a #BdkWindow
 * @state_type: a state
 * @shadow_type: the type of shadow to draw
 * @area: (allow-none): clip rectangle, or %NULL if the
 *        output should not be clipped
 * @widget: (allow-none): the widget
 * @detail: (allow-none): a style detail
 * @x: x origin of the rectangle to draw the check in
 * @y: y origin of the rectangle to draw the check in
 * @width: the width of the rectangle to draw the check in
 * @height: the height of the rectangle to draw the check in
 * 
 * Draws a check button indicator in the given rectangle on @window with 
 * the given parameters.
 */
void
btk_paint_check (BtkStyle           *style,
                 BdkWindow          *window,
                 BtkStateType        state_type,
                 BtkShadowType       shadow_type,
                 const BdkRectangle *area,
                 BtkWidget          *widget,
                 const gchar        *detail,
                 gint                x,
                 gint                y,
                 gint                width,
                 gint                height)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (BTK_STYLE_GET_CLASS (style)->draw_check != NULL);
  g_return_if_fail (style->depth == bdk_drawable_get_depth (window));

  BTK_STYLE_GET_CLASS (style)->draw_check (style, window, state_type, shadow_type,
                                           (BdkRectangle *) area, widget, detail,
                                           x, y, width, height);
}

/**
 * btk_paint_option:
 * @style: a #BtkStyle
 * @window: a #BdkWindow
 * @state_type: a state
 * @shadow_type: the type of shadow to draw
 * @area: (allow-none): clip rectangle, or %NULL if the
 *        output should not be clipped
 * @widget: (allow-none): the widget
 * @detail: (allow-none): a style detail
 * @x: x origin of the rectangle to draw the option in
 * @y: y origin of the rectangle to draw the option in
 * @width: the width of the rectangle to draw the option in
 * @height: the height of the rectangle to draw the option in
 *
 * Draws a radio button indicator in the given rectangle on @window with 
 * the given parameters.
 */
void
btk_paint_option (BtkStyle           *style,
                  BdkWindow          *window,
                  BtkStateType        state_type,
                  BtkShadowType       shadow_type,
                  const BdkRectangle *area,
                  BtkWidget          *widget,
                  const gchar        *detail,
                  gint                x,
                  gint                y,
                  gint                width,
                  gint                height)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (BTK_STYLE_GET_CLASS (style)->draw_option != NULL);
  g_return_if_fail (style->depth == bdk_drawable_get_depth (window));

  BTK_STYLE_GET_CLASS (style)->draw_option (style, window, state_type, shadow_type,
                                            (BdkRectangle *) area, widget, detail,
                                            x, y, width, height);
}

/**
 * btk_paint_tab:
 * @style: a #BtkStyle
 * @window: a #BdkWindow
 * @state_type: a state
 * @shadow_type: the type of shadow to draw
 * @area: (allow-none): clip rectangle, or %NULL if the
 *        output should not be clipped
 * @widget: (allow-none): the widget
 * @detail: (allow-none): a style detail
 * @x: x origin of the rectangle to draw the tab in
 * @y: y origin of the rectangle to draw the tab in
 * @width: the width of the rectangle to draw the tab in
 * @height: the height of the rectangle to draw the tab in
 *
 * Draws an option menu tab (i.e. the up and down pointing arrows)
 * in the given rectangle on @window using the given parameters.
 */ 
void
btk_paint_tab (BtkStyle           *style,
               BdkWindow          *window,
               BtkStateType        state_type,
               BtkShadowType       shadow_type,
               const BdkRectangle *area,
               BtkWidget          *widget,
               const gchar        *detail,
               gint                x,
               gint                y,
               gint                width,
               gint                height)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (BTK_STYLE_GET_CLASS (style)->draw_tab != NULL);
  g_return_if_fail (style->depth == bdk_drawable_get_depth (window));

  BTK_STYLE_GET_CLASS (style)->draw_tab (style, window, state_type, shadow_type,
                                         (BdkRectangle *) area, widget, detail,
                                         x, y, width, height);
}

/**
 * btk_paint_shadow_gap:
 * @style: a #BtkStyle
 * @window: a #BdkWindow
 * @state_type: a state
 * @shadow_type: type of shadow to draw
 * @area: (allow-none): clip rectangle, or %NULL if the
 *        output should not be clipped
 * @widget: (allow-none): the widget
 * @detail: (allow-none): a style detail
 * @x: x origin of the rectangle
 * @y: y origin of the rectangle
 * @width: width of the rectangle
 * @height: width of the rectangle
 * @gap_side: side in which to leave the gap
 * @gap_x: starting position of the gap
 * @gap_width: width of the gap
 *
 * Draws a shadow around the given rectangle in @window 
 * using the given style and state and shadow type, leaving a 
 * gap in one side.
*/
void
btk_paint_shadow_gap (BtkStyle           *style,
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
                      gint                gap_width)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (BTK_STYLE_GET_CLASS (style)->draw_shadow_gap != NULL);
  g_return_if_fail (style->depth == bdk_drawable_get_depth (window));

  BTK_STYLE_GET_CLASS (style)->draw_shadow_gap (style, window, state_type, shadow_type,
                                                (BdkRectangle *) area, widget, detail,
                                                x, y, width, height, gap_side, gap_x, gap_width);
}


/**
 * btk_paint_box_gap:
 * @style: a #BtkStyle
 * @window: a #BdkWindow
 * @state_type: a state
 * @shadow_type: type of shadow to draw
 * @area: (allow-none): clip rectangle, or %NULL if the
 *        output should not be clipped
 * @widget: (allow-none): the widget
 * @detail: (allow-none): a style detail
 * @x: x origin of the rectangle
 * @y: y origin of the rectangle
 * @width: width of the rectangle
 * @height: width of the rectangle
 * @gap_side: side in which to leave the gap
 * @gap_x: starting position of the gap
 * @gap_width: width of the gap
 *
 * Draws a box in @window using the given style and state and shadow type, 
 * leaving a gap in one side.
 */
void
btk_paint_box_gap (BtkStyle           *style,
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
                   gint                gap_width)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (BTK_STYLE_GET_CLASS (style)->draw_box_gap != NULL);
  g_return_if_fail (style->depth == bdk_drawable_get_depth (window));

  BTK_STYLE_GET_CLASS (style)->draw_box_gap (style, window, state_type, shadow_type,
                                             (BdkRectangle *) area, widget, detail,
                                             x, y, width, height, gap_side, gap_x, gap_width);
}

/**
 * btk_paint_extension: 
 * @style: a #BtkStyle
 * @window: a #BdkWindow
 * @state_type: a state
 * @shadow_type: type of shadow to draw
 * @area: (allow-none): clip rectangle, or %NULL if the
 *        output should not be clipped
 * @widget: (allow-none): the widget
 * @detail: (allow-none): a style detail
 * @x: x origin of the extension
 * @y: y origin of the extension
 * @width: width of the extension
 * @height: width of the extension
 * @gap_side: the side on to which the extension is attached
 * 
 * Draws an extension, i.e. a notebook tab.
 **/
void
btk_paint_extension (BtkStyle           *style,
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
                     BtkPositionType     gap_side)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (BTK_STYLE_GET_CLASS (style)->draw_extension != NULL);
  g_return_if_fail (style->depth == bdk_drawable_get_depth (window));

  BTK_STYLE_GET_CLASS (style)->draw_extension (style, window, state_type, shadow_type,
                                               (BdkRectangle *) area, widget, detail,
                                               x, y, width, height, gap_side);
}

/**
 * btk_paint_focus:
 * @style: a #BtkStyle
 * @window: a #BdkWindow
 * @state_type: a state
 * @area: (allow-none):  clip rectangle, or %NULL if the
 *        output should not be clipped
 * @widget: (allow-none): the widget
 * @detail: (allow-none): a style detail
 * @x: the x origin of the rectangle around which to draw a focus indicator
 * @y: the y origin of the rectangle around which to draw a focus indicator
 * @width: the width of the rectangle around which to draw a focus indicator
 * @height: the height of the rectangle around which to draw a focus indicator
 *
 * Draws a focus indicator around the given rectangle on @window using the
 * given style.
 */
void
btk_paint_focus (BtkStyle           *style,
                 BdkWindow          *window,
		 BtkStateType        state_type,
                 const BdkRectangle *area,
                 BtkWidget          *widget,
                 const gchar        *detail,
                 gint                x,
                 gint                y,
                 gint                width,
                 gint                height)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (BTK_STYLE_GET_CLASS (style)->draw_focus != NULL);
  g_return_if_fail (style->depth == bdk_drawable_get_depth (window));

  BTK_STYLE_GET_CLASS (style)->draw_focus (style, window, state_type,
                                           (BdkRectangle *) area, widget, detail,
                                           x, y, width, height);
}

/**
 * btk_paint_slider:
 * @style: a #BtkStyle
 * @window: a #BdkWindow
 * @state_type: a state
 * @shadow_type: a shadow
 * @area: (allow-none): clip rectangle, or %NULL if the
 *        output should not be clipped
 * @widget: (allow-none): the widget
 * @detail: (allow-none): a style detail
 * @x: the x origin of the rectangle in which to draw a slider
 * @y: the y origin of the rectangle in which to draw a slider
 * @width: the width of the rectangle in which to draw a slider
 * @height: the height of the rectangle in which to draw a slider
 * @orientation: the orientation to be used
 *
 * Draws a slider in the given rectangle on @window using the
 * given style and orientation.
 **/
void
btk_paint_slider (BtkStyle           *style,
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
                  BtkOrientation      orientation)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (BTK_STYLE_GET_CLASS (style)->draw_slider != NULL);
  g_return_if_fail (style->depth == bdk_drawable_get_depth (window));

  BTK_STYLE_GET_CLASS (style)->draw_slider (style, window, state_type, shadow_type,
                                            (BdkRectangle *) area, widget, detail,
                                            x, y, width, height, orientation);
}

/**
 * btk_paint_handle:
 * @style: a #BtkStyle
 * @window: a #BdkWindow
 * @state_type: a state
 * @shadow_type: type of shadow to draw
 * @area: (allow-none): clip rectangle, or %NULL if the
 *        output should not be clipped
 * @widget: (allow-none): the widget
 * @detail: (allow-none): a style detail
 * @x: x origin of the handle
 * @y: y origin of the handle
 * @width: with of the handle
 * @height: height of the handle
 * @orientation: the orientation of the handle
 * 
 * Draws a handle as used in #BtkHandleBox and #BtkPaned.
 **/
void
btk_paint_handle (BtkStyle           *style,
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
                  BtkOrientation      orientation)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (BTK_STYLE_GET_CLASS (style)->draw_handle != NULL);
  g_return_if_fail (style->depth == bdk_drawable_get_depth (window));

  BTK_STYLE_GET_CLASS (style)->draw_handle (style, window, state_type, shadow_type,
                                            (BdkRectangle *) area, widget, detail,
                                            x, y, width, height, orientation);
}

/**
 * btk_paint_expander:
 * @style: a #BtkStyle
 * @window: a #BdkWindow
 * @state_type: a state
 * @area: (allow-none): clip rectangle, or %NULL if the
 *        output should not be clipped
 * @widget: (allow-none): the widget
 * @detail: (allow-none): a style detail
 * @x: the x position to draw the expander at
 * @y: the y position to draw the expander at
 * @expander_style: the style to draw the expander in; determines
 *   whether the expander is collapsed, expanded, or in an
 *   intermediate state.
 * 
 * Draws an expander as used in #BtkTreeView. @x and @y specify the
 * center the expander. The size of the expander is determined by the
 * "expander-size" style property of @widget.  (If widget is not
 * specified or doesn't have an "expander-size" property, an
 * unspecified default size will be used, since the caller doesn't
 * have sufficient information to position the expander, this is
 * likely not useful.) The expander is expander_size pixels tall
 * in the collapsed position and expander_size pixels wide in the
 * expanded position.
 **/
void
btk_paint_expander (BtkStyle           *style,
                    BdkWindow          *window,
                    BtkStateType        state_type,
                    const BdkRectangle *area,
                    BtkWidget          *widget,
                    const gchar        *detail,
                    gint                x,
                    gint                y,
		    BtkExpanderStyle    expander_style)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (BTK_STYLE_GET_CLASS (style)->draw_expander != NULL);
  g_return_if_fail (style->depth == bdk_drawable_get_depth (window));

  BTK_STYLE_GET_CLASS (style)->draw_expander (style, window, state_type,
                                              (BdkRectangle *) area, widget, detail,
                                              x, y, expander_style);
}

/**
 * btk_paint_layout:
 * @style: a #BtkStyle
 * @window: a #BdkWindow
 * @state_type: a state
 * @use_text: whether to use the text or foreground
 *            graphics context of @style
 * @area: (allow-none): clip rectangle, or %NULL if the
 *        output should not be clipped
 * @widget: (allow-none): the widget
 * @detail: (allow-none): a style detail
 * @x: x origin
 * @y: y origin
 * @layout: the layout to draw
 *
 * Draws a layout on @window using the given parameters.
 **/
void
btk_paint_layout (BtkStyle           *style,
                  BdkWindow          *window,
                  BtkStateType        state_type,
                  gboolean            use_text,
                  const BdkRectangle *area,
                  BtkWidget          *widget,
                  const gchar        *detail,
                  gint                x,
                  gint                y,
                  BangoLayout        *layout)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (BTK_STYLE_GET_CLASS (style)->draw_layout != NULL);
  g_return_if_fail (style->depth == bdk_drawable_get_depth (window));

  BTK_STYLE_GET_CLASS (style)->draw_layout (style, window, state_type, use_text,
                                            (BdkRectangle *) area, widget, detail,
                                            x, y, layout);
}

/**
 * btk_paint_resize_grip:
 * @style: a #BtkStyle
 * @window: a #BdkWindow
 * @state_type: a state
 * @area: (allow-none): clip rectangle, or %NULL if the
 *        output should not be clipped
 * @widget: (allow-none): the widget
 * @detail: (allow-none): a style detail
 * @edge: the edge in which to draw the resize grip
 * @x: the x origin of the rectangle in which to draw the resize grip
 * @y: the y origin of the rectangle in which to draw the resize grip
 * @width: the width of the rectangle in which to draw the resize grip
 * @height: the height of the rectangle in which to draw the resize grip
 *
 * Draws a resize grip in the given rectangle on @window using the given
 * parameters. 
 */
void
btk_paint_resize_grip (BtkStyle           *style,
                       BdkWindow          *window,
                       BtkStateType        state_type,
                       const BdkRectangle *area,
                       BtkWidget          *widget,
                       const gchar        *detail,
                       BdkWindowEdge       edge,
                       gint                x,
                       gint                y,
                       gint                width,
                       gint                height)

{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (BTK_STYLE_GET_CLASS (style)->draw_resize_grip != NULL);
  g_return_if_fail (style->depth == bdk_drawable_get_depth (window));

  BTK_STYLE_GET_CLASS (style)->draw_resize_grip (style, window, state_type,
                                                 (BdkRectangle *) area, widget, detail,
                                                 edge, x, y, width, height);
}

/**
 * btk_paint_spinner:
 * @style: a #BtkStyle
 * @window: a #BdkWindow
 * @state_type: a state
 * @area: (allow-none): clip rectangle, or %NULL if the
 *        output should not be clipped
 * @widget: (allow-none): the widget (may be %NULL)
 * @detail: (allow-none): a style detail (may be %NULL)
 * @step: the nth step, a value between 0 and #BtkSpinner:num-steps
 * @x: the x origin of the rectangle in which to draw the spinner
 * @y: the y origin of the rectangle in which to draw the spinner
 * @width: the width of the rectangle in which to draw the spinner
 * @height: the height of the rectangle in which to draw the spinner
 *
 * Draws a spinner on @window using the given parameters.
 *
 * Since: 2.20
 */
void
btk_paint_spinner (BtkStyle           *style,
		   BdkWindow          *window,
		   BtkStateType        state_type,
                   const BdkRectangle *area,
                   BtkWidget          *widget,
                   const gchar        *detail,
		   guint               step,
		   gint                x,
		   gint                y,
		   gint                width,
		   gint                height)
{
  g_return_if_fail (BTK_IS_STYLE (style));
  g_return_if_fail (BTK_STYLE_GET_CLASS (style)->draw_spinner != NULL);
  g_return_if_fail (style->depth == bdk_drawable_get_depth (window));

  BTK_STYLE_GET_CLASS (style)->draw_spinner (style, window, state_type,
                                             (BdkRectangle *)area, widget, detail,
					     step, x, y, width, height);
}

/**
 * btk_border_new:
 *
 * Allocates a new #BtkBorder structure and initializes its elements to zero.
 * 
 * Returns: a new empty #BtkBorder. The newly allocated #BtkBorder should be 
 *     freed with btk_border_free()
 *
 * Since: 2.14
 **/
BtkBorder *
btk_border_new (void)
{
  return g_slice_new0 (BtkBorder);
}

/**
 * btk_border_copy:
 * @border_: a #BtkBorder.
 * @returns: a copy of @border_.
 *
 * Copies a #BtkBorder structure.
 **/
BtkBorder *
btk_border_copy (const BtkBorder *border)
{
  g_return_val_if_fail (border != NULL, NULL);

  return g_slice_dup (BtkBorder, border);
}

/**
 * btk_border_free:
 * @border_: a #BtkBorder.
 * 
 * Frees a #BtkBorder structure.
 **/
void
btk_border_free (BtkBorder *border)
{
  g_slice_free (BtkBorder, border);
}

GType
btk_border_get_type (void)
{
  static GType our_type = 0;
  
  if (our_type == 0)
    our_type = g_boxed_type_register_static (I_("BtkBorder"),
					     (GBoxedCopyFunc) btk_border_copy,
					     (GBoxedFreeFunc) btk_border_free);

  return our_type;
}

static BdkFont *
btk_style_get_font_internal (BtkStyle *style)
{
  g_return_val_if_fail (BTK_IS_STYLE (style), NULL);

  if (style->private_font && style->private_font_desc)
    {
      if (!style->font_desc ||
	  !bango_font_description_equal (style->private_font_desc, style->font_desc))
	{
	  bdk_font_unref (style->private_font);
	  style->private_font = NULL;
	  
	  if (style->private_font_desc)
	    {
	      bango_font_description_free (style->private_font_desc);
	      style->private_font_desc = NULL;
	    }
	}
    }
  
  if (!style->private_font)
    {
      BdkDisplay *display;

      if (style->colormap)
	{
	  display = bdk_screen_get_display (bdk_colormap_get_screen (style->colormap));
	}
      else
	{
	  display = bdk_display_get_default ();
	  BTK_NOTE (MULTIHEAD,
		    g_warning ("btk_style_get_font() should not be called on an unattached style"));
	}
      
      if (style->font_desc)
	{
	  style->private_font = bdk_font_from_description_for_display (display, style->font_desc);
	  style->private_font_desc = bango_font_description_copy (style->font_desc);
	}

      if (!style->private_font)
	style->private_font = bdk_font_load_for_display (display, "fixed");
      
      if (!style->private_font) 
	g_error ("Unable to load \"fixed\" font");
    }

  return style->private_font;
}

/**
 * btk_style_get_font:
 * @style: a #BtkStyle
 * 
 * Gets the #BdkFont to use for the given style. This is
 * meant only as a replacement for direct access to @style->font
 * and should not be used in new code. New code should
 * use @style->font_desc instead.
 * 
 * Return value: the #BdkFont for the style. This font is owned
 *   by the style; if you want to keep around a copy, you must
 *   call bdk_font_ref().
 **/
BdkFont *
btk_style_get_font (BtkStyle *style)
{
  g_return_val_if_fail (BTK_IS_STYLE (style), NULL);

  return btk_style_get_font_internal (style);
}

/**
 * btk_style_set_font:
 * @style: a #BtkStyle.
 * @font: (allow-none): a #BdkFont, or %NULL to use the #BdkFont corresponding
 *   to style->font_desc.
 * 
 * Sets the #BdkFont to use for a given style. This is
 * meant only as a replacement for direct access to style->font
 * and should not be used in new code. New code should
 * use style->font_desc instead.
 **/
void
btk_style_set_font (BtkStyle *style,
		    BdkFont  *font)
{
  BdkFont *old_font;

  g_return_if_fail (BTK_IS_STYLE (style));

  old_font = style->private_font;

  style->private_font = font;
  if (font)
    bdk_font_ref (font);

  if (old_font)
    bdk_font_unref (old_font);

  if (style->private_font_desc)
    {
      bango_font_description_free (style->private_font_desc);
      style->private_font_desc = NULL;
    }
}

typedef struct _CursorInfo CursorInfo;

struct _CursorInfo
{
  GType for_type;
  BdkGC *primary_gc;
  BdkGC *secondary_gc;
};

static void
style_unrealize_cursor_gcs (BtkStyle *style)
{
  CursorInfo *
  
  cursor_info = g_object_get_data (G_OBJECT (style), "btk-style-cursor-info");
  if (cursor_info)
    {
      if (cursor_info->primary_gc)
	btk_gc_release (cursor_info->primary_gc);

      if (cursor_info->secondary_gc)
	btk_gc_release (cursor_info->secondary_gc);
      
      g_free (cursor_info);
      g_object_set_data (G_OBJECT (style), I_("btk-style-cursor-info"), NULL);
    }
}

static BdkGC *
make_cursor_gc (BtkWidget      *widget,
		const gchar    *property_name,
		const BdkColor *fallback)
{
  BdkGCValues gc_values;
  BdkGCValuesMask gc_values_mask;
  BdkColor *cursor_color;

  btk_widget_style_get (widget, property_name, &cursor_color, NULL);
  
  gc_values_mask = BDK_GC_FOREGROUND;
  if (cursor_color)
    {
      gc_values.foreground = *cursor_color;
      bdk_color_free (cursor_color);
    }
  else
    gc_values.foreground = *fallback;
  
  bdk_rgb_find_color (widget->style->colormap, &gc_values.foreground);
  return btk_gc_get (widget->style->depth, widget->style->colormap, &gc_values, gc_values_mask);
}

static BdkGC *
get_insertion_cursor_gc (BtkWidget *widget,
			 gboolean   is_primary)
{
  CursorInfo *cursor_info;

  cursor_info = g_object_get_data (G_OBJECT (widget->style), "btk-style-cursor-info");
  if (!cursor_info)
    {
      cursor_info = g_new (CursorInfo, 1);
      g_object_set_data (G_OBJECT (widget->style), I_("btk-style-cursor-info"), cursor_info);
      cursor_info->primary_gc = NULL;
      cursor_info->secondary_gc = NULL;
      cursor_info->for_type = G_TYPE_INVALID;
    }

  /* We have to keep track of the type because btk_widget_style_get()
   * can return different results when called on the same property and
   * same style but for different widgets. :-(. That is,
   * BtkEntry::cursor-color = "red" in a style will modify the cursor
   * color for entries but not for text view.
   */
  if (cursor_info->for_type != G_OBJECT_TYPE (widget))
    {
      cursor_info->for_type = G_OBJECT_TYPE (widget);
      if (cursor_info->primary_gc)
	{
	  btk_gc_release (cursor_info->primary_gc);
	  cursor_info->primary_gc = NULL;
	}
      if (cursor_info->secondary_gc)
	{
	  btk_gc_release (cursor_info->secondary_gc);
	  cursor_info->secondary_gc = NULL;
	}
    }

  /* Cursors in text widgets are drawn only in NORMAL state,
   * so we can use text[BTK_STATE_NORMAL] as text color here */
  if (is_primary)
    {
      if (!cursor_info->primary_gc)
	cursor_info->primary_gc = make_cursor_gc (widget,
						  "cursor-color",
						  &widget->style->text[BTK_STATE_NORMAL]);

      return cursor_info->primary_gc;
    }
  else
    {
      if (!cursor_info->secondary_gc)
	cursor_info->secondary_gc = make_cursor_gc (widget,
						    "secondary-cursor-color",
						    /* text_aa is the average of text and base colors,
						     * in usual black-on-white case it's grey. */
						    &widget->style->text_aa[BTK_STATE_NORMAL]);

      return cursor_info->secondary_gc;
    }
}

BdkGC *
_btk_widget_get_cursor_gc (BtkWidget *widget)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);
  g_return_val_if_fail (btk_widget_get_realized (widget), NULL);
  return get_insertion_cursor_gc (widget, TRUE);
}

void
_btk_widget_get_cursor_color (BtkWidget *widget,
			      BdkColor  *color)
{
  BdkColor *style_color;

  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (color != NULL);

  btk_widget_style_get (widget, "cursor-color", &style_color, NULL);

  if (style_color)
    {
      *color = *style_color;
      bdk_color_free (style_color);
    }
  else
    *color = widget->style->text[BTK_STATE_NORMAL];
}

static void
draw_insertion_cursor (BtkWidget          *widget,
		       BdkDrawable        *drawable,
		       BdkGC              *gc,
		       const BdkRectangle *location,
		       BtkTextDirection    direction,
		       gboolean            draw_arrow)
{
  gint stem_width;
  gint arrow_width;
  gint x, y;
  gint i;
  gfloat cursor_aspect_ratio;
  gint offset;
  
  /* When changing the shape or size of the cursor here,
   * propagate the changes to btktextview.c:text_window_invalidate_cursors().
   */

  btk_widget_style_get (widget, "cursor-aspect-ratio", &cursor_aspect_ratio, NULL);
  
  stem_width = location->height * cursor_aspect_ratio + 1;
  arrow_width = stem_width + 1;

  /* put (stem_width % 2) on the proper side of the cursor */
  if (direction == BTK_TEXT_DIR_LTR)
    offset = stem_width / 2;
  else
    offset = stem_width - stem_width / 2;
  
  for (i = 0; i < stem_width; i++)
    bdk_draw_line (drawable, gc,
		   location->x + i - offset, location->y,
		   location->x + i - offset, location->y + location->height - 1);

  if (draw_arrow)
    {
      if (direction == BTK_TEXT_DIR_RTL)
        {
          x = location->x - offset - 1;
          y = location->y + location->height - arrow_width * 2 - arrow_width + 1;
  
          for (i = 0; i < arrow_width; i++)
            {
              bdk_draw_line (drawable, gc,
                             x, y + i + 1,
                             x, y + 2 * arrow_width - i - 1);
              x --;
            }
        }
      else if (direction == BTK_TEXT_DIR_LTR)
        {
          x = location->x + stem_width - offset;
          y = location->y + location->height - arrow_width * 2 - arrow_width + 1;
  
          for (i = 0; i < arrow_width; i++) 
            {
              bdk_draw_line (drawable, gc,
                             x, y + i + 1,
                             x, y + 2 * arrow_width - i - 1);
              x++;
            }
        }
    }
}

/**
 * btk_draw_insertion_cursor:
 * @widget:  a #BtkWidget
 * @drawable: a #BdkDrawable
 * @area: (allow-none): rectangle to which the output is clipped, or %NULL if the
 *        output should not be clipped
 * @location: location where to draw the cursor (@location->width is ignored)
 * @is_primary: if the cursor should be the primary cursor color.
 * @direction: whether the cursor is left-to-right or
 *             right-to-left. Should never be #BTK_TEXT_DIR_NONE
 * @draw_arrow: %TRUE to draw a directional arrow on the
 *        cursor. Should be %FALSE unless the cursor is split.
 * 
 * Draws a text caret on @drawable at @location. This is not a style function
 * but merely a convenience function for drawing the standard cursor shape.
 *
 * Since: 2.4
 **/
void
btk_draw_insertion_cursor (BtkWidget          *widget,
			   BdkDrawable        *drawable,
			   const BdkRectangle *area,
			   const BdkRectangle *location,
			   gboolean            is_primary,
			   BtkTextDirection    direction,
			   gboolean            draw_arrow)
{
  BdkGC *gc;

  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (BDK_IS_DRAWABLE (drawable));
  g_return_if_fail (location != NULL);
  g_return_if_fail (direction != BTK_TEXT_DIR_NONE);

  gc = get_insertion_cursor_gc (widget, is_primary);
  if (area)
    bdk_gc_set_clip_rectangle (gc, area);
  
  draw_insertion_cursor (widget, drawable, gc,
			 location, direction, draw_arrow);
  
  if (area)
    bdk_gc_set_clip_rectangle (gc, NULL);
}

#define __BTK_STYLE_C__
#include "btkaliasdef.c"
