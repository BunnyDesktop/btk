/* HSV color selector for BTK+
 *
 * Copyright (C) 1999 The Free Software Foundation
 *
 * Authors: Simon Budig <Simon.Budig@unix-ag.org> (original code)
 *          Federico Mena-Quintero <federico@gimp.org> (cleanup for BTK+)
 *          Jonathan Blandford <jrb@redhat.com> (cleanup for BTK+)
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

#ifndef __BTK_HSV_H__
#define __BTK_HSV_H__

#if !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkwidget.h>

G_BEGIN_DECLS

#define BTK_TYPE_HSV            (btk_hsv_get_type ())
#define BTK_HSV(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_HSV, BtkHSV))
#define BTK_HSV_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_HSV, BtkHSVClass))
#define BTK_IS_HSV(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_HSV))
#define BTK_IS_HSV_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_HSV))
#define BTK_HSV_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_HSV, BtkHSVClass))


typedef struct _BtkHSV      BtkHSV;
typedef struct _BtkHSVClass BtkHSVClass;

struct _BtkHSV
{
  BtkWidget parent_instance;

  /* Private data */
  gpointer GSEAL (priv);
};

struct _BtkHSVClass
{
  BtkWidgetClass parent_class;

  /* Notification signals */
  void (* changed) (BtkHSV          *hsv);

  /* Keybindings */
  void (* move)    (BtkHSV          *hsv,
                    BtkDirectionType type);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};


GType      btk_hsv_get_type     (void) G_GNUC_CONST;
BtkWidget* btk_hsv_new          (void);
void       btk_hsv_set_color    (BtkHSV    *hsv,
				 double     h,
				 double     s,
				 double     v);
void       btk_hsv_get_color    (BtkHSV    *hsv,
				 gdouble   *h,
				 gdouble   *s,
				 gdouble   *v);
void       btk_hsv_set_metrics  (BtkHSV    *hsv,
				 gint       size,
				 gint       ring_width);
void       btk_hsv_get_metrics  (BtkHSV    *hsv,
				 gint      *size,
				 gint      *ring_width);
gboolean   btk_hsv_is_adjusting (BtkHSV    *hsv);

/* Convert colors between the RGB and HSV color spaces */
void       btk_hsv_to_rgb       (gdouble    h,
				 gdouble    s,
				 gdouble    v,
				 gdouble   *r,
				 gdouble   *g,
				 gdouble   *b);
void       btk_rgb_to_hsv       (gdouble    r,
				 gdouble    g,
				 gdouble    b,
				 gdouble   *h,
				 gdouble   *s,
				 gdouble   *v);

G_END_DECLS

#endif /* __BTK_HSV_H__ */
