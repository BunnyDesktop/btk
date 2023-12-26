
/* testpixbuf -- test program for bdk-pixbuf code
 * Copyright (C) 1999 Mark Crichton, Larry Ewing
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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <btk/btk.h>

typedef struct _LoadContext LoadContext;

struct _LoadContext
{
  bchar *filename;
  BtkWidget *window;
  BdkPixbufLoader *pixbuf_loader;
  buint load_timeout;
  FILE* image_stream;
};

static void
destroy_context (bpointer data)
{
  LoadContext *lc = data;

  g_free (lc->filename);
  
  if (lc->load_timeout)
    g_source_remove (lc->load_timeout);

  if (lc->image_stream)
    fclose (lc->image_stream);

  if (lc->pixbuf_loader)
    {
      bdk_pixbuf_loader_close (lc->pixbuf_loader, NULL);
      g_object_unref (lc->pixbuf_loader);
    }
  
  g_free (lc);
}

static LoadContext*
get_load_context (BtkWidget *image)
{
  LoadContext *lc;

  lc = g_object_get_data (B_OBJECT (image), "lc");

  if (lc == NULL)
    {
      lc = g_new0 (LoadContext, 1);

      g_object_set_data_full (B_OBJECT (image),        
                              "lc",
                              lc,
                              destroy_context);
    }

  return lc;
}

static void
progressive_prepared_callback (BdkPixbufLoader* loader,
                               bpointer         data)
{
  BdkPixbuf* pixbuf;
  BtkWidget* image;

  image = BTK_WIDGET (data);
    
  pixbuf = bdk_pixbuf_loader_get_pixbuf (loader);

  /* Avoid displaying random memory contents, since the pixbuf
   * isn't filled in yet.
   */
  bdk_pixbuf_fill (pixbuf, 0xaaaaaaff);

  /* Could set the pixbuf instead, if we only wanted to display
   * static images.
   */
  btk_image_set_from_animation (BTK_IMAGE (image),
                                bdk_pixbuf_loader_get_animation (loader));
}

static void
progressive_updated_callback (BdkPixbufLoader* loader,
                              bint x, bint y, bint width, bint height,
                              bpointer data)
{
  BtkWidget* image;
  
  image = BTK_WIDGET (data);

  /* We know the pixbuf inside the BtkImage has changed, but the image
   * itself doesn't know this; so queue a redraw.  If we wanted to be
   * really efficient, we could use a drawing area or something
   * instead of a BtkImage, so we could control the exact position of
   * the pixbuf on the display, then we could queue a draw for only
   * the updated area of the image.
   */

  /* We only really need to redraw if the image's animation iterator
   * is bdk_pixbuf_animation_iter_on_currently_loading_frame(), but
   * who cares.
   */
  
  btk_widget_queue_draw (image);
}

static bint
progressive_timeout (bpointer data)
{
  BtkWidget *image;
  LoadContext *lc;
  
  image = BTK_WIDGET (data);
  lc = get_load_context (image);
  
  /* This shows off fully-paranoid error handling, so looks scary.
   * You could factor out the error handling code into a nice separate
   * function to make things nicer.
   */
  
  if (lc->image_stream)
    {
      size_t bytes_read;
      buchar buf[256];
      GError *error = NULL;
      
      bytes_read = fread (buf, 1, 256, lc->image_stream);

      if (ferror (lc->image_stream))
        {
          BtkWidget *dialog;
          
          dialog = btk_message_dialog_new (BTK_WINDOW (lc->window),
                                           BTK_DIALOG_DESTROY_WITH_PARENT,
                                           BTK_MESSAGE_ERROR,
                                           BTK_BUTTONS_CLOSE,
                                           "Failure reading image file 'alphatest.png': %s",
                                           g_strerror (errno));

          g_signal_connect (dialog, "response",
			    G_CALLBACK (btk_widget_destroy), NULL);

          fclose (lc->image_stream);
          lc->image_stream = NULL;

          btk_widget_show (dialog);
          
          lc->load_timeout = 0;

          return FALSE; /* uninstall the timeout */
        }

      if (!bdk_pixbuf_loader_write (lc->pixbuf_loader,
                                    buf, bytes_read,
                                    &error))
        {
          BtkWidget *dialog;
          
          dialog = btk_message_dialog_new (BTK_WINDOW (lc->window),
                                           BTK_DIALOG_DESTROY_WITH_PARENT,
                                           BTK_MESSAGE_ERROR,
                                           BTK_BUTTONS_CLOSE,
                                           "Failed to load image: %s",
                                           error->message);

          g_error_free (error);
          
          g_signal_connect (dialog, "response",
			    G_CALLBACK (btk_widget_destroy), NULL);

          fclose (lc->image_stream);
          lc->image_stream = NULL;
          
          btk_widget_show (dialog);

          lc->load_timeout = 0;

          return FALSE; /* uninstall the timeout */
        }

      if (feof (lc->image_stream))
        {
          fclose (lc->image_stream);
          lc->image_stream = NULL;

          /* Errors can happen on close, e.g. if the image
           * file was truncated we'll know on close that
           * it was incomplete.
           */
          error = NULL;
          if (!bdk_pixbuf_loader_close (lc->pixbuf_loader,
                                        &error))
            {
              BtkWidget *dialog;
              
              dialog = btk_message_dialog_new (BTK_WINDOW (lc->window),
                                               BTK_DIALOG_DESTROY_WITH_PARENT,
                                               BTK_MESSAGE_ERROR,
                                               BTK_BUTTONS_CLOSE,
                                               "Failed to load image: %s",
                                               error->message);
              
              g_error_free (error);
              
              g_signal_connect (dialog, "response",
				G_CALLBACK (btk_widget_destroy), NULL);
              
              btk_widget_show (dialog);

              g_object_unref (lc->pixbuf_loader);
              lc->pixbuf_loader = NULL;
              
              lc->load_timeout = 0;
              
              return FALSE; /* uninstall the timeout */
            }
          
          g_object_unref (lc->pixbuf_loader);
          lc->pixbuf_loader = NULL;
        }
    }
  else
    {
      lc->image_stream = fopen (lc->filename, "r");

      if (lc->image_stream == NULL)
        {
          BtkWidget *dialog;
          
          dialog = btk_message_dialog_new (BTK_WINDOW (lc->window),
                                           BTK_DIALOG_DESTROY_WITH_PARENT,
                                           BTK_MESSAGE_ERROR,
                                           BTK_BUTTONS_CLOSE,
                                           "Unable to open image file '%s': %s",
                                           lc->filename,
                                           g_strerror (errno));

          g_signal_connect (dialog, "response",
			    G_CALLBACK (btk_widget_destroy), NULL);
          
          btk_widget_show (dialog);

          lc->load_timeout = 0;

          return FALSE; /* uninstall the timeout */
        }

      if (lc->pixbuf_loader)
        {
          bdk_pixbuf_loader_close (lc->pixbuf_loader, NULL);
          g_object_unref (lc->pixbuf_loader);
          lc->pixbuf_loader = NULL;
        }
      
      lc->pixbuf_loader = bdk_pixbuf_loader_new ();
      
      g_signal_connect (lc->pixbuf_loader, "area_prepared",
			G_CALLBACK (progressive_prepared_callback), image);
      g_signal_connect (lc->pixbuf_loader, "area_updated",
			G_CALLBACK (progressive_updated_callback), image);
    }

  /* leave timeout installed */
  return TRUE;
}

static void
start_progressive_loading (BtkWidget *image)
{
  LoadContext *lc;

  lc = get_load_context (image);
  
  /* This is obviously totally contrived (we slow down loading
   * on purpose to show how incremental loading works).
   * The real purpose of incremental loading is the case where
   * you are reading data from a slow source such as the network.
   * The timeout simply simulates a slow data source by inserting
   * pauses in the reading process.
   */
  lc->load_timeout = bdk_threads_add_timeout (100,
                                    progressive_timeout,
                                    image);
}

static BtkWidget *
do_image (const char *filename)
{
  BtkWidget *frame;
  BtkWidget *vbox;
  BtkWidget *image;
  BtkWidget *label;
  BtkWidget *align;
  BtkWidget *window;
  bchar *str, *escaped;
  LoadContext *lc;
  
  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  btk_window_set_title (BTK_WINDOW (window), "Image Loading");

  btk_container_set_border_width (BTK_CONTAINER (window), 8);

  vbox = btk_vbox_new (FALSE, 8);
  btk_container_set_border_width (BTK_CONTAINER (vbox), 8);
  btk_container_add (BTK_CONTAINER (window), vbox);

  label = btk_label_new (NULL);
  btk_label_set_line_wrap (BTK_LABEL (label), TRUE);
  escaped = g_markup_escape_text (filename, -1);
  str = g_strdup_printf ("Progressively loading: <b>%s</b>", escaped);
  btk_label_set_markup (BTK_LABEL (label),
                        str);
  g_free (escaped);
  g_free (str);
  
  btk_box_pack_start (BTK_BOX (vbox), label, FALSE, FALSE, 0);
      
  frame = btk_frame_new (NULL);
  btk_frame_set_shadow_type (BTK_FRAME (frame), BTK_SHADOW_IN);
  /* The alignment keeps the frame from growing when users resize
   * the window
   */
  align = btk_alignment_new (0.5, 0.5, 0, 0);
  btk_container_add (BTK_CONTAINER (align), frame);
  btk_box_pack_start (BTK_BOX (vbox), align, FALSE, FALSE, 0);      

  image = btk_image_new_from_pixbuf (NULL);
  btk_container_add (BTK_CONTAINER (frame), image);

  lc = get_load_context (image);

  lc->window = window;
  lc->filename = g_strdup (filename);
  
  start_progressive_loading (image);

  g_signal_connect (window, "destroy",
		    G_CALLBACK (btk_main_quit), NULL);
  
  g_signal_connect (window, "delete_event",
		    G_CALLBACK (btk_main_quit), NULL);

  btk_widget_show_all (window);

  return window;
}

static void
do_nonprogressive (const bchar *filename)
{
  BtkWidget *frame;
  BtkWidget *vbox;
  BtkWidget *image;
  BtkWidget *label;
  BtkWidget *align;
  BtkWidget *window;
  bchar *str, *escaped;
  
  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  btk_window_set_title (BTK_WINDOW (window), "Animation");

  btk_container_set_border_width (BTK_CONTAINER (window), 8);

  vbox = btk_vbox_new (FALSE, 8);
  btk_container_set_border_width (BTK_CONTAINER (vbox), 8);
  btk_container_add (BTK_CONTAINER (window), vbox);

  label = btk_label_new (NULL);
  btk_label_set_line_wrap (BTK_LABEL (label), TRUE);
  escaped = g_markup_escape_text (filename, -1);
  str = g_strdup_printf ("Loaded from file: <b>%s</b>", escaped);
  btk_label_set_markup (BTK_LABEL (label),
                        str);
  g_free (escaped);
  g_free (str);
  
  btk_box_pack_start (BTK_BOX (vbox), label, FALSE, FALSE, 0);
      
  frame = btk_frame_new (NULL);
  btk_frame_set_shadow_type (BTK_FRAME (frame), BTK_SHADOW_IN);
  /* The alignment keeps the frame from growing when users resize
   * the window
   */
  align = btk_alignment_new (0.5, 0.5, 0, 0);
  btk_container_add (BTK_CONTAINER (align), frame);
  btk_box_pack_start (BTK_BOX (vbox), align, FALSE, FALSE, 0);      

  image = btk_image_new_from_file (filename);
  btk_container_add (BTK_CONTAINER (frame), image);

  g_signal_connect (window, "destroy",
		    G_CALLBACK (btk_main_quit), NULL);
  
  g_signal_connect (window, "delete_event",
		    G_CALLBACK (btk_main_quit), NULL);

  btk_widget_show_all (window);
}

int
main (int    argc,
      char **argv)
{
  bint i;
  
  btk_init (&argc, &argv);

  i = 1;
  while (i < argc)
    {
      do_image (argv[i]);
      do_nonprogressive (argv[i]);
      
      ++i;
    }

  btk_main ();
  
  return 0;
}

