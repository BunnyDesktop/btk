/* BTK - The GIMP Toolkit
 * btkrecentmanager.h: a manager for the recently used resources
 *
 * Copyright (C) 2006 Emmanuele Bassi
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 */

#ifndef __BTK_RECENT_MANAGER_H__
#define __BTK_RECENT_MANAGER_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <bdk-pixbuf/bdk-pixbuf.h>
#include <bdk/bdk.h>
#include <time.h>

B_BEGIN_DECLS

#define BTK_TYPE_RECENT_INFO			(btk_recent_info_get_type ())

#define BTK_TYPE_RECENT_MANAGER			(btk_recent_manager_get_type ())
#define BTK_RECENT_MANAGER(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_RECENT_MANAGER, BtkRecentManager))
#define BTK_IS_RECENT_MANAGER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_RECENT_MANAGER))
#define BTK_RECENT_MANAGER_CLASS(klass) 	(G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_RECENT_MANAGER, BtkRecentManagerClass))
#define BTK_IS_RECENT_MANAGER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_RECENT_MANAGER))
#define BTK_RECENT_MANAGER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_RECENT_MANAGER, BtkRecentManagerClass))

typedef struct _BtkRecentInfo		BtkRecentInfo;
typedef struct _BtkRecentData		BtkRecentData;
typedef struct _BtkRecentManager	BtkRecentManager;
typedef struct _BtkRecentManagerClass	BtkRecentManagerClass;
typedef struct _BtkRecentManagerPrivate BtkRecentManagerPrivate;

/**
 * BtkRecentData:
 * @display_name: a UTF-8 encoded string, containing the name of the recently
 *   used resource to be displayed, or %NULL;
 * @description: a UTF-8 encoded string, containing a short description of
 *   the resource, or %NULL;
 * @mime_type: the MIME type of the resource;
 * @app_name: the name of the application that is registering this recently
 *   used resource;
 * @app_exec: command line used to launch this resource; may contain the
 *   "&percnt;f" and "&percnt;u" escape characters which will be expanded
 *   to the resource file path and URI respectively when the command line
 *   is retrieved;
 * @groups: a vector of strings containing groups names;
 * @is_private: whether this resource should be displayed only by the
 *   applications that have registered it or not.
 *
 * Meta-data to be passed to btk_recent_manager_add_full() when
 * registering a recently used resource.
 **/
struct _BtkRecentData
{
  gchar *display_name;
  gchar *description;

  gchar *mime_type;

  gchar *app_name;
  gchar *app_exec;

  gchar **groups;

  gboolean is_private;
};

struct _BtkRecentManager
{
  /*< private >*/
  GObject parent_instance;

  BtkRecentManagerPrivate *GSEAL (priv);
};

struct _BtkRecentManagerClass
{
  /*< private >*/
  GObjectClass parent_class;

  void (*changed) (BtkRecentManager *manager);

  /* padding for future expansion */
  void (*_btk_recent1) (void);
  void (*_btk_recent2) (void);
  void (*_btk_recent3) (void);
  void (*_btk_recent4) (void);
};

/**
 * BtkRecentManagerError:
 * @BTK_RECENT_MANAGER_ERROR_NOT_FOUND: the URI specified does not exists in
 *   the recently used resources list.
 * @BTK_RECENT_MANAGER_ERROR_INVALID_URI: the URI specified is not valid.
 * @BTK_RECENT_MANAGER_ERROR_INVALID_ENCODING: the supplied string is not
 *   UTF-8 encoded.
 * @BTK_RECENT_MANAGER_ERROR_NOT_REGISTERED: no application has registered
 *   the specified item.
 * @BTK_RECENT_MANAGER_ERROR_READ: failure while reading the recently used
 *   resources file.
 * @BTK_RECENT_MANAGER_ERROR_WRITE: failure while writing the recently used
 *   resources file.
 * @BTK_RECENT_MANAGER_ERROR_UNKNOWN: unspecified error.
 *
 * Error codes for BtkRecentManager operations
 **/
typedef enum
{
  BTK_RECENT_MANAGER_ERROR_NOT_FOUND,
  BTK_RECENT_MANAGER_ERROR_INVALID_URI,
  BTK_RECENT_MANAGER_ERROR_INVALID_ENCODING,
  BTK_RECENT_MANAGER_ERROR_NOT_REGISTERED,
  BTK_RECENT_MANAGER_ERROR_READ,
  BTK_RECENT_MANAGER_ERROR_WRITE,
  BTK_RECENT_MANAGER_ERROR_UNKNOWN
} BtkRecentManagerError;

#define BTK_RECENT_MANAGER_ERROR	(btk_recent_manager_error_quark ())
GQuark 	btk_recent_manager_error_quark (void);


GType 		  btk_recent_manager_get_type       (void) B_GNUC_CONST;

BtkRecentManager *btk_recent_manager_new            (void);
BtkRecentManager *btk_recent_manager_get_default    (void);

#ifndef BTK_DISABLE_DEPRECATED
BtkRecentManager *btk_recent_manager_get_for_screen (BdkScreen            *screen);
void              btk_recent_manager_set_screen     (BtkRecentManager     *manager,
						     BdkScreen            *screen);
#endif

gboolean          btk_recent_manager_add_item       (BtkRecentManager     *manager,
						     const gchar          *uri);
gboolean          btk_recent_manager_add_full       (BtkRecentManager     *manager,
						     const gchar          *uri,
						     const BtkRecentData  *recent_data);
gboolean          btk_recent_manager_remove_item    (BtkRecentManager     *manager,
						     const gchar          *uri,
						     GError              **error);
BtkRecentInfo *   btk_recent_manager_lookup_item    (BtkRecentManager     *manager,
						     const gchar          *uri,
						     GError              **error);
gboolean          btk_recent_manager_has_item       (BtkRecentManager     *manager,
						     const gchar          *uri);
gboolean          btk_recent_manager_move_item      (BtkRecentManager     *manager,
						     const gchar          *uri,
						     const gchar          *new_uri,
						     GError              **error);
void              btk_recent_manager_set_limit      (BtkRecentManager     *manager,
						     gint                  limit);
gint              btk_recent_manager_get_limit      (BtkRecentManager     *manager);
GList *           btk_recent_manager_get_items      (BtkRecentManager     *manager);
gint              btk_recent_manager_purge_items    (BtkRecentManager     *manager,
						     GError              **error);


GType	              btk_recent_info_get_type             (void) B_GNUC_CONST;

BtkRecentInfo *       btk_recent_info_ref                  (BtkRecentInfo  *info);
void                  btk_recent_info_unref                (BtkRecentInfo  *info);

const gchar *         btk_recent_info_get_uri              (BtkRecentInfo  *info);
const gchar *         btk_recent_info_get_display_name     (BtkRecentInfo  *info);
const gchar *         btk_recent_info_get_description      (BtkRecentInfo  *info);
const gchar *         btk_recent_info_get_mime_type        (BtkRecentInfo  *info);
time_t                btk_recent_info_get_added            (BtkRecentInfo  *info);
time_t                btk_recent_info_get_modified         (BtkRecentInfo  *info);
time_t                btk_recent_info_get_visited          (BtkRecentInfo  *info);
gboolean              btk_recent_info_get_private_hint     (BtkRecentInfo  *info);
gboolean              btk_recent_info_get_application_info (BtkRecentInfo  *info,
							    const gchar    *app_name,
							    const gchar   **app_exec,
							    guint          *count,
							    time_t         *time_);
gchar **              btk_recent_info_get_applications     (BtkRecentInfo  *info,
							    gsize          *length) B_GNUC_MALLOC;
gchar *               btk_recent_info_last_application     (BtkRecentInfo  *info) B_GNUC_MALLOC;
gboolean              btk_recent_info_has_application      (BtkRecentInfo  *info,
							    const gchar    *app_name);
gchar **              btk_recent_info_get_groups           (BtkRecentInfo  *info,
							    gsize          *length) B_GNUC_MALLOC;
gboolean              btk_recent_info_has_group            (BtkRecentInfo  *info,
							    const gchar    *group_name);
BdkPixbuf *           btk_recent_info_get_icon             (BtkRecentInfo  *info,
							    gint            size);
gchar *               btk_recent_info_get_short_name       (BtkRecentInfo  *info) B_GNUC_MALLOC;
gchar *               btk_recent_info_get_uri_display      (BtkRecentInfo  *info) B_GNUC_MALLOC;
gint                  btk_recent_info_get_age              (BtkRecentInfo  *info);
gboolean              btk_recent_info_is_local             (BtkRecentInfo  *info);
gboolean              btk_recent_info_exists               (BtkRecentInfo  *info);
gboolean              btk_recent_info_match                (BtkRecentInfo  *info_a,
							    BtkRecentInfo  *info_b);

/* private */
void _btk_recent_manager_sync (void);

B_END_DECLS

#endif /* __BTK_RECENT_MANAGER_H__ */
