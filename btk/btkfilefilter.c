/* BTK - The GIMP Toolkit
 * btkfilefilter.c: Filters for selecting a file subset
 * Copyright (C) 2003, Red Hat, Inc.
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

#include "btkfilefilter.h"
#include "btkintl.h"
#include "btkprivate.h"

#include "btkalias.h"

typedef struct _BtkFileFilterClass BtkFileFilterClass;
typedef struct _FilterRule FilterRule;

#define BTK_FILE_FILTER_CLASS(klass)     (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_FILE_FILTER, BtkFileFilterClass))
#define BTK_IS_FILE_FILTER_CLASS(klass)  (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_FILE_FILTER))
#define BTK_FILE_FILTER_GET_CLASS(obj)   (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_FILE_FILTER, BtkFileFilterClass))

typedef enum {
  FILTER_RULE_PATTERN,
  FILTER_RULE_MIME_TYPE,
  FILTER_RULE_PIXBUF_FORMATS,
  FILTER_RULE_CUSTOM
} FilterRuleType;

struct _BtkFileFilterClass
{
  BtkObjectClass parent_class;
};

struct _BtkFileFilter
{
  BtkObject parent_instance;
  
  bchar *name;
  GSList *rules;

  BtkFileFilterFlags needed;
};

struct _FilterRule
{
  FilterRuleType type;
  BtkFileFilterFlags needed;
  
  union {
    bchar *pattern;
    bchar *mime_type;
    GSList *pixbuf_formats;
    struct {
      BtkFileFilterFunc func;
      bpointer data;
      GDestroyNotify notify;
    } custom;
  } u;
};

static void btk_file_filter_finalize   (BObject            *object);


G_DEFINE_TYPE (BtkFileFilter, btk_file_filter, BTK_TYPE_OBJECT)

static void
btk_file_filter_init (BtkFileFilter *object)
{
}

static void
btk_file_filter_class_init (BtkFileFilterClass *class)
{
  BObjectClass *bobject_class = B_OBJECT_CLASS (class);

  bobject_class->finalize = btk_file_filter_finalize;
}

static void
filter_rule_free (FilterRule *rule)
{
  switch (rule->type)
    {
    case FILTER_RULE_MIME_TYPE:
      g_free (rule->u.mime_type);
      break;
    case FILTER_RULE_PATTERN:
      g_free (rule->u.pattern);
      break;
    case FILTER_RULE_CUSTOM:
      if (rule->u.custom.notify)
	rule->u.custom.notify (rule->u.custom.data);
      break;
    case FILTER_RULE_PIXBUF_FORMATS:
      b_slist_free (rule->u.pixbuf_formats);
      break;
    default:
      g_assert_not_reached ();
    }

  g_slice_free (FilterRule, rule);
}

static void
btk_file_filter_finalize (BObject  *object)
{
  BtkFileFilter *filter = BTK_FILE_FILTER (object);

  b_slist_foreach (filter->rules, (GFunc)filter_rule_free, NULL);
  b_slist_free (filter->rules);

  g_free (filter->name);

  B_OBJECT_CLASS (btk_file_filter_parent_class)->finalize (object);
}

/**
 * btk_file_filter_new:
 * 
 * Creates a new #BtkFileFilter with no rules added to it.
 * Such a filter doesn't accept any files, so is not
 * particularly useful until you add rules with
 * btk_file_filter_add_mime_type(), btk_file_filter_add_pattern(),
 * or btk_file_filter_add_custom(). To create a filter
 * that accepts any file, use:
 * |[
 * BtkFileFilter *filter = btk_file_filter_new ();
 * btk_file_filter_add_pattern (filter, "*");
 * ]|
 * 
 * Return value: a new #BtkFileFilter
 * 
 * Since: 2.4
 **/
BtkFileFilter *
btk_file_filter_new (void)
{
  return g_object_new (BTK_TYPE_FILE_FILTER, NULL);
}

/**
 * btk_file_filter_set_name:
 * @filter: a #BtkFileFilter
 * @name: (allow-none): the human-readable-name for the filter, or %NULL
 *   to remove any existing name.
 * 
 * Sets the human-readable name of the filter; this is the string
 * that will be displayed in the file selector user interface if
 * there is a selectable list of filters.
 * 
 * Since: 2.4
 **/
void
btk_file_filter_set_name (BtkFileFilter *filter,
			  const bchar   *name)
{
  g_return_if_fail (BTK_IS_FILE_FILTER (filter));
  
  g_free (filter->name);

  filter->name = g_strdup (name);
}

/**
 * btk_file_filter_get_name:
 * @filter: a #BtkFileFilter
 * 
 * Gets the human-readable name for the filter. See btk_file_filter_set_name().
 * 
 * Return value: The human-readable name of the filter,
 *   or %NULL. This value is owned by BTK+ and must not
 *   be modified or freed.
 * 
 * Since: 2.4
 **/
const bchar *
btk_file_filter_get_name (BtkFileFilter *filter)
{
  g_return_val_if_fail (BTK_IS_FILE_FILTER (filter), NULL);
  
  return filter->name;
}

static void
file_filter_add_rule (BtkFileFilter *filter,
		      FilterRule    *rule)
{
  filter->needed |= rule->needed;
  filter->rules = b_slist_append (filter->rules, rule);
}

/**
 * btk_file_filter_add_mime_type:
 * @filter: A #BtkFileFilter
 * @mime_type: name of a MIME type
 * 
 * Adds a rule allowing a given mime type to @filter.
 * 
 * Since: 2.4
 **/
void
btk_file_filter_add_mime_type (BtkFileFilter *filter,
			       const bchar   *mime_type)
{
  FilterRule *rule;
  
  g_return_if_fail (BTK_IS_FILE_FILTER (filter));
  g_return_if_fail (mime_type != NULL);

  rule = g_slice_new (FilterRule);
  rule->type = FILTER_RULE_MIME_TYPE;
  rule->needed = BTK_FILE_FILTER_MIME_TYPE;
  rule->u.mime_type = g_strdup (mime_type);

  file_filter_add_rule (filter, rule);
}

/**
 * btk_file_filter_add_pattern:
 * @filter: a #BtkFileFilter
 * @pattern: a shell style glob
 * 
 * Adds a rule allowing a shell style glob to a filter.
 * 
 * Since: 2.4
 **/
void
btk_file_filter_add_pattern (BtkFileFilter *filter,
			     const bchar   *pattern)
{
  FilterRule *rule;
  
  g_return_if_fail (BTK_IS_FILE_FILTER (filter));
  g_return_if_fail (pattern != NULL);

  rule = g_slice_new (FilterRule);
  rule->type = FILTER_RULE_PATTERN;
  rule->needed = BTK_FILE_FILTER_DISPLAY_NAME;
  rule->u.pattern = g_strdup (pattern);

  file_filter_add_rule (filter, rule);
}

/**
 * btk_file_filter_add_pixbuf_formats:
 * @filter: a #BtkFileFilter
 * 
 * Adds a rule allowing image files in the formats supported
 * by BdkPixbuf.
 * 
 * Since: 2.6
 **/
void
btk_file_filter_add_pixbuf_formats (BtkFileFilter *filter)
{
  FilterRule *rule;
  
  g_return_if_fail (BTK_IS_FILE_FILTER (filter));

  rule = g_slice_new (FilterRule);
  rule->type = FILTER_RULE_PIXBUF_FORMATS;
  rule->needed = BTK_FILE_FILTER_MIME_TYPE;
  rule->u.pixbuf_formats = bdk_pixbuf_get_formats ();
  file_filter_add_rule (filter, rule);
}


/**
 * btk_file_filter_add_custom:
 * @filter: a #BtkFileFilter
 * @needed: bitfield of flags indicating the information that the custom
 *          filter function needs.
 * @func: callback function; if the function returns %TRUE, then
 *   the file will be displayed.
 * @data: data to pass to @func
 * @notify: function to call to free @data when it is no longer needed.
 * 
 * Adds rule to a filter that allows files based on a custom callback
 * function. The bitfield @needed which is passed in provides information
 * about what sorts of information that the filter function needs;
 * this allows BTK+ to avoid retrieving expensive information when
 * it isn't needed by the filter.
 * 
 * Since: 2.4
 **/
void
btk_file_filter_add_custom (BtkFileFilter         *filter,
			    BtkFileFilterFlags     needed,
			    BtkFileFilterFunc      func,
			    bpointer               data,
			    GDestroyNotify         notify)
{
  FilterRule *rule;
  
  g_return_if_fail (BTK_IS_FILE_FILTER (filter));
  g_return_if_fail (func != NULL);

  rule = g_slice_new (FilterRule);
  rule->type = FILTER_RULE_CUSTOM;
  rule->needed = needed;
  rule->u.custom.func = func;
  rule->u.custom.data = data;
  rule->u.custom.notify = notify;

  file_filter_add_rule (filter, rule);
}

/**
 * btk_file_filter_get_needed:
 * @filter: a #BtkFileFilter
 * 
 * Gets the fields that need to be filled in for the structure
 * passed to btk_file_filter_filter()
 * 
 * This function will not typically be used by applications; it
 * is intended principally for use in the implementation of
 * #BtkFileChooser.
 * 
 * Return value: bitfield of flags indicating needed fields when
 *   calling btk_file_filter_filter()
 * 
 * Since: 2.4
 **/
BtkFileFilterFlags
btk_file_filter_get_needed (BtkFileFilter *filter)
{
  return filter->needed;
}

/**
 * btk_file_filter_filter:
 * @filter: a #BtkFileFilter
 * @filter_info: a #BtkFileFilterInfo structure containing information
 *  about a file.
 * 
 * Tests whether a file should be displayed according to @filter.
 * The #BtkFileFilterInfo structure @filter_info should include
 * the fields returned from btk_file_filter_get_needed().
 *
 * This function will not typically be used by applications; it
 * is intended principally for use in the implementation of
 * #BtkFileChooser.
 * 
 * Return value: %TRUE if the file should be displayed
 * 
 * Since: 2.4
 **/
bboolean
btk_file_filter_filter (BtkFileFilter           *filter,
			const BtkFileFilterInfo *filter_info)
{
  GSList *tmp_list;

  for (tmp_list = filter->rules; tmp_list; tmp_list = tmp_list->next)
    {
      FilterRule *rule = tmp_list->data;

      if ((filter_info->contains & rule->needed) != rule->needed)
	continue;

      switch (rule->type)
	{
	case FILTER_RULE_MIME_TYPE:
          if (filter_info->mime_type != NULL)
            {
              bchar *filter_content_type, *rule_content_type;
              bboolean match;

              filter_content_type = g_content_type_from_mime_type (filter_info->mime_type);
              rule_content_type = g_content_type_from_mime_type (rule->u.mime_type);
              match = g_content_type_is_a (filter_content_type, rule_content_type);
              g_free (filter_content_type);
              g_free (rule_content_type);

              if (match)
                return TRUE;
            }
	  break;
	case FILTER_RULE_PATTERN:
	  if (filter_info->display_name != NULL &&
	      _btk_fnmatch (rule->u.pattern, filter_info->display_name, FALSE))
	    return TRUE;
	  break;
	case FILTER_RULE_PIXBUF_FORMATS:
	  {
	    GSList *list;

	    if (!filter_info->mime_type)
	      break;

	    for (list = rule->u.pixbuf_formats; list; list = list->next)
	      {
		int i;
		bchar **mime_types;

		mime_types = bdk_pixbuf_format_get_mime_types (list->data);

		for (i = 0; mime_types[i] != NULL; i++)
		  {
		    if (strcmp (mime_types[i], filter_info->mime_type) == 0)
		      {
			g_strfreev (mime_types);
			return TRUE;
		      }
		  }

		g_strfreev (mime_types);
	      }
	    break;
	  }
	case FILTER_RULE_CUSTOM:
	  if (rule->u.custom.func (filter_info, rule->u.custom.data))
	    return TRUE;
	  break;
	}
    }

  return FALSE;
}

#define __BTK_FILE_FILTER_C__
#include "btkaliasdef.c"
