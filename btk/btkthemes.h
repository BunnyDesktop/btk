/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * Themes added by The Rasterman <raster@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/.
 */

#ifndef __BTK_THEMES_H__
#define __BTK_THEMES_H__


#include <btk/btkstyle.h>
#include <btk/btkwidget.h>


B_BEGIN_DECLS

#define BTK_TYPE_THEME_ENGINE             (btk_theme_engine_get_type ())
#define BTK_THEME_ENGINE(theme_engine)    (B_TYPE_CHECK_INSTANCE_CAST ((theme_engine), BTK_TYPE_THEME_ENGINE, BtkThemeEngine))
#define BTK_IS_THEME_ENGINE(theme_engine) (B_TYPE_CHECK_INSTANCE_TYPE ((theme_engine), BTK_TYPE_THEME_ENGINE))

GType           btk_theme_engine_get_type        (void) B_GNUC_CONST;
BtkThemeEngine *btk_theme_engine_get             (const gchar     *name);
BtkRcStyle     *btk_theme_engine_create_rc_style (BtkThemeEngine  *engine);

B_END_DECLS

#endif /* __BTK_THEMES_H__ */
