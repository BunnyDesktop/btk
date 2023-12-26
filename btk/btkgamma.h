/* BTK - The GIMP Toolkit
 * Copyright (C) 1997 David Mosberger
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

#ifndef __BTK_GAMMA_CURVE_H__
#define __BTK_GAMMA_CURVE_H__


#include <btk/btkvbox.h>


G_BEGIN_DECLS

#define BTK_TYPE_GAMMA_CURVE            (btk_gamma_curve_get_type ())
#define BTK_GAMMA_CURVE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_GAMMA_CURVE, BtkGammaCurve))
#define BTK_GAMMA_CURVE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_GAMMA_CURVE, BtkGammaCurveClass))
#define BTK_IS_GAMMA_CURVE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_GAMMA_CURVE))
#define BTK_IS_GAMMA_CURVE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_GAMMA_CURVE))
#define BTK_GAMMA_CURVE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_GAMMA_CURVE, BtkGammaCurveClass))

typedef struct _BtkGammaCurve		BtkGammaCurve;
typedef struct _BtkGammaCurveClass	BtkGammaCurveClass;


struct _BtkGammaCurve
{
  BtkVBox vbox;

  BtkWidget *GSEAL (table);
  BtkWidget *GSEAL (curve);
  BtkWidget *GSEAL (button[5]);	/* spline, linear, free, gamma, reset */

  gfloat GSEAL (gamma);
  BtkWidget *GSEAL (gamma_dialog);
  BtkWidget *GSEAL (gamma_text);
};

struct _BtkGammaCurveClass
{
  BtkVBoxClass parent_class;

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};


GType      btk_gamma_curve_get_type (void) G_GNUC_CONST;
BtkWidget* btk_gamma_curve_new      (void);


G_END_DECLS

#endif /* __BTK_GAMMA_CURVE_H__ */

#endif /* BTK_DISABLE_DEPRECATED */
