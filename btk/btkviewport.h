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

#ifndef __BTK_VIEWPORT_H__
#define __BTK_VIEWPORT_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkadjustment.h>
#include <btk/btkbin.h>


G_BEGIN_DECLS


#define BTK_TYPE_VIEWPORT            (btk_viewport_get_type ())
#define BTK_VIEWPORT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_VIEWPORT, BtkViewport))
#define BTK_VIEWPORT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_VIEWPORT, BtkViewportClass))
#define BTK_IS_VIEWPORT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_VIEWPORT))
#define BTK_IS_VIEWPORT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_VIEWPORT))
#define BTK_VIEWPORT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_VIEWPORT, BtkViewportClass))


typedef struct _BtkViewport       BtkViewport;
typedef struct _BtkViewportClass  BtkViewportClass;

struct _BtkViewport
{
  BtkBin bin;

  BtkShadowType GSEAL (shadow_type);
  BdkWindow *GSEAL (view_window);
  BdkWindow *GSEAL (bin_window);
  BtkAdjustment *GSEAL (hadjustment);
  BtkAdjustment *GSEAL (vadjustment);
};

struct _BtkViewportClass
{
  BtkBinClass parent_class;

  void	(*set_scroll_adjustments)	(BtkViewport	*viewport,
					 BtkAdjustment	*hadjustment,
					 BtkAdjustment	*vadjustment);
};


GType          btk_viewport_get_type        (void) G_GNUC_CONST;
BtkWidget*     btk_viewport_new             (BtkAdjustment *hadjustment,
					     BtkAdjustment *vadjustment);
BtkAdjustment* btk_viewport_get_hadjustment (BtkViewport   *viewport);
BtkAdjustment* btk_viewport_get_vadjustment (BtkViewport   *viewport);
void           btk_viewport_set_hadjustment (BtkViewport   *viewport,
					     BtkAdjustment *adjustment);
void           btk_viewport_set_vadjustment (BtkViewport   *viewport,
					     BtkAdjustment *adjustment);
void           btk_viewport_set_shadow_type (BtkViewport   *viewport,
					     BtkShadowType  type);
BtkShadowType  btk_viewport_get_shadow_type (BtkViewport   *viewport);
BdkWindow*     btk_viewport_get_bin_window  (BtkViewport   *viewport);
BdkWindow*     btk_viewport_get_view_window (BtkViewport   *viewport);


G_END_DECLS


#endif /* __BTK_VIEWPORT_H__ */
