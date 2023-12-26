/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * BTK Calendar Widget
 * Copyright (C) 1998 Cesar Miquel and Shawn T. Amundson
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

#ifndef __BTK_CALENDAR_H__
#define __BTK_CALENDAR_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkwidget.h>

/* Not needed, retained for compatibility -Yosh */
#include <btk/btksignal.h>


B_BEGIN_DECLS

#define BTK_TYPE_CALENDAR                  (btk_calendar_get_type ())
#define BTK_CALENDAR(obj)                  (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_CALENDAR, BtkCalendar))
#define BTK_CALENDAR_CLASS(klass)          (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_CALENDAR, BtkCalendarClass))
#define BTK_IS_CALENDAR(obj)               (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_CALENDAR))
#define BTK_IS_CALENDAR_CLASS(klass)       (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_CALENDAR))
#define BTK_CALENDAR_GET_CLASS(obj)        (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_CALENDAR, BtkCalendarClass))


typedef struct _BtkCalendar	       BtkCalendar;
typedef struct _BtkCalendarClass       BtkCalendarClass;

typedef struct _BtkCalendarPrivate     BtkCalendarPrivate;

/**
 * BtkCalendarDisplayOptions:
 * @BTK_CALENDAR_SHOW_HEADING: Specifies that the month and year should be displayed.
 * @BTK_CALENDAR_SHOW_DAY_NAMES: Specifies that three letter day descriptions should be present.
 * @BTK_CALENDAR_NO_MONTH_CHANGE: Prevents the user from switching months with the calendar.
 * @BTK_CALENDAR_SHOW_WEEK_NUMBERS: Displays each week numbers of the current year, down the
 * left side of the calendar.
 * @BTK_CALENDAR_WEEK_START_MONDAY: Since BTK+ 2.4, this option is deprecated and ignored by BTK+.
 * The information on which day the calendar week starts is derived from the locale.
 * @BTK_CALENDAR_SHOW_DETAILS: Just show an indicator, not the full details
 * text when details are provided. See btk_calendar_set_detail_func().
 *
 * These options can be used to influence the display and behaviour of a #BtkCalendar.
 */
typedef enum
{
  BTK_CALENDAR_SHOW_HEADING		= 1 << 0,
  BTK_CALENDAR_SHOW_DAY_NAMES		= 1 << 1,
  BTK_CALENDAR_NO_MONTH_CHANGE		= 1 << 2,
  BTK_CALENDAR_SHOW_WEEK_NUMBERS	= 1 << 3,
  BTK_CALENDAR_WEEK_START_MONDAY	= 1 << 4,
  BTK_CALENDAR_SHOW_DETAILS		= 1 << 5
} BtkCalendarDisplayOptions;

/**
 * BtkCalendarDetailFunc:
 * @calendar: a #BtkCalendar.
 * @year: the year for which details are needed.
 * @month: the month for which details are needed.
 * @day: the day of @month for which details are needed.
 * @user_data: the data passed with btk_calendar_set_detail_func().
 *
 * This kind of functions provide Bango markup with detail information for the
 * specified day. Examples for such details are holidays or appointments. The
 * function returns %NULL when no information is available.
 *
 * Since: 2.14
 *
 * Return value: Newly allocated string with Bango markup with details
 * for the specified day, or %NULL.
 */
typedef bchar* (*BtkCalendarDetailFunc) (BtkCalendar *calendar,
                                         buint        year,
                                         buint        month,
                                         buint        day,
                                         bpointer     user_data);

struct _BtkCalendar
{
  BtkWidget widget;
  
  BtkStyle  *GSEAL (header_style);
  BtkStyle  *GSEAL (label_style);
  
  bint GSEAL (month);
  bint GSEAL (year);
  bint GSEAL (selected_day);
  
  bint GSEAL (day_month[6][7]);
  bint GSEAL (day[6][7]);
  
  bint GSEAL (num_marked_dates);
  bint GSEAL (marked_date[31]);
  BtkCalendarDisplayOptions  GSEAL (display_flags);
  BdkColor GSEAL (marked_date_color[31]);
  
  BdkGC *GSEAL (gc);			/* unused */
  BdkGC *GSEAL (xor_gc);		/* unused */

  bint GSEAL (focus_row);
  bint GSEAL (focus_col);

  bint GSEAL (highlight_row);
  bint GSEAL (highlight_col);
  
  BtkCalendarPrivate *GSEAL (priv);
  bchar GSEAL (grow_space [32]);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};

struct _BtkCalendarClass
{
  BtkWidgetClass parent_class;
  
  /* Signal handlers */
  void (* month_changed)		(BtkCalendar *calendar);
  void (* day_selected)			(BtkCalendar *calendar);
  void (* day_selected_double_click)	(BtkCalendar *calendar);
  void (* prev_month)			(BtkCalendar *calendar);
  void (* next_month)			(BtkCalendar *calendar);
  void (* prev_year)			(BtkCalendar *calendar);
  void (* next_year)			(BtkCalendar *calendar);
  
};


GType	   btk_calendar_get_type	(void) B_GNUC_CONST;
BtkWidget* btk_calendar_new		(void);

bboolean   btk_calendar_select_month	(BtkCalendar *calendar,
					 buint	      month,
					 buint	      year);
void	   btk_calendar_select_day	(BtkCalendar *calendar,
					 buint	      day);

bboolean   btk_calendar_mark_day	(BtkCalendar *calendar,
					 buint	      day);
bboolean   btk_calendar_unmark_day	(BtkCalendar *calendar,
					 buint	      day);
void	   btk_calendar_clear_marks	(BtkCalendar *calendar);


void	   btk_calendar_set_display_options (BtkCalendar    	      *calendar,
					     BtkCalendarDisplayOptions flags);
BtkCalendarDisplayOptions
           btk_calendar_get_display_options (BtkCalendar   	      *calendar);
#ifndef BTK_DISABLE_DEPRECATED
void	   btk_calendar_display_options (BtkCalendar		  *calendar,
					 BtkCalendarDisplayOptions flags);
#endif

void	   btk_calendar_get_date	(BtkCalendar *calendar, 
					 buint	     *year,
					 buint	     *month,
					 buint	     *day);

void       btk_calendar_set_detail_func (BtkCalendar           *calendar,
                                         BtkCalendarDetailFunc  func,
                                         bpointer               data,
                                         GDestroyNotify         destroy);

void       btk_calendar_set_detail_width_chars (BtkCalendar    *calendar,
                                                bint            chars);
void       btk_calendar_set_detail_height_rows (BtkCalendar    *calendar,
                                                bint            rows);

bint       btk_calendar_get_detail_width_chars (BtkCalendar    *calendar);
bint       btk_calendar_get_detail_height_rows (BtkCalendar    *calendar);

#ifndef BTK_DISABLE_DEPRECATED
void	   btk_calendar_freeze		(BtkCalendar *calendar);
void	   btk_calendar_thaw		(BtkCalendar *calendar);
#endif

B_END_DECLS

#endif /* __BTK_CALENDAR_H__ */
