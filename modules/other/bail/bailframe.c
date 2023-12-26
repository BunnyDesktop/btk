/* BAIL - The BUNNY Accessibility Implementation Library
 * Copyright 2001 Sun Microsystems Inc.
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

#include "config.h"

#include <string.h>
#include <btk/btk.h>
#include "bailframe.h"

static void                  bail_frame_class_init       (BailFrameClass  *klass);
static void                  bail_frame_init             (BailFrame       *frame);
static void                  bail_frame_initialize       (BatkObject       *accessible,
                                                          gpointer         data);
static const gchar*          bail_frame_get_name         (BatkObject       *obj);

G_DEFINE_TYPE (BailFrame, bail_frame, BAIL_TYPE_CONTAINER)

static void
bail_frame_class_init (BailFrameClass *klass)
{
  BatkObjectClass *class = BATK_OBJECT_CLASS (klass);

  class->initialize = bail_frame_initialize;
  class->get_name = bail_frame_get_name;
}

static void
bail_frame_init (BailFrame       *frame)
{
}

static void
bail_frame_initialize (BatkObject *accessible,
                       gpointer  data)
{
  BATK_OBJECT_CLASS (bail_frame_parent_class)->initialize (accessible, data);

  accessible->role = BATK_ROLE_PANEL;
}

static const gchar*
bail_frame_get_name (BatkObject *obj)
{
  const gchar *name;
  g_return_val_if_fail (BAIL_IS_FRAME (obj), NULL);

  name = BATK_OBJECT_CLASS (bail_frame_parent_class)->get_name (obj);
  if (name != NULL)
  {
    return name;
  }
  else
  {
    /*
     * Get the text on the label
     */
    BtkWidget *widget;

    widget = BTK_ACCESSIBLE (obj)->widget;
    if (widget == NULL)
    {
      /*
       * State is defunct
       */
      return NULL;
    }
    return btk_frame_get_label (BTK_FRAME (widget));
  }
}
