/* BAIL - The BUNNY Accessibility Implementation Library
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

#include <stdlib.h>
#include <string.h>
#include <btk/btk.h>
#include "bailutil.h"
#include "bailtoplevel.h"
#include "bailwindow.h"
#include "bail-private-macros.h"

static void		bail_util_class_init			(BailUtilClass		*klass);
static void             bail_util_init                          (BailUtil               *utils);
/* batkutil.h */

static buint		bail_util_add_global_event_listener	(GSignalEmissionHook	listener,
							         const bchar*           event_type);
static void 		bail_util_remove_global_event_listener	(buint			remove_listener);
static buint		bail_util_add_key_event_listener	(BatkKeySnoopFunc	listener,
								 bpointer               data);
static void 		bail_util_remove_key_event_listener	(buint			remove_listener);
static BatkObject*	bail_util_get_root			(void);
static const bchar *    bail_util_get_toolkit_name		(void);
static const bchar *    bail_util_get_toolkit_version      (void);

/* bailmisc/BatkMisc */
static void		bail_misc_class_init			(BailMiscClass		*klass);
static void             bail_misc_init                          (BailMisc               *misc);

static void bail_misc_threads_enter (BatkMisc *misc);
static void bail_misc_threads_leave (BatkMisc *misc);

/* Misc */

static void		_listener_info_destroy			(bpointer		data);
static buint            add_listener	                        (GSignalEmissionHook    listener,
                                                                 const bchar            *object_type,
                                                                 const bchar            *signal,
                                                                 const bchar            *hook_data);
static void             do_window_event_initialization          (void);
static bboolean         state_event_watcher                     (GSignalInvocationHint  *hint,
                                                                 buint                  n_param_values,
                                                                 const BValue           *param_values,
                                                                 bpointer               data);
static void             window_added                             (BatkObject             *batk_obj,
                                                                  buint                 index,
                                                                  BatkObject             *child);
static void             window_removed                           (BatkObject             *batk_obj,
                                                                  buint                 index,
                                                                  BatkObject             *child);
static bboolean        window_focus                              (BtkWidget             *widget,
                                                                  BdkEventFocus         *event);
static bboolean         configure_event_watcher                 (GSignalInvocationHint  *hint,
                                                                 buint                  n_param_values,
                                                                 const BValue           *param_values,
                                                                 bpointer               data);
                                                                  

static BatkObject* root = NULL;
static GHashTable *listener_list = NULL;
static bint listener_idx = 1;
static GSList *key_listener_list = NULL;
static buint key_snooper_id = 0;

typedef struct _BailUtilListenerInfo BailUtilListenerInfo;
typedef struct _BailKeyEventInfo BailKeyEventInfo;

struct _BailUtilListenerInfo
{
   bint key;
   buint signal_id;
   bulong hook_id;
};

struct _BailKeyEventInfo
{
  BatkKeyEventStruct *key_event;
  bpointer func_data;
};

G_DEFINE_TYPE (BailUtil, bail_util, BATK_TYPE_UTIL)

static void	 
bail_util_class_init (BailUtilClass *klass)
{
  BatkUtilClass *batk_class;
  bpointer data;

  data = g_type_class_peek (BATK_TYPE_UTIL);
  batk_class = BATK_UTIL_CLASS (data);

  batk_class->add_global_event_listener =
    bail_util_add_global_event_listener;
  batk_class->remove_global_event_listener =
    bail_util_remove_global_event_listener;
  batk_class->add_key_event_listener =
    bail_util_add_key_event_listener;
  batk_class->remove_key_event_listener =
    bail_util_remove_key_event_listener;
  batk_class->get_root = bail_util_get_root;
  batk_class->get_toolkit_name = bail_util_get_toolkit_name;
  batk_class->get_toolkit_version = bail_util_get_toolkit_version;

  listener_list = g_hash_table_new_full(g_int_hash, g_int_equal, NULL, 
     _listener_info_destroy);
}

static void
bail_util_init (BailUtil *utils)
{
}

static buint
bail_util_add_global_event_listener (GSignalEmissionHook listener,
				     const bchar *event_type)
{
  buint rc = 0;
  bchar **split_string;

  split_string = g_strsplit (event_type, ":", 3);

  if (split_string)
    {
      if (!strcmp ("window", split_string[0]))
        {
          static bboolean initialized = FALSE;

          if (!initialized)
            {
              do_window_event_initialization ();
              initialized = TRUE;
            }
          rc = add_listener (listener, "BailWindow", split_string[1], event_type);
        }
      else
        {
          rc = add_listener (listener, split_string[1], split_string[2], event_type);
        }

      g_strfreev (split_string);
    }

  return rc;
}

static void
bail_util_remove_global_event_listener (buint remove_listener)
{
  if (remove_listener > 0)
  {
    BailUtilListenerInfo *listener_info;
    bint tmp_idx = remove_listener;

    listener_info = (BailUtilListenerInfo *)
      g_hash_table_lookup(listener_list, &tmp_idx);

    if (listener_info != NULL)
      {
        /* Hook id of 0 and signal id of 0 are invalid */
        if (listener_info->hook_id != 0 && listener_info->signal_id != 0)
          {
            /* Remove the emission hook */
            g_signal_remove_emission_hook(listener_info->signal_id,
              listener_info->hook_id);

            /* Remove the element from the hash */
            g_hash_table_remove(listener_list, &tmp_idx);
          }
        else
          {
            g_warning("Invalid listener hook_id %ld or signal_id %d\n",
              listener_info->hook_id, listener_info->signal_id);
          }
      }
    else
      {
        g_warning("No listener with the specified listener id %d", 
          remove_listener);
      }
  }
  else
  {
    g_warning("Invalid listener_id %d", remove_listener);
  }
}


static
BatkKeyEventStruct *
batk_key_event_from_bdk_event_key (BdkEventKey *key)
{
  BatkKeyEventStruct *event = g_new0 (BatkKeyEventStruct, 1);
  switch (key->type)
    {
    case BDK_KEY_PRESS:
	    event->type = BATK_KEY_EVENT_PRESS;
	    break;
    case BDK_KEY_RELEASE:
	    event->type = BATK_KEY_EVENT_RELEASE;
	    break;
    default:
	    g_assert_not_reached ();
	    return NULL;
    }
  event->state = key->state;
  event->keyval = key->keyval;
  event->length = key->length;
  if (key->string && key->string [0] && 
      g_unichar_isgraph (g_utf8_get_char (key->string)))
    {
      event->string = key->string;
    }
  else if (key->type == BDK_KEY_PRESS ||
           key->type == BDK_KEY_RELEASE)
    {
      event->string = bdk_keyval_name (key->keyval);	    
    }
  event->keycode = key->hardware_keycode;
  event->timestamp = key->time;
#ifdef BAIL_DEBUG  
  g_print ("BailKey:\tsym %u\n\tmods %x\n\tcode %u\n\ttime %lx\n",
	   (unsigned int) event->keyval,
	   (unsigned int) event->state,
	   (unsigned int) event->keycode,
	   (unsigned long int) event->timestamp);
#endif
  return event;
}

typedef struct {
  BatkKeySnoopFunc func;
  bpointer        data;
  buint           key;
} KeyEventListener;

static bint
bail_key_snooper (BtkWidget *the_widget, BdkEventKey *event, bpointer data)
{
  GSList *l;
  BatkKeyEventStruct *batk_event;
  bboolean result;

  batk_event = batk_key_event_from_bdk_event_key (event);

  result = FALSE;

  for (l = key_listener_list; l; l = l->next)
    {
      KeyEventListener *listener = l->data;

      result |= listener->func (batk_event, listener->data);
    }
  g_free (batk_event);

  return result;
}

static buint
bail_util_add_key_event_listener (BatkKeySnoopFunc  listener_func,
                                  bpointer         listener_data)
{
  static buint key = 0;
  KeyEventListener *listener;

  if (key_snooper_id == 0)
    key_snooper_id = btk_key_snooper_install (bail_key_snooper, NULL);

  key++;

  listener = g_slice_new0 (KeyEventListener);
  listener->func = listener_func;
  listener->data = listener_data;
  listener->key = key;

  key_listener_list = b_slist_append (key_listener_list, listener);

  return key;
}

static void
bail_util_remove_key_event_listener (buint listener_key)
{
  GSList *l;

  for (l = key_listener_list; l; l = l->next)
    {
      KeyEventListener *listener = l->data;

      if (listener->key == listener_key)
        {
          g_slice_free (KeyEventListener, listener);
          key_listener_list = b_slist_delete_link (key_listener_list, l);

          break;
        }
    }

  if (key_listener_list == NULL)
    {
      btk_key_snooper_remove (key_snooper_id);
      key_snooper_id = 0;
    }
}

static BatkObject*
bail_util_get_root (void)
{
  if (!root)
    {
      root = g_object_new (BAIL_TYPE_TOPLEVEL, NULL);
      batk_object_initialize (root, NULL);
    }

  return root;
}

static const bchar *
bail_util_get_toolkit_name (void)
{
  return "BAIL";
}

static const bchar *
bail_util_get_toolkit_version (void)
{
 /*
  * Version is passed in as a -D flag when this file is
  * compiled.
  */
  return BTK_VERSION;
}

static void
_listener_info_destroy (bpointer data)
{
   g_free(data);
}

static buint
add_listener (GSignalEmissionHook listener,
              const bchar         *object_type,
              const bchar         *signal,
              const bchar         *hook_data)
{
  GType type;
  buint signal_id;
  bint  rc = 0;

  type = g_type_from_name (object_type);
  if (type)
    {
      signal_id  = g_signal_lookup (signal, type);
      if (signal_id > 0)
        {
          BailUtilListenerInfo *listener_info;

          rc = listener_idx;

          listener_info = g_malloc(sizeof(BailUtilListenerInfo));
          listener_info->key = listener_idx;
          listener_info->hook_id =
                          g_signal_add_emission_hook (signal_id, 0, listener,
			        		      g_strdup (hook_data),
			        		      (GDestroyNotify) g_free);
          listener_info->signal_id = signal_id;

	  g_hash_table_insert(listener_list, &(listener_info->key), listener_info);
          listener_idx++;
        }
      else
        {
          g_warning("Invalid signal type %s\n", signal);
        }
    }
  else
    {
      g_warning("Invalid object type %s\n", object_type);
    }
  return rc;
}

static void
do_window_event_initialization (void)
{
  BatkObject *root;

  /*
   * Ensure that BailWindowClass exists.
   */
  g_type_class_ref (BAIL_TYPE_WINDOW);
  g_signal_add_emission_hook (g_signal_lookup ("window-state-event", BTK_TYPE_WIDGET),
                              0, state_event_watcher, NULL, (GDestroyNotify) NULL);
  g_signal_add_emission_hook (g_signal_lookup ("configure-event", BTK_TYPE_WIDGET),
                              0, configure_event_watcher, NULL, (GDestroyNotify) NULL);

  root = batk_get_root ();
  g_signal_connect (root, "children-changed::add",
                    (GCallback) window_added, NULL);
  g_signal_connect (root, "children-changed::remove",
                    (GCallback) window_removed, NULL);
}

static bboolean
state_event_watcher (GSignalInvocationHint  *hint,
                     buint                  n_param_values,
                     const BValue           *param_values,
                     bpointer               data)
{
  BObject *object;
  BtkWidget *widget;
  BatkObject *batk_obj;
  BatkObject *parent;
  BdkEventWindowState *event;
  bchar *signal_name;
  buint signal_id;

  object = b_value_get_object (param_values + 0);
  /*
   * The object can be a BtkMenu when it is popped up; we ignore this
   */
  if (!BTK_IS_WINDOW (object))
    return FALSE;

  event = b_value_get_boxed (param_values + 1);
  bail_return_val_if_fail (event->type == BDK_WINDOW_STATE, FALSE);
  widget = BTK_WIDGET (object);

  if (event->new_window_state & BDK_WINDOW_STATE_MAXIMIZED)
    {
      signal_name = "maximize";
    }
  else if (event->new_window_state & BDK_WINDOW_STATE_ICONIFIED)
    {
      signal_name = "minimize";
    }
  else if (event->new_window_state == 0)
    {
      signal_name = "restore";
    }
  else
    return TRUE;
  
  batk_obj = btk_widget_get_accessible (widget);

  if (BAIL_IS_WINDOW (batk_obj))
    {
      parent = batk_object_get_parent (batk_obj);
      if (parent == batk_get_root ())
	{
	  signal_id = g_signal_lookup (signal_name, BAIL_TYPE_WINDOW); 
	  g_signal_emit (batk_obj, signal_id, 0);
	}
      
      return TRUE;
    }
  else
    {
      return FALSE;
    }
}

static void
window_added (BatkObject *batk_obj,
              buint     index,
              BatkObject *child)
{
  BtkWidget *widget;

  if (!BAIL_IS_WINDOW (child)) return;

  widget = BTK_ACCESSIBLE (child)->widget;
  bail_return_if_fail (widget);

  g_signal_connect (widget, "focus-in-event",  
                    (GCallback) window_focus, NULL);
  g_signal_connect (widget, "focus-out-event",  
                    (GCallback) window_focus, NULL);
  g_signal_emit (child, g_signal_lookup ("create", BAIL_TYPE_WINDOW), 0); 
}


static void
window_removed (BatkObject *batk_obj,
                 buint     index,
                 BatkObject *child)
{
  BtkWidget *widget;
  BtkWindow *window;

  if (!BAIL_IS_WINDOW (child)) return;

  widget = BTK_ACCESSIBLE (child)->widget;
  bail_return_if_fail (widget);

  window = BTK_WINDOW (widget);
  /*
   * Deactivate window if it is still focused and we are removing it. This
   * can happen when a dialog displayed by gok is removed.
   */
  if (window->is_active &&
      window->has_toplevel_focus)
    {
      bchar *signal_name;
      BatkObject *batk_obj;

      batk_obj = btk_widget_get_accessible (widget);
      signal_name =  "deactivate";
      g_signal_emit (batk_obj, g_signal_lookup (signal_name, BAIL_TYPE_WINDOW), 0); 
    }

  g_signal_handlers_disconnect_by_func (widget, (bpointer) window_focus, NULL);
  g_signal_emit (child, g_signal_lookup ("destroy", BAIL_TYPE_WINDOW), 0); 
}

static bboolean
window_focus (BtkWidget     *widget,
              BdkEventFocus *event)
{
  bchar *signal_name;
  BatkObject *batk_obj;

  g_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);

  batk_obj = btk_widget_get_accessible (widget);
  signal_name =  (event->in) ? "activate" : "deactivate";
  g_signal_emit (batk_obj, g_signal_lookup (signal_name, BAIL_TYPE_WINDOW), 0); 

  return FALSE;
}

static bboolean 
configure_event_watcher (GSignalInvocationHint  *hint,
                         buint                  n_param_values,
                         const BValue           *param_values,
                         bpointer               data)
{
  BObject *object;
  BtkWidget *widget;
  BatkObject *batk_obj;
  BatkObject *parent;
  BdkEvent *event;
  bchar *signal_name;
  buint signal_id;

  object = b_value_get_object (param_values + 0);
  if (!BTK_IS_WINDOW (object))
    /*
     * BtkDrawingArea can send a BDK_CONFIGURE event but we ignore here
     */
    return FALSE;

  event = b_value_get_boxed (param_values + 1);
  if (event->type != BDK_CONFIGURE)
    return FALSE;
  if (BTK_WINDOW (object)->configure_request_count)
    /*
     * There is another ConfigureRequest pending so we ignore this one.
     */
    return TRUE;
  widget = BTK_WIDGET (object);
  if (widget->allocation.x == ((BdkEventConfigure *)event)->x &&
      widget->allocation.y == ((BdkEventConfigure *)event)->y &&
      widget->allocation.width == ((BdkEventConfigure *)event)->width &&
      widget->allocation.height == ((BdkEventConfigure *)event)->height)
    return TRUE;

  if (widget->allocation.width != ((BdkEventConfigure *)event)->width ||
      widget->allocation.height != ((BdkEventConfigure *)event)->height)
    {
      signal_name = "resize";
    }
  else
    {
      signal_name = "move";
    }

  batk_obj = btk_widget_get_accessible (widget);
  if (BAIL_IS_WINDOW (batk_obj))
    {
      parent = batk_object_get_parent (batk_obj);
      if (parent == batk_get_root ())
	{
	  signal_id = g_signal_lookup (signal_name, BAIL_TYPE_WINDOW); 
	  g_signal_emit (batk_obj, signal_id, 0);
	}
      
      return TRUE;
    }
  else
    {
      return FALSE;
    }
}

G_DEFINE_TYPE (BailMisc, bail_misc, BATK_TYPE_MISC)

static void	 
bail_misc_class_init (BailMiscClass *klass)
{
  BatkMiscClass *miscclass = BATK_MISC_CLASS (klass);
  miscclass->threads_enter =
    bail_misc_threads_enter;
  miscclass->threads_leave =
    bail_misc_threads_leave;
  batk_misc_instance = g_object_new (BAIL_TYPE_MISC, NULL);
}

static void
bail_misc_init (BailMisc *misc)
{
}

static void bail_misc_threads_enter (BatkMisc *misc)
{
  BDK_THREADS_ENTER ();
}

static void bail_misc_threads_leave (BatkMisc *misc)
{
  BDK_THREADS_LEAVE ();
}
