/* BTK - The GIMP Toolkit
 * btkrecentfilter.h - Filter object for recently used resources
 * Copyright (C) 2006, Emmanuele Bassi
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

#ifndef __BTK_RECENT_FILTER_H__
#define __BTK_RECENT_FILTER_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <bunnylib-object.h>

G_BEGIN_DECLS

#define BTK_TYPE_RECENT_FILTER		(btk_recent_filter_get_type ())
#define BTK_RECENT_FILTER(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_RECENT_FILTER, BtkRecentFilter))
#define BTK_IS_RECENT_FILTER(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_RECENT_FILTER))

typedef struct _BtkRecentFilter		BtkRecentFilter;
typedef struct _BtkRecentFilterInfo	BtkRecentFilterInfo;

typedef enum {
  BTK_RECENT_FILTER_URI          = 1 << 0,
  BTK_RECENT_FILTER_DISPLAY_NAME = 1 << 1,
  BTK_RECENT_FILTER_MIME_TYPE    = 1 << 2,
  BTK_RECENT_FILTER_APPLICATION  = 1 << 3,
  BTK_RECENT_FILTER_GROUP        = 1 << 4,
  BTK_RECENT_FILTER_AGE          = 1 << 5
} BtkRecentFilterFlags;

typedef gboolean (*BtkRecentFilterFunc) (const BtkRecentFilterInfo *filter_info,
					 gpointer                   user_data);

struct _BtkRecentFilterInfo
{
  BtkRecentFilterFlags contains;

  const gchar *uri;
  const gchar *display_name;
  const gchar *mime_type;
  const gchar **applications;
  const gchar **groups;

  gint age;
};

GType                 btk_recent_filter_get_type (void) G_GNUC_CONST;

BtkRecentFilter *     btk_recent_filter_new      (void);
void                  btk_recent_filter_set_name (BtkRecentFilter *filter,
						  const gchar     *name);
const gchar *         btk_recent_filter_get_name (BtkRecentFilter *filter);

void btk_recent_filter_add_mime_type      (BtkRecentFilter      *filter,
					   const gchar          *mime_type);
void btk_recent_filter_add_pattern        (BtkRecentFilter      *filter,
					   const gchar          *pattern);
void btk_recent_filter_add_pixbuf_formats (BtkRecentFilter      *filter);
void btk_recent_filter_add_application    (BtkRecentFilter      *filter,
					   const gchar          *application);
void btk_recent_filter_add_group          (BtkRecentFilter      *filter,
					   const gchar          *group);
void btk_recent_filter_add_age            (BtkRecentFilter      *filter,
					   gint                  days);
void btk_recent_filter_add_custom         (BtkRecentFilter      *filter,
					   BtkRecentFilterFlags  needed,
					   BtkRecentFilterFunc   func,
					   gpointer              data,
					   GDestroyNotify        data_destroy);

BtkRecentFilterFlags btk_recent_filter_get_needed (BtkRecentFilter           *filter);
gboolean             btk_recent_filter_filter     (BtkRecentFilter           *filter,
						   const BtkRecentFilterInfo *filter_info);

G_END_DECLS

#endif /* ! __BTK_RECENT_FILTER_H__ */
