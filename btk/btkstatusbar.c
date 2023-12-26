/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * BtkStatusbar Copyright (C) 1998 Shawn T. Amundson
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
#include "btkframe.h"
#include "btklabel.h"
#include "btkmarshalers.h"
#include "btkstatusbar.h"
#include "btkwindow.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkbuildable.h"
#include "btkalias.h"

typedef struct _BtkStatusbarMsg BtkStatusbarMsg;

struct _BtkStatusbarMsg
{
  gchar *text;
  guint context_id;
  guint message_id;
};

enum
{
  SIGNAL_TEXT_PUSHED,
  SIGNAL_TEXT_POPPED,
  SIGNAL_LAST
};

enum
{
  PROP_0,
  PROP_HAS_RESIZE_GRIP
};

static void     btk_statusbar_buildable_interface_init    (BtkBuildableIface *iface);
static BObject *btk_statusbar_buildable_get_internal_child (BtkBuildable *buildable,
                                                            BtkBuilder   *builder,
                                                            const gchar  *childname);
static void     btk_statusbar_destroy           (BtkObject         *object);
static void     btk_statusbar_update            (BtkStatusbar      *statusbar,
						 guint              context_id,
						 const gchar       *text);
static void     btk_statusbar_size_allocate     (BtkWidget         *widget,
						 BtkAllocation     *allocation);
static void     btk_statusbar_realize           (BtkWidget         *widget);
static void     btk_statusbar_unrealize         (BtkWidget         *widget);
static void     btk_statusbar_map               (BtkWidget         *widget);
static void     btk_statusbar_unmap             (BtkWidget         *widget);
static gboolean btk_statusbar_button_press      (BtkWidget         *widget,
						 BdkEventButton    *event);
static gboolean btk_statusbar_expose_event      (BtkWidget         *widget,
						 BdkEventExpose    *event);
static void     btk_statusbar_size_request      (BtkWidget         *widget,
						 BtkRequisition    *requisition);
static void     btk_statusbar_size_allocate     (BtkWidget         *widget,
						 BtkAllocation     *allocation);
static void     btk_statusbar_direction_changed (BtkWidget         *widget,
						 BtkTextDirection   prev_dir);
static void     btk_statusbar_state_changed     (BtkWidget        *widget,
                                                 BtkStateType      previous_state);
static void     btk_statusbar_create_window     (BtkStatusbar      *statusbar);
static void     btk_statusbar_destroy_window    (BtkStatusbar      *statusbar);
static void     btk_statusbar_get_property      (BObject           *object,
						 guint              prop_id,
						 BValue            *value,
						 BParamSpec        *pspec);
static void     btk_statusbar_set_property      (BObject           *object,
						 guint              prop_id,
						 const BValue      *value,
						 BParamSpec        *pspec);
static void     label_selectable_changed        (BtkWidget         *label,
                                                 BParamSpec        *pspec,
						 gpointer           data);


static guint              statusbar_signals[SIGNAL_LAST] = { 0 };

G_DEFINE_TYPE_WITH_CODE (BtkStatusbar, btk_statusbar, BTK_TYPE_HBOX,
                         G_IMPLEMENT_INTERFACE (BTK_TYPE_BUILDABLE,
                                                btk_statusbar_buildable_interface_init));

static void
btk_statusbar_class_init (BtkStatusbarClass *class)
{
  BObjectClass *bobject_class;
  BtkObjectClass *object_class;
  BtkWidgetClass *widget_class;

  bobject_class = (BObjectClass *) class;
  object_class = (BtkObjectClass *) class;
  widget_class = (BtkWidgetClass *) class;

  bobject_class->set_property = btk_statusbar_set_property;
  bobject_class->get_property = btk_statusbar_get_property;

  object_class->destroy = btk_statusbar_destroy;

  widget_class->realize = btk_statusbar_realize;
  widget_class->unrealize = btk_statusbar_unrealize;
  widget_class->map = btk_statusbar_map;
  widget_class->unmap = btk_statusbar_unmap;
  widget_class->button_press_event = btk_statusbar_button_press;
  widget_class->expose_event = btk_statusbar_expose_event;
  widget_class->size_request = btk_statusbar_size_request;
  widget_class->size_allocate = btk_statusbar_size_allocate;
  widget_class->direction_changed = btk_statusbar_direction_changed;
  widget_class->state_changed = btk_statusbar_state_changed;
  
  class->text_pushed = btk_statusbar_update;
  class->text_popped = btk_statusbar_update;
  
  /**
   * BtkStatusbar:has-resize-grip:
   *
   * Whether the statusbar has a grip for resizing the toplevel window.
   *
   * Since: 2.4
   */
  g_object_class_install_property (bobject_class,
				   PROP_HAS_RESIZE_GRIP,
				   g_param_spec_boolean ("has-resize-grip",
 							 P_("Has Resize Grip"),
 							 P_("Whether the statusbar has a grip for resizing the toplevel"),
 							 TRUE,
 							 BTK_PARAM_READWRITE));

  /** 
   * BtkStatusbar::text-pushed:
   * @statusbar: the object which received the signal.
   * @context_id: the context id of the relevant message/statusbar.
   * @text: the message that was pushed.
   * 
   * Is emitted whenever a new message gets pushed onto a statusbar's stack.
   */
  statusbar_signals[SIGNAL_TEXT_PUSHED] =
    g_signal_new (I_("text-pushed"),
		  B_OBJECT_CLASS_TYPE (class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkStatusbarClass, text_pushed),
		  NULL, NULL,
		  _btk_marshal_VOID__UINT_STRING,
		  B_TYPE_NONE, 2,
		  B_TYPE_UINT,
		  B_TYPE_STRING);

  /**
   * BtkStatusbar::text-popped:
   * @statusbar: the object which received the signal.
   * @context_id: the context id of the relevant message/statusbar.
   * @text: the message that was just popped.
   *
   * Is emitted whenever a new message is popped off a statusbar's stack.
   */
  statusbar_signals[SIGNAL_TEXT_POPPED] =
    g_signal_new (I_("text-popped"),
		  B_OBJECT_CLASS_TYPE (class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkStatusbarClass, text_popped),
		  NULL, NULL,
		  _btk_marshal_VOID__UINT_STRING,
		  B_TYPE_NONE, 2,
		  B_TYPE_UINT,
		  B_TYPE_STRING);

  btk_widget_class_install_style_property (widget_class,
                                           g_param_spec_enum ("shadow-type",
                                                              P_("Shadow type"),
                                                              P_("Style of bevel around the statusbar text"),
                                                              BTK_TYPE_SHADOW_TYPE,
                                                              BTK_SHADOW_IN,
                                                              BTK_PARAM_READABLE));
}

static void
btk_statusbar_init (BtkStatusbar *statusbar)
{
  BtkBox *box;
  BtkWidget *message_area;
  BtkShadowType shadow_type;
  
  box = BTK_BOX (statusbar);

  btk_widget_set_redraw_on_allocate (BTK_WIDGET (box), TRUE);

  box->spacing = 2;
  box->homogeneous = FALSE;

  statusbar->has_resize_grip = TRUE;

  btk_widget_style_get (BTK_WIDGET (statusbar), "shadow-type", &shadow_type, NULL);
  
  statusbar->frame = btk_frame_new (NULL);
  btk_frame_set_shadow_type (BTK_FRAME (statusbar->frame), shadow_type);
  btk_box_pack_start (box, statusbar->frame, TRUE, TRUE, 0);
  btk_widget_show (statusbar->frame);

  message_area = btk_hbox_new (FALSE, 4);
  btk_container_add (BTK_CONTAINER (statusbar->frame), message_area);
  btk_widget_show (message_area);

  statusbar->label = btk_label_new ("");
  btk_label_set_single_line_mode (BTK_LABEL (statusbar->label), TRUE);
  btk_misc_set_alignment (BTK_MISC (statusbar->label), 0.0, 0.5);
  g_signal_connect (statusbar->label, "notify::selectable",
		    G_CALLBACK (label_selectable_changed), statusbar);
  btk_label_set_ellipsize (BTK_LABEL (statusbar->label), BANGO_ELLIPSIZE_END);
  btk_container_add (BTK_CONTAINER (message_area), statusbar->label);
  btk_widget_show (statusbar->label);

  statusbar->seq_context_id = 1;
  statusbar->seq_message_id = 1;
  statusbar->messages = NULL;
  statusbar->keys = NULL;
}

static BtkBuildableIface *parent_buildable_iface;

static void
btk_statusbar_buildable_interface_init (BtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->get_internal_child = btk_statusbar_buildable_get_internal_child;
}

static BObject *
btk_statusbar_buildable_get_internal_child (BtkBuildable *buildable,
                                            BtkBuilder   *builder,
                                            const gchar  *childname)
{
    if (strcmp (childname, "message_area") == 0)
      return B_OBJECT (btk_bin_get_child (BTK_BIN (BTK_STATUSBAR (buildable)->frame)));

    return parent_buildable_iface->get_internal_child (buildable,
                                                       builder,
                                                       childname);
}

/**
 * btk_statusbar_new:
 *
 * Creates a new #BtkStatusbar ready for messages.
 *
 * Returns: the new #BtkStatusbar
 */
BtkWidget* 
btk_statusbar_new (void)
{
  return g_object_new (BTK_TYPE_STATUSBAR, NULL);
}

static void
btk_statusbar_update (BtkStatusbar *statusbar,
		      guint	    context_id,
		      const gchar  *text)
{
  g_return_if_fail (BTK_IS_STATUSBAR (statusbar));

  if (!text)
    text = "";

  btk_label_set_text (BTK_LABEL (statusbar->label), text);
}

/**
 * btk_statusbar_get_context_id:
 * @statusbar: a #BtkStatusbar
 * @context_description: textual description of what context 
 *                       the new message is being used in
 *
 * Returns a new context identifier, given a description 
 * of the actual context. Note that the description is 
 * <emphasis>not</emphasis> shown in the UI.
 *
 * Returns: an integer id
 */
guint
btk_statusbar_get_context_id (BtkStatusbar *statusbar,
			      const gchar  *context_description)
{
  gchar *string;
  guint id;
  
  g_return_val_if_fail (BTK_IS_STATUSBAR (statusbar), 0);
  g_return_val_if_fail (context_description != NULL, 0);

  /* we need to preserve namespaces on object datas */
  string = g_strconcat ("btk-status-bar-context:", context_description, NULL);

  id = GPOINTER_TO_UINT (g_object_get_data (B_OBJECT (statusbar), string));
  if (id == 0)
    {
      id = statusbar->seq_context_id++;
      g_object_set_data_full (B_OBJECT (statusbar), string, GUINT_TO_POINTER (id), NULL);
      statusbar->keys = b_slist_prepend (statusbar->keys, string);
    }
  else
    g_free (string);

  return id;
}

/**
 * btk_statusbar_push:
 * @statusbar: a #BtkStatusbar
 * @context_id: the message's context id, as returned by
 *              btk_statusbar_get_context_id()
 * @text: the message to add to the statusbar
 * 
 * Pushes a new message onto a statusbar's stack.
 *
 * Returns: a message id that can be used with 
 *          btk_statusbar_remove().
 */
guint
btk_statusbar_push (BtkStatusbar *statusbar,
		    guint	  context_id,
		    const gchar  *text)
{
  BtkStatusbarMsg *msg;

  g_return_val_if_fail (BTK_IS_STATUSBAR (statusbar), 0);
  g_return_val_if_fail (text != NULL, 0);

  msg = g_slice_new (BtkStatusbarMsg);
  msg->text = g_strdup (text);
  msg->context_id = context_id;
  msg->message_id = statusbar->seq_message_id++;

  statusbar->messages = b_slist_prepend (statusbar->messages, msg);

  g_signal_emit (statusbar,
		 statusbar_signals[SIGNAL_TEXT_PUSHED],
		 0,
		 msg->context_id,
		 msg->text);

  return msg->message_id;
}

/**
 * btk_statusbar_pop:
 * @statusbar: a #BtkStatusBar
 * @context_id: a context identifier
 * 
 * Removes the first message in the #BtkStatusBar's stack
 * with the given context id. 
 *
 * Note that this may not change the displayed message, if 
 * the message at the top of the stack has a different 
 * context id.
 */
void
btk_statusbar_pop (BtkStatusbar *statusbar,
		   guint	 context_id)
{
  BtkStatusbarMsg *msg;

  g_return_if_fail (BTK_IS_STATUSBAR (statusbar));

  if (statusbar->messages)
    {
      GSList *list;

      for (list = statusbar->messages; list; list = list->next)
	{
	  msg = list->data;

	  if (msg->context_id == context_id)
	    {
	      statusbar->messages = b_slist_remove_link (statusbar->messages,
							 list);
	      g_free (msg->text);
              g_slice_free (BtkStatusbarMsg, msg);
	      b_slist_free_1 (list);
	      break;
	    }
	}
    }

  msg = statusbar->messages ? statusbar->messages->data : NULL;

  g_signal_emit (statusbar,
		 statusbar_signals[SIGNAL_TEXT_POPPED],
		 0,
		 (guint) (msg ? msg->context_id : 0),
		 msg ? msg->text : NULL);
}

/**
 * btk_statusbar_remove:
 * @statusbar: a #BtkStatusBar
 * @context_id: a context identifier
 * @message_id: a message identifier, as returned by btk_statusbar_push()
 *
 * Forces the removal of a message from a statusbar's stack. 
 * The exact @context_id and @message_id must be specified.
 */
void
btk_statusbar_remove (BtkStatusbar *statusbar,
		      guint	   context_id,
		      guint        message_id)
{
  BtkStatusbarMsg *msg;

  g_return_if_fail (BTK_IS_STATUSBAR (statusbar));
  g_return_if_fail (message_id > 0);

  msg = statusbar->messages ? statusbar->messages->data : NULL;
  if (msg)
    {
      GSList *list;

      /* care about signal emission if the topmost item is removed */
      if (msg->context_id == context_id &&
	  msg->message_id == message_id)
	{
	  btk_statusbar_pop (statusbar, context_id);
	  return;
	}
      
      for (list = statusbar->messages; list; list = list->next)
	{
	  msg = list->data;
	  
	  if (msg->context_id == context_id &&
	      msg->message_id == message_id)
	    {
	      statusbar->messages = b_slist_remove_link (statusbar->messages, list);
	      g_free (msg->text);
              g_slice_free (BtkStatusbarMsg, msg);
	      b_slist_free_1 (list);
	      
	      break;
	    }
	}
    }
}

/**
 * btk_statusbar_remove_all:
 * @statusbar: a #BtkStatusBar
 * @context_id: a context identifier
 *
 * Forces the removal of all messages from a statusbar's
 * stack with the exact @context_id.
 *
 * Since: 2.22
 */
void
btk_statusbar_remove_all (BtkStatusbar *statusbar,
                          guint         context_id)
{
  BtkStatusbarMsg *msg;
  GSList *prev, *list;

  g_return_if_fail (BTK_IS_STATUSBAR (statusbar));

  if (statusbar->messages == NULL)
    return;

  msg = statusbar->messages->data;

  /* care about signal emission if the topmost item is removed */
  if (msg->context_id == context_id)
    {
      btk_statusbar_pop (statusbar, context_id);

      prev = NULL;
      list = statusbar->messages;
    }
  else
    {
      prev = statusbar->messages;
      list = prev->next;
    }

  while (list != NULL)
    {
      msg = list->data;

      if (msg->context_id == context_id)
        {
          if (prev == NULL)
            statusbar->messages = list->next;
          else
            prev->next = list->next;

          g_free (msg->text);
          g_slice_free (BtkStatusbarMsg, msg);
          b_slist_free_1 (list);

          if (prev == NULL)
            prev = statusbar->messages;

          if (prev)
            list = prev->next;
          else
            list = NULL;
        }
      else
        {
          prev = list;
          list = prev->next;
        }
    }
}

/**
 * btk_statusbar_set_has_resize_grip:
 * @statusbar: a #BtkStatusBar
 * @setting: %TRUE to have a resize grip
 *
 * Sets whether the statusbar has a resize grip. 
 * %TRUE by default.
 */
void
btk_statusbar_set_has_resize_grip (BtkStatusbar *statusbar,
				   gboolean      setting)
{
  g_return_if_fail (BTK_IS_STATUSBAR (statusbar));

  setting = setting != FALSE;

  if (setting != statusbar->has_resize_grip)
    {
      statusbar->has_resize_grip = setting;
      btk_widget_queue_resize (statusbar->label);
      btk_widget_queue_draw (BTK_WIDGET (statusbar));

      if (btk_widget_get_realized (BTK_WIDGET (statusbar)))
        {
          if (statusbar->has_resize_grip && statusbar->grip_window == NULL)
	    {
	      btk_statusbar_create_window (statusbar);
	      if (btk_widget_get_mapped (BTK_WIDGET (statusbar)))
		bdk_window_show (statusbar->grip_window);
	    }
          else if (!statusbar->has_resize_grip && statusbar->grip_window != NULL)
            btk_statusbar_destroy_window (statusbar);
        }
      
      g_object_notify (B_OBJECT (statusbar), "has-resize-grip");
    }
}

/**
 * btk_statusbar_get_has_resize_grip:
 * @statusbar: a #BtkStatusBar
 * 
 * Returns whether the statusbar has a resize grip.
 *
 * Returns: %TRUE if the statusbar has a resize grip.
 */
gboolean
btk_statusbar_get_has_resize_grip (BtkStatusbar *statusbar)
{
  g_return_val_if_fail (BTK_IS_STATUSBAR (statusbar), FALSE);

  return statusbar->has_resize_grip;
}

/**
 * btk_statusbar_get_message_area:
 * @statusbar: a #BtkStatusBar
 *
 * Retrieves the box containing the label widget.
 *
 * Returns: (transfer none): a #BtkBox
 *
 * Since: 2.20
 */
BtkWidget*
btk_statusbar_get_message_area (BtkStatusbar *statusbar)
{
  g_return_val_if_fail (BTK_IS_STATUSBAR (statusbar), NULL);

  return btk_bin_get_child (BTK_BIN (statusbar->frame));
}

static void
btk_statusbar_destroy (BtkObject *object)
{
  BtkStatusbar *statusbar = BTK_STATUSBAR (object);
  GSList *list;

  for (list = statusbar->messages; list; list = list->next)
    {
      BtkStatusbarMsg *msg;

      msg = list->data;
      g_free (msg->text);
      g_slice_free (BtkStatusbarMsg, msg);
    }
  b_slist_free (statusbar->messages);
  statusbar->messages = NULL;

  for (list = statusbar->keys; list; list = list->next)
    g_free (list->data);
  b_slist_free (statusbar->keys);
  statusbar->keys = NULL;

  BTK_OBJECT_CLASS (btk_statusbar_parent_class)->destroy (object);
}

static void
btk_statusbar_set_property (BObject      *object, 
			    guint         prop_id, 
			    const BValue *value, 
			    BParamSpec   *pspec)
{
  BtkStatusbar *statusbar = BTK_STATUSBAR (object);

  switch (prop_id) 
    {
    case PROP_HAS_RESIZE_GRIP:
      btk_statusbar_set_has_resize_grip (statusbar, b_value_get_boolean (value));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_statusbar_get_property (BObject    *object, 
			    guint       prop_id, 
			    BValue     *value, 
			    BParamSpec *pspec)
{
  BtkStatusbar *statusbar = BTK_STATUSBAR (object);
	
  switch (prop_id) 
    {
    case PROP_HAS_RESIZE_GRIP:
      b_value_set_boolean (value, statusbar->has_resize_grip);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static BdkWindowEdge
get_grip_edge (BtkStatusbar *statusbar)
{
  BtkWidget *widget = BTK_WIDGET (statusbar);

  if (btk_widget_get_direction (widget) == BTK_TEXT_DIR_LTR) 
    return BDK_WINDOW_EDGE_SOUTH_EAST; 
  else
    return BDK_WINDOW_EDGE_SOUTH_WEST; 
}

static void
get_grip_rect (BtkStatusbar *statusbar,
               BdkRectangle *rect)
{
  BtkWidget *widget;
  gint w, h;
  
  widget = BTK_WIDGET (statusbar);

  /* These are in effect the max/default size of the grip. */
  w = 18;
  h = 18;

  if (w > widget->allocation.width)
    w = widget->allocation.width;

  if (h > widget->allocation.height - widget->style->ythickness)
    h = widget->allocation.height - widget->style->ythickness;
  
  rect->width = w;
  rect->height = h;
  rect->y = widget->allocation.y + widget->allocation.height - h;

  if (btk_widget_get_direction (widget) == BTK_TEXT_DIR_LTR) 
    rect->x = widget->allocation.x + widget->allocation.width - w;
  else 
    rect->x = widget->allocation.x + widget->style->xthickness;
}

static void
set_grip_cursor (BtkStatusbar *statusbar)
{
  if (statusbar->has_resize_grip && statusbar->grip_window != NULL)
    {
      BtkWidget *widget = BTK_WIDGET (statusbar);
      BdkDisplay *display = btk_widget_get_display (widget);
      BdkCursorType cursor_type;
      BdkCursor *cursor;
      
      if (btk_widget_is_sensitive (widget))
        {
          if (btk_widget_get_direction (widget) == BTK_TEXT_DIR_LTR)
	    cursor_type = BDK_BOTTOM_RIGHT_CORNER;
          else
	    cursor_type = BDK_BOTTOM_LEFT_CORNER;

          cursor = bdk_cursor_new_for_display (display, cursor_type);
          bdk_window_set_cursor (statusbar->grip_window, cursor);
          bdk_cursor_unref (cursor);
        }
      else
        bdk_window_set_cursor (statusbar->grip_window, NULL);
    }
}

static void
btk_statusbar_create_window (BtkStatusbar *statusbar)
{
  BtkWidget *widget;
  BdkWindowAttr attributes;
  gint attributes_mask;
  BdkRectangle rect;

  widget = BTK_WIDGET (statusbar);

  g_return_if_fail (btk_widget_get_realized (widget));
  g_return_if_fail (statusbar->has_resize_grip);

  get_grip_rect (statusbar, &rect);

  attributes.x = rect.x;
  attributes.y = rect.y;
  attributes.width = rect.width;
  attributes.height = rect.height;
  attributes.window_type = BDK_WINDOW_CHILD;
  attributes.wclass = BDK_INPUT_ONLY;
  attributes.event_mask = btk_widget_get_events (widget) |
    BDK_BUTTON_PRESS_MASK;

  attributes_mask = BDK_WA_X | BDK_WA_Y;

  statusbar->grip_window = bdk_window_new (widget->window,
                                           &attributes, attributes_mask);

  bdk_window_set_user_data (statusbar->grip_window, widget);

  bdk_window_raise (statusbar->grip_window);

  set_grip_cursor (statusbar);
}

static void
btk_statusbar_direction_changed (BtkWidget        *widget,
				 BtkTextDirection  prev_dir)
{
  BtkStatusbar *statusbar = BTK_STATUSBAR (widget);

  set_grip_cursor (statusbar);
}

static void
btk_statusbar_state_changed (BtkWidget    *widget,
	                     BtkStateType  previous_state)   
{
  BtkStatusbar *statusbar = BTK_STATUSBAR (widget);

  set_grip_cursor (statusbar);
}

static void
btk_statusbar_destroy_window (BtkStatusbar *statusbar)
{
  bdk_window_set_user_data (statusbar->grip_window, NULL);
  bdk_window_destroy (statusbar->grip_window);
  statusbar->grip_window = NULL;
}

static void
btk_statusbar_realize (BtkWidget *widget)
{
  BtkStatusbar *statusbar;

  statusbar = BTK_STATUSBAR (widget);

  BTK_WIDGET_CLASS (btk_statusbar_parent_class)->realize (widget);

  if (statusbar->has_resize_grip)
    btk_statusbar_create_window (statusbar);
}

static void
btk_statusbar_unrealize (BtkWidget *widget)
{
  BtkStatusbar *statusbar;

  statusbar = BTK_STATUSBAR (widget);

  if (statusbar->grip_window)
    btk_statusbar_destroy_window (statusbar);

  BTK_WIDGET_CLASS (btk_statusbar_parent_class)->unrealize (widget);
}

static void
btk_statusbar_map (BtkWidget *widget)
{
  BtkStatusbar *statusbar;

  statusbar = BTK_STATUSBAR (widget);

  BTK_WIDGET_CLASS (btk_statusbar_parent_class)->map (widget);

  if (statusbar->grip_window)
    bdk_window_show (statusbar->grip_window);
}

static void
btk_statusbar_unmap (BtkWidget *widget)
{
  BtkStatusbar *statusbar;

  statusbar = BTK_STATUSBAR (widget);

  if (statusbar->grip_window)
    bdk_window_hide (statusbar->grip_window);

  BTK_WIDGET_CLASS (btk_statusbar_parent_class)->unmap (widget);
}

static gboolean
btk_statusbar_button_press (BtkWidget      *widget,
                            BdkEventButton *event)
{
  BtkStatusbar *statusbar;
  BtkWidget *ancestor;
  BdkWindowEdge edge;
  
  statusbar = BTK_STATUSBAR (widget);
  
  if (!statusbar->has_resize_grip ||
      event->type != BDK_BUTTON_PRESS ||
      event->window != statusbar->grip_window)
    return FALSE;
  
  ancestor = btk_widget_get_toplevel (widget);

  if (!BTK_IS_WINDOW (ancestor))
    return FALSE;

  edge = get_grip_edge (statusbar);

  if (event->button == 1)
    btk_window_begin_resize_drag (BTK_WINDOW (ancestor),
                                  edge,
                                  event->button,
                                  event->x_root, event->y_root,
                                  event->time);
  else if (event->button == 2)
    btk_window_begin_move_drag (BTK_WINDOW (ancestor),
                                event->button,
                                event->x_root, event->y_root,
                                event->time);
  else
    return FALSE;
  
  return TRUE;
}

static gboolean
btk_statusbar_expose_event (BtkWidget      *widget,
                            BdkEventExpose *event)
{
  BtkStatusbar *statusbar;
  BdkRectangle rect;
  
  statusbar = BTK_STATUSBAR (widget);

  BTK_WIDGET_CLASS (btk_statusbar_parent_class)->expose_event (widget, event);

  if (statusbar->has_resize_grip)
    {
      BdkWindowEdge edge;
      
      edge = get_grip_edge (statusbar);

      get_grip_rect (statusbar, &rect);

      btk_paint_resize_grip (widget->style,
                             widget->window,
                             btk_widget_get_state (widget),
                             &event->area,
                             widget,
                             "statusbar",
                             edge,
                             rect.x, rect.y,
                             /* don't draw grip over the frame, though you
                              * can click on the frame.
                              */
                             rect.width - widget->style->xthickness,
                             rect.height - widget->style->ythickness);
    }

  return FALSE;
}

static void
btk_statusbar_size_request (BtkWidget      *widget,
			    BtkRequisition *requisition)
{
  BtkStatusbar *statusbar;
  BtkShadowType shadow_type;
  
  statusbar = BTK_STATUSBAR (widget);

  btk_widget_style_get (BTK_WIDGET (statusbar), "shadow-type", &shadow_type, NULL);  
  btk_frame_set_shadow_type (BTK_FRAME (statusbar->frame), shadow_type);
  
  BTK_WIDGET_CLASS (btk_statusbar_parent_class)->size_request (widget, requisition);
}

/* look for extra children between the frame containing
 * the label and where we want to draw the resize grip 
 */
static gboolean
has_extra_children (BtkStatusbar *statusbar)
{
  GList *l;
  BtkBoxChild *child, *frame;

  /* If the internal frame has been modified assume we have extra children */
  if (btk_bin_get_child (BTK_BIN (statusbar->frame)) != statusbar->label)
    return TRUE;

  frame = NULL;
  for (l = BTK_BOX (statusbar)->children; l; l = l->next)
    {
      frame = l->data;

      if (frame->widget == statusbar->frame)
	break;
    }
  
  for (l = l->next; l; l = l->next)
    {
      child = l->data;

      if (!btk_widget_get_visible (child->widget))
	continue;

      if (frame->pack == BTK_PACK_START || child->pack == BTK_PACK_END)
	return TRUE;
    }

  return FALSE;
}

static void
btk_statusbar_size_allocate  (BtkWidget     *widget,
                              BtkAllocation *allocation)
{
  BtkStatusbar *statusbar = BTK_STATUSBAR (widget);
  gboolean extra_children = FALSE;
  BdkRectangle rect;

  if (statusbar->has_resize_grip)
    {
      get_grip_rect (statusbar, &rect);

      extra_children = has_extra_children (statusbar);

      /* If there are extra children, we don't want them to occupy
       * the space where we draw the resize grip, so we temporarily
       * shrink the allocation.
       * If there are no extra children, we want the frame to get
       * the full allocation, and we fix up the allocation of the
       * label afterwards to make room for the grip.
       */
      if (extra_children)
	{
	  allocation->width -= rect.width;
	  if (btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL) 
	    allocation->x += rect.width;
	}
    }

  /* chain up normally */
  BTK_WIDGET_CLASS (btk_statusbar_parent_class)->size_allocate (widget, allocation);

  if (statusbar->has_resize_grip)
    {
      if (extra_children) 
	{
	  allocation->width += rect.width;
	  if (btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL) 
	    allocation->x -= rect.width;
	  
	  widget->allocation = *allocation;
	}
      else
	{
	  BtkWidget *child;

	  /* Use the frame's child instead of statusbar->label directly, in case
	   * the label has been replaced by a container as the frame's child
	   * (and the label reparented into that container).
	   */
	  child = btk_bin_get_child (BTK_BIN (statusbar->frame));

	  if (child->allocation.width + rect.width > statusbar->frame->allocation.width)
	    {
	      /* shrink the label to make room for the grip */
	      *allocation = child->allocation;
	      allocation->width = MAX (1, allocation->width - rect.width);
	      if (btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL)
		allocation->x += child->allocation.width - allocation->width;

	      btk_widget_size_allocate (child, allocation);
	    }
	}

      if (statusbar->grip_window)
	{
          get_grip_rect (statusbar, &rect);

	  bdk_window_raise (statusbar->grip_window);
	  bdk_window_move_resize (statusbar->grip_window,
				  rect.x, rect.y,
				  rect.width, rect.height);
	}

    }
}

static void
label_selectable_changed (BtkWidget  *label,
			  BParamSpec *pspec,
			  gpointer    data)
{
  BtkStatusbar *statusbar = BTK_STATUSBAR (data);

  if (statusbar && 
      statusbar->has_resize_grip && statusbar->grip_window)
    bdk_window_raise (statusbar->grip_window);
}

#define __BTK_STATUSBAR_C__
#include "btkaliasdef.c"
