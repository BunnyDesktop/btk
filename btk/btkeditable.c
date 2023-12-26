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

#include "config.h"
#include <string.h>

#include "btkeditable.h"
#include "btkmarshalers.h"
#include "btkintl.h"
#include "btkalias.h"


static void btk_editable_base_init (gpointer g_class);


GType
btk_editable_get_type (void)
{
  static GType editable_type = 0;

  if (!editable_type)
    {
      const GTypeInfo editable_info =
      {
	sizeof (BtkEditableClass),  /* class_size */
	btk_editable_base_init,	    /* base_init */
	NULL,			    /* base_finalize */
      };

      editable_type = g_type_register_static (B_TYPE_INTERFACE, I_("BtkEditable"),
					      &editable_info, 0);
    }

  return editable_type;
}

static void
btk_editable_base_init (gpointer g_class)
{
  static gboolean initialized = FALSE;

  if (! initialized)
    {
      /**
       * BtkEditable::insert-text:
       * @editable: the object which received the signal
       * @new_text: the new text to insert
       * @new_text_length: the length of the new text, in bytes,
       *     or -1 if new_text is nul-terminated
       * @position: (inout) (type int): the position, in characters,
       *     at which to insert the new text. this is an in-out
       *     parameter.  After the signal emission is finished, it
       *     should point after the newly inserted text.
       *
       * This signal is emitted when text is inserted into
       * the widget by the user. The default handler for
       * this signal will normally be responsible for inserting
       * the text, so by connecting to this signal and then
       * stopping the signal with g_signal_stop_emission(), it
       * is possible to modify the inserted text, or prevent
       * it from being inserted entirely.
       */
      g_signal_new (I_("insert-text"),
		    BTK_TYPE_EDITABLE,
		    G_SIGNAL_RUN_LAST,
		    G_STRUCT_OFFSET (BtkEditableClass, insert_text),
		    NULL, NULL,
		    _btk_marshal_VOID__STRING_INT_POINTER,
		    B_TYPE_NONE, 3,
		    B_TYPE_STRING,
		    B_TYPE_INT,
		    B_TYPE_POINTER);

      /**
       * BtkEditable::delete-text:
       * @editable: the object which received the signal
       * @start_pos: the starting position
       * @end_pos: the end position
       * 
       * This signal is emitted when text is deleted from
       * the widget by the user. The default handler for
       * this signal will normally be responsible for deleting
       * the text, so by connecting to this signal and then
       * stopping the signal with g_signal_stop_emission(), it
       * is possible to modify the range of deleted text, or
       * prevent it from being deleted entirely. The @start_pos
       * and @end_pos parameters are interpreted as for
       * btk_editable_delete_text().
       */
      g_signal_new (I_("delete-text"),
		    BTK_TYPE_EDITABLE,
		    G_SIGNAL_RUN_LAST,
		    G_STRUCT_OFFSET (BtkEditableClass, delete_text),
		    NULL, NULL,
		    _btk_marshal_VOID__INT_INT,
		    B_TYPE_NONE, 2,
		    B_TYPE_INT,
		    B_TYPE_INT);
      /**
       * BtkEditable::changed:
       * @editable: the object which received the signal
       *
       * The ::changed signal is emitted at the end of a single
       * user-visible operation on the contents of the #BtkEditable.
       *
       * E.g., a paste operation that replaces the contents of the
       * selection will cause only one signal emission (even though it
       * is implemented by first deleting the selection, then inserting
       * the new content, and may cause multiple ::notify::text signals
       * to be emitted).
       */ 
      g_signal_new (I_("changed"),
		    BTK_TYPE_EDITABLE,
		    G_SIGNAL_RUN_LAST,
		    G_STRUCT_OFFSET (BtkEditableClass, changed),
		    NULL, NULL,
		    _btk_marshal_VOID__VOID,
		    B_TYPE_NONE, 0);

      initialized = TRUE;
    }
}

/**
 * btk_editable_insert_text:
 * @editable: a #BtkEditable
 * @new_text: the text to append
 * @new_text_length: the length of the text in bytes, or -1
 * @position: (inout): location of the position text will be inserted at
 *
 * Inserts @new_text_length bytes of @new_text into the contents of the
 * widget, at position @position.
 *
 * Note that the position is in characters, not in bytes. 
 * The function updates @position to point after the newly inserted text.
 */
void
btk_editable_insert_text (BtkEditable *editable,
			  const gchar *new_text,
			  gint         new_text_length,
			  gint        *position)
{
  g_return_if_fail (BTK_IS_EDITABLE (editable));
  g_return_if_fail (position != NULL);

  if (new_text_length < 0)
    new_text_length = strlen (new_text);
  
  BTK_EDITABLE_GET_CLASS (editable)->do_insert_text (editable, new_text, new_text_length, position);
}

/**
 * btk_editable_delete_text:
 * @editable: a #BtkEditable
 * @start_pos: start position
 * @end_pos: end position
 *
 * Deletes a sequence of characters. The characters that are deleted are 
 * those characters at positions from @start_pos up to, but not including 
 * @end_pos. If @end_pos is negative, then the the characters deleted
 * are those from @start_pos to the end of the text.
 *
 * Note that the positions are specified in characters, not bytes.
 */
void
btk_editable_delete_text (BtkEditable *editable,
			  gint         start_pos,
			  gint         end_pos)
{
  g_return_if_fail (BTK_IS_EDITABLE (editable));

  BTK_EDITABLE_GET_CLASS (editable)->do_delete_text (editable, start_pos, end_pos);
}

/**
 * btk_editable_get_chars:
 * @editable: a #BtkEditable
 * @start_pos: start of text
 * @end_pos: end of text
 *
 * Retrieves a sequence of characters. The characters that are retrieved 
 * are those characters at positions from @start_pos up to, but not 
 * including @end_pos. If @end_pos is negative, then the the characters 
 * retrieved are those characters from @start_pos to the end of the text.
 * 
 * Note that positions are specified in characters, not bytes.
 *
 * Return value: a pointer to the contents of the widget as a
 *      string. This string is allocated by the #BtkEditable
 *      implementation and should be freed by the caller.
 */
gchar *    
btk_editable_get_chars (BtkEditable *editable,
			gint         start_pos,
			gint         end_pos)
{
  g_return_val_if_fail (BTK_IS_EDITABLE (editable), NULL);

  return BTK_EDITABLE_GET_CLASS (editable)->get_chars (editable, start_pos, end_pos);
}

/**
 * btk_editable_set_position:
 * @editable: a #BtkEditable
 * @position: the position of the cursor 
 *
 * Sets the cursor position in the editable to the given value.
 *
 * The cursor is displayed before the character with the given (base 0) 
 * index in the contents of the editable. The value must be less than or 
 * equal to the number of characters in the editable. A value of -1 
 * indicates that the position should be set after the last character 
 * of the editable. Note that @position is in characters, not in bytes.
 */
void
btk_editable_set_position (BtkEditable      *editable,
			   gint              position)
{
  g_return_if_fail (BTK_IS_EDITABLE (editable));

  BTK_EDITABLE_GET_CLASS (editable)->set_position (editable, position);
}

/**
 * btk_editable_get_position:
 * @editable: a #BtkEditable
 *
 * Retrieves the current position of the cursor relative to the start
 * of the content of the editable. 
 * 
 * Note that this position is in characters, not in bytes.
 *
 * Return value: the cursor position
 */
gint
btk_editable_get_position (BtkEditable *editable)
{
  g_return_val_if_fail (BTK_IS_EDITABLE (editable), 0);

  return BTK_EDITABLE_GET_CLASS (editable)->get_position (editable);
}

/**
 * btk_editable_get_selection_bounds:
 * @editable: a #BtkEditable
 * @start_pos: (out) (allow-none): location to store the starting position, or %NULL
 * @end_pos: (out) (allow-none): location to store the end position, or %NULL
 *
 * Retrieves the selection bound of the editable. start_pos will be filled
 * with the start of the selection and @end_pos with end. If no text was
 * selected both will be identical and %FALSE will be returned.
 *
 * Note that positions are specified in characters, not bytes.
 *
 * Return value: %TRUE if an area is selected, %FALSE otherwise
 */
gboolean
btk_editable_get_selection_bounds (BtkEditable *editable,
				   gint        *start_pos,
				   gint        *end_pos)
{
  gint tmp_start, tmp_end;
  gboolean result;
  
  g_return_val_if_fail (BTK_IS_EDITABLE (editable), FALSE);

  result = BTK_EDITABLE_GET_CLASS (editable)->get_selection_bounds (editable, &tmp_start, &tmp_end);

  if (start_pos)
    *start_pos = MIN (tmp_start, tmp_end);
  if (end_pos)
    *end_pos = MAX (tmp_start, tmp_end);

  return result;
}

/**
 * btk_editable_delete_selection:
 * @editable: a #BtkEditable
 *
 * Deletes the currently selected text of the editable.
 * This call doesn't do anything if there is no selected text.
 */
void
btk_editable_delete_selection (BtkEditable *editable)
{
  gint start, end;

  g_return_if_fail (BTK_IS_EDITABLE (editable));

  if (btk_editable_get_selection_bounds (editable, &start, &end))
    btk_editable_delete_text (editable, start, end);
}

/**
 * btk_editable_select_rebunnyion:
 * @editable: a #BtkEditable
 * @start_pos: start of rebunnyion
 * @end_pos: end of rebunnyion
 *
 * Selects a rebunnyion of text. The characters that are selected are 
 * those characters at positions from @start_pos up to, but not 
 * including @end_pos. If @end_pos is negative, then the the 
 * characters selected are those characters from @start_pos to 
 * the end of the text.
 * 
 * Note that positions are specified in characters, not bytes.
 */
void
btk_editable_select_rebunnyion (BtkEditable *editable,
			    gint         start_pos,
			    gint         end_pos)
{
  g_return_if_fail (BTK_IS_EDITABLE (editable));
  
  BTK_EDITABLE_GET_CLASS (editable)->set_selection_bounds (editable, start_pos, end_pos);
}

/**
 * btk_editable_cut_clipboard:
 * @editable: a #BtkEditable
 *
 * Removes the contents of the currently selected content in the editable and
 * puts it on the clipboard.
 */
void
btk_editable_cut_clipboard (BtkEditable *editable)
{
  g_return_if_fail (BTK_IS_EDITABLE (editable));
  
  g_signal_emit_by_name (editable, "cut-clipboard");
}

/**
 * btk_editable_copy_clipboard:
 * @editable: a #BtkEditable
 *
 * Copies the contents of the currently selected content in the editable and
 * puts it on the clipboard.
 */
void
btk_editable_copy_clipboard (BtkEditable *editable)
{
  g_return_if_fail (BTK_IS_EDITABLE (editable));
  
  g_signal_emit_by_name (editable, "copy-clipboard");
}

/**
 * btk_editable_paste_clipboard:
 * @editable: a #BtkEditable
 *
 * Pastes the content of the clipboard to the current position of the
 * cursor in the editable.
 */
void
btk_editable_paste_clipboard (BtkEditable *editable)
{
  g_return_if_fail (BTK_IS_EDITABLE (editable));
  
  g_signal_emit_by_name (editable, "paste-clipboard");
}

/**
 * btk_editable_set_editable:
 * @editable: a #BtkEditable
 * @is_editable: %TRUE if the user is allowed to edit the text
 *   in the widget
 *
 * Determines if the user can edit the text in the editable
 * widget or not. 
 */
void
btk_editable_set_editable (BtkEditable    *editable,
			   gboolean        is_editable)
{
  g_return_if_fail (BTK_IS_EDITABLE (editable));

  g_object_set (editable,
		"editable", is_editable != FALSE,
		NULL);
}

/**
 * btk_editable_get_editable:
 * @editable: a #BtkEditable
 *
 * Retrieves whether @editable is editable. See
 * btk_editable_set_editable().
 *
 * Return value: %TRUE if @editable is editable.
 */
gboolean
btk_editable_get_editable (BtkEditable *editable)
{
  gboolean value;

  g_return_val_if_fail (BTK_IS_EDITABLE (editable), FALSE);

  g_object_get (editable, "editable", &value, NULL);

  return value;
}

#define __BTK_EDITABLE_C__
#include "btkaliasdef.c"
