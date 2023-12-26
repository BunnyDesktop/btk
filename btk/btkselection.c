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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* This file implements most of the work of the ICCCM selection protocol.
 * The code was written after an intensive study of the equivalent part
 * of John Ousterhout's Tk toolkit, and does many things in much the 
 * same way.
 *
 * The one thing in the ICCCM that isn't fully supported here (or in Tk)
 * is side effects targets. For these to be handled properly, MULTIPLE
 * targets need to be done in the order specified. This cannot be
 * guaranteed with the way we do things, since if we are doing INCR
 * transfers, the order will depend on the timing of the requestor.
 *
 * By Owen Taylor <owt1@cornell.edu>	      8/16/97
 */

/* Terminology note: when not otherwise specified, the term "incr" below
 * refers to the _sending_ part of the INCR protocol. The receiving
 * portion is referred to just as "retrieval". (Terminology borrowed
 * from Tk, because there is no good opposite to "retrieval" in English.
 * "send" can't be made into a noun gracefully and we're already using
 * "emission" for something else ....)
 */

/* The MOTIF entry widget seems to ask for the TARGETS target, then
   (regardless of the reply) ask for the TEXT target. It's slightly
   possible though that it somehow thinks we are responding negatively
   to the TARGETS request, though I don't really think so ... */

/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#include "config.h"
#include <stdarg.h>
#include <string.h>
#include "bdk.h"

#include "btkmain.h"
#include "btkselection.h"
#include "btktextbufferrichtext.h"
#include "btkintl.h"
#include "bdk-pixbuf/bdk-pixbuf.h"

#ifdef BDK_WINDOWING_X11
#include "x11/bdkx.h"
#endif

#ifdef BDK_WINDOWING_WIN32
#include "win32/bdkwin32.h"
#endif

#include "btkalias.h"

#undef DEBUG_SELECTION

/* Maximum size of a sent chunk, in bytes. Also the default size of
   our buffers */
#ifdef BDK_WINDOWING_X11
#define BTK_SELECTION_MAX_SIZE(display)                                 \
  MIN(262144,                                                           \
      XExtendedMaxRequestSize (BDK_DISPLAY_XDISPLAY (display)) == 0     \
       ? XMaxRequestSize (BDK_DISPLAY_XDISPLAY (display)) - 100         \
       : XExtendedMaxRequestSize (BDK_DISPLAY_XDISPLAY (display)) - 100)
#else
/* No chunks on Win32 */
#define BTK_SELECTION_MAX_SIZE(display) G_MAXINT
#endif

#define IDLE_ABORT_TIME 30

enum {
  INCR,
  MULTIPLE,
  TARGETS,
  TIMESTAMP,
  SAVE_TARGETS,
  LAST_ATOM
};

typedef struct _BtkSelectionInfo BtkSelectionInfo;
typedef struct _BtkIncrConversion BtkIncrConversion;
typedef struct _BtkIncrInfo BtkIncrInfo;
typedef struct _BtkRetrievalInfo BtkRetrievalInfo;

struct _BtkSelectionInfo
{
  BdkAtom	 selection;
  BtkWidget	*widget;	/* widget that owns selection */
  guint32	 time;		/* time used to acquire selection */
  BdkDisplay	*display;	/* needed in btk_selection_remove_all */    
};

struct _BtkIncrConversion 
{
  BdkAtom	    target;	/* Requested target */
  BdkAtom	    property;	/* Property to store in */
  BtkSelectionData  data;	/* The data being supplied */
  gint		    offset;	/* Current offset in sent selection.
				 *  -1 => All done
				 *  -2 => Only the final (empty) portion
				 *	  left to send */
};

struct _BtkIncrInfo
{
  BdkWindow *requestor;		/* Requestor window - we create a BdkWindow
				   so we can receive events */
  BdkAtom    selection;		/* Selection we're sending */
  
  BtkIncrConversion *conversions; /* Information about requested conversions -
				   * With MULTIPLE requests (benighted 1980's
				   * hardware idea), there can be more than
				   * one */
  gint num_conversions;
  gint num_incrs;		/* number of remaining INCR style transactions */
  guint32 idle_time;
};


struct _BtkRetrievalInfo
{
  BtkWidget *widget;
  BdkAtom selection;		/* Selection being retrieved. */
  BdkAtom target;		/* Form of selection that we requested */
  guint32 idle_time;		/* Number of seconds since we last heard
				   from selection owner */
  guchar   *buffer;		/* Buffer in which to accumulate results */
  gint	   offset;		/* Current offset in buffer, -1 indicates
				   not yet started */
  guint32 notify_time;		/* Timestamp from SelectionNotify */
};

/* Local Functions */
static void btk_selection_init              (void);
static gboolean btk_selection_incr_timeout      (BtkIncrInfo      *info);
static gboolean btk_selection_retrieval_timeout (BtkRetrievalInfo *info);
static void btk_selection_retrieval_report  (BtkRetrievalInfo *info,
					     BdkAtom           type,
					     gint              format,
					     guchar           *buffer,
					     gint              length,
					     guint32           time);
static void btk_selection_invoke_handler    (BtkWidget        *widget,
					     BtkSelectionData *data,
					     guint             time);
static void btk_selection_default_handler   (BtkWidget        *widget,
					     BtkSelectionData *data);
static int  btk_selection_bytes_per_item    (gint              format);

/* Local Data */
static gint initialize = TRUE;
static GList *current_retrievals = NULL;
static GList *current_incrs = NULL;
static GList *current_selections = NULL;

static BdkAtom btk_selection_atoms[LAST_ATOM];
static const char btk_selection_handler_key[] = "btk-selection-handlers";

/****************
 * Target Lists *
 ****************/

/*
 * Target lists
 */


/**
 * btk_target_list_new:
 * @targets: (array length=ntargets): Pointer to an array of #BtkTargetEntry
 * @ntargets: number of entries in @targets.
 * 
 * Creates a new #BtkTargetList from an array of #BtkTargetEntry.
 * 
 * Return value: (transfer full): the new #BtkTargetList.
 **/
BtkTargetList *
btk_target_list_new (const BtkTargetEntry *targets,
		     guint                 ntargets)
{
  BtkTargetList *result = g_slice_new (BtkTargetList);
  result->list = NULL;
  result->ref_count = 1;

  if (targets)
    btk_target_list_add_table (result, targets, ntargets);
  
  return result;
}

/**
 * btk_target_list_ref:
 * @list:  a #BtkTargetList
 * 
 * Increases the reference count of a #BtkTargetList by one.
 *
 * Return value: the passed in #BtkTargetList.
 **/
BtkTargetList *
btk_target_list_ref (BtkTargetList *list)
{
  g_return_val_if_fail (list != NULL, NULL);

  list->ref_count++;

  return list;
}

/**
 * btk_target_list_unref:
 * @list: a #BtkTargetList
 * 
 * Decreases the reference count of a #BtkTargetList by one.
 * If the resulting reference count is zero, frees the list.
 **/
void               
btk_target_list_unref (BtkTargetList *list)
{
  g_return_if_fail (list != NULL);
  g_return_if_fail (list->ref_count > 0);

  list->ref_count--;
  if (list->ref_count == 0)
    {
      GList *tmp_list = list->list;
      while (tmp_list)
	{
	  BtkTargetPair *pair = tmp_list->data;
	  g_slice_free (BtkTargetPair, pair);

	  tmp_list = tmp_list->next;
	}
      
      g_list_free (list->list);
      g_slice_free (BtkTargetList, list);
    }
}

/**
 * btk_target_list_add:
 * @list:  a #BtkTargetList
 * @target: the interned atom representing the target
 * @flags: the flags for this target
 * @info: an ID that will be passed back to the application
 * 
 * Appends another target to a #BtkTargetList.
 **/
void 
btk_target_list_add (BtkTargetList *list,
		     BdkAtom        target,
		     guint          flags,
		     guint          info)
{
  BtkTargetPair *pair;

  g_return_if_fail (list != NULL);
  
  pair = g_slice_new (BtkTargetPair);
  pair->target = target;
  pair->flags = flags;
  pair->info = info;

  list->list = g_list_append (list->list, pair);
}

static BdkAtom utf8_atom;
static BdkAtom text_atom;
static BdkAtom ctext_atom;
static BdkAtom text_plain_atom;
static BdkAtom text_plain_utf8_atom;
static BdkAtom text_plain_locale_atom;
static BdkAtom text_uri_list_atom;

static void 
init_atoms (void)
{
  gchar *tmp;
  const gchar *charset;

  if (!utf8_atom)
    {
      utf8_atom = bdk_atom_intern_static_string ("UTF8_STRING");
      text_atom = bdk_atom_intern_static_string ("TEXT");
      ctext_atom = bdk_atom_intern_static_string ("COMPOUND_TEXT");
      text_plain_atom = bdk_atom_intern_static_string ("text/plain");
      text_plain_utf8_atom = bdk_atom_intern_static_string ("text/plain;charset=utf-8");
      g_get_charset (&charset);
      tmp = g_strdup_printf ("text/plain;charset=%s", charset);
      text_plain_locale_atom = bdk_atom_intern (tmp, FALSE);
      g_free (tmp);

      text_uri_list_atom = bdk_atom_intern_static_string ("text/uri-list");
    }
}

/**
 * btk_target_list_add_text_targets:
 * @list: a #BtkTargetList
 * @info: an ID that will be passed back to the application
 * 
 * Appends the text targets supported by #BtkSelection to
 * the target list. All targets are added with the same @info.
 * 
 * Since: 2.6
 **/
void 
btk_target_list_add_text_targets (BtkTargetList *list,
				  guint          info)
{
  g_return_if_fail (list != NULL);
  
  init_atoms ();

  /* Keep in sync with btk_selection_data_targets_include_text()
   */
  btk_target_list_add (list, utf8_atom, 0, info);  
  btk_target_list_add (list, ctext_atom, 0, info);  
  btk_target_list_add (list, text_atom, 0, info);  
  btk_target_list_add (list, BDK_TARGET_STRING, 0, info);  
  btk_target_list_add (list, text_plain_utf8_atom, 0, info);  
  if (!g_get_charset (NULL))
    btk_target_list_add (list, text_plain_locale_atom, 0, info);  
  btk_target_list_add (list, text_plain_atom, 0, info);  
}

/**
 * btk_target_list_add_rich_text_targets:
 * @list: a #BtkTargetList
 * @info: an ID that will be passed back to the application
 * @deserializable: if %TRUE, then deserializable rich text formats
 *                  will be added, serializable formats otherwise.
 * @buffer: a #BtkTextBuffer.
 *
 * Appends the rich text targets registered with
 * btk_text_buffer_register_serialize_format() or
 * btk_text_buffer_register_deserialize_format() to the target list. All
 * targets are added with the same @info.
 *
 * Since: 2.10
 **/
void
btk_target_list_add_rich_text_targets (BtkTargetList  *list,
                                       guint           info,
                                       gboolean        deserializable,
                                       BtkTextBuffer  *buffer)
{
  BdkAtom *atoms;
  gint     n_atoms;
  gint     i;

  g_return_if_fail (list != NULL);
  g_return_if_fail (BTK_IS_TEXT_BUFFER (buffer));

  if (deserializable)
    atoms = btk_text_buffer_get_deserialize_formats (buffer, &n_atoms);
  else
    atoms = btk_text_buffer_get_serialize_formats (buffer, &n_atoms);

  for (i = 0; i < n_atoms; i++)
    btk_target_list_add (list, atoms[i], 0, info);

  g_free (atoms);
}

/**
 * btk_target_list_add_image_targets:
 * @list: a #BtkTargetList
 * @info: an ID that will be passed back to the application
 * @writable: whether to add only targets for which BTK+ knows
 *   how to convert a pixbuf into the format
 * 
 * Appends the image targets supported by #BtkSelection to
 * the target list. All targets are added with the same @info.
 * 
 * Since: 2.6
 **/
void 
btk_target_list_add_image_targets (BtkTargetList *list,
				   guint          info,
				   gboolean       writable)
{
  GSList *formats, *f;
  gchar **mimes, **m;
  BdkAtom atom;

  g_return_if_fail (list != NULL);

  formats = bdk_pixbuf_get_formats ();

  /* Make sure png comes first */
  for (f = formats; f; f = f->next)
    {
      BdkPixbufFormat *fmt = f->data;
      gchar *name; 
 
      name = bdk_pixbuf_format_get_name (fmt);
      if (strcmp (name, "png") == 0)
	{
	  formats = g_slist_delete_link (formats, f);
	  formats = g_slist_prepend (formats, fmt);

	  g_free (name);

	  break;
	}

      g_free (name);
    }  

  for (f = formats; f; f = f->next)
    {
      BdkPixbufFormat *fmt = f->data;

      if (writable && !bdk_pixbuf_format_is_writable (fmt))
	continue;
      
      mimes = bdk_pixbuf_format_get_mime_types (fmt);
      for (m = mimes; *m; m++)
	{
	  atom = bdk_atom_intern (*m, FALSE);
	  btk_target_list_add (list, atom, 0, info);  
	}
      g_strfreev (mimes);
    }

  g_slist_free (formats);
}

/**
 * btk_target_list_add_uri_targets:
 * @list: a #BtkTargetList
 * @info: an ID that will be passed back to the application
 * 
 * Appends the URI targets supported by #BtkSelection to
 * the target list. All targets are added with the same @info.
 * 
 * Since: 2.6
 **/
void 
btk_target_list_add_uri_targets (BtkTargetList *list,
				 guint          info)
{
  g_return_if_fail (list != NULL);
  
  init_atoms ();

  btk_target_list_add (list, text_uri_list_atom, 0, info);  
}

/**
 * btk_target_list_add_table:
 * @list: a #BtkTargetList
 * @targets: (array length=ntargets): the table of #BtkTargetEntry
 * @ntargets: number of targets in the table
 * 
 * Prepends a table of #BtkTargetEntry to a target list.
 **/
void               
btk_target_list_add_table (BtkTargetList        *list,
			   const BtkTargetEntry *targets,
			   guint                 ntargets)
{
  gint i;

  for (i=ntargets-1; i >= 0; i--)
    {
      BtkTargetPair *pair = g_slice_new (BtkTargetPair);
      pair->target = bdk_atom_intern (targets[i].target, FALSE);
      pair->flags = targets[i].flags;
      pair->info = targets[i].info;
      
      list->list = g_list_prepend (list->list, pair);
    }
}

/**
 * btk_target_list_remove:
 * @list: a #BtkTargetList
 * @target: the interned atom representing the target
 * 
 * Removes a target from a target list.
 **/
void 
btk_target_list_remove (BtkTargetList *list,
			BdkAtom            target)
{
  GList *tmp_list;

  g_return_if_fail (list != NULL);

  tmp_list = list->list;
  while (tmp_list)
    {
      BtkTargetPair *pair = tmp_list->data;
      
      if (pair->target == target)
	{
	  g_slice_free (BtkTargetPair, pair);

	  list->list = g_list_remove_link (list->list, tmp_list);
	  g_list_free_1 (tmp_list);

	  return;
	}
      
      tmp_list = tmp_list->next;
    }
}

/**
 * btk_target_list_find:
 * @list: a #BtkTargetList
 * @target: an interned atom representing the target to search for
 * @info: a pointer to the location to store application info for target,
 *        or %NULL
 *
 * Looks up a given target in a #BtkTargetList.
 *
 * Return value: %TRUE if the target was found, otherwise %FALSE
 **/
gboolean
btk_target_list_find (BtkTargetList *list,
		      BdkAtom        target,
		      guint         *info)
{
  GList *tmp_list;

  g_return_val_if_fail (list != NULL, FALSE);

  tmp_list = list->list;
  while (tmp_list)
    {
      BtkTargetPair *pair = tmp_list->data;

      if (pair->target == target)
	{
          if (info)
            *info = pair->info;

	  return TRUE;
	}

      tmp_list = tmp_list->next;
    }

  return FALSE;
}

/**
 * btk_target_table_new_from_list:
 * @list: a #BtkTargetList
 * @n_targets: return location for the number ot targets in the table
 *
 * This function creates an #BtkTargetEntry array that contains the
 * same targets as the passed %list. The returned table is newly
 * allocated and should be freed using btk_target_table_free() when no
 * longer needed.
 *
 * Return value: (array length=n_targets) (transfer full): the new table.
 *
 * Since: 2.10
 **/
BtkTargetEntry *
btk_target_table_new_from_list (BtkTargetList *list,
                                gint          *n_targets)
{
  BtkTargetEntry *targets;
  GList          *tmp_list;
  gint            i;

  g_return_val_if_fail (list != NULL, NULL);
  g_return_val_if_fail (n_targets != NULL, NULL);

  *n_targets = g_list_length (list->list);
  targets = g_new0 (BtkTargetEntry, *n_targets);

  for (i = 0, tmp_list = list->list;
       i < *n_targets;
       i++, tmp_list = g_list_next (tmp_list))
    {
      BtkTargetPair *pair = tmp_list->data;

      targets[i].target = bdk_atom_name (pair->target);
      targets[i].flags  = pair->flags;
      targets[i].info   = pair->info;
    }

  return targets;
}

/**
 * btk_target_table_free:
 * @targets: (array length=n_targets): a #BtkTargetEntry array
 * @n_targets: the number of entries in the array
 *
 * This function frees a target table as returned by
 * btk_target_table_new_from_list()
 *
 * Since: 2.10
 **/
void
btk_target_table_free (BtkTargetEntry *targets,
                       gint            n_targets)
{
  gint i;

  g_return_if_fail (targets == NULL || n_targets > 0);

  for (i = 0; i < n_targets; i++)
    g_free (targets[i].target);

  g_free (targets);
}

/**
 * btk_selection_owner_set_for_display:
 * @display: the #Bdkdisplay where the selection is set
 * @widget: (allow-none): new selection owner (a #BdkWidget), or %NULL.
 * @selection: an interned atom representing the selection to claim.
 * @time_: timestamp with which to claim the selection
 *
 * Claim ownership of a given selection for a particular widget, or,
 * if @widget is %NULL, release ownership of the selection.
 *
 * Return value: TRUE if the operation succeeded 
 * 
 * Since: 2.2
 */
gboolean
btk_selection_owner_set_for_display (BdkDisplay   *display,
				     BtkWidget    *widget,
				     BdkAtom       selection,
				     guint32       time)
{
  GList *tmp_list;
  BtkWidget *old_owner;
  BtkSelectionInfo *selection_info = NULL;
  BdkWindow *window;

  g_return_val_if_fail (BDK_IS_DISPLAY (display), FALSE);
  g_return_val_if_fail (selection != BDK_NONE, FALSE);
  g_return_val_if_fail (widget == NULL || btk_widget_get_realized (widget), FALSE);
  g_return_val_if_fail (widget == NULL || btk_widget_get_display (widget) == display, FALSE);
  
  if (widget == NULL)
    window = NULL;
  else
    window = widget->window;

  tmp_list = current_selections;
  while (tmp_list)
    {
      if (((BtkSelectionInfo *)tmp_list->data)->selection == selection)
	{
	  selection_info = tmp_list->data;
	  break;
	}
      
      tmp_list = tmp_list->next;
    }
  
  if (bdk_selection_owner_set_for_display (display, window, selection, time, TRUE))
    {
      old_owner = NULL;
      
      if (widget == NULL)
	{
	  if (selection_info)
	    {
	      old_owner = selection_info->widget;
	      current_selections = g_list_remove_link (current_selections,
						       tmp_list);
	      g_list_free (tmp_list);
	      g_slice_free (BtkSelectionInfo, selection_info);
	    }
	}
      else
	{
	  if (selection_info == NULL)
	    {
	      selection_info = g_slice_new (BtkSelectionInfo);
	      selection_info->selection = selection;
	      selection_info->widget = widget;
	      selection_info->time = time;
	      selection_info->display = display;
	      current_selections = g_list_prepend (current_selections,
						   selection_info);
	    }
	  else
	    {
	      old_owner = selection_info->widget;
	      selection_info->widget = widget;
	      selection_info->time = time;
	      selection_info->display = display;
	    }
	}
      /* If another widget in the application lost the selection,
       *  send it a BDK_SELECTION_CLEAR event.
       */
      if (old_owner && old_owner != widget)
	{
	  BdkEvent *event = bdk_event_new (BDK_SELECTION_CLEAR);
	  
	  event->selection.window = g_object_ref (old_owner->window);
	  event->selection.selection = selection;
	  event->selection.time = time;
	  
	  btk_widget_event (old_owner, event);

	  bdk_event_free (event);
	}
      return TRUE;
    }
  else
    return FALSE;
}

/**
 * btk_selection_owner_set:
 * @widget: (allow-none):  a #BtkWidget, or %NULL.
 * @selection:  an interned atom representing the selection to claim
 * @time_: timestamp with which to claim the selection
 * 
 * Claims ownership of a given selection for a particular widget,
 * or, if @widget is %NULL, release ownership of the selection.
 * 
 * Return value: %TRUE if the operation succeeded
 **/
gboolean
btk_selection_owner_set (BtkWidget *widget,
			 BdkAtom    selection,
			 guint32    time)
{
  BdkDisplay *display;
  
  g_return_val_if_fail (widget == NULL || btk_widget_get_realized (widget), FALSE);
  g_return_val_if_fail (selection != BDK_NONE, FALSE);

  if (widget)
    display = btk_widget_get_display (widget);
  else
    {
      BTK_NOTE (MULTIHEAD,
		g_warning ("btk_selection_owner_set (NULL,...) is not multihead safe"));
		 
      display = bdk_display_get_default ();
    }
  
  return btk_selection_owner_set_for_display (display, widget,
					      selection, time);
}

typedef struct _BtkSelectionTargetList BtkSelectionTargetList;

struct _BtkSelectionTargetList {
  BdkAtom selection;
  BtkTargetList *list;
};

static BtkTargetList *
btk_selection_target_list_get (BtkWidget    *widget,
			       BdkAtom       selection)
{
  BtkSelectionTargetList *sellist;
  GList *tmp_list;
  GList *lists;

  lists = g_object_get_data (G_OBJECT (widget), btk_selection_handler_key);
  
  tmp_list = lists;
  while (tmp_list)
    {
      sellist = tmp_list->data;
      if (sellist->selection == selection)
	return sellist->list;
      tmp_list = tmp_list->next;
    }

  sellist = g_slice_new (BtkSelectionTargetList);
  sellist->selection = selection;
  sellist->list = btk_target_list_new (NULL, 0);

  lists = g_list_prepend (lists, sellist);
  g_object_set_data (G_OBJECT (widget), I_(btk_selection_handler_key), lists);

  return sellist->list;
}

static void
btk_selection_target_list_remove (BtkWidget    *widget)
{
  BtkSelectionTargetList *sellist;
  GList *tmp_list;
  GList *lists;

  lists = g_object_get_data (G_OBJECT (widget), btk_selection_handler_key);
  
  tmp_list = lists;
  while (tmp_list)
    {
      sellist = tmp_list->data;

      btk_target_list_unref (sellist->list);

      g_slice_free (BtkSelectionTargetList, sellist);
      tmp_list = tmp_list->next;
    }

  g_list_free (lists);
  g_object_set_data (G_OBJECT (widget), I_(btk_selection_handler_key), NULL);
}

/**
 * btk_selection_clear_targets:
 * @widget:    a #BtkWidget
 * @selection: an atom representing a selection
 *
 * Remove all targets registered for the given selection for the
 * widget.
 **/
void 
btk_selection_clear_targets (BtkWidget *widget,
			     BdkAtom    selection)
{
  BtkSelectionTargetList *sellist;
  GList *tmp_list;
  GList *lists;

  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (selection != BDK_NONE);

  lists = g_object_get_data (G_OBJECT (widget), btk_selection_handler_key);
  
  tmp_list = lists;
  while (tmp_list)
    {
      sellist = tmp_list->data;
      if (sellist->selection == selection)
	{
	  lists = g_list_delete_link (lists, tmp_list);
	  btk_target_list_unref (sellist->list);
	  g_slice_free (BtkSelectionTargetList, sellist);

	  break;
	}
      
      tmp_list = tmp_list->next;
    }
  
  g_object_set_data (G_OBJECT (widget), I_(btk_selection_handler_key), lists);
}

/**
 * btk_selection_add_target:
 * @widget:  a #BtkTarget
 * @selection: the selection
 * @target: target to add.
 * @info: A unsigned integer which will be passed back to the application.
 * 
 * Appends a specified target to the list of supported targets for a 
 * given widget and selection.
 **/
void 
btk_selection_add_target (BtkWidget	    *widget, 
			  BdkAtom	     selection,
			  BdkAtom	     target,
			  guint              info)
{
  BtkTargetList *list;

  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (selection != BDK_NONE);

  list = btk_selection_target_list_get (widget, selection);
  btk_target_list_add (list, target, 0, info);
#ifdef BDK_WINDOWING_WIN32
  bdk_win32_selection_add_targets (widget->window, selection, 1, &target);
#endif
}

/**
 * btk_selection_add_targets:
 * @widget: a #BtkWidget
 * @selection: the selection
 * @targets: (array length=ntargets): a table of targets to add
 * @ntargets:  number of entries in @targets
 * 
 * Prepends a table of targets to the list of supported targets
 * for a given widget and selection.
 **/
void 
btk_selection_add_targets (BtkWidget            *widget, 
			   BdkAtom               selection,
			   const BtkTargetEntry *targets,
			   guint                 ntargets)
{
  BtkTargetList *list;

  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (selection != BDK_NONE);
  g_return_if_fail (targets != NULL);
  
  list = btk_selection_target_list_get (widget, selection);
  btk_target_list_add_table (list, targets, ntargets);

#ifdef BDK_WINDOWING_WIN32
  {
    int i;
    BdkAtom *atoms = g_new (BdkAtom, ntargets);

    for (i = 0; i < ntargets; ++i)
      atoms[i] = bdk_atom_intern (targets[i].target, FALSE);
    bdk_win32_selection_add_targets (widget->window, selection, ntargets, atoms);
    g_free (atoms);
  }
#endif
}


/**
 * btk_selection_remove_all:
 * @widget: a #BtkWidget 
 * 
 * Removes all handlers and unsets ownership of all 
 * selections for a widget. Called when widget is being
 * destroyed. This function will not generally be
 * called by applications.
 **/
void
btk_selection_remove_all (BtkWidget *widget)
{
  GList *tmp_list;
  GList *next;
  BtkSelectionInfo *selection_info;

  g_return_if_fail (BTK_IS_WIDGET (widget));

  /* Remove pending requests/incrs for this widget */
  
  tmp_list = current_retrievals;
  while (tmp_list)
    {
      next = tmp_list->next;
      if (((BtkRetrievalInfo *)tmp_list->data)->widget == widget)
	{
	  current_retrievals = g_list_remove_link (current_retrievals, 
						   tmp_list);
	  /* structure will be freed in timeout */
	  g_list_free (tmp_list);
	}
      tmp_list = next;
    }
  
  /* Disclaim ownership of any selections */
  
  tmp_list = current_selections;
  while (tmp_list)
    {
      next = tmp_list->next;
      selection_info = (BtkSelectionInfo *)tmp_list->data;
      
      if (selection_info->widget == widget)
	{	
	  bdk_selection_owner_set_for_display (selection_info->display,
					       NULL, 
					       selection_info->selection,
				               BDK_CURRENT_TIME, FALSE);
	  current_selections = g_list_remove_link (current_selections,
						   tmp_list);
	  g_list_free (tmp_list);
	  g_slice_free (BtkSelectionInfo, selection_info);
	}
      
      tmp_list = next;
    }

  /* Remove all selection lists */
  btk_selection_target_list_remove (widget);
}


/**
 * btk_selection_convert:
 * @widget: The widget which acts as requestor
 * @selection: Which selection to get
 * @target: Form of information desired (e.g., STRING)
 * @time_: Time of request (usually of triggering event)
       In emergency, you could use #BDK_CURRENT_TIME
 * 
 * Requests the contents of a selection. When received, 
 * a "selection-received" signal will be generated.
 * 
 * Return value: %TRUE if requested succeeded. %FALSE if we could not process
 *          request. (e.g., there was already a request in process for
 *          this widget).
 **/
gboolean
btk_selection_convert (BtkWidget *widget, 
		       BdkAtom	  selection, 
		       BdkAtom	  target,
		       guint32	  time_)
{
  BtkRetrievalInfo *info;
  GList *tmp_list;
  BdkWindow *owner_window;
  BdkDisplay *display;
  
  g_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);
  g_return_val_if_fail (selection != BDK_NONE, FALSE);
  
  if (initialize)
    btk_selection_init ();
  
  if (!btk_widget_get_realized (widget))
    btk_widget_realize (widget);
  
  /* Check to see if there are already any retrievals in progress for
     this widget. If we changed BDK to use the selection for the 
     window property in which to store the retrieved information, then
     we could support multiple retrievals for different selections.
     This might be useful for DND. */
  
  tmp_list = current_retrievals;
  while (tmp_list)
    {
      info = (BtkRetrievalInfo *)tmp_list->data;
      if (info->widget == widget)
	return FALSE;
      tmp_list = tmp_list->next;
    }
  
  info = g_slice_new (BtkRetrievalInfo);
  
  info->widget = widget;
  info->selection = selection;
  info->target = target;
  info->idle_time = 0;
  info->buffer = NULL;
  info->offset = -1;
  
  /* Check if this process has current owner. If so, call handler
     procedure directly to avoid deadlocks with INCR. */

  display = btk_widget_get_display (widget);
  owner_window = bdk_selection_owner_get_for_display (display, selection);
  
  if (owner_window != NULL)
    {
      BtkWidget *owner_widget;
      gpointer owner_widget_ptr;
      BtkSelectionData selection_data;
      
      selection_data.selection = selection;
      selection_data.target = target;
      selection_data.data = NULL;
      selection_data.length = -1;
      selection_data.display = display;
      
      bdk_window_get_user_data (owner_window, &owner_widget_ptr);
      owner_widget = owner_widget_ptr;
      
      if (owner_widget != NULL)
	{
	  btk_selection_invoke_handler (owner_widget, 
					&selection_data,
					time_);
	  
	  btk_selection_retrieval_report (info,
					  selection_data.type, 
					  selection_data.format,
					  selection_data.data,
					  selection_data.length,
					  time_);
	  
	  g_free (selection_data.data);
          selection_data.data = NULL;
          selection_data.length = -1;
	  
	  g_slice_free (BtkRetrievalInfo, info);
	  return TRUE;
	}
    }
  
  /* Otherwise, we need to go through X */
  
  current_retrievals = g_list_append (current_retrievals, info);
  bdk_selection_convert (widget->window, selection, target, time_);
  bdk_threads_add_timeout (1000,
      (GSourceFunc) btk_selection_retrieval_timeout, info);
  
  return TRUE;
}

/**
 * btk_selection_data_get_selection:
 * @selection_data: a pointer to a #BtkSelectionData structure.
 *
 * Retrieves the selection #BdkAtom of the selection data.
 *
 * Returns: (transfer none): the selection #BdkAtom of the selection data.
 *
 * Since: 2.16
 **/
BdkAtom
btk_selection_data_get_selection (BtkSelectionData *selection_data)
{
  g_return_val_if_fail (selection_data != NULL, 0);

  return selection_data->selection;
}

/**
 * btk_selection_data_get_target:
 * @selection_data: a pointer to a #BtkSelectionData structure.
 *
 * Retrieves the target of the selection.
 *
 * Returns: (transfer none): the target of the selection.
 *
 * Since: 2.14
 **/
BdkAtom
btk_selection_data_get_target (BtkSelectionData *selection_data)
{
  g_return_val_if_fail (selection_data != NULL, 0);

  return selection_data->target;
}

/**
 * btk_selection_data_get_data_type:
 * @selection_data: a pointer to a #BtkSelectionData structure.
 *
 * Retrieves the data type of the selection.
 *
 * Returns: (transfer none): the data type of the selection.
 *
 * Since: 2.14
 **/
BdkAtom
btk_selection_data_get_data_type (BtkSelectionData *selection_data)
{
  g_return_val_if_fail (selection_data != NULL, 0);

  return selection_data->type;
}

/**
 * btk_selection_data_get_format:
 * @selection_data: a pointer to a #BtkSelectionData structure.
 *
 * Retrieves the format of the selection.
 *
 * Returns: the format of the selection.
 *
 * Since: 2.14
 **/
gint
btk_selection_data_get_format (BtkSelectionData *selection_data)
{
  g_return_val_if_fail (selection_data != NULL, 0);

  return selection_data->format;
}

/**
 * btk_selection_data_get_data:
 * @selection_data: a pointer to a #BtkSelectionData structure.
 *
 * Retrieves the raw data of the selection.
 *
 * Returns: the raw data of the selection.
 *
 * Since: 2.14
 **/
const guchar*
btk_selection_data_get_data (BtkSelectionData *selection_data)
{
  g_return_val_if_fail (selection_data != NULL, NULL);

  return selection_data->data;
}

/**
 * btk_selection_data_get_length:
 * @selection_data: a pointer to a #BtkSelectionData structure.
 *
 * Retrieves the length of the raw data of the selection.
 *
 * Returns: the length of the data of the selection.
 *
 * Since: 2.14
 */
gint
btk_selection_data_get_length (BtkSelectionData *selection_data)
{
  g_return_val_if_fail (selection_data != NULL, -1);

  return selection_data->length;
}

/**
 * btk_selection_data_get_display:
 * @selection_data: a pointer to a #BtkSelectionData structure.
 *
 * Retrieves the display of the selection.
 *
 * Returns: (transfer none): the display of the selection.
 *
 * Since: 2.14
 **/
BdkDisplay *
btk_selection_data_get_display (BtkSelectionData *selection_data)
{
  g_return_val_if_fail (selection_data != NULL, NULL);

  return selection_data->display;
}

/**
 * btk_selection_data_set:
 * @selection_data: a pointer to a #BtkSelectionData structure.
 * @type: the type of selection data
 * @format: format (number of bits in a unit)
 * @data: (array length=length): pointer to the data (will be copied)
 * @length: length of the data
 * 
 * Stores new data into a #BtkSelectionData object. Should
 * <emphasis>only</emphasis> be called from a selection handler callback.
 * Zero-terminates the stored data.
 **/
void 
btk_selection_data_set (BtkSelectionData *selection_data,
			BdkAtom		  type,
			gint		  format,
			const guchar	 *data,
			gint		  length)
{
  g_return_if_fail (selection_data != NULL);

  g_free (selection_data->data);
  
  selection_data->type = type;
  selection_data->format = format;
  
  if (data)
    {
      selection_data->data = g_new (guchar, length+1);
      memcpy (selection_data->data, data, length);
      selection_data->data[length] = 0;
    }
  else
    {
      g_return_if_fail (length <= 0);
      
      if (length < 0)
	selection_data->data = NULL;
      else
	selection_data->data = (guchar *) g_strdup ("");
    }
  
  selection_data->length = length;
}

static gboolean
selection_set_string (BtkSelectionData *selection_data,
		      const gchar      *str,
		      gint              len)
{
  gchar *tmp = g_strndup (str, len);
  gchar *latin1 = bdk_utf8_to_string_target (tmp);
  g_free (tmp);
  
  if (latin1)
    {
      btk_selection_data_set (selection_data,
			      BDK_SELECTION_TYPE_STRING,
			      8, (guchar *) latin1, strlen (latin1));
      g_free (latin1);
      
      return TRUE;
    }
  else
    return FALSE;
}

static gboolean
selection_set_compound_text (BtkSelectionData *selection_data,
			     const gchar      *str,
			     gint              len)
{
  gchar *tmp;
  guchar *text;
  BdkAtom encoding;
  gint format;
  gint new_length;
  gboolean result = FALSE;

#ifdef BDK_WINDOWING_X11
  tmp = g_strndup (str, len);
  if (bdk_x11_display_utf8_to_compound_text (selection_data->display, tmp,
                                             &encoding, &format, &text, &new_length))
    {
      btk_selection_data_set (selection_data, encoding, format, text, new_length);
      bdk_x11_free_compound_text (text);

      result = TRUE;
    }
  g_free (tmp);
#elif defined(BDK_WINDOWING_WIN32) || defined(BDK_WINDOWING_QUARTZ)
  result = FALSE; /* not needed on Win32 or Quartz */
#else
  g_warning ("%s is not implemented", B_STRFUNC);
  result = FALSE;
#endif

  return result;
}

/* Normalize \r and \n into \r\n
 */
static gchar *
normalize_to_crlf (const gchar *str, 
		   gint         len)
{
  GString *result = g_string_sized_new (len);
  const gchar *p = str;
  const gchar *end = str + len;

  while (p < end)
    {
      if (*p == '\n')
	g_string_append_c (result, '\r');

      if (*p == '\r')
	{
	  g_string_append_c (result, *p);
	  p++;
	  if (p == end || *p != '\n')
	    g_string_append_c (result, '\n');
	  if (p == end)
	    break;
	}

      g_string_append_c (result, *p);
      p++;
    }

  return g_string_free (result, FALSE);  
}

/* Normalize \r and \r\n into \n
 */
static gchar *
normalize_to_lf (gchar *str, 
		 gint   len)
{
  GString *result = g_string_sized_new (len);
  const gchar *p = str;

  while (1)
    {
      if (*p == '\r')
	{
	  p++;
	  if (*p != '\n')
	    g_string_append_c (result, '\n');
	}

      if (*p == '\0')
	break;

      g_string_append_c (result, *p);
      p++;
    }

  return g_string_free (result, FALSE);  
}

static gboolean
selection_set_text_plain (BtkSelectionData *selection_data,
			  const gchar      *str,
			  gint              len)
{
  const gchar *charset = NULL;
  gchar *result;
  GError *error = NULL;

  result = normalize_to_crlf (str, len);
  if (selection_data->target == text_plain_atom)
    charset = "ASCII";
  else if (selection_data->target == text_plain_locale_atom)
    g_get_charset (&charset);

  if (charset)
    {
      gchar *tmp = result;
      result = g_convert_with_fallback (tmp, -1, 
					charset, "UTF-8", 
					NULL, NULL, NULL, &error);
      g_free (tmp);
    }

  if (!result)
    {
      g_warning ("Error converting from %s to %s: %s",
		 "UTF-8", charset, error->message);
      g_error_free (error);
      
      return FALSE;
    }
  
  btk_selection_data_set (selection_data,
			  selection_data->target, 
			  8, (guchar *) result, strlen (result));
  g_free (result);
  
  return TRUE;
}

static guchar *
selection_get_text_plain (BtkSelectionData *selection_data)
{
  const gchar *charset = NULL;
  gchar *str, *result;
  gsize len;
  GError *error = NULL;

  str = g_strdup ((const gchar *) selection_data->data);
  len = selection_data->length;
  
  if (selection_data->type == text_plain_atom)
    charset = "ISO-8859-1";
  else if (selection_data->type == text_plain_locale_atom)
    g_get_charset (&charset);

  if (charset)
    {
      gchar *tmp = str;
      str = g_convert_with_fallback (tmp, len, 
				     "UTF-8", charset,
				     NULL, NULL, &len, &error);
      g_free (tmp);

      if (!str)
	{
	  g_warning ("Error converting from %s to %s: %s",
		     charset, "UTF-8", error->message);
	  g_error_free (error);

	  return NULL;
	}
    }
  else if (!g_utf8_validate (str, -1, NULL))
    {
      g_warning ("Error converting from %s to %s: %s",
		 "text/plain;charset=utf-8", "UTF-8", "invalid UTF-8");
      g_free (str);

      return NULL;
    }

  result = normalize_to_lf (str, len);
  g_free (str);

  return (guchar *) result;
}

/**
 * btk_selection_data_set_text:
 * @selection_data: a #BtkSelectionData
 * @str: a UTF-8 string
 * @len: the length of @str, or -1 if @str is nul-terminated.
 * 
 * Sets the contents of the selection from a UTF-8 encoded string.
 * The string is converted to the form determined by
 * @selection_data->target.
 * 
 * Return value: %TRUE if the selection was successfully set,
 *   otherwise %FALSE.
 **/
gboolean
btk_selection_data_set_text (BtkSelectionData     *selection_data,
			     const gchar          *str,
			     gint                  len)
{
  g_return_val_if_fail (selection_data != NULL, FALSE);

  if (len < 0)
    len = strlen (str);
  
  init_atoms ();

  if (selection_data->target == utf8_atom)
    {
      btk_selection_data_set (selection_data,
			      utf8_atom,
			      8, (guchar *)str, len);
      return TRUE;
    }
  else if (selection_data->target == BDK_TARGET_STRING)
    {
      return selection_set_string (selection_data, str, len);
    }
  else if (selection_data->target == ctext_atom ||
	   selection_data->target == text_atom)
    {
      if (selection_set_compound_text (selection_data, str, len))
	return TRUE;
      else if (selection_data->target == text_atom)
	return selection_set_string (selection_data, str, len);
    }
  else if (selection_data->target == text_plain_atom ||
	   selection_data->target == text_plain_utf8_atom ||
	   selection_data->target == text_plain_locale_atom)
    {
      return selection_set_text_plain (selection_data, str, len);
    }

  return FALSE;
}

/**
 * btk_selection_data_get_text:
 * @selection_data: a #BtkSelectionData
 * 
 * Gets the contents of the selection data as a UTF-8 string.
 * 
 * Return value: if the selection data contained a recognized
 *   text type and it could be converted to UTF-8, a newly allocated
 *   string containing the converted text, otherwise %NULL.
 *   If the result is non-%NULL it must be freed with g_free().
 **/
guchar *
btk_selection_data_get_text (BtkSelectionData *selection_data)
{
  guchar *result = NULL;

  g_return_val_if_fail (selection_data != NULL, NULL);

  init_atoms ();
  
  if (selection_data->length >= 0 &&
      (selection_data->type == BDK_TARGET_STRING ||
       selection_data->type == ctext_atom ||
       selection_data->type == utf8_atom))
    {
      gchar **list;
      gint i;
      gint count = bdk_text_property_to_utf8_list_for_display (selection_data->display,
      							       selection_data->type,
						   	       selection_data->format, 
						               selection_data->data,
						               selection_data->length,
						               &list);
      if (count > 0)
	result = (guchar *) list[0];

      for (i = 1; i < count; i++)
	g_free (list[i]);
      g_free (list);
    }
  else if (selection_data->length >= 0 &&
	   (selection_data->type == text_plain_atom ||
	    selection_data->type == text_plain_utf8_atom ||
	    selection_data->type == text_plain_locale_atom))
    {
      result = selection_get_text_plain (selection_data);
    }

  return result;
}

/**
 * btk_selection_data_set_pixbuf:
 * @selection_data: a #BtkSelectionData
 * @pixbuf: a #BdkPixbuf
 * 
 * Sets the contents of the selection from a #BdkPixbuf
 * The pixbuf is converted to the form determined by
 * @selection_data->target.
 * 
 * Return value: %TRUE if the selection was successfully set,
 *   otherwise %FALSE.
 *
 * Since: 2.6
 **/
gboolean
btk_selection_data_set_pixbuf (BtkSelectionData *selection_data,
			       BdkPixbuf        *pixbuf)
{
  GSList *formats, *f;
  gchar **mimes, **m;
  BdkAtom atom;
  gboolean result;
  gchar *str, *type;
  gsize len;

  g_return_val_if_fail (selection_data != NULL, FALSE);
  g_return_val_if_fail (BDK_IS_PIXBUF (pixbuf), FALSE);

  formats = bdk_pixbuf_get_formats ();

  for (f = formats; f; f = f->next)
    {
      BdkPixbufFormat *fmt = f->data;

      mimes = bdk_pixbuf_format_get_mime_types (fmt);
      for (m = mimes; *m; m++)
	{
	  atom = bdk_atom_intern (*m, FALSE);
	  if (selection_data->target == atom)
	    {
	      str = NULL;
	      type = bdk_pixbuf_format_get_name (fmt);
	      result = bdk_pixbuf_save_to_buffer (pixbuf, &str, &len,
						  type, NULL,
                                                  ((strcmp (type, "png") == 0) ?
                                                   "compression" : NULL), "2",
                                                  NULL);
	      if (result)
		btk_selection_data_set (selection_data,
					atom, 8, (guchar *)str, len);
	      g_free (type);
	      g_free (str);
	      g_strfreev (mimes);
	      g_slist_free (formats);

	      return result;
	    }
	}

      g_strfreev (mimes);
    }

  g_slist_free (formats);
 
  return FALSE;
}

/**
 * btk_selection_data_get_pixbuf:
 * @selection_data: a #BtkSelectionData
 * 
 * Gets the contents of the selection data as a #BdkPixbuf.
 * 
 * Return value: (transfer full): if the selection data contained a recognized
 *   image type and it could be converted to a #BdkPixbuf, a 
 *   newly allocated pixbuf is returned, otherwise %NULL.
 *   If the result is non-%NULL it must be freed with g_object_unref().
 *
 * Since: 2.6
 **/
BdkPixbuf *
btk_selection_data_get_pixbuf (BtkSelectionData *selection_data)
{
  BdkPixbufLoader *loader;
  BdkPixbuf *result = NULL;

  g_return_val_if_fail (selection_data != NULL, NULL);

  if (selection_data->length > 0)
    {
      loader = bdk_pixbuf_loader_new ();
      
      bdk_pixbuf_loader_write (loader, 
			       selection_data->data,
			       selection_data->length,
			       NULL);
      bdk_pixbuf_loader_close (loader, NULL);
      result = bdk_pixbuf_loader_get_pixbuf (loader);
      
      if (result)
	g_object_ref (result);
      
      g_object_unref (loader);
    }

  return result;
}

/**
 * btk_selection_data_set_uris:
 * @selection_data: a #BtkSelectionData
 * @uris: (array zero-terminated=1): a %NULL-terminated array of
 *     strings holding URIs
 * 
 * Sets the contents of the selection from a list of URIs.
 * The string is converted to the form determined by
 * @selection_data->target.
 * 
 * Return value: %TRUE if the selection was successfully set,
 *   otherwise %FALSE.
 *
 * Since: 2.6
 **/
gboolean
btk_selection_data_set_uris (BtkSelectionData  *selection_data,
			     gchar            **uris)
{
  g_return_val_if_fail (selection_data != NULL, FALSE);
  g_return_val_if_fail (uris != NULL, FALSE);

  init_atoms ();

  if (selection_data->target == text_uri_list_atom)
    {
      GString *list;
      gint i;
      gchar *result;
      gsize length;
      
      list = g_string_new (NULL);
      for (i = 0; uris[i]; i++)
	{
	  g_string_append (list, uris[i]);
	  g_string_append (list, "\r\n");
	}

      result = g_convert (list->str, list->len,
			  "ASCII", "UTF-8", 
			  NULL, &length, NULL);
      g_string_free (list, TRUE);
      
      if (result)
	{
	  btk_selection_data_set (selection_data,
				  text_uri_list_atom,
				  8, (guchar *)result, length);
	  
	  g_free (result);

	  return TRUE;
	}
    }

  return FALSE;
}

/**
 * btk_selection_data_get_uris:
 * @selection_data: a #BtkSelectionData
 * 
 * Gets the contents of the selection data as array of URIs.
 *
 * Return value:  (array zero-terminated=1) (element-type utf8) (transfer full): if
 *   the selection data contains a list of
 *   URIs, a newly allocated %NULL-terminated string array
 *   containing the URIs, otherwise %NULL. If the result is
 *   non-%NULL it must be freed with g_strfreev().
 *
 * Since: 2.6
 **/
gchar **
btk_selection_data_get_uris (BtkSelectionData *selection_data)
{
  gchar **result = NULL;

  g_return_val_if_fail (selection_data != NULL, NULL);

  init_atoms ();
  
  if (selection_data->length >= 0 &&
      selection_data->type == text_uri_list_atom)
    {
      gchar **list;
      gint count = bdk_text_property_to_utf8_list_for_display (selection_data->display,
      							       utf8_atom,
						   	       selection_data->format, 
						               selection_data->data,
						               selection_data->length,
						               &list);
      if (count > 0)
	result = g_uri_list_extract_uris (list[0]);
      
      g_strfreev (list);
    }

  return result;
}


/**
 * btk_selection_data_get_targets:
 * @selection_data: a #BtkSelectionData object
 * @targets: (out) (array length=n_atoms) (transfer container):
 *           location to store an array of targets. The result
 *           stored here must be freed with g_free().
 * @n_atoms: location to store number of items in @targets.
 * 
 * Gets the contents of @selection_data as an array of targets.
 * This can be used to interpret the results of getting
 * the standard TARGETS target that is always supplied for
 * any selection.
 * 
 * Return value: %TRUE if @selection_data contains a valid
 *    array of targets, otherwise %FALSE.
 **/
gboolean
btk_selection_data_get_targets (BtkSelectionData  *selection_data,
				BdkAtom          **targets,
				gint              *n_atoms)
{
  g_return_val_if_fail (selection_data != NULL, FALSE);

  if (selection_data->length >= 0 &&
      selection_data->format == 32 &&
      selection_data->type == BDK_SELECTION_TYPE_ATOM)
    {
      if (targets)
	*targets = g_memdup (selection_data->data, selection_data->length);
      if (n_atoms)
	*n_atoms = selection_data->length / sizeof (BdkAtom);

      return TRUE;
    }
  else
    {
      if (targets)
	*targets = NULL;
      if (n_atoms)
	*n_atoms = -1;

      return FALSE;
    }
}

/**
 * btk_targets_include_text:
 * @targets: (array length=n_targets): an array of #BdkAtom<!-- -->s
 * @n_targets: the length of @targets
 * 
 * Determines if any of the targets in @targets can be used to
 * provide text.
 * 
 * Return value: %TRUE if @targets include a suitable target for text,
 *   otherwise %FALSE.
 *
 * Since: 2.10
 **/
gboolean 
btk_targets_include_text (BdkAtom *targets,
                          gint     n_targets)
{
  gint i;
  gboolean result = FALSE;

  g_return_val_if_fail (targets != NULL || n_targets == 0, FALSE);

  /* Keep in sync with btk_target_list_add_text_targets()
   */
 
  init_atoms ();
 
  for (i = 0; i < n_targets; i++)
    {
      if (targets[i] == utf8_atom ||
	  targets[i] == text_atom ||
	  targets[i] == BDK_TARGET_STRING ||
	  targets[i] == ctext_atom ||
	  targets[i] == text_plain_atom ||
	  targets[i] == text_plain_utf8_atom ||
	  targets[i] == text_plain_locale_atom)
	{
	  result = TRUE;
	  break;
	}
    }
  
  return result;
}

/**
 * btk_targets_include_rich_text:
 * @targets: (array length=n_targets): an array of #BdkAtom<!-- -->s
 * @n_targets: the length of @targets
 * @buffer: a #BtkTextBuffer
 *
 * Determines if any of the targets in @targets can be used to
 * provide rich text.
 *
 * Return value: %TRUE if @targets include a suitable target for rich text,
 *               otherwise %FALSE.
 *
 * Since: 2.10
 **/
gboolean
btk_targets_include_rich_text (BdkAtom       *targets,
                               gint           n_targets,
                               BtkTextBuffer *buffer)
{
  BdkAtom *rich_targets;
  gint n_rich_targets;
  gint i, j;
  gboolean result = FALSE;

  g_return_val_if_fail (targets != NULL || n_targets == 0, FALSE);
  g_return_val_if_fail (BTK_IS_TEXT_BUFFER (buffer), FALSE);

  init_atoms ();

  rich_targets = btk_text_buffer_get_deserialize_formats (buffer,
                                                          &n_rich_targets);

  for (i = 0; i < n_targets; i++)
    {
      for (j = 0; j < n_rich_targets; j++)
        {
          if (targets[i] == rich_targets[j])
            {
              result = TRUE;
              goto done;
            }
        }
    }

 done:
  g_free (rich_targets);

  return result;
}

/**
 * btk_selection_data_targets_include_text:
 * @selection_data: a #BtkSelectionData object
 * 
 * Given a #BtkSelectionData object holding a list of targets,
 * determines if any of the targets in @targets can be used to
 * provide text.
 * 
 * Return value: %TRUE if @selection_data holds a list of targets,
 *   and a suitable target for text is included, otherwise %FALSE.
 **/
gboolean
btk_selection_data_targets_include_text (BtkSelectionData *selection_data)
{
  BdkAtom *targets;
  gint n_targets;
  gboolean result = FALSE;

  g_return_val_if_fail (selection_data != NULL, FALSE);

  init_atoms ();

  if (btk_selection_data_get_targets (selection_data, &targets, &n_targets))
    {
      result = btk_targets_include_text (targets, n_targets);
      g_free (targets);
    }

  return result;
}

/**
 * btk_selection_data_targets_include_rich_text:
 * @selection_data: a #BtkSelectionData object
 * @buffer: a #BtkTextBuffer
 *
 * Given a #BtkSelectionData object holding a list of targets,
 * determines if any of the targets in @targets can be used to
 * provide rich text.
 *
 * Return value: %TRUE if @selection_data holds a list of targets,
 *               and a suitable target for rich text is included,
 *               otherwise %FALSE.
 *
 * Since: 2.10
 **/
gboolean
btk_selection_data_targets_include_rich_text (BtkSelectionData *selection_data,
                                              BtkTextBuffer    *buffer)
{
  BdkAtom *targets;
  gint n_targets;
  gboolean result = FALSE;

  g_return_val_if_fail (selection_data != NULL, FALSE);
  g_return_val_if_fail (BTK_IS_TEXT_BUFFER (buffer), FALSE);

  init_atoms ();

  if (btk_selection_data_get_targets (selection_data, &targets, &n_targets))
    {
      result = btk_targets_include_rich_text (targets, n_targets, buffer);
      g_free (targets);
    }

  return result;
}

/**
 * btk_targets_include_image:
 * @targets: (array length=n_targets): an array of #BdkAtom<!-- -->s
 * @n_targets: the length of @targets
 * @writable: whether to accept only targets for which BTK+ knows
 *   how to convert a pixbuf into the format
 * 
 * Determines if any of the targets in @targets can be used to
 * provide a #BdkPixbuf.
 * 
 * Return value: %TRUE if @targets include a suitable target for images,
 *   otherwise %FALSE.
 *
 * Since: 2.10
 **/
gboolean 
btk_targets_include_image (BdkAtom *targets,
			   gint     n_targets,
			   gboolean writable)
{
  BtkTargetList *list;
  GList *l;
  gint i;
  gboolean result = FALSE;

  g_return_val_if_fail (targets != NULL || n_targets == 0, FALSE);

  list = btk_target_list_new (NULL, 0);
  btk_target_list_add_image_targets (list, 0, writable);
  for (i = 0; i < n_targets && !result; i++)
    {
      for (l = list->list; l; l = l->next)
	{
	  BtkTargetPair *pair = (BtkTargetPair *)l->data;
	  if (pair->target == targets[i])
	    {
	      result = TRUE;
	      break;
	    }
	}
    }
  btk_target_list_unref (list);

  return result;
}
				    
/**
 * btk_selection_data_targets_include_image:
 * @selection_data: a #BtkSelectionData object
 * @writable: whether to accept only targets for which BTK+ knows
 *   how to convert a pixbuf into the format
 * 
 * Given a #BtkSelectionData object holding a list of targets,
 * determines if any of the targets in @targets can be used to
 * provide a #BdkPixbuf.
 * 
 * Return value: %TRUE if @selection_data holds a list of targets,
 *   and a suitable target for images is included, otherwise %FALSE.
 *
 * Since: 2.6
 **/
gboolean 
btk_selection_data_targets_include_image (BtkSelectionData *selection_data,
					  gboolean          writable)
{
  BdkAtom *targets;
  gint n_targets;
  gboolean result = FALSE;

  g_return_val_if_fail (selection_data != NULL, FALSE);

  init_atoms ();

  if (btk_selection_data_get_targets (selection_data, &targets, &n_targets))
    {
      result = btk_targets_include_image (targets, n_targets, writable);
      g_free (targets);
    }

  return result;
}

/**
 * btk_targets_include_uri:
 * @targets: (array length=n_targets): an array of #BdkAtom<!-- -->s
 * @n_targets: the length of @targets
 * 
 * Determines if any of the targets in @targets can be used to
 * provide an uri list.
 * 
 * Return value: %TRUE if @targets include a suitable target for uri lists,
 *   otherwise %FALSE.
 *
 * Since: 2.10
 **/
gboolean 
btk_targets_include_uri (BdkAtom *targets,
			 gint     n_targets)
{
  gint i;
  gboolean result = FALSE;

  g_return_val_if_fail (targets != NULL || n_targets == 0, FALSE);

  /* Keep in sync with btk_target_list_add_uri_targets()
   */

  init_atoms ();

  for (i = 0; i < n_targets; i++)
    {
      if (targets[i] == text_uri_list_atom)
	{
	  result = TRUE;
	  break;
	}
    }
  
  return result;
}

/**
 * btk_selection_data_targets_include_uri:
 * @selection_data: a #BtkSelectionData object
 * 
 * Given a #BtkSelectionData object holding a list of targets,
 * determines if any of the targets in @targets can be used to
 * provide a list or URIs.
 * 
 * Return value: %TRUE if @selection_data holds a list of targets,
 *   and a suitable target for URI lists is included, otherwise %FALSE.
 *
 * Since: 2.10
 **/
gboolean
btk_selection_data_targets_include_uri (BtkSelectionData *selection_data)
{
  BdkAtom *targets;
  gint n_targets;
  gboolean result = FALSE;

  g_return_val_if_fail (selection_data != NULL, FALSE);

  init_atoms ();

  if (btk_selection_data_get_targets (selection_data, &targets, &n_targets))
    {
      result = btk_targets_include_uri (targets, n_targets);
      g_free (targets);
    }

  return result;
}

	  
/*************************************************************
 * btk_selection_init:
 *     Initialize local variables
 *   arguments:
 *     
 *   results:
 *************************************************************/

static void
btk_selection_init (void)
{
  btk_selection_atoms[INCR] = bdk_atom_intern_static_string ("INCR");
  btk_selection_atoms[MULTIPLE] = bdk_atom_intern_static_string ("MULTIPLE");
  btk_selection_atoms[TIMESTAMP] = bdk_atom_intern_static_string ("TIMESTAMP");
  btk_selection_atoms[TARGETS] = bdk_atom_intern_static_string ("TARGETS");
  btk_selection_atoms[SAVE_TARGETS] = bdk_atom_intern_static_string ("SAVE_TARGETS");

  initialize = FALSE;
}

/**
 * btk_selection_clear:
 * @widget: a #BtkWidget
 * @event: the event
 * 
 * The default handler for the #BtkWidget::selection-clear-event
 * signal. 
 * 
 * Return value: %TRUE if the event was handled, otherwise false
 * 
 * Since: 2.2
 *
 * Deprecated: 2.4: Instead of calling this function, chain up from
 * your selection-clear-event handler. Calling this function
 * from any other context is illegal. 
 **/
gboolean
btk_selection_clear (BtkWidget         *widget,
		     BdkEventSelection *event)
{
  /* Note that we filter clear events in bdkselection-x11.c, so
   * that we only will get here if the clear event actually
   * represents a change that we didn't do ourself.
   */
  GList *tmp_list;
  BtkSelectionInfo *selection_info = NULL;
  
  tmp_list = current_selections;
  while (tmp_list)
    {
      selection_info = (BtkSelectionInfo *)tmp_list->data;
      
      if ((selection_info->selection == event->selection) &&
	  (selection_info->widget == widget))
	break;
      
      tmp_list = tmp_list->next;
    }
  
  if (tmp_list)
    {
      current_selections = g_list_remove_link (current_selections, tmp_list);
      g_list_free (tmp_list);
      g_slice_free (BtkSelectionInfo, selection_info);
    }
  
  return TRUE;
}


/*************************************************************
 * _btk_selection_request:
 *     Handler for "selection_request_event" 
 *   arguments:
 *     widget:
 *     event:
 *   results:
 *************************************************************/

gboolean
_btk_selection_request (BtkWidget *widget,
			BdkEventSelection *event)
{
  BdkDisplay *display = btk_widget_get_display (widget);
  BtkIncrInfo *info;
  GList *tmp_list;
  int i;
  gulong selection_max_size;

  if (initialize)
    btk_selection_init ();
  
  selection_max_size = BTK_SELECTION_MAX_SIZE (display);

  /* Check if we own selection */
  
  tmp_list = current_selections;
  while (tmp_list)
    {
      BtkSelectionInfo *selection_info = (BtkSelectionInfo *)tmp_list->data;
      
      if ((selection_info->selection == event->selection) &&
	  (selection_info->widget == widget))
	break;
      
      tmp_list = tmp_list->next;
    }
  
  if (tmp_list == NULL)
    return FALSE;
  
  info = g_slice_new (BtkIncrInfo);

  g_object_ref (widget);
  
  info->selection = event->selection;
  info->num_incrs = 0;

  /* Create BdkWindow structure for the requestor */
#ifdef BDK_WINDOWING_X11
  info->requestor = bdk_x11_window_foreign_new_for_display (display, event->requestor);
#elif defined BDK_WINDOWING_WIN32
  info->requestor = bdk_win32_window_lookup_for_display (display, event->requestor);
  if (!info->requestor)
    info->requestor = bdk_win32_window_foreign_new_for_display (display, event->requestor);
#else
  g_warning ("%s is not implemented", B_STRFUNC);
  info->requestor = NULL;
#endif

  /* Determine conversions we need to perform */
  
  if (event->target == btk_selection_atoms[MULTIPLE])
    {
      BdkAtom  type;
      guchar  *mult_atoms;
      gint     format;
      gint     length;
      
      mult_atoms = NULL;
      
      bdk_error_trap_push ();
      if (!bdk_property_get (info->requestor, event->property, BDK_NONE, /* AnyPropertyType */
			     0, selection_max_size, FALSE,
			     &type, &format, &length, &mult_atoms))
	{
	  bdk_selection_send_notify_for_display (display,
						 event->requestor, 
						 event->selection,
						 event->target, 
						 BDK_NONE, 
						 event->time);
	  g_free (mult_atoms);
	  g_slice_free (BtkIncrInfo, info);
          bdk_error_trap_pop ();
	  return TRUE;
	}
      bdk_error_trap_pop ();

      /* This is annoying; the ICCCM doesn't specify the property type
       * used for the property contents, so the autoconversion for
       * ATOM / ATOM_PAIR in BDK doesn't work properly.
       */
#ifdef BDK_WINDOWING_X11
      if (type != BDK_SELECTION_TYPE_ATOM &&
	  type != bdk_atom_intern_static_string ("ATOM_PAIR"))
	{
	  info->num_conversions = length / (2*sizeof (glong));
	  info->conversions = g_new (BtkIncrConversion, info->num_conversions);
	  
	  for (i=0; i<info->num_conversions; i++)
	    {
	      info->conversions[i].target = bdk_x11_xatom_to_atom_for_display (display,
									       ((glong *)mult_atoms)[2*i]);
	      info->conversions[i].property = bdk_x11_xatom_to_atom_for_display (display,
										 ((glong *)mult_atoms)[2*i + 1]);
	    }

	  g_free (mult_atoms);
	}
      else
#endif
	{
	  info->num_conversions = length / (2*sizeof (BdkAtom));
	  info->conversions = g_new (BtkIncrConversion, info->num_conversions);
	  
	  for (i=0; i<info->num_conversions; i++)
	    {
	      info->conversions[i].target = ((BdkAtom *)mult_atoms)[2*i];
	      info->conversions[i].property = ((BdkAtom *)mult_atoms)[2*i+1];
	    }

	  g_free (mult_atoms);
	}
    }
  else				/* only a single conversion */
    {
      info->conversions = g_new (BtkIncrConversion, 1);
      info->num_conversions = 1;
      info->conversions[0].target = event->target;
      info->conversions[0].property = event->property;
    }
  
  /* Loop through conversions and determine which of these are big
     enough to require doing them via INCR */
  for (i=0; i<info->num_conversions; i++)
    {
      BtkSelectionData data;
      glong items;
      
      data.selection = event->selection;
      data.target = info->conversions[i].target;
      data.data = NULL;
      data.length = -1;
      data.display = btk_widget_get_display (widget);
      
#ifdef DEBUG_SELECTION
      g_message ("Selection %ld, target %ld (%s) requested by 0x%x (property = %ld)",
		 event->selection, 
		 info->conversions[i].target,
		 bdk_atom_name (info->conversions[i].target),
		 event->requestor, info->conversions[i].property);
#endif
      
      btk_selection_invoke_handler (widget, &data, event->time);
      if (data.length < 0)
	{
	  info->conversions[i].property = BDK_NONE;
	  continue;
	}
      
      g_return_val_if_fail ((data.format >= 8) && (data.format % 8 == 0), FALSE);
      
      items = data.length / btk_selection_bytes_per_item (data.format);
      
      if (data.length > selection_max_size)
	{
	  /* Sending via INCR */
#ifdef DEBUG_SELECTION
	  g_message ("Target larger (%d) than max. request size (%ld), sending incrementally\n",
		     data.length, selection_max_size);
#endif
	  
	  info->conversions[i].offset = 0;
	  info->conversions[i].data = data;
	  info->num_incrs++;
	  
	  bdk_property_change (info->requestor, 
			       info->conversions[i].property,
			       btk_selection_atoms[INCR],
			       32,
			       BDK_PROP_MODE_REPLACE,
			       (guchar *)&items, 1);
	}
      else
	{
	  info->conversions[i].offset = -1;
	  
	  bdk_property_change (info->requestor, 
			       info->conversions[i].property,
			       data.type,
			       data.format,
			       BDK_PROP_MODE_REPLACE,
			       data.data, items);
	  
	  g_free (data.data);
	}
    }
  
  /* If we have some INCR's, we need to send the rest of the data in
     a callback */
  
  if (info->num_incrs > 0)
    {
      /* FIXME: this could be dangerous if window doesn't still
	 exist */
      
#ifdef DEBUG_SELECTION
      g_message ("Starting INCR...");
#endif
      
      bdk_window_set_events (info->requestor,
			     bdk_window_get_events (info->requestor) |
			     BDK_PROPERTY_CHANGE_MASK);
      current_incrs = g_list_append (current_incrs, info);
      bdk_threads_add_timeout (1000, (GSourceFunc) btk_selection_incr_timeout, info);
    }
  
  /* If it was a MULTIPLE request, set the property to indicate which
     conversions succeeded */
  if (event->target == btk_selection_atoms[MULTIPLE])
    {
      BdkAtom *mult_atoms = g_new (BdkAtom, 2 * info->num_conversions);
      for (i = 0; i < info->num_conversions; i++)
	{
	  mult_atoms[2*i] = info->conversions[i].target;
	  mult_atoms[2*i+1] = info->conversions[i].property;
	}
      
      bdk_property_change (info->requestor, event->property,
			   bdk_atom_intern_static_string ("ATOM_PAIR"), 32, 
			   BDK_PROP_MODE_REPLACE,
			   (guchar *)mult_atoms, 2*info->num_conversions);
      g_free (mult_atoms);
    }

  if (info->num_conversions == 1 &&
      info->conversions[0].property == BDK_NONE)
    {
      /* Reject the entire conversion */
      bdk_selection_send_notify_for_display (btk_widget_get_display (widget),
					     event->requestor, 
					     event->selection, 
					     event->target, 
					     BDK_NONE, 
					     event->time);
    }
  else
    {
      bdk_selection_send_notify_for_display (btk_widget_get_display (widget),
					     event->requestor, 
					     event->selection,
					     event->target,
					     event->property, 
					     event->time);
    }

  if (info->num_incrs == 0)
    {
      g_free (info->conversions);
      g_slice_free (BtkIncrInfo, info);
    }

  g_object_unref (widget);
  
  return TRUE;
}

/*************************************************************
 * _btk_selection_incr_event:
 *     Called whenever an PropertyNotify event occurs for an 
 *     BdkWindow with user_data == NULL. These will be notifications
 *     that a window we are sending the selection to via the
 *     INCR protocol has deleted a property and is ready for
 *     more data.
 *
 *   arguments:
 *     window:	the requestor window
 *     event:	the property event structure
 *
 *   results:
 *************************************************************/

gboolean
_btk_selection_incr_event (BdkWindow	   *window,
			   BdkEventProperty *event)
{
  GList *tmp_list;
  BtkIncrInfo *info = NULL;
  gint num_bytes;
  guchar *buffer;
  gulong selection_max_size;
  
  int i;
  
  if (event->state != BDK_PROPERTY_DELETE)
    return FALSE;
  
#ifdef DEBUG_SELECTION
  g_message ("PropertyDelete, property %ld", event->atom);
#endif

  selection_max_size = BTK_SELECTION_MAX_SIZE (bdk_window_get_display (window));

  /* Now find the appropriate ongoing INCR */
  tmp_list = current_incrs;
  while (tmp_list)
    {
      info = (BtkIncrInfo *)tmp_list->data;
      if (info->requestor == event->window)
	break;
      
      tmp_list = tmp_list->next;
    }
  
  if (tmp_list == NULL)
    return FALSE;
  
  /* Find out which target this is for */
  for (i=0; i<info->num_conversions; i++)
    {
      if (info->conversions[i].property == event->atom &&
	  info->conversions[i].offset != -1)
	{
	  int bytes_per_item;
	  
	  info->idle_time = 0;
	  
	  if (info->conversions[i].offset == -2) /* only the last 0-length
						    piece*/
	    {
	      num_bytes = 0;
	      buffer = NULL;
	    }
	  else
	    {
	      num_bytes = info->conversions[i].data.length -
		info->conversions[i].offset;
	      buffer = info->conversions[i].data.data + 
		info->conversions[i].offset;
	      
	      if (num_bytes > selection_max_size)
		{
		  num_bytes = selection_max_size;
		  info->conversions[i].offset += selection_max_size;
		}
	      else
		info->conversions[i].offset = -2;
	    }
#ifdef DEBUG_SELECTION
	  g_message ("INCR: put %d bytes (offset = %d) into window 0x%lx , property %ld",
		     num_bytes, info->conversions[i].offset, 
		     BDK_WINDOW_XWINDOW(info->requestor), event->atom);
#endif

	  bytes_per_item = btk_selection_bytes_per_item (info->conversions[i].data.format);
	  bdk_property_change (info->requestor, event->atom,
			       info->conversions[i].data.type,
			       info->conversions[i].data.format,
			       BDK_PROP_MODE_REPLACE,
			       buffer,
			       num_bytes / bytes_per_item);
	  
	  if (info->conversions[i].offset == -2)
	    {
	      g_free (info->conversions[i].data.data);
	      info->conversions[i].data.data = NULL;
	    }
	  
	  if (num_bytes == 0)
	    {
	      info->num_incrs--;
	      info->conversions[i].offset = -1;
	    }
	}
    }
  
  /* Check if we're finished with all the targets */
  
  if (info->num_incrs == 0)
    {
      current_incrs = g_list_remove_link (current_incrs, tmp_list);
      g_list_free (tmp_list);
      /* Let the timeout free it */
    }
  
  return TRUE;
}

/*************************************************************
 * btk_selection_incr_timeout:
 *     Timeout callback for the sending portion of the INCR
 *     protocol
 *   arguments:
 *     info:	Information about this incr
 *   results:
 *************************************************************/

static gint
btk_selection_incr_timeout (BtkIncrInfo *info)
{
  GList *tmp_list;
  gboolean retval;

  /* Determine if retrieval has finished by checking if it still in
     list of pending retrievals */
  
  tmp_list = current_incrs;
  while (tmp_list)
    {
      if (info == (BtkIncrInfo *)tmp_list->data)
	break;
      tmp_list = tmp_list->next;
    }
  
  /* If retrieval is finished */
  if (!tmp_list || info->idle_time >= IDLE_ABORT_TIME)
    {
      if (tmp_list && info->idle_time >= IDLE_ABORT_TIME)
	{
	  current_incrs = g_list_remove_link (current_incrs, tmp_list);
	  g_list_free (tmp_list);
	}
      
      g_free (info->conversions);
      /* FIXME: we should check if requestor window is still in use,
	 and if not, remove it? */
      
      g_slice_free (BtkIncrInfo, info);
      
      retval =  FALSE;		/* remove timeout */
    }
  else
    {
      info->idle_time++;
      
      retval = TRUE;		/* timeout will happen again */
    }
  
  return retval;
}

/*************************************************************
 * _btk_selection_notify:
 *     Handler for "selection-notify-event" signals on windows
 *     where a retrieval is currently in process. The selection
 *     owner has responded to our conversion request.
 *   arguments:
 *     widget:		Widget getting signal
 *     event:		Selection event structure
 *     info:		Information about this retrieval
 *   results:
 *     was event handled?
 *************************************************************/

gboolean
_btk_selection_notify (BtkWidget	       *widget,
		       BdkEventSelection *event)
{
  GList *tmp_list;
  BtkRetrievalInfo *info = NULL;
  guchar  *buffer = NULL;
  gint length;
  BdkAtom type;
  gint	  format;
  
#ifdef DEBUG_SELECTION
  g_message ("Initial receipt of selection %ld, target %ld (property = %ld)",
	     event->selection, event->target, event->property);
#endif
  
  tmp_list = current_retrievals;
  while (tmp_list)
    {
      info = (BtkRetrievalInfo *)tmp_list->data;
      if (info->widget == widget && info->selection == event->selection)
	break;
      tmp_list = tmp_list->next;
    }
  
  if (!tmp_list)		/* no retrieval in progress */
    return FALSE;

  if (event->property != BDK_NONE)
    length = bdk_selection_property_get (widget->window, &buffer, 
					 &type, &format);
  else
    length = 0; /* silence gcc */
  
  if (event->property == BDK_NONE || buffer == NULL)
    {
      current_retrievals = g_list_remove_link (current_retrievals, tmp_list);
      g_list_free (tmp_list);
      /* structure will be freed in timeout */
      btk_selection_retrieval_report (info,
				      BDK_NONE, 0, NULL, -1, event->time);
      
      return TRUE;
    }
  
  if (type == btk_selection_atoms[INCR])
    {
      /* The remainder of the selection will come through PropertyNotify
	 events */

      info->notify_time = event->time;
      info->idle_time = 0;
      info->offset = 0;		/* Mark as OK to proceed */
      bdk_window_set_events (widget->window,
			     bdk_window_get_events (widget->window)
			     | BDK_PROPERTY_CHANGE_MASK);
    }
  else
    {
      /* We don't delete the info structure - that will happen in timeout */
      current_retrievals = g_list_remove_link (current_retrievals, tmp_list);
      g_list_free (tmp_list);
      
      info->offset = length;
      btk_selection_retrieval_report (info,
				      type, format, 
				      buffer, length, event->time);
    }
  
  bdk_property_delete (widget->window, event->property);
  
  g_free (buffer);
  
  return TRUE;
}

/*************************************************************
 * _btk_selection_property_notify:
 *     Handler for "property-notify-event" signals on windows
 *     where a retrieval is currently in process. The selection
 *     owner has added more data.
 *   arguments:
 *     widget:		Widget getting signal
 *     event:		Property event structure
 *     info:		Information about this retrieval
 *   results:
 *     was event handled?
 *************************************************************/

gboolean
_btk_selection_property_notify (BtkWidget	*widget,
				BdkEventProperty *event)
{
  GList *tmp_list;
  BtkRetrievalInfo *info = NULL;
  guchar *new_buffer;
  int length;
  BdkAtom type;
  gint	  format;
  
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

#if defined(BDK_WINDOWING_WIN32) || defined(BDK_WINDOWING_X11)
  if ((event->state != BDK_PROPERTY_NEW_VALUE) ||  /* property was deleted */
      (event->atom != bdk_atom_intern_static_string ("BDK_SELECTION"))) /* not the right property */
#endif
    return FALSE;
  
#ifdef DEBUG_SELECTION
  g_message ("PropertyNewValue, property %ld",
	     event->atom);
#endif
  
  tmp_list = current_retrievals;
  while (tmp_list)
    {
      info = (BtkRetrievalInfo *)tmp_list->data;
      if (info->widget == widget)
	break;
      tmp_list = tmp_list->next;
    }
  
  if (!tmp_list)		/* No retrieval in progress */
    return FALSE;
  
  if (info->offset < 0)		/* We haven't got the SelectionNotify
				   for this retrieval yet */
    return FALSE;
  
  info->idle_time = 0;
  
  length = bdk_selection_property_get (widget->window, &new_buffer, 
				       &type, &format);
  bdk_property_delete (widget->window, event->atom);
  
  /* We could do a lot better efficiency-wise by paying attention to
     what length was sent in the initial INCR transaction, instead of
     doing memory allocation at every step. But its only guaranteed to
     be a _lower bound_ (pretty useless!) */
  
  if (length == 0 || type == BDK_NONE)		/* final zero length portion */
    {
      /* Info structure will be freed in timeout */
      current_retrievals = g_list_remove_link (current_retrievals, tmp_list);
      g_list_free (tmp_list);
      btk_selection_retrieval_report (info,
				      type, format, 
				      (type == BDK_NONE) ?  NULL : info->buffer,
				      (type == BDK_NONE) ?  -1 : info->offset,
				      info->notify_time);
    }
  else				/* append on newly arrived data */
    {
      if (!info->buffer)
	{
#ifdef DEBUG_SELECTION
	  g_message ("Start - Adding %d bytes at offset 0",
		     length);
#endif
	  info->buffer = new_buffer;
	  info->offset = length;
	}
      else
	{
	  
#ifdef DEBUG_SELECTION
	  g_message ("Appending %d bytes at offset %d",
		     length,info->offset);
#endif
	  /* We copy length+1 bytes to preserve guaranteed null termination */
	  info->buffer = g_realloc (info->buffer, info->offset+length+1);
	  memcpy (info->buffer + info->offset, new_buffer, length+1);
	  info->offset += length;
	  g_free (new_buffer);
	}
    }
  
  return TRUE;
}

/*************************************************************
 * btk_selection_retrieval_timeout:
 *     Timeout callback while receiving a selection.
 *   arguments:
 *     info:	Information about this retrieval
 *   results:
 *************************************************************/

static gboolean
btk_selection_retrieval_timeout (BtkRetrievalInfo *info)
{
  GList *tmp_list;
  gboolean retval;

  /* Determine if retrieval has finished by checking if it still in
     list of pending retrievals */
  
  tmp_list = current_retrievals;
  while (tmp_list)
    {
      if (info == (BtkRetrievalInfo *)tmp_list->data)
	break;
      tmp_list = tmp_list->next;
    }
  
  /* If retrieval is finished */
  if (!tmp_list || info->idle_time >= IDLE_ABORT_TIME)
    {
      if (tmp_list && info->idle_time >= IDLE_ABORT_TIME)
	{
	  current_retrievals = g_list_remove_link (current_retrievals, tmp_list);
	  g_list_free (tmp_list);
	  btk_selection_retrieval_report (info, BDK_NONE, 0, NULL, -1, BDK_CURRENT_TIME);
	}
      
      g_free (info->buffer);
      g_slice_free (BtkRetrievalInfo, info);
      
      retval =  FALSE;		/* remove timeout */
    }
  else
    {
      info->idle_time++;
      
      retval =  TRUE;		/* timeout will happen again */
    }

  return retval;
}

/*************************************************************
 * btk_selection_retrieval_report:
 *     Emits a "selection-received" signal.
 *   arguments:
 *     info:	  information about the retrieval that completed
 *     buffer:	  buffer containing data (NULL => errror)
 *     time:      timestamp for data in buffer
 *   results:
 *************************************************************/

static void
btk_selection_retrieval_report (BtkRetrievalInfo *info,
				BdkAtom type, gint format, 
				guchar *buffer, gint length,
				guint32 time)
{
  BtkSelectionData data;
  
  data.selection = info->selection;
  data.target = info->target;
  data.type = type;
  data.format = format;
  
  data.length = length;
  data.data = buffer;
  data.display = btk_widget_get_display (info->widget);
  
  g_signal_emit_by_name (info->widget,
			 "selection-received", 
			 &data, time);
}

/*************************************************************
 * btk_selection_invoke_handler:
 *     Finds and invokes handler for specified
 *     widget/selection/target combination, calls 
 *     btk_selection_default_handler if none exists.
 *
 *   arguments:
 *     widget:	    selection owner
 *     data:	    selection data [INOUT]
 *     time:        time from requeset
 *     
 *   results:
 *     Number of bytes written to buffer, -1 if error
 *************************************************************/

static void
btk_selection_invoke_handler (BtkWidget	       *widget,
			      BtkSelectionData *data,
			      guint             time)
{
  BtkTargetList *target_list;
  guint info;
  

  g_return_if_fail (widget != NULL);

  target_list = btk_selection_target_list_get (widget, data->selection);
  if (data->target != btk_selection_atoms[SAVE_TARGETS] &&
      target_list &&
      btk_target_list_find (target_list, data->target, &info))
    {
      g_signal_emit_by_name (widget,
			     "selection-get",
			     data,
			     info, time);
    }
  else
    btk_selection_default_handler (widget, data);
}

/*************************************************************
 * btk_selection_default_handler:
 *     Handles some default targets that exist for any widget
 *     If it can't fit results into buffer, returns -1. This
 *     won't happen in any conceivable case, since it would
 *     require 1000 selection targets!
 *
 *   arguments:
 *     widget:	    selection owner
 *     data:	    selection data [INOUT]
 *
 *************************************************************/

static void
btk_selection_default_handler (BtkWidget	*widget,
			       BtkSelectionData *data)
{
  if (data->target == btk_selection_atoms[TIMESTAMP])
    {
      /* Time which was used to obtain selection */
      GList *tmp_list;
      BtkSelectionInfo *selection_info;
      
      tmp_list = current_selections;
      while (tmp_list)
	{
	  selection_info = (BtkSelectionInfo *)tmp_list->data;
	  if ((selection_info->widget == widget) &&
	      (selection_info->selection == data->selection))
	    {
	      gulong time = selection_info->time;

	      btk_selection_data_set (data,
				      BDK_SELECTION_TYPE_INTEGER,
				      32,
				      (guchar *)&time,
				      sizeof (time));
	      return;
	    }
	  
	  tmp_list = tmp_list->next;
	}
      
      data->length = -1;
    }
  else if (data->target == btk_selection_atoms[TARGETS])
    {
      /* List of all targets supported for this widget/selection pair */
      BdkAtom *p;
      guint count;
      GList *tmp_list;
      BtkTargetList *target_list;
      BtkTargetPair *pair;
      
      target_list = btk_selection_target_list_get (widget,
						   data->selection);
      count = g_list_length (target_list->list) + 3;
      
      data->type = BDK_SELECTION_TYPE_ATOM;
      data->format = 32;
      data->length = count * sizeof (BdkAtom);

      /* selection data is always terminated by a trailing \0
       */
      p = g_malloc (data->length + 1);
      data->data = (guchar *)p;
      data->data[data->length] = '\0';
      
      *p++ = btk_selection_atoms[TIMESTAMP];
      *p++ = btk_selection_atoms[TARGETS];
      *p++ = btk_selection_atoms[MULTIPLE];
      
      tmp_list = target_list->list;
      while (tmp_list)
	{
	  pair = (BtkTargetPair *)tmp_list->data;
	  *p++ = pair->target;
	  
	  tmp_list = tmp_list->next;
	}
    }
  else if (data->target == btk_selection_atoms[SAVE_TARGETS])
    {
      btk_selection_data_set (data,
			      bdk_atom_intern_static_string ("NULL"),
			      32, NULL, 0);
    }
  else
    {
      data->length = -1;
    }
}


/**
 * btk_selection_data_copy:
 * @data: a pointer to a #BtkSelectionData structure.
 * 
 * Makes a copy of a #BtkSelectionData structure and its data.
 * 
 * Return value: a pointer to a copy of @data.
 **/
BtkSelectionData*
btk_selection_data_copy (BtkSelectionData *data)
{
  BtkSelectionData *new_data;
  
  g_return_val_if_fail (data != NULL, NULL);
  
  new_data = g_slice_new (BtkSelectionData);
  *new_data = *data;

  if (data->data)
    {
      new_data->data = g_malloc (data->length + 1);
      memcpy (new_data->data, data->data, data->length + 1);
    }
  
  return new_data;
}

/**
 * btk_selection_data_free:
 * @data: a pointer to a #BtkSelectionData structure.
 * 
 * Frees a #BtkSelectionData structure returned from
 * btk_selection_data_copy().
 **/
void
btk_selection_data_free (BtkSelectionData *data)
{
  g_return_if_fail (data != NULL);
  
  g_free (data->data);
  
  g_slice_free (BtkSelectionData, data);
}

GType
btk_selection_data_get_type (void)
{
  static GType our_type = 0;
  
  if (our_type == 0)
    our_type = g_boxed_type_register_static (I_("BtkSelectionData"),
					     (GBoxedCopyFunc) btk_selection_data_copy,
					     (GBoxedFreeFunc) btk_selection_data_free);

  return our_type;
}

GType
btk_target_list_get_type (void)
{
  static GType our_type = 0;

  if (our_type == 0)
    our_type = g_boxed_type_register_static (I_("BtkTargetList"),
					     (GBoxedCopyFunc) btk_target_list_ref,
					     (GBoxedFreeFunc) btk_target_list_unref);

  return our_type;
}

static int 
btk_selection_bytes_per_item (gint format)
{
  switch (format)
    {
    case 8:
      return sizeof (char);
      break;
    case 16:
      return sizeof (short);
      break;
    case 32:
      return sizeof (long);
      break;
    default:
      g_assert_not_reached();
    }
  return 0;
}

#define __BTK_SELECTION_C__
#include "btkaliasdef.c"
