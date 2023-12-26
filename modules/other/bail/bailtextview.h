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

#ifndef __BAIL_TEXT_VIEW_H__
#define __BAIL_TEXT_VIEW_H__

#include <bail/bailcontainer.h>
#include <libbail-util/bailtextutil.h>

B_BEGIN_DECLS

#define BAIL_TYPE_TEXT_VIEW                  (bail_text_view_get_type ())
#define BAIL_TEXT_VIEW(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAIL_TYPE_TEXT_VIEW, BailTextView))
#define BAIL_TEXT_VIEW_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), BAIL_TYPE_TEXT_VIEW, BailTextViewClass))
#define BAIL_IS_TEXT_VIEW(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAIL_TYPE_TEXT_VIEW))
#define BAIL_IS_TEXT_VIEW_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), BAIL_TYPE_TEXT_VIEW))
#define BAIL_TEXT_VIEW_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), BAIL_TYPE_TEXT_VIEW, BailTextViewClass))

typedef struct _BailTextView              BailTextView;
typedef struct _BailTextViewClass         BailTextViewClass;

struct _BailTextView
{
  BailContainer  parent;

  BailTextUtil   *textutil;
  gint           previous_insert_offset;
  gint           previous_selection_bound;
  /*
   * These fields store information about text changed
   */
  gchar          *signal_name;
  gint           position;
  gint           length;

  guint          insert_notify_handler;
};

GType bail_text_view_get_type (void);

struct _BailTextViewClass
{
  BailContainerClass parent_class;
};

B_END_DECLS

#endif /* __BAIL_TEXT_VIEW_H__ */
