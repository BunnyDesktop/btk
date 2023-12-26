/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

#ifndef __BTK_OLD_EDITABLE_H__
#define __BTK_OLD_EDITABLE_H__

#include <btk/btk.h>


G_BEGIN_DECLS

#define BTK_TYPE_OLD_EDITABLE            (btk_old_editable_get_type ())
#define BTK_OLD_EDITABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_OLD_EDITABLE, BtkOldEditable))
#define BTK_OLD_EDITABLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_OLD_EDITABLE, BtkOldEditableClass))
#define BTK_IS_OLD_EDITABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_OLD_EDITABLE))
#define BTK_IS_OLD_EDITABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_OLD_EDITABLE))
#define BTK_OLD_EDITABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_OLD_EDITABLE, BtkOldEditableClass))


typedef struct _BtkOldEditable       BtkOldEditable;
typedef struct _BtkOldEditableClass  BtkOldEditableClass;

typedef void (*BtkTextFunction) (BtkOldEditable  *editable, guint32 time_);

struct _BtkOldEditable
{
  BtkWidget widget;

  /*< public >*/
  guint      current_pos;

  guint      selection_start_pos;
  guint      selection_end_pos;
  guint      has_selection : 1;

  /*< private >*/
  guint      editable : 1;
  guint      visible : 1;
  
  gchar *clipboard_text;
};

struct _BtkOldEditableClass
{
  BtkWidgetClass parent_class;
  
  /* Bindings actions */
  void (* activate)        (BtkOldEditable *editable);
  void (* set_editable)    (BtkOldEditable *editable,
			    gboolean	    is_editable);
  void (* move_cursor)     (BtkOldEditable *editable,
			    gint            x,
			    gint            y);
  void (* move_word)       (BtkOldEditable *editable,
			    gint            n);
  void (* move_page)       (BtkOldEditable *editable,
			    gint            x,
			    gint            y);
  void (* move_to_row)     (BtkOldEditable *editable,
			    gint            row);
  void (* move_to_column)  (BtkOldEditable *editable,
			    gint            row);
  void (* kill_char)       (BtkOldEditable *editable,
			    gint            direction);
  void (* kill_word)       (BtkOldEditable *editable,
			    gint            direction);
  void (* kill_line)       (BtkOldEditable *editable,
			    gint            direction);
  void (* cut_clipboard)   (BtkOldEditable *editable);
  void (* copy_clipboard)  (BtkOldEditable *editable);
  void (* paste_clipboard) (BtkOldEditable *editable);

  /* Virtual functions. get_chars is in paricular not a signal because
   * it returns malloced memory. The others are not signals because
   * they would not be particularly useful as such. (All changes to
   * selection and position do not go through these functions)
   */
  void (* update_text)  (BtkOldEditable  *editable,
			 gint             start_pos,
			 gint             end_pos);
  gchar* (* get_chars)  (BtkOldEditable  *editable,
			 gint             start_pos,
			 gint             end_pos);
  void (* set_selection)(BtkOldEditable  *editable,
			 gint             start_pos,
			 gint             end_pos);
  void (* set_position) (BtkOldEditable  *editable,
			 gint             position);
};

GType      btk_old_editable_get_type        (void) G_GNUC_CONST;
void       btk_old_editable_claim_selection (BtkOldEditable *old_editable,
					     gboolean        claim,
					     guint32         time_);
void       btk_old_editable_changed         (BtkOldEditable *old_editable);

G_END_DECLS

#endif /* __BTK_OLD_EDITABLE_H__ */

#endif /* BTK_DISABLE_DEPRECATED */
