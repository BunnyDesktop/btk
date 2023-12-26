#include "config.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <btk/btk.h>
#include <bunnylib/gstdio.h>

#include "demos.h"

static BtkTextBuffer *info_buffer;
static BtkTextBuffer *source_buffer;

static gchar *current_file = NULL;


enum {
  TITLE_COLUMN,
  FILENAME_COLUMN,
  FUNC_COLUMN,
  STYLE_COLUMN,
  NUM_COLUMNS
};

typedef struct _CallbackData CallbackData;
struct _CallbackData
{
  BtkTreeModel *model;
  BtkTreePath *path;
};

#ifdef G_OS_WIN32

#undef DEMOCODEDIR

static char *
get_democodedir (void)
{
  static char *result = NULL;

  if (result == NULL)
    {
      result = g_win32_get_package_installation_directory_of_module (NULL);
      if (result == NULL)
	result = "unknown-location";

      result = g_strconcat (result, "\\share\\btk-2.0\\demo", NULL);
    }

  return result;
}

#define DEMOCODEDIR get_democodedir ()

#endif

/**
 * demo_find_file:
 * @base: base filename
 * @err:  location to store error, or %NULL.
 * 
 * Looks for @base first in the current directory, then in the
 * location BTK+ where it will be installed on make install,
 * returns the first file found.
 * 
 * Return value: the filename, if found or %NULL
 **/
gchar *
demo_find_file (const char *base,
		GError    **err)
{
  g_return_val_if_fail (err == NULL || *err == NULL, NULL);
  
  if (g_file_test ("btk-logo-rgb.gif", G_FILE_TEST_EXISTS) &&
      g_file_test (base, G_FILE_TEST_EXISTS))
    return g_strdup (base);
  else
    {
      char *filename = g_build_filename (DEMOCODEDIR, base, NULL);
      if (!g_file_test (filename, G_FILE_TEST_EXISTS))
	{
	  g_set_error (err, G_FILE_ERROR, G_FILE_ERROR_NOENT,
		       "Cannot find demo data file \"%s\"", base);
	  g_free (filename);
	  return NULL;
	}
      return filename;
    }
}

static void
window_closed_cb (BtkWidget *window, gpointer data)
{
  CallbackData *cbdata = data;
  BtkTreeIter iter;
  BangoStyle style;

  btk_tree_model_get_iter (cbdata->model, &iter, cbdata->path);
  btk_tree_model_get (BTK_TREE_MODEL (cbdata->model), &iter,
		      STYLE_COLUMN, &style,
		      -1);
  if (style == BANGO_STYLE_ITALIC)
    btk_tree_store_set (BTK_TREE_STORE (cbdata->model), &iter,
			STYLE_COLUMN, BANGO_STYLE_NORMAL,
			-1);

  btk_tree_path_free (cbdata->path);
  g_free (cbdata);
}

gboolean
read_line (FILE *stream, GString *str)
{
  int n_read = 0;
  
#ifdef HAVE_FLOCKFILE
  flockfile (stream);
#endif

  g_string_truncate (str, 0);
  
  while (1)
    {
      int c;
      
#ifdef HAVE_FLOCKFILE
      c = getc_unlocked (stream);
#else
      c = getc (stream);
#endif

      if (c == EOF)
	goto done;
      else
	n_read++;

      switch (c)
	{
	case '\r':
	case '\n':
	  {
#ifdef HAVE_FLOCKFILE
	    int next_c = getc_unlocked (stream);
#else
	    int next_c = getc (stream);
#endif
	    
	    if (!(next_c == EOF ||
		  (c == '\r' && next_c == '\n') ||
		  (c == '\n' && next_c == '\r')))
	      ungetc (next_c, stream);
	    
	    goto done;
	  }
	default:
	  g_string_append_c (str, c);
	}
    }

 done:

#ifdef HAVE_FLOCKFILE
  funlockfile (stream);
#endif

  return n_read > 0;
}


/* Stupid syntax highlighting.
 *
 * No regex was used in the making of this highlighting.
 * It should only work for simple cases.  This is good, as
 * that's all we should have in the demos.
 */
/* This code should not be used elsewhere, except perhaps as an example of how
 * to iterate through a text buffer.
 */
enum {
  STATE_NORMAL,
  STATE_IN_COMMENT
};

static gchar *tokens[] =
{
  "/*",
  "\"",
  NULL
};

static gchar *types[] =
{
  "static",
  "const ",
  "void",
  "gint",
  " int ",
  " char ",
  "gchar ",
  "gfloat",
  "float",
  "double",
  "gint8",
  "gint16",
  "gint32",
  "guint",
  "guint8",
  "guint16",
  "guint32",
  "guchar",
  "glong",
  "gboolean" ,
  "gshort",
  "gushort",
  "gulong",
  "gdouble",
  "gldouble",
  "gpointer",
  "NULL",
  "GList",
  "GSList",
  "FALSE",
  "TRUE",
  "FILE ",
  "BtkObject ",
  "BtkColorSelection ",
  "BtkWidget ",
  "BtkButton ",
  "BdkColor ",
  "BdkRectangle ",
  "BdkEventExpose ",
  "BdkGC ",
  "BdkPixbufLoader ",
  "BdkPixbuf ",
  "GError",
  "size_t",
  "BtkAboutDialog ",
  "BtkAction ",
  "BtkActionEntry ",
  "BtkRadioActionEntry ",
  "BtkIconFactory ",
  "BtkStockItem ",
  "BtkIconSet ",
  "BtkTextBuffer ",
  "BtkStatusbar ",
  "BtkTextIter ",
  "BtkTextMark ",
  "BdkEventWindowState ",
  "BtkActionGroup ",
  "BtkUIManager ",
  "BtkRadioAction ",
  "BtkActionClass ",
  "BtkToggleActionEntry ",
  "BtkAssistant ",
  "BtkBuilder ",
  "BtkSizeGroup ",
  "BtkTreeModel ",
  "BtkTreeSelection ",
  "BdkDisplay ",
  "BdkScreen ",
  "BdkWindow ",
  "BdkEventButton ",
  "BdkCursor ",
  "BtkTreeIter ",
  "BtkTreeViewColumn ",
  "BdkDisplayManager ",
  "BtkClipboard ",
  "BtkIconSize ",
  "BtkImage ",
  "BdkDragContext ",
  "BtkSelectionData ",
  "BtkDialog ",
  "BtkMenuItem ",
  "BtkListStore ",
  "BtkCellLayout ",
  "BtkCellRenderer ",
  "BtkTreePath ",
  "BtkTreeStore ",
  "BtkEntry ",
  "BtkEditable ",
  "BtkEditableClass ",
  "BdkPixmap ",
  "BdkEventConfigure ",
  "BdkEventMotion ",
  "BdkModifierType ",
  "BtkEntryCompletion ",
  "BtkToolItem ",
  "GDir ",
  "BtkIconView ",
  "BtkCellRendererText ",
  "BtkContainer ",
  "BtkAccelGroup ",
  "BtkPaned ",
  "BtkPrintOperation ",
  "BtkPrintContext ",
  "bairo_t ",
  "BangoLayout "
  "BangoFontDescription ",
  "BangoRenderer ",
  "BangoMatrix ",
  "BangoContext ",
  "BangoLayout ",
  "BtkTable ",
  "BtkToggleButton ",
  "GString ",
  "BtkIconSize ",
  "BtkTreeView ",
  "BtkTextTag ",
  "BdkEvent ",
  "BdkEventKey ",
  "BtkTextView ",
  "BdkEventVisibility ",
  "BdkBitmap ",
  "BtkTextChildAnchor ",
  "GArray ",
  "BtkCellEditable ",
  "BtkCellRendererToggle ",
  NULL
};

static gchar *control[] =
{
  " if ",
  " while ",
  " else",
  " do ",
  " for ",
  "?",
  ":",
  "return ",
  "goto ",
  NULL
};
void
parse_chars (gchar     *text,
	     gchar    **end_ptr,
	     gint      *state,
	     gchar    **tag,
	     gboolean   start)
{
  gint i;
  gchar *next_token;

  /* Handle comments first */
  if (*state == STATE_IN_COMMENT)
    {
      *end_ptr = strstr (text, "*/");
      if (*end_ptr)
	{
	  *end_ptr += 2;
	  *state = STATE_NORMAL;
	  *tag = "comment";
	}
      return;
    }

  *tag = NULL;
  *end_ptr = NULL;

  /* check for comment */
  if (!strncmp (text, "/*", 2))
    {
      *end_ptr = strstr (text, "*/");
      if (*end_ptr)
	*end_ptr += 2;
      else
	*state = STATE_IN_COMMENT;
      *tag = "comment";
      return;
    }

  /* check for preprocessor defines */
  if (*text == '#' && start)
    {
      *end_ptr = NULL;
      *tag = "preprocessor";
      return;
    }

  /* functions */
  if (start && * text != '\t' && *text != ' ' && *text != '{' && *text != '}')
    {
      if (strstr (text, "("))
	{
	  *end_ptr = strstr (text, "(");
	  *tag = "function";
	  return;
	}
    }
  /* check for types */
  for (i = 0; types[i] != NULL; i++)
    if (!strncmp (text, types[i], strlen (types[i])) ||
        (start && types[i][0] == ' ' && !strncmp (text, types[i] + 1, strlen (types[i]) - 1)))
      {
	*end_ptr = text + strlen (types[i]);
	*tag = "type";
	return;
      }

  /* check for control */
  for (i = 0; control[i] != NULL; i++)
    if (!strncmp (text, control[i], strlen (control[i])))
      {
	*end_ptr = text + strlen (control[i]);
	*tag = "control";
	return;
      }

  /* check for string */
  if (text[0] == '"')
    {
      gint maybe_escape = FALSE;

      *end_ptr = text + 1;
      *tag = "string";
      while (**end_ptr != '\000')
	{
	  if (**end_ptr == '\"' && !maybe_escape)
	    {
	      *end_ptr += 1;
	      return;
	    }
	  if (**end_ptr == '\\')
	    maybe_escape = TRUE;
	  else
	    maybe_escape = FALSE;
	  *end_ptr += 1;
	}
      return;
    }

  /* not at the start of a tag.  Find the next one. */
  for (i = 0; tokens[i] != NULL; i++)
    {
      next_token = strstr (text, tokens[i]);
      if (next_token)
	{
	  if (*end_ptr)
	    *end_ptr = (*end_ptr<next_token)?*end_ptr:next_token;
	  else
	    *end_ptr = next_token;
	}
    }

  for (i = 0; types[i] != NULL; i++)
    {
      next_token = strstr (text, types[i]);
      if (next_token)
	{
	  if (*end_ptr)
	    *end_ptr = (*end_ptr<next_token)?*end_ptr:next_token;
	  else
	    *end_ptr = next_token;
	}
    }

  for (i = 0; control[i] != NULL; i++)
    {
      next_token = strstr (text, control[i]);
      if (next_token)
	{
	  if (*end_ptr)
	    *end_ptr = (*end_ptr<next_token)?*end_ptr:next_token;
	  else
	    *end_ptr = next_token;
	}
    }
}

/* While not as cool as c-mode, this will do as a quick attempt at highlighting */
static void
fontify (void)
{
  BtkTextIter start_iter, next_iter, tmp_iter;
  gint state;
  gchar *text;
  gchar *start_ptr, *end_ptr;
  gchar *tag;

  state = STATE_NORMAL;

  btk_text_buffer_get_iter_at_offset (source_buffer, &start_iter, 0);

  next_iter = start_iter;
  while (btk_text_iter_forward_line (&next_iter))
    {
      gboolean start = TRUE;
      start_ptr = text = btk_text_iter_get_text (&start_iter, &next_iter);

      do
	{
	  parse_chars (start_ptr, &end_ptr, &state, &tag, start);

	  start = FALSE;
	  if (end_ptr)
	    {
	      tmp_iter = start_iter;
	      btk_text_iter_forward_chars (&tmp_iter, end_ptr - start_ptr);
	    }
	  else
	    {
	      tmp_iter = next_iter;
	    }
	  if (tag)
	    btk_text_buffer_apply_tag_by_name (source_buffer, tag, &start_iter, &tmp_iter);

	  start_iter = tmp_iter;
	  start_ptr = end_ptr;
	}
      while (end_ptr);

      g_free (text);
      start_iter = next_iter;
    }
}

void
load_file (const gchar *filename)
{
  FILE *file;
  BtkTextIter start, end;
  char *full_filename;
  GError *err = NULL;
  GString *buffer = g_string_new (NULL);
  int state = 0;
  gboolean in_para = 0;

  if (current_file && !strcmp (current_file, filename))
    {
      g_string_free (buffer, TRUE);
      return;
    }

  g_free (current_file);
  current_file = g_strdup (filename);
  
  btk_text_buffer_get_bounds (info_buffer, &start, &end);
  btk_text_buffer_delete (info_buffer, &start, &end);

  btk_text_buffer_get_bounds (source_buffer, &start, &end);
  btk_text_buffer_delete (source_buffer, &start, &end);

  full_filename = demo_find_file (filename, &err);
  if (!full_filename)
    {
      g_warning ("%s", err->message);
      g_error_free (err);
      return;
    }

  file = g_fopen (full_filename, "r");

  if (!file)
    g_warning ("Cannot open %s: %s\n", full_filename, g_strerror (errno));

  g_free (full_filename);

  if (!file)
    return;

  btk_text_buffer_get_iter_at_offset (info_buffer, &start, 0);
  while (read_line (file, buffer))
    {
      gchar *p = buffer->str;
      gchar *q;
      gchar *r;
      
      switch (state)
	{
	case 0:
	  /* Reading title */
	  while (*p == '/' || *p == '*' || g_ascii_isspace (*p))
	    p++;
	  r = p;
	  while (*r != '/' && strlen (r))
	    r++;
	  if (strlen (r) > 0)
	    p = r + 1;
	  q = p + strlen (p);
	  while (q > p && g_ascii_isspace (*(q - 1)))
	    q--;

	  if (q > p)
	    {
	      int len_chars = g_utf8_pointer_to_offset (p, q);

	      end = start;

	      g_assert (strlen (p) >= q - p);
	      btk_text_buffer_insert (info_buffer, &end, p, q - p);
	      start = end;

	      btk_text_iter_backward_chars (&start, len_chars);
	      btk_text_buffer_apply_tag_by_name (info_buffer, "title", &start, &end);

	      start = end;
	      
	      state++;
	    }
	  break;
	    
	case 1:
	  /* Reading body of info section */
	  while (g_ascii_isspace (*p))
	    p++;
	  if (*p == '*' && *(p + 1) == '/')
	    {
	      btk_text_buffer_get_iter_at_offset (source_buffer, &start, 0);
	      state++;
	    }
	  else
	    {
	      int len;
	      
	      while (*p == '*' || g_ascii_isspace (*p))
		p++;

	      len = strlen (p);
	      while (g_ascii_isspace (*(p + len - 1)))
		len--;
	      
	      if (len > 0)
		{
		  if (in_para)
		    btk_text_buffer_insert (info_buffer, &start, " ", 1);

		  g_assert (strlen (p) >= len);
		  btk_text_buffer_insert (info_buffer, &start, p, len);
		  in_para = 1;
		}
	      else
		{
		  btk_text_buffer_insert (info_buffer, &start, "\n", 1);
		  in_para = 0;
		}
	    }
	  break;

	case 2:
	  /* Skipping blank lines */
	  while (g_ascii_isspace (*p))
	    p++;
	  if (*p)
	    {
	      p = buffer->str;
	      state++;
	      /* Fall through */
	    }
	  else
	    break;
	  
	case 3:
	  /* Reading program body */
	  btk_text_buffer_insert (source_buffer, &start, p, -1);
	  btk_text_buffer_insert (source_buffer, &start, "\n", 1);
	  break;
	}
    }

  fclose (file);
  
  fontify ();

  g_string_free (buffer, TRUE);
}

void
row_activated_cb (BtkTreeView       *tree_view,
                  BtkTreePath       *path,
		  BtkTreeViewColumn *column)
{
  BtkTreeIter iter;
  BangoStyle style;
  GDoDemoFunc func;
  BtkWidget *window;
  BtkTreeModel *model;

  model = btk_tree_view_get_model (tree_view);
  
  btk_tree_model_get_iter (model, &iter, path);
  btk_tree_model_get (BTK_TREE_MODEL (model),
		      &iter,
		      FUNC_COLUMN, &func,
		      STYLE_COLUMN, &style,
		      -1);

  if (func)
    {
      btk_tree_store_set (BTK_TREE_STORE (model),
			  &iter,
			  STYLE_COLUMN, (style == BANGO_STYLE_ITALIC ? BANGO_STYLE_NORMAL : BANGO_STYLE_ITALIC),
			  -1);
      window = (func) (btk_widget_get_toplevel (BTK_WIDGET (tree_view)));
      
      if (window != NULL)
	{
	  CallbackData *cbdata;
	  
	  cbdata = g_new (CallbackData, 1);
	  cbdata->model = model;
	  cbdata->path = btk_tree_path_copy (path);
	  
	  g_signal_connect (window, "destroy",
			    G_CALLBACK (window_closed_cb), cbdata);
	}
    }
}

static void
selection_cb (BtkTreeSelection *selection,
	      BtkTreeModel     *model)
{
  BtkTreeIter iter;
  GValue value = {0, };

  if (! btk_tree_selection_get_selected (selection, NULL, &iter))
    return;

  btk_tree_model_get_value (model, &iter,
			    FILENAME_COLUMN,
			    &value);
  if (g_value_get_string (&value))
    load_file (g_value_get_string (&value));
  g_value_unset (&value);
}

static BtkWidget *
create_text (BtkTextBuffer **buffer,
	     gboolean        is_source)
{
  BtkWidget *scrolled_window;
  BtkWidget *text_view;
  BangoFontDescription *font_desc;

  scrolled_window = btk_scrolled_window_new (NULL, NULL);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (scrolled_window),
				  BTK_POLICY_AUTOMATIC,
				  BTK_POLICY_AUTOMATIC);
  btk_scrolled_window_set_shadow_type (BTK_SCROLLED_WINDOW (scrolled_window),
				       BTK_SHADOW_IN);
  
  text_view = btk_text_view_new ();
  
  *buffer = btk_text_buffer_new (NULL);
  btk_text_view_set_buffer (BTK_TEXT_VIEW (text_view), *buffer);
  btk_text_view_set_editable (BTK_TEXT_VIEW (text_view), FALSE);
  btk_text_view_set_cursor_visible (BTK_TEXT_VIEW (text_view), FALSE);

  btk_container_add (BTK_CONTAINER (scrolled_window), text_view);
  
  if (is_source)
    {
      font_desc = bango_font_description_from_string ("monospace");
      btk_widget_modify_font (text_view, font_desc);
      bango_font_description_free (font_desc);

      btk_text_view_set_wrap_mode (BTK_TEXT_VIEW (text_view),
                                   BTK_WRAP_NONE);
    }
  else
    {
      /* Make it a bit nicer for text. */
      btk_text_view_set_wrap_mode (BTK_TEXT_VIEW (text_view),
                                   BTK_WRAP_WORD);
      btk_text_view_set_pixels_above_lines (BTK_TEXT_VIEW (text_view),
                                            2);
      btk_text_view_set_pixels_below_lines (BTK_TEXT_VIEW (text_view),
                                            2);
    }
  
  return scrolled_window;
}

static BtkWidget *
create_tree (void)
{
  BtkTreeSelection *selection;
  BtkCellRenderer *cell;
  BtkWidget *tree_view;
  BtkTreeViewColumn *column;
  BtkTreeStore *model;
  BtkTreeIter iter;
  BtkWidget *box, *label, *scrolled_window;

  Demo *d = testbtk_demos;

  model = btk_tree_store_new (NUM_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_INT);
  tree_view = btk_tree_view_new ();
  btk_tree_view_set_model (BTK_TREE_VIEW (tree_view), BTK_TREE_MODEL (model));
  selection = btk_tree_view_get_selection (BTK_TREE_VIEW (tree_view));

  btk_tree_selection_set_mode (BTK_TREE_SELECTION (selection),
			       BTK_SELECTION_BROWSE);
  btk_widget_set_size_request (tree_view, 200, -1);

  /* this code only supports 1 level of children. If we
   * want more we probably have to use a recursing function.
   */
  while (d->title)
    {
      Demo *children = d->children;

      btk_tree_store_append (BTK_TREE_STORE (model), &iter, NULL);

      btk_tree_store_set (BTK_TREE_STORE (model),
			  &iter,
			  TITLE_COLUMN, d->title,
			  FILENAME_COLUMN, d->filename,
			  FUNC_COLUMN, d->func,
			  STYLE_COLUMN, BANGO_STYLE_NORMAL,
			  -1);

      d++;

      if (!children)
	continue;
      
      while (children->title)
	{
	  BtkTreeIter child_iter;

	  btk_tree_store_append (BTK_TREE_STORE (model), &child_iter, &iter);
	  
	  btk_tree_store_set (BTK_TREE_STORE (model),
			      &child_iter,
			      TITLE_COLUMN, children->title,
			      FILENAME_COLUMN, children->filename,
			      FUNC_COLUMN, children->func,
			      STYLE_COLUMN, BANGO_STYLE_NORMAL,
			      -1);
	  
	  children++;
	}
    }

  cell = btk_cell_renderer_text_new ();

  column = btk_tree_view_column_new_with_attributes ("Widget (double click for demo)",
						     cell,
						     "text", TITLE_COLUMN,
						     "style", STYLE_COLUMN,
						     NULL);
  
  btk_tree_view_append_column (BTK_TREE_VIEW (tree_view),
			       BTK_TREE_VIEW_COLUMN (column));

  btk_tree_model_get_iter_first (BTK_TREE_MODEL (model), &iter);
  btk_tree_selection_select_iter (BTK_TREE_SELECTION (selection), &iter);

  g_signal_connect (selection, "changed", G_CALLBACK (selection_cb), model);
  g_signal_connect (tree_view, "row_activated", G_CALLBACK (row_activated_cb), model);

  btk_tree_view_collapse_all (BTK_TREE_VIEW (tree_view));
  btk_tree_view_set_headers_visible (BTK_TREE_VIEW (tree_view), FALSE);
  				    
  scrolled_window = btk_scrolled_window_new (NULL, NULL);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (scrolled_window),
				  BTK_POLICY_NEVER,
				  BTK_POLICY_AUTOMATIC);
  btk_container_add (BTK_CONTAINER (scrolled_window), tree_view);

  label = btk_label_new ("Widget (double click for demo)");

  box = btk_notebook_new ();
  btk_notebook_append_page (BTK_NOTEBOOK (box), scrolled_window, label);

  btk_widget_grab_focus (tree_view);

   g_object_unref (model);

  return box;
}

static void
setup_default_icon (void)
{
  BdkPixbuf *pixbuf;
  char *filename;
  GError *err;

  err = NULL;

  pixbuf = NULL;
  filename = demo_find_file ("btk-logo-rgb.gif", &err);
  if (filename)
    {
      pixbuf = bdk_pixbuf_new_from_file (filename, &err);
      g_free (filename);
    }

  /* Ignoring this error (passing NULL instead of &err above)
   * would probably be reasonable for most apps.  We're just
   * showing off.
   */
  if (err)
    {
      BtkWidget *dialog;
      
      dialog = btk_message_dialog_new (NULL, 0,
				       BTK_MESSAGE_ERROR,
				       BTK_BUTTONS_CLOSE,
				       "Failed to read icon file: %s",
				       err->message);
      g_error_free (err);

      g_signal_connect (dialog, "response",
			G_CALLBACK (btk_widget_destroy), NULL);
    }

  if (pixbuf)
    {
      GList *list;      
      BdkPixbuf *transparent;

      /* The btk-logo-rgb icon has a white background, make it transparent */
      transparent = bdk_pixbuf_add_alpha (pixbuf, TRUE, 0xff, 0xff, 0xff);

      list = NULL;
      list = g_list_append (list, transparent);
      btk_window_set_default_icon_list (list);
      g_list_free (list);
      g_object_unref (pixbuf);
      g_object_unref (transparent);
    }
}

int
main (int argc, char **argv)
{
  BtkWidget *window;
  BtkWidget *notebook;
  BtkWidget *hbox;
  BtkWidget *tree;
  BtkTextTag *tag;

  /* Most code in btk-demo is intended to be exemplary, but not
   * these few lines, which are just a hack so btk-demo will work
   * in the BTK tree without installing it.
   */
  if (g_file_test ("../../bdk-pixbuf/libpixbufloader-pnm.la",
                   G_FILE_TEST_EXISTS))
    {
      g_setenv ("BDK_PIXBUF_MODULE_FILE", "../../bdk-pixbuf/bdk-pixbuf.loaders", TRUE);
      g_setenv ("BTK_IM_MODULE_FILE", "../../modules/input/immodules.cache", TRUE);
    }
  /* -- End of hack -- */
  
  btk_init (&argc, &argv);

  setup_default_icon ();
  
  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  btk_window_set_title (BTK_WINDOW (window), "BTK+ Code Demos");
  g_signal_connect_after (window, "destroy",
		    G_CALLBACK (btk_main_quit), NULL);

  hbox = btk_hbox_new (FALSE, 0);
  btk_container_add (BTK_CONTAINER (window), hbox);

  tree = create_tree ();
  btk_box_pack_start (BTK_BOX (hbox), tree, FALSE, FALSE, 0);

  notebook = btk_notebook_new ();
  btk_box_pack_start (BTK_BOX (hbox), notebook, TRUE, TRUE, 0);

  btk_notebook_append_page (BTK_NOTEBOOK (notebook),
			    create_text (&info_buffer, FALSE),
			    btk_label_new_with_mnemonic ("_Info"));

  tag = btk_text_buffer_create_tag (info_buffer, "title",
                                    "font", "Sans 18",
                                    NULL);
   g_object_unref (info_buffer);

  btk_notebook_append_page (BTK_NOTEBOOK (notebook),
			    create_text (&source_buffer, TRUE),
			    btk_label_new_with_mnemonic ("_Source"));


  tag = btk_text_buffer_create_tag (source_buffer, "comment",
				    "foreground", "DodgerBlue",
                                    NULL);
  tag = btk_text_buffer_create_tag (source_buffer, "type",
				    "foreground", "ForestGreen",
                                    NULL);
  tag = btk_text_buffer_create_tag (source_buffer, "string",
				    "foreground", "RosyBrown",
				    "weight", BANGO_WEIGHT_BOLD,
                                    NULL);
  tag = btk_text_buffer_create_tag (source_buffer, "control",
				    "foreground", "purple",
                                    NULL);
  tag = btk_text_buffer_create_tag (source_buffer, "preprocessor",
				    "style", BANGO_STYLE_OBLIQUE,
 				    "foreground", "burlywood4",
                                    NULL);
  tag = btk_text_buffer_create_tag (source_buffer, "function",
				    "weight", BANGO_WEIGHT_BOLD,
 				    "foreground", "DarkGoldenrod4",
                                    NULL);
   g_object_unref (source_buffer);
  
  btk_window_set_default_size (BTK_WINDOW (window), 600, 400);
  btk_widget_show_all (window);
  

  load_file (testbtk_demos[0].filename);
  
  btk_main ();

  return 0;
}
