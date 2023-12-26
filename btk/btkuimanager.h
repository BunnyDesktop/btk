/*
 * BTK - The GIMP Toolkit
 * Copyright (C) 1998, 1999 Red Hat, Inc.
 * All rights reserved.
 *
 * This Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Bunny Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Author: James Henstridge <james@daa.com.au>
 *
 * Modified by the BTK+ Team and others 2003.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/.
 */

#ifndef __BTK_UI_MANAGER_H__
#define __BTK_UI_MANAGER_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkaccelgroup.h>
#include <btk/btkwidget.h>
#include <btk/btkaction.h>
#include <btk/btkactiongroup.h>

B_BEGIN_DECLS

#define BTK_TYPE_UI_MANAGER            (btk_ui_manager_get_type ())
#define BTK_UI_MANAGER(obj)            (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_UI_MANAGER, BtkUIManager))
#define BTK_UI_MANAGER_CLASS(klass)    (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_UI_MANAGER, BtkUIManagerClass))
#define BTK_IS_UI_MANAGER(obj)         (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_UI_MANAGER))
#define BTK_IS_UI_MANAGER_CLASS(klass) (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_UI_MANAGER))
#define BTK_UI_MANAGER_GET_CLASS(obj)  (B_TYPE_INSTANCE_GET_CLASS((obj), BTK_TYPE_UI_MANAGER, BtkUIManagerClass))

typedef struct _BtkUIManager      BtkUIManager;
typedef struct _BtkUIManagerClass BtkUIManagerClass;
typedef struct _BtkUIManagerPrivate BtkUIManagerPrivate;


struct _BtkUIManager {
  BObject parent;

  /*< private >*/

  BtkUIManagerPrivate *GSEAL (private_data);
};

struct _BtkUIManagerClass {
  BObjectClass parent_class;

  /* Signals */
  void (* add_widget)       (BtkUIManager *merge,
                             BtkWidget    *widget);
  void (* actions_changed)  (BtkUIManager *merge);
  void (* connect_proxy)    (BtkUIManager *merge,
			     BtkAction    *action,
			     BtkWidget    *proxy);
  void (* disconnect_proxy) (BtkUIManager *merge,
			     BtkAction    *action,
			     BtkWidget    *proxy);
  void (* pre_activate)     (BtkUIManager *merge,
			     BtkAction    *action);
  void (* post_activate)    (BtkUIManager *merge,
			     BtkAction    *action);

  /* Virtual functions */
  BtkWidget * (* get_widget) (BtkUIManager *manager,
                              const gchar  *path);
  BtkAction * (* get_action) (BtkUIManager *manager,
                              const gchar  *path);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
};

typedef enum {
  BTK_UI_MANAGER_AUTO              = 0,
  BTK_UI_MANAGER_MENUBAR           = 1 << 0,
  BTK_UI_MANAGER_MENU              = 1 << 1,
  BTK_UI_MANAGER_TOOLBAR           = 1 << 2,
  BTK_UI_MANAGER_PLACEHOLDER       = 1 << 3,
  BTK_UI_MANAGER_POPUP             = 1 << 4,
  BTK_UI_MANAGER_MENUITEM          = 1 << 5,
  BTK_UI_MANAGER_TOOLITEM          = 1 << 6,
  BTK_UI_MANAGER_SEPARATOR         = 1 << 7,
  BTK_UI_MANAGER_ACCELERATOR       = 1 << 8,
  BTK_UI_MANAGER_POPUP_WITH_ACCELS = 1 << 9
} BtkUIManagerItemType;

#ifdef G_OS_WIN32
/* Reserve old name for DLL ABI backward compatibility */
#define btk_ui_manager_add_ui_from_file btk_ui_manager_add_ui_from_file_utf8
#endif

GType          btk_ui_manager_get_type            (void) B_GNUC_CONST;
BtkUIManager  *btk_ui_manager_new                 (void);
void           btk_ui_manager_set_add_tearoffs    (BtkUIManager          *self,
						   gboolean               add_tearoffs);
gboolean       btk_ui_manager_get_add_tearoffs    (BtkUIManager          *self);
void           btk_ui_manager_insert_action_group (BtkUIManager          *self,
						   BtkActionGroup        *action_group,
						   gint                   pos);
void           btk_ui_manager_remove_action_group (BtkUIManager          *self,
						   BtkActionGroup        *action_group);
GList         *btk_ui_manager_get_action_groups   (BtkUIManager          *self);
BtkAccelGroup *btk_ui_manager_get_accel_group     (BtkUIManager          *self);
BtkWidget     *btk_ui_manager_get_widget          (BtkUIManager          *self,
						   const gchar           *path);
GSList        *btk_ui_manager_get_toplevels       (BtkUIManager          *self,
						   BtkUIManagerItemType   types);
BtkAction     *btk_ui_manager_get_action          (BtkUIManager          *self,
						   const gchar           *path);
guint          btk_ui_manager_add_ui_from_string  (BtkUIManager          *self,
						   const gchar           *buffer,
						   gssize                 length,
						   GError               **error);
guint          btk_ui_manager_add_ui_from_file    (BtkUIManager          *self,
						   const gchar           *filename,
						   GError               **error);
void           btk_ui_manager_add_ui              (BtkUIManager          *self,
						   guint                  merge_id,
						   const gchar           *path,
						   const gchar           *name,
						   const gchar           *action,
						   BtkUIManagerItemType   type,
						   gboolean               top);
void           btk_ui_manager_remove_ui           (BtkUIManager          *self,
						   guint                  merge_id);
gchar         *btk_ui_manager_get_ui              (BtkUIManager          *self);
void           btk_ui_manager_ensure_update       (BtkUIManager          *self);
guint          btk_ui_manager_new_merge_id        (BtkUIManager          *self);

B_END_DECLS

#endif /* __BTK_UI_MANAGER_H__ */
