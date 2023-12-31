/* BTK+ Pixbuf Engine
 * Copyright (C) 1998-2000 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Written by Owen Taylor <otaylor@redhat.com>, based on code by
 * Carsten Haitzler <raster@rasterman.com>
 */

#include <btk/btk.h>
#include <bdk-pixbuf/bdk-pixbuf.h>

/* internals */

typedef struct _ThemeData ThemeData;
typedef struct _ThemeImage ThemeImage;
typedef struct _ThemeMatchData ThemeMatchData;
typedef struct _ThemePixbuf ThemePixbuf;

enum
{
  TOKEN_IMAGE = G_TOKEN_LAST + 1,
  TOKEN_FUNCTION,
  TOKEN_FILE,
  TOKEN_STRETCH,
  TOKEN_RECOLORABLE,
  TOKEN_BORDER,
  TOKEN_DETAIL,
  TOKEN_STATE,
  TOKEN_SHADOW,
  TOKEN_GAP_SIDE,
  TOKEN_GAP_FILE,
  TOKEN_GAP_BORDER,
  TOKEN_GAP_START_FILE,
  TOKEN_GAP_START_BORDER,
  TOKEN_GAP_END_FILE,
  TOKEN_GAP_END_BORDER,
  TOKEN_OVERLAY_FILE,
  TOKEN_OVERLAY_BORDER,
  TOKEN_OVERLAY_STRETCH,
  TOKEN_ARROW_DIRECTION,
  TOKEN_EXPANDER_STYLE,
  TOKEN_WINDOW_EDGE,
  TOKEN_D_HLINE,
  TOKEN_D_VLINE,
  TOKEN_D_SHADOW,
  TOKEN_D_POLYGON,
  TOKEN_D_ARROW,
  TOKEN_D_DIAMOND,
  TOKEN_D_OVAL,
  TOKEN_D_STRING,
  TOKEN_D_BOX,
  TOKEN_D_FLAT_BOX,
  TOKEN_D_CHECK,
  TOKEN_D_OPTION,
  TOKEN_D_CROSS,
  TOKEN_D_RAMP,
  TOKEN_D_TAB,
  TOKEN_D_SHADOW_GAP,
  TOKEN_D_BOX_GAP,
  TOKEN_D_EXTENSION,
  TOKEN_D_FOCUS,
  TOKEN_D_SLIDER,
  TOKEN_D_ENTRY,
  TOKEN_D_HANDLE,
  TOKEN_D_STEPPER,
  TOKEN_D_EXPANDER,
  TOKEN_D_RESIZE_GRIP,
  TOKEN_TRUE,
  TOKEN_FALSE,
  TOKEN_TOP,
  TOKEN_UP,
  TOKEN_BOTTOM,
  TOKEN_DOWN,
  TOKEN_LEFT,
  TOKEN_RIGHT,
  TOKEN_NORMAL,
  TOKEN_ACTIVE,
  TOKEN_PRELIGHT,
  TOKEN_SELECTED,
  TOKEN_INSENSITIVE,
  TOKEN_NONE,
  TOKEN_IN,
  TOKEN_OUT,
  TOKEN_ETCHED_IN,
  TOKEN_ETCHED_OUT,
  TOKEN_ORIENTATION,
  TOKEN_HORIZONTAL,
  TOKEN_VERTICAL,
  TOKEN_COLLAPSED,
  TOKEN_SEMI_COLLAPSED,
  TOKEN_SEMI_EXPANDED,
  TOKEN_EXPANDED,
  TOKEN_NORTH_WEST,
  TOKEN_NORTH,
  TOKEN_NORTH_EAST,
  TOKEN_WEST,
  TOKEN_EAST,
  TOKEN_SOUTH_WEST,
  TOKEN_SOUTH,
  TOKEN_SOUTH_EAST,
  TOKEN_DIRECTION,
  TOKEN_LTR,
  TOKEN_RTL
};

typedef enum
{
  COMPONENT_NORTH_WEST = 1 << 0,
  COMPONENT_NORTH      = 1 << 1,
  COMPONENT_NORTH_EAST = 1 << 2, 
  COMPONENT_WEST       = 1 << 3,
  COMPONENT_CENTER     = 1 << 4,
  COMPONENT_EAST       = 1 << 5, 
  COMPONENT_SOUTH_EAST = 1 << 6,
  COMPONENT_SOUTH      = 1 << 7,
  COMPONENT_SOUTH_WEST = 1 << 8,
  COMPONENT_ALL 	  = 1 << 9
} ThemePixbufComponent;

typedef enum {
  THEME_MATCH_GAP_SIDE        = 1 << 0,
  THEME_MATCH_ORIENTATION     = 1 << 1,
  THEME_MATCH_STATE           = 1 << 2,
  THEME_MATCH_SHADOW          = 1 << 3,
  THEME_MATCH_ARROW_DIRECTION = 1 << 4,
  THEME_MATCH_EXPANDER_STYLE  = 1 << 5,
  THEME_MATCH_WINDOW_EDGE     = 1 << 6,
  THEME_MATCH_DIRECTION       = 1 << 7
} ThemeMatchFlags;

typedef enum {
  THEME_CONSTANT_ROWS = 1 << 0,
  THEME_CONSTANT_COLS = 1 << 1,
  THEME_MISSING = 1 << 2
} ThemeRenderHints;

struct _ThemePixbuf
{
  bchar     *filename;
  BdkPixbuf *pixbuf;
  bboolean   stretch;
  bint       border_left;
  bint       border_right;
  bint       border_bottom;
  bint       border_top;
  buint      hints[3][3];
};

struct _ThemeMatchData
{
  buint            function;	/* Mandatory */
  bchar           *detail;

  ThemeMatchFlags  flags;

  BtkPositionType  gap_side;
  BtkOrientation   orientation;
  BtkStateType     state;
  BtkShadowType    shadow;
  BtkArrowType     arrow_direction;
  BtkExpanderStyle expander_style;
  BdkWindowEdge    window_edge;
  BtkTextDirection direction;
};

struct _ThemeImage
{
  buint           refcount;

  ThemePixbuf    *background;
  ThemePixbuf    *overlay;
  ThemePixbuf    *gap_start;
  ThemePixbuf    *gap;
  ThemePixbuf    *gap_end;
  
  bchar           recolorable;

  ThemeMatchData  match_data;
};


B_GNUC_INTERNAL ThemePixbuf *theme_pixbuf_new          (void);
B_GNUC_INTERNAL void         theme_pixbuf_destroy      (ThemePixbuf  *theme_pb);
B_GNUC_INTERNAL void         theme_clear_pixbuf        (ThemePixbuf **theme_pb);
B_GNUC_INTERNAL void         theme_pixbuf_set_filename (ThemePixbuf  *theme_pb,
					const char   *filename);
B_GNUC_INTERNAL BdkPixbuf *  theme_pixbuf_get_pixbuf   (ThemePixbuf  *theme_pb);
B_GNUC_INTERNAL void         theme_pixbuf_set_border   (ThemePixbuf  *theme_pb,
					bint          left,
					bint          right,
					bint          top,
					bint          bottom);
B_GNUC_INTERNAL void         theme_pixbuf_set_stretch  (ThemePixbuf  *theme_pb,
					bboolean      stretch);
B_GNUC_INTERNAL void         theme_pixbuf_render       (ThemePixbuf  *theme_pb,
					BdkWindow    *window,
					BdkBitmap    *mask,
					BdkRectangle *clip_rect,
					buint         component_mask,
					bboolean      center,
					bint          dest_x,
					bint          dest_y,
					bint          dest_width,
					bint          dest_height);



extern BtkStyleClass pixmap_default_class;
