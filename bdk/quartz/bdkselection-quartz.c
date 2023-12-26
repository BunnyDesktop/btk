/* bdkselection-quartz.c
 *
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 1998-2002 Tor Lillqvist
 * Copyright (C) 2005 Imendio AB
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

#include "bdkselection.h"
#include "bdkproperty.h"
#include "bdkquartz.h"

gboolean
bdk_selection_owner_set_for_display (BdkDisplay *display,
				     BdkWindow  *owner,
				     BdkAtom     selection,
				     guint32     time,
				     gint        send_event)
{
  /* FIXME: Implement */
  return TRUE;
}

BdkWindow*
bdk_selection_owner_get_for_display (BdkDisplay *display,
				     BdkAtom     selection)
{
  /* FIXME: Implement */
  return NULL;
}

void
bdk_selection_convert (BdkWindow *requestor,
		       BdkAtom    selection,
		       BdkAtom    target,
		       guint32    time)
{
  /* FIXME: Implement */
}

gint
bdk_selection_property_get (BdkWindow  *requestor,
			    guchar    **data,
			    BdkAtom    *ret_type,
			    gint       *ret_format)
{
  /* FIXME: Implement */
  return 0;
}

void
bdk_selection_send_notify_for_display (BdkDisplay      *display,
				       BdkNativeWindow  requestor,
				       BdkAtom          selection,
				       BdkAtom          target,
				       BdkAtom          property,
				       guint32          time)
{
  /* FIXME: Implement */
}

gint
bdk_text_property_to_text_list_for_display (BdkDisplay   *display,
					    BdkAtom       encoding,
					    gint          format, 
					    const guchar *text,
					    gint          length,
					    gchar      ***list)
{
  /* FIXME: Implement */
  return 0;
}

gint
bdk_string_to_compound_text_for_display (BdkDisplay  *display,
					 const gchar *str,
					 BdkAtom     *encoding,
					 gint        *format,
					 guchar     **ctext,
					 gint        *length)
{
  /* FIXME: Implement */
  return 0;
}

void bdk_free_compound_text (guchar *ctext)
{
  /* FIXME: Implement */
}

gchar *
bdk_utf8_to_string_target (const gchar *str)
{
  /* FIXME: Implement */
  return NULL;
}

gboolean
bdk_utf8_to_compound_text_for_display (BdkDisplay  *display,
				       const gchar *str,
				       BdkAtom     *encoding,
				       gint        *format,
				       guchar     **ctext,
				       gint        *length)
{
  /* FIXME: Implement */
  return 0;
}

void
bdk_free_text_list (gchar **list)
{
  g_return_if_fail (list != NULL);

  g_free (*list);
  g_free (list);
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
    *list = g_new0 (gchar *, n_strings + 1);

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
      gchar *enc_name = bdk_atom_name (encoding);

      g_warning ("bdk_text_property_to_utf8_list_for_display: encoding %s not handled\n", enc_name);
      g_free (enc_name);

      if (list)
	*list = NULL;

      return 0;
    }
}

BdkAtom
bdk_quartz_pasteboard_type_to_atom_libbtk_only (NSString *type)
{
  if ([type isEqualToString:NSStringPboardType])
    return bdk_atom_intern_static_string ("UTF8_STRING");
  else if ([type isEqualToString:NSTIFFPboardType])
    return bdk_atom_intern_static_string ("image/tiff");
  else if ([type isEqualToString:NSColorPboardType])
    return bdk_atom_intern_static_string ("application/x-color");
  else if ([type isEqualToString:NSURLPboardType])
    return bdk_atom_intern_static_string ("text/uri-list");
  else
    return bdk_atom_intern ([type UTF8String], FALSE);
}

NSString *
bdk_quartz_target_to_pasteboard_type_libbtk_only (const char *target)
{
  if (strcmp (target, "UTF8_STRING") == 0)
    return NSStringPboardType;
  else if (strcmp (target, "image/tiff") == 0)
    return NSTIFFPboardType;
  else if (strcmp (target, "application/x-color") == 0)
    return NSColorPboardType;
  else if (strcmp (target, "text/uri-list") == 0)
    return NSURLPboardType;
  else
    return [NSString stringWithUTF8String:target];
}

NSString *
bdk_quartz_atom_to_pasteboard_type_libbtk_only (BdkAtom atom)
{
  gchar *target = bdk_atom_name (atom);
  NSString *ret = bdk_quartz_target_to_pasteboard_type_libbtk_only (target);
  g_free (target);

  return ret;
}
