/* testsocket_child.c
 * Copyright (C) 2001 Red Hat, Inc
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

#include "config.h"
#include <stdio.h>
#include <stdlib.h>

#include <btk/btk.h>

extern buint32 create_child_plug (buint32  xid,
				  bboolean local);

int
main (int argc, char *argv[])
{
  buint32 xid;
  buint32 plug_xid;

  btk_init (&argc, &argv);

  if (argc != 1 && argc != 2)
    {
      fprintf (stderr, "Usage: testsocket_child [WINDOW_ID]\n");
      exit (1);
    }

  if (argc == 2)
    {
      xid = strtol (argv[1], NULL, 0);
      if (xid == 0)
	{
	  fprintf (stderr, "Invalid window id '%s'\n", argv[1]);
	  exit (1);
	}
      
      create_child_plug (xid, FALSE);
    }
  else
    {
      plug_xid = create_child_plug (0, FALSE);
      printf ("%d\n", plug_xid);
      fflush (stdout);
    }

  btk_main ();

  return 0;
}


