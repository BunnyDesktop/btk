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

#ifndef __BDK_PROPERTY_H__
#define __BDK_PROPERTY_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BDK_H_INSIDE__) && !defined (BDK_COMPILATION)
#error "Only <bdk/bdk.h> can be included directly."
#endif

#include <bdk/bdktypes.h>

B_BEGIN_DECLS

typedef enum
{
  BDK_PROP_MODE_REPLACE,
  BDK_PROP_MODE_PREPEND,
  BDK_PROP_MODE_APPEND
} BdkPropMode;

BdkAtom bdk_atom_intern (const gchar *atom_name,
			 gboolean     only_if_exists);
BdkAtom bdk_atom_intern_static_string (const gchar *atom_name);
gchar*  bdk_atom_name   (BdkAtom      atom);

gboolean bdk_property_get    (BdkWindow     *window,
			      BdkAtom        property,
			      BdkAtom        type,
			      gulong         offset,
			      gulong         length,
			      gint           pdelete,
			      BdkAtom       *actual_property_type,
			      gint          *actual_format,
			      gint          *actual_length,
			      guchar       **data);
void     bdk_property_change (BdkWindow     *window,
			      BdkAtom        property,
			      BdkAtom        type,
			      gint           format,
			      BdkPropMode    mode,
			      const guchar  *data,
			      gint           nelements);
void     bdk_property_delete (BdkWindow     *window,
			      BdkAtom        property);
#ifndef BDK_MULTIHEAD_SAFE
#ifndef BDK_DISABLE_DEPRECATED
gint bdk_text_property_to_text_list (BdkAtom        encoding,
				     gint           format,
				     const guchar  *text,
				     gint           length,
				     gchar       ***list);
gboolean bdk_utf8_to_compound_text (const gchar *str,
				    BdkAtom     *encoding,
				    gint        *format,
				    guchar     **ctext,
				    gint        *length);
gint bdk_string_to_compound_text    (const gchar   *str,
				     BdkAtom       *encoding,
				     gint          *format,
				     guchar       **ctext,
				     gint          *length);
gint bdk_text_property_to_utf8_list (BdkAtom        encoding,
				     gint           format,
				     const guchar  *text,
				     gint           length,
				     gchar       ***list);
#endif
#endif

gint bdk_text_property_to_utf8_list_for_display (BdkDisplay     *display,
						 BdkAtom         encoding,
						 gint            format,
						 const guchar   *text,
						 gint            length,
						 gchar        ***list);

gchar   *bdk_utf8_to_string_target   (const gchar *str);
#ifndef BDK_DISABLE_DEPRECATED
gint bdk_text_property_to_text_list_for_display (BdkDisplay     *display,
						 BdkAtom         encoding,
						 gint            format,
						 const guchar   *text,
						 gint            length,
						 gchar        ***list);
gint     bdk_string_to_compound_text_for_display (BdkDisplay   *display,
						  const gchar  *str,
						  BdkAtom      *encoding,
						  gint         *format,
						  guchar      **ctext,
						  gint         *length);
gboolean bdk_utf8_to_compound_text_for_display   (BdkDisplay   *display,
						  const gchar  *str,
						  BdkAtom      *encoding,
						  gint         *format,
						  guchar      **ctext,
						  gint         *length);

void bdk_free_text_list             (gchar        **list);
void bdk_free_compound_text         (guchar        *ctext);
#endif

B_END_DECLS

#endif /* __BDK_PROPERTY_H__ */
