/* btkcombo - combo widget for btk+
 * Copyright 1997 Paolo Molaro
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

/* Do NOT, I repeat, NOT, copy any of the code in this file.
 * The code here relies on all sorts of internal details of BTK+
 */

#undef BTK_DISABLE_DEPRECATED
/* For GCompletion */
#undef G_DISABLE_DEPRECATED

#include "config.h"
#include <string.h>

#include <bdk/bdkkeysyms.h>

#include "btkarrow.h"
#include "btklabel.h"
#include "btklist.h"
#include "btkentry.h"
#include "btkeventbox.h"
#include "btkbutton.h"
#include "btklistitem.h"
#include "btkscrolledwindow.h"
#include "btkmain.h"
#include "btkwindow.h"
#include "btkcombo.h"
#include "btkframe.h"
#include "btkprivate.h"
#include "btkintl.h"

#include "btkalias.h"

static const bchar btk_combo_string_key[] = "btk-combo-string-value";

#define COMBO_LIST_MAX_HEIGHT	(400)
#define	EMPTY_LIST_HEIGHT	(15)

enum {
  PROP_0,
  PROP_ENABLE_ARROW_KEYS,
  PROP_ENABLE_ARROWS_ALWAYS,
  PROP_CASE_SENSITIVE,
  PROP_ALLOW_EMPTY,
  PROP_VALUE_IN_LIST
};

static void         btk_combo_realize		 (BtkWidget	   *widget);
static void         btk_combo_unrealize		 (BtkWidget	   *widget);
static void         btk_combo_destroy            (BtkObject        *combo);
static BtkListItem *btk_combo_find               (BtkCombo         *combo);
static bchar *      btk_combo_func               (BtkListItem      *li);
static bboolean     btk_combo_focus_idle         (BtkCombo         *combo);
static bint         btk_combo_entry_focus_out    (BtkEntry         *entry,
						  BdkEventFocus    *event,
						  BtkCombo         *combo);
static void         btk_combo_get_pos            (BtkCombo         *combo,
						  bint             *x,
						  bint             *y,
						  bint             *height,
						  bint             *width);
static void         btk_combo_popup_list         (BtkCombo         *combo);
static void         btk_combo_popdown_list       (BtkCombo         *combo);

static void         btk_combo_activate           (BtkWidget        *widget,
						  BtkCombo         *combo);
static bboolean     btk_combo_popup_button_press (BtkWidget        *button,
						  BdkEventButton   *event,
						  BtkCombo         *combo);
static bboolean     btk_combo_popup_button_leave (BtkWidget        *button,
						  BdkEventCrossing *event,
						  BtkCombo         *combo);
static void         btk_combo_update_entry       (BtkCombo         *combo);
static void         btk_combo_update_list        (BtkEntry         *entry,
						  BtkCombo         *combo);
static bint         btk_combo_button_press       (BtkWidget        *widget,
						  BdkEvent         *event,
						  BtkCombo         *combo);
static void         btk_combo_button_event_after (BtkWidget        *widget,
						  BdkEvent         *event,
						  BtkCombo         *combo);
static bint         btk_combo_list_enter         (BtkWidget        *widget,
						  BdkEventCrossing *event,
						  BtkCombo         *combo);
static bint         btk_combo_list_key_press     (BtkWidget        *widget,
						  BdkEventKey      *event,
						  BtkCombo         *combo);
static bint         btk_combo_entry_key_press    (BtkEntry         *widget,
						  BdkEventKey      *event,
						  BtkCombo         *combo);
static bint         btk_combo_window_key_press   (BtkWidget        *window,
						  BdkEventKey      *event,
						  BtkCombo         *combo);
static void         btk_combo_size_allocate      (BtkWidget        *widget,
						  BtkAllocation   *allocation);
static void         btk_combo_set_property       (BObject         *object,
						  buint            prop_id,
						  const BValue    *value,
						  BParamSpec      *pspec);
static void         btk_combo_get_property       (BObject         *object,
						  buint            prop_id,
						  BValue          *value,
						  BParamSpec      *pspec);

G_DEFINE_TYPE (BtkCombo, btk_combo, BTK_TYPE_HBOX)

static void
btk_combo_class_init (BtkComboClass * klass)
{
  BObjectClass *bobject_class;
  BtkObjectClass *oclass;
  BtkWidgetClass *widget_class;

  bobject_class = (BObjectClass *) klass;
  oclass = (BtkObjectClass *) klass;
  widget_class = (BtkWidgetClass *) klass;

  bobject_class->set_property = btk_combo_set_property; 
  bobject_class->get_property = btk_combo_get_property; 

  g_object_class_install_property (bobject_class,
                                   PROP_ENABLE_ARROW_KEYS,
                                   g_param_spec_boolean ("enable-arrow-keys",
                                                         P_("Enable arrow keys"),
                                                         P_("Whether the arrow keys move through the list of items"),
                                                         TRUE,
                                                         BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
                                   PROP_ENABLE_ARROWS_ALWAYS,
                                   g_param_spec_boolean ("enable-arrows-always",
                                                         P_("Always enable arrows"),
                                                         P_("Obsolete property, ignored"),
                                                         TRUE,
                                                         BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
                                   PROP_CASE_SENSITIVE,
                                   g_param_spec_boolean ("case-sensitive",
                                                         P_("Case sensitive"),
                                                         P_("Whether list item matching is case sensitive"),
                                                         FALSE,
                                                         BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_ALLOW_EMPTY,
                                   g_param_spec_boolean ("allow-empty",
                                                         P_("Allow empty"),
							 P_("Whether an empty value may be entered in this field"),
                                                         TRUE,
                                                         BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_VALUE_IN_LIST,
                                   g_param_spec_boolean ("value-in-list",
                                                         P_("Value in list"),
                                                         P_("Whether entered values must already be present in the list"),
                                                         FALSE,
                                                         BTK_PARAM_READWRITE));
  
   
  oclass->destroy = btk_combo_destroy;
  
  widget_class->size_allocate = btk_combo_size_allocate;
  widget_class->realize = btk_combo_realize;
  widget_class->unrealize = btk_combo_unrealize;
}

static void
btk_combo_destroy (BtkObject *object)
{
  BtkCombo *combo = BTK_COMBO (object);

  if (combo->popwin)
    {
      btk_widget_destroy (combo->popwin);
      g_object_unref (combo->popwin);
      combo->popwin = NULL;
    }

  BTK_OBJECT_CLASS (btk_combo_parent_class)->destroy (object);
}

static int
btk_combo_entry_key_press (BtkEntry * entry, BdkEventKey * event, BtkCombo * combo)
{
  GList *li;
  buint state = event->state & btk_accelerator_get_default_mod_mask ();

  /* completion */
  if ((event->keyval == BDK_Tab ||  event->keyval == BDK_KP_Tab) &&
      state == BDK_MOD1_MASK)
    {
      BtkEditable *editable = BTK_EDITABLE (entry);
      GCompletion * cmpl;
      bchar* prefix;
      bchar* nprefix = NULL;
      bint pos;

      if ( !BTK_LIST (combo->list)->children )
	return FALSE;
    
      cmpl = g_completion_new ((GCompletionFunc)btk_combo_func);
      g_completion_add_items (cmpl, BTK_LIST (combo->list)->children);

      pos = btk_editable_get_position (editable);
      prefix = btk_editable_get_chars (editable, 0, pos);

      g_completion_complete_utf8 (cmpl, prefix, &nprefix);

      if (nprefix && strlen (nprefix) > strlen (prefix)) 
	{
	  btk_editable_insert_text (editable, g_utf8_offset_to_pointer (nprefix, pos), 
				    strlen (nprefix) - strlen (prefix), &pos);
	  btk_editable_set_position (editable, pos);
	}

      g_free (nprefix);
      g_free (prefix);
      g_completion_free (cmpl);

      return TRUE;
    }

  if ((event->keyval == BDK_Down || event->keyval == BDK_KP_Down) &&
      state == BDK_MOD1_MASK)
    {
      btk_combo_activate (NULL, combo);
      return TRUE;
    }

  if (!combo->use_arrows || !BTK_LIST (combo->list)->children)
    return FALSE;

  btk_combo_update_list (BTK_ENTRY (combo->entry), combo);
  li = g_list_find (BTK_LIST (combo->list)->children, btk_combo_find (combo));

  if (((event->keyval == BDK_Up || event->keyval == BDK_KP_Up) && state == 0) ||
      ((event->keyval == 'p' || event->keyval == 'P') && state == BDK_MOD1_MASK))
    {
      if (!li)
	li = g_list_last (BTK_LIST (combo->list)->children);
      else
	li = li->prev;

      if (li)
	{
	  btk_list_select_child (BTK_LIST (combo->list), BTK_WIDGET (li->data));
	  btk_combo_update_entry (combo);
	}
      
      return TRUE;
    }
  if (((event->keyval == BDK_Down || event->keyval == BDK_KP_Down) && state == 0) ||
      ((event->keyval == 'n' || event->keyval == 'N') && state == BDK_MOD1_MASK))
    {
      if (!li)
	li = BTK_LIST (combo->list)->children;
      else if (li)
	li = li->next;
      if (li)
	{
	  btk_list_select_child (BTK_LIST (combo->list), BTK_WIDGET (li->data));
	  btk_combo_update_entry (combo);
	}
      
      return TRUE;
    }
  return FALSE;
}

static int
btk_combo_window_key_press (BtkWidget   *window,
			    BdkEventKey *event,
			    BtkCombo    *combo)
{
  buint state = event->state & btk_accelerator_get_default_mod_mask ();

  if ((event->keyval == BDK_Return ||
       event->keyval == BDK_ISO_Enter ||
       event->keyval == BDK_KP_Enter) &&
      state == 0)
    {
      btk_combo_popdown_list (combo);
      btk_combo_update_entry (combo);

      return TRUE;
    }
  else if ((event->keyval == BDK_Up || event->keyval == BDK_KP_Up) &&
	   state == BDK_MOD1_MASK)
    {
      btk_combo_popdown_list (combo);

      return TRUE;
    }
  else if ((event->keyval == BDK_space || event->keyval == BDK_KP_Space) &&
	   state == 0)
    {
      btk_combo_update_entry (combo);
    }

  return FALSE;
}

static BtkListItem *
btk_combo_find (BtkCombo * combo)
{
  const bchar *text;
  BtkListItem *found = NULL;
  bchar *ltext;
  bchar *compare_text;
  GList *clist;

  text = btk_entry_get_text (BTK_ENTRY (combo->entry));
  if (combo->case_sensitive)
    compare_text = (bchar *)text;
  else
    compare_text = g_utf8_casefold (text, -1);
  
  for (clist = BTK_LIST (combo->list)->children;
       !found && clist;
       clist = clist->next)
    {
      ltext = btk_combo_func (BTK_LIST_ITEM (clist->data));
      if (!ltext)
	continue;

      if (!combo->case_sensitive)
	ltext = g_utf8_casefold (ltext, -1);

      if (strcmp (ltext, compare_text) == 0)
	found = clist->data;

      if (!combo->case_sensitive)
	g_free (ltext);
    }

  if (!combo->case_sensitive)
    g_free (compare_text);

  return found;
}

static bchar *
btk_combo_func (BtkListItem * li)
{
  BtkWidget *label;
  bchar *ltext = NULL;

  ltext = g_object_get_data (B_OBJECT (li), I_(btk_combo_string_key));
  if (!ltext)
    {
      label = BTK_BIN (li)->child;
      if (!label || !BTK_IS_LABEL (label))
	return NULL;
      ltext = (bchar *) btk_label_get_text (BTK_LABEL (label));
    }
  return ltext;
}

static bint
btk_combo_focus_idle (BtkCombo * combo)
{
  if (combo)
    {
      BDK_THREADS_ENTER ();
      btk_widget_grab_focus (combo->entry);
      BDK_THREADS_LEAVE ();
    }
  return FALSE;
}

static bint
btk_combo_entry_focus_out (BtkEntry * entry, BdkEventFocus * event, BtkCombo * combo)
{

  if (combo->value_in_list && !btk_combo_find (combo))
    {
      GSource *focus_idle;
      
      /* bdk_beep(); *//* this can be annoying */
      if (combo->ok_if_empty && !strcmp (btk_entry_get_text (entry), ""))
	return FALSE;
#ifdef TEST
      printf ("INVALID ENTRY: `%s'\n", btk_entry_get_text (entry));
#endif
      btk_grab_add (BTK_WIDGET (combo));
      /* this is needed because if we call btk_widget_grab_focus() 
         it isn't guaranteed it's the *last* call before the main-loop,
         so the focus can be lost anyway...
         the signal_stop_emission doesn't seem to work either...
       */
      focus_idle = g_idle_source_new ();
      g_source_set_closure (focus_idle,
			    g_cclosure_new_object (G_CALLBACK (btk_combo_focus_idle),
						   B_OBJECT (combo)));
      g_source_attach (focus_idle, NULL);
	g_source_unref (focus_idle);
      
      /*g_signal_stop_emission_by_name (entry, "focus_out_event"); */
      return TRUE;
    }
  return FALSE;
}

static void
btk_combo_get_pos (BtkCombo * combo, bint * x, bint * y, bint * height, bint * width)
{
  BtkBin *popwin;
  BtkWidget *widget;
  BtkScrolledWindow *popup;
  
  bint real_height;
  BtkRequisition list_requisition;
  bboolean show_hscroll = FALSE;
  bboolean show_vscroll = FALSE;
  bint avail_height;
  bint min_height;
  bint alloc_width;
  bint work_height;
  bint old_height;
  bint old_width;
  bint scrollbar_spacing;
  
  widget = BTK_WIDGET (combo);
  popup  = BTK_SCROLLED_WINDOW (combo->popup);
  popwin = BTK_BIN (combo->popwin);

  scrollbar_spacing = _btk_scrolled_window_get_scrollbar_spacing (popup);

  bdk_window_get_origin (combo->entry->window, x, y);
  if (btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL) 
    *x -= widget->allocation.width - combo->entry->allocation.width;
  real_height = MIN (combo->entry->requisition.height, 
		     combo->entry->allocation.height);
  *y += real_height;
  avail_height = bdk_screen_get_height (btk_widget_get_screen (widget)) - *y;
  
  btk_widget_size_request (combo->list, &list_requisition);
  min_height = MIN (list_requisition.height, 
		    popup->vscrollbar->requisition.height);
  if (!BTK_LIST (combo->list)->children)
    list_requisition.height += EMPTY_LIST_HEIGHT;
  
  alloc_width = (widget->allocation.width -
		 2 * popwin->child->style->xthickness -
		 2 * BTK_CONTAINER (popwin->child)->border_width -
		 2 * BTK_CONTAINER (combo->popup)->border_width -
		 2 * BTK_CONTAINER (BTK_BIN (popup)->child)->border_width - 
		 2 * BTK_BIN (popup)->child->style->xthickness);
  
  work_height = (2 * popwin->child->style->ythickness +
		 2 * BTK_CONTAINER (popwin->child)->border_width +
		 2 * BTK_CONTAINER (combo->popup)->border_width +
		 2 * BTK_CONTAINER (BTK_BIN (popup)->child)->border_width +
		 2 * BTK_BIN (popup)->child->style->ythickness);
  
  do 
    {
      old_width = alloc_width;
      old_height = work_height;
      
      if (!show_hscroll &&
	  alloc_width < list_requisition.width)
	{
	  BtkRequisition requisition;
	  
	  btk_widget_size_request (popup->hscrollbar, &requisition);
	  work_height += (requisition.height + scrollbar_spacing);
	  
	  show_hscroll = TRUE;
	}
      if (!show_vscroll && 
	  work_height + list_requisition.height > avail_height)
	{
	  BtkRequisition requisition;
	  
	  if (work_height + min_height > avail_height && 
	      *y - real_height > avail_height)
	    {
	      *y -= (work_height + list_requisition.height + real_height);
	      break;
	    }
	  btk_widget_size_request (popup->hscrollbar, &requisition);
	  alloc_width -= (requisition.width + scrollbar_spacing);
	  show_vscroll = TRUE;
	}
    } while (old_width != alloc_width || old_height != work_height);
  
  *width = widget->allocation.width;
  if (show_vscroll)
    *height = avail_height;
  else
    *height = work_height + list_requisition.height;
  
  if (*x < 0)
    *x = 0;
}

static void
btk_combo_popup_list (BtkCombo *combo)
{
  BtkWidget *toplevel;
  BtkList *list;
  bint height, width, x, y;
  bint old_width, old_height;

  old_width = combo->popwin->allocation.width;
  old_height  = combo->popwin->allocation.height;

  btk_combo_get_pos (combo, &x, &y, &height, &width);

  /* workaround for btk_scrolled_window_size_allocate bug */
  if (old_width != width || old_height != height)
    {
      btk_widget_hide (BTK_SCROLLED_WINDOW (combo->popup)->hscrollbar);
      btk_widget_hide (BTK_SCROLLED_WINDOW (combo->popup)->vscrollbar);
    }

  btk_combo_update_list (BTK_ENTRY (combo->entry), combo);

  /* We need to make sure some child of combo->popwin
   * is focused to disable BtkWindow's automatic
   * "focus-the-first-item" code. If there is no selected
   * child, we focus the list itself with some hackery.
   */
  list = BTK_LIST (combo->list);
  
  if (list->selection)
    {
      btk_widget_grab_focus (list->selection->data);
    }
  else
    {
      btk_widget_set_can_focus (BTK_WIDGET (list), TRUE);
      btk_widget_grab_focus (combo->list);
      BTK_LIST (combo->list)->last_focus_child = NULL;
      btk_widget_set_can_focus (BTK_WIDGET (list), FALSE);
    }
  
  btk_window_move (BTK_WINDOW (combo->popwin), x, y);

  toplevel = btk_widget_get_toplevel (BTK_WIDGET (combo));

  if (BTK_IS_WINDOW (toplevel))
    {
      btk_window_group_add_window (btk_window_get_group (BTK_WINDOW (toplevel)), 
                                   BTK_WINDOW (combo->popwin));
      btk_window_set_transient_for (BTK_WINDOW (combo->popwin), BTK_WINDOW (toplevel));
    }

  btk_widget_set_size_request (combo->popwin, width, height);
  btk_widget_show (combo->popwin);

  btk_widget_grab_focus (combo->popwin);
}

static void
btk_combo_popdown_list (BtkCombo *combo)
{
  combo->current_button = 0;
      
  if (BTK_BUTTON (combo->button)->in_button)
    {
      BTK_BUTTON (combo->button)->in_button = FALSE;
      g_signal_emit_by_name (combo->button, "released");
    }

  if (BTK_WIDGET_HAS_GRAB (combo->popwin))
    {
      btk_grab_remove (combo->popwin);
      bdk_display_pointer_ungrab (btk_widget_get_display (BTK_WIDGET (combo)),
				  btk_get_current_event_time ());
      bdk_display_keyboard_ungrab (btk_widget_get_display (BTK_WIDGET (combo)),
				   btk_get_current_event_time ());
    }
  
  btk_widget_hide (combo->popwin);

  btk_window_group_add_window (btk_window_get_group (NULL), BTK_WINDOW (combo->popwin));
}

static bboolean
popup_grab_on_window (BdkWindow *window,
		      buint32    activate_time)
{
  if ((bdk_pointer_grab (window, TRUE,
			 BDK_BUTTON_PRESS_MASK | BDK_BUTTON_RELEASE_MASK |
			 BDK_POINTER_MOTION_MASK,
			 NULL, NULL, activate_time) == 0))
    {
      if (bdk_keyboard_grab (window, TRUE,
			     activate_time) == 0)
	return TRUE;
      else
	{
	  bdk_display_pointer_ungrab (bdk_window_get_display (window),
				      activate_time);
	  return FALSE;
	}
    }

  return FALSE;
}

static void        
btk_combo_activate (BtkWidget        *widget,
		    BtkCombo         *combo)
{
  if (!combo->button->window ||
      !popup_grab_on_window (combo->button->window,
			     btk_get_current_event_time ()))
    return;

  btk_combo_popup_list (combo);
  
  /* This must succeed since we already have the grab */
  popup_grab_on_window (combo->popwin->window,
			btk_get_current_event_time ());

  if (!btk_widget_has_focus (combo->entry))
    btk_widget_grab_focus (combo->entry);

  btk_grab_add (combo->popwin);
}

static bboolean
btk_combo_popup_button_press (BtkWidget        *button,
			      BdkEventButton   *event,
			      BtkCombo         *combo)
{
  if (!btk_widget_has_focus (combo->entry))
    btk_widget_grab_focus (combo->entry);

  if (event->button != 1)
    return FALSE;

  if (!popup_grab_on_window (combo->button->window,
			     btk_get_current_event_time ()))
    return FALSE;

  combo->current_button = event->button;

  btk_combo_popup_list (combo);

  /* This must succeed since we already have the grab */
  popup_grab_on_window (combo->popwin->window,
			btk_get_current_event_time ());

  g_signal_emit_by_name (button, "pressed");

  btk_grab_add (combo->popwin);

  return TRUE;
}

static bboolean
btk_combo_popup_button_leave (BtkWidget        *button,
			      BdkEventCrossing *event,
			      BtkCombo         *combo)
{
  /* The idea here is that we want to keep the button down if the
   * popup is popped up.
   */
  return combo->current_button != 0;
}

static void
btk_combo_update_entry (BtkCombo * combo)
{
  BtkList *list = BTK_LIST (combo->list);
  char *text;

  g_signal_handler_block (list, combo->list_change_id);
  if (list->selection)
    {
      text = btk_combo_func (BTK_LIST_ITEM (list->selection->data));
      if (!text)
	text = "";
      btk_entry_set_text (BTK_ENTRY (combo->entry), text);
    }
  g_signal_handler_unblock (list, combo->list_change_id);
}

static void
btk_combo_selection_changed (BtkList  *list,
			     BtkCombo *combo)
{
  if (!btk_widget_get_visible (combo->popwin))
    btk_combo_update_entry (combo);
}

static void
btk_combo_update_list (BtkEntry * entry, BtkCombo * combo)
{
  BtkList *list = BTK_LIST (combo->list);
  GList *slist = list->selection;
  BtkListItem *li;

  btk_grab_remove (BTK_WIDGET (combo));

  g_signal_handler_block (entry, combo->entry_change_id);
  if (slist && slist->data)
    btk_list_unselect_child (list, BTK_WIDGET (slist->data));
  li = btk_combo_find (combo);
  if (li)
    btk_list_select_child (list, BTK_WIDGET (li));
  g_signal_handler_unblock (entry, combo->entry_change_id);
}

static bint
btk_combo_button_press (BtkWidget * widget, BdkEvent * event, BtkCombo * combo)
{
  BtkWidget *child;

  child = btk_get_event_widget (event);

  /* We don't ask for button press events on the grab widget, so
   *  if an event is reported directly to the grab widget, it must
   *  be on a window outside the application (and thus we remove
   *  the popup window). Otherwise, we check if the widget is a child
   *  of the grab widget, and only remove the popup window if it
   *  is not.
   */
  if (child != widget)
    {
      while (child)
	{
	  if (child == widget)
	    return FALSE;
	  child = child->parent;
	}
    }

  btk_combo_popdown_list (combo);

  return TRUE;
}

static bboolean
is_within (BtkWidget *widget,
	   BtkWidget *ancestor)
{
  return widget == ancestor || btk_widget_is_ancestor (widget, ancestor);
}

static void
btk_combo_button_event_after (BtkWidget *widget,
			      BdkEvent  *event,
			      BtkCombo  *combo)
{
  BtkWidget *child;

  if (event->type != BDK_BUTTON_RELEASE)
    return;
  
  child = btk_get_event_widget ((BdkEvent*) event);

  if ((combo->current_button != 0) && (event->button.button == 1))
    {
      /* This was the initial button press */

      combo->current_button = 0;

      /* Check to see if we released inside the button */
      if (child && is_within (child, combo->button))
	{
	  btk_grab_add (combo->popwin);
	  bdk_pointer_grab (combo->popwin->window, TRUE,
			    BDK_BUTTON_PRESS_MASK | 
			    BDK_BUTTON_RELEASE_MASK |
			    BDK_POINTER_MOTION_MASK, 
			    NULL, NULL, BDK_CURRENT_TIME);
	  return;
	}
    }

  if (is_within (child, combo->list))
    btk_combo_update_entry (combo);
    
  btk_combo_popdown_list (combo);

}

static void
find_child_foreach (BtkWidget *widget,
		    bpointer   data)
{
  BdkEventButton *event = data;

  if (!event->window)
    {
      if (event->x >= widget->allocation.x &&
	  event->x < widget->allocation.x + widget->allocation.width &&
	  event->y >= widget->allocation.y &&
	  event->y < widget->allocation.y + widget->allocation.height)
	event->window = g_object_ref (widget->window);
    }
}

static void
find_child_window (BtkContainer   *container,
		   BdkEventButton *event)
{
  btk_container_foreach (container, find_child_foreach, event);
}

static bint         
btk_combo_list_enter (BtkWidget        *widget,
		      BdkEventCrossing *event,
		      BtkCombo         *combo)
{
  BtkWidget *event_widget;

  event_widget = btk_get_event_widget ((BdkEvent*) event);

  if ((event_widget == combo->list) &&
      (combo->current_button != 0) && 
      (!BTK_WIDGET_HAS_GRAB (combo->list)))
    {
      BdkEvent *tmp_event = bdk_event_new (BDK_BUTTON_PRESS);
      bint x, y;
      BdkModifierType mask;

      btk_grab_remove (combo->popwin);

      /* Transfer the grab over to the list by synthesizing
       * a button press event
       */
      bdk_window_get_pointer (combo->list->window, &x, &y, &mask);

      tmp_event->button.send_event = TRUE;
      tmp_event->button.time = BDK_CURRENT_TIME; /* bad */
      tmp_event->button.x = x;
      tmp_event->button.y = y;
      /* We leave all the XInput fields unfilled here, in the expectation
       * that BtkList doesn't care.
       */
      tmp_event->button.button = combo->current_button;
      tmp_event->button.state = mask;

      find_child_window (BTK_CONTAINER (combo->list), &tmp_event->button);
      if (!tmp_event->button.window)
	{
	  BtkWidget *child;
	  
	  if (BTK_LIST (combo->list)->children)
	    child = BTK_LIST (combo->list)->children->data;
	  else
	    child = combo->list;

	  tmp_event->button.window = g_object_ref (child->window);
	}

      btk_widget_event (combo->list, tmp_event);
      bdk_event_free (tmp_event);
    }

  return FALSE;
}

static int
btk_combo_list_key_press (BtkWidget * widget, BdkEventKey * event, BtkCombo * combo)
{
  buint state = event->state & btk_accelerator_get_default_mod_mask ();

  if (event->keyval == BDK_Escape && state == 0)
    {
      if (BTK_WIDGET_HAS_GRAB (combo->list))
	btk_list_end_drag_selection (BTK_LIST (combo->list));

      btk_combo_popdown_list (combo);
      
      return TRUE;
    }
  return FALSE;
}

static void
combo_event_box_realize (BtkWidget *widget)
{
  BdkCursor *cursor = bdk_cursor_new_for_display (btk_widget_get_display (widget),
						  BDK_TOP_LEFT_ARROW);
  bdk_window_set_cursor (widget->window, cursor);
  bdk_cursor_unref (cursor);
}

static void
btk_combo_init (BtkCombo * combo)
{
  BtkWidget *arrow;
  BtkWidget *frame;
  BtkWidget *event_box;

  combo->case_sensitive = FALSE;
  combo->value_in_list = FALSE;
  combo->ok_if_empty = TRUE;
  combo->use_arrows = TRUE;
  combo->use_arrows_always = TRUE;
  combo->entry = btk_entry_new ();
  combo->button = btk_button_new ();
  combo->current_button = 0;
  arrow = btk_arrow_new (BTK_ARROW_DOWN, BTK_SHADOW_OUT);
  btk_widget_show (arrow);
  btk_container_add (BTK_CONTAINER (combo->button), arrow);
  btk_box_pack_start (BTK_BOX (combo), combo->entry, TRUE, TRUE, 0);
  btk_box_pack_end (BTK_BOX (combo), combo->button, FALSE, FALSE, 0);
  btk_widget_set_can_focus (combo->button, FALSE);
  btk_widget_show (combo->entry);
  btk_widget_show (combo->button);
  combo->entry_change_id = g_signal_connect (combo->entry, "changed",
					     G_CALLBACK (btk_combo_update_list),
					     combo);
  g_signal_connect_after (combo->entry, "key-press-event",
			  G_CALLBACK (btk_combo_entry_key_press), combo);
  g_signal_connect_after (combo->entry, "focus-out-event",
			  G_CALLBACK (btk_combo_entry_focus_out), combo);
  combo->activate_id = g_signal_connect (combo->entry, "activate",
					 G_CALLBACK (btk_combo_activate),
					 combo);
  g_signal_connect (combo->button, "button-press-event",
		    G_CALLBACK (btk_combo_popup_button_press), combo);
  g_signal_connect (combo->button, "leave-notify-event",
		    G_CALLBACK (btk_combo_popup_button_leave), combo);

  combo->popwin = btk_window_new (BTK_WINDOW_POPUP);
  btk_widget_set_name (combo->popwin, "btk-combo-popup-window");
  btk_window_set_type_hint (BTK_WINDOW (combo->popwin), BDK_WINDOW_TYPE_HINT_COMBO);
  g_object_ref (combo->popwin);
  btk_window_set_resizable (BTK_WINDOW (combo->popwin), FALSE);

  g_signal_connect (combo->popwin, "key-press-event",
		    G_CALLBACK (btk_combo_window_key_press), combo);
  
  btk_widget_set_events (combo->popwin, BDK_KEY_PRESS_MASK);

  event_box = btk_event_box_new ();
  btk_container_add (BTK_CONTAINER (combo->popwin), event_box);
  g_signal_connect (event_box, "realize",
		    G_CALLBACK (combo_event_box_realize), NULL);
  btk_widget_show (event_box);


  frame = btk_frame_new (NULL);
  btk_container_add (BTK_CONTAINER (event_box), frame);
  btk_frame_set_shadow_type (BTK_FRAME (frame), BTK_SHADOW_OUT);
  btk_widget_show (frame);

  combo->popup = btk_scrolled_window_new (NULL, NULL);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (combo->popup),
				  BTK_POLICY_AUTOMATIC, BTK_POLICY_AUTOMATIC);
  btk_widget_set_can_focus (BTK_SCROLLED_WINDOW (combo->popup)->hscrollbar, FALSE);
  btk_widget_set_can_focus (BTK_SCROLLED_WINDOW (combo->popup)->vscrollbar, FALSE);
  btk_container_add (BTK_CONTAINER (frame), combo->popup);
  btk_widget_show (combo->popup);

  combo->list = btk_list_new ();
  /* We'll use enter notify events to figure out when to transfer
   * the grab to the list
   */
  btk_widget_set_events (combo->list, BDK_ENTER_NOTIFY_MASK);

  btk_list_set_selection_mode (BTK_LIST(combo->list), BTK_SELECTION_BROWSE);
  btk_scrolled_window_add_with_viewport (BTK_SCROLLED_WINDOW (combo->popup), combo->list);
  btk_container_set_focus_vadjustment (BTK_CONTAINER (combo->list),
				       btk_scrolled_window_get_vadjustment (BTK_SCROLLED_WINDOW (combo->popup)));
  btk_container_set_focus_hadjustment (BTK_CONTAINER (combo->list),
				       btk_scrolled_window_get_hadjustment (BTK_SCROLLED_WINDOW (combo->popup)));
  btk_widget_show (combo->list);

  combo->list_change_id = g_signal_connect (combo->list, "selection-changed",
					    G_CALLBACK (btk_combo_selection_changed), combo);
  
  g_signal_connect (combo->popwin, "key-press-event",
		    G_CALLBACK (btk_combo_list_key_press), combo);
  g_signal_connect (combo->popwin, "button-press-event",
		    G_CALLBACK (btk_combo_button_press), combo);

  g_signal_connect (combo->popwin, "event-after",
		    G_CALLBACK (btk_combo_button_event_after), combo);
  g_signal_connect (combo->list, "event-after",
		    G_CALLBACK (btk_combo_button_event_after), combo);

  g_signal_connect (combo->list, "enter-notify-event",
		    G_CALLBACK (btk_combo_list_enter), combo);
}

static void
btk_combo_realize (BtkWidget *widget)
{
  BtkCombo *combo = BTK_COMBO (widget);

  btk_window_set_screen (BTK_WINDOW (combo->popwin), 
			 btk_widget_get_screen (widget));
  
  BTK_WIDGET_CLASS (btk_combo_parent_class)->realize (widget);  
}

static void        
btk_combo_unrealize (BtkWidget *widget)
{
  BtkCombo *combo = BTK_COMBO (widget);

  btk_combo_popdown_list (combo);
  btk_widget_unrealize (combo->popwin);
  
  BTK_WIDGET_CLASS (btk_combo_parent_class)->unrealize (widget);
}

BtkWidget*
btk_combo_new (void)
{
  return g_object_new (BTK_TYPE_COMBO, NULL);
}

void
btk_combo_set_value_in_list (BtkCombo * combo, bboolean val, bboolean ok_if_empty)
{
  g_return_if_fail (BTK_IS_COMBO (combo));
  val = val != FALSE;
  ok_if_empty = ok_if_empty != FALSE;

  g_object_freeze_notify (B_OBJECT (combo));
  if (combo->value_in_list != val)
    {
       combo->value_in_list = val;
  g_object_notify (B_OBJECT (combo), "value-in-list");
    }
  if (combo->ok_if_empty != ok_if_empty)
    {
       combo->ok_if_empty = ok_if_empty;
  g_object_notify (B_OBJECT (combo), "allow-empty");
    }
  g_object_thaw_notify (B_OBJECT (combo));
}

void
btk_combo_set_case_sensitive (BtkCombo * combo, bboolean val)
{
  g_return_if_fail (BTK_IS_COMBO (combo));
  val = val != FALSE;

  if (combo->case_sensitive != val) 
    {
  combo->case_sensitive = val;
  g_object_notify (B_OBJECT (combo), "case-sensitive");
    }
}

void
btk_combo_set_use_arrows (BtkCombo * combo, bboolean val)
{
  g_return_if_fail (BTK_IS_COMBO (combo));
  val = val != FALSE;

  if (combo->use_arrows != val) 
    {
  combo->use_arrows = val;
  g_object_notify (B_OBJECT (combo), "enable-arrow-keys");
    }
}

void
btk_combo_set_use_arrows_always (BtkCombo * combo, bboolean val)
{
  g_return_if_fail (BTK_IS_COMBO (combo));
  val = val != FALSE;

  if (combo->use_arrows_always != val) 
    {
       g_object_freeze_notify (B_OBJECT (combo));
  combo->use_arrows_always = val;
       g_object_notify (B_OBJECT (combo), "enable-arrows-always");

       if (combo->use_arrows != TRUE) 
         {
  combo->use_arrows = TRUE;
  g_object_notify (B_OBJECT (combo), "enable-arrow-keys");
         }
  g_object_thaw_notify (B_OBJECT (combo));
    }
}

void
btk_combo_set_popdown_strings (BtkCombo *combo, 
			       GList    *strings)
{
  GList *list;
  BtkWidget *li;

  g_return_if_fail (BTK_IS_COMBO (combo));

  btk_combo_popdown_list (combo);

  btk_list_clear_items (BTK_LIST (combo->list), 0, -1);
  list = strings;
  while (list)
    {
      li = btk_list_item_new_with_label ((bchar *) list->data);
      btk_widget_show (li);
      btk_container_add (BTK_CONTAINER (combo->list), li);
      list = list->next;
    }
}

void
btk_combo_set_item_string (BtkCombo    *combo,
                           BtkItem     *item,
                           const bchar *item_value)
{
  g_return_if_fail (BTK_IS_COMBO (combo));
  g_return_if_fail (item != NULL);

  g_object_set_data_full (B_OBJECT (item), I_(btk_combo_string_key),
			  g_strdup (item_value), g_free);
}

static void
btk_combo_size_allocate (BtkWidget     *widget,
			 BtkAllocation *allocation)
{
  BtkCombo *combo = BTK_COMBO (widget);

  BTK_WIDGET_CLASS (btk_combo_parent_class)->size_allocate (widget, allocation);

  if (combo->entry->allocation.height > combo->entry->requisition.height)
    {
      BtkAllocation button_allocation;

      button_allocation = combo->button->allocation;
      button_allocation.height = combo->entry->requisition.height;
      button_allocation.y = combo->entry->allocation.y + 
	(combo->entry->allocation.height - combo->entry->requisition.height) 
	/ 2;
      btk_widget_size_allocate (combo->button, &button_allocation);
    }
}

void
btk_combo_disable_activate (BtkCombo *combo)
{
  g_return_if_fail (BTK_IS_COMBO (combo));

  if ( combo->activate_id ) {
    g_signal_handler_disconnect (combo->entry, combo->activate_id);
    combo->activate_id = 0;
  }
}

static void
btk_combo_set_property (BObject      *object,
			buint         prop_id,
			const BValue *value,
			BParamSpec   *pspec)
{
  BtkCombo *combo = BTK_COMBO (object);
  
  switch (prop_id)
    {
    case PROP_ENABLE_ARROW_KEYS:
      btk_combo_set_use_arrows (combo, b_value_get_boolean (value));
      break;
    case PROP_ENABLE_ARROWS_ALWAYS:
      btk_combo_set_use_arrows_always (combo, b_value_get_boolean (value));
      break;
    case PROP_CASE_SENSITIVE:
      btk_combo_set_case_sensitive (combo, b_value_get_boolean (value));
      break;
    case PROP_ALLOW_EMPTY:
      combo->ok_if_empty = b_value_get_boolean (value);
      break;
    case PROP_VALUE_IN_LIST:
      combo->value_in_list = b_value_get_boolean (value);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
  
}

static void
btk_combo_get_property (BObject    *object,
			buint       prop_id,
			BValue     *value,
			BParamSpec *pspec)
{
  BtkCombo *combo = BTK_COMBO (object);
  
  switch (prop_id)
    {
    case PROP_ENABLE_ARROW_KEYS:
      b_value_set_boolean (value, combo->use_arrows);
      break;
    case PROP_ENABLE_ARROWS_ALWAYS:
      b_value_set_boolean (value, combo->use_arrows_always);
      break;
    case PROP_CASE_SENSITIVE:
      b_value_set_boolean (value, combo->case_sensitive);
      break;
    case PROP_ALLOW_EMPTY:
      b_value_set_boolean (value, combo->ok_if_empty);
      break;
    case PROP_VALUE_IN_LIST:
      b_value_set_boolean (value, combo->value_in_list);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
   
}

#define __BTK_SMART_COMBO_C__
#include "btkaliasdef.c"
