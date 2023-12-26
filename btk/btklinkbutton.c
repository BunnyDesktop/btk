/* BTK - The GIMP Toolkit
 * btklinkbutton.c - an hyperlink-enabled button
 * 
 * Copyright (C) 2006 Emmanuele Bassi <ebassi@gmail.com>
 * All rights reserved.
 *
 * Based on bunny-href code by:
 * 	James Henstridge <james@daa.com.au>
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Cambridge, MA 02139, USA.
 */

#include "config.h"

#include <string.h>

#include "btkclipboard.h"
#include "btkdnd.h"
#include "btkimagemenuitem.h"
#include "btklabel.h"
#include "btkmain.h"
#include "btkmenu.h"
#include "btkmenuitem.h"
#include "btkstock.h"
#include "btkshow.h"
#include "btktooltip.h"
#include "btklinkbutton.h"
#include "btkprivate.h"

#include "btkintl.h"
#include "btkalias.h"


struct _BtkLinkButtonPrivate
{
  bchar *uri;

  bboolean visited;

  BtkWidget *popup_menu;
};

enum
{
  PROP_0,
  PROP_URI,
  PROP_VISITED
};

#define BTK_LINK_BUTTON_GET_PRIVATE(obj)	(B_TYPE_INSTANCE_GET_PRIVATE ((obj), BTK_TYPE_LINK_BUTTON, BtkLinkButtonPrivate))

static void     btk_link_button_finalize     (BObject          *object);
static void     btk_link_button_get_property (BObject          *object,
					      buint             prop_id,
					      BValue           *value,
					      BParamSpec       *pspec);
static void     btk_link_button_set_property (BObject          *object,
					      buint             prop_id,
					      const BValue     *value,
					      BParamSpec       *pspec);
static void     btk_link_button_add          (BtkContainer     *container,
					      BtkWidget        *widget);
static bboolean btk_link_button_button_press (BtkWidget        *widget,
					      BdkEventButton   *event);
static void     btk_link_button_clicked      (BtkButton        *button);
static bboolean btk_link_button_popup_menu   (BtkWidget        *widget);
static void     btk_link_button_style_set    (BtkWidget        *widget,
					      BtkStyle         *old_style);
static bboolean btk_link_button_enter_cb     (BtkWidget        *widget,
					      BdkEventCrossing *event,
					      bpointer          user_data);
static bboolean btk_link_button_leave_cb     (BtkWidget        *widget,
					      BdkEventCrossing *event,
					      bpointer          user_data);
static void btk_link_button_drag_data_get_cb (BtkWidget        *widget,
					      BdkDragContext   *context,
					      BtkSelectionData *selection,
					      buint             _info,
					      buint             _time,
					      bpointer          user_data);
static bboolean btk_link_button_query_tooltip_cb (BtkWidget    *widget,
                                                  bint          x,
                                                  bint          y,
                                                  bboolean      keyboard_tip,
                                                  BtkTooltip   *tooltip,
                                                  bpointer      data);


static const BtkTargetEntry link_drop_types[] = {
  { "text/uri-list", 0, 0 },
  { "_NETSCAPE_URL", 0, 0 }
};

static const BdkColor default_link_color = { 0, 0, 0, 0xeeee };
static const BdkColor default_visited_link_color = { 0, 0x5555, 0x1a1a, 0x8b8b };

static BtkLinkButtonUriFunc uri_func = NULL;
static bpointer uri_func_data = NULL;
static GDestroyNotify uri_func_destroy = NULL;

G_DEFINE_TYPE (BtkLinkButton, btk_link_button, BTK_TYPE_BUTTON)

static void
btk_link_button_class_init (BtkLinkButtonClass *klass)
{
  BObjectClass *bobject_class = B_OBJECT_CLASS (klass);
  BtkWidgetClass *widget_class = BTK_WIDGET_CLASS (klass);
  BtkContainerClass *container_class = BTK_CONTAINER_CLASS (klass);
  BtkButtonClass *button_class = BTK_BUTTON_CLASS (klass);
  
  bobject_class->set_property = btk_link_button_set_property;
  bobject_class->get_property = btk_link_button_get_property;
  bobject_class->finalize = btk_link_button_finalize;
  
  widget_class->button_press_event = btk_link_button_button_press;
  widget_class->popup_menu = btk_link_button_popup_menu;
  widget_class->style_set = btk_link_button_style_set;
  
  container_class->add = btk_link_button_add;

  button_class->clicked = btk_link_button_clicked;

  /**
   * BtkLinkButton:uri
   * 
   * The URI bound to this button. 
   *
   * Since: 2.10
   */
  g_object_class_install_property (bobject_class,
  				   PROP_URI,
  				   g_param_spec_string ("uri",
  				   			P_("URI"),
  				   			P_("The URI bound to this button"),
  				   			NULL,
  				   			G_PARAM_READWRITE));
  /**
   * BtkLinkButton:visited
   * 
   * The 'visited' state of this button. A visited link is drawn in a
   * different color.
   *
   * Since: 2.14
   */
  g_object_class_install_property (bobject_class,
  				   PROP_VISITED,
  				   g_param_spec_boolean ("visited",
                                                         P_("Visited"),
                                                         P_("Whether this link has been visited."),
                                                         FALSE,
                                                         G_PARAM_READWRITE));
  
  g_type_class_add_private (bobject_class, sizeof (BtkLinkButtonPrivate));
}

static void
btk_link_button_init (BtkLinkButton *link_button)
{
  link_button->priv = BTK_LINK_BUTTON_GET_PRIVATE (link_button),
  
  btk_button_set_relief (BTK_BUTTON (link_button), BTK_RELIEF_NONE);
  
  g_signal_connect (link_button, "enter-notify-event",
  		    G_CALLBACK (btk_link_button_enter_cb), NULL);
  g_signal_connect (link_button, "leave-notify-event",
  		    G_CALLBACK (btk_link_button_leave_cb), NULL);
  g_signal_connect (link_button, "drag-data-get",
  		    G_CALLBACK (btk_link_button_drag_data_get_cb), NULL);

  g_object_set (link_button, "has-tooltip", TRUE, NULL);
  g_signal_connect (link_button, "query-tooltip",
                    G_CALLBACK (btk_link_button_query_tooltip_cb), NULL);
  
  /* enable drag source */
  btk_drag_source_set (BTK_WIDGET (link_button),
  		       BDK_BUTTON1_MASK,
  		       link_drop_types, G_N_ELEMENTS (link_drop_types),
  		       BDK_ACTION_COPY);
}

static void
btk_link_button_finalize (BObject *object)
{
  BtkLinkButton *link_button = BTK_LINK_BUTTON (object);
  
  g_free (link_button->priv->uri);
  
  B_OBJECT_CLASS (btk_link_button_parent_class)->finalize (object);
}

static void
btk_link_button_get_property (BObject    *object,
			      buint       prop_id,
			      BValue     *value,
			      BParamSpec *pspec)
{
  BtkLinkButton *link_button = BTK_LINK_BUTTON (object);
  
  switch (prop_id)
    {
    case PROP_URI:
      b_value_set_string (value, link_button->priv->uri);
      break;
    case PROP_VISITED:
      b_value_set_boolean (value, link_button->priv->visited);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_link_button_set_property (BObject      *object,
			      buint         prop_id,
			      const BValue *value,
			      BParamSpec   *pspec)
{
  BtkLinkButton *link_button = BTK_LINK_BUTTON (object);
  
  switch (prop_id)
    {
    case PROP_URI:
      btk_link_button_set_uri (link_button, b_value_get_string (value));
      break;
    case PROP_VISITED:
      btk_link_button_set_visited (link_button, b_value_get_boolean (value));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
set_link_color (BtkLinkButton *link_button)
{
  BdkColor *link_color = NULL;
  BtkWidget *label;

  label = btk_bin_get_child (BTK_BIN (link_button));
  if (!BTK_IS_LABEL (label))
    return;

  if (link_button->priv->visited)
    {
      btk_widget_style_get (BTK_WIDGET (link_button),
			    "visited-link-color", &link_color, NULL);
      if (!link_color)
	link_color = (BdkColor *) &default_visited_link_color;
    }
  else
    {
      btk_widget_style_get (BTK_WIDGET (link_button),
			    "link-color", &link_color, NULL);
      if (!link_color)
	link_color = (BdkColor *) &default_link_color;
    }

  btk_widget_modify_fg (label, BTK_STATE_NORMAL, link_color);
  btk_widget_modify_fg (label, BTK_STATE_ACTIVE, link_color);
  btk_widget_modify_fg (label, BTK_STATE_PRELIGHT, link_color);
  btk_widget_modify_fg (label, BTK_STATE_SELECTED, link_color);

  if (link_color != &default_link_color &&
      link_color != &default_visited_link_color)
    bdk_color_free (link_color);
}

static void
set_link_underline (BtkLinkButton *link_button)
{
  BtkWidget *label;
  
  label = btk_bin_get_child (BTK_BIN (link_button));
  if (BTK_IS_LABEL (label))
    {
      BangoAttrList *attributes;
      BangoAttribute *uline;

      uline = bango_attr_underline_new (BANGO_UNDERLINE_SINGLE);
      uline->start_index = 0;
      uline->end_index = B_MAXUINT;
      attributes = bango_attr_list_new ();
      bango_attr_list_insert (attributes, uline); 
      btk_label_set_attributes (BTK_LABEL (label), attributes);
      bango_attr_list_unref (attributes);
    }
}

static void
btk_link_button_add (BtkContainer *container,
		     BtkWidget    *widget)
{
  BTK_CONTAINER_CLASS (btk_link_button_parent_class)->add (container, widget);

  set_link_color (BTK_LINK_BUTTON (container));
  set_link_underline (BTK_LINK_BUTTON (container));
}

static void
btk_link_button_style_set (BtkWidget *widget,
			   BtkStyle  *old_style)
{
  BtkLinkButton *link_button = BTK_LINK_BUTTON (widget);

  set_link_color (link_button);
}

static void
set_hand_cursor (BtkWidget *widget,
		 bboolean   show_hand)
{
  BdkDisplay *display;
  BdkCursor *cursor;

  display = btk_widget_get_display (widget);

  cursor = NULL;
  if (show_hand)
    cursor = bdk_cursor_new_for_display (display, BDK_HAND2);

  bdk_window_set_cursor (widget->window, cursor);
  bdk_display_flush (display);

  if (cursor)
    bdk_cursor_unref (cursor);
}

static void
popup_menu_detach (BtkWidget *attach_widget,
		   BtkMenu   *menu)
{
  BtkLinkButton *link_button = BTK_LINK_BUTTON (attach_widget);

  link_button->priv->popup_menu = NULL;
}

static void
popup_position_func (BtkMenu  *menu,
		     bint     *x,
		     bint     *y,
		     bboolean *push_in,
		     bpointer  user_data)
{
  BtkLinkButton *link_button = BTK_LINK_BUTTON (user_data);
  BtkLinkButtonPrivate *priv = link_button->priv;
  BtkWidget *widget = BTK_WIDGET (link_button);
  BdkScreen *screen = btk_widget_get_screen (widget);
  BtkRequisition req;
  bint monitor_num;
  BdkRectangle monitor;
  
  g_return_if_fail (btk_widget_get_realized (widget));

  bdk_window_get_origin (widget->window, x, y);

  btk_widget_size_request (priv->popup_menu, &req);

  *x += widget->allocation.width / 2;
  *y += widget->allocation.height;

  monitor_num = bdk_screen_get_monitor_at_point (screen, *x, *y);
  btk_menu_set_monitor (menu, monitor_num);
  bdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);

  *x = CLAMP (*x, monitor.x, monitor.x + MAX (0, monitor.width - req.width));
  *y = CLAMP (*y, monitor.y, monitor.y + MAX (0, monitor.height - req.height));

  *push_in = FALSE;
}

static void
copy_activate_cb (BtkWidget     *widget,
		  BtkLinkButton *link_button)
{
  BtkLinkButtonPrivate *priv = link_button->priv;
  
  btk_clipboard_set_text (btk_widget_get_clipboard (BTK_WIDGET (link_button),
			  			    BDK_SELECTION_CLIPBOARD),
		  	  priv->uri, -1);
}

static void
btk_link_button_do_popup (BtkLinkButton  *link_button,
			  BdkEventButton *event)
{
  BtkLinkButtonPrivate *priv = link_button->priv;
  bint button;
  buint time;
  
  if (event)
    {
      button = event->button;
      time = event->time;
    }
  else
    {
      button = 0;
      time = btk_get_current_event_time ();
    }

  if (btk_widget_get_realized (BTK_WIDGET (link_button)))
    {
      BtkWidget *menu_item;
      
      if (priv->popup_menu)
	btk_widget_destroy (priv->popup_menu);

      priv->popup_menu = btk_menu_new ();
      
      btk_menu_attach_to_widget (BTK_MENU (priv->popup_menu),
		      		 BTK_WIDGET (link_button),
				 popup_menu_detach);

      menu_item = btk_image_menu_item_new_with_mnemonic (_("Copy URL"));
      btk_image_menu_item_set_image (BTK_IMAGE_MENU_ITEM (menu_item),
		      		     btk_image_new_from_stock (BTK_STOCK_COPY,
							       BTK_ICON_SIZE_MENU));
      g_signal_connect (menu_item, "activate",
		        G_CALLBACK (copy_activate_cb), link_button);
      btk_widget_show (menu_item);
      btk_menu_shell_append (BTK_MENU_SHELL (priv->popup_menu), menu_item);
      
      if (button)
        btk_menu_popup (BTK_MENU (priv->popup_menu), NULL, NULL,
		        NULL, NULL,
			button, time);
      else
        {
          btk_menu_popup (BTK_MENU (priv->popup_menu), NULL, NULL,
			  popup_position_func, link_button,
			  button, time);
	  btk_menu_shell_select_first (BTK_MENU_SHELL (priv->popup_menu), FALSE);
	}
    }
}

static bboolean
btk_link_button_button_press (BtkWidget      *widget,
			      BdkEventButton *event)
{
  if (!btk_widget_has_focus (widget))
    btk_widget_grab_focus (widget);

  if (_btk_button_event_triggers_context_menu (event))
    {
      btk_link_button_do_popup (BTK_LINK_BUTTON (widget), event);
      
      return TRUE;
    }

  if (BTK_WIDGET_CLASS (btk_link_button_parent_class)->button_press_event)
    return BTK_WIDGET_CLASS (btk_link_button_parent_class)->button_press_event (widget, event);
  
  return FALSE;
}

static void
btk_link_button_clicked (BtkButton *button)
{
  BtkLinkButton *link_button = BTK_LINK_BUTTON (button);

  if (uri_func)
    (* uri_func) (link_button, link_button->priv->uri, uri_func_data);
  else
    {
      BdkScreen *screen;
      GError *error;

      if (btk_widget_has_screen (BTK_WIDGET (button)))
        screen = btk_widget_get_screen (BTK_WIDGET (button));
      else
        screen = NULL;

      error = NULL;
      btk_show_uri (screen, link_button->priv->uri, BDK_CURRENT_TIME, &error);
      if (error)
        {
          g_warning ("Unable to show '%s': %s",
                     link_button->priv->uri,
                     error->message);
          g_error_free (error);
        }
    }

  btk_link_button_set_visited (link_button, TRUE);
}

static bboolean
btk_link_button_popup_menu (BtkWidget *widget)
{
  btk_link_button_do_popup (BTK_LINK_BUTTON (widget), NULL);

  return TRUE; 
}

static bboolean
btk_link_button_enter_cb (BtkWidget        *widget,
			  BdkEventCrossing *crossing,
			  bpointer          user_data)
{
  set_hand_cursor (widget, TRUE);
  
  return FALSE;
}

static bboolean
btk_link_button_leave_cb (BtkWidget        *widget,
			  BdkEventCrossing *crossing,
			  bpointer          user_data)
{
  set_hand_cursor (widget, FALSE);
  
  return FALSE;
}

static void
btk_link_button_drag_data_get_cb (BtkWidget        *widget,
				  BdkDragContext   *context,
				  BtkSelectionData *selection,
				  buint             _info,
				  buint             _time,
				  bpointer          user_data)
{
  BtkLinkButton *link_button = BTK_LINK_BUTTON (widget);
  bchar *uri;
  
  uri = g_strdup_printf ("%s\r\n", link_button->priv->uri);
  btk_selection_data_set (selection,
  			  selection->target,
  			  8,
  			  (buchar *) uri,
			  strlen (uri));
  
  g_free (uri);
}

/**
 * btk_link_button_new:
 * @uri: a valid URI
 *
 * Creates a new #BtkLinkButton with the URI as its text.
 *
 * Return value: a new link button widget.
 *
 * Since: 2.10
 */
BtkWidget *
btk_link_button_new (const bchar *uri)
{
  bchar *utf8_uri = NULL;
  BtkWidget *retval;
  
  g_return_val_if_fail (uri != NULL, NULL);
  
  if (g_utf8_validate (uri, -1, NULL))
    {
      utf8_uri = g_strdup (uri);
    }
  else
    {
      GError *conv_err = NULL;
    
      utf8_uri = g_locale_to_utf8 (uri, -1, NULL, NULL, &conv_err);
      if (conv_err)
        {
          g_warning ("Attempting to convert URI `%s' to UTF-8, but failed "
                     "with error: %s\n",
                     uri,
                     conv_err->message);
          g_error_free (conv_err);
        
          utf8_uri = g_strdup (_("Invalid URI"));
        }
    }
  
  retval = g_object_new (BTK_TYPE_LINK_BUTTON,
  			 "label", utf8_uri,
  			 "uri", uri,
  			 NULL);
  
  g_free (utf8_uri);
  
  return retval;
}

/**
 * btk_link_button_new_with_label:
 * @uri: a valid URI
 * @label: (allow-none): the text of the button
 *
 * Creates a new #BtkLinkButton containing a label.
 *
 * Return value: (transfer none): a new link button widget.
 *
 * Since: 2.10
 */
BtkWidget *
btk_link_button_new_with_label (const bchar *uri,
				const bchar *label)
{
  BtkWidget *retval;
  
  g_return_val_if_fail (uri != NULL, NULL);
  
  if (!label)
    return btk_link_button_new (uri);

  retval = g_object_new (BTK_TYPE_LINK_BUTTON,
		         "label", label,
			 "uri", uri,
			 NULL);

  return retval;
}

static bboolean 
btk_link_button_query_tooltip_cb (BtkWidget    *widget,
                                  bint          x,
                                  bint          y,
                                  bboolean      keyboard_tip,
                                  BtkTooltip   *tooltip,
                                  bpointer      data)
{
  BtkLinkButton *link_button = BTK_LINK_BUTTON (widget);
  const bchar *label, *uri;

  label = btk_button_get_label (BTK_BUTTON (link_button));
  uri = link_button->priv->uri;

  if (!btk_widget_get_tooltip_text (widget)
    && !btk_widget_get_tooltip_markup (widget)
    && label && *label != '\0' && uri && strcmp (label, uri) != 0)
    {
      btk_tooltip_set_text (tooltip, uri);
      return TRUE;
    }

  return FALSE;
}


/**
 * btk_link_button_set_uri:
 * @link_button: a #BtkLinkButton
 * @uri: a valid URI
 *
 * Sets @uri as the URI where the #BtkLinkButton points. As a side-effect
 * this unsets the 'visited' state of the button.
 *
 * Since: 2.10
 */
void
btk_link_button_set_uri (BtkLinkButton *link_button,
			 const bchar   *uri)
{
  BtkLinkButtonPrivate *priv;

  g_return_if_fail (BTK_IS_LINK_BUTTON (link_button));
  g_return_if_fail (uri != NULL);

  priv = link_button->priv;

  g_free (priv->uri);
  priv->uri = g_strdup (uri);

  g_object_notify (B_OBJECT (link_button), "uri");

  btk_link_button_set_visited (link_button, FALSE);
}

/**
 * btk_link_button_get_uri:
 * @link_button: a #BtkLinkButton
 *
 * Retrieves the URI set using btk_link_button_set_uri().
 *
 * Return value: a valid URI.  The returned string is owned by the link button
 *   and should not be modified or freed.
 *
 * Since: 2.10
 */
const bchar *
btk_link_button_get_uri (BtkLinkButton *link_button)
{
  g_return_val_if_fail (BTK_IS_LINK_BUTTON (link_button), NULL);
  
  return link_button->priv->uri;
}

/**
 * btk_link_button_set_uri_hook:
 * @func: (allow-none): a function called each time a #BtkLinkButton is clicked, or %NULL
 * @data: (allow-none): user data to be passed to @func, or %NULL
 * @destroy: (allow-none): a #GDestroyNotify that gets called when @data is no longer needed, or %NULL
 *
 * Sets @func as the function that should be invoked every time a user clicks
 * a #BtkLinkButton. This function is called before every callback registered
 * for the "clicked" signal.
 *
 * If no uri hook has been set, BTK+ defaults to calling btk_show_uri().
 *
 * Return value: the previously set hook function.
 *
 * Since: 2.10
 *
 * Deprecated: 2.24: Use the #BtkButton::clicked signal instead
 */
BtkLinkButtonUriFunc
btk_link_button_set_uri_hook (BtkLinkButtonUriFunc func,
			      bpointer             data,
			      GDestroyNotify       destroy)
{
  BtkLinkButtonUriFunc old_uri_func;

  if (uri_func_destroy)
    (* uri_func_destroy) (uri_func_data);

  old_uri_func = uri_func;

  uri_func = func;
  uri_func_data = data;
  uri_func_destroy = destroy;

  return old_uri_func;
}

/**
 * btk_link_button_set_visited:
 * @link_button: a #BtkLinkButton
 * @visited: the new 'visited' state
 *
 * Sets the 'visited' state of the URI where the #BtkLinkButton
 * points.  See btk_link_button_get_visited() for more details.
 *
 * Since: 2.14
 */
void
btk_link_button_set_visited (BtkLinkButton *link_button,
                             bboolean       visited)
{
  g_return_if_fail (BTK_IS_LINK_BUTTON (link_button));

  visited = visited != FALSE;

  if (link_button->priv->visited != visited)
    {
      link_button->priv->visited = visited;

      set_link_color (link_button);

      g_object_notify (B_OBJECT (link_button), "visited");
    }
}

/**
 * btk_link_button_get_visited:
 * @link_button: a #BtkLinkButton
 *
 * Retrieves the 'visited' state of the URI where the #BtkLinkButton
 * points. The button becomes visited when it is clicked. If the URI
 * is changed on the button, the 'visited' state is unset again.
 *
 * The state may also be changed using btk_link_button_set_visited().
 *
 * Return value: %TRUE if the link has been visited, %FALSE otherwise
 *
 * Since: 2.14
 */
bboolean
btk_link_button_get_visited (BtkLinkButton *link_button)
{
  g_return_val_if_fail (BTK_IS_LINK_BUTTON (link_button), FALSE);
  
  return link_button->priv->visited;
}


#define __BTK_LINK_BUTTON_C__
#include "btkaliasdef.c"
