/* BTK - The GIMP Toolkit
 * Copyright (C) 2000 Red Hat, Inc.
 * Copyright (C) 2004 Nokia Corporation
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
 *
 * Global clipboard abstraction. 
 */

#include "config.h"
#include <string.h>

#include "btkclipboard.h"
#include "btkinvisible.h"
#include "btkmain.h"
#include "btkmarshalers.h"
#include "btktextbufferrichtext.h"
#include "btkintl.h"
#include "btkalias.h"

#ifdef BDK_WINDOWING_X11
#include "x11/bdkx.h"
#endif

#ifdef BDK_WINDOWING_WIN32
#include "win32/bdkwin32.h"
#endif

enum {
  OWNER_CHANGE,
  LAST_SIGNAL
};

typedef struct _BtkClipboardClass BtkClipboardClass;

typedef struct _RequestContentsInfo RequestContentsInfo;
typedef struct _RequestTextInfo RequestTextInfo;
typedef struct _RequestRichTextInfo RequestRichTextInfo;
typedef struct _RequestImageInfo RequestImageInfo;
typedef struct _RequestURIInfo RequestURIInfo;
typedef struct _RequestTargetsInfo RequestTargetsInfo;

struct _BtkClipboard 
{
  BObject parent_instance;

  BdkAtom selection;

  BtkClipboardGetFunc get_func;
  BtkClipboardClearFunc clear_func;
  bpointer user_data;
  bboolean have_owner;

  buint32 timestamp;

  bboolean have_selection;
  BdkDisplay *display;

  BdkAtom *cached_targets;
  bint     n_cached_targets;

  buint      notify_signal_id;
  bboolean   storing_selection;
  GMainLoop *store_loop;
  buint      store_timeout;
  bint       n_storable_targets;
  BdkAtom   *storable_targets;
};

struct _BtkClipboardClass
{
  BObjectClass parent_class;

  void (*owner_change) (BtkClipboard        *clipboard,
			BdkEventOwnerChange *event);
};

struct _RequestContentsInfo
{
  BtkClipboardReceivedFunc callback;
  bpointer user_data;
};

struct _RequestTextInfo
{
  BtkClipboardTextReceivedFunc callback;
  bpointer user_data;
};

struct _RequestRichTextInfo
{
  BtkClipboardRichTextReceivedFunc callback;
  BdkAtom *atoms;
  bint     n_atoms;
  bint     current_atom;
  bpointer user_data;
};

struct _RequestImageInfo
{
  BtkClipboardImageReceivedFunc callback;
  bpointer user_data;
};

struct _RequestURIInfo
{
  BtkClipboardURIReceivedFunc callback;
  bpointer user_data;
};

struct _RequestTargetsInfo
{
  BtkClipboardTargetsReceivedFunc callback;
  bpointer user_data;
};

static void btk_clipboard_class_init   (BtkClipboardClass   *class);
static void btk_clipboard_finalize     (BObject             *object);
static void btk_clipboard_owner_change (BtkClipboard        *clipboard,
					BdkEventOwnerChange *event);

static void          clipboard_unset      (BtkClipboard     *clipboard);
static void          selection_received   (BtkWidget        *widget,
					   BtkSelectionData *selection_data,
					   buint             time);
static BtkClipboard *clipboard_peek       (BdkDisplay       *display,
					   BdkAtom           selection,
					   bboolean          only_if_exists);
static BtkWidget *   get_clipboard_widget (BdkDisplay       *display);


enum {
  TARGET_STRING,
  TARGET_TEXT,
  TARGET_COMPOUND_TEXT,
  TARGET_UTF8_STRING,
  TARGET_SAVE_TARGETS
};

static const bchar request_contents_key[] = "btk-request-contents";
static GQuark request_contents_key_id = 0;

static const bchar clipboards_owned_key[] = "btk-clipboards-owned";
static GQuark clipboards_owned_key_id = 0;

static buint         clipboard_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (BtkClipboard, btk_clipboard, B_TYPE_OBJECT)

static void
btk_clipboard_init (BtkClipboard *object)
{
}

static void
btk_clipboard_class_init (BtkClipboardClass *class)
{
  BObjectClass *bobject_class = B_OBJECT_CLASS (class);

  bobject_class->finalize = btk_clipboard_finalize;

  class->owner_change = btk_clipboard_owner_change;

  /**
   * BtkClipboard::owner-change:
   * @clipboard: the #BtkClipboard on which the signal is emitted
   * @event: the @BdkEventOwnerChange event 
   *
   * The ::owner-change signal is emitted when BTK+ receives an
   * event that indicates that the ownership of the selection 
   * associated with @clipboard has changed.
   *
   * Since: 2.6
   */ 
  clipboard_signals[OWNER_CHANGE] =
    g_signal_new (I_("owner-change"),
		  B_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkClipboardClass, owner_change),
		  NULL, NULL,
		  _btk_marshal_VOID__BOXED,
		  B_TYPE_NONE, 1,
		  BDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
}

static void
btk_clipboard_finalize (BObject *object)
{
  BtkClipboard *clipboard;
  BtkWidget *clipboard_widget = NULL;
  GSList *clipboards = NULL;

  clipboard = BTK_CLIPBOARD (object);

  if (clipboard->display)
    {
      clipboards = g_object_get_data (B_OBJECT (clipboard->display), "btk-clipboard-list");

      if (b_slist_index (clipboards, clipboard) >= 0)
        g_warning ("BtkClipboard prematurely finalized");

      clipboards = b_slist_remove (clipboards, clipboard);

      g_object_set_data (B_OBJECT (clipboard->display), "btk-clipboard-list", 
                         clipboards);

      /* don't use get_clipboard_widget() here because it would create the
       * widget if it doesn't exist.
       */
      clipboard_widget = g_object_get_data (B_OBJECT (clipboard->display),
                                            "btk-clipboard-widget");
    }

  clipboard_unset (clipboard);

  if (clipboard->store_loop && g_main_loop_is_running (clipboard->store_loop))
    g_main_loop_quit (clipboard->store_loop);

  if (clipboard->store_timeout != 0)
    g_source_remove (clipboard->store_timeout);

  if (clipboard->notify_signal_id != 0)
    g_signal_handler_disconnect (clipboard_widget, clipboard->notify_signal_id);
  
  g_free (clipboard->storable_targets);
  g_free (clipboard->cached_targets);

  B_OBJECT_CLASS (btk_clipboard_parent_class)->finalize (object);
}

static void
clipboard_display_closed (BdkDisplay   *display,
			  bboolean      is_error,
			  BtkClipboard *clipboard)
{
  GSList *clipboards;

  clipboards = g_object_get_data (B_OBJECT (display), "btk-clipboard-list");
  g_object_run_dispose (B_OBJECT (clipboard));
  clipboards = b_slist_remove (clipboards, clipboard);
  g_object_set_data (B_OBJECT (display), I_("btk-clipboard-list"), clipboards);
  g_object_unref (clipboard);
}

/**
 * btk_clipboard_get_for_display:
 * @display: the display for which the clipboard is to be retrieved or created
 * @selection: a #BdkAtom which identifies the clipboard
 *             to use.
 * 
 * Returns the clipboard object for the given selection.
 * Cut/copy/paste menu items and keyboard shortcuts should use
 * the default clipboard, returned by passing %BDK_SELECTION_CLIPBOARD for @selection.
 * (%BDK_NONE is supported as a synonym for BDK_SELECTION_CLIPBOARD
 * for backwards compatibility reasons.)
 * The currently-selected object or text should be provided on the clipboard
 * identified by #BDK_SELECTION_PRIMARY. Cut/copy/paste menu items
 * conceptually copy the contents of the #BDK_SELECTION_PRIMARY clipboard
 * to the default clipboard, i.e. they copy the selection to what the
 * user sees as the clipboard.
 *
 * (Passing #BDK_NONE is the same as using <literal>bdk_atom_intern
 * ("CLIPBOARD", FALSE)</literal>. See <ulink
 * url="http://www.freedesktop.org/Standards/clipboards-spec">
 * http://www.freedesktop.org/Standards/clipboards-spec</ulink>
 * for a detailed discussion of the "CLIPBOARD" vs. "PRIMARY"
 * selections under the X window system. On Win32 the
 * #BDK_SELECTION_PRIMARY clipboard is essentially ignored.)
 *
 * It's possible to have arbitrary named clipboards; if you do invent
 * new clipboards, you should prefix the selection name with an
 * underscore (because the ICCCM requires that nonstandard atoms are
 * underscore-prefixed), and namespace it as well. For example,
 * if your application called "Foo" has a special-purpose
 * clipboard, you might call it "_FOO_SPECIAL_CLIPBOARD".
 * 
 * Return value: (transfer none): the appropriate clipboard object. If no
 *             clipboard already exists, a new one will
 *             be created. Once a clipboard object has
 *             been created, it is persistent and, since
 *             it is owned by BTK+, must not be freed or
 *             unrefd.
 *
 * Since: 2.2
 **/
BtkClipboard *
btk_clipboard_get_for_display (BdkDisplay *display, 
			       BdkAtom     selection)
{
  g_return_val_if_fail (display != NULL, NULL); /* See bgo#463773; this is needed because Flash Player sucks */
  g_return_val_if_fail (BDK_IS_DISPLAY (display), NULL);
  g_return_val_if_fail (!display->closed, NULL);

  return clipboard_peek (display, selection, FALSE);
}


/**
 * btk_clipboard_get:
 * @selection: a #BdkAtom which identifies the clipboard to use
 *
 * Returns the clipboard object for the given selection.
 * See btk_clipboard_get_for_display() for complete details.
 *
 * Return value: (transfer none): the appropriate clipboard object. If no clipboard
 *     already exists, a new one will be created. Once a clipboard
 *     object has been created, it is persistent and, since it is
 *     owned by BTK+, must not be freed or unreffed.
 */
BtkClipboard *
btk_clipboard_get (BdkAtom selection)
{
  return btk_clipboard_get_for_display (bdk_display_get_default (), selection);
}

static void 
selection_get_cb (BtkWidget          *widget,
		  BtkSelectionData   *selection_data,
		  buint               info,
		  buint               time)
{
  BtkClipboard *clipboard = btk_widget_get_clipboard (widget, selection_data->selection);

  if (clipboard && clipboard->get_func)
    clipboard->get_func (clipboard, selection_data, info, clipboard->user_data);
}

static bboolean
selection_clear_event_cb (BtkWidget	    *widget,
			  BdkEventSelection *event)
{
  BtkClipboard *clipboard = btk_widget_get_clipboard (widget, event->selection);

  if (clipboard)
    {
      clipboard_unset (clipboard);

      return TRUE;
    }

  return FALSE;
}

static BtkWidget *
make_clipboard_widget (BdkDisplay *display, 
		       bboolean    provider)
{
  BtkWidget *widget = btk_invisible_new_for_screen (bdk_display_get_default_screen (display));

  g_signal_connect (widget, "selection-received",
		    G_CALLBACK (selection_received), NULL);

  if (provider)
    {
      /* We need this for bdk_x11_get_server_time() */
      btk_widget_add_events (widget, BDK_PROPERTY_CHANGE_MASK);
      
      g_signal_connect (widget, "selection-get",
			G_CALLBACK (selection_get_cb), NULL);
      g_signal_connect (widget, "selection-clear-event",
			G_CALLBACK (selection_clear_event_cb), NULL);
    }

  return widget;
}

static BtkWidget *
get_clipboard_widget (BdkDisplay *display)
{
  BtkWidget *clip_widget = g_object_get_data (B_OBJECT (display), "btk-clipboard-widget");
  if (!clip_widget)
    {
      clip_widget = make_clipboard_widget (display, TRUE);
      g_object_set_data (B_OBJECT (display), I_("btk-clipboard-widget"), clip_widget);
    }

  return clip_widget;
}

/* This function makes a very good guess at what the correct
 * timestamp for a selection request should be. If there is
 * a currently processed event, it uses the timestamp for that
 * event, otherwise it uses the current server time. However,
 * if the time resulting from that is older than the time used
 * last time, it uses the time used last time instead.
 *
 * In order implement this correctly, we never use CurrentTime,
 * but actually retrieve the actual timestamp from the server.
 * This is a little slower but allows us to make the guarantee
 * that the times used by this application will always ascend
 * and we won't get selections being rejected just because
 * we are using a correct timestamp from an event, but used
 * CurrentTime previously.
 */
static buint32
clipboard_get_timestamp (BtkClipboard *clipboard)
{
  BtkWidget *clipboard_widget = get_clipboard_widget (clipboard->display);
  buint32 timestamp = btk_get_current_event_time ();

  if (timestamp == BDK_CURRENT_TIME)
    {
#ifdef BDK_WINDOWING_X11
      timestamp = bdk_x11_get_server_time (clipboard_widget->window);
#elif defined BDK_WINDOWING_WIN32
      timestamp = GetMessageTime ();
#endif
    }
  else
    {
      if (clipboard->timestamp != BDK_CURRENT_TIME)
	{
	  /* Check to see if clipboard->timestamp is newer than
	   * timestamp, accounting for wraparound.
	   */

	  buint32 max = timestamp + 0x80000000;

	  if ((max > timestamp &&
	       (clipboard->timestamp > timestamp &&
		clipboard->timestamp <= max)) ||
	      (max <= timestamp &&
	       (clipboard->timestamp > timestamp ||
		clipboard->timestamp <= max)))
	    {
	      timestamp = clipboard->timestamp;
	    }
	}
    }

  clipboard->timestamp = timestamp;

  return timestamp;
}

static void
clipboard_owner_destroyed (bpointer data)
{
  GSList *clipboards = data;
  GSList *tmp_list;

  tmp_list = clipboards;
  while (tmp_list)
    {
      BtkClipboard *clipboard = tmp_list->data;

      clipboard->get_func = NULL;
      clipboard->clear_func = NULL;
      clipboard->user_data = NULL;
      clipboard->have_owner = FALSE;

      btk_clipboard_clear (clipboard);

      tmp_list = tmp_list->next;
    }
  
  b_slist_free (clipboards);
}

static void
clipboard_add_owner_notify (BtkClipboard *clipboard)
{
  if (!clipboards_owned_key_id)
    clipboards_owned_key_id = g_quark_from_static_string (clipboards_owned_key);
  
  if (clipboard->have_owner)
    g_object_set_qdata_full (clipboard->user_data, clipboards_owned_key_id,
			     b_slist_prepend (g_object_steal_qdata (clipboard->user_data,
								    clipboards_owned_key_id),
					      clipboard),
			     clipboard_owner_destroyed);
}

static void
clipboard_remove_owner_notify (BtkClipboard *clipboard)
{
  if (clipboard->have_owner)
     g_object_set_qdata_full (clipboard->user_data, clipboards_owned_key_id,
			      b_slist_remove (g_object_steal_qdata (clipboard->user_data,
								    clipboards_owned_key_id),
					      clipboard),
			      clipboard_owner_destroyed);
}
	  
static bboolean
btk_clipboard_set_contents (BtkClipboard         *clipboard,
			    const BtkTargetEntry *targets,
			    buint                 n_targets,
			    BtkClipboardGetFunc   get_func,
			    BtkClipboardClearFunc clear_func,
			    bpointer              user_data,
			    bboolean              have_owner)
{
  BtkWidget *clipboard_widget = get_clipboard_widget (clipboard->display);

  if (btk_selection_owner_set_for_display (clipboard->display,
					   clipboard_widget,
					   clipboard->selection,
					   clipboard_get_timestamp (clipboard)))
    {
      clipboard->have_selection = TRUE;

      if (clipboard->n_cached_targets != -1)
        {
          g_free (clipboard->cached_targets);
	  clipboard->cached_targets = NULL;
          clipboard->n_cached_targets = -1;
        }

      if (!(clipboard->have_owner && have_owner) ||
	  clipboard->user_data != user_data)
	{
	  clipboard_unset (clipboard);

	  if (clipboard->get_func)
	    {
	      /* Calling unset() caused the clipboard contents to be reset!
	       * Avoid leaking and return 
	       */
	      if (!(clipboard->have_owner && have_owner) ||
		  clipboard->user_data != user_data)
		{
		  (*clear_func) (clipboard, user_data);
		  return FALSE;
		}
	      else
		return TRUE;
	    }
	  else
	    {
	      clipboard->user_data = user_data;
	      clipboard->have_owner = have_owner;
	      if (have_owner)
		clipboard_add_owner_notify (clipboard);
	    }
	  
 	}

      clipboard->get_func = get_func;
      clipboard->clear_func = clear_func;

      btk_selection_clear_targets (clipboard_widget, clipboard->selection);
      btk_selection_add_targets (clipboard_widget, clipboard->selection,
				 targets, n_targets);

      return TRUE;
    }
  else
    return FALSE;
}

/**
 * btk_clipboard_set_with_data:
 * @clipboard:  a #BtkClipboard
 * @targets:    array containing information about the available forms for the
 *              clipboard data
 * @n_targets:  number of elements in @targets
 * @get_func:   function to call to get the actual clipboard data
 * @clear_func: when the clipboard contents are set again, this function will
 *              be called, and @get_func will not be subsequently called.
 * @user_data:  user data to pass to @get_func and @clear_func.
 * 
 * Virtually sets the contents of the specified clipboard by providing
 * a list of supported formats for the clipboard data and a function
 * to call to get the actual data when it is requested.
 * 
 * Return value: %TRUE if setting the clipboard data succeeded. If setting
 *               the clipboard data failed the provided callback functions
 *               will be ignored.
 **/
bboolean
btk_clipboard_set_with_data (BtkClipboard          *clipboard,
			     const BtkTargetEntry  *targets,
			     buint                  n_targets,
			     BtkClipboardGetFunc    get_func,
			     BtkClipboardClearFunc  clear_func,
			     bpointer               user_data)
{
  g_return_val_if_fail (clipboard != NULL, FALSE);
  g_return_val_if_fail (targets != NULL, FALSE);
  g_return_val_if_fail (get_func != NULL, FALSE);

  return btk_clipboard_set_contents (clipboard, targets, n_targets,
				     get_func, clear_func, user_data,
				     FALSE);
}

/**
 * btk_clipboard_set_with_owner:
 * @clipboard:  a #BtkClipboard
 * @targets:    array containing information about the available forms for the
 *              clipboard data
 * @n_targets:  number of elements in @targets
 * @get_func:   function to call to get the actual clipboard data
 * @clear_func: when the clipboard contents are set again, this function will
 *              be called, and @get_func will not be subsequently called.
 * @owner:      an object that "owns" the data. This object will be passed
 *              to the callbacks when called. 
 * 
 * Virtually sets the contents of the specified clipboard by providing
 * a list of supported formats for the clipboard data and a function
 * to call to get the actual data when it is requested.
 *
 * The difference between this function and btk_clipboard_set_with_data()
 * is that instead of an generic @user_data pointer, a #BObject is passed
 * in. 
 * 
 * Return value: %TRUE if setting the clipboard data succeeded. If setting
 *               the clipboard data failed the provided callback functions
 *               will be ignored.
 **/
bboolean
btk_clipboard_set_with_owner (BtkClipboard          *clipboard,
			      const BtkTargetEntry  *targets,
			      buint                  n_targets,
			      BtkClipboardGetFunc    get_func,
			      BtkClipboardClearFunc  clear_func,
			      BObject               *owner)
{
  g_return_val_if_fail (clipboard != NULL, FALSE);
  g_return_val_if_fail (targets != NULL, FALSE);
  g_return_val_if_fail (get_func != NULL, FALSE);
  g_return_val_if_fail (G_IS_OBJECT (owner), FALSE);

  return btk_clipboard_set_contents (clipboard, targets, n_targets,
				     get_func, clear_func, owner,
				     TRUE);
}

/**
 * btk_clipboard_get_owner:
 * @clipboard: a #BtkClipboard
 * 
 * If the clipboard contents callbacks were set with 
 * btk_clipboard_set_with_owner(), and the btk_clipboard_set_with_data() or 
 * btk_clipboard_clear() has not subsequently called, returns the owner set 
 * by btk_clipboard_set_with_owner().
 * 
 * Return value: (transfer none): the owner of the clipboard, if any; otherwise %NULL.
 **/
BObject *
btk_clipboard_get_owner (BtkClipboard *clipboard)
{
  g_return_val_if_fail (clipboard != NULL, NULL);

  if (clipboard->have_owner)
    return clipboard->user_data;
  else
    return NULL;
}

static void
clipboard_unset (BtkClipboard *clipboard)
{
  BtkClipboardClearFunc old_clear_func;
  bpointer old_data;
  bboolean old_have_owner;
  bint old_n_storable_targets;
  
  old_clear_func = clipboard->clear_func;
  old_data = clipboard->user_data;
  old_have_owner = clipboard->have_owner;
  old_n_storable_targets = clipboard->n_storable_targets;
  
  if (old_have_owner)
    {
      clipboard_remove_owner_notify (clipboard);
      clipboard->have_owner = FALSE;
    }

  clipboard->n_storable_targets = -1;
  g_free (clipboard->storable_targets);
  clipboard->storable_targets = NULL;
      
  clipboard->get_func = NULL;
  clipboard->clear_func = NULL;
  clipboard->user_data = NULL;
  
  if (old_clear_func)
    old_clear_func (clipboard, old_data);

  /* If we've transferred the clipboard data to the manager,
   * unref the owner
   */
  if (old_have_owner &&
      old_n_storable_targets != -1)
    g_object_unref (old_data);
}

/**
 * btk_clipboard_clear:
 * @clipboard:  a #BtkClipboard
 * 
 * Clears the contents of the clipboard. Generally this should only
 * be called between the time you call btk_clipboard_set_with_owner()
 * or btk_clipboard_set_with_data(),
 * and when the @clear_func you supplied is called. Otherwise, the
 * clipboard may be owned by someone else.
 **/
void
btk_clipboard_clear (BtkClipboard *clipboard)
{
  g_return_if_fail (clipboard != NULL);

  if (clipboard->have_selection)
    btk_selection_owner_set_for_display (clipboard->display, 
					 NULL,
					 clipboard->selection,
					 clipboard_get_timestamp (clipboard));
}

static void 
text_get_func (BtkClipboard     *clipboard,
	       BtkSelectionData *selection_data,
	       buint             info,
	       bpointer          data)
{
  btk_selection_data_set_text (selection_data, data, -1);
}

static void 
text_clear_func (BtkClipboard *clipboard,
		 bpointer      data)
{
  g_free (data);
}


/**
 * btk_clipboard_set_text:
 * @clipboard: a #BtkClipboard object
 * @text:      a UTF-8 string.
 * @len:       length of @text, in bytes, or -1, in which case
 *             the length will be determined with <function>strlen()</function>.
 * 
 * Sets the contents of the clipboard to the given UTF-8 string. BTK+ will
 * make a copy of the text and take responsibility for responding
 * for requests for the text, and for converting the text into
 * the requested format.
 **/
void 
btk_clipboard_set_text (BtkClipboard *clipboard,
			const bchar  *text,
			bint          len)
{
  BtkTargetList *list;
  BtkTargetEntry *targets;
  bint n_targets;

  g_return_if_fail (clipboard != NULL);
  g_return_if_fail (text != NULL);

  list = btk_target_list_new (NULL, 0);
  btk_target_list_add_text_targets (list, 0);

  targets = btk_target_table_new_from_list (list, &n_targets);
  
  if (len < 0)
    len = strlen (text);
  
  btk_clipboard_set_with_data (clipboard, 
			       targets, n_targets,
			       text_get_func, text_clear_func,
			       g_strndup (text, len));
  btk_clipboard_set_can_store (clipboard, NULL, 0);

  btk_target_table_free (targets, n_targets);
  btk_target_list_unref (list);
}

static void 
pixbuf_get_func (BtkClipboard     *clipboard,
		 BtkSelectionData *selection_data,
		 buint             info,
		 bpointer          data)
{
  btk_selection_data_set_pixbuf (selection_data, data);
}

static void 
pixbuf_clear_func (BtkClipboard *clipboard,
		   bpointer      data)
{
  g_object_unref (data);
}

/**
 * btk_clipboard_set_image:
 * @clipboard: a #BtkClipboard object
 * @pixbuf:    a #BdkPixbuf 
 * 
 * Sets the contents of the clipboard to the given #BdkPixbuf. 
 * BTK+ will take responsibility for responding for requests 
 * for the image, and for converting the image into the 
 * requested format.
 * 
 * Since: 2.6
 **/
void
btk_clipboard_set_image (BtkClipboard *clipboard,
			  BdkPixbuf    *pixbuf)
{
  BtkTargetList *list;
  BtkTargetEntry *targets;
  bint n_targets;

  g_return_if_fail (clipboard != NULL);
  g_return_if_fail (BDK_IS_PIXBUF (pixbuf));

  list = btk_target_list_new (NULL, 0);
  btk_target_list_add_image_targets (list, 0, TRUE);

  targets = btk_target_table_new_from_list (list, &n_targets);

  btk_clipboard_set_with_data (clipboard, 
			       targets, n_targets,
			       pixbuf_get_func, pixbuf_clear_func,
			       g_object_ref (pixbuf));
  btk_clipboard_set_can_store (clipboard, NULL, 0);

  btk_target_table_free (targets, n_targets);
  btk_target_list_unref (list);
}

static void
set_request_contents_info (BtkWidget           *widget,
			   RequestContentsInfo *info)
{
  if (!request_contents_key_id)
    request_contents_key_id = g_quark_from_static_string (request_contents_key);

  g_object_set_qdata (B_OBJECT (widget), request_contents_key_id, info);
}

static RequestContentsInfo *
get_request_contents_info (BtkWidget *widget)
{
  if (!request_contents_key_id)
    return NULL;
  else
    return g_object_get_qdata (B_OBJECT (widget), request_contents_key_id);
}

static void 
selection_received (BtkWidget            *widget,
		    BtkSelectionData     *selection_data,
		    buint                 time)
{
  RequestContentsInfo *request_info = get_request_contents_info (widget);
  set_request_contents_info (widget, NULL);
  
  request_info->callback (btk_widget_get_clipboard (widget, selection_data->selection), 
			  selection_data,
			  request_info->user_data);

  g_free (request_info);

  if (widget != get_clipboard_widget (btk_widget_get_display (widget)))
    btk_widget_destroy (widget);
}

/**
 * btk_clipboard_request_contents:
 * @clipboard: a #BtkClipboard
 * @target:    an atom representing the form into which the clipboard
 *             owner should convert the selection.
 * @callback:  A function to call when the results are received
 *             (or the retrieval fails). If the retrieval fails
 *             the length field of @selection_data will be
 *             negative.
 * @user_data: user data to pass to @callback
 * 
 * Requests the contents of clipboard as the given target.
 * When the results of the result are later received the supplied callback
 * will be called.
 **/
void 
btk_clipboard_request_contents (BtkClipboard            *clipboard,
				BdkAtom                  target,
				BtkClipboardReceivedFunc callback,
				bpointer                 user_data)
{
  RequestContentsInfo *info;
  BtkWidget *widget;
  BtkWidget *clipboard_widget;

  g_return_if_fail (clipboard != NULL);
  g_return_if_fail (target != BDK_NONE);
  g_return_if_fail (callback != NULL);
  
  clipboard_widget = get_clipboard_widget (clipboard->display);

  if (get_request_contents_info (clipboard_widget))
    widget = make_clipboard_widget (clipboard->display, FALSE);
  else
    widget = clipboard_widget;

  info = g_new (RequestContentsInfo, 1);
  info->callback = callback;
  info->user_data = user_data;

  set_request_contents_info (widget, info);

  btk_selection_convert (widget, clipboard->selection, target,
			 clipboard_get_timestamp (clipboard));
}

static void 
request_text_received_func (BtkClipboard     *clipboard,
			    BtkSelectionData *selection_data,
			    bpointer          data)
{
  RequestTextInfo *info = data;
  bchar *result = NULL;

  result = (bchar *) btk_selection_data_get_text (selection_data);

  if (!result)
    {
      /* If we asked for UTF8 and didn't get it, try compound_text;
       * if we asked for compound_text and didn't get it, try string;
       * If we asked for anything else and didn't get it, give up.
       */
      if (selection_data->target == bdk_atom_intern_static_string ("UTF8_STRING"))
	{
	  btk_clipboard_request_contents (clipboard,
					  bdk_atom_intern_static_string ("COMPOUND_TEXT"), 
					  request_text_received_func, info);
	  return;
	}
      else if (selection_data->target == bdk_atom_intern_static_string ("COMPOUND_TEXT"))
	{
	  btk_clipboard_request_contents (clipboard,
					  BDK_TARGET_STRING, 
					  request_text_received_func, info);
	  return;
	}
    }

  info->callback (clipboard, result, info->user_data);
  g_free (info);
  g_free (result);
}

/**
 * btk_clipboard_request_text:
 * @clipboard: a #BtkClipboard
 * @callback:  a function to call when the text is received,
 *             or the retrieval fails. (It will always be called
 *             one way or the other.)
 * @user_data: user data to pass to @callback.
 * 
 * Requests the contents of the clipboard as text. When the text is
 * later received, it will be converted to UTF-8 if necessary, and
 * @callback will be called. 
 *
 * The @text parameter to @callback will contain the resulting text if
 * the request succeeded, or %NULL if it failed. This could happen for
 * various reasons, in particular if the clipboard was empty or if the
 * contents of the clipboard could not be converted into text form.
 **/
void 
btk_clipboard_request_text (BtkClipboard                *clipboard,
			    BtkClipboardTextReceivedFunc callback,
			    bpointer                     user_data)
{
  RequestTextInfo *info;
  
  g_return_if_fail (clipboard != NULL);
  g_return_if_fail (callback != NULL);
  
  info = g_new (RequestTextInfo, 1);
  info->callback = callback;
  info->user_data = user_data;

  btk_clipboard_request_contents (clipboard, bdk_atom_intern_static_string ("UTF8_STRING"),
				  request_text_received_func,
				  info);
}

static void
request_rich_text_received_func (BtkClipboard     *clipboard,
                                 BtkSelectionData *selection_data,
                                 bpointer          data)
{
  RequestRichTextInfo *info = data;
  buint8 *result = NULL;
  bsize length = 0;

  result = selection_data->data;
  length = selection_data->length;

  info->current_atom++;

  if ((!result || length < 1) && (info->current_atom < info->n_atoms))
    {
      btk_clipboard_request_contents (clipboard, info->atoms[info->current_atom],
                                      request_rich_text_received_func,
                                      info);
      return;
    }

  info->callback (clipboard, selection_data->target, result, length,
                  info->user_data);
  g_free (info->atoms);
  g_free (info);
}

/**
 * btk_clipboard_request_rich_text:
 * @clipboard: a #BtkClipboard
 * @buffer:    a #BtkTextBuffer
 * @callback:  a function to call when the text is received,
 *             or the retrieval fails. (It will always be called
 *             one way or the other.)
 * @user_data: user data to pass to @callback.
 *
 * Requests the contents of the clipboard as rich text. When the rich
 * text is later received, @callback will be called.
 *
 * The @text parameter to @callback will contain the resulting rich
 * text if the request succeeded, or %NULL if it failed. The @length
 * parameter will contain @text's length. This function can fail for
 * various reasons, in particular if the clipboard was empty or if the
 * contents of the clipboard could not be converted into rich text form.
 *
 * Since: 2.10
 **/
void
btk_clipboard_request_rich_text (BtkClipboard                    *clipboard,
                                 BtkTextBuffer                   *buffer,
                                 BtkClipboardRichTextReceivedFunc callback,
                                 bpointer                         user_data)
{
  RequestRichTextInfo *info;

  g_return_if_fail (clipboard != NULL);
  g_return_if_fail (BTK_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (callback != NULL);

  info = g_new (RequestRichTextInfo, 1);
  info->callback = callback;
  info->atoms = NULL;
  info->n_atoms = 0;
  info->current_atom = 0;
  info->user_data = user_data;

  info->atoms = btk_text_buffer_get_deserialize_formats (buffer, &info->n_atoms);

  btk_clipboard_request_contents (clipboard, info->atoms[info->current_atom],
				  request_rich_text_received_func,
				  info);
}

static void 
request_image_received_func (BtkClipboard     *clipboard,
			     BtkSelectionData *selection_data,
			     bpointer          data)
{
  RequestImageInfo *info = data;
  BdkPixbuf *result = NULL;

  result = btk_selection_data_get_pixbuf (selection_data);

  if (!result)
    {
      /* If we asked for image/png and didn't get it, try image/jpeg;
       * if we asked for image/jpeg and didn't get it, try image/gif;
       * if we asked for image/gif and didn't get it, try image/bmp;
       * If we asked for anything else and didn't get it, give up.
       */
      if (selection_data->target == bdk_atom_intern_static_string ("image/png"))
	{
	  btk_clipboard_request_contents (clipboard,
					  bdk_atom_intern_static_string ("image/jpeg"), 
					  request_image_received_func, info);
	  return;
	}
      else if (selection_data->target == bdk_atom_intern_static_string ("image/jpeg"))
	{
	  btk_clipboard_request_contents (clipboard,
					  bdk_atom_intern_static_string ("image/gif"), 
					  request_image_received_func, info);
	  return;
	}
      else if (selection_data->target == bdk_atom_intern_static_string ("image/gif"))
	{
	  btk_clipboard_request_contents (clipboard,
					  bdk_atom_intern_static_string ("image/bmp"), 
					  request_image_received_func, info);
	  return;
	}
    }

  info->callback (clipboard, result, info->user_data);
  g_free (info);

  if (result)
    g_object_unref (result);
}

/**
 * btk_clipboard_request_image:
 * @clipboard: a #BtkClipboard
 * @callback:  a function to call when the image is received,
 *             or the retrieval fails. (It will always be called
 *             one way or the other.)
 * @user_data: user data to pass to @callback.
 * 
 * Requests the contents of the clipboard as image. When the image is
 * later received, it will be converted to a #BdkPixbuf, and
 * @callback will be called. 
 *
 * The @pixbuf parameter to @callback will contain the resulting 
 * #BdkPixbuf if the request succeeded, or %NULL if it failed. This 
 * could happen for various reasons, in particular if the clipboard 
 * was empty or if the contents of the clipboard could not be 
 * converted into an image.
 *
 * Since: 2.6
 **/
void 
btk_clipboard_request_image (BtkClipboard                  *clipboard,
			     BtkClipboardImageReceivedFunc  callback,
			     bpointer                       user_data)
{
  RequestImageInfo *info;
  
  g_return_if_fail (clipboard != NULL);
  g_return_if_fail (callback != NULL);
  
  info = g_new (RequestImageInfo, 1);
  info->callback = callback;
  info->user_data = user_data;

  btk_clipboard_request_contents (clipboard, 
				  bdk_atom_intern_static_string ("image/png"),
				  request_image_received_func,
				  info);
}

static void 
request_uris_received_func (BtkClipboard     *clipboard,
			    BtkSelectionData *selection_data,
			    bpointer          data)
{
  RequestURIInfo *info = data;
  bchar **uris;

  uris = btk_selection_data_get_uris (selection_data);
  info->callback (clipboard, uris, info->user_data);
  g_strfreev (uris);

  g_slice_free (RequestURIInfo, info);
}

/**
 * btk_clipboard_request_uris:
 * @clipboard: a #BtkClipboard
 * @callback:  a function to call when the URIs are received,
 *             or the retrieval fails. (It will always be called
 *             one way or the other.)
 * @user_data: user data to pass to @callback.
 * 
 * Requests the contents of the clipboard as URIs. When the URIs are
 * later received @callback will be called.
 *
 * The @uris parameter to @callback will contain the resulting array of
 * URIs if the request succeeded, or %NULL if it failed. This could happen
 * for various reasons, in particular if the clipboard was empty or if the
 * contents of the clipboard could not be converted into URI form.
 *
 * Since: 2.14
 **/
void 
btk_clipboard_request_uris (BtkClipboard                *clipboard,
			    BtkClipboardURIReceivedFunc  callback,
			    bpointer                     user_data)
{
  RequestURIInfo *info;
  
  g_return_if_fail (clipboard != NULL);
  g_return_if_fail (callback != NULL);
  
  info = g_slice_new (RequestURIInfo);
  info->callback = callback;
  info->user_data = user_data;

  btk_clipboard_request_contents (clipboard, bdk_atom_intern_static_string ("text/uri-list"),
				  request_uris_received_func,
				  info);
}

static void 
request_targets_received_func (BtkClipboard     *clipboard,
			       BtkSelectionData *selection_data,
			       bpointer          data)
{
  RequestTargetsInfo *info = data;
  BdkAtom *targets = NULL;
  bint n_targets = 0;

  btk_selection_data_get_targets (selection_data, &targets, &n_targets);

  info->callback (clipboard, targets, n_targets, info->user_data);

  g_free (info);
  g_free (targets);
}

/**
 * btk_clipboard_request_targets:
 * @clipboard: a #BtkClipboard
 * @callback:  a function to call when the targets are received,
 *             or the retrieval fails. (It will always be called
 *             one way or the other.)
 * @user_data: user data to pass to @callback.
 * 
 * Requests the contents of the clipboard as list of supported targets. 
 * When the list is later received, @callback will be called. 
 *
 * The @targets parameter to @callback will contain the resulting targets if
 * the request succeeded, or %NULL if it failed.
 *
 * Since: 2.4
 **/
void 
btk_clipboard_request_targets (BtkClipboard                *clipboard,
			       BtkClipboardTargetsReceivedFunc callback,
			       bpointer                     user_data)
{
  RequestTargetsInfo *info;
  
  g_return_if_fail (clipboard != NULL);
  g_return_if_fail (callback != NULL);

  /* If the display supports change notification we cache targets */
  if (bdk_display_supports_selection_notification (btk_clipboard_get_display (clipboard)) &&
      clipboard->n_cached_targets != -1)
    {
      (* callback) (clipboard, clipboard->cached_targets, clipboard->n_cached_targets, user_data);
      return;
    }
  
  info = g_new (RequestTargetsInfo, 1);
  info->callback = callback;
  info->user_data = user_data;

  btk_clipboard_request_contents (clipboard, bdk_atom_intern_static_string ("TARGETS"),
				  request_targets_received_func,
				  info);
}

typedef struct
{
  GMainLoop *loop;
  bpointer data;
  BdkAtom format; /* used by rich text */
  bsize length; /* used by rich text */
} WaitResults;

static void 
clipboard_received_func (BtkClipboard     *clipboard,
			 BtkSelectionData *selection_data,
			 bpointer          data)
{
  WaitResults *results = data;

  if (selection_data->length >= 0)
    results->data = btk_selection_data_copy (selection_data);
  
  g_main_loop_quit (results->loop);
}

/**
 * btk_clipboard_wait_for_contents:
 * @clipboard: a #BtkClipboard
 * @target: an atom representing the form into which the clipboard
 *          owner should convert the selection.
 * 
 * Requests the contents of the clipboard using the given target.
 * This function waits for the data to be received using the main 
 * loop, so events, timeouts, etc, may be dispatched during the wait.
 * 
 * Return value: a newly-allocated #BtkSelectionData object or %NULL
 *               if retrieving the given target failed. If non-%NULL,
 *               this value must be freed with btk_selection_data_free() 
 *               when you are finished with it.
 **/
BtkSelectionData *
btk_clipboard_wait_for_contents (BtkClipboard *clipboard,
				 BdkAtom       target)
{
  WaitResults results;

  g_return_val_if_fail (clipboard != NULL, NULL);
  g_return_val_if_fail (target != BDK_NONE, NULL);
  
  results.data = NULL;
  results.loop = g_main_loop_new (NULL, TRUE);

  btk_clipboard_request_contents (clipboard, target, 
				  clipboard_received_func,
				  &results);

  if (g_main_loop_is_running (results.loop))
    {
      BDK_THREADS_LEAVE ();
      g_main_loop_run (results.loop);
      BDK_THREADS_ENTER ();
    }

  g_main_loop_unref (results.loop);

  return results.data;
}

static void 
clipboard_text_received_func (BtkClipboard *clipboard,
			      const bchar  *text,
			      bpointer      data)
{
  WaitResults *results = data;

  results->data = g_strdup (text);
  g_main_loop_quit (results->loop);
}

/**
 * btk_clipboard_wait_for_text:
 * @clipboard: a #BtkClipboard
 * 
 * Requests the contents of the clipboard as text and converts
 * the result to UTF-8 if necessary. This function waits for
 * the data to be received using the main loop, so events,
 * timeouts, etc, may be dispatched during the wait.
 * 
 * Return value: a newly-allocated UTF-8 string which must
 *               be freed with g_free(), or %NULL if retrieving
 *               the selection data failed. (This could happen
 *               for various reasons, in particular if the
 *               clipboard was empty or if the contents of the
 *               clipboard could not be converted into text form.)
 **/
bchar *
btk_clipboard_wait_for_text (BtkClipboard *clipboard)
{
  WaitResults results;

  g_return_val_if_fail (clipboard != NULL, NULL);
  
  results.data = NULL;
  results.loop = g_main_loop_new (NULL, TRUE);

  btk_clipboard_request_text (clipboard,
			      clipboard_text_received_func,
			      &results);

  if (g_main_loop_is_running (results.loop))
    {
      BDK_THREADS_LEAVE ();
      g_main_loop_run (results.loop);
      BDK_THREADS_ENTER ();
    }

  g_main_loop_unref (results.loop);

  return results.data;
}

static void
clipboard_rich_text_received_func (BtkClipboard *clipboard,
                                   BdkAtom       format,
                                   const buint8 *text,
                                   bsize         length,
                                   bpointer      data)
{
  WaitResults *results = data;

  results->data = g_memdup (text, length);
  results->format = format;
  results->length = length;
  g_main_loop_quit (results->loop);
}

/**
 * btk_clipboard_wait_for_rich_text:
 * @clipboard: a #BtkClipboard
 * @buffer: a #BtkTextBuffer
 * @format: return location for the format of the returned data
 * @length: return location for the length of the returned data
 *
 * Requests the contents of the clipboard as rich text.  This function
 * waits for the data to be received using the main loop, so events,
 * timeouts, etc, may be dispatched during the wait.
 *
 * Return value: (array length=length) (transfer full): a
 *               newly-allocated binary block of data which must
 *               be freed with g_free(), or %NULL if retrieving
 *               the selection data failed. (This could happen
 *               for various reasons, in particular if the
 *               clipboard was empty or if the contents of the
 *               clipboard could not be converted into text form.)
 *
 * Since: 2.10
 **/
buint8 *
btk_clipboard_wait_for_rich_text (BtkClipboard  *clipboard,
                                  BtkTextBuffer *buffer,
                                  BdkAtom       *format,
                                  bsize         *length)
{
  WaitResults results;

  g_return_val_if_fail (clipboard != NULL, NULL);
  g_return_val_if_fail (BTK_IS_TEXT_BUFFER (buffer), NULL);
  g_return_val_if_fail (format != NULL, NULL);
  g_return_val_if_fail (length != NULL, NULL);

  results.data = NULL;
  results.loop = g_main_loop_new (NULL, TRUE);

  btk_clipboard_request_rich_text (clipboard, buffer,
                                   clipboard_rich_text_received_func,
                                   &results);

  if (g_main_loop_is_running (results.loop))
    {
      BDK_THREADS_LEAVE ();
      g_main_loop_run (results.loop);
      BDK_THREADS_ENTER ();
    }

  g_main_loop_unref (results.loop);

  *format = results.format;
  *length = results.length;

  return results.data;
}

static void 
clipboard_image_received_func (BtkClipboard *clipboard,
			       BdkPixbuf    *pixbuf,
			       bpointer      data)
{
  WaitResults *results = data;

  if (pixbuf)
    results->data = g_object_ref (pixbuf);

  g_main_loop_quit (results->loop);
}

/**
 * btk_clipboard_wait_for_image:
 * @clipboard: a #BtkClipboard
 * 
 * Requests the contents of the clipboard as image and converts
 * the result to a #BdkPixbuf. This function waits for
 * the data to be received using the main loop, so events,
 * timeouts, etc, may be dispatched during the wait.
 * 
 * Return value: (transfer full): a newly-allocated #BdkPixbuf object which must
 *               be disposed with g_object_unref(), or %NULL if 
 *               retrieving the selection data failed. (This 
 *               could happen for various reasons, in particular 
 *               if the clipboard was empty or if the contents of 
 *               the clipboard could not be converted into an image.)
 *
 * Since: 2.6
 **/
BdkPixbuf *
btk_clipboard_wait_for_image (BtkClipboard *clipboard)
{
  WaitResults results;

  g_return_val_if_fail (clipboard != NULL, NULL);
  
  results.data = NULL;
  results.loop = g_main_loop_new (NULL, TRUE);

  btk_clipboard_request_image (clipboard,
			       clipboard_image_received_func,
			       &results);

  if (g_main_loop_is_running (results.loop))
    {
      BDK_THREADS_LEAVE ();
      g_main_loop_run (results.loop);
      BDK_THREADS_ENTER ();
    }

  g_main_loop_unref (results.loop);

  return results.data;
}

static void 
clipboard_uris_received_func (BtkClipboard *clipboard,
			      bchar       **uris,
			      bpointer      data)
{
  WaitResults *results = data;

  results->data = g_strdupv (uris);
  g_main_loop_quit (results->loop);
}

/**
 * btk_clipboard_wait_for_uris:
 * @clipboard: a #BtkClipboard
 * 
 * Requests the contents of the clipboard as URIs. This function waits
 * for the data to be received using the main loop, so events,
 * timeouts, etc, may be dispatched during the wait.
 *
 * Return value: (array zero-terminated=1) (element-type utf8) (transfer full): a newly-allocated
 * 		 %NULL-terminated array of strings which must
 *               be freed with g_strfreev(), or %NULL if
 *               retrieving the selection data failed. (This
 *               could happen for various reasons, in particular
 *               if the clipboard was empty or if the contents of
 *               the clipboard could not be converted into URI form.)
 *
 * Since: 2.14
 **/
bchar **
btk_clipboard_wait_for_uris (BtkClipboard *clipboard)
{
  WaitResults results;

  g_return_val_if_fail (clipboard != NULL, NULL);
  
  results.data = NULL;
  results.loop = g_main_loop_new (NULL, TRUE);

  btk_clipboard_request_uris (clipboard,
			      clipboard_uris_received_func,
			      &results);

  if (g_main_loop_is_running (results.loop))
    {
      BDK_THREADS_LEAVE ();
      g_main_loop_run (results.loop);
      BDK_THREADS_ENTER ();
    }

  g_main_loop_unref (results.loop);

  return results.data;
}

/**
 * btk_clipboard_get_display:
 * @clipboard: a #BtkClipboard
 *
 * Gets the #BdkDisplay associated with @clipboard
 *
 * Return value: (transfer none): the #BdkDisplay associated with @clipboard
 *
 * Since: 2.2
 **/
BdkDisplay *
btk_clipboard_get_display (BtkClipboard *clipboard)
{
  g_return_val_if_fail (clipboard != NULL, NULL);

  return clipboard->display;
}

/**
 * btk_clipboard_wait_is_text_available:
 * @clipboard: a #BtkClipboard
 * 
 * Test to see if there is text available to be pasted
 * This is done by requesting the TARGETS atom and checking
 * if it contains any of the supported text targets. This function 
 * waits for the data to be received using the main loop, so events, 
 * timeouts, etc, may be dispatched during the wait.
 *
 * This function is a little faster than calling
 * btk_clipboard_wait_for_text() since it doesn't need to retrieve
 * the actual text.
 * 
 * Return value: %TRUE is there is text available, %FALSE otherwise.
 **/
bboolean
btk_clipboard_wait_is_text_available (BtkClipboard *clipboard)
{
  BtkSelectionData *data;
  bboolean result = FALSE;

  data = btk_clipboard_wait_for_contents (clipboard, bdk_atom_intern_static_string ("TARGETS"));
  if (data)
    {
      result = btk_selection_data_targets_include_text (data);
      btk_selection_data_free (data);
    }

  return result;
}

/**
 * btk_clipboard_wait_is_rich_text_available:
 * @clipboard: a #BtkClipboard
 * @buffer: a #BtkTextBuffer
 *
 * Test to see if there is rich text available to be pasted
 * This is done by requesting the TARGETS atom and checking
 * if it contains any of the supported rich text targets. This function
 * waits for the data to be received using the main loop, so events,
 * timeouts, etc, may be dispatched during the wait.
 *
 * This function is a little faster than calling
 * btk_clipboard_wait_for_rich_text() since it doesn't need to retrieve
 * the actual text.
 *
 * Return value: %TRUE is there is rich text available, %FALSE otherwise.
 *
 * Since: 2.10
 **/
bboolean
btk_clipboard_wait_is_rich_text_available (BtkClipboard  *clipboard,
                                           BtkTextBuffer *buffer)
{
  BtkSelectionData *data;
  bboolean result = FALSE;

  g_return_val_if_fail (BTK_IS_CLIPBOARD (clipboard), FALSE);
  g_return_val_if_fail (BTK_IS_TEXT_BUFFER (buffer), FALSE);

  data = btk_clipboard_wait_for_contents (clipboard, bdk_atom_intern_static_string ("TARGETS"));
  if (data)
    {
      result = btk_selection_data_targets_include_rich_text (data, buffer);
      btk_selection_data_free (data);
    }

  return result;
}

/**
 * btk_clipboard_wait_is_image_available:
 * @clipboard: a #BtkClipboard
 * 
 * Test to see if there is an image available to be pasted
 * This is done by requesting the TARGETS atom and checking
 * if it contains any of the supported image targets. This function 
 * waits for the data to be received using the main loop, so events, 
 * timeouts, etc, may be dispatched during the wait.
 *
 * This function is a little faster than calling
 * btk_clipboard_wait_for_image() since it doesn't need to retrieve
 * the actual image data.
 * 
 * Return value: %TRUE is there is an image available, %FALSE otherwise.
 *
 * Since: 2.6
 **/
bboolean
btk_clipboard_wait_is_image_available (BtkClipboard *clipboard)
{
  BtkSelectionData *data;
  bboolean result = FALSE;

  data = btk_clipboard_wait_for_contents (clipboard, 
					  bdk_atom_intern_static_string ("TARGETS"));
  if (data)
    {
      result = btk_selection_data_targets_include_image (data, FALSE);
      btk_selection_data_free (data);
    }

  return result;
}

/**
 * btk_clipboard_wait_is_uris_available:
 * @clipboard: a #BtkClipboard
 * 
 * Test to see if there is a list of URIs available to be pasted
 * This is done by requesting the TARGETS atom and checking
 * if it contains the URI targets. This function
 * waits for the data to be received using the main loop, so events, 
 * timeouts, etc, may be dispatched during the wait.
 *
 * This function is a little faster than calling
 * btk_clipboard_wait_for_uris() since it doesn't need to retrieve
 * the actual URI data.
 * 
 * Return value: %TRUE is there is an URI list available, %FALSE otherwise.
 *
 * Since: 2.14
 **/
bboolean
btk_clipboard_wait_is_uris_available (BtkClipboard *clipboard)
{
  BtkSelectionData *data;
  bboolean result = FALSE;

  data = btk_clipboard_wait_for_contents (clipboard, 
					  bdk_atom_intern_static_string ("TARGETS"));
  if (data)
    {
      result = btk_selection_data_targets_include_uri (data);
      btk_selection_data_free (data);
    }

  return result;
}

/**
 * btk_clipboard_wait_for_targets
 * @clipboard: a #BtkClipboard
 * @targets: (out) (array length=n_targets) (transfer container):  
 *           location to store an array of targets. The result
 *           stored here must be freed with g_free().
 * @n_targets: location to store number of items in @targets.
 *
 * Returns a list of targets that are present on the clipboard, or %NULL
 * if there aren't any targets available. The returned list must be 
 * freed with g_free().
 * This function waits for the data to be received using the main 
 * loop, so events, timeouts, etc, may be dispatched during the wait.
 *
 * Return value: %TRUE if any targets are present on the clipboard,
 *               otherwise %FALSE.
 *
 * Since: 2.4
 */
bboolean
btk_clipboard_wait_for_targets (BtkClipboard  *clipboard, 
				BdkAtom      **targets,
				bint          *n_targets)
{
  BtkSelectionData *data;
  bboolean result = FALSE;
  
  g_return_val_if_fail (clipboard != NULL, FALSE);

  /* If the display supports change notification we cache targets */
  if (bdk_display_supports_selection_notification (btk_clipboard_get_display (clipboard)) &&
      clipboard->n_cached_targets != -1)
    {
      if (n_targets)
 	*n_targets = clipboard->n_cached_targets;
 
      if (targets)
 	*targets = g_memdup (clipboard->cached_targets,
 			     clipboard->n_cached_targets * sizeof (BdkAtom));

       return TRUE;
    }
  
  if (n_targets)
    *n_targets = 0;
      
  if (targets)
    *targets = NULL;      

  data = btk_clipboard_wait_for_contents (clipboard, bdk_atom_intern_static_string ("TARGETS"));

  if (data)
    {
      BdkAtom *tmp_targets;
      bint tmp_n_targets;
       
      result = btk_selection_data_get_targets (data, &tmp_targets, &tmp_n_targets);
 
      if (bdk_display_supports_selection_notification (btk_clipboard_get_display (clipboard)))
 	{
 	  clipboard->n_cached_targets = tmp_n_targets;
 	  clipboard->cached_targets = g_memdup (tmp_targets,
 						tmp_n_targets * sizeof (BdkAtom));
 	}
 
      if (n_targets)
 	*n_targets = tmp_n_targets;
 
      if (targets)
 	*targets = tmp_targets;
      else
 	g_free (tmp_targets);
      
      btk_selection_data_free (data);
    }

  return result;
}

static BtkClipboard *
clipboard_peek (BdkDisplay *display, 
		BdkAtom     selection,
		bboolean    only_if_exists)
{
  BtkClipboard *clipboard = NULL;
  GSList *clipboards;
  GSList *tmp_list;

  if (selection == BDK_NONE)
    selection = BDK_SELECTION_CLIPBOARD;

  clipboards = g_object_get_data (B_OBJECT (display), "btk-clipboard-list");

  tmp_list = clipboards;
  while (tmp_list)
    {
      clipboard = tmp_list->data;
      if (clipboard->selection == selection)
	break;

      tmp_list = tmp_list->next;
    }

  if (!tmp_list && !only_if_exists)
    {
      clipboard = g_object_new (BTK_TYPE_CLIPBOARD, NULL);
      clipboard->selection = selection;
      clipboard->display = display;
      clipboard->n_cached_targets = -1;
      clipboard->n_storable_targets = -1;
      clipboards = b_slist_prepend (clipboards, clipboard);
      g_object_set_data (B_OBJECT (display), I_("btk-clipboard-list"), clipboards);
      g_signal_connect (display, "closed",
			G_CALLBACK (clipboard_display_closed), clipboard);
      bdk_display_request_selection_notification (display, selection);
    }
  
  return clipboard;
}

static void
btk_clipboard_owner_change (BtkClipboard        *clipboard,
			    BdkEventOwnerChange *event)
{
  if (clipboard->n_cached_targets != -1)
    {
      g_free (clipboard->cached_targets);
      clipboard->cached_targets = NULL;
      clipboard->n_cached_targets = -1;
    }
}

/**
 * btk_clipboard_wait_is_target_available:
 * @clipboard: a #BtkClipboard
 * @target:    A #BdkAtom indicating which target to look for.
 *
 * Checks if a clipboard supports pasting data of a given type. This
 * function can be used to determine if a "Paste" menu item should be
 * insensitive or not.
 *
 * If you want to see if there's text available on the clipboard, use
 * btk_clipboard_wait_is_text_available () instead.
 *
 * Return value: %TRUE if the target is available, %FALSE otherwise.
 *
 * Since: 2.6
 */
bboolean
btk_clipboard_wait_is_target_available (BtkClipboard *clipboard,
					BdkAtom       target)
{
  BdkAtom *targets;
  bint i, n_targets;
  bboolean retval = FALSE;
    
  if (!btk_clipboard_wait_for_targets (clipboard, &targets, &n_targets))
    return FALSE;

  for (i = 0; i < n_targets; i++)
    {
      if (targets[i] == target)
	{
	  retval = TRUE;
	  break;
	}
    }

  g_free (targets);
  
  return retval;
}

/**
 * _btk_clipboard_handle_event:
 * @event: a owner change event
 * 
 * Emits the #BtkClipboard::owner-change signal on the appropriate @clipboard.
 *
 * Since: 2.6
 **/
void 
_btk_clipboard_handle_event (BdkEventOwnerChange *event)
{
  BdkDisplay *display;
  BtkClipboard *clipboard;
  
  display = bdk_window_get_display (event->window);
  clipboard = clipboard_peek (display, event->selection, TRUE);
      
  if (clipboard)
    g_signal_emit (clipboard, 
		   clipboard_signals[OWNER_CHANGE], 0, event, NULL);
}

static bboolean
btk_clipboard_store_timeout (BtkClipboard *clipboard)
{
  g_main_loop_quit (clipboard->store_loop);
  
  return FALSE;
}

/**
 * btk_clipboard_set_can_store:
 * @clipboard: a #BtkClipboard
 * @targets: (allow-none) (array length=n_targets): array containing
 *           information about which forms should be stored or %NULL
 *           to indicate that all forms should be stored.
 * @n_targets: number of elements in @targets
 *
 * Hints that the clipboard data should be stored somewhere when the
 * application exits or when btk_clipboard_store () is called.
 *
 * This value is reset when the clipboard owner changes.
 * Where the clipboard data is stored is platform dependent,
 * see bdk_display_store_clipboard () for more information.
 * 
 * Since: 2.6
 */
void
btk_clipboard_set_can_store (BtkClipboard         *clipboard,
 			     const BtkTargetEntry *targets,
 			     bint                  n_targets)
{
  BtkWidget *clipboard_widget;
  int i;
  static const BtkTargetEntry save_targets[] = {
    { "SAVE_TARGETS", 0, TARGET_SAVE_TARGETS }
  };
  
  g_return_if_fail (BTK_IS_CLIPBOARD (clipboard));
  g_return_if_fail (n_targets >= 0);

  if (clipboard->selection != BDK_SELECTION_CLIPBOARD)
    return;
  
  g_free (clipboard->storable_targets);
  
  clipboard_widget = get_clipboard_widget (clipboard->display);

  /* n_storable_targets being -1 means that
   * btk_clipboard_set_can_store hasn't been called since the
   * clipboard owner changed. We only want to add SAVE_TARGETS and 
   * ref the owner once , so we do that here
   */  
  if (clipboard->n_storable_targets == -1)
    {
      btk_selection_add_targets (clipboard_widget, clipboard->selection,
				 save_targets, 1);

      /* Ref the owner so it won't go away */
      if (clipboard->have_owner)
	g_object_ref (clipboard->user_data);
    }
  
  clipboard->n_storable_targets = n_targets;
  clipboard->storable_targets = g_new (BdkAtom, n_targets);
  for (i = 0; i < n_targets; i++)
    clipboard->storable_targets[i] = bdk_atom_intern (targets[i].target, FALSE);
}

static bboolean
btk_clipboard_selection_notify (BtkWidget         *widget,
				BdkEventSelection *event,
				BtkClipboard      *clipboard)
{
  if (event->selection == bdk_atom_intern_static_string ("CLIPBOARD_MANAGER") &&
      clipboard->storing_selection)
    g_main_loop_quit (clipboard->store_loop);

  return FALSE;
}

/**
 * btk_clipboard_store:
 * @clipboard: a #BtkClipboard
 *
 * Stores the current clipboard data somewhere so that it will stay
 * around after the application has quit.
 *
 * Since: 2.6
 */
void
btk_clipboard_store (BtkClipboard *clipboard)
{
  BtkWidget *clipboard_widget;

  g_return_if_fail (BTK_IS_CLIPBOARD (clipboard));

  if (clipboard->n_storable_targets < 0)
    return;
  
  if (!bdk_display_supports_clipboard_persistence (clipboard->display))
    return;

  g_object_ref (clipboard);

  clipboard_widget = get_clipboard_widget (clipboard->display);
  clipboard->notify_signal_id = g_signal_connect (clipboard_widget,
						  "selection-notify-event",
						  G_CALLBACK (btk_clipboard_selection_notify),
						  clipboard);

  bdk_display_store_clipboard (clipboard->display,
			       clipboard_widget->window,
			       clipboard_get_timestamp (clipboard),
			       clipboard->storable_targets,
			       clipboard->n_storable_targets);

  clipboard->storing_selection = TRUE;

  clipboard->store_loop = g_main_loop_new (NULL, TRUE);
  clipboard->store_timeout = g_timeout_add_seconds (10, (GSourceFunc) btk_clipboard_store_timeout, clipboard);

  if (g_main_loop_is_running (clipboard->store_loop))
    {
      BDK_THREADS_LEAVE ();
      g_main_loop_run (clipboard->store_loop);
      BDK_THREADS_ENTER ();
    }
  
  g_main_loop_unref (clipboard->store_loop);
  clipboard->store_loop = NULL;
  
  g_source_remove (clipboard->store_timeout);
  clipboard->store_timeout = 0;
  g_signal_handler_disconnect (clipboard_widget, clipboard->notify_signal_id);
  clipboard->notify_signal_id = 0;
  
  clipboard->storing_selection = FALSE;

  g_object_unref (clipboard);
}

/* Stores all clipboard selections on all displays, called from
 * btk_main_quit ().
 */
void
_btk_clipboard_store_all (void)
{
  BtkClipboard *clipboard;
  GSList *displays, *list;
  
  displays = bdk_display_manager_list_displays (bdk_display_manager_get ());

  list = displays;
  while (list)
    {
      BdkDisplay *display = list->data;

      clipboard = clipboard_peek (display, BDK_SELECTION_CLIPBOARD, TRUE);

      if (clipboard)
	btk_clipboard_store (clipboard);
      
      list = list->next;
    }
  b_slist_free (displays);
  
}

#define __BTK_CLIPBOARD_C__
#include "btkaliasdef.c"
