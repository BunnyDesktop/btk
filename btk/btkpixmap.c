/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * Insensitive pixmap building code by Eckehard Berns from BUNNY Stock
 * Copyright (C) 1997, 1998 Free Software Foundation
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
#include <math.h>

#undef BDK_DISABLE_DEPRECATED
#undef BTK_DISABLE_DEPRECATED
#define __BTK_PIXMAP_C__

#include "btkcontainer.h"
#include "btkpixmap.h"
#include "btkintl.h"

#include "btkalias.h"


static gint btk_pixmap_expose     (BtkWidget       *widget,
				   BdkEventExpose  *event);
static void btk_pixmap_finalize   (BObject         *object);
static void build_insensitive_pixmap (BtkPixmap *btkpixmap);

G_DEFINE_TYPE (BtkPixmap, btk_pixmap, BTK_TYPE_MISC)

static void
btk_pixmap_class_init (BtkPixmapClass *class)
{
  BObjectClass *bobject_class = B_OBJECT_CLASS (class);
  BtkWidgetClass *widget_class;

  widget_class = (BtkWidgetClass*) class;

  bobject_class->finalize = btk_pixmap_finalize;

  widget_class->expose_event = btk_pixmap_expose;
}

static void
btk_pixmap_init (BtkPixmap *pixmap)
{
  btk_widget_set_has_window (BTK_WIDGET (pixmap), FALSE);

  pixmap->pixmap = NULL;
  pixmap->mask = NULL;
}

/**
 * btk_pixmap_new:
 * @mask: (allow-none):
 */
BtkWidget*
btk_pixmap_new (BdkPixmap *val,
		BdkBitmap *mask)
{
  BtkPixmap *pixmap;
   
  g_return_val_if_fail (val != NULL, NULL);
  
  pixmap = btk_type_new (btk_pixmap_get_type ());
  
  pixmap->build_insensitive = TRUE;
  btk_pixmap_set (pixmap, val, mask);
  
  return BTK_WIDGET (pixmap);
}

static void
btk_pixmap_finalize (BObject *object)
{
  btk_pixmap_set (BTK_PIXMAP (object), NULL, NULL);

  B_OBJECT_CLASS (btk_pixmap_parent_class)->finalize (object);
}

void
btk_pixmap_set (BtkPixmap *pixmap,
		BdkPixmap *val,
		BdkBitmap *mask)
{
  gint width;
  gint height;
  gint oldwidth;
  gint oldheight;

  g_return_if_fail (BTK_IS_PIXMAP (pixmap));
  if(BDK_IS_DRAWABLE(val))
    g_return_if_fail (bdk_colormap_get_visual (btk_widget_get_colormap (BTK_WIDGET (pixmap)))->depth == bdk_drawable_get_depth (BDK_DRAWABLE (val)));

  if (pixmap->pixmap != val)
    {
      oldwidth = BTK_WIDGET (pixmap)->requisition.width;
      oldheight = BTK_WIDGET (pixmap)->requisition.height;
      if (pixmap->pixmap)
	g_object_unref (pixmap->pixmap);
      if (pixmap->pixmap_insensitive)
	g_object_unref (pixmap->pixmap_insensitive);
      pixmap->pixmap = val;
      pixmap->pixmap_insensitive = NULL;
      if (pixmap->pixmap)
	{
	  g_object_ref (pixmap->pixmap);
	  bdk_drawable_get_size (pixmap->pixmap, &width, &height);
	  BTK_WIDGET (pixmap)->requisition.width =
	    width + BTK_MISC (pixmap)->xpad * 2;
	  BTK_WIDGET (pixmap)->requisition.height =
	    height + BTK_MISC (pixmap)->ypad * 2;
	}
      else
	{
	  BTK_WIDGET (pixmap)->requisition.width = 0;
	  BTK_WIDGET (pixmap)->requisition.height = 0;
	}
      if (btk_widget_get_visible (BTK_WIDGET (pixmap)))
	{
	  if ((BTK_WIDGET (pixmap)->requisition.width != oldwidth) ||
	      (BTK_WIDGET (pixmap)->requisition.height != oldheight))
	    btk_widget_queue_resize (BTK_WIDGET (pixmap));
	  else
	    btk_widget_queue_draw (BTK_WIDGET (pixmap));
	}
    }

  if (pixmap->mask != mask)
    {
      if (pixmap->mask)
	g_object_unref (pixmap->mask);
      pixmap->mask = mask;
      if (pixmap->mask)
	g_object_ref (pixmap->mask);
    }
}

void
btk_pixmap_get (BtkPixmap  *pixmap,
		BdkPixmap **val,
		BdkBitmap **mask)
{
  g_return_if_fail (BTK_IS_PIXMAP (pixmap));

  if (val)
    *val = pixmap->pixmap;
  if (mask)
    *mask = pixmap->mask;
}

static gint
btk_pixmap_expose (BtkWidget      *widget,
		   BdkEventExpose *event)
{
  BtkPixmap *pixmap;
  BtkMisc *misc;
  gint x, y;
  gfloat xalign;

  g_return_val_if_fail (BTK_IS_PIXMAP (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  if (BTK_WIDGET_DRAWABLE (widget))
    {
      pixmap = BTK_PIXMAP (widget);
      misc = BTK_MISC (widget);

      if (btk_widget_get_direction (widget) == BTK_TEXT_DIR_LTR)
	xalign = misc->xalign;
      else
	xalign = 1.0 - misc->xalign;
  
      x = floor (widget->allocation.x + misc->xpad
		 + ((widget->allocation.width - widget->requisition.width) * xalign));
      y = floor (widget->allocation.y + misc->ypad 
		 + ((widget->allocation.height - widget->requisition.height) * misc->yalign));
      
      if (pixmap->mask)
	{
	  bdk_gc_set_clip_mask (widget->style->black_gc, pixmap->mask);
	  bdk_gc_set_clip_origin (widget->style->black_gc, x, y);
	}

      if (btk_widget_get_state (widget) == BTK_STATE_INSENSITIVE
          && pixmap->build_insensitive)
        {
	  if (!pixmap->pixmap_insensitive)
	    build_insensitive_pixmap (pixmap);
          bdk_draw_drawable (widget->window,
                             widget->style->black_gc,
                             pixmap->pixmap_insensitive,
                             0, 0, x, y, -1, -1);
        }
      else
	{
          bdk_draw_drawable (widget->window,
                             widget->style->black_gc,
                             pixmap->pixmap,
                             0, 0, x, y, -1, -1);
	}

      if (pixmap->mask)
	{
	  bdk_gc_set_clip_mask (widget->style->black_gc, NULL);
	  bdk_gc_set_clip_origin (widget->style->black_gc, 0, 0);
	}
    }
  return FALSE;
}

void
btk_pixmap_set_build_insensitive (BtkPixmap *pixmap, gboolean build)
{
  g_return_if_fail (BTK_IS_PIXMAP (pixmap));

  pixmap->build_insensitive = build;

  if (btk_widget_get_visible (BTK_WIDGET (pixmap)))
    {
      btk_widget_queue_draw (BTK_WIDGET (pixmap));
    }
}

static void
build_insensitive_pixmap (BtkPixmap *btkpixmap)
{
  BdkPixmap *pixmap = btkpixmap->pixmap;
  BdkPixmap *insensitive;
  gint w, h;
  BdkPixbuf *pixbuf;
  BdkPixbuf *stated;
  
  bdk_drawable_get_size (pixmap, &w, &h);

  pixbuf = bdk_pixbuf_get_from_drawable (NULL,
                                         pixmap,
                                         btk_widget_get_colormap (BTK_WIDGET (btkpixmap)),
                                         0, 0,
                                         0, 0,
                                         w, h);
  
  stated = bdk_pixbuf_copy (pixbuf);
  
  bdk_pixbuf_saturate_and_pixelate (pixbuf, stated,
                                    0.8, TRUE);

  g_object_unref (pixbuf);
  pixbuf = NULL;
  
  insensitive = bdk_pixmap_new (BTK_WIDGET (btkpixmap)->window, w, h, -1);

  bdk_draw_pixbuf (insensitive,
		   BTK_WIDGET (btkpixmap)->style->white_gc,
		   stated,
		   0, 0,
		   0, 0,
		   w, h,
		   BDK_RGB_DITHER_NORMAL,
		   0, 0);

  btkpixmap->pixmap_insensitive = insensitive;

  g_object_unref (stated);
}

#include "btkaliasdef.c"
