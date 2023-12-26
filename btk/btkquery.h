/*
 * Copyright (C) 2005 Novell, Inc.
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
 *
 * Author: Anders Carlsson <andersca@imendio.com>
 *
 * Based on nautilus-query.h
 */

#ifndef __BTK_QUERY_H__
#define __BTK_QUERY_H__

#include <bunnylib-object.h>

G_BEGIN_DECLS

#define BTK_TYPE_QUERY		(_btk_query_get_type ())
#define BTK_QUERY(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_QUERY, BtkQuery))
#define BTK_QUERY_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_QUERY, BtkQueryClass))
#define BTK_IS_QUERY(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_QUERY))
#define BTK_IS_QUERY_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_QUERY))
#define BTK_QUERY_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_QUERY, BtkQueryClass))

typedef struct _BtkQuery BtkQuery;
typedef struct _BtkQueryClass BtkQueryClass;
typedef struct _BtkQueryPrivate BtkQueryPrivate;

struct _BtkQuery 
{
  GObject parent;

  BtkQueryPrivate *priv;
};

struct _BtkQueryClass
{
  GObjectClass parent_class;
};

GType     _btk_query_get_type       (void);
gboolean  _btk_query_enabled        (void);

BtkQuery* _btk_query_new            (void);

gchar*    _btk_query_get_text       (BtkQuery    *query);
void      _btk_query_set_text       (BtkQuery    *query, 
				     const gchar *text);

gchar*    _btk_query_get_location   (BtkQuery    *query);
void      _btk_query_set_location   (BtkQuery    *query, 
				     const gchar *uri);

GList*    _btk_query_get_mime_types (BtkQuery    *query);
void      _btk_query_set_mime_types (BtkQuery    *query, 
				     GList       *mime_types);
void      _btk_query_add_mime_type  (BtkQuery    *query, 
				     const gchar *mime_type);

G_END_DECLS

#endif /* __BTK_QUERY_H__ */
