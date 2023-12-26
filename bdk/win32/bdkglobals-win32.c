/* BDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 1998-2002 Tor Lillqvist
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
#include "bdktypes.h"
#include "bdkprivate-win32.h"

BdkDisplay	 *_bdk_display = NULL;
BdkScreen	 *_bdk_screen = NULL;
BdkWindow	 *_bdk_root = NULL;

gint		  _bdk_num_monitors;
BdkWin32Monitor  *_bdk_monitors = NULL;

gint		  _bdk_offset_x, _bdk_offset_y;

HDC		  _bdk_display_hdc;
HINSTANCE	  _bdk_dll_hinstance;
HINSTANCE	  _bdk_app_hmodule;

HKL		  _bdk_input_locale;
gboolean	  _bdk_input_locale_is_ime;
UINT		  _bdk_input_codepage;

BdkAtom           _bdk_selection;
BdkAtom	          _wm_transient_for;
BdkAtom		  _targets;
BdkAtom		  _delete;
BdkAtom		  _save_targets;
BdkAtom           _utf8_string;
BdkAtom		  _text;
BdkAtom		  _compound_text;
BdkAtom		  _text_uri_list;
BdkAtom		  _text_html;
BdkAtom		  _image_png;
BdkAtom		  _image_jpeg;
BdkAtom		  _image_bmp;
BdkAtom		  _image_gif;

BdkAtom		  _local_dnd;
BdkAtom		  _bdk_win32_dropfiles;
BdkAtom		  _bdk_ole2_dnd;

UINT		  _cf_png;
UINT		  _cf_jfif;
UINT		  _cf_gif;
UINT		  _cf_url;
UINT		  _cf_html_format;
UINT		  _cf_text_html;

BdkWin32DndState  _dnd_target_state = BDK_WIN32_DND_NONE;
BdkWin32DndState  _dnd_source_state = BDK_WIN32_DND_NONE;

gint		  _bdk_input_ignore_wintab = FALSE;
gint		  _bdk_max_colors = 0;

gboolean	  _modal_operation_in_progress = FALSE;
HWND              _modal_move_resize_window = NULL;
gboolean	  _ignore_destroy_clipboard = FALSE;

HGLOBAL           _delayed_rendering_data = NULL;
GHashTable       *_format_atom_table = NULL;
