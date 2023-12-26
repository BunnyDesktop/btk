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

#undef BTK_DISABLE_DEPRECATED

#include "config.h"
#include <string.h>
#include "bdk/bdkkeysyms.h"
#include "bdk/bdki18n.h"
#include "btkclipboard.h"
#include "btkoldeditable.h"
#include "btkmain.h"
#include "btkmarshalers.h"
#include "btkselection.h"
#include "btksignal.h"
#include "btkintl.h"

#include "btkalias.h"

#define MIN_EDITABLE_WIDTH  150
#define DRAW_TIMEOUT     20
#define INNER_BORDER     2

enum {
  /* Binding actions */
  ACTIVATE,
  SET_EDITABLE,
  MOVE_CURSOR,
  MOVE_WORD,
  MOVE_PAGE,
  MOVE_TO_ROW,
  MOVE_TO_COLUMN,
  KILL_CHAR,
  KILL_WORD,
  KILL_LINE,
  CUT_CLIPBOARD,
  COPY_CLIPBOARD,
  PASTE_CLIPBOARD,
  LAST_SIGNAL
};

enum {
  ARG_0,
  ARG_TEXT_POSITION,
  ARG_EDITABLE
};

/* values for selection info */

enum {
  TARGET_STRING,
  TARGET_TEXT,
  TARGET_COMPOUND_TEXT
};

static void  btk_old_editable_editable_init        (BtkEditableClass    *iface);
static void  btk_old_editable_set_arg              (BtkObject           *object,
						    BtkArg              *arg,
						    guint                arg_id);
static void  btk_old_editable_get_arg              (BtkObject           *object,
						    BtkArg              *arg,
						    guint                arg_id);
static void *btk_old_editable_get_public_chars     (BtkOldEditable      *old_editable,
						    gint                 start,
						    gint                 end);

static gint btk_old_editable_selection_clear    (BtkWidget         *widget,
						 BdkEventSelection *event);
static void btk_old_editable_selection_get      (BtkWidget         *widget,
						 BtkSelectionData  *selection_data,
						 guint              info,
						 guint              time);
static void btk_old_editable_selection_received (BtkWidget         *widget,
						 BtkSelectionData  *selection_data,
						 guint              time);

static void  btk_old_editable_set_selection        (BtkOldEditable      *old_editable,
						    gint                 start,
						    gint                 end);

static void btk_old_editable_real_set_editable    (BtkOldEditable *old_editable,
						   gboolean        is_editable);
static void btk_old_editable_real_cut_clipboard   (BtkOldEditable *old_editable);
static void btk_old_editable_real_copy_clipboard  (BtkOldEditable *old_editable);
static void btk_old_editable_real_paste_clipboard (BtkOldEditable *old_editable);

static void     btk_old_editable_insert_text         (BtkEditable *editable,
						      const gchar *new_text,
						      gint         new_text_length,
						      gint        *position);
static void     btk_old_editable_delete_text         (BtkEditable *editable,
						      gint         start_pos,
						      gint         end_pos);
static gchar *  btk_old_editable_get_chars           (BtkEditable *editable,
						      gint         start,
						      gint         end);
static void     btk_old_editable_set_selection_bounds (BtkEditable *editable,
						       gint         start,
						       gint         end);
static gboolean btk_old_editable_get_selection_bounds (BtkEditable *editable,
						       gint        *start,
						       gint        *end);
static void     btk_old_editable_set_position        (BtkEditable *editable,
						      gint         position);
static gint     btk_old_editable_get_position        (BtkEditable *editable);
static void     btk_old_editable_finalize            (GObject     *object);

static guint editable_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (BtkOldEditable, btk_old_editable, BTK_TYPE_WIDGET,
				  G_IMPLEMENT_INTERFACE (BTK_TYPE_EDITABLE,
							 btk_old_editable_editable_init))

static void
btk_old_editable_class_init (BtkOldEditableClass *class)
{
  GObjectClass *bobject_class;
  BtkObjectClass *object_class;
  BtkWidgetClass *widget_class;

  bobject_class = (GObjectClass*) class;
  object_class = (BtkObjectClass*) class;
  widget_class = (BtkWidgetClass*) class;

  bobject_class->finalize = btk_old_editable_finalize;

  object_class->set_arg = btk_old_editable_set_arg;
  object_class->get_arg = btk_old_editable_get_arg;

  widget_class->selection_clear_event = btk_old_editable_selection_clear;
  widget_class->selection_received = btk_old_editable_selection_received;
  widget_class->selection_get = btk_old_editable_selection_get;

  class->activate = NULL;
  class->set_editable = btk_old_editable_real_set_editable;

  class->move_cursor = NULL;
  class->move_word = NULL;
  class->move_page = NULL;
  class->move_to_row = NULL;
  class->move_to_column = NULL;

  class->kill_char = NULL;
  class->kill_word = NULL;
  class->kill_line = NULL;

  class->cut_clipboard = btk_old_editable_real_cut_clipboard;
  class->copy_clipboard = btk_old_editable_real_copy_clipboard;
  class->paste_clipboard = btk_old_editable_real_paste_clipboard;

  class->update_text = NULL;
  class->get_chars = NULL;
  class->set_selection = NULL;
  class->set_position = NULL;

  editable_signals[ACTIVATE] =
    btk_signal_new (I_("activate"),
		    BTK_RUN_LAST | BTK_RUN_ACTION,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkOldEditableClass, activate),
		    _btk_marshal_VOID__VOID,
		    BTK_TYPE_NONE, 0);
  widget_class->activate_signal = editable_signals[ACTIVATE];

  editable_signals[SET_EDITABLE] =
    btk_signal_new (I_("set-editable"),
		    BTK_RUN_LAST | BTK_RUN_ACTION,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkOldEditableClass, set_editable),
		    _btk_marshal_VOID__BOOLEAN,
		    BTK_TYPE_NONE, 1,
		    BTK_TYPE_BOOL);

  editable_signals[MOVE_CURSOR] =
    btk_signal_new (I_("move-cursor"),
		    BTK_RUN_LAST | BTK_RUN_ACTION,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkOldEditableClass, move_cursor),
		    _btk_marshal_VOID__INT_INT,
		    BTK_TYPE_NONE, 2, 
		    BTK_TYPE_INT, 
		    BTK_TYPE_INT);

  editable_signals[MOVE_WORD] =
    btk_signal_new (I_("move-word"),
		    BTK_RUN_LAST | BTK_RUN_ACTION,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkOldEditableClass, move_word),
		    _btk_marshal_VOID__INT,
		    BTK_TYPE_NONE, 1, 
		    BTK_TYPE_INT);

  editable_signals[MOVE_PAGE] =
    btk_signal_new (I_("move-page"),
		    BTK_RUN_LAST | BTK_RUN_ACTION,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkOldEditableClass, move_page),
		    _btk_marshal_VOID__INT_INT,
		    BTK_TYPE_NONE, 2, 
		    BTK_TYPE_INT, 
		    BTK_TYPE_INT);

  editable_signals[MOVE_TO_ROW] =
    btk_signal_new (I_("move-to-row"),
		    BTK_RUN_LAST | BTK_RUN_ACTION,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkOldEditableClass, move_to_row),
		    _btk_marshal_VOID__INT,
		    BTK_TYPE_NONE, 1, 
		    BTK_TYPE_INT);

  editable_signals[MOVE_TO_COLUMN] =
    btk_signal_new (I_("move-to-column"),
		    BTK_RUN_LAST | BTK_RUN_ACTION,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkOldEditableClass, move_to_column),
		    _btk_marshal_VOID__INT,
		    BTK_TYPE_NONE, 1, 
		    BTK_TYPE_INT);

  editable_signals[KILL_CHAR] =
    btk_signal_new (I_("kill-char"),
		    BTK_RUN_LAST | BTK_RUN_ACTION,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkOldEditableClass, kill_char),
		    _btk_marshal_VOID__INT,
		    BTK_TYPE_NONE, 1, 
		    BTK_TYPE_INT);

  editable_signals[KILL_WORD] =
    btk_signal_new (I_("kill-word"),
		    BTK_RUN_LAST | BTK_RUN_ACTION,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkOldEditableClass, kill_word),
		    _btk_marshal_VOID__INT,
		    BTK_TYPE_NONE, 1, 
		    BTK_TYPE_INT);

  editable_signals[KILL_LINE] =
    btk_signal_new (I_("kill-line"),
		    BTK_RUN_LAST | BTK_RUN_ACTION,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkOldEditableClass, kill_line),
		    _btk_marshal_VOID__INT,
		    BTK_TYPE_NONE, 1, 
		    BTK_TYPE_INT);

  editable_signals[CUT_CLIPBOARD] =
    btk_signal_new (I_("cut-clipboard"),
		    BTK_RUN_LAST | BTK_RUN_ACTION,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkOldEditableClass, cut_clipboard),
		    _btk_marshal_VOID__VOID,
		    BTK_TYPE_NONE, 0);

  editable_signals[COPY_CLIPBOARD] =
    btk_signal_new (I_("copy-clipboard"),
		    BTK_RUN_LAST | BTK_RUN_ACTION,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkOldEditableClass, copy_clipboard),
		    _btk_marshal_VOID__VOID,
		    BTK_TYPE_NONE, 0);

  editable_signals[PASTE_CLIPBOARD] =
    btk_signal_new (I_("paste-clipboard"),
		    BTK_RUN_LAST | BTK_RUN_ACTION,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkOldEditableClass, paste_clipboard),
		    _btk_marshal_VOID__VOID,
		    BTK_TYPE_NONE, 0);

  btk_object_add_arg_type ("BtkOldEditable::text-position", BTK_TYPE_INT, BTK_ARG_READWRITE | G_PARAM_STATIC_NAME, ARG_TEXT_POSITION);
  btk_object_add_arg_type ("BtkOldEditable::editable", BTK_TYPE_BOOL, BTK_ARG_READWRITE | G_PARAM_STATIC_NAME, ARG_EDITABLE);
}

static void
btk_old_editable_editable_init (BtkEditableClass *iface)
{
  iface->do_insert_text = btk_old_editable_insert_text;
  iface->do_delete_text = btk_old_editable_delete_text;
  iface->get_chars = btk_old_editable_get_chars;
  iface->set_selection_bounds = btk_old_editable_set_selection_bounds;
  iface->get_selection_bounds = btk_old_editable_get_selection_bounds;
  iface->set_position = btk_old_editable_set_position;
  iface->get_position = btk_old_editable_get_position;
}

static void
btk_old_editable_set_arg (BtkObject *object,
			  BtkArg    *arg,
			  guint      arg_id)
{
  BtkEditable *editable = BTK_EDITABLE (object);

  switch (arg_id)
    {
    case ARG_TEXT_POSITION:
      btk_editable_set_position (editable, BTK_VALUE_INT (*arg));
      break;
    case ARG_EDITABLE:
      btk_signal_emit (object, editable_signals[SET_EDITABLE],
		       BTK_VALUE_BOOL (*arg) != FALSE);
      break;
    default:
      break;
    }
}

static void
btk_old_editable_get_arg (BtkObject *object,
			  BtkArg    *arg,
			  guint      arg_id)
{
  BtkOldEditable *old_editable;

  old_editable = BTK_OLD_EDITABLE (object);

  switch (arg_id)
    {
    case ARG_TEXT_POSITION:
      BTK_VALUE_INT (*arg) = old_editable->current_pos;
      break;
    case ARG_EDITABLE:
      BTK_VALUE_BOOL (*arg) = old_editable->editable;
      break;
    default:
      arg->type = BTK_TYPE_INVALID;
      break;
    }
}

static void
btk_old_editable_init (BtkOldEditable *old_editable)
{
  static const BtkTargetEntry targets[] = {
    { "UTF8_STRING", 0, 0 },
    { "STRING", 0, 0 },
    { "TEXT",   0, 0 }, 
    { "COMPOUND_TEXT", 0, 0 }
  };

  btk_widget_set_can_focus (BTK_WIDGET (old_editable), TRUE);

  old_editable->selection_start_pos = 0;
  old_editable->selection_end_pos = 0;
  old_editable->has_selection = FALSE;
  old_editable->editable = 1;
  old_editable->visible = 1;
  old_editable->clipboard_text = NULL;

  btk_selection_add_targets (BTK_WIDGET (old_editable), BDK_SELECTION_PRIMARY,
			     targets, G_N_ELEMENTS (targets));
}

static void
btk_old_editable_finalize (GObject *object)
{
  btk_selection_clear_targets (BTK_WIDGET (object), BDK_SELECTION_PRIMARY);

  G_OBJECT_CLASS (btk_old_editable_parent_class)->finalize (object);
}

static void
btk_old_editable_insert_text (BtkEditable *editable,
			      const gchar *new_text,
			      gint         new_text_length,
			      gint        *position)
{
  gchar buf[64];
  gchar *text;

  g_object_ref (editable);

  if (new_text_length <= 63)
    text = buf;
  else
    text = g_new (gchar, new_text_length + 1);

  text[new_text_length] = '\0';
  strncpy (text, new_text, new_text_length);
  
  g_signal_emit_by_name (editable, "insert-text", text, new_text_length,
			 position);
  g_signal_emit_by_name (editable, "changed");
  
  if (new_text_length > 63)
    g_free (text);

  g_object_unref (editable);
}

static void
btk_old_editable_delete_text (BtkEditable *editable,
			      gint         start_pos,
			      gint         end_pos)
{
  BtkOldEditable *old_editable = BTK_OLD_EDITABLE (editable);

  g_object_ref (old_editable);

  g_signal_emit_by_name (editable, "delete-text", start_pos, end_pos);
  g_signal_emit_by_name (editable, "changed");

  if (old_editable->selection_start_pos == old_editable->selection_end_pos &&
      old_editable->has_selection)
    btk_old_editable_claim_selection (old_editable, FALSE, BDK_CURRENT_TIME);
  
  g_object_unref (old_editable);
}

static void
btk_old_editable_update_text (BtkOldEditable *old_editable,
			      gint            start_pos,
			      gint            end_pos)
{
  BtkOldEditableClass *klass = BTK_OLD_EDITABLE_GET_CLASS (old_editable);
  klass->update_text (BTK_OLD_EDITABLE (old_editable), start_pos, end_pos);
}

static gchar *    
btk_old_editable_get_chars  (BtkEditable      *editable,
			     gint              start,
			     gint              end)
{
  BtkOldEditableClass *klass = BTK_OLD_EDITABLE_GET_CLASS (editable);
  return klass->get_chars (BTK_OLD_EDITABLE (editable), start, end);
}

/*
 * Like btk_editable_get_chars, but if the editable is not
 * visible, return asterisks; also convert result to UTF-8.
 */
static void *    
btk_old_editable_get_public_chars (BtkOldEditable   *old_editable,
				   gint              start,
				   gint              end)
{
  gchar *str = NULL;
  const gchar *charset;
  gboolean need_conversion = !g_get_charset (&charset);

  if (old_editable->visible)
    {
      GError *error = NULL;
      gchar *tmp = btk_editable_get_chars (BTK_EDITABLE (old_editable), start, end);

      if (need_conversion)
	{
	  str = g_convert (tmp, -1,
			   "UTF-8", charset,
			   NULL, NULL, &error);
	  
	  if (!str)
	    {
	      g_warning ("Cannot convert text from charset to UTF-8 %s: %s", charset, error->message);
	      g_error_free (error);
	    }

	  g_free (tmp);
	}
      else
	str = tmp;
    }
  else
    {
      gint i;
      gint nchars = end - start;
       
      if (nchars < 0)
	nchars = -nchars;

      str = g_new (gchar, nchars + 1);
      for (i = 0; i<nchars; i++)
	str[i] = '*';
      str[i] = '\0';
    }

  return str;
}

static void
btk_old_editable_set_selection (BtkOldEditable *old_editable,
				gint            start_pos,
				gint            end_pos)
{
  BtkOldEditableClass *klass = BTK_OLD_EDITABLE_GET_CLASS (old_editable);
  klass->set_selection (old_editable, start_pos, end_pos);
}

static void
btk_old_editable_set_position (BtkEditable *editable,
			       gint            position)
{
  BtkOldEditableClass *klass = BTK_OLD_EDITABLE_GET_CLASS (editable);

  klass->set_position (BTK_OLD_EDITABLE (editable), position);
}

static gint
btk_old_editable_get_position (BtkEditable *editable)
{
  return BTK_OLD_EDITABLE (editable)->current_pos;
}

static gint
btk_old_editable_selection_clear (BtkWidget         *widget,
				  BdkEventSelection *event)
{
  BtkOldEditable *old_editable = BTK_OLD_EDITABLE (widget);
  
  /* Let the selection handling code know that the selection
   * has been changed, since we've overriden the default handler */
  if (!BTK_WIDGET_CLASS (btk_old_editable_parent_class)->selection_clear_event (widget, event))
    return FALSE;
  
  if (old_editable->has_selection)
    {
      old_editable->has_selection = FALSE;
      btk_old_editable_update_text (old_editable, old_editable->selection_start_pos,
				    old_editable->selection_end_pos);
    }
  
  return TRUE;
}

static void
btk_old_editable_selection_get (BtkWidget        *widget,
				BtkSelectionData *selection_data,
				guint             info,
				guint             time)
{
  BtkOldEditable *old_editable = BTK_OLD_EDITABLE (widget);
  gint selection_start_pos;
  gint selection_end_pos;

  gchar *str;

  selection_start_pos = MIN (old_editable->selection_start_pos, old_editable->selection_end_pos);
  selection_end_pos = MAX (old_editable->selection_start_pos, old_editable->selection_end_pos);

  str = btk_old_editable_get_public_chars (old_editable, 
					   selection_start_pos, 
					   selection_end_pos);

  if (str)
    {
      btk_selection_data_set_text (selection_data, str, -1);
      g_free (str);
    }
}

static void
btk_old_editable_paste_received (BtkOldEditable *old_editable,
				 const gchar    *text,
				 gboolean        is_clipboard)
{
  const gchar *str = NULL;
  const gchar *charset;
  gboolean need_conversion = FALSE;

  if (text)
    {
      GError *error = NULL;
      
      need_conversion = !g_get_charset (&charset);

      if (need_conversion)
	{
	  str = g_convert_with_fallback (text, -1,
					 charset, "UTF-8", NULL,
					 NULL, NULL, &error);
	  if (!str)
	    {
	      g_warning ("Cannot convert text from UTF-8 to %s: %s",
			 charset, error->message);
	      g_error_free (error);
	      return;
	    }
	}
      else
	str = text;
    }

  if (str)
    {
      gboolean reselect;
      gint old_pos;
      gint tmp_pos;
  
      reselect = FALSE;

      if ((old_editable->selection_start_pos != old_editable->selection_end_pos) && 
	  (!old_editable->has_selection || is_clipboard))
	{
	  reselect = TRUE;
	  
	  /* Don't want to call btk_editable_delete_selection here if we are going
	   * to reclaim the selection to avoid extra server traffic */
	  if (old_editable->has_selection)
	    {
	      btk_editable_delete_text (BTK_EDITABLE (old_editable),
					MIN (old_editable->selection_start_pos, old_editable->selection_end_pos),
					MAX (old_editable->selection_start_pos, old_editable->selection_end_pos));
	    }
	  else
	    btk_editable_delete_selection (BTK_EDITABLE (old_editable));
	}
      
      tmp_pos = old_pos = old_editable->current_pos;
      
      btk_editable_insert_text (BTK_EDITABLE (old_editable), str, -1, &tmp_pos);

      if (reselect)
	btk_old_editable_set_selection (old_editable, old_pos, old_editable->current_pos);

      if (str && str != text)
	g_free ((gchar *) str);
    }
}

static void
btk_old_editable_selection_received  (BtkWidget         *widget,
				      BtkSelectionData  *selection_data,
				      guint              time)
{
  BtkOldEditable *old_editable = BTK_OLD_EDITABLE (widget);

  guchar *text = btk_selection_data_get_text (selection_data);

  if (!text)
    {
      /* If we asked for UTF8 and didn't get it, try text; if we asked
       * for text and didn't get it, try string.  If we asked for
       * anything else and didn't get it, give up.
       */
      if (selection_data->target == bdk_atom_intern_static_string ("UTF8_STRING"))
	{
	  btk_selection_convert (widget, BDK_SELECTION_PRIMARY,
				 bdk_atom_intern_static_string ("TEXT"),
				 time);
	  return;
	}
      else if (selection_data->target == bdk_atom_intern_static_string ("TEXT"))
	{
	  btk_selection_convert (widget, BDK_SELECTION_PRIMARY,
				 BDK_TARGET_STRING,
				 time);
	  return;
	}
    }

  if (text)
    {
      btk_old_editable_paste_received (old_editable, (gchar *) text, FALSE);
      g_free (text);
    }
}

static void
old_editable_text_received_cb (BtkClipboard *clipboard,
			       const gchar  *text,
			       gpointer      data)
{
  BtkOldEditable *old_editable = BTK_OLD_EDITABLE (data);

  btk_old_editable_paste_received (old_editable, text, TRUE);
  g_object_unref (G_OBJECT (old_editable));
}

/**
 * btk_old_editable_claim_selection:
 * @old_editable: a #BtkOldEditable
 * @claim: if %TRUE, claim ownership of the selection, if %FALSE, give
 *   up ownership
 * @time_: timestamp for this operation
 * 
 * Claims or gives up ownership of the selection.
 */
void
btk_old_editable_claim_selection (BtkOldEditable *old_editable, 
				  gboolean        claim, 
				  guint32         time)
{
  BtkWidget  *widget;
  BdkDisplay *display;
  
  g_return_if_fail (BTK_IS_OLD_EDITABLE (old_editable));
  widget = BTK_WIDGET (old_editable);
  g_return_if_fail (btk_widget_get_realized (widget));

  display = btk_widget_get_display (widget);
  old_editable->has_selection = FALSE;
  
  if (claim)
    {
      if (btk_selection_owner_set_for_display (display, widget,
					       BDK_SELECTION_PRIMARY, time))
	old_editable->has_selection = TRUE;
    }
  else
    {
      if (bdk_selection_owner_get_for_display (display, BDK_SELECTION_PRIMARY) == widget->window)
	btk_selection_owner_set_for_display (display,
					     NULL,
					     BDK_SELECTION_PRIMARY, time);
    }
}

static void
btk_old_editable_set_selection_bounds (BtkEditable *editable,
				       gint         start,
				       gint         end)
{
  BtkOldEditable *old_editable = BTK_OLD_EDITABLE (editable);
  
  if (btk_widget_get_realized (BTK_WIDGET (editable)))
    btk_old_editable_claim_selection (old_editable, start != end, BDK_CURRENT_TIME);
  
  btk_old_editable_set_selection (old_editable, start, end);
}

static gboolean
btk_old_editable_get_selection_bounds (BtkEditable *editable,
				       gint        *start,
				       gint        *end)
{
  BtkOldEditable *old_editable = BTK_OLD_EDITABLE (editable);

  *start = old_editable->selection_start_pos;
  *end = old_editable->selection_end_pos;

  return (old_editable->selection_start_pos != old_editable->selection_end_pos);
}

static void
btk_old_editable_real_set_editable (BtkOldEditable *old_editable,
				    gboolean        is_editable)
{
  is_editable = is_editable != FALSE;

  if (old_editable->editable != is_editable)
    {
      old_editable->editable = is_editable;
      btk_widget_queue_draw (BTK_WIDGET (old_editable));
    }
}

static void
btk_old_editable_real_cut_clipboard (BtkOldEditable *old_editable)
{
  btk_old_editable_real_copy_clipboard (old_editable);
  btk_editable_delete_selection (BTK_EDITABLE (old_editable));
}

static void
btk_old_editable_real_copy_clipboard (BtkOldEditable *old_editable)
{
  gint selection_start_pos; 
  gint selection_end_pos;

  selection_start_pos = MIN (old_editable->selection_start_pos, old_editable->selection_end_pos);
  selection_end_pos = MAX (old_editable->selection_start_pos, old_editable->selection_end_pos);

  if (selection_start_pos != selection_end_pos)
    {
      gchar *text = btk_old_editable_get_public_chars (old_editable,
						       selection_start_pos,
						       selection_end_pos);

      if (text)
	{
	  BtkClipboard *clipboard = btk_widget_get_clipboard (BTK_WIDGET (old_editable),
							      BDK_SELECTION_CLIPBOARD);
	  
	  btk_clipboard_set_text (clipboard, text, -1);
	  g_free (text);
	}
    }
}

static void
btk_old_editable_real_paste_clipboard (BtkOldEditable *old_editable)
{
  BtkClipboard *clipboard = btk_widget_get_clipboard (BTK_WIDGET (old_editable), 
						      BDK_SELECTION_CLIPBOARD);

  g_object_ref (G_OBJECT (old_editable));
  btk_clipboard_request_text (clipboard, old_editable_text_received_cb, old_editable);
}

/**
 * btk_old_editable_changed:
 * @old_editable: a #BtkOldEditable
 *
 * Emits the ::changed signal on @old_editable.
 */
void
btk_old_editable_changed (BtkOldEditable *old_editable)
{
  g_return_if_fail (BTK_IS_OLD_EDITABLE (old_editable));
  
  g_signal_emit_by_name (old_editable, "changed");
}

#define __BTK_OLD_EDITABLE_C__
#include "btkaliasdef.c"
