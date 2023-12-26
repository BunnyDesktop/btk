/* BTK - The GIMP Toolkit
 *
 * Copyright (C) 2010 Christian Dywan
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __BTK_COMBO_BOX_TEXT_H__
#define __BTK_COMBO_BOX_TEXT_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkcombobox.h>

G_BEGIN_DECLS

#define BTK_TYPE_COMBO_BOX_TEXT                 (btk_combo_box_text_get_type ())
#define BTK_COMBO_BOX_TEXT(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_COMBO_BOX_TEXT, BtkComboBoxText))
#define BTK_COMBO_BOX_TEXT_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_COMBO_BOX_TEXT, BtkComboBoxTextClass))
#define BTK_IS_COMBO_BOX_TEXT(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_COMBO_BOX_TEXT))
#define BTK_IS_COMBO_BOX_TEXT_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_COMBO_BOX_TEXT))
#define BTK_COMBO_BOX_TEXT_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_COMBO_BOX_TEXT, BtkComboBoxTextClass))

typedef struct _BtkComboBoxText             BtkComboBoxText;
typedef struct _BtkComboBoxTextPrivate      BtkComboBoxTextPrivate;
typedef struct _BtkComboBoxTextClass        BtkComboBoxTextClass;

struct _BtkComboBoxText
{
  /* <private> */
  BtkComboBox parent_instance;

  BtkComboBoxTextPrivate *priv;
};

struct _BtkComboBoxTextClass
{
  BtkComboBoxClass parent_class;

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};

GType         btk_combo_box_text_get_type        (void) G_GNUC_CONST;
BtkWidget*    btk_combo_box_text_new             (void);
BtkWidget*    btk_combo_box_text_new_with_entry  (void);
void          btk_combo_box_text_append_text     (BtkComboBoxText     *combo_box,
                                                  const gchar         *text);
void          btk_combo_box_text_insert_text     (BtkComboBoxText     *combo_box,
                                                  gint                 position,
                                                  const gchar         *text);
void          btk_combo_box_text_prepend_text    (BtkComboBoxText     *combo_box,
                                                  const gchar         *text);
void          btk_combo_box_text_remove          (BtkComboBoxText     *combo_box,
                                                  gint                 position);
gchar        *btk_combo_box_text_get_active_text (BtkComboBoxText     *combo_box);


G_END_DECLS

#endif /* __BTK_COMBO_BOX_TEXT_H__ */
