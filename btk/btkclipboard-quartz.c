/* BTK - The GIMP Toolkit
 * Copyright (C) 2000 Red Hat, Inc.
 * Copyright (C) 2004 Nokia Corporation
 * Copyright (C) 2006-2008 Imendio AB
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
 */

#include "config.h"
#include <string.h>

#import <Cocoa/Cocoa.h>

#include "btkclipboard.h"
#include "btkinvisible.h"
#include "btkmain.h"
#include "btkmarshalers.h"
#include "btkintl.h"
#include "btktextbuffer.h"
#include "btkquartz.h"
#include "btkalias.h"
#include <bdk/quartz/bdkquartz.h>

enum {
  OWNER_CHANGE,
  LAST_SIGNAL
};

@interface BtkClipboardOwner : NSObject {
  BtkClipboard *clipboard;
  @public
  bboolean setting_same_owner;
}

@end

typedef struct _BtkClipboardClass BtkClipboardClass;

struct _BtkClipboard
{
  BObject parent_instance;

  NSPasteboard *pasteboard;
  BtkClipboardOwner *owner;
  NSInteger change_count;

  BdkAtom selection;

  BtkClipboardGetFunc get_func;
  BtkClipboardClearFunc clear_func;
  bpointer user_data;
  bboolean have_owner;
  BtkTargetList *target_list;

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

static void btk_clipboard_class_init   (BtkClipboardClass   *class);
static void btk_clipboard_finalize     (BObject             *object);
static void btk_clipboard_owner_change (BtkClipboard        *clipboard,
					BdkEventOwnerChange *event);

static void          clipboard_unset      (BtkClipboard     *clipboard);
static BtkClipboard *clipboard_peek       (BdkDisplay       *display,
					   BdkAtom           selection,
					   bboolean          only_if_exists);

@implementation BtkClipboardOwner
-(void)pasteboard:(NSPasteboard *)sender provideDataForType:(NSString *)type
{
  BtkSelectionData selection_data;
  buint info;

  if (!clipboard->target_list)
    return;

  memset (&selection_data, 0, sizeof (BtkSelectionData));

  selection_data.selection = clipboard->selection;
  selection_data.target = bdk_quartz_pasteboard_type_to_atom_libbtk_only (type);
  selection_data.display = bdk_display_get_default ();
  selection_data.length = -1;

  if (btk_target_list_find (clipboard->target_list, selection_data.target, &info))
    {
      clipboard->get_func (clipboard, &selection_data,
                           info,
                           clipboard->user_data);

      if (selection_data.length >= 0)
        _btk_quartz_set_selection_data_for_pasteboard (clipboard->pasteboard,
                                                       &selection_data);

      g_free (selection_data.data);
    }
}

/*  pasteboardChangedOwner is not called immediately, and it's not called
 *  reliably. It is somehow documented in the apple api docs, but the docs
 *  suck and don't really give clear instructions. Therefore we track
 *  changeCount in several places below and clear the clipboard if it
 *  changed.
 */
- (void)pasteboardChangedOwner:(NSPasteboard *)sender
{
  if (! setting_same_owner)
    clipboard_unset (clipboard);
}

- (id)initWithClipboard:(BtkClipboard *)aClipboard
{
  self = [super init];

  if (self)
    {
      clipboard = aClipboard;
      setting_same_owner = FALSE;
    }

  return self;
}

@end


static const bchar clipboards_owned_key[] = "btk-clipboards-owned";
static GQuark clipboards_owned_key_id = 0;

static BObjectClass *parent_class;
static buint         clipboard_signals[LAST_SIGNAL] = { 0 };

GType
btk_clipboard_get_type (void)
{
  static GType clipboard_type = 0;

  if (!clipboard_type)
    {
      const GTypeInfo clipboard_info =
      {
	sizeof (BtkClipboardClass),
	NULL,           /* base_init */
	NULL,           /* base_finalize */
	(GClassInitFunc) btk_clipboard_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data */
	sizeof (BtkClipboard),
	0,              /* n_preallocs */
	(GInstanceInitFunc) NULL,
      };

      clipboard_type = g_type_register_static (B_TYPE_OBJECT, I_("BtkClipboard"),
					       &clipboard_info, 0);
    }

  return clipboard_type;
}

static void
btk_clipboard_class_init (BtkClipboardClass *class)
{
  BObjectClass *bobject_class = B_OBJECT_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  bobject_class->finalize = btk_clipboard_finalize;

  class->owner_change = btk_clipboard_owner_change;

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
  GSList *clipboards;

  clipboard = BTK_CLIPBOARD (object);

  clipboards = g_object_get_data (B_OBJECT (clipboard->display), "btk-clipboard-list");
  if (b_slist_index (clipboards, clipboard) >= 0)
    g_warning ("BtkClipboard prematurely finalized");

  clipboard_unset (clipboard);

  clipboards = g_object_get_data (B_OBJECT (clipboard->display), "btk-clipboard-list");
  clipboards = b_slist_remove (clipboards, clipboard);
  g_object_set_data (B_OBJECT (clipboard->display), I_("btk-clipboard-list"), clipboards);

  if (clipboard->store_loop && g_main_loop_is_running (clipboard->store_loop))
    g_main_loop_quit (clipboard->store_loop);

  if (clipboard->store_timeout != 0)
    g_source_remove (clipboard->store_timeout);

  g_free (clipboard->storable_targets);

  B_OBJECT_CLASS (parent_class)->finalize (object);
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

BtkClipboard *
btk_clipboard_get_for_display (BdkDisplay *display,
			       BdkAtom     selection)
{
  g_return_val_if_fail (BDK_IS_DISPLAY (display), NULL);
  g_return_val_if_fail (!display->closed, NULL);

  return clipboard_peek (display, selection, FALSE);
}

BtkClipboard *
btk_clipboard_get (BdkAtom selection)
{
  return btk_clipboard_get_for_display (bdk_display_get_default (), selection);
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

      if (clipboard->target_list)
        {
          btk_target_list_unref (clipboard->target_list);
          clipboard->target_list = NULL;
        }

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
  BtkClipboardOwner *owner;
  NSSet *types;
  NSAutoreleasePool *pool;

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
            {
              return TRUE;
            }
        }
    }

  pool = [[NSAutoreleasePool alloc] init];

  types = _btk_quartz_target_entries_to_pasteboard_types (targets, n_targets);

  /*  call declareTypes before setting the clipboard members because
   *  declareTypes might clear the clipboard
   */
  if (user_data && user_data == clipboard->user_data)
    {
      owner = [clipboard->owner retain];

      owner->setting_same_owner = TRUE;
      clipboard->change_count = [clipboard->pasteboard declareTypes: [types allObjects]
                                                              owner: owner];
      owner->setting_same_owner = FALSE;
    }
  else
    {
      owner = [[BtkClipboardOwner alloc] initWithClipboard:clipboard];

      clipboard->change_count = [clipboard->pasteboard declareTypes: [types allObjects]
                                                              owner: owner];
    }

  [owner release];
  [types release];
  [pool release];

  clipboard->owner = owner;
  clipboard->user_data = user_data;
  clipboard->have_owner = have_owner;
  if (have_owner)
    clipboard_add_owner_notify (clipboard);
  clipboard->get_func = get_func;
  clipboard->clear_func = clear_func;

  if (clipboard->target_list)
    btk_target_list_unref (clipboard->target_list);
  clipboard->target_list = btk_target_list_new (targets, n_targets);

  return TRUE;
}

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

BObject *
btk_clipboard_get_owner (BtkClipboard *clipboard)
{
  g_return_val_if_fail (clipboard != NULL, NULL);

  if (clipboard->change_count < [clipboard->pasteboard changeCount])
    {
      clipboard_unset (clipboard);
      clipboard->change_count = [clipboard->pasteboard changeCount];
    }

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

  clipboard->owner = NULL;
  clipboard->get_func = NULL;
  clipboard->clear_func = NULL;
  clipboard->user_data = NULL;

  if (old_clear_func)
    old_clear_func (clipboard, old_data);

  if (clipboard->target_list)
    {
      btk_target_list_unref (clipboard->target_list);
      clipboard->target_list = NULL;
    }

  /* If we've transferred the clipboard data to the manager,
   * unref the owner
   */
  if (old_have_owner &&
      old_n_storable_targets != -1)
    g_object_unref (old_data);
}

void
btk_clipboard_clear (BtkClipboard *clipboard)
{
  clipboard_unset (clipboard);

  [clipboard->pasteboard declareTypes:nil owner:nil];
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

void
btk_clipboard_set_text (BtkClipboard *clipboard,
			const bchar  *text,
			bint          len)
{
  BtkTargetEntry target = { "UTF8_STRING", 0, 0 };

  g_return_if_fail (clipboard != NULL);
  g_return_if_fail (text != NULL);

  if (len < 0)
    len = strlen (text);

  btk_clipboard_set_with_data (clipboard,
			       &target, 1,
			       text_get_func, text_clear_func,
			       g_strndup (text, len));
  btk_clipboard_set_can_store (clipboard, NULL, 0);
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

void
btk_clipboard_set_image (BtkClipboard *clipboard,
			 BdkPixbuf    *pixbuf)
{
  BtkTargetList *list;
  GList *l;
  BtkTargetEntry *targets;
  bint n_targets, i;

  g_return_if_fail (clipboard != NULL);
  g_return_if_fail (BDK_IS_PIXBUF (pixbuf));

  list = btk_target_list_new (NULL, 0);
  btk_target_list_add_image_targets (list, 0, TRUE);

  n_targets = g_list_length (list->list);
  targets = g_new0 (BtkTargetEntry, n_targets);
  for (l = list->list, i = 0; l; l = l->next, i++)
    {
      BtkTargetPair *pair = (BtkTargetPair *)l->data;
      targets[i].target = bdk_atom_name (pair->target);
    }

  btk_clipboard_set_with_data (clipboard,
			       targets, n_targets,
			       pixbuf_get_func, pixbuf_clear_func,
			       g_object_ref (pixbuf));
  btk_clipboard_set_can_store (clipboard, NULL, 0);

  for (i = 0; i < n_targets; i++)
    g_free (targets[i].target);
  g_free (targets);
  btk_target_list_unref (list);
}

void
btk_clipboard_request_contents (BtkClipboard            *clipboard,
				BdkAtom                  target,
				BtkClipboardReceivedFunc callback,
				bpointer                 user_data)
{
  BtkSelectionData *data;

  data = btk_clipboard_wait_for_contents (clipboard, target);

  callback (clipboard, data, user_data);

  btk_selection_data_free (data);
}

void
btk_clipboard_request_text (BtkClipboard                *clipboard,
			    BtkClipboardTextReceivedFunc callback,
			    bpointer                     user_data)
{
  bchar *data = btk_clipboard_wait_for_text (clipboard);

  callback (clipboard, data, user_data);

  g_free (data);
}

void
btk_clipboard_request_rich_text (BtkClipboard                    *clipboard,
                                 BtkTextBuffer                   *buffer,
                                 BtkClipboardRichTextReceivedFunc callback,
                                 bpointer                         user_data)
{
  /* FIXME: Implement */
}


buint8 *
btk_clipboard_wait_for_rich_text (BtkClipboard  *clipboard,
                                  BtkTextBuffer *buffer,
                                  BdkAtom       *format,
                                  bsize         *length)
{
  /* FIXME: Implement */
  return NULL;
}

void
btk_clipboard_request_image (BtkClipboard                  *clipboard,
			     BtkClipboardImageReceivedFunc  callback,
			     bpointer                       user_data)
{
  BdkPixbuf *pixbuf = btk_clipboard_wait_for_image (clipboard);

  callback (clipboard, pixbuf, user_data);

  if (pixbuf)
    g_object_unref (pixbuf);
}

void
btk_clipboard_request_uris (BtkClipboard                *clipboard,
			    BtkClipboardURIReceivedFunc  callback,
			    bpointer                     user_data)
{
  bchar **uris = btk_clipboard_wait_for_uris (clipboard);

  callback (clipboard, uris, user_data);

  g_strfreev (uris);
}

void
btk_clipboard_request_targets (BtkClipboard                *clipboard,
			       BtkClipboardTargetsReceivedFunc callback,
			       bpointer                     user_data)
{
  BdkAtom *targets;
  bint n_targets;

  btk_clipboard_wait_for_targets (clipboard, &targets, &n_targets);

  callback (clipboard, targets, n_targets, user_data);
}


BtkSelectionData *
btk_clipboard_wait_for_contents (BtkClipboard *clipboard,
				 BdkAtom       target)
{
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  BtkSelectionData *selection_data = NULL;

  if (clipboard->change_count < [clipboard->pasteboard changeCount])
    {
      clipboard_unset (clipboard);
      clipboard->change_count = [clipboard->pasteboard changeCount];
    }

  if (target == bdk_atom_intern_static_string ("TARGETS"))
    {
      NSArray *types = [clipboard->pasteboard types];
      int i, length;
      GList *atom_list, *l;
      BdkAtom *atoms;

      length = [types count] * sizeof (BdkAtom);

      selection_data = g_slice_new0 (BtkSelectionData);
      selection_data->selection = clipboard->selection;
      selection_data->target = target;
      if (!selection_data->display)
	selection_data->display = bdk_display_get_default ();

      atoms = g_malloc (length);

      atom_list = _btk_quartz_pasteboard_types_to_atom_list (types);
      for (l = atom_list, i = 0; l ; l = l->next, i++)
	atoms[i] = BDK_POINTER_TO_ATOM (l->data);
      g_list_free (atom_list);

      btk_selection_data_set (selection_data,
                              BDK_SELECTION_TYPE_ATOM, 32,
                              (buchar *)atoms, length);

      [pool release];

      return selection_data;
    }

  selection_data = _btk_quartz_get_selection_data_from_pasteboard (clipboard->pasteboard,
								   target,
								   clipboard->selection);

  [pool release];

  return selection_data;
}

bchar *
btk_clipboard_wait_for_text (BtkClipboard *clipboard)
{
  BtkSelectionData *data;
  bchar *result;

  data = btk_clipboard_wait_for_contents (clipboard,
					  bdk_atom_intern_static_string ("UTF8_STRING"));

  result = (bchar *)btk_selection_data_get_text (data);

  btk_selection_data_free (data);

  return result;
}

BdkPixbuf *
btk_clipboard_wait_for_image (BtkClipboard *clipboard)
{
  BdkAtom target = bdk_atom_intern_static_string("image/tiff");
  int i;
  BtkSelectionData *data;

  data = btk_clipboard_wait_for_contents (clipboard, target);

  if (data && data->data)
    {
      BdkPixbuf *pixbuf = btk_selection_data_get_pixbuf (data);
      btk_selection_data_free (data);
      return pixbuf;
    }

  return NULL;
}

bchar **
btk_clipboard_wait_for_uris (BtkClipboard *clipboard)
{
  BtkSelectionData *data;

  data = btk_clipboard_wait_for_contents (clipboard, bdk_atom_intern_static_string ("text/uri-list"));
  if (data)
    {
      bchar **uris;

      uris = btk_selection_data_get_uris (data);
      btk_selection_data_free (data);

      return uris;
    }

  return NULL;
}

BdkDisplay *
btk_clipboard_get_display (BtkClipboard *clipboard)
{
  g_return_val_if_fail (clipboard != NULL, NULL);

  return clipboard->display;
}

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
      NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
      NSString *pasteboard_name;
      clipboard = g_object_new (BTK_TYPE_CLIPBOARD, NULL);

      if (selection == BDK_SELECTION_CLIPBOARD)
	pasteboard_name = NSGeneralPboard;
      else
	{
	  char *atom_string = bdk_atom_name (selection);

	  pasteboard_name = [NSString stringWithFormat:@"_BTK_%@",
			     [NSString stringWithUTF8String:atom_string]];
	  g_free (atom_string);
	}

      clipboard->pasteboard = [NSPasteboard pasteboardWithName:pasteboard_name];

      [pool release];

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
      clipboard->n_cached_targets = -1;
      g_free (clipboard->cached_targets);
    }
}

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

void
_btk_clipboard_handle_event (BdkEventOwnerChange *event)
{
}

void
btk_clipboard_set_can_store (BtkClipboard         *clipboard,
 			     const BtkTargetEntry *targets,
 			     bint                  n_targets)
{
  /* FIXME: Implement */
}

void
btk_clipboard_store (BtkClipboard *clipboard)
{
  int i;
  int n_targets = 0;
  BtkTargetEntry *targets;

  g_return_if_fail (BTK_IS_CLIPBOARD (clipboard));

  if (!clipboard->target_list || !clipboard->get_func)
    return;

  /* We simply store all targets into the OS X clipboard. We should be
   * using the functions bdk_display_supports_clipboard_persistence() and
   * bdk_display_store_clipboard(), but since for OS X the clipboard support
   * was implemented in BTK+ and not through BdkSelections, we do it this
   * way. Doing this properly could be worthwhile to implement in the future.
   */

  targets = btk_target_table_new_from_list (clipboard->target_list,
                                            &n_targets);
  for (i = 0; i < n_targets; i++)
    {
      BtkSelectionData selection_data;

      /* in each loop iteration, check if the content is still
       * there, because calling get_func() can do anything to
       * the clipboard
       */
      if (!clipboard->target_list || !clipboard->get_func)
        break;

      memset (&selection_data, 0, sizeof (BtkSelectionData));

      selection_data.selection = clipboard->selection;
      selection_data.target = bdk_atom_intern_static_string (targets[i].target);
      selection_data.display = bdk_display_get_default ();
      selection_data.length = -1;

      clipboard->get_func (clipboard, &selection_data,
                           targets[i].info, clipboard->user_data);

      if (selection_data.length >= 0)
        _btk_quartz_set_selection_data_for_pasteboard (clipboard->pasteboard,
                                                       &selection_data);

      g_free (selection_data.data);
    }

  if (targets)
    btk_target_table_free (targets, n_targets);
}

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
