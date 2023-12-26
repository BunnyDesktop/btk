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

#ifndef __BTK_ENUMS_H__
#define __BTK_ENUMS_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <bunnylib-object.h>

B_BEGIN_DECLS

/* Anchor types */
typedef enum
{
  BTK_ANCHOR_CENTER,
  BTK_ANCHOR_NORTH,
  BTK_ANCHOR_NORTH_WEST,
  BTK_ANCHOR_NORTH_EAST,
  BTK_ANCHOR_SOUTH,
  BTK_ANCHOR_SOUTH_WEST,
  BTK_ANCHOR_SOUTH_EAST,
  BTK_ANCHOR_WEST,
  BTK_ANCHOR_EAST,
  BTK_ANCHOR_N		= BTK_ANCHOR_NORTH,
  BTK_ANCHOR_NW		= BTK_ANCHOR_NORTH_WEST,
  BTK_ANCHOR_NE		= BTK_ANCHOR_NORTH_EAST,
  BTK_ANCHOR_S		= BTK_ANCHOR_SOUTH,
  BTK_ANCHOR_SW		= BTK_ANCHOR_SOUTH_WEST,
  BTK_ANCHOR_SE		= BTK_ANCHOR_SOUTH_EAST,
  BTK_ANCHOR_W		= BTK_ANCHOR_WEST,
  BTK_ANCHOR_E		= BTK_ANCHOR_EAST
} BtkAnchorType;

/* Arrow placement */
typedef enum
{
  BTK_ARROWS_BOTH,
  BTK_ARROWS_START,
  BTK_ARROWS_END
} BtkArrowPlacement;

/* Arrow types */
typedef enum
{
  BTK_ARROW_UP,
  BTK_ARROW_DOWN,
  BTK_ARROW_LEFT,
  BTK_ARROW_RIGHT,
  BTK_ARROW_NONE
} BtkArrowType;

/* Attach options (for tables) */
typedef enum
{
  BTK_EXPAND = 1 << 0,
  BTK_SHRINK = 1 << 1,
  BTK_FILL   = 1 << 2
} BtkAttachOptions;

/* Button box styles */
typedef enum
{
  BTK_BUTTONBOX_DEFAULT_STYLE,
  BTK_BUTTONBOX_SPREAD,
  BTK_BUTTONBOX_EDGE,
  BTK_BUTTONBOX_START,
  BTK_BUTTONBOX_END,
  BTK_BUTTONBOX_CENTER
} BtkButtonBoxStyle;

#ifndef BTK_DISABLE_DEPRECATED
/* Curve types */
typedef enum
{
  BTK_CURVE_TYPE_LINEAR,       /* linear interpolation */
  BTK_CURVE_TYPE_SPLINE,       /* spline interpolation */
  BTK_CURVE_TYPE_FREE          /* free form curve */
} BtkCurveType;
#endif

typedef enum
{
  BTK_DELETE_CHARS,
  BTK_DELETE_WORD_ENDS,           /* delete only the portion of the word to the
                                   * left/right of cursor if we're in the middle
                                   * of a word */
  BTK_DELETE_WORDS,
  BTK_DELETE_DISPLAY_LINES,
  BTK_DELETE_DISPLAY_LINE_ENDS,
  BTK_DELETE_PARAGRAPH_ENDS,      /* like C-k in Emacs (or its reverse) */
  BTK_DELETE_PARAGRAPHS,          /* C-k in pico, kill whole line */
  BTK_DELETE_WHITESPACE           /* M-\ in Emacs */
} BtkDeleteType;

/* Focus movement types */
typedef enum
{
  BTK_DIR_TAB_FORWARD,
  BTK_DIR_TAB_BACKWARD,
  BTK_DIR_UP,
  BTK_DIR_DOWN,
  BTK_DIR_LEFT,
  BTK_DIR_RIGHT
} BtkDirectionType;

/* Expander styles */
typedef enum
{
  BTK_EXPANDER_COLLAPSED,
  BTK_EXPANDER_SEMI_COLLAPSED,
  BTK_EXPANDER_SEMI_EXPANDED,
  BTK_EXPANDER_EXPANDED
} BtkExpanderStyle;

/* Built-in stock icon sizes */
typedef enum
{
  BTK_ICON_SIZE_INVALID,
  BTK_ICON_SIZE_MENU,
  BTK_ICON_SIZE_SMALL_TOOLBAR,
  BTK_ICON_SIZE_LARGE_TOOLBAR,
  BTK_ICON_SIZE_BUTTON,
  BTK_ICON_SIZE_DND,
  BTK_ICON_SIZE_DIALOG
} BtkIconSize;

/* automatic sensitivity */
typedef enum
{
  BTK_SENSITIVITY_AUTO,
  BTK_SENSITIVITY_ON,
  BTK_SENSITIVITY_OFF
} BtkSensitivityType;

#ifndef BTK_DISABLE_DEPRECATED
/* side types */
typedef enum
{
  BTK_SIDE_TOP,
  BTK_SIDE_BOTTOM,
  BTK_SIDE_LEFT,
  BTK_SIDE_RIGHT
} BtkSideType;
#endif /* BTK_DISABLE_DEPRECATED */

/* Reading directions for text */
typedef enum
{
  BTK_TEXT_DIR_NONE,
  BTK_TEXT_DIR_LTR,
  BTK_TEXT_DIR_RTL
} BtkTextDirection;

/* justification for label and maybe other widgets (text?) */
typedef enum
{
  BTK_JUSTIFY_LEFT,
  BTK_JUSTIFY_RIGHT,
  BTK_JUSTIFY_CENTER,
  BTK_JUSTIFY_FILL
} BtkJustification;

#ifndef BTK_DISABLE_DEPRECATED
/* BtkPatternSpec match types */
typedef enum
{
  BTK_MATCH_ALL,       /* "*A?A*" */
  BTK_MATCH_ALL_TAIL,  /* "*A?AA" */
  BTK_MATCH_HEAD,      /* "AAAA*" */
  BTK_MATCH_TAIL,      /* "*AAAA" */
  BTK_MATCH_EXACT,     /* "AAAAA" */
  BTK_MATCH_LAST
} BtkMatchType;
#endif /* BTK_DISABLE_DEPRECATED */

/* Menu keyboard movement types */
typedef enum
{
  BTK_MENU_DIR_PARENT,
  BTK_MENU_DIR_CHILD,
  BTK_MENU_DIR_NEXT,
  BTK_MENU_DIR_PREV
} BtkMenuDirectionType;

/**
 * BtkMessageType:
 * @BTK_MESSAGE_INFO: Informational message
 * @BTK_MESSAGE_WARNING: Nonfatal warning message
 * @BTK_MESSAGE_QUESTION: Question requiring a choice
 * @BTK_MESSAGE_ERROR: Fatal error message
 * @BTK_MESSAGE_OTHER: None of the above, doesn't get an icon
 *
 * The type of message being displayed in the dialog.
 */
typedef enum
{
  BTK_MESSAGE_INFO,
  BTK_MESSAGE_WARNING,
  BTK_MESSAGE_QUESTION,
  BTK_MESSAGE_ERROR,
  BTK_MESSAGE_OTHER
} BtkMessageType;

typedef enum
{
  BTK_PIXELS,
  BTK_INCHES,
  BTK_CENTIMETERS
} BtkMetricType;

typedef enum
{
  BTK_MOVEMENT_LOGICAL_POSITIONS, /* move by forw/back graphemes */
  BTK_MOVEMENT_VISUAL_POSITIONS,  /* move by left/right graphemes */
  BTK_MOVEMENT_WORDS,             /* move by forward/back words */
  BTK_MOVEMENT_DISPLAY_LINES,     /* move up/down lines (wrapped lines) */
  BTK_MOVEMENT_DISPLAY_LINE_ENDS, /* move to either end of a line */
  BTK_MOVEMENT_PARAGRAPHS,        /* move up/down paragraphs (newline-ended lines) */
  BTK_MOVEMENT_PARAGRAPH_ENDS,    /* move to either end of a paragraph */
  BTK_MOVEMENT_PAGES,	          /* move by pages */
  BTK_MOVEMENT_BUFFER_ENDS,       /* move to ends of the buffer */
  BTK_MOVEMENT_HORIZONTAL_PAGES   /* move horizontally by pages */
} BtkMovementStep;

typedef enum
{
  BTK_SCROLL_STEPS,
  BTK_SCROLL_PAGES,
  BTK_SCROLL_ENDS,
  BTK_SCROLL_HORIZONTAL_STEPS,
  BTK_SCROLL_HORIZONTAL_PAGES,
  BTK_SCROLL_HORIZONTAL_ENDS
} BtkScrollStep;

/* Orientation for toolbars, etc. */
typedef enum
{
  BTK_ORIENTATION_HORIZONTAL,
  BTK_ORIENTATION_VERTICAL
} BtkOrientation;

/* Placement type for scrolled window */
typedef enum
{
  BTK_CORNER_TOP_LEFT,
  BTK_CORNER_BOTTOM_LEFT,
  BTK_CORNER_TOP_RIGHT,
  BTK_CORNER_BOTTOM_RIGHT
} BtkCornerType;

/* Packing types (for boxes) */
typedef enum
{
  BTK_PACK_START,
  BTK_PACK_END
} BtkPackType;

/* priorities for path lookups */
typedef enum
{
  BTK_PATH_PRIO_LOWEST      = 0,
  BTK_PATH_PRIO_BTK	    = 4,
  BTK_PATH_PRIO_APPLICATION = 8,
  BTK_PATH_PRIO_THEME       = 10,
  BTK_PATH_PRIO_RC          = 12,
  BTK_PATH_PRIO_HIGHEST     = 15
} BtkPathPriorityType;
#define BTK_PATH_PRIO_MASK 0x0f

/* widget path types */
typedef enum
{
  BTK_PATH_WIDGET,
  BTK_PATH_WIDGET_CLASS,
  BTK_PATH_CLASS
} BtkPathType;

/* Scrollbar policy types (for scrolled windows) */
typedef enum
{
  BTK_POLICY_ALWAYS,
  BTK_POLICY_AUTOMATIC,
  BTK_POLICY_NEVER
} BtkPolicyType;

typedef enum
{
  BTK_POS_LEFT,
  BTK_POS_RIGHT,
  BTK_POS_TOP,
  BTK_POS_BOTTOM
} BtkPositionType;

#ifndef BTK_DISABLE_DEPRECATED
typedef enum
{
  BTK_PREVIEW_COLOR,
  BTK_PREVIEW_GRAYSCALE
} BtkPreviewType;
#endif /* BTK_DISABLE_DEPRECATED */

/* Style for buttons */
typedef enum
{
  BTK_RELIEF_NORMAL,
  BTK_RELIEF_HALF,
  BTK_RELIEF_NONE
} BtkReliefStyle;

/* Resize type */
typedef enum
{
  BTK_RESIZE_PARENT,		/* Pass resize request to the parent */
  BTK_RESIZE_QUEUE,		/* Queue resizes on this widget */
  BTK_RESIZE_IMMEDIATE		/* Perform the resizes now */
} BtkResizeMode;

#ifndef BTK_DISABLE_DEPRECATED
/* signal run types */
typedef enum			/*< flags >*/
{
  BTK_RUN_FIRST      = G_SIGNAL_RUN_FIRST,
  BTK_RUN_LAST       = G_SIGNAL_RUN_LAST,
  BTK_RUN_BOTH       = (BTK_RUN_FIRST | BTK_RUN_LAST),
  BTK_RUN_NO_RECURSE = G_SIGNAL_NO_RECURSE,
  BTK_RUN_ACTION     = G_SIGNAL_ACTION,
  BTK_RUN_NO_HOOKS   = G_SIGNAL_NO_HOOKS
} BtkSignalRunType;
#endif /* BTK_DISABLE_DEPRECATED */

/* scrolling types */
typedef enum
{
  BTK_SCROLL_NONE,
  BTK_SCROLL_JUMP,
  BTK_SCROLL_STEP_BACKWARD,
  BTK_SCROLL_STEP_FORWARD,
  BTK_SCROLL_PAGE_BACKWARD,
  BTK_SCROLL_PAGE_FORWARD,
  BTK_SCROLL_STEP_UP,
  BTK_SCROLL_STEP_DOWN,
  BTK_SCROLL_PAGE_UP,
  BTK_SCROLL_PAGE_DOWN,
  BTK_SCROLL_STEP_LEFT,
  BTK_SCROLL_STEP_RIGHT,
  BTK_SCROLL_PAGE_LEFT,
  BTK_SCROLL_PAGE_RIGHT,
  BTK_SCROLL_START,
  BTK_SCROLL_END
} BtkScrollType;

/* list selection modes */
typedef enum
{
  BTK_SELECTION_NONE,                             /* Nothing can be selected */
  BTK_SELECTION_SINGLE,
  BTK_SELECTION_BROWSE,
  BTK_SELECTION_MULTIPLE,
  BTK_SELECTION_EXTENDED = BTK_SELECTION_MULTIPLE /* Deprecated */
} BtkSelectionMode;

/* Shadow types */
typedef enum
{
  BTK_SHADOW_NONE,
  BTK_SHADOW_IN,
  BTK_SHADOW_OUT,
  BTK_SHADOW_ETCHED_IN,
  BTK_SHADOW_ETCHED_OUT
} BtkShadowType;

/* Widget states */
typedef enum
{
  BTK_STATE_NORMAL,
  BTK_STATE_ACTIVE,
  BTK_STATE_PRELIGHT,
  BTK_STATE_SELECTED,
  BTK_STATE_INSENSITIVE
} BtkStateType;

#if !defined(BTK_DISABLE_DEPRECATED) || defined (BTK_MENU_INTERNALS)
/* Directions for submenus */
typedef enum
{
  BTK_DIRECTION_LEFT,
  BTK_DIRECTION_RIGHT
} BtkSubmenuDirection;

/* Placement of submenus */
typedef enum
{
  BTK_TOP_BOTTOM,
  BTK_LEFT_RIGHT
} BtkSubmenuPlacement;
#endif /* BTK_DISABLE_DEPRECATED */

/* Style for toolbars */
typedef enum
{
  BTK_TOOLBAR_ICONS,
  BTK_TOOLBAR_TEXT,
  BTK_TOOLBAR_BOTH,
  BTK_TOOLBAR_BOTH_HORIZ
} BtkToolbarStyle;

/* Data update types (for ranges) */
typedef enum
{
  BTK_UPDATE_CONTINUOUS,
  BTK_UPDATE_DISCONTINUOUS,
  BTK_UPDATE_DELAYED
} BtkUpdateType;

/* Generic visibility flags */
typedef enum
{
  BTK_VISIBILITY_NONE,
  BTK_VISIBILITY_PARTIAL,
  BTK_VISIBILITY_FULL
} BtkVisibility;

/* Window position types */
typedef enum
{
  BTK_WIN_POS_NONE,
  BTK_WIN_POS_CENTER,
  BTK_WIN_POS_MOUSE,
  BTK_WIN_POS_CENTER_ALWAYS,
  BTK_WIN_POS_CENTER_ON_PARENT
} BtkWindowPosition;

/* Window types */
typedef enum
{
  BTK_WINDOW_TOPLEVEL,
  BTK_WINDOW_POPUP
} BtkWindowType;

/* Text wrap */
typedef enum
{
  BTK_WRAP_NONE,
  BTK_WRAP_CHAR,
  BTK_WRAP_WORD,
  BTK_WRAP_WORD_CHAR
} BtkWrapMode;

/* How to sort */
typedef enum
{
  BTK_SORT_ASCENDING,
  BTK_SORT_DESCENDING
} BtkSortType;

/* Style for btk input method preedit/status */
typedef enum
{
  BTK_IM_PREEDIT_NOTHING,
  BTK_IM_PREEDIT_CALLBACK,
  BTK_IM_PREEDIT_NONE
} BtkIMPreeditStyle;

typedef enum
{
  BTK_IM_STATUS_NOTHING,
  BTK_IM_STATUS_CALLBACK,
  BTK_IM_STATUS_NONE
} BtkIMStatusStyle;

typedef enum
{
  BTK_PACK_DIRECTION_LTR,
  BTK_PACK_DIRECTION_RTL,
  BTK_PACK_DIRECTION_TTB,
  BTK_PACK_DIRECTION_BTT
} BtkPackDirection;

typedef enum
{
  BTK_PRINT_PAGES_ALL,
  BTK_PRINT_PAGES_CURRENT,
  BTK_PRINT_PAGES_RANGES,
  BTK_PRINT_PAGES_SELECTION
} BtkPrintPages;

typedef enum
{
  BTK_PAGE_SET_ALL,
  BTK_PAGE_SET_EVEN,
  BTK_PAGE_SET_ODD
} BtkPageSet;

typedef enum
{
  BTK_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_TOP_TO_BOTTOM, /*< nick=lrtb >*/
  BTK_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_BOTTOM_TO_TOP, /*< nick=lrbt >*/
  BTK_NUMBER_UP_LAYOUT_RIGHT_TO_LEFT_TOP_TO_BOTTOM, /*< nick=rltb >*/
  BTK_NUMBER_UP_LAYOUT_RIGHT_TO_LEFT_BOTTOM_TO_TOP, /*< nick=rlbt >*/
  BTK_NUMBER_UP_LAYOUT_TOP_TO_BOTTOM_LEFT_TO_RIGHT, /*< nick=tblr >*/
  BTK_NUMBER_UP_LAYOUT_TOP_TO_BOTTOM_RIGHT_TO_LEFT, /*< nick=tbrl >*/
  BTK_NUMBER_UP_LAYOUT_BOTTOM_TO_TOP_LEFT_TO_RIGHT, /*< nick=btlr >*/
  BTK_NUMBER_UP_LAYOUT_BOTTOM_TO_TOP_RIGHT_TO_LEFT  /*< nick=btrl >*/
} BtkNumberUpLayout;

typedef enum
{
  BTK_PAGE_ORIENTATION_PORTRAIT,
  BTK_PAGE_ORIENTATION_LANDSCAPE,
  BTK_PAGE_ORIENTATION_REVERSE_PORTRAIT,
  BTK_PAGE_ORIENTATION_REVERSE_LANDSCAPE
} BtkPageOrientation;

typedef enum
{
  BTK_PRINT_QUALITY_LOW,
  BTK_PRINT_QUALITY_NORMAL,
  BTK_PRINT_QUALITY_HIGH,
  BTK_PRINT_QUALITY_DRAFT
} BtkPrintQuality;

typedef enum
{
  BTK_PRINT_DUPLEX_SIMPLEX,
  BTK_PRINT_DUPLEX_HORIZONTAL,
  BTK_PRINT_DUPLEX_VERTICAL
} BtkPrintDuplex;


typedef enum
{
  BTK_UNIT_PIXEL,
  BTK_UNIT_POINTS,
  BTK_UNIT_INCH,
  BTK_UNIT_MM
} BtkUnit;

typedef enum
{
  BTK_TREE_VIEW_GRID_LINES_NONE,
  BTK_TREE_VIEW_GRID_LINES_HORIZONTAL,
  BTK_TREE_VIEW_GRID_LINES_VERTICAL,
  BTK_TREE_VIEW_GRID_LINES_BOTH
} BtkTreeViewGridLines;

typedef enum
{
  BTK_DRAG_RESULT_SUCCESS,
  BTK_DRAG_RESULT_NO_TARGET,
  BTK_DRAG_RESULT_USER_CANCELLED,
  BTK_DRAG_RESULT_TIMEOUT_EXPIRED,
  BTK_DRAG_RESULT_GRAB_BROKEN,
  BTK_DRAG_RESULT_ERROR
} BtkDragResult;

B_END_DECLS

#endif /* __BTK_ENUMS_H__ */
