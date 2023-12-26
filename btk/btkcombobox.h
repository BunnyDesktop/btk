/* btkcombobox.h
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

#ifndef __BTK_COMBO_BOX_H__
#define __BTK_COMBO_BOX_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkbin.h>
#include <btk/btktreemodel.h>
#include <btk/btktreeview.h>

B_BEGIN_DECLS

#define BTK_TYPE_COMBO_BOX             (btk_combo_box_get_type ())
#define BTK_COMBO_BOX(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_COMBO_BOX, BtkComboBox))
#define BTK_COMBO_BOX_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), BTK_TYPE_COMBO_BOX, BtkComboBoxClass))
#define BTK_IS_COMBO_BOX(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_COMBO_BOX))
#define BTK_IS_COMBO_BOX_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), BTK_TYPE_COMBO_BOX))
#define BTK_COMBO_BOX_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), BTK_TYPE_COMBO_BOX, BtkComboBoxClass))

typedef struct _BtkComboBox        BtkComboBox;
typedef struct _BtkComboBoxClass   BtkComboBoxClass;
typedef struct _BtkComboBoxPrivate BtkComboBoxPrivate;

struct _BtkComboBox
{
  BtkBin parent_instance;

  /*< private >*/
  BtkComboBoxPrivate *GSEAL (priv);
};

struct _BtkComboBoxClass
{
  BtkBinClass parent_class;

  /* signals */
  void     (* changed)          (BtkComboBox *combo_box);

  /* vfuncs */
  gchar *  (* get_active_text)  (BtkComboBox *combo_box);

  /* Padding for future expansion */
  void (*_btk_reserved0) (void);
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
};


/* construction */
GType         btk_combo_box_get_type                 (void) B_GNUC_CONST;
BtkWidget    *btk_combo_box_new                      (void);
BtkWidget    *btk_combo_box_new_with_entry           (void);
BtkWidget    *btk_combo_box_new_with_model           (BtkTreeModel *model);
BtkWidget    *btk_combo_box_new_with_model_and_entry (BtkTreeModel *model);

/* grids */
gint          btk_combo_box_get_wrap_width         (BtkComboBox *combo_box);
void          btk_combo_box_set_wrap_width         (BtkComboBox *combo_box,
                                                    gint         width);
gint          btk_combo_box_get_row_span_column    (BtkComboBox *combo_box);
void          btk_combo_box_set_row_span_column    (BtkComboBox *combo_box,
                                                    gint         row_span);
gint          btk_combo_box_get_column_span_column (BtkComboBox *combo_box);
void          btk_combo_box_set_column_span_column (BtkComboBox *combo_box,
                                                    gint         column_span);

gboolean      btk_combo_box_get_add_tearoffs       (BtkComboBox *combo_box);
void          btk_combo_box_set_add_tearoffs       (BtkComboBox *combo_box,
						    gboolean     add_tearoffs);

const gchar * btk_combo_box_get_title              (BtkComboBox *combo_box);
void                  btk_combo_box_set_title      (BtkComboBox *combo_box,
					            const gchar *title);

gboolean      btk_combo_box_get_focus_on_click     (BtkComboBox *combo);
void          btk_combo_box_set_focus_on_click     (BtkComboBox *combo,
						    gboolean     focus_on_click);

/* get/set active item */
gint          btk_combo_box_get_active       (BtkComboBox     *combo_box);
void          btk_combo_box_set_active       (BtkComboBox     *combo_box,
                                              gint             index_);
gboolean      btk_combo_box_get_active_iter  (BtkComboBox     *combo_box,
                                              BtkTreeIter     *iter);
void          btk_combo_box_set_active_iter  (BtkComboBox     *combo_box,
                                              BtkTreeIter     *iter);

/* getters and setters */
void          btk_combo_box_set_model        (BtkComboBox     *combo_box,
                                              BtkTreeModel    *model);
BtkTreeModel *btk_combo_box_get_model        (BtkComboBox     *combo_box);

BtkTreeViewRowSeparatorFunc btk_combo_box_get_row_separator_func (BtkComboBox                *combo_box);
void                        btk_combo_box_set_row_separator_func (BtkComboBox                *combo_box,
								  BtkTreeViewRowSeparatorFunc func,
								  gpointer                    data,
								  GDestroyNotify              destroy);

void               btk_combo_box_set_button_sensitivity (BtkComboBox        *combo_box,
							 BtkSensitivityType  sensitivity);
BtkSensitivityType btk_combo_box_get_button_sensitivity (BtkComboBox        *combo_box);

gboolean           btk_combo_box_get_has_entry          (BtkComboBox        *combo_box);
void               btk_combo_box_set_entry_text_column  (BtkComboBox        *combo_box,
							 gint                text_column);
gint               btk_combo_box_get_entry_text_column  (BtkComboBox        *combo_box);

#if !defined (BTK_DISABLE_DEPRECATED) || defined (BTK_COMPILATION)

/* convenience -- text */
BtkWidget    *btk_combo_box_new_text         (void);
void          btk_combo_box_append_text      (BtkComboBox     *combo_box,
                                              const gchar     *text);
void          btk_combo_box_insert_text      (BtkComboBox     *combo_box,
                                              gint             position,
                                              const gchar     *text);
void          btk_combo_box_prepend_text     (BtkComboBox     *combo_box,
                                              const gchar     *text);
void          btk_combo_box_remove_text      (BtkComboBox     *combo_box,
                                              gint             position);
gchar        *btk_combo_box_get_active_text  (BtkComboBox     *combo_box);

#endif

/* programmatic control */
void          btk_combo_box_popup            (BtkComboBox     *combo_box);
void          btk_combo_box_popdown          (BtkComboBox     *combo_box);
BatkObject*    btk_combo_box_get_popup_accessible (BtkComboBox *combo_box);


B_END_DECLS

#endif /* __BTK_COMBO_BOX_H__ */
