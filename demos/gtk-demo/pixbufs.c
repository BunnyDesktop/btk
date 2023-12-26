/* Pixbufs
 *
 * A BdkPixbuf represents an image, normally in RGB or RGBA format.
 * Pixbufs are normally used to load files from disk and perform
 * image scaling.
 *
 * This demo is not all that educational, but looks cool. It was written
 * by Extreme Pixbuf Hacker Federico Mena Quintero. It also shows
 * off how to use BtkDrawingArea to do a simple animation.
 *
 * Look at the Image demo for additional pixbuf usage examples.
 *
 */

#include <stdlib.h>
#include <btk/btk.h>
#include <math.h>

#include "demo-common.h"

#define FRAME_DELAY 50

#define BACKGROUND_NAME "background.jpg"

static const char *image_names[] = {
  "apple-red.png",
  "gnome-applets.png",
  "gnome-calendar.png",
  "gnome-foot.png",
  "gnome-gmush.png",
  "gnome-gimp.png",
  "gnome-gsame.png",
  "gnu-keys.png"
};

#define N_IMAGES G_N_ELEMENTS (image_names)

/* demo window */
static BtkWidget *window = NULL;

/* Current frame */
static BdkPixbuf *frame;

/* Background image */
static BdkPixbuf *background;
static gint back_width, back_height;

/* Images */
static BdkPixbuf *images[N_IMAGES];

/* Widgets */
static BtkWidget *da;

/* Loads the images for the demo and returns whether the operation succeeded */
static gboolean
load_pixbufs (GError **error)
{
  gint i;
  char *filename;

  if (background)
    return TRUE; /* already loaded earlier */

  /* demo_find_file() looks in the current directory first,
   * so you can run btk-demo without installing BTK, then looks
   * in the location where the file is installed.
   */
  filename = demo_find_file (BACKGROUND_NAME, error);
  if (!filename)
    return FALSE; /* note that "error" was filled in and returned */

  background = bdk_pixbuf_new_from_file (filename, error);
  g_free (filename);
  
  if (!background)
    return FALSE; /* Note that "error" was filled with a GError */

  back_width = bdk_pixbuf_get_width (background);
  back_height = bdk_pixbuf_get_height (background);

  for (i = 0; i < N_IMAGES; i++)
    {
      filename = demo_find_file (image_names[i], error);
      if (!filename)
        return FALSE; /* Note that "error" was filled with a GError */
      
      images[i] = bdk_pixbuf_new_from_file (filename, error);
      g_free (filename);
      
      if (!images[i])
        return FALSE; /* Note that "error" was filled with a GError */
    }

  return TRUE;
}

/* Expose callback for the drawing area */
static gint
expose_cb (BtkWidget      *widget,
           BdkEventExpose *event,
           gpointer        data)
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

  for (i = 0; i < N_IMAGES; i++)
    {
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

  BDK_THREADS_ENTER ();
  btk_widget_queue_draw (da);
  BDK_THREADS_LEAVE ();

  frame_num++;
  return TRUE;
}

static guint timeout_id;

static void
cleanup_callback (BtkObject *object,
                  gpointer   data)
{
  g_source_remove (timeout_id);
  timeout_id = 0;
}

BtkWidget *
do_pixbufs (BtkWidget *do_widget)
{
  if (!window)
    {
      GError *error;

      window = btk_window_new (BTK_WINDOW_TOPLEVEL);
      btk_window_set_screen (BTK_WINDOW (window),
                             btk_widget_get_screen (do_widget));
      btk_window_set_title (BTK_WINDOW (window), "Pixbufs");
      btk_window_set_resizable (BTK_WINDOW (window), FALSE);

      g_signal_connect (window, "destroy",
                        G_CALLBACK (btk_widget_destroyed), &window);
      g_signal_connect (window, "destroy",
                        G_CALLBACK (cleanup_callback), NULL);


      error = NULL;
      if (!load_pixbufs (&error))
        {
          BtkWidget *dialog;

          dialog = btk_message_dialog_new (BTK_WINDOW (window),
                                           BTK_DIALOG_DESTROY_WITH_PARENT,
                                           BTK_MESSAGE_ERROR,
                                           BTK_BUTTONS_CLOSE,
                                           "Failed to load an image: %s",
                                           error->message);

          g_error_free (error);

          g_signal_connect (dialog, "response",
                            G_CALLBACK (btk_widget_destroy), NULL);

          btk_widget_show (dialog);
        }
      else
        {
          btk_widget_set_size_request (window, back_width, back_height);

          frame = bdk_pixbuf_new (BDK_COLORSPACE_RGB, FALSE, 8, back_width, back_height);

          da = btk_drawing_area_new ();

          g_signal_connect (da, "expose-event",
                            G_CALLBACK (expose_cb), NULL);

          btk_container_add (BTK_CONTAINER (window), da);

          timeout_id = g_timeout_add (FRAME_DELAY, timeout, NULL);
        }
    }

  if (!btk_widget_get_visible (window))
    {
      btk_widget_show_all (window);
    }
  else
    {
      btk_widget_destroy (window);
      window = NULL;
      g_object_unref (frame);
    }

  return window;
}
