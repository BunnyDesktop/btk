/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#undef BTK_DISABLE_DEPRECATED

#include "btklabel.h"
#include "btkmain.h"
#include "btkmenuitem.h"
#include "btkprivate.h"
#include "btkwidget.h"
#include "btkwindow.h"
#include "btkstyle.h"
#include "btktooltips.h"
#include "btkintl.h"
#include "btkalias.h"


#define DEFAULT_DELAY 500           /* Default delay in ms */
#define STICKY_DELAY 0              /* Delay before popping up next tip
                                     * if we're sticky
                                     */
#define STICKY_REVERT_DELAY 1000    /* Delay before sticky tooltips revert
				     * to normal
                                     */
#define BTK_TOOLTIPS_GET_PRIVATE(obj) (B_TYPE_INSTANCE_GET_PRIVATE ((obj), BTK_TYPE_TOOLTIPS, BtkTooltipsPrivate))

typedef struct _BtkTooltipsPrivate BtkTooltipsPrivate;

struct _BtkTooltipsPrivate
{
  GHashTable *tips_data_table;
};


static void btk_tooltips_finalize          (BObject         *object);
static void btk_tooltips_destroy           (BtkObject       *object);

static void btk_tooltips_destroy_data      (BtkTooltipsData *tooltipsdata);

static void btk_tooltips_widget_remove     (BtkWidget       *widget,
                                            gpointer         data);

static const gchar  tooltips_data_key[] = "_BtkTooltipsData";
static const gchar  tooltips_info_key[] = "_BtkTooltipsInfo";

G_DEFINE_TYPE (BtkTooltips, btk_tooltips, BTK_TYPE_OBJECT)

static void
btk_tooltips_class_init (BtkTooltipsClass *class)
{
  BtkObjectClass *object_class = (BtkObjectClass *) class;
  BObjectClass *bobject_class = (BObjectClass *) class;

  bobject_class->finalize = btk_tooltips_finalize;

  object_class->destroy = btk_tooltips_destroy;

  g_type_class_add_private (bobject_class, sizeof (BtkTooltipsPrivate));
}

static void
btk_tooltips_init (BtkTooltips *tooltips)
{
  BtkTooltipsPrivate *private = BTK_TOOLTIPS_GET_PRIVATE (tooltips);

  tooltips->tip_window = NULL;
  tooltips->active_tips_data = NULL;
  tooltips->tips_data_list = NULL;

  tooltips->delay = DEFAULT_DELAY;
  tooltips->enabled = TRUE;
  tooltips->timer_tag = 0;
  tooltips->use_sticky_delay = FALSE;
  tooltips->last_popdown.tv_sec = -1;
  tooltips->last_popdown.tv_usec = -1;

  private->tips_data_table =
    g_hash_table_new_full (NULL, NULL, NULL,
                           (GDestroyNotify) btk_tooltips_destroy_data);

  btk_tooltips_force_window (tooltips);
}

static void
btk_tooltips_finalize (BObject *object)
{
  BtkTooltipsPrivate *private = BTK_TOOLTIPS_GET_PRIVATE (object);

  g_hash_table_destroy (private->tips_data_table);

  B_OBJECT_CLASS (btk_tooltips_parent_class)->finalize (object);
}

BtkTooltips *
btk_tooltips_new (void)
{
  return g_object_new (BTK_TYPE_TOOLTIPS, NULL);
}

static void
btk_tooltips_destroy_data (BtkTooltipsData *tooltipsdata)
{
  g_free (tooltipsdata->tip_text);
  g_free (tooltipsdata->tip_private);

  g_signal_handlers_disconnect_by_func (tooltipsdata->widget,
					btk_tooltips_widget_remove,
					tooltipsdata);

  g_object_set_data (B_OBJECT (tooltipsdata->widget), I_(tooltips_data_key), NULL);
  g_object_unref (tooltipsdata->widget);
  g_free (tooltipsdata);
}

static void
btk_tooltips_destroy (BtkObject *object)
{
  BtkTooltips *tooltips = BTK_TOOLTIPS (object);
  BtkTooltipsPrivate *private = BTK_TOOLTIPS_GET_PRIVATE (tooltips);

  if (tooltips->tip_window)
    {
      btk_widget_destroy (tooltips->tip_window);
      tooltips->tip_window = NULL;
    }

  g_hash_table_remove_all (private->tips_data_table);

  BTK_OBJECT_CLASS (btk_tooltips_parent_class)->destroy (object);
}

void
btk_tooltips_force_window (BtkTooltips *tooltips)
{
  g_return_if_fail (BTK_IS_TOOLTIPS (tooltips));

  if (!tooltips->tip_window)
    {
      tooltips->tip_window = btk_window_new (BTK_WINDOW_POPUP);
      g_signal_connect (tooltips->tip_window,
			"destroy",
			G_CALLBACK (btk_widget_destroyed),
			&tooltips->tip_window);

      tooltips->tip_label = btk_label_new (NULL);
      btk_container_add (BTK_CONTAINER (tooltips->tip_window),
			 tooltips->tip_label);
    }
}

void
btk_tooltips_enable (BtkTooltips *tooltips)
{
  g_return_if_fail (tooltips != NULL);

  tooltips->enabled = TRUE;
}

void
btk_tooltips_disable (BtkTooltips *tooltips)
{
  g_return_if_fail (tooltips != NULL);

  tooltips->enabled = FALSE;
}

void
btk_tooltips_set_delay (BtkTooltips *tooltips,
                        guint         delay)
{
  g_return_if_fail (tooltips != NULL);

  tooltips->delay = delay;
}

BtkTooltipsData*
btk_tooltips_data_get (BtkWidget       *widget)
{
  g_return_val_if_fail (widget != NULL, NULL);

  return g_object_get_data (B_OBJECT (widget), tooltips_data_key);
}


/**
 * btk_tooltips_set_tip:
 * @tooltips: a #BtkTooltips.
 * @widget: the #BtkWidget you wish to associate the tip with.
 * @tip_text: (allow-none): a string containing the tip itself.
 * @tip_private: (allow-none): a string of any further information that may be useful if the user gets stuck.
 *
 * Adds a tooltip containing the message @tip_text to the specified #BtkWidget.
 * Deprecated: 2.12:
 */
void
btk_tooltips_set_tip (BtkTooltips *tooltips,
		      BtkWidget   *widget,
		      const gchar *tip_text,
		      const gchar *tip_private)
{
  BtkTooltipsData *tooltipsdata;

  g_return_if_fail (BTK_IS_TOOLTIPS (tooltips));
  g_return_if_fail (widget != NULL);

  tooltipsdata = btk_tooltips_data_get (widget);

  if (!tip_text)
    {
      if (tooltipsdata)
	btk_tooltips_widget_remove (tooltipsdata->widget, tooltipsdata);
      return;
    }
  
  if (tooltips->active_tips_data 
      && tooltipsdata
      && tooltips->active_tips_data->widget == widget
      && BTK_WIDGET_DRAWABLE (tooltips->active_tips_data->widget))
    {
      g_free (tooltipsdata->tip_text);
      g_free (tooltipsdata->tip_private);

      tooltipsdata->tip_text = g_strdup (tip_text);
      tooltipsdata->tip_private = g_strdup (tip_private);
    }
  else 
    {
      g_object_ref (widget);
      
      if (tooltipsdata)
        btk_tooltips_widget_remove (tooltipsdata->widget, tooltipsdata);
      
      tooltipsdata = g_new0 (BtkTooltipsData, 1);
      
      tooltipsdata->tooltips = tooltips;
      tooltipsdata->widget = widget;

      tooltipsdata->tip_text = g_strdup (tip_text);
      tooltipsdata->tip_private = g_strdup (tip_private);

      g_hash_table_insert (BTK_TOOLTIPS_GET_PRIVATE (tooltips)->tips_data_table,
                           widget, tooltipsdata);

      g_object_set_data (B_OBJECT (widget), I_(tooltips_data_key),
                         tooltipsdata);

      g_signal_connect (widget, "destroy",
                        G_CALLBACK (btk_tooltips_widget_remove),
			tooltipsdata);
    }

  btk_widget_set_tooltip_text (widget, tip_text);
}

static void
btk_tooltips_widget_remove (BtkWidget *widget,
			    gpointer   data)
{
  BtkTooltipsData *tooltipsdata = (BtkTooltipsData*) data;
  BtkTooltips *tooltips = tooltipsdata->tooltips;
  BtkTooltipsPrivate *private = BTK_TOOLTIPS_GET_PRIVATE (tooltips);

  g_hash_table_remove (private->tips_data_table, tooltipsdata->widget);
}

/**
 * btk_tooltips_get_info_from_tip_window:
 * @tip_window: a #BtkWindow 
 * @tooltips: the return location for the tooltips which are displayed 
 *    in @tip_window, or %NULL
 * @current_widget: the return location for the widget whose tooltips 
 *    are displayed, or %NULL
 * 
 * Determines the tooltips and the widget they belong to from the window in 
 * which they are displayed. 
 *
 * This function is mostly intended for use by accessibility technologies;
 * applications should have little use for it.
 * 
 * Return value: %TRUE if @tip_window is displaying tooltips, otherwise %FALSE.
 *
 * Since: 2.4
 *
 * Deprecated: 2.12:
 **/
gboolean
btk_tooltips_get_info_from_tip_window (BtkWindow    *tip_window,
                                       BtkTooltips **tooltips,
                                       BtkWidget   **current_widget)
{
  BtkTooltips  *current_tooltips;  
  gboolean has_tips;

  g_return_val_if_fail (BTK_IS_WINDOW (tip_window), FALSE);

  current_tooltips = g_object_get_data (B_OBJECT (tip_window), tooltips_info_key);

  has_tips = current_tooltips != NULL;

  if (tooltips)
    *tooltips = current_tooltips;
  if (current_widget)
    *current_widget = (has_tips && current_tooltips->active_tips_data) ? current_tooltips->active_tips_data->widget : NULL;

  return has_tips;
}

#define __BTK_TOOLTIPS_C__
#include "btkaliasdef.c"
