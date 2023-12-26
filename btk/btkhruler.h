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

#ifndef __BTK_HRULER_H__
#define __BTK_HRULER_H__


#include <btk/btkruler.h>


G_BEGIN_DECLS


#define BTK_TYPE_HRULER	           (btk_hruler_get_type ())
#define BTK_HRULER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_HRULER, BtkHRuler))
#define BTK_HRULER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_HRULER, BtkHRulerClass))
#define BTK_IS_HRULER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_HRULER))
#define BTK_IS_HRULER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_HRULER))
#define BTK_HRULER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_HRULER, BtkHRulerClass))


typedef struct _BtkHRuler       BtkHRuler;
typedef struct _BtkHRulerClass  BtkHRulerClass;

struct _BtkHRuler
{
  BtkRuler ruler;
};

struct _BtkHRulerClass
{
  BtkRulerClass parent_class;
};


GType      btk_hruler_get_type (void) G_GNUC_CONST;
BtkWidget* btk_hruler_new      (void);


G_END_DECLS


#endif /* __BTK_HRULER_H__ */

#endif /* BTK_DISABLE_DEPRECATED */
