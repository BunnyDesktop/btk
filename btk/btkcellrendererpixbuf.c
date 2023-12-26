/* btkcellrendererpixbuf.c
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

#include "config.h"
#include <stdlib.h>
#include "btkcellrendererpixbuf.h"
#include "btkiconfactory.h"
#include "btkicontheme.h"
#include "btkintl.h"
#include "btkprivate.h"
#include "btkalias.h"

static void btk_cell_renderer_pixbuf_get_property  (BObject                    *object,
						    buint                       param_id,
						    BValue                     *value,
						    BParamSpec                 *pspec);
static void btk_cell_renderer_pixbuf_set_property  (BObject                    *object,
						    buint                       param_id,
						    const BValue               *value,
						    BParamSpec                 *pspec);
static void btk_cell_renderer_pixbuf_finalize   (BObject                    *object);
static void btk_cell_renderer_pixbuf_create_stock_pixbuf (BtkCellRendererPixbuf *cellpixbuf,
							  BtkWidget             *widget);
static void btk_cell_renderer_pixbuf_get_size   (BtkCellRenderer            *cell,
						 BtkWidget                  *widget,
						 BdkRectangle               *rectangle,
						 bint                       *x_offset,
						 bint                       *y_offset,
						 bint                       *width,
						 bint                       *height);
static void btk_cell_renderer_pixbuf_render     (BtkCellRenderer            *cell,
						 BdkDrawable                *window,
						 BtkWidget                  *widget,
						 BdkRectangle               *background_area,
						 BdkRectangle               *cell_area,
						 BdkRectangle               *expose_area,
						 BtkCellRendererState        flags);


enum {
  PROP_0,
  PROP_PIXBUF,
  PROP_PIXBUF_EXPANDER_OPEN,
  PROP_PIXBUF_EXPANDER_CLOSED,
  PROP_STOCK_ID,
  PROP_STOCK_SIZE,
  PROP_STOCK_DETAIL,
  PROP_FOLLOW_STATE,
  PROP_ICON_NAME,
  PROP_GICON
};


#define BTK_CELL_RENDERER_PIXBUF_GET_PRIVATE(obj) (B_TYPE_INSTANCE_GET_PRIVATE ((obj), BTK_TYPE_CELL_RENDERER_PIXBUF, BtkCellRendererPixbufPrivate))

typedef struct _BtkCellRendererPixbufPrivate BtkCellRendererPixbufPrivate;
struct _BtkCellRendererPixbufPrivate
{
  bchar *stock_id;
  BtkIconSize stock_size;
  bchar *stock_detail;
  bboolean follow_state;
  bchar *icon_name;
  GIcon *gicon;
};

G_DEFINE_TYPE (BtkCellRendererPixbuf, btk_cell_renderer_pixbuf, BTK_TYPE_CELL_RENDERER)

static void
btk_cell_renderer_pixbuf_init (BtkCellRendererPixbuf *cellpixbuf)
{
  BtkCellRendererPixbufPrivate *priv;

  priv = BTK_CELL_RENDERER_PIXBUF_GET_PRIVATE (cellpixbuf);
  priv->stock_size = BTK_ICON_SIZE_MENU;
}

static void
btk_cell_renderer_pixbuf_class_init (BtkCellRendererPixbufClass *class)
{
  BObjectClass *object_class = B_OBJECT_CLASS (class);
  BtkCellRendererClass *cell_class = BTK_CELL_RENDERER_CLASS (class);

  object_class->finalize = btk_cell_renderer_pixbuf_finalize;

  object_class->get_property = btk_cell_renderer_pixbuf_get_property;
  object_class->set_property = btk_cell_renderer_pixbuf_set_property;

  cell_class->get_size = btk_cell_renderer_pixbuf_get_size;
  cell_class->render = btk_cell_renderer_pixbuf_render;

  g_object_class_install_property (object_class,
				   PROP_PIXBUF,
				   g_param_spec_object ("pixbuf",
							P_("Pixbuf Object"),
							P_("The pixbuf to render"),
							BDK_TYPE_PIXBUF,
							BTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_PIXBUF_EXPANDER_OPEN,
				   g_param_spec_object ("pixbuf-expander-open",
							P_("Pixbuf Expander Open"),
							P_("Pixbuf for open expander"),
							BDK_TYPE_PIXBUF,
							BTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_PIXBUF_EXPANDER_CLOSED,
				   g_param_spec_object ("pixbuf-expander-closed",
							P_("Pixbuf Expander Closed"),
							P_("Pixbuf for closed expander"),
							BDK_TYPE_PIXBUF,
							BTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_STOCK_ID,
				   g_param_spec_string ("stock-id",
							P_("Stock ID"),
							P_("The stock ID of the stock icon to render"),
							NULL,
							BTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_STOCK_SIZE,
				   g_param_spec_uint ("stock-size",
						      P_("Size"),
						      P_("The BtkIconSize value that specifies the size of the rendered icon"),
						      0,
						      B_MAXUINT,
						      BTK_ICON_SIZE_MENU,
						      BTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_STOCK_DETAIL,
				   g_param_spec_string ("stock-detail",
							P_("Detail"),
							P_("Render detail to pass to the theme engine"),
							NULL,
							BTK_PARAM_READWRITE));

  
  /**
   * BtkCellRendererPixbuf:icon-name:
   *
   * The name of the themed icon to display.
   * This property only has an effect if not overridden by "stock_id" 
   * or "pixbuf" properties.
   *
   * Since: 2.8 
   */
  g_object_class_install_property (object_class,
				   PROP_ICON_NAME,
				   g_param_spec_string ("icon-name",
							P_("Icon Name"),
							P_("The name of the icon from the icon theme"),
							NULL,
							BTK_PARAM_READWRITE));

  /**
   * BtkCellRendererPixbuf:follow-state:
   *
   * Specifies whether the rendered pixbuf should be colorized
   * according to the #BtkCellRendererState.
   *
   * Since: 2.8
   */
  g_object_class_install_property (object_class,
				   PROP_FOLLOW_STATE,
				   g_param_spec_boolean ("follow-state",
 							 P_("Follow State"),
 							 P_("Whether the rendered pixbuf should be "
							    "colorized according to the state"),
 							 FALSE,
 							 BTK_PARAM_READWRITE));

  /**
   * BtkCellRendererPixbuf:gicon:
   *
   * The GIcon representing the icon to display.
   * If the icon theme is changed, the image will be updated
   * automatically.
   *
   * Since: 2.14
   */
  g_object_class_install_property (object_class,
                                   PROP_GICON,
                                   g_param_spec_object ("gicon",
                                                        P_("Icon"),
                                                        P_("The GIcon being displayed"),
                                                        B_TYPE_ICON,
                                                        BTK_PARAM_READWRITE));



  g_type_class_add_private (object_class, sizeof (BtkCellRendererPixbufPrivate));
}

static void
btk_cell_renderer_pixbuf_finalize (BObject *object)
{
  BtkCellRendererPixbuf *cellpixbuf = BTK_CELL_RENDERER_PIXBUF (object);
  BtkCellRendererPixbufPrivate *priv;

  priv = BTK_CELL_RENDERER_PIXBUF_GET_PRIVATE (object);
  
  if (cellpixbuf->pixbuf)
    g_object_unref (cellpixbuf->pixbuf);
  if (cellpixbuf->pixbuf_expander_open)
    g_object_unref (cellpixbuf->pixbuf_expander_open);
  if (cellpixbuf->pixbuf_expander_closed)
    g_object_unref (cellpixbuf->pixbuf_expander_closed);

  g_free (priv->stock_id);
  g_free (priv->stock_detail);
  g_free (priv->icon_name);

  if (priv->gicon)
    g_object_unref (priv->gicon);

  B_OBJECT_CLASS (btk_cell_renderer_pixbuf_parent_class)->finalize (object);
}

static void
btk_cell_renderer_pixbuf_get_property (BObject        *object,
				       buint           param_id,
				       BValue         *value,
				       BParamSpec     *pspec)
{
  BtkCellRendererPixbuf *cellpixbuf = BTK_CELL_RENDERER_PIXBUF (object);
  BtkCellRendererPixbufPrivate *priv;

  priv = BTK_CELL_RENDERER_PIXBUF_GET_PRIVATE (object);
  
  switch (param_id)
    {
    case PROP_PIXBUF:
      b_value_set_object (value, cellpixbuf->pixbuf);
      break;
    case PROP_PIXBUF_EXPANDER_OPEN:
      b_value_set_object (value, cellpixbuf->pixbuf_expander_open);
      break;
    case PROP_PIXBUF_EXPANDER_CLOSED:
      b_value_set_object (value, cellpixbuf->pixbuf_expander_closed);
      break;
    case PROP_STOCK_ID:
      b_value_set_string (value, priv->stock_id);
      break;
    case PROP_STOCK_SIZE:
      b_value_set_uint (value, priv->stock_size);
      break;
    case PROP_STOCK_DETAIL:
      b_value_set_string (value, priv->stock_detail);
      break;
    case PROP_FOLLOW_STATE:
      b_value_set_boolean (value, priv->follow_state);
      break;
    case PROP_ICON_NAME:
      b_value_set_string (value, priv->icon_name);
      break;
    case PROP_GICON:
      b_value_set_object (value, priv->gicon);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
btk_cell_renderer_pixbuf_set_property (BObject      *object,
				       buint         param_id,
				       const BValue *value,
				       BParamSpec   *pspec)
{
  BtkCellRendererPixbuf *cellpixbuf = BTK_CELL_RENDERER_PIXBUF (object);
  BtkCellRendererPixbufPrivate *priv;

  priv = BTK_CELL_RENDERER_PIXBUF_GET_PRIVATE (object);
  
  switch (param_id)
    {
    case PROP_PIXBUF:
      if (cellpixbuf->pixbuf)
	g_object_unref (cellpixbuf->pixbuf);
      cellpixbuf->pixbuf = (BdkPixbuf*) b_value_dup_object (value);
      if (cellpixbuf->pixbuf)
        {
          if (priv->stock_id)
            {
              g_free (priv->stock_id);
              priv->stock_id = NULL;
              g_object_notify (object, "stock-id");
            }
          if (priv->icon_name)
            {
              g_free (priv->icon_name);
              priv->icon_name = NULL;
              g_object_notify (object, "icon-name");
            }
          if (priv->gicon)
            {
              g_object_unref (priv->gicon);
              priv->gicon = NULL;
              g_object_notify (object, "gicon");
            }
        }
      break;
    case PROP_PIXBUF_EXPANDER_OPEN:
      if (cellpixbuf->pixbuf_expander_open)
	g_object_unref (cellpixbuf->pixbuf_expander_open);
      cellpixbuf->pixbuf_expander_open = (BdkPixbuf*) b_value_dup_object (value);
      break;
    case PROP_PIXBUF_EXPANDER_CLOSED:
      if (cellpixbuf->pixbuf_expander_closed)
	g_object_unref (cellpixbuf->pixbuf_expander_closed);
      cellpixbuf->pixbuf_expander_closed = (BdkPixbuf*) b_value_dup_object (value);
      break;
    case PROP_STOCK_ID:
      if (priv->stock_id)
        {
          if (cellpixbuf->pixbuf)
            {
              g_object_unref (cellpixbuf->pixbuf);
              cellpixbuf->pixbuf = NULL;
              g_object_notify (object, "pixbuf");
            }
          g_free (priv->stock_id);
        }
      priv->stock_id = b_value_dup_string (value);
      if (priv->stock_id)
        {
          if (cellpixbuf->pixbuf)
            {
              g_object_unref (cellpixbuf->pixbuf);
              cellpixbuf->pixbuf = NULL;
              g_object_notify (object, "pixbuf");
            }
          if (priv->icon_name)
            {
              g_free (priv->icon_name);
              priv->icon_name = NULL;
              g_object_notify (object, "icon-name");
            }
          if (priv->gicon)
            {
              g_object_unref (priv->gicon);
              priv->gicon = NULL;
              g_object_notify (object, "gicon");
            }
        }
      break;
    case PROP_STOCK_SIZE:
      priv->stock_size = b_value_get_uint (value);
      break;
    case PROP_STOCK_DETAIL:
      g_free (priv->stock_detail);
      priv->stock_detail = b_value_dup_string (value);
      break;
    case PROP_ICON_NAME:
      if (priv->icon_name)
	{
	  if (cellpixbuf->pixbuf)
	    {
	      g_object_unref (cellpixbuf->pixbuf);
	      cellpixbuf->pixbuf = NULL;
              g_object_notify (object, "pixbuf");
	    }
	  g_free (priv->icon_name);
	}
      priv->icon_name = b_value_dup_string (value);
      if (priv->icon_name)
        {
	  if (cellpixbuf->pixbuf)
	    {
              g_object_unref (cellpixbuf->pixbuf);
              cellpixbuf->pixbuf = NULL;
              g_object_notify (object, "pixbuf");
	    }
          if (priv->stock_id)
            {
              g_free (priv->stock_id);
              priv->stock_id = NULL;
              g_object_notify (object, "stock-id");
            }
          if (priv->gicon)
            {
              g_object_unref (priv->gicon);
              priv->gicon = NULL;
              g_object_notify (object, "gicon");
            }
        }
      break;
    case PROP_FOLLOW_STATE:
      priv->follow_state = b_value_get_boolean (value);
      break;
    case PROP_GICON:
      if (priv->gicon)
	{
	  if (cellpixbuf->pixbuf)
	    {
	      g_object_unref (cellpixbuf->pixbuf);
	      cellpixbuf->pixbuf = NULL;
              g_object_notify (object, "pixbuf");
	    }
	  g_object_unref (priv->gicon);
	}
      priv->gicon = (GIcon *) b_value_dup_object (value);
      if (priv->gicon)
        {
	  if (cellpixbuf->pixbuf)
	    {
              g_object_unref (cellpixbuf->pixbuf);
              cellpixbuf->pixbuf = NULL;
              g_object_notify (object, "pixbuf");
	    }
          if (priv->stock_id)
            {
              g_free (priv->stock_id);
              priv->stock_id = NULL;
              g_object_notify (object, "stock-id");
            }
          if (priv->icon_name)
            {
              g_free (priv->icon_name);
              priv->icon_name = NULL;
              g_object_notify (object, "icon-name");
            }
        }
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

/**
 * btk_cell_renderer_pixbuf_new:
 * 
 * Creates a new #BtkCellRendererPixbuf. Adjust rendering
 * parameters using object properties. Object properties can be set
 * globally (with g_object_set()). Also, with #BtkTreeViewColumn, you
 * can bind a property to a value in a #BtkTreeModel. For example, you
 * can bind the "pixbuf" property on the cell renderer to a pixbuf value
 * in the model, thus rendering a different image in each row of the
 * #BtkTreeView.
 * 
 * Return value: the new cell renderer
 **/
BtkCellRenderer *
btk_cell_renderer_pixbuf_new (void)
{
  return g_object_new (BTK_TYPE_CELL_RENDERER_PIXBUF, NULL);
}

static void
btk_cell_renderer_pixbuf_create_stock_pixbuf (BtkCellRendererPixbuf *cellpixbuf,
                                              BtkWidget             *widget)
{
  BtkCellRendererPixbufPrivate *priv;

  priv = BTK_CELL_RENDERER_PIXBUF_GET_PRIVATE (cellpixbuf);

  if (cellpixbuf->pixbuf)
    g_object_unref (cellpixbuf->pixbuf);

  cellpixbuf->pixbuf = btk_widget_render_icon (widget,
                                               priv->stock_id,
                                               priv->stock_size,
                                               priv->stock_detail);

  g_object_notify (B_OBJECT (cellpixbuf), "pixbuf");
}

static void 
btk_cell_renderer_pixbuf_create_themed_pixbuf (BtkCellRendererPixbuf *cellpixbuf,
					       BtkWidget             *widget)
{
  BtkCellRendererPixbufPrivate *priv;
  BdkScreen *screen;
  BtkIconTheme *icon_theme;
  BtkSettings *settings;
  bint width, height;

  priv = BTK_CELL_RENDERER_PIXBUF_GET_PRIVATE (cellpixbuf);

  if (cellpixbuf->pixbuf)
    {
      g_object_unref (cellpixbuf->pixbuf);
      cellpixbuf->pixbuf = NULL;
    }

  screen = btk_widget_get_screen (BTK_WIDGET (widget));
  icon_theme = btk_icon_theme_get_for_screen (screen);
  settings = btk_settings_get_for_screen (screen);

  if (!btk_icon_size_lookup_for_settings (settings,
					  priv->stock_size,
					  &width, &height))
    {
      g_warning ("Invalid icon size %u\n", priv->stock_size);
      width = height = 24;
    }

  if (priv->icon_name)
    cellpixbuf->pixbuf = btk_icon_theme_load_icon (icon_theme,
			                           priv->icon_name,
			                           MIN (width, height), 
                                                   BTK_ICON_LOOKUP_USE_BUILTIN,
                                                   NULL);
  else if (priv->gicon)
    {
      BtkIconInfo *info;

      info = btk_icon_theme_lookup_by_gicon (icon_theme,
                                             priv->gicon,
			                     MIN (width, height), 
                                             BTK_ICON_LOOKUP_USE_BUILTIN);
      if (info)
        {
          cellpixbuf->pixbuf = btk_icon_info_load_icon (info, NULL);
          btk_icon_info_free (info);
        }
    }

  g_object_notify (B_OBJECT (cellpixbuf), "pixbuf");
}

static BdkPixbuf *
create_colorized_pixbuf (BdkPixbuf *src, 
			 BdkColor  *new_color)
{
  bint i, j;
  bint width, height, has_alpha, src_row_stride, dst_row_stride;
  bint red_value, green_value, blue_value;
  buchar *target_pixels;
  buchar *original_pixels;
  buchar *pixsrc;
  buchar *pixdest;
  BdkPixbuf *dest;
  
  red_value = new_color->red / 255.0;
  green_value = new_color->green / 255.0;
  blue_value = new_color->blue / 255.0;
  
  dest = bdk_pixbuf_new (bdk_pixbuf_get_colorspace (src),
			 bdk_pixbuf_get_has_alpha (src),
			 bdk_pixbuf_get_bits_per_sample (src),
			 bdk_pixbuf_get_width (src),
			 bdk_pixbuf_get_height (src));
  
  has_alpha = bdk_pixbuf_get_has_alpha (src);
  width = bdk_pixbuf_get_width (src);
  height = bdk_pixbuf_get_height (src);
  src_row_stride = bdk_pixbuf_get_rowstride (src);
  dst_row_stride = bdk_pixbuf_get_rowstride (dest);
  target_pixels = bdk_pixbuf_get_pixels (dest);
  original_pixels = bdk_pixbuf_get_pixels (src);
  
  for (i = 0; i < height; i++) {
    pixdest = target_pixels + i*dst_row_stride;
    pixsrc = original_pixels + i*src_row_stride;
    for (j = 0; j < width; j++) {		
      *pixdest++ = (*pixsrc++ * red_value) >> 8;
      *pixdest++ = (*pixsrc++ * green_value) >> 8;
      *pixdest++ = (*pixsrc++ * blue_value) >> 8;
      if (has_alpha) {
	*pixdest++ = *pixsrc++;
      }
    }
  }
  return dest;
}


static void
btk_cell_renderer_pixbuf_get_size (BtkCellRenderer *cell,
				   BtkWidget       *widget,
				   BdkRectangle    *cell_area,
				   bint            *x_offset,
				   bint            *y_offset,
				   bint            *width,
				   bint            *height)
{
  BtkCellRendererPixbuf *cellpixbuf = (BtkCellRendererPixbuf *) cell;
  BtkCellRendererPixbufPrivate *priv;
  bint pixbuf_width  = 0;
  bint pixbuf_height = 0;
  bint calc_width;
  bint calc_height;

  priv = BTK_CELL_RENDERER_PIXBUF_GET_PRIVATE (cell);

  if (!cellpixbuf->pixbuf)
    {
      if (priv->stock_id)
	btk_cell_renderer_pixbuf_create_stock_pixbuf (cellpixbuf, widget);
      else if (priv->icon_name || priv->gicon)
	btk_cell_renderer_pixbuf_create_themed_pixbuf (cellpixbuf, widget);
    }
  
  if (cellpixbuf->pixbuf)
    {
      pixbuf_width  = bdk_pixbuf_get_width (cellpixbuf->pixbuf);
      pixbuf_height = bdk_pixbuf_get_height (cellpixbuf->pixbuf);
    }
  if (cellpixbuf->pixbuf_expander_open)
    {
      pixbuf_width  = MAX (pixbuf_width, bdk_pixbuf_get_width (cellpixbuf->pixbuf_expander_open));
      pixbuf_height = MAX (pixbuf_height, bdk_pixbuf_get_height (cellpixbuf->pixbuf_expander_open));
    }
  if (cellpixbuf->pixbuf_expander_closed)
    {
      pixbuf_width  = MAX (pixbuf_width, bdk_pixbuf_get_width (cellpixbuf->pixbuf_expander_closed));
      pixbuf_height = MAX (pixbuf_height, bdk_pixbuf_get_height (cellpixbuf->pixbuf_expander_closed));
    }
  
  calc_width  = (bint) cell->xpad * 2 + pixbuf_width;
  calc_height = (bint) cell->ypad * 2 + pixbuf_height;
  
  if (cell_area && pixbuf_width > 0 && pixbuf_height > 0)
    {
      if (x_offset)
	{
	  *x_offset = (((btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL) ?
                        (1.0 - cell->xalign) : cell->xalign) * 
                       (cell_area->width - calc_width));
	  *x_offset = MAX (*x_offset, 0);
	}
      if (y_offset)
	{
	  *y_offset = (cell->yalign *
                       (cell_area->height - calc_height));
          *y_offset = MAX (*y_offset, 0);
	}
    }
  else
    {
      if (x_offset) *x_offset = 0;
      if (y_offset) *y_offset = 0;
    }

  if (width)
    *width = calc_width;
  
  if (height)
    *height = calc_height;
}

static void
btk_cell_renderer_pixbuf_render (BtkCellRenderer      *cell,
				 BdkWindow            *window,
				 BtkWidget            *widget,
				 BdkRectangle         *background_area,
				 BdkRectangle         *cell_area,
				 BdkRectangle         *expose_area,
				 BtkCellRendererState  flags)

{
  BtkCellRendererPixbuf *cellpixbuf = (BtkCellRendererPixbuf *) cell;
  BtkCellRendererPixbufPrivate *priv;
  BdkPixbuf *pixbuf;
  BdkPixbuf *invisible = NULL;
  BdkPixbuf *colorized = NULL;
  BdkRectangle pix_rect;
  BdkRectangle draw_rect;
  bairo_t *cr;

  priv = BTK_CELL_RENDERER_PIXBUF_GET_PRIVATE (cell);

  btk_cell_renderer_pixbuf_get_size (cell, widget, cell_area,
				     &pix_rect.x,
				     &pix_rect.y,
				     &pix_rect.width,
				     &pix_rect.height);

  pix_rect.x += cell_area->x + cell->xpad;
  pix_rect.y += cell_area->y + cell->ypad;
  pix_rect.width  -= cell->xpad * 2;
  pix_rect.height -= cell->ypad * 2;

  if (!bdk_rectangle_intersect (cell_area, &pix_rect, &draw_rect) ||
      !bdk_rectangle_intersect (expose_area, &draw_rect, &draw_rect))
    return;

  pixbuf = cellpixbuf->pixbuf;

  if (cell->is_expander)
    {
      if (cell->is_expanded &&
	  cellpixbuf->pixbuf_expander_open != NULL)
	pixbuf = cellpixbuf->pixbuf_expander_open;
      else if (!cell->is_expanded &&
	       cellpixbuf->pixbuf_expander_closed != NULL)
	pixbuf = cellpixbuf->pixbuf_expander_closed;
    }

  if (!pixbuf)
    return;

  if (btk_widget_get_state (widget) == BTK_STATE_INSENSITIVE || !cell->sensitive)
    {
      BtkIconSource *source;
      
      source = btk_icon_source_new ();
      btk_icon_source_set_pixbuf (source, pixbuf);
      /* The size here is arbitrary; since size isn't
       * wildcarded in the source, it isn't supposed to be
       * scaled by the engine function
       */
      btk_icon_source_set_size (source, BTK_ICON_SIZE_SMALL_TOOLBAR);
      btk_icon_source_set_size_wildcarded (source, FALSE);
      
     invisible = btk_style_render_icon (widget->style,
					source,
					btk_widget_get_direction (widget),
					BTK_STATE_INSENSITIVE,
					/* arbitrary */
					(BtkIconSize)-1,
					widget,
					"btkcellrendererpixbuf");
     
     btk_icon_source_free (source);
     
     pixbuf = invisible;
    }
  else if (priv->follow_state && 
	   (flags & (BTK_CELL_RENDERER_SELECTED|BTK_CELL_RENDERER_PRELIT)) != 0)
    {
      BtkStateType state;

      if ((flags & BTK_CELL_RENDERER_SELECTED) != 0)
	{
	  if (btk_widget_has_focus (widget))
	    state = BTK_STATE_SELECTED;
	  else
	    state = BTK_STATE_ACTIVE;
	}
      else
	state = BTK_STATE_PRELIGHT;

      colorized = create_colorized_pixbuf (pixbuf,
					   &widget->style->base[state]);

      pixbuf = colorized;
    }

  cr = bdk_bairo_create (window);
  
  bdk_bairo_set_source_pixbuf (cr, pixbuf, pix_rect.x, pix_rect.y);
  bdk_bairo_rectangle (cr, &draw_rect);
  bairo_fill (cr);

  bairo_destroy (cr);
  
  if (invisible)
    g_object_unref (invisible);

  if (colorized)
    g_object_unref (colorized);
}

#define __BTK_CELL_RENDERER_PIXBUF_C__
#include "btkaliasdef.c"
