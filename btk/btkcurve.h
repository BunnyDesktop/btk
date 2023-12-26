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

#ifndef __BTK_CURVE_H__
#define __BTK_CURVE_H__


#include <btk/btkdrawingarea.h>


B_BEGIN_DECLS

#define BTK_TYPE_CURVE                  (btk_curve_get_type ())
#define BTK_CURVE(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_CURVE, BtkCurve))
#define BTK_CURVE_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_CURVE, BtkCurveClass))
#define BTK_IS_CURVE(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_CURVE))
#define BTK_IS_CURVE_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_CURVE))
#define BTK_CURVE_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_CURVE, BtkCurveClass))


typedef struct _BtkCurve	BtkCurve;
typedef struct _BtkCurveClass	BtkCurveClass;


struct _BtkCurve
{
  BtkDrawingArea graph;

  gint cursor_type;
  gfloat min_x;
  gfloat max_x;
  gfloat min_y;
  gfloat max_y;
  BdkPixmap *pixmap;
  BtkCurveType curve_type;
  gint height;                  /* (cached) graph height in pixels */
  gint grab_point;              /* point currently grabbed */
  gint last;

  /* (cached) curve points: */
  gint num_points;
  BdkPoint *point;

  /* control points: */
  gint num_ctlpoints;           /* number of control points */
  gfloat (*ctlpoint)[2];        /* array of control points */
};

struct _BtkCurveClass
{
  BtkDrawingAreaClass parent_class;

  void (* curve_type_changed) (BtkCurve *curve);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};


GType		btk_curve_get_type	(void) B_GNUC_CONST;
BtkWidget*	btk_curve_new		(void);
void		btk_curve_reset		(BtkCurve *curve);
void		btk_curve_set_gamma	(BtkCurve *curve, gfloat gamma_);
void		btk_curve_set_range	(BtkCurve *curve,
					 gfloat min_x, gfloat max_x,
					 gfloat min_y, gfloat max_y);
void		btk_curve_get_vector	(BtkCurve *curve,
					 int veclen, gfloat vector[]);
void		btk_curve_set_vector	(BtkCurve *curve,
					 int veclen, gfloat vector[]);
void		btk_curve_set_curve_type (BtkCurve *curve, BtkCurveType type);


B_END_DECLS

#endif /* __BTK_CURVE_H__ */

#endif /* BTK_DISABLE_DEPRECATED */
