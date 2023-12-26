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

#ifndef __BTK_ACTION_H__
#define __BTK_ACTION_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkwidget.h>

B_BEGIN_DECLS

#define BTK_TYPE_ACTION            (btk_action_get_type ())
#define BTK_ACTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_ACTION, BtkAction))
#define BTK_ACTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_ACTION, BtkActionClass))
#define BTK_IS_ACTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_ACTION))
#define BTK_IS_ACTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_ACTION))
#define BTK_ACTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), BTK_TYPE_ACTION, BtkActionClass))

typedef struct _BtkAction      BtkAction;
typedef struct _BtkActionClass BtkActionClass;
typedef struct _BtkActionPrivate BtkActionPrivate;

struct _BtkAction
{
  GObject object;

  /*< private >*/

  BtkActionPrivate *GSEAL (private_data);
};

struct _BtkActionClass
{
  GObjectClass parent_class;

  /* activation signal */
  void       (* activate)           (BtkAction    *action);

  GType      menu_item_type;
  GType      toolbar_item_type;

  /* widget creation routines (not signals) */
  BtkWidget *(* create_menu_item)   (BtkAction *action);
  BtkWidget *(* create_tool_item)   (BtkAction *action);
  void       (* connect_proxy)      (BtkAction *action,
				     BtkWidget *proxy);
  void       (* disconnect_proxy)   (BtkAction *action,
				     BtkWidget *proxy);

  BtkWidget *(* create_menu)        (BtkAction *action);

  /* Padding for future expansion */
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};

GType        btk_action_get_type               (void) B_GNUC_CONST;
BtkAction   *btk_action_new                    (const gchar *name,
						const gchar *label,
						const gchar *tooltip,
						const gchar *stock_id);
const gchar* btk_action_get_name               (BtkAction     *action);
gboolean     btk_action_is_sensitive           (BtkAction     *action);
gboolean     btk_action_get_sensitive          (BtkAction     *action);
void         btk_action_set_sensitive          (BtkAction     *action,
						gboolean       sensitive);
gboolean     btk_action_is_visible             (BtkAction     *action);
gboolean     btk_action_get_visible            (BtkAction     *action);
void         btk_action_set_visible            (BtkAction     *action,
						gboolean       visible);
void         btk_action_activate               (BtkAction     *action);
BtkWidget *  btk_action_create_icon            (BtkAction     *action,
						BtkIconSize    icon_size);
BtkWidget *  btk_action_create_menu_item       (BtkAction     *action);
BtkWidget *  btk_action_create_tool_item       (BtkAction     *action);
BtkWidget *  btk_action_create_menu            (BtkAction     *action);
GSList *     btk_action_get_proxies            (BtkAction     *action);
void         btk_action_connect_accelerator    (BtkAction     *action);
void         btk_action_disconnect_accelerator (BtkAction     *action);
const gchar *btk_action_get_accel_path         (BtkAction     *action);
GClosure    *btk_action_get_accel_closure      (BtkAction     *action);

#ifndef BTK_DISABLE_DEPRECATED
BtkAction   *btk_widget_get_action             (BtkWidget     *widget);
void         btk_action_connect_proxy          (BtkAction     *action,
						BtkWidget     *proxy);
void         btk_action_disconnect_proxy       (BtkAction     *action,
						BtkWidget     *proxy);
void         btk_action_block_activate_from    (BtkAction     *action,
						BtkWidget     *proxy);
void         btk_action_unblock_activate_from  (BtkAction     *action,
						BtkWidget     *proxy);
#endif /* BTK_DISABLE_DEPRECATED */
void         btk_action_block_activate         (BtkAction     *action);
void         btk_action_unblock_activate       (BtkAction     *action);


void         _btk_action_add_to_proxy_list     (BtkAction     *action,
						BtkWidget     *proxy);
void         _btk_action_remove_from_proxy_list(BtkAction     *action,
						BtkWidget     *proxy);

/* protected ... for use by child actions */
void         _btk_action_emit_activate         (BtkAction     *action);

/* protected ... for use by action groups */
void         btk_action_set_accel_path         (BtkAction     *action,
						const gchar   *accel_path);
void         btk_action_set_accel_group        (BtkAction     *action,
						BtkAccelGroup *accel_group);
void         _btk_action_sync_menu_visible     (BtkAction     *action,
						BtkWidget     *proxy,
						gboolean       empty);

void                  btk_action_set_label              (BtkAction   *action,
                                                         const gchar *label);
const gchar *         btk_action_get_label              (BtkAction   *action);
void                  btk_action_set_short_label        (BtkAction   *action,
                                                         const gchar *short_label);
const gchar *         btk_action_get_short_label        (BtkAction   *action);
void                  btk_action_set_tooltip            (BtkAction   *action,
                                                         const gchar *tooltip);
const gchar *         btk_action_get_tooltip            (BtkAction   *action);
void                  btk_action_set_stock_id           (BtkAction   *action,
                                                         const gchar *stock_id);
const gchar *         btk_action_get_stock_id           (BtkAction   *action);
void                  btk_action_set_gicon              (BtkAction   *action,
                                                         GIcon       *icon);
GIcon                *btk_action_get_gicon              (BtkAction   *action);
void                  btk_action_set_icon_name          (BtkAction   *action,
                                                         const gchar *icon_name);
const gchar *         btk_action_get_icon_name          (BtkAction   *action);
void                  btk_action_set_visible_horizontal (BtkAction   *action,
                                                         gboolean     visible_horizontal);
gboolean              btk_action_get_visible_horizontal (BtkAction   *action);
void                  btk_action_set_visible_vertical   (BtkAction   *action,
                                                         gboolean     visible_vertical);
gboolean              btk_action_get_visible_vertical   (BtkAction   *action);
void                  btk_action_set_is_important       (BtkAction   *action,
                                                         gboolean     is_important);
gboolean              btk_action_get_is_important       (BtkAction   *action);
void                  btk_action_set_always_show_image  (BtkAction   *action,
                                                         gboolean     always_show);
gboolean              btk_action_get_always_show_image  (BtkAction   *action);


B_END_DECLS

#endif  /* __BTK_ACTION_H__ */
