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

bboolean
bdk_selection_owner_set_for_display (BdkDisplay *display,
				     BdkWindow  *owner,
				     BdkAtom     selection,
				     buint32     time,
				     bint        send_event)
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
		       buint32    time)
{
  /* FIXME: Implement */
}

bint
bdk_selection_property_get (BdkWindow  *requestor,
			    buchar    **data,
			    BdkAtom    *ret_type,
			    bint       *ret_format)
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
				       buint32          time)
{
  /* FIXME: Implement */
}

bint
bdk_text_property_to_text_list_for_display (BdkDisplay   *display,
					    BdkAtom       encoding,
					    bint          format, 
					    const buchar *text,
					    bint          length,
					    bchar      ***list)
{
  /* FIXME: Implement */
  return 0;
}

bint
bdk_string_to_compound_text_for_display (BdkDisplay  *display,
					 const bchar *str,
					 BdkAtom     *encoding,
					 bint        *format,
					 buchar     **ctext,
					 bint        *length)
{
  /* FIXME: Implement */
  return 0;
}

void bdk_free_compound_text (buchar *ctext)
{
  /* FIXME: Implement */
}

bchar *
bdk_utf8_to_string_target (const bchar *str)
{
  /* FIXME: Implement */
  return NULL;
}

bboolean
bdk_utf8_to_compound_text_for_display (BdkDisplay  *display,
				       const bchar *str,
				       BdkAtom     *encoding,
				       bint        *format,
				       buchar     **ctext,
				       bint        *length)
{
  /* FIXME: Implement */
  return 0;
}

void
bdk_free_text_list (bchar **list)
{
  g_return_if_fail (list != NULL);

  g_free (*list);
  g_free (list);
}

static bint
make_list (const bchar  *text,
	   bint          length,
	   bboolean      latin1,
	   bchar      ***list)
{
  GSList *strings = NULL;
  bint n_strings = 0;
  bint i;
  const bchar *p = text;
  const bchar *q;
  GSList *tmp_list;
  GError *error = NULL;

  while (p < text + length)
    {
      bchar *str;
      
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
    *list = g_new0 (bchar *, n_strings + 1);

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

bint 
bdk_text_property_to_utf8_list_for_display (BdkDisplay    *display,
					    BdkAtom        encoding,
					    bint           format,
					    const buchar  *text,
					    bint           length,
					    bchar       ***list)
{
  g_return_val_if_fail (text != NULL, 0);
  g_return_val_if_fail (length >= 0, 0);

  if (encoding == BDK_TARGET_STRING)
    {
      return make_list ((bchar *)text, length, TRUE, list);
    }
  else if (encoding == bdk_atom_intern_static_string ("UTF8_STRING"))
    {
      return make_list ((bchar *)text, length, FALSE, list);
    }
  else
    {
      bchar *enc_name = bdk_atom_name (encoding);

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
  bchar *target = bdk_atom_name (atom);
  NSString *ret = bdk_quartz_target_to_pasteboard_type_libbtk_only (target);
  g_free (target);

  return ret;
}
