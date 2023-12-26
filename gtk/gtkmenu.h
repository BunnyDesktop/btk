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
 */

/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/.
 */

#ifndef __BTK_MENU_H__
#define __BTK_MENU_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkaccelgroup.h>
#include <btk/btkmenushell.h>


G_BEGIN_DECLS

#define BTK_TYPE_MENU			(btk_menu_get_type ())
#define BTK_MENU(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_MENU, BtkMenu))
#define BTK_MENU_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_MENU, BtkMenuClass))
#define BTK_IS_MENU(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_MENU))
#define BTK_IS_MENU_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_MENU))
#define BTK_MENU_GET_CLASS(obj)         (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_MENU, BtkMenuClass))


typedef struct _BtkMenu	      BtkMenu;
typedef struct _BtkMenuClass  BtkMenuClass;

typedef void (*BtkMenuPositionFunc) (BtkMenu   *menu,
				     gint      *x,
				     gint      *y,
				     gboolean  *push_in,
				     gpointer	user_data);
typedef void (*BtkMenuDetachFunc)   (BtkWidget *attach_widget,
				     BtkMenu   *menu);

struct _BtkMenu
{
  BtkMenuShell GSEAL (menu_shell);
  
  BtkWidget *GSEAL (parent_menu_item);
  BtkWidget *GSEAL (old_active_menu_item);

  BtkAccelGroup *GSEAL (accel_group);
  gchar         *GSEAL (accel_path);
  BtkMenuPositionFunc GSEAL (position_func);
  gpointer GSEAL (position_func_data);

  guint GSEAL (toggle_size);
  /* Do _not_ touch these widgets directly. We hide the reference
   * count from the toplevel to the menu, so it must be restored
   * before operating on these widgets
   */
  BtkWidget *GSEAL (toplevel);
  
  BtkWidget *GSEAL (tearoff_window);
  BtkWidget *GSEAL (tearoff_hbox);
  BtkWidget *GSEAL (tearoff_scrollbar);
  BtkAdjustment *GSEAL (tearoff_adjustment);

  BdkWindow *GSEAL (view_window);
  BdkWindow *GSEAL (bin_window);

  gint GSEAL (scroll_offset);
  gint GSEAL (saved_scroll_offset);
  gint GSEAL (scroll_step);
  guint GSEAL (timeout_id);
  
  /* When a submenu of this menu is popped up, motion in this
   * rebunnyion is ignored
   */
  BdkRebunnyion *GSEAL (navigation_rebunnyion); /* unused */
  guint GSEAL (navigation_timeout);

  guint GSEAL (needs_destruction_ref_count) : 1;
  guint GSEAL (torn_off) : 1;
  /* The tearoff is active when it is torn off and the not-torn-off
   * menu is not popped up.
   */
  guint GSEAL (tearoff_active) : 1;

  guint GSEAL (scroll_fast) : 1;

  guint GSEAL (upper_arrow_visible) : 1;
  guint GSEAL (lower_arrow_visible) : 1;
  guint GSEAL (upper_arrow_prelight) : 1;
  guint GSEAL (lower_arrow_prelight) : 1;
};

struct _BtkMenuClass
{
  BtkMenuShellClass parent_class;

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};


GType	   btk_menu_get_type		  (void) G_GNUC_CONST;
BtkWidget* btk_menu_new			  (void);

/* Display the menu onscreen */
void	   btk_menu_popup		  (BtkMenu	       *menu,
					   BtkWidget	       *parent_menu_shell,
					   BtkWidget	       *parent_menu_item,
					   BtkMenuPositionFunc	func,
					   gpointer		data,
					   guint		button,
					   guint32		activate_time);

/* Position the menu according to its position function. Called
 * from btkmenuitem.c when a menu-item changes its allocation
 */
void	   btk_menu_reposition		  (BtkMenu	       *menu);

void	   btk_menu_popdown		  (BtkMenu	       *menu);

/* Keep track of the last menu item selected. (For the purposes
 * of the option menu
 */
BtkWidget* btk_menu_get_active		  (BtkMenu	       *menu);
void	   btk_menu_set_active		  (BtkMenu	       *menu,
					   guint		index_);

/* set/get the accelerator group that holds global accelerators (should
 * be added to the corresponding toplevel with btk_window_add_accel_group().
 */
void	       btk_menu_set_accel_group	  (BtkMenu	       *menu,
					   BtkAccelGroup       *accel_group);
BtkAccelGroup* btk_menu_get_accel_group	  (BtkMenu	       *menu);
void           btk_menu_set_accel_path    (BtkMenu             *menu,
					   const gchar         *accel_path);
const gchar*   btk_menu_get_accel_path    (BtkMenu             *menu);

/* A reference count is kept for a widget when it is attached to
 * a particular widget. This is typically a menu item; it may also
 * be a widget with a popup menu - for instance, the Notebook widget.
 */
void	   btk_menu_attach_to_widget	  (BtkMenu	       *menu,
					   BtkWidget	       *attach_widget,
					   BtkMenuDetachFunc	detacher);
void	   btk_menu_detach		  (BtkMenu	       *menu);

/* This should be dumped in favor of data set when the menu is popped
 * up - that is currently in the ItemFactory code, but should be
 * in the Menu code.
 */
BtkWidget* btk_menu_get_attach_widget	  (BtkMenu	       *menu);

void       btk_menu_set_tearoff_state     (BtkMenu             *menu,
					   gboolean             torn_off);
gboolean   btk_menu_get_tearoff_state     (BtkMenu             *menu);

/* This sets the window manager title for the window that
 * appears when a menu is torn off
 */
void       btk_menu_set_title             (BtkMenu             *menu,
					   const gchar         *title);
const gchar *btk_menu_get_title           (BtkMenu             *menu);

void       btk_menu_reorder_child         (BtkMenu             *menu,
                                           BtkWidget           *child,
                                           gint                position);

void	   btk_menu_set_screen		  (BtkMenu	       *menu,
					   BdkScreen	       *screen);

void       btk_menu_attach                (BtkMenu             *menu,
                                           BtkWidget           *child,
                                           guint                left_attach,
                                           guint                right_attach,
                                           guint                top_attach,
                                           guint                bottom_attach);

void       btk_menu_set_monitor           (BtkMenu             *menu,
                                           gint                 monitor_num);
gint       btk_menu_get_monitor           (BtkMenu             *menu);
GList*     btk_menu_get_for_attach_widget (BtkWidget           *widget); 

#ifndef BTK_DISABLE_DEPRECATED
#define btk_menu_append(menu,child)	btk_menu_shell_append  ((BtkMenuShell *)(menu),(child))
#define btk_menu_prepend(menu,child)    btk_menu_shell_prepend ((BtkMenuShell *)(menu),(child))
#define btk_menu_insert(menu,child,pos)	btk_menu_shell_insert ((BtkMenuShell *)(menu),(child),(pos))
#endif /* BTK_DISABLE_DEPRECATED */

void     btk_menu_set_reserve_toggle_size (BtkMenu  *menu,
                                          gboolean   reserve_toggle_size);
gboolean btk_menu_get_reserve_toggle_size (BtkMenu  *menu);


G_END_DECLS

#endif /* __BTK_MENU_H__ */
