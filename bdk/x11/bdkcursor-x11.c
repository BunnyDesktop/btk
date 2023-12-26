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

#define BDK_PIXBUF_ENABLE_BACKEND

#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#ifdef HAVE_XCURSOR
#include <X11/Xcursor/Xcursor.h>
#endif
#ifdef HAVE_XFIXES
#include <X11/extensions/Xfixes.h>
#endif
#include <string.h>
#include <errno.h>

#include "bdkprivate-x11.h"
#include "bdkcursor.h"
#include "bdkdisplay-x11.h"
#include "bdkpixmap-x11.h"
#include "bdkx.h"
#include <bdk/bdkpixmap.h>
#include <bdk-pixbuf/bdk-pixbuf.h>
#include "bdkalias.h"

static guint theme_serial = 0;

/* cursor_cache holds a cache of non-pixmap cursors to avoid expensive 
 * libXcursor searches, cursors are added to it but only removed when
 * their display is closed. We make the assumption that since there are 
 * a small number of display's and a small number of cursor's that this 
 * list will stay small enough not to be a problem.
 */
static GSList* cursor_cache = NULL;

struct cursor_cache_key
{
  BdkDisplay* display;
  BdkCursorType type;
  const char* name;
};

/* Caller should check if there is already a match first.
 * Cursor MUST be either a typed cursor or a pixmap with 
 * a non-NULL name.
 */
static void
add_to_cache (BdkCursorPrivate* cursor)
{
  cursor_cache = b_slist_prepend (cursor_cache, cursor);

  /* Take a ref so that if the caller frees it we still have it */
  bdk_cursor_ref ((BdkCursor*) cursor);
}

/* Returns 0 on a match
 */
static gint
cache_compare_func (gconstpointer listelem, 
                    gconstpointer target)
{
  BdkCursorPrivate* cursor = (BdkCursorPrivate*)listelem;
  struct cursor_cache_key* key = (struct cursor_cache_key*)target;

  if ((cursor->cursor.type != key->type) ||
      (cursor->display != key->display))
    return 1; /* No match */
  
  /* Elements marked as pixmap must be named cursors 
   * (since we don't store normal pixmap cursors 
   */
  if (key->type == BDK_CURSOR_IS_PIXMAP)
    return strcmp (key->name, cursor->name);

  return 0; /* Match */
}

/* Returns the cursor if there is a match, NULL if not
 * For named cursors type shall be BDK_CURSOR_IS_PIXMAP
 * For unnamed, typed cursors, name shall be NULL
 */
static BdkCursorPrivate*
find_in_cache (BdkDisplay    *display, 
               BdkCursorType  type,
               const char    *name)
{
  GSList* res;
  struct cursor_cache_key key;

  key.display = display;
  key.type = type;
  key.name = name;

  res = b_slist_find_custom (cursor_cache, &key, cache_compare_func);

  if (res)
    return (BdkCursorPrivate *) res->data;

  return NULL;
}

/* Called by bdk_display_x11_finalize to flush any cached cursors
 * for a dead display.
 */
void 
_bdk_x11_cursor_display_finalize (BdkDisplay *display)
{
  GSList* item;
  GSList** itemp; /* Pointer to the thing to fix when we delete an item */
  item = cursor_cache;
  itemp = &cursor_cache;
  while (item)
    {
      BdkCursorPrivate* cursor = (BdkCursorPrivate*)(item->data);
      if (cursor->display == display)
        {
	  GSList* olditem;
          bdk_cursor_unref ((BdkCursor*) cursor);
	  /* Remove this item from the list */
	  *(itemp) = item->next;
	  olditem = item;
	  item = b_slist_next (item);
	  b_slist_free_1 (olditem);
        } 
      else 
        {
	  itemp = &(item->next);
	  item = b_slist_next (item);
	}
    }
}

static Cursor
get_blank_cursor (BdkDisplay *display)
{
  BdkScreen *screen;
  BdkPixmap *pixmap;
  Pixmap source_pixmap;
  XColor color;
  Cursor cursor;

  screen = bdk_display_get_default_screen (display);
  pixmap = bdk_bitmap_create_from_data (bdk_screen_get_root_window (screen), 
					"\0\0\0\0\0\0\0\0", 1, 1);
 
  source_pixmap = BDK_PIXMAP_XID (pixmap);

  color.pixel = 0; 
  color.red = color.blue = color.green = 0;
  
  if (display->closed)
    cursor = None;
  else
    cursor = XCreatePixmapCursor (BDK_DISPLAY_XDISPLAY (display),
                                  source_pixmap, source_pixmap,
                                  &color, &color, 1, 1);
  g_object_unref (pixmap);

  return cursor;
}

/**
 * bdk_cursor_new_for_display:
 * @display: the #BdkDisplay for which the cursor will be created
 * @cursor_type: cursor to create
 * 
 * Creates a new cursor from the set of builtin cursors.
 * Some useful ones are:
 * <itemizedlist>
 * <listitem><para>
 *  <inlinegraphic format="PNG" fileref="right_ptr.png"></inlinegraphic> #BDK_RIGHT_PTR (right-facing arrow)
 * </para></listitem>
 * <listitem><para>
 *  <inlinegraphic format="PNG" fileref="crosshair.png"></inlinegraphic> #BDK_CROSSHAIR (crosshair)
 * </para></listitem>
 * <listitem><para>
 *  <inlinegraphic format="PNG" fileref="xterm.png"></inlinegraphic> #BDK_XTERM (I-beam)
 * </para></listitem>
 * <listitem><para>
 * <inlinegraphic format="PNG" fileref="watch.png"></inlinegraphic> #BDK_WATCH (busy)
 * </para></listitem>
 * <listitem><para>
 * <inlinegraphic format="PNG" fileref="fleur.png"></inlinegraphic> #BDK_FLEUR (for moving objects)
 * </para></listitem>
 * <listitem><para>
 * <inlinegraphic format="PNG" fileref="hand1.png"></inlinegraphic> #BDK_HAND1 (a right-pointing hand)
 * </para></listitem>
 * <listitem><para>
 * <inlinegraphic format="PNG" fileref="hand2.png"></inlinegraphic> #BDK_HAND2 (a left-pointing hand)
 * </para></listitem>
 * <listitem><para>
 * <inlinegraphic format="PNG" fileref="left_side.png"></inlinegraphic> #BDK_LEFT_SIDE (resize left side)
 * </para></listitem>
 * <listitem><para>
 * <inlinegraphic format="PNG" fileref="right_side.png"></inlinegraphic> #BDK_RIGHT_SIDE (resize right side)
 * </para></listitem>
 * <listitem><para>
 * <inlinegraphic format="PNG" fileref="top_left_corner.png"></inlinegraphic> #BDK_TOP_LEFT_CORNER (resize northwest corner)
 * </para></listitem>
 * <listitem><para>
 * <inlinegraphic format="PNG" fileref="top_right_corner.png"></inlinegraphic> #BDK_TOP_RIGHT_CORNER (resize northeast corner)
 * </para></listitem>
 * <listitem><para>
 * <inlinegraphic format="PNG" fileref="bottom_left_corner.png"></inlinegraphic> #BDK_BOTTOM_LEFT_CORNER (resize southwest corner)
 * </para></listitem>
 * <listitem><para>
 * <inlinegraphic format="PNG" fileref="bottom_right_corner.png"></inlinegraphic> #BDK_BOTTOM_RIGHT_CORNER (resize southeast corner)
 * </para></listitem>
 * <listitem><para>
 * <inlinegraphic format="PNG" fileref="top_side.png"></inlinegraphic> #BDK_TOP_SIDE (resize top side)
 * </para></listitem>
 * <listitem><para>
 * <inlinegraphic format="PNG" fileref="bottom_side.png"></inlinegraphic> #BDK_BOTTOM_SIDE (resize bottom side)
 * </para></listitem>
 * <listitem><para>
 * <inlinegraphic format="PNG" fileref="sb_h_double_arrow.png"></inlinegraphic> #BDK_SB_H_DOUBLE_ARROW (move vertical splitter)
 * </para></listitem>
 * <listitem><para>
 * <inlinegraphic format="PNG" fileref="sb_v_double_arrow.png"></inlinegraphic> #BDK_SB_V_DOUBLE_ARROW (move horizontal splitter)
 * </para></listitem>
 * <listitem><para>
 * #BDK_BLANK_CURSOR (Blank cursor). Since 2.16
 * </para></listitem>
 * </itemizedlist>
 *
 * Return value: a new #BdkCursor
 *
 * Since: 2.2
 **/
BdkCursor*
bdk_cursor_new_for_display (BdkDisplay    *display,
			    BdkCursorType  cursor_type)
{
  BdkCursorPrivate *private;
  BdkCursor *cursor;
  Cursor xcursor;

  g_return_val_if_fail (BDK_IS_DISPLAY (display), NULL);

  if (display->closed)
    {
      xcursor = None;
    } 
  else 
    {
      private = find_in_cache (display, cursor_type, NULL);

      if (private)
        {
          /* Cache had it, add a ref for this user */
          bdk_cursor_ref ((BdkCursor*) private);
       
          return (BdkCursor*) private;
        } 
      else 
        {
	  if (cursor_type != BDK_BLANK_CURSOR)
            xcursor = XCreateFontCursor (BDK_DISPLAY_XDISPLAY (display),
                                         cursor_type);
	  else
	    xcursor = get_blank_cursor (display);
       }
    }
  
  private = g_new (BdkCursorPrivate, 1);
  private->display = display;
  private->xcursor = xcursor;
  private->name = NULL;
  private->serial = theme_serial;

  cursor = (BdkCursor *) private;
  cursor->type = cursor_type;
  cursor->ref_count = 1;
  
  if (xcursor != None)
    add_to_cache (private);

  return cursor;
}

/**
 * bdk_cursor_new_from_pixmap:
 * @source: the pixmap specifying the cursor.
 * @mask: the pixmap specifying the mask, which must be the same size as 
 *    @source.
 * @fg: the foreground color, used for the bits in the source which are 1.
 *    The color does not have to be allocated first. 
 * @bg: the background color, used for the bits in the source which are 0.
 *    The color does not have to be allocated first.
 * @x: the horizontal offset of the 'hotspot' of the cursor. 
 * @y: the vertical offset of the 'hotspot' of the cursor.
 * 
 * Creates a new cursor from a given pixmap and mask. Both the pixmap and mask
 * must have a depth of 1 (i.e. each pixel has only 2 values - on or off).
 * The standard cursor size is 16 by 16 pixels. You can create a bitmap 
 * from inline data as in the below example.
 * 
 * <example><title>Creating a custom cursor</title>
 * <programlisting>
 * /<!-- -->* This data is in X bitmap format, and can be created with the 'bitmap'
 *    utility. *<!-- -->/
 * &num;define cursor1_width 16
 * &num;define cursor1_height 16
 * static unsigned char cursor1_bits[] = {
 *   0x80, 0x01, 0x40, 0x02, 0x20, 0x04, 0x10, 0x08, 0x08, 0x10, 0x04, 0x20,
 *   0x82, 0x41, 0x41, 0x82, 0x41, 0x82, 0x82, 0x41, 0x04, 0x20, 0x08, 0x10,
 *   0x10, 0x08, 0x20, 0x04, 0x40, 0x02, 0x80, 0x01};
 *  
 * static unsigned char cursor1mask_bits[] = {
 *   0x80, 0x01, 0xc0, 0x03, 0x60, 0x06, 0x30, 0x0c, 0x18, 0x18, 0x8c, 0x31,
 *   0xc6, 0x63, 0x63, 0xc6, 0x63, 0xc6, 0xc6, 0x63, 0x8c, 0x31, 0x18, 0x18,
 *   0x30, 0x0c, 0x60, 0x06, 0xc0, 0x03, 0x80, 0x01};
 *  
 *  
 *  BdkCursor *cursor;
 *  BdkPixmap *source, *mask;
 *  BdkColor fg = { 0, 65535, 0, 0 }; /<!-- -->* Red. *<!-- -->/
 *  BdkColor bg = { 0, 0, 0, 65535 }; /<!-- -->* Blue. *<!-- -->/
 *  
 *  
 *  source = bdk_bitmap_create_from_data (NULL, cursor1_bits,
 *                                        cursor1_width, cursor1_height);
 *  mask = bdk_bitmap_create_from_data (NULL, cursor1mask_bits,
 *                                      cursor1_width, cursor1_height);
 *  cursor = bdk_cursor_new_from_pixmap (source, mask, &amp;fg, &amp;bg, 8, 8);
 *  g_object_unref (source);
 *  g_object_unref (mask);
 *  
 *  
 *  bdk_window_set_cursor (widget->window, cursor);
 * </programlisting>
 * </example>
 *
 * Return value: a new #BdkCursor.
 **/
BdkCursor*
bdk_cursor_new_from_pixmap (BdkPixmap      *source,
			    BdkPixmap      *mask,
			    const BdkColor *fg,
			    const BdkColor *bg,
			    gint            x,
			    gint            y)
{
  BdkCursorPrivate *private;
  BdkCursor *cursor;
  Pixmap source_pixmap, mask_pixmap;
  Cursor xcursor;
  XColor xfg, xbg;
  BdkDisplay *display;

  g_return_val_if_fail (BDK_IS_PIXMAP (source), NULL);
  g_return_val_if_fail (BDK_IS_PIXMAP (mask), NULL);
  g_return_val_if_fail (fg != NULL, NULL);
  g_return_val_if_fail (bg != NULL, NULL);

  source_pixmap = BDK_PIXMAP_XID (source);
  mask_pixmap   = BDK_PIXMAP_XID (mask);
  display = BDK_PIXMAP_DISPLAY (source);

  xfg.pixel = fg->pixel;
  xfg.red = fg->red;
  xfg.blue = fg->blue;
  xfg.green = fg->green;
  xbg.pixel = bg->pixel;
  xbg.red = bg->red;
  xbg.blue = bg->blue;
  xbg.green = bg->green;
  
  if (display->closed)
    xcursor = None;
  else
    xcursor = XCreatePixmapCursor (BDK_DISPLAY_XDISPLAY (display),
				   source_pixmap, mask_pixmap, &xfg, &xbg, x, y);
  private = g_new (BdkCursorPrivate, 1);
  private->display = display;
  private->xcursor = xcursor;
  private->name = NULL;
  private->serial = theme_serial;

  cursor = (BdkCursor *) private;
  cursor->type = BDK_CURSOR_IS_PIXMAP;
  cursor->ref_count = 1;
  
  return cursor;
}

void
_bdk_cursor_destroy (BdkCursor *cursor)
{
  BdkCursorPrivate *private;

  g_return_if_fail (cursor != NULL);
  g_return_if_fail (cursor->ref_count == 0);

  private = (BdkCursorPrivate *) cursor;
  if (!private->display->closed && private->xcursor)
    XFreeCursor (BDK_DISPLAY_XDISPLAY (private->display), private->xcursor);

  g_free (private->name);
  g_free (private);
}

/**
 * bdk_x11_cursor_get_xdisplay:
 * @cursor: a #BdkCursor.
 * 
 * Returns the display of a #BdkCursor.
 * 
 * Return value: an Xlib <type>Display*</type>.
 **/
Display *
bdk_x11_cursor_get_xdisplay (BdkCursor *cursor)
{
  g_return_val_if_fail (cursor != NULL, NULL);

  return BDK_DISPLAY_XDISPLAY(((BdkCursorPrivate *)cursor)->display);
}

/**
 * bdk_x11_cursor_get_xcursor:
 * @cursor: a #BdkCursor.
 * 
 * Returns the X cursor belonging to a #BdkCursor.
 * 
 * Return value: an Xlib <type>Cursor</type>.
 **/
Cursor
bdk_x11_cursor_get_xcursor (BdkCursor *cursor)
{
  g_return_val_if_fail (cursor != NULL, None);

  return ((BdkCursorPrivate *)cursor)->xcursor;
}

/** 
 * bdk_cursor_get_display:
 * @cursor: a #BdkCursor.
 *
 * Returns the display on which the #BdkCursor is defined.
 *
 * Returns: the #BdkDisplay associated to @cursor
 *
 * Since: 2.2
 */

BdkDisplay *
bdk_cursor_get_display (BdkCursor *cursor)
{
  g_return_val_if_fail (cursor != NULL, NULL);

  return ((BdkCursorPrivate *)cursor)->display;
}

#if defined(HAVE_XCURSOR) && defined(HAVE_XFIXES) && XFIXES_MAJOR >= 2

/**
 * bdk_cursor_get_image:
 * @cursor: a #BdkCursor
 *
 * Returns a #BdkPixbuf with the image used to display the cursor.
 *
 * Note that depending on the capabilities of the windowing system and 
 * on the cursor, BDK may not be able to obtain the image data. In this 
 * case, %NULL is returned.
 *
 * Returns: a #BdkPixbuf representing @cursor, or %NULL
 *
 * Since: 2.8
 */
BdkPixbuf*  
bdk_cursor_get_image (BdkCursor *cursor)
{
  Display *xdisplay;
  BdkCursorPrivate *private;
  XcursorImages *images = NULL;
  XcursorImage *image;
  gint size;
  gchar buf[32];
  guchar *data, *p, tmp;
  BdkPixbuf *pixbuf;
  gchar *theme;
  
  g_return_val_if_fail (cursor != NULL, NULL);

  private = (BdkCursorPrivate *) cursor;
    
  xdisplay = BDK_DISPLAY_XDISPLAY (private->display);

  size = XcursorGetDefaultSize (xdisplay);
  theme = XcursorGetTheme (xdisplay);

  if (cursor->type == BDK_CURSOR_IS_PIXMAP)
    {
      if (private->name)
	images = XcursorLibraryLoadImages (private->name, theme, size);
    }
  else 
    images = XcursorShapeLoadImages (cursor->type, theme, size);

  if (!images)
    return NULL;
  
  image = images->images[0];

  data = g_malloc (4 * image->width * image->height);
  memcpy (data, image->pixels, 4 * image->width * image->height);

  for (p = data; p < data + (4 * image->width * image->height); p += 4)
    {
      tmp = p[0];
      p[0] = p[2];
      p[2] = tmp;
    }

  pixbuf = bdk_pixbuf_new_from_data (data, BDK_COLORSPACE_RGB, TRUE,
				     8, image->width, image->height,
				     4 * image->width, 
				     (BdkPixbufDestroyNotify)g_free, NULL);

  if (private->name)
    bdk_pixbuf_set_option (pixbuf, "name", private->name);
  g_snprintf (buf, 32, "%d", image->xhot);
  bdk_pixbuf_set_option (pixbuf, "x_hot", buf);
  g_snprintf (buf, 32, "%d", image->yhot);
  bdk_pixbuf_set_option (pixbuf, "y_hot", buf);

  XcursorImagesDestroy (images);

  return pixbuf;
}

void
_bdk_x11_cursor_update_theme (BdkCursor *cursor)
{
  Display *xdisplay;
  BdkCursorPrivate *private;
  Cursor new_cursor = None;
  BdkDisplayX11 *display_x11;

  private = (BdkCursorPrivate *) cursor;
  xdisplay = BDK_DISPLAY_XDISPLAY (private->display);
  display_x11 = BDK_DISPLAY_X11 (private->display);

  if (!display_x11->have_xfixes)
    return;

  if (private->serial == theme_serial)
    return;

  private->serial = theme_serial;

  if (private->xcursor != None)
    {
      if (cursor->type == BDK_BLANK_CURSOR)
        return;

      if (cursor->type == BDK_CURSOR_IS_PIXMAP)
	{
	  if (private->name)
	    new_cursor = XcursorLibraryLoadCursor (xdisplay, private->name);
	}
      else 
	new_cursor = XcursorShapeLoadCursor (xdisplay, cursor->type);
      
      if (new_cursor != None)
	{
	  XFixesChangeCursor (xdisplay, new_cursor, private->xcursor);
 	  private->xcursor = new_cursor;
	}
    }
}

static void
update_cursor (gpointer data,
	       gpointer user_data)
{
  BdkCursor *cursor;

  cursor = (BdkCursor*)(data);

  if (!cursor)
    return;
  
  _bdk_x11_cursor_update_theme (cursor);
}

/**
 * bdk_x11_display_set_cursor_theme:
 * @display: a #BdkDisplay
 * @theme: the name of the cursor theme to use, or %NULL to unset
 *         a previously set value 
 * @size: the cursor size to use, or 0 to keep the previous size
 *
 * Sets the cursor theme from which the images for cursor
 * should be taken. 
 * 
 * If the windowing system supports it, existing cursors created 
 * with bdk_cursor_new(), bdk_cursor_new_for_display() and 
 * bdk_cursor_new_for_name() are updated to reflect the theme 
 * change. Custom cursors constructed with bdk_cursor_new_from_pixmap() 
 * or bdk_cursor_new_from_pixbuf() will have to be handled
 * by the application (BTK+ applications can learn about 
 * cursor theme changes by listening for change notification
 * for the corresponding #BtkSetting).
 *
 * Since: 2.8
 */
void
bdk_x11_display_set_cursor_theme (BdkDisplay  *display,
				  const gchar *theme,
				  const gint   size)
{
  BdkDisplayX11 *display_x11;
  Display *xdisplay;
  gchar *old_theme;
  gint old_size;

  g_return_if_fail (BDK_IS_DISPLAY (display));

  display_x11 = BDK_DISPLAY_X11 (display);
  xdisplay = BDK_DISPLAY_XDISPLAY (display);

  old_theme = XcursorGetTheme (xdisplay);
  old_size = XcursorGetDefaultSize (xdisplay);

  if (old_size == size &&
      (old_theme == theme ||
       (old_theme && theme && strcmp (old_theme, theme) == 0)))
    return;

  theme_serial++;

  XcursorSetTheme (xdisplay, theme);
  if (size > 0)
    XcursorSetDefaultSize (xdisplay, size);
    
  b_slist_foreach (cursor_cache, update_cursor, NULL);
}

#else

BdkPixbuf*  
bdk_cursor_get_image (BdkCursor *cursor)
{
  g_return_val_if_fail (cursor != NULL, NULL);
  
  return NULL;
}

void
bdk_x11_display_set_cursor_theme (BdkDisplay  *display,
				  const gchar *theme,
				  const gint   size)
{
  g_return_if_fail (BDK_IS_DISPLAY (display));
}

void
_bdk_x11_cursor_update_theme (BdkCursor *cursor)
{
  g_return_if_fail (cursor != NULL);
}

#endif

#ifdef HAVE_XCURSOR

static XcursorImage*
create_cursor_image (BdkPixbuf *pixbuf,
		     gint       x,
		     gint       y)
{
  guint width, height, rowstride, n_channels;
  guchar *pixels, *src;
  XcursorImage *xcimage;
  XcursorPixel *dest;

  width = bdk_pixbuf_get_width (pixbuf);
  height = bdk_pixbuf_get_height (pixbuf);

  n_channels = bdk_pixbuf_get_n_channels (pixbuf);
  rowstride = bdk_pixbuf_get_rowstride (pixbuf);
  pixels = bdk_pixbuf_get_pixels (pixbuf);

  xcimage = XcursorImageCreate (width, height);

  xcimage->xhot = x;
  xcimage->yhot = y;

  dest = xcimage->pixels;

  if (n_channels == 3)
    {
      gint i, j;

      for (j = 0; j < height; j++)
        {
          src = pixels + j * rowstride;
          for (i = 0; i < width; i++)
            {
              *dest = (0xff << 24) | (src[0] << 16) | (src[1] << 8) | src[2];
            }

	  src += n_channels;
	  dest++;
	}
    }
  else
    {
      _bdk_x11_convert_to_format (pixels, rowstride,
                                  (guchar *) dest, 4 * width,
                                  BDK_X11_FORMAT_ARGB,
                                  (G_BYTE_ORDER == G_BIG_ENDIAN) ?
                                  BDK_MSB_FIRST : BDK_LSB_FIRST,
                                  width, height);
    }

  return xcimage;
}


/**
 * bdk_cursor_new_from_pixbuf:
 * @display: the #BdkDisplay for which the cursor will be created
 * @pixbuf: the #BdkPixbuf containing the cursor image
 * @x: the horizontal offset of the 'hotspot' of the cursor. 
 * @y: the vertical offset of the 'hotspot' of the cursor.
 *
 * Creates a new cursor from a pixbuf. 
 *
 * Not all BDK backends support RGBA cursors. If they are not 
 * supported, a monochrome approximation will be displayed. 
 * The functions bdk_display_supports_cursor_alpha() and 
 * bdk_display_supports_cursor_color() can be used to determine
 * whether RGBA cursors are supported; 
 * bdk_display_get_default_cursor_size() and 
 * bdk_display_get_maximal_cursor_size() give information about 
 * cursor sizes.
 *
 * If @x or @y are <literal>-1</literal>, the pixbuf must have
 * options named "x_hot" and "y_hot", resp., containing
 * integer values between %0 and the width resp. height of
 * the pixbuf. (Since: 3.0)
 *
 * On the X backend, support for RGBA cursors requires a
 * sufficently new version of the X Render extension. 
 *
 * Returns: a new #BdkCursor.
 * 
 * Since: 2.4
 */
BdkCursor *
bdk_cursor_new_from_pixbuf (BdkDisplay *display, 
			    BdkPixbuf  *pixbuf,
			    gint        x,
			    gint        y)
{
  XcursorImage *xcimage;
  Cursor xcursor;
  BdkCursorPrivate *private;
  BdkCursor *cursor;
  const char *option;
  char *end;
  gint64 value;

  g_return_val_if_fail (BDK_IS_DISPLAY (display), NULL);
  g_return_val_if_fail (BDK_IS_PIXBUF (pixbuf), NULL);

  if (x == -1 && (option = bdk_pixbuf_get_option (pixbuf, "x_hot")))
    {
      errno = 0;
      end = NULL;
      value = g_ascii_strtoll (option, &end, 10);
      if (errno == 0 &&
          end != option &&
          value >= 0 && value < G_MAXINT)
        x = (gint) value;
    }
  if (y == -1 && (option = bdk_pixbuf_get_option (pixbuf, "y_hot")))
    {
      errno = 0;
      end = NULL;
      value = g_ascii_strtoll (option, &end, 10);
      if (errno == 0 &&
          end != option &&
          value >= 0 && value < G_MAXINT)
        y = (gint) value;
    }

  g_return_val_if_fail (0 <= x && x < bdk_pixbuf_get_width (pixbuf), NULL);
  g_return_val_if_fail (0 <= y && y < bdk_pixbuf_get_height (pixbuf), NULL);

  if (display->closed)
    xcursor = None;
  else 
    {
      xcimage = create_cursor_image (pixbuf, x, y);
      xcursor = XcursorImageLoadCursor (BDK_DISPLAY_XDISPLAY (display), xcimage);
      XcursorImageDestroy (xcimage);
    }

  private = g_new (BdkCursorPrivate, 1);
  private->display = display;
  private->xcursor = xcursor;
  private->name = NULL;
  private->serial = theme_serial;

  cursor = (BdkCursor *) private;
  cursor->type = BDK_CURSOR_IS_PIXMAP;
  cursor->ref_count = 1;
  
  return cursor;
}

/**
 * bdk_cursor_new_from_name:
 * @display: the #BdkDisplay for which the cursor will be created
 * @name: the name of the cursor
 *
 * Creates a new cursor by looking up @name in the current cursor
 * theme. 
 * 
 * Returns: a new #BdkCursor, or %NULL if there is no cursor with 
 *   the given name 
 *
 * Since: 2.8
 */
BdkCursor*  
bdk_cursor_new_from_name (BdkDisplay  *display,
			  const gchar *name)
{
  Cursor xcursor;
  Display *xdisplay;
  BdkCursorPrivate *private;
  BdkCursor *cursor;

  g_return_val_if_fail (BDK_IS_DISPLAY (display), NULL);

  if (display->closed)
    xcursor = None;
  else 
    {
      private = find_in_cache (display, BDK_CURSOR_IS_PIXMAP, name);

      if (private)
        {
          /* Cache had it, add a ref for this user */
          bdk_cursor_ref ((BdkCursor*) private);

          return (BdkCursor*) private;
        }

      xdisplay = BDK_DISPLAY_XDISPLAY (display);
      xcursor = XcursorLibraryLoadCursor (xdisplay, name);
      if (xcursor == None)
	return NULL;
    }

  private = g_new (BdkCursorPrivate, 1);
  private->display = display;
  private->xcursor = xcursor;
  private->name = g_strdup (name);
  private->serial = theme_serial;

  cursor = (BdkCursor *) private;
  cursor->type = BDK_CURSOR_IS_PIXMAP;
  cursor->ref_count = 1;
  add_to_cache (private);

  return cursor;
}

/**
 * bdk_display_supports_cursor_alpha:
 * @display: a #BdkDisplay
 *
 * Returns %TRUE if cursors can use an 8bit alpha channel 
 * on @display. Otherwise, cursors are restricted to bilevel 
 * alpha (i.e. a mask).
 *
 * Returns: whether cursors can have alpha channels.
 *
 * Since: 2.4
 */
gboolean 
bdk_display_supports_cursor_alpha (BdkDisplay *display)
{
  g_return_val_if_fail (BDK_IS_DISPLAY (display), FALSE);

  return XcursorSupportsARGB (BDK_DISPLAY_XDISPLAY (display));
}

/**
 * bdk_display_supports_cursor_color:
 * @display: a #BdkDisplay
 *
 * Returns %TRUE if multicolored cursors are supported
 * on @display. Otherwise, cursors have only a forground
 * and a background color.
 *
 * Returns: whether cursors can have multiple colors.
 *
 * Since: 2.4
 */
gboolean 
bdk_display_supports_cursor_color (BdkDisplay *display)
{
  g_return_val_if_fail (BDK_IS_DISPLAY (display), FALSE);

  return XcursorSupportsARGB (BDK_DISPLAY_XDISPLAY (display));
}

/**
 * bdk_display_get_default_cursor_size:
 * @display: a #BdkDisplay
 *
 * Returns the default size to use for cursors on @display.
 *
 * Returns: the default cursor size.
 *
 * Since: 2.4
 */
guint     
bdk_display_get_default_cursor_size (BdkDisplay *display)
{
  g_return_val_if_fail (BDK_IS_DISPLAY (display), FALSE);

  return XcursorGetDefaultSize (BDK_DISPLAY_XDISPLAY (display));
}

#else

BdkCursor *
bdk_cursor_new_from_pixbuf (BdkDisplay *display, 
			    BdkPixbuf  *pixbuf,
			    gint        x,
			    gint        y)
{
  BdkCursor *cursor;
  BdkPixmap *pixmap, *mask;
  guint width, height, n_channels, rowstride, i, j;
  guint8 *data, *mask_data, *pixels;
  BdkColor fg = { 0, 0, 0, 0 };
  BdkColor bg = { 0, 0xffff, 0xffff, 0xffff };
  BdkScreen *screen;

  g_return_val_if_fail (BDK_IS_DISPLAY (display), NULL);
  g_return_val_if_fail (BDK_IS_PIXBUF (pixbuf), NULL);

  width = bdk_pixbuf_get_width (pixbuf);
  height = bdk_pixbuf_get_height (pixbuf);

  g_return_val_if_fail (0 <= x && x < width, NULL);
  g_return_val_if_fail (0 <= y && y < height, NULL);

  n_channels = bdk_pixbuf_get_n_channels (pixbuf);
  rowstride = bdk_pixbuf_get_rowstride (pixbuf);
  pixels = bdk_pixbuf_get_pixels (pixbuf);

  data = g_new0 (guint8, (width + 7) / 8 * height);
  mask_data = g_new0 (guint8, (width + 7) / 8 * height);

  for (j = 0; j < height; j++)
    {
      guint8 *src = pixels + j * rowstride;
      guint8 *d = data + (width + 7) / 8 * j;
      guint8 *md = mask_data + (width + 7) / 8 * j;
	
      for (i = 0; i < width; i++)
	{
	  if (src[1] < 0x80)
	    *d |= 1 << (i % 8);
	  
	  if (n_channels == 3 || src[3] >= 0x80)
	    *md |= 1 << (i % 8);
	  
	  src += n_channels;
	  if (i % 8 == 7)
	    {
	      d++; 
	      md++;
	    }
	}
    }
      
  screen = bdk_display_get_default_screen (display);
  pixmap = bdk_bitmap_create_from_data (bdk_screen_get_root_window (screen), 
					data, width, height);
 
  mask = bdk_bitmap_create_from_data (bdk_screen_get_root_window (screen),
				      mask_data, width, height);
   
  cursor = bdk_cursor_new_from_pixmap (pixmap, mask, &fg, &bg, x, y);
   
  g_object_unref (pixmap);
  g_object_unref (mask);

  g_free (data);
  g_free (mask_data);
  
  return cursor;
}

BdkCursor*  
bdk_cursor_new_from_name (BdkDisplay  *display,
			  const gchar *name)
{
  g_return_val_if_fail (BDK_IS_DISPLAY (display), NULL);

  return NULL;
}

gboolean 
bdk_display_supports_cursor_alpha (BdkDisplay    *display)
{
  g_return_val_if_fail (BDK_IS_DISPLAY (display), FALSE);

  return FALSE;
}

gboolean 
bdk_display_supports_cursor_color (BdkDisplay    *display)
{
  g_return_val_if_fail (BDK_IS_DISPLAY (display), FALSE);

  return FALSE;
}

guint     
bdk_display_get_default_cursor_size (BdkDisplay    *display)
{
  g_return_val_if_fail (BDK_IS_DISPLAY (display), 0);
  
  /* no idea, really */
  return 20; 
}

#endif


/**
 * bdk_display_get_maximal_cursor_size:
 * @display: a #BdkDisplay
 * @width: (out): the return location for the maximal cursor width
 * @height: (out): the return location for the maximal cursor height
 *
 * Gets the maximal size to use for cursors on @display.
 *
 * Since: 2.4
 */
void     
bdk_display_get_maximal_cursor_size (BdkDisplay *display,
				     guint       *width,
				     guint       *height)
{
  BdkScreen *screen;
  BdkWindow *window;

  g_return_if_fail (BDK_IS_DISPLAY (display));
  
  screen = bdk_display_get_default_screen (display);
  window = bdk_screen_get_root_window (screen);
  XQueryBestCursor (BDK_DISPLAY_XDISPLAY (display), 
		    BDK_WINDOW_XWINDOW (window), 
		    128, 128, width, height);
}

#define __BDK_CURSOR_X11_C__
#include "bdkaliasdef.c"
