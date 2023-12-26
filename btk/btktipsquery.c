/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * BtkQueryTips: Query onscreen widgets for their tooltips
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
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#undef BTK_DISABLE_DEPRECATED

#include "config.h"
#include "btktipsquery.h"
#include "btksignal.h"
#include "btktooltips.h"
#include "btkmain.h"
#include "btkmarshalers.h"
#include "btkintl.h"

#include "btkalias.h"



/* --- arguments --- */
enum {
  ARG_0,
  ARG_EMIT_ALWAYS,
  ARG_CALLER,
  ARG_LABEL_INACTIVE,
  ARG_LABEL_NO_TIP
};


/* --- signals --- */
enum
{
  SIGNAL_START_QUERY,
  SIGNAL_STOP_QUERY,
  SIGNAL_WIDGET_ENTERED,
  SIGNAL_WIDGET_SELECTED,
  SIGNAL_LAST
};

/* --- prototypes --- */
static void	btk_tips_query_class_init	(BtkTipsQueryClass	*class);
static void	btk_tips_query_init		(BtkTipsQuery		*tips_query);
static void	btk_tips_query_destroy		(BtkObject		*object);
static bint	btk_tips_query_event		(BtkWidget		*widget,
						 BdkEvent		*event);
static void	btk_tips_query_set_arg		(BtkObject              *object,
						 BtkArg			*arg,
						 buint			 arg_id);
static void	btk_tips_query_get_arg		(BtkObject              *object,
						 BtkArg			*arg,
						 buint			arg_id);
static void	btk_tips_query_real_start_query	(BtkTipsQuery		*tips_query);
static void	btk_tips_query_real_stop_query	(BtkTipsQuery		*tips_query);
static void	btk_tips_query_widget_entered	(BtkTipsQuery		*tips_query,
						 BtkWidget		*widget,
						 const bchar		*tip_text,
						 const bchar		*tip_private);


/* --- variables --- */
static BtkLabelClass	*parent_class = NULL;
static buint		 tips_query_signals[SIGNAL_LAST] = { 0 };


/* --- functions --- */
BtkType
btk_tips_query_get_type (void)
{
  static BtkType tips_query_type = 0;

  if (!tips_query_type)
    {
      static const BtkTypeInfo tips_query_info =
      {
	"BtkTipsQuery",
	sizeof (BtkTipsQuery),
	sizeof (BtkTipsQueryClass),
	(BtkClassInitFunc) btk_tips_query_class_init,
	(BtkObjectInitFunc) btk_tips_query_init,
        /* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(BtkClassInitFunc) NULL,
      };

      I_("BtkTipsQuery");
      tips_query_type = btk_type_unique (btk_label_get_type (), &tips_query_info);
    }

  return tips_query_type;
}

static void
btk_tips_query_class_init (BtkTipsQueryClass *class)
{
  BtkObjectClass *object_class;
  BtkWidgetClass *widget_class;

  object_class = (BtkObjectClass*) class;
  widget_class = (BtkWidgetClass*) class;

  parent_class = btk_type_class (btk_label_get_type ());


  object_class->set_arg = btk_tips_query_set_arg;
  object_class->get_arg = btk_tips_query_get_arg;
  object_class->destroy = btk_tips_query_destroy;

  widget_class->event = btk_tips_query_event;

  class->start_query = btk_tips_query_real_start_query;
  class->stop_query = btk_tips_query_real_stop_query;
  class->widget_entered = btk_tips_query_widget_entered;
  class->widget_selected = NULL;

  btk_object_add_arg_type ("BtkTipsQuery::emit-always", BTK_TYPE_BOOL, BTK_ARG_READWRITE | G_PARAM_STATIC_NAME, ARG_EMIT_ALWAYS);
  btk_object_add_arg_type ("BtkTipsQuery::caller", BTK_TYPE_WIDGET, BTK_ARG_READWRITE | G_PARAM_STATIC_NAME, ARG_CALLER);
  btk_object_add_arg_type ("BtkTipsQuery::label-inactive", BTK_TYPE_STRING, BTK_ARG_READWRITE | G_PARAM_STATIC_NAME, ARG_LABEL_INACTIVE);
  btk_object_add_arg_type ("BtkTipsQuery::label-no-tip", BTK_TYPE_STRING, BTK_ARG_READWRITE | G_PARAM_STATIC_NAME, ARG_LABEL_NO_TIP);

  tips_query_signals[SIGNAL_START_QUERY] =
    btk_signal_new (I_("start-query"),
		    BTK_RUN_FIRST,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkTipsQueryClass, start_query),
		    _btk_marshal_VOID__VOID,
		    BTK_TYPE_NONE, 0);
  tips_query_signals[SIGNAL_STOP_QUERY] =
    btk_signal_new (I_("stop-query"),
		    BTK_RUN_FIRST,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkTipsQueryClass, stop_query),
		    _btk_marshal_VOID__VOID,
		    BTK_TYPE_NONE, 0);
  tips_query_signals[SIGNAL_WIDGET_ENTERED] =
    btk_signal_new (I_("widget-entered"),
		    BTK_RUN_LAST,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkTipsQueryClass, widget_entered),
		    _btk_marshal_VOID__OBJECT_STRING_STRING,
		    BTK_TYPE_NONE, 3,
		    BTK_TYPE_WIDGET,
		    BTK_TYPE_STRING,
		    BTK_TYPE_STRING);
  tips_query_signals[SIGNAL_WIDGET_SELECTED] =
    g_signal_new (I_("widget-selected"),
                  B_TYPE_FROM_CLASS(object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET(BtkTipsQueryClass, widget_selected),
                  _btk_boolean_handled_accumulator, NULL,
                  _btk_marshal_BOOLEAN__OBJECT_STRING_STRING_BOXED,
                  B_TYPE_BOOLEAN, 4,
                  BTK_TYPE_WIDGET,
                  B_TYPE_STRING,
                  B_TYPE_STRING,
                  BDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
}

static void
btk_tips_query_init (BtkTipsQuery *tips_query)
{
  tips_query->emit_always = FALSE;
  tips_query->in_query = FALSE;
  tips_query->label_inactive = g_strdup ("");
  tips_query->label_no_tip = g_strdup (_("--- No Tip ---"));
  tips_query->caller = NULL;
  tips_query->last_crossed = NULL;
  tips_query->query_cursor = NULL;

  btk_label_set_text (BTK_LABEL (tips_query), tips_query->label_inactive);
}

static void
btk_tips_query_set_arg (BtkObject              *object,
			BtkArg                 *arg,
			buint                   arg_id)
{
  BtkTipsQuery *tips_query;

  tips_query = BTK_TIPS_QUERY (object);

  switch (arg_id)
    {
    case ARG_EMIT_ALWAYS:
      tips_query->emit_always = (BTK_VALUE_BOOL (*arg) != FALSE);
      break;
    case ARG_CALLER:
      btk_tips_query_set_caller (tips_query, BTK_WIDGET (BTK_VALUE_OBJECT (*arg)));
      break;
    case ARG_LABEL_INACTIVE:
      btk_tips_query_set_labels (tips_query, BTK_VALUE_STRING (*arg), tips_query->label_no_tip);
      break;
    case ARG_LABEL_NO_TIP:
      btk_tips_query_set_labels (tips_query, tips_query->label_inactive, BTK_VALUE_STRING (*arg));
      break;
    default:
      break;
    }
}

static void
btk_tips_query_get_arg (BtkObject             *object,
			BtkArg                *arg,
			buint                  arg_id)
{
  BtkTipsQuery *tips_query;

  tips_query = BTK_TIPS_QUERY (object);

  switch (arg_id)
    {
    case ARG_EMIT_ALWAYS:
      BTK_VALUE_BOOL (*arg) = tips_query->emit_always;
      break;
    case ARG_CALLER:
      BTK_VALUE_OBJECT (*arg) = (BtkObject*) tips_query->caller;
      break;
    case ARG_LABEL_INACTIVE:
      BTK_VALUE_STRING (*arg) = g_strdup (tips_query->label_inactive);
      break;
    case ARG_LABEL_NO_TIP:
      BTK_VALUE_STRING (*arg) = g_strdup (tips_query->label_no_tip);
      break;
    default:
      arg->type = BTK_TYPE_INVALID;
      break;
    }
}

static void
btk_tips_query_destroy (BtkObject	*object)
{
  BtkTipsQuery *tips_query = BTK_TIPS_QUERY (object);

  if (tips_query->in_query)
    btk_tips_query_stop_query (tips_query);

  btk_tips_query_set_caller (tips_query, NULL);

  g_free (tips_query->label_inactive);
  tips_query->label_inactive = NULL;
  g_free (tips_query->label_no_tip);
  tips_query->label_no_tip = NULL;

  BTK_OBJECT_CLASS (parent_class)->destroy (object);
}

BtkWidget*
btk_tips_query_new (void)
{
  BtkTipsQuery *tips_query;

  tips_query = btk_type_new (btk_tips_query_get_type ());

  return BTK_WIDGET (tips_query);
}

void
btk_tips_query_set_labels (BtkTipsQuery   *tips_query,
			   const bchar	  *label_inactive,
			   const bchar	  *label_no_tip)
{
  bchar *old;

  g_return_if_fail (BTK_IS_TIPS_QUERY (tips_query));
  g_return_if_fail (label_inactive != NULL);
  g_return_if_fail (label_no_tip != NULL);

  old = tips_query->label_inactive;
  tips_query->label_inactive = g_strdup (label_inactive);
  g_free (old);
  old = tips_query->label_no_tip;
  tips_query->label_no_tip = g_strdup (label_no_tip);
  g_free (old);
}

void
btk_tips_query_set_caller (BtkTipsQuery   *tips_query,
			   BtkWidget	   *caller)
{
  g_return_if_fail (BTK_IS_TIPS_QUERY (tips_query));
  g_return_if_fail (tips_query->in_query == FALSE);
  if (caller)
    g_return_if_fail (BTK_IS_WIDGET (caller));

  if (caller)
    g_object_ref (caller);

  if (tips_query->caller)
    g_object_unref (tips_query->caller);

  tips_query->caller = caller;
}

void
btk_tips_query_start_query (BtkTipsQuery *tips_query)
{
  g_return_if_fail (BTK_IS_TIPS_QUERY (tips_query));
  g_return_if_fail (tips_query->in_query == FALSE);
  g_return_if_fail (btk_widget_get_realized (BTK_WIDGET (tips_query)));

  tips_query->in_query = TRUE;
  btk_signal_emit (BTK_OBJECT (tips_query), tips_query_signals[SIGNAL_START_QUERY]);
}

void
btk_tips_query_stop_query (BtkTipsQuery *tips_query)
{
  g_return_if_fail (BTK_IS_TIPS_QUERY (tips_query));
  g_return_if_fail (tips_query->in_query == TRUE);

  btk_signal_emit (BTK_OBJECT (tips_query), tips_query_signals[SIGNAL_STOP_QUERY]);
  tips_query->in_query = FALSE;
}

static void
btk_tips_query_real_start_query (BtkTipsQuery *tips_query)
{
  bint failure;
  
  g_return_if_fail (BTK_IS_TIPS_QUERY (tips_query));
  
  tips_query->query_cursor = bdk_cursor_new_for_display (btk_widget_get_display (BTK_WIDGET (tips_query)),
							 BDK_QUESTION_ARROW);
  failure = bdk_pointer_grab (BTK_WIDGET (tips_query)->window,
			      TRUE,
			      BDK_BUTTON_PRESS_MASK | BDK_BUTTON_RELEASE_MASK |
			      BDK_ENTER_NOTIFY_MASK | BDK_LEAVE_NOTIFY_MASK,
			      NULL,
			      tips_query->query_cursor,
			      BDK_CURRENT_TIME);
  if (failure)
    {
      bdk_cursor_unref (tips_query->query_cursor);
      tips_query->query_cursor = NULL;
    }
  btk_grab_add (BTK_WIDGET (tips_query));
}

static void
btk_tips_query_real_stop_query (BtkTipsQuery *tips_query)
{
  g_return_if_fail (BTK_IS_TIPS_QUERY (tips_query));
  
  btk_grab_remove (BTK_WIDGET (tips_query));
  if (tips_query->query_cursor)
    {
      bdk_display_pointer_ungrab (btk_widget_get_display (BTK_WIDGET (tips_query)),
				  BDK_CURRENT_TIME);
      bdk_cursor_unref (tips_query->query_cursor);
      tips_query->query_cursor = NULL;
    }
  if (tips_query->last_crossed)
    {
      g_object_unref (tips_query->last_crossed);
      tips_query->last_crossed = NULL;
    }
  
  btk_label_set_text (BTK_LABEL (tips_query), tips_query->label_inactive);
}

static void
btk_tips_query_widget_entered (BtkTipsQuery   *tips_query,
			       BtkWidget      *widget,
			       const bchar    *tip_text,
			       const bchar    *tip_private)
{
  g_return_if_fail (BTK_IS_TIPS_QUERY (tips_query));

  if (!tip_text)
    tip_text = tips_query->label_no_tip;

  if (!g_str_equal (BTK_LABEL (tips_query)->label, (bchar*) tip_text))
    btk_label_set_text (BTK_LABEL (tips_query), tip_text);
}

static void
btk_tips_query_emit_widget_entered (BtkTipsQuery *tips_query,
				    BtkWidget	 *widget)
{
  BtkTooltipsData *tdata;

  if (widget == (BtkWidget*) tips_query)
    widget = NULL;

  if (widget)
    tdata = btk_tooltips_data_get (widget);
  else
    tdata = NULL;

  if (!widget && tips_query->last_crossed)
    {
      btk_signal_emit (BTK_OBJECT (tips_query),
		       tips_query_signals[SIGNAL_WIDGET_ENTERED],
		       NULL,
		       NULL,
		       NULL);
      g_object_unref (tips_query->last_crossed);
      tips_query->last_crossed = NULL;
    }
  else if (widget && widget != tips_query->last_crossed)
    {
      g_object_ref (widget);
      if (tdata || tips_query->emit_always)
	  btk_signal_emit (BTK_OBJECT (tips_query),
			   tips_query_signals[SIGNAL_WIDGET_ENTERED],
			   widget,
			   tdata ? tdata->tip_text : NULL,
			   tdata ? tdata->tip_private : NULL);
      if (tips_query->last_crossed)
	g_object_unref (tips_query->last_crossed);
      tips_query->last_crossed = widget;
    }
}

static bint
btk_tips_query_event (BtkWidget	       *widget,
		      BdkEvent	       *event)
{
  BtkTipsQuery *tips_query;
  BtkWidget *event_widget;
  bboolean event_handled;
  
  g_return_val_if_fail (BTK_IS_TIPS_QUERY (widget), FALSE);

  tips_query = BTK_TIPS_QUERY (widget);
  if (!tips_query->in_query)
    {
      if (BTK_WIDGET_CLASS (parent_class)->event)
	return BTK_WIDGET_CLASS (parent_class)->event (widget, event);
      else
	return FALSE;
    }

  event_widget = btk_get_event_widget (event);

  event_handled = FALSE;
  switch (event->type)
    {
      BdkWindow *pointer_window;
      
    case  BDK_LEAVE_NOTIFY:
      if (event_widget)
	pointer_window = bdk_window_get_pointer (event_widget->window, NULL, NULL, NULL);
      else
	pointer_window = NULL;
      event_widget = NULL;
      if (pointer_window)
	{
	  bpointer event_widget_ptr;
	  bdk_window_get_user_data (pointer_window, &event_widget_ptr);
	  event_widget = event_widget_ptr;
	}
      btk_tips_query_emit_widget_entered (tips_query, event_widget);
      event_handled = TRUE;
      break;

    case  BDK_ENTER_NOTIFY:
      btk_tips_query_emit_widget_entered (tips_query, event_widget);
      event_handled = TRUE;
      break;

    case  BDK_BUTTON_PRESS:
    case  BDK_BUTTON_RELEASE:
      if (event_widget)
	{
	  if (event_widget == (BtkWidget*) tips_query ||
	      event_widget == tips_query->caller)
	    btk_tips_query_stop_query (tips_query);
	  else
	    {
	      bint stop;
	      BtkTooltipsData *tdata;
	      
	      stop = TRUE;
	      tdata = btk_tooltips_data_get (event_widget);
	      if (tdata || tips_query->emit_always)
		btk_signal_emit (BTK_OBJECT (tips_query),
				 tips_query_signals[SIGNAL_WIDGET_SELECTED],
				 event_widget,
				 tdata ? tdata->tip_text : NULL,
				 tdata ? tdata->tip_private : NULL,
				 event,
				 &stop);
	      
	      if (stop)
		btk_tips_query_stop_query (tips_query);
	    }
	}
      event_handled = TRUE;
      break;

    default:
      break;
    }

  return event_handled;
}

#define __BTK_TIPS_QUERY_C__
#include "btkaliasdef.c"
