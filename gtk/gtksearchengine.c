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
 * Based on nautilus-search-engine.c
 */

#include "config.h"
#include "btksearchengine.h"
#include "btksearchenginebeagle.h"
#include "btksearchenginesimple.h"
#include "btksearchenginetracker.h"
#include "btksearchenginequartz.h"

#include <bdk/bdkconfig.h> /* for BDK_WINDOWING_QUARTZ */

#ifndef G_OS_WIN32		/* Beagle and tracker are not ported
				 * to Windows, as far as I know.
				 */

#define HAVE_BEAGLE  1 
#define HAVE_TRACKER 1

#endif

enum 
{
  HITS_ADDED,
  HITS_SUBTRACTED,
  FINISHED,
  ERROR,
  LAST_SIGNAL
}; 

static guint signals[LAST_SIGNAL];

G_DEFINE_ABSTRACT_TYPE (BtkSearchEngine, _btk_search_engine, G_TYPE_OBJECT);

static void
finalize (GObject *object)
{
  G_OBJECT_CLASS (_btk_search_engine_parent_class)->finalize (object);
}

static void
_btk_search_engine_class_init (BtkSearchEngineClass *class)
{
  GObjectClass *bobject_class;
  
  bobject_class = G_OBJECT_CLASS (class);
  bobject_class->finalize = finalize;
  
  signals[HITS_ADDED] =
    g_signal_new ("hits-added",
		  G_TYPE_FROM_CLASS (class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkSearchEngineClass, hits_added),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__POINTER,
		  G_TYPE_NONE, 1,
		  G_TYPE_POINTER);
  
  signals[HITS_SUBTRACTED] =
    g_signal_new ("hits-subtracted",
		  G_TYPE_FROM_CLASS (class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkSearchEngineClass, hits_subtracted),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__POINTER,
		  G_TYPE_NONE, 1,
		  G_TYPE_POINTER);
  
  signals[FINISHED] =
    g_signal_new ("finished",
		  G_TYPE_FROM_CLASS (class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkSearchEngineClass, finished),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  
  signals[ERROR] =
    g_signal_new ("error",
		  G_TYPE_FROM_CLASS (class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkSearchEngineClass, error),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__STRING,
		  G_TYPE_NONE, 1,
		  G_TYPE_STRING);  
}

static void
_btk_search_engine_init (BtkSearchEngine *engine)
{
}

BtkSearchEngine *
_btk_search_engine_new (void)
{
  BtkSearchEngine *engine = NULL;
	
#ifdef HAVE_TRACKER
  engine = _btk_search_engine_tracker_new ();
  if (engine)
    return engine;
#endif
  
#ifdef HAVE_BEAGLE
  engine = _btk_search_engine_beagle_new ();
  if (engine)
    return engine;
#endif

#ifdef BDK_WINDOWING_QUARTZ
  engine = _btk_search_engine_quartz_new ();
  if (engine)
    return engine;
#endif

  if (g_thread_supported ())
    engine = _btk_search_engine_simple_new ();
  
  return engine;
}

void
_btk_search_engine_set_query (BtkSearchEngine *engine, 
			      BtkQuery        *query)
{
  g_return_if_fail (BTK_IS_SEARCH_ENGINE (engine));
  g_return_if_fail (BTK_SEARCH_ENGINE_GET_CLASS (engine)->set_query != NULL);
  
  BTK_SEARCH_ENGINE_GET_CLASS (engine)->set_query (engine, query);
}

void
_btk_search_engine_start (BtkSearchEngine *engine)
{
  g_return_if_fail (BTK_IS_SEARCH_ENGINE (engine));
  g_return_if_fail (BTK_SEARCH_ENGINE_GET_CLASS (engine)->start != NULL);
  
  BTK_SEARCH_ENGINE_GET_CLASS (engine)->start (engine);
}

void
_btk_search_engine_stop (BtkSearchEngine *engine)
{
  g_return_if_fail (BTK_IS_SEARCH_ENGINE (engine));
  g_return_if_fail (BTK_SEARCH_ENGINE_GET_CLASS (engine)->stop != NULL);
  
  BTK_SEARCH_ENGINE_GET_CLASS (engine)->stop (engine);
}

gboolean
_btk_search_engine_is_indexed (BtkSearchEngine *engine)
{
  g_return_val_if_fail (BTK_IS_SEARCH_ENGINE (engine), FALSE);
  g_return_val_if_fail (BTK_SEARCH_ENGINE_GET_CLASS (engine)->is_indexed != NULL, FALSE);
  
  return BTK_SEARCH_ENGINE_GET_CLASS (engine)->is_indexed (engine);
}

void	       
_btk_search_engine_hits_added (BtkSearchEngine *engine, 
			       GList           *hits)
{
  g_return_if_fail (BTK_IS_SEARCH_ENGINE (engine));
  
  g_signal_emit (engine, signals[HITS_ADDED], 0, hits);
}


void	       
_btk_search_engine_hits_subtracted (BtkSearchEngine *engine, 
				    GList           *hits)
{
  g_return_if_fail (BTK_IS_SEARCH_ENGINE (engine));
  
  g_signal_emit (engine, signals[HITS_SUBTRACTED], 0, hits);
}


void	       
_btk_search_engine_finished (BtkSearchEngine *engine)
{
  g_return_if_fail (BTK_IS_SEARCH_ENGINE (engine));
  
  g_signal_emit (engine, signals[FINISHED], 0);
}

void
_btk_search_engine_error (BtkSearchEngine *engine, 
			  const gchar     *error_message)
{
  g_return_if_fail (BTK_IS_SEARCH_ENGINE (engine));
  
  g_signal_emit (engine, signals[ERROR], 0, error_message);
}
