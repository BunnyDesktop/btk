/*
 * Copyright (C) 2007  Kristian Rietveld  <kris@btk.org>
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
 */

#ifndef __BTK_SEARCH_ENGINE_QUARTZ_H__
#define __BTK_SEARCH_ENGINE_QUARTZ_H__

#include "btksearchengine.h"

G_BEGIN_DECLS

#define BTK_TYPE_SEARCH_ENGINE_QUARTZ			(_btk_search_engine_quartz_get_type ())
#define BTK_SEARCH_ENGINE_QUARTZ(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_SEARCH_ENGINE_QUARTZ, BtkSearchEngineQuartz))
#define BTK_SEARCH_ENGINE_QUARTZ_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_SEARCH_ENGINE_QUARTZ, BtkSearchEngineQuartzClass))
#define BTK_IS_SEARCH_ENGINE_QUARTZ(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_SEARCH_ENGINE_QUARTZ))
#define BTK_IS_SEARCH_ENGINE_QUARTZ_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_SEARCH_ENGINE_QUARTZ))
#define BTK_SEARCH_ENGINE_QUARTZ_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_SEARCH_ENGINE_QUARTZ, BtkSearchEngineQuartzClass))

typedef struct _BtkSearchEngineQuartz BtkSearchEngineQuartz;
typedef struct _BtkSearchEngineQuartzClass BtkSearchEngineQuartzClass;
typedef struct _BtkSearchEngineQuartzPrivate BtkSearchEngineQuartzPrivate;

struct _BtkSearchEngineQuartz
{
  BtkSearchEngine parent;

  BtkSearchEngineQuartzPrivate *priv;
};

struct _BtkSearchEngineQuartzClass
{
  BtkSearchEngineClass parent_class;
};

GType            _btk_search_engine_quartz_get_type (void);
BtkSearchEngine *_btk_search_engine_quartz_new      (void);

G_END_DECLS

#endif /* __BTK_SEARCH_ENGINE_QUARTZ_H__ */
