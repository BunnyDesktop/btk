/* btkcomboboxentry.h
 * Copyright (C) 2002, 2003  Kristian Rietveld <kris@btk.org>
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

#ifndef __BTK_COMBO_BOX_ENTRY_H__
#define __BTK_COMBO_BOX_ENTRY_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkcombobox.h>
#include <btk/btktreemodel.h>

B_BEGIN_DECLS

#ifndef BTK_DISABLE_DEPRECATED

#define BTK_TYPE_COMBO_BOX_ENTRY             (btk_combo_box_entry_get_type ())
#define BTK_COMBO_BOX_ENTRY(obj)             (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_COMBO_BOX_ENTRY, BtkComboBoxEntry))
#define BTK_COMBO_BOX_ENTRY_CLASS(vtable)    (B_TYPE_CHECK_CLASS_CAST ((vtable), BTK_TYPE_COMBO_BOX_ENTRY, BtkComboBoxEntryClass))
#define BTK_IS_COMBO_BOX_ENTRY(obj)          (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_COMBO_BOX_ENTRY))
#define BTK_IS_COMBO_BOX_ENTRY_CLASS(vtable) (B_TYPE_CHECK_CLASS_TYPE ((vtable), BTK_TYPE_COMBO_BOX_ENTRY))
#define BTK_COMBO_BOX_ENTRY_GET_CLASS(inst)  (B_TYPE_INSTANCE_GET_CLASS ((inst), BTK_TYPE_COMBO_BOX_ENTRY, BtkComboBoxEntryClass))

typedef struct _BtkComboBoxEntry             BtkComboBoxEntry;
typedef struct _BtkComboBoxEntryClass        BtkComboBoxEntryClass;
typedef struct _BtkComboBoxEntryPrivate      BtkComboBoxEntryPrivate;

struct _BtkComboBoxEntry
{
  BtkComboBox parent_instance;

  /*< private >*/
  BtkComboBoxEntryPrivate *GSEAL (priv);
};

struct _BtkComboBoxEntryClass
{
  BtkComboBoxClass parent_class;

  /* Padding for future expansion */
  void (*_btk_reserved0) (void);
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
};


GType       btk_combo_box_entry_get_type        (void) B_GNUC_CONST;
BtkWidget  *btk_combo_box_entry_new             (void);
BtkWidget  *btk_combo_box_entry_new_with_model  (BtkTreeModel     *model,
                                                 gint              text_column);

void        btk_combo_box_entry_set_text_column (BtkComboBoxEntry *entry_box,
                                                 gint              text_column);
gint        btk_combo_box_entry_get_text_column (BtkComboBoxEntry *entry_box);

/* convenience -- text */
BtkWidget  *btk_combo_box_entry_new_text        (void);

#endif

B_END_DECLS

#endif /* __BTK_COMBO_BOX_ENTRY_H__ */
