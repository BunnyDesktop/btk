/* BTK - The GIMP Toolkit
 * btkfilefilter.h: Filters for selecting a file subset
 * Copyright (C) 2003, Red Hat, Inc.
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

#ifndef __BTK_FILE_FILTER_H__
#define __BTK_FILE_FILTER_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <bunnylib-object.h>

B_BEGIN_DECLS

#define BTK_TYPE_FILE_FILTER              (btk_file_filter_get_type ())
#define BTK_FILE_FILTER(obj)              (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_FILE_FILTER, BtkFileFilter))
#define BTK_IS_FILE_FILTER(obj)           (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_FILE_FILTER))

typedef struct _BtkFileFilter     BtkFileFilter;
typedef struct _BtkFileFilterInfo BtkFileFilterInfo;

typedef enum {
  BTK_FILE_FILTER_FILENAME     = 1 << 0,
  BTK_FILE_FILTER_URI          = 1 << 1,
  BTK_FILE_FILTER_DISPLAY_NAME = 1 << 2,
  BTK_FILE_FILTER_MIME_TYPE    = 1 << 3
} BtkFileFilterFlags;

typedef bboolean (*BtkFileFilterFunc) (const BtkFileFilterInfo *filter_info,
				       bpointer                 data);

struct _BtkFileFilterInfo
{
  BtkFileFilterFlags contains;

  const bchar *filename;
  const bchar *uri;
  const bchar *display_name;
  const bchar *mime_type;
};

GType btk_file_filter_get_type (void) B_GNUC_CONST;

BtkFileFilter *       btk_file_filter_new      (void);
void                  btk_file_filter_set_name (BtkFileFilter *filter,
						const bchar   *name);
const bchar *         btk_file_filter_get_name (BtkFileFilter *filter);

void btk_file_filter_add_mime_type      (BtkFileFilter      *filter,
					 const bchar        *mime_type);
void btk_file_filter_add_pattern        (BtkFileFilter      *filter,
					 const bchar        *pattern);
void btk_file_filter_add_pixbuf_formats (BtkFileFilter      *filter);
void btk_file_filter_add_custom         (BtkFileFilter      *filter,
					 BtkFileFilterFlags  needed,
					 BtkFileFilterFunc   func,
					 bpointer            data,
					 GDestroyNotify      notify);

BtkFileFilterFlags btk_file_filter_get_needed (BtkFileFilter           *filter);
bboolean           btk_file_filter_filter     (BtkFileFilter           *filter,
					       const BtkFileFilterInfo *filter_info);

B_END_DECLS

#endif /* __BTK_FILE_FILTER_H__ */
