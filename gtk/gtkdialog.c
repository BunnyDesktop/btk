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

#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "btkbutton.h"
#include "btkdialog.h"
#include "btkhbbox.h"
#include "btklabel.h"
#include "btkhseparator.h"
#include "btkmarshalers.h"
#include "btkvbox.h"
#include "bdkkeysyms.h"
#include "btkmain.h"
#include "btkintl.h"
#include "btkbindings.h"
#include "btkprivate.h"
#include "btkbuildable.h"
#include "btkalias.h"

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), BTK_TYPE_DIALOG, BtkDialogPrivate))

typedef struct {
  guint ignore_separator : 1;
} BtkDialogPrivate;

typedef struct _ResponseData ResponseData;

struct _ResponseData
{
  gint response_id;
};

static void      btk_dialog_add_buttons_valist   (BtkDialog    *dialog,
                                                  const gchar  *first_button_text,
                                                  va_list       args);

static gboolean  btk_dialog_delete_event_handler (BtkWidget    *widget,
                                                  BdkEventAny  *event,
                                                  gpointer      user_data);

static void      btk_dialog_set_property         (GObject      *object,
                                                  guint         prop_id,
                                                  const GValue *value,
                                                  GParamSpec   *pspec);
static void      btk_dialog_get_property         (GObject      *object,
                                                  guint         prop_id,
                                                  GValue       *value,
                                                  GParamSpec   *pspec);
static void      btk_dialog_style_set            (BtkWidget    *widget,
                                                  BtkStyle     *prev_style);
static void      btk_dialog_map                  (BtkWidget    *widget);

static void      btk_dialog_close                (BtkDialog    *dialog);

static ResponseData * get_response_data          (BtkWidget    *widget,
                                                  gboolean      create);

static void      btk_dialog_buildable_interface_init     (BtkBuildableIface *iface);
static GObject * btk_dialog_buildable_get_internal_child (BtkBuildable  *buildable,
                                                          BtkBuilder    *builder,
                                                          const gchar   *childname);
static gboolean  btk_dialog_buildable_custom_tag_start   (BtkBuildable  *buildable,
                                                          BtkBuilder    *builder,
                                                          GObject       *child,
                                                          const gchar   *tagname,
                                                          GMarkupParser *parser,
                                                          gpointer      *data);
static void      btk_dialog_buildable_custom_finished    (BtkBuildable  *buildable,
                                                          BtkBuilder    *builder,
                                                          GObject       *child,
                                                          const gchar   *tagname,
                                                          gpointer       user_data);


enum {
  PROP_0,
  PROP_HAS_SEPARATOR
};

enum {
  RESPONSE,
  CLOSE,
  LAST_SIGNAL
};

static guint dialog_signals[LAST_SIGNAL];

G_DEFINE_TYPE_WITH_CODE (BtkDialog, btk_dialog, BTK_TYPE_WINDOW,
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_BUILDABLE,
						btk_dialog_buildable_interface_init))

static void
btk_dialog_class_init (BtkDialogClass *class)
{
  GObjectClass *bobject_class;
  BtkWidgetClass *widget_class;
  BtkBindingSet *binding_set;
  
  bobject_class = G_OBJECT_CLASS (class);
  widget_class = BTK_WIDGET_CLASS (class);
  
  bobject_class->set_property = btk_dialog_set_property;
  bobject_class->get_property = btk_dialog_get_property;
  
  widget_class->map = btk_dialog_map;
  widget_class->style_set = btk_dialog_style_set;

  class->close = btk_dialog_close;
  
  g_type_class_add_private (bobject_class, sizeof (BtkDialogPrivate));

  /**
   * BtkDialog:has-separator:
   *
   * When %TRUE, the dialog has a separator bar above its buttons.
   *
   * Deprecated: 2.22: This property will be removed in BTK+ 3.
   */
  g_object_class_install_property (bobject_class,
                                   PROP_HAS_SEPARATOR,
                                   g_param_spec_boolean ("has-separator",
							 P_("Has separator"),
							 P_("The dialog has a separator bar above its buttons"),
                                                         FALSE,
                                                         BTK_PARAM_READWRITE | G_PARAM_DEPRECATED));

  /**
   * BtkDialog::response:
   * @dialog: the object on which the signal is emitted
   * @response_id: the response ID
   * 
   * Emitted when an action widget is clicked, the dialog receives a 
   * delete event, or the application programmer calls btk_dialog_response(). 
   * On a delete event, the response ID is #BTK_RESPONSE_DELETE_EVENT. 
   * Otherwise, it depends on which action widget was clicked.
   */
  dialog_signals[RESPONSE] =
    g_signal_new (I_("response"),
		  G_OBJECT_CLASS_TYPE (class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkDialogClass, response),
		  NULL, NULL,
		  _btk_marshal_VOID__INT,
		  G_TYPE_NONE, 1,
		  G_TYPE_INT);

  /**
   * BtkDialog::close:
   *
   * The ::close signal is a 
   * <link linkend="keybinding-signals">keybinding signal</link>
   * which gets emitted when the user uses a keybinding to close
   * the dialog.
   *
   * The default binding for this signal is the Escape key.
   */ 
  dialog_signals[CLOSE] =
    g_signal_new (I_("close"),
		  G_OBJECT_CLASS_TYPE (class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkDialogClass, close),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  
  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("content-area-border",
                                                             P_("Content area border"),
                                                             P_("Width of border around the main dialog area"),
                                                             0,
                                                             G_MAXINT,
                                                             2,
                                                             BTK_PARAM_READABLE));
  /**
   * BtkDialog:content-area-spacing:
   *
   * The default spacing used between elements of the
   * content area of the dialog, as returned by
   * btk_dialog_get_content_area(), unless btk_box_set_spacing()
   * was called on that widget directly.
   *
   * Since: 2.16
   */
  btk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("content-area-spacing",
                                                             P_("Content area spacing"),
                                                             P_("Spacing between elements of the main dialog area"),
                                                             0,
                                                             G_MAXINT,
                                                             0,
                                                             BTK_PARAM_READABLE));
  btk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("button-spacing",
                                                             P_("Button spacing"),
                                                             P_("Spacing between buttons"),
                                                             0,
                                                             G_MAXINT,
                                                             6,
                                                             BTK_PARAM_READABLE));
  
  btk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("action-area-border",
                                                             P_("Action area border"),
                                                             P_("Width of border around the button area at the bottom of the dialog"),
                                                             0,
                                                             G_MAXINT,
                                                             5,
                                                             BTK_PARAM_READABLE));

  binding_set = btk_binding_set_by_class (class);
  
  btk_binding_entry_add_signal (binding_set, BDK_Escape, 0, "close", 0);
}

static void
update_spacings (BtkDialog *dialog)
{
  gint content_area_border;
  gint content_area_spacing;
  gint button_spacing;
  gint action_area_border;

  btk_widget_style_get (BTK_WIDGET (dialog),
                        "content-area-border", &content_area_border,
                        "content-area-spacing", &content_area_spacing,
                        "button-spacing", &button_spacing,
                        "action-area-border", &action_area_border,
                        NULL);

  btk_container_set_border_width (BTK_CONTAINER (dialog->vbox),
                                  content_area_border);
  if (!_btk_box_get_spacing_set (BTK_BOX (dialog->vbox)))
    {
      btk_box_set_spacing (BTK_BOX (dialog->vbox), content_area_spacing);
      _btk_box_set_spacing_set (BTK_BOX (dialog->vbox), FALSE);
    }
  btk_box_set_spacing (BTK_BOX (dialog->action_area),
                       button_spacing);
  btk_container_set_border_width (BTK_CONTAINER (dialog->action_area),
                                  action_area_border);
}

static void
btk_dialog_init (BtkDialog *dialog)
{
  BtkDialogPrivate *priv;

  priv = GET_PRIVATE (dialog);
  priv->ignore_separator = FALSE;

  /* To avoid breaking old code that prevents destroy on delete event
   * by connecting a handler, we have to have the FIRST signal
   * connection on the dialog.
   */
  g_signal_connect (dialog,
                    "delete-event",
                    G_CALLBACK (btk_dialog_delete_event_handler),
                    NULL);

  dialog->vbox = btk_vbox_new (FALSE, 0);

  btk_container_add (BTK_CONTAINER (dialog), dialog->vbox);
  btk_widget_show (dialog->vbox);

  dialog->action_area = btk_hbutton_box_new ();

  btk_button_box_set_layout (BTK_BUTTON_BOX (dialog->action_area),
                             BTK_BUTTONBOX_END);

  btk_box_pack_end (BTK_BOX (dialog->vbox), dialog->action_area,
                    FALSE, TRUE, 0);
  btk_widget_show (dialog->action_area);

  dialog->separator = NULL;

  btk_window_set_type_hint (BTK_WINDOW (dialog),
                            BDK_WINDOW_TYPE_HINT_DIALOG);
  btk_window_set_position (BTK_WINDOW (dialog), BTK_WIN_POS_CENTER_ON_PARENT);
}

static BtkBuildableIface *parent_buildable_iface;

static void
btk_dialog_buildable_interface_init (BtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->get_internal_child = btk_dialog_buildable_get_internal_child;
  iface->custom_tag_start = btk_dialog_buildable_custom_tag_start;
  iface->custom_finished = btk_dialog_buildable_custom_finished;
}

static GObject *
btk_dialog_buildable_get_internal_child (BtkBuildable *buildable,
					 BtkBuilder   *builder,
					 const gchar  *childname)
{
    if (strcmp (childname, "vbox") == 0)
      return G_OBJECT (BTK_DIALOG (buildable)->vbox);
    else if (strcmp (childname, "action_area") == 0)
      return G_OBJECT (BTK_DIALOG (buildable)->action_area);

    return parent_buildable_iface->get_internal_child (buildable,
						       builder,
						       childname);
}

static void 
btk_dialog_set_property (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  BtkDialog *dialog;
  
  dialog = BTK_DIALOG (object);
  
  switch (prop_id)
    {
    case PROP_HAS_SEPARATOR:
      btk_dialog_set_has_separator (dialog, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void 
btk_dialog_get_property (GObject     *object,
                         guint        prop_id,
                         GValue      *value,
                         GParamSpec  *pspec)
{
  BtkDialog *dialog;
  
  dialog = BTK_DIALOG (object);
  
  switch (prop_id)
    {
    case PROP_HAS_SEPARATOR:
      g_value_set_boolean (value, dialog->separator != NULL);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static gboolean
btk_dialog_delete_event_handler (BtkWidget   *widget,
                                 BdkEventAny *event,
                                 gpointer     user_data)
{
  /* emit response signal */
  btk_dialog_response (BTK_DIALOG (widget), BTK_RESPONSE_DELETE_EVENT);

  /* Do the destroy by default */
  return FALSE;
}

/* A far too tricky heuristic for getting the right initial
 * focus widget if none was set. What we do is we focus the first
 * widget in the tab chain, but if this results in the focus
 * ending up on one of the response widgets _other_ than the
 * default response, we focus the default response instead.
 *
 * Additionally, skip selectable labels when looking for the
 * right initial focus widget.
 */
static void
btk_dialog_map (BtkWidget *widget)
{
  BtkWindow *window = BTK_WINDOW (widget);
  BtkDialog *dialog = BTK_DIALOG (widget);
  
  BTK_WIDGET_CLASS (btk_dialog_parent_class)->map (widget);

  if (!window->focus_widget)
    {
      GList *children, *tmp_list;
      BtkWidget *first_focus = NULL;
      
      do 
	{
	  g_signal_emit_by_name (window, "move_focus", BTK_DIR_TAB_FORWARD);

	  if (first_focus == NULL)
	    first_focus = window->focus_widget;
	  else if (first_focus == window->focus_widget)
            break;
	  if (!BTK_IS_LABEL (window->focus_widget))
	    break;
          if (!btk_label_get_current_uri (BTK_LABEL (window->focus_widget)))
            btk_label_select_rebunnyion (BTK_LABEL (window->focus_widget), 0, 0);
	}
      while (TRUE);

      tmp_list = children = btk_container_get_children (BTK_CONTAINER (dialog->action_area));
      
      while (tmp_list)
	{
	  BtkWidget *child = tmp_list->data;
	  
	  if ((window->focus_widget == NULL || 
	       child == window->focus_widget) && 
	      child != window->default_widget &&
	      window->default_widget)
	    {
	      btk_widget_grab_focus (window->default_widget);
	      break;
	    }
	  
	  tmp_list = tmp_list->next;
	}
      
      g_list_free (children);
    }
}

static void
btk_dialog_style_set (BtkWidget *widget,
                      BtkStyle  *prev_style)
{
  update_spacings (BTK_DIALOG (widget));
}

static BtkWidget *
dialog_find_button (BtkDialog *dialog,
		    gint       response_id)
{
  GList *children, *tmp_list;
  BtkWidget *child = NULL;
      
  children = btk_container_get_children (BTK_CONTAINER (dialog->action_area));

  for (tmp_list = children; tmp_list; tmp_list = tmp_list->next)
    {
      ResponseData *rd = get_response_data (tmp_list->data, FALSE);
      
      if (rd && rd->response_id == response_id)
	{
	  child = tmp_list->data;
	  break;
	}
    }

  g_list_free (children);

  return child;
}

static void
btk_dialog_close (BtkDialog *dialog)
{
  /* Synthesize delete_event to close dialog. */
  
  BtkWidget *widget = BTK_WIDGET (dialog);
  BdkEvent *event;

  event = bdk_event_new (BDK_DELETE);
  
  event->any.window = g_object_ref (widget->window);
  event->any.send_event = TRUE;
  
  btk_main_do_event (event);
  bdk_event_free (event);
}

BtkWidget*
btk_dialog_new (void)
{
  return g_object_new (BTK_TYPE_DIALOG, NULL);
}

static BtkWidget*
btk_dialog_new_empty (const gchar     *title,
                      BtkWindow       *parent,
                      BtkDialogFlags   flags)
{
  BtkDialog *dialog;

  dialog = g_object_new (BTK_TYPE_DIALOG, NULL);

  if (title)
    btk_window_set_title (BTK_WINDOW (dialog), title);

  if (parent)
    btk_window_set_transient_for (BTK_WINDOW (dialog), parent);

  if (flags & BTK_DIALOG_MODAL)
    btk_window_set_modal (BTK_WINDOW (dialog), TRUE);
  
  if (flags & BTK_DIALOG_DESTROY_WITH_PARENT)
    btk_window_set_destroy_with_parent (BTK_WINDOW (dialog), TRUE);

  if (flags & BTK_DIALOG_NO_SEPARATOR)
    btk_dialog_set_has_separator (dialog, FALSE);
  
  return BTK_WIDGET (dialog);
}

/**
 * btk_dialog_new_with_buttons:
 * @title: (allow-none): Title of the dialog, or %NULL
 * @parent: (allow-none): Transient parent of the dialog, or %NULL
 * @flags: from #BtkDialogFlags
 * @first_button_text: (allow-none): stock ID or text to go in first button, or %NULL
 * @Varargs: response ID for first button, then additional buttons, ending with %NULL
 *
 * Creates a new #BtkDialog with title @title (or %NULL for the default
 * title; see btk_window_set_title()) and transient parent @parent (or
 * %NULL for none; see btk_window_set_transient_for()). The @flags
 * argument can be used to make the dialog modal (#BTK_DIALOG_MODAL)
 * and/or to have it destroyed along with its transient parent
 * (#BTK_DIALOG_DESTROY_WITH_PARENT). After @flags, button
 * text/response ID pairs should be listed, with a %NULL pointer ending
 * the list. Button text can be either a stock ID such as
 * #BTK_STOCK_OK, or some arbitrary text. A response ID can be
 * any positive number, or one of the values in the #BtkResponseType
 * enumeration. If the user clicks one of these dialog buttons,
 * #BtkDialog will emit the #BtkDialog::response signal with the corresponding
 * response ID. If a #BtkDialog receives the #BtkWidget::delete-event signal, 
 * it will emit ::response with a response ID of #BTK_RESPONSE_DELETE_EVENT.
 * However, destroying a dialog does not emit the ::response signal;
 * so be careful relying on ::response when using the 
 * #BTK_DIALOG_DESTROY_WITH_PARENT flag. Buttons are from left to right,
 * so the first button in the list will be the leftmost button in the dialog.
 *
 * Here's a simple example:
 * |[
 *  BtkWidget *dialog = btk_dialog_new_with_buttons ("My dialog",
 *                                                   main_app_window,
 *                                                   BTK_DIALOG_MODAL | BTK_DIALOG_DESTROY_WITH_PARENT,
 *                                                   BTK_STOCK_OK,
 *                                                   BTK_RESPONSE_ACCEPT,
 *                                                   BTK_STOCK_CANCEL,
 *                                                   BTK_RESPONSE_REJECT,
 *                                                   NULL);
 * ]|
 * 
 * Return value: a new #BtkDialog
 **/
BtkWidget*
btk_dialog_new_with_buttons (const gchar    *title,
                             BtkWindow      *parent,
                             BtkDialogFlags  flags,
                             const gchar    *first_button_text,
                             ...)
{
  BtkDialog *dialog;
  va_list args;
  
  dialog = BTK_DIALOG (btk_dialog_new_empty (title, parent, flags));

  va_start (args, first_button_text);

  btk_dialog_add_buttons_valist (dialog,
                                 first_button_text,
                                 args);
  
  va_end (args);

  return BTK_WIDGET (dialog);
}

static void 
response_data_free (gpointer data)
{
  g_slice_free (ResponseData, data);
}

static ResponseData*
get_response_data (BtkWidget *widget,
		   gboolean   create)
{
  ResponseData *ad = g_object_get_data (G_OBJECT (widget),
                                        "btk-dialog-response-data");

  if (ad == NULL && create)
    {
      ad = g_slice_new (ResponseData);
      
      g_object_set_data_full (G_OBJECT (widget),
                              I_("btk-dialog-response-data"),
                              ad,
			      response_data_free);
    }

  return ad;
}

static void
action_widget_activated (BtkWidget *widget, BtkDialog *dialog)
{
  gint response_id;
  
  response_id = btk_dialog_get_response_for_widget (dialog, widget);

  btk_dialog_response (dialog, response_id);
}

/**
 * btk_dialog_add_action_widget:
 * @dialog: a #BtkDialog
 * @child: an activatable widget
 * @response_id: response ID for @child
 * 
 * Adds an activatable widget to the action area of a #BtkDialog,
 * connecting a signal handler that will emit the #BtkDialog::response 
 * signal on the dialog when the widget is activated. The widget is 
 * appended to the end of the dialog's action area. If you want to add a
 * non-activatable widget, simply pack it into the @action_area field 
 * of the #BtkDialog struct.
 **/
void
btk_dialog_add_action_widget (BtkDialog *dialog,
                              BtkWidget *child,
                              gint       response_id)
{
  ResponseData *ad;
  guint signal_id;
  
  g_return_if_fail (BTK_IS_DIALOG (dialog));
  g_return_if_fail (BTK_IS_WIDGET (child));

  ad = get_response_data (child, TRUE);

  ad->response_id = response_id;

  if (BTK_IS_BUTTON (child))
    signal_id = g_signal_lookup ("clicked", BTK_TYPE_BUTTON);
  else
    signal_id = BTK_WIDGET_GET_CLASS (child)->activate_signal;

  if (signal_id)
    {
      GClosure *closure;

      closure = g_cclosure_new_object (G_CALLBACK (action_widget_activated),
				       G_OBJECT (dialog));
      g_signal_connect_closure_by_id (child,
				      signal_id,
				      0,
				      closure,
				      FALSE);
    }
  else
    g_warning ("Only 'activatable' widgets can be packed into the action area of a BtkDialog");

  btk_box_pack_end (BTK_BOX (dialog->action_area),
                    child,
                    FALSE, TRUE, 0);
  
  if (response_id == BTK_RESPONSE_HELP)
    btk_button_box_set_child_secondary (BTK_BUTTON_BOX (dialog->action_area), child, TRUE);
}

/**
 * btk_dialog_add_button:
 * @dialog: a #BtkDialog
 * @button_text: text of button, or stock ID
 * @response_id: response ID for the button
 * 
 * Adds a button with the given text (or a stock button, if @button_text is a
 * stock ID) and sets things up so that clicking the button will emit the
 * #BtkDialog::response signal with the given @response_id. The button is 
 * appended to the end of the dialog's action area. The button widget is 
 * returned, but usually you don't need it.
 *
 * Return value: (transfer none): the button widget that was added
 **/
BtkWidget*
btk_dialog_add_button (BtkDialog   *dialog,
                       const gchar *button_text,
                       gint         response_id)
{
  BtkWidget *button;
  
  g_return_val_if_fail (BTK_IS_DIALOG (dialog), NULL);
  g_return_val_if_fail (button_text != NULL, NULL);

  button = btk_button_new_from_stock (button_text);

  btk_widget_set_can_default (button, TRUE);
  
  btk_widget_show (button);
  
  btk_dialog_add_action_widget (dialog,
                                button,
                                response_id);

  return button;
}

static void
btk_dialog_add_buttons_valist (BtkDialog      *dialog,
                               const gchar    *first_button_text,
                               va_list         args)
{
  const gchar* text;
  gint response_id;

  g_return_if_fail (BTK_IS_DIALOG (dialog));
  
  if (first_button_text == NULL)
    return;
  
  text = first_button_text;
  response_id = va_arg (args, gint);

  while (text != NULL)
    {
      btk_dialog_add_button (dialog, text, response_id);

      text = va_arg (args, gchar*);
      if (text == NULL)
        break;
      response_id = va_arg (args, int);
    }
}

/**
 * btk_dialog_add_buttons:
 * @dialog: a #BtkDialog
 * @first_button_text: button text or stock ID
 * @Varargs: response ID for first button, then more text-response_id pairs
 * 
 * Adds more buttons, same as calling btk_dialog_add_button()
 * repeatedly.  The variable argument list should be %NULL-terminated
 * as with btk_dialog_new_with_buttons(). Each button must have both
 * text and response ID.
 **/
void
btk_dialog_add_buttons (BtkDialog   *dialog,
                        const gchar *first_button_text,
                        ...)
{  
  va_list args;

  va_start (args, first_button_text);

  btk_dialog_add_buttons_valist (dialog,
                                 first_button_text,
                                 args);
  
  va_end (args);
}

/**
 * btk_dialog_set_response_sensitive:
 * @dialog: a #BtkDialog
 * @response_id: a response ID
 * @setting: %TRUE for sensitive
 *
 * Calls <literal>btk_widget_set_sensitive (widget, @setting)</literal> 
 * for each widget in the dialog's action area with the given @response_id.
 * A convenient way to sensitize/desensitize dialog buttons.
 **/
void
btk_dialog_set_response_sensitive (BtkDialog *dialog,
                                   gint       response_id,
                                   gboolean   setting)
{
  GList *children;
  GList *tmp_list;

  g_return_if_fail (BTK_IS_DIALOG (dialog));

  children = btk_container_get_children (BTK_CONTAINER (dialog->action_area));

  tmp_list = children;
  while (tmp_list != NULL)
    {
      BtkWidget *widget = tmp_list->data;
      ResponseData *rd = get_response_data (widget, FALSE);

      if (rd && rd->response_id == response_id)
        btk_widget_set_sensitive (widget, setting);

      tmp_list = g_list_next (tmp_list);
    }

  g_list_free (children);
}

/**
 * btk_dialog_set_default_response:
 * @dialog: a #BtkDialog
 * @response_id: a response ID
 * 
 * Sets the last widget in the dialog's action area with the given @response_id
 * as the default widget for the dialog. Pressing "Enter" normally activates
 * the default widget.
 **/
void
btk_dialog_set_default_response (BtkDialog *dialog,
                                 gint       response_id)
{
  GList *children;
  GList *tmp_list;

  g_return_if_fail (BTK_IS_DIALOG (dialog));

  children = btk_container_get_children (BTK_CONTAINER (dialog->action_area));

  tmp_list = children;
  while (tmp_list != NULL)
    {
      BtkWidget *widget = tmp_list->data;
      ResponseData *rd = get_response_data (widget, FALSE);

      if (rd && rd->response_id == response_id)
	btk_widget_grab_default (widget);
	    
      tmp_list = g_list_next (tmp_list);
    }

  g_list_free (children);
}

/**
 * btk_dialog_set_has_separator:
 * @dialog: a #BtkDialog
 * @setting: %TRUE to have a separator
 *
 * Sets whether the dialog has a separator above the buttons.
 *
 * Deprecated: 2.22: This function will be removed in BTK+ 3
 **/
void
btk_dialog_set_has_separator (BtkDialog *dialog,
                              gboolean   setting)
{
  BtkDialogPrivate *priv;

  g_return_if_fail (BTK_IS_DIALOG (dialog));

  priv = GET_PRIVATE (dialog);

  /* this might fail if we get called before _init() somehow */
  g_assert (dialog->vbox != NULL);

  if (priv->ignore_separator)
    {
      g_warning ("Ignoring the separator setting");
      return;
    }
  
  if (setting && dialog->separator == NULL)
    {
      dialog->separator = btk_hseparator_new ();
      btk_box_pack_end (BTK_BOX (dialog->vbox), dialog->separator, FALSE, TRUE, 0);

      /* The app programmer could screw this up, but, their own fault.
       * Moves the separator just above the action area.
       */
      btk_box_reorder_child (BTK_BOX (dialog->vbox), dialog->separator, 1);
      btk_widget_show (dialog->separator);
    }
  else if (!setting && dialog->separator != NULL)
    {
      btk_widget_destroy (dialog->separator);
      dialog->separator = NULL;
    }

  g_object_notify (G_OBJECT (dialog), "has-separator");
}

/**
 * btk_dialog_get_has_separator:
 * @dialog: a #BtkDialog
 * 
 * Accessor for whether the dialog has a separator.
 * 
 * Return value: %TRUE if the dialog has a separator
 *
 * Deprecated: 2.22: This function will be removed in BTK+ 3
 **/
gboolean
btk_dialog_get_has_separator (BtkDialog *dialog)
{
  g_return_val_if_fail (BTK_IS_DIALOG (dialog), FALSE);

  return dialog->separator != NULL;
}

/**
 * btk_dialog_response:
 * @dialog: a #BtkDialog
 * @response_id: response ID 
 * 
 * Emits the #BtkDialog::response signal with the given response ID. 
 * Used to indicate that the user has responded to the dialog in some way;
 * typically either you or btk_dialog_run() will be monitoring the
 * ::response signal and take appropriate action.
 **/
void
btk_dialog_response (BtkDialog *dialog,
                     gint       response_id)
{
  g_return_if_fail (BTK_IS_DIALOG (dialog));

  g_signal_emit (dialog,
		 dialog_signals[RESPONSE],
		 0,
		 response_id);
}

typedef struct
{
  BtkDialog *dialog;
  gint response_id;
  GMainLoop *loop;
  gboolean destroyed;
} RunInfo;

static void
shutdown_loop (RunInfo *ri)
{
  if (g_main_loop_is_running (ri->loop))
    g_main_loop_quit (ri->loop);
}

static void
run_unmap_handler (BtkDialog *dialog, gpointer data)
{
  RunInfo *ri = data;

  shutdown_loop (ri);
}

static void
run_response_handler (BtkDialog *dialog,
                      gint response_id,
                      gpointer data)
{
  RunInfo *ri;

  ri = data;

  ri->response_id = response_id;

  shutdown_loop (ri);
}

static gint
run_delete_handler (BtkDialog *dialog,
                    BdkEventAny *event,
                    gpointer data)
{
  RunInfo *ri = data;
    
  shutdown_loop (ri);
  
  return TRUE; /* Do not destroy */
}

static void
run_destroy_handler (BtkDialog *dialog, gpointer data)
{
  RunInfo *ri = data;

  /* shutdown_loop will be called by run_unmap_handler */
  
  ri->destroyed = TRUE;
}

/**
 * btk_dialog_run:
 * @dialog: a #BtkDialog
 * 
 * Blocks in a recursive main loop until the @dialog either emits the
 * #BtkDialog::response signal, or is destroyed. If the dialog is 
 * destroyed during the call to btk_dialog_run(), btk_dialog_run() returns 
 * #BTK_RESPONSE_NONE. Otherwise, it returns the response ID from the 
 * ::response signal emission.
 *
 * Before entering the recursive main loop, btk_dialog_run() calls
 * btk_widget_show() on the dialog for you. Note that you still
 * need to show any children of the dialog yourself.
 *
 * During btk_dialog_run(), the default behavior of #BtkWidget::delete-event 
 * is disabled; if the dialog receives ::delete_event, it will not be
 * destroyed as windows usually are, and btk_dialog_run() will return
 * #BTK_RESPONSE_DELETE_EVENT. Also, during btk_dialog_run() the dialog 
 * will be modal. You can force btk_dialog_run() to return at any time by
 * calling btk_dialog_response() to emit the ::response signal. Destroying 
 * the dialog during btk_dialog_run() is a very bad idea, because your 
 * post-run code won't know whether the dialog was destroyed or not.
 *
 * After btk_dialog_run() returns, you are responsible for hiding or
 * destroying the dialog if you wish to do so.
 *
 * Typical usage of this function might be:
 * |[
 *   gint result = btk_dialog_run (BTK_DIALOG (dialog));
 *   switch (result)
 *     {
 *       case BTK_RESPONSE_ACCEPT:
 *          do_application_specific_something ();
 *          break;
 *       default:
 *          do_nothing_since_dialog_was_cancelled ();
 *          break;
 *     }
 *   btk_widget_destroy (dialog);
 * ]|
 * 
 * Note that even though the recursive main loop gives the effect of a
 * modal dialog (it prevents the user from interacting with other 
 * windows in the same window group while the dialog is run), callbacks 
 * such as timeouts, IO channel watches, DND drops, etc, <emphasis>will</emphasis> 
 * be triggered during a btk_dialog_run() call.
 * 
 * Return value: response ID
 **/
gint
btk_dialog_run (BtkDialog *dialog)
{
  RunInfo ri = { NULL, BTK_RESPONSE_NONE, NULL, FALSE };
  gboolean was_modal;
  gulong response_handler;
  gulong unmap_handler;
  gulong destroy_handler;
  gulong delete_handler;
  
  g_return_val_if_fail (BTK_IS_DIALOG (dialog), -1);

  g_object_ref (dialog);

  was_modal = BTK_WINDOW (dialog)->modal;
  if (!was_modal)
    btk_window_set_modal (BTK_WINDOW (dialog), TRUE);

  if (!btk_widget_get_visible (BTK_WIDGET (dialog)))
    btk_widget_show (BTK_WIDGET (dialog));
  
  response_handler =
    g_signal_connect (dialog,
                      "response",
                      G_CALLBACK (run_response_handler),
                      &ri);
  
  unmap_handler =
    g_signal_connect (dialog,
                      "unmap",
                      G_CALLBACK (run_unmap_handler),
                      &ri);
  
  delete_handler =
    g_signal_connect (dialog,
                      "delete-event",
                      G_CALLBACK (run_delete_handler),
                      &ri);
  
  destroy_handler =
    g_signal_connect (dialog,
                      "destroy",
                      G_CALLBACK (run_destroy_handler),
                      &ri);
  
  ri.loop = g_main_loop_new (NULL, FALSE);

  BDK_THREADS_LEAVE ();  
  g_main_loop_run (ri.loop);
  BDK_THREADS_ENTER ();  

  g_main_loop_unref (ri.loop);

  ri.loop = NULL;
  
  if (!ri.destroyed)
    {
      if (!was_modal)
        btk_window_set_modal (BTK_WINDOW(dialog), FALSE);
      
      g_signal_handler_disconnect (dialog, response_handler);
      g_signal_handler_disconnect (dialog, unmap_handler);
      g_signal_handler_disconnect (dialog, delete_handler);
      g_signal_handler_disconnect (dialog, destroy_handler);
    }

  g_object_unref (dialog);

  return ri.response_id;
}

void
_btk_dialog_set_ignore_separator (BtkDialog *dialog,
				  gboolean   ignore_separator)
{
  BtkDialogPrivate *priv;

  priv = GET_PRIVATE (dialog);
  priv->ignore_separator = ignore_separator;
}

/**
 * btk_dialog_get_widget_for_response:
 * @dialog: a #BtkDialog
 * @response_id: the response ID used by the @dialog widget
 *
 * Gets the widget button that uses the given response ID in the action area
 * of a dialog.
 *
 * Returns: (transfer none):the @widget button that uses the given @response_id, or %NULL.
 *
 * Since: 2.20
 */
BtkWidget*
btk_dialog_get_widget_for_response (BtkDialog *dialog,
				    gint       response_id)
{
  GList *children;
  GList *tmp_list;

  g_return_val_if_fail (BTK_IS_DIALOG (dialog), NULL);

  children = btk_container_get_children (BTK_CONTAINER (dialog->action_area));

  tmp_list = children;
  while (tmp_list != NULL)
    {
      BtkWidget *widget = tmp_list->data;
      ResponseData *rd = get_response_data (widget, FALSE);

      if (rd && rd->response_id == response_id)
        {
          g_list_free (children);
          return widget;
        }

      tmp_list = g_list_next (tmp_list);
    }

  g_list_free (children);

  return NULL;
}

/**
 * btk_dialog_get_response_for_widget:
 * @dialog: a #BtkDialog
 * @widget: a widget in the action area of @dialog
 *
 * Gets the response id of a widget in the action area
 * of a dialog.
 *
 * Returns: the response id of @widget, or %BTK_RESPONSE_NONE
 *  if @widget doesn't have a response id set.
 *
 * Since: 2.8
 */
gint
btk_dialog_get_response_for_widget (BtkDialog *dialog,
				    BtkWidget *widget)
{
  ResponseData *rd;

  rd = get_response_data (widget, FALSE);
  if (!rd)
    return BTK_RESPONSE_NONE;
  else
    return rd->response_id;
}

/**
 * btk_alternative_dialog_button_order:
 * @screen: (allow-none): a #BdkScreen, or %NULL to use the default screen
 *
 * Returns %TRUE if dialogs are expected to use an alternative
 * button order on the screen @screen. See
 * btk_dialog_set_alternative_button_order() for more details
 * about alternative button order. 
 *
 * If you need to use this function, you should probably connect
 * to the ::notify:btk-alternative-button-order signal on the
 * #BtkSettings object associated to @screen, in order to be 
 * notified if the button order setting changes.
 *
 * Returns: Whether the alternative button order should be used
 *
 * Since: 2.6
 */
gboolean 
btk_alternative_dialog_button_order (BdkScreen *screen)
{
  BtkSettings *settings;
  gboolean result;

  if (screen)
    settings = btk_settings_get_for_screen (screen);
  else
    settings = btk_settings_get_default ();
  
  g_object_get (settings,
		"btk-alternative-button-order", &result, NULL);

  return result;
}

static void
btk_dialog_set_alternative_button_order_valist (BtkDialog *dialog,
						gint       first_response_id,
						va_list    args)
{
  BtkWidget *child;
  gint response_id;
  gint position;

  response_id = first_response_id;
  position = 0;
  while (response_id != -1)
    {
      /* reorder child with response_id to position */
      child = dialog_find_button (dialog, response_id);
      if (child != NULL)
        btk_box_reorder_child (BTK_BOX (dialog->action_area), child, position);
      else
        g_warning ("%s : no child button with response id %d.", G_STRFUNC,
                   response_id);

      response_id = va_arg (args, gint);
      position++;
    }
}

/**
 * btk_dialog_set_alternative_button_order:
 * @dialog: a #BtkDialog
 * @first_response_id: a response id used by one @dialog's buttons
 * @Varargs: a list of more response ids of @dialog's buttons, terminated by -1
 *
 * Sets an alternative button order. If the 
 * #BtkSettings:btk-alternative-button-order setting is set to %TRUE, 
 * the dialog buttons are reordered according to the order of the 
 * response ids passed to this function.
 *
 * By default, BTK+ dialogs use the button order advocated by the Bunny 
 * <ulink url="http://developer.bunny.org/projects/gup/hig/2.0/">Human 
 * Interface Guidelines</ulink> with the affirmative button at the far 
 * right, and the cancel button left of it. But the builtin BTK+ dialogs
 * and #BtkMessageDialog<!-- -->s do provide an alternative button order,
 * which is more suitable on some platforms, e.g. Windows.
 *
 * Use this function after adding all the buttons to your dialog, as the 
 * following example shows:
 * |[
 * cancel_button = btk_dialog_add_button (BTK_DIALOG (dialog),
 *                                        BTK_STOCK_CANCEL,
 *                                        BTK_RESPONSE_CANCEL);
 *  
 * ok_button = btk_dialog_add_button (BTK_DIALOG (dialog),
 *                                    BTK_STOCK_OK,
 *                                    BTK_RESPONSE_OK);
 *   
 * btk_widget_grab_default (ok_button);
 *   
 * help_button = btk_dialog_add_button (BTK_DIALOG (dialog),
 *                                      BTK_STOCK_HELP,
 *                                      BTK_RESPONSE_HELP);
 *  
 * btk_dialog_set_alternative_button_order (BTK_DIALOG (dialog),
 *                                          BTK_RESPONSE_OK,
 *                                          BTK_RESPONSE_CANCEL,
 *                                          BTK_RESPONSE_HELP,
 *                                          -1);
 * ]|
 * 
 * Since: 2.6
 */
void 
btk_dialog_set_alternative_button_order (BtkDialog *dialog,
					 gint       first_response_id,
					 ...)
{
  BdkScreen *screen;
  va_list args;
  
  g_return_if_fail (BTK_IS_DIALOG (dialog));

  screen = btk_widget_get_screen (BTK_WIDGET (dialog));
  if (!btk_alternative_dialog_button_order (screen))
      return;

  va_start (args, first_response_id);

  btk_dialog_set_alternative_button_order_valist (dialog,
						  first_response_id,
						  args);
  va_end (args);
}
/**
 * btk_dialog_set_alternative_button_order_from_array:
 * @dialog: a #BtkDialog
 * @n_params: the number of response ids in @new_order
 * @new_order: (array length=n_params): an array of response ids of
 *     @dialog's buttons
 *
 * Sets an alternative button order. If the 
 * #BtkSettings:btk-alternative-button-order setting is set to %TRUE, 
 * the dialog buttons are reordered according to the order of the 
 * response ids in @new_order.
 *
 * See btk_dialog_set_alternative_button_order() for more information.
 *
 * This function is for use by language bindings.
 * 
 * Since: 2.6
 */
void 
btk_dialog_set_alternative_button_order_from_array (BtkDialog *dialog,
                                                    gint       n_params,
                                                    gint      *new_order)
{
  BdkScreen *screen;
  BtkWidget *child;
  gint position;

  g_return_if_fail (BTK_IS_DIALOG (dialog));
  g_return_if_fail (new_order != NULL);

  screen = btk_widget_get_screen (BTK_WIDGET (dialog));
  if (!btk_alternative_dialog_button_order (screen))
      return;

  for (position = 0; position < n_params; position++)
  {
      /* reorder child with response_id to position */
      child = dialog_find_button (dialog, new_order[position]);
      if (child != NULL)
        btk_box_reorder_child (BTK_BOX (dialog->action_area), child, position);
      else
        g_warning ("%s : no child button with response id %d.", G_STRFUNC,
                   new_order[position]);
    }
}

typedef struct {
  gchar *widget_name;
  gchar *response_id;
} ActionWidgetInfo;

typedef struct {
  BtkDialog *dialog;
  BtkBuilder *builder;
  GSList *items;
  gchar *response;
} ActionWidgetsSubParserData;

static void
attributes_start_element (GMarkupParseContext *context,
			  const gchar         *element_name,
			  const gchar        **names,
			  const gchar        **values,
			  gpointer             user_data,
			  GError             **error)
{
  ActionWidgetsSubParserData *parser_data = (ActionWidgetsSubParserData*)user_data;
  guint i;

  if (strcmp (element_name, "action-widget") == 0)
    {
      for (i = 0; names[i]; i++)
	if (strcmp (names[i], "response") == 0)
	  parser_data->response = g_strdup (values[i]);
    }
  else if (strcmp (element_name, "action-widgets") == 0)
    return;
  else
    g_warning ("Unsupported tag for BtkDialog: %s\n", element_name);
}

static void
attributes_text_element (GMarkupParseContext *context,
			 const gchar         *text,
			 gsize                text_len,
			 gpointer             user_data,
			 GError             **error)
{
  ActionWidgetsSubParserData *parser_data = (ActionWidgetsSubParserData*)user_data;
  ActionWidgetInfo *item;

  if (!parser_data->response)
    return;

  item = g_new (ActionWidgetInfo, 1);
  item->widget_name = g_strndup (text, text_len);
  item->response_id = parser_data->response;
  parser_data->items = g_slist_prepend (parser_data->items, item);
  parser_data->response = NULL;
}

static const GMarkupParser attributes_parser =
  {
    attributes_start_element,
    NULL,
    attributes_text_element,
  };

static gboolean
btk_dialog_buildable_custom_tag_start (BtkBuildable  *buildable,
				       BtkBuilder    *builder,
				       GObject       *child,
				       const gchar   *tagname,
				       GMarkupParser *parser,
				       gpointer      *data)
{
  ActionWidgetsSubParserData *parser_data;

  if (child)
    return FALSE;

  if (strcmp (tagname, "action-widgets") == 0)
    {
      parser_data = g_slice_new0 (ActionWidgetsSubParserData);
      parser_data->dialog = BTK_DIALOG (buildable);
      parser_data->items = NULL;

      *parser = attributes_parser;
      *data = parser_data;
      return TRUE;
    }

  return parent_buildable_iface->custom_tag_start (buildable, builder, child,
						   tagname, parser, data);
}

static void
btk_dialog_buildable_custom_finished (BtkBuildable *buildable,
				      BtkBuilder   *builder,
				      GObject      *child,
				      const gchar  *tagname,
				      gpointer      user_data)
{
  GSList *l;
  ActionWidgetsSubParserData *parser_data;
  GObject *object;
  BtkDialog *dialog;
  ResponseData *ad;
  guint signal_id;
  
  if (strcmp (tagname, "action-widgets"))
    {
    parent_buildable_iface->custom_finished (buildable, builder, child,
					     tagname, user_data);
    return;
    }

  dialog = BTK_DIALOG (buildable);
  parser_data = (ActionWidgetsSubParserData*)user_data;
  parser_data->items = g_slist_reverse (parser_data->items);

  for (l = parser_data->items; l; l = l->next)
    {
      ActionWidgetInfo *item = l->data;

      object = btk_builder_get_object (builder, item->widget_name);
      if (!object)
	{
	  g_warning ("Unknown object %s specified in action-widgets of %s",
		     item->widget_name,
		     btk_buildable_get_name (BTK_BUILDABLE (buildable)));
	  continue;
	}

      ad = get_response_data (BTK_WIDGET (object), TRUE);
      ad->response_id = atoi (item->response_id);

      if (BTK_IS_BUTTON (object))
	signal_id = g_signal_lookup ("clicked", BTK_TYPE_BUTTON);
      else
	signal_id = BTK_WIDGET_GET_CLASS (object)->activate_signal;
      
      if (signal_id)
	{
	  GClosure *closure;
	  
	  closure = g_cclosure_new_object (G_CALLBACK (action_widget_activated),
					   G_OBJECT (dialog));
	  g_signal_connect_closure_by_id (object,
					  signal_id,
					  0,
					  closure,
					  FALSE);
	}

      if (ad->response_id == BTK_RESPONSE_HELP)
	btk_button_box_set_child_secondary (BTK_BUTTON_BOX (dialog->action_area),
					    BTK_WIDGET (object), TRUE);

      g_free (item->widget_name);
      g_free (item->response_id);
      g_free (item);
    }
  g_slist_free (parser_data->items);
  g_slice_free (ActionWidgetsSubParserData, parser_data);
}

/**
 * btk_dialog_get_action_area:
 * @dialog: a #BtkDialog
 *
 * Returns the action area of @dialog.
 *
 * Returns: (transfer none): the action area.
 *
 * Since: 2.14
 **/
BtkWidget *
btk_dialog_get_action_area (BtkDialog *dialog)
{
  g_return_val_if_fail (BTK_IS_DIALOG (dialog), NULL);

  return dialog->action_area;
}

/**
 * btk_dialog_get_content_area:
 * @dialog: a #BtkDialog
 *
 * Returns the content area of @dialog.
 *
 * Returns: (transfer none): the content area #BtkVBox.
 *
 * Since: 2.14
 **/
BtkWidget *
btk_dialog_get_content_area (BtkDialog *dialog)
{
  g_return_val_if_fail (BTK_IS_DIALOG (dialog), NULL);

  return dialog->vbox;
}

#define __BTK_DIALOG_C__
#include "btkaliasdef.c"
