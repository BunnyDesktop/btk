/* BTK - The GIMP Toolkit
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
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#define BANGO_ENABLE_BACKEND /* for bango_fc_font_map_cache_clear() */

#include "config.h"

#include <string.h>

#include "btkmodules.h"
#include "btksettings.h"
#include "btkrc.h"
#include "btkintl.h"
#include "btkwidget.h"
#include "btkprivate.h"
#include "btkalias.h"

#ifdef BDK_WINDOWING_X11
#include "x11/bdkx.h"
#include <bango/bangofc-fontmap.h>
#endif

#ifdef BDK_WINDOWING_QUARTZ
#define DEFAULT_KEY_THEME "Mac"
#else
#define DEFAULT_KEY_THEME NULL
#endif

#define DEFAULT_TIMEOUT_INITIAL 200
#define DEFAULT_TIMEOUT_REPEAT   20
#define DEFAULT_TIMEOUT_EXPAND  500

typedef struct _BtkSettingsValuePrivate BtkSettingsValuePrivate;

typedef enum
{
  BTK_SETTINGS_SOURCE_DEFAULT,
  BTK_SETTINGS_SOURCE_RC_FILE,
  BTK_SETTINGS_SOURCE_XSETTING,
  BTK_SETTINGS_SOURCE_APPLICATION
} BtkSettingsSource;

struct _BtkSettingsValuePrivate
{
  BtkSettingsValue public;
  BtkSettingsSource source;
};

struct _BtkSettingsPropertyValue
{
  BValue value;
  BtkSettingsSource source;
};

enum {
  PROP_0,
  PROP_DOUBLE_CLICK_TIME,
  PROP_DOUBLE_CLICK_DISTANCE,
  PROP_CURSOR_BLINK,
  PROP_CURSOR_BLINK_TIME,
  PROP_CURSOR_BLINK_TIMEOUT,
  PROP_SPLIT_CURSOR,
  PROP_THEME_NAME,
  PROP_ICON_THEME_NAME,
  PROP_FALLBACK_ICON_THEME,
  PROP_KEY_THEME_NAME,
  PROP_MENU_BAR_ACCEL,
  PROP_DND_DRAG_THRESHOLD,
  PROP_FONT_NAME,
  PROP_ICON_SIZES,
  PROP_MODULES,
#ifdef BDK_WINDOWING_X11
  PROP_XFT_ANTIALIAS,
  PROP_XFT_HINTING,
  PROP_XFT_HINTSTYLE,
  PROP_XFT_RGBA,
  PROP_XFT_DPI,
  PROP_CURSOR_THEME_NAME,
  PROP_CURSOR_THEME_SIZE,
#endif
  PROP_ALTERNATIVE_BUTTON_ORDER,
  PROP_ALTERNATIVE_SORT_ARROWS,
  PROP_SHOW_INPUT_METHOD_MENU,
  PROP_SHOW_UNICODE_MENU,
  PROP_TIMEOUT_INITIAL,
  PROP_TIMEOUT_REPEAT,
  PROP_TIMEOUT_EXPAND,
  PROP_COLOR_SCHEME,
  PROP_ENABLE_ANIMATIONS,
  PROP_TOUCHSCREEN_MODE,
  PROP_TOOLTIP_TIMEOUT,
  PROP_TOOLTIP_BROWSE_TIMEOUT,
  PROP_TOOLTIP_BROWSE_MODE_TIMEOUT,
  PROP_KEYNAV_CURSOR_ONLY,
  PROP_KEYNAV_WRAP_AROUND,
  PROP_ERROR_BELL,
  PROP_COLOR_HASH,
  PROP_FILE_CHOOSER_BACKEND,
  PROP_PRINT_BACKENDS,
  PROP_PRINT_PREVIEW_COMMAND,
  PROP_ENABLE_MNEMONICS,
  PROP_ENABLE_ACCELS,
  PROP_RECENT_FILES_LIMIT,
  PROP_IM_MODULE,
  PROP_RECENT_FILES_MAX_AGE,
  PROP_FONTCONFIG_TIMESTAMP,
  PROP_SOUND_THEME_NAME,
  PROP_ENABLE_INPUT_FEEDBACK_SOUNDS,
  PROP_ENABLE_EVENT_SOUNDS,
  PROP_ENABLE_TOOLTIPS,
  PROP_TOOLBAR_STYLE,
  PROP_TOOLBAR_ICON_SIZE,
  PROP_AUTO_MNEMONICS,
  PROP_PRIMARY_BUTTON_WARPS_SLIDER,
  PROP_BUTTON_IMAGES,
  PROP_ENTRY_SELECT_ON_FOCUS,
  PROP_ENTRY_PASSWORD_HINT_TIMEOUT,
  PROP_MENU_IMAGES,
  PROP_MENU_BAR_POPUP_DELAY,
  PROP_SCROLLED_WINDOW_PLACEMENT,
  PROP_CAN_CHANGE_ACCELS,
  PROP_MENU_POPUP_DELAY,
  PROP_MENU_POPDOWN_DELAY,
  PROP_LABEL_SELECT_ON_FOCUS,
  PROP_COLOR_PALETTE,
  PROP_IM_PREEDIT_STYLE,
  PROP_IM_STATUS_STYLE
};

/* --- prototypes --- */
static void	btk_settings_finalize		 (BObject		*object);
static void	btk_settings_get_property	 (BObject		*object,
						  buint			 property_id,
						  BValue		*value,
						  BParamSpec		*pspec);
static void	btk_settings_set_property	 (BObject		*object,
						  buint			 property_id,
						  const BValue		*value,
						  BParamSpec		*pspec);
static void	btk_settings_notify		 (BObject		*object,
						  BParamSpec		*pspec);
static buint	settings_install_property_parser (BtkSettingsClass      *class,
						  BParamSpec            *pspec,
						  BtkRcPropertyParser    parser);
static void    settings_update_double_click      (BtkSettings           *settings);
static void    settings_update_modules           (BtkSettings           *settings);

#ifdef BDK_WINDOWING_X11
static void    settings_update_cursor_theme      (BtkSettings           *settings);
static void    settings_update_resolution        (BtkSettings           *settings);
static void    settings_update_font_options      (BtkSettings           *settings);
static bboolean settings_update_fontconfig       (BtkSettings           *settings);
#endif
static void    settings_update_color_scheme      (BtkSettings *settings);

static void    merge_color_scheme                (BtkSettings           *settings, 
						  const BValue          *value, 
						  BtkSettingsSource      source);
static bchar  *get_color_scheme                  (BtkSettings           *settings);
static GHashTable *get_color_hash                (BtkSettings           *settings);

/* the default palette for BtkColorSelelection */
static const bchar default_color_palette[] =
  "black:white:gray50:red:purple:blue:light blue:green:yellow:orange:"
  "lavender:brown:goldenrod4:dodger blue:pink:light green:gray10:gray30:gray75:gray90";

/* --- variables --- */
static GQuark		 quark_property_parser = 0;
static GSList           *object_list = NULL;
static buint		 class_n_properties = 0;


G_DEFINE_TYPE (BtkSettings, btk_settings, B_TYPE_OBJECT)

/* --- functions --- */
static void
btk_settings_init (BtkSettings *settings)
{
  BParamSpec **pspecs, **p;
  buint i = 0;
  
  g_datalist_init (&settings->queued_settings);
  object_list = b_slist_prepend (object_list, settings);

  /* build up property array for all yet existing properties and queue
   * notification for them (at least notification for internal properties
   * will instantly be caught)
   */
  pspecs = g_object_class_list_properties (B_OBJECT_GET_CLASS (settings), NULL);
  for (p = pspecs; *p; p++)
    if ((*p)->owner_type == B_OBJECT_TYPE (settings))
      i++;
  settings->property_values = g_new0 (BtkSettingsPropertyValue, i);
  i = 0;
  g_object_freeze_notify (B_OBJECT (settings));
  for (p = pspecs; *p; p++)
    {
      BParamSpec *pspec = *p;

      if (pspec->owner_type != B_OBJECT_TYPE (settings))
	continue;
      b_value_init (&settings->property_values[i].value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      g_param_value_set_default (pspec, &settings->property_values[i].value);
      g_object_notify (B_OBJECT (settings), pspec->name);
      settings->property_values[i].source = BTK_SETTINGS_SOURCE_DEFAULT;
      i++;
    }
  g_object_thaw_notify (B_OBJECT (settings));
  g_free (pspecs);
}

static void
btk_settings_class_init (BtkSettingsClass *class)
{
  BObjectClass *bobject_class = B_OBJECT_CLASS (class);
  buint result;
  
  bobject_class->finalize = btk_settings_finalize;
  bobject_class->get_property = btk_settings_get_property;
  bobject_class->set_property = btk_settings_set_property;
  bobject_class->notify = btk_settings_notify;

  quark_property_parser = g_quark_from_static_string ("btk-rc-property-parser");
  result = settings_install_property_parser (class,
                                             g_param_spec_int ("btk-double-click-time",
                                                               P_("Double Click Time"),
                                                               P_("Maximum time allowed between two clicks for them to be considered a double click (in milliseconds)"),
                                                               0, B_MAXINT, 250,
                                                               BTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_DOUBLE_CLICK_TIME);
  result = settings_install_property_parser (class,
                                             g_param_spec_int ("btk-double-click-distance",
                                                               P_("Double Click Distance"),
                                                               P_("Maximum distance allowed between two clicks for them to be considered a double click (in pixels)"),
                                                               0, B_MAXINT, 5,
                                                               BTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_DOUBLE_CLICK_DISTANCE);

  /**
   * BtkSettings:btk-cursor-blink:
   *
   * Whether the cursor should blink. 
   *
   * Also see the #BtkSettings:btk-cursor-blink-timeout setting, 
   * which allows more flexible control over cursor blinking.
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("btk-cursor-blink",
								   P_("Cursor Blink"),
								   P_("Whether the cursor should blink"),
								   TRUE,
								   BTK_PARAM_READWRITE),
					     NULL);
  g_assert (result == PROP_CURSOR_BLINK);
  result = settings_install_property_parser (class,
                                             g_param_spec_int ("btk-cursor-blink-time",
                                                               P_("Cursor Blink Time"),
                                                               P_("Length of the cursor blink cycle, in milliseconds"),
                                                               100, B_MAXINT, 1200,
                                                               BTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_CURSOR_BLINK_TIME);
 
  /**
   * BtkSettings:btk-cursor-blink-timeout:
   *
   * Time after which the cursor stops blinking, in seconds.
   * The timer is reset after each user interaction.
   *
   * Setting this to zero has the same effect as setting
   * #BtkSettings:btk-cursor-blink to %FALSE. 
   *
   * Since: 2.12
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_int ("btk-cursor-blink-timeout",
                                                               P_("Cursor Blink Timeout"),
                                                               P_("Time after which the cursor stops blinking, in seconds"),
                                                               1, B_MAXINT, B_MAXINT,
                                                               BTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_CURSOR_BLINK_TIMEOUT);
  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("btk-split-cursor",
								   P_("Split Cursor"),
								   P_("Whether two cursors should be displayed for mixed left-to-right and right-to-left text"),
								   TRUE,
								   BTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_SPLIT_CURSOR);
  result = settings_install_property_parser (class,
                                             g_param_spec_string ("btk-theme-name",
								   P_("Theme Name"),
								   P_("Name of theme RC file to load"),
#ifdef G_OS_WIN32
								  "MS-Windows",
#else
								  "Raleigh",
#endif
								  BTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_THEME_NAME);

  result = settings_install_property_parser (class,
                                             g_param_spec_string ("btk-icon-theme-name",
								  P_("Icon Theme Name"),
								  P_("Name of icon theme to use"),
								  "hicolor",
								  BTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_ICON_THEME_NAME);

  result = settings_install_property_parser (class,
                                             g_param_spec_string ("btk-fallback-icon-theme",
								  P_("Fallback Icon Theme Name"),
								  P_("Name of a icon theme to fall back to"),
								  NULL,
								  BTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_FALLBACK_ICON_THEME);
  
  result = settings_install_property_parser (class,
                                             g_param_spec_string ("btk-key-theme-name",
								  P_("Key Theme Name"),
								  P_("Name of key theme RC file to load"),
								  DEFAULT_KEY_THEME,
								  BTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_KEY_THEME_NAME);    

  result = settings_install_property_parser (class,
                                             g_param_spec_string ("btk-menu-bar-accel",
                                                                  P_("Menu bar accelerator"),
                                                                  P_("Keybinding to activate the menu bar"),
                                                                  "F10",
                                                                  BTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_MENU_BAR_ACCEL);

  result = settings_install_property_parser (class,
					     g_param_spec_int ("btk-dnd-drag-threshold",
							       P_("Drag threshold"),
							       P_("Number of pixels the cursor can move before dragging"),
							       1, B_MAXINT, 8,
                                                               BTK_PARAM_READWRITE),
					     NULL);
  g_assert (result == PROP_DND_DRAG_THRESHOLD);

  result = settings_install_property_parser (class,
                                             g_param_spec_string ("btk-font-name",
								   P_("Font Name"),
								   P_("Name of default font to use"),
								  "Sans 10",
								  BTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_FONT_NAME);

  /**
   * BtkSettings:btk-icon-sizes:
   *
   * A list of icon sizes. The list is separated by colons, and
   * item has the form:
   *
   * <replaceable>size-name</replaceable> = <replaceable>width</replaceable> , <replaceable>height</replaceable>
   *
   * E.g. "btk-menu=16,16:btk-button=20,20:btk-dialog=48,48". 
   * BTK+ itself use the following named icon sizes: btk-menu, 
   * btk-button, btk-small-toolbar, btk-large-toolbar, btk-dnd, 
   * btk-dialog. Applications can register their own named icon 
   * sizes with btk_icon_size_register().
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_string ("btk-icon-sizes",
								   P_("Icon Sizes"),
								   P_("List of icon sizes (btk-menu=16,16:btk-button=20,20..."),
								  NULL,
								  BTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_ICON_SIZES);

  result = settings_install_property_parser (class,
                                             g_param_spec_string ("btk-modules",
								  P_("BTK Modules"),
								  P_("List of currently active BTK modules"),
								  NULL,
								  BTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_MODULES);

#ifdef BDK_WINDOWING_X11
  result = settings_install_property_parser (class,
					     g_param_spec_int ("btk-xft-antialias",
 							       P_("Xft Antialias"),
 							       P_("Whether to antialias Xft fonts; 0=no, 1=yes, -1=default"),
 							       -1, 1, -1,
 							       BTK_PARAM_READWRITE),
					     NULL);
 
  g_assert (result == PROP_XFT_ANTIALIAS);
  
  result = settings_install_property_parser (class,
					     g_param_spec_int ("btk-xft-hinting",
 							       P_("Xft Hinting"),
 							       P_("Whether to hint Xft fonts; 0=no, 1=yes, -1=default"),
 							       -1, 1, -1,
 							       BTK_PARAM_READWRITE),
					     NULL);
  
  g_assert (result == PROP_XFT_HINTING);
  
  result = settings_install_property_parser (class,
					     g_param_spec_string ("btk-xft-hintstyle",
 								  P_("Xft Hint Style"),
 								  P_("What degree of hinting to use; hintnone, hintslight, hintmedium, or hintfull"),
 								  NULL,
 								  BTK_PARAM_READWRITE),
                                              NULL);
  
  g_assert (result == PROP_XFT_HINTSTYLE);
  
  result = settings_install_property_parser (class,
					     g_param_spec_string ("btk-xft-rgba",
 								  P_("Xft RGBA"),
 								  P_("Type of subpixel antialiasing; none, rgb, bgr, vrgb, vbgr"),
 								  NULL,
 								  BTK_PARAM_READWRITE),
					     NULL);
  
  g_assert (result == PROP_XFT_RGBA);
  
  result = settings_install_property_parser (class,
					     g_param_spec_int ("btk-xft-dpi",
 							       P_("Xft DPI"),
 							       P_("Resolution for Xft, in 1024 * dots/inch. -1 to use default value"),
 							       -1, 1024*1024, -1,
 							       BTK_PARAM_READWRITE),
					     NULL);
  
  g_assert (result == PROP_XFT_DPI);

  result = settings_install_property_parser (class,
                                             g_param_spec_string ("btk-cursor-theme-name",
								  P_("Cursor theme name"),
								  P_("Name of the cursor theme to use, or NULL to use the default theme"),
								  NULL,
								  BTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_CURSOR_THEME_NAME);

  result = settings_install_property_parser (class,
					     g_param_spec_int ("btk-cursor-theme-size",
 							       P_("Cursor theme size"),
 							       P_("Size to use for cursors, or 0 to use the default size"),
 							       0, 128, 0,
 							       BTK_PARAM_READWRITE),
					     NULL);
  
  g_assert (result == PROP_CURSOR_THEME_SIZE);

#endif  /* BDK_WINDOWING_X11 */
  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("btk-alternative-button-order",
								   P_("Alternative button order"),
								   P_("Whether buttons in dialogs should use the alternative button order"),
								   FALSE,
								   BTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_ALTERNATIVE_BUTTON_ORDER);

  /**
   * BtkSettings:btk-alternative-sort-arrows:
   *
   * Controls the direction of the sort indicators in sorted list and tree
   * views. By default an arrow pointing down means the column is sorted
   * in ascending order. When set to %TRUE, this order will be inverted.
   *
   * Since: 2.12
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("btk-alternative-sort-arrows",
								   P_("Alternative sort indicator direction"),
								   P_("Whether the direction of the sort indicators in list and tree views is inverted compared to the default (where down means ascending)"),
								   FALSE,
								   BTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_ALTERNATIVE_SORT_ARROWS);

  result = settings_install_property_parser (class,
					     g_param_spec_boolean ("btk-show-input-method-menu",
								   P_("Show the 'Input Methods' menu"),
								   P_("Whether the context menus of entries and text views should offer to change the input method"),
								   TRUE,
								   BTK_PARAM_READWRITE),
					     NULL);
  g_assert (result == PROP_SHOW_INPUT_METHOD_MENU);

  result = settings_install_property_parser (class,
					     g_param_spec_boolean ("btk-show-unicode-menu",
								   P_("Show the 'Insert Unicode Control Character' menu"),
								   P_("Whether the context menus of entries and text views should offer to insert control characters"),
								   TRUE,
								   BTK_PARAM_READWRITE),
					     NULL);
  g_assert (result == PROP_SHOW_UNICODE_MENU);

  result = settings_install_property_parser (class,
					     g_param_spec_int ("btk-timeout-initial",
 							       P_("Start timeout"),
 							       P_("Starting value for timeouts, when button is pressed"),
 							       0, B_MAXINT, DEFAULT_TIMEOUT_INITIAL,
 							       BTK_PARAM_READWRITE),
					     NULL);

  g_assert (result == PROP_TIMEOUT_INITIAL);

  result = settings_install_property_parser (class,
					     g_param_spec_int ("btk-timeout-repeat",
 							       P_("Repeat timeout"),
 							       P_("Repeat value for timeouts, when button is pressed"),
 							       0, B_MAXINT, DEFAULT_TIMEOUT_REPEAT,
 							       BTK_PARAM_READWRITE),
					     NULL);

  g_assert (result == PROP_TIMEOUT_REPEAT);

  result = settings_install_property_parser (class,
					     g_param_spec_int ("btk-timeout-expand",
 							       P_("Expand timeout"),
 							       P_("Expand value for timeouts, when a widget is expanding a new rebunnyion"),
 							       0, B_MAXINT, DEFAULT_TIMEOUT_EXPAND,
 							       BTK_PARAM_READWRITE),
					     NULL);

  g_assert (result == PROP_TIMEOUT_EXPAND);

  /**
   * BtkSettings:btk-color-scheme:
   *
   * A palette of named colors for use in themes. The format of the string is
   * <programlisting>
   * name1: color1
   * name2: color2
   * ...
   * </programlisting>
   * Color names must be acceptable as identifiers in the 
   * <link linkend="btk-Resource-Files">btkrc</link> syntax, and
   * color specifications must be in the format accepted by
   * bdk_color_parse().
   * 
   * Note that due to the way the color tables from different sources are
   * merged, color specifications will be converted to hexadecimal form
   * when getting this property.
   *
   * Starting with BTK+ 2.12, the entries can alternatively be separated
   * by ';' instead of newlines:
   * <programlisting>
   * name1: color1; name2: color2; ...
   * </programlisting>
   *
   * Since: 2.10
   */
  result = settings_install_property_parser (class,
					     g_param_spec_string ("btk-color-scheme",
 								  P_("Color scheme"),
 								  P_("A palette of named colors for use in themes"),
 								  "",
 								  BTK_PARAM_READWRITE),
					     NULL);

  g_assert (result == PROP_COLOR_SCHEME);

  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("btk-enable-animations",
                                                                   P_("Enable Animations"),
                                                                   P_("Whether to enable toolkit-wide animations."),
                                                                   TRUE,
                                                                   BTK_PARAM_READWRITE),
                                             NULL);

  g_assert (result == PROP_ENABLE_ANIMATIONS);

  /**
   * BtkSettings:btk-touchscreen-mode:
   *
   * When %TRUE, there are no motion notify events delivered on this screen,
   * and widgets can't use the pointer hovering them for any essential
   * functionality.
   *
   * Since: 2.10
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("btk-touchscreen-mode",
                                                                   P_("Enable Touchscreen Mode"),
                                                                   P_("When TRUE, there are no motion notify events delivered on this screen"),
                                                                   FALSE,
                                                                   BTK_PARAM_READWRITE),
                                             NULL);

  g_assert (result == PROP_TOUCHSCREEN_MODE);

  /**
   * BtkSettings:btk-tooltip-timeout:
   *
   * Time, in milliseconds, after which a tooltip could appear if the
   * cursor is hovering on top of a widget.
   *
   * Since: 2.12
   */
  result = settings_install_property_parser (class,
					     g_param_spec_int ("btk-tooltip-timeout",
							       P_("Tooltip timeout"),
							       P_("Timeout before tooltip is shown"),
							       0, B_MAXINT,
							       500,
							       BTK_PARAM_READWRITE),
					     NULL);

  g_assert (result == PROP_TOOLTIP_TIMEOUT);

  /**
   * BtkSettings:btk-tooltip-browse-timeout:
   *
   * Controls the time after which tooltips will appear when
   * browse mode is enabled, in milliseconds.
   *
   * Browse mode is enabled when the mouse pointer moves off an object
   * where a tooltip was currently being displayed. If the mouse pointer
   * hits another object before the browse mode timeout expires (see
   * #BtkSettings:btk-tooltip-browse-mode-timeout), it will take the 
   * amount of milliseconds specified by this setting to popup the tooltip
   * for the new object.
   *
   * Since: 2.12
   */
  result = settings_install_property_parser (class,
					     g_param_spec_int ("btk-tooltip-browse-timeout",
							       P_("Tooltip browse timeout"),
							       P_("Timeout before tooltip is shown when browse mode is enabled"),
							       0, B_MAXINT,
							       60,
							       BTK_PARAM_READWRITE),
					     NULL);

  g_assert (result == PROP_TOOLTIP_BROWSE_TIMEOUT);

  /**
   * BtkSettings:btk-tooltip-browse-mode-timeout:
   *
   * Amount of time, in milliseconds, after which the browse mode
   * will be disabled.
   *
   * See #BtkSettings:btk-tooltip-browse-timeout for more information
   * about browse mode.
   *
   * Since: 2.12
   */
  result = settings_install_property_parser (class,
					     g_param_spec_int ("btk-tooltip-browse-mode-timeout",
 							       P_("Tooltip browse mode timeout"),
 							       P_("Timeout after which browse mode is disabled"),
 							       0, B_MAXINT,
							       500,
 							       BTK_PARAM_READWRITE),
					     NULL);

  g_assert (result == PROP_TOOLTIP_BROWSE_MODE_TIMEOUT);

  /**
   * BtkSettings:btk-keynav-cursor-only:
   *
   * When %TRUE, keyboard navigation should be able to reach all widgets
   * by using the cursor keys only. Tab, Shift etc. keys can't be expected
   * to be present on the used input device.
   *
   * Since: 2.12
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("btk-keynav-cursor-only",
                                                                   P_("Keynav Cursor Only"),
                                                                   P_("When TRUE, there are only cursor keys available to navigate widgets"),
                                                                   FALSE,
                                                                   BTK_PARAM_READWRITE),
                                             NULL);

  g_assert (result == PROP_KEYNAV_CURSOR_ONLY);

  /**
   * BtkSettings:btk-keynav-wrap-around:
   *
   * When %TRUE, some widgets will wrap around when doing keyboard
   * navigation, such as menus, menubars and notebooks.
   *
   * Since: 2.12
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("btk-keynav-wrap-around",
                                                                   P_("Keynav Wrap Around"),
                                                                   P_("Whether to wrap around when keyboard-navigating widgets"),
                                                                   TRUE,
                                                                   BTK_PARAM_READWRITE),
                                             NULL);

  g_assert (result == PROP_KEYNAV_WRAP_AROUND);

  /**
   * BtkSettings:btk-error-bell:
   *
   * When %TRUE, keyboard navigation and other input-related errors
   * will cause a beep. Since the error bell is implemented using
   * bdk_window_beep(), the windowing system may offer ways to
   * configure the error bell in many ways, such as flashing the
   * window or similar visual effects.
   *
   * Since: 2.12
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("btk-error-bell",
                                                                   P_("Error Bell"),
                                                                   P_("When TRUE, keyboard navigation and other errors will cause a beep"),
                                                                   TRUE,
                                                                   BTK_PARAM_READWRITE),
                                             NULL);

  g_assert (result == PROP_ERROR_BELL);

  /**
   * BtkSettings:color-hash:
   *
   * Holds a hash table representation of the #BtkSettings:btk-color-scheme 
   * setting, mapping color names to #BdkColor<!-- -->s. 
   *
   * Since: 2.10
   */
  result = settings_install_property_parser (class, 
                                             g_param_spec_boxed ("color-hash",
                                                                 P_("Color Hash"),
                                                                 P_("A hash table representation of the color scheme."),
                                                                 B_TYPE_HASH_TABLE,
                                                                 BTK_PARAM_READABLE),
                                             NULL);
  g_assert (result == PROP_COLOR_HASH);

  result = settings_install_property_parser (class, 
                                             g_param_spec_string ("btk-file-chooser-backend",
                                                                  P_("Default file chooser backend"),
                                                                  P_("Name of the BtkFileChooser backend to use by default"),
                                                                  NULL,
                                                                  BTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_FILE_CHOOSER_BACKEND);

  /**
   * BtkSettings:btk-print-backends:
   *
   * A comma-separated list of print backends to use in the print
   * dialog. Available print backends depend on the BTK+ installation,
   * and may include "file", "cups", "lpr" or "papi".
   *
   * Since: 2.10
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_string ("btk-print-backends",
                                                                  P_("Default print backend"),
                                                                  P_("List of the BtkPrintBackend backends to use by default"),
                                                                  BTK_PRINT_BACKENDS,
                                                                  BTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_PRINT_BACKENDS);

  /**
   * BtkSettings:btk-print-preview-command:
   *
   * A command to run for displaying the print preview. The command
   * should contain a %f placeholder, which will get replaced by
   * the path to the pdf file. The command may also contain a %s
   * placeholder, which will get replaced by the path to a file
   * containing the print settings in the format produced by 
   * btk_print_settings_to_file().
   *
   * The preview application is responsible for removing the pdf file
   * and the print settings file when it is done.
   *
   * Since: 2.10
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_string ("btk-print-preview-command",
                                                                  P_("Default command to run when displaying a print preview"),
                                                                  P_("Command to run when displaying a print preview"),
                                                                  BTK_PRINT_PREVIEW_COMMAND,
                                                                  BTK_PARAM_READWRITE),
                                             NULL); 
  g_assert (result == PROP_PRINT_PREVIEW_COMMAND);

  /**
   * BtkSettings:btk-enable-mnemonics:
   *
   * Whether labels and menu items should have visible mnemonics which
   * can be activated.
   *
   * Since: 2.12
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("btk-enable-mnemonics",
                                                                   P_("Enable Mnemonics"),
                                                                   P_("Whether labels should have mnemonics"),
                                                                   TRUE,
                                                                   BTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_ENABLE_MNEMONICS);

  /**
   * BtkSettings:btk-enable-accels:
   *
   * Whether menu items should have visible accelerators which can be
   * activated.
   *
   * Since: 2.12
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("btk-enable-accels",
                                                                   P_("Enable Accelerators"),
                                                                   P_("Whether menu items should have accelerators"),
                                                                   TRUE,
                                                                   BTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_ENABLE_ACCELS);

  /**
   * BtkSettings:btk-recent-files-limit:
   *
   * The number of recently used files that should be displayed by default by
   * #BtkRecentChooser implementations and by the #BtkFileChooser. A value of
   * -1 means every recently used file stored.
   *
   * Since: 2.12
   */
  result = settings_install_property_parser (class,
					     g_param_spec_int ("btk-recent-files-limit",
 							       P_("Recent Files Limit"),
 							       P_("Number of recently used files"),
 							       -1, B_MAXINT,
							       50,
 							       BTK_PARAM_READWRITE),
					     NULL);
  g_assert (result == PROP_RECENT_FILES_LIMIT);

  /**
   * BtkSettings:btk-im-module:
   *
   * Which IM (input method) module should be used by default. This is the 
   * input method that will be used if the user has not explicitly chosen 
   * another input method from the IM context menu.  
   * This also can be a colon-separated list of input methods, which BTK+
   * will try in turn until it finds one available on the system.
   *
   * See #BtkIMContext and see the #BtkSettings:btk-show-input-method-menu property.
   */
  result = settings_install_property_parser (class,
					     g_param_spec_string ("btk-im-module",
								  P_("Default IM module"),
								  P_("Which IM module should be used by default"),
								  NULL,
								  BTK_PARAM_READWRITE),
					     NULL);
  g_assert (result == PROP_IM_MODULE);

  /**
   * BtkSettings:btk-recent-files-max-age:
   *
   * The maximum age, in days, of the items inside the recently used
   * resources list. Items older than this setting will be excised
   * from the list. If set to 0, the list will always be empty; if
   * set to -1, no item will be removed.
   *
   * Since: 2.14
   */
  result = settings_install_property_parser (class,
					     g_param_spec_int ("btk-recent-files-max-age",
 							       P_("Recent Files Max Age"),
 							       P_("Maximum age of recently used files, in days"),
 							       -1, B_MAXINT,
							       30,
 							       BTK_PARAM_READWRITE),
					     NULL);
  g_assert (result == PROP_RECENT_FILES_MAX_AGE);

  result = settings_install_property_parser (class,
					     g_param_spec_uint ("btk-fontconfig-timestamp",
								P_("Fontconfig configuration timestamp"),
								P_("Timestamp of current fontconfig configuration"),
								0, B_MAXUINT, 0,
								BTK_PARAM_READWRITE),
					     NULL);
  
  g_assert (result == PROP_FONTCONFIG_TIMESTAMP);

  /**
   * BtkSettings:btk-sound-theme-name:
   *
   * The XDG sound theme to use for event sounds.
   *
   * See the <ulink url="http://www.freedesktop.org/wiki/Specifications/sound-theme-spec">Sound Theme spec</ulink> 
   * for more information on event sounds and sound themes.
   *
   * BTK+ itself does not support event sounds, you have to use a loadable 
   * module like the one that comes with libcanberra.
   *
   * Since: 2.14
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_string ("btk-sound-theme-name",
                                                                  P_("Sound Theme Name"),
                                                                  P_("XDG sound theme name"),
                                                                  "freedesktop",
                                                                  BTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_SOUND_THEME_NAME);

  /**
   * BtkSettings:btk-enable-input-feedback-sounds:
   *
   * Whether to play event sounds as feedback to user input.
   *
   * See the <ulink url="http://www.freedesktop.org/wiki/Specifications/sound-theme-spec">Sound Theme spec</ulink> 
   * for more information on event sounds and sound themes.
   *
   * BTK+ itself does not support event sounds, you have to use a loadable 
   * module like the one that comes with libcanberra.
   *
   * Since: 2.14
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("btk-enable-input-feedback-sounds",
                                                                   /* Translators: this means sounds that are played as feedback to user input */
								   P_("Audible Input Feedback"),
								   P_("Whether to play event sounds as feedback to user input"),
								   TRUE,
								   BTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_ENABLE_INPUT_FEEDBACK_SOUNDS);

  /**
   * BtkSettings:btk-enable-event-sounds:
   *
   * Whether to play any event sounds at all.
   *
   * See the <ulink url="http://www.freedesktop.org/wiki/Specifications/sound-theme-spec">Sound Theme spec</ulink> 
   * for more information on event sounds and sound themes.
   *
   * BTK+ itself does not support event sounds, you have to use a loadable 
   * module like the one that comes with libcanberra.
   *
   * Since: 2.14
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("btk-enable-event-sounds",
								   P_("Enable Event Sounds"),
								   P_("Whether to play any event sounds at all"),
								   TRUE,
								   BTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_ENABLE_EVENT_SOUNDS);

  /**
   * BtkSettings:btk-enable-tooltips:
   *
   * Whether tooltips should be shown on widgets.
   *
   * Since: 2.14
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("btk-enable-tooltips",
                                                                   P_("Enable Tooltips"),
                                                                   P_("Whether tooltips should be shown on widgets"),
                                                                   TRUE,
                                                                   BTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_ENABLE_TOOLTIPS);

  /**
   * BtkSettings:toolbar-style:
   *
   * The size of icons in default toolbars.
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_enum ("btk-toolbar-style",
                                                                   P_("Toolbar style"),
                                                                   P_("Whether default toolbars have text only, text and icons, icons only, etc."),
                                                                   BTK_TYPE_TOOLBAR_STYLE,
                                                                   BTK_TOOLBAR_BOTH,
                                                                   BTK_PARAM_READWRITE),
                                             btk_rc_property_parse_enum);
  g_assert (result == PROP_TOOLBAR_STYLE);

  /**
   * BtkSettings:toolbar-icon-size:
   *
   * The size of icons in default toolbars.
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_enum ("btk-toolbar-icon-size",
                                                                   P_("Toolbar Icon Size"),
                                                                   P_("The size of icons in default toolbars."),
                                                                   BTK_TYPE_ICON_SIZE,
                                                                   BTK_ICON_SIZE_LARGE_TOOLBAR,
                                                                   BTK_PARAM_READWRITE),
                                             btk_rc_property_parse_enum);
  g_assert (result == PROP_TOOLBAR_ICON_SIZE);

  /**
   * BtkSettings:btk-auto-mnemonics:
   *
   * Whether mnemonics should be automatically shown and hidden when the user
   * presses the mnemonic activator.
   *
   * Since: 2.20
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("btk-auto-mnemonics",
                                                                   P_("Auto Mnemonics"),
                                                                   P_("Whether mnemonics should be automatically shown and hidden when the user presses the mnemonic activator."),
                                                                   FALSE,
                                                                   BTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_AUTO_MNEMONICS);

  /**
   * BtkSettings:btk-primary-button-warps-slider
   *
   * Whether a click in a #BtkRange trough should scroll to the click position or
   * scroll by a single page in the respective direction.
   *
   * Since: 2.24
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("btk-primary-button-warps-slider",
                                                                   P_("Primary button warps slider"),
                                                                   P_("Whether a primary click on the trough should warp the slider into position"),
                                                                   FALSE,
                                                                   BTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_PRIMARY_BUTTON_WARPS_SLIDER);

  /**
   * BtkSettings::btk-button-images:
   *
   * Whether images should be shown on buttons
   *
   * Since: 2.4
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("btk-button-images",
                                                                   P_("Show button images"),
                                                                   P_("Whether images should be shown on buttons"),
                                                                   TRUE,
                                                                   BTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_BUTTON_IMAGES);

  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("btk-entry-select-on-focus",
                                                                   P_("Select on focus"),
                                                                   P_("Whether to select the contents of an entry when it is focused"),
                                                                   TRUE,
                                                                   BTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_ENTRY_SELECT_ON_FOCUS);

  /**
   * BtkSettings:btk-entry-password-hint-timeout:
   *
   * How long to show the last input character in hidden
   * entries. This value is in milliseconds. 0 disables showing the
   * last char. 600 is a good value for enabling it.
   *
   * Since: 2.10
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_uint ("btk-entry-password-hint-timeout",
                                                                P_("Password Hint Timeout"),
                                                                P_("How long to show the last input character in hidden entries"),
                                                                0, B_MAXUINT,
                                                                0,
                                                                BTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_ENTRY_PASSWORD_HINT_TIMEOUT);

  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("btk-menu-images",
                                                                   P_("Show menu images"),
                                                                   P_("Whether images should be shown in menus"),
                                                                   TRUE,
                                                                   BTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_MENU_IMAGES);

  result = settings_install_property_parser (class,
                                             g_param_spec_int ("btk-menu-bar-popup-delay",
                                                               P_("Delay before drop down menus appear"),
                                                               P_("Delay before the submenus of a menu bar appear"),
                                                               0, B_MAXINT,
                                                               0,
                                                               BTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_MENU_BAR_POPUP_DELAY);

  /**
   * BtkSettings:btk-scrolled-window-placement:
   *
   * Where the contents of scrolled windows are located with respect to the 
   * scrollbars, if not overridden by the scrolled window's own placement.
   *
   * Since: 2.10
   */
  result = settings_install_property_parser (class,
                                             g_param_spec_enum ("btk-scrolled-window-placement",
                                                                P_("Scrolled Window Placement"),
                                                                P_("Where the contents of scrolled windows are located with respect to the scrollbars, if not overridden by the scrolled window's own placement."),
                                                                BTK_TYPE_CORNER_TYPE,
                                                                BTK_CORNER_TOP_LEFT,
                                                                BTK_PARAM_READWRITE),
                                             btk_rc_property_parse_enum);
  g_assert (result == PROP_SCROLLED_WINDOW_PLACEMENT);

  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("btk-can-change-accels",
                                                                   P_("Can change accelerators"),
                                                                   P_("Whether menu accelerators can be changed by pressing a key over the menu item"),
                                                                   FALSE,
                                                                   BTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_CAN_CHANGE_ACCELS);

  result = settings_install_property_parser (class,
                                             g_param_spec_int ("btk-menu-popup-delay",
                                                               P_("Delay before submenus appear"),
                                                               P_("Minimum time the pointer must stay over a menu item before the submenu appear"),
                                                               0, B_MAXINT,
                                                               225,
                                                               BTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_MENU_POPUP_DELAY);

  result = settings_install_property_parser (class,
                                             g_param_spec_int ("btk-menu-popdown-delay",
                                                               P_("Delay before hiding a submenu"),
                                                               P_("The time before hiding a submenu when the pointer is moving towards the submenu"),
                                                               0, B_MAXINT,
                                                               1000,
                                                               BTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_MENU_POPDOWN_DELAY);

  result = settings_install_property_parser (class,
                                             g_param_spec_boolean ("btk-label-select-on-focus",
                                                                   P_("Select on focus"),
                                                                   P_("Whether to select the contents of a selectable label when it is focused"),
                                                                   TRUE,
                                                                   BTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_LABEL_SELECT_ON_FOCUS);

  result = settings_install_property_parser (class,
                                             g_param_spec_string ("btk-color-palette",
                                                                  P_("Custom palette"),
                                                                  P_("Palette to use in the color selector"),
                                                                  default_color_palette,
                                                                  BTK_PARAM_READWRITE),
                                             NULL);
  g_assert (result == PROP_COLOR_PALETTE);

  result = settings_install_property_parser (class,
                                             g_param_spec_enum ("btk-im-preedit-style",
                                                                P_("IM Preedit style"),
                                                                P_("How to draw the input method preedit string"),
                                                                BTK_TYPE_IM_PREEDIT_STYLE,
                                                                BTK_IM_PREEDIT_CALLBACK,
                                                                BTK_PARAM_READWRITE),
                                             btk_rc_property_parse_enum);
  g_assert (result == PROP_IM_PREEDIT_STYLE);

  result = settings_install_property_parser (class,
                                             g_param_spec_enum ("btk-im-status-style",
                                                                P_("IM Status style"),
                                                                P_("How to draw the input method statusbar"),
                                                                BTK_TYPE_IM_STATUS_STYLE,
                                                                BTK_IM_STATUS_CALLBACK,
                                                                BTK_PARAM_READWRITE),
                                             btk_rc_property_parse_enum);
  g_assert (result == PROP_IM_STATUS_STYLE);
}

static void
btk_settings_finalize (BObject *object)
{
  BtkSettings *settings = BTK_SETTINGS (object);
  buint i;

  object_list = b_slist_remove (object_list, settings);

  _btk_rc_context_destroy (settings);

  for (i = 0; i < class_n_properties; i++)
    b_value_unset (&settings->property_values[i].value);
  g_free (settings->property_values);
  
  g_datalist_clear (&settings->queued_settings);

  B_OBJECT_CLASS (btk_settings_parent_class)->finalize (object);
}

/**
 * btk_settings_get_for_screen:
 * @screen: a #BdkScreen.
 *
 * Gets the #BtkSettings object for @screen, creating it if necessary.
 *
 * Return value: (transfer none): a #BtkSettings object.
 *
 * Since: 2.2
 */
BtkSettings*
btk_settings_get_for_screen (BdkScreen *screen)
{
  BtkSettings *settings;

  g_return_val_if_fail (BDK_IS_SCREEN (screen), NULL);

  settings = g_object_get_data (B_OBJECT (screen), "btk-settings");
  if (!settings)
    {
      settings = g_object_new (BTK_TYPE_SETTINGS, NULL);
      settings->screen = screen;
      g_object_set_data_full (B_OBJECT (screen), I_("btk-settings"), 
			      settings, g_object_unref);

      btk_rc_reparse_all_for_settings (settings, TRUE);
      settings_update_double_click (settings);
#ifdef BDK_WINDOWING_X11
      settings_update_cursor_theme (settings);
      settings_update_resolution (settings);
      settings_update_font_options (settings);
#endif
      settings_update_color_scheme (settings);
    }
  
  return settings;
}

/**
 * btk_settings_get_default:
 * 
 * Gets the #BtkSettings object for the default BDK screen, creating
 * it if necessary. See btk_settings_get_for_screen().
 *
 * Return value: (transfer none): a #BtkSettings object. If there is no default
 *  screen, then returns %NULL.
 **/
BtkSettings*
btk_settings_get_default (void)
{
  BdkScreen *screen = bdk_screen_get_default ();

  if (screen)
    return btk_settings_get_for_screen (screen);
  else
    return NULL;
}

static void
btk_settings_set_property (BObject      *object,
			   buint	 property_id,
			   const BValue *value,
			   BParamSpec   *pspec)
{
  BtkSettings *settings = BTK_SETTINGS (object);

  b_value_copy (value, &settings->property_values[property_id - 1].value);
  settings->property_values[property_id - 1].source = BTK_SETTINGS_SOURCE_APPLICATION;
  
  if (pspec->param_id == PROP_COLOR_SCHEME)
    merge_color_scheme (settings, value, BTK_SETTINGS_SOURCE_APPLICATION);
}

static void
btk_settings_get_property (BObject     *object,
			   buint	property_id,
			   BValue      *value,
			   BParamSpec  *pspec)
{
  BtkSettings *settings = BTK_SETTINGS (object);
  GType value_type = G_VALUE_TYPE (value);
  GType fundamental_type = B_TYPE_FUNDAMENTAL (value_type);

  /* handle internal properties */
  switch (property_id)
    {
    case PROP_COLOR_HASH:
      b_value_set_boxed (value, get_color_hash (settings));
      return;
    case PROP_COLOR_SCHEME:
      b_value_take_string (value, get_color_scheme (settings));
      return;
    default: ;
    }

  /* For enums and strings, we need to get the value as a string,
   * not as an int, since we support using names/nicks as the setting
   * value.
   */
  if ((b_value_type_transformable (B_TYPE_INT, value_type) &&
       !(fundamental_type == B_TYPE_ENUM || fundamental_type == B_TYPE_FLAGS)) ||
      b_value_type_transformable (B_TYPE_STRING, G_VALUE_TYPE (value)) ||
      b_value_type_transformable (BDK_TYPE_COLOR, G_VALUE_TYPE (value)))
    {
      if (settings->property_values[property_id - 1].source == BTK_SETTINGS_SOURCE_APPLICATION ||
	  !bdk_screen_get_setting (settings->screen, pspec->name, value))
        b_value_copy (&settings->property_values[property_id - 1].value, value);
      else 
        g_param_value_validate (pspec, value);
    }
  else
    {
      BValue val = { 0, };

      /* Try to get xsetting as a string and parse it. */
      
      b_value_init (&val, B_TYPE_STRING);

      if (settings->property_values[property_id - 1].source == BTK_SETTINGS_SOURCE_APPLICATION ||
	  !bdk_screen_get_setting (settings->screen, pspec->name, &val))
        {
          b_value_copy (&settings->property_values[property_id - 1].value, value);
        }
      else
        {
          BValue tmp_value = { 0, };
          BValue gstrinb_value = { 0, };
          BtkRcPropertyParser parser = (BtkRcPropertyParser) g_param_spec_get_qdata (pspec, quark_property_parser);
          
          b_value_init (&gstrinb_value, B_TYPE_GSTRING);
          b_value_take_boxed (&gstrinb_value,
                              g_string_new (b_value_get_string (&val)));

          b_value_init (&tmp_value, G_PARAM_SPEC_VALUE_TYPE (pspec));

          if (parser && _btk_settings_parse_convert (parser, &gstrinb_value,
                                                     pspec, &tmp_value))
            {
              b_value_copy (&tmp_value, value);
              g_param_value_validate (pspec, value);
            }
          else
            {
              b_value_copy (&settings->property_values[property_id - 1].value, value);
            }

          b_value_unset (&gstrinb_value);
          b_value_unset (&tmp_value);
        }

      b_value_unset (&val);
    }
}

static void
btk_settings_notify (BObject    *object,
		     BParamSpec *pspec)
{
  BtkSettings *settings = BTK_SETTINGS (object);
  buint property_id = pspec->param_id;

  if (settings->screen == NULL) /* initialization */
    return;

  switch (property_id)
    {
    case PROP_MODULES:
      settings_update_modules (settings);
      break;
    case PROP_DOUBLE_CLICK_TIME:
    case PROP_DOUBLE_CLICK_DISTANCE:
      settings_update_double_click (settings);
      break;
    case PROP_COLOR_SCHEME:
      settings_update_color_scheme (settings);
      break;
#ifdef BDK_WINDOWING_X11
    case PROP_XFT_DPI:
      settings_update_resolution (settings);
      /* This is a hack because with btk_rc_reset_styles() doesn't get
       * widgets with btk_widget_style_set(), and also causes more
       * recomputation than necessary.
       */
      btk_rc_reset_styles (BTK_SETTINGS (object));
      break;
    case PROP_XFT_ANTIALIAS:
    case PROP_XFT_HINTING:
    case PROP_XFT_HINTSTYLE:
    case PROP_XFT_RGBA:
      settings_update_font_options (settings);
      btk_rc_reset_styles (BTK_SETTINGS (object));
      break;
    case PROP_FONTCONFIG_TIMESTAMP:
      if (settings_update_fontconfig (settings))
	btk_rc_reset_styles (BTK_SETTINGS (object));
      break;
    case PROP_CURSOR_THEME_NAME:
    case PROP_CURSOR_THEME_SIZE:
      settings_update_cursor_theme (settings);
      break;
#endif /* BDK_WINDOWING_X11 */
    }
}

bboolean
_btk_settings_parse_convert (BtkRcPropertyParser parser,
			     const BValue       *src_value,
			     BParamSpec         *pspec,
			     BValue	        *dest_value)
{
  bboolean success = FALSE;

  g_return_val_if_fail (G_VALUE_HOLDS (dest_value, G_PARAM_SPEC_VALUE_TYPE (pspec)), FALSE);

  if (parser)
    {
      GString *gstring;
      bboolean free_gstring = TRUE;
      
      if (G_VALUE_HOLDS (src_value, B_TYPE_GSTRING))
	{
	  gstring = b_value_get_boxed (src_value);
	  free_gstring = FALSE;
	}
      else if (G_VALUE_HOLDS_LONG (src_value))
	{
	  gstring = g_string_new (NULL);
	  g_string_append_printf (gstring, "%ld", b_value_get_long (src_value));
	}
      else if (G_VALUE_HOLDS_DOUBLE (src_value))
	{
	  gstring = g_string_new (NULL);
	  g_string_append_printf (gstring, "%f", b_value_get_double (src_value));
	}
      else if (G_VALUE_HOLDS_STRING (src_value))
	{
	  bchar *tstr = g_strescape (b_value_get_string (src_value), NULL);
	  
	  gstring = g_string_new ("\"");
	  g_string_append (gstring, tstr);
	  g_string_append_c (gstring, '\"');
	  g_free (tstr);
	}
      else
	{
	  g_return_val_if_fail (G_VALUE_HOLDS (src_value, B_TYPE_GSTRING), FALSE);
	  gstring = NULL; /* silence compiler */
	}

      success = (parser (pspec, gstring, dest_value) &&
		 !g_param_value_validate (pspec, dest_value));

      if (free_gstring)
	g_string_free (gstring, TRUE);
    }
  else if (G_VALUE_HOLDS (src_value, B_TYPE_GSTRING))
    {
      if (G_VALUE_HOLDS (dest_value, B_TYPE_STRING))
	{
	  GString *gstring = b_value_get_boxed (src_value);

	  b_value_set_string (dest_value, gstring ? gstring->str : NULL);
	  success = !g_param_value_validate (pspec, dest_value);
	}
    }
  else if (b_value_type_transformable (G_VALUE_TYPE (src_value), G_VALUE_TYPE (dest_value)))
    success = g_param_value_convert (pspec, src_value, dest_value, TRUE);

  return success;
}

static void
apply_queued_setting (BtkSettings             *data,
		      BParamSpec              *pspec,
		      BtkSettingsValuePrivate *qvalue)
{
  BValue tmp_value = { 0, };
  BtkRcPropertyParser parser = (BtkRcPropertyParser) g_param_spec_get_qdata (pspec, quark_property_parser);

  b_value_init (&tmp_value, G_PARAM_SPEC_VALUE_TYPE (pspec));
  if (_btk_settings_parse_convert (parser, &qvalue->public.value,
				   pspec, &tmp_value))
    {
      if (pspec->param_id == PROP_COLOR_SCHEME) 
        merge_color_scheme (data, &tmp_value, qvalue->source);

      if (data->property_values[pspec->param_id - 1].source <= qvalue->source)
	{
          b_value_copy (&tmp_value, &data->property_values[pspec->param_id - 1].value);
	  data->property_values[pspec->param_id - 1].source = qvalue->source;
          g_object_notify (B_OBJECT (data), g_param_spec_get_name (pspec));
	}

    }
  else
    {
      bchar *debug = g_strdup_value_contents (&qvalue->public.value);
      
      g_message ("%s: failed to retrieve property `%s' of type `%s' from rc file value \"%s\" of type `%s'",
		 qvalue->public.origin ? qvalue->public.origin : "(for origin information, set BTK_DEBUG)",
		 pspec->name,
		 g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)),
		 debug,
		 G_VALUE_TYPE_NAME (&tmp_value));
      g_free (debug);
    }
  b_value_unset (&tmp_value);
}

static buint
settings_install_property_parser (BtkSettingsClass   *class,
				  BParamSpec         *pspec,
				  BtkRcPropertyParser parser)
{
  GSList *node, *next;

  switch (B_TYPE_FUNDAMENTAL (G_PARAM_SPEC_VALUE_TYPE (pspec)))
    {
    case B_TYPE_BOOLEAN:
    case B_TYPE_UCHAR:
    case B_TYPE_CHAR:
    case B_TYPE_UINT:
    case B_TYPE_INT:
    case B_TYPE_ULONG:
    case B_TYPE_LONG:
    case B_TYPE_FLOAT:
    case B_TYPE_DOUBLE:
    case B_TYPE_STRING:
    case B_TYPE_ENUM:
      break;
    case B_TYPE_BOXED:
      if (strcmp (g_param_spec_get_name (pspec), "color-hash") == 0)
        {
          break;
        }
      /* fall through */
    default:
      if (!parser)
        {
          g_warning (B_STRLOC ": parser needs to be specified for property \"%s\" of type `%s'",
                     pspec->name, g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)));
          return 0;
        }
    }
  if (g_object_class_find_property (B_OBJECT_CLASS (class), pspec->name))
    {
      g_warning (B_STRLOC ": an rc-data property \"%s\" already exists",
		 pspec->name);
      return 0;
    }
  
  for (node = object_list; node; node = node->next)
    g_object_freeze_notify (node->data);

  g_object_class_install_property (B_OBJECT_CLASS (class), ++class_n_properties, pspec);
  g_param_spec_set_qdata (pspec, quark_property_parser, (bpointer) parser);

  for (node = object_list; node; node = node->next)
    {
      BtkSettings *settings = node->data;
      BtkSettingsValuePrivate *qvalue;
      
      settings->property_values = g_renew (BtkSettingsPropertyValue, settings->property_values, class_n_properties);
      settings->property_values[class_n_properties - 1].value.g_type = 0;
      b_value_init (&settings->property_values[class_n_properties - 1].value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      g_param_value_set_default (pspec, &settings->property_values[class_n_properties - 1].value);
      settings->property_values[class_n_properties - 1].source = BTK_SETTINGS_SOURCE_DEFAULT;
      g_object_notify (B_OBJECT (settings), pspec->name);
      
      qvalue = g_datalist_get_data (&settings->queued_settings, pspec->name);
      if (qvalue)
	apply_queued_setting (settings, pspec, qvalue);
    }

  for (node = object_list; node; node = next)
    {
      next = node->next;
      g_object_thaw_notify (node->data);
    }

  return class_n_properties;
}

BtkRcPropertyParser
_btk_rc_property_parser_from_type (GType type)
{
  if (type == BDK_TYPE_COLOR)
    return btk_rc_property_parse_color;
  else if (type == BTK_TYPE_REQUISITION)
    return btk_rc_property_parse_requisition;
  else if (type == BTK_TYPE_BORDER)
    return btk_rc_property_parse_border;
  else if (B_TYPE_FUNDAMENTAL (type) == B_TYPE_ENUM && B_TYPE_IS_DERIVED (type))
    return btk_rc_property_parse_enum;
  else if (B_TYPE_FUNDAMENTAL (type) == B_TYPE_FLAGS && B_TYPE_IS_DERIVED (type))
    return btk_rc_property_parse_flags;
  else
    return NULL;
}

void
btk_settings_install_property (BParamSpec *pspec)
{
  static BtkSettingsClass *klass = NULL;

  BtkRcPropertyParser parser;

  g_return_if_fail (G_IS_PARAM_SPEC (pspec));

  if (! klass)
    klass = g_type_class_ref (BTK_TYPE_SETTINGS);

  parser = _btk_rc_property_parser_from_type (G_PARAM_SPEC_VALUE_TYPE (pspec));

  settings_install_property_parser (klass, pspec, parser);
}

void
btk_settings_install_property_parser (BParamSpec          *pspec,
				      BtkRcPropertyParser  parser)
{
  static BtkSettingsClass *klass = NULL;

  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  g_return_if_fail (parser != NULL);

  if (! klass)
    klass = g_type_class_ref (BTK_TYPE_SETTINGS);

  settings_install_property_parser (klass, pspec, parser);
}

static void
free_value (bpointer data)
{
  BtkSettingsValuePrivate *qvalue = data;
  
  b_value_unset (&qvalue->public.value);
  g_free (qvalue->public.origin);
  g_slice_free (BtkSettingsValuePrivate, qvalue);
}

static void
btk_settings_set_property_value_internal (BtkSettings            *settings,
					  const bchar            *prop_name,
					  const BtkSettingsValue *new_value,
					  BtkSettingsSource       source)
{
  BtkSettingsValuePrivate *qvalue;
  BParamSpec *pspec;
  bchar *name;
  GQuark name_quark;

  if (!G_VALUE_HOLDS_LONG (&new_value->value) &&
      !G_VALUE_HOLDS_DOUBLE (&new_value->value) &&
      !G_VALUE_HOLDS_STRING (&new_value->value) &&
      !G_VALUE_HOLDS (&new_value->value, B_TYPE_GSTRING))
    {
      g_warning (B_STRLOC ": value type invalid");
      return;
    }
  
  name = g_strdup (prop_name);
  g_strcanon (name, G_CSET_DIGITS "-" G_CSET_a_2_z G_CSET_A_2_Z, '-');
  name_quark = g_quark_from_string (name);
  g_free (name);

  qvalue = g_datalist_id_get_data (&settings->queued_settings, name_quark);
  if (!qvalue)
    {
      qvalue = g_slice_new0 (BtkSettingsValuePrivate);
      g_datalist_id_set_data_full (&settings->queued_settings, name_quark, qvalue, free_value);
    }
  else
    {
      g_free (qvalue->public.origin);
      b_value_unset (&qvalue->public.value);
    }
  qvalue->public.origin = g_strdup (new_value->origin);
  b_value_init (&qvalue->public.value, G_VALUE_TYPE (&new_value->value));
  b_value_copy (&new_value->value, &qvalue->public.value);
  qvalue->source = source;
  pspec = g_object_class_find_property (B_OBJECT_GET_CLASS (settings), g_quark_to_string (name_quark));
  if (pspec)
    apply_queued_setting (settings, pspec, qvalue);
}

void
btk_settings_set_property_value (BtkSettings            *settings,
				 const bchar            *prop_name,
				 const BtkSettingsValue *new_value)
{
  g_return_if_fail (BTK_SETTINGS (settings));
  g_return_if_fail (prop_name != NULL);
  g_return_if_fail (new_value != NULL);

  btk_settings_set_property_value_internal (settings, prop_name, new_value,
					    BTK_SETTINGS_SOURCE_APPLICATION);
}

void
_btk_settings_set_property_value_from_rc (BtkSettings            *settings,
					  const bchar            *prop_name,
					  const BtkSettingsValue *new_value)
{
  g_return_if_fail (BTK_SETTINGS (settings));
  g_return_if_fail (prop_name != NULL);
  g_return_if_fail (new_value != NULL);

  btk_settings_set_property_value_internal (settings, prop_name, new_value,
					    BTK_SETTINGS_SOURCE_RC_FILE);
}

void
btk_settings_set_string_property (BtkSettings *settings,
				  const bchar *name,
				  const bchar *v_string,
				  const bchar *origin)
{
  BtkSettingsValue svalue = { NULL, { 0, }, };

  g_return_if_fail (BTK_SETTINGS (settings));
  g_return_if_fail (name != NULL);
  g_return_if_fail (v_string != NULL);

  svalue.origin = (bchar*) origin;
  b_value_init (&svalue.value, B_TYPE_STRING);
  b_value_set_static_string (&svalue.value, v_string);
  btk_settings_set_property_value (settings, name, &svalue);
  b_value_unset (&svalue.value);
}

void
btk_settings_set_long_property (BtkSettings *settings,
				const bchar *name,
				blong	     v_long,
				const bchar *origin)
{
  BtkSettingsValue svalue = { NULL, { 0, }, };
  
  g_return_if_fail (BTK_SETTINGS (settings));
  g_return_if_fail (name != NULL);

  svalue.origin = (bchar*) origin;
  b_value_init (&svalue.value, B_TYPE_LONG);
  b_value_set_long (&svalue.value, v_long);
  btk_settings_set_property_value (settings, name, &svalue);
  b_value_unset (&svalue.value);
}

void
btk_settings_set_double_property (BtkSettings *settings,
				  const bchar *name,
				  bdouble      v_double,
				  const bchar *origin)
{
  BtkSettingsValue svalue = { NULL, { 0, }, };

  g_return_if_fail (BTK_SETTINGS (settings));
  g_return_if_fail (name != NULL);

  svalue.origin = (bchar*) origin;
  b_value_init (&svalue.value, B_TYPE_DOUBLE);
  b_value_set_double (&svalue.value, v_double);
  btk_settings_set_property_value (settings, name, &svalue);
  b_value_unset (&svalue.value);
}

/**
 * btk_rc_property_parse_color:
 * @pspec: a #BParamSpec
 * @gstring: the #GString to be parsed
 * @property_value: a #BValue which must hold #BdkColor values.
 * 
 * A #BtkRcPropertyParser for use with btk_settings_install_property_parser()
 * or btk_widget_class_install_style_property_parser() which parses a
 * color given either by its name or in the form 
 * <literal>{ red, green, blue }</literal> where %red, %green and
 * %blue are integers between 0 and 65535 or floating-point numbers
 * between 0 and 1.
 * 
 * Return value: %TRUE if @gstring could be parsed and @property_value
 * has been set to the resulting #BdkColor.
 **/
bboolean
btk_rc_property_parse_color (const BParamSpec *pspec,
			     const GString    *gstring,
			     BValue           *property_value)
{
  BdkColor color = { 0, 0, 0, 0, };
  GScanner *scanner;
  bboolean success;

  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), FALSE);
  g_return_val_if_fail (G_VALUE_HOLDS (property_value, BDK_TYPE_COLOR), FALSE);

  scanner = btk_rc_scanner_new ();
  g_scanner_input_text (scanner, gstring->str, gstring->len);
  if (btk_rc_parse_color (scanner, &color) == G_TOKEN_NONE &&
      g_scanner_get_next_token (scanner) == G_TOKEN_EOF)
    {
      b_value_set_boxed (property_value, &color);
      success = TRUE;
    }
  else
    success = FALSE;
  g_scanner_destroy (scanner);

  return success;
}

/**
 * btk_rc_property_parse_enum:
 * @pspec: a #BParamSpec
 * @gstring: the #GString to be parsed
 * @property_value: a #BValue which must hold enum values.
 * 
 * A #BtkRcPropertyParser for use with btk_settings_install_property_parser()
 * or btk_widget_class_install_style_property_parser() which parses a single
 * enumeration value.
 *
 * The enumeration value can be specified by its name, its nickname or
 * its numeric value. For consistency with flags parsing, the value
 * may be surrounded by parentheses.
 * 
 * Return value: %TRUE if @gstring could be parsed and @property_value
 * has been set to the resulting #GEnumValue.
 **/
bboolean
btk_rc_property_parse_enum (const BParamSpec *pspec,
			    const GString    *gstring,
			    BValue           *property_value)
{
  bboolean need_closing_brace = FALSE, success = FALSE;
  GScanner *scanner;
  GEnumValue *enum_value = NULL;

  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), FALSE);
  g_return_val_if_fail (G_VALUE_HOLDS_ENUM (property_value), FALSE);

  scanner = btk_rc_scanner_new ();
  g_scanner_input_text (scanner, gstring->str, gstring->len);

  /* we just want to parse _one_ value, but for consistency with flags parsing
   * we support optional parenthesis
   */
  g_scanner_get_next_token (scanner);
  if (scanner->token == '(')
    {
      need_closing_brace = TRUE;
      g_scanner_get_next_token (scanner);
    }
  if (scanner->token == G_TOKEN_IDENTIFIER)
    {
      GEnumClass *class = G_PARAM_SPEC_ENUM (pspec)->enum_class;
      
      enum_value = g_enum_get_value_by_name (class, scanner->value.v_identifier);
      if (!enum_value)
	enum_value = g_enum_get_value_by_nick (class, scanner->value.v_identifier);
      if (enum_value)
	{
	  b_value_set_enum (property_value, enum_value->value);
	  success = TRUE;
	}
    }
  else if (scanner->token == G_TOKEN_INT)
    {
      b_value_set_enum (property_value, scanner->value.v_int);
      success = TRUE;
    }
  if (need_closing_brace && g_scanner_get_next_token (scanner) != ')')
    success = FALSE;
  if (g_scanner_get_next_token (scanner) != G_TOKEN_EOF)
    success = FALSE;

  g_scanner_destroy (scanner);

  return success;
}

static buint
parse_flags_value (GScanner    *scanner,
		   GFlagsClass *class,
		   buint       *number)
{
  g_scanner_get_next_token (scanner);
  if (scanner->token == G_TOKEN_IDENTIFIER)
    {
      GFlagsValue *flags_value;

      flags_value = g_flags_get_value_by_name (class, scanner->value.v_identifier);
      if (!flags_value)
	flags_value = g_flags_get_value_by_nick (class, scanner->value.v_identifier);
      if (flags_value)
	{
	  *number |= flags_value->value;
	  return G_TOKEN_NONE;
	}
    }
  else if (scanner->token == G_TOKEN_INT)
    {
      *number |= scanner->value.v_int;
      return G_TOKEN_NONE;
    }
  return G_TOKEN_IDENTIFIER;
}

/**
 * btk_rc_property_parse_flags:
 * @pspec: a #BParamSpec
 * @gstring: the #GString to be parsed
 * @property_value: a #BValue which must hold flags values.
 * 
 * A #BtkRcPropertyParser for use with btk_settings_install_property_parser()
 * or btk_widget_class_install_style_property_parser() which parses flags. 
 * 
 * Flags can be specified by their name, their nickname or
 * numerically. Multiple flags can be specified in the form 
 * <literal>"( flag1 | flag2 | ... )"</literal>.
 * 
 * Return value: %TRUE if @gstring could be parsed and @property_value
 * has been set to the resulting flags value.
 **/
bboolean
btk_rc_property_parse_flags (const BParamSpec *pspec,
			     const GString    *gstring,
			     BValue           *property_value)
{
  GFlagsClass *class;
   bboolean success = FALSE;
  GScanner *scanner;

  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), FALSE);
  g_return_val_if_fail (G_VALUE_HOLDS_FLAGS (property_value), FALSE);

  class = G_PARAM_SPEC_FLAGS (pspec)->flags_class;
  scanner = btk_rc_scanner_new ();
  g_scanner_input_text (scanner, gstring->str, gstring->len);

  /* parse either a single flags value or a "\( ... [ \| ... ] \)" compound */
  if (g_scanner_peek_next_token (scanner) == G_TOKEN_IDENTIFIER ||
      scanner->next_token == G_TOKEN_INT)
    {
      buint token, flags_value = 0;
      
      token = parse_flags_value (scanner, class, &flags_value);

      if (token == G_TOKEN_NONE && g_scanner_peek_next_token (scanner) == G_TOKEN_EOF)
	{
	  success = TRUE;
	  b_value_set_flags (property_value, flags_value);
	}
      
    }
  else if (g_scanner_get_next_token (scanner) == '(')
    {
      buint token, flags_value = 0;

      /* parse first value */
      token = parse_flags_value (scanner, class, &flags_value);

      /* parse nth values, preceeded by '|' */
      while (token == G_TOKEN_NONE && g_scanner_get_next_token (scanner) == '|')
	token = parse_flags_value (scanner, class, &flags_value);

      /* done, last token must have closed expression */
      if (token == G_TOKEN_NONE && scanner->token == ')' &&
	  g_scanner_peek_next_token (scanner) == G_TOKEN_EOF)
	{
	  b_value_set_flags (property_value, flags_value);
	  success = TRUE;
	}
    }
  g_scanner_destroy (scanner);

  return success;
}

static bboolean
get_braced_int (GScanner *scanner,
		bboolean  first,
		bboolean  last,
		bint     *value)
{
  if (first)
    {
      g_scanner_get_next_token (scanner);
      if (scanner->token != '{')
	return FALSE;
    }

  g_scanner_get_next_token (scanner);
  if (scanner->token != G_TOKEN_INT)
    return FALSE;

  *value = scanner->value.v_int;

  if (last)
    {
      g_scanner_get_next_token (scanner);
      if (scanner->token != '}')
	return FALSE;
    }
  else
    {
      g_scanner_get_next_token (scanner);
      if (scanner->token != ',')
	return FALSE;
    }

  return TRUE;
}

/**
 * btk_rc_property_parse_requisition:
 * @pspec: a #BParamSpec
 * @gstring: the #GString to be parsed
 * @property_value: a #BValue which must hold boxed values.
 * 
 * A #BtkRcPropertyParser for use with btk_settings_install_property_parser()
 * or btk_widget_class_install_style_property_parser() which parses a
 * requisition in the form 
 * <literal>"{ width, height }"</literal> for integers %width and %height.
 * 
 * Return value: %TRUE if @gstring could be parsed and @property_value
 * has been set to the resulting #BtkRequisition.
 **/
bboolean
btk_rc_property_parse_requisition  (const BParamSpec *pspec,
				    const GString    *gstring,
				    BValue           *property_value)
{
  BtkRequisition requisition;
  GScanner *scanner;
  bboolean success = FALSE;

  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), FALSE);
  g_return_val_if_fail (G_VALUE_HOLDS_BOXED (property_value), FALSE);

  scanner = btk_rc_scanner_new ();
  g_scanner_input_text (scanner, gstring->str, gstring->len);

  if (get_braced_int (scanner, TRUE, FALSE, &requisition.width) &&
      get_braced_int (scanner, FALSE, TRUE, &requisition.height))
    {
      b_value_set_boxed (property_value, &requisition);
      success = TRUE;
    }

  g_scanner_destroy (scanner);

  return success;
}

/**
 * btk_rc_property_parse_border:
 * @pspec: a #BParamSpec
 * @gstring: the #GString to be parsed
 * @property_value: a #BValue which must hold boxed values.
 * 
 * A #BtkRcPropertyParser for use with btk_settings_install_property_parser()
 * or btk_widget_class_install_style_property_parser() which parses
 * borders in the form 
 * <literal>"{ left, right, top, bottom }"</literal> for integers 
 * %left, %right, %top and %bottom.
 * 
 * Return value: %TRUE if @gstring could be parsed and @property_value
 * has been set to the resulting #BtkBorder.
 **/
bboolean
btk_rc_property_parse_border (const BParamSpec *pspec,
			      const GString    *gstring,
			      BValue           *property_value)
{
  BtkBorder border;
  GScanner *scanner;
  bboolean success = FALSE;

  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), FALSE);
  g_return_val_if_fail (G_VALUE_HOLDS_BOXED (property_value), FALSE);

  scanner = btk_rc_scanner_new ();
  g_scanner_input_text (scanner, gstring->str, gstring->len);

  if (get_braced_int (scanner, TRUE, FALSE, &border.left) &&
      get_braced_int (scanner, FALSE, FALSE, &border.right) &&
      get_braced_int (scanner, FALSE, FALSE, &border.top) &&
      get_braced_int (scanner, FALSE, TRUE, &border.bottom))
    {
      b_value_set_boxed (property_value, &border);
      success = TRUE;
    }

  g_scanner_destroy (scanner);

  return success;
}

void
_btk_settings_handle_event (BdkEventSetting *event)
{
  BtkSettings *settings;
  BParamSpec *pspec;
  buint property_id;

  settings = btk_settings_get_for_screen (bdk_window_get_screen (event->window));
  pspec = g_object_class_find_property (B_OBJECT_GET_CLASS (settings), event->name);
 
  if (pspec) 
    {
      property_id = pspec->param_id;

      if (property_id == PROP_COLOR_SCHEME)
        {
          BValue value = { 0, };
 
          b_value_init (&value, B_TYPE_STRING);
          if (!bdk_screen_get_setting (settings->screen, pspec->name, &value))
            b_value_set_static_string (&value, "");
          merge_color_scheme (settings, &value, BTK_SETTINGS_SOURCE_XSETTING);
          b_value_unset (&value);
        }
      g_object_notify (B_OBJECT (settings), pspec->name);
   }
}

static void
reset_rc_values_foreach (GQuark    key_id,
			 bpointer  data,
			 bpointer  user_data)
{
  BtkSettingsValuePrivate *qvalue = data;
  GSList **to_reset = user_data;

  if (qvalue->source == BTK_SETTINGS_SOURCE_RC_FILE)
    *to_reset = b_slist_prepend (*to_reset, BUINT_TO_POINTER (key_id));
}

void
_btk_settings_reset_rc_values (BtkSettings *settings)
{
  GSList *to_reset = NULL;
  GSList *tmp_list;
  BParamSpec **pspecs, **p;
  bint i;

  /* Remove any queued settings
   */
  g_datalist_foreach (&settings->queued_settings,
		      reset_rc_values_foreach,
		      &to_reset);

  for (tmp_list = to_reset; tmp_list; tmp_list = tmp_list->next)
    {
      GQuark key_id = BPOINTER_TO_UINT (tmp_list->data);
      g_datalist_id_remove_data (&settings->queued_settings, key_id);
    }

   b_slist_free (to_reset);

  /* Now reset the active settings
   */
  pspecs = g_object_class_list_properties (B_OBJECT_GET_CLASS (settings), NULL);
  i = 0;

  g_object_freeze_notify (B_OBJECT (settings));
  for (p = pspecs; *p; p++)
    {
      if (settings->property_values[i].source == BTK_SETTINGS_SOURCE_RC_FILE)
	{
	  BParamSpec *pspec = *p;

	  g_param_value_set_default (pspec, &settings->property_values[i].value);
	  g_object_notify (B_OBJECT (settings), pspec->name);
	}
      i++;
    }
  g_object_thaw_notify (B_OBJECT (settings));
  g_free (pspecs);
}

static void
settings_update_double_click (BtkSettings *settings)
{
  if (bdk_screen_get_number (settings->screen) == 0)
    {
      BdkDisplay *display = bdk_screen_get_display (settings->screen);
      bint double_click_time;
      bint double_click_distance;
  
      g_object_get (settings, 
		    "btk-double-click-time", &double_click_time, 
		    "btk-double-click-distance", &double_click_distance,
		    NULL);
      
      bdk_display_set_double_click_time (display, double_click_time);
      bdk_display_set_double_click_distance (display, double_click_distance);
    }
}

static void
settings_update_modules (BtkSettings *settings)
{
  bchar *modules;
  
  g_object_get (settings, 
		"btk-modules", &modules,
		NULL);
  
  _btk_modules_settings_changed (settings, modules);
  
  g_free (modules);
}

#ifdef BDK_WINDOWING_X11
static void
settings_update_cursor_theme (BtkSettings *settings)
{
  BdkDisplay *display = bdk_screen_get_display (settings->screen);
  bchar *theme = NULL;
  bint size = 0;
  
  g_object_get (settings, 
		"btk-cursor-theme-name", &theme,
		"btk-cursor-theme-size", &size,
		NULL);
  
  bdk_x11_display_set_cursor_theme (display, theme, size);

  g_free (theme);
}

static void
settings_update_font_options (BtkSettings *settings)
{
  bint hinting;
  bchar *hint_style_str;
  bairo_hint_style_t hint_style = BAIRO_HINT_STYLE_NONE;
  bint antialias;
  bairo_antialias_t antialias_mode = BAIRO_ANTIALIAS_GRAY;
  bchar *rgba_str;
  bairo_subpixel_order_t subpixel_order = BAIRO_SUBPIXEL_ORDER_DEFAULT;
  bairo_font_options_t *options;
  
  g_object_get (settings,
		"btk-xft-antialias", &antialias,
		"btk-xft-hinting", &hinting,
		"btk-xft-hintstyle", &hint_style_str,
		"btk-xft-rgba", &rgba_str,
		NULL);

  options = bairo_font_options_create ();

  bairo_font_options_set_hint_metrics (options, BAIRO_HINT_METRICS_ON);
  
  if (hinting >= 0 && !hinting)
    {
      hint_style = BAIRO_HINT_STYLE_NONE;
    }
  else if (hint_style_str)
    {
      if (strcmp (hint_style_str, "hintnone") == 0)
	hint_style = BAIRO_HINT_STYLE_NONE;
      else if (strcmp (hint_style_str, "hintslight") == 0)
	hint_style = BAIRO_HINT_STYLE_SLIGHT;
      else if (strcmp (hint_style_str, "hintmedium") == 0)
	hint_style = BAIRO_HINT_STYLE_MEDIUM;
      else if (strcmp (hint_style_str, "hintfull") == 0)
	hint_style = BAIRO_HINT_STYLE_FULL;
    }

  g_free (hint_style_str);

  bairo_font_options_set_hint_style (options, hint_style);

  if (rgba_str)
    {
      if (strcmp (rgba_str, "rgb") == 0)
	subpixel_order = BAIRO_SUBPIXEL_ORDER_RGB;
      else if (strcmp (rgba_str, "bgr") == 0)
	subpixel_order = BAIRO_SUBPIXEL_ORDER_BGR;
      else if (strcmp (rgba_str, "vrgb") == 0)
	subpixel_order = BAIRO_SUBPIXEL_ORDER_VRGB;
      else if (strcmp (rgba_str, "vbgr") == 0)
	subpixel_order = BAIRO_SUBPIXEL_ORDER_VBGR;

      g_free (rgba_str);
    }

  bairo_font_options_set_subpixel_order (options, subpixel_order);
  
  if (antialias >= 0 && !antialias)
    antialias_mode = BAIRO_ANTIALIAS_NONE;
  else if (subpixel_order != BAIRO_SUBPIXEL_ORDER_DEFAULT)
    antialias_mode = BAIRO_ANTIALIAS_SUBPIXEL;
  else if (antialias >= 0)
    antialias_mode = BAIRO_ANTIALIAS_GRAY;
  
  bairo_font_options_set_antialias (options, antialias_mode);

  bdk_screen_set_font_options (settings->screen, options);
  
  bairo_font_options_destroy (options);
}

#ifdef BDK_WINDOWING_X11
static bboolean
settings_update_fontconfig (BtkSettings *settings)
{
  static buint    last_update_timestamp;
  static bboolean last_update_needed;

  buint timestamp;

  g_object_get (settings,
		"btk-fontconfig-timestamp", &timestamp,
		NULL);

  /* if timestamp is the same as last_update_timestamp, we already have
   * updated fontconig on this timestamp (another screen requested it perhaps?),
   * just return the cached result.*/

  if (timestamp != last_update_timestamp)
    {
      BangoFontMap *fontmap = bango_bairo_font_map_get_default ();
      bboolean update_needed = FALSE;

      /* bug 547680 */
      if (BANGO_IS_FC_FONT_MAP (fontmap) && !FcConfigUptoDate (NULL))
	{
	  bango_fc_font_map_cache_clear (BANGO_FC_FONT_MAP (fontmap));
	  if (FcInitReinitialize ())
	    update_needed = TRUE;
	}

      last_update_timestamp = timestamp;
      last_update_needed = update_needed;
    }

  return last_update_needed;
}
#endif /* BDK_WINDOWING_X11 */

static void
settings_update_resolution (BtkSettings *settings)
{
  bint dpi_int;
  double dpi;
  
  g_object_get (settings,
		"btk-xft-dpi", &dpi_int,
		NULL);

  if (dpi_int > 0)
    dpi = dpi_int / 1024.;
  else
    dpi = -1.;

  bdk_screen_set_resolution (settings->screen, dpi);
}
#endif

typedef struct {
  GHashTable *color_hash;
  GHashTable *tables[BTK_SETTINGS_SOURCE_APPLICATION + 1];
  bchar *lastentry[BTK_SETTINGS_SOURCE_APPLICATION + 1];
} ColorSchemeData;

static void
color_scheme_data_free (ColorSchemeData *data)
{
  bint i;

  g_hash_table_unref (data->color_hash);

  for (i = 0; i <= BTK_SETTINGS_SOURCE_APPLICATION; i++)
    {
      if (data->tables[i])
	g_hash_table_unref (data->tables[i]);
      g_free (data->lastentry[i]);
    }

  g_slice_free (ColorSchemeData, data);
}

static void
settings_update_color_scheme (BtkSettings *settings)
{
  if (!g_object_get_data (B_OBJECT (settings), "btk-color-scheme"))
    {
      ColorSchemeData *data;
      BValue value = { 0, };

      data = g_slice_new0 (ColorSchemeData);
      data->color_hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free,
					        (GDestroyNotify) bdk_color_free);
      g_object_set_data_full (B_OBJECT (settings), "btk-color-scheme",
			      data, (GDestroyNotify) color_scheme_data_free); 

      b_value_init (&value, B_TYPE_STRING);
      if (bdk_screen_get_setting (settings->screen, "btk-color-scheme", &value))
        {
          merge_color_scheme (settings, &value, BTK_SETTINGS_SOURCE_XSETTING);
          b_value_unset (&value);
        }
   }
}

static bboolean
add_color_to_hash (bchar      *name,
		   BdkColor   *color,
		   GHashTable *target)
{
  BdkColor *old;

  old = g_hash_table_lookup (target, name);
  if (!old || !bdk_color_equal (old, color))
    {
      g_hash_table_insert (target, g_strdup (name), bdk_color_copy (color));

      return TRUE;
    }

  return FALSE;
}

static bboolean
add_colors_to_hash_from_string (GHashTable  *hash,
				const bchar *colors)
{
  bchar *s, *p, *name;
  BdkColor color;
  bboolean changed = FALSE;
  bchar *copy;

  copy = g_strdup (colors);
  s = copy;
  while (s && *s)
    {
      name = s;
      p = strchr (s, ':');
      if (p)
        {
          *p = '\0';
          p++;
        }
      else
	break;

      while (*p == ' ')
        p++;

      s = p;
      while (*s) 
	{
	  if (*s == '\n' || *s == ';')
	    {
	      *s = '\0';
	      s++;
	      break;
	    }
	  s++;
	}

      if (bdk_color_parse (p, &color))
	changed |= add_color_to_hash (name, &color, hash);
    }

  g_free (copy);

  return changed;
}

static bboolean
update_color_hash (ColorSchemeData   *data,
		   const bchar       *str,
		   BtkSettingsSource  source)
{
  bboolean changed = FALSE;
  bint i;
  GHashTable *old_hash;
  GHashTableIter iter;
  bpointer name;
  bpointer color;

  if ((str == NULL || *str == '\0') &&
      (data->lastentry[source] == NULL || data->lastentry[source][0] == '\0'))
    return FALSE;

  if (str && data->lastentry[source] && strcmp (str, data->lastentry[source]) == 0)
    return FALSE;

  /* For the RC_FILE source we merge the values rather than over-writing
   * them, since multiple rc files might define independent sets of colors
   */
  if ((source != BTK_SETTINGS_SOURCE_RC_FILE) &&
      data->tables[source] && g_hash_table_size (data->tables[source]) > 0)
    {
      g_hash_table_unref (data->tables[source]);
      data->tables[source] = NULL;
      changed = TRUE; /* We can't rely on the code below since str might be "" */
    }

  if (data->tables[source] == NULL)
    data->tables[source] = g_hash_table_new_full (g_str_hash, g_str_equal,
						  g_free,
						  (GDestroyNotify) bdk_color_free);

  g_free (data->lastentry[source]);
  data->lastentry[source] = g_strdup (str);

  changed |= add_colors_to_hash_from_string (data->tables[source], str);

  if (!changed)
    return FALSE;

  /* Rebuild the merged hash table. */
  if (data->color_hash)
    {
      old_hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free,
                                        (GDestroyNotify) bdk_color_free);

      g_hash_table_iter_init (&iter, data->color_hash);
      while (g_hash_table_iter_next (&iter, &name, &color))
        {
          g_hash_table_insert (old_hash, name, color);
          g_hash_table_iter_steal (&iter);
        }
    }
  else
    {
      old_hash = NULL;
    }

  for (i = 0; i <= BTK_SETTINGS_SOURCE_APPLICATION; i++)
    {
      if (data->tables[i])
	g_hash_table_foreach (data->tables[i], (GHFunc) add_color_to_hash,
			      data->color_hash);
    }

  if (old_hash)
    {
      /* now check if the merged hash has changed */
      changed = FALSE;
      if (g_hash_table_size (old_hash) != g_hash_table_size (data->color_hash))
        changed = TRUE;
      else
        {
          GHashTableIter iter;
          bpointer key, value, new_value;

          g_hash_table_iter_init (&iter, old_hash);
          while (g_hash_table_iter_next (&iter, &key, &value))
            {
              new_value = g_hash_table_lookup (data->color_hash, key);
              if (!new_value || !bdk_color_equal (value, new_value))
                {
                  changed = TRUE;
                  break;
                }
            }
        }

      g_hash_table_unref (old_hash);
    }
  else
    changed = TRUE;

  return changed;
}

static void
merge_color_scheme (BtkSettings       *settings, 
		    const BValue      *value, 
		    BtkSettingsSource  source)
{
  ColorSchemeData *data;
  const bchar *colors;

  g_object_freeze_notify (B_OBJECT (settings));

  colors = b_value_get_string (value);

  settings_update_color_scheme (settings);

  data = (ColorSchemeData *) g_object_get_data (B_OBJECT (settings),
						"btk-color-scheme");
  
  if (update_color_hash (data, colors, source))
    g_object_notify (B_OBJECT (settings), "color-hash");

  g_object_thaw_notify (B_OBJECT (settings));
}

static GHashTable *
get_color_hash (BtkSettings *settings)
{
  ColorSchemeData *data;

  settings_update_color_scheme (settings);
  
  data = (ColorSchemeData *)g_object_get_data (B_OBJECT (settings), 
					       "btk-color-scheme");

  return data->color_hash;
}

static void 
append_color_scheme (bpointer key,
		     bpointer value,
		     bpointer data)
{
  bchar *name = (bchar *)key;
  BdkColor *color = (BdkColor *)value;
  GString *string = (GString *)data;

  g_string_append_printf (string, "%s: #%04x%04x%04x\n",
			  name, color->red, color->green, color->blue);
}

static bchar *
get_color_scheme (BtkSettings *settings)
{
  ColorSchemeData *data;
  GString *string;
  
  settings_update_color_scheme (settings);

  data = (ColorSchemeData *) g_object_get_data (B_OBJECT (settings),
						"btk-color-scheme");

  string = g_string_new ("");

  g_hash_table_foreach (data->color_hash, append_color_scheme, string);

  return g_string_free (string, FALSE);
}


#define __BTK_SETTINGS_C__
#include "btkaliasdef.c"
