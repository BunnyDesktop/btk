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

#ifndef __BTK_FILESEL_H__
#define __BTK_FILESEL_H__

#include <btk/btk.h>


B_BEGIN_DECLS


#define BTK_TYPE_FILE_SELECTION            (btk_file_selection_get_type ())
#define BTK_FILE_SELECTION(obj)            (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_FILE_SELECTION, BtkFileSelection))
#define BTK_FILE_SELECTION_CLASS(klass)    (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_FILE_SELECTION, BtkFileSelectionClass))
#define BTK_IS_FILE_SELECTION(obj)         (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_FILE_SELECTION))
#define BTK_IS_FILE_SELECTION_CLASS(klass) (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_FILE_SELECTION))
#define BTK_FILE_SELECTION_GET_CLASS(obj)  (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_FILE_SELECTION, BtkFileSelectionClass))


typedef struct _BtkFileSelection       BtkFileSelection;
typedef struct _BtkFileSelectionClass  BtkFileSelectionClass;

struct _BtkFileSelection
{
  /*< private >*/
  BtkDialog parent_instance;

  /*< public >*/
  BtkWidget *dir_list;
  BtkWidget *file_list;
  BtkWidget *selection_entry;
  BtkWidget *selection_text;
  BtkWidget *main_vbox;
  BtkWidget *ok_button;
  BtkWidget *cancel_button;
  BtkWidget *help_button;
  BtkWidget *history_pulldown;
  BtkWidget *history_menu;
  GList     *history_list;
  BtkWidget *fileop_dialog;
  BtkWidget *fileop_entry;
  gchar     *fileop_file;
  gpointer   cmpl_state;
  
  BtkWidget *fileop_c_dir;
  BtkWidget *fileop_del_file;
  BtkWidget *fileop_ren_file;
  
  BtkWidget *button_area;
  BtkWidget *action_area;

  /*< private >*/
  GPtrArray *selected_names;
  gchar     *last_selected;
};

struct _BtkFileSelectionClass
{
  BtkDialogClass parent_class;

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};


#ifdef G_OS_WIN32
/* Reserve old names for DLL ABI backward compatibility */
#define btk_file_selection_get_filename btk_file_selection_get_filename_utf8
#define btk_file_selection_set_filename btk_file_selection_set_filename_utf8
#define btk_file_selection_get_selections btk_file_selection_get_selections_utf8
#endif

GType      btk_file_selection_get_type            (void) B_GNUC_CONST;
BtkWidget* btk_file_selection_new                 (const gchar      *title);
void       btk_file_selection_set_filename        (BtkFileSelection *filesel,
						   const gchar      *filename);
const gchar* btk_file_selection_get_filename      (BtkFileSelection *filesel);

void	   btk_file_selection_complete		  (BtkFileSelection *filesel,
						   const gchar	    *pattern);
void       btk_file_selection_show_fileop_buttons (BtkFileSelection *filesel);
void       btk_file_selection_hide_fileop_buttons (BtkFileSelection *filesel);

gchar**    btk_file_selection_get_selections      (BtkFileSelection *filesel);

void       btk_file_selection_set_select_multiple (BtkFileSelection *filesel,
						   gboolean          select_multiple);
gboolean   btk_file_selection_get_select_multiple (BtkFileSelection *filesel);


B_END_DECLS


#endif /* __BTK_FILESEL_H__ */

#endif /* BTK_DISABLE_DEPRECATED */
