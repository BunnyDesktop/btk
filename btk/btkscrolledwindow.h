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

#ifndef __BTK_SCROLLED_WINDOW_H__
#define __BTK_SCROLLED_WINDOW_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkhscrollbar.h>
#include <btk/btkvscrollbar.h>
#include <btk/btkviewport.h>


B_BEGIN_DECLS


#define BTK_TYPE_SCROLLED_WINDOW            (btk_scrolled_window_get_type ())
#define BTK_SCROLLED_WINDOW(obj)            (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_SCROLLED_WINDOW, BtkScrolledWindow))
#define BTK_SCROLLED_WINDOW_CLASS(klass)    (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_SCROLLED_WINDOW, BtkScrolledWindowClass))
#define BTK_IS_SCROLLED_WINDOW(obj)         (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_SCROLLED_WINDOW))
#define BTK_IS_SCROLLED_WINDOW_CLASS(klass) (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_SCROLLED_WINDOW))
#define BTK_SCROLLED_WINDOW_GET_CLASS(obj)  (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_SCROLLED_WINDOW, BtkScrolledWindowClass))



typedef struct _BtkScrolledWindow       BtkScrolledWindow;
typedef struct _BtkScrolledWindowClass  BtkScrolledWindowClass;

struct _BtkScrolledWindow
{
  BtkBin container;

  /*< public >*/
  BtkWidget *GSEAL (hscrollbar);
  BtkWidget *GSEAL (vscrollbar);

  /*< private >*/
  buint GSEAL (hscrollbar_policy)      : 2;
  buint GSEAL (vscrollbar_policy)      : 2;
  buint GSEAL (hscrollbar_visible)     : 1;
  buint GSEAL (vscrollbar_visible)     : 1;
  buint GSEAL (window_placement)       : 2;
  buint GSEAL (focus_out)              : 1;	/* Flag used by ::move-focus-out implementation */

  buint16 GSEAL (shadow_type);
};

struct _BtkScrolledWindowClass
{
  BtkBinClass parent_class;

  bint scrollbar_spacing;

  /* Action signals for keybindings. Do not connect to these signals
   */

  /* Unfortunately, BtkScrollType is deficient in that there is
   * no horizontal/vertical variants for BTK_SCROLL_START/END,
   * so we have to add an additional boolean flag.
   */
  bboolean (*scroll_child) (BtkScrolledWindow *scrolled_window,
	  		    BtkScrollType      scroll,
			    bboolean           horizontal);

  void (* move_focus_out) (BtkScrolledWindow *scrolled_window,
			   BtkDirectionType   direction);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};


GType          btk_scrolled_window_get_type          (void) B_GNUC_CONST;
BtkWidget*     btk_scrolled_window_new               (BtkAdjustment     *hadjustment,
						      BtkAdjustment     *vadjustment);
void           btk_scrolled_window_set_hadjustment   (BtkScrolledWindow *scrolled_window,
						      BtkAdjustment     *hadjustment);
void           btk_scrolled_window_set_vadjustment   (BtkScrolledWindow *scrolled_window,
						      BtkAdjustment     *vadjustment);
BtkAdjustment* btk_scrolled_window_get_hadjustment   (BtkScrolledWindow *scrolled_window);
BtkAdjustment* btk_scrolled_window_get_vadjustment   (BtkScrolledWindow *scrolled_window);
BtkWidget*     btk_scrolled_window_get_hscrollbar    (BtkScrolledWindow *scrolled_window);
BtkWidget*     btk_scrolled_window_get_vscrollbar    (BtkScrolledWindow *scrolled_window);
void           btk_scrolled_window_set_policy        (BtkScrolledWindow *scrolled_window,
						      BtkPolicyType      hscrollbar_policy,
						      BtkPolicyType      vscrollbar_policy);
void           btk_scrolled_window_get_policy        (BtkScrolledWindow *scrolled_window,
						      BtkPolicyType     *hscrollbar_policy,
						      BtkPolicyType     *vscrollbar_policy);
void           btk_scrolled_window_set_placement     (BtkScrolledWindow *scrolled_window,
						      BtkCornerType      window_placement);
void           btk_scrolled_window_unset_placement   (BtkScrolledWindow *scrolled_window);

BtkCornerType  btk_scrolled_window_get_placement     (BtkScrolledWindow *scrolled_window);
void           btk_scrolled_window_set_shadow_type   (BtkScrolledWindow *scrolled_window,
						      BtkShadowType      type);
BtkShadowType  btk_scrolled_window_get_shadow_type   (BtkScrolledWindow *scrolled_window);
void	       btk_scrolled_window_add_with_viewport (BtkScrolledWindow *scrolled_window,
						      BtkWidget		*child);

bint _btk_scrolled_window_get_scrollbar_spacing (BtkScrolledWindow *scrolled_window);


B_END_DECLS


#endif /* __BTK_SCROLLED_WINDOW_H__ */
