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

#ifndef __BAIL_BUTTON_H__
#define __BAIL_BUTTON_H__

#include <bail/bailcontainer.h>
#include <libbail-util/bailtextutil.h>

G_BEGIN_DECLS

#define BAIL_TYPE_BUTTON                     (bail_button_get_type ())
#define BAIL_BUTTON(obj)                     (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAIL_TYPE_BUTTON, BailButton))
#define BAIL_BUTTON_CLASS(klass)             (G_TYPE_CHECK_CLASS_CAST ((klass), BAIL_TYPE_BUTTON, BailButtonClass))
#define BAIL_IS_BUTTON(obj)                  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAIL_TYPE_BUTTON))
#define BAIL_IS_BUTTON_CLASS(klass)          (G_TYPE_CHECK_CLASS_TYPE ((klass), BAIL_TYPE_BUTTON))
#define BAIL_BUTTON_GET_CLASS(obj)           (G_TYPE_INSTANCE_GET_CLASS ((obj), BAIL_TYPE_BUTTON, BailButtonClass))

typedef struct _BailButton                   BailButton;
typedef struct _BailButtonClass              BailButtonClass;

struct _BailButton
{
  BailContainer parent;

  /*
   * Cache the widget state so we know the previous state when it changed
   */
  gint8         state;

  gchar         *click_description;
  gchar         *press_description;
  gchar         *release_description;
  gchar         *click_keybinding;
  guint         action_idle_handler;
  GQueue        *action_queue;

  BailTextUtil	 *textutil;

  gboolean     default_is_press;
};

GType bail_button_get_type (void);

struct _BailButtonClass
{
  BailContainerClass parent_class;
};

G_END_DECLS

#endif /* __BAIL_BUTTON_H__ */
