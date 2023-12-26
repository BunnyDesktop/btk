/* BAIL - The BUNNY Accessibility Implementation Library
 * Copyright 2001, 2002, 2003 Sun Microsystems Inc.
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

#undef BTK_DISABLE_DEPRECATED

#include <btk/btk.h>

#include "bailprogressbar.h"
#include "bailadjustment.h"

static void	 bail_progress_bar_class_init        (BailProgressBarClass *klass);
static void	 bail_progress_bar_init              (BailProgressBar *bar);
static void      bail_progress_bar_real_initialize   (BatkObject      *obj,
                                                      gpointer       data);
static void      bail_progress_bar_finalize          (GObject        *object);


static void	 batk_value_interface_init            (BatkValueIface  *iface);


static void      bail_progress_bar_real_notify_btk   (GObject        *obj,
                                                      GParamSpec     *pspec);

static void	 bail_progress_bar_get_current_value (BatkValue       *obj,
                                           	      GValue         *value);
static void	 bail_progress_bar_get_maximum_value (BatkValue       *obj,
                                            	      GValue         *value);
static void	 bail_progress_bar_get_minimum_value (BatkValue       *obj,
                                           	      GValue         *value);
static void      bail_progress_bar_value_changed     (BtkAdjustment  *adjustment,
                                                      gpointer       data);

G_DEFINE_TYPE_WITH_CODE (BailProgressBar, bail_progress_bar, BAIL_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_VALUE, batk_value_interface_init))

static void	 
bail_progress_bar_class_init		(BailProgressBarClass *klass)
{
  GObjectClass *bobject_class = G_OBJECT_CLASS (klass);
  BatkObjectClass *class = BATK_OBJECT_CLASS (klass);
  BailWidgetClass *widget_class;

  widget_class = (BailWidgetClass*)klass;

  widget_class->notify_btk = bail_progress_bar_real_notify_btk;

  class->initialize = bail_progress_bar_real_initialize;

  bobject_class->finalize = bail_progress_bar_finalize;
}

static void
bail_progress_bar_init (BailProgressBar *bar)
{
}

static void
bail_progress_bar_real_initialize (BatkObject *obj,
                                   gpointer  data)
{
  BailProgressBar *progress_bar = BAIL_PROGRESS_BAR (obj);
  BtkProgress *btk_progress;

  BATK_OBJECT_CLASS (bail_progress_bar_parent_class)->initialize (obj, data);

  btk_progress = BTK_PROGRESS (data);
  /*
   * If a BtkAdjustment already exists for the spin_button,
   * create the BailAdjustment
   */
  if (btk_progress->adjustment)
    {
      progress_bar->adjustment = bail_adjustment_new (btk_progress->adjustment);
      g_signal_connect (btk_progress->adjustment,
                        "value-changed",
                        G_CALLBACK (bail_progress_bar_value_changed),
                        obj);
    }
  else
    progress_bar->adjustment = NULL;

  obj->role = BATK_ROLE_PROGRESS_BAR;
}

static void	 
batk_value_interface_init (BatkValueIface *iface)
{
  iface->get_current_value = bail_progress_bar_get_current_value;
  iface->get_maximum_value = bail_progress_bar_get_maximum_value;
  iface->get_minimum_value = bail_progress_bar_get_minimum_value;
}

static void	 
bail_progress_bar_get_current_value (BatkValue   *obj,
                                     GValue     *value)
{
  BailProgressBar *progress_bar;

  g_return_if_fail (BAIL_IS_PROGRESS_BAR (obj));

  progress_bar = BAIL_PROGRESS_BAR (obj);
  if (progress_bar->adjustment == NULL)
    /*
     * Adjustment has not been specified
     */
    return;

  batk_value_get_current_value (BATK_VALUE (progress_bar->adjustment), value);
}

static void	 
bail_progress_bar_get_maximum_value (BatkValue   *obj,
                                     GValue     *value)
{
  BailProgressBar *progress_bar;

  g_return_if_fail (BAIL_IS_PROGRESS_BAR (obj));

  progress_bar = BAIL_PROGRESS_BAR (obj);
  if (progress_bar->adjustment == NULL)
    /*
     * Adjustment has not been specified
     */
    return;

  batk_value_get_maximum_value (BATK_VALUE (progress_bar->adjustment), value);
}

static void	 
bail_progress_bar_get_minimum_value (BatkValue    *obj,
                              	     GValue      *value)
{
  BailProgressBar *progress_bar;

  g_return_if_fail (BAIL_IS_PROGRESS_BAR (obj));

  progress_bar = BAIL_PROGRESS_BAR (obj);
  if (progress_bar->adjustment == NULL)
    /*
     * Adjustment has not been specified
     */
    return;

  batk_value_get_minimum_value (BATK_VALUE (progress_bar->adjustment), value);
}

static void
bail_progress_bar_finalize (GObject            *object)
{
  BailProgressBar *progress_bar = BAIL_PROGRESS_BAR (object);

  if (progress_bar->adjustment)
    {
      g_object_unref (progress_bar->adjustment);
      progress_bar->adjustment = NULL;
    }

  G_OBJECT_CLASS (bail_progress_bar_parent_class)->finalize (object);
}


static void
bail_progress_bar_real_notify_btk (GObject           *obj,
                                   GParamSpec        *pspec)
{
  BtkWidget *widget = BTK_WIDGET (obj);
  BailProgressBar *progress_bar = BAIL_PROGRESS_BAR (btk_widget_get_accessible (widget));

  if (strcmp (pspec->name, "adjustment") == 0)
    {
      /*
       * Get rid of the BailAdjustment for the BtkAdjustment
       * which was associated with the progress_bar.
       */
      if (progress_bar->adjustment)
        {
          g_object_unref (progress_bar->adjustment);
          progress_bar->adjustment = NULL;
        }
      /*
       * Create the BailAdjustment when notify for "adjustment" property
       * is received
       */
      progress_bar->adjustment = bail_adjustment_new (BTK_PROGRESS (widget)->adjustment);
      g_signal_connect (BTK_PROGRESS (widget)->adjustment,
                        "value-changed",
                        G_CALLBACK (bail_progress_bar_value_changed),
                        progress_bar);
    }
  else
    BAIL_WIDGET_CLASS (bail_progress_bar_parent_class)->notify_btk (obj, pspec);
}

static void
bail_progress_bar_value_changed (BtkAdjustment    *adjustment,
                                 gpointer         data)
{
  BailProgressBar *progress_bar;

  g_return_if_fail (adjustment != NULL);
  g_return_if_fail (data != NULL);

  progress_bar = BAIL_PROGRESS_BAR (data);

  g_object_notify (G_OBJECT (progress_bar), "accessible-value");
}
