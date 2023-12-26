/* BDK - The GIMP Drawing Kit
 * Copyright (C) 2000 Red Hat, Inc.
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

#include "bdkscreen.h"
#include "bdkdisplay.h"
#include "bdkdisplaymanager.h"

#include "bdkinternals.h"
#include "bdkmarshalers.h"

#include "bdkintl.h"

#include "bdkalias.h"

struct _BdkDisplayManager
{
  GObject parent_instance;
};

enum {
  PROP_0,

  PROP_DEFAULT_DISPLAY
};

enum {
  DISPLAY_OPENED,
  LAST_SIGNAL
};

static void bdk_display_manager_class_init   (BdkDisplayManagerClass *klass);
static void bdk_display_manager_set_property (GObject                *object,
					      guint                   prop_id,
					      const GValue           *value,
					      GParamSpec             *pspec);
static void bdk_display_manager_get_property (GObject                *object,
					      guint                   prop_id,
					      GValue                 *value,
					      GParamSpec             *pspec);

static guint signals[LAST_SIGNAL] = { 0 };

static BdkDisplay *default_display = NULL;

G_DEFINE_TYPE (BdkDisplayManager, bdk_display_manager, G_TYPE_OBJECT)

static void
bdk_display_manager_class_init (BdkDisplayManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = bdk_display_manager_set_property;
  object_class->get_property = bdk_display_manager_get_property;

  /**
   * BdkDisplayManager::display-opened:
   * @display_manager: the object on which the signal is emitted
   * @display: the opened display
   *
   * The ::display_opened signal is emitted when a display is opened.
   *
   * Since: 2.2
   */
  signals[DISPLAY_OPENED] =
    g_signal_new (g_intern_static_string ("display-opened"),
		  G_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BdkDisplayManagerClass, display_opened),
		  NULL, NULL,
		  _bdk_marshal_VOID__OBJECT,
		  G_TYPE_NONE,
		  1,
		  BDK_TYPE_DISPLAY);

  g_object_class_install_property (object_class,
				   PROP_DEFAULT_DISPLAY,
				   g_param_spec_object ("default-display",
 							P_("Default Display"),
 							P_("The default display for BDK"),
							BDK_TYPE_DISPLAY,
 							G_PARAM_READWRITE|G_PARAM_STATIC_NAME|
							G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB));
}

static void
bdk_display_manager_init (BdkDisplayManager *manager)
{
}

static void
bdk_display_manager_set_property (GObject      *object,
				  guint         prop_id,
				  const GValue *value,
				  GParamSpec   *pspec)
{
  switch (prop_id)
    {
    case PROP_DEFAULT_DISPLAY:
      bdk_display_manager_set_default_display (BDK_DISPLAY_MANAGER (object),
					       g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bdk_display_manager_get_property (GObject      *object,
				  guint         prop_id,
				  GValue       *value,
				  GParamSpec   *pspec)
{
  switch (prop_id)
    {
    case PROP_DEFAULT_DISPLAY:
      g_value_set_object (value, default_display);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/**
 * bdk_display_manager_get:
 *
 * Gets the singleton #BdkDisplayManager object.
 *
 * Returns: (transfer none): The global #BdkDisplayManager singleton; bdk_parse_pargs(),
 * bdk_init(), or bdk_init_check() must have been called first.
 *
 * Since: 2.2
 **/
BdkDisplayManager*
bdk_display_manager_get (void)
{
  static BdkDisplayManager *display_manager = NULL;

  if (!display_manager)
    display_manager = g_object_new (BDK_TYPE_DISPLAY_MANAGER, NULL);

  return display_manager;
}

/**
 * bdk_display_manager_get_default_display:
 * @display_manager: a #BdkDisplayManager 
 *
 * Gets the default #BdkDisplay.
 *
 * Returns: (transfer none): a #BdkDisplay, or %NULL if there is no default
 *   display.
 *
 * Since: 2.2
 */
BdkDisplay *
bdk_display_manager_get_default_display (BdkDisplayManager *display_manager)
{
  return default_display;
}

/**
 * bdk_display_get_default:
 *
 * Gets the default #BdkDisplay. This is a convenience
 * function for
 * <literal>bdk_display_manager_get_default_display (bdk_display_manager_get ())</literal>.
 *
 * Returns: (transfer none): a #BdkDisplay, or %NULL if there is no default
 *   display.
 *
 * Since: 2.2
 */
BdkDisplay *
bdk_display_get_default (void)
{
  return default_display;
}

/**
 * bdk_screen_get_default:
 *
 * Gets the default screen for the default display. (See
 * bdk_display_get_default ()).
 *
 * Returns: (transfer none): a #BdkScreen, or %NULL if there is no default display.
 *
 * Since: 2.2
 */
BdkScreen *
bdk_screen_get_default (void)
{
  if (default_display)
    return bdk_display_get_default_screen (default_display);
  else
    return NULL;
}

/**
 * bdk_display_manager_set_default_display:
 * @display_manager: a #BdkDisplayManager
 * @display: a #BdkDisplay
 * 
 * Sets @display as the default display.
 *
 * Since: 2.2
 **/
void
bdk_display_manager_set_default_display (BdkDisplayManager *display_manager,
					 BdkDisplay        *display)
{
  default_display = display;

  _bdk_windowing_set_default_display (display);

  g_object_notify (G_OBJECT (display_manager), "default-display");
}

/**
 * bdk_display_manager_list_displays:
 * @display_manager: a #BdkDisplayManager 
 *
 * List all currently open displays.
 * 
 * Return value: (transfer container) (element-type BdkDisplay): a newly allocated
 * #GSList of #BdkDisplay objects. Free this list with g_slist_free() when you
 * are done with it.
 *
 * Since: 2.2
 **/
GSList *
bdk_display_manager_list_displays (BdkDisplayManager *display_manager)
{
  return g_slist_copy (_bdk_displays);
}

#define __BDK_DISPLAY_MANAGER_C__
#include "bdkaliasdef.c"
