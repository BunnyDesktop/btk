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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * BtkLayout: Widget for scrolling of arbitrary-sized areas.
 *
 * Copyright Owen Taylor, 1998
 */

/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/.
 */

#ifndef __BTK_LAYOUT_H__
#define __BTK_LAYOUT_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkcontainer.h>
#include <btk/btkadjustment.h>


G_BEGIN_DECLS

#define BTK_TYPE_LAYOUT            (btk_layout_get_type ())
#define BTK_LAYOUT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_LAYOUT, BtkLayout))
#define BTK_LAYOUT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_LAYOUT, BtkLayoutClass))
#define BTK_IS_LAYOUT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_LAYOUT))
#define BTK_IS_LAYOUT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_LAYOUT))
#define BTK_LAYOUT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_LAYOUT, BtkLayoutClass))


typedef struct _BtkLayout        BtkLayout;
typedef struct _BtkLayoutClass   BtkLayoutClass;

struct _BtkLayout
{
  BtkContainer GSEAL (container);

  GList *GSEAL (children);

  guint GSEAL (width);
  guint GSEAL (height);

  BtkAdjustment *GSEAL (hadjustment);
  BtkAdjustment *GSEAL (vadjustment);

  /*< public >*/
  BdkWindow *GSEAL (bin_window);

  /*< private >*/
  BdkVisibilityState GSEAL (visibility);
  gint GSEAL (scroll_x);
  gint GSEAL (scroll_y);

  guint GSEAL (freeze_count);
};

struct _BtkLayoutClass
{
  BtkContainerClass parent_class;

  void  (*set_scroll_adjustments)   (BtkLayout	    *layout,
				     BtkAdjustment  *hadjustment,
				     BtkAdjustment  *vadjustment);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};

GType          btk_layout_get_type        (void) G_GNUC_CONST;
BtkWidget*     btk_layout_new             (BtkAdjustment *hadjustment,
				           BtkAdjustment *vadjustment);
BdkWindow*     btk_layout_get_bin_window  (BtkLayout     *layout);
void           btk_layout_put             (BtkLayout     *layout,
		                           BtkWidget     *child_widget,
		                           gint           x,
		                           gint           y);

void           btk_layout_move            (BtkLayout     *layout,
		                           BtkWidget     *child_widget,
		                           gint           x,
		                           gint           y);

void           btk_layout_set_size        (BtkLayout     *layout,
			                   guint          width,
			                   guint          height);
void           btk_layout_get_size        (BtkLayout     *layout,
					   guint         *width,
					   guint         *height);

BtkAdjustment* btk_layout_get_hadjustment (BtkLayout     *layout);
BtkAdjustment* btk_layout_get_vadjustment (BtkLayout     *layout);
void           btk_layout_set_hadjustment (BtkLayout     *layout,
					   BtkAdjustment *adjustment);
void           btk_layout_set_vadjustment (BtkLayout     *layout,
					   BtkAdjustment *adjustment);


#ifndef BTK_DISABLE_DEPRECATED
/* These disable and enable moving and repainting the scrolling window
 * of the BtkLayout, respectively.  If you want to update the layout's
 * offsets but do not want it to repaint itself, you should use these
 * functions.
 *
 * - I don't understand these are supposed to work, so I suspect
 * - they don't now.                    OWT 1/20/98
 */
void           btk_layout_freeze          (BtkLayout     *layout);
void           btk_layout_thaw            (BtkLayout     *layout);
#endif /* BTK_DISABLE_DEPRECATED */

G_END_DECLS

#endif /* __BTK_LAYOUT_H__ */
