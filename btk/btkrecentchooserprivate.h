/* btkrecentprivatechooser.h - Interface definitions for recent selectors UI
 *
 * Copyright (C) 2006 Emmanuele Bassi
 *
 * All rights reserved
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
 * Boston, MA 02111-1307, USA.
 */

#ifndef __BTK_RECENT_CHOOSER_PRIVATE_H__
#define __BTK_RECENT_CHOOSER_PRIVATE_H__

#include "btkrecentmanager.h"
#include "btkrecentchooser.h"
#include "btkactivatable.h"

G_BEGIN_DECLS

BtkRecentManager *_btk_recent_chooser_get_recent_manager     (BtkRecentChooser  *chooser);
GList *           _btk_recent_chooser_get_items              (BtkRecentChooser  *chooser,
							      BtkRecentFilter   *filter,
							      BtkRecentSortFunc  func,
							      gpointer           data);

void              _btk_recent_chooser_item_activated         (BtkRecentChooser  *chooser);
void              _btk_recent_chooser_selection_changed      (BtkRecentChooser  *chooser);

void              _btk_recent_chooser_update                 (BtkActivatable       *activatable,
							      BtkAction            *action,
							      const gchar          *property_name);
void              _btk_recent_chooser_sync_action_properties (BtkActivatable       *activatable,
							      BtkAction            *action);
void              _btk_recent_chooser_set_related_action     (BtkRecentChooser     *recent_chooser, 
							      BtkAction            *action);
BtkAction        *_btk_recent_chooser_get_related_action     (BtkRecentChooser     *recent_chooser);
void              _btk_recent_chooser_set_use_action_appearance (BtkRecentChooser  *recent_chooser, 
								 gboolean           use_appearance);
gboolean          _btk_recent_chooser_get_use_action_appearance (BtkRecentChooser  *recent_chooser);

G_END_DECLS

#endif /* ! __BTK_RECENT_CHOOSER_PRIVATE_H__ */
