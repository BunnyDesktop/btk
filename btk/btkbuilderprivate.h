/* btkbuilderprivate.h
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

#ifndef __BTK_BUILDER_PRIVATE_H__
#define __BTK_BUILDER_PRIVATE_H__

#include "btkbuilder.h"

typedef struct {
  const gchar *name;
} TagInfo;

typedef struct {
  TagInfo tag;
} CommonInfo;

typedef struct {
  TagInfo tag;
  gchar *class_name;
  gchar *id;
  gchar *constructor;
  GSList *properties;
  GSList *signals;
  BObject *object;
  CommonInfo *parent;
} ObjectInfo;

typedef struct {
  TagInfo tag;
  GSList *packing_properties;
  BObject *object;
  CommonInfo *parent;
  gchar *type;
  gchar *internal_child;
  gboolean added;
} ChildInfo;

typedef struct {
  TagInfo tag;
  gchar *name;
  GString *text;
  gchar *data;
  gboolean translatable;
  gchar *context;
} PropertyInfo;

typedef struct {
  TagInfo tag;
  gchar *object_name;
  gchar *name;
  gchar *handler;
  GConnectFlags flags;
  gchar *connect_object_name;
} SignalInfo;

typedef struct {
  TagInfo  tag;
  gchar   *library;
  gint     major;
  gint     minor;
} RequiresInfo;

typedef struct {
  GMarkupParser *parser;
  gchar *tagname;
  const gchar *start;
  gpointer data;
  BObject *object;
  BObject *child;
} SubParser;

typedef struct {
  const gchar *last_element;
  BtkBuilder *builder;
  gchar *domain;
  GSList *stack;
  SubParser *subparser;
  GMarkupParseContext *ctx;
  const gchar *filename;
  GSList *finalizers;
  GSList *custom_finalizers;

  GSList *requested_objects; /* NULL if all the objects are requested */
  gboolean inside_requested_object;
  gint requested_object_level;
  gint cur_object_level;

  GHashTable *object_ids;
} ParserData;

typedef GType (*GTypeGetFunc) (void);

/* Things only BtkBuilder should use */
void _btk_builder_parser_parse_buffer (BtkBuilder *builder,
                                       const gchar *filename,
                                       const gchar *buffer,
                                       gsize length,
                                       gchar **requested_objs,
                                       GError **error);
BObject * _btk_builder_construct (BtkBuilder *builder,
                                  ObjectInfo *info,
				  GError    **error);
void      _btk_builder_add (BtkBuilder *builder,
                            ChildInfo *child_info);
void      _btk_builder_add_signals (BtkBuilder *builder,
				    GSList     *signals);
void      _btk_builder_finish (BtkBuilder *builder);
void _free_signal_info (SignalInfo *info,
                        gpointer user_data);

/* Internal API which might be made public at some point */
gboolean _btk_builder_boolean_from_string (const gchar  *string,
					   gboolean     *value,
					   GError      **error);
gboolean _btk_builder_enum_from_string (GType         type,
                                        const gchar  *string,
                                        gint         *enum_value,
                                        GError      **error);
gboolean  _btk_builder_flags_from_string (GType       type,
					  const char *string,
					  guint      *value,
					  GError    **error);
gchar * _btk_builder_parser_translate (const gchar *domain,
				       const gchar *context,
				       const gchar *text);
gchar *   _btk_builder_get_absolute_filename (BtkBuilder *builder,
					      const gchar *string);

#endif /* __BTK_BUILDER_PRIVATE_H__ */
