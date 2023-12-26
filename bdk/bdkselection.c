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
#include "bdkproperty.h"
#include "bdkdisplay.h"
#include "bdkselection.h"
#include "bdkalias.h"

bboolean
bdk_selection_owner_set (BdkWindow *owner,
			 BdkAtom    selection,
			 buint32    time,
			 bboolean   send_event)
{
  return bdk_selection_owner_set_for_display (bdk_display_get_default (),
					      owner, selection, 
					      time, send_event);
}

BdkWindow*
bdk_selection_owner_get (BdkAtom selection)
{
  return bdk_selection_owner_get_for_display (bdk_display_get_default (), 
					      selection);
}

void
bdk_selection_send_notify (BdkNativeWindow requestor,
			   BdkAtom         selection,
			   BdkAtom         target,
			   BdkAtom         property,
			   buint32         time)
{
  bdk_selection_send_notify_for_display (bdk_display_get_default (), 
					 requestor, selection, 
					 target, property, time);
}

bint
bdk_text_property_to_text_list (BdkAtom       encoding,
				bint          format, 
				const buchar *text,
				bint          length,
				bchar      ***list)
{
  return bdk_text_property_to_text_list_for_display (bdk_display_get_default (),
						     encoding, format, text, length, list);
}

/**
 * bdk_text_property_to_utf8_list:
 * @encoding: an atom representing the encoding of the text
 * @format:   the format of the property
 * @text:     the text to convert
 * @length:   the length of @text, in bytes
 * @list: (allow-none):     location to store the list of strings or %NULL. The
 *            list should be freed with g_strfreev().
 * 
 * Convert a text property in the giving encoding to
 * a list of UTF-8 strings. 
 * 
 * Return value: the number of strings in the resulting
 *               list.
 **/
bint 
bdk_text_property_to_utf8_list (BdkAtom        encoding,
				bint           format,
				const buchar  *text,
				bint           length,
				bchar       ***list)
{
  return bdk_text_property_to_utf8_list_for_display (bdk_display_get_default (),
						     encoding, format, text, length, list);
}

bint
bdk_string_to_compound_text (const bchar *str,
			     BdkAtom     *encoding,
			     bint        *format,
			     buchar     **ctext,
			     bint        *length)
{
  return bdk_string_to_compound_text_for_display (bdk_display_get_default (),
						  str, encoding, format, 
						  ctext, length);
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
bboolean
bdk_utf8_to_compound_text (const bchar *str,
			   BdkAtom     *encoding,
			   bint        *format,
			   buchar     **ctext,
			   bint        *length)
{
  return bdk_utf8_to_compound_text_for_display (bdk_display_get_default (),
						str, encoding, format, 
						ctext, length);
}

#define __BDK_SELECTION_C__
#include "bdkaliasdef.c"
