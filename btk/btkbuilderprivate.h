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
  const bchar *name;
} TagInfo;

typedef struct {
  TagInfo tag;
} CommonInfo;

typedef struct {
  TagInfo tag;
  bchar *class_name;
  bchar *id;
  bchar *constructor;
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
  bchar *type;
  bchar *internal_child;
  bboolean added;
} ChildInfo;

typedef struct {
  TagInfo tag;
  bchar *name;
  GString *text;
  bchar *data;
  bboolean translatable;
  bchar *context;
} PropertyInfo;

typedef struct {
  TagInfo tag;
  bchar *object_name;
  bchar *name;
  bchar *handler;
  GConnectFlags flags;
  bchar *connect_object_name;
} SignalInfo;

typedef struct {
  TagInfo  tag;
  bchar   *library;
  bint     major;
  bint     minor;
} RequiresInfo;

typedef struct {
  GMarkupParser *parser;
  bchar *tagname;
  const bchar *start;
  bpointer data;
  BObject *object;
  BObject *child;
} SubParser;

typedef struct {
  const bchar *last_element;
  BtkBuilder *builder;
  bchar *domain;
  GSList *stack;
  SubParser *subparser;
  GMarkupParseContext *ctx;
  const bchar *filename;
  GSList *finalizers;
  GSList *custom_finalizers;

  GSList *requested_objects; /* NULL if all the objects are requested */
  bboolean inside_requested_object;
  bint requested_object_level;
  bint cur_object_level;

  GHashTable *object_ids;
} ParserData;

typedef GType (*GTypeGetFunc) (void);

/* Things only BtkBuilder should use */
void _btk_builder_parser_parse_buffer (BtkBuilder *builder,
                                       const bchar *filename,
                                       const bchar *buffer,
                                       bsize length,
                                       bchar **requested_objs,
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
                        bpointer user_data);

/* Internal API which might be made public at some point */
bboolean _btk_builder_boolean_from_string (const bchar  *string,
					   bboolean     *value,
					   GError      **error);
bboolean _btk_builder_enum_from_string (GType         type,
                                        const bchar  *string,
                                        bint         *enum_value,
                                        GError      **error);
bboolean  _btk_builder_flags_from_string (GType       type,
					  const char *string,
					  buint      *value,
					  GError    **error);
bchar * _btk_builder_parser_translate (const bchar *domain,
				       const bchar *context,
				       const bchar *text);
bchar *   _btk_builder_get_absolute_filename (BtkBuilder *builder,
					      const bchar *string);

#endif /* __BTK_BUILDER_PRIVATE_H__ */
