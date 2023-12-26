/* btkiconcachevalidator.c
 * Copyright (C) 2007 Red Hat, Inc
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
#include "config.h"
#include "btkiconcachevalidator.h"

#include <bunnylib.h>

#undef BDK_PIXBUF_DISABLE_DEPRECATED
#include <bdk-pixbuf/bdk-pixdata.h>


#define VERBOSE(x) 

#define check(name,condition) \
  if (!(condition)) \
    { \
      VERBOSE(g_print ("bad %s\n", (name))); \
      return FALSE; \
    } 

static inline bboolean 
get_uint16 (CacheInfo *info, 
            buint32    offset, 
            buint16   *value)
{
  if (offset < info->cache_size) 
    { 
      *value = GUINT16_FROM_BE(*(buint16*)(info->cache + offset)); 
      return TRUE;
    }
  else 
    { 
      *value = 0;
      return FALSE; 
    } 
}

static inline bboolean 
get_uint32 (CacheInfo *info, 
            buint32    offset, 
            buint32   *value)
{
  if (offset < info->cache_size) 
    { 
      *value = GUINT32_FROM_BE(*(buint32*)(info->cache + offset)); 
      return TRUE;
    }
  else 
    { 
      *value = 0;
      return FALSE; 
    } 
}

static bboolean 
check_version (CacheInfo *info)
{
  buint16 major, minor;

  check ("major version", get_uint16 (info, 0, &major) && major == 1);
  check ("minor version", get_uint16 (info, 2, &minor) && minor == 0);

  return TRUE;
}

static bboolean 
check_string (CacheInfo *info, 
              buint32    offset)
{
  check ("string offset", offset < info->cache_size);

  if (info->flags & CHECK_STRINGS) 
    {
      bint i;
      bchar c;

      /* assume no string is longer than 1k */
      for (i = 0; i < 1024; i++) 
        { 
          check ("string offset", offset + i < info->cache_size)
          c = *(info->cache + offset + i);
          if (c == '\0')
            break;
          check ("string content", g_ascii_isgraph (c));
        }
      check ("string length", i < 1024);
    }

  return TRUE;
}

static bboolean 
check_string_utf8 (CacheInfo *info, 
                   buint32    offset)
{
  check ("string offset", offset < info->cache_size);

  if (info->flags & CHECK_STRINGS) 
    {
      bint i;
      bchar c;

      /* assume no string is longer than 1k */
      for (i = 0; i < 1024; i++) 
        { 
          check ("string offset", offset + i < info->cache_size)
            c = *(info->cache + offset + i);
          if (c == '\0')
            break;
        }
      check ("string length", i < 1024);
      check ("string utf8 data", g_utf8_validate((char *)(info->cache + offset), -1, NULL));
    }

  return TRUE;
}

static bboolean 
check_directory_list (CacheInfo *info, 
                      buint32    offset)
{
  buint32 directory_offset;
  bint i;
	
  check ("offset, directory list", get_uint32 (info, offset, &info->n_directories));
	
  for (i = 0; i < info->n_directories; i++) 
    {
      check ("offset, directory", get_uint32 (info, offset + 4 + 4 * i, &directory_offset));
      if (!check_string (info, directory_offset))
        return FALSE;
    }

  return TRUE;
}

static bboolean 
check_pixel_data (CacheInfo *info, 
                  buint32    offset)
{
  buint32 type;
  buint32 length;

  check ("offset, pixel data type", get_uint32 (info, offset, &type));
  check ("offset, pixel data length", get_uint32 (info, offset + 4, &length));

  check ("pixel data type", type == 0);
  check ("pixel data length", offset + 8 + length < info->cache_size);

  if (info->flags & CHECK_PIXBUFS) 
    {
      BdkPixdata data; 
 
      check ("pixel data", bdk_pixdata_deserialize (&data, length,
                                                    (const buint8*)info->cache + offset + 8, 
                                                    NULL));
    }
	
  return TRUE;
}

static bboolean 
check_embedded_rect (CacheInfo *info, 
                     buint32    offset)
{
  check ("embedded rect", offset + 4 < info->cache_size);

  return TRUE;
}

static bboolean 
check_attach_point_list (CacheInfo *info, 
                         buint32    offset)
{
  buint32 n_attach_points;

  check ("offset, attach point list", get_uint32 (info, offset, &n_attach_points));
  check ("attach points", offset + 4 + 4 * n_attach_points < info->cache_size); 

  return TRUE;
}

static bboolean 
check_display_name_list (CacheInfo *info, 
                         buint32    offset)
{
  buint32 n_display_names, ofs;
  bint i;

  check ("offset, display name list", 
         get_uint32 (info, offset, &n_display_names));
  for (i = 0; i < n_display_names; i++) 
    {
      get_uint32(info, offset + 4 + 8 * i, &ofs);
      if (!check_string (info, ofs))
        return FALSE;
      get_uint32(info, offset + 4 + 8 * i + 4, &ofs);
      if (!check_string_utf8 (info, ofs))
        return FALSE;
    }
	
  return TRUE;	
}

static bboolean 
check_meta_data (CacheInfo *info, 
                 buint32    offset)
{
  buint32 embedded_rect_offset;
  buint32 attach_point_list_offset;
  buint32 display_name_list_offset;

  check ("offset, embedded rect", 
         get_uint32 (info, offset, &embedded_rect_offset));
  check ("offset, attach point list", 
         get_uint32 (info, offset + 4, &attach_point_list_offset));
  check ("offset, display name list", 
         get_uint32 (info, offset + 8, &display_name_list_offset));

  if (embedded_rect_offset != 0) 
    {
      if (!check_embedded_rect (info, embedded_rect_offset))
        return FALSE;
    }

  if (attach_point_list_offset != 0) 
    {
      if (!check_attach_point_list (info, attach_point_list_offset))
        return FALSE;
    }

  if (display_name_list_offset != 0) 
    {
      if (!check_display_name_list (info, display_name_list_offset))
        return FALSE;
    }

  return TRUE;
}

static bboolean 
check_image_data (CacheInfo *info, 
                  buint32    offset)
{
  buint32 pixel_data_offset;
  buint32 meta_data_offset;

  check ("offset, pixel data", get_uint32 (info, offset, &pixel_data_offset));
  check ("offset, meta data", get_uint32 (info, offset + 4, &meta_data_offset));

  if (pixel_data_offset != 0) 
    {
      if (!check_pixel_data (info, pixel_data_offset))
        return FALSE;
    }
  if (meta_data_offset != 0) 
    {
      if (!check_meta_data (info, meta_data_offset))
        return FALSE;
    }
	
  return TRUE;
}

static bboolean 
check_image (CacheInfo *info, 
             buint32    offset)
{
  buint16 index;
  buint16 flags;
  buint32 image_data_offset;

  check ("offset, image index", get_uint16 (info, offset, &index));
  check ("offset, image flags", get_uint16 (info, offset + 2, &flags));	
  check ("offset, image data offset", 
         get_uint32 (info, offset + 4, &image_data_offset));

  check ("image index", index < info->n_directories);
  check ("image flags", flags < 16);

  if (image_data_offset != 0) 
    {
      if (!check_image_data (info, image_data_offset))
        return FALSE;
    }

  return TRUE;
}

static bboolean 
check_image_list (CacheInfo *info, 
                  buint32    offset)
{
  buint32 n_images;
  bint i;

  check ("offset, image list", get_uint32 (info, offset, &n_images));
	
  for (i = 0; i < n_images; i++) 
    {
      if (!check_image (info, offset + 4 + 8 * i))
        return FALSE;
    }

  return TRUE;
}

static bboolean 
check_icon (CacheInfo *info, 
            buint32    offset)
{
  buint32 chain_offset;
  buint32 name_offset;
  buint32 image_list_offset;

  check ("offset, icon chain", get_uint32 (info, offset, &chain_offset));
  check ("offset, icon name", get_uint32 (info, offset + 4, &name_offset));
  check ("offset, icon image list", get_uint32 (info, offset + 8, 
         &image_list_offset));

  if (!check_string (info, name_offset))
    return FALSE;
  if (!check_image_list (info, image_list_offset))
    return FALSE;
  if (chain_offset != 0xffffffff) 
    {
      if (!check_icon (info, chain_offset))
        return FALSE;
    }

  return TRUE;
}

static bboolean 
check_hash (CacheInfo *info, 
            buint32    offset)
{
  buint32 n_buckets, icon_offset;
  bint i;

  check ("offset, hash size", get_uint32 (info, offset, &n_buckets));

  for (i = 0; i < n_buckets; i++) 
    {
      check ("offset, hash chain", 
             get_uint32 (info, offset + 4 + 4 * i, &icon_offset));
      if (icon_offset != 0xffffffff) 
        {
          if (!check_icon (info, icon_offset))
            return FALSE;
        }
    }

  return TRUE;
}

/**
 * _btk_icon_cache_validate:
 * @info: a CacheInfo structure 
 *
 * Validates the icon cache passed in the @cache and
 * @cache_size fields of the @info structure. The
 * validator checks that offsets specified in the
 * cache do not point outside the mapped area, that
 * strings look reasonable, and that pixbufs can
 * be deserialized. The amount of validation can
 * be controlled with the @flags field.  
 *
 * Return value: %TRUE if the cache is valid
 */
bboolean 
_btk_icon_cache_validate (CacheInfo *info)
{
  buint32 hash_offset;
  buint32 directory_list_offset;

  if (!check_version (info))
    return FALSE;
  check ("header, hash offset", get_uint32 (info, 4, &hash_offset));
  check ("header, directory list offset", get_uint32 (info, 8, &directory_list_offset));
  if (!check_directory_list (info, directory_list_offset))
    return FALSE;

  if (!check_hash (info, hash_offset))
    return FALSE;

  return TRUE;
}

