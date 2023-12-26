/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

#include "config.h"
#include <stdio.h>

#include <btk/btk.h>

static void
compare_pixbufs (BdkPixbuf *pixbuf, BdkPixbuf *compare, const gchar *file_type)
{
        if ((bdk_pixbuf_get_width (pixbuf) !=
             bdk_pixbuf_get_width (compare)) ||
            (bdk_pixbuf_get_height (pixbuf) !=
             bdk_pixbuf_get_height (compare)) ||
            (bdk_pixbuf_get_n_channels (pixbuf) !=
             bdk_pixbuf_get_n_channels (compare)) ||
            (bdk_pixbuf_get_has_alpha (pixbuf) !=
             bdk_pixbuf_get_has_alpha (compare)) ||
            (bdk_pixbuf_get_bits_per_sample (pixbuf) !=
             bdk_pixbuf_get_bits_per_sample (compare))) {
                fprintf (stderr,
                         "saved %s file differs from copy in memory\n",
                         file_type);
        } else {
                guchar *orig_pixels;
                guchar *compare_pixels;
                gint    orig_rowstride;
                gint    compare_rowstride;
                gint    width;
                gint    height;
                gint    bytes_per_pixel;
                gint    x, y;
                guchar *p1, *p2;
                gint    count = 0;

                orig_pixels = bdk_pixbuf_get_pixels (pixbuf);
                compare_pixels = bdk_pixbuf_get_pixels (compare);

                orig_rowstride = bdk_pixbuf_get_rowstride (pixbuf);
                compare_rowstride = bdk_pixbuf_get_rowstride (compare);

                width = bdk_pixbuf_get_width (pixbuf);
                height = bdk_pixbuf_get_height (pixbuf);

                /*  well...  */
                bytes_per_pixel = bdk_pixbuf_get_n_channels (pixbuf);

                p1 = orig_pixels;
                p2 = compare_pixels;

                for (y = 0; y < height; y++) {
                        for (x = 0; x < width * bytes_per_pixel; x++)
                                count += (*p1++ != *p2++);

                        orig_pixels += orig_rowstride;
                        compare_pixels += compare_rowstride;

                        p1 = orig_pixels;
                        p2 = compare_pixels;
                }

                if (count > 0) {
                        fprintf (stderr,
                                 "saved %s file differs from copy in memory\n",
                                 file_type);
                }
        }
}

static gboolean
save_to_loader (const gchar *buf, gsize count, GError **err, gpointer data)
{
        BdkPixbufLoader *loader = data;

        return bdk_pixbuf_loader_write (loader, (const guchar *)buf, count, err);
}

static BdkPixbuf *
buffer_to_pixbuf (const gchar *buf, gsize count, GError **err)
{
        BdkPixbufLoader *loader;
        BdkPixbuf *pixbuf;

        loader = bdk_pixbuf_loader_new ();
        if (bdk_pixbuf_loader_write (loader, (const guchar *)buf, count, err) &&
            bdk_pixbuf_loader_close (loader, err)) {
                pixbuf = g_object_ref (bdk_pixbuf_loader_get_pixbuf (loader));
                g_object_unref (loader);
                return pixbuf;
        } else {
                return NULL;
        }
}

static void
do_compare (BdkPixbuf *pixbuf, BdkPixbuf *compare, GError *err)
{
        if (compare == NULL) {
                fprintf (stderr, "%s", err->message);
                g_error_free (err);
        } else {
                compare_pixbufs (pixbuf, compare, "jpeg");
                g_object_unref (compare);
        }
}

static void
keypress_check (BtkWidget *widget, BdkEventKey *evt, gpointer data)
{
        BdkPixbuf *pixbuf;
        BtkDrawingArea *da = (BtkDrawingArea*)data;
        GError *err = NULL;
        gchar *buffer;
        gsize count;
        BdkPixbufLoader *loader;

        pixbuf = (BdkPixbuf *) g_object_get_data (B_OBJECT (da), "pixbuf");

        if (evt->keyval == 'q')
                btk_main_quit ();

        if (evt->keyval == 's' && (evt->state & BDK_CONTROL_MASK)) {
                /* save to callback */
                if (pixbuf == NULL) {
                        fprintf (stderr, "PIXBUF NULL\n");
                        return;
                }	

                loader = bdk_pixbuf_loader_new ();
                if (!bdk_pixbuf_save_to_callback (pixbuf, save_to_loader, loader, "jpeg",
                                                  &err,
                                                  "quality", "100",
                                                  NULL) ||
                    !bdk_pixbuf_loader_close (loader, &err)) {
                        fprintf (stderr, "%s", err->message);
                        g_error_free (err);
                } else {
                        do_compare (pixbuf,
                                    g_object_ref (bdk_pixbuf_loader_get_pixbuf (loader)),
                                    err);
                        g_object_unref (loader);
                }
        }
        else if (evt->keyval == 'S') {
                /* save to buffer */
                if (!bdk_pixbuf_save_to_buffer (pixbuf, &buffer, &count, "jpeg",
                                                &err,
                                                "quality", "100",
                                                NULL)) {
                        fprintf (stderr, "%s", err->message);
                        g_error_free (err);
                } else {
                        do_compare (pixbuf,
                                    buffer_to_pixbuf (buffer, count, &err),
                                    err);
                }
        }
        else if (evt->keyval == 's') {
                /* save normally */
                if (pixbuf == NULL) {
                        fprintf (stderr, "PIXBUF NULL\n");
                        return;
                }	

                if (!bdk_pixbuf_save (pixbuf, "foo.jpg", "jpeg",
                                      &err,
                                      "quality", "100",
                                      NULL)) {
                        fprintf (stderr, "%s", err->message);
                        g_error_free (err);
                } else {
                        do_compare (pixbuf,
                                    bdk_pixbuf_new_from_file ("foo.jpg", &err),
                                    err);
                }
        }

        if (evt->keyval == 'p' && (evt->state & BDK_CONTROL_MASK)) {
                /* save to callback */
                if (pixbuf == NULL) {
                        fprintf (stderr, "PIXBUF NULL\n");
                        return;
                }

                loader = bdk_pixbuf_loader_new ();
                if (!bdk_pixbuf_save_to_callback (pixbuf, save_to_loader, loader, "png",
                                                  &err,
                                                  "tEXt::Software", "testpixbuf-save",
                                                  NULL)
                    || !bdk_pixbuf_loader_close (loader, &err)) {
                        fprintf (stderr, "%s", err->message);
                        g_error_free (err);
                } else {
                        do_compare (pixbuf,
                                    g_object_ref (bdk_pixbuf_loader_get_pixbuf (loader)),
                                    err);
                        g_object_unref (loader);
                }
        }
        else if (evt->keyval == 'P') {
                /* save to buffer */
                if (!bdk_pixbuf_save_to_buffer (pixbuf, &buffer, &count, "png",
                                                &err,
                                                "tEXt::Software", "testpixbuf-save",
                                                NULL)) {
                        fprintf (stderr, "%s", err->message);
                        g_error_free (err);
                } else {
                        do_compare (pixbuf,
                                    buffer_to_pixbuf (buffer, count, &err),
                                    err);
                }
        }
        else if (evt->keyval == 'p') {
                if (pixbuf == NULL) {
                        fprintf (stderr, "PIXBUF NULL\n");
                        return;
                }

                if (!bdk_pixbuf_save (pixbuf, "foo.png", "png", 
                                      &err,
                                      "tEXt::Software", "testpixbuf-save",
                                      NULL)) {
                        fprintf (stderr, "%s", err->message);
                        g_error_free (err);
                } else {
                        do_compare(pixbuf,
                                   bdk_pixbuf_new_from_file ("foo.png", &err),
                                   err);
                }
        }

        if (evt->keyval == 'i' && (evt->state & BDK_CONTROL_MASK)) {
                /* save to callback */
                if (pixbuf == NULL) {
                        fprintf (stderr, "PIXBUF NULL\n");
                        return;
                }

                loader = bdk_pixbuf_loader_new ();
                if (!bdk_pixbuf_save_to_callback (pixbuf, save_to_loader, loader, "ico",
                                                  &err,
                                                  NULL)
                    || !bdk_pixbuf_loader_close (loader, &err)) {
                        fprintf (stderr, "%s", err->message);
                        g_error_free (err);
                } else {
                        do_compare (pixbuf,
                                    g_object_ref (bdk_pixbuf_loader_get_pixbuf (loader)),
                                    err);
                        g_object_unref (loader);
                }
        }
        else if (evt->keyval == 'I') {
                /* save to buffer */
                if (!bdk_pixbuf_save_to_buffer (pixbuf, &buffer, &count, "ico",
                                                &err,
                                                NULL)) {
                        fprintf (stderr, "%s", err->message);
                        g_error_free (err);
                } else {
                        do_compare (pixbuf,
                                    buffer_to_pixbuf (buffer, count, &err),
                                    err);
                }
        }
        else if (evt->keyval == 'i') {
                if (pixbuf == NULL) {
                        fprintf (stderr, "PIXBUF NULL\n");
                        return;
                }

                if (!bdk_pixbuf_save (pixbuf, "foo.ico", "ico", 
                                      &err,
                                      NULL)) {
                        fprintf (stderr, "%s", err->message);
                        g_error_free (err);
                } else {
                        do_compare(pixbuf,
                                   bdk_pixbuf_new_from_file ("foo.ico", &err),
                                   err);
                }
        }

        if (evt->keyval == 'a') {
                if (pixbuf == NULL) {
                        fprintf (stderr, "PIXBUF NULL\n");
                        return;
                } else {
                        BdkPixbuf *alpha_buf;

                        alpha_buf = bdk_pixbuf_add_alpha (pixbuf,
                                                          FALSE, 0, 0, 0);

                        g_object_set_data_full (B_OBJECT (da),
                                                "pixbuf", alpha_buf,
                                                (GDestroyNotify) g_object_unref);
                }
        }
}


static int
close_app (BtkWidget *widget, gpointer data)
{
        btk_main_quit ();
        return TRUE;
}

static int
expose_cb (BtkWidget *drawing_area, BdkEventExpose *evt, gpointer data)
{
        BdkPixbuf *pixbuf;
        bairo_t *cr;
         
        pixbuf = (BdkPixbuf *) g_object_get_data (B_OBJECT (drawing_area),
						  "pixbuf");

        cr = bdk_bairo_create (evt->window);
        bdk_bairo_set_source_pixbuf (cr, pixbuf, 0, 0);
        bdk_bairo_rectangle (cr, &evt->area);
        bairo_fill (cr);

        bairo_destroy (cr);

        return FALSE;
}

static int
configure_cb (BtkWidget *drawing_area, BdkEventConfigure *evt, gpointer data)
{
        BdkPixbuf *pixbuf;
                           
        pixbuf = (BdkPixbuf *) g_object_get_data (B_OBJECT (drawing_area),   
						  "pixbuf");
    
        g_print ("X:%d Y:%d\n", evt->width, evt->height);
        if (evt->width != bdk_pixbuf_get_width (pixbuf) || evt->height != bdk_pixbuf_get_height (pixbuf)) {
                BdkWindow *root;
                BdkPixbuf *new_pixbuf;

                root = bdk_get_default_root_window ();
                new_pixbuf = bdk_pixbuf_get_from_drawable (NULL, root, NULL,
                                                           0, 0, 0, 0, evt->width, evt->height);
                g_object_set_data_full (B_OBJECT (drawing_area), "pixbuf", new_pixbuf,
                                        (GDestroyNotify) g_object_unref);
        }

        return FALSE;
}

int
main (int argc, char **argv)
{   
        BdkWindow     *root;
        BtkWidget     *window;
        BtkWidget     *vbox;
        BtkWidget     *drawing_area;
        BdkPixbuf     *pixbuf;    
   
        btk_init (&argc, &argv);   

        root = bdk_get_default_root_window ();
        pixbuf = bdk_pixbuf_get_from_drawable (NULL, root, NULL,
                                               0, 0, 0, 0, 150, 160);
   
        window = btk_window_new (BTK_WINDOW_TOPLEVEL);
        g_signal_connect (window, "delete_event",
			  G_CALLBACK (close_app), NULL);
        g_signal_connect (window, "destroy",   
			  G_CALLBACK (close_app), NULL);
   
        vbox = btk_vbox_new (FALSE, 0);
        btk_container_add (BTK_CONTAINER (window), vbox);  
   
        drawing_area = btk_drawing_area_new ();
        btk_widget_set_size_request (BTK_WIDGET (drawing_area),
                                     bdk_pixbuf_get_width (pixbuf),
                                     bdk_pixbuf_get_height (pixbuf));
        g_signal_connect (drawing_area, "expose_event",
			  G_CALLBACK (expose_cb), NULL);

        g_signal_connect (drawing_area, "configure_event",
			  G_CALLBACK (configure_cb), NULL);
        g_signal_connect (window, "key_press_event", 
			  G_CALLBACK (keypress_check), drawing_area);    
        g_object_set_data_full (B_OBJECT (drawing_area), "pixbuf", pixbuf,
                                (GDestroyNotify) g_object_unref);
        btk_box_pack_start (BTK_BOX (vbox), drawing_area, TRUE, TRUE, 0);
   
        btk_widget_show_all (window);
        btk_main ();
        return 0;
}
