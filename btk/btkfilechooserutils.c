/* BTK - The GIMP Toolkit
 * btkfilechooserutils.c: Private utility functions useful for
 *                        implementing a BtkFileChooser interface
 * Copyright (C) 2003, Red Hat, Inc.
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

#include "config.h"
#include "btkfilechooserutils.h"
#include "btkfilechooser.h"
#include "btkfilesystem.h"
#include "btktypebuiltins.h"
#include "btkintl.h"
#include "btkalias.h"

static gboolean       delegate_set_current_folder     (BtkFileChooser    *chooser,
						       GFile             *file,
						       GError           **error);
static GFile *        delegate_get_current_folder     (BtkFileChooser    *chooser);
static void           delegate_set_current_name       (BtkFileChooser    *chooser,
						       const gchar       *name);
static gboolean       delegate_select_file            (BtkFileChooser    *chooser,
						       GFile             *file,
						       GError           **error);
static void           delegate_unselect_file          (BtkFileChooser    *chooser,
						       GFile             *file);
static void           delegate_select_all             (BtkFileChooser    *chooser);
static void           delegate_unselect_all           (BtkFileChooser    *chooser);
static GSList *       delegate_get_files              (BtkFileChooser    *chooser);
static GFile *        delegate_get_preview_file       (BtkFileChooser    *chooser);
static BtkFileSystem *delegate_get_file_system        (BtkFileChooser    *chooser);
static void           delegate_add_filter             (BtkFileChooser    *chooser,
						       BtkFileFilter     *filter);
static void           delegate_remove_filter          (BtkFileChooser    *chooser,
						       BtkFileFilter     *filter);
static GSList *       delegate_list_filters           (BtkFileChooser    *chooser);
static gboolean       delegate_add_shortcut_folder    (BtkFileChooser    *chooser,
						       GFile             *file,
						       GError           **error);
static gboolean       delegate_remove_shortcut_folder (BtkFileChooser    *chooser,
						       GFile             *file,
						       GError           **error);
static GSList *       delegate_list_shortcut_folders  (BtkFileChooser    *chooser);
static void           delegate_notify                 (GObject           *object,
						       GParamSpec        *pspec,
						       gpointer           data);
static void           delegate_current_folder_changed (BtkFileChooser    *chooser,
						       gpointer           data);
static void           delegate_selection_changed      (BtkFileChooser    *chooser,
						       gpointer           data);
static void           delegate_update_preview         (BtkFileChooser    *chooser,
						       gpointer           data);
static void           delegate_file_activated         (BtkFileChooser    *chooser,
						       gpointer           data);

static BtkFileChooserConfirmation delegate_confirm_overwrite (BtkFileChooser    *chooser,
							      gpointer           data);

/**
 * _btk_file_chooser_install_properties:
 * @klass: the class structure for a type deriving from #GObject
 *
 * Installs the necessary properties for a class implementing
 * #BtkFileChooser. A #BtkParamSpecOverride property is installed
 * for each property, using the values from the #BtkFileChooserProp
 * enumeration. The caller must make sure itself that the enumeration
 * values don't collide with some other property values they
 * are using.
 **/
void
_btk_file_chooser_install_properties (GObjectClass *klass)
{
  g_object_class_override_property (klass,
				    BTK_FILE_CHOOSER_PROP_ACTION,
				    "action");
  g_object_class_override_property (klass,
				    BTK_FILE_CHOOSER_PROP_EXTRA_WIDGET,
				    "extra-widget");
  g_object_class_override_property (klass,
				    BTK_FILE_CHOOSER_PROP_FILE_SYSTEM_BACKEND,
				    "file-system-backend");
  g_object_class_override_property (klass,
				    BTK_FILE_CHOOSER_PROP_FILTER,
				    "filter");
  g_object_class_override_property (klass,
				    BTK_FILE_CHOOSER_PROP_LOCAL_ONLY,
				    "local-only");
  g_object_class_override_property (klass,
				    BTK_FILE_CHOOSER_PROP_PREVIEW_WIDGET,
				    "preview-widget");
  g_object_class_override_property (klass,
				    BTK_FILE_CHOOSER_PROP_PREVIEW_WIDGET_ACTIVE,
				    "preview-widget-active");
  g_object_class_override_property (klass,
				    BTK_FILE_CHOOSER_PROP_USE_PREVIEW_LABEL,
				    "use-preview-label");
  g_object_class_override_property (klass,
				    BTK_FILE_CHOOSER_PROP_SELECT_MULTIPLE,
				    "select-multiple");
  g_object_class_override_property (klass,
				    BTK_FILE_CHOOSER_PROP_SHOW_HIDDEN,
				    "show-hidden");
  g_object_class_override_property (klass,
				    BTK_FILE_CHOOSER_PROP_DO_OVERWRITE_CONFIRMATION,
				    "do-overwrite-confirmation");
  g_object_class_override_property (klass,
				    BTK_FILE_CHOOSER_PROP_CREATE_FOLDERS,
				    "create-folders");
}

/**
 * _btk_file_chooser_delegate_iface_init:
 * @iface: a #BtkFileChoserIface structure
 *
 * An interface-initialization function for use in cases where
 * an object is simply delegating the methods, signals of
 * the #BtkFileChooser interface to another object.
 * _btk_file_chooser_set_delegate() must be called on each
 * instance of the object so that the delegate object can
 * be found.
 **/
void
_btk_file_chooser_delegate_iface_init (BtkFileChooserIface *iface)
{
  iface->set_current_folder = delegate_set_current_folder;
  iface->get_current_folder = delegate_get_current_folder;
  iface->set_current_name = delegate_set_current_name;
  iface->select_file = delegate_select_file;
  iface->unselect_file = delegate_unselect_file;
  iface->select_all = delegate_select_all;
  iface->unselect_all = delegate_unselect_all;
  iface->get_files = delegate_get_files;
  iface->get_preview_file = delegate_get_preview_file;
  iface->get_file_system = delegate_get_file_system;
  iface->add_filter = delegate_add_filter;
  iface->remove_filter = delegate_remove_filter;
  iface->list_filters = delegate_list_filters;
  iface->add_shortcut_folder = delegate_add_shortcut_folder;
  iface->remove_shortcut_folder = delegate_remove_shortcut_folder;
  iface->list_shortcut_folders = delegate_list_shortcut_folders;
}

/**
 * _btk_file_chooser_set_delegate:
 * @receiver: a #GObject implementing #BtkFileChooser
 * @delegate: another #GObject implementing #BtkFileChooser
 *
 * Establishes that calls on @receiver for #BtkFileChooser
 * methods should be delegated to @delegate, and that
 * #BtkFileChooser signals emitted on @delegate should be
 * forwarded to @receiver. Must be used in conjunction with
 * _btk_file_chooser_delegate_iface_init().
 **/
void
_btk_file_chooser_set_delegate (BtkFileChooser *receiver,
				BtkFileChooser *delegate)
{
  g_return_if_fail (BTK_IS_FILE_CHOOSER (receiver));
  g_return_if_fail (BTK_IS_FILE_CHOOSER (delegate));

  g_object_set_data (G_OBJECT (receiver), I_("btk-file-chooser-delegate"), delegate);
  g_signal_connect (delegate, "notify",
		    G_CALLBACK (delegate_notify), receiver);
  g_signal_connect (delegate, "current-folder-changed",
		    G_CALLBACK (delegate_current_folder_changed), receiver);
  g_signal_connect (delegate, "selection-changed",
		    G_CALLBACK (delegate_selection_changed), receiver);
  g_signal_connect (delegate, "update-preview",
		    G_CALLBACK (delegate_update_preview), receiver);
  g_signal_connect (delegate, "file-activated",
		    G_CALLBACK (delegate_file_activated), receiver);
  g_signal_connect (delegate, "confirm-overwrite",
		    G_CALLBACK (delegate_confirm_overwrite), receiver);
}

GQuark
_btk_file_chooser_delegate_get_quark (void)
{
  static GQuark quark = 0;

  if (B_UNLIKELY (quark == 0))
    quark = g_quark_from_static_string ("btk-file-chooser-delegate");
  
  return quark;
}

static BtkFileChooser *
get_delegate (BtkFileChooser *receiver)
{
  return g_object_get_qdata (G_OBJECT (receiver),
			     BTK_FILE_CHOOSER_DELEGATE_QUARK);
}

static gboolean
delegate_select_file (BtkFileChooser    *chooser,
		      GFile             *file,
		      GError           **error)
{
  return btk_file_chooser_select_file (get_delegate (chooser), file, error);
}

static void
delegate_unselect_file (BtkFileChooser *chooser,
			GFile          *file)
{
  btk_file_chooser_unselect_file (get_delegate (chooser), file);
}

static void
delegate_select_all (BtkFileChooser *chooser)
{
  btk_file_chooser_select_all (get_delegate (chooser));
}

static void
delegate_unselect_all (BtkFileChooser *chooser)
{
  btk_file_chooser_unselect_all (get_delegate (chooser));
}

static GSList *
delegate_get_files (BtkFileChooser *chooser)
{
  return btk_file_chooser_get_files (get_delegate (chooser));
}

static GFile *
delegate_get_preview_file (BtkFileChooser *chooser)
{
  return btk_file_chooser_get_preview_file (get_delegate (chooser));
}

static BtkFileSystem *
delegate_get_file_system (BtkFileChooser *chooser)
{
  return _btk_file_chooser_get_file_system (get_delegate (chooser));
}

static void
delegate_add_filter (BtkFileChooser *chooser,
		     BtkFileFilter  *filter)
{
  btk_file_chooser_add_filter (get_delegate (chooser), filter);
}

static void
delegate_remove_filter (BtkFileChooser *chooser,
			BtkFileFilter  *filter)
{
  btk_file_chooser_remove_filter (get_delegate (chooser), filter);
}

static GSList *
delegate_list_filters (BtkFileChooser *chooser)
{
  return btk_file_chooser_list_filters (get_delegate (chooser));
}

static gboolean
delegate_add_shortcut_folder (BtkFileChooser  *chooser,
			      GFile           *file,
			      GError         **error)
{
  return _btk_file_chooser_add_shortcut_folder (get_delegate (chooser), file, error);
}

static gboolean
delegate_remove_shortcut_folder (BtkFileChooser  *chooser,
				 GFile           *file,
				 GError         **error)
{
  return _btk_file_chooser_remove_shortcut_folder (get_delegate (chooser), file, error);
}

static GSList *
delegate_list_shortcut_folders (BtkFileChooser *chooser)
{
  return _btk_file_chooser_list_shortcut_folder_files (get_delegate (chooser));
}

static gboolean
delegate_set_current_folder (BtkFileChooser  *chooser,
			     GFile           *file,
			     GError         **error)
{
  return btk_file_chooser_set_current_folder_file (get_delegate (chooser), file, error);
}

static GFile *
delegate_get_current_folder (BtkFileChooser *chooser)
{
  return btk_file_chooser_get_current_folder_file (get_delegate (chooser));
}

static void
delegate_set_current_name (BtkFileChooser *chooser,
			   const gchar    *name)
{
  btk_file_chooser_set_current_name (get_delegate (chooser), name);
}

static void
delegate_notify (GObject    *object,
		 GParamSpec *pspec,
		 gpointer    data)
{
  gpointer iface;

  iface = g_type_interface_peek (g_type_class_peek (G_OBJECT_TYPE (object)),
				 btk_file_chooser_get_type ());
  if (g_object_interface_find_property (iface, pspec->name))
    g_object_notify (data, pspec->name);
}

static void
delegate_selection_changed (BtkFileChooser *chooser,
			    gpointer        data)
{
  g_signal_emit_by_name (data, "selection-changed");
}

static void
delegate_current_folder_changed (BtkFileChooser *chooser,
				 gpointer        data)
{
  g_signal_emit_by_name (data, "current-folder-changed");
}

static void
delegate_update_preview (BtkFileChooser    *chooser,
			 gpointer           data)
{
  g_signal_emit_by_name (data, "update-preview");
}

static void
delegate_file_activated (BtkFileChooser    *chooser,
			 gpointer           data)
{
  g_signal_emit_by_name (data, "file-activated");
}

static BtkFileChooserConfirmation
delegate_confirm_overwrite (BtkFileChooser    *chooser,
			    gpointer           data)
{
  BtkFileChooserConfirmation conf;

  g_signal_emit_by_name (data, "confirm-overwrite", &conf);
  return conf;
}

static GFile *
get_parent_for_uri (const char *uri)
{
  GFile *file;
  GFile *parent;

  file = g_file_new_for_uri (uri);
  parent = g_file_get_parent (file);

  g_object_unref (file);
  return parent;
	
}

/* Extracts the parent folders out of the supplied list of BtkRecentInfo* items, and returns
 * a list of GFile* for those unique parents.
 */
GList *
_btk_file_chooser_extract_recent_folders (GList *infos)
{
  GList *l;
  GList *result;
  GHashTable *folders;

  result = NULL;

  folders = g_hash_table_new (g_file_hash, (GEqualFunc) g_file_equal);

  for (l = infos; l; l = l->next)
    {
      BtkRecentInfo *info = l->data;
      const char *uri;
      GFile *parent;

      uri = btk_recent_info_get_uri (info);
      parent = get_parent_for_uri (uri);

      if (parent)
	{
	  if (!g_hash_table_lookup (folders, parent))
	    {
	      g_hash_table_insert (folders, parent, (gpointer) 1);
	      result = g_list_prepend (result, g_object_ref (parent));
	    }

	  g_object_unref (parent);
	}
    }

  result = g_list_reverse (result);

  g_hash_table_destroy (folders);

  return result;
}
