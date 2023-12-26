/* BDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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


#define BDK_SETTINGS_N_ELEMENTS()       G_N_ELEMENTS (bdk_settings_map)
#define BDK_SETTINGS_X_NAME(nth)        (bdk_settings_names + bdk_settings_map[nth].xsettings_offset)
#define BDK_SETTINGS_BDK_NAME(nth)      (bdk_settings_names + bdk_settings_map[nth].bdk_offset)

/* WARNING:
 * You will need to update bdk_settings_map when adding a
 * new setting, and make sure that checksettings does not
 * fail before committing
 */
static const char bdk_settings_names[] =
  "Net/DoubleClickTime\0"     "btk-double-click-time\0"
  "Net/DoubleClickDistance\0" "btk-double-click-distance\0"
  "Net/DndDragThreshold\0"    "btk-dnd-drag-threshold\0"
  "Net/CursorBlink\0"         "btk-cursor-blink\0"
  "Net/CursorBlinkTime\0"     "btk-cursor-blink-time\0"
  "Net/ThemeName\0"           "btk-theme-name\0"
  "Net/IconThemeName\0"       "btk-icon-theme-name\0"
  "Btk/CanChangeAccels\0"     "btk-can-change-accels\0"
  "Btk/ColorPalette\0"        "btk-color-palette\0"
  "Btk/FontName\0"            "btk-font-name\0"
  "Btk/IconSizes\0"           "btk-icon-sizes\0"
  "Btk/KeyThemeName\0"        "btk-key-theme-name\0"
  "Btk/ToolbarStyle\0"        "btk-toolbar-style\0"
  "Btk/ToolbarIconSize\0"     "btk-toolbar-icon-size\0"
  "Btk/IMPreeditStyle\0"      "btk-im-preedit-style\0"
  "Btk/IMStatusStyle\0"       "btk-im-status-style\0"
  "Btk/Modules\0"             "btk-modules\0"
  "Btk/FileChooserBackend\0"  "btk-file-chooser-backend\0"
  "Btk/ButtonImages\0"        "btk-button-images\0"
  "Btk/MenuImages\0"          "btk-menu-images\0"
  "Btk/MenuBarAccel\0"        "btk-menu-bar-accel\0"
  "Btk/CursorThemeName\0"     "btk-cursor-theme-name\0"
  "Btk/CursorThemeSize\0"     "btk-cursor-theme-size\0"
  "Btk/ShowInputMethodMenu\0" "btk-show-input-method-menu\0"
  "Btk/ShowUnicodeMenu\0"     "btk-show-unicode-menu\0"
  "Btk/TimeoutInitial\0"      "btk-timeout-initial\0"
  "Btk/TimeoutRepeat\0"       "btk-timeout-repeat\0"
  "Btk/ColorScheme\0"         "btk-color-scheme\0"
  "Btk/EnableAnimations\0"    "btk-enable-animations\0"
  "Xft/Antialias\0"           "btk-xft-antialias\0"
  "Xft/Hinting\0"             "btk-xft-hinting\0"
  "Xft/HintStyle\0"           "btk-xft-hintstyle\0"
  "Xft/RGBA\0"                "btk-xft-rgba\0"
  "Xft/DPI\0"                 "btk-xft-dpi\0"
  "Net/FallbackIconTheme\0"   "btk-fallback-icon-theme\0"
  "Btk/TouchscreenMode\0"     "btk-touchscreen-mode\0"
  "Btk/EnableAccels\0"        "btk-enable-accels\0"
  "Btk/EnableMnemonics\0"     "btk-enable-mnemonics\0"
  "Btk/ScrolledWindowPlacement\0" "btk-scrolled-window-placement\0"
  "Btk/IMModule\0"            "btk-im-module\0"
  "Fontconfig/Timestamp\0"    "btk-fontconfig-timestamp\0"
  "Net/SoundThemeName\0"      "btk-sound-theme-name\0"
  "Net/EnableInputFeedbackSounds\0" "btk-enable-input-feedback-sounds\0"
  "Net/EnableEventSounds\0"   "btk-enable-event-sounds\0"
  "Btk/CursorBlinkTimeout\0"  "btk-cursor-blink-timeout\0"
  "Btk/AutoMnemonics\0"       "btk-auto-mnemonics\0";


static const struct
{
  bint xsettings_offset;
  bint bdk_offset;
} bdk_settings_map[] = {
  {    0,   20 },
  {   42,   66 },
  {   92,  113 },
  {  136,  152 },
  {  169,  189 },
  {  211,  225 },
  {  240,  258 },
  {  278,  298 },
  {  320,  337 },
  {  355,  368 },
  {  382,  396 },
  {  411,  428 },
  {  447,  464 },
  {  482,  502 },
  {  524,  543 },
  {  564,  582 },
  {  602,  614 },
  {  626,  649 },
  {  674,  691 },
  {  709,  724 },
  {  740,  757 },
  {  776,  796 },
  {  818,  838 },
  {  860,  884 },
  {  911,  931 },
  {  953,  972 },
  {  992, 1010 },
  { 1029, 1045 },
  { 1062, 1083 },
  { 1105, 1119 },
  { 1137, 1149 },
  { 1165, 1179 },
  { 1197, 1206 },
  { 1219, 1227 },
  { 1239, 1261 },
  { 1285, 1305 },
  { 1326, 1343 },
  { 1361, 1381 },
  { 1402, 1430 },
  { 1460, 1473 },
  { 1487, 1508 },
  { 1533, 1552 },
  { 1573, 1603 },
  { 1636, 1658 },
  { 1682, 1705 },
  { 1730, 1748 }
};
