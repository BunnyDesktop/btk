/* testimage.c
 * Copyright (C) 2005  Red Hat, Inc.
 * Based on bairo-demo/X11/bairo-knockout.c
 *
 * Author: Owen Taylor
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <math.h>

#include <btk/btk.h>

static void
oval_path (bairo_t *cr,
           double xc, double yc,
           double xr, double yr)
{
  bairo_save (cr);

  bairo_translate (cr, xc, yc);
  bairo_scale (cr, 1.0, yr / xr);
  bairo_move_to (cr, xr, 0.0);
  bairo_arc (cr,
	     0, 0,
	     xr,
	     0, 2 * G_PI);
  bairo_close_path (cr);

  bairo_restore (cr);
}

/* Create a path that is a circular oval with radii xr, yr at xc,
 * yc.
 */
/* Fill the given area with checks in the standard style
 * for showing compositing effects.
 *
 * It would make sense to do this as a repeating surface,
 * but most implementations of RENDER currently have broken
 * implementations of repeat + transform, even when the
 * transform is a translation.
 */
static void
fill_checks (bairo_t *cr,
             int x,     int y,
             int width, int height)
{
  int i, j;
  
#define CHECK_SIZE 32

  bairo_rectangle (cr, x, y, width, height);
  bairo_set_source_rgb (cr, 0.4, 0.4, 0.4);
  bairo_fill (cr);

  /* Only works for CHECK_SIZE a power of 2 */
  j = x & (-CHECK_SIZE);
  
  for (; j < height; j += CHECK_SIZE)
    {
      i = y & (-CHECK_SIZE);
      for (; i < width; i += CHECK_SIZE)
	if ((i / CHECK_SIZE + j / CHECK_SIZE) % 2 == 0)
	  bairo_rectangle (cr, i, j, CHECK_SIZE, CHECK_SIZE);
    }

  bairo_set_source_rgb (cr, 0.7, 0.7, 0.7);
  bairo_fill (cr);
}

/* Draw a red, green, and blue circle equally spaced inside
 * the larger circle of radius r at (xc, yc)
 */
static void
draw_3circles (bairo_t *cr,
               double xc, double yc,
               double radius,
	       double alpha)
{
  double subradius = radius * (2 / 3. - 0.1);
    
  bairo_set_source_rgba (cr, 1., 0., 0., alpha);
  oval_path (cr,
	     xc + radius / 3. * cos (G_PI * (0.5)),
	     yc - radius / 3. * sin (G_PI * (0.5)),
	     subradius, subradius);
  bairo_fill (cr);
    
  bairo_set_source_rgba (cr, 0., 1., 0., alpha);
  oval_path (cr,
	     xc + radius / 3. * cos (G_PI * (0.5 + 2/.3)),
	     yc - radius / 3. * sin (G_PI * (0.5 + 2/.3)),
	     subradius, subradius);
  bairo_fill (cr);
    
  bairo_set_source_rgba (cr, 0., 0., 1., alpha);
  oval_path (cr,
	     xc + radius / 3. * cos (G_PI * (0.5 + 4/.3)),
	     yc - radius / 3. * sin (G_PI * (0.5 + 4/.3)),
	     subradius, subradius);
  bairo_fill (cr);
}

static void
draw (bairo_t *cr,
      int      width,
      int      height)
{
  bairo_surface_t *overlay, *punch, *circles;
  bairo_t *overlay_cr, *punch_cr, *circles_cr;

  /* Fill the background */
  double radius = 0.5 * (width < height ? width : height) - 10;
  double xc = width / 2.;
  double yc = height / 2.;

  overlay = bairo_surface_create_similar (bairo_get_target (cr),
					  BAIRO_CONTENT_COLOR_ALPHA,
					  width, height);
  if (overlay == NULL)
    return;

  punch = bairo_surface_create_similar (bairo_get_target (cr),
					BAIRO_CONTENT_ALPHA,
					width, height);
  if (punch == NULL)
    return;

  circles = bairo_surface_create_similar (bairo_get_target (cr),
					  BAIRO_CONTENT_COLOR_ALPHA,
					  width, height);
  if (circles == NULL)
    return;
    
  fill_checks (cr, 0, 0, width, height);

  /* Draw a black circle on the overlay
   */
  overlay_cr = bairo_create (overlay);
  bairo_set_source_rgb (overlay_cr, 0., 0., 0.);
  oval_path (overlay_cr, xc, yc, radius, radius);
  bairo_fill (overlay_cr);

  /* Draw 3 circles to the punch surface, then cut
   * that out of the main circle in the overlay
   */
  punch_cr = bairo_create (punch);
  draw_3circles (punch_cr, xc, yc, radius, 1.0);
  bairo_destroy (punch_cr);

  bairo_set_operator (overlay_cr, BAIRO_OPERATOR_DEST_OUT);
  bairo_set_source_surface (overlay_cr, punch, 0, 0);
  bairo_paint (overlay_cr);

  /* Now draw the 3 circles in a subgroup again
   * at half intensity, and use OperatorAdd to join up
   * without seams.
   */
  circles_cr = bairo_create (circles);
  
  bairo_set_operator (circles_cr, BAIRO_OPERATOR_OVER);
  draw_3circles (circles_cr, xc, yc, radius, 0.5);
  bairo_destroy (circles_cr);

  bairo_set_operator (overlay_cr, BAIRO_OPERATOR_ADD);
  bairo_set_source_surface (overlay_cr, circles, 0, 0);
  bairo_paint (overlay_cr);

  bairo_destroy (overlay_cr);

  bairo_set_source_surface (cr, overlay, 0, 0);
  bairo_paint (cr);

  bairo_surface_destroy (overlay);
  bairo_surface_destroy (punch);
  bairo_surface_destroy (circles);
}

static bboolean
on_expose_event (BtkWidget      *widget,
		 BdkEventExpose *event,
		 bpointer        data)
{
  bairo_t *cr;

  cr = bdk_bairo_create (widget->window);

  draw (cr, widget->allocation.width, widget->allocation.height);

  bairo_destroy (cr);

  return FALSE;
}

int
main (int argc, char **argv)
{
  BtkWidget *window, *darea;

  btk_init (&argc, &argv);

  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  
  btk_window_set_default_size (BTK_WINDOW (window), 400, 400);
  btk_window_set_title (BTK_WINDOW (window), "bairo: Knockout Groups");

  darea = btk_drawing_area_new ();
  btk_container_add (BTK_CONTAINER (window), darea);

  g_signal_connect (darea, "expose-event",
		    G_CALLBACK (on_expose_event), NULL);
  g_signal_connect (window, "destroy-event",
		    G_CALLBACK (btk_main_quit), NULL);

  btk_widget_show_all (window);
  
  btk_main ();

  return 0;
}
