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
 * Based on nautilus-search-engine-beagle.h
 */

#ifndef __BTK_SEARCH_ENGINE_BEAGLE_H__
#define __BTK_SEARCH_ENGINE_BEAGLE_H__

#include "btksearchengine.h"

B_BEGIN_DECLS

#define BTK_TYPE_SEARCH_ENGINE_BEAGLE		(_btk_search_engine_beagle_get_type ())
#define BTK_SEARCH_ENGINE_BEAGLE(obj)		(B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_SEARCH_ENGINE_BEAGLE, BtkSearchEngineBeagle))
#define BTK_SEARCH_ENGINE_BEAGLE_CLASS(klass)	(B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_SEARCH_ENGINE_BEAGLE, BtkSearchEngineBeagleClass))
#define BTK_IS_SEARCH_ENGINE_BEAGLE(obj)		(B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_SEARCH_ENGINE_BEAGLE))
#define BTK_IS_SEARCH_ENGINE_BEAGLE_CLASS(klass)	(B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_SEARCH_ENGINE_BEAGLE))
#define BTK_SEARCH_ENGINE_BEAGLE_GET_CLASS(obj)    (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_SEARCH_ENGINE_BEAGLE, BtkSearchEngineBeagleClass))

typedef struct _BtkSearchEngineBeagle BtkSearchEngineBeagle;
typedef struct _BtkSearchEngineBeagleClass BtkSearchEngineBeagleClass;
typedef struct _BtkSearchEngineBeaglePrivate BtkSearchEngineBeaglePrivate;

struct _BtkSearchEngineBeagle 
{
  BtkSearchEngine parent;

  BtkSearchEngineBeaglePrivate *priv;
};

struct _BtkSearchEngineBeagleClass 
{
  BtkSearchEngineClass parent_class;
};

GType            _btk_search_engine_beagle_get_type (void);

BtkSearchEngine* _btk_search_engine_beagle_new      (void);

B_END_DECLS

#endif /* __BTK_SEARCH_ENGINE_BEAGLE_H__ */
