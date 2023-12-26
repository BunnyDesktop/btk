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

#ifndef __BDK_CURSOR_H__
#define __BDK_CURSOR_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BDK_H_INSIDE__) && !defined (BDK_COMPILATION)
#error "Only <bdk/bdk.h> can be included directly."
#endif

#include <bdk/bdktypes.h>
#include <bdk-pixbuf/bdk-pixbuf.h>

G_BEGIN_DECLS

#define BDK_TYPE_CURSOR (bdk_cursor_get_type ())

/* Cursor types.
 */
typedef enum
{
  BDK_X_CURSOR 		  = 0,
  BDK_ARROW 		  = 2,
  BDK_BASED_ARROW_DOWN    = 4,
  BDK_BASED_ARROW_UP 	  = 6,
  BDK_BOAT 		  = 8,
  BDK_BOGOSITY 		  = 10,
  BDK_BOTTOM_LEFT_CORNER  = 12,
  BDK_BOTTOM_RIGHT_CORNER = 14,
  BDK_BOTTOM_SIDE 	  = 16,
  BDK_BOTTOM_TEE 	  = 18,
  BDK_BOX_SPIRAL 	  = 20,
  BDK_CENTER_PTR 	  = 22,
  BDK_CIRCLE 		  = 24,
  BDK_CLOCK	 	  = 26,
  BDK_COFFEE_MUG 	  = 28,
  BDK_CROSS 		  = 30,
  BDK_CROSS_REVERSE 	  = 32,
  BDK_CROSSHAIR 	  = 34,
  BDK_DIAMOND_CROSS 	  = 36,
  BDK_DOT 		  = 38,
  BDK_DOTBOX 		  = 40,
  BDK_DOUBLE_ARROW 	  = 42,
  BDK_DRAFT_LARGE 	  = 44,
  BDK_DRAFT_SMALL 	  = 46,
  BDK_DRAPED_BOX 	  = 48,
  BDK_EXCHANGE 		  = 50,
  BDK_FLEUR 		  = 52,
  BDK_GOBBLER 		  = 54,
  BDK_GUMBY 		  = 56,
  BDK_HAND1 		  = 58,
  BDK_HAND2 		  = 60,
  BDK_HEART 		  = 62,
  BDK_ICON 		  = 64,
  BDK_IRON_CROSS 	  = 66,
  BDK_LEFT_PTR 		  = 68,
  BDK_LEFT_SIDE 	  = 70,
  BDK_LEFT_TEE 		  = 72,
  BDK_LEFTBUTTON 	  = 74,
  BDK_LL_ANGLE 		  = 76,
  BDK_LR_ANGLE 	 	  = 78,
  BDK_MAN 		  = 80,
  BDK_MIDDLEBUTTON 	  = 82,
  BDK_MOUSE 		  = 84,
  BDK_PENCIL 		  = 86,
  BDK_PIRATE 		  = 88,
  BDK_PLUS 		  = 90,
  BDK_QUESTION_ARROW 	  = 92,
  BDK_RIGHT_PTR 	  = 94,
  BDK_RIGHT_SIDE 	  = 96,
  BDK_RIGHT_TEE 	  = 98,
  BDK_RIGHTBUTTON 	  = 100,
  BDK_RTL_LOGO 		  = 102,
  BDK_SAILBOAT 		  = 104,
  BDK_SB_DOWN_ARROW 	  = 106,
  BDK_SB_H_DOUBLE_ARROW   = 108,
  BDK_SB_LEFT_ARROW 	  = 110,
  BDK_SB_RIGHT_ARROW 	  = 112,
  BDK_SB_UP_ARROW 	  = 114,
  BDK_SB_V_DOUBLE_ARROW   = 116,
  BDK_SHUTTLE 		  = 118,
  BDK_SIZING 		  = 120,
  BDK_SPIDER		  = 122,
  BDK_SPRAYCAN 		  = 124,
  BDK_STAR 		  = 126,
  BDK_TARGET 		  = 128,
  BDK_TCROSS 		  = 130,
  BDK_TOP_LEFT_ARROW 	  = 132,
  BDK_TOP_LEFT_CORNER 	  = 134,
  BDK_TOP_RIGHT_CORNER 	  = 136,
  BDK_TOP_SIDE 		  = 138,
  BDK_TOP_TEE 		  = 140,
  BDK_TREK 		  = 142,
  BDK_UL_ANGLE 		  = 144,
  BDK_UMBRELLA 		  = 146,
  BDK_UR_ANGLE 		  = 148,
  BDK_WATCH 		  = 150,
  BDK_XTERM 		  = 152,
  BDK_LAST_CURSOR,
  BDK_BLANK_CURSOR        = -2,
  BDK_CURSOR_IS_PIXMAP 	  = -1
} BdkCursorType;

struct _BdkCursor
{
  BdkCursorType GSEAL (type);
  /*< private >*/
  guint GSEAL (ref_count);
};

/* Cursors
 */

GType      bdk_cursor_get_type           (void) G_GNUC_CONST;

BdkCursor* bdk_cursor_new_for_display	 (BdkDisplay      *display,
					  BdkCursorType    cursor_type);
#ifndef BDK_MULTIHEAD_SAFE
BdkCursor* bdk_cursor_new		 (BdkCursorType	   cursor_type);
#endif
BdkCursor* bdk_cursor_new_from_pixmap	 (BdkPixmap	  *source,
					  BdkPixmap	  *mask,
					  const BdkColor  *fg,
					  const BdkColor  *bg,
					  gint		   x,
					  gint		   y);
BdkCursor* bdk_cursor_new_from_pixbuf	 (BdkDisplay      *display,
					  BdkPixbuf       *pixbuf,
					  gint             x,
					  gint             y);
BdkDisplay* bdk_cursor_get_display	 (BdkCursor	  *cursor);
BdkCursor*  bdk_cursor_ref               (BdkCursor       *cursor);
void        bdk_cursor_unref             (BdkCursor       *cursor);
BdkCursor*  bdk_cursor_new_from_name	 (BdkDisplay      *display,
					  const gchar     *name);
BdkPixbuf*  bdk_cursor_get_image         (BdkCursor       *cursor);
BdkCursorType bdk_cursor_get_cursor_type (BdkCursor       *cursor);

#ifndef BDK_DISABLE_DEPRECATED
#define bdk_cursor_destroy             bdk_cursor_unref
#endif /* BDK_DISABLE_DEPRECATED */

G_END_DECLS

#endif /* __BDK_CURSOR_H__ */
