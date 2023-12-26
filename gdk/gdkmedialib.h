/* BDK - The GIMP Drawing Kit
 * Copyright (C) 2001-2007 Sun Microsystems, Inc.  All rights reserved.
 * (Brian Cameron, Dmitriy Demin, James Cheng, Padraig O'Briain)
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

#ifndef __BDK_MEDIALIB_H__
#define __BDK_MEDIALIB_H__

#ifdef USE_MEDIALIB
#include <mlib_image.h>
#include <mlib_video.h>

#include <bdktypes.h>

G_BEGIN_DECLS

gboolean _bdk_use_medialib (void);

G_END_DECLS

#endif /* USE_MEDIALIB */
#endif /* __BDK_MEDIALIB_H__ */

