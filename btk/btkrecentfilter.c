/* BTK - The GIMP Toolkit
 * btkrecentfilter.h - Filter object for recently used resources
 * Copyright (C) 2006, Emmanuele Bassi
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

#include "btkrecentfilter.h"
#include "btkintl.h"
#include "btkprivate.h"

#include "btkalias.h"

typedef struct _BtkRecentFilterClass BtkRecentFilterClass;
typedef struct _FilterRule FilterRule;

typedef enum {
  FILTER_RULE_URI,
  FILTER_RULE_DISPLAY_NAME,
  FILTER_RULE_MIME_TYPE,
  FILTER_RULE_PIXBUF_FORMATS,
  FILTER_RULE_APPLICATION,
  FILTER_RULE_AGE,
  FILTER_RULE_GROUP,
  FILTER_RULE_CUSTOM
} FilterRuleType;

struct _BtkRecentFilter
{
  BtkObject parent_instance;
  
  gchar *name;
  GSList *rules;
  
  BtkRecentFilterFlags needed;
};

struct _BtkRecentFilterClass
{
  BtkObjectClass parent_class;
};

struct _FilterRule
{
  FilterRuleType type;
  BtkRecentFilterFlags needed;
  
  union {
    gchar *uri;
    gchar *pattern;
    gchar *mime_type;
    GSList *pixbuf_formats;
    gchar *application;
    gchar *group;
    gint age;
    struct {
      BtkRecentFilterFunc func;
      gpointer data;
      GDestroyNotify data_destroy;
    } custom;
  } u;
};

G_DEFINE_TYPE (BtkRecentFilter, btk_recent_filter, BTK_TYPE_OBJECT)


static void
filter_rule_free (FilterRule *rule)
{
  switch (rule->type)
    {
    case FILTER_RULE_MIME_TYPE:
      g_free (rule->u.mime_type);
      break;
    case FILTER_RULE_URI:
      g_free (rule->u.uri);
      break;
    case FILTER_RULE_DISPLAY_NAME:
      g_free (rule->u.pattern);
      break;
    case FILTER_RULE_PIXBUF_FORMATS:
      b_slist_free (rule->u.pixbuf_formats);
      break;
    case FILTER_RULE_AGE:
      break;
    case FILTER_RULE_APPLICATION:
      g_free (rule->u.application);
      break;
    case FILTER_RULE_GROUP:
      g_free (rule->u.group);
      break;
    case FILTER_RULE_CUSTOM:
      if (rule->u.custom.data_destroy)
        rule->u.custom.data_destroy (rule->u.custom.data);
      break;
    default:
      g_assert_not_reached ();
      break;
    }
  
  g_free (rule);
}

static void
btk_recent_filter_finalize (BObject *object)
{
  BtkRecentFilter *filter = BTK_RECENT_FILTER (object);
  
  g_free (filter->name);
  
  if (filter->rules)
    {
      b_slist_foreach (filter->rules,
      		       (GFunc) filter_rule_free,
      		       NULL);
      b_slist_free (filter->rules);
    }
  
  B_OBJECT_CLASS (btk_recent_filter_parent_class)->finalize (object);
}

static void
btk_recent_filter_class_init (BtkRecentFilterClass *klass)
{
  BObjectClass *bobject_class = B_OBJECT_CLASS (klass);
  
  bobject_class->finalize = btk_recent_filter_finalize;
}

static void
btk_recent_filter_init (BtkRecentFilter *filter)
{

}

/*
 * Public API
 */
 
/**
 * btk_recent_filter_new:
 *
 * Creates a new #BtkRecentFilter with no rules added to it.
 * Such filter does not accept any recently used resources, so is not
 * particularly useful until you add rules with
 * btk_recent_filter_add_pattern(), btk_recent_filter_add_mime_type(),
 * btk_recent_filter_add_application(), btk_recent_filter_add_age().
 * To create a filter that accepts any recently used resource, use:
 * |[
 * BtkRecentFilter *filter = btk_recent_filter_new ();
 * btk_recent_filter_add_pattern (filter, "*");
 * ]|
 *
 * Return value: a new #BtkRecentFilter
 *
 * Since: 2.10
 */
BtkRecentFilter *
btk_recent_filter_new (void)
{
  return g_object_new (BTK_TYPE_RECENT_FILTER, NULL);
}

/**
 * btk_recent_filter_set_name:
 * @filter: a #BtkRecentFilter
 * @name: then human readable name of @filter
 *
 * Sets the human-readable name of the filter; this is the string
 * that will be displayed in the recently used resources selector
 * user interface if there is a selectable list of filters.
 *
 * Since: 2.10
 */
void
btk_recent_filter_set_name (BtkRecentFilter *filter,
			    const gchar     *name)
{
  g_return_if_fail (BTK_IS_RECENT_FILTER (filter));
  
  g_free (filter->name);
  
  if (name)
    filter->name = g_strdup (name);
}

/**
 * btk_recent_filter_get_name:
 * @filter: a #BtkRecentFilter
 *
 * Gets the human-readable name for the filter.
 * See btk_recent_filter_set_name().
 *
 * Return value: the name of the filter, or %NULL.  The returned string
 *   is owned by the filter object and should not be freed.
 *
 * Since: 2.10
 */
const gchar *
btk_recent_filter_get_name (BtkRecentFilter *filter)
{
  g_return_val_if_fail (BTK_IS_RECENT_FILTER (filter), NULL);
  
  return filter->name;
}

/**
 * btk_recent_filter_get_needed:
 * @filter: a #BtkRecentFilter
 *
 * Gets the fields that need to be filled in for the structure
 * passed to btk_recent_filter_filter()
 * 
 * This function will not typically be used by applications; it
 * is intended principally for use in the implementation of
 * #BtkRecentChooser.
 * 
 * Return value: bitfield of flags indicating needed fields when
 *   calling btk_recent_filter_filter()
 *
 * Since: 2.10
 */
BtkRecentFilterFlags
btk_recent_filter_get_needed (BtkRecentFilter *filter)
{
  return filter->needed;
}

static void
recent_filter_add_rule (BtkRecentFilter *filter,
			FilterRule      *rule)
{
  filter->needed |= rule->needed;
  filter->rules = b_slist_append (filter->rules, rule);
}

/**
 * btk_recent_filter_add_mime_type:
 * @filter: a #BtkRecentFilter
 * @mime_type: a MIME type
 *
 * Adds a rule that allows resources based on their registered MIME type.
 *
 * Since: 2.10
 */
void
btk_recent_filter_add_mime_type (BtkRecentFilter *filter,
				 const gchar     *mime_type)
{
  FilterRule *rule;
  
  g_return_if_fail (BTK_IS_RECENT_FILTER (filter));
  g_return_if_fail (mime_type != NULL);
  
  rule = g_new0 (FilterRule, 1);
  rule->type = FILTER_RULE_MIME_TYPE;
  rule->needed = BTK_RECENT_FILTER_MIME_TYPE;
  rule->u.mime_type = g_strdup (mime_type);
  
  recent_filter_add_rule (filter, rule);
}

/**
 * btk_recent_filter_add_pattern:
 * @filter: a #BtkRecentFilter
 * @pattern: a file pattern
 *
 * Adds a rule that allows resources based on a pattern matching their
 * display name.
 *
 * Since: 2.10
 */
void
btk_recent_filter_add_pattern (BtkRecentFilter *filter,
			       const gchar     *pattern)
{
  FilterRule *rule;
  
  g_return_if_fail (BTK_IS_RECENT_FILTER (filter));
  g_return_if_fail (pattern != NULL);
  
  rule = g_new0 (FilterRule, 1);
  rule->type = FILTER_RULE_DISPLAY_NAME;
  rule->needed = BTK_RECENT_FILTER_DISPLAY_NAME;
  rule->u.pattern = g_strdup (pattern);
  
  recent_filter_add_rule (filter, rule);
}

/**
 * btk_recent_filter_add_pixbuf_formats:
 * @filter: a #BtkRecentFilter
 *
 * Adds a rule allowing image files in the formats supported
 * by BdkPixbuf.
 *
 * Since: 2.10
 */
void
btk_recent_filter_add_pixbuf_formats (BtkRecentFilter *filter)
{
  FilterRule *rule;

  g_return_if_fail (BTK_IS_RECENT_FILTER (filter));

  rule = g_new0 (FilterRule, 1);
  rule->type = FILTER_RULE_PIXBUF_FORMATS;
  rule->needed = BTK_RECENT_FILTER_MIME_TYPE;
  rule->u.pixbuf_formats = bdk_pixbuf_get_formats ();
  
  recent_filter_add_rule (filter, rule);
}

/**
 * btk_recent_filter_add_application:
 * @filter: a #BtkRecentFilter
 * @application: an application name
 *
 * Adds a rule that allows resources based on the name of the application
 * that has registered them.
 *
 * Since: 2.10
 */
void
btk_recent_filter_add_application (BtkRecentFilter *filter,
				   const gchar     *application)
{
  FilterRule *rule;
  
  g_return_if_fail (BTK_IS_RECENT_FILTER (filter));
  g_return_if_fail (application != NULL);
  
  rule = g_new0 (FilterRule, 1);
  rule->type = FILTER_RULE_APPLICATION;
  rule->needed = BTK_RECENT_FILTER_APPLICATION;
  rule->u.application = g_strdup (application);
  
  recent_filter_add_rule (filter, rule);
}

/**
 * btk_recent_filter_add_group:
 * @filter: a #BtkRecentFilter
 * @group: a group name
 *
 * Adds a rule that allows resources based on the name of the group
 * to which they belong
 *
 * Since: 2.10
 */
void
btk_recent_filter_add_group (BtkRecentFilter *filter,
			     const gchar     *group)
{
  FilterRule *rule;
  
  g_return_if_fail (BTK_IS_RECENT_FILTER (filter));
  g_return_if_fail (group != NULL);
  
  rule = g_new0 (FilterRule, 1);
  rule->type = FILTER_RULE_GROUP;
  rule->needed = BTK_RECENT_FILTER_GROUP;
  rule->u.group = g_strdup (group);
  
  recent_filter_add_rule (filter, rule);
}

/**
 * btk_recent_filter_add_age:
 * @filter: a #BtkRecentFilter
 * @days: number of days
 *
 * Adds a rule that allows resources based on their age - that is, the number
 * of days elapsed since they were last modified.
 *
 * Since: 2.10
 */
void
btk_recent_filter_add_age (BtkRecentFilter *filter,
			   gint             days)
{
  FilterRule *rule;
  
  g_return_if_fail (BTK_IS_RECENT_FILTER (filter));
  
  rule = g_new0 (FilterRule, 1);
  rule->type = FILTER_RULE_AGE;
  rule->needed = BTK_RECENT_FILTER_AGE;
  rule->u.age = days;
  
  recent_filter_add_rule (filter, rule);
}

/**
 * btk_recent_filter_add_custom:
 * @filter: a #BtkRecentFilter
 * @needed: bitfield of flags indicating the information that the custom
 *          filter function needs.
 * @func: callback function; if the function returns %TRUE, then
 *   the file will be displayed.
 * @data: data to pass to @func
 * @data_destroy: function to call to free @data when it is no longer needed.
 * 
 * Adds a rule to a filter that allows resources based on a custom callback
 * function. The bitfield @needed which is passed in provides information
 * about what sorts of information that the filter function needs;
 * this allows BTK+ to avoid retrieving expensive information when
 * it isn't needed by the filter.
 * 
 * Since: 2.10
 **/
void
btk_recent_filter_add_custom (BtkRecentFilter      *filter,
			      BtkRecentFilterFlags  needed,
			      BtkRecentFilterFunc   func,
			      gpointer              data,
			      GDestroyNotify        data_destroy)
{
  FilterRule *rule;
  
  g_return_if_fail (BTK_IS_RECENT_FILTER (filter));
  g_return_if_fail (func != NULL);

  rule = g_new0 (FilterRule, 1);
  rule->type = FILTER_RULE_CUSTOM;
  rule->needed = needed;
  rule->u.custom.func = func;
  rule->u.custom.data = data;
  rule->u.custom.data_destroy = data_destroy;

  recent_filter_add_rule (filter, rule);
}


/**
 * btk_recent_filter_filter:
 * @filter: a #BtkRecentFilter
 * @filter_info: a #BtkRecentFilterInfo structure containing information
 *   about a recently used resource
 *
 * Tests whether a file should be displayed according to @filter.
 * The #BtkRecentFilterInfo structure @filter_info should include
 * the fields returned from btk_recent_filter_get_needed().
 *
 * This function will not typically be used by applications; it
 * is intended principally for use in the implementation of
 * #BtkRecentChooser.
 * 
 * Return value: %TRUE if the file should be displayed
 *
 * Since: 2.10
 */
gboolean
btk_recent_filter_filter (BtkRecentFilter           *filter,
			  const BtkRecentFilterInfo *filter_info)
{
  GSList *l;
  
  g_return_val_if_fail (BTK_IS_RECENT_FILTER (filter), FALSE);
  g_return_val_if_fail (filter_info != NULL, FALSE);
  
  for (l = filter->rules; l != NULL; l = l->next)
    {
      FilterRule *rule = (FilterRule *) l->data;

      if ((filter_info->contains & rule->needed) != rule->needed)
        continue;

      switch (rule->type)
        {
        case FILTER_RULE_MIME_TYPE:
          if (filter_info->mime_type != NULL)
            {
              gchar *filter_content_type, *rule_content_type;
              gboolean match;

              filter_content_type = g_content_type_from_mime_type (filter_info->mime_type);
              rule_content_type = g_content_type_from_mime_type (rule->u.mime_type);
              match = g_content_type_is_a (filter_content_type, rule_content_type);
              g_free (filter_content_type);
              g_free (rule_content_type);

              if (match)
                return TRUE;
            }
          break;
        case FILTER_RULE_APPLICATION:
          if (filter_info->applications)
            {
              gint i;
              
              for (i = 0; filter_info->applications[i] != NULL; i++)
                {
                  if (strcmp (filter_info->applications[i], rule->u.application) == 0)
                    return TRUE;
                }
            }
          break;
	case FILTER_RULE_GROUP:
	  if (filter_info->groups)
            {
	      gint i;

	      for (i = 0; filter_info->groups[i] != NULL; i++)
		{
		  if (strcmp (filter_info->groups[i], rule->u.group) == 0)
		    return TRUE;
		}
	    }
	  break;
	case FILTER_RULE_PIXBUF_FORMATS:
	  {
            GSList *list;
	    if (!filter_info->mime_type)
	      break;

	    for (list = rule->u.pixbuf_formats; list; list = list->next)
              {
                gint i;
		gchar **mime_types;

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
        case FILTER_RULE_URI:
          if ((filter_info->uri != NULL) &&
              _btk_fnmatch (rule->u.uri, filter_info->uri, FALSE))
            return TRUE;
          break;
        case FILTER_RULE_DISPLAY_NAME:
          if ((filter_info->display_name != NULL) &&
              _btk_fnmatch (rule->u.pattern, filter_info->display_name, FALSE))
            return TRUE;
          break;
        case FILTER_RULE_AGE:
          if ((filter_info->age != -1) &&
              (filter_info->age < rule->u.age))
            return TRUE;
          break;
        case FILTER_RULE_CUSTOM:
          if (rule->u.custom.func (filter_info, rule->u.custom.data))
            return TRUE;
          break;
        }
    }
  
  return FALSE;
}

#define __BTK_RECENT_FILTER_C__
#include "btkaliasdef.c"
