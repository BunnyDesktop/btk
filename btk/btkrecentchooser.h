/* BTK - The GIMP Toolkit
 * btkrecentchooser.h - Abstract interface for recent file selectors GUIs
 *
 * Copyright (C) 2006, Emmanuele Bassi
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

#ifndef __BTK_RECENT_CHOOSER_H__
#define __BTK_RECENT_CHOOSER_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkwidget.h>
#include <btk/btkrecentmanager.h>
#include <btk/btkrecentfilter.h>

B_BEGIN_DECLS

#define BTK_TYPE_RECENT_CHOOSER			(btk_recent_chooser_get_type ())
#define BTK_RECENT_CHOOSER(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_RECENT_CHOOSER, BtkRecentChooser))
#define BTK_IS_RECENT_CHOOSER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_RECENT_CHOOSER))
#define BTK_RECENT_CHOOSER_GET_IFACE(inst)	(G_TYPE_INSTANCE_GET_INTERFACE ((inst), BTK_TYPE_RECENT_CHOOSER, BtkRecentChooserIface))

/**
 * BtkRecentSortType:
 * @BTK_RECENT_SORT_NONE: Do not sort the returned list of recently used
 *   resources.
 * @BTK_RECENT_SORT_MRU: Sort the returned list with the most recently used
 *   items first.
 * @BTK_RECENT_SORT_LRU: Sort the returned list with the least recently used
 *   items first.
 * @BTK_RECENT_SORT_CUSTOM: Sort the returned list using a custom sorting
 *   function passed using btk_recent_manager_set_sort_func().
 *
 * Used to specify the sorting method to be applyed to the recently
 * used resource list.
 **/
typedef enum
{
  BTK_RECENT_SORT_NONE = 0,
  BTK_RECENT_SORT_MRU,
  BTK_RECENT_SORT_LRU,
  BTK_RECENT_SORT_CUSTOM
} BtkRecentSortType;

typedef gint (*BtkRecentSortFunc) (BtkRecentInfo *a,
				   BtkRecentInfo *b,
				   gpointer       user_data);


typedef struct _BtkRecentChooser      BtkRecentChooser; /* dummy */
typedef struct _BtkRecentChooserIface BtkRecentChooserIface;

#define BTK_RECENT_CHOOSER_ERROR	(btk_recent_chooser_error_quark ())

typedef enum
{
  BTK_RECENT_CHOOSER_ERROR_NOT_FOUND,
  BTK_RECENT_CHOOSER_ERROR_INVALID_URI
} BtkRecentChooserError;

GQuark  btk_recent_chooser_error_quark (void);


struct _BtkRecentChooserIface
{
  GTypeInterface base_iface;

  /*
   * Methods
   */
  gboolean          (* set_current_uri)    (BtkRecentChooser  *chooser,
  					    const gchar       *uri,
  					    GError           **error);
  gchar *           (* get_current_uri)    (BtkRecentChooser  *chooser);
  gboolean          (* select_uri)         (BtkRecentChooser  *chooser,
  					    const gchar       *uri,
  					    GError           **error);
  void              (* unselect_uri)       (BtkRecentChooser  *chooser,
                                            const gchar       *uri);
  void              (* select_all)         (BtkRecentChooser  *chooser);
  void              (* unselect_all)       (BtkRecentChooser  *chooser);
  GList *           (* get_items)          (BtkRecentChooser  *chooser);
  BtkRecentManager *(* get_recent_manager) (BtkRecentChooser  *chooser);
  void              (* add_filter)         (BtkRecentChooser  *chooser,
  					    BtkRecentFilter   *filter);
  void              (* remove_filter)      (BtkRecentChooser  *chooser,
  					    BtkRecentFilter   *filter);
  GSList *          (* list_filters)       (BtkRecentChooser  *chooser);
  void              (* set_sort_func)      (BtkRecentChooser  *chooser,
  					    BtkRecentSortFunc  sort_func,
  					    gpointer           data,
  					    GDestroyNotify     destroy);

  /*
   * Signals
   */
  void		    (* item_activated)     (BtkRecentChooser  *chooser);
  void		    (* selection_changed)  (BtkRecentChooser  *chooser);
};

GType   btk_recent_chooser_get_type    (void) B_GNUC_CONST;

/*
 * Configuration
 */
void              btk_recent_chooser_set_show_private    (BtkRecentChooser  *chooser,
							  gboolean           show_private);
gboolean          btk_recent_chooser_get_show_private    (BtkRecentChooser  *chooser);
void              btk_recent_chooser_set_show_not_found  (BtkRecentChooser  *chooser,
							  gboolean           show_not_found);
gboolean          btk_recent_chooser_get_show_not_found  (BtkRecentChooser  *chooser);
void              btk_recent_chooser_set_select_multiple (BtkRecentChooser  *chooser,
							  gboolean           select_multiple);
gboolean          btk_recent_chooser_get_select_multiple (BtkRecentChooser  *chooser);
void              btk_recent_chooser_set_limit           (BtkRecentChooser  *chooser,
							  gint               limit);
gint              btk_recent_chooser_get_limit           (BtkRecentChooser  *chooser);
void              btk_recent_chooser_set_local_only      (BtkRecentChooser  *chooser,
							  gboolean           local_only);
gboolean          btk_recent_chooser_get_local_only      (BtkRecentChooser  *chooser);
void              btk_recent_chooser_set_show_tips       (BtkRecentChooser  *chooser,
							  gboolean           show_tips);
gboolean          btk_recent_chooser_get_show_tips       (BtkRecentChooser  *chooser);
#ifndef BTK_DISABLE_DEPRECATED
void              btk_recent_chooser_set_show_numbers    (BtkRecentChooser  *chooser,
							  gboolean           show_numbers);
gboolean          btk_recent_chooser_get_show_numbers    (BtkRecentChooser  *chooser);
#endif /* BTK_DISABLE_DEPRECATED */
void              btk_recent_chooser_set_show_icons      (BtkRecentChooser  *chooser,
							  gboolean           show_icons);
gboolean          btk_recent_chooser_get_show_icons      (BtkRecentChooser  *chooser);
void              btk_recent_chooser_set_sort_type       (BtkRecentChooser  *chooser,
							  BtkRecentSortType  sort_type);
BtkRecentSortType btk_recent_chooser_get_sort_type       (BtkRecentChooser  *chooser);
void              btk_recent_chooser_set_sort_func       (BtkRecentChooser  *chooser,
							  BtkRecentSortFunc  sort_func,
							  gpointer           sort_data,
							  GDestroyNotify     data_destroy);

/*
 * Items handling
 */
gboolean       btk_recent_chooser_set_current_uri  (BtkRecentChooser  *chooser,
						    const gchar       *uri,
						    GError           **error);
gchar *        btk_recent_chooser_get_current_uri  (BtkRecentChooser  *chooser);
BtkRecentInfo *btk_recent_chooser_get_current_item (BtkRecentChooser  *chooser);
gboolean       btk_recent_chooser_select_uri       (BtkRecentChooser  *chooser,
						    const gchar       *uri,
						    GError           **error);
void           btk_recent_chooser_unselect_uri     (BtkRecentChooser  *chooser,
					            const gchar       *uri);
void           btk_recent_chooser_select_all       (BtkRecentChooser  *chooser);
void           btk_recent_chooser_unselect_all     (BtkRecentChooser  *chooser);
GList *        btk_recent_chooser_get_items        (BtkRecentChooser  *chooser);
gchar **       btk_recent_chooser_get_uris         (BtkRecentChooser  *chooser,
						    gsize             *length);

/*
 * Filters
 */
void 		 btk_recent_chooser_add_filter    (BtkRecentChooser *chooser,
			 			   BtkRecentFilter  *filter);
void 		 btk_recent_chooser_remove_filter (BtkRecentChooser *chooser,
						   BtkRecentFilter  *filter);
GSList * 	 btk_recent_chooser_list_filters  (BtkRecentChooser *chooser);
void 		 btk_recent_chooser_set_filter    (BtkRecentChooser *chooser,
						   BtkRecentFilter  *filter);
BtkRecentFilter *btk_recent_chooser_get_filter    (BtkRecentChooser *chooser);


B_END_DECLS

#endif /* __BTK_RECENT_CHOOSER_H__ */
