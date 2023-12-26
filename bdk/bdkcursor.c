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
#include "bdkcursor.h"
#include "bdkdisplay.h"
#include "bdkinternals.h"
#include "bdkalias.h"

GType
bdk_cursor_get_type (void)
{
  static GType our_type = 0;
  
  if (our_type == 0)
    our_type = g_boxed_type_register_static (g_intern_static_string ("BdkCursor"),
					     (GBoxedCopyFunc)bdk_cursor_ref,
					     (GBoxedFreeFunc)bdk_cursor_unref);
  return our_type;
}

/**
 * bdk_cursor_ref:
 * @cursor: a #BdkCursor
 * 
 * Adds a reference to @cursor.
 * 
 * Return value: (transfer full): Same @cursor that was passed in
 **/
BdkCursor*
bdk_cursor_ref (BdkCursor *cursor)
{
  g_return_val_if_fail (cursor != NULL, NULL);
  g_return_val_if_fail (cursor->ref_count > 0, NULL);

  cursor->ref_count += 1;

  return cursor;
}

/**
 * bdk_cursor_unref:
 * @cursor: a #BdkCursor
 *
 * Removes a reference from @cursor, deallocating the cursor
 * if no references remain.
 * 
 **/
void
bdk_cursor_unref (BdkCursor *cursor)
{
  g_return_if_fail (cursor != NULL);
  g_return_if_fail (cursor->ref_count > 0);

  cursor->ref_count -= 1;

  if (cursor->ref_count == 0)
    _bdk_cursor_destroy (cursor);
}

/**
 * bdk_cursor_new:
 * @cursor_type: cursor to create
 * 
 * Creates a new cursor from the set of builtin cursors for the default display.
 * See bdk_cursor_new_for_display().
 *
 * To make the cursor invisible, use %BDK_BLANK_CURSOR.
 * 
 * Return value: a new #BdkCursor
 **/
BdkCursor*
bdk_cursor_new (BdkCursorType cursor_type)
{
  return bdk_cursor_new_for_display (bdk_display_get_default(), cursor_type);
}

/**
 * bdk_cursor_get_cursor_type:
 * @cursor:  a #BdkCursor
 *
 * Returns the cursor type for this cursor.
 *
 * Return value: a #BdkCursorType
 *
 * Since: 2.22
 **/
BdkCursorType
bdk_cursor_get_cursor_type (BdkCursor *cursor)
{
  g_return_val_if_fail (cursor != NULL, BDK_BLANK_CURSOR);
  return cursor->type;
}

#define __BDK_CURSOR_C__
#include "bdkaliasdef.c"
