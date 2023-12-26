/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * BTK Calendar Widget
 * Copyright (C) 1998 Cesar Miquel, Shawn T. Amundson and Mattias Groenlund
 * 
 * lib_date routines
 * Copyright (c) 1995, 1996, 1997, 1998 by Steffen Beyer
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
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#include "config.h"

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE__NL_TIME_FIRST_WEEKDAY
#include <langinfo.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <bunnylib.h>

#ifdef G_OS_WIN32
#include <windows.h>
#endif

#include "btkcalendar.h"
#include "btkdnd.h"
#include "btkintl.h"
#include "btkmain.h"
#include "btkmarshalers.h"
#include "btktooltip.h"
#include "btkprivate.h"
#include "bdk/bdkkeysyms.h"
#include "btkalias.h"

/***************************************************************************/
/* The following date routines are taken from the lib_date package. 
 * They have been minimally edited to avoid conflict with types defined
 * in win32 headers.
 */

static const guint month_length[2][13] =
{
  { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
  { 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

static const guint days_in_months[2][14] =
{
  { 0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
  { 0, 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
};

static glong  calc_days(guint year, guint mm, guint dd);
static guint  day_of_week(guint year, guint mm, guint dd);
static glong  dates_difference(guint year1, guint mm1, guint dd1,
			       guint year2, guint mm2, guint dd2);
static guint  weeks_in_year(guint year);

static gboolean 
leap (guint year)
{
  return((((year % 4) == 0) && ((year % 100) != 0)) || ((year % 400) == 0));
}

static guint 
day_of_week (guint year, guint mm, guint dd)
{
  glong  days;
  
  days = calc_days(year, mm, dd);
  if (days > 0L)
    {
      days--;
      days %= 7L;
      days++;
    }
  return( (guint) days );
}

static guint weeks_in_year(guint year)
{
  return(52 + ((day_of_week(year,1,1)==4) || (day_of_week(year,12,31)==4)));
}

static gboolean 
check_date(guint year, guint mm, guint dd)
{
  if (year < 1) return FALSE;
  if ((mm < 1) || (mm > 12)) return FALSE;
  if ((dd < 1) || (dd > month_length[leap(year)][mm])) return FALSE;
  return TRUE;
}

static guint 
week_number(guint year, guint mm, guint dd)
{
  guint first;
  
  first = day_of_week(year,1,1) - 1;
  return( (guint) ( (dates_difference(year,1,1, year,mm,dd) + first) / 7L ) +
	  (first < 4) );
}

static glong 
year_to_days(guint year)
{
  return( year * 365L + (year / 4) - (year / 100) + (year / 400) );
}


static glong 
calc_days(guint year, guint mm, guint dd)
{
  gboolean lp;
  
  if (year < 1) return(0L);
  if ((mm < 1) || (mm > 12)) return(0L);
  if ((dd < 1) || (dd > month_length[(lp = leap(year))][mm])) return(0L);
  return( year_to_days(--year) + days_in_months[lp][mm] + dd );
}

static gboolean 
week_of_year(guint *week, guint *year, guint mm, guint dd)
{
  if (check_date(*year,mm,dd))
    {
      *week = week_number(*year,mm,dd);
      if (*week == 0) 
	*week = weeks_in_year(--(*year));
      else if (*week > weeks_in_year(*year))
	{
	  *week = 1;
	  (*year)++;
	}
      return TRUE;
    }
  return FALSE;
}

static glong 
dates_difference(guint year1, guint mm1, guint dd1,
		 guint year2, guint mm2, guint dd2)
{
  return( calc_days(year2, mm2, dd2) - calc_days(year1, mm1, dd1) );
}

/*** END OF lib_date routines ********************************************/

/* Spacing around day/week headers and main area, inside those windows */
#define CALENDAR_MARGIN		 0

#define DAY_XSEP		 0 /* not really good for small calendar */
#define DAY_YSEP		 0 /* not really good for small calendar */

#define SCROLL_DELAY_FACTOR      5

/* Color usage */
#define HEADER_FG_COLOR(widget)		 (& (widget)->style->fg[btk_widget_get_state (widget)])
#define HEADER_BG_COLOR(widget)		 (& (widget)->style->bg[btk_widget_get_state (widget)])
#define SELECTED_BG_COLOR(widget)	 (& (widget)->style->base[btk_widget_has_focus (widget) ? BTK_STATE_SELECTED : BTK_STATE_ACTIVE])
#define SELECTED_FG_COLOR(widget)	 (& (widget)->style->text[btk_widget_has_focus (widget) ? BTK_STATE_SELECTED : BTK_STATE_ACTIVE])
#define NORMAL_DAY_COLOR(widget)	 (& (widget)->style->text[btk_widget_get_state (widget)])
#define PREV_MONTH_COLOR(widget)	 (& (widget)->style->mid[btk_widget_get_state (widget)])
#define NEXT_MONTH_COLOR(widget)	 (& (widget)->style->mid[btk_widget_get_state (widget)])
#define MARKED_COLOR(widget)		 (& (widget)->style->text[btk_widget_get_state (widget)])
#define BACKGROUND_COLOR(widget)	 (& (widget)->style->base[btk_widget_get_state (widget)])
#define HIGHLIGHT_BACK_COLOR(widget)	 (& (widget)->style->mid[btk_widget_get_state (widget)])

enum {
  ARROW_YEAR_LEFT,
  ARROW_YEAR_RIGHT,
  ARROW_MONTH_LEFT,
  ARROW_MONTH_RIGHT
};

enum {
  MONTH_PREV,
  MONTH_CURRENT,
  MONTH_NEXT
};

enum {
  MONTH_CHANGED_SIGNAL,
  DAY_SELECTED_SIGNAL,
  DAY_SELECTED_DOUBLE_CLICK_SIGNAL,
  PREV_MONTH_SIGNAL,
  NEXT_MONTH_SIGNAL,
  PREV_YEAR_SIGNAL,
  NEXT_YEAR_SIGNAL,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_YEAR,
  PROP_MONTH,
  PROP_DAY,
  PROP_SHOW_HEADING,
  PROP_SHOW_DAY_NAMES,
  PROP_NO_MONTH_CHANGE,
  PROP_SHOW_WEEK_NUMBERS,
  PROP_SHOW_DETAILS,
  PROP_DETAIL_WIDTH_CHARS,
  PROP_DETAIL_HEIGHT_ROWS
};

static guint btk_calendar_signals[LAST_SIGNAL] = { 0 };

struct _BtkCalendarPrivate
{
  BdkWindow *header_win;
  BdkWindow *day_name_win;
  BdkWindow *main_win;
  BdkWindow *week_win;
  BdkWindow *arrow_win[4];

  guint header_h;
  guint day_name_h;
  guint main_h;

  guint	     arrow_state[4];
  guint	     arrow_width;
  guint	     max_month_width;
  guint	     max_year_width;
  
  guint day_width;
  guint week_width;

  guint min_day_width;
  guint max_day_char_width;
  guint max_day_char_ascent;
  guint max_day_char_descent;
  guint max_label_char_ascent;
  guint max_label_char_descent;
  guint max_week_char_width;
  
  /* flags */
  guint year_before : 1;

  guint need_timer  : 1;

  guint in_drag : 1;
  guint drag_highlight : 1;

  guint32 timer;
  gint click_child;

  gint week_start;

  gint drag_start_x;
  gint drag_start_y;

  /* Optional callback, used to display extra information for each day. */
  BtkCalendarDetailFunc detail_func;
  gpointer              detail_func_user_data;
  GDestroyNotify        detail_func_destroy;

  /* Size requistion for details provided by the hook. */
  gint detail_height_rows;
  gint detail_width_chars;
  gint detail_overflow[6];
};

#define BTK_CALENDAR_GET_PRIVATE(widget)  (BTK_CALENDAR (widget)->priv)

static void btk_calendar_finalize     (BObject      *calendar);
static void btk_calendar_destroy      (BtkObject    *calendar);
static void btk_calendar_set_property (BObject      *object,
				       guint         prop_id,
				       const BValue *value,
				       BParamSpec   *pspec);
static void btk_calendar_get_property (BObject      *object,
				       guint         prop_id,
				       BValue       *value,
				       BParamSpec   *pspec);

static void     btk_calendar_realize        (BtkWidget        *widget);
static void     btk_calendar_unrealize      (BtkWidget        *widget);
static void     btk_calendar_size_request   (BtkWidget        *widget,
					     BtkRequisition   *requisition);
static void     btk_calendar_size_allocate  (BtkWidget        *widget,
					     BtkAllocation    *allocation);
static gboolean btk_calendar_expose         (BtkWidget        *widget,
					     BdkEventExpose   *event);
static gboolean btk_calendar_button_press   (BtkWidget        *widget,
					     BdkEventButton   *event);
static gboolean btk_calendar_button_release (BtkWidget        *widget,
					     BdkEventButton   *event);
static gboolean btk_calendar_motion_notify  (BtkWidget        *widget,
					     BdkEventMotion   *event);
static gboolean btk_calendar_enter_notify   (BtkWidget        *widget,
					     BdkEventCrossing *event);
static gboolean btk_calendar_leave_notify   (BtkWidget        *widget,
					     BdkEventCrossing *event);
static gboolean btk_calendar_scroll         (BtkWidget        *widget,
					     BdkEventScroll   *event);
static gboolean btk_calendar_key_press      (BtkWidget        *widget,
					     BdkEventKey      *event);
static gboolean btk_calendar_focus_out      (BtkWidget        *widget,
					     BdkEventFocus    *event);
static void     btk_calendar_grab_notify    (BtkWidget        *widget,
					     gboolean          was_grabbed);
static void     btk_calendar_state_changed  (BtkWidget        *widget,
					     BtkStateType      previous_state);
static void     btk_calendar_style_set      (BtkWidget        *widget,
					     BtkStyle         *previous_style);
static gboolean btk_calendar_query_tooltip  (BtkWidget        *widget,
					     gint              x,
					     gint              y,
					     gboolean          keyboard_mode,
					     BtkTooltip       *tooltip);

static void     btk_calendar_drag_data_get      (BtkWidget        *widget,
						 BdkDragContext   *context,
						 BtkSelectionData *selection_data,
						 guint             info,
						 guint             time);
static void     btk_calendar_drag_data_received (BtkWidget        *widget,
						 BdkDragContext   *context,
						 gint              x,
						 gint              y,
						 BtkSelectionData *selection_data,
						 guint             info,
						 guint             time);
static gboolean btk_calendar_drag_motion        (BtkWidget        *widget,
						 BdkDragContext   *context,
						 gint              x,
						 gint              y,
						 guint             time);
static void     btk_calendar_drag_leave         (BtkWidget        *widget,
						 BdkDragContext   *context,
						 guint             time);
static gboolean btk_calendar_drag_drop          (BtkWidget        *widget,
						 BdkDragContext   *context,
						 gint              x,
						 gint              y,
						 guint             time);

static void calendar_start_spinning (BtkCalendar *calendar,
				     gint         click_child);
static void calendar_stop_spinning  (BtkCalendar *calendar);

static void calendar_invalidate_day     (BtkCalendar *widget,
					 gint       row,
					 gint       col);
static void calendar_invalidate_day_num (BtkCalendar *widget,
					 gint       day);
static void calendar_invalidate_arrow   (BtkCalendar *widget,
					 guint      arrow);

static void calendar_compute_days      (BtkCalendar *calendar);
static gint calendar_get_xsep          (BtkCalendar *calendar);
static gint calendar_get_ysep          (BtkCalendar *calendar);

static char    *default_abbreviated_dayname[7];
static char    *default_monthname[12];

G_DEFINE_TYPE (BtkCalendar, btk_calendar, BTK_TYPE_WIDGET)

static void
btk_calendar_class_init (BtkCalendarClass *class)
{
  BObjectClass   *bobject_class;
  BtkObjectClass   *object_class;
  BtkWidgetClass *widget_class;

  bobject_class = (BObjectClass*)  class;
  object_class = (BtkObjectClass*)  class;
  widget_class = (BtkWidgetClass*) class;
  
  bobject_class->set_property = btk_calendar_set_property;
  bobject_class->get_property = btk_calendar_get_property;
  bobject_class->finalize = btk_calendar_finalize;

  object_class->destroy = btk_calendar_destroy;

  widget_class->realize = btk_calendar_realize;
  widget_class->unrealize = btk_calendar_unrealize;
  widget_class->expose_event = btk_calendar_expose;
  widget_class->size_request = btk_calendar_size_request;
  widget_class->size_allocate = btk_calendar_size_allocate;
  widget_class->button_press_event = btk_calendar_button_press;
  widget_class->button_release_event = btk_calendar_button_release;
  widget_class->motion_notify_event = btk_calendar_motion_notify;
  widget_class->enter_notify_event = btk_calendar_enter_notify;
  widget_class->leave_notify_event = btk_calendar_leave_notify;
  widget_class->key_press_event = btk_calendar_key_press;
  widget_class->scroll_event = btk_calendar_scroll;
  widget_class->style_set = btk_calendar_style_set;
  widget_class->state_changed = btk_calendar_state_changed;
  widget_class->grab_notify = btk_calendar_grab_notify;
  widget_class->focus_out_event = btk_calendar_focus_out;
  widget_class->query_tooltip = btk_calendar_query_tooltip;

  widget_class->drag_data_get = btk_calendar_drag_data_get;
  widget_class->drag_motion = btk_calendar_drag_motion;
  widget_class->drag_leave = btk_calendar_drag_leave;
  widget_class->drag_drop = btk_calendar_drag_drop;
  widget_class->drag_data_received = btk_calendar_drag_data_received;
  
  /**
   * BtkCalendar:year:
   *
   * The selected year. 
   * This property gets initially set to the current year.
   */  
  g_object_class_install_property (bobject_class,
                                   PROP_YEAR,
                                   g_param_spec_int ("year",
						     P_("Year"),
						     P_("The selected year"),
						     0, G_MAXINT >> 9, 0,
						     BTK_PARAM_READWRITE));

  /**
   * BtkCalendar:month:
   *
   * The selected month (as a number between 0 and 11). 
   * This property gets initially set to the current month.
   */
  g_object_class_install_property (bobject_class,
                                   PROP_MONTH,
                                   g_param_spec_int ("month",
						     P_("Month"),
						     P_("The selected month (as a number between 0 and 11)"),
						     0, 11, 0,
						     BTK_PARAM_READWRITE));

  /**
   * BtkCalendar:day:
   *
   * The selected day (as a number between 1 and 31, or 0 
   * to unselect the currently selected day).
   * This property gets initially set to the current day.
   */
  g_object_class_install_property (bobject_class,
                                   PROP_DAY,
                                   g_param_spec_int ("day",
						     P_("Day"),
						     P_("The selected day (as a number between 1 and 31, or 0 to unselect the currently selected day)"),
						     0, 31, 0,
						     BTK_PARAM_READWRITE));

/**
 * BtkCalendar:show-heading:
 *
 * Determines whether a heading is displayed.
 *
 * Since: 2.4
 */
  g_object_class_install_property (bobject_class,
                                   PROP_SHOW_HEADING,
                                   g_param_spec_boolean ("show-heading",
							 P_("Show Heading"),
							 P_("If TRUE, a heading is displayed"),
							 TRUE,
							 BTK_PARAM_READWRITE));

/**
 * BtkCalendar:show-day-names:
 *
 * Determines whether day names are displayed.
 *
 * Since: 2.4
 */
  g_object_class_install_property (bobject_class,
                                   PROP_SHOW_DAY_NAMES,
                                   g_param_spec_boolean ("show-day-names",
							 P_("Show Day Names"),
							 P_("If TRUE, day names are displayed"),
							 TRUE,
							 BTK_PARAM_READWRITE));
/**
 * BtkCalendar:no-month-change:
 *
 * Determines whether the selected month can be changed.
 *
 * Since: 2.4
 */
  g_object_class_install_property (bobject_class,
                                   PROP_NO_MONTH_CHANGE,
                                   g_param_spec_boolean ("no-month-change",
							 P_("No Month Change"),
							 P_("If TRUE, the selected month cannot be changed"),
							 FALSE,
							 BTK_PARAM_READWRITE));

/**
 * BtkCalendar:show-week-numbers:
 *
 * Determines whether week numbers are displayed.
 *
 * Since: 2.4
 */
  g_object_class_install_property (bobject_class,
                                   PROP_SHOW_WEEK_NUMBERS,
                                   g_param_spec_boolean ("show-week-numbers",
							 P_("Show Week Numbers"),
							 P_("If TRUE, week numbers are displayed"),
							 FALSE,
							 BTK_PARAM_READWRITE));

/**
 * BtkCalendar:detail-width-chars:
 *
 * Width of a detail cell, in characters.
 * A value of 0 allows any width. See btk_calendar_set_detail_func().
 *
 * Since: 2.14
 */
  g_object_class_install_property (bobject_class,
                                   PROP_DETAIL_WIDTH_CHARS,
                                   g_param_spec_int ("detail-width-chars",
						     P_("Details Width"),
						     P_("Details width in characters"),
						     0, 127, 0,
						     BTK_PARAM_READWRITE));

/**
 * BtkCalendar:detail-height-rows:
 *
 * Height of a detail cell, in rows.
 * A value of 0 allows any width. See btk_calendar_set_detail_func().
 *
 * Since: 2.14
 */
  g_object_class_install_property (bobject_class,
                                   PROP_DETAIL_HEIGHT_ROWS,
                                   g_param_spec_int ("detail-height-rows",
						     P_("Details Height"),
						     P_("Details height in rows"),
						     0, 127, 0,
						     BTK_PARAM_READWRITE));

/**
 * BtkCalendar:show-details:
 *
 * Determines whether details are shown directly in the widget, or if they are
 * available only as tooltip. When this property is set days with details are
 * marked.
 *
 * Since: 2.14
 */
  g_object_class_install_property (bobject_class,
                                   PROP_SHOW_DETAILS,
                                   g_param_spec_boolean ("show-details",
							 P_("Show Details"),
							 P_("If TRUE, details are shown"),
							 TRUE,
							 BTK_PARAM_READWRITE));


  /**
   * BtkCalendar:inner-border
   *
   * The spacing around the day/week headers and main area.
   */
  btk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("inner-border",
                                                             P_("Inner border"),
                                                             P_("Inner border space"),
                                                             0, G_MAXINT, 4,
                                                             BTK_PARAM_READABLE));

  /**
   * BtkCalndar:vertical-separation
   *
   * Separation between day headers and main area.
   */
  btk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("vertical-separation",
                                                             P_("Vertical separation"),
                                                             P_("Space between day headers and main area"),
                                                             0, G_MAXINT, 4,
                                                             BTK_PARAM_READABLE));

  /**
   * BtkCalendar:horizontal-separation
   *
   * Separation between week headers and main area.
   */
  btk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("horizontal-separation",
                                                             P_("Horizontal separation"),
                                                             P_("Space between week headers and main area"),
                                                             0, G_MAXINT, 4,
                                                             BTK_PARAM_READABLE));

  /**
   * BtkCalendar::month-changed:
   * @calendar: the object which received the signal.
   *
   * Emitted when the user clicks a button to change the selected month on a
   * calendar.
   */
  btk_calendar_signals[MONTH_CHANGED_SIGNAL] =
    g_signal_new (I_("month-changed"),
		  B_OBJECT_CLASS_TYPE (bobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkCalendarClass, month_changed),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  B_TYPE_NONE, 0);
  btk_calendar_signals[DAY_SELECTED_SIGNAL] =
    g_signal_new (I_("day-selected"),
		  B_OBJECT_CLASS_TYPE (bobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkCalendarClass, day_selected),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  B_TYPE_NONE, 0);
  btk_calendar_signals[DAY_SELECTED_DOUBLE_CLICK_SIGNAL] =
    g_signal_new (I_("day-selected-double-click"),
		  B_OBJECT_CLASS_TYPE (bobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkCalendarClass, day_selected_double_click),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  B_TYPE_NONE, 0);
  btk_calendar_signals[PREV_MONTH_SIGNAL] =
    g_signal_new (I_("prev-month"),
		  B_OBJECT_CLASS_TYPE (bobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkCalendarClass, prev_month),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  B_TYPE_NONE, 0);
  btk_calendar_signals[NEXT_MONTH_SIGNAL] =
    g_signal_new (I_("next-month"),
		  B_OBJECT_CLASS_TYPE (bobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkCalendarClass, next_month),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  B_TYPE_NONE, 0);
  btk_calendar_signals[PREV_YEAR_SIGNAL] =
    g_signal_new (I_("prev-year"),
		  B_OBJECT_CLASS_TYPE (bobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkCalendarClass, prev_year),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  B_TYPE_NONE, 0);
  btk_calendar_signals[NEXT_YEAR_SIGNAL] =
    g_signal_new (I_("next-year"),
		  B_OBJECT_CLASS_TYPE (bobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkCalendarClass, next_year),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  B_TYPE_NONE, 0);
  
  g_type_class_add_private (bobject_class, sizeof (BtkCalendarPrivate));
}

static void
btk_calendar_init (BtkCalendar *calendar)
{
  BtkWidget *widget = BTK_WIDGET (calendar);
  time_t secs;
  struct tm *tm;
  gint i;
#ifdef G_OS_WIN32
  wchar_t wbuffer[100];
#else
  static const char *month_format = NULL;
  char buffer[255];
  time_t tmp_time;
#endif
  BtkCalendarPrivate *priv;
  gchar *year_before;
#ifdef HAVE__NL_TIME_FIRST_WEEKDAY
  union { unsigned int word; char *string; } langinfo;
  gint week_1stday = 0;
  gint first_weekday = 1;
  guint week_origin;
#else
  gchar *week_start;
#endif

  priv = calendar->priv = B_TYPE_INSTANCE_GET_PRIVATE (calendar,
						       BTK_TYPE_CALENDAR,
						       BtkCalendarPrivate);

  btk_widget_set_can_focus (widget, TRUE);
  
  if (!default_abbreviated_dayname[0])
    for (i=0; i<7; i++)
      {
#ifndef G_OS_WIN32
	tmp_time= (i+3)*86400;
	strftime (buffer, sizeof (buffer), "%a", gmtime (&tmp_time));
	default_abbreviated_dayname[i] = g_locale_to_utf8 (buffer, -1, NULL, NULL, NULL);
#else
	if (!GetLocaleInfoW (GetThreadLocale (), LOCALE_SABBREVDAYNAME1 + (i+6)%7,
			     wbuffer, G_N_ELEMENTS (wbuffer)))
	  default_abbreviated_dayname[i] = g_strdup_printf ("(%d)", i);
	else
	  default_abbreviated_dayname[i] = g_utf16_to_utf8 (wbuffer, -1, NULL, NULL, NULL);
#endif
      }
  
  if (!default_monthname[0])
    for (i=0; i<12; i++)
      {
#ifndef G_OS_WIN32
	tmp_time=i*2764800;
	if (B_UNLIKELY (month_format == NULL))
	  {
	    buffer[0] = '\0';
	    month_format = "%OB";
	    strftime (buffer, sizeof (buffer), month_format, gmtime (&tmp_time));
	    /* "%OB" is not supported in Linux with bunnylibc < 2.27  */
	    if (!strcmp (buffer, "%OB") || !strcmp (buffer, "OB") || !strcmp (buffer, ""))
	      {
		month_format = "%B";
		strftime (buffer, sizeof (buffer), month_format, gmtime (&tmp_time));
	      }
	  }
	else
	  strftime (buffer, sizeof (buffer), month_format, gmtime (&tmp_time));

	default_monthname[i] = g_locale_to_utf8 (buffer, -1, NULL, NULL, NULL);
#else
	if (!GetLocaleInfoW (GetThreadLocale (), LOCALE_SMONTHNAME1 + i,
			     wbuffer, G_N_ELEMENTS (wbuffer)))
	  default_monthname[i] = g_strdup_printf ("(%d)", i);
	else
	  default_monthname[i] = g_utf16_to_utf8 (wbuffer, -1, NULL, NULL, NULL);
#endif
      }
  
  /* Set defaults */
  secs = time (NULL);
  tm = localtime (&secs);
  calendar->month = tm->tm_mon;
  calendar->year  = 1900 + tm->tm_year;

  for (i=0;i<31;i++)
    calendar->marked_date[i] = FALSE;
  calendar->num_marked_dates = 0;
  calendar->selected_day = tm->tm_mday;
  
  calendar->display_flags = (BTK_CALENDAR_SHOW_HEADING |
			     BTK_CALENDAR_SHOW_DAY_NAMES |
			     BTK_CALENDAR_SHOW_DETAILS);
  
  calendar->highlight_row = -1;
  calendar->highlight_col = -1;
  
  calendar->focus_row = -1;
  calendar->focus_col = -1;

  priv->max_year_width = 0;
  priv->max_month_width = 0;
  priv->max_day_char_width = 0;
  priv->max_week_char_width = 0;

  priv->max_day_char_ascent = 0;
  priv->max_day_char_descent = 0;
  priv->max_label_char_ascent = 0;
  priv->max_label_char_descent = 0;

  priv->arrow_width = 10;

  priv->need_timer = 0;
  priv->timer = 0;
  priv->click_child = -1;

  priv->in_drag = 0;
  priv->drag_highlight = 0;

  btk_drag_dest_set (widget, 0, NULL, 0, BDK_ACTION_COPY);
  btk_drag_dest_add_text_targets (widget);

  priv->year_before = 0;

  /* Translate to calendar:YM if you want years to be displayed
   * before months; otherwise translate to calendar:MY.
   * Do *not* translate it to anything else, if it
   * it isn't calendar:YM or calendar:MY it will not work.
   *
   * Note that the ordering described here is logical order, which is
   * further influenced by BIDI ordering. Thus, if you have a default
   * text direction of RTL and specify "calendar:YM", then the year
   * will appear to the right of the month.
   */
  year_before = _("calendar:MY");
  if (strcmp (year_before, "calendar:YM") == 0)
    priv->year_before = 1;
  else if (strcmp (year_before, "calendar:MY") != 0)
    g_warning ("Whoever translated calendar:MY did so wrongly.\n");

#ifdef G_OS_WIN32
  priv->week_start = 0;
  week_start = NULL;

  if (GetLocaleInfoW (GetThreadLocale (), LOCALE_IFIRSTDAYOFWEEK,
		      wbuffer, G_N_ELEMENTS (wbuffer)))
    week_start = g_utf16_to_utf8 (wbuffer, -1, NULL, NULL, NULL);
      
  if (week_start != NULL)
    {
      priv->week_start = (week_start[0] - '0' + 1) % 7;
      g_free(week_start);
    }
#else
#ifdef HAVE__NL_TIME_FIRST_WEEKDAY
  langinfo.string = nl_langinfo (_NL_TIME_FIRST_WEEKDAY);
  first_weekday = langinfo.string[0];
  langinfo.string = nl_langinfo (_NL_TIME_WEEK_1STDAY);
  week_origin = langinfo.word;
  if (week_origin == 19971130) /* Sunday */
    week_1stday = 0;
  else if (week_origin == 19971201) /* Monday */
    week_1stday = 1;
  else
    g_warning ("Unknown value of _NL_TIME_WEEK_1STDAY.\n");

  priv->week_start = (week_1stday + first_weekday - 1) % 7;
#else
  /* Translate to calendar:week_start:0 if you want Sunday to be the
   * first day of the week to calendar:week_start:1 if you want Monday
   * to be the first day of the week, and so on.
   */  
  week_start = _("calendar:week_start:0");

  if (strncmp (week_start, "calendar:week_start:", 20) == 0)
    priv->week_start = *(week_start + 20) - '0';
  else 
    priv->week_start = -1;
  
  if (priv->week_start < 0 || priv->week_start > 6)
    {
      g_warning ("Whoever translated calendar:week_start:0 did so wrongly.\n");
      priv->week_start = 0;
    }
#endif
#endif

  calendar_compute_days (calendar);
}


/****************************************
 *          Utility Functions           *
 ****************************************/

static void
calendar_queue_refresh (BtkCalendar *calendar)
{
  BtkCalendarPrivate *priv = BTK_CALENDAR_GET_PRIVATE (calendar);

  if (!(priv->detail_func) ||
      !(calendar->display_flags & BTK_CALENDAR_SHOW_DETAILS) ||
       (priv->detail_width_chars && priv->detail_height_rows))
    btk_widget_queue_draw (BTK_WIDGET (calendar));
  else
    btk_widget_queue_resize (BTK_WIDGET (calendar));
}

static void
calendar_set_month_next (BtkCalendar *calendar)
{
  gint month_len;

  if (calendar->display_flags & BTK_CALENDAR_NO_MONTH_CHANGE)
    return;
  
  
  if (calendar->month == 11)
    {
      calendar->month = 0;
      calendar->year++;
    } 
  else 
    calendar->month++;
  
  calendar_compute_days (calendar);
  g_signal_emit (calendar,
		 btk_calendar_signals[NEXT_MONTH_SIGNAL],
		 0);
  g_signal_emit (calendar,
		 btk_calendar_signals[MONTH_CHANGED_SIGNAL],
		 0);
  
  month_len = month_length[leap (calendar->year)][calendar->month + 1];
  
  if (month_len < calendar->selected_day)
    {
      calendar->selected_day = 0;
      btk_calendar_select_day (calendar, month_len);
    }
  else
    btk_calendar_select_day (calendar, calendar->selected_day);

  calendar_queue_refresh (calendar);
}

static void
calendar_set_year_prev (BtkCalendar *calendar)
{
  gint month_len;

  calendar->year--;
  calendar_compute_days (calendar);
  g_signal_emit (calendar,
		 btk_calendar_signals[PREV_YEAR_SIGNAL],
		 0);
  g_signal_emit (calendar,
		 btk_calendar_signals[MONTH_CHANGED_SIGNAL],
		 0);
  
  month_len = month_length[leap (calendar->year)][calendar->month + 1];
  
  if (month_len < calendar->selected_day)
    {
      calendar->selected_day = 0;
      btk_calendar_select_day (calendar, month_len);
    }
  else
    btk_calendar_select_day (calendar, calendar->selected_day);
  
  calendar_queue_refresh (calendar);
}

static void
calendar_set_year_next (BtkCalendar *calendar)
{
  gint month_len;

  calendar->year++;
  calendar_compute_days (calendar);
  g_signal_emit (calendar,
		 btk_calendar_signals[NEXT_YEAR_SIGNAL],
		 0);
  g_signal_emit (calendar,
		 btk_calendar_signals[MONTH_CHANGED_SIGNAL],
		 0);
  
  month_len = month_length[leap (calendar->year)][calendar->month + 1];
  
  if (month_len < calendar->selected_day)
    {
      calendar->selected_day = 0;
      btk_calendar_select_day (calendar, month_len);
    }
  else
    btk_calendar_select_day (calendar, calendar->selected_day);
  
  calendar_queue_refresh (calendar);
}

static void
calendar_compute_days (BtkCalendar *calendar)
{
  BtkCalendarPrivate *priv = BTK_CALENDAR_GET_PRIVATE (BTK_WIDGET (calendar));
  gint month;
  gint year;
  gint ndays_in_month;
  gint ndays_in_prev_month;
  gint first_day;
  gint row;
  gint col;
  gint day;

  year = calendar->year;
  month = calendar->month + 1;
  
  ndays_in_month = month_length[leap (year)][month];
  
  first_day = day_of_week (year, month, 1);
  first_day = (first_day + 7 - priv->week_start) % 7;
  
  /* Compute days of previous month */
  if (month > 1)
    ndays_in_prev_month = month_length[leap (year)][month-1];
  else
    ndays_in_prev_month = month_length[leap (year)][12];
  day = ndays_in_prev_month - first_day + 1;
  
  row = 0;
  if (first_day > 0)
    {
      for (col = 0; col < first_day; col++)
	{
	  calendar->day[row][col] = day;
	  calendar->day_month[row][col] = MONTH_PREV;
	  day++;
	}
    }
  
  /* Compute days of current month */
  col = first_day;
  for (day = 1; day <= ndays_in_month; day++)
    {
      calendar->day[row][col] = day;
      calendar->day_month[row][col] = MONTH_CURRENT;
      
      col++;
      if (col == 7)
	{
	  row++;
	  col = 0;
	}
    }
  
  /* Compute days of next month */
  day = 1;
  for (; row <= 5; row++)
    {
      for (; col <= 6; col++)
	{
	  calendar->day[row][col] = day;
	  calendar->day_month[row][col] = MONTH_NEXT;
	  day++;
	}
      col = 0;
    }
}

static void
calendar_select_and_focus_day (BtkCalendar *calendar,
			       guint        day)
{
  gint old_focus_row = calendar->focus_row;
  gint old_focus_col = calendar->focus_col;
  gint row;
  gint col;
  
  for (row = 0; row < 6; row ++)
    for (col = 0; col < 7; col++)
      {
	if (calendar->day_month[row][col] == MONTH_CURRENT 
	    && calendar->day[row][col] == day)
	  {
	    calendar->focus_row = row;
	    calendar->focus_col = col;
	  }
      }

  if (old_focus_row != -1 && old_focus_col != -1)
    calendar_invalidate_day (calendar, old_focus_row, old_focus_col);
  
  btk_calendar_select_day (calendar, day);
}


/****************************************
 *     Layout computation utilities     *
 ****************************************/

static gint
calendar_row_height (BtkCalendar *calendar)
{
  return (BTK_CALENDAR_GET_PRIVATE (calendar)->main_h - CALENDAR_MARGIN
	  - ((calendar->display_flags & BTK_CALENDAR_SHOW_DAY_NAMES)
	     ? calendar_get_ysep (calendar) : CALENDAR_MARGIN)) / 6;
}


/* calendar_left_x_for_column: returns the x coordinate
 * for the left of the column */
static gint
calendar_left_x_for_column (BtkCalendar *calendar,
			    gint	 column)
{
  gint width;
  gint x_left;
  gint calendar_xsep = calendar_get_xsep (calendar);

  if (btk_widget_get_direction (BTK_WIDGET (calendar)) == BTK_TEXT_DIR_RTL)
    column = 6 - column;

  width = BTK_CALENDAR_GET_PRIVATE (calendar)->day_width;
  if (calendar->display_flags & BTK_CALENDAR_SHOW_WEEK_NUMBERS)
    x_left = calendar_xsep + (width + DAY_XSEP) * column;
  else
    x_left = CALENDAR_MARGIN + (width + DAY_XSEP) * column;
  
  return x_left;
}

/* column_from_x: returns the column 0-6 that the
 * x pixel of the xwindow is in */
static gint
calendar_column_from_x (BtkCalendar *calendar,
			gint	     event_x)
{
  gint c, column;
  gint x_left, x_right;
  
  column = -1;
  
  for (c = 0; c < 7; c++)
    {
      x_left = calendar_left_x_for_column (calendar, c);
      x_right = x_left + BTK_CALENDAR_GET_PRIVATE (calendar)->day_width;
      
      if (event_x >= x_left && event_x < x_right)
	{
	  column = c;
	  break;
	}
    }
  
  return column;
}

/* calendar_top_y_for_row: returns the y coordinate
 * for the top of the row */
static gint
calendar_top_y_for_row (BtkCalendar *calendar,
			gint	     row)
{
  
  return (BTK_CALENDAR_GET_PRIVATE (calendar)->main_h 
	  - (CALENDAR_MARGIN + (6 - row)
	     * calendar_row_height (calendar)));
}

/* row_from_y: returns the row 0-5 that the
 * y pixel of the xwindow is in */
static gint
calendar_row_from_y (BtkCalendar *calendar,
		     gint	  event_y)
{
  gint r, row;
  gint height;
  gint y_top, y_bottom;
  
  height = calendar_row_height (calendar);
  row = -1;
  
  for (r = 0; r < 6; r++)
    {
      y_top = calendar_top_y_for_row (calendar, r);
      y_bottom = y_top + height;
      
      if (event_y >= y_top && event_y < y_bottom)
	{
	  row = r;
	  break;
	}
    }
  
  return row;
}

static void
calendar_arrow_rectangle (BtkCalendar  *calendar,
			  guint	        arrow,
			  BdkRectangle *rect)
{
  BtkWidget *widget = BTK_WIDGET (calendar);
  BtkCalendarPrivate *priv = BTK_CALENDAR_GET_PRIVATE (calendar);
  gboolean year_left;

  if (btk_widget_get_direction (widget) == BTK_TEXT_DIR_LTR) 
    year_left = priv->year_before;
  else
    year_left = !priv->year_before;
    
  rect->y = 3;
  rect->width = priv->arrow_width;
  rect->height = priv->header_h - 7;
  
  switch (arrow)
    {
    case ARROW_MONTH_LEFT:
      if (year_left) 
	rect->x = (widget->allocation.width - 2 * widget->style->xthickness
		   - (3 + 2*priv->arrow_width 
		      + priv->max_month_width));
      else
	rect->x = 3;
      break;
    case ARROW_MONTH_RIGHT:
      if (year_left) 
	rect->x = (widget->allocation.width - 2 * widget->style->xthickness 
		   - 3 - priv->arrow_width);
      else
	rect->x = (priv->arrow_width 
		   + priv->max_month_width);
      break;
    case ARROW_YEAR_LEFT:
      if (year_left) 
	rect->x = 3;
      else
	rect->x = (widget->allocation.width - 2 * widget->style->xthickness
		   - (3 + 2*priv->arrow_width 
		      + priv->max_year_width));
      break;
    case ARROW_YEAR_RIGHT:
      if (year_left) 
	rect->x = (priv->arrow_width 
		   + priv->max_year_width);
      else
	rect->x = (widget->allocation.width - 2 * widget->style->xthickness 
		   - 3 - priv->arrow_width);
      break;
    }
}

static void
calendar_day_rectangle (BtkCalendar  *calendar,
			gint          row,
			gint          col,
			BdkRectangle *rect)
{
  BtkCalendarPrivate *priv = BTK_CALENDAR_GET_PRIVATE (calendar);

  rect->x = calendar_left_x_for_column (calendar, col);
  rect->y = calendar_top_y_for_row (calendar, row);
  rect->height = calendar_row_height (calendar);
  rect->width = priv->day_width;
}

static void
calendar_set_month_prev (BtkCalendar *calendar)
{
  gint month_len;
  
  if (calendar->display_flags & BTK_CALENDAR_NO_MONTH_CHANGE)
    return;
  
  if (calendar->month == 0)
    {
      calendar->month = 11;
      calendar->year--;
    } 
  else 
    calendar->month--;
  
  month_len = month_length[leap (calendar->year)][calendar->month + 1];
  
  calendar_compute_days (calendar);
  
  g_signal_emit (calendar,
		 btk_calendar_signals[PREV_MONTH_SIGNAL],
		 0);
  g_signal_emit (calendar,
		 btk_calendar_signals[MONTH_CHANGED_SIGNAL],
		 0);
  
  if (month_len < calendar->selected_day)
    {
      calendar->selected_day = 0;
      btk_calendar_select_day (calendar, month_len);
    }
  else
    {
      if (calendar->selected_day < 0)
	calendar->selected_day = calendar->selected_day + 1 + month_length[leap (calendar->year)][calendar->month + 1];
      btk_calendar_select_day (calendar, calendar->selected_day);
    }

  calendar_queue_refresh (calendar);
}


/****************************************
 *           Basic object methods       *
 ****************************************/

static void
btk_calendar_finalize (BObject *object)
{
  B_OBJECT_CLASS (btk_calendar_parent_class)->finalize (object);
}

static void
btk_calendar_destroy (BtkObject *object)
{
  BtkCalendarPrivate *priv = BTK_CALENDAR_GET_PRIVATE (object);

  calendar_stop_spinning (BTK_CALENDAR (object));
  
  /* Call the destroy function for the extra display callback: */
  if (priv->detail_func_destroy && priv->detail_func_user_data)
    {
      priv->detail_func_destroy (priv->detail_func_user_data);
      priv->detail_func_user_data = NULL;
      priv->detail_func_destroy = NULL;
    }

  BTK_OBJECT_CLASS (btk_calendar_parent_class)->destroy (object);
}


static void
calendar_set_display_option (BtkCalendar              *calendar,
			     BtkCalendarDisplayOptions flag,
			     gboolean                  setting)
{
  BtkCalendarDisplayOptions flags;
  if (setting) 
    flags = calendar->display_flags | flag;
  else
    flags = calendar->display_flags & ~flag; 
  btk_calendar_set_display_options (calendar, flags);
}

static gboolean
calendar_get_display_option (BtkCalendar              *calendar,
			     BtkCalendarDisplayOptions flag)
{
  return (calendar->display_flags & flag) != 0;
}

static void 
btk_calendar_set_property (BObject      *object,
			   guint         prop_id,
			   const BValue *value,
			   BParamSpec   *pspec)
{
  BtkCalendar *calendar;

  calendar = BTK_CALENDAR (object);

  switch (prop_id) 
    {
    case PROP_YEAR:
      btk_calendar_select_month (calendar,
				 calendar->month,
				 b_value_get_int (value));
      break;
    case PROP_MONTH:
      btk_calendar_select_month (calendar,
				 b_value_get_int (value),
				 calendar->year);
      break;
    case PROP_DAY:
      btk_calendar_select_day (calendar,
			       b_value_get_int (value));
      break;
    case PROP_SHOW_HEADING:
      calendar_set_display_option (calendar,
				   BTK_CALENDAR_SHOW_HEADING,
				   b_value_get_boolean (value));
      break;
    case PROP_SHOW_DAY_NAMES:
      calendar_set_display_option (calendar,
				   BTK_CALENDAR_SHOW_DAY_NAMES,
				   b_value_get_boolean (value));
      break;
    case PROP_NO_MONTH_CHANGE:
      calendar_set_display_option (calendar,
				   BTK_CALENDAR_NO_MONTH_CHANGE,
				   b_value_get_boolean (value));
      break;
    case PROP_SHOW_WEEK_NUMBERS:
      calendar_set_display_option (calendar,
				   BTK_CALENDAR_SHOW_WEEK_NUMBERS,
				   b_value_get_boolean (value));
      break;
    case PROP_SHOW_DETAILS:
      calendar_set_display_option (calendar,
				   BTK_CALENDAR_SHOW_DETAILS,
				   b_value_get_boolean (value));
      break;
    case PROP_DETAIL_WIDTH_CHARS:
      btk_calendar_set_detail_width_chars (calendar,
                                           b_value_get_int (value));
      break;
    case PROP_DETAIL_HEIGHT_ROWS:
      btk_calendar_set_detail_height_rows (calendar,
                                           b_value_get_int (value));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void 
btk_calendar_get_property (BObject      *object,
			   guint         prop_id,
			   BValue       *value,
			   BParamSpec   *pspec)
{
  BtkCalendarPrivate *priv = BTK_CALENDAR_GET_PRIVATE (object);
  BtkCalendar *calendar = BTK_CALENDAR (object);

  switch (prop_id) 
    {
    case PROP_YEAR:
      b_value_set_int (value, calendar->year);
      break;
    case PROP_MONTH:
      b_value_set_int (value, calendar->month);
      break;
    case PROP_DAY:
      b_value_set_int (value, calendar->selected_day);
      break;
    case PROP_SHOW_HEADING:
      b_value_set_boolean (value, calendar_get_display_option (calendar,
							       BTK_CALENDAR_SHOW_HEADING));
      break;
    case PROP_SHOW_DAY_NAMES:
      b_value_set_boolean (value, calendar_get_display_option (calendar,
							       BTK_CALENDAR_SHOW_DAY_NAMES));
      break;
    case PROP_NO_MONTH_CHANGE:
      b_value_set_boolean (value, calendar_get_display_option (calendar,
							       BTK_CALENDAR_NO_MONTH_CHANGE));
      break;
    case PROP_SHOW_WEEK_NUMBERS:
      b_value_set_boolean (value, calendar_get_display_option (calendar,
							       BTK_CALENDAR_SHOW_WEEK_NUMBERS));
      break;
    case PROP_SHOW_DETAILS:
      b_value_set_boolean (value, calendar_get_display_option (calendar,
							       BTK_CALENDAR_SHOW_DETAILS));
      break;
    case PROP_DETAIL_WIDTH_CHARS:
      b_value_set_int (value, priv->detail_width_chars);
      break;
    case PROP_DETAIL_HEIGHT_ROWS:
      b_value_set_int (value, priv->detail_height_rows);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


/****************************************
 *             Realization              *
 ****************************************/

static void
calendar_realize_arrows (BtkCalendar *calendar)
{
  BtkWidget *widget = BTK_WIDGET (calendar);
  BtkCalendarPrivate *priv = BTK_CALENDAR_GET_PRIVATE (calendar);
  BdkWindowAttr attributes;
  gint attributes_mask;
  gint i;
  
  /* Arrow windows ------------------------------------- */
  if (! (calendar->display_flags & BTK_CALENDAR_NO_MONTH_CHANGE)
      && (calendar->display_flags & BTK_CALENDAR_SHOW_HEADING))
    {
      attributes.wclass = BDK_INPUT_OUTPUT;
      attributes.window_type = BDK_WINDOW_CHILD;
      attributes.visual = btk_widget_get_visual (widget);
      attributes.colormap = btk_widget_get_colormap (widget);
      attributes.event_mask = (btk_widget_get_events (widget) | BDK_EXPOSURE_MASK
			       | BDK_BUTTON_PRESS_MASK	| BDK_BUTTON_RELEASE_MASK
			       | BDK_ENTER_NOTIFY_MASK | BDK_LEAVE_NOTIFY_MASK);
      attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_VISUAL | BDK_WA_COLORMAP;
      for (i = 0; i < 4; i++)
	{
	  BdkRectangle rect;
	  calendar_arrow_rectangle (calendar, i, &rect);
	  
	  attributes.x = rect.x;
	  attributes.y = rect.y;
	  attributes.width = rect.width;
	  attributes.height = rect.height;
	  priv->arrow_win[i] = bdk_window_new (priv->header_win,
					       &attributes, 
					       attributes_mask);
	  if (btk_widget_is_sensitive (widget))
	    priv->arrow_state[i] = BTK_STATE_NORMAL;
	  else 
	    priv->arrow_state[i] = BTK_STATE_INSENSITIVE;
	  bdk_window_set_background (priv->arrow_win[i],
				     HEADER_BG_COLOR (BTK_WIDGET (calendar)));
	  bdk_window_show (priv->arrow_win[i]);
	  bdk_window_set_user_data (priv->arrow_win[i], widget);
	}
    }
  else
    {
      for (i = 0; i < 4; i++)
	priv->arrow_win[i] = NULL;
    }
}

static void
calendar_realize_header (BtkCalendar *calendar)
{
  BtkWidget *widget = BTK_WIDGET (calendar);
  BtkCalendarPrivate *priv = BTK_CALENDAR_GET_PRIVATE (calendar);
  BdkWindowAttr attributes;
  gint attributes_mask;
  
  /* Header window ------------------------------------- */
  if (calendar->display_flags & BTK_CALENDAR_SHOW_HEADING)
    {
      attributes.wclass = BDK_INPUT_OUTPUT;
      attributes.window_type = BDK_WINDOW_CHILD;
      attributes.visual = btk_widget_get_visual (widget);
      attributes.colormap = btk_widget_get_colormap (widget);
      attributes.event_mask = btk_widget_get_events (widget) | BDK_EXPOSURE_MASK;
      attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_VISUAL | BDK_WA_COLORMAP;
      attributes.x = widget->style->xthickness;
      attributes.y = widget->style->ythickness;
      attributes.width = widget->allocation.width - 2 * attributes.x;
      attributes.height = priv->header_h;
      priv->header_win = bdk_window_new (widget->window,
					 &attributes, attributes_mask);
      
      bdk_window_set_background (priv->header_win,
				 HEADER_BG_COLOR (BTK_WIDGET (calendar)));
      bdk_window_show (priv->header_win);
      bdk_window_set_user_data (priv->header_win, widget);
      
    }
  else
    {
      priv->header_win = NULL;
    }
  calendar_realize_arrows (calendar);
}

static gint
calendar_get_inner_border (BtkCalendar *calendar)
{
  gint inner_border;

  btk_widget_style_get (BTK_WIDGET (calendar),
                        "inner-border", &inner_border,
                        NULL);

  return inner_border;
}

static gint
calendar_get_xsep (BtkCalendar *calendar)
{
  gint xsep;

  btk_widget_style_get (BTK_WIDGET (calendar),
                        "horizontal-separation", &xsep,
                        NULL);

  return xsep;
}

static gint
calendar_get_ysep (BtkCalendar *calendar)
{
  gint ysep;

  btk_widget_style_get (BTK_WIDGET (calendar),
                        "vertical-separation", &ysep,
                        NULL);

  return ysep;
}

static void
calendar_realize_day_names (BtkCalendar *calendar)
{
  BtkWidget *widget = BTK_WIDGET (calendar);
  BtkCalendarPrivate *priv = BTK_CALENDAR_GET_PRIVATE (calendar);
  BdkWindowAttr attributes;
  gint attributes_mask;
  gint inner_border = calendar_get_inner_border (calendar);

  /* Day names	window --------------------------------- */
  if ( calendar->display_flags & BTK_CALENDAR_SHOW_DAY_NAMES)
    {
      attributes.wclass = BDK_INPUT_OUTPUT;
      attributes.window_type = BDK_WINDOW_CHILD;
      attributes.visual = btk_widget_get_visual (widget);
      attributes.colormap = btk_widget_get_colormap (widget);
      attributes.event_mask = btk_widget_get_events (widget) | BDK_EXPOSURE_MASK;
      attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_VISUAL | BDK_WA_COLORMAP;
      attributes.x = (widget->style->xthickness + inner_border);
      attributes.y = priv->header_h + (widget->style->ythickness 
					   + inner_border);
      attributes.width = (widget->allocation.width 
			  - (widget->style->xthickness + inner_border) 
			  * 2);
      attributes.height = priv->day_name_h;
      priv->day_name_win = bdk_window_new (widget->window,
					   &attributes, 
					   attributes_mask);
      bdk_window_set_background (priv->day_name_win, 
				 BACKGROUND_COLOR ( BTK_WIDGET ( calendar)));
      bdk_window_show (priv->day_name_win);
      bdk_window_set_user_data (priv->day_name_win, widget);
    }
  else
    {
      priv->day_name_win = NULL;
    }
}

static void
calendar_realize_week_numbers (BtkCalendar *calendar)
{
  BtkWidget *widget = BTK_WIDGET (calendar);
  BtkCalendarPrivate *priv = BTK_CALENDAR_GET_PRIVATE (calendar);
  BdkWindowAttr attributes;
  gint attributes_mask;
  gint inner_border = calendar_get_inner_border (calendar);

  /* Week number window -------------------------------- */
  if (calendar->display_flags & BTK_CALENDAR_SHOW_WEEK_NUMBERS)
    {
      attributes.wclass = BDK_INPUT_OUTPUT;
      attributes.window_type = BDK_WINDOW_CHILD;
      attributes.visual = btk_widget_get_visual (widget);
      attributes.colormap = btk_widget_get_colormap (widget);
      attributes.event_mask = btk_widget_get_events (widget) | BDK_EXPOSURE_MASK;
      
      attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_VISUAL | BDK_WA_COLORMAP;
      if (btk_widget_get_direction (widget) == BTK_TEXT_DIR_LTR) 
	attributes.x = widget->style->xthickness + inner_border;
      else 
	attributes.x = widget->allocation.width - priv->week_width - (widget->style->xthickness + inner_border);
      attributes.y = (priv->header_h + priv->day_name_h 
		      + (widget->style->ythickness + inner_border));
      attributes.width = priv->week_width;
      attributes.height = priv->main_h;
      priv->week_win = bdk_window_new (widget->window,
				       &attributes, attributes_mask);
      bdk_window_set_background (priv->week_win,  
				 BACKGROUND_COLOR (BTK_WIDGET (calendar)));
      bdk_window_show (priv->week_win);
      bdk_window_set_user_data (priv->week_win, widget);
    } 
  else
    {
      priv->week_win = NULL;
    }
}

static void
btk_calendar_realize (BtkWidget *widget)
{
  BtkCalendar *calendar = BTK_CALENDAR (widget);
  BtkCalendarPrivate *priv = BTK_CALENDAR_GET_PRIVATE (widget);
  BdkWindowAttr attributes;
  gint attributes_mask;
  gint inner_border = calendar_get_inner_border (calendar);

  btk_widget_set_realized (widget, TRUE);
  
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = BDK_INPUT_OUTPUT;
  attributes.window_type = BDK_WINDOW_CHILD;
  attributes.event_mask =  (btk_widget_get_events (widget) 
			    | BDK_EXPOSURE_MASK |BDK_KEY_PRESS_MASK | BDK_SCROLL_MASK);
  attributes.visual = btk_widget_get_visual (widget);
  attributes.colormap = btk_widget_get_colormap (widget);
  
  attributes_mask = BDK_WA_X | BDK_WA_Y | BDK_WA_VISUAL | BDK_WA_COLORMAP;
  widget->window = bdk_window_new (widget->parent->window,
				   &attributes, attributes_mask);
  
  widget->style = btk_style_attach (widget->style, widget->window);
  
  /* Header window ------------------------------------- */
  calendar_realize_header (calendar);
  /* Day names	window --------------------------------- */
  calendar_realize_day_names (calendar);
  /* Week number window -------------------------------- */
  calendar_realize_week_numbers (calendar);
  /* Main Window --------------------------------------	 */
  attributes.event_mask =  (btk_widget_get_events (widget) | BDK_EXPOSURE_MASK
			    | BDK_BUTTON_PRESS_MASK | BDK_BUTTON_RELEASE_MASK
			    | BDK_POINTER_MOTION_MASK | BDK_LEAVE_NOTIFY_MASK);
  
  if (btk_widget_get_direction (widget) == BTK_TEXT_DIR_LTR) 
    attributes.x = priv->week_width + (widget->style->ythickness + inner_border);
  else
    attributes.x = widget->style->ythickness + inner_border;

  attributes.y = (priv->header_h + priv->day_name_h 
		  + (widget->style->ythickness + inner_border));
  attributes.width = (widget->allocation.width - attributes.x 
		      - (widget->style->xthickness + inner_border));
  if (btk_widget_get_direction (widget) == BTK_TEXT_DIR_RTL)
    attributes.width -= priv->week_width;

  attributes.height = priv->main_h;
  priv->main_win = bdk_window_new (widget->window,
				   &attributes, attributes_mask);
  bdk_window_set_background (priv->main_win, 
			     BACKGROUND_COLOR ( BTK_WIDGET ( calendar)));
  bdk_window_show (priv->main_win);
  bdk_window_set_user_data (priv->main_win, widget);
  bdk_window_set_background (widget->window, BACKGROUND_COLOR (widget));
  bdk_window_show (widget->window);
  bdk_window_set_user_data (widget->window, widget);
}

static void
btk_calendar_unrealize (BtkWidget *widget)
{
  BtkCalendarPrivate *priv = BTK_CALENDAR_GET_PRIVATE (widget);
  gint i;
  
  if (priv->header_win)
    {
      for (i = 0; i < 4; i++)
	{
	  if (priv->arrow_win[i])
	    {
	      bdk_window_set_user_data (priv->arrow_win[i], NULL);
	      bdk_window_destroy (priv->arrow_win[i]);
	      priv->arrow_win[i] = NULL;
	    }
	}
      bdk_window_set_user_data (priv->header_win, NULL);
      bdk_window_destroy (priv->header_win);
      priv->header_win = NULL;
    }
  
  if (priv->week_win)
    {
      bdk_window_set_user_data (priv->week_win, NULL);
      bdk_window_destroy (priv->week_win);
      priv->week_win = NULL;      
    }
  
  if (priv->main_win)
    {
      bdk_window_set_user_data (priv->main_win, NULL);
      bdk_window_destroy (priv->main_win);
      priv->main_win = NULL;      
    }
  if (priv->day_name_win)
    {
      bdk_window_set_user_data (priv->day_name_win, NULL);
      bdk_window_destroy (priv->day_name_win);
      priv->day_name_win = NULL;      
    }

  BTK_WIDGET_CLASS (btk_calendar_parent_class)->unrealize (widget);
}

static gchar*
btk_calendar_get_detail (BtkCalendar *calendar,
                         gint         row,
                         gint         column)
{
  BtkCalendarPrivate *priv = BTK_CALENDAR_GET_PRIVATE (calendar);
  gint year, month;

  if (priv->detail_func == NULL)
    return NULL;

  year = calendar->year;
  month = calendar->month + calendar->day_month[row][column] - MONTH_CURRENT;

  if (month < 0)
    {
      month += 12;
      year -= 1;
    }
  else if (month > 11)
    {
      month -= 12;
      year += 1;
    }

  return priv->detail_func (calendar,
                            year, month,
                            calendar->day[row][column],
                            priv->detail_func_user_data);
}

static gboolean
btk_calendar_query_tooltip (BtkWidget  *widget,
                            gint        x,
                            gint        y,
                            gboolean    keyboard_mode,
                            BtkTooltip *tooltip)
{
  BtkCalendarPrivate *priv = BTK_CALENDAR_GET_PRIVATE (widget);
  BtkCalendar *calendar = BTK_CALENDAR (widget);
  gchar *detail = NULL;
  BdkRectangle day_rect;

  if (priv->main_win)
    {
      gint x0, y0, row, col;

      bdk_window_get_position (priv->main_win, &x0, &y0);
      col = calendar_column_from_x (calendar, x - x0);
      row = calendar_row_from_y (calendar, y - y0);

      if (col != -1 && row != -1 &&
          (0 != (priv->detail_overflow[row] & (1 << col)) ||
           0 == (calendar->display_flags & BTK_CALENDAR_SHOW_DETAILS)))
        {
          detail = btk_calendar_get_detail (calendar, row, col);
          calendar_day_rectangle (calendar, row, col, &day_rect);

          day_rect.x += x0;
          day_rect.y += y0;
        }
    }

  if (detail)
    {
      btk_tooltip_set_tip_area (tooltip, &day_rect);
      btk_tooltip_set_markup (tooltip, detail);

      g_free (detail);

      return TRUE;
    }

  if (BTK_WIDGET_CLASS (btk_calendar_parent_class)->query_tooltip)
    return BTK_WIDGET_CLASS (btk_calendar_parent_class)->query_tooltip (widget, x, y, keyboard_mode, tooltip);

  return FALSE;
}


/****************************************
 *       Size Request and Allocate      *
 ****************************************/

static void
btk_calendar_size_request (BtkWidget	  *widget,
			   BtkRequisition *requisition)
{
  BtkCalendar *calendar = BTK_CALENDAR (widget);
  BtkCalendarPrivate *priv = BTK_CALENDAR_GET_PRIVATE (widget);
  BangoLayout *layout;
  BangoRectangle logical_rect;

  gint height;
  gint i, r, c;
  gint calendar_margin = CALENDAR_MARGIN;
  gint header_width, main_width;
  gint max_header_height = 0;
  gint focus_width;
  gint focus_padding;
  gint max_detail_height;
  gint inner_border = calendar_get_inner_border (calendar);
  gint calendar_ysep = calendar_get_ysep (calendar);
  gint calendar_xsep = calendar_get_xsep (calendar);

  btk_widget_style_get (BTK_WIDGET (widget),
			"focus-line-width", &focus_width,
			"focus-padding", &focus_padding,
			NULL);

  layout = btk_widget_create_bango_layout (widget, NULL);
  
  /*
   * Calculate the requisition	width for the widget.
   */
  
  /* Header width */
  
  if (calendar->display_flags & BTK_CALENDAR_SHOW_HEADING)
    {
      priv->max_month_width = 0;
      for (i = 0; i < 12; i++)
	{
	  bango_layout_set_text (layout, default_monthname[i], -1);
	  bango_layout_get_pixel_extents (layout, NULL, &logical_rect);
	  priv->max_month_width = MAX (priv->max_month_width,
					       logical_rect.width + 8);
	  max_header_height = MAX (max_header_height, logical_rect.height); 
	}

      priv->max_year_width = 0;
      /* Translators:  This is a text measurement template.
       * Translate it to the widest year text
       *
       * If you don't understand this, leave it as "2000"
       */
      bango_layout_set_text (layout, C_("year measurement template", "2000"), -1);	  
      bango_layout_get_pixel_extents (layout, NULL, &logical_rect);
      priv->max_year_width = MAX (priv->max_year_width,
				  logical_rect.width + 8);
      max_header_height = MAX (max_header_height, logical_rect.height); 
    } 
  else 
    {
      priv->max_month_width = 0;
      priv->max_year_width = 0;
    }
  
  if (calendar->display_flags & BTK_CALENDAR_NO_MONTH_CHANGE)
    header_width = (priv->max_month_width 
		    + priv->max_year_width
		    + 3 * 3);
  else
    header_width = (priv->max_month_width 
		    + priv->max_year_width
		    + 4 * priv->arrow_width + 3 * 3);

  /* Mainwindow labels width */
  
  priv->max_day_char_width = 0;
  priv->max_day_char_ascent = 0;
  priv->max_day_char_descent = 0;
  priv->min_day_width = 0;

  for (i = 0; i < 9; i++)
    {
      gchar buffer[32];
      g_snprintf (buffer, sizeof (buffer), C_("calendar:day:digits", "%d"), i * 11);
      bango_layout_set_text (layout, buffer, -1);	  
      bango_layout_get_pixel_extents (layout, NULL, &logical_rect);
      priv->min_day_width = MAX (priv->min_day_width,
					 logical_rect.width);

      priv->max_day_char_ascent = MAX (priv->max_day_char_ascent,
					       BANGO_ASCENT (logical_rect));
      priv->max_day_char_descent = MAX (priv->max_day_char_descent, 
						BANGO_DESCENT (logical_rect));
    }
  
  priv->max_label_char_ascent = 0;
  priv->max_label_char_descent = 0;
  if (calendar->display_flags & BTK_CALENDAR_SHOW_DAY_NAMES)
    for (i = 0; i < 7; i++)
      {
	bango_layout_set_text (layout, default_abbreviated_dayname[i], -1);
	bango_layout_line_get_pixel_extents (bango_layout_get_lines_readonly (layout)->data, NULL, &logical_rect);

	priv->min_day_width = MAX (priv->min_day_width, logical_rect.width);
	priv->max_label_char_ascent = MAX (priv->max_label_char_ascent,
						   BANGO_ASCENT (logical_rect));
	priv->max_label_char_descent = MAX (priv->max_label_char_descent, 
						    BANGO_DESCENT (logical_rect));
      }
  
  priv->max_week_char_width = 0;
  if (calendar->display_flags & BTK_CALENDAR_SHOW_WEEK_NUMBERS)
    for (i = 0; i < 9; i++)
      {
	gchar buffer[32];
	g_snprintf (buffer, sizeof (buffer), C_("calendar:week:digits", "%d"), i * 11);
	bango_layout_set_text (layout, buffer, -1);	  
	bango_layout_get_pixel_extents (layout, NULL, &logical_rect);
	priv->max_week_char_width = MAX (priv->max_week_char_width,
					   logical_rect.width / 2);
      }
  
  /* Calculate detail extents. Do this as late as possible since
   * bango_layout_set_markup is called which alters font settings. */
  max_detail_height = 0;

  if (priv->detail_func && (calendar->display_flags & BTK_CALENDAR_SHOW_DETAILS))
    {
      gchar *markup, *tail;

      if (priv->detail_width_chars || priv->detail_height_rows)
        {
          gint rows = MAX (1, priv->detail_height_rows) - 1;
          gsize len = priv->detail_width_chars + rows + 16;

          markup = tail = g_alloca (len);

          memcpy (tail,     "<small>", 7);
          tail += 7;

          memset (tail, 'm', priv->detail_width_chars);
          tail += priv->detail_width_chars;

          memset (tail, '\n', rows);
          tail += rows;

          memcpy (tail,     "</small>", 9);
          tail += 9;

          g_assert (len == (tail - markup));

          bango_layout_set_markup (layout, markup, -1);
          bango_layout_get_pixel_extents (layout, NULL, &logical_rect);

          if (priv->detail_width_chars)
            priv->min_day_width = MAX (priv->min_day_width, logical_rect.width);
          if (priv->detail_height_rows)
            max_detail_height = MAX (max_detail_height, logical_rect.height);
        }

      if (!priv->detail_width_chars || !priv->detail_height_rows)
        for (r = 0; r < 6; r++)
          for (c = 0; c < 7; c++)
            {
              gchar *detail = btk_calendar_get_detail (calendar, r, c);

              if (detail)
                {
                  markup = g_strconcat ("<small>", detail, "</small>", NULL);
                  bango_layout_set_markup (layout, markup, -1);

                  if (priv->detail_width_chars)
                    {
                      bango_layout_set_wrap (layout, BANGO_WRAP_WORD_CHAR);
                      bango_layout_set_width (layout, BANGO_SCALE * priv->min_day_width);
                    }

                  bango_layout_get_pixel_extents (layout, NULL, &logical_rect);

                  if (!priv->detail_width_chars)
                    priv->min_day_width = MAX (priv->min_day_width, logical_rect.width);
                  if (!priv->detail_height_rows)
                    max_detail_height = MAX (max_detail_height, logical_rect.height);

                  g_free (markup);
                  g_free (detail);
                }
            }
    }

  /* We add one to max_day_char_width to be able to make the marked day "bold" */
  priv->max_day_char_width = priv->min_day_width / 2 + 1;

  main_width = (7 * (priv->min_day_width + (focus_padding + focus_width) * 2) + (DAY_XSEP * 6) + CALENDAR_MARGIN * 2
		+ (priv->max_week_char_width
		   ? priv->max_week_char_width * 2 + (focus_padding + focus_width) * 2 + calendar_xsep * 2
		   : 0));
  
  
  requisition->width = MAX (header_width, main_width + inner_border * 2) + widget->style->xthickness * 2;
  
  /*
   * Calculate the requisition height for the widget.
   */
  
  if (calendar->display_flags & BTK_CALENDAR_SHOW_HEADING)
    {
      priv->header_h = (max_header_height + calendar_ysep * 2);
    }
  else
    {
      priv->header_h = 0;
    }
  
  if (calendar->display_flags & BTK_CALENDAR_SHOW_DAY_NAMES)
    {
      priv->day_name_h = (priv->max_label_char_ascent
				  + priv->max_label_char_descent
				  + 2 * (focus_padding + focus_width) + calendar_margin);
      calendar_margin = calendar_ysep;
    } 
  else
    {
      priv->day_name_h = 0;
    }

  priv->main_h = (CALENDAR_MARGIN + calendar_margin
			  + 6 * (priv->max_day_char_ascent
				 + priv->max_day_char_descent 
                                 + max_detail_height
				 + 2 * (focus_padding + focus_width))
			  + DAY_YSEP * 5);
  
  height = (priv->header_h + priv->day_name_h 
	    + priv->main_h);
  
  requisition->height = height + (widget->style->ythickness + inner_border) * 2;

  g_object_unref (layout);
}

static void
btk_calendar_size_allocate (BtkWidget	  *widget,
			    BtkAllocation *allocation)
{
  BtkCalendar *calendar = BTK_CALENDAR (widget);
  BtkCalendarPrivate *priv = BTK_CALENDAR_GET_PRIVATE (widget);
  gint xthickness = widget->style->xthickness;
  gint ythickness = widget->style->xthickness;
  guint i;
  gint inner_border = calendar_get_inner_border (calendar);
  gint calendar_xsep = calendar_get_xsep (calendar);

  widget->allocation = *allocation;
    
  if (calendar->display_flags & BTK_CALENDAR_SHOW_WEEK_NUMBERS)
    {
      priv->day_width = (priv->min_day_width
			 * ((allocation->width - (xthickness + inner_border) * 2
			     - (CALENDAR_MARGIN * 2) -  (DAY_XSEP * 6) - calendar_xsep * 2))
			 / (7 * priv->min_day_width + priv->max_week_char_width * 2));
      priv->week_width = ((allocation->width - (xthickness + inner_border) * 2
			   - (CALENDAR_MARGIN * 2) - (DAY_XSEP * 6) - calendar_xsep * 2 )
			  - priv->day_width * 7 + CALENDAR_MARGIN + calendar_xsep);
    } 
  else 
    {
      priv->day_width = (allocation->width
			 - (xthickness + inner_border) * 2
			 - (CALENDAR_MARGIN * 2)
			 - (DAY_XSEP * 6))/7;
      priv->week_width = 0;
    }
  
  if (btk_widget_get_realized (widget))
    {
      bdk_window_move_resize (widget->window,
			      allocation->x, allocation->y,
			      allocation->width, allocation->height);
      if (priv->header_win)
	bdk_window_move_resize (priv->header_win,
				xthickness, ythickness,
				allocation->width - 2 * xthickness, priv->header_h);

      for (i = 0 ; i < 4 ; i++)
	{
	  if (priv->arrow_win[i])
	    {
	      BdkRectangle rect;
	      calendar_arrow_rectangle (calendar, i, &rect);
	  
	      bdk_window_move_resize (priv->arrow_win[i],
				      rect.x, rect.y, rect.width, rect.height);
	    }
	}
      
      if (priv->day_name_win)
	bdk_window_move_resize (priv->day_name_win,
				xthickness + inner_border,
				priv->header_h + (widget->style->ythickness + inner_border),
				allocation->width - (xthickness + inner_border) * 2,
				priv->day_name_h);
      if (btk_widget_get_direction (widget) == BTK_TEXT_DIR_LTR) 
	{
	  if (priv->week_win)
	    bdk_window_move_resize (priv->week_win,
				    (xthickness + inner_border),
				    priv->header_h + priv->day_name_h
				    + (widget->style->ythickness + inner_border),
				    priv->week_width,
				    priv->main_h);
	  bdk_window_move_resize (priv->main_win,
				  priv->week_width + (xthickness + inner_border),
				  priv->header_h + priv->day_name_h
				  + (widget->style->ythickness + inner_border),
				  allocation->width 
				  - priv->week_width 
				  - (xthickness + inner_border) * 2,
				  priv->main_h);
	}
      else 
	{
	  bdk_window_move_resize (priv->main_win,
				  (xthickness + inner_border),
				  priv->header_h + priv->day_name_h
				  + (widget->style->ythickness + inner_border),
				  allocation->width 
				  - priv->week_width 
				  - (xthickness + inner_border) * 2,
				  priv->main_h);
	  if (priv->week_win)
	    bdk_window_move_resize (priv->week_win,
				    allocation->width 
				    - priv->week_width 
				    - (xthickness + inner_border),
				    priv->header_h + priv->day_name_h
				    + (widget->style->ythickness + inner_border),
				    priv->week_width,
				    priv->main_h);
	}
    }
}


/****************************************
 *              Repainting              *
 ****************************************/

static void
calendar_paint_header (BtkCalendar *calendar)
{
  BtkWidget *widget = BTK_WIDGET (calendar);
  BtkCalendarPrivate *priv = BTK_CALENDAR_GET_PRIVATE (calendar);
  bairo_t *cr;
  char buffer[255];
  int x, y;
  gint header_width;
  gint max_month_width;
  gint max_year_width;
  BangoLayout *layout;
  BangoRectangle logical_rect;
  gboolean year_left;
  time_t tmp_time;
  struct tm *tm;
  gchar *str;

  if (btk_widget_get_direction (widget) == BTK_TEXT_DIR_LTR) 
    year_left = priv->year_before;
  else
    year_left = !priv->year_before;

  cr = bdk_bairo_create (priv->header_win);
  
  header_width = widget->allocation.width - 2 * widget->style->xthickness;
  
  max_month_width = priv->max_month_width;
  max_year_width = priv->max_year_width;
  
  btk_paint_shadow (widget->style, priv->header_win,
		    BTK_STATE_NORMAL, BTK_SHADOW_OUT,
		    NULL, widget, "calendar",
		    0, 0, header_width, priv->header_h);

  tmp_time = 1;  /* Jan 1 1970, 00:00:01 UTC */
  tm = gmtime (&tmp_time);
  tm->tm_year = calendar->year - 1900;

  /* Translators: This dictates how the year is displayed in
   * btkcalendar widget.  See strftime() manual for the format.
   * Use only ASCII in the translation.
   *
   * Also look for the msgid "2000".
   * Translate that entry to a year with the widest output of this
   * msgid.
   *
   * "%Y" is appropriate for most locales.
   */
  strftime (buffer, sizeof (buffer), C_("calendar year format", "%Y"), tm);
  str = g_locale_to_utf8 (buffer, -1, NULL, NULL, NULL);
  layout = btk_widget_create_bango_layout (widget, str);
  g_free (str);
  
  bango_layout_get_pixel_extents (layout, NULL, &logical_rect);
  
  /* Draw title */
  y = (priv->header_h - logical_rect.height) / 2;
  
  /* Draw year and its arrows */
  
  if (calendar->display_flags & BTK_CALENDAR_NO_MONTH_CHANGE)
    if (year_left)
      x = 3 + (max_year_width - logical_rect.width)/2;
    else
      x = header_width - (3 + max_year_width
			  - (max_year_width - logical_rect.width)/2);
  else
    if (year_left)
      x = 3 + priv->arrow_width + (max_year_width - logical_rect.width)/2;
    else
      x = header_width - (3 + priv->arrow_width + max_year_width
			  - (max_year_width - logical_rect.width)/2);
  

  bdk_bairo_set_source_color (cr, HEADER_FG_COLOR (BTK_WIDGET (calendar)));
  bairo_move_to (cr, x, y);
  bango_bairo_show_layout (cr, layout);
  
  /* Draw month */
  g_snprintf (buffer, sizeof (buffer), "%s", default_monthname[calendar->month]);
  bango_layout_set_text (layout, buffer, -1);
  bango_layout_get_pixel_extents (layout, NULL, &logical_rect);

  if (calendar->display_flags & BTK_CALENDAR_NO_MONTH_CHANGE)
    if (year_left)
      x = header_width - (3 + max_month_width
			  - (max_month_width - logical_rect.width)/2);      
    else
    x = 3 + (max_month_width - logical_rect.width) / 2;
  else
    if (year_left)
      x = header_width - (3 + priv->arrow_width + max_month_width
			  - (max_month_width - logical_rect.width)/2);
    else
    x = 3 + priv->arrow_width + (max_month_width - logical_rect.width)/2;

  bairo_move_to (cr, x, y);
  bango_bairo_show_layout (cr, layout);

  g_object_unref (layout);
  bairo_destroy (cr);
}

static void
calendar_paint_day_names (BtkCalendar *calendar)
{
  BtkWidget *widget = BTK_WIDGET (calendar);
  BtkCalendarPrivate *priv = BTK_CALENDAR_GET_PRIVATE (calendar);
  bairo_t *cr;
  char buffer[255];
  int day,i;
  int day_width, cal_width;
  int day_wid_sep;
  BangoLayout *layout;
  BangoRectangle logical_rect;
  gint focus_padding;
  gint focus_width;
  gint calendar_ysep = calendar_get_ysep (calendar);
  gint calendar_xsep = calendar_get_xsep (calendar);

  cr = bdk_bairo_create (priv->day_name_win);
  
  btk_widget_style_get (BTK_WIDGET (widget),
			"focus-line-width", &focus_width,
			"focus-padding", &focus_padding,
			NULL);
  
  day_width = priv->day_width;
  cal_width = widget->allocation.width;
  day_wid_sep = day_width + DAY_XSEP;
  
  /*
   * Draw rectangles as inverted background for the labels.
   */

  bdk_bairo_set_source_color (cr, SELECTED_BG_COLOR (widget));
  bairo_rectangle (cr,
		   CALENDAR_MARGIN, CALENDAR_MARGIN,
		   cal_width-CALENDAR_MARGIN * 2,
		   priv->day_name_h - CALENDAR_MARGIN);
  bairo_fill (cr);
  
  if (calendar->display_flags & BTK_CALENDAR_SHOW_WEEK_NUMBERS)
    {
      bairo_rectangle (cr, 
		       CALENDAR_MARGIN,
		       priv->day_name_h - calendar_ysep,
		       priv->week_width - calendar_ysep - CALENDAR_MARGIN,
		       calendar_ysep);
      bairo_fill (cr);
    }
  
  /*
   * Write the labels
   */

  layout = btk_widget_create_bango_layout (widget, NULL);

  bdk_bairo_set_source_color (cr, SELECTED_FG_COLOR (widget));
  for (i = 0; i < 7; i++)
    {
      if (btk_widget_get_direction (BTK_WIDGET (calendar)) == BTK_TEXT_DIR_RTL)
	day = 6 - i;
      else
	day = i;
      day = (day + priv->week_start) % 7;
      g_snprintf (buffer, sizeof (buffer), "%s", default_abbreviated_dayname[day]);

      bango_layout_set_text (layout, buffer, -1);
      bango_layout_get_pixel_extents (layout, NULL, &logical_rect);

      bairo_move_to (cr, 
		     (CALENDAR_MARGIN +
		      + (btk_widget_get_direction (widget) == BTK_TEXT_DIR_LTR ?
			 (priv->week_width + (priv->week_width ? calendar_xsep : 0))
			 : 0)
		      + day_wid_sep * i
		      + (day_width - logical_rect.width)/2),
		     CALENDAR_MARGIN + focus_width + focus_padding + logical_rect.y);
      bango_bairo_show_layout (cr, layout);
    }
  
  g_object_unref (layout);
  bairo_destroy (cr);
}

static void
calendar_paint_week_numbers (BtkCalendar *calendar)
{
  BtkWidget *widget = BTK_WIDGET (calendar);
  BtkCalendarPrivate *priv = BTK_CALENDAR_GET_PRIVATE (calendar);
  bairo_t *cr;

  guint week = 0, year;
  gint row, x_loc, y_loc;
  gint day_height;
  char buffer[32];
  BangoLayout *layout;
  BangoRectangle logical_rect;
  gint focus_padding;
  gint focus_width;
  gint calendar_xsep = calendar_get_xsep (calendar);

  cr = bdk_bairo_create (priv->week_win);
  
  btk_widget_style_get (BTK_WIDGET (widget),
			"focus-line-width", &focus_width,
			"focus-padding", &focus_padding,
			NULL);
  
  /*
   * Draw a rectangle as inverted background for the labels.
   */

  bdk_bairo_set_source_color (cr, SELECTED_BG_COLOR (widget));
  if (priv->day_name_win)
    bairo_rectangle (cr, 
		     CALENDAR_MARGIN,
		     0,
		     priv->week_width - CALENDAR_MARGIN,
		     priv->main_h - CALENDAR_MARGIN);
  else
    bairo_rectangle (cr,
		     CALENDAR_MARGIN,
		     CALENDAR_MARGIN,
		     priv->week_width - CALENDAR_MARGIN,
		     priv->main_h - 2 * CALENDAR_MARGIN);
  bairo_fill (cr);
  
  /*
   * Write the labels
   */
  
  layout = btk_widget_create_bango_layout (widget, NULL);
  
  bdk_bairo_set_source_color (cr, SELECTED_FG_COLOR (widget));
  day_height = calendar_row_height (calendar);
  for (row = 0; row < 6; row++)
    {
      gboolean result;
      
      year = calendar->year;
      if (calendar->day[row][6] < 15 && row > 3 && calendar->month == 11)
	year++;

      result = week_of_year (&week, &year,		
			     ((calendar->day[row][6] < 15 && row > 3 ? 1 : 0)
			      + calendar->month) % 12 + 1, calendar->day[row][6]);
      g_return_if_fail (result);

      /* Translators: this defines whether the week numbers should use
       * localized digits or the ones used in English (0123...).
       *
       * Translate to "%Id" if you want to use localized digits, or
       * translate to "%d" otherwise.
       *
       * Note that translating this doesn't guarantee that you get localized
       * digits. That needs support from your system and locale definition
       * too.
       */
      g_snprintf (buffer, sizeof (buffer), C_("calendar:week:digits", "%d"), week);
      bango_layout_set_text (layout, buffer, -1);
      bango_layout_get_pixel_extents (layout, NULL, &logical_rect);

      y_loc = calendar_top_y_for_row (calendar, row) + (day_height - logical_rect.height) / 2;

      x_loc = (priv->week_width
	       - logical_rect.width
	       - calendar_xsep - focus_padding - focus_width);

      bairo_move_to (cr, x_loc, y_loc);
      bango_bairo_show_layout (cr, layout);
    }
  
  g_object_unref (layout);
  bairo_destroy (cr);
}

static void
calendar_invalidate_day_num (BtkCalendar *calendar,
			     gint         day)
{
  gint r, c, row, col;
  
  row = -1;
  col = -1;
  for (r = 0; r < 6; r++)
    for (c = 0; c < 7; c++)
      if (calendar->day_month[r][c] == MONTH_CURRENT &&
	  calendar->day[r][c] == day)
	{
	  row = r;
	  col = c;
	}
  
  g_return_if_fail (row != -1);
  g_return_if_fail (col != -1);
  
  calendar_invalidate_day (calendar, row, col);
}

static void
calendar_invalidate_day (BtkCalendar *calendar,
			 gint         row,
			 gint         col)
{
  BtkCalendarPrivate *priv = BTK_CALENDAR_GET_PRIVATE (calendar);

  if (priv->main_win)
    {
      BdkRectangle day_rect;
      
      calendar_day_rectangle (calendar, row, col, &day_rect);
      bdk_window_invalidate_rect (priv->main_win, &day_rect, FALSE);
    }
}

static gboolean
is_color_attribute (BangoAttribute *attribute,
                    gpointer        data)
{
  return (attribute->klass->type == BANGO_ATTR_FOREGROUND ||
          attribute->klass->type == BANGO_ATTR_BACKGROUND);
}

static void
calendar_paint_day (BtkCalendar *calendar,
		    gint	     row,
		    gint	     col)
{
  BtkWidget *widget = BTK_WIDGET (calendar);
  BtkCalendarPrivate *priv = BTK_CALENDAR_GET_PRIVATE (calendar);
  bairo_t *cr;
  BdkColor *text_color;
  gchar *detail;
  gchar buffer[32];
  gint day;
  gint x_loc, y_loc;
  BdkRectangle day_rect;

  BangoLayout *layout;
  BangoRectangle logical_rect;
  gboolean overflow = FALSE;
  gboolean show_details;

  g_return_if_fail (row < 6);
  g_return_if_fail (col < 7);

  cr = bdk_bairo_create (priv->main_win);

  day = calendar->day[row][col];
  show_details = (calendar->display_flags & BTK_CALENDAR_SHOW_DETAILS);

  calendar_day_rectangle (calendar, row, col, &day_rect);
  
  if (calendar->day_month[row][col] == MONTH_PREV)
    {
      text_color = PREV_MONTH_COLOR (widget);
    } 
  else if (calendar->day_month[row][col] == MONTH_NEXT)
    {
      text_color =  NEXT_MONTH_COLOR (widget);
    } 
  else 
    {
#if 0      
      if (calendar->highlight_row == row && calendar->highlight_col == col)
	{
	  bairo_set_source_color (cr, HIGHLIGHT_BG_COLOR (widget));
	  bdk_bairo_rectangle (cr, &day_rect);
	  bairo_fill (cr);
	}
#endif     
      if (calendar->selected_day == day)
	{
	  bdk_bairo_set_source_color (cr, SELECTED_BG_COLOR (widget));
	  bdk_bairo_rectangle (cr, &day_rect);
	  bairo_fill (cr);
	}
      if (calendar->selected_day == day)
	text_color = SELECTED_FG_COLOR (widget);
      else if (calendar->marked_date[day-1])
	text_color = MARKED_COLOR (widget);
      else
	text_color = NORMAL_DAY_COLOR (widget);
    }

  /* Translators: this defines whether the day numbers should use
   * localized digits or the ones used in English (0123...).
   *
   * Translate to "%Id" if you want to use localized digits, or
   * translate to "%d" otherwise.
   *
   * Note that translating this doesn't guarantee that you get localized
   * digits. That needs support from your system and locale definition
   * too.
   */
  g_snprintf (buffer, sizeof (buffer), C_("calendar:day:digits", "%d"), day);

  /* Get extra information to show, if any: */

  detail = btk_calendar_get_detail (calendar, row, col);

  layout = btk_widget_create_bango_layout (widget, buffer);
  bango_layout_set_alignment (layout, BANGO_ALIGN_CENTER);
  bango_layout_get_pixel_extents (layout, NULL, &logical_rect);
  
  x_loc = day_rect.x + (day_rect.width - logical_rect.width) / 2;
  y_loc = day_rect.y;

  bdk_bairo_set_source_color (cr, text_color);
  bairo_move_to (cr, x_loc, y_loc);
  bango_bairo_show_layout (cr, layout);

  if (calendar->day_month[row][col] == MONTH_CURRENT &&
     (calendar->marked_date[day-1] || (detail && !show_details)))
    {
      bairo_move_to (cr, x_loc - 1, y_loc);
      bango_bairo_show_layout (cr, layout);
    }

  y_loc += priv->max_day_char_descent;

  if (priv->detail_func && show_details)
    {
      bairo_save (cr);

      if (calendar->selected_day == day)
        bdk_bairo_set_source_color (cr, &widget->style->text[BTK_STATE_ACTIVE]);
      else if (calendar->day_month[row][col] == MONTH_CURRENT)
        bdk_bairo_set_source_color (cr, &widget->style->base[BTK_STATE_ACTIVE]);
      else
        bdk_bairo_set_source_color (cr, &widget->style->base[BTK_STATE_INSENSITIVE]);

      bairo_set_line_width (cr, 1);
      bairo_move_to (cr, day_rect.x + 2, y_loc + 0.5);
      bairo_line_to (cr, day_rect.x + day_rect.width - 2, y_loc + 0.5);
      bairo_stroke (cr);

      bairo_restore (cr);

      y_loc += 2;
    }

  if (detail && show_details)
    {
      gchar *markup = g_strconcat ("<small>", detail, "</small>", NULL);
      bango_layout_set_markup (layout, markup, -1);
      g_free (markup);

      if (day == calendar->selected_day)
        {
          /* Stripping colors as they conflict with selection marking. */

          BangoAttrList *attrs = bango_layout_get_attributes (layout);
          BangoAttrList *colors = NULL;

          if (attrs)
            colors = bango_attr_list_filter (attrs, is_color_attribute, NULL);
          if (colors)
            bango_attr_list_unref (colors);
        }

      bango_layout_set_wrap (layout, BANGO_WRAP_WORD_CHAR);
      bango_layout_set_width (layout, BANGO_SCALE * day_rect.width);

      if (priv->detail_height_rows)
        {
          gint dy = day_rect.height - (y_loc - day_rect.y);
          bango_layout_set_height (layout, BANGO_SCALE * dy);
          bango_layout_set_ellipsize (layout, BANGO_ELLIPSIZE_END);
        }

      bairo_move_to (cr, day_rect.x, y_loc);
      bango_bairo_show_layout (cr, layout);
    }

  if (btk_widget_has_focus (widget)
      && calendar->focus_row == row && calendar->focus_col == col)
    {
      BtkStateType state;

      if (calendar->selected_day == day)
	state = btk_widget_has_focus (widget) ? BTK_STATE_SELECTED : BTK_STATE_ACTIVE;
      else
	state = BTK_STATE_NORMAL;
      
      btk_paint_focus (widget->style, 
		       priv->main_win,
	               state,
		       NULL, widget, "calendar-day",
		       day_rect.x,     day_rect.y, 
		       day_rect.width, day_rect.height);
    }

  if (overflow)
    priv->detail_overflow[row] |= (1 << col);
  else
    priv->detail_overflow[row] &= ~(1 << col);

  g_object_unref (layout);
  bairo_destroy (cr);
  g_free (detail);
}

static void
calendar_paint_main (BtkCalendar *calendar)
{
  gint row, col;
  
  for (col = 0; col < 7; col++)
    for (row = 0; row < 6; row++)
      calendar_paint_day (calendar, row, col);
}

static void
calendar_invalidate_arrow (BtkCalendar *calendar,
			   guint        arrow)
{
  BtkCalendarPrivate *priv = BTK_CALENDAR_GET_PRIVATE (calendar);
  BdkWindow *window;
  
  window = priv->arrow_win[arrow];
  if (window)
    bdk_window_invalidate_rect (window, NULL, FALSE);
}

static void
calendar_paint_arrow (BtkCalendar *calendar,
		      guint	       arrow)
{
  BtkWidget *widget = BTK_WIDGET (calendar);
  BtkCalendarPrivate *priv = BTK_CALENDAR_GET_PRIVATE (widget);
  BdkWindow *window;
  
  window = priv->arrow_win[arrow];
  if (window)
    {
      bairo_t *cr = bdk_bairo_create (window);
      gint width, height;
      gint state;
	
      state = priv->arrow_state[arrow];

      bdk_bairo_set_source_color (cr, &widget->style->bg[state]);
      bairo_paint (cr);
      bairo_destroy (cr);
      
      width = bdk_window_get_width (window);
      height = bdk_window_get_height (window);
      if (arrow == ARROW_MONTH_LEFT || arrow == ARROW_YEAR_LEFT)
	btk_paint_arrow (widget->style, window, state, 
			 BTK_SHADOW_OUT, NULL, widget, "calendar",
			 BTK_ARROW_LEFT, TRUE, 
			 width/2 - 3, height/2 - 4, 8, 8);
      else 
	btk_paint_arrow (widget->style, window, state, 
			 BTK_SHADOW_OUT, NULL, widget, "calendar",
			 BTK_ARROW_RIGHT, TRUE, 
			 width/2 - 4, height/2 - 4, 8, 8);
    }
}

static gboolean
btk_calendar_expose (BtkWidget	    *widget,
		     BdkEventExpose *event)
{
  BtkCalendar *calendar = BTK_CALENDAR (widget);
  BtkCalendarPrivate *priv = BTK_CALENDAR_GET_PRIVATE (widget);
  int i;

  if (btk_widget_is_drawable (widget))
    {
      if (event->window == priv->main_win)
	calendar_paint_main (calendar);
      
      if (event->window == priv->header_win)
	calendar_paint_header (calendar);

      for (i = 0; i < 4; i++)
	if (event->window == priv->arrow_win[i])
	  calendar_paint_arrow (calendar, i);
      
      if (event->window == priv->day_name_win)
	calendar_paint_day_names (calendar);
      
      if (event->window == priv->week_win)
	calendar_paint_week_numbers (calendar);
      if (event->window == widget->window)
	{
	  btk_paint_shadow (widget->style, widget->window, btk_widget_get_state (widget),
			    BTK_SHADOW_IN, NULL, widget, "calendar",
			    0, 0, widget->allocation.width, widget->allocation.height);
	}
    }
  
  return FALSE;
}


/****************************************
 *           Mouse handling             *
 ****************************************/

static void
calendar_arrow_action (BtkCalendar *calendar,
		       guint        arrow)
{
  switch (arrow)
    {
    case ARROW_YEAR_LEFT:
      calendar_set_year_prev (calendar);
      break;
    case ARROW_YEAR_RIGHT:
      calendar_set_year_next (calendar);
      break;
    case ARROW_MONTH_LEFT:
      calendar_set_month_prev (calendar);
      break;
    case ARROW_MONTH_RIGHT:
      calendar_set_month_next (calendar);
      break;
    default:;
      /* do nothing */
    }
}

static gboolean
calendar_timer (gpointer data)
{
  BtkCalendar *calendar = data;
  BtkCalendarPrivate *priv = BTK_CALENDAR_GET_PRIVATE (calendar);
  gboolean retval = FALSE;
  
  if (priv->timer)
    {
      calendar_arrow_action (calendar, priv->click_child);

      if (priv->need_timer)
	{
          BtkSettings *settings;
          guint        timeout;

          settings = btk_widget_get_settings (BTK_WIDGET (calendar));
          g_object_get (settings, "btk-timeout-repeat", &timeout, NULL);

	  priv->need_timer = FALSE;
	  priv->timer = bdk_threads_add_timeout_full (G_PRIORITY_DEFAULT_IDLE,
					    timeout * SCROLL_DELAY_FACTOR,
					    (GSourceFunc) calendar_timer,
					    (gpointer) calendar, NULL);
	}
      else 
	retval = TRUE;
    }

  return retval;
}

static void
calendar_start_spinning (BtkCalendar *calendar,
			 gint         click_child)
{
  BtkCalendarPrivate *priv = BTK_CALENDAR_GET_PRIVATE (calendar);

  priv->click_child = click_child;
  
  if (!priv->timer)
    {
      BtkSettings *settings;
      guint        timeout;

      settings = btk_widget_get_settings (BTK_WIDGET (calendar));
      g_object_get (settings, "btk-timeout-initial", &timeout, NULL);

      priv->need_timer = TRUE;
      priv->timer = bdk_threads_add_timeout_full (G_PRIORITY_DEFAULT_IDLE,
					timeout,
					(GSourceFunc) calendar_timer,
					(gpointer) calendar, NULL);
    }
}

static void
calendar_stop_spinning (BtkCalendar *calendar)
{
  BtkCalendarPrivate *priv = BTK_CALENDAR_GET_PRIVATE (calendar);

  if (priv->timer)
    {
      g_source_remove (priv->timer);
      priv->timer = 0;
      priv->need_timer = FALSE;
    }
}

static void
calendar_main_button_press (BtkCalendar	   *calendar,
			    BdkEventButton *event)
{
  BtkWidget *widget = BTK_WIDGET (calendar);
  BtkCalendarPrivate *priv = BTK_CALENDAR_GET_PRIVATE (calendar);
  gint x, y;
  gint row, col;
  gint day_month;
  gint day;
  
  x = (gint) (event->x);
  y = (gint) (event->y);
  
  row = calendar_row_from_y (calendar, y);
  col = calendar_column_from_x (calendar, x);

  /* If row or column isn't found, just return. */
  if (row == -1 || col == -1)
    return;
  
  day_month = calendar->day_month[row][col];

  if (event->type == BDK_BUTTON_PRESS)
    {
      day = calendar->day[row][col];
      
      if (day_month == MONTH_PREV)
	calendar_set_month_prev (calendar);
      else if (day_month == MONTH_NEXT)
	calendar_set_month_next (calendar);
      
      if (!btk_widget_has_focus (widget))
	btk_widget_grab_focus (widget);
	  
      if (event->button == 1) 
	{
	  priv->in_drag = 1;
	  priv->drag_start_x = x;
	  priv->drag_start_y = y;
	}

      calendar_select_and_focus_day (calendar, day);
    }
  else if (event->type == BDK_2BUTTON_PRESS)
    {
      priv->in_drag = 0;
      if (day_month == MONTH_CURRENT)
	g_signal_emit (calendar,
		       btk_calendar_signals[DAY_SELECTED_DOUBLE_CLICK_SIGNAL],
		       0);
    }
}

static gboolean
btk_calendar_button_press (BtkWidget	  *widget,
			   BdkEventButton *event)
{
  BtkCalendar *calendar = BTK_CALENDAR (widget);
  BtkCalendarPrivate *priv = BTK_CALENDAR_GET_PRIVATE (widget);
  gint arrow = -1;
  
  if (event->window == priv->main_win)
    calendar_main_button_press (calendar, event);

  if (!btk_widget_has_focus (widget))
    btk_widget_grab_focus (widget);

  for (arrow = ARROW_YEAR_LEFT; arrow <= ARROW_MONTH_RIGHT; arrow++)
    {
      if (event->window == priv->arrow_win[arrow])
	{
	  
	  /* only call the action on single click, not double */
	  if (event->type == BDK_BUTTON_PRESS)
	    {
	      if (event->button == 1)
		calendar_start_spinning (calendar, arrow);

	      calendar_arrow_action (calendar, arrow);	      
	    }

	  return TRUE;
	}
    }

  return FALSE;
}

static gboolean
btk_calendar_button_release (BtkWidget	  *widget,
			     BdkEventButton *event)
{
  BtkCalendar *calendar = BTK_CALENDAR (widget);
  BtkCalendarPrivate *priv = BTK_CALENDAR_GET_PRIVATE (widget);

  if (event->button == 1) 
    {
      calendar_stop_spinning (calendar);

      if (priv->in_drag)
	priv->in_drag = 0;
    }

  return TRUE;
}

static gboolean
btk_calendar_motion_notify (BtkWidget	   *widget,
			    BdkEventMotion *event)
{
  BtkCalendar *calendar = BTK_CALENDAR (widget);
  BtkCalendarPrivate *priv = BTK_CALENDAR_GET_PRIVATE (widget);
  gint event_x, event_y;
  gint row, col;
  gint old_row, old_col;
  
  event_x = (gint) (event->x);
  event_y = (gint) (event->y);
  
  if (event->window == priv->main_win)
    {
      
      if (priv->in_drag) 
	{
	  if (btk_drag_check_threshold (widget,
					priv->drag_start_x, priv->drag_start_y,
					event->x, event->y))
	    {
	      BdkDragContext *context;
	      BtkTargetList *target_list = btk_target_list_new (NULL, 0);
	      btk_target_list_add_text_targets (target_list, 0);
	      context = btk_drag_begin (widget, target_list, BDK_ACTION_COPY,
					1, (BdkEvent *)event);

	  
	      priv->in_drag = 0;
	      
	      btk_target_list_unref (target_list);
	      btk_drag_set_icon_default (context);
	    }
	}
      else 
	{
	  row = calendar_row_from_y (calendar, event_y);
	  col = calendar_column_from_x (calendar, event_x);
	  
	  if (row != calendar->highlight_row || calendar->highlight_col != col)
	    {
	      old_row = calendar->highlight_row;
	      old_col = calendar->highlight_col;
	      if (old_row > -1 && old_col > -1)
		{
		  calendar->highlight_row = -1;
		  calendar->highlight_col = -1;
		  calendar_invalidate_day (calendar, old_row, old_col);
		}
	      
	      calendar->highlight_row = row;
	      calendar->highlight_col = col;
	      
	      if (row > -1 && col > -1)
		calendar_invalidate_day (calendar, row, col);
	    }
	}
    }
  return TRUE;
}

static gboolean
btk_calendar_enter_notify (BtkWidget	    *widget,
			   BdkEventCrossing *event)
{
  BtkCalendar *calendar = BTK_CALENDAR (widget);
  BtkCalendarPrivate *priv = BTK_CALENDAR_GET_PRIVATE (widget);
  
  if (event->window == priv->arrow_win[ARROW_MONTH_LEFT])
    {
      priv->arrow_state[ARROW_MONTH_LEFT] = BTK_STATE_PRELIGHT;
      calendar_invalidate_arrow (calendar, ARROW_MONTH_LEFT);
    }
  
  if (event->window == priv->arrow_win[ARROW_MONTH_RIGHT])
    {
      priv->arrow_state[ARROW_MONTH_RIGHT] = BTK_STATE_PRELIGHT;
      calendar_invalidate_arrow (calendar, ARROW_MONTH_RIGHT);
    }
  
  if (event->window == priv->arrow_win[ARROW_YEAR_LEFT])
    {
      priv->arrow_state[ARROW_YEAR_LEFT] = BTK_STATE_PRELIGHT;
      calendar_invalidate_arrow (calendar, ARROW_YEAR_LEFT);
    }
  
  if (event->window == priv->arrow_win[ARROW_YEAR_RIGHT])
    {
      priv->arrow_state[ARROW_YEAR_RIGHT] = BTK_STATE_PRELIGHT;
      calendar_invalidate_arrow (calendar, ARROW_YEAR_RIGHT);
    }
  
  return TRUE;
}

static gboolean
btk_calendar_leave_notify (BtkWidget	    *widget,
			   BdkEventCrossing *event)
{
  BtkCalendar *calendar = BTK_CALENDAR (widget);
  BtkCalendarPrivate *priv = BTK_CALENDAR_GET_PRIVATE (widget);
  gint row;
  gint col;
  
  if (event->window == priv->main_win)
    {
      row = calendar->highlight_row;
      col = calendar->highlight_col;
      calendar->highlight_row = -1;
      calendar->highlight_col = -1;
      if (row > -1 && col > -1)
	calendar_invalidate_day (calendar, row, col);
    }
  
  if (event->window == priv->arrow_win[ARROW_MONTH_LEFT])
    {
      priv->arrow_state[ARROW_MONTH_LEFT] = BTK_STATE_NORMAL;
      calendar_invalidate_arrow (calendar, ARROW_MONTH_LEFT);
    }
  
  if (event->window == priv->arrow_win[ARROW_MONTH_RIGHT])
    {
      priv->arrow_state[ARROW_MONTH_RIGHT] = BTK_STATE_NORMAL;
      calendar_invalidate_arrow (calendar, ARROW_MONTH_RIGHT);
    }
  
  if (event->window == priv->arrow_win[ARROW_YEAR_LEFT])
    {
      priv->arrow_state[ARROW_YEAR_LEFT] = BTK_STATE_NORMAL;
      calendar_invalidate_arrow (calendar, ARROW_YEAR_LEFT);
    }
  
  if (event->window == priv->arrow_win[ARROW_YEAR_RIGHT])
    {
      priv->arrow_state[ARROW_YEAR_RIGHT] = BTK_STATE_NORMAL;
      calendar_invalidate_arrow (calendar, ARROW_YEAR_RIGHT);
    }
  
  return TRUE;
}

static gboolean
btk_calendar_scroll (BtkWidget      *widget,
		     BdkEventScroll *event)
{
  BtkCalendar *calendar = BTK_CALENDAR (widget);

  if (event->direction == BDK_SCROLL_UP) 
    {
      if (!btk_widget_has_focus (widget))
	btk_widget_grab_focus (widget);
      calendar_set_month_prev (calendar);
    }
  else if (event->direction == BDK_SCROLL_DOWN) 
    {
      if (!btk_widget_has_focus (widget))
	btk_widget_grab_focus (widget);
      calendar_set_month_next (calendar);
    }
  else
    return FALSE;

  return TRUE;
}


/****************************************
 *             Key handling              *
 ****************************************/

static void 
move_focus (BtkCalendar *calendar, 
	    gint         direction)
{
  BtkTextDirection text_dir = btk_widget_get_direction (BTK_WIDGET (calendar));
 
  if ((text_dir == BTK_TEXT_DIR_LTR && direction == -1) ||
      (text_dir == BTK_TEXT_DIR_RTL && direction == 1)) 
    {
      if (calendar->focus_col > 0)
	  calendar->focus_col--;
      else if (calendar->focus_row > 0)
	{
	  calendar->focus_col = 6;
	  calendar->focus_row--;
	}

      if (calendar->focus_col < 0)
        calendar->focus_col = 6;
      if (calendar->focus_row < 0)
        calendar->focus_row = 5;
    }
  else 
    {
      if (calendar->focus_col < 6)
	calendar->focus_col++;
      else if (calendar->focus_row < 5)
	{
	  calendar->focus_col = 0;
	  calendar->focus_row++;
	}

      if (calendar->focus_col < 0)
        calendar->focus_col = 0;
      if (calendar->focus_row < 0)
        calendar->focus_row = 0;
    }
}

static gboolean
btk_calendar_key_press (BtkWidget   *widget,
			BdkEventKey *event)
{
  BtkCalendar *calendar;
  gint return_val;
  gint old_focus_row;
  gint old_focus_col;
  gint row, col, day;
  
  calendar = BTK_CALENDAR (widget);
  return_val = FALSE;
  
  old_focus_row = calendar->focus_row;
  old_focus_col = calendar->focus_col;

  switch (event->keyval)
    {
    case BDK_KP_Left:
    case BDK_Left:
      return_val = TRUE;
      if (event->state & BDK_CONTROL_MASK)
	calendar_set_month_prev (calendar);
      else
	{
	  move_focus (calendar, -1);
	  calendar_invalidate_day (calendar, old_focus_row, old_focus_col);
	  calendar_invalidate_day (calendar, calendar->focus_row,
				   calendar->focus_col);
	}
      break;
    case BDK_KP_Right:
    case BDK_Right:
      return_val = TRUE;
      if (event->state & BDK_CONTROL_MASK)
	calendar_set_month_next (calendar);
      else
	{
	  move_focus (calendar, 1);
	  calendar_invalidate_day (calendar, old_focus_row, old_focus_col);
	  calendar_invalidate_day (calendar, calendar->focus_row,
				   calendar->focus_col);
	}
      break;
    case BDK_KP_Up:
    case BDK_Up:
      return_val = TRUE;
      if (event->state & BDK_CONTROL_MASK)
	calendar_set_year_prev (calendar);
      else
	{
	  if (calendar->focus_row > 0)
	    calendar->focus_row--;
          if (calendar->focus_row < 0)
            calendar->focus_row = 5;
          if (calendar->focus_col < 0)
            calendar->focus_col = 6;
	  calendar_invalidate_day (calendar, old_focus_row, old_focus_col);
	  calendar_invalidate_day (calendar, calendar->focus_row,
				   calendar->focus_col);
	}
      break;
    case BDK_KP_Down:
    case BDK_Down:
      return_val = TRUE;
      if (event->state & BDK_CONTROL_MASK)
	calendar_set_year_next (calendar);
      else
	{
	  if (calendar->focus_row < 5)
	    calendar->focus_row++;
          if (calendar->focus_col < 0)
            calendar->focus_col = 0;
	  calendar_invalidate_day (calendar, old_focus_row, old_focus_col);
	  calendar_invalidate_day (calendar, calendar->focus_row,
				   calendar->focus_col);
	}
      break;
    case BDK_KP_Space:
    case BDK_space:
      row = calendar->focus_row;
      col = calendar->focus_col;
      
      if (row > -1 && col > -1)
	{
	  return_val = TRUE;

          day = calendar->day[row][col];
	  if (calendar->day_month[row][col] == MONTH_PREV)
	    calendar_set_month_prev (calendar);
	  else if (calendar->day_month[row][col] == MONTH_NEXT)
	    calendar_set_month_next (calendar);

	  calendar_select_and_focus_day (calendar, day);
	}
    }	
  
  return return_val;
}


/****************************************
 *           Misc widget methods        *
 ****************************************/

static void
calendar_set_background (BtkWidget *widget)
{
  BtkCalendarPrivate *priv = BTK_CALENDAR_GET_PRIVATE (widget);
  gint i;
  
  if (btk_widget_get_realized (widget))
    {
      for (i = 0; i < 4; i++)
	{
	  if (priv->arrow_win[i])
	    bdk_window_set_background (priv->arrow_win[i], 
				       HEADER_BG_COLOR (widget));
	}
      if (priv->header_win)
	bdk_window_set_background (priv->header_win, 
				   HEADER_BG_COLOR (widget));
      if (priv->day_name_win)
	bdk_window_set_background (priv->day_name_win, 
				   BACKGROUND_COLOR (widget));
      if (priv->week_win)
	bdk_window_set_background (priv->week_win,
				   BACKGROUND_COLOR (widget));
      if (priv->main_win)
	bdk_window_set_background (priv->main_win,
				   BACKGROUND_COLOR (widget));
      if (widget->window)
	bdk_window_set_background (widget->window,
				   BACKGROUND_COLOR (widget)); 
    }
}

static void
btk_calendar_style_set (BtkWidget *widget,
			BtkStyle  *previous_style)
{
  if (previous_style && btk_widget_get_realized (widget))
    calendar_set_background (widget);
}

static void
btk_calendar_state_changed (BtkWidget	   *widget,
			    BtkStateType    previous_state)
{
  BtkCalendar *calendar = BTK_CALENDAR (widget);
  BtkCalendarPrivate *priv = BTK_CALENDAR_GET_PRIVATE (widget);
  int i;
  
  if (!btk_widget_is_sensitive (widget))
    {
      priv->in_drag = 0;
      calendar_stop_spinning (calendar);    
    }

  for (i = 0; i < 4; i++)
    if (btk_widget_is_sensitive (widget))
      priv->arrow_state[i] = BTK_STATE_NORMAL;
    else 
      priv->arrow_state[i] = BTK_STATE_INSENSITIVE;
  
  calendar_set_background (widget);
}

static void
btk_calendar_grab_notify (BtkWidget *widget,
			  gboolean   was_grabbed)
{
  if (!was_grabbed)
    calendar_stop_spinning (BTK_CALENDAR (widget));
}

static gboolean
btk_calendar_focus_out (BtkWidget     *widget,
			BdkEventFocus *event)
{
  BtkCalendarPrivate *priv = BTK_CALENDAR_GET_PRIVATE (widget);
  BtkCalendar *calendar = BTK_CALENDAR (widget);

  calendar_queue_refresh (calendar);
  calendar_stop_spinning (calendar);
  
  priv->in_drag = 0; 

  return FALSE;
}


/****************************************
 *          Drag and Drop               *
 ****************************************/

static void
btk_calendar_drag_data_get (BtkWidget        *widget,
			    BdkDragContext   *context,
			    BtkSelectionData *selection_data,
			    guint             info,
			    guint             time)
{
  BtkCalendar *calendar = BTK_CALENDAR (widget);
  GDate *date;
  gchar str[128];
  gsize len;

  date = g_date_new_dmy (calendar->selected_day, calendar->month + 1, calendar->year);
  len = g_date_strftime (str, 127, "%x", date);
  btk_selection_data_set_text (selection_data, str, len);
  
  g_free (date);
}

/* Get/set whether drag_motion requested the drag data and
 * drag_data_received should thus not actually insert the data,
 * since the data doesn't result from a drop.
 */
static void
set_status_pending (BdkDragContext *context,
                    BdkDragAction   suggested_action)
{
  g_object_set_data (B_OBJECT (context),
                     I_("btk-calendar-status-pending"),
                     GINT_TO_POINTER (suggested_action));
}

static BdkDragAction
get_status_pending (BdkDragContext *context)
{
  return GPOINTER_TO_INT (g_object_get_data (B_OBJECT (context),
                                             "btk-calendar-status-pending"));
}

static void
btk_calendar_drag_leave (BtkWidget      *widget,
			 BdkDragContext *context,
			 guint           time)
{
  BtkCalendarPrivate *priv = BTK_CALENDAR_GET_PRIVATE (widget);

  priv->drag_highlight = 0;
  btk_drag_unhighlight (widget);
  
}

static gboolean
btk_calendar_drag_motion (BtkWidget      *widget,
			  BdkDragContext *context,
			  gint            x,
			  gint            y,
			  guint           time)
{
  BtkCalendarPrivate *priv = BTK_CALENDAR_GET_PRIVATE (widget);
  BdkAtom target;

  if (!priv->drag_highlight)
    {
      priv->drag_highlight = 1;
      btk_drag_highlight (widget);
    }

  target = btk_drag_dest_find_target (widget, context, NULL);
  if (target == BDK_NONE || bdk_drag_context_get_suggested_action (context) == 0)
    bdk_drag_status (context, 0, time);
  else
    {
      set_status_pending (context, bdk_drag_context_get_suggested_action (context));
      btk_drag_get_data (widget, context, target, time);
    }
  
  return TRUE;
}

static gboolean
btk_calendar_drag_drop (BtkWidget      *widget,
			BdkDragContext *context,
			gint            x,
			gint            y,
			guint           time)
{
  BdkAtom target;

  target = btk_drag_dest_find_target (widget, context, NULL);  
  if (target != BDK_NONE)
    {
      btk_drag_get_data (widget, context, 
			 target, 
			 time);
      return TRUE;
    }

  return FALSE;
}

static void
btk_calendar_drag_data_received (BtkWidget        *widget,
				 BdkDragContext   *context,
				 gint              x,
				 gint              y,
				 BtkSelectionData *selection_data,
				 guint             info,
				 guint             time)
{
  BtkCalendar *calendar = BTK_CALENDAR (widget);
  guint day, month, year;
  gchar *str;
  GDate *date;
  BdkDragAction suggested_action;

  suggested_action = get_status_pending (context);

  if (suggested_action) 
    {
      set_status_pending (context, 0);
     
      /* We are getting this data due to a request in drag_motion,
       * rather than due to a request in drag_drop, so we are just
       * supposed to call drag_status, not actually paste in the
       * data.
       */
      str = (gchar*) btk_selection_data_get_text (selection_data);

      if (str) 
	{
	  date = g_date_new ();
	  g_date_set_parse (date, str);
	  if (!g_date_valid (date)) 
	      suggested_action = 0;
	  g_date_free (date);
	  g_free (str);
	}
      else
	suggested_action = 0;

      bdk_drag_status (context, suggested_action, time);

      return;
    }

  date = g_date_new ();
  str = (gchar*) btk_selection_data_get_text (selection_data);
  if (str) 
    {
      g_date_set_parse (date, str);
      g_free (str);
    }
  
  if (!g_date_valid (date)) 
    {
      g_warning ("Received invalid date data\n");
      g_date_free (date);	
      btk_drag_finish (context, FALSE, FALSE, time);
      return;
    }

  day = g_date_get_day (date);
  month = g_date_get_month (date);
  year = g_date_get_year (date);
  g_date_free (date);	

  btk_drag_finish (context, TRUE, FALSE, time);

  
  g_object_freeze_notify (B_OBJECT (calendar));
  if (!(calendar->display_flags & BTK_CALENDAR_NO_MONTH_CHANGE)
      && (calendar->display_flags & BTK_CALENDAR_SHOW_HEADING))
    btk_calendar_select_month (calendar, month - 1, year);
  btk_calendar_select_day (calendar, day);
  g_object_thaw_notify (B_OBJECT (calendar));  
}


/****************************************
 *              Public API              *
 ****************************************/

/**
 * btk_calendar_new:
 * 
 * Creates a new calendar, with the current date being selected. 
 * 
 * Return value: a newly #BtkCalendar widget
 **/
BtkWidget*
btk_calendar_new (void)
{
  return g_object_new (BTK_TYPE_CALENDAR, NULL);
}

/**
 * btk_calendar_display_options:
 * @calendar: a #BtkCalendar.
 * @flags: the display options to set.
 *
 * Sets display options (whether to display the heading and the month headings).
 * 
 * Deprecated: 2.4: Use btk_calendar_set_display_options() instead
 **/
void
btk_calendar_display_options (BtkCalendar	       *calendar,
			      BtkCalendarDisplayOptions flags)
{
  btk_calendar_set_display_options (calendar, flags);
}

/**
 * btk_calendar_get_display_options:
 * @calendar: a #BtkCalendar
 * 
 * Returns the current display options of @calendar. 
 * 
 * Return value: the display options.
 *
 * Since: 2.4
 **/
BtkCalendarDisplayOptions 
btk_calendar_get_display_options (BtkCalendar         *calendar)
{
  g_return_val_if_fail (BTK_IS_CALENDAR (calendar), 0);

  return calendar->display_flags;
}

/**
 * btk_calendar_set_display_options:
 * @calendar: a #BtkCalendar
 * @flags: the display options to set
 * 
 * Sets display options (whether to display the heading and the month  
 * headings).
 *
 * Since: 2.4
 **/
void
btk_calendar_set_display_options (BtkCalendar	       *calendar,
				  BtkCalendarDisplayOptions flags)
{
  BtkWidget *widget = BTK_WIDGET (calendar);
  BtkCalendarPrivate *priv = BTK_CALENDAR_GET_PRIVATE (calendar);
  gint resize = 0;
  gint i;
  BtkCalendarDisplayOptions old_flags;
  
  g_return_if_fail (BTK_IS_CALENDAR (calendar));
  
  old_flags = calendar->display_flags;
  
  if (btk_widget_get_realized (widget))
    {
      if ((flags ^ calendar->display_flags) & BTK_CALENDAR_NO_MONTH_CHANGE)
	{
	  resize ++;
	  if (! (flags & BTK_CALENDAR_NO_MONTH_CHANGE)
	      && (priv->header_win))
	    {
	      calendar->display_flags &= ~BTK_CALENDAR_NO_MONTH_CHANGE;
	      calendar_realize_arrows (calendar);
	    }
	  else
	    {
	      for (i = 0; i < 4; i++)
		{
		  if (priv->arrow_win[i])
		    {
		      bdk_window_set_user_data (priv->arrow_win[i], 
						NULL);
		      bdk_window_destroy (priv->arrow_win[i]);
		      priv->arrow_win[i] = NULL;
		    }
		}
	    }
	}
      
      if ((flags ^ calendar->display_flags) & BTK_CALENDAR_SHOW_HEADING)
	{
	  resize++;
	  
	  if (flags & BTK_CALENDAR_SHOW_HEADING)
	    {
	      calendar->display_flags |= BTK_CALENDAR_SHOW_HEADING;
	      calendar_realize_header (calendar);
	    }
	  else
	    {
	      for (i = 0; i < 4; i++)
		{
		  if (priv->arrow_win[i])
		    {
		      bdk_window_set_user_data (priv->arrow_win[i], 
						NULL);
		      bdk_window_destroy (priv->arrow_win[i]);
		      priv->arrow_win[i] = NULL;
		    }
		}
	      bdk_window_set_user_data (priv->header_win, NULL);
	      bdk_window_destroy (priv->header_win);
	      priv->header_win = NULL;
	    }
	}
      
      
      if ((flags ^ calendar->display_flags) & BTK_CALENDAR_SHOW_DAY_NAMES)
	{
	  resize++;
	  
	  if (flags & BTK_CALENDAR_SHOW_DAY_NAMES)
	    {
	      calendar->display_flags |= BTK_CALENDAR_SHOW_DAY_NAMES;
	      calendar_realize_day_names (calendar);
	    }
	  else
	    {
	      bdk_window_set_user_data (priv->day_name_win, NULL);
	      bdk_window_destroy (priv->day_name_win);
	      priv->day_name_win = NULL;
	    }
	}
      
      if ((flags ^ calendar->display_flags) & BTK_CALENDAR_SHOW_WEEK_NUMBERS)
	{
	  resize++;
	  
	  if (flags & BTK_CALENDAR_SHOW_WEEK_NUMBERS)
	    {
	      calendar->display_flags |= BTK_CALENDAR_SHOW_WEEK_NUMBERS;
	      calendar_realize_week_numbers (calendar);
	    }
	  else
	    {
	      bdk_window_set_user_data (priv->week_win, NULL);
	      bdk_window_destroy (priv->week_win);
	      priv->week_win = NULL;
	    }
	}

      if ((flags ^ calendar->display_flags) & BTK_CALENDAR_WEEK_START_MONDAY)
	g_warning ("BTK_CALENDAR_WEEK_START_MONDAY is ignored; the first day of the week is determined from the locale");
      
      if ((flags ^ calendar->display_flags) & BTK_CALENDAR_SHOW_DETAILS)
        resize++;

      calendar->display_flags = flags;
      if (resize)
	btk_widget_queue_resize (BTK_WIDGET (calendar));
      
    } 
  else
    calendar->display_flags = flags;
  
  g_object_freeze_notify (B_OBJECT (calendar));
  if ((old_flags ^ calendar->display_flags) & BTK_CALENDAR_SHOW_HEADING)
    g_object_notify (B_OBJECT (calendar), "show-heading");
  if ((old_flags ^ calendar->display_flags) & BTK_CALENDAR_SHOW_DAY_NAMES)
    g_object_notify (B_OBJECT (calendar), "show-day-names");
  if ((old_flags ^ calendar->display_flags) & BTK_CALENDAR_NO_MONTH_CHANGE)
    g_object_notify (B_OBJECT (calendar), "no-month-change");
  if ((old_flags ^ calendar->display_flags) & BTK_CALENDAR_SHOW_WEEK_NUMBERS)
    g_object_notify (B_OBJECT (calendar), "show-week-numbers");
  g_object_thaw_notify (B_OBJECT (calendar));
}

/**
 * btk_calendar_select_month:
 * @calendar: a #BtkCalendar
 * @month: a month number between 0 and 11.
 * @year: the year the month is in.
 *
 * Shifts the calendar to a different month.
 *
 * Note that this function always returns %TRUE, and you should
 * ignore the return value. In BTK+ 3, this function will not
 * return a value.
 *
 * Returns: %TRUE, always
 **/
gboolean
btk_calendar_select_month (BtkCalendar *calendar,
			   guint	month,
			   guint	year)
{
  g_return_val_if_fail (BTK_IS_CALENDAR (calendar), FALSE);
  g_return_val_if_fail (month <= 11, FALSE);

  calendar->month = month;
  calendar->year  = year;

  calendar_compute_days (calendar);
  calendar_queue_refresh (calendar);

  g_object_freeze_notify (B_OBJECT (calendar));
  g_object_notify (B_OBJECT (calendar), "month");
  g_object_notify (B_OBJECT (calendar), "year");
  g_object_thaw_notify (B_OBJECT (calendar));

  g_signal_emit (calendar,
		 btk_calendar_signals[MONTH_CHANGED_SIGNAL],
		 0);

  return TRUE;
}

/**
 * btk_calendar_select_day:
 * @calendar: a #BtkCalendar.
 * @day: the day number between 1 and 31, or 0 to unselect 
 *   the currently selected day.
 * 
 * Selects a day from the current month.
 **/
void
btk_calendar_select_day (BtkCalendar *calendar,
			 guint	      day)
{
  g_return_if_fail (BTK_IS_CALENDAR (calendar));
  g_return_if_fail (day <= 31);
  
  /* Deselect the old day */
  if (calendar->selected_day > 0)
    {
      gint selected_day;
      
      selected_day = calendar->selected_day;
      calendar->selected_day = 0;
      if (btk_widget_is_drawable (BTK_WIDGET (calendar)))
	calendar_invalidate_day_num (calendar, selected_day);
    }
  
  calendar->selected_day = day;
  
  /* Select the new day */
  if (day != 0)
    {
      if (btk_widget_is_drawable (BTK_WIDGET (calendar)))
	calendar_invalidate_day_num (calendar, day);
    }
  
  g_object_notify (B_OBJECT (calendar), "day");

  g_signal_emit (calendar,
		 btk_calendar_signals[DAY_SELECTED_SIGNAL],
		 0);
}

/**
 * btk_calendar_clear_marks:
 * @calendar: a #BtkCalendar
 * 
 * Remove all visual markers.
 **/
void
btk_calendar_clear_marks (BtkCalendar *calendar)
{
  guint day;
  
  g_return_if_fail (BTK_IS_CALENDAR (calendar));
  
  for (day = 0; day < 31; day++)
    {
      calendar->marked_date[day] = FALSE;
    }

  calendar->num_marked_dates = 0;
  calendar_queue_refresh (calendar);
}

/**
 * btk_calendar_mark_day:
 * @calendar: a #BtkCalendar
 * @day: the day number to mark between 1 and 31.
 *
 * Places a visual marker on a particular day.
 *
 * Note that this function always returns %TRUE, and you should
 * ignore the return value. In BTK+ 3, this function will not
 * return a value.
 *
 * Returns: %TRUE, always
 */
gboolean
btk_calendar_mark_day (BtkCalendar *calendar,
		       guint	    day)
{
  g_return_val_if_fail (BTK_IS_CALENDAR (calendar), FALSE);

  if (day >= 1 && day <= 31 && !calendar->marked_date[day-1])
    {
      calendar->marked_date[day - 1] = TRUE;
      calendar->num_marked_dates++;
      calendar_invalidate_day_num (calendar, day);
    }

  return TRUE;
}

/**
 * btk_calendar_unmark_day:
 * @calendar: a #BtkCalendar.
 * @day: the day number to unmark between 1 and 31.
 *
 * Removes the visual marker from a particular day.
 *
 * Note that this function always returns %TRUE, and you should
 * ignore the return value. In BTK+ 3, this function will not
 * return a value.
 *
 * Returns: %TRUE, always
 */
gboolean
btk_calendar_unmark_day (BtkCalendar *calendar,
			 guint	      day)
{
  g_return_val_if_fail (BTK_IS_CALENDAR (calendar), FALSE);

  if (day >= 1 && day <= 31 && calendar->marked_date[day-1])
    {
      calendar->marked_date[day - 1] = FALSE;
      calendar->num_marked_dates--;
      calendar_invalidate_day_num (calendar, day);
    }

  return TRUE;
}

/**
 * btk_calendar_get_date:
 * @calendar: a #BtkCalendar
 * @year: (out) (allow-none): location to store the year as a decimal
 *     number (e.g. 2011), or %NULL
 * @month: (out) (allow-none): location to store the month number
 *     (between 0 and 11), or %NULL
 * @day: (out) (allow-none): location to store the day number (between
 *     1 and 31), or %NULL
 *
 * Obtains the selected date from a #BtkCalendar.
 **/
void
btk_calendar_get_date (BtkCalendar *calendar,
		       guint	   *year,
		       guint	   *month,
		       guint	   *day)
{
  g_return_if_fail (BTK_IS_CALENDAR (calendar));
  
  if (year)
    *year = calendar->year;
  
  if (month)
    *month = calendar->month;
  
  if (day)
    *day = calendar->selected_day;
}

/**
 * btk_calendar_set_detail_func:
 * @calendar: a #BtkCalendar.
 * @func: a function providing details for each day.
 * @data: data to pass to @func invokations.
 * @destroy: a function for releasing @data.
 *
 * Installs a function which provides Bango markup with detail information
 * for each day. Examples for such details are holidays or appointments. That
 * information is shown below each day when #BtkCalendar:show-details is set.
 * A tooltip containing with full detail information is provided, if the entire
 * text should not fit into the details area, or if #BtkCalendar:show-details
 * is not set.
 *
 * The size of the details area can be restricted by setting the
 * #BtkCalendar:detail-width-chars and #BtkCalendar:detail-height-rows
 * properties.
 *
 * Since: 2.14
 */
void
btk_calendar_set_detail_func (BtkCalendar           *calendar,
                              BtkCalendarDetailFunc  func,
                              gpointer               data,
                              GDestroyNotify         destroy)
{
  BtkCalendarPrivate *priv;

  g_return_if_fail (BTK_IS_CALENDAR (calendar));

  priv = BTK_CALENDAR_GET_PRIVATE (calendar);

  if (priv->detail_func_destroy)
    priv->detail_func_destroy (priv->detail_func_user_data);

  priv->detail_func = func;
  priv->detail_func_user_data = data;
  priv->detail_func_destroy = destroy;

  btk_widget_set_has_tooltip (BTK_WIDGET (calendar),
                              NULL != priv->detail_func);
  btk_widget_queue_resize (BTK_WIDGET (calendar));
}

/**
 * btk_calendar_set_detail_width_chars:
 * @calendar: a #BtkCalendar.
 * @chars: detail width in characters.
 *
 * Updates the width of detail cells.
 * See #BtkCalendar:detail-width-chars.
 *
 * Since: 2.14
 */
void
btk_calendar_set_detail_width_chars (BtkCalendar *calendar,
                                     gint         chars)
{
  BtkCalendarPrivate *priv;

  g_return_if_fail (BTK_IS_CALENDAR (calendar));

  priv = BTK_CALENDAR_GET_PRIVATE (calendar);

  if (chars != priv->detail_width_chars)
    {
      priv->detail_width_chars = chars;
      g_object_notify (B_OBJECT (calendar), "detail-width-chars");
      btk_widget_queue_resize_no_redraw (BTK_WIDGET (calendar));
    }
}

/**
 * btk_calendar_set_detail_height_rows:
 * @calendar: a #BtkCalendar.
 * @rows: detail height in rows.
 *
 * Updates the height of detail cells.
 * See #BtkCalendar:detail-height-rows.
 *
 * Since: 2.14
 */
void
btk_calendar_set_detail_height_rows (BtkCalendar *calendar,
                                     gint         rows)
{
  BtkCalendarPrivate *priv;

  g_return_if_fail (BTK_IS_CALENDAR (calendar));

  priv = BTK_CALENDAR_GET_PRIVATE (calendar);

  if (rows != priv->detail_height_rows)
    {
      priv->detail_height_rows = rows;
      g_object_notify (B_OBJECT (calendar), "detail-height-rows");
      btk_widget_queue_resize_no_redraw (BTK_WIDGET (calendar));
    }
}

/**
 * btk_calendar_get_detail_width_chars:
 * @calendar: a #BtkCalendar.
 *
 * Queries the width of detail cells, in characters.
 * See #BtkCalendar:detail-width-chars.
 *
 * Since: 2.14
 *
 * Return value: The width of detail cells, in characters.
 */
gint
btk_calendar_get_detail_width_chars (BtkCalendar *calendar)
{
  g_return_val_if_fail (BTK_IS_CALENDAR (calendar), 0);
  return BTK_CALENDAR_GET_PRIVATE (calendar)->detail_width_chars;
}

/**
 * btk_calendar_get_detail_height_rows:
 * @calendar: a #BtkCalendar.
 *
 * Queries the height of detail cells, in rows.
 * See #BtkCalendar:detail-width-chars.
 *
 * Since: 2.14
 *
 * Return value: The height of detail cells, in rows.
 */
gint
btk_calendar_get_detail_height_rows (BtkCalendar *calendar)
{
  g_return_val_if_fail (BTK_IS_CALENDAR (calendar), 0);
  return BTK_CALENDAR_GET_PRIVATE (calendar)->detail_height_rows;
}

/**
 * btk_calendar_freeze:
 * @calendar: a #BtkCalendar
 * 
 * Does nothing. Previously locked the display of the calendar until
 * it was thawed with btk_calendar_thaw().
 *
 * Deprecated: 2.8: 
 **/
void
btk_calendar_freeze (BtkCalendar *calendar)
{
  g_return_if_fail (BTK_IS_CALENDAR (calendar));
}

/**
 * btk_calendar_thaw:
 * @calendar: a #BtkCalendar
 * 
 * Does nothing. Previously defrosted a calendar; all the changes made
 * since the last btk_calendar_freeze() were displayed.
 *
 * Deprecated: 2.8: 
 **/
void
btk_calendar_thaw (BtkCalendar *calendar)
{
  g_return_if_fail (BTK_IS_CALENDAR (calendar));
}

#define __BTK_CALENDAR_C__
#include "btkaliasdef.c"
