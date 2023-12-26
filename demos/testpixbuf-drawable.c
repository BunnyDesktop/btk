#include "config.h"

#undef BDK_DISABLE_DEPRECATED

#include <btk/btk.h>

int
close_app (BtkWidget *widget, gpointer data)
{
   btk_main_quit ();
   return TRUE;
}

int
expose_cb (BtkWidget *drawing_area, BdkEventExpose *evt, gpointer data)
{
   BdkPixbuf *pixbuf;
         
   pixbuf = (BdkPixbuf *) g_object_get_data (G_OBJECT (drawing_area), "pixbuf");
   if (bdk_pixbuf_get_has_alpha (pixbuf))
   {
      bdk_draw_rgb_32_image (drawing_area->window,
			     drawing_area->style->black_gc,
			     evt->area.x, evt->area.y,
			     evt->area.width,
			     evt->area.height,
			     BDK_RGB_DITHER_MAX,
			     bdk_pixbuf_get_pixels (pixbuf) +
			     (evt->area.y * bdk_pixbuf_get_rowstride (pixbuf)) +
			     (evt->area.x * bdk_pixbuf_get_n_channels (pixbuf)),
			     bdk_pixbuf_get_rowstride (pixbuf));
   }
   else
   {
      bdk_draw_rgb_image (drawing_area->window, 
			  drawing_area->style->black_gc, 
			  evt->area.x, evt->area.y,
			  evt->area.width,
			  evt->area.height,  
			  BDK_RGB_DITHER_NORMAL,
			  bdk_pixbuf_get_pixels (pixbuf) +
			  (evt->area.y * bdk_pixbuf_get_rowstride (pixbuf)) +
			  (evt->area.x * bdk_pixbuf_get_n_channels (pixbuf)),
			  bdk_pixbuf_get_rowstride (pixbuf));
   }
   return FALSE;
}

int
configure_cb (BtkWidget *drawing_area, BdkEventConfigure *evt, gpointer data)
{
   BdkPixbuf *pixbuf;
                           
   pixbuf = (BdkPixbuf *) g_object_get_data (G_OBJECT (drawing_area), "pixbuf");
    
   g_print ("X:%d Y:%d\n", evt->width, evt->height);
   if (evt->width != bdk_pixbuf_get_width (pixbuf) || evt->height != bdk_pixbuf_get_height (pixbuf))
   {
      BdkWindow *root;
      BdkPixbuf *new_pixbuf;

      root = bdk_get_default_root_window ();
      new_pixbuf = bdk_pixbuf_get_from_drawable (NULL, root, NULL,
						 0, 0, 0, 0, evt->width, evt->height);
      g_object_set_data (G_OBJECT (drawing_area), "pixbuf", new_pixbuf);
      g_object_unref (pixbuf);
   }

   return FALSE;
}

extern void pixbuf_init (void);

int
main (int argc, char **argv)
{   
   BdkWindow     *root;
   BtkWidget     *window;
   BtkWidget     *vbox;
   BtkWidget     *drawing_area;
   BdkPixbuf     *pixbuf;    
   
   pixbuf_init ();

   btk_init (&argc, &argv);   
   bdk_rgb_set_verbose (TRUE);

   btk_widget_set_default_colormap (bdk_rgb_get_colormap ());

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
   g_object_set_data (G_OBJECT (drawing_area), "pixbuf", pixbuf);
   btk_box_pack_start (BTK_BOX (vbox), drawing_area, TRUE, TRUE, 0);
   
   btk_widget_show_all (window);
   btk_main ();
   return 0;
}
