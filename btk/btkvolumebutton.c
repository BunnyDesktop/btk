/* BTK - The GIMP Toolkit
 * Copyright (C) 2007 Red Hat, Inc.
 *
 * Authors:
 * - Bastien Nocera <bnocera@redhat.com>
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
 * Modified by the BTK+ Team and others 2007.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/.
 */

#include "config.h"

#include "btkvolumebutton.h"
#include "btkstock.h"
#include "btktooltip.h"
#include "btkintl.h"

#include "btkalias.h"

/**
 * SECTION:btkvolumebutton
 * @Short_description: A button which pops up a volume control
 * @Title: BtkVolumeButton
 *
 * #BtkVolumeButton is a subclass of #BtkScaleButton that has
 * been tailored for use as a volume control widget with suitable
 * icons, tooltips and accessible labels.
 */

#define EPSILON (1e-10)


static gboolean	cb_query_tooltip (BtkWidget       *button,
                                  gint             x,
                                  gint             y,
                                  gboolean         keyboard_mode,
                                  BtkTooltip      *tooltip,
                                  gpointer         user_data);
static void	cb_value_changed (BtkVolumeButton *button,
                                  gdouble          value,
                                  gpointer         user_data);

G_DEFINE_TYPE (BtkVolumeButton, btk_volume_button, BTK_TYPE_SCALE_BUTTON)

static void
btk_volume_button_class_init (BtkVolumeButtonClass *klass)
{
}

static void
btk_volume_button_init (BtkVolumeButton *button)
{
  BtkScaleButton *sbutton = BTK_SCALE_BUTTON (button);
  BtkObject *adj;
  const char *icons[] = {
	"audio-volume-muted",
	"audio-volume-high",
	"audio-volume-low",
	"audio-volume-medium",
	NULL
  };

  batk_object_set_name (btk_widget_get_accessible (BTK_WIDGET (button)),
		       _("Volume"));
  batk_object_set_description (btk_widget_get_accessible (BTK_WIDGET (button)),
		       _("Turns volume down or up"));
  batk_action_set_description (BATK_ACTION (btk_widget_get_accessible (BTK_WIDGET (button))),
			      1,
			      _("Adjusts the volume"));

  batk_object_set_name (btk_widget_get_accessible (sbutton->minus_button),
		       _("Volume Down"));
  batk_object_set_description (btk_widget_get_accessible (sbutton->minus_button),
		       _("Decreases the volume"));
  btk_widget_set_tooltip_text (sbutton->minus_button, _("Volume Down"));

  batk_object_set_name (btk_widget_get_accessible (sbutton->plus_button),
		       _("Volume Up"));
  batk_object_set_description (btk_widget_get_accessible (sbutton->plus_button),
		       _("Increases the volume"));
  btk_widget_set_tooltip_text (sbutton->plus_button, _("Volume Up"));

  btk_scale_button_set_icons (sbutton, icons);

  adj = btk_adjustment_new (0., 0., 1., 0.02, 0.2, 0.);
  g_object_set (B_OBJECT (button),
		"adjustment", adj,
		"size", BTK_ICON_SIZE_SMALL_TOOLBAR,
		"has-tooltip", TRUE,
	       	NULL);

  g_signal_connect (B_OBJECT (button), "query-tooltip",
		    G_CALLBACK (cb_query_tooltip), NULL);
  g_signal_connect (B_OBJECT (button), "value-changed",
		    G_CALLBACK (cb_value_changed), NULL);
}

/**
 * btk_volume_button_new
 *
 * Creates a #BtkVolumeButton, with a range between 0.0 and 1.0, with
 * a stepping of 0.02. Volume values can be obtained and modified using
 * the functions from #BtkScaleButton.
 *
 * Return value: a new #BtkVolumeButton
 *
 * Since: 2.12
 */
BtkWidget *
btk_volume_button_new (void)
{
  BObject *button;
  button = g_object_new (BTK_TYPE_VOLUME_BUTTON, NULL);
  return BTK_WIDGET (button);
}

static gboolean
cb_query_tooltip (BtkWidget  *button,
		  gint        x,
		  gint        y,
		  gboolean    keyboard_mode,
		  BtkTooltip *tooltip,
		  gpointer    user_data)
{
  BtkScaleButton *scale_button = BTK_SCALE_BUTTON (button);
  BtkAdjustment *adj;
  gdouble val;
  char *str;
  BatkImage *image;

  image = BATK_IMAGE (btk_widget_get_accessible (button));

  adj = btk_scale_button_get_adjustment (scale_button);
  val = btk_scale_button_get_value (scale_button);

  if (val < (adj->lower + EPSILON))
    {
      str = g_strdup (_("Muted"));
    }
  else if (val >= (adj->upper - EPSILON))
    {
      str = g_strdup (_("Full Volume"));
    }
  else
    {
      int percent;

      percent = (int) (100. * val / (adj->upper - adj->lower) + .5);

      /* Translators: this is the percentage of the current volume,
       * as used in the tooltip, eg. "49 %".
       * Translate the "%d" to "%Id" if you want to use localised digits,
       * or otherwise translate the "%d" to "%d".
       */
      str = g_strdup_printf (C_("volume percentage", "%d %%"), percent);
    }

  btk_tooltip_set_text (tooltip, str);
  batk_image_set_image_description (image, str);
  g_free (str);

  return TRUE;
}

static void
cb_value_changed (BtkVolumeButton *button, gdouble value, gpointer user_data)
{
  btk_widget_trigger_tooltip_query (BTK_WIDGET (button));
}

#define __BTK_VOLUME_BUTTON_C__
#include "btkaliasdef.c"
