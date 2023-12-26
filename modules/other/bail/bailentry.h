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

#ifndef __BAIL_ENTRY_H__
#define __BAIL_ENTRY_H__

#include <bail/bailwidget.h>
#include <libbail-util/bailtextutil.h>

B_BEGIN_DECLS

#define BAIL_TYPE_ENTRY                      (bail_entry_get_type ())
#define BAIL_ENTRY(obj)                      (B_TYPE_CHECK_INSTANCE_CAST ((obj), BAIL_TYPE_ENTRY, BailEntry))
#define BAIL_ENTRY_CLASS(klass)              (B_TYPE_CHECK_CLASS_CAST ((klass), BAIL_TYPE_ENTRY, BailEntryClass))
#define BAIL_IS_ENTRY(obj)                   (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BAIL_TYPE_ENTRY))
#define BAIL_IS_ENTRY_CLASS(klass)           (B_TYPE_CHECK_CLASS_TYPE ((klass), BAIL_TYPE_ENTRY))
#define BAIL_ENTRY_GET_CLASS(obj)            (B_TYPE_INSTANCE_GET_CLASS ((obj), BAIL_TYPE_ENTRY, BailEntryClass))

typedef struct _BailEntry              BailEntry;
typedef struct _BailEntryClass         BailEntryClass;

struct _BailEntry
{
  BailWidget parent;

  BailTextUtil *textutil;
  /*
   * These fields store information about text changed
   */
  gchar          *signal_name_insert;
  gchar          *signal_name_delete;
  gint           position_insert;
  gint           position_delete;
  gint           length_insert;
  gint           length_delete;
  gint           cursor_position;
  gint           selection_bound;

  gchar          *activate_description;
  gchar          *activate_keybinding;
  guint          action_idle_handler;
  guint          insert_idle_handler;
};

GType bail_entry_get_type (void);

struct _BailEntryClass
{
  BailWidgetClass parent_class;
};

B_END_DECLS

#endif /* __BAIL_ENTRY_H__ */
