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

#include <ctype.h>
#include <string.h>

#undef BDK_DISABLE_DEPRECATED

#include "bdk/bdkkeysyms.h"
#include "bdk/bdki18n.h"

#undef BTK_DISABLE_DEPRECATED

#include "btkmain.h"
#include "btkmarshalers.h"
#include "btkselection.h"
#include "btksignal.h"
#include "btkstyle.h"
#define BTK_ENABLE_BROKEN
#include "btktext.h"
#include "line-wrap.xbm"
#include "line-arrow.xbm"
#include "btkprivate.h"
#include "btkintl.h"

#include "btkalias.h"


#define INITIAL_BUFFER_SIZE      1024
#define INITIAL_LINE_CACHE_SIZE  256
#define MIN_GAP_SIZE             256
#define LINE_DELIM               '\n'
#define MIN_TEXT_WIDTH_LINES     20
#define MIN_TEXT_HEIGHT_LINES    10
#define TEXT_BORDER_ROOM         1
#define LINE_WRAP_ROOM           8           /* The bitmaps are 6 wide. */
#define DEFAULT_TAB_STOP_WIDTH   4
#define SCROLL_PIXELS            5
#define KEY_SCROLL_PIXELS        10
#define SCROLL_TIME              100
#define FREEZE_LENGTH            1024        
/* Freeze text when inserting or deleting more than this many characters */

#define SET_PROPERTY_MARK(m, p, o)  do {                   \
                                      (m)->property = (p); \
			              (m)->offset = (o);   \
			            } while (0)
#define MARK_CURRENT_PROPERTY(mark) ((TextProperty*)(mark)->property->data)
#define MARK_NEXT_PROPERTY(mark)    ((TextProperty*)(mark)->property->next->data)
#define MARK_PREV_PROPERTY(mark)    ((TextProperty*)((mark)->property->prev ?     \
						     (mark)->property->prev->data \
						     : NULL))
#define MARK_PREV_LIST_PTR(mark)    ((mark)->property->prev)
#define MARK_LIST_PTR(mark)         ((mark)->property)
#define MARK_NEXT_LIST_PTR(mark)    ((mark)->property->next)
#define MARK_OFFSET(mark)           ((mark)->offset)
#define MARK_PROPERTY_LENGTH(mark)  (MARK_CURRENT_PROPERTY(mark)->length)


#define MARK_CURRENT_FONT(text, mark) \
  ((MARK_CURRENT_PROPERTY(mark)->flags & PROPERTY_FONT) ? \
         MARK_CURRENT_PROPERTY(mark)->font->bdk_font : \
         btk_style_get_font (BTK_WIDGET (text)->style))
#define MARK_CURRENT_FORE(text, mark) \
  ((MARK_CURRENT_PROPERTY(mark)->flags & PROPERTY_FOREGROUND) ? \
         &MARK_CURRENT_PROPERTY(mark)->fore_color : \
         &((BtkWidget *)text)->style->text[((BtkWidget *)text)->state])
#define MARK_CURRENT_BACK(text, mark) \
  ((MARK_CURRENT_PROPERTY(mark)->flags & PROPERTY_BACKGROUND) ? \
         &MARK_CURRENT_PROPERTY(mark)->back_color : \
         &((BtkWidget *)text)->style->base[((BtkWidget *)text)->state])
#define MARK_CURRENT_TEXT_FONT(text, mark) \
  ((MARK_CURRENT_PROPERTY(mark)->flags & PROPERTY_FONT) ? \
         MARK_CURRENT_PROPERTY(mark)->font : \
         text->current_font)

#define TEXT_LENGTH(t)              ((t)->text_end - (t)->gap_size)
#define FONT_HEIGHT(f)              ((f)->ascent + (f)->descent)
#define LINE_HEIGHT(l)              ((l).font_ascent + (l).font_descent)
#define LINE_CONTAINS(l, i)         ((l).start.index <= (i) && (l).end.index >= (i))
#define LINE_STARTS_AT(l, i)        ((l).start.index == (i))
#define LINE_START_PIXEL(l)         ((l).tab_cont.pixel_offset)
#define LAST_INDEX(t, m)            ((m).index == TEXT_LENGTH(t))
#define CACHE_DATA(c)               (*(LineParams*)(c)->data)

enum {
  PROP_0,
  PROP_HADJUSTMENT,
  PROP_VADJUSTMENT,
  PROP_LINE_WRAP,
  PROP_WORD_WRAP
};

typedef struct _TextProperty          TextProperty;
typedef struct _TabStopMark           TabStopMark;
typedef struct _PrevTabCont           PrevTabCont;
typedef struct _FetchLinesData        FetchLinesData;
typedef struct _LineParams            LineParams;
typedef struct _SetVerticalScrollData SetVerticalScrollData;

typedef gint (*LineIteratorFunction) (BtkText* text, LineParams* lp, void* data);

typedef enum
{
  FetchLinesPixels,
  FetchLinesCount
} FLType;

struct _SetVerticalScrollData {
  gint pixel_height;
  gint last_didnt_wrap;
  gint last_line_start;
  BtkPropertyMark mark;
};

struct _BtkTextFont
{
  /* The actual font. */
  BdkFont *bdk_font;
  guint ref_count;

  gint16 char_widths[256];
};

typedef enum {
  PROPERTY_FONT =       1 << 0,
  PROPERTY_FOREGROUND = 1 << 1,
  PROPERTY_BACKGROUND = 1 << 2
} TextPropertyFlags;

struct _TextProperty
{
  /* Font. */
  BtkTextFont* font;

  /* Background Color. */
  BdkColor back_color;
  
  /* Foreground Color. */
  BdkColor fore_color;

  /* Show which properties are set */
  TextPropertyFlags flags;

  /* Length of this property. */
  guint length;
};

struct _TabStopMark
{
  GList* tab_stops; /* Index into list containing the next tab position.  If
		     * NULL, using default widths. */
  gint to_next_tab;
};

struct _PrevTabCont
{
  guint pixel_offset;
  TabStopMark tab_start;
};

struct _FetchLinesData
{
  GList* new_lines;
  FLType fl_type;
  gint data;
  gint data_max;
};

struct _LineParams
{
  guint font_ascent;
  guint font_descent;
  guint pixel_width;
  guint displayable_chars;
  guint wraps : 1;
  
  PrevTabCont tab_cont;
  PrevTabCont tab_cont_next;
  
  BtkPropertyMark start;
  BtkPropertyMark end;
};


static void  btk_text_class_init     (BtkTextClass   *klass);
static void  btk_text_set_property   (BObject         *object,
				      guint            prop_id,
				      const BValue    *value,
				      BParamSpec      *pspec);
static void  btk_text_get_property   (BObject         *object,
				      guint            prop_id,
				      BValue          *value,
				      BParamSpec      *pspec);
static void  btk_text_editable_init  (BtkEditableClass *iface);
static void  btk_text_init           (BtkText        *text);
static void  btk_text_destroy        (BtkObject      *object);
static void  btk_text_finalize       (BObject        *object);
static void  btk_text_realize        (BtkWidget      *widget);
static void  btk_text_unrealize      (BtkWidget      *widget);
static void  btk_text_style_set	     (BtkWidget      *widget,
				      BtkStyle       *previous_style);
static void  btk_text_state_changed  (BtkWidget      *widget,
				      BtkStateType    previous_state);
static void  btk_text_draw_focus     (BtkWidget      *widget);
static void  btk_text_size_request   (BtkWidget      *widget,
				      BtkRequisition *requisition);
static void  btk_text_size_allocate  (BtkWidget      *widget,
				      BtkAllocation  *allocation);
static void  btk_text_adjustment     (BtkAdjustment  *adjustment,
				      BtkText        *text);
static void   btk_text_insert_text       (BtkEditable    *editable,
					  const gchar    *new_text,
					  gint            new_text_length,
					  gint           *position);
static void   btk_text_delete_text       (BtkEditable    *editable,
					  gint            start_pos,
					  gint            end_pos);
static void   btk_text_update_text       (BtkOldEditable *old_editable,
					  gint            start_pos,
					  gint            end_pos);
static gchar *btk_text_get_chars         (BtkOldEditable *old_editable,
					  gint            start,
					  gint            end);
static void   btk_text_set_selection     (BtkOldEditable *old_editable,
					  gint            start,
					  gint            end);
static void   btk_text_real_set_editable (BtkOldEditable *old_editable,
					  gboolean        is_editable);

static void  btk_text_adjustment_destroyed (BtkAdjustment  *adjustment,
                                            BtkText        *text);

/* Event handlers */
static gint  btk_text_expose            (BtkWidget         *widget,
					 BdkEventExpose    *event);
static gint  btk_text_button_press      (BtkWidget         *widget,
					 BdkEventButton    *event);
static gint  btk_text_button_release    (BtkWidget         *widget,
					 BdkEventButton    *event);
static gint  btk_text_motion_notify     (BtkWidget         *widget,
					 BdkEventMotion    *event);
static gint  btk_text_key_press         (BtkWidget         *widget,
					 BdkEventKey       *event);

static void move_gap (BtkText* text, guint index);
static void make_forward_space (BtkText* text, guint len);

/* Property management */
static BtkTextFont* get_text_font (BdkFont* gfont);
static void         text_font_unref (BtkTextFont *text_font);

static void insert_text_property (BtkText* text, BdkFont* font,
				  const BdkColor *fore, const BdkColor* back, guint len);
static TextProperty* new_text_property (BtkText *text, BdkFont* font, 
					const BdkColor* fore, const BdkColor* back, guint length);
static void destroy_text_property (TextProperty *prop);
static void init_properties      (BtkText *text);
static void realize_property     (BtkText *text, TextProperty *prop);
static void realize_properties   (BtkText *text);
static void unrealize_property   (BtkText *text, TextProperty *prop);
static void unrealize_properties (BtkText *text);

static void delete_text_property (BtkText* text, guint len);

static guint pixel_height_of (BtkText* text, GList* cache_line);

/* Property Movement and Size Computations */
static void advance_mark (BtkPropertyMark* mark);
static void decrement_mark (BtkPropertyMark* mark);
static void advance_mark_n (BtkPropertyMark* mark, gint n);
static void decrement_mark_n (BtkPropertyMark* mark, gint n);
static void move_mark_n (BtkPropertyMark* mark, gint n);
static BtkPropertyMark find_mark (BtkText* text, guint mark_position);
static BtkPropertyMark find_mark_near (BtkText* text, guint mark_position, const BtkPropertyMark* near);
static void find_line_containing_point (BtkText* text, guint point,
					gboolean scroll);

/* Display */
static void compute_lines_pixels (BtkText* text, guint char_count,
				  guint *lines, guint *pixels);

static gint total_line_height (BtkText* text,
			       GList* line,
			       gint line_count);
static LineParams find_line_params (BtkText* text,
				    const BtkPropertyMark *mark,
				    const PrevTabCont *tab_cont,
				    PrevTabCont *next_cont);
static void recompute_geometry (BtkText* text);
static void insert_expose (BtkText* text, guint old_pixels, gint nchars, guint new_line_count);
static void delete_expose (BtkText* text,
			   guint nchars,
			   guint old_lines, 
			   guint old_pixels);
static BdkGC *create_bg_gc (BtkText *text);
static void clear_area (BtkText *text, BdkRectangle *area);
static void draw_line (BtkText* text,
		       gint pixel_height,
		       LineParams* lp);
static void draw_line_wrap (BtkText* text,
			    guint height);
static void draw_cursor (BtkText* text, gint absolute);
static void undraw_cursor (BtkText* text, gint absolute);
static gint drawn_cursor_min (BtkText* text);
static gint drawn_cursor_max (BtkText* text);
static void expose_text (BtkText* text, BdkRectangle *area, gboolean cursor);

/* Search and Placement. */
static void find_cursor (BtkText* text,
			 gboolean scroll);
static void find_cursor_at_line (BtkText* text,
				 const LineParams* start_line,
				 gint pixel_height);
static void find_mouse_cursor (BtkText* text, gint x, gint y);

/* Scrolling. */
static void adjust_adj  (BtkText* text, BtkAdjustment* adj);
static void scroll_up   (BtkText* text, gint diff);
static void scroll_down (BtkText* text, gint diff);
static void scroll_int  (BtkText* text, gint diff);

static void process_exposes (BtkText *text);

/* Cache Management. */
static void   free_cache        (BtkText* text);
static GList* remove_cache_line (BtkText* text, GList* list);

/* Key Motion. */
static void move_cursor_buffer_ver (BtkText *text, int dir);
static void move_cursor_page_ver (BtkText *text, int dir);
static void move_cursor_ver (BtkText *text, int count);
static void move_cursor_hor (BtkText *text, int count);

/* Binding actions */
static void btk_text_move_cursor    (BtkOldEditable *old_editable,
				     gint            x,
				     gint            y);
static void btk_text_move_word      (BtkOldEditable *old_editable,
				     gint            n);
static void btk_text_move_page      (BtkOldEditable *old_editable,
				     gint            x,
				     gint            y);
static void btk_text_move_to_row    (BtkOldEditable *old_editable,
				     gint            row);
static void btk_text_move_to_column (BtkOldEditable *old_editable,
				     gint            row);
static void btk_text_kill_char      (BtkOldEditable *old_editable,
				     gint            direction);
static void btk_text_kill_word      (BtkOldEditable *old_editable,
				     gint            direction);
static void btk_text_kill_line      (BtkOldEditable *old_editable,
				     gint            direction);

/* To be removed */
static void btk_text_move_forward_character    (BtkText          *text);
static void btk_text_move_backward_character   (BtkText          *text);
static void btk_text_move_forward_word         (BtkText          *text);
static void btk_text_move_backward_word        (BtkText          *text);
static void btk_text_move_beginning_of_line    (BtkText          *text);
static void btk_text_move_end_of_line          (BtkText          *text);
static void btk_text_move_next_line            (BtkText          *text);
static void btk_text_move_previous_line        (BtkText          *text);

static void btk_text_delete_forward_character  (BtkText          *text);
static void btk_text_delete_backward_character (BtkText          *text);
static void btk_text_delete_forward_word       (BtkText          *text);
static void btk_text_delete_backward_word      (BtkText          *text);
static void btk_text_delete_line               (BtkText          *text);
static void btk_text_delete_to_line_end        (BtkText          *text);
static void btk_text_select_word               (BtkText          *text,
						guint32           time);
static void btk_text_select_line               (BtkText          *text,
						guint32           time);

static void btk_text_set_position (BtkOldEditable *old_editable,
				   gint            position);

/* #define DEBUG_BTK_TEXT */

#if defined(DEBUG_BTK_TEXT) && defined(__GNUC__)
/* Debugging utilities. */
static void btk_text_assert_mark (BtkText         *text,
				  BtkPropertyMark *mark,
				  BtkPropertyMark *before,
				  BtkPropertyMark *after,
				  const gchar     *msg,
				  const gchar     *where,
				  gint             line);

static void btk_text_assert (BtkText         *text,
			     const gchar     *msg,
			     gint             line);
static void btk_text_show_cache_line (BtkText *text, GList *cache,
				      const char* what, const char* func, gint line);
static void btk_text_show_cache (BtkText *text, const char* func, gint line);
static void btk_text_show_adj (BtkText *text,
			       BtkAdjustment *adj,
			       const char* what,
			       const char* func,
			       gint line);
static void btk_text_show_props (BtkText* test,
				 const char* func,
				 int line);

#define TDEBUG(args) g_message args
#define TEXT_ASSERT(text) btk_text_assert (text,__PRETTY_FUNCTION__,__LINE__)
#define TEXT_ASSERT_MARK(text,mark,msg) btk_text_assert_mark (text,mark, \
					   __PRETTY_FUNCTION__,msg,__LINE__)
#define TEXT_SHOW(text) btk_text_show_cache (text, __PRETTY_FUNCTION__,__LINE__)
#define TEXT_SHOW_LINE(text,line,msg) btk_text_show_cache_line (text,line,msg,\
					   __PRETTY_FUNCTION__,__LINE__)
#define TEXT_SHOW_ADJ(text,adj,msg) btk_text_show_adj (text,adj,msg, \
					  __PRETTY_FUNCTION__,__LINE__)
#else
#define TDEBUG(args)
#define TEXT_ASSERT(text)
#define TEXT_ASSERT_MARK(text,mark,msg)
#define TEXT_SHOW(text)
#define TEXT_SHOW_LINE(text,line,msg)
#define TEXT_SHOW_ADJ(text,adj,msg)
#endif

static BtkWidgetClass *parent_class = NULL;

/**********************************************************************/
/*			        Widget Crap                           */
/**********************************************************************/

BtkType
btk_text_get_type (void)
{
  static BtkType text_type = 0;
  
  if (!text_type)
    {
      static const BtkTypeInfo text_info =
      {
	"BtkText",
	sizeof (BtkText),
	sizeof (BtkTextClass),
	(BtkClassInitFunc) btk_text_class_init,
	(BtkObjectInitFunc) btk_text_init,
	/* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (BtkClassInitFunc) NULL,
      };

      const GInterfaceInfo editable_info =
      {
	(GInterfaceInitFunc) btk_text_editable_init, /* interface_init */
	NULL, /* interface_finalize */
	NULL  /* interface_data */
      };
      
      I_("BtkText");
      text_type = btk_type_unique (BTK_TYPE_OLD_EDITABLE, &text_info);
      g_type_add_interface_static (text_type,
				   BTK_TYPE_EDITABLE,
				   &editable_info);
    }
  
  return text_type;
}

static void
btk_text_class_init (BtkTextClass *class)
{
  BObjectClass *bobject_class;
  BtkObjectClass *object_class;
  BtkWidgetClass *widget_class;
  BtkOldEditableClass *old_editable_class;

  bobject_class = B_OBJECT_CLASS (class);
  object_class = (BtkObjectClass*) class;
  widget_class = (BtkWidgetClass*) class;
  old_editable_class = (BtkOldEditableClass*) class;
  parent_class = btk_type_class (BTK_TYPE_OLD_EDITABLE);

  bobject_class->finalize = btk_text_finalize;
  bobject_class->set_property = btk_text_set_property;
  bobject_class->get_property = btk_text_get_property;
  
  object_class->destroy = btk_text_destroy;
  
  widget_class->realize = btk_text_realize;
  widget_class->unrealize = btk_text_unrealize;
  widget_class->style_set = btk_text_style_set;
  widget_class->state_changed = btk_text_state_changed;
  widget_class->size_request = btk_text_size_request;
  widget_class->size_allocate = btk_text_size_allocate;
  widget_class->expose_event = btk_text_expose;
  widget_class->button_press_event = btk_text_button_press;
  widget_class->button_release_event = btk_text_button_release;
  widget_class->motion_notify_event = btk_text_motion_notify;
  widget_class->key_press_event = btk_text_key_press;

  old_editable_class->set_editable = btk_text_real_set_editable;
  
  old_editable_class->move_cursor = btk_text_move_cursor;
  old_editable_class->move_word = btk_text_move_word;
  old_editable_class->move_page = btk_text_move_page;
  old_editable_class->move_to_row = btk_text_move_to_row;
  old_editable_class->move_to_column = btk_text_move_to_column;
  
  old_editable_class->kill_char = btk_text_kill_char;
  old_editable_class->kill_word = btk_text_kill_word;
  old_editable_class->kill_line = btk_text_kill_line;
  
  old_editable_class->update_text = btk_text_update_text;
  old_editable_class->get_chars   = btk_text_get_chars;
  old_editable_class->set_selection = btk_text_set_selection;
  old_editable_class->set_position = btk_text_set_position;

  class->set_scroll_adjustments = btk_text_set_adjustments;

  g_object_class_install_property (bobject_class,
                                   PROP_HADJUSTMENT,
                                   g_param_spec_object ("hadjustment",
                                                        P_("Horizontal Adjustment"),
                                                        P_("Horizontal adjustment for the text widget"),
                                                        BTK_TYPE_ADJUSTMENT,
                                                        BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_VADJUSTMENT,
                                   g_param_spec_object ("vadjustment",
                                                        P_("Vertical Adjustment"),
                                                        P_("Vertical adjustment for the text widget"),
                                                        BTK_TYPE_ADJUSTMENT,
                                                        BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_LINE_WRAP,
                                   g_param_spec_boolean ("line-wrap",
							 P_("Line Wrap"),
							 P_("Whether lines are wrapped at widget edges"),
							 TRUE,
							 BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_WORD_WRAP,
                                   g_param_spec_boolean ("word-wrap",
							 P_("Word Wrap"),
							 P_("Whether words are wrapped at widget edges"),
							 FALSE,
							 BTK_PARAM_READWRITE));

  widget_class->set_scroll_adjustments_signal =
    btk_signal_new (I_("set-scroll-adjustments"),
		    BTK_RUN_LAST,
		    BTK_CLASS_TYPE (object_class),
		    BTK_SIGNAL_OFFSET (BtkTextClass, set_scroll_adjustments),
		    _btk_marshal_VOID__OBJECT_OBJECT,
		    BTK_TYPE_NONE, 2, BTK_TYPE_ADJUSTMENT, BTK_TYPE_ADJUSTMENT);
}

static void
btk_text_set_property (BObject         *object,
		       guint            prop_id,
		       const BValue    *value,
		       BParamSpec      *pspec)
{
  BtkText *text;
  
  text = BTK_TEXT (object);
  
  switch (prop_id)
    {
    case PROP_HADJUSTMENT:
      btk_text_set_adjustments (text,
				b_value_get_object (value),
				text->vadj);
      break;
    case PROP_VADJUSTMENT:
      btk_text_set_adjustments (text,
				text->hadj,
				b_value_get_object (value));
      break;
    case PROP_LINE_WRAP:
      btk_text_set_line_wrap (text, b_value_get_boolean (value));
      break;
    case PROP_WORD_WRAP:
      btk_text_set_word_wrap (text, b_value_get_boolean (value));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_text_get_property (BObject         *object,
		       guint            prop_id,
		       BValue          *value,
		       BParamSpec      *pspec)
{
  BtkText *text;
  
  text = BTK_TEXT (object);
  
  switch (prop_id)
    {
    case PROP_HADJUSTMENT:
      b_value_set_object (value, text->hadj);
      break;
    case PROP_VADJUSTMENT:
      b_value_set_object (value, text->vadj);
      break;
    case PROP_LINE_WRAP:
      b_value_set_boolean (value, text->line_wrap);
      break;
    case PROP_WORD_WRAP:
      b_value_set_boolean (value, text->word_wrap);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_text_editable_init (BtkEditableClass *iface)
{
  iface->insert_text = btk_text_insert_text;
  iface->delete_text = btk_text_delete_text;
}

static void
btk_text_init (BtkText *text)
{
  btk_widget_set_can_focus (BTK_WIDGET (text), TRUE);

  text->text_area = NULL;
  text->hadj = NULL;
  text->vadj = NULL;
  text->gc = NULL;
  text->bg_gc = NULL;
  text->line_wrap_bitmap = NULL;
  text->line_arrow_bitmap = NULL;
  
  text->use_wchar = FALSE;
  text->text.ch = g_new (guchar, INITIAL_BUFFER_SIZE);
  text->text_len = INITIAL_BUFFER_SIZE;
 
  text->scratch_buffer.ch = NULL;
  text->scratch_buffer_len = 0;
 
  text->freeze_count = 0;
  
  text->default_tab_width = 4;
  text->tab_stops = NULL;
  
  text->tab_stops = g_list_prepend (text->tab_stops, (void*)8);
  text->tab_stops = g_list_prepend (text->tab_stops, (void*)8);
  
  text->line_start_cache = NULL;
  text->first_cut_pixels = 0;
  
  text->line_wrap = TRUE;
  text->word_wrap = FALSE;
  
  text->timer = 0;
  text->button = 0;
  
  text->current_font = NULL;
  
  init_properties (text);
  
  BTK_OLD_EDITABLE (text)->editable = FALSE;
  
  btk_text_set_adjustments (text, NULL, NULL);
  btk_editable_set_position (BTK_EDITABLE (text), 0);
}

BtkWidget*
btk_text_new (BtkAdjustment *hadj,
	      BtkAdjustment *vadj)
{
  BtkWidget *text;

  if (hadj)
    g_return_val_if_fail (BTK_IS_ADJUSTMENT (hadj), NULL);
  if (vadj)
    g_return_val_if_fail (BTK_IS_ADJUSTMENT (vadj), NULL);

  text = g_object_new (BTK_TYPE_TEXT,
			 "hadjustment", hadj,
			 "vadjustment", vadj,
			 NULL);

  return text;
}

void
btk_text_set_word_wrap (BtkText *text,
			gboolean word_wrap)
{
  g_return_if_fail (BTK_IS_TEXT (text));
  
  text->word_wrap = (word_wrap != FALSE);
  
  if (btk_widget_get_realized (BTK_WIDGET (text)))
    {
      recompute_geometry (text);
      btk_widget_queue_draw (BTK_WIDGET (text));
    }
  
  g_object_notify (B_OBJECT (text), "word-wrap");
}

void
btk_text_set_line_wrap (BtkText *text,
			gboolean line_wrap)
{
  g_return_if_fail (BTK_IS_TEXT (text));
  
  text->line_wrap = (line_wrap != FALSE);
  
  if (btk_widget_get_realized (BTK_WIDGET (text)))
    {
      recompute_geometry (text);
      btk_widget_queue_draw (BTK_WIDGET (text));
    }
  
  g_object_notify (B_OBJECT (text), "line-wrap");
}

void
btk_text_set_editable (BtkText *text,
		       gboolean is_editable)
{
  g_return_if_fail (BTK_IS_TEXT (text));
  
  btk_editable_set_editable (BTK_EDITABLE (text), is_editable);
}

static void
btk_text_real_set_editable (BtkOldEditable *old_editable,
			    gboolean        is_editable)
{
  BtkText *text;
  
  g_return_if_fail (BTK_IS_TEXT (old_editable));
  
  text = BTK_TEXT (old_editable);
  
  old_editable->editable = (is_editable != FALSE);
  
  if (is_editable)
    draw_cursor (text, TRUE);
  else
    undraw_cursor (text, TRUE);
}

void
btk_text_set_adjustments (BtkText       *text,
			  BtkAdjustment *hadj,
			  BtkAdjustment *vadj)
{
  g_return_if_fail (BTK_IS_TEXT (text));
  if (hadj)
    g_return_if_fail (BTK_IS_ADJUSTMENT (hadj));
  else
    hadj = BTK_ADJUSTMENT (btk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
  if (vadj)
    g_return_if_fail (BTK_IS_ADJUSTMENT (vadj));
  else
    vadj = BTK_ADJUSTMENT (btk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
  
  if (text->hadj && (text->hadj != hadj))
    {
      btk_signal_disconnect_by_data (BTK_OBJECT (text->hadj), text);
      g_object_unref (text->hadj);
    }
  
  if (text->vadj && (text->vadj != vadj))
    {
      btk_signal_disconnect_by_data (BTK_OBJECT (text->vadj), text);
      g_object_unref (text->vadj);
    }
  
  g_object_freeze_notify (B_OBJECT (text));
  if (text->hadj != hadj)
    {
      text->hadj = hadj;
      g_object_ref_sink (text->hadj);
      
      btk_signal_connect (BTK_OBJECT (text->hadj), "changed",
			  G_CALLBACK (btk_text_adjustment),
			  text);
      btk_signal_connect (BTK_OBJECT (text->hadj), "value-changed",
			  G_CALLBACK (btk_text_adjustment),
			  text);
      btk_signal_connect (BTK_OBJECT (text->hadj), "destroy",
			  G_CALLBACK (btk_text_adjustment_destroyed),
			  text);
      btk_text_adjustment (hadj, text);

      g_object_notify (B_OBJECT (text), "hadjustment");
    }
  
  if (text->vadj != vadj)
    {
      text->vadj = vadj;
      g_object_ref_sink (text->vadj);
      
      btk_signal_connect (BTK_OBJECT (text->vadj), "changed",
			  G_CALLBACK (btk_text_adjustment),
			  text);
      btk_signal_connect (BTK_OBJECT (text->vadj), "value-changed",
			  G_CALLBACK (btk_text_adjustment),
			  text);
      btk_signal_connect (BTK_OBJECT (text->vadj), "destroy",
			  G_CALLBACK (btk_text_adjustment_destroyed),
			  text);
      btk_text_adjustment (vadj, text);

      g_object_notify (B_OBJECT (text), "vadjustment");
    }
  g_object_thaw_notify (B_OBJECT (text));
}

void
btk_text_set_point (BtkText *text,
		    guint    index)
{
  g_return_if_fail (BTK_IS_TEXT (text));
  g_return_if_fail (index <= TEXT_LENGTH (text));
  
  text->point = find_mark (text, index);
}

guint
btk_text_get_point (BtkText *text)
{
  g_return_val_if_fail (BTK_IS_TEXT (text), 0);
  
  return text->point.index;
}

guint
btk_text_get_length (BtkText *text)
{
  g_return_val_if_fail (BTK_IS_TEXT (text), 0);
  
  return TEXT_LENGTH (text);
}

void
btk_text_freeze (BtkText *text)
{
  g_return_if_fail (BTK_IS_TEXT (text));
  
  text->freeze_count++;
}

void
btk_text_thaw (BtkText *text)
{
  g_return_if_fail (BTK_IS_TEXT (text));
  
  if (text->freeze_count)
    if (!(--text->freeze_count) && btk_widget_get_realized (BTK_WIDGET (text)))
      {
	recompute_geometry (text);
	btk_widget_queue_draw (BTK_WIDGET (text));
      }
}

void
btk_text_insert (BtkText        *text,
		 BdkFont        *font,
		 const BdkColor *fore,
		 const BdkColor *back,
		 const char     *chars,
		 gint            nchars)
{
  BtkOldEditable *old_editable = BTK_OLD_EDITABLE (text);
  gboolean frozen = FALSE;
  
  gint new_line_count = 1;
  guint old_height = 0;
  guint length;
  guint i;
  gint numwcs;
  
  g_return_if_fail (BTK_IS_TEXT (text));

  if (nchars < 0)
    length = strlen (chars);
  else
    length = nchars;
  
  if (length == 0)
    return;
  
  if (!text->freeze_count && (length > FREEZE_LENGTH))
    {
      btk_text_freeze (text);
      frozen = TRUE;
    }
  
  if (!text->freeze_count && (text->line_start_cache != NULL))
    {
      find_line_containing_point (text, text->point.index, TRUE);
      old_height = total_line_height (text, text->current_line, 1);
    }
  
  if ((TEXT_LENGTH (text) == 0) && (text->use_wchar == FALSE))
    {
      BtkWidget *widget = BTK_WIDGET (text);
      
      btk_widget_ensure_style (widget);
      if ((widget->style) &&
	  (btk_style_get_font (widget->style)->type == BDK_FONT_FONTSET))
 	{
 	  text->use_wchar = TRUE;
 	  g_free (text->text.ch);
 	  text->text.wc = g_new (BdkWChar, INITIAL_BUFFER_SIZE);
 	  text->text_len = INITIAL_BUFFER_SIZE;
 	  g_free (text->scratch_buffer.ch);
 	  text->scratch_buffer.wc = NULL;
 	  text->scratch_buffer_len = 0;
 	}
    }
 
  move_gap (text, text->point.index);
  make_forward_space (text, length);
 
  if (text->use_wchar)
    {
      char *chars_nt = (char *)chars;
      if (nchars > 0)
	{
	  chars_nt = g_new (char, length+1);
	  memcpy (chars_nt, chars, length);
	  chars_nt[length] = 0;
	}
      numwcs = bdk_mbstowcs (text->text.wc + text->gap_position, chars_nt,
 			     length);
      if (chars_nt != chars)
	g_free(chars_nt);
      if (numwcs < 0)
	numwcs = 0;
    }
  else
    {
      numwcs = length;
      memcpy(text->text.ch + text->gap_position, chars, length);
    }
 
  if (!text->freeze_count && (text->line_start_cache != NULL))
    {
      if (text->use_wchar)
 	{
 	  for (i=0; i<numwcs; i++)
 	    if (text->text.wc[text->gap_position + i] == '\n')
 	      new_line_count++;
	}
      else
 	{
 	  for (i=0; i<numwcs; i++)
 	    if (text->text.ch[text->gap_position + i] == '\n')
 	      new_line_count++;
 	}
    }
 
  if (numwcs > 0)
    {
      insert_text_property (text, font, fore, back, numwcs);
   
      text->gap_size -= numwcs;
      text->gap_position += numwcs;
   
      if (text->point.index < text->first_line_start_index)
 	text->first_line_start_index += numwcs;
      if (text->point.index < old_editable->selection_start_pos)
 	old_editable->selection_start_pos += numwcs;
      if (text->point.index < old_editable->selection_end_pos)
 	old_editable->selection_end_pos += numwcs;
      /* We'll reset the cursor later anyways if we aren't frozen */
      if (text->point.index < text->cursor_mark.index)
 	text->cursor_mark.index += numwcs;
  
      advance_mark_n (&text->point, numwcs);
  
      if (!text->freeze_count && (text->line_start_cache != NULL))
	insert_expose (text, old_height, numwcs, new_line_count);
    }

  if (frozen)
    btk_text_thaw (text);
}

gboolean
btk_text_backward_delete (BtkText *text,
			  guint    nchars)
{
  g_return_val_if_fail (BTK_IS_TEXT (text), FALSE);
  
  if (nchars > text->point.index || nchars <= 0)
    return FALSE;
  
  btk_text_set_point (text, text->point.index - nchars);
  
  return btk_text_forward_delete (text, nchars);
}

gboolean
btk_text_forward_delete (BtkText *text,
			 guint    nchars)
{
  guint old_lines = 0, old_height = 0;
  BtkOldEditable *old_editable = BTK_OLD_EDITABLE (text);
  gboolean frozen = FALSE;
  
  g_return_val_if_fail (BTK_IS_TEXT (text), FALSE);
  
  if (text->point.index + nchars > TEXT_LENGTH (text) || nchars <= 0)
    return FALSE;
  
  if (!text->freeze_count && nchars > FREEZE_LENGTH)
    {
      btk_text_freeze (text);
      frozen = TRUE;
    }
  
  if (!text->freeze_count && text->line_start_cache != NULL)
    {
      /* We need to undraw the cursor here, since we may later
       * delete the cursor's property
       */
      undraw_cursor (text, FALSE);
      find_line_containing_point (text, text->point.index, TRUE);
      compute_lines_pixels (text, nchars, &old_lines, &old_height);
    }
  
  /* FIXME, or resizing after deleting will be odd */
  if (text->point.index < text->first_line_start_index)
    {
      if (text->point.index + nchars >= text->first_line_start_index)
	{
	  text->first_line_start_index = text->point.index;
	  while ((text->first_line_start_index > 0) &&
		 (BTK_TEXT_INDEX (text, text->first_line_start_index - 1)
		  != LINE_DELIM))
	    text->first_line_start_index -= 1;
	  
	}
      else
	text->first_line_start_index -= nchars;
    }
  
  if (text->point.index < old_editable->selection_start_pos)
    old_editable->selection_start_pos -= 
      MIN(nchars, old_editable->selection_start_pos - text->point.index);
  if (text->point.index < old_editable->selection_end_pos)
    old_editable->selection_end_pos -= 
      MIN(nchars, old_editable->selection_end_pos - text->point.index);
  /* We'll reset the cursor later anyways if we aren't frozen */
  if (text->point.index < text->cursor_mark.index)
    move_mark_n (&text->cursor_mark, 
		 -MIN(nchars, text->cursor_mark.index - text->point.index));
  
  move_gap (text, text->point.index);
  
  text->gap_size += nchars;
  
  delete_text_property (text, nchars);
  
  if (!text->freeze_count && (text->line_start_cache != NULL))
    {
      delete_expose (text, nchars, old_lines, old_height);
      draw_cursor (text, FALSE);
    }
  
  if (frozen)
    btk_text_thaw (text);
  
  return TRUE;
}

static void
btk_text_set_position (BtkOldEditable *old_editable,
		       gint            position)
{
  BtkText *text = (BtkText *) old_editable;

  if (position < 0)
    position = btk_text_get_length (text);                                    
  
  undraw_cursor (text, FALSE);
  text->cursor_mark = find_mark (text, position);
  find_cursor (text, TRUE);
  draw_cursor (text, FALSE);
  btk_editable_select_rebunnyion (BTK_EDITABLE (old_editable), 0, 0);
}

static gchar *    
btk_text_get_chars (BtkOldEditable *old_editable,
		    gint            start_pos,
		    gint            end_pos)
{
  BtkText *text;

  gchar *retval;
  
  g_return_val_if_fail (BTK_IS_TEXT (old_editable), NULL);
  text = BTK_TEXT (old_editable);
  
  if (end_pos < 0)
    end_pos = TEXT_LENGTH (text);
  
  if ((start_pos < 0) || 
      (end_pos > TEXT_LENGTH (text)) || 
      (end_pos < start_pos))
    return NULL;
  
  move_gap (text, TEXT_LENGTH (text));
  make_forward_space (text, 1);

  if (text->use_wchar)
    {
      BdkWChar ch;
      ch = text->text.wc[end_pos];
      text->text.wc[end_pos] = 0;
      retval = bdk_wcstombs (text->text.wc + start_pos);
      text->text.wc[end_pos] = ch;
    }
  else
    {
      guchar ch;
      ch = text->text.ch[end_pos];
      text->text.ch[end_pos] = 0;
      retval = g_strdup ((gchar *)(text->text.ch + start_pos));
      text->text.ch[end_pos] = ch;
    }

  return retval;
}


static void
btk_text_destroy (BtkObject *object)
{
  BtkText *text = BTK_TEXT (object);

  if (text->hadj)
    {
      btk_signal_disconnect_by_data (BTK_OBJECT (text->hadj), text);
      g_object_unref (text->hadj);
      text->hadj = NULL;
    }
  if (text->vadj)
    {
      btk_signal_disconnect_by_data (BTK_OBJECT (text->vadj), text);
      g_object_unref (text->vadj);
      text->vadj = NULL;
    }

  if (text->timer)
    {
      g_source_remove (text->timer);
      text->timer = 0;
    }
  
  BTK_OBJECT_CLASS(parent_class)->destroy (object);
}

static void
btk_text_finalize (BObject *object)
{
  BtkText *text = BTK_TEXT (object);
  GList *tmp_list;

  /* Clean up the internal structures */
  if (text->use_wchar)
    g_free (text->text.wc);
  else
    g_free (text->text.ch);
  
  tmp_list = text->text_properties;
  while (tmp_list)
    {
      destroy_text_property (tmp_list->data);
      tmp_list = tmp_list->next;
    }

  if (text->current_font)
    text_font_unref (text->current_font);
  
  g_list_free (text->text_properties);
  
  if (text->use_wchar)
    {
      g_free (text->scratch_buffer.wc);
    }
  else
    {
      g_free (text->scratch_buffer.ch);
    }
  
  g_list_free (text->tab_stops);
  
  B_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
btk_text_realize (BtkWidget *widget)
{
  BtkText *text = BTK_TEXT (widget);
  BtkOldEditable *old_editable = BTK_OLD_EDITABLE (widget);
  BdkWindowAttr attributes;
  gint attributes_mask;

  btk_widget_set_realized (widget, TRUE);
  
  attributes.window_type = BDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = BDK_INPUT_OUTPUT;
  attributes.visual = btk_widget_get_visual (widget);
  attributes.colormap = btk_widget_get_colormap (widget);
  attributes.event_mask = btk_widget_get_events (widget);
  attributes.event_mask |= (BDK_EXPOSURE_MASK |
			    BDK_BUTTON_PRESS_MASK |
			    BDK_BUTTON_RELEASE_MASK |
			    BDK_BUTTON_MOTION_MASK |
			    BDK_ENTER_NOTIFY_MASK |
			    BDK_LEAVE_NOTIFY_MASK |
			    BDK_KEY_PRESS_MASK);
  attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_VISUAL | BDK_WA_COLORMAP;
  
  widget->window = bdk_window_new (btk_widget_get_parent_window (widget), &attributes, attributes_mask);
  bdk_window_set_user_data (widget->window, text);
  
  attributes.x = (widget->style->xthickness + TEXT_BORDER_ROOM);
  attributes.y = (widget->style->ythickness + TEXT_BORDER_ROOM);
  attributes.width = MAX (1, (gint)widget->allocation.width - (gint)attributes.x * 2);
  attributes.height = MAX (1, (gint)widget->allocation.height - (gint)attributes.y * 2);

  attributes.cursor = bdk_cursor_new_for_display (btk_widget_get_display (widget), BDK_XTERM);
  attributes_mask |= BDK_WA_CURSOR;
  
  text->text_area = bdk_window_new (widget->window, &attributes, attributes_mask);
  bdk_window_set_user_data (text->text_area, text);

  bdk_cursor_unref (attributes.cursor); /* The X server will keep it around as long as necessary */
  
  widget->style = btk_style_attach (widget->style, widget->window);
  
  /* Can't call btk_style_set_background here because it's handled specially */
  bdk_window_set_background (widget->window, &widget->style->base[btk_widget_get_state (widget)]);
  bdk_window_set_background (text->text_area, &widget->style->base[btk_widget_get_state (widget)]);

  if (widget->style->bg_pixmap[BTK_STATE_NORMAL])
    text->bg_gc = create_bg_gc (text);
  
  text->line_wrap_bitmap = bdk_bitmap_create_from_data (text->text_area,
							(gchar*) line_wrap_bits,
							line_wrap_width,
							line_wrap_height);
  
  text->line_arrow_bitmap = bdk_bitmap_create_from_data (text->text_area,
							 (gchar*) line_arrow_bits,
							 line_arrow_width,
							 line_arrow_height);
  
  text->gc = bdk_gc_new (text->text_area);
  bdk_gc_set_exposures (text->gc, TRUE);
  bdk_gc_set_foreground (text->gc, &widget->style->text[BTK_STATE_NORMAL]);
  
  realize_properties (text);
  bdk_window_show (text->text_area);
  init_properties (text);

  if (old_editable->selection_start_pos != old_editable->selection_end_pos)
    btk_old_editable_claim_selection (old_editable, TRUE, BDK_CURRENT_TIME);
  
  recompute_geometry (text);
}

static void 
btk_text_style_set (BtkWidget *widget,
		    BtkStyle  *previous_style)
{
  BtkText *text = BTK_TEXT (widget);

  if (btk_widget_get_realized (widget))
    {
      bdk_window_set_background (widget->window, &widget->style->base[btk_widget_get_state (widget)]);
      bdk_window_set_background (text->text_area, &widget->style->base[btk_widget_get_state (widget)]);
      
      if (text->bg_gc)
	{
	  g_object_unref (text->bg_gc);
	  text->bg_gc = NULL;
	}

      if (widget->style->bg_pixmap[BTK_STATE_NORMAL])
	text->bg_gc = create_bg_gc (text);

      recompute_geometry (text);
    }
  
  if (text->current_font)
    text_font_unref (text->current_font);
  text->current_font = get_text_font (btk_style_get_font (widget->style));
}

static void
btk_text_state_changed (BtkWidget   *widget,
			BtkStateType previous_state)
{
  BtkText *text = BTK_TEXT (widget);
  
  if (btk_widget_get_realized (widget))
    {
      bdk_window_set_background (widget->window, &widget->style->base[btk_widget_get_state (widget)]);
      bdk_window_set_background (text->text_area, &widget->style->base[btk_widget_get_state (widget)]);
    }
}

static void
btk_text_unrealize (BtkWidget *widget)
{
  BtkText *text = BTK_TEXT (widget);

  bdk_window_set_user_data (text->text_area, NULL);
  bdk_window_destroy (text->text_area);
  text->text_area = NULL;
  
  g_object_unref (text->gc);
  text->gc = NULL;

  if (text->bg_gc)
    {
      g_object_unref (text->bg_gc);
      text->bg_gc = NULL;
    }
  
  g_object_unref (text->line_wrap_bitmap);
  g_object_unref (text->line_arrow_bitmap);

  unrealize_properties (text);

  free_cache (text);

  BTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static void
clear_focus_area (BtkText *text, gint area_x, gint area_y, gint area_width, gint area_height)
{
  BtkWidget *widget = BTK_WIDGET (text);
  BdkGC *gc;
 
  gint ythick = TEXT_BORDER_ROOM + widget->style->ythickness;
  gint xthick = TEXT_BORDER_ROOM + widget->style->xthickness;
  
  gint width, height;
  
  if (area_width == 0 || area_height == 0)
    return;
    
  if (widget->style->bg_pixmap[BTK_STATE_NORMAL])
    {
      bdk_drawable_get_size (widget->style->bg_pixmap[BTK_STATE_NORMAL], &width, &height);
  
      bdk_gc_set_ts_origin (text->bg_gc,
			    (- text->first_onscreen_hor_pixel + xthick) % width,
			    (- text->first_onscreen_ver_pixel + ythick) % height);
      
       gc = text->bg_gc;
    }
  else
    gc = widget->style->base_gc[widget->state];

  bdk_draw_rectangle (BTK_WIDGET (text)->window, gc, TRUE,
		      area_x, area_y, area_width, area_height);
}

static void
btk_text_draw_focus (BtkWidget *widget)
{
  BtkText *text;
  gint width, height;
  gint x, y;
  
  g_return_if_fail (BTK_IS_TEXT (widget));
  
  text = BTK_TEXT (widget);
  
  if (BTK_WIDGET_DRAWABLE (widget))
    {
      gint ythick = widget->style->ythickness;
      gint xthick = widget->style->xthickness;
      gint xextra = TEXT_BORDER_ROOM;
      gint yextra = TEXT_BORDER_ROOM;
      
      TDEBUG (("in btk_text_draw_focus\n"));
      
      x = 0;
      y = 0;
      width = widget->allocation.width;
      height = widget->allocation.height;
      
      if (btk_widget_has_focus (widget))
	{
	  x += 1;
	  y += 1;
	  width -=  2;
	  height -= 2;
	  xextra -= 1;
	  yextra -= 1;

	  btk_paint_focus (widget->style, widget->window, btk_widget_get_state (widget),
			   NULL, widget, "text",
			   0, 0,
			   widget->allocation.width,
			   widget->allocation.height);
	}

      btk_paint_shadow (widget->style, widget->window,
			BTK_STATE_NORMAL, BTK_SHADOW_IN,
			NULL, widget, "text",
			x, y, width, height);

      x += xthick; 
      y += ythick;
      width -= 2 * xthick;
      height -= 2 * ythick;
      
      /* top rect */
      clear_focus_area (text, x, y, width, yextra);
      /* left rect */
      clear_focus_area (text, x, y + yextra, 
			xextra, y + height - 2 * yextra);
      /* right rect */
      clear_focus_area (text, x + width - xextra, y + yextra, 
			xextra, height - 2 * ythick);
      /* bottom rect */
      clear_focus_area (text, x, x + height - yextra, width, yextra);
    }
  else
    {
      TDEBUG (("in btk_text_draw_focus (undrawable !!!)\n"));
    }
}

static void
btk_text_size_request (BtkWidget      *widget,
		       BtkRequisition *requisition)
{
  BdkFont *font;
  gint xthickness;
  gint ythickness;
  gint char_height;
  gint char_width;

  xthickness = widget->style->xthickness + TEXT_BORDER_ROOM;
  ythickness = widget->style->ythickness + TEXT_BORDER_ROOM;

  font = btk_style_get_font (widget->style);
  
  char_height = MIN_TEXT_HEIGHT_LINES * (font->ascent +
					 font->descent);
  
  char_width = MIN_TEXT_WIDTH_LINES * (bdk_text_width (font,
						       "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
						       26)
				       / 26);
  
  requisition->width  = char_width  + xthickness * 2;
  requisition->height = char_height + ythickness * 2;
}

static void
btk_text_size_allocate (BtkWidget     *widget,
			BtkAllocation *allocation)
{
  BtkText *text = BTK_TEXT (widget);

  widget->allocation = *allocation;
  if (btk_widget_get_realized (widget))
    {
      bdk_window_move_resize (widget->window,
			      allocation->x, allocation->y,
			      allocation->width, allocation->height);
      
      bdk_window_move_resize (text->text_area,
			      widget->style->xthickness + TEXT_BORDER_ROOM,
			      widget->style->ythickness + TEXT_BORDER_ROOM,
			      MAX (1, (gint)widget->allocation.width - (gint)(widget->style->xthickness +
							  (gint)TEXT_BORDER_ROOM) * 2),
			      MAX (1, (gint)widget->allocation.height - (gint)(widget->style->ythickness +
							   (gint)TEXT_BORDER_ROOM) * 2));
      
      recompute_geometry (text);
    }
}

static gint
btk_text_expose (BtkWidget      *widget,
		 BdkEventExpose *event)
{
  if (event->window == BTK_TEXT (widget)->text_area)
    {
      TDEBUG (("in btk_text_expose (expose)\n"));
      expose_text (BTK_TEXT (widget), &event->area, TRUE);
    }
  else if (event->count == 0)
    {
      TDEBUG (("in btk_text_expose (focus)\n"));
      btk_text_draw_focus (widget);
    }
  
  return FALSE;
}

static gint
btk_text_scroll_timeout (gpointer data)
{
  BtkText *text;
  gint x, y;
  BdkModifierType mask;
  
  text = BTK_TEXT (data);
  
  text->timer = 0;
  bdk_window_get_pointer (text->text_area, &x, &y, &mask);
  
  if (mask & (BDK_BUTTON1_MASK | BDK_BUTTON3_MASK))
    {
      BdkEvent *event = bdk_event_new (BDK_MOTION_NOTIFY);
      
      event->motion.is_hint = 0;
      event->motion.x = x;
      event->motion.y = y;
      event->motion.state = mask;
      
      btk_text_motion_notify (BTK_WIDGET (text), (BdkEventMotion *)event);

      bdk_event_free (event);
    }

  return FALSE;
}

static gint
btk_text_button_press (BtkWidget      *widget,
		       BdkEventButton *event)
{
  BtkText *text = BTK_TEXT (widget);
  BtkOldEditable *old_editable = BTK_OLD_EDITABLE (widget);
  
  if (text->button && (event->button != text->button))
    return FALSE;
  
  text->button = event->button;
  
  if (!btk_widget_has_focus (widget))
    btk_widget_grab_focus (widget);
  
  if (event->button == 1)
    {
      switch (event->type)
	{
	case BDK_BUTTON_PRESS:
	  btk_grab_add (widget);
	  
	  undraw_cursor (text, FALSE);
	  find_mouse_cursor (text, (gint)event->x, (gint)event->y);
	  draw_cursor (text, FALSE);
	  
	  /* Set it now, so we display things right. We'll unset it
	   * later if things don't work out */
	  old_editable->has_selection = TRUE;
	  btk_text_set_selection (BTK_OLD_EDITABLE (text),
				  text->cursor_mark.index,
				  text->cursor_mark.index);
	  
	  break;
	  
	case BDK_2BUTTON_PRESS:
	  btk_text_select_word (text, event->time);
	  break;
	  
	case BDK_3BUTTON_PRESS:
	  btk_text_select_line (text, event->time);
	  break;
	  
	default:
	  break;
	}
    }
  else if (event->type == BDK_BUTTON_PRESS)
    {
      if ((event->button == 2) && old_editable->editable)
	{
	  if (old_editable->selection_start_pos == old_editable->selection_end_pos ||
	      old_editable->has_selection)
	    {
	      undraw_cursor (text, FALSE);
	      find_mouse_cursor (text, (gint)event->x, (gint)event->y);
	      draw_cursor (text, FALSE);
	      
	    }
	  
	  btk_selection_convert (widget, BDK_SELECTION_PRIMARY,
				 bdk_atom_intern_static_string ("UTF8_STRING"),
				 event->time);
	}
      else
	{
	  BdkDisplay *display = btk_widget_get_display (widget);
	  
	  btk_grab_add (widget);
	  
	  undraw_cursor (text, FALSE);
	  find_mouse_cursor (text, event->x, event->y);
	  draw_cursor (text, FALSE);
	  
	  btk_text_set_selection (BTK_OLD_EDITABLE (text),
				  text->cursor_mark.index,
				  text->cursor_mark.index);
	  
	  old_editable->has_selection = FALSE;
	  if (bdk_selection_owner_get_for_display (display,
						   BDK_SELECTION_PRIMARY) == widget->window)
	    btk_selection_owner_set_for_display (display,
						 NULL, 
						 BDK_SELECTION_PRIMARY,
						 event->time);
	}
    }
  
  return TRUE;
}

static gint
btk_text_button_release (BtkWidget      *widget,
			 BdkEventButton *event)
{
  BtkText *text = BTK_TEXT (widget);
  BtkOldEditable *old_editable;
  BdkDisplay *display;

  btk_grab_remove (widget);
  
  if (text->button != event->button)
    return FALSE;
  
  text->button = 0;
  
  if (text->timer)
    {
      g_source_remove (text->timer);
      text->timer = 0;
    }
  
  if (event->button == 1)
    {
      text = BTK_TEXT (widget);
      old_editable = BTK_OLD_EDITABLE (widget);
      display = btk_widget_get_display (widget);
      
      btk_grab_remove (widget);
      
      old_editable->has_selection = FALSE;
      if (old_editable->selection_start_pos != old_editable->selection_end_pos)
	{
	  if (btk_selection_owner_set_for_display (display,
						   widget,
						   BDK_SELECTION_PRIMARY,
						   event->time))
	    old_editable->has_selection = TRUE;
	  else
	    btk_text_update_text (old_editable, old_editable->selection_start_pos,
				  old_editable->selection_end_pos);
	}
      else
	{
	  if (bdk_selection_owner_get_for_display (display,
						   BDK_SELECTION_PRIMARY) == widget->window)
	    btk_selection_owner_set_for_display (display, 
						 NULL,
						 BDK_SELECTION_PRIMARY, 
						 event->time);
	}
    }
  else if (event->button == 3)
    {
      btk_grab_remove (widget);
    }
  
  undraw_cursor (text, FALSE);
  find_cursor (text, TRUE);
  draw_cursor (text, FALSE);
  
  return TRUE;
}

static gint
btk_text_motion_notify (BtkWidget      *widget,
			BdkEventMotion *event)
{
  BtkText *text = BTK_TEXT (widget);
  gint x, y;
  gint height;
  BdkModifierType mask;

  x = event->x;
  y = event->y;
  mask = event->state;
  if (event->is_hint || (text->text_area != event->window))
    {
      bdk_window_get_pointer (text->text_area, &x, &y, &mask);
    }
  
  if ((text->button == 0) ||
      !(mask & (BDK_BUTTON1_MASK | BDK_BUTTON3_MASK)))
    return FALSE;
  
  bdk_drawable_get_size (text->text_area, NULL, &height);
  
  if ((y < 0) || (y > height))
    {
      if (text->timer == 0)
	{
	  text->timer = bdk_threads_add_timeout (SCROLL_TIME, 
				       btk_text_scroll_timeout,
				       text);
	  
	  if (y < 0)
	    scroll_int (text, y/2);
	  else
	    scroll_int (text, (y - height)/2);
	}
      else
	return FALSE;
    }
  
  undraw_cursor (BTK_TEXT (widget), FALSE);
  find_mouse_cursor (BTK_TEXT (widget), x, y);
  draw_cursor (BTK_TEXT (widget), FALSE);
  
  btk_text_set_selection (BTK_OLD_EDITABLE (text), 
			  BTK_OLD_EDITABLE (text)->selection_start_pos,
			  text->cursor_mark.index);
  
  return FALSE;
}

static void 
btk_text_insert_text    (BtkEditable       *editable,
			 const gchar       *new_text,
			 gint               new_text_length,
			 gint              *position)
{
  BtkText *text = BTK_TEXT (editable);
  BdkFont *font;
  BdkColor *fore, *back;

  TextProperty *property;

  btk_text_set_point (text, *position);

  property = MARK_CURRENT_PROPERTY (&text->point);
  font = property->flags & PROPERTY_FONT ? property->font->bdk_font : NULL; 
  fore = property->flags & PROPERTY_FOREGROUND ? &property->fore_color : NULL; 
  back = property->flags & PROPERTY_BACKGROUND ? &property->back_color : NULL; 
  
  btk_text_insert (text, font, fore, back, new_text, new_text_length);

  *position = text->point.index;
}

static void 
btk_text_delete_text    (BtkEditable       *editable,
			 gint               start_pos,
			 gint               end_pos)
{
  BtkText *text = BTK_TEXT (editable);
  
  g_return_if_fail (start_pos >= 0);

  btk_text_set_point (text, start_pos);
  if (end_pos < 0)
    end_pos = TEXT_LENGTH (text);
  
  if (end_pos > start_pos)
    btk_text_forward_delete (text, end_pos - start_pos);
}

static gint
btk_text_key_press (BtkWidget   *widget,
		    BdkEventKey *event)
{
  BtkText *text = BTK_TEXT (widget);
  BtkOldEditable *old_editable = BTK_OLD_EDITABLE (widget);
  gchar key;
  gint return_val;
  gint position;

  key = event->keyval;
  return_val = TRUE;
  
  if ((BTK_OLD_EDITABLE(text)->editable == FALSE))
    {
      switch (event->keyval)
	{
	case BDK_Home:
        case BDK_KP_Home:
	  if (event->state & BDK_CONTROL_MASK)
	    scroll_int (text, -text->vadj->value);
	  else
	    return_val = FALSE;
	  break;
	case BDK_End:
        case BDK_KP_End:
	  if (event->state & BDK_CONTROL_MASK)
	    scroll_int (text, +text->vadj->upper); 
	  else
	    return_val = FALSE;
	  break;
        case BDK_KP_Page_Up:
	case BDK_Page_Up:   scroll_int (text, -text->vadj->page_increment); break;
        case BDK_KP_Page_Down:
	case BDK_Page_Down: scroll_int (text, +text->vadj->page_increment); break;
        case BDK_KP_Up:
	case BDK_Up:        scroll_int (text, -KEY_SCROLL_PIXELS); break;
        case BDK_KP_Down:
	case BDK_Down:      scroll_int (text, +KEY_SCROLL_PIXELS); break;
	case BDK_Return:
        case BDK_ISO_Enter:
        case BDK_KP_Enter:
	  if (event->state & BDK_CONTROL_MASK)
	    btk_signal_emit_by_name (BTK_OBJECT (text), "activate");
	  else
	    return_val = FALSE;
	  break;
	default:
	  return_val = FALSE;
	  break;
	}
    }
  else
    {
      gint extend_selection;
      gint extend_start;
      guint initial_pos = old_editable->current_pos;
      
      text->point = find_mark (text, text->cursor_mark.index);
      
      extend_selection = event->state & BDK_SHIFT_MASK;
      extend_start = FALSE;
      
      if (extend_selection)
	{
	  old_editable->has_selection = TRUE;
	  
	  if (old_editable->selection_start_pos == old_editable->selection_end_pos)
	    {
	      old_editable->selection_start_pos = text->point.index;
	      old_editable->selection_end_pos = text->point.index;
	    }
	  
	  extend_start = (text->point.index == old_editable->selection_start_pos);
	}
      
      switch (event->keyval)
	{
        case BDK_KP_Home:
	case BDK_Home:
	  if (event->state & BDK_CONTROL_MASK)
	    move_cursor_buffer_ver (text, -1);
	  else
	    btk_text_move_beginning_of_line (text);
	  break;
        case BDK_KP_End:
	case BDK_End:
	  if (event->state & BDK_CONTROL_MASK)
	    move_cursor_buffer_ver (text, +1);
	  else
	    btk_text_move_end_of_line (text);
	  break;
        case BDK_KP_Page_Up:
	case BDK_Page_Up:   move_cursor_page_ver (text, -1); break;
        case BDK_KP_Page_Down:
	case BDK_Page_Down: move_cursor_page_ver (text, +1); break;
	  /* CUA has Ctrl-Up/Ctrl-Down as paragraph up down */
        case BDK_KP_Up:
	case BDK_Up:        move_cursor_ver (text, -1); break;
        case BDK_KP_Down:
	case BDK_Down:      move_cursor_ver (text, +1); break;
        case BDK_KP_Left:
	case BDK_Left:
	  if (event->state & BDK_CONTROL_MASK)
	    btk_text_move_backward_word (text);
	  else
	    move_cursor_hor (text, -1); 
	  break;
        case BDK_KP_Right:
	case BDK_Right:     
	  if (event->state & BDK_CONTROL_MASK)
	    btk_text_move_forward_word (text);
	  else
	    move_cursor_hor (text, +1); 
	  break;
	  
	case BDK_BackSpace:
	  if (event->state & BDK_CONTROL_MASK)
	    btk_text_delete_backward_word (text);
	  else
	    btk_text_delete_backward_character (text);
	  break;
	case BDK_Clear:
	  btk_text_delete_line (text);
	  break;
        case BDK_KP_Insert:
	case BDK_Insert:
	  if (event->state & BDK_SHIFT_MASK)
	    {
	      extend_selection = FALSE;
	      btk_editable_paste_clipboard (BTK_EDITABLE (old_editable));
	    }
	  else if (event->state & BDK_CONTROL_MASK)
	    {
	      btk_editable_copy_clipboard (BTK_EDITABLE (old_editable));
	    }
	  else
	    {
	      /* btk_toggle_insert(text) -- IMPLEMENT */
	    }
	  break;
	case BDK_Delete:
        case BDK_KP_Delete:
	  if (event->state & BDK_CONTROL_MASK)
	    btk_text_delete_forward_word (text);
	  else if (event->state & BDK_SHIFT_MASK)
	    {
	      extend_selection = FALSE;
	      btk_editable_cut_clipboard (BTK_EDITABLE (old_editable));
	    }
	  else
	    btk_text_delete_forward_character (text);
	  break;
	case BDK_Tab:
        case BDK_ISO_Left_Tab:
        case BDK_KP_Tab:
	  position = text->point.index;
	  btk_editable_insert_text (BTK_EDITABLE (old_editable), "\t", 1, &position);
	  break;
        case BDK_KP_Enter:
        case BDK_ISO_Enter:
	case BDK_Return:
	  if (event->state & BDK_CONTROL_MASK)
	    btk_signal_emit_by_name (BTK_OBJECT (text), "activate");
	  else
	    {
	      position = text->point.index;
	      btk_editable_insert_text (BTK_EDITABLE (old_editable), "\n", 1, &position);
	    }
	  break;
	case BDK_Escape:
	  /* Don't insert literally */
	  return_val = FALSE;
	  break;
	  
	default:
	  return_val = FALSE;
	  
	  if (event->state & BDK_CONTROL_MASK)
	    {
	      return_val = TRUE;
	      if ((key >= 'A') && (key <= 'Z'))
		key -= 'A' - 'a';

	      switch (key)
		{
		case 'a':
		  btk_text_move_beginning_of_line (text);
		  break;
		case 'b':
		  btk_text_move_backward_character (text);
		  break;
		case 'c':
		  btk_editable_copy_clipboard (BTK_EDITABLE (text));
		  break;
		case 'd':
		  btk_text_delete_forward_character (text);
		  break;
		case 'e':
		  btk_text_move_end_of_line (text);
		  break;
		case 'f':
		  btk_text_move_forward_character (text);
		  break;
		case 'h':
		  btk_text_delete_backward_character (text);
		  break;
		case 'k':
		  btk_text_delete_to_line_end (text);
		  break;
		case 'n':
		  btk_text_move_next_line (text);
		  break;
		case 'p':
		  btk_text_move_previous_line (text);
		  break;
		case 'u':
		  btk_text_delete_line (text);
		  break;
		case 'v':
		  btk_editable_paste_clipboard (BTK_EDITABLE (text));
		  break;
		case 'w':
		  btk_text_delete_backward_word (text);
		  break;
		case 'x':
		  btk_editable_cut_clipboard (BTK_EDITABLE (text));
		  break;
		default:
		  return_val = FALSE;
		}

	      break;
	    }
	  else if (event->state & BDK_MOD1_MASK)
	    {
	      return_val = TRUE;
	      if ((key >= 'A') && (key <= 'Z'))
		key -= 'A' - 'a';
	      
	      switch (key)
		{
		case 'b':
		  btk_text_move_backward_word (text);
		  break;
		case 'd':
		  btk_text_delete_forward_word (text);
		  break;
		case 'f':
		  btk_text_move_forward_word (text);
		  break;
		default:
		  return_val = FALSE;
		}
	      
	      break;
	    }
	  else if (event->length > 0)
	    {
	      extend_selection = FALSE;
	      
	      btk_editable_delete_selection (BTK_EDITABLE (old_editable));
	      position = text->point.index;
	      btk_editable_insert_text (BTK_EDITABLE (old_editable), event->string, event->length, &position);
	      
	      return_val = TRUE;
	    }
	}
      
      if (return_val && (old_editable->current_pos != initial_pos))
	{
	  if (extend_selection)
	    {
	      if (old_editable->current_pos < old_editable->selection_start_pos)
		btk_text_set_selection (old_editable, old_editable->current_pos,
					old_editable->selection_end_pos);
	      else if (old_editable->current_pos > old_editable->selection_end_pos)
		btk_text_set_selection (old_editable, old_editable->selection_start_pos,
					old_editable->current_pos);
	      else
		{
		  if (extend_start)
		    btk_text_set_selection (old_editable, old_editable->current_pos,
					    old_editable->selection_end_pos);
		  else
		    btk_text_set_selection (old_editable, old_editable->selection_start_pos,
					    old_editable->current_pos);
		}
	    }
	  else
	    btk_text_set_selection (old_editable, 0, 0);
	  
	  btk_old_editable_claim_selection (old_editable,
					    old_editable->selection_start_pos != old_editable->selection_end_pos,
					    event->time);
	}
    }
  
  return return_val;
}

static void
btk_text_adjustment (BtkAdjustment *adjustment,
		     BtkText       *text)
{
  g_return_if_fail (BTK_IS_ADJUSTMENT (adjustment));
  g_return_if_fail (BTK_IS_TEXT (text));
  
  /* Just ignore it if we haven't been size-allocated and realized yet */
  if (text->line_start_cache == NULL) 
    return;
  
  if (adjustment == text->hadj)
    {
      g_warning ("horizontal scrolling not implemented");
    }
  else
    {
      gint diff = ((gint)adjustment->value) - text->last_ver_value;
      
      if (diff != 0)
	{
	  undraw_cursor (text, FALSE);
	  
	  if (diff > 0)
	    scroll_down (text, diff);
	  else /* if (diff < 0) */
	    scroll_up (text, diff);
	  
	  draw_cursor (text, FALSE);
	  
	  text->last_ver_value = adjustment->value;
	}
    }
}

static void
btk_text_adjustment_destroyed (BtkAdjustment *adjustment,
                               BtkText       *text)
{
  g_return_if_fail (BTK_IS_ADJUSTMENT (adjustment));
  g_return_if_fail (BTK_IS_TEXT (text));

  if (adjustment == text->hadj)
    btk_text_set_adjustments (text, NULL, text->vadj);
  if (adjustment == text->vadj)
    btk_text_set_adjustments (text, text->hadj, NULL);
}


static BtkPropertyMark
find_this_line_start_mark (BtkText* text, guint point_position, const BtkPropertyMark* near)
{
  BtkPropertyMark mark;
  
  mark = find_mark_near (text, point_position, near);
  
  while (mark.index > 0 &&
	 BTK_TEXT_INDEX (text, mark.index - 1) != LINE_DELIM)
    decrement_mark (&mark);
  
  return mark;
}

static void
init_tab_cont (BtkText* text, PrevTabCont* tab_cont)
{
  tab_cont->pixel_offset          = 0;
  tab_cont->tab_start.tab_stops   = text->tab_stops;
  tab_cont->tab_start.to_next_tab = (gintptr) text->tab_stops->data;
  
  if (!tab_cont->tab_start.to_next_tab)
    tab_cont->tab_start.to_next_tab = text->default_tab_width;
}

static void
line_params_iterate (BtkText* text,
		     const BtkPropertyMark* mark0,
		     const PrevTabCont* tab_mark0,
		     gint8 alloc,
		     void* data,
		     LineIteratorFunction iter)
     /* mark0 MUST be a real line start.  if ALLOC, allocate line params
      * from a mem chunk.  DATA is passed to ITER_CALL, which is called
      * for each line following MARK, iteration continues unless ITER_CALL
      * returns TRUE. */
{
  BtkPropertyMark mark = *mark0;
  PrevTabCont  tab_conts[2];
  LineParams   *lp, lpbuf;
  gint         tab_cont_index = 0;
  
  if (tab_mark0)
    tab_conts[0] = *tab_mark0;
  else
    init_tab_cont (text, tab_conts);
  
  for (;;)
    {
      if (alloc)
	lp = g_slice_new (LineParams);
      else
	lp = &lpbuf;
      
      *lp = find_line_params (text, &mark, tab_conts + tab_cont_index,
			      tab_conts + (tab_cont_index + 1) % 2);
      
      if ((*iter) (text, lp, data))
	return;
      
      if (LAST_INDEX (text, lp->end))
	break;
      
      mark = lp->end;
      advance_mark (&mark);
      tab_cont_index = (tab_cont_index + 1) % 2;
    }
}

static gint
fetch_lines_iterator (BtkText* text, LineParams* lp, void* data)
{
  FetchLinesData *fldata = (FetchLinesData*) data;
  
  fldata->new_lines = g_list_prepend (fldata->new_lines, lp);
  
  switch (fldata->fl_type)
    {
    case FetchLinesCount:
      if (!text->line_wrap || !lp->wraps)
	fldata->data += 1;
      
      if (fldata->data >= fldata->data_max)
	return TRUE;
      
      break;
    case FetchLinesPixels:
      
      fldata->data += LINE_HEIGHT(*lp);
      
      if (fldata->data >= fldata->data_max)
	return TRUE;
      
      break;
    }
  
  return FALSE;
}

static GList*
fetch_lines (BtkText* text,
	     const BtkPropertyMark* mark0,
	     const PrevTabCont* tab_cont0,
	     FLType fl_type,
	     gint data)
{
  FetchLinesData fl_data;
  
  fl_data.new_lines = NULL;
  fl_data.data      = 0;
  fl_data.data_max  = data;
  fl_data.fl_type   = fl_type;
  
  line_params_iterate (text, mark0, tab_cont0, TRUE, &fl_data, fetch_lines_iterator);
  
  return g_list_reverse (fl_data.new_lines);
}

static void
fetch_lines_backward (BtkText* text)
{
  GList *new_line_start;
  BtkPropertyMark mark;
  
  if (CACHE_DATA(text->line_start_cache).start.index == 0)
    return;
  
  mark = find_this_line_start_mark (text,
				    CACHE_DATA(text->line_start_cache).start.index - 1,
				    &CACHE_DATA(text->line_start_cache).start);
  
  new_line_start = fetch_lines (text, &mark, NULL, FetchLinesCount, 1);
  
  while (new_line_start->next)
    new_line_start = new_line_start->next;
  
  new_line_start->next = text->line_start_cache;
  text->line_start_cache->prev = new_line_start;
}

static void
fetch_lines_forward (BtkText* text, gint line_count)
{
  BtkPropertyMark mark;
  GList* line = text->line_start_cache;
  
  while(line->next)
    line = line->next;
  
  mark = CACHE_DATA(line).end;
  
  if (LAST_INDEX (text, mark))
    return;
  
  advance_mark(&mark);
  
  line->next = fetch_lines (text, &mark, &CACHE_DATA(line).tab_cont_next, FetchLinesCount, line_count);
  
  if (line->next)
    line->next->prev = line;
}

/* Compute the number of lines, and vertical pixels for n characters
 * starting from the point 
 */
static void
compute_lines_pixels (BtkText* text, guint char_count,
		      guint *lines, guint *pixels)
{
  GList *line = text->current_line;
  gint chars_left = char_count;
  
  *lines = 0;
  *pixels = 0;
  
  /* If chars_left == 0, that means we're joining two lines in a
   * deletion, so add in the values for the next line as well 
   */
  for (; line && chars_left >= 0; line = line->next)
    {
      *pixels += LINE_HEIGHT(CACHE_DATA(line));
      
      if (line == text->current_line)
	chars_left -= CACHE_DATA(line).end.index - text->point.index + 1;
      else
	chars_left -= CACHE_DATA(line).end.index - CACHE_DATA(line).start.index + 1;
      
      if (!text->line_wrap || !CACHE_DATA(line).wraps)
	*lines += 1;
      else
	if (chars_left < 0)
	  chars_left = 0;	/* force another loop */
      
      if (!line->next)
	fetch_lines_forward (text, 1);
    }
}

static gint
total_line_height (BtkText* text, GList* line, gint line_count)
{
  gint height = 0;
  
  for (; line && line_count > 0; line = line->next)
    {
      height += LINE_HEIGHT(CACHE_DATA(line));
      
      if (!text->line_wrap || !CACHE_DATA(line).wraps)
	line_count -= 1;
      
      if (!line->next)
	fetch_lines_forward (text, line_count);
    }
  
  return height;
}

static void
swap_lines (BtkText* text, GList* old, GList* new, guint old_line_count)
{
  if (old == text->line_start_cache)
    {
      GList* last;
      
      for (; old_line_count > 0; old_line_count -= 1)
	{
	  while (text->line_start_cache &&
		 text->line_wrap &&
		 CACHE_DATA(text->line_start_cache).wraps)
	    remove_cache_line(text, text->line_start_cache);
	  
	  remove_cache_line(text, text->line_start_cache);
	}
      
      last = g_list_last (new);
      
      last->next = text->line_start_cache;
      
      if (text->line_start_cache)
	text->line_start_cache->prev = last;
      
      text->line_start_cache = new;
    }
  else
    {
      GList *last;
      
      g_assert (old->prev);
      
      last = old->prev;
      
      for (; old_line_count > 0; old_line_count -= 1)
	{
	  while (old && text->line_wrap && CACHE_DATA(old).wraps)
	    old = remove_cache_line (text, old);
	  
	  old = remove_cache_line (text, old);
	}
      
      last->next = new;
      new->prev = last;
      
      last = g_list_last (new);
      
      last->next = old;
      
      if (old)
	old->prev = last;
    }
}

static void
correct_cache_delete (BtkText* text, gint nchars, gint lines)
{
  GList* cache = text->current_line;
  gint i;
  
  for (i = 0; cache && i < lines; i += 1, cache = cache->next)
    /* nothing */;
  
  for (; cache; cache = cache->next)
    {
      BtkPropertyMark *start = &CACHE_DATA(cache).start;
      BtkPropertyMark *end = &CACHE_DATA(cache).end;
      
      start->index -= nchars;
      end->index -= nchars;
      
      if (LAST_INDEX (text, text->point) &&
	  start->index == text->point.index)
	*start = text->point;
      else if (start->property == text->point.property)
	start->offset = start->index - (text->point.index - text->point.offset);
      
      if (LAST_INDEX (text, text->point) &&
	  end->index == text->point.index)
	*end = text->point;
      if (end->property == text->point.property)
	end->offset = end->index - (text->point.index - text->point.offset);
      
      /*TEXT_ASSERT_MARK(text, start, "start");*/
      /*TEXT_ASSERT_MARK(text, end, "end");*/
    }
}

static void
delete_expose (BtkText* text, guint nchars, guint old_lines, guint old_pixels)
{
  BtkWidget *widget = BTK_WIDGET (text);
  
  gint pixel_height;
  guint new_pixels = 0;
  BdkRectangle rect;
  GList* new_line = NULL;
  gint width, height;
  
  text->cursor_virtual_x = 0;
  
  correct_cache_delete (text, nchars, old_lines);
  
  pixel_height = pixel_height_of(text, text->current_line) -
    LINE_HEIGHT(CACHE_DATA(text->current_line));
  
  if (CACHE_DATA(text->current_line).start.index == text->point.index)
    CACHE_DATA(text->current_line).start = text->point;
  
  new_line = fetch_lines (text,
			  &CACHE_DATA(text->current_line).start,
			  &CACHE_DATA(text->current_line).tab_cont,
			  FetchLinesCount,
			  1);
  
  swap_lines (text, text->current_line, new_line, old_lines);
  
  text->current_line = new_line;
  
  new_pixels = total_line_height (text, new_line, 1);
  
  bdk_drawable_get_size (text->text_area, &width, &height);
  
  if (old_pixels != new_pixels)
    {
      if (!widget->style->bg_pixmap[BTK_STATE_NORMAL])
	{
	  bdk_draw_drawable (text->text_area,
                             text->gc,
                             text->text_area,
                             0,
                             pixel_height + old_pixels,
                             0,
                             pixel_height + new_pixels,
                             width,
                             height);
	}
      text->vadj->upper += new_pixels;
      text->vadj->upper -= old_pixels;
      adjust_adj (text, text->vadj);
    }
  
  rect.x = 0;
  rect.y = pixel_height;
  rect.width = width;
  rect.height = new_pixels;
  
  expose_text (text, &rect, FALSE);
  btk_text_draw_focus ( (BtkWidget *) text);
  
  text->cursor_mark = text->point;
  
  find_cursor (text, TRUE);
  
  if (old_pixels != new_pixels)
    {
      if (widget->style->bg_pixmap[BTK_STATE_NORMAL])
	{
	  rect.x = 0;
	  rect.y = pixel_height + new_pixels;
	  rect.width = width;
	  rect.height = height - rect.y;
	  
	  expose_text (text, &rect, FALSE);
	}
      else
	process_exposes (text);
    }
  
  TEXT_ASSERT (text);
  TEXT_SHOW(text);
}

/* note, the point has already been moved forward */
static void
correct_cache_insert (BtkText* text, gint nchars)
{
  GList *cache;
  BtkPropertyMark *start;
  BtkPropertyMark *end;
  gboolean was_split = FALSE;
  
  /* We need to distinguish whether the property was split in the
   * insert or not, so we check if the point (which points after
   * the insertion here), points to the same character as the
   * point before. Ugh.
   */
  if (nchars > 0)
    {
      BtkPropertyMark tmp_mark = text->point;
      move_mark_n (&tmp_mark, -1);
      
      if (tmp_mark.property != text->point.property)
	was_split = TRUE;
    }
  
  /* If we inserted a property exactly at the beginning of the
   * line, we have to correct here, or fetch_lines will
   * fetch junk.
   */
  start = &CACHE_DATA(text->current_line).start;

  /* Check if if we split exactly at the beginning of the line:
   * (was_split won't be set if we are inserting at the end of the text, 
   *  so we don't check)
   */
  if (start->offset ==  MARK_CURRENT_PROPERTY (start)->length)
    SET_PROPERTY_MARK (start, start->property->next, 0);
  /* Check if we inserted a property at the beginning of the text: */
  else if (was_split &&
	   (start->property == text->point.property) &&
	   (start->index == text->point.index - nchars))
    SET_PROPERTY_MARK (start, start->property->prev, 0);

  /* Now correct the offsets, and check for start or end marks that
   * are after the point, yet point to a property before the point's
   * property. This indicates that they are meant to point to the
   * second half of a property we split in insert_text_property(), so
   * we fix them up that way.  
   */
  cache = text->current_line->next;
  
  for (; cache; cache = cache->next)
    {
      start = &CACHE_DATA(cache).start;
      end = &CACHE_DATA(cache).end;
      
      if (LAST_INDEX (text, text->point) &&
	  start->index == text->point.index)
	*start = text->point;
      else if (start->index >= text->point.index - nchars)
	{
	  if (!was_split && start->property == text->point.property)
	    move_mark_n(start, nchars);
	  else
	    {
	      if (start->property->next &&
		  (start->property->next->next == text->point.property))
		{
		  g_assert (start->offset >=  MARK_CURRENT_PROPERTY (start)->length);
		  start->offset -= MARK_CURRENT_PROPERTY (start)->length;
		  start->property = text->point.property;
		}
	      start->index += nchars;
	    }
	}
      
      if (LAST_INDEX (text, text->point) &&
	  end->index == text->point.index)
	*end = text->point;
      if (end->index >= text->point.index - nchars)
	{
	  if (!was_split && end->property == text->point.property)
	    move_mark_n(end, nchars);
	  else 
	    {
	      if (end->property->next &&
		  (end->property->next->next == text->point.property))
		{
		  g_assert (end->offset >=  MARK_CURRENT_PROPERTY (end)->length);
		  end->offset -= MARK_CURRENT_PROPERTY (end)->length;
		  end->property = text->point.property;
		}
	      end->index += nchars;
	    }
	}
      
      /*TEXT_ASSERT_MARK(text, start, "start");*/
      /*TEXT_ASSERT_MARK(text, end, "end");*/
    }
}


static void
insert_expose (BtkText* text, guint old_pixels, gint nchars,
	       guint new_line_count)
{
  BtkWidget *widget = BTK_WIDGET (text);
  
  gint pixel_height;
  guint new_pixels = 0;
  BdkRectangle rect;
  GList* new_lines = NULL;
  gint width, height;
  
  text->cursor_virtual_x = 0;
  
  undraw_cursor (text, FALSE);
  
  correct_cache_insert (text, nchars);
  
  TEXT_SHOW_ADJ (text, text->vadj, "vadj");
  
  pixel_height = pixel_height_of(text, text->current_line) -
    LINE_HEIGHT(CACHE_DATA(text->current_line));
  
  new_lines = fetch_lines (text,
			   &CACHE_DATA(text->current_line).start,
			   &CACHE_DATA(text->current_line).tab_cont,
			   FetchLinesCount,
			   new_line_count);
  
  swap_lines (text, text->current_line, new_lines, 1);
  
  text->current_line = new_lines;
  
  new_pixels = total_line_height (text, new_lines, new_line_count);
  
  bdk_drawable_get_size (text->text_area, &width, &height);
  
  if (old_pixels != new_pixels)
    {
      if (!widget->style->bg_pixmap[BTK_STATE_NORMAL])
	{
	  bdk_draw_drawable (text->text_area,
                             text->gc,
                             text->text_area,
                             0,
                             pixel_height + old_pixels,
                             0,
                             pixel_height + new_pixels,
                             width,
                             height + (old_pixels - new_pixels) - pixel_height);
	  
	}
      text->vadj->upper += new_pixels;
      text->vadj->upper -= old_pixels;
      adjust_adj (text, text->vadj);
    }
  
  rect.x = 0;
  rect.y = pixel_height;
  rect.width = width;
  rect.height = new_pixels;
  
  expose_text (text, &rect, FALSE);
  btk_text_draw_focus ( (BtkWidget *) text);
  
  text->cursor_mark = text->point;
  
  find_cursor (text, TRUE);
  
  draw_cursor (text, FALSE);
  
  if (old_pixels != new_pixels)
    {
      if (widget->style->bg_pixmap[BTK_STATE_NORMAL])
	{
	  rect.x = 0;
	  rect.y = pixel_height + new_pixels;
	  rect.width = width;
	  rect.height = height - rect.y;
	  
	  expose_text (text, &rect, FALSE);
	}
      else
	process_exposes (text);
    }
  
  TEXT_SHOW_ADJ (text, text->vadj, "vadj");
  TEXT_ASSERT (text);
  TEXT_SHOW(text);
}

/* Text property functions */

static guint
font_hash (gconstpointer font)
{
  return bdk_font_id ((const BdkFont*) font);
}

static GHashTable *font_cache_table = NULL;

static BtkTextFont*
get_text_font (BdkFont* gfont)
{
  BtkTextFont* tf;
  gint i;
  
  if (!font_cache_table)
    font_cache_table = g_hash_table_new (font_hash, (GEqualFunc) bdk_font_equal);
  
  tf = g_hash_table_lookup (font_cache_table, gfont);
  
  if (tf)
    {
      tf->ref_count++;
      return tf;
    }

  tf = g_new (BtkTextFont, 1);
  tf->ref_count = 1;

  tf->bdk_font = gfont;
  bdk_font_ref (gfont);
  
  for(i = 0; i < 256; i += 1)
    tf->char_widths[i] = bdk_char_width (gfont, (char)i);
  
  g_hash_table_insert (font_cache_table, gfont, tf);
  
  return tf;
}

static void
text_font_unref (BtkTextFont *text_font)
{
  text_font->ref_count--;
  if (text_font->ref_count == 0)
    {
      g_hash_table_remove (font_cache_table, text_font->bdk_font);
      bdk_font_unref (text_font->bdk_font);
      g_free (text_font);
    }
}

static gint
text_properties_equal (TextProperty* prop, BdkFont* font, const BdkColor *fore, const BdkColor *back)
{
  if (prop->flags & PROPERTY_FONT)
    {
      gboolean retval;
      BtkTextFont *text_font;

      if (!font)
	return FALSE;

      text_font = get_text_font (font);

      retval = (prop->font == text_font);
      text_font_unref (text_font);
      
      if (!retval)
	return FALSE;
    }
  else
    if (font != NULL)
      return FALSE;

  if (prop->flags & PROPERTY_FOREGROUND)
    {
      if (!fore || !bdk_color_equal (&prop->fore_color, fore))
	return FALSE;
    }
  else
    if (fore != NULL)
      return FALSE;

  if (prop->flags & PROPERTY_BACKGROUND)
    {
      if (!back || !bdk_color_equal (&prop->back_color, back))
	return FALSE;
    }
  else
    if (back != NULL)
      return FALSE;
  
  return TRUE;
}

static void
realize_property (BtkText *text, TextProperty *prop)
{
  BdkColormap *colormap = btk_widget_get_colormap (BTK_WIDGET (text));

  if (prop->flags & PROPERTY_FOREGROUND)
    bdk_colormap_alloc_color (colormap, &prop->fore_color, FALSE, FALSE);
  
  if (prop->flags & PROPERTY_BACKGROUND)
    bdk_colormap_alloc_color (colormap, &prop->back_color, FALSE, FALSE);
}

static void
realize_properties (BtkText *text)
{
  GList *tmp_list = text->text_properties;

  while (tmp_list)
    {
      realize_property (text, tmp_list->data);
      
      tmp_list = tmp_list->next;
    }
}

static void
unrealize_property (BtkText *text, TextProperty *prop)
{
  BdkColormap *colormap = btk_widget_get_colormap (BTK_WIDGET (text));

  if (prop->flags & PROPERTY_FOREGROUND)
    bdk_colormap_free_colors (colormap, &prop->fore_color, 1);
  
  if (prop->flags & PROPERTY_BACKGROUND)
    bdk_colormap_free_colors (colormap, &prop->back_color, 1);
}

static void
unrealize_properties (BtkText *text)
{
  GList *tmp_list = text->text_properties;

  while (tmp_list)
    {
      unrealize_property (text, tmp_list->data);

      tmp_list = tmp_list->next;
    }
}

static TextProperty*
new_text_property (BtkText *text, BdkFont *font, const BdkColor* fore,
		   const BdkColor* back, guint length)
{
  TextProperty *prop;
  
  prop = g_slice_new (TextProperty);

  prop->flags = 0;
  if (font)
    {
      prop->flags |= PROPERTY_FONT;
      prop->font = get_text_font (font);
    }
  else
    prop->font = NULL;
  
  if (fore)
    {
      prop->flags |= PROPERTY_FOREGROUND;
      prop->fore_color = *fore;
    }
      
  if (back)
    {
      prop->flags |= PROPERTY_BACKGROUND;
      prop->back_color = *back;
    }

  prop->length = length;

  if (btk_widget_get_realized (BTK_WIDGET (text)))
    realize_property (text, prop);

  return prop;
}

static void
destroy_text_property (TextProperty *prop)
{
  if (prop->font)
    text_font_unref (prop->font);
  
  g_slice_free (TextProperty, prop);
}

/* Flop the memory between the point and the gap around like a
 * dead fish. */
static void
move_gap (BtkText* text, guint index)
{
  if (text->gap_position < index)
    {
      gint diff = index - text->gap_position;
      
      if (text->use_wchar)
	g_memmove (text->text.wc + text->gap_position,
		   text->text.wc + text->gap_position + text->gap_size,
		   diff*sizeof (BdkWChar));
      else
	g_memmove (text->text.ch + text->gap_position,
		   text->text.ch + text->gap_position + text->gap_size,
		   diff);
      
      text->gap_position = index;
    }
  else if (text->gap_position > index)
    {
      gint diff = text->gap_position - index;
      
      if (text->use_wchar)
	g_memmove (text->text.wc + index + text->gap_size,
		   text->text.wc + index,
		   diff*sizeof (BdkWChar));
      else
	g_memmove (text->text.ch + index + text->gap_size,
		   text->text.ch + index,
		   diff);
      
      text->gap_position = index;
    }
}

/* Increase the gap size. */
static void
make_forward_space (BtkText* text, guint len)
{
  if (text->gap_size < len)
    {
      guint sum = MAX(2*len, MIN_GAP_SIZE) + text->text_end;
      
      if (sum >= text->text_len)
	{
	  guint i = 1;
	  
	  while (i <= sum) i <<= 1;
	  
	  if (text->use_wchar)
	    text->text.wc = (BdkWChar *)g_realloc(text->text.wc,
						  i*sizeof(BdkWChar));
	  else
	    text->text.ch = (guchar *)g_realloc(text->text.ch, i);
	  text->text_len = i;
	}
      
      if (text->use_wchar)
	g_memmove (text->text.wc + text->gap_position + text->gap_size + 2*len,
		   text->text.wc + text->gap_position + text->gap_size,
		   (text->text_end - (text->gap_position + text->gap_size))
		   *sizeof(BdkWChar));
      else
	g_memmove (text->text.ch + text->gap_position + text->gap_size + 2*len,
		   text->text.ch + text->gap_position + text->gap_size,
		   text->text_end - (text->gap_position + text->gap_size));
      
      text->text_end += len*2;
      text->gap_size += len*2;
    }
}

/* Inserts into the text property list a list element that guarantees
 * that for len characters following the point, text has the correct
 * property.  does not move point.  adjusts text_properties_point and
 * text_properties_point_offset relative to the current value of
 * point. */
static void
insert_text_property (BtkText* text, BdkFont* font,
		      const BdkColor *fore, const BdkColor* back, guint len)
{
  BtkPropertyMark *mark = &text->point;
  TextProperty* forward_prop = MARK_CURRENT_PROPERTY(mark);
  TextProperty* backward_prop = MARK_PREV_PROPERTY(mark);
  
  if (MARK_OFFSET(mark) == 0)
    {
      /* Point is on the boundary of two properties.
       * If it is the same as either, grow, else insert
       * a new one. */
      
      if (text_properties_equal(forward_prop, font, fore, back))
	{
	  /* Grow the property in front of us. */
	  
	  MARK_PROPERTY_LENGTH(mark) += len;
	}
      else if (backward_prop &&
	       text_properties_equal(backward_prop, font, fore, back))
	{
	  /* Grow property behind us, point property and offset
	   * change. */
	  
	  SET_PROPERTY_MARK (&text->point,
			     MARK_PREV_LIST_PTR (mark),
			     backward_prop->length);
	  
	  backward_prop->length += len;
	}
      else if ((MARK_NEXT_LIST_PTR(mark) == NULL) &&
	       (forward_prop->length == 1))
	{
	  /* Next property just has last position, take it over */

	  if (btk_widget_get_realized (BTK_WIDGET (text)))
	    unrealize_property (text, forward_prop);

	  forward_prop->flags = 0;
	  if (font)
	    {
	      forward_prop->flags |= PROPERTY_FONT;
	      forward_prop->font = get_text_font (font);
	    }
	  else
	    forward_prop->font = NULL;
	    
	  if (fore)
	    {
	      forward_prop->flags |= PROPERTY_FOREGROUND;
	      forward_prop->fore_color = *fore;
	    }
	  if (back)
	    {
	      forward_prop->flags |= PROPERTY_BACKGROUND;
	      forward_prop->back_color = *back;
	    }
	  forward_prop->length += len;

	  if (btk_widget_get_realized (BTK_WIDGET (text)))
	    realize_property (text, forward_prop);
	}
      else
	{
	  /* Splice a new property into the list. */
	  
	  GList* new_prop = g_list_alloc();
	  
	  new_prop->next = MARK_LIST_PTR(mark);
	  new_prop->prev = MARK_PREV_LIST_PTR(mark);
	  new_prop->next->prev = new_prop;
	  
	  if (new_prop->prev)
	    new_prop->prev->next = new_prop;

	  new_prop->data = new_text_property (text, font, fore, back, len);

	  SET_PROPERTY_MARK (mark, new_prop, 0);
	}
    }
  else
    {
      /* The following will screw up the line_start cache,
       * we'll fix it up in correct_cache_insert
       */
      
      /* In the middle of forward_prop, if properties are equal,
       * just add to its length, else split it into two and splice
       * in a new one. */
      if (text_properties_equal (forward_prop, font, fore, back))
	{
	  forward_prop->length += len;
	}
      else if ((MARK_NEXT_LIST_PTR(mark) == NULL) &&
	       (MARK_OFFSET(mark) + 1 == forward_prop->length))
	{
	  /* Inserting before only the last position in the text */
	  
	  GList* new_prop;
	  forward_prop->length -= 1;
	  
	  new_prop = g_list_alloc();
	  new_prop->data = new_text_property (text, font, fore, back, len+1);
	  new_prop->prev = MARK_LIST_PTR(mark);
	  new_prop->next = NULL;
	  MARK_NEXT_LIST_PTR(mark) = new_prop;
	  
	  SET_PROPERTY_MARK (mark, new_prop, 0);
	}
      else
	{
	  GList* new_prop = g_list_alloc();
	  GList* new_prop_forward = g_list_alloc();
	  gint old_length = forward_prop->length;
	  GList* next = MARK_NEXT_LIST_PTR(mark);
	  
	  /* Set the new lengths according to where they are split.  Construct
	   * two new properties. */
	  forward_prop->length = MARK_OFFSET(mark);

	  new_prop_forward->data = 
	    new_text_property(text,
			      forward_prop->flags & PROPERTY_FONT ? 
                                     forward_prop->font->bdk_font : NULL,
			      forward_prop->flags & PROPERTY_FOREGROUND ? 
  			             &forward_prop->fore_color : NULL,
			      forward_prop->flags & PROPERTY_BACKGROUND ? 
  			             &forward_prop->back_color : NULL,
			      old_length - forward_prop->length);

	  new_prop->data = new_text_property(text, font, fore, back, len);

	  /* Now splice things in. */
	  MARK_NEXT_LIST_PTR(mark) = new_prop;
	  new_prop->prev = MARK_LIST_PTR(mark);
	  
	  new_prop->next = new_prop_forward;
	  new_prop_forward->prev = new_prop;
	  
	  new_prop_forward->next = next;
	  
	  if (next)
	    next->prev = new_prop_forward;
	  
	  SET_PROPERTY_MARK (mark, new_prop, 0);
	}
    }
  
  while (text->text_properties_end->next)
    text->text_properties_end = text->text_properties_end->next;
  
  while (text->text_properties->prev)
    text->text_properties = text->text_properties->prev;
}

static void
delete_text_property (BtkText* text, guint nchars)
{
  /* Delete nchars forward from point. */
  
  /* Deleting text properties is problematical, because we
   * might be storing around marks pointing to a property.
   *
   * The marks in question and how we handle them are:
   *
   *  point: We know the new value, since it will be at the
   *         end of the deleted text, and we move it there
   *         first.
   *  cursor: We just remove the mark and set it equal to the
   *         point after the operation.
   *  line-start cache: We replace most affected lines.
   *         The current line gets used to fetch the new
   *         lines so, if necessary, (delete at the beginning
   *         of a line) we fix it up by setting it equal to the
   *         point.
   */
  
  TextProperty *prop;
  GList        *tmp;
  gint          is_first;
  
  for(; nchars; nchars -= 1)
    {
      prop = MARK_CURRENT_PROPERTY(&text->point);
      
      prop->length -= 1;
      
      if (prop->length == 0)
	{
	  tmp = MARK_LIST_PTR (&text->point);
	  
	  is_first = tmp == text->text_properties;
	  
	  MARK_LIST_PTR (&text->point) = g_list_remove_link (tmp, tmp);
	  text->point.offset = 0;

	  if (btk_widget_get_realized (BTK_WIDGET (text)))
	    unrealize_property (text, prop);

	  destroy_text_property (prop);
	  g_list_free_1 (tmp);
	  
	  prop = MARK_CURRENT_PROPERTY (&text->point);
	  
	  if (is_first)
	    text->text_properties = MARK_LIST_PTR (&text->point);
	  
	  g_assert (prop->length != 0);
	}
      else if (prop->length == text->point.offset)
	{
	  MARK_LIST_PTR (&text->point) = MARK_NEXT_LIST_PTR (&text->point);
	  text->point.offset = 0;
	}
    }
  
  /* Check to see if we have just the single final position remaining
   * along in a property; if so, combine it with the previous property
   */
  if (LAST_INDEX (text, text->point) && 
      (MARK_OFFSET (&text->point) == 0) &&
      (MARK_PREV_LIST_PTR(&text->point) != NULL))
    {
      tmp = MARK_LIST_PTR (&text->point);
      prop = MARK_CURRENT_PROPERTY(&text->point);
      
      MARK_LIST_PTR (&text->point) = MARK_PREV_LIST_PTR (&text->point);
      MARK_CURRENT_PROPERTY(&text->point)->length += 1;
      MARK_NEXT_LIST_PTR(&text->point) = NULL;
      
      text->point.offset = MARK_CURRENT_PROPERTY(&text->point)->length - 1;
      
      if (btk_widget_get_realized (BTK_WIDGET (text)))
	unrealize_property (text, prop);

      destroy_text_property (prop);
      g_list_free_1 (tmp);
    }
}

static void
init_properties (BtkText *text)
{
  if (!text->text_properties)
    {
      text->text_properties = g_list_alloc();
      text->text_properties->next = NULL;
      text->text_properties->prev = NULL;
      text->text_properties->data = new_text_property (text, NULL, NULL, NULL, 1);
      text->text_properties_end = text->text_properties;
      
      SET_PROPERTY_MARK (&text->point, text->text_properties, 0);
      
      text->point.index = 0;
    }
}


/**********************************************************************/
/*			   Property Movement                          */
/**********************************************************************/

static void
move_mark_n (BtkPropertyMark* mark, gint n)
{
  if (n > 0)
    advance_mark_n(mark, n);
  else if (n < 0)
    decrement_mark_n(mark, -n);
}

static void
advance_mark (BtkPropertyMark* mark)
{
  TextProperty* prop = MARK_CURRENT_PROPERTY (mark);
  
  mark->index += 1;
  
  if (prop->length > mark->offset + 1)
    mark->offset += 1;
  else
    {
      mark->property = MARK_NEXT_LIST_PTR (mark);
      mark->offset   = 0;
    }
}

static void
advance_mark_n (BtkPropertyMark* mark, gint n)
{
  gint i;
  TextProperty* prop;

  g_assert (n > 0);

  i = 0;			/* otherwise it migth not be init. */
  prop = MARK_CURRENT_PROPERTY(mark);

  if ((prop->length - mark->offset - 1) < n) { /* if we need to change prop. */
    /* to make it easier */
    n += (mark->offset);
    mark->index -= mark->offset;
    mark->offset = 0;
    /* first we take seven-mile-leaps to get to the right text
     * property. */
    while ((n-i) > prop->length - 1) {
      i += prop->length;
      mark->index += prop->length;
      mark->property = MARK_NEXT_LIST_PTR (mark);
      prop = MARK_CURRENT_PROPERTY (mark);
    }
  }

  /* and then the rest */
  mark->index += n - i;
  mark->offset += n - i;
}

static void
decrement_mark (BtkPropertyMark* mark)
{
  mark->index -= 1;
  
  if (mark->offset > 0)
    mark->offset -= 1;
  else
    {
      mark->property = MARK_PREV_LIST_PTR (mark);
      mark->offset   = MARK_CURRENT_PROPERTY (mark)->length - 1;
    }
}

static void
decrement_mark_n (BtkPropertyMark* mark, gint n)
{
  g_assert (n > 0);

  while (mark->offset < n) {
    /* jump to end of prev */
    n -= mark->offset + 1;
    mark->index -= mark->offset + 1;
    mark->property = MARK_PREV_LIST_PTR (mark);
    mark->offset = MARK_CURRENT_PROPERTY (mark)->length - 1;
  }

  /* and the rest */
  mark->index -= n;
  mark->offset -= n;
}
 
static BtkPropertyMark
find_mark (BtkText* text, guint mark_position)
{
  return find_mark_near (text, mark_position, &text->point);
}

/*
 * You can also start from the end, what a drag.
 */
static BtkPropertyMark
find_mark_near (BtkText* text, guint mark_position, const BtkPropertyMark* near)
{
  gint diffa;
  gint diffb;
  
  BtkPropertyMark mark;
  
  if (!near)
    diffa = mark_position + 1;
  else
    diffa = mark_position - near->index;
  
  diffb = mark_position;
  
  if (diffa < 0)
    diffa = -diffa;
  
  if (diffa <= diffb)
    {
      mark = *near;
    }
  else
    {
      mark.index = 0;
      mark.property = text->text_properties;
      mark.offset = 0;
    }

  move_mark_n (&mark, mark_position - mark.index);
   
  return mark;
}

/* This routine must be called with scroll == FALSE, only when
 * point is at least partially on screen
 */

static void
find_line_containing_point (BtkText* text, guint point,
			    gboolean scroll)
{
  GList* cache;
  gint height;
  
  text->current_line = NULL;

  TEXT_SHOW (text);

  /* Scroll backwards until the point is on screen
   */
  while (CACHE_DATA(text->line_start_cache).start.index > point)
    scroll_int (text, - LINE_HEIGHT(CACHE_DATA(text->line_start_cache)));

  /* Now additionally try to make sure that the point is fully on screen
   */
  if (scroll)
    {
      while (text->first_cut_pixels != 0 && 
	     text->line_start_cache->next &&
	     CACHE_DATA(text->line_start_cache->next).start.index > point)
	scroll_int (text, - LINE_HEIGHT(CACHE_DATA(text->line_start_cache->next)));
    }

  bdk_drawable_get_size (text->text_area, NULL, &height);
  
  for (cache = text->line_start_cache; cache; cache = cache->next)
    {
      guint lph;
      
      if (CACHE_DATA(cache).end.index >= point ||
	  LAST_INDEX(text, CACHE_DATA(cache).end))
	{
	  text->current_line = cache; /* LOOK HERE, this proc has an
				       * important side effect. */
	  return;
	}
      
      TEXT_SHOW_LINE (text, cache, "cache");
      
      if (cache->next == NULL)
	fetch_lines_forward (text, 1);
      
      if (scroll)
	{
	  lph = pixel_height_of (text, cache->next);
	  
	  /* Scroll the bottom of the line is on screen, or until
	   * the line is the first onscreen line.
	   */
	  while (cache->next != text->line_start_cache && lph > height)
	    {
	      TEXT_SHOW_LINE (text, cache, "cache");
	      TEXT_SHOW_LINE (text, cache->next, "cache->next");
	      scroll_int (text, LINE_HEIGHT(CACHE_DATA(cache->next)));
	      lph = pixel_height_of (text, cache->next);
	    }
	}
    }
  
  g_assert_not_reached (); /* Must set text->current_line here */
}

static guint
pixel_height_of (BtkText* text, GList* cache_line)
{
  gint pixels = - text->first_cut_pixels;
  GList *cache = text->line_start_cache;
  
  while (TRUE) {
    pixels += LINE_HEIGHT (CACHE_DATA(cache));
    
    if (cache->data == cache_line->data)
      break;
    
    cache = cache->next;
  }
  
  return pixels;
}

/**********************************************************************/
/*			Search and Placement                          */
/**********************************************************************/

static gint
find_char_width (BtkText* text, const BtkPropertyMark *mark, const TabStopMark *tab_mark)
{
  BdkWChar ch;
  gint16* char_widths;
  
  if (LAST_INDEX (text, *mark))
    return 0;
  
  ch = BTK_TEXT_INDEX (text, mark->index);
  char_widths = MARK_CURRENT_TEXT_FONT (text, mark)->char_widths;

  if (ch == '\t')
    {
      return tab_mark->to_next_tab * char_widths[' '];
    }
  else if (ch < 256)
    {
      return char_widths[ch];
    }
  else
    {
      return bdk_char_width_wc(MARK_CURRENT_TEXT_FONT(text, mark)->bdk_font, ch);
    }
}

static void
advance_tab_mark (BtkText* text, TabStopMark* tab_mark, BdkWChar ch)
{
  if (tab_mark->to_next_tab == 1 || ch == '\t')
    {
      if (tab_mark->tab_stops->next)
	{
	  tab_mark->tab_stops = tab_mark->tab_stops->next;
	  tab_mark->to_next_tab = (gintptr) tab_mark->tab_stops->data;
	}
      else
	{
	  tab_mark->to_next_tab = text->default_tab_width;
	}
    }
  else
    {
      tab_mark->to_next_tab -= 1;
    }
}

static void
advance_tab_mark_n (BtkText* text, TabStopMark* tab_mark, gint n)
     /* No tabs! */
{
  while (n--)
    advance_tab_mark (text, tab_mark, 0);
}

static void
find_cursor_at_line (BtkText* text, const LineParams* start_line, gint pixel_height)
{
  BdkWChar ch;
  
  BtkPropertyMark mark        = start_line->start;
  TabStopMark  tab_mark    = start_line->tab_cont.tab_start;
  gint         pixel_width = LINE_START_PIXEL (*start_line);
  
  while (mark.index < text->cursor_mark.index)
    {
      pixel_width += find_char_width (text, &mark, &tab_mark);
      
      advance_tab_mark (text, &tab_mark, BTK_TEXT_INDEX(text, mark.index));
      advance_mark (&mark);
    }
  
  text->cursor_pos_x       = pixel_width;
  text->cursor_pos_y       = pixel_height;
  text->cursor_char_offset = start_line->font_descent;
  text->cursor_mark        = mark;
  
  ch = LAST_INDEX (text, mark) ? 
    LINE_DELIM : BTK_TEXT_INDEX (text, mark.index);
  
  if ((text->use_wchar) ? bdk_iswspace (ch) : isspace (ch))
    text->cursor_char = 0;
  else
    text->cursor_char = ch;
}

static void
find_cursor (BtkText* text, gboolean scroll)
{
  if (btk_widget_get_realized (BTK_WIDGET (text)))
    {
      find_line_containing_point (text, text->cursor_mark.index, scroll);
      
      if (text->current_line)
	find_cursor_at_line (text,
			     &CACHE_DATA(text->current_line),
			     pixel_height_of(text, text->current_line));
    }
  
  BTK_OLD_EDITABLE (text)->current_pos = text->cursor_mark.index;
}

static void
find_mouse_cursor_at_line (BtkText *text, const LineParams* lp,
			   guint line_pixel_height,
			   gint button_x)
{
  BtkPropertyMark mark     = lp->start;
  TabStopMark  tab_mark = lp->tab_cont.tab_start;
  
  gint char_width = find_char_width(text, &mark, &tab_mark);
  gint pixel_width = LINE_START_PIXEL (*lp) + (char_width+1)/2;
  
  text->cursor_pos_y = line_pixel_height;
  
  for (;;)
    {
      BdkWChar ch = LAST_INDEX (text, mark) ? 
	LINE_DELIM : BTK_TEXT_INDEX (text, mark.index);
      
      if (button_x < pixel_width || mark.index == lp->end.index)
	{
	  text->cursor_pos_x       = pixel_width - (char_width+1)/2;
	  text->cursor_mark        = mark;
	  text->cursor_char_offset = lp->font_descent;
	  
	  if ((text->use_wchar) ? bdk_iswspace (ch) : isspace (ch))
	    text->cursor_char = 0;
	  else
	    text->cursor_char = ch;
	  
	  break;
	}
      
      advance_tab_mark (text, &tab_mark, ch);
      advance_mark (&mark);
      
      pixel_width += char_width/2;
      
      char_width = find_char_width (text, &mark, &tab_mark);
      
      pixel_width += (char_width+1)/2;
    }
}

static void
find_mouse_cursor (BtkText* text, gint x, gint y)
{
  gint pixel_height;
  GList* cache = text->line_start_cache;
  
  g_assert (cache);
  
  pixel_height = - text->first_cut_pixels;
  
  for (; cache; cache = cache->next)
    {
      pixel_height += LINE_HEIGHT(CACHE_DATA(cache));
      
      if (y < pixel_height || !cache->next)
	{
	  find_mouse_cursor_at_line (text, &CACHE_DATA(cache), pixel_height, x);
	  
	  find_cursor (text, FALSE);
	  
	  return;
	}
    }
}

/**********************************************************************/
/*			    Cache Manager                             */
/**********************************************************************/

static void
free_cache (BtkText* text)
{
  GList* cache = text->line_start_cache;
  
  if (cache)
    {
      while (cache->prev)
	cache = cache->prev;
      
      text->line_start_cache = cache;
    }
  
  for (; cache; cache = cache->next)
    g_slice_free (LineParams, cache->data);
  
  g_list_free (text->line_start_cache);
  
  text->line_start_cache = NULL;
}

static GList*
remove_cache_line (BtkText* text, GList* member)
{
  GList *list;
  
  if (member == NULL)
    return NULL;
  
  if (member == text->line_start_cache)
    text->line_start_cache = text->line_start_cache->next;
  
  if (member->prev)
    member->prev->next = member->next;
  
  if (member->next)
    member->next->prev = member->prev;
  
  list = member->next;
  
  g_slice_free (LineParams, member->data);
  g_list_free_1 (member);
  
  return list;
}

/**********************************************************************/
/*			     Key Motion                               */
/**********************************************************************/

static void
move_cursor_buffer_ver (BtkText *text, int dir)
{
  undraw_cursor (text, FALSE);
  
  if (dir > 0)
    {
      scroll_int (text, text->vadj->upper);
      text->cursor_mark = find_this_line_start_mark (text,
						     TEXT_LENGTH (text),
						     &text->cursor_mark);
    }
  else
    {
      scroll_int (text, - text->vadj->value);
      text->cursor_mark = find_this_line_start_mark (text,
						     0,
						     &text->cursor_mark);
    }
  
  find_cursor (text, TRUE);
  draw_cursor (text, FALSE);
}

static void
move_cursor_page_ver (BtkText *text, int dir)
{
  scroll_int (text, dir * text->vadj->page_increment);
}

static void
move_cursor_ver (BtkText *text, int count)
{
  gint i;
  BtkPropertyMark mark;
  gint offset;
  
  mark = find_this_line_start_mark (text, text->cursor_mark.index, &text->cursor_mark);
  offset = text->cursor_mark.index - mark.index;
  
  if (offset > text->cursor_virtual_x)
    text->cursor_virtual_x = offset;
  
  if (count < 0)
    {
      if (mark.index == 0)
	return;
      
      decrement_mark (&mark);
      mark = find_this_line_start_mark (text, mark.index, &mark);
    }
  else
    {
      mark = text->cursor_mark;
      
      while (!LAST_INDEX(text, mark) && BTK_TEXT_INDEX(text, mark.index) != LINE_DELIM)
	advance_mark (&mark);
      
      if (LAST_INDEX(text, mark))
	return;
      
      advance_mark (&mark);
    }
  
  for (i=0; i < text->cursor_virtual_x; i += 1, advance_mark(&mark))
    if (LAST_INDEX(text, mark) ||
	BTK_TEXT_INDEX(text, mark.index) == LINE_DELIM)
      break;
  
  undraw_cursor (text, FALSE);
  
  text->cursor_mark = mark;
  
  find_cursor (text, TRUE);
  
  draw_cursor (text, FALSE);
}

static void
move_cursor_hor (BtkText *text, int count)
{
  /* count should be +-1. */
  if ( (count > 0 && text->cursor_mark.index + count > TEXT_LENGTH(text)) ||
       (count < 0 && text->cursor_mark.index < (- count)) ||
       (count == 0) )
    return;
  
  text->cursor_virtual_x = 0;
  
  undraw_cursor (text, FALSE);
  
  move_mark_n (&text->cursor_mark, count);
  
  find_cursor (text, TRUE);
  
  draw_cursor (text, FALSE);
}

static void 
btk_text_move_cursor (BtkOldEditable *old_editable,
		      gint            x,
		      gint            y)
{
  if (x > 0)
    {
      while (x-- != 0)
	move_cursor_hor (BTK_TEXT (old_editable), 1);
    }
  else if (x < 0)
    {
      while (x++ != 0)
	move_cursor_hor (BTK_TEXT (old_editable), -1);
    }
  
  if (y > 0)
    {
      while (y-- != 0)
	move_cursor_ver (BTK_TEXT (old_editable), 1);
    }
  else if (y < 0)
    {
      while (y++ != 0)
	move_cursor_ver (BTK_TEXT (old_editable), -1);
    }
}

static void
btk_text_move_forward_character (BtkText *text)
{
  move_cursor_hor (text, 1);
}

static void
btk_text_move_backward_character (BtkText *text)
{
  move_cursor_hor (text, -1);
}

static void
btk_text_move_next_line (BtkText *text)
{
  move_cursor_ver (text, 1);
}

static void
btk_text_move_previous_line (BtkText *text)
{
  move_cursor_ver (text, -1);
}

static void 
btk_text_move_word (BtkOldEditable *old_editable,
		    gint            n)
{
  if (n > 0)
    {
      while (n-- != 0)
	btk_text_move_forward_word (BTK_TEXT (old_editable));
    }
  else if (n < 0)
    {
      while (n++ != 0)
	btk_text_move_backward_word (BTK_TEXT (old_editable));
    }
}

static void
btk_text_move_forward_word (BtkText *text)
{
  text->cursor_virtual_x = 0;
  
  undraw_cursor (text, FALSE);
  
  if (text->use_wchar)
    {
      while (!LAST_INDEX (text, text->cursor_mark) && 
	     !bdk_iswalnum (BTK_TEXT_INDEX(text, text->cursor_mark.index)))
	advance_mark (&text->cursor_mark);
      
      while (!LAST_INDEX (text, text->cursor_mark) && 
	     bdk_iswalnum (BTK_TEXT_INDEX(text, text->cursor_mark.index)))
	advance_mark (&text->cursor_mark);
    }
  else
    {
      while (!LAST_INDEX (text, text->cursor_mark) && 
	     !isalnum (BTK_TEXT_INDEX(text, text->cursor_mark.index)))
	advance_mark (&text->cursor_mark);
      
      while (!LAST_INDEX (text, text->cursor_mark) && 
	     isalnum (BTK_TEXT_INDEX(text, text->cursor_mark.index)))
	advance_mark (&text->cursor_mark);
    }
  
  find_cursor (text, TRUE);
  draw_cursor (text, FALSE);
}

static void
btk_text_move_backward_word (BtkText *text)
{
  text->cursor_virtual_x = 0;
  
  undraw_cursor (text, FALSE);
  
  if (text->use_wchar)
    {
      while ((text->cursor_mark.index > 0) &&
	     !bdk_iswalnum (BTK_TEXT_INDEX(text, text->cursor_mark.index-1)))
	decrement_mark (&text->cursor_mark);
      
      while ((text->cursor_mark.index > 0) &&
	     bdk_iswalnum (BTK_TEXT_INDEX(text, text->cursor_mark.index-1)))
	decrement_mark (&text->cursor_mark);
    }
  else
    {
      while ((text->cursor_mark.index > 0) &&
	     !isalnum (BTK_TEXT_INDEX(text, text->cursor_mark.index-1)))
	decrement_mark (&text->cursor_mark);
      
      while ((text->cursor_mark.index > 0) &&
	     isalnum (BTK_TEXT_INDEX(text, text->cursor_mark.index-1)))
	decrement_mark (&text->cursor_mark);
    }
  
  find_cursor (text, TRUE);
  draw_cursor (text, FALSE);
}

static void 
btk_text_move_page (BtkOldEditable *old_editable,
		    gint            x,
		    gint            y)
{
  if (y != 0)
    scroll_int (BTK_TEXT (old_editable), 
		y * BTK_TEXT(old_editable)->vadj->page_increment);  
}

static void 
btk_text_move_to_row (BtkOldEditable *old_editable,
		      gint            row)
{
}

static void 
btk_text_move_to_column (BtkOldEditable *old_editable,
			 gint            column)
{
  BtkText *text;
  
  text = BTK_TEXT (old_editable);
  
  text->cursor_virtual_x = 0;	/* FIXME */
  
  undraw_cursor (text, FALSE);
  
  /* Move to the beginning of the line */
  while ((text->cursor_mark.index > 0) &&
	 (BTK_TEXT_INDEX (text, text->cursor_mark.index - 1) != LINE_DELIM))
    decrement_mark (&text->cursor_mark);
  
  while (!LAST_INDEX (text, text->cursor_mark) &&
	 (BTK_TEXT_INDEX (text, text->cursor_mark.index) != LINE_DELIM))
    {
      if (column > 0)
	column--;
      else if (column == 0)
	break;
      
      advance_mark (&text->cursor_mark);
    }
  
  find_cursor (text, TRUE);
  draw_cursor (text, FALSE);
}

static void
btk_text_move_beginning_of_line (BtkText *text)
{
  btk_text_move_to_column (BTK_OLD_EDITABLE (text), 0);
  
}

static void
btk_text_move_end_of_line (BtkText *text)
{
  btk_text_move_to_column (BTK_OLD_EDITABLE (text), -1);
}

static void 
btk_text_kill_char (BtkOldEditable *old_editable,
		    gint            direction)
{
  BtkText *text;
  
  text = BTK_TEXT (old_editable);
  
  if (old_editable->selection_start_pos != old_editable->selection_end_pos)
    btk_editable_delete_selection (BTK_EDITABLE (old_editable));
  else
    {
      if (direction >= 0)
	{
	  if (text->point.index + 1 <= TEXT_LENGTH (text))
	    btk_editable_delete_text (BTK_EDITABLE (old_editable), text->point.index, text->point.index + 1);
	}
      else
	{
	  if (text->point.index > 0)
	    btk_editable_delete_text (BTK_EDITABLE (old_editable), text->point.index - 1, text->point.index);
	}
    }
}

static void
btk_text_delete_forward_character (BtkText *text)
{
  btk_text_kill_char (BTK_OLD_EDITABLE (text), 1);
}

static void
btk_text_delete_backward_character (BtkText *text)
{
  btk_text_kill_char (BTK_OLD_EDITABLE (text), -1);
}

static void 
btk_text_kill_word (BtkOldEditable *old_editable,
		    gint            direction)
{
  if (old_editable->selection_start_pos != old_editable->selection_end_pos)
    btk_editable_delete_selection (BTK_EDITABLE (old_editable));
  else
    {
      gint old_pos = old_editable->current_pos;
      if (direction >= 0)
	{
	  btk_text_move_word (old_editable, 1);
	  btk_editable_delete_text (BTK_EDITABLE (old_editable), old_pos, old_editable->current_pos);
	}
      else
	{
	  btk_text_move_word (old_editable, -1);
	  btk_editable_delete_text (BTK_EDITABLE (old_editable), old_editable->current_pos, old_pos);
	}
    }
}

static void
btk_text_delete_forward_word (BtkText *text)
{
  btk_text_kill_word (BTK_OLD_EDITABLE (text), 1);
}

static void
btk_text_delete_backward_word (BtkText *text)
{
  btk_text_kill_word (BTK_OLD_EDITABLE (text), -1);
}

static void 
btk_text_kill_line (BtkOldEditable *old_editable,
		    gint            direction)
{
  gint old_pos = old_editable->current_pos;
  if (direction >= 0)
    {
      btk_text_move_to_column (old_editable, -1);
      btk_editable_delete_text (BTK_EDITABLE (old_editable), old_pos, old_editable->current_pos);
    }
  else
    {
      btk_text_move_to_column (old_editable, 0);
      btk_editable_delete_text (BTK_EDITABLE (old_editable), old_editable->current_pos, old_pos);
    }
}

static void
btk_text_delete_line (BtkText *text)
{
  btk_text_move_to_column (BTK_OLD_EDITABLE (text), 0);
  btk_text_kill_line (BTK_OLD_EDITABLE (text), 1);
}

static void
btk_text_delete_to_line_end (BtkText *text)
{
  btk_text_kill_line (BTK_OLD_EDITABLE (text), 1);
}

static void
btk_text_select_word (BtkText *text, guint32 time)
{
  gint start_pos;
  gint end_pos;
  
  BtkOldEditable *old_editable;
  old_editable = BTK_OLD_EDITABLE (text);
  
  btk_text_move_backward_word (text);
  start_pos = text->cursor_mark.index;
  
  btk_text_move_forward_word (text);
  end_pos = text->cursor_mark.index;
  
  old_editable->has_selection = TRUE;
  btk_text_set_selection (old_editable, start_pos, end_pos);
  btk_old_editable_claim_selection (old_editable, start_pos != end_pos, time);
}

static void
btk_text_select_line (BtkText *text, guint32 time)
{
  gint start_pos;
  gint end_pos;
  
  BtkOldEditable *old_editable;
  old_editable = BTK_OLD_EDITABLE (text);
  
  btk_text_move_beginning_of_line (text);
  start_pos = text->cursor_mark.index;
  
  btk_text_move_end_of_line (text);
  btk_text_move_forward_character (text);
  end_pos = text->cursor_mark.index;
  
  old_editable->has_selection = TRUE;
  btk_text_set_selection (old_editable, start_pos, end_pos);
  btk_old_editable_claim_selection (old_editable, start_pos != end_pos, time);
}

/**********************************************************************/
/*			      Scrolling                               */
/**********************************************************************/

static void
adjust_adj (BtkText* text, BtkAdjustment* adj)
{
  gint height;
  
  bdk_drawable_get_size (text->text_area, NULL, &height);
  
  adj->step_increment = MIN (adj->upper, SCROLL_PIXELS);
  adj->page_increment = MIN (adj->upper, height - KEY_SCROLL_PIXELS);
  adj->page_size      = MIN (adj->upper, height);
  adj->value          = MIN (adj->value, adj->upper - adj->page_size);
  adj->value          = MAX (adj->value, 0.0);
  
  btk_signal_emit_by_name (BTK_OBJECT (adj), "changed");
}

static gint
set_vertical_scroll_iterator (BtkText* text, LineParams* lp, void* data)
{
  SetVerticalScrollData *svdata = (SetVerticalScrollData *) data;
  
  if ((text->first_line_start_index >= lp->start.index) &&
      (text->first_line_start_index <= lp->end.index))
    {
      svdata->mark = lp->start;
  
      if (text->first_line_start_index == lp->start.index)
	{
	  text->first_onscreen_ver_pixel = svdata->pixel_height + text->first_cut_pixels;
	}
      else
	{
	  text->first_onscreen_ver_pixel = svdata->pixel_height;
	  text->first_cut_pixels = 0;
	}
      
      text->vadj->value = text->first_onscreen_ver_pixel;
    }
  
  svdata->pixel_height += LINE_HEIGHT (*lp);
  
  return FALSE;
}

static gint
set_vertical_scroll_find_iterator (BtkText* text, LineParams* lp, void* data)
{
  SetVerticalScrollData *svdata = (SetVerticalScrollData *) data;
  gint return_val;
  
  if (svdata->pixel_height <= (gint) text->vadj->value &&
      svdata->pixel_height + LINE_HEIGHT(*lp) > (gint) text->vadj->value)
    {
      svdata->mark = lp->start;
      
      text->first_cut_pixels = (gint)text->vadj->value - svdata->pixel_height;
      text->first_onscreen_ver_pixel = svdata->pixel_height;
      text->first_line_start_index = lp->start.index;
      
      return_val = TRUE;
    }
  else
    {
      svdata->pixel_height += LINE_HEIGHT (*lp);
      
      return_val = FALSE;
    }
  
  return return_val;
}

static BtkPropertyMark
set_vertical_scroll (BtkText* text)
{
  BtkPropertyMark mark = find_mark (text, 0);
  SetVerticalScrollData data;
  gint height;
  gint orib_value;
  
  data.pixel_height = 0;
  line_params_iterate (text, &mark, NULL, FALSE, &data, set_vertical_scroll_iterator);
  
  text->vadj->upper = data.pixel_height;
  orib_value = (gint) text->vadj->value;
  
  bdk_drawable_get_size (text->text_area, NULL, &height);
  
  text->vadj->step_increment = MIN (text->vadj->upper, SCROLL_PIXELS);
  text->vadj->page_increment = MIN (text->vadj->upper, height - KEY_SCROLL_PIXELS);
  text->vadj->page_size      = MIN (text->vadj->upper, height);
  text->vadj->value          = MIN (text->vadj->value, text->vadj->upper - text->vadj->page_size);
  text->vadj->value          = MAX (text->vadj->value, 0.0);
  
  text->last_ver_value = (gint)text->vadj->value;
  
  btk_signal_emit_by_name (BTK_OBJECT (text->vadj), "changed");
  
  if (text->vadj->value != orib_value)
    {
      /* We got clipped, and don't really know which line to put first. */
      data.pixel_height = 0;
      data.last_didnt_wrap = TRUE;
      
      line_params_iterate (text, &mark, NULL,
			   FALSE, &data,
			   set_vertical_scroll_find_iterator);
    }

  return data.mark;
}

static void
scroll_int (BtkText* text, gint diff)
{
  gdouble upper;
  
  text->vadj->value += diff;
  
  upper = text->vadj->upper - text->vadj->page_size;
  text->vadj->value = MIN (text->vadj->value, upper);
  text->vadj->value = MAX (text->vadj->value, 0.0);
  
  btk_signal_emit_by_name (BTK_OBJECT (text->vadj), "value-changed");
}

static void 
process_exposes (BtkText *text)
{
  BdkEvent *event;
  
  /* Make sure graphics expose events are processed before scrolling
   * again */
  
  while ((event = bdk_event_get_graphics_expose (text->text_area)) != NULL)
    {
      btk_widget_send_expose (BTK_WIDGET (text), event);
      if (event->expose.count == 0)
	{
	  bdk_event_free (event);
	  break;
	}
      bdk_event_free (event);
    }
}

static gint last_visible_line_height (BtkText* text)
{
  GList *cache = text->line_start_cache;
  gint height;
  
  bdk_drawable_get_size (text->text_area, NULL, &height);
  
  for (; cache->next; cache = cache->next)
    if (pixel_height_of(text, cache->next) > height)
      break;
  
  if (cache)
    return pixel_height_of(text, cache) - 1;
  else
    return 0;
}

static gint first_visible_line_height (BtkText* text)
{
  if (text->first_cut_pixels)
    return pixel_height_of(text, text->line_start_cache) + 1;
  else
    return 1;
}

static void
scroll_down (BtkText* text, gint diff0)
{
  BdkRectangle rect;
  gint real_diff = 0;
  gint width, height;
  
  text->first_onscreen_ver_pixel += diff0;
  
  while (diff0-- > 0)
    {
      g_assert (text->line_start_cache);
      
      if (text->first_cut_pixels < LINE_HEIGHT(CACHE_DATA(text->line_start_cache)) - 1)
	{
	  text->first_cut_pixels += 1;
	}
      else
	{
	  text->first_cut_pixels = 0;
	  
	  text->line_start_cache = text->line_start_cache->next;
	  
	  text->first_line_start_index =
	    CACHE_DATA(text->line_start_cache).start.index;
	  
	  if (!text->line_start_cache->next)
	    fetch_lines_forward (text, 1);
	}
      
      real_diff += 1;
    }
  
  bdk_drawable_get_size (text->text_area, &width, &height);
  if (height > real_diff)
    bdk_draw_drawable (text->text_area,
                       text->gc,
                       text->text_area,
                       0,
                       real_diff,
                       0,
                       0,
                       width,
                       height - real_diff);
  
  rect.x      = 0;
  rect.y      = MAX (0, height - real_diff);
  rect.width  = width;
  rect.height = MIN (height, real_diff);
  
  expose_text (text, &rect, FALSE);
  btk_text_draw_focus ( (BtkWidget *) text);
  
  if (text->current_line)
    {
      gint cursor_min;
      
      text->cursor_pos_y -= real_diff;
      cursor_min = drawn_cursor_min(text);
      
      if (cursor_min < 0)
	find_mouse_cursor (text, text->cursor_pos_x,
			   first_visible_line_height (text));
    }
  
  if (height > real_diff)
    process_exposes (text);
}

static void
scroll_up (BtkText* text, gint diff0)
{
  gint real_diff = 0;
  BdkRectangle rect;
  gint width, height;
  
  text->first_onscreen_ver_pixel += diff0;
  
  while (diff0++ < 0)
    {
      g_assert (text->line_start_cache);
      
      if (text->first_cut_pixels > 0)
	{
	  text->first_cut_pixels -= 1;
	}
      else
	{
	  if (!text->line_start_cache->prev)
	    fetch_lines_backward (text);
	  
	  text->line_start_cache = text->line_start_cache->prev;
	  
	  text->first_line_start_index =
	    CACHE_DATA(text->line_start_cache).start.index;
	  
	  text->first_cut_pixels = LINE_HEIGHT(CACHE_DATA(text->line_start_cache)) - 1;
	}
      
      real_diff += 1;
    }
  
  bdk_drawable_get_size (text->text_area, &width, &height);
  if (height > real_diff)
    bdk_draw_drawable (text->text_area,
                       text->gc,
                       text->text_area,
                       0,
                       0,
                       0,
                       real_diff,
                       width,
                       height - real_diff);
  
  rect.x      = 0;
  rect.y      = 0;
  rect.width  = width;
  rect.height = MIN (height, real_diff);
  
  expose_text (text, &rect, FALSE);
  btk_text_draw_focus ( (BtkWidget *) text);
  
  if (text->current_line)
    {
      gint cursor_max;
      gint height;
      
      text->cursor_pos_y += real_diff;
      cursor_max = drawn_cursor_max(text);
      bdk_drawable_get_size (text->text_area, NULL, &height);
      
      if (cursor_max >= height)
	find_mouse_cursor (text, text->cursor_pos_x,
			   last_visible_line_height (text));
    }
  
  if (height > real_diff)
    process_exposes (text);
}

/**********************************************************************/
/*			      Display Code                            */
/**********************************************************************/

/* Assumes mark starts a line.  Calculates the height, width, and
 * displayable character count of a single DISPLAYABLE line.  That
 * means that in line-wrap mode, this does may not compute the
 * properties of an entire line. */
static LineParams
find_line_params (BtkText* text,
		  const BtkPropertyMark* mark,
		  const PrevTabCont *tab_cont,
		  PrevTabCont *next_cont)
{
  LineParams lp;
  TabStopMark tab_mark = tab_cont->tab_start;
  guint max_display_pixels;
  BdkWChar ch;
  gint ch_width;
  BdkFont *font;
  
  bdk_drawable_get_size (text->text_area, (gint*) &max_display_pixels, NULL);
  max_display_pixels -= LINE_WRAP_ROOM;
  
  lp.wraps             = 0;
  lp.tab_cont          = *tab_cont;
  lp.start             = *mark;
  lp.end               = *mark;
  lp.pixel_width       = tab_cont->pixel_offset;
  lp.displayable_chars = 0;
  lp.font_ascent       = 0;
  lp.font_descent      = 0;
  
  init_tab_cont (text, next_cont);
  
  while (!LAST_INDEX(text, lp.end))
    {
      g_assert (lp.end.property);
      
      ch   = BTK_TEXT_INDEX (text, lp.end.index);
      font = MARK_CURRENT_FONT (text, &lp.end);

      if (ch == LINE_DELIM)
	{
	  /* Newline doesn't count in computation of line height, even
	   * if its in a bigger font than the rest of the line.  Unless,
	   * of course, there are no other characters. */
	  
	  if (!lp.font_ascent && !lp.font_descent)
	    {
	      lp.font_ascent = font->ascent;
	      lp.font_descent = font->descent;
	    }
	  
	  lp.tab_cont_next = *next_cont;
	  
	  return lp;
	}
      
      ch_width = find_char_width (text, &lp.end, &tab_mark);
      
      if ((ch_width + lp.pixel_width > max_display_pixels) &&
	  (lp.end.index > lp.start.index))
	{
	  lp.wraps = 1;
	  
	  if (text->line_wrap)
	    {
	      next_cont->tab_start    = tab_mark;
	      next_cont->pixel_offset = 0;
	      
	      if (ch == '\t')
		{
		  /* Here's the tough case, a tab is wrapping. */
		  gint pixels_avail = max_display_pixels - lp.pixel_width;
		  gint space_width  = MARK_CURRENT_TEXT_FONT(text, &lp.end)->char_widths[' '];
		  gint spaces_avail = pixels_avail / space_width;
		  
		  if (spaces_avail == 0)
		    {
		      decrement_mark (&lp.end);
		    }
		  else
		    {
		      advance_tab_mark (text, &next_cont->tab_start, '\t');
		      next_cont->pixel_offset = space_width * (tab_mark.to_next_tab -
							       spaces_avail);
		      lp.displayable_chars += 1;
		    }
		}
	      else
		{
		  if (text->word_wrap)
		    {
		      BtkPropertyMark saved_mark = lp.end;
		      guint saved_characters = lp.displayable_chars;
		      
		      lp.displayable_chars += 1;
		      
		      if (text->use_wchar)
			{
			  while (!bdk_iswspace (BTK_TEXT_INDEX (text, lp.end.index)) &&
				 (lp.end.index > lp.start.index))
			    {
			      decrement_mark (&lp.end);
			      lp.displayable_chars -= 1;
			    }
			}
		      else
			{
			  while (!isspace(BTK_TEXT_INDEX (text, lp.end.index)) &&
				 (lp.end.index > lp.start.index))
			    {
			      decrement_mark (&lp.end);
			      lp.displayable_chars -= 1;
			    }
			}
		      
		      /* If whole line is one word, revert to char wrapping */
		      if (lp.end.index == lp.start.index)
			{
			  lp.end = saved_mark;
			  lp.displayable_chars = saved_characters;
			  decrement_mark (&lp.end);
			}
		    }
		  else
		    {
		      /* Don't include this character, it will wrap. */
		      decrement_mark (&lp.end);
		    }
		}
	      
	      lp.tab_cont_next = *next_cont;
	      
	      return lp;
	    }
	}
      else
	{
	  lp.displayable_chars += 1;
	}
      
      lp.font_ascent = MAX (font->ascent, lp.font_ascent);
      lp.font_descent = MAX (font->descent, lp.font_descent);
      lp.pixel_width  += ch_width;
      
      advance_mark(&lp.end);
      advance_tab_mark (text, &tab_mark, ch);
    }
  
  if (LAST_INDEX(text, lp.start))
    {
      /* Special case, empty last line. */
      font = MARK_CURRENT_FONT (text, &lp.end);

      lp.font_ascent = font->ascent;
      lp.font_descent = font->descent;
    }
  
  lp.tab_cont_next = *next_cont;
  
  return lp;
}

static void
expand_scratch_buffer (BtkText* text, guint len)
{
  if (len >= text->scratch_buffer_len)
    {
      guint i = 1;
      
      while (i <= len && i < MIN_GAP_SIZE) i <<= 1;
      
      if (text->use_wchar)
        {
	  if (text->scratch_buffer.wc)
	    text->scratch_buffer.wc = g_new (BdkWChar, i);
	  else
	    text->scratch_buffer.wc = g_realloc (text->scratch_buffer.wc,
					      i*sizeof (BdkWChar));
        }
      else
        {
	  if (text->scratch_buffer.ch)
	    text->scratch_buffer.ch = g_new (guchar, i);
	  else
	    text->scratch_buffer.ch = g_realloc (text->scratch_buffer.ch, i);
        }
      
      text->scratch_buffer_len = i;
    }
}

/* Side effect: modifies text->gc
 */
static void
draw_bg_rect (BtkText* text, BtkPropertyMark *mark,
	      gint x, gint y, gint width, gint height,
	      gboolean already_cleared)
{
  BtkOldEditable *old_editable = BTK_OLD_EDITABLE (text);

  if ((mark->index >= MIN(old_editable->selection_start_pos, old_editable->selection_end_pos) &&
       mark->index < MAX(old_editable->selection_start_pos, old_editable->selection_end_pos)))
    {
      btk_paint_flat_box(BTK_WIDGET(text)->style, text->text_area,
			 old_editable->has_selection ?
			    BTK_STATE_SELECTED : BTK_STATE_ACTIVE, 
			 BTK_SHADOW_NONE,
			 NULL, BTK_WIDGET(text), "text",
			 x, y, width, height);
    }
  else if (!bdk_color_equal(MARK_CURRENT_BACK (text, mark),
			    &BTK_WIDGET(text)->style->base[btk_widget_get_state (BTK_WIDGET (text))]))
    {
      bdk_gc_set_foreground (text->gc, MARK_CURRENT_BACK (text, mark));

      bdk_draw_rectangle (text->text_area,
			  text->gc,
			  TRUE, x, y, width, height);
    }
  else if (BTK_WIDGET (text)->style->bg_pixmap[BTK_STATE_NORMAL])
    {
      BdkRectangle rect;
      
      rect.x = x;
      rect.y = y;
      rect.width = width;
      rect.height = height;
      
      clear_area (text, &rect);
    }
  else if (!already_cleared)
    bdk_window_clear_area (text->text_area, x, y, width, height);
}

static void
draw_line (BtkText* text,
	   gint pixel_start_height,
	   LineParams* lp)
{
  BdkGCValues gc_values;
  gint i;
  gint len = 0;
  guint running_offset = lp->tab_cont.pixel_offset;
  union { BdkWChar *wc; guchar *ch; } buffer;
  BdkGC *fg_gc;
  
  BtkOldEditable *old_editable = BTK_OLD_EDITABLE (text);
  
  guint selection_start_pos = MIN (old_editable->selection_start_pos, old_editable->selection_end_pos);
  guint selection_end_pos = MAX (old_editable->selection_start_pos, old_editable->selection_end_pos);
  
  BtkPropertyMark mark = lp->start;
  TabStopMark tab_mark = lp->tab_cont.tab_start;
  gint pixel_height = pixel_start_height + lp->font_ascent;
  guint chars = lp->displayable_chars;
  
  /* First provide a contiguous segment of memory.  This makes reading
   * the code below *much* easier, and only incurs the cost of copying
   * when the line being displayed spans the gap. */
  if (mark.index <= text->gap_position &&
      mark.index + chars > text->gap_position)
    {
      expand_scratch_buffer (text, chars);
      
      if (text->use_wchar)
	{
	  for (i = 0; i < chars; i += 1)
	    text->scratch_buffer.wc[i] = BTK_TEXT_INDEX(text, mark.index + i);
          buffer.wc = text->scratch_buffer.wc;
	}
      else
	{
	  for (i = 0; i < chars; i += 1)
	    text->scratch_buffer.ch[i] = BTK_TEXT_INDEX(text, mark.index + i);
	  buffer.ch = text->scratch_buffer.ch;
	}
    }
  else
    {
      if (text->use_wchar)
	{
	  if (mark.index >= text->gap_position)
	    buffer.wc = text->text.wc + mark.index + text->gap_size;
	  else
	    buffer.wc = text->text.wc + mark.index;
	}
      else
	{
	  if (mark.index >= text->gap_position)
	    buffer.ch = text->text.ch + mark.index + text->gap_size;
	  else
	    buffer.ch = text->text.ch + mark.index;
	}
    }
  
  
  if (running_offset > 0)
    {
      draw_bg_rect (text, &mark, 0, pixel_start_height, running_offset,
		    LINE_HEIGHT (*lp), TRUE);
    }
  
  while (chars > 0)
    {
      len = 0;
      if ((text->use_wchar && buffer.wc[0] != '\t') ||
	  (!text->use_wchar && buffer.ch[0] != '\t'))
	{
	  union { BdkWChar *wc; guchar *ch; } next_tab;
	  gint pixel_width;
	  BdkFont *font;

	  next_tab.wc = NULL;
	  if (text->use_wchar)
	    for (i=0; i<chars; i++)
	      {
		if (buffer.wc[i] == '\t')
		  {
		    next_tab.wc = buffer.wc + i;
		    break;
		  }
	      }
	  else
	    next_tab.ch = memchr (buffer.ch, '\t', chars);

	  len = MIN (MARK_CURRENT_PROPERTY (&mark)->length - mark.offset, chars);
	  
	  if (text->use_wchar)
	    {
	      if (next_tab.wc)
		len = MIN (len, next_tab.wc - buffer.wc);
	    }
	  else
	    {
	      if (next_tab.ch)
		len = MIN (len, next_tab.ch - buffer.ch);
	    }

	  if (mark.index < selection_start_pos)
	    len = MIN (len, selection_start_pos - mark.index);
	  else if (mark.index < selection_end_pos)
	    len = MIN (len, selection_end_pos - mark.index);

	  font = MARK_CURRENT_FONT (text, &mark);
	  if (font->type == BDK_FONT_FONT)
	    {
	      bdk_gc_set_font (text->gc, font);
	      bdk_gc_get_values (text->gc, &gc_values);
	      if (text->use_wchar)
	        pixel_width = bdk_text_width_wc (gc_values.font,
						 buffer.wc, len);
	      else
	      pixel_width = bdk_text_width (gc_values.font,
					    (gchar *)buffer.ch, len);
	    }
	  else
	    {
	      if (text->use_wchar)
		pixel_width = bdk_text_width_wc (font, buffer.wc, len);
	      else
		pixel_width = bdk_text_width (font, (gchar *)buffer.ch, len);
	    }
	  
	  draw_bg_rect (text, &mark, running_offset, pixel_start_height,
			pixel_width, LINE_HEIGHT (*lp), TRUE);
	  
	  if ((mark.index >= selection_start_pos) && 
	      (mark.index < selection_end_pos))
	    {
	      if (old_editable->has_selection)
		fg_gc = BTK_WIDGET(text)->style->text_gc[BTK_STATE_SELECTED];
	      else
		fg_gc = BTK_WIDGET(text)->style->text_gc[BTK_STATE_ACTIVE];
	    }
	  else
	    {
	      bdk_gc_set_foreground (text->gc, MARK_CURRENT_FORE (text, &mark));
	      fg_gc = text->gc;
	    }

	  if (text->use_wchar)
	    bdk_draw_text_wc (text->text_area, MARK_CURRENT_FONT (text, &mark),
			      fg_gc,
			      running_offset,
			      pixel_height,
			      buffer.wc,
			      len);
	  else
	    bdk_draw_text (text->text_area, MARK_CURRENT_FONT (text, &mark),
			   fg_gc,
			   running_offset,
			   pixel_height,
			   (gchar *)buffer.ch,
			   len);
	  
	  running_offset += pixel_width;
	  
	  advance_tab_mark_n (text, &tab_mark, len);
	}
      else
	{
	  gint pixels_remaining;
	  gint space_width;
	  gint spaces_avail;
	      
	  len = 1;
	  
	  bdk_drawable_get_size (text->text_area, &pixels_remaining, NULL);
	  pixels_remaining -= (LINE_WRAP_ROOM + running_offset);
	  
	  space_width = MARK_CURRENT_TEXT_FONT(text, &mark)->char_widths[' '];
	  
	  spaces_avail = pixels_remaining / space_width;
	  spaces_avail = MIN (spaces_avail, tab_mark.to_next_tab);

	  draw_bg_rect (text, &mark, running_offset, pixel_start_height,
			spaces_avail * space_width, LINE_HEIGHT (*lp), TRUE);

	  running_offset += tab_mark.to_next_tab *
	    MARK_CURRENT_TEXT_FONT(text, &mark)->char_widths[' '];

	  advance_tab_mark (text, &tab_mark, '\t');
	}
      
      advance_mark_n (&mark, len);
      if (text->use_wchar)
	buffer.wc += len;
      else
	buffer.ch += len;
      chars -= len;
    }
}

static void
draw_line_wrap (BtkText* text, guint height /* baseline height */)
{
  gint width;
  BdkPixmap *bitmap;
  gint bitmap_width;
  gint bitmap_height;
  
  if (text->line_wrap)
    {
      bitmap = text->line_wrap_bitmap;
      bitmap_width = line_wrap_width;
      bitmap_height = line_wrap_height;
    }
  else
    {
      bitmap = text->line_arrow_bitmap;
      bitmap_width = line_arrow_width;
      bitmap_height = line_arrow_height;
    }
  
  bdk_drawable_get_size (text->text_area, &width, NULL);
  width -= LINE_WRAP_ROOM;
  
  bdk_gc_set_stipple (text->gc,
		      bitmap);
  
  bdk_gc_set_fill (text->gc, BDK_STIPPLED);
  
  bdk_gc_set_foreground (text->gc, &BTK_WIDGET (text)->style->text[BTK_STATE_NORMAL]);
  
  bdk_gc_set_ts_origin (text->gc,
			width + 1,
			height - bitmap_height - 1);
  
  bdk_draw_rectangle (text->text_area,
		      text->gc,
		      TRUE,
		      width + 1,
		      height - bitmap_height - 1 /* one pixel above the baseline. */,
		      bitmap_width,
		      bitmap_height);
  
  bdk_gc_set_ts_origin (text->gc, 0, 0);
  
  bdk_gc_set_fill (text->gc, BDK_SOLID);
}

static void
undraw_cursor (BtkText* text, gint absolute)
{
  BtkOldEditable *old_editable = (BtkOldEditable *) text;

  TDEBUG (("in undraw_cursor\n"));
  
  if (absolute)
    text->cursor_drawn_level = 0;
  
  if ((text->cursor_drawn_level ++ == 0) &&
      (old_editable->selection_start_pos == old_editable->selection_end_pos) &&
      BTK_WIDGET_DRAWABLE (text) && text->line_start_cache)
    {
      BdkFont* font;
      
      g_assert(text->cursor_mark.property);

      font = MARK_CURRENT_FONT(text, &text->cursor_mark);

      draw_bg_rect (text, &text->cursor_mark, 
		    text->cursor_pos_x,
		    text->cursor_pos_y - text->cursor_char_offset - font->ascent,
		    1, font->ascent + 1, FALSE);
      
      if (text->cursor_char)
	{
	  if (font->type == BDK_FONT_FONT)
	    bdk_gc_set_font (text->gc, font);

	  bdk_gc_set_foreground (text->gc, MARK_CURRENT_FORE (text, &text->cursor_mark));

	  bdk_draw_text_wc (text->text_area, font,
			 text->gc,
			 text->cursor_pos_x,
			 text->cursor_pos_y - text->cursor_char_offset,
			 &text->cursor_char,
			 1);
	}
    }
}

static gint
drawn_cursor_min (BtkText* text)
{
  BdkFont* font;
  
  g_assert(text->cursor_mark.property);
  
  font = MARK_CURRENT_FONT(text, &text->cursor_mark);
  
  return text->cursor_pos_y - text->cursor_char_offset - font->ascent;
}

static gint
drawn_cursor_max (BtkText* text)
{
  g_assert(text->cursor_mark.property);
  
  return text->cursor_pos_y - text->cursor_char_offset;
}

static void
draw_cursor (BtkText* text, gint absolute)
{
  BtkOldEditable *old_editable = (BtkOldEditable *)text;
  
  TDEBUG (("in draw_cursor\n"));
  
  if (absolute)
    text->cursor_drawn_level = 1;
  
  if ((--text->cursor_drawn_level == 0) &&
      old_editable->editable &&
      (old_editable->selection_start_pos == old_editable->selection_end_pos) &&
      BTK_WIDGET_DRAWABLE (text) && text->line_start_cache)
    {
      BdkFont* font;
      
      g_assert (text->cursor_mark.property);

      font = MARK_CURRENT_FONT (text, &text->cursor_mark);

      bdk_gc_set_foreground (text->gc, &BTK_WIDGET (text)->style->text[BTK_STATE_NORMAL]);
      
      bdk_draw_line (text->text_area, text->gc, text->cursor_pos_x,
		     text->cursor_pos_y - text->cursor_char_offset,
		     text->cursor_pos_x,
		     text->cursor_pos_y - text->cursor_char_offset - font->ascent);
    }
}

static BdkGC *
create_bg_gc (BtkText *text)
{
  BdkGCValues values;
  
  values.tile = BTK_WIDGET (text)->style->bg_pixmap[BTK_STATE_NORMAL];
  values.fill = BDK_TILED;

  return bdk_gc_new_with_values (text->text_area, &values,
				 BDK_GC_FILL | BDK_GC_TILE);
}

static void
clear_area (BtkText *text, BdkRectangle *area)
{
  BtkWidget *widget = BTK_WIDGET (text);
  
  if (text->bg_gc)
    {
      gint width, height;
      
      bdk_drawable_get_size (widget->style->bg_pixmap[BTK_STATE_NORMAL], &width, &height);
      
      bdk_gc_set_ts_origin (text->bg_gc,
			    (- text->first_onscreen_hor_pixel) % width,
			    (- text->first_onscreen_ver_pixel) % height);

      bdk_draw_rectangle (text->text_area, text->bg_gc, TRUE,
			  area->x, area->y, area->width, area->height);
    }
  else
    bdk_window_clear_area (text->text_area, area->x, area->y, area->width, area->height);
}

static void
expose_text (BtkText* text, BdkRectangle *area, gboolean cursor)
{
  GList *cache = text->line_start_cache;
  gint pixels = - text->first_cut_pixels;
  gint min_y = MAX (0, area->y);
  gint max_y = MAX (0, area->y + area->height);
  gint height;
  
  bdk_drawable_get_size (text->text_area, NULL, &height);
  max_y = MIN (max_y, height);
  
  TDEBUG (("in expose x=%d y=%d w=%d h=%d\n", area->x, area->y, area->width, area->height));
  
  clear_area (text, area);
  
  for (; pixels < height; cache = cache->next)
    {
      if (pixels < max_y && (pixels + (gint)LINE_HEIGHT(CACHE_DATA(cache))) >= min_y)
	{
	  draw_line (text, pixels, &CACHE_DATA(cache));
	  
	  if (CACHE_DATA(cache).wraps)
	    draw_line_wrap (text, pixels + CACHE_DATA(cache).font_ascent);
	}
      
      if (cursor && btk_widget_has_focus (BTK_WIDGET (text)))
	{
	  if (CACHE_DATA(cache).start.index <= text->cursor_mark.index &&
	      CACHE_DATA(cache).end.index >= text->cursor_mark.index)
	    {
	      /* We undraw and draw the cursor here to get the drawn
	       * level right ... FIXME - maybe the second parameter
	       * of draw_cursor should work differently
	       */
	      undraw_cursor (text, FALSE);
	      draw_cursor (text, FALSE);
	    }
	}
      
      pixels += LINE_HEIGHT(CACHE_DATA(cache));
      
      if (!cache->next)
	{
	  fetch_lines_forward (text, 1);
	  
	  if (!cache->next)
	    break;
	}
    }
}

static void 
btk_text_update_text (BtkOldEditable    *old_editable,
		      gint               start_pos,
		      gint               end_pos)
{
  BtkText *text = BTK_TEXT (old_editable);
  
  GList *cache = text->line_start_cache;
  gint pixels = - text->first_cut_pixels;
  BdkRectangle area;
  gint width;
  gint height;
  
  if (end_pos < 0)
    end_pos = TEXT_LENGTH (text);
  
  if (end_pos < start_pos)
    return;
  
  bdk_drawable_get_size (text->text_area, &width, &height);
  area.x = 0;
  area.y = -1;
  area.width = width;
  area.height = 0;
  
  TDEBUG (("in expose span start=%d stop=%d\n", start_pos, end_pos));
  
  for (; pixels < height; cache = cache->next)
    {
      if (CACHE_DATA(cache).start.index < end_pos)
	{
	  if (CACHE_DATA(cache).end.index >= start_pos)
	    {
	      if (area.y < 0)
		area.y = MAX(0,pixels);
	      area.height = pixels + LINE_HEIGHT(CACHE_DATA(cache)) - area.y;
	    }
	}
      else
	break;
      
      pixels += LINE_HEIGHT(CACHE_DATA(cache));
      
      if (!cache->next)
	{
	  fetch_lines_forward (text, 1);
	  
	  if (!cache->next)
	    break;
	}
    }
  
  if (area.y >= 0)
    expose_text (text, &area, TRUE);
}

static void
recompute_geometry (BtkText* text)
{
  BtkPropertyMark mark, start_mark;
  GList *new_lines;
  gint height;
  gint width;
  
  free_cache (text);
  
  mark = start_mark = set_vertical_scroll (text);

  /* We need a real start of a line when calling fetch_lines().
   * not the start of a wrapped line.
   */
  while (mark.index > 0 &&
	 BTK_TEXT_INDEX (text, mark.index - 1) != LINE_DELIM)
    decrement_mark (&mark);

  bdk_drawable_get_size (text->text_area, &width, &height);

  /* Fetch an entire line, to make sure that we get all the text
   * we backed over above, in addition to enough text to fill up
   * the space vertically
   */

  new_lines = fetch_lines (text,
			   &mark,
			   NULL,
			   FetchLinesCount,
			   1);

  mark = CACHE_DATA (g_list_last (new_lines)).end;
  if (!LAST_INDEX (text, mark))
    {
      advance_mark (&mark);

      new_lines = g_list_concat (new_lines, 
				 fetch_lines (text,
					      &mark,
					      NULL,
					      FetchLinesPixels,
					      height + text->first_cut_pixels));
    }

  /* Now work forward to the actual first onscreen line */

  while (CACHE_DATA (new_lines).start.index < start_mark.index)
    new_lines = new_lines->next;
  
  text->line_start_cache = new_lines;
  
  find_cursor (text, TRUE);
}

/**********************************************************************/
/*                            Selection                               */
/**********************************************************************/

static void 
btk_text_set_selection  (BtkOldEditable  *old_editable,
			 gint             start,
			 gint             end)
{
  BtkText *text = BTK_TEXT (old_editable);
  
  guint start1, end1, start2, end2;
  
  if (end < 0)
    end = TEXT_LENGTH (text);
  
  start1 = MIN(start,end);
  end1 = MAX(start,end);
  start2 = MIN(old_editable->selection_start_pos, old_editable->selection_end_pos);
  end2 = MAX(old_editable->selection_start_pos, old_editable->selection_end_pos);
  
  if (start2 < start1)
    {
      guint tmp;
      
      tmp = start1; start1 = start2; start2 = tmp;
      tmp = end1;   end1   = end2;   end2   = tmp;
    }
  
  undraw_cursor (text, FALSE);
  old_editable->selection_start_pos = start;
  old_editable->selection_end_pos = end;
  draw_cursor (text, FALSE);
  
  /* Expose only what changed */
  
  if (start1 < start2)
    btk_text_update_text (old_editable, start1, MIN(end1, start2));
  
  if (end2 > end1)
    btk_text_update_text (old_editable, MAX(end1, start2), end2);
  else if (end2 < end1)
    btk_text_update_text (old_editable, end2, end1);
}


/**********************************************************************/
/*                              Debug                                 */
/**********************************************************************/

#ifdef DEBUG_BTK_TEXT
static void
btk_text_show_cache_line (BtkText *text, GList *cache,
			  const char* what, const char* func, gint line)
{
  LineParams *lp = &CACHE_DATA(cache);
  gint i;
  
  if (cache == text->line_start_cache)
    g_message ("Line Start Cache: ");
  
  if (cache == text->current_line)
    g_message("Current Line: ");
  
  g_message ("%s:%d: cache line %s s=%d,e=%d,lh=%d (",
	     func,
	     line,
	     what,
	     lp->start.index,
	     lp->end.index,
	     LINE_HEIGHT(*lp));
  
  for (i = lp->start.index; i < (lp->end.index + lp->wraps); i += 1)
    g_message ("%c", BTK_TEXT_INDEX (text, i));
  
  g_message (")\n");
}

static void
btk_text_show_cache (BtkText *text, const char* func, gint line)
{
  GList *l = text->line_start_cache;
  
  if (!l) {
    return;
  }
  
  /* back up to the absolute beginning of the line cache */
  while (l->prev)
    l = l->prev;
  
  g_message ("*** line cache ***\n");
  for (; l; l = l->next)
    btk_text_show_cache_line (text, l, "all", func, line);
}

static void
btk_text_assert_mark (BtkText         *text,
		      BtkPropertyMark *mark,
		      BtkPropertyMark *before,
		      BtkPropertyMark *after,
		      const gchar     *msg,
		      const gchar     *where,
		      gint             line)
{
  BtkPropertyMark correct_mark = find_mark (text, mark->index);
  
  if (mark->offset != correct_mark.offset ||
      mark->property != correct_mark.property)
    g_warning ("incorrect %s text property marker in %s:%d, index %d -- bad!", where, msg, line, mark->index);
}

static void
btk_text_assert (BtkText         *text,
		 const gchar     *msg,
		 gint             line)
{
  GList* cache = text->line_start_cache;
  BtkPropertyMark* before_mark = NULL;
  BtkPropertyMark* after_mark = NULL;
  
  btk_text_show_props (text, msg, line);
  
  for (; cache->prev; cache = cache->prev)
    /* nothing */;
  
  g_message ("*** line markers ***\n");
  
  for (; cache; cache = cache->next)
    {
      after_mark = &CACHE_DATA(cache).end;
      btk_text_assert_mark (text, &CACHE_DATA(cache).start, before_mark, after_mark, msg, "start", line);
      before_mark = &CACHE_DATA(cache).start;
      
      if (cache->next)
	after_mark = &CACHE_DATA(cache->next).start;
      else
	after_mark = NULL;
      
      btk_text_assert_mark (text, &CACHE_DATA(cache).end, before_mark, after_mark, msg, "end", line);
      before_mark = &CACHE_DATA(cache).end;
    }
}

static void
btk_text_show_adj (BtkText *text,
		   BtkAdjustment *adj,
		   const char* what,
		   const char* func,
		   gint line)
{
  g_message ("*** adjustment ***\n");
  
  g_message ("%s:%d: %s adjustment l=%.1f u=%.1f v=%.1f si=%.1f pi=%.1f ps=%.1f\n",
	     func,
	     line,
	     what,
	     adj->lower,
	     adj->upper,
	     adj->value,
	     adj->step_increment,
	     adj->page_increment,
	     adj->page_size);
}

static void
btk_text_show_props (BtkText *text,
		     const char* msg,
		     int line)
{
  GList* props = text->text_properties;
  int proplen = 0;
  
  g_message ("%s:%d: ", msg, line);
  
  for (; props; props = props->next)
    {
      TextProperty *p = (TextProperty*)props->data;
      
      proplen += p->length;

      g_message ("[%d,%p,", p->length, p);
      if (p->flags & PROPERTY_FONT)
	g_message ("%p,", p->font);
      else
	g_message ("-,");
      if (p->flags & PROPERTY_FOREGROUND)
	g_message ("%ld, ", p->fore_color.pixel);
      else
	g_message ("-,");
      if (p->flags & PROPERTY_BACKGROUND)
	g_message ("%ld] ", p->back_color.pixel);
      else
	g_message ("-] ");
    }
  
  g_message ("\n");
  
  if (proplen - 1 != TEXT_LENGTH(text))
    g_warning ("incorrect property list length in %s:%d -- bad!", msg, line);
}
#endif

#define __BTK_TEXT_C__
#include "btkaliasdef.c"
