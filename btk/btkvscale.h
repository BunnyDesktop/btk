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

#ifndef __BTK_VSCALE_H__
#define __BTK_VSCALE_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkscale.h>


G_BEGIN_DECLS


#define BTK_TYPE_VSCALE            (btk_vscale_get_type ())
#define BTK_VSCALE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_VSCALE, BtkVScale))
#define BTK_VSCALE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_VSCALE, BtkVScaleClass))
#define BTK_IS_VSCALE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_VSCALE))
#define BTK_IS_VSCALE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_VSCALE))
#define BTK_VSCALE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_VSCALE, BtkVScaleClass))


typedef struct _BtkVScale       BtkVScale;
typedef struct _BtkVScaleClass  BtkVScaleClass;

/**
 * BtkVScale:
 *
 * The #BtkVScale struct contains private data only, and
 * should be accessed using the functions below.
 */
struct _BtkVScale
{
  BtkScale scale;
};

struct _BtkVScaleClass
{
  BtkScaleClass parent_class;
};


GType      btk_vscale_get_type       (void) G_GNUC_CONST;
BtkWidget* btk_vscale_new            (BtkAdjustment *adjustment);
BtkWidget* btk_vscale_new_with_range (gdouble        min,
                                      gdouble        max,
                                      gdouble        step);


G_END_DECLS


#endif /* __BTK_VSCALE_H__ */
