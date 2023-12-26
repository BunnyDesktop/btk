/* BTK - The GIMP Toolkit
 * btkfilechoosersettings.h: Internal settings for the BtkFileChooser widget
 * Copyright (C) 2006, Novell, Inc.
 *
 * Authors: Federico Mena-Quintero <federico@novell.com>
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

#ifndef __BTK_FILE_CHOOSER_SETTINGS_H__
#define __BTK_FILE_CHOOSER_SETTINGS_H__

#include "btkfilechooserprivate.h"

B_BEGIN_DECLS

#define BTK_FILE_CHOOSER_SETTINGS_TYPE (_btk_file_chooser_settings_get_type ())

/* Column numbers for the file list */
enum {
  FILE_LIST_COL_NAME,
  FILE_LIST_COL_SIZE,
  FILE_LIST_COL_MTIME,
  FILE_LIST_COL_NUM_COLUMNS
};

typedef struct _BtkFileChooserSettings BtkFileChooserSettings;
typedef struct _BtkFileChooserSettingsClass BtkFileChooserSettingsClass;

struct _BtkFileChooserSettings
{
  BObject object;

  LocationMode location_mode;

  BtkSortType sort_order;
  gint sort_column;
  StartupMode startup_mode;

  int geometry_x;
  int geometry_y;
  int geometry_width;
  int geometry_height;

  guint settings_read    : 1;
  guint show_hidden      : 1;
  guint show_size_column : 1;
};

struct _BtkFileChooserSettingsClass
{
  BObjectClass parent_class;
};

GType _btk_file_chooser_settings_get_type (void) B_GNUC_CONST;

BtkFileChooserSettings *_btk_file_chooser_settings_new (void);

LocationMode _btk_file_chooser_settings_get_location_mode (BtkFileChooserSettings *settings);
void         _btk_file_chooser_settings_set_location_mode (BtkFileChooserSettings *settings,
							   LocationMode            location_mode);

gboolean _btk_file_chooser_settings_get_show_hidden (BtkFileChooserSettings *settings);
void     _btk_file_chooser_settings_set_show_hidden (BtkFileChooserSettings *settings,
						     gboolean                show_hidden);

gboolean _btk_file_chooser_settings_get_show_size_column (BtkFileChooserSettings *settings);
void     _btk_file_chooser_settings_set_show_size_column (BtkFileChooserSettings *settings,
                                                          gboolean                show_column);

gint _btk_file_chooser_settings_get_sort_column (BtkFileChooserSettings *settings);
void _btk_file_chooser_settings_set_sort_column (BtkFileChooserSettings *settings,
						 gint sort_column);

BtkSortType _btk_file_chooser_settings_get_sort_order (BtkFileChooserSettings *settings);
void        _btk_file_chooser_settings_set_sort_order (BtkFileChooserSettings *settings,
						       BtkSortType sort_order);

void _btk_file_chooser_settings_get_geometry (BtkFileChooserSettings *settings,
					      int                    *out_x,
					      int                    *out_y,
					      int                    *out_width,
					      int                    *out_heigth);
void _btk_file_chooser_settings_set_geometry (BtkFileChooserSettings *settings,
					      int                     x,
					      int                     y,
					      int                     width,
					      int                     heigth);

void _btk_file_chooser_settings_set_startup_mode (BtkFileChooserSettings *settings,
						  StartupMode             startup_mode);
StartupMode _btk_file_chooser_settings_get_startup_mode (BtkFileChooserSettings *settings);

gboolean _btk_file_chooser_settings_save (BtkFileChooserSettings *settings,
					  GError                **error);

/* FIXME: persist these options:
 *
 * - paned width
 * - show_hidden
 */

B_END_DECLS

#endif
