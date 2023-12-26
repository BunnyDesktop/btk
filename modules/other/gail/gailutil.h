/* BAIL - The GNOME Accessibility Implementation Library
 * Copyright 2001 Sun Microsystems Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __BAIL_UTIL_H__
#define __BAIL_UTIL_H__

#include <batk/batk.h>

G_BEGIN_DECLS

#define BAIL_TYPE_UTIL                           (bail_util_get_type ())
#define BAIL_UTIL(obj)                           (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAIL_TYPE_UTIL, BailUtil))
#define BAIL_UTIL_CLASS(klass)                   (G_TYPE_CHECK_CLASS_CAST ((klass), BAIL_TYPE_UTIL, BailUtilClass))
#define BAIL_IS_UTIL(obj)                        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAIL_TYPE_UTIL))
#define BAIL_IS_UTIL_CLASS(klass)                (G_TYPE_CHECK_CLASS_TYPE ((klass), BAIL_TYPE_UTIL))
#define BAIL_UTIL_GET_CLASS(obj)                 (G_TYPE_INSTANCE_GET_CLASS ((obj), BAIL_TYPE_UTIL, BailUtilClass))

typedef struct _BailUtil                  BailUtil;
typedef struct _BailUtilClass             BailUtilClass;
  
struct _BailUtil
{
  BatkUtil parent;
  GList *listener_list;
};

GType bail_util_get_type (void);

struct _BailUtilClass
{
  BatkUtilClass parent_class;
};

#define BAIL_TYPE_MISC                           (bail_misc_get_type ())
#define BAIL_MISC(obj)                           (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAIL_TYPE_MISC, BailMisc))
#define BAIL_MISC_CLASS(klass)                   (G_TYPE_CHECK_CLASS_CAST ((klass), BAIL_TYPE_MISC, BailMiscClass))
#define BAIL_IS_MISC(obj)                        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAIL_TYPE_MISC))
#define BAIL_IS_MISC_CLASS(klass)                (G_TYPE_CHECK_CLASS_TYPE ((klass), BAIL_TYPE_MISC))
#define BAIL_MISC_GET_CLASS(obj)                 (G_TYPE_INSTANCE_GET_CLASS ((obj), BAIL_TYPE_MISC, BailMiscClass))

typedef struct _BailMisc                  BailMisc;
typedef struct _BailMiscClass             BailMiscClass;
  
struct _BailMisc
{
  BatkMisc parent;
};

GType bail_misc_get_type (void);

struct _BailMiscClass
{
  BatkMiscClass parent_class;
};

G_END_DECLS

#endif /* __BAIL_UTIL_H__ */
