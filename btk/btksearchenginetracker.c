/*
 * Copyright (C) 2009-2011 Nokia <ivan.frade@nokia.com>
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
 * Authors: Jürg Billeter <juerg.billeter@codethink.co.uk>
 *          Martyn Russell <martyn@lanedo.com>
 *
 * Based on nautilus-search-engine-tracker.c
 */

#include "config.h"

#include <string.h>

#include <bunnyio/bunnyio.h>
#include <bmodule.h>
#include <bdk/bdk.h>
#include <btk/btk.h>

#include "btksearchenginetracker.h"

#define DBUS_SERVICE_RESOURCES   "org.freedesktop.Tracker1"
#define DBUS_PATH_RESOURCES      "/org/freedesktop/Tracker1/Resources"
#define DBUS_INTERFACE_RESOURCES "org.freedesktop.Tracker1.Resources"

#define DBUS_SERVICE_STATUS      "org.freedesktop.Tracker1"
#define DBUS_PATH_STATUS         "/org/freedesktop/Tracker1/Status"
#define DBUS_INTERFACE_STATUS    "org.freedesktop.Tracker1.Status"

/* Time in second to wait for service before deciding it's not available */
#define WAIT_TIMEOUT_SECONDS 1

/* Time in second to wait for query results to come back */
#define QUERY_TIMEOUT_SECONDS 10

/* If defined, we use fts:match, this has to be enabled in Tracker to
 * work which it usually is. The alternative is to undefine it and
 * use filename matching instead. This doesn't use the content of the
 * file however.
 */
#undef FTS_MATCHING

/*
 * BtkSearchEngineTracker object
 */
struct _BtkSearchEngineTrackerPrivate
{
  GDBusConnection *connection;
  GCancellable *cancellable;
  BtkQuery *query;
  bboolean query_pending;
};

G_DEFINE_TYPE (BtkSearchEngineTracker, _btk_search_engine_tracker, BTK_TYPE_SEARCH_ENGINE);

static void
finalize (BObject *object)
{
  BtkSearchEngineTracker *tracker;

  g_debug ("Finalizing BtkSearchEngineTracker");

  tracker = BTK_SEARCH_ENGINE_TRACKER (object);

  if (tracker->priv->cancellable)
    {
      g_cancellable_cancel (tracker->priv->cancellable);
      g_object_unref (tracker->priv->cancellable);
      tracker->priv->cancellable = NULL;
    }

  if (tracker->priv->query)
    {
      g_object_unref (tracker->priv->query);
      tracker->priv->query = NULL;
    }

  if (tracker->priv->connection)
    {
      g_object_unref (tracker->priv->connection);
      tracker->priv->connection = NULL;
    }

  B_OBJECT_CLASS (_btk_search_engine_tracker_parent_class)->finalize (object);
}

static GDBusConnection *
get_connection (void)
{
  GDBusConnection *connection;
  GError *error = NULL;
  GVariant *reply;

  /* Normally I hate sync calls with UIs, but we need to return NULL
   * or a BtkSearchEngine as a result of this function.
   */
  connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);

  if (error)
    {
      g_debug ("Couldn't connect to D-Bus session bus, %s", error->message);
      g_error_free (error);
      return NULL;
    }

  /* If connection is set, we know it worked. */
  g_debug ("Finding out if Tracker is available via D-Bus...");

  /* We only wait 1 second max, we expect it to be very fast. If we
   * don't get a response by then, clearly we're replaying a journal
   * or cleaning up the DB internally. Either way, services is not
   * available.
   *
   * We use the sync call here because we don't expect to be waiting
   * long enough to block UI painting.
   */
  reply = g_dbus_connection_call_sync (connection,
                                       DBUS_SERVICE_STATUS,
                                       DBUS_PATH_STATUS,
                                       DBUS_INTERFACE_STATUS,
                                       "Wait",
                                       NULL,
                                       NULL,
                                       G_DBUS_CALL_FLAGS_NONE,
                                       WAIT_TIMEOUT_SECONDS * 1000,
                                       NULL,
                                       &error);

  if (error)
    {
      g_debug ("Tracker is not available, %s", error->message);
      g_error_free (error);
      g_object_unref (connection);
      return NULL;
    }

  g_variant_unref (reply);

  g_debug ("Tracker is ready");

  return connection;
}

static void
get_query_results (BtkSearchEngineTracker *engine,
                   const bchar            *sparql,
                   GAsyncReadyCallback     callback,
                   bpointer                user_data)
{
  g_dbus_connection_call (engine->priv->connection,
                          DBUS_SERVICE_RESOURCES,
                          DBUS_PATH_RESOURCES,
                          DBUS_INTERFACE_RESOURCES,
                          "SparqlQuery",
                          g_variant_new ("(s)", sparql),
                          NULL,
                          G_DBUS_CALL_FLAGS_NONE,
                          QUERY_TIMEOUT_SECONDS * 1000,
                          engine->priv->cancellable,
                          callback,
                          user_data);
}

/* Stolen from libtracker-common */
static GList *
string_list_to_gslist (bchar **strv)
{
  GList *list;
  bsize i;

  list = NULL;

  for (i = 0; strv[i]; i++)
    list = g_list_prepend (list, g_strdup (strv[i]));

  return g_list_reverse (list);
}

/* Stolen from libtracker-sparql */
static bchar *
sparql_escape_string (const bchar *literal)
{
  GString *str;
  const bchar *p;

  g_return_val_if_fail (literal != NULL, NULL);

  str = g_string_new ("");
  p = literal;

  while (TRUE)
     {
      bsize len;

      if (!((*p) != '\0'))
        break;

      len = strcspn ((const bchar *) p, "\t\n\r\b\f\"\\");
      g_string_append_len (str, (const bchar *) p, (bssize) ((blong) len));
      p = p + len;

      switch (*p)
        {
        case '\t':
          g_string_append (str, "\\t");
          break;
        case '\n':
          g_string_append (str, "\\n");
          break;
        case '\r':
          g_string_append (str, "\\r");
          break;
        case '\b':
          g_string_append (str, "\\b");
          break;
        case '\f':
          g_string_append (str, "\\f");
          break;
        case '"':
          g_string_append (str, "\\\"");
          break;
        case '\\':
          g_string_append (str, "\\\\");
          break;
        default:
          continue;
        }

      p++;
     }
  return g_string_free (str, FALSE);
 }

static void
sparql_append_string_literal (GString     *sparql,
                              const bchar *str)
{
  bchar *s;

  s = sparql_escape_string (str);

  g_string_append_c (sparql, '"');
  g_string_append (sparql, s);
  g_string_append_c (sparql, '"');

  g_free (s);
}

static void
sparql_append_string_literal_lower_case (GString     *sparql,
                                         const bchar *str)
{
  bchar *s;

  s = g_utf8_strdown (str, -1);
  sparql_append_string_literal (sparql, s);
  g_free (s);
}

static void
query_callback (BObject      *object,
                GAsyncResult *res,
                bpointer      user_data)
{
  BtkSearchEngineTracker *tracker;
  GList *hits;
  GVariant *reply;
  GVariant *r;
  GVariantIter iter;
  bchar **result;
  GError *error = NULL;
  bint i, n;

  bdk_threads_enter ();

  tracker = BTK_SEARCH_ENGINE_TRACKER (user_data);

  tracker->priv->query_pending = FALSE;

  reply = g_dbus_connection_call_finish (tracker->priv->connection, res, &error);
  if (error)
    {
      _btk_search_engine_error (BTK_SEARCH_ENGINE (tracker), error->message);
      g_error_free (error);
      bdk_threads_leave ();
      return;
    }

  if (!reply)
    {
      _btk_search_engine_finished (BTK_SEARCH_ENGINE (tracker));
      bdk_threads_leave ();
      return;
    }

  r = g_variant_get_child_value (reply, 0);
  g_variant_iter_init (&iter, r);
  n = g_variant_iter_n_children (&iter);
  result = g_new0 (bchar *, n + 1);
  for (i = 0; i < n; i++)
    {
      GVariant *v;
      const bchar **strv;

      v = g_variant_iter_next_value (&iter);
      strv = g_variant_get_strv (v, NULL);
      result[i] = (bchar*)strv[0];
      g_free (strv);
    }

  /* We iterate result by result, not n at a time. */
  hits = string_list_to_gslist (result);
  _btk_search_engine_hits_added (BTK_SEARCH_ENGINE (tracker), hits);
  _btk_search_engine_finished (BTK_SEARCH_ENGINE (tracker));
  g_list_free (hits);
  g_free (result);
  g_variant_unref (reply);
  g_variant_unref (r);

  bdk_threads_leave ();
}

static void
btk_search_engine_tracker_start (BtkSearchEngine *engine)
{
  BtkSearchEngineTracker *tracker;
  bchar *search_text;
#ifdef FTS_MATCHING
  bchar *location_uri;
#endif
  GString *sparql;

  tracker = BTK_SEARCH_ENGINE_TRACKER (engine);

  if (tracker->priv->query_pending)
    {
      g_debug ("Attempt to start a new search while one is pending, doing nothing");
      return;
    }

  if (tracker->priv->query == NULL)
    {
      g_debug ("Attempt to start a new search with no BtkQuery, doing nothing");
      return;
    }

  search_text = _btk_query_get_text (tracker->priv->query);

#ifdef FTS_MATCHING
  location_uri = _btk_query_get_location (tracker->priv->query);
  /* Using FTS: */
  sparql = g_string_new ("SELECT nie:url(?urn) "
                         "WHERE {"
                         "  ?urn a nfo:FileDataObject ;"
                         "  tracker:available true ; "
                         "  fts:match ");
  sparql_append_string_literal (sparql, search_text);

  if (location_uri)
    {
      g_string_append (sparql, " . FILTER (fn:starts-with(nie:url(?urn),");
      sparql_append_string_literal (sparql, location_uri);
      g_string_append (sparql, "))");
    }

  g_string_append (sparql, " } ORDER BY DESC(fts:rank(?urn)) ASC(nie:url(?urn))");
#else  /* FTS_MATCHING */
  /* Using filename matching: */
  sparql = g_string_new ("SELECT nie:url(?urn) "
                         "WHERE {"
                         "  ?urn a nfo:FileDataObject ;"
                         "    tracker:available true ."
                         "  FILTER (fn:contains(fn:lower-case(nfo:fileName(?urn)),");
  sparql_append_string_literal_lower_case (sparql, search_text);

  g_string_append (sparql,
                   "))"
                   "} ORDER BY DESC(nie:url(?urn)) DESC(nfo:fileName(?urn))");
#endif /* FTS_MATCHING */

  tracker->priv->query_pending = TRUE;

  get_query_results (tracker, sparql->str, query_callback, tracker);

  g_string_free (sparql, TRUE);
  g_free (search_text);
}

static void
btk_search_engine_tracker_stop (BtkSearchEngine *engine)
{
  BtkSearchEngineTracker *tracker;

  tracker = BTK_SEARCH_ENGINE_TRACKER (engine);

  if (tracker->priv->query && tracker->priv->query_pending)
    {
      g_cancellable_cancel (tracker->priv->cancellable);
      tracker->priv->query_pending = FALSE;
    }
}

static bboolean
btk_search_engine_tracker_is_indexed (BtkSearchEngine *engine)
{
  return TRUE;
}

static void
btk_search_engine_tracker_set_query (BtkSearchEngine *engine,
                                     BtkQuery        *query)
{
  BtkSearchEngineTracker *tracker;

  tracker = BTK_SEARCH_ENGINE_TRACKER (engine);

  if (query)
    g_object_ref (query);

  if (tracker->priv->query)
    g_object_unref (tracker->priv->query);

  tracker->priv->query = query;
}

static void
_btk_search_engine_tracker_class_init (BtkSearchEngineTrackerClass *class)
{
  BObjectClass *bobject_class;
  BtkSearchEngineClass *engine_class;

  bobject_class = B_OBJECT_CLASS (class);
  bobject_class->finalize = finalize;

  engine_class = BTK_SEARCH_ENGINE_CLASS (class);
  engine_class->set_query = btk_search_engine_tracker_set_query;
  engine_class->start = btk_search_engine_tracker_start;
  engine_class->stop = btk_search_engine_tracker_stop;
  engine_class->is_indexed = btk_search_engine_tracker_is_indexed;

  g_type_class_add_private (bobject_class,
                            sizeof (BtkSearchEngineTrackerPrivate));
}

static void
_btk_search_engine_tracker_init (BtkSearchEngineTracker *engine)
{
  engine->priv = B_TYPE_INSTANCE_GET_PRIVATE (engine,
                                              BTK_TYPE_SEARCH_ENGINE_TRACKER,
                                              BtkSearchEngineTrackerPrivate);
}


BtkSearchEngine *
_btk_search_engine_tracker_new (void)
{
  BtkSearchEngineTracker *engine;
  GDBusConnection *connection;

  g_debug ("--");

  connection = get_connection ();
  if (!connection)
    return NULL;

  g_debug ("Creating BtkSearchEngineTracker...");

  engine = g_object_new (BTK_TYPE_SEARCH_ENGINE_TRACKER, NULL);

  engine->priv->connection = connection;
  engine->priv->cancellable = g_cancellable_new ();
  engine->priv->query_pending = FALSE;

  return BTK_SEARCH_ENGINE (engine);
}
