#include "config.h"
#include <btk/btk.h>

#include <stdio.h>
#include <stdlib.h>

BdkInterpType interp_type = BDK_INTERP_BILINEAR;
int overall_alpha = 255;
BdkPixbuf *pixbuf;
BtkWidget *darea;
  
void
set_interp_type (BtkWidget *widget, gpointer data)
{
  guint types[] = { BDK_INTERP_NEAREST,
                    BDK_INTERP_BILINEAR,
                    BDK_INTERP_TILES,
                    BDK_INTERP_HYPER };

  interp_type = types[btk_combo_box_get_active (BTK_COMBO_BOX (widget))];
  btk_widget_queue_draw (darea);
}

void
overall_changed_cb (BtkAdjustment *adjustment, gpointer data)
{
  if (adjustment->value != overall_alpha)
    {
      overall_alpha = adjustment->value;
      btk_widget_queue_draw (darea);
    }
}

gboolean
expose_cb (BtkWidget *widget, BdkEventExpose *event, gpointer data)
{
  BdkPixbuf *dest;
  bairo_t *cr;

  bdk_window_set_back_pixmap (widget->window, NULL, FALSE);
  
  dest = bdk_pixbuf_new (BDK_COLORSPACE_RGB, FALSE, 8, event->area.width, event->area.height);

  bdk_pixbuf_composite_color (pixbuf, dest,
			      0, 0, event->area.width, event->area.height,
			      -event->area.x, -event->area.y,
			      (double) widget->allocation.width / bdk_pixbuf_get_width (pixbuf),
			      (double) widget->allocation.height / bdk_pixbuf_get_height (pixbuf),
			      interp_type, overall_alpha,
			      event->area.x, event->area.y, 16, 0xaaaaaa, 0x555555);

  cr = bdk_bairo_create (event->window);

  bdk_bairo_set_source_pixbuf (cr, dest, 0, 0);
  bdk_bairo_rectangle (cr, &event->area);
  bairo_fill (cr);

  bairo_destroy (cr);
  g_object_unref (dest);
  
  return TRUE;
}

extern void pixbuf_init();

int
main(int argc, char **argv)
{
	BtkWidget *window, *vbox;
        BtkWidget *combo_box;
	BtkWidget *alignment;
	BtkWidget *hbox, *label, *hscale;
	BtkAdjustment *adjustment;
	BtkRequisition scratch_requisition;
        const gchar *creator;
        GError *error;
        
	pixbuf_init ();

	btk_init (&argc, &argv);

	if (argc != 2) {
		fprintf (stderr, "Usage: testpixbuf-scale FILE\n");
		exit (1);
	}

        error = NULL;
	pixbuf = bdk_pixbuf_new_from_file (argv[1], &error);
	if (!pixbuf) {
		fprintf (stderr, "Cannot load image: %s\n",
                         error->message);
                g_error_free (error);
		exit(1);
	}

        creator = bdk_pixbuf_get_option (pixbuf, "tEXt::Software");
        if (creator)
                g_print ("%s was created by '%s'\n", argv[1], creator);

	window = btk_window_new (BTK_WINDOW_TOPLEVEL);
	g_signal_connect (window, "destroy",
			  G_CALLBACK (btk_main_quit), NULL);
	
	vbox = btk_vbox_new (FALSE, 0);
	btk_container_add (BTK_CONTAINER (window), vbox);

        combo_box = btk_combo_box_text_new ();

        btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (combo_box), "NEAREST");
        btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (combo_box), "BILINEAR");
        btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (combo_box), "TILES");
        btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (combo_box), "HYPER");

        btk_combo_box_set_active (BTK_COMBO_BOX (combo_box), 1);

        g_signal_connect (combo_box, "changed",
                          G_CALLBACK (set_interp_type),
                          NULL);
	
	alignment = btk_alignment_new (0.0, 0.0, 0.0, 0.5);
	btk_box_pack_start (BTK_BOX (vbox), alignment, FALSE, FALSE, 0);

	hbox = btk_hbox_new (FALSE, 4);
	btk_box_pack_start (BTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	label = btk_label_new ("Overall Alpha:");
	btk_box_pack_start (BTK_BOX (hbox), label, FALSE, FALSE, 0);

	adjustment = BTK_ADJUSTMENT (btk_adjustment_new (overall_alpha, 0, 255, 1, 10, 0));
	g_signal_connect (adjustment, "value_changed",
			  G_CALLBACK (overall_changed_cb), NULL);
	
	hscale = btk_hscale_new (adjustment);
	btk_scale_set_digits (BTK_SCALE (hscale), 0);
	btk_box_pack_start (BTK_BOX (hbox), hscale, TRUE, TRUE, 0);

	btk_container_add (BTK_CONTAINER (alignment), combo_box);
	btk_widget_show_all (vbox);

	/* Compute the size without the drawing area, so we know how big to make the default size */
	btk_widget_size_request (vbox, &scratch_requisition);

	darea = btk_drawing_area_new ();
	btk_box_pack_start (BTK_BOX (vbox), darea, TRUE, TRUE, 0);

	g_signal_connect (darea, "expose_event",
			  G_CALLBACK (expose_cb), NULL);

	btk_window_set_default_size (BTK_WINDOW (window),
				     bdk_pixbuf_get_width (pixbuf),
				     scratch_requisition.height + bdk_pixbuf_get_height (pixbuf));
	
	btk_widget_show_all (window);

	btk_main ();

	return 0;
}
