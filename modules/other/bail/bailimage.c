/* BAIL - The BUNNY Accessibility Implementation Library
 * Copyright 2001 Sun Microsystems Inc.
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

#include "config.h"

#include <string.h>
#include <btk/btk.h>
#include "bailimage.h"

static void      bail_image_class_init         (BailImageClass *klass);
static void      bail_image_init               (BailImage      *image);
static void      bail_image_initialize         (BatkObject       *accessible,
                                                gpointer        data);
static const gchar* bail_image_get_name  (BatkObject     *accessible);


static void      batk_image_interface_init      (BatkImageIface  *iface);

static const gchar *
                 bail_image_get_image_description (BatkImage     *image);
static void	 bail_image_get_image_position    (BatkImage     *image,
                                                   gint         *x,
                                                   gint         *y,
                                                   BatkCoordType coord_type);
static void      bail_image_get_image_size     (BatkImage        *image,
                                                gint            *width,
                                                gint            *height);
static gboolean  bail_image_set_image_description (BatkImage     *image,
                                                const gchar     *description);
static void      bail_image_finalize           (BObject         *object);

G_DEFINE_TYPE_WITH_CODE (BailImage, bail_image, BAIL_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_IMAGE, batk_image_interface_init))

static void
bail_image_class_init (BailImageClass *klass)
{
  BObjectClass *bobject_class = B_OBJECT_CLASS (klass);
  BatkObjectClass  *class = BATK_OBJECT_CLASS (klass);

  bobject_class->finalize = bail_image_finalize;
  class->initialize = bail_image_initialize;
  class->get_name = bail_image_get_name;
}

static void
bail_image_init (BailImage *image)
{
  image->image_description = NULL;
}

static void
bail_image_initialize (BatkObject *accessible,
                       gpointer data)
{
  BATK_OBJECT_CLASS (bail_image_parent_class)->initialize (accessible, data);

  accessible->role = BATK_ROLE_ICON;
}

/* Copied from btktoolbar.c, keep in sync */
static gchar *
elide_underscores (const gchar *original)
{
  gchar *q, *result;
  const gchar *p, *end;
  gsize len;
  gboolean last_underscore;
  
  if (!original)
    return NULL;

  len = strlen (original);
  q = result = g_malloc (len + 1);
  last_underscore = FALSE;
  
  end = original + len;
  for (p = original; p < end; p++)
    {
      if (!last_underscore && *p == '_')
	last_underscore = TRUE;
      else
	{
	  last_underscore = FALSE;
	  if (original + 2 <= p && p + 1 <= end && 
              p[-2] == '(' && p[-1] == '_' && p[0] != '_' && p[1] == ')')
	    {
	      q--;
	      *q = '\0';
	      p++;
	    }
	  else
	    *q++ = *p;
	}
    }

  if (last_underscore)
    *q++ = '_';
  
  *q = '\0';
  
  return result;
}

static const gchar*
bail_image_get_name (BatkObject *accessible)
{
  BtkWidget* widget;
  BtkImage *image;
  BailImage *image_accessible;
  BtkStockItem stock_item;
  const gchar *name;

  name = BATK_OBJECT_CLASS (bail_image_parent_class)->get_name (accessible);
  if (name)
    return name;

  widget = BTK_ACCESSIBLE (accessible)->widget;
  /*
   * State is defunct
   */
  if (widget == NULL)
    return NULL;

  g_return_val_if_fail (BTK_IS_IMAGE (widget), NULL);
  image = BTK_IMAGE (widget);
  image_accessible = BAIL_IMAGE (accessible);

  g_free (image_accessible->stock_name);
  image_accessible->stock_name = NULL;

  if (image->storage_type != BTK_IMAGE_STOCK ||
      image->data.stock.stock_id == NULL)
    return NULL;

  if (!btk_stock_lookup (image->data.stock.stock_id, &stock_item))
    return NULL;

  image_accessible->stock_name = elide_underscores (stock_item.label);
  return image_accessible->stock_name;
}

static void
batk_image_interface_init (BatkImageIface *iface)
{
  iface->get_image_description = bail_image_get_image_description;
  iface->get_image_position = bail_image_get_image_position;
  iface->get_image_size = bail_image_get_image_size;
  iface->set_image_description = bail_image_set_image_description;
}

static const gchar *
bail_image_get_image_description (BatkImage     *image)
{
  BailImage* aimage = BAIL_IMAGE (image);

  return aimage->image_description;
}

static void
bail_image_get_image_position (BatkImage     *image,
                               gint         *x,
                               gint         *y,
                               BatkCoordType coord_type)
{
  batk_component_get_position (BATK_COMPONENT (image), x, y, coord_type);
}

static void
bail_image_get_image_size (BatkImage *image, 
                           gint     *width,
                           gint     *height)
{
  BtkWidget* widget;
  BtkImage *btk_image;
  BtkImageType image_type;

  widget = BTK_ACCESSIBLE (image)->widget;
  if (widget == 0)
  {
    /* State is defunct */
    *height = -1;
    *width = -1;
    return;
  }

  btk_image = BTK_IMAGE(widget);

  image_type = btk_image_get_storage_type(btk_image);
 
  switch(image_type) {
    case BTK_IMAGE_PIXMAP:
    {	
      BdkPixmap *pixmap;
      btk_image_get_pixmap(btk_image, &pixmap, NULL);
      bdk_pixmap_get_size (pixmap, width, height);
      break;
    }
    case BTK_IMAGE_PIXBUF:
    {
      BdkPixbuf *pixbuf;
      pixbuf = btk_image_get_pixbuf(btk_image);
      *height = bdk_pixbuf_get_height(pixbuf);
      *width = bdk_pixbuf_get_width(pixbuf);
      break;
    }
    case BTK_IMAGE_IMAGE:
    {
      BdkImage *bdk_image;
      btk_image_get_image(btk_image, &bdk_image, NULL);
      *height = bdk_image->height;
      *width = bdk_image->width;
      break;
    }
    case BTK_IMAGE_STOCK:
    case BTK_IMAGE_ICON_SET:
    case BTK_IMAGE_ICON_NAME:
    case BTK_IMAGE_GICON:
    {
      BtkIconSize size;
      BtkSettings *settings;

      settings = btk_settings_get_for_screen (btk_widget_get_screen (widget));

      g_object_get (btk_image, "icon-size", &size, NULL);
      btk_icon_size_lookup_for_settings (settings, size, width, height);
      break;
    }
    case BTK_IMAGE_ANIMATION:
    {
      BdkPixbufAnimation *animation;
      animation = btk_image_get_animation(btk_image);
      *height = bdk_pixbuf_animation_get_height(animation);
      *width = bdk_pixbuf_animation_get_width(animation);
      break;
    }
    default:
    {
      *height = -1;
      *width = -1;
      break;
    }
  }
}

static gboolean
bail_image_set_image_description (BatkImage     *image,
                                  const gchar  *description)
{
  BailImage* aimage = BAIL_IMAGE (image);

  g_free (aimage->image_description);
  aimage->image_description = g_strdup (description);
  return TRUE;
}

static void
bail_image_finalize (BObject      *object)
{
  BailImage *aimage = BAIL_IMAGE (object);

  g_free (aimage->image_description);
  g_free (aimage->stock_name);

  B_OBJECT_CLASS (bail_image_parent_class)->finalize (object);
}
