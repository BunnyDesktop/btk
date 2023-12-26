/* BTK - The GIMP Toolkit
 * btkfilechooserdefault.h: Default implementation of BtkFileChooser
 * Copyright (C) 2003, Red Hat, Inc.
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

#ifndef __BTK_FILE_CHOOSER_DEFAULT_H__
#define __BTK_FILE_CHOOSER_DEFAULT_H__

#include "btkfilesystem.h"
#include <btk/btkwidget.h>

B_BEGIN_DECLS

#define BTK_TYPE_FILE_CHOOSER_DEFAULT    (_btk_file_chooser_default_get_type ())
#define BTK_FILE_CHOOSER_DEFAULT(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_FILE_CHOOSER_DEFAULT, BtkFileChooserDefault))
#define BTK_IS_FILE_CHOOSER_DEFAULT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_FILE_CHOOSER_DEFAULT))

typedef struct _BtkFileChooserDefault      BtkFileChooserDefault;

GType      _btk_file_chooser_default_get_type (void) B_GNUC_CONST;
BtkWidget *_btk_file_chooser_default_new      (void);

B_END_DECLS

#endif /* __BTK_FILE_CHOOSER_DEFAULT_H__ */
