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

#ifndef __BTK_IMAGE_H__
#define __BTK_IMAGE_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <bunnyio/bunnyio.h>
#include <btk/btkmisc.h>


B_BEGIN_DECLS

#define BTK_TYPE_IMAGE                  (btk_image_get_type ())
#define BTK_IMAGE(obj)                  (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_IMAGE, BtkImage))
#define BTK_IMAGE_CLASS(klass)          (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_IMAGE, BtkImageClass))
#define BTK_IS_IMAGE(obj)               (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_IMAGE))
#define BTK_IS_IMAGE_CLASS(klass)       (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_IMAGE))
#define BTK_IMAGE_GET_CLASS(obj)        (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_IMAGE, BtkImageClass))


typedef struct _BtkImage       BtkImage;
typedef struct _BtkImageClass  BtkImageClass;

typedef struct _BtkImagePixmapData  BtkImagePixmapData;
typedef struct _BtkImageImageData   BtkImageImageData;
typedef struct _BtkImagePixbufData  BtkImagePixbufData;
typedef struct _BtkImageStockData   BtkImageStockData;
typedef struct _BtkImageIconSetData BtkImageIconSetData;
typedef struct _BtkImageAnimationData BtkImageAnimationData;
typedef struct _BtkImageIconNameData  BtkImageIconNameData;
typedef struct _BtkImageGIconData     BtkImageGIconData;

struct _BtkImagePixmapData
{
  BdkPixmap *pixmap;
};

struct _BtkImageImageData
{
  BdkImage *image;
};

struct _BtkImagePixbufData
{
  BdkPixbuf *pixbuf;
};

struct _BtkImageStockData
{
  gchar *stock_id;
};

struct _BtkImageIconSetData
{
  BtkIconSet *icon_set;
};

struct _BtkImageAnimationData
{
  BdkPixbufAnimation *anim;
  BdkPixbufAnimationIter *iter;
  guint frame_timeout;
};

struct _BtkImageIconNameData
{
  gchar *icon_name;
  BdkPixbuf *pixbuf;
  guint theme_change_id;
};

struct _BtkImageGIconData
{
  GIcon *icon;
  BdkPixbuf *pixbuf;
  guint theme_change_id;
};

/**
 * BtkImageType:
 * @BTK_IMAGE_EMPTY: there is no image displayed by the widget
 * @BTK_IMAGE_PIXMAP: the widget contains a #BdkPixmap
 * @BTK_IMAGE_IMAGE: the widget contains a #BdkImage
 * @BTK_IMAGE_PIXBUF: the widget contains a #BdkPixbuf
 * @BTK_IMAGE_STOCK: the widget contains a stock icon name (see <xref linkend="btk-Stock-Items"/>)
 * @BTK_IMAGE_ICON_SET: the widget contains a #BtkIconSet
 * @BTK_IMAGE_ANIMATION: the widget contains a #BdkPixbufAnimation
 * @BTK_IMAGE_ICON_NAME: the widget contains a named icon.
 *  This image type was added in BTK+ 2.6
 * @BTK_IMAGE_GICON: the widget contains a #GIcon.
 *  This image type was added in BTK+ 2.14
 *
 * Describes the image data representation used by a #BtkImage. If you
 * want to get the image from the widget, you can only get the
 * currently-stored representation. e.g.  if the
 * btk_image_get_storage_type() returns #BTK_IMAGE_PIXBUF, then you can
 * call btk_image_get_pixbuf() but not btk_image_get_stock().  For empty
 * images, you can request any storage type (call any of the "get"
 * functions), but they will all return %NULL values.
 */
typedef enum
{
  BTK_IMAGE_EMPTY,
  BTK_IMAGE_PIXMAP,
  BTK_IMAGE_IMAGE,
  BTK_IMAGE_PIXBUF,
  BTK_IMAGE_STOCK,
  BTK_IMAGE_ICON_SET,
  BTK_IMAGE_ANIMATION,
  BTK_IMAGE_ICON_NAME,
  BTK_IMAGE_GICON
} BtkImageType;

/**
 * BtkImage:
 *
 * This struct contain private data only and should be accessed by the functions
 * below.
 */
struct _BtkImage
{
  BtkMisc misc;

  BtkImageType GSEAL (storage_type);
  
  union
  {
    BtkImagePixmapData pixmap;
    BtkImageImageData image;
    BtkImagePixbufData pixbuf;
    BtkImageStockData stock;
    BtkImageIconSetData icon_set;
    BtkImageAnimationData anim;
    BtkImageIconNameData name;
    BtkImageGIconData gicon;
  } GSEAL (data);

  /* Only used with BTK_IMAGE_PIXMAP, BTK_IMAGE_IMAGE */
  BdkBitmap *GSEAL (mask);

  /* Only used with BTK_IMAGE_STOCK, BTK_IMAGE_ICON_SET, BTK_IMAGE_ICON_NAME */
  BtkIconSize GSEAL (icon_size);
};

struct _BtkImageClass
{
  BtkMiscClass parent_class;

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};

#ifdef G_OS_WIN32
/* Reserve old names for DLL ABI backward compatibility */
#define btk_image_new_from_file btk_image_new_from_file_utf8
#define btk_image_set_from_file btk_image_set_from_file_utf8
#endif

GType      btk_image_get_type (void) B_GNUC_CONST;

BtkWidget* btk_image_new                (void);
BtkWidget* btk_image_new_from_pixmap    (BdkPixmap       *pixmap,
                                         BdkBitmap       *mask);
BtkWidget* btk_image_new_from_image     (BdkImage        *image,
                                         BdkBitmap       *mask);
BtkWidget* btk_image_new_from_file      (const gchar     *filename);
BtkWidget* btk_image_new_from_pixbuf    (BdkPixbuf       *pixbuf);
BtkWidget* btk_image_new_from_stock     (const gchar     *stock_id,
                                         BtkIconSize      size);
BtkWidget* btk_image_new_from_icon_set  (BtkIconSet      *icon_set,
                                         BtkIconSize      size);
BtkWidget* btk_image_new_from_animation (BdkPixbufAnimation *animation);
BtkWidget* btk_image_new_from_icon_name (const gchar     *icon_name,
					 BtkIconSize      size);
BtkWidget* btk_image_new_from_gicon     (GIcon           *icon,
					 BtkIconSize      size);

void btk_image_clear              (BtkImage        *image);
void btk_image_set_from_pixmap    (BtkImage        *image,
                                   BdkPixmap       *pixmap,
                                   BdkBitmap       *mask);
void btk_image_set_from_image     (BtkImage        *image,
                                   BdkImage        *bdk_image,
                                   BdkBitmap       *mask);
void btk_image_set_from_file      (BtkImage        *image,
                                   const gchar     *filename);
void btk_image_set_from_pixbuf    (BtkImage        *image,
                                   BdkPixbuf       *pixbuf);
void btk_image_set_from_stock     (BtkImage        *image,
                                   const gchar     *stock_id,
                                   BtkIconSize      size);
void btk_image_set_from_icon_set  (BtkImage        *image,
                                   BtkIconSet      *icon_set,
                                   BtkIconSize      size);
void btk_image_set_from_animation (BtkImage           *image,
                                   BdkPixbufAnimation *animation);
void btk_image_set_from_icon_name (BtkImage        *image,
				   const gchar     *icon_name,
				   BtkIconSize      size);
void btk_image_set_from_gicon     (BtkImage        *image,
				   GIcon           *icon,
				   BtkIconSize      size);
void btk_image_set_pixel_size     (BtkImage        *image,
				   gint             pixel_size);

BtkImageType btk_image_get_storage_type (BtkImage   *image);

void       btk_image_get_pixmap   (BtkImage         *image,
                                   BdkPixmap       **pixmap,
                                   BdkBitmap       **mask);
void       btk_image_get_image    (BtkImage         *image,
                                   BdkImage        **bdk_image,
                                   BdkBitmap       **mask);
BdkPixbuf* btk_image_get_pixbuf   (BtkImage         *image);
void       btk_image_get_stock    (BtkImage         *image,
                                   gchar           **stock_id,
                                   BtkIconSize      *size);
void       btk_image_get_icon_set (BtkImage         *image,
                                   BtkIconSet      **icon_set,
                                   BtkIconSize      *size);
BdkPixbufAnimation* btk_image_get_animation (BtkImage *image);
void       btk_image_get_icon_name (BtkImage              *image,
				    const gchar          **icon_name,
				    BtkIconSize           *size);
void       btk_image_get_gicon     (BtkImage              *image,
				    GIcon                **gicon,
				    BtkIconSize           *size);
gint       btk_image_get_pixel_size (BtkImage             *image);

#ifndef BTK_DISABLE_DEPRECATED
/* These three are deprecated */

void       btk_image_set      (BtkImage   *image,
			       BdkImage   *val,
			       BdkBitmap  *mask);
void       btk_image_get      (BtkImage   *image,
			       BdkImage  **val,
			       BdkBitmap **mask);
#endif /* BTK_DISABLE_DEPRECATED */

B_END_DECLS

#endif /* __BTK_IMAGE_H__ */
