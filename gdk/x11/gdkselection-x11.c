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
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#include "config.h"
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <string.h>

#include "bdkx.h"
#include "bdkproperty.h"
#include "bdkselection.h"
#include "bdkprivate.h"
#include "bdkprivate-x11.h"
#include "bdkdisplay-x11.h"
#include "bdkalias.h"

typedef struct _OwnerInfo OwnerInfo;

struct _OwnerInfo
{
  BdkAtom    selection;
  BdkWindow *owner;
  gulong     serial;
};

static GSList *owner_list;

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
	  owner_list = g_slist_remove (owner_list, info);
	  g_free (info);
	}
    }
}

/* We only pass through those SelectionClear events that actually
 * reflect changes to the selection owner that we didn't make ourself.
 */
gboolean
_bdk_selection_filter_clear_event (XSelectionClearEvent *event)
{
  GSList *tmp_list = owner_list;
  BdkDisplay *display = bdk_x11_lookup_xdisplay (event->display);
  
  while (tmp_list)
    {
      OwnerInfo *info = tmp_list->data;

      if (bdk_drawable_get_display (info->owner) == display &&
	  info->selection == bdk_x11_xatom_to_atom_for_display (display, event->selection))
	{
	  if ((BDK_DRAWABLE_XID (info->owner) == event->window &&
	       event->serial >= info->serial))
	    {
	      owner_list = g_slist_remove (owner_list, info);
	      g_free (info);
	      return TRUE;
	    }
	  else
	    return FALSE;
	}
      tmp_list = tmp_list->next;
    }

  return FALSE;
}
/**
 * bdk_selection_owner_set_for_display:
 * @display: the #BdkDisplay.
 * @owner: a #BdkWindow or %NULL to indicate that the owner for
 *         the given should be unset.
 * @selection: an atom identifying a selection.
 * @time_: timestamp to use when setting the selection. 
 *         If this is older than the timestamp given last time the owner was 
 *         set for the given selection, the request will be ignored.
 * @send_event: if %TRUE, and the new owner is different from the current
 *              owner, the current owner will be sent a SelectionClear event.
 *
 * Sets the #BdkWindow @owner as the current owner of the selection @selection.
 * 
 * Returns: %TRUE if the selection owner was successfully changed to owner,
 *	    otherwise %FALSE. 
 *
 * Since: 2.2
 */
gboolean
bdk_selection_owner_set_for_display (BdkDisplay *display,
				     BdkWindow  *owner,
				     BdkAtom     selection,
				     guint32     time, 
				     gboolean    send_event)
{
  Display *xdisplay;
  Window xwindow;
  Atom xselection;
  GSList *tmp_list;
  OwnerInfo *info;

  g_return_val_if_fail (BDK_IS_DISPLAY (display), FALSE);
  g_return_val_if_fail (selection != BDK_NONE, FALSE);

  if (display->closed)
    return FALSE;

  if (owner) 
    {
      if (BDK_WINDOW_DESTROYED (owner) || !BDK_WINDOW_IS_X11 (owner))
	return FALSE;
      
      bdk_window_ensure_native (owner);
      xdisplay = BDK_WINDOW_XDISPLAY (owner);
      xwindow = BDK_WINDOW_XID (owner);
    }
  else 
    {
      xdisplay = BDK_DISPLAY_XDISPLAY (display);
      xwindow = None;
    }
  
  xselection = bdk_x11_atom_to_xatom_for_display (display, selection);

  tmp_list = owner_list;
  while (tmp_list)
    {
      info = tmp_list->data;
      if (info->selection == selection) 
	{
	  owner_list = g_slist_remove (owner_list, info);
	  g_free (info);
	  break;
	}
      tmp_list = tmp_list->next;
    }

  if (owner)
    {
      info = g_new (OwnerInfo, 1);
      info->owner = owner;
      info->serial = NextRequest (BDK_WINDOW_XDISPLAY (owner));
      info->selection = selection;

      owner_list = g_slist_prepend (owner_list, info);
    }

  XSetSelectionOwner (xdisplay, xselection, xwindow, time);

  return (XGetSelectionOwner (xdisplay, xselection) == xwindow);
}

/**
 * bdk_selection_owner_get_for_display:
 * @display: a #BdkDisplay.
 * @selection: an atom indentifying a selection.
 *
 * Determine the owner of the given selection.
 *
 * Note that the return value may be owned by a different 
 * process if a foreign window was previously created for that
 * window, but a new foreign window will never be created by this call. 
 *
 * Returns: if there is a selection owner for this window, and it is a 
 *    window known to the current process, the #BdkWindow that owns the 
 *    selection, otherwise %NULL.
 *
 * Since: 2.2
 */ 
BdkWindow *
bdk_selection_owner_get_for_display (BdkDisplay *display,
				     BdkAtom     selection)
{
  Window xwindow;
  
  g_return_val_if_fail (BDK_IS_DISPLAY (display), NULL);
  g_return_val_if_fail (selection != BDK_NONE, NULL);
  
  if (display->closed)
    return NULL;
  
  xwindow = XGetSelectionOwner (BDK_DISPLAY_XDISPLAY (display),
				bdk_x11_atom_to_xatom_for_display (display, 
								   selection));
  if (xwindow == None)
    return NULL;

  return bdk_window_lookup_for_display (display, xwindow);
}

void
bdk_selection_convert (BdkWindow *requestor,
		       BdkAtom    selection,
		       BdkAtom    target,
		       guint32    time)
{
  BdkDisplay *display;

  g_return_if_fail (selection != BDK_NONE);

  if (BDK_WINDOW_DESTROYED (requestor) || !BDK_WINDOW_IS_X11 (requestor))
    return;

  bdk_window_ensure_native (requestor);
  display = BDK_WINDOW_DISPLAY (requestor);

  XConvertSelection (BDK_WINDOW_XDISPLAY (requestor),
		     bdk_x11_atom_to_xatom_for_display (display, selection),
		     bdk_x11_atom_to_xatom_for_display (display, target),
		     bdk_x11_atom_to_xatom_for_display (display, _bdk_selection_property), 
		     BDK_WINDOW_XID (requestor), time);
}

/**
 * bdk_selection_property_get:
 * @requestor: the window on which the data is stored
 * @data: location to store a pointer to the retrieved data.
       If the retrieval failed, %NULL we be stored here, otherwise, it
       will be non-%NULL and the returned data should be freed with g_free()
       when you are finished using it. The length of the
       allocated memory is one more than the length
       of the returned data, and the final byte will always
       be zero, to ensure nul-termination of strings.
 * @prop_type: location to store the type of the property.
 * @prop_format: location to store the format of the property.
 * 
 * Retrieves selection data that was stored by the selection
 * data in response to a call to bdk_selection_convert(). This function
 * will not be used by applications, who should use the #BtkClipboard
 * API instead.
 * 
 * Return value: the length of the retrieved data.
 **/
gint
bdk_selection_property_get (BdkWindow  *requestor,
			    guchar    **data,
			    BdkAtom    *ret_type,
			    gint       *ret_format)
{
  gulong nitems;
  gulong nbytes;
  gulong length = 0;		/* Quiet GCC */
  Atom prop_type;
  gint prop_format;
  guchar *t = NULL;
  BdkDisplay *display; 

  g_return_val_if_fail (requestor != NULL, 0);
  g_return_val_if_fail (BDK_IS_WINDOW (requestor), 0);
  g_return_val_if_fail (BDK_WINDOW_IS_X11 (requestor), 0);
  
  display = BDK_WINDOW_DISPLAY (requestor);

  if (BDK_WINDOW_DESTROYED (requestor) || !BDK_WINDOW_IS_X11 (requestor))
    goto err;

  t = NULL;

  /* We can't delete the selection here, because it might be the INCR
     protocol, in which case the client has to make sure they'll be
     notified of PropertyChange events _before_ the property is deleted.
     Otherwise there's no guarantee we'll win the race ... */
  if (XGetWindowProperty (BDK_DRAWABLE_XDISPLAY (requestor),
			  BDK_DRAWABLE_XID (requestor),
			  bdk_x11_atom_to_xatom_for_display (display, _bdk_selection_property),
			  0, 0x1FFFFFFF /* MAXINT32 / 4 */, False, 
			  AnyPropertyType, &prop_type, &prop_format,
			  &nitems, &nbytes, &t) != Success)
    goto err;
    
  if (prop_type != None)
    {
      if (ret_type)
	*ret_type = bdk_x11_xatom_to_atom_for_display (display, prop_type);
      if (ret_format)
	*ret_format = prop_format;

      if (prop_type == XA_ATOM ||
	  prop_type == bdk_x11_get_xatom_by_name_for_display (display, "ATOM_PAIR"))
	{
	  Atom* atoms = (Atom*) t;
	  BdkAtom* atoms_dest;
	  gint num_atom, i;

	  if (prop_format != 32)
	    goto err;
	  
	  num_atom = nitems;
	  length = sizeof (BdkAtom) * num_atom + 1;

	  if (data)
	    {
	      *data = g_malloc (length);
	      (*data)[length - 1] = '\0';
	      atoms_dest = (BdkAtom *)(*data);
	  
	      for (i=0; i < num_atom; i++)
		atoms_dest[i] = bdk_x11_xatom_to_atom_for_display (display, atoms[i]);
	    }
	}
      else
	{
	  switch (prop_format)
	    {
	    case 8:
	      length = nitems;
	      break;
	    case 16:
	      length = sizeof(short) * nitems;
	      break;
	    case 32:
	      length = sizeof(long) * nitems;
	      break;
	    default:
	      g_assert_not_reached ();
	      break;
	    }
	  
	  /* Add on an extra byte to handle null termination.  X guarantees
	     that t will be 1 longer than nitems and null terminated */
	  length += 1;

	  if (data)
	    *data = g_memdup (t, length);
	}
      
      if (t)
	XFree (t);
      
      return length - 1;
    }

 err:
  if (ret_type)
    *ret_type = BDK_NONE;
  if (ret_format)
    *ret_format = 0;
  if (data)
    *data = NULL;
  
  return 0;
}

/**
 * bdk_selection_send_notify_for_display:
 * @display: the #BdkDisplay where @requestor is realized
 * @requestor: window to which to deliver response.
 * @selection: selection that was requested.
 * @target: target that was selected.
 * @property: property in which the selection owner stored the data,
 *            or %BDK_NONE to indicate that the request was rejected.
 * @time_: timestamp. 
 *
 * Send a response to SelectionRequest event.
 *
 * Since: 2.2
 **/
void
bdk_selection_send_notify_for_display (BdkDisplay       *display,
				       BdkNativeWindow  requestor,
				       BdkAtom          selection,
				       BdkAtom          target,
				       BdkAtom          property, 
				       guint32          time)
{
  XSelectionEvent xevent;
  
  g_return_if_fail (BDK_IS_DISPLAY (display));
  
  xevent.type = SelectionNotify;
  xevent.serial = 0;
  xevent.send_event = True;
  xevent.requestor = requestor;
  xevent.selection = bdk_x11_atom_to_xatom_for_display (display, selection);
  xevent.target = bdk_x11_atom_to_xatom_for_display (display, target);
  if (property == BDK_NONE)
    xevent.property = None;
  else
    xevent.property = bdk_x11_atom_to_xatom_for_display (display, property);
  xevent.time = time;

  _bdk_send_xevent (display, requestor, False, NoEventMask, (XEvent*) & xevent);
}

/**
 * bdk_text_property_to_text_list_for_display:
 * @display: The #BdkDisplay where the encoding is defined.
 * @encoding: an atom representing the encoding. The most 
 *    common values for this are STRING, or COMPOUND_TEXT. 
 *    This is value used as the type for the property.
 * @format: the format of the property.
 * @text: The text data.
 * @length: The number of items to transform.
 * @list: location to store a terminated array of strings in 
 *    the encoding of the current locale. This array should be 
 *    freed using bdk_free_text_list().
 *
 * Convert a text string from the encoding as it is stored 
 * in a property into an array of strings in the encoding of
 * the current locale. (The elements of the array represent the
 * nul-separated elements of the original text string.)
 *
 * Returns: the number of strings stored in list, or 0, 
 * if the conversion failed. 
 *
 * Since: 2.2
 *
 * Deprecated:2.24: Use bdk_x11_display_text_property_to_text_list()
 */
gint
bdk_text_property_to_text_list_for_display (BdkDisplay   *display,
					    BdkAtom       encoding,
					    gint          format,
					    const guchar *text,
					    gint          length,
					    gchar      ***list)
{
  return bdk_x11_display_text_property_to_text_list (display,
                                                     encoding,
                                                     format,
                                                     text,
                                                     length,
                                                     list);
}

gint
bdk_x11_display_text_property_to_text_list (BdkDisplay   *display,
					    BdkAtom       encoding,
					    gint          format, 
					    const guchar *text,
					    gint          length,
					    gchar      ***list)
{
  XTextProperty property;
  gint count = 0;
  gint res;
  gchar **local_list;
  g_return_val_if_fail (BDK_IS_DISPLAY (display), 0);

  if (list)
    *list = NULL;

  if (display->closed)
    return 0;

  property.value = (guchar *)text;
  property.encoding = bdk_x11_atom_to_xatom_for_display (display, encoding);
  property.format = format;
  property.nitems = length;
  res = XmbTextPropertyToTextList (BDK_DISPLAY_XDISPLAY (display), &property, 
				   &local_list, &count);
  if (res == XNoMemory || res == XLocaleNotSupported || res == XConverterNotFound)
    return 0;
  else
    {
      if (list)
	*list = local_list;
      else
	XFreeStringList (local_list);
      
      return count;
    }
}

void
bdk_free_text_list (gchar **list)
{
  bdk_x11_free_text_list (list);
}

void
bdk_x11_free_text_list (gchar **list)
{
  g_return_if_fail (list != NULL);

  XFreeStringList (list);
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
	{
	  str = g_strndup (p, q - p);
	  if (!g_utf8_validate (str, -1, NULL))
	    {
	      g_warning ("Error converting selection from UTF8_STRING");
	      g_free (str);
	      str = NULL;
	    }
	}

      if (str)
	{
	  strings = g_slist_prepend (strings, str);
	  n_strings++;
	}

      p = q + 1;
    }

  if (list)
    {
      *list = g_new (gchar *, n_strings + 1);
      (*list)[n_strings] = NULL;
    }
     
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
  
  g_slist_free (strings);

  return n_strings;
}

/**
 * bdk_text_property_to_utf8_list_for_display:
 * @display:  a #BdkDisplay
 * @encoding: an atom representing the encoding of the text
 * @format:   the format of the property
 * @text:     the text to convert
 * @length:   the length of @text, in bytes
 * @list:     location to store the list of strings or %NULL. The
 *            list should be freed with g_strfreev().
 * 
 * Converts a text property in the given encoding to
 * a list of UTF-8 strings. 
 * 
 * Return value: the number of strings in the resulting
 *               list.
 *
 * Since: 2.2
 **/
gint 
bdk_text_property_to_utf8_list_for_display (BdkDisplay    *display,
					    BdkAtom        encoding,
					    gint           format,
					    const guchar  *text,
					    gint           length,
					    gchar       ***list)
{
  g_return_val_if_fail (text != NULL, 0);
  g_return_val_if_fail (length >= 0, 0);
  g_return_val_if_fail (BDK_IS_DISPLAY (display), 0);
  
  if (encoding == BDK_TARGET_STRING)
    {
      return make_list ((gchar *)text, length, TRUE, list);
    }
  else if (encoding == bdk_atom_intern_static_string ("UTF8_STRING"))
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
      local_count = bdk_text_property_to_text_list_for_display (display,
								encoding,
								format, 
								text,
								length,
								&local_list);
      if (list)
	*list = g_new (gchar *, local_count + 1);
      
      for (i=0; i<local_count; i++)
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
		{
		  if (g_utf8_validate (local_list[i], -1, NULL))
		    (*list)[count++] = g_strdup (local_list[i]);
		  else
		    g_warning ("Error converting selection");
		}
	    }
	}

      if (local_count)
	bdk_free_text_list (local_list);
      
      if (list)
	(*list)[count] = NULL;

      return count;
    }
}

/**
 * bdk_string_to_compound_text_for_display:
 * @display:  the #BdkDisplay where the encoding is defined.
 * @str:      a nul-terminated string.
 * @encoding: location to store the encoding atom 
 *	      (to be used as the type for the property).
 * @format:   location to store the format of the property
 * @ctext:    location to store newly allocated data for the property.
 * @length:   the length of @text, in bytes
 * 
 * Convert a string from the encoding of the current 
 * locale into a form suitable for storing in a window property.
 * 
 * Returns: 0 upon success, non-zero upon failure. 
 *
 * Since: 2.2
 *
 * Deprecated:2.24: Use bdk_x11_display_string_to_compound_text()
 **/
gint
bdk_string_to_compound_text_for_display (BdkDisplay  *display,
					 const gchar *str,
					 BdkAtom     *encoding,
					 gint        *format,
					 guchar     **ctext,
					 gint        *length)
{
  return bdk_x11_display_string_to_compound_text (display, str, encoding, format, ctext, length);
}

gint
bdk_x11_display_string_to_compound_text (BdkDisplay  *display,
					 const gchar *str,
					 BdkAtom     *encoding,
					 gint        *format,
					 guchar     **ctext,
					 gint        *length)
{
  gint res;
  XTextProperty property;

  g_return_val_if_fail (BDK_IS_DISPLAY (display), 0);

  if (display->closed)
    res = XLocaleNotSupported;
  else
    res = XmbTextListToTextProperty (BDK_DISPLAY_XDISPLAY (display), 
				     (char **)&str, 1, XCompoundTextStyle,
				     &property);
  if (res != Success)
    {
      property.encoding = None;
      property.format = None;
      property.value = NULL;
      property.nitems = 0;
    }

  if (encoding)
    *encoding = bdk_x11_xatom_to_atom_for_display (display, property.encoding);
  if (format)
    *format = property.format;
  if (ctext)
    *ctext = property.value;
  if (length)
    *length = property.nitems;

  return res;
}

/* The specifications for COMPOUND_TEXT and STRING specify that C0 and
 * C1 are not allowed except for \n and \t, however the X conversions
 * routines for COMPOUND_TEXT only enforce this in one direction,
 * causing cut-and-paste of \r and \r\n separated text to fail.
 * This routine strips out all non-allowed C0 and C1 characters
 * from the input string and also canonicalizes \r, and \r\n to \n
 */
static gchar * 
sanitize_utf8 (const gchar *src,
	       gboolean return_latin1)
{
  gint len = strlen (src);
  GString *result = g_string_sized_new (len);
  const gchar *p = src;

  while (*p)
    {
      if (*p == '\r')
	{
	  p++;
	  if (*p == '\n')
	    p++;

	  g_string_append_c (result, '\n');
	}
      else
	{
	  gunichar ch = g_utf8_get_char (p);
	  
	  if (!((ch < 0x20 && ch != '\t' && ch != '\n') || (ch >= 0x7f && ch < 0xa0)))
	    {
	      if (return_latin1)
		{
		  if (ch <= 0xff)
		    g_string_append_c (result, ch);
		  else
		    g_string_append_printf (result,
					    ch < 0x10000 ? "\\u%04x" : "\\U%08x",
					    ch);
		}
	      else
		{
		  char buf[7];
		  gint buflen;
		  
		  buflen = g_unichar_to_utf8 (ch, buf);
		  g_string_append_len (result, buf, buflen);
		}
	    }

	  p = g_utf8_next_char (p);
	}
    }

  return g_string_free (result, FALSE);
}

/**
 * bdk_utf8_to_string_target:
 * @str: a UTF-8 string
 * 
 * Converts an UTF-8 string into the best possible representation
 * as a STRING. The representation of characters not in STRING
 * is not specified; it may be as pseudo-escape sequences
 * \x{ABCD}, or it may be in some other form of approximation.
 * 
 * Return value: the newly-allocated string, or %NULL if the
 *               conversion failed. (It should not fail for
 *               any properly formed UTF-8 string unless system
 *               limits like memory or file descriptors are exceeded.)
 **/
gchar *
bdk_utf8_to_string_target (const gchar *str)
{
  return sanitize_utf8 (str, TRUE);
}

/**
 * bdk_utf8_to_compound_text_for_display:
 * @display:  a #BdkDisplay
 * @str:      a UTF-8 string
 * @encoding: location to store resulting encoding
 * @format:   location to store format of the result
 * @ctext:    location to store the data of the result
 * @length:   location to store the length of the data
 *            stored in @ctext
 * 
 * Converts from UTF-8 to compound text. 
 * 
 * Return value: %TRUE if the conversion succeeded, otherwise
 *               %FALSE.
 *
 * Since: 2.2
 *
 * Deprecated:2.24: Use bdk_x11_display_utf8_to_compound_text()
 **/
gboolean
bdk_utf8_to_compound_text_for_display (BdkDisplay  *display,
				       const gchar *str,
				       BdkAtom     *encoding,
				       gint        *format,
				       guchar     **ctext,
				       gint        *length)
{
  return bdk_x11_display_utf8_to_compound_text (display, str, encoding, format, ctext, length);
}

gboolean
bdk_x11_display_utf8_to_compound_text (BdkDisplay  *display,
				       const gchar *str,
				       BdkAtom     *encoding,
				       gint        *format,
				       guchar     **ctext,
				       gint        *length)
{
  gboolean need_conversion;
  const gchar *charset;
  gchar *locale_str, *tmp_str;
  GError *error = NULL;
  gboolean result;

  g_return_val_if_fail (str != NULL, FALSE);
  g_return_val_if_fail (BDK_IS_DISPLAY (display), FALSE);

  need_conversion = !g_get_charset (&charset);

  tmp_str = sanitize_utf8 (str, FALSE);

  if (need_conversion)
    {
      locale_str = g_convert (tmp_str, -1,
			      charset, "UTF-8",
			      NULL, NULL, &error);
      g_free (tmp_str);

      if (!locale_str)
	{
	  if (!(error->domain = G_CONVERT_ERROR &&
		error->code == G_CONVERT_ERROR_ILLEGAL_SEQUENCE))
	    {
	      g_warning ("Error converting from UTF-8 to '%s': %s",
			 charset, error->message);
	    }
	  g_error_free (error);

	  if (encoding)
	    *encoding = None;
	  if (format)
	    *format = None;
	  if (ctext)
	    *ctext = NULL;
	  if (length)
	    *length = 0;

	  return FALSE;
	}
    }
  else
    locale_str = tmp_str;
    
  result = bdk_string_to_compound_text_for_display (display, locale_str,
						    encoding, format, 
						    ctext, length);
  result = (result == Success? TRUE : FALSE);
  
  g_free (locale_str);

  return result;
}

void bdk_free_compound_text (guchar *ctext)
{
  bdk_x11_free_compound_text (ctext);
}

void bdk_x11_free_compound_text (guchar *ctext)
{
  if (ctext)
    XFree (ctext);
}

#define __BDK_SELECTION_X11_C__
#include "bdkaliasdef.c"
