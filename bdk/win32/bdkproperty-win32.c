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
#include <string.h>
#include <stdlib.h>
#include <bunnylib/gprintf.h>

#include "bdkscreen.h"
#include "bdkproperty.h"
#include "bdkselection.h"
#include "bdkprivate-win32.h"

BdkAtom
bdk_atom_intern (const gchar *atom_name,
		 gint         only_if_exists)
{
  ATOM win32_atom;
  BdkAtom retval;
  static GHashTable *atom_hash = NULL;
  
  if (!atom_hash)
    atom_hash = g_hash_table_new (g_str_hash, g_str_equal);

  retval = g_hash_table_lookup (atom_hash, atom_name);
  if (!retval)
    {
      if (strcmp (atom_name, "PRIMARY") == 0)
	retval = BDK_SELECTION_PRIMARY;
      else if (strcmp (atom_name, "SECONDARY") == 0)
	retval = BDK_SELECTION_SECONDARY;
      else if (strcmp (atom_name, "CLIPBOARD") == 0)
	retval = BDK_SELECTION_CLIPBOARD;
      else if (strcmp (atom_name, "ATOM") == 0)
	retval = BDK_SELECTION_TYPE_ATOM;
      else if (strcmp (atom_name, "BITMAP") == 0)
	retval = BDK_SELECTION_TYPE_BITMAP;
      else if (strcmp (atom_name, "COLORMAP") == 0)
	retval = BDK_SELECTION_TYPE_COLORMAP;
      else if (strcmp (atom_name, "DRAWABLE") == 0)
	retval = BDK_SELECTION_TYPE_DRAWABLE;
      else if (strcmp (atom_name, "INTEGER") == 0)
	retval = BDK_SELECTION_TYPE_INTEGER;
      else if (strcmp (atom_name, "PIXMAP") == 0)
	retval = BDK_SELECTION_TYPE_PIXMAP;
      else if (strcmp (atom_name, "WINDOW") == 0)
	retval = BDK_SELECTION_TYPE_WINDOW;
      else if (strcmp (atom_name, "STRING") == 0)
	retval = BDK_SELECTION_TYPE_STRING;
      else
	{
	  win32_atom = GlobalAddAtom (atom_name);
	  retval = GUINT_TO_POINTER ((guint) win32_atom);
	}
      g_hash_table_insert (atom_hash, 
			   g_strdup (atom_name), 
			   retval);
    }

  return retval;
}

BdkAtom
bdk_atom_intern_static_string (const gchar *atom_name)
{
  /* on X11 this is supposed to save memory. On win32 there seems to be
   * no way to make a difference ?
   */
  return bdk_atom_intern (atom_name, FALSE);
}

gchar *
bdk_atom_name (BdkAtom atom)
{
  ATOM win32_atom;
  gchar name[256];

  if (BDK_NONE == atom) return g_strdup ("<none>");
  else if (BDK_SELECTION_PRIMARY == atom) return g_strdup ("PRIMARY");
  else if (BDK_SELECTION_SECONDARY == atom) return g_strdup ("SECONDARY");
  else if (BDK_SELECTION_CLIPBOARD == atom) return g_strdup ("CLIPBOARD");
  else if (BDK_SELECTION_TYPE_ATOM == atom) return g_strdup ("ATOM");
  else if (BDK_SELECTION_TYPE_BITMAP == atom) return g_strdup ("BITMAP");
  else if (BDK_SELECTION_TYPE_COLORMAP == atom) return g_strdup ("COLORMAP");
  else if (BDK_SELECTION_TYPE_DRAWABLE == atom) return g_strdup ("DRAWABLE");
  else if (BDK_SELECTION_TYPE_INTEGER == atom) return g_strdup ("INTEGER");
  else if (BDK_SELECTION_TYPE_PIXMAP == atom) return g_strdup ("PIXMAP");
  else if (BDK_SELECTION_TYPE_WINDOW == atom) return g_strdup ("WINDOW");
  else if (BDK_SELECTION_TYPE_STRING == atom) return g_strdup ("STRING");
  
  win32_atom = GPOINTER_TO_UINT (atom);
  
  if (win32_atom < 0xC000)
    return g_strdup_printf ("#%p", atom);
  else if (GlobalGetAtomName (win32_atom, name, sizeof (name)) == 0)
    return NULL;
  return g_strdup (name);
}

gint
bdk_property_get (BdkWindow   *window,
		  BdkAtom      property,
		  BdkAtom      type,
		  gulong       offset,
		  gulong       length,
		  gint         pdelete,
		  BdkAtom     *actual_property_type,
		  gint        *actual_format_type,
		  gint        *actual_length,
		  guchar     **data)
{
  g_return_val_if_fail (window != NULL, FALSE);
  g_return_val_if_fail (BDK_IS_WINDOW (window), FALSE);

  if (BDK_WINDOW_DESTROYED (window))
    return FALSE;

  g_warning ("bdk_property_get: Not implemented");

  return FALSE;
}

void
bdk_property_change (BdkWindow    *window,
		     BdkAtom       property,
		     BdkAtom       type,
		     gint          format,
		     BdkPropMode   mode,
		     const guchar *data,
		     gint          nelements)
{
  HGLOBAL hdata;
  gint i, size;
  guchar *ucptr;
  wchar_t *wcptr, *p;
  glong wclen;
  GError *err = NULL;

  g_return_if_fail (window != NULL);
  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_WINDOW_DESTROYED (window))
    return;

  BDK_NOTE (DND, {
      gchar *prop_name = bdk_atom_name (property);
      gchar *type_name = bdk_atom_name (type);
      
      g_print ("bdk_property_change: %p %s %s %s %d*%d bits: %s\n",
	       BDK_WINDOW_HWND (window),
	       prop_name,
	       type_name,
	       (mode == BDK_PROP_MODE_REPLACE ? "REPLACE" :
		(mode == BDK_PROP_MODE_PREPEND ? "PREPEND" :
		 (mode == BDK_PROP_MODE_APPEND ? "APPEND" :
		  "???"))),
	       format, nelements,
	       _bdk_win32_data_to_string (data, MIN (10, format*nelements/8)));
      g_free (prop_name);
      g_free (type_name);
    });

  /* We should never come here for these types */
  g_return_if_fail (type != BDK_TARGET_STRING);
  g_return_if_fail (type != _text);
  g_return_if_fail (type != _compound_text);
  g_return_if_fail (type != _save_targets);

  if (property == _bdk_selection &&
      format == 8 &&
      mode == BDK_PROP_MODE_REPLACE)
    {
      if (type == _image_bmp && nelements < sizeof (BITMAPFILEHEADER))
        {
           g_warning ("Clipboard contains invalid bitmap data");
           return;
        }

      if (type == _utf8_string)
	{
	  if (!OpenClipboard (BDK_WINDOW_HWND (window)))
	    {
	      WIN32_API_FAILED ("OpenClipboard");
	      return;
	    }

	  wcptr = g_utf8_to_utf16 ((char *) data, nelements, NULL, &wclen, &err);
          if (err != NULL)
            {
              g_warning ("Failed to convert utf8: %s", err->message);
              g_clear_error (&err);
              return;
            }

	  wclen++;		/* Terminating 0 */
	  size = wclen * 2;
	  for (i = 0; i < wclen; i++)
	    if (wcptr[i] == '\n' && (i == 0 || wcptr[i - 1] != '\r'))
	      size += 2;
	  
	  if (!(hdata = GlobalAlloc (GMEM_MOVEABLE, size)))
	    {
	      WIN32_API_FAILED ("GlobalAlloc");
	      if (!CloseClipboard ())
		WIN32_API_FAILED ("CloseClipboard");
	      g_free (wcptr);
	      return;
	    }

	  ucptr = GlobalLock (hdata);

	  p = (wchar_t *) ucptr;
	  for (i = 0; i < wclen; i++)
	    {
	      if (wcptr[i] == '\n' && (i == 0 || wcptr[i - 1] != '\r'))
		*p++ = '\r';
	      *p++ = wcptr[i];
	    }
	  g_free (wcptr);

	  GlobalUnlock (hdata);
	  BDK_NOTE (DND, g_print ("... SetClipboardData(CF_UNICODETEXT,%p)\n",
				  hdata));
	  if (!SetClipboardData (CF_UNICODETEXT, hdata))
	    WIN32_API_FAILED ("SetClipboardData");
      
	  if (!CloseClipboard ())
	    WIN32_API_FAILED ("CloseClipboard");
	}
      else
        {
	  /* We use delayed rendering for everything else than
	   * text. We can't assign hdata to the clipboard here as type
	   * may be "image/png", "image/jpg", etc. In this case
	   * there's a further conversion afterwards.
	   */
	  BDK_NOTE (DND, g_print ("... delayed rendering\n"));
	  _delayed_rendering_data = NULL;
	  if (!(hdata = GlobalAlloc (GMEM_MOVEABLE, nelements > 0 ? nelements : 1)))
	    {
	      WIN32_API_FAILED ("GlobalAlloc");
	      return;
	    }
	  ucptr = GlobalLock (hdata);
	  memcpy (ucptr, data, nelements);
	  GlobalUnlock (hdata);
	  _delayed_rendering_data = hdata;
	}
    }
  else if (property == _bdk_ole2_dnd)
    {
      /* Will happen only if bdkdnd-win32.c has OLE2 dnd support compiled in */
      _bdk_win32_ole2_dnd_property_change (type, format, data, nelements);
    }
  else
    g_warning ("bdk_property_change: General case not implemented");
}

void
bdk_property_delete (BdkWindow *window,
		     BdkAtom    property)
{
  gchar *prop_name;

  g_return_if_fail (window != NULL);
  g_return_if_fail (BDK_IS_WINDOW (window));

  BDK_NOTE (DND, {
      prop_name = bdk_atom_name (property);

      g_print ("bdk_property_delete: %p %s\n",
	       BDK_WINDOW_HWND (window),
	       prop_name);
      g_free (prop_name);
    });

  if (property == _bdk_selection)
    _bdk_selection_property_delete (window);
  else if (property == _wm_transient_for)
    bdk_window_set_transient_for (window, _bdk_root);
  else
    {
      prop_name = bdk_atom_name (property);
      g_warning ("bdk_property_delete: General case (%s) not implemented",
		 prop_name);
      g_free (prop_name);
    }
}

/*
  For reference, from bdk/x11/bdksettings.c:

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
  "Btk/CursorBlinkTimeout\0"  "btk-cursor-blink-timeout\0"
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
  "Net/EnableEventSounds\0"  "btk-enable-event-sounds\0";

  More, from various places in btk sources:

  btk-entry-select-on-focus
  btk-split-cursor

*/
gboolean
bdk_screen_get_setting (BdkScreen   *screen,
                        const gchar *name,
                        BValue      *value)
{
  g_return_val_if_fail (BDK_IS_SCREEN (screen), FALSE);

  /*
   * XXX : if these values get changed through the Windoze UI the
   *       respective bdk_events are not generated yet.
   */
  if (strcmp ("btk-theme-name", name) == 0) 
    {
      b_value_set_string (value, "ms-windows");
    }
  else if (strcmp ("btk-double-click-time", name) == 0)
    {
      gint i = GetDoubleClickTime ();
      BDK_NOTE(MISC, g_print("bdk_screen_get_setting(\"%s\") : %d\n", name, i));
      b_value_set_int (value, i);
      return TRUE;
    }
  else if (strcmp ("btk-double-click-distance", name) == 0)
    {
      gint i = MAX(GetSystemMetrics (SM_CXDOUBLECLK), GetSystemMetrics (SM_CYDOUBLECLK));
      BDK_NOTE(MISC, g_print("bdk_screen_get_setting(\"%s\") : %d\n", name, i));
      b_value_set_int (value, i);
      return TRUE;
    }
  else if (strcmp ("btk-dnd-drag-threshold", name) == 0)
    {
      gint i = MAX(GetSystemMetrics (SM_CXDRAG), GetSystemMetrics (SM_CYDRAG));
      BDK_NOTE(MISC, g_print("bdk_screen_get_setting(\"%s\") : %d\n", name, i));
      b_value_set_int (value, i);
      return TRUE;
    }
  else if (strcmp ("btk-split-cursor", name) == 0)
    {
      BDK_NOTE(MISC, g_print("bdk_screen_get_setting(\"%s\") : FALSE\n", name));
      b_value_set_boolean (value, FALSE);
      return TRUE;
    }
  else if (strcmp ("btk-alternative-button-order", name) == 0)
    {
      BDK_NOTE(MISC, g_print("bdk_screen_get_setting(\"%s\") : TRUE\n", name));
      b_value_set_boolean (value, TRUE);
      return TRUE;
    }
  else if (strcmp ("btk-alternative-sort-arrows", name) == 0)
    {
      BDK_NOTE(MISC, g_print("bdk_screen_get_setting(\"%s\") : TRUE\n", name));
      b_value_set_boolean (value, TRUE);
      return TRUE;
    }
#if 0
  /*
   * With 'MS Sans Serif' as windows menu font (default on win98se) you'll get a 
   * bunch of :
   *   WARNING **: Couldn't load font "MS Sans Serif 8" falling back to "Sans 8"
   * at least with testfilechooser (regardless of the bitmap check below)
   * so just disabling this code seems to be the best we can do --hb
   */
  else if (strcmp ("btk-font-name", name) == 0)
    {
      NONCLIENTMETRICS ncm;
      ncm.cbSize = sizeof(NONCLIENTMETRICS);
      if (SystemParametersInfo (SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, FALSE))
        {
          /* Bango finally uses GetDeviceCaps to scale, we use simple
	   * approximation here.
	   */
          int nHeight = (0 > ncm.lfMenuFont.lfHeight ? -3*ncm.lfMenuFont.lfHeight/4 : 10);
          if (OUT_STRING_PRECIS == ncm.lfMenuFont.lfOutPrecision)
            BDK_NOTE(MISC, g_print("bdk_screen_get_setting(%s) : ignoring bitmap font '%s'\n", 
                                   name, ncm.lfMenuFont.lfFaceName));
          else if (ncm.lfMenuFont.lfFaceName && strlen(ncm.lfMenuFont.lfFaceName) > 0 &&
                   /* Avoid issues like those described in bug #135098 */
                   g_utf8_validate (ncm.lfMenuFont.lfFaceName, -1, NULL))
            {
              char* s = g_strdup_printf ("%s %d", ncm.lfMenuFont.lfFaceName, nHeight);
              BDK_NOTE(MISC, g_print("bdk_screen_get_setting(%s) : %s\n", name, s));
              b_value_set_string (value, s);

              g_free(s);
              return TRUE;
            }
        }
    }
#endif

  return FALSE;
}
