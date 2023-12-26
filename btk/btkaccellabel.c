/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * BtkAccelLabel: BtkLabel with accelerator monitoring facilities.
 * Copyright (C) 1998 Tim Janik
 *
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
 * Modified by the BTK+ Team and others 1997-2001.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#include "config.h"
#include <string.h>

#include "btkaccellabel.h"
#include "btkaccelmap.h"
#include "btkmain.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkalias.h"
#include <bdk/bdkkeysyms.h>

/**
 * SECTION:btkaccellabel
 * @Short_description: A label which displays an accelerator key on the right of the text
 * @Title: BtkAccelLabel
 * @See_also: #BtkItemFactory, #BtkAccelGroup
 *
 * The #BtkAccelLabel widget is a subclass of #BtkLabel that also displays an
 * accelerator key on the right of the label text, e.g. 'Ctl+S'.
 * It is commonly used in menus to show the keyboard short-cuts for commands.
 *
 * The accelerator key to display is not set explicitly.
 * Instead, the #BtkAccelLabel displays the accelerators which have been added to
 * a particular widget. This widget is set by calling
 * btk_accel_label_set_accel_widget().
 *
 * For example, a #BtkMenuItem widget may have an accelerator added to emit the
 * "activate" signal when the 'Ctl+S' key combination is pressed.
 * A #BtkAccelLabel is created and added to the #BtkMenuItem, and
 * btk_accel_label_set_accel_widget() is called with the #BtkMenuItem as the
 * second argument. The #BtkAccelLabel will now display 'Ctl+S' after its label.
 *
 * Note that creating a #BtkMenuItem with btk_menu_item_new_with_label() (or
 * one of the similar functions for #BtkCheckMenuItem and #BtkRadioMenuItem)
 * automatically adds a #BtkAccelLabel to the #BtkMenuItem and calls
 * btk_accel_label_set_accel_widget() to set it up for you.
 *
 * A #BtkAccelLabel will only display accelerators which have %BTK_ACCEL_VISIBLE
 * set (see #BtkAccelFlags).
 * A #BtkAccelLabel can display multiple accelerators and even signal names,
 * though it is almost always used to display just one accelerator key.
 * <example>
 * <title>Creating a simple menu item with an accelerator key.</title>
 * <programlisting>
 *   BtkWidget *save_item;
 *   BtkAccelGroup *accel_group;
 *
 *   /<!---->* Create a BtkAccelGroup and add it to the window. *<!---->/
 *   accel_group = btk_accel_group_new (<!-- -->);
 *   btk_window_add_accel_group (BTK_WINDOW (window), accel_group);
 *
 *   /<!---->* Create the menu item using the convenience function. *<!---->/
 *   save_item = btk_menu_item_new_with_label ("Save");
 *   btk_widget_show (save_item);
 *   btk_container_add (BTK_CONTAINER (menu), save_item);
 *
 *   /<!---->* Now add the accelerator to the BtkMenuItem. Note that since we called
 *      btk_menu_item_new_with_label(<!-- -->) to create the BtkMenuItem the
 *      BtkAccelLabel is automatically set up to display the BtkMenuItem
 *      accelerators. We just need to make sure we use BTK_ACCEL_VISIBLE here. *<!---->/
 *   btk_widget_add_accelerator (save_item, "activate", accel_group,
 *                               BDK_s, BDK_CONTROL_MASK, BTK_ACCEL_VISIBLE);
 * </programlisting>
 * </example>
 */

enum {
  PROP_0,
  PROP_ACCEL_CLOSURE,
  PROP_ACCEL_WIDGET
};

static void         btk_accel_label_set_property (BObject            *object,
						  guint               prop_id,
						  const BValue       *value,
						  BParamSpec         *pspec);
static void         btk_accel_label_get_property (BObject            *object,
						  guint               prop_id,
						  BValue             *value,
						  BParamSpec         *pspec);
static void         btk_accel_label_destroy      (BtkObject          *object);
static void         btk_accel_label_finalize     (BObject            *object);
static void         btk_accel_label_size_request (BtkWidget          *widget,
						  BtkRequisition     *requisition);
static gboolean     btk_accel_label_expose_event (BtkWidget          *widget,
						  BdkEventExpose     *event);
static const gchar *btk_accel_label_get_string   (BtkAccelLabel      *accel_label);


G_DEFINE_TYPE (BtkAccelLabel, btk_accel_label, BTK_TYPE_LABEL)

static void
btk_accel_label_class_init (BtkAccelLabelClass *class)
{
  BObjectClass *bobject_class = B_OBJECT_CLASS (class);
  BtkObjectClass *object_class = BTK_OBJECT_CLASS (class);
  BtkWidgetClass *widget_class = BTK_WIDGET_CLASS (class);
  
  bobject_class->finalize = btk_accel_label_finalize;
  bobject_class->set_property = btk_accel_label_set_property;
  bobject_class->get_property = btk_accel_label_get_property;
  
  object_class->destroy = btk_accel_label_destroy;
   
  widget_class->size_request = btk_accel_label_size_request;
  widget_class->expose_event = btk_accel_label_expose_event;

  class->signal_quote1 = g_strdup ("<:");
  class->signal_quote2 = g_strdup (":>");

#ifndef BDK_WINDOWING_QUARTZ
  /* This is the text that should appear next to menu accelerators
   * that use the shift key. If the text on this key isn't typically
   * translated on keyboards used for your language, don't translate
   * this.
   */
  class->mod_name_shift = g_strdup (C_("keyboard label", "Shift"));
  /* This is the text that should appear next to menu accelerators
   * that use the control key. If the text on this key isn't typically
   * translated on keyboards used for your language, don't translate
   * this.
   */
  class->mod_name_control = g_strdup (C_("keyboard label", "Ctrl"));
  /* This is the text that should appear next to menu accelerators
   * that use the alt key. If the text on this key isn't typically
   * translated on keyboards used for your language, don't translate
   * this.
   */
  class->mod_name_alt = g_strdup (C_("keyboard label", "Alt"));
  class->mod_separator = g_strdup ("+");
#else /* BDK_WINDOWING_QUARTZ */

  /* U+21E7 UPWARDS WHITE ARROW */
  class->mod_name_shift = g_strdup ("\xe2\x87\xa7");
  /* U+2303 UP ARROWHEAD */
  class->mod_name_control = g_strdup ("\xe2\x8c\x83");
  /* U+2325 OPTION KEY */
  class->mod_name_alt = g_strdup ("\xe2\x8c\xa5");
  class->mod_separator = g_strdup ("");

#endif /* BDK_WINDOWING_QUARTZ */

  class->accel_seperator = g_strdup (" / ");
  class->latin1_to_char = TRUE;
  
  g_object_class_install_property (bobject_class,
                                   PROP_ACCEL_CLOSURE,
                                   g_param_spec_boxed ("accel-closure",
						       P_("Accelerator Closure"),
						       P_("The closure to be monitored for accelerator changes"),
						       B_TYPE_CLOSURE,
						       BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
                                   PROP_ACCEL_WIDGET,
                                   g_param_spec_object ("accel-widget",
                                                        P_("Accelerator Widget"),
                                                        P_("The widget to be monitored for accelerator changes"),
                                                        BTK_TYPE_WIDGET,
                                                        BTK_PARAM_READWRITE));
}

static void
btk_accel_label_set_property (BObject      *object,
			      guint         prop_id,
			      const BValue *value,
			      BParamSpec   *pspec)
{
  BtkAccelLabel  *accel_label;

  accel_label = BTK_ACCEL_LABEL (object);

  switch (prop_id)
    {
    case PROP_ACCEL_CLOSURE:
      btk_accel_label_set_accel_closure (accel_label, b_value_get_boxed (value));
      break;
    case PROP_ACCEL_WIDGET:
      btk_accel_label_set_accel_widget (accel_label, b_value_get_object (value));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_accel_label_get_property (BObject    *object,
			      guint       prop_id,
			      BValue     *value,
			      BParamSpec *pspec)
{
  BtkAccelLabel  *accel_label;

  accel_label = BTK_ACCEL_LABEL (object);

  switch (prop_id)
    {
    case PROP_ACCEL_CLOSURE:
      b_value_set_boxed (value, accel_label->accel_closure);
      break;
    case PROP_ACCEL_WIDGET:
      b_value_set_object (value, accel_label->accel_widget);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_accel_label_init (BtkAccelLabel *accel_label)
{
  accel_label->accel_padding = 3;
  accel_label->accel_widget = NULL;
  accel_label->accel_closure = NULL;
  accel_label->accel_group = NULL;
  accel_label->accel_string = NULL;
}

/**
 * btk_accel_label_new:
 * @string: the label string. Must be non-%NULL.
 *
 * Creates a new #BtkAccelLabel.
 *
 * Returns: a new #BtkAccelLabel.
 */
BtkWidget*
btk_accel_label_new (const gchar *string)
{
  BtkAccelLabel *accel_label;
  
  g_return_val_if_fail (string != NULL, NULL);
  
  accel_label = g_object_new (BTK_TYPE_ACCEL_LABEL, NULL);
  
  btk_label_set_text (BTK_LABEL (accel_label), string);
  
  return BTK_WIDGET (accel_label);
}

static void
btk_accel_label_destroy (BtkObject *object)
{
  BtkAccelLabel *accel_label = BTK_ACCEL_LABEL (object);

  btk_accel_label_set_accel_widget (accel_label, NULL);
  btk_accel_label_set_accel_closure (accel_label, NULL);
  
  BTK_OBJECT_CLASS (btk_accel_label_parent_class)->destroy (object);
}

static void
btk_accel_label_finalize (BObject *object)
{
  BtkAccelLabel *accel_label = BTK_ACCEL_LABEL (object);

  g_free (accel_label->accel_string);
  
  B_OBJECT_CLASS (btk_accel_label_parent_class)->finalize (object);
}

/**
 * btk_accel_label_get_accel_widget:
 * @accel_label: a #BtkAccelLabel
 *
 * Fetches the widget monitored by this accelerator label. See
 * btk_accel_label_set_accel_widget().
 *
 * Returns: (transfer none): the object monitored by the accelerator label, or %NULL.
 **/
BtkWidget*
btk_accel_label_get_accel_widget (BtkAccelLabel *accel_label)
{
  g_return_val_if_fail (BTK_IS_ACCEL_LABEL (accel_label), NULL);

  return accel_label->accel_widget;
}

/**
 * btk_accel_label_get_accel_width:
 * @accel_label: a #BtkAccelLabel.
 *
 * Returns the width needed to display the accelerator key(s).
 * This is used by menus to align all of the #BtkMenuItem widgets, and shouldn't
 * be needed by applications.
 *
 * Returns: the width needed to display the accelerator key(s).
 */
guint
btk_accel_label_get_accel_width (BtkAccelLabel *accel_label)
{
  g_return_val_if_fail (BTK_IS_ACCEL_LABEL (accel_label), 0);
  
  return (accel_label->accel_string_width +
	  (accel_label->accel_string_width ? accel_label->accel_padding : 0));
}

static void
btk_accel_label_size_request (BtkWidget	     *widget,
			      BtkRequisition *requisition)
{
  BtkAccelLabel *accel_label = BTK_ACCEL_LABEL (widget);
  BangoLayout *layout;
  gint width;

  BTK_WIDGET_CLASS (btk_accel_label_parent_class)->size_request (widget, requisition);

  layout = btk_widget_create_bango_layout (widget, btk_accel_label_get_string (accel_label));
  bango_layout_get_pixel_size (layout, &width, NULL);
  accel_label->accel_string_width = width;
  
  g_object_unref (layout);
}

static gint
get_first_baseline (BangoLayout *layout)
{
  BangoLayoutIter *iter;
  gint result;

  iter = bango_layout_get_iter (layout);
  result = bango_layout_iter_get_baseline (iter);
  bango_layout_iter_free (iter);

  return BANGO_PIXELS (result);
}

static gboolean 
btk_accel_label_expose_event (BtkWidget      *widget,
			      BdkEventExpose *event)
{
  BtkAccelLabel *accel_label = BTK_ACCEL_LABEL (widget);
  BtkMisc *misc = BTK_MISC (accel_label);
  BtkTextDirection direction;

  direction = btk_widget_get_direction (widget);

  if (btk_widget_is_drawable (widget))
    {
      guint ac_width;
      
      ac_width = btk_accel_label_get_accel_width (accel_label);
      
      if (widget->allocation.width >= widget->requisition.width + ac_width)
	{
	  BangoLayout *label_layout;
	  BangoLayout *accel_layout;
	  BtkLabel *label = BTK_LABEL (widget);

	  gint x;
	  gint y;
	  
	  label_layout = btk_label_get_layout (BTK_LABEL (accel_label));

	  if (direction == BTK_TEXT_DIR_RTL)
	    widget->allocation.x += ac_width;
	  widget->allocation.width -= ac_width;
	  if (btk_label_get_ellipsize (label))
	    bango_layout_set_width (label_layout,
				    bango_layout_get_width (label_layout) 
				    - ac_width * BANGO_SCALE);
	  
	  if (BTK_WIDGET_CLASS (btk_accel_label_parent_class)->expose_event)
	    BTK_WIDGET_CLASS (btk_accel_label_parent_class)->expose_event (widget, event);
	  if (direction == BTK_TEXT_DIR_RTL)
	    widget->allocation.x -= ac_width;
	  widget->allocation.width += ac_width;
	  if (btk_label_get_ellipsize (label))
	    bango_layout_set_width (label_layout,
				    bango_layout_get_width (label_layout) 
				    + ac_width * BANGO_SCALE);
	  
	  if (direction == BTK_TEXT_DIR_RTL)
	    x = widget->allocation.x + misc->xpad;
	  else
	    x = widget->allocation.x + widget->allocation.width - misc->xpad - ac_width;

	  btk_label_get_layout_offsets (BTK_LABEL (accel_label), NULL, &y);

	  accel_layout = btk_widget_create_bango_layout (widget, btk_accel_label_get_string (accel_label));

	  y += get_first_baseline (label_layout) - get_first_baseline (accel_layout);

          btk_paint_layout (widget->style,
                            widget->window,
                            btk_widget_get_state (widget),
			    FALSE,
                            &event->area,
                            widget,
                            "accellabel",
                            x, y,
                            accel_layout);                            

          g_object_unref (accel_layout);
	}
      else
	{
	  if (BTK_WIDGET_CLASS (btk_accel_label_parent_class)->expose_event)
	    BTK_WIDGET_CLASS (btk_accel_label_parent_class)->expose_event (widget, event);
	}
    }
  
  return FALSE;
}

static void
refetch_widget_accel_closure (BtkAccelLabel *accel_label)
{
  GClosure *closure = NULL;
  GList *clist, *list;
  
  g_return_if_fail (BTK_IS_ACCEL_LABEL (accel_label));
  g_return_if_fail (BTK_IS_WIDGET (accel_label->accel_widget));
  
  clist = btk_widget_list_accel_closures (accel_label->accel_widget);
  for (list = clist; list; list = list->next)
    {
      /* we just take the first closure used */
      closure = list->data;
      break;
    }
  g_list_free (clist);
  btk_accel_label_set_accel_closure (accel_label, closure);
}

/**
 * btk_accel_label_set_accel_widget:
 * @accel_label: a #BtkAccelLabel
 * @accel_widget: the widget to be monitored.
 *
 * Sets the widget to be monitored by this accelerator label. 
 **/
void
btk_accel_label_set_accel_widget (BtkAccelLabel *accel_label,
				  BtkWidget     *accel_widget)
{
  g_return_if_fail (BTK_IS_ACCEL_LABEL (accel_label));
  if (accel_widget)
    g_return_if_fail (BTK_IS_WIDGET (accel_widget));
    
  if (accel_widget != accel_label->accel_widget)
    {
      if (accel_label->accel_widget)
	{
	  btk_accel_label_set_accel_closure (accel_label, NULL);
	  g_signal_handlers_disconnect_by_func (accel_label->accel_widget,
						refetch_widget_accel_closure,
						accel_label);
	  g_object_unref (accel_label->accel_widget);
	}
      accel_label->accel_widget = accel_widget;
      if (accel_label->accel_widget)
	{
	  g_object_ref (accel_label->accel_widget);
	  g_signal_connect_object (accel_label->accel_widget, "accel-closures-changed",
				   G_CALLBACK (refetch_widget_accel_closure),
				   accel_label, G_CONNECT_SWAPPED);
	  refetch_widget_accel_closure (accel_label);
	}
      g_object_notify (B_OBJECT (accel_label), "accel-widget");
    }
}

static void
btk_accel_label_reset (BtkAccelLabel *accel_label)
{
  if (accel_label->accel_string)
    {
      g_free (accel_label->accel_string);
      accel_label->accel_string = NULL;
    }
  
  btk_widget_queue_resize (BTK_WIDGET (accel_label));
}

static void
check_accel_changed (BtkAccelGroup  *accel_group,
		     guint           keyval,
		     BdkModifierType modifier,
		     GClosure       *accel_closure,
		     BtkAccelLabel  *accel_label)
{
  if (accel_closure == accel_label->accel_closure)
    btk_accel_label_reset (accel_label);
}

/**
 * btk_accel_label_set_accel_closure:
 * @accel_label: a #BtkAccelLabel
 * @accel_closure: the closure to monitor for accelerator changes.
 *
 * Sets the closure to be monitored by this accelerator label. The closure
 * must be connected to an accelerator group; see btk_accel_group_connect().
 **/
void
btk_accel_label_set_accel_closure (BtkAccelLabel *accel_label,
				   GClosure      *accel_closure)
{
  g_return_if_fail (BTK_IS_ACCEL_LABEL (accel_label));
  if (accel_closure)
    g_return_if_fail (btk_accel_group_from_accel_closure (accel_closure) != NULL);

  if (accel_closure != accel_label->accel_closure)
    {
      if (accel_label->accel_closure)
	{
	  g_signal_handlers_disconnect_by_func (accel_label->accel_group,
						check_accel_changed,
						accel_label);
	  accel_label->accel_group = NULL;
	  g_closure_unref (accel_label->accel_closure);
	}
      accel_label->accel_closure = accel_closure;
      if (accel_label->accel_closure)
	{
	  g_closure_ref (accel_label->accel_closure);
	  accel_label->accel_group = btk_accel_group_from_accel_closure (accel_closure);
	  g_signal_connect_object (accel_label->accel_group, "accel-changed",
				   G_CALLBACK (check_accel_changed),
				   accel_label, 0);
	}
      btk_accel_label_reset (accel_label);
      g_object_notify (B_OBJECT (accel_label), "accel-closure");
    }
}

static gboolean
find_accel (BtkAccelKey *key,
	    GClosure    *closure,
	    gpointer     data)
{
  return data == (gpointer) closure;
}

static const gchar *
btk_accel_label_get_string (BtkAccelLabel *accel_label)
{
  if (!accel_label->accel_string)
    btk_accel_label_refetch (accel_label);
  
  return accel_label->accel_string;
}

/* Underscores in key names are better displayed as spaces
 * E.g., Page_Up should be "Page Up"
 */
static void
substitute_underscores (char *str)
{
  char *p;

  for (p = str; *p; p++)
    if (*p == '_')
      *p = ' ';
}

/* On Mac, if the key has symbolic representation (e.g. arrow keys),
 * append it to gstring and return TRUE; otherwise return FALSE.
 * See http://docs.info.apple.com/article.html?path=Mac/10.5/en/cdb_symbs.html 
 * for the list of special keys. */
static gboolean
append_keyval_symbol (guint    accelerator_key,
                      GString *gstring)
{
#ifdef BDK_WINDOWING_QUARTZ
  switch (accelerator_key)
  {
  case BDK_Return:
    /* U+21A9 LEFTWARDS ARROW WITH HOOK */
    g_string_append (gstring, "\xe2\x86\xa9");
    return TRUE;

  case BDK_ISO_Enter:
    /* U+2324 UP ARROWHEAD BETWEEN TWO HORIZONTAL BARS */
    g_string_append (gstring, "\xe2\x8c\xa4");
    return TRUE;

  case BDK_Left:
    /* U+2190 LEFTWARDS ARROW */
    g_string_append (gstring, "\xe2\x86\x90");
    return TRUE;

  case BDK_Up:
    /* U+2191 UPWARDS ARROW */
    g_string_append (gstring, "\xe2\x86\x91");
    return TRUE;

  case BDK_Right:
    /* U+2192 RIGHTWARDS ARROW */
    g_string_append (gstring, "\xe2\x86\x92");
    return TRUE;

  case BDK_Down:
    /* U+2193 DOWNWARDS ARROW */
    g_string_append (gstring, "\xe2\x86\x93");
    return TRUE;

  case BDK_Page_Up:
    /* U+21DE UPWARDS ARROW WITH DOUBLE STROKE */
    g_string_append (gstring, "\xe2\x87\x9e");
    return TRUE;

  case BDK_Page_Down:
    /* U+21DF DOWNWARDS ARROW WITH DOUBLE STROKE */
    g_string_append (gstring, "\xe2\x87\x9f");
    return TRUE;

  case BDK_Home:
    /* U+2196 NORTH WEST ARROW */
    g_string_append (gstring, "\xe2\x86\x96");
    return TRUE;

  case BDK_End:
    /* U+2198 SOUTH EAST ARROW */
    g_string_append (gstring, "\xe2\x86\x98");
    return TRUE;

  case BDK_Escape:
    /* U+238B BROKEN CIRCLE WITH NORTHWEST ARROW */
    g_string_append (gstring, "\xe2\x8e\x8b");
    return TRUE;

  case BDK_BackSpace:
    /* U+232B ERASE TO THE LEFT */
    g_string_append (gstring, "\xe2\x8c\xab");
    return TRUE;

  case BDK_Delete:
    /* U+2326 ERASE TO THE RIGHT */
    g_string_append (gstring, "\xe2\x8c\xa6");
    return TRUE;

  default:
    return FALSE;
  }
#else /* !BDK_WINDOWING_QUARTZ */
  return FALSE;
#endif
}

gchar *
_btk_accel_label_class_get_accelerator_label (BtkAccelLabelClass *klass,
					      guint               accelerator_key,
					      BdkModifierType     accelerator_mods)
{
  GString *gstring;
  gboolean seen_mod = FALSE;
  gunichar ch;
  
  gstring = g_string_new ("");
  
  if (accelerator_mods & BDK_SHIFT_MASK)
    {
      g_string_append (gstring, klass->mod_name_shift);
      seen_mod = TRUE;
    }
  if (accelerator_mods & BDK_CONTROL_MASK)
    {
      if (seen_mod)
	g_string_append (gstring, klass->mod_separator);
      g_string_append (gstring, klass->mod_name_control);
      seen_mod = TRUE;
    }
  if (accelerator_mods & BDK_MOD1_MASK)
    {
      if (seen_mod)
	g_string_append (gstring, klass->mod_separator);
      g_string_append (gstring, klass->mod_name_alt);
      seen_mod = TRUE;
    }
  if (accelerator_mods & BDK_MOD2_MASK)
    {
      if (seen_mod)
	g_string_append (gstring, klass->mod_separator);

      g_string_append (gstring, "Mod2");
      seen_mod = TRUE;
    }
  if (accelerator_mods & BDK_MOD3_MASK)
    {
      if (seen_mod)
	g_string_append (gstring, klass->mod_separator);

      g_string_append (gstring, "Mod3");
      seen_mod = TRUE;
    }
  if (accelerator_mods & BDK_MOD4_MASK)
    {
      if (seen_mod)
	g_string_append (gstring, klass->mod_separator);

      g_string_append (gstring, "Mod4");
      seen_mod = TRUE;
    }
  if (accelerator_mods & BDK_MOD5_MASK)
    {
      if (seen_mod)
	g_string_append (gstring, klass->mod_separator);

      g_string_append (gstring, "Mod5");
      seen_mod = TRUE;
    }
  if (accelerator_mods & BDK_SUPER_MASK)
    {
      if (seen_mod)
	g_string_append (gstring, klass->mod_separator);

      /* This is the text that should appear next to menu accelerators
       * that use the super key. If the text on this key isn't typically
       * translated on keyboards used for your language, don't translate
       * this.
       */
      g_string_append (gstring, C_("keyboard label", "Super"));
      seen_mod = TRUE;
    }
  if (accelerator_mods & BDK_HYPER_MASK)
    {
      if (seen_mod)
	g_string_append (gstring, klass->mod_separator);

      /* This is the text that should appear next to menu accelerators
       * that use the hyper key. If the text on this key isn't typically
       * translated on keyboards used for your language, don't translate
       * this.
       */
      g_string_append (gstring, C_("keyboard label", "Hyper"));
      seen_mod = TRUE;
    }
  if (accelerator_mods & BDK_META_MASK)
    {
      if (seen_mod)
	g_string_append (gstring, klass->mod_separator);

#ifndef BDK_WINDOWING_QUARTZ
      /* This is the text that should appear next to menu accelerators
       * that use the meta key. If the text on this key isn't typically
       * translated on keyboards used for your language, don't translate
       * this.
       */
      g_string_append (gstring, C_("keyboard label", "Meta"));
#else
      /* Command key symbol U+2318 PLACE OF INTEREST SIGN */
      g_string_append (gstring, "\xe2\x8c\x98");
#endif
      seen_mod = TRUE;
    }
  if (seen_mod)
    g_string_append (gstring, klass->mod_separator);
  
  ch = bdk_keyval_to_unicode (accelerator_key);
  if (ch && (g_unichar_isgraph (ch) || ch == ' ') &&
      (ch < 0x80 || klass->latin1_to_char))
    {
      switch (ch)
	{
	case ' ':
	  g_string_append (gstring, C_("keyboard label", "Space"));
	  break;
	case '\\':
	  g_string_append (gstring, C_("keyboard label", "Backslash"));
	  break;
	default:
	  g_string_append_unichar (gstring, g_unichar_toupper (ch));
	  break;
	}
    }
  else if (!append_keyval_symbol (accelerator_key, gstring))
    {
      gchar *tmp;

      tmp = bdk_keyval_name (bdk_keyval_to_lower (accelerator_key));
      if (tmp != NULL)
	{
	  if (tmp[0] != 0 && tmp[1] == 0)
	    g_string_append_c (gstring, g_ascii_toupper (tmp[0]));
	  else
	    {
	      const gchar *str;
              str = g_dpgettext2 (GETTEXT_PACKAGE, "keyboard label", tmp);
	      if (str == tmp)
		{
		  g_string_append (gstring, tmp);
		  substitute_underscores (gstring->str);
		}
	      else
		g_string_append (gstring, str);
	    }
	}
    }

  return g_string_free (gstring, FALSE);
}

/**
 * btk_accel_label_refetch:
 * @accel_label: a #BtkAccelLabel.
 *
 * Recreates the string representing the accelerator keys.
 * This should not be needed since the string is automatically updated whenever
 * accelerators are added or removed from the associated widget.
 *
 * Returns: always returns %FALSE.
 */
gboolean
btk_accel_label_refetch (BtkAccelLabel *accel_label)
{
  gboolean enable_accels;

  g_return_val_if_fail (BTK_IS_ACCEL_LABEL (accel_label), FALSE);

  if (accel_label->accel_string)
    {
      g_free (accel_label->accel_string);
      accel_label->accel_string = NULL;
    }

  g_object_get (btk_widget_get_settings (BTK_WIDGET (accel_label)),
                "btk-enable-accels", &enable_accels,
                NULL);

  if (enable_accels && accel_label->accel_closure)
    {
      BtkAccelKey *key = btk_accel_group_find (accel_label->accel_group, find_accel, accel_label->accel_closure);

      if (key && key->accel_flags & BTK_ACCEL_VISIBLE)
	{
	  BtkAccelLabelClass *klass;
	  gchar *tmp;

	  klass = BTK_ACCEL_LABEL_GET_CLASS (accel_label);
	  tmp = _btk_accel_label_class_get_accelerator_label (klass,
							      key->accel_key,
							      key->accel_mods);
	  accel_label->accel_string = g_strconcat ("   ", tmp, NULL);
	  g_free (tmp);
	}
      if (!accel_label->accel_string)
	accel_label->accel_string = g_strdup ("-/-");
    }
  
  if (!accel_label->accel_string)
    accel_label->accel_string = g_strdup ("");

  btk_widget_queue_resize (BTK_WIDGET (accel_label));

  return FALSE;
}

#define __BTK_ACCEL_LABEL_C__
#include "btkaliasdef.c"
