/* btkentrycompletion.c
 * Copyright (C) 2003  Kristian Rietveld  <kris@btk.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include "btkentrycompletion.h"
#include "btkentryprivate.h"
#include "btkcelllayout.h"

#include "btkintl.h"
#include "btkcellrenderertext.h"
#include "btkframe.h"
#include "btktreeselection.h"
#include "btktreeview.h"
#include "btkscrolledwindow.h"
#include "btkvbox.h"
#include "btkwindow.h"
#include "btkentry.h"
#include "btkmain.h"
#include "btkmarshalers.h"

#include "btkprivate.h"
#include "btkalias.h"

#include <string.h>


/* signals */
enum
{
  INSERT_PREFIX,
  MATCH_SELECTED,
  ACTION_ACTIVATED,
  CURSOR_ON_MATCH,
  LAST_SIGNAL
};

/* properties */
enum
{
  PROP_0,
  PROP_MODEL,
  PROP_MINIMUM_KEY_LENGTH,
  PROP_TEXT_COLUMN,
  PROP_INLINE_COMPLETION,
  PROP_POPUP_COMPLETION,
  PROP_POPUP_SET_WIDTH,
  PROP_POPUP_SINGLE_MATCH,
  PROP_INLINE_SELECTION
};

#define BTK_ENTRY_COMPLETION_GET_PRIVATE(obj)(B_TYPE_INSTANCE_GET_PRIVATE ((obj), BTK_TYPE_ENTRY_COMPLETION, BtkEntryCompletionPrivate))

static void                                                             btk_entry_completion_cell_layout_init    (BtkCellLayoutIface      *iface);
static void     btk_entry_completion_set_property        (BObject      *object,
                                                          guint         prop_id,
                                                          const BValue *value,
                                                          BParamSpec   *pspec);
static void     btk_entry_completion_get_property        (BObject      *object,
                                                          guint         prop_id,
                                                          BValue       *value,
                                                          BParamSpec   *pspec);
static void                                                             btk_entry_completion_finalize            (BObject                 *object);

static void     btk_entry_completion_pack_start          (BtkCellLayout         *cell_layout,
                                                          BtkCellRenderer       *cell,
                                                          gboolean               expand);
static void     btk_entry_completion_pack_end            (BtkCellLayout         *cell_layout,
                                                          BtkCellRenderer       *cell,
                                                          gboolean               expand);
static void                                                                      btk_entry_completion_clear               (BtkCellLayout           *cell_layout);
static void     btk_entry_completion_add_attribute       (BtkCellLayout         *cell_layout,
                                                          BtkCellRenderer       *cell,
                                                          const char            *attribute,
                                                          gint                   column);
static void     btk_entry_completion_set_cell_data_func  (BtkCellLayout         *cell_layout,
                                                          BtkCellRenderer       *cell,
                                                          BtkCellLayoutDataFunc  func,
                                                          gpointer               func_data,
                                                          GDestroyNotify         destroy);
static void     btk_entry_completion_clear_attributes    (BtkCellLayout         *cell_layout,
                                                          BtkCellRenderer       *cell);
static void     btk_entry_completion_reorder             (BtkCellLayout         *cell_layout,
                                                          BtkCellRenderer       *cell,
                                                          gint                   position);
static GList *  btk_entry_completion_get_cells           (BtkCellLayout         *cell_layout);

static gboolean btk_entry_completion_visible_func        (BtkTreeModel       *model,
                                                          BtkTreeIter        *iter,
                                                          gpointer            data);
static gboolean btk_entry_completion_popup_key_event     (BtkWidget          *widget,
                                                          BdkEventKey        *event,
                                                          gpointer            user_data);
static gboolean btk_entry_completion_popup_button_press  (BtkWidget          *widget,
                                                          BdkEventButton     *event,
                                                          gpointer            user_data);
static gboolean btk_entry_completion_list_button_press   (BtkWidget          *widget,
                                                          BdkEventButton     *event,
                                                          gpointer            user_data);
static gboolean btk_entry_completion_action_button_press (BtkWidget          *widget,
                                                          BdkEventButton     *event,
                                                          gpointer            user_data);
static void     btk_entry_completion_selection_changed   (BtkTreeSelection   *selection,
                                                          gpointer            data);
static gboolean	btk_entry_completion_list_enter_notify	 (BtkWidget          *widget,
							  BdkEventCrossing   *event,
							  gpointer            data);
static gboolean btk_entry_completion_list_motion_notify	 (BtkWidget	     *widget,
							  BdkEventMotion     *event,
							  gpointer 	      data);
static void     btk_entry_completion_insert_action       (BtkEntryCompletion *completion,
                                                          gint                index,
                                                          const gchar        *string,
                                                          gboolean            markup);
static void     btk_entry_completion_action_data_func    (BtkTreeViewColumn  *tree_column,
                                                          BtkCellRenderer    *cell,
                                                          BtkTreeModel       *model,
                                                          BtkTreeIter        *iter,
                                                          gpointer            data);

static gboolean btk_entry_completion_match_selected      (BtkEntryCompletion *completion,
							  BtkTreeModel       *model,
							  BtkTreeIter        *iter);
static gboolean btk_entry_completion_real_insert_prefix  (BtkEntryCompletion *completion,
							  const gchar        *prefix);
static gboolean btk_entry_completion_cursor_on_match     (BtkEntryCompletion *completion,
							  BtkTreeModel       *model,
							  BtkTreeIter        *iter);
static gboolean btk_entry_completion_insert_completion   (BtkEntryCompletion *completion,
                                                          BtkTreeModel       *model,
                                                          BtkTreeIter        *iter);
static void     btk_entry_completion_insert_completion_text (BtkEntryCompletion *completion,
                                                             const gchar *text);

static guint entry_completion_signals[LAST_SIGNAL] = { 0 };

/* BtkBuildable */
static void     btk_entry_completion_buildable_init      (BtkBuildableIface  *iface);

G_DEFINE_TYPE_WITH_CODE (BtkEntryCompletion, btk_entry_completion, B_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_CELL_LAYOUT,
						btk_entry_completion_cell_layout_init)
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_BUILDABLE,
						btk_entry_completion_buildable_init))


static void
btk_entry_completion_class_init (BtkEntryCompletionClass *klass)
{
  BObjectClass *object_class;

  object_class = (BObjectClass *)klass;

  object_class->set_property = btk_entry_completion_set_property;
  object_class->get_property = btk_entry_completion_get_property;
  object_class->finalize = btk_entry_completion_finalize;

  klass->match_selected = btk_entry_completion_match_selected;
  klass->insert_prefix = btk_entry_completion_real_insert_prefix;
  klass->cursor_on_match = btk_entry_completion_cursor_on_match;

  /**
   * BtkEntryCompletion::insert-prefix:
   * @widget: the object which received the signal
   * @prefix: the common prefix of all possible completions
   *
   * Gets emitted when the inline autocompletion is triggered.
   * The default behaviour is to make the entry display the
   * whole prefix and select the newly inserted part.
   *
   * Applications may connect to this signal in order to insert only a
   * smaller part of the @prefix into the entry - e.g. the entry used in
   * the #BtkFileChooser inserts only the part of the prefix up to the
   * next '/'.
   *
   * Return value: %TRUE if the signal has been handled
   *
   * Since: 2.6
   */
  entry_completion_signals[INSERT_PREFIX] =
    g_signal_new (I_("insert-prefix"),
                  B_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BtkEntryCompletionClass, insert_prefix),
                  _btk_boolean_handled_accumulator, NULL,
                  _btk_marshal_BOOLEAN__STRING,
                  B_TYPE_BOOLEAN, 1,
                  B_TYPE_STRING);

  /**
   * BtkEntryCompletion::match-selected:
   * @widget: the object which received the signal
   * @model: the #BtkTreeModel containing the matches
   * @iter: a #BtkTreeIter positioned at the selected match
   *
   * Gets emitted when a match from the list is selected.
   * The default behaviour is to replace the contents of the
   * entry with the contents of the text column in the row
   * pointed to by @iter.
   *
   * Return value: %TRUE if the signal has been handled
   *
   * Since: 2.4
   */
  entry_completion_signals[MATCH_SELECTED] =
    g_signal_new (I_("match-selected"),
                  B_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BtkEntryCompletionClass, match_selected),
                  _btk_boolean_handled_accumulator, NULL,
                  _btk_marshal_BOOLEAN__OBJECT_BOXED,
                  B_TYPE_BOOLEAN, 2,
                  BTK_TYPE_TREE_MODEL,
                  BTK_TYPE_TREE_ITER);

  /**
   * BtkEntryCompletion::cursor-on-match:
   * @widget: the object which received the signal
   * @model: the #BtkTreeModel containing the matches
   * @iter: a #BtkTreeIter positioned at the selected match
   *
   * Gets emitted when a match from the cursor is on a match
   * of the list. The default behaviour is to replace the contents
   * of the entry with the contents of the text column in the row
   * pointed to by @iter.
   *
   * Return value: %TRUE if the signal has been handled
   *
   * Since: 2.12
   */
  entry_completion_signals[CURSOR_ON_MATCH] =
    g_signal_new (I_("cursor-on-match"),
		  B_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkEntryCompletionClass, cursor_on_match),
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__OBJECT_BOXED,
		  B_TYPE_BOOLEAN, 2,
		  BTK_TYPE_TREE_MODEL,
		  BTK_TYPE_TREE_ITER);

  /**
   * BtkEntryCompletion::action-activated:
   * @widget: the object which received the signal
   * @index: the index of the activated action
   *
   * Gets emitted when an action is activated.
   * 
   * Since: 2.4
   */
  entry_completion_signals[ACTION_ACTIVATED] =
    g_signal_new (I_("action-activated"),
                  B_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BtkEntryCompletionClass, action_activated),
                  NULL, NULL,
                  _btk_marshal_VOID__INT,
                  B_TYPE_NONE, 1,
                  B_TYPE_INT);

  g_object_class_install_property (object_class,
                                   PROP_MODEL,
                                   g_param_spec_object ("model",
                                                        P_("Completion Model"),
                                                        P_("The model to find matches in"),
                                                        BTK_TYPE_TREE_MODEL,
                                                        BTK_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_MINIMUM_KEY_LENGTH,
                                   g_param_spec_int ("minimum-key-length",
                                                     P_("Minimum Key Length"),
                                                     P_("Minimum length of the search key in order to look up matches"),
                                                     0,
                                                     G_MAXINT,
                                                     1,
                                                     BTK_PARAM_READWRITE));
  /**
   * BtkEntryCompletion:text-column:
   *
   * The column of the model containing the strings.
   * Note that the strings must be UTF-8.
   *
   * Since: 2.6
   */
  g_object_class_install_property (object_class,
                                   PROP_TEXT_COLUMN,
                                   g_param_spec_int ("text-column",
                                                     P_("Text column"),
                                                     P_("The column of the model containing the strings."),
                                                     -1,
                                                     G_MAXINT,
                                                     -1,
                                                     BTK_PARAM_READWRITE));

  /**
   * BtkEntryCompletion:inline-completion:
   * 
   * Determines whether the common prefix of the possible completions 
   * should be inserted automatically in the entry. Note that this
   * requires text-column to be set, even if you are using a custom
   * match function.
   *
   * Since: 2.6
   **/
  g_object_class_install_property (object_class,
				   PROP_INLINE_COMPLETION,
				   g_param_spec_boolean ("inline-completion",
 							 P_("Inline completion"),
 							 P_("Whether the common prefix should be inserted automatically"),
 							 FALSE,
 							 BTK_PARAM_READWRITE));
  /**
   * BtkEntryCompletion:popup-completion:
   * 
   * Determines whether the possible completions should be 
   * shown in a popup window. 
   *
   * Since: 2.6
   **/
  g_object_class_install_property (object_class,
				   PROP_POPUP_COMPLETION,
				   g_param_spec_boolean ("popup-completion",
 							 P_("Popup completion"),
 							 P_("Whether the completions should be shown in a popup window"),
 							 TRUE,
 							 BTK_PARAM_READWRITE));

  /**
   * BtkEntryCompletion:popup-set-width:
   * 
   * Determines whether the completions popup window will be
   * resized to the width of the entry.
   *
   * Since: 2.8
   */
  g_object_class_install_property (object_class,
				   PROP_POPUP_SET_WIDTH,
				   g_param_spec_boolean ("popup-set-width",
 							 P_("Popup set width"),
 							 P_("If TRUE, the popup window will have the same size as the entry"),
 							 TRUE,
 							 BTK_PARAM_READWRITE));

  /**
   * BtkEntryCompletion:popup-single-match:
   * 
   * Determines whether the completions popup window will shown
   * for a single possible completion. You probably want to set
   * this to %FALSE if you are using 
   * <link linkend="BtkEntryCompletion--inline-completion">inline 
   * completion</link>.
   *
   * Since: 2.8
   */
  g_object_class_install_property (object_class,
				   PROP_POPUP_SINGLE_MATCH,
				   g_param_spec_boolean ("popup-single-match",
 							 P_("Popup single match"),
 							 P_("If TRUE, the popup window will appear for a single match."),
 							 TRUE,
 							 BTK_PARAM_READWRITE));
  /**
   * BtkEntryCompletion:inline-selection:
   * 
   * Determines whether the possible completions on the popup
   * will appear in the entry as you navigate through them.
   
   * Since: 2.12
   */
  g_object_class_install_property (object_class,
				   PROP_INLINE_SELECTION,
				   g_param_spec_boolean ("inline-selection",
							 P_("Inline selection"),
							 P_("Your description here"),
							 FALSE,
							 BTK_PARAM_READWRITE));

  g_type_class_add_private (object_class, sizeof (BtkEntryCompletionPrivate));
}

static void
btk_entry_completion_buildable_init (BtkBuildableIface *iface)
{
  iface->add_child = _btk_cell_layout_buildable_add_child;
  iface->custom_tag_start = _btk_cell_layout_buildable_custom_tag_start;
  iface->custom_tag_end = _btk_cell_layout_buildable_custom_tag_end;
}

static void
btk_entry_completion_cell_layout_init (BtkCellLayoutIface *iface)
{
  iface->pack_start = btk_entry_completion_pack_start;
  iface->pack_end = btk_entry_completion_pack_end;
  iface->clear = btk_entry_completion_clear;
  iface->add_attribute = btk_entry_completion_add_attribute;
  iface->set_cell_data_func = btk_entry_completion_set_cell_data_func;
  iface->clear_attributes = btk_entry_completion_clear_attributes;
  iface->reorder = btk_entry_completion_reorder;
  iface->get_cells = btk_entry_completion_get_cells;
}

static void
btk_entry_completion_init (BtkEntryCompletion *completion)
{
  BtkCellRenderer *cell;
  BtkTreeSelection *sel;
  BtkEntryCompletionPrivate *priv;
  BtkWidget *popup_frame;

  /* yes, also priv, need to keep the code readable */
  priv = completion->priv = BTK_ENTRY_COMPLETION_GET_PRIVATE (completion);

  priv->minimum_key_length = 1;
  priv->text_column = -1;
  priv->has_completion = FALSE;
  priv->inline_completion = FALSE;
  priv->popup_completion = TRUE;
  priv->popup_set_width = TRUE;
  priv->popup_single_match = TRUE;
  priv->inline_selection = FALSE;

  /* completions */
  priv->filter_model = NULL;

  priv->tree_view = btk_tree_view_new ();
  g_signal_connect (priv->tree_view, "button-press-event",
                    G_CALLBACK (btk_entry_completion_list_button_press),
                    completion);
  g_signal_connect (priv->tree_view, "enter-notify-event",
		    G_CALLBACK (btk_entry_completion_list_enter_notify),
		    completion);
  g_signal_connect (priv->tree_view, "motion-notify-event",
		    G_CALLBACK (btk_entry_completion_list_motion_notify),
		    completion);

  btk_tree_view_set_headers_visible (BTK_TREE_VIEW (priv->tree_view), FALSE);
  btk_tree_view_set_hover_selection (BTK_TREE_VIEW (priv->tree_view), TRUE);

  sel = btk_tree_view_get_selection (BTK_TREE_VIEW (priv->tree_view));
  btk_tree_selection_set_mode (sel, BTK_SELECTION_SINGLE);
  btk_tree_selection_unselect_all (sel);
  g_signal_connect (sel, "changed",
                    G_CALLBACK (btk_entry_completion_selection_changed),
                    completion);
  priv->first_sel_changed = TRUE;

  priv->column = btk_tree_view_column_new ();
  btk_tree_view_append_column (BTK_TREE_VIEW (priv->tree_view), priv->column);

  priv->scrolled_window = btk_scrolled_window_new (NULL, NULL);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (priv->scrolled_window),
                                  BTK_POLICY_NEVER,
                                  BTK_POLICY_AUTOMATIC);
  btk_scrolled_window_set_shadow_type (BTK_SCROLLED_WINDOW (priv->scrolled_window),
                                       BTK_SHADOW_NONE);

  /* a nasty hack to get the completions treeview to size nicely */
  btk_widget_set_size_request (BTK_SCROLLED_WINDOW (priv->scrolled_window)->vscrollbar, -1, 0);

  /* actions */
  priv->actions = btk_list_store_new (2, B_TYPE_STRING, B_TYPE_BOOLEAN);

  priv->action_view =
    btk_tree_view_new_with_model (BTK_TREE_MODEL (priv->actions));
  g_object_ref_sink (priv->action_view);
  g_signal_connect (priv->action_view, "button-press-event",
                    G_CALLBACK (btk_entry_completion_action_button_press),
                    completion);
  g_signal_connect (priv->action_view, "enter-notify-event",
		    G_CALLBACK (btk_entry_completion_list_enter_notify),
		    completion);
  g_signal_connect (priv->action_view, "motion-notify-event",
		    G_CALLBACK (btk_entry_completion_list_motion_notify),
		    completion);
  btk_tree_view_set_headers_visible (BTK_TREE_VIEW (priv->action_view), FALSE);
  btk_tree_view_set_hover_selection (BTK_TREE_VIEW (priv->action_view), TRUE);

  sel = btk_tree_view_get_selection (BTK_TREE_VIEW (priv->action_view));
  btk_tree_selection_set_mode (sel, BTK_SELECTION_SINGLE);
  btk_tree_selection_unselect_all (sel);

  cell = btk_cell_renderer_text_new ();
  btk_tree_view_insert_column_with_data_func (BTK_TREE_VIEW (priv->action_view),
                                              0, "",
                                              cell,
                                              btk_entry_completion_action_data_func,
                                              NULL,
                                              NULL);

  /* pack it all */
  priv->popup_window = btk_window_new (BTK_WINDOW_POPUP);
  btk_window_set_resizable (BTK_WINDOW (priv->popup_window), FALSE);
  btk_window_set_type_hint (BTK_WINDOW(priv->popup_window),
                            BDK_WINDOW_TYPE_HINT_COMBO);
  g_signal_connect (priv->popup_window, "key-press-event",
                    G_CALLBACK (btk_entry_completion_popup_key_event),
                    completion);
  g_signal_connect (priv->popup_window, "key-release-event",
                    G_CALLBACK (btk_entry_completion_popup_key_event),
                    completion);
  g_signal_connect (priv->popup_window, "button-press-event",
                    G_CALLBACK (btk_entry_completion_popup_button_press),
                    completion);

  popup_frame = btk_frame_new (NULL);
  btk_frame_set_shadow_type (BTK_FRAME (popup_frame),
			     BTK_SHADOW_ETCHED_IN);
  btk_widget_show (popup_frame);
  btk_container_add (BTK_CONTAINER (priv->popup_window), popup_frame);
  
  priv->vbox = btk_vbox_new (FALSE, 0);
  btk_container_add (BTK_CONTAINER (popup_frame), priv->vbox);

  btk_container_add (BTK_CONTAINER (priv->scrolled_window), priv->tree_view);
  btk_box_pack_start (BTK_BOX (priv->vbox), priv->scrolled_window,
                      TRUE, TRUE, 0);

  /* we don't want to see the action treeview when no actions have
   * been inserted, so we pack the action treeview after the first
   * action has been added
   */
}

static void
btk_entry_completion_set_property (BObject      *object,
                                   guint         prop_id,
                                   const BValue *value,
                                   BParamSpec   *pspec)
{
  BtkEntryCompletion *completion = BTK_ENTRY_COMPLETION (object);
  BtkEntryCompletionPrivate *priv = completion->priv;

  switch (prop_id)
    {
      case PROP_MODEL:
        btk_entry_completion_set_model (completion,
                                        b_value_get_object (value));
        break;

      case PROP_MINIMUM_KEY_LENGTH:
        btk_entry_completion_set_minimum_key_length (completion,
                                                     b_value_get_int (value));
        break;

      case PROP_TEXT_COLUMN:
	priv->text_column = b_value_get_int (value);
        break;

      case PROP_INLINE_COMPLETION:
	priv->inline_completion = b_value_get_boolean (value);
        break;

      case PROP_POPUP_COMPLETION:
	priv->popup_completion = b_value_get_boolean (value);
        break;

      case PROP_POPUP_SET_WIDTH:
	priv->popup_set_width = b_value_get_boolean (value);
        break;

      case PROP_POPUP_SINGLE_MATCH:
	priv->popup_single_match = b_value_get_boolean (value);
        break;

      case PROP_INLINE_SELECTION:
        priv->inline_selection = b_value_get_boolean (value);
        break;
      
      default:
        B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
btk_entry_completion_get_property (BObject    *object,
                                   guint       prop_id,
                                   BValue     *value,
                                   BParamSpec *pspec)
{
  BtkEntryCompletion *completion = BTK_ENTRY_COMPLETION (object);

  switch (prop_id)
    {
      case PROP_MODEL:
        b_value_set_object (value,
                            btk_entry_completion_get_model (completion));
        break;

      case PROP_MINIMUM_KEY_LENGTH:
        b_value_set_int (value, btk_entry_completion_get_minimum_key_length (completion));
        break;

      case PROP_TEXT_COLUMN:
        b_value_set_int (value, btk_entry_completion_get_text_column (completion));
        break;

      case PROP_INLINE_COMPLETION:
        b_value_set_boolean (value, btk_entry_completion_get_inline_completion (completion));
        break;

      case PROP_POPUP_COMPLETION:
        b_value_set_boolean (value, btk_entry_completion_get_popup_completion (completion));
        break;

      case PROP_POPUP_SET_WIDTH:
        b_value_set_boolean (value, btk_entry_completion_get_popup_set_width (completion));
        break;

      case PROP_POPUP_SINGLE_MATCH:
        b_value_set_boolean (value, btk_entry_completion_get_popup_single_match (completion));
        break;

      case PROP_INLINE_SELECTION:
        b_value_set_boolean (value, btk_entry_completion_get_inline_selection (completion));
        break;

      default:
        B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
btk_entry_completion_finalize (BObject *object)
{
  BtkEntryCompletion *completion = BTK_ENTRY_COMPLETION (object);
  BtkEntryCompletionPrivate *priv = completion->priv;

  if (priv->tree_view)
    btk_widget_destroy (priv->tree_view);

  if (priv->entry)
    btk_entry_set_completion (BTK_ENTRY (priv->entry), NULL);

  if (priv->actions)
    g_object_unref (priv->actions);
  if (priv->action_view)
    g_object_unref (priv->action_view);

  g_free (priv->case_normalized_key);
  g_free (priv->completion_prefix);

  if (priv->popup_window)
    btk_widget_destroy (priv->popup_window);

  if (priv->match_notify)
    (* priv->match_notify) (priv->match_data);

  B_OBJECT_CLASS (btk_entry_completion_parent_class)->finalize (object);
}

/* implement cell layout interface */
static void
btk_entry_completion_pack_start (BtkCellLayout   *cell_layout,
                                 BtkCellRenderer *cell,
                                 gboolean         expand)
{
  BtkEntryCompletionPrivate *priv;

  priv = BTK_ENTRY_COMPLETION_GET_PRIVATE (cell_layout);

  btk_tree_view_column_pack_start (priv->column, cell, expand);
}

static void
btk_entry_completion_pack_end (BtkCellLayout   *cell_layout,
                               BtkCellRenderer *cell,
                               gboolean         expand)
{
  BtkEntryCompletionPrivate *priv;

  priv = BTK_ENTRY_COMPLETION_GET_PRIVATE (cell_layout);

  btk_tree_view_column_pack_end (priv->column, cell, expand);
}

static void
btk_entry_completion_clear (BtkCellLayout *cell_layout)
{
  BtkEntryCompletionPrivate *priv;

  priv = BTK_ENTRY_COMPLETION_GET_PRIVATE (cell_layout);

  btk_tree_view_column_clear (priv->column);
}

static void
btk_entry_completion_add_attribute (BtkCellLayout   *cell_layout,
                                    BtkCellRenderer *cell,
                                    const gchar     *attribute,
                                    gint             column)
{
  BtkEntryCompletionPrivate *priv;

  priv = BTK_ENTRY_COMPLETION_GET_PRIVATE (cell_layout);

  btk_tree_view_column_add_attribute (priv->column, cell, attribute, column);
}

static void
btk_entry_completion_set_cell_data_func (BtkCellLayout          *cell_layout,
                                         BtkCellRenderer        *cell,
                                         BtkCellLayoutDataFunc   func,
                                         gpointer                func_data,
                                         GDestroyNotify          destroy)
{
  BtkEntryCompletionPrivate *priv;

  priv = BTK_ENTRY_COMPLETION_GET_PRIVATE (cell_layout);

  btk_cell_layout_set_cell_data_func (BTK_CELL_LAYOUT (priv->column),
                                      cell, func, func_data, destroy);
}

static void
btk_entry_completion_clear_attributes (BtkCellLayout   *cell_layout,
                                       BtkCellRenderer *cell)
{
  BtkEntryCompletionPrivate *priv;

  priv = BTK_ENTRY_COMPLETION_GET_PRIVATE (cell_layout);

  btk_tree_view_column_clear_attributes (priv->column, cell);
}

static void
btk_entry_completion_reorder (BtkCellLayout   *cell_layout,
                              BtkCellRenderer *cell,
                              gint             position)
{
  BtkEntryCompletionPrivate *priv;

  priv = BTK_ENTRY_COMPLETION_GET_PRIVATE (cell_layout);

  btk_cell_layout_reorder (BTK_CELL_LAYOUT (priv->column), cell, position);
}

static GList *
btk_entry_completion_get_cells (BtkCellLayout *cell_layout)
{
  BtkEntryCompletionPrivate *priv;

  priv = BTK_ENTRY_COMPLETION_GET_PRIVATE (cell_layout);

  return btk_cell_layout_get_cells (BTK_CELL_LAYOUT (priv->column));
}

/* all those callbacks */
static gboolean
btk_entry_completion_default_completion_func (BtkEntryCompletion *completion,
                                              const gchar        *key,
                                              BtkTreeIter        *iter,
                                              gpointer            user_data)
{
  gchar *item = NULL;
  gchar *normalized_string;
  gchar *case_normalized_string;

  gboolean ret = FALSE;

  BtkTreeModel *model;

  model = btk_tree_model_filter_get_model (completion->priv->filter_model);

  g_return_val_if_fail (btk_tree_model_get_column_type (model, completion->priv->text_column) == B_TYPE_STRING, 
			FALSE);

  btk_tree_model_get (model, iter,
                      completion->priv->text_column, &item,
                      -1);

  if (item != NULL)
    {
      normalized_string = g_utf8_normalize (item, -1, G_NORMALIZE_ALL);

      if (normalized_string != NULL)
        {
          case_normalized_string = g_utf8_casefold (normalized_string, -1);

          if (!strncmp (key, case_normalized_string, strlen (key)))
	    ret = TRUE;

          g_free (case_normalized_string);
        }
      g_free (normalized_string);
    }
  g_free (item);

  return ret;
}

static gboolean
btk_entry_completion_visible_func (BtkTreeModel *model,
                                   BtkTreeIter  *iter,
                                   gpointer      data)
{
  gboolean ret = FALSE;

  BtkEntryCompletion *completion = BTK_ENTRY_COMPLETION (data);

  if (!completion->priv->case_normalized_key)
    return ret;

  if (completion->priv->match_func)
    ret = (* completion->priv->match_func) (completion,
                                            completion->priv->case_normalized_key,
                                            iter,
                                            completion->priv->match_data);
  else if (completion->priv->text_column >= 0)
    ret = btk_entry_completion_default_completion_func (completion,
                                                        completion->priv->case_normalized_key,
                                                        iter,
                                                        NULL);

  return ret;
}

static gboolean
btk_entry_completion_popup_key_event (BtkWidget   *widget,
                                      BdkEventKey *event,
                                      gpointer     user_data)
{
  BtkEntryCompletion *completion = BTK_ENTRY_COMPLETION (user_data);

  if (!btk_widget_get_mapped (completion->priv->popup_window))
    return FALSE;

  /* propagate event to the entry */
  btk_widget_event (completion->priv->entry, (BdkEvent *)event);

  return TRUE;
}

static gboolean
btk_entry_completion_popup_button_press (BtkWidget      *widget,
                                         BdkEventButton *event,
                                         gpointer        user_data)
{
  BtkEntryCompletion *completion = BTK_ENTRY_COMPLETION (user_data);

  if (!btk_widget_get_mapped (completion->priv->popup_window))
    return FALSE;

  /* if we come here, it's usually time to popdown */
  _btk_entry_completion_popdown (completion);

  return TRUE;
}

static gboolean
btk_entry_completion_list_button_press (BtkWidget      *widget,
                                        BdkEventButton *event,
                                        gpointer        user_data)
{
  BtkEntryCompletion *completion = BTK_ENTRY_COMPLETION (user_data);
  BtkTreePath *path = NULL;

  if (!btk_widget_get_mapped (completion->priv->popup_window))
    return FALSE;

  if (btk_tree_view_get_path_at_pos (BTK_TREE_VIEW (widget),
                                     event->x, event->y,
                                     &path, NULL, NULL, NULL))
    {
      BtkTreeIter iter;
      gboolean entry_set;
      BtkTreeModel *model;
      BtkTreeIter child_iter;

      btk_tree_model_get_iter (BTK_TREE_MODEL (completion->priv->filter_model),
                               &iter, path);
      btk_tree_path_free (path);
      btk_tree_model_filter_convert_iter_to_child_iter (completion->priv->filter_model,
                                                        &child_iter,
                                                        &iter);
      model = btk_tree_model_filter_get_model (completion->priv->filter_model);

      g_signal_handler_block (completion->priv->entry,
                              completion->priv->changed_id);
      g_signal_emit (completion, entry_completion_signals[MATCH_SELECTED],
                     0, model, &child_iter, &entry_set);
      g_signal_handler_unblock (completion->priv->entry,
                                completion->priv->changed_id);

      _btk_entry_completion_popdown (completion);

      return TRUE;
    }

  return FALSE;
}

static gboolean
btk_entry_completion_action_button_press (BtkWidget      *widget,
                                          BdkEventButton *event,
                                          gpointer        user_data)
{
  BtkEntryCompletion *completion = BTK_ENTRY_COMPLETION (user_data);
  BtkTreePath *path = NULL;

  if (!btk_widget_get_mapped (completion->priv->popup_window))
    return FALSE;

  _btk_entry_reset_im_context (BTK_ENTRY (completion->priv->entry));

  if (btk_tree_view_get_path_at_pos (BTK_TREE_VIEW (widget),
                                     event->x, event->y,
                                     &path, NULL, NULL, NULL))
    {
      g_signal_emit (completion, entry_completion_signals[ACTION_ACTIVATED],
                     0, btk_tree_path_get_indices (path)[0]);
      btk_tree_path_free (path);

      _btk_entry_completion_popdown (completion);
      return TRUE;
    }

  return FALSE;
}

static void
btk_entry_completion_action_data_func (BtkTreeViewColumn *tree_column,
                                       BtkCellRenderer   *cell,
                                       BtkTreeModel      *model,
                                       BtkTreeIter       *iter,
                                       gpointer           data)
{
  gchar *string = NULL;
  gboolean markup;

  btk_tree_model_get (model, iter,
                      0, &string,
                      1, &markup,
                      -1);

  if (!string)
    return;

  if (markup)
    g_object_set (cell,
                  "text", NULL,
                  "markup", string,
                  NULL);
  else
    g_object_set (cell,
                  "markup", NULL,
                  "text", string,
                  NULL);

  g_free (string);
}

static void
btk_entry_completion_selection_changed (BtkTreeSelection *selection,
                                        gpointer          data)
{
  BtkEntryCompletion *completion = BTK_ENTRY_COMPLETION (data);

  if (completion->priv->first_sel_changed)
    {
      completion->priv->first_sel_changed = FALSE;
      if (btk_widget_is_focus (completion->priv->tree_view))
        btk_tree_selection_unselect_all (selection);
    }
}

/* public API */

/**
 * btk_entry_completion_new:
 *
 * Creates a new #BtkEntryCompletion object.
 *
 * Return value: A newly created #BtkEntryCompletion object.
 *
 * Since: 2.4
 */
BtkEntryCompletion *
btk_entry_completion_new (void)
{
  BtkEntryCompletion *completion;

  completion = g_object_new (BTK_TYPE_ENTRY_COMPLETION, NULL);

  return completion;
}

/**
 * btk_entry_completion_get_entry:
 * @completion: A #BtkEntryCompletion.
 *
 * Gets the entry @completion has been attached to.
 *
 * Return value: (transfer none): The entry @completion has been attached to.
 *
 * Since: 2.4
 */
BtkWidget *
btk_entry_completion_get_entry (BtkEntryCompletion *completion)
{
  g_return_val_if_fail (BTK_IS_ENTRY_COMPLETION (completion), NULL);

  return completion->priv->entry;
}

/**
 * btk_entry_completion_set_model:
 * @completion: A #BtkEntryCompletion.
 * @model: (allow-none): The #BtkTreeModel.
 *
 * Sets the model for a #BtkEntryCompletion. If @completion already has
 * a model set, it will remove it before setting the new model.
 * If model is %NULL, then it will unset the model.
 *
 * Since: 2.4
 */
void
btk_entry_completion_set_model (BtkEntryCompletion *completion,
                                BtkTreeModel       *model)
{
  g_return_if_fail (BTK_IS_ENTRY_COMPLETION (completion));
  g_return_if_fail (model == NULL || BTK_IS_TREE_MODEL (model));

  if (!model)
    {
      btk_tree_view_set_model (BTK_TREE_VIEW (completion->priv->tree_view),
			       NULL);
      _btk_entry_completion_popdown (completion);
      completion->priv->filter_model = NULL;
      return;
    }
     
  /* code will unref the old filter model (if any) */
  completion->priv->filter_model =
    BTK_TREE_MODEL_FILTER (btk_tree_model_filter_new (model, NULL));
  btk_tree_model_filter_set_visible_func (completion->priv->filter_model,
                                          btk_entry_completion_visible_func,
                                          completion,
                                          NULL);

  btk_tree_view_set_model (BTK_TREE_VIEW (completion->priv->tree_view),
                           BTK_TREE_MODEL (completion->priv->filter_model));
  g_object_unref (completion->priv->filter_model);

  g_object_notify (B_OBJECT (completion), "model");

  if (btk_widget_get_visible (completion->priv->popup_window))
    _btk_entry_completion_resize_popup (completion);
}

/**
 * btk_entry_completion_get_model:
 * @completion: A #BtkEntryCompletion.
 *
 * Returns the model the #BtkEntryCompletion is using as data source.
 * Returns %NULL if the model is unset.
 *
 * Return value: (transfer none): A #BtkTreeModel, or %NULL if none
 *     is currently being used.
 *
 * Since: 2.4
 */
BtkTreeModel *
btk_entry_completion_get_model (BtkEntryCompletion *completion)
{
  g_return_val_if_fail (BTK_IS_ENTRY_COMPLETION (completion), NULL);

  if (!completion->priv->filter_model)
    return NULL;

  return btk_tree_model_filter_get_model (completion->priv->filter_model);
}

/**
 * btk_entry_completion_set_match_func:
 * @completion: A #BtkEntryCompletion.
 * @func: The #BtkEntryCompletionMatchFunc to use.
 * @func_data: The user data for @func.
 * @func_notify: Destroy notifier for @func_data.
 *
 * Sets the match function for @completion to be @func. The match function
 * is used to determine if a row should or should not be in the completion
 * list.
 *
 * Since: 2.4
 */
void
btk_entry_completion_set_match_func (BtkEntryCompletion          *completion,
                                     BtkEntryCompletionMatchFunc  func,
                                     gpointer                     func_data,
                                     GDestroyNotify               func_notify)
{
  g_return_if_fail (BTK_IS_ENTRY_COMPLETION (completion));

  if (completion->priv->match_notify)
    (* completion->priv->match_notify) (completion->priv->match_data);

  completion->priv->match_func = func;
  completion->priv->match_data = func_data;
  completion->priv->match_notify = func_notify;
}

/**
 * btk_entry_completion_set_minimum_key_length:
 * @completion: A #BtkEntryCompletion.
 * @length: The minimum length of the key in order to start completing.
 *
 * Requires the length of the search key for @completion to be at least
 * @length. This is useful for long lists, where completing using a small
 * key takes a lot of time and will come up with meaningless results anyway
 * (ie, a too large dataset).
 *
 * Since: 2.4
 */
void
btk_entry_completion_set_minimum_key_length (BtkEntryCompletion *completion,
                                             gint                length)
{
  g_return_if_fail (BTK_IS_ENTRY_COMPLETION (completion));
  g_return_if_fail (length >= 0);

  if (completion->priv->minimum_key_length != length)
    {
      completion->priv->minimum_key_length = length;
     
      g_object_notify (B_OBJECT (completion), "minimum-key-length");
    }
}

/**
 * btk_entry_completion_get_minimum_key_length:
 * @completion: A #BtkEntryCompletion.
 *
 * Returns the minimum key length as set for @completion.
 *
 * Return value: The currently used minimum key length.
 *
 * Since: 2.4
 */
gint
btk_entry_completion_get_minimum_key_length (BtkEntryCompletion *completion)
{
  g_return_val_if_fail (BTK_IS_ENTRY_COMPLETION (completion), 0);

  return completion->priv->minimum_key_length;
}

/**
 * btk_entry_completion_complete:
 * @completion: A #BtkEntryCompletion.
 *
 * Requests a completion operation, or in other words a refiltering of the
 * current list with completions, using the current key. The completion list
 * view will be updated accordingly.
 *
 * Since: 2.4
 */
void
btk_entry_completion_complete (BtkEntryCompletion *completion)
{
  gchar *tmp;

  g_return_if_fail (BTK_IS_ENTRY_COMPLETION (completion));

  if (!completion->priv->filter_model)
    return;
  
  g_free (completion->priv->case_normalized_key);

  tmp = g_utf8_normalize (btk_entry_get_text (BTK_ENTRY (completion->priv->entry)),
                          -1, G_NORMALIZE_ALL);
  completion->priv->case_normalized_key = g_utf8_casefold (tmp, -1);
  g_free (tmp);

  btk_tree_model_filter_refilter (completion->priv->filter_model);

  if (btk_widget_get_visible (completion->priv->popup_window))
    _btk_entry_completion_resize_popup (completion);
}

static void
btk_entry_completion_insert_action (BtkEntryCompletion *completion,
                                    gint                index,
                                    const gchar        *string,
                                    gboolean            markup)
{
  BtkTreeIter iter;

  btk_list_store_insert (completion->priv->actions, &iter, index);
  btk_list_store_set (completion->priv->actions, &iter,
                      0, string,
                      1, markup,
                      -1);

  if (!completion->priv->action_view->parent)
    {
      BtkTreePath *path = btk_tree_path_new_from_indices (0, -1);

      btk_tree_view_set_cursor (BTK_TREE_VIEW (completion->priv->action_view),
                                path, NULL, FALSE);
      btk_tree_path_free (path);

      btk_box_pack_start (BTK_BOX (completion->priv->vbox),
                          completion->priv->action_view, FALSE, FALSE, 0);
      btk_widget_show (completion->priv->action_view);
    }
}

/**
 * btk_entry_completion_insert_action_text:
 * @completion: A #BtkEntryCompletion.
 * @index_: The index of the item to insert.
 * @text: Text of the item to insert.
 *
 * Inserts an action in @completion's action item list at position @index_
 * with text @text. If you want the action item to have markup, use
 * btk_entry_completion_insert_action_markup().
 *
 * Since: 2.4
 */
void
btk_entry_completion_insert_action_text (BtkEntryCompletion *completion,
                                         gint                index_,
                                         const gchar        *text)
{
  g_return_if_fail (BTK_IS_ENTRY_COMPLETION (completion));
  g_return_if_fail (text != NULL);

  btk_entry_completion_insert_action (completion, index_, text, FALSE);
}

/**
 * btk_entry_completion_insert_action_markup:
 * @completion: A #BtkEntryCompletion.
 * @index_: The index of the item to insert.
 * @markup: Markup of the item to insert.
 *
 * Inserts an action in @completion's action item list at position @index_
 * with markup @markup.
 *
 * Since: 2.4
 */
void
btk_entry_completion_insert_action_markup (BtkEntryCompletion *completion,
                                           gint                index_,
                                           const gchar        *markup)
{
  g_return_if_fail (BTK_IS_ENTRY_COMPLETION (completion));
  g_return_if_fail (markup != NULL);

  btk_entry_completion_insert_action (completion, index_, markup, TRUE);
}

/**
 * btk_entry_completion_delete_action:
 * @completion: A #BtkEntryCompletion.
 * @index_: The index of the item to Delete.
 *
 * Deletes the action at @index_ from @completion's action list.
 *
 * Since: 2.4
 */
void
btk_entry_completion_delete_action (BtkEntryCompletion *completion,
                                    gint                index_)
{
  BtkTreeIter iter;

  g_return_if_fail (BTK_IS_ENTRY_COMPLETION (completion));
  g_return_if_fail (index_ >= 0);

  btk_tree_model_iter_nth_child (BTK_TREE_MODEL (completion->priv->actions),
                                 &iter, NULL, index_);
  btk_list_store_remove (completion->priv->actions, &iter);
}

/**
 * btk_entry_completion_set_text_column:
 * @completion: A #BtkEntryCompletion.
 * @column: The column in the model of @completion to get strings from.
 *
 * Convenience function for setting up the most used case of this code: a
 * completion list with just strings. This function will set up @completion
 * to have a list displaying all (and just) strings in the completion list,
 * and to get those strings from @column in the model of @completion.
 *
 * This functions creates and adds a #BtkCellRendererText for the selected 
 * column. If you need to set the text column, but don't want the cell 
 * renderer, use g_object_set() to set the ::text_column property directly.
 * 
 * Since: 2.4
 */
void
btk_entry_completion_set_text_column (BtkEntryCompletion *completion,
                                      gint                column)
{
  BtkCellRenderer *cell;

  g_return_if_fail (BTK_IS_ENTRY_COMPLETION (completion));
  g_return_if_fail (column >= 0);

  completion->priv->text_column = column;

  cell = btk_cell_renderer_text_new ();
  btk_cell_layout_pack_start (BTK_CELL_LAYOUT (completion),
                              cell, TRUE);
  btk_cell_layout_add_attribute (BTK_CELL_LAYOUT (completion),
                                 cell,
                                 "text", column);

  g_object_notify (B_OBJECT (completion), "text-column");
}

/**
 * btk_entry_completion_get_text_column:
 * @completion: a #BtkEntryCompletion
 * 
 * Returns the column in the model of @completion to get strings from.
 * 
 * Return value: the column containing the strings
 *
 * Since: 2.6
 **/
gint
btk_entry_completion_get_text_column (BtkEntryCompletion *completion)
{
  g_return_val_if_fail (BTK_IS_ENTRY_COMPLETION (completion), -1);

  return completion->priv->text_column;  
}

/* private */

static gboolean
btk_entry_completion_list_enter_notify (BtkWidget        *widget,
					BdkEventCrossing *event,
					gpointer          data)
{
  BtkEntryCompletion *completion = BTK_ENTRY_COMPLETION (data);
  
  return completion->priv->ignore_enter;
}

static gboolean
btk_entry_completion_list_motion_notify (BtkWidget      *widget,
					 BdkEventMotion *event,
					 gpointer        data)
{
  BtkEntryCompletion *completion = BTK_ENTRY_COMPLETION (data);

  completion->priv->ignore_enter = FALSE;
  
  return FALSE;
}


/* some nasty size requisition */
gboolean
_btk_entry_completion_resize_popup (BtkEntryCompletion *completion)
{
  gint x, y;
  gint matches, actions, items, height, x_border, y_border;
  BdkScreen *screen;
  gint monitor_num;
  gint vertical_separator;
  BdkRectangle monitor;
  BtkRequisition popup_req;
  BtkRequisition entry_req;
  BtkTreePath *path;
  gboolean above;
  gint width;
  BtkTreeViewColumn *action_column;
  gint action_height;

  if (!completion->priv->entry->window)
    return FALSE;

  bdk_window_get_origin (completion->priv->entry->window, &x, &y);
  _btk_entry_get_borders (BTK_ENTRY (completion->priv->entry), &x_border, &y_border);

  matches = btk_tree_model_iter_n_children (BTK_TREE_MODEL (completion->priv->filter_model), NULL);
  actions = btk_tree_model_iter_n_children (BTK_TREE_MODEL (completion->priv->actions), NULL);
  action_column  = btk_tree_view_get_column (BTK_TREE_VIEW (completion->priv->action_view), 0);

  btk_tree_view_column_cell_get_size (completion->priv->column, NULL,
                                      NULL, NULL, NULL, &height);
  btk_tree_view_column_cell_get_size (action_column, NULL,
                                      NULL, NULL, NULL, &action_height);

  btk_widget_style_get (BTK_WIDGET (completion->priv->tree_view),
                        "vertical-separator", &vertical_separator,
                        NULL);

  height += vertical_separator;
  
  btk_widget_realize (completion->priv->tree_view);

  screen = btk_widget_get_screen (BTK_WIDGET (completion->priv->entry));
  monitor_num = bdk_screen_get_monitor_at_window (screen, 
						  BTK_WIDGET (completion->priv->entry)->window);
  bdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);

  

  if (y > monitor.height / 2)
    items = MIN (matches, (((monitor.y + y) - (actions * action_height)) / height) - 1);
  else
    items = MIN (matches, (((monitor.height - y) - (actions * action_height)) / height) - 1);

  if (items <= 0)
    btk_widget_hide (completion->priv->scrolled_window);
  else
    btk_widget_show (completion->priv->scrolled_window);

  if (completion->priv->popup_set_width)
    width = MIN (completion->priv->entry->allocation.width, monitor.width) - 2 * x_border;
  else
    width = -1;

  btk_tree_view_columns_autosize (BTK_TREE_VIEW (completion->priv->tree_view));
  btk_widget_set_size_request (completion->priv->tree_view, width, items * height);

  if (actions)
    {
      btk_widget_show (completion->priv->action_view);
      btk_widget_set_size_request (completion->priv->action_view, width, -1);
    }
  else
    btk_widget_hide (completion->priv->action_view);

  btk_widget_size_request (completion->priv->popup_window, &popup_req);
  btk_widget_size_request (completion->priv->entry, &entry_req);
  
  if (x < monitor.x)
    x = monitor.x;
  else if (x + popup_req.width > monitor.x + monitor.width)
    x = monitor.x + monitor.width - popup_req.width;
  
  if (y + entry_req.height + popup_req.height <= monitor.y + monitor.height ||
      y - monitor.y < (monitor.y + monitor.height) - (y + entry_req.height))
    {
      y += entry_req.height;
      above = FALSE;
    }
  else
    {
      y -= popup_req.height;
      above = TRUE;
    }
  
  if (matches > 0) 
    {
      path = btk_tree_path_new_from_indices (above ? matches - 1 : 0, -1);
      btk_tree_view_scroll_to_cell (BTK_TREE_VIEW (completion->priv->tree_view), path, 
				    NULL, FALSE, 0.0, 0.0);
      btk_tree_path_free (path);
    }

  btk_window_move (BTK_WINDOW (completion->priv->popup_window), x, y);

  return above;
}

void
_btk_entry_completion_popup (BtkEntryCompletion *completion)
{
  BtkTreeViewColumn *column;
  GList *renderers;
  BtkWidget *toplevel;

  if (btk_widget_get_mapped (completion->priv->popup_window))
    return;

  if (!btk_widget_get_mapped (completion->priv->entry))
    return;

  if (!btk_widget_has_focus (completion->priv->entry))
    return;

  completion->priv->ignore_enter = TRUE;
    
  column = btk_tree_view_get_column (BTK_TREE_VIEW (completion->priv->action_view), 0);
  renderers = btk_cell_layout_get_cells (BTK_CELL_LAYOUT (column));
  btk_widget_ensure_style (completion->priv->tree_view);
  g_object_set (BTK_CELL_RENDERER (renderers->data), "cell-background-bdk",
                &completion->priv->tree_view->style->bg[BTK_STATE_NORMAL],
                NULL);
  g_list_free (renderers);

  btk_widget_show_all (completion->priv->vbox);

  /* default on no match */
  completion->priv->current_selected = -1;

  _btk_entry_completion_resize_popup (completion);

  toplevel = btk_widget_get_toplevel (completion->priv->entry);
  if (BTK_IS_WINDOW (toplevel))
    btk_window_group_add_window (btk_window_get_group (BTK_WINDOW (toplevel)), 
				 BTK_WINDOW (completion->priv->popup_window));

  /* prevent the first row being focused */
  btk_widget_grab_focus (completion->priv->tree_view);

  btk_tree_selection_unselect_all (btk_tree_view_get_selection (BTK_TREE_VIEW (completion->priv->tree_view)));
  btk_tree_selection_unselect_all (btk_tree_view_get_selection (BTK_TREE_VIEW (completion->priv->action_view)));

  btk_window_set_screen (BTK_WINDOW (completion->priv->popup_window),
                         btk_widget_get_screen (completion->priv->entry));

  btk_widget_show (completion->priv->popup_window);
    
  btk_grab_add (completion->priv->popup_window);
  bdk_pointer_grab (completion->priv->popup_window->window, TRUE,
                    BDK_BUTTON_PRESS_MASK |
                    BDK_BUTTON_RELEASE_MASK |
                    BDK_POINTER_MOTION_MASK,
                    NULL, NULL, BDK_CURRENT_TIME);
}

void
_btk_entry_completion_popdown (BtkEntryCompletion *completion)
{
  if (!btk_widget_get_mapped (completion->priv->popup_window))
    return;

  completion->priv->ignore_enter = FALSE;
  
  bdk_pointer_ungrab (BDK_CURRENT_TIME);
  btk_grab_remove (completion->priv->popup_window);

  btk_widget_hide (completion->priv->popup_window);
}

static gboolean 
btk_entry_completion_match_selected (BtkEntryCompletion *completion,
				     BtkTreeModel       *model,
				     BtkTreeIter        *iter)
{
  gchar *str = NULL;

  btk_tree_model_get (model, iter, completion->priv->text_column, &str, -1);
  btk_entry_set_text (BTK_ENTRY (completion->priv->entry), str ? str : "");
  
  /* move cursor to the end */
  btk_editable_set_position (BTK_EDITABLE (completion->priv->entry), -1);
  
  g_free (str);

  return TRUE;
}

static gboolean
btk_entry_completion_cursor_on_match (BtkEntryCompletion *completion,
				      BtkTreeModel       *model,
				      BtkTreeIter        *iter)
{
  btk_entry_completion_insert_completion (completion, model, iter);

  return TRUE;
}

gchar *
_btk_entry_completion_compute_prefix (BtkEntryCompletion *completion,
				      const char         *key)
{
  BtkTreeIter iter;
  gchar *prefix = NULL;
  gboolean valid;

  if (completion->priv->text_column < 0)
    return NULL;

  valid = btk_tree_model_get_iter_first (BTK_TREE_MODEL (completion->priv->filter_model),
					 &iter);
  
  while (valid)
    {
      gchar *text;
      
      btk_tree_model_get (BTK_TREE_MODEL (completion->priv->filter_model),
			  &iter, completion->priv->text_column, &text,
			  -1);

      if (text && g_str_has_prefix (text, key))
	{
	  if (!prefix)
	    prefix = g_strdup (text);
	  else
	    {
	      gchar *p = prefix;
	      gchar *q = text;
	      
	      while (*p && *p == *q)
		{
		  p++;
		  q++;
		}
	      
	      *p = '\0';
              
              if (p > prefix)
                {
                  /* strip a partial multibyte character */
                  q = g_utf8_find_prev_char (prefix, p);
                  switch (g_utf8_get_char_validated (q, p - q))
                    {
                    case (gunichar)-2:
                    case (gunichar)-1:
                      *q = 0;
                    default: ;
                    }
                }
	    }
	}
      
      g_free (text);
      valid = btk_tree_model_iter_next (BTK_TREE_MODEL (completion->priv->filter_model),
					&iter);
    }

  return prefix;
}


static gboolean
btk_entry_completion_real_insert_prefix (BtkEntryCompletion *completion,
					 const gchar        *prefix)
{
  if (prefix)
    {
      gint key_len;
      gint prefix_len;
      const gchar *key;

      prefix_len = g_utf8_strlen (prefix, -1);

      key = btk_entry_get_text (BTK_ENTRY (completion->priv->entry));
      key_len = g_utf8_strlen (key, -1);

      if (prefix_len > key_len)
	{
	  gint pos = prefix_len;

	  btk_editable_insert_text (BTK_EDITABLE (completion->priv->entry),
				    prefix + strlen (key), -1, &pos);
	  btk_editable_select_rebunnyion (BTK_EDITABLE (completion->priv->entry),
				      key_len, prefix_len);

	  completion->priv->has_completion = TRUE;
	}
    }

  return TRUE;
}

/**
 * btk_entry_completion_get_completion_prefix:
 * @completion: a #BtkEntryCompletion
 * 
 * Get the original text entered by the user that triggered
 * the completion or %NULL if there's no completion ongoing.
 * 
 * Returns: the prefix for the current completion
 * 
 * Since: 2.12
 **/
const gchar*
btk_entry_completion_get_completion_prefix (BtkEntryCompletion *completion)
{
  g_return_val_if_fail (BTK_IS_ENTRY_COMPLETION (completion), NULL);

  return completion->priv->completion_prefix;
}

static void
btk_entry_completion_insert_completion_text (BtkEntryCompletion *completion,
					     const gchar *text)
{
  BtkEntryCompletionPrivate *priv = completion->priv;
  gint len;

  priv = completion->priv;

  if (priv->changed_id > 0)
    g_signal_handler_block (priv->entry, priv->changed_id);

  if (priv->insert_text_id > 0)
    g_signal_handler_block (priv->entry, priv->insert_text_id);

  btk_entry_set_text (BTK_ENTRY (priv->entry), text);

  len = strlen (priv->completion_prefix);
  btk_editable_select_rebunnyion (BTK_EDITABLE (priv->entry), len, -1);

  if (priv->changed_id > 0)
    g_signal_handler_unblock (priv->entry, priv->changed_id);

  if (priv->insert_text_id > 0)
    g_signal_handler_unblock (priv->entry, priv->insert_text_id);
}

static gboolean
btk_entry_completion_insert_completion (BtkEntryCompletion *completion,
					BtkTreeModel       *model,
					BtkTreeIter        *iter)
{
  gchar *str = NULL;

  if (completion->priv->text_column < 0)
    return FALSE;

  btk_tree_model_get (model, iter,
		      completion->priv->text_column, &str,
		      -1);

  btk_entry_completion_insert_completion_text (completion, str);

  g_free (str);

  return TRUE;
}

/**
 * btk_entry_completion_insert_prefix:
 * @completion: a #BtkEntryCompletion
 * 
 * Requests a prefix insertion. 
 * 
 * Since: 2.6
 **/

void
btk_entry_completion_insert_prefix (BtkEntryCompletion *completion)
{
  gboolean done;
  gchar *prefix;

  if (completion->priv->insert_text_id > 0)
    g_signal_handler_block (completion->priv->entry,
                            completion->priv->insert_text_id);

  prefix = _btk_entry_completion_compute_prefix (completion,
						 btk_entry_get_text (BTK_ENTRY (completion->priv->entry)));
  if (prefix)
    {
      g_signal_emit (completion, entry_completion_signals[INSERT_PREFIX],
		     0, prefix, &done);
      g_free (prefix);
    }

  if (completion->priv->insert_text_id > 0)
    g_signal_handler_unblock (completion->priv->entry,
                              completion->priv->insert_text_id);
}

/**
 * btk_entry_completion_set_inline_completion:
 * @completion: a #BtkEntryCompletion
 * @inline_completion: %TRUE to do inline completion
 * 
 * Sets whether the common prefix of the possible completions should
 * be automatically inserted in the entry.
 * 
 * Since: 2.6
 **/
void 
btk_entry_completion_set_inline_completion (BtkEntryCompletion *completion,
					    gboolean            inline_completion)
{
  g_return_if_fail (BTK_IS_ENTRY_COMPLETION (completion));
  
  inline_completion = inline_completion != FALSE;

  if (completion->priv->inline_completion != inline_completion)
    {
      completion->priv->inline_completion = inline_completion;

      g_object_notify (B_OBJECT (completion), "inline-completion");
    }
}

/**
 * btk_entry_completion_get_inline_completion:
 * @completion: a #BtkEntryCompletion
 * 
 * Returns whether the common prefix of the possible completions should
 * be automatically inserted in the entry.
 * 
 * Return value: %TRUE if inline completion is turned on
 * 
 * Since: 2.6
 **/
gboolean
btk_entry_completion_get_inline_completion (BtkEntryCompletion *completion)
{
  g_return_val_if_fail (BTK_IS_ENTRY_COMPLETION (completion), FALSE);
  
  return completion->priv->inline_completion;
}

/**
 * btk_entry_completion_set_popup_completion:
 * @completion: a #BtkEntryCompletion
 * @popup_completion: %TRUE to do popup completion
 * 
 * Sets whether the completions should be presented in a popup window.
 * 
 * Since: 2.6
 **/
void 
btk_entry_completion_set_popup_completion (BtkEntryCompletion *completion,
					   gboolean            popup_completion)
{
  g_return_if_fail (BTK_IS_ENTRY_COMPLETION (completion));
  
  popup_completion = popup_completion != FALSE;

  if (completion->priv->popup_completion != popup_completion)
    {
      completion->priv->popup_completion = popup_completion;

      g_object_notify (B_OBJECT (completion), "popup-completion");
    }
}


/**
 * btk_entry_completion_get_popup_completion:
 * @completion: a #BtkEntryCompletion
 * 
 * Returns whether the completions should be presented in a popup window.
 * 
 * Return value: %TRUE if popup completion is turned on
 * 
 * Since: 2.6
 **/
gboolean
btk_entry_completion_get_popup_completion (BtkEntryCompletion *completion)
{
  g_return_val_if_fail (BTK_IS_ENTRY_COMPLETION (completion), TRUE);
  
  return completion->priv->popup_completion;
}

/**
 * btk_entry_completion_set_popup_set_width:
 * @completion: a #BtkEntryCompletion
 * @popup_set_width: %TRUE to make the width of the popup the same as the entry
 *
 * Sets whether the completion popup window will be resized to be the same
 * width as the entry.
 *
 * Since: 2.8
 */
void 
btk_entry_completion_set_popup_set_width (BtkEntryCompletion *completion,
					  gboolean            popup_set_width)
{
  g_return_if_fail (BTK_IS_ENTRY_COMPLETION (completion));
  
  popup_set_width = popup_set_width != FALSE;

  if (completion->priv->popup_set_width != popup_set_width)
    {
      completion->priv->popup_set_width = popup_set_width;

      g_object_notify (B_OBJECT (completion), "popup-set-width");
    }
}

/**
 * btk_entry_completion_get_popup_set_width:
 * @completion: a #BtkEntryCompletion
 * 
 * Returns whether the  completion popup window will be resized to the 
 * width of the entry.
 * 
 * Return value: %TRUE if the popup window will be resized to the width of 
 *   the entry
 * 
 * Since: 2.8
 **/
gboolean
btk_entry_completion_get_popup_set_width (BtkEntryCompletion *completion)
{
  g_return_val_if_fail (BTK_IS_ENTRY_COMPLETION (completion), TRUE);
  
  return completion->priv->popup_set_width;
}


/**
 * btk_entry_completion_set_popup_single_match:
 * @completion: a #BtkEntryCompletion
 * @popup_single_match: %TRUE if the popup should appear even for a single
 *   match
 *
 * Sets whether the completion popup window will appear even if there is
 * only a single match. You may want to set this to %FALSE if you
 * are using <link linkend="BtkEntryCompletion--inline-completion">inline
 * completion</link>.
 *
 * Since: 2.8
 */
void 
btk_entry_completion_set_popup_single_match (BtkEntryCompletion *completion,
					     gboolean            popup_single_match)
{
  g_return_if_fail (BTK_IS_ENTRY_COMPLETION (completion));
  
  popup_single_match = popup_single_match != FALSE;

  if (completion->priv->popup_single_match != popup_single_match)
    {
      completion->priv->popup_single_match = popup_single_match;

      g_object_notify (B_OBJECT (completion), "popup-single-match");
    }
}

/**
 * btk_entry_completion_get_popup_single_match:
 * @completion: a #BtkEntryCompletion
 * 
 * Returns whether the completion popup window will appear even if there is
 * only a single match. 
 * 
 * Return value: %TRUE if the popup window will appear regardless of the
 *    number of matches.
 * 
 * Since: 2.8
 **/
gboolean
btk_entry_completion_get_popup_single_match (BtkEntryCompletion *completion)
{
  g_return_val_if_fail (BTK_IS_ENTRY_COMPLETION (completion), TRUE);
  
  return completion->priv->popup_single_match;
}

/**
 * btk_entry_completion_set_inline_selection:
 * @completion: a #BtkEntryCompletion
 * @inline_selection: %TRUE to do inline selection
 * 
 * Sets whether it is possible to cycle through the possible completions
 * inside the entry.
 * 
 * Since: 2.12
 **/
void
btk_entry_completion_set_inline_selection (BtkEntryCompletion *completion,
					   gboolean inline_selection)
{
  g_return_if_fail (BTK_IS_ENTRY_COMPLETION (completion));

  inline_selection = inline_selection != FALSE;

  if (completion->priv->inline_selection != inline_selection)
    {
      completion->priv->inline_selection = inline_selection;

      g_object_notify (B_OBJECT (completion), "inline-selection");
    }
}

/**
 * btk_entry_completion_get_inline_selection:
 * @completion: a #BtkEntryCompletion
 *
 * Returns %TRUE if inline-selection mode is turned on.
 *
 * Returns: %TRUE if inline-selection mode is on
 *
 * Since: 2.12
 **/
gboolean
btk_entry_completion_get_inline_selection (BtkEntryCompletion *completion)
{
  g_return_val_if_fail (BTK_IS_ENTRY_COMPLETION (completion), FALSE);

  return completion->priv->inline_selection;
}

#define __BTK_ENTRY_COMPLETION_C__
#include "btkaliasdef.c"
