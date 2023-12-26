/* BdkPixbuf library - Scaling and compositing demo
 *
 * Copyright (C) 1999 The Free Software Foundation
 *
 * Authors: Federico Mena-Quintero <federico@gimp.org>
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

#include "config.h"
#include <stdlib.h>
#include <btk/btk.h>
#include <math.h>



#define FRAME_DELAY 50

#define BACKGROUND_NAME "background.jpg"

static const char *image_names[] = {
	"apple-red.png",
	"bunny-applets.png",
	"bunny-calendar.png",
	"bunny-foot.png",
	"bunny-gmush.png",
	"bunny-gimp.png",
	"bunny-gsame.png",
	"gnu-keys.png"
};

#define N_IMAGES (sizeof (image_names) / sizeof (image_names[0]))

/* Current frame */
static BdkPixbuf *frame;

/* Background image */
static BdkPixbuf *background;
static int back_width, back_height;

/* Images */
static BdkPixbuf *images[N_IMAGES];

/* Widgets */
static BtkWidget *da;



/* Loads the images for the demo and returns whether the operation succeeded */
static gboolean
load_pixbufs (void)
{
	int i;

        /* We pass NULL for the error return location, we don't care
         * about the error message.
         */
        
	background = bdk_pixbuf_new_from_file (BACKGROUND_NAME, NULL);
	if (!background)
		return FALSE;

	back_width = bdk_pixbuf_get_width (background);
	back_height = bdk_pixbuf_get_height (background);

	for (i = 0; i < N_IMAGES; i++) {
		images[i] = bdk_pixbuf_new_from_file (image_names[i], NULL);
		if (!images[i])
			return FALSE;
	}

	return TRUE;
}

/* Expose callback for the drawing area */
static gint
expose_cb (BtkWidget *widget, BdkEventExpose *event, gpointer data)
{
        bairo_t *cr;
         
        cr = bdk_bairo_create (event->window);
        bdk_bairo_set_source_pixbuf (cr, frame, 0, 0);
        bdk_bairo_rectangle (cr, &event->area);
        bairo_fill (cr);

        bairo_destroy (cr);

	return TRUE;
}

#define CYCLE_LEN 60

static int frame_num;

/* Timeout handler to regenerate the frame */
static gint
timeout (gpointer data)
{
	double f;
	int i;
	double xmid, ymid;
	double radius;

	bdk_pixbuf_copy_area (background, 0, 0, back_width, back_height,
			      frame, 0, 0);

	f = (double) (frame_num % CYCLE_LEN) / CYCLE_LEN;

	xmid = back_width / 2.0;
	ymid = back_height / 2.0;

	radius = MIN (xmid, ymid) / 2.0;

	for (i = 0; i < N_IMAGES; i++) {
		double ang;
		int xpos, ypos;
		int iw, ih;
		double r;
		BdkRectangle r1, r2, dest;
		double k;

		ang = 2.0 * G_PI * (double) i / N_IMAGES - f * 2.0 * G_PI;

		iw = bdk_pixbuf_get_width (images[i]);
		ih = bdk_pixbuf_get_height (images[i]);

		r = radius + (radius / 3.0) * sin (f * 2.0 * G_PI);

		xpos = floor (xmid + r * cos (ang) - iw / 2.0 + 0.5);
		ypos = floor (ymid + r * sin (ang) - ih / 2.0 + 0.5);

		k = (i & 1) ? sin (f * 2.0 * G_PI) : cos (f * 2.0 * G_PI);
		k = 2.0 * k * k;
		k = MAX (0.25, k);

		r1.x = xpos;
		r1.y = ypos;
		r1.width = iw * k;
		r1.height = ih * k;

		r2.x = 0;
		r2.y = 0;
		r2.width = back_width;
		r2.height = back_height;

		if (bdk_rectangle_intersect (&r1, &r2, &dest))
			bdk_pixbuf_composite (images[i],
					      frame,
					      dest.x, dest.y,
					      dest.width, dest.height,
					      xpos, ypos,
					      k, k,
					      BDK_INTERP_NEAREST,
					      ((i & 1)
					       ? MAX (127, fabs (255 * sin (f * 2.0 * G_PI)))
					       : MAX (127, fabs (255 * cos (f * 2.0 * G_PI)))));
	}

	btk_widget_queue_draw (da);

	frame_num++;
	return TRUE;
}

static guint timeout_id;

/* Destroy handler for the window */
static void
destroy_cb (BtkObject *object, gpointer data)
{
	g_source_remove (timeout_id);
	timeout_id = 0;

	btk_main_quit ();
}

extern void pixbuf_init();

int
main (int argc, char **argv)
{
	BtkWidget *window;

	pixbuf_init ();
	btk_init (&argc, &argv);

	if (!load_pixbufs ()) {
		g_message ("main(): Could not load all the pixbufs!");
		exit (EXIT_FAILURE);
	}

	frame = bdk_pixbuf_new (BDK_COLORSPACE_RGB, FALSE, 8, back_width, back_height);

	window = btk_window_new (BTK_WINDOW_TOPLEVEL);

	btk_widget_set_size_request (window, back_width, back_height);
        btk_window_set_resizable (BTK_WINDOW (window), FALSE);

	g_signal_connect (window, "destroy",
			  G_CALLBACK (destroy_cb), NULL);

	da = btk_drawing_area_new ();

	g_signal_connect (da, "expose_event",
			  G_CALLBACK (expose_cb), NULL);

	btk_container_add (BTK_CONTAINER (window), da);

	timeout_id = bdk_threads_add_timeout (FRAME_DELAY, timeout, NULL);

	btk_widget_show_all (window);
	btk_main ();

	return 0;
}
