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

#include "config.h"
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "btkcontainer.h"
#include "btkbuildable.h"
#include "btkbuilderprivate.h"
#include "btkprivate.h"
#include "btkmain.h"
#include "btkmarshalers.h"
#include "btkwindow.h"
#include "btkintl.h"
#include "btktoolbar.h"
#include <bobject/bobjectnotifyqueue.c>
#include <bobject/gvaluecollector.h>
#include "btkalias.h"


enum {
  ADD,
  REMOVE,
  CHECK_RESIZE,
  SET_FOCUS_CHILD,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_BORDER_WIDTH,
  PROP_RESIZE_MODE,
  PROP_CHILD
};

#define PARAM_SPEC_PARAM_ID(pspec)              ((pspec)->param_id)
#define PARAM_SPEC_SET_PARAM_ID(pspec, id)      ((pspec)->param_id = (id))


/* --- prototypes --- */
static void     btk_container_base_class_init      (BtkContainerClass *klass);
static void     btk_container_base_class_finalize  (BtkContainerClass *klass);
static void     btk_container_class_init           (BtkContainerClass *klass);
static void     btk_container_init                 (BtkContainer      *container);
static void     btk_container_destroy              (BtkObject         *object);
static void     btk_container_set_property         (GObject         *object,
						    guint            prop_id,
						    const GValue    *value,
						    GParamSpec      *pspec);
static void     btk_container_get_property         (GObject         *object,
						    guint            prop_id,
						    GValue          *value,
						    GParamSpec      *pspec);
static void     btk_container_add_unimplemented    (BtkContainer      *container,
						    BtkWidget         *widget);
static void     btk_container_remove_unimplemented (BtkContainer      *container,
						    BtkWidget         *widget);
static void     btk_container_real_check_resize    (BtkContainer      *container);
static gboolean btk_container_focus                (BtkWidget         *widget,
						    BtkDirectionType   direction);
static void     btk_container_real_set_focus_child (BtkContainer      *container,
						    BtkWidget         *widget);

static gboolean btk_container_focus_move           (BtkContainer      *container,
						    GList             *children,
						    BtkDirectionType   direction);
static void     btk_container_children_callback    (BtkWidget         *widget,
						    gpointer           client_data);
static void     btk_container_show_all             (BtkWidget         *widget);
static void     btk_container_hide_all             (BtkWidget         *widget);
static gint     btk_container_expose               (BtkWidget         *widget,
						    BdkEventExpose    *event);
static void     btk_container_map                  (BtkWidget         *widget);
static void     btk_container_unmap                (BtkWidget         *widget);

static gchar* btk_container_child_default_composite_name (BtkContainer *container,
							  BtkWidget    *child);

/* BtkBuildable */
static void btk_container_buildable_init           (BtkBuildableIface *iface);
static void btk_container_buildable_add_child      (BtkBuildable *buildable,
						    BtkBuilder   *builder,
						    GObject      *child,
						    const gchar  *type);
static gboolean btk_container_buildable_custom_tag_start (BtkBuildable  *buildable,
							  BtkBuilder    *builder,
							  GObject       *child,
							  const gchar   *tagname,
							  GMarkupParser *parser,
							  gpointer      *data);
static void    btk_container_buildable_custom_tag_end (BtkBuildable *buildable,
						       BtkBuilder   *builder,
						       GObject      *child,
						       const gchar  *tagname,
						       gpointer     *data);


/* --- variables --- */
static const gchar           vadjustment_key[] = "btk-vadjustment";
static guint                 vadjustment_key_id = 0;
static const gchar           hadjustment_key[] = "btk-hadjustment";
static guint                 hadjustment_key_id = 0;
static GSList	            *container_resize_queue = NULL;
static guint                 container_signals[LAST_SIGNAL] = { 0 };
static BtkWidgetClass       *parent_class = NULL;
extern GParamSpecPool       *_btk_widget_child_property_pool;
extern GObjectNotifyContext *_btk_widget_child_property_notify_context;
static BtkBuildableIface    *parent_buildable_iface;


/* --- functions --- */
GType
btk_container_get_type (void)
{
  static GType container_type = 0;

  if (!container_type)
    {
      const GTypeInfo container_info =
      {
	sizeof (BtkContainerClass),
	(GBaseInitFunc) btk_container_base_class_init,
	(GBaseFinalizeFunc) btk_container_base_class_finalize,
	(GClassInitFunc) btk_container_class_init,
	NULL        /* class_finalize */,
	NULL        /* class_data */,
	sizeof (BtkContainer),
	0           /* n_preallocs */,
	(GInstanceInitFunc) btk_container_init,
	NULL,       /* value_table */
      };

      const GInterfaceInfo buildable_info =
      {
	(GInterfaceInitFunc) btk_container_buildable_init,
	NULL,
	NULL
      };

      container_type =
	g_type_register_static (BTK_TYPE_WIDGET, I_("BtkContainer"), 
				&container_info, G_TYPE_FLAG_ABSTRACT);

      g_type_add_interface_static (container_type,
				   BTK_TYPE_BUILDABLE,
				   &buildable_info);

    }

  return container_type;
}

static void
btk_container_base_class_init (BtkContainerClass *class)
{
  /* reset instance specifc class fields that don't get inherited */
  class->set_child_property = NULL;
  class->get_child_property = NULL;
}

static void
btk_container_base_class_finalize (BtkContainerClass *class)
{
  GList *list, *node;

  list = g_param_spec_pool_list_owned (_btk_widget_child_property_pool, G_OBJECT_CLASS_TYPE (class));
  for (node = list; node; node = node->next)
    {
      GParamSpec *pspec = node->data;

      g_param_spec_pool_remove (_btk_widget_child_property_pool, pspec);
      PARAM_SPEC_SET_PARAM_ID (pspec, 0);
      g_param_spec_unref (pspec);
    }
  g_list_free (list);
}

static void
btk_container_class_init (BtkContainerClass *class)
{
  GObjectClass *bobject_class = G_OBJECT_CLASS (class);
  BtkObjectClass *object_class = BTK_OBJECT_CLASS (class);
  BtkWidgetClass *widget_class = BTK_WIDGET_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  vadjustment_key_id = g_quark_from_static_string (vadjustment_key);
  hadjustment_key_id = g_quark_from_static_string (hadjustment_key);
  
  bobject_class->set_property = btk_container_set_property;
  bobject_class->get_property = btk_container_get_property;

  object_class->destroy = btk_container_destroy;

  widget_class->show_all = btk_container_show_all;
  widget_class->hide_all = btk_container_hide_all;
  widget_class->expose_event = btk_container_expose;
  widget_class->map = btk_container_map;
  widget_class->unmap = btk_container_unmap;
  widget_class->focus = btk_container_focus;
  
  class->add = btk_container_add_unimplemented;
  class->remove = btk_container_remove_unimplemented;
  class->check_resize = btk_container_real_check_resize;
  class->forall = NULL;
  class->set_focus_child = btk_container_real_set_focus_child;
  class->child_type = NULL;
  class->composite_name = btk_container_child_default_composite_name;

  g_object_class_install_property (bobject_class,
                                   PROP_RESIZE_MODE,
                                   g_param_spec_enum ("resize-mode",
                                                      P_("Resize mode"),
                                                      P_("Specify how resize events are handled"),
                                                      BTK_TYPE_RESIZE_MODE,
                                                      BTK_RESIZE_PARENT,
                                                      BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
                                   PROP_BORDER_WIDTH,
                                   g_param_spec_uint ("border-width",
                                                      P_("Border width"),
                                                      P_("The width of the empty border outside the containers children"),
						      0,
						      65535,
						      0,
                                                      BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
                                   PROP_CHILD,
                                   g_param_spec_object ("child",
                                                      P_("Child"),
                                                      P_("Can be used to add a new child to the container"),
                                                      BTK_TYPE_WIDGET,
						      BTK_PARAM_WRITABLE));
  container_signals[ADD] =
    g_signal_new (I_("add"),
		  G_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkContainerClass, add),
		  NULL, NULL,
		  _btk_marshal_VOID__OBJECT,
		  G_TYPE_NONE, 1,
		  BTK_TYPE_WIDGET);
  container_signals[REMOVE] =
    g_signal_new (I_("remove"),
		  G_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkContainerClass, remove),
		  NULL, NULL,
		  _btk_marshal_VOID__OBJECT,
		  G_TYPE_NONE, 1,
		  BTK_TYPE_WIDGET);
  container_signals[CHECK_RESIZE] =
    g_signal_new (I_("check-resize"),
		  G_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkContainerClass, check_resize),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  container_signals[SET_FOCUS_CHILD] =
    g_signal_new (I_("set-focus-child"),
		  G_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkContainerClass, set_focus_child),
		  NULL, NULL,
		  _btk_marshal_VOID__OBJECT,
		  G_TYPE_NONE, 1,
		  BTK_TYPE_WIDGET);
}

static void
btk_container_buildable_init (BtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->add_child = btk_container_buildable_add_child;
  iface->custom_tag_start = btk_container_buildable_custom_tag_start;
  iface->custom_tag_end = btk_container_buildable_custom_tag_end;
}

static void
btk_container_buildable_add_child (BtkBuildable  *buildable,
				   BtkBuilder    *builder,
				   GObject       *child,
				   const gchar   *type)
{
  if (type)
    {
      BTK_BUILDER_WARN_INVALID_CHILD_TYPE (buildable, type);
    }
  else if (BTK_IS_WIDGET (child) && BTK_WIDGET (child)->parent == NULL)
    {
      btk_container_add (BTK_CONTAINER (buildable), BTK_WIDGET (child));
    }
  else
    g_warning ("Cannot add an object of type %s to a container of type %s", 
	       g_type_name (G_OBJECT_TYPE (child)), g_type_name (G_OBJECT_TYPE (buildable)));
}

static void
btk_container_buildable_set_child_property (BtkContainer *container,
					    BtkBuilder   *builder,
					    BtkWidget    *child,
					    gchar        *name,
					    const gchar  *value)
{
  GParamSpec *pspec;
  GValue gvalue = { 0, };
  GError *error = NULL;
  
  pspec = btk_container_class_find_child_property
    (G_OBJECT_GET_CLASS (container), name);
  if (!pspec)
    {
      g_warning ("%s does not have a property called %s",
		 g_type_name (G_OBJECT_TYPE (container)), name);
      return;
    }

  if (!btk_builder_value_from_string (builder, pspec, value, &gvalue, &error))
    {
      g_warning ("Could not read property %s:%s with value %s of type %s: %s",
		 g_type_name (G_OBJECT_TYPE (container)),
		 name,
		 value,
		 g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)),
		 error->message);
      g_error_free (error);
      return;
    }

  btk_container_child_set_property (container, child, name, &gvalue);
  g_value_unset (&gvalue);
}

typedef struct {
  BtkBuilder   *builder;
  BtkContainer *container;
  BtkWidget    *child;
  gchar        *child_prop_name;
  gchar        *context;
  gboolean     translatable;
} PackingPropertiesData;

static void
attributes_start_element (GMarkupParseContext *context,
			  const gchar         *element_name,
			  const gchar        **names,
			  const gchar        **values,
			  gpointer             user_data,
			  GError             **error)
{
  PackingPropertiesData *parser_data = (PackingPropertiesData*)user_data;
  guint i;

  if (strcmp (element_name, "property") == 0)
    {
      for (i = 0; names[i]; i++)
	if (strcmp (names[i], "name") == 0)
	  parser_data->child_prop_name = g_strdup (values[i]);
	else if (strcmp (names[i], "translatable") == 0)
	  {
	    if (!_btk_builder_boolean_from_string (values[1],
						   &parser_data->translatable,
						   error))
	      return;
	  }
	else if (strcmp (names[i], "comments") == 0)
	  ; /* for translators */
	else if (strcmp (names[i], "context") == 0)
	  parser_data->context = g_strdup (values[1]);
	else
	  g_warning ("Unsupported attribute for BtkContainer Child "
		     "property: %s\n", names[i]);
    }
  else if (strcmp (element_name, "packing") == 0)
    return;
  else
    g_warning ("Unsupported tag for BtkContainer: %s\n", element_name);
}

static void
attributes_text_element (GMarkupParseContext *context,
			 const gchar         *text,
			 gsize                text_len,
			 gpointer             user_data,
			 GError             **error)
{
  PackingPropertiesData *parser_data = (PackingPropertiesData*)user_data;
  gchar* value;

  if (!parser_data->child_prop_name)
    return;
  
  if (parser_data->translatable && text_len)
    {
      const gchar* domain;
      domain = btk_builder_get_translation_domain (parser_data->builder);
      
      value = _btk_builder_parser_translate (domain,
					     parser_data->context,
					     text);
    }
  else
    {
      value = g_strdup (text);
    }

  btk_container_buildable_set_child_property (parser_data->container,
					      parser_data->builder,
					      parser_data->child,
					      parser_data->child_prop_name,
					      value);

  g_free (parser_data->child_prop_name);
  g_free (parser_data->context);
  g_free (value);
  parser_data->child_prop_name = NULL;
  parser_data->context = NULL;
  parser_data->translatable = FALSE;
}

static const GMarkupParser attributes_parser =
  {
    attributes_start_element,
    NULL,
    attributes_text_element,
  };

static gboolean
btk_container_buildable_custom_tag_start (BtkBuildable  *buildable,
					  BtkBuilder    *builder,
					  GObject       *child,
					  const gchar   *tagname,
					  GMarkupParser *parser,
					  gpointer      *data)
{
  PackingPropertiesData *parser_data;

  if (parent_buildable_iface->custom_tag_start (buildable, builder, child,
						tagname, parser, data))
    return TRUE;

  if (child && strcmp (tagname, "packing") == 0)
    {
      parser_data = g_slice_new0 (PackingPropertiesData);
      parser_data->builder = builder;
      parser_data->container = BTK_CONTAINER (buildable);
      parser_data->child = BTK_WIDGET (child);
      parser_data->child_prop_name = NULL;

      *parser = attributes_parser;
      *data = parser_data;
      return TRUE;
    }

  return FALSE;
}

static void
btk_container_buildable_custom_tag_end (BtkBuildable *buildable,
					BtkBuilder   *builder,
					GObject      *child,
					const gchar  *tagname,
					gpointer     *data)
{
  if (strcmp (tagname, "packing") == 0)
    {
      g_slice_free (PackingPropertiesData, (gpointer)data);
      return;

    }

  if (parent_buildable_iface->custom_tag_end)
    parent_buildable_iface->custom_tag_end (buildable, builder,
					    child, tagname, data);

}

/**
 * btk_container_child_type: 
 * @container: a #BtkContainer
 *
 * Returns the type of the children supported by the container.
 *
 * Note that this may return %G_TYPE_NONE to indicate that no more
 * children can be added, e.g. for a #BtkPaned which already has two 
 * children.
 *
 * Return value: a #GType.
 **/
GType
btk_container_child_type (BtkContainer *container)
{
  GType slot;
  BtkContainerClass *class;

  g_return_val_if_fail (BTK_IS_CONTAINER (container), 0);

  class = BTK_CONTAINER_GET_CLASS (container);
  if (class->child_type)
    slot = class->child_type (container);
  else
    slot = G_TYPE_NONE;

  return slot;
}

/* --- BtkContainer child property mechanism --- */
static inline void
container_get_child_property (BtkContainer *container,
			      BtkWidget    *child,
			      GParamSpec   *pspec,
			      GValue       *value)
{
  BtkContainerClass *class = g_type_class_peek (pspec->owner_type);
  
  class->get_child_property (container, child, PARAM_SPEC_PARAM_ID (pspec), value, pspec);
}

static inline void
container_set_child_property (BtkContainer       *container,
			      BtkWidget		 *child,
			      GParamSpec         *pspec,
			      const GValue       *value,
			      GObjectNotifyQueue *nqueue)
{
  GValue tmp_value = { 0, };
  BtkContainerClass *class = g_type_class_peek (pspec->owner_type);

  /* provide a copy to work from, convert (if necessary) and validate */
  g_value_init (&tmp_value, G_PARAM_SPEC_VALUE_TYPE (pspec));
  if (!g_value_transform (value, &tmp_value))
    g_warning ("unable to set child property `%s' of type `%s' from value of type `%s'",
	       pspec->name,
	       g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)),
	       G_VALUE_TYPE_NAME (value));
  else if (g_param_value_validate (pspec, &tmp_value) && !(pspec->flags & G_PARAM_LAX_VALIDATION))
    {
      gchar *contents = g_strdup_value_contents (value);

      g_warning ("value \"%s\" of type `%s' is invalid for property `%s' of type `%s'",
		 contents,
		 G_VALUE_TYPE_NAME (value),
		 pspec->name,
		 g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)));
      g_free (contents);
    }
  else
    {
      class->set_child_property (container, child, PARAM_SPEC_PARAM_ID (pspec), &tmp_value, pspec);
      g_object_notify_queue_add (G_OBJECT (child), nqueue, pspec);
    }
  g_value_unset (&tmp_value);
}

/**
 * btk_container_child_get_valist:
 * @container: a #BtkContainer
 * @child: a widget which is a child of @container
 * @first_property_name: the name of the first property to get
 * @var_args: return location for the first property, followed 
 *     optionally by more name/return location pairs, followed by %NULL
 * 
 * Gets the values of one or more child properties for @child and @container.
 **/
void
btk_container_child_get_valist (BtkContainer *container,
				BtkWidget    *child,
				const gchar  *first_property_name,
				va_list       var_args)
{
  const gchar *name;

  g_return_if_fail (BTK_IS_CONTAINER (container));
  g_return_if_fail (BTK_IS_WIDGET (child));
  g_return_if_fail (child->parent == BTK_WIDGET (container));

  g_object_ref (container);
  g_object_ref (child);

  name = first_property_name;
  while (name)
    {
      GValue value = { 0, };
      GParamSpec *pspec;
      gchar *error;

      pspec = g_param_spec_pool_lookup (_btk_widget_child_property_pool,
					name,
					G_OBJECT_TYPE (container),
					TRUE);
      if (!pspec)
	{
	  g_warning ("%s: container class `%s' has no child property named `%s'",
		     G_STRLOC,
		     G_OBJECT_TYPE_NAME (container),
		     name);
	  break;
	}
      if (!(pspec->flags & G_PARAM_READABLE))
	{
	  g_warning ("%s: child property `%s' of container class `%s' is not readable",
		     G_STRLOC,
		     pspec->name,
		     G_OBJECT_TYPE_NAME (container));
	  break;
	}
      g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      container_get_child_property (container, child, pspec, &value);
      G_VALUE_LCOPY (&value, var_args, 0, &error);
      if (error)
	{
	  g_warning ("%s: %s", G_STRLOC, error);
	  g_free (error);
	  g_value_unset (&value);
	  break;
	}
      g_value_unset (&value);
      name = va_arg (var_args, gchar*);
    }

  g_object_unref (child);
  g_object_unref (container);
}

/**
 * btk_container_child_get_property:
 * @container: a #BtkContainer
 * @child: a widget which is a child of @container
 * @property_name: the name of the property to get
 * @value: a location to return the value
 * 
 * Gets the value of a child property for @child and @container.
 **/
void
btk_container_child_get_property (BtkContainer *container,
				  BtkWidget    *child,
				  const gchar  *property_name,
				  GValue       *value)
{
  GParamSpec *pspec;

  g_return_if_fail (BTK_IS_CONTAINER (container));
  g_return_if_fail (BTK_IS_WIDGET (child));
  g_return_if_fail (child->parent == BTK_WIDGET (container));
  g_return_if_fail (property_name != NULL);
  g_return_if_fail (G_IS_VALUE (value));
  
  g_object_ref (container);
  g_object_ref (child);
  pspec = g_param_spec_pool_lookup (_btk_widget_child_property_pool, property_name,
				    G_OBJECT_TYPE (container), TRUE);
  if (!pspec)
    g_warning ("%s: container class `%s' has no child property named `%s'",
	       G_STRLOC,
	       G_OBJECT_TYPE_NAME (container),
	       property_name);
  else if (!(pspec->flags & G_PARAM_READABLE))
    g_warning ("%s: child property `%s' of container class `%s' is not readable",
	       G_STRLOC,
	       pspec->name,
	       G_OBJECT_TYPE_NAME (container));
  else
    {
      GValue *prop_value, tmp_value = { 0, };

      /* auto-conversion of the callers value type
       */
      if (G_VALUE_TYPE (value) == G_PARAM_SPEC_VALUE_TYPE (pspec))
	{
	  g_value_reset (value);
	  prop_value = value;
	}
      else if (!g_value_type_transformable (G_PARAM_SPEC_VALUE_TYPE (pspec), G_VALUE_TYPE (value)))
	{
	  g_warning ("can't retrieve child property `%s' of type `%s' as value of type `%s'",
		     pspec->name,
		     g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)),
		     G_VALUE_TYPE_NAME (value));
	  g_object_unref (child);
	  g_object_unref (container);
	  return;
	}
      else
	{
	  g_value_init (&tmp_value, G_PARAM_SPEC_VALUE_TYPE (pspec));
	  prop_value = &tmp_value;
	}
      container_get_child_property (container, child, pspec, prop_value);
      if (prop_value != value)
	{
	  g_value_transform (prop_value, value);
	  g_value_unset (&tmp_value);
	}
    }
  g_object_unref (child);
  g_object_unref (container);
}

/**
 * btk_container_child_set_valist:
 * @container: a #BtkContainer
 * @child: a widget which is a child of @container
 * @first_property_name: the name of the first property to set
 * @var_args: a %NULL-terminated list of property names and values, starting
 *           with @first_prop_name
 * 
 * Sets one or more child properties for @child and @container.
 **/
void
btk_container_child_set_valist (BtkContainer *container,
				BtkWidget    *child,
				const gchar  *first_property_name,
				va_list       var_args)
{
  GObjectNotifyQueue *nqueue;
  const gchar *name;

  g_return_if_fail (BTK_IS_CONTAINER (container));
  g_return_if_fail (BTK_IS_WIDGET (child));
  g_return_if_fail (child->parent == BTK_WIDGET (container));

  g_object_ref (container);
  g_object_ref (child);

  nqueue = g_object_notify_queue_freeze (G_OBJECT (child), _btk_widget_child_property_notify_context);
  name = first_property_name;
  while (name)
    {
      GValue value = { 0, };
      gchar *error = NULL;
      GParamSpec *pspec = g_param_spec_pool_lookup (_btk_widget_child_property_pool,
						    name,
						    G_OBJECT_TYPE (container),
						    TRUE);
      if (!pspec)
	{
	  g_warning ("%s: container class `%s' has no child property named `%s'",
		     G_STRLOC,
		     G_OBJECT_TYPE_NAME (container),
		     name);
	  break;
	}
      if (!(pspec->flags & G_PARAM_WRITABLE))
	{
	  g_warning ("%s: child property `%s' of container class `%s' is not writable",
		     G_STRLOC,
		     pspec->name,
		     G_OBJECT_TYPE_NAME (container));
	  break;
	}
      g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      G_VALUE_COLLECT (&value, var_args, 0, &error);
      if (error)
	{
	  g_warning ("%s: %s", G_STRLOC, error);
	  g_free (error);

	  /* we purposely leak the value here, it might not be
	   * in a sane state if an error condition occoured
	   */
	  break;
	}
      container_set_child_property (container, child, pspec, &value, nqueue);
      g_value_unset (&value);
      name = va_arg (var_args, gchar*);
    }
  g_object_notify_queue_thaw (G_OBJECT (child), nqueue);

  g_object_unref (container);
  g_object_unref (child);
}

/**
 * btk_container_child_set_property:
 * @container: a #BtkContainer
 * @child: a widget which is a child of @container
 * @property_name: the name of the property to set
 * @value: the value to set the property to
 * 
 * Sets a child property for @child and @container.
 **/
void
btk_container_child_set_property (BtkContainer *container,
				  BtkWidget    *child,
				  const gchar  *property_name,
				  const GValue *value)
{
  GObjectNotifyQueue *nqueue;
  GParamSpec *pspec;

  g_return_if_fail (BTK_IS_CONTAINER (container));
  g_return_if_fail (BTK_IS_WIDGET (child));
  g_return_if_fail (child->parent == BTK_WIDGET (container));
  g_return_if_fail (property_name != NULL);
  g_return_if_fail (G_IS_VALUE (value));
  
  g_object_ref (container);
  g_object_ref (child);

  nqueue = g_object_notify_queue_freeze (G_OBJECT (child), _btk_widget_child_property_notify_context);
  pspec = g_param_spec_pool_lookup (_btk_widget_child_property_pool, property_name,
				    G_OBJECT_TYPE (container), TRUE);
  if (!pspec)
    g_warning ("%s: container class `%s' has no child property named `%s'",
	       G_STRLOC,
	       G_OBJECT_TYPE_NAME (container),
	       property_name);
  else if (!(pspec->flags & G_PARAM_WRITABLE))
    g_warning ("%s: child property `%s' of container class `%s' is not writable",
	       G_STRLOC,
	       pspec->name,
	       G_OBJECT_TYPE_NAME (container));
  else
    {
      container_set_child_property (container, child, pspec, value, nqueue);
    }
  g_object_notify_queue_thaw (G_OBJECT (child), nqueue);
  g_object_unref (container);
  g_object_unref (child);
}

/**
 * btk_container_add_with_properties:
 * @container: a #BtkContainer 
 * @widget: a widget to be placed inside @container 
 * @first_prop_name: the name of the first child property to set 
 * @Varargs: a %NULL-terminated list of property names and values, starting
 *           with @first_prop_name
 * 
 * Adds @widget to @container, setting child properties at the same time.
 * See btk_container_add() and btk_container_child_set() for more details.
 **/
void
btk_container_add_with_properties (BtkContainer *container,
				   BtkWidget    *widget,
				   const gchar  *first_prop_name,
				   ...)
{
  g_return_if_fail (BTK_IS_CONTAINER (container));
  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (widget->parent == NULL);

  g_object_ref (container);
  g_object_ref (widget);
  btk_widget_freeze_child_notify (widget);

  g_signal_emit (container, container_signals[ADD], 0, widget);
  if (widget->parent)
    {
      va_list var_args;

      va_start (var_args, first_prop_name);
      btk_container_child_set_valist (container, widget, first_prop_name, var_args);
      va_end (var_args);
    }

  btk_widget_thaw_child_notify (widget);
  g_object_unref (widget);
  g_object_unref (container);
}

/**
 * btk_container_child_set:
 * @container: a #BtkContainer
 * @child: a widget which is a child of @container
 * @first_prop_name: the name of the first property to set
 * @Varargs: a %NULL-terminated list of property names and values, starting
 *           with @first_prop_name
 * 
 * Sets one or more child properties for @child and @container.
 **/
void
btk_container_child_set (BtkContainer      *container,
			 BtkWidget         *child,
			 const gchar       *first_prop_name,
			 ...)
{
  va_list var_args;
  
  g_return_if_fail (BTK_IS_CONTAINER (container));
  g_return_if_fail (BTK_IS_WIDGET (child));
  g_return_if_fail (child->parent == BTK_WIDGET (container));

  va_start (var_args, first_prop_name);
  btk_container_child_set_valist (container, child, first_prop_name, var_args);
  va_end (var_args);
}

/**
 * btk_container_child_get:
 * @container: a #BtkContainer
 * @child: a widget which is a child of @container
 * @first_prop_name: the name of the first property to get
 * @Varargs: return location for the first property, followed 
 *     optionally by more name/return location pairs, followed by %NULL
 * 
 * Gets the values of one or more child properties for @child and @container.
 **/
void
btk_container_child_get (BtkContainer      *container,
			 BtkWidget         *child,
			 const gchar       *first_prop_name,
			 ...)
{
  va_list var_args;
  
  g_return_if_fail (BTK_IS_CONTAINER (container));
  g_return_if_fail (BTK_IS_WIDGET (child));
  g_return_if_fail (child->parent == BTK_WIDGET (container));

  va_start (var_args, first_prop_name);
  btk_container_child_get_valist (container, child, first_prop_name, var_args);
  va_end (var_args);
}

/**
 * btk_container_class_install_child_property:
 * @cclass: a #BtkContainerClass
 * @property_id: the id for the property
 * @pspec: the #GParamSpec for the property
 * 
 * Installs a child property on a container class. 
 **/
void
btk_container_class_install_child_property (BtkContainerClass *cclass,
					    guint              property_id,
					    GParamSpec        *pspec)
{
  g_return_if_fail (BTK_IS_CONTAINER_CLASS (cclass));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  if (pspec->flags & G_PARAM_WRITABLE)
    g_return_if_fail (cclass->set_child_property != NULL);
  if (pspec->flags & G_PARAM_READABLE)
    g_return_if_fail (cclass->get_child_property != NULL);
  g_return_if_fail (property_id > 0);
  g_return_if_fail (PARAM_SPEC_PARAM_ID (pspec) == 0);  /* paranoid */
  if (pspec->flags & (G_PARAM_CONSTRUCT | G_PARAM_CONSTRUCT_ONLY))
    g_return_if_fail ((pspec->flags & (G_PARAM_CONSTRUCT | G_PARAM_CONSTRUCT_ONLY)) == 0);

  if (g_param_spec_pool_lookup (_btk_widget_child_property_pool, pspec->name, G_OBJECT_CLASS_TYPE (cclass), FALSE))
    {
      g_warning (G_STRLOC ": class `%s' already contains a child property named `%s'",
		 G_OBJECT_CLASS_NAME (cclass),
		 pspec->name);
      return;
    }
  g_param_spec_ref (pspec);
  g_param_spec_sink (pspec);
  PARAM_SPEC_SET_PARAM_ID (pspec, property_id);
  g_param_spec_pool_insert (_btk_widget_child_property_pool, pspec, G_OBJECT_CLASS_TYPE (cclass));
}

/**
 * btk_container_class_find_child_property:
 * @cclass: (type BtkContainerClass): a #BtkContainerClass
 * @property_name: the name of the child property to find
 * @returns: (transfer none): the #GParamSpec of the child property or
 *           %NULL if @class has no child property with that name.
 *
 * Finds a child property of a container class by name.
 */
GParamSpec*
btk_container_class_find_child_property (GObjectClass *cclass,
					 const gchar  *property_name)
{
  g_return_val_if_fail (BTK_IS_CONTAINER_CLASS (cclass), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);

  return g_param_spec_pool_lookup (_btk_widget_child_property_pool,
				   property_name,
				   G_OBJECT_CLASS_TYPE (cclass),
				   TRUE);
}

/**
 * btk_container_class_list_child_properties:
 * @cclass: (type BtkContainerClass): a #BtkContainerClass
 * @n_properties: location to return the number of child properties found
 * @returns: (array length=n_properties) (transfer container):  a newly 
 *           allocated %NULL-terminated array of #GParamSpec*. 
 *           The array must be freed with g_free().
 *
 * Returns all child properties of a container class.
 */
GParamSpec**
btk_container_class_list_child_properties (GObjectClass *cclass,
					   guint        *n_properties)
{
  GParamSpec **pspecs;
  guint n;

  g_return_val_if_fail (BTK_IS_CONTAINER_CLASS (cclass), NULL);

  pspecs = g_param_spec_pool_list (_btk_widget_child_property_pool,
				   G_OBJECT_CLASS_TYPE (cclass),
				   &n);
  if (n_properties)
    *n_properties = n;

  return pspecs;
}

static void
btk_container_add_unimplemented (BtkContainer     *container,
				 BtkWidget        *widget)
{
  g_warning ("BtkContainerClass::add not implemented for `%s'", g_type_name (G_TYPE_FROM_INSTANCE (container)));
}

static void
btk_container_remove_unimplemented (BtkContainer     *container,
				    BtkWidget        *widget)
{
  g_warning ("BtkContainerClass::remove not implemented for `%s'", g_type_name (G_TYPE_FROM_INSTANCE (container)));
}

static void
btk_container_init (BtkContainer *container)
{
  container->focus_child = NULL;
  container->border_width = 0;
  container->need_resize = FALSE;
  container->resize_mode = BTK_RESIZE_PARENT;
  container->reallocate_redraws = FALSE;
}

static void
btk_container_destroy (BtkObject *object)
{
  BtkContainer *container = BTK_CONTAINER (object);

  if (BTK_CONTAINER_RESIZE_PENDING (container))
    _btk_container_dequeue_resize_handler (container);

  if (container->focus_child)
    {
      g_object_unref (container->focus_child);
      container->focus_child = NULL;
    }

  /* do this before walking child widgets, to avoid
   * removing children from focus chain one by one.
   */
  if (container->has_focus_chain)
    btk_container_unset_focus_chain (container);

  btk_container_foreach (container, (BtkCallback) btk_widget_destroy, NULL);

  BTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
btk_container_set_property (GObject         *object,
			    guint            prop_id,
			    const GValue    *value,
			    GParamSpec      *pspec)
{
  BtkContainer *container = BTK_CONTAINER (object);

  switch (prop_id)
    {
    case PROP_BORDER_WIDTH:
      btk_container_set_border_width (container, g_value_get_uint (value));
      break;
    case PROP_RESIZE_MODE:
      btk_container_set_resize_mode (container, g_value_get_enum (value));
      break;
    case PROP_CHILD:
      btk_container_add (container, BTK_WIDGET (g_value_get_object (value)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_container_get_property (GObject         *object,
			    guint            prop_id,
			    GValue          *value,
			    GParamSpec      *pspec)
{
  BtkContainer *container = BTK_CONTAINER (object);
  
  switch (prop_id)
    {
    case PROP_BORDER_WIDTH:
      g_value_set_uint (value, container->border_width);
      break;
    case PROP_RESIZE_MODE:
      g_value_set_enum (value, container->resize_mode);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/**
 * btk_container_set_border_width:
 * @container: a #BtkContainer
 * @border_width: amount of blank space to leave <emphasis>outside</emphasis> 
 *   the container. Valid values are in the range 0-65535 pixels.
 *
 * Sets the border width of the container.
 *
 * The border width of a container is the amount of space to leave
 * around the outside of the container. The only exception to this is
 * #BtkWindow; because toplevel windows can't leave space outside,
 * they leave the space inside. The border is added on all sides of
 * the container. To add space to only one side, one approach is to
 * create a #BtkAlignment widget, call btk_widget_set_size_request()
 * to give it a size, and place it on the side of the container as
 * a spacer.
 **/
void
btk_container_set_border_width (BtkContainer *container,
				guint         border_width)
{
  g_return_if_fail (BTK_IS_CONTAINER (container));

  if (container->border_width != border_width)
    {
      container->border_width = border_width;
      g_object_notify (G_OBJECT (container), "border-width");
      
      if (btk_widget_get_realized (BTK_WIDGET (container)))
	btk_widget_queue_resize (BTK_WIDGET (container));
    }
}

/**
 * btk_container_get_border_width:
 * @container: a #BtkContainer
 * 
 * Retrieves the border width of the container. See
 * btk_container_set_border_width().
 *
 * Return value: the current border width
 **/
guint
btk_container_get_border_width (BtkContainer *container)
{
  g_return_val_if_fail (BTK_IS_CONTAINER (container), 0);

  return container->border_width;
}

/**
 * btk_container_add:
 * @container: a #BtkContainer
 * @widget: a widget to be placed inside @container
 * 
 * Adds @widget to @container. Typically used for simple containers
 * such as #BtkWindow, #BtkFrame, or #BtkButton; for more complicated
 * layout containers such as #BtkBox or #BtkTable, this function will
 * pick default packing parameters that may not be correct.  So
 * consider functions such as btk_box_pack_start() and
 * btk_table_attach() as an alternative to btk_container_add() in
 * those cases. A widget may be added to only one container at a time;
 * you can't place the same widget inside two different containers.
 **/
void
btk_container_add (BtkContainer *container,
		   BtkWidget    *widget)
{
  g_return_if_fail (BTK_IS_CONTAINER (container));
  g_return_if_fail (BTK_IS_WIDGET (widget));

  if (widget->parent != NULL)
    {
      g_warning ("Attempting to add a widget with type %s to a container of "
                 "type %s, but the widget is already inside a container of type %s, "
                 "the BTK+ FAQ at http://library.bunny.org/devel/btk-faq/stable/ "
                 "explains how to reparent a widget.",
                 g_type_name (G_OBJECT_TYPE (widget)),
                 g_type_name (G_OBJECT_TYPE (container)),
                 g_type_name (G_OBJECT_TYPE (widget->parent)));
      return;
    }

  g_signal_emit (container, container_signals[ADD], 0, widget);
}

/**
 * btk_container_remove:
 * @container: a #BtkContainer
 * @widget: a current child of @container
 * 
 * Removes @widget from @container. @widget must be inside @container.
 * Note that @container will own a reference to @widget, and that this
 * may be the last reference held; so removing a widget from its
 * container can destroy that widget. If you want to use @widget
 * again, you need to add a reference to it while it's not inside
 * a container, using g_object_ref(). If you don't want to use @widget
 * again it's usually more efficient to simply destroy it directly
 * using btk_widget_destroy() since this will remove it from the
 * container and help break any circular reference count cycles.
 **/
void
btk_container_remove (BtkContainer *container,
		      BtkWidget    *widget)
{
  g_return_if_fail (BTK_IS_CONTAINER (container));
  g_return_if_fail (BTK_IS_WIDGET (widget));

  /* When using the deprecated API of the toolbar, it is possible
   * to legitimately call this function with a widget that is not
   * a direct child of the container.
   */
  g_return_if_fail (BTK_IS_TOOLBAR (container) ||
		    widget->parent == BTK_WIDGET (container));
  
  g_signal_emit (container, container_signals[REMOVE], 0, widget);
}

void
_btk_container_dequeue_resize_handler (BtkContainer *container)
{
  g_return_if_fail (BTK_IS_CONTAINER (container));
  g_return_if_fail (BTK_CONTAINER_RESIZE_PENDING (container));

  container_resize_queue = g_slist_remove (container_resize_queue, container);
  BTK_PRIVATE_UNSET_FLAG (container, BTK_RESIZE_PENDING);
}

/**
 * btk_container_set_resize_mode:
 * @container: a #BtkContainer
 * @resize_mode: the new resize mode
 * 
 * Sets the resize mode for the container.
 *
 * The resize mode of a container determines whether a resize request 
 * will be passed to the container's parent, queued for later execution
 * or executed immediately.
 **/
void
btk_container_set_resize_mode (BtkContainer  *container,
			       BtkResizeMode  resize_mode)
{
  g_return_if_fail (BTK_IS_CONTAINER (container));
  g_return_if_fail (resize_mode <= BTK_RESIZE_IMMEDIATE);
  
  if (btk_widget_is_toplevel (BTK_WIDGET (container)) &&
      resize_mode == BTK_RESIZE_PARENT)
    {
      resize_mode = BTK_RESIZE_QUEUE;
    }
  
  if (container->resize_mode != resize_mode)
    {
      container->resize_mode = resize_mode;
      
      btk_widget_queue_resize (BTK_WIDGET (container));
      g_object_notify (G_OBJECT (container), "resize-mode");
    }
}

/**
 * btk_container_get_resize_mode:
 * @container: a #BtkContainer
 * 
 * Returns the resize mode for the container. See
 * btk_container_set_resize_mode ().
 *
 * Return value: the current resize mode
 **/
BtkResizeMode
btk_container_get_resize_mode (BtkContainer *container)
{
  g_return_val_if_fail (BTK_IS_CONTAINER (container), BTK_RESIZE_PARENT);

  return container->resize_mode;
}

/**
 * btk_container_set_reallocate_redraws:
 * @container: a #BtkContainer
 * @needs_redraws: the new value for the container's @reallocate_redraws flag
 *
 * Sets the @reallocate_redraws flag of the container to the given value.
 * 
 * Containers requesting reallocation redraws get automatically
 * redrawn if any of their children changed allocation. 
 **/ 
void
btk_container_set_reallocate_redraws (BtkContainer *container,
				      gboolean      needs_redraws)
{
  g_return_if_fail (BTK_IS_CONTAINER (container));

  container->reallocate_redraws = needs_redraws ? TRUE : FALSE;
}

static BtkContainer*
btk_container_get_resize_container (BtkContainer *container)
{
  BtkWidget *widget = BTK_WIDGET (container);

  while (widget->parent)
    {
      widget = widget->parent;
      if (BTK_IS_RESIZE_CONTAINER (widget))
	break;
    }

  return BTK_IS_RESIZE_CONTAINER (widget) ? (BtkContainer*) widget : NULL;
}

static gboolean
btk_container_idle_sizer (gpointer data)
{
  /* we may be invoked with a container_resize_queue of NULL, because
   * queue_resize could have been adding an extra idle function while
   * the queue still got processed. we better just ignore such case
   * than trying to explicitely work around them with some extra flags,
   * since it doesn't cause any actual harm.
   */
  while (container_resize_queue)
    {
      GSList *slist;
      BtkWidget *widget;

      slist = container_resize_queue;
      container_resize_queue = slist->next;
      widget = slist->data;
      g_slist_free_1 (slist);

      BTK_PRIVATE_UNSET_FLAG (widget, BTK_RESIZE_PENDING);
      btk_container_check_resize (BTK_CONTAINER (widget));
    }

  bdk_window_process_all_updates ();

  return FALSE;
}

void
_btk_container_queue_resize (BtkContainer *container)
{
  BtkContainer *resize_container;
  BtkWidget *widget;
  
  g_return_if_fail (BTK_IS_CONTAINER (container));

  widget = BTK_WIDGET (container);
  resize_container = btk_container_get_resize_container (container);
  
  while (TRUE)
    {
      BTK_PRIVATE_SET_FLAG (widget, BTK_ALLOC_NEEDED);
      BTK_PRIVATE_SET_FLAG (widget, BTK_REQUEST_NEEDED);
      if ((resize_container && widget == BTK_WIDGET (resize_container)) ||
	  !widget->parent)
	break;
      
      widget = widget->parent;
    }
      
  if (resize_container)
    {
      if (btk_widget_get_visible (BTK_WIDGET (resize_container)) &&
          (btk_widget_is_toplevel (BTK_WIDGET (resize_container)) ||
           btk_widget_get_realized (BTK_WIDGET (resize_container))))
	{
	  switch (resize_container->resize_mode)
	    {
	    case BTK_RESIZE_QUEUE:
	      if (!BTK_CONTAINER_RESIZE_PENDING (resize_container))
		{
		  BTK_PRIVATE_SET_FLAG (resize_container, BTK_RESIZE_PENDING);
		  if (container_resize_queue == NULL)
		    bdk_threads_add_idle_full (BTK_PRIORITY_RESIZE,
				     btk_container_idle_sizer,
				     NULL, NULL);
		  container_resize_queue = g_slist_prepend (container_resize_queue, resize_container);
		}
	      break;

	    case BTK_RESIZE_IMMEDIATE:
	      btk_container_check_resize (resize_container);
	      break;

	    case BTK_RESIZE_PARENT:
	      g_assert_not_reached ();
	      break;
	    }
	}
      else
	{
	  /* we need to let hidden resize containers know that something
	   * changed while they where hidden (currently only evaluated by
	   * toplevels).
	   */
	  resize_container->need_resize = TRUE;
	}
    }
}

void
btk_container_check_resize (BtkContainer *container)
{
  g_return_if_fail (BTK_IS_CONTAINER (container));
  
  g_signal_emit (container, container_signals[CHECK_RESIZE], 0);
}

static void
btk_container_real_check_resize (BtkContainer *container)
{
  BtkWidget *widget = BTK_WIDGET (container);
  BtkRequisition requisition;
  
  btk_widget_size_request (widget, &requisition);
  
  if (requisition.width > widget->allocation.width ||
      requisition.height > widget->allocation.height)
    {
      if (BTK_IS_RESIZE_CONTAINER (container))
	btk_widget_size_allocate (BTK_WIDGET (container),
				  &BTK_WIDGET (container)->allocation);
      else
	btk_widget_queue_resize (widget);
    }
  else
    {
      btk_container_resize_children (container);
    }
}

/* The container hasn't changed size but one of its children
 *  queued a resize request. Which means that the allocation
 *  is not sufficient for the requisition of some child.
 *  We've already performed a size request at this point,
 *  so we simply need to reallocate and let the allocation
 *  trickle down via BTK_WIDGET_ALLOC_NEEDED flags. 
 */
void
btk_container_resize_children (BtkContainer *container)
{
  BtkWidget *widget;
  
  /* resizing invariants:
   * toplevels have *always* resize_mode != BTK_RESIZE_PARENT set.
   * containers that have an idle sizer pending must be flagged with
   * RESIZE_PENDING.
   */
  g_return_if_fail (BTK_IS_CONTAINER (container));

  widget = BTK_WIDGET (container);
  btk_widget_size_allocate (widget, &widget->allocation);
}

/**
 * btk_container_forall:
 * @container: a #BtkContainer
 * @callback: a callback
 * @callback_data: callback user data
 * 
 * Invokes @callback on each child of @container, including children
 * that are considered "internal" (implementation details of the
 * container). "Internal" children generally weren't added by the user
 * of the container, but were added by the container implementation
 * itself.  Most applications should use btk_container_foreach(),
 * rather than btk_container_forall().
 **/
void
btk_container_forall (BtkContainer *container,
		      BtkCallback   callback,
		      gpointer      callback_data)
{
  BtkContainerClass *class;

  g_return_if_fail (BTK_IS_CONTAINER (container));
  g_return_if_fail (callback != NULL);

  class = BTK_CONTAINER_GET_CLASS (container);

  if (class->forall)
    class->forall (container, TRUE, callback, callback_data);
}

/**
 * btk_container_foreach:
 * @container: a #BtkContainer
 * @callback: (scope call):  a callback
 * @callback_data: callback user data
 * 
 * Invokes @callback on each non-internal child of @container. See
 * btk_container_forall() for details on what constitutes an
 * "internal" child.  Most applications should use
 * btk_container_foreach(), rather than btk_container_forall().
 **/
void
btk_container_foreach (BtkContainer *container,
		       BtkCallback   callback,
		       gpointer      callback_data)
{
  BtkContainerClass *class;
  
  g_return_if_fail (BTK_IS_CONTAINER (container));
  g_return_if_fail (callback != NULL);

  class = BTK_CONTAINER_GET_CLASS (container);

  if (class->forall)
    class->forall (container, FALSE, callback, callback_data);
}

typedef struct _BtkForeachData	BtkForeachData;
struct _BtkForeachData
{
  BtkObject         *container;
  BtkCallbackMarshal callback;
  gpointer           callback_data;
};

static void
btk_container_foreach_unmarshal (BtkWidget *child,
				 gpointer data)
{
  BtkForeachData *fdata = (BtkForeachData*) data;
  BtkArg args[2];
  
  /* first argument */
  args[0].name = NULL;
  args[0].type = G_TYPE_FROM_INSTANCE (child);
  BTK_VALUE_OBJECT (args[0]) = BTK_OBJECT (child);
  
  /* location for return value */
  args[1].name = NULL;
  args[1].type = G_TYPE_NONE;
  
  fdata->callback (fdata->container, fdata->callback_data, 1, args);
}

void
btk_container_foreach_full (BtkContainer       *container,
			    BtkCallback         callback,
			    BtkCallbackMarshal  marshal,
			    gpointer            callback_data,
			    GDestroyNotify      notify)
{
  g_return_if_fail (BTK_IS_CONTAINER (container));

  if (marshal)
    {
      BtkForeachData fdata;
  
      fdata.container     = BTK_OBJECT (container);
      fdata.callback      = marshal;
      fdata.callback_data = callback_data;

      btk_container_foreach (container, btk_container_foreach_unmarshal, &fdata);
    }
  else
    {
      g_return_if_fail (callback != NULL);

      btk_container_foreach (container, callback, &callback_data);
    }

  if (notify)
    notify (callback_data);
}

/**
 * btk_container_set_focus_child:
 * @container: a #BtkContainer
 * @child: (allow-none): a #BtkWidget, or %NULL
 *
 * Sets, or unsets if @child is %NULL, the focused child of @container.
 *
 * This function emits the BtkContainer::set_focus_child signal of
 * @container. Implementations of #BtkContainer can override the
 * default behaviour by overriding the class closure of this signal.
 *
 * This is function is mostly meant to be used by widgets. Applications can use
 * btk_widget_grab_focus() to manualy set the focus to a specific widget.
 */
void
btk_container_set_focus_child (BtkContainer *container,
			       BtkWidget    *child)
{
  g_return_if_fail (BTK_IS_CONTAINER (container));
  if (child)
    g_return_if_fail (BTK_IS_WIDGET (child));

  g_signal_emit (container, container_signals[SET_FOCUS_CHILD], 0, child);
}

/**
 * btk_container_get_focus_child:
 * @container: a #BtkContainer
 *
 * Returns the current focus child widget inside @container. This is not the
 * currently focused widget. That can be obtained by calling
 * btk_window_get_focus().
 *
 * Returns: (transfer none): The child widget which will receive the
 *          focus inside @container when the @conatiner is focussed,
 *          or %NULL if none is set.
 *
 * Since: 2.14
 **/
BtkWidget *
btk_container_get_focus_child (BtkContainer *container)
{
  g_return_val_if_fail (BTK_IS_CONTAINER (container), NULL);

  return container->focus_child;
}

/**
 * btk_container_get_children:
 * @container: a #BtkContainer
 * 
 * Returns the container's non-internal children. See
 * btk_container_forall() for details on what constitutes an "internal" child.
 *
 * Return value: (element-type BtkWidget) (transfer container): a newly-allocated list of the container's non-internal children.
 **/
GList*
btk_container_get_children (BtkContainer *container)
{
  GList *children = NULL;

  btk_container_foreach (container,
			 btk_container_children_callback,
			 &children);

  return g_list_reverse (children);
}

static void
btk_container_child_position_callback (BtkWidget *widget,
				       gpointer   client_data)
{
  struct {
    BtkWidget *child;
    guint i;
    guint index;
  } *data = client_data;

  data->i++;
  if (data->child == widget)
    data->index = data->i;
}

static gchar*
btk_container_child_default_composite_name (BtkContainer *container,
					    BtkWidget    *child)
{
  struct {
    BtkWidget *child;
    guint i;
    guint index;
  } data;
  gchar *name;

  /* fallback implementation */
  data.child = child;
  data.i = 0;
  data.index = 0;
  btk_container_forall (container,
			btk_container_child_position_callback,
			&data);
  
  name = g_strdup_printf ("%s-%u",
			  g_type_name (G_TYPE_FROM_INSTANCE (child)),
			  data.index);

  return name;
}

gchar*
_btk_container_child_composite_name (BtkContainer *container,
				    BtkWidget    *child)
{
  gboolean composite_child;

  g_return_val_if_fail (BTK_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (BTK_IS_WIDGET (child), NULL);
  g_return_val_if_fail (child->parent == BTK_WIDGET (container), NULL);

  g_object_get (child, "composite-child", &composite_child, NULL);
  if (composite_child)
    {
      static GQuark quark_composite_name = 0;
      gchar *name;

      if (!quark_composite_name)
	quark_composite_name = g_quark_from_static_string ("btk-composite-name");

      name = g_object_get_qdata (G_OBJECT (child), quark_composite_name);
      if (!name)
	{
	  BtkContainerClass *class;

	  class = BTK_CONTAINER_GET_CLASS (container);
	  if (class->composite_name)
	    name = class->composite_name (container, child);
	}
      else
	name = g_strdup (name);

      return name;
    }
  
  return NULL;
}

static void
btk_container_real_set_focus_child (BtkContainer     *container,
				    BtkWidget        *child)
{
  g_return_if_fail (BTK_IS_CONTAINER (container));
  g_return_if_fail (child == NULL || BTK_IS_WIDGET (child));

  if (child != container->focus_child)
    {
      if (container->focus_child)
	g_object_unref (container->focus_child);

      container->focus_child = child;

      if (container->focus_child)
	g_object_ref (container->focus_child);
    }

  /* Check for h/v adjustments and scroll to show the focus child if possible */
  if (container->focus_child)
    {
      BtkAdjustment *hadj;
      BtkAdjustment *vadj;
      BtkWidget *focus_child;
      gint x, y;

      hadj = g_object_get_qdata (G_OBJECT (container), hadjustment_key_id);   
      vadj = g_object_get_qdata (G_OBJECT (container), vadjustment_key_id);
      if (hadj || vadj) 
	{
	  focus_child = container->focus_child;
	  while (BTK_IS_CONTAINER (focus_child) && 
		 BTK_CONTAINER (focus_child)->focus_child)
	    {
	      focus_child = BTK_CONTAINER (focus_child)->focus_child;
	    }
	  
           if (!btk_widget_translate_coordinates (focus_child, container->focus_child,
                                                  0, 0, &x, &y))
             return;

	   x += container->focus_child->allocation.x;
	   y += container->focus_child->allocation.y;
	  
	  if (vadj)
	    btk_adjustment_clamp_page (vadj, y, y + focus_child->allocation.height);
	  
	  if (hadj)
	    btk_adjustment_clamp_page (hadj, x, x + focus_child->allocation.width);
	}
    }
}

static GList*
get_focus_chain (BtkContainer *container)
{
  return g_object_get_data (G_OBJECT (container), "btk-container-focus-chain");
}

/* same as btk_container_get_children, except it includes internals
 */
static GList *
btk_container_get_all_children (BtkContainer *container)
{
  GList *children = NULL;

  btk_container_forall (container,
			 btk_container_children_callback,
			 &children);

  return children;
}

static gboolean
btk_container_focus (BtkWidget        *widget,
                     BtkDirectionType  direction)
{
  GList *children;
  GList *sorted_children;
  gint return_val;
  BtkContainer *container;

  g_return_val_if_fail (BTK_IS_CONTAINER (widget), FALSE);

  container = BTK_CONTAINER (widget);

  return_val = FALSE;

  if (btk_widget_get_can_focus (widget))
    {
      if (!btk_widget_has_focus (widget))
	{
	  btk_widget_grab_focus (widget);
	  return_val = TRUE;
	}
    }
  else
    {
      /* Get a list of the containers children, allowing focus
       * chain to override.
       */
      if (container->has_focus_chain)
	children = g_list_copy (get_focus_chain (container));
      else
	children = btk_container_get_all_children (container);

      if (container->has_focus_chain &&
	  (direction == BTK_DIR_TAB_FORWARD ||
	   direction == BTK_DIR_TAB_BACKWARD))
	{
	  sorted_children = g_list_copy (children);
	  
	  if (direction == BTK_DIR_TAB_BACKWARD)
	    sorted_children = g_list_reverse (sorted_children);
	}
      else
	sorted_children = _btk_container_focus_sort (container, children, direction, NULL);
      
      return_val = btk_container_focus_move (container, sorted_children, direction);

      g_list_free (sorted_children);
      g_list_free (children);
    }

  return return_val;
}

static gint
tab_compare (gconstpointer a,
	     gconstpointer b,
	     gpointer      data)
{
  const BtkWidget *child1 = a;
  const BtkWidget *child2 = b;
  BtkTextDirection text_direction = GPOINTER_TO_INT (data);

  gint y1 = child1->allocation.y + child1->allocation.height / 2;
  gint y2 = child2->allocation.y + child2->allocation.height / 2;

  if (y1 == y2)
    {
      gint x1 = child1->allocation.x + child1->allocation.width / 2;
      gint x2 = child2->allocation.x + child2->allocation.width / 2;
      
      if (text_direction == BTK_TEXT_DIR_RTL) 
	return (x1 < x2) ? 1 : ((x1 == x2) ? 0 : -1);
      else
	return (x1 < x2) ? -1 : ((x1 == x2) ? 0 : 1);
    }
  else
    return (y1 < y2) ? -1 : 1;
}

static GList *
btk_container_focus_sort_tab (BtkContainer     *container,
			      GList            *children,
			      BtkDirectionType  direction,
			      BtkWidget        *old_focus)
{
  BtkTextDirection text_direction = btk_widget_get_direction (BTK_WIDGET (container));
  children = g_list_sort_with_data (children, tab_compare, GINT_TO_POINTER (text_direction));

  /* if we are going backwards then reverse the order
   *  of the children.
   */
  if (direction == BTK_DIR_TAB_BACKWARD)
    children = g_list_reverse (children);

  return children;
}

/* Get coordinates of @widget's allocation with respect to
 * allocation of @container.
 */
static gboolean
get_allocation_coords (BtkContainer  *container,
		       BtkWidget     *widget,
		       BdkRectangle  *allocation)
{
  *allocation = widget->allocation;

  return btk_widget_translate_coordinates (widget, BTK_WIDGET (container),
					   0, 0, &allocation->x, &allocation->y);
}

/* Look for a child in @children that is intermediate between
 * the focus widget and container. This widget, if it exists,
 * acts as the starting widget for focus navigation.
 */
static BtkWidget *
find_old_focus (BtkContainer *container,
		GList        *children)
{
  GList *tmp_list = children;
  while (tmp_list)
    {
      BtkWidget *child = tmp_list->data;
      BtkWidget *widget = child;

      while (widget && widget != (BtkWidget *)container)
	{
	  BtkWidget *parent = widget->parent;
	  if (parent && ((BtkContainer *)parent)->focus_child != widget)
	    goto next;

	  widget = parent;
	}

      return child;

    next:
      tmp_list = tmp_list->next;
    }

  return NULL;
}

static gboolean
old_focus_coords (BtkContainer *container,
		  BdkRectangle *old_focus_rect)
{
  BtkWidget *widget = BTK_WIDGET (container);
  BtkWidget *toplevel = btk_widget_get_toplevel (widget);

  if (BTK_IS_WINDOW (toplevel) && BTK_WINDOW (toplevel)->focus_widget)
    {
      BtkWidget *old_focus = BTK_WINDOW (toplevel)->focus_widget;
      
      return get_allocation_coords (container, old_focus, old_focus_rect);
    }
  else
    return FALSE;
}

typedef struct _CompareInfo CompareInfo;

struct _CompareInfo
{
  BtkContainer *container;
  gint x;
  gint y;
  gboolean reverse;
};

static gint
up_down_compare (gconstpointer a,
		 gconstpointer b,
		 gpointer      data)
{
  BdkRectangle allocation1;
  BdkRectangle allocation2;
  CompareInfo *compare = data;
  gint y1, y2;

  get_allocation_coords (compare->container, (BtkWidget *)a, &allocation1);
  get_allocation_coords (compare->container, (BtkWidget *)b, &allocation2);

  y1 = allocation1.y + allocation1.height / 2;
  y2 = allocation2.y + allocation2.height / 2;

  if (y1 == y2)
    {
      gint x1 = abs (allocation1.x + allocation1.width / 2 - compare->x);
      gint x2 = abs (allocation2.x + allocation2.width / 2 - compare->x);

      if (compare->reverse)
	return (x1 < x2) ? 1 : ((x1 == x2) ? 0 : -1);
      else
	return (x1 < x2) ? -1 : ((x1 == x2) ? 0 : 1);
    }
  else
    return (y1 < y2) ? -1 : 1;
}

static GList *
btk_container_focus_sort_up_down (BtkContainer     *container,
				  GList            *children,
				  BtkDirectionType  direction,
				  BtkWidget        *old_focus)
{
  CompareInfo compare;
  GList *tmp_list;
  BdkRectangle old_allocation;

  compare.container = container;
  compare.reverse = (direction == BTK_DIR_UP);

  if (!old_focus)
      old_focus = find_old_focus (container, children);
  
  if (old_focus && get_allocation_coords (container, old_focus, &old_allocation))
    {
      gint compare_x1;
      gint compare_x2;
      gint compare_y;

      /* Delete widgets from list that don't match minimum criteria */

      compare_x1 = old_allocation.x;
      compare_x2 = old_allocation.x + old_allocation.width;

      if (direction == BTK_DIR_UP)
	compare_y = old_allocation.y;
      else
	compare_y = old_allocation.y + old_allocation.height;
      
      tmp_list = children;
      while (tmp_list)
	{
	  BtkWidget *child = tmp_list->data;
	  GList *next = tmp_list->next;
	  gint child_x1, child_x2;
	  BdkRectangle child_allocation;
	  
	  if (child != old_focus)
	    {
	      if (get_allocation_coords (container, child, &child_allocation))
		{
		  child_x1 = child_allocation.x;
		  child_x2 = child_allocation.x + child_allocation.width;
		  
		  if ((child_x2 <= compare_x1 || child_x1 >= compare_x2) /* No horizontal overlap */ ||
		      (direction == BTK_DIR_DOWN && child_allocation.y + child_allocation.height < compare_y) || /* Not below */
		      (direction == BTK_DIR_UP && child_allocation.y > compare_y)) /* Not above */
		    {
		      children = g_list_delete_link (children, tmp_list);
		    }
		}
	      else
		children = g_list_delete_link (children, tmp_list);
	    }
	  
	  tmp_list = next;
	}

      compare.x = (compare_x1 + compare_x2) / 2;
      compare.y = old_allocation.y + old_allocation.height / 2;
    }
  else
    {
      /* No old focus widget, need to figure out starting x,y some other way
       */
      BtkWidget *widget = BTK_WIDGET (container);
      BdkRectangle old_focus_rect;

      if (old_focus_coords (container, &old_focus_rect))
	{
	  compare.x = old_focus_rect.x + old_focus_rect.width / 2;
	}
      else
	{
	  if (!btk_widget_get_has_window (widget))
	    compare.x = widget->allocation.x + widget->allocation.width / 2;
	  else
	    compare.x = widget->allocation.width / 2;
	}
      
      if (!btk_widget_get_has_window (widget))
	compare.y = (direction == BTK_DIR_DOWN) ? widget->allocation.y : widget->allocation.y + widget->allocation.height;
      else
	compare.y = (direction == BTK_DIR_DOWN) ? 0 : + widget->allocation.height;
    }

  children = g_list_sort_with_data (children, up_down_compare, &compare);

  if (compare.reverse)
    children = g_list_reverse (children);

  return children;
}

static gint
left_right_compare (gconstpointer a,
		    gconstpointer b,
		    gpointer      data)
{
  BdkRectangle allocation1;
  BdkRectangle allocation2;
  CompareInfo *compare = data;
  gint x1, x2;

  get_allocation_coords (compare->container, (BtkWidget *)a, &allocation1);
  get_allocation_coords (compare->container, (BtkWidget *)b, &allocation2);

  x1 = allocation1.x + allocation1.width / 2;
  x2 = allocation2.x + allocation2.width / 2;

  if (x1 == x2)
    {
      gint y1 = abs (allocation1.y + allocation1.height / 2 - compare->y);
      gint y2 = abs (allocation2.y + allocation2.height / 2 - compare->y);

      if (compare->reverse)
	return (y1 < y2) ? 1 : ((y1 == y2) ? 0 : -1);
      else
	return (y1 < y2) ? -1 : ((y1 == y2) ? 0 : 1);
    }
  else
    return (x1 < x2) ? -1 : 1;
}

static GList *
btk_container_focus_sort_left_right (BtkContainer     *container,
				     GList            *children,
				     BtkDirectionType  direction,
				     BtkWidget        *old_focus)
{
  CompareInfo compare;
  GList *tmp_list;
  BdkRectangle old_allocation;

  compare.container = container;
  compare.reverse = (direction == BTK_DIR_LEFT);

  if (!old_focus)
    old_focus = find_old_focus (container, children);
  
  if (old_focus && get_allocation_coords (container, old_focus, &old_allocation))
    {
      gint compare_y1;
      gint compare_y2;
      gint compare_x;
      
      /* Delete widgets from list that don't match minimum criteria */

      compare_y1 = old_allocation.y;
      compare_y2 = old_allocation.y + old_allocation.height;

      if (direction == BTK_DIR_LEFT)
	compare_x = old_allocation.x;
      else
	compare_x = old_allocation.x + old_allocation.width;
      
      tmp_list = children;
      while (tmp_list)
	{
	  BtkWidget *child = tmp_list->data;
	  GList *next = tmp_list->next;
	  gint child_y1, child_y2;
	  BdkRectangle child_allocation;
	  
	  if (child != old_focus)
	    {
	      if (get_allocation_coords (container, child, &child_allocation))
		{
		  child_y1 = child_allocation.y;
		  child_y2 = child_allocation.y + child_allocation.height;
		  
		  if ((child_y2 <= compare_y1 || child_y1 >= compare_y2) /* No vertical overlap */ ||
		      (direction == BTK_DIR_RIGHT && child_allocation.x + child_allocation.width < compare_x) || /* Not to left */
		      (direction == BTK_DIR_LEFT && child_allocation.x > compare_x)) /* Not to right */
		    {
		      children = g_list_delete_link (children, tmp_list);
		    }
		}
	      else
		children = g_list_delete_link (children, tmp_list);
	    }
	  
	  tmp_list = next;
	}

      compare.y = (compare_y1 + compare_y2) / 2;
      compare.x = old_allocation.x + old_allocation.width / 2;
    }
  else
    {
      /* No old focus widget, need to figure out starting x,y some other way
       */
      BtkWidget *widget = BTK_WIDGET (container);
      BdkRectangle old_focus_rect;

      if (old_focus_coords (container, &old_focus_rect))
	{
	  compare.y = old_focus_rect.y + old_focus_rect.height / 2;
	}
      else
	{
	  if (!btk_widget_get_has_window (widget))
	    compare.y = widget->allocation.y + widget->allocation.height / 2;
	  else
	    compare.y = widget->allocation.height / 2;
	}
      
      if (!btk_widget_get_has_window (widget))
	compare.x = (direction == BTK_DIR_RIGHT) ? widget->allocation.x : widget->allocation.x + widget->allocation.width;
      else
	compare.x = (direction == BTK_DIR_RIGHT) ? 0 : widget->allocation.width;
    }

  children = g_list_sort_with_data (children, left_right_compare, &compare);

  if (compare.reverse)
    children = g_list_reverse (children);

  return children;
}

/**
 * btk_container_focus_sort:
 * @container: a #BtkContainer
 * @children:  a list of descendents of @container (they don't
 *             have to be direct children)
 * @direction: focus direction
 * @old_focus: (allow-none): widget to use for the starting position, or %NULL
 *             to determine this automatically.
 *             (Note, this argument isn't used for BTK_DIR_TAB_*,
 *              which is the only @direction we use currently,
 *              so perhaps this argument should be removed)
 * 
 * Sorts @children in the correct order for focusing with
 * direction type @direction.
 * 
 * Return value: a copy of @children, sorted in correct focusing order,
 *   with children that aren't suitable for focusing in this direction
 *   removed.
 **/
GList *
_btk_container_focus_sort (BtkContainer     *container,
			   GList            *children,
			   BtkDirectionType  direction,
			   BtkWidget        *old_focus)
{
  GList *visible_children = NULL;

  while (children)
    {
      if (btk_widget_get_realized (children->data))
	visible_children = g_list_prepend (visible_children, children->data);
      children = children->next;
    }
  
  switch (direction)
    {
    case BTK_DIR_TAB_FORWARD:
    case BTK_DIR_TAB_BACKWARD:
      return btk_container_focus_sort_tab (container, visible_children, direction, old_focus);
    case BTK_DIR_UP:
    case BTK_DIR_DOWN:
      return btk_container_focus_sort_up_down (container, visible_children, direction, old_focus);
    case BTK_DIR_LEFT:
    case BTK_DIR_RIGHT:
      return btk_container_focus_sort_left_right (container, visible_children, direction, old_focus);
    }

  g_assert_not_reached ();

  return NULL;
}

static gboolean
btk_container_focus_move (BtkContainer     *container,
			  GList            *children,
			  BtkDirectionType  direction)
{
  BtkWidget *focus_child;
  BtkWidget *child;

  focus_child = container->focus_child;

  while (children)
    {
      child = children->data;
      children = children->next;

      if (!child)
	continue;
      
      if (focus_child)
        {
          if (focus_child == child)
            {
              focus_child = NULL;

		if (btk_widget_child_focus (child, direction))
		  return TRUE;
            }
        }
      else if (btk_widget_is_drawable (child) &&
               btk_widget_is_ancestor (child, BTK_WIDGET (container)))
        {
          if (btk_widget_child_focus (child, direction))
            return TRUE;
        }
    }

  return FALSE;
}


static void
btk_container_children_callback (BtkWidget *widget,
				 gpointer   client_data)
{
  GList **children;

  children = (GList**) client_data;
  *children = g_list_prepend (*children, widget);
}

static void
chain_widget_destroyed (BtkWidget *widget,
                        gpointer   user_data)
{
  BtkContainer *container;
  GList *chain;
  
  container = BTK_CONTAINER (user_data);

  chain = g_object_get_data (G_OBJECT (container),
                             "btk-container-focus-chain");

  chain = g_list_remove (chain, widget);

  g_signal_handlers_disconnect_by_func (widget,
                                        chain_widget_destroyed,
                                        user_data);
  
  g_object_set_data (G_OBJECT (container),
                     I_("btk-container-focus-chain"),
                     chain);  
}

/**
 * btk_container_set_focus_chain:
 * @container: a #BtkContainer
 * @focusable_widgets: (transfer none) (element-type BtkWidget):
 *     the new focus chain
 *
 * Sets a focus chain, overriding the one computed automatically by BTK+.
 * 
 * In principle each widget in the chain should be a descendant of the 
 * container, but this is not enforced by this method, since it's allowed 
 * to set the focus chain before you pack the widgets, or have a widget 
 * in the chain that isn't always packed. The necessary checks are done 
 * when the focus chain is actually traversed.
 **/
void
btk_container_set_focus_chain (BtkContainer *container,
                               GList        *focusable_widgets)
{
  GList *chain;
  GList *tmp_list;
  
  g_return_if_fail (BTK_IS_CONTAINER (container));
  
  if (container->has_focus_chain)
    btk_container_unset_focus_chain (container);

  container->has_focus_chain = TRUE;
  
  chain = NULL;
  tmp_list = focusable_widgets;
  while (tmp_list != NULL)
    {
      g_return_if_fail (BTK_IS_WIDGET (tmp_list->data));
      
      /* In principle each widget in the chain should be a descendant
       * of the container, but we don't want to check that here, it's
       * expensive and also it's allowed to set the focus chain before
       * you pack the widgets, or have a widget in the chain that isn't
       * always packed. So we check for ancestor during actual traversal.
       */

      chain = g_list_prepend (chain, tmp_list->data);

      g_signal_connect (tmp_list->data,
                        "destroy",
                        G_CALLBACK (chain_widget_destroyed),
                        container);
      
      tmp_list = g_list_next (tmp_list);
    }

  chain = g_list_reverse (chain);
  
  g_object_set_data (G_OBJECT (container),
                     I_("btk-container-focus-chain"),
                     chain);
}

/**
 * btk_container_get_focus_chain:
 * @container:         a #BtkContainer
 * @focusable_widgets: (element-type BtkWidget) (out) (transfer container): location
 *                     to store the focus chain of the
 *                     container, or %NULL. You should free this list
 *                     using g_list_free() when you are done with it, however
 *                     no additional reference count is added to the
 *                     individual widgets in the focus chain.
 * 
 * Retrieves the focus chain of the container, if one has been
 * set explicitly. If no focus chain has been explicitly
 * set, BTK+ computes the focus chain based on the positions
 * of the children. In that case, BTK+ stores %NULL in
 * @focusable_widgets and returns %FALSE.
 *
 * Return value: %TRUE if the focus chain of the container 
 * has been set explicitly.
 **/
gboolean
btk_container_get_focus_chain (BtkContainer *container,
			       GList       **focus_chain)
{
  g_return_val_if_fail (BTK_IS_CONTAINER (container), FALSE);

  if (focus_chain)
    {
      if (container->has_focus_chain)
	*focus_chain = g_list_copy (get_focus_chain (container));
      else
	*focus_chain = NULL;
    }

  return container->has_focus_chain;
}

/**
 * btk_container_unset_focus_chain:
 * @container: a #BtkContainer
 * 
 * Removes a focus chain explicitly set with btk_container_set_focus_chain().
 **/
void
btk_container_unset_focus_chain (BtkContainer  *container)
{  
  g_return_if_fail (BTK_IS_CONTAINER (container));

  if (container->has_focus_chain)
    {
      GList *chain;
      GList *tmp_list;
      
      chain = get_focus_chain (container);
      
      container->has_focus_chain = FALSE;
      
      g_object_set_data (G_OBJECT (container), 
                         I_("btk-container-focus-chain"),
                         NULL);

      tmp_list = chain;
      while (tmp_list != NULL)
        {
          g_signal_handlers_disconnect_by_func (tmp_list->data,
                                                chain_widget_destroyed,
                                                container);
          
          tmp_list = g_list_next (tmp_list);
        }

      g_list_free (chain);
    }
}

/**
 * btk_container_set_focus_vadjustment:
 * @container: a #BtkContainer
 * @adjustment: an adjustment which should be adjusted when the focus 
 *   is moved among the descendents of @container
 * 
 * Hooks up an adjustment to focus handling in a container, so when a 
 * child of the container is focused, the adjustment is scrolled to 
 * show that widget. This function sets the vertical alignment. See 
 * btk_scrolled_window_get_vadjustment() for a typical way of obtaining 
 * the adjustment and btk_container_set_focus_hadjustment() for setting
 * the horizontal adjustment.
 *
 * The adjustments have to be in pixel units and in the same coordinate 
 * system as the allocation for immediate children of the container. 
 */
void
btk_container_set_focus_vadjustment (BtkContainer  *container,
				     BtkAdjustment *adjustment)
{
  g_return_if_fail (BTK_IS_CONTAINER (container));
  if (adjustment)
    g_return_if_fail (BTK_IS_ADJUSTMENT (adjustment));

  if (adjustment)
    g_object_ref (adjustment);

  g_object_set_qdata_full (G_OBJECT (container),
			   vadjustment_key_id,
			   adjustment,
			   g_object_unref);
}

/**
 * btk_container_get_focus_vadjustment:
 * @container: a #BtkContainer
 *
 * Retrieves the vertical focus adjustment for the container. See
 * btk_container_set_focus_vadjustment().
 *
 * Return value: (transfer none): the vertical focus adjustment, or %NULL if
 *   none has been set.
 **/
BtkAdjustment *
btk_container_get_focus_vadjustment (BtkContainer *container)
{
  BtkAdjustment *vadjustment;
    
  g_return_val_if_fail (BTK_IS_CONTAINER (container), NULL);

  vadjustment = g_object_get_qdata (G_OBJECT (container), vadjustment_key_id);

  return vadjustment;
}

/**
 * btk_container_set_focus_hadjustment:
 * @container: a #BtkContainer
 * @adjustment: an adjustment which should be adjusted when the focus is 
 *   moved among the descendents of @container
 * 
 * Hooks up an adjustment to focus handling in a container, so when a child 
 * of the container is focused, the adjustment is scrolled to show that 
 * widget. This function sets the horizontal alignment. 
 * See btk_scrolled_window_get_hadjustment() for a typical way of obtaining 
 * the adjustment and btk_container_set_focus_vadjustment() for setting
 * the vertical adjustment.
 *
 * The adjustments have to be in pixel units and in the same coordinate 
 * system as the allocation for immediate children of the container. 
 */
void
btk_container_set_focus_hadjustment (BtkContainer  *container,
				     BtkAdjustment *adjustment)
{
  g_return_if_fail (BTK_IS_CONTAINER (container));
  if (adjustment)
    g_return_if_fail (BTK_IS_ADJUSTMENT (adjustment));

  if (adjustment)
    g_object_ref (adjustment);

  g_object_set_qdata_full (G_OBJECT (container),
			   hadjustment_key_id,
			   adjustment,
			   g_object_unref);
}

/**
 * btk_container_get_focus_hadjustment:
 * @container: a #BtkContainer
 *
 * Retrieves the horizontal focus adjustment for the container. See
 * btk_container_set_focus_hadjustment ().
 *
 * Return value: (transfer none): the horizontal focus adjustment, or %NULL if
 *   none has been set.
 **/
BtkAdjustment *
btk_container_get_focus_hadjustment (BtkContainer *container)
{
  BtkAdjustment *hadjustment;

  g_return_val_if_fail (BTK_IS_CONTAINER (container), NULL);

  hadjustment = g_object_get_qdata (G_OBJECT (container), hadjustment_key_id);

  return hadjustment;
}


static void
btk_container_show_all (BtkWidget *widget)
{
  g_return_if_fail (BTK_IS_CONTAINER (widget));

  btk_container_foreach (BTK_CONTAINER (widget),
			 (BtkCallback) btk_widget_show_all,
			 NULL);
  btk_widget_show (widget);
}

static void
btk_container_hide_all (BtkWidget *widget)
{
  g_return_if_fail (BTK_IS_CONTAINER (widget));

  btk_widget_hide (widget);
  btk_container_foreach (BTK_CONTAINER (widget),
			 (BtkCallback) btk_widget_hide_all,
			 NULL);
}


static void
btk_container_expose_child (BtkWidget *child,
			    gpointer   client_data)
{
  struct {
    BtkWidget *container;
    BdkEventExpose *event;
  } *data = client_data;
  
  btk_container_propagate_expose (BTK_CONTAINER (data->container),
				  child,
				  data->event);
}

static gint 
btk_container_expose (BtkWidget      *widget,
		      BdkEventExpose *event)
{
  struct {
    BtkWidget *container;
    BdkEventExpose *event;
  } data;

  g_return_val_if_fail (BTK_IS_CONTAINER (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  
  if (btk_widget_is_drawable (widget))
    {
      data.container = widget;
      data.event = event;
      
      btk_container_forall (BTK_CONTAINER (widget),
			    btk_container_expose_child,
			    &data);
    }   
  
  return FALSE;
}

static void
btk_container_map_child (BtkWidget *child,
			 gpointer   client_data)
{
  if (btk_widget_get_visible (child) &&
      BTK_WIDGET_CHILD_VISIBLE (child) &&
      !btk_widget_get_mapped (child))
    btk_widget_map (child);
}

static void
btk_container_map (BtkWidget *widget)
{
  btk_widget_set_mapped (widget, TRUE);

  btk_container_forall (BTK_CONTAINER (widget),
			btk_container_map_child,
			NULL);

  if (btk_widget_get_has_window (widget))
    bdk_window_show (widget->window);
}

static void
btk_container_unmap (BtkWidget *widget)
{
  btk_widget_set_mapped (widget, FALSE);

  if (btk_widget_get_has_window (widget))
    bdk_window_hide (widget->window);
  else
    btk_container_forall (BTK_CONTAINER (widget),
			  (BtkCallback)btk_widget_unmap,
			  NULL);
}

/**
 * btk_container_propagate_expose:
 * @container: a #BtkContainer
 * @child: a child of @container
 * @event: a expose event sent to container
 *
 * When a container receives an expose event, it must send synthetic
 * expose events to all children that don't have their own #BdkWindows.
 * This function provides a convenient way of doing this. A container,
 * when it receives an expose event, calls btk_container_propagate_expose()
 * once for each child, passing in the event the container received.
 *
 * btk_container_propagate_expose() takes care of deciding whether
 * an expose event needs to be sent to the child, intersecting
 * the event's area with the child area, and sending the event.
 *
 * In most cases, a container can simply either simply inherit the
 * #BtkWidget::expose implementation from #BtkContainer, or, do some drawing
 * and then chain to the ::expose implementation from #BtkContainer.
 *
 * Note that the ::expose-event signal has been replaced by a ::draw
 * signal in BTK+ 3, and consequently, btk_container_propagate_expose()
 * has been replaced by btk_container_propagate_draw().
 * The <link linkend="http://library.bunny.org/devel/btk3/3.0/btk-migrating-2-to-3.html">BTK+ 3 migration guide</link>
 * for hints on how to port from ::expose-event to ::draw.
 *
 **/
void
btk_container_propagate_expose (BtkContainer   *container,
				BtkWidget      *child,
				BdkEventExpose *event)
{
  BdkEvent *child_event;

  g_return_if_fail (BTK_IS_CONTAINER (container));
  g_return_if_fail (BTK_IS_WIDGET (child));
  g_return_if_fail (event != NULL);

  g_assert (child->parent == BTK_WIDGET (container));
  
  if (btk_widget_is_drawable (child) &&
      !btk_widget_get_has_window (child) &&
      (child->window == event->window))
    {
      child_event = bdk_event_new (BDK_EXPOSE);
      child_event->expose = *event;
      g_object_ref (child_event->expose.window);

      child_event->expose.rebunnyion = btk_widget_rebunnyion_intersect (child, event->rebunnyion);
      if (!bdk_rebunnyion_empty (child_event->expose.rebunnyion))
	{
	  bdk_rebunnyion_get_clipbox (child_event->expose.rebunnyion, &child_event->expose.area);
	  btk_widget_send_expose (child, child_event);
	}
      bdk_event_free (child_event);
    }
}

#define __BTK_CONTAINER_C__
#include "btkaliasdef.c"
