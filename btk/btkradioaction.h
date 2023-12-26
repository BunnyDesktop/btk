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

#ifndef __BTK_RADIO_ACTION_H__
#define __BTK_RADIO_ACTION_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btktoggleaction.h>

B_BEGIN_DECLS

#define BTK_TYPE_RADIO_ACTION            (btk_radio_action_get_type ())
#define BTK_RADIO_ACTION(obj)            (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_RADIO_ACTION, BtkRadioAction))
#define BTK_RADIO_ACTION_CLASS(klass)    (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_RADIO_ACTION, BtkRadioActionClass))
#define BTK_IS_RADIO_ACTION(obj)         (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_RADIO_ACTION))
#define BTK_IS_RADIO_ACTION_CLASS(klass) (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_RADIO_ACTION))
#define BTK_RADIO_ACTION_GET_CLASS(obj)  (B_TYPE_INSTANCE_GET_CLASS((obj), BTK_TYPE_RADIO_ACTION, BtkRadioActionClass))

typedef struct _BtkRadioAction        BtkRadioAction;
typedef struct _BtkRadioActionPrivate BtkRadioActionPrivate;
typedef struct _BtkRadioActionClass   BtkRadioActionClass;

struct _BtkRadioAction
{
  BtkToggleAction parent;

  /*< private >*/

  BtkRadioActionPrivate *GSEAL (private_data);
};

struct _BtkRadioActionClass
{
  BtkToggleActionClass parent_class;

  void       (* changed) (BtkRadioAction *action, BtkRadioAction *current);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};

GType           btk_radio_action_get_type          (void) B_GNUC_CONST;
BtkRadioAction *btk_radio_action_new               (const gchar           *name,
                                                    const gchar           *label,
                                                    const gchar           *tooltip,
                                                    const gchar           *stock_id,
                                                    gint                   value);
GSList         *btk_radio_action_get_group         (BtkRadioAction        *action);
void            btk_radio_action_set_group         (BtkRadioAction        *action,
                                                    GSList                *group);
gint            btk_radio_action_get_current_value (BtkRadioAction        *action);
void            btk_radio_action_set_current_value (BtkRadioAction        *action,
                                                    gint                   current_value);

B_END_DECLS

#endif  /* __BTK_RADIO_ACTION_H__ */
