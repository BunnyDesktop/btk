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

#include <math.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#undef BDK_DISABLE_DEPRECATED
#undef BTK_DISABLE_DEPRECATED

#include "btkpreview.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkalias.h"


#define PREVIEW_CLASS(w)      BTK_PREVIEW_CLASS (BTK_OBJECT (w)->klass)

enum {
  PROP_0,
  PROP_EXPAND
};


static void   btk_preview_set_property  (BObject          *object,
					 guint             prop_id,
					 const BValue     *value,
					 BParamSpec       *pspec);
static void   btk_preview_get_property  (BObject          *object,
					 guint             prop_id,
					 BValue           *value,
					 BParamSpec       *pspec);
static void   btk_preview_finalize      (BObject          *object);
static void   btk_preview_realize       (BtkWidget        *widget);
static void   btk_preview_size_allocate (BtkWidget        *widget,
					 BtkAllocation    *allocation);
static gint   btk_preview_expose        (BtkWidget        *widget,
				         BdkEventExpose   *event);
static void   btk_preview_make_buffer   (BtkPreview       *preview);
static void   btk_fill_lookup_array     (guchar           *array);

static BtkPreviewClass *preview_class = NULL;
static gint install_cmap = FALSE;


G_DEFINE_TYPE (BtkPreview, btk_preview, BTK_TYPE_WIDGET)

static void
btk_preview_class_init (BtkPreviewClass *klass)
{
  BObjectClass *bobject_class = B_OBJECT_CLASS (klass);
  BtkWidgetClass *widget_class;

  widget_class = (BtkWidgetClass*) klass;

  preview_class = klass;

  bobject_class->finalize = btk_preview_finalize;

  bobject_class->set_property = btk_preview_set_property;
  bobject_class->get_property = btk_preview_get_property;

  widget_class->realize = btk_preview_realize;
  widget_class->size_allocate = btk_preview_size_allocate;
  widget_class->expose_event = btk_preview_expose;

  klass->info.lookup = NULL;

  klass->info.gamma = 1.0;

  g_object_class_install_property (bobject_class,
                                   PROP_EXPAND,
                                   g_param_spec_boolean ("expand",
							 P_("Expand"),
							 P_("Whether the preview widget should take up the entire space it is allocated"),
							 FALSE,
							 BTK_PARAM_READWRITE));
}

static void
btk_preview_set_property (BObject      *object,
			  guint         prop_id,
			  const BValue *value,
			  BParamSpec   *pspec)
{
  BtkPreview *preview = BTK_PREVIEW (object);
  
  switch (prop_id)
    {
    case PROP_EXPAND:
      btk_preview_set_expand (preview, b_value_get_boolean (value));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_preview_get_property (BObject      *object,
			  guint         prop_id,
			  BValue       *value,
			  BParamSpec   *pspec)
{
  BtkPreview *preview;
  
  preview = BTK_PREVIEW (object);
  
  switch (prop_id)
    {
    case PROP_EXPAND:
      b_value_set_boolean (value, preview->expand);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

void
btk_preview_reset (void)
{
  /* unimplemented */
}

static void
btk_preview_init (BtkPreview *preview)
{
  preview->buffer = NULL;
  preview->buffer_width = 0;
  preview->buffer_height = 0;
  preview->expand = FALSE;
}

void
btk_preview_uninit (void)
{
  /* unimplemented */
}

BtkWidget*
btk_preview_new (BtkPreviewType type)
{
  BtkPreview *preview;

  preview = btk_type_new (btk_preview_get_type ());
  preview->type = type;

  if (type == BTK_PREVIEW_COLOR)
    preview->bpp = 3;
  else
    preview->bpp = 1;

  preview->dither = BDK_RGB_DITHER_NORMAL;

  return BTK_WIDGET (preview);
}

void
btk_preview_size (BtkPreview *preview,
		  gint        width,
		  gint        height)
{
  g_return_if_fail (BTK_IS_PREVIEW (preview));

  if ((width != BTK_WIDGET (preview)->requisition.width) ||
      (height != BTK_WIDGET (preview)->requisition.height))
    {
      BTK_WIDGET (preview)->requisition.width = width;
      BTK_WIDGET (preview)->requisition.height = height;

      g_free (preview->buffer);
      preview->buffer = NULL;
    }
}

void
btk_preview_put (BtkPreview   *preview,
		 BdkWindow    *window,
		 BdkGC        *gc,
		 gint          srcx,
		 gint          srcy,
		 gint          destx,
		 gint          desty,
		 gint          width,
		 gint          height)
{
  BdkRectangle r1, r2, r3;
  guchar *src;
  guint bpp;
  guint rowstride;

  g_return_if_fail (BTK_IS_PREVIEW (preview));
  g_return_if_fail (window != NULL);

  if (!preview->buffer)
    return;

  r1.x = 0;
  r1.y = 0;
  r1.width = preview->buffer_width;
  r1.height = preview->buffer_height;

  r2.x = srcx;
  r2.y = srcy;
  r2.width = width;
  r2.height = height;

  if (!bdk_rectangle_intersect (&r1, &r2, &r3))
    return;

  bpp = preview->bpp;
  rowstride = preview->rowstride;

  src = preview->buffer + r3.y * rowstride + r3.x * bpp;

  if (preview->type == BTK_PREVIEW_COLOR)
    bdk_draw_rgb_image (window,
			gc,
			destx + (r3.x - srcx),
			desty + (r3.y - srcy),
			r3.width,
			r3.height,
			preview->dither,
			src,
			rowstride);
  else
    bdk_draw_gray_image (window,
			 gc,
			 destx + (r3.x - srcx),
			 desty + (r3.y - srcy),
			 r3.width,
			 r3.height,
			 preview->dither,
			 src,
			 rowstride);
			
}

void
btk_preview_draw_row (BtkPreview *preview,
		      guchar     *data,
		      gint        x,
		      gint        y,
		      gint        w)
{
  guint bpp;
  guint rowstride;

  g_return_if_fail (BTK_IS_PREVIEW (preview));
  g_return_if_fail (data != NULL);
  
  bpp = (preview->type == BTK_PREVIEW_COLOR ? 3 : 1);
  rowstride = (preview->buffer_width * bpp + 3) & -4;

  if ((w <= 0) || (y < 0))
    return;

  g_return_if_fail (data != NULL);

  btk_preview_make_buffer (preview);

  if (x + w > preview->buffer_width)
    return;

  if (y + 1 > preview->buffer_height)
    return;

  if (preview_class->info.gamma == 1.0)
    memcpy (preview->buffer + y * rowstride + x * bpp, data, w * bpp);
  else
    {
      guint i, size;
      guchar *src, *dst;
      guchar *lookup;

      if (preview_class->info.lookup != NULL)
	lookup = preview_class->info.lookup;
      else
	{
	  preview_class->info.lookup = g_new (guchar, 256);
	  btk_fill_lookup_array (preview_class->info.lookup);
	  lookup = preview_class->info.lookup;
	}
      size = w * bpp;
      src = data;
      dst = preview->buffer + y * rowstride + x * bpp;
      for (i = 0; i < size; i++)
	*dst++ = lookup[*src++];
    }
}

void
btk_preview_set_expand (BtkPreview *preview,
			gboolean    expand)
{
  g_return_if_fail (BTK_IS_PREVIEW (preview));

  expand = expand != FALSE;

  if (preview->expand != expand)
    {
      preview->expand = expand;
      btk_widget_queue_resize (BTK_WIDGET (preview));
 
      g_object_notify (B_OBJECT (preview), "expand"); 
    }
}

void
btk_preview_set_gamma (double _gamma)
{
  if (!preview_class)
    preview_class = btk_type_class (btk_preview_get_type ());

  if (preview_class->info.gamma != _gamma)
    {
      preview_class->info.gamma = _gamma;
      if (preview_class->info.lookup != NULL)
	{
	  g_free (preview_class->info.lookup);
	  preview_class->info.lookup = NULL;
	}
    }
}

void
btk_preview_set_color_cube (guint nred_shades,
			    guint ngreen_shades,
			    guint nblue_shades,
			    guint ngray_shades)
{
  /* unimplemented */
}

void
btk_preview_set_install_cmap (gint _install_cmap)
{
  /* effectively unimplemented */
  install_cmap = _install_cmap;
}

void
btk_preview_set_reserved (gint nreserved)
{

  /* unimplemented */
}

void
btk_preview_set_dither (BtkPreview      *preview,
			BdkRgbDither     dither)
{
  preview->dither = dither;
}

BdkVisual*
btk_preview_get_visual (void)
{
  return bdk_screen_get_rgb_visual (bdk_screen_get_default ());
}

BdkColormap*
btk_preview_get_cmap (void)
{
  return bdk_screen_get_rgb_colormap (bdk_screen_get_default ());
}

BtkPreviewInfo*
btk_preview_get_info (void)
{
  if (!preview_class)
    preview_class = btk_type_class (btk_preview_get_type ());

  return &preview_class->info;
}


static void
btk_preview_finalize (BObject *object)
{
  BtkPreview *preview = BTK_PREVIEW (object);

  g_free (preview->buffer);

  B_OBJECT_CLASS (btk_preview_parent_class)->finalize (object);
}

static void
btk_preview_realize (BtkWidget *widget)
{
  BtkPreview *preview = BTK_PREVIEW (widget);
  BdkWindowAttr attributes;
  gint attributes_mask;

  btk_widget_set_realized (widget, TRUE);

  attributes.window_type = BDK_WINDOW_CHILD;

  if (preview->expand)
    {
      attributes.width = widget->allocation.width;
      attributes.height = widget->allocation.height;
    }
  else
    {
      attributes.width = MIN (widget->requisition.width, widget->allocation.width);
      attributes.height = MIN (widget->requisition.height, widget->allocation.height);
    }

  attributes.x = widget->allocation.x + (widget->allocation.width - attributes.width) / 2;
  attributes.y = widget->allocation.y + (widget->allocation.height - attributes.height) / 2;;

  attributes.wclass = BDK_INPUT_OUTPUT;
  attributes.event_mask = btk_widget_get_events (widget) | BDK_EXPOSURE_MASK;
  attributes_mask = BDK_WA_X | BDK_WA_Y;

  widget->window = bdk_window_new (btk_widget_get_parent_window (widget), &attributes, attributes_mask);
  bdk_window_set_user_data (widget->window, widget);

  widget->style = btk_style_attach (widget->style, widget->window);
  btk_style_set_background (widget->style, widget->window, BTK_STATE_NORMAL);
}

static void   
btk_preview_size_allocate (BtkWidget        *widget,
			   BtkAllocation    *allocation)
{
  BtkPreview *preview = BTK_PREVIEW (widget);
  gint width, height;

  widget->allocation = *allocation;

  if (btk_widget_get_realized (widget))
    {
      if (preview->expand)
	{
	  width = widget->allocation.width;
	  height = widget->allocation.height;
	}
      else
	{
	  width = MIN (widget->allocation.width, widget->requisition.width);
	  height = MIN (widget->allocation.height, widget->requisition.height);
	}

      bdk_window_move_resize (widget->window,
			      widget->allocation.x + (widget->allocation.width - width) / 2,
			      widget->allocation.y + (widget->allocation.height - height) / 2,
			      width, height);
    }
}

static gint
btk_preview_expose (BtkWidget      *widget,
		    BdkEventExpose *event)
{
  BtkPreview *preview;
  gint width, height;

  if (BTK_WIDGET_DRAWABLE (widget))
    {
      preview = BTK_PREVIEW (widget);

      bdk_drawable_get_size (widget->window, &width, &height);

      btk_preview_put (BTK_PREVIEW (widget),
		       widget->window, widget->style->black_gc,
		       event->area.x - (width - preview->buffer_width)/2,
		       event->area.y - (height - preview->buffer_height)/2,
		       event->area.x, event->area.y,
		       event->area.width, event->area.height);
    }
  
  return FALSE;
}

static void
btk_preview_make_buffer (BtkPreview *preview)
{
  BtkWidget *widget;
  gint width;
  gint height;

  g_return_if_fail (BTK_IS_PREVIEW (preview));

  widget = BTK_WIDGET (preview);

  if (preview->expand &&
      (widget->allocation.width != 0) &&
      (widget->allocation.height != 0))
    {
      width = widget->allocation.width;
      height = widget->allocation.height;
    }
  else
    {
      width = widget->requisition.width;
      height = widget->requisition.height;
    }

  if (!preview->buffer ||
      (preview->buffer_width != width) ||
      (preview->buffer_height != height))
    {
      g_free (preview->buffer);

      preview->buffer_width = width;
      preview->buffer_height = height;

      preview->rowstride = (preview->buffer_width * preview->bpp + 3) & -4;
      preview->buffer = g_new0 (guchar,
				preview->buffer_height *
				preview->rowstride);
    }
}

/* This is used for implementing gamma. */
static void
btk_fill_lookup_array (guchar *array)
{
  double one_over_gamma;
  double ind;
  int val;
  int i;

  one_over_gamma = 1.0 / preview_class->info.gamma;

  for (i = 0; i < 256; i++)
    {
      ind = (double) i / 255.0;
      val = (int) (255 * pow (ind, one_over_gamma));
      array[i] = val;
    }
}

#define __BTK_PREVIEW_C__
#include "btkaliasdef.c"
