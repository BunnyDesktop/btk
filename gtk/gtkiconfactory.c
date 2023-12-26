/* BTK - The GIMP Toolkit
 * Copyright (C) 2000 Red Hat, Inc.
 *               2008 Johan Dahlin
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

#include "config.h"
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "btkiconfactory.h"
#include "btkiconcache.h"
#include "btkdebug.h"
#include "btkicontheme.h"
#include "btksettings.h"
#include "btkstock.h"
#include "btkwidget.h"
#include "btkintl.h"
#include "btkbuildable.h"
#include "btkbuilderprivate.h"
#include "btkalias.h"


static GSList *all_icon_factories = NULL;

typedef enum {
  BTK_ICON_SOURCE_EMPTY,
  BTK_ICON_SOURCE_ICON_NAME,
  BTK_ICON_SOURCE_STATIC_ICON_NAME,
  BTK_ICON_SOURCE_FILENAME,
  BTK_ICON_SOURCE_PIXBUF
} BtkIconSourceType;

struct _BtkIconSource
{
  BtkIconSourceType type;

  union {
    gchar *icon_name;
    gchar *filename;
    BdkPixbuf *pixbuf;
  } source;

  BdkPixbuf *filename_pixbuf;

  BtkTextDirection direction;
  BtkStateType state;
  BtkIconSize size;

  /* If TRUE, then the parameter is wildcarded, and the above
   * fields should be ignored. If FALSE, the parameter is
   * specified, and the above fields should be valid.
   */
  guint any_direction : 1;
  guint any_state : 1;
  guint any_size : 1;

#if defined (G_OS_WIN32) && !defined (_WIN64)
  /* System codepage version of filename, for DLL ABI backward
   * compatibility functions.
   */
  gchar *cp_filename;
#endif
};


static void
btk_icon_factory_buildable_init  (BtkBuildableIface      *iface);

static gboolean btk_icon_factory_buildable_custom_tag_start (BtkBuildable     *buildable,
							     BtkBuilder       *builder,
							     GObject          *child,
							     const gchar      *tagname,
							     GMarkupParser    *parser,
							     gpointer         *data);
static void btk_icon_factory_buildable_custom_tag_end (BtkBuildable *buildable,
						       BtkBuilder   *builder,
						       GObject      *child,
						       const gchar  *tagname,
						       gpointer     *user_data);
static void btk_icon_factory_finalize   (GObject             *object);
static void get_default_icons           (BtkIconFactory      *icon_factory);
static void icon_source_clear           (BtkIconSource       *source);

static BtkIconSize icon_size_register_intern (const gchar *name,
					      gint         width,
					      gint         height);

#define BTK_ICON_SOURCE_INIT(any_direction, any_state, any_size)	\
  { BTK_ICON_SOURCE_EMPTY, { NULL }, NULL,				\
   0, 0, 0,								\
   any_direction, any_state, any_size }

G_DEFINE_TYPE_WITH_CODE (BtkIconFactory, btk_icon_factory, G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_BUILDABLE,
						btk_icon_factory_buildable_init))

static void
btk_icon_factory_init (BtkIconFactory *factory)
{
  factory->icons = g_hash_table_new (g_str_hash, g_str_equal);
  all_icon_factories = g_slist_prepend (all_icon_factories, factory);
}

static void
btk_icon_factory_class_init (BtkIconFactoryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = btk_icon_factory_finalize;
}

static void
btk_icon_factory_buildable_init (BtkBuildableIface *iface)
{
  iface->custom_tag_start = btk_icon_factory_buildable_custom_tag_start;
  iface->custom_tag_end = btk_icon_factory_buildable_custom_tag_end;
}

static void
free_icon_set (gpointer key, gpointer value, gpointer data)
{
  g_free (key);
  btk_icon_set_unref (value);
}

static void
btk_icon_factory_finalize (GObject *object)
{
  BtkIconFactory *factory = BTK_ICON_FACTORY (object);

  all_icon_factories = g_slist_remove (all_icon_factories, factory);

  g_hash_table_foreach (factory->icons, free_icon_set, NULL);

  g_hash_table_destroy (factory->icons);

  G_OBJECT_CLASS (btk_icon_factory_parent_class)->finalize (object);
}

/**
 * btk_icon_factory_new:
 *
 * Creates a new #BtkIconFactory. An icon factory manages a collection
 * of #BtkIconSet<!-- -->s; a #BtkIconSet manages a set of variants of a
 * particular icon (i.e. a #BtkIconSet contains variants for different
 * sizes and widget states). Icons in an icon factory are named by a
 * stock ID, which is a simple string identifying the icon. Each
 * #BtkStyle has a list of #BtkIconFactory<!-- -->s derived from the current
 * theme; those icon factories are consulted first when searching for
 * an icon. If the theme doesn't set a particular icon, BTK+ looks for
 * the icon in a list of default icon factories, maintained by
 * btk_icon_factory_add_default() and
 * btk_icon_factory_remove_default(). Applications with icons should
 * add a default icon factory with their icons, which will allow
 * themes to override the icons for the application.
 *
 * Return value: a new #BtkIconFactory
 */
BtkIconFactory*
btk_icon_factory_new (void)
{
  return g_object_new (BTK_TYPE_ICON_FACTORY, NULL);
}

/**
 * btk_icon_factory_add:
 * @factory: a #BtkIconFactory
 * @stock_id: icon name
 * @icon_set: icon set
 *
 * Adds the given @icon_set to the icon factory, under the name
 * @stock_id.  @stock_id should be namespaced for your application,
 * e.g. "myapp-whatever-icon".  Normally applications create a
 * #BtkIconFactory, then add it to the list of default factories with
 * btk_icon_factory_add_default(). Then they pass the @stock_id to
 * widgets such as #BtkImage to display the icon. Themes can provide
 * an icon with the same name (such as "myapp-whatever-icon") to
 * override your application's default icons. If an icon already
 * existed in @factory for @stock_id, it is unreferenced and replaced
 * with the new @icon_set.
 */
void
btk_icon_factory_add (BtkIconFactory *factory,
                      const gchar    *stock_id,
                      BtkIconSet     *icon_set)
{
  gpointer old_key = NULL;
  gpointer old_value = NULL;

  g_return_if_fail (BTK_IS_ICON_FACTORY (factory));
  g_return_if_fail (stock_id != NULL);
  g_return_if_fail (icon_set != NULL);

  g_hash_table_lookup_extended (factory->icons, stock_id,
                                &old_key, &old_value);

  if (old_value == icon_set)
    return;

  btk_icon_set_ref (icon_set);

  /* GHashTable key memory management is so fantastically broken. */
  if (old_key)
    g_hash_table_insert (factory->icons, old_key, icon_set);
  else
    g_hash_table_insert (factory->icons, g_strdup (stock_id), icon_set);

  if (old_value)
    btk_icon_set_unref (old_value);
}

/**
 * btk_icon_factory_lookup:
 * @factory: a #BtkIconFactory
 * @stock_id: an icon name
 *
 * Looks up @stock_id in the icon factory, returning an icon set
 * if found, otherwise %NULL. For display to the user, you should
 * use btk_style_lookup_icon_set() on the #BtkStyle for the
 * widget that will display the icon, instead of using this
 * function directly, so that themes are taken into account.
 *
 * Return value: (transfer none): icon set of @stock_id.
 */
BtkIconSet *
btk_icon_factory_lookup (BtkIconFactory *factory,
                         const gchar    *stock_id)
{
  g_return_val_if_fail (BTK_IS_ICON_FACTORY (factory), NULL);
  g_return_val_if_fail (stock_id != NULL, NULL);

  return g_hash_table_lookup (factory->icons, stock_id);
}

static BtkIconFactory *btk_default_icons = NULL;
static GSList *default_factories = NULL;

/**
 * btk_icon_factory_add_default:
 * @factory: a #BtkIconFactory
 *
 * Adds an icon factory to the list of icon factories searched by
 * btk_style_lookup_icon_set(). This means that, for example,
 * btk_image_new_from_stock() will be able to find icons in @factory.
 * There will normally be an icon factory added for each library or
 * application that comes with icons. The default icon factories
 * can be overridden by themes.
 */
void
btk_icon_factory_add_default (BtkIconFactory *factory)
{
  g_return_if_fail (BTK_IS_ICON_FACTORY (factory));

  g_object_ref (factory);

  default_factories = g_slist_prepend (default_factories, factory);
}

/**
 * btk_icon_factory_remove_default:
 * @factory: a #BtkIconFactory previously added with btk_icon_factory_add_default()
 *
 * Removes an icon factory from the list of default icon
 * factories. Not normally used; you might use it for a library that
 * can be unloaded or shut down.
 */
void
btk_icon_factory_remove_default (BtkIconFactory  *factory)
{
  g_return_if_fail (BTK_IS_ICON_FACTORY (factory));

  default_factories = g_slist_remove (default_factories, factory);

  g_object_unref (factory);
}

void
_btk_icon_factory_ensure_default_icons (void)
{
  if (btk_default_icons == NULL)
    {
      btk_default_icons = btk_icon_factory_new ();

      get_default_icons (btk_default_icons);
    }
}

/**
 * btk_icon_factory_lookup_default:
 * @stock_id: an icon name
 *
 * Looks for an icon in the list of default icon factories.  For
 * display to the user, you should use btk_style_lookup_icon_set() on
 * the #BtkStyle for the widget that will display the icon, instead of
 * using this function directly, so that themes are taken into
 * account.
 *
 * Return value: (transfer none): a #BtkIconSet, or %NULL
 */
BtkIconSet *
btk_icon_factory_lookup_default (const gchar *stock_id)
{
  GSList *tmp_list;

  g_return_val_if_fail (stock_id != NULL, NULL);

  tmp_list = default_factories;
  while (tmp_list != NULL)
    {
      BtkIconSet *icon_set =
        btk_icon_factory_lookup (BTK_ICON_FACTORY (tmp_list->data),
                                 stock_id);

      if (icon_set)
        return icon_set;

      tmp_list = g_slist_next (tmp_list);
    }

  _btk_icon_factory_ensure_default_icons ();

  return btk_icon_factory_lookup (btk_default_icons, stock_id);
}

static void
register_stock_icon (BtkIconFactory *factory,
		     const gchar    *stock_id,
                     const gchar    *icon_name)
{
  BtkIconSet *set = btk_icon_set_new ();
  BtkIconSource source = BTK_ICON_SOURCE_INIT (TRUE, TRUE, TRUE);

  source.type = BTK_ICON_SOURCE_STATIC_ICON_NAME;
  source.source.icon_name = (gchar *)icon_name;
  source.direction = BTK_TEXT_DIR_NONE;
  btk_icon_set_add_source (set, &source);

  btk_icon_factory_add (factory, stock_id, set);
  btk_icon_set_unref (set);
}

static void
register_bidi_stock_icon (BtkIconFactory *factory,
			  const gchar    *stock_id,
                          const gchar    *icon_name)
{
  BtkIconSet *set = btk_icon_set_new ();
  BtkIconSource source = BTK_ICON_SOURCE_INIT (FALSE, TRUE, TRUE);

  source.type = BTK_ICON_SOURCE_STATIC_ICON_NAME;
  source.source.icon_name = (gchar *)icon_name;
  source.direction = BTK_TEXT_DIR_LTR;
  btk_icon_set_add_source (set, &source);

  source.type = BTK_ICON_SOURCE_STATIC_ICON_NAME;
  source.source.icon_name = (gchar *)icon_name;
  source.direction = BTK_TEXT_DIR_RTL;
  btk_icon_set_add_source (set, &source);

  btk_icon_factory_add (factory, stock_id, set);
  btk_icon_set_unref (set);
}

static void
get_default_icons (BtkIconFactory *factory)
{
  /* KEEP IN SYNC with btkstock.c */

  register_stock_icon (factory, BTK_STOCK_DIALOG_AUTHENTICATION, "dialog-password");
  register_stock_icon (factory, BTK_STOCK_DIALOG_ERROR, "dialog-error");
  register_stock_icon (factory, BTK_STOCK_DIALOG_INFO, "dialog-information");
  register_stock_icon (factory, BTK_STOCK_DIALOG_QUESTION, "dialog-question");
  register_stock_icon (factory, BTK_STOCK_DIALOG_WARNING, "dialog-warning");
  register_stock_icon (factory, BTK_STOCK_DND, BTK_STOCK_DND);
  register_stock_icon (factory, BTK_STOCK_DND_MULTIPLE, BTK_STOCK_DND_MULTIPLE);
  register_stock_icon (factory, BTK_STOCK_APPLY, BTK_STOCK_APPLY);
  register_stock_icon (factory, BTK_STOCK_CANCEL, BTK_STOCK_CANCEL);
  register_stock_icon (factory, BTK_STOCK_NO, BTK_STOCK_NO);
  register_stock_icon (factory, BTK_STOCK_OK, BTK_STOCK_OK);
  register_stock_icon (factory, BTK_STOCK_YES, BTK_STOCK_YES);
  register_stock_icon (factory, BTK_STOCK_CLOSE, "window-close");
  register_stock_icon (factory, BTK_STOCK_ADD, "list-add");
  register_stock_icon (factory, BTK_STOCK_JUSTIFY_CENTER, "format-justify-center");
  register_stock_icon (factory, BTK_STOCK_JUSTIFY_FILL, "format-justify-fill");
  register_stock_icon (factory, BTK_STOCK_JUSTIFY_LEFT, "format-justify-left");
  register_stock_icon (factory, BTK_STOCK_JUSTIFY_RIGHT, "format-justify-right");
  register_stock_icon (factory, BTK_STOCK_GOTO_BOTTOM, "go-bottom");
  register_stock_icon (factory, BTK_STOCK_CDROM, "media-optical");
  register_stock_icon (factory, BTK_STOCK_CONVERT, BTK_STOCK_CONVERT);
  register_stock_icon (factory, BTK_STOCK_COPY, "edit-copy");
  register_stock_icon (factory, BTK_STOCK_CUT, "edit-cut");
  register_stock_icon (factory, BTK_STOCK_GO_DOWN, "go-down");
  register_stock_icon (factory, BTK_STOCK_EXECUTE, "system-run");
  register_stock_icon (factory, BTK_STOCK_QUIT, "application-exit");
  register_bidi_stock_icon (factory, BTK_STOCK_GOTO_FIRST, "go-first");
  register_stock_icon (factory, BTK_STOCK_SELECT_FONT, BTK_STOCK_SELECT_FONT);
  register_stock_icon (factory, BTK_STOCK_FULLSCREEN, "view-fullscreen");
  register_stock_icon (factory, BTK_STOCK_LEAVE_FULLSCREEN, "view-restore");
  register_stock_icon (factory, BTK_STOCK_HARDDISK, "drive-harddisk");
  register_stock_icon (factory, BTK_STOCK_HELP, "help-contents");
  register_stock_icon (factory, BTK_STOCK_HOME, "go-home");
  register_stock_icon (factory, BTK_STOCK_INFO, "dialog-information");
  register_bidi_stock_icon (factory, BTK_STOCK_JUMP_TO, "go-jump");
  register_bidi_stock_icon (factory, BTK_STOCK_GOTO_LAST, "go-last");
  register_bidi_stock_icon (factory, BTK_STOCK_GO_BACK, "go-previous");
  register_stock_icon (factory, BTK_STOCK_MISSING_IMAGE, "image-missing");
  register_stock_icon (factory, BTK_STOCK_NETWORK, "network-idle");
  register_stock_icon (factory, BTK_STOCK_NEW, "document-new");
  register_stock_icon (factory, BTK_STOCK_OPEN, "document-open");
  register_stock_icon (factory, BTK_STOCK_ORIENTATION_PORTRAIT, BTK_STOCK_ORIENTATION_PORTRAIT);
  register_stock_icon (factory, BTK_STOCK_ORIENTATION_LANDSCAPE, BTK_STOCK_ORIENTATION_LANDSCAPE);
  register_stock_icon (factory, BTK_STOCK_ORIENTATION_REVERSE_PORTRAIT, BTK_STOCK_ORIENTATION_REVERSE_PORTRAIT);
  register_stock_icon (factory, BTK_STOCK_ORIENTATION_REVERSE_LANDSCAPE, BTK_STOCK_ORIENTATION_REVERSE_LANDSCAPE);
  register_stock_icon (factory, BTK_STOCK_PAGE_SETUP, BTK_STOCK_PAGE_SETUP);
  register_stock_icon (factory, BTK_STOCK_PASTE, "edit-paste");
  register_stock_icon (factory, BTK_STOCK_PREFERENCES, BTK_STOCK_PREFERENCES);
  register_stock_icon (factory, BTK_STOCK_PRINT, "document-print");
  register_stock_icon (factory, BTK_STOCK_PRINT_ERROR, "printer-error");
  register_stock_icon (factory, BTK_STOCK_PRINT_PAUSED, "printer-paused");
  register_stock_icon (factory, BTK_STOCK_PRINT_PREVIEW, "document-print-preview");
  register_stock_icon (factory, BTK_STOCK_PRINT_REPORT, "printer-info");
  register_stock_icon (factory, BTK_STOCK_PRINT_WARNING, "printer-warning");
  register_stock_icon (factory, BTK_STOCK_PROPERTIES, "document-properties");
  register_bidi_stock_icon (factory, BTK_STOCK_REDO, "edit-redo");
  register_stock_icon (factory, BTK_STOCK_REMOVE, "list-remove");
  register_stock_icon (factory, BTK_STOCK_REFRESH, "view-refresh");
  register_bidi_stock_icon (factory, BTK_STOCK_REVERT_TO_SAVED, "document-revert");
  register_bidi_stock_icon (factory, BTK_STOCK_GO_FORWARD, "go-next");
  register_stock_icon (factory, BTK_STOCK_SAVE, "document-save");
  register_stock_icon (factory, BTK_STOCK_FLOPPY, "media-floppy");
  register_stock_icon (factory, BTK_STOCK_SAVE_AS, "document-save-as");
  register_stock_icon (factory, BTK_STOCK_FIND, "edit-find");
  register_stock_icon (factory, BTK_STOCK_FIND_AND_REPLACE, "edit-find-replace");
  register_stock_icon (factory, BTK_STOCK_SORT_DESCENDING, "view-sort-descending");
  register_stock_icon (factory, BTK_STOCK_SORT_ASCENDING, "view-sort-ascending");
  register_stock_icon (factory, BTK_STOCK_SPELL_CHECK, "tools-check-spelling");
  register_stock_icon (factory, BTK_STOCK_STOP, "process-stop");
  register_stock_icon (factory, BTK_STOCK_BOLD, "format-text-bold");
  register_stock_icon (factory, BTK_STOCK_ITALIC, "format-text-italic");
  register_stock_icon (factory, BTK_STOCK_STRIKETHROUGH, "format-text-strikethrough");
  register_stock_icon (factory, BTK_STOCK_UNDERLINE, "format-text-underline");
  register_bidi_stock_icon (factory, BTK_STOCK_INDENT, "format-indent-more");
  register_bidi_stock_icon (factory, BTK_STOCK_UNINDENT, "format-indent-less");
  register_stock_icon (factory, BTK_STOCK_GOTO_TOP, "go-top");
  register_stock_icon (factory, BTK_STOCK_DELETE, "edit-delete");
  register_bidi_stock_icon (factory, BTK_STOCK_UNDELETE, BTK_STOCK_UNDELETE);
  register_bidi_stock_icon (factory, BTK_STOCK_UNDO, "edit-undo");
  register_stock_icon (factory, BTK_STOCK_GO_UP, "go-up");
  register_stock_icon (factory, BTK_STOCK_FILE, "text-x-generic");
  register_stock_icon (factory, BTK_STOCK_DIRECTORY, "folder");
  register_stock_icon (factory, BTK_STOCK_ABOUT, "help-about");
  register_stock_icon (factory, BTK_STOCK_CONNECT, BTK_STOCK_CONNECT);
  register_stock_icon (factory, BTK_STOCK_DISCONNECT, BTK_STOCK_DISCONNECT);
  register_stock_icon (factory, BTK_STOCK_EDIT, BTK_STOCK_EDIT);
  register_stock_icon (factory, BTK_STOCK_CAPS_LOCK_WARNING, BTK_STOCK_CAPS_LOCK_WARNING);
  register_bidi_stock_icon (factory, BTK_STOCK_MEDIA_FORWARD, "media-seek-forward");
  register_bidi_stock_icon (factory, BTK_STOCK_MEDIA_NEXT, "media-skip-forward");
  register_stock_icon (factory, BTK_STOCK_MEDIA_PAUSE, "media-playback-pause");
  register_bidi_stock_icon (factory, BTK_STOCK_MEDIA_PLAY, "media-playback-start");
  register_bidi_stock_icon (factory, BTK_STOCK_MEDIA_PREVIOUS, "media-skip-backward");
  register_stock_icon (factory, BTK_STOCK_MEDIA_RECORD, "media-record");
  register_bidi_stock_icon (factory, BTK_STOCK_MEDIA_REWIND, "media-seek-backward");
  register_stock_icon (factory, BTK_STOCK_MEDIA_STOP, "media-playback-stop");
  register_stock_icon (factory, BTK_STOCK_INDEX, BTK_STOCK_INDEX);
  register_stock_icon (factory, BTK_STOCK_ZOOM_100, "zoom-original");
  register_stock_icon (factory, BTK_STOCK_ZOOM_IN, "zoom-in");
  register_stock_icon (factory, BTK_STOCK_ZOOM_OUT, "zoom-out");
  register_stock_icon (factory, BTK_STOCK_ZOOM_FIT, "zoom-fit-best");
  register_stock_icon (factory, BTK_STOCK_SELECT_ALL, "edit-select-all");
  register_stock_icon (factory, BTK_STOCK_CLEAR, "edit-clear");
  register_stock_icon (factory, BTK_STOCK_SELECT_COLOR, BTK_STOCK_SELECT_COLOR);
  register_stock_icon (factory, BTK_STOCK_COLOR_PICKER, BTK_STOCK_COLOR_PICKER);
}

/************************************************************
 *                    Icon size handling                    *
 ************************************************************/

typedef struct _IconSize IconSize;

struct _IconSize
{
  gint size;
  gchar *name;

  gint width;
  gint height;
};

typedef struct _IconAlias IconAlias;

struct _IconAlias
{
  gchar *name;
  gint   target;
};

typedef struct _SettingsIconSize SettingsIconSize;

struct _SettingsIconSize
{
  gint width;
  gint height;
};

static GHashTable *icon_aliases = NULL;
static IconSize *icon_sizes = NULL;
static gint      icon_sizes_allocated = 0;
static gint      icon_sizes_used = 0;

static void
init_icon_sizes (void)
{
  if (icon_sizes == NULL)
    {
#define NUM_BUILTIN_SIZES 7
      gint i;

      icon_aliases = g_hash_table_new (g_str_hash, g_str_equal);

      icon_sizes = g_new (IconSize, NUM_BUILTIN_SIZES);
      icon_sizes_allocated = NUM_BUILTIN_SIZES;
      icon_sizes_used = NUM_BUILTIN_SIZES;

      icon_sizes[BTK_ICON_SIZE_INVALID].size = 0;
      icon_sizes[BTK_ICON_SIZE_INVALID].name = NULL;
      icon_sizes[BTK_ICON_SIZE_INVALID].width = 0;
      icon_sizes[BTK_ICON_SIZE_INVALID].height = 0;

      /* the name strings aren't copied since we don't ever remove
       * icon sizes, so we don't need to know whether they're static.
       * Even if we did I suppose removing the builtin sizes would be
       * disallowed.
       */

      icon_sizes[BTK_ICON_SIZE_MENU].size = BTK_ICON_SIZE_MENU;
      icon_sizes[BTK_ICON_SIZE_MENU].name = "btk-menu";
      icon_sizes[BTK_ICON_SIZE_MENU].width = 16;
      icon_sizes[BTK_ICON_SIZE_MENU].height = 16;

      icon_sizes[BTK_ICON_SIZE_BUTTON].size = BTK_ICON_SIZE_BUTTON;
      icon_sizes[BTK_ICON_SIZE_BUTTON].name = "btk-button";
      icon_sizes[BTK_ICON_SIZE_BUTTON].width = 20;
      icon_sizes[BTK_ICON_SIZE_BUTTON].height = 20;

      icon_sizes[BTK_ICON_SIZE_SMALL_TOOLBAR].size = BTK_ICON_SIZE_SMALL_TOOLBAR;
      icon_sizes[BTK_ICON_SIZE_SMALL_TOOLBAR].name = "btk-small-toolbar";
      icon_sizes[BTK_ICON_SIZE_SMALL_TOOLBAR].width = 18;
      icon_sizes[BTK_ICON_SIZE_SMALL_TOOLBAR].height = 18;

      icon_sizes[BTK_ICON_SIZE_LARGE_TOOLBAR].size = BTK_ICON_SIZE_LARGE_TOOLBAR;
      icon_sizes[BTK_ICON_SIZE_LARGE_TOOLBAR].name = "btk-large-toolbar";
      icon_sizes[BTK_ICON_SIZE_LARGE_TOOLBAR].width = 24;
      icon_sizes[BTK_ICON_SIZE_LARGE_TOOLBAR].height = 24;

      icon_sizes[BTK_ICON_SIZE_DND].size = BTK_ICON_SIZE_DND;
      icon_sizes[BTK_ICON_SIZE_DND].name = "btk-dnd";
      icon_sizes[BTK_ICON_SIZE_DND].width = 32;
      icon_sizes[BTK_ICON_SIZE_DND].height = 32;

      icon_sizes[BTK_ICON_SIZE_DIALOG].size = BTK_ICON_SIZE_DIALOG;
      icon_sizes[BTK_ICON_SIZE_DIALOG].name = "btk-dialog";
      icon_sizes[BTK_ICON_SIZE_DIALOG].width = 48;
      icon_sizes[BTK_ICON_SIZE_DIALOG].height = 48;

      g_assert ((BTK_ICON_SIZE_DIALOG + 1) == NUM_BUILTIN_SIZES);

      /* Alias everything to itself. */
      i = 1; /* skip invalid size */
      while (i < NUM_BUILTIN_SIZES)
        {
          btk_icon_size_register_alias (icon_sizes[i].name, icon_sizes[i].size);

          ++i;
        }

#undef NUM_BUILTIN_SIZES
    }
}

static void
free_settings_sizes (gpointer data)
{
  g_array_free (data, TRUE);
}

static GArray *
get_settings_sizes (BtkSettings *settings,
		    gboolean    *created)
{
  GArray *settings_sizes;
  static GQuark sizes_quark = 0;

  if (!sizes_quark)
    sizes_quark = g_quark_from_static_string ("btk-icon-sizes");

  settings_sizes = g_object_get_qdata (G_OBJECT (settings), sizes_quark);
  if (!settings_sizes)
    {
      settings_sizes = g_array_new (FALSE, FALSE, sizeof (SettingsIconSize));
      g_object_set_qdata_full (G_OBJECT (settings), sizes_quark,
			       settings_sizes, free_settings_sizes);
      if (created)
	*created = TRUE;
    }

  return settings_sizes;
}

static void
icon_size_set_for_settings (BtkSettings *settings,
			    const gchar *size_name,
			    gint         width,
			    gint         height)
{
  BtkIconSize size;
  GArray *settings_sizes;
  SettingsIconSize *settings_size;

  g_return_if_fail (size_name != NULL);

  size = btk_icon_size_from_name (size_name);
  if (size == BTK_ICON_SIZE_INVALID)
    /* Reserve a place */
    size = icon_size_register_intern (size_name, -1, -1);

  settings_sizes = get_settings_sizes (settings, NULL);
  if (size >= settings_sizes->len)
    {
      SettingsIconSize unset = { -1, -1 };
      gint i;

      for (i = settings_sizes->len; i <= size; i++)
	g_array_append_val (settings_sizes, unset);
    }

  settings_size = &g_array_index (settings_sizes, SettingsIconSize, size);

  settings_size->width = width;
  settings_size->height = height;
}

/* Like bango_parse_word, but accept - as well
 */
static gboolean
scan_icon_size_name (const char **pos, GString *out)
{
  const char *p = *pos;

  while (g_ascii_isspace (*p))
    p++;

  if (!((*p >= 'A' && *p <= 'Z') ||
	(*p >= 'a' && *p <= 'z') ||
	*p == '_' || *p == '-'))
    return FALSE;

  g_string_truncate (out, 0);
  g_string_append_c (out, *p);
  p++;

  while ((*p >= 'A' && *p <= 'Z') ||
	 (*p >= 'a' && *p <= 'z') ||
	 (*p >= '0' && *p <= '9') ||
	 *p == '_' || *p == '-')
    {
      g_string_append_c (out, *p);
      p++;
    }

  *pos = p;

  return TRUE;
}

static void
icon_size_setting_parse (BtkSettings *settings,
			 const gchar *icon_size_string)
{
  GString *name_buf = g_string_new (NULL);
  const gchar *p = icon_size_string;

  while (bango_skip_space (&p))
    {
      gint width, height;

      if (!scan_icon_size_name (&p, name_buf))
	goto err;

      if (!bango_skip_space (&p))
	goto err;

      if (*p != '=')
	goto err;

      p++;

      if (!bango_scan_int (&p, &width))
	goto err;

      if (!bango_skip_space (&p))
	goto err;

      if (*p != ',')
	goto err;

      p++;

      if (!bango_scan_int (&p, &height))
	goto err;

      if (width > 0 && height > 0)
	{
	  icon_size_set_for_settings (settings, name_buf->str,
				      width, height);
	}
      else
	{
	  g_warning ("Invalid size in btk-icon-sizes: %d,%d\n", width, height);
	}

      bango_skip_space (&p);
      if (*p == '\0')
	break;
      if (*p == ':')
	p++;
      else
	goto err;
    }

  g_string_free (name_buf, TRUE);
  return;

 err:
  g_warning ("Error parsing btk-icon-sizes string:\n\t'%s'", icon_size_string);
  g_string_free (name_buf, TRUE);
}

static void
icon_size_set_all_from_settings (BtkSettings *settings)
{
  GArray *settings_sizes;
  gchar *icon_size_string;

  /* Reset old settings */
  settings_sizes = get_settings_sizes (settings, NULL);
  g_array_set_size (settings_sizes, 0);

  g_object_get (settings,
		"btk-icon-sizes", &icon_size_string,
		NULL);

  if (icon_size_string)
    {
      icon_size_setting_parse (settings, icon_size_string);
      g_free (icon_size_string);
    }
}

static void
icon_size_settings_changed (BtkSettings  *settings,
			    GParamSpec   *pspec)
{
  icon_size_set_all_from_settings (settings);

  btk_rc_reset_styles (settings);
}

static void
icon_sizes_init_for_settings (BtkSettings *settings)
{
  g_signal_connect (settings,
		    "notify::btk-icon-sizes",
		    G_CALLBACK (icon_size_settings_changed),
		    NULL);

  icon_size_set_all_from_settings (settings);
}

static gboolean
icon_size_lookup_intern (BtkSettings *settings,
			 BtkIconSize  size,
			 gint        *widthp,
			 gint        *heightp)
{
  GArray *settings_sizes;
  gint width_for_settings = -1;
  gint height_for_settings = -1;

  init_icon_sizes ();

  if (size == (BtkIconSize)-1)
    return FALSE;

  if (size >= icon_sizes_used)
    return FALSE;

  if (size == BTK_ICON_SIZE_INVALID)
    return FALSE;

  if (settings)
    {
      gboolean initial = FALSE;

      settings_sizes = get_settings_sizes (settings, &initial);

      if (initial)
	icon_sizes_init_for_settings (settings);

      if (size < settings_sizes->len)
	{
	  SettingsIconSize *settings_size;

	  settings_size = &g_array_index (settings_sizes, SettingsIconSize, size);

	  width_for_settings = settings_size->width;
	  height_for_settings = settings_size->height;
	}
    }

  if (widthp)
    *widthp = width_for_settings >= 0 ? width_for_settings : icon_sizes[size].width;

  if (heightp)
    *heightp = height_for_settings >= 0 ? height_for_settings : icon_sizes[size].height;

  return TRUE;
}

/**
 * btk_icon_size_lookup_for_settings:
 * @settings: a #BtkSettings object, used to determine
 *   which set of user preferences to used.
 * @size: (type int): an icon size
 * @width: (out): location to store icon width
 * @height: (out): location to store icon height
 *
 * Obtains the pixel size of a semantic icon size, possibly
 * modified by user preferences for a particular
 * #BtkSettings. Normally @size would be
 * #BTK_ICON_SIZE_MENU, #BTK_ICON_SIZE_BUTTON, etc.  This function
 * isn't normally needed, btk_widget_render_icon() is the usual
 * way to get an icon for rendering, then just look at the size of
 * the rendered pixbuf. The rendered pixbuf may not even correspond to
 * the width/height returned by btk_icon_size_lookup(), because themes
 * are free to render the pixbuf however they like, including changing
 * the usual size.
 *
 * Return value: %TRUE if @size was a valid size
 *
 * Since: 2.2
 */
gboolean
btk_icon_size_lookup_for_settings (BtkSettings *settings,
				   BtkIconSize  size,
				   gint        *width,
				   gint        *height)
{
  g_return_val_if_fail (BTK_IS_SETTINGS (settings), FALSE);

  return icon_size_lookup_intern (settings, size, width, height);
}

/**
 * btk_icon_size_lookup:
 * @size: (type int): an icon size
 * @width: (out): location to store icon width
 * @height: (out): location to store icon height
 *
 * Obtains the pixel size of a semantic icon size, possibly
 * modified by user preferences for the default #BtkSettings.
 * (See btk_icon_size_lookup_for_settings().)
 * Normally @size would be
 * #BTK_ICON_SIZE_MENU, #BTK_ICON_SIZE_BUTTON, etc.  This function
 * isn't normally needed, btk_widget_render_icon() is the usual
 * way to get an icon for rendering, then just look at the size of
 * the rendered pixbuf. The rendered pixbuf may not even correspond to
 * the width/height returned by btk_icon_size_lookup(), because themes
 * are free to render the pixbuf however they like, including changing
 * the usual size.
 *
 * Return value: %TRUE if @size was a valid size
 */
gboolean
btk_icon_size_lookup (BtkIconSize  size,
                      gint        *widthp,
                      gint        *heightp)
{
  BTK_NOTE (MULTIHEAD,
	    g_warning ("btk_icon_size_lookup ()) is not multihead safe"));

  return btk_icon_size_lookup_for_settings (btk_settings_get_default (),
					    size, widthp, heightp);
}

static BtkIconSize
icon_size_register_intern (const gchar *name,
			   gint         width,
			   gint         height)
{
  IconAlias *old_alias;
  BtkIconSize size;

  init_icon_sizes ();

  old_alias = g_hash_table_lookup (icon_aliases, name);
  if (old_alias && icon_sizes[old_alias->target].width > 0)
    {
      g_warning ("Icon size name '%s' already exists", name);
      return BTK_ICON_SIZE_INVALID;
    }

  if (old_alias)
    {
      size = old_alias->target;
    }
  else
    {
      if (icon_sizes_used == icon_sizes_allocated)
	{
	  icon_sizes_allocated *= 2;
	  icon_sizes = g_renew (IconSize, icon_sizes, icon_sizes_allocated);
	}

      size = icon_sizes_used++;

      /* alias to self. */
      btk_icon_size_register_alias (name, size);

      icon_sizes[size].size = size;
      icon_sizes[size].name = g_strdup (name);
    }

  icon_sizes[size].width = width;
  icon_sizes[size].height = height;

  return size;
}

/**
 * btk_icon_size_register:
 * @name: name of the icon size
 * @width: the icon width
 * @height: the icon height
 *
 * Registers a new icon size, along the same lines as #BTK_ICON_SIZE_MENU,
 * etc. Returns the integer value for the size.
 *
 * Returns: (type int): integer value representing the size
 */
BtkIconSize
btk_icon_size_register (const gchar *name,
                        gint         width,
                        gint         height)
{
  g_return_val_if_fail (name != NULL, 0);
  g_return_val_if_fail (width > 0, 0);
  g_return_val_if_fail (height > 0, 0);

  return icon_size_register_intern (name, width, height);
}

/**
 * btk_icon_size_register_alias:
 * @alias: an alias for @target
 * @target: (type int): an existing icon size
 *
 * Registers @alias as another name for @target.
 * So calling btk_icon_size_from_name() with @alias as argument
 * will return @target.
 */
void
btk_icon_size_register_alias (const gchar *alias,
                              BtkIconSize  target)
{
  IconAlias *ia;

  g_return_if_fail (alias != NULL);

  init_icon_sizes ();

  if (!icon_size_lookup_intern (NULL, target, NULL, NULL))
    g_warning ("btk_icon_size_register_alias: Icon size %u does not exist", target);

  ia = g_hash_table_lookup (icon_aliases, alias);
  if (ia)
    {
      if (icon_sizes[ia->target].width > 0)
	{
	  g_warning ("btk_icon_size_register_alias: Icon size name '%s' already exists", alias);
	  return;
	}

      ia->target = target;
    }

  if (!ia)
    {
      ia = g_new (IconAlias, 1);
      ia->name = g_strdup (alias);
      ia->target = target;

      g_hash_table_insert (icon_aliases, ia->name, ia);
    }
}

/**
 * btk_icon_size_from_name:
 * @name: the name to look up.
 * @returns: the icon size with the given name.
 *
 * Looks up the icon size associated with @name.
 *
 * Return value: (type int): the icon size
 */
BtkIconSize
btk_icon_size_from_name (const gchar *name)
{
  IconAlias *ia;

  init_icon_sizes ();

  ia = g_hash_table_lookup (icon_aliases, name);

  if (ia && icon_sizes[ia->target].width > 0)
    return ia->target;
  else
    return BTK_ICON_SIZE_INVALID;
}

/**
 * btk_icon_size_get_name:
 * @size: (type int): a #BtkIconSize.
 * @returns: the name of the given icon size.
 *
 * Gets the canonical name of the given icon size. The returned string
 * is statically allocated and should not be freed.
 */
const gchar*
btk_icon_size_get_name (BtkIconSize  size)
{
  if (size >= icon_sizes_used)
    return NULL;
  else
    return icon_sizes[size].name;
}

/************************************************************/

/* Icon Set */


static BdkPixbuf *find_in_cache     (BtkIconSet       *icon_set,
                                     BtkStyle         *style,
                                     BtkTextDirection  direction,
                                     BtkStateType      state,
                                     BtkIconSize       size);
static void       add_to_cache      (BtkIconSet       *icon_set,
                                     BtkStyle         *style,
                                     BtkTextDirection  direction,
                                     BtkStateType      state,
                                     BtkIconSize       size,
                                     BdkPixbuf        *pixbuf);
/* Clear icon set contents, drop references to all contained
 * BdkPixbuf objects and forget all BtkIconSources. Used to
 * recycle an icon set.
 */
static void       clear_cache       (BtkIconSet       *icon_set,
                                     gboolean          style_detach);
static GSList*    copy_cache        (BtkIconSet       *icon_set,
                                     BtkIconSet       *copy_recipient);
static void       attach_to_style   (BtkIconSet       *icon_set,
                                     BtkStyle         *style);
static void       detach_from_style (BtkIconSet       *icon_set,
                                     BtkStyle         *style);
static void       style_dnotify     (gpointer          data);

struct _BtkIconSet
{
  guint ref_count;

  GSList *sources;

  /* Cache of the last few rendered versions of the icon. */
  GSList *cache;

  guint cache_size;

  guint cache_serial;
};

static guint cache_serial = 0;

/**
 * btk_icon_set_new:
 *
 * Creates a new #BtkIconSet. A #BtkIconSet represents a single icon
 * in various sizes and widget states. It can provide a #BdkPixbuf
 * for a given size and state on request, and automatically caches
 * some of the rendered #BdkPixbuf objects.
 *
 * Normally you would use btk_widget_render_icon() instead of
 * using #BtkIconSet directly. The one case where you'd use
 * #BtkIconSet is to create application-specific icon sets to place in
 * a #BtkIconFactory.
 *
 * Return value: a new #BtkIconSet
 */
BtkIconSet*
btk_icon_set_new (void)
{
  BtkIconSet *icon_set;

  icon_set = g_new (BtkIconSet, 1);

  icon_set->ref_count = 1;
  icon_set->sources = NULL;
  icon_set->cache = NULL;
  icon_set->cache_size = 0;
  icon_set->cache_serial = cache_serial;

  return icon_set;
}

/**
 * btk_icon_set_new_from_pixbuf:
 * @pixbuf: a #BdkPixbuf
 *
 * Creates a new #BtkIconSet with @pixbuf as the default/fallback
 * source image. If you don't add any additional #BtkIconSource to the
 * icon set, all variants of the icon will be created from @pixbuf,
 * using scaling, pixelation, etc. as required to adjust the icon size
 * or make the icon look insensitive/prelighted.
 *
 * Return value: a new #BtkIconSet
 */
BtkIconSet *
btk_icon_set_new_from_pixbuf (BdkPixbuf *pixbuf)
{
  BtkIconSet *set;

  BtkIconSource source = BTK_ICON_SOURCE_INIT (TRUE, TRUE, TRUE);

  g_return_val_if_fail (pixbuf != NULL, NULL);

  set = btk_icon_set_new ();

  btk_icon_source_set_pixbuf (&source, pixbuf);
  btk_icon_set_add_source (set, &source);
  btk_icon_source_set_pixbuf (&source, NULL);

  return set;
}


/**
 * btk_icon_set_ref:
 * @icon_set: a #BtkIconSet.
 *
 * Increments the reference count on @icon_set.
 *
 * Return value: @icon_set.
 */
BtkIconSet*
btk_icon_set_ref (BtkIconSet *icon_set)
{
  g_return_val_if_fail (icon_set != NULL, NULL);
  g_return_val_if_fail (icon_set->ref_count > 0, NULL);

  icon_set->ref_count += 1;

  return icon_set;
}

/**
 * btk_icon_set_unref:
 * @icon_set: a #BtkIconSet
 *
 * Decrements the reference count on @icon_set, and frees memory
 * if the reference count reaches 0.
 */
void
btk_icon_set_unref (BtkIconSet *icon_set)
{
  g_return_if_fail (icon_set != NULL);
  g_return_if_fail (icon_set->ref_count > 0);

  icon_set->ref_count -= 1;

  if (icon_set->ref_count == 0)
    {
      GSList *tmp_list = icon_set->sources;
      while (tmp_list != NULL)
        {
          btk_icon_source_free (tmp_list->data);

          tmp_list = g_slist_next (tmp_list);
        }
      g_slist_free (icon_set->sources);

      clear_cache (icon_set, TRUE);

      g_free (icon_set);
    }
}

GType
btk_icon_set_get_type (void)
{
  static GType our_type = 0;

  if (our_type == 0)
    our_type = g_boxed_type_register_static (I_("BtkIconSet"),
					     (GBoxedCopyFunc) btk_icon_set_ref,
					     (GBoxedFreeFunc) btk_icon_set_unref);

  return our_type;
}

/**
 * btk_icon_set_copy:
 * @icon_set: a #BtkIconSet
 *
 * Copies @icon_set by value.
 *
 * Return value: a new #BtkIconSet identical to the first.
 **/
BtkIconSet*
btk_icon_set_copy (BtkIconSet *icon_set)
{
  BtkIconSet *copy;
  GSList *tmp_list;

  copy = btk_icon_set_new ();

  tmp_list = icon_set->sources;
  while (tmp_list != NULL)
    {
      copy->sources = g_slist_prepend (copy->sources,
                                       btk_icon_source_copy (tmp_list->data));

      tmp_list = g_slist_next (tmp_list);
    }

  copy->sources = g_slist_reverse (copy->sources);

  copy->cache = copy_cache (icon_set, copy);
  copy->cache_size = icon_set->cache_size;
  copy->cache_serial = icon_set->cache_serial;

  return copy;
}

static gboolean
sizes_equivalent (BtkIconSize lhs,
                  BtkIconSize rhs)
{
  /* We used to consider sizes equivalent if they were
   * the same pixel size, but we don't have the BtkSettings
   * here, so we can't do that. Plus, it's not clear that
   * it is right... it was just a workaround for the fact
   * that we register icons by logical size, not pixel size.
   */
#if 1
  return lhs == rhs;
#else

  gint r_w, r_h, l_w, l_h;

  icon_size_lookup_intern (NULL, rhs, &r_w, &r_h);
  icon_size_lookup_intern (NULL, lhs, &l_w, &l_h);

  return r_w == l_w && r_h == l_h;
#endif
}

static BtkIconSource *
find_best_matching_source (BtkIconSet       *icon_set,
			   BtkTextDirection  direction,
			   BtkStateType      state,
			   BtkIconSize       size,
			   GSList           *failed)
{
  BtkIconSource *source;
  GSList *tmp_list;

  /* We need to find the best icon source.  Direction matters more
   * than state, state matters more than size. icon_set->sources
   * is sorted according to wildness, so if we take the first
   * match we find it will be the least-wild match (if there are
   * multiple matches for a given "wildness" then the RC file contained
   * dumb stuff, and we end up with an arbitrary matching source)
   */

  source = NULL;
  tmp_list = icon_set->sources;
  while (tmp_list != NULL)
    {
      BtkIconSource *s = tmp_list->data;

      if ((s->any_direction || (s->direction == direction)) &&
          (s->any_state || (s->state == state)) &&
          (s->any_size || size == (BtkIconSize)-1 || (sizes_equivalent (size, s->size))))
        {
	  if (!g_slist_find (failed, s))
	    {
	      source = s;
	      break;
	    }
	}

      tmp_list = g_slist_next (tmp_list);
    }

  return source;
}

static gboolean
ensure_filename_pixbuf (BtkIconSet    *icon_set,
			BtkIconSource *source)
{
  if (source->filename_pixbuf == NULL)
    {
      GError *error = NULL;

      source->filename_pixbuf = bdk_pixbuf_new_from_file (source->source.filename, &error);

      if (source->filename_pixbuf == NULL)
	{
	  /* Remove this icon source so we don't keep trying to
	   * load it.
	   */
	  g_warning (_("Error loading icon: %s"), error->message);
	  g_error_free (error);

	  icon_set->sources = g_slist_remove (icon_set->sources, source);

	  btk_icon_source_free (source);

	  return FALSE;
	}
    }

  return TRUE;
}

static BdkPixbuf *
render_icon_name_pixbuf (BtkIconSource    *icon_source,
			 BtkStyle         *style,
			 BtkTextDirection  direction,
			 BtkStateType      state,
			 BtkIconSize       size,
			 BtkWidget        *widget,
			 const char       *detail)
{
  BdkPixbuf *pixbuf;
  BdkPixbuf *tmp_pixbuf;
  BtkIconSource tmp_source;
  BdkScreen *screen;
  BtkIconTheme *icon_theme;
  BtkSettings *settings;
  gint width, height, pixel_size;
  gint *sizes, *s, dist;
  GError *error = NULL;

  if (widget && btk_widget_has_screen (widget))
    screen = btk_widget_get_screen (widget);
  else if (style && style->colormap)
    screen = bdk_colormap_get_screen (style->colormap);
  else
    {
      screen = bdk_screen_get_default ();
      BTK_NOTE (MULTIHEAD,
		g_warning ("Using the default screen for btk_icon_source_render_icon()"));
    }

  icon_theme = btk_icon_theme_get_for_screen (screen);
  settings = btk_settings_get_for_screen (screen);

  if (!btk_icon_size_lookup_for_settings (settings, size, &width, &height))
    {
      if (size == (BtkIconSize)-1)
	{
	  /* Find an available size close to 48 */
	  sizes = btk_icon_theme_get_icon_sizes (icon_theme, icon_source->source.icon_name);
	  dist = 1000;
	  width = height = 48;
	  for (s = sizes; *s; s++)
	    {
	      if (*s == -1)
		{
		  width = height = 48;
		  break;
		}
	      if (*s < 48)
		{
		  if (48 - *s < dist)
		    {
		      width = height = *s;
		      dist = 48 - *s;
		    }
		}
	      else
		{
		  if (*s - 48 < dist)
		    {
		      width = height = *s;
		      dist = *s - 48;
		    }
		}
	    }

	  g_free (sizes);
	}
      else
	{
	  g_warning ("Invalid icon size %u\n", size);
	  width = height = 24;
	}
    }

  pixel_size = MIN (width, height);

  if (icon_source->direction != BTK_TEXT_DIR_NONE)
    {
      gchar *suffix[3] = { NULL, "-ltr", "-rtl" };
      const gchar *names[3];
      gchar *name_with_dir;
      BtkIconInfo *info;

      name_with_dir = g_strconcat (icon_source->source.icon_name, suffix[icon_source->direction], NULL);
      names[0] = name_with_dir;
      names[1] = icon_source->source.icon_name;
      names[2] = NULL;

      info = btk_icon_theme_choose_icon (icon_theme,
                                         names,
                                         pixel_size, BTK_ICON_LOOKUP_USE_BUILTIN);
      g_free (name_with_dir);
      if (info)
        {
          tmp_pixbuf = btk_icon_info_load_icon (info, &error);
          btk_icon_info_free (info);
        }
      else
        tmp_pixbuf = NULL;
    }
  else
    {
      tmp_pixbuf = btk_icon_theme_load_icon (icon_theme,
                                             icon_source->source.icon_name,
                                             pixel_size, 0,
                                             &error);
    }

  if (!tmp_pixbuf)
    {
      g_warning ("Error loading theme icon '%s' for stock: %s",
                 icon_source->source.icon_name, error ? error->message : "");
      if (error)
        g_error_free (error);
      return NULL;
    }

  tmp_source = *icon_source;
  tmp_source.type = BTK_ICON_SOURCE_PIXBUF;
  tmp_source.source.pixbuf = tmp_pixbuf;

  pixbuf = btk_style_render_icon (style, &tmp_source,
				  direction, state, -1,
				  widget, detail);

  if (!pixbuf)
    g_warning ("Failed to render icon");

  g_object_unref (tmp_pixbuf);

  return pixbuf;
}

static BdkPixbuf *
find_and_render_icon_source (BtkIconSet       *icon_set,
			     BtkStyle         *style,
			     BtkTextDirection  direction,
			     BtkStateType      state,
			     BtkIconSize       size,
			     BtkWidget         *widget,
			     const char        *detail)
{
  GSList *failed = NULL;
  BdkPixbuf *pixbuf = NULL;

  /* We treat failure in two different ways:
   *
   *  A) If loading a source that specifies a filename fails,
   *     we treat that as permanent, and remove the source
   *     from the BtkIconSet. (in ensure_filename_pixbuf ()
   *  B) If loading a themed icon fails, or scaling an icon
   *     fails, we treat that as transient and will try
   *     again next time the icon falls out of the cache
   *     and we need to recreate it.
   */
  while (pixbuf == NULL)
    {
      BtkIconSource *source = find_best_matching_source (icon_set, direction, state, size, failed);

      if (source == NULL)
	break;

      switch (source->type)
	{
	case BTK_ICON_SOURCE_FILENAME:
	  if (!ensure_filename_pixbuf (icon_set, source))
	    break;
	  /* Fall through */
	case BTK_ICON_SOURCE_PIXBUF:
	  pixbuf = btk_style_render_icon (style, source,
					  direction, state, size,
					  widget, detail);
	  if (!pixbuf)
	    {
	      g_warning ("Failed to render icon");
	      failed = g_slist_prepend (failed, source);
	    }
	  break;
	case BTK_ICON_SOURCE_ICON_NAME:
	case BTK_ICON_SOURCE_STATIC_ICON_NAME:
	  pixbuf = render_icon_name_pixbuf (source, style,
					    direction, state, size,
					    widget, detail);
	  if (!pixbuf)
	    failed = g_slist_prepend (failed, source);
	  break;
	case BTK_ICON_SOURCE_EMPTY:
	  g_assert_not_reached ();
	}
    }

  g_slist_free (failed);

  return pixbuf;
}

extern BtkIconCache *_builtin_cache;

static BdkPixbuf*
render_fallback_image (BtkStyle          *style,
                       BtkTextDirection   direction,
                       BtkStateType       state,
                       BtkIconSize        size,
                       BtkWidget         *widget,
                       const char        *detail)
{
  /* This icon can be used for any direction/state/size */
  static BtkIconSource fallback_source = BTK_ICON_SOURCE_INIT (TRUE, TRUE, TRUE);

  if (fallback_source.type == BTK_ICON_SOURCE_EMPTY)
    {
      gint index;
      BdkPixbuf *pixbuf;

      _btk_icon_theme_ensure_builtin_cache ();

      index = _btk_icon_cache_get_directory_index (_builtin_cache, "24");
      pixbuf = _btk_icon_cache_get_icon (_builtin_cache, "image-missing", index);

      g_return_val_if_fail(pixbuf != NULL, NULL);

      btk_icon_source_set_pixbuf (&fallback_source, pixbuf);
      g_object_unref (pixbuf);
    }

  return btk_style_render_icon (style,
                                &fallback_source,
                                direction,
                                state,
                                size,
                                widget,
                                detail);
}

/**
 * btk_icon_set_render_icon:
 * @icon_set: a #BtkIconSet
 * @style: (allow-none): a #BtkStyle associated with @widget, or %NULL
 * @direction: text direction
 * @state: widget state
 * @size: (type int): icon size. A size of (BtkIconSize)-1
 *        means render at the size of the source and don't scale.
 * @widget: (allow-none): widget that will display the icon, or %NULL.
 *          The only use that is typically made of this
 *          is to determine the appropriate #BdkScreen.
 * @detail: (allow-none): detail to pass to the theme engine, or %NULL.
 *          Note that passing a detail of anything but %NULL
 *          will disable caching.
 *
 * Renders an icon using btk_style_render_icon(). In most cases,
 * btk_widget_render_icon() is better, since it automatically provides
 * most of the arguments from the current widget settings.  This
 * function never returns %NULL; if the icon can't be rendered
 * (perhaps because an image file fails to load), a default "missing
 * image" icon will be returned instead.
 *
 * Return value: (transfer full): a #BdkPixbuf to be displayed
 */
BdkPixbuf*
btk_icon_set_render_icon (BtkIconSet        *icon_set,
                          BtkStyle          *style,
                          BtkTextDirection   direction,
                          BtkStateType       state,
                          BtkIconSize        size,
                          BtkWidget         *widget,
                          const char        *detail)
{
  BdkPixbuf *icon;

  g_return_val_if_fail (icon_set != NULL, NULL);
  g_return_val_if_fail (style == NULL || BTK_IS_STYLE (style), NULL);

  if (icon_set->sources == NULL)
    return render_fallback_image (style, direction, state, size, widget, detail);

  if (detail == NULL)
    {
      icon = find_in_cache (icon_set, style, direction,
                        state, size);

      if (icon)
	{
	  g_object_ref (icon);
	  return icon;
	}
    }


  icon = find_and_render_icon_source (icon_set, style, direction, state, size,
				      widget, detail);

  if (icon == NULL)
    icon = render_fallback_image (style, direction, state, size, widget, detail);

  if (detail == NULL)
    add_to_cache (icon_set, style, direction, state, size, icon);

  return icon;
}

/* Order sources by their "wildness", so that "wilder" sources are
 * greater than "specific" sources; for determining ordering,
 * direction beats state beats size.
 */

static int
icon_source_compare (gconstpointer ap, gconstpointer bp)
{
  const BtkIconSource *a = ap;
  const BtkIconSource *b = bp;

  if (!a->any_direction && b->any_direction)
    return -1;
  else if (a->any_direction && !b->any_direction)
    return 1;
  else if (!a->any_state && b->any_state)
    return -1;
  else if (a->any_state && !b->any_state)
    return 1;
  else if (!a->any_size && b->any_size)
    return -1;
  else if (a->any_size && !b->any_size)
    return 1;
  else
    return 0;
}

/**
 * btk_icon_set_add_source:
 * @icon_set: a #BtkIconSet
 * @source: a #BtkIconSource
 *
 * Icon sets have a list of #BtkIconSource, which they use as base
 * icons for rendering icons in different states and sizes. Icons are
 * scaled, made to look insensitive, etc. in
 * btk_icon_set_render_icon(), but #BtkIconSet needs base images to
 * work with. The base images and when to use them are described by
 * a #BtkIconSource.
 *
 * This function copies @source, so you can reuse the same source immediately
 * without affecting the icon set.
 *
 * An example of when you'd use this function: a web browser's "Back
 * to Previous Page" icon might point in a different direction in
 * Hebrew and in English; it might look different when insensitive;
 * and it might change size depending on toolbar mode (small/large
 * icons). So a single icon set would contain all those variants of
 * the icon, and you might add a separate source for each one.
 *
 * You should nearly always add a "default" icon source with all
 * fields wildcarded, which will be used as a fallback if no more
 * specific source matches. #BtkIconSet always prefers more specific
 * icon sources to more generic icon sources. The order in which you
 * add the sources to the icon set does not matter.
 *
 * btk_icon_set_new_from_pixbuf() creates a new icon set with a
 * default icon source based on the given pixbuf.
 */
void
btk_icon_set_add_source (BtkIconSet          *icon_set,
                         const BtkIconSource *source)
{
  g_return_if_fail (icon_set != NULL);
  g_return_if_fail (source != NULL);

  if (source->type == BTK_ICON_SOURCE_EMPTY)
    {
      g_warning ("Useless empty BtkIconSource");
      return;
    }

  icon_set->sources = g_slist_insert_sorted (icon_set->sources,
                                             btk_icon_source_copy (source),
                                             icon_source_compare);
}

/**
 * btk_icon_set_get_sizes:
 * @icon_set: a #BtkIconSet
 * @sizes: (array length=n_sizes) (out) (type int): return location
 *     for array of sizes
 * @n_sizes: location to store number of elements in returned array
 *
 * Obtains a list of icon sizes this icon set can render. The returned
 * array must be freed with g_free().
 */
void
btk_icon_set_get_sizes (BtkIconSet   *icon_set,
                        BtkIconSize **sizes,
                        gint         *n_sizes)
{
  GSList *tmp_list;
  gboolean all_sizes = FALSE;
  GSList *specifics = NULL;

  g_return_if_fail (icon_set != NULL);
  g_return_if_fail (sizes != NULL);
  g_return_if_fail (n_sizes != NULL);

  tmp_list = icon_set->sources;
  while (tmp_list != NULL)
    {
      BtkIconSource *source;

      source = tmp_list->data;

      if (source->any_size)
        {
          all_sizes = TRUE;
          break;
        }
      else
        specifics = g_slist_prepend (specifics, GINT_TO_POINTER (source->size));

      tmp_list = g_slist_next (tmp_list);
    }

  if (all_sizes)
    {
      /* Need to find out what sizes exist */
      gint i;

      init_icon_sizes ();

      *sizes = g_new (BtkIconSize, icon_sizes_used);
      *n_sizes = icon_sizes_used - 1;

      i = 1;
      while (i < icon_sizes_used)
        {
          (*sizes)[i - 1] = icon_sizes[i].size;
          ++i;
        }
    }
  else
    {
      gint i;

      *n_sizes = g_slist_length (specifics);
      *sizes = g_new (BtkIconSize, *n_sizes);

      i = 0;
      tmp_list = specifics;
      while (tmp_list != NULL)
        {
          (*sizes)[i] = GPOINTER_TO_INT (tmp_list->data);

          ++i;
          tmp_list = g_slist_next (tmp_list);
        }
    }

  g_slist_free (specifics);
}


/**
 * btk_icon_source_new:
 *
 * Creates a new #BtkIconSource. A #BtkIconSource contains a #BdkPixbuf (or
 * image filename) that serves as the base image for one or more of the
 * icons in a #BtkIconSet, along with a specification for which icons in the
 * icon set will be based on that pixbuf or image file. An icon set contains
 * a set of icons that represent "the same" logical concept in different states,
 * different global text directions, and different sizes.
 *
 * So for example a web browser's "Back to Previous Page" icon might
 * point in a different direction in Hebrew and in English; it might
 * look different when insensitive; and it might change size depending
 * on toolbar mode (small/large icons). So a single icon set would
 * contain all those variants of the icon. #BtkIconSet contains a list
 * of #BtkIconSource from which it can derive specific icon variants in
 * the set.
 *
 * In the simplest case, #BtkIconSet contains one source pixbuf from
 * which it derives all variants. The convenience function
 * btk_icon_set_new_from_pixbuf() handles this case; if you only have
 * one source pixbuf, just use that function.
 *
 * If you want to use a different base pixbuf for different icon
 * variants, you create multiple icon sources, mark which variants
 * they'll be used to create, and add them to the icon set with
 * btk_icon_set_add_source().
 *
 * By default, the icon source has all parameters wildcarded. That is,
 * the icon source will be used as the base icon for any desired text
 * direction, widget state, or icon size.
 *
 * Return value: a new #BtkIconSource
 */
BtkIconSource*
btk_icon_source_new (void)
{
  BtkIconSource *src;

  src = g_new0 (BtkIconSource, 1);

  src->direction = BTK_TEXT_DIR_NONE;
  src->size = BTK_ICON_SIZE_INVALID;
  src->state = BTK_STATE_NORMAL;

  src->any_direction = TRUE;
  src->any_state = TRUE;
  src->any_size = TRUE;

  return src;
}

/**
 * btk_icon_source_copy:
 * @source: a #BtkIconSource
 *
 * Creates a copy of @source; mostly useful for language bindings.
 *
 * Return value: a new #BtkIconSource
 */
BtkIconSource*
btk_icon_source_copy (const BtkIconSource *source)
{
  BtkIconSource *copy;

  g_return_val_if_fail (source != NULL, NULL);

  copy = g_new (BtkIconSource, 1);

  *copy = *source;

  switch (copy->type)
    {
    case BTK_ICON_SOURCE_EMPTY:
    case BTK_ICON_SOURCE_STATIC_ICON_NAME:
      break;
    case BTK_ICON_SOURCE_ICON_NAME:
      copy->source.icon_name = g_strdup (copy->source.icon_name);
      break;
    case BTK_ICON_SOURCE_FILENAME:
      copy->source.filename = g_strdup (copy->source.filename);
#if defined (G_OS_WIN32) && !defined (_WIN64)
      copy->cp_filename = g_strdup (copy->cp_filename);
#endif
      if (copy->filename_pixbuf)
	g_object_ref (copy->filename_pixbuf);
      break;
    case BTK_ICON_SOURCE_PIXBUF:
      g_object_ref (copy->source.pixbuf);
      break;
    default:
      g_assert_not_reached();
    }

  return copy;
}

/**
 * btk_icon_source_free:
 * @source: a #BtkIconSource
 *
 * Frees a dynamically-allocated icon source, along with its
 * filename, size, and pixbuf fields if those are not %NULL.
 */
void
btk_icon_source_free (BtkIconSource *source)
{
  g_return_if_fail (source != NULL);

  icon_source_clear (source);
  g_free (source);
}

GType
btk_icon_source_get_type (void)
{
  static GType our_type = 0;

  if (our_type == 0)
    our_type = g_boxed_type_register_static (I_("BtkIconSource"),
					     (GBoxedCopyFunc) btk_icon_source_copy,
					     (GBoxedFreeFunc) btk_icon_source_free);

  return our_type;
}

static void
icon_source_clear (BtkIconSource *source)
{
  switch (source->type)
    {
    case BTK_ICON_SOURCE_EMPTY:
      break;
    case BTK_ICON_SOURCE_ICON_NAME:
      g_free (source->source.icon_name);
      /* fall thru */
    case BTK_ICON_SOURCE_STATIC_ICON_NAME:
      source->source.icon_name = NULL;
      break;
    case BTK_ICON_SOURCE_FILENAME:
      g_free (source->source.filename);
      source->source.filename = NULL;
#if defined (G_OS_WIN32) && !defined (_WIN64)
      g_free (source->cp_filename);
      source->cp_filename = NULL;
#endif
      if (source->filename_pixbuf) 
	g_object_unref (source->filename_pixbuf);
      source->filename_pixbuf = NULL;
      break;
    case BTK_ICON_SOURCE_PIXBUF:
      g_object_unref (source->source.pixbuf);
      source->source.pixbuf = NULL;
      break;
    default:
      g_assert_not_reached();
    }

  source->type = BTK_ICON_SOURCE_EMPTY;
}

/**
 * btk_icon_source_set_filename:
 * @source: a #BtkIconSource
 * @filename: image file to use
 *
 * Sets the name of an image file to use as a base image when creating
 * icon variants for #BtkIconSet. The filename must be absolute.
 */
void
btk_icon_source_set_filename (BtkIconSource *source,
			      const gchar   *filename)
{
  g_return_if_fail (source != NULL);
  g_return_if_fail (filename == NULL || g_path_is_absolute (filename));

  if (source->type == BTK_ICON_SOURCE_FILENAME &&
      source->source.filename == filename)
    return;

  icon_source_clear (source);

  if (filename != NULL)
    {
      source->type = BTK_ICON_SOURCE_FILENAME;
      source->source.filename = g_strdup (filename);
#if defined (G_OS_WIN32) && !defined (_WIN64)
      source->cp_filename = g_locale_from_utf8 (filename, -1, NULL, NULL, NULL);
#endif
    }
}

/**
 * btk_icon_source_set_icon_name
 * @source: a #BtkIconSource
 * @icon_name: (allow-none): name of icon to use
 *
 * Sets the name of an icon to look up in the current icon theme
 * to use as a base image when creating icon variants for #BtkIconSet.
 */
void
btk_icon_source_set_icon_name (BtkIconSource *source,
			       const gchar   *icon_name)
{
  g_return_if_fail (source != NULL);

  if (source->type == BTK_ICON_SOURCE_ICON_NAME &&
      source->source.icon_name == icon_name)
    return;

  icon_source_clear (source);

  if (icon_name != NULL)
    {
      source->type = BTK_ICON_SOURCE_ICON_NAME;
      source->source.icon_name = g_strdup (icon_name);
    }
}

/**
 * btk_icon_source_set_pixbuf:
 * @source: a #BtkIconSource
 * @pixbuf: pixbuf to use as a source
 *
 * Sets a pixbuf to use as a base image when creating icon variants
 * for #BtkIconSet.
 */
void
btk_icon_source_set_pixbuf (BtkIconSource *source,
                            BdkPixbuf     *pixbuf)
{
  g_return_if_fail (source != NULL);
  g_return_if_fail (pixbuf == NULL || BDK_IS_PIXBUF (pixbuf));

  if (source->type == BTK_ICON_SOURCE_PIXBUF &&
      source->source.pixbuf == pixbuf)
    return;

  icon_source_clear (source);

  if (pixbuf != NULL)
    {
      source->type = BTK_ICON_SOURCE_PIXBUF;
      source->source.pixbuf = g_object_ref (pixbuf);
    }
}

/**
 * btk_icon_source_get_filename:
 * @source: a #BtkIconSource
 *
 * Retrieves the source filename, or %NULL if none is set. The
 * filename is not a copy, and should not be modified or expected to
 * persist beyond the lifetime of the icon source.
 *
 * Return value: image filename. This string must not be modified
 * or freed.
 */
const gchar*
btk_icon_source_get_filename (const BtkIconSource *source)
{
  g_return_val_if_fail (source != NULL, NULL);

  if (source->type == BTK_ICON_SOURCE_FILENAME)
    return source->source.filename;
  else
    return NULL;
}

/**
 * btk_icon_source_get_icon_name:
 * @source: a #BtkIconSource
 *
 * Retrieves the source icon name, or %NULL if none is set. The
 * icon_name is not a copy, and should not be modified or expected to
 * persist beyond the lifetime of the icon source.
 *
 * Return value: icon name. This string must not be modified or freed.
 */
const gchar*
btk_icon_source_get_icon_name (const BtkIconSource *source)
{
  g_return_val_if_fail (source != NULL, NULL);

  if (source->type == BTK_ICON_SOURCE_ICON_NAME ||
     source->type == BTK_ICON_SOURCE_STATIC_ICON_NAME)
    return source->source.icon_name;
  else
    return NULL;
}

/**
 * btk_icon_source_get_pixbuf:
 * @source: a #BtkIconSource
 *
 * Retrieves the source pixbuf, or %NULL if none is set.
 * In addition, if a filename source is in use, this
 * function in some cases will return the pixbuf from
 * loaded from the filename. This is, for example, true
 * for the BtkIconSource passed to the BtkStyle::render_icon()
 * virtual function. The reference count on the pixbuf is
 * not incremented.
 *
 * Return value: (transfer none): source pixbuf
 */
BdkPixbuf*
btk_icon_source_get_pixbuf (const BtkIconSource *source)
{
  g_return_val_if_fail (source != NULL, NULL);

  if (source->type == BTK_ICON_SOURCE_PIXBUF)
    return source->source.pixbuf;
  else if (source->type == BTK_ICON_SOURCE_FILENAME)
    return source->filename_pixbuf;
  else
    return NULL;
}

/**
 * btk_icon_source_set_direction_wildcarded:
 * @source: a #BtkIconSource
 * @setting: %TRUE to wildcard the text direction
 *
 * If the text direction is wildcarded, this source can be used
 * as the base image for an icon in any #BtkTextDirection.
 * If the text direction is not wildcarded, then the
 * text direction the icon source applies to should be set
 * with btk_icon_source_set_direction(), and the icon source
 * will only be used with that text direction.
 *
 * #BtkIconSet prefers non-wildcarded sources (exact matches) over
 * wildcarded sources, and will use an exact match when possible.
 */
void
btk_icon_source_set_direction_wildcarded (BtkIconSource *source,
                                          gboolean       setting)
{
  g_return_if_fail (source != NULL);

  source->any_direction = setting != FALSE;
}

/**
 * btk_icon_source_set_state_wildcarded:
 * @source: a #BtkIconSource
 * @setting: %TRUE to wildcard the widget state
 *
 * If the widget state is wildcarded, this source can be used as the
 * base image for an icon in any #BtkStateType.  If the widget state
 * is not wildcarded, then the state the source applies to should be
 * set with btk_icon_source_set_state() and the icon source will
 * only be used with that specific state.
 *
 * #BtkIconSet prefers non-wildcarded sources (exact matches) over
 * wildcarded sources, and will use an exact match when possible.
 *
 * #BtkIconSet will normally transform wildcarded source images to
 * produce an appropriate icon for a given state, for example
 * lightening an image on prelight, but will not modify source images
 * that match exactly.
 */
void
btk_icon_source_set_state_wildcarded (BtkIconSource *source,
                                      gboolean       setting)
{
  g_return_if_fail (source != NULL);

  source->any_state = setting != FALSE;
}


/**
 * btk_icon_source_set_size_wildcarded:
 * @source: a #BtkIconSource
 * @setting: %TRUE to wildcard the widget state
 *
 * If the icon size is wildcarded, this source can be used as the base
 * image for an icon of any size.  If the size is not wildcarded, then
 * the size the source applies to should be set with
 * btk_icon_source_set_size() and the icon source will only be used
 * with that specific size.
 *
 * #BtkIconSet prefers non-wildcarded sources (exact matches) over
 * wildcarded sources, and will use an exact match when possible.
 *
 * #BtkIconSet will normally scale wildcarded source images to produce
 * an appropriate icon at a given size, but will not change the size
 * of source images that match exactly.
 */
void
btk_icon_source_set_size_wildcarded (BtkIconSource *source,
                                     gboolean       setting)
{
  g_return_if_fail (source != NULL);

  source->any_size = setting != FALSE;
}

/**
 * btk_icon_source_get_size_wildcarded:
 * @source: a #BtkIconSource
 *
 * Gets the value set by btk_icon_source_set_size_wildcarded().
 *
 * Return value: %TRUE if this icon source is a base for any icon size variant
 */
gboolean
btk_icon_source_get_size_wildcarded (const BtkIconSource *source)
{
  g_return_val_if_fail (source != NULL, TRUE);

  return source->any_size;
}

/**
 * btk_icon_source_get_state_wildcarded:
 * @source: a #BtkIconSource
 *
 * Gets the value set by btk_icon_source_set_state_wildcarded().
 *
 * Return value: %TRUE if this icon source is a base for any widget state variant
 */
gboolean
btk_icon_source_get_state_wildcarded (const BtkIconSource *source)
{
  g_return_val_if_fail (source != NULL, TRUE);

  return source->any_state;
}

/**
 * btk_icon_source_get_direction_wildcarded:
 * @source: a #BtkIconSource
 *
 * Gets the value set by btk_icon_source_set_direction_wildcarded().
 *
 * Return value: %TRUE if this icon source is a base for any text direction variant
 */
gboolean
btk_icon_source_get_direction_wildcarded (const BtkIconSource *source)
{
  g_return_val_if_fail (source != NULL, TRUE);

  return source->any_direction;
}

/**
 * btk_icon_source_set_direction:
 * @source: a #BtkIconSource
 * @direction: text direction this source applies to
 *
 * Sets the text direction this icon source is intended to be used
 * with.
 *
 * Setting the text direction on an icon source makes no difference
 * if the text direction is wildcarded. Therefore, you should usually
 * call btk_icon_source_set_direction_wildcarded() to un-wildcard it
 * in addition to calling this function.
 */
void
btk_icon_source_set_direction (BtkIconSource   *source,
                               BtkTextDirection direction)
{
  g_return_if_fail (source != NULL);

  source->direction = direction;
}

/**
 * btk_icon_source_set_state:
 * @source: a #BtkIconSource
 * @state: widget state this source applies to
 *
 * Sets the widget state this icon source is intended to be used
 * with.
 *
 * Setting the widget state on an icon source makes no difference
 * if the state is wildcarded. Therefore, you should usually
 * call btk_icon_source_set_state_wildcarded() to un-wildcard it
 * in addition to calling this function.
 */
void
btk_icon_source_set_state (BtkIconSource *source,
                           BtkStateType   state)
{
  g_return_if_fail (source != NULL);

  source->state = state;
}

/**
 * btk_icon_source_set_size:
 * @source: a #BtkIconSource
 * @size: (type int): icon size this source applies to
 *
 * Sets the icon size this icon source is intended to be used
 * with.
 *
 * Setting the icon size on an icon source makes no difference
 * if the size is wildcarded. Therefore, you should usually
 * call btk_icon_source_set_size_wildcarded() to un-wildcard it
 * in addition to calling this function.
 */
void
btk_icon_source_set_size (BtkIconSource *source,
                          BtkIconSize    size)
{
  g_return_if_fail (source != NULL);

  source->size = size;
}

/**
 * btk_icon_source_get_direction:
 * @source: a #BtkIconSource
 *
 * Obtains the text direction this icon source applies to. The return
 * value is only useful/meaningful if the text direction is <emphasis>not</emphasis>
 * wildcarded.
 *
 * Return value: text direction this source matches
 */
BtkTextDirection
btk_icon_source_get_direction (const BtkIconSource *source)
{
  g_return_val_if_fail (source != NULL, 0);

  return source->direction;
}

/**
 * btk_icon_source_get_state:
 * @source: a #BtkIconSource
 *
 * Obtains the widget state this icon source applies to. The return
 * value is only useful/meaningful if the widget state is <emphasis>not</emphasis>
 * wildcarded.
 *
 * Return value: widget state this source matches
 */
BtkStateType
btk_icon_source_get_state (const BtkIconSource *source)
{
  g_return_val_if_fail (source != NULL, 0);

  return source->state;
}

/**
 * btk_icon_source_get_size:
 * @source: a #BtkIconSource
 *
 * Obtains the icon size this source applies to. The return value
 * is only useful/meaningful if the icon size is <emphasis>not</emphasis> wildcarded.
 *
 * Return value: (type int): icon size this source matches.
 */
BtkIconSize
btk_icon_source_get_size (const BtkIconSource *source)
{
  g_return_val_if_fail (source != NULL, 0);

  return source->size;
}

#define NUM_CACHED_ICONS 8

typedef struct _CachedIcon CachedIcon;

struct _CachedIcon
{
  /* These must all match to use the cached pixbuf.
   * If any don't match, we must re-render the pixbuf.
   */
  BtkStyle *style;
  BtkTextDirection direction;
  BtkStateType state;
  BtkIconSize size;

  BdkPixbuf *pixbuf;
};

static void
ensure_cache_up_to_date (BtkIconSet *icon_set)
{
  if (icon_set->cache_serial != cache_serial)
    {
      clear_cache (icon_set, TRUE);
      icon_set->cache_serial = cache_serial;
    }
}

static void
cached_icon_free (CachedIcon *icon)
{
  g_object_unref (icon->pixbuf);

  if (icon->style)
    g_object_unref (icon->style);

  g_free (icon);
}

static BdkPixbuf *
find_in_cache (BtkIconSet      *icon_set,
               BtkStyle        *style,
               BtkTextDirection direction,
               BtkStateType     state,
               BtkIconSize      size)
{
  GSList *tmp_list;
  GSList *prev;

  ensure_cache_up_to_date (icon_set);

  prev = NULL;
  tmp_list = icon_set->cache;
  while (tmp_list != NULL)
    {
      CachedIcon *icon = tmp_list->data;

      if (icon->style == style &&
          icon->direction == direction &&
          icon->state == state &&
          (size == (BtkIconSize)-1 || icon->size == size))
        {
          if (prev)
            {
              /* Move this icon to the front of the list. */
              prev->next = tmp_list->next;
              tmp_list->next = icon_set->cache;
              icon_set->cache = tmp_list;
            }

          return icon->pixbuf;
        }

      prev = tmp_list;
      tmp_list = g_slist_next (tmp_list);
    }

  return NULL;
}

static void
add_to_cache (BtkIconSet      *icon_set,
              BtkStyle        *style,
              BtkTextDirection direction,
              BtkStateType     state,
              BtkIconSize      size,
              BdkPixbuf       *pixbuf)
{
  CachedIcon *icon;

  ensure_cache_up_to_date (icon_set);

  g_object_ref (pixbuf);

  /* We have to ref the style, since if the style was finalized
   * its address could be reused by another style, creating a
   * really weird bug
   */

  if (style)
    g_object_ref (style);

  icon = g_new (CachedIcon, 1);
  icon_set->cache = g_slist_prepend (icon_set->cache, icon);
  icon_set->cache_size++;

  icon->style = style;
  icon->direction = direction;
  icon->state = state;
  icon->size = size;
  icon->pixbuf = pixbuf;

  if (icon->style)
    attach_to_style (icon_set, icon->style);

  if (icon_set->cache_size >= NUM_CACHED_ICONS)
    {
      /* Remove oldest item in the cache */
      GSList *tmp_list;

      tmp_list = icon_set->cache;

      /* Find next-to-last link */
      g_assert (NUM_CACHED_ICONS > 2);
      while (tmp_list->next->next)
        tmp_list = tmp_list->next;

      g_assert (tmp_list != NULL);
      g_assert (tmp_list->next != NULL);
      g_assert (tmp_list->next->next == NULL);

      /* Free the last icon */
      icon = tmp_list->next->data;

      g_slist_free (tmp_list->next);
      tmp_list->next = NULL;

      cached_icon_free (icon);
    }
}

static void
clear_cache (BtkIconSet *icon_set,
             gboolean    style_detach)
{
  GSList *cache, *tmp_list;
  BtkStyle *last_style = NULL;

  cache = icon_set->cache;
  icon_set->cache = NULL;
  icon_set->cache_size = 0;
  tmp_list = cache;
  while (tmp_list != NULL)
    {
      CachedIcon *icon = tmp_list->data;

      if (style_detach)
        {
          /* simple optimization for the case where the cache
           * contains contiguous icons from the same style.
           * it's safe to call detach_from_style more than
           * once on the same style though.
           */
          if (last_style != icon->style)
            {
              detach_from_style (icon_set, icon->style);
              last_style = icon->style;
            }
        }

      cached_icon_free (icon);

      tmp_list = g_slist_next (tmp_list);
    }

  g_slist_free (cache);
}

static GSList*
copy_cache (BtkIconSet *icon_set,
            BtkIconSet *copy_recipient)
{
  GSList *tmp_list;
  GSList *copy = NULL;

  ensure_cache_up_to_date (icon_set);

  tmp_list = icon_set->cache;
  while (tmp_list != NULL)
    {
      CachedIcon *icon = tmp_list->data;
      CachedIcon *icon_copy = g_new (CachedIcon, 1);

      *icon_copy = *icon;

      if (icon_copy->style)
	{
	  attach_to_style (copy_recipient, icon_copy->style);
	  g_object_ref (icon_copy->style);
	}

      g_object_ref (icon_copy->pixbuf);

      icon_copy->size = icon->size;

      copy = g_slist_prepend (copy, icon_copy);

      tmp_list = g_slist_next (tmp_list);
    }

  return g_slist_reverse (copy);
}

static void
attach_to_style (BtkIconSet *icon_set,
                 BtkStyle   *style)
{
  GHashTable *table;

  table = g_object_get_qdata (G_OBJECT (style),
                              g_quark_try_string ("btk-style-icon-sets"));

  if (table == NULL)
    {
      table = g_hash_table_new (NULL, NULL);
      g_object_set_qdata_full (G_OBJECT (style),
                               g_quark_from_static_string ("btk-style-icon-sets"),
                               table,
                               style_dnotify);
    }

  g_hash_table_insert (table, icon_set, icon_set);
}

static void
detach_from_style (BtkIconSet *icon_set,
                   BtkStyle   *style)
{
  GHashTable *table;

  table = g_object_get_qdata (G_OBJECT (style),
                              g_quark_try_string ("btk-style-icon-sets"));

  if (table != NULL)
    g_hash_table_remove (table, icon_set);
}

static void
iconsets_foreach (gpointer key,
                  gpointer value,
                  gpointer user_data)
{
  BtkIconSet *icon_set = key;

  /* We only need to remove cache entries for the given style;
   * but that complicates things because in destroy notify
   * we don't know which style got destroyed, and 95% of the
   * time all cache entries will have the same style,
   * so this is faster anyway.
   */

  clear_cache (icon_set, FALSE);
}

static void
style_dnotify (gpointer data)
{
  GHashTable *table = data;

  g_hash_table_foreach (table, iconsets_foreach, NULL);

  g_hash_table_destroy (table);
}

/* This allows the icon set to detect that its cache is out of date. */
void
_btk_icon_set_invalidate_caches (void)
{
  ++cache_serial;
}

/**
 * _btk_icon_factory_list_ids:
 *
 * Gets all known IDs stored in an existing icon factory.
 * The strings in the returned list aren't copied.
 * The list itself should be freed.
 *
 * Return value: List of ids in icon factories
 */
GList*
_btk_icon_factory_list_ids (void)
{
  GSList *tmp_list;
  GList *ids;

  ids = NULL;

  _btk_icon_factory_ensure_default_icons ();

  tmp_list = all_icon_factories;
  while (tmp_list != NULL)
    {
      GList *these_ids;

      BtkIconFactory *factory = BTK_ICON_FACTORY (tmp_list->data);

      these_ids = g_hash_table_get_keys (factory->icons);

      ids = g_list_concat (ids, these_ids);

      tmp_list = g_slist_next (tmp_list);
    }

  return ids;
}

typedef struct {
  GSList *sources;
  gboolean in_source;

} IconFactoryParserData;

typedef struct {
  gchar            *stock_id;
  gchar            *filename;
  gchar            *icon_name;
  BtkTextDirection  direction;
  BtkIconSize       size;
  BtkStateType      state;
} IconSourceParserData;

static void
icon_source_start_element (GMarkupParseContext *context,
			   const gchar         *element_name,
			   const gchar        **names,
			   const gchar        **values,
			   gpointer             user_data,
			   GError             **error)
{
  gint i;
  gchar *stock_id = NULL;
  gchar *filename = NULL;
  gchar *icon_name = NULL;
  gint size = -1;
  gint direction = -1;
  gint state = -1;
  IconFactoryParserData *parser_data;
  IconSourceParserData *source_data;
  gchar *error_msg;
  GQuark error_domain;

  parser_data = (IconFactoryParserData*)user_data;

  if (!parser_data->in_source)
    {
      if (strcmp (element_name, "sources") != 0)
	{
	  error_msg = g_strdup_printf ("Unexpected element %s, expected <sources>", element_name);
	  error_domain = BTK_BUILDER_ERROR_INVALID_TAG;
	  goto error;
	}
      parser_data->in_source = TRUE;
      return;
    }
  else
    {
      if (strcmp (element_name, "source") != 0)
	{
	  error_msg = g_strdup_printf ("Unexpected element %s, expected <source>", element_name);
	  error_domain = BTK_BUILDER_ERROR_INVALID_TAG;
	  goto error;
	}
    }

  for (i = 0; names[i]; i++)
    {
      if (strcmp (names[i], "stock-id") == 0)
	stock_id = g_strdup (values[i]);
      else if (strcmp (names[i], "filename") == 0)
	filename = g_strdup (values[i]);
      else if (strcmp (names[i], "icon-name") == 0)
	icon_name = g_strdup (values[i]);
      else if (strcmp (names[i], "size") == 0)
	{
          if (!_btk_builder_enum_from_string (BTK_TYPE_ICON_SIZE,
                                              values[i],
                                              &size,
                                              error))
	      return;
	}
      else if (strcmp (names[i], "direction") == 0)
	{
          if (!_btk_builder_enum_from_string (BTK_TYPE_TEXT_DIRECTION,
                                              values[i],
                                              &direction,
                                              error))
	      return;
	}
      else if (strcmp (names[i], "state") == 0)
	{
          if (!_btk_builder_enum_from_string (BTK_TYPE_STATE_TYPE,
                                              values[i],
                                              &state,
                                              error))
	      return;
	}
      else
	{
	  error_msg = g_strdup_printf ("'%s' is not a valid attribute of <%s>",
				       names[i], "source");
	  error_domain = BTK_BUILDER_ERROR_INVALID_ATTRIBUTE;
	  goto error;
	}
    }

  if (!stock_id)
    {
      error_msg = g_strdup_printf ("<source> requires a stock_id");
      error_domain = BTK_BUILDER_ERROR_MISSING_ATTRIBUTE;
      goto error;
    }

  source_data = g_slice_new (IconSourceParserData);
  source_data->stock_id = stock_id;
  source_data->filename = filename;
  source_data->icon_name = icon_name;
  source_data->size = size;
  source_data->direction = direction;
  source_data->state = state;

  parser_data->sources = g_slist_prepend (parser_data->sources, source_data);
  return;

 error:
  {
    gchar *tmp;
    gint line_number, char_number;

    g_markup_parse_context_get_position (context,
					 &line_number,
					 &char_number);

    tmp = g_strdup_printf ("%s:%d:%d %s", "input",
			   line_number, char_number, error_msg);
#if 0
    g_set_error_literal (error,
		 BTK_BUILDER_ERROR,
		 error_domain,
		 tmp);
#else
    g_warning ("%s", tmp);
#endif
    g_free (tmp);
    g_free (stock_id);
    g_free (filename);
    g_free (icon_name);
    return;
  }
}

static const GMarkupParser icon_source_parser =
  {
    icon_source_start_element,
  };

static gboolean
btk_icon_factory_buildable_custom_tag_start (BtkBuildable     *buildable,
					     BtkBuilder       *builder,
					     GObject          *child,
					     const gchar      *tagname,
					     GMarkupParser    *parser,
					     gpointer         *data)
{
  g_assert (buildable);

  if (strcmp (tagname, "sources") == 0)
    {
      IconFactoryParserData *parser_data;

      parser_data = g_slice_new0 (IconFactoryParserData);
      *parser = icon_source_parser;
      *data = parser_data;
      return TRUE;
    }
  return FALSE;
}

static void
btk_icon_factory_buildable_custom_tag_end (BtkBuildable *buildable,
					   BtkBuilder   *builder,
					   GObject      *child,
					   const gchar  *tagname,
					   gpointer     *user_data)
{
  BtkIconFactory *icon_factory;

  icon_factory = BTK_ICON_FACTORY (buildable);

  if (strcmp (tagname, "sources") == 0)
    {
      IconFactoryParserData *parser_data;
      BtkIconSource *icon_source;
      BtkIconSet *icon_set;
      GSList *l;

      parser_data = (IconFactoryParserData*)user_data;

      for (l = parser_data->sources; l; l = l->next)
	{
	  IconSourceParserData *source_data = l->data;

	  icon_set = btk_icon_factory_lookup (icon_factory, source_data->stock_id);
	  if (!icon_set)
	    {
	      icon_set = btk_icon_set_new ();
	      btk_icon_factory_add (icon_factory, source_data->stock_id, icon_set);
              btk_icon_set_unref (icon_set);
	    }

	  icon_source = btk_icon_source_new ();

	  if (source_data->filename)
	    {
	      gchar *filename;
	      filename = _btk_builder_get_absolute_filename (builder, source_data->filename);
	      btk_icon_source_set_filename (icon_source, filename);
	      g_free (filename);
	    }
	  if (source_data->icon_name)
	    btk_icon_source_set_icon_name (icon_source, source_data->icon_name);
	  if (source_data->size != -1)
            {
              btk_icon_source_set_size (icon_source, source_data->size);
              btk_icon_source_set_size_wildcarded (icon_source, FALSE);
            }
	  if (source_data->direction != -1)
            {
              btk_icon_source_set_direction (icon_source, source_data->direction);
              btk_icon_source_set_direction_wildcarded (icon_source, FALSE);
            }
	  if (source_data->state != -1)
            {
              btk_icon_source_set_state (icon_source, source_data->state);
              btk_icon_source_set_state_wildcarded (icon_source, FALSE);
            }

	  /* Inline source_add() to avoid creating a copy */
	  g_assert (icon_source->type != BTK_ICON_SOURCE_EMPTY);
	  icon_set->sources = g_slist_insert_sorted (icon_set->sources,
						     icon_source,
						     icon_source_compare);

	  g_free (source_data->stock_id);
	  g_free (source_data->filename);
	  g_free (source_data->icon_name);
	  g_slice_free (IconSourceParserData, source_data);
	}
      g_slist_free (parser_data->sources);
      g_slice_free (IconFactoryParserData, parser_data);

      /* TODO: Add an attribute/tag to prevent this.
       * Usually it's the right thing to do though.
       */
      btk_icon_factory_add_default (icon_factory);
    }
}

#if defined (G_OS_WIN32) && !defined (_WIN64)

/* DLL ABI stability backward compatibility versions */

#undef btk_icon_source_set_filename

void
btk_icon_source_set_filename (BtkIconSource *source,
			      const gchar   *filename)
{
  gchar *utf8_filename = g_locale_to_utf8 (filename, -1, NULL, NULL, NULL);

  btk_icon_source_set_filename_utf8 (source, utf8_filename);

  g_free (utf8_filename);
}

#undef btk_icon_source_get_filename

const gchar*
btk_icon_source_get_filename (const BtkIconSource *source)
{
  g_return_val_if_fail (source != NULL, NULL);

  if (source->type == BTK_ICON_SOURCE_FILENAME)
    return source->cp_filename;
  else
    return NULL;
}

#endif

#define __BTK_ICON_FACTORY_C__
#include "btkaliasdef.c"
