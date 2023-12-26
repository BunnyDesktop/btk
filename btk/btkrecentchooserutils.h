/* btkrecentchooserutils.h - Private utility functions for implementing a
 *                           BtkRecentChooser interface
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
 *
 * Based on btkfilechooserutils.h:
 *	Copyright (C) 2003 Red Hat, Inc.
 */
 
#ifndef __BTK_RECENT_CHOOSER_UTILS_H__
#define __BTK_RECENT_CHOOSER_UTILS_H__

#include "btkrecentchooserprivate.h"

B_BEGIN_DECLS


#define BTK_RECENT_CHOOSER_DELEGATE_QUARK	(_btk_recent_chooser_delegate_get_quark ())

typedef enum {
  BTK_RECENT_CHOOSER_PROP_FIRST           = 0x3000,
  BTK_RECENT_CHOOSER_PROP_RECENT_MANAGER,
  BTK_RECENT_CHOOSER_PROP_SHOW_PRIVATE,
  BTK_RECENT_CHOOSER_PROP_SHOW_NOT_FOUND,
  BTK_RECENT_CHOOSER_PROP_SHOW_TIPS,
  BTK_RECENT_CHOOSER_PROP_SHOW_ICONS,
  BTK_RECENT_CHOOSER_PROP_SELECT_MULTIPLE,
  BTK_RECENT_CHOOSER_PROP_LIMIT,
  BTK_RECENT_CHOOSER_PROP_LOCAL_ONLY,
  BTK_RECENT_CHOOSER_PROP_SORT_TYPE,
  BTK_RECENT_CHOOSER_PROP_FILTER,
  BTK_RECENT_CHOOSER_PROP_LAST
} BtkRecentChooserProp;

void   _btk_recent_chooser_install_properties  (BObjectClass          *klass);

void   _btk_recent_chooser_delegate_iface_init (BtkRecentChooserIface *iface);
void   _btk_recent_chooser_set_delegate        (BtkRecentChooser      *receiver,
						BtkRecentChooser      *delegate);

GQuark _btk_recent_chooser_delegate_get_quark  (void) B_GNUC_CONST;

B_END_DECLS

#endif /* __BTK_RECENT_CHOOSER_UTILS_H__ */
