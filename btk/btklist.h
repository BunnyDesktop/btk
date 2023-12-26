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

#if !defined (BTK_DISABLE_DEPRECATED) || defined (__BTK_LIST_C__)

#ifndef __BTK_LIST_H__
#define __BTK_LIST_H__

#include <btk/btk.h>

B_BEGIN_DECLS

#define BTK_TYPE_LIST                  (btk_list_get_type ())
#define BTK_LIST(obj)                  (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_LIST, BtkList))
#define BTK_LIST_CLASS(klass)          (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_LIST, BtkListClass))
#define BTK_IS_LIST(obj)               (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_LIST))
#define BTK_IS_LIST_CLASS(klass)       (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_LIST))
#define BTK_LIST_GET_CLASS(obj)        (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_LIST, BtkListClass))


typedef struct _BtkList	      BtkList;
typedef struct _BtkListClass  BtkListClass;

struct _BtkList
{
  BtkContainer container;

  GList *children;
  GList *selection;

  GList *undo_selection;
  GList *undo_unselection;

  BtkWidget *last_focus_child;
  BtkWidget *undo_focus_child;

  guint htimer;
  guint vtimer;

  gint anchor;
  gint drag_pos;
  BtkStateType anchor_state;

  guint selection_mode : 2;
  guint drag_selection:1;
  guint add_mode:1;
};

struct _BtkListClass
{
  BtkContainerClass parent_class;

  void (* selection_changed) (BtkList	*list);
  void (* select_child)	     (BtkList	*list,
			      BtkWidget *child);
  void (* unselect_child)    (BtkList	*list,
			      BtkWidget *child);
};


GType      btk_list_get_type		  (void) B_GNUC_CONST;
BtkWidget* btk_list_new			  (void);
void	   btk_list_insert_items	  (BtkList	    *list,
					   GList	    *items,
					   gint		     position);
void	   btk_list_append_items	  (BtkList	    *list,
					   GList	    *items);
void	   btk_list_prepend_items	  (BtkList	    *list,
					   GList	    *items);
void	   btk_list_remove_items	  (BtkList	    *list,
					   GList	    *items);
void	   btk_list_remove_items_no_unref (BtkList	    *list,
					   GList	    *items);
void	   btk_list_clear_items		  (BtkList	    *list,
					   gint		     start,
					   gint		     end);
void	   btk_list_select_item		  (BtkList	    *list,
					   gint		     item);
void	   btk_list_unselect_item	  (BtkList	    *list,
					   gint		     item);
void	   btk_list_select_child	  (BtkList	    *list,
					   BtkWidget	    *child);
void	   btk_list_unselect_child	  (BtkList	    *list,
					   BtkWidget	    *child);
gint	   btk_list_child_position	  (BtkList	    *list,
					   BtkWidget	    *child);
void	   btk_list_set_selection_mode	  (BtkList	    *list,
					   BtkSelectionMode  mode);

void       btk_list_extend_selection      (BtkList          *list,
					   BtkScrollType     scroll_type,
					   gfloat            position,
					   gboolean          auto_start_selection);
void       btk_list_start_selection       (BtkList          *list);
void       btk_list_end_selection         (BtkList          *list);
void       btk_list_select_all            (BtkList          *list);
void       btk_list_unselect_all          (BtkList          *list);
void       btk_list_scroll_horizontal     (BtkList          *list,
					   BtkScrollType     scroll_type,
					   gfloat            position);
void       btk_list_scroll_vertical       (BtkList          *list,
					   BtkScrollType     scroll_type,
					   gfloat            position);
void       btk_list_toggle_add_mode       (BtkList          *list);
void       btk_list_toggle_focus_row      (BtkList          *list);
void       btk_list_toggle_row            (BtkList          *list,
					   BtkWidget        *item);
void       btk_list_undo_selection        (BtkList          *list);
void       btk_list_end_drag_selection    (BtkList          *list);

B_END_DECLS

#endif /* __BTK_LIST_H__ */

#endif /* BTK_DISABLE_DEPRECATED */
