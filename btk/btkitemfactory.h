/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * BtkItemFactory: Flexible item factory with automatic rc handling
 * Copyright (C) 1998 Tim Janik
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

#ifndef BTK_DISABLE_DEPRECATED

#ifndef __BTK_ITEM_FACTORY_H__
#define	__BTK_ITEM_FACTORY_H__

#include <btk/btk.h>


G_BEGIN_DECLS

typedef void	(*BtkPrintFunc)		   (gpointer		 func_data,
					    const gchar		*str);
/* We use () here to mean unspecified arguments. This is deprecated
 * as of C99, but we can't change it without breaking compatibility.
 * (Note that if we are included from a C++ program () will mean
 * (void) so an explicit cast will be needed.)
 */
typedef	void	(*BtkItemFactoryCallback)  ();
typedef	void	(*BtkItemFactoryCallback1) (gpointer		 callback_data,
					    guint		 callback_action,
					    BtkWidget		*widget);

#define BTK_TYPE_ITEM_FACTORY            (btk_item_factory_get_type ())
#define BTK_ITEM_FACTORY(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), BTK_TYPE_ITEM_FACTORY, BtkItemFactory))
#define BTK_ITEM_FACTORY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_ITEM_FACTORY, BtkItemFactoryClass))
#define BTK_IS_ITEM_FACTORY(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), BTK_TYPE_ITEM_FACTORY))
#define BTK_IS_ITEM_FACTORY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_ITEM_FACTORY))
#define BTK_ITEM_FACTORY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_ITEM_FACTORY, BtkItemFactoryClass))


typedef	struct	_BtkItemFactory			BtkItemFactory;
typedef	struct	_BtkItemFactoryClass		BtkItemFactoryClass;
typedef	struct	_BtkItemFactoryEntry		BtkItemFactoryEntry;
typedef	struct	_BtkItemFactoryItem		BtkItemFactoryItem;

struct _BtkItemFactory
{
  BtkObject		 object;

  gchar			*path;
  BtkAccelGroup		*accel_group;
  BtkWidget		*widget;
  GSList		*items;

  BtkTranslateFunc       translate_func;
  gpointer               translate_data;
  GDestroyNotify         translate_notify;
};

struct _BtkItemFactoryClass
{
  BtkObjectClass	 object_class;

  GHashTable		*item_ht;

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};

struct _BtkItemFactoryEntry
{
  gchar *path;
  gchar *accelerator;

  BtkItemFactoryCallback callback;
  guint			 callback_action;

  /* possible values:
   * NULL		-> "<Item>"
   * ""			-> "<Item>"
   * "<Title>"		-> create a title item
   * "<Item>"		-> create a simple item
   * "<ImageItem>"	-> create an item holding an image
   * "<StockItem>"	-> create an item holding a stock image
   * "<CheckItem>"	-> create a check item
   * "<ToggleItem>"	-> create a toggle item
   * "<RadioItem>"	-> create a radio item
   * <path>		-> path of a radio item to link against
   * "<Separator>"	-> create a separator
   * "<Tearoff>"	-> create a tearoff separator
   * "<Branch>"		-> create an item to hold sub items
   * "<LastBranch>"	-> create a right justified item to hold sub items
   */
  gchar		 *item_type;

  /* Extra data for some item types:
   *  ImageItem  -> pointer to inlined pixbuf stream
   *  StockItem  -> name of stock item
   */
  gconstpointer extra_data;
};

struct _BtkItemFactoryItem
{
  gchar *path;
  GSList *widgets;
};


GType		btk_item_factory_get_type	    (void) G_GNUC_CONST;

/* `container_type' must be of BTK_TYPE_MENU_BAR, BTK_TYPE_MENU,
 * or BTK_TYPE_OPTION_MENU.
 */
BtkItemFactory*	btk_item_factory_new	   (GType		 container_type,
					    const gchar		*path,
					    BtkAccelGroup       *accel_group);
void		btk_item_factory_construct (BtkItemFactory	*ifactory,
					    GType		 container_type,
					    const gchar		*path,
					    BtkAccelGroup       *accel_group);
     
/* These functions operate on BtkItemFactoryClass basis.
 */
void		btk_item_factory_add_foreign        (BtkWidget	    *accel_widget,
						     const gchar    *full_path,
						     BtkAccelGroup  *accel_group,
						     guint	     keyval,
						     BdkModifierType modifiers);
     
BtkItemFactory*       btk_item_factory_from_widget      (BtkWidget *widget);
const gchar *         btk_item_factory_path_from_widget (BtkWidget *widget);

BtkWidget*	btk_item_factory_get_item	      (BtkItemFactory *ifactory,
						       const gchar    *path);
BtkWidget*	btk_item_factory_get_widget	      (BtkItemFactory *ifactory,
						       const gchar    *path);
BtkWidget*	btk_item_factory_get_widget_by_action (BtkItemFactory *ifactory,
						       guint	       action);
BtkWidget*	btk_item_factory_get_item_by_action   (BtkItemFactory *ifactory,
						       guint	       action);

void	btk_item_factory_create_item	(BtkItemFactory		*ifactory,
					 BtkItemFactoryEntry	*entry,
					 gpointer		 callback_data,
					 guint			 callback_type);
void	btk_item_factory_create_items	(BtkItemFactory		*ifactory,
					 guint			 n_entries,
					 BtkItemFactoryEntry	*entries,
					 gpointer		 callback_data);
void	btk_item_factory_delete_item	(BtkItemFactory		*ifactory,
					 const gchar		*path);
void	btk_item_factory_delete_entry	(BtkItemFactory		*ifactory,
					 BtkItemFactoryEntry	*entry);
void	btk_item_factory_delete_entries	(BtkItemFactory		*ifactory,
					 guint			 n_entries,
					 BtkItemFactoryEntry	*entries);
void	btk_item_factory_popup		(BtkItemFactory		*ifactory,
					 guint			 x,
					 guint			 y,
					 guint			 mouse_button,
					 guint32		 time_);
void	btk_item_factory_popup_with_data(BtkItemFactory		*ifactory,
					 gpointer		 popup_data,
					 GDestroyNotify          destroy,
					 guint			 x,
					 guint			 y,
					 guint			 mouse_button,
					 guint32		 time_);
gpointer btk_item_factory_popup_data	(BtkItemFactory		*ifactory);
gpointer btk_item_factory_popup_data_from_widget (BtkWidget	*widget);
void   btk_item_factory_set_translate_func (BtkItemFactory      *ifactory,
					    BtkTranslateFunc     func,
					    gpointer             data,
					    GDestroyNotify       notify);

/* Compatibility functions for deprecated BtkMenuFactory code
 */

/* Used by btk_item_factory_create_menu_entries () */
typedef void (*BtkMenuCallback) (BtkWidget *widget,
				 gpointer   user_data);
typedef struct {
  gchar *path;
  gchar *accelerator;
  BtkMenuCallback callback;
  gpointer callback_data;
  BtkWidget *widget;
} BtkMenuEntry;

/* Used by btk_item_factory_callback_marshal () */
typedef	void	(*BtkItemFactoryCallback2) (BtkWidget		*widget,
					    gpointer		 callback_data,
					    guint		 callback_action);

/* Used by btk_item_factory_create_items () */
void	btk_item_factory_create_items_ac (BtkItemFactory	*ifactory,
					  guint			 n_entries,
					  BtkItemFactoryEntry	*entries,
					  gpointer		 callback_data,
					  guint			 callback_type);

BtkItemFactory*	btk_item_factory_from_path   (const gchar       *path);
void	btk_item_factory_create_menu_entries (guint		 n_entries,
					      BtkMenuEntry      *entries);
void	btk_item_factories_path_delete	   (const gchar		*ifactory_path,
					    const gchar		*path);

G_END_DECLS

#endif /* !BTK_DISABLE_DEPRECATED */

#endif	/* __BTK_ITEM_FACTORY_H__ */

