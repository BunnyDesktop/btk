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

#include <Cocoa/Cocoa.h>
#include <quartz/bdkquartz.h>

#include "btksearchenginequartz.h"

/* This file is a mixture of an objective-C object and a BObject,
 * so be careful to not confuse yourself.
 */

#define QUARTZ_POOL_ALLOC NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init]
#define QUARTZ_POOL_RELEASE [pool release]


/* Definition of objective-c object */
@interface ResultReceiver : NSObject
{
  int submitted_hits;
  BtkSearchEngine *engine;
}

- (void) setEngine:(BtkSearchEngine *)quartz_engine;

- (void) queryUpdate:(id)sender;
- (void) queryProgress:(id)sender;
- (void) queryFinished:(id)sender;

@end


/* Definition of BObject */
struct _BtkSearchEngineQuartzPrivate
{
  BtkQuery *query;

  ResultReceiver *receiver;
  NSMetadataQuery *ns_query;

  bboolean query_finished;
};

G_DEFINE_TYPE (BtkSearchEngineQuartz, _btk_search_engine_quartz, BTK_TYPE_SEARCH_ENGINE);


/* Implementation of the objective-C object */
@implementation ResultReceiver

- (void) setEngine:(BtkSearchEngine *)quartz_engine
{
  g_return_if_fail (BTK_IS_SEARCH_ENGINE (quartz_engine));

  engine = quartz_engine;
  submitted_hits = 0;
}

- (void) submitHits:(NSMetadataQuery *)ns_query
{
  int i;
  GList *hits = NULL;

  /* Here we submit hits "submitted_hits" to "resultCount" */
  for (i = submitted_hits; i < [ns_query resultCount]; i++)
    {
      id result = [ns_query resultAtIndex:i];
      char *result_path;

      result_path = g_strdup_printf ("file://%s", [[result valueForAttribute:@"kMDItemPath"] cString]);
      hits = g_list_prepend (hits, result_path);
    }

  _btk_search_engine_hits_added (engine, hits);
  g_list_free (hits);

  submitted_hits = [ns_query resultCount];

  /* The beagle backend stops at 1000 hits, so guess we do so too here.
   * It works pretty snappy on my MacBook, if we get rid of this limit
   * we are almost definitely going to need some code to submit hits
   * in batches.
   */
  if (submitted_hits > 1000)
    [ns_query stopQuery];
}

- (void) queryUpdate:(id)sender
{
  NSMetadataQuery *ns_query = [sender object];

  [self submitHits:ns_query];
}

- (void) queryProgress:(id)sender
{
  NSMetadataQuery *ns_query = [sender object];

  [self submitHits:ns_query];
}

- (void) queryFinished:(id)sender
{
  NSMetadataQuery *ns_query = [sender object];

  [self submitHits:ns_query];

  _btk_search_engine_finished (engine);
  submitted_hits = 0;
}

@end

/* Implementation of the BObject */

static void
btk_search_engine_quartz_finalize (BObject *object)
{
  BtkSearchEngineQuartz *quartz;

  QUARTZ_POOL_ALLOC;

  quartz = BTK_SEARCH_ENGINE_QUARTZ (object);

  [[NSNotificationCenter defaultCenter] removeObserver:quartz->priv->receiver];

  [quartz->priv->ns_query release];
  [quartz->priv->receiver release];

  QUARTZ_POOL_RELEASE;

  if (quartz->priv->query)
    {
      g_object_unref (quartz->priv->query);
      quartz->priv->query = NULL;
    }

  B_OBJECT_CLASS (_btk_search_engine_quartz_parent_class)->finalize (object);
}

static void
btk_search_engine_quartz_start (BtkSearchEngine *engine)
{
  BtkSearchEngineQuartz *quartz;

  QUARTZ_POOL_ALLOC;

  quartz = BTK_SEARCH_ENGINE_QUARTZ (engine);

  [quartz->priv->ns_query startQuery];

  QUARTZ_POOL_RELEASE;
}

static void
btk_search_engine_quartz_stop (BtkSearchEngine *engine)
{
  BtkSearchEngineQuartz *quartz;

  QUARTZ_POOL_ALLOC;

  quartz = BTK_SEARCH_ENGINE_QUARTZ (engine);

  [quartz->priv->ns_query stopQuery];

  QUARTZ_POOL_RELEASE;
}

static bboolean
btk_search_engine_quartz_is_indexed (BtkSearchEngine *engine)
{
  return TRUE;
}

static void
btk_search_engine_quartz_set_query (BtkSearchEngine *engine, 
				    BtkQuery        *query)
{
  BtkSearchEngineQuartz *quartz;

  QUARTZ_POOL_ALLOC;

  quartz = BTK_SEARCH_ENGINE_QUARTZ (engine);

  if (query)
    g_object_ref (query);

  if (quartz->priv->query)
    g_object_unref (quartz->priv->query);

  quartz->priv->query = query;

  /* We create a query to look for ".*text.*" in the text contents of
   * all indexed files.  (Should we also search for text in file and folder
   * names?).
   */
  [quartz->priv->ns_query setPredicate:
    [NSPredicate predicateWithFormat:
      [NSString stringWithFormat:@"(kMDItemTextContent LIKE[cd] \"*%s*\")",
                                 _btk_query_get_text (query)]]];

  QUARTZ_POOL_RELEASE;
}

static void
_btk_search_engine_quartz_class_init (BtkSearchEngineQuartzClass *class)
{
  BObjectClass *bobject_class;
  BtkSearchEngineClass *engine_class;
  
  bobject_class = B_OBJECT_CLASS (class);
  bobject_class->finalize = btk_search_engine_quartz_finalize;
  
  engine_class = BTK_SEARCH_ENGINE_CLASS (class);
  engine_class->set_query = btk_search_engine_quartz_set_query;
  engine_class->start = btk_search_engine_quartz_start;
  engine_class->stop = btk_search_engine_quartz_stop;
  engine_class->is_indexed = btk_search_engine_quartz_is_indexed;

  g_type_class_add_private (bobject_class, sizeof (BtkSearchEngineQuartzPrivate));
}

static void
_btk_search_engine_quartz_init (BtkSearchEngineQuartz *engine)
{
  QUARTZ_POOL_ALLOC;

  engine->priv = B_TYPE_INSTANCE_GET_PRIVATE (engine, BTK_TYPE_SEARCH_ENGINE_QUARTZ, BtkSearchEngineQuartzPrivate);

  engine->priv->ns_query = [[NSMetadataQuery alloc] init];
  engine->priv->receiver = [[ResultReceiver alloc] init];

  [engine->priv->receiver setEngine:BTK_SEARCH_ENGINE (engine)];

  [[NSNotificationCenter defaultCenter] addObserver:engine->priv->receiver
				        selector:@selector(queryUpdate:)
				        name:@"NSMetadataQueryDidUpdateNotification"
				        object:engine->priv->ns_query];
  [[NSNotificationCenter defaultCenter] addObserver:engine->priv->receiver
				        selector:@selector(queryFinished:)
				        name:@"NSMetadataQueryDidFinishGatheringNotification"
				        object:engine->priv->ns_query];

  [[NSNotificationCenter defaultCenter] addObserver:engine->priv->receiver
				        selector:@selector(queryProgress:)
				        name:@"NSMetadataQueryGatheringProgressNotification"
				        object:engine->priv->ns_query];

  QUARTZ_POOL_RELEASE;
}

BtkSearchEngine *
_btk_search_engine_quartz_new (void)
{
#ifdef BDK_WINDOWING_QUARTZ
  return g_object_new (BTK_TYPE_SEARCH_ENGINE_QUARTZ, NULL);
#else
  return NULL;
#endif
}
