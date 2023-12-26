/* Btk+ testing utilities
 * Copyright (C) 2007 Imendio AB
 * Authors: Tim Janik
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

#ifndef __BTK_TEST_UTILS_H__
#define __BTK_TEST_UTILS_H__

#if !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

G_BEGIN_DECLS

/* --- Btk+ Test Utility API --- */
void            btk_test_init                   (int            *argcp,
                                                 char         ***argvp,
                                                 ...);
void            btk_test_register_all_types     (void);
const GType*    btk_test_list_all_types         (guint          *n_types);
BtkWidget*      btk_test_find_widget            (BtkWidget      *widget,
                                                 const gchar    *label_pattern,
                                                 GType           widget_type);
BtkWidget*      btk_test_create_widget          (GType           widget_type,
                                                 const gchar    *first_property_name,
                                                 ...);
BtkWidget*      btk_test_create_simple_window   (const gchar    *window_title,
                                                 const gchar    *dialog_text);
BtkWidget*      btk_test_display_button_window  (const gchar    *window_title,
                                                 const gchar    *dialog_text,
                                                 ...); /* NULL terminated list of (label, &int) pairs */
void            btk_test_slider_set_perc        (BtkWidget      *widget, /* BtkRange-alike */
                                                 double          percentage);
double          btk_test_slider_get_value       (BtkWidget      *widget);
gboolean        btk_test_spin_button_click      (BtkSpinButton  *spinner,
                                                 guint           button,
                                                 gboolean        upwards);
gboolean        btk_test_widget_click           (BtkWidget      *widget,
                                                 guint           button,
                                                 BdkModifierType modifiers);
gboolean        btk_test_widget_send_key        (BtkWidget      *widget,
                                                 guint           keyval,
                                                 BdkModifierType modifiers);
/* operate on BtkEntry, BtkText, BtkTextView or BtkLabel */
void            btk_test_text_set               (BtkWidget      *widget,
                                                 const gchar    *string);
gchar*          btk_test_text_get               (BtkWidget      *widget);

/* --- Btk+ Test low-level API --- */
BtkWidget*      btk_test_find_sibling           (BtkWidget      *base_widget,
                                                 GType           widget_type);
BtkWidget*      btk_test_find_label             (BtkWidget      *widget,
                                                 const gchar    *label_pattern);
G_END_DECLS

#endif /* __BTK_TEST_UTILS_H__ */
