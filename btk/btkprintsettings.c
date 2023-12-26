/* BTK - The GIMP Toolkit
 * btkprintsettings.c: Print Settings
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
#include <stdlib.h>
#include <bunnylib/gprintf.h>
#include <btk/btk.h>
#include "btkprintsettings.h"
#include "btkprintutils.h"
#include "btkalias.h"


typedef struct _BtkPrintSettingsClass BtkPrintSettingsClass;

#define BTK_IS_PRINT_SETTINGS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_PRINT_SETTINGS))
#define BTK_PRINT_SETTINGS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_PRINT_SETTINGS, BtkPrintSettingsClass))
#define BTK_PRINT_SETTINGS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_PRINT_SETTINGS, BtkPrintSettingsClass))

struct _BtkPrintSettings
{
  GObject parent_instance;
  
  GHashTable *hash;
};

struct _BtkPrintSettingsClass
{
  GObjectClass parent_class;
};

#define KEYFILE_GROUP_NAME "Print Settings"

G_DEFINE_TYPE (BtkPrintSettings, btk_print_settings, G_TYPE_OBJECT)

static void
btk_print_settings_finalize (GObject *object)
{
  BtkPrintSettings *settings = BTK_PRINT_SETTINGS (object);

  g_hash_table_destroy (settings->hash);

  G_OBJECT_CLASS (btk_print_settings_parent_class)->finalize (object);
}

static void
btk_print_settings_init (BtkPrintSettings *settings)
{
  settings->hash = g_hash_table_new_full (g_str_hash, g_str_equal,
					  g_free, g_free);
}

static void
btk_print_settings_class_init (BtkPrintSettingsClass *class)
{
  GObjectClass *bobject_class = (GObjectClass *)class;

  bobject_class->finalize = btk_print_settings_finalize;
}

/**
 * btk_print_settings_new:
 * 
 * Creates a new #BtkPrintSettings object.
 *  
 * Return value: a new #BtkPrintSettings object
 *
 * Since: 2.10
 */
BtkPrintSettings *
btk_print_settings_new (void)
{
  return g_object_new (BTK_TYPE_PRINT_SETTINGS, NULL);
}

static void
copy_hash_entry  (gpointer  key,
		  gpointer  value,
		  gpointer  user_data)
{
  BtkPrintSettings *settings = user_data;

  g_hash_table_insert (settings->hash, 
		       g_strdup (key), 
		       g_strdup (value));
}



/**
 * btk_print_settings_copy:
 * @other: a #BtkPrintSettings
 *
 * Copies a #BtkPrintSettings object.
 *
 * Return value: (transfer full): a newly allocated copy of @other
 *
 * Since: 2.10
 */
BtkPrintSettings *
btk_print_settings_copy (BtkPrintSettings *other)
{
  BtkPrintSettings *settings;

  if (other == NULL)
    return NULL;
  
  g_return_val_if_fail (BTK_IS_PRINT_SETTINGS (other), NULL);

  settings = btk_print_settings_new ();

  g_hash_table_foreach (other->hash,
			copy_hash_entry,
			settings);

  return settings;
}

/**
 * btk_print_settings_get:
 * @settings: a #BtkPrintSettings
 * @key: a key
 * 
 * Looks up the string value associated with @key.
 * 
 * Return value: the string value for @key
 * 
 * Since: 2.10
 */
const gchar *
btk_print_settings_get (BtkPrintSettings *settings,
			const gchar      *key)
{
  return g_hash_table_lookup (settings->hash, key);
}

/**
 * btk_print_settings_set:
 * @settings: a #BtkPrintSettings
 * @key: a key
 * @value: (allow-none): a string value, or %NULL
 *
 * Associates @value with @key.
 *
 * Since: 2.10
 */
void
btk_print_settings_set (BtkPrintSettings *settings,
			const gchar      *key,
			const gchar      *value)
{
  if (value == NULL)
    btk_print_settings_unset (settings, key);
  else
    g_hash_table_insert (settings->hash, 
			 g_strdup (key), 
			 g_strdup (value));
}

/**
 * btk_print_settings_unset:
 * @settings: a #BtkPrintSettings
 * @key: a key
 * 
 * Removes any value associated with @key. 
 * This has the same effect as setting the value to %NULL.
 *
 * Since: 2.10 
 */
void
btk_print_settings_unset (BtkPrintSettings *settings,
			  const gchar      *key)
{
  g_hash_table_remove (settings->hash, key);
}

/**
 * btk_print_settings_has_key:
 * @settings: a #BtkPrintSettings
 * @key: a key
 * 
 * Returns %TRUE, if a value is associated with @key.
 * 
 * Return value: %TRUE, if @key has a value
 *
 * Since: 2.10
 */
gboolean        
btk_print_settings_has_key (BtkPrintSettings *settings,
			    const gchar      *key)
{
  return btk_print_settings_get (settings, key) != NULL;
}


/**
 * btk_print_settings_get_bool:
 * @settings: a #BtkPrintSettings
 * @key: a key
 * 
 * Returns the boolean represented by the value
 * that is associated with @key. 
 *
 * The string "true" represents %TRUE, any other 
 * string %FALSE.
 *
 * Return value: %TRUE, if @key maps to a true value.
 * 
 * Since: 2.10
 **/
gboolean
btk_print_settings_get_bool (BtkPrintSettings *settings,
			     const gchar      *key)
{
  const gchar *val;

  val = btk_print_settings_get (settings, key);
  if (g_strcmp0 (val, "true") == 0)
    return TRUE;
  
  return FALSE;
}

/**
 * btk_print_settings_get_bool_with_default:
 * @settings: a #BtkPrintSettings
 * @key: a key
 * @default_val: the default value
 * 
 * Returns the boolean represented by the value
 * that is associated with @key, or @default_val
 * if the value does not represent a boolean.
 *
 * The string "true" represents %TRUE, the string
 * "false" represents %FALSE.
 *
 * Return value: the boolean value associated with @key
 * 
 * Since: 2.10
 */
static gboolean
btk_print_settings_get_bool_with_default (BtkPrintSettings *settings,
					  const gchar      *key,
					  gboolean          default_val)
{
  const gchar *val;

  val = btk_print_settings_get (settings, key);
  if (g_strcmp0 (val, "true") == 0)
    return TRUE;

  if (g_strcmp0 (val, "false") == 0)
    return FALSE;
  
  return default_val;
}

/**
 * btk_print_settings_set_bool:
 * @settings: a #BtkPrintSettings
 * @key: a key
 * @value: a boolean
 * 
 * Sets @key to a boolean value.
 *
 * Since: 2.10
 */
void
btk_print_settings_set_bool (BtkPrintSettings *settings,
			     const gchar      *key,
			     gboolean          value)
{
  if (value)
    btk_print_settings_set (settings, key, "true");
  else
    btk_print_settings_set (settings, key, "false");
}

/**
 * btk_print_settings_get_double_with_default:
 * @settings: a #BtkPrintSettings
 * @key: a key
 * @def: the default value
 * 
 * Returns the floating point number represented by 
 * the value that is associated with @key, or @default_val
 * if the value does not represent a floating point number.
 *
 * Floating point numbers are parsed with g_ascii_strtod().
 *
 * Return value: the floating point number associated with @key
 * 
 * Since: 2.10
 */
gdouble
btk_print_settings_get_double_with_default (BtkPrintSettings *settings,
					    const gchar      *key,
					    gdouble           def)
{
  const gchar *val;

  val = btk_print_settings_get (settings, key);
  if (val == NULL)
    return def;

  return g_ascii_strtod (val, NULL);
}

/**
 * btk_print_settings_get_double:
 * @settings: a #BtkPrintSettings
 * @key: a key
 * 
 * Returns the double value associated with @key, or 0.
 * 
 * Return value: the double value of @key
 *
 * Since: 2.10
 */
gdouble
btk_print_settings_get_double (BtkPrintSettings *settings,
			       const gchar      *key)
{
  return btk_print_settings_get_double_with_default (settings, key, 0.0);
}

/**
 * btk_print_settings_set_double:
 * @settings: a #BtkPrintSettings
 * @key: a key 
 * @value: a double value
 * 
 * Sets @key to a double value.
 * 
 * Since: 2.10
 */
void
btk_print_settings_set_double (BtkPrintSettings *settings,
			       const gchar      *key,
			       gdouble           value)
{
  gchar buf[G_ASCII_DTOSTR_BUF_SIZE];
  
  g_ascii_dtostr (buf, G_ASCII_DTOSTR_BUF_SIZE, value);
  btk_print_settings_set (settings, key, buf);
}

/**
 * btk_print_settings_get_length:
 * @settings: a #BtkPrintSettings
 * @key: a key
 * @unit: the unit of the return value
 * 
 * Returns the value associated with @key, interpreted
 * as a length. The returned value is converted to @units.
 * 
 * Return value: the length value of @key, converted to @unit
 *
 * Since: 2.10
 */
gdouble
btk_print_settings_get_length (BtkPrintSettings *settings,
			       const gchar      *key,
			       BtkUnit           unit)
{
  gdouble length = btk_print_settings_get_double (settings, key);
  return _btk_print_convert_from_mm (length, unit);
}

/**
 * btk_print_settings_set_length:
 * @settings: a #BtkPrintSettings
 * @key: a key
 * @value: a length
 * @unit: the unit of @length
 * 
 * Associates a length in units of @unit with @key.
 *
 * Since: 2.10
 */
void
btk_print_settings_set_length (BtkPrintSettings *settings,
			       const gchar      *key,
			       gdouble           value, 
			       BtkUnit           unit)
{
  btk_print_settings_set_double (settings, key,
				 _btk_print_convert_to_mm (value, unit));
}

/**
 * btk_print_settings_get_int_with_default:
 * @settings: a #BtkPrintSettings
 * @key: a key
 * @def: the default value
 * 
 * Returns the value of @key, interpreted as
 * an integer, or the default value.
 * 
 * Return value: the integer value of @key
 *
 * Since: 2.10
 */
gint
btk_print_settings_get_int_with_default (BtkPrintSettings *settings,
					 const gchar      *key,
					 gint              def)
{
  const gchar *val;

  val = btk_print_settings_get (settings, key);
  if (val == NULL)
    return def;

  return atoi (val);
}

/**
 * btk_print_settings_get_int:
 * @settings: a #BtkPrintSettings
 * @key: a key
 * 
 * Returns the integer value of @key, or 0.
 * 
 * Return value: the integer value of @key 
 *
 * Since: 2.10
 */
gint
btk_print_settings_get_int (BtkPrintSettings *settings,
			    const gchar      *key)
{
  return btk_print_settings_get_int_with_default (settings, key, 0);
}

/**
 * btk_print_settings_set_int:
 * @settings: a #BtkPrintSettings
 * @key: a key
 * @value: an integer 
 * 
 * Sets @key to an integer value.
 *
 * Since: 2.10 
 */
void
btk_print_settings_set_int (BtkPrintSettings *settings,
			    const gchar      *key,
			    gint              value)
{
  gchar buf[128];
  g_sprintf (buf, "%d", value);
  btk_print_settings_set (settings, key, buf);
}

/**
 * btk_print_settings_foreach:
 * @settings: a #BtkPrintSettings
 * @func: (scope call): the function to call
 * @user_data: user data for @func
 *
 * Calls @func for each key-value pair of @settings.
 *
 * Since: 2.10
 */
void
btk_print_settings_foreach (BtkPrintSettings    *settings,
			    BtkPrintSettingsFunc func,
			    gpointer             user_data)
{
  g_hash_table_foreach (settings->hash, (GHFunc)func, user_data);
}

/**
 * btk_print_settings_get_printer:
 * @settings: a #BtkPrintSettings
 * 
 * Convenience function to obtain the value of 
 * %BTK_PRINT_SETTINGS_PRINTER.
 *
 * Return value: the printer name
 *
 * Since: 2.10
 */
const gchar *
btk_print_settings_get_printer (BtkPrintSettings *settings)
{
  return btk_print_settings_get (settings, BTK_PRINT_SETTINGS_PRINTER);
}


/**
 * btk_print_settings_set_printer:
 * @settings: a #BtkPrintSettings
 * @printer: the printer name
 * 
 * Convenience function to set %BTK_PRINT_SETTINGS_PRINTER
 * to @printer.
 *
 * Since: 2.10
 */
void
btk_print_settings_set_printer (BtkPrintSettings *settings,
				const gchar      *printer)
{
  btk_print_settings_set (settings, BTK_PRINT_SETTINGS_PRINTER, printer);
}

/**
 * btk_print_settings_get_orientation:
 * @settings: a #BtkPrintSettings
 * 
 * Get the value of %BTK_PRINT_SETTINGS_ORIENTATION, 
 * converted to a #BtkPageOrientation.
 * 
 * Return value: the orientation
 *
 * Since: 2.10
 */
BtkPageOrientation
btk_print_settings_get_orientation (BtkPrintSettings *settings)
{
  const gchar *val;

  val = btk_print_settings_get (settings, BTK_PRINT_SETTINGS_ORIENTATION);

  if (val == NULL || strcmp (val, "portrait") == 0)
    return BTK_PAGE_ORIENTATION_PORTRAIT;

  if (strcmp (val, "landscape") == 0)
    return BTK_PAGE_ORIENTATION_LANDSCAPE;
  
  if (strcmp (val, "reverse_portrait") == 0)
    return BTK_PAGE_ORIENTATION_REVERSE_PORTRAIT;
  
  if (strcmp (val, "reverse_landscape") == 0)
    return BTK_PAGE_ORIENTATION_REVERSE_LANDSCAPE;
  
  return BTK_PAGE_ORIENTATION_PORTRAIT;
}

/**
 * btk_print_settings_set_orientation:
 * @settings: a #BtkPrintSettings
 * @orientation: a page orientation
 * 
 * Sets the value of %BTK_PRINT_SETTINGS_ORIENTATION.
 * 
 * Since: 2.10
 */
void
btk_print_settings_set_orientation (BtkPrintSettings   *settings,
				    BtkPageOrientation  orientation)
{
  const gchar *val;

  switch (orientation)
    {
    case BTK_PAGE_ORIENTATION_LANDSCAPE:
      val = "landscape";
      break;
    default:
    case BTK_PAGE_ORIENTATION_PORTRAIT:
      val = "portrait";
      break;
    case BTK_PAGE_ORIENTATION_REVERSE_LANDSCAPE:
      val = "reverse_landscape";
      break;
    case BTK_PAGE_ORIENTATION_REVERSE_PORTRAIT:
      val = "reverse_portrait";
      break;
    }
  btk_print_settings_set (settings, BTK_PRINT_SETTINGS_ORIENTATION, val);
}

/**
 * btk_print_settings_get_paper_size:
 * @settings: a #BtkPrintSettings
 * 
 * Gets the value of %BTK_PRINT_SETTINGS_PAPER_FORMAT, 
 * converted to a #BtkPaperSize.
 * 
 * Return value: the paper size
 *
 * Since: 2.10
 */
BtkPaperSize *     
btk_print_settings_get_paper_size (BtkPrintSettings *settings)
{
  const gchar *val;
  const gchar *name;
  gdouble w, h;

  val = btk_print_settings_get (settings, BTK_PRINT_SETTINGS_PAPER_FORMAT);
  if (val == NULL)
    return NULL;

  if (g_str_has_prefix (val, "custom-")) 
    {
      name = val + strlen ("custom-");
      w = btk_print_settings_get_paper_width (settings, BTK_UNIT_MM);
      h = btk_print_settings_get_paper_height (settings, BTK_UNIT_MM);
      return btk_paper_size_new_custom (name, name, w, h, BTK_UNIT_MM);
    }

  return btk_paper_size_new (val);
}

/**
 * btk_print_settings_set_paper_size:
 * @settings: a #BtkPrintSettings
 * @paper_size: a paper size
 * 
 * Sets the value of %BTK_PRINT_SETTINGS_PAPER_FORMAT,
 * %BTK_PRINT_SETTINGS_PAPER_WIDTH and
 * %BTK_PRINT_SETTINGS_PAPER_HEIGHT.
 *
 * Since: 2.10
 */
void
btk_print_settings_set_paper_size (BtkPrintSettings *settings,
				   BtkPaperSize     *paper_size)
{
  gchar *custom_name;

  if (paper_size == NULL) 
    {
      btk_print_settings_set (settings, BTK_PRINT_SETTINGS_PAPER_FORMAT, NULL);
      btk_print_settings_set (settings, BTK_PRINT_SETTINGS_PAPER_WIDTH, NULL);
      btk_print_settings_set (settings, BTK_PRINT_SETTINGS_PAPER_HEIGHT, NULL);
    }
  else if (btk_paper_size_is_custom (paper_size)) 
    {
      custom_name = g_strdup_printf ("custom-%s", 
				     btk_paper_size_get_name (paper_size));
      btk_print_settings_set (settings, BTK_PRINT_SETTINGS_PAPER_FORMAT, custom_name);
      g_free (custom_name);
      btk_print_settings_set_paper_width (settings, 
					  btk_paper_size_get_width (paper_size, 
								    BTK_UNIT_MM),
					  BTK_UNIT_MM);
      btk_print_settings_set_paper_height (settings, 
					   btk_paper_size_get_height (paper_size, 
								      BTK_UNIT_MM),
					   BTK_UNIT_MM);
    } 
  else
    btk_print_settings_set (settings, BTK_PRINT_SETTINGS_PAPER_FORMAT, 
			    btk_paper_size_get_name (paper_size));
}

/**
 * btk_print_settings_get_paper_width:
 * @settings: a #BtkPrintSettings
 * @unit: the unit for the return value
 * 
 * Gets the value of %BTK_PRINT_SETTINGS_PAPER_WIDTH,
 * converted to @unit. 
 * 
 * Return value: the paper width, in units of @unit
 *
 * Since: 2.10
 */
gdouble
btk_print_settings_get_paper_width (BtkPrintSettings *settings,
				    BtkUnit           unit)
{
  return btk_print_settings_get_length (settings, BTK_PRINT_SETTINGS_PAPER_WIDTH, unit);
}

/**
 * btk_print_settings_set_paper_width:
 * @settings: a #BtkPrintSettings
 * @width: the paper width
 * @unit: the units of @width
 * 
 * Sets the value of %BTK_PRINT_SETTINGS_PAPER_WIDTH.
 *
 * Since: 2.10
 */
void
btk_print_settings_set_paper_width (BtkPrintSettings *settings,
				    gdouble           width, 
				    BtkUnit           unit)
{
  btk_print_settings_set_length (settings, BTK_PRINT_SETTINGS_PAPER_WIDTH, width, unit);
}

/**
 * btk_print_settings_get_paper_height:
 * @settings: a #BtkPrintSettings
 * @unit: the unit for the return value
 * 
 * Gets the value of %BTK_PRINT_SETTINGS_PAPER_HEIGHT,
 * converted to @unit. 
 * 
 * Return value: the paper height, in units of @unit
 *
 * Since: 2.10
 */
gdouble
btk_print_settings_get_paper_height (BtkPrintSettings *settings,
				     BtkUnit           unit)
{
  return btk_print_settings_get_length (settings, 
					BTK_PRINT_SETTINGS_PAPER_HEIGHT,
					unit);
}

/**
 * btk_print_settings_set_paper_height:
 * @settings: a #BtkPrintSettings
 * @height: the paper height
 * @unit: the units of @height
 * 
 * Sets the value of %BTK_PRINT_SETTINGS_PAPER_HEIGHT.
 *
 * Since: 2.10
 */
void
btk_print_settings_set_paper_height (BtkPrintSettings *settings,
				     gdouble           height, 
				     BtkUnit           unit)
{
  btk_print_settings_set_length (settings, 
				 BTK_PRINT_SETTINGS_PAPER_HEIGHT, 
				 height, unit);
}

/**
 * btk_print_settings_get_use_color:
 * @settings: a #BtkPrintSettings
 * 
 * Gets the value of %BTK_PRINT_SETTINGS_USE_COLOR.
 * 
 * Return value: whether to use color
 *
 * Since: 2.10
 */
gboolean
btk_print_settings_get_use_color (BtkPrintSettings *settings)
{
  return btk_print_settings_get_bool_with_default (settings, 
						   BTK_PRINT_SETTINGS_USE_COLOR,
						   TRUE);
}

/**
 * btk_print_settings_set_use_color:
 * @settings: a #BtkPrintSettings
 * @use_color: whether to use color
 * 
 * Sets the value of %BTK_PRINT_SETTINGS_USE_COLOR.
 * 
 * Since: 2.10
 */
void
btk_print_settings_set_use_color (BtkPrintSettings *settings,
				  gboolean          use_color)
{
  btk_print_settings_set_bool (settings,
			       BTK_PRINT_SETTINGS_USE_COLOR, 
			       use_color);
}

/**
 * btk_print_settings_get_collate:
 * @settings: a #BtkPrintSettings
 * 
 * Gets the value of %BTK_PRINT_SETTINGS_COLLATE.
 * 
 * Return value: whether to collate the printed pages
 *
 * Since: 2.10
 */
gboolean
btk_print_settings_get_collate (BtkPrintSettings *settings)
{
  return btk_print_settings_get_bool (settings, 
				      BTK_PRINT_SETTINGS_COLLATE);
}

/**
 * btk_print_settings_set_collate:
 * @settings: a #BtkPrintSettings
 * @collate: whether to collate the output
 * 
 * Sets the value of %BTK_PRINT_SETTINGS_COLLATE.
 * 
 * Since: 2.10
 */
void
btk_print_settings_set_collate (BtkPrintSettings *settings,
				gboolean          collate)
{
  btk_print_settings_set_bool (settings,
			       BTK_PRINT_SETTINGS_COLLATE, 
			       collate);
}

/**
 * btk_print_settings_get_reverse:
 * @settings: a #BtkPrintSettings
 * 
 * Gets the value of %BTK_PRINT_SETTINGS_REVERSE.
 * 
 * Return value: whether to reverse the order of the printed pages
 *
 * Since: 2.10
 */
gboolean
btk_print_settings_get_reverse (BtkPrintSettings *settings)
{
  return btk_print_settings_get_bool (settings, 
				      BTK_PRINT_SETTINGS_REVERSE);
}

/**
 * btk_print_settings_set_reverse:
 * @settings: a #BtkPrintSettings
 * @reverse: whether to reverse the output
 * 
 * Sets the value of %BTK_PRINT_SETTINGS_REVERSE.
 * 
 * Since: 2.10
 */
void
btk_print_settings_set_reverse (BtkPrintSettings *settings,
				  gboolean        reverse)
{
  btk_print_settings_set_bool (settings,
			       BTK_PRINT_SETTINGS_REVERSE, 
			       reverse);
}

/**
 * btk_print_settings_get_duplex:
 * @settings: a #BtkPrintSettings
 * 
 * Gets the value of %BTK_PRINT_SETTINGS_DUPLEX.
 * 
 * Return value: whether to print the output in duplex.
 *
 * Since: 2.10
 */
BtkPrintDuplex
btk_print_settings_get_duplex (BtkPrintSettings *settings)
{
  const gchar *val;

  val = btk_print_settings_get (settings, BTK_PRINT_SETTINGS_DUPLEX);

  if (val == NULL || (strcmp (val, "simplex") == 0))
    return BTK_PRINT_DUPLEX_SIMPLEX;

  if (strcmp (val, "horizontal") == 0)
    return BTK_PRINT_DUPLEX_HORIZONTAL;
  
  if (strcmp (val, "vertical") == 0)
    return BTK_PRINT_DUPLEX_VERTICAL;
  
  return BTK_PRINT_DUPLEX_SIMPLEX;
}

/**
 * btk_print_settings_set_duplex:
 * @settings: a #BtkPrintSettings
 * @duplex: a #BtkPrintDuplex value
 * 
 * Sets the value of %BTK_PRINT_SETTINGS_DUPLEX.
 * 
 * Since: 2.10
 */
void
btk_print_settings_set_duplex (BtkPrintSettings *settings,
			       BtkPrintDuplex    duplex)
{
  const gchar *str;

  switch (duplex)
    {
    default:
    case BTK_PRINT_DUPLEX_SIMPLEX:
      str = "simplex";
      break;
    case BTK_PRINT_DUPLEX_HORIZONTAL:
      str = "horizontal";
      break;
    case BTK_PRINT_DUPLEX_VERTICAL:
      str = "vertical";
      break;
    }
  
  btk_print_settings_set (settings, BTK_PRINT_SETTINGS_DUPLEX, str);
}

/**
 * btk_print_settings_get_quality:
 * @settings: a #BtkPrintSettings
 * 
 * Gets the value of %BTK_PRINT_SETTINGS_QUALITY.
 * 
 * Return value: the print quality
 *
 * Since: 2.10
 */
BtkPrintQuality
btk_print_settings_get_quality (BtkPrintSettings *settings)
{
  const gchar *val;

  val = btk_print_settings_get (settings, BTK_PRINT_SETTINGS_QUALITY);

  if (val == NULL || (strcmp (val, "normal") == 0))
    return BTK_PRINT_QUALITY_NORMAL;

  if (strcmp (val, "high") == 0)
    return BTK_PRINT_QUALITY_HIGH;
  
  if (strcmp (val, "low") == 0)
    return BTK_PRINT_QUALITY_LOW;
  
  if (strcmp (val, "draft") == 0)
    return BTK_PRINT_QUALITY_DRAFT;
  
  return BTK_PRINT_QUALITY_NORMAL;
}

/**
 * btk_print_settings_set_quality:
 * @settings: a #BtkPrintSettings
 * @quality: a #BtkPrintQuality value
 * 
 * Sets the value of %BTK_PRINT_SETTINGS_QUALITY.
 * 
 * Since: 2.10
 */
void
btk_print_settings_set_quality (BtkPrintSettings *settings,
				BtkPrintQuality   quality)
{
  const gchar *str;

  switch (quality)
    {
    default:
    case BTK_PRINT_QUALITY_NORMAL:
      str = "normal";
      break;
    case BTK_PRINT_QUALITY_HIGH:
      str = "high";
      break;
    case BTK_PRINT_QUALITY_LOW:
      str = "low";
      break;
    case BTK_PRINT_QUALITY_DRAFT:
      str = "draft";
      break;
    }
  
  btk_print_settings_set (settings, BTK_PRINT_SETTINGS_QUALITY, str);
}

/**
 * btk_print_settings_get_page_set:
 * @settings: a #BtkPrintSettings
 * 
 * Gets the value of %BTK_PRINT_SETTINGS_PAGE_SET.
 * 
 * Return value: the set of pages to print
 *
 * Since: 2.10
 */
BtkPageSet
btk_print_settings_get_page_set (BtkPrintSettings *settings)
{
  const gchar *val;

  val = btk_print_settings_get (settings, BTK_PRINT_SETTINGS_PAGE_SET);

  if (val == NULL || (strcmp (val, "all") == 0))
    return BTK_PAGE_SET_ALL;

  if (strcmp (val, "even") == 0)
    return BTK_PAGE_SET_EVEN;
  
  if (strcmp (val, "odd") == 0)
    return BTK_PAGE_SET_ODD;
  
  return BTK_PAGE_SET_ALL;
}

/**
 * btk_print_settings_set_page_set:
 * @settings: a #BtkPrintSettings
 * @page_set: a #BtkPageSet value
 * 
 * Sets the value of %BTK_PRINT_SETTINGS_PAGE_SET.
 * 
 * Since: 2.10
 */
void
btk_print_settings_set_page_set (BtkPrintSettings *settings,
				 BtkPageSet        page_set)
{
  const gchar *str;

  switch (page_set)
    {
    default:
    case BTK_PAGE_SET_ALL:
      str = "all";
      break;
    case BTK_PAGE_SET_EVEN:
      str = "even";
      break;
    case BTK_PAGE_SET_ODD:
      str = "odd";
      break;
    }
  
  btk_print_settings_set (settings, BTK_PRINT_SETTINGS_PAGE_SET, str);
}

/**
 * btk_print_settings_get_number_up_layout:
 * @settings: a #BtkPrintSettings
 * 
 * Gets the value of %BTK_PRINT_SETTINGS_NUMBER_UP_LAYOUT.
 * 
 * Return value: layout of page in number-up mode
 *
 * Since: 2.14
 */
BtkNumberUpLayout
btk_print_settings_get_number_up_layout (BtkPrintSettings *settings)
{
  BtkNumberUpLayout layout;
  BtkTextDirection  text_direction;
  GEnumClass       *enum_class;
  GEnumValue       *enum_value;
  const gchar      *val;

  g_return_val_if_fail (BTK_IS_PRINT_SETTINGS (settings), BTK_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_TOP_TO_BOTTOM);

  val = btk_print_settings_get (settings, BTK_PRINT_SETTINGS_NUMBER_UP_LAYOUT);
  text_direction = btk_widget_get_default_direction ();

  if (text_direction == BTK_TEXT_DIR_LTR)
    layout = BTK_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_TOP_TO_BOTTOM;
  else
    layout = BTK_NUMBER_UP_LAYOUT_RIGHT_TO_LEFT_TOP_TO_BOTTOM;

  if (val == NULL)
    return layout;

  enum_class = g_type_class_ref (BTK_TYPE_NUMBER_UP_LAYOUT);
  enum_value = g_enum_get_value_by_nick (enum_class, val);
  if (enum_value)
    layout = enum_value->value;
  g_type_class_unref (enum_class);

  return layout;
}

/**
 * btk_print_settings_set_number_up_layout:
 * @settings: a #BtkPrintSettings
 * @number_up_layout: a #BtkNumberUpLayout value
 * 
 * Sets the value of %BTK_PRINT_SETTINGS_NUMBER_UP_LAYOUT.
 * 
 * Since: 2.14
 */
void
btk_print_settings_set_number_up_layout (BtkPrintSettings  *settings,
					 BtkNumberUpLayout  number_up_layout)
{
  GEnumClass *enum_class;
  GEnumValue *enum_value;

  g_return_if_fail (BTK_IS_PRINT_SETTINGS (settings));

  enum_class = g_type_class_ref (BTK_TYPE_NUMBER_UP_LAYOUT);
  enum_value = g_enum_get_value (enum_class, number_up_layout);
  g_return_if_fail (enum_value != NULL);

  btk_print_settings_set (settings, BTK_PRINT_SETTINGS_NUMBER_UP_LAYOUT, enum_value->value_nick);
  g_type_class_unref (enum_class);
}

/**
 * btk_print_settings_get_n_copies:
 * @settings: a #BtkPrintSettings
 * 
 * Gets the value of %BTK_PRINT_SETTINGS_N_COPIES.
 * 
 * Return value: the number of copies to print
 *
 * Since: 2.10
 */
gint
btk_print_settings_get_n_copies (BtkPrintSettings *settings)
{
  return btk_print_settings_get_int_with_default (settings, BTK_PRINT_SETTINGS_N_COPIES, 1);
}

/**
 * btk_print_settings_set_n_copies:
 * @settings: a #BtkPrintSettings
 * @num_copies: the number of copies 
 * 
 * Sets the value of %BTK_PRINT_SETTINGS_N_COPIES.
 * 
 * Since: 2.10
 */
void
btk_print_settings_set_n_copies (BtkPrintSettings *settings,
				 gint              num_copies)
{
  btk_print_settings_set_int (settings, BTK_PRINT_SETTINGS_N_COPIES,
			      num_copies);
}

/**
 * btk_print_settings_get_number_up:
 * @settings: a #BtkPrintSettings
 * 
 * Gets the value of %BTK_PRINT_SETTINGS_NUMBER_UP.
 * 
 * Return value: the number of pages per sheet
 *
 * Since: 2.10
 */
gint
btk_print_settings_get_number_up (BtkPrintSettings *settings)
{
  return btk_print_settings_get_int_with_default (settings, BTK_PRINT_SETTINGS_NUMBER_UP, 1);
}

/**
 * btk_print_settings_set_number_up:
 * @settings: a #BtkPrintSettings
 * @number_up: the number of pages per sheet 
 * 
 * Sets the value of %BTK_PRINT_SETTINGS_NUMBER_UP.
 * 
 * Since: 2.10
 */
void
btk_print_settings_set_number_up (BtkPrintSettings *settings,
				  gint              number_up)
{
  btk_print_settings_set_int (settings, BTK_PRINT_SETTINGS_NUMBER_UP,
				number_up);
}

/**
 * btk_print_settings_get_resolution:
 * @settings: a #BtkPrintSettings
 * 
 * Gets the value of %BTK_PRINT_SETTINGS_RESOLUTION.
 * 
 * Return value: the resolution in dpi
 *
 * Since: 2.10
 */
gint
btk_print_settings_get_resolution (BtkPrintSettings *settings)
{
  return btk_print_settings_get_int_with_default (settings, BTK_PRINT_SETTINGS_RESOLUTION, 300);
}

/**
 * btk_print_settings_set_resolution:
 * @settings: a #BtkPrintSettings
 * @resolution: the resolution in dpi
 * 
 * Sets the values of %BTK_PRINT_SETTINGS_RESOLUTION,
 * %BTK_PRINT_SETTINGS_RESOLUTION_X and 
 * %BTK_PRINT_SETTINGS_RESOLUTION_Y.
 * 
 * Since: 2.10
 */
void
btk_print_settings_set_resolution (BtkPrintSettings *settings,
				   gint              resolution)
{
  btk_print_settings_set_int (settings, BTK_PRINT_SETTINGS_RESOLUTION,
			      resolution);
  btk_print_settings_set_int (settings, BTK_PRINT_SETTINGS_RESOLUTION_X,
			      resolution);
  btk_print_settings_set_int (settings, BTK_PRINT_SETTINGS_RESOLUTION_Y,
			      resolution);
}

/**
 * btk_print_settings_get_resolution_x:
 * @settings: a #BtkPrintSettings
 * 
 * Gets the value of %BTK_PRINT_SETTINGS_RESOLUTION_X.
 * 
 * Return value: the horizontal resolution in dpi
 *
 * Since: 2.16
 */
gint
btk_print_settings_get_resolution_x (BtkPrintSettings *settings)
{
  return btk_print_settings_get_int_with_default (settings, BTK_PRINT_SETTINGS_RESOLUTION_X, 300);
}

/**
 * btk_print_settings_get_resolution_y:
 * @settings: a #BtkPrintSettings
 * 
 * Gets the value of %BTK_PRINT_SETTINGS_RESOLUTION_Y.
 * 
 * Return value: the vertical resolution in dpi
 *
 * Since: 2.16
 */
gint
btk_print_settings_get_resolution_y (BtkPrintSettings *settings)
{
  return btk_print_settings_get_int_with_default (settings, BTK_PRINT_SETTINGS_RESOLUTION_Y, 300);
}

/**
 * btk_print_settings_set_resolution_xy:
 * @settings: a #BtkPrintSettings
 * @resolution_x: the horizontal resolution in dpi
 * @resolution_y: the vertical resolution in dpi
 * 
 * Sets the values of %BTK_PRINT_SETTINGS_RESOLUTION,
 * %BTK_PRINT_SETTINGS_RESOLUTION_X and
 * %BTK_PRINT_SETTINGS_RESOLUTION_Y.
 * 
 * Since: 2.16
 */
void
btk_print_settings_set_resolution_xy (BtkPrintSettings *settings,
				      gint              resolution_x,
				      gint              resolution_y)
{
  btk_print_settings_set_int (settings, BTK_PRINT_SETTINGS_RESOLUTION_X,
			      resolution_x);
  btk_print_settings_set_int (settings, BTK_PRINT_SETTINGS_RESOLUTION_Y,
			      resolution_y);
  btk_print_settings_set_int (settings, BTK_PRINT_SETTINGS_RESOLUTION,
			      resolution_x);
}

/**
 * btk_print_settings_get_printer_lpi:
 * @settings: a #BtkPrintSettings
 * 
 * Gets the value of %BTK_PRINT_SETTINGS_PRINTER_LPI.
 * 
 * Return value: the resolution in lpi (lines per inch)
 *
 * Since: 2.16
 */
gdouble
btk_print_settings_get_printer_lpi (BtkPrintSettings *settings)
{
  return btk_print_settings_get_double_with_default (settings, BTK_PRINT_SETTINGS_PRINTER_LPI, 150.0);
}

/**
 * btk_print_settings_set_printer_lpi:
 * @settings: a #BtkPrintSettings
 * @lpi: the resolution in lpi (lines per inch)
 * 
 * Sets the value of %BTK_PRINT_SETTINGS_PRINTER_LPI.
 * 
 * Since: 2.16
 */
void
btk_print_settings_set_printer_lpi (BtkPrintSettings *settings,
				    gdouble           lpi)
{
  btk_print_settings_set_double (settings, BTK_PRINT_SETTINGS_PRINTER_LPI,
			         lpi);
}

/**
 * btk_print_settings_get_scale:
 * @settings: a #BtkPrintSettings
 * 
 * Gets the value of %BTK_PRINT_SETTINGS_SCALE.
 * 
 * Return value: the scale in percent
 *
 * Since: 2.10
 */
gdouble
btk_print_settings_get_scale (BtkPrintSettings *settings)
{
  return btk_print_settings_get_double_with_default (settings,
						     BTK_PRINT_SETTINGS_SCALE,
						     100.0);
}

/**
 * btk_print_settings_set_scale:
 * @settings: a #BtkPrintSettings
 * @scale: the scale in percent
 * 
 * Sets the value of %BTK_PRINT_SETTINGS_SCALE.
 * 
 * Since: 2.10
 */
void
btk_print_settings_set_scale (BtkPrintSettings *settings,
			      gdouble           scale)
{
  btk_print_settings_set_double (settings, BTK_PRINT_SETTINGS_SCALE,
				 scale);
}

/**
 * btk_print_settings_get_print_pages:
 * @settings: a #BtkPrintSettings
 * 
 * Gets the value of %BTK_PRINT_SETTINGS_PRINT_PAGES.
 * 
 * Return value: which pages to print
 *
 * Since: 2.10
 */
BtkPrintPages
btk_print_settings_get_print_pages (BtkPrintSettings *settings)
{
  const gchar *val;

  val = btk_print_settings_get (settings, BTK_PRINT_SETTINGS_PRINT_PAGES);

  if (val == NULL || (strcmp (val, "all") == 0))
    return BTK_PRINT_PAGES_ALL;

  if (strcmp (val, "selection") == 0)
    return BTK_PRINT_PAGES_SELECTION;

  if (strcmp (val, "current") == 0)
    return BTK_PRINT_PAGES_CURRENT;
  
  if (strcmp (val, "ranges") == 0)
    return BTK_PRINT_PAGES_RANGES;
  
  return BTK_PRINT_PAGES_ALL;
}

/**
 * btk_print_settings_set_print_pages:
 * @settings: a #BtkPrintSettings
 * @pages: a #BtkPrintPages value
 * 
 * Sets the value of %BTK_PRINT_SETTINGS_PRINT_PAGES.
 * 
 * Since: 2.10
 */
void
btk_print_settings_set_print_pages (BtkPrintSettings *settings,
				    BtkPrintPages     pages)
{
  const gchar *str;

  switch (pages)
    {
    default:
    case BTK_PRINT_PAGES_ALL:
      str = "all";
      break;
    case BTK_PRINT_PAGES_CURRENT:
      str = "current";
      break;
    case BTK_PRINT_PAGES_SELECTION:
      str = "selection";
      break;
    case BTK_PRINT_PAGES_RANGES:
      str = "ranges";
      break;
    }
  
  btk_print_settings_set (settings, BTK_PRINT_SETTINGS_PRINT_PAGES, str);
}

/**
 * btk_print_settings_get_page_ranges:
 * @settings: a #BtkPrintSettings
 * @num_ranges: (out): return location for the length of the returned array
 *
 * Gets the value of %BTK_PRINT_SETTINGS_PAGE_RANGES.
 *
 * Return value: (array length=num_ranges) (transfer full): an array
 *     of #BtkPageRange<!-- -->s.  Use g_free() to free the array when
 *     it is no longer needed.
 *
 * Since: 2.10
 */
BtkPageRange *
btk_print_settings_get_page_ranges (BtkPrintSettings *settings,
				    gint             *num_ranges)
{
  const gchar *val;
  gchar **range_strs;
  BtkPageRange *ranges;
  gint i, n;
  
  val = btk_print_settings_get (settings, BTK_PRINT_SETTINGS_PAGE_RANGES);

  if (val == NULL)
    {
      *num_ranges = 0;
      return NULL;
    }
  
  range_strs = g_strsplit (val, ",", 0);

  for (i = 0; range_strs[i] != NULL; i++)
    ;

  n = i;

  ranges = g_new0 (BtkPageRange, n);

  for (i = 0; i < n; i++)
    {
      gint start, end;
      gchar *str;

      start = (gint)strtol (range_strs[i], &str, 10);
      end = start;

      if (*str == '-')
	{
	  str++;
	  end = (gint)strtol (str, NULL, 10);
	}

      ranges[i].start = start;
      ranges[i].end = end;
    }

  g_strfreev (range_strs);

  *num_ranges = n;
  return ranges;
}

/**
 * btk_print_settings_set_page_ranges:
 * @settings: a #BtkPrintSettings
 * @page_ranges: (array length=num_ranges): an array of #BtkPageRange<!-- -->s
 * @num_ranges: the length of @page_ranges
 * 
 * Sets the value of %BTK_PRINT_SETTINGS_PAGE_RANGES.
 * 
 * Since: 2.10
 */
void
btk_print_settings_set_page_ranges  (BtkPrintSettings *settings,
				     BtkPageRange     *page_ranges,
				     gint              num_ranges)
{
  GString *s;
  gint i;
  
  s = g_string_new ("");

  for (i = 0; i < num_ranges; i++)
    {
      if (page_ranges[i].start == page_ranges[i].end)
	g_string_append_printf (s, "%d", page_ranges[i].start);
      else
	g_string_append_printf (s, "%d-%d",
				page_ranges[i].start,
				page_ranges[i].end);
      if (i < num_ranges - 1)
	g_string_append (s, ",");
    }

  
  btk_print_settings_set (settings, BTK_PRINT_SETTINGS_PAGE_RANGES, 
			  s->str);

  g_string_free (s, TRUE);
}

/**
 * btk_print_settings_get_default_source:
 * @settings: a #BtkPrintSettings
 * 
 * Gets the value of %BTK_PRINT_SETTINGS_DEFAULT_SOURCE.
 * 
 * Return value: the default source
 *
 * Since: 2.10
 */
const gchar *
btk_print_settings_get_default_source (BtkPrintSettings *settings)
{
  return btk_print_settings_get (settings, BTK_PRINT_SETTINGS_DEFAULT_SOURCE);
}

/**
 * btk_print_settings_set_default_source:
 * @settings: a #BtkPrintSettings
 * @default_source: the default source
 * 
 * Sets the value of %BTK_PRINT_SETTINGS_DEFAULT_SOURCE.
 * 
 * Since: 2.10
 */
void
btk_print_settings_set_default_source (BtkPrintSettings *settings,
				       const gchar      *default_source)
{
  btk_print_settings_set (settings, BTK_PRINT_SETTINGS_DEFAULT_SOURCE, default_source);
}
     
/**
 * btk_print_settings_get_media_type:
 * @settings: a #BtkPrintSettings
 * 
 * Gets the value of %BTK_PRINT_SETTINGS_MEDIA_TYPE.
 *
 * The set of media types is defined in PWG 5101.1-2002 PWG.
 * <!-- FIXME link here -->
 * 
 * Return value: the media type
 *
 * Since: 2.10
 */
const gchar *
btk_print_settings_get_media_type (BtkPrintSettings *settings)
{
  return btk_print_settings_get (settings, BTK_PRINT_SETTINGS_MEDIA_TYPE);
}

/**
 * btk_print_settings_set_media_type:
 * @settings: a #BtkPrintSettings
 * @media_type: the media type
 * 
 * Sets the value of %BTK_PRINT_SETTINGS_MEDIA_TYPE.
 * 
 * The set of media types is defined in PWG 5101.1-2002 PWG.
 * <!-- FIXME link here -->
 *
 * Since: 2.10
 */
void
btk_print_settings_set_media_type (BtkPrintSettings *settings,
				   const gchar      *media_type)
{
  btk_print_settings_set (settings, BTK_PRINT_SETTINGS_MEDIA_TYPE, media_type);
}

/**
 * btk_print_settings_get_dither:
 * @settings: a #BtkPrintSettings
 * 
 * Gets the value of %BTK_PRINT_SETTINGS_DITHER.
 * 
 * Return value: the dithering that is used
 *
 * Since: 2.10
 */
const gchar *
btk_print_settings_get_dither (BtkPrintSettings *settings)
{
  return btk_print_settings_get (settings, BTK_PRINT_SETTINGS_DITHER);
}

/**
 * btk_print_settings_set_dither:
 * @settings: a #BtkPrintSettings
 * @dither: the dithering that is used
 * 
 * Sets the value of %BTK_PRINT_SETTINGS_DITHER.
 *
 * Since: 2.10
 */
void
btk_print_settings_set_dither (BtkPrintSettings *settings,
			       const gchar      *dither)
{
  btk_print_settings_set (settings, BTK_PRINT_SETTINGS_DITHER, dither);
}
     
/**
 * btk_print_settings_get_finishings:
 * @settings: a #BtkPrintSettings
 * 
 * Gets the value of %BTK_PRINT_SETTINGS_FINISHINGS.
 * 
 * Return value: the finishings
 *
 * Since: 2.10
 */
const gchar *
btk_print_settings_get_finishings (BtkPrintSettings *settings)
{
  return btk_print_settings_get (settings, BTK_PRINT_SETTINGS_FINISHINGS);
}

/**
 * btk_print_settings_set_finishings:
 * @settings: a #BtkPrintSettings
 * @finishings: the finishings
 * 
 * Sets the value of %BTK_PRINT_SETTINGS_FINISHINGS.
 *
 * Since: 2.10
 */
void
btk_print_settings_set_finishings (BtkPrintSettings *settings,
				   const gchar      *finishings)
{
  btk_print_settings_set (settings, BTK_PRINT_SETTINGS_FINISHINGS, finishings);
}
     
/**
 * btk_print_settings_get_output_bin:
 * @settings: a #BtkPrintSettings
 * 
 * Gets the value of %BTK_PRINT_SETTINGS_OUTPUT_BIN.
 * 
 * Return value: the output bin
 *
 * Since: 2.10
 */
const gchar *
btk_print_settings_get_output_bin (BtkPrintSettings *settings)
{
  return btk_print_settings_get (settings, BTK_PRINT_SETTINGS_OUTPUT_BIN);
}

/**
 * btk_print_settings_set_output_bin:
 * @settings: a #BtkPrintSettings
 * @output_bin: the output bin
 * 
 * Sets the value of %BTK_PRINT_SETTINGS_OUTPUT_BIN.
 *
 * Since: 2.10
 */
void
btk_print_settings_set_output_bin (BtkPrintSettings *settings,
				   const gchar      *output_bin)
{
  btk_print_settings_set (settings, BTK_PRINT_SETTINGS_OUTPUT_BIN, output_bin);
}

/**
 * btk_print_settings_load_file:
 * @settings: a #BtkPrintSettings
 * @file_name: the filename to read the settings from
 * @error: (allow-none): return location for errors, or %NULL
 *
 * Reads the print settings from @file_name. If the file could not be loaded
 * then error is set to either a #GFileError or #GKeyFileError.
 * See btk_print_settings_to_file().
 *
 * Return value: %TRUE on success
 *
 * Since: 2.14
 */
gboolean
btk_print_settings_load_file (BtkPrintSettings *settings,
                              const gchar      *file_name,
                              GError          **error)
{
  gboolean retval = FALSE;
  GKeyFile *key_file;

  g_return_val_if_fail (BTK_IS_PRINT_SETTINGS (settings), FALSE);
  g_return_val_if_fail (file_name != NULL, FALSE);

  key_file = g_key_file_new ();

  if (g_key_file_load_from_file (key_file, file_name, 0, error) &&
      btk_print_settings_load_key_file (settings, key_file, NULL, error))
    retval = TRUE;

  g_key_file_free (key_file);

  return retval;
}

/**
 * btk_print_settings_new_from_file:
 * @file_name: the filename to read the settings from
 * @error: (allow-none): return location for errors, or %NULL
 * 
 * Reads the print settings from @file_name. Returns a new #BtkPrintSettings
 * object with the restored settings, or %NULL if an error occurred. If the
 * file could not be loaded then error is set to either a #GFileError or
 * #GKeyFileError.  See btk_print_settings_to_file().
 *
 * Return value: the restored #BtkPrintSettings
 * 
 * Since: 2.12
 */
BtkPrintSettings *
btk_print_settings_new_from_file (const gchar  *file_name,
			          GError      **error)
{
  BtkPrintSettings *settings = btk_print_settings_new ();

  if (!btk_print_settings_load_file (settings, file_name, error))
    {
      g_object_unref (settings);
      settings = NULL;
    }

  return settings;
}

/**
 * btk_print_settings_load_key_file:
 * @settings: a #BtkPrintSettings
 * @key_file: the #GKeyFile to retrieve the settings from
 * @group_name: (allow-none): the name of the group to use, or %NULL to use the default
 *     "Print Settings"
 * @error: (allow-none): return location for errors, or %NULL
 * 
 * Reads the print settings from the group @group_name in @key_file. If the
 * file could not be loaded then error is set to either a #GFileError or
 * #GKeyFileError.
 *
 * Return value: %TRUE on success
 * 
 * Since: 2.14
 */
gboolean
btk_print_settings_load_key_file (BtkPrintSettings *settings,
				  GKeyFile         *key_file,
				  const gchar      *group_name,
				  GError          **error)
{
  gchar **keys;
  gsize n_keys, i;
  GError *err = NULL;

  g_return_val_if_fail (BTK_IS_PRINT_SETTINGS (settings), FALSE);
  g_return_val_if_fail (key_file != NULL, FALSE);

  if (!group_name)
    group_name = KEYFILE_GROUP_NAME;

  keys = g_key_file_get_keys (key_file,
			      group_name,
			      &n_keys,
			      &err);
  if (err != NULL)
    {
      g_propagate_error (error, err);
      return FALSE;
    }
   
  for (i = 0 ; i < n_keys; ++i)
    {
      gchar *value;

      value = g_key_file_get_string (key_file,
				     group_name,
				     keys[i],
				     NULL);
      if (!value)
        continue;

      btk_print_settings_set (settings, keys[i], value);
      g_free (value);
    }

  g_strfreev (keys);

  return TRUE;
}

/**
 * btk_print_settings_new_from_key_file:
 * @key_file: the #GKeyFile to retrieve the settings from
 * @group_name: (allow-none): the name of the group to use, or %NULL to use
 *     the default "Print Settings"
 * @error: (allow-none): return location for errors, or %NULL
 *
 * Reads the print settings from the group @group_name in @key_file.  Returns a
 * new #BtkPrintSettings object with the restored settings, or %NULL if an
 * error occurred. If the file could not be loaded then error is set to either
 * a #GFileError or #GKeyFileError.
 *
 * Return value: the restored #BtkPrintSettings
 *
 * Since: 2.12
 */
BtkPrintSettings *
btk_print_settings_new_from_key_file (GKeyFile     *key_file,
				      const gchar  *group_name,
				      GError      **error)
{
  BtkPrintSettings *settings = btk_print_settings_new ();

  if (!btk_print_settings_load_key_file (settings, key_file,
                                         group_name, error))
    {
      g_object_unref (settings);
      settings = NULL;
    }

  return settings;
}

/**
 * btk_print_settings_to_file:
 * @settings: a #BtkPrintSettings
 * @file_name: the file to save to
 * @error: (allow-none): return location for errors, or %NULL
 * 
 * This function saves the print settings from @settings to @file_name. If the
 * file could not be loaded then error is set to either a #GFileError or
 * #GKeyFileError.
 * 
 * Return value: %TRUE on success
 *
 * Since: 2.12
 */
gboolean
btk_print_settings_to_file (BtkPrintSettings  *settings,
			    const gchar       *file_name,
			    GError           **error)
{
  GKeyFile *key_file;
  gboolean retval = FALSE;
  char *data = NULL;
  gsize len;
  GError *err = NULL;

  g_return_val_if_fail (BTK_IS_PRINT_SETTINGS (settings), FALSE);
  g_return_val_if_fail (file_name != NULL, FALSE);

  key_file = g_key_file_new ();
  btk_print_settings_to_key_file (settings, key_file, NULL);

  data = g_key_file_to_data (key_file, &len, &err);
  if (!data)
    goto out;

  retval = g_file_set_contents (file_name, data, len, &err);

out:
  if (err != NULL)
    g_propagate_error (error, err);

  g_key_file_free (key_file);
  g_free (data);

  return retval;
}

typedef struct {
  GKeyFile *key_file;
  const gchar *group_name;
} SettingsData;

static void
add_value_to_key_file (const gchar  *key,
		       const gchar  *value,
		       SettingsData *data)
{
  g_key_file_set_string (data->key_file, data->group_name, key, value);
}

/**
 * btk_print_settings_to_key_file:
 * @settings: a #BtkPrintSettings
 * @key_file: the #GKeyFile to save the print settings to
 * @group_name: the group to add the settings to in @key_file, or 
 *     %NULL to use the default "Print Settings"
 *
 * This function adds the print settings from @settings to @key_file.
 * 
 * Since: 2.12
 */
void
btk_print_settings_to_key_file (BtkPrintSettings  *settings,
			        GKeyFile          *key_file,
				const gchar       *group_name)
{
  SettingsData data;

  g_return_if_fail (BTK_IS_PRINT_SETTINGS (settings));
  g_return_if_fail (key_file != NULL);

  if (!group_name)
    group_name = KEYFILE_GROUP_NAME;

  data.key_file = key_file;
  data.group_name = group_name;

  btk_print_settings_foreach (settings,
			      (BtkPrintSettingsFunc) add_value_to_key_file,
			      &data);
}


#define __BTK_PRINT_SETTINGS_C__
#include "btkaliasdef.c"
