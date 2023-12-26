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

#ifndef __BTK_PANED_H__
#define __BTK_PANED_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkcontainer.h>

B_BEGIN_DECLS

#define BTK_TYPE_PANED                  (btk_paned_get_type ())
#define BTK_PANED(obj)                  (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_PANED, BtkPaned))
#define BTK_PANED_CLASS(klass)          (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_PANED, BtkPanedClass))
#define BTK_IS_PANED(obj)               (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_PANED))
#define BTK_IS_PANED_CLASS(klass)       (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_PANED))
#define BTK_PANED_GET_CLASS(obj)        (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_PANED, BtkPanedClass))


typedef struct _BtkPaned        BtkPaned;
typedef struct _BtkPanedClass   BtkPanedClass;
typedef struct _BtkPanedPrivate BtkPanedPrivate;

struct _BtkPaned
{
  BtkContainer container;

  BtkWidget *GSEAL (child1);
  BtkWidget *GSEAL (child2);

  BdkWindow *GSEAL (handle);
  BdkGC *GSEAL (xor_gc);
  BdkCursorType GSEAL (cursor_type);

  /*< private >*/
  BdkRectangle GSEAL (handle_pos);

  gint GSEAL (child1_size);
  gint GSEAL (last_allocation);
  gint GSEAL (min_position);
  gint GSEAL (max_position);

  guint GSEAL (position_set) : 1;
  guint GSEAL (in_drag) : 1;
  guint GSEAL (child1_shrink) : 1;
  guint GSEAL (child1_resize) : 1;
  guint GSEAL (child2_shrink) : 1;
  guint GSEAL (child2_resize) : 1;
  guint GSEAL (orientation) : 1;
  guint GSEAL (in_recursion) : 1;
  guint GSEAL (handle_prelit) : 1;

  BtkWidget *GSEAL (last_child1_focus);
  BtkWidget *GSEAL (last_child2_focus);
  BtkPanedPrivate *GSEAL (priv);

  gint GSEAL (drag_pos);
  gint GSEAL (original_position);
};

struct _BtkPanedClass
{
  BtkContainerClass parent_class;

  gboolean (* cycle_child_focus)   (BtkPaned      *paned,
				    gboolean       reverse);
  gboolean (* toggle_handle_focus) (BtkPaned      *paned);
  gboolean (* move_handle)         (BtkPaned      *paned,
				    BtkScrollType  scroll);
  gboolean (* cycle_handle_focus)  (BtkPaned      *paned,
				    gboolean       reverse);
  gboolean (* accept_position)     (BtkPaned	  *paned);
  gboolean (* cancel_position)     (BtkPaned	  *paned);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};


GType       btk_paned_get_type     (void) B_GNUC_CONST;
void        btk_paned_add1         (BtkPaned       *paned,
                                    BtkWidget      *child);
void        btk_paned_add2         (BtkPaned       *paned,
                                    BtkWidget      *child);
void        btk_paned_pack1        (BtkPaned       *paned,
                                    BtkWidget      *child,
                                    gboolean        resize,
                                    gboolean        shrink);
void        btk_paned_pack2        (BtkPaned       *paned,
                                    BtkWidget      *child,
                                    gboolean        resize,
                                    gboolean        shrink);

gint        btk_paned_get_position (BtkPaned       *paned);
void        btk_paned_set_position (BtkPaned       *paned,
                                    gint            position);

BtkWidget * btk_paned_get_child1   (BtkPaned       *paned);
BtkWidget * btk_paned_get_child2   (BtkPaned       *paned);

BdkWindow * btk_paned_get_handle_window (BtkPaned  *paned);

#ifndef BTK_DISABLE_DEPRECATED
/* Internal function */
void    btk_paned_compute_position (BtkPaned  *paned,
                                    gint       allocation,
                                    gint       child1_req,
                                    gint       child2_req);
#define	btk_paned_gutter_size(p,s)		(void) 0
#define	btk_paned_set_gutter_size(p,s)		(void) 0
#endif /* BTK_DISABLE_DEPRECATED */

B_END_DECLS

#endif /* __BTK_PANED_H__ */
