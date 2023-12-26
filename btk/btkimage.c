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
#include <string.h>

#undef BDK_DISABLE_DEPRECATED

#include "btkcontainer.h"
#include "btkimage.h"
#include "btkiconfactory.h"
#include "btkstock.h"
#include "btkicontheme.h"
#include "btkintl.h"
#include "btkprivate.h"
#include "btkalias.h"

/**
 * SECTION:btkimage
 * @Short_description: A widget displaying an image
 * @Title: BtkImage
 * @See_also:#BdkPixbuf
 *
 * The #BtkImage widget displays an image. Various kinds of object
 * can be displayed as an image; most typically, you would load a
 * #BdkPixbuf ("pixel buffer") from a file, and then display that.
 * There's a convenience function to do this, btk_image_new_from_file(),
 * used as follows:
 * <informalexample><programlisting>
 *   BtkWidget *image;
 *   image = btk_image_new_from_file ("myfile.png");
 * </programlisting></informalexample>
 * If the file isn't loaded successfully, the image will contain a
 * "broken image" icon similar to that used in many web browsers.
 * If you want to handle errors in loading the file yourself,
 * for example by displaying an error message, then load the image with
 * bdk_pixbuf_new_from_file(), then create the #BtkImage with
 * btk_image_new_from_pixbuf().
 *
 * The image file may contain an animation, if so the #BtkImage will
 * display an animation (#BdkPixbufAnimation) instead of a static image.
 *
 * #BtkImage is a subclass of #BtkMisc, which implies that you can
 * align it (center, left, right) and add padding to it, using
 * #BtkMisc methods.
 *
 * #BtkImage is a "no window" widget (has no #BdkWindow of its own),
 * so by default does not receive events. If you want to receive events
 * on the image, such as button clicks, place the image inside a
 * #BtkEventBox, then connect to the event signals on the event box.
 * <example>
 * <title>Handling button press events on a
 * <structname>BtkImage</structname>.</title>
 * <programlisting>
 *   static gboolean
 *   button_press_callback (BtkWidget      *event_box,
 *                          BdkEventButton *event,
 *                          gpointer        data)
 *   {
 *     g_print ("Event box clicked at coordinates &percnt;f,&percnt;f\n",
 *              event->x, event->y);
 *
 *     /<!---->* Returning TRUE means we handled the event, so the signal
 *      * emission should be stopped (don't call any further
 *      * callbacks that may be connected). Return FALSE
 *      * to continue invoking callbacks.
 *      *<!---->/
 *     return TRUE;
 *   }
 *
 *   static BtkWidget*
 *   create_image (void)
 *   {
 *     BtkWidget *image;
 *     BtkWidget *event_box;
 *
 *     image = btk_image_new_from_file ("myfile.png");
 *
 *     event_box = btk_event_box_new (<!-- -->);
 *
 *     btk_container_add (BTK_CONTAINER (event_box), image);
 *
 *     g_signal_connect (B_OBJECT (event_box),
 *                       "button_press_event",
 *                       G_CALLBACK (button_press_callback),
 *                       image);
 *
 *     return image;
 *   }
 * </programlisting>
 * </example>
 *
 * When handling events on the event box, keep in mind that coordinates
 * in the image may be different from event box coordinates due to
 * the alignment and padding settings on the image (see #BtkMisc).
 * The simplest way to solve this is to set the alignment to 0.0
 * (left/top), and set the padding to zero. Then the origin of
 * the image will be the same as the origin of the event box.
 *
 * Sometimes an application will want to avoid depending on external data
 * files, such as image files. BTK+ comes with a program to avoid this,
 * called <application>bdk-pixbuf-csource</application>. This program
 * allows you to convert an image into a C variable declaration, which
 * can then be loaded into a #BdkPixbuf using
 * bdk_pixbuf_new_from_inline().
 */

typedef struct _BtkImagePrivate BtkImagePrivate;

struct _BtkImagePrivate
{
  /* Only used with BTK_IMAGE_ANIMATION, BTK_IMAGE_PIXBUF */
  gchar *filename;

  gint pixel_size;
  guint need_calc_size : 1;
};

#define BTK_IMAGE_GET_PRIVATE(obj) (B_TYPE_INSTANCE_GET_PRIVATE ((obj), BTK_TYPE_IMAGE, BtkImagePrivate))


#define DEFAULT_ICON_SIZE BTK_ICON_SIZE_BUTTON
static gint btk_image_expose       (BtkWidget      *widget,
                                    BdkEventExpose *event);
static void btk_image_unmap        (BtkWidget      *widget);
static void btk_image_unrealize    (BtkWidget      *widget);
static void btk_image_size_request (BtkWidget      *widget,
                                    BtkRequisition *requisition);
static void btk_image_style_set    (BtkWidget      *widget,
				    BtkStyle       *prev_style);
static void btk_image_screen_changed (BtkWidget    *widget,
				      BdkScreen    *prev_screen);
static void btk_image_destroy      (BtkObject      *object);
static void btk_image_reset        (BtkImage       *image);
static void btk_image_calc_size    (BtkImage       *image);

static void btk_image_update_size  (BtkImage       *image,
                                    gint            image_width,
                                    gint            image_height);

static void btk_image_set_property      (BObject          *object,
					 guint             prop_id,
					 const BValue     *value,
					 BParamSpec       *pspec);
static void btk_image_get_property      (BObject          *object,
					 guint             prop_id,
					 BValue           *value,
					 BParamSpec       *pspec);

static void icon_theme_changed          (BtkImage         *image);

enum
{
  PROP_0,
  PROP_PIXBUF,
  PROP_PIXMAP,
  PROP_IMAGE,
  PROP_MASK,
  PROP_FILE,
  PROP_STOCK,
  PROP_ICON_SET,
  PROP_ICON_SIZE,
  PROP_PIXEL_SIZE,
  PROP_PIXBUF_ANIMATION,
  PROP_ICON_NAME,
  PROP_STORAGE_TYPE,
  PROP_GICON
};

G_DEFINE_TYPE (BtkImage, btk_image, BTK_TYPE_MISC)

static void
btk_image_class_init (BtkImageClass *class)
{
  BObjectClass *bobject_class;
  BtkObjectClass *object_class;
  BtkWidgetClass *widget_class;

  bobject_class = B_OBJECT_CLASS (class);
  
  bobject_class->set_property = btk_image_set_property;
  bobject_class->get_property = btk_image_get_property;
  
  object_class = BTK_OBJECT_CLASS (class);
  
  object_class->destroy = btk_image_destroy;

  widget_class = BTK_WIDGET_CLASS (class);
  
  widget_class->expose_event = btk_image_expose;
  widget_class->size_request = btk_image_size_request;
  widget_class->unmap = btk_image_unmap;
  widget_class->unrealize = btk_image_unrealize;
  widget_class->style_set = btk_image_style_set;
  widget_class->screen_changed = btk_image_screen_changed;
  
  g_object_class_install_property (bobject_class,
                                   PROP_PIXBUF,
                                   g_param_spec_object ("pixbuf",
                                                        P_("Pixbuf"),
                                                        P_("A BdkPixbuf to display"),
                                                        BDK_TYPE_PIXBUF,
                                                        BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_PIXMAP,
                                   g_param_spec_object ("pixmap",
                                                        P_("Pixmap"),
                                                        P_("A BdkPixmap to display"),
                                                        BDK_TYPE_PIXMAP,
                                                        BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_IMAGE,
                                   g_param_spec_object ("image",
                                                        P_("Image"),
                                                        P_("A BdkImage to display"),
                                                        BDK_TYPE_IMAGE,
                                                        BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_MASK,
                                   g_param_spec_object ("mask",
                                                        P_("Mask"),
                                                        P_("Mask bitmap to use with BdkImage or BdkPixmap"),
                                                        BDK_TYPE_PIXMAP,
                                                        BTK_PARAM_READWRITE));
  
  g_object_class_install_property (bobject_class,
                                   PROP_FILE,
                                   g_param_spec_string ("file",
                                                        P_("Filename"),
                                                        P_("Filename to load and display"),
                                                        NULL,
                                                        BTK_PARAM_READWRITE));
  

  g_object_class_install_property (bobject_class,
                                   PROP_STOCK,
                                   g_param_spec_string ("stock",
                                                        P_("Stock ID"),
                                                        P_("Stock ID for a stock image to display"),
                                                        NULL,
                                                        BTK_PARAM_READWRITE));
  
  g_object_class_install_property (bobject_class,
                                   PROP_ICON_SET,
                                   g_param_spec_boxed ("icon-set",
                                                       P_("Icon set"),
                                                       P_("Icon set to display"),
                                                       BTK_TYPE_ICON_SET,
                                                       BTK_PARAM_READWRITE));
  
  g_object_class_install_property (bobject_class,
                                   PROP_ICON_SIZE,
                                   g_param_spec_int ("icon-size",
                                                     P_("Icon size"),
                                                     P_("Symbolic size to use for stock icon, icon set or named icon"),
                                                     0, G_MAXINT,
                                                     DEFAULT_ICON_SIZE,
                                                     BTK_PARAM_READWRITE));
  /**
   * BtkImage:pixel-size:
   *
   * The "pixel-size" property can be used to specify a fixed size
   * overriding the #BtkImage:icon-size property for images of type 
   * %BTK_IMAGE_ICON_NAME. 
   *
   * Since: 2.6
   */
  g_object_class_install_property (bobject_class,
				   PROP_PIXEL_SIZE,
				   g_param_spec_int ("pixel-size",
						     P_("Pixel size"),
						     P_("Pixel size to use for named icon"),
						     -1, G_MAXINT,
						     -1,
						     BTK_PARAM_READWRITE));
  
  g_object_class_install_property (bobject_class,
                                   PROP_PIXBUF_ANIMATION,
                                   g_param_spec_object ("pixbuf-animation",
                                                        P_("Animation"),
                                                        P_("BdkPixbufAnimation to display"),
                                                        BDK_TYPE_PIXBUF_ANIMATION,
                                                        BTK_PARAM_READWRITE));

  /**
   * BtkImage:icon-name:
   *
   * The name of the icon in the icon theme. If the icon theme is
   * changed, the image will be updated automatically.
   *
   * Since: 2.6
   */
  g_object_class_install_property (bobject_class,
                                   PROP_ICON_NAME,
                                   g_param_spec_string ("icon-name",
                                                        P_("Icon Name"),
                                                        P_("The name of the icon from the icon theme"),
                                                        NULL,
                                                        BTK_PARAM_READWRITE));
  
  /**
   * BtkImage:gicon:
   *
   * The GIcon displayed in the BtkImage. For themed icons,
   * If the icon theme is changed, the image will be updated
   * automatically.
   *
   * Since: 2.14
   */
  g_object_class_install_property (bobject_class,
                                   PROP_GICON,
                                   g_param_spec_object ("gicon",
                                                        P_("Icon"),
                                                        P_("The GIcon being displayed"),
                                                        B_TYPE_ICON,
                                                        BTK_PARAM_READWRITE));
  
  g_object_class_install_property (bobject_class,
                                   PROP_STORAGE_TYPE,
                                   g_param_spec_enum ("storage-type",
                                                      P_("Storage type"),
                                                      P_("The representation being used for image data"),
                                                      BTK_TYPE_IMAGE_TYPE,
                                                      BTK_IMAGE_EMPTY,
                                                      BTK_PARAM_READABLE));

  g_type_class_add_private (object_class, sizeof (BtkImagePrivate));
}

static void
btk_image_init (BtkImage *image)
{
  BtkImagePrivate *priv = BTK_IMAGE_GET_PRIVATE (image);

  btk_widget_set_has_window (BTK_WIDGET (image), FALSE);

  image->storage_type = BTK_IMAGE_EMPTY;
  image->icon_size = DEFAULT_ICON_SIZE;
  image->mask = NULL;

  priv->pixel_size = -1;

  priv->filename = NULL;
}

static void
btk_image_destroy (BtkObject *object)
{
  BtkImage *image = BTK_IMAGE (object);

  btk_image_reset (image);
  
  BTK_OBJECT_CLASS (btk_image_parent_class)->destroy (object);
}

static void 
btk_image_set_property (BObject      *object,
			guint         prop_id,
			const BValue *value,
			BParamSpec   *pspec)
{
  BtkImage *image;

  image = BTK_IMAGE (object);
  
  switch (prop_id)
    {
    case PROP_PIXBUF:
      btk_image_set_from_pixbuf (image,
                                 b_value_get_object (value));
      break;
    case PROP_PIXMAP:
      btk_image_set_from_pixmap (image,
                                 b_value_get_object (value),
                                 image->mask);
      break;
    case PROP_IMAGE:
      btk_image_set_from_image (image,
                                b_value_get_object (value),
                                image->mask);
      break;
    case PROP_MASK:
      if (image->storage_type == BTK_IMAGE_PIXMAP)
        btk_image_set_from_pixmap (image,
                                   image->data.pixmap.pixmap,
                                   b_value_get_object (value));
      else if (image->storage_type == BTK_IMAGE_IMAGE)
        btk_image_set_from_image (image,
                                  image->data.image.image,
                                  b_value_get_object (value));
      else
        {
          BdkBitmap *mask;

          mask = b_value_get_object (value);

          if (mask)
            g_object_ref (mask);
          
          btk_image_clear (image);

          image->mask = mask;
        }
      break;
    case PROP_FILE:
      btk_image_set_from_file (image, b_value_get_string (value));
      break;
    case PROP_STOCK:
      btk_image_set_from_stock (image, b_value_get_string (value),
                                image->icon_size);
      break;
    case PROP_ICON_SET:
      btk_image_set_from_icon_set (image, b_value_get_boxed (value),
                                   image->icon_size);
      break;
    case PROP_ICON_SIZE:
      if (image->storage_type == BTK_IMAGE_STOCK)
        btk_image_set_from_stock (image,
                                  image->data.stock.stock_id,
                                  b_value_get_int (value));
      else if (image->storage_type == BTK_IMAGE_ICON_SET)
        btk_image_set_from_icon_set (image,
                                     image->data.icon_set.icon_set,
                                     b_value_get_int (value));
      else if (image->storage_type == BTK_IMAGE_ICON_NAME)
        btk_image_set_from_icon_name (image,
				      image->data.name.icon_name,
				      b_value_get_int (value));
      else if (image->storage_type == BTK_IMAGE_GICON)
        btk_image_set_from_gicon (image,
                                  image->data.gicon.icon,
                                  b_value_get_int (value));
      else
        /* Save to be used when STOCK, ICON_SET, ICON_NAME or GICON property comes in */
        image->icon_size = b_value_get_int (value);
      break;
    case PROP_PIXEL_SIZE:
      btk_image_set_pixel_size (image, b_value_get_int (value));
      break;
    case PROP_PIXBUF_ANIMATION:
      btk_image_set_from_animation (image,
                                    b_value_get_object (value));
      break;
    case PROP_ICON_NAME:
      btk_image_set_from_icon_name (image, b_value_get_string (value),
				    image->icon_size);
      break;
    case PROP_GICON:
      btk_image_set_from_gicon (image, b_value_get_object (value),
				image->icon_size);
      break;

    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void 
btk_image_get_property (BObject     *object,
			guint        prop_id,
			BValue      *value,
			BParamSpec  *pspec)
{
  BtkImage *image;
  BtkImagePrivate *priv;

  image = BTK_IMAGE (object);
  priv = BTK_IMAGE_GET_PRIVATE (image);

  /* The "getter" functions whine if you try to get the wrong
   * storage type. This function is instead robust against that,
   * so that GUI builders don't have to jump through hoops
   * to avoid g_warning
   */
  
  switch (prop_id)
    {
    case PROP_PIXBUF:
      if (image->storage_type != BTK_IMAGE_PIXBUF)
        b_value_set_object (value, NULL);
      else
        b_value_set_object (value,
                            btk_image_get_pixbuf (image));
      break;
    case PROP_PIXMAP:
      if (image->storage_type != BTK_IMAGE_PIXMAP)
        b_value_set_object (value, NULL);
      else
        b_value_set_object (value,
                            image->data.pixmap.pixmap);
      break;
    case PROP_MASK:
      b_value_set_object (value, image->mask);
      break;
    case PROP_IMAGE:
      if (image->storage_type != BTK_IMAGE_IMAGE)
        b_value_set_object (value, NULL);
      else
        b_value_set_object (value,
                            image->data.image.image);
      break;
    case PROP_FILE:
      b_value_set_string (value, priv->filename);
      break;
    case PROP_STOCK:
      if (image->storage_type != BTK_IMAGE_STOCK)
        b_value_set_string (value, NULL);
      else
        b_value_set_string (value,
                            image->data.stock.stock_id);
      break;
    case PROP_ICON_SET:
      if (image->storage_type != BTK_IMAGE_ICON_SET)
        b_value_set_boxed (value, NULL);
      else
        b_value_set_boxed (value,
                           image->data.icon_set.icon_set);
      break;      
    case PROP_ICON_SIZE:
      b_value_set_int (value, image->icon_size);
      break;
    case PROP_PIXEL_SIZE:
      b_value_set_int (value, priv->pixel_size);
      break;
    case PROP_PIXBUF_ANIMATION:
      if (image->storage_type != BTK_IMAGE_ANIMATION)
        b_value_set_object (value, NULL);
      else
        b_value_set_object (value,
                            image->data.anim.anim);
      break;
    case PROP_ICON_NAME:
      if (image->storage_type != BTK_IMAGE_ICON_NAME)
	b_value_set_string (value, NULL);
      else
	b_value_set_string (value,
			    image->data.name.icon_name);
      break;
    case PROP_GICON:
      if (image->storage_type != BTK_IMAGE_GICON)
	b_value_set_object (value, NULL);
      else
	b_value_set_object (value,
			    image->data.gicon.icon);
      break;
    case PROP_STORAGE_TYPE:
      b_value_set_enum (value, image->storage_type);
      break;
      
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


/**
 * btk_image_new_from_pixmap:
 * @pixmap: (allow-none): a #BdkPixmap, or %NULL
 * @mask: (allow-none): a #BdkBitmap, or %NULL
 *
 * Creates a #BtkImage widget displaying @pixmap with a @mask.
 * A #BdkPixmap is a server-side image buffer in the pixel format of the
 * current display. The #BtkImage does not assume a reference to the
 * pixmap or mask; you still need to unref them if you own references.
 * #BtkImage will add its own reference rather than adopting yours.
 * 
 * Return value: a new #BtkImage
 **/
BtkWidget*
btk_image_new_from_pixmap (BdkPixmap *pixmap,
                           BdkBitmap *mask)
{
  BtkImage *image;

  image = g_object_new (BTK_TYPE_IMAGE, NULL);

  btk_image_set_from_pixmap (image, pixmap, mask);

  return BTK_WIDGET (image);
}

/**
 * btk_image_new_from_image:
 * @image: (allow-none): a #BdkImage, or %NULL
 * @mask: (allow-none): a #BdkBitmap, or %NULL
 *
 * Creates a #BtkImage widget displaying a @image with a @mask.
 * A #BdkImage is a client-side image buffer in the pixel format of the
 * current display. The #BtkImage does not assume a reference to the
 * image or mask; you still need to unref them if you own references.
 * #BtkImage will add its own reference rather than adopting yours.
 * 
 * Return value: a new #BtkImage
 **/
BtkWidget*
btk_image_new_from_image  (BdkImage  *bdk_image,
                           BdkBitmap *mask)
{
  BtkImage *image;

  image = g_object_new (BTK_TYPE_IMAGE, NULL);

  btk_image_set_from_image (image, bdk_image, mask);

  return BTK_WIDGET (image);
}

/**
 * btk_image_new_from_file:
 * @filename: a filename
 * 
 * Creates a new #BtkImage displaying the file @filename. If the file
 * isn't found or can't be loaded, the resulting #BtkImage will
 * display a "broken image" icon. This function never returns %NULL,
 * it always returns a valid #BtkImage widget.
 *
 * If the file contains an animation, the image will contain an
 * animation.
 *
 * If you need to detect failures to load the file, use
 * bdk_pixbuf_new_from_file() to load the file yourself, then create
 * the #BtkImage from the pixbuf. (Or for animations, use
 * bdk_pixbuf_animation_new_from_file()).
 *
 * The storage type (btk_image_get_storage_type()) of the returned
 * image is not defined, it will be whatever is appropriate for
 * displaying the file.
 * 
 * Return value: a new #BtkImage
 **/
BtkWidget*
btk_image_new_from_file   (const gchar *filename)
{
  BtkImage *image;

  image = g_object_new (BTK_TYPE_IMAGE, NULL);

  btk_image_set_from_file (image, filename);

  return BTK_WIDGET (image);
}

/**
 * btk_image_new_from_pixbuf:
 * @pixbuf: (allow-none): a #BdkPixbuf, or %NULL
 *
 * Creates a new #BtkImage displaying @pixbuf.
 * The #BtkImage does not assume a reference to the
 * pixbuf; you still need to unref it if you own references.
 * #BtkImage will add its own reference rather than adopting yours.
 * 
 * Note that this function just creates an #BtkImage from the pixbuf. The
 * #BtkImage created will not react to state changes. Should you want that, 
 * you should use btk_image_new_from_icon_set().
 * 
 * Return value: a new #BtkImage
 **/
BtkWidget*
btk_image_new_from_pixbuf (BdkPixbuf *pixbuf)
{
  BtkImage *image;

  image = g_object_new (BTK_TYPE_IMAGE, NULL);

  btk_image_set_from_pixbuf (image, pixbuf);

  return BTK_WIDGET (image);  
}

/**
 * btk_image_new_from_stock:
 * @stock_id: a stock icon name
 * @size: (type int): a stock icon size
 * 
 * Creates a #BtkImage displaying a stock icon. Sample stock icon
 * names are #BTK_STOCK_OPEN, #BTK_STOCK_QUIT. Sample stock sizes
 * are #BTK_ICON_SIZE_MENU, #BTK_ICON_SIZE_SMALL_TOOLBAR. If the stock
 * icon name isn't known, the image will be empty.
 * You can register your own stock icon names, see
 * btk_icon_factory_add_default() and btk_icon_factory_add().
 * 
 * Return value: a new #BtkImage displaying the stock icon
 **/
BtkWidget*
btk_image_new_from_stock (const gchar    *stock_id,
                          BtkIconSize     size)
{
  BtkImage *image;

  image = g_object_new (BTK_TYPE_IMAGE, NULL);

  btk_image_set_from_stock (image, stock_id, size);

  return BTK_WIDGET (image);
}

/**
 * btk_image_new_from_icon_set:
 * @icon_set: a #BtkIconSet
 * @size: (type int): a stock icon size
 *
 * Creates a #BtkImage displaying an icon set. Sample stock sizes are
 * #BTK_ICON_SIZE_MENU, #BTK_ICON_SIZE_SMALL_TOOLBAR. Instead of using
 * this function, usually it's better to create a #BtkIconFactory, put
 * your icon sets in the icon factory, add the icon factory to the
 * list of default factories with btk_icon_factory_add_default(), and
 * then use btk_image_new_from_stock(). This will allow themes to
 * override the icon you ship with your application.
 *
 * The #BtkImage does not assume a reference to the
 * icon set; you still need to unref it if you own references.
 * #BtkImage will add its own reference rather than adopting yours.
 * 
 * Return value: a new #BtkImage
 **/
BtkWidget*
btk_image_new_from_icon_set (BtkIconSet     *icon_set,
                             BtkIconSize     size)
{
  BtkImage *image;

  image = g_object_new (BTK_TYPE_IMAGE, NULL);

  btk_image_set_from_icon_set (image, icon_set, size);

  return BTK_WIDGET (image);
}

/**
 * btk_image_new_from_animation:
 * @animation: an animation
 * 
 * Creates a #BtkImage displaying the given animation.
 * The #BtkImage does not assume a reference to the
 * animation; you still need to unref it if you own references.
 * #BtkImage will add its own reference rather than adopting yours.
 *
 * Note that the animation frames are shown using a timeout with
 * #G_PRIORITY_DEFAULT. When using animations to indicate busyness,
 * keep in mind that the animation will only be shown if the main loop
 * is not busy with something that has a higher priority.
 *
 * Return value: a new #BtkImage widget
 **/
BtkWidget*
btk_image_new_from_animation (BdkPixbufAnimation *animation)
{
  BtkImage *image;

  g_return_val_if_fail (BDK_IS_PIXBUF_ANIMATION (animation), NULL);
  
  image = g_object_new (BTK_TYPE_IMAGE, NULL);

  btk_image_set_from_animation (image, animation);

  return BTK_WIDGET (image);
}

/**
 * btk_image_new_from_icon_name:
 * @icon_name: an icon name
 * @size: (type int): a stock icon size
 * 
 * Creates a #BtkImage displaying an icon from the current icon theme.
 * If the icon name isn't known, a "broken image" icon will be
 * displayed instead.  If the current icon theme is changed, the icon
 * will be updated appropriately.
 * 
 * Return value: a new #BtkImage displaying the themed icon
 *
 * Since: 2.6
 **/
BtkWidget*
btk_image_new_from_icon_name (const gchar    *icon_name,
			      BtkIconSize     size)
{
  BtkImage *image;

  image = g_object_new (BTK_TYPE_IMAGE, NULL);

  btk_image_set_from_icon_name (image, icon_name, size);

  return BTK_WIDGET (image);
}

/**
 * btk_image_new_from_gicon:
 * @icon: an icon
 * @size: (type int): a stock icon size
 * 
 * Creates a #BtkImage displaying an icon from the current icon theme.
 * If the icon name isn't known, a "broken image" icon will be
 * displayed instead.  If the current icon theme is changed, the icon
 * will be updated appropriately.
 * 
 * Return value: a new #BtkImage displaying the themed icon
 *
 * Since: 2.14
 **/
BtkWidget*
btk_image_new_from_gicon (GIcon *icon,
			  BtkIconSize     size)
{
  BtkImage *image;

  image = g_object_new (BTK_TYPE_IMAGE, NULL);

  btk_image_set_from_gicon (image, icon, size);

  return BTK_WIDGET (image);
}

/**
 * btk_image_set_from_pixmap:
 * @image: a #BtkImage
 * @pixmap: (allow-none): a #BdkPixmap or %NULL
 * @mask: (allow-none): a #BdkBitmap or %NULL
 *
 * See btk_image_new_from_pixmap() for details.
 **/
void
btk_image_set_from_pixmap (BtkImage  *image,
                           BdkPixmap *pixmap,
                           BdkBitmap *mask)
{
  g_return_if_fail (BTK_IS_IMAGE (image));
  g_return_if_fail (pixmap == NULL ||
                    BDK_IS_PIXMAP (pixmap));
  g_return_if_fail (mask == NULL ||
                    BDK_IS_PIXMAP (mask));

  g_object_freeze_notify (B_OBJECT (image));
  
  if (pixmap)
    g_object_ref (pixmap);

  if (mask)
    g_object_ref (mask);

  btk_image_clear (image);

  image->mask = mask;
  
  if (pixmap)
    {
      int width;
      int height;
      
      image->storage_type = BTK_IMAGE_PIXMAP;

      image->data.pixmap.pixmap = pixmap;

      bdk_drawable_get_size (BDK_DRAWABLE (pixmap), &width, &height);

      btk_image_update_size (image, width, height);
    }

  g_object_notify (B_OBJECT (image), "pixmap");
  g_object_notify (B_OBJECT (image), "mask");
  
  g_object_thaw_notify (B_OBJECT (image));
}

/**
 * btk_image_set_from_image:
 * @image: a #BtkImage
 * @bdk_image: (allow-none): a #BdkImage or %NULL
 * @mask:  (allow-none): a #BdkBitmap or %NULL
 *
 * See btk_image_new_from_image() for details.
 **/
void
btk_image_set_from_image  (BtkImage  *image,
                           BdkImage  *bdk_image,
                           BdkBitmap *mask)
{
  g_return_if_fail (BTK_IS_IMAGE (image));
  g_return_if_fail (bdk_image == NULL ||
                    BDK_IS_IMAGE (bdk_image));
  g_return_if_fail (mask == NULL ||
                    BDK_IS_PIXMAP (mask));

  g_object_freeze_notify (B_OBJECT (image));
  
  if (bdk_image)
    g_object_ref (bdk_image);

  if (mask)
    g_object_ref (mask);

  btk_image_clear (image);

  if (bdk_image)
    {
      image->storage_type = BTK_IMAGE_IMAGE;

      image->data.image.image = bdk_image;
      image->mask = mask;

      btk_image_update_size (image, bdk_image->width, bdk_image->height);
    }
  else
    {
      /* Clean up the mask if bdk_image was NULL */
      if (mask)
        g_object_unref (mask);
    }

  g_object_notify (B_OBJECT (image), "image");
  g_object_notify (B_OBJECT (image), "mask");
  
  g_object_thaw_notify (B_OBJECT (image));
}

/**
 * btk_image_set_from_file:
 * @image: a #BtkImage
 * @filename: (allow-none): a filename or %NULL
 *
 * See btk_image_new_from_file() for details.
 **/
void
btk_image_set_from_file   (BtkImage    *image,
                           const gchar *filename)
{
  BtkImagePrivate *priv = BTK_IMAGE_GET_PRIVATE (image);
  BdkPixbufAnimation *anim;
  
  g_return_if_fail (BTK_IS_IMAGE (image));

  g_object_freeze_notify (B_OBJECT (image));
  
  btk_image_clear (image);

  if (filename == NULL)
    {
      priv->filename = NULL;
      g_object_thaw_notify (B_OBJECT (image));
      return;
    }

  anim = bdk_pixbuf_animation_new_from_file (filename, NULL);

  if (anim == NULL)
    {
      btk_image_set_from_stock (image,
                                BTK_STOCK_MISSING_IMAGE,
                                BTK_ICON_SIZE_BUTTON);
      g_object_thaw_notify (B_OBJECT (image));
      return;
    }

  /* We could just unconditionally set_from_animation,
   * but it's nicer for memory if we toss the animation
   * if it's just a single pixbuf
   */

  if (bdk_pixbuf_animation_is_static_image (anim))
    btk_image_set_from_pixbuf (image,
			       bdk_pixbuf_animation_get_static_image (anim));
  else
    btk_image_set_from_animation (image, anim);

  g_object_unref (anim);

  priv->filename = g_strdup (filename);
  
  g_object_thaw_notify (B_OBJECT (image));
}

/**
 * btk_image_set_from_pixbuf:
 * @image: a #BtkImage
 * @pixbuf: (allow-none): a #BdkPixbuf or %NULL
 *
 * See btk_image_new_from_pixbuf() for details.
 **/
void
btk_image_set_from_pixbuf (BtkImage  *image,
                           BdkPixbuf *pixbuf)
{
  g_return_if_fail (BTK_IS_IMAGE (image));
  g_return_if_fail (pixbuf == NULL ||
                    BDK_IS_PIXBUF (pixbuf));

  g_object_freeze_notify (B_OBJECT (image));
  
  if (pixbuf)
    g_object_ref (pixbuf);

  btk_image_clear (image);

  if (pixbuf != NULL)
    {
      image->storage_type = BTK_IMAGE_PIXBUF;

      image->data.pixbuf.pixbuf = pixbuf;

      btk_image_update_size (image,
                             bdk_pixbuf_get_width (pixbuf),
                             bdk_pixbuf_get_height (pixbuf));
    }

  g_object_notify (B_OBJECT (image), "pixbuf");
  
  g_object_thaw_notify (B_OBJECT (image));
}

/**
 * btk_image_set_from_stock:
 * @image: a #BtkImage
 * @stock_id: a stock icon name
 * @size: (type int): a stock icon size
 *
 * See btk_image_new_from_stock() for details.
 **/
void
btk_image_set_from_stock  (BtkImage       *image,
                           const gchar    *stock_id,
                           BtkIconSize     size)
{
  gchar *new_id;
  
  g_return_if_fail (BTK_IS_IMAGE (image));

  g_object_freeze_notify (B_OBJECT (image));

  /* in case stock_id == image->data.stock.stock_id */
  new_id = g_strdup (stock_id);
  
  btk_image_clear (image);

  if (new_id)
    {
      image->storage_type = BTK_IMAGE_STOCK;
      
      image->data.stock.stock_id = new_id;
      image->icon_size = size;

      /* Size is demand-computed in size request method
       * if we're a stock image, since changing the
       * style impacts the size request
       */
    }

  g_object_notify (B_OBJECT (image), "stock");
  g_object_notify (B_OBJECT (image), "icon-size");
  
  g_object_thaw_notify (B_OBJECT (image));
}

/**
 * btk_image_set_from_icon_set:
 * @image: a #BtkImage
 * @icon_set: a #BtkIconSet
 * @size: (type int): a stock icon size
 *
 * See btk_image_new_from_icon_set() for details.
 **/
void
btk_image_set_from_icon_set  (BtkImage       *image,
                              BtkIconSet     *icon_set,
                              BtkIconSize     size)
{
  g_return_if_fail (BTK_IS_IMAGE (image));

  g_object_freeze_notify (B_OBJECT (image));
  
  if (icon_set)
    btk_icon_set_ref (icon_set);
  
  btk_image_clear (image);

  if (icon_set)
    {      
      image->storage_type = BTK_IMAGE_ICON_SET;
      
      image->data.icon_set.icon_set = icon_set;
      image->icon_size = size;

      /* Size is demand-computed in size request method
       * if we're an icon set
       */
    }
  
  g_object_notify (B_OBJECT (image), "icon-set");
  g_object_notify (B_OBJECT (image), "icon-size");
  
  g_object_thaw_notify (B_OBJECT (image));
}

/**
 * btk_image_set_from_animation:
 * @image: a #BtkImage
 * @animation: the #BdkPixbufAnimation
 * 
 * Causes the #BtkImage to display the given animation (or display
 * nothing, if you set the animation to %NULL).
 **/
void
btk_image_set_from_animation (BtkImage           *image,
                              BdkPixbufAnimation *animation)
{
  g_return_if_fail (BTK_IS_IMAGE (image));
  g_return_if_fail (animation == NULL ||
                    BDK_IS_PIXBUF_ANIMATION (animation));

  g_object_freeze_notify (B_OBJECT (image));
  
  if (animation)
    g_object_ref (animation);

  btk_image_clear (image);

  if (animation != NULL)
    {
      image->storage_type = BTK_IMAGE_ANIMATION;

      image->data.anim.anim = animation;
      image->data.anim.frame_timeout = 0;
      image->data.anim.iter = NULL;
      
      btk_image_update_size (image,
                             bdk_pixbuf_animation_get_width (animation),
                             bdk_pixbuf_animation_get_height (animation));
    }

  g_object_notify (B_OBJECT (image), "pixbuf-animation");
  
  g_object_thaw_notify (B_OBJECT (image));
}

/**
 * btk_image_set_from_icon_name:
 * @image: a #BtkImage
 * @icon_name: an icon name
 * @size: (type int): an icon size
 *
 * See btk_image_new_from_icon_name() for details.
 * 
 * Since: 2.6
 **/
void
btk_image_set_from_icon_name  (BtkImage       *image,
			       const gchar    *icon_name,
			       BtkIconSize     size)
{
  gchar *new_name;
  
  g_return_if_fail (BTK_IS_IMAGE (image));

  g_object_freeze_notify (B_OBJECT (image));

  /* in case icon_name == image->data.name.icon_name */
  new_name = g_strdup (icon_name);
  
  btk_image_clear (image);

  if (new_name)
    {
      image->storage_type = BTK_IMAGE_ICON_NAME;
      
      image->data.name.icon_name = new_name;
      image->icon_size = size;

      /* Size is demand-computed in size request method
       * if we're a icon theme image, since changing the
       * style impacts the size request
       */
    }

  g_object_notify (B_OBJECT (image), "icon-name");
  g_object_notify (B_OBJECT (image), "icon-size");
  
  g_object_thaw_notify (B_OBJECT (image));
}

/**
 * btk_image_set_from_gicon:
 * @image: a #BtkImage
 * @icon: an icon
 * @size: (type int): an icon size
 *
 * See btk_image_new_from_gicon() for details.
 * 
 * Since: 2.14
 **/
void
btk_image_set_from_gicon  (BtkImage       *image,
			   GIcon          *icon,
			   BtkIconSize     size)
{
  g_return_if_fail (BTK_IS_IMAGE (image));

  g_object_freeze_notify (B_OBJECT (image));

  /* in case icon == image->data.gicon.icon */
  if (icon)
    g_object_ref (icon);
  
  btk_image_clear (image);

  if (icon)
    {
      image->storage_type = BTK_IMAGE_GICON;
      
      image->data.gicon.icon = icon;
      image->icon_size = size;

      /* Size is demand-computed in size request method
       * if we're a icon theme image, since changing the
       * style impacts the size request
       */
    }

  g_object_notify (B_OBJECT (image), "gicon");
  g_object_notify (B_OBJECT (image), "icon-size");
  
  g_object_thaw_notify (B_OBJECT (image));
}

/**
 * btk_image_get_storage_type:
 * @image: a #BtkImage
 * 
 * Gets the type of representation being used by the #BtkImage
 * to store image data. If the #BtkImage has no image data,
 * the return value will be %BTK_IMAGE_EMPTY.
 * 
 * Return value: image representation being used
 **/
BtkImageType
btk_image_get_storage_type (BtkImage *image)
{
  g_return_val_if_fail (BTK_IS_IMAGE (image), BTK_IMAGE_EMPTY);

  return image->storage_type;
}

/**
 * btk_image_get_pixmap:
 * @image: a #BtkImage
 * @pixmap: (out) (transfer none) (allow-none): location to store the
 *     pixmap, or %NULL
 * @mask: (out) (transfer none) (allow-none): location to store the
 *     mask, or %NULL
 *
 * Gets the pixmap and mask being displayed by the #BtkImage.
 * The storage type of the image must be %BTK_IMAGE_EMPTY or
 * %BTK_IMAGE_PIXMAP (see btk_image_get_storage_type()).
 * The caller of this function does not own a reference to the
 * returned pixmap and mask.
 **/
void
btk_image_get_pixmap (BtkImage   *image,
                      BdkPixmap **pixmap,
                      BdkBitmap **mask)
{
  g_return_if_fail (BTK_IS_IMAGE (image)); 
  g_return_if_fail (image->storage_type == BTK_IMAGE_PIXMAP ||
                    image->storage_type == BTK_IMAGE_EMPTY);
  
  if (pixmap)
    *pixmap = image->data.pixmap.pixmap;
  
  if (mask)
    *mask = image->mask;
}

/**
 * btk_image_get_image:
 * @image: a #BtkImage
 * @bdk_image: (out) (transfer none) (allow-none): return location for
 *     a #BtkImage, or %NULL
 * @mask: (out) (transfer none) (allow-none): return location for a
 *     #BdkBitmap, or %NULL
 * 
 * Gets the #BdkImage and mask being displayed by the #BtkImage.
 * The storage type of the image must be %BTK_IMAGE_EMPTY or
 * %BTK_IMAGE_IMAGE (see btk_image_get_storage_type()).
 * The caller of this function does not own a reference to the
 * returned image and mask.
 **/
void
btk_image_get_image  (BtkImage   *image,
                      BdkImage  **bdk_image,
                      BdkBitmap **mask)
{
  g_return_if_fail (BTK_IS_IMAGE (image));
  g_return_if_fail (image->storage_type == BTK_IMAGE_IMAGE ||
                    image->storage_type == BTK_IMAGE_EMPTY);

  if (bdk_image)
    *bdk_image = image->data.image.image;
  
  if (mask)
    *mask = image->mask;
}

/**
 * btk_image_get_pixbuf:
 * @image: a #BtkImage
 *
 * Gets the #BdkPixbuf being displayed by the #BtkImage.
 * The storage type of the image must be %BTK_IMAGE_EMPTY or
 * %BTK_IMAGE_PIXBUF (see btk_image_get_storage_type()).
 * The caller of this function does not own a reference to the
 * returned pixbuf.
 * 
 * Return value: (transfer none): the displayed pixbuf, or %NULL if
 * the image is empty
 **/
BdkPixbuf*
btk_image_get_pixbuf (BtkImage *image)
{
  g_return_val_if_fail (BTK_IS_IMAGE (image), NULL);
  g_return_val_if_fail (image->storage_type == BTK_IMAGE_PIXBUF ||
                        image->storage_type == BTK_IMAGE_EMPTY, NULL);

  if (image->storage_type == BTK_IMAGE_EMPTY)
    image->data.pixbuf.pixbuf = NULL;
  
  return image->data.pixbuf.pixbuf;
}

/**
 * btk_image_get_stock:
 * @image: a #BtkImage
 * @stock_id: (out) (transfer none) (allow-none): place to store a
 *     stock icon name, or %NULL
 * @size: (out) (allow-none) (type int): place to store a stock icon
 *     size, or %NULL
 *
 * Gets the stock icon name and size being displayed by the #BtkImage.
 * The storage type of the image must be %BTK_IMAGE_EMPTY or
 * %BTK_IMAGE_STOCK (see btk_image_get_storage_type()).
 * The returned string is owned by the #BtkImage and should not
 * be freed.
 **/
void
btk_image_get_stock  (BtkImage        *image,
                      gchar          **stock_id,
                      BtkIconSize     *size)
{
  g_return_if_fail (BTK_IS_IMAGE (image));
  g_return_if_fail (image->storage_type == BTK_IMAGE_STOCK ||
                    image->storage_type == BTK_IMAGE_EMPTY);

  if (image->storage_type == BTK_IMAGE_EMPTY)
    image->data.stock.stock_id = NULL;
  
  if (stock_id)
    *stock_id = image->data.stock.stock_id;

  if (size)
    *size = image->icon_size;
}

/**
 * btk_image_get_icon_set:
 * @image: a #BtkImage
 * @icon_set: (out) (transfer none) (allow-none): location to store a
 *     #BtkIconSet, or %NULL
 * @size: (out) (allow-none) (type int): location to store a stock
 *     icon size, or %NULL
 *
 * Gets the icon set and size being displayed by the #BtkImage.
 * The storage type of the image must be %BTK_IMAGE_EMPTY or
 * %BTK_IMAGE_ICON_SET (see btk_image_get_storage_type()).
 **/
void
btk_image_get_icon_set  (BtkImage        *image,
                         BtkIconSet     **icon_set,
                         BtkIconSize     *size)
{
  g_return_if_fail (BTK_IS_IMAGE (image));
  g_return_if_fail (image->storage_type == BTK_IMAGE_ICON_SET ||
                    image->storage_type == BTK_IMAGE_EMPTY);
      
  if (icon_set)    
    *icon_set = image->data.icon_set.icon_set;

  if (size)
    *size = image->icon_size;
}

/**
 * btk_image_get_animation:
 * @image: a #BtkImage
 *
 * Gets the #BdkPixbufAnimation being displayed by the #BtkImage.
 * The storage type of the image must be %BTK_IMAGE_EMPTY or
 * %BTK_IMAGE_ANIMATION (see btk_image_get_storage_type()).
 * The caller of this function does not own a reference to the
 * returned animation.
 * 
 * Return value: (transfer none): the displayed animation, or %NULL if
 * the image is empty
 **/
BdkPixbufAnimation*
btk_image_get_animation (BtkImage *image)
{
  g_return_val_if_fail (BTK_IS_IMAGE (image), NULL);
  g_return_val_if_fail (image->storage_type == BTK_IMAGE_ANIMATION ||
                        image->storage_type == BTK_IMAGE_EMPTY,
                        NULL);

  if (image->storage_type == BTK_IMAGE_EMPTY)
    image->data.anim.anim = NULL;
  
  return image->data.anim.anim;
}

/**
 * btk_image_get_icon_name:
 * @image: a #BtkImage
 * @icon_name: (out) (transfer none) (allow-none): place to store an
 *     icon name, or %NULL
 * @size: (out) (allow-none) (type int): place to store an icon size,
 *     or %NULL
 *
 * Gets the icon name and size being displayed by the #BtkImage.
 * The storage type of the image must be %BTK_IMAGE_EMPTY or
 * %BTK_IMAGE_ICON_NAME (see btk_image_get_storage_type()).
 * The returned string is owned by the #BtkImage and should not
 * be freed.
 * 
 * Since: 2.6
 **/
void
btk_image_get_icon_name  (BtkImage              *image,
			  const gchar          **icon_name,
			  BtkIconSize           *size)
{
  g_return_if_fail (BTK_IS_IMAGE (image));
  g_return_if_fail (image->storage_type == BTK_IMAGE_ICON_NAME ||
                    image->storage_type == BTK_IMAGE_EMPTY);

  if (image->storage_type == BTK_IMAGE_EMPTY)
    image->data.name.icon_name = NULL;
  
  if (icon_name)
    *icon_name = image->data.name.icon_name;

  if (size)
    *size = image->icon_size;
}

/**
 * btk_image_get_gicon:
 * @image: a #BtkImage
 * @gicon: (out) (transfer none) (allow-none): place to store a
 *     #GIcon, or %NULL
 * @size: (out) (allow-none) (type int): place to store an icon size,
 *     or %NULL
 *
 * Gets the #GIcon and size being displayed by the #BtkImage.
 * The storage type of the image must be %BTK_IMAGE_EMPTY or
 * %BTK_IMAGE_GICON (see btk_image_get_storage_type()).
 * The caller of this function does not own a reference to the
 * returned #GIcon.
 * 
 * Since: 2.14
 **/
void
btk_image_get_gicon (BtkImage     *image,
		     GIcon       **gicon,
		     BtkIconSize  *size)
{
  g_return_if_fail (BTK_IS_IMAGE (image));
  g_return_if_fail (image->storage_type == BTK_IMAGE_GICON ||
                    image->storage_type == BTK_IMAGE_EMPTY);

  if (image->storage_type == BTK_IMAGE_EMPTY)
    image->data.gicon.icon = NULL;
  
  if (gicon)
    *gicon = image->data.gicon.icon;

  if (size)
    *size = image->icon_size;
}

/**
 * btk_image_new:
 * 
 * Creates a new empty #BtkImage widget.
 * 
 * Return value: a newly created #BtkImage widget. 
 **/
BtkWidget*
btk_image_new (void)
{
  return g_object_new (BTK_TYPE_IMAGE, NULL);
}

/**
 * btk_image_set:
 * @image: a #BtkImage
 * @val: a #BdkImage
 * @mask: a #BdkBitmap that indicates which parts of the image should be transparent.
 *
 * Sets the #BtkImage.
 *
 * Deprecated: 2.0: Use btk_image_set_from_image() instead.
 */
void
btk_image_set (BtkImage  *image,
	       BdkImage  *val,
	       BdkBitmap *mask)
{
  g_return_if_fail (BTK_IS_IMAGE (image));

  btk_image_set_from_image (image, val, mask);
}

/**
 * btk_image_get:
 * @image: a #BtkImage
 * @val: return location for a #BdkImage
 * @mask: a #BdkBitmap that indicates which parts of the image should be transparent.
 *
 * Gets the #BtkImage.
 *
 * Deprecated: 2.0: Use btk_image_get_image() instead.
 */
void
btk_image_get (BtkImage   *image,
	       BdkImage  **val,
	       BdkBitmap **mask)
{
  g_return_if_fail (BTK_IS_IMAGE (image));

  btk_image_get_image (image, val, mask);
}

static void
btk_image_reset_anim_iter (BtkImage *image)
{
  if (image->storage_type == BTK_IMAGE_ANIMATION)
    {
      /* Reset the animation */
      
      if (image->data.anim.frame_timeout)
        {
          g_source_remove (image->data.anim.frame_timeout);
          image->data.anim.frame_timeout = 0;
        }

      if (image->data.anim.iter)
        {
          g_object_unref (image->data.anim.iter);
          image->data.anim.iter = NULL;
        }
    }
}

static void
btk_image_unmap (BtkWidget *widget)
{
  btk_image_reset_anim_iter (BTK_IMAGE (widget));

  BTK_WIDGET_CLASS (btk_image_parent_class)->unmap (widget);
}

static void
btk_image_unrealize (BtkWidget *widget)
{
  btk_image_reset_anim_iter (BTK_IMAGE (widget));

  BTK_WIDGET_CLASS (btk_image_parent_class)->unrealize (widget);
}

static gint
animation_timeout (gpointer data)
{
  BtkImage *image;
  int delay;

  image = BTK_IMAGE (data);
  
  image->data.anim.frame_timeout = 0;

  bdk_pixbuf_animation_iter_advance (image->data.anim.iter, NULL);

  delay = bdk_pixbuf_animation_iter_get_delay_time (image->data.anim.iter);
  if (delay >= 0)
    {
      image->data.anim.frame_timeout =
        bdk_threads_add_timeout (delay, animation_timeout, image);

      btk_widget_queue_draw (BTK_WIDGET (image));

      if (btk_widget_is_drawable (BTK_WIDGET (image)))
        bdk_window_process_updates (BTK_WIDGET (image)->window, TRUE);
    }

  return FALSE;
}

static void
icon_theme_changed (BtkImage *image)
{
  if (image->storage_type == BTK_IMAGE_ICON_NAME) 
    {
      if (image->data.name.pixbuf)
	g_object_unref (image->data.name.pixbuf);
      image->data.name.pixbuf = NULL;

      btk_widget_queue_draw (BTK_WIDGET (image));
    }
  if (image->storage_type == BTK_IMAGE_GICON) 
    {
      if (image->data.gicon.pixbuf)
	g_object_unref (image->data.gicon.pixbuf);
      image->data.gicon.pixbuf = NULL;

      btk_widget_queue_draw (BTK_WIDGET (image));
    }
}

static void
ensure_pixbuf_for_icon_name (BtkImage *image)
{
  BtkImagePrivate *priv;
  BdkScreen *screen;
  BtkIconTheme *icon_theme;
  BtkSettings *settings;
  gint width, height;
  gint *sizes, *s, dist;
  BtkIconLookupFlags flags;
  GError *error = NULL;

  g_return_if_fail (image->storage_type == BTK_IMAGE_ICON_NAME);

  priv = BTK_IMAGE_GET_PRIVATE (image);
  screen = btk_widget_get_screen (BTK_WIDGET (image));
  icon_theme = btk_icon_theme_get_for_screen (screen);
  settings = btk_settings_get_for_screen (screen);
  flags = BTK_ICON_LOOKUP_USE_BUILTIN;
  if (image->data.name.pixbuf == NULL)
    {
      if (priv->pixel_size != -1)
	{
	  width = height = priv->pixel_size;
          flags |= BTK_ICON_LOOKUP_FORCE_SIZE;
	}
      else if (!btk_icon_size_lookup_for_settings (settings,
						   image->icon_size,
						   &width, &height))
	{
	  if (image->icon_size == -1)
	    {
	      /* Find an available size close to 48 */
	      sizes = btk_icon_theme_get_icon_sizes (icon_theme, image->data.name.icon_name);
	      dist = 100;
	      width = height = 48;
	      for (s = sizes; *s; s++)
		{
		  if (*s == -1)
		    {
		      width = height = 48;
		      break;
		    }
		  if (*s < 48)
		    {
		      if (48 - *s < dist)
			{
			  width = height = *s;
			  dist = 48 - *s;
			}
		    }
		  else
		    {
		      if (*s - 48 < dist)
			{
			  width = height = *s;
			  dist = *s - 48;
			}
		    }
		}
	      g_free (sizes);
	    }
	  else
	    {
	      g_warning ("Invalid icon size %d\n", image->icon_size);
	      width = height = 24;
	    }
	}
      image->data.name.pixbuf =
	btk_icon_theme_load_icon (icon_theme,
				  image->data.name.icon_name,
				  MIN (width, height), flags, &error);
      if (image->data.name.pixbuf == NULL)
	{
	  g_error_free (error);
	  image->data.name.pixbuf =
	    btk_widget_render_icon (BTK_WIDGET (image),
				    BTK_STOCK_MISSING_IMAGE,
				    image->icon_size,
				    NULL);
	}
    }
}

static void
ensure_pixbuf_for_gicon (BtkImage *image)
{
  BtkImagePrivate *priv;
  BdkScreen *screen;
  BtkIconTheme *icon_theme;
  BtkSettings *settings;
  gint width, height;
  BtkIconInfo *info;
  BtkIconLookupFlags flags;

  g_return_if_fail (image->storage_type == BTK_IMAGE_GICON);

  priv = BTK_IMAGE_GET_PRIVATE (image);
  screen = btk_widget_get_screen (BTK_WIDGET (image));
  icon_theme = btk_icon_theme_get_for_screen (screen);
  settings = btk_settings_get_for_screen (screen);
  flags = BTK_ICON_LOOKUP_USE_BUILTIN;
  if (image->data.gicon.pixbuf == NULL)
    {
      if (priv->pixel_size != -1)
	{
	  width = height = priv->pixel_size;
          flags |= BTK_ICON_LOOKUP_FORCE_SIZE;
	}
      else if (!btk_icon_size_lookup_for_settings (settings,
						   image->icon_size,
						   &width, &height))
	{
	  if (image->icon_size == -1)
	    width = height = 48;
	  else
	    {
	      g_warning ("Invalid icon size %d\n", image->icon_size);
	      width = height = 24;
	    }
	}

      info = btk_icon_theme_lookup_by_gicon (icon_theme,
					     image->data.gicon.icon,
					     MIN (width, height), flags);
      if (info)
        {
          image->data.gicon.pixbuf = btk_icon_info_load_icon (info, NULL);
          btk_icon_info_free (info);
        }

      if (image->data.gicon.pixbuf == NULL)
	{
	  image->data.gicon.pixbuf =
	    btk_widget_render_icon (BTK_WIDGET (image),
				    BTK_STOCK_MISSING_IMAGE,
				    image->icon_size,
				    NULL);
	}
    }
}


/*
 * Like bdk_rectangle_intersect (dest, src, dest), but make 
 * sure that the origin of dest is moved by an "even" offset. 
 * If necessary grow the intersection by one row or column 
 * to achieve this.
 *
 * This is necessary since we can't pass alignment information
 * for the pixelation pattern down to bdk_pixbuf_saturate_and_pixelate(), 
 * thus we have to makesure that the subimages are properly aligned.
 */
static gboolean
rectangle_intersect_even (BdkRectangle *src, 
			  BdkRectangle *dest)
{
  gboolean isect;
  gint x, y;

  x = dest->x;
  y = dest->y;
  isect = bdk_rectangle_intersect (dest, src, dest);

  if ((dest->x - x + dest->y - y) % 2 != 0)
    {
      if (dest->x > x)
	{
	  dest->x--;
	  dest->width++;
	}
      else
	{
	  dest->y--;
	  dest->height++;
	}
    }
  
  return isect;
}

static gint
btk_image_expose (BtkWidget      *widget,
		  BdkEventExpose *event)
{
  g_return_val_if_fail (BTK_IS_IMAGE (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);
  
  if (btk_widget_get_mapped (widget) &&
      BTK_IMAGE (widget)->storage_type != BTK_IMAGE_EMPTY)
    {
      BtkImage *image;
      BtkMisc *misc;
      BtkImagePrivate *priv;
      BdkRectangle area, image_bound;
      gfloat xalign;
      gint x, y, mask_x, mask_y;
      BdkBitmap *mask;
      BdkPixbuf *pixbuf;
      gboolean needs_state_transform;

      image = BTK_IMAGE (widget);
      misc = BTK_MISC (widget);
      priv = BTK_IMAGE_GET_PRIVATE (image);

      area = event->area;

      /* For stock items and icon sets, we lazily calculate
       * the size; we might get here between a queue_resize()
       * and size_request() if something explicitely forces
       * a redraw.
       */
      if (priv->need_calc_size)
	btk_image_calc_size (image);
      
      if (!bdk_rectangle_intersect (&area, &widget->allocation, &area))
	return FALSE;

      if (btk_widget_get_direction (widget) == BTK_TEXT_DIR_LTR)
	xalign = misc->xalign;
      else
	xalign = 1.0 - misc->xalign;

      x = floor (widget->allocation.x + misc->xpad
		 + ((widget->allocation.width - widget->requisition.width) * xalign));
      y = floor (widget->allocation.y + misc->ypad 
		 + ((widget->allocation.height - widget->requisition.height) * misc->yalign));
      mask_x = x;
      mask_y = y;
      
      image_bound.x = x;
      image_bound.y = y;      
      image_bound.width = 0;
      image_bound.height = 0;      

      mask = NULL;
      pixbuf = NULL;
      needs_state_transform = btk_widget_get_state (widget) != BTK_STATE_NORMAL;
      
      switch (image->storage_type)
        {
        case BTK_IMAGE_PIXMAP:
          mask = image->mask;
          bdk_drawable_get_size (image->data.pixmap.pixmap,
                                 &image_bound.width,
                                 &image_bound.height);
	  if (rectangle_intersect_even (&area, &image_bound) &&
	      needs_state_transform)
            {
              pixbuf = bdk_pixbuf_get_from_drawable (NULL,
                                                     image->data.pixmap.pixmap,
                                                     btk_widget_get_colormap (widget),
                                                     image_bound.x - x, image_bound.y - y,
						     0, 0,
                                                     image_bound.width,
                                                     image_bound.height);

	      x = image_bound.x;
	      y = image_bound.y;
            }
	  
          break;

        case BTK_IMAGE_IMAGE:
          mask = image->mask;
          image_bound.width = image->data.image.image->width;
          image_bound.height = image->data.image.image->height;

	  if (rectangle_intersect_even (&area, &image_bound) &&
	      needs_state_transform)
            {
              pixbuf = bdk_pixbuf_get_from_image (NULL,
                                                  image->data.image.image,
                                                  btk_widget_get_colormap (widget),
						  image_bound.x - x, image_bound.y - y,
                                                  0, 0,
                                                  image_bound.width,
                                                  image_bound.height);

	      x = image_bound.x;
	      y = image_bound.y;
            }
          break;

        case BTK_IMAGE_PIXBUF:
          image_bound.width = bdk_pixbuf_get_width (image->data.pixbuf.pixbuf);
          image_bound.height = bdk_pixbuf_get_height (image->data.pixbuf.pixbuf);            

	  if (rectangle_intersect_even (&area, &image_bound) &&
	      needs_state_transform)
	    {
	      pixbuf = bdk_pixbuf_new_subpixbuf (image->data.pixbuf.pixbuf,
						 image_bound.x - x, image_bound.y - y,
						 image_bound.width, image_bound.height);

	      x = image_bound.x;
	      y = image_bound.y;
	    }
	  else
	    {
	      pixbuf = image->data.pixbuf.pixbuf;
	      g_object_ref (pixbuf);
	    }
          break;

        case BTK_IMAGE_STOCK:
          pixbuf = btk_widget_render_icon (widget,
                                           image->data.stock.stock_id,
                                           image->icon_size,
                                           NULL);
          if (pixbuf)
            {              
              image_bound.width = bdk_pixbuf_get_width (pixbuf);
              image_bound.height = bdk_pixbuf_get_height (pixbuf);
            }

          /* already done */
          needs_state_transform = FALSE;
          break;

        case BTK_IMAGE_ICON_SET:
          pixbuf =
            btk_icon_set_render_icon (image->data.icon_set.icon_set,
                                      widget->style,
                                      btk_widget_get_direction (widget),
                                      btk_widget_get_state (widget),
                                      image->icon_size,
                                      widget,
                                      NULL);

          if (pixbuf)
            {
              image_bound.width = bdk_pixbuf_get_width (pixbuf);
              image_bound.height = bdk_pixbuf_get_height (pixbuf);
            }

          /* already done */
          needs_state_transform = FALSE;
          break;

        case BTK_IMAGE_ANIMATION:
          {
            if (image->data.anim.iter == NULL)
              {
                image->data.anim.iter = bdk_pixbuf_animation_get_iter (image->data.anim.anim, NULL);
                
                if (bdk_pixbuf_animation_iter_get_delay_time (image->data.anim.iter) >= 0)
                  image->data.anim.frame_timeout =
                    bdk_threads_add_timeout (bdk_pixbuf_animation_iter_get_delay_time (image->data.anim.iter),
                                   animation_timeout,
                                   image);
              }

            image_bound.width = bdk_pixbuf_animation_get_width (image->data.anim.anim);
            image_bound.height = bdk_pixbuf_animation_get_height (image->data.anim.anim);
                  
            /* don't advance the anim iter here, or we could get frame changes between two
             * exposes of different areas.
             */
            
            pixbuf = bdk_pixbuf_animation_iter_get_pixbuf (image->data.anim.iter);
            g_object_ref (pixbuf);
          }
          break;

	case BTK_IMAGE_ICON_NAME:
	  ensure_pixbuf_for_icon_name (image);
	  pixbuf = image->data.name.pixbuf;
	  if (pixbuf)
	    {
	      g_object_ref (pixbuf);
	      image_bound.width = bdk_pixbuf_get_width (pixbuf);
	      image_bound.height = bdk_pixbuf_get_height (pixbuf);
	    }
	  break;

	case BTK_IMAGE_GICON:
	  ensure_pixbuf_for_gicon (image);
	  pixbuf = image->data.gicon.pixbuf;
	  if (pixbuf)
	    {
	      g_object_ref (pixbuf);
	      image_bound.width = bdk_pixbuf_get_width (pixbuf);
	      image_bound.height = bdk_pixbuf_get_height (pixbuf);
	    }
	  break;
	  
        case BTK_IMAGE_EMPTY:
          g_assert_not_reached ();
          break;
        }

      if (mask)
	{
	  bdk_gc_set_clip_mask (widget->style->black_gc, mask);
	  bdk_gc_set_clip_origin (widget->style->black_gc, mask_x, mask_y);
	}

      if (rectangle_intersect_even (&area, &image_bound))
        {
          if (pixbuf)
            {
              if (needs_state_transform)
                {
                  BtkIconSource *source;
                  BdkPixbuf *rendered;

                  source = btk_icon_source_new ();
                  btk_icon_source_set_pixbuf (source, pixbuf);
                  /* The size here is arbitrary; since size isn't
                   * wildcarded in the souce, it isn't supposed to be
                   * scaled by the engine function
                   */
                  btk_icon_source_set_size (source,
                                            BTK_ICON_SIZE_SMALL_TOOLBAR);
                  btk_icon_source_set_size_wildcarded (source, FALSE);
                  
                  rendered = btk_style_render_icon (widget->style,
                                                    source,
                                                    btk_widget_get_direction (widget),
                                                    btk_widget_get_state (widget),
                                                    /* arbitrary */
                                                    (BtkIconSize)-1,
                                                    widget,
                                                    "btk-image");

                  btk_icon_source_free (source);

                  g_object_unref (pixbuf);
                  pixbuf = rendered;
                }

              if (pixbuf)
                {
                  bdk_draw_pixbuf (widget->window,
				   widget->style->black_gc,
				   pixbuf,
				   image_bound.x - x,
				   image_bound.y - y,
				   image_bound.x,
				   image_bound.y,
				   image_bound.width,
				   image_bound.height,
				   BDK_RGB_DITHER_NORMAL,
				   0, 0);
                }
            }
          else
            {
              switch (image->storage_type)
                {
                case BTK_IMAGE_PIXMAP:
                  bdk_draw_drawable (widget->window,
                                     widget->style->black_gc,
                                     image->data.pixmap.pixmap,
                                     image_bound.x - x, image_bound.y - y,
                                     image_bound.x, image_bound.y,
                                     image_bound.width, image_bound.height);
                  break;
              
                case BTK_IMAGE_IMAGE:
                  bdk_draw_image (widget->window,
                                  widget->style->black_gc,
                                  image->data.image.image,
                                  image_bound.x - x, image_bound.y - y,
                                  image_bound.x, image_bound.y,
                                  image_bound.width, image_bound.height);
                  break;

                case BTK_IMAGE_PIXBUF:
                case BTK_IMAGE_STOCK:
                case BTK_IMAGE_ICON_SET:
                case BTK_IMAGE_ANIMATION:
		case BTK_IMAGE_ICON_NAME:
                case BTK_IMAGE_EMPTY:
		case BTK_IMAGE_GICON:
                  g_assert_not_reached ();
                  break;
                }
            }
        } /* if rectangle intersects */      

      if (mask)
        {
          bdk_gc_set_clip_mask (widget->style->black_gc, NULL);
          bdk_gc_set_clip_origin (widget->style->black_gc, 0, 0);
        }
      
      if (pixbuf)
	g_object_unref (pixbuf);

    } /* if widget is drawable */

  return FALSE;
}

static void
btk_image_reset (BtkImage *image)
{
  BtkImagePrivate *priv;

  priv = BTK_IMAGE_GET_PRIVATE (image);

  g_object_freeze_notify (B_OBJECT (image));
  
  if (image->storage_type != BTK_IMAGE_EMPTY)
    g_object_notify (B_OBJECT (image), "storage-type");

  if (image->mask)
    {
      g_object_unref (image->mask);
      image->mask = NULL;
      g_object_notify (B_OBJECT (image), "mask");
    }

  if (image->icon_size != DEFAULT_ICON_SIZE)
    {
      image->icon_size = DEFAULT_ICON_SIZE;
      g_object_notify (B_OBJECT (image), "icon-size");
    }
  
  switch (image->storage_type)
    {
    case BTK_IMAGE_PIXMAP:

      if (image->data.pixmap.pixmap)
        g_object_unref (image->data.pixmap.pixmap);
      image->data.pixmap.pixmap = NULL;
      
      g_object_notify (B_OBJECT (image), "pixmap");
      
      break;

    case BTK_IMAGE_IMAGE:

      if (image->data.image.image)
        g_object_unref (image->data.image.image);
      image->data.image.image = NULL;
      
      g_object_notify (B_OBJECT (image), "image");
      
      break;

    case BTK_IMAGE_PIXBUF:

      if (image->data.pixbuf.pixbuf)
        g_object_unref (image->data.pixbuf.pixbuf);

      g_object_notify (B_OBJECT (image), "pixbuf");
      
      break;

    case BTK_IMAGE_STOCK:

      g_free (image->data.stock.stock_id);

      image->data.stock.stock_id = NULL;
      
      g_object_notify (B_OBJECT (image), "stock");      
      break;

    case BTK_IMAGE_ICON_SET:
      if (image->data.icon_set.icon_set)
        btk_icon_set_unref (image->data.icon_set.icon_set);
      image->data.icon_set.icon_set = NULL;
      
      g_object_notify (B_OBJECT (image), "icon-set");      
      break;

    case BTK_IMAGE_ANIMATION:
      btk_image_reset_anim_iter (image);
      
      if (image->data.anim.anim)
        g_object_unref (image->data.anim.anim);
      image->data.anim.anim = NULL;
      
      g_object_notify (B_OBJECT (image), "pixbuf-animation");
      
      break;

    case BTK_IMAGE_ICON_NAME:
      g_free (image->data.name.icon_name);
      image->data.name.icon_name = NULL;
      if (image->data.name.pixbuf)
	g_object_unref (image->data.name.pixbuf);
      image->data.name.pixbuf = NULL;

      g_object_notify (B_OBJECT (image), "icon-name");

      break;
      
    case BTK_IMAGE_GICON:
      if (image->data.gicon.icon)
	g_object_unref (image->data.gicon.icon);
      image->data.gicon.icon = NULL;
      if (image->data.gicon.pixbuf)
	g_object_unref (image->data.gicon.pixbuf);
      image->data.gicon.pixbuf = NULL;

      g_object_notify (B_OBJECT (image), "gicon");

      break;
      
    case BTK_IMAGE_EMPTY:
    default:
      break;
      
    }

  if (priv->filename)
    {
      g_free (priv->filename);
      priv->filename = NULL;
      g_object_notify (B_OBJECT (image), "file");
    }

  image->storage_type = BTK_IMAGE_EMPTY;

  memset (&image->data, '\0', sizeof (image->data));

  g_object_thaw_notify (B_OBJECT (image));
}

/**
 * btk_image_clear:
 * @image: a #BtkImage
 *
 * Resets the image to be empty.
 *
 * Since: 2.8
 */
void
btk_image_clear (BtkImage *image)
{
  BtkImagePrivate *priv;

  priv = BTK_IMAGE_GET_PRIVATE (image);

  priv->need_calc_size = 1;

  btk_image_reset (image);
  btk_image_update_size (image, 0, 0);
}

static void
btk_image_calc_size (BtkImage *image)
{
  BtkWidget *widget = BTK_WIDGET (image);
  BdkPixbuf *pixbuf = NULL;
  BtkImagePrivate *priv;

  priv = BTK_IMAGE_GET_PRIVATE (image);

  priv->need_calc_size = 0;

  /* We update stock/icon set on every size request, because
   * the theme could have affected the size; for other kinds of
   * image, we just update the requisition when the image data
   * is set.
   */
  switch (image->storage_type)
    {
    case BTK_IMAGE_STOCK:
      pixbuf = btk_widget_render_icon (widget,
				       image->data.stock.stock_id,
                                       image->icon_size,
                                       NULL);
      break;
      
    case BTK_IMAGE_ICON_SET:
      pixbuf = btk_icon_set_render_icon (image->data.icon_set.icon_set,
                                         widget->style,
                                         btk_widget_get_direction (widget),
                                         btk_widget_get_state (widget),
                                         image->icon_size,
                                         widget,
                                         NULL);
      break;
    case BTK_IMAGE_ICON_NAME:
      ensure_pixbuf_for_icon_name (image);
      pixbuf = image->data.name.pixbuf;
      if (pixbuf) g_object_ref (pixbuf);
      break;
    case BTK_IMAGE_GICON:
      ensure_pixbuf_for_gicon (image);
      pixbuf = image->data.gicon.pixbuf;
      if (pixbuf)
	g_object_ref (pixbuf);
      break;
    default:
      break;
    }

  if (pixbuf)
    {
      widget->requisition.width = bdk_pixbuf_get_width (pixbuf) + BTK_MISC (image)->xpad * 2;
      widget->requisition.height = bdk_pixbuf_get_height (pixbuf) + BTK_MISC (image)->ypad * 2;

      g_object_unref (pixbuf);
    }
}

static void
btk_image_size_request (BtkWidget      *widget,
                        BtkRequisition *requisition)
{
  BtkImage *image;
  
  image = BTK_IMAGE (widget);

  btk_image_calc_size (image);

  /* Chain up to default that simply reads current requisition */
  BTK_WIDGET_CLASS (btk_image_parent_class)->size_request (widget, requisition);
}

static void
btk_image_style_set (BtkWidget      *widget,
		     BtkStyle       *prev_style)
{
  BtkImage *image;

  image = BTK_IMAGE (widget);

  BTK_WIDGET_CLASS (btk_image_parent_class)->style_set (widget, prev_style);

  icon_theme_changed (image);
}

static void
btk_image_screen_changed (BtkWidget *widget,
			  BdkScreen *prev_screen)
{
  BtkImage *image;

  image = BTK_IMAGE (widget);

  if (BTK_WIDGET_CLASS (btk_image_parent_class)->screen_changed)
    BTK_WIDGET_CLASS (btk_image_parent_class)->screen_changed (widget, prev_screen);

  icon_theme_changed (image);
}


static void
btk_image_update_size (BtkImage *image,
                       gint      image_width,
                       gint      image_height)
{
  BtkWidget *widget = BTK_WIDGET (image);

  widget->requisition.width = image_width + BTK_MISC (image)->xpad * 2;
  widget->requisition.height = image_height + BTK_MISC (image)->ypad * 2;

  if (btk_widget_get_visible (widget))
    btk_widget_queue_resize (widget);
}


/**
 * btk_image_set_pixel_size:
 * @image: a #BtkImage
 * @pixel_size: the new pixel size
 * 
 * Sets the pixel size to use for named icons. If the pixel size is set
 * to a value != -1, it is used instead of the icon size set by
 * btk_image_set_from_icon_name().
 *
 * Since: 2.6
 */
void 
btk_image_set_pixel_size (BtkImage *image,
			  gint      pixel_size)
{
  BtkImagePrivate *priv;

  g_return_if_fail (BTK_IS_IMAGE (image));
  
  priv = BTK_IMAGE_GET_PRIVATE (image);

  if (priv->pixel_size != pixel_size)
    {
      priv->pixel_size = pixel_size;
      
      if (image->storage_type == BTK_IMAGE_ICON_NAME)
	{
	  if (image->data.name.pixbuf)
	    {
	      g_object_unref (image->data.name.pixbuf);
	      image->data.name.pixbuf = NULL;
	    }
	  
	  btk_image_update_size (image, pixel_size, pixel_size);
	}
      
      if (image->storage_type == BTK_IMAGE_GICON)
	{
	  if (image->data.gicon.pixbuf)
	    {
	      g_object_unref (image->data.gicon.pixbuf);
	      image->data.gicon.pixbuf = NULL;
	    }
	  
	  btk_image_update_size (image, pixel_size, pixel_size);
	}
      
      g_object_notify (B_OBJECT (image), "pixel-size");
    }
}

/**
 * btk_image_get_pixel_size:
 * @image: a #BtkImage
 * 
 * Gets the pixel size used for named icons.
 *
 * Returns: the pixel size used for named icons.
 *
 * Since: 2.6
 */
gint
btk_image_get_pixel_size (BtkImage *image)
{
  BtkImagePrivate *priv;

  g_return_val_if_fail (BTK_IS_IMAGE (image), -1);
  
  priv = BTK_IMAGE_GET_PRIVATE (image);

  return priv->pixel_size;
}

#if defined (G_OS_WIN32) && !defined (_WIN64)

#undef btk_image_new_from_file

BtkWidget*
btk_image_new_from_file   (const gchar *filename)
{
  gchar *utf8_filename = g_locale_to_utf8 (filename, -1, NULL, NULL, NULL);
  BtkWidget *retval;

  retval = btk_image_new_from_file_utf8 (utf8_filename);

  g_free (utf8_filename);

  return retval;
}

#undef btk_image_set_from_file

void
btk_image_set_from_file   (BtkImage    *image,
                           const gchar *filename)
{
  gchar *utf8_filename = g_locale_to_utf8 (filename, -1, NULL, NULL, NULL);

  btk_image_set_from_file_utf8 (image, utf8_filename);

  g_free (utf8_filename);
}

#endif

#define __BTK_IMAGE_C__
#include "btkaliasdef.c"
