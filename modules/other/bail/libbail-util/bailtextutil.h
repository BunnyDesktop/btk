/* BAIL - The BUNNY Accessibility Implementation Library
 * Copyright 2001 Sun Microsystems Inc.
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

#ifndef __BAIL_TEXT_UTIL_H__
#define __BAIL_TEXT_UTIL_H__

#include <bunnylib-object.h>
#include <btk/btk.h>

B_BEGIN_DECLS

#define BAIL_TYPE_TEXT_UTIL                  (bail_text_util_get_type ())
#define BAIL_TEXT_UTIL(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAIL_TYPE_TEXT_UTIL, BailTextUtil))
#define BAIL_TEXT_UTIL_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), BAIL_TYPE_TEXT_UTIL, BailTextUtilClass))
#define BAIL_IS_TEXT_UTIL(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAIL_TYPE_TEXT_UTIL))
#define BAIL_IS_TEXT_UTIL_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), BAIL_TYPE_TEXT_UTIL))
#define BAIL_TEXT_UTIL_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), BAIL_TYPE_TEXT_UTIL, BailTextUtilClass))

/**
 *BailOffsetType:
 *@BAIL_BEFORE_OFFSET: Text before offset is required.
 *@BAIL_AT_OFFSET: Text at offset is required,
 *@BAIL_AFTER_OFFSET: Text after offset is required.
 *
 * Specifies which of the functions batk_text_get_text_before_offset(),
 * batk_text_get_text_at_offset(), batk_text_get_text_after_offset() the
 * function bail_text_util_get_text() is being called for.
 **/
typedef enum
{
  BAIL_BEFORE_OFFSET,
  BAIL_AT_OFFSET,
  BAIL_AFTER_OFFSET
}BailOffsetType;

typedef struct _BailTextUtil		BailTextUtil;
typedef struct _BailTextUtilClass	BailTextUtilClass;

struct _BailTextUtil
{
  GObject parent;

  BtkTextBuffer *buffer;
};

struct _BailTextUtilClass
{
  GObjectClass parent_class;
};

GType         bail_text_util_get_type      (void);
BailTextUtil* bail_text_util_new           (void);

void          bail_text_util_text_setup    (BailTextUtil    *textutil,
                                            const gchar     *text);
void          bail_text_util_buffer_setup  (BailTextUtil    *textutil,
                                            BtkTextBuffer   *buffer);
gchar*        bail_text_util_get_text      (BailTextUtil    *textutil,
                                             gpointer        layout,
                                            BailOffsetType  function,
                                            BatkTextBoundary boundary_type,
                                            gint            offset,
                                            gint            *start_offset,
                                            gint            *end_offset);
gchar*        bail_text_util_get_substring (BailTextUtil    *textutil,
                                            gint            start_pos,
                                            gint            end_pos);

B_END_DECLS

#endif /*__BAIL_TEXT_UTIL_H__ */
