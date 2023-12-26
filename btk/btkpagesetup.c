/* BTK - The GIMP Toolkit
 * btkpagesetup.c: Page Setup
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

#include "btkpagesetup.h"
#include "btkprintutils.h"
#include "btkprintoperation.h" /* for BtkPrintError */
#include "btkintl.h"
#include "btktypebuiltins.h"
#include "btkalias.h"

#define KEYFILE_GROUP_NAME "Page Setup"

typedef struct _BtkPageSetupClass BtkPageSetupClass;

#define BTK_IS_PAGE_SETUP_CLASS(klass)  (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_PAGE_SETUP))
#define BTK_PAGE_SETUP_CLASS(klass)     (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_PAGE_SETUP, BtkPageSetupClass))
#define BTK_PAGE_SETUP_GET_CLASS(obj)   (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_PAGE_SETUP, BtkPageSetupClass))

struct _BtkPageSetup
{
  BObject parent_instance;

  BtkPageOrientation orientation;
  BtkPaperSize *paper_size;
  /* These are stored in mm */
  double top_margin, bottom_margin, left_margin, right_margin;
};

struct _BtkPageSetupClass
{
  BObjectClass parent_class;
};

G_DEFINE_TYPE (BtkPageSetup, btk_page_setup, B_TYPE_OBJECT)

static void
btk_page_setup_finalize (BObject *object)
{
  BtkPageSetup *setup = BTK_PAGE_SETUP (object);
  
  btk_paper_size_free (setup->paper_size);
  
  B_OBJECT_CLASS (btk_page_setup_parent_class)->finalize (object);
}

static void
btk_page_setup_init (BtkPageSetup *setup)
{
  setup->paper_size = btk_paper_size_new (NULL);
  setup->orientation = BTK_PAGE_ORIENTATION_PORTRAIT;
  setup->top_margin = btk_paper_size_get_default_top_margin (setup->paper_size, BTK_UNIT_MM);
  setup->bottom_margin = btk_paper_size_get_default_bottom_margin (setup->paper_size, BTK_UNIT_MM);
  setup->left_margin = btk_paper_size_get_default_left_margin (setup->paper_size, BTK_UNIT_MM);
  setup->right_margin = btk_paper_size_get_default_right_margin (setup->paper_size, BTK_UNIT_MM);
}

static void
btk_page_setup_class_init (BtkPageSetupClass *class)
{
  BObjectClass *bobject_class = (BObjectClass *)class;

  bobject_class->finalize = btk_page_setup_finalize;
}

/**
 * btk_page_setup_new:
 *
 * Creates a new #BtkPageSetup. 
 * 
 * Return value: a new #BtkPageSetup.
 *
 * Since: 2.10
 */
BtkPageSetup *
btk_page_setup_new (void)
{
  return g_object_new (BTK_TYPE_PAGE_SETUP, NULL);
}

/**
 * btk_page_setup_copy:
 * @other: the #BtkPageSetup to copy
 *
 * Copies a #BtkPageSetup.
 *
 * Return value: (transfer full): a copy of @other
 *
 * Since: 2.10
 */
BtkPageSetup *
btk_page_setup_copy (BtkPageSetup *other)
{
  BtkPageSetup *copy;

  copy = btk_page_setup_new ();
  copy->orientation = other->orientation;
  btk_paper_size_free (copy->paper_size);
  copy->paper_size = btk_paper_size_copy (other->paper_size);
  copy->top_margin = other->top_margin;
  copy->bottom_margin = other->bottom_margin;
  copy->left_margin = other->left_margin;
  copy->right_margin = other->right_margin;

  return copy;
}

/**
 * btk_page_setup_get_orientation:
 * @setup: a #BtkPageSetup
 * 
 * Gets the page orientation of the #BtkPageSetup.
 * 
 * Return value: the page orientation
 *
 * Since: 2.10
 */
BtkPageOrientation
btk_page_setup_get_orientation (BtkPageSetup *setup)
{
  return setup->orientation;
}

/**
 * btk_page_setup_set_orientation:
 * @setup: a #BtkPageSetup
 * @orientation: a #BtkPageOrientation value
 * 
 * Sets the page orientation of the #BtkPageSetup.
 *
 * Since: 2.10
 */
void
btk_page_setup_set_orientation (BtkPageSetup       *setup,
				BtkPageOrientation  orientation)
{
  setup->orientation = orientation;
}

/**
 * btk_page_setup_get_paper_size:
 * @setup: a #BtkPageSetup
 * 
 * Gets the paper size of the #BtkPageSetup.
 * 
 * Return value: the paper size
 *
 * Since: 2.10
 */
BtkPaperSize *
btk_page_setup_get_paper_size (BtkPageSetup *setup)
{
  g_return_val_if_fail (BTK_IS_PAGE_SETUP (setup), NULL);

  return setup->paper_size;
}

/**
 * btk_page_setup_set_paper_size:
 * @setup: a #BtkPageSetup
 * @size: a #BtkPaperSize 
 * 
 * Sets the paper size of the #BtkPageSetup without
 * changing the margins. See 
 * btk_page_setup_set_paper_size_and_default_margins().
 *
 * Since: 2.10
 */
void
btk_page_setup_set_paper_size (BtkPageSetup *setup,
			       BtkPaperSize *size)
{
  BtkPaperSize *old_size;

  g_return_if_fail (BTK_IS_PAGE_SETUP (setup));
  g_return_if_fail (size != NULL);

  old_size = setup->paper_size;

  setup->paper_size = btk_paper_size_copy (size);

  if (old_size)
    btk_paper_size_free (old_size);
}

/**
 * btk_page_setup_set_paper_size_and_default_margins:
 * @setup: a #BtkPageSetup
 * @size: a #BtkPaperSize 
 * 
 * Sets the paper size of the #BtkPageSetup and modifies
 * the margins according to the new paper size.
 *
 * Since: 2.10
 */
void
btk_page_setup_set_paper_size_and_default_margins (BtkPageSetup *setup,
						   BtkPaperSize *size)
{
  btk_page_setup_set_paper_size (setup, size);
  setup->top_margin = btk_paper_size_get_default_top_margin (setup->paper_size, BTK_UNIT_MM);
  setup->bottom_margin = btk_paper_size_get_default_bottom_margin (setup->paper_size, BTK_UNIT_MM);
  setup->left_margin = btk_paper_size_get_default_left_margin (setup->paper_size, BTK_UNIT_MM);
  setup->right_margin = btk_paper_size_get_default_right_margin (setup->paper_size, BTK_UNIT_MM);
}

/**
 * btk_page_setup_get_top_margin:
 * @setup: a #BtkPageSetup
 * @unit: the unit for the return value
 * 
 * Gets the top margin in units of @unit.
 * 
 * Return value: the top margin
 *
 * Since: 2.10
 */
gdouble
btk_page_setup_get_top_margin (BtkPageSetup *setup,
			       BtkUnit       unit)
{
  return _btk_print_convert_from_mm (setup->top_margin, unit);
}

/**
 * btk_page_setup_set_top_margin:
 * @setup: a #BtkPageSetup
 * @margin: the new top margin in units of @unit
 * @unit: the units for @margin
 * 
 * Sets the top margin of the #BtkPageSetup.
 *
 * Since: 2.10
 */
void
btk_page_setup_set_top_margin (BtkPageSetup *setup,
			       gdouble       margin,
			       BtkUnit       unit)
{
  setup->top_margin = _btk_print_convert_to_mm (margin, unit);
}

/**
 * btk_page_setup_get_bottom_margin:
 * @setup: a #BtkPageSetup
 * @unit: the unit for the return value
 * 
 * Gets the bottom margin in units of @unit.
 * 
 * Return value: the bottom margin
 *
 * Since: 2.10
 */
gdouble
btk_page_setup_get_bottom_margin (BtkPageSetup *setup,
				  BtkUnit       unit)
{
  return _btk_print_convert_from_mm (setup->bottom_margin, unit);
}

/**
 * btk_page_setup_set_bottom_margin:
 * @setup: a #BtkPageSetup
 * @margin: the new bottom margin in units of @unit
 * @unit: the units for @margin
 * 
 * Sets the bottom margin of the #BtkPageSetup.
 *
 * Since: 2.10
 */
void
btk_page_setup_set_bottom_margin (BtkPageSetup *setup,
				  gdouble       margin,
				  BtkUnit       unit)
{
  setup->bottom_margin = _btk_print_convert_to_mm (margin, unit);
}

/**
 * btk_page_setup_get_left_margin:
 * @setup: a #BtkPageSetup
 * @unit: the unit for the return value
 * 
 * Gets the left margin in units of @unit.
 * 
 * Return value: the left margin
 *
 * Since: 2.10
 */
gdouble
btk_page_setup_get_left_margin (BtkPageSetup *setup,
				BtkUnit       unit)
{
  return _btk_print_convert_from_mm (setup->left_margin, unit);
}

/**
 * btk_page_setup_set_left_margin:
 * @setup: a #BtkPageSetup
 * @margin: the new left margin in units of @unit
 * @unit: the units for @margin
 * 
 * Sets the left margin of the #BtkPageSetup.
 *
 * Since: 2.10
 */
void
btk_page_setup_set_left_margin (BtkPageSetup *setup,
				gdouble       margin,
				BtkUnit       unit)
{
  setup->left_margin = _btk_print_convert_to_mm (margin, unit);
}

/**
 * btk_page_setup_get_right_margin:
 * @setup: a #BtkPageSetup
 * @unit: the unit for the return value
 * 
 * Gets the right margin in units of @unit.
 * 
 * Return value: the right margin
 *
 * Since: 2.10
 */
gdouble
btk_page_setup_get_right_margin (BtkPageSetup *setup,
				 BtkUnit       unit)
{
  return _btk_print_convert_from_mm (setup->right_margin, unit);
}

/**
 * btk_page_setup_set_right_margin:
 * @setup: a #BtkPageSetup
 * @margin: the new right margin in units of @unit
 * @unit: the units for @margin
 * 
 * Sets the right margin of the #BtkPageSetup.
 *
 * Since: 2.10
 */
void
btk_page_setup_set_right_margin (BtkPageSetup *setup,
				 gdouble       margin,
				 BtkUnit       unit)
{
  setup->right_margin = _btk_print_convert_to_mm (margin, unit);
}

/**
 * btk_page_setup_get_paper_width:
 * @setup: a #BtkPageSetup
 * @unit: the unit for the return value
 * 
 * Returns the paper width in units of @unit.
 * 
 * Note that this function takes orientation, but 
 * not margins into consideration. 
 * See btk_page_setup_get_page_width().
 *
 * Return value: the paper width.
 *
 * Since: 2.10
 */
gdouble
btk_page_setup_get_paper_width (BtkPageSetup *setup,
				BtkUnit       unit)
{
  if (setup->orientation == BTK_PAGE_ORIENTATION_PORTRAIT ||
      setup->orientation == BTK_PAGE_ORIENTATION_REVERSE_PORTRAIT)
    return btk_paper_size_get_width (setup->paper_size, unit);
  else
    return btk_paper_size_get_height (setup->paper_size, unit);
}

/**
 * btk_page_setup_get_paper_height:
 * @setup: a #BtkPageSetup
 * @unit: the unit for the return value
 * 
 * Returns the paper height in units of @unit.
 * 
 * Note that this function takes orientation, but 
 * not margins into consideration.
 * See btk_page_setup_get_page_height().
 *
 * Return value: the paper height.
 *
 * Since: 2.10
 */
gdouble
btk_page_setup_get_paper_height (BtkPageSetup *setup,
				 BtkUnit       unit)
{
  if (setup->orientation == BTK_PAGE_ORIENTATION_PORTRAIT ||
      setup->orientation == BTK_PAGE_ORIENTATION_REVERSE_PORTRAIT)
    return btk_paper_size_get_height (setup->paper_size, unit);
  else
    return btk_paper_size_get_width (setup->paper_size, unit);
}

/**
 * btk_page_setup_get_page_width:
 * @setup: a #BtkPageSetup
 * @unit: the unit for the return value
 * 
 * Returns the page width in units of @unit.
 * 
 * Note that this function takes orientation and
 * margins into consideration. 
 * See btk_page_setup_get_paper_width().
 *
 * Return value: the page width.
 *
 * Since: 2.10
 */
gdouble
btk_page_setup_get_page_width (BtkPageSetup *setup,
			       BtkUnit       unit)
{
  gdouble width;
  
  width = btk_page_setup_get_paper_width (setup, BTK_UNIT_MM);
  width -= setup->left_margin + setup->right_margin;
  
  return _btk_print_convert_from_mm (width, unit);
}

/**
 * btk_page_setup_get_page_height:
 * @setup: a #BtkPageSetup
 * @unit: the unit for the return value
 * 
 * Returns the page height in units of @unit.
 * 
 * Note that this function takes orientation and
 * margins into consideration. 
 * See btk_page_setup_get_paper_height().
 *
 * Return value: the page height.
 *
 * Since: 2.10
 */
gdouble
btk_page_setup_get_page_height (BtkPageSetup *setup,
				BtkUnit       unit)
{
  gdouble height;
  
  height = btk_page_setup_get_paper_height (setup, BTK_UNIT_MM);
  height -= setup->top_margin + setup->bottom_margin;
  
  return _btk_print_convert_from_mm (height, unit);
}

/**
 * btk_page_setup_load_file:
 * @setup: a #BtkPageSetup
 * @file_name: the filename to read the page setup from
 * @error: (allow-none): return location for an error, or %NULL
 *
 * Reads the page setup from the file @file_name.
 * See btk_page_setup_to_file().
 *
 * Return value: %TRUE on success
 *
 * Since: 2.14
 */
gboolean
btk_page_setup_load_file (BtkPageSetup *setup,
                          const gchar  *file_name,
			  GError      **error)
{
  gboolean retval = FALSE;
  GKeyFile *key_file;

  g_return_val_if_fail (BTK_IS_PAGE_SETUP (setup), FALSE);
  g_return_val_if_fail (file_name != NULL, FALSE);

  key_file = g_key_file_new ();

  if (g_key_file_load_from_file (key_file, file_name, 0, error) &&
      btk_page_setup_load_key_file (setup, key_file, NULL, error))
    retval = TRUE;

  g_key_file_free (key_file);

  return retval;
}

/**
 * btk_page_setup_new_from_file:
 * @file_name: the filename to read the page setup from
 * @error: (allow-none): return location for an error, or %NULL
 * 
 * Reads the page setup from the file @file_name. Returns a 
 * new #BtkPageSetup object with the restored page setup, 
 * or %NULL if an error occurred. See btk_page_setup_to_file().
 *
 * Return value: the restored #BtkPageSetup
 * 
 * Since: 2.12
 */
BtkPageSetup *
btk_page_setup_new_from_file (const gchar  *file_name,
			      GError      **error)
{
  BtkPageSetup *setup = btk_page_setup_new ();

  if (!btk_page_setup_load_file (setup, file_name, error))
    {
      g_object_unref (setup);
      setup = NULL;
    }

  return setup;
}

/* something like this should really be in bobject! */
static guint
string_to_enum (GType type,
                const char *enum_string)
{
  GEnumClass *enum_class;
  const GEnumValue *value;
  guint retval = 0;

  g_return_val_if_fail (enum_string != NULL, 0);

  enum_class = g_type_class_ref (type);
  value = g_enum_get_value_by_nick (enum_class, enum_string);
  if (value)
    retval = value->value;

  g_type_class_unref (enum_class);

  return retval;
}

/**
 * btk_page_setup_load_key_file:
 * @setup: a #BtkPageSetup
 * @key_file: the #GKeyFile to retrieve the page_setup from
 * @group_name: (allow-none): the name of the group in the key_file to read, or %NULL
 *              to use the default name "Page Setup"
 * @error: (allow-none): return location for an error, or %NULL
 * 
 * Reads the page setup from the group @group_name in the key file
 * @key_file.
 * 
 * Return value: %TRUE on success
 *
 * Since: 2.14
 */
gboolean
btk_page_setup_load_key_file (BtkPageSetup *setup,
                              GKeyFile     *key_file,
                              const gchar  *group_name,
                              GError      **error)
{
  BtkPaperSize *paper_size;
  gdouble top, bottom, left, right;
  char *orientation = NULL, *freeme = NULL;
  gboolean retval = FALSE;
  GError *err = NULL;

  g_return_val_if_fail (BTK_IS_PAGE_SETUP (setup), FALSE);
  g_return_val_if_fail (key_file != NULL, FALSE);

  if (!group_name)
    group_name = KEYFILE_GROUP_NAME;

  if (!g_key_file_has_group (key_file, group_name))
    {
      g_set_error_literal (error,
                           BTK_PRINT_ERROR,
                           BTK_PRINT_ERROR_INVALID_FILE,
                           _("Not a valid page setup file"));
      goto out;
    }

#define GET_DOUBLE(kf, group, name, v) \
  v = g_key_file_get_double (kf, group, name, &err); \
  if (err != NULL) \
    { \
      g_propagate_error (error, err);\
      goto out;\
    }

  GET_DOUBLE (key_file, group_name, "MarginTop", top);
  GET_DOUBLE (key_file, group_name, "MarginBottom", bottom);
  GET_DOUBLE (key_file, group_name, "MarginLeft", left);
  GET_DOUBLE (key_file, group_name, "MarginRight", right);

#undef GET_DOUBLE

  paper_size = btk_paper_size_new_from_key_file (key_file, group_name, &err);
  if (!paper_size)
    {
      g_propagate_error (error, err);
      goto out;
    }

  btk_page_setup_set_paper_size (setup, paper_size);
  btk_paper_size_free (paper_size);

  btk_page_setup_set_top_margin (setup, top, BTK_UNIT_MM);
  btk_page_setup_set_bottom_margin (setup, bottom, BTK_UNIT_MM);
  btk_page_setup_set_left_margin (setup, left, BTK_UNIT_MM);
  btk_page_setup_set_right_margin (setup, right, BTK_UNIT_MM);

  orientation = g_key_file_get_string (key_file, group_name,
				       "Orientation", NULL);
  if (orientation)
    {
      btk_page_setup_set_orientation (setup,
				      string_to_enum (BTK_TYPE_PAGE_ORIENTATION,
						      orientation));
      g_free (orientation);
    }

  retval = TRUE;

out:
  g_free (freeme);
  return retval;
}

/**
 * btk_page_setup_new_from_key_file:
 * @key_file: the #GKeyFile to retrieve the page_setup from
 * @group_name: (allow-none): the name of the group in the key_file to read, or %NULL
 *              to use the default name "Page Setup"
 * @error: (allow-none): return location for an error, or %NULL
 *
 * Reads the page setup from the group @group_name in the key file
 * @key_file. Returns a new #BtkPageSetup object with the restored
 * page setup, or %NULL if an error occurred.
 *
 * Return value: the restored #BtkPageSetup
 *
 * Since: 2.12
 */
BtkPageSetup *
btk_page_setup_new_from_key_file (GKeyFile     *key_file,
				  const gchar  *group_name,
				  GError      **error)
{
  BtkPageSetup *setup = btk_page_setup_new ();

  if (!btk_page_setup_load_key_file (setup, key_file, group_name, error))
    {
      g_object_unref (setup);
      setup = NULL;
    }

  return setup;
}

/**
 * btk_page_setup_to_file:
 * @setup: a #BtkPageSetup
 * @file_name: the file to save to
 * @error: (allow-none): return location for errors, or %NULL
 * 
 * This function saves the information from @setup to @file_name.
 * 
 * Return value: %TRUE on success
 *
 * Since: 2.12
 */
gboolean
btk_page_setup_to_file (BtkPageSetup  *setup,
		        const char    *file_name,
			GError       **error)
{
  GKeyFile *key_file;
  gboolean retval = FALSE;
  char *data = NULL;
  gsize len;

  g_return_val_if_fail (BTK_IS_PAGE_SETUP (setup), FALSE);
  g_return_val_if_fail (file_name != NULL, FALSE);

  key_file = g_key_file_new ();
  btk_page_setup_to_key_file (setup, key_file, NULL);

  data = g_key_file_to_data (key_file, &len, error);
  if (!data)
    goto out;

  retval = g_file_set_contents (file_name, data, len, error);

out:
  g_key_file_free (key_file);
  g_free (data);

  return retval;
}

/* something like this should really be in bobject! */
static char *
enum_to_string (GType type,
                guint enum_value)
{
  GEnumClass *enum_class;
  GEnumValue *value;
  char *retval = NULL;

  enum_class = g_type_class_ref (type);

  value = g_enum_get_value (enum_class, enum_value);
  if (value)
    retval = g_strdup (value->value_nick);

  g_type_class_unref (enum_class);

  return retval;
}

/**
 * btk_page_setup_to_key_file:
 * @setup: a #BtkPageSetup
 * @key_file: the #GKeyFile to save the page setup to
 * @group_name: the group to add the settings to in @key_file, 
 *      or %NULL to use the default name "Page Setup"
 * 
 * This function adds the page setup from @setup to @key_file.
 * 
 * Since: 2.12
 */
void
btk_page_setup_to_key_file (BtkPageSetup *setup,
			    GKeyFile     *key_file,
			    const gchar  *group_name)
{
  BtkPaperSize *paper_size;
  char *orientation;

  g_return_if_fail (BTK_IS_PAGE_SETUP (setup));
  g_return_if_fail (key_file != NULL);

  if (!group_name)
    group_name = KEYFILE_GROUP_NAME;

  paper_size = btk_page_setup_get_paper_size (setup);
  g_assert (paper_size != NULL);

  btk_paper_size_to_key_file (paper_size, key_file, group_name);

  g_key_file_set_double (key_file, group_name,
			 "MarginTop", btk_page_setup_get_top_margin (setup, BTK_UNIT_MM));
  g_key_file_set_double (key_file, group_name,
			 "MarginBottom", btk_page_setup_get_bottom_margin (setup, BTK_UNIT_MM));
  g_key_file_set_double (key_file, group_name,
			 "MarginLeft", btk_page_setup_get_left_margin (setup, BTK_UNIT_MM));
  g_key_file_set_double (key_file, group_name,
			 "MarginRight", btk_page_setup_get_right_margin (setup, BTK_UNIT_MM));

  orientation = enum_to_string (BTK_TYPE_PAGE_ORIENTATION,
				btk_page_setup_get_orientation (setup));
  g_key_file_set_string (key_file, group_name,
			 "Orientation", orientation);
  g_free (orientation);
}

#define __BTK_PAGE_SETUP_C__
#include "btkaliasdef.c"
