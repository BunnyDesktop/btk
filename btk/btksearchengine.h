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
 * Based on nautilus-search-engine.h
 */

#ifndef __BTK_SEARCH_ENGINE_H__
#define __BTK_SEARCH_ENGINE_H__

#include "btkquery.h"

B_BEGIN_DECLS

#define BTK_TYPE_SEARCH_ENGINE		(_btk_search_engine_get_type ())
#define BTK_SEARCH_ENGINE(obj)		(B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_SEARCH_ENGINE, BtkSearchEngine))
#define BTK_SEARCH_ENGINE_CLASS(klass)	(B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_SEARCH_ENGINE, BtkSearchEngineClass))
#define BTK_IS_SEARCH_ENGINE(obj)		(B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_SEARCH_ENGINE))
#define BTK_IS_SEARCH_ENGINE_CLASS(klass)	(B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_SEARCH_ENGINE))
#define BTK_SEARCH_ENGINE_GET_CLASS(obj)    (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_SEARCH_ENGINE, BtkSearchEngineClass))

typedef struct _BtkSearchEngine BtkSearchEngine;
typedef struct _BtkSearchEngineClass BtkSearchEngineClass;
typedef struct _BtkSearchEnginePrivate BtkSearchEnginePrivate;

struct _BtkSearchEngine 
{
  BObject parent;

  BtkSearchEnginePrivate *priv;
};

struct _BtkSearchEngineClass 
{
  BObjectClass parent_class;
  
  /* VTable */
  void     (*set_query)       (BtkSearchEngine *engine, 
			       BtkQuery        *query);
  void     (*start)           (BtkSearchEngine *engine);
  void     (*stop)            (BtkSearchEngine *engine);
  bboolean (*is_indexed)      (BtkSearchEngine *engine);
  
  /* Signals */
  void     (*hits_added)      (BtkSearchEngine *engine, 
			       GList           *hits);
  void     (*hits_subtracted) (BtkSearchEngine *engine, 
			       GList           *hits);
  void     (*finished)        (BtkSearchEngine *engine);
  void     (*error)           (BtkSearchEngine *engine, 
			       const bchar     *error_message);
};

GType            _btk_search_engine_get_type        (void);
bboolean         _btk_search_engine_enabled         (void);

BtkSearchEngine* _btk_search_engine_new             (void);

void             _btk_search_engine_set_query       (BtkSearchEngine *engine, 
                                                     BtkQuery        *query);
void	         _btk_search_engine_start           (BtkSearchEngine *engine);
void	         _btk_search_engine_stop            (BtkSearchEngine *engine);
bboolean         _btk_search_engine_is_indexed      (BtkSearchEngine *engine);

void	         _btk_search_engine_hits_added      (BtkSearchEngine *engine, 
						     GList           *hits);
void	         _btk_search_engine_hits_subtracted (BtkSearchEngine *engine, 
						     GList           *hits);
void	         _btk_search_engine_finished        (BtkSearchEngine *engine);
void	         _btk_search_engine_error           (BtkSearchEngine *engine, 
						     const bchar     *error_message);

B_END_DECLS

#endif /* __BTK_SEARCH_ENGINE_H__ */
