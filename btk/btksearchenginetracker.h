/*
 * Copyright (C) 2005 Mr Jamie McCracken
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
 * Author: Jamie McCracken (jamiemcc@bunny.org)
 *
 * Based on nautilus-search-engine-tracker.h
 */

#ifndef __BTK_SEARCH_ENGINE_TRACKER_H__
#define __BTK_SEARCH_ENGINE_TRACKER_H__

#include "btksearchengine.h"

B_BEGIN_DECLS

#define BTK_TYPE_SEARCH_ENGINE_TRACKER		(_btk_search_engine_tracker_get_type ())
#define BTK_SEARCH_ENGINE_TRACKER(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_SEARCH_ENGINE_TRACKER, BtkSearchEngineTracker))
#define BTK_SEARCH_ENGINE_TRACKER_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_SEARCH_ENGINE_TRACKER, BtkSearchEngineTrackerClass))
#define BTK_IS_SEARCH_ENGINE_TRACKER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_SEARCH_ENGINE_TRACKER))
#define BTK_IS_SEARCH_ENGINE_TRACKER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_SEARCH_ENGINE_TRACKER))
#define BTK_SEARCH_ENGINE_TRACKER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_SEARCH_ENGINE_TRACKER, BtkSearchEngineTrackerClass))

typedef struct _BtkSearchEngineTracker BtkSearchEngineTracker;
typedef struct _BtkSearchEngineTrackerClass BtkSearchEngineTrackerClass;
typedef struct _BtkSearchEngineTrackerPrivate BtkSearchEngineTrackerPrivate;

struct _BtkSearchEngineTracker 
{
  BtkSearchEngine parent;

  BtkSearchEngineTrackerPrivate *priv;
};

struct _BtkSearchEngineTrackerClass 
{
  BtkSearchEngineClass parent_class;
};

GType            _btk_search_engine_tracker_get_type (void);

BtkSearchEngine* _btk_search_engine_tracker_new      (void);

B_END_DECLS

#endif /* __BTK_SEARCH_ENGINE_TRACKER_H__ */
