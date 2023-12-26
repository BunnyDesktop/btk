/* testspinbutton.c
 * Copyright (C) 2004 Morten Welinder
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

#include "config.h"
#include <btk/btk.h>

int
main (int argc, char **argv)
{
        BtkWidget *window, *mainbox;
	int max;

        btk_init (&argc, &argv);

        window = btk_window_new (BTK_WINDOW_TOPLEVEL);
        g_signal_connect (window, "delete_event", btk_main_quit, NULL);

        mainbox = btk_vbox_new (FALSE, 2);
        btk_container_add (BTK_CONTAINER (window), mainbox);

	for (max = 9; max <= 999999999; max = max * 10 + 9) {
		BtkAdjustment *adj =
			BTK_ADJUSTMENT (btk_adjustment_new (max,
							    1, max,
							    1,
							    (max + 1) / 10,
							    0.0));
     
		BtkWidget *spin = btk_spin_button_new (adj, 1.0, 0);
		BtkWidget *hbox = btk_hbox_new (FALSE, 2);
		
		btk_box_pack_start (BTK_BOX (hbox),
				    spin,
				    FALSE,
				    FALSE,
				    2);

		btk_container_add (BTK_CONTAINER (mainbox), hbox);

	}

        btk_widget_show_all (window);

        btk_main ();

        return 0;
}
