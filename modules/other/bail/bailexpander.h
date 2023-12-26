/* BAIL - The BUNNY Accessibility Implementation Library
 * Copyright 2003 Sun Microsystems Inc.
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

#ifndef __BAIL_EXPANDER_H__
#define __BAIL_EXPANDER_H__

#include <bail/bailcontainer.h>
#include <libbail-util/bailtextutil.h>

B_BEGIN_DECLS

#define BAIL_TYPE_EXPANDER              (bail_expander_get_type ())
#define BAIL_EXPANDER(obj)              (B_TYPE_CHECK_INSTANCE_CAST ((obj), BAIL_TYPE_EXPANDER, BailExpander))
#define BAIL_EXPANDER_CLASS(klass)      (B_TYPE_CHECK_CLASS_CAST ((klass), BAIL_TYPE_EXPANDER, BailExpanderClass))
#define BAIL_IS_EXPANDER(obj)           (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BAIL_TYPE_EXPANDER))
#define BAIL_IS_EXPANDER_CLASS(klass)   (B_TYPE_CHECK_CLASS_TYPE ((klass), BAIL_TYPE_EXPANDER))
#define BAIL_EXPANDER_GET_CLASS(obj)    (B_TYPE_INSTANCE_GET_CLASS ((obj), BAIL_TYPE_EXPANDER, BailExpanderClass))

typedef struct _BailExpander              BailExpander;
typedef struct _BailExpanderClass         BailExpanderClass;

struct _BailExpander
{
  BailContainer parent;

  bchar         *activate_description;
  bchar         *activate_keybinding;
  buint         action_idle_handler;

  BailTextUtil   *textutil;
};

GType bail_expander_get_type (void);

struct _BailExpanderClass
{
  BailContainerClass parent_class;
};

B_END_DECLS

#endif /* __BAIL_EXPANDER_H__ */
