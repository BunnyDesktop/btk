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
#include "btklabel.h"
#include "btkmarshalers.h"
#include "btkradiobutton.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkalias.h"


enum {
  PROP_0,
  PROP_GROUP
};


static void     btk_radio_button_destroy        (BtkObject           *object);
static gboolean btk_radio_button_focus          (BtkWidget           *widget,
						 BtkDirectionType     direction);
static void     btk_radio_button_clicked        (BtkButton           *button);
static void     btk_radio_button_draw_indicator (BtkCheckButton      *check_button,
						 BdkRectangle        *area);
static void     btk_radio_button_set_property   (GObject             *object,
						 guint                prop_id,
						 const GValue        *value,
						 GParamSpec          *pspec);
static void     btk_radio_button_get_property   (GObject             *object,
						 guint                prop_id,
						 GValue              *value,
						 GParamSpec          *pspec);

G_DEFINE_TYPE (BtkRadioButton, btk_radio_button, BTK_TYPE_CHECK_BUTTON)

static guint group_changed_signal = 0;

static void
btk_radio_button_class_init (BtkRadioButtonClass *class)
{
  GObjectClass *bobject_class;
  BtkObjectClass *object_class;
  BtkButtonClass *button_class;
  BtkCheckButtonClass *check_button_class;
  BtkWidgetClass *widget_class;

  bobject_class = G_OBJECT_CLASS (class);
  object_class = (BtkObjectClass*) class;
  widget_class = (BtkWidgetClass*) class;
  button_class = (BtkButtonClass*) class;
  check_button_class = (BtkCheckButtonClass*) class;

  bobject_class->set_property = btk_radio_button_set_property;
  bobject_class->get_property = btk_radio_button_get_property;

  g_object_class_install_property (bobject_class,
				   PROP_GROUP,
				   g_param_spec_object ("group",
							P_("Group"),
							P_("The radio button whose group this widget belongs to."),
							BTK_TYPE_RADIO_BUTTON,
							BTK_PARAM_WRITABLE));
  object_class->destroy = btk_radio_button_destroy;

  widget_class->focus = btk_radio_button_focus;

  button_class->clicked = btk_radio_button_clicked;

  check_button_class->draw_indicator = btk_radio_button_draw_indicator;

  class->group_changed = NULL;

  /**
   * BtkRadioButton::group-changed:
   * @style: the object which received the signal
   *
   * Emitted when the group of radio buttons that a radio button belongs
   * to changes. This is emitted when a radio button switches from
   * being alone to being part of a group of 2 or more buttons, or
   * vice-versa, and when a button is moved from one group of 2 or
   * more buttons to a different one, but not when the composition
   * of the group that a button belongs to changes.
   *
   * Since: 2.4
   */
  group_changed_signal = g_signal_new (I_("group-changed"),
				       G_OBJECT_CLASS_TYPE (object_class),
				       G_SIGNAL_RUN_FIRST,
				       G_STRUCT_OFFSET (BtkRadioButtonClass, group_changed),
				       NULL, NULL,
				       _btk_marshal_VOID__VOID,
				       G_TYPE_NONE, 0);
}

static void
btk_radio_button_init (BtkRadioButton *radio_button)
{
  btk_widget_set_has_window (BTK_WIDGET (radio_button), FALSE);
  btk_widget_set_receives_default (BTK_WIDGET (radio_button), FALSE);

  BTK_TOGGLE_BUTTON (radio_button)->active = TRUE;

  BTK_BUTTON (radio_button)->depress_on_activate = FALSE;

  radio_button->group = g_slist_prepend (NULL, radio_button);

  _btk_button_set_depressed (BTK_BUTTON (radio_button), TRUE);
  btk_widget_set_state (BTK_WIDGET (radio_button), BTK_STATE_ACTIVE);
}

static void
btk_radio_button_set_property (GObject      *object,
			       guint         prop_id,
			       const GValue *value,
			       GParamSpec   *pspec)
{
  BtkRadioButton *radio_button;

  radio_button = BTK_RADIO_BUTTON (object);

  switch (prop_id)
    {
      GSList *slist;
      BtkRadioButton *button;

    case PROP_GROUP:
        button = g_value_get_object (value);

      if (button)
	slist = btk_radio_button_get_group (button);
      else
	slist = NULL;
      btk_radio_button_set_group (radio_button, slist);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_radio_button_get_property (GObject    *object,
			       guint       prop_id,
			       GValue     *value,
			       GParamSpec *pspec)
{
  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/**
 * btk_radio_button_set_group:
 * @radio_button: a #BtkRadioButton.
 * @group: (transfer none) (element-type BtkRadioButton): an existing radio
 *     button group, such as one returned from btk_radio_button_get_group().
 *
 * Sets a #BtkRadioButton's group. It should be noted that this does not change
 * the layout of your interface in any way, so if you are changing the group,
 * it is likely you will need to re-arrange the user interface to reflect these
 * changes.
 */
void
btk_radio_button_set_group (BtkRadioButton *radio_button,
			    GSList         *group)
{
  BtkWidget *old_group_singleton = NULL;
  BtkWidget *new_group_singleton = NULL;
  
  g_return_if_fail (BTK_IS_RADIO_BUTTON (radio_button));
  g_return_if_fail (!g_slist_find (group, radio_button));

  if (radio_button->group)
    {
      GSList *slist;

      radio_button->group = g_slist_remove (radio_button->group, radio_button);
      
      if (radio_button->group && !radio_button->group->next)
	old_group_singleton = g_object_ref (radio_button->group->data);
	  
      for (slist = radio_button->group; slist; slist = slist->next)
	{
	  BtkRadioButton *tmp_button;
	  
	  tmp_button = slist->data;
	  
	  tmp_button->group = radio_button->group;
	}
    }
  
  if (group && !group->next)
    new_group_singleton = g_object_ref (group->data);
  
  radio_button->group = g_slist_prepend (group, radio_button);
  
  if (group)
    {
      GSList *slist;
      
      for (slist = group; slist; slist = slist->next)
	{
	  BtkRadioButton *tmp_button;
	  
	  tmp_button = slist->data;
	  
	  tmp_button->group = radio_button->group;
	}
    }

  g_object_ref (radio_button);
  
  g_object_notify (G_OBJECT (radio_button), "group");
  g_signal_emit (radio_button, group_changed_signal, 0);
  if (old_group_singleton)
    {
      g_signal_emit (old_group_singleton, group_changed_signal, 0);
      g_object_unref (old_group_singleton);
    }
  if (new_group_singleton)
    {
      g_signal_emit (new_group_singleton, group_changed_signal, 0);
      g_object_unref (new_group_singleton);
    }

  btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (radio_button), group == NULL);

  g_object_unref (radio_button);
}

/**
 * btk_radio_button_new:
 * @group: (allow-none): an existing radio button group, or %NULL if you are
 *  creating a new group.
 *
 * Creates a new #BtkRadioButton. To be of any practical value, a widget should
 * then be packed into the radio button.
 *
 * Returns: a new radio button
 */
BtkWidget*
btk_radio_button_new (GSList *group)
{
  BtkRadioButton *radio_button;

  radio_button = g_object_new (BTK_TYPE_RADIO_BUTTON, NULL);

  if (group)
    btk_radio_button_set_group (radio_button, group);

  return BTK_WIDGET (radio_button);
}

/**
 * btk_radio_button_new_with_label:
 * @group: (allow-none): an existing radio button group, or %NULL if you are
 *  creating a new group.
 * @label: the text label to display next to the radio button.
 *
 * Creates a new #BtkRadioButton with a text label.
 *
 * Returns: a new radio button.
 */
BtkWidget*
btk_radio_button_new_with_label (GSList      *group,
				 const gchar *label)
{
  BtkWidget *radio_button;

  radio_button = g_object_new (BTK_TYPE_RADIO_BUTTON, "label", label, NULL) ;

  if (group)
    btk_radio_button_set_group (BTK_RADIO_BUTTON (radio_button), group);

  return radio_button;
}

/**
 * btk_radio_button_new_with_mnemonic:
 * @group: the radio button group
 * @label: the text of the button, with an underscore in front of the
 *         mnemonic character
 *
 * Creates a new #BtkRadioButton containing a label, adding it to the same 
 * group as @group. The label will be created using 
 * btk_label_new_with_mnemonic(), so underscores in @label indicate the 
 * mnemonic for the button.
 *
 * Returns: a new #BtkRadioButton
 **/
BtkWidget*
btk_radio_button_new_with_mnemonic (GSList      *group,
				    const gchar *label)
{
  BtkWidget *radio_button;

  radio_button = g_object_new (BTK_TYPE_RADIO_BUTTON, 
			       "label", label, 
			       "use-underline", TRUE, 
			       NULL);

  if (group)
    btk_radio_button_set_group (BTK_RADIO_BUTTON (radio_button), group);

  return radio_button;
}

/**
 * btk_radio_button_new_from_widget:
 * @radio_group_member: (allow-none): an existing #BtkRadioButton.
 *
 * Creates a new #BtkRadioButton, adding it to the same group as
 * @radio_group_member. As with btk_radio_button_new(), a widget
 * should be packed into the radio button.
 *
 * Returns: (transfer none): a new radio button.
 */
BtkWidget*
btk_radio_button_new_from_widget (BtkRadioButton *radio_group_member)
{
  GSList *l = NULL;
  if (radio_group_member)
    l = btk_radio_button_get_group (radio_group_member);
  return btk_radio_button_new (l);
}

/**
 * btk_radio_button_new_with_label_from_widget: (constructor)
 * @radio_group_member: (allow-none): widget to get radio group from or %NULL
 * @label: a text string to display next to the radio button.
 *
 * Creates a new #BtkRadioButton with a text label, adding it to
 * the same group as @radio_group_member.
 *
 * Returns: (transfer none): a new radio button.
 */
BtkWidget*
btk_radio_button_new_with_label_from_widget (BtkRadioButton *radio_group_member,
					     const gchar    *label)
{
  GSList *l = NULL;
  if (radio_group_member)
    l = btk_radio_button_get_group (radio_group_member);
  return btk_radio_button_new_with_label (l, label);
}

/**
 * btk_radio_button_new_with_mnemonic_from_widget: (constructor)
 * @radio_group_member: (allow-none): widget to get radio group from or %NULL
 * @label: the text of the button, with an underscore in front of the
 *         mnemonic character
 *
 * Creates a new #BtkRadioButton containing a label. The label
 * will be created using btk_label_new_with_mnemonic(), so underscores
 * in @label indicate the mnemonic for the button.
 *
 * Returns: (transfer none): a new #BtkRadioButton
 **/
BtkWidget*
btk_radio_button_new_with_mnemonic_from_widget (BtkRadioButton *radio_group_member,
					        const gchar    *label)
{
  GSList *l = NULL;
  if (radio_group_member)
    l = btk_radio_button_get_group (radio_group_member);
  return btk_radio_button_new_with_mnemonic (l, label);
}


/**
 * btk_radio_button_get_group:
 * @radio_button: a #BtkRadioButton.
 *
 * Retrieves the group assigned to a radio button.
 *
 * Return value: (element-type BtkRadioButton) (transfer none): a linked list
 * containing all the radio buttons in the same group
 * as @radio_button. The returned list is owned by the radio button
 * and must not be modified or freed.
 */
GSList*
btk_radio_button_get_group (BtkRadioButton *radio_button)
{
  g_return_val_if_fail (BTK_IS_RADIO_BUTTON (radio_button), NULL);

  return radio_button->group;
}


static void
btk_radio_button_destroy (BtkObject *object)
{
  BtkWidget *old_group_singleton = NULL;
  BtkRadioButton *radio_button;
  BtkRadioButton *tmp_button;
  GSList *tmp_list;
  gboolean was_in_group;
  
  radio_button = BTK_RADIO_BUTTON (object);

  was_in_group = radio_button->group && radio_button->group->next;
  
  radio_button->group = g_slist_remove (radio_button->group, radio_button);
  if (radio_button->group && !radio_button->group->next)
    old_group_singleton = radio_button->group->data;

  tmp_list = radio_button->group;

  while (tmp_list)
    {
      tmp_button = tmp_list->data;
      tmp_list = tmp_list->next;

      tmp_button->group = radio_button->group;
    }

  /* this button is no longer in the group */
  radio_button->group = NULL;

  if (old_group_singleton)
    g_signal_emit (old_group_singleton, group_changed_signal, 0);
  if (was_in_group)
    g_signal_emit (radio_button, group_changed_signal, 0);

  BTK_OBJECT_CLASS (btk_radio_button_parent_class)->destroy (object);
}

static void
get_coordinates (BtkWidget    *widget,
		 BtkWidget    *reference,
		 gint         *x,
		 gint         *y)
{
  *x = widget->allocation.x + widget->allocation.width / 2;
  *y = widget->allocation.y + widget->allocation.height / 2;
  
  btk_widget_translate_coordinates (widget, reference, *x, *y, x, y);
}

static gint
left_right_compare (gconstpointer a,
		    gconstpointer b,
		    gpointer      data)
{
  gint x1, y1, x2, y2;

  get_coordinates ((BtkWidget *)a, data, &x1, &y1);
  get_coordinates ((BtkWidget *)b, data, &x2, &y2);

  if (y1 == y2)
    return (x1 < x2) ? -1 : ((x1 == x2) ? 0 : 1);
  else
    return (y1 < y2) ? -1 : 1;
}

static gint
up_down_compare (gconstpointer a,
		 gconstpointer b,
		 gpointer      data)
{
  gint x1, y1, x2, y2;
  
  get_coordinates ((BtkWidget *)a, data, &x1, &y1);
  get_coordinates ((BtkWidget *)b, data, &x2, &y2);
  
  if (x1 == x2)
    return (y1 < y2) ? -1 : ((y1 == y2) ? 0 : 1);
  else
    return (x1 < x2) ? -1 : 1;
}

static gboolean
btk_radio_button_focus (BtkWidget         *widget,
			BtkDirectionType   direction)
{
  BtkRadioButton *radio_button = BTK_RADIO_BUTTON (widget);
  GSList *tmp_slist;

  /* Radio buttons with draw_indicator unset focus "normally", since
   * they look like buttons to the user.
   */
  if (!BTK_TOGGLE_BUTTON (widget)->draw_indicator)
    return BTK_WIDGET_CLASS (btk_radio_button_parent_class)->focus (widget, direction);
  
  if (btk_widget_is_focus (widget))
    {
      BtkSettings *settings = btk_widget_get_settings (widget);
      GSList *focus_list, *tmp_list;
      BtkWidget *toplevel = btk_widget_get_toplevel (widget);
      BtkWidget *new_focus = NULL;
      gboolean cursor_only;
      gboolean wrap_around;

      switch (direction)
	{
	case BTK_DIR_LEFT:
	case BTK_DIR_RIGHT:
	  focus_list = g_slist_copy (radio_button->group);
	  focus_list = g_slist_sort_with_data (focus_list, left_right_compare, toplevel);
	  break;
	case BTK_DIR_UP:
	case BTK_DIR_DOWN:
	  focus_list = g_slist_copy (radio_button->group);
	  focus_list = g_slist_sort_with_data (focus_list, up_down_compare, toplevel);
	  break;
	case BTK_DIR_TAB_FORWARD:
	case BTK_DIR_TAB_BACKWARD:
          /* fall through */
        default:
	  return FALSE;
	}

      if (direction == BTK_DIR_LEFT || direction == BTK_DIR_UP)
	focus_list = g_slist_reverse (focus_list);

      tmp_list = g_slist_find (focus_list, widget);

      if (tmp_list)
	{
	  tmp_list = tmp_list->next;
	  
	  while (tmp_list)
	    {
	      BtkWidget *child = tmp_list->data;
	      
	      if (btk_widget_get_mapped (child) && btk_widget_is_sensitive (child))
		{
		  new_focus = child;
		  break;
		}

	      tmp_list = tmp_list->next;
	    }
	}

      g_object_get (settings,
                    "btk-keynav-cursor-only", &cursor_only,
                    "btk-keynav-wrap-around", &wrap_around,
                    NULL);

      if (!new_focus)
	{
          if (cursor_only)
            {
              g_slist_free (focus_list);
              return FALSE;
            }

          if (!wrap_around)
            {
              g_slist_free (focus_list);
              btk_widget_error_bell (widget);
              return TRUE;
            }

	  tmp_list = focus_list;

	  while (tmp_list)
	    {
	      BtkWidget *child = tmp_list->data;
	      
	      if (btk_widget_get_mapped (child) && btk_widget_is_sensitive (child))
		{
		  new_focus = child;
		  break;
		}
	      
	      tmp_list = tmp_list->next;
	    }
	}
      
      g_slist_free (focus_list);

      if (new_focus)
	{
	  btk_widget_grab_focus (new_focus);

          if (!cursor_only)
            btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (new_focus), TRUE);
	}

      return TRUE;
    }
  else
    {
      BtkRadioButton *selected_button = NULL;
      
      /* We accept the focus if, we don't have the focus and
       *  - we are the currently active button in the group
       *  - there is no currently active radio button.
       */
      
      tmp_slist = radio_button->group;
      while (tmp_slist)
	{
	  if (BTK_TOGGLE_BUTTON (tmp_slist->data)->active)
	    selected_button = tmp_slist->data;
	  tmp_slist = tmp_slist->next;
	}
      
      if (selected_button && selected_button != radio_button)
	return FALSE;

      btk_widget_grab_focus (widget);
      return TRUE;
    }
}

static void
btk_radio_button_clicked (BtkButton *button)
{
  BtkToggleButton *toggle_button;
  BtkRadioButton *radio_button;
  BtkToggleButton *tmp_button;
  BtkStateType new_state;
  GSList *tmp_list;
  gint toggled;
  gboolean depressed;

  radio_button = BTK_RADIO_BUTTON (button);
  toggle_button = BTK_TOGGLE_BUTTON (button);
  toggled = FALSE;

  g_object_ref (BTK_WIDGET (button));

  if (toggle_button->active)
    {
      tmp_button = NULL;
      tmp_list = radio_button->group;

      while (tmp_list)
	{
	  tmp_button = tmp_list->data;
	  tmp_list = tmp_list->next;

	  if (tmp_button->active && tmp_button != toggle_button)
	    break;

	  tmp_button = NULL;
	}

      if (!tmp_button)
	{
	  new_state = (button->in_button ? BTK_STATE_PRELIGHT : BTK_STATE_ACTIVE);
	}
      else
	{
	  toggled = TRUE;
	  toggle_button->active = !toggle_button->active;
	  new_state = (button->in_button ? BTK_STATE_PRELIGHT : BTK_STATE_NORMAL);
	}
    }
  else
    {
      toggled = TRUE;
      toggle_button->active = !toggle_button->active;
      
      tmp_list = radio_button->group;
      while (tmp_list)
	{
	  tmp_button = tmp_list->data;
	  tmp_list = tmp_list->next;

	  if (tmp_button->active && (tmp_button != toggle_button))
	    {
	      btk_button_clicked (BTK_BUTTON (tmp_button));
	      break;
	    }
	}

      new_state = (button->in_button ? BTK_STATE_PRELIGHT : BTK_STATE_ACTIVE);
    }

  if (toggle_button->inconsistent)
    depressed = FALSE;
  else if (button->in_button && button->button_down)
    depressed = !toggle_button->active;
  else
    depressed = toggle_button->active;

  if (btk_widget_get_state (BTK_WIDGET (button)) != new_state)
    btk_widget_set_state (BTK_WIDGET (button), new_state);

  if (toggled)
    {
      btk_toggle_button_toggled (toggle_button);

      g_object_notify (G_OBJECT (toggle_button), "active");
    }

  _btk_button_set_depressed (button, depressed);

  btk_widget_queue_draw (BTK_WIDGET (button));

  g_object_unref (button);
}

static void
btk_radio_button_draw_indicator (BtkCheckButton *check_button,
				 BdkRectangle   *area)
{
  BtkWidget *widget;
  BtkWidget *child;
  BtkButton *button;
  BtkToggleButton *toggle_button;
  BtkStateType state_type;
  BtkShadowType shadow_type;
  gint x, y;
  gint indicator_size, indicator_spacing;
  gint focus_width;
  gint focus_pad;
  gboolean interior_focus;

  widget = BTK_WIDGET (check_button);

  if (btk_widget_is_drawable (widget))
    {
      button = BTK_BUTTON (check_button);
      toggle_button = BTK_TOGGLE_BUTTON (check_button);

      btk_widget_style_get (widget,
			    "interior-focus", &interior_focus,
			    "focus-line-width", &focus_width,
			    "focus-padding", &focus_pad,
			    NULL);

      _btk_check_button_get_props (check_button, &indicator_size, &indicator_spacing);

      x = widget->allocation.x + indicator_spacing + BTK_CONTAINER (widget)->border_width;
      y = widget->allocation.y + (widget->allocation.height - indicator_size) / 2;

      child = BTK_BIN (check_button)->child;
      if (!interior_focus || !(child && btk_widget_get_visible (child)))
	x += focus_width + focus_pad;      

      if (toggle_button->inconsistent)
	shadow_type = BTK_SHADOW_ETCHED_IN;
      else if (toggle_button->active)
	shadow_type = BTK_SHADOW_IN;
      else
	shadow_type = BTK_SHADOW_OUT;

      if (button->activate_timeout || (button->button_down && button->in_button))
	state_type = BTK_STATE_ACTIVE;
      else if (button->in_button)
	state_type = BTK_STATE_PRELIGHT;
      else if (!btk_widget_is_sensitive (widget))
	state_type = BTK_STATE_INSENSITIVE;
      else
	state_type = BTK_STATE_NORMAL;

      if (btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL)
	x = widget->allocation.x + widget->allocation.width - (indicator_size + x - widget->allocation.x);

      if (btk_widget_get_state (widget) == BTK_STATE_PRELIGHT)
	{
	  BdkRectangle restrict_area;
	  BdkRectangle new_area;
	      
	  restrict_area.x = widget->allocation.x + BTK_CONTAINER (widget)->border_width;
	  restrict_area.y = widget->allocation.y + BTK_CONTAINER (widget)->border_width;
	  restrict_area.width = widget->allocation.width - (2 * BTK_CONTAINER (widget)->border_width);
	  restrict_area.height = widget->allocation.height - (2 * BTK_CONTAINER (widget)->border_width);
	  
	  if (bdk_rectangle_intersect (area, &restrict_area, &new_area))
	    {
	      btk_paint_flat_box (widget->style, widget->window, BTK_STATE_PRELIGHT,
				  BTK_SHADOW_ETCHED_OUT, 
				  area, widget, "checkbutton",
				  new_area.x, new_area.y,
				  new_area.width, new_area.height);
	    }
	}

      btk_paint_option (widget->style, widget->window,
			state_type, shadow_type,
			area, widget, "radiobutton",
			x, y, indicator_size, indicator_size);
    }
}

#define __BTK_RADIO_BUTTON_C__
#include "btkaliasdef.c"
