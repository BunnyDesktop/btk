/*
 * btkinfobar.h
 * This file is part of BTK+
 *
 * Copyright (C) 2005 - Paolo Maggi
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

/*
 * Modified by the gedit Team, 2005. See the gedit AUTHORS file for a
 * list of people on the gedit Team.
 * See the gedit ChangeLog files for a list of changes.
 *
 * Modified by the BTK+ Team, 2008-2009.
 */

#ifndef __BTK_INFO_BAR_H__
#define __BTK_INFO_BAR_H__

#if !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkhbox.h>
#include <btk/btkenums.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define BTK_TYPE_INFO_BAR              (btk_info_bar_get_type())
#define BTK_INFO_BAR(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), BTK_TYPE_INFO_BAR, BtkInfoBar))
#define BTK_INFO_BAR_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), BTK_TYPE_INFO_BAR, BtkInfoBarClass))
#define BTK_IS_INFO_BAR(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), BTK_TYPE_INFO_BAR))
#define BTK_IS_INFO_BAR_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_INFO_BAR))
#define BTK_INFO_BAR_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), BTK_TYPE_INFO_BAR, BtkInfoBarClass))


typedef struct _BtkInfoBarPrivate BtkInfoBarPrivate;
typedef struct _BtkInfoBarClass BtkInfoBarClass;
typedef struct _BtkInfoBar BtkInfoBar;


struct _BtkInfoBar
{
  BtkHBox parent;

  /*< private > */
  BtkInfoBarPrivate *priv;
};


struct _BtkInfoBarClass
{
  BtkHBoxClass parent_class;

  /* Signals */
  void (* response) (BtkInfoBar *info_bar, gint response_id);

  /* Keybinding signals */
  void (* close)    (BtkInfoBar *info_bar);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
  void (*_btk_reserved5) (void);
  void (*_btk_reserved6) (void);
};

GType          btk_info_bar_get_type               (void) G_GNUC_CONST;
BtkWidget     *btk_info_bar_new                    (void);

BtkWidget     *btk_info_bar_new_with_buttons       (const gchar    *first_button_text,
                                                    ...);

BtkWidget     *btk_info_bar_get_action_area        (BtkInfoBar     *info_bar);
BtkWidget     *btk_info_bar_get_content_area       (BtkInfoBar     *info_bar);
void           btk_info_bar_add_action_widget      (BtkInfoBar     *info_bar,
                                                    BtkWidget      *child,
                                                    gint            response_id);
BtkWidget     *btk_info_bar_add_button             (BtkInfoBar     *info_bar,
                                                    const gchar    *button_text,
                                                    gint            response_id);
void           btk_info_bar_add_buttons            (BtkInfoBar     *info_bar,
                                                    const gchar    *first_button_text,
                                                    ...);
void           btk_info_bar_set_response_sensitive (BtkInfoBar     *info_bar,
                                                    gint            response_id,
                                                    gboolean        setting);
void           btk_info_bar_set_default_response   (BtkInfoBar     *info_bar,
                                                    gint            response_id);

/* Emit response signal */
void           btk_info_bar_response               (BtkInfoBar     *info_bar,
                                                    gint            response_id);

void           btk_info_bar_set_message_type       (BtkInfoBar     *info_bar,
                                                    BtkMessageType  message_type);
BtkMessageType btk_info_bar_get_message_type       (BtkInfoBar     *info_bar);

G_END_DECLS

#endif  /* __BTK_INFO_BAR_H__  */
