/* BTK - The GIMP Toolkit
 * btkfilechooserentry.c: Entry with filename completion
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

#include "btkfilechooserentry.h"

#include <string.h>

#include "btkalignment.h"
#include "btkcelllayout.h"
#include "btkcellrenderertext.h"
#include "btkentryprivate.h"
#include "btkfilesystemmodel.h"
#include "btklabel.h"
#include "btkmain.h"
#include "btkwindow.h"
#include "btkintl.h"
#include "btkalias.h"

#include "bdkkeysyms.h"

typedef struct _BtkFileChooserEntryClass BtkFileChooserEntryClass;

#define BTK_FILE_CHOOSER_ENTRY_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_FILE_CHOOSER_ENTRY, BtkFileChooserEntryClass))
#define BTK_IS_FILE_CHOOSER_ENTRY_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_FILE_CHOOSER_ENTRY))
#define BTK_FILE_CHOOSER_ENTRY_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_FILE_CHOOSER_ENTRY, BtkFileChooserEntryClass))

struct _BtkFileChooserEntryClass
{
  BtkEntryClass parent_class;
};

struct _BtkFileChooserEntry
{
  BtkEntry parent_instance;

  BtkFileChooserAction action;

  GFile *base_folder;
  GFile *current_folder_file;
  gchar *dir_part;
  gchar *file_part;

  BtkTreeModel *completion_store;

  guint current_folder_loaded : 1;
  guint complete_on_load : 1;
  guint eat_tabs       : 1;
  guint local_only     : 1;
};

enum
{
  DISPLAY_NAME_COLUMN,
  FULL_PATH_COLUMN,
  N_COLUMNS
};

static void     btk_file_chooser_entry_finalize       (GObject          *object);
static void     btk_file_chooser_entry_dispose        (GObject          *object);
static void     btk_file_chooser_entry_grab_focus     (BtkWidget        *widget);
static gboolean btk_file_chooser_entry_tab_handler    (BtkWidget *widget,
						       BdkEventKey *event);
static gboolean btk_file_chooser_entry_focus_out_event (BtkWidget       *widget,
							BdkEventFocus   *event);

#ifdef G_OS_WIN32
static gint     insert_text_callback      (BtkFileChooserEntry *widget,
					   const gchar         *new_text,
					   gint                 new_text_length,
					   gint                *position,
					   gpointer             user_data);
static void     delete_text_callback      (BtkFileChooserEntry *widget,
					   gint                 start_pos,
					   gint                 end_pos,
					   gpointer             user_data);
#endif

static gboolean match_selected_callback   (BtkEntryCompletion  *completion,
					   BtkTreeModel        *model,
					   BtkTreeIter         *iter,
					   BtkFileChooserEntry *chooser_entry);

static void set_complete_on_load (BtkFileChooserEntry *chooser_entry,
                                  gboolean             complete_on_load);
static void refresh_current_folder_and_file_part (BtkFileChooserEntry *chooser_entry);
static void set_completion_folder (BtkFileChooserEntry *chooser_entry,
                                   GFile               *folder,
				   char                *dir_part);
static void finished_loading_cb (BtkFileSystemModel  *model,
                                 GError              *error,
		                 BtkFileChooserEntry *chooser_entry);

G_DEFINE_TYPE (BtkFileChooserEntry, _btk_file_chooser_entry, BTK_TYPE_ENTRY)

static char *
btk_file_chooser_entry_get_completion_text (BtkFileChooserEntry *chooser_entry)
{
  BtkEditable *editable = BTK_EDITABLE (chooser_entry);
  int start, end;

  btk_editable_get_selection_bounds (editable, &start, &end);
  return btk_editable_get_chars (editable, 0, MIN (start, end));
}

static void
btk_file_chooser_entry_dispatch_properties_changed (GObject     *object,
                                                    guint        n_pspecs,
                                                    GParamSpec **pspecs)
{
  BtkFileChooserEntry *chooser_entry = BTK_FILE_CHOOSER_ENTRY (object);
  guint i;

  G_OBJECT_CLASS (_btk_file_chooser_entry_parent_class)->dispatch_properties_changed (object, n_pspecs, pspecs);

  /* Don't do this during or after disposal */
  if (btk_widget_get_parent (BTK_WIDGET (object)) != NULL)
    {
      /* What we are after: The text in front of the cursor was modified.
       * Unfortunately, there's no other way to catch this.
       */
      for (i = 0; i < n_pspecs; i++)
        {
          if (pspecs[i]->name == I_("cursor-position") ||
              pspecs[i]->name == I_("selection-bound") ||
              pspecs[i]->name == I_("text"))
            {
              set_complete_on_load (chooser_entry, FALSE);
              refresh_current_folder_and_file_part (chooser_entry);
              break;
            }
        }
    }
}

static void
_btk_file_chooser_entry_class_init (BtkFileChooserEntryClass *class)
{
  GObjectClass *bobject_class = G_OBJECT_CLASS (class);
  BtkWidgetClass *widget_class = BTK_WIDGET_CLASS (class);

  bobject_class->finalize = btk_file_chooser_entry_finalize;
  bobject_class->dispose = btk_file_chooser_entry_dispose;
  bobject_class->dispatch_properties_changed = btk_file_chooser_entry_dispatch_properties_changed;

  widget_class->grab_focus = btk_file_chooser_entry_grab_focus;
  widget_class->focus_out_event = btk_file_chooser_entry_focus_out_event;
}

static void
_btk_file_chooser_entry_init (BtkFileChooserEntry *chooser_entry)
{
  BtkEntryCompletion *comp;
  BtkCellRenderer *cell;

  chooser_entry->local_only = TRUE;

  g_object_set (chooser_entry, "truncate-multiline", TRUE, NULL);

  comp = btk_entry_completion_new ();
  btk_entry_completion_set_popup_single_match (comp, FALSE);
  btk_entry_completion_set_minimum_key_length (comp, 0);
  /* see docs for btk_entry_completion_set_text_column() */
  g_object_set (comp, "text-column", FULL_PATH_COLUMN, NULL);

  /* Need a match func here or entry completion uses a wrong one.
   * We do our own filtering after all. */
  btk_entry_completion_set_match_func (comp,
				       (BtkEntryCompletionMatchFunc) btk_true,
				       chooser_entry,
				       NULL);

  cell = btk_cell_renderer_text_new ();
  btk_cell_layout_pack_start (BTK_CELL_LAYOUT (comp),
                              cell, TRUE);
  btk_cell_layout_add_attribute (BTK_CELL_LAYOUT (comp),
                                 cell,
                                 "text", DISPLAY_NAME_COLUMN);

  g_signal_connect (comp, "match-selected",
		    G_CALLBACK (match_selected_callback), chooser_entry);

  btk_entry_set_completion (BTK_ENTRY (chooser_entry), comp);
  g_object_unref (comp);
  /* NB: This needs to happen after the completion is set, so this handler
   * runs before the handler installed by entrycompletion */
  g_signal_connect (chooser_entry, "key-press-event",
                    G_CALLBACK (btk_file_chooser_entry_tab_handler), NULL);

#ifdef G_OS_WIN32
  g_signal_connect (chooser_entry, "insert-text",
		    G_CALLBACK (insert_text_callback), NULL);
  g_signal_connect (chooser_entry, "delete-text",
		    G_CALLBACK (delete_text_callback), NULL);
#endif
}

static void
btk_file_chooser_entry_finalize (GObject *object)
{
  BtkFileChooserEntry *chooser_entry = BTK_FILE_CHOOSER_ENTRY (object);

  if (chooser_entry->base_folder)
    g_object_unref (chooser_entry->base_folder);

  if (chooser_entry->current_folder_file)
    g_object_unref (chooser_entry->current_folder_file);

  g_free (chooser_entry->dir_part);
  g_free (chooser_entry->file_part);

  G_OBJECT_CLASS (_btk_file_chooser_entry_parent_class)->finalize (object);
}

static void
btk_file_chooser_entry_dispose (GObject *object)
{
  BtkFileChooserEntry *chooser_entry = BTK_FILE_CHOOSER_ENTRY (object);

  set_completion_folder (chooser_entry, NULL, NULL);

  G_OBJECT_CLASS (_btk_file_chooser_entry_parent_class)->dispose (object);
}

/* Match functions for the BtkEntryCompletion */
static gboolean
match_selected_callback (BtkEntryCompletion  *completion,
                         BtkTreeModel        *model,
                         BtkTreeIter         *iter,
                         BtkFileChooserEntry *chooser_entry)
{
  char *path;
  gint pos;

  btk_tree_model_get (model, iter,
                      FULL_PATH_COLUMN, &path,
                      -1);

  btk_editable_delete_text (BTK_EDITABLE (chooser_entry),
                            0,
                            btk_editable_get_position (BTK_EDITABLE (chooser_entry)));
  pos = 0;
  btk_editable_insert_text (BTK_EDITABLE (chooser_entry),
                            path,
                            -1,
                            &pos);

  btk_editable_set_position (BTK_EDITABLE (chooser_entry), pos);

  g_free (path);

  return TRUE;
}

static void
set_complete_on_load (BtkFileChooserEntry *chooser_entry,
                      gboolean             complete_on_load)
{
  /* a completion was triggered, but we couldn't do it.
   * So no text was inserted when pressing tab, so we beep
   */
  if (chooser_entry->complete_on_load && !complete_on_load)
    btk_widget_error_bell (BTK_WIDGET (chooser_entry));

  chooser_entry->complete_on_load = complete_on_load;
}

static gboolean
is_valid_scheme_character (char c)
{
  return g_ascii_isalnum (c) || c == '+' || c == '-' || c == '.';
}

static gboolean
has_uri_scheme (const char *str)
{
  const char *p;

  p = str;

  if (!is_valid_scheme_character (*p))
    return FALSE;

  do
    p++;
  while (is_valid_scheme_character (*p));

  return (strncmp (p, "://", 3) == 0);
}

static GFile *
btk_file_chooser_get_file_for_text (BtkFileChooserEntry *chooser_entry,
                                    const gchar         *str)
{
  GFile *file;

  if (str[0] == '~' || g_path_is_absolute (str) || has_uri_scheme (str))
    file = g_file_parse_name (str);
  else if (chooser_entry->base_folder != NULL)
    file = g_file_resolve_relative_path (chooser_entry->base_folder, str);
  else
    file = NULL;

  return file;
}

static gboolean
is_directory_shortcut (const char *text)
{
  return strcmp (text, ".") == 0 ||
         strcmp (text, "..") == 0 ||
         strcmp (text, "~" ) == 0;
}

static GFile *
btk_file_chooser_get_directory_for_text (BtkFileChooserEntry *chooser_entry,
                                         const char *         text)
{
  GFile *file, *parent;

  file = btk_file_chooser_get_file_for_text (chooser_entry, text);

  if (file == NULL)
    return NULL;

  if (text[0] == 0 || text[strlen (text) - 1] == G_DIR_SEPARATOR ||
      is_directory_shortcut (text))
    return file;

  parent = g_file_get_parent (file);
  g_object_unref (file);

  return parent;
}

/* Finds a common prefix based on the contents of the entry
 * and mandatorily appends it
 */
static void
explicitly_complete (BtkFileChooserEntry *chooser_entry)
{
  chooser_entry->complete_on_load = FALSE;

  if (chooser_entry->completion_store)
    {
      char *completion, *text;
      gsize completion_len, text_len;

      text = btk_file_chooser_entry_get_completion_text (chooser_entry);
      text_len = strlen (text);
      completion = _btk_entry_completion_compute_prefix (btk_entry_get_completion (BTK_ENTRY (chooser_entry)), text);
      completion_len = completion ? strlen (completion) : 0;

      if (completion_len > text_len)
        {
          BtkEditable *editable = BTK_EDITABLE (chooser_entry);
          int pos = btk_editable_get_position (editable);

          btk_editable_insert_text (editable,
                                    completion + text_len,
                                    completion_len - text_len,
                                    &pos);
          btk_editable_set_position (editable, pos);
          return;
        }
    }

  btk_widget_error_bell (BTK_WIDGET (chooser_entry));
}

static void
btk_file_chooser_entry_grab_focus (BtkWidget *widget)
{
  BTK_WIDGET_CLASS (_btk_file_chooser_entry_parent_class)->grab_focus (widget);
  _btk_file_chooser_entry_select_filename (BTK_FILE_CHOOSER_ENTRY (widget));
}

static void
start_explicit_completion (BtkFileChooserEntry *chooser_entry)
{
  if (chooser_entry->current_folder_loaded)
    explicitly_complete (chooser_entry);
  else
    set_complete_on_load (chooser_entry, TRUE);
}

static gboolean
btk_file_chooser_entry_tab_handler (BtkWidget *widget,
				    BdkEventKey *event)
{
  BtkFileChooserEntry *chooser_entry;
  BtkEditable *editable;
  BdkModifierType state;
  gint start, end;

  chooser_entry = BTK_FILE_CHOOSER_ENTRY (widget);
  editable = BTK_EDITABLE (widget);

  if (!chooser_entry->eat_tabs)
    return FALSE;

  if (event->keyval != BDK_KEY_Tab)
    return FALSE;

  if (btk_get_current_event_state (&state) &&
      (state & BDK_CONTROL_MASK) == BDK_CONTROL_MASK)
    return FALSE;

  /* This is a bit evil -- it makes Tab never leave the entry. It basically
   * makes it 'safe' for people to hit. */
  btk_editable_get_selection_bounds (editable, &start, &end);
      
  if (start != end)
    btk_editable_set_position (editable, MAX (start, end));
  else
    start_explicit_completion (chooser_entry);

  return TRUE;
}

static gboolean
btk_file_chooser_entry_focus_out_event (BtkWidget     *widget,
					BdkEventFocus *event)
{
  BtkFileChooserEntry *chooser_entry = BTK_FILE_CHOOSER_ENTRY (widget);

  set_complete_on_load (chooser_entry, FALSE);
 
  return BTK_WIDGET_CLASS (_btk_file_chooser_entry_parent_class)->focus_out_event (widget, event);
}

static void
update_inline_completion (BtkFileChooserEntry *chooser_entry)
{
  BtkEntryCompletion *completion = btk_entry_get_completion (BTK_ENTRY (chooser_entry));

  if (!chooser_entry->current_folder_loaded)
    {
      btk_entry_completion_set_inline_completion (completion, FALSE);
      return;
    }

  switch (chooser_entry->action)
    {
    case BTK_FILE_CHOOSER_ACTION_OPEN:
    case BTK_FILE_CHOOSER_ACTION_SELECT_FOLDER:
      btk_entry_completion_set_inline_completion (completion, TRUE);
      break;
    case BTK_FILE_CHOOSER_ACTION_SAVE:
    case BTK_FILE_CHOOSER_ACTION_CREATE_FOLDER:
      btk_entry_completion_set_inline_completion (completion, FALSE);
      break;
    }
}

static void
discard_completion_store (BtkFileChooserEntry *chooser_entry)
{
  if (!chooser_entry->completion_store)
    return;

  btk_entry_completion_set_model (btk_entry_get_completion (BTK_ENTRY (chooser_entry)), NULL);
  update_inline_completion (chooser_entry);
  g_object_unref (chooser_entry->completion_store);
  chooser_entry->completion_store = NULL;
}

static gboolean
completion_store_set (BtkFileSystemModel  *model,
                      GFile               *file,
                      GFileInfo           *info,
                      int                  column,
                      GValue              *value,
                      gpointer             data)
{
  BtkFileChooserEntry *chooser_entry = data;

  const char *prefix = "";
  const char *suffix = "";

  switch (column)
    {
    case FULL_PATH_COLUMN:
      prefix = chooser_entry->dir_part;
      /* fall through */
    case DISPLAY_NAME_COLUMN:
      if (_btk_file_info_consider_as_directory (info))
        suffix = G_DIR_SEPARATOR_S;

      g_value_take_string (value,
			   g_strconcat (prefix,
					g_file_info_get_display_name (info),
					suffix,
					NULL));
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  return TRUE;
}

/* Fills the completion store from the contents of the current folder */
static void
populate_completion_store (BtkFileChooserEntry *chooser_entry)
{
  chooser_entry->completion_store = BTK_TREE_MODEL (
      _btk_file_system_model_new_for_directory (chooser_entry->current_folder_file,
                                                "standard::name,standard::display-name,standard::type",
                                                completion_store_set,
                                                chooser_entry,
                                                N_COLUMNS,
                                                G_TYPE_STRING,
                                                G_TYPE_STRING));
  g_signal_connect (chooser_entry->completion_store, "finished-loading",
		    G_CALLBACK (finished_loading_cb), chooser_entry);

  _btk_file_system_model_set_filter_folders (BTK_FILE_SYSTEM_MODEL (chooser_entry->completion_store),
                                             TRUE);
  _btk_file_system_model_set_show_files (BTK_FILE_SYSTEM_MODEL (chooser_entry->completion_store),
                                         chooser_entry->action == BTK_FILE_CHOOSER_ACTION_OPEN ||
                                         chooser_entry->action == BTK_FILE_CHOOSER_ACTION_SAVE);
  btk_tree_sortable_set_sort_column_id (BTK_TREE_SORTABLE (chooser_entry->completion_store),
					DISPLAY_NAME_COLUMN, BTK_SORT_ASCENDING);

  btk_entry_completion_set_model (btk_entry_get_completion (BTK_ENTRY (chooser_entry)),
				  chooser_entry->completion_store);
}

/* Callback when the current folder finishes loading */
static void
finished_loading_cb (BtkFileSystemModel  *model,
                     GError              *error,
		     BtkFileChooserEntry *chooser_entry)
{
  BtkEntryCompletion *completion;

  chooser_entry->current_folder_loaded = TRUE;

  if (error)
    {
      discard_completion_store (chooser_entry);
      set_complete_on_load (chooser_entry, FALSE);
      return;
    }

  if (chooser_entry->complete_on_load)
    explicitly_complete (chooser_entry);

  btk_widget_set_tooltip_text (BTK_WIDGET (chooser_entry), NULL);

  completion = btk_entry_get_completion (BTK_ENTRY (chooser_entry));
  update_inline_completion (chooser_entry);

  if (btk_widget_has_focus (BTK_WIDGET (chooser_entry)))
    {
      btk_entry_completion_complete (completion);
      btk_entry_completion_insert_prefix (completion);
    }
}

static void
set_completion_folder (BtkFileChooserEntry *chooser_entry,
                       GFile               *folder_file,
		       char                *dir_part)
{
  if (folder_file &&
      chooser_entry->local_only
      && !_btk_file_has_native_path (folder_file))
    folder_file = NULL;

  if (((chooser_entry->current_folder_file
	&& folder_file
	&& g_file_equal (folder_file, chooser_entry->current_folder_file))
       || chooser_entry->current_folder_file == folder_file)
      && g_strcmp0 (dir_part, chooser_entry->dir_part) == 0)
    {
      return;
    }

  if (chooser_entry->current_folder_file)
    {
      g_object_unref (chooser_entry->current_folder_file);
      chooser_entry->current_folder_file = NULL;
    }

  g_free (chooser_entry->dir_part);
  chooser_entry->dir_part = g_strdup (dir_part);
  
  chooser_entry->current_folder_loaded = FALSE;

  discard_completion_store (chooser_entry);

  if (folder_file)
    {
      chooser_entry->current_folder_file = g_object_ref (folder_file);
      populate_completion_store (chooser_entry);
    }
}

static void
refresh_current_folder_and_file_part (BtkFileChooserEntry *chooser_entry)
{
  GFile *folder_file;
  char *text, *last_slash, *old_file_part;
  char *dir_part;

  old_file_part = chooser_entry->file_part;

  text = btk_file_chooser_entry_get_completion_text (chooser_entry);

  last_slash = strrchr (text, G_DIR_SEPARATOR);
  if (last_slash)
    {
      dir_part = g_strndup (text, last_slash - text + 1);
      chooser_entry->file_part = g_strdup (last_slash + 1);
    }
  else
    {
      dir_part = g_strdup ("");
      chooser_entry->file_part = g_strdup (text);
    }

  folder_file = btk_file_chooser_get_directory_for_text (chooser_entry, text);

  set_completion_folder (chooser_entry, folder_file, dir_part);

  if (folder_file)
    g_object_unref (folder_file);

  g_free (dir_part);

  if (chooser_entry->completion_store &&
      (g_strcmp0 (old_file_part, chooser_entry->file_part) != 0))
    {
      BtkFileFilter *filter;
      char *pattern;

      filter = btk_file_filter_new ();
      pattern = g_strconcat (chooser_entry->file_part, "*", NULL);
      btk_file_filter_add_pattern (filter, pattern);

      g_object_ref_sink (filter);

      _btk_file_system_model_set_filter (BTK_FILE_SYSTEM_MODEL (chooser_entry->completion_store),
                                         filter);

      g_free (pattern);
      g_object_unref (filter);
    }

  g_free (text);
  g_free (old_file_part);
}

#ifdef G_OS_WIN32
static gint
insert_text_callback (BtkFileChooserEntry *chooser_entry,
		      const gchar	  *new_text,
		      gint       	   new_text_length,
		      gint       	  *position,
		      gpointer   	   user_data)
{
  const gchar *colon = memchr (new_text, ':', new_text_length);
  gint i;

  /* Disallow these characters altogether */
  for (i = 0; i < new_text_length; i++)
    {
      if (new_text[i] == '<' ||
	  new_text[i] == '>' ||
	  new_text[i] == '"' ||
	  new_text[i] == '|' ||
	  new_text[i] == '*' ||
	  new_text[i] == '?')
	break;
    }

  if (i < new_text_length ||
      /* Disallow entering text that would cause a colon to be anywhere except
       * after a drive letter.
       */
      (colon != NULL &&
       *position + (colon - new_text) != 1) ||
      (new_text_length > 0 &&
       *position <= 1 &&
       btk_entry_get_text_length (BTK_ENTRY (chooser_entry)) >= 2 &&
       btk_entry_get_text (BTK_ENTRY (chooser_entry))[1] == ':'))
    {
      btk_widget_error_bell (BTK_WIDGET (chooser_entry));
      g_signal_stop_emission_by_name (chooser_entry, "insert_text");
      return FALSE;
    }

  return TRUE;
}

static void
delete_text_callback (BtkFileChooserEntry *chooser_entry,
		      gint                 start_pos,
		      gint                 end_pos,
		      gpointer             user_data)
{
  /* If deleting a drive letter, delete the colon, too */
  if (start_pos == 0 && end_pos == 1 &&
      btk_entry_get_text_length (BTK_ENTRY (chooser_entry)) >= 2 &&
      btk_entry_get_text (BTK_ENTRY (chooser_entry))[1] == ':')
    {
      g_signal_handlers_block_by_func (chooser_entry,
				       G_CALLBACK (delete_text_callback),
				       user_data);
      btk_editable_delete_text (BTK_EDITABLE (chooser_entry), 0, 1);
      g_signal_handlers_unblock_by_func (chooser_entry,
					 G_CALLBACK (delete_text_callback),
					 user_data);
    }
}
#endif

/**
 * _btk_file_chooser_entry_new:
 * @eat_tabs: If %FALSE, allow focus navigation with the tab key.
 *
 * Creates a new #BtkFileChooserEntry object. #BtkFileChooserEntry
 * is an internal implementation widget for the BTK+ file chooser
 * which is an entry with completion with respect to a
 * #BtkFileSystem object.
 *
 * Return value: the newly created #BtkFileChooserEntry
 **/
BtkWidget *
_btk_file_chooser_entry_new (gboolean       eat_tabs)
{
  BtkFileChooserEntry *chooser_entry;

  chooser_entry = g_object_new (BTK_TYPE_FILE_CHOOSER_ENTRY, NULL);
  chooser_entry->eat_tabs = (eat_tabs != FALSE);

  return BTK_WIDGET (chooser_entry);
}

/**
 * _btk_file_chooser_entry_set_base_folder:
 * @chooser_entry: a #BtkFileChooserEntry
 * @file: file for a folder in the chooser entries current file system.
 *
 * Sets the folder with respect to which completions occur.
 **/
void
_btk_file_chooser_entry_set_base_folder (BtkFileChooserEntry *chooser_entry,
					 GFile               *file)
{
  g_return_if_fail (BTK_IS_FILE_CHOOSER_ENTRY (chooser_entry));
  g_return_if_fail (file == NULL || G_IS_FILE (file));

  if (chooser_entry->base_folder == file ||
      (file != NULL && chooser_entry->base_folder != NULL 
       && g_file_equal (chooser_entry->base_folder, file)))
    return;

  if (file)
    g_object_ref (file);

  if (chooser_entry->base_folder)
    g_object_unref (chooser_entry->base_folder);

  chooser_entry->base_folder = file;

  refresh_current_folder_and_file_part (chooser_entry);
}

/**
 * _btk_file_chooser_entry_get_current_folder:
 * @chooser_entry: a #BtkFileChooserEntry
 *
 * Gets the current folder for the #BtkFileChooserEntry. If the
 * user has only entered a filename, this will be in the base folder
 * (see _btk_file_chooser_entry_set_base_folder()), but if the
 * user has entered a relative or absolute path, then it will
 * be different.  If the user has entered unparsable text, or text which
 * the entry cannot handle, this will return %NULL.
 *
 * Return value: the file for the current folder - you must g_object_unref()
 *   the value after use.
 **/
GFile *
_btk_file_chooser_entry_get_current_folder (BtkFileChooserEntry *chooser_entry)
{
  g_return_val_if_fail (BTK_IS_FILE_CHOOSER_ENTRY (chooser_entry), NULL);

  return btk_file_chooser_get_directory_for_text (chooser_entry,
                                                  btk_entry_get_text (BTK_ENTRY (chooser_entry)));
}

/**
 * _btk_file_chooser_entry_get_file_part:
 * @chooser_entry: a #BtkFileChooserEntry
 *
 * Gets the non-folder portion of whatever the user has entered
 * into the file selector. What is returned is a UTF-8 string,
 * and if a filename path is needed, g_file_get_child_for_display_name()
 * must be used
  *
 * Return value: the entered filename - this value is owned by the
 *  chooser entry and must not be modified or freed.
 **/
const gchar *
_btk_file_chooser_entry_get_file_part (BtkFileChooserEntry *chooser_entry)
{
  const char *last_slash, *text;

  g_return_val_if_fail (BTK_IS_FILE_CHOOSER_ENTRY (chooser_entry), NULL);

  text = btk_entry_get_text (BTK_ENTRY (chooser_entry));
  last_slash = strrchr (text, G_DIR_SEPARATOR);
  if (last_slash)
    return last_slash + 1;
  else if (is_directory_shortcut (text))
    return "";
  else
    return text;
}

/**
 * _btk_file_chooser_entry_set_action:
 * @chooser_entry: a #BtkFileChooserEntry
 * @action: the action which is performed by the file selector using this entry
 *
 * Sets action which is performed by the file selector using this entry. 
 * The #BtkFileChooserEntry will use different completion strategies for 
 * different actions.
 **/
void
_btk_file_chooser_entry_set_action (BtkFileChooserEntry *chooser_entry,
				    BtkFileChooserAction action)
{
  g_return_if_fail (BTK_IS_FILE_CHOOSER_ENTRY (chooser_entry));
  
  if (chooser_entry->action != action)
    {
      BtkEntryCompletion *comp;

      chooser_entry->action = action;

      comp = btk_entry_get_completion (BTK_ENTRY (chooser_entry));

      /* FIXME: do we need to actually set the following? */

      switch (action)
	{
	case BTK_FILE_CHOOSER_ACTION_OPEN:
	case BTK_FILE_CHOOSER_ACTION_SELECT_FOLDER:
	  btk_entry_completion_set_popup_single_match (comp, FALSE);
	  break;
	case BTK_FILE_CHOOSER_ACTION_SAVE:
	case BTK_FILE_CHOOSER_ACTION_CREATE_FOLDER:
	  btk_entry_completion_set_popup_single_match (comp, TRUE);
	  break;
	}

      if (chooser_entry->completion_store)
        _btk_file_system_model_set_show_files (BTK_FILE_SYSTEM_MODEL (chooser_entry->completion_store),
                                               action == BTK_FILE_CHOOSER_ACTION_OPEN ||
                                               action == BTK_FILE_CHOOSER_ACTION_SAVE);

      update_inline_completion (chooser_entry);
    }
}


/**
 * _btk_file_chooser_entry_get_action:
 * @chooser_entry: a #BtkFileChooserEntry
 *
 * Gets the action for this entry. 
 *
 * Returns: the action
 **/
BtkFileChooserAction
_btk_file_chooser_entry_get_action (BtkFileChooserEntry *chooser_entry)
{
  g_return_val_if_fail (BTK_IS_FILE_CHOOSER_ENTRY (chooser_entry),
			BTK_FILE_CHOOSER_ACTION_OPEN);
  
  return chooser_entry->action;
}

gboolean
_btk_file_chooser_entry_get_is_folder (BtkFileChooserEntry *chooser_entry,
				       GFile               *file)
{
  BtkTreeIter iter;
  GFileInfo *info;

  if (chooser_entry->completion_store == NULL ||
      !_btk_file_system_model_get_iter_for_file (BTK_FILE_SYSTEM_MODEL (chooser_entry->completion_store),
                                                 &iter,
                                                 file))
    return FALSE;

  info = _btk_file_system_model_get_info (BTK_FILE_SYSTEM_MODEL (chooser_entry->completion_store),
                                          &iter);

  return _btk_file_info_consider_as_directory (info);
}


/*
 * _btk_file_chooser_entry_select_filename:
 * @chooser_entry: a #BtkFileChooserEntry
 *
 * Selects the filename (without the extension) for user edition.
 */
void
_btk_file_chooser_entry_select_filename (BtkFileChooserEntry *chooser_entry)
{
  const gchar *str, *ext;
  glong len = -1;

  if (chooser_entry->action == BTK_FILE_CHOOSER_ACTION_SAVE)
    {
      str = btk_entry_get_text (BTK_ENTRY (chooser_entry));
      ext = g_strrstr (str, ".");

      if (ext)
       len = g_utf8_pointer_to_offset (str, ext);
    }

  btk_editable_select_rebunnyion (BTK_EDITABLE (chooser_entry), 0, (gint) len);
}

void
_btk_file_chooser_entry_set_local_only (BtkFileChooserEntry *chooser_entry,
                                        gboolean             local_only)
{
  chooser_entry->local_only = local_only;
  refresh_current_folder_and_file_part (chooser_entry);
}

gboolean
_btk_file_chooser_entry_get_local_only (BtkFileChooserEntry *chooser_entry)
{
  return chooser_entry->local_only;
}
