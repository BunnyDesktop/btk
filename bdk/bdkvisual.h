/* BDK - The GIMP Drawing Kit
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

#ifndef __BDK_VISUAL_H__
#define __BDK_VISUAL_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BDK_H_INSIDE__) && !defined (BDK_COMPILATION)
#error "Only <bdk/bdk.h> can be included directly."
#endif

#include <bdk/bdktypes.h>

B_BEGIN_DECLS

#define BDK_TYPE_VISUAL              (bdk_visual_get_type ())
#define BDK_VISUAL(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BDK_TYPE_VISUAL, BdkVisual))
#define BDK_VISUAL_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BDK_TYPE_VISUAL, BdkVisualClass))
#define BDK_IS_VISUAL(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BDK_TYPE_VISUAL))
#define BDK_IS_VISUAL_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BDK_TYPE_VISUAL))
#define BDK_VISUAL_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), BDK_TYPE_VISUAL, BdkVisualClass))

typedef struct _BdkVisualClass    BdkVisualClass;

/* Types of visuals.
 *   StaticGray:
 *   Grayscale:
 *   StaticColor:
 *   PseudoColor:
 *   TrueColor:
 *   DirectColor:
 */
typedef enum
{
  BDK_VISUAL_STATIC_GRAY,
  BDK_VISUAL_GRAYSCALE,
  BDK_VISUAL_STATIC_COLOR,
  BDK_VISUAL_PSEUDO_COLOR,
  BDK_VISUAL_TRUE_COLOR,
  BDK_VISUAL_DIRECT_COLOR
} BdkVisualType;

/* The visual type.
 *   "type" is the type of visual this is (PseudoColor, TrueColor, etc).
 *   "depth" is the bit depth of this visual.
 *   "colormap_size" is the size of a colormap for this visual.
 *   "bits_per_rgb" is the number of significant bits per red, green and blue.
 *  The red, green and blue masks, shifts and precisions refer
 *   to value needed to calculate pixel values in TrueColor and DirectColor
 *   visuals. The "mask" is the significant bits within the pixel. The
 *   "shift" is the number of bits left we must shift a primary for it
 *   to be in position (according to the "mask"). "prec" refers to how
 *   much precision the pixel value contains for a particular primary.
 */
struct _BdkVisual
{
  GObject parent_instance;
  
  BdkVisualType GSEAL (type);
  gint GSEAL (depth);
  BdkByteOrder GSEAL (byte_order);
  gint GSEAL (colormap_size);
  gint GSEAL (bits_per_rgb);

  guint32 GSEAL (red_mask);
  gint GSEAL (red_shift);
  gint GSEAL (red_prec);

  guint32 GSEAL (green_mask);
  gint GSEAL (green_shift);
  gint GSEAL (green_prec);

  guint32 GSEAL (blue_mask);
  gint GSEAL (blue_shift);
  gint GSEAL (blue_prec);
};

GType         bdk_visual_get_type            (void) B_GNUC_CONST;

#ifndef BDK_MULTIHEAD_SAFE
gint	      bdk_visual_get_best_depth	     (void);
BdkVisualType bdk_visual_get_best_type	     (void);
BdkVisual*    bdk_visual_get_system	     (void);
BdkVisual*    bdk_visual_get_best	     (void);
BdkVisual*    bdk_visual_get_best_with_depth (gint	     depth);
BdkVisual*    bdk_visual_get_best_with_type  (BdkVisualType  visual_type);
BdkVisual*    bdk_visual_get_best_with_both  (gint	     depth,
					      BdkVisualType  visual_type);

void bdk_query_depths	    (gint	    **depths,
			     gint	     *count);
void bdk_query_visual_types (BdkVisualType  **visual_types,
			     gint	     *count);

GList* bdk_list_visuals (void);
#endif

BdkScreen *bdk_visual_get_screen (BdkVisual *visual);

BdkVisualType bdk_visual_get_visual_type         (BdkVisual *visual);
gint          bdk_visual_get_depth               (BdkVisual *visual);
BdkByteOrder  bdk_visual_get_byte_order          (BdkVisual *visual);
gint          bdk_visual_get_colormap_size       (BdkVisual *visual);
gint          bdk_visual_get_bits_per_rgb        (BdkVisual *visual);
void          bdk_visual_get_red_pixel_details   (BdkVisual *visual,
                                                  guint32   *mask,
                                                  gint      *shift,
                                                  gint      *precision);
void          bdk_visual_get_green_pixel_details (BdkVisual *visual,
                                                  guint32   *mask,
                                                  gint      *shift,
                                                  gint      *precision);
void          bdk_visual_get_blue_pixel_details  (BdkVisual *visual,
                                                  guint32   *mask,
                                                  gint      *shift,
                                                  gint      *precision);

#ifndef BDK_DISABLE_DEPRECATED
#define bdk_visual_ref(v) g_object_ref(v)
#define bdk_visual_unref(v) g_object_unref(v)
#endif

B_END_DECLS

#endif /* __BDK_VISUAL_H__ */
