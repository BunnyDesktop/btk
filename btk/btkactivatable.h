/* BTK - The GIMP Toolkit
 * Copyright (C) 2008 Tristan Van Berkom <tristan.van.berkom@gmail.com>
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

#ifndef __BTK_ACTIVATABLE_H__
#define __BTK_ACTIVATABLE_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkaction.h>
#include <btk/btktypeutils.h>

B_BEGIN_DECLS

#define BTK_TYPE_ACTIVATABLE            (btk_activatable_get_type ())
#define BTK_ACTIVATABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_ACTIVATABLE, BtkActivatable))
#define BTK_ACTIVATABLE_CLASS(obj)      (G_TYPE_CHECK_CLASS_CAST ((obj), BTK_TYPE_ACTIVATABLE, BtkActivatableIface))
#define BTK_IS_ACTIVATABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_ACTIVATABLE))
#define BTK_ACTIVATABLE_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), BTK_TYPE_ACTIVATABLE, BtkActivatableIface))


typedef struct _BtkActivatable      BtkActivatable; /* Dummy typedef */
typedef struct _BtkActivatableIface BtkActivatableIface;


/**
 * BtkActivatableIface:
 * @update: Called to update the activatable when its related action's properties change.
 * You must check the #BtkActivatable:use-action-appearance property only apply action
 * properties that are meant to effect the appearance accordingly.
 * @sync_action_properties: Called to update the activatable completely, this is called internally when
 * #BtkActivatable::related-action property is set or unset and by the implementor when
 * #BtkActivatable::use-action-appearance changes.<note><para>This method can be called
 * with a %NULL action at times</para></note>
 *
 * Since: 2.16
 */

struct _BtkActivatableIface
{
  GTypeInterface g_iface;

  /* virtual table */
  void   (* update)                   (BtkActivatable *activatable,
		                       BtkAction      *action,
		                       const gchar    *property_name);
  void   (* sync_action_properties)   (BtkActivatable *activatable,
		                       BtkAction      *action);
};


GType      btk_activatable_get_type                   (void) B_GNUC_CONST;

void       btk_activatable_sync_action_properties     (BtkActivatable *activatable,
						       BtkAction      *action);

void       btk_activatable_set_related_action         (BtkActivatable *activatable,
						       BtkAction      *action);
BtkAction *btk_activatable_get_related_action         (BtkActivatable *activatable);

void       btk_activatable_set_use_action_appearance  (BtkActivatable *activatable,
						       gboolean        use_appearance);
gboolean   btk_activatable_get_use_action_appearance  (BtkActivatable *activatable);

/* For use in activatable implementations */
void       btk_activatable_do_set_related_action      (BtkActivatable *activatable,
						       BtkAction      *action);

B_END_DECLS

#endif /* __BTK_ACTIVATABLE_H__ */
