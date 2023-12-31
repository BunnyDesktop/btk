#include <bdk/bdk.h>
#include <btk/btk.h>
#include <bdkx.h>
#include <stdio.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>
#include <X11/extensions/shape.h>

#include <bdk-pixbuf/bdk-pixbuf.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <locale.h>
#include "widgets.h"
#include "shadow.h"

#define MAXIMUM_WM_REPARENTING_DEPTH 4
#ifndef _
#define _(x) (x)
#endif

static Window
find_toplevel_window (Window xid)
{
  Window root, parent, *children;
  buint nchildren;

  do
    {
      if (XQueryTree (BDK_DISPLAY_XDISPLAY (bdk_display_get_default ()), xid, &root,
		      &parent, &children, &nchildren) == 0)
	{
	  g_warning ("Couldn't find window manager window");
	  return 0;
	}

      if (root == parent)
	return xid;

      xid = parent;
    }
  while (TRUE);
}

static BdkPixbuf *
add_border_to_shot (BdkPixbuf *pixbuf)
{
  BdkPixbuf *retval;

  retval = bdk_pixbuf_new (BDK_COLORSPACE_RGB, TRUE, 8,
			   bdk_pixbuf_get_width (pixbuf) + 2,
			   bdk_pixbuf_get_height (pixbuf) + 2);

  /* Fill with solid black */
  bdk_pixbuf_fill (retval, 0xFF);
  bdk_pixbuf_copy_area (pixbuf,
			0, 0,
			bdk_pixbuf_get_width (pixbuf),
			bdk_pixbuf_get_height (pixbuf),
			retval, 1, 1);

  return retval;
}

static BdkPixbuf *
remove_shaped_area (BdkPixbuf *pixbuf,
		    Window     window)
{
  BdkPixbuf *retval;
  XRectangle *rectangles;
  int rectangle_count, rectangle_order;
  int i;

  retval = bdk_pixbuf_new (BDK_COLORSPACE_RGB, TRUE, 8,
			   bdk_pixbuf_get_width (pixbuf),
			   bdk_pixbuf_get_height (pixbuf));
  
  bdk_pixbuf_fill (retval, 0);
  rectangles = XShapeGetRectangles (BDK_DISPLAY_XDISPLAY (bdk_display_get_default ()), window,
				    ShapeBounding, &rectangle_count, &rectangle_order);

  for (i = 0; i < rectangle_count; i++)
    {
      int y, x;

      for (y = rectangles[i].y; y < rectangles[i].y + rectangles[i].height; y++)
	{
	  buchar *src_pixels, *dest_pixels;

	  src_pixels = bdk_pixbuf_get_pixels (pixbuf) +
	    y * bdk_pixbuf_get_rowstride (pixbuf) +
	    rectangles[i].x * (bdk_pixbuf_get_has_alpha (pixbuf) ? 4 : 3);
	  dest_pixels = bdk_pixbuf_get_pixels (retval) +
	    y * bdk_pixbuf_get_rowstride (retval) +
	    rectangles[i].x * 4;

	  for (x = rectangles[i].x; x < rectangles[i].x + rectangles[i].width; x++)
	    {
	      *dest_pixels++ = *src_pixels ++;
	      *dest_pixels++ = *src_pixels ++;
	      *dest_pixels++ = *src_pixels ++;
	      *dest_pixels++ = 255;

	      if (bdk_pixbuf_get_has_alpha (pixbuf))
		src_pixels++;
	    }
	}
    }

  return retval;
}

static BdkPixbuf *
take_window_shot (Window   child,
		  bboolean include_decoration)
{
  BdkWindow *window;
  Display *disp;
  Window w, xid;
  bint x_orig, y_orig;
  bint x = 0, y = 0;
  bint width, height;

  BdkPixbuf *tmp, *tmp2;
  BdkPixbuf *retval;

  disp = BDK_DISPLAY_XDISPLAY (bdk_display_get_default ());
  w = BDK_ROOT_WINDOW ();

  if (include_decoration)
    xid = find_toplevel_window (child);
  else
    xid = child;

  window = bdk_window_foreign_new (xid);

  bdk_drawable_get_size (window, &width, &height);
  bdk_window_get_origin (window, &x_orig, &y_orig);

  if (x_orig < 0)
    {
      x = - x_orig;
      width = width + x_orig;
      x_orig = 0;
    }

  if (y_orig < 0)
    {
      y = - y_orig;
      height = height + y_orig;
      y_orig = 0;
    }

  if (x_orig + width > bdk_screen_width ())
    width = bdk_screen_width () - x_orig;

  if (y_orig + height > bdk_screen_height ())
    height = bdk_screen_height () - y_orig;

  tmp = bdk_pixbuf_get_from_drawable (NULL, window, NULL,
				      x, y, 0, 0, width, height);

  if (include_decoration)
    tmp2 = remove_shaped_area (tmp, xid);
  else
    tmp2 = add_border_to_shot (tmp);

  retval = create_shadowed_pixbuf (tmp2);
  g_object_unref (tmp);
  g_object_unref (tmp2);

  return retval;
}

int main (int argc, char **argv)
{
  GList *toplevels;
  BdkPixbuf *screenshot = NULL;
  GList *node;

  /* If there's no DISPLAY, we silently error out.  We don't want to break
   * headless builds. */
  if (! btk_init_check (&argc, &argv))
    return 0;

  toplevels = get_all_widgets ();

  for (node = toplevels; node; node = g_list_next (node))
    {
      BdkWindow *window;
      WidgetInfo *info;
      XID id;
      char *filename;

      info = node->data;

      btk_widget_show (info->window);

      window = info->window->window;

      btk_widget_show_now (info->window);
      btk_widget_draw (info->window, &(info->window->allocation));

      while (btk_events_pending ())
	{
	  btk_main_iteration ();
	}
      sleep (1);

      while (btk_events_pending ())
	{
	  btk_main_iteration ();
	}

      id = bdk_x11_drawable_get_xid (BDK_DRAWABLE (window));
      screenshot = take_window_shot (id, info->include_decorations);
      filename = g_strdup_printf ("./%s.png", info->name);
      bdk_pixbuf_save (screenshot, filename, "png", NULL, NULL);
      g_free(filename);
      btk_widget_hide (info->window);
    }

  return 0;
}
