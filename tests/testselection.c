/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#undef BTK_DISABLE_DEPRECATED

#include "config.h"
#include <stdio.h>
#include <string.h>
#define BTK_ENABLE_BROKEN
#include "btk/btk.h"

typedef enum {
  SEL_TYPE_NONE,
  APPLE_PICT,
  ATOM,
  ATOM_PAIR,
  BITMAP,
  C_STRING,
  COLORMAP,
  COMPOUND_TEXT,
  DRAWABLE,
  INTEGER,
  PIXEL,
  PIXMAP,
  SPAN,
  STRING,
  TEXT,
  WINDOW,
  LAST_SEL_TYPE
} SelType;

BdkAtom seltypes[LAST_SEL_TYPE];

typedef struct _Target {
  bchar *target_name;
  SelType type;
  BdkAtom target;
  bint format;
} Target;

/* The following is a list of all the selection targets defined
   in the ICCCM */

static Target targets[] = {
  { "ADOBE_PORTABLE_DOCUMENT_FORMAT",	    STRING, 	   NULL, 8 },
  { "APPLE_PICT", 			    APPLE_PICT,    NULL, 8 },
  { "BACKGROUND",			    PIXEL,         NULL, 32 },
  { "BITMAP", 				    BITMAP,        NULL, 32 },
  { "CHARACTER_POSITION",                   SPAN, 	   NULL, 32 },
  { "CLASS", 				    TEXT, 	   NULL, 8 },
  { "CLIENT_WINDOW", 			    WINDOW, 	   NULL, 32 },
  { "COLORMAP", 			    COLORMAP,      NULL, 32 },
  { "COLUMN_NUMBER", 			    SPAN, 	   NULL, 32 },
  { "COMPOUND_TEXT", 			    COMPOUND_TEXT, NULL, 8 },
  /*  { "DELETE", "NULL", 0, ? }, */
  { "DRAWABLE", 			    DRAWABLE,      NULL, 32 },
  { "ENCAPSULATED_POSTSCRIPT", 		    STRING, 	   NULL, 8 },
  { "ENCAPSULATED_POSTSCRIPT_INTERCHANGE",  STRING, 	   NULL, 8 },
  { "FILE_NAME", 			    TEXT, 	   NULL, 8 },
  { "FOREGROUND", 			    PIXEL, 	   NULL, 32 },
  { "HOST_NAME", 			    TEXT, 	   NULL, 8 },
  /*  { "INSERT_PROPERTY", "NULL", 0, ? NULL }, */
  /*  { "INSERT_SELECTION", "NULL", 0, ? NULL }, */
  { "LENGTH", 				    INTEGER, 	   NULL, 32 },
  { "LINE_NUMBER", 			    SPAN, 	   NULL, 32 },
  { "LIST_LENGTH", 			    INTEGER,       NULL, 32 },
  { "MODULE", 				    TEXT, 	   NULL, 8 },
  /*  { "MULTIPLE", "ATOM_PAIR", 0, 32 }, */
  { "NAME", 				    TEXT, 	   NULL, 8 },
  { "ODIF", 				    TEXT,          NULL, 8 },
  { "OWNER_OS", 			    TEXT, 	   NULL, 8 },
  { "PIXMAP", 				    PIXMAP,        NULL, 32 },
  { "POSTSCRIPT", 			    STRING,        NULL, 8 },
  { "PROCEDURE", 			    TEXT,          NULL, 8 },
  { "PROCESS",				    INTEGER,       NULL, 32 },
  { "STRING", 				    STRING,        NULL, 8 },
  { "TARGETS", 				    ATOM, 	   NULL, 32 },
  { "TASK", 				    INTEGER,       NULL, 32 },
  { "TEXT", 				    TEXT,          NULL, 8  },
  { "TIMESTAMP", 			    INTEGER,       NULL, 32 },
  { "USER", 				    TEXT, 	   NULL, 8 },
};

static int num_targets = sizeof(targets)/sizeof(Target);

static int have_selection = FALSE;

BtkWidget *selection_widget;
BtkWidget *selection_text;
BtkWidget *selection_button;
GString *selection_string = NULL;

static void
init_atoms (void)
{
  int i;

  seltypes[SEL_TYPE_NONE] = BDK_NONE;
  seltypes[APPLE_PICT] = bdk_atom_intern ("APPLE_PICT",FALSE);
  seltypes[ATOM]       = bdk_atom_intern ("ATOM",FALSE);
  seltypes[ATOM_PAIR]  = bdk_atom_intern ("ATOM_PAIR",FALSE);
  seltypes[BITMAP]     = bdk_atom_intern ("BITMAP",FALSE);
  seltypes[C_STRING]   = bdk_atom_intern ("C_STRING",FALSE);
  seltypes[COLORMAP]   = bdk_atom_intern ("COLORMAP",FALSE);
  seltypes[COMPOUND_TEXT] = bdk_atom_intern ("COMPOUND_TEXT",FALSE);
  seltypes[DRAWABLE]   = bdk_atom_intern ("DRAWABLE",FALSE);
  seltypes[INTEGER]    = bdk_atom_intern ("INTEGER",FALSE);
  seltypes[PIXEL]      = bdk_atom_intern ("PIXEL",FALSE);
  seltypes[PIXMAP]     = bdk_atom_intern ("PIXMAP",FALSE);
  seltypes[SPAN]       = bdk_atom_intern ("SPAN",FALSE);
  seltypes[STRING]     = bdk_atom_intern ("STRING",FALSE);
  seltypes[TEXT]       = bdk_atom_intern ("TEXT",FALSE);
  seltypes[WINDOW]     = bdk_atom_intern ("WINDOW",FALSE);

  for (i=0; i<num_targets; i++)
    targets[i].target = bdk_atom_intern (targets[i].target_name, FALSE);
}

void
selection_toggled (BtkWidget *widget)
{
  if (BTK_TOGGLE_BUTTON(widget)->active)
    {
      have_selection = btk_selection_owner_set (selection_widget,
						BDK_SELECTION_PRIMARY,
						BDK_CURRENT_TIME);
      if (!have_selection)
	btk_toggle_button_set_active (BTK_TOGGLE_BUTTON(widget), FALSE);
    }
  else
    {
      if (have_selection)
	{
	  if (bdk_selection_owner_get (BDK_SELECTION_PRIMARY) == widget->window)
	    btk_selection_owner_set (NULL, BDK_SELECTION_PRIMARY,
				     BDK_CURRENT_TIME);
	  have_selection = FALSE;
	}
    }
}

void
selection_get (BtkWidget *widget, 
	       BtkSelectionData *selection_data,
	       buint      info,
	       buint      time,
	       bpointer   data)
{
  buchar *buffer;
  bint len;
  BdkAtom type = BDK_NONE;

  if (!selection_string)
    {
      buffer = NULL;
      len = 0;
    }      
  else
    {
      buffer = (buchar *)selection_string->str;
      len = selection_string->len;
    }

  switch (info)
    {
    case COMPOUND_TEXT:
    case TEXT:
      type = seltypes[COMPOUND_TEXT];
    case STRING:
      type = seltypes[STRING];
    }
  
  btk_selection_data_set (selection_data, type, 8, buffer, len);
}

bint
selection_clear (BtkWidget *widget, BdkEventSelection *event)
{
  have_selection = FALSE;
  btk_toggle_button_set_active (BTK_TOGGLE_BUTTON(selection_button), FALSE);

  return TRUE;
}

bchar *
stringify_atom (buchar *data, bint *position)
{
  bchar *str = bdk_atom_name (*(BdkAtom *)(data+*position));
  *position += sizeof(BdkAtom);
    
  return str;
}

bchar *
stringify_text (buchar *data, bint *position)
{
  bchar *str = g_strdup ((bchar *)(data+*position));
  *position += strlen (str) + 1;
    
  return str;
}

bchar *
stringify_xid (buchar *data, bint *position)
{
  bchar buffer[20];
  bchar *str;

  sprintf(buffer,"0x%x",*(buint32 *)(data+*position));
  str = g_strdup (buffer);

  *position += sizeof(buint32);
    
  return str;
}

bchar *
stringify_integer (buchar *data, bint *position)
{
  bchar buffer[20];
  bchar *str;

  sprintf(buffer,"%d",*(int *)(data+*position));
  str = g_strdup (buffer);

  *position += sizeof(int);
    
  return str;
}

bchar *
stringify_span (buchar *data, bint *position)
{
  bchar buffer[42];
  bchar *str;

  sprintf(buffer,"%d - %d",((int *)(data+*position))[0],
	  ((int *)(data+*position))[1]);
  str = g_strdup (buffer);

  *position += 2*sizeof(int);
    
  return str;
}

void
selection_received (BtkWidget *widget, BtkSelectionData *data)
{
  int position;
  int i;
  SelType seltype;
  char *str;
  
  if (data->length < 0)
    {
      g_print("Error retrieving selection\n");
      return;
    }

  seltype = SEL_TYPE_NONE;
  for (i=0; i<LAST_SEL_TYPE; i++)
    {
      if (seltypes[i] == data->type)
	{
	  seltype = i;
	  break;
	}
    }

  if (seltype == SEL_TYPE_NONE)
    {
      char *name = bdk_atom_name (data->type);
      g_print("Don't know how to handle type: %s\n",
	      name?name:"<unknown>");
      return;
    }

  if (selection_string != NULL)
    g_string_free (selection_string, TRUE);

  selection_string = g_string_new (NULL);

  btk_text_freeze (BTK_TEXT (selection_text));
  btk_text_set_point (BTK_TEXT (selection_text), 0);
  btk_text_forward_delete (BTK_TEXT (selection_text), 
			   btk_text_get_length (BTK_TEXT (selection_text)));

  position = 0;
  while (position < data->length)
    {
      switch (seltype)
	{
	case ATOM:
	  str = stringify_atom (data->data, &position);
	  break;
	case COMPOUND_TEXT:
	case STRING:
	case TEXT:
	  str = stringify_text (data->data, &position);
	  break;
	case BITMAP:
	case DRAWABLE:
	case PIXMAP:
	case WINDOW:
	case COLORMAP:
	  str = stringify_xid (data->data, &position);
	  break;
	case INTEGER:
	case PIXEL:
	  str = stringify_integer (data->data, &position);
	  break;
	case SPAN:
	  str = stringify_span (data->data, &position);
	  break;
	default:
	  {
	    char *name = bdk_atom_name (data->type);
	    g_print("Can't convert type %s to string\n",
		    name?name:"<unknown>");
	    position = data->length;
	    continue;
	  }
	}
      btk_text_insert (BTK_TEXT (selection_text), NULL, 
		       &selection_text->style->black, 
		       NULL, str, -1);
      btk_text_insert (BTK_TEXT (selection_text), NULL, 
		       &selection_text->style->black, 
		       NULL, "\n", -1);
      g_string_append (selection_string, str);
      g_free (str);
    }
  btk_text_thaw (BTK_TEXT (selection_text));
}

void
paste (BtkWidget *widget, BtkWidget *entry)
{
  const char *name;
  BdkAtom atom;

  name = btk_entry_get_text (BTK_ENTRY(entry));
  atom = bdk_atom_intern (name, FALSE);

  if (atom == BDK_NONE)
    {
      g_print("Could not create atom: \"%s\"\n",name);
      return;
    }

  btk_selection_convert (selection_widget, BDK_SELECTION_PRIMARY, atom, 
			 BDK_CURRENT_TIME);
}

void
quit (void)
{
  btk_exit (0);
}

int
main (int argc, char *argv[])
{
  BtkWidget *dialog;
  BtkWidget *button;
  BtkWidget *table;
  BtkWidget *label;
  BtkWidget *entry;
  BtkWidget *hscrollbar;
  BtkWidget *vscrollbar;
  BtkWidget *hbox;

  static BtkTargetEntry targetlist[] = {
    { "STRING",        0, STRING },
    { "TEXT",          0, TEXT },
    { "COMPOUND_TEXT", 0, COMPOUND_TEXT }
  };
  static bint ntargets = sizeof(targetlist) / sizeof(targetlist[0]);
  
  btk_init (&argc, &argv);

  init_atoms();

  selection_widget = btk_invisible_new ();

  dialog = btk_dialog_new ();
  btk_widget_set_name (dialog, "Test Input");
  btk_container_set_border_width (BTK_CONTAINER(dialog), 0);

  g_signal_connect (dialog, "destroy",
		    G_CALLBACK (quit), NULL);

  table = btk_table_new (4, 2, FALSE);
  btk_container_set_border_width (BTK_CONTAINER(table), 10);

  btk_table_set_row_spacing (BTK_TABLE (table), 0, 5);
  btk_table_set_row_spacing (BTK_TABLE (table), 1, 2);
  btk_table_set_row_spacing (BTK_TABLE (table), 2, 2);
  btk_table_set_col_spacing (BTK_TABLE (table), 0, 2);
  btk_box_pack_start (BTK_BOX (BTK_DIALOG(dialog)->vbox), 
		      table, TRUE, TRUE, 0);
  btk_widget_show (table);
  
  selection_button = btk_toggle_button_new_with_label ("Claim Selection");
  btk_table_attach (BTK_TABLE (table), selection_button, 0, 2, 0, 1,
		    BTK_EXPAND | BTK_FILL, 0, 0, 0);
  btk_widget_show (selection_button);

  g_signal_connect (selection_button, "toggled",
		    G_CALLBACK (selection_toggled), NULL);
  g_signal_connect (selection_widget, "selection_clear_event",
		    G_CALLBACK (selection_clear), NULL);
  g_signal_connect (selection_widget, "selection_received",
		    G_CALLBACK (selection_received), NULL);

  btk_selection_add_targets (selection_widget, BDK_SELECTION_PRIMARY,
			     targetlist, ntargets);

  g_signal_connect (selection_widget, "selection_get",
		    G_CALLBACK (selection_get), NULL);

  selection_text = btk_text_new (NULL, NULL);
  btk_table_attach_defaults (BTK_TABLE (table), selection_text, 0, 1, 1, 2);
  btk_widget_show (selection_text);
  
  hscrollbar = btk_hscrollbar_new (BTK_TEXT (selection_text)->hadj);
  btk_table_attach (BTK_TABLE (table), hscrollbar, 0, 1, 2, 3,
		    BTK_EXPAND | BTK_FILL, BTK_FILL, 0, 0);
  btk_widget_show (hscrollbar);
  
  vscrollbar = btk_vscrollbar_new (BTK_TEXT (selection_text)->vadj);
  btk_table_attach (BTK_TABLE (table), vscrollbar, 1, 2, 1, 2,
		    BTK_FILL, BTK_EXPAND | BTK_FILL, 0, 0);
  btk_widget_show (vscrollbar);

  hbox = btk_hbox_new (FALSE, 2);
  btk_table_attach (BTK_TABLE (table), hbox, 0, 2, 3, 4,
		    BTK_EXPAND | BTK_FILL, 0, 0, 0);
  btk_widget_show (hbox);

  label = btk_label_new ("Target:");
  btk_box_pack_start (BTK_BOX(hbox), label, FALSE, FALSE, 0);
  btk_widget_show (label);

  entry = btk_entry_new ();
  btk_box_pack_start (BTK_BOX(hbox), entry, TRUE, TRUE, 0);
  btk_widget_show (entry);

  /* .. And create some buttons */
  button = btk_button_new_with_label ("Paste");
  btk_box_pack_start (BTK_BOX (BTK_DIALOG(dialog)->action_area), 
		      button, TRUE, TRUE, 0);
  g_signal_connect (button, "clicked",
		    G_CALLBACK (paste), entry);
  btk_widget_show (button);

  button = btk_button_new_with_label ("Quit");
  btk_box_pack_start (BTK_BOX (BTK_DIALOG(dialog)->action_area), 
		      button, TRUE, TRUE, 0);

  g_signal_connect_swapped (button, "clicked",
			    G_CALLBACK (btk_widget_destroy), dialog);
  btk_widget_show (button);

  btk_widget_show (dialog);

  btk_main ();

  return 0;
}
