/* Rotated Text
 *
 * This demo shows how to use BangoBairo to draw rotated and transformed
 * text.  The right pane shows a rotated BtkLabel widget.
 *
 * In both cases, a custom BangoBairo shape renderer is installed to draw
 * a red heard using bairo drawing operations instead of the Unicode heart
 * character.
 */

#include <btk/btk.h>
#include <string.h>

static BtkWidget *window = NULL;

#define HEART "♥"
const char text[] = "I ♥ BTK+";

static void
fancy_shape_renderer (bairo_t        *cr,
		      BangoAttrShape *attr,
		      gboolean        do_path,
		      gpointer        data)
{
  double x, y;
  bairo_get_current_point (cr, &x, &y);
  bairo_translate (cr, x, y);

  bairo_scale (cr,
	       (double) attr->ink_rect.width  / BANGO_SCALE,
	       (double) attr->ink_rect.height / BANGO_SCALE);

  switch (GPOINTER_TO_UINT (attr->data))
    {
    case 0x2665: /* U+2665 BLACK HEART SUIT */
      {
        bairo_move_to (cr, .5, .0);
        bairo_line_to (cr, .9, -.4);
	bairo_curve_to (cr, 1.1, -.8, .5, -.9, .5, -.5);
	bairo_curve_to (cr, .5, -.9, -.1, -.8, .1, -.4);
	bairo_close_path (cr);
      }
      break;
    }

  if (!do_path) {
    bairo_set_source_rgb (cr, 1., 0., 0.);
    bairo_fill (cr);
  }
}

BangoAttrList *
create_fancy_attr_list_for_layout (BangoLayout *layout)
{
  BangoAttrList *attrs;
  BangoFontMetrics *metrics;
  int ascent;
  BangoRectangle ink_rect, logical_rect;
  const char *p;

  /* Get font metrics and prepare fancy shape size */
  metrics = bango_context_get_metrics (bango_layout_get_context (layout),
				       bango_layout_get_font_description (layout),
				       NULL);
  ascent = bango_font_metrics_get_ascent (metrics);
  logical_rect.x = 0;
  logical_rect.width = ascent;
  logical_rect.y = -ascent;
  logical_rect.height = ascent;
  ink_rect = logical_rect;
  bango_font_metrics_unref (metrics);

  /* Set fancy shape attributes for all hearts */
  attrs = bango_attr_list_new ();
  for (p = text; (p = strstr (p, HEART)); p += strlen (HEART))
    {
      BangoAttribute *attr;
      
      attr = bango_attr_shape_new_with_data (&ink_rect,
					     &logical_rect,
					     GUINT_TO_POINTER (g_utf8_get_char (p)),
					     NULL, NULL);

      attr->start_index = p - text;
      attr->end_index = attr->start_index + strlen (HEART);

      bango_attr_list_insert (attrs, attr);
    }

  return attrs;
}

static gboolean
rotated_text_expose_event (BtkWidget      *widget,
			   BdkEventExpose *event,
			   gpointer	   data)
{
#define RADIUS 150
#define N_WORDS 5
#define FONT "Serif 18"

  BangoContext *context;
  BangoLayout *layout;
  BangoFontDescription *desc;

  bairo_t *cr;
  bairo_pattern_t *pattern;

  BangoAttrList *attrs;
  BtkAllocation allocation;
  int width, height;
  double device_radius;
  int i;
  
  btk_widget_get_allocation (widget, &allocation);

  width = allocation.width;
  height = allocation.height;

  /* Create a bairo context and set up a transformation matrix so that the user
   * space coordinates for the centered square where we draw are [-RADIUS, RADIUS],
   * [-RADIUS, RADIUS].
   * We first center, then change the scale. */
  cr = bdk_bairo_create (btk_widget_get_window (widget));
  device_radius = MIN (width, height) / 2.;
  bairo_translate (cr,
		   device_radius + (width - 2 * device_radius) / 2,
		   device_radius + (height - 2 * device_radius) / 2);
  bairo_scale (cr, device_radius / RADIUS, device_radius / RADIUS);

  /* Create and a subtle gradient source and use it. */
  pattern = bairo_pattern_create_linear (-RADIUS, -RADIUS, RADIUS, RADIUS);
  bairo_pattern_add_color_stop_rgb (pattern, 0., .5, .0, .0);
  bairo_pattern_add_color_stop_rgb (pattern, 1., .0, .0, .5);
  bairo_set_source (cr, pattern);

  /* Create a BangoContext and set up our shape renderer */
  context = btk_widget_create_bango_context (widget);
  bango_bairo_context_set_shape_renderer (context,
					  fancy_shape_renderer,
					  NULL, NULL);

  /* Create a BangoLayout, set the text, font, and attributes */
  layout = bango_layout_new (context);
  bango_layout_set_text (layout, text, -1);
  desc = bango_font_description_from_string (FONT);
  bango_layout_set_font_description (layout, desc);

  attrs = create_fancy_attr_list_for_layout (layout);
  bango_layout_set_attributes (layout, attrs);
  bango_attr_list_unref (attrs);

  /* Draw the layout N_WORDS times in a circle */
  for (i = 0; i < N_WORDS; i++)
    {
      int width, height;

      /* Inform Bango to re-layout the text with the new transformation matrix */
      bango_bairo_update_layout (cr, layout);
    
      bango_layout_get_pixel_size (layout, &width, &height);
      bairo_move_to (cr, - width / 2, - RADIUS * .9);
      bango_bairo_show_layout (cr, layout);

      /* Rotate for the next turn */
      bairo_rotate (cr, G_PI*2 / N_WORDS);
    }

  /* free the objects we created */
  bango_font_description_free (desc);
  g_object_unref (layout);
  g_object_unref (context);
  bairo_pattern_destroy (pattern);
  bairo_destroy (cr);
  
  return FALSE;
}

BtkWidget *
do_rotated_text (BtkWidget *do_widget)
{
  if (!window)
    {
      BtkWidget *box;
      BtkWidget *drawing_area;
      BtkWidget *label;
      BangoLayout *layout;
      BangoAttrList *attrs;

      const BdkColor white = { 0, 0xffff, 0xffff, 0xffff };
      
      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (do_widget));
      btk_window_set_title (BTK_WINDOW (window), "Rotated Text");
      btk_window_set_default_size (BTK_WINDOW (window), 4 * RADIUS, 2 * RADIUS);
      g_signal_connect (window, "destroy", G_CALLBACK (btk_widget_destroyed), &window);

      box = btk_hbox_new (TRUE, 0);
      btk_container_add (BTK_CONTAINER (window), box);

      /* Add a drawing area */

      drawing_area = btk_drawing_area_new ();
      btk_container_add (BTK_CONTAINER (box), drawing_area);

      /* This overrides the background color from the theme */
      btk_widget_modify_bg (drawing_area, BTK_STATE_NORMAL, &white);

      g_signal_connect (drawing_area, "expose-event",
			G_CALLBACK (rotated_text_expose_event), NULL);

      /* And a label */

      label = btk_label_new (text);
      btk_container_add (BTK_CONTAINER (box), label);

      btk_label_set_angle (BTK_LABEL (label), 45);

      /* Set up fancy stuff on the label */
      layout = btk_label_get_layout (BTK_LABEL (label));
      bango_bairo_context_set_shape_renderer (bango_layout_get_context (layout),
					      fancy_shape_renderer,
					      NULL, NULL);
      attrs = create_fancy_attr_list_for_layout (layout);
      btk_label_set_attributes (BTK_LABEL (label), attrs);
      bango_attr_list_unref (attrs);
    }

  if (!btk_widget_get_visible (window))
    {
      btk_widget_show_all (window);
    }
  else
    {
      btk_widget_destroy (window);
      window = NULL;
    }

  return window;
}
