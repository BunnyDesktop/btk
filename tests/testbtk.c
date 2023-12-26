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

#undef BTK_DISABLE_DEPRECATED

#undef BDK_DISABLE_DEPRECATED

#include "config.h"

#undef	G_LOG_DOMAIN

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#define BTK_ENABLE_BROKEN
#include "btk/btk.h"
#include "bdk/bdk.h"
#include "bdk/bdkkeysyms.h"
#include "bunnylib/gstdio.h"

#ifdef G_OS_WIN32
#define sleep(n) _sleep(n)
#endif

#include "prop-editor.h"

#include "circles.xbm"
#include "test.xpm"

gboolean
file_exists (const char *filename)
{
  GStatBuf statbuf;

  return g_stat (filename, &statbuf) == 0;
}

BtkWidget *
shape_create_icon (BdkScreen *screen,
		   char      *xpm_file,
		   gint       x,
		   gint       y,
		   gint       px,
		   gint       py,
		   gint       window_type);

static BtkWidget *
build_option_menu (gchar           *items[],
		   gint             num_items,
		   gint             history,
		   void           (*func) (BtkWidget *widget, gpointer data),
		   gpointer         data);

/* macro, structure and variables used by tree window demos */
#define DEFAULT_NUMBER_OF_ITEM  3
#define DEFAULT_RECURSION_LEVEL 3

struct {
  GSList* selection_mode_group;
  BtkWidget* single_button;
  BtkWidget* browse_button;
  BtkWidget* multiple_button;
  BtkWidget* draw_line_button;
  BtkWidget* view_line_button;
  BtkWidget* no_root_item_button;
  BtkWidget* nb_item_spinner;
  BtkWidget* recursion_spinner;
} sTreeSampleSelection;

typedef struct sTreeButtons {
  guint nb_item_add;
  BtkWidget* add_button;
  BtkWidget* remove_button;
  BtkWidget* subtree_button;
} sTreeButtons;
/* end of tree section */

static BtkWidget *
build_option_menu (gchar           *items[],
		   gint             num_items,
		   gint             history,
		   void           (*func)(BtkWidget *widget, gpointer data),
		   gpointer         data)
{
  BtkWidget *omenu;
  BtkWidget *menu;
  GSList *group;
  gint i;

  omenu = btk_combo_box_text_new ();
  g_signal_connect (omenu, "changed",
		    G_CALLBACK (func), data);
      
  menu = btk_menu_new ();
  group = NULL;
  
  for (i = 0; i < num_items; i++)
      btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (omenu), items[i]);

  btk_option_menu_set_menu (BTK_OPTION_MENU (omenu), menu);
  btk_option_menu_set_history (BTK_OPTION_MENU (omenu), history);
  
  return omenu;
}

static void
destroy_tooltips (BtkWidget *widget, BtkWindow **window)
{
  BtkTooltips *tt = g_object_get_data (B_OBJECT (*window), "tooltips");
  g_object_unref (tt);
  *window = NULL;
}


/*
 * Windows with an alpha channel
 */


static gboolean
on_alpha_window_expose (BtkWidget      *widget,
			BdkEventExpose *expose)
{
  bairo_t *cr;
  bairo_pattern_t *pattern;
  int radius;

  cr = bdk_bairo_create (widget->window);

  radius = MIN (widget->allocation.width, widget->allocation.height) / 2;
  pattern = bairo_pattern_create_radial (widget->allocation.width / 2,
					 widget->allocation.height / 2,
					 0.0,
					 widget->allocation.width / 2,
					 widget->allocation.height / 2,
					 radius * 1.33);

  if (bdk_screen_get_rgba_colormap (btk_widget_get_screen (widget)) &&
      btk_widget_is_composited (widget))
    bairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.0); /* transparent */
  else
    bairo_set_source_rgb (cr, 1.0, 1.0, 1.0); /* opaque white */
    
  bairo_set_operator (cr, BAIRO_OPERATOR_SOURCE);
  bairo_paint (cr);
  
  bairo_pattern_add_color_stop_rgba (pattern, 0.0,
				     1.0, 0.75, 0.0, 1.0); /* solid orange */
  bairo_pattern_add_color_stop_rgba (pattern, 1.0,
				     1.0, 0.75, 0.0, 0.0); /* transparent orange */

  bairo_set_source (cr, pattern);
  bairo_pattern_destroy (pattern);
  
  bairo_set_operator (cr, BAIRO_OPERATOR_OVER);
  bairo_paint (cr);

  bairo_destroy (cr);

  return FALSE;
}

static BtkWidget *
build_alpha_widgets (void)
{
  BtkWidget *table;
  BtkWidget *radio_button;
  BtkWidget *hbox;
  BtkWidget *label;
  BtkWidget *entry;

  table = btk_table_new (1, 1, FALSE);

  radio_button = btk_radio_button_new_with_label (NULL, "Red");
  btk_table_attach (BTK_TABLE (table),
		    radio_button,
		    0, 1,                  0, 1,
		    BTK_EXPAND | BTK_FILL, 0,
		    0,                     0);

  radio_button = btk_radio_button_new_with_label_from_widget (BTK_RADIO_BUTTON (radio_button), "Green");
  btk_table_attach (BTK_TABLE (table),
		    radio_button,
		    0, 1,                  1, 2,
		    BTK_EXPAND | BTK_FILL, 0,
		    0,                     0);

  radio_button = btk_radio_button_new_with_label_from_widget (BTK_RADIO_BUTTON (radio_button), "Blue"),
  btk_table_attach (BTK_TABLE (table),
		    radio_button,
		    0, 1,                  2, 3,
		    BTK_EXPAND | BTK_FILL, 0,
		    0,                     0);

  btk_table_attach (BTK_TABLE (table),
		    btk_check_button_new_with_label ("Sedentary"),
		    1, 2,                  0, 1,
		    BTK_EXPAND | BTK_FILL, 0,
		    0,                     0);
  btk_table_attach (BTK_TABLE (table),
		    btk_check_button_new_with_label ("Nocturnal"),
		    1, 2,                  1, 2,
		    BTK_EXPAND | BTK_FILL, 0,
		    0,                     0);
  btk_table_attach (BTK_TABLE (table),
		    btk_check_button_new_with_label ("Compulsive"),
		    1, 2,                  2, 3,
		    BTK_EXPAND | BTK_FILL, 0,
		    0,                     0);

  radio_button = btk_radio_button_new_with_label_from_widget (BTK_RADIO_BUTTON (radio_button), "Green");
  btk_table_attach (BTK_TABLE (table),
		    radio_button,
		    0, 1,                  1, 2,
		    BTK_EXPAND | BTK_FILL, 0,
		    0,                     0);

  radio_button = btk_radio_button_new_with_label_from_widget (BTK_RADIO_BUTTON (radio_button), "Blue"),
  btk_table_attach (BTK_TABLE (table),
		    radio_button,
		    0, 1,                  2, 3,
		    BTK_EXPAND | BTK_FILL, 0,
		    0,                     0);
  
  hbox = btk_hbox_new (FALSE, 0);
  label = btk_label_new (NULL);
  btk_label_set_markup (BTK_LABEL (label), "<i>Entry: </i>");
  btk_box_pack_start (BTK_BOX (hbox), label, FALSE, FALSE, 0);
  entry = btk_entry_new ();
  btk_box_pack_start (BTK_BOX (hbox), entry, TRUE, TRUE, 0);
  btk_table_attach (BTK_TABLE (table),
		    hbox,
		    0, 1,                  3, 4,
		    BTK_EXPAND | BTK_FILL, 0,
		    0,                     0);
  
  return table;
}

static void
on_alpha_screen_changed (BtkWidget *widget,
			 BdkScreen *old_screen,
			 BtkWidget *label)
{
  BdkScreen *screen = btk_widget_get_screen (widget);
  BdkColormap *colormap = bdk_screen_get_rgba_colormap (screen);

  if (!colormap)
    {
      colormap = bdk_screen_get_default_colormap (screen);
      btk_label_set_markup (BTK_LABEL (label), "<b>Screen doesn't support alpha</b>");
    }
  else
    {
      btk_label_set_markup (BTK_LABEL (label), "<b>Screen supports alpha</b>");
    }

  btk_widget_set_colormap (widget, colormap);
}

static void
on_composited_changed (BtkWidget *window,
		      BtkLabel *label)
{
  gboolean is_composited = btk_widget_is_composited (window);

  if (is_composited)
    btk_label_set_text (label, "Composited");
  else
    btk_label_set_text (label, "Not composited");
}

void
create_alpha_window (BtkWidget *widget)
{
  static BtkWidget *window;

  if (!window)
    {
      BtkWidget *vbox;
      BtkWidget *label;
      
      window = btk_dialog_new_with_buttons ("Alpha Window",
					    BTK_WINDOW (btk_widget_get_toplevel (widget)), 0,
					    BTK_STOCK_CLOSE, 0,
					    NULL);

      btk_widget_set_app_paintable (window, TRUE);
      g_signal_connect (window, "expose-event",
			G_CALLBACK (on_alpha_window_expose), NULL);
      
      vbox = btk_vbox_new (FALSE, 8);
      btk_container_set_border_width (BTK_CONTAINER (vbox), 12);
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->vbox), vbox,
			  TRUE, TRUE, 0);

      label = btk_label_new (NULL);
      btk_box_pack_start (BTK_BOX (vbox), label, TRUE, TRUE, 0);
      on_alpha_screen_changed (window, NULL, label);
      g_signal_connect (window, "screen-changed",
			G_CALLBACK (on_alpha_screen_changed), label);
      
      label = btk_label_new (NULL);
      btk_box_pack_start (BTK_BOX (vbox), label, TRUE, TRUE, 0);
      on_composited_changed (window, BTK_LABEL (label));
      g_signal_connect (window, "composited_changed", G_CALLBACK (on_composited_changed), label);
      
      btk_box_pack_start (BTK_BOX (vbox), build_alpha_widgets (), TRUE, TRUE, 0);

      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window);
      
      g_signal_connect (window, "response",
                        G_CALLBACK (btk_widget_destroy),
                        NULL); 
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    btk_widget_destroy (window);
}

/*
 * Composited non-toplevel window
 */

/* The expose event handler for the event box.
 *
 * This function simply draws a transparency onto a widget on the area
 * for which it receives expose events.  This is intended to give the
 * event box a "transparent" background.
 *
 * In order for this to work properly, the widget must have an RGBA
 * colourmap.  The widget should also be set as app-paintable since it
 * doesn't make sense for BTK to draw a background if we are drawing it
 * (and because BTK might actually replace our transparency with its
 * default background colour).
 */
static gboolean
transparent_expose (BtkWidget *widget,
                    BdkEventExpose *event)
{
  bairo_t *cr;

  cr = bdk_bairo_create (widget->window);
  bairo_set_operator (cr, BAIRO_OPERATOR_CLEAR);
  bdk_bairo_rebunnyion (cr, event->rebunnyion);
  bairo_fill (cr);
  bairo_destroy (cr);

  return FALSE;
}

/* The expose event handler for the window.
 *
 * This function performs the actual compositing of the event box onto
 * the already-existing background of the window at 50% normal opacity.
 *
 * In this case we do not want app-paintable to be set on the widget
 * since we want it to draw its own (red) background.  Because of this,
 * however, we must ensure that we use g_signal_register_after so that
 * this handler is called after the red has been drawn.  If it was
 * called before then BTK would just blindly paint over our work.
 */
static gboolean
window_expose_event (BtkWidget *widget,
                     BdkEventExpose *event)
{
  BdkRebunnyion *rebunnyion;
  BtkWidget *child;
  bairo_t *cr;

  /* get our child (in this case, the event box) */ 
  child = btk_bin_get_child (BTK_BIN (widget));

  /* create a bairo context to draw to the window */
  cr = bdk_bairo_create (widget->window);

  /* the source data is the (composited) event box */
  bdk_bairo_set_source_pixmap (cr, child->window,
                               child->allocation.x,
                               child->allocation.y);

  /* draw no more than our expose event intersects our child */
  rebunnyion = bdk_rebunnyion_rectangle (&child->allocation);
  bdk_rebunnyion_intersect (rebunnyion, event->rebunnyion);
  bdk_bairo_rebunnyion (cr, rebunnyion);
  bairo_clip (cr);

  /* composite, with a 50% opacity */
  bairo_set_operator (cr, BAIRO_OPERATOR_OVER);
  bairo_paint_with_alpha (cr, 0.5);

  /* we're done */
  bairo_destroy (cr);

  return FALSE;
}

void
create_composited_window (BtkWidget *widget)
{
  static BtkWidget *window;

  if (!window)
    {
      BtkWidget *event, *button;
      BdkScreen *screen;
      BdkColormap *rgba;
      BdkColor red;

      /* make the widgets */
      button = btk_button_new_with_label ("A Button");
      event = btk_event_box_new ();
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);

      g_signal_connect (window, "destroy",
                        G_CALLBACK (btk_widget_destroyed),
                        &window);

      /* put a red background on the window */
      bdk_color_parse ("red", &red);
      btk_widget_modify_bg (window, BTK_STATE_NORMAL, &red);

      /* set the colourmap for the event box.
       * must be done before the event box is realised.
       */
      screen = btk_widget_get_screen (event);
      rgba = bdk_screen_get_rgba_colormap (screen);
      btk_widget_set_colormap (event, rgba);

      /* set our event box to have a fully-transparent background
       * drawn on it.  currently there is no way to simply tell btk
       * that "transparency" is the background colour for a widget.
       */
      btk_widget_set_app_paintable (BTK_WIDGET (event), TRUE);
      g_signal_connect (event, "expose-event",
                        G_CALLBACK (transparent_expose), NULL);

      /* put them inside one another */
      btk_container_set_border_width (BTK_CONTAINER (window), 10);
      btk_container_add (BTK_CONTAINER (window), event);
      btk_container_add (BTK_CONTAINER (event), button);

      /* realise and show everything */
      btk_widget_realize (button);

      /* set the event box BdkWindow to be composited.
       * obviously must be performed after event box is realised.
       */
      bdk_window_set_composited (event->window, TRUE);

      /* set up the compositing handler.
       * note that we do _after so that the normal (red) background is drawn
       * by btk before our compositing occurs.
       */
      g_signal_connect_after (window, "expose-event",
                              G_CALLBACK (window_expose_event), NULL);
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    btk_widget_destroy (window);
}

/*
 * Big windows and guffaw scrolling
 */

static gboolean
pattern_expose (BtkWidget      *widget,
		BdkEventExpose *event,
		gpointer        data)
{
  BdkColor *color;
  BdkWindow *window = event->window;

  color = g_object_get_data (B_OBJECT (window), "pattern-color");
  if (color)
    {
      bairo_t *cr = bdk_bairo_create (window);

      bdk_bairo_set_source_color (cr, color);
      bdk_bairo_rectangle (cr, &event->area);
      bairo_fill (cr);

      bairo_destroy (cr);
    }

  return FALSE;
}

static void
pattern_set_bg (BtkWidget   *widget,
		BdkWindow   *child,
		gint         level)
{
  static const BdkColor colors[] = {
    { 0, 0x4444, 0x4444, 0xffff },
    { 0, 0x8888, 0x8888, 0xffff },
    { 0, 0xaaaa, 0xaaaa, 0xffff }
  };
    
  g_object_set_data (B_OBJECT (child), "pattern-color", (gpointer) &colors[level]);
  bdk_window_set_user_data (child, widget);
}

static void
create_pattern (BtkWidget   *widget,
		BdkWindow   *parent,
		gint         level,
		gint         width,
		gint         height)
{
  gint h = 1;
  gint i = 0;
    
  BdkWindow *child;

  while (2 * h <= height)
    {
      gint w = 1;
      gint j = 0;
      
      while (2 * w <= width)
	{
	  if ((i + j) % 2 == 0)
	    {
	      gint x = w  - 1;
	      gint y = h - 1;
	      
	      BdkWindowAttr attributes;

	      attributes.window_type = BDK_WINDOW_CHILD;
	      attributes.x = x;
	      attributes.y = y;
	      attributes.width = w;
	      attributes.height = h;
	      attributes.wclass = BDK_INPUT_OUTPUT;
	      attributes.event_mask = BDK_EXPOSURE_MASK;
	      attributes.visual = btk_widget_get_visual (widget);
	      attributes.colormap = btk_widget_get_colormap (widget);
	      
	      child = bdk_window_new (parent, &attributes,
				      BDK_WA_X | BDK_WA_Y | BDK_WA_VISUAL | BDK_WA_COLORMAP);

	      pattern_set_bg (widget, child, level);

	      if (level < 2)
		create_pattern (widget, child, level + 1, w, h);

	      bdk_window_show (child);
	    }
	  j++;
	  w *= 2;
	}
      i++;
      h *= 2;
    }
}

#define PATTERN_SIZE (1 << 18)

static void
pattern_hadj_changed (BtkAdjustment *adj,
		      BtkWidget     *darea)
{
  gint *old_value = g_object_get_data (B_OBJECT (adj), "old-value");
  gint new_value = adj->value;

  if (btk_widget_get_realized (darea))
    {
      bdk_window_scroll (darea->window, *old_value - new_value, 0);
      *old_value = new_value;
    }
}

static void
pattern_vadj_changed (BtkAdjustment *adj,
		      BtkWidget *darea)
{
  gint *old_value = g_object_get_data (B_OBJECT (adj), "old-value");
  gint new_value = adj->value;

  if (btk_widget_get_realized (darea))
    {
      bdk_window_scroll (darea->window, 0, *old_value - new_value);
      *old_value = new_value;
    }
}

static void
pattern_realize (BtkWidget *widget,
		 gpointer   data)
{
  pattern_set_bg (widget, widget->window, 0);
  create_pattern (widget, widget->window, 1, PATTERN_SIZE, PATTERN_SIZE);
}

static void 
create_big_windows (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  BtkWidget *darea, *table, *scrollbar;
  BtkWidget *eventbox;
  BtkAdjustment *hadj;
  BtkAdjustment *vadj;
  static gint current_x;
  static gint current_y;
 
  if (!window)
    {
      current_x = 0;
      current_y = 0;
      
      window = btk_dialog_new_with_buttons ("Big Windows",
                                            NULL, 0,
                                            BTK_STOCK_CLOSE,
                                            BTK_RESPONSE_NONE,
                                            NULL);
 
      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));

      btk_window_set_default_size (BTK_WINDOW (window), 200, 300);

      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window);

      g_signal_connect (window, "response",
                        G_CALLBACK (btk_widget_destroy),
                        NULL);

      table = btk_table_new (2, 2, FALSE);
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->vbox),
			  table, TRUE, TRUE, 0);

      darea = btk_drawing_area_new ();

      hadj = (BtkAdjustment *)btk_adjustment_new (0, 0, PATTERN_SIZE, 10, 100, 100);
      g_signal_connect (hadj, "value_changed",
			G_CALLBACK (pattern_hadj_changed), darea);
      g_object_set_data (B_OBJECT (hadj), "old-value", &current_x);
      
      vadj = (BtkAdjustment *)btk_adjustment_new (0, 0, PATTERN_SIZE, 10, 100, 100);
      g_signal_connect (vadj, "value_changed",
			G_CALLBACK (pattern_vadj_changed), darea);
      g_object_set_data (B_OBJECT (vadj), "old-value", &current_y);
      
      g_signal_connect (darea, "realize",
                        G_CALLBACK (pattern_realize),
                        NULL);
      g_signal_connect (darea, "expose_event",
                        G_CALLBACK (pattern_expose),
                        NULL);

      eventbox = btk_event_box_new ();
      btk_table_attach (BTK_TABLE (table), eventbox,
			0, 1,                  0, 1,
			BTK_FILL | BTK_EXPAND, BTK_FILL | BTK_EXPAND,
			0,                     0);

      btk_container_add (BTK_CONTAINER (eventbox), darea);

      scrollbar = btk_hscrollbar_new (hadj);
      btk_table_attach (BTK_TABLE (table), scrollbar,
			0, 1,                  1, 2,
			BTK_FILL | BTK_EXPAND, BTK_FILL,
			0,                     0);

      scrollbar = btk_vscrollbar_new (vadj);
      btk_table_attach (BTK_TABLE (table), scrollbar,
			1, 2,                  0, 1,
			BTK_FILL,              BTK_EXPAND | BTK_FILL,
			0,                     0);

    }

  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    btk_widget_hide (window);
}

/*
 * BtkButton
 */

static void
button_window (BtkWidget *widget,
	       BtkWidget *button)
{
  if (!btk_widget_get_visible (button))
    btk_widget_show (button);
  else
    btk_widget_hide (button);
}

static void
create_buttons (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  BtkWidget *box1;
  BtkWidget *box2;
  BtkWidget *table;
  BtkWidget *button[10];
  BtkWidget *separator;

  if (!window)
    {
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window);

      btk_window_set_title (BTK_WINDOW (window), "BtkButton");
      btk_container_set_border_width (BTK_CONTAINER (window), 0);

      box1 = btk_vbox_new (FALSE, 0);
      btk_container_add (BTK_CONTAINER (window), box1);

      table = btk_table_new (3, 3, FALSE);
      btk_table_set_row_spacings (BTK_TABLE (table), 5);
      btk_table_set_col_spacings (BTK_TABLE (table), 5);
      btk_container_set_border_width (BTK_CONTAINER (table), 10);
      btk_box_pack_start (BTK_BOX (box1), table, TRUE, TRUE, 0);

      button[0] = btk_button_new_with_label ("button1");
      button[1] = btk_button_new_with_mnemonic ("_button2");
      button[2] = btk_button_new_with_mnemonic ("_button3");
      button[3] = btk_button_new_from_stock (BTK_STOCK_OK);
      button[4] = btk_button_new_with_label ("button5");
      button[5] = btk_button_new_with_label ("button6");
      button[6] = btk_button_new_with_label ("button7");
      button[7] = btk_button_new_from_stock (BTK_STOCK_CLOSE);
      button[8] = btk_button_new_with_label ("button9");
      
      g_signal_connect (button[0], "clicked",
			G_CALLBACK (button_window),
			button[1]);

      btk_table_attach (BTK_TABLE (table), button[0], 0, 1, 0, 1,
			BTK_EXPAND | BTK_FILL, BTK_EXPAND | BTK_FILL, 0, 0);

      g_signal_connect (button[1], "clicked",
			G_CALLBACK (button_window),
			button[2]);

      btk_table_attach (BTK_TABLE (table), button[1], 1, 2, 1, 2,
			BTK_EXPAND | BTK_FILL, BTK_EXPAND | BTK_FILL, 0, 0);

      g_signal_connect (button[2], "clicked",
			G_CALLBACK (button_window),
			button[3]);
      btk_table_attach (BTK_TABLE (table), button[2], 2, 3, 2, 3,
			BTK_EXPAND | BTK_FILL, BTK_EXPAND | BTK_FILL, 0, 0);

      g_signal_connect (button[3], "clicked",
			G_CALLBACK (button_window),
			button[4]);
      btk_table_attach (BTK_TABLE (table), button[3], 0, 1, 2, 3,
			BTK_EXPAND | BTK_FILL, BTK_EXPAND | BTK_FILL, 0, 0);

      g_signal_connect (button[4], "clicked",
			G_CALLBACK (button_window),
			button[5]);
      btk_table_attach (BTK_TABLE (table), button[4], 2, 3, 0, 1,
			BTK_EXPAND | BTK_FILL, BTK_EXPAND | BTK_FILL, 0, 0);

      g_signal_connect (button[5], "clicked",
			G_CALLBACK (button_window),
			button[6]);
      btk_table_attach (BTK_TABLE (table), button[5], 1, 2, 2, 3,
			BTK_EXPAND | BTK_FILL, BTK_EXPAND | BTK_FILL, 0, 0);

      g_signal_connect (button[6], "clicked",
			G_CALLBACK (button_window),
			button[7]);
      btk_table_attach (BTK_TABLE (table), button[6], 1, 2, 0, 1,
			BTK_EXPAND | BTK_FILL, BTK_EXPAND | BTK_FILL, 0, 0);

      g_signal_connect (button[7], "clicked",
			G_CALLBACK (button_window),
			button[8]);
      btk_table_attach (BTK_TABLE (table), button[7], 2, 3, 1, 2,
			BTK_EXPAND | BTK_FILL, BTK_EXPAND | BTK_FILL, 0, 0);

      g_signal_connect (button[8], "clicked",
			G_CALLBACK (button_window),
			button[0]);
      btk_table_attach (BTK_TABLE (table), button[8], 0, 1, 1, 2,
			BTK_EXPAND | BTK_FILL, BTK_EXPAND | BTK_FILL, 0, 0);

      separator = btk_hseparator_new ();
      btk_box_pack_start (BTK_BOX (box1), separator, FALSE, TRUE, 0);

      box2 = btk_vbox_new (FALSE, 10);
      btk_container_set_border_width (BTK_CONTAINER (box2), 10);
      btk_box_pack_start (BTK_BOX (box1), box2, FALSE, TRUE, 0);

      button[9] = btk_button_new_with_label ("close");
      g_signal_connect_swapped (button[9], "clicked",
				G_CALLBACK (btk_widget_destroy),
				window);
      btk_box_pack_start (BTK_BOX (box2), button[9], TRUE, TRUE, 0);
      btk_widget_set_can_default (button[9], TRUE);
      btk_widget_grab_default (button[9]);
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    btk_widget_destroy (window);
}

/*
 * BtkToggleButton
 */

static void
create_toggle_buttons (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  BtkWidget *box1;
  BtkWidget *box2;
  BtkWidget *button;
  BtkWidget *separator;

  if (!window)
    {
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window);

      btk_window_set_title (BTK_WINDOW (window), "BtkToggleButton");
      btk_container_set_border_width (BTK_CONTAINER (window), 0);

      box1 = btk_vbox_new (FALSE, 0);
      btk_container_add (BTK_CONTAINER (window), box1);

      box2 = btk_vbox_new (FALSE, 10);
      btk_container_set_border_width (BTK_CONTAINER (box2), 10);
      btk_box_pack_start (BTK_BOX (box1), box2, TRUE, TRUE, 0);

      button = btk_toggle_button_new_with_label ("button1");
      btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);

      button = btk_toggle_button_new_with_label ("button2");
      btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);

      button = btk_toggle_button_new_with_label ("button3");
      btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);

      button = btk_toggle_button_new_with_label ("inconsistent");
      btk_toggle_button_set_inconsistent (BTK_TOGGLE_BUTTON (button), TRUE);
      btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);
      
      separator = btk_hseparator_new ();
      btk_box_pack_start (BTK_BOX (box1), separator, FALSE, TRUE, 0);

      box2 = btk_vbox_new (FALSE, 10);
      btk_container_set_border_width (BTK_CONTAINER (box2), 10);
      btk_box_pack_start (BTK_BOX (box1), box2, FALSE, TRUE, 0);

      button = btk_button_new_with_label ("close");
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (btk_widget_destroy),
				window);
      btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);
      btk_widget_set_can_default (button, TRUE);
      btk_widget_grab_default (button);
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    btk_widget_destroy (window);
}

static BtkWidget *
create_widget_grid (GType widget_type)
{
  BtkWidget *table;
  BtkWidget *group_widget = NULL;
  gint i, j;
  
  table = btk_table_new (FALSE, 3, 3);
  
  for (i = 0; i < 5; i++)
    {
      for (j = 0; j < 5; j++)
	{
	  BtkWidget *widget;
	  char *tmp;
	  
	  if (i == 0 && j == 0)
	    {
	      widget = NULL;
	    }
	  else if (i == 0)
	    {
	      tmp = g_strdup_printf ("%d", j);
	      widget = btk_label_new (tmp);
	      g_free (tmp);
	    }
	  else if (j == 0)
	    {
	      tmp = g_strdup_printf ("%c", 'A' + i - 1);
	      widget = btk_label_new (tmp);
	      g_free (tmp);
	    }
	  else
	    {
	      widget = g_object_new (widget_type, NULL);
	      
	      if (g_type_is_a (widget_type, BTK_TYPE_RADIO_BUTTON))
		{
		  if (!group_widget)
		    group_widget = widget;
		  else
		    g_object_set (widget, "group", group_widget, NULL);
		}
	    }
	  
	  if (widget)
	    btk_table_attach (BTK_TABLE (table), widget,
			      i, i + 1, j, j + 1,
			      0,        0,
			      0,        0);
	}
    }

  return table;
}

/*
 * BtkCheckButton
 */

static void
create_check_buttons (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  BtkWidget *box1;
  BtkWidget *box2;
  BtkWidget *button;
  BtkWidget *separator;
  BtkWidget *table;
  
  if (!window)
    {
      window = btk_dialog_new_with_buttons ("Check Buttons",
                                            NULL, 0,
                                            BTK_STOCK_CLOSE,
                                            BTK_RESPONSE_NONE,
                                            NULL);

      btk_window_set_screen (BTK_WINDOW (window), 
			     btk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window);
      g_signal_connect (window, "response",
                        G_CALLBACK (btk_widget_destroy),
                        NULL);

      box1 = BTK_DIALOG (window)->vbox;
      
      box2 = btk_vbox_new (FALSE, 10);
      btk_container_set_border_width (BTK_CONTAINER (box2), 10);
      btk_box_pack_start (BTK_BOX (box1), box2, TRUE, TRUE, 0);

      button = btk_check_button_new_with_mnemonic ("_button1");
      btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);

      button = btk_check_button_new_with_label ("button2");
      btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);

      button = btk_check_button_new_with_label ("button3");
      btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);

      button = btk_check_button_new_with_label ("inconsistent");
      btk_toggle_button_set_inconsistent (BTK_TOGGLE_BUTTON (button), TRUE);
      btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);

      separator = btk_hseparator_new ();
      btk_box_pack_start (BTK_BOX (box1), separator, FALSE, TRUE, 0);

      table = create_widget_grid (BTK_TYPE_CHECK_BUTTON);
      btk_container_set_border_width (BTK_CONTAINER (table), 10);
      btk_box_pack_start (BTK_BOX (box1), table, TRUE, TRUE, 0);
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    btk_widget_destroy (window);
}

/*
 * BtkRadioButton
 */

static void
create_radio_buttons (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  BtkWidget *box1;
  BtkWidget *box2;
  BtkWidget *button;
  BtkWidget *separator;
  BtkWidget *table;

  if (!window)
    {
      window = btk_dialog_new_with_buttons ("Radio Buttons",
                                            NULL, 0,
                                            BTK_STOCK_CLOSE,
                                            BTK_RESPONSE_NONE,
                                            NULL);

      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window);
      g_signal_connect (window, "response",
                        G_CALLBACK (btk_widget_destroy),
                        NULL);

      box1 = BTK_DIALOG (window)->vbox;

      box2 = btk_vbox_new (FALSE, 10);
      btk_container_set_border_width (BTK_CONTAINER (box2), 10);
      btk_box_pack_start (BTK_BOX (box1), box2, TRUE, TRUE, 0);

      button = btk_radio_button_new_with_label (NULL, "button1");
      btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);

      button = btk_radio_button_new_with_label (
	         btk_radio_button_get_group (BTK_RADIO_BUTTON (button)),
		 "button2");
      btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (button), TRUE);
      btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);

      button = btk_radio_button_new_with_label (
                 btk_radio_button_get_group (BTK_RADIO_BUTTON (button)),
		 "button3");
      btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);

      button = btk_radio_button_new_with_label (
                 btk_radio_button_get_group (BTK_RADIO_BUTTON (button)),
		 "inconsistent");
      btk_toggle_button_set_inconsistent (BTK_TOGGLE_BUTTON (button), TRUE);
      btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);
      
      separator = btk_hseparator_new ();
      btk_box_pack_start (BTK_BOX (box1), separator, FALSE, TRUE, 0);

      box2 = btk_vbox_new (FALSE, 10);
      btk_container_set_border_width (BTK_CONTAINER (box2), 10);
      btk_box_pack_start (BTK_BOX (box1), box2, TRUE, TRUE, 0);

      button = btk_radio_button_new_with_label (NULL, "button4");
      btk_toggle_button_set_mode (BTK_TOGGLE_BUTTON (button), FALSE);
      btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);

      button = btk_radio_button_new_with_label (
	         btk_radio_button_get_group (BTK_RADIO_BUTTON (button)),
		 "button5");
      btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (button), TRUE);
      btk_toggle_button_set_mode (BTK_TOGGLE_BUTTON (button), FALSE);
      btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);

      button = btk_radio_button_new_with_label (
                 btk_radio_button_get_group (BTK_RADIO_BUTTON (button)),
		 "button6");
      btk_toggle_button_set_mode (BTK_TOGGLE_BUTTON (button), FALSE);
      btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);

      separator = btk_hseparator_new ();
      btk_box_pack_start (BTK_BOX (box1), separator, FALSE, TRUE, 0);

      table = create_widget_grid (BTK_TYPE_RADIO_BUTTON);
      btk_container_set_border_width (BTK_CONTAINER (table), 10);
      btk_box_pack_start (BTK_BOX (box1), table, TRUE, TRUE, 0);
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    btk_widget_destroy (window);
}

/*
 * BtkButtonBox
 */

static BtkWidget *
create_bbox (gint  horizontal,
	     char* title, 
	     gint  spacing,
	     gint  child_w,
	     gint  child_h,
	     gint  layout)
{
  BtkWidget *frame;
  BtkWidget *bbox;
  BtkWidget *button;
	
  frame = btk_frame_new (title);

  if (horizontal)
    bbox = btk_hbutton_box_new ();
  else
    bbox = btk_vbutton_box_new ();

  btk_container_set_border_width (BTK_CONTAINER (bbox), 5);
  btk_container_add (BTK_CONTAINER (frame), bbox);

  btk_button_box_set_layout (BTK_BUTTON_BOX (bbox), layout);
  btk_box_set_spacing (BTK_BOX (bbox), spacing);
  btk_button_box_set_child_size (BTK_BUTTON_BOX (bbox), child_w, child_h);
  
  button = btk_button_new_with_label ("OK");
  btk_container_add (BTK_CONTAINER (bbox), button);
  
  button = btk_button_new_with_label ("Cancel");
  btk_container_add (BTK_CONTAINER (bbox), button);
  
  button = btk_button_new_with_label ("Help");
  btk_container_add (BTK_CONTAINER (bbox), button);

  return frame;
}

static void
create_button_box (BtkWidget *widget)
{
  static BtkWidget* window = NULL;
  BtkWidget *main_vbox;
  BtkWidget *vbox;
  BtkWidget *hbox;
  BtkWidget *frame_horz;
  BtkWidget *frame_vert;

  if (!window)
  {
    window = btk_window_new (BTK_WINDOW_TOPLEVEL);
    btk_window_set_screen (BTK_WINDOW (window), btk_widget_get_screen (widget));
    btk_window_set_title (BTK_WINDOW (window), "Button Boxes");
    
    g_signal_connect (window, "destroy",
		      G_CALLBACK (btk_widget_destroyed),
		      &window);

    btk_container_set_border_width (BTK_CONTAINER (window), 10);

    main_vbox = btk_vbox_new (FALSE, 0);
    btk_container_add (BTK_CONTAINER (window), main_vbox);
    
    frame_horz = btk_frame_new ("Horizontal Button Boxes");
    btk_box_pack_start (BTK_BOX (main_vbox), frame_horz, TRUE, TRUE, 10);
    
    vbox = btk_vbox_new (FALSE, 0);
    btk_container_set_border_width (BTK_CONTAINER (vbox), 10);
    btk_container_add (BTK_CONTAINER (frame_horz), vbox);
    
    btk_box_pack_start (BTK_BOX (vbox), 
                        create_bbox (TRUE, "Spread", 40, 85, 20, BTK_BUTTONBOX_SPREAD),
			TRUE, TRUE, 0);
    
    btk_box_pack_start (BTK_BOX (vbox), 
                        create_bbox (TRUE, "Edge", 40, 85, 20, BTK_BUTTONBOX_EDGE),
			TRUE, TRUE, 5);
    
    btk_box_pack_start (BTK_BOX (vbox), 
                        create_bbox (TRUE, "Start", 40, 85, 20, BTK_BUTTONBOX_START),
			TRUE, TRUE, 5);
    
    btk_box_pack_start (BTK_BOX (vbox), 
                        create_bbox (TRUE, "End", 40, 85, 20, BTK_BUTTONBOX_END),
			TRUE, TRUE, 5);
    
    btk_box_pack_start (BTK_BOX (vbox),
                        create_bbox (TRUE, "Center", 40, 85, 20, BTK_BUTTONBOX_CENTER),
			TRUE, TRUE, 5);
    
    frame_vert = btk_frame_new ("Vertical Button Boxes");
    btk_box_pack_start (BTK_BOX (main_vbox), frame_vert, TRUE, TRUE, 10);
    
    hbox = btk_hbox_new (FALSE, 0);
    btk_container_set_border_width (BTK_CONTAINER (hbox), 10);
    btk_container_add (BTK_CONTAINER (frame_vert), hbox);

    btk_box_pack_start (BTK_BOX (hbox), 
                        create_bbox (FALSE, "Spread", 30, 85, 20, BTK_BUTTONBOX_SPREAD),
			TRUE, TRUE, 0);
    
    btk_box_pack_start (BTK_BOX (hbox), 
                        create_bbox (FALSE, "Edge", 30, 85, 20, BTK_BUTTONBOX_EDGE),
			TRUE, TRUE, 5);
    
    btk_box_pack_start (BTK_BOX (hbox), 
                        create_bbox (FALSE, "Start", 30, 85, 20, BTK_BUTTONBOX_START),
			TRUE, TRUE, 5);
    
    btk_box_pack_start (BTK_BOX (hbox), 
                        create_bbox (FALSE, "End", 30, 85, 20, BTK_BUTTONBOX_END),
			TRUE, TRUE, 5);
    
    btk_box_pack_start (BTK_BOX (hbox),
                        create_bbox (FALSE, "Center", 30, 85, 20, BTK_BUTTONBOX_CENTER),
			TRUE, TRUE, 5);
  }

  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    btk_widget_destroy (window);
}

/*
 * BtkToolBar
 */

static BtkWidget*
new_pixmap (char      *filename,
	    BdkWindow *window,
	    BdkColor  *background)
{
  BtkWidget *wpixmap;
  BdkPixmap *pixmap;
  BdkBitmap *mask;

  if (strcmp (filename, "test.xpm") == 0 ||
      !file_exists (filename))
    {
      pixmap = bdk_pixmap_create_from_xpm_d (window, &mask,
					     background,
					     openfile);
    }
  else
    pixmap = bdk_pixmap_create_from_xpm (window, &mask,
					 background,
					 filename);
  
  wpixmap = btk_image_new_from_pixmap (pixmap, mask);

  return wpixmap;
}


static void
set_toolbar_small_stock (BtkWidget *widget,
			 gpointer   data)
{
  btk_toolbar_set_icon_size (BTK_TOOLBAR (data), BTK_ICON_SIZE_SMALL_TOOLBAR);
}

static void
set_toolbar_large_stock (BtkWidget *widget,
			 gpointer   data)
{
  btk_toolbar_set_icon_size (BTK_TOOLBAR (data), BTK_ICON_SIZE_LARGE_TOOLBAR);
}

static void
set_toolbar_horizontal (BtkWidget *widget,
			gpointer   data)
{
  btk_toolbar_set_orientation (BTK_TOOLBAR (data), BTK_ORIENTATION_HORIZONTAL);
}

static void
set_toolbar_vertical (BtkWidget *widget,
		      gpointer   data)
{
  btk_toolbar_set_orientation (BTK_TOOLBAR (data), BTK_ORIENTATION_VERTICAL);
}

static void
set_toolbar_icons (BtkWidget *widget,
		   gpointer   data)
{
  btk_toolbar_set_style (BTK_TOOLBAR (data), BTK_TOOLBAR_ICONS);
}

static void
set_toolbar_text (BtkWidget *widget,
	          gpointer   data)
{
  btk_toolbar_set_style (BTK_TOOLBAR (data), BTK_TOOLBAR_TEXT);
}

static void
set_toolbar_both (BtkWidget *widget,
		  gpointer   data)
{
  btk_toolbar_set_style (BTK_TOOLBAR (data), BTK_TOOLBAR_BOTH);
}

static void
set_toolbar_both_horiz (BtkWidget *widget,
			gpointer   data)
{
  btk_toolbar_set_style (BTK_TOOLBAR (data), BTK_TOOLBAR_BOTH_HORIZ);
}

static void
set_toolbar_enable (BtkWidget *widget,
		    gpointer   data)
{
  btk_toolbar_set_tooltips (BTK_TOOLBAR (data), TRUE);
}

static void
set_toolbar_disable (BtkWidget *widget,
		     gpointer   data)
{
  btk_toolbar_set_tooltips (BTK_TOOLBAR (data), FALSE);
}

static void
create_toolbar (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  BtkWidget *toolbar;
  BtkWidget *entry;

  if (!window)
    {
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));
      
      btk_window_set_title (BTK_WINDOW (window), "Toolbar test");

      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window);

      btk_container_set_border_width (BTK_CONTAINER (window), 0);
      btk_widget_realize (window);

      toolbar = btk_toolbar_new ();

      btk_toolbar_insert_stock (BTK_TOOLBAR (toolbar),
				BTK_STOCK_NEW,
				"Stock icon: New", "Toolbar/New",
				G_CALLBACK (set_toolbar_small_stock), toolbar, -1);
      
      btk_toolbar_insert_stock (BTK_TOOLBAR (toolbar),
				BTK_STOCK_OPEN,
				"Stock icon: Open", "Toolbar/Open",
				G_CALLBACK (set_toolbar_large_stock), toolbar, -1);
      
      btk_toolbar_append_item (BTK_TOOLBAR (toolbar),
			       "Horizontal", "Horizontal toolbar layout", "Toolbar/Horizontal",
			       new_pixmap ("test.xpm", window->window, &window->style->bg[BTK_STATE_NORMAL]),
			       G_CALLBACK (set_toolbar_horizontal), toolbar);
      btk_toolbar_append_item (BTK_TOOLBAR (toolbar),
			       "Vertical", "Vertical toolbar layout", "Toolbar/Vertical",
			       new_pixmap ("test.xpm", window->window, &window->style->bg[BTK_STATE_NORMAL]),
			       G_CALLBACK (set_toolbar_vertical), toolbar);

      btk_toolbar_append_space (BTK_TOOLBAR(toolbar));

      btk_toolbar_append_item (BTK_TOOLBAR (toolbar),
			       "Icons", "Only show toolbar icons", "Toolbar/IconsOnly",
			       new_pixmap ("test.xpm", window->window, &window->style->bg[BTK_STATE_NORMAL]),
			       G_CALLBACK (set_toolbar_icons), toolbar);
      btk_toolbar_append_item (BTK_TOOLBAR (toolbar),
			       "Text", "Only show toolbar text", "Toolbar/TextOnly",
			       new_pixmap ("test.xpm", window->window, &window->style->bg[BTK_STATE_NORMAL]),
			       G_CALLBACK (set_toolbar_text), toolbar);
      btk_toolbar_append_item (BTK_TOOLBAR (toolbar),
			       "Both", "Show toolbar icons and text", "Toolbar/Both",
			       new_pixmap ("test.xpm", window->window, &window->style->bg[BTK_STATE_NORMAL]),
			       G_CALLBACK (set_toolbar_both), toolbar);
      btk_toolbar_append_item (BTK_TOOLBAR (toolbar),
			       "Both (horizontal)",
			       "Show toolbar icons and text in a horizontal fashion",
			       "Toolbar/BothHoriz",
			       new_pixmap ("test.xpm", window->window, &window->style->bg[BTK_STATE_NORMAL]),
			       G_CALLBACK (set_toolbar_both_horiz), toolbar);
			       
      btk_toolbar_append_space (BTK_TOOLBAR (toolbar));

      entry = btk_entry_new ();

      btk_toolbar_append_widget (BTK_TOOLBAR (toolbar), entry, "This is an unusable BtkEntry ;)", "Hey don't click me!!!");

      btk_toolbar_append_space (BTK_TOOLBAR (toolbar));


      btk_toolbar_append_space (BTK_TOOLBAR (toolbar));

      btk_toolbar_append_item (BTK_TOOLBAR (toolbar),
			       "Enable", "Enable tooltips", NULL,
			       new_pixmap ("test.xpm", window->window, &window->style->bg[BTK_STATE_NORMAL]),
			       G_CALLBACK (set_toolbar_enable), toolbar);
      btk_toolbar_append_item (BTK_TOOLBAR (toolbar),
			       "Disable", "Disable tooltips", NULL,
			       new_pixmap ("test.xpm", window->window, &window->style->bg[BTK_STATE_NORMAL]),
			       G_CALLBACK (set_toolbar_disable), toolbar);

      btk_toolbar_append_space (BTK_TOOLBAR (toolbar));

      btk_toolbar_append_item (BTK_TOOLBAR (toolbar),
			       "Frobate", "Frobate tooltip", NULL,
			       new_pixmap ("test.xpm", window->window, &window->style->bg[BTK_STATE_NORMAL]),
			       NULL, toolbar);
      btk_toolbar_append_item (BTK_TOOLBAR (toolbar),
			       "Baz", "Baz tooltip", NULL,
			       new_pixmap ("test.xpm", window->window, &window->style->bg[BTK_STATE_NORMAL]),
			       NULL, toolbar);

      btk_toolbar_append_space (BTK_TOOLBAR (toolbar));
      
      btk_toolbar_append_item (BTK_TOOLBAR (toolbar),
			       "Blah", "Blah tooltip", NULL,
			       new_pixmap ("test.xpm", window->window, &window->style->bg[BTK_STATE_NORMAL]),
			       NULL, toolbar);
      btk_toolbar_append_item (BTK_TOOLBAR (toolbar),
			       "Bar", "Bar tooltip", NULL,
			       new_pixmap ("test.xpm", window->window, &window->style->bg[BTK_STATE_NORMAL]),
			       NULL, toolbar);

      btk_container_add (BTK_CONTAINER (window), toolbar);

      btk_widget_set_size_request (toolbar, 200, -1);
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    btk_widget_destroy (window);
}

static BtkWidget*
make_toolbar (BtkWidget *window)
{
  BtkWidget *toolbar;

  if (!btk_widget_get_realized (window))
    btk_widget_realize (window);

  toolbar = btk_toolbar_new ();

  btk_toolbar_append_item (BTK_TOOLBAR (toolbar),
			   "Horizontal", "Horizontal toolbar layout", NULL,
			   new_pixmap ("test.xpm", window->window, &window->style->bg[BTK_STATE_NORMAL]),
			   G_CALLBACK (set_toolbar_horizontal), toolbar);
  btk_toolbar_append_item (BTK_TOOLBAR (toolbar),
			   "Vertical", "Vertical toolbar layout", NULL,
			   new_pixmap ("test.xpm", window->window, &window->style->bg[BTK_STATE_NORMAL]),
			   G_CALLBACK (set_toolbar_vertical), toolbar);

  btk_toolbar_append_space (BTK_TOOLBAR(toolbar));

  btk_toolbar_append_item (BTK_TOOLBAR (toolbar),
			   "Icons", "Only show toolbar icons", NULL,
			   new_pixmap ("test.xpm", window->window, &window->style->bg[BTK_STATE_NORMAL]),
			   G_CALLBACK (set_toolbar_icons), toolbar);
  btk_toolbar_append_item (BTK_TOOLBAR (toolbar),
			   "Text", "Only show toolbar text", NULL,
			   new_pixmap ("test.xpm", window->window, &window->style->bg[BTK_STATE_NORMAL]),
			   G_CALLBACK (set_toolbar_text), toolbar);
  btk_toolbar_append_item (BTK_TOOLBAR (toolbar),
			   "Both", "Show toolbar icons and text", NULL,
			   new_pixmap ("test.xpm", window->window, &window->style->bg[BTK_STATE_NORMAL]),
			   G_CALLBACK (set_toolbar_both), toolbar);

  btk_toolbar_append_space (BTK_TOOLBAR (toolbar));

  btk_toolbar_append_item (BTK_TOOLBAR (toolbar),
			   "Woot", "Woot woot woot", NULL,
			   new_pixmap ("test.xpm", window->window, &window->style->bg[BTK_STATE_NORMAL]),
			   NULL, toolbar);
  btk_toolbar_append_item (BTK_TOOLBAR (toolbar),
			   "Blah", "Blah blah blah", "Toolbar/Big",
			   new_pixmap ("test.xpm", window->window, &window->style->bg[BTK_STATE_NORMAL]),
			   NULL, toolbar);

  btk_toolbar_append_space (BTK_TOOLBAR (toolbar));

  btk_toolbar_append_item (BTK_TOOLBAR (toolbar),
			   "Enable", "Enable tooltips", NULL,
			   new_pixmap ("test.xpm", window->window, &window->style->bg[BTK_STATE_NORMAL]),
			   G_CALLBACK (set_toolbar_enable), toolbar);
  btk_toolbar_append_item (BTK_TOOLBAR (toolbar),
			   "Disable", "Disable tooltips", NULL,
			   new_pixmap ("test.xpm", window->window, &window->style->bg[BTK_STATE_NORMAL]),
			   G_CALLBACK (set_toolbar_disable), toolbar);

  btk_toolbar_append_space (BTK_TOOLBAR (toolbar));
  
  btk_toolbar_append_item (BTK_TOOLBAR (toolbar),
			   "Hoo", "Hoo tooltip", NULL,
			   new_pixmap ("test.xpm", window->window, &window->style->bg[BTK_STATE_NORMAL]),
			   NULL, toolbar);
  btk_toolbar_append_item (BTK_TOOLBAR (toolbar),
			   "Woo", "Woo tooltip", NULL,
			   new_pixmap ("test.xpm", window->window, &window->style->bg[BTK_STATE_NORMAL]),
			   NULL, toolbar);

  return toolbar;
}

/*
 * BtkStatusBar
 */

static guint statusbar_counter = 1;

static void
statusbar_push (BtkWidget *button,
		BtkStatusbar *statusbar)
{
  gchar text[1024];

  sprintf (text, "something %d", statusbar_counter++);

  btk_statusbar_push (statusbar, 1, text);
}

static void
statusbar_push_long (BtkWidget *button,
                     BtkStatusbar *statusbar)
{
  gchar text[1024];

  sprintf (text, "Just because a system has menu choices written with English words, phrases or sentences, that is no guarantee, that it is comprehensible. Individual words may not be familiar to some users (for example, \"repaginate\"), and two menu items may appear to satisfy the users's needs, whereas only one does (for example, \"put away\" or \"eject\").");

  btk_statusbar_push (statusbar, 1, text);
}

static void
statusbar_pop (BtkWidget *button,
	       BtkStatusbar *statusbar)
{
  btk_statusbar_pop (statusbar, 1);
}

static void
statusbar_steal (BtkWidget *button,
	         BtkStatusbar *statusbar)
{
  btk_statusbar_remove (statusbar, 1, 4);
}

static void
statusbar_popped (BtkStatusbar  *statusbar,
		  guint          context_id,
		  const gchar	*text)
{
  if (!statusbar->messages)
    statusbar_counter = 1;
}

static void
statusbar_contexts (BtkStatusbar *statusbar)
{
  gchar *string;

  string = "any context";
  g_print ("BtkStatusBar: context=\"%s\", context_id=%d\n",
	   string,
	   btk_statusbar_get_context_id (statusbar, string));
  
  string = "idle messages";
  g_print ("BtkStatusBar: context=\"%s\", context_id=%d\n",
	   string,
	   btk_statusbar_get_context_id (statusbar, string));
  
  string = "some text";
  g_print ("BtkStatusBar: context=\"%s\", context_id=%d\n",
	   string,
	   btk_statusbar_get_context_id (statusbar, string));

  string = "hit the mouse";
  g_print ("BtkStatusBar: context=\"%s\", context_id=%d\n",
	   string,
	   btk_statusbar_get_context_id (statusbar, string));

  string = "hit the mouse2";
  g_print ("BtkStatusBar: context=\"%s\", context_id=%d\n",
	   string,
	   btk_statusbar_get_context_id (statusbar, string));
}

static void
create_statusbar (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  BtkWidget *box1;
  BtkWidget *box2;
  BtkWidget *button;
  BtkWidget *separator;
  BtkWidget *statusbar;

  if (!window)
    {
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window);

      btk_window_set_title (BTK_WINDOW (window), "statusbar");
      btk_container_set_border_width (BTK_CONTAINER (window), 0);

      box1 = btk_vbox_new (FALSE, 0);
      btk_container_add (BTK_CONTAINER (window), box1);

      box2 = btk_vbox_new (FALSE, 10);
      btk_container_set_border_width (BTK_CONTAINER (box2), 10);
      btk_box_pack_start (BTK_BOX (box1), box2, TRUE, TRUE, 0);

      statusbar = btk_statusbar_new ();
      btk_box_pack_end (BTK_BOX (box1), statusbar, TRUE, TRUE, 0);
      g_signal_connect (statusbar,
			"text_popped",
			G_CALLBACK (statusbar_popped),
			NULL);

      button = g_object_new (btk_button_get_type (),
			       "label", "push something",
			       "visible", TRUE,
			       "parent", box2,
			       NULL);
      g_object_connect (button,
			"signal::clicked", statusbar_push, statusbar,
			NULL);

      button = g_object_connect (g_object_new (btk_button_get_type (),
						 "label", "pop",
						 "visible", TRUE,
						 "parent", box2,
						 NULL),
				 "signal_after::clicked", statusbar_pop, statusbar,
				 NULL);

      button = g_object_connect (g_object_new (btk_button_get_type (),
						 "label", "steal #4",
						 "visible", TRUE,
						 "parent", box2,
						 NULL),
				 "signal_after::clicked", statusbar_steal, statusbar,
				 NULL);

      button = g_object_connect (g_object_new (btk_button_get_type (),
						 "label", "test contexts",
						 "visible", TRUE,
						 "parent", box2,
						 NULL),
				 "swapped_signal_after::clicked", statusbar_contexts, statusbar,
				 NULL);

      button = g_object_connect (g_object_new (btk_button_get_type (),
						 "label", "push something long",
						 "visible", TRUE,
						 "parent", box2,
						 NULL),
				 "signal_after::clicked", statusbar_push_long, statusbar,
				 NULL);
      
      separator = btk_hseparator_new ();
      btk_box_pack_start (BTK_BOX (box1), separator, FALSE, TRUE, 0);

      box2 = btk_vbox_new (FALSE, 10);
      btk_container_set_border_width (BTK_CONTAINER (box2), 10);
      btk_box_pack_start (BTK_BOX (box1), box2, FALSE, TRUE, 0);

      button = btk_button_new_with_label ("close");
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (btk_widget_destroy),
				window);
      btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);
      btk_widget_set_can_default (button, TRUE);
      btk_widget_grab_default (button);
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    btk_widget_destroy (window);
}

/*
 * BtkTree
 */

static void
cb_tree_destroy_event(BtkWidget* w)
{
  sTreeButtons* tree_buttons;

  /* free buttons structure associate at this tree */
  tree_buttons = g_object_get_data (B_OBJECT (w), "user_data");
  g_free (tree_buttons);
}

static void
cb_add_new_item(BtkWidget* w, BtkTree* tree)
{
  sTreeButtons* tree_buttons;
  GList* selected_list;
  BtkWidget* selected_item;
  BtkWidget* subtree;
  BtkWidget* item_new;
  char buffer[255];

  tree_buttons = g_object_get_data (B_OBJECT (tree), "user_data");

  selected_list = BTK_TREE_SELECTION_OLD(tree);

  if(selected_list == NULL)
    {
      /* there is no item in tree */
      subtree = BTK_WIDGET(tree);
    }
  else
    {
      /* list can have only one element */
      selected_item = BTK_WIDGET(selected_list->data);
      
      subtree = BTK_TREE_ITEM_SUBTREE(selected_item);

      if(subtree == NULL)
	{
	  /* current selected item have not subtree ... create it */
	  subtree = btk_tree_new();
	  btk_tree_item_set_subtree(BTK_TREE_ITEM(selected_item), 
				    subtree);
	}
    }

  /* at this point, we know which subtree will be used to add new item */
  /* create a new item */
  sprintf(buffer, "item add %d", tree_buttons->nb_item_add);
  item_new = btk_tree_item_new_with_label(buffer);
  btk_tree_append(BTK_TREE(subtree), item_new);
  btk_widget_show(item_new);

  tree_buttons->nb_item_add++;
}

static void
cb_remove_item(BtkWidget*w, BtkTree* tree)
{
  GList* selected_list;
  GList* clear_list;
  
  selected_list = BTK_TREE_SELECTION_OLD(tree);

  clear_list = NULL;
    
  while (selected_list) 
    {
      clear_list = g_list_prepend (clear_list, selected_list->data);
      selected_list = selected_list->next;
    }
  
  clear_list = g_list_reverse (clear_list);
  btk_tree_remove_items(tree, clear_list);

  g_list_free (clear_list);
}

static void
cb_remove_subtree(BtkWidget*w, BtkTree* tree)
{
  GList* selected_list;
  BtkTreeItem *item;
  
  selected_list = BTK_TREE_SELECTION_OLD(tree);

  if (selected_list)
    {
      item = BTK_TREE_ITEM (selected_list->data);
      if (item->subtree)
	btk_tree_item_remove_subtree (item);
    }
}

static void
cb_tree_changed(BtkTree* tree)
{
  sTreeButtons* tree_buttons;
  GList* selected_list;
  guint nb_selected;

  tree_buttons = g_object_get_data (B_OBJECT (tree), "user_data");

  selected_list = BTK_TREE_SELECTION_OLD(tree);
  nb_selected = g_list_length(selected_list);

  if(nb_selected == 0) 
    {
      if(tree->children == NULL)
	btk_widget_set_sensitive(tree_buttons->add_button, TRUE);
      else
	btk_widget_set_sensitive(tree_buttons->add_button, FALSE);
      btk_widget_set_sensitive(tree_buttons->remove_button, FALSE);
      btk_widget_set_sensitive(tree_buttons->subtree_button, FALSE);
    } 
  else 
    {
      btk_widget_set_sensitive(tree_buttons->remove_button, TRUE);
      btk_widget_set_sensitive(tree_buttons->add_button, (nb_selected == 1));
      btk_widget_set_sensitive(tree_buttons->subtree_button, (nb_selected == 1));
    }  
}

static void 
create_subtree(BtkWidget* item, guint level, guint nb_item_max, guint recursion_level_max)
{
  BtkWidget* item_subtree;
  BtkWidget* item_new;
  guint nb_item;
  char buffer[255];
  int no_root_item;

  if(level == recursion_level_max) return;

  if(level == -1)
    {
      /* query with no root item */
      level = 0;
      item_subtree = item;
      no_root_item = 1;
    }
  else
    {
      /* query with no root item */
      /* create subtree and associate it with current item */
      item_subtree = btk_tree_new();
      no_root_item = 0;
    }
  
  for(nb_item = 0; nb_item < nb_item_max; nb_item++)
    {
      sprintf(buffer, "item %d-%d", level, nb_item);
      item_new = btk_tree_item_new_with_label(buffer);
      btk_tree_append(BTK_TREE(item_subtree), item_new);
      create_subtree(item_new, level+1, nb_item_max, recursion_level_max);
      btk_widget_show(item_new);
    }

  if(!no_root_item)
    btk_tree_item_set_subtree(BTK_TREE_ITEM(item), item_subtree);
}

static void
create_tree_sample(BdkScreen *screen, guint selection_mode, 
		   guint draw_line, guint view_line, guint no_root_item,
		   guint nb_item_max, guint recursion_level_max) 
{
  BtkWidget* window;
  BtkWidget* box1;
  BtkWidget* box2;
  BtkWidget* separator;
  BtkWidget* button;
  BtkWidget* scrolled_win;
  BtkWidget* root_tree;
  BtkWidget* root_item;
  sTreeButtons* tree_buttons;

  /* create tree buttons struct */
  if ((tree_buttons = g_malloc (sizeof (sTreeButtons))) == NULL)
    {
      g_error("can't allocate memory for tree structure !\n");
      return;
    }
  tree_buttons->nb_item_add = 0;

  /* create top level window */
  window = btk_window_new(BTK_WINDOW_TOPLEVEL);
  btk_window_set_screen (BTK_WINDOW (window), screen);
  btk_window_set_title(BTK_WINDOW(window), "Tree Sample");
  g_signal_connect (window, "destroy",
		    G_CALLBACK (cb_tree_destroy_event), NULL);
  g_object_set_data (B_OBJECT (window), "user_data", tree_buttons);

  box1 = btk_vbox_new(FALSE, 0);
  btk_container_add(BTK_CONTAINER(window), box1);
  btk_widget_show(box1);

  /* create tree box */
  box2 = btk_vbox_new(FALSE, 0);
  btk_box_pack_start(BTK_BOX(box1), box2, TRUE, TRUE, 0);
  btk_container_set_border_width(BTK_CONTAINER(box2), 5);
  btk_widget_show(box2);

  /* create scrolled window */
  scrolled_win = btk_scrolled_window_new (NULL, NULL);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (scrolled_win),
				  BTK_POLICY_AUTOMATIC, BTK_POLICY_AUTOMATIC);
  btk_box_pack_start (BTK_BOX (box2), scrolled_win, TRUE, TRUE, 0);
  btk_widget_set_size_request (scrolled_win, 200, 200);
  btk_widget_show (scrolled_win);
  
  /* create root tree widget */
  root_tree = btk_tree_new();
  g_signal_connect (root_tree, "selection_changed",
		    G_CALLBACK (cb_tree_changed),
		    NULL);
  g_object_set_data (B_OBJECT (root_tree), "user_data", tree_buttons);
  btk_scrolled_window_add_with_viewport (BTK_SCROLLED_WINDOW (scrolled_win), root_tree);
  btk_tree_set_selection_mode(BTK_TREE(root_tree), selection_mode);
  btk_tree_set_view_lines(BTK_TREE(root_tree), draw_line);
  btk_tree_set_view_mode(BTK_TREE(root_tree), !view_line);
  btk_widget_show(root_tree);

  if ( no_root_item )
    {
      /* set root tree to subtree function with root item variable */
      root_item = BTK_WIDGET(root_tree);
    }
  else
    {
      /* create root tree item widget */
      root_item = btk_tree_item_new_with_label("root item");
      btk_tree_append(BTK_TREE(root_tree), root_item);
      btk_widget_show(root_item);
     }
  create_subtree(root_item, -no_root_item, nb_item_max, recursion_level_max);

  box2 = btk_vbox_new(FALSE, 0);
  btk_box_pack_start(BTK_BOX(box1), box2, FALSE, FALSE, 0);
  btk_container_set_border_width(BTK_CONTAINER(box2), 5);
  btk_widget_show(box2);

  button = btk_button_new_with_label("Add Item");
  btk_widget_set_sensitive(button, FALSE);
  g_signal_connect (button, "clicked",
		    G_CALLBACK (cb_add_new_item),
		    root_tree);
  btk_box_pack_start(BTK_BOX(box2), button, TRUE, TRUE, 0);
  btk_widget_show(button);
  tree_buttons->add_button = button;

  button = btk_button_new_with_label("Remove Item(s)");
  btk_widget_set_sensitive(button, FALSE);
  g_signal_connect (button, "clicked",
		    G_CALLBACK (cb_remove_item),
		    root_tree);
  btk_box_pack_start(BTK_BOX(box2), button, TRUE, TRUE, 0);
  btk_widget_show(button);
  tree_buttons->remove_button = button;

  button = btk_button_new_with_label("Remove Subtree");
  btk_widget_set_sensitive(button, FALSE);
  g_signal_connect (button, "clicked",
		    G_CALLBACK (cb_remove_subtree),
		    root_tree);
  btk_box_pack_start(BTK_BOX(box2), button, TRUE, TRUE, 0);
  btk_widget_show(button);
  tree_buttons->subtree_button = button;

  /* create separator */
  separator = btk_hseparator_new();
  btk_box_pack_start(BTK_BOX(box1), separator, FALSE, FALSE, 0);
  btk_widget_show(separator);

  /* create button box */
  box2 = btk_vbox_new(FALSE, 0);
  btk_box_pack_start(BTK_BOX(box1), box2, FALSE, FALSE, 0);
  btk_container_set_border_width(BTK_CONTAINER(box2), 5);
  btk_widget_show(box2);

  button = btk_button_new_with_label("Close");
  btk_box_pack_start(BTK_BOX(box2), button, TRUE, TRUE, 0);
  g_signal_connect_swapped (button, "clicked",
			    G_CALLBACK (btk_widget_destroy),
			    window);
  btk_widget_show(button);

  btk_widget_show(window);
}

static void
cb_create_tree(BtkWidget* w)
{
  guint selection_mode = BTK_SELECTION_SINGLE;
  guint view_line;
  guint draw_line;
  guint no_root_item;
  guint nb_item;
  guint recursion_level;

  /* get selection mode choice */
  if(BTK_TOGGLE_BUTTON(sTreeSampleSelection.single_button)->active)
    selection_mode = BTK_SELECTION_SINGLE;
  else
    if(BTK_TOGGLE_BUTTON(sTreeSampleSelection.browse_button)->active)
      selection_mode = BTK_SELECTION_BROWSE;
    else
      selection_mode = BTK_SELECTION_MULTIPLE;

  /* get options choice */
  draw_line = BTK_TOGGLE_BUTTON(sTreeSampleSelection.draw_line_button)->active;
  view_line = BTK_TOGGLE_BUTTON(sTreeSampleSelection.view_line_button)->active;
  no_root_item = BTK_TOGGLE_BUTTON(sTreeSampleSelection.no_root_item_button)->active;
    
  /* get levels */
  nb_item = btk_spin_button_get_value_as_int(BTK_SPIN_BUTTON(sTreeSampleSelection.nb_item_spinner));
  recursion_level = btk_spin_button_get_value_as_int(BTK_SPIN_BUTTON(sTreeSampleSelection.recursion_spinner));

  if (pow (nb_item, recursion_level) > 10000)
    {
      g_print ("%g total items? That will take a very long time. Try less\n",
	       pow (nb_item, recursion_level));
      return;
    }

  create_tree_sample(btk_widget_get_screen (w),
		     selection_mode, draw_line, 
		     view_line, no_root_item, nb_item, recursion_level);
}

void 
create_tree_mode_window(BtkWidget *widget)
{
  static BtkWidget* window;
  BtkWidget* box1;
  BtkWidget* box2;
  BtkWidget* box3;
  BtkWidget* box4;
  BtkWidget* box5;
  BtkWidget* button;
  BtkWidget* frame;
  BtkWidget* separator;
  BtkWidget* label;
  BtkWidget* spinner;
  BtkAdjustment *adj;

  if (!window)
    {
      /* create toplevel window  */
      window = btk_window_new(BTK_WINDOW_TOPLEVEL);
      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));
      btk_window_set_title(BTK_WINDOW(window), "Set Tree Parameters");
      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window);
      box1 = btk_vbox_new(FALSE, 0);
      btk_container_add(BTK_CONTAINER(window), box1);

      /* create upper box - selection box */
      box2 = btk_vbox_new(FALSE, 5);
      btk_box_pack_start(BTK_BOX(box1), box2, TRUE, TRUE, 0);
      btk_container_set_border_width(BTK_CONTAINER(box2), 5);

      box3 = btk_hbox_new(FALSE, 5);
      btk_box_pack_start(BTK_BOX(box2), box3, TRUE, TRUE, 0);

      /* create selection mode frame */
      frame = btk_frame_new("Selection Mode");
      btk_box_pack_start(BTK_BOX(box3), frame, TRUE, TRUE, 0);

      box4 = btk_vbox_new(FALSE, 0);
      btk_container_add(BTK_CONTAINER(frame), box4);
      btk_container_set_border_width(BTK_CONTAINER(box4), 5);

      /* create radio button */  
      button = btk_radio_button_new_with_label(NULL, "SINGLE");
      btk_box_pack_start(BTK_BOX(box4), button, TRUE, TRUE, 0);
      sTreeSampleSelection.single_button = button;

      button = btk_radio_button_new_with_label(btk_radio_button_get_group (BTK_RADIO_BUTTON (button)),
					       "BROWSE");
      btk_box_pack_start(BTK_BOX(box4), button, TRUE, TRUE, 0);
      sTreeSampleSelection.browse_button = button;

      button = btk_radio_button_new_with_label(btk_radio_button_get_group (BTK_RADIO_BUTTON (button)),
					       "MULTIPLE");
      btk_box_pack_start(BTK_BOX(box4), button, TRUE, TRUE, 0);
      sTreeSampleSelection.multiple_button = button;

      sTreeSampleSelection.selection_mode_group = btk_radio_button_get_group (BTK_RADIO_BUTTON (button));

      /* create option mode frame */
      frame = btk_frame_new("Options");
      btk_box_pack_start(BTK_BOX(box3), frame, TRUE, TRUE, 0);

      box4 = btk_vbox_new(FALSE, 0);
      btk_container_add(BTK_CONTAINER(frame), box4);
      btk_container_set_border_width(BTK_CONTAINER(box4), 5);

      /* create check button */
      button = btk_check_button_new_with_label("Draw line");
      btk_box_pack_start(BTK_BOX(box4), button, TRUE, TRUE, 0);
      btk_toggle_button_set_active(BTK_TOGGLE_BUTTON(button), TRUE);
      sTreeSampleSelection.draw_line_button = button;
  
      button = btk_check_button_new_with_label("View Line mode");
      btk_box_pack_start(BTK_BOX(box4), button, TRUE, TRUE, 0);
      btk_toggle_button_set_active(BTK_TOGGLE_BUTTON(button), TRUE);
      sTreeSampleSelection.view_line_button = button;
  
      button = btk_check_button_new_with_label("Without Root item");
      btk_box_pack_start(BTK_BOX(box4), button, TRUE, TRUE, 0);
      sTreeSampleSelection.no_root_item_button = button;

      /* create recursion parameter */
      frame = btk_frame_new("Size Parameters");
      btk_box_pack_start(BTK_BOX(box2), frame, TRUE, TRUE, 0);

      box4 = btk_hbox_new(FALSE, 5);
      btk_container_add(BTK_CONTAINER(frame), box4);
      btk_container_set_border_width(BTK_CONTAINER(box4), 5);

      /* create number of item spin button */
      box5 = btk_hbox_new(FALSE, 5);
      btk_box_pack_start(BTK_BOX(box4), box5, FALSE, FALSE, 0);

      label = btk_label_new("Number of items : ");
      btk_misc_set_alignment (BTK_MISC (label), 0, 0.5);
      btk_box_pack_start (BTK_BOX (box5), label, FALSE, TRUE, 0);

      adj = (BtkAdjustment *) btk_adjustment_new (DEFAULT_NUMBER_OF_ITEM, 1.0, 255.0, 1.0,
						  5.0, 0.0);
      spinner = btk_spin_button_new (adj, 0, 0);
      btk_box_pack_start (BTK_BOX (box5), spinner, FALSE, TRUE, 0);
      sTreeSampleSelection.nb_item_spinner = spinner;
  
      /* create recursion level spin button */
      box5 = btk_hbox_new(FALSE, 5);
      btk_box_pack_start(BTK_BOX(box4), box5, FALSE, FALSE, 0);

      label = btk_label_new("Depth : ");
      btk_misc_set_alignment (BTK_MISC (label), 0, 0.5);
      btk_box_pack_start (BTK_BOX (box5), label, FALSE, TRUE, 0);

      adj = (BtkAdjustment *) btk_adjustment_new (DEFAULT_RECURSION_LEVEL, 0.0, 255.0, 1.0,
						  5.0, 0.0);
      spinner = btk_spin_button_new (adj, 0, 0);
      btk_box_pack_start (BTK_BOX (box5), spinner, FALSE, TRUE, 0);
      sTreeSampleSelection.recursion_spinner = spinner;
  
      /* create horizontal separator */
      separator = btk_hseparator_new();
      btk_box_pack_start(BTK_BOX(box1), separator, FALSE, FALSE, 0);

      /* create bottom button box */
      box2 = btk_hbox_new(TRUE, 10);
      btk_box_pack_start(BTK_BOX(box1), box2, FALSE, FALSE, 0);
      btk_container_set_border_width(BTK_CONTAINER(box2), 5);

      button = btk_button_new_with_label("Create Tree");
      btk_box_pack_start(BTK_BOX(box2), button, TRUE, TRUE, 0);
      g_signal_connect (button, "clicked",
			G_CALLBACK (cb_create_tree), NULL);

      button = btk_button_new_with_label("Close");
      btk_box_pack_start(BTK_BOX(box2), button, TRUE, TRUE, 0);
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (btk_widget_destroy),
				window);
    }
  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    btk_widget_destroy (window);
}

/*
 * Gridded geometry
 */
#define GRID_SIZE 20
#define DEFAULT_GEOMETRY "10x10"

static gboolean
gridded_geometry_expose (BtkWidget      *widget,
			 BdkEventExpose *event)
{
  int i, j;
  bairo_t *cr;

  cr = bdk_bairo_create (widget->window);

  bairo_rectangle (cr, 0, 0, widget->allocation.width, widget->allocation.height);
  bdk_bairo_set_source_color (cr, &widget->style->base[widget->state]);
  bairo_fill (cr);
  
  for (i = 0 ; i * GRID_SIZE < widget->allocation.width; i++)
    for (j = 0 ; j * GRID_SIZE < widget->allocation.height; j++)
      {
	if ((i + j) % 2 == 0)
	  bairo_rectangle (cr, i * GRID_SIZE, j * GRID_SIZE, GRID_SIZE, GRID_SIZE);
      }

  bdk_bairo_set_source_color (cr, &widget->style->text[widget->state]);
  bairo_fill (cr);

  bairo_destroy (cr);

  return FALSE;
}

static void
gridded_geometry_subresponse (BtkDialog *dialog,
			      gint       response_id,
			      gchar     *geometry_string)
{
  if (response_id == BTK_RESPONSE_NONE)
    {
      btk_widget_destroy (BTK_WIDGET (dialog));
    }
  else
    {
      if (!btk_window_parse_geometry (BTK_WINDOW (dialog), geometry_string))
	{
	  g_print ("Can't parse geometry string %s\n", geometry_string);
	  btk_window_parse_geometry (BTK_WINDOW (dialog), DEFAULT_GEOMETRY);
	}
    }
}

static void
gridded_geometry_response (BtkDialog *dialog,
			   gint       response_id,
			   BtkEntry  *entry)
{
  if (response_id == BTK_RESPONSE_NONE)
    {
      btk_widget_destroy (BTK_WIDGET (dialog));
    }
  else
    {
      gchar *geometry_string = g_strdup (btk_entry_get_text (entry));
      gchar *title = g_strdup_printf ("Gridded window at: %s", geometry_string);
      BtkWidget *window;
      BtkWidget *drawing_area;
      BtkWidget *box;
      BdkGeometry geometry;
      
      window = btk_dialog_new_with_buttons (title,
                                            NULL, 0,
                                            "Reset", 1,
                                            BTK_STOCK_CLOSE, BTK_RESPONSE_NONE,
                                            NULL);

      btk_window_set_screen (BTK_WINDOW (window), 
			     btk_widget_get_screen (BTK_WIDGET (dialog)));
      g_free (title);
      g_signal_connect (window, "response",
			G_CALLBACK (gridded_geometry_subresponse), geometry_string);

      box = btk_vbox_new (FALSE, 0);
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->vbox), box, TRUE, TRUE, 0);
      
      btk_container_set_border_width (BTK_CONTAINER (box), 7);
      
      drawing_area = btk_drawing_area_new ();
      g_signal_connect (drawing_area, "expose_event",
			G_CALLBACK (gridded_geometry_expose), NULL);
      btk_box_pack_start (BTK_BOX (box), drawing_area, TRUE, TRUE, 0);

      /* Gross hack to work around bug 68668... if we set the size request
       * large enough, then  the current
       *
       *   request_of_window - request_of_geometry_widget
       *
       * method of getting the base size works more or less works.
       */
      btk_widget_set_size_request (drawing_area, 2000, 2000);

      geometry.base_width = 0;
      geometry.base_height = 0;
      geometry.min_width = 2 * GRID_SIZE;
      geometry.min_height = 2 * GRID_SIZE;
      geometry.width_inc = GRID_SIZE;
      geometry.height_inc = GRID_SIZE;

      btk_window_set_geometry_hints (BTK_WINDOW (window), drawing_area,
				     &geometry,
				     BDK_HINT_BASE_SIZE | BDK_HINT_MIN_SIZE | BDK_HINT_RESIZE_INC);

      if (!btk_window_parse_geometry (BTK_WINDOW (window), geometry_string))
	{
	  g_print ("Can't parse geometry string %s\n", geometry_string);
	  btk_window_parse_geometry (BTK_WINDOW (window), DEFAULT_GEOMETRY);
	}

      btk_widget_show_all (window);
    }
}

static void 
create_gridded_geometry (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  gpointer window_ptr;
  BtkWidget *entry;
  BtkWidget *label;

  if (!window)
    {
      window = btk_dialog_new_with_buttons ("Gridded Geometry",
                                            NULL, 0,
					    "Create", 1,
                                            BTK_STOCK_CLOSE, BTK_RESPONSE_NONE,
                                            NULL);
      
      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));

      label = btk_label_new ("Geometry string:");
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->vbox), label, FALSE, FALSE, 0);

      entry = btk_entry_new ();
      btk_entry_set_text (BTK_ENTRY (entry), DEFAULT_GEOMETRY);
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->vbox), entry, FALSE, FALSE, 0);

      g_signal_connect (window, "response",
			G_CALLBACK (gridded_geometry_response), entry);
      window_ptr = &window;
      g_object_add_weak_pointer (B_OBJECT (window), window_ptr);

      btk_widget_show_all (window);
    }
  else
    btk_widget_destroy (window);
}

/*
 * BtkHandleBox
 */

static void
handle_box_child_signal (BtkHandleBox *hb,
			 BtkWidget    *child,
			 const gchar  *action)
{
  printf ("%s: child <%s> %sed\n",
	  g_type_name (B_OBJECT_TYPE (hb)),
	  g_type_name (B_OBJECT_TYPE (child)),
	  action);
}

static void
create_handle_box (BtkWidget *widget)
{
  static BtkWidget* window = NULL;
  BtkWidget *handle_box;
  BtkWidget *handle_box2;
  BtkWidget *vbox;
  BtkWidget *hbox;
  BtkWidget *toolbar;
  BtkWidget *label;
  BtkWidget *separator;

  if (!window)
  {
    window = btk_window_new (BTK_WINDOW_TOPLEVEL);
    
    btk_window_set_screen (BTK_WINDOW (window),
			   btk_widget_get_screen (widget));
    btk_window_set_modal (BTK_WINDOW (window), FALSE);
    btk_window_set_title (BTK_WINDOW (window),
			  "Handle Box Test");
    btk_window_set_resizable (BTK_WINDOW (window), TRUE);
    
    g_signal_connect (window, "destroy",
		      G_CALLBACK (btk_widget_destroyed),
		      &window);
    
    btk_container_set_border_width (BTK_CONTAINER (window), 20);

    vbox = btk_vbox_new (FALSE, 0);
    btk_container_add (BTK_CONTAINER (window), vbox);
    btk_widget_show (vbox);

    label = btk_label_new ("Above");
    btk_container_add (BTK_CONTAINER (vbox), label);
    btk_widget_show (label);

    separator = btk_hseparator_new ();
    btk_container_add (BTK_CONTAINER (vbox), separator);
    btk_widget_show (separator);
    
    hbox = btk_hbox_new (FALSE, 10);
    btk_container_add (BTK_CONTAINER (vbox), hbox);
    btk_widget_show (hbox);

    separator = btk_hseparator_new ();
    btk_container_add (BTK_CONTAINER (vbox), separator);
    btk_widget_show (separator);

    label = btk_label_new ("Below");
    btk_container_add (BTK_CONTAINER (vbox), label);
    btk_widget_show (label);

    handle_box = btk_handle_box_new ();
    btk_box_pack_start (BTK_BOX (hbox), handle_box, FALSE, FALSE, 0);
    g_signal_connect (handle_box,
		      "child_attached",
		      G_CALLBACK (handle_box_child_signal),
		      "attached");
    g_signal_connect (handle_box,
		      "child_detached",
		      G_CALLBACK (handle_box_child_signal),
		      "detached");
    btk_widget_show (handle_box);

    toolbar = make_toolbar (window);
    
    btk_container_add (BTK_CONTAINER (handle_box), toolbar);
    btk_widget_show (toolbar);

    handle_box = btk_handle_box_new ();
    btk_box_pack_start (BTK_BOX (hbox), handle_box, FALSE, FALSE, 0);
    g_signal_connect (handle_box,
		      "child_attached",
		      G_CALLBACK (handle_box_child_signal),
		      "attached");
    g_signal_connect (handle_box,
		      "child_detached",
		      G_CALLBACK (handle_box_child_signal),
		      "detached");
    btk_widget_show (handle_box);

    handle_box2 = btk_handle_box_new ();
    btk_container_add (BTK_CONTAINER (handle_box), handle_box2);
    g_signal_connect (handle_box2,
		      "child_attached",
		      G_CALLBACK (handle_box_child_signal),
		      "attached");
    g_signal_connect (handle_box2,
		      "child_detached",
		      G_CALLBACK (handle_box_child_signal),
		      "detached");
    btk_widget_show (handle_box2);

    hbox = g_object_new (BTK_TYPE_HBOX, "visible", 1, "parent", handle_box2, NULL);
    label = btk_label_new ("Fooo!");
    btk_container_add (BTK_CONTAINER (hbox), label);
    btk_widget_show (label);
    g_object_new (BTK_TYPE_ARROW, "visible", 1, "parent", hbox, NULL);
  }

  if (!btk_widget_get_visible (window))
    btk_widget_show (window);
  else
    btk_widget_destroy (window);
}

/* 
 * Label Demo
 */
static void
sensitivity_toggled (BtkWidget *toggle,
                     BtkWidget *widget)
{
  btk_widget_set_sensitive (widget,  BTK_TOGGLE_BUTTON (toggle)->active);  
}

static BtkWidget*
create_sensitivity_control (BtkWidget *widget)
{
  BtkWidget *button;

  button = btk_toggle_button_new_with_label ("Sensitive");  

  btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (button),
                                btk_widget_is_sensitive (widget));
  
  g_signal_connect (button,
                    "toggled",
                    G_CALLBACK (sensitivity_toggled),
                    widget);
  
  btk_widget_show_all (button);

  return button;
}

static void
set_selectable_recursive (BtkWidget *widget,
                          gboolean   setting)
{
  if (BTK_IS_CONTAINER (widget))
    {
      GList *children;
      GList *tmp;
      
      children = btk_container_get_children (BTK_CONTAINER (widget));
      tmp = children;
      while (tmp)
        {
          set_selectable_recursive (tmp->data, setting);

          tmp = tmp->next;
        }
      g_list_free (children);
    }
  else if (BTK_IS_LABEL (widget))
    {
      btk_label_set_selectable (BTK_LABEL (widget), setting);
    }
}

static void
selectable_toggled (BtkWidget *toggle,
                    BtkWidget *widget)
{
  set_selectable_recursive (widget,
                            BTK_TOGGLE_BUTTON (toggle)->active);
}

static BtkWidget*
create_selectable_control (BtkWidget *widget)
{
  BtkWidget *button;

  button = btk_toggle_button_new_with_label ("Selectable");  

  btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (button),
                                FALSE);
  
  g_signal_connect (button,
                    "toggled",
                    G_CALLBACK (selectable_toggled),
                    widget);
  
  btk_widget_show_all (button);

  return button;
}

static void
dialog_response (BtkWidget *dialog, gint response_id, BtkLabel *label)
{
  const gchar *text;

  btk_widget_destroy (dialog);

  text = "Some <a href=\"http://en.wikipedia.org/wiki/Text\" title=\"plain text\">text</a> may be marked up\n"
         "as hyperlinks, which can be clicked\n"
         "or activated via <a href=\"keynav\">keynav</a>.\n"
         "The links remain the same.";
  btk_label_set_markup (label, text);
}

static gboolean
activate_link (BtkWidget *label, const gchar *uri, gpointer data)
{
  if (g_strcmp0 (uri, "keynav") == 0)
    {
      BtkWidget *dialog;

      dialog = btk_message_dialog_new_with_markup (BTK_WINDOW (btk_widget_get_toplevel (label)),
                                       BTK_DIALOG_DESTROY_WITH_PARENT,
                                       BTK_MESSAGE_INFO,
                                       BTK_BUTTONS_OK,
                                       "The term <i>keynav</i> is a shorthand for "
                                       "keyboard navigation and refers to the process of using a program "
                                       "(exclusively) via keyboard input.");

      btk_window_present (BTK_WINDOW (dialog));

      g_signal_connect (dialog, "response", G_CALLBACK (dialog_response), label);

      return TRUE;
    }

  return FALSE;
}

void create_labels (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  BtkWidget *hbox;
  BtkWidget *vbox;
  BtkWidget *frame;
  BtkWidget *label;
  BtkWidget *button;

  if (!window)
    {
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);

      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window);

      btk_window_set_title (BTK_WINDOW (window), "Label");

      vbox = btk_vbox_new (FALSE, 5);
      
      hbox = btk_hbox_new (FALSE, 5);
      btk_container_add (BTK_CONTAINER (window), vbox);

      btk_box_pack_end (BTK_BOX (vbox), hbox, FALSE, FALSE, 0);

      button = create_sensitivity_control (hbox);

      btk_box_pack_start (BTK_BOX (vbox), button, FALSE, FALSE, 0);

      button = create_selectable_control (hbox);

      btk_box_pack_start (BTK_BOX (vbox), button, FALSE, FALSE, 0);
      
      vbox = btk_vbox_new (FALSE, 5);
      
      btk_box_pack_start (BTK_BOX (hbox), vbox, FALSE, FALSE, 0);
      btk_container_set_border_width (BTK_CONTAINER (window), 5);

      frame = btk_frame_new ("Normal Label");
      label = btk_label_new ("This is a Normal label");
      btk_label_set_ellipsize (BTK_LABEL (label), BANGO_ELLIPSIZE_START);
      btk_container_add (BTK_CONTAINER (frame), label);
      btk_box_pack_start (BTK_BOX (vbox), frame, FALSE, FALSE, 0);

      frame = btk_frame_new ("Multi-line Label");
      label = btk_label_new ("This is a Multi-line label.\nSecond line\nThird line");
      btk_label_set_ellipsize (BTK_LABEL (label), BANGO_ELLIPSIZE_END);
      btk_container_add (BTK_CONTAINER (frame), label);
      btk_box_pack_start (BTK_BOX (vbox), frame, FALSE, FALSE, 0);

      frame = btk_frame_new ("Left Justified Label");
      label = btk_label_new ("This is a Left-Justified\nMulti-line label.\nThird      line");
      btk_label_set_ellipsize (BTK_LABEL (label), BANGO_ELLIPSIZE_MIDDLE);
      btk_label_set_justify (BTK_LABEL (label), BTK_JUSTIFY_LEFT);
      btk_container_add (BTK_CONTAINER (frame), label);
      btk_box_pack_start (BTK_BOX (vbox), frame, FALSE, FALSE, 0);

      frame = btk_frame_new ("Right Justified Label");
      btk_label_set_ellipsize (BTK_LABEL (label), BANGO_ELLIPSIZE_START);
      label = btk_label_new ("This is a Right-Justified\nMulti-line label.\nFourth line, (j/k)");
      btk_label_set_justify (BTK_LABEL (label), BTK_JUSTIFY_RIGHT);
      btk_container_add (BTK_CONTAINER (frame), label);
      btk_box_pack_start (BTK_BOX (vbox), frame, FALSE, FALSE, 0);

      frame = btk_frame_new ("Internationalized Label");
      label = btk_label_new (NULL);
      btk_label_set_markup (BTK_LABEL (label),
			    "French (Fran\303\247ais) Bonjour, Salut\n"
			    "Korean (\355\225\234\352\270\200)   \354\225\210\353\205\225\355\225\230\354\204\270\354\232\224, \354\225\210\353\205\225\355\225\230\354\213\255\353\213\210\352\271\214\n"
			    "Russian (\320\240\321\203\321\201\321\201\320\272\320\270\320\271) \320\227\320\264\321\200\320\260\320\262\321\201\321\202\320\262\321\203\320\271\321\202\320\265!\n"
			    "Chinese (Simplified) <span lang=\"zh-cn\">\345\205\203\346\260\224	\345\274\200\345\217\221</span>\n"
			    "Chinese (Traditional) <span lang=\"zh-tw\">\345\205\203\346\260\243	\351\226\213\347\231\274</span>\n"
			    "Japanese <span lang=\"ja\">\345\205\203\346\260\227	\351\226\213\347\231\272</span>");
      btk_label_set_justify (BTK_LABEL (label), BTK_JUSTIFY_LEFT);
      btk_container_add (BTK_CONTAINER (frame), label);
      btk_box_pack_start (BTK_BOX (vbox), frame, FALSE, FALSE, 0);

      frame = btk_frame_new ("Bidirection Label");
      label = btk_label_new ("\342\200\217Arabic	\330\247\331\204\330\263\331\204\330\247\331\205 \330\271\331\204\331\212\331\203\331\205\n"
			     "\342\200\217Hebrew	\327\251\327\234\327\225\327\235");
      btk_container_add (BTK_CONTAINER (frame), label);
      btk_box_pack_start (BTK_BOX (vbox), frame, FALSE, FALSE, 0);

      frame = btk_frame_new ("Links in a label");
      label = btk_label_new ("Some <a href=\"http://en.wikipedia.org/wiki/Text\" title=\"plain text\">text</a> may be marked up\n"
                             "as hyperlinks, which can be clicked\n"
                             "or activated via <a href=\"keynav\">keynav</a>");
      btk_label_set_use_markup (BTK_LABEL (label), TRUE);
      btk_container_add (BTK_CONTAINER (frame), label);
      btk_box_pack_start (BTK_BOX (vbox), frame, FALSE, FALSE, 0);
      g_signal_connect (label, "activate-link", G_CALLBACK (activate_link), NULL);

      vbox = btk_vbox_new (FALSE, 5);
      btk_box_pack_start (BTK_BOX (hbox), vbox, FALSE, FALSE, 0);
      frame = btk_frame_new ("Line wrapped label");
      label = btk_label_new ("This is an example of a line-wrapped label.  It should not be taking "\
			     "up the entire             "/* big space to test spacing */\
			     "width allocated to it, but automatically wraps the words to fit.  "\
			     "The time has come, for all good men, to come to the aid of their party.  "\
			     "The sixth sheik's six sheep's sick.\n"\
			     "     It supports multiple paragraphs correctly, and  correctly   adds "\
			     "many          extra  spaces. ");

      btk_label_set_line_wrap (BTK_LABEL (label), TRUE);
      btk_container_add (BTK_CONTAINER (frame), label);
      btk_box_pack_start (BTK_BOX (vbox), frame, FALSE, FALSE, 0);

      frame = btk_frame_new ("Filled, wrapped label");
      label = btk_label_new ("This is an example of a line-wrapped, filled label.  It should be taking "\
			     "up the entire              width allocated to it.  Here is a seneance to prove "\
			     "my point.  Here is another sentence. "\
			     "Here comes the sun, do de do de do.\n"\
			     "    This is a new paragraph.\n"\
			     "    This is another newer, longer, better paragraph.  It is coming to an end, "\
			     "unfortunately.");
      btk_label_set_justify (BTK_LABEL (label), BTK_JUSTIFY_FILL);
      btk_label_set_line_wrap (BTK_LABEL (label), TRUE);
      btk_container_add (BTK_CONTAINER (frame), label);
      btk_box_pack_start (BTK_BOX (vbox), frame, FALSE, FALSE, 0);

      frame = btk_frame_new ("Underlined label");
      label = btk_label_new ("This label is underlined!\n"
			     "This one is underlined (\343\201\223\343\202\223\343\201\253\343\201\241\343\201\257) in quite a funky fashion");
      btk_label_set_justify (BTK_LABEL (label), BTK_JUSTIFY_LEFT);
      btk_label_set_pattern (BTK_LABEL (label), "_________________________ _ _________ _ _____ _ __ __  ___ ____ _____");
      btk_container_add (BTK_CONTAINER (frame), label);
      btk_box_pack_start (BTK_BOX (vbox), frame, FALSE, FALSE, 0);

      frame = btk_frame_new ("Markup label");
      label = btk_label_new (NULL);

      /* There's also a btk_label_set_markup() without accel if you
       * don't have an accelerator key
       */
      btk_label_set_markup_with_mnemonic (BTK_LABEL (label),
					  "This <span foreground=\"blue\" background=\"orange\">label</span> has "
					  "<b>markup</b> _such as "
					  "<big><i>Big Italics</i></big>\n"
					  "<tt>Monospace font</tt>\n"
					  "<u>Underline!</u>\n"
					  "foo\n"
					  "<span foreground=\"green\" background=\"red\">Ugly colors</span>\n"
					  "and nothing on this line,\n"
					  "or this.\n"
					  "or this either\n"
					  "or even on this one\n"
					  "la <big>la <big>la <big>la <big>la</big></big></big></big>\n"
					  "but this _word is <span foreground=\"purple\"><big>purple</big></span>\n"
					  "<span underline=\"double\">We like <sup>superscript</sup> and <sub>subscript</sub> too</span>");

      g_assert (btk_label_get_mnemonic_keyval (BTK_LABEL (label)) == BDK_s);
      
      btk_container_add (BTK_CONTAINER (frame), label);
      btk_box_pack_start (BTK_BOX (vbox), frame, FALSE, FALSE, 0);
    }
  
  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    btk_widget_destroy (window);
}

static void
on_angle_scale_changed (BtkRange *range,
			BtkLabel *label)
{
  btk_label_set_angle (BTK_LABEL (label), btk_range_get_value (range));
}

static void
create_rotated_label (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  BtkWidget *vbox;
  BtkWidget *hscale;
  BtkWidget *label;  
  BtkWidget *scale_label;  
  BtkWidget *scale_hbox;  

  if (!window)
    {
      window = btk_dialog_new_with_buttons ("Rotated Label",
					    BTK_WINDOW (btk_widget_get_toplevel (widget)), 0,
					    BTK_STOCK_CLOSE, BTK_RESPONSE_CLOSE,
					    NULL);

      btk_window_set_resizable (BTK_WINDOW (window), TRUE);

      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));

      g_signal_connect (window, "response",
			G_CALLBACK (btk_object_destroy), NULL);
      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed), &window);

      vbox = btk_vbox_new (FALSE, 5);
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->vbox), vbox, TRUE, TRUE, 0);
      btk_container_set_border_width (BTK_CONTAINER (vbox), 10);

      label = btk_label_new (NULL);
      btk_label_set_markup (BTK_LABEL (label), "Hello World\n<i>Rotate</i> <span underline='single' foreground='blue'>me</span>");
      btk_box_pack_start (BTK_BOX (vbox), label, TRUE, TRUE, 0);

      scale_hbox = btk_hbox_new (FALSE, 0);
      btk_box_pack_start (BTK_BOX (vbox), scale_hbox, FALSE, FALSE, 0);
      
      scale_label = btk_label_new (NULL);
      btk_label_set_markup (BTK_LABEL (scale_label), "<i>Angle: </i>");
      btk_box_pack_start (BTK_BOX (scale_hbox), scale_label, FALSE, FALSE, 0);

      hscale = btk_hscale_new_with_range (0, 360, 5);
      g_signal_connect (hscale, "value-changed",
			G_CALLBACK (on_angle_scale_changed), label);
      
      btk_range_set_value (BTK_RANGE (hscale), 45);
      btk_widget_set_usize (hscale, 200, -1);
      btk_box_pack_start (BTK_BOX (scale_hbox), hscale, TRUE, TRUE, 0);
    }
  
  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    btk_widget_destroy (window);
}

#define DEFAULT_TEXT_RADIUS 200

static void
on_rotated_text_unrealize (BtkWidget *widget)
{
  g_object_set_data (B_OBJECT (widget), "text-gc", NULL);
}

static gboolean
on_rotated_text_expose (BtkWidget      *widget,
			BdkEventExpose *event,
			BdkPixbuf      *tile_pixbuf)
{
  static const gchar *words[] = { "The", "grand", "old", "Duke", "of", "York",
                                  "had", "10,000", "men" };
  int n_words;
  int i;
  double radius;
  BangoLayout *layout;
  BangoContext *context;
  BangoFontDescription *desc;
  bairo_t *cr;

  cr = bdk_bairo_create (event->window);

  if (tile_pixbuf)
    {
      bdk_bairo_set_source_pixbuf (cr, tile_pixbuf, 0, 0);
      bairo_pattern_set_extend (bairo_get_source (cr), BAIRO_EXTEND_REPEAT);
    }
  else
    bairo_set_source_rgb (cr, 0, 0, 0);

  radius = MIN (widget->allocation.width, widget->allocation.height) / 2.;

  bairo_translate (cr,
                   radius + (widget->allocation.width - 2 * radius) / 2,
                   radius + (widget->allocation.height - 2 * radius) / 2);
  bairo_scale (cr, radius / DEFAULT_TEXT_RADIUS, radius / DEFAULT_TEXT_RADIUS);

  context = btk_widget_get_bango_context (widget);
  layout = bango_layout_new (context);
  desc = bango_font_description_from_string ("Sans Bold 30");
  bango_layout_set_font_description (layout, desc);
  bango_font_description_free (desc);
    
  n_words = G_N_ELEMENTS (words);
  for (i = 0; i < n_words; i++)
    {
      int width, height;

      bairo_save (cr);

      bairo_rotate (cr, 2 * G_PI * i / n_words);
      bango_bairo_update_layout (cr, layout);

      bango_layout_set_text (layout, words[i], -1);
      bango_layout_get_size (layout, &width, &height);

      bairo_move_to (cr, - width / 2 / BANGO_SCALE, - DEFAULT_TEXT_RADIUS);
      bango_bairo_show_layout (cr, layout);

      bairo_restore (cr);
    }
  
  g_object_unref (layout);
  bairo_destroy (cr);

  return FALSE;
}

static void
create_rotated_text (BtkWidget *widget)
{
  static BtkWidget *window = NULL;

  if (!window)
    {
      const BdkColor white = { 0, 0xffff, 0xffff, 0xffff };
      BtkRequisition requisition;
      BtkWidget *drawing_area;
      BdkPixbuf *tile_pixbuf;

      window = btk_dialog_new_with_buttons ("Rotated Text",
					    BTK_WINDOW (btk_widget_get_toplevel (widget)), 0,
					    BTK_STOCK_CLOSE, BTK_RESPONSE_CLOSE,
					    NULL);

      btk_window_set_resizable (BTK_WINDOW (window), TRUE);

      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));

      g_signal_connect (window, "response",
			G_CALLBACK (btk_object_destroy), NULL);
      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed), &window);

      drawing_area = btk_drawing_area_new ();
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->vbox), drawing_area, TRUE, TRUE, 0);
      btk_widget_modify_bg (drawing_area, BTK_STATE_NORMAL, &white);

      tile_pixbuf = bdk_pixbuf_new_from_file ("marble.xpm", NULL);
      
      g_signal_connect (drawing_area, "expose-event",
			G_CALLBACK (on_rotated_text_expose), tile_pixbuf);
      g_signal_connect (drawing_area, "unrealize",
			G_CALLBACK (on_rotated_text_unrealize), NULL);

      btk_widget_show_all (BTK_BIN (window)->child);
      
      btk_widget_set_size_request (drawing_area, DEFAULT_TEXT_RADIUS * 2, DEFAULT_TEXT_RADIUS * 2);
      btk_widget_size_request (window, &requisition);
      btk_widget_set_size_request (drawing_area, -1, -1);
      btk_window_resize (BTK_WINDOW (window), requisition.width, requisition.height);
    }
  
  if (!btk_widget_get_visible (window))
    btk_widget_show (window);
  else
    btk_widget_destroy (window);
}

/*
 * Reparent demo
 */

static void
reparent_label (BtkWidget *widget,
		BtkWidget *new_parent)
{
  BtkWidget *label;

  label = g_object_get_data (B_OBJECT (widget), "user_data");

  btk_widget_reparent (label, new_parent);
}

static void
set_parent_signal (BtkWidget *child,
		   BtkWidget *old_parent,
		   gpointer   func_data)
{
  g_message ("set_parent for \"%s\": new parent: \"%s\", old parent: \"%s\", data: %d\n",
             g_type_name (B_OBJECT_TYPE (child)),
             child->parent ? g_type_name (B_OBJECT_TYPE (child->parent)) : "NULL",
             old_parent ? g_type_name (B_OBJECT_TYPE (old_parent)) : "NULL",
             GPOINTER_TO_INT (func_data));
}

static void
create_reparent (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  BtkWidget *box1;
  BtkWidget *box2;
  BtkWidget *box3;
  BtkWidget *frame;
  BtkWidget *button;
  BtkWidget *label;
  BtkWidget *separator;
  BtkWidget *event_box;

  if (!window)
    {
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);

      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window);

      btk_window_set_title (BTK_WINDOW (window), "reparent");
      btk_container_set_border_width (BTK_CONTAINER (window), 0);

      box1 = btk_vbox_new (FALSE, 0);
      btk_container_add (BTK_CONTAINER (window), box1);

      box2 = btk_hbox_new (FALSE, 5);
      btk_container_set_border_width (BTK_CONTAINER (box2), 10);
      btk_box_pack_start (BTK_BOX (box1), box2, TRUE, TRUE, 0);

      label = btk_label_new ("Hello World");

      frame = btk_frame_new ("Frame 1");
      btk_box_pack_start (BTK_BOX (box2), frame, TRUE, TRUE, 0);

      box3 = btk_vbox_new (FALSE, 5);
      btk_container_set_border_width (BTK_CONTAINER (box3), 5);
      btk_container_add (BTK_CONTAINER (frame), box3);

      button = btk_button_new_with_label ("switch");
      g_object_set_data (B_OBJECT (button), "user_data", label);
      btk_box_pack_start (BTK_BOX (box3), button, FALSE, TRUE, 0);

      event_box = btk_event_box_new ();
      btk_box_pack_start (BTK_BOX (box3), event_box, FALSE, TRUE, 0);
      btk_container_add (BTK_CONTAINER (event_box), label);
			 
      g_signal_connect (button, "clicked",
			G_CALLBACK (reparent_label),
			event_box);
      
      g_signal_connect (label, "parent_set",
			G_CALLBACK (set_parent_signal),
			GINT_TO_POINTER (42));

      frame = btk_frame_new ("Frame 2");
      btk_box_pack_start (BTK_BOX (box2), frame, TRUE, TRUE, 0);

      box3 = btk_vbox_new (FALSE, 5);
      btk_container_set_border_width (BTK_CONTAINER (box3), 5);
      btk_container_add (BTK_CONTAINER (frame), box3);

      button = btk_button_new_with_label ("switch");
      g_object_set_data (B_OBJECT (button), "user_data", label);
      btk_box_pack_start (BTK_BOX (box3), button, FALSE, TRUE, 0);
      
      event_box = btk_event_box_new ();
      btk_box_pack_start (BTK_BOX (box3), event_box, FALSE, TRUE, 0);

      g_signal_connect (button, "clicked",
			G_CALLBACK (reparent_label),
			event_box);

      separator = btk_hseparator_new ();
      btk_box_pack_start (BTK_BOX (box1), separator, FALSE, TRUE, 0);

      box2 = btk_vbox_new (FALSE, 10);
      btk_container_set_border_width (BTK_CONTAINER (box2), 10);
      btk_box_pack_start (BTK_BOX (box1), box2, FALSE, TRUE, 0);

      button = btk_button_new_with_label ("close");
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (btk_widget_destroy), window);
      btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);
      btk_widget_set_can_default (button, TRUE);
      btk_widget_grab_default (button);
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    btk_widget_destroy (window);
}

/*
 * Resize Grips
 */
static gboolean
grippy_button_press (BtkWidget *area, BdkEventButton *event, BdkWindowEdge edge)
{
  if (event->type == BDK_BUTTON_PRESS) 
    {
      if (event->button == 1)
	btk_window_begin_resize_drag (BTK_WINDOW (btk_widget_get_toplevel (area)), edge,
				      event->button, event->x_root, event->y_root,
				      event->time);
      else if (event->button == 2)
	btk_window_begin_move_drag (BTK_WINDOW (btk_widget_get_toplevel (area)), 
				    event->button, event->x_root, event->y_root,
				    event->time);
    }
  return TRUE;
}

static gboolean
grippy_expose (BtkWidget *area, BdkEventExpose *event, BdkWindowEdge edge)
{
  btk_paint_resize_grip (area->style,
			 area->window,
			 btk_widget_get_state (area),
			 &event->area,
			 area,
			 "statusbar",
			 edge,
			 0, 0,
			 area->allocation.width,
			 area->allocation.height);

  return TRUE;
}

static void
create_resize_grips (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  BtkWidget *area;
  BtkWidget *hbox, *vbox;
  if (!window)
    {
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);

      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));

      btk_window_set_title (BTK_WINDOW (window), "resize grips");
      
      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window);

      vbox = btk_vbox_new (FALSE, 0);
      btk_container_add (BTK_CONTAINER (window), vbox);
      
      hbox = btk_hbox_new (FALSE, 0);
      btk_box_pack_start (BTK_BOX (vbox), hbox, TRUE, TRUE, 0);

      /* North west */
      area = btk_drawing_area_new ();
      btk_widget_add_events (area, BDK_BUTTON_PRESS_MASK);
      btk_box_pack_start (BTK_BOX (hbox), area, TRUE, TRUE, 0);
      g_signal_connect (area, "expose_event", G_CALLBACK (grippy_expose),
			GINT_TO_POINTER (BDK_WINDOW_EDGE_NORTH_WEST));
      g_signal_connect (area, "button_press_event", G_CALLBACK (grippy_button_press),
			GINT_TO_POINTER (BDK_WINDOW_EDGE_NORTH_WEST));
      
      /* North */
      area = btk_drawing_area_new ();
      btk_widget_add_events (area, BDK_BUTTON_PRESS_MASK);
      btk_box_pack_start (BTK_BOX (hbox), area, TRUE, TRUE, 0);
      g_signal_connect (area, "expose_event", G_CALLBACK (grippy_expose),
			GINT_TO_POINTER (BDK_WINDOW_EDGE_NORTH));
      g_signal_connect (area, "button_press_event", G_CALLBACK (grippy_button_press),
			GINT_TO_POINTER (BDK_WINDOW_EDGE_NORTH));

      /* North east */
      area = btk_drawing_area_new ();
      btk_widget_add_events (area, BDK_BUTTON_PRESS_MASK);
      btk_box_pack_start (BTK_BOX (hbox), area, TRUE, TRUE, 0);
      g_signal_connect (area, "expose_event", G_CALLBACK (grippy_expose),
			GINT_TO_POINTER (BDK_WINDOW_EDGE_NORTH_EAST));
      g_signal_connect (area, "button_press_event", G_CALLBACK (grippy_button_press),
			GINT_TO_POINTER (BDK_WINDOW_EDGE_NORTH_EAST));

      hbox = btk_hbox_new (FALSE, 0);
      btk_box_pack_start (BTK_BOX (vbox), hbox, TRUE, TRUE, 0);

      /* West */
      area = btk_drawing_area_new ();
      btk_widget_add_events (area, BDK_BUTTON_PRESS_MASK);
      btk_box_pack_start (BTK_BOX (hbox), area, TRUE, TRUE, 0);
      g_signal_connect (area, "expose_event", G_CALLBACK (grippy_expose),
			GINT_TO_POINTER (BDK_WINDOW_EDGE_WEST));
      g_signal_connect (area, "button_press_event", G_CALLBACK (grippy_button_press),
			GINT_TO_POINTER (BDK_WINDOW_EDGE_WEST));

      /* Middle */
      area = btk_drawing_area_new ();
      btk_box_pack_start (BTK_BOX (hbox), area, TRUE, TRUE, 0);

      /* East */
      area = btk_drawing_area_new ();
      btk_widget_add_events (area, BDK_BUTTON_PRESS_MASK);
      btk_box_pack_start (BTK_BOX (hbox), area, TRUE, TRUE, 0);
      g_signal_connect (area, "expose_event", G_CALLBACK (grippy_expose),
			GINT_TO_POINTER (BDK_WINDOW_EDGE_EAST));
      g_signal_connect (area, "button_press_event", G_CALLBACK (grippy_button_press),
			GINT_TO_POINTER (BDK_WINDOW_EDGE_EAST));


      hbox = btk_hbox_new (FALSE, 0);
      btk_box_pack_start (BTK_BOX (vbox), hbox, TRUE, TRUE, 0);

      /* South west */
      area = btk_drawing_area_new ();
      btk_widget_add_events (area, BDK_BUTTON_PRESS_MASK);
      btk_box_pack_start (BTK_BOX (hbox), area, TRUE, TRUE, 0);
      g_signal_connect (area, "expose_event", G_CALLBACK (grippy_expose),
			GINT_TO_POINTER (BDK_WINDOW_EDGE_SOUTH_WEST));
      g_signal_connect (area, "button_press_event", G_CALLBACK (grippy_button_press),
			GINT_TO_POINTER (BDK_WINDOW_EDGE_SOUTH_WEST));
      /* South */
      area = btk_drawing_area_new ();
      btk_widget_add_events (area, BDK_BUTTON_PRESS_MASK);
      btk_box_pack_start (BTK_BOX (hbox), area, TRUE, TRUE, 0);
      g_signal_connect (area, "expose_event", G_CALLBACK (grippy_expose),
			GINT_TO_POINTER (BDK_WINDOW_EDGE_SOUTH));
      g_signal_connect (area, "button_press_event", G_CALLBACK (grippy_button_press),
			GINT_TO_POINTER (BDK_WINDOW_EDGE_SOUTH));
      
      /* South east */
      area = btk_drawing_area_new ();
      btk_widget_add_events (area, BDK_BUTTON_PRESS_MASK);
      btk_box_pack_start (BTK_BOX (hbox), area, TRUE, TRUE, 0);
      g_signal_connect (area, "expose_event", G_CALLBACK (grippy_expose),
			GINT_TO_POINTER (BDK_WINDOW_EDGE_SOUTH_EAST));
      g_signal_connect (area, "button_press_event", G_CALLBACK (grippy_button_press),
			GINT_TO_POINTER (BDK_WINDOW_EDGE_SOUTH_EAST));
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    btk_widget_destroy (window);
}

/*
 * Saved Position
 */
gint upositionx = 0;
gint upositiony = 0;

static gint
uposition_configure (BtkWidget *window)
{
  BtkLabel *lx;
  BtkLabel *ly;
  gchar buffer[64];

  lx = g_object_get_data (B_OBJECT (window), "x");
  ly = g_object_get_data (B_OBJECT (window), "y");

  bdk_window_get_root_origin (window->window, &upositionx, &upositiony);
  sprintf (buffer, "%d", upositionx);
  btk_label_set_text (lx, buffer);
  sprintf (buffer, "%d", upositiony);
  btk_label_set_text (ly, buffer);

  return FALSE;
}

static void
uposition_stop_configure (BtkToggleButton *toggle,
			  BtkObject       *window)
{
  if (toggle->active)
    g_signal_handlers_block_by_func (window, G_CALLBACK (uposition_configure), NULL);
  else
    g_signal_handlers_unblock_by_func (window, G_CALLBACK (uposition_configure), NULL);
}

static void
create_saved_position (BtkWidget *widget)
{
  static BtkWidget *window = NULL;

  if (!window)
    {
      BtkWidget *hbox;
      BtkWidget *main_vbox;
      BtkWidget *vbox;
      BtkWidget *x_label;
      BtkWidget *y_label;
      BtkWidget *button;
      BtkWidget *label;
      BtkWidget *any;

      window = g_object_connect (g_object_new (BTK_TYPE_WINDOW,
						 "type", BTK_WINDOW_TOPLEVEL,
						 "title", "Saved Position",
						 NULL),
				 "signal::configure_event", uposition_configure, NULL,
				 NULL);

      btk_window_move (BTK_WINDOW (window), upositionx, upositiony);

      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));
      

      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window);

      main_vbox = btk_vbox_new (FALSE, 5);
      btk_container_set_border_width (BTK_CONTAINER (main_vbox), 0);
      btk_container_add (BTK_CONTAINER (window), main_vbox);

      vbox =
	g_object_new (btk_vbox_get_type (),
			"BtkBox::homogeneous", FALSE,
			"BtkBox::spacing", 5,
			"BtkContainer::border_width", 10,
			"BtkWidget::parent", main_vbox,
			"BtkWidget::visible", TRUE,
			"child", g_object_connect (g_object_new (BTK_TYPE_TOGGLE_BUTTON,
								   "label", "Stop Events",
								   "active", FALSE,
								   "visible", TRUE,
								   NULL),
						   "signal::clicked", uposition_stop_configure, window,
						   NULL),
			NULL);

      hbox = btk_hbox_new (FALSE, 0);
      btk_container_set_border_width (BTK_CONTAINER (hbox), 5);
      btk_box_pack_start (BTK_BOX (vbox), hbox, FALSE, TRUE, 0);

      label = btk_label_new ("X Origin : ");
      btk_misc_set_alignment (BTK_MISC (label), 0, 0.5);
      btk_box_pack_start (BTK_BOX (hbox), label, FALSE, TRUE, 0);

      x_label = btk_label_new ("");
      btk_box_pack_start (BTK_BOX (hbox), x_label, TRUE, TRUE, 0);
      g_object_set_data (B_OBJECT (window), "x", x_label);

      hbox = btk_hbox_new (FALSE, 0);
      btk_container_set_border_width (BTK_CONTAINER (hbox), 5);
      btk_box_pack_start (BTK_BOX (vbox), hbox, FALSE, TRUE, 0);

      label = btk_label_new ("Y Origin : ");
      btk_misc_set_alignment (BTK_MISC (label), 0, 0.5);
      btk_box_pack_start (BTK_BOX (hbox), label, FALSE, TRUE, 0);

      y_label = btk_label_new ("");
      btk_box_pack_start (BTK_BOX (hbox), y_label, TRUE, TRUE, 0);
      g_object_set_data (B_OBJECT (window), "y", y_label);

      any =
	g_object_new (btk_hseparator_get_type (),
			"BtkWidget::visible", TRUE,
			NULL);
      btk_box_pack_start (BTK_BOX (main_vbox), any, FALSE, TRUE, 0);

      hbox = btk_hbox_new (FALSE, 0);
      btk_container_set_border_width (BTK_CONTAINER (hbox), 10);
      btk_box_pack_start (BTK_BOX (main_vbox), hbox, FALSE, TRUE, 0);

      button = btk_button_new_with_label ("Close");
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (btk_widget_destroy),
				window);
      btk_box_pack_start (BTK_BOX (hbox), button, TRUE, TRUE, 5);
      btk_widget_set_can_default (button, TRUE);
      btk_widget_grab_default (button);
      
      btk_widget_show_all (window);
    }
  else
    btk_widget_destroy (window);
}

/*
 * BtkPixmap
 */

static void
create_pixmap (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  BtkWidget *box1;
  BtkWidget *box2;
  BtkWidget *box3;
  BtkWidget *button;
  BtkWidget *label;
  BtkWidget *separator;
  BtkWidget *pixmapwid;

  if (!window)
    {
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);

      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
                        G_CALLBACK (btk_widget_destroyed),
                        &window);

      btk_window_set_title (BTK_WINDOW (window), "BtkPixmap");
      btk_container_set_border_width (BTK_CONTAINER (window), 0);
      btk_widget_realize(window);

      box1 = btk_vbox_new (FALSE, 0);
      btk_container_add (BTK_CONTAINER (window), box1);

      box2 = btk_vbox_new (FALSE, 10);
      btk_container_set_border_width (BTK_CONTAINER (box2), 10);
      btk_box_pack_start (BTK_BOX (box1), box2, TRUE, TRUE, 0);

      button = btk_button_new ();
      btk_box_pack_start (BTK_BOX (box2), button, FALSE, FALSE, 0);

      pixmapwid = new_pixmap ("test.xpm", window->window, NULL);

      label = btk_label_new ("Pixmap\ntest");
      box3 = btk_hbox_new (FALSE, 0);
      btk_container_set_border_width (BTK_CONTAINER (box3), 2);
      btk_container_add (BTK_CONTAINER (box3), pixmapwid);
      btk_container_add (BTK_CONTAINER (box3), label);
      btk_container_add (BTK_CONTAINER (button), box3);

      button = btk_button_new ();
      btk_box_pack_start (BTK_BOX (box2), button, FALSE, FALSE, 0);
      
      pixmapwid = new_pixmap ("test.xpm", window->window, NULL);

      label = btk_label_new ("Pixmap\ntest");
      box3 = btk_hbox_new (FALSE, 0);
      btk_container_set_border_width (BTK_CONTAINER (box3), 2);
      btk_container_add (BTK_CONTAINER (box3), pixmapwid);
      btk_container_add (BTK_CONTAINER (box3), label);
      btk_container_add (BTK_CONTAINER (button), box3);

      btk_widget_set_sensitive (button, FALSE);
      
      separator = btk_hseparator_new ();
      btk_box_pack_start (BTK_BOX (box1), separator, FALSE, TRUE, 0);

      box2 = btk_vbox_new (FALSE, 10);
      btk_container_set_border_width (BTK_CONTAINER (box2), 10);
      btk_box_pack_start (BTK_BOX (box1), box2, FALSE, TRUE, 0);

      button = btk_button_new_with_label ("close");
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (btk_widget_destroy),
				window);
      btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);
      btk_widget_set_can_default (button, TRUE);
      btk_widget_grab_default (button);
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    btk_widget_destroy (window);
}

static void
tips_query_widget_entered (BtkTipsQuery   *tips_query,
			   BtkWidget      *widget,
			   const gchar    *tip_text,
			   const gchar    *tip_private,
			   BtkWidget	  *toggle)
{
  if (BTK_TOGGLE_BUTTON (toggle)->active)
    {
      btk_label_set_text (BTK_LABEL (tips_query), tip_text ? "There is a Tip!" : "There is no Tip!");
      /* don't let BtkTipsQuery reset its label */
      g_signal_stop_emission_by_name (tips_query, "widget_entered");
    }
}

static gint
tips_query_widget_selected (BtkWidget      *tips_query,
			    BtkWidget      *widget,
			    const gchar    *tip_text,
			    const gchar    *tip_private,
			    BdkEventButton *event,
			    gpointer        func_data)
{
  if (widget)
    g_print ("Help \"%s\" requested for <%s>\n",
	     tip_private ? tip_private : "None",
	     g_type_name (B_OBJECT_TYPE (widget)));
  return TRUE;
}

static void
create_tooltips (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  BtkWidget *box1;
  BtkWidget *box2;
  BtkWidget *box3;
  BtkWidget *button;
  BtkWidget *toggle;
  BtkWidget *frame;
  BtkWidget *tips_query;
  BtkWidget *separator;
  BtkTooltips *tooltips;

  if (!window)
    {
      window =
	g_object_new (btk_window_get_type (),
			"BtkWindow::type", BTK_WINDOW_TOPLEVEL,
			"BtkContainer::border_width", 0,
			"BtkWindow::title", "Tooltips",
			"BtkWindow::allow_shrink", TRUE,
			"BtkWindow::allow_grow", FALSE,
			NULL);

      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
                        G_CALLBACK (destroy_tooltips),
                        &window);

      tooltips=btk_tooltips_new();
      g_object_ref (tooltips);
      btk_object_sink (BTK_OBJECT (tooltips));
      g_object_set_data (B_OBJECT (window), "tooltips", tooltips);
      
      box1 = btk_vbox_new (FALSE, 0);
      btk_container_add (BTK_CONTAINER (window), box1);

      box2 = btk_vbox_new (FALSE, 10);
      btk_container_set_border_width (BTK_CONTAINER (box2), 10);
      btk_box_pack_start (BTK_BOX (box1), box2, TRUE, TRUE, 0);

      button = btk_toggle_button_new_with_label ("button1");
      btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);

      btk_tooltips_set_tip (tooltips,
			    button,
			    "This is button 1",
			    "ContextHelp/buttons/1");

      button = btk_toggle_button_new_with_label ("button2");
      btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);

      btk_tooltips_set_tip (tooltips,
			    button,
			    "This is button 2. This is also a really long tooltip which probably won't fit on a single line and will therefore need to be wrapped. Hopefully the wrapping will work correctly.",
			    "ContextHelp/buttons/2_long");

      toggle = btk_toggle_button_new_with_label ("Override TipsQuery Label");
      btk_box_pack_start (BTK_BOX (box2), toggle, TRUE, TRUE, 0);

      btk_tooltips_set_tip (tooltips,
			    toggle,
			    "Toggle TipsQuery view.",
			    "Hi msw! ;)");

      box3 =
	g_object_new (btk_vbox_get_type (),
			"homogeneous", FALSE,
			"spacing", 5,
			"border_width", 5,
			"visible", TRUE,
			NULL);

      tips_query = btk_tips_query_new ();

      button =
	g_object_new (btk_button_get_type (),
			"label", "[?]",
			"visible", TRUE,
			"parent", box3,
			NULL);
      g_object_connect (button,
			"swapped_signal::clicked", btk_tips_query_start_query, tips_query,
			NULL);
      btk_box_set_child_packing (BTK_BOX (box3), button, FALSE, FALSE, 0, BTK_PACK_START);
      btk_tooltips_set_tip (tooltips,
			    button,
			    "Start the Tooltips Inspector",
			    "ContextHelp/buttons/?");
      
      
      g_object_set (g_object_connect (tips_query,
				      "signal::widget_entered", tips_query_widget_entered, toggle,
				      "signal::widget_selected", tips_query_widget_selected, NULL,
				      NULL),
		    "visible", TRUE,
		    "parent", box3,
		    "caller", button,
		    NULL);
      
      frame = g_object_new (btk_frame_get_type (),
			      "label", "ToolTips Inspector",
			      "label_xalign", (double) 0.5,
			      "border_width", 0,
			      "visible", TRUE,
			      "parent", box2,
			      "child", box3,
			      NULL);
      btk_box_set_child_packing (BTK_BOX (box2), frame, TRUE, TRUE, 10, BTK_PACK_START);

      separator = btk_hseparator_new ();
      btk_box_pack_start (BTK_BOX (box1), separator, FALSE, TRUE, 0);

      box2 = btk_vbox_new (FALSE, 10);
      btk_container_set_border_width (BTK_CONTAINER (box2), 10);
      btk_box_pack_start (BTK_BOX (box1), box2, FALSE, TRUE, 0);

      button = btk_button_new_with_label ("close");
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (btk_widget_destroy),
				window);
      btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);
      btk_widget_set_can_default (button, TRUE);
      btk_widget_grab_default (button);

      btk_tooltips_set_tip (tooltips, button, "Push this button to close window", "ContextHelp/buttons/Close");
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    btk_widget_destroy (window);
}

/*
 * BtkImage
 */

static void
pack_image (BtkWidget *box,
            const gchar *text,
            BtkWidget *image)
{
  btk_box_pack_start (BTK_BOX (box),
                      btk_label_new (text),
                      FALSE, FALSE, 0);

  btk_box_pack_start (BTK_BOX (box),
                      image,
                      TRUE, TRUE, 0);  
}

static void
create_image (BtkWidget *widget)
{
  static BtkWidget *window = NULL;

  if (window == NULL)
    {
      BtkWidget *vbox;
      BdkPixmap *pixmap;
      BdkBitmap *mask;
        
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      
      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));

      /* this is bogus for testing drawing when allocation < request,
       * don't copy into real code
       */
      g_object_set (window, "allow_shrink", TRUE, "allow_grow", TRUE, NULL);
      
      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window);

      vbox = btk_vbox_new (FALSE, 5);

      btk_container_add (BTK_CONTAINER (window), vbox);

      pack_image (vbox, "Stock Warning Dialog",
                  btk_image_new_from_stock (BTK_STOCK_DIALOG_WARNING,
                                            BTK_ICON_SIZE_DIALOG));

      pixmap = bdk_pixmap_colormap_create_from_xpm_d (NULL,
                                                      btk_widget_get_colormap (window),
                                                      &mask,
                                                      NULL,
                                                      openfile);
      
      pack_image (vbox, "Pixmap",
                  btk_image_new_from_pixmap (pixmap, mask));
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    btk_widget_destroy (window);
}
     
/*
 * Menu demo
 */

static BtkWidget*
create_menu (BdkScreen *screen, gint depth, gint length, gboolean tearoff)
{
  BtkWidget *menu;
  BtkWidget *menuitem;
  BtkWidget *image;
  GSList *group;
  char buf[32];
  int i, j;

  if (depth < 1)
    return NULL;

  menu = btk_menu_new ();
  btk_menu_set_screen (BTK_MENU (menu), screen);

  group = NULL;

  if (tearoff)
    {
      menuitem = btk_tearoff_menu_item_new ();
      btk_menu_shell_append (BTK_MENU_SHELL (menu), menuitem);
      btk_widget_show (menuitem);
    }

  image = btk_image_new_from_stock (BTK_STOCK_OPEN,
                                    BTK_ICON_SIZE_MENU);
  btk_widget_show (image);
  menuitem = btk_image_menu_item_new_with_label ("Image item");
  btk_image_menu_item_set_image (BTK_IMAGE_MENU_ITEM (menuitem), image);
  btk_menu_shell_append (BTK_MENU_SHELL (menu), menuitem);
  btk_widget_show (menuitem);
  
  for (i = 0, j = 1; i < length; i++, j++)
    {
      sprintf (buf, "item %2d - %d", depth, j);

      menuitem = btk_radio_menu_item_new_with_label (group, buf);
      group = btk_radio_menu_item_get_group (BTK_RADIO_MENU_ITEM (menuitem));

#if 0
      if (depth % 2)
	btk_check_menu_item_set_show_toggle (BTK_CHECK_MENU_ITEM (menuitem), TRUE);
#endif

      btk_menu_shell_append (BTK_MENU_SHELL (menu), menuitem);
      btk_widget_show (menuitem);
      if (i == 3)
	btk_widget_set_sensitive (menuitem, FALSE);

      if (i == 5)
        btk_check_menu_item_set_inconsistent (BTK_CHECK_MENU_ITEM (menuitem),
                                              TRUE);

      if (i < 5)
	btk_menu_item_set_submenu (BTK_MENU_ITEM (menuitem), 
				   create_menu (screen, depth - 1, 5,  TRUE));
    }

  return menu;
}

static BtkWidget*
create_table_menu (BdkScreen *screen, gint cols, gint rows, gboolean tearoff)
{
  BtkWidget *menu;
  BtkWidget *menuitem;
  BtkWidget *submenu;
  BtkWidget *image;
  char buf[32];
  int i, j;

  menu = btk_menu_new ();
  btk_menu_set_screen (BTK_MENU (menu), screen);

  j = 0;
  if (tearoff)
    {
      menuitem = btk_tearoff_menu_item_new ();
      btk_menu_attach (BTK_MENU (menu), menuitem, 0, cols, j, j + 1);
      btk_widget_show (menuitem);
      j++;
    }
  
  menuitem = btk_menu_item_new_with_label ("items");
  btk_menu_attach (BTK_MENU (menu), menuitem, 0, cols, j, j + 1);

  submenu = btk_menu_new ();
  btk_menu_set_screen (BTK_MENU (submenu), screen);
  btk_menu_item_set_submenu (BTK_MENU_ITEM (menuitem), submenu);
  btk_widget_show (menuitem);
  j++;

  /* now fill the items submenu */
  image = btk_image_new_from_stock (BTK_STOCK_HELP,
				    BTK_ICON_SIZE_MENU);
  btk_widget_show (image);
  menuitem = btk_image_menu_item_new_with_label ("Image");
  btk_image_menu_item_set_image (BTK_IMAGE_MENU_ITEM (menuitem), image);
  btk_menu_attach (BTK_MENU (submenu), menuitem, 0, 1, 0, 1);
  btk_widget_show (menuitem);

  menuitem = btk_menu_item_new_with_label ("x");
  btk_menu_attach (BTK_MENU (submenu), menuitem, 1, 2, 0, 1);
  btk_widget_show (menuitem);

  menuitem = btk_menu_item_new_with_label ("x");
  btk_menu_attach (BTK_MENU (submenu), menuitem, 0, 1, 1, 2);
  btk_widget_show (menuitem);
  
  image = btk_image_new_from_stock (BTK_STOCK_HELP,
				    BTK_ICON_SIZE_MENU);
  btk_widget_show (image);
  menuitem = btk_image_menu_item_new_with_label ("Image");
  btk_image_menu_item_set_image (BTK_IMAGE_MENU_ITEM (menuitem), image);
  btk_menu_attach (BTK_MENU (submenu), menuitem, 1, 2, 1, 2);
  btk_widget_show (menuitem);

  menuitem = btk_radio_menu_item_new_with_label (NULL, "Radio");
  btk_menu_attach (BTK_MENU (submenu), menuitem, 0, 1, 2, 3);
  btk_widget_show (menuitem);

  menuitem = btk_menu_item_new_with_label ("x");
  btk_menu_attach (BTK_MENU (submenu), menuitem, 1, 2, 2, 3);
  btk_widget_show (menuitem);

  menuitem = btk_menu_item_new_with_label ("x");
  btk_menu_attach (BTK_MENU (submenu), menuitem, 0, 1, 3, 4);
  btk_widget_show (menuitem);
  
  menuitem = btk_radio_menu_item_new_with_label (NULL, "Radio");
  btk_menu_attach (BTK_MENU (submenu), menuitem, 1, 2, 3, 4);
  btk_widget_show (menuitem);

  menuitem = btk_check_menu_item_new_with_label ("Check");
  btk_menu_attach (BTK_MENU (submenu), menuitem, 0, 1, 4, 5);
  btk_widget_show (menuitem);

  menuitem = btk_menu_item_new_with_label ("x");
  btk_menu_attach (BTK_MENU (submenu), menuitem, 1, 2, 4, 5);
  btk_widget_show (menuitem);

  menuitem = btk_menu_item_new_with_label ("x");
  btk_menu_attach (BTK_MENU (submenu), menuitem, 0, 1, 5, 6);
  btk_widget_show (menuitem);
  
  menuitem = btk_check_menu_item_new_with_label ("Check");
  btk_menu_attach (BTK_MENU (submenu), menuitem, 1, 2, 5, 6);
  btk_widget_show (menuitem);

  menuitem = btk_menu_item_new_with_label ("1. Inserted normally (8)");
  btk_widget_show (menuitem);
  btk_menu_shell_insert (BTK_MENU_SHELL (submenu), menuitem, 8);

  menuitem = btk_menu_item_new_with_label ("2. Inserted normally (2)");
  btk_widget_show (menuitem);
  btk_menu_shell_insert (BTK_MENU_SHELL (submenu), menuitem, 2);

  menuitem = btk_menu_item_new_with_label ("3. Inserted normally (0)");
  btk_widget_show (menuitem);
  btk_menu_shell_insert (BTK_MENU_SHELL (submenu), menuitem, 0);

  menuitem = btk_menu_item_new_with_label ("4. Inserted normally (-1)");
  btk_widget_show (menuitem);
  btk_menu_shell_insert (BTK_MENU_SHELL (submenu), menuitem, -1);
  
  /* end of items submenu */

  menuitem = btk_menu_item_new_with_label ("spanning");
  btk_menu_attach (BTK_MENU (menu), menuitem, 0, cols, j, j + 1);

  submenu = btk_menu_new ();
  btk_menu_set_screen (BTK_MENU (submenu), screen);
  btk_menu_item_set_submenu (BTK_MENU_ITEM (menuitem), submenu);
  btk_widget_show (menuitem);
  j++;

  /* now fill the spanning submenu */
  menuitem = btk_menu_item_new_with_label ("a");
  btk_menu_attach (BTK_MENU (submenu), menuitem, 0, 2, 0, 1);
  btk_widget_show (menuitem);

  menuitem = btk_menu_item_new_with_label ("b");
  btk_menu_attach (BTK_MENU (submenu), menuitem, 2, 3, 0, 2);
  btk_widget_show (menuitem);

  menuitem = btk_menu_item_new_with_label ("c");
  btk_menu_attach (BTK_MENU (submenu), menuitem, 0, 1, 1, 3);
  btk_widget_show (menuitem);

  menuitem = btk_menu_item_new_with_label ("d");
  btk_menu_attach (BTK_MENU (submenu), menuitem, 1, 2, 1, 2);
  btk_widget_show (menuitem);

  menuitem = btk_menu_item_new_with_label ("e");
  btk_menu_attach (BTK_MENU (submenu), menuitem, 1, 3, 2, 3);
  btk_widget_show (menuitem);
  /* end of spanning submenu */
  
  menuitem = btk_menu_item_new_with_label ("left");
  btk_menu_attach (BTK_MENU (menu), menuitem, 0, 1, j, j + 1);
  submenu = btk_menu_new ();
  btk_menu_set_screen (BTK_MENU (submenu), screen);
  btk_menu_item_set_submenu (BTK_MENU_ITEM (menuitem), submenu);
  btk_widget_show (menuitem);

  menuitem = btk_menu_item_new_with_label ("Empty");
  btk_menu_attach (BTK_MENU (submenu), menuitem, 0, 1, 0, 1);
  submenu = btk_menu_new ();
  btk_menu_set_screen (BTK_MENU (submenu), screen);
  btk_menu_item_set_submenu (BTK_MENU_ITEM (menuitem), submenu);
  btk_widget_show (menuitem);

  menuitem = btk_menu_item_new_with_label ("right");
  btk_menu_attach (BTK_MENU (menu), menuitem, 1, 2, j, j + 1);
  submenu = btk_menu_new ();
  btk_menu_set_screen (BTK_MENU (submenu), screen);
  btk_menu_item_set_submenu (BTK_MENU_ITEM (menuitem), submenu);
  btk_widget_show (menuitem);

  menuitem = btk_menu_item_new_with_label ("Empty");
  btk_menu_attach (BTK_MENU (submenu), menuitem, 0, 1, 0, 1);
  btk_widget_show (menuitem);

  j++;

  for (; j < rows; j++)
      for (i = 0; i < cols; i++)
      {
	sprintf (buf, "(%d %d)", i, j);
	menuitem = btk_menu_item_new_with_label (buf);
	btk_menu_attach (BTK_MENU (menu), menuitem, i, i + 1, j, j + 1);
	btk_widget_show (menuitem);
      }
  
  menuitem = btk_menu_item_new_with_label ("1. Inserted normally (8)");
  btk_menu_shell_insert (BTK_MENU_SHELL (menu), menuitem, 8);
  btk_widget_show (menuitem);
  menuitem = btk_menu_item_new_with_label ("2. Inserted normally (2)");
  btk_menu_shell_insert (BTK_MENU_SHELL (menu), menuitem, 2);
  btk_widget_show (menuitem);
  menuitem = btk_menu_item_new_with_label ("3. Inserted normally (0)");
  btk_menu_shell_insert (BTK_MENU_SHELL (menu), menuitem, 0);
  btk_widget_show (menuitem);
  menuitem = btk_menu_item_new_with_label ("4. Inserted normally (-1)");
  btk_menu_shell_insert (BTK_MENU_SHELL (menu), menuitem, -1);
  btk_widget_show (menuitem);
  
  return menu;
}

static void
create_menus (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  BtkWidget *box1;
  BtkWidget *box2;
  BtkWidget *button;
  BtkWidget *optionmenu;
  BtkWidget *separator;
  
  if (!window)
    {
      BtkWidget *menubar;
      BtkWidget *menu;
      BtkWidget *menuitem;
      BtkAccelGroup *accel_group;
      BtkWidget *image;
      BdkScreen *screen = btk_widget_get_screen (widget);
      
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);

      btk_window_set_screen (BTK_WINDOW (window), screen);
      
      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window);
      g_signal_connect (window, "delete-event",
			G_CALLBACK (btk_true),
			NULL);
      
      accel_group = btk_accel_group_new ();
      btk_window_add_accel_group (BTK_WINDOW (window), accel_group);

      btk_window_set_title (BTK_WINDOW (window), "menus");
      btk_container_set_border_width (BTK_CONTAINER (window), 0);
      
      
      box1 = btk_vbox_new (FALSE, 0);
      btk_container_add (BTK_CONTAINER (window), box1);
      btk_widget_show (box1);
      
      menubar = btk_menu_bar_new ();
      btk_box_pack_start (BTK_BOX (box1), menubar, FALSE, TRUE, 0);
      btk_widget_show (menubar);
      
      menu = create_menu (screen, 2, 50, TRUE);
      
      menuitem = btk_menu_item_new_with_label ("test\nline2");
      btk_menu_item_set_submenu (BTK_MENU_ITEM (menuitem), menu);
      btk_menu_shell_append (BTK_MENU_SHELL (menubar), menuitem);
      btk_widget_show (menuitem);

      menu = create_table_menu (screen, 2, 50, TRUE);
      
      menuitem = btk_menu_item_new_with_label ("table");
      btk_menu_item_set_submenu (BTK_MENU_ITEM (menuitem), menu);
      btk_menu_shell_append (BTK_MENU_SHELL (menubar), menuitem);
      btk_widget_show (menuitem);
      
      menuitem = btk_menu_item_new_with_label ("foo");
      btk_menu_item_set_submenu (BTK_MENU_ITEM (menuitem), create_menu (screen, 3, 5, TRUE));
      btk_menu_shell_append (BTK_MENU_SHELL (menubar), menuitem);
      btk_widget_show (menuitem);

      image = btk_image_new_from_stock (BTK_STOCK_HELP,
                                        BTK_ICON_SIZE_MENU);
      btk_widget_show (image);
      menuitem = btk_image_menu_item_new_with_label ("Help");
      btk_image_menu_item_set_image (BTK_IMAGE_MENU_ITEM (menuitem), image);
      btk_menu_item_set_submenu (BTK_MENU_ITEM (menuitem), create_menu (screen, 4, 5, TRUE));
      btk_menu_item_set_right_justified (BTK_MENU_ITEM (menuitem), TRUE);
      btk_menu_shell_append (BTK_MENU_SHELL (menubar), menuitem);
      btk_widget_show (menuitem);
      
      menubar = btk_menu_bar_new ();
      btk_box_pack_start (BTK_BOX (box1), menubar, FALSE, TRUE, 0);
      btk_widget_show (menubar);
      
      menu = create_menu (screen, 2, 10, TRUE);
      
      menuitem = btk_menu_item_new_with_label ("Second menu bar");
      btk_menu_item_set_submenu (BTK_MENU_ITEM (menuitem), menu);
      btk_menu_shell_append (BTK_MENU_SHELL (menubar), menuitem);
      btk_widget_show (menuitem);
      
      box2 = btk_vbox_new (FALSE, 10);
      btk_container_set_border_width (BTK_CONTAINER (box2), 10);
      btk_box_pack_start (BTK_BOX (box1), box2, TRUE, TRUE, 0);
      btk_widget_show (box2);
      
      menu = create_menu (screen, 1, 5, FALSE);
      btk_menu_set_accel_group (BTK_MENU (menu), accel_group);

      menuitem = btk_image_menu_item_new_from_stock (BTK_STOCK_NEW, accel_group);
      btk_menu_shell_append (BTK_MENU_SHELL (menu), menuitem);
      btk_widget_show (menuitem);
      
      menuitem = btk_check_menu_item_new_with_label ("Accelerate Me");
      btk_menu_shell_append (BTK_MENU_SHELL (menu), menuitem);
      btk_widget_show (menuitem);
      btk_widget_add_accelerator (menuitem,
				  "activate",
				  accel_group,
				  BDK_F1,
				  0,
				  BTK_ACCEL_VISIBLE);
      menuitem = btk_check_menu_item_new_with_label ("Accelerator Locked");
      btk_menu_shell_append (BTK_MENU_SHELL (menu), menuitem);
      btk_widget_show (menuitem);
      btk_widget_add_accelerator (menuitem,
				  "activate",
				  accel_group,
				  BDK_F2,
				  0,
				  BTK_ACCEL_VISIBLE | BTK_ACCEL_LOCKED);
      menuitem = btk_check_menu_item_new_with_label ("Accelerators Frozen");
      btk_menu_shell_append (BTK_MENU_SHELL (menu), menuitem);
      btk_widget_show (menuitem);
      btk_widget_add_accelerator (menuitem,
				  "activate",
				  accel_group,
				  BDK_F2,
				  0,
				  BTK_ACCEL_VISIBLE);
      btk_widget_add_accelerator (menuitem,
				  "activate",
				  accel_group,
				  BDK_F3,
				  0,
				  BTK_ACCEL_VISIBLE);
      
      optionmenu = btk_option_menu_new ();
      btk_option_menu_set_menu (BTK_OPTION_MENU (optionmenu), menu);
      btk_option_menu_set_history (BTK_OPTION_MENU (optionmenu), 3);
      btk_box_pack_start (BTK_BOX (box2), optionmenu, TRUE, TRUE, 0);
      btk_widget_show (optionmenu);

      separator = btk_hseparator_new ();
      btk_box_pack_start (BTK_BOX (box1), separator, FALSE, TRUE, 0);
      btk_widget_show (separator);

      box2 = btk_vbox_new (FALSE, 10);
      btk_container_set_border_width (BTK_CONTAINER (box2), 10);
      btk_box_pack_start (BTK_BOX (box1), box2, FALSE, TRUE, 0);
      btk_widget_show (box2);

      button = btk_button_new_with_label ("close");
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (btk_widget_destroy),
				window);
      btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);
      btk_widget_set_can_default (button, TRUE);
      btk_widget_grab_default (button);
      btk_widget_show (button);
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show (window);
  else
    btk_widget_destroy (window);
}

static void
btk_ifactory_cb (gpointer             callback_data,
		 guint                callback_action,
		 BtkWidget           *widget)
{
  g_message ("ItemFactory: activated \"%s\"", btk_item_factory_path_from_widget (widget));
}

/* BdkPixbuf RGBA C-Source image dump */

static const guint8 apple[] = 
{ ""
  /* Pixbuf magic (0x47646b50) */
  "BdkP"
  /* length: header (24) + pixel_data (2304) */
  "\0\0\11\30"
  /* pixdata_type (0x1010002) */
  "\1\1\0\2"
  /* rowstride (96) */
  "\0\0\0`"
  /* width (24) */
  "\0\0\0\30"
  /* height (24) */
  "\0\0\0\30"
  /* pixel_data: */
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\26\24"
  "\17\11\0\0\0\2\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0`m"
  "[pn{a\344hv_\345_k[`\0\0\0\0\0\0\0\0\0\0\0\0D>/\305\0\0\0_\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0`l[Blza\373s\202d\354w\206g\372p~c"
  "\374`l[y\0\0\0\0[S\77/\27\25\17\335\0\0\0\20\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0`l\\\20iw_\356y\211h\373x\207g\364~\216i\364u\204e\366gt"
  "_\374^jX\241A;-_\0\0\0~\0\0\0\0SM4)SM21B9&\22\320\270\204\1\320\270\204"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0eq"
  "]\212r\200c\366v\205f\371jx_\323_kY\232_kZH^jY\26]iW\211@G9\272:6%j\220"
  "\211]\320\221\211`\377\212\203Z\377~xP\377mkE\331]^;|/0\37\21\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0ly`\40p~b\360lz`\353^kY\246["
  "eT<\216\200Z\203\227\211_\354\234\217c\377\232\217b\362\232\220c\337"
  "\243\233k\377\252\241p\377\250\236p\377\241\225h\377\231\214_\377\210"
  "\202U\377srI\377[]:\355KO0U\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0i"
  "v^\200`lY\211^jY\"\0\0\0\0\221\204\\\273\250\233r\377\302\267\224\377"
  "\311\300\237\377\272\256\204\377\271\256\177\377\271\257\200\377\267"
  "\260\177\377\260\251x\377\250\236l\377\242\225e\377\226\213]\377~zP\377"
  "ff@\377QT5\377LR2d\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0]iW(\0\0\0\0\0\0\0"
  "\0\213\203[v\253\240t\377\334\326\301\377\344\340\317\377\321\312\253"
  "\377\303\271\217\377\300\270\213\377\277\267\210\377\272\264\203\377"
  "\261\255z\377\250\242n\377\243\232h\377\232\220`\377\210\202V\377nnE"
  "\377SW6\377RX6\364Za<\34\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0b]@\20"
  "\234\222e\362\304\274\232\377\337\333\306\377\332\325\273\377\311\302"
  "\232\377\312\303\236\377\301\273\216\377\300\271\212\377\270\264\200"
  "\377\256\253v\377\246\243n\377\236\232h\377\230\220`\377\213\203V\377"
  "wvL\377X]:\377KR0\377NU5v\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\212"
  "\203Zl\242\234l\377\321\315\260\377\331\324\271\377\320\313\251\377\307"
  "\301\232\377\303\276\224\377\300\272\214\377\274\267\206\377\264\260"
  "|\377\253\251s\377\244\243n\377\232\230e\377\223\216^\377\207\200U\377"
  "ttJ\377[_<\377HO/\377GN0\200\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\210\204Y\240\245\237o\377\316\310\253\377\310\303\237\377\304\300\230"
  "\377\303\277\225\377\277\272\216\377\274\270\210\377\266\263\200\377"
  "\256\254v\377\247\246p\377\237\236j\377\227\226d\377\215\212[\377\203"
  "\177T\377qsH\377X]8\377FN.\377DK-\200\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\207\204X\257\244\240o\377\300\275\231\377\301\275\226\377\274"
  "\270\213\377\274\270\214\377\267\264\205\377\264\262\200\377\260\256"
  "z\377\251\251s\377\243\244n\377\231\232g\377\220\222`\377\210\211Y\377"
  "|}Q\377hlC\377PU3\377CK,\377DL/Y\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\205\204X\220\232\230h\377\261\260\204\377\266\264\212\377\261\260"
  "\201\377\263\260\200\377\260\257}\377\256\256x\377\253\254t\377\244\246"
  "o\377\233\236i\377\221\224b\377\211\214\\\377\202\204V\377txM\377]b>"
  "\377HP0\377@H+\373CJ-\25\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0xxO>"
  "\215\215_\377\237\237r\377\247\247x\377\247\247t\377\252\252w\377\252"
  "\252u\377\252\253t\377\243\246o\377\235\240j\377\223\230c\377\213\217"
  "]\377\201\206V\377x}P\377gkD\377RY5\377BI,\377AI,\262\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\202\205W\312\216\220`\377\230"
  "\232g\377\234\236i\377\236\241l\377\241\244n\377\240\244m\377\232\237"
  "i\377\223\230c\377\212\221]\377\200\210W\377v|P\377jnG\377Za>\377HP2"
  "\377=D)\377HQ1:\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0wzQ6\177\201U\371\206\211Z\377\216\222`\377\220\225a\377\220\225b\377"
  "\220\226a\377\213\221_\377\204\213Z\377{\203R\377ryN\377iqH\377^fA\377"
  "R[;\377BJ-\3778@'\317\0\0\0>\0\0\0\36\0\0\0\7\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0ptJTw|Q\371z\177R\377}\202T\377|\203T\377z\200"
  "R\377v|O\377pwL\377jpF\377dlB\377`hB\377Yb@\377LT6\377<C*\377\11\12\6"
  "\376\0\0\0\347\0\0\0\262\0\0\0Y\0\0\0\32\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\\`=UgnE\370hnG\377gmE\377djB\377]d>\377[c<\377Y"
  "b<\377Zc>\377V_>\377OW8\377BK/\377\16\20\12\377\0\0\0\377\0\0\0\377\0"
  "\0\0\374\0\0\0\320\0\0\0I\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\1\0\0\0\40\22\24\15\260@D+\377W`;\377OV5\377.3\36\377.3\37\377IP0"
  "\377RZ7\377PZ8\3776=&\377\14\15\10\377\0\0\0\377\0\0\0\377\0\0\0\377"
  "\0\0\0\347\0\0\0\217\0\0\0""4\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\20\0\0\0P\0\0\0\252\7\10\5\346\7\7\5\375\0\0\0\377\0\0"
  "\0\377\0\0\0\377\0\0\0\377\0\0\0\377\0\0\0\377\0\0\0\377\0\0\0\374\0"
  "\0\0\336\0\0\0\254\0\0\0i\0\0\0""2\0\0\0\10\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\11\0\0\0\40\0\0\0D\0\0\0m\0\0\0"
  "\226\0\0\0\234\0\0\0\234\0\0\0\244\0\0\0\246\0\0\0\232\0\0\0\202\0\0"
  "\0i\0\0\0T\0\0\0,\0\0\0\15\0\0\0\2\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\2\0\0\0\6\0\0\0"
  "\16\0\0\0\22\0\0\0\24\0\0\0\23\0\0\0\17\0\0\0\14\0\0\0\13\0\0\0\10\0"
  "\0\0\5\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"};


static void
dump_accels (gpointer             callback_data,
	     guint                callback_action,
	     BtkWidget           *widget)
{
  btk_accel_map_save_fd (1 /* stdout */);
}
    
static BtkItemFactoryEntry menu_items[] =
{
  { "/_File",                  NULL,         NULL,                  0, "<Branch>" },
  { "/File/tearoff1",          NULL,         btk_ifactory_cb,       0, "<Tearoff>" },
  { "/File/_New",              NULL,         btk_ifactory_cb,       0, "<StockItem>", BTK_STOCK_NEW },
  { "/File/_Open",             NULL,         btk_ifactory_cb,       0, "<StockItem>", BTK_STOCK_OPEN },
  { "/File/_Save",             NULL,         btk_ifactory_cb,       0, "<StockItem>", BTK_STOCK_SAVE },
  { "/File/Save _As...",       "<control>A", btk_ifactory_cb,       0, "<StockItem>", BTK_STOCK_SAVE },
  { "/File/_Dump \"_Accels\"",  NULL,        dump_accels,           0 },
  { "/File/\\/Test__Escaping/And\\/\n\tWei\\\\rdly",
                                NULL,        btk_ifactory_cb,       0 },
  { "/File/sep1",        NULL,               btk_ifactory_cb,       0, "<Separator>" },
  { "/File/_Quit",       NULL,               btk_ifactory_cb,       0, "<StockItem>", BTK_STOCK_QUIT },

  { "/_Preferences",     		NULL, 0,               0, "<Branch>" },
  { "/_Preferences/_Color", 		NULL, 0,               0, "<Branch>" },
  { "/_Preferences/Color/_Red",      	NULL, btk_ifactory_cb, 0, "<RadioItem>" },
  { "/_Preferences/Color/_Green",   	NULL, btk_ifactory_cb, 0, "/Preferences/Color/Red" },
  { "/_Preferences/Color/_Blue",        NULL, btk_ifactory_cb, 0, "/Preferences/Color/Red" },
  { "/_Preferences/_Shape", 		NULL, 0,               0, "<Branch>" },
  { "/_Preferences/Shape/_Square",      NULL, btk_ifactory_cb, 0, "<RadioItem>" },
  { "/_Preferences/Shape/_Rectangle",   NULL, btk_ifactory_cb, 0, "/Preferences/Shape/Square" },
  { "/_Preferences/Shape/_Oval",        NULL, btk_ifactory_cb, 0, "/Preferences/Shape/Rectangle" },
  { "/_Preferences/Shape/_Rectangle",   NULL, btk_ifactory_cb, 0, "/Preferences/Shape/Square" },
  { "/_Preferences/Shape/_Oval",        NULL, btk_ifactory_cb, 0, "/Preferences/Shape/Rectangle" },
  { "/_Preferences/Shape/_Image",       NULL, btk_ifactory_cb, 0, "<ImageItem>", apple },
  { "/_Preferences/Coffee",                  NULL, btk_ifactory_cb, 0, "<CheckItem>" },
  { "/_Preferences/Toast",                   NULL, btk_ifactory_cb, 0, "<CheckItem>" },
  { "/_Preferences/Marshmallow Froot Loops", NULL, btk_ifactory_cb, 0, "<CheckItem>" },

  /* For testing deletion of menus */
  { "/_Preferences/Should_NotAppear",          NULL, 0,               0, "<Branch>" },
  { "/Preferences/ShouldNotAppear/SubItem1",   NULL, btk_ifactory_cb, 0 },
  { "/Preferences/ShouldNotAppear/SubItem2",   NULL, btk_ifactory_cb, 0 },

  { "/_Help",            NULL,         0,                     0, "<LastBranch>" },
  { "/Help/_Help",       NULL,         btk_ifactory_cb,       0, "<StockItem>", BTK_STOCK_HELP},
  { "/Help/_About",      NULL,         btk_ifactory_cb,       0 },
};


static int nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);

static void
create_item_factory (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  
  if (!window)
    {
      BtkWidget *box1;
      BtkWidget *box2;
      BtkWidget *separator;
      BtkWidget *label;
      BtkWidget *button;
      BtkAccelGroup *accel_group;
      BtkItemFactory *item_factory;
      BtkTooltips *tooltips;
      
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      
      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));
      
      g_signal_connect (window, "destroy",
			G_CALLBACK(btk_widget_destroyed),
			&window);
      g_signal_connect (window, "delete-event",
			G_CALLBACK (btk_true),
			NULL);
      
      accel_group = btk_accel_group_new ();
      item_factory = btk_item_factory_new (BTK_TYPE_MENU_BAR, "<main>", accel_group);
      g_object_set_data_full (B_OBJECT (window),
			      "<main>",
			      item_factory,
			      g_object_unref);
      btk_window_add_accel_group (BTK_WINDOW (window), accel_group);
      btk_window_set_title (BTK_WINDOW (window), "Item Factory");
      btk_container_set_border_width (BTK_CONTAINER (window), 0);
      btk_item_factory_create_items (item_factory, nmenu_items, menu_items, NULL);

      /* preselect /Preferences/Shape/Oval over the other radios
       */
      btk_check_menu_item_set_active (BTK_CHECK_MENU_ITEM (btk_item_factory_get_item (item_factory,
										      "/Preferences/Shape/Oval")),
				      TRUE);

      /* preselect /Preferences/Coffee
       */
      btk_check_menu_item_set_active (BTK_CHECK_MENU_ITEM (btk_item_factory_get_item (item_factory,
										      "/Preferences/Coffee")),
				      TRUE);

      /* preselect /Preferences/Marshmallow Froot Loops and set it insensitive
       */
      btk_check_menu_item_set_active (BTK_CHECK_MENU_ITEM (btk_item_factory_get_item (item_factory,
										      "/Preferences/Marshmallow Froot Loops")),
				      TRUE);
      btk_widget_set_sensitive (BTK_WIDGET (btk_item_factory_get_item (item_factory,
								       "/Preferences/Marshmallow Froot Loops")),
				FALSE);
       
      /* Test how tooltips (ugh) work on menu items
       */
      tooltips = btk_tooltips_new ();
      g_object_ref (tooltips);
      btk_object_sink (BTK_OBJECT (tooltips));
      g_object_set_data_full (B_OBJECT (window), "testbtk-tooltips",
			      tooltips, (GDestroyNotify)g_object_unref);
      
      btk_tooltips_set_tip (tooltips, btk_item_factory_get_item (item_factory, "/File/New"),
			    "Create a new file", NULL);
      btk_tooltips_set_tip (tooltips, btk_item_factory_get_item (item_factory, "/File/Open"),
			    "Open a file", NULL);
      btk_tooltips_set_tip (tooltips, btk_item_factory_get_item (item_factory, "/File/Save"),
			    "Safe file", NULL);
      btk_tooltips_set_tip (tooltips, btk_item_factory_get_item (item_factory, "/Preferences/Color"),
			    "Modify color", NULL);

      box1 = btk_vbox_new (FALSE, 0);
      btk_container_add (BTK_CONTAINER (window), box1);
      
      btk_box_pack_start (BTK_BOX (box1),
			  btk_item_factory_get_widget (item_factory, "<main>"),
			  FALSE, FALSE, 0);

      label = btk_label_new ("Type\n<alt>\nto start");
      btk_widget_set_size_request (label, 200, 200);
      btk_misc_set_alignment (BTK_MISC (label), 0.5, 0.5);
      btk_box_pack_start (BTK_BOX (box1), label, TRUE, TRUE, 0);


      separator = btk_hseparator_new ();
      btk_box_pack_start (BTK_BOX (box1), separator, FALSE, TRUE, 0);


      box2 = btk_vbox_new (FALSE, 10);
      btk_container_set_border_width (BTK_CONTAINER (box2), 10);
      btk_box_pack_start (BTK_BOX (box1), box2, FALSE, TRUE, 0);

      button = btk_button_new_with_label ("close");
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (btk_widget_destroy),
				window);
      btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);
      btk_widget_set_can_default (button, TRUE);
      btk_widget_grab_default (button);

      btk_item_factory_delete_item (item_factory, "/Preferences/ShouldNotAppear");
      
      btk_widget_show_all (window);
    }
  else
    btk_widget_destroy (window);
}

static BtkWidget *
accel_button_new (BtkAccelGroup *accel_group,
		  const gchar   *text,
		  const gchar   *accel)
{
  guint keyval;
  BdkModifierType modifiers;
  BtkWidget *button;
  BtkWidget *label;

  btk_accelerator_parse (accel, &keyval, &modifiers);
  g_assert (keyval);

  button = btk_button_new ();
  btk_widget_add_accelerator (button, "activate", accel_group,
			      keyval, modifiers, BTK_ACCEL_VISIBLE | BTK_ACCEL_LOCKED);

  label = btk_accel_label_new (text);
  btk_accel_label_set_accel_widget (BTK_ACCEL_LABEL (label), button);
  btk_widget_show (label);
  
  btk_container_add (BTK_CONTAINER (button), label);

  return button;
}

static void
create_key_lookup (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  gpointer window_ptr;

  if (!window)
    {
      BtkAccelGroup *accel_group = btk_accel_group_new ();
      BtkWidget *button;
      
      window = btk_dialog_new_with_buttons ("Key Lookup", NULL, 0,
					    BTK_STOCK_CLOSE, BTK_RESPONSE_CLOSE,
					    NULL);

      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));

      /* We have to expand it so the accel labels will draw their labels
       */
      btk_window_set_default_size (BTK_WINDOW (window), 300, -1);
      
      btk_window_add_accel_group (BTK_WINDOW (window), accel_group);
      
      button = btk_button_new_with_mnemonic ("Button 1 (_a)");
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->vbox), button, FALSE, FALSE, 0);
      button = btk_button_new_with_mnemonic ("Button 2 (_A)");
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->vbox), button, FALSE, FALSE, 0);
      button = btk_button_new_with_mnemonic ("Button 3 (_\321\204)");
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->vbox), button, FALSE, FALSE, 0);
      button = btk_button_new_with_mnemonic ("Button 4 (_\320\244)");
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->vbox), button, FALSE, FALSE, 0);
      button = btk_button_new_with_mnemonic ("Button 6 (_b)");
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->vbox), button, FALSE, FALSE, 0);
      button = accel_button_new (accel_group, "Button 7", "<Alt><Shift>b");
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->vbox), button, FALSE, FALSE, 0);
      button = accel_button_new (accel_group, "Button 8", "<Alt>d");
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->vbox), button, FALSE, FALSE, 0);
      button = accel_button_new (accel_group, "Button 9", "<Alt>Cyrillic_ve");
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->vbox), button, FALSE, FALSE, 0);
      button = btk_button_new_with_mnemonic ("Button 10 (_1)");
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->vbox), button, FALSE, FALSE, 0);
      button = btk_button_new_with_mnemonic ("Button 11 (_!)");
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->vbox), button, FALSE, FALSE, 0);
      button = accel_button_new (accel_group, "Button 12", "<Super>a");
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->vbox), button, FALSE, FALSE, 0);
      button = accel_button_new (accel_group, "Button 13", "<Hyper>a");
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->vbox), button, FALSE, FALSE, 0);
      button = accel_button_new (accel_group, "Button 14", "<Meta>a");
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->vbox), button, FALSE, FALSE, 0);
      button = accel_button_new (accel_group, "Button 15", "<Shift><Mod4>b");
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->vbox), button, FALSE, FALSE, 0);

      window_ptr = &window;
      g_object_add_weak_pointer (B_OBJECT (window), window_ptr);
      g_signal_connect (window, "response", G_CALLBACK (btk_object_destroy), NULL);

      btk_widget_show_all (window);
    }
  else
    btk_widget_destroy (window);
}


/*
 create_modal_window
 */

static gboolean
cmw_destroy_cb(BtkWidget *widget)
{
  /* This is needed to get out of btk_main */
  btk_main_quit ();

  return FALSE;
}

static void
cmw_color (BtkWidget *widget, BtkWidget *parent)
{
    BtkWidget *csd;

    csd = btk_color_selection_dialog_new ("This is a modal color selection dialog");

    btk_window_set_screen (BTK_WINDOW (csd), btk_widget_get_screen (parent));

    btk_color_selection_set_has_palette (BTK_COLOR_SELECTION (BTK_COLOR_SELECTION_DIALOG (csd)->colorsel),
                                         TRUE);
    
    /* Set as modal */
    btk_window_set_modal (BTK_WINDOW(csd),TRUE);

    /* And mark it as a transient dialog */
    btk_window_set_transient_for (BTK_WINDOW (csd), BTK_WINDOW (parent));
    
    g_signal_connect (csd, "destroy",
		      G_CALLBACK (cmw_destroy_cb), NULL);

    g_signal_connect_swapped (BTK_COLOR_SELECTION_DIALOG (csd)->ok_button,
			     "clicked", G_CALLBACK (btk_widget_destroy), csd);
    g_signal_connect_swapped (BTK_COLOR_SELECTION_DIALOG (csd)->cancel_button,
			     "clicked", G_CALLBACK (btk_widget_destroy), csd);
    
    /* wait until destroy calls btk_main_quit */
    btk_widget_show (csd);    
    btk_main ();
}

static void
cmw_file (BtkWidget *widget, BtkWidget *parent)
{
    BtkWidget *fs;

    fs = btk_file_selection_new("This is a modal file selection dialog");

    btk_window_set_screen (BTK_WINDOW (fs), btk_widget_get_screen (parent));

    /* Set as modal */
    btk_window_set_modal (BTK_WINDOW(fs),TRUE);

    /* And mark it as a transient dialog */
    btk_window_set_transient_for (BTK_WINDOW (fs), BTK_WINDOW (parent));

    g_signal_connect (fs, "destroy",
                      G_CALLBACK (cmw_destroy_cb), NULL);

    g_signal_connect_swapped (BTK_FILE_SELECTION (fs)->ok_button,
			      "clicked", G_CALLBACK (btk_widget_destroy), fs);
    g_signal_connect_swapped (BTK_FILE_SELECTION (fs)->cancel_button,
			      "clicked", G_CALLBACK (btk_widget_destroy), fs);
    
    /* wait until destroy calls btk_main_quit */
    btk_widget_show (fs);
    
    btk_main();
}


static void
create_modal_window (BtkWidget *widget)
{
  BtkWidget *window = NULL;
  BtkWidget *box1,*box2;
  BtkWidget *frame1;
  BtkWidget *btnColor,*btnFile,*btnClose;

  /* Create modal window (Here you can use any window descendent )*/
  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  btk_window_set_screen (BTK_WINDOW (window),
			 btk_widget_get_screen (widget));

  btk_window_set_title (BTK_WINDOW(window),"This window is modal");

  /* Set window as modal */
  btk_window_set_modal (BTK_WINDOW(window),TRUE);

  /* Create widgets */
  box1 = btk_vbox_new (FALSE,5);
  frame1 = btk_frame_new ("Standard dialogs in modal form");
  box2 = btk_vbox_new (TRUE,5);
  btnColor = btk_button_new_with_label ("Color");
  btnFile = btk_button_new_with_label ("File Selection");
  btnClose = btk_button_new_with_label ("Close");

  /* Init widgets */
  btk_container_set_border_width (BTK_CONTAINER (box1), 3);
  btk_container_set_border_width (BTK_CONTAINER (box2), 3);
    
  /* Pack widgets */
  btk_container_add (BTK_CONTAINER (window), box1);
  btk_box_pack_start (BTK_BOX (box1), frame1, TRUE, TRUE, 4);
  btk_container_add (BTK_CONTAINER (frame1), box2);
  btk_box_pack_start (BTK_BOX (box2), btnColor, FALSE, FALSE, 4);
  btk_box_pack_start (BTK_BOX (box2), btnFile, FALSE, FALSE, 4);
  btk_box_pack_start (BTK_BOX (box1), btk_hseparator_new (), FALSE, FALSE, 4);
  btk_box_pack_start (BTK_BOX (box1), btnClose, FALSE, FALSE, 4);
   
  /* connect signals */
  g_signal_connect_swapped (btnClose, "clicked",
			    G_CALLBACK (btk_widget_destroy), window);

  g_signal_connect (window, "destroy",
                    G_CALLBACK (cmw_destroy_cb), NULL);
  
  g_signal_connect (btnColor, "clicked",
                    G_CALLBACK (cmw_color), window);
  g_signal_connect (btnFile, "clicked",
                    G_CALLBACK (cmw_file), window);

  /* Show widgets */
  btk_widget_show_all (window);

  /* wait until dialog get destroyed */
  btk_main();
}

/*
 * BtkMessageDialog
 */

static void
make_message_dialog (BdkScreen *screen,
		     BtkWidget **dialog,
                     BtkMessageType  type,
                     BtkButtonsType  buttons,
		     guint           default_response)
{
  if (*dialog)
    {
      btk_widget_destroy (*dialog);

      return;
    }

  *dialog = btk_message_dialog_new (NULL, 0, type, buttons,
                                    "This is a message dialog; it can wrap long lines. This is a long line. La la la. Look this line is wrapped. Blah blah blah blah blah blah. (Note: testbtk has a nonstandard btkrc that changes some of the message dialog icons.)");

  btk_window_set_screen (BTK_WINDOW (*dialog), screen);

  g_signal_connect_swapped (*dialog,
			    "response",
			    G_CALLBACK (btk_widget_destroy),
			    *dialog);
  
  g_signal_connect (*dialog,
                    "destroy",
                    G_CALLBACK (btk_widget_destroyed),
                    dialog);

  btk_dialog_set_default_response (BTK_DIALOG (*dialog), default_response);

  btk_widget_show (*dialog);
}

static void
create_message_dialog (BtkWidget *widget)
{
  static BtkWidget *info = NULL;
  static BtkWidget *warning = NULL;
  static BtkWidget *error = NULL;
  static BtkWidget *question = NULL;
  BdkScreen *screen = btk_widget_get_screen (widget);

  make_message_dialog (screen, &info, BTK_MESSAGE_INFO, BTK_BUTTONS_OK, BTK_RESPONSE_OK);
  make_message_dialog (screen, &warning, BTK_MESSAGE_WARNING, BTK_BUTTONS_CLOSE, BTK_RESPONSE_CLOSE);
  make_message_dialog (screen, &error, BTK_MESSAGE_ERROR, BTK_BUTTONS_OK_CANCEL, BTK_RESPONSE_OK);
  make_message_dialog (screen, &question, BTK_MESSAGE_QUESTION, BTK_BUTTONS_YES_NO, BTK_RESPONSE_NO);
}

/*
 * BtkScrolledWindow
 */

static BtkWidget *sw_parent = NULL;
static BtkWidget *sw_float_parent;
static guint sw_destroyed_handler = 0;

static gboolean
scrolled_windows_delete_cb (BtkWidget *widget, BdkEventAny *event, BtkWidget *scrollwin)
{
  btk_widget_reparent (scrollwin, sw_parent);
  
  g_signal_handler_disconnect (sw_parent, sw_destroyed_handler);
  sw_float_parent = NULL;
  sw_parent = NULL;
  sw_destroyed_handler = 0;

  return FALSE;
}

static void
scrolled_windows_destroy_cb (BtkWidget *widget, BtkWidget *scrollwin)
{
  btk_widget_destroy (sw_float_parent);

  sw_float_parent = NULL;
  sw_parent = NULL;
  sw_destroyed_handler = 0;
}

static void
scrolled_windows_remove (BtkWidget *widget, BtkWidget *scrollwin)
{
  if (sw_parent)
    {
      btk_widget_reparent (scrollwin, sw_parent);
      btk_widget_destroy (sw_float_parent);

      g_signal_handler_disconnect (sw_parent, sw_destroyed_handler);
      sw_float_parent = NULL;
      sw_parent = NULL;
      sw_destroyed_handler = 0;
    }
  else
    {
      sw_parent = scrollwin->parent;
      sw_float_parent = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_screen (BTK_WINDOW (sw_float_parent),
			     btk_widget_get_screen (widget));
      
      btk_window_set_default_size (BTK_WINDOW (sw_float_parent), 200, 200);
      
      btk_widget_reparent (scrollwin, sw_float_parent);
      btk_widget_show (sw_float_parent);

      sw_destroyed_handler =
	g_signal_connect (sw_parent, "destroy",
			  G_CALLBACK (scrolled_windows_destroy_cb), scrollwin);
      g_signal_connect (sw_float_parent, "delete_event",
			G_CALLBACK (scrolled_windows_delete_cb), scrollwin);
    }
}

static void
create_scrolled_windows (BtkWidget *widget)
{
  static BtkWidget *window;
  BtkWidget *scrolled_window;
  BtkWidget *table;
  BtkWidget *button;
  char buffer[32];
  int i, j;

  if (!window)
    {
      window = btk_dialog_new ();

      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window);

      btk_window_set_title (BTK_WINDOW (window), "dialog");
      btk_container_set_border_width (BTK_CONTAINER (window), 0);


      scrolled_window = btk_scrolled_window_new (NULL, NULL);
      btk_container_set_border_width (BTK_CONTAINER (scrolled_window), 10);
      btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (scrolled_window),
				      BTK_POLICY_AUTOMATIC,
				      BTK_POLICY_AUTOMATIC);
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->vbox), 
			  scrolled_window, TRUE, TRUE, 0);
      btk_widget_show (scrolled_window);

      table = btk_table_new (20, 20, FALSE);
      btk_table_set_row_spacings (BTK_TABLE (table), 10);
      btk_table_set_col_spacings (BTK_TABLE (table), 10);
      btk_scrolled_window_add_with_viewport (BTK_SCROLLED_WINDOW (scrolled_window), table);
      btk_container_set_focus_hadjustment (BTK_CONTAINER (table),
					   btk_scrolled_window_get_hadjustment (BTK_SCROLLED_WINDOW (scrolled_window)));
      btk_container_set_focus_vadjustment (BTK_CONTAINER (table),
					   btk_scrolled_window_get_vadjustment (BTK_SCROLLED_WINDOW (scrolled_window)));
      btk_widget_show (table);

      for (i = 0; i < 20; i++)
	for (j = 0; j < 20; j++)
	  {
	    sprintf (buffer, "button (%d,%d)\n", i, j);
	    button = btk_toggle_button_new_with_label (buffer);
	    btk_table_attach_defaults (BTK_TABLE (table), button,
				       i, i+1, j, j+1);
	    btk_widget_show (button);
	  }


      button = btk_button_new_with_label ("Close");
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (btk_widget_destroy),
				window);
      btk_widget_set_can_default (button, TRUE);
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->action_area), 
			  button, TRUE, TRUE, 0);
      btk_widget_grab_default (button);
      btk_widget_show (button);

      button = btk_button_new_with_label ("Reparent Out");
      g_signal_connect (button, "clicked",
			G_CALLBACK (scrolled_windows_remove),
			scrolled_window);
      btk_widget_set_can_default (button, TRUE);
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->action_area), 
			  button, TRUE, TRUE, 0);
      btk_widget_grab_default (button);
      btk_widget_show (button);

      btk_window_set_default_size (BTK_WINDOW (window), 300, 300);
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show (window);
  else
    btk_widget_destroy (window);
}

/*
 * BtkEntry
 */

static void
entry_toggle_frame (BtkWidget *checkbutton,
                    BtkWidget *entry)
{
   btk_entry_set_has_frame (BTK_ENTRY(entry),
                            BTK_TOGGLE_BUTTON(checkbutton)->active);
}

static void
entry_toggle_sensitive (BtkWidget *checkbutton,
			BtkWidget *entry)
{
   btk_widget_set_sensitive (entry, BTK_TOGGLE_BUTTON(checkbutton)->active);
}

static gboolean
entry_progress_timeout (gpointer data)
{
  if (GPOINTER_TO_INT (g_object_get_data (B_OBJECT (data), "progress-pulse")))
    {
      btk_entry_progress_pulse (BTK_ENTRY (data));
    }
  else
    {
      gdouble fraction;

      fraction = btk_entry_get_progress_fraction (BTK_ENTRY (data));

      fraction += 0.05;
      if (fraction > 1.0001)
        fraction = 0.0;

      btk_entry_set_progress_fraction (BTK_ENTRY (data), fraction);
    }

  return TRUE;
}

static void
entry_remove_timeout (gpointer data)
{
  g_source_remove (GPOINTER_TO_UINT (data));
}

static void
entry_toggle_progress (BtkWidget *checkbutton,
                       BtkWidget *entry)
{
  if (BTK_TOGGLE_BUTTON (checkbutton)->active)
    {
      guint timeout = bdk_threads_add_timeout (100,
                                               entry_progress_timeout,
                                               entry);
      g_object_set_data_full (B_OBJECT (entry), "timeout-id",
                              GUINT_TO_POINTER (timeout),
                              entry_remove_timeout);
    }
  else
    {
      g_object_set_data (B_OBJECT (entry), "timeout-id",
                         GUINT_TO_POINTER (0));

      btk_entry_set_progress_fraction (BTK_ENTRY (entry), 0.0);
    }
}

static void
entry_toggle_pulse (BtkWidget *checkbutton,
                    BtkWidget *entry)
{
  g_object_set_data (B_OBJECT (entry), "progress-pulse",
                     GUINT_TO_POINTER ((guint) BTK_TOGGLE_BUTTON (checkbutton)->active));
}

static void
props_clicked (BtkWidget *button,
               BObject   *object)
{
  BtkWidget *window = create_prop_editor (object, 0);

  btk_window_set_title (BTK_WINDOW (window), "Object Properties");
}

static void
create_entry (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  BtkWidget *box1;
  BtkWidget *box2;
  BtkWidget *hbox;
  BtkWidget *has_frame_check;
  BtkWidget *sensitive_check;
  BtkWidget *progress_check;
  BtkWidget *entry, *cb;
  BtkWidget *button;
  BtkWidget *separator;
  GList *cbitems = NULL;

  if (!window)
    {
      cbitems = g_list_append(cbitems, "item0");
      cbitems = g_list_append(cbitems, "item1 item1");
      cbitems = g_list_append(cbitems, "item2 item2 item2");
      cbitems = g_list_append(cbitems, "item3 item3 item3 item3");
      cbitems = g_list_append(cbitems, "item4 item4 item4 item4 item4");
      cbitems = g_list_append(cbitems, "item5 item5 item5 item5 item5 item5");
      cbitems = g_list_append(cbitems, "item6 item6 item6 item6 item6");
      cbitems = g_list_append(cbitems, "item7 item7 item7 item7");
      cbitems = g_list_append(cbitems, "item8 item8 item8");
      cbitems = g_list_append(cbitems, "item9 item9");

      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window);

      btk_window_set_title (BTK_WINDOW (window), "entry");
      btk_container_set_border_width (BTK_CONTAINER (window), 0);


      box1 = btk_vbox_new (FALSE, 0);
      btk_container_add (BTK_CONTAINER (window), box1);


      box2 = btk_vbox_new (FALSE, 10);
      btk_container_set_border_width (BTK_CONTAINER (box2), 10);
      btk_box_pack_start (BTK_BOX (box1), box2, TRUE, TRUE, 0);

      hbox = btk_hbox_new (FALSE, 5);
      btk_box_pack_start (BTK_BOX (box2), hbox, TRUE, TRUE, 0);
      
      entry = btk_entry_new ();
      btk_entry_set_text (BTK_ENTRY (entry), "hello world \330\247\331\204\330\263\331\204\330\247\331\205 \330\271\331\204\331\212\331\203\331\205");
      btk_editable_select_rebunnyion (BTK_EDITABLE (entry), 0, 5);
      btk_box_pack_start (BTK_BOX (hbox), entry, TRUE, TRUE, 0);

      button = btk_button_new_with_mnemonic ("_Props");
      btk_box_pack_start (BTK_BOX (hbox), button, FALSE, FALSE, 0);
      g_signal_connect (button, "clicked",
			G_CALLBACK (props_clicked),
			entry);

      cb = btk_combo_new ();
      btk_combo_set_popdown_strings (BTK_COMBO (cb), cbitems);
      btk_entry_set_text (BTK_ENTRY (BTK_COMBO(cb)->entry), "hello world \n\n\n foo");
      btk_editable_select_rebunnyion (BTK_EDITABLE (BTK_COMBO(cb)->entry),
				  0, -1);
      btk_box_pack_start (BTK_BOX (box2), cb, TRUE, TRUE, 0);

      sensitive_check = btk_check_button_new_with_label("Sensitive");
      btk_box_pack_start (BTK_BOX (box2), sensitive_check, FALSE, TRUE, 0);
      g_signal_connect (sensitive_check, "toggled",
			G_CALLBACK (entry_toggle_sensitive), entry);
      btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (sensitive_check), TRUE);

      has_frame_check = btk_check_button_new_with_label("Has Frame");
      btk_box_pack_start (BTK_BOX (box2), has_frame_check, FALSE, TRUE, 0);
      g_signal_connect (has_frame_check, "toggled",
			G_CALLBACK (entry_toggle_frame), entry);
      btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (has_frame_check), TRUE);

      progress_check = btk_check_button_new_with_label("Show Progress");
      btk_box_pack_start (BTK_BOX (box2), progress_check, FALSE, TRUE, 0);
      g_signal_connect (progress_check, "toggled",
			G_CALLBACK (entry_toggle_progress), entry);

      progress_check = btk_check_button_new_with_label("Pulse Progress");
      btk_box_pack_start (BTK_BOX (box2), progress_check, FALSE, TRUE, 0);
      g_signal_connect (progress_check, "toggled",
			G_CALLBACK (entry_toggle_pulse), entry);

      separator = btk_hseparator_new ();
      btk_box_pack_start (BTK_BOX (box1), separator, FALSE, TRUE, 0);

      box2 = btk_vbox_new (FALSE, 10);
      btk_container_set_border_width (BTK_CONTAINER (box2), 10);
      btk_box_pack_start (BTK_BOX (box1), box2, FALSE, TRUE, 0);

      button = btk_button_new_with_label ("close");
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (btk_widget_destroy),
				window);
      btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);
      btk_widget_set_can_default (button, TRUE);
      btk_widget_grab_default (button);
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    btk_widget_destroy (window);
}

static void
create_expander (BtkWidget *widget)
{
  BtkWidget *box1;
  BtkWidget *expander;
  BtkWidget *hidden;
  static BtkWidget *window = NULL;

  if (!window)
    {
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));
      
      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window);
      
      btk_window_set_title (BTK_WINDOW (window), "expander");
      btk_container_set_border_width (BTK_CONTAINER (window), 0);
      
      box1 = btk_vbox_new (FALSE, 0);
      btk_container_add (BTK_CONTAINER (window), box1);
      
      expander = btk_expander_new ("The Hidden");
      
      btk_box_pack_start (BTK_BOX (box1), expander, TRUE, TRUE, 0);
      
      hidden = btk_label_new ("Revealed!");
      
      btk_container_add (BTK_CONTAINER (expander), hidden);
    }
  
  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    btk_widget_destroy (window);
}


/* BtkEventBox */


static void
event_box_label_pressed (BtkWidget        *widget,
			 BdkEventButton   *event,
			 gpointer user_data)
{
  g_print ("clicked on event box\n");
}

static void
event_box_button_clicked (BtkWidget *widget,
			  BtkWidget *button,
			  gpointer user_data)
{
  g_print ("pushed button\n");
}

static void
event_box_toggle_visible_window (BtkWidget *checkbutton,
				 BtkEventBox *event_box)
{
  btk_event_box_set_visible_window (event_box,
				    BTK_TOGGLE_BUTTON(checkbutton)->active);
}

static void
event_box_toggle_above_child (BtkWidget *checkbutton,
			      BtkEventBox *event_box)
{
  btk_event_box_set_above_child (event_box,
				 BTK_TOGGLE_BUTTON(checkbutton)->active);
}

static void
create_event_box (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  BtkWidget *box1;
  BtkWidget *box2;
  BtkWidget *hbox;
  BtkWidget *vbox;
  BtkWidget *button;
  BtkWidget *separator;
  BtkWidget *event_box;
  BtkWidget *label;
  BtkWidget *visible_window_check;
  BtkWidget *above_child_check;
  BdkColor color;

  if (!window)
    {
      color.red = 0;
      color.blue = 65535;
      color.green = 0;
      
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window);

      btk_window_set_title (BTK_WINDOW (window), "event box");
      btk_container_set_border_width (BTK_CONTAINER (window), 0);

      box1 = btk_vbox_new (FALSE, 0);
      btk_container_add (BTK_CONTAINER (window), box1);
      btk_widget_modify_bg (window, BTK_STATE_NORMAL, &color);

      hbox = btk_hbox_new (FALSE, 0);
      btk_box_pack_start (BTK_BOX (box1), hbox, TRUE, FALSE, 0);
      
      event_box = btk_event_box_new ();
      btk_box_pack_start (BTK_BOX (hbox), event_box, TRUE, FALSE, 0);

      vbox = btk_vbox_new (FALSE, 0);
      btk_container_add (BTK_CONTAINER (event_box), vbox);
      g_signal_connect (event_box, "button_press_event",
			G_CALLBACK (event_box_label_pressed),
			NULL);
      
      label = btk_label_new ("Click on this label");
      btk_box_pack_start (BTK_BOX (vbox), label, TRUE, FALSE, 0);

      button = btk_button_new_with_label ("button in eventbox");
      btk_box_pack_start (BTK_BOX (vbox), button, TRUE, FALSE, 0);
      g_signal_connect (button, "clicked",
			G_CALLBACK (event_box_button_clicked),
			NULL);
      

      visible_window_check = btk_check_button_new_with_label("Visible Window");
      btk_box_pack_start (BTK_BOX (box1), visible_window_check, FALSE, TRUE, 0);
      g_signal_connect (visible_window_check, "toggled",
			G_CALLBACK (event_box_toggle_visible_window), event_box);
      btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (visible_window_check), FALSE);
      
      above_child_check = btk_check_button_new_with_label("Above Child");
      btk_box_pack_start (BTK_BOX (box1), above_child_check, FALSE, TRUE, 0);
      g_signal_connect (above_child_check, "toggled",
			G_CALLBACK (event_box_toggle_above_child), event_box);
      btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (above_child_check), FALSE);
      
      separator = btk_hseparator_new ();
      btk_box_pack_start (BTK_BOX (box1), separator, FALSE, TRUE, 0);

      box2 = btk_vbox_new (FALSE, 10);
      btk_container_set_border_width (BTK_CONTAINER (box2), 10);
      btk_box_pack_start (BTK_BOX (box1), box2, FALSE, TRUE, 0);

      button = btk_button_new_with_label ("close");
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (btk_widget_destroy),
				window);
      btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);
      btk_widget_set_can_default (button, TRUE);
      btk_widget_grab_default (button);
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    btk_widget_destroy (window);
}


/*
 * BtkSizeGroup
 */

#define SIZE_GROUP_INITIAL_SIZE 50

static void
size_group_hsize_changed (BtkSpinButton *spin_button,
			  BtkWidget     *button)
{
  btk_widget_set_size_request (BTK_BIN (button)->child,
			       btk_spin_button_get_value_as_int (spin_button),
			       -1);
}

static void
size_group_vsize_changed (BtkSpinButton *spin_button,
			  BtkWidget     *button)
{
  btk_widget_set_size_request (BTK_BIN (button)->child,
			       -1,
			       btk_spin_button_get_value_as_int (spin_button));
}

static BtkWidget *
create_size_group_window (BdkScreen    *screen,
			  BtkSizeGroup *master_size_group)
{
  BtkWidget *window;
  BtkWidget *table;
  BtkWidget *main_button;
  BtkWidget *button;
  BtkWidget *spin_button;
  BtkWidget *hbox;
  BtkSizeGroup *hgroup1;
  BtkSizeGroup *hgroup2;
  BtkSizeGroup *vgroup1;
  BtkSizeGroup *vgroup2;

  window = btk_dialog_new_with_buttons ("BtkSizeGroup",
					NULL, 0,
					BTK_STOCK_CLOSE,
					BTK_RESPONSE_NONE,
					NULL);

  btk_window_set_screen (BTK_WINDOW (window), screen);

  btk_window_set_resizable (BTK_WINDOW (window), TRUE);

  g_signal_connect (window, "response",
		    G_CALLBACK (btk_widget_destroy),
		    NULL);

  table = btk_table_new (2, 2, FALSE);
  btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->vbox), table, TRUE, TRUE, 0);

  btk_table_set_row_spacings (BTK_TABLE (table), 5);
  btk_table_set_col_spacings (BTK_TABLE (table), 5);
  btk_container_set_border_width (BTK_CONTAINER (table), 5);
  btk_widget_set_size_request (table, 250, 250);

  hgroup1 = btk_size_group_new (BTK_SIZE_GROUP_HORIZONTAL);
  hgroup2 = btk_size_group_new (BTK_SIZE_GROUP_HORIZONTAL);
  vgroup1 = btk_size_group_new (BTK_SIZE_GROUP_VERTICAL);
  vgroup2 = btk_size_group_new (BTK_SIZE_GROUP_VERTICAL);

  main_button = btk_button_new_with_label ("X");
  
  btk_table_attach (BTK_TABLE (table), main_button,
		    0, 1,       0, 1,
		    BTK_EXPAND, BTK_EXPAND,
		    0,          0);
  btk_size_group_add_widget (master_size_group, main_button);
  btk_size_group_add_widget (hgroup1, main_button);
  btk_size_group_add_widget (vgroup1, main_button);
  btk_widget_set_size_request (BTK_BIN (main_button)->child,
			       SIZE_GROUP_INITIAL_SIZE,
			       SIZE_GROUP_INITIAL_SIZE);

  button = btk_button_new ();
  btk_table_attach (BTK_TABLE (table), button,
		    1, 2,       0, 1,
		    BTK_EXPAND, BTK_EXPAND,
		    0,          0);
  btk_size_group_add_widget (vgroup1, button);
  btk_size_group_add_widget (vgroup2, button);

  button = btk_button_new ();
  btk_table_attach (BTK_TABLE (table), button,
		    0, 1,       1, 2,
		    BTK_EXPAND, BTK_EXPAND,
		    0,          0);
  btk_size_group_add_widget (hgroup1, button);
  btk_size_group_add_widget (hgroup2, button);

  button = btk_button_new ();
  btk_table_attach (BTK_TABLE (table), button,
		    1, 2,       1, 2,
		    BTK_EXPAND, BTK_EXPAND,
		    0,          0);
  btk_size_group_add_widget (hgroup2, button);
  btk_size_group_add_widget (vgroup2, button);

  g_object_unref (hgroup1);
  g_object_unref (hgroup2);
  g_object_unref (vgroup1);
  g_object_unref (vgroup2);
  
  hbox = btk_hbox_new (FALSE, 5);
  btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->vbox), hbox, FALSE, FALSE, 0);
  
  spin_button = btk_spin_button_new_with_range (1, 100, 1);
  btk_spin_button_set_value (BTK_SPIN_BUTTON (spin_button), SIZE_GROUP_INITIAL_SIZE);
  btk_box_pack_start (BTK_BOX (hbox), spin_button, TRUE, TRUE, 0);
  g_signal_connect (spin_button, "value_changed",
		    G_CALLBACK (size_group_hsize_changed), main_button);

  spin_button = btk_spin_button_new_with_range (1, 100, 1);
  btk_spin_button_set_value (BTK_SPIN_BUTTON (spin_button), SIZE_GROUP_INITIAL_SIZE);
  btk_box_pack_start (BTK_BOX (hbox), spin_button, TRUE, TRUE, 0);
  g_signal_connect (spin_button, "value_changed",
		    G_CALLBACK (size_group_vsize_changed), main_button);

  return window;
}

static void
create_size_groups (BtkWidget *widget)
{
  static BtkWidget *window1 = NULL;
  static BtkWidget *window2 = NULL;
  static BtkSizeGroup *master_size_group;

  if (!master_size_group)
    master_size_group = btk_size_group_new (BTK_SIZE_GROUP_BOTH);

  if (!window1)
    {
      window1 = create_size_group_window (btk_widget_get_screen (widget),
					  master_size_group);

      g_signal_connect (window1, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window1);
    }

  if (!window2)
    {
      window2 = create_size_group_window (btk_widget_get_screen (widget),
					  master_size_group);

      g_signal_connect (window2, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window2);
    }

  if (btk_widget_get_visible (window1) && btk_widget_get_visible (window2))
    {
      btk_widget_destroy (window1);
      btk_widget_destroy (window2);
    }
  else
    {
      if (!btk_widget_get_visible (window1))
	btk_widget_show_all (window1);
      if (!btk_widget_get_visible (window2))
	btk_widget_show_all (window2);
    }
}

/*
 * BtkSpinButton
 */

static BtkWidget *spinner1;

static void
toggle_snap (BtkWidget *widget, BtkSpinButton *spin)
{
  btk_spin_button_set_snap_to_ticks (spin, BTK_TOGGLE_BUTTON (widget)->active);
}

static void
toggle_numeric (BtkWidget *widget, BtkSpinButton *spin)
{
  btk_spin_button_set_numeric (spin, BTK_TOGGLE_BUTTON (widget)->active);
}

static void
change_digits (BtkWidget *widget, BtkSpinButton *spin)
{
  btk_spin_button_set_digits (BTK_SPIN_BUTTON (spinner1),
			      btk_spin_button_get_value_as_int (spin));
}

static void
get_value (BtkWidget *widget, gpointer data)
{
  gchar buf[32];
  BtkLabel *label;
  BtkSpinButton *spin;

  spin = BTK_SPIN_BUTTON (spinner1);
  label = BTK_LABEL (g_object_get_data (B_OBJECT (widget), "user_data"));
  if (GPOINTER_TO_INT (data) == 1)
    sprintf (buf, "%d", btk_spin_button_get_value_as_int (spin));
  else
    sprintf (buf, "%0.*f", spin->digits, btk_spin_button_get_value (spin));
  btk_label_set_text (label, buf);
}

static void
get_spin_value (BtkWidget *widget, gpointer data)
{
  gchar *buffer;
  BtkLabel *label;
  BtkSpinButton *spin;

  spin = BTK_SPIN_BUTTON (widget);
  label = BTK_LABEL (data);

  buffer = g_strdup_printf ("%0.*f", spin->digits,
			    btk_spin_button_get_value (spin));
  btk_label_set_text (label, buffer);

  g_free (buffer);
}

static gint
spin_button_time_output_func (BtkSpinButton *spin_button)
{
  static gchar buf[6];
  gdouble hours;
  gdouble minutes;

  hours = spin_button->adjustment->value / 60.0;
  minutes = (fabs(floor (hours) - hours) < 1e-5) ? 0.0 : 30;
  sprintf (buf, "%02.0f:%02.0f", floor (hours), minutes);
  if (strcmp (buf, btk_entry_get_text (BTK_ENTRY (spin_button))))
    btk_entry_set_text (BTK_ENTRY (spin_button), buf);
  return TRUE;
}

static gint
spin_button_month_input_func (BtkSpinButton *spin_button,
			      gdouble       *new_val)
{
  gint i;
  static gchar *month[12] = { "January", "February", "March", "April",
			      "May", "June", "July", "August",
			      "September", "October", "November", "December" };
  gchar *tmp1, *tmp2;
  gboolean found = FALSE;

  for (i = 1; i <= 12; i++)
    {
      tmp1 = g_ascii_strup (month[i - 1], -1);
      tmp2 = g_ascii_strup (btk_entry_get_text (BTK_ENTRY (spin_button)), -1);
      if (strstr (tmp1, tmp2) == tmp1)
	found = TRUE;
      g_free (tmp1);
      g_free (tmp2);
      if (found)
	break;
    }
  if (!found)
    {
      *new_val = 0.0;
      return BTK_INPUT_ERROR;
    }
  *new_val = (gdouble) i;
  return TRUE;
}

static gint
spin_button_month_output_func (BtkSpinButton *spin_button)
{
  gint i;
  static gchar *month[12] = { "January", "February", "March", "April",
			      "May", "June", "July", "August", "September",
			      "October", "November", "December" };

  for (i = 1; i <= 12; i++)
    if (fabs (spin_button->adjustment->value - (double)i) < 1e-5)
      {
	if (strcmp (month[i-1], btk_entry_get_text (BTK_ENTRY (spin_button))))
	  btk_entry_set_text (BTK_ENTRY (spin_button), month[i-1]);
      }
  return TRUE;
}

static gint
spin_button_hex_input_func (BtkSpinButton *spin_button,
			    gdouble       *new_val)
{
  const gchar *buf;
  gchar *err;
  gdouble res;

  buf = btk_entry_get_text (BTK_ENTRY (spin_button));
  res = strtol(buf, &err, 16);
  *new_val = res;
  if (*err)
    return BTK_INPUT_ERROR;
  else
    return TRUE;
}

static gint
spin_button_hex_output_func (BtkSpinButton *spin_button)
{
  static gchar buf[7];
  gint val;

  val = (gint) spin_button->adjustment->value;
  if (fabs (val) < 1e-5)
    sprintf (buf, "0x00");
  else
    sprintf (buf, "0x%.2X", val);
  if (strcmp (buf, btk_entry_get_text (BTK_ENTRY (spin_button))))
    btk_entry_set_text (BTK_ENTRY (spin_button), buf);
  return TRUE;
}

static void
create_spins (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  BtkWidget *frame;
  BtkWidget *hbox;
  BtkWidget *main_vbox;
  BtkWidget *vbox;
  BtkWidget *vbox2;
  BtkWidget *spinner2;
  BtkWidget *spinner;
  BtkWidget *button;
  BtkWidget *label;
  BtkWidget *val_label;
  BtkAdjustment *adj;

  if (!window)
    {
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));
      
      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window);
      
      btk_window_set_title (BTK_WINDOW (window), "BtkSpinButton");
      
      main_vbox = btk_vbox_new (FALSE, 5);
      btk_container_set_border_width (BTK_CONTAINER (main_vbox), 10);
      btk_container_add (BTK_CONTAINER (window), main_vbox);
      
      frame = btk_frame_new ("Not accelerated");
      btk_box_pack_start (BTK_BOX (main_vbox), frame, TRUE, TRUE, 0);
      
      vbox = btk_vbox_new (FALSE, 0);
      btk_container_set_border_width (BTK_CONTAINER (vbox), 5);
      btk_container_add (BTK_CONTAINER (frame), vbox);
      
      /* Time, month, hex spinners */
      
      hbox = btk_hbox_new (FALSE, 0);
      btk_box_pack_start (BTK_BOX (vbox), hbox, TRUE, TRUE, 5);
      
      vbox2 = btk_vbox_new (FALSE, 0);
      btk_box_pack_start (BTK_BOX (hbox), vbox2, TRUE, TRUE, 5);
      
      label = btk_label_new ("Time :");
      btk_misc_set_alignment (BTK_MISC (label), 0, 0.5);
      btk_box_pack_start (BTK_BOX (vbox2), label, FALSE, TRUE, 0);
      
      adj = (BtkAdjustment *) btk_adjustment_new (0, 0, 1410, 30, 60, 0);
      spinner = btk_spin_button_new (adj, 0, 0);
      btk_editable_set_editable (BTK_EDITABLE (spinner), FALSE);
      g_signal_connect (spinner,
			"output",
			G_CALLBACK (spin_button_time_output_func),
			NULL);
      btk_spin_button_set_wrap (BTK_SPIN_BUTTON (spinner), TRUE);
      btk_entry_set_width_chars (BTK_ENTRY (spinner), 5);
      btk_box_pack_start (BTK_BOX (vbox2), spinner, FALSE, TRUE, 0);

      vbox2 = btk_vbox_new (FALSE, 0);
      btk_box_pack_start (BTK_BOX (hbox), vbox2, TRUE, TRUE, 5);
      
      label = btk_label_new ("Month :");
      btk_misc_set_alignment (BTK_MISC (label), 0, 0.5);
      btk_box_pack_start (BTK_BOX (vbox2), label, FALSE, TRUE, 0);
      
      adj = (BtkAdjustment *) btk_adjustment_new (1.0, 1.0, 12.0, 1.0,
						  5.0, 0.0);
      spinner = btk_spin_button_new (adj, 0, 0);
      btk_spin_button_set_update_policy (BTK_SPIN_BUTTON (spinner),
					 BTK_UPDATE_IF_VALID);
      g_signal_connect (spinner,
			"input",
			G_CALLBACK (spin_button_month_input_func),
			NULL);
      g_signal_connect (spinner,
			"output",
			G_CALLBACK (spin_button_month_output_func),
			NULL);
      btk_spin_button_set_wrap (BTK_SPIN_BUTTON (spinner), TRUE);
      btk_entry_set_width_chars (BTK_ENTRY (spinner), 9);
      btk_box_pack_start (BTK_BOX (vbox2), spinner, FALSE, TRUE, 0);
      
      vbox2 = btk_vbox_new (FALSE, 0);
      btk_box_pack_start (BTK_BOX (hbox), vbox2, TRUE, TRUE, 5);

      label = btk_label_new ("Hex :");
      btk_misc_set_alignment (BTK_MISC (label), 0, 0.5);
      btk_box_pack_start (BTK_BOX (vbox2), label, FALSE, TRUE, 0);

      adj = (BtkAdjustment *) btk_adjustment_new (0, 0, 255, 1, 16, 0);
      spinner = btk_spin_button_new (adj, 0, 0);
      btk_editable_set_editable (BTK_EDITABLE (spinner), TRUE);
      g_signal_connect (spinner,
			"input",
			G_CALLBACK (spin_button_hex_input_func),
			NULL);
      g_signal_connect (spinner,
			"output",
			G_CALLBACK (spin_button_hex_output_func),
			NULL);
      btk_spin_button_set_wrap (BTK_SPIN_BUTTON (spinner), TRUE);
      btk_entry_set_width_chars (BTK_ENTRY (spinner), 4);
      btk_box_pack_start (BTK_BOX (vbox2), spinner, FALSE, TRUE, 0);

      frame = btk_frame_new ("Accelerated");
      btk_box_pack_start (BTK_BOX (main_vbox), frame, TRUE, TRUE, 0);
  
      vbox = btk_vbox_new (FALSE, 0);
      btk_container_set_border_width (BTK_CONTAINER (vbox), 5);
      btk_container_add (BTK_CONTAINER (frame), vbox);
      
      hbox = btk_hbox_new (FALSE, 0);
      btk_box_pack_start (BTK_BOX (vbox), hbox, FALSE, TRUE, 5);
      
      vbox2 = btk_vbox_new (FALSE, 0);
      btk_box_pack_start (BTK_BOX (hbox), vbox2, FALSE, FALSE, 5);
      
      label = btk_label_new ("Value :");
      btk_misc_set_alignment (BTK_MISC (label), 0, 0.5);
      btk_box_pack_start (BTK_BOX (vbox2), label, FALSE, TRUE, 0);

      adj = (BtkAdjustment *) btk_adjustment_new (0.0, -10000.0, 10000.0,
						  0.5, 100.0, 0.0);
      spinner1 = btk_spin_button_new (adj, 1.0, 2);
      btk_spin_button_set_wrap (BTK_SPIN_BUTTON (spinner1), TRUE);
      btk_box_pack_start (BTK_BOX (vbox2), spinner1, FALSE, TRUE, 0);

      vbox2 = btk_vbox_new (FALSE, 0);
      btk_box_pack_start (BTK_BOX (hbox), vbox2, FALSE, FALSE, 5);

      label = btk_label_new ("Digits :");
      btk_misc_set_alignment (BTK_MISC (label), 0, 0.5);
      btk_box_pack_start (BTK_BOX (vbox2), label, FALSE, TRUE, 0);

      adj = (BtkAdjustment *) btk_adjustment_new (2, 1, 15, 1, 1, 0);
      spinner2 = btk_spin_button_new (adj, 0.0, 0);
      g_signal_connect (adj, "value_changed",
			G_CALLBACK (change_digits),
			spinner2);
      btk_box_pack_start (BTK_BOX (vbox2), spinner2, FALSE, TRUE, 0);

      hbox = btk_hbox_new (FALSE, 0);
      btk_box_pack_start (BTK_BOX (vbox), hbox, FALSE, FALSE, 5);

      button = btk_check_button_new_with_label ("Snap to 0.5-ticks");
      g_signal_connect (button, "clicked",
			G_CALLBACK (toggle_snap),
			spinner1);
      btk_box_pack_start (BTK_BOX (vbox), button, TRUE, TRUE, 0);
      btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (button), TRUE);

      button = btk_check_button_new_with_label ("Numeric only input mode");
      g_signal_connect (button, "clicked",
			G_CALLBACK (toggle_numeric),
			spinner1);
      btk_box_pack_start (BTK_BOX (vbox), button, TRUE, TRUE, 0);
      btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (button), TRUE);

      val_label = btk_label_new ("");

      hbox = btk_hbox_new (FALSE, 0);
      btk_box_pack_start (BTK_BOX (vbox), hbox, FALSE, TRUE, 5);

      button = btk_button_new_with_label ("Value as Int");
      g_object_set_data (B_OBJECT (button), "user_data", val_label);
      g_signal_connect (button, "clicked",
			G_CALLBACK (get_value),
			GINT_TO_POINTER (1));
      btk_box_pack_start (BTK_BOX (hbox), button, TRUE, TRUE, 5);

      button = btk_button_new_with_label ("Value as Float");
      g_object_set_data (B_OBJECT (button), "user_data", val_label);
      g_signal_connect (button, "clicked",
			G_CALLBACK (get_value),
			GINT_TO_POINTER (2));
      btk_box_pack_start (BTK_BOX (hbox), button, TRUE, TRUE, 5);

      btk_box_pack_start (BTK_BOX (vbox), val_label, TRUE, TRUE, 0);
      btk_label_set_text (BTK_LABEL (val_label), "0");

      frame = btk_frame_new ("Using Convenience Constructor");
      btk_box_pack_start (BTK_BOX (main_vbox), frame, TRUE, TRUE, 0);
  
      hbox = btk_hbox_new (FALSE, 0);
      btk_container_set_border_width (BTK_CONTAINER (hbox), 5);
      btk_container_add (BTK_CONTAINER (frame), hbox);
      
      val_label = btk_label_new ("0.0");

      spinner = btk_spin_button_new_with_range (0.0, 10.0, 0.009);
      btk_spin_button_set_value (BTK_SPIN_BUTTON (spinner), 0.0);
      g_signal_connect (spinner, "value_changed",
			G_CALLBACK (get_spin_value), val_label);
      btk_box_pack_start (BTK_BOX (hbox), spinner, TRUE, TRUE, 5);
      btk_box_pack_start (BTK_BOX (hbox), val_label, TRUE, TRUE, 5);

      hbox = btk_hbox_new (FALSE, 0);
      btk_box_pack_start (BTK_BOX (main_vbox), hbox, FALSE, TRUE, 0);
  
      button = btk_button_new_with_label ("Close");
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (btk_widget_destroy),
				window);
      btk_box_pack_start (BTK_BOX (hbox), button, TRUE, TRUE, 5);
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    btk_widget_destroy (window);
}


/*
 * Cursors
 */

static gint
cursor_expose_event (BtkWidget *widget,
		     BdkEvent  *event,
		     gpointer   user_data)
{
  BtkDrawingArea *darea;
  BdkDrawable *drawable;
  guint max_width;
  guint max_height;
  bairo_t *cr;

  g_return_val_if_fail (widget != NULL, TRUE);
  g_return_val_if_fail (BTK_IS_DRAWING_AREA (widget), TRUE);

  darea = BTK_DRAWING_AREA (widget);
  drawable = widget->window;
  max_width = widget->allocation.width;
  max_height = widget->allocation.height;

  cr = bdk_bairo_create (drawable);

  bairo_set_source_rgb (cr, 1, 1, 1);
  bairo_rectangle (cr, 0, 0, max_width, max_height / 2);
  bairo_fill (cr);

  bairo_set_source_rgb (cr, 0, 0, 0);
  bairo_rectangle (cr, 0, max_height / 2, max_width, max_height / 2);
  bairo_fill (cr);

  bdk_bairo_set_source_color (cr, &widget->style->bg[BTK_STATE_NORMAL]);
  bairo_rectangle (cr, max_width / 3, max_height / 3, max_width / 3, max_height / 3);
  bairo_fill (cr);

  bairo_destroy (cr);

  return TRUE;
}

static void
set_cursor (BtkWidget *spinner,
	    BtkWidget *widget)
{
  guint c;
  BdkCursor *cursor;
  BtkWidget *label;
  GEnumClass *class;
  GEnumValue *vals;

  c = CLAMP (btk_spin_button_get_value_as_int (BTK_SPIN_BUTTON (spinner)), 0, 152);
  c &= 0xfe;

  label = g_object_get_data (B_OBJECT (spinner), "user_data");
  
  class = g_type_class_ref (BDK_TYPE_CURSOR_TYPE);
  vals = class->values;

  while (vals && vals->value != c)
    vals++;
  if (vals)
    btk_label_set_text (BTK_LABEL (label), vals->value_nick);
  else
    btk_label_set_text (BTK_LABEL (label), "<unknown>");

  g_type_class_unref (class);

  cursor = bdk_cursor_new_for_display (btk_widget_get_display (widget), c);
  bdk_window_set_cursor (widget->window, cursor);
  bdk_cursor_unref (cursor);
}

static gint
cursor_event (BtkWidget          *widget,
	      BdkEvent           *event,
	      BtkSpinButton	 *spinner)
{
  if ((event->type == BDK_BUTTON_PRESS) &&
      ((event->button.button == 1) ||
       (event->button.button == 3)))
    {
      btk_spin_button_spin (spinner, event->button.button == 1 ?
			    BTK_SPIN_STEP_FORWARD : BTK_SPIN_STEP_BACKWARD, 0);
      return TRUE;
    }

  return FALSE;
}

#ifdef BDK_WINDOWING_X11
#include "x11/bdkx.h"

static void
change_cursor_theme (BtkWidget *widget,
		     gpointer   data)
{
  const gchar *theme;
  gint size;
  GList *children;

  children = btk_container_get_children (BTK_CONTAINER (data));

  theme = btk_entry_get_text (BTK_ENTRY (children->next->data));
  size = (gint) btk_spin_button_get_value (BTK_SPIN_BUTTON (children->next->next->data));

  g_list_free (children);

  bdk_x11_display_set_cursor_theme (btk_widget_get_display (widget),
				    theme, size);
}
#endif


static void
create_cursors (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  BtkWidget *frame;
  BtkWidget *hbox;
  BtkWidget *main_vbox;
  BtkWidget *vbox;
  BtkWidget *darea;
  BtkWidget *spinner;
  BtkWidget *button;
  BtkWidget *label;
  BtkWidget *any;
  BtkAdjustment *adj;
  BtkWidget *entry;
  BtkWidget *size;  

  if (!window)
    {
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_screen (BTK_WINDOW (window), 
			     btk_widget_get_screen (widget));
      
      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window);
      
      btk_window_set_title (BTK_WINDOW (window), "Cursors");
      
      main_vbox = btk_vbox_new (FALSE, 5);
      btk_container_set_border_width (BTK_CONTAINER (main_vbox), 0);
      btk_container_add (BTK_CONTAINER (window), main_vbox);

      vbox =
	g_object_new (btk_vbox_get_type (),
			"BtkBox::homogeneous", FALSE,
			"BtkBox::spacing", 5,
			"BtkContainer::border_width", 10,
			"BtkWidget::parent", main_vbox,
			"BtkWidget::visible", TRUE,
			NULL);

#ifdef BDK_WINDOWING_X11
      hbox = btk_hbox_new (FALSE, 0);
      btk_container_set_border_width (BTK_CONTAINER (hbox), 5);
      btk_box_pack_start (BTK_BOX (vbox), hbox, FALSE, TRUE, 0);

      label = btk_label_new ("Cursor Theme : ");
      btk_misc_set_alignment (BTK_MISC (label), 0, 0.5);
      btk_box_pack_start (BTK_BOX (hbox), label, FALSE, TRUE, 0);

      entry = btk_entry_new ();
      btk_entry_set_text (BTK_ENTRY (entry), "default");
      btk_box_pack_start (BTK_BOX (hbox), entry, FALSE, TRUE, 0);

      size = btk_spin_button_new_with_range (1.0, 64.0, 1.0);
      btk_spin_button_set_value (BTK_SPIN_BUTTON (size), 24.0);
      btk_box_pack_start (BTK_BOX (hbox), size, TRUE, TRUE, 0);
      
      g_signal_connect (entry, "changed", 
			G_CALLBACK (change_cursor_theme), hbox);
      g_signal_connect (size, "changed", 
			G_CALLBACK (change_cursor_theme), hbox);
#endif

      hbox = btk_hbox_new (FALSE, 0);
      btk_container_set_border_width (BTK_CONTAINER (hbox), 5);
      btk_box_pack_start (BTK_BOX (vbox), hbox, FALSE, TRUE, 0);

      label = btk_label_new ("Cursor Value : ");
      btk_misc_set_alignment (BTK_MISC (label), 0, 0.5);
      btk_box_pack_start (BTK_BOX (hbox), label, FALSE, TRUE, 0);
      
      adj = (BtkAdjustment *) btk_adjustment_new (0,
						  0, 152,
						  2,
						  10, 0);
      spinner = btk_spin_button_new (adj, 0, 0);
      btk_box_pack_start (BTK_BOX (hbox), spinner, TRUE, TRUE, 0);

      frame =
	g_object_new (btk_frame_get_type (),
			"BtkFrame::shadow", BTK_SHADOW_ETCHED_IN,
			"BtkFrame::label_xalign", 0.5,
			"BtkFrame::label", "Cursor Area",
			"BtkContainer::border_width", 10,
			"BtkWidget::parent", vbox,
			"BtkWidget::visible", TRUE,
			NULL);

      darea = btk_drawing_area_new ();
      btk_widget_set_size_request (darea, 80, 80);
      btk_container_add (BTK_CONTAINER (frame), darea);
      g_signal_connect (darea,
			"expose_event",
			G_CALLBACK (cursor_expose_event),
			NULL);
      btk_widget_set_events (darea, BDK_EXPOSURE_MASK | BDK_BUTTON_PRESS_MASK);
      g_signal_connect (darea,
			"button_press_event",
			G_CALLBACK (cursor_event),
			spinner);
      btk_widget_show (darea);

      g_signal_connect (spinner, "changed",
			G_CALLBACK (set_cursor),
			darea);

      label = g_object_new (BTK_TYPE_LABEL,
			      "visible", TRUE,
			      "label", "XXX",
			      "parent", vbox,
			      NULL);
      btk_container_child_set (BTK_CONTAINER (vbox), label,
			       "expand", FALSE,
			       NULL);
      g_object_set_data (B_OBJECT (spinner), "user_data", label);

      any =
	g_object_new (btk_hseparator_get_type (),
			"BtkWidget::visible", TRUE,
			NULL);
      btk_box_pack_start (BTK_BOX (main_vbox), any, FALSE, TRUE, 0);
  
      hbox = btk_hbox_new (FALSE, 0);
      btk_container_set_border_width (BTK_CONTAINER (hbox), 10);
      btk_box_pack_start (BTK_BOX (main_vbox), hbox, FALSE, TRUE, 0);

      button = btk_button_new_with_label ("Close");
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (btk_widget_destroy),
				window);
      btk_box_pack_start (BTK_BOX (hbox), button, TRUE, TRUE, 5);

      btk_widget_show_all (window);

      set_cursor (spinner, darea);
    }
  else
    btk_widget_destroy (window);
}

/*
 * BtkList
 */

static void
list_add (BtkWidget *widget,
	  BtkWidget *list)
{
  static int i = 1;
  gchar buffer[64];
  BtkWidget *list_item;
  BtkContainer *container;

  container = BTK_CONTAINER (list);

  sprintf (buffer, "added item %d", i++);
  list_item = btk_list_item_new_with_label (buffer);
  btk_widget_show (list_item);

  btk_container_add (container, list_item);
}

static void
list_remove (BtkWidget *widget,
	     BtkList   *list)
{
  GList *clear_list = NULL;
  GList *sel_row = NULL;
  GList *work = NULL;

  if (list->selection_mode == BTK_SELECTION_EXTENDED)
    {
      BtkWidget *item;

      item = BTK_CONTAINER (list)->focus_child;
      if (!item && list->selection)
	item = list->selection->data;

      if (item)
	{
	  work = g_list_find (list->children, item);
	  for (sel_row = work; sel_row; sel_row = sel_row->next)
	    if (BTK_WIDGET (sel_row->data)->state != BTK_STATE_SELECTED)
	      break;

	  if (!sel_row)
	    {
	      for (sel_row = work; sel_row; sel_row = sel_row->prev)
		if (BTK_WIDGET (sel_row->data)->state != BTK_STATE_SELECTED)
		  break;
	    }
	}
    }

  for (work = list->selection; work; work = work->next)
    clear_list = g_list_prepend (clear_list, work->data);

  clear_list = g_list_reverse (clear_list);
  btk_list_remove_items (BTK_LIST (list), clear_list);
  g_list_free (clear_list);

  if (list->selection_mode == BTK_SELECTION_EXTENDED && sel_row)
    btk_list_select_child (list, BTK_WIDGET(sel_row->data));
}

static gchar *selection_mode_items[] =
{
  "Single",
  "Browse",
  "Multiple"
};

static const BtkSelectionMode selection_modes[] = {
  BTK_SELECTION_SINGLE,
  BTK_SELECTION_BROWSE,
  BTK_SELECTION_MULTIPLE
};

static BtkWidget *list_omenu;

static void 
list_toggle_sel_mode (BtkWidget *widget, gpointer data)
{
  BtkList *list;
  gint i;

  list = BTK_LIST (data);

  if (!btk_widget_get_mapped (widget))
    return;

  i = btk_option_menu_get_history (BTK_OPTION_MENU (widget));

  btk_list_set_selection_mode (list, selection_modes[i]);
}

static void
create_list (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  BtkComboBoxText *cb;
  BtkWidget *cb_entry;

  if (!window)
    {
      BtkWidget *cbox;
      BtkWidget *vbox;
      BtkWidget *hbox;
      BtkWidget *label;
      BtkWidget *scrolled_win;
      BtkWidget *list;
      BtkWidget *button;
      BtkWidget *separator;
      FILE *infile;

      window = btk_window_new (BTK_WINDOW_TOPLEVEL);

      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window);

      btk_window_set_title (BTK_WINDOW (window), "list");
      btk_container_set_border_width (BTK_CONTAINER (window), 0);

      vbox = btk_vbox_new (FALSE, 0);
      btk_container_add (BTK_CONTAINER (window), vbox);

      scrolled_win = btk_scrolled_window_new (NULL, NULL);
      btk_container_set_border_width (BTK_CONTAINER (scrolled_win), 5);
      btk_widget_set_size_request (scrolled_win, -1, 300);
      btk_box_pack_start (BTK_BOX (vbox), scrolled_win, TRUE, TRUE, 0);
      btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (scrolled_win),
				      BTK_POLICY_AUTOMATIC,
				      BTK_POLICY_AUTOMATIC);

      list = btk_list_new ();
      btk_list_set_selection_mode (BTK_LIST (list), BTK_SELECTION_SINGLE);
      btk_scrolled_window_add_with_viewport
	(BTK_SCROLLED_WINDOW (scrolled_win), list);
      btk_container_set_focus_vadjustment
	(BTK_CONTAINER (list),
	 btk_scrolled_window_get_vadjustment
	 (BTK_SCROLLED_WINDOW (scrolled_win)));
      btk_container_set_focus_hadjustment
	(BTK_CONTAINER (list),
	 btk_scrolled_window_get_hadjustment
	 (BTK_SCROLLED_WINDOW (scrolled_win)));

      if ((infile = fopen("../btk/btkenums.h", "r")))
	{
	  char buffer[256];
	  char *pos;
	  BtkWidget *item;

	  while (fgets (buffer, 256, infile))
	    {
	      if ((pos = strchr (buffer, '\n')))
		*pos = 0;
	      item = btk_list_item_new_with_label (buffer);
	      btk_container_add (BTK_CONTAINER (list), item);
	    }
	  
	  fclose (infile);
	}


      hbox = btk_hbox_new (TRUE, 5);
      btk_container_set_border_width (BTK_CONTAINER (hbox), 5);
      btk_box_pack_start (BTK_BOX (vbox), hbox, FALSE, TRUE, 0);

      button = btk_button_new_with_label ("Insert Row");
      btk_box_pack_start (BTK_BOX (hbox), button, TRUE, TRUE, 0);
      g_signal_connect (button, "clicked",
			G_CALLBACK (list_add),
			list);

      cb = BTK_COMBO_BOX_TEXT (btk_combo_box_text_new_with_entry ());

      btk_combo_box_text_append_text (cb, "item0");
      btk_combo_box_text_append_text (cb, "item0");
      btk_combo_box_text_append_text (cb, "item1 item1");
      btk_combo_box_text_append_text (cb, "item2 item2 item2");
      btk_combo_box_text_append_text (cb, "item3 item3 item3 item3");
      btk_combo_box_text_append_text (cb, "item4 item4 item4 item4 item4");
      btk_combo_box_text_append_text (cb, "item5 item5 item5 item5 item5 item5");
      btk_combo_box_text_append_text (cb, "item6 item6 item6 item6 item6");
      btk_combo_box_text_append_text (cb, "item7 item7 item7 item7");
      btk_combo_box_text_append_text (cb, "item8 item8 item8");
      btk_combo_box_text_append_text (cb, "item9 item9");

      cb_entry = btk_bin_get_child (BTK_BIN (cb));
      btk_entry_set_text (BTK_ENTRY (cb_entry), "hello world \n\n\n foo");
      btk_editable_select_rebunnyion (BTK_EDITABLE (cb_entry), 0, -1);
      btk_box_pack_start (BTK_BOX (hbox), BTK_WIDGET (cb), TRUE, TRUE, 0);

      button = btk_button_new_with_label ("Remove Selection");
      btk_box_pack_start (BTK_BOX (hbox), button, TRUE, TRUE, 0);
      g_signal_connect (button, "clicked",
			G_CALLBACK (list_remove),
			list);

      cbox = btk_hbox_new (FALSE, 0);
      btk_box_pack_start (BTK_BOX (vbox), cbox, FALSE, TRUE, 0);

      hbox = btk_hbox_new (FALSE, 5);
      btk_container_set_border_width (BTK_CONTAINER (hbox), 5);
      btk_box_pack_start (BTK_BOX (cbox), hbox, TRUE, FALSE, 0);

      label = btk_label_new ("Selection Mode :");
      btk_box_pack_start (BTK_BOX (hbox), label, FALSE, TRUE, 0);

      list_omenu = build_option_menu (selection_mode_items, 3, 3, 
				      list_toggle_sel_mode,
				      list);
      btk_box_pack_start (BTK_BOX (hbox), list_omenu, FALSE, TRUE, 0);

      separator = btk_hseparator_new ();
      btk_box_pack_start (BTK_BOX (vbox), separator, FALSE, TRUE, 0);

      cbox = btk_hbox_new (FALSE, 0);
      btk_box_pack_start (BTK_BOX (vbox), cbox, FALSE, TRUE, 0);

      button = btk_button_new_with_label ("close");
      btk_container_set_border_width (BTK_CONTAINER (button), 10);
      btk_box_pack_start (BTK_BOX (cbox), button, TRUE, TRUE, 0);
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (btk_widget_destroy),
				window);

      btk_widget_set_can_default (button, TRUE);
      btk_widget_grab_default (button);
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    btk_widget_destroy (window);
}

/*
 * BtkCList
 */

static char * book_open_xpm[] = {
"16 16 4 1",
"       c None s None",
".      c black",
"X      c #808080",
"o      c white",
"                ",
"  ..            ",
" .Xo.    ...    ",
" .Xoo. ..oo.    ",
" .Xooo.Xooo...  ",
" .Xooo.oooo.X.  ",
" .Xooo.Xooo.X.  ",
" .Xooo.oooo.X.  ",
" .Xooo.Xooo.X.  ",
" .Xooo.oooo.X.  ",
"  .Xoo.Xoo..X.  ",
"   .Xo.o..ooX.  ",
"    .X..XXXXX.  ",
"    ..X.......  ",
"     ..         ",
"                "};

static char * book_closed_xpm[] = {
"16 16 6 1",
"       c None s None",
".      c black",
"X      c red",
"o      c yellow",
"O      c #808080",
"#      c white",
"                ",
"       ..       ",
"     ..XX.      ",
"   ..XXXXX.     ",
" ..XXXXXXXX.    ",
".ooXXXXXXXXX.   ",
"..ooXXXXXXXXX.  ",
".X.ooXXXXXXXXX. ",
".XX.ooXXXXXX..  ",
" .XX.ooXXX..#O  ",
"  .XX.oo..##OO. ",
"   .XX..##OO..  ",
"    .X.#OO..    ",
"     ..O..      ",
"      ..        ",
"                "};

static char * mini_page_xpm[] = {
"16 16 4 1",
"       c None s None",
".      c black",
"X      c white",
"o      c #808080",
"                ",
"   .......      ",
"   .XXXXX..     ",
"   .XoooX.X.    ",
"   .XXXXX....   ",
"   .XooooXoo.o  ",
"   .XXXXXXXX.o  ",
"   .XooooooX.o  ",
"   .XXXXXXXX.o  ",
"   .XooooooX.o  ",
"   .XXXXXXXX.o  ",
"   .XooooooX.o  ",
"   .XXXXXXXX.o  ",
"   ..........o  ",
"    oooooooooo  ",
"                "};

static char * btk_mini_xpm[] = {
"15 20 17 1",
"       c None",
".      c #14121F",
"+      c #278828",
"@      c #9B3334",
"#      c #284C72",
"$      c #24692A",
"%      c #69282E",
"&      c #37C539",
"*      c #1D2F4D",
"=      c #6D7076",
"-      c #7D8482",
";      c #E24A49",
">      c #515357",
",      c #9B9C9B",
"'      c #2FA232",
")      c #3CE23D",
"!      c #3B6CCB",
"               ",
"      ***>     ",
"    >.*!!!*    ",
"   ***....#*=  ",
"  *!*.!!!**!!# ",
" .!!#*!#*!!!!# ",
" @%#!.##.*!!$& ",
" @;%*!*.#!#')) ",
" @;;@%!!*$&)'' ",
" @%.%@%$'&)$+' ",
" @;...@$'*'*)+ ",
" @;%..@$+*.')$ ",
" @;%%;;$+..$)# ",
" @;%%;@$$$'.$# ",
" %;@@;;$$+))&* ",
"  %;;;@+$&)&*  ",
"   %;;@'))+>   ",
"    %;@'&#     ",
"     >%$$      ",
"      >=       "};

#define TESTBTK_CLIST_COLUMNS 12
static gint clist_rows = 0;
static BtkWidget *clist_omenu;

static void
add1000_clist (BtkWidget *widget, gpointer data)
{
  gint i, row;
  char text[TESTBTK_CLIST_COLUMNS][50];
  char *texts[TESTBTK_CLIST_COLUMNS];
  BdkBitmap *mask;
  BdkPixmap *pixmap;
  BtkCList  *clist;

  clist = BTK_CLIST (data);

  pixmap = bdk_pixmap_create_from_xpm_d (clist->clist_window,
					 &mask, 
					 &BTK_WIDGET (data)->style->white,
					 btk_mini_xpm);

  for (i = 0; i < TESTBTK_CLIST_COLUMNS; i++)
    {
      texts[i] = text[i];
      sprintf (text[i], "Column %d", i);
    }
  
  texts[3] = NULL;
  sprintf (text[1], "Right");
  sprintf (text[2], "Center");
  
  btk_clist_freeze (BTK_CLIST (data));
  for (i = 0; i < 1000; i++)
    {
      sprintf (text[0], "CListRow %d", rand() % 10000);
      row = btk_clist_append (clist, texts);
      btk_clist_set_pixtext (clist, row, 3, "btk+", 5, pixmap, mask);
    }

  btk_clist_thaw (BTK_CLIST (data));

  g_object_unref (pixmap);
  g_object_unref (mask);
}

static void
add10000_clist (BtkWidget *widget, gpointer data)
{
  gint i;
  char text[TESTBTK_CLIST_COLUMNS][50];
  char *texts[TESTBTK_CLIST_COLUMNS];

  for (i = 0; i < TESTBTK_CLIST_COLUMNS; i++)
    {
      texts[i] = text[i];
      sprintf (text[i], "Column %d", i);
    }
  
  sprintf (text[1], "Right");
  sprintf (text[2], "Center");
  
  btk_clist_freeze (BTK_CLIST (data));
  for (i = 0; i < 10000; i++)
    {
      sprintf (text[0], "CListRow %d", rand() % 10000);
      btk_clist_append (BTK_CLIST (data), texts);
    }
  btk_clist_thaw (BTK_CLIST (data));
}

void
clear_clist (BtkWidget *widget, gpointer data)
{
  btk_clist_clear (BTK_CLIST (data));
  clist_rows = 0;
}

void clist_remove_selection (BtkWidget *widget, BtkCList *clist)
{
  btk_clist_freeze (clist);

  while (clist->selection)
    {
      gint row;

      clist_rows--;
      row = GPOINTER_TO_INT (clist->selection->data);

      btk_clist_remove (clist, row);

      if (clist->selection_mode == BTK_SELECTION_BROWSE)
	break;
    }

  if (clist->selection_mode == BTK_SELECTION_EXTENDED && !clist->selection &&
      clist->focus_row >= 0)
    btk_clist_select_row (clist, clist->focus_row, -1);

  btk_clist_thaw (clist);
}

void toggle_title_buttons (BtkWidget *widget, BtkCList *clist)
{
  if (BTK_TOGGLE_BUTTON (widget)->active)
    btk_clist_column_titles_show (clist);
  else
    btk_clist_column_titles_hide (clist);
}

void toggle_reorderable (BtkWidget *widget, BtkCList *clist)
{
  btk_clist_set_reorderable (clist, BTK_TOGGLE_BUTTON (widget)->active);
}

static void
insert_row_clist (BtkWidget *widget, gpointer data)
{
  static char *text[] =
  {
    "This", "is an", "inserted", "row.",
    "This", "is an", "inserted", "row.",
    "This", "is an", "inserted", "row."
  };

  static BtkStyle *style1 = NULL;
  static BtkStyle *style2 = NULL;
  static BtkStyle *style3 = NULL;
  gint row;
  
  if (BTK_CLIST (data)->focus_row >= 0)
    row = btk_clist_insert (BTK_CLIST (data), BTK_CLIST (data)->focus_row,
			    text);
  else
    row = btk_clist_prepend (BTK_CLIST (data), text);

  if (!style1)
    {
      BdkColor col1 = { 0, 0, 56000, 0};
      BdkColor col2 = { 0, 32000, 0, 56000};

      style1 = btk_style_copy (BTK_WIDGET (data)->style);
      style1->base[BTK_STATE_NORMAL] = col1;
      style1->base[BTK_STATE_SELECTED] = col2;

      style2 = btk_style_copy (BTK_WIDGET (data)->style);
      style2->fg[BTK_STATE_NORMAL] = col1;
      style2->fg[BTK_STATE_SELECTED] = col2;

      style3 = btk_style_copy (BTK_WIDGET (data)->style);
      style3->fg[BTK_STATE_NORMAL] = col1;
      style3->base[BTK_STATE_NORMAL] = col2;
      bango_font_description_free (style3->font_desc);
      style3->font_desc = bango_font_description_from_string ("courier 12");
    }

  btk_clist_set_cell_style (BTK_CLIST (data), row, 3, style1);
  btk_clist_set_cell_style (BTK_CLIST (data), row, 4, style2);
  btk_clist_set_cell_style (BTK_CLIST (data), row, 0, style3);

  clist_rows++;
}

static void
clist_warning_test (BtkWidget *button,
		    BtkWidget *clist)
{
  BtkWidget *child;
  static gboolean add_remove = FALSE;

  add_remove = !add_remove;

  child = btk_label_new ("Test");
  g_object_ref (child);
  btk_object_sink (BTK_OBJECT (child));

  if (add_remove)
    btk_container_add (BTK_CONTAINER (clist), child);
  else
    {
      child->parent = clist;
      btk_container_remove (BTK_CONTAINER (clist), child);
      child->parent = NULL;
    }

  btk_widget_destroy (child);
  g_object_unref (child);
}

static void
undo_selection (BtkWidget *button, BtkCList *clist)
{
  btk_clist_undo_selection (clist);
}

static void 
clist_toggle_sel_mode (BtkWidget *widget, gpointer data)
{
  BtkCList *clist;
  gint i;

  clist = BTK_CLIST (data);

  if (!btk_widget_get_mapped (widget))
    return;

  i = btk_option_menu_get_history (BTK_OPTION_MENU (widget));

  btk_clist_set_selection_mode (clist, selection_modes[i]);
}

static void 
clist_click_column (BtkCList *clist, gint column, gpointer data)
{
  if (column == 4)
    btk_clist_set_column_visibility (clist, column, FALSE);
  else if (column == clist->sort_column)
    {
      if (clist->sort_type == BTK_SORT_ASCENDING)
	clist->sort_type = BTK_SORT_DESCENDING;
      else
	clist->sort_type = BTK_SORT_ASCENDING;
    }
  else
    btk_clist_set_sort_column (clist, column);

  btk_clist_sort (clist);
}

static void
create_clist (BtkWidget *widget)
{
  gint i;
  static BtkWidget *window = NULL;

  static char *titles[] =
  {
    "auto resize", "not resizeable", "max width 100", "min width 50",
    "hide column", "Title 5", "Title 6", "Title 7",
    "Title 8",  "Title 9",  "Title 10", "Title 11"
  };

  char text[TESTBTK_CLIST_COLUMNS][50];
  char *texts[TESTBTK_CLIST_COLUMNS];

  BtkWidget *vbox;
  BtkWidget *hbox;
  BtkWidget *clist;
  BtkWidget *button;
  BtkWidget *separator;
  BtkWidget *scrolled_win;
  BtkWidget *check;

  BtkWidget *undo_button;
  BtkWidget *label;

  BtkStyle *style;
  BdkColor red_col = { 0, 56000, 0, 0};
  BdkColor light_green_col = { 0, 0, 56000, 32000};

  if (!window)
    {
      clist_rows = 0;
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_screen (BTK_WINDOW (window), 
			     btk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed), &window);

      btk_window_set_title (BTK_WINDOW (window), "clist");
      btk_container_set_border_width (BTK_CONTAINER (window), 0);

      vbox = btk_vbox_new (FALSE, 0);
      btk_container_add (BTK_CONTAINER (window), vbox);

      scrolled_win = btk_scrolled_window_new (NULL, NULL);
      btk_container_set_border_width (BTK_CONTAINER (scrolled_win), 5);
      btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (scrolled_win),
				      BTK_POLICY_AUTOMATIC, 
				      BTK_POLICY_AUTOMATIC);

      /* create BtkCList here so we have a pointer to throw at the 
       * button callbacks -- more is done with it later */
      clist = btk_clist_new_with_titles (TESTBTK_CLIST_COLUMNS, titles);
      btk_container_add (BTK_CONTAINER (scrolled_win), clist);
      g_signal_connect (clist, "click_column",
			G_CALLBACK (clist_click_column), NULL);

      /* control buttons */
      hbox = btk_hbox_new (FALSE, 5);
      btk_container_set_border_width (BTK_CONTAINER (hbox), 5);
      btk_box_pack_start (BTK_BOX (vbox), hbox, FALSE, FALSE, 0);

      button = btk_button_new_with_label ("Insert Row");
      btk_box_pack_start (BTK_BOX (hbox), button, TRUE, TRUE, 0);
      g_signal_connect (button, "clicked",
			G_CALLBACK (insert_row_clist), clist);

      button = btk_button_new_with_label ("Add 1,000 Rows With Pixmaps");
      btk_box_pack_start (BTK_BOX (hbox), button, TRUE, TRUE, 0);
      g_signal_connect (button, "clicked",
			G_CALLBACK (add1000_clist), clist);

      button = btk_button_new_with_label ("Add 10,000 Rows");
      btk_box_pack_start (BTK_BOX (hbox), button, TRUE, TRUE, 0);
      g_signal_connect (button, "clicked",
			G_CALLBACK (add10000_clist), clist);

      /* second layer of buttons */
      hbox = btk_hbox_new (FALSE, 5);
      btk_container_set_border_width (BTK_CONTAINER (hbox), 5);
      btk_box_pack_start (BTK_BOX (vbox), hbox, FALSE, FALSE, 0);

      button = btk_button_new_with_label ("Clear List");
      btk_box_pack_start (BTK_BOX (hbox), button, TRUE, TRUE, 0);
      g_signal_connect (button, "clicked",
			G_CALLBACK (clear_clist), clist);

      button = btk_button_new_with_label ("Remove Selection");
      btk_box_pack_start (BTK_BOX (hbox), button, TRUE, TRUE, 0);
      g_signal_connect (button, "clicked",
			G_CALLBACK (clist_remove_selection), clist);

      undo_button = btk_button_new_with_label ("Undo Selection");
      btk_box_pack_start (BTK_BOX (hbox), undo_button, TRUE, TRUE, 0);
      g_signal_connect (undo_button, "clicked",
			G_CALLBACK (undo_selection), clist);

      button = btk_button_new_with_label ("Warning Test");
      btk_box_pack_start (BTK_BOX (hbox), button, TRUE, TRUE, 0);
      g_signal_connect (button, "clicked",
			G_CALLBACK (clist_warning_test), clist);

      /* third layer of buttons */
      hbox = btk_hbox_new (FALSE, 5);
      btk_container_set_border_width (BTK_CONTAINER (hbox), 5);
      btk_box_pack_start (BTK_BOX (vbox), hbox, FALSE, FALSE, 0);

      check = btk_check_button_new_with_label ("Show Title Buttons");
      btk_box_pack_start (BTK_BOX (hbox), check, FALSE, TRUE, 0);
      g_signal_connect (check, "clicked",
			G_CALLBACK (toggle_title_buttons), clist);
      btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (check), TRUE);

      check = btk_check_button_new_with_label ("Reorderable");
      btk_box_pack_start (BTK_BOX (hbox), check, FALSE, TRUE, 0);
      g_signal_connect (check, "clicked",
			G_CALLBACK (toggle_reorderable), clist);
      btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (check), TRUE);

      label = btk_label_new ("Selection Mode :");
      btk_box_pack_start (BTK_BOX (hbox), label, FALSE, TRUE, 0);

      clist_omenu = build_option_menu (selection_mode_items, 3, 3, 
				       clist_toggle_sel_mode,
				       clist);
      btk_box_pack_start (BTK_BOX (hbox), clist_omenu, FALSE, TRUE, 0);

      /* 
       * the rest of the clist configuration
       */

      btk_box_pack_start (BTK_BOX (vbox), scrolled_win, TRUE, TRUE, 0);
      btk_clist_set_row_height (BTK_CLIST (clist), 18);
      btk_widget_set_size_request (clist, -1, 300);

      for (i = 1; i < TESTBTK_CLIST_COLUMNS; i++)
	btk_clist_set_column_width (BTK_CLIST (clist), i, 80);

      btk_clist_set_column_auto_resize (BTK_CLIST (clist), 0, TRUE);
      btk_clist_set_column_resizeable (BTK_CLIST (clist), 1, FALSE);
      btk_clist_set_column_max_width (BTK_CLIST (clist), 2, 100);
      btk_clist_set_column_min_width (BTK_CLIST (clist), 3, 50);
      btk_clist_set_selection_mode (BTK_CLIST (clist), BTK_SELECTION_EXTENDED);
      btk_clist_set_column_justification (BTK_CLIST (clist), 1,
					  BTK_JUSTIFY_RIGHT);
      btk_clist_set_column_justification (BTK_CLIST (clist), 2,
					  BTK_JUSTIFY_CENTER);
      
      for (i = 0; i < TESTBTK_CLIST_COLUMNS; i++)
	{
	  texts[i] = text[i];
	  sprintf (text[i], "Column %d", i);
	}

      sprintf (text[1], "Right");
      sprintf (text[2], "Center");

      style = btk_style_new ();
      style->fg[BTK_STATE_NORMAL] = red_col;
      style->base[BTK_STATE_NORMAL] = light_green_col;

      bango_font_description_set_size (style->font_desc, 14 * BANGO_SCALE);
      bango_font_description_set_weight (style->font_desc, BANGO_WEIGHT_BOLD);

      for (i = 0; i < 10; i++)
	{
	  sprintf (text[0], "CListRow %d", clist_rows++);
	  btk_clist_append (BTK_CLIST (clist), texts);

	  switch (i % 4)
	    {
	    case 2:
	      btk_clist_set_row_style (BTK_CLIST (clist), i, style);
	      break;
	    default:
	      btk_clist_set_cell_style (BTK_CLIST (clist), i, i % 4, style);
	      break;
	    }
	}

      g_object_unref (style);
      
      separator = btk_hseparator_new ();
      btk_box_pack_start (BTK_BOX (vbox), separator, FALSE, TRUE, 0);

      hbox = btk_hbox_new (FALSE, 0);
      btk_box_pack_start (BTK_BOX (vbox), hbox, FALSE, TRUE, 0);

      button = btk_button_new_with_label ("close");
      btk_container_set_border_width (BTK_CONTAINER (button), 10);
      btk_box_pack_start (BTK_BOX (hbox), button, TRUE, TRUE, 0);
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (btk_widget_destroy),
				window);

      btk_widget_set_can_default (button, TRUE);
      btk_widget_grab_default (button);
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    {
      clist_rows = 0;
      btk_widget_destroy (window);
    }
}

/*
 * BtkCTree
 */

typedef struct 
{
  BdkPixmap *pixmap1;
  BdkPixmap *pixmap2;
  BdkPixmap *pixmap3;
  BdkBitmap *mask1;
  BdkBitmap *mask2;
  BdkBitmap *mask3;
} CTreePixmaps;

static gint books = 0;
static gint pages = 0;

static BtkWidget *book_label;
static BtkWidget *page_label;
static BtkWidget *sel_label;
static BtkWidget *vis_label;
static BtkWidget *omenu1;
static BtkWidget *omenu2;
static BtkWidget *omenu3;
static BtkWidget *omenu4;
static BtkWidget *spin1;
static BtkWidget *spin2;
static BtkWidget *spin3;
static gint line_style;


static CTreePixmaps *
get_ctree_pixmaps (BtkCTree *ctree)
{
  BdkScreen *screen = btk_widget_get_screen (BTK_WIDGET (ctree));
  CTreePixmaps *pixmaps = g_object_get_data (B_OBJECT (screen), "ctree-pixmaps");

  if (!pixmaps)
    {
      BdkColormap *colormap = bdk_screen_get_rgb_colormap (screen);
      pixmaps = g_new (CTreePixmaps, 1);
      
      pixmaps->pixmap1 = bdk_pixmap_colormap_create_from_xpm_d (NULL, colormap,
								&pixmaps->mask1, 
								NULL, book_closed_xpm);
      pixmaps->pixmap2 = bdk_pixmap_colormap_create_from_xpm_d (NULL, colormap,
								&pixmaps->mask2, 
								NULL, book_open_xpm);
      pixmaps->pixmap3 = bdk_pixmap_colormap_create_from_xpm_d (NULL, colormap,
								&pixmaps->mask3,
								NULL, mini_page_xpm);
      
      g_object_set_data (B_OBJECT (screen), "ctree-pixmaps", pixmaps);
    }

  return pixmaps;
}

void after_press (BtkCTree *ctree, gpointer data)
{
  char buf[80];

  sprintf (buf, "%d", g_list_length (BTK_CLIST (ctree)->selection));
  btk_label_set_text (BTK_LABEL (sel_label), buf);

  sprintf (buf, "%d", g_list_length (BTK_CLIST (ctree)->row_list));
  btk_label_set_text (BTK_LABEL (vis_label), buf);

  sprintf (buf, "%d", books);
  btk_label_set_text (BTK_LABEL (book_label), buf);

  sprintf (buf, "%d", pages);
  btk_label_set_text (BTK_LABEL (page_label), buf);
}

void after_move (BtkCTree *ctree, BtkCTreeNode *child, BtkCTreeNode *parent, 
		 BtkCTreeNode *sibling, gpointer data)
{
  char *source;
  char *target1;
  char *target2;

  btk_ctree_get_node_info (ctree, child, &source, 
			   NULL, NULL, NULL, NULL, NULL, NULL, NULL);
  if (parent)
    btk_ctree_get_node_info (ctree, parent, &target1, 
			     NULL, NULL, NULL, NULL, NULL, NULL, NULL);
  if (sibling)
    btk_ctree_get_node_info (ctree, sibling, &target2, 
			     NULL, NULL, NULL, NULL, NULL, NULL, NULL);

  g_print ("Moving \"%s\" to \"%s\" with sibling \"%s\".\n", source,
	   (parent) ? target1 : "nil", (sibling) ? target2 : "nil");
}

void count_items (BtkCTree *ctree, BtkCTreeNode *list)
{
  if (BTK_CTREE_ROW (list)->is_leaf)
    pages--;
  else
    books--;
}

void expand_all (BtkWidget *widget, BtkCTree *ctree)
{
  btk_ctree_expand_recursive (ctree, NULL);
  after_press (ctree, NULL);
}

void collapse_all (BtkWidget *widget, BtkCTree *ctree)
{
  btk_ctree_collapse_recursive (ctree, NULL);
  after_press (ctree, NULL);
}

void select_all (BtkWidget *widget, BtkCTree *ctree)
{
  btk_ctree_select_recursive (ctree, NULL);
  after_press (ctree, NULL);
}

void change_style (BtkWidget *widget, BtkCTree *ctree)
{
  static BtkStyle *style1 = NULL;
  static BtkStyle *style2 = NULL;

  BtkCTreeNode *node;
  BdkColor green_col = { 0, 0, 56000, 0};
  BdkColor purple_col = { 0, 32000, 0, 56000};

  if (BTK_CLIST (ctree)->focus_row >= 0)
    node = BTK_CTREE_NODE
      (g_list_nth (BTK_CLIST (ctree)->row_list,BTK_CLIST (ctree)->focus_row));
  else
    node = BTK_CTREE_NODE (BTK_CLIST (ctree)->row_list);

  if (!node)
    return;

  if (!style1)
    {
      style1 = btk_style_new ();
      style1->base[BTK_STATE_NORMAL] = green_col;
      style1->fg[BTK_STATE_SELECTED] = purple_col;

      style2 = btk_style_new ();
      style2->base[BTK_STATE_SELECTED] = purple_col;
      style2->fg[BTK_STATE_NORMAL] = green_col;
      style2->base[BTK_STATE_NORMAL] = purple_col;
      bango_font_description_free (style2->font_desc);
      style2->font_desc = bango_font_description_from_string ("courier 30");
    }

  btk_ctree_node_set_cell_style (ctree, node, 1, style1);
  btk_ctree_node_set_cell_style (ctree, node, 0, style2);

  if (BTK_CTREE_ROW (node)->children)
    btk_ctree_node_set_row_style (ctree, BTK_CTREE_ROW (node)->children,
				  style2);
}

void unselect_all (BtkWidget *widget, BtkCTree *ctree)
{
  btk_ctree_unselect_recursive (ctree, NULL);
  after_press (ctree, NULL);
}

void remove_selection (BtkWidget *widget, BtkCTree *ctree)
{
  BtkCList *clist;
  BtkCTreeNode *node;

  clist = BTK_CLIST (ctree);

  btk_clist_freeze (clist);

  while (clist->selection)
    {
      node = clist->selection->data;

      if (BTK_CTREE_ROW (node)->is_leaf)
	pages--;
      else
	btk_ctree_post_recursive (ctree, node,
				  (BtkCTreeFunc) count_items, NULL);

      btk_ctree_remove_node (ctree, node);

      if (clist->selection_mode == BTK_SELECTION_BROWSE)
	break;
    }

  if (clist->selection_mode == BTK_SELECTION_EXTENDED && !clist->selection &&
      clist->focus_row >= 0)
    {
      node = btk_ctree_node_nth (ctree, clist->focus_row);

      if (node)
	btk_ctree_select (ctree, node);
    }
    
  btk_clist_thaw (clist);
  after_press (ctree, NULL);
}

struct _ExportStruct {
  gchar *tree;
  gchar *info;
  gboolean is_leaf;
};

typedef struct _ExportStruct ExportStruct;

gboolean
gnode2ctree (BtkCTree   *ctree,
	     guint       depth,
	     GNode        *gnode,
	     BtkCTreeNode *cnode,
	     gpointer    data)
{
  ExportStruct *es;
  BdkPixmap *pixmap_closed;
  BdkBitmap *mask_closed;
  BdkPixmap *pixmap_opened;
  BdkBitmap *mask_opened;
  CTreePixmaps *pixmaps;

  if (!cnode || !gnode || (!(es = gnode->data)))
    return FALSE;

  pixmaps = get_ctree_pixmaps (ctree);

  if (es->is_leaf)
    {
      pixmap_closed = pixmaps->pixmap3;
      mask_closed = pixmaps->mask3;
      pixmap_opened = NULL;
      mask_opened = NULL;
    }
  else
    {
      pixmap_closed = pixmaps->pixmap1;
      mask_closed = pixmaps->mask1;
      pixmap_opened = pixmaps->pixmap2;
      mask_opened = pixmaps->mask2;
    }

  btk_ctree_set_node_info (ctree, cnode, es->tree, 2, pixmap_closed,
			   mask_closed, pixmap_opened, mask_opened,
			   es->is_leaf, (depth < 3));
  btk_ctree_node_set_text (ctree, cnode, 1, es->info);
  g_free (es);
  gnode->data = NULL;

  return TRUE;
}

gboolean
ctree2gnode (BtkCTree   *ctree,
	     guint       depth,
	     GNode        *gnode,
	     BtkCTreeNode *cnode,
	     gpointer    data)
{
  ExportStruct *es;

  if (!cnode || !gnode)
    return FALSE;
  
  es = g_new (ExportStruct, 1);
  gnode->data = es;
  es->is_leaf = BTK_CTREE_ROW (cnode)->is_leaf;
  es->tree = BTK_CELL_PIXTEXT (BTK_CTREE_ROW (cnode)->row.cell[0])->text;
  es->info = BTK_CELL_PIXTEXT (BTK_CTREE_ROW (cnode)->row.cell[1])->text;
  return TRUE;
}

void export_ctree (BtkWidget *widget, BtkCTree *ctree)
{
  char *title[] = { "Tree" , "Info" };
  static BtkWidget *export_window = NULL;
  static BtkCTree *export_ctree;
  BtkWidget *vbox;
  BtkWidget *scrolled_win;
  BtkWidget *button;
  BtkWidget *sep;
  GNode *gnode;
  BtkCTreeNode *node;

  if (!export_window)
    {
      export_window = btk_window_new (BTK_WINDOW_TOPLEVEL);

      btk_window_set_screen (BTK_WINDOW (export_window),
			     btk_widget_get_screen (widget));
  
      g_signal_connect (export_window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&export_window);

      btk_window_set_title (BTK_WINDOW (export_window), "exported ctree");
      btk_container_set_border_width (BTK_CONTAINER (export_window), 5);

      vbox = btk_vbox_new (FALSE, 0);
      btk_container_add (BTK_CONTAINER (export_window), vbox);
      
      button = btk_button_new_with_label ("Close");
      btk_box_pack_end (BTK_BOX (vbox), button, FALSE, TRUE, 0);

      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (btk_widget_destroy),
				export_window);

      sep = btk_hseparator_new ();
      btk_box_pack_end (BTK_BOX (vbox), sep, FALSE, TRUE, 10);

      export_ctree = BTK_CTREE (btk_ctree_new_with_titles (2, 0, title));
      btk_ctree_set_line_style (export_ctree, BTK_CTREE_LINES_DOTTED);

      scrolled_win = btk_scrolled_window_new (NULL, NULL);
      btk_container_add (BTK_CONTAINER (scrolled_win),
			 BTK_WIDGET (export_ctree));
      btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (scrolled_win),
				      BTK_POLICY_AUTOMATIC,
				      BTK_POLICY_AUTOMATIC);
      btk_box_pack_start (BTK_BOX (vbox), scrolled_win, TRUE, TRUE, 0);
      btk_clist_set_selection_mode (BTK_CLIST (export_ctree),
				    BTK_SELECTION_EXTENDED);
      btk_clist_set_column_width (BTK_CLIST (export_ctree), 0, 200);
      btk_clist_set_column_width (BTK_CLIST (export_ctree), 1, 200);
      btk_widget_set_size_request (BTK_WIDGET (export_ctree), 300, 200);
    }

  if (!btk_widget_get_visible (export_window))
    btk_widget_show_all (export_window);
      
  btk_clist_clear (BTK_CLIST (export_ctree));

  node = BTK_CTREE_NODE (g_list_nth (BTK_CLIST (ctree)->row_list,
				     BTK_CLIST (ctree)->focus_row));
  if (!node)
    return;

  gnode = btk_ctree_export_to_gnode (ctree, NULL, NULL, node,
				     ctree2gnode, NULL);
  if (gnode)
    {
      btk_ctree_insert_gnode (export_ctree, NULL, NULL, gnode,
			      gnode2ctree, NULL);
      g_node_destroy (gnode);
    }
}

void change_indent (BtkWidget *widget, BtkCTree *ctree)
{
  btk_ctree_set_indent (ctree, BTK_ADJUSTMENT (widget)->value);
}

void change_spacing (BtkWidget *widget, BtkCTree *ctree)
{
  btk_ctree_set_spacing (ctree, BTK_ADJUSTMENT (widget)->value);
}

void change_row_height (BtkWidget *widget, BtkCList *clist)
{
  btk_clist_set_row_height (clist, BTK_ADJUSTMENT (widget)->value);
}

void set_background (BtkCTree *ctree, BtkCTreeNode *node, gpointer data)
{
  BtkStyle *style = NULL;
  
  if (!node)
    return;
  
  if (ctree->line_style != BTK_CTREE_LINES_TABBED)
    {
      if (!BTK_CTREE_ROW (node)->is_leaf)
	style = BTK_CTREE_ROW (node)->row.data;
      else if (BTK_CTREE_ROW (node)->parent)
	style = BTK_CTREE_ROW (BTK_CTREE_ROW (node)->parent)->row.data;
    }

  btk_ctree_node_set_row_style (ctree, node, style);
}

void 
ctree_toggle_line_style (BtkWidget *widget, gpointer data)
{
  BtkCTree *ctree;
  gint i;

  ctree = BTK_CTREE (data);

  if (!btk_widget_get_mapped (widget))
    return;

  i = btk_option_menu_get_history (BTK_OPTION_MENU (widget));

  if ((ctree->line_style == BTK_CTREE_LINES_TABBED && 
       ((BtkCTreeLineStyle) i) != BTK_CTREE_LINES_TABBED) ||
      (ctree->line_style != BTK_CTREE_LINES_TABBED && 
       ((BtkCTreeLineStyle) i) == BTK_CTREE_LINES_TABBED))
    btk_ctree_pre_recursive (ctree, NULL, set_background, NULL);
  btk_ctree_set_line_style (ctree, i);
  line_style = i;
}

void 
ctree_toggle_expander_style (BtkWidget *widget, gpointer data)
{
  BtkCTree *ctree;
  gint i;

  ctree = BTK_CTREE (data);

  if (!btk_widget_get_mapped (widget))
    return;
  
  i = btk_option_menu_get_history (BTK_OPTION_MENU (widget));
  
  btk_ctree_set_expander_style (ctree, (BtkCTreeExpanderStyle) i);
}

void 
ctree_toggle_justify (BtkWidget *widget, gpointer data)
{
  BtkCTree *ctree;
  gint i;

  ctree = BTK_CTREE (data);

  if (!btk_widget_get_mapped (widget))
    return;

  i = btk_option_menu_get_history (BTK_OPTION_MENU (widget));

  btk_clist_set_column_justification (BTK_CLIST (ctree), ctree->tree_column, 
				      (BtkJustification) i);
}

void 
ctree_toggle_sel_mode (BtkWidget *widget, gpointer data)
{
  BtkCTree *ctree;
  gint i;

  ctree = BTK_CTREE (data);

  if (!btk_widget_get_mapped (widget))
    return;

  i = btk_option_menu_get_history (BTK_OPTION_MENU (widget));

  btk_clist_set_selection_mode (BTK_CLIST (ctree), selection_modes[i]);
  after_press (ctree, NULL);
}
    
void build_recursive (BtkCTree *ctree, gint cur_depth, gint depth, 
		      gint num_books, gint num_pages, BtkCTreeNode *parent)
{
  gchar *text[2];
  gchar buf1[60];
  gchar buf2[60];
  BtkCTreeNode *sibling;
  CTreePixmaps *pixmaps;
  gint i;

  text[0] = buf1;
  text[1] = buf2;
  sibling = NULL;

  pixmaps = get_ctree_pixmaps (ctree);

  for (i = num_pages + num_books; i > num_books; i--)
    {
      pages++;
      sprintf (buf1, "Page %02d", (gint) rand() % 100);
      sprintf (buf2, "Item %d-%d", cur_depth, i);
      sibling = btk_ctree_insert_node (ctree, parent, sibling, text, 5,
				       pixmaps->pixmap3, pixmaps->mask3, NULL, NULL,
				       TRUE, FALSE);

      if (parent && ctree->line_style == BTK_CTREE_LINES_TABBED)
	btk_ctree_node_set_row_style (ctree, sibling,
				      BTK_CTREE_ROW (parent)->row.style);
    }

  if (cur_depth == depth)
    return;

  for (i = num_books; i > 0; i--)
    {
      BtkStyle *style;

      books++;
      sprintf (buf1, "Book %02d", (gint) rand() % 100);
      sprintf (buf2, "Item %d-%d", cur_depth, i);
      sibling = btk_ctree_insert_node (ctree, parent, sibling, text, 5,
				       pixmaps->pixmap1, pixmaps->mask1, pixmaps->pixmap2, pixmaps->mask2,
				       FALSE, FALSE);

      style = btk_style_new ();
      switch (cur_depth % 3)
	{
	case 0:
	  style->base[BTK_STATE_NORMAL].red   = 10000 * (cur_depth % 6);
	  style->base[BTK_STATE_NORMAL].green = 0;
	  style->base[BTK_STATE_NORMAL].blue  = 65535 - ((i * 10000) % 65535);
	  break;
	case 1:
	  style->base[BTK_STATE_NORMAL].red   = 10000 * (cur_depth % 6);
	  style->base[BTK_STATE_NORMAL].green = 65535 - ((i * 10000) % 65535);
	  style->base[BTK_STATE_NORMAL].blue  = 0;
	  break;
	default:
	  style->base[BTK_STATE_NORMAL].red   = 65535 - ((i * 10000) % 65535);
	  style->base[BTK_STATE_NORMAL].green = 0;
	  style->base[BTK_STATE_NORMAL].blue  = 10000 * (cur_depth % 6);
	  break;
	}
      btk_ctree_node_set_row_data_full (ctree, sibling, style,
					(GDestroyNotify) g_object_unref);

      if (ctree->line_style == BTK_CTREE_LINES_TABBED)
	btk_ctree_node_set_row_style (ctree, sibling, style);

      build_recursive (ctree, cur_depth + 1, depth, num_books, num_pages,
		       sibling);
    }
}

void rebuild_tree (BtkWidget *widget, BtkCTree *ctree)
{
  gchar *text [2];
  gchar label1[] = "Root";
  gchar label2[] = "";
  BtkCTreeNode *parent;
  BtkStyle *style;
  guint b, d, p, n;
  CTreePixmaps *pixmaps;

  pixmaps = get_ctree_pixmaps (ctree);

  text[0] = label1;
  text[1] = label2;
  
  d = btk_spin_button_get_value_as_int (BTK_SPIN_BUTTON (spin1)); 
  b = btk_spin_button_get_value_as_int (BTK_SPIN_BUTTON (spin2));
  p = btk_spin_button_get_value_as_int (BTK_SPIN_BUTTON (spin3));

  n = ((pow (b, d) - 1) / (b - 1)) * (p + 1);

  if (n > 100000)
    {
      g_print ("%d total items? Try less\n",n);
      return;
    }

  btk_clist_freeze (BTK_CLIST (ctree));
  btk_clist_clear (BTK_CLIST (ctree));

  books = 1;
  pages = 0;

  parent = btk_ctree_insert_node (ctree, NULL, NULL, text, 5, pixmaps->pixmap1,
				  pixmaps->mask1, pixmaps->pixmap2, pixmaps->mask2, FALSE, TRUE);

  style = btk_style_new ();
  style->base[BTK_STATE_NORMAL].red   = 0;
  style->base[BTK_STATE_NORMAL].green = 45000;
  style->base[BTK_STATE_NORMAL].blue  = 55000;
  btk_ctree_node_set_row_data_full (ctree, parent, style,
				    (GDestroyNotify) g_object_unref);

  if (ctree->line_style == BTK_CTREE_LINES_TABBED)
    btk_ctree_node_set_row_style (ctree, parent, style);

  build_recursive (ctree, 1, d, b, p, parent);
  btk_clist_thaw (BTK_CLIST (ctree));
  after_press (ctree, NULL);
}

static void 
ctree_click_column (BtkCTree *ctree, gint column, gpointer data)
{
  BtkCList *clist;

  clist = BTK_CLIST (ctree);

  if (column == clist->sort_column)
    {
      if (clist->sort_type == BTK_SORT_ASCENDING)
	clist->sort_type = BTK_SORT_DESCENDING;
      else
	clist->sort_type = BTK_SORT_ASCENDING;
    }
  else
    btk_clist_set_sort_column (clist, column);

  btk_ctree_sort_recursive (ctree, NULL);
}

void create_ctree (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  BtkTooltips *tooltips;
  BtkCTree *ctree;
  BtkWidget *scrolled_win;
  BtkWidget *vbox;
  BtkWidget *bbox;
  BtkWidget *mbox;
  BtkWidget *hbox;
  BtkWidget *hbox2;
  BtkWidget *frame;
  BtkWidget *label;
  BtkWidget *button;
  BtkWidget *check;
  BtkAdjustment *adj;
  BtkWidget *spinner;

  char *title[] = { "Tree" , "Info" };
  char buf[80];

  static gchar *items1[] =
  {
    "No lines",
    "Solid",
    "Dotted",
    "Tabbed"
  };

  static gchar *items2[] =
  {
    "None",
    "Square",
    "Triangle",
    "Circular"
  };

  static gchar *items3[] =
  {
    "Left",
    "Right"
  };
  
  if (!window)
    {
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_screen (BTK_WINDOW (window), 
			     btk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window);

      btk_window_set_title (BTK_WINDOW (window), "BtkCTree");
      btk_container_set_border_width (BTK_CONTAINER (window), 0);

      tooltips = btk_tooltips_new ();
      g_object_ref (tooltips);
      btk_object_sink (BTK_OBJECT (tooltips));

      g_object_set_data_full (B_OBJECT (window), "tooltips", tooltips,
			      g_object_unref);

      vbox = btk_vbox_new (FALSE, 0);
      btk_container_add (BTK_CONTAINER (window), vbox);

      hbox = btk_hbox_new (FALSE, 5);
      btk_container_set_border_width (BTK_CONTAINER (hbox), 5);
      btk_box_pack_start (BTK_BOX (vbox), hbox, FALSE, TRUE, 0);
      
      label = btk_label_new ("Depth :");
      btk_box_pack_start (BTK_BOX (hbox), label, FALSE, TRUE, 0);
      
      adj = (BtkAdjustment *) btk_adjustment_new (4, 1, 10, 1, 5, 0);
      spin1 = btk_spin_button_new (adj, 0, 0);
      btk_box_pack_start (BTK_BOX (hbox), spin1, FALSE, TRUE, 5);
  
      label = btk_label_new ("Books :");
      btk_box_pack_start (BTK_BOX (hbox), label, FALSE, TRUE, 0);
      
      adj = (BtkAdjustment *) btk_adjustment_new (3, 1, 20, 1, 5, 0);
      spin2 = btk_spin_button_new (adj, 0, 0);
      btk_box_pack_start (BTK_BOX (hbox), spin2, FALSE, TRUE, 5);

      label = btk_label_new ("Pages :");
      btk_box_pack_start (BTK_BOX (hbox), label, FALSE, TRUE, 0);
      
      adj = (BtkAdjustment *) btk_adjustment_new (5, 1, 20, 1, 5, 0);
      spin3 = btk_spin_button_new (adj, 0, 0);
      btk_box_pack_start (BTK_BOX (hbox), spin3, FALSE, TRUE, 5);

      button = btk_button_new_with_label ("Close");
      btk_box_pack_end (BTK_BOX (hbox), button, TRUE, TRUE, 0);

      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (btk_widget_destroy),
				window);

      button = btk_button_new_with_label ("Rebuild Tree");
      btk_box_pack_start (BTK_BOX (hbox), button, TRUE, TRUE, 0);

      scrolled_win = btk_scrolled_window_new (NULL, NULL);
      btk_container_set_border_width (BTK_CONTAINER (scrolled_win), 5);
      btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (scrolled_win),
				      BTK_POLICY_AUTOMATIC,
				      BTK_POLICY_ALWAYS);
      btk_box_pack_start (BTK_BOX (vbox), scrolled_win, TRUE, TRUE, 0);

      ctree = BTK_CTREE (btk_ctree_new_with_titles (2, 0, title));
      btk_container_add (BTK_CONTAINER (scrolled_win), BTK_WIDGET (ctree));

      btk_clist_set_column_auto_resize (BTK_CLIST (ctree), 0, TRUE);
      btk_clist_set_column_width (BTK_CLIST (ctree), 1, 200);
      btk_clist_set_selection_mode (BTK_CLIST (ctree), BTK_SELECTION_EXTENDED);
      btk_ctree_set_line_style (ctree, BTK_CTREE_LINES_DOTTED);
      line_style = BTK_CTREE_LINES_DOTTED;

      g_signal_connect (button, "clicked",
			G_CALLBACK (rebuild_tree), ctree);
      g_signal_connect (ctree, "click_column",
			G_CALLBACK (ctree_click_column), NULL);

      g_signal_connect_after (ctree, "button_press_event",
			      G_CALLBACK (after_press), NULL);
      g_signal_connect_after (ctree, "button_release_event",
			      G_CALLBACK (after_press), NULL);
      g_signal_connect_after (ctree, "tree_move",
			      G_CALLBACK (after_move), NULL);
      g_signal_connect_after (ctree, "end_selection",
			      G_CALLBACK (after_press), NULL);
      g_signal_connect_after (ctree, "toggle_focus_row",
			      G_CALLBACK (after_press), NULL);
      g_signal_connect_after (ctree, "select_all",
			      G_CALLBACK (after_press), NULL);
      g_signal_connect_after (ctree, "unselect_all",
			      G_CALLBACK (after_press), NULL);
      g_signal_connect_after (ctree, "scroll_vertical",
			      G_CALLBACK (after_press), NULL);

      bbox = btk_hbox_new (FALSE, 5);
      btk_container_set_border_width (BTK_CONTAINER (bbox), 5);
      btk_box_pack_start (BTK_BOX (vbox), bbox, FALSE, TRUE, 0);

      mbox = btk_vbox_new (TRUE, 5);
      btk_box_pack_start (BTK_BOX (bbox), mbox, FALSE, TRUE, 0);

      label = btk_label_new ("Row Height :");
      btk_box_pack_start (BTK_BOX (mbox), label, FALSE, FALSE, 0);

      label = btk_label_new ("Indent :");
      btk_box_pack_start (BTK_BOX (mbox), label, FALSE, FALSE, 0);

      label = btk_label_new ("Spacing :");
      btk_box_pack_start (BTK_BOX (mbox), label, FALSE, FALSE, 0);

      mbox = btk_vbox_new (TRUE, 5);
      btk_box_pack_start (BTK_BOX (bbox), mbox, FALSE, TRUE, 0);

      adj = (BtkAdjustment *) btk_adjustment_new (20, 12, 100, 1, 10, 0);
      spinner = btk_spin_button_new (adj, 0, 0);
      btk_box_pack_start (BTK_BOX (mbox), spinner, FALSE, FALSE, 5);
      btk_tooltips_set_tip (tooltips, spinner,
			    "Row height of list items", NULL);
      g_signal_connect (adj, "value_changed",
			G_CALLBACK (change_row_height), ctree);
      btk_clist_set_row_height ( BTK_CLIST (ctree), adj->value);

      adj = (BtkAdjustment *) btk_adjustment_new (20, 0, 60, 1, 10, 0);
      spinner = btk_spin_button_new (adj, 0, 0);
      btk_box_pack_start (BTK_BOX (mbox), spinner, FALSE, FALSE, 5);
      btk_tooltips_set_tip (tooltips, spinner, "Tree Indentation.", NULL);
      g_signal_connect (adj, "value_changed",
			G_CALLBACK (change_indent), ctree);

      adj = (BtkAdjustment *) btk_adjustment_new (5, 0, 60, 1, 10, 0);
      spinner = btk_spin_button_new (adj, 0, 0);
      btk_box_pack_start (BTK_BOX (mbox), spinner, FALSE, FALSE, 5);
      btk_tooltips_set_tip (tooltips, spinner, "Tree Spacing.", NULL);
      g_signal_connect (adj, "value_changed",
			G_CALLBACK (change_spacing), ctree);

      mbox = btk_vbox_new (TRUE, 5);
      btk_box_pack_start (BTK_BOX (bbox), mbox, FALSE, TRUE, 0);

      hbox = btk_hbox_new (FALSE, 5);
      btk_box_pack_start (BTK_BOX (mbox), hbox, FALSE, FALSE, 0);

      button = btk_button_new_with_label ("Expand All");
      btk_box_pack_start (BTK_BOX (hbox), button, TRUE, TRUE, 0);
      g_signal_connect (button, "clicked",
			G_CALLBACK (expand_all), ctree);

      button = btk_button_new_with_label ("Collapse All");
      btk_box_pack_start (BTK_BOX (hbox), button, TRUE, TRUE, 0);
      g_signal_connect (button, "clicked",
			G_CALLBACK (collapse_all), ctree);

      button = btk_button_new_with_label ("Change Style");
      btk_box_pack_start (BTK_BOX (hbox), button, TRUE, TRUE, 0);
      g_signal_connect (button, "clicked",
			G_CALLBACK (change_style), ctree);

      button = btk_button_new_with_label ("Export Tree");
      btk_box_pack_start (BTK_BOX (hbox), button, TRUE, TRUE, 0);
      g_signal_connect (button, "clicked",
			G_CALLBACK (export_ctree), ctree);

      hbox = btk_hbox_new (FALSE, 5);
      btk_box_pack_start (BTK_BOX (mbox), hbox, FALSE, FALSE, 0);

      button = btk_button_new_with_label ("Select All");
      btk_box_pack_start (BTK_BOX (hbox), button, TRUE, TRUE, 0);
      g_signal_connect (button, "clicked",
			G_CALLBACK (select_all), ctree);

      button = btk_button_new_with_label ("Unselect All");
      btk_box_pack_start (BTK_BOX (hbox), button, TRUE, TRUE, 0);
      g_signal_connect (button, "clicked",
			G_CALLBACK (unselect_all), ctree);

      button = btk_button_new_with_label ("Remove Selection");
      btk_box_pack_start (BTK_BOX (hbox), button, TRUE, TRUE, 0);
      g_signal_connect (button, "clicked",
			G_CALLBACK (remove_selection), ctree);

      check = btk_check_button_new_with_label ("Reorderable");
      btk_box_pack_start (BTK_BOX (hbox), check, FALSE, TRUE, 0);
      btk_tooltips_set_tip (tooltips, check,
			    "Tree items can be reordered by dragging.", NULL);
      g_signal_connect (check, "clicked",
			G_CALLBACK (toggle_reorderable), ctree);
      btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (check), TRUE);

      hbox = btk_hbox_new (TRUE, 5);
      btk_box_pack_start (BTK_BOX (mbox), hbox, FALSE, FALSE, 0);

      omenu1 = build_option_menu (items1, 4, 2, 
				  ctree_toggle_line_style,
				  ctree);
      btk_box_pack_start (BTK_BOX (hbox), omenu1, FALSE, TRUE, 0);
      btk_tooltips_set_tip (tooltips, omenu1, "The tree's line style.", NULL);

      omenu2 = build_option_menu (items2, 4, 1, 
				  ctree_toggle_expander_style,
				  ctree);
      btk_box_pack_start (BTK_BOX (hbox), omenu2, FALSE, TRUE, 0);
      btk_tooltips_set_tip (tooltips, omenu2, "The tree's expander style.",
			    NULL);

      omenu3 = build_option_menu (items3, 2, 0, 
				  ctree_toggle_justify, ctree);
      btk_box_pack_start (BTK_BOX (hbox), omenu3, FALSE, TRUE, 0);
      btk_tooltips_set_tip (tooltips, omenu3, "The tree's justification.",
			    NULL);

      omenu4 = build_option_menu (selection_mode_items, 3, 3, 
				  ctree_toggle_sel_mode, ctree);
      btk_box_pack_start (BTK_BOX (hbox), omenu4, FALSE, TRUE, 0);
      btk_tooltips_set_tip (tooltips, omenu4, "The list's selection mode.",
			    NULL);

      btk_widget_realize (window);
      
      btk_widget_set_size_request (BTK_WIDGET (ctree), -1, 300);

      frame = btk_frame_new (NULL);
      btk_container_set_border_width (BTK_CONTAINER (frame), 0);
      btk_frame_set_shadow_type (BTK_FRAME (frame), BTK_SHADOW_OUT);
      btk_box_pack_start (BTK_BOX (vbox), frame, FALSE, TRUE, 0);

      hbox = btk_hbox_new (TRUE, 2);
      btk_container_set_border_width (BTK_CONTAINER (hbox), 2);
      btk_container_add (BTK_CONTAINER (frame), hbox);

      frame = btk_frame_new (NULL);
      btk_frame_set_shadow_type (BTK_FRAME (frame), BTK_SHADOW_IN);
      btk_box_pack_start (BTK_BOX (hbox), frame, FALSE, TRUE, 0);

      hbox2 = btk_hbox_new (FALSE, 0);
      btk_container_set_border_width (BTK_CONTAINER (hbox2), 2);
      btk_container_add (BTK_CONTAINER (frame), hbox2);

      label = btk_label_new ("Books :");
      btk_box_pack_start (BTK_BOX (hbox2), label, FALSE, TRUE, 0);

      sprintf (buf, "%d", books);
      book_label = btk_label_new (buf);
      btk_box_pack_end (BTK_BOX (hbox2), book_label, FALSE, TRUE, 5);

      frame = btk_frame_new (NULL);
      btk_frame_set_shadow_type (BTK_FRAME (frame), BTK_SHADOW_IN);
      btk_box_pack_start (BTK_BOX (hbox), frame, FALSE, TRUE, 0);

      hbox2 = btk_hbox_new (FALSE, 0);
      btk_container_set_border_width (BTK_CONTAINER (hbox2), 2);
      btk_container_add (BTK_CONTAINER (frame), hbox2);

      label = btk_label_new ("Pages :");
      btk_box_pack_start (BTK_BOX (hbox2), label, FALSE, TRUE, 0);

      sprintf (buf, "%d", pages);
      page_label = btk_label_new (buf);
      btk_box_pack_end (BTK_BOX (hbox2), page_label, FALSE, TRUE, 5);

      frame = btk_frame_new (NULL);
      btk_frame_set_shadow_type (BTK_FRAME (frame), BTK_SHADOW_IN);
      btk_box_pack_start (BTK_BOX (hbox), frame, FALSE, TRUE, 0);

      hbox2 = btk_hbox_new (FALSE, 0);
      btk_container_set_border_width (BTK_CONTAINER (hbox2), 2);
      btk_container_add (BTK_CONTAINER (frame), hbox2);

      label = btk_label_new ("Selected :");
      btk_box_pack_start (BTK_BOX (hbox2), label, FALSE, TRUE, 0);

      sprintf (buf, "%d", g_list_length (BTK_CLIST (ctree)->selection));
      sel_label = btk_label_new (buf);
      btk_box_pack_end (BTK_BOX (hbox2), sel_label, FALSE, TRUE, 5);

      frame = btk_frame_new (NULL);
      btk_frame_set_shadow_type (BTK_FRAME (frame), BTK_SHADOW_IN);
      btk_box_pack_start (BTK_BOX (hbox), frame, FALSE, TRUE, 0);

      hbox2 = btk_hbox_new (FALSE, 0);
      btk_container_set_border_width (BTK_CONTAINER (hbox2), 2);
      btk_container_add (BTK_CONTAINER (frame), hbox2);

      label = btk_label_new ("Visible :");
      btk_box_pack_start (BTK_BOX (hbox2), label, FALSE, TRUE, 0);

      sprintf (buf, "%d", g_list_length (BTK_CLIST (ctree)->row_list));
      vis_label = btk_label_new (buf);
      btk_box_pack_end (BTK_BOX (hbox2), vis_label, FALSE, TRUE, 5);

      rebuild_tree (NULL, ctree);
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    btk_widget_destroy (window);
}

/*
 * BtkColorSelection
 */

void
color_selection_ok (BtkWidget               *w,
                    BtkColorSelectionDialog *cs)
{
  BtkColorSelection *colorsel;
  gdouble color[4];

  colorsel=BTK_COLOR_SELECTION(cs->colorsel);

  btk_color_selection_get_color(colorsel,color);
  btk_color_selection_set_color(colorsel,color);
}

void
color_selection_changed (BtkWidget *w,
                         BtkColorSelectionDialog *cs)
{
  BtkColorSelection *colorsel;
  gdouble color[4];

  colorsel=BTK_COLOR_SELECTION(cs->colorsel);
  btk_color_selection_get_color(colorsel,color);
}

#if 0 /* unused */
static void
opacity_toggled_cb (BtkWidget *w,
		    BtkColorSelectionDialog *cs)
{
  BtkColorSelection *colorsel;

  colorsel = BTK_COLOR_SELECTION (cs->colorsel);
  btk_color_selection_set_has_opacity_control (colorsel,
					       btk_toggle_button_get_active (BTK_TOGGLE_BUTTON (w)));
}

static void
palette_toggled_cb (BtkWidget *w,
		    BtkColorSelectionDialog *cs)
{
  BtkColorSelection *colorsel;

  colorsel = BTK_COLOR_SELECTION (cs->colorsel);
  btk_color_selection_set_has_palette (colorsel,
				       btk_toggle_button_get_active (BTK_TOGGLE_BUTTON (w)));
}
#endif

void
create_color_selection (BtkWidget *widget)
{
  static BtkWidget *window = NULL;

  if (!window)
    {
      BtkWidget *picker;
      BtkWidget *hbox;
      BtkWidget *label;
      BtkWidget *button;
      
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_screen (BTK_WINDOW (window), 
			     btk_widget_get_screen (widget));
			     
      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
                        &window);

      btk_window_set_title (BTK_WINDOW (window), "BtkColorButton");
      btk_container_set_border_width (BTK_CONTAINER (window), 0);

      hbox = btk_hbox_new (FALSE, 8);
      btk_container_set_border_width (BTK_CONTAINER (hbox), 8);
      btk_container_add (BTK_CONTAINER (window), hbox);
      
      label = btk_label_new ("Pick a color");
      btk_container_add (BTK_CONTAINER (hbox), label);

      picker = btk_color_button_new ();
      btk_color_button_set_use_alpha (BTK_COLOR_BUTTON (picker), TRUE);
      btk_container_add (BTK_CONTAINER (hbox), picker);

      button = btk_button_new_with_mnemonic ("_Props");
      btk_box_pack_start (BTK_BOX (hbox), button, FALSE, FALSE, 0);
      g_signal_connect (button, "clicked",
			G_CALLBACK (props_clicked),
			picker);
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    btk_widget_destroy (window);
}

/*
 * BtkFileSelection
 */

void
show_fileops (BtkWidget        *widget,
	      BtkFileSelection *fs)
{
  gboolean show_ops;

  show_ops = btk_toggle_button_get_active (BTK_TOGGLE_BUTTON (widget));

  if (show_ops)
    btk_file_selection_show_fileop_buttons (fs);
  else
    btk_file_selection_hide_fileop_buttons (fs);
}

void
select_multiple (BtkWidget        *widget,
		 BtkFileSelection *fs)
{
  gboolean select_multiple;

  select_multiple = btk_toggle_button_get_active (BTK_TOGGLE_BUTTON (widget));
  btk_file_selection_set_select_multiple (fs, select_multiple);
}

void
file_selection_ok (BtkFileSelection *fs)
{
  int i;
  gchar **selections;

  selections = btk_file_selection_get_selections (fs);

  for (i = 0; selections[i] != NULL; i++)
    g_print ("%s\n", selections[i]);

  g_strfreev (selections);

  btk_widget_destroy (BTK_WIDGET (fs));
}

void
create_file_selection (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  BtkWidget *button;

  if (!window)
    {
      window = btk_file_selection_new ("file selection dialog");
      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));

      btk_file_selection_hide_fileop_buttons (BTK_FILE_SELECTION (window));

      btk_window_set_position (BTK_WINDOW (window), BTK_WIN_POS_MOUSE);

      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window);

      g_signal_connect_swapped (BTK_FILE_SELECTION (window)->ok_button,
				"clicked",
				G_CALLBACK (file_selection_ok),
				window);
      g_signal_connect_swapped (BTK_FILE_SELECTION (window)->cancel_button,
			        "clicked",
				G_CALLBACK (btk_widget_destroy),
				window);
      
      button = btk_check_button_new_with_label ("Show Fileops");
      g_signal_connect (button, "toggled",
			G_CALLBACK (show_fileops),
			window);
      btk_box_pack_start (BTK_BOX (BTK_FILE_SELECTION (window)->action_area), 
			  button, FALSE, FALSE, 0);
      btk_widget_show (button);

      button = btk_check_button_new_with_label ("Select Multiple");
      g_signal_connect (button, "clicked",
			G_CALLBACK (select_multiple),
			window);
      btk_box_pack_start (BTK_BOX (BTK_FILE_SELECTION (window)->action_area), 
			  button, FALSE, FALSE, 0);
      btk_widget_show (button);
    }
  
  if (!btk_widget_get_visible (window))
    btk_widget_show (window);
  else
    btk_widget_destroy (window);
}

void
flipping_toggled_cb (BtkWidget *widget, gpointer data)
{
  int state = btk_toggle_button_get_active (BTK_TOGGLE_BUTTON (widget));
  int new_direction = state ? BTK_TEXT_DIR_RTL : BTK_TEXT_DIR_LTR;

  btk_widget_set_default_direction (new_direction);
}

static void
orientable_toggle_orientation (BtkOrientable *orientable)
{
  BtkOrientation orientation;

  orientation = btk_orientable_get_orientation (orientable);
  btk_orientable_set_orientation (orientable,
                                  orientation == BTK_ORIENTATION_HORIZONTAL ?
                                  BTK_ORIENTATION_VERTICAL :
                                  BTK_ORIENTATION_HORIZONTAL);

  if (BTK_IS_CONTAINER (orientable))
    {
      GList *children;
      GList *child;

      children = btk_container_get_children (BTK_CONTAINER (orientable));

      for (child = children; child; child = child->next)
        {
          if (BTK_IS_ORIENTABLE (child->data))
            orientable_toggle_orientation (child->data);
        }

      g_list_free (children);
    }
}

void
flipping_orientation_toggled_cb (BtkWidget *widget, gpointer data)
{
  orientable_toggle_orientation (BTK_ORIENTABLE (BTK_DIALOG (btk_widget_get_toplevel (widget))->vbox));
}

static void
set_direction_recurse (BtkWidget *widget,
		       gpointer   data)
{
  BtkTextDirection *dir = data;
  
  btk_widget_set_direction (widget, *dir);
  if (BTK_IS_CONTAINER (widget))
    btk_container_foreach (BTK_CONTAINER (widget),
			   set_direction_recurse,
			   data);
}

static BtkWidget *
create_forward_back (const char       *title,
		     BtkTextDirection  text_dir)
{
  BtkWidget *frame = btk_frame_new (title);
  BtkWidget *bbox = btk_hbutton_box_new ();
  BtkWidget *back_button = btk_button_new_from_stock (BTK_STOCK_GO_BACK);
  BtkWidget *forward_button = btk_button_new_from_stock (BTK_STOCK_GO_FORWARD);

  btk_container_set_border_width (BTK_CONTAINER (bbox), 5);
  
  btk_container_add (BTK_CONTAINER (frame), bbox);
  btk_container_add (BTK_CONTAINER (bbox), back_button);
  btk_container_add (BTK_CONTAINER (bbox), forward_button);

  set_direction_recurse (frame, &text_dir);

  return frame;
}

void
create_flipping (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  BtkWidget *check_button, *button;

  if (!window)
    {
      window = btk_dialog_new ();

      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window);

      btk_window_set_title (BTK_WINDOW (window), "Bidirectional Flipping");

      check_button = btk_check_button_new_with_label ("Right-to-left global direction");
      btk_container_set_border_width (BTK_CONTAINER (check_button), 10);
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->vbox),
			  check_button, TRUE, TRUE, 0);

      if (btk_widget_get_default_direction () == BTK_TEXT_DIR_RTL)
	btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (check_button), TRUE);

      g_signal_connect (check_button, "toggled",
			G_CALLBACK (flipping_toggled_cb), NULL);

      check_button = btk_check_button_new_with_label ("Toggle orientation of all boxes");
      btk_container_set_border_width (BTK_CONTAINER (check_button), 10);
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->vbox), 
			  check_button, TRUE, TRUE, 0);

      g_signal_connect (check_button, "toggled",
			G_CALLBACK (flipping_orientation_toggled_cb), NULL);

      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->vbox), 
			  create_forward_back ("Default", BTK_TEXT_DIR_NONE),
			  TRUE, TRUE, 0);

      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->vbox), 
			  create_forward_back ("Left-to-Right", BTK_TEXT_DIR_LTR),
			  TRUE, TRUE, 0);

      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->vbox), 
			  create_forward_back ("Right-to-Left", BTK_TEXT_DIR_RTL),
			  TRUE, TRUE, 0);

      button = btk_button_new_with_label ("Close");
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (btk_widget_destroy), window);
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->action_area), 
			  button, TRUE, TRUE, 0);
    }
  
  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    btk_widget_destroy (window);
}

/*
 * Focus test
 */

static BtkWidget*
make_focus_table (GList **list)
{
  BtkWidget *table;
  gint i, j;
  
  table = btk_table_new (5, 5, FALSE);

  i = 0;
  j = 0;

  while (i < 5)
    {
      j = 0;
      while (j < 5)
        {
          BtkWidget *widget;
          
          if ((i + j) % 2)
            widget = btk_entry_new ();
          else
            widget = btk_button_new_with_label ("Foo");

          *list = g_list_prepend (*list, widget);
          
          btk_table_attach (BTK_TABLE (table),
                            widget,
                            i, i + 1,
                            j, j + 1,
                            BTK_EXPAND | BTK_FILL,
                            BTK_EXPAND | BTK_FILL,
                            5, 5);
          
          ++j;
        }

      ++i;
    }

  *list = g_list_reverse (*list);
  
  return table;
}

static void
create_focus (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  
  if (!window)
    {
      BtkWidget *table;
      BtkWidget *frame;
      GList *list = NULL;
      
      window = btk_dialog_new_with_buttons ("Keyboard focus navigation",
                                            NULL, 0,
                                            BTK_STOCK_CLOSE,
                                            BTK_RESPONSE_NONE,
                                            NULL);

      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window);

      g_signal_connect (window, "response",
                        G_CALLBACK (btk_widget_destroy),
                        NULL);
      
      btk_window_set_title (BTK_WINDOW (window), "Keyboard Focus Navigation");

      frame = btk_frame_new ("Weird tab focus chain");

      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->vbox), 
			  frame, TRUE, TRUE, 0);
      
      table = make_focus_table (&list);

      btk_container_add (BTK_CONTAINER (frame), table);

      btk_container_set_focus_chain (BTK_CONTAINER (table),
                                     list);

      g_list_free (list);
      
      frame = btk_frame_new ("Default tab focus chain");

      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->vbox), 
			  frame, TRUE, TRUE, 0);

      list = NULL;
      table = make_focus_table (&list);

      g_list_free (list);
      
      btk_container_add (BTK_CONTAINER (frame), table);      
    }
  
  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    btk_widget_destroy (window);
}

/*
 * BtkFontSelection
 */

void
font_selection_ok (BtkWidget              *w,
		   BtkFontSelectionDialog *fs)
{
  gchar *s = btk_font_selection_dialog_get_font_name (fs);

  g_print ("%s\n", s);
  g_free (s);
  btk_widget_destroy (BTK_WIDGET (fs));
}

void
create_font_selection (BtkWidget *widget)
{
  static BtkWidget *window = NULL;

  if (!window)
    {
      BtkWidget *picker;
      BtkWidget *hbox;
      BtkWidget *label;
      
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window);

      btk_window_set_title (BTK_WINDOW (window), "BtkFontButton");
      btk_container_set_border_width (BTK_CONTAINER (window), 0);

      hbox = btk_hbox_new (FALSE, 8);
      btk_container_set_border_width (BTK_CONTAINER (hbox), 8);
      btk_container_add (BTK_CONTAINER (window), hbox);
      
      label = btk_label_new ("Pick a font");
      btk_container_add (BTK_CONTAINER (hbox), label);

      picker = btk_font_button_new ();
      btk_font_button_set_use_font (BTK_FONT_BUTTON (picker), TRUE);
      btk_container_add (BTK_CONTAINER (hbox), picker);
    }
  
  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    btk_widget_destroy (window);
}

/*
 * BtkDialog
 */

static BtkWidget *dialog_window = NULL;

static void
label_toggle (BtkWidget  *widget,
	      BtkWidget **label)
{
  if (!(*label))
    {
      *label = btk_label_new ("Dialog Test");
      g_signal_connect (*label,
			"destroy",
			G_CALLBACK (btk_widget_destroyed),
			label);
      btk_misc_set_padding (BTK_MISC (*label), 10, 10);
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (dialog_window)->vbox), 
			  *label, TRUE, TRUE, 0);
      btk_widget_show (*label);
    }
  else
    btk_widget_destroy (*label);
}

#define RESPONSE_TOGGLE_SEPARATOR 1

static void
print_response (BtkWidget *dialog,
                gint       response_id,
                gpointer   data)
{
  g_print ("response signal received (%d)\n", response_id);

  if (response_id == RESPONSE_TOGGLE_SEPARATOR)
    {
      btk_dialog_set_has_separator (BTK_DIALOG (dialog),
                                    !btk_dialog_get_has_separator (BTK_DIALOG (dialog)));
    }
}

static void
create_dialog (BtkWidget *widget)
{
  static BtkWidget *label;
  BtkWidget *button;

  if (!dialog_window)
    {
      /* This is a terrible example; it's much simpler to create
       * dialogs than this. Don't use testbtk for example code,
       * use btk-demo ;-)
       */
      
      dialog_window = btk_dialog_new ();
      btk_window_set_screen (BTK_WINDOW (dialog_window),
			     btk_widget_get_screen (widget));

      g_signal_connect (dialog_window,
                        "response",
                        G_CALLBACK (print_response),
                        NULL);
      
      g_signal_connect (dialog_window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&dialog_window);

      btk_window_set_title (BTK_WINDOW (dialog_window), "BtkDialog");
      btk_container_set_border_width (BTK_CONTAINER (dialog_window), 0);

      button = btk_button_new_with_label ("OK");
      btk_widget_set_can_default (button, TRUE);
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (dialog_window)->action_area), 
			  button, TRUE, TRUE, 0);
      btk_widget_grab_default (button);
      btk_widget_show (button);

      button = btk_button_new_with_label ("Toggle");
      g_signal_connect (button, "clicked",
			G_CALLBACK (label_toggle),
			&label);
      btk_widget_set_can_default (button, TRUE);
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (dialog_window)->action_area),
			  button, TRUE, TRUE, 0);
      btk_widget_show (button);

      label = NULL;
      
      button = btk_button_new_with_label ("Separator");

      btk_widget_set_can_default (button, TRUE);

      btk_dialog_add_action_widget (BTK_DIALOG (dialog_window),
                                    button,
                                    RESPONSE_TOGGLE_SEPARATOR);
      btk_widget_show (button);
    }

  if (!btk_widget_get_visible (dialog_window))
    btk_widget_show (dialog_window);
  else
    btk_widget_destroy (dialog_window);
}

/* Display & Screen test 
 */

typedef struct 
{ 
  BtkEntry *entry;
  BtkWidget *radio_dpy;
  BtkWidget *toplevel; 
  BtkWidget *dialog_window;
  BtkWidget *combo;
  GList *valid_display_list;
} ScreenDisplaySelection;

static void
screen_display_check (BtkWidget *widget, ScreenDisplaySelection *data)
{
  char *display_name;
  BdkDisplay *display = btk_widget_get_display (widget);
  BtkWidget *dialog;
  BdkScreen *new_screen = NULL;
  BdkScreen *current_screen = btk_widget_get_screen (widget);
  
  if (btk_toggle_button_get_active (BTK_TOGGLE_BUTTON (data->radio_dpy)))
    {
      display_name = g_strdup (btk_entry_get_text (data->entry));
      display = bdk_display_open (display_name);
      
      if (!display)
	{
	  dialog = btk_message_dialog_new (BTK_WINDOW (btk_widget_get_toplevel (widget)),
					   BTK_DIALOG_DESTROY_WITH_PARENT,
					   BTK_MESSAGE_ERROR,
					   BTK_BUTTONS_OK,
					   "The display :\n%s\ncannot be opened",
					   display_name);
	  btk_window_set_screen (BTK_WINDOW (dialog), current_screen);
	  btk_widget_show (dialog);
	  g_signal_connect (dialog, "response",
			    G_CALLBACK (btk_widget_destroy),
			    NULL);
	}
      else
        {
          BtkTreeModel *model = btk_combo_box_get_model (BTK_COMBO_BOX (data->combo));
          gint i = 0;
          BtkTreeIter iter;
          gboolean found = FALSE;
          while (btk_tree_model_iter_nth_child (model, &iter, NULL, i++))
            {
              gchar *name;
              btk_tree_model_get (model, &iter, 0, &name, -1);
              found = !g_ascii_strcasecmp (display_name, name);
              g_free (name);

              if (found)
                break;
            }
          if (!found)
            btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (data->combo), display_name);
          new_screen = bdk_display_get_default_screen (display);
        }
    }
  else
    {
      gint number_of_screens = bdk_display_get_n_screens (display);
      gint screen_num = bdk_screen_get_number (current_screen);
      if ((screen_num +1) < number_of_screens)
	new_screen = bdk_display_get_screen (display, screen_num + 1);
      else
	new_screen = bdk_display_get_screen (display, 0);
    }
  
  if (new_screen) 
    {
      btk_window_set_screen (BTK_WINDOW (data->toplevel), new_screen);
      btk_widget_destroy (data->dialog_window);
    }
}

void
screen_display_destroy_diag (BtkWidget *widget, BtkWidget *data)
{
  btk_widget_destroy (data);
}

void
create_display_screen (BtkWidget *widget)
{
  BtkWidget *table, *frame, *window, *combo_dpy, *vbox;
  BtkWidget *radio_dpy, *radio_scr, *applyb, *cancelb;
  BtkWidget *bbox;
  ScreenDisplaySelection *scr_dpy_data;
  BdkScreen *screen = btk_widget_get_screen (widget);
  static GList *valid_display_list = NULL;
  
  BdkDisplay *display = bdk_screen_get_display (screen);

  window = g_object_new (btk_window_get_type (),
			   "screen", screen,
			   "user_data", NULL,
			   "type", BTK_WINDOW_TOPLEVEL,
			   "title",
			   "Screen or Display selection",
			   "border_width", 10, NULL);
  g_signal_connect (window, "destroy", 
		    G_CALLBACK (btk_widget_destroy), NULL);

  vbox = btk_vbox_new (FALSE, 3);
  btk_container_add (BTK_CONTAINER (window), vbox);
  
  frame = btk_frame_new ("Select screen or display");
  btk_container_add (BTK_CONTAINER (vbox), frame);
  
  table = btk_table_new (2, 2, TRUE);
  btk_table_set_row_spacings (BTK_TABLE (table), 3);
  btk_table_set_col_spacings (BTK_TABLE (table), 3);

  btk_container_add (BTK_CONTAINER (frame), table);

  radio_dpy = btk_radio_button_new_with_label (NULL, "move to another X display");
  if (bdk_display_get_n_screens(display) > 1)
    radio_scr = btk_radio_button_new_with_label 
    (btk_radio_button_get_group (BTK_RADIO_BUTTON (radio_dpy)), "move to next screen");
  else
    {    
      radio_scr = btk_radio_button_new_with_label 
	(btk_radio_button_get_group (BTK_RADIO_BUTTON (radio_dpy)), 
	 "only one screen on the current display");
      btk_widget_set_sensitive (radio_scr, FALSE);
    }
  combo_dpy = btk_combo_box_text_new_with_entry ();
  btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (combo_dpy), "diabolo:0.0");
  btk_entry_set_text (BTK_ENTRY (btk_bin_get_child (BTK_BIN (combo_dpy))),
                      "<hostname>:<X Server Num>.<Screen Num>");

  btk_table_attach_defaults (BTK_TABLE (table), radio_dpy, 0, 1, 0, 1);
  btk_table_attach_defaults (BTK_TABLE (table), radio_scr, 0, 1, 1, 2);
  btk_table_attach_defaults (BTK_TABLE (table), combo_dpy, 1, 2, 0, 1);

  bbox = btk_hbutton_box_new ();
  applyb = btk_button_new_from_stock (BTK_STOCK_APPLY);
  cancelb = btk_button_new_from_stock (BTK_STOCK_CANCEL);
  
  btk_container_add (BTK_CONTAINER (vbox), bbox);

  btk_container_add (BTK_CONTAINER (bbox), applyb);
  btk_container_add (BTK_CONTAINER (bbox), cancelb);

  scr_dpy_data = g_new0 (ScreenDisplaySelection, 1);

  scr_dpy_data->entry = BTK_ENTRY (BTK_COMBO (combo_dpy)->entry);
  scr_dpy_data->radio_dpy = radio_dpy;
  scr_dpy_data->toplevel = btk_widget_get_toplevel (widget);
  scr_dpy_data->dialog_window = window;
  scr_dpy_data->valid_display_list = valid_display_list;
  scr_dpy_data->combo = combo_dpy;

  g_signal_connect (cancelb, "clicked", 
		    G_CALLBACK (screen_display_destroy_diag), window);
  g_signal_connect (applyb, "clicked", 
		    G_CALLBACK (screen_display_check), scr_dpy_data);
  btk_widget_show_all (window);
}

/* Event Watcher
 */
static gboolean event_watcher_enter_id = 0;
static gboolean event_watcher_leave_id = 0;

static gboolean
event_watcher (GSignalInvocationHint *ihint,
	       guint                  n_param_values,
	       const BValue          *param_values,
	       gpointer               data)
{
  g_print ("Watch: \"%s\" emitted for %s\n",
	   g_signal_name (ihint->signal_id),
	   B_OBJECT_TYPE_NAME (b_value_get_object (param_values + 0)));

  return TRUE;
}

static void
event_watcher_down (void)
{
  if (event_watcher_enter_id)
    {
      guint signal_id;

      signal_id = g_signal_lookup ("enter_notify_event", BTK_TYPE_WIDGET);
      g_signal_remove_emission_hook (signal_id, event_watcher_enter_id);
      event_watcher_enter_id = 0;
      signal_id = g_signal_lookup ("leave_notify_event", BTK_TYPE_WIDGET);
      g_signal_remove_emission_hook (signal_id, event_watcher_leave_id);
      event_watcher_leave_id = 0;
    }
}

static void
event_watcher_toggle (void)
{
  if (event_watcher_enter_id)
    event_watcher_down ();
  else
    {
      guint signal_id;

      signal_id = g_signal_lookup ("enter_notify_event", BTK_TYPE_WIDGET);
      event_watcher_enter_id = g_signal_add_emission_hook (signal_id, 0, event_watcher, NULL, NULL);
      signal_id = g_signal_lookup ("leave_notify_event", BTK_TYPE_WIDGET);
      event_watcher_leave_id = g_signal_add_emission_hook (signal_id, 0, event_watcher, NULL, NULL);
    }
}

static void
create_event_watcher (BtkWidget *widget)
{
  BtkWidget *button;

  if (!dialog_window)
    {
      dialog_window = btk_dialog_new ();
      btk_window_set_screen (BTK_WINDOW (dialog_window),
			     btk_widget_get_screen (widget));

      g_signal_connect (dialog_window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&dialog_window);
      g_signal_connect (dialog_window, "destroy",
			G_CALLBACK (event_watcher_down),
			NULL);

      btk_window_set_title (BTK_WINDOW (dialog_window), "Event Watcher");
      btk_container_set_border_width (BTK_CONTAINER (dialog_window), 0);
      btk_widget_set_size_request (dialog_window, 200, 110);

      button = btk_toggle_button_new_with_label ("Activate Watch");
      g_signal_connect (button, "clicked",
			G_CALLBACK (event_watcher_toggle),
			NULL);
      btk_container_set_border_width (BTK_CONTAINER (button), 10);
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (dialog_window)->vbox), 
			  button, TRUE, TRUE, 0);
      btk_widget_show (button);

      button = btk_button_new_with_label ("Close");
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (btk_widget_destroy),
				dialog_window);
      btk_widget_set_can_default (button, TRUE);
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (dialog_window)->action_area),
			  button, TRUE, TRUE, 0);
      btk_widget_grab_default (button);
      btk_widget_show (button);
    }

  if (!btk_widget_get_visible (dialog_window))
    btk_widget_show (dialog_window);
  else
    btk_widget_destroy (dialog_window);
}

/*
 * BtkRange
 */

static gchar*
reformat_value (BtkScale *scale,
                gdouble   value)
{
  return g_strdup_printf ("-->%0.*g<--",
                          btk_scale_get_digits (scale), value);
}

static void
create_range_controls (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  BtkWidget *box1;
  BtkWidget *box2;
  BtkWidget *button;
  BtkWidget *scrollbar;
  BtkWidget *scale;
  BtkWidget *separator;
  BtkObject *adjustment;
  BtkWidget *hbox;

  if (!window)
    {
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);

      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window);

      btk_window_set_title (BTK_WINDOW (window), "range controls");
      btk_container_set_border_width (BTK_CONTAINER (window), 0);


      box1 = btk_vbox_new (FALSE, 0);
      btk_container_add (BTK_CONTAINER (window), box1);
      btk_widget_show (box1);


      box2 = btk_vbox_new (FALSE, 10);
      btk_container_set_border_width (BTK_CONTAINER (box2), 10);
      btk_box_pack_start (BTK_BOX (box1), box2, TRUE, TRUE, 0);
      btk_widget_show (box2);


      adjustment = btk_adjustment_new (0.0, 0.0, 101.0, 0.1, 1.0, 1.0);

      scale = btk_hscale_new (BTK_ADJUSTMENT (adjustment));
      btk_widget_set_size_request (BTK_WIDGET (scale), 150, -1);
      btk_range_set_update_policy (BTK_RANGE (scale), BTK_UPDATE_DELAYED);
      btk_scale_set_digits (BTK_SCALE (scale), 1);
      btk_scale_set_draw_value (BTK_SCALE (scale), TRUE);
      btk_box_pack_start (BTK_BOX (box2), scale, TRUE, TRUE, 0);
      btk_widget_show (scale);

      scrollbar = btk_hscrollbar_new (BTK_ADJUSTMENT (adjustment));
      btk_range_set_update_policy (BTK_RANGE (scrollbar), 
				   BTK_UPDATE_CONTINUOUS);
      btk_box_pack_start (BTK_BOX (box2), scrollbar, TRUE, TRUE, 0);
      btk_widget_show (scrollbar);

      scale = btk_hscale_new (BTK_ADJUSTMENT (adjustment));
      btk_scale_set_draw_value (BTK_SCALE (scale), TRUE);
      g_signal_connect (scale,
                        "format_value",
                        G_CALLBACK (reformat_value),
                        NULL);
      btk_box_pack_start (BTK_BOX (box2), scale, TRUE, TRUE, 0);
      btk_widget_show (scale);
      
      hbox = btk_hbox_new (FALSE, 0);

      scale = btk_vscale_new (BTK_ADJUSTMENT (adjustment));
      btk_widget_set_size_request (scale, -1, 200);
      btk_scale_set_digits (BTK_SCALE (scale), 2);
      btk_scale_set_draw_value (BTK_SCALE (scale), TRUE);
      btk_box_pack_start (BTK_BOX (hbox), scale, TRUE, TRUE, 0);
      btk_widget_show (scale);

      scale = btk_vscale_new (BTK_ADJUSTMENT (adjustment));
      btk_widget_set_size_request (scale, -1, 200);
      btk_scale_set_digits (BTK_SCALE (scale), 2);
      btk_scale_set_draw_value (BTK_SCALE (scale), TRUE);
      btk_range_set_inverted (BTK_RANGE (scale), TRUE);
      btk_box_pack_start (BTK_BOX (hbox), scale, TRUE, TRUE, 0);
      btk_widget_show (scale);

      scale = btk_vscale_new (BTK_ADJUSTMENT (adjustment));
      btk_scale_set_draw_value (BTK_SCALE (scale), TRUE);
      g_signal_connect (scale,
                        "format_value",
                        G_CALLBACK (reformat_value),
                        NULL);
      btk_box_pack_start (BTK_BOX (hbox), scale, TRUE, TRUE, 0);
      btk_widget_show (scale);

      
      btk_box_pack_start (BTK_BOX (box2), hbox, TRUE, TRUE, 0);
      btk_widget_show (hbox);
      
      separator = btk_hseparator_new ();
      btk_box_pack_start (BTK_BOX (box1), separator, FALSE, TRUE, 0);
      btk_widget_show (separator);


      box2 = btk_vbox_new (FALSE, 10);
      btk_container_set_border_width (BTK_CONTAINER (box2), 10);
      btk_box_pack_start (BTK_BOX (box1), box2, FALSE, TRUE, 0);
      btk_widget_show (box2);


      button = btk_button_new_with_label ("close");
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (btk_widget_destroy),
				window);
      btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);
      btk_widget_set_can_default (button, TRUE);
      btk_widget_grab_default (button);
      btk_widget_show (button);
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show (window);
  else
    btk_widget_destroy (window);
}

/*
 * BtkRulers
 */

void
create_rulers (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  BtkWidget *table;
  BtkWidget *ruler;

  if (!window)
    {
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);

      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));

      g_object_set (window, "allow_shrink", TRUE, "allow_grow", TRUE, NULL);

      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window);

      btk_window_set_title (BTK_WINDOW (window), "rulers");
      btk_widget_set_size_request (window, 300, 300);
      btk_widget_set_events (window, 
			     BDK_POINTER_MOTION_MASK 
			     | BDK_POINTER_MOTION_HINT_MASK);
      btk_container_set_border_width (BTK_CONTAINER (window), 0);

      table = btk_table_new (2, 2, FALSE);
      btk_container_add (BTK_CONTAINER (window), table);
      btk_widget_show (table);

      ruler = btk_hruler_new ();
      btk_ruler_set_metric (BTK_RULER (ruler), BTK_CENTIMETERS);
      btk_ruler_set_range (BTK_RULER (ruler), 100, 0, 0, 20);

      g_signal_connect_swapped (window, 
			        "motion_notify_event",
				G_CALLBACK (BTK_WIDGET_GET_CLASS (ruler)->motion_notify_event),
			        ruler);
      
      btk_table_attach (BTK_TABLE (table), ruler, 1, 2, 0, 1,
			BTK_EXPAND | BTK_FILL, BTK_FILL, 0, 0);
      btk_widget_show (ruler);


      ruler = btk_vruler_new ();
      btk_ruler_set_range (BTK_RULER (ruler), 5, 15, 0, 20);

      g_signal_connect_swapped (window, 
			        "motion_notify_event",
			        G_CALLBACK (BTK_WIDGET_GET_CLASS (ruler)->motion_notify_event),
			        ruler);
      
      btk_table_attach (BTK_TABLE (table), ruler, 0, 1, 1, 2,
			BTK_FILL, BTK_EXPAND | BTK_FILL, 0, 0);
      btk_widget_show (ruler);
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show (window);
  else
    btk_widget_destroy (window);
}

static void
text_toggle_editable (BtkWidget *checkbutton,
		       BtkWidget *text)
{
   btk_text_set_editable(BTK_TEXT(text),
			  BTK_TOGGLE_BUTTON(checkbutton)->active);
}

static void
text_toggle_word_wrap (BtkWidget *checkbutton,
		       BtkWidget *text)
{
   btk_text_set_word_wrap(BTK_TEXT(text),
			  BTK_TOGGLE_BUTTON(checkbutton)->active);
}

struct {
  BdkColor color;
  gchar *name;
} text_colors[] = {
 { { 0, 0x0000, 0x0000, 0x0000 }, "black" },
 { { 0, 0xFFFF, 0xFFFF, 0xFFFF }, "white" },
 { { 0, 0xFFFF, 0x0000, 0x0000 }, "red" },
 { { 0, 0x0000, 0xFFFF, 0x0000 }, "green" },
 { { 0, 0x0000, 0x0000, 0xFFFF }, "blue" }, 
 { { 0, 0x0000, 0xFFFF, 0xFFFF }, "cyan" },
 { { 0, 0xFFFF, 0x0000, 0xFFFF }, "magenta" },
 { { 0, 0xFFFF, 0xFFFF, 0x0000 }, "yellow" }
};

int ntext_colors = sizeof(text_colors) / sizeof(text_colors[0]);

/*
 * BtkText
 */
void
text_insert_random (BtkWidget *w, BtkText *text)
{
  int i;
  char c;
   for (i=0; i<10; i++)
    {
      c = 'A' + rand() % ('Z' - 'A');
      btk_text_set_point (text, rand() % btk_text_get_length (text));
      btk_text_insert (text, NULL, NULL, NULL, &c, 1);
    }
}

void
create_text (BtkWidget *widget)
{
  int i, j;

  static BtkWidget *window = NULL;
  BtkWidget *box1;
  BtkWidget *box2;
  BtkWidget *hbox;
  BtkWidget *button;
  BtkWidget *check;
  BtkWidget *separator;
  BtkWidget *scrolled_window;
  BtkWidget *text;

  FILE *infile;

  if (!window)
    {
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));

      btk_widget_set_name (window, "text window");
      g_object_set (window, "allow_shrink", TRUE, "allow_grow", TRUE, NULL);
      btk_widget_set_size_request (window, 500, 500);

      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window);

      btk_window_set_title (BTK_WINDOW (window), "test");
      btk_container_set_border_width (BTK_CONTAINER (window), 0);


      box1 = btk_vbox_new (FALSE, 0);
      btk_container_add (BTK_CONTAINER (window), box1);
      btk_widget_show (box1);


      box2 = btk_vbox_new (FALSE, 10);
      btk_container_set_border_width (BTK_CONTAINER (box2), 10);
      btk_box_pack_start (BTK_BOX (box1), box2, TRUE, TRUE, 0);
      btk_widget_show (box2);


      scrolled_window = btk_scrolled_window_new (NULL, NULL);
      btk_box_pack_start (BTK_BOX (box2), scrolled_window, TRUE, TRUE, 0);
      btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (scrolled_window),
				      BTK_POLICY_NEVER,
				      BTK_POLICY_ALWAYS);
      btk_widget_show (scrolled_window);

      text = btk_text_new (NULL, NULL);
      btk_text_set_editable (BTK_TEXT (text), TRUE);
      btk_container_add (BTK_CONTAINER (scrolled_window), text);
      btk_widget_grab_focus (text);
      btk_widget_show (text);


      btk_text_freeze (BTK_TEXT (text));

      for (i=0; i<ntext_colors; i++)
	{
	  btk_text_insert (BTK_TEXT (text), NULL, NULL, NULL, 
			   text_colors[i].name, -1);
	  btk_text_insert (BTK_TEXT (text), NULL, NULL, NULL, "\t", -1);

	  for (j=0; j<ntext_colors; j++)
	    {
	      btk_text_insert (BTK_TEXT (text), NULL,
			       &text_colors[j].color, &text_colors[i].color,
			       "XYZ", -1);
	    }
	  btk_text_insert (BTK_TEXT (text), NULL, NULL, NULL, "\n", -1);
	}

      infile = fopen("testbtk.c", "r");
      
      if (infile)
	{
	  char *buffer;
	  int nbytes_read, nbytes_alloc;
	  
	  nbytes_read = 0;
	  nbytes_alloc = 1024;
	  buffer = g_new (char, nbytes_alloc);
	  while (1)
	    {
	      int len;
	      if (nbytes_alloc < nbytes_read + 1024)
		{
		  nbytes_alloc *= 2;
		  buffer = g_realloc (buffer, nbytes_alloc);
		}
	      len = fread (buffer + nbytes_read, 1, 1024, infile);
	      nbytes_read += len;
	      if (len < 1024)
		break;
	    }
	  
	  btk_text_insert (BTK_TEXT (text), NULL, NULL,
			   NULL, buffer, nbytes_read);
	  g_free(buffer);
	  fclose (infile);
	}
      
      btk_text_thaw (BTK_TEXT (text));

      hbox = btk_hbutton_box_new ();
      btk_box_pack_start (BTK_BOX (box2), hbox, FALSE, FALSE, 0);
      btk_widget_show (hbox);

      check = btk_check_button_new_with_label("Editable");
      btk_box_pack_start (BTK_BOX (hbox), check, FALSE, FALSE, 0);
      g_signal_connect (check, "toggled",
			G_CALLBACK (text_toggle_editable), text);
      btk_toggle_button_set_active(BTK_TOGGLE_BUTTON(check), TRUE);
      btk_widget_show (check);

      check = btk_check_button_new_with_label("Wrap Words");
      btk_box_pack_start (BTK_BOX (hbox), check, FALSE, TRUE, 0);
      g_signal_connect (check, "toggled",
			G_CALLBACK (text_toggle_word_wrap), text);
      btk_toggle_button_set_active(BTK_TOGGLE_BUTTON(check), FALSE);
      btk_widget_show (check);

      separator = btk_hseparator_new ();
      btk_box_pack_start (BTK_BOX (box1), separator, FALSE, TRUE, 0);
      btk_widget_show (separator);


      box2 = btk_vbox_new (FALSE, 10);
      btk_container_set_border_width (BTK_CONTAINER (box2), 10);
      btk_box_pack_start (BTK_BOX (box1), box2, FALSE, TRUE, 0);
      btk_widget_show (box2);


      button = btk_button_new_with_label ("insert random");
      g_signal_connect (button, "clicked",
			G_CALLBACK (text_insert_random),
			text);
      btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);
      btk_widget_show (button);

      button = btk_button_new_with_label ("close");
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (btk_widget_destroy),
				window);
      btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);
      btk_widget_set_can_default (button, TRUE);
      btk_widget_grab_default (button);
      btk_widget_show (button);
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show (window);
  else
    btk_widget_destroy (window);
}

/*
 * BtkNotebook
 */

BdkPixbuf *book_open;
BdkPixbuf *book_closed;
BtkWidget *sample_notebook;

static void
set_page_image (BtkNotebook *notebook, gint page_num, BdkPixbuf *pixbuf)
{
  BtkWidget *page_widget;
  BtkWidget *pixwid;

  page_widget = btk_notebook_get_nth_page (notebook, page_num);

  pixwid = g_object_get_data (B_OBJECT (page_widget), "tab_pixmap");
  btk_image_set_from_pixbuf (BTK_IMAGE (pixwid), pixbuf);
  
  pixwid = g_object_get_data (B_OBJECT (page_widget), "menu_pixmap");
  btk_image_set_from_pixbuf (BTK_IMAGE (pixwid), pixbuf);
}

static void
page_switch (BtkWidget *widget, gpointer *page, gint page_num)
{
  BtkNotebook *notebook = BTK_NOTEBOOK (widget);
  gint old_page_num = btk_notebook_get_current_page (notebook);
 
  if (page_num == old_page_num)
    return;

  set_page_image (notebook, page_num, book_open);

  if (old_page_num != -1)
    set_page_image (notebook, old_page_num, book_closed);
}

static void
tab_fill (BtkToggleButton *button, BtkWidget *child)
{
  gboolean expand;
  BtkPackType pack_type;

  btk_notebook_query_tab_label_packing (BTK_NOTEBOOK (sample_notebook), child,
					&expand, NULL, &pack_type);
  btk_notebook_set_tab_label_packing (BTK_NOTEBOOK (sample_notebook), child,
				      expand, button->active, pack_type);
}

static void
tab_expand (BtkToggleButton *button, BtkWidget *child)
{
  gboolean fill;
  BtkPackType pack_type;

  btk_notebook_query_tab_label_packing (BTK_NOTEBOOK (sample_notebook), child,
					NULL, &fill, &pack_type);
  btk_notebook_set_tab_label_packing (BTK_NOTEBOOK (sample_notebook), child,
				      button->active, fill, pack_type);
}

static void
tab_pack (BtkToggleButton *button, BtkWidget *child)
	  
{ 
  gboolean expand;
  gboolean fill;

  btk_notebook_query_tab_label_packing (BTK_NOTEBOOK (sample_notebook), child,
					&expand, &fill, NULL);
  btk_notebook_set_tab_label_packing (BTK_NOTEBOOK (sample_notebook), child,
				      expand, fill, button->active);
}

static void
create_pages (BtkNotebook *notebook, gint start, gint end)
{
  BtkWidget *child = NULL;
  BtkWidget *button;
  BtkWidget *label;
  BtkWidget *hbox;
  BtkWidget *vbox;
  BtkWidget *label_box;
  BtkWidget *menu_box;
  BtkWidget *pixwid;
  gint i;
  char buffer[32];
  char accel_buffer[32];

  for (i = start; i <= end; i++)
    {
      sprintf (buffer, "Page %d", i);
      sprintf (accel_buffer, "Page _%d", i);

      child = btk_frame_new (buffer);
      btk_container_set_border_width (BTK_CONTAINER (child), 10);

      vbox = btk_vbox_new (TRUE,0);
      btk_container_set_border_width (BTK_CONTAINER (vbox), 10);
      btk_container_add (BTK_CONTAINER (child), vbox);

      hbox = btk_hbox_new (TRUE,0);
      btk_box_pack_start (BTK_BOX (vbox), hbox, FALSE, TRUE, 5);

      button = btk_check_button_new_with_label ("Fill Tab");
      btk_box_pack_start (BTK_BOX (hbox), button, TRUE, TRUE, 5);
      btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (button), TRUE);
      g_signal_connect (button, "toggled",
			G_CALLBACK (tab_fill), child);

      button = btk_check_button_new_with_label ("Expand Tab");
      btk_box_pack_start (BTK_BOX (hbox), button, TRUE, TRUE, 5);
      g_signal_connect (button, "toggled",
			G_CALLBACK (tab_expand), child);

      button = btk_check_button_new_with_label ("Pack end");
      btk_box_pack_start (BTK_BOX (hbox), button, TRUE, TRUE, 5);
      g_signal_connect (button, "toggled",
			G_CALLBACK (tab_pack), child);

      button = btk_button_new_with_label ("Hide Page");
      btk_box_pack_end (BTK_BOX (vbox), button, FALSE, FALSE, 5);
      g_signal_connect_swapped (button, "clicked",
				G_CALLBACK (btk_widget_hide),
				child);

      btk_widget_show_all (child);

      label_box = btk_hbox_new (FALSE, 0);
      pixwid = btk_image_new_from_pixbuf (book_closed);
      g_object_set_data (B_OBJECT (child), "tab_pixmap", pixwid);
			   
      btk_box_pack_start (BTK_BOX (label_box), pixwid, FALSE, TRUE, 0);
      btk_misc_set_padding (BTK_MISC (pixwid), 3, 1);
      label = btk_label_new_with_mnemonic (accel_buffer);
      btk_box_pack_start (BTK_BOX (label_box), label, FALSE, TRUE, 0);
      btk_widget_show_all (label_box);
      
				       
      menu_box = btk_hbox_new (FALSE, 0);
      pixwid = btk_image_new_from_pixbuf (book_closed);
      g_object_set_data (B_OBJECT (child), "menu_pixmap", pixwid);
      
      btk_box_pack_start (BTK_BOX (menu_box), pixwid, FALSE, TRUE, 0);
      btk_misc_set_padding (BTK_MISC (pixwid), 3, 1);
      label = btk_label_new (buffer);
      btk_box_pack_start (BTK_BOX (menu_box), label, FALSE, TRUE, 0);
      btk_widget_show_all (menu_box);

      btk_notebook_append_page_menu (notebook, child, label_box, menu_box);
    }
}

static void
rotate_notebook (BtkButton   *button,
		 BtkNotebook *notebook)
{
  btk_notebook_set_tab_pos (notebook, (notebook->tab_pos + 1) % 4);
}

static void
show_all_pages (BtkButton   *button,
		BtkNotebook *notebook)
{  
  btk_container_foreach (BTK_CONTAINER (notebook),
			 (BtkCallback) btk_widget_show, NULL);
}

static void
notebook_type_changed (BtkWidget *optionmenu,
		       gpointer   data)
{
  BtkNotebook *notebook;
  gint i, c;

  enum {
    STANDARD,
    NOTABS,
    BORDERLESS,
    SCROLLABLE
  };

  notebook = BTK_NOTEBOOK (data);

  c = btk_option_menu_get_history (BTK_OPTION_MENU (optionmenu));

  switch (c)
    {
    case STANDARD:
      /* standard notebook */
      btk_notebook_set_show_tabs (notebook, TRUE);
      btk_notebook_set_show_border (notebook, TRUE);
      btk_notebook_set_scrollable (notebook, FALSE);
      break;

    case NOTABS:
      /* notabs notebook */
      btk_notebook_set_show_tabs (notebook, FALSE);
      btk_notebook_set_show_border (notebook, TRUE);
      break;

    case BORDERLESS:
      /* borderless */
      btk_notebook_set_show_tabs (notebook, FALSE);
      btk_notebook_set_show_border (notebook, FALSE);
      break;

    case SCROLLABLE:  
      /* scrollable */
      btk_notebook_set_show_tabs (notebook, TRUE);
      btk_notebook_set_show_border (notebook, TRUE);
      btk_notebook_set_scrollable (notebook, TRUE);
      if (g_list_length (notebook->children) == 5)
	create_pages (notebook, 6, 15);
      
      return;
      break;
    }
  
  if (g_list_length (notebook->children) == 15)
    for (i = 0; i < 10; i++)
      btk_notebook_remove_page (notebook, 5);
}

static void
notebook_popup (BtkToggleButton *button,
		BtkNotebook     *notebook)
{
  if (button->active)
    btk_notebook_popup_enable (notebook);
  else
    btk_notebook_popup_disable (notebook);
}

static void
notebook_homogeneous (BtkToggleButton *button,
		      BtkNotebook     *notebook)
{
  g_object_set (notebook, "homogeneous", button->active, NULL);
}

static void
create_notebook (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  BtkWidget *box1;
  BtkWidget *box2;
  BtkWidget *button;
  BtkWidget *separator;
  BtkWidget *omenu;
  BtkWidget *label;

  static gchar *items[] =
  {
    "Standard",
    "No tabs",
    "Borderless",
    "Scrollable"
  };
  
  if (!window)
    {
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window);

      btk_window_set_title (BTK_WINDOW (window), "notebook");
      btk_container_set_border_width (BTK_CONTAINER (window), 0);

      box1 = btk_vbox_new (FALSE, 0);
      btk_container_add (BTK_CONTAINER (window), box1);

      sample_notebook = btk_notebook_new ();
      g_signal_connect (sample_notebook, "switch_page",
			G_CALLBACK (page_switch), NULL);
      btk_notebook_set_tab_pos (BTK_NOTEBOOK (sample_notebook), BTK_POS_TOP);
      btk_box_pack_start (BTK_BOX (box1), sample_notebook, TRUE, TRUE, 0);
      btk_container_set_border_width (BTK_CONTAINER (sample_notebook), 10);

      btk_widget_realize (sample_notebook);

      if (!book_open)
	book_open = bdk_pixbuf_new_from_xpm_data ((const char **)book_open_xpm);
						  
      if (!book_closed)
	book_closed = bdk_pixbuf_new_from_xpm_data ((const char **)book_closed_xpm);

      create_pages (BTK_NOTEBOOK (sample_notebook), 1, 5);

      separator = btk_hseparator_new ();
      btk_box_pack_start (BTK_BOX (box1), separator, FALSE, TRUE, 10);
      
      box2 = btk_hbox_new (FALSE, 5);
      btk_container_set_border_width (BTK_CONTAINER (box2), 10);
      btk_box_pack_start (BTK_BOX (box1), box2, FALSE, TRUE, 0);

      button = btk_check_button_new_with_label ("popup menu");
      btk_box_pack_start (BTK_BOX (box2), button, TRUE, FALSE, 0);
      g_signal_connect (button, "clicked",
			G_CALLBACK (notebook_popup),
			sample_notebook);

      button = btk_check_button_new_with_label ("homogeneous tabs");
      btk_box_pack_start (BTK_BOX (box2), button, TRUE, FALSE, 0);
      g_signal_connect (button, "clicked",
			G_CALLBACK (notebook_homogeneous),
			sample_notebook);

      box2 = btk_hbox_new (FALSE, 5);
      btk_container_set_border_width (BTK_CONTAINER (box2), 10);
      btk_box_pack_start (BTK_BOX (box1), box2, FALSE, TRUE, 0);

      label = btk_label_new ("Notebook Style :");
      btk_box_pack_start (BTK_BOX (box2), label, FALSE, TRUE, 0);

      omenu = build_option_menu (items, G_N_ELEMENTS (items), 0,
				 notebook_type_changed,
				 sample_notebook);
      btk_box_pack_start (BTK_BOX (box2), omenu, FALSE, TRUE, 0);

      button = btk_button_new_with_label ("Show all Pages");
      btk_box_pack_start (BTK_BOX (box2), button, FALSE, TRUE, 0);
      g_signal_connect (button, "clicked",
			G_CALLBACK (show_all_pages), sample_notebook);

      box2 = btk_hbox_new (TRUE, 10);
      btk_container_set_border_width (BTK_CONTAINER (box2), 10);
      btk_box_pack_start (BTK_BOX (box1), box2, FALSE, TRUE, 0);

      button = btk_button_new_with_label ("prev");
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (btk_notebook_prev_page),
				sample_notebook);
      btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);

      button = btk_button_new_with_label ("next");
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (btk_notebook_next_page),
				sample_notebook);
      btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);

      button = btk_button_new_with_label ("rotate");
      g_signal_connect (button, "clicked",
			G_CALLBACK (rotate_notebook), sample_notebook);
      btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);

      separator = btk_hseparator_new ();
      btk_box_pack_start (BTK_BOX (box1), separator, FALSE, TRUE, 5);

      button = btk_button_new_with_label ("close");
      btk_container_set_border_width (BTK_CONTAINER (button), 5);
      g_signal_connect_swapped (button, "clicked",
			        G_CALLBACK (btk_widget_destroy),
				window);
      btk_box_pack_start (BTK_BOX (box1), button, FALSE, FALSE, 0);
      btk_widget_set_can_default (button, TRUE);
      btk_widget_grab_default (button);
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    btk_widget_destroy (window);
}

/*
 * BtkPanes
 */

void
toggle_resize (BtkWidget *widget, BtkWidget *child)
{
  BtkContainer *container = BTK_CONTAINER (btk_widget_get_parent (child));
  BValue value = { 0, };
  b_value_init (&value, B_TYPE_BOOLEAN);
  btk_container_child_get_property (container, child, "resize", &value);
  b_value_set_boolean (&value, !b_value_get_boolean (&value));
  btk_container_child_set_property (container, child, "resize", &value);
}

void
toggle_shrink (BtkWidget *widget, BtkWidget *child)
{
  BtkContainer *container = BTK_CONTAINER (btk_widget_get_parent (child));
  BValue value = { 0, };
  b_value_init (&value, B_TYPE_BOOLEAN);
  btk_container_child_get_property (container, child, "shrink", &value);
  b_value_set_boolean (&value, !b_value_get_boolean (&value));
  btk_container_child_set_property (container, child, "shrink", &value);
}

static void
paned_props_clicked (BtkWidget *button,
		     BObject   *paned)
{
  BtkWidget *window = create_prop_editor (paned, BTK_TYPE_PANED);
  
  btk_window_set_title (BTK_WINDOW (window), "Paned Properties");
}

BtkWidget *
create_pane_options (BtkPaned    *paned,
		     const gchar *frame_label,
		     const gchar *label1,
		     const gchar *label2)
{
  BtkWidget *frame;
  BtkWidget *table;
  BtkWidget *label;
  BtkWidget *button;
  BtkWidget *check_button;
  
  frame = btk_frame_new (frame_label);
  btk_container_set_border_width (BTK_CONTAINER (frame), 4);
  
  table = btk_table_new (4, 2, 4);
  btk_container_add (BTK_CONTAINER (frame), table);
  
  label = btk_label_new (label1);
  btk_table_attach_defaults (BTK_TABLE (table), label,
			     0, 1, 0, 1);
  
  check_button = btk_check_button_new_with_label ("Resize");
  btk_table_attach_defaults (BTK_TABLE (table), check_button,
			     0, 1, 1, 2);
  g_signal_connect (check_button, "toggled",
		    G_CALLBACK (toggle_resize),
		    paned->child1);
  
  check_button = btk_check_button_new_with_label ("Shrink");
  btk_table_attach_defaults (BTK_TABLE (table), check_button,
			     0, 1, 2, 3);
  btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (check_button),
			       TRUE);
  g_signal_connect (check_button, "toggled",
		    G_CALLBACK (toggle_shrink),
		    paned->child1);
  
  label = btk_label_new (label2);
  btk_table_attach_defaults (BTK_TABLE (table), label,
			     1, 2, 0, 1);
  
  check_button = btk_check_button_new_with_label ("Resize");
  btk_table_attach_defaults (BTK_TABLE (table), check_button,
			     1, 2, 1, 2);
  btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (check_button),
			       TRUE);
  g_signal_connect (check_button, "toggled",
		    G_CALLBACK (toggle_resize),
		    paned->child2);
  
  check_button = btk_check_button_new_with_label ("Shrink");
  btk_table_attach_defaults (BTK_TABLE (table), check_button,
			     1, 2, 2, 3);
  btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (check_button),
			       TRUE);
  g_signal_connect (check_button, "toggled",
		    G_CALLBACK (toggle_shrink),
		    paned->child2);

  button = btk_button_new_with_mnemonic ("_Properties");
  btk_table_attach_defaults (BTK_TABLE (table), button,
			     0, 2, 3, 4);
  g_signal_connect (button, "clicked",
		    G_CALLBACK (paned_props_clicked),
		    paned);

  return frame;
}

void
create_panes (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  BtkWidget *frame;
  BtkWidget *hpaned;
  BtkWidget *vpaned;
  BtkWidget *button;
  BtkWidget *vbox;

  if (!window)
    {
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);

      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));
      
      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window);

      btk_window_set_title (BTK_WINDOW (window), "Panes");
      btk_container_set_border_width (BTK_CONTAINER (window), 0);

      vbox = btk_vbox_new (FALSE, 0);
      btk_container_add (BTK_CONTAINER (window), vbox);
      
      vpaned = btk_vpaned_new ();
      btk_box_pack_start (BTK_BOX (vbox), vpaned, TRUE, TRUE, 0);
      btk_container_set_border_width (BTK_CONTAINER(vpaned), 5);

      hpaned = btk_hpaned_new ();
      btk_paned_add1 (BTK_PANED (vpaned), hpaned);

      frame = btk_frame_new (NULL);
      btk_frame_set_shadow_type (BTK_FRAME(frame), BTK_SHADOW_IN);
      btk_widget_set_size_request (frame, 60, 60);
      btk_paned_add1 (BTK_PANED (hpaned), frame);
      
      button = btk_button_new_with_label ("Hi there");
      btk_container_add (BTK_CONTAINER(frame), button);

      frame = btk_frame_new (NULL);
      btk_frame_set_shadow_type (BTK_FRAME(frame), BTK_SHADOW_IN);
      btk_widget_set_size_request (frame, 80, 60);
      btk_paned_add2 (BTK_PANED (hpaned), frame);

      frame = btk_frame_new (NULL);
      btk_frame_set_shadow_type (BTK_FRAME(frame), BTK_SHADOW_IN);
      btk_widget_set_size_request (frame, 60, 80);
      btk_paned_add2 (BTK_PANED (vpaned), frame);

      /* Now create toggle buttons to control sizing */

      btk_box_pack_start (BTK_BOX (vbox),
			  create_pane_options (BTK_PANED (hpaned),
					       "Horizontal",
					       "Left",
					       "Right"),
			  FALSE, FALSE, 0);

      btk_box_pack_start (BTK_BOX (vbox),
			  create_pane_options (BTK_PANED (vpaned),
					       "Vertical",
					       "Top",
					       "Bottom"),
			  FALSE, FALSE, 0);

      btk_widget_show_all (vbox);
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show (window);
  else
    btk_widget_destroy (window);
}

/*
 * Paned keyboard navigation
 */

static BtkWidget*
paned_keyboard_window1 (BtkWidget *widget)
{
  BtkWidget *window1;
  BtkWidget *hpaned1;
  BtkWidget *frame1;
  BtkWidget *vbox1;
  BtkWidget *button7;
  BtkWidget *button8;
  BtkWidget *button9;
  BtkWidget *vpaned1;
  BtkWidget *frame2;
  BtkWidget *frame5;
  BtkWidget *hbox1;
  BtkWidget *button5;
  BtkWidget *button6;
  BtkWidget *frame3;
  BtkWidget *frame4;
  BtkWidget *table1;
  BtkWidget *button1;
  BtkWidget *button2;
  BtkWidget *button3;
  BtkWidget *button4;

  window1 = btk_window_new (BTK_WINDOW_TOPLEVEL);
  btk_window_set_title (BTK_WINDOW (window1), "Basic paned navigation");
  btk_window_set_screen (BTK_WINDOW (window1), 
			 btk_widget_get_screen (widget));

  hpaned1 = btk_hpaned_new ();
  btk_container_add (BTK_CONTAINER (window1), hpaned1);

  frame1 = btk_frame_new (NULL);
  btk_paned_pack1 (BTK_PANED (hpaned1), frame1, FALSE, TRUE);
  btk_frame_set_shadow_type (BTK_FRAME (frame1), BTK_SHADOW_IN);

  vbox1 = btk_vbox_new (FALSE, 0);
  btk_container_add (BTK_CONTAINER (frame1), vbox1);

  button7 = btk_button_new_with_label ("button7");
  btk_box_pack_start (BTK_BOX (vbox1), button7, FALSE, FALSE, 0);

  button8 = btk_button_new_with_label ("button8");
  btk_box_pack_start (BTK_BOX (vbox1), button8, FALSE, FALSE, 0);

  button9 = btk_button_new_with_label ("button9");
  btk_box_pack_start (BTK_BOX (vbox1), button9, FALSE, FALSE, 0);

  vpaned1 = btk_vpaned_new ();
  btk_paned_pack2 (BTK_PANED (hpaned1), vpaned1, TRUE, TRUE);

  frame2 = btk_frame_new (NULL);
  btk_paned_pack1 (BTK_PANED (vpaned1), frame2, FALSE, TRUE);
  btk_frame_set_shadow_type (BTK_FRAME (frame2), BTK_SHADOW_IN);

  frame5 = btk_frame_new (NULL);
  btk_container_add (BTK_CONTAINER (frame2), frame5);

  hbox1 = btk_hbox_new (FALSE, 0);
  btk_container_add (BTK_CONTAINER (frame5), hbox1);

  button5 = btk_button_new_with_label ("button5");
  btk_box_pack_start (BTK_BOX (hbox1), button5, FALSE, FALSE, 0);

  button6 = btk_button_new_with_label ("button6");
  btk_box_pack_start (BTK_BOX (hbox1), button6, FALSE, FALSE, 0);

  frame3 = btk_frame_new (NULL);
  btk_paned_pack2 (BTK_PANED (vpaned1), frame3, TRUE, TRUE);
  btk_frame_set_shadow_type (BTK_FRAME (frame3), BTK_SHADOW_IN);

  frame4 = btk_frame_new ("Buttons");
  btk_container_add (BTK_CONTAINER (frame3), frame4);
  btk_container_set_border_width (BTK_CONTAINER (frame4), 15);

  table1 = btk_table_new (2, 2, FALSE);
  btk_container_add (BTK_CONTAINER (frame4), table1);
  btk_container_set_border_width (BTK_CONTAINER (table1), 11);

  button1 = btk_button_new_with_label ("button1");
  btk_table_attach (BTK_TABLE (table1), button1, 0, 1, 0, 1,
                    (BtkAttachOptions) (BTK_FILL),
                    (BtkAttachOptions) (0), 0, 0);

  button2 = btk_button_new_with_label ("button2");
  btk_table_attach (BTK_TABLE (table1), button2, 1, 2, 0, 1,
                    (BtkAttachOptions) (BTK_FILL),
                    (BtkAttachOptions) (0), 0, 0);

  button3 = btk_button_new_with_label ("button3");
  btk_table_attach (BTK_TABLE (table1), button3, 0, 1, 1, 2,
                    (BtkAttachOptions) (BTK_FILL),
                    (BtkAttachOptions) (0), 0, 0);

  button4 = btk_button_new_with_label ("button4");
  btk_table_attach (BTK_TABLE (table1), button4, 1, 2, 1, 2,
                    (BtkAttachOptions) (BTK_FILL),
                    (BtkAttachOptions) (0), 0, 0);

  return window1;
}

static BtkWidget*
paned_keyboard_window2 (BtkWidget *widget)
{
  BtkWidget *window2;
  BtkWidget *hpaned2;
  BtkWidget *frame6;
  BtkWidget *button13;
  BtkWidget *hbox2;
  BtkWidget *vpaned2;
  BtkWidget *frame7;
  BtkWidget *button12;
  BtkWidget *frame8;
  BtkWidget *button11;
  BtkWidget *button10;

  window2 = btk_window_new (BTK_WINDOW_TOPLEVEL);
  btk_window_set_title (BTK_WINDOW (window2), "\"button 10\" is not inside the horisontal pane");

  btk_window_set_screen (BTK_WINDOW (window2), 
			 btk_widget_get_screen (widget));

  hpaned2 = btk_hpaned_new ();
  btk_container_add (BTK_CONTAINER (window2), hpaned2);

  frame6 = btk_frame_new (NULL);
  btk_paned_pack1 (BTK_PANED (hpaned2), frame6, FALSE, TRUE);
  btk_frame_set_shadow_type (BTK_FRAME (frame6), BTK_SHADOW_IN);

  button13 = btk_button_new_with_label ("button13");
  btk_container_add (BTK_CONTAINER (frame6), button13);

  hbox2 = btk_hbox_new (FALSE, 0);
  btk_paned_pack2 (BTK_PANED (hpaned2), hbox2, TRUE, TRUE);

  vpaned2 = btk_vpaned_new ();
  btk_box_pack_start (BTK_BOX (hbox2), vpaned2, TRUE, TRUE, 0);

  frame7 = btk_frame_new (NULL);
  btk_paned_pack1 (BTK_PANED (vpaned2), frame7, FALSE, TRUE);
  btk_frame_set_shadow_type (BTK_FRAME (frame7), BTK_SHADOW_IN);

  button12 = btk_button_new_with_label ("button12");
  btk_container_add (BTK_CONTAINER (frame7), button12);

  frame8 = btk_frame_new (NULL);
  btk_paned_pack2 (BTK_PANED (vpaned2), frame8, TRUE, TRUE);
  btk_frame_set_shadow_type (BTK_FRAME (frame8), BTK_SHADOW_IN);

  button11 = btk_button_new_with_label ("button11");
  btk_container_add (BTK_CONTAINER (frame8), button11);

  button10 = btk_button_new_with_label ("button10");
  btk_box_pack_start (BTK_BOX (hbox2), button10, FALSE, FALSE, 0);

  return window2;
}

static BtkWidget*
paned_keyboard_window3 (BtkWidget *widget)
{
  BtkWidget *window3;
  BtkWidget *vbox2;
  BtkWidget *label1;
  BtkWidget *hpaned3;
  BtkWidget *frame9;
  BtkWidget *button14;
  BtkWidget *hpaned4;
  BtkWidget *frame10;
  BtkWidget *button15;
  BtkWidget *hpaned5;
  BtkWidget *frame11;
  BtkWidget *button16;
  BtkWidget *frame12;
  BtkWidget *button17;

  window3 = btk_window_new (BTK_WINDOW_TOPLEVEL);
  g_object_set_data (B_OBJECT (window3), "window3", window3);
  btk_window_set_title (BTK_WINDOW (window3), "Nested panes");

  btk_window_set_screen (BTK_WINDOW (window3), 
			 btk_widget_get_screen (widget));
  

  vbox2 = btk_vbox_new (FALSE, 0);
  btk_container_add (BTK_CONTAINER (window3), vbox2);

  label1 = btk_label_new ("Three panes nested inside each other");
  btk_box_pack_start (BTK_BOX (vbox2), label1, FALSE, FALSE, 0);

  hpaned3 = btk_hpaned_new ();
  btk_box_pack_start (BTK_BOX (vbox2), hpaned3, TRUE, TRUE, 0);

  frame9 = btk_frame_new (NULL);
  btk_paned_pack1 (BTK_PANED (hpaned3), frame9, FALSE, TRUE);
  btk_frame_set_shadow_type (BTK_FRAME (frame9), BTK_SHADOW_IN);

  button14 = btk_button_new_with_label ("button14");
  btk_container_add (BTK_CONTAINER (frame9), button14);

  hpaned4 = btk_hpaned_new ();
  btk_paned_pack2 (BTK_PANED (hpaned3), hpaned4, TRUE, TRUE);

  frame10 = btk_frame_new (NULL);
  btk_paned_pack1 (BTK_PANED (hpaned4), frame10, FALSE, TRUE);
  btk_frame_set_shadow_type (BTK_FRAME (frame10), BTK_SHADOW_IN);

  button15 = btk_button_new_with_label ("button15");
  btk_container_add (BTK_CONTAINER (frame10), button15);

  hpaned5 = btk_hpaned_new ();
  btk_paned_pack2 (BTK_PANED (hpaned4), hpaned5, TRUE, TRUE);

  frame11 = btk_frame_new (NULL);
  btk_paned_pack1 (BTK_PANED (hpaned5), frame11, FALSE, TRUE);
  btk_frame_set_shadow_type (BTK_FRAME (frame11), BTK_SHADOW_IN);

  button16 = btk_button_new_with_label ("button16");
  btk_container_add (BTK_CONTAINER (frame11), button16);

  frame12 = btk_frame_new (NULL);
  btk_paned_pack2 (BTK_PANED (hpaned5), frame12, TRUE, TRUE);
  btk_frame_set_shadow_type (BTK_FRAME (frame12), BTK_SHADOW_IN);

  button17 = btk_button_new_with_label ("button17");
  btk_container_add (BTK_CONTAINER (frame12), button17);

  return window3;
}

static BtkWidget*
paned_keyboard_window4 (BtkWidget *widget)
{
  BtkWidget *window4;
  BtkWidget *vbox3;
  BtkWidget *label2;
  BtkWidget *hpaned6;
  BtkWidget *vpaned3;
  BtkWidget *button19;
  BtkWidget *button18;
  BtkWidget *hbox3;
  BtkWidget *vpaned4;
  BtkWidget *button21;
  BtkWidget *button20;
  BtkWidget *vpaned5;
  BtkWidget *button23;
  BtkWidget *button22;
  BtkWidget *vpaned6;
  BtkWidget *button25;
  BtkWidget *button24;

  window4 = btk_window_new (BTK_WINDOW_TOPLEVEL);
  g_object_set_data (B_OBJECT (window4), "window4", window4);
  btk_window_set_title (BTK_WINDOW (window4), "window4");

  btk_window_set_screen (BTK_WINDOW (window4), 
			 btk_widget_get_screen (widget));

  vbox3 = btk_vbox_new (FALSE, 0);
  btk_container_add (BTK_CONTAINER (window4), vbox3);

  label2 = btk_label_new ("Widget tree:\n\nhpaned \n - vpaned\n - hbox\n    - vpaned\n    - vpaned\n    - vpaned\n");
  btk_box_pack_start (BTK_BOX (vbox3), label2, FALSE, FALSE, 0);
  btk_label_set_justify (BTK_LABEL (label2), BTK_JUSTIFY_LEFT);

  hpaned6 = btk_hpaned_new ();
  btk_box_pack_start (BTK_BOX (vbox3), hpaned6, TRUE, TRUE, 0);

  vpaned3 = btk_vpaned_new ();
  btk_paned_pack1 (BTK_PANED (hpaned6), vpaned3, FALSE, TRUE);

  button19 = btk_button_new_with_label ("button19");
  btk_paned_pack1 (BTK_PANED (vpaned3), button19, FALSE, TRUE);

  button18 = btk_button_new_with_label ("button18");
  btk_paned_pack2 (BTK_PANED (vpaned3), button18, TRUE, TRUE);

  hbox3 = btk_hbox_new (FALSE, 0);
  btk_paned_pack2 (BTK_PANED (hpaned6), hbox3, TRUE, TRUE);

  vpaned4 = btk_vpaned_new ();
  btk_box_pack_start (BTK_BOX (hbox3), vpaned4, TRUE, TRUE, 0);

  button21 = btk_button_new_with_label ("button21");
  btk_paned_pack1 (BTK_PANED (vpaned4), button21, FALSE, TRUE);

  button20 = btk_button_new_with_label ("button20");
  btk_paned_pack2 (BTK_PANED (vpaned4), button20, TRUE, TRUE);

  vpaned5 = btk_vpaned_new ();
  btk_box_pack_start (BTK_BOX (hbox3), vpaned5, TRUE, TRUE, 0);

  button23 = btk_button_new_with_label ("button23");
  btk_paned_pack1 (BTK_PANED (vpaned5), button23, FALSE, TRUE);

  button22 = btk_button_new_with_label ("button22");
  btk_paned_pack2 (BTK_PANED (vpaned5), button22, TRUE, TRUE);

  vpaned6 = btk_vpaned_new ();
  btk_box_pack_start (BTK_BOX (hbox3), vpaned6, TRUE, TRUE, 0);

  button25 = btk_button_new_with_label ("button25");
  btk_paned_pack1 (BTK_PANED (vpaned6), button25, FALSE, TRUE);

  button24 = btk_button_new_with_label ("button24");
  btk_paned_pack2 (BTK_PANED (vpaned6), button24, TRUE, TRUE);

  return window4;
}

static void
create_paned_keyboard_navigation (BtkWidget *widget)
{
  static BtkWidget *window1 = NULL;
  static BtkWidget *window2 = NULL;
  static BtkWidget *window3 = NULL;
  static BtkWidget *window4 = NULL;

  if (window1 && 
     (btk_widget_get_screen (window1) != btk_widget_get_screen (widget)))
    {
      btk_widget_destroy (window1);
      btk_widget_destroy (window2);
      btk_widget_destroy (window3);
      btk_widget_destroy (window4);
    }
  
  if (!window1)
    {
      window1 = paned_keyboard_window1 (widget);
      g_signal_connect (window1, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window1);
    }

  if (!window2)
    {
      window2 = paned_keyboard_window2 (widget);
      g_signal_connect (window2, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window2);
    }

  if (!window3)
    {
      window3 = paned_keyboard_window3 (widget);
      g_signal_connect (window3, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window3);
    }

  if (!window4)
    {
      window4 = paned_keyboard_window4 (widget);
      g_signal_connect (window4, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window4);
    }

  if (btk_widget_get_visible (window1))
    btk_widget_destroy (BTK_WIDGET (window1));
  else
    btk_widget_show_all (BTK_WIDGET (window1));

  if (btk_widget_get_visible (window2))
    btk_widget_destroy (BTK_WIDGET (window2));
  else
    btk_widget_show_all (BTK_WIDGET (window2));

  if (btk_widget_get_visible (window3))
    btk_widget_destroy (BTK_WIDGET (window3));
  else
    btk_widget_show_all (BTK_WIDGET (window3));

  if (btk_widget_get_visible (window4))
    btk_widget_destroy (BTK_WIDGET (window4));
  else
    btk_widget_show_all (BTK_WIDGET (window4));
}


/*
 * Shaped Windows
 */

typedef struct _cursoroffset {gint x,y;} CursorOffset;

static void
shape_pressed (BtkWidget *widget, BdkEventButton *event)
{
  CursorOffset *p;

  /* ignore double and triple click */
  if (event->type != BDK_BUTTON_PRESS)
    return;

  p = g_object_get_data (B_OBJECT (widget), "cursor_offset");
  p->x = (int) event->x;
  p->y = (int) event->y;

  btk_grab_add (widget);
  bdk_pointer_grab (widget->window, TRUE,
		    BDK_BUTTON_RELEASE_MASK |
		    BDK_BUTTON_MOTION_MASK |
		    BDK_POINTER_MOTION_HINT_MASK,
		    NULL, NULL, 0);
}

static void
shape_released (BtkWidget *widget)
{
  btk_grab_remove (widget);
  bdk_display_pointer_ungrab (btk_widget_get_display (widget),
			      BDK_CURRENT_TIME);
}

static void
shape_motion (BtkWidget      *widget, 
	      BdkEventMotion *event)
{
  gint xp, yp;
  CursorOffset * p;
  BdkModifierType mask;

  p = g_object_get_data (B_OBJECT (widget), "cursor_offset");

  /*
   * Can't use event->x / event->y here 
   * because I need absolute coordinates.
   */
  bdk_window_get_pointer (NULL, &xp, &yp, &mask);
  btk_widget_set_uposition (widget, xp  - p->x, yp  - p->y);
}

BtkWidget *
shape_create_icon (BdkScreen *screen,
		   char      *xpm_file,
		   gint       x,
		   gint       y,
		   gint       px,
		   gint       py,
		   gint       window_type)
{
  BtkWidget *window;
  BtkWidget *pixmap;
  BtkWidget *fixed;
  CursorOffset* icon_pos;
  BdkBitmap *bdk_pixmap_mask;
  BdkPixmap *bdk_pixmap;
  BtkStyle *style;

  style = btk_widget_get_default_style ();

  /*
   * BDK_WINDOW_TOPLEVEL works also, giving you a title border
   */
  window = btk_window_new (window_type);
  btk_window_set_screen (BTK_WINDOW (window), screen);
  
  fixed = btk_fixed_new ();
  btk_widget_set_size_request (fixed, 100, 100);
  btk_container_add (BTK_CONTAINER (window), fixed);
  btk_widget_show (fixed);
  
  btk_widget_set_events (window, 
			 btk_widget_get_events (window) |
			 BDK_BUTTON_MOTION_MASK |
			 BDK_POINTER_MOTION_HINT_MASK |
			 BDK_BUTTON_PRESS_MASK);

  btk_widget_realize (window);
  bdk_pixmap = bdk_pixmap_create_from_xpm (window->window, &bdk_pixmap_mask, 
					   &style->bg[BTK_STATE_NORMAL],
					   xpm_file);

  pixmap = btk_image_new_from_pixmap (bdk_pixmap, bdk_pixmap_mask);
  btk_fixed_put (BTK_FIXED (fixed), pixmap, px,py);
  btk_widget_show (pixmap);
  
  btk_widget_shape_combine_mask (window, bdk_pixmap_mask, px, py);
  
  g_object_unref (bdk_pixmap_mask);
  g_object_unref (bdk_pixmap);

  g_signal_connect (window, "button_press_event",
		    G_CALLBACK (shape_pressed), NULL);
  g_signal_connect (window, "button_release_event",
		    G_CALLBACK (shape_released), NULL);
  g_signal_connect (window, "motion_notify_event",
		    G_CALLBACK (shape_motion), NULL);

  icon_pos = g_new (CursorOffset, 1);
  g_object_set_data (B_OBJECT (window), "cursor_offset", icon_pos);

  btk_widget_set_uposition (window, x, y);
  btk_widget_show (window);
  
  return window;
}

void 
create_shapes (BtkWidget *widget)
{
  /* Variables used by the Drag/Drop and Shape Window demos */
  static BtkWidget *modeller = NULL;
  static BtkWidget *sheets = NULL;
  static BtkWidget *rings = NULL;
  static BtkWidget *with_rebunnyion = NULL;
  BdkScreen *screen = btk_widget_get_screen (widget);
  
  if (!(file_exists ("Modeller.xpm") &&
	file_exists ("FilesQueue.xpm") &&
	file_exists ("3DRings.xpm")))
    return;
  

  if (!modeller)
    {
      modeller = shape_create_icon (screen, "Modeller.xpm",
				    440, 140, 0,0, BTK_WINDOW_POPUP);

      g_signal_connect (modeller, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&modeller);
    }
  else
    btk_widget_destroy (modeller);

  if (!sheets)
    {
      sheets = shape_create_icon (screen, "FilesQueue.xpm",
				  580, 170, 0,0, BTK_WINDOW_POPUP);

      g_signal_connect (sheets, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&sheets);

    }
  else
    btk_widget_destroy (sheets);

  if (!rings)
    {
      rings = shape_create_icon (screen, "3DRings.xpm",
				 460, 270, 25,25, BTK_WINDOW_TOPLEVEL);

      g_signal_connect (rings, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&rings);
    }
  else
    btk_widget_destroy (rings);

  if (!with_rebunnyion)
    {
      BdkRebunnyion *rebunnyion;
      gint x, y;
      
      with_rebunnyion = shape_create_icon (screen, "3DRings.xpm",
                                       460, 270, 25,25, BTK_WINDOW_TOPLEVEL);

      btk_window_set_decorated (BTK_WINDOW (with_rebunnyion), FALSE);
      
      g_signal_connect (with_rebunnyion, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&with_rebunnyion);

      /* reset shape from mask to a rebunnyion */
      x = 0;
      y = 0;
      rebunnyion = bdk_rebunnyion_new ();

      while (x < 460)
        {
          while (y < 270)
            {
              BdkRectangle rect;
              rect.x = x;
              rect.y = y;
              rect.width = 10;
              rect.height = 10;

              bdk_rebunnyion_union_with_rect (rebunnyion, &rect);
              
              y += 20;
            }
          y = 0;
          x += 20;
        }

      bdk_window_shape_combine_rebunnyion (with_rebunnyion->window,
                                       rebunnyion,
                                       0, 0);
    }
  else
    btk_widget_destroy (with_rebunnyion);
}

/*
 * WM Hints demo
 */

void
create_wmhints (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  BtkWidget *label;
  BtkWidget *separator;
  BtkWidget *button;
  BtkWidget *box1;
  BtkWidget *box2;

  BdkBitmap *circles;

  if (!window)
    {
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);

      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));
      
      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window);

      btk_window_set_title (BTK_WINDOW (window), "WM Hints");
      btk_container_set_border_width (BTK_CONTAINER (window), 0);

      btk_widget_realize (window);
      
      circles = bdk_bitmap_create_from_data (window->window,
					     (gchar *) circles_bits,
					     circles_width,
					     circles_height);
      bdk_window_set_icon (window->window, NULL,
			   circles, circles);
      
      bdk_window_set_icon_name (window->window, "WMHints Test Icon");
  
      bdk_window_set_decorations (window->window, BDK_DECOR_ALL | BDK_DECOR_MENU);
      bdk_window_set_functions (window->window, BDK_FUNC_ALL | BDK_FUNC_RESIZE);
      
      box1 = btk_vbox_new (FALSE, 0);
      btk_container_add (BTK_CONTAINER (window), box1);
      btk_widget_show (box1);

      label = btk_label_new ("Try iconizing me!");
      btk_widget_set_size_request (label, 150, 50);
      btk_box_pack_start (BTK_BOX (box1), label, TRUE, TRUE, 0);
      btk_widget_show (label);


      separator = btk_hseparator_new ();
      btk_box_pack_start (BTK_BOX (box1), separator, FALSE, TRUE, 0);
      btk_widget_show (separator);


      box2 = btk_vbox_new (FALSE, 10);
      btk_container_set_border_width (BTK_CONTAINER (box2), 10);
      btk_box_pack_start (BTK_BOX (box1), box2, FALSE, TRUE, 0);
      btk_widget_show (box2);


      button = btk_button_new_with_label ("close");

      g_signal_connect_swapped (button, "clicked",
				G_CALLBACK (btk_widget_destroy),
				window);

      btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);
      btk_widget_set_can_default (button, TRUE);
      btk_widget_grab_default (button);
      btk_widget_show (button);
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show (window);
  else
    btk_widget_destroy (window);
}


/*
 * Window state tracking
 */

static gint
window_state_callback (BtkWidget *widget,
                       BdkEventWindowState *event,
                       gpointer data)
{
  BtkWidget *label = data;
  gchar *msg;

  msg = g_strconcat (BTK_WINDOW (widget)->title, ": ",
                     (event->new_window_state & BDK_WINDOW_STATE_WITHDRAWN) ?
                     "withdrawn" : "not withdrawn", ", ",
                     (event->new_window_state & BDK_WINDOW_STATE_ICONIFIED) ?
                     "iconified" : "not iconified", ", ",
                     (event->new_window_state & BDK_WINDOW_STATE_STICKY) ?
                     "sticky" : "not sticky", ", ",
                     (event->new_window_state & BDK_WINDOW_STATE_MAXIMIZED) ?
                     "maximized" : "not maximized", ", ",
                     (event->new_window_state & BDK_WINDOW_STATE_FULLSCREEN) ?
                     "fullscreen" : "not fullscreen",
                     (event->new_window_state & BDK_WINDOW_STATE_ABOVE) ?
                     "above" : "not above", ", ",
                     (event->new_window_state & BDK_WINDOW_STATE_BELOW) ?
                     "below" : "not below", ", ",
                     NULL);
  
  btk_label_set_text (BTK_LABEL (label), msg);

  g_free (msg);

  return FALSE;
}

static BtkWidget*
tracking_label (BtkWidget *window)
{
  BtkWidget *label;
  BtkWidget *hbox;
  BtkWidget *button;

  hbox = btk_hbox_new (FALSE, 5);

  g_signal_connect_object (hbox,
			   "destroy",
			   G_CALLBACK (btk_widget_destroy),
			   window,
			   G_CONNECT_SWAPPED);
  
  label = btk_label_new ("<no window state events received>");
  btk_label_set_line_wrap (BTK_LABEL (label), TRUE);
  btk_box_pack_start (BTK_BOX (hbox), label, FALSE, FALSE, 0);
  
  g_signal_connect (window,
		    "window_state_event",
		    G_CALLBACK (window_state_callback),
		    label);

  button = btk_button_new_with_label ("Deiconify");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (btk_window_deiconify),
                           window,
			   G_CONNECT_SWAPPED);
  btk_box_pack_end (BTK_BOX (hbox), button, FALSE, FALSE, 0);

  button = btk_button_new_with_label ("Iconify");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (btk_window_iconify),
                           window,
			   G_CONNECT_SWAPPED);
  btk_box_pack_end (BTK_BOX (hbox), button, FALSE, FALSE, 0);

  button = btk_button_new_with_label ("Fullscreen");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (btk_window_fullscreen),
                           window,
			   G_CONNECT_SWAPPED);
  btk_box_pack_end (BTK_BOX (hbox), button, FALSE, FALSE, 0);

  button = btk_button_new_with_label ("Unfullscreen");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (btk_window_unfullscreen),
                           window,
			   G_CONNECT_SWAPPED);
  btk_box_pack_end (BTK_BOX (hbox), button, FALSE, FALSE, 0);
  
  button = btk_button_new_with_label ("Present");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (btk_window_present),
                           window,
			   G_CONNECT_SWAPPED);
  btk_box_pack_end (BTK_BOX (hbox), button, FALSE, FALSE, 0);

  button = btk_button_new_with_label ("Show");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (btk_widget_show),
                           window,
			   G_CONNECT_SWAPPED);
  btk_box_pack_end (BTK_BOX (hbox), button, FALSE, FALSE, 0);
  
  btk_widget_show_all (hbox);
  
  return hbox;
}

void
keep_window_above (BtkToggleButton *togglebutton, gpointer data)
{
  BtkWidget *button = g_object_get_data (B_OBJECT (togglebutton), "radio");

  btk_window_set_keep_above (BTK_WINDOW (data),
                             btk_toggle_button_get_active (togglebutton));

  if (btk_toggle_button_get_active (togglebutton))
    btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (button), FALSE);
}

void
keep_window_below (BtkToggleButton *togglebutton, gpointer data)
{
  BtkWidget *button = g_object_get_data (B_OBJECT (togglebutton), "radio");

  btk_window_set_keep_below (BTK_WINDOW (data),
                             btk_toggle_button_get_active (togglebutton));

  if (btk_toggle_button_get_active (togglebutton))
    btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (button), FALSE);
}


static BtkWidget*
get_state_controls (BtkWidget *window)
{
  BtkWidget *vbox;
  BtkWidget *button;
  BtkWidget *button_above;
  BtkWidget *button_below;

  vbox = btk_vbox_new (FALSE, 0);
  
  button = btk_button_new_with_label ("Stick");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (btk_window_stick),
			   window,
			   G_CONNECT_SWAPPED);
  btk_box_pack_start (BTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = btk_button_new_with_label ("Unstick");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (btk_window_unstick),
			   window,
			   G_CONNECT_SWAPPED);
  btk_box_pack_start (BTK_BOX (vbox), button, FALSE, FALSE, 0);
  
  button = btk_button_new_with_label ("Maximize");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (btk_window_maximize),
			   window,
			   G_CONNECT_SWAPPED);
  btk_box_pack_start (BTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = btk_button_new_with_label ("Unmaximize");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (btk_window_unmaximize),
			   window,
			   G_CONNECT_SWAPPED);
  btk_box_pack_start (BTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = btk_button_new_with_label ("Iconify");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (btk_window_iconify),
			   window,
			   G_CONNECT_SWAPPED);
  btk_box_pack_start (BTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = btk_button_new_with_label ("Fullscreen");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (btk_window_fullscreen),
                           window,
			   G_CONNECT_SWAPPED);
  btk_box_pack_start (BTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = btk_button_new_with_label ("Unfullscreen");
  g_signal_connect_object (button,
			   "clicked",
                           G_CALLBACK (btk_window_unfullscreen),
			   window,
			   G_CONNECT_SWAPPED);
  btk_box_pack_start (BTK_BOX (vbox), button, FALSE, FALSE, 0);

  button_above = btk_toggle_button_new_with_label ("Keep above");
  g_signal_connect (button_above,
		    "toggled",
		    G_CALLBACK (keep_window_above),
		    window);
  btk_box_pack_start (BTK_BOX (vbox), button_above, FALSE, FALSE, 0);

  button_below = btk_toggle_button_new_with_label ("Keep below");
  g_signal_connect (button_below,
		    "toggled",
		    G_CALLBACK (keep_window_below),
		    window);
  btk_box_pack_start (BTK_BOX (vbox), button_below, FALSE, FALSE, 0);

  g_object_set_data (B_OBJECT (button_above), "radio", button_below);
  g_object_set_data (B_OBJECT (button_below), "radio", button_above);

  button = btk_button_new_with_label ("Hide (withdraw)");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (btk_widget_hide),
			   window,
			   G_CONNECT_SWAPPED);
  btk_box_pack_start (BTK_BOX (vbox), button, FALSE, FALSE, 0);
  
  btk_widget_show_all (vbox);

  return vbox;
}

void
create_window_states (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  BtkWidget *label;
  BtkWidget *box1;
  BtkWidget *iconified;
  BtkWidget *normal;
  BtkWidget *controls;

  if (!window)
    {
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window);

      btk_window_set_title (BTK_WINDOW (window), "Window states");
      
      box1 = btk_vbox_new (FALSE, 0);
      btk_container_add (BTK_CONTAINER (window), box1);

      iconified = btk_window_new (BTK_WINDOW_TOPLEVEL);

      btk_window_set_screen (BTK_WINDOW (iconified),
			     btk_widget_get_screen (widget));
      
      g_signal_connect_object (iconified, "destroy",
			       G_CALLBACK (btk_widget_destroy),
			       window,
			       G_CONNECT_SWAPPED);
      btk_window_iconify (BTK_WINDOW (iconified));
      btk_window_set_title (BTK_WINDOW (iconified), "Iconified initially");
      controls = get_state_controls (iconified);
      btk_container_add (BTK_CONTAINER (iconified), controls);
      
      normal = btk_window_new (BTK_WINDOW_TOPLEVEL);

      btk_window_set_screen (BTK_WINDOW (normal),
			     btk_widget_get_screen (widget));
      
      g_signal_connect_object (normal, "destroy",
			       G_CALLBACK (btk_widget_destroy),
			       window,
			       G_CONNECT_SWAPPED);
      
      btk_window_set_title (BTK_WINDOW (normal), "Deiconified initially");
      controls = get_state_controls (normal);
      btk_container_add (BTK_CONTAINER (normal), controls);
      
      label = tracking_label (iconified);
      btk_container_add (BTK_CONTAINER (box1), label);

      label = tracking_label (normal);
      btk_container_add (BTK_CONTAINER (box1), label);

      btk_widget_show_all (iconified);
      btk_widget_show_all (normal);
      btk_widget_show_all (box1);
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show (window);
  else
    btk_widget_destroy (window);
}

/*
 * Window sizing
 */

static gint
configure_event_callback (BtkWidget *widget,
                          BdkEventConfigure *event,
                          gpointer data)
{
  BtkWidget *label = data;
  gchar *msg;
  gint x, y;
  
  btk_window_get_position (BTK_WINDOW (widget), &x, &y);
  
  msg = g_strdup_printf ("event: %d,%d  %d x %d\n"
                         "position: %d, %d",
                         event->x, event->y, event->width, event->height,
                         x, y);
  
  btk_label_set_text (BTK_LABEL (label), msg);

  g_free (msg);

  return FALSE;
}

static void
get_ints (BtkWidget *window,
          gint      *a,
          gint      *b)
{
  BtkWidget *spin1;
  BtkWidget *spin2;

  spin1 = g_object_get_data (B_OBJECT (window), "spin1");
  spin2 = g_object_get_data (B_OBJECT (window), "spin2");

  *a = btk_spin_button_get_value_as_int (BTK_SPIN_BUTTON (spin1));
  *b = btk_spin_button_get_value_as_int (BTK_SPIN_BUTTON (spin2));
}

static void
set_size_callback (BtkWidget *widget,
                   gpointer   data)
{
  gint w, h;
  
  get_ints (data, &w, &h);

  btk_window_resize (BTK_WINDOW (g_object_get_data (data, "target")), w, h);
}

static void
unset_default_size_callback (BtkWidget *widget,
                             gpointer   data)
{
  btk_window_set_default_size (g_object_get_data (data, "target"),
                               -1, -1);
}

static void
set_default_size_callback (BtkWidget *widget,
                           gpointer   data)
{
  gint w, h;
  
  get_ints (data, &w, &h);

  btk_window_set_default_size (g_object_get_data (data, "target"),
                               w, h);
}

static void
unset_size_request_callback (BtkWidget *widget,
			     gpointer   data)
{
  btk_widget_set_size_request (g_object_get_data (data, "target"),
                               -1, -1);
}

static void
set_size_request_callback (BtkWidget *widget,
			   gpointer   data)
{
  gint w, h;
  
  get_ints (data, &w, &h);

  btk_widget_set_size_request (g_object_get_data (data, "target"),
                               w, h);
}

static void
set_location_callback (BtkWidget *widget,
                       gpointer   data)
{
  gint x, y;
  
  get_ints (data, &x, &y);

  btk_window_move (g_object_get_data (data, "target"), x, y);
}

static void
move_to_position_callback (BtkWidget *widget,
                           gpointer   data)
{
  gint x, y;
  BtkWindow *window;

  window = g_object_get_data (data, "target");
  
  btk_window_get_position (window, &x, &y);

  btk_window_move (window, x, y);
}

static void
set_geometry_callback (BtkWidget *entry,
                       gpointer   data)
{
  gchar *text;
  BtkWindow *target;

  target = BTK_WINDOW (g_object_get_data (B_OBJECT (data), "target"));
  
  text = btk_editable_get_chars (BTK_EDITABLE (entry), 0, -1);

  if (!btk_window_parse_geometry (target, text))
    g_print ("Bad geometry string '%s'\n", text);

  g_free (text);
}

static void
allow_shrink_callback (BtkWidget *widget,
                       gpointer   data)
{
  g_object_set (g_object_get_data (data, "target"),
                "allow_shrink",
                BTK_TOGGLE_BUTTON (widget)->active,
                NULL);
}

static void
allow_grow_callback (BtkWidget *widget,
                     gpointer   data)
{
  g_object_set (g_object_get_data (data, "target"),
                "allow_grow",
                BTK_TOGGLE_BUTTON (widget)->active,
                NULL);
}

static void
gravity_selected (BtkWidget *widget,
                  gpointer   data)
{
  btk_window_set_gravity (BTK_WINDOW (g_object_get_data (data, "target")),
                          btk_option_menu_get_history (BTK_OPTION_MENU (widget)) + BDK_GRAVITY_NORTH_WEST);
}

static void
pos_selected (BtkWidget *widget,
              gpointer   data)
{
  btk_window_set_position (BTK_WINDOW (g_object_get_data (data, "target")),
                           btk_option_menu_get_history (BTK_OPTION_MENU (widget)) + BTK_WIN_POS_NONE);
}

static void
move_gravity_window_to_current_position (BtkWidget *widget,
                                         gpointer   data)
{
  gint x, y;
  BtkWindow *window;

  window = BTK_WINDOW (data);    
  
  btk_window_get_position (window, &x, &y);

  btk_window_move (window, x, y);
}

static void
get_screen_corner (BtkWindow *window,
                   gint      *x,
                   gint      *y)
{
  int w, h;
  BdkScreen * screen = btk_window_get_screen (window);
  
  btk_window_get_size (BTK_WINDOW (window), &w, &h);

  switch (btk_window_get_gravity (window))
    {
    case BDK_GRAVITY_SOUTH_EAST:
      *x = bdk_screen_get_width (screen) - w;
      *y = bdk_screen_get_height (screen) - h;
      break;

    case BDK_GRAVITY_NORTH_EAST:
      *x = bdk_screen_get_width (screen) - w;
      *y = 0;
      break;

    case BDK_GRAVITY_SOUTH_WEST:
      *x = 0;
      *y = bdk_screen_get_height (screen) - h;
      break;

    case BDK_GRAVITY_NORTH_WEST:
      *x = 0;
      *y = 0;
      break;
      
    case BDK_GRAVITY_SOUTH:
      *x = (bdk_screen_get_width (screen) - w) / 2;
      *y = bdk_screen_get_height (screen) - h;
      break;

    case BDK_GRAVITY_NORTH:
      *x = (bdk_screen_get_width (screen) - w) / 2;
      *y = 0;
      break;

    case BDK_GRAVITY_WEST:
      *x = 0;
      *y = (bdk_screen_get_height (screen) - h) / 2;
      break;

    case BDK_GRAVITY_EAST:
      *x = bdk_screen_get_width (screen) - w;
      *y = (bdk_screen_get_height (screen) - h) / 2;
      break;

    case BDK_GRAVITY_CENTER:
      *x = (bdk_screen_get_width (screen) - w) / 2;
      *y = (bdk_screen_get_height (screen) - h) / 2;
      break;

    case BDK_GRAVITY_STATIC:
      /* pick some random numbers */
      *x = 350;
      *y = 350;
      break;

    default:
      g_assert_not_reached ();
      break;
    }
}

static void
move_gravity_window_to_starting_position (BtkWidget *widget,
                                          gpointer   data)
{
  gint x, y;
  BtkWindow *window;

  window = BTK_WINDOW (data);    
  
  get_screen_corner (window,
                     &x, &y);
  
  btk_window_move (window, x, y);
}

static BtkWidget*
make_gravity_window (BtkWidget   *destroy_with,
                     BdkGravity   gravity,
                     const gchar *title)
{
  BtkWidget *window;
  BtkWidget *button;
  BtkWidget *vbox;
  int x, y;
  
  window = btk_window_new (BTK_WINDOW_TOPLEVEL);

  btk_window_set_screen (BTK_WINDOW (window),
			 btk_widget_get_screen (destroy_with));

  vbox = btk_vbox_new (FALSE, 0);
  btk_widget_show (vbox);
  
  btk_container_add (BTK_CONTAINER (window), vbox);
  btk_window_set_title (BTK_WINDOW (window), title);
  btk_window_set_gravity (BTK_WINDOW (window), gravity);

  g_signal_connect_object (destroy_with,
			   "destroy",
			   G_CALLBACK (btk_widget_destroy),
			   window,
			   G_CONNECT_SWAPPED);

  
  button = btk_button_new_with_mnemonic ("_Move to current position");

  g_signal_connect (button, "clicked",
                    G_CALLBACK (move_gravity_window_to_current_position),
                    window);

  btk_container_add (BTK_CONTAINER (vbox), button);
  btk_widget_show (button);

  button = btk_button_new_with_mnemonic ("Move to _starting position");

  g_signal_connect (button, "clicked",
                    G_CALLBACK (move_gravity_window_to_starting_position),
                    window);

  btk_container_add (BTK_CONTAINER (vbox), button);
  btk_widget_show (button);
  
  /* Pretend this is the result of --geometry.
   * DO NOT COPY THIS CODE unless you are setting --geometry results,
   * and in that case you probably should just use btk_window_parse_geometry().
   * AGAIN, DO NOT SET BDK_HINT_USER_POS! It violates the ICCCM unless
   * you are parsing --geometry or equivalent.
   */
  btk_window_set_geometry_hints (BTK_WINDOW (window),
                                 NULL, NULL,
                                 BDK_HINT_USER_POS);

  btk_window_set_default_size (BTK_WINDOW (window),
                               200, 200);

  get_screen_corner (BTK_WINDOW (window), &x, &y);
  
  btk_window_move (BTK_WINDOW (window),
                   x, y);
  
  return window;
}

static void
do_gravity_test (BtkWidget *widget,
                 gpointer   data)
{
  BtkWidget *destroy_with = data;
  BtkWidget *window;
  
  /* We put a window at each gravity point on the screen. */
  window = make_gravity_window (destroy_with, BDK_GRAVITY_NORTH_WEST,
                                "NorthWest");
  btk_widget_show (window);
  
  window = make_gravity_window (destroy_with, BDK_GRAVITY_SOUTH_EAST,
                                "SouthEast");
  btk_widget_show (window);

  window = make_gravity_window (destroy_with, BDK_GRAVITY_NORTH_EAST,
                                "NorthEast");
  btk_widget_show (window);

  window = make_gravity_window (destroy_with, BDK_GRAVITY_SOUTH_WEST,
                                "SouthWest");
  btk_widget_show (window);

  window = make_gravity_window (destroy_with, BDK_GRAVITY_SOUTH,
                                "South");
  btk_widget_show (window);

  window = make_gravity_window (destroy_with, BDK_GRAVITY_NORTH,
                                "North");
  btk_widget_show (window);

  
  window = make_gravity_window (destroy_with, BDK_GRAVITY_WEST,
                                "West");
  btk_widget_show (window);

    
  window = make_gravity_window (destroy_with, BDK_GRAVITY_EAST,
                                "East");
  btk_widget_show (window);

  window = make_gravity_window (destroy_with, BDK_GRAVITY_CENTER,
                                "Center");
  btk_widget_show (window);

  window = make_gravity_window (destroy_with, BDK_GRAVITY_STATIC,
                                "Static");
  btk_widget_show (window);
}

static BtkWidget*
window_controls (BtkWidget *window)
{
  BtkWidget *control_window;
  BtkWidget *label;
  BtkWidget *vbox;
  BtkWidget *button;
  BtkWidget *spin;
  BtkAdjustment *adj;
  BtkWidget *entry;
  BtkWidget *om;
  gint i;
  
  control_window = btk_window_new (BTK_WINDOW_TOPLEVEL);

  btk_window_set_screen (BTK_WINDOW (control_window),
			 btk_widget_get_screen (window));

  btk_window_set_title (BTK_WINDOW (control_window), "Size controls");
  
  g_object_set_data (B_OBJECT (control_window),
                     "target",
                     window);
  
  g_signal_connect_object (control_window,
			   "destroy",
			   G_CALLBACK (btk_widget_destroy),
                           window,
			   G_CONNECT_SWAPPED);

  vbox = btk_vbox_new (FALSE, 5);
  
  btk_container_add (BTK_CONTAINER (control_window), vbox);
  
  label = btk_label_new ("<no configure events>");
  btk_box_pack_start (BTK_BOX (vbox), label, FALSE, FALSE, 0);
  
  g_signal_connect (window,
		    "configure_event",
		    G_CALLBACK (configure_event_callback),
		    label);

  adj = (BtkAdjustment *) btk_adjustment_new (10.0, -2000.0, 2000.0, 1.0,
                                              5.0, 0.0);
  spin = btk_spin_button_new (adj, 0, 0);

  btk_box_pack_start (BTK_BOX (vbox), spin, FALSE, FALSE, 0);

  g_object_set_data (B_OBJECT (control_window), "spin1", spin);

  adj = (BtkAdjustment *) btk_adjustment_new (10.0, -2000.0, 2000.0, 1.0,
                                              5.0, 0.0);
  spin = btk_spin_button_new (adj, 0, 0);

  btk_box_pack_start (BTK_BOX (vbox), spin, FALSE, FALSE, 0);

  g_object_set_data (B_OBJECT (control_window), "spin2", spin);

  entry = btk_entry_new ();
  btk_box_pack_start (BTK_BOX (vbox), entry, FALSE, FALSE, 0);

  g_signal_connect (entry, "changed",
		    G_CALLBACK (set_geometry_callback),
		    control_window);

  button = btk_button_new_with_label ("Show gravity test windows");
  g_signal_connect_swapped (button,
			    "clicked",
			    G_CALLBACK (do_gravity_test),
			    control_window);
  btk_box_pack_end (BTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = btk_button_new_with_label ("Reshow with initial size");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (btk_window_reshow_with_initial_size),
			   window,
			   G_CONNECT_SWAPPED);
  btk_box_pack_end (BTK_BOX (vbox), button, FALSE, FALSE, 0);
  
  button = btk_button_new_with_label ("Queue resize");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (btk_widget_queue_resize),
			   window,
			   G_CONNECT_SWAPPED);
  btk_box_pack_end (BTK_BOX (vbox), button, FALSE, FALSE, 0);
  
  button = btk_button_new_with_label ("Resize");
  g_signal_connect (button,
		    "clicked",
		    G_CALLBACK (set_size_callback),
		    control_window);
  btk_box_pack_end (BTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = btk_button_new_with_label ("Set default size");
  g_signal_connect (button,
		    "clicked",
		    G_CALLBACK (set_default_size_callback),
		    control_window);
  btk_box_pack_end (BTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = btk_button_new_with_label ("Unset default size");
  g_signal_connect (button,
		    "clicked",
		    G_CALLBACK (unset_default_size_callback),
                    control_window);
  btk_box_pack_end (BTK_BOX (vbox), button, FALSE, FALSE, 0);
  
  button = btk_button_new_with_label ("Set size request");
  g_signal_connect (button,
		    "clicked",
		    G_CALLBACK (set_size_request_callback),
		    control_window);
  btk_box_pack_end (BTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = btk_button_new_with_label ("Unset size request");
  g_signal_connect (button,
		    "clicked",
		    G_CALLBACK (unset_size_request_callback),
                    control_window);
  btk_box_pack_end (BTK_BOX (vbox), button, FALSE, FALSE, 0);
  
  button = btk_button_new_with_label ("Move");
  g_signal_connect (button,
		    "clicked",
		    G_CALLBACK (set_location_callback),
		    control_window);
  btk_box_pack_end (BTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = btk_button_new_with_label ("Move to current position");
  g_signal_connect (button,
		    "clicked",
		    G_CALLBACK (move_to_position_callback),
		    control_window);
  btk_box_pack_end (BTK_BOX (vbox), button, FALSE, FALSE, 0);
  
  button = btk_check_button_new_with_label ("Allow shrink");
  btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (button), FALSE);
  g_signal_connect (button,
		    "toggled",
		    G_CALLBACK (allow_shrink_callback),
		    control_window);
  btk_box_pack_end (BTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = btk_check_button_new_with_label ("Allow grow");
  btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (button), TRUE);
  g_signal_connect (button,
		    "toggled",
		    G_CALLBACK (allow_grow_callback),
                    control_window);
  btk_box_pack_end (BTK_BOX (vbox), button, FALSE, FALSE, 0);
  
  button = btk_button_new_with_mnemonic ("_Show");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (btk_widget_show),
			   window,
			   G_CONNECT_SWAPPED);
  btk_box_pack_end (BTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = btk_button_new_with_mnemonic ("_Hide");
  g_signal_connect_object (button,
			   "clicked",
			   G_CALLBACK (btk_widget_hide),
                           window,
			   G_CONNECT_SWAPPED);
  btk_box_pack_end (BTK_BOX (vbox), button, FALSE, FALSE, 0);

  om = btk_combo_box_text_new ();
  i = 0;
  while (i < 10)
    {
      static gchar *names[] = {
        "BDK_GRAVITY_NORTH_WEST",
        "BDK_GRAVITY_NORTH",
        "BDK_GRAVITY_NORTH_EAST",
        "BDK_GRAVITY_WEST",
        "BDK_GRAVITY_CENTER",
        "BDK_GRAVITY_EAST",
        "BDK_GRAVITY_SOUTH_WEST",
        "BDK_GRAVITY_SOUTH",
        "BDK_GRAVITY_SOUTH_EAST",
        "BDK_GRAVITY_STATIC",
        NULL
      };

      g_assert (names[i]);
      btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (om), names[i]);

      ++i;
    }
  
  g_signal_connect (om,
		    "changed",
		    G_CALLBACK (gravity_selected),
		    control_window);

  btk_box_pack_end (BTK_BOX (vbox), om, FALSE, FALSE, 0);


  om = btk_combo_box_text_new ();
  i = 0;
  while (i < 5)
    {
      static gchar *names[] = {
        "BTK_WIN_POS_NONE",
        "BTK_WIN_POS_CENTER",
        "BTK_WIN_POS_MOUSE",
        "BTK_WIN_POS_CENTER_ALWAYS",
        "BTK_WIN_POS_CENTER_ON_PARENT",
        NULL
      };

      g_assert (names[i]);
      btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (om), names[i]);

      ++i;
    }
  
  g_signal_connect (om,
		    "changed",
		    G_CALLBACK (pos_selected),
		    control_window);

  btk_box_pack_end (BTK_BOX (vbox), om, FALSE, FALSE, 0);
  
  btk_widget_show_all (vbox);
  
  return control_window;
}

void
create_window_sizing (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  static BtkWidget *target_window = NULL;

  if (!target_window)
    {
      BtkWidget *label;
      
      target_window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_screen (BTK_WINDOW (target_window),
			     btk_widget_get_screen (widget));
      label = btk_label_new (NULL);
      btk_label_set_markup (BTK_LABEL (label), "<span foreground=\"purple\"><big>Window being resized</big></span>\nBlah blah blah blah\nblah blah blah\nblah blah blah blah blah");
      btk_container_add (BTK_CONTAINER (target_window), label);
      btk_widget_show (label);
      
      g_signal_connect (target_window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&target_window);

      window = window_controls (target_window);
      
      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window);
      
      btk_window_set_title (BTK_WINDOW (target_window), "Window to size");
    }

  /* don't show target window by default, we want to allow testing
   * of behavior on first show.
   */
  
  if (!btk_widget_get_visible (window))
    btk_widget_show (window);
  else
    btk_widget_destroy (window);
}

/*
 * BtkProgressBar
 */

typedef struct _ProgressData {
  BtkWidget *window;
  BtkWidget *pbar;
  BtkWidget *block_spin;
  BtkWidget *x_align_spin;
  BtkWidget *y_align_spin;
  BtkWidget *step_spin;
  BtkWidget *act_blocks_spin;
  BtkWidget *label;
  BtkWidget *omenu1;
  BtkWidget *elmenu;
  BtkWidget *omenu2;
  BtkWidget *entry;
  int timer;
} ProgressData;

gint
progress_timeout (gpointer data)
{
  gdouble new_val;
  BtkAdjustment *adj;

  adj = BTK_PROGRESS (data)->adjustment;

  new_val = adj->value + 1;
  if (new_val > adj->upper)
    new_val = adj->lower;

  btk_progress_set_value (BTK_PROGRESS (data), new_val);

  return TRUE;
}

static void
destroy_progress (BtkWidget     *widget,
		  ProgressData **pdata)
{
  btk_timeout_remove ((*pdata)->timer);
  (*pdata)->timer = 0;
  (*pdata)->window = NULL;
  g_free (*pdata);
  *pdata = NULL;
}

static void
progressbar_toggle_orientation (BtkWidget *widget, gpointer data)
{
  ProgressData *pdata;
  gint i;

  pdata = (ProgressData *) data;

  if (!btk_widget_get_mapped (widget))
    return;

  i = btk_option_menu_get_history (BTK_OPTION_MENU (widget));

  btk_progress_bar_set_orientation (BTK_PROGRESS_BAR (pdata->pbar),
				    (BtkProgressBarOrientation) i);
}

static void
toggle_show_text (BtkWidget *widget, ProgressData *pdata)
{
  btk_progress_set_show_text (BTK_PROGRESS (pdata->pbar),
			      BTK_TOGGLE_BUTTON (widget)->active);
  btk_widget_set_sensitive (pdata->entry, BTK_TOGGLE_BUTTON (widget)->active);
  btk_widget_set_sensitive (pdata->x_align_spin,
			    BTK_TOGGLE_BUTTON (widget)->active);
  btk_widget_set_sensitive (pdata->y_align_spin,
			    BTK_TOGGLE_BUTTON (widget)->active);
}

static void
progressbar_toggle_ellipsize (BtkWidget *widget,
                              gpointer   data)
{
  ProgressData *pdata = data;
  if (btk_widget_is_drawable (widget))
    {
      gint i = btk_option_menu_get_history (BTK_OPTION_MENU (widget));
      btk_progress_bar_set_ellipsize (BTK_PROGRESS_BAR (pdata->pbar), i);
    }
}

static void
progressbar_toggle_bar_style (BtkWidget *widget, gpointer data)
{
  ProgressData *pdata;
  gint i;

  pdata = (ProgressData *) data;

  if (!btk_widget_get_mapped (widget))
    return;

  i = btk_option_menu_get_history (BTK_OPTION_MENU (widget));

  if (i == 1)
    btk_widget_set_sensitive (pdata->block_spin, TRUE);
  else
    btk_widget_set_sensitive (pdata->block_spin, FALSE);
  
  btk_progress_bar_set_bar_style (BTK_PROGRESS_BAR (pdata->pbar),
				  (BtkProgressBarStyle) i);
}

static void
progress_value_changed (BtkAdjustment *adj, ProgressData *pdata)
{
  char buf[20];

  if (BTK_PROGRESS (pdata->pbar)->activity_mode)
    sprintf (buf, "???");
  else
    sprintf (buf, "%.0f%%", 100 *
	     btk_progress_get_current_percentage (BTK_PROGRESS (pdata->pbar)));
  btk_label_set_text (BTK_LABEL (pdata->label), buf);
}

static void
adjust_blocks (BtkAdjustment *adj, ProgressData *pdata)
{
  btk_progress_set_percentage (BTK_PROGRESS (pdata->pbar), 0);
  btk_progress_bar_set_discrete_blocks (BTK_PROGRESS_BAR (pdata->pbar),
     btk_spin_button_get_value_as_int (BTK_SPIN_BUTTON (pdata->block_spin)));
}

static void
adjust_step (BtkAdjustment *adj, ProgressData *pdata)
{
  btk_progress_bar_set_activity_step (BTK_PROGRESS_BAR (pdata->pbar),
     btk_spin_button_get_value_as_int (BTK_SPIN_BUTTON (pdata->step_spin)));
}

static void
adjust_act_blocks (BtkAdjustment *adj, ProgressData *pdata)
{
  btk_progress_bar_set_activity_blocks (BTK_PROGRESS_BAR (pdata->pbar),
               btk_spin_button_get_value_as_int 
		      (BTK_SPIN_BUTTON (pdata->act_blocks_spin)));
}

static void
adjust_align (BtkAdjustment *adj, ProgressData *pdata)
{
  btk_progress_set_text_alignment (BTK_PROGRESS (pdata->pbar),
	 btk_spin_button_get_value (BTK_SPIN_BUTTON (pdata->x_align_spin)),
	 btk_spin_button_get_value (BTK_SPIN_BUTTON (pdata->y_align_spin)));
}

static void
toggle_activity_mode (BtkWidget *widget, ProgressData *pdata)
{
  btk_progress_set_activity_mode (BTK_PROGRESS (pdata->pbar),
				  BTK_TOGGLE_BUTTON (widget)->active);
  btk_widget_set_sensitive (pdata->step_spin, 
			    BTK_TOGGLE_BUTTON (widget)->active);
  btk_widget_set_sensitive (pdata->act_blocks_spin, 
			    BTK_TOGGLE_BUTTON (widget)->active);
}

static void
entry_changed (BtkWidget *widget, ProgressData *pdata)
{
  btk_progress_set_format_string (BTK_PROGRESS (pdata->pbar),
			  btk_entry_get_text (BTK_ENTRY (pdata->entry)));
}

void
create_progress_bar (BtkWidget *widget)
{
  BtkWidget *button;
  BtkWidget *vbox;
  BtkWidget *vbox2;
  BtkWidget *hbox;
  BtkWidget *check;
  BtkWidget *frame;
  BtkWidget *tab;
  BtkWidget *label;
  BtkWidget *align;
  BtkAdjustment *adj;
  static ProgressData *pdata = NULL;

  static gchar *items1[] =
  {
    "Left-Right",
    "Right-Left",
    "Bottom-Top",
    "Top-Bottom"
  };

  static gchar *items2[] =
  {
    "Continuous",
    "Discrete"
  };

  static char *ellipsize_items[] = {
    "None",     // BANGO_ELLIPSIZE_NONE,
    "Start",    // BANGO_ELLIPSIZE_START,
    "Middle",   // BANGO_ELLIPSIZE_MIDDLE,
    "End",      // BANGO_ELLIPSIZE_END
  };
  
  if (!pdata)
    pdata = g_new0 (ProgressData, 1);

  if (!pdata->window)
    {
      pdata->window = btk_dialog_new ();

      btk_window_set_screen (BTK_WINDOW (pdata->window),
			     btk_widget_get_screen (widget));

      btk_window_set_resizable (BTK_WINDOW (pdata->window), TRUE);

      g_signal_connect (pdata->window, "destroy",
			G_CALLBACK (destroy_progress),
			&pdata);

      pdata->timer = 0;

      btk_window_set_title (BTK_WINDOW (pdata->window), "BtkProgressBar");
      btk_container_set_border_width (BTK_CONTAINER (pdata->window), 0);

      vbox = btk_vbox_new (FALSE, 5);
      btk_container_set_border_width (BTK_CONTAINER (vbox), 10);
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (pdata->window)->vbox), 
			  vbox, FALSE, TRUE, 0);

      frame = btk_frame_new ("Progress");
      btk_box_pack_start (BTK_BOX (vbox), frame, FALSE, TRUE, 0);

      vbox2 = btk_vbox_new (FALSE, 5);
      btk_container_add (BTK_CONTAINER (frame), vbox2);

      align = btk_alignment_new (0.5, 0.5, 0, 0);
      btk_box_pack_start (BTK_BOX (vbox2), align, FALSE, FALSE, 5);

      adj = (BtkAdjustment *) btk_adjustment_new (0, 1, 300, 0, 0, 0);
      g_signal_connect (adj, "value_changed",
			G_CALLBACK (progress_value_changed), pdata);

      pdata->pbar = g_object_new (BTK_TYPE_PROGRESS_BAR,
				    "adjustment", adj,
				    "ellipsize", BANGO_ELLIPSIZE_MIDDLE,
				    NULL);
      btk_progress_set_format_string (BTK_PROGRESS (pdata->pbar),
				      "%v from [%l,%u] (=%p%%)");
      btk_container_add (BTK_CONTAINER (align), pdata->pbar);
      pdata->timer = btk_timeout_add (100, progress_timeout, pdata->pbar);

      align = btk_alignment_new (0.5, 0.5, 0, 0);
      btk_box_pack_start (BTK_BOX (vbox2), align, FALSE, FALSE, 5);

      hbox = btk_hbox_new (FALSE, 5);
      btk_container_add (BTK_CONTAINER (align), hbox);
      label = btk_label_new ("Label updated by user :"); 
      btk_box_pack_start (BTK_BOX (hbox), label, FALSE, TRUE, 0);
      pdata->label = btk_label_new ("");
      btk_box_pack_start (BTK_BOX (hbox), pdata->label, FALSE, TRUE, 0);

      frame = btk_frame_new ("Options");
      btk_box_pack_start (BTK_BOX (vbox), frame, FALSE, TRUE, 0);

      vbox2 = btk_vbox_new (FALSE, 5);
      btk_container_add (BTK_CONTAINER (frame), vbox2);

      tab = btk_table_new (7, 2, FALSE);
      btk_box_pack_start (BTK_BOX (vbox2), tab, FALSE, TRUE, 0);

      label = btk_label_new ("Orientation :");
      btk_table_attach (BTK_TABLE (tab), label, 0, 1, 0, 1,
			BTK_EXPAND | BTK_FILL, BTK_EXPAND | BTK_FILL,
			5, 5);
      btk_misc_set_alignment (BTK_MISC (label), 0, 0.5);

      pdata->omenu1 = build_option_menu (items1, 4, 0,
					 progressbar_toggle_orientation,
					 pdata);
      hbox = btk_hbox_new (FALSE, 0);
      btk_table_attach (BTK_TABLE (tab), hbox, 1, 2, 0, 1,
			BTK_EXPAND | BTK_FILL, BTK_EXPAND | BTK_FILL,
			5, 5);
      btk_box_pack_start (BTK_BOX (hbox), pdata->omenu1, TRUE, TRUE, 0);
      
      check = btk_check_button_new_with_label ("Show text");
      g_signal_connect (check, "clicked",
			G_CALLBACK (toggle_show_text),
			pdata);
      btk_table_attach (BTK_TABLE (tab), check, 0, 1, 1, 2,
			BTK_EXPAND | BTK_FILL, BTK_EXPAND | BTK_FILL,
			5, 5);

      hbox = btk_hbox_new (FALSE, 0);
      btk_table_attach (BTK_TABLE (tab), hbox, 1, 2, 1, 2,
			BTK_EXPAND | BTK_FILL, BTK_EXPAND | BTK_FILL,
			5, 5);

      label = btk_label_new ("Format : ");
      btk_box_pack_start (BTK_BOX (hbox), label, FALSE, TRUE, 0);

      pdata->entry = btk_entry_new ();
      g_signal_connect (pdata->entry, "changed",
			G_CALLBACK (entry_changed),
			pdata);
      btk_box_pack_start (BTK_BOX (hbox), pdata->entry, TRUE, TRUE, 0);
      btk_entry_set_text (BTK_ENTRY (pdata->entry), "%v from [%l,%u] (=%p%%)");
      btk_widget_set_size_request (pdata->entry, 100, -1);
      btk_widget_set_sensitive (pdata->entry, FALSE);

      label = btk_label_new ("Text align :");
      btk_table_attach (BTK_TABLE (tab), label, 0, 1, 2, 3,
			BTK_EXPAND | BTK_FILL, BTK_EXPAND | BTK_FILL,
			5, 5);
      btk_misc_set_alignment (BTK_MISC (label), 0, 0.5);

      hbox = btk_hbox_new (FALSE, 0);
      btk_table_attach (BTK_TABLE (tab), hbox, 1, 2, 2, 3,
			BTK_EXPAND | BTK_FILL, BTK_EXPAND | BTK_FILL,
			5, 5);

      label = btk_label_new ("x :");
      btk_box_pack_start (BTK_BOX (hbox), label, FALSE, TRUE, 5);
      
      adj = (BtkAdjustment *) btk_adjustment_new (0.5, 0, 1, 0.1, 0.1, 0);
      pdata->x_align_spin = btk_spin_button_new (adj, 0, 1);
      g_signal_connect (adj, "value_changed",
			G_CALLBACK (adjust_align), pdata);
      btk_box_pack_start (BTK_BOX (hbox), pdata->x_align_spin, FALSE, TRUE, 0);
      btk_widget_set_sensitive (pdata->x_align_spin, FALSE);

      label = btk_label_new ("y :");
      btk_box_pack_start (BTK_BOX (hbox), label, FALSE, TRUE, 5);

      adj = (BtkAdjustment *) btk_adjustment_new (0.5, 0, 1, 0.1, 0.1, 0);
      pdata->y_align_spin = btk_spin_button_new (adj, 0, 1);
      g_signal_connect (adj, "value_changed",
			G_CALLBACK (adjust_align), pdata);
      btk_box_pack_start (BTK_BOX (hbox), pdata->y_align_spin, FALSE, TRUE, 0);
      btk_widget_set_sensitive (pdata->y_align_spin, FALSE);

      label = btk_label_new ("Ellipsize text :");
      btk_table_attach (BTK_TABLE (tab), label, 0, 1, 10, 11,
			BTK_EXPAND | BTK_FILL, BTK_EXPAND | BTK_FILL,
			5, 5);
      btk_misc_set_alignment (BTK_MISC (label), 0, 0.5);
      pdata->elmenu = build_option_menu (ellipsize_items,
                                         sizeof (ellipsize_items) / sizeof (ellipsize_items[0]),
                                         2, // BANGO_ELLIPSIZE_MIDDLE
					 progressbar_toggle_ellipsize,
					 pdata);
      hbox = btk_hbox_new (FALSE, 0);
      btk_table_attach (BTK_TABLE (tab), hbox, 1, 2, 10, 11,
			BTK_EXPAND | BTK_FILL, BTK_EXPAND | BTK_FILL,
			5, 5);
      btk_box_pack_start (BTK_BOX (hbox), pdata->elmenu, TRUE, TRUE, 0);

      label = btk_label_new ("Bar Style :");
      btk_table_attach (BTK_TABLE (tab), label, 0, 1, 13, 14,
			BTK_EXPAND | BTK_FILL, BTK_EXPAND | BTK_FILL,
			5, 5);
      btk_misc_set_alignment (BTK_MISC (label), 0, 0.5);

      pdata->omenu2 = build_option_menu	(items2, 2, 0,
					 progressbar_toggle_bar_style,
					 pdata);
      hbox = btk_hbox_new (FALSE, 0);
      btk_table_attach (BTK_TABLE (tab), hbox, 1, 2, 13, 14,
			BTK_EXPAND | BTK_FILL, BTK_EXPAND | BTK_FILL,
			5, 5);
      btk_box_pack_start (BTK_BOX (hbox), pdata->omenu2, TRUE, TRUE, 0);

      label = btk_label_new ("Block count :");
      btk_table_attach (BTK_TABLE (tab), label, 0, 1, 14, 15,
			BTK_EXPAND | BTK_FILL, BTK_EXPAND | BTK_FILL,
			5, 5);
      btk_misc_set_alignment (BTK_MISC (label), 0, 0.5);

      hbox = btk_hbox_new (FALSE, 0);
      btk_table_attach (BTK_TABLE (tab), hbox, 1, 2, 14, 15,
			BTK_EXPAND | BTK_FILL, BTK_EXPAND | BTK_FILL,
			5, 5);
      adj = (BtkAdjustment *) btk_adjustment_new (10, 2, 20, 1, 5, 0);
      pdata->block_spin = btk_spin_button_new (adj, 0, 0);
      g_signal_connect (adj, "value_changed",
			G_CALLBACK (adjust_blocks), pdata);
      btk_box_pack_start (BTK_BOX (hbox), pdata->block_spin, FALSE, TRUE, 0);
      btk_widget_set_sensitive (pdata->block_spin, FALSE);

      check = btk_check_button_new_with_label ("Activity mode");
      g_signal_connect (check, "clicked",
			G_CALLBACK (toggle_activity_mode), pdata);
      btk_table_attach (BTK_TABLE (tab), check, 0, 1, 15, 16,
			BTK_EXPAND | BTK_FILL, BTK_EXPAND | BTK_FILL,
			5, 5);

      hbox = btk_hbox_new (FALSE, 0);
      btk_table_attach (BTK_TABLE (tab), hbox, 1, 2, 15, 16,
			BTK_EXPAND | BTK_FILL, BTK_EXPAND | BTK_FILL,
			5, 5);
      label = btk_label_new ("Step size : ");
      btk_box_pack_start (BTK_BOX (hbox), label, FALSE, TRUE, 0);
      adj = (BtkAdjustment *) btk_adjustment_new (3, 1, 20, 1, 5, 0);
      pdata->step_spin = btk_spin_button_new (adj, 0, 0);
      g_signal_connect (adj, "value_changed",
			G_CALLBACK (adjust_step), pdata);
      btk_box_pack_start (BTK_BOX (hbox), pdata->step_spin, FALSE, TRUE, 0);
      btk_widget_set_sensitive (pdata->step_spin, FALSE);

      hbox = btk_hbox_new (FALSE, 0);
      btk_table_attach (BTK_TABLE (tab), hbox, 1, 2, 16, 17,
			BTK_EXPAND | BTK_FILL, BTK_EXPAND | BTK_FILL,
			5, 5);
      label = btk_label_new ("Blocks :     ");
      btk_box_pack_start (BTK_BOX (hbox), label, FALSE, TRUE, 0);
      adj = (BtkAdjustment *) btk_adjustment_new (5, 2, 10, 1, 5, 0);
      pdata->act_blocks_spin = btk_spin_button_new (adj, 0, 0);
      g_signal_connect (adj, "value_changed",
			G_CALLBACK (adjust_act_blocks), pdata);
      btk_box_pack_start (BTK_BOX (hbox), pdata->act_blocks_spin, FALSE, TRUE,
			  0);
      btk_widget_set_sensitive (pdata->act_blocks_spin, FALSE);

      button = btk_button_new_with_label ("close");
      g_signal_connect_swapped (button, "clicked",
				G_CALLBACK (btk_widget_destroy),
				pdata->window);
      btk_widget_set_can_default (button, TRUE);
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (pdata->window)->action_area), 
			  button, TRUE, TRUE, 0);
      btk_widget_grab_default (button);
    }

  if (!btk_widget_get_visible (pdata->window))
    btk_widget_show_all (pdata->window);
  else
    btk_widget_destroy (pdata->window);
}

/*
 * Properties
 */

typedef struct {
  int x;
  int y;
  gboolean found;
  gboolean first;
  BtkWidget *res_widget;
} FindWidgetData;

static void
find_widget (BtkWidget *widget, FindWidgetData *data)
{
  BtkAllocation new_allocation;
  gint x_offset = 0;
  gint y_offset = 0;

  new_allocation = widget->allocation;

  if (data->found || !btk_widget_get_mapped (widget))
    return;

  /* Note that in the following code, we only count the
   * position as being inside a WINDOW widget if it is inside
   * widget->window; points that are outside of widget->window
   * but within the allocation are not counted. This is consistent
   * with the way we highlight drag targets.
   */
  if (btk_widget_get_has_window (widget))
    {
      new_allocation.x = 0;
      new_allocation.y = 0;
    }
  
  if (widget->parent && !data->first)
    {
      BdkWindow *window = widget->window;
      while (window != widget->parent->window)
	{
	  gint tx, ty, twidth, theight;
	  bdk_drawable_get_size (window, &twidth, &theight);

	  if (new_allocation.x < 0)
	    {
	      new_allocation.width += new_allocation.x;
	      new_allocation.x = 0;
	    }
	  if (new_allocation.y < 0)
	    {
	      new_allocation.height += new_allocation.y;
	      new_allocation.y = 0;
	    }
	  if (new_allocation.x + new_allocation.width > twidth)
	    new_allocation.width = twidth - new_allocation.x;
	  if (new_allocation.y + new_allocation.height > theight)
	    new_allocation.height = theight - new_allocation.y;

	  bdk_window_get_position (window, &tx, &ty);
	  new_allocation.x += tx;
	  x_offset += tx;
	  new_allocation.y += ty;
	  y_offset += ty;
	  
	  window = bdk_window_get_parent (window);
	}
    }

  if ((data->x >= new_allocation.x) && (data->y >= new_allocation.y) &&
      (data->x < new_allocation.x + new_allocation.width) && 
      (data->y < new_allocation.y + new_allocation.height))
    {
      /* First, check if the drag is in a valid drop site in
       * one of our children 
       */
      if (BTK_IS_CONTAINER (widget))
	{
	  FindWidgetData new_data = *data;
	  
	  new_data.x -= x_offset;
	  new_data.y -= y_offset;
	  new_data.found = FALSE;
	  new_data.first = FALSE;
	  
	  btk_container_forall (BTK_CONTAINER (widget),
				(BtkCallback)find_widget,
				&new_data);
	  
	  data->found = new_data.found;
	  if (data->found)
	    data->res_widget = new_data.res_widget;
	}

      /* If not, and this widget is registered as a drop site, check to
       * emit "drag_motion" to check if we are actually in
       * a drop site.
       */
      if (!data->found)
	{
	  data->found = TRUE;
	  data->res_widget = widget;
	}
    }
}

static BtkWidget *
find_widget_at_pointer (BdkDisplay *display)
{
  BtkWidget *widget = NULL;
  BdkWindow *pointer_window;
  gint x, y;
  FindWidgetData data;
 
 pointer_window = bdk_display_get_window_at_pointer (display, NULL, NULL);
 
 if (pointer_window)
   {
     gpointer widget_ptr;

     bdk_window_get_user_data (pointer_window, &widget_ptr);
     widget = widget_ptr;
   }

 if (widget)
   {
     bdk_window_get_pointer (widget->window,
			     &x, &y, NULL);
     
     data.x = x;
     data.y = y;
     data.found = FALSE;
     data.first = TRUE;

     find_widget (widget, &data);
     if (data.found)
       return data.res_widget;
     return widget;
   }
 return NULL;
}

struct PropertiesData {
  BtkWidget **window;
  BdkCursor *cursor;
  gboolean in_query;
  gint handler;
};

static void
destroy_properties (BtkWidget             *widget,
		    struct PropertiesData *data)
{
  if (data->window)
    {
      *data->window = NULL;
      data->window = NULL;
    }

  if (data->cursor)
    {
      bdk_cursor_unref (data->cursor);
      data->cursor = NULL;
    }

  if (data->handler)
    {
      g_signal_handler_disconnect (widget, data->handler);
      data->handler = 0;
    }

  g_free (data);
}

static gint
property_query_event (BtkWidget	       *widget,
		      BdkEvent	       *event,
		      struct PropertiesData *data)
{
  BtkWidget *res_widget = NULL;

  if (!data->in_query)
    return FALSE;
  
  if (event->type == BDK_BUTTON_RELEASE)
    {
      btk_grab_remove (widget);
      bdk_display_pointer_ungrab (btk_widget_get_display (widget),
				  BDK_CURRENT_TIME);
      
      res_widget = find_widget_at_pointer (btk_widget_get_display (widget));
      if (res_widget)
	{
	  g_object_set_data (B_OBJECT (res_widget), "prop-editor-screen",
			     btk_widget_get_screen (widget));
	  create_prop_editor (B_OBJECT (res_widget), 0);
	}

      data->in_query = FALSE;
    }
  return FALSE;
}


static void
query_properties (BtkButton *button,
		  struct PropertiesData *data)
{
  gint failure;

  g_signal_connect (button, "event",
		    G_CALLBACK (property_query_event), data);


  if (!data->cursor)
    data->cursor = bdk_cursor_new_for_display (btk_widget_get_display (BTK_WIDGET (button)),
					       BDK_TARGET);
  
  failure = bdk_pointer_grab (BTK_WIDGET (button)->window,
			      TRUE,
			      BDK_BUTTON_RELEASE_MASK,
			      NULL,
			      data->cursor,
			      BDK_CURRENT_TIME);

  btk_grab_add (BTK_WIDGET (button));

  data->in_query = TRUE;
}

static void
create_properties (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  BtkWidget *button;
  BtkWidget *vbox;
  BtkWidget *label;
  struct PropertiesData *data;

  data = g_new (struct PropertiesData, 1);
  data->window = &window;
  data->in_query = FALSE;
  data->cursor = NULL;
  data->handler = 0;

  if (!window)
    {
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);

      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));      

      data->handler = g_signal_connect (window, "destroy",
					G_CALLBACK (destroy_properties),
					data);

      btk_window_set_title (BTK_WINDOW (window), "test properties");
      btk_container_set_border_width (BTK_CONTAINER (window), 10);

      vbox = btk_vbox_new (FALSE, 1);
      btk_container_add (BTK_CONTAINER (window), vbox);
            
      label = btk_label_new ("This is just a dumb test to test properties.\nIf you need a generic module, get GLE.");
      btk_box_pack_start (BTK_BOX (vbox), label, TRUE, TRUE, 0);
      
      button = btk_button_new_with_label ("Query properties");
      btk_box_pack_start (BTK_BOX (vbox), button, TRUE, TRUE, 0);
      g_signal_connect (button, "clicked",
			G_CALLBACK (query_properties),
			data);
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    btk_widget_destroy (window);
  
}

struct SnapshotData {
  BtkWidget *toplevel_button;
  BtkWidget **window;
  BdkCursor *cursor;
  gboolean in_query;
  gboolean is_toplevel;
  gint handler;
};

static void
destroy_snapshot_data (BtkWidget             *widget,
		       struct SnapshotData *data)
{
  if (*data->window)
    *data->window = NULL;
  
  if (data->cursor)
    {
      bdk_cursor_unref (data->cursor);
      data->cursor = NULL;
    }

  if (data->handler)
    {
      g_signal_handler_disconnect (widget, data->handler);
      data->handler = 0;
    }

  g_free (data);
}

static gint
snapshot_widget_event (BtkWidget	       *widget,
		       BdkEvent	       *event,
		       struct SnapshotData *data)
{
  BtkWidget *res_widget = NULL;

  if (!data->in_query)
    return FALSE;
  
  if (event->type == BDK_BUTTON_RELEASE)
    {
      btk_grab_remove (widget);
      bdk_display_pointer_ungrab (btk_widget_get_display (widget),
				  BDK_CURRENT_TIME);
      
      res_widget = find_widget_at_pointer (btk_widget_get_display (widget));
      if (data->is_toplevel && res_widget)
	res_widget = btk_widget_get_toplevel (res_widget);
      if (res_widget)
	{
	  BdkPixmap *pixmap;
	  BtkWidget *window, *image;

	  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
	  pixmap = btk_widget_get_snapshot (res_widget, NULL);
          btk_widget_realize (window);
          if (bdk_drawable_get_depth (window->window) != bdk_drawable_get_depth (pixmap))
            {
              /* this branch is needed to convert ARGB -> RGB */
              int width, height;
              BdkPixbuf *pixbuf;
              bdk_drawable_get_size (pixmap, &width, &height);
              pixbuf = bdk_pixbuf_get_from_drawable (NULL, pixmap,
                                                     btk_widget_get_colormap (res_widget),
                                                     0, 0,
                                                     0, 0,
                                                     width, height);
              image = btk_image_new_from_pixbuf (pixbuf);
              g_object_unref (pixbuf);
            }
          else
            image = btk_image_new_from_pixmap (pixmap, NULL);
	  btk_container_add (BTK_CONTAINER (window), image);
          g_object_unref (pixmap);
	  btk_widget_show_all (window);
	}

      data->in_query = FALSE;
    }
  return FALSE;
}


static void
snapshot_widget (BtkButton *button,
		 struct SnapshotData *data)
{
  gint failure;

  g_signal_connect (button, "event",
		    G_CALLBACK (snapshot_widget_event), data);

  data->is_toplevel = BTK_WIDGET (button) == data->toplevel_button;
  
  if (!data->cursor)
    data->cursor = bdk_cursor_new_for_display (btk_widget_get_display (BTK_WIDGET (button)),
					       BDK_TARGET);
  
  failure = bdk_pointer_grab (BTK_WIDGET (button)->window,
			      TRUE,
			      BDK_BUTTON_RELEASE_MASK,
			      NULL,
			      data->cursor,
			      BDK_CURRENT_TIME);

  btk_grab_add (BTK_WIDGET (button));

  data->in_query = TRUE;
}

static void
create_snapshot (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  BtkWidget *button;
  BtkWidget *vbox;
  struct SnapshotData *data;

  data = g_new (struct SnapshotData, 1);
  data->window = &window;
  data->in_query = FALSE;
  data->cursor = NULL;
  data->handler = 0;

  if (!window)
    {
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);

      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));      

      data->handler = g_signal_connect (window, "destroy",
					G_CALLBACK (destroy_snapshot_data),
					data);

      btk_window_set_title (BTK_WINDOW (window), "test snapshot");
      btk_container_set_border_width (BTK_CONTAINER (window), 10);

      vbox = btk_vbox_new (FALSE, 1);
      btk_container_add (BTK_CONTAINER (window), vbox);
            
      button = btk_button_new_with_label ("Snapshot widget");
      btk_box_pack_start (BTK_BOX (vbox), button, TRUE, TRUE, 0);
      g_signal_connect (button, "clicked",
			G_CALLBACK (snapshot_widget),
			data);
      
      button = btk_button_new_with_label ("Snapshot toplevel");
      data->toplevel_button = button;
      btk_box_pack_start (BTK_BOX (vbox), button, TRUE, TRUE, 0);
      g_signal_connect (button, "clicked",
			G_CALLBACK (snapshot_widget),
			data);
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    btk_widget_destroy (window);
  
}



/*
 * Color Preview
 */

static int color_idle = 0;

gint
color_idle_func (BtkWidget *preview)
{
  static int count = 1;
  guchar buf[768];
  int i, j, k;

  for (i = 0; i < 256; i++)
    {
      for (j = 0, k = 0; j < 256; j++)
	{
	  buf[k+0] = i + count;
	  buf[k+1] = 0;
	  buf[k+2] = j + count;
	  k += 3;
	}

      btk_preview_draw_row (BTK_PREVIEW (preview), buf, 0, i, 256);
    }

  count += 1;

  btk_widget_queue_draw (preview);
  bdk_window_process_updates (preview->window, TRUE);

  return TRUE;
}

static void
color_preview_destroy (BtkWidget  *widget,
		       BtkWidget **window)
{
  btk_idle_remove (color_idle);
  color_idle = 0;

  *window = NULL;
}

void
create_color_preview (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  BtkWidget *preview;
  guchar buf[768];
  int i, j, k;

  if (!window)
    {
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      
      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (color_preview_destroy),
			&window);

      btk_window_set_title (BTK_WINDOW (window), "test");
      btk_container_set_border_width (BTK_CONTAINER (window), 10);

      preview = btk_preview_new (BTK_PREVIEW_COLOR);
      btk_preview_size (BTK_PREVIEW (preview), 256, 256);
      btk_container_add (BTK_CONTAINER (window), preview);

      for (i = 0; i < 256; i++)
	{
	  for (j = 0, k = 0; j < 256; j++)
	    {
	      buf[k+0] = i;
	      buf[k+1] = 0;
	      buf[k+2] = j;
	      k += 3;
	    }

	  btk_preview_draw_row (BTK_PREVIEW (preview), buf, 0, i, 256);
	}

      color_idle = btk_idle_add ((BtkFunction) color_idle_func, preview);
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    btk_widget_destroy (window);
}

/*
 * Gray Preview
 */

static int gray_idle = 0;

gint
gray_idle_func (BtkWidget *preview)
{
  static int count = 1;
  guchar buf[256];
  int i, j;

  for (i = 0; i < 256; i++)
    {
      for (j = 0; j < 256; j++)
	buf[j] = i + j + count;

      btk_preview_draw_row (BTK_PREVIEW (preview), buf, 0, i, 256);
    }

  count += 1;

  btk_widget_draw (preview, NULL);

  return TRUE;
}

static void
gray_preview_destroy (BtkWidget  *widget,
		      BtkWidget **window)
{
  btk_idle_remove (gray_idle);
  gray_idle = 0;

  *window = NULL;
}

void
create_gray_preview (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  BtkWidget *preview;
  guchar buf[256];
  int i, j;

  if (!window)
    {
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);

      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (gray_preview_destroy),
			&window);

      btk_window_set_title (BTK_WINDOW (window), "test");
      btk_container_set_border_width (BTK_CONTAINER (window), 10);

      preview = btk_preview_new (BTK_PREVIEW_GRAYSCALE);
      btk_preview_size (BTK_PREVIEW (preview), 256, 256);
      btk_container_add (BTK_CONTAINER (window), preview);

      for (i = 0; i < 256; i++)
	{
	  for (j = 0; j < 256; j++)
	    buf[j] = i + j;

	  btk_preview_draw_row (BTK_PREVIEW (preview), buf, 0, i, 256);
	}

      gray_idle = btk_idle_add ((BtkFunction) gray_idle_func, preview);
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    btk_widget_destroy (window);
}


/*
 * Selection Test
 */

void
selection_test_received (BtkWidget *list, BtkSelectionData *data)
{
  BdkAtom *atoms;
  BtkWidget *list_item;
  GList *item_list;
  int i, l;

  if (data->length < 0)
    {
      g_print ("Selection retrieval failed\n");
      return;
    }
  if (data->type != BDK_SELECTION_TYPE_ATOM)
    {
      g_print ("Selection \"TARGETS\" was not returned as atoms!\n");
      return;
    }

  /* Clear out any current list items */

  btk_list_clear_items (BTK_LIST(list), 0, -1);

  /* Add new items to list */

  atoms = (BdkAtom *)data->data;

  item_list = NULL;
  l = data->length / sizeof (BdkAtom);
  for (i = 0; i < l; i++)
    {
      char *name;
      name = bdk_atom_name (atoms[i]);
      if (name != NULL)
	{
	  list_item = btk_list_item_new_with_label (name);
	  g_free (name);
	}
      else
	list_item = btk_list_item_new_with_label ("(bad atom)");

      btk_widget_show (list_item);
      item_list = g_list_append (item_list, list_item);
    }

  btk_list_append_items (BTK_LIST (list), item_list);

  return;
}

void
selection_test_get_targets (BtkWidget *widget, BtkWidget *list)
{
  static BdkAtom targets_atom = BDK_NONE;

  if (targets_atom == BDK_NONE)
    targets_atom = bdk_atom_intern ("TARGETS", FALSE);

  btk_selection_convert (list, BDK_SELECTION_PRIMARY, targets_atom,
			 BDK_CURRENT_TIME);
}

void
create_selection_test (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  BtkWidget *button;
  BtkWidget *vbox;
  BtkWidget *scrolled_win;
  BtkWidget *list;
  BtkWidget *label;

  if (!window)
    {
      window = btk_dialog_new ();
      
      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window);

      btk_window_set_title (BTK_WINDOW (window), "Selection Test");
      btk_container_set_border_width (BTK_CONTAINER (window), 0);

      /* Create the list */

      vbox = btk_vbox_new (FALSE, 5);
      btk_container_set_border_width (BTK_CONTAINER (vbox), 10);
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->vbox), vbox,
			  TRUE, TRUE, 0);

      label = btk_label_new ("Gets available targets for current selection");
      btk_box_pack_start (BTK_BOX (vbox), label, FALSE, FALSE, 0);

      scrolled_win = btk_scrolled_window_new (NULL, NULL);
      btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (scrolled_win),
				      BTK_POLICY_AUTOMATIC, 
				      BTK_POLICY_AUTOMATIC);
      btk_box_pack_start (BTK_BOX (vbox), scrolled_win, TRUE, TRUE, 0);
      btk_widget_set_size_request (scrolled_win, 100, 200);

      list = btk_list_new ();
      btk_scrolled_window_add_with_viewport (BTK_SCROLLED_WINDOW (scrolled_win), list);

      g_signal_connect (list, "selection_received",
			G_CALLBACK (selection_test_received), NULL);

      /* .. And create some buttons */
      button = btk_button_new_with_label ("Get Targets");
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->action_area),
			  button, TRUE, TRUE, 0);

      g_signal_connect (button, "clicked",
			G_CALLBACK (selection_test_get_targets), list);

      button = btk_button_new_with_label ("Quit");
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->action_area),
			  button, TRUE, TRUE, 0);

      g_signal_connect_swapped (button, "clicked",
				G_CALLBACK (btk_widget_destroy),
				window);
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    btk_widget_destroy (window);
}

/*
 * Gamma Curve
 */

void
create_gamma_curve (BtkWidget *widget)
{
  static BtkWidget *window = NULL, *curve;
  static int count = 0;
  gfloat vec[256];
  gint max;
  gint i;
  
  if (!window)
    {
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));

      btk_window_set_title (BTK_WINDOW (window), "test");
      btk_container_set_border_width (BTK_CONTAINER (window), 10);

      g_signal_connect (window, "destroy",
			G_CALLBACK(btk_widget_destroyed),
			&window);

      curve = btk_gamma_curve_new ();
      btk_container_add (BTK_CONTAINER (window), curve);
      btk_widget_show (curve);
    }

  max = 127 + (count % 2)*128;
  btk_curve_set_range (BTK_CURVE (BTK_GAMMA_CURVE (curve)->curve),
		       0, max, 0, max);
  for (i = 0; i < max; ++i)
    vec[i] = (127 / sqrt (max)) * sqrt (i);
  btk_curve_set_vector (BTK_CURVE (BTK_GAMMA_CURVE (curve)->curve),
			max, vec);

  if (!btk_widget_get_visible (window))
    btk_widget_show (window);
  else if (count % 4 == 3)
    {
      btk_widget_destroy (window);
      window = NULL;
    }

  ++count;
}

/*
 * Test scrolling
 */

static int scroll_test_pos = 0.0;

static gint
scroll_test_expose (BtkWidget *widget, BdkEventExpose *event,
		    BtkAdjustment *adj)
{
  gint i,j;
  gint imin, imax, jmin, jmax;
  bairo_t *cr;
  
  imin = (event->area.x) / 10;
  imax = (event->area.x + event->area.width + 9) / 10;

  jmin = ((int)adj->value + event->area.y) / 10;
  jmax = ((int)adj->value + event->area.y + event->area.height + 9) / 10;

  bdk_window_clear_area (widget->window,
			 event->area.x, event->area.y,
			 event->area.width, event->area.height);

  cr = bdk_bairo_create (widget->window);

  for (i=imin; i<imax; i++)
    for (j=jmin; j<jmax; j++)
      if ((i+j) % 2)
	bairo_rectangle (cr, 10*i, 10*j - (int)adj->value, 1+i%10, 1+j%10);

  bairo_fill (cr);

  bairo_destroy (cr);

  return TRUE;
}

static gint
scroll_test_scroll (BtkWidget *widget, BdkEventScroll *event,
		    BtkAdjustment *adj)
{
  gdouble new_value = adj->value + ((event->direction == BDK_SCROLL_UP) ?
				    -adj->page_increment / 2:
				    adj->page_increment / 2);
  new_value = CLAMP (new_value, adj->lower, adj->upper - adj->page_size);
  btk_adjustment_set_value (adj, new_value);  
  
  return TRUE;
}

static void
scroll_test_configure (BtkWidget *widget, BdkEventConfigure *event,
		       BtkAdjustment *adj)
{
  adj->page_increment = 0.9 * widget->allocation.height;
  adj->page_size = widget->allocation.height;

  g_signal_emit_by_name (adj, "changed");
}

static void
scroll_test_adjustment_changed (BtkAdjustment *adj, BtkWidget *widget)
{
  /* gint source_min = (int)adj->value - scroll_test_pos; */
  gint dy;

  dy = scroll_test_pos - (int)adj->value;
  scroll_test_pos = adj->value;

  if (!btk_widget_is_drawable (widget))
    return;
  bdk_window_scroll (widget->window, 0, dy);
  bdk_window_process_updates (widget->window, FALSE);
}


void
create_scroll_test (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  BtkWidget *hbox;
  BtkWidget *drawing_area;
  BtkWidget *scrollbar;
  BtkWidget *button;
  BtkAdjustment *adj;
  BdkGeometry geometry;
  BdkWindowHints geometry_mask;

  if (!window)
    {
      window = btk_dialog_new ();

      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window);

      btk_window_set_title (BTK_WINDOW (window), "Scroll Test");
      btk_container_set_border_width (BTK_CONTAINER (window), 0);

      hbox = btk_hbox_new (FALSE, 0);
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->vbox), hbox,
			  TRUE, TRUE, 0);
      btk_widget_show (hbox);

      drawing_area = btk_drawing_area_new ();
      btk_widget_set_size_request (drawing_area, 200, 200);
      btk_box_pack_start (BTK_BOX (hbox), drawing_area, TRUE, TRUE, 0);
      btk_widget_show (drawing_area);

      btk_widget_set_events (drawing_area, BDK_EXPOSURE_MASK | BDK_SCROLL_MASK);

      adj = BTK_ADJUSTMENT (btk_adjustment_new (0.0, 0.0, 1000.0, 1.0, 180.0, 200.0));
      scroll_test_pos = 0.0;

      scrollbar = btk_vscrollbar_new (adj);
      btk_box_pack_start (BTK_BOX (hbox), scrollbar, FALSE, FALSE, 0);
      btk_widget_show (scrollbar);

      g_signal_connect (drawing_area, "expose_event",
			G_CALLBACK (scroll_test_expose), adj);
      g_signal_connect (drawing_area, "configure_event",
			G_CALLBACK (scroll_test_configure), adj);
      g_signal_connect (drawing_area, "scroll_event",
			G_CALLBACK (scroll_test_scroll), adj);
      
      g_signal_connect (adj, "value_changed",
			G_CALLBACK (scroll_test_adjustment_changed),
			drawing_area);
      
      /* .. And create some buttons */

      button = btk_button_new_with_label ("Quit");
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->action_area),
			  button, TRUE, TRUE, 0);

      g_signal_connect_swapped (button, "clicked",
				G_CALLBACK (btk_widget_destroy),
				window);
      btk_widget_show (button);

      /* Set up gridded geometry */

      geometry_mask = BDK_HINT_MIN_SIZE | 
	               BDK_HINT_BASE_SIZE | 
	               BDK_HINT_RESIZE_INC;

      geometry.min_width = 20;
      geometry.min_height = 20;
      geometry.base_width = 0;
      geometry.base_height = 0;
      geometry.width_inc = 10;
      geometry.height_inc = 10;
      
      btk_window_set_geometry_hints (BTK_WINDOW (window),
			       drawing_area, &geometry, geometry_mask);
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show (window);
  else
    btk_widget_destroy (window);
}

/*
 * Timeout Test
 */

static int timer = 0;

gint
timeout_test (BtkWidget *label)
{
  static int count = 0;
  static char buffer[32];

  sprintf (buffer, "count: %d", ++count);
  btk_label_set_text (BTK_LABEL (label), buffer);

  return TRUE;
}

void
start_timeout_test (BtkWidget *widget,
		    BtkWidget *label)
{
  if (!timer)
    {
      timer = btk_timeout_add (100, (BtkFunction) timeout_test, label);
    }
}

void
stop_timeout_test (BtkWidget *widget,
		   gpointer   data)
{
  if (timer)
    {
      btk_timeout_remove (timer);
      timer = 0;
    }
}

void
destroy_timeout_test (BtkWidget  *widget,
		      BtkWidget **window)
{
  stop_timeout_test (NULL, NULL);

  *window = NULL;
}

void
create_timeout_test (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  BtkWidget *button;
  BtkWidget *label;

  if (!window)
    {
      window = btk_dialog_new ();

      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (destroy_timeout_test),
			&window);

      btk_window_set_title (BTK_WINDOW (window), "Timeout Test");
      btk_container_set_border_width (BTK_CONTAINER (window), 0);

      label = btk_label_new ("count: 0");
      btk_misc_set_padding (BTK_MISC (label), 10, 10);
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->vbox), 
			  label, TRUE, TRUE, 0);
      btk_widget_show (label);

      button = btk_button_new_with_label ("close");
      g_signal_connect_swapped (button, "clicked",
				G_CALLBACK (btk_widget_destroy),
				window);
      btk_widget_set_can_default (button, TRUE);
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->action_area), 
			  button, TRUE, TRUE, 0);
      btk_widget_grab_default (button);
      btk_widget_show (button);

      button = btk_button_new_with_label ("start");
      g_signal_connect (button, "clicked",
			G_CALLBACK(start_timeout_test),
			label);
      btk_widget_set_can_default (button, TRUE);
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->action_area), 
			  button, TRUE, TRUE, 0);
      btk_widget_show (button);

      button = btk_button_new_with_label ("stop");
      g_signal_connect (button, "clicked",
			G_CALLBACK (stop_timeout_test),
			NULL);
      btk_widget_set_can_default (button, TRUE);
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->action_area), 
			  button, TRUE, TRUE, 0);
      btk_widget_show (button);
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show (window);
  else
    btk_widget_destroy (window);
}

/*
 * Idle Test
 */

static int idle_id = 0;

static gint
idle_test (BtkWidget *label)
{
  static int count = 0;
  static char buffer[32];

  sprintf (buffer, "count: %d", ++count);
  btk_label_set_text (BTK_LABEL (label), buffer);

  return TRUE;
}

static void
start_idle_test (BtkWidget *widget,
		 BtkWidget *label)
{
  if (!idle_id)
    {
      idle_id = btk_idle_add ((BtkFunction) idle_test, label);
    }
}

static void
stop_idle_test (BtkWidget *widget,
		gpointer   data)
{
  if (idle_id)
    {
      btk_idle_remove (idle_id);
      idle_id = 0;
    }
}

static void
destroy_idle_test (BtkWidget  *widget,
		   BtkWidget **window)
{
  stop_idle_test (NULL, NULL);

  *window = NULL;
}

static void
toggle_idle_container (BObject *button,
		       BtkContainer *container)
{
  btk_container_set_resize_mode (container, GPOINTER_TO_INT (g_object_get_data (button, "user_data")));
}

static void
create_idle_test (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  BtkWidget *button;
  BtkWidget *label;
  BtkWidget *container;

  if (!window)
    {
      BtkWidget *button2;
      BtkWidget *frame;
      BtkWidget *box;

      window = btk_dialog_new ();

      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (destroy_idle_test),
			&window);

      btk_window_set_title (BTK_WINDOW (window), "Idle Test");
      btk_container_set_border_width (BTK_CONTAINER (window), 0);

      label = btk_label_new ("count: 0");
      btk_misc_set_padding (BTK_MISC (label), 10, 10);
      btk_widget_show (label);
      
      container =
	g_object_new (BTK_TYPE_HBOX,
			"visible", TRUE,
			/* "BtkContainer::child", g_object_new (BTK_TYPE_HBOX,
			 * "BtkWidget::visible", TRUE,
			 */
			 "child", label,
			/* NULL), */
			NULL);
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->vbox), 
			  container, TRUE, TRUE, 0);

      frame =
	g_object_new (BTK_TYPE_FRAME,
			"border_width", 5,
			"label", "Label Container",
			"visible", TRUE,
			"parent", BTK_DIALOG (window)->vbox,
			NULL);
      box =
	g_object_new (BTK_TYPE_VBOX,
			"visible", TRUE,
			"parent", frame,
			NULL);
      button =
	g_object_connect (g_object_new (BTK_TYPE_RADIO_BUTTON,
					  "label", "Resize-Parent",
					  "user_data", (void*)BTK_RESIZE_PARENT,
					  "visible", TRUE,
					  "parent", box,
					  NULL),
			  "signal::clicked", toggle_idle_container, container,
			  NULL);
      button = g_object_new (BTK_TYPE_RADIO_BUTTON,
			       "label", "Resize-Queue",
			       "user_data", (void*)BTK_RESIZE_QUEUE,
			       "group", button,
			       "visible", TRUE,
			       "parent", box,
			       NULL);
      g_object_connect (button,
			"signal::clicked", toggle_idle_container, container,
			NULL);
      button2 = g_object_new (BTK_TYPE_RADIO_BUTTON,
				"label", "Resize-Immediate",
				"user_data", (void*)BTK_RESIZE_IMMEDIATE,
				NULL);
      g_object_connect (button2,
			"signal::clicked", toggle_idle_container, container,
			NULL);
      g_object_set (button2,
		    "group", button,
		    "visible", TRUE,
		    "parent", box,
		    NULL);

      button = btk_button_new_with_label ("close");
      g_signal_connect_swapped (button, "clicked",
				G_CALLBACK (btk_widget_destroy),
				window);
      btk_widget_set_can_default (button, TRUE);
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->action_area), 
			  button, TRUE, TRUE, 0);
      btk_widget_grab_default (button);
      btk_widget_show (button);

      button = btk_button_new_with_label ("start");
      g_signal_connect (button, "clicked",
			G_CALLBACK (start_idle_test),
			label);
      btk_widget_set_can_default (button, TRUE);
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->action_area), 
			  button, TRUE, TRUE, 0);
      btk_widget_show (button);

      button = btk_button_new_with_label ("stop");
      g_signal_connect (button, "clicked",
			G_CALLBACK (stop_idle_test),
			NULL);
      btk_widget_set_can_default (button, TRUE);
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->action_area), 
			  button, TRUE, TRUE, 0);
      btk_widget_show (button);
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show (window);
  else
    btk_widget_destroy (window);
}

/*
 * rc file test
 */

void
reload_all_rc_files (void)
{
  static BdkAtom atom_rcfiles = BDK_NONE;

  BdkEvent *send_event = bdk_event_new (BDK_CLIENT_EVENT);
  int i;
  
  if (!atom_rcfiles)
    atom_rcfiles = bdk_atom_intern("_BTK_READ_RCFILES", FALSE);

  for(i = 0; i < 5; i++)
    send_event->client.data.l[i] = 0;
  send_event->client.data_format = 32;
  send_event->client.message_type = atom_rcfiles;
  bdk_event_send_clientmessage_toall (send_event);

  bdk_event_free (send_event);
}

void
create_rc_file (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  BtkWidget *button;
  BtkWidget *frame;
  BtkWidget *vbox;
  BtkWidget *label;

  if (!window)
    {
      window = btk_dialog_new ();

      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window);

      frame = btk_aspect_frame_new ("Testing RC file prioritization", 0.5, 0.5, 0.0, TRUE);
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->vbox), frame, FALSE, FALSE, 0);

      vbox = btk_vbox_new (FALSE, 0);
      btk_container_add (BTK_CONTAINER (frame), vbox);
      
      label = btk_label_new ("This label should be red");
      btk_widget_set_name (label, "testbtk-red-label");
      btk_box_pack_start (BTK_BOX (vbox), label, FALSE, FALSE, 0);

      label = btk_label_new ("This label should be green");
      btk_widget_set_name (label, "testbtk-green-label");
      btk_box_pack_start (BTK_BOX (vbox), label, FALSE, FALSE, 0);

      label = btk_label_new ("This label should be blue");
      btk_widget_set_name (label, "testbtk-blue-label");
      btk_box_pack_start (BTK_BOX (vbox), label, FALSE, FALSE, 0);

      btk_window_set_title (BTK_WINDOW (window), "Reload Rc file");
      btk_container_set_border_width (BTK_CONTAINER (window), 0);

      button = btk_button_new_with_label ("Reload");
      g_signal_connect (button, "clicked",
			G_CALLBACK (btk_rc_reparse_all), NULL);
      btk_widget_set_can_default (button, TRUE);
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->action_area), 
			  button, TRUE, TRUE, 0);
      btk_widget_grab_default (button);

      button = btk_button_new_with_label ("Reload All");
      g_signal_connect (button, "clicked",
			G_CALLBACK (reload_all_rc_files), NULL);
      btk_widget_set_can_default (button, TRUE);
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->action_area), 
			  button, TRUE, TRUE, 0);

      button = btk_button_new_with_label ("Close");
      g_signal_connect_swapped (button, "clicked",
				G_CALLBACK (btk_widget_destroy),
				window);
      btk_widget_set_can_default (button, TRUE);
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->action_area), 
			  button, TRUE, TRUE, 0);
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    btk_widget_destroy (window);
}

/*
 * Test of recursive mainloop
 */

void
mainloop_destroyed (BtkWidget *w, BtkWidget **window)
{
  *window = NULL;
  btk_main_quit ();
}

void
create_mainloop (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  BtkWidget *label;
  BtkWidget *button;

  if (!window)
    {
      window = btk_dialog_new ();

      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));

      btk_window_set_title (BTK_WINDOW (window), "Test Main Loop");

      g_signal_connect (window, "destroy",
			G_CALLBACK (mainloop_destroyed),
			&window);

      label = btk_label_new ("In recursive main loop...");
      btk_misc_set_padding (BTK_MISC(label), 20, 20);

      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->vbox), label,
			  TRUE, TRUE, 0);
      btk_widget_show (label);

      button = btk_button_new_with_label ("Leave");
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->action_area), button, 
			  FALSE, TRUE, 0);

      g_signal_connect_swapped (button, "clicked",
				G_CALLBACK (btk_widget_destroy),
				window);

      btk_widget_set_can_default (button, TRUE);
      btk_widget_grab_default (button);

      btk_widget_show (button);
    }

  if (!btk_widget_get_visible (window))
    {
      btk_widget_show (window);

      g_print ("create_mainloop: start\n");
      btk_main ();
      g_print ("create_mainloop: done\n");
    }
  else
    btk_widget_destroy (window);
}

gboolean
layout_expose_handler (BtkWidget *widget, BdkEventExpose *event)
{
  BtkLayout *layout;
  BdkWindow *bin_window;
  bairo_t *cr;

  gint i,j;
  gint imin, imax, jmin, jmax;

  layout = BTK_LAYOUT (widget);
  bin_window = btk_layout_get_bin_window (layout);

  if (event->window != bin_window)
    return FALSE;
  
  imin = (event->area.x) / 10;
  imax = (event->area.x + event->area.width + 9) / 10;

  jmin = (event->area.y) / 10;
  jmax = (event->area.y + event->area.height + 9) / 10;

  cr = bdk_bairo_create (bin_window);

  for (i=imin; i<imax; i++)
    for (j=jmin; j<jmax; j++)
      if ((i+j) % 2)
	bairo_rectangle (cr,
			 10*i, 10*j, 
			 1+i%10, 1+j%10);
  
  bairo_fill (cr);

  bairo_destroy (cr);

  return FALSE;
}

void create_layout (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  BtkWidget *layout;
  BtkWidget *scrolledwindow;
  BtkWidget *button;

  if (!window)
    {
      gchar buf[16];

      gint i, j;
      
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));

      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window);

      btk_window_set_title (BTK_WINDOW (window), "Layout");
      btk_widget_set_size_request (window, 200, 200);

      scrolledwindow = btk_scrolled_window_new (NULL, NULL);
      btk_scrolled_window_set_shadow_type (BTK_SCROLLED_WINDOW (scrolledwindow),
					   BTK_SHADOW_IN);
      btk_scrolled_window_set_placement (BTK_SCROLLED_WINDOW (scrolledwindow),
					 BTK_CORNER_TOP_RIGHT);

      btk_container_add (BTK_CONTAINER (window), scrolledwindow);
      
      layout = btk_layout_new (NULL, NULL);
      btk_container_add (BTK_CONTAINER (scrolledwindow), layout);

      /* We set step sizes here since BtkLayout does not set
       * them itself.
       */
      BTK_LAYOUT (layout)->hadjustment->step_increment = 10.0;
      BTK_LAYOUT (layout)->vadjustment->step_increment = 10.0;
      
      btk_widget_set_events (layout, BDK_EXPOSURE_MASK);
      g_signal_connect (layout, "expose_event",
			G_CALLBACK (layout_expose_handler), NULL);
      
      btk_layout_set_size (BTK_LAYOUT (layout), 1600, 128000);
      
      for (i=0 ; i < 16 ; i++)
	for (j=0 ; j < 16 ; j++)
	  {
	    sprintf(buf, "Button %d, %d", i, j);
	    if ((i + j) % 2)
	      button = btk_button_new_with_label (buf);
	    else
	      button = btk_label_new (buf);

	    btk_layout_put (BTK_LAYOUT (layout), button,
			    j*100, i*100);
	  }

      for (i=16; i < 1280; i++)
	{
	  sprintf(buf, "Button %d, %d", i, 0);
	  if (i % 2)
	    button = btk_button_new_with_label (buf);
	  else
	    button = btk_label_new (buf);

	  btk_layout_put (BTK_LAYOUT (layout), button,
			  0, i*100);
	}
    }

  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    btk_widget_destroy (window);
}

void
create_styles (BtkWidget *widget)
{
  static BtkWidget *window = NULL;
  BtkWidget *label;
  BtkWidget *button;
  BtkWidget *entry;
  BtkWidget *vbox;
  static BdkColor red =    { 0, 0xffff, 0,      0      };
  static BdkColor green =  { 0, 0,      0xffff, 0      };
  static BdkColor blue =   { 0, 0,      0,      0xffff };
  static BdkColor yellow = { 0, 0xffff, 0xffff, 0      };
  static BdkColor cyan =   { 0, 0     , 0xffff, 0xffff };
  BangoFontDescription *font_desc;

  BtkRcStyle *rc_style;

  if (!window)
    {
      window = btk_dialog_new ();
      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (widget));
     
      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window);

      
      button = btk_button_new_with_label ("Close");
      g_signal_connect_swapped (button, "clicked",
				G_CALLBACK (btk_widget_destroy),
				window);
      btk_widget_set_can_default (button, TRUE);
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->action_area), 
			  button, TRUE, TRUE, 0);
      btk_widget_show (button);

      vbox = btk_vbox_new (FALSE, 5);
      btk_container_set_border_width (BTK_CONTAINER (vbox), 10);
      btk_box_pack_start (BTK_BOX (BTK_DIALOG (window)->vbox), vbox, FALSE, FALSE, 0);
      
      label = btk_label_new ("Font:");
      btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
      btk_box_pack_start (BTK_BOX (vbox), label, FALSE, FALSE, 0);

      font_desc = bango_font_description_from_string ("Helvetica,Sans Oblique 18");

      button = btk_button_new_with_label ("Some Text");
      btk_widget_modify_font (BTK_BIN (button)->child, font_desc);
      btk_box_pack_start (BTK_BOX (vbox), button, FALSE, FALSE, 0);

      label = btk_label_new ("Foreground:");
      btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
      btk_box_pack_start (BTK_BOX (vbox), label, FALSE, FALSE, 0);

      button = btk_button_new_with_label ("Some Text");
      btk_widget_modify_fg (BTK_BIN (button)->child, BTK_STATE_NORMAL, &red);
      btk_box_pack_start (BTK_BOX (vbox), button, FALSE, FALSE, 0);

      label = btk_label_new ("Background:");
      btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
      btk_box_pack_start (BTK_BOX (vbox), label, FALSE, FALSE, 0);

      button = btk_button_new_with_label ("Some Text");
      btk_widget_modify_bg (button, BTK_STATE_NORMAL, &green);
      btk_box_pack_start (BTK_BOX (vbox), button, FALSE, FALSE, 0);

      label = btk_label_new ("Text:");
      btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
      btk_box_pack_start (BTK_BOX (vbox), label, FALSE, FALSE, 0);

      entry = btk_entry_new ();
      btk_entry_set_text (BTK_ENTRY (entry), "Some Text");
      btk_widget_modify_text (entry, BTK_STATE_NORMAL, &blue);
      btk_box_pack_start (BTK_BOX (vbox), entry, FALSE, FALSE, 0);

      label = btk_label_new ("Base:");
      btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
      btk_box_pack_start (BTK_BOX (vbox), label, FALSE, FALSE, 0);

      entry = btk_entry_new ();
      btk_entry_set_text (BTK_ENTRY (entry), "Some Text");
      btk_widget_modify_base (entry, BTK_STATE_NORMAL, &yellow);
      btk_box_pack_start (BTK_BOX (vbox), entry, FALSE, FALSE, 0);

      label = btk_label_new ("Cursor:");
      btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
      btk_box_pack_start (BTK_BOX (vbox), label, FALSE, FALSE, 0);

      entry = btk_entry_new ();
      btk_entry_set_text (BTK_ENTRY (entry), "Some Text");
      btk_widget_modify_cursor (entry, &red, &red);
      btk_box_pack_start (BTK_BOX (vbox), entry, FALSE, FALSE, 0);

      label = btk_label_new ("Multiple:");
      btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
      btk_box_pack_start (BTK_BOX (vbox), label, FALSE, FALSE, 0);

      button = btk_button_new_with_label ("Some Text");

      rc_style = btk_rc_style_new ();

      rc_style->font_desc = bango_font_description_copy (font_desc);
      rc_style->color_flags[BTK_STATE_NORMAL] = BTK_RC_FG | BTK_RC_BG;
      rc_style->color_flags[BTK_STATE_PRELIGHT] = BTK_RC_FG | BTK_RC_BG;
      rc_style->color_flags[BTK_STATE_ACTIVE] = BTK_RC_FG | BTK_RC_BG;
      rc_style->fg[BTK_STATE_NORMAL] = yellow;
      rc_style->bg[BTK_STATE_NORMAL] = blue;
      rc_style->fg[BTK_STATE_PRELIGHT] = blue;
      rc_style->bg[BTK_STATE_PRELIGHT] = yellow;
      rc_style->fg[BTK_STATE_ACTIVE] = red;
      rc_style->bg[BTK_STATE_ACTIVE] = cyan;
      rc_style->xthickness = 5;
      rc_style->ythickness = 5;

      btk_widget_modify_style (button, rc_style);
      btk_widget_modify_style (BTK_BIN (button)->child, rc_style);

      g_object_unref (rc_style);
      
      btk_box_pack_start (BTK_BOX (vbox), button, FALSE, FALSE, 0);
    }
  
  if (!btk_widget_get_visible (window))
    btk_widget_show_all (window);
  else
    btk_widget_destroy (window);
}

/*
 * Main Window and Exit
 */

void
do_exit (BtkWidget *widget, BtkWidget *window)
{
  btk_widget_destroy (window);
  btk_main_quit ();
}

struct {
  char *label;
  void (*func) (BtkWidget *widget);
  gboolean do_not_benchmark;
} buttons[] =
{
  { "alpha window", create_alpha_window },
  { "big windows", create_big_windows },
  { "button box", create_button_box },
  { "buttons", create_buttons },
  { "check buttons", create_check_buttons },
  { "clist", create_clist},
  { "color selection", create_color_selection },
  { "composited window", create_composited_window },
  { "ctree", create_ctree },
  { "cursors", create_cursors },
  { "dialog", create_dialog },
  { "display & screen", create_display_screen, TRUE },
  { "entry", create_entry },
  { "event box", create_event_box },
  { "event watcher", create_event_watcher },
  { "expander", create_expander },
  { "file selection", create_file_selection },
  { "flipping", create_flipping },
  { "focus", create_focus },
  { "font selection", create_font_selection },
  { "gamma curve", create_gamma_curve, TRUE },
  { "gridded geometry", create_gridded_geometry },
  { "handle box", create_handle_box },
  { "image", create_image },
  { "item factory", create_item_factory },
  { "key lookup", create_key_lookup },
  { "labels", create_labels },
  { "layout", create_layout },
  { "list", create_list },
  { "menus", create_menus },
  { "message dialog", create_message_dialog },
  { "modal window", create_modal_window, TRUE },
  { "notebook", create_notebook },
  { "panes", create_panes },
  { "paned keyboard", create_paned_keyboard_navigation },
  { "pixmap", create_pixmap },
  { "preview color", create_color_preview, TRUE },
  { "preview gray", create_gray_preview, TRUE },
  { "progress bar", create_progress_bar },
  { "properties", create_properties },
  { "radio buttons", create_radio_buttons },
  { "range controls", create_range_controls },
  { "rc file", create_rc_file },
  { "reparent", create_reparent },
  { "resize grips", create_resize_grips },
  { "rotated label", create_rotated_label },
  { "rotated text", create_rotated_text },
  { "rulers", create_rulers },
  { "saved position", create_saved_position },
  { "scrolled windows", create_scrolled_windows },
  { "shapes", create_shapes },
  { "size groups", create_size_groups },
  { "snapshot", create_snapshot },
  { "spinbutton", create_spins },
  { "statusbar", create_statusbar },
  { "styles", create_styles },
  { "test idle", create_idle_test },
  { "test mainloop", create_mainloop, TRUE },
  { "test scrolling", create_scroll_test },
  { "test selection", create_selection_test },
  { "test timeout", create_timeout_test },
  { "text", create_text },
  { "toggle buttons", create_toggle_buttons },
  { "toolbar", create_toolbar },
  { "tooltips", create_tooltips },
  { "tree", create_tree_mode_window},
  { "WM hints", create_wmhints },
  { "window sizing", create_window_sizing },
  { "window states", create_window_states }
};
int nbuttons = sizeof (buttons) / sizeof (buttons[0]);

void
create_main_window (void)
{
  BtkWidget *window;
  BtkWidget *box1;
  BtkWidget *box2;
  BtkWidget *scrolled_window;
  BtkWidget *button;
  BtkWidget *label;
  gchar buffer[64];
  BtkWidget *separator;
  BdkGeometry geometry;
  int i;

  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  btk_widget_set_name (window, "main window");
  btk_widget_set_uposition (window, 50, 20);
  btk_window_set_default_size (BTK_WINDOW (window), -1, 400);

  geometry.min_width = -1;
  geometry.min_height = -1;
  geometry.max_width = -1;
  geometry.max_height = G_MAXSHORT;
  btk_window_set_geometry_hints (BTK_WINDOW (window), NULL,
				 &geometry,
				 BDK_HINT_MIN_SIZE | BDK_HINT_MAX_SIZE);

  g_signal_connect (window, "destroy",
		    G_CALLBACK (btk_main_quit),
		    NULL);
  g_signal_connect (window, "delete-event",
		    G_CALLBACK (btk_false),
		    NULL);

  box1 = btk_vbox_new (FALSE, 0);
  btk_container_add (BTK_CONTAINER (window), box1);

  if (btk_micro_version > 0)
    sprintf (buffer,
	     "Btk+ v%d.%d.%d",
	     btk_major_version,
	     btk_minor_version,
	     btk_micro_version);
  else
    sprintf (buffer,
	     "Btk+ v%d.%d",
	     btk_major_version,
	     btk_minor_version);

  label = btk_label_new (buffer);
  btk_box_pack_start (BTK_BOX (box1), label, FALSE, FALSE, 0);
  btk_widget_set_name (label, "testbtk-version-label");

  scrolled_window = btk_scrolled_window_new (NULL, NULL);
  btk_container_set_border_width (BTK_CONTAINER (scrolled_window), 10);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (scrolled_window),
     		                  BTK_POLICY_NEVER, 
                                  BTK_POLICY_AUTOMATIC);
  btk_box_pack_start (BTK_BOX (box1), scrolled_window, TRUE, TRUE, 0);

  box2 = btk_vbox_new (FALSE, 0);
  btk_container_set_border_width (BTK_CONTAINER (box2), 10);
  btk_scrolled_window_add_with_viewport (BTK_SCROLLED_WINDOW (scrolled_window), box2);
  btk_container_set_focus_vadjustment (BTK_CONTAINER (box2),
				       btk_scrolled_window_get_vadjustment (BTK_SCROLLED_WINDOW (scrolled_window)));
  btk_widget_show (box2);

  for (i = 0; i < nbuttons; i++)
    {
      button = btk_button_new_with_label (buttons[i].label);
      if (buttons[i].func)
        g_signal_connect (button, 
			  "clicked", 
			  G_CALLBACK(buttons[i].func),
			  NULL);
      else
        btk_widget_set_sensitive (button, FALSE);
      btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);
    }

  separator = btk_hseparator_new ();
  btk_box_pack_start (BTK_BOX (box1), separator, FALSE, TRUE, 0);

  box2 = btk_vbox_new (FALSE, 10);
  btk_container_set_border_width (BTK_CONTAINER (box2), 10);
  btk_box_pack_start (BTK_BOX (box1), box2, FALSE, TRUE, 0);

  button = btk_button_new_with_mnemonic ("_Close");
  g_signal_connect (button, "clicked",
		    G_CALLBACK (do_exit),
		    window);
  btk_box_pack_start (BTK_BOX (box2), button, TRUE, TRUE, 0);
  btk_widget_set_can_default (button, TRUE);
  btk_widget_grab_default (button);

  btk_widget_show_all (window);
}

static void
test_init (void)
{
  if (g_file_test ("../bdk-pixbuf/libpixbufloader-pnm.la",
		   G_FILE_TEST_EXISTS))
    {
      g_setenv ("BDK_PIXBUF_MODULE_FILE", "../bdk-pixbuf/bdk-pixbuf.loaders", TRUE);
      g_setenv ("BTK_IM_MODULE_FILE", "../modules/input/immodules.cache", TRUE);
    }
}

static char *
pad (const char *str, int to)
{
  static char buf[256];
  int len = strlen (str);
  int i;

  for (i = 0; i < to; i++)
    buf[i] = ' ';

  buf[to] = '\0';

  memcpy (buf, str, len);

  return buf;
}

static void
bench_iteration (BtkWidget *widget, void (* fn) (BtkWidget *widget))
{
  fn (widget); /* on */
  while (g_main_context_iteration (NULL, FALSE));
  fn (widget); /* off */
  while (g_main_context_iteration (NULL, FALSE));
}

void
do_real_bench (BtkWidget *widget, void (* fn) (BtkWidget *widget), char *name, int num)
{
  GTimeVal tv0, tv1;
  double dt_first;
  double dt;
  int n;
  static gboolean printed_headers = FALSE;

  if (!printed_headers) {
    g_print ("Test                 Iters      First      Other\n");
    g_print ("-------------------- ----- ---------- ----------\n");
    printed_headers = TRUE;
  }

  g_get_current_time (&tv0);
  bench_iteration (widget, fn); 
  g_get_current_time (&tv1);

  dt_first = ((double)tv1.tv_sec - tv0.tv_sec) * 1000.0
	+ (tv1.tv_usec - tv0.tv_usec) / 1000.0;

  g_get_current_time (&tv0);
  for (n = 0; n < num - 1; n++)
    bench_iteration (widget, fn); 
  g_get_current_time (&tv1);
  dt = ((double)tv1.tv_sec - tv0.tv_sec) * 1000.0
	+ (tv1.tv_usec - tv0.tv_usec) / 1000.0;

  g_print ("%s %5d ", pad (name, 20), num);
  if (num > 1)
    g_print ("%10.1f %10.1f\n", dt_first, dt/(num-1));
  else
    g_print ("%10.1f\n", dt_first);
}

void
do_bench (char* what, int num)
{
  int i;
  BtkWidget *widget;
  void (* fn) (BtkWidget *widget);
  fn = NULL;
  widget = btk_window_new (BTK_WINDOW_TOPLEVEL);

  if (g_ascii_strcasecmp (what, "ALL") == 0)
    {
      for (i = 0; i < nbuttons; i++)
	{
	  if (!buttons[i].do_not_benchmark)
	    do_real_bench (widget, buttons[i].func, buttons[i].label, num);
	}

      return;
    }
  else
    {
      for (i = 0; i < nbuttons; i++)
	{
	  if (strcmp (buttons[i].label, what) == 0)
	    {
	      fn = buttons[i].func;
	      break;
	    }
	}
      
      if (!fn)
	g_print ("Can't bench: \"%s\" not found.\n", what);
      else
	do_real_bench (widget, fn, buttons[i].label, num);
    }
}

void 
usage (void)
{
  fprintf (stderr, "Usage: testbtk [--bench ALL|<bench>[:<count>]]\n");
  exit (1);
}

int
main (int argc, char *argv[])
{
  BtkBindingSet *binding_set;
  int i;
  gboolean done_benchmarks = FALSE;

  srand (time (NULL));

  test_init ();

  /* Check to see if we are being run from the correct
   * directory.
   */
  if (file_exists ("testbtkrc"))
    btk_rc_add_default_file ("testbtkrc");
  else if (file_exists ("tests/testbtkrc"))
    btk_rc_add_default_file ("tests/testbtkrc");
  else
    g_warning ("Couldn't find file \"testbtkrc\".");

  g_set_application_name ("BTK+ Test Program");

  btk_init (&argc, &argv);

  btk_accelerator_set_default_mod_mask (BDK_SHIFT_MASK |
					BDK_CONTROL_MASK |
					BDK_MOD1_MASK | 
					BDK_META_MASK |
					BDK_SUPER_MASK |
					BDK_HYPER_MASK |
					BDK_MOD4_MASK);
  /*  benchmarking
   */
  for (i = 1; i < argc; i++)
    {
      if (strncmp (argv[i], "--bench", strlen("--bench")) == 0)
        {
          int num = 1;
	  char *nextarg;
	  char *what;
	  char *count;
	  
	  nextarg = strchr (argv[i], '=');
	  if (nextarg)
	    nextarg++;
	  else
	    {
	      i++;
	      if (i == argc)
		usage ();
	      nextarg = argv[i];
	    }

	  count = strchr (nextarg, ':');
	  if (count)
	    {
	      what = g_strndup (nextarg, count - nextarg);
	      count++;
	      num = atoi (count);
	      if (num <= 0)
		usage ();
	    }
	  else
	    what = g_strdup (nextarg);

          do_bench (what, num ? num : 1);
	  done_benchmarks = TRUE;
        }
      else
	usage ();
    }
  if (done_benchmarks)
    return 0;

  /* bindings test
   */
  binding_set = btk_binding_set_by_class (g_type_class_ref (BTK_TYPE_WIDGET));
  btk_binding_entry_add_signal (binding_set,
				'9', BDK_CONTROL_MASK | BDK_RELEASE_MASK,
				"debug_msg",
				1,
				B_TYPE_STRING, "BtkWidgetClass <ctrl><release>9 test");
  
  /* We use btk_rc_parse_string() here so we can make sure it works across theme
   * changes
   */

  btk_rc_parse_string ("style \"testbtk-version-label\" { "
		       "   fg[NORMAL] = \"#ff0000\"\n"
		       "   font = \"Sans 18\"\n"
		       "}\n"
		       "widget \"*.testbtk-version-label\" style \"testbtk-version-label\"");
  
  create_main_window ();

  btk_main ();

  if (1)
    {
      while (g_main_context_pending (NULL))
	g_main_context_iteration (NULL, FALSE);
#if 0
      sleep (1);
      while (g_main_context_pending (NULL))
	g_main_context_iteration (NULL, FALSE);
#endif
    }
  return 0;
}
