/* testimage.c
 * Copyright (C) 2004  Red Hat, Inc.
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

#include <btk/btk.h>
#include <bunnyio/bunnyio.h>

static void
drag_begin (BtkWidget      *widget,
	    BdkDragContext *context,
	    gpointer        data)
{
  BtkWidget *image = BTK_WIDGET (data);

  BdkPixbuf *pixbuf = btk_image_get_pixbuf (BTK_IMAGE (image));

  btk_drag_set_icon_pixbuf (context, pixbuf, -2, -2);
}

void  
drag_data_get  (BtkWidget        *widget,
		BdkDragContext   *context,
		BtkSelectionData *selection_data,
		guint             info,
		guint             time,
		gpointer          data)
{
  BtkWidget *image = BTK_WIDGET (data);

  BdkPixbuf *pixbuf = btk_image_get_pixbuf (BTK_IMAGE (image));

  btk_selection_data_set_pixbuf (selection_data, pixbuf);
}

static void
drag_data_received (BtkWidget        *widget,
		    BdkDragContext   *context,
		    gint              x,
		    gint              y,
		    BtkSelectionData *selection_data,
		    guint             info,
		    guint32           time,
		    gpointer          data)
{
  BtkWidget *image = BTK_WIDGET (data);

  BdkPixbuf *pixbuf;

  if (selection_data->length < 0)
    return;

  pixbuf = btk_selection_data_get_pixbuf (selection_data);

  btk_image_set_from_pixbuf (BTK_IMAGE (image), pixbuf);
}

static gboolean
idle_func (gpointer data)
{
  g_print ("keep me busy\n");

  return TRUE;
}

static gboolean
anim_image_expose (BtkWidget      *widget,
                   BdkEventExpose *eevent,
                   gpointer        data)
{
  g_print ("start busyness\n");

  g_signal_handlers_disconnect_by_func (widget, anim_image_expose, data);

  /* produce high load */
  g_idle_add_full (G_PRIORITY_DEFAULT,
                   idle_func, NULL, NULL);

  return FALSE;
}

int
main (int argc, char **argv)
{
  BtkWidget *window, *table;
  BtkWidget *label, *image, *box;
  BtkIconTheme *theme;
  BdkPixbuf *pixbuf;
  BtkIconSet *iconset;
  BtkIconSource *iconsource;
  gchar *icon_name = "bunny-terminal";
  gchar *anim_filename = NULL;
  GIcon *icon;
  GFile *file;

  btk_init (&argc, &argv);

  if (argc > 1)
    icon_name = argv[1];

  if (argc > 2)
    anim_filename = argv[2];

  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  table = btk_table_new (6, 3, FALSE);
  btk_container_add (BTK_CONTAINER (window), table);

  label = btk_label_new ("symbolic size");
  btk_table_attach (BTK_TABLE (table), label, 1, 2, 0, 1,
		    0, 0, 5, 5);
  label = btk_label_new ("fixed size");
  btk_table_attach (BTK_TABLE (table), label, 2, 3, 0, 1,
		    0, 0, 5, 5);

  label = btk_label_new ("BTK_IMAGE_PIXBUF");
  btk_table_attach_defaults (BTK_TABLE (table), label, 0, 1, 1, 2);

  theme = btk_icon_theme_get_default ();
  pixbuf = btk_icon_theme_load_icon (theme, icon_name, 48, 0, NULL);
  image = btk_image_new_from_pixbuf (pixbuf);
  box = btk_event_box_new ();
  btk_container_add (BTK_CONTAINER (box), image);
  btk_table_attach_defaults (BTK_TABLE (table), box, 2, 3, 1, 2);

  btk_drag_source_set (box, BDK_BUTTON1_MASK, 
		       NULL, 0,
		       BDK_ACTION_COPY);
  btk_drag_source_add_image_targets (box);
  g_signal_connect (box, "drag_begin", G_CALLBACK (drag_begin), image);
  g_signal_connect (box, "drag_data_get", G_CALLBACK (drag_data_get), image);

  btk_drag_dest_set (box,
                     BTK_DEST_DEFAULT_MOTION |
                     BTK_DEST_DEFAULT_HIGHLIGHT |
                     BTK_DEST_DEFAULT_DROP,
                     NULL, 0, BDK_ACTION_COPY);
  btk_drag_dest_add_image_targets (box);
  g_signal_connect (box, "drag_data_received", 
		    G_CALLBACK (drag_data_received), image);

  label = btk_label_new ("BTK_IMAGE_STOCK");
  btk_table_attach_defaults (BTK_TABLE (table), label, 0, 1, 2, 3);

  image = btk_image_new_from_stock (BTK_STOCK_REDO, BTK_ICON_SIZE_DIALOG);
  btk_table_attach_defaults (BTK_TABLE (table), image, 1, 2, 2, 3);

  label = btk_label_new ("BTK_IMAGE_ICON_SET");
  btk_table_attach_defaults (BTK_TABLE (table), label, 0, 1, 3, 4);

  iconsource = btk_icon_source_new ();
  btk_icon_source_set_icon_name (iconsource, icon_name);
  iconset = btk_icon_set_new ();
  btk_icon_set_add_source (iconset, iconsource);
  image = btk_image_new_from_icon_set (iconset, BTK_ICON_SIZE_DIALOG);
  btk_table_attach_defaults (BTK_TABLE (table), image, 1, 2, 3, 4);

  label = btk_label_new ("BTK_IMAGE_ICON_NAME");
  btk_table_attach_defaults (BTK_TABLE (table), label, 0, 1, 4, 5);
  image = btk_image_new_from_icon_name (icon_name, BTK_ICON_SIZE_DIALOG);
  btk_table_attach_defaults (BTK_TABLE (table), image, 1, 2, 4, 5);
  image = btk_image_new_from_icon_name (icon_name, BTK_ICON_SIZE_DIALOG);
  btk_image_set_pixel_size (BTK_IMAGE (image), 30);
  btk_table_attach_defaults (BTK_TABLE (table), image, 2, 3, 4, 5);

  label = btk_label_new ("BTK_IMAGE_GICON");
  btk_table_attach_defaults (BTK_TABLE (table), label, 0, 1, 5, 6);
  icon = g_themed_icon_new_with_default_fallbacks ("folder-remote");
  image = btk_image_new_from_gicon (icon, BTK_ICON_SIZE_DIALOG);
  g_object_unref (icon);
  btk_table_attach_defaults (BTK_TABLE (table), image, 1, 2, 5, 6);
  file = g_file_new_for_path ("apple-red.png");
  icon = g_file_icon_new (file);
  image = btk_image_new_from_gicon (icon, BTK_ICON_SIZE_DIALOG);
  g_object_unref (icon);
  btk_image_set_pixel_size (BTK_IMAGE (image), 30);
  btk_table_attach_defaults (BTK_TABLE (table), image, 2, 3, 5, 6);

  
  if (anim_filename)
    {
      label = btk_label_new ("BTK_IMAGE_ANIMATION (from file)");
      btk_table_attach_defaults (BTK_TABLE (table), label, 0, 1, 5, 6);
      image = btk_image_new_from_file (anim_filename);
      btk_image_set_pixel_size (BTK_IMAGE (image), 30);
      btk_table_attach_defaults (BTK_TABLE (table), image, 2, 3, 5, 6);

      /* produce high load */
      g_signal_connect_after (image, "expose-event",
                              G_CALLBACK (anim_image_expose),
                              NULL);
    }

  btk_widget_show_all (window);

  btk_main ();

  return 0;
}
