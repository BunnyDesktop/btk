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

#ifndef BTK_DISABLE_DEPRECATED

#ifndef __BTK_PREVIEW_H__
#define __BTK_PREVIEW_H__

#include <btk/btk.h>


B_BEGIN_DECLS

#define BTK_TYPE_PREVIEW            (btk_preview_get_type ())
#define BTK_PREVIEW(obj)            (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_PREVIEW, BtkPreview))
#define BTK_PREVIEW_CLASS(klass)    (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_PREVIEW, BtkPreviewClass))
#define BTK_IS_PREVIEW(obj)         (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_PREVIEW))
#define BTK_IS_PREVIEW_CLASS(klass) (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_PREVIEW))
#define BTK_PREVIEW_GET_CLASS(obj)  (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_PREVIEW, BtkPreviewClass))


typedef struct _BtkPreview       BtkPreview;
typedef struct _BtkPreviewInfo   BtkPreviewInfo;
typedef union  _BtkDitherInfo    BtkDitherInfo;
typedef struct _BtkPreviewClass  BtkPreviewClass;

struct _BtkPreview
{
  BtkWidget widget;

  buchar *buffer;
  buint16 buffer_width;
  buint16 buffer_height;

  buint16 bpp;
  buint16 rowstride;

  BdkRgbDither dither;

  buint type : 1;
  buint expand : 1;
};

struct _BtkPreviewInfo
{
  buchar *lookup;

  bdouble gamma;
};

union _BtkDitherInfo
{
  bushort s[2];
  buchar c[4];
};

struct _BtkPreviewClass
{
  BtkWidgetClass parent_class;

  BtkPreviewInfo info;

};


GType           btk_preview_get_type           (void) B_GNUC_CONST;
void            btk_preview_uninit             (void);
BtkWidget*      btk_preview_new                (BtkPreviewType   type);
void            btk_preview_size               (BtkPreview      *preview,
						bint             width,
						bint             height);
void            btk_preview_put                (BtkPreview      *preview,
						BdkWindow       *window,
						BdkGC           *gc,
						bint             srcx,
						bint             srcy,
						bint             destx,
						bint             desty,
						bint             width,
						bint             height);
void            btk_preview_draw_row           (BtkPreview      *preview,
						buchar          *data,
						bint             x,
						bint             y,
						bint             w);
void            btk_preview_set_expand         (BtkPreview      *preview,
						bboolean         expand);

void            btk_preview_set_gamma          (double           gamma_);
void            btk_preview_set_color_cube     (buint            nred_shades,
						buint            ngreen_shades,
						buint            nblue_shades,
						buint            ngray_shades);
void            btk_preview_set_install_cmap   (bint             install_cmap);
void            btk_preview_set_reserved       (bint             nreserved);
void            btk_preview_set_dither         (BtkPreview      *preview,
						BdkRgbDither     dither);
BdkVisual*      btk_preview_get_visual         (void);
BdkColormap*    btk_preview_get_cmap           (void);
BtkPreviewInfo* btk_preview_get_info           (void);

/* This function reinitializes the preview colormap and visual from
 * the current gamma/color_cube/install_cmap settings. It must only
 * be called if there are no previews or users's of the preview
 * colormap in existence.
 */
void            btk_preview_reset              (void);


B_END_DECLS

#endif /* __BTK_PREVIEW_H__ */

#endif /* BTK_DISABLE_DEPRECATED */
