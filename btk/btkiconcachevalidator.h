/* btkiconcachevalidator.4
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
#ifndef __BTK_ICON_CACHE_VALIDATOR_H__
#define __BTK_ICON_CACHE_VALIDATOR_H__


#include <bunnylib.h>

B_BEGIN_DECLS

enum {
  CHECK_OFFSETS = 1,
  CHECK_STRINGS = 2,
  CHECK_PIXBUFS = 4
};

typedef struct {
  const bchar *cache;
  bsize cache_size;
  buint32 n_directories;
  bint flags;
} CacheInfo;

bboolean _btk_icon_cache_validate (CacheInfo *info);

B_END_DECLS

#endif  /* __BTK_ICON_CACHE_VALIDATOR_H__ */
