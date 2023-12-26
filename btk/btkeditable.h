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

#ifndef __BTK_EDITABLE_H__
#define __BTK_EDITABLE_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkwidget.h>


B_BEGIN_DECLS

#define BTK_TYPE_EDITABLE             (btk_editable_get_type ())
#define BTK_EDITABLE(obj)             (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_EDITABLE, BtkEditable))
#define BTK_EDITABLE_CLASS(vtable)    (B_TYPE_CHECK_CLASS_CAST ((vtable), BTK_TYPE_EDITABLE, BtkEditableClass))
#define BTK_IS_EDITABLE(obj)          (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_EDITABLE))
#define BTK_IS_EDITABLE_CLASS(vtable) (B_TYPE_CHECK_CLASS_TYPE ((vtable), BTK_TYPE_EDITABLE))
#define BTK_EDITABLE_GET_CLASS(inst)  (B_TYPE_INSTANCE_GET_INTERFACE ((inst), BTK_TYPE_EDITABLE, BtkEditableClass))

typedef struct _BtkEditable       BtkEditable;         /* Dummy typedef */
typedef struct _BtkEditableClass  BtkEditableClass;

struct _BtkEditableClass
{
  GTypeInterface		   base_iface;

  /* signals */
  void (* insert_text)              (BtkEditable    *editable,
				     const gchar    *text,
				     gint            length,
				     gint           *position);
  void (* delete_text)              (BtkEditable    *editable,
				     gint            start_pos,
				     gint            end_pos);
  void (* changed)                  (BtkEditable    *editable);

  /* vtable */
  void (* do_insert_text)           (BtkEditable    *editable,
				     const gchar    *text,
				     gint            length,
				     gint           *position);
  void (* do_delete_text)           (BtkEditable    *editable,
				     gint            start_pos,
				     gint            end_pos);

  gchar* (* get_chars)              (BtkEditable    *editable,
				     gint            start_pos,
				     gint            end_pos);
  void (* set_selection_bounds)     (BtkEditable    *editable,
				     gint            start_pos,
				     gint            end_pos);
  gboolean (* get_selection_bounds) (BtkEditable    *editable,
				     gint           *start_pos,
				     gint           *end_pos);
  void (* set_position)             (BtkEditable    *editable,
				     gint            position);
  gint (* get_position)             (BtkEditable    *editable);
};

GType    btk_editable_get_type             (void) B_GNUC_CONST;
void     btk_editable_select_rebunnyion        (BtkEditable *editable,
					    gint         start_pos,
					    gint         end_pos);
gboolean btk_editable_get_selection_bounds (BtkEditable *editable,
					    gint        *start_pos,
					    gint        *end_pos);
void     btk_editable_insert_text          (BtkEditable *editable,
					    const gchar *new_text,
					    gint         new_text_length,
					    gint        *position);
void     btk_editable_delete_text          (BtkEditable *editable,
					    gint         start_pos,
					    gint         end_pos);
gchar*   btk_editable_get_chars            (BtkEditable *editable,
					    gint         start_pos,
					    gint         end_pos);
void     btk_editable_cut_clipboard        (BtkEditable *editable);
void     btk_editable_copy_clipboard       (BtkEditable *editable);
void     btk_editable_paste_clipboard      (BtkEditable *editable);
void     btk_editable_delete_selection     (BtkEditable *editable);
void     btk_editable_set_position         (BtkEditable *editable,
					    gint         position);
gint     btk_editable_get_position         (BtkEditable *editable);
void     btk_editable_set_editable         (BtkEditable *editable,
					    gboolean     is_editable);
gboolean btk_editable_get_editable         (BtkEditable *editable);

B_END_DECLS

#endif /* __BTK_EDITABLE_H__ */
