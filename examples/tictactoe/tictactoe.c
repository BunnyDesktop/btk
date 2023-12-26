
/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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
#include <btk/btksignal.h>
#include <btk/btktable.h>
#include <btk/btktogglebutton.h>
#include "tictactoe.h"

enum {
  TICTACTOE_SIGNAL,
  LAST_SIGNAL
};

static void tictactoe_class_init          (TictactoeClass *klass);
static void tictactoe_init                (Tictactoe      *ttt);
static void tictactoe_toggle              (BtkWidget *widget, Tictactoe *ttt);

static guint tictactoe_signals[LAST_SIGNAL] = { 0 };

GType
tictactoe_get_type (void)
{
  static GType ttt_type = 0;

  if (!ttt_type)
    {
      const GTypeInfo ttt_info =
      {
	sizeof (TictactoeClass),
	NULL, /* base_init */
        NULL, /* base_finalize */
	(GClassInitFunc) tictactoe_class_init,
        NULL, /* class_finalize */
	NULL, /* class_data */
        sizeof (Tictactoe),
	0,
	(GInstanceInitFunc) tictactoe_init,
      };

      ttt_type = g_type_register_static (BTK_TYPE_TABLE, "Tictactoe", &ttt_info, 0);
    }

  return ttt_type;
}

static void
tictactoe_class_init (TictactoeClass *klass)
{
  
  tictactoe_signals[TICTACTOE_SIGNAL] = g_signal_new ("tictactoe",
					 B_TYPE_FROM_CLASS (klass),
	                                 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
	                                 G_STRUCT_OFFSET (TictactoeClass, tictactoe),
                                         NULL, 
                                         NULL,                
					 g_cclosure_marshal_VOID__VOID,
                                         B_TYPE_NONE, 0);


}

static void
tictactoe_init (Tictactoe *ttt)
{
  gint i,j;
  
  btk_table_resize (BTK_TABLE (ttt), 3, 3);
  btk_table_set_homogeneous (BTK_TABLE (ttt), TRUE);

  for (i=0;i<3; i++)
    for (j=0;j<3; j++)      {
	ttt->buttons[i][j] = btk_toggle_button_new ();
	btk_table_attach_defaults (BTK_TABLE (ttt), ttt->buttons[i][j], 
				   i, i+1, j, j+1);
	g_signal_connect (B_OBJECT (ttt->buttons[i][j]), "toggled",
			  G_CALLBACK (tictactoe_toggle), (gpointer) ttt);
	btk_widget_set_size_request (ttt->buttons[i][j], 20, 20);
	btk_widget_show (ttt->buttons[i][j]);
      }
}

BtkWidget*
tictactoe_new ()
{
  return BTK_WIDGET (g_object_new (tictactoe_get_type (), NULL));
}

void	       
tictactoe_clear (Tictactoe *ttt)
{
  int i,j;

  for (i = 0; i<3; i++)
    for (j = 0; j<3; j++)
      {
	g_signal_handlers_block_matched (B_OBJECT (ttt->buttons[i][j]), 
                                         G_SIGNAL_MATCH_DATA,
                                         0, 0, NULL, NULL, ttt);
	btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (ttt->buttons[i][j]),
				      FALSE);
	g_signal_handlers_unblock_matched (B_OBJECT (ttt->buttons[i][j]),
                                           G_SIGNAL_MATCH_DATA,
                                           0, 0, NULL, NULL, ttt);
      }
}

static void
tictactoe_toggle (BtkWidget *widget, Tictactoe *ttt)
{
  int i,k;

  static int rwins[8][3] = { { 0, 0, 0 }, { 1, 1, 1 }, { 2, 2, 2 },
			     { 0, 1, 2 }, { 0, 1, 2 }, { 0, 1, 2 },
			     { 0, 1, 2 }, { 0, 1, 2 } };
  static int cwins[8][3] = { { 0, 1, 2 }, { 0, 1, 2 }, { 0, 1, 2 },
			     { 0, 0, 0 }, { 1, 1, 1 }, { 2, 2, 2 },
			     { 0, 1, 2 }, { 2, 1, 0 } };

  int success, found;

  for (k = 0; k<8; k++)
    {
      success = TRUE;
      found = FALSE;

      for (i = 0; i<3; i++)
	{
	  success = success && 
	    BTK_TOGGLE_BUTTON (ttt->buttons[rwins[k][i]][cwins[k][i]])->active;
	  found = found ||
	    ttt->buttons[rwins[k][i]][cwins[k][i]] == widget;
	}
      
      if (success && found)
	{
	  g_signal_emit (B_OBJECT (ttt), 
	                 tictactoe_signals[TICTACTOE_SIGNAL], 0);
	  break;
	}
    }
}

