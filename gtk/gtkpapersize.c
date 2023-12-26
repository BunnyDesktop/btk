/* BTK - The GIMP Toolkit
 * btkpapersize.c: Paper Size
 * Copyright (C) 2006, Red Hat, Inc.
 * Copyright © 2006, 2007 Christian Persch
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
#include <locale.h>
#if defined(HAVE__NL_PAPER_HEIGHT) && defined(HAVE__NL_PAPER_WIDTH)
#include <langinfo.h>
#endif

#include "btkpapersize.h"
#include "btkprintutils.h"
#include "btkprintoperation.h"  /* for BtkPrintError */
#include "btkintl.h"
#include "btkalias.h"

#include "paper_names_offsets.c"

struct _BtkPaperSize
{
  const PaperInfo *info;

  /* If these are not set we fall back to info */
  gchar *name;
  gchar *display_name;
  gchar *ppd_name;
  
  gdouble width, height; /* Stored in mm */
  gboolean is_custom;
};

GType
btk_paper_size_get_type (void)
{
  static GType our_type = 0;
  
  if (our_type == 0)
    our_type = g_boxed_type_register_static (I_("BtkPaperSize"),
					     (GBoxedCopyFunc)btk_paper_size_copy,
					     (GBoxedFreeFunc)btk_paper_size_free);
  return our_type;
}

static const PaperInfo *
lookup_paper_info (const gchar *name)
{
  int lower = 0;
  int upper = G_N_ELEMENTS (standard_names_offsets) - 1;
  int mid;
  int cmp;

  do 
    {
       mid = (lower + upper) / 2;
       cmp = strcmp (name, paper_names + standard_names_offsets[mid].name);
       if (cmp < 0)
         upper = mid - 1;
       else if (cmp > 0)
         lower = mid + 1;
       else
	 return &standard_names_offsets[mid];
    }
  while (lower <= upper);

  return NULL;
}

static gboolean
parse_media_size (const gchar *size,
		  gdouble     *width_mm, 
		  gdouble     *height_mm)
{
  const char *p;
  char *e;
  double short_dim, long_dim;

  p = size;
  
  short_dim = g_ascii_strtod (p, &e);

  if (p == e || *e != 'x')
    return FALSE;

  p = e + 1; /* Skip x */

  long_dim = g_ascii_strtod (p, &e);

  if (p == e)
    return FALSE;

  p = e;

  if (strcmp (p, "in") == 0)
    {
      short_dim = short_dim * MM_PER_INCH;
      long_dim = long_dim * MM_PER_INCH;
    }
  else if (strcmp (p, "mm") != 0)
    return FALSE;

  if (width_mm)
    *width_mm = short_dim;
  if (height_mm)
    *height_mm = long_dim;
  
  return TRUE;  
}

static gboolean
parse_full_media_size_name (const gchar  *full_name,
			    gchar       **name,
			    gdouble      *width_mm, 
			    gdouble      *height_mm)
{
  const char *p;
  const char *end_of_name;
  
  /* From the spec:
   media-size-self-describing-name =
        ( class-in "_" size-name "_" short-dim "x" long-dim "in" ) |
        ( class-mm "_" size-name "_" short-dim "x" long-dim "mm" )
   class-in = "custom" | "na" | "asme" | "roc" | "oe"
   class-mm = "custom" | "iso" | "jis" | "jpn" | "prc" | "om"
   size-name = ( lowalpha | digit ) *( lowalpha | digit | "-" )
   short-dim = dim
   long-dim = dim
   dim = integer-part [fraction-part] | "0" fraction-part
   integer-part = non-zero-digit *digit
   fraction-part = "." *digit non-zero-digit
   lowalpha = "a" | "b" | "c" | "d" | "e" | "f" | "g" | "h" | "i" |
              "j" | "k" | "l" | "m" | "n" | "o" | "p" | "q" | "r" |
              "s" | "t" | "u" | "v" | "w" | "x" | "y" | "z"
   non-zero-digit = "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9"
   digit    = "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9"
 */

  p = strchr (full_name, '_');
  if (p == NULL)
    return FALSE;

  p++; /* Skip _ */
  
  p = strchr (p, '_');
  if (p == NULL)
    return FALSE;

  end_of_name = p;

  p++; /* Skip _ */

  if (!parse_media_size (p, width_mm, height_mm))
    return FALSE;

  if (name)
    *name = g_strndup (full_name, end_of_name - full_name);
  
  return TRUE;  
}

static BtkPaperSize *
btk_paper_size_new_from_info (const PaperInfo *info)
{
  BtkPaperSize *size;
  
  size = g_slice_new0 (BtkPaperSize);
  size->info = info;
  size->width = info->width;
  size->height = info->height;
  
  return size;
}

/**
 * btk_paper_size_new:
 * @name: (allow-none): a paper size name, or %NULL
 *
 * Creates a new #BtkPaperSize object by parsing a
 * <ulink url="ftp://ftp.pwg.org/pub/pwg/candidates/cs-pwgmsn10-20020226-5101.1.pdf">PWG 5101.1-2002</ulink>
 * paper name.
 *
 * If @name is %NULL, the default paper size is returned,
 * see btk_paper_size_get_default().
 * 
 * Return value: a new #BtkPaperSize, use btk_paper_size_free()
 * to free it
 *
 * Since: 2.10
 */
BtkPaperSize *
btk_paper_size_new (const gchar *name)
{
  BtkPaperSize *size;
  char *short_name;
  double width, height;
  const PaperInfo *info;

  if (name == NULL)
    name = btk_paper_size_get_default ();
  
  if (parse_full_media_size_name (name, &short_name, &width, &height))
    {
      size = g_slice_new0 (BtkPaperSize);

      size->width = width;
      size->height = height;
      size->name = short_name;
      size->display_name = g_strdup (short_name);
      if (strncmp (short_name, "custom", 6) == 0)
        size->is_custom = TRUE;
    }
  else
    {
      info = lookup_paper_info (name);
      if (info != NULL)
	size = btk_paper_size_new_from_info (info);
      else
	{
	  g_warning ("Unknown paper size %s\n", name);
	  size = g_slice_new0 (BtkPaperSize);
	  size->name = g_strdup (name);
	  size->display_name = g_strdup (name);
	  /* Default to A4 size */
	  size->width = 210;
	  size->height = 297;
	}
    }
  
  return size;
}

/**
 * btk_paper_size_new_from_ppd:
 * @ppd_name: a PPD paper name
 * @ppd_display_name: the corresponding human-readable name
 * @width: the paper width, in points
 * @height: the paper height in points
 * 
 * Creates a new #BtkPaperSize object by using 
 * PPD information. 
 * 
 * If @ppd_name is not a recognized PPD paper name, 
 * @ppd_display_name, @width and @height are used to 
 * construct a custom #BtkPaperSize object.
 *
 * Return value: a new #BtkPaperSize, use btk_paper_size_free()
 * to free it
 *
 * Since: 2.10
 */
BtkPaperSize *
btk_paper_size_new_from_ppd (const gchar *ppd_name,
			     const gchar *ppd_display_name,
			     gdouble      width,
			     gdouble      height)
{
  char *name;
  const char *lookup_ppd_name;
  char *freeme;
  BtkPaperSize *size;
  int i;

  lookup_ppd_name = ppd_name;
  
  freeme = NULL;
  /* Strip out Traverse suffix in matching. */
  if (g_str_has_suffix (ppd_name, ".Transverse"))
    {
      lookup_ppd_name = freeme =
	g_strndup (ppd_name, strlen (ppd_name) - strlen (".Transverse"));
    }
  
  for (i = 0; i < G_N_ELEMENTS(standard_names_offsets); i++)
    {
      if (standard_names_offsets[i].ppd_name != -1 &&
	  strcmp (paper_names + standard_names_offsets[i].ppd_name, lookup_ppd_name) == 0)
	{
	  size = btk_paper_size_new_from_info (&standard_names_offsets[i]);
	  goto out;
	}
    }
  
  for (i = 0; i < G_N_ELEMENTS(extra_ppd_names_offsets); i++)
    {
      if (strcmp (paper_names + extra_ppd_names_offsets[i].ppd_name, lookup_ppd_name) == 0)
	{
	  size = btk_paper_size_new (paper_names + extra_ppd_names_offsets[i].standard_name);
	  goto out;
	}
    }

  name = g_strconcat ("ppd_", ppd_name, NULL);
  size = btk_paper_size_new_custom (name, ppd_display_name, width, height, BTK_UNIT_POINTS);
  g_free (name);

 out:

  if (size->info == NULL ||
      size->info->ppd_name == -1 ||
      strcmp (paper_names + size->info->ppd_name, ppd_name) != 0)
    size->ppd_name = g_strdup (ppd_name);
  
  g_free (freeme);
  
  return size;
}

/**
 * btk_paper_size_new_custom:
 * @name: the paper name 
 * @display_name: the human-readable name
 * @width: the paper width, in units of @unit
 * @height: the paper height, in units of @unit
 * @unit: the unit for @width and @height
 * 
 * Creates a new #BtkPaperSize object with the
 * given parameters.
 * 
 * Return value: a new #BtkPaperSize object, use btk_paper_size_free()
 * to free it
 *
 * Since: 2.10
 */
BtkPaperSize *
btk_paper_size_new_custom (const gchar *name, 
			   const gchar *display_name,
			   gdouble      width, 
			   gdouble      height, 
			   BtkUnit      unit)
{
  BtkPaperSize *size;
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (unit != BTK_UNIT_PIXEL, NULL);

  size = g_slice_new0 (BtkPaperSize);
  
  size->name = g_strdup (name);
  size->display_name = g_strdup (display_name);
  size->is_custom = TRUE;
  
  size->width = _btk_print_convert_to_mm (width, unit);
  size->height = _btk_print_convert_to_mm (height, unit);
  
  return size;
}

/**
 * btk_paper_size_copy:
 * @other: a #BtkPaperSize
 * 
 * Copies an existing #BtkPaperSize.
 * 
 * Return value: a copy of @other
 *
 * Since: 2.10
 */
BtkPaperSize *
btk_paper_size_copy (BtkPaperSize *other)
{
  BtkPaperSize *size;

  size = g_slice_new0 (BtkPaperSize);

  size->info = other->info;
  if (other->name)
    size->name = g_strdup (other->name);
  if (other->display_name)
    size->display_name = g_strdup (other->display_name);
  if (other->ppd_name)
    size->ppd_name = g_strdup (other->ppd_name);
  
  size->width = other->width;
  size->height = other->height;
  size->is_custom = other->is_custom;

  return size;
}

/**
 * btk_paper_size_free:
 * @size: a #BtkPaperSize
 * 
 * Free the given #BtkPaperSize object.
 *
 * Since: 2.10
 */
void
btk_paper_size_free (BtkPaperSize *size)
{
  g_free (size->name);
  g_free (size->display_name);
  g_free (size->ppd_name);

  g_slice_free (BtkPaperSize, size);
}

/**
 * btk_paper_size_is_equal:
 * @size1: a #BtkPaperSize object
 * @size2: another #BtkPaperSize object
 * 
 * Compares two #BtkPaperSize objects.
 * 
 * Return value: %TRUE, if @size1 and @size2 
 * represent the same paper size
 *
 * Since: 2.10
 */
gboolean
btk_paper_size_is_equal (BtkPaperSize *size1,
			 BtkPaperSize *size2)
{
  if (size1->info != NULL && size2->info != NULL)
    return size1->info == size2->info;
  
  return strcmp (btk_paper_size_get_name (size1),
		 btk_paper_size_get_name (size2)) == 0;
}

GList * _btk_load_custom_papers (void);

/**
 * btk_paper_size_get_paper_sizes:
 * @include_custom: whether to include custom paper sizes
 *     as defined in the page setup dialog
 *
 * Creates a list of known paper sizes.
 *
 * Return value:  (element-type BtkPaperSize) (transfer full): a newly allocated list of newly
 *    allocated #BtkPaperSize objects
 *
 * Since: 2.12
 */
GList *
btk_paper_size_get_paper_sizes (gboolean include_custom)
{
  GList *list = NULL;
  guint i;
#ifdef G_OS_UNIX		/* _btk_load_custom_papers() only on Unix so far  */
  if (include_custom)
    {
      GList *page_setups, *l;

      page_setups = _btk_load_custom_papers ();
      for (l = page_setups; l != NULL; l = l->next)
        {
          BtkPageSetup *setup = (BtkPageSetup *) l->data;
          BtkPaperSize *size;

          size = btk_page_setup_get_paper_size (setup);
          list = g_list_prepend (list, btk_paper_size_copy (size));
        }

      g_list_foreach (page_setups, (GFunc) g_object_unref, NULL);
      g_list_free (page_setups);
    }
#endif
  for (i = 0; i < G_N_ELEMENTS (standard_names_offsets); ++i)
    {
       BtkPaperSize *size;

       size = btk_paper_size_new_from_info (&standard_names_offsets[i]);
       list = g_list_prepend (list, size);
    }

  return g_list_reverse (list);
}


/**
 * btk_paper_size_get_name:
 * @size: a #BtkPaperSize object
 * 
 * Gets the name of the #BtkPaperSize.
 * 
 * Return value: the name of @size
 *
 * Since: 2.10
 */
const gchar *
btk_paper_size_get_name (BtkPaperSize *size)
{
  if (size->name)
    return size->name;
  g_assert (size->info != NULL);
  return paper_names + size->info->name;
}

/**
 * btk_paper_size_get_display_name:
 * @size: a #BtkPaperSize object
 * 
 * Gets the human-readable name of the #BtkPaperSize.
 * 
 * Return value: the human-readable name of @size
 *
 * Since: 2.10
 */
const gchar *
btk_paper_size_get_display_name (BtkPaperSize *size)
{
  const gchar *display_name;

  if (size->display_name)
    return size->display_name;

  g_assert (size->info != NULL);

  display_name = paper_names + size->info->display_name;
  return g_dpgettext2 (GETTEXT_PACKAGE, "paper size", display_name);
}

/**
 * btk_paper_size_get_ppd_name:
 * @size: a #BtkPaperSize object
 * 
 * Gets the PPD name of the #BtkPaperSize, which
 * may be %NULL.
 * 
 * Return value: the PPD name of @size
 *
 * Since: 2.10
 */
const gchar *
btk_paper_size_get_ppd_name (BtkPaperSize *size)
{
  if (size->ppd_name)
    return size->ppd_name;
  if (size->info)
    return paper_names + size->info->ppd_name;
  return NULL;
}

/**
 * btk_paper_size_get_width:
 * @size: a #BtkPaperSize object
 * @unit: the unit for the return value
 * 
 * Gets the paper width of the #BtkPaperSize, in 
 * units of @unit.
 * 
 * Return value: the paper width 
 *
 * Since: 2.10
 */
gdouble
btk_paper_size_get_width (BtkPaperSize *size, 
			  BtkUnit       unit)
{
  return _btk_print_convert_from_mm (size->width, unit);
}

/**
 * btk_paper_size_get_height:
 * @size: a #BtkPaperSize object
 * @unit: the unit for the return value
 * 
 * Gets the paper height of the #BtkPaperSize, in 
 * units of @unit.
 * 
 * Return value: the paper height 
 *
 * Since: 2.10
 */
gdouble
btk_paper_size_get_height (BtkPaperSize *size, 
			   BtkUnit       unit)
{
  return _btk_print_convert_from_mm (size->height, unit);
}

/**
 * btk_paper_size_is_custom:
 * @size: a #BtkPaperSize object
 * 
 * Returns %TRUE if @size is not a standard paper size.
 * 
 * Return value: whether @size is a custom paper size.
 **/
gboolean
btk_paper_size_is_custom (BtkPaperSize *size)
{
  return size->is_custom;
}

/**
 * btk_paper_size_set_size:
 * @size: a custom #BtkPaperSize object
 * @width: the new width in units of @unit
 * @height: the new height in units of @unit
 * @unit: the unit for @width and @height
 * 
 * Changes the dimensions of a @size to @width x @height.
 *
 * Since: 2.10
 */
void
btk_paper_size_set_size (BtkPaperSize *size, 
			 gdouble       width, 
			 gdouble       height, 
			 BtkUnit       unit)
{
  g_return_if_fail (size != NULL);
  g_return_if_fail (size->is_custom);

  size->width = _btk_print_convert_to_mm (width, unit);
  size->height = _btk_print_convert_to_mm (height, unit);
}

#define NL_PAPER_GET(x)         \
  ((union { char *string; unsigned int word; })nl_langinfo(x)).word

/**
 * btk_paper_size_get_default:
 *
 * Returns the name of the default paper size, which 
 * depends on the current locale.  
 * 
 * Return value: the name of the default paper size. The string
 * is owned by BTK+ and should not be modified.
 * 
 * Since: 2.10
 */
const gchar *
btk_paper_size_get_default (void)
{
  char *locale, *freeme = NULL;
  const char *paper_size;

#if defined(HAVE__NL_PAPER_HEIGHT) && defined(HAVE__NL_PAPER_WIDTH)
  {
    int width = NL_PAPER_GET (_NL_PAPER_WIDTH);
    int height = NL_PAPER_GET (_NL_PAPER_HEIGHT);
    
    if (width == 210 && height == 297)
      return BTK_PAPER_NAME_A4;
    
    if (width == 216 && height == 279)
      return BTK_PAPER_NAME_LETTER;
  }
#endif

#ifdef G_OS_WIN32
  freeme = locale = g_win32_getlocale ();
#elif defined(LC_PAPER)
  locale = setlocale(LC_PAPER, NULL);
#else
  locale = setlocale(LC_MESSAGES, NULL);
#endif

  if (!locale)
    return BTK_PAPER_NAME_A4;

  /* CLDR 1.8.1
   * http://unicode.org/repos/cldr-tmp/trunk/diff/supplemental/territory_language_information.html
   */
  if (g_regex_match_simple("[^_.@]{2,3}_(BZ|CA|CL|CO|CR|GT|MX|NI|PA|PH|PR|SV|US|VE)",
                           locale, G_REGEX_ANCHORED, G_REGEX_MATCH_ANCHORED))
    paper_size = BTK_PAPER_NAME_LETTER;
  else
    paper_size = BTK_PAPER_NAME_A4;

  g_free (freeme);
  return paper_size;
}

/* These get the default margins used for the paper size. Its
 * larger than most printers margins, so that it will be within
 * the imageble area on any printer.
 *
 * I've taken the actual values used from the OSX page setup dialog.
 * I'm not sure exactly where they got these values for, but might
 * correspond to this (from ghostscript docs):
 * 
 * All DeskJets have 0.5 inches (1.27cm) of unprintable bottom margin,
 * due to the mechanical arrangement used to grab the paper. Side margins
 * are approximately 0.25 inches (0.64cm) for U.S. letter paper, and 0.15
 * inches (0.38cm) for A4.
 */

/**
 * btk_paper_size_get_default_top_margin:
 * @size: a #BtkPaperSize object
 * @unit: the unit for the return value
 * 
 * Gets the default top margin for the #BtkPaperSize.
 * 
 * Return value: the default top margin
 *
 * Since: 2.10
 */
gdouble
btk_paper_size_get_default_top_margin (BtkPaperSize *size, 
				       BtkUnit       unit)
{
  gdouble margin;

  margin = _btk_print_convert_to_mm (0.25, BTK_UNIT_INCH);
  return _btk_print_convert_from_mm (margin, unit);
}

/**
 * btk_paper_size_get_default_bottom_margin:
 * @size: a #BtkPaperSize object
 * @unit: the unit for the return value
 * 
 * Gets the default bottom margin for the #BtkPaperSize.
 * 
 * Return value: the default bottom margin
 *
 * Since: 2.10
 */
gdouble
btk_paper_size_get_default_bottom_margin (BtkPaperSize *size, 
					  BtkUnit       unit)
{
  gdouble margin;
  const gchar *name;

  margin = _btk_print_convert_to_mm (0.25, BTK_UNIT_INCH);

  name = btk_paper_size_get_name (size);
  if (strcmp (name, "na_letter") == 0 ||
      strcmp (name, "na_legal") == 0 ||
      strcmp (name, "iso_a4") == 0)
    margin = _btk_print_convert_to_mm (0.56, BTK_UNIT_INCH);
  
  return _btk_print_convert_from_mm (margin, unit);
}

/**
 * btk_paper_size_get_default_left_margin:
 * @size: a #BtkPaperSize object
 * @unit: the unit for the return value
 * 
 * Gets the default left margin for the #BtkPaperSize.
 * 
 * Return value: the default left margin
 *
 * Since: 2.10
 */
gdouble
btk_paper_size_get_default_left_margin (BtkPaperSize *size, 
					BtkUnit       unit)
{
  gdouble margin;

  margin = _btk_print_convert_to_mm (0.25, BTK_UNIT_INCH);
  return _btk_print_convert_from_mm (margin, unit);
}

/**
 * btk_paper_size_get_default_right_margin:
 * @size: a #BtkPaperSize object
 * @unit: the unit for the return value
 * 
 * Gets the default right margin for the #BtkPaperSize.
 * 
 * Return value: the default right margin
 *
 * Since: 2.10
 */
gdouble
btk_paper_size_get_default_right_margin (BtkPaperSize *size, 
					 BtkUnit       unit)
{
  gdouble margin;

  margin = _btk_print_convert_to_mm (0.25, BTK_UNIT_INCH);
  return _btk_print_convert_from_mm (margin, unit);
}

/**
 * btk_paper_size_new_from_key_file:
 * @key_file: the #GKeyFile to retrieve the papersize from
 * @group_name: the name ofthe group in the key file to read,
 *     or %NULL to read the first group
 * @error: (allow-none): return location for an error, or %NULL
 *
 * Reads a paper size from the group @group_name in the key file
 * @key_file. 
 *
 * Returns: a new #BtkPaperSize object with the restored
 *          paper size, or %NULL if an error occurred.
 *
 * Since: 2.12
 */
BtkPaperSize *
btk_paper_size_new_from_key_file (GKeyFile    *key_file,
				  const gchar *group_name,
				  GError     **error)
{
  BtkPaperSize *paper_size = NULL;
  char *name = NULL, *ppd_name = NULL, *display_name = NULL, *freeme = NULL;
  gdouble width, height;
  gboolean retval = TRUE;
  GError *err = NULL;

  g_return_val_if_fail (key_file != NULL, NULL);

  if (!group_name)
    group_name = freeme = g_key_file_get_start_group (key_file);
  if (!group_name || !g_key_file_has_group (key_file, group_name))
    {
      g_set_error_literal (error,
                           BTK_PRINT_ERROR,
                           BTK_PRINT_ERROR_INVALID_FILE,
                           _("Not a valid page setup file"));
      retval = FALSE;
      goto out;
    }

#define GET_DOUBLE(kf, group, name, v) \
  v = g_key_file_get_double (kf, group, name, &err); \
  if (err != NULL) \
    {\
      g_propagate_error (error, err);\
      retval = FALSE;\
      goto out;\
    }

  GET_DOUBLE (key_file, group_name, "Width", width);
  GET_DOUBLE (key_file, group_name, "Height", height);

#undef GET_DOUBLE

  name = g_key_file_get_string (key_file, group_name,
				"Name", NULL);
  ppd_name = g_key_file_get_string (key_file, group_name,
				    "PPDName", NULL);
  display_name = g_key_file_get_string (key_file, group_name,
					"DisplayName", NULL);
  /* Fallback for old ~/.btk-custom-paper entries */
  if (!display_name)
    display_name = g_strdup (name);

  if (ppd_name != NULL)
    paper_size = btk_paper_size_new_from_ppd (ppd_name,
                                              display_name,
                                              _btk_print_convert_from_mm (width, BTK_UNIT_POINTS),
                                              _btk_print_convert_from_mm (height, BTK_UNIT_POINTS));
  else if (name != NULL)
    paper_size = btk_paper_size_new_custom (name, display_name,
					    width, height, BTK_UNIT_MM);
  else
    {
      g_set_error_literal (error,
                           BTK_PRINT_ERROR,
                           BTK_PRINT_ERROR_INVALID_FILE,
                           _("Not a valid page setup file"));
      retval = FALSE;
      goto out;
    }
  g_assert (paper_size != NULL);

out:
  g_free (ppd_name);
  g_free (name);
  g_free (display_name);
  g_free (freeme);

  return paper_size;
}

/**
 * btk_paper_size_to_key_file:
 * @size: a #BtkPaperSize
 * @key_file: the #GKeyFile to save the paper size to
 * @group_name: the group to add the settings to in @key_file
 *
 * This function adds the paper size from @size to @key_file.
 *
 * Since: 2.12
 */
void
btk_paper_size_to_key_file (BtkPaperSize *size,
			    GKeyFile     *key_file,
			    const gchar  *group_name)
{
  const char *name, *ppd_name, *display_name;

  g_return_if_fail (size != NULL);
  g_return_if_fail (key_file != NULL);

  name = btk_paper_size_get_name (size);
  display_name = btk_paper_size_get_display_name (size);
  ppd_name = btk_paper_size_get_ppd_name (size);

  if (ppd_name != NULL) 
    g_key_file_set_string (key_file, group_name,
			   "PPDName", ppd_name);
  else
    g_key_file_set_string (key_file, group_name,
			   "Name", name);

  if (display_name) 
    g_key_file_set_string (key_file, group_name,
			   "DisplayName", display_name);

  g_key_file_set_double (key_file, group_name,
			 "Width", btk_paper_size_get_width (size, BTK_UNIT_MM));
  g_key_file_set_double (key_file, group_name,
			 "Height", btk_paper_size_get_height (size, BTK_UNIT_MM));
}


#define __BTK_PAPER_SIZE_C__
#include "btkaliasdef.c"
