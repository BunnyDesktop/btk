/* BTK - The GIMP Toolkit
 * btkprinteroption.c: Handling possible settings for a specific printer setting
 * Copyright (C) 2006, Red Hat, Inc.
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
#include <string.h>
#include <bmodule.h>

#include "btkprinteroption.h"
#include "btkalias.h"

/*****************************************
 *            BtkPrinterOption           *
 *****************************************/

enum {
  CHANGED,
  LAST_SIGNAL
};

static buint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (BtkPrinterOption, btk_printer_option, B_TYPE_OBJECT)

static void
btk_printer_option_finalize (BObject *object)
{
  BtkPrinterOption *option = BTK_PRINTER_OPTION (object);
  int i;
  
  g_free (option->name);
  g_free (option->display_text);
  g_free (option->value);
  for (i = 0; i < option->num_choices; i++)
    {
      g_free (option->choices[i]);
      g_free (option->choices_display[i]);
    }
  g_free (option->choices);
  g_free (option->choices_display);
  g_free (option->group);
  
  B_OBJECT_CLASS (btk_printer_option_parent_class)->finalize (object);
}

static void
btk_printer_option_init (BtkPrinterOption *option)
{
  option->value = g_strdup ("");
  option->activates_default = FALSE;
}

static void
btk_printer_option_class_init (BtkPrinterOptionClass *class)
{
  BObjectClass *bobject_class = (BObjectClass *)class;

  bobject_class->finalize = btk_printer_option_finalize;

  signals[CHANGED] =
    g_signal_new ("changed",
		  B_TYPE_FROM_CLASS (class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkPrinterOptionClass, changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  B_TYPE_NONE, 0);
}

BtkPrinterOption *
btk_printer_option_new (const char *name, const char *display_text,
			BtkPrinterOptionType type)
{
  BtkPrinterOption *option;

  option = g_object_new (BTK_TYPE_PRINTER_OPTION, NULL);

  option->name = g_strdup (name);
  option->display_text = g_strdup (display_text);
  option->type = type;
  
  return option;
}

static void
emit_changed (BtkPrinterOption *option)
{
  g_signal_emit (option, signals[CHANGED], 0);
}

void
btk_printer_option_set (BtkPrinterOption *option,
			const char *value)
{
  if (value == NULL)
    value = "";
  
  if (strcmp (option->value, value) == 0)
    return;

  if ((option->type == BTK_PRINTER_OPTION_TYPE_PICKONE ||
       option->type == BTK_PRINTER_OPTION_TYPE_ALTERNATIVE) &&
      value != NULL)
    {
      int i;
      
      for (i = 0; i < option->num_choices; i++)
	{
	  if (g_ascii_strcasecmp (value, option->choices[i]) == 0)
	    {
	      value = option->choices[i];
	      break;
	    }
	}

      if (i == option->num_choices)
	return; /* Not found in available choices */
    }
  
  g_free (option->value);
  option->value = g_strdup (value);
  
  emit_changed (option);
}

void
btk_printer_option_set_boolean (BtkPrinterOption *option,
				bboolean value)
{
  btk_printer_option_set (option, value ? "True" : "False");
}

void
btk_printer_option_set_has_conflict  (BtkPrinterOption *option,
				      bboolean  has_conflict)
{
  has_conflict = has_conflict != 0;
  
  if (option->has_conflict == has_conflict)
    return;

  option->has_conflict = has_conflict;
  emit_changed (option);
}

void
btk_printer_option_clear_has_conflict (BtkPrinterOption     *option)
{
  btk_printer_option_set_has_conflict  (option, FALSE);
}

void
btk_printer_option_allocate_choices (BtkPrinterOption     *option,
				     int num)
{
  g_free (option->choices);
  g_free (option->choices_display);

  option->num_choices = num;
  if (num == 0)
    {
      option->choices = NULL;
      option->choices_display = NULL;
    }
  else
    {
      option->choices = g_new0 (char *, num);
      option->choices_display = g_new0 (char *, num);
    }
}

void
btk_printer_option_choices_from_array (BtkPrinterOption   *option,
				       int                 num_choices,
				       char               *choices[],
				       char              *choices_display[])
{
  int i;
  
  btk_printer_option_allocate_choices (option, num_choices);
  for (i = 0; i < num_choices; i++)
    {
      option->choices[i] = g_strdup (choices[i]);
      option->choices_display[i] = g_strdup (choices_display[i]);
    }
}

bboolean
btk_printer_option_has_choice (BtkPrinterOption     *option,
			       const char           *choice)
{
  int i;
  
  for (i = 0; i < option->num_choices; i++)
    {
      if (strcmp (option->choices[i], choice) == 0)
	return TRUE;
    }
  
  return FALSE;
}

void
btk_printer_option_set_activates_default (BtkPrinterOption *option,
					  bboolean          activates)
{
  g_return_if_fail (BTK_IS_PRINTER_OPTION (option));

  option->activates_default = activates;
}

bboolean
btk_printer_option_get_activates_default (BtkPrinterOption *option)
{
  g_return_val_if_fail (BTK_IS_PRINTER_OPTION (option), FALSE);

  return option->activates_default;
}


#define __BTK_PRINTER_OPTION_C__
#include "btkaliasdef.c"
