/* btkiconcache.h
 * Copyright (C) 2004  Anders Carlsson <andersca@bunny.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __BTK_ICON_CACHE_H__
#define __BTK_ICON_CACHE_H__

#include <bdk-pixbuf/bdk-pixbuf.h>
#include <bdk/bdk.h>

typedef struct _BtkIconCache BtkIconCache;
typedef struct _BtkIconData BtkIconData;

struct _BtkIconData
{
  gboolean has_embedded_rect;
  gint x0, y0, x1, y1;
  
  BdkPoint *attach_points;
  gint n_attach_points;

  gchar *display_name;
};

BtkIconCache *_btk_icon_cache_new            (const gchar  *data);
BtkIconCache *_btk_icon_cache_new_for_path   (const gchar  *path);
gint          _btk_icon_cache_get_directory_index  (BtkIconCache *cache,
					            const gchar  *directory);
gboolean      _btk_icon_cache_has_icon       (BtkIconCache *cache,
					      const gchar  *icon_name);
gboolean      _btk_icon_cache_has_icon_in_directory (BtkIconCache *cache,
					             const gchar  *icon_name,
					             const gchar  *directory);
void	      _btk_icon_cache_add_icons      (BtkIconCache *cache,
					      const gchar  *directory,
					      GHashTable   *hash_table);

gint          _btk_icon_cache_get_icon_flags (BtkIconCache *cache,
					      const gchar  *icon_name,
					      gint          directory_index);
BdkPixbuf    *_btk_icon_cache_get_icon       (BtkIconCache *cache,
					      const gchar  *icon_name,
					      gint          directory_index);
BtkIconData  *_btk_icon_cache_get_icon_data  (BtkIconCache *cache,
 					      const gchar  *icon_name,
 					      gint          directory_index);

BtkIconCache *_btk_icon_cache_ref            (BtkIconCache *cache);
void          _btk_icon_cache_unref          (BtkIconCache *cache);


#endif /* __BTK_ICON_CACHE_H__ */
