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

#ifndef __BTK_FRAME_H__
#define __BTK_FRAME_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkbin.h>


B_BEGIN_DECLS


#define BTK_TYPE_FRAME                  (btk_frame_get_type ())
#define BTK_FRAME(obj)                  (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_FRAME, BtkFrame))
#define BTK_FRAME_CLASS(klass)          (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_FRAME, BtkFrameClass))
#define BTK_IS_FRAME(obj)               (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_FRAME))
#define BTK_IS_FRAME_CLASS(klass)       (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_FRAME))
#define BTK_FRAME_GET_CLASS(obj)        (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_FRAME, BtkFrameClass))


typedef struct _BtkFrame       BtkFrame;
typedef struct _BtkFrameClass  BtkFrameClass;

struct _BtkFrame
{
  BtkBin bin;

  BtkWidget *GSEAL (label_widget);
  bint16 GSEAL (shadow_type);
  bfloat GSEAL (label_xalign);
  bfloat GSEAL (label_yalign);

  BtkAllocation GSEAL (child_allocation);
};

struct _BtkFrameClass
{
  BtkBinClass parent_class;

  void (*compute_child_allocation) (BtkFrame *frame, BtkAllocation *allocation);
};


GType      btk_frame_get_type         (void) B_GNUC_CONST;
BtkWidget* btk_frame_new              (const bchar   *label);

void                  btk_frame_set_label (BtkFrame    *frame,
					   const bchar *label);
const bchar *btk_frame_get_label      (BtkFrame    *frame);

void       btk_frame_set_label_widget (BtkFrame      *frame,
				       BtkWidget     *label_widget);
BtkWidget *btk_frame_get_label_widget (BtkFrame      *frame);
void       btk_frame_set_label_align  (BtkFrame      *frame,
				       bfloat         xalign,
				       bfloat         yalign);
void       btk_frame_get_label_align  (BtkFrame      *frame,
				       bfloat        *xalign,
				       bfloat        *yalign);
void       btk_frame_set_shadow_type  (BtkFrame      *frame,
				       BtkShadowType  type);
BtkShadowType btk_frame_get_shadow_type (BtkFrame    *frame);


B_END_DECLS


#endif /* __BTK_FRAME_H__ */
