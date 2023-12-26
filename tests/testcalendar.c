/* example-start calendar calendar.c */
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

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <btk/btk.h>

#define DEF_PAD 12
#define DEF_PAD_SMALL 6

#define TM_YEAR_BASE 1900

typedef struct _CalendarData
{
  BtkWidget *calendar_widget;
  BtkWidget *flag_checkboxes[6];
  gboolean  settings[6];
  BtkWidget *font_dialog;
  BtkWidget *window;
  BtkWidget *prev2_sig;
  BtkWidget *prev_sig;
  BtkWidget *last_sig;
  BtkWidget *month;

  GHashTable    *details_table;
  BtkTextBuffer *details_buffer;
  guint          details_changed;
} CalendarData;

enum
{
  calendar_show_header,
  calendar_show_days,
  calendar_month_change, 
  calendar_show_week,
  calendar_monday_first
};

/*
 * BtkCalendar
 */

static void
calendar_date_to_string (CalendarData *data,
			      char         *buffer,
			      gint          buff_len)
{
  GDate *date;
  guint year, month, day;

  btk_calendar_get_date (BTK_CALENDAR(data->window),
			 &year, &month, &day);
  date = g_date_new_dmy (day, month + 1, year);
  g_date_strftime (buffer, buff_len-1, "%x", date);

  g_date_free (date);
}

static void
calendar_set_detail (CalendarData *data,
                     guint         year,
                     guint         month,
                     guint         day,
                     gchar        *detail)
{
  gchar *key = g_strdup_printf ("%04d-%02d-%02d", year, month + 1, day);
  g_hash_table_replace (data->details_table, key, detail);
}

static gchar*
calendar_get_detail (CalendarData *data,
                     guint         year,
                     guint         month,
                     guint         day)
{
  const gchar *detail;
  gchar *key;

  key = g_strdup_printf ("%04d-%02d-%02d", year, month + 1, day);
  detail = g_hash_table_lookup (data->details_table, key);
  g_free (key);

  return (detail ? g_strdup (detail) : NULL);
}

static void
calendar_update_details (CalendarData *data)
{
  guint year, month, day;
  gchar *detail;

  btk_calendar_get_date (BTK_CALENDAR (data->calendar_widget), &year, &month, &day);
  detail = calendar_get_detail (data, year, month, day);

  g_signal_handler_block (data->details_buffer, data->details_changed);
  btk_text_buffer_set_text (data->details_buffer, detail ? detail : "", -1);
  g_signal_handler_unblock (data->details_buffer, data->details_changed);

  g_free (detail);
}

static void
calendar_set_signal_strings (char         *sig_str,
				  CalendarData *data)
{
  const gchar *prev_sig;

  prev_sig = btk_label_get_text (BTK_LABEL (data->prev_sig));
  btk_label_set_text (BTK_LABEL (data->prev2_sig), prev_sig);

  prev_sig = btk_label_get_text (BTK_LABEL (data->last_sig));
  btk_label_set_text (BTK_LABEL (data->prev_sig), prev_sig);
  btk_label_set_text (BTK_LABEL (data->last_sig), sig_str);
}

static void
calendar_month_changed (BtkWidget    *widget,
                             CalendarData *data)
{
  char buffer[256] = "month_changed: ";

  calendar_date_to_string (data, buffer+15, 256-15);
  calendar_set_signal_strings (buffer, data);
}

static void
calendar_day_selected (BtkWidget    *widget,
                            CalendarData *data)
{
  char buffer[256] = "day_selected: ";

  calendar_date_to_string (data, buffer+14, 256-14);
  calendar_set_signal_strings (buffer, data);

  calendar_update_details (data);
}

static void
calendar_day_selected_double_click (BtkWidget    *widget,
                                         CalendarData *data)
{
  char buffer[256] = "day_selected_double_click: ";
  guint day;

  calendar_date_to_string (data, buffer+27, 256-27);
  calendar_set_signal_strings (buffer, data);

  btk_calendar_get_date (BTK_CALENDAR (data->window),
			 NULL, NULL, &day);

  if (BTK_CALENDAR (data->window)->marked_date[day-1] == 0) {
    btk_calendar_mark_day (BTK_CALENDAR (data->window), day);
  } else { 
    btk_calendar_unmark_day (BTK_CALENDAR (data->window), day);
  }
}

static void
calendar_prev_month (BtkWidget    *widget,
                          CalendarData *data)
{
  char buffer[256] = "prev_month: ";

  calendar_date_to_string (data, buffer+12, 256-12);
  calendar_set_signal_strings (buffer, data);
}

static void
calendar_next_month (BtkWidget    *widget,
                     CalendarData *data)
{
  char buffer[256] = "next_month: ";

  calendar_date_to_string (data, buffer+12, 256-12);
  calendar_set_signal_strings (buffer, data);
}

static void
calendar_prev_year (BtkWidget    *widget,
                    CalendarData *data)
{
  char buffer[256] = "prev_year: ";

  calendar_date_to_string (data, buffer+11, 256-11);
  calendar_set_signal_strings (buffer, data);
}

static void
calendar_next_year (BtkWidget    *widget,
                    CalendarData *data)
{
  char buffer[256] = "next_year: ";

  calendar_date_to_string (data, buffer+11, 256-11);
  calendar_set_signal_strings (buffer, data);
}


static void
calendar_set_flags (CalendarData *calendar)
{
  gint options = 0, i;

  for (i = 0; i < G_N_ELEMENTS (calendar->settings); i++)
    if (calendar->settings[i])
      options=options + (1 << i);

  if (calendar->window)
    btk_calendar_set_display_options (BTK_CALENDAR (calendar->window), options);
}

static void
calendar_toggle_flag (BtkWidget    *toggle,
                      CalendarData *calendar)
{
  gint i;

  for (i = 0; i < G_N_ELEMENTS (calendar->flag_checkboxes); i++)
    if (calendar->flag_checkboxes[i] == toggle)
      calendar->settings[i] = btk_toggle_button_get_active (BTK_TOGGLE_BUTTON (toggle));

  calendar_set_flags(calendar);
  
}

void calendar_select_font (BtkWidget    *button,
                                 CalendarData *calendar)
{
  const char *font = NULL;
  BtkRcStyle *style;

  if (calendar->window)
    font = btk_font_button_get_font_name (BTK_FONT_BUTTON (button));

  if (font)
	{
	  style = btk_rc_style_new ();
	  bango_font_description_free (style->font_desc);
      style->font_desc = bango_font_description_from_string (font);
	  btk_widget_modify_style (calendar->window, style);
	}
}

static gchar*
calendar_detail_cb (BtkCalendar *calendar,
                    guint        year,
                    guint        month,
                    guint        day,
                    gpointer     data)
{
  return calendar_get_detail (data, year, month, day);
}

static void
calendar_details_changed (BtkTextBuffer *buffer,
                          CalendarData  *data)
{
  BtkTextIter start, end;
  guint year, month, day;
  gchar *detail;

  btk_text_buffer_get_start_iter(buffer, &start);
  btk_text_buffer_get_end_iter(buffer, &end);

  btk_calendar_get_date (BTK_CALENDAR (data->calendar_widget), &year, &month, &day);
  detail = btk_text_buffer_get_text (buffer, &start, &end, FALSE);

  if (!detail[0])
    {
      g_free (detail);
      detail = NULL;
    }

  calendar_set_detail (data, year, month, day, detail);
  btk_widget_queue_resize (data->calendar_widget);
}

static void
demonstrate_details (CalendarData *data)
{
  static char *rainbow[] = { "#900", "#980", "#390", "#095", "#059", "#309", "#908" };
  BtkCalendar *calendar = BTK_CALENDAR (data->calendar_widget);
  gint row, col;

  for (row = 0; row < 6; ++row)
    for (col = 0; col < 7; ++col)
      {
        gint year, month, day;
        gchar *detail;
    
        year = calendar->year;
        month = calendar->month;
        month += calendar->day_month[row][col];
        day = calendar->day[row][col];
    
        if (month < 1)
          {
            month += 12;
            year -= 1;
          }
        else if (month > 12)
          {
            month -= 12;
            year += 1;
  }

        detail = g_strdup_printf ("<span color='%s'>yadda\n"
                                  "(%04d-%02d-%02d)</span>",
                                  rainbow[(day - 1) % 7],
                                  year, month, day);

        calendar_set_detail (data, year, month - 1, day, detail);
      }

  btk_widget_queue_resize (data->calendar_widget);
  calendar_update_details (data);
}

static void
reset_details (CalendarData *data)
{
  g_hash_table_remove_all (data->details_table);
  btk_widget_queue_resize (data->calendar_widget);
  calendar_update_details (data);
}

static void
calendar_toggle_details (BtkWidget    *widget,
                         CalendarData *data)
{
  if (btk_toggle_button_get_active (BTK_TOGGLE_BUTTON (widget)))
    btk_calendar_set_detail_func (BTK_CALENDAR (data->calendar_widget),
                                  calendar_detail_cb, data, NULL);
  else
    btk_calendar_set_detail_func (BTK_CALENDAR (data->calendar_widget),
                                  NULL, NULL, NULL);
}

static BtkWidget*
create_expander (const char *caption,
                 BtkWidget  *child,
                 gdouble     xscale,
                 gdouble     yscale)
{
  BtkWidget *expander = btk_expander_new ("");
  BtkWidget *label = btk_expander_get_label_widget (BTK_EXPANDER (expander));
  BtkWidget *align = btk_alignment_new (0, 0, xscale, yscale);

  btk_alignment_set_padding (BTK_ALIGNMENT (align), 6, 0, 18, 0);
  btk_label_set_markup (BTK_LABEL (label), caption);

  btk_container_add (BTK_CONTAINER (expander), align);
  btk_container_add (BTK_CONTAINER (align), child);

  return expander;
}

static BtkWidget*
create_frame (const char *caption,
              BtkWidget  *child,
              gdouble     xscale,
              gdouble     yscale)
{
  BtkWidget *frame = btk_frame_new ("");
  BtkWidget *label = btk_frame_get_label_widget (BTK_FRAME (frame));
  BtkWidget *align = btk_alignment_new (0, 0, xscale, yscale);

  btk_frame_set_shadow_type (BTK_FRAME (frame), BTK_SHADOW_NONE);
  btk_alignment_set_padding (BTK_ALIGNMENT (align), 6, 0, 18, 0);
  btk_label_set_markup (BTK_LABEL (label), caption);

  btk_container_add (BTK_CONTAINER (frame), align);
  btk_container_add (BTK_CONTAINER (align), child);

  return frame;
}

static void
detail_width_changed (BtkSpinButton *button,
                      CalendarData  *data)
{
  gint value = (gint) btk_spin_button_get_value (button);
  btk_calendar_set_detail_width_chars (BTK_CALENDAR (data->calendar_widget), value);
}

static void
detail_height_changed (BtkSpinButton *button,
                      CalendarData  *data)
{
  gint value = (gint) btk_spin_button_get_value (button);
  btk_calendar_set_detail_height_rows (BTK_CALENDAR (data->calendar_widget), value);
}

static void
create_calendar(void)
{
  static CalendarData calendar_data;

  BtkWidget *window, *hpaned, *vbox, *rpane, *hbox;
  BtkWidget *calendar, *toggle, *scroller, *button;
  BtkWidget *frame, *label, *bbox, *align, *details;

  BtkSizeGroup *size;
  BtkStyle *style;
  gchar *font;
  gint i;
  
  struct {
    gboolean init;
    char *label;
  } flags[] =
    {
      { TRUE,  "Show _Heading" },
      { TRUE,  "Show Day _Names" },
      { FALSE, "No Month _Change" },
      { TRUE,  "Show _Week Numbers" },
      { FALSE, "Week Start _Monday" },
      { TRUE,  "Show De_tails" },
    };

  calendar_data.window = NULL;
  calendar_data.font_dialog = NULL;
  calendar_data.details_table = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

  for (i = 0; i < G_N_ELEMENTS (calendar_data.settings); i++)
    calendar_data.settings[i] = 0;

  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  btk_window_set_title (BTK_WINDOW (window), "BtkCalendar Example");
  btk_container_set_border_width (BTK_CONTAINER (window), 12);
  g_signal_connect (window, "destroy",
		    G_CALLBACK (btk_main_quit),
		    NULL);
  g_signal_connect (window, "delete-event",
		    G_CALLBACK (btk_false),
		    NULL);

  hpaned = btk_hpaned_new ();

  /* Calendar widget */

  calendar = btk_calendar_new ();
  calendar_data.calendar_widget = calendar;
  frame = create_frame ("<b>Calendar</b>", calendar, 0, 0);
  btk_paned_pack1 (BTK_PANED (hpaned), frame, TRUE, FALSE);

  calendar_data.window = calendar;
  calendar_set_flags(&calendar_data);
  btk_calendar_mark_day (BTK_CALENDAR (calendar), 19);	

  g_signal_connect (calendar, "month_changed", 
		    G_CALLBACK (calendar_month_changed),
		    &calendar_data);
  g_signal_connect (calendar, "day_selected", 
		    G_CALLBACK (calendar_day_selected),
		    &calendar_data);
  g_signal_connect (calendar, "day_selected_double_click", 
		    G_CALLBACK (calendar_day_selected_double_click),
		    &calendar_data);
  g_signal_connect (calendar, "prev_month", 
		    G_CALLBACK (calendar_prev_month),
		    &calendar_data);
  g_signal_connect (calendar, "next_month", 
		    G_CALLBACK (calendar_next_month),
		    &calendar_data);
  g_signal_connect (calendar, "prev_year", 
		    G_CALLBACK (calendar_prev_year),
		    &calendar_data);
  g_signal_connect (calendar, "next_year", 
		    G_CALLBACK (calendar_next_year),
		    &calendar_data);

  rpane = btk_vbox_new (FALSE, DEF_PAD_SMALL);
  btk_paned_pack2 (BTK_PANED (hpaned), rpane, FALSE, FALSE);

  /* Build the right font-button */

  vbox = btk_vbox_new(FALSE, DEF_PAD_SMALL);
  frame = create_frame ("<b>Options</b>", vbox, 1, 0);
  btk_box_pack_start (BTK_BOX (rpane), frame, FALSE, TRUE, 0);
  size = btk_size_group_new (BTK_SIZE_GROUP_HORIZONTAL);

  btk_widget_ensure_style (calendar);
  style = btk_widget_get_style (calendar);
  font = bango_font_description_to_string (style->font_desc);
  button = btk_font_button_new_with_font (font);
  g_free (font);

  g_signal_connect (button, "font-set",
                    G_CALLBACK(calendar_select_font),
                    &calendar_data);

  label = btk_label_new_with_mnemonic ("_Font:");
  btk_label_set_mnemonic_widget (BTK_LABEL (label), button);
  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
  btk_size_group_add_widget (size, label);

  hbox = btk_hbox_new (FALSE, DEF_PAD_SMALL);
  btk_box_pack_start (BTK_BOX (hbox), label, FALSE, TRUE, 0);
  btk_box_pack_start (BTK_BOX (hbox), button, FALSE, TRUE, 0);
  btk_box_pack_start (BTK_BOX (vbox), hbox, FALSE, TRUE, 0);

  /* Build the width entry */

  button = btk_spin_button_new_with_range (0, 127, 1);
  btk_spin_button_set_value (BTK_SPIN_BUTTON (button),
                             btk_calendar_get_detail_width_chars (BTK_CALENDAR (calendar)));

  g_signal_connect (button, "value-changed",
                    G_CALLBACK (detail_width_changed),
                    &calendar_data);

  label = btk_label_new_with_mnemonic ("Details W_idth:");
  btk_label_set_mnemonic_widget (BTK_LABEL (label), button);
  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
  btk_size_group_add_widget (size, label);

  hbox = btk_hbox_new (FALSE, DEF_PAD_SMALL);
  btk_box_pack_start (BTK_BOX (hbox), label, FALSE, TRUE, 0);
  btk_box_pack_start (BTK_BOX (hbox), button, FALSE, TRUE, 0);
  btk_box_pack_start (BTK_BOX (vbox), hbox, FALSE, TRUE, 0);

  /* Build the height entry */

  button = btk_spin_button_new_with_range (0, 127, 1);
  btk_spin_button_set_value (BTK_SPIN_BUTTON (button),
                             btk_calendar_get_detail_height_rows (BTK_CALENDAR (calendar)));

  g_signal_connect (button, "value-changed",
                    G_CALLBACK (detail_height_changed),
                    &calendar_data);

  label = btk_label_new_with_mnemonic ("Details H_eight:");
  btk_label_set_mnemonic_widget (BTK_LABEL (label), button);
  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
  btk_size_group_add_widget (size, label);

  hbox = btk_hbox_new (FALSE, DEF_PAD_SMALL);
  btk_box_pack_start (BTK_BOX (hbox), label, FALSE, TRUE, 0);
  btk_box_pack_start (BTK_BOX (hbox), button, FALSE, TRUE, 0);
  btk_box_pack_start (BTK_BOX (vbox), hbox, FALSE, TRUE, 0);

  /* Build the right details frame */

  vbox = btk_vbox_new(FALSE, DEF_PAD_SMALL);
  frame = create_frame ("<b>Details</b>", vbox, 1, 1);
  btk_box_pack_start (BTK_BOX (rpane), frame, FALSE, TRUE, 0);

  details = btk_text_view_new();
  calendar_data.details_buffer = btk_text_view_get_buffer (BTK_TEXT_VIEW (details));

  calendar_data.details_changed = g_signal_connect (calendar_data.details_buffer, "changed",
                                                    G_CALLBACK (calendar_details_changed),
                                                    &calendar_data);

  scroller = btk_scrolled_window_new (NULL, NULL);
  btk_container_add (BTK_CONTAINER (scroller), details);

  btk_scrolled_window_set_shadow_type (BTK_SCROLLED_WINDOW (scroller),
                                       BTK_SHADOW_IN);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (scroller),
                                  BTK_POLICY_AUTOMATIC,
                                  BTK_POLICY_AUTOMATIC);

  btk_box_pack_start (BTK_BOX (vbox), scroller, FALSE, TRUE, 0);

  hbox = btk_hbox_new (FALSE, DEF_PAD_SMALL);
  align = btk_alignment_new (0.0, 0.5, 0.0, 0.0);
  btk_container_add (BTK_CONTAINER (align), hbox);
  btk_box_pack_start (BTK_BOX (vbox), align, FALSE, TRUE, 0);

  button = btk_button_new_with_mnemonic ("Demonstrate _Details");

  g_signal_connect_swapped (button,
                            "clicked",
                            G_CALLBACK (demonstrate_details),
                            &calendar_data);

  btk_box_pack_start (BTK_BOX (hbox), button, FALSE, TRUE, 0);

  button = btk_button_new_with_mnemonic ("_Reset Details");

  g_signal_connect_swapped (button,
                            "clicked",
                            G_CALLBACK (reset_details),
                            &calendar_data);

  btk_box_pack_start (BTK_BOX (hbox), button, FALSE, TRUE, 0);

  toggle = btk_check_button_new_with_mnemonic ("_Use Details");
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK(calendar_toggle_details),
                    &calendar_data);
  btk_box_pack_start (BTK_BOX (vbox), toggle, FALSE, TRUE, 0);
  
  /* Build the Right frame with the flags in */ 

  vbox = btk_vbox_new(FALSE, 0);
  frame = create_expander ("<b>Flags</b>", vbox, 1, 0);
  btk_box_pack_start (BTK_BOX (rpane), frame, TRUE, TRUE, 0);

  for (i = 0; i < G_N_ELEMENTS (calendar_data.settings); i++)
    {
      toggle = btk_check_button_new_with_mnemonic(flags[i].label);
      btk_box_pack_start (BTK_BOX (vbox), toggle, FALSE, TRUE, 0);
      calendar_data.flag_checkboxes[i] = toggle;

      g_signal_connect (toggle, "toggled",
                        G_CALLBACK (calendar_toggle_flag),
			&calendar_data);

      btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (toggle), flags[i].init);
    }

  /*
   *  Build the Signal-event part.
   */

  vbox = btk_vbox_new (TRUE, DEF_PAD_SMALL);
  frame = create_frame ("<b>Signal Events</b>", vbox, 1, 0);
  
  hbox = btk_hbox_new (FALSE, 3);
  btk_box_pack_start (BTK_BOX (vbox), hbox, FALSE, TRUE, 0);
  label = btk_label_new ("Signal:");
  btk_box_pack_start (BTK_BOX (hbox), label, FALSE, TRUE, 0);
  calendar_data.last_sig = btk_label_new ("");
  btk_box_pack_start (BTK_BOX (hbox), calendar_data.last_sig, FALSE, TRUE, 0);

  hbox = btk_hbox_new (FALSE, 3);
  btk_box_pack_start (BTK_BOX (vbox), hbox, FALSE, TRUE, 0);
  label = btk_label_new ("Previous signal:");
  btk_box_pack_start (BTK_BOX (hbox), label, FALSE, TRUE, 0);
  calendar_data.prev_sig = btk_label_new ("");
  btk_box_pack_start (BTK_BOX (hbox), calendar_data.prev_sig, FALSE, TRUE, 0);

  hbox = btk_hbox_new (FALSE, 3);
  btk_box_pack_start (BTK_BOX (vbox), hbox, FALSE, TRUE, 0);
  label = btk_label_new ("Second previous signal:");
  btk_box_pack_start (BTK_BOX (hbox), label, FALSE, TRUE, 0);
  calendar_data.prev2_sig = btk_label_new ("");
  btk_box_pack_start (BTK_BOX (hbox), calendar_data.prev2_sig, FALSE, TRUE, 0);

  /*
   *  Glue everything together
   */

  bbox = btk_hbutton_box_new ();
  btk_button_box_set_layout(BTK_BUTTON_BOX(bbox), BTK_BUTTONBOX_END);

  button = btk_button_new_with_label ("Close");
  g_signal_connect (button, "clicked", G_CALLBACK (btk_main_quit), NULL);
  btk_container_add (BTK_CONTAINER (bbox), button);

  vbox = btk_vbox_new (FALSE, DEF_PAD_SMALL);

  btk_box_pack_start (BTK_BOX (vbox), hpaned,                TRUE,  TRUE, 0);
  btk_box_pack_start (BTK_BOX (vbox), btk_hseparator_new (), FALSE, TRUE, 0);
  btk_box_pack_start (BTK_BOX (vbox), frame,                 FALSE, TRUE, 0);
  btk_box_pack_start (BTK_BOX (vbox), btk_hseparator_new (), FALSE, TRUE, 0);
  btk_box_pack_start (BTK_BOX (vbox), bbox,                  FALSE, TRUE, 0);

  btk_container_add (BTK_CONTAINER (window), vbox);

  btk_widget_set_can_default (button, TRUE);
  btk_widget_grab_default (button);

  btk_window_set_default_size (BTK_WINDOW (window), 600, 0);
  btk_widget_show_all (window);
}


int main(int   argc,
         char *argv[] )
{
  btk_init (&argc, &argv);

  create_calendar();

  btk_main();

  return(0);
}
/* example-end */
