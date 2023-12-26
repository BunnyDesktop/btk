/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
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

#ifndef __BTK_NOTEBOOK_H__
#define __BTK_NOTEBOOK_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkcontainer.h>


B_BEGIN_DECLS

#define BTK_TYPE_NOTEBOOK                  (btk_notebook_get_type ())
#define BTK_NOTEBOOK(obj)                  (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_NOTEBOOK, BtkNotebook))
#define BTK_NOTEBOOK_CLASS(klass)          (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_NOTEBOOK, BtkNotebookClass))
#define BTK_IS_NOTEBOOK(obj)               (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_NOTEBOOK))
#define BTK_IS_NOTEBOOK_CLASS(klass)       (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_NOTEBOOK))
#define BTK_NOTEBOOK_GET_CLASS(obj)        (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_NOTEBOOK, BtkNotebookClass))


typedef enum
{
  BTK_NOTEBOOK_TAB_FIRST,
  BTK_NOTEBOOK_TAB_LAST
} BtkNotebookTab;

typedef struct _BtkNotebook       BtkNotebook;
typedef struct _BtkNotebookClass  BtkNotebookClass;
#if !defined (BTK_DISABLE_DEPRECATED) || defined (BTK_COMPILATION)
typedef struct _BtkNotebookPage   BtkNotebookPage;
#endif

struct _BtkNotebook
{
  BtkContainer container;
  
#if !defined (BTK_DISABLE_DEPRECATED) || defined (BTK_COMPILATION)
  BtkNotebookPage *GSEAL (cur_page);
#else
  bpointer GSEAL (cur_page);
#endif
  GList *GSEAL (children);
  GList *GSEAL (first_tab);		/* The first tab visible (for scrolling notebooks) */
  GList *GSEAL (focus_tab);
  
  BtkWidget *GSEAL (menu);
  BdkWindow *GSEAL (event_window);
  
  buint32 GSEAL (timer);
  
  buint16 GSEAL (tab_hborder);
  buint16 GSEAL (tab_vborder);
  
  buint GSEAL (show_tabs)          : 1;
  buint GSEAL (homogeneous)        : 1;
  buint GSEAL (show_border)        : 1;
  buint GSEAL (tab_pos)            : 2;
  buint GSEAL (scrollable)         : 1;
  buint GSEAL (in_child)           : 3;
  buint GSEAL (click_child)        : 3;
  buint GSEAL (button)             : 2;
  buint GSEAL (need_timer)         : 1;
  buint GSEAL (child_has_focus)    : 1;
  buint GSEAL (have_visible_child) : 1;
  buint GSEAL (focus_out)          : 1;	/* Flag used by ::move-focus-out implementation */

  buint GSEAL (has_before_previous) : 1;
  buint GSEAL (has_before_next)     : 1;
  buint GSEAL (has_after_previous)  : 1;
  buint GSEAL (has_after_next)      : 1;
};

struct _BtkNotebookClass
{
  BtkContainerClass parent_class;

  void (* switch_page)       (BtkNotebook     *notebook,
#if !defined (BTK_DISABLE_DEPRECATED) || defined (BTK_COMPILATION)
                              BtkNotebookPage *page,
#else
                              bpointer         page,
#endif
			      buint            page_num);

  /* Action signals for keybindings */
  bboolean (* select_page)     (BtkNotebook       *notebook,
                                bboolean           move_focus);
  bboolean (* focus_tab)       (BtkNotebook       *notebook,
                                BtkNotebookTab     type);
  bboolean (* change_current_page) (BtkNotebook   *notebook,
                                bint               offset);
  void (* move_focus_out)      (BtkNotebook       *notebook,
				BtkDirectionType   direction);
  bboolean (* reorder_tab)     (BtkNotebook       *notebook,
				BtkDirectionType   direction,
				bboolean           move_to_last);

  /* More vfuncs */
  bint (* insert_page)         (BtkNotebook       *notebook,
			        BtkWidget         *child,
				BtkWidget         *tab_label,
				BtkWidget         *menu_label,
				bint               position);

  BtkNotebook * (* create_window) (BtkNotebook       *notebook,
                                   BtkWidget         *page,
                                   bint               x,
                                   bint               y);

  void (*_btk_reserved1) (void);
};

typedef BtkNotebook* (*BtkNotebookWindowCreationFunc) (BtkNotebook *source,
                                                       BtkWidget   *page,
                                                       bint         x,
                                                       bint         y,
                                                       bpointer     data);

/***********************************************************
 *           Creation, insertion, deletion                 *
 ***********************************************************/

GType   btk_notebook_get_type       (void) B_GNUC_CONST;
BtkWidget * btk_notebook_new        (void);
bint btk_notebook_append_page       (BtkNotebook *notebook,
				     BtkWidget   *child,
				     BtkWidget   *tab_label);
bint btk_notebook_append_page_menu  (BtkNotebook *notebook,
				     BtkWidget   *child,
				     BtkWidget   *tab_label,
				     BtkWidget   *menu_label);
bint btk_notebook_prepend_page      (BtkNotebook *notebook,
				     BtkWidget   *child,
				     BtkWidget   *tab_label);
bint btk_notebook_prepend_page_menu (BtkNotebook *notebook,
				     BtkWidget   *child,
				     BtkWidget   *tab_label,
				     BtkWidget   *menu_label);
bint btk_notebook_insert_page       (BtkNotebook *notebook,
				     BtkWidget   *child,
				     BtkWidget   *tab_label,
				     bint         position);
bint btk_notebook_insert_page_menu  (BtkNotebook *notebook,
				     BtkWidget   *child,
				     BtkWidget   *tab_label,
				     BtkWidget   *menu_label,
				     bint         position);
void btk_notebook_remove_page       (BtkNotebook *notebook,
				     bint         page_num);

/***********************************************************
 *           Tabs drag and drop                            *
 ***********************************************************/

#ifndef BTK_DISABLE_DEPRECATED
void btk_notebook_set_window_creation_hook (BtkNotebookWindowCreationFunc  func,
					    bpointer                       data,
                                            GDestroyNotify                 destroy);
void btk_notebook_set_group_id             (BtkNotebook *notebook,
					    bint         group_id);
bint btk_notebook_get_group_id             (BtkNotebook *notebook);

void btk_notebook_set_group                (BtkNotebook *notebook,
					    bpointer     group);
bpointer btk_notebook_get_group            (BtkNotebook *notebook);
#endif /* BTK_DISABLE_DEPRECATED */

void         btk_notebook_set_group_name   (BtkNotebook *notebook,
                                            const bchar *group_name);
const bchar *btk_notebook_get_group_name   (BtkNotebook *notebook);


/***********************************************************
 *            query, set current NotebookPage              *
 ***********************************************************/

bint       btk_notebook_get_current_page (BtkNotebook *notebook);
BtkWidget* btk_notebook_get_nth_page     (BtkNotebook *notebook,
					  bint         page_num);
bint       btk_notebook_get_n_pages      (BtkNotebook *notebook);
bint       btk_notebook_page_num         (BtkNotebook *notebook,
					  BtkWidget   *child);
void       btk_notebook_set_current_page (BtkNotebook *notebook,
					  bint         page_num);
void       btk_notebook_next_page        (BtkNotebook *notebook);
void       btk_notebook_prev_page        (BtkNotebook *notebook);

/***********************************************************
 *            set Notebook, NotebookTab style              *
 ***********************************************************/

void     btk_notebook_set_show_border      (BtkNotebook     *notebook,
					    bboolean         show_border);
bboolean btk_notebook_get_show_border      (BtkNotebook     *notebook);
void     btk_notebook_set_show_tabs        (BtkNotebook     *notebook,
					    bboolean         show_tabs);
bboolean btk_notebook_get_show_tabs        (BtkNotebook     *notebook);
void     btk_notebook_set_tab_pos          (BtkNotebook     *notebook,
				            BtkPositionType  pos);
BtkPositionType btk_notebook_get_tab_pos   (BtkNotebook     *notebook);

#ifndef BTK_DISABLE_DEPRECATED
void     btk_notebook_set_homogeneous_tabs (BtkNotebook     *notebook,
					    bboolean         homogeneous);
void     btk_notebook_set_tab_border       (BtkNotebook     *notebook,
					    buint            border_width);
void     btk_notebook_set_tab_hborder      (BtkNotebook     *notebook,
					    buint            tab_hborder);
void     btk_notebook_set_tab_vborder      (BtkNotebook     *notebook,
					    buint            tab_vborder);
#endif /* BTK_DISABLE_DEPRECATED */

void     btk_notebook_set_scrollable       (BtkNotebook     *notebook,
					    bboolean         scrollable);
bboolean btk_notebook_get_scrollable       (BtkNotebook     *notebook);
buint16  btk_notebook_get_tab_hborder      (BtkNotebook     *notebook);
buint16  btk_notebook_get_tab_vborder      (BtkNotebook     *notebook);

/***********************************************************
 *               enable/disable PopupMenu                  *
 ***********************************************************/

void btk_notebook_popup_enable  (BtkNotebook *notebook);
void btk_notebook_popup_disable (BtkNotebook *notebook);

/***********************************************************
 *             query/set NotebookPage Properties           *
 ***********************************************************/

BtkWidget * btk_notebook_get_tab_label    (BtkNotebook *notebook,
					   BtkWidget   *child);
void btk_notebook_set_tab_label           (BtkNotebook *notebook,
					   BtkWidget   *child,
					   BtkWidget   *tab_label);
void btk_notebook_set_tab_label_text      (BtkNotebook *notebook,
					   BtkWidget   *child,
					   const bchar *tab_text);
const bchar *btk_notebook_get_tab_label_text (BtkNotebook *notebook,
                                              BtkWidget   *child);
BtkWidget * btk_notebook_get_menu_label   (BtkNotebook *notebook,
					   BtkWidget   *child);
void btk_notebook_set_menu_label          (BtkNotebook *notebook,
					   BtkWidget   *child,
					   BtkWidget   *menu_label);
void btk_notebook_set_menu_label_text     (BtkNotebook *notebook,
					   BtkWidget   *child,
					   const bchar *menu_text);
const bchar *btk_notebook_get_menu_label_text (BtkNotebook *notebook,
                                               BtkWidget   *child);
#ifndef BTK_DISABLE_DEPRECATED
void btk_notebook_query_tab_label_packing (BtkNotebook *notebook,
					   BtkWidget   *child,
					   bboolean    *expand,
					   bboolean    *fill,
					   BtkPackType *pack_type);
void btk_notebook_set_tab_label_packing   (BtkNotebook *notebook,
					   BtkWidget   *child,
					   bboolean     expand,
					   bboolean     fill,
					   BtkPackType  pack_type);
#endif
void btk_notebook_reorder_child           (BtkNotebook *notebook,
					   BtkWidget   *child,
					   bint         position);
bboolean btk_notebook_get_tab_reorderable (BtkNotebook *notebook,
					   BtkWidget   *child);
void btk_notebook_set_tab_reorderable     (BtkNotebook *notebook,
					   BtkWidget   *child,
					   bboolean     reorderable);
bboolean btk_notebook_get_tab_detachable  (BtkNotebook *notebook,
					   BtkWidget   *child);
void btk_notebook_set_tab_detachable      (BtkNotebook *notebook,
					   BtkWidget   *child,
					   bboolean     detachable);

BtkWidget* btk_notebook_get_action_widget (BtkNotebook *notebook,
                                           BtkPackType  pack_type);
void       btk_notebook_set_action_widget (BtkNotebook *notebook,
                                           BtkWidget   *widget,
                                           BtkPackType  pack_type);

#ifndef BTK_DISABLE_DEPRECATED
#define	btk_notebook_current_page               btk_notebook_get_current_page
#define btk_notebook_set_page                   btk_notebook_set_current_page
#endif /* BTK_DISABLE_DEPRECATED */

B_END_DECLS

#endif /* __BTK_NOTEBOOK_H__ */
