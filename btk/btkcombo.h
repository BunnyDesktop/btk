/* btkcombo - combo widget for btk+
 * Copyright 1997 Paolo Molaro
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
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#ifndef BTK_DISABLE_DEPRECATED

#ifndef __BTK_SMART_COMBO_H__
#define __BTK_SMART_COMBO_H__

#include <btk/btk.h>


B_BEGIN_DECLS

#define BTK_TYPE_COMBO              (btk_combo_get_type ())
#define BTK_COMBO(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_COMBO, BtkCombo))
#define BTK_COMBO_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_COMBO, BtkComboClass))
#define BTK_IS_COMBO(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_COMBO))
#define BTK_IS_COMBO_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_COMBO))
#define BTK_COMBO_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_COMBO, BtkComboClass))


typedef struct _BtkCombo	BtkCombo;
typedef struct _BtkComboClass	BtkComboClass;

/* you should access only the entry and list fields directly */
struct _BtkCombo {
	BtkHBox hbox;
  
        /*< public >*/
	BtkWidget *entry;
	
        /*< private >*/
	BtkWidget *button;
	BtkWidget *popup;
	BtkWidget *popwin;
	
        /*< public >*/
	BtkWidget *list;

        /*< private >*/
	guint entry_change_id;
	guint list_change_id;	/* unused */

	guint value_in_list:1;
	guint ok_if_empty:1;
	guint case_sensitive:1;
	guint use_arrows:1;
	guint use_arrows_always:1;

        guint16 current_button;
	guint activate_id;
};

struct _BtkComboClass {
	BtkHBoxClass parent_class;

        /* Padding for future expansion */
        void (*_btk_reserved1) (void);
        void (*_btk_reserved2) (void);
        void (*_btk_reserved3) (void);
        void (*_btk_reserved4) (void);
};

GType      btk_combo_get_type              (void) B_GNUC_CONST;

BtkWidget* btk_combo_new                   (void);
/* the text in the entry must be or not be in the list */
void       btk_combo_set_value_in_list     (BtkCombo*    combo, 
                                            gboolean     val,
                                            gboolean     ok_if_empty);
/* set/unset arrows working for changing the value (can be annoying) */
void       btk_combo_set_use_arrows        (BtkCombo*    combo, 
                                            gboolean     val);
/* up/down arrows change value if current value not in list */
void       btk_combo_set_use_arrows_always (BtkCombo*    combo, 
                                            gboolean     val);
/* perform case-sensitive compares */
void       btk_combo_set_case_sensitive    (BtkCombo*    combo, 
                                            gboolean     val);
/* call this function on an item if it isn't a label or you
   want it to have a different value to be displayed in the entry */
void       btk_combo_set_item_string       (BtkCombo*    combo,
                                            BtkItem*     item,
                                            const gchar* item_value);
/* simple interface */
void       btk_combo_set_popdown_strings   (BtkCombo*    combo, 
                                            GList        *strings);

void       btk_combo_disable_activate      (BtkCombo*    combo);

B_END_DECLS

#endif /* __BTK_SMART_COMBO_H__ */

#endif /* BTK_DISABLE_DEPRECATED */
