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

/*
 * NOTE this widget is considered too specialized/little-used for
 * BTK+, and will in the future be moved to some other package.  If
 * your application needs this widget, feel free to use it, as the
 * widget does work and is useful in some applications; it's just not
 * of general interest. However, we are not accepting new features for
 * the widget, and it will eventually move out of the BTK+
 * distribution.
 */

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#ifndef BTK_DISABLE_DEPRECATED

#ifndef __BTK_RULER_H__
#define __BTK_RULER_H__


#include <btk/btkwidget.h>


B_BEGIN_DECLS

#define BTK_TYPE_RULER            (btk_ruler_get_type ())
#define BTK_RULER(obj)            (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_RULER, BtkRuler))
#define BTK_RULER_CLASS(klass)    (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_RULER, BtkRulerClass))
#define BTK_IS_RULER(obj)         (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_RULER))
#define BTK_IS_RULER_CLASS(klass) (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_RULER))
#define BTK_RULER_GET_CLASS(obj)  (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_RULER, BtkRulerClass))


typedef struct _BtkRuler        BtkRuler;
typedef struct _BtkRulerClass   BtkRulerClass;
typedef struct _BtkRulerMetric  BtkRulerMetric;

/* All distances below are in 1/72nd's of an inch. (According to
 * Adobe that's a point, but points are really 1/72.27 in.)
 */
struct _BtkRuler
{
  BtkWidget widget;

  BdkPixmap *GSEAL (backing_store);
  BdkGC *GSEAL (non_gr_exp_gc);		/* unused */
  BtkRulerMetric *GSEAL (metric);
  gint GSEAL (xsrc);
  gint GSEAL (ysrc);
  gint GSEAL (slider_size);

  /* The upper limit of the ruler (in points) */
  gdouble GSEAL (lower);
  /* The lower limit of the ruler */
  gdouble GSEAL (upper);
  /* The position of the mark on the ruler */
  gdouble GSEAL (position);
  /* The maximum size of the ruler */
  gdouble GSEAL (max_size);
};

struct _BtkRulerClass
{
  BtkWidgetClass parent_class;

  void (* draw_ticks) (BtkRuler *ruler);
  void (* draw_pos)   (BtkRuler *ruler);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};

struct _BtkRulerMetric
{
  gchar *metric_name;
  gchar *abbrev;
  /* This should be points_per_unit. This is the size of the unit
   * in 1/72nd's of an inch and has nothing to do with screen pixels */
  gdouble pixels_per_unit;
  gdouble ruler_scale[10];
  gint subdivide[5];        /* five possible modes of subdivision */
};


GType           btk_ruler_get_type   (void) B_GNUC_CONST;
void            btk_ruler_set_metric (BtkRuler       *ruler,
                                      BtkMetricType   metric);
BtkMetricType   btk_ruler_get_metric (BtkRuler       *ruler);
void            btk_ruler_set_range  (BtkRuler       *ruler,
                                      gdouble         lower,
                                      gdouble         upper,
                                      gdouble         position,
                                      gdouble         max_size);
void            btk_ruler_get_range  (BtkRuler       *ruler,
                                      gdouble        *lower,
                                      gdouble        *upper,
                                      gdouble        *position,
                                      gdouble        *max_size);

void            btk_ruler_draw_ticks (BtkRuler       *ruler);
void            btk_ruler_draw_pos   (BtkRuler       *ruler);

B_END_DECLS

#endif /* __BTK_RULER_H__ */

#endif /* BTK_DISABLE_DEPRECATED */
