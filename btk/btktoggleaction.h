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

#ifndef __BTK_TOGGLE_ACTION_H__
#define __BTK_TOGGLE_ACTION_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkaction.h>

G_BEGIN_DECLS

#define BTK_TYPE_TOGGLE_ACTION            (btk_toggle_action_get_type ())
#define BTK_TOGGLE_ACTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_TOGGLE_ACTION, BtkToggleAction))
#define BTK_TOGGLE_ACTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_TOGGLE_ACTION, BtkToggleActionClass))
#define BTK_IS_TOGGLE_ACTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_TOGGLE_ACTION))
#define BTK_IS_TOGGLE_ACTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_TOGGLE_ACTION))
#define BTK_TOGGLE_ACTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), BTK_TYPE_TOGGLE_ACTION, BtkToggleActionClass))

typedef struct _BtkToggleAction        BtkToggleAction;
typedef struct _BtkToggleActionPrivate BtkToggleActionPrivate;
typedef struct _BtkToggleActionClass   BtkToggleActionClass;

struct _BtkToggleAction
{
  BtkAction parent;

  /*< private >*/

  BtkToggleActionPrivate *GSEAL (private_data);
};

struct _BtkToggleActionClass
{
  BtkActionClass parent_class;

  void (* toggled) (BtkToggleAction *action);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};

GType            btk_toggle_action_get_type          (void) G_GNUC_CONST;
BtkToggleAction *btk_toggle_action_new               (const gchar     *name,
                                                      const gchar     *label,
                                                      const gchar     *tooltip,
                                                      const gchar     *stock_id);
void             btk_toggle_action_toggled           (BtkToggleAction *action);
void             btk_toggle_action_set_active        (BtkToggleAction *action,
                                                      gboolean         is_active);
gboolean         btk_toggle_action_get_active        (BtkToggleAction *action);
void             btk_toggle_action_set_draw_as_radio (BtkToggleAction *action,
                                                      gboolean         draw_as_radio);
gboolean         btk_toggle_action_get_draw_as_radio (BtkToggleAction *action);


G_END_DECLS

#endif  /* __BTK_TOGGLE_ACTION_H__ */
