/* BAIL - The BUNNY Accessibility Enabling Library
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

#include <btk/btk.h>
#include "bailseparator.h"

static void         bail_separator_class_init            (BailSeparatorClass  *klass);
static void         bail_separator_init                  (BailSeparator       *accessible);
static void         bail_separator_initialize            (BatkObject           *accessible,
                                                          bpointer             data);
static BatkStateSet* bail_separator_ref_state_set	 (BatkObject	      *accessible);

G_DEFINE_TYPE (BailSeparator, bail_separator, BAIL_TYPE_WIDGET)

static void
bail_separator_class_init (BailSeparatorClass *klass)
{
  BatkObjectClass  *class = BATK_OBJECT_CLASS (klass);

  class->initialize = bail_separator_initialize;
  class->ref_state_set = bail_separator_ref_state_set;
}

static void
bail_separator_init (BailSeparator *accessible)
{
}

static void
bail_separator_initialize (BatkObject *accessible,
                           bpointer  data)
{
  BATK_OBJECT_CLASS (bail_separator_parent_class)->initialize (accessible, data);

  accessible->role = BATK_ROLE_SEPARATOR;
}

static BatkStateSet*
bail_separator_ref_state_set (BatkObject *accessible)
{
  BatkStateSet *state_set;
  BtkWidget *widget;

  state_set = BATK_OBJECT_CLASS (bail_separator_parent_class)->ref_state_set (accessible);
  widget = BTK_ACCESSIBLE (accessible)->widget;

  if (widget == NULL)
    return state_set;

  if (BTK_IS_VSEPARATOR (widget))
    batk_state_set_add_state (state_set, BATK_STATE_VERTICAL);
  else if (BTK_IS_HSEPARATOR (widget))
    batk_state_set_add_state (state_set, BATK_STATE_HORIZONTAL);

  return state_set;
}
