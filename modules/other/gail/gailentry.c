/* BAIL - The GNOME Accessibility Implementation Library
 * Copyright 2001, 2002, 2003 Sun Microsystems Inc.
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

#include <string.h>
#include <btk/btk.h>
#include <bdk/bdkkeysyms.h>
#include "bailentry.h"
#include "bailcombo.h"
#include "bailcombobox.h"
#include <libbail-util/bailmisc.h>

static void       bail_entry_class_init            (BailEntryClass       *klass);
static void       bail_entry_init                  (BailEntry            *entry);
static void	  bail_entry_real_initialize       (BatkObject            *obj,
                                                    gpointer             data);
static void       text_setup                       (BailEntry            *entry,
                                                    BtkEntry             *btk_entry);
static void	  bail_entry_real_notify_btk	   (GObject		 *obj,
                                                    GParamSpec		 *pspec);
static void       bail_entry_finalize              (GObject              *object);

static gint       bail_entry_get_index_in_parent   (BatkObject            *accessible);

/* batkobject.h */

static BatkStateSet* bail_entry_ref_state_set       (BatkObject            *accessible);

/* batktext.h */

static void       batk_text_interface_init          (BatkTextIface         *iface);

static gchar*     bail_entry_get_text              (BatkText              *text,
                                                    gint                 start_pos,
                                                    gint                 end_pos);
static gunichar	  bail_entry_get_character_at_offset
						   (BatkText		 *text,
						    gint		 offset);
static gchar*	  bail_entry_get_text_before_offset(BatkText		 *text,
						    gint		 offset,
						    BatkTextBoundary	 boundary_type,
						    gint		 *start_offset,
						    gint		 *end_offset);
static gchar*	  bail_entry_get_text_at_offset	   (BatkText		 *text,
						    gint		 offset,
						    BatkTextBoundary	 boundary_type,
						    gint		 *start_offset,
						    gint		 *end_offset);
static gchar*	  bail_entry_get_text_after_offset (BatkText		 *text,
						    gint		 offset,
						    BatkTextBoundary	 boundary_type,
						    gint		 *start_offset,
						    gint		 *end_offset);
static gint       bail_entry_get_caret_offset      (BatkText              *text);
static gboolean   bail_entry_set_caret_offset      (BatkText              *text,
						    gint                 offset);
static gint	  bail_entry_get_n_selections	   (BatkText		 *text);
static gchar*	  bail_entry_get_selection	   (BatkText		 *text,
						    gint		 selection_num,
						    gint		 *start_offset,
						    gint		 *end_offset);
static gboolean	  bail_entry_add_selection	   (BatkText		 *text,
						    gint		 start_offset,
						    gint		 end_offset);
static gboolean	  bail_entry_remove_selection	   (BatkText		 *text,
						    gint		 selection_num);
static gboolean	  bail_entry_set_selection	   (BatkText		 *text,
						    gint		 selection_num,
						    gint		 start_offset,
						    gint		 end_offset);
static gint	  bail_entry_get_character_count   (BatkText		 *text);
static BatkAttributeSet *  bail_entry_get_run_attributes 
                                                   (BatkText              *text,
						    gint		 offset,
        					    gint		 *start_offset,
					       	    gint 		 *end_offset);
static BatkAttributeSet *  bail_entry_get_default_attributes 
                                                   (BatkText              *text);
static void bail_entry_get_character_extents       (BatkText	         *text,
						    gint 	         offset,
		                                    gint 	         *x,
                    		   	            gint 	         *y,
                                		    gint 	         *width,
                                     		    gint 	         *height,
			        		    BatkCoordType         coords);
static gint bail_entry_get_offset_at_point         (BatkText              *text,
                                                    gint                 x,
                                                    gint                 y,
			                            BatkCoordType         coords);
/* batkeditabletext.h */

static void       batk_editable_text_interface_init (BatkEditableTextIface *iface);
static void       bail_entry_set_text_contents     (BatkEditableText      *text,
                                                    const gchar          *string);
static void       bail_entry_insert_text           (BatkEditableText      *text,
                                                    const gchar          *string,
                                                    gint                 length,
                                                    gint                 *position);
static void       bail_entry_copy_text             (BatkEditableText      *text,
                                                    gint                 start_pos,
                                                    gint                 end_pos);
static void       bail_entry_cut_text              (BatkEditableText      *text,
                                                    gint                 start_pos,
                                                    gint                 end_pos);
static void       bail_entry_delete_text           (BatkEditableText      *text,
                                                    gint                 start_pos,
                                                    gint                 end_pos);
static void       bail_entry_paste_text            (BatkEditableText      *text,
                                                    gint                 position);
static void       bail_entry_paste_received	   (BtkClipboard *clipboard,
						    const gchar  *text,
						    gpointer     data);


/* Callbacks */

static gboolean   bail_entry_idle_notify_insert    (gpointer data);
static void       bail_entry_notify_insert         (BailEntry            *entry);
static void       bail_entry_notify_delete         (BailEntry            *entry);
static void	  _bail_entry_insert_text_cb	   (BtkEntry     	 *entry,
                                                    gchar		 *arg1,
                                                    gint		 arg2,
                                                    gpointer		 arg3);
static void	  _bail_entry_delete_text_cb	   (BtkEntry		 *entry,
                                                    gint		 arg1,
                                                    gint		 arg2);
static void	  _bail_entry_changed_cb           (BtkEntry		 *entry);
static gboolean   check_for_selection_change       (BailEntry            *entry,
                                                    BtkEntry             *btk_entry);

static void                  batk_action_interface_init   (BatkActionIface  *iface);

static gboolean              bail_entry_do_action        (BatkAction       *action,
                                                          gint            i);
static gboolean              idle_do_action              (gpointer        data);
static gint                  bail_entry_get_n_actions    (BatkAction       *action);
static const gchar*          bail_entry_get_description  (BatkAction       *action,
                                                          gint            i);
static const gchar*          bail_entry_get_keybinding   (BatkAction       *action,
                                                          gint            i);
static const gchar*          bail_entry_action_get_name  (BatkAction       *action,
                                                          gint            i);
static gboolean              bail_entry_set_description  (BatkAction       *action,
                                                          gint            i,
                                                          const gchar     *desc);

typedef struct _BailEntryPaste			BailEntryPaste;

struct _BailEntryPaste
{
  BtkEntry* entry;
  gint position;
};

G_DEFINE_TYPE_WITH_CODE (BailEntry, bail_entry, BAIL_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_EDITABLE_TEXT, batk_editable_text_interface_init)
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_TEXT, batk_text_interface_init)
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_ACTION, batk_action_interface_init))

static void
bail_entry_class_init (BailEntryClass *klass)
{
  GObjectClass *bobject_class = G_OBJECT_CLASS (klass);
  BatkObjectClass  *class = BATK_OBJECT_CLASS (klass);
  BailWidgetClass *widget_class;

  widget_class = (BailWidgetClass*)klass;

  bobject_class->finalize = bail_entry_finalize;

  class->ref_state_set = bail_entry_ref_state_set;
  class->get_index_in_parent = bail_entry_get_index_in_parent;
  class->initialize = bail_entry_real_initialize;

  widget_class->notify_btk = bail_entry_real_notify_btk;
}

static void
bail_entry_init (BailEntry *entry)
{
  entry->textutil = NULL;
  entry->signal_name_insert = NULL;
  entry->signal_name_delete = NULL;
  entry->cursor_position = 0;
  entry->selection_bound = 0;
  entry->activate_description = NULL;
  entry->activate_keybinding = NULL;
}

static void
bail_entry_real_initialize (BatkObject *obj, 
                            gpointer  data)
{
  BtkEntry *entry;
  BailEntry *bail_entry;

  BATK_OBJECT_CLASS (bail_entry_parent_class)->initialize (obj, data);

  bail_entry = BAIL_ENTRY (obj);
  bail_entry->textutil = bail_text_util_new ();
  
  g_assert (BTK_IS_ENTRY (data));

  entry = BTK_ENTRY (data);
  text_setup (bail_entry, entry);
  bail_entry->cursor_position = entry->current_pos;
  bail_entry->selection_bound = entry->selection_bound;

  /* Set up signal callbacks */
  g_signal_connect (data, "insert-text",
	G_CALLBACK (_bail_entry_insert_text_cb), NULL);
  g_signal_connect (data, "delete-text",
	G_CALLBACK (_bail_entry_delete_text_cb), NULL);
  g_signal_connect (data, "changed",
	G_CALLBACK (_bail_entry_changed_cb), NULL);

  if (btk_entry_get_visibility (entry))
    obj->role = BATK_ROLE_TEXT;
  else
    obj->role = BATK_ROLE_PASSWORD_TEXT;
}

static void
bail_entry_real_notify_btk (GObject		*obj,
                            GParamSpec		*pspec)
{
  BtkWidget *widget;
  BatkObject* batk_obj;
  BtkEntry* btk_entry;
  BailEntry* entry;

  widget = BTK_WIDGET (obj);
  batk_obj = btk_widget_get_accessible (widget);
  btk_entry = BTK_ENTRY (widget);
  entry = BAIL_ENTRY (batk_obj);

  if (strcmp (pspec->name, "cursor-position") == 0)
    {
      if (entry->insert_idle_handler == 0)
        entry->insert_idle_handler = bdk_threads_add_idle (bail_entry_idle_notify_insert, entry);

      if (check_for_selection_change (entry, btk_entry))
        g_signal_emit_by_name (batk_obj, "text_selection_changed");
      /*
       * The entry cursor position has moved so generate the signal.
       */
      g_signal_emit_by_name (batk_obj, "text_caret_moved", 
                             entry->cursor_position);
    }
  else if (strcmp (pspec->name, "selection-bound") == 0)
    {
      if (entry->insert_idle_handler == 0)
        entry->insert_idle_handler = bdk_threads_add_idle (bail_entry_idle_notify_insert, entry);

      if (check_for_selection_change (entry, btk_entry))
        g_signal_emit_by_name (batk_obj, "text_selection_changed");
    }
  else if (strcmp (pspec->name, "editable") == 0)
    {
      gboolean value;

      g_object_get (obj, "editable", &value, NULL);
      batk_object_notify_state_change (batk_obj, BATK_STATE_EDITABLE,
                                               value);
    }
  else if (strcmp (pspec->name, "visibility") == 0)
    {
      gboolean visibility;
      BatkRole new_role;

      text_setup (entry, btk_entry);
      visibility = btk_entry_get_visibility (btk_entry);
      new_role = visibility ? BATK_ROLE_TEXT : BATK_ROLE_PASSWORD_TEXT;
      batk_object_set_role (batk_obj, new_role);
    }
  else if (strcmp (pspec->name, "invisible-char") == 0)
    {
      text_setup (entry, btk_entry);
    }
  else if (strcmp (pspec->name, "editing-canceled") == 0)
    {
      if (entry->insert_idle_handler)
        {
          g_source_remove (entry->insert_idle_handler);
          entry->insert_idle_handler = 0;
        }
    }
  else
    BAIL_WIDGET_CLASS (bail_entry_parent_class)->notify_btk (obj, pspec);
}

static void
text_setup (BailEntry *entry,
            BtkEntry  *btk_entry)
{
  if (btk_entry_get_visibility (btk_entry))
    {
      bail_text_util_text_setup (entry->textutil, btk_entry_get_text (btk_entry));
    }
  else
    {
      gunichar invisible_char;
      GString *tmp_string = g_string_new (NULL);
      gint ch_len; 
      gchar buf[7];
      guint length;
      gint i;

      invisible_char = btk_entry_get_invisible_char (btk_entry);
      if (invisible_char == 0)
        invisible_char = ' ';

      ch_len = g_unichar_to_utf8 (invisible_char, buf);
      length = btk_entry_get_text_length (btk_entry);
      for (i = 0; i < length; i++)
        {
          g_string_append_len (tmp_string, buf, ch_len);
        }

      bail_text_util_text_setup (entry->textutil, tmp_string->str);
      g_string_free (tmp_string, TRUE);

    } 
}

static void
bail_entry_finalize (GObject            *object)
{
  BailEntry *entry = BAIL_ENTRY (object);

  g_object_unref (entry->textutil);
  g_free (entry->activate_description);
  g_free (entry->activate_keybinding);
  if (entry->action_idle_handler)
    {
      g_source_remove (entry->action_idle_handler);
      entry->action_idle_handler = 0;
    }
  if (entry->insert_idle_handler)
    {
      g_source_remove (entry->insert_idle_handler);
      entry->insert_idle_handler = 0;
    }
  G_OBJECT_CLASS (bail_entry_parent_class)->finalize (object);
}

static gint
bail_entry_get_index_in_parent (BatkObject *accessible)
{
  /*
   * If the parent widget is a combo box then the index is 1
   * otherwise do the normal thing.
   */
  if (accessible->accessible_parent)
    if (BAIL_IS_COMBO (accessible->accessible_parent) ||
        BAIL_IS_COMBO_BOX (accessible->accessible_parent))
      return 1;

  return BATK_OBJECT_CLASS (bail_entry_parent_class)->get_index_in_parent (accessible);
}

/* batkobject.h */

static BatkStateSet*
bail_entry_ref_state_set (BatkObject *accessible)
{
  BatkStateSet *state_set;
  BtkEntry *entry;
  gboolean value;
  BtkWidget *widget;

  state_set = BATK_OBJECT_CLASS (bail_entry_parent_class)->ref_state_set (accessible);
  widget = BTK_ACCESSIBLE (accessible)->widget;
  
  if (widget == NULL)
    return state_set;

  entry = BTK_ENTRY (widget);

  g_object_get (G_OBJECT (entry), "editable", &value, NULL);
  if (value)
    batk_state_set_add_state (state_set, BATK_STATE_EDITABLE);
  batk_state_set_add_state (state_set, BATK_STATE_SINGLE_LINE);

  return state_set;
}

/* batktext.h */

static void
batk_text_interface_init (BatkTextIface *iface)
{
  iface->get_text = bail_entry_get_text;
  iface->get_character_at_offset = bail_entry_get_character_at_offset;
  iface->get_text_before_offset = bail_entry_get_text_before_offset;
  iface->get_text_at_offset = bail_entry_get_text_at_offset;
  iface->get_text_after_offset = bail_entry_get_text_after_offset;
  iface->get_caret_offset = bail_entry_get_caret_offset;
  iface->set_caret_offset = bail_entry_set_caret_offset;
  iface->get_character_count = bail_entry_get_character_count;
  iface->get_n_selections = bail_entry_get_n_selections;
  iface->get_selection = bail_entry_get_selection;
  iface->add_selection = bail_entry_add_selection;
  iface->remove_selection = bail_entry_remove_selection;
  iface->set_selection = bail_entry_set_selection;
  iface->get_run_attributes = bail_entry_get_run_attributes;
  iface->get_default_attributes = bail_entry_get_default_attributes;
  iface->get_character_extents = bail_entry_get_character_extents;
  iface->get_offset_at_point = bail_entry_get_offset_at_point;
}

static gchar*
bail_entry_get_text (BatkText *text,
                     gint    start_pos,
                     gint    end_pos)
{
  BtkWidget *widget;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  return bail_text_util_get_substring (BAIL_ENTRY (text)->textutil, start_pos, end_pos);
}

static gchar*
bail_entry_get_text_before_offset (BatkText	    *text,
				   gint		    offset,
				   BatkTextBoundary  boundary_type,
				   gint		    *start_offset,
				   gint		    *end_offset)
{
  BtkWidget *widget;
  BtkEntry *entry;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  /* Get Entry */
  entry = BTK_ENTRY (widget);

  return bail_text_util_get_text (BAIL_ENTRY (text)->textutil,
                          btk_entry_get_layout (entry), BAIL_BEFORE_OFFSET, 
                          boundary_type, offset, start_offset, end_offset);
}

static gchar*
bail_entry_get_text_at_offset (BatkText          *text,
                               gint             offset,
                               BatkTextBoundary  boundary_type,
                               gint             *start_offset,
                               gint             *end_offset)
{
  BtkWidget *widget;
  BtkEntry *entry;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  /* Get Entry */
  entry = BTK_ENTRY (widget);

  return bail_text_util_get_text (BAIL_ENTRY (text)->textutil,
                            btk_entry_get_layout (entry), BAIL_AT_OFFSET, 
                            boundary_type, offset, start_offset, end_offset);
}

static gchar*
bail_entry_get_text_after_offset  (BatkText	    *text,
				   gint		    offset,
				   BatkTextBoundary  boundary_type,
				   gint		    *start_offset,
				   gint		    *end_offset)
{
  BtkWidget *widget;
  BtkEntry *entry;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  /* Get Entry */
  entry = BTK_ENTRY (widget);

  return bail_text_util_get_text (BAIL_ENTRY (text)->textutil,
                           btk_entry_get_layout (entry), BAIL_AFTER_OFFSET, 
                           boundary_type, offset, start_offset, end_offset);
}

static gint
bail_entry_get_character_count (BatkText *text)
{
  BtkEntry *entry;
  BtkWidget *widget;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return 0;

  entry = BTK_ENTRY (widget);
  return g_utf8_strlen (btk_entry_get_text (entry), -1);
}

static gint
bail_entry_get_caret_offset (BatkText *text)
{
  BtkEntry *entry;
  BtkWidget *widget;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return 0;

  entry = BTK_ENTRY (widget);

  return btk_editable_get_position (BTK_EDITABLE (entry));
}

static gboolean
bail_entry_set_caret_offset (BatkText *text, gint offset)
{
  BtkEntry *entry;
  BtkWidget *widget;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return FALSE;

  entry = BTK_ENTRY (widget);

  btk_editable_set_position (BTK_EDITABLE (entry), offset);
  return TRUE;
}

static BatkAttributeSet*
bail_entry_get_run_attributes (BatkText *text,
			       gint    offset,
                               gint    *start_offset,
                               gint    *end_offset)
{
  BtkWidget *widget;
  BtkEntry *entry;
  BatkAttributeSet *at_set = NULL;
  BtkTextDirection dir;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  entry = BTK_ENTRY (widget);
 
  dir = btk_widget_get_direction (widget);
  if (dir == BTK_TEXT_DIR_RTL)
    {
      at_set = bail_misc_add_attribute (at_set,
                                        BATK_TEXT_ATTR_DIRECTION,
       g_strdup (batk_text_attribute_get_value (BATK_TEXT_ATTR_DIRECTION, dir)));
    }

  at_set = bail_misc_layout_get_run_attributes (at_set,
                                                btk_entry_get_layout (entry),
                                                (gchar*)btk_entry_get_text (entry),
                                                offset,
                                                start_offset,
                                                end_offset);
  return at_set;
}

static BatkAttributeSet*
bail_entry_get_default_attributes (BatkText *text)
{
  BtkWidget *widget;
  BtkEntry *entry;
  BatkAttributeSet *at_set = NULL;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  entry = BTK_ENTRY (widget);

  at_set = bail_misc_get_default_attributes (at_set,
                                             btk_entry_get_layout (entry),
                                             widget);
  return at_set;
}
  
static void
bail_entry_get_character_extents (BatkText *text,
				  gint    offset,
		                  gint    *x,
                    		  gint 	  *y,
                                  gint 	  *width,
                                  gint 	  *height,
			          BatkCoordType coords)
{
  BtkWidget *widget;
  BtkEntry *entry;
  BangoRectangle char_rect;
  gint index, cursor_index, x_layout, y_layout;
  const gchar *entry_text;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return;

  entry = BTK_ENTRY (widget);

  btk_entry_get_layout_offsets (entry, &x_layout, &y_layout);
  entry_text = btk_entry_get_text (entry);
  index = g_utf8_offset_to_pointer (entry_text, offset) - entry_text;
  cursor_index = g_utf8_offset_to_pointer (entry_text, entry->current_pos) - entry_text;
  if (index > cursor_index)
    index += entry->preedit_length;
  bango_layout_index_to_pos (btk_entry_get_layout(entry), index, &char_rect);
 
  bail_misc_get_extents_from_bango_rectangle (widget, &char_rect, 
                        x_layout, y_layout, x, y, width, height, coords);
} 

static gint 
bail_entry_get_offset_at_point (BatkText *text,
                                gint x,
                                gint y,
			        BatkCoordType coords)
{ 
  BtkWidget *widget;
  BtkEntry *entry;
  gint index, cursor_index, x_layout, y_layout;
  const gchar *entry_text;
  
  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return -1;

  entry = BTK_ENTRY (widget);
  
  btk_entry_get_layout_offsets (entry, &x_layout, &y_layout);
  entry_text = btk_entry_get_text (entry);
  
  index = bail_misc_get_index_at_point_in_layout (widget, 
               btk_entry_get_layout(entry), x_layout, y_layout, x, y, coords);
  if (index == -1)
    {
      if (coords == BATK_XY_SCREEN || coords == BATK_XY_WINDOW)
        return g_utf8_strlen (entry_text, -1);

      return index;  
    }
  else
    {
      cursor_index = g_utf8_offset_to_pointer (entry_text, entry->current_pos) - entry_text;
      if (index >= cursor_index && entry->preedit_length)
        {
          if (index >= cursor_index + entry->preedit_length)
            index -= entry->preedit_length;
          else
            index = cursor_index;
        }
      return g_utf8_pointer_to_offset (entry_text, entry_text + index);
    }
}

static gint
bail_entry_get_n_selections (BatkText              *text)
{
  BtkEntry *entry;
  BtkWidget *widget;
  gint select_start, select_end;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return -1;

  entry = BTK_ENTRY (widget);

  btk_editable_get_selection_bounds (BTK_EDITABLE (entry), &select_start, 
                                     &select_end);

  if (select_start != select_end)
    return 1;
  else
    return 0;
}

static gchar*
bail_entry_get_selection (BatkText *text,
			  gint    selection_num,
                          gint    *start_pos,
                          gint    *end_pos)
{
  BtkEntry *entry;
  BtkWidget *widget;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return NULL;

 /* Only let the user get the selection if one is set, and if the
  * selection_num is 0.
  */
  if (selection_num != 0)
     return NULL;

  entry = BTK_ENTRY (widget);
  btk_editable_get_selection_bounds (BTK_EDITABLE (entry), start_pos, end_pos);

  if (*start_pos != *end_pos)
     return btk_editable_get_chars (BTK_EDITABLE (entry), *start_pos, *end_pos);
  else
     return NULL;
}

static gboolean
bail_entry_add_selection (BatkText *text,
                          gint    start_pos,
                          gint    end_pos)
{
  BtkEntry *entry;
  BtkWidget *widget;
  gint select_start, select_end;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return FALSE;

  entry = BTK_ENTRY (widget);

  btk_editable_get_selection_bounds (BTK_EDITABLE (entry), &select_start, 
                                     &select_end);

 /* If there is already a selection, then don't allow another to be added,
  * since BtkEntry only supports one selected rebunnyion.
  */
  if (select_start == select_end)
    {
       btk_editable_select_rebunnyion (BTK_EDITABLE (entry), start_pos, end_pos);
       return TRUE;
    }
  else
   return FALSE;
}

static gboolean
bail_entry_remove_selection (BatkText *text,
                             gint    selection_num)
{
  BtkEntry *entry;
  BtkWidget *widget;
  gint select_start, select_end, caret_pos;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return FALSE;

  if (selection_num != 0)
     return FALSE;

  entry = BTK_ENTRY (widget);
  btk_editable_get_selection_bounds (BTK_EDITABLE (entry), &select_start, 
                                     &select_end);

  if (select_start != select_end)
    {
     /* Setting the start & end of the selected rebunnyion to the caret position
      * turns off the selection.
      */
      caret_pos = btk_editable_get_position (BTK_EDITABLE (entry));
      btk_editable_select_rebunnyion (BTK_EDITABLE (entry), caret_pos, caret_pos);
      return TRUE;
    }
  else
    return FALSE;
}

static gboolean
bail_entry_set_selection (BatkText *text,
			  gint	  selection_num,
                          gint    start_pos,
                          gint    end_pos)
{
  BtkEntry *entry;
  BtkWidget *widget;
  gint select_start, select_end;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return FALSE;

 /* Only let the user move the selection if one is set, and if the
  * selection_num is 0
  */
  if (selection_num != 0)
     return FALSE;

  entry = BTK_ENTRY (widget);

  btk_editable_get_selection_bounds (BTK_EDITABLE (entry), &select_start, 
                                     &select_end);

  if (select_start != select_end)
    {
      btk_editable_select_rebunnyion (BTK_EDITABLE (entry), start_pos, end_pos);
      return TRUE;
    }
  else
    return FALSE;
}

static void
batk_editable_text_interface_init (BatkEditableTextIface *iface)
{
  iface->set_text_contents = bail_entry_set_text_contents;
  iface->insert_text = bail_entry_insert_text;
  iface->copy_text = bail_entry_copy_text;
  iface->cut_text = bail_entry_cut_text;
  iface->delete_text = bail_entry_delete_text;
  iface->paste_text = bail_entry_paste_text;
  iface->set_run_attributes = NULL;
}

static void
bail_entry_set_text_contents (BatkEditableText *text,
                              const gchar     *string)
{
  BtkEntry *entry;
  BtkWidget *widget;
  BtkEditable *editable;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return;

  entry = BTK_ENTRY (widget);
  editable = BTK_EDITABLE (entry);
  if (!btk_editable_get_editable (editable))
    return;

  btk_entry_set_text (entry, string);
}

static void
bail_entry_insert_text (BatkEditableText *text,
                        const gchar     *string,
                        gint            length,
                        gint            *position)
{
  BtkEntry *entry;
  BtkWidget *widget;
  BtkEditable *editable;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return;

  entry = BTK_ENTRY (widget);
  editable = BTK_EDITABLE (entry);
  if (!btk_editable_get_editable (editable))
    return;

  btk_editable_insert_text (editable, string, length, position);
  btk_editable_set_position (editable, *position);
}

static void
bail_entry_copy_text   (BatkEditableText *text,
                        gint            start_pos,
                        gint            end_pos)
{
  BtkEntry *entry;
  BtkWidget *widget;
  BtkEditable *editable;
  gchar *str;
  BtkClipboard *clipboard;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return;

  entry = BTK_ENTRY (widget);
  editable = BTK_EDITABLE (entry);
  str = btk_editable_get_chars (editable, start_pos, end_pos);
  clipboard = btk_clipboard_get_for_display (btk_widget_get_display (widget),
                                             BDK_SELECTION_CLIPBOARD);
  btk_clipboard_set_text (clipboard, str, -1);
}

static void
bail_entry_cut_text (BatkEditableText *text,
                     gint            start_pos,
                     gint            end_pos)
{
  BtkEntry *entry;
  BtkWidget *widget;
  BtkEditable *editable;
  gchar *str;
  BtkClipboard *clipboard;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return;

  entry = BTK_ENTRY (widget);
  editable = BTK_EDITABLE (entry);
  if (!btk_editable_get_editable (editable))
    return;
  str = btk_editable_get_chars (editable, start_pos, end_pos);
  clipboard = btk_clipboard_get_for_display (btk_widget_get_display (widget),
                                             BDK_SELECTION_CLIPBOARD);
  btk_clipboard_set_text (clipboard, str, -1);
  btk_editable_delete_text (editable, start_pos, end_pos);
}

static void
bail_entry_delete_text (BatkEditableText *text,
                        gint            start_pos,
                        gint            end_pos)
{
  BtkEntry *entry;
  BtkWidget *widget;
  BtkEditable *editable;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return;

  entry = BTK_ENTRY (widget);
  editable = BTK_EDITABLE (entry);
  if (!btk_editable_get_editable (editable))
    return;

  btk_editable_delete_text (editable, start_pos, end_pos);
}

static void
bail_entry_paste_text (BatkEditableText *text,
                       gint            position)
{
  BtkWidget *widget;
  BtkEditable *editable;
  BailEntryPaste paste_struct;
  BtkClipboard *clipboard;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return;

  editable = BTK_EDITABLE (widget);
  if (!btk_editable_get_editable (editable))
    return;
  paste_struct.entry = BTK_ENTRY (widget);
  paste_struct.position = position;

  g_object_ref (paste_struct.entry);
  clipboard = btk_clipboard_get_for_display (btk_widget_get_display (widget),
                                             BDK_SELECTION_CLIPBOARD);
  btk_clipboard_request_text (clipboard,
    bail_entry_paste_received, &paste_struct);
}

static void
bail_entry_paste_received (BtkClipboard *clipboard,
		const gchar  *text,
		gpointer     data)
{
  BailEntryPaste* paste_struct = (BailEntryPaste *)data;

  if (text)
    btk_editable_insert_text (BTK_EDITABLE (paste_struct->entry), text, -1,
       &(paste_struct->position));

  g_object_unref (paste_struct->entry);
}

/* Callbacks */

static gboolean
bail_entry_idle_notify_insert (gpointer data)
{
  BailEntry *entry;

  entry = BAIL_ENTRY (data);
  entry->insert_idle_handler = 0;
  bail_entry_notify_insert (entry);

  return FALSE;
}

static void
bail_entry_notify_insert (BailEntry *entry)
{
  if (entry->signal_name_insert)
    {
      g_signal_emit_by_name (entry, 
                             entry->signal_name_insert,
                             entry->position_insert,
                             entry->length_insert);
      entry->signal_name_insert = NULL;
    }
}

/* Note arg1 returns the character at the start of the insert.
 * arg2 returns the number of characters inserted.
 */
static void 
_bail_entry_insert_text_cb (BtkEntry *entry, 
                            gchar    *arg1, 
                            gint     arg2,
                            gpointer arg3)
{
  BatkObject *accessible;
  BailEntry *bail_entry;
  gint *position = (gint *) arg3;

  accessible = btk_widget_get_accessible (BTK_WIDGET (entry));
  bail_entry = BAIL_ENTRY (accessible);
  if (!bail_entry->signal_name_insert)
    {
      bail_entry->signal_name_insert = "text_changed::insert";
      bail_entry->position_insert = *position;
      bail_entry->length_insert = g_utf8_strlen(arg1, arg2);
    }
  /*
   * The signal will be emitted when the cursor position is updated.
   * or in an idle handler if it not updated.
   */
   if (bail_entry->insert_idle_handler == 0)
     bail_entry->insert_idle_handler = bdk_threads_add_idle (bail_entry_idle_notify_insert, bail_entry);
}

static gunichar 
bail_entry_get_character_at_offset (BatkText *text,
                                    gint     offset)
{
  BtkWidget *widget;
  BailEntry *entry;
  gchar *string;
  gchar *index;
  gunichar unichar;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return '\0';

  entry = BAIL_ENTRY (text);
  string = bail_text_util_get_substring (entry->textutil, 0, -1);
  if (offset >= g_utf8_strlen (string, -1))
    {
      unichar = '\0';
    }
  else
    {
      index = g_utf8_offset_to_pointer (string, offset);

      unichar = g_utf8_get_char(index);
    }

  g_free(string);
  return unichar;
}

static void
bail_entry_notify_delete (BailEntry *entry)
{
  if (entry->signal_name_delete)
    {
      g_signal_emit_by_name (entry, 
                             entry->signal_name_delete,
                             entry->position_delete,
                             entry->length_delete);
      entry->signal_name_delete = NULL;
    }
}

/* Note arg1 returns the start of the delete range, arg2 returns the
 * end of the delete range if multiple characters are deleted.	
 */
static void 
_bail_entry_delete_text_cb (BtkEntry *entry, 
                            gint      arg1, 
                            gint      arg2)
{
  BatkObject *accessible;
  BailEntry *bail_entry;

  /*
   * Zero length text deleted so ignore
   */
  if (arg2 - arg1 == 0)
    return;

  accessible = btk_widget_get_accessible (BTK_WIDGET (entry));
  bail_entry = BAIL_ENTRY (accessible);
  if (!bail_entry->signal_name_delete)
    {
      bail_entry->signal_name_delete = "text_changed::delete";
      bail_entry->position_delete = arg1;
      bail_entry->length_delete = arg2 - arg1;
    }
  bail_entry_notify_delete (bail_entry);
}

static void
_bail_entry_changed_cb (BtkEntry *entry)
{
  BatkObject *accessible;
  BailEntry *bail_entry;

  accessible = btk_widget_get_accessible (BTK_WIDGET (entry));

  bail_entry = BAIL_ENTRY (accessible);

  text_setup (bail_entry, entry);
}

static gboolean 
check_for_selection_change (BailEntry   *entry,
                            BtkEntry    *btk_entry)
{
  gboolean ret_val = FALSE;
 
  if (btk_entry->current_pos != btk_entry->selection_bound)
    {
      if (btk_entry->current_pos != entry->cursor_position ||
          btk_entry->selection_bound != entry->selection_bound)
        /*
         * This check is here as this function can be called
         * for notification of selection_bound and current_pos.
         * The values of current_pos and selection_bound may be the same 
         * for both notifications and we only want to generate one
         * text_selection_changed signal.
         */
        ret_val = TRUE;
    }
  else 
    {
      /* We had a selection */
      ret_val = (entry->cursor_position != entry->selection_bound);
    }
  entry->cursor_position = btk_entry->current_pos;
  entry->selection_bound = btk_entry->selection_bound;

  return ret_val;
}

static void
batk_action_interface_init (BatkActionIface *iface)
{
  iface->do_action = bail_entry_do_action;
  iface->get_n_actions = bail_entry_get_n_actions;
  iface->get_description = bail_entry_get_description;
  iface->get_keybinding = bail_entry_get_keybinding;
  iface->get_name = bail_entry_action_get_name;
  iface->set_description = bail_entry_set_description;
}

static gboolean
bail_entry_do_action (BatkAction *action,
                      gint      i)
{
  BailEntry *entry;
  BtkWidget *widget;
  gboolean return_value = TRUE;

  entry = BAIL_ENTRY (action);
  widget = BTK_ACCESSIBLE (action)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return FALSE;

  if (!btk_widget_get_sensitive (widget) || !btk_widget_get_visible (widget))
    return FALSE;

  switch (i)
    {
    case 0:
      if (entry->action_idle_handler)
        return_value = FALSE;
      else
        entry->action_idle_handler = bdk_threads_add_idle (idle_do_action, entry);
      break;
    default:
      return_value = FALSE;
      break;
    }
  return return_value; 
}

static gboolean
idle_do_action (gpointer data)
{
  BailEntry *entry;
  BtkWidget *widget;

  entry = BAIL_ENTRY (data);
  entry->action_idle_handler = 0;
  widget = BTK_ACCESSIBLE (entry)->widget;
  if (widget == NULL /* State is defunct */ ||
      !btk_widget_get_sensitive (widget) || !btk_widget_get_visible (widget))
    return FALSE;

  btk_widget_activate (widget);

  return FALSE;
}

static gint
bail_entry_get_n_actions (BatkAction *action)
{
  return 1;
}

static const gchar*
bail_entry_get_description (BatkAction *action,
                            gint      i)
{
  BailEntry *entry;
  const gchar *return_value;

  entry = BAIL_ENTRY (action);
  switch (i)
    {
    case 0:
      return_value = entry->activate_description;
      break;
    default:
      return_value = NULL;
      break;
    }
  return return_value; 
}

static const gchar*
bail_entry_get_keybinding (BatkAction *action,
                           gint      i)
{
  BailEntry *entry;
  gchar *return_value = NULL;

  entry = BAIL_ENTRY (action);
  switch (i)
    {
    case 0:
      {
        /*
         * We look for a mnemonic on the label
         */
        BtkWidget *widget;
        BtkWidget *label;
        BatkRelationSet *set;
        BatkRelation *relation;
        GPtrArray *target;
        gpointer target_object;
        guint key_val; 

        entry = BAIL_ENTRY (action);
        widget = BTK_ACCESSIBLE (entry)->widget;
        if (widget == NULL)
          /*
           * State is defunct
           */
          return NULL;

        /* Find labelled-by relation */

        set = batk_object_ref_relation_set (BATK_OBJECT (action));
        if (!set)
          return NULL;
        label = NULL;
        relation = batk_relation_set_get_relation_by_type (set, BATK_RELATION_LABELLED_BY);
        if (relation)
          {              
            target = batk_relation_get_target (relation);
          
            target_object = g_ptr_array_index (target, 0);
            if (BTK_IS_ACCESSIBLE (target_object))
              {
                label = BTK_ACCESSIBLE (target_object)->widget;
              } 
          }

        g_object_unref (set);

        if (BTK_IS_LABEL (label))
          {
            key_val = btk_label_get_mnemonic_keyval (BTK_LABEL (label)); 
            if (key_val != BDK_VoidSymbol)
              return_value = btk_accelerator_name (key_val, BDK_MOD1_MASK);
          }
        g_free (entry->activate_keybinding);
        entry->activate_keybinding = return_value;
        break;
      }
    default:
      break;
    }
  return return_value; 
}

static const gchar*
bail_entry_action_get_name (BatkAction *action,
                            gint      i)
{
  const gchar *return_value;

  switch (i)
    {
    case 0:
      return_value = "activate";
      break;
    default:
      return_value = NULL;
      break;
  }
  return return_value; 
}

static gboolean
bail_entry_set_description (BatkAction      *action,
                            gint           i,
                            const gchar    *desc)
{
  BailEntry *entry;
  gchar **value;

  entry = BAIL_ENTRY (action);
  switch (i)
    {
    case 0:
      value = &entry->activate_description;
      break;
    default:
      value = NULL;
      break;
    }

  if (value)
    {
      g_free (*value);
      *value = g_strdup (desc);
      return TRUE;
    }
  else
    return FALSE;
}
