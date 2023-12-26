/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball, Josh MacDonald
 * Copyright (C) 1997-1998 Jay Painter <jpaint@serv.net><jpaint@gimp.org>
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

#if !defined (BTK_DISABLE_DEPRECATED) || defined (__BTK_CLIST_C__) || defined (__BTK_CTREE_C__)

#ifndef __BTK_CLIST_H__
#define __BTK_CLIST_H__


#include <btk/btksignal.h>
#include <btk/btkalignment.h>
#include <btk/btklabel.h>
#include <btk/btkbutton.h>
#include <btk/btkhscrollbar.h>
#include <btk/btkvscrollbar.h>


B_BEGIN_DECLS


/* clist flags */
enum {
  BTK_CLIST_IN_DRAG             = 1 <<  0,
  BTK_CLIST_ROW_HEIGHT_SET      = 1 <<  1,
  BTK_CLIST_SHOW_TITLES         = 1 <<  2,
  /* Unused */
  BTK_CLIST_ADD_MODE            = 1 <<  4,
  BTK_CLIST_AUTO_SORT           = 1 <<  5,
  BTK_CLIST_AUTO_RESIZE_BLOCKED = 1 <<  6,
  BTK_CLIST_REORDERABLE         = 1 <<  7,
  BTK_CLIST_USE_DRAG_ICONS      = 1 <<  8,
  BTK_CLIST_DRAW_DRAG_LINE      = 1 <<  9,
  BTK_CLIST_DRAW_DRAG_RECT      = 1 << 10
};

/* cell types */
typedef enum
{
  BTK_CELL_EMPTY,
  BTK_CELL_TEXT,
  BTK_CELL_PIXMAP,
  BTK_CELL_PIXTEXT,
  BTK_CELL_WIDGET
} BtkCellType;

typedef enum
{
  BTK_CLIST_DRAG_NONE,
  BTK_CLIST_DRAG_BEFORE,
  BTK_CLIST_DRAG_INTO,
  BTK_CLIST_DRAG_AFTER
} BtkCListDragPos;

typedef enum
{
  BTK_BUTTON_IGNORED = 0,
  BTK_BUTTON_SELECTS = 1 << 0,
  BTK_BUTTON_DRAGS   = 1 << 1,
  BTK_BUTTON_EXPANDS = 1 << 2
} BtkButtonAction;

#define BTK_TYPE_CLIST            (btk_clist_get_type ())
#define BTK_CLIST(obj)            (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_CLIST, BtkCList))
#define BTK_CLIST_CLASS(klass)    (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_CLIST, BtkCListClass))
#define BTK_IS_CLIST(obj)         (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_CLIST))
#define BTK_IS_CLIST_CLASS(klass) (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_CLIST))
#define BTK_CLIST_GET_CLASS(obj)  (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_CLIST, BtkCListClass))


#define BTK_CLIST_FLAGS(clist)             (BTK_CLIST (clist)->flags)
#define BTK_CLIST_SET_FLAG(clist,flag)     (BTK_CLIST_FLAGS (clist) |= (BTK_ ## flag))
#define BTK_CLIST_UNSET_FLAG(clist,flag)   (BTK_CLIST_FLAGS (clist) &= ~(BTK_ ## flag))

#define BTK_CLIST_IN_DRAG(clist)           (BTK_CLIST_FLAGS (clist) & BTK_CLIST_IN_DRAG)
#define BTK_CLIST_ROW_HEIGHT_SET(clist)    (BTK_CLIST_FLAGS (clist) & BTK_CLIST_ROW_HEIGHT_SET)
#define BTK_CLIST_SHOW_TITLES(clist)       (BTK_CLIST_FLAGS (clist) & BTK_CLIST_SHOW_TITLES)
#define BTK_CLIST_ADD_MODE(clist)          (BTK_CLIST_FLAGS (clist) & BTK_CLIST_ADD_MODE)
#define BTK_CLIST_AUTO_SORT(clist)         (BTK_CLIST_FLAGS (clist) & BTK_CLIST_AUTO_SORT)
#define BTK_CLIST_AUTO_RESIZE_BLOCKED(clist) (BTK_CLIST_FLAGS (clist) & BTK_CLIST_AUTO_RESIZE_BLOCKED)
#define BTK_CLIST_REORDERABLE(clist)       (BTK_CLIST_FLAGS (clist) & BTK_CLIST_REORDERABLE)
#define BTK_CLIST_USE_DRAG_ICONS(clist)    (BTK_CLIST_FLAGS (clist) & BTK_CLIST_USE_DRAG_ICONS)
#define BTK_CLIST_DRAW_DRAG_LINE(clist)    (BTK_CLIST_FLAGS (clist) & BTK_CLIST_DRAW_DRAG_LINE)
#define BTK_CLIST_DRAW_DRAG_RECT(clist)    (BTK_CLIST_FLAGS (clist) & BTK_CLIST_DRAW_DRAG_RECT)

#define BTK_CLIST_ROW(_glist_) ((BtkCListRow *)((_glist_)->data))

/* pointer casting for cells */
#define BTK_CELL_TEXT(cell)     (((BtkCellText *) &(cell)))
#define BTK_CELL_PIXMAP(cell)   (((BtkCellPixmap *) &(cell)))
#define BTK_CELL_PIXTEXT(cell)  (((BtkCellPixText *) &(cell)))
#define BTK_CELL_WIDGET(cell)   (((BtkCellWidget *) &(cell)))

typedef struct _BtkCList BtkCList;
typedef struct _BtkCListClass BtkCListClass;
typedef struct _BtkCListColumn BtkCListColumn;
typedef struct _BtkCListRow BtkCListRow;

typedef struct _BtkCell BtkCell;
typedef struct _BtkCellText BtkCellText;
typedef struct _BtkCellPixmap BtkCellPixmap;
typedef struct _BtkCellPixText BtkCellPixText;
typedef struct _BtkCellWidget BtkCellWidget;

typedef bint (*BtkCListCompareFunc) (BtkCList     *clist,
				     gconstpointer ptr1,
				     gconstpointer ptr2);

typedef struct _BtkCListCellInfo BtkCListCellInfo;
typedef struct _BtkCListDestInfo BtkCListDestInfo;

struct _BtkCListCellInfo
{
  bint row;
  bint column;
};

struct _BtkCListDestInfo
{
  BtkCListCellInfo cell;
  BtkCListDragPos  insert_pos;
};

struct _BtkCList
{
  BtkContainer container;
  
  buint16 flags;
  
  bpointer reserved1;
  bpointer reserved2;

  buint freeze_count;
  
  /* allocation rectangle after the container_border_width
   * and the width of the shadow border */
  BdkRectangle internal_allocation;
  
  /* rows */
  bint rows;
  bint row_height;
  GList *row_list;
  GList *row_list_end;
  
  /* columns */
  bint columns;
  BdkRectangle column_title_area;
  BdkWindow *title_window;
  
  /* dynamicly allocated array of column structures */
  BtkCListColumn *column;
  
  /* the scrolling window and its height and width to
   * make things a little speedier */
  BdkWindow *clist_window;
  bint clist_window_width;
  bint clist_window_height;
  
  /* offsets for scrolling */
  bint hoffset;
  bint voffset;
  
  /* border shadow style */
  BtkShadowType shadow_type;
  
  /* the list's selection mode (btkenums.h) */
  BtkSelectionMode selection_mode;
  
  /* list of selected rows */
  GList *selection;
  GList *selection_end;
  
  GList *undo_selection;
  GList *undo_unselection;
  bint undo_anchor;
  
  /* mouse buttons */
  buint8 button_actions[5];

  buint8 drag_button;

  /* dnd */
  BtkCListCellInfo click_cell;

  /* scroll adjustments */
  BtkAdjustment *hadjustment;
  BtkAdjustment *vadjustment;
  
  /* xor GC for the vertical drag line */
  BdkGC *xor_gc;
  
  /* gc for drawing unselected cells */
  BdkGC *fg_gc;
  BdkGC *bg_gc;
  
  /* cursor used to indicate dragging */
  BdkCursor *cursor_drag;
  
  /* the current x-pixel location of the xor-drag line */
  bint x_drag;
  
  /* focus handling */
  bint focus_row;

  bint focus_header_column;
  
  /* dragging the selection */
  bint anchor;
  BtkStateType anchor_state;
  bint drag_pos;
  bint htimer;
  bint vtimer;
  
  BtkSortType sort_type;
  BtkCListCompareFunc compare;
  bint sort_column;

  bint drag_highlight_row;
  BtkCListDragPos drag_highlight_pos;
};

struct _BtkCListClass
{
  BtkContainerClass parent_class;
  
  void  (*set_scroll_adjustments) (BtkCList       *clist,
				   BtkAdjustment  *hadjustment,
				   BtkAdjustment  *vadjustment);
  void   (*refresh)             (BtkCList       *clist);
  void   (*select_row)          (BtkCList       *clist,
				 bint            row,
				 bint            column,
				 BdkEvent       *event);
  void   (*unselect_row)        (BtkCList       *clist,
				 bint            row,
				 bint            column,
				 BdkEvent       *event);
  void   (*row_move)            (BtkCList       *clist,
				 bint            source_row,
				 bint            dest_row);
  void   (*click_column)        (BtkCList       *clist,
				 bint            column);
  void   (*resize_column)       (BtkCList       *clist,
				 bint            column,
                                 bint            width);
  void   (*toggle_focus_row)    (BtkCList       *clist);
  void   (*select_all)          (BtkCList       *clist);
  void   (*unselect_all)        (BtkCList       *clist);
  void   (*undo_selection)      (BtkCList       *clist);
  void   (*start_selection)     (BtkCList       *clist);
  void   (*end_selection)       (BtkCList       *clist);
  void   (*extend_selection)    (BtkCList       *clist,
				 BtkScrollType   scroll_type,
				 bfloat          position,
				 bboolean        auto_start_selection);
  void   (*scroll_horizontal)   (BtkCList       *clist,
				 BtkScrollType   scroll_type,
				 bfloat          position);
  void   (*scroll_vertical)     (BtkCList       *clist,
				 BtkScrollType   scroll_type,
				 bfloat          position);
  void   (*toggle_add_mode)     (BtkCList       *clist);
  void   (*abort_column_resize) (BtkCList       *clist);
  void   (*resync_selection)    (BtkCList       *clist,
				 BdkEvent       *event);
  GList* (*selection_find)      (BtkCList       *clist,
				 bint            row_number,
				 GList          *row_list_element);
  void   (*draw_row)            (BtkCList       *clist,
				 BdkRectangle   *area,
				 bint            row,
				 BtkCListRow    *clist_row);
  void   (*draw_drag_highlight) (BtkCList        *clist,
				 BtkCListRow     *target_row,
				 bint             target_row_number,
				 BtkCListDragPos  drag_pos);
  void   (*clear)               (BtkCList       *clist);
  void   (*fake_unselect_all)   (BtkCList       *clist,
				 bint            row);
  void   (*sort_list)           (BtkCList       *clist);
  bint   (*insert_row)          (BtkCList       *clist,
				 bint            row,
				 bchar          *text[]);
  void   (*remove_row)          (BtkCList       *clist,
				 bint            row);
  void   (*set_cell_contents)   (BtkCList       *clist,
				 BtkCListRow    *clist_row,
				 bint            column,
				 BtkCellType     type,
				 const bchar    *text,
				 buint8          spacing,
				 BdkPixmap      *pixmap,
				 BdkBitmap      *mask);
  void   (*cell_size_request)   (BtkCList       *clist,
				 BtkCListRow    *clist_row,
				 bint            column,
				 BtkRequisition *requisition);

};

struct _BtkCListColumn
{
  bchar *title;
  BdkRectangle area;
  
  BtkWidget *button;
  BdkWindow *window;
  
  bint width;
  bint min_width;
  bint max_width;
  BtkJustification justification;
  
  buint visible        : 1;  
  buint width_set      : 1;
  buint resizeable     : 1;
  buint auto_resize    : 1;
  buint button_passive : 1;
};

struct _BtkCListRow
{
  BtkCell *cell;
  BtkStateType state;
  
  BdkColor foreground;
  BdkColor background;
  
  BtkStyle *style;

  bpointer data;
  GDestroyNotify destroy;
  
  buint fg_set     : 1;
  buint bg_set     : 1;
  buint selectable : 1;
};

/* Cell Structures */
struct _BtkCellText
{
  BtkCellType type;
  
  bint16 vertical;
  bint16 horizontal;
  
  BtkStyle *style;

  bchar *text;
};

struct _BtkCellPixmap
{
  BtkCellType type;
  
  bint16 vertical;
  bint16 horizontal;
  
  BtkStyle *style;

  BdkPixmap *pixmap;
  BdkBitmap *mask;
};

struct _BtkCellPixText
{
  BtkCellType type;
  
  bint16 vertical;
  bint16 horizontal;
  
  BtkStyle *style;

  bchar *text;
  buint8 spacing;
  BdkPixmap *pixmap;
  BdkBitmap *mask;
};

struct _BtkCellWidget
{
  BtkCellType type;
  
  bint16 vertical;
  bint16 horizontal;
  
  BtkStyle *style;

  BtkWidget *widget;
};

struct _BtkCell
{
  BtkCellType type;
  
  bint16 vertical;
  bint16 horizontal;
  
  BtkStyle *style;

  union {
    bchar *text;
    
    struct {
      BdkPixmap *pixmap;
      BdkBitmap *mask;
    } pm;
    
    struct {
      bchar *text;
      buint8 spacing;
      BdkPixmap *pixmap;
      BdkBitmap *mask;
    } pt;
    
    BtkWidget *widget;
  } u;
};

GType btk_clist_get_type (void) B_GNUC_CONST;

/* create a new BtkCList */
BtkWidget* btk_clist_new             (bint   columns);
BtkWidget* btk_clist_new_with_titles (bint   columns,
				      bchar *titles[]);

/* set adjustments of clist */
void btk_clist_set_hadjustment (BtkCList      *clist,
				BtkAdjustment *adjustment);
void btk_clist_set_vadjustment (BtkCList      *clist,
				BtkAdjustment *adjustment);

/* get adjustments of clist */
BtkAdjustment* btk_clist_get_hadjustment (BtkCList *clist);
BtkAdjustment* btk_clist_get_vadjustment (BtkCList *clist);

/* set the border style of the clist */
void btk_clist_set_shadow_type (BtkCList      *clist,
				BtkShadowType  type);

/* set the clist's selection mode */
void btk_clist_set_selection_mode (BtkCList         *clist,
				   BtkSelectionMode  mode);

/* enable clists reorder ability */
void btk_clist_set_reorderable (BtkCList *clist,
				bboolean  reorderable);
void btk_clist_set_use_drag_icons (BtkCList *clist,
				   bboolean  use_icons);
void btk_clist_set_button_actions (BtkCList *clist,
				   buint     button,
				   buint8    button_actions);

/* freeze all visual updates of the list, and then thaw the list after
 * you have made a number of changes and the updates wil occure in a
 * more efficent mannor than if you made them on a unfrozen list
 */
void btk_clist_freeze (BtkCList *clist);
void btk_clist_thaw   (BtkCList *clist);

/* show and hide the column title buttons */
void btk_clist_column_titles_show (BtkCList *clist);
void btk_clist_column_titles_hide (BtkCList *clist);

/* set the column title to be a active title (responds to button presses, 
 * prelights, and grabs keyboard focus), or passive where it acts as just
 * a title
 */
void btk_clist_column_title_active   (BtkCList *clist,
				      bint      column);
void btk_clist_column_title_passive  (BtkCList *clist,
				      bint      column);
void btk_clist_column_titles_active  (BtkCList *clist);
void btk_clist_column_titles_passive (BtkCList *clist);

/* set the title in the column title button */
void btk_clist_set_column_title (BtkCList    *clist,
				 bint         column,
				 const bchar *title);

/* returns the title of column. Returns NULL if title is not set */
bchar * btk_clist_get_column_title (BtkCList *clist,
				    bint      column);

/* set a widget instead of a title for the column title button */
void btk_clist_set_column_widget (BtkCList  *clist,
				  bint       column,
				  BtkWidget *widget);

/* returns the column widget */
BtkWidget * btk_clist_get_column_widget (BtkCList *clist,
					 bint      column);

/* set the justification on a column */
void btk_clist_set_column_justification (BtkCList         *clist,
					 bint              column,
					 BtkJustification  justification);

/* set visibility of a column */
void btk_clist_set_column_visibility (BtkCList *clist,
				      bint      column,
				      bboolean  visible);

/* enable/disable column resize operations by mouse */
void btk_clist_set_column_resizeable (BtkCList *clist,
				      bint      column,
				      bboolean  resizeable);

/* resize column automatically to its optimal width */
void btk_clist_set_column_auto_resize (BtkCList *clist,
				       bint      column,
				       bboolean  auto_resize);

bint btk_clist_columns_autosize (BtkCList *clist);

/* return the optimal column width, i.e. maximum of all cell widths */
bint btk_clist_optimal_column_width (BtkCList *clist,
				     bint      column);

/* set the pixel width of a column; this is a necessary step in
 * creating a CList because otherwise the column width is chozen from
 * the width of the column title, which will never be right
 */
void btk_clist_set_column_width (BtkCList *clist,
				 bint      column,
				 bint      width);

/* set column minimum/maximum width. min/max_width < 0 => no restriction */
void btk_clist_set_column_min_width (BtkCList *clist,
				     bint      column,
				     bint      min_width);
void btk_clist_set_column_max_width (BtkCList *clist,
				     bint      column,
				     bint      max_width);

/* change the height of the rows, the default (height=0) is
 * the hight of the current font.
 */
void btk_clist_set_row_height (BtkCList *clist,
			       buint     height);

/* scroll the viewing area of the list to the given column and row;
 * row_align and col_align are between 0-1 representing the location the
 * row should appear on the screnn, 0.0 being top or left, 1.0 being
 * bottom or right; if row or column is -1 then then there is no change
 */
void btk_clist_moveto (BtkCList *clist,
		       bint      row,
		       bint      column,
		       bfloat    row_align,
		       bfloat    col_align);

/* returns whether the row is visible */
BtkVisibility btk_clist_row_is_visible (BtkCList *clist,
					bint      row);

/* returns the cell type */
BtkCellType btk_clist_get_cell_type (BtkCList *clist,
				     bint      row,
				     bint      column);

/* sets a given cell's text, replacing its current contents */
void btk_clist_set_text (BtkCList    *clist,
			 bint         row,
			 bint         column,
			 const bchar *text);

/* for the "get" functions, any of the return pointer can be
 * NULL if you are not interested
 */
bint btk_clist_get_text (BtkCList  *clist,
			 bint       row,
			 bint       column,
			 bchar    **text);

/* sets a given cell's pixmap, replacing its current contents */
void btk_clist_set_pixmap (BtkCList  *clist,
			   bint       row,
			   bint       column,
			   BdkPixmap *pixmap,
			   BdkBitmap *mask);

bint btk_clist_get_pixmap (BtkCList   *clist,
			   bint        row,
			   bint        column,
			   BdkPixmap **pixmap,
			   BdkBitmap **mask);

/* sets a given cell's pixmap and text, replacing its current contents */
void btk_clist_set_pixtext (BtkCList    *clist,
			    bint         row,
			    bint         column,
			    const bchar *text,
			    buint8       spacing,
			    BdkPixmap   *pixmap,
			    BdkBitmap   *mask);

bint btk_clist_get_pixtext (BtkCList   *clist,
			    bint        row,
			    bint        column,
			    bchar     **text,
			    buint8     *spacing,
			    BdkPixmap **pixmap,
			    BdkBitmap **mask);

/* sets the foreground color of a row, the color must already
 * be allocated
 */
void btk_clist_set_foreground (BtkCList       *clist,
			       bint            row,
			       const BdkColor *color);

/* sets the background color of a row, the color must already
 * be allocated
 */
void btk_clist_set_background (BtkCList       *clist,
			       bint            row,
			       const BdkColor *color);

/* set / get cell styles */
void btk_clist_set_cell_style (BtkCList *clist,
			       bint      row,
			       bint      column,
			       BtkStyle *style);

BtkStyle *btk_clist_get_cell_style (BtkCList *clist,
				    bint      row,
				    bint      column);

void btk_clist_set_row_style (BtkCList *clist,
			      bint      row,
			      BtkStyle *style);

BtkStyle *btk_clist_get_row_style (BtkCList *clist,
				   bint      row);

/* this sets a horizontal and vertical shift for drawing
 * the contents of a cell; it can be positive or negitive;
 * this is particulary useful for indenting items in a column
 */
void btk_clist_set_shift (BtkCList *clist,
			  bint      row,
			  bint      column,
			  bint      vertical,
			  bint      horizontal);

/* set/get selectable flag of a single row */
void btk_clist_set_selectable (BtkCList *clist,
			       bint      row,
			       bboolean  selectable);
bboolean btk_clist_get_selectable (BtkCList *clist,
				   bint      row);

/* prepend/append returns the index of the row you just added,
 * making it easier to append and modify a row
 */
bint btk_clist_prepend (BtkCList    *clist,
		        bchar       *text[]);
bint btk_clist_append  (BtkCList    *clist,
			bchar       *text[]);

/* inserts a row at index row and returns the row where it was
 * actually inserted (may be different from "row" in auto_sort mode)
 */
bint btk_clist_insert (BtkCList    *clist,
		       bint         row,
		       bchar       *text[]);

/* removes row at index row */
void btk_clist_remove (BtkCList *clist,
		       bint      row);

/* sets a arbitrary data pointer for a given row */
void btk_clist_set_row_data (BtkCList *clist,
			     bint      row,
			     bpointer  data);

/* sets a data pointer for a given row with destroy notification */
void btk_clist_set_row_data_full (BtkCList       *clist,
			          bint            row,
			          bpointer        data,
				  GDestroyNotify  destroy);

/* returns the data set for a row */
bpointer btk_clist_get_row_data (BtkCList *clist,
				 bint      row);

/* givin a data pointer, find the first (and hopefully only!)
 * row that points to that data, or -1 if none do
 */
bint btk_clist_find_row_from_data (BtkCList *clist,
				   bpointer  data);

/* force selection of a row */
void btk_clist_select_row (BtkCList *clist,
			   bint      row,
			   bint      column);

/* force unselection of a row */
void btk_clist_unselect_row (BtkCList *clist,
			     bint      row,
			     bint      column);

/* undo the last select/unselect operation */
void btk_clist_undo_selection (BtkCList *clist);

/* clear the entire list -- this is much faster than removing
 * each item with btk_clist_remove
 */
void btk_clist_clear (BtkCList *clist);

/* return the row column corresponding to the x and y coordinates,
 * the returned values are only valid if the x and y coordinates
 * are respectively to a window == clist->clist_window
 */
bint btk_clist_get_selection_info (BtkCList *clist,
			     	   bint      x,
			     	   bint      y,
			     	   bint     *row,
			     	   bint     *column);

/* in multiple or extended mode, select all rows */
void btk_clist_select_all (BtkCList *clist);

/* in all modes except browse mode, deselect all rows */
void btk_clist_unselect_all (BtkCList *clist);

/* swap the position of two rows */
void btk_clist_swap_rows (BtkCList *clist,
			  bint      row1,
			  bint      row2);

/* move row from source_row position to dest_row position */
void btk_clist_row_move (BtkCList *clist,
			 bint      source_row,
			 bint      dest_row);

/* sets a compare function different to the default */
void btk_clist_set_compare_func (BtkCList            *clist,
				 BtkCListCompareFunc  cmp_func);

/* the column to sort by */
void btk_clist_set_sort_column (BtkCList *clist,
				bint      column);

/* how to sort : ascending or descending */
void btk_clist_set_sort_type (BtkCList    *clist,
			      BtkSortType  sort_type);

/* sort the list with the current compare function */
void btk_clist_sort (BtkCList *clist);

/* Automatically sort upon insertion */
void btk_clist_set_auto_sort (BtkCList *clist,
			      bboolean  auto_sort);

/* Private function for clist, ctree */

BangoLayout *_btk_clist_create_cell_layout (BtkCList       *clist,
					    BtkCListRow    *clist_row,
					    bint            column);


B_END_DECLS


#endif				/* __BTK_CLIST_H__ */

#endif /* BTK_DISABLE_DEPRECATED */
