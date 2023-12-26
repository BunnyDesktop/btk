/* BTK - The GIMP Toolkit
 * Copyright (C) 2006-2007 Async Open Source,
 *                         Johan Dahlin <jdahlin@async.com.br>
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

#ifndef __BTK_BUILDER_H__
#define __BTK_BUILDER_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <bdkconfig.h>
#include <bunnylib-object.h>

G_BEGIN_DECLS

#define BTK_TYPE_BUILDER                 (btk_builder_get_type ())
#define BTK_BUILDER(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_BUILDER, BtkBuilder))
#define BTK_BUILDER_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_BUILDER, BtkBuilderClass))
#define BTK_IS_BUILDER(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_BUILDER))
#define BTK_IS_BUILDER_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_BUILDER))
#define BTK_BUILDER_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_BUILDER, BtkBuilderClass))

#define BTK_BUILDER_ERROR                (btk_builder_error_quark ())

typedef struct _BtkBuilder        BtkBuilder;
typedef struct _BtkBuilderClass   BtkBuilderClass;
typedef struct _BtkBuilderPrivate BtkBuilderPrivate;

typedef enum
{
  BTK_BUILDER_ERROR_INVALID_TYPE_FUNCTION,
  BTK_BUILDER_ERROR_UNHANDLED_TAG,
  BTK_BUILDER_ERROR_MISSING_ATTRIBUTE,
  BTK_BUILDER_ERROR_INVALID_ATTRIBUTE,
  BTK_BUILDER_ERROR_INVALID_TAG,
  BTK_BUILDER_ERROR_MISSING_PROPERTY_VALUE,
  BTK_BUILDER_ERROR_INVALID_VALUE,
  BTK_BUILDER_ERROR_VERSION_MISMATCH,
  BTK_BUILDER_ERROR_DUPLICATE_ID
} BtkBuilderError;

GQuark btk_builder_error_quark (void);

struct _BtkBuilder
{
  GObject parent_instance;

  BtkBuilderPrivate *GSEAL (priv);
};

struct _BtkBuilderClass
{
  GObjectClass parent_class;
  
  GType (* get_type_from_name) (BtkBuilder *builder,
                                const char *type_name);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
  void (*_btk_reserved5) (void);
  void (*_btk_reserved6) (void);
  void (*_btk_reserved7) (void);
  void (*_btk_reserved8) (void);
};

typedef void (*BtkBuilderConnectFunc) (BtkBuilder    *builder,
				       GObject       *object,
				       const gchar   *signal_name,
				       const gchar   *handler_name,
				       GObject       *connect_object,
				       GConnectFlags  flags,
				       gpointer       user_data);

GType        btk_builder_get_type                (void) G_GNUC_CONST;
BtkBuilder*  btk_builder_new                     (void);

guint        btk_builder_add_from_file           (BtkBuilder    *builder,
                                                  const gchar   *filename,
                                                  GError       **error);
guint        btk_builder_add_from_string         (BtkBuilder    *builder,
                                                  const gchar   *buffer,
                                                  gsize          length,
                                                  GError       **error);
guint        btk_builder_add_objects_from_file   (BtkBuilder    *builder,
                                                  const gchar   *filename,
                                                  gchar        **object_ids,
                                                  GError       **error);
guint        btk_builder_add_objects_from_string (BtkBuilder    *builder,
                                                  const gchar   *buffer,
                                                  gsize          length,
                                                  gchar        **object_ids,
                                                  GError       **error);
GObject*     btk_builder_get_object              (BtkBuilder    *builder,
                                                  const gchar   *name);
GSList*      btk_builder_get_objects             (BtkBuilder    *builder);
void         btk_builder_connect_signals         (BtkBuilder    *builder,
						  gpointer       user_data);
void         btk_builder_connect_signals_full    (BtkBuilder    *builder,
                                                  BtkBuilderConnectFunc func,
						  gpointer       user_data);
void         btk_builder_set_translation_domain  (BtkBuilder   	*builder,
                                                  const gchar  	*domain);
const gchar* btk_builder_get_translation_domain  (BtkBuilder   	*builder);
GType        btk_builder_get_type_from_name      (BtkBuilder   	*builder,
                                                  const char   	*type_name);

gboolean     btk_builder_value_from_string       (BtkBuilder    *builder,
						  GParamSpec   	*pspec,
                                                  const gchar  	*string,
                                                  GValue       	*value,
						  GError       **error);
gboolean     btk_builder_value_from_string_type  (BtkBuilder    *builder,
						  GType        	 type,
                                                  const gchar  	*string,
                                                  GValue       	*value,
						  GError       **error);

#define BTK_BUILDER_WARN_INVALID_CHILD_TYPE(object, type) \
  g_warning ("'%s' is not a valid child type of '%s'", type, g_type_name (G_OBJECT_TYPE (object)))

G_END_DECLS

#endif /* __BTK_BUILDER_H__ */
