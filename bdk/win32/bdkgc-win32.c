/* BDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 1998-2004 Tor Lillqvist
 * Copyright (C) 2000-2004 Hans Breuer
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

#define LINE_ATTRIBUTES (BDK_GC_LINE_WIDTH|BDK_GC_LINE_STYLE| \
			 BDK_GC_CAP_STYLE|BDK_GC_JOIN_STYLE)

#include "config.h"
#include <string.h>

#include "bdkgc.h"
#include "bdkfont.h"
#include "bdkpixmap.h"
#include "bdkrebunnyion-generic.h"
#include "bdkprivate-win32.h"

static void bdk_win32_gc_get_values (BdkGC           *gc,
				     BdkGCValues     *values);
static void bdk_win32_gc_set_values (BdkGC           *gc,
				     BdkGCValues     *values,
				     BdkGCValuesMask  values_mask);
static void bdk_win32_gc_set_dashes (BdkGC           *gc,
				     gint             dash_offset,
				     gint8            dash_list[],
				     gint             n);

static void bdk_gc_win32_class_init (BdkGCWin32Class *klass);
static void bdk_gc_win32_finalize   (BObject         *object);

static gpointer parent_class = NULL;

GType
_bdk_gc_win32_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      const GTypeInfo object_info =
      {
        sizeof (BdkGCWin32Class),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) bdk_gc_win32_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (BdkGCWin32),
        0,              /* n_preallocs */
        (GInstanceInitFunc) NULL,
      };
      
      object_type = g_type_register_static (BDK_TYPE_GC,
                                            "BdkGCWin32",
                                            &object_info, 0);
    }
  
  return object_type;
}

static void
bdk_gc_win32_class_init (BdkGCWin32Class *klass)
{
  BObjectClass *object_class = B_OBJECT_CLASS (klass);
  BdkGCClass *gc_class = BDK_GC_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = bdk_gc_win32_finalize;

  gc_class->get_values = bdk_win32_gc_get_values;
  gc_class->set_values = bdk_win32_gc_set_values;
  gc_class->set_dashes = bdk_win32_gc_set_dashes;
}

static void
bdk_gc_win32_finalize (BObject *object)
{
  BdkGCWin32 *win32_gc = BDK_GC_WIN32 (object);
  
  if (win32_gc->hcliprgn != NULL)
    DeleteObject (win32_gc->hcliprgn);
  
  if (win32_gc->values_mask & BDK_GC_FONT)
    bdk_font_unref (win32_gc->font);
  
  g_free (win32_gc->pen_dashes);
  
  B_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
fixup_pen (BdkGCWin32 *win32_gc)
{
  win32_gc->pen_style = 0;

  /* First look at BDK width and end cap style, set GDI pen type and
   * end cap.
   */
  if (win32_gc->pen_width == 0 &&
      win32_gc->cap_style == BDK_CAP_NOT_LAST)
    {
      /* Use a cosmetic pen, always width 1 */
      win32_gc->pen_style |= PS_COSMETIC;
    }
  else if (win32_gc->pen_width <= 1 &&
	   win32_gc->cap_style == BDK_CAP_BUTT)
    {
      /* For 1 pixel wide lines PS_ENDCAP_ROUND means draw both ends,
       * even for one pixel length lines. But if we are drawing dashed
       * lines we can't use PS_ENDCAP_ROUND.
       */
      if (win32_gc->line_style == BDK_LINE_SOLID)
	win32_gc->pen_style |= PS_GEOMETRIC | PS_ENDCAP_ROUND;
      else
	win32_gc->pen_style |= PS_GEOMETRIC | PS_ENDCAP_FLAT;
    }
  else
    {
      win32_gc->pen_style |= PS_GEOMETRIC;
      switch (win32_gc->cap_style)
	{
	/* For non-zero-width lines X11's CapNotLast works like CapButt */
	case BDK_CAP_NOT_LAST:
	case BDK_CAP_BUTT:
	  win32_gc->pen_style |= PS_ENDCAP_FLAT;
	  break;
	case BDK_CAP_ROUND:
	  win32_gc->pen_style |= PS_ENDCAP_ROUND;
	  break;
	case BDK_CAP_PROJECTING:
	  win32_gc->pen_style |= PS_ENDCAP_SQUARE;
	  break;
	}
    }

  /* Next look at BDK line style, set GDI pen style attribute */
  switch (win32_gc->line_style)
    {
    case BDK_LINE_SOLID:
      win32_gc->pen_style |= PS_SOLID;
      break;
    case BDK_LINE_ON_OFF_DASH:
    case BDK_LINE_DOUBLE_DASH:
      if (win32_gc->pen_dashes == NULL)
	{
	  win32_gc->pen_dashes = g_new (DWORD, 1);
	  win32_gc->pen_dashes[0] = 4;
	  win32_gc->pen_num_dashes = 1;
	}

      if (!(win32_gc->pen_style & PS_TYPE_MASK) == PS_GEOMETRIC &&
	  win32_gc->pen_dashes[0] == 1 &&
	  (win32_gc->pen_num_dashes == 1 ||
	   (win32_gc->pen_num_dashes == 2 && win32_gc->pen_dashes[0] == 1)))
	win32_gc->pen_style |= PS_ALTERNATE;
      else
	win32_gc->pen_style |= PS_USERSTYLE;
     break;
    }

  /* Last, for if the GDI pen is geometric, set the join attribute */
  if ((win32_gc->pen_style & PS_TYPE_MASK) == PS_GEOMETRIC)
    {
      switch (win32_gc->join_style)
	{
	case BDK_JOIN_MITER:
	  win32_gc->pen_style |= PS_JOIN_MITER;
	  break;
	case BDK_JOIN_ROUND:
	  win32_gc->pen_style |= PS_JOIN_ROUND;
	  break;
	case BDK_JOIN_BEVEL:
	  win32_gc->pen_style |= PS_JOIN_BEVEL;
	  break;
	}
    }
}

static void
bdk_win32_gc_values_to_win32values (BdkGCValues    *values,
				    BdkGCValuesMask mask,
				    BdkGCWin32     *win32_gc)
{				    
#ifdef G_ENABLE_DEBUG
  char *s = "";
#endif

  BDK_NOTE (GC, g_print ("{"));

  if (mask & BDK_GC_FOREGROUND)
    {
      win32_gc->values_mask |= BDK_GC_FOREGROUND;
      BDK_NOTE (GC, (g_print ("fg=%.06x",
			      _bdk_gc_get_fg_pixel (&win32_gc->parent_instance)),
		     s = ","));
    }
  
  if (mask & BDK_GC_BACKGROUND)
    {
      win32_gc->values_mask |= BDK_GC_BACKGROUND;
      BDK_NOTE (GC, (g_print ("%sbg=%.06x", s,
			      _bdk_gc_get_bg_pixel (&win32_gc->parent_instance)),
		     s = ","));
    }

  if ((mask & BDK_GC_FONT) && (values->font->type == BDK_FONT_FONT
			       || values->font->type == BDK_FONT_FONTSET))
    {
      if (win32_gc->font != NULL)
	bdk_font_unref (win32_gc->font);
      win32_gc->font = values->font;
      if (win32_gc->font != NULL)
	{
	  bdk_font_ref (win32_gc->font);
	  win32_gc->values_mask |= BDK_GC_FONT;
	  BDK_NOTE (GC, (g_print ("%sfont=%p", s, win32_gc->font),
			 s = ","));
	}
      else
	{
	  win32_gc->values_mask &= ~BDK_GC_FONT;
	  BDK_NOTE (GC, (g_print ("%sfont=NULL", s),
			 s = ","));
	}
    }

  if (mask & BDK_GC_FUNCTION)
    {
      BDK_NOTE (GC, (g_print ("%srop2=", s),
		     s = ","));
      switch (values->function)
	{
#define CASE(x,y) case BDK_##x: win32_gc->rop2 = R2_##y; BDK_NOTE (GC, g_print (#y)); break
	CASE (COPY, COPYPEN);
	CASE (INVERT, NOT);
	CASE (XOR, XORPEN);
	CASE (CLEAR, BLACK);
	CASE (AND, MASKPEN);
	CASE (AND_REVERSE, MASKPENNOT);
	CASE (AND_INVERT, MASKNOTPEN);
	CASE (NOOP, NOP);
	CASE (OR, MERGEPEN);
	CASE (EQUIV, NOTXORPEN);
	CASE (OR_REVERSE, MERGEPENNOT);
	CASE (COPY_INVERT, NOTCOPYPEN);
	CASE (OR_INVERT, MERGENOTPEN);
	CASE (NAND, NOTMASKPEN);
	CASE (NOR, NOTMERGEPEN);
	CASE (SET, WHITE);
#undef CASE
	}
      win32_gc->values_mask |= BDK_GC_FUNCTION;
    }

  if (mask & BDK_GC_FILL)
    {
      win32_gc->values_mask |= BDK_GC_FILL;
      BDK_NOTE (GC, (g_print ("%sfill=%s", s,
			      _bdk_win32_fill_style_to_string (values->fill)),
		     s = ","));
    }

  if (mask & BDK_GC_TILE)
    {
      if (values->tile != NULL)
	{
	  win32_gc->values_mask |= BDK_GC_TILE;
	  BDK_NOTE (GC,
		    (g_print ("%stile=%p", s,
			      BDK_PIXMAP_HBITMAP (values->tile)),
		     s = ","));
	}
      else
	{
	  win32_gc->values_mask &= ~BDK_GC_TILE;
	  BDK_NOTE (GC, (g_print ("%stile=NULL", s),
			 s = ","));
	}
    }

  if (mask & BDK_GC_STIPPLE)
    {
      if (values->stipple != NULL)
	{
	  win32_gc->values_mask |= BDK_GC_STIPPLE;
	  BDK_NOTE (GC,
		    (g_print ("%sstipple=%p", s,
			      BDK_PIXMAP_HBITMAP (values->stipple)),
		     s = ","));
	}
      else
	{
	  win32_gc->values_mask &= ~BDK_GC_STIPPLE;
	  BDK_NOTE (GC, (g_print ("%sstipple=NULL", s),
			 s = ","));
	}
    }

  if (mask & BDK_GC_CLIP_MASK)
    {
      if (win32_gc->hcliprgn != NULL)
	DeleteObject (win32_gc->hcliprgn);

      if (values->clip_mask != NULL)
	{
	  win32_gc->hcliprgn = _bdk_win32_bitmap_to_hrgn (values->clip_mask);
	  win32_gc->values_mask |= BDK_GC_CLIP_MASK;
	}
      else
	{
	  win32_gc->hcliprgn = NULL;
	  win32_gc->values_mask &= ~BDK_GC_CLIP_MASK;
	}
      BDK_NOTE (GC, (g_print ("%sclip=%p", s, win32_gc->hcliprgn),
		     s = ","));
    }

  if (mask & BDK_GC_SUBWINDOW)
    {
      win32_gc->subwindow_mode = values->subwindow_mode;
      win32_gc->values_mask |= BDK_GC_SUBWINDOW;
      BDK_NOTE (GC, (g_print ("%ssubw=%d", s, win32_gc->subwindow_mode),
		     s = ","));
    }

  if (mask & BDK_GC_TS_X_ORIGIN)
    {
      win32_gc->values_mask |= BDK_GC_TS_X_ORIGIN;
      BDK_NOTE (GC, (g_print ("%sts_x=%d", s, values->ts_x_origin),
		     s = ","));
    }

  if (mask & BDK_GC_TS_Y_ORIGIN)
    {
      win32_gc->values_mask |= BDK_GC_TS_Y_ORIGIN;
      BDK_NOTE (GC, (g_print ("%sts_y=%d", s, values->ts_y_origin),
		     s = ","));
    }

  if (mask & BDK_GC_CLIP_X_ORIGIN)
    {
      win32_gc->values_mask |= BDK_GC_CLIP_X_ORIGIN;
      BDK_NOTE (GC, (g_print ("%sclip_x=%d", s, values->clip_x_origin),
		     s = ","));
    }

  if (mask & BDK_GC_CLIP_Y_ORIGIN)
    {
      win32_gc->values_mask |= BDK_GC_CLIP_Y_ORIGIN;
      BDK_NOTE (GC, (g_print ("%sclip_y=%d", s, values->clip_y_origin),
		     s = ","));
    }

  if (mask & BDK_GC_EXPOSURES)
    {
      win32_gc->graphics_exposures = values->graphics_exposures;
      win32_gc->values_mask |= BDK_GC_EXPOSURES;
      BDK_NOTE (GC, (g_print ("%sexp=%d", s, win32_gc->graphics_exposures),
		     s = ","));
    }

  if (mask & BDK_GC_LINE_WIDTH)
    {
      win32_gc->pen_width = values->line_width;
      win32_gc->values_mask |= BDK_GC_LINE_WIDTH;
      BDK_NOTE (GC, (g_print ("%spw=%d", s, win32_gc->pen_width),
		     s = ","));
    }

  if (mask & BDK_GC_LINE_STYLE)
    {
      win32_gc->line_style = values->line_style;
      win32_gc->values_mask |= BDK_GC_LINE_STYLE;
    }

  if (mask & BDK_GC_CAP_STYLE)
    {
      win32_gc->cap_style = values->cap_style;
      win32_gc->values_mask |= BDK_GC_CAP_STYLE;
    }

  if (mask & BDK_GC_JOIN_STYLE)
    {
      win32_gc->join_style = values->join_style;
      win32_gc->values_mask |= BDK_GC_JOIN_STYLE;
    }

  if (mask & (BDK_GC_LINE_WIDTH|BDK_GC_LINE_STYLE|BDK_GC_CAP_STYLE|BDK_GC_JOIN_STYLE))
    {
      fixup_pen (win32_gc);
      BDK_NOTE (GC, (g_print ("%sps|=PS_STYLE_%s|PS_ENDCAP_%s|PS_JOIN_%s", s,
			      _bdk_win32_psstyle_to_string (win32_gc->pen_style),
			      _bdk_win32_psendcap_to_string (win32_gc->pen_style),
			      _bdk_win32_psjoin_to_string (win32_gc->pen_style)),
		     s = ","));
    }

  BDK_NOTE (GC, g_print ("} mask=(%s)", _bdk_win32_gcvalues_mask_to_string (win32_gc->values_mask)));
}

BdkGC*
_bdk_win32_gc_new (BdkDrawable	  *drawable,
		   BdkGCValues	  *values,
		   BdkGCValuesMask values_mask)
{
  BdkGC *gc;
  BdkGCWin32 *win32_gc;

  /* NOTICE that the drawable here has to be the impl drawable,
   * not the publically-visible drawables.
   */
  g_return_val_if_fail (BDK_IS_DRAWABLE_IMPL_WIN32 (drawable), NULL);

  gc = g_object_new (_bdk_gc_win32_get_type (), NULL);
  win32_gc = BDK_GC_WIN32 (gc);

  _bdk_gc_init (gc, drawable, values, values_mask);

  win32_gc->hcliprgn = NULL;

  win32_gc->font = NULL;
  win32_gc->rop2 = R2_COPYPEN;
  win32_gc->subwindow_mode = BDK_CLIP_BY_CHILDREN;
  win32_gc->graphics_exposures = TRUE;
  win32_gc->pen_width = 0;
  /* Don't get confused by the PS_ENDCAP_ROUND. For narrow GDI pens
   * (width == 1), PS_GEOMETRIC|PS_ENDCAP_ROUND works like X11's
   * CapButt.
   */
  win32_gc->pen_style = PS_GEOMETRIC|PS_ENDCAP_ROUND|PS_JOIN_MITER;
  win32_gc->line_style = BDK_LINE_SOLID;
  win32_gc->cap_style = BDK_CAP_BUTT;
  win32_gc->join_style = BDK_JOIN_MITER;
  win32_gc->pen_dashes = NULL;
  win32_gc->pen_num_dashes = 0;
  win32_gc->pen_dash_offset = 0;
  win32_gc->pen_hbrbg = NULL;

  win32_gc->values_mask = BDK_GC_FUNCTION | BDK_GC_FILL;

  BDK_NOTE (GC, g_print ("_bdk_win32_gc_new: %p: ", win32_gc));
  bdk_win32_gc_values_to_win32values (values, values_mask, win32_gc);
  BDK_NOTE (GC, g_print ("\n"));

  win32_gc->hdc = NULL;

  return gc;
}

static void
bdk_win32_gc_get_values (BdkGC       *gc,
			 BdkGCValues *values)
{
  BdkGCWin32 *win32_gc = BDK_GC_WIN32 (gc);

  values->foreground.pixel = _bdk_gc_get_fg_pixel (gc);
  values->background.pixel = _bdk_gc_get_bg_pixel (gc);
  values->font = win32_gc->font;

  switch (win32_gc->rop2)
    {
    case R2_COPYPEN:
      values->function = BDK_COPY; break;
    case R2_NOT:
      values->function = BDK_INVERT; break;
    case R2_XORPEN:
      values->function = BDK_XOR; break;
    case R2_BLACK:
      values->function = BDK_CLEAR; break;
    case R2_MASKPEN:
      values->function = BDK_AND; break;
    case R2_MASKPENNOT:
      values->function = BDK_AND_REVERSE; break;
    case R2_MASKNOTPEN:
      values->function = BDK_AND_INVERT; break;
    case R2_NOP:
      values->function = BDK_NOOP; break;
    case R2_MERGEPEN:
      values->function = BDK_OR; break;
    case R2_NOTXORPEN:
      values->function = BDK_EQUIV; break;
    case R2_MERGEPENNOT:
      values->function = BDK_OR_REVERSE; break;
    case R2_NOTCOPYPEN:
      values->function = BDK_COPY_INVERT; break;
    case R2_MERGENOTPEN:
      values->function = BDK_OR_INVERT; break;
    case R2_NOTMASKPEN:
      values->function = BDK_NAND; break;
    case R2_NOTMERGEPEN:
      values->function = BDK_NOR; break;
    case R2_WHITE:
      values->function = BDK_SET; break;
    }

  values->fill = _bdk_gc_get_fill (gc);
  values->tile = _bdk_gc_get_tile (gc);
  values->stipple = _bdk_gc_get_stipple (gc);

  /* Also the X11 backend always returns a NULL clip_mask */
  values->clip_mask = NULL;

  values->subwindow_mode = win32_gc->subwindow_mode;
  values->ts_x_origin = win32_gc->parent_instance.ts_x_origin;
  values->ts_y_origin = win32_gc->parent_instance.ts_y_origin;
  values->clip_x_origin = win32_gc->parent_instance.clip_x_origin;
  values->clip_y_origin = win32_gc->parent_instance.clip_y_origin;
  values->graphics_exposures = win32_gc->graphics_exposures;
  values->line_width = win32_gc->pen_width;
  
  values->line_style = win32_gc->line_style;
  values->cap_style = win32_gc->cap_style;
  values->join_style = win32_gc->join_style;
}

static void
bdk_win32_gc_set_values (BdkGC           *gc,
			 BdkGCValues     *values,
			 BdkGCValuesMask  mask)
{
  g_return_if_fail (BDK_IS_GC (gc));

  BDK_NOTE (GC, g_print ("bdk_win32_gc_set_values: %p: ", BDK_GC_WIN32 (gc)));
  bdk_win32_gc_values_to_win32values (values, mask, BDK_GC_WIN32 (gc));
  BDK_NOTE (GC, g_print ("\n"));
}

static void
bdk_win32_gc_set_dashes (BdkGC *gc,
			 gint	dash_offset,
			 gint8  dash_list[],
			 gint   n)
{
  BdkGCWin32 *win32_gc;
  int i;

  g_return_if_fail (BDK_IS_GC (gc));
  g_return_if_fail (dash_list != NULL);

  win32_gc = BDK_GC_WIN32 (gc);

  win32_gc->pen_num_dashes = n;
  g_free (win32_gc->pen_dashes);
  win32_gc->pen_dashes = g_new (DWORD, n);
  for (i = 0; i < n; i++)
    win32_gc->pen_dashes[i] = dash_list[i];
  win32_gc->pen_dash_offset = dash_offset;
  fixup_pen (win32_gc);
}

void
_bdk_windowing_gc_set_clip_rebunnyion (BdkGC           *gc,
                                   const BdkRebunnyion *rebunnyion,
				   gboolean         reset_origin)
{
  BdkGCWin32 *win32_gc = BDK_GC_WIN32 (gc);

  if (win32_gc->hcliprgn)
    DeleteObject (win32_gc->hcliprgn);

  if (rebunnyion)
    {
      BDK_NOTE (GC, g_print ("bdk_gc_set_clip_rebunnyion: %p: %s\n",
			     win32_gc,
			     _bdk_win32_bdkrebunnyion_to_string (rebunnyion)));

      win32_gc->hcliprgn = _bdk_win32_bdkrebunnyion_to_hrgn (rebunnyion, 0, 0);
      win32_gc->values_mask |= BDK_GC_CLIP_MASK;
    }
  else
    {
      BDK_NOTE (GC, g_print ("bdk_gc_set_clip_rebunnyion: NULL\n"));

      win32_gc->hcliprgn = NULL;
      win32_gc->values_mask &= ~BDK_GC_CLIP_MASK;
    }

  if (reset_origin)
    {
      gc->clip_x_origin = 0;
      gc->clip_y_origin = 0;
      win32_gc->values_mask &= ~(BDK_GC_CLIP_X_ORIGIN | BDK_GC_CLIP_Y_ORIGIN);
    }
}

void
_bdk_windowing_gc_copy (BdkGC *dst_gc,
			BdkGC *src_gc)
{
  BdkGCWin32 *dst_win32_gc = BDK_GC_WIN32 (dst_gc);
  BdkGCWin32 *src_win32_gc = BDK_GC_WIN32 (src_gc);

  BDK_NOTE (GC, g_print ("bdk_gc_copy: %p := %p\n", dst_win32_gc, src_win32_gc));

  if (dst_win32_gc->hcliprgn != NULL)
    DeleteObject (dst_win32_gc->hcliprgn);

  if (dst_win32_gc->font != NULL)
    bdk_font_unref (dst_win32_gc->font);

  g_free (dst_win32_gc->pen_dashes);
  
  dst_win32_gc->hcliprgn = src_win32_gc->hcliprgn;
  if (dst_win32_gc->hcliprgn)
    {
      /* create a new rebunnyion, to copy to */
      dst_win32_gc->hcliprgn = CreateRectRgn (0,0,1,1);
      /* overwrite from source */
      CombineRgn (dst_win32_gc->hcliprgn, src_win32_gc->hcliprgn,
		  NULL, RGN_COPY);
    }

  dst_win32_gc->values_mask = src_win32_gc->values_mask; 
  dst_win32_gc->font = src_win32_gc->font;
  if (dst_win32_gc->font != NULL)
    bdk_font_ref (dst_win32_gc->font);

  dst_win32_gc->rop2 = src_win32_gc->rop2;

  dst_win32_gc->subwindow_mode = src_win32_gc->subwindow_mode;
  dst_win32_gc->graphics_exposures = src_win32_gc->graphics_exposures;
  dst_win32_gc->pen_width = src_win32_gc->pen_width;
  dst_win32_gc->pen_style = src_win32_gc->pen_style;
  dst_win32_gc->line_style = src_win32_gc->line_style;
  dst_win32_gc->cap_style = src_win32_gc->cap_style;
  dst_win32_gc->join_style = src_win32_gc->join_style;
  if (src_win32_gc->pen_dashes)
    dst_win32_gc->pen_dashes = g_memdup (src_win32_gc->pen_dashes, 
                                         sizeof (DWORD) * src_win32_gc->pen_num_dashes);
  else
    dst_win32_gc->pen_dashes = NULL;
  dst_win32_gc->pen_num_dashes = src_win32_gc->pen_num_dashes;
  dst_win32_gc->pen_dash_offset = src_win32_gc->pen_dash_offset;


  dst_win32_gc->hdc = NULL;
  dst_win32_gc->saved_dc = FALSE;
  dst_win32_gc->holdpal = NULL;
  dst_win32_gc->pen_hbrbg = NULL;
}

BdkScreen *  
bdk_gc_get_screen (BdkGC *gc)
{
  g_return_val_if_fail (BDK_IS_GC_WIN32 (gc), NULL);
  
  return _bdk_screen;
}

static guint bitmask[9] = { 0, 1, 3, 7, 15, 31, 63, 127, 255 };

COLORREF
_bdk_win32_colormap_color (BdkColormap *colormap,
                           gulong       pixel)
{
  const BdkVisual *visual;
  BdkColormapPrivateWin32 *colormap_private;
  guchar r, g, b;

  if (colormap == NULL)
    return DIBINDEX (pixel & 1);

  colormap_private = BDK_WIN32_COLORMAP_DATA (colormap);

  g_assert (colormap_private != NULL);

  visual = colormap->visual;
  switch (visual->type)
    {
    case BDK_VISUAL_GRAYSCALE:
    case BDK_VISUAL_PSEUDO_COLOR:
    case BDK_VISUAL_STATIC_COLOR:
      return PALETTEINDEX (pixel);

    case BDK_VISUAL_TRUE_COLOR:
      r = (pixel & visual->red_mask) >> visual->red_shift;
      r = (r * 255) / bitmask[visual->red_prec];
      g = (pixel & visual->green_mask) >> visual->green_shift;
      g = (g * 255) / bitmask[visual->green_prec];
      b = (pixel & visual->blue_mask) >> visual->blue_shift;
      b = (b * 255) / bitmask[visual->blue_prec];
      return RGB (r, g, b);

    default:
      g_assert_not_reached ();
      return 0;
    }
}

gboolean
predraw (BdkGC       *gc,
	 BdkColormap *colormap)
{
  BdkGCWin32 *win32_gc = (BdkGCWin32 *) gc;
  BdkColormapPrivateWin32 *colormap_private;
  gint k;
  gboolean ok = TRUE;

  if (colormap &&
      (colormap->visual->type == BDK_VISUAL_PSEUDO_COLOR ||
       colormap->visual->type == BDK_VISUAL_STATIC_COLOR))
    {
      colormap_private = BDK_WIN32_COLORMAP_DATA (colormap);

      g_assert (colormap_private != NULL);

      if (!(win32_gc->holdpal = SelectPalette (win32_gc->hdc, colormap_private->hpal, FALSE)))
	WIN32_GDI_FAILED ("SelectPalette"), ok = FALSE;
      else if ((k = RealizePalette (win32_gc->hdc)) == GDI_ERROR)
	WIN32_GDI_FAILED ("RealizePalette"), ok = FALSE;
      else if (k > 0)
	BDK_NOTE (COLORMAP, g_print ("predraw: realized %p: %d colors\n",
				     colormap_private->hpal, k));
    }

  return ok;
}

static BdkDrawableImplWin32 *
get_impl_drawable (BdkDrawable *drawable)
{
  if (BDK_IS_OFFSCREEN_WINDOW (drawable))
    return _bdk_offscreen_window_get_real_drawable (BDK_OFFSCREEN_WINDOW (drawable));
  if (BDK_IS_DRAWABLE_IMPL_WIN32 (drawable))
    return BDK_DRAWABLE_IMPL_WIN32(drawable);
  else if (BDK_IS_WINDOW (drawable))
    return BDK_DRAWABLE_IMPL_WIN32 ((BDK_WINDOW_OBJECT (drawable))->impl);
  else if (BDK_IS_PIXMAP (drawable))
    return BDK_DRAWABLE_IMPL_WIN32 ((BDK_PIXMAP_OBJECT (drawable))->impl);
  else
    g_assert_not_reached ();

  return NULL;
}

/**
 * bdk_win32_hdc_get:
 * @drawable: destination #BdkDrawable
 * @gc: #BdkGC to use for drawing on @drawable
 * @usage: mask indicating what properties needs to be set up
 *
 * Allocates a Windows device context handle (HDC) for drawing into
 * @drawable, and sets it up appropriately according to @usage.
 *
 * Each #BdkGC can at one time have only one HDC associated with it.
 *
 * The following flags in @mask are handled:
 *
 * If %BDK_GC_FOREGROUND is set in @mask, a solid brush of the
 * foreground color in @gc is selected into the HDC. The text color of
 * the HDC is also set. If the @drawable has a palette (256-color
 * mode), the palette is selected and realized.
 *
 * If any of the line attribute flags (%BDK_GC_LINE_WIDTH,
 * %BDK_GC_LINE_STYLE, %BDK_GC_CAP_STYLE and %BDK_GC_JOIN_STYLE) is
 * set in @mask, a solid pen of the foreground color and appropriate
 * width and stule is created and selected into the HDC. Note that the
 * dash properties are not completely implemented.
 *
 * If the %BDK_GC_FONT flag is set, the background mix mode is set to
 * %TRANSPARENT. and the text alignment is set to
 * %TA_BASELINE|%TA_LEFT. Note that no font gets selected into the HDC
 * by this function.
 *
 * Some things are done regardless of @mask: If the function in @gc is
 * any other than %BDK_COPY, the raster operation of the HDC is
 * set. If @gc has a clip mask, the clip rebunnyion of the HDC is set.
 *
 * Note that the fill style, tile, stipple, and tile and stipple
 * origins in the @gc are ignored by this function. (In general, tiles
 * and stipples can't be implemented directly on Win32; you need to do
 * multiple pass drawing and blitting to implement tiles or
 * stipples. BDK does just that when you call the BDK drawing
 * functions with a GC that asks for tiles or stipples.)
 *
 * When the HDC is no longer used, it should be released by calling
 * <function>bdk_win32_hdc_release()</function> with the same
 * parameters.
 *
 * If you modify the HDC by calling <function>SelectObject</function>
 * you should undo those modifications before calling
 * <function>bdk_win32_hdc_release()</function>.
 *
 * Return value: The HDC.
 **/
HDC
bdk_win32_hdc_get (BdkDrawable    *drawable,
		   BdkGC          *gc,
		   BdkGCValuesMask usage)
{
  BdkGCWin32 *win32_gc = (BdkGCWin32 *) gc;
  BdkDrawableImplWin32 *impl = NULL;
  gboolean ok = TRUE;
  COLORREF fg = RGB (0, 0, 0), bg = RGB (255, 255, 255);
  HPEN hpen;
  HBRUSH hbr;

  g_assert (win32_gc->hdc == NULL);

  impl = get_impl_drawable (drawable);
  
  win32_gc->hdc = _bdk_win32_drawable_acquire_dc (BDK_DRAWABLE (impl));
  ok = win32_gc->hdc != NULL;

  if (ok && (win32_gc->saved_dc = SaveDC (win32_gc->hdc)) == 0)
    WIN32_GDI_FAILED ("SaveDC"), ok = FALSE;
      
  if (ok && (usage & (BDK_GC_FOREGROUND | BDK_GC_BACKGROUND)))
      ok = predraw (gc, impl->colormap);

  if (ok && (usage & BDK_GC_FOREGROUND))
    {
      fg = _bdk_win32_colormap_color (impl->colormap, _bdk_gc_get_fg_pixel (gc));
      if ((hbr = CreateSolidBrush (fg)) == NULL)
	WIN32_GDI_FAILED ("CreateSolidBrush"), ok = FALSE;

      if (ok && SelectObject (win32_gc->hdc, hbr) == NULL)
	WIN32_GDI_FAILED ("SelectObject"), ok = FALSE;

      if (ok && SetTextColor (win32_gc->hdc, fg) == CLR_INVALID)
	WIN32_GDI_FAILED ("SetTextColor"), ok = FALSE;
    }

  if (ok && (usage & LINE_ATTRIBUTES))
    {
      /* For drawing BDK_LINE_DOUBLE_DASH */
      if ((usage & BDK_GC_BACKGROUND) && win32_gc->line_style == BDK_LINE_DOUBLE_DASH)
        {
          bg = _bdk_win32_colormap_color (impl->colormap, _bdk_gc_get_bg_pixel (gc));
          if ((win32_gc->pen_hbrbg = CreateSolidBrush (bg)) == NULL)
	    WIN32_GDI_FAILED ("CreateSolidBrush"), ok = FALSE;
        }

      if (ok)
        {
	  LOGBRUSH logbrush;
	  DWORD style_count = 0;
	  const DWORD *style = NULL;

	  /* Create and select pen */
	  logbrush.lbStyle = BS_SOLID;
	  logbrush.lbColor = fg;
	  logbrush.lbHatch = 0;

	  if ((win32_gc->pen_style & PS_STYLE_MASK) == PS_USERSTYLE)
	    {
	      style_count = win32_gc->pen_num_dashes;
	      style = win32_gc->pen_dashes;
	    }

	  if ((hpen = ExtCreatePen (win32_gc->pen_style,
				    MAX (win32_gc->pen_width, 1),
				    &logbrush, 
				    style_count, style)) == NULL)
	    WIN32_GDI_FAILED ("ExtCreatePen"), ok = FALSE;
	  
	  if (ok && SelectObject (win32_gc->hdc, hpen) == NULL)
	    WIN32_GDI_FAILED ("SelectObject"), ok = FALSE;
	}
    }

  if (ok && (usage & BDK_GC_FONT))
    {
      if (SetBkMode (win32_gc->hdc, TRANSPARENT) == 0)
	WIN32_GDI_FAILED ("SetBkMode"), ok = FALSE;
  
      if (ok && SetTextAlign (win32_gc->hdc, TA_BASELINE|TA_LEFT|TA_NOUPDATECP) == GDI_ERROR)
	WIN32_GDI_FAILED ("SetTextAlign"), ok = FALSE;
    }
  
  if (ok && win32_gc->rop2 != R2_COPYPEN)
    if (SetROP2 (win32_gc->hdc, win32_gc->rop2) == 0)
      WIN32_GDI_FAILED ("SetROP2"), ok = FALSE;

  if (ok &&
      (win32_gc->values_mask & BDK_GC_CLIP_MASK) &&
      win32_gc->hcliprgn != NULL)
    {
      if (SelectClipRgn (win32_gc->hdc, win32_gc->hcliprgn) == ERROR)
	WIN32_API_FAILED ("SelectClipRgn"), ok = FALSE;

      if (ok && win32_gc->values_mask & (BDK_GC_CLIP_X_ORIGIN | BDK_GC_CLIP_Y_ORIGIN) &&
	  OffsetClipRgn (win32_gc->hdc,
	    win32_gc->values_mask & BDK_GC_CLIP_X_ORIGIN ? gc->clip_x_origin : 0,
	    win32_gc->values_mask & BDK_GC_CLIP_Y_ORIGIN ? gc->clip_y_origin : 0) == ERROR)
	WIN32_API_FAILED ("OffsetClipRgn"), ok = FALSE;
    }
  else if (ok)
    SelectClipRgn (win32_gc->hdc, NULL);

  BDK_NOTE (GC, (g_print ("bdk_win32_hdc_get: %p (%s): ",
			  win32_gc, _bdk_win32_gcvalues_mask_to_string (usage)),
		 _bdk_win32_print_dc (win32_gc->hdc)));

  return win32_gc->hdc;
}

/**
 * bdk_win32_hdc_release:
 * @drawable: destination #BdkDrawable
 * @gc: #BdkGC to use for drawing on @drawable
 * @usage: mask indicating what properties were set up
 *
 * This function deallocates the Windows device context allocated by
 * <funcion>bdk_win32_hdc_get()</function>. It should be called with
 * the same parameters.
 **/
void
bdk_win32_hdc_release (BdkDrawable    *drawable,
		       BdkGC          *gc,
		       BdkGCValuesMask usage)
{
  BdkGCWin32 *win32_gc = (BdkGCWin32 *) gc;
  BdkDrawableImplWin32 *impl = NULL;
  HGDIOBJ hpen = NULL;
  HGDIOBJ hbr = NULL;

  BDK_NOTE (GC, g_print ("bdk_win32_hdc_release: %p: %p (%s)\n",
			 win32_gc, win32_gc->hdc,
			 _bdk_win32_gcvalues_mask_to_string (usage)));

  impl = get_impl_drawable (drawable);

  if (win32_gc->holdpal != NULL)
    {
      gint k;
      
      if (!SelectPalette (win32_gc->hdc, win32_gc->holdpal, FALSE))
	WIN32_GDI_FAILED ("SelectPalette");
      else if ((k = RealizePalette (win32_gc->hdc)) == GDI_ERROR)
	WIN32_GDI_FAILED ("RealizePalette");
      else if (k > 0)
	BDK_NOTE (COLORMAP, g_print ("bdk_win32_hdc_release: realized %p: %d colors\n",
				     win32_gc->holdpal, k));
      win32_gc->holdpal = NULL;
    }

  if (usage & LINE_ATTRIBUTES)
    if ((hpen = GetCurrentObject (win32_gc->hdc, OBJ_PEN)) == NULL)
      WIN32_GDI_FAILED ("GetCurrentObject");
  
  if (usage & BDK_GC_FOREGROUND)
    if ((hbr = GetCurrentObject (win32_gc->hdc, OBJ_BRUSH)) == NULL)
      WIN32_GDI_FAILED ("GetCurrentObject");

  GDI_CALL (RestoreDC, (win32_gc->hdc, win32_gc->saved_dc));

  _bdk_win32_drawable_release_dc (BDK_DRAWABLE (impl));

  if (hpen != NULL)
    GDI_CALL (DeleteObject, (hpen));
  
  if (hbr != NULL)
    GDI_CALL (DeleteObject, (hbr));

  if (win32_gc->pen_hbrbg != NULL)
    GDI_CALL (DeleteObject, (win32_gc->pen_hbrbg));

  win32_gc->hdc = NULL;
}

/* This function originally from Jean-Edouard Lachand-Robert, and
 * available at www.codeguru.com. Simplified for our needs, not sure
 * how much of the original code left any longer. Now handles just
 * one-bit deep bitmaps (in Window parlance, ie those that BDK calls
 * bitmaps (and not pixmaps), with zero pixels being transparent.
 */

/* _bdk_win32_bitmap_to_hrgn : Create a rebunnyion from the
 * "non-transparent" pixels of a bitmap.
 */

HRGN
_bdk_win32_bitmap_to_hrgn (BdkPixmap *pixmap)
{
  HRGN hRgn = NULL;
  HRGN h;
  DWORD maxRects;
  RGNDATA *pData;
  guchar *bits;
  gint width, height, bpl;
  guchar *p;
  gint x, y;

  g_assert (BDK_PIXMAP_OBJECT(pixmap)->depth == 1);

  bits = BDK_PIXMAP_IMPL_WIN32 (BDK_PIXMAP_OBJECT (pixmap)->impl)->bits;
  width = BDK_PIXMAP_IMPL_WIN32 (BDK_PIXMAP_OBJECT (pixmap)->impl)->width;
  height = BDK_PIXMAP_IMPL_WIN32 (BDK_PIXMAP_OBJECT (pixmap)->impl)->height;
  bpl = ((width - 1)/32 + 1)*4;

  /* For better performances, we will use the ExtCreateRebunnyion()
   * function to create the rebunnyion. This function take a RGNDATA
   * structure on entry. We will add rectangles by amount of
   * ALLOC_UNIT number in this structure.
   */
  #define ALLOC_UNIT  100
  maxRects = ALLOC_UNIT;

  pData = g_malloc (sizeof (RGNDATAHEADER) + (sizeof (RECT) * maxRects));
  pData->rdh.dwSize = sizeof (RGNDATAHEADER);
  pData->rdh.iType = RDH_RECTANGLES;
  pData->rdh.nCount = pData->rdh.nRgnSize = 0;
  SetRect (&pData->rdh.rcBound, MAXLONG, MAXLONG, 0, 0);

  for (y = 0; y < height; y++)
    {
      /* Scan each bitmap row from left to right*/
      p = (guchar *) bits + y * bpl;
      for (x = 0; x < width; x++)
	{
	  /* Search for a continuous range of "non transparent pixels"*/
	  gint x0 = x;
	  while (x < width)
	    {
	      if ((((p[x/8])>>(7-(x%8)))&1) == 0)
		/* This pixel is "transparent"*/
		break;
	      x++;
	    }
	  
	  if (x > x0)
	    {
	      RECT *pr;
	      /* Add the pixels (x0, y) to (x, y+1) as a new rectangle
	       * in the rebunnyion
	       */
	      if (pData->rdh.nCount >= maxRects)
		{
		  maxRects += ALLOC_UNIT;
		  pData = g_realloc (pData, sizeof(RGNDATAHEADER)
				     + (sizeof(RECT) * maxRects));
		}
	      pr = (RECT *) &pData->Buffer;
	      SetRect (&pr[pData->rdh.nCount], x0, y, x, y+1);
	      if (x0 < pData->rdh.rcBound.left)
		pData->rdh.rcBound.left = x0;
	      if (y < pData->rdh.rcBound.top)
		pData->rdh.rcBound.top = y;
	      if (x > pData->rdh.rcBound.right)
		pData->rdh.rcBound.right = x;
	      if (y+1 > pData->rdh.rcBound.bottom)
		pData->rdh.rcBound.bottom = y+1;
	      pData->rdh.nCount++;
	      
	      /* On Windows98, ExtCreateRebunnyion() may fail if the
	       * number of rectangles is too large (ie: >
	       * 4000). Therefore, we have to create the rebunnyion by
	       * multiple steps.
	       */
	      if (pData->rdh.nCount == 2000)
		{
		  HRGN h = ExtCreateRebunnyion (NULL, sizeof(RGNDATAHEADER) + (sizeof(RECT) * maxRects), pData);
		  if (hRgn)
		    {
		      CombineRgn(hRgn, hRgn, h, RGN_OR);
		      DeleteObject(h);
		    }
		  else
		    hRgn = h;
		  pData->rdh.nCount = 0;
		  SetRect (&pData->rdh.rcBound, MAXLONG, MAXLONG, 0, 0);
		}
	    }
	}
    }
  
  /* Create or extend the rebunnyion with the remaining rectangles*/
  h = ExtCreateRebunnyion (NULL, sizeof (RGNDATAHEADER)
		       + (sizeof (RECT) * maxRects), pData);
  if (hRgn)
    {
      CombineRgn (hRgn, hRgn, h, RGN_OR);
      DeleteObject (h);
    }
  else
    hRgn = h;

  /* Clean up*/
  g_free (pData);

  return hRgn;
}

HRGN
_bdk_win32_bdkrebunnyion_to_hrgn (const BdkRebunnyion *rebunnyion,
			      gint             x_origin,
			      gint             y_origin)
{
  HRGN hrgn;
  RGNDATA *rgndata;
  RECT *rect;
  BdkRebunnyionBox *boxes = rebunnyion->rects;
  guint nbytes =
    sizeof (RGNDATAHEADER) + (sizeof (RECT) * rebunnyion->numRects);
  int i;

  rgndata = g_malloc (nbytes);
  rgndata->rdh.dwSize = sizeof (RGNDATAHEADER);
  rgndata->rdh.iType = RDH_RECTANGLES;
  rgndata->rdh.nCount = rgndata->rdh.nRgnSize = 0;
  SetRect (&rgndata->rdh.rcBound,
	   G_MAXLONG, G_MAXLONG, G_MINLONG, G_MINLONG);

  for (i = 0; i < rebunnyion->numRects; i++)
    {
      rect = ((RECT *) rgndata->Buffer) + rgndata->rdh.nCount++;

      rect->left = boxes[i].x1 + x_origin;
      rect->right = boxes[i].x2 + x_origin;
      rect->top = boxes[i].y1 + y_origin;
      rect->bottom = boxes[i].y2 + y_origin;

      if (rect->left < rgndata->rdh.rcBound.left)
	rgndata->rdh.rcBound.left = rect->left;
      if (rect->right > rgndata->rdh.rcBound.right)
	rgndata->rdh.rcBound.right = rect->right;
      if (rect->top < rgndata->rdh.rcBound.top)
	rgndata->rdh.rcBound.top = rect->top;
      if (rect->bottom > rgndata->rdh.rcBound.bottom)
	rgndata->rdh.rcBound.bottom = rect->bottom;
    }
  if ((hrgn = ExtCreateRebunnyion (NULL, nbytes, rgndata)) == NULL)
    WIN32_API_FAILED ("ExtCreateRebunnyion");

  g_free (rgndata);

  return (hrgn);
}
