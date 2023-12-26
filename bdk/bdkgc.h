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

#ifndef __BDK_GC_H__
#define __BDK_GC_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BDK_H_INSIDE__) && !defined (BDK_COMPILATION)
#error "Only <bdk/bdk.h> can be included directly."
#endif

#include <bdk/bdkcolor.h>
#include <bdk/bdktypes.h>

B_BEGIN_DECLS

typedef struct _BdkGCValues	      BdkGCValues;
typedef struct _BdkGCClass	      BdkGCClass;

/* GC cap styles
 *  CapNotLast:
 *  CapButt:
 *  CapRound:
 *  CapProjecting:
 */
typedef enum
{
  BDK_CAP_NOT_LAST,
  BDK_CAP_BUTT,
  BDK_CAP_ROUND,
  BDK_CAP_PROJECTING
} BdkCapStyle;

/* GC fill types.
 *  Solid:
 *  Tiled:
 *  Stippled:
 *  OpaqueStippled:
 */
typedef enum
{
  BDK_SOLID,
  BDK_TILED,
  BDK_STIPPLED,
  BDK_OPAQUE_STIPPLED
} BdkFill;

/* GC function types.
 *   Copy: Overwrites destination pixels with the source pixels.
 *   Invert: Inverts the destination pixels.
 *   Xor: Xor's the destination pixels with the source pixels.
 *   Clear: set pixels to 0
 *   And: source AND destination
 *   And Reverse: source AND (NOT destination)
 *   And Invert: (NOT source) AND destination
 *   Noop: destination
 *   Or: source OR destination
 *   Nor: (NOT source) AND (NOT destination)
 *   Equiv: (NOT source) XOR destination
 *   Xor Reverse: source OR (NOT destination)
 *   Copy Inverted: NOT source
 *   Xor Inverted: (NOT source) OR destination
 *   Nand: (NOT source) OR (NOT destination)
 *   Set: set pixels to 1
 */
typedef enum
{
  BDK_COPY,
  BDK_INVERT,
  BDK_XOR,
  BDK_CLEAR,
  BDK_AND,
  BDK_AND_REVERSE,
  BDK_AND_INVERT,
  BDK_NOOP,
  BDK_OR,
  BDK_EQUIV,
  BDK_OR_REVERSE,
  BDK_COPY_INVERT,
  BDK_OR_INVERT,
  BDK_NAND,
  BDK_NOR,
  BDK_SET
} BdkFunction;

/* GC join styles
 *  JoinMiter:
 *  JoinRound:
 *  JoinBevel:
 */
typedef enum
{
  BDK_JOIN_MITER,
  BDK_JOIN_ROUND,
  BDK_JOIN_BEVEL
} BdkJoinStyle;

/* GC line styles
 *  Solid:
 *  OnOffDash:
 *  DoubleDash:
 */
typedef enum
{
  BDK_LINE_SOLID,
  BDK_LINE_ON_OFF_DASH,
  BDK_LINE_DOUBLE_DASH
} BdkLineStyle;

typedef enum
{
  BDK_CLIP_BY_CHILDREN	= 0,
  BDK_INCLUDE_INFERIORS = 1
} BdkSubwindowMode;

typedef enum
{
  BDK_GC_FOREGROUND    = 1 << 0,
  BDK_GC_BACKGROUND    = 1 << 1,
  BDK_GC_FONT	       = 1 << 2,
  BDK_GC_FUNCTION      = 1 << 3,
  BDK_GC_FILL	       = 1 << 4,
  BDK_GC_TILE	       = 1 << 5,
  BDK_GC_STIPPLE       = 1 << 6,
  BDK_GC_CLIP_MASK     = 1 << 7,
  BDK_GC_SUBWINDOW     = 1 << 8,
  BDK_GC_TS_X_ORIGIN   = 1 << 9,
  BDK_GC_TS_Y_ORIGIN   = 1 << 10,
  BDK_GC_CLIP_X_ORIGIN = 1 << 11,
  BDK_GC_CLIP_Y_ORIGIN = 1 << 12,
  BDK_GC_EXPOSURES     = 1 << 13,
  BDK_GC_LINE_WIDTH    = 1 << 14,
  BDK_GC_LINE_STYLE    = 1 << 15,
  BDK_GC_CAP_STYLE     = 1 << 16,
  BDK_GC_JOIN_STYLE    = 1 << 17
} BdkGCValuesMask;

struct _BdkGCValues
{
  BdkColor	    foreground;
  BdkColor	    background;
  BdkFont	   *font;
  BdkFunction	    function;
  BdkFill	    fill;
  BdkPixmap	   *tile;
  BdkPixmap	   *stipple;
  BdkPixmap	   *clip_mask;
  BdkSubwindowMode  subwindow_mode;
  bint		    ts_x_origin;
  bint		    ts_y_origin;
  bint		    clip_x_origin;
  bint		    clip_y_origin;
  bint		    graphics_exposures;
  bint		    line_width;
  BdkLineStyle	    line_style;
  BdkCapStyle	    cap_style;
  BdkJoinStyle	    join_style;
};

#define BDK_TYPE_GC              (bdk_gc_get_type ())
#define BDK_GC(object)           (B_TYPE_CHECK_INSTANCE_CAST ((object), BDK_TYPE_GC, BdkGC))
#define BDK_GC_CLASS(klass)      (B_TYPE_CHECK_CLASS_CAST ((klass), BDK_TYPE_GC, BdkGCClass))
#define BDK_IS_GC(object)        (B_TYPE_CHECK_INSTANCE_TYPE ((object), BDK_TYPE_GC))
#define BDK_IS_GC_CLASS(klass)   (B_TYPE_CHECK_CLASS_TYPE ((klass), BDK_TYPE_GC))
#define BDK_GC_GET_CLASS(obj)    (B_TYPE_INSTANCE_GET_CLASS ((obj), BDK_TYPE_GC, BdkGCClass))

struct _BdkGC
{
  BObject parent_instance;

  bint GSEAL (clip_x_origin);
  bint GSEAL (clip_y_origin);
  bint GSEAL (ts_x_origin);
  bint GSEAL (ts_y_origin);

  BdkColormap *GSEAL (colormap);
};

struct _BdkGCClass 
{
  BObjectClass parent_class;
  
  void (*get_values)     (BdkGC          *gc,
			  BdkGCValues    *values);
  void (*set_values)     (BdkGC          *gc,
			  BdkGCValues    *values,
			  BdkGCValuesMask mask);
  void (*set_dashes)     (BdkGC          *gc,
			  bint	          dash_offset,
			  bint8           dash_list[],
			  bint            n);
  
  /* Padding for future expansion */
  void         (*_bdk_reserved1)  (void);
  void         (*_bdk_reserved2)  (void);
  void         (*_bdk_reserved3)  (void);
  void         (*_bdk_reserved4)  (void);
};


#ifndef BDK_DISABLE_DEPRECATED
GType  bdk_gc_get_type            (void) B_GNUC_CONST;
BdkGC *bdk_gc_new		  (BdkDrawable	    *drawable);
BdkGC *bdk_gc_new_with_values	  (BdkDrawable	    *drawable,
				   BdkGCValues	    *values,
				   BdkGCValuesMask   values_mask);

BdkGC *bdk_gc_ref		  (BdkGC	    *gc);
void   bdk_gc_unref		  (BdkGC	    *gc);

void   bdk_gc_get_values	  (BdkGC	    *gc,
				   BdkGCValues	    *values);
void   bdk_gc_set_values          (BdkGC           *gc,
                                   BdkGCValues	   *values,
                                   BdkGCValuesMask  values_mask);
void   bdk_gc_set_foreground	  (BdkGC	    *gc,
				   const BdkColor   *color);
void   bdk_gc_set_background	  (BdkGC	    *gc,
				   const BdkColor   *color);
void   bdk_gc_set_font		  (BdkGC	    *gc,
				   BdkFont	    *font);
void   bdk_gc_set_function	  (BdkGC	    *gc,
				   BdkFunction	     function);
void   bdk_gc_set_fill		  (BdkGC	    *gc,
				   BdkFill	     fill);
void   bdk_gc_set_tile		  (BdkGC	    *gc,
				   BdkPixmap	    *tile);
void   bdk_gc_set_stipple	  (BdkGC	    *gc,
				   BdkPixmap	    *stipple);
void   bdk_gc_set_ts_origin	  (BdkGC	    *gc,
				   bint		     x,
				   bint		     y);
void   bdk_gc_set_clip_origin	  (BdkGC	    *gc,
				   bint		     x,
				   bint		     y);
void   bdk_gc_set_clip_mask	  (BdkGC	    *gc,
				   BdkBitmap	    *mask);
void   bdk_gc_set_clip_rectangle  (BdkGC	    *gc,
				   const BdkRectangle *rectangle);
void   bdk_gc_set_clip_rebunnyion	  (BdkGC	    *gc,
				   const BdkRebunnyion  *rebunnyion);
void   bdk_gc_set_subwindow	  (BdkGC	    *gc,
				   BdkSubwindowMode  mode);
void   bdk_gc_set_exposures	  (BdkGC	    *gc,
				   bboolean	     exposures);
void   bdk_gc_set_line_attributes (BdkGC	    *gc,
				   bint		     line_width,
				   BdkLineStyle	     line_style,
				   BdkCapStyle	     cap_style,
				   BdkJoinStyle	     join_style);
void   bdk_gc_set_dashes          (BdkGC            *gc,
				   bint	             dash_offset,
				   bint8             dash_list[],
				   bint              n);
void   bdk_gc_offset              (BdkGC            *gc,
				   bint              x_offset,
				   bint              y_offset);
void   bdk_gc_copy		  (BdkGC	    *dst_gc,
				   BdkGC	    *src_gc);


void         bdk_gc_set_colormap     (BdkGC          *gc,
				      BdkColormap    *colormap);
BdkColormap *bdk_gc_get_colormap     (BdkGC          *gc);
void         bdk_gc_set_rgb_fg_color (BdkGC          *gc,
				      const BdkColor *color);
void         bdk_gc_set_rgb_bg_color (BdkGC          *gc,
				      const BdkColor *color);
BdkScreen *  bdk_gc_get_screen	     (BdkGC          *gc);

#define bdk_gc_destroy                 g_object_unref
#endif /* BDK_DISABLE_DEPRECATED */

B_END_DECLS

#endif /* __BDK_DRAWABLE_H__ */
