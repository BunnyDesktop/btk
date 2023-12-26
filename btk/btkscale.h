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

#ifndef __BTK_SCALE_H__
#define __BTK_SCALE_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkrange.h>


B_BEGIN_DECLS

#define BTK_TYPE_SCALE            (btk_scale_get_type ())
#define BTK_SCALE(obj)            (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_SCALE, BtkScale))
#define BTK_SCALE_CLASS(klass)    (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_SCALE, BtkScaleClass))
#define BTK_IS_SCALE(obj)         (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_SCALE))
#define BTK_IS_SCALE_CLASS(klass) (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_SCALE))
#define BTK_SCALE_GET_CLASS(obj)  (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_SCALE, BtkScaleClass))


typedef struct _BtkScale        BtkScale;
typedef struct _BtkScaleClass   BtkScaleClass;

struct _BtkScale
{
  BtkRange range;

  bint  GSEAL (digits);
  buint GSEAL (draw_value) : 1;
  buint GSEAL (value_pos) : 2;
};

struct _BtkScaleClass
{
  BtkRangeClass parent_class;

  bchar* (* format_value) (BtkScale *scale,
                           bdouble   value);

  void (* draw_value) (BtkScale *scale);

  void (* get_layout_offsets) (BtkScale *scale,
                               bint     *x,
                               bint     *y);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
};

GType             btk_scale_get_type           (void) B_GNUC_CONST;
void              btk_scale_set_digits         (BtkScale        *scale,
                                                bint             digits);
bint              btk_scale_get_digits         (BtkScale        *scale);
void              btk_scale_set_draw_value     (BtkScale        *scale,
                                                bboolean         draw_value);
bboolean          btk_scale_get_draw_value     (BtkScale        *scale);
void              btk_scale_set_value_pos      (BtkScale        *scale,
                                                BtkPositionType  pos);
BtkPositionType   btk_scale_get_value_pos      (BtkScale        *scale);

BangoLayout     * btk_scale_get_layout         (BtkScale        *scale);
void              btk_scale_get_layout_offsets (BtkScale        *scale,
                                                bint            *x,
                                                bint            *y);

void              btk_scale_add_mark           (BtkScale        *scale,
                                                bdouble          value,
                                                BtkPositionType  position,
                                                const bchar     *markup);
void              btk_scale_clear_marks        (BtkScale        *scale);

/* internal API */
void              _btk_scale_clear_layout      (BtkScale        *scale);
void              _btk_scale_get_value_size    (BtkScale        *scale,
                                                bint            *width,
                                                bint            *height);
bchar           * _btk_scale_format_value      (BtkScale        *scale,
                                                bdouble          value);

B_END_DECLS

#endif /* __BTK_SCALE_H__ */
