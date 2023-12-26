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

#include <btk/btk.h>
#include "bailcalendar.h"

static void         bail_calendar_class_init          (BailCalendarClass *klass);
static void         bail_calendar_init                (BailCalendar      *calendar);
static void         bail_calendar_initialize          (BatkObject         *accessible,
                                                       gpointer           data);

G_DEFINE_TYPE (BailCalendar, bail_calendar, BAIL_TYPE_WIDGET)

static void
bail_calendar_class_init (BailCalendarClass *klass)
{
  BatkObjectClass *batk_object_class = BATK_OBJECT_CLASS (klass);

  batk_object_class->initialize = bail_calendar_initialize;
}

static void
bail_calendar_init (BailCalendar *calendar)
{
}

static void
bail_calendar_initialize (BatkObject *accessible,
                          gpointer  data)
{
  BATK_OBJECT_CLASS (bail_calendar_parent_class)->initialize (accessible, data);

  accessible->role = BATK_ROLE_CALENDAR;
}
