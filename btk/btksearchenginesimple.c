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
 * Based on nautilus-search-engine-simple.c
 */

#include "config.h"

/* these must be defined even when HAVE_GNU_FTW is not defined
 * because (really) old versions of GNU libc have ftw.h but do
 * export ftw() and friends only if _XOPEN_SOURCE and _GNU_SOURCE
 * are defined. see bug #444097.
 */
#define _XOPEN_SOURCE 500
#define _GNU_SOURCE 

#ifdef HAVE_FTW_H
#include <ftw.h>
#endif

#include "btksearchenginesimple.h"
#include "btkprivate.h"

#include <string.h>

#define BATCH_SIZE 500

typedef struct 
{
  BtkSearchEngineSimple *engine;
  
  bchar *path;
  bchar **words;
  GList *found_list;
  
  bint n_processed_files;
  GList *uri_hits;
  
  /* accessed on both threads: */
  volatile bboolean cancelled;
} SearchThreadData;


struct _BtkSearchEngineSimplePrivate 
{
  BtkQuery *query;
  
  SearchThreadData *active_search;
  
  bboolean query_finished;
};


G_DEFINE_TYPE (BtkSearchEngineSimple, _btk_search_engine_simple, BTK_TYPE_SEARCH_ENGINE);

static void
btk_search_engine_simple_dispose (BObject *object)
{
  BtkSearchEngineSimple *simple;
  BtkSearchEngineSimplePrivate *priv;
  
  simple = BTK_SEARCH_ENGINE_SIMPLE (object);
  priv = simple->priv;
  
  if (priv->query) 
    {
      g_object_unref (priv->query);
      priv->query = NULL;
    }

  if (priv->active_search)
    {
      priv->active_search->cancelled = TRUE;
      priv->active_search = NULL;
    }
  
  B_OBJECT_CLASS (_btk_search_engine_simple_parent_class)->dispose (object);
}

static SearchThreadData *
search_thread_data_new (BtkSearchEngineSimple *engine,
			BtkQuery              *query)
{
  SearchThreadData *data;
  char *text, *lower, *uri;
  
  data = g_new0 (SearchThreadData, 1);
  
  data->engine = g_object_ref (engine);
  uri = _btk_query_get_location (query);
  if (uri != NULL) 
    {
      data->path = g_filename_from_uri (uri, NULL, NULL);
      g_free (uri);
    }
  if (data->path == NULL)
    data->path = g_strdup (g_get_home_dir ());
	
  text = _btk_query_get_text (query);
  lower = g_ascii_strdown (text, -1);
  data->words = g_strsplit (lower, " ", -1);
  g_free (text);
  g_free (lower);
  
  return data;
}

static void 
search_thread_data_free (SearchThreadData *data)
{
  g_object_unref (data->engine);
  g_free (data->path);
  g_strfreev (data->words);
  g_free (data);
}

static bboolean
search_thread_done_idle (bpointer user_data)
{
  SearchThreadData *data;

  data = user_data;
  
  if (!data->cancelled)
    _btk_search_engine_finished (BTK_SEARCH_ENGINE (data->engine));
     
  data->engine->priv->active_search = NULL;
  search_thread_data_free (data);
  
  return FALSE;
}

typedef struct 
{
  GList *uris;
  SearchThreadData *thread_data;
} SearchHits;


static bboolean
search_thread_add_hits_idle (bpointer user_data)
{
  SearchHits *hits;

  hits = user_data;

  if (!hits->thread_data->cancelled) 
    {
      _btk_search_engine_hits_added (BTK_SEARCH_ENGINE (hits->thread_data->engine),
				    hits->uris);
    }

  g_list_foreach (hits->uris, (GFunc)g_free, NULL);
  g_list_free (hits->uris);
  g_free (hits);
	
  return FALSE;
}

static void
send_batch (SearchThreadData *data)
{
  SearchHits *hits;
  
  data->n_processed_files = 0;
  
  if (data->uri_hits) 
    {
      hits = g_new (SearchHits, 1);
      hits->uris = data->uri_hits;
      hits->thread_data = data;
      
      bdk_threads_add_idle (search_thread_add_hits_idle, hits);
    }
  data->uri_hits = NULL;
}

static GStaticPrivate search_thread_data = G_STATIC_PRIVATE_INIT;

#ifdef HAVE_FTW_H
static int
search_visit_func (const char        *fpath,
		   const struct stat *sb,
		   int                typeflag,
		   struct FTW        *ftwbuf)
{
  SearchThreadData *data;
  bint i;
  const bchar *name; 
  bchar *lower_name;
  bchar *uri;
  bboolean hit;
  bboolean is_hidden;
  
  data = (SearchThreadData*)g_static_private_get (&search_thread_data);

  if (data->cancelled)
#ifdef HAVE_GNU_FTW
    return FTW_STOP;
#else
    return 1;
#endif /* HAVE_GNU_FTW */

  name = strrchr (fpath, '/');
  if (name)
    name++;
  else
    name = fpath;

  is_hidden = *name == '.';
	
  hit = FALSE;
  
  if (!is_hidden) 
    {
      lower_name = g_ascii_strdown (name, -1);
      
      hit = TRUE;
      for (i = 0; data->words[i] != NULL; i++) 
	{
	  if (strstr (lower_name, data->words[i]) == NULL) 
	    {
	      hit = FALSE;
	      break;
	    }
	}
      g_free (lower_name);
    }

  if (hit) 
    {
      uri = g_filename_to_uri (fpath, NULL, NULL);
      data->uri_hits = g_list_prepend (data->uri_hits, uri);
    }

  data->n_processed_files++;
  
  if (data->n_processed_files > BATCH_SIZE)
    send_batch (data);

#ifdef HAVE_GNU_FTW
  if (is_hidden)
    return FTW_SKIP_SUBTREE;
  else
    return FTW_CONTINUE;
#else
  return 0;
#endif /* HAVE_GNU_FTW */
}
#endif /* HAVE_FTW_H */

static bpointer 
search_thread_func (bpointer user_data)
{
#ifdef HAVE_FTW_H
  SearchThreadData *data;
  
  data = user_data;
  
  g_static_private_set (&search_thread_data, data, NULL);

  nftw (data->path, search_visit_func, 20,
#ifdef HAVE_GNU_FTW
        FTW_ACTIONRETVAL |
#endif
        FTW_PHYS);

  send_batch (data);
  
  bdk_threads_add_idle (search_thread_done_idle, data);
#endif /* HAVE_FTW_H */
  
  return NULL;
}

static void
btk_search_engine_simple_start (BtkSearchEngine *engine)
{
  BtkSearchEngineSimple *simple;
  SearchThreadData *data;
  
  simple = BTK_SEARCH_ENGINE_SIMPLE (engine);
  
  if (simple->priv->active_search != NULL)
    return;
  
  if (simple->priv->query == NULL)
    return;
	
  data = search_thread_data_new (simple, simple->priv->query);
  
  g_thread_create (search_thread_func, data, FALSE, NULL);
  
  simple->priv->active_search = data;
}

static void
btk_search_engine_simple_stop (BtkSearchEngine *engine)
{
  BtkSearchEngineSimple *simple;
  
  simple = BTK_SEARCH_ENGINE_SIMPLE (engine);
  
  if (simple->priv->active_search != NULL) 
    {
      simple->priv->active_search->cancelled = TRUE;
      simple->priv->active_search = NULL;
    }
}

static bboolean
btk_search_engine_simple_is_indexed (BtkSearchEngine *engine)
{
  return FALSE;
}

static void
btk_search_engine_simple_set_query (BtkSearchEngine *engine, 
				    BtkQuery        *query)
{
  BtkSearchEngineSimple *simple;
  
  simple = BTK_SEARCH_ENGINE_SIMPLE (engine);
  
  if (query)
    g_object_ref (query);

  if (simple->priv->query) 
    g_object_unref (simple->priv->query);

  simple->priv->query = query;
}

static void
_btk_search_engine_simple_class_init (BtkSearchEngineSimpleClass *class)
{
  BObjectClass *bobject_class;
  BtkSearchEngineClass *engine_class;
  
  bobject_class = B_OBJECT_CLASS (class);
  bobject_class->dispose = btk_search_engine_simple_dispose;
  
  engine_class = BTK_SEARCH_ENGINE_CLASS (class);
  engine_class->set_query = btk_search_engine_simple_set_query;
  engine_class->start = btk_search_engine_simple_start;
  engine_class->stop = btk_search_engine_simple_stop;
  engine_class->is_indexed = btk_search_engine_simple_is_indexed;

  g_type_class_add_private (bobject_class, sizeof (BtkSearchEngineSimplePrivate));
}

static void
_btk_search_engine_simple_init (BtkSearchEngineSimple *engine)
{
  engine->priv = B_TYPE_INSTANCE_GET_PRIVATE (engine, BTK_TYPE_SEARCH_ENGINE_SIMPLE, BtkSearchEngineSimplePrivate);
}

BtkSearchEngine *
_btk_search_engine_simple_new (void)
{
#ifdef HAVE_FTW_H
  return g_object_new (BTK_TYPE_SEARCH_ENGINE_SIMPLE, NULL);
#else
  return NULL;
#endif
}
