/* testbuttons.c
 * Copyright (C) 2009 Red Hat, Inc.
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

/* various combinations of use_underline and use_stock */

int main (int argc, char *argv[])
{
	BtkWidget *window, *box, *button, *hbox;
        bchar *text;
	bboolean use_underline, use_stock;
	BtkWidget *image, *label;

	btk_init (&argc, &argv);

	window = btk_window_new (BTK_WINDOW_TOPLEVEL);

	box = btk_vbox_new (0, FALSE);

	btk_container_add (BTK_CONTAINER (window), box);

	hbox = btk_hbox_new (0, FALSE);
	btk_container_add (BTK_CONTAINER (box), hbox);
	button = btk_button_new_from_stock (BTK_STOCK_SAVE);
	btk_container_add (BTK_CONTAINER (hbox), button);

	g_object_get (button,
                      "label", &text,
                      "use-stock", &use_stock,
                      "use-underline", &use_underline,
		      "image", &image,
                      NULL);
	text = g_strdup_printf ("label: \"%s\" image: %p use-stock: %s use-underline: %s\n", text, image, use_stock ? "TRUE" : "FALSE", use_underline ? "TRUE" : "FALSE");
	label = btk_label_new (text);
	g_free (text);
	btk_container_add (BTK_CONTAINER (hbox), label);

	hbox = btk_hbox_new (0, FALSE);
	btk_container_add (BTK_CONTAINER (box), hbox);
	button = g_object_new (BTK_TYPE_BUTTON,
                               "label", "btk-save",
			       "use-stock", TRUE,
			       NULL);
	btk_container_add (BTK_CONTAINER (hbox), button);

	g_object_get (button,
                      "label", &text,
                      "use-stock", &use_stock,
                      "use-underline", &use_underline,
		      "image", &image,
                      NULL);
	text = g_strdup_printf ("label: \"%s\" image: %p use-stock: %s use-underline: %s\n", text, image, use_stock ? "TRUE" : "FALSE", use_underline ? "TRUE" : "FALSE");
	label = btk_label_new (text);
	g_free (text);
	btk_container_add (BTK_CONTAINER (hbox), label);

	hbox = btk_hbox_new (0, FALSE);
	btk_container_add (BTK_CONTAINER (box), hbox);
	button = btk_button_new_with_label ("_Save");
	btk_container_add (BTK_CONTAINER (hbox), button);

	g_object_get (button,
                      "label", &text,
                      "use-stock", &use_stock,
                      "use-underline", &use_underline,
		      "image", &image,
                      NULL);
	text = g_strdup_printf ("label: \"%s\" image: %p use-stock: %s use-underline: %s\n", text, image, use_stock ? "TRUE" : "FALSE", use_underline ? "TRUE" : "FALSE");
	label = btk_label_new (text);
	g_free (text);
	btk_container_add (BTK_CONTAINER (hbox), label);

	hbox = btk_hbox_new (0, FALSE);
	btk_container_add (BTK_CONTAINER (box), hbox);
	button = btk_button_new_with_mnemonic ("_Save");
	btk_container_add (BTK_CONTAINER (hbox), button);

	g_object_get (button,
                      "label", &text,
                      "use-stock", &use_stock,
                      "use-underline", &use_underline,
		      "image", &image,
                      NULL);
	text = g_strdup_printf ("label: \"%s\" image: %p use-stock: %s use-underline: %s\n", text, image, use_stock ? "TRUE" : "FALSE", use_underline ? "TRUE" : "FALSE");
	label = btk_label_new (text);
	g_free (text);
	btk_container_add (BTK_CONTAINER (hbox), label);

	hbox = btk_hbox_new (0, FALSE);
	btk_container_add (BTK_CONTAINER (box), hbox);
	button = btk_button_new_with_label ("_Save");
	btk_button_set_image (BTK_BUTTON (button), btk_image_new_from_stock (BTK_STOCK_ABOUT, BTK_ICON_SIZE_BUTTON));
	btk_container_add (BTK_CONTAINER (hbox), button);

	g_object_get (button,
                      "label", &text,
                      "use-stock", &use_stock,
                      "use-underline", &use_underline,
		      "image", &image,
                      NULL);
	text = g_strdup_printf ("label: \"%s\" image: %p use-stock: %s use-underline: %s\n", text, image, use_stock ? "TRUE" : "FALSE", use_underline ? "TRUE" : "FALSE");
	label = btk_label_new (text);
	g_free (text);
	btk_container_add (BTK_CONTAINER (hbox), label);

	hbox = btk_hbox_new (0, FALSE);
	btk_container_add (BTK_CONTAINER (box), hbox);
	button = btk_button_new_with_mnemonic ("_Save");
	btk_button_set_image (BTK_BUTTON (button), btk_image_new_from_stock (BTK_STOCK_ABOUT, BTK_ICON_SIZE_BUTTON));
	btk_container_add (BTK_CONTAINER (hbox), button);
	g_object_get (button,
                      "label", &text,
                      "use-stock", &use_stock,
                      "use-underline", &use_underline,
		      "image", &image,
                      NULL);
	text = g_strdup_printf ("label: \"%s\" image: %p use-stock: %s use-underline: %s\n", text, image, use_stock ? "TRUE" : "FALSE", use_underline ? "TRUE" : "FALSE");
	label = btk_label_new (text);
	g_free (text);
	btk_container_add (BTK_CONTAINER (hbox), label);

	btk_widget_show_all (window);

	btk_main ();

	return 0;
}

