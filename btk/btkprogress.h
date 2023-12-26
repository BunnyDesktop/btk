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

#ifndef __BTK_PROGRESS_H__
#define __BTK_PROGRESS_H__


#include <btk/btkwidget.h>
#include <btk/btkadjustment.h>


B_BEGIN_DECLS

#if !defined (BTK_DISABLE_DEPRECATED) || defined (__BTK_PROGRESS_C__) || defined (__BTK_PROGRESS_BAR_C__)

#define BTK_TYPE_PROGRESS            (btk_progress_get_type ())
#define BTK_PROGRESS(obj)            (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_PROGRESS, BtkProgress))
#define BTK_PROGRESS_CLASS(klass)    (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_PROGRESS, BtkProgressClass))
#define BTK_IS_PROGRESS(obj)         (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_PROGRESS))
#define BTK_IS_PROGRESS_CLASS(klass) (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_PROGRESS))
#define BTK_PROGRESS_GET_CLASS(obj)  (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_PROGRESS, BtkProgressClass))

#endif /* !BTK_DISABLE_DEPRECATED */

typedef struct _BtkProgress       BtkProgress;
typedef struct _BtkProgressClass  BtkProgressClass;


struct _BtkProgress
{
  BtkWidget widget;

  BtkAdjustment *adjustment;
  BdkPixmap     *offscreen_pixmap;
  gchar         *format;
  gfloat         x_align;
  gfloat         y_align;

  guint          show_text : 1;
  guint          activity_mode : 1;
  guint          use_text_format : 1;
};

struct _BtkProgressClass
{
  BtkWidgetClass parent_class;

  void (* paint)            (BtkProgress *progress);
  void (* update)           (BtkProgress *progress);
  void (* act_mode_enter)   (BtkProgress *progress);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};

/* This entire interface is deprecated. Use BtkProgressBar
 * directly.
 */

#if !defined (BTK_DISABLE_DEPRECATED) || defined (__BTK_PROGRESS_C__) || defined (__BTK_PROGRESS_BAR_C__)

GType      btk_progress_get_type            (void) B_GNUC_CONST;
void       btk_progress_set_show_text       (BtkProgress   *progress,
					     gboolean       show_text);
void       btk_progress_set_text_alignment  (BtkProgress   *progress,
					     gfloat         x_align,
					     gfloat         y_align);
void       btk_progress_set_format_string   (BtkProgress   *progress,
					     const gchar   *format);
void       btk_progress_set_adjustment      (BtkProgress   *progress,
					     BtkAdjustment *adjustment);
void       btk_progress_configure           (BtkProgress   *progress,
					     gdouble        value,
					     gdouble        min,
					     gdouble        max);
void       btk_progress_set_percentage      (BtkProgress   *progress,
					     gdouble        percentage);
void       btk_progress_set_value           (BtkProgress   *progress,
					     gdouble        value);
gdouble    btk_progress_get_value           (BtkProgress   *progress);
void       btk_progress_set_activity_mode   (BtkProgress   *progress,
					     gboolean       activity_mode);
gchar*     btk_progress_get_current_text    (BtkProgress   *progress);
gchar*     btk_progress_get_text_from_value (BtkProgress   *progress,
					     gdouble        value);
gdouble    btk_progress_get_current_percentage (BtkProgress *progress);
gdouble    btk_progress_get_percentage_from_value (BtkProgress *progress,
						   gdouble      value);

#endif /* !BTK_DISABLE_DEPRECATED */

B_END_DECLS

#endif /* __BTK_PROGRESS_H__ */
