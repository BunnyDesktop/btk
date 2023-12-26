/*
 * Copyright (C) 1998 Cesar Miquel, Shawn T. Amundson, Mattias Grönlund
 * Copyright (C) 2000 Tony Gale
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <string.h>
#include <btk/btk.h>

#define DEF_PAD 10
#define DEF_PAD_SMALL 5

#define TM_YEAR_BASE 1900

typedef struct _CalendarData {
  BtkWidget *flag_checkboxes[5];
  bboolean  settings[5];
  BtkWidget *font_dialog;
  BtkWidget *window;
  BtkWidget *prev2_sig;
  BtkWidget *prev_sig;
  BtkWidget *last_sig;
  BtkWidget *month;
} CalendarData;

enum {
  calendar_show_header,
  calendar_show_days,
  calendar_month_change,
  calendar_show_week,
  calendar_monday_first
};

/*
 * BtkCalendar
 */

static void calendar_date_to_string( CalendarData *data,
                                     char         *buffer,
                                     bint          buff_len )
{
  GDate date;
  buint year, month, day;

  btk_calendar_get_date (BTK_CALENDAR (data->window),
			 &year, &month, &day);
  g_date_set_dmy (&date, day, month + 1, year);
  g_date_strftime (buffer, buff_len - 1, "%x", &date);

}

static void calendar_set_signal_strings( char         *sig_str,
                                         CalendarData *data )
{
  const bchar *prev_sig;

  prev_sig = btk_label_get_text (BTK_LABEL (data->prev_sig));
  btk_label_set_text (BTK_LABEL (data->prev2_sig), prev_sig);

  prev_sig = btk_label_get_text (BTK_LABEL (data->last_sig));
  btk_label_set_text (BTK_LABEL (data->prev_sig), prev_sig);
  btk_label_set_text (BTK_LABEL (data->last_sig), sig_str);
}

static void calendar_month_changed( BtkWidget    *widget,
                                    CalendarData *data )
{
  char buffer[256] = "month_changed: ";

  calendar_date_to_string (data, buffer + 15, 256 - 15);
  calendar_set_signal_strings (buffer, data);
}

static void calendar_day_selected( BtkWidget    *widget,
                                   CalendarData *data )
{
  char buffer[256] = "day_selected: ";

  calendar_date_to_string (data, buffer + 14, 256 - 14);
  calendar_set_signal_strings (buffer, data);
}

static void calendar_day_selected_double_click ( BtkWidget    *widget,
                                                 CalendarData *data )
{
  char buffer[256] = "day_selected_double_click: ";
  buint day;

  calendar_date_to_string (data, buffer + 27, 256 - 27);
  calendar_set_signal_strings (buffer, data);

  btk_calendar_get_date (BTK_CALENDAR (data->window),
			 NULL, NULL, &day);

  if (BTK_CALENDAR (data->window)->marked_date[day-1] == 0) {
    btk_calendar_mark_day (BTK_CALENDAR (data->window), day);
  } else {
    btk_calendar_unmark_day (BTK_CALENDAR (data->window), day);
  }
}

static void calendar_prev_month( BtkWidget    *widget,
                                 CalendarData *data )
{
  char buffer[256] = "prev_month: ";

  calendar_date_to_string (data, buffer + 12, 256 - 12);
  calendar_set_signal_strings (buffer, data);
}

static void calendar_next_month( BtkWidget    *widget,
                                 CalendarData *data )
{
  char buffer[256] = "next_month: ";

  calendar_date_to_string (data, buffer + 12, 256 - 12);
  calendar_set_signal_strings (buffer, data);
}

static void calendar_prev_year( BtkWidget    *widget,
                                CalendarData *data )
{
  char buffer[256] = "prev_year: ";

  calendar_date_to_string (data, buffer + 11, 256 - 11);
  calendar_set_signal_strings (buffer, data);
}

static void calendar_next_year( BtkWidget    *widget,
                                CalendarData *data )
{
  char buffer[256] = "next_year: ";

  calendar_date_to_string (data, buffer + 11, 256 - 11);
  calendar_set_signal_strings (buffer, data);
}


static void calendar_set_flags( CalendarData *calendar )
{
  bint i;
  bint options = 0;
  for (i = 0;i < 5; i++)
    if (calendar->settings[i])
      {
	options = options + (1 << i);
      }
  if (calendar->window)
    btk_calendar_display_options (BTK_CALENDAR (calendar->window), options);
}

static void calendar_toggle_flag( BtkWidget    *toggle,
                                  CalendarData *calendar)
{
  bint i;
  bint j;
  j = 0;
  for (i = 0; i < 5; i++)
    if (calendar->flag_checkboxes[i] == toggle)
      j = i;

  calendar->settings[j] = !calendar->settings[j];
  calendar_set_flags (calendar);

}

static void calendar_font_selection_ok( BtkWidget    *button,
                                        CalendarData *calendar )
{
  BtkRcStyle *style;
  char *font_name;

  if (calendar->window)
    {
      font_name = btk_font_selection_dialog_get_font_name (BTK_FONT_SELECTION_DIALOG (calendar->font_dialog));
      if (font_name)
	{
	  style = btk_rc_style_new ();
	  bango_font_description_free (style->font_desc);
	  style->font_desc = bango_font_description_from_string (font_name);
	  btk_widget_modify_style (calendar->window, style);
	  g_free (font_name);
	}
    }

  btk_widget_destroy (calendar->font_dialog);
}

static void calendar_select_font( BtkWidget    *button,
                                  CalendarData *calendar )
{
  BtkWidget *window;

  if (!calendar->font_dialog) {
    window = btk_font_selection_dialog_new ("Font Selection Dialog");
    g_return_if_fail (BTK_IS_FONT_SELECTION_DIALOG (window));
    calendar->font_dialog = window;

    btk_window_set_position (BTK_WINDOW (window), BTK_WIN_POS_MOUSE);

    g_signal_connect (window, "destroy",
		      G_CALLBACK (btk_widget_destroyed),
		      &calendar->font_dialog);

    g_signal_connect (BTK_FONT_SELECTION_DIALOG (window)->ok_button,
		      "clicked", G_CALLBACK (calendar_font_selection_ok),
		      calendar);
    g_signal_connect_swapped (BTK_FONT_SELECTION_DIALOG (window)->cancel_button,
			     "clicked", G_CALLBACK (btk_widget_destroy),
			     calendar->font_dialog);
  }
  window = calendar->font_dialog;
  if (!btk_widget_get_visible (window))
    btk_widget_show (window);
  else
    btk_widget_destroy (window);

}

static void create_calendar( void )
{
  BtkWidget *window;
  BtkWidget *vbox, *vbox2, *vbox3;
  BtkWidget *hbox;
  BtkWidget *hbbox;
  BtkWidget *calendar;
  BtkWidget *toggle;
  BtkWidget *button;
  BtkWidget *frame;
  BtkWidget *separator;
  BtkWidget *label;
  BtkWidget *bbox;
  static CalendarData calendar_data;
  bint i;

  struct {
    char *label;
  } flags[] =
    {
      { "Show Heading" },
      { "Show Day Names" },
      { "No Month Change" },
      { "Show Week Numbers" },
      { "Week Start Monday" }
    };


  calendar_data.window = NULL;
  calendar_data.font_dialog = NULL;

  for (i = 0; i < 5; i++) {
    calendar_data.settings[i] = 0;
  }

  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  btk_window_set_title (BTK_WINDOW (window), "BtkCalendar Example");
  btk_container_set_border_width (BTK_CONTAINER (window), 5);
  g_signal_connect (window, "destroy",
		    G_CALLBACK (btk_main_quit),
		    NULL);
  g_signal_connect (window, "delete-event",
		    G_CALLBACK (btk_false),
		    NULL);
  btk_window_set_resizable (BTK_WINDOW (window), FALSE);

  vbox = btk_vbox_new (FALSE, DEF_PAD);
  btk_container_add (BTK_CONTAINER (window), vbox);

  /*
   * The top part of the window, Calendar, flags and fontsel.
   */

  hbox = btk_hbox_new (FALSE, DEF_PAD);
  btk_box_pack_start (BTK_BOX (vbox), hbox, TRUE, TRUE, DEF_PAD);
  hbbox = btk_hbutton_box_new ();
  btk_box_pack_start (BTK_BOX (hbox), hbbox, FALSE, FALSE, DEF_PAD);
  btk_button_box_set_layout (BTK_BUTTON_BOX (hbbox), BTK_BUTTONBOX_SPREAD);
  btk_box_set_spacing (BTK_BOX (hbbox), 5);

  /* Calendar widget */
  frame = btk_frame_new ("Calendar");
  btk_box_pack_start(BTK_BOX (hbbox), frame, FALSE, TRUE, DEF_PAD);
  calendar=btk_calendar_new ();
  calendar_data.window = calendar;
  calendar_set_flags (&calendar_data);
  btk_calendar_mark_day (BTK_CALENDAR (calendar), 19);
  btk_container_add (BTK_CONTAINER (frame), calendar);
  g_signal_connect (calendar, "month-changed",
		    G_CALLBACK (calendar_month_changed),
		    &calendar_data);
  g_signal_connect (calendar, "day-selected",
		    G_CALLBACK (calendar_day_selected),
		    &calendar_data);
  g_signal_connect (calendar, "day-selected-double-click",
		    G_CALLBACK (calendar_day_selected_double_click),
		    &calendar_data);
  g_signal_connect (calendar, "prev-month",
		    G_CALLBACK (calendar_prev_month),
		    &calendar_data);
  g_signal_connect (calendar, "next-month",
		    G_CALLBACK (calendar_next_month),
		    &calendar_data);
  g_signal_connect (calendar, "prev-year",
		    G_CALLBACK (calendar_prev_year),
		    &calendar_data);
  g_signal_connect (calendar, "next-year",
		    G_CALLBACK (calendar_next_year),
		    &calendar_data);


  separator = btk_vseparator_new ();
  btk_box_pack_start (BTK_BOX (hbox), separator, FALSE, TRUE, 0);

  vbox2 = btk_vbox_new (FALSE, DEF_PAD);
  btk_box_pack_start (BTK_BOX (hbox), vbox2, FALSE, FALSE, DEF_PAD);

  /* Build the Right frame with the flags in */

  frame = btk_frame_new ("Flags");
  btk_box_pack_start (BTK_BOX (vbox2), frame, TRUE, TRUE, DEF_PAD);
  vbox3 = btk_vbox_new (TRUE, DEF_PAD_SMALL);
  btk_container_add (BTK_CONTAINER (frame), vbox3);

  for (i = 0; i < 5; i++)
    {
      toggle = btk_check_button_new_with_label (flags[i].label);
      g_signal_connect (toggle,
			"toggled",
			G_CALLBACK (calendar_toggle_flag),
			&calendar_data);
      btk_box_pack_start (BTK_BOX (vbox3), toggle, TRUE, TRUE, 0);
      calendar_data.flag_checkboxes[i] = toggle;
    }
  /* Build the right font-button */
  button = btk_button_new_with_label ("Font...");
  g_signal_connect (button,
		    "clicked",
		    G_CALLBACK (calendar_select_font),
		    &calendar_data);
  btk_box_pack_start (BTK_BOX (vbox2), button, FALSE, FALSE, 0);

  /*
   *  Build the Signal-event part.
   */

  frame = btk_frame_new ("Signal events");
  btk_box_pack_start (BTK_BOX (vbox), frame, TRUE, TRUE, DEF_PAD);

  vbox2 = btk_vbox_new (TRUE, DEF_PAD_SMALL);
  btk_container_add (BTK_CONTAINER (frame), vbox2);

  hbox = btk_hbox_new (FALSE, 3);
  btk_box_pack_start (BTK_BOX (vbox2), hbox, FALSE, TRUE, 0);
  label = btk_label_new ("Signal:");
  btk_box_pack_start (BTK_BOX (hbox), label, FALSE, TRUE, 0);
  calendar_data.last_sig = btk_label_new ("");
  btk_box_pack_start (BTK_BOX (hbox), calendar_data.last_sig, FALSE, TRUE, 0);

  hbox = btk_hbox_new (FALSE, 3);
  btk_box_pack_start (BTK_BOX (vbox2), hbox, FALSE, TRUE, 0);
  label = btk_label_new ("Previous signal:");
  btk_box_pack_start (BTK_BOX (hbox), label, FALSE, TRUE, 0);
  calendar_data.prev_sig = btk_label_new ("");
  btk_box_pack_start (BTK_BOX (hbox), calendar_data.prev_sig, FALSE, TRUE, 0);

  hbox = btk_hbox_new (FALSE, 3);
  btk_box_pack_start (BTK_BOX (vbox2), hbox, FALSE, TRUE, 0);
  label = btk_label_new ("Second previous signal:");
  btk_box_pack_start (BTK_BOX (hbox), label, FALSE, TRUE, 0);
  calendar_data.prev2_sig = btk_label_new ("");
  btk_box_pack_start (BTK_BOX (hbox), calendar_data.prev2_sig, FALSE, TRUE, 0);

  bbox = btk_hbutton_box_new ();
  btk_box_pack_start (BTK_BOX (vbox), bbox, FALSE, FALSE, 0);
  btk_button_box_set_layout (BTK_BUTTON_BOX (bbox), BTK_BUTTONBOX_END);

  button = btk_button_new_with_label ("Close");
  g_signal_connect (button, "clicked",
		    G_CALLBACK (btk_main_quit),
		    NULL);
  btk_container_add (BTK_CONTAINER (bbox), button);
  btk_widget_set_can_default (button, TRUE);
  btk_widget_grab_default (button);

  btk_widget_show_all (window);
}


int main (int   argc,
          char *argv[])
{
  btk_init (&argc, &argv);

  create_calendar ();

  btk_main ();

  return 0;
}
