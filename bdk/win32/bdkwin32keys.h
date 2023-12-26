/* BDK - The GIMP Drawing Kit
 * Copyright (C) 2010 Red Hat, Inc.
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
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __BDK_WIN32_KEYS_H__
#define __BDK_WIN32_KEYS_H__


#include <bdk/bdk.h>

B_BEGIN_DECLS

/**
 * BdkWin32KeymapMatch:
 * @BDK_WIN32_KEYMAP_MATCH_NONE: no matches found. Output is not valid.
 * @BDK_WIN32_KEYMAP_MATCH_INCOMPLETE: the sequence matches so far, but is incomplete. Output is not valid.
 * @BDK_WIN32_KEYMAP_MATCH_PARTIAL: the sequence matches up to the last key,
 *     which does not match. Output is valid.
 * @BDK_WIN32_KEYMAP_MATCH_EXACT: the sequence matches exactly. Output is valid.
 *
 * An enumeration describing the result of a deadkey combination matching.
 */
typedef enum
{
  BDK_WIN32_KEYMAP_MATCH_NONE,
  BDK_WIN32_KEYMAP_MATCH_INCOMPLETE,
  BDK_WIN32_KEYMAP_MATCH_PARTIAL,
  BDK_WIN32_KEYMAP_MATCH_EXACT
} BdkWin32KeymapMatch;

#ifdef BDK_COMPILATION
typedef struct _BdkWin32Keymap BdkWin32Keymap;
#else
typedef BdkKeymap BdkWin32Keymap;
#endif
typedef struct _BdkWin32KeymapClass BdkWin32KeymapClass;

#define BDK_TYPE_WIN32_KEYMAP              (bdk_win32_keymap_get_type())
#define BDK_WIN32_KEYMAP(object)           (B_TYPE_CHECK_INSTANCE_CAST ((object), BDK_TYPE_WIN32_KEYMAP, BdkWin32Keymap))
#define BDK_WIN32_KEYMAP_CLASS(klass)      (B_TYPE_CHECK_CLASS_CAST ((klass), BDK_TYPE_WIN32_KEYMAP, BdkWin32KeymapClass))
#define BDK_IS_WIN32_KEYMAP(object)        (B_TYPE_CHECK_INSTANCE_TYPE ((object), BDK_TYPE_WIN32_KEYMAP))
#define BDK_IS_WIN32_KEYMAP_CLASS(klass)   (B_TYPE_CHECK_CLASS_TYPE ((klass), BDK_TYPE_WIN32_KEYMAP))
#define BDK_WIN32_KEYMAP_GET_CLASS(obj)    (B_TYPE_INSTANCE_GET_CLASS ((obj), BDK_TYPE_WIN32_KEYMAP, BdkWin32KeymapClass))

GType bdk_win32_keymap_get_type (void);

BdkWin32KeymapMatch bdk_win32_keymap_check_compose (BdkWin32Keymap *keymap,
                                                    guint          *compose_buffer,
                                                    gsize           compose_buffer_len,
                                                    guint16        *output,
                                                    gsize          *output_len);

B_END_DECLS

#endif /* __BDK_WIN32_KEYMAP_H__ */
