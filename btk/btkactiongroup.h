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

#ifndef __BTK_ACTION_GROUP_H__
#define __BTK_ACTION_GROUP_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkaction.h>
#include <btk/btktypeutils.h> /* for BtkTranslateFunc */

B_BEGIN_DECLS

#define BTK_TYPE_ACTION_GROUP              (btk_action_group_get_type ())
#define BTK_ACTION_GROUP(obj)              (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_ACTION_GROUP, BtkActionGroup))
#define BTK_ACTION_GROUP_CLASS(vtable)     (B_TYPE_CHECK_CLASS_CAST ((vtable), BTK_TYPE_ACTION_GROUP, BtkActionGroupClass))
#define BTK_IS_ACTION_GROUP(obj)           (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_ACTION_GROUP))
#define BTK_IS_ACTION_GROUP_CLASS(vtable)  (B_TYPE_CHECK_CLASS_TYPE ((vtable), BTK_TYPE_ACTION_GROUP))
#define BTK_ACTION_GROUP_GET_CLASS(inst)   (B_TYPE_INSTANCE_GET_CLASS ((inst), BTK_TYPE_ACTION_GROUP, BtkActionGroupClass))

typedef struct _BtkActionGroup        BtkActionGroup;
typedef struct _BtkActionGroupPrivate BtkActionGroupPrivate;
typedef struct _BtkActionGroupClass   BtkActionGroupClass;
typedef struct _BtkActionEntry        BtkActionEntry;
typedef struct _BtkToggleActionEntry  BtkToggleActionEntry;
typedef struct _BtkRadioActionEntry   BtkRadioActionEntry;

struct _BtkActionGroup
{
  BObject parent;

  /*< private >*/

  BtkActionGroupPrivate *GSEAL (private_data);
};

struct _BtkActionGroupClass
{
  BObjectClass parent_class;

  BtkAction *(* get_action) (BtkActionGroup *action_group,
                             const bchar    *action_name);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};

struct _BtkActionEntry 
{
  const bchar     *name;
  const bchar     *stock_id;
  const bchar     *label;
  const bchar     *accelerator;
  const bchar     *tooltip;
  GCallback  callback;
};

struct _BtkToggleActionEntry 
{
  const bchar     *name;
  const bchar     *stock_id;
  const bchar     *label;
  const bchar     *accelerator;
  const bchar     *tooltip;
  GCallback  callback;
  bboolean   is_active;
};

struct _BtkRadioActionEntry 
{
  const bchar *name;
  const bchar *stock_id;
  const bchar *label;
  const bchar *accelerator;
  const bchar *tooltip;
  bint   value; 
};

GType           btk_action_group_get_type                (void) B_GNUC_CONST;
BtkActionGroup *btk_action_group_new                     (const bchar                *name);
const bchar *btk_action_group_get_name          (BtkActionGroup             *action_group);
bboolean        btk_action_group_get_sensitive           (BtkActionGroup             *action_group);
void            btk_action_group_set_sensitive           (BtkActionGroup             *action_group,
							  bboolean                    sensitive);
bboolean        btk_action_group_get_visible             (BtkActionGroup             *action_group);
void            btk_action_group_set_visible             (BtkActionGroup             *action_group,
							  bboolean                    visible);
BtkAction      *btk_action_group_get_action              (BtkActionGroup             *action_group,
							  const bchar                *action_name);
GList          *btk_action_group_list_actions            (BtkActionGroup             *action_group);
void            btk_action_group_add_action              (BtkActionGroup             *action_group,
							  BtkAction                  *action);
void            btk_action_group_add_action_with_accel   (BtkActionGroup             *action_group,
							  BtkAction                  *action,
							  const bchar                *accelerator);
void            btk_action_group_remove_action           (BtkActionGroup             *action_group,
							  BtkAction                  *action);
void            btk_action_group_add_actions             (BtkActionGroup             *action_group,
							  const BtkActionEntry       *entries,
							  buint                       n_entries,
							  bpointer                    user_data);
void            btk_action_group_add_toggle_actions      (BtkActionGroup             *action_group,
							  const BtkToggleActionEntry *entries,
							  buint                       n_entries,
							  bpointer                    user_data);
void            btk_action_group_add_radio_actions       (BtkActionGroup             *action_group,
							  const BtkRadioActionEntry  *entries,
							  buint                       n_entries,
							  bint                        value,
							  GCallback                   on_change,
							  bpointer                    user_data);
void            btk_action_group_add_actions_full        (BtkActionGroup             *action_group,
							  const BtkActionEntry       *entries,
							  buint                       n_entries,
							  bpointer                    user_data,
							  GDestroyNotify              destroy);
void            btk_action_group_add_toggle_actions_full (BtkActionGroup             *action_group,
							  const BtkToggleActionEntry *entries,
							  buint                       n_entries,
							  bpointer                    user_data,
							  GDestroyNotify              destroy);
void            btk_action_group_add_radio_actions_full  (BtkActionGroup             *action_group,
							  const BtkRadioActionEntry  *entries,
							  buint                       n_entries,
							  bint                        value,
							  GCallback                   on_change,
							  bpointer                    user_data,
							  GDestroyNotify              destroy);
void            btk_action_group_set_translate_func      (BtkActionGroup             *action_group,
							  BtkTranslateFunc            func,
							  bpointer                    data,
							  GDestroyNotify              notify);
void            btk_action_group_set_translation_domain  (BtkActionGroup             *action_group,
							  const bchar                *domain);
const bchar *btk_action_group_translate_string  (BtkActionGroup             *action_group,
  	                                                  const bchar                *string);

/* Protected for use by BtkAction */
void _btk_action_group_emit_connect_proxy    (BtkActionGroup *action_group,
                                              BtkAction      *action,
                                              BtkWidget      *proxy);
void _btk_action_group_emit_disconnect_proxy (BtkActionGroup *action_group,
                                              BtkAction      *action,
                                              BtkWidget      *proxy);
void _btk_action_group_emit_pre_activate     (BtkActionGroup *action_group,
                                              BtkAction      *action);
void _btk_action_group_emit_post_activate    (BtkActionGroup *action_group,
                                              BtkAction      *action);

B_END_DECLS

#endif  /* __BTK_ACTION_GROUP_H__ */
