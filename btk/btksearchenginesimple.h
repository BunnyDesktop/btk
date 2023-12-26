/*
 * Copyright (C) 2005 Red Hat, Inc
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
 * Author: Alexander Larsson <alexl@redhat.com>
 *
 * Based on nautilus-search-engine-simple.h
 */

#ifndef __BTK_SEARCH_ENGINE_SIMPLE_H__
#define __BTK_SEARCH_ENGINE_SIMPLE_H__

#include "btksearchengine.h"

B_BEGIN_DECLS

#define BTK_TYPE_SEARCH_ENGINE_SIMPLE		(_btk_search_engine_simple_get_type ())
#define BTK_SEARCH_ENGINE_SIMPLE(obj)		(B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_SEARCH_ENGINE_SIMPLE, BtkSearchEngineSimple))
#define BTK_SEARCH_ENGINE_SIMPLE_CLASS(klass)	(B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_SEARCH_ENGINE_SIMPLE, BtkSearchEngineSimpleClass))
#define BTK_IS_SEARCH_ENGINE_SIMPLE(obj)		(B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_SEARCH_ENGINE_SIMPLE))
#define BTK_IS_SEARCH_ENGINE_SIMPLE_CLASS(klass)	(B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_SEARCH_ENGINE_SIMPLE))
#define BTK_SEARCH_ENGINE_SIMPLE_GET_CLASS(obj)    (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_SEARCH_ENGINE_SIMPLE, BtkSearchEngineSimpleClass))

typedef struct _BtkSearchEngineSimple BtkSearchEngineSimple;
typedef struct _BtkSearchEngineSimpleClass BtkSearchEngineSimpleClass;
typedef struct _BtkSearchEngineSimplePrivate BtkSearchEngineSimplePrivate;

struct _BtkSearchEngineSimple 
{
  BtkSearchEngine parent;

  BtkSearchEngineSimplePrivate *priv;
};

struct _BtkSearchEngineSimpleClass
{
  BtkSearchEngineClass parent_class;
};

GType            _btk_search_engine_simple_get_type (void);

BtkSearchEngine* _btk_search_engine_simple_new      (void);

B_END_DECLS

#endif /* __BTK_SEARCH_ENGINE_SIMPLE_H__ */
