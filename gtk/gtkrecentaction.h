/* BTK - The GIMP Toolkit
 * Recent chooser action for BtkUIManager
 *
 * Copyright (C) 2007, Emmanuele Bassi
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
 * Boston, MA 02111-1307, USA.
 */

#ifndef __BTK_RECENT_ACTION_H__
#define __BTK_RECENT_ACTION_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkaction.h>
#include <btk/btkrecentmanager.h>

G_BEGIN_DECLS

#define BTK_TYPE_RECENT_ACTION                  (btk_recent_action_get_type ())
#define BTK_RECENT_ACTION(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_RECENT_ACTION, BtkRecentAction))
#define BTK_IS_RECENT_ACTION(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_RECENT_ACTION))
#define BTK_RECENT_ACTION_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_RECENT_ACTION, BtkRecentActionClass))
#define BTK_IS_RECENT_ACTION_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_RECENT_ACTION))
#define BTK_RECENT_ACTION_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_RECENT_ACTION, BtkRecentActionClass))

typedef struct _BtkRecentAction         BtkRecentAction;
typedef struct _BtkRecentActionPrivate  BtkRecentActionPrivate;
typedef struct _BtkRecentActionClass    BtkRecentActionClass;

struct _BtkRecentAction
{
  BtkAction parent_instance;

  /*< private >*/
  BtkRecentActionPrivate *GSEAL (priv);
};

struct _BtkRecentActionClass
{
  BtkActionClass parent_class;
};

GType      btk_recent_action_get_type         (void) G_GNUC_CONST;
BtkAction *btk_recent_action_new              (const gchar      *name,
                                               const gchar      *label,
                                               const gchar      *tooltip,
                                               const gchar      *stock_id);
BtkAction *btk_recent_action_new_for_manager  (const gchar      *name,
                                               const gchar      *label,
                                               const gchar      *tooltip,
                                               const gchar      *stock_id,
                                               BtkRecentManager *manager);
gboolean   btk_recent_action_get_show_numbers (BtkRecentAction  *action);
void       btk_recent_action_set_show_numbers (BtkRecentAction  *action,
                                               gboolean          show_numbers);

G_END_DECLS

#endif /* __BTK_RECENT_ACTION_H__ */
