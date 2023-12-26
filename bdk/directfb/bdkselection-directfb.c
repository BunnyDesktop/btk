/* BDK - The GIMP Drawing Kit
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
 * file for a list of people on the BTK+ Team.
 */

/*
 * BTK+ DirectFB backend
 * Copyright (C) 2001-2002  convergence integrated media GmbH
 * Copyright (C) 2002-2004  convergence GmbH
 * Written by Denis Oliver Kropp <dok@convergence.de> and
 *            Sven Neumann <sven@convergence.de>
 */

#include "config.h"

#include <string.h>

#include "bdkdirectfb.h"
#include "bdkprivate-directfb.h"

#include "bdkproperty.h"
#include "bdkselection.h"
#include "bdkprivate.h"
#include "bdkalias.h"


typedef struct _OwnerInfo OwnerInfo;

struct _OwnerInfo
{
  BdkAtom    selection;
  BdkWindow *owner;
};

GSList *owner_list = NULL;

/* When a window is destroyed we check if it is the owner
 * of any selections. This is somewhat inefficient, but
 * owner_list is typically short, and it is a low memory,
 * low code solution
 */
void
_bdk_selection_window_destroyed (BdkWindow *window)
{
  GSList *tmp_list = owner_list;

  while (tmp_list)
    {
      OwnerInfo *info = tmp_list->data;

      tmp_list = tmp_list->next;

      if (info->owner == window)
	{
	  owner_list = b_slist_remove (owner_list, info);
	  g_free (info);
	}
    }
}

gint
bdk_selection_owner_set_for_display (BdkDisplay *display,
                                     BdkWindow  *owner,
                                     BdkAtom     selection,
                                     guint32     time,
                                     gint        send_event)
{
  GSList    *tmp_list;
  OwnerInfo *info;

  tmp_list = owner_list;
  while (tmp_list)
    {
      info = tmp_list->data;
      if (info->selection == selection)
	{
	  owner_list = b_slist_remove (owner_list, info);
	  g_free (info);
	  break;
	}
      tmp_list = tmp_list->next;
    }

  if (owner)
    {
      info = g_new (OwnerInfo, 1);
      info->owner = owner;
      info->selection = selection;

      owner_list = b_slist_prepend (owner_list, info);
    }

  return TRUE;
}

BdkWindow *
bdk_selection_owner_get_for_display (BdkDisplay *display,
                                     BdkAtom     selection)
{
  OwnerInfo *info;
  GSList    *tmp_list;

  tmp_list = owner_list;
  while (tmp_list)
    {
      info = tmp_list->data;
      if (info->selection == selection)
	{
	  return info->owner;
	}
      tmp_list = tmp_list->next;
    }
  return NULL;
}

void
bdk_selection_convert (BdkWindow *requestor,
		       BdkAtom    selection,
		       BdkAtom    target,
		       guint32    time)
{
  BdkEvent  *event;
  BdkWindow *owner;
  BdkWindow *event_window;

  owner = bdk_selection_owner_get (selection);

  if (owner)
    {
      event_window = bdk_directfb_other_event_window (owner,
                                                      BDK_SELECTION_REQUEST);
      if (event_window)
	{
	  event = bdk_directfb_event_make (event_window,
                                           BDK_SELECTION_REQUEST);
	  event->selection.requestor = BDK_WINDOW_DFB_ID (requestor);
	  event->selection.selection = selection;
	  event->selection.target    = target;
	  event->selection.property  = _bdk_selection_property;
	}
    }
  else
    {
      /* If no owner for the specified selection exists, the X server
       * generates a SelectionNotify event to the requestor with property None.
       */
      bdk_selection_send_notify (BDK_WINDOW_DFB_ID (requestor),
				 selection,
				 target,
				 BDK_NONE,
				 0);
    }
}

gint
bdk_selection_property_get (BdkWindow  *requestor,
			    guchar    **data,
			    BdkAtom    *ret_type,
			    gint       *ret_format)
{
  guchar *t = NULL;
  BdkAtom prop_type;
  gint prop_format;
  gint prop_len;

  g_return_val_if_fail (requestor != NULL, 0);
  g_return_val_if_fail (BDK_IS_WINDOW (requestor), 0);

  if (!bdk_property_get (requestor,
			 _bdk_selection_property,
			 0/*AnyPropertyType?*/,
			 0, 0,
			 FALSE,
			 &prop_type, &prop_format, &prop_len,
			 &t))
    {
      *data = NULL;
      return 0;
    }

  if (ret_type)
    *ret_type = prop_type;
  if (ret_format)
    *ret_format = prop_format;

  if (!bdk_property_get (requestor,
			 _bdk_selection_property,
			 0/*AnyPropertyType?*/,
			 0, prop_len + 1,
			 FALSE,
			 &prop_type, &prop_format, &prop_len,
			 &t))
    {
      *data = NULL;
      return 0;
    }

  *data = t;

  return prop_len;
}


void
bdk_selection_send_notify_for_display (BdkDisplay *display,
                                       guint32     requestor,
                                       BdkAtom     selection,
                                       BdkAtom     target,
                                       BdkAtom     property,
                                       guint32     time)
{
  BdkEvent  *event;
  BdkWindow *event_window;

  event_window = bdk_window_lookup ((BdkNativeWindow) requestor);

  if (!event_window)
    return;

  event_window = bdk_directfb_other_event_window (event_window,
                                                  BDK_SELECTION_NOTIFY);

  if (event_window)
    {
      event = bdk_directfb_event_make (event_window, BDK_SELECTION_NOTIFY);
      event->selection.selection = selection;
      event->selection.target = target;
      event->selection.property = property;
      event->selection.requestor = (BdkNativeWindow) requestor;
    }
}

gint
bdk_text_property_to_text_list_for_display (BdkDisplay      *display,
                                            BdkAtom          encoding,
                                            gint             format,
                                            const guchar    *text,
                                            gint             length,
                                            gchar         ***list)
{
  g_warning ("bdk_text_property_to_text_list() not implemented\n");
  return 0;
}

void
bdk_free_text_list (gchar **list)
{
  g_return_if_fail (list != NULL);
  g_warning ("bdk_free_text_list() not implemented\n");
}

gint
bdk_string_to_compound_text_for_display (BdkDisplay   *display,
                                         const gchar  *str,
                                         BdkAtom      *encoding,
                                         gint         *format,
                                         guchar      **ctext,
                                         gint         *length)
{
  g_warning ("bdk_string_to_compound_text() not implemented\n");
  return 0;
}

void
bdk_free_compound_text (guchar *ctext)
{
  g_warning ("bdk_free_compound_text() not implemented\n");
}

/**
 * bdk_utf8_to_string_target:
 * @str: a UTF-8 string
 *
 * Convert an UTF-8 string into the best possible representation
 * as a STRING. The representation of characters not in STRING
 * is not specified; it may be as pseudo-escape sequences
 * \x{ABCD}, or it may be in some other form of approximation.
 *
 * Return value: the newly allocated string, or %NULL if the
 *               conversion failed. (It should not fail for
 *               any properly formed UTF-8 string.)
 **/
gchar *
bdk_utf8_to_string_target (const gchar *str)
{
  g_warning ("bdk_utf8_to_string_target() not implemented\n");
  return 0;
}

/**
 * bdk_utf8_to_compound_text:
 * @str:      a UTF-8 string
 * @encoding: location to store resulting encoding
 * @format:   location to store format of the result
 * @ctext:    location to store the data of the result
 * @length:   location to store the length of the data
 *            stored in @ctext
 *
 * Convert from UTF-8 to compound text.
 *
 * Return value: %TRUE if the conversion succeeded, otherwise
 *               false.
 **/
gboolean
bdk_utf8_to_compound_text_for_display (BdkDisplay   *display,
                                       const gchar  *str,
                                       BdkAtom      *encoding,
                                       gint         *format,
                                       guchar      **ctext,
                                       gint         *length)
{
  g_warning ("bdk_utf8_to_compound_text() not implemented\n");
  return 0;
}

static gint
make_list (const gchar  *text,
	   gint          length,
	   gboolean      latin1,
	   gchar      ***list)
{
  GSList *strings = NULL;
  gint n_strings = 0;
  gint i;
  const gchar *p = text;
  const gchar *q;
  GSList *tmp_list;
  GError *error = NULL;

  while (p < text + length)
    {
      gchar *str;

      q = p;
      while (*q && q < text + length)
	q++;

      if (latin1)
	{
	  str = g_convert (p, q - p,
			   "UTF-8", "ISO-8859-1",
			   NULL, NULL, &error);

	  if (!str)
	    {
	      g_warning ("Error converting selection from STRING: %s",
			 error->message);
	      g_error_free (error);
	    }
	}
      else
	str = g_strndup (p, q - p);

      if (str)
	{
	  strings = b_slist_prepend (strings, str);
	  n_strings++;
	}

      p = q + 1;
    }

  if (list)
    *list = g_new (gchar *, n_strings + 1);

  (*list)[n_strings] = NULL;

  i = n_strings;
  tmp_list = strings;
  while (tmp_list)
    {
      if (list)
	(*list)[--i] = tmp_list->data;
      else
	g_free (tmp_list->data);

      tmp_list = tmp_list->next;
    }

  b_slist_free (strings);

  return n_strings;
}


/**
 * bdk_text_property_to_utf8_list:
 * @encoding: an atom representing the encoding of the text
 * @format:   the format of the property
 * @text:     the text to convert
 * @length:   the length of @text, in bytes
 * @list:     location to store the list of strings or %NULL. The
 *            list should be freed with g_strfreev().
 *
 * Convert a text property in the giving encoding to
 * a list of UTF-8 strings.
 *
 * Return value: the number of strings in the resulting
 *               list.
 **/
gint
bdk_text_property_to_utf8_list_for_display (BdkDisplay     *display,
                                            BdkAtom         encoding,
                                            gint            format,
                                            const guchar   *text,
                                            gint            length,
                                            gchar        ***list)
{
  g_return_val_if_fail (text != NULL, 0);
  g_return_val_if_fail (length >= 0, 0);

  if (encoding == BDK_TARGET_STRING)
    {
      return make_list ((gchar *)text, length, TRUE, list);
    }
  else if (encoding == bdk_atom_intern ("UTF8_STRING", FALSE))
    {
      return make_list ((gchar *)text, length, FALSE, list);
    }
  else
    {
      gchar **local_list;
      gint local_count;
      gint i;
      const gchar *charset = NULL;
      gboolean need_conversion = !g_get_charset (&charset);
      gint count = 0;
      GError *error = NULL;

      /* Probably COMPOUND text, we fall back to Xlib routines
       */
      local_count = bdk_text_property_to_text_list (encoding,
						    format,
						    text,
						    length,
						    &local_list);
      if (list)
	*list = g_new (gchar *, local_count + 1);

      for (i = 0; i < local_count; i++)
	{
	  /* list contains stuff in our default encoding
	   */
	  if (need_conversion)
	    {
	      gchar *utf = g_convert (local_list[i], -1,
				      "UTF-8", charset,
				      NULL, NULL, &error);
	      if (utf)
		{
		  if (list)
		    (*list)[count++] = utf;
		  else
		    g_free (utf);
		}
	      else
		{
		  g_warning ("Error converting to UTF-8 from '%s': %s",
			     charset, error->message);
		  g_error_free (error);
		  error = NULL;
		}
	    }
	  else
	    {
	      if (list)
		(*list)[count++] = g_strdup (local_list[i]);
	    }
	}

      bdk_free_text_list (local_list);
      (*list)[count] = NULL;

      return count;
    }
}

#define __BDK_SELECTION_X11_C__
#include "bdkaliasdef.c"
