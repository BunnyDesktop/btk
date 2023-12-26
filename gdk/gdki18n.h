/* BDK - The GIMP Drawing Kit
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

#ifndef __BDK_I18N_H__
#define __BDK_I18N_H__

#ifndef BDK_DISABLE_DEPRECATED

/* BDK uses "bunnylib". (And so does BTK).
 */
#include <bunnylib.h>
#include <bdkconfig.h>

/* international string support */

#include <stdlib.h>

#if !defined(BDK_HAVE_BROKEN_WCTYPE) && (defined(BDK_HAVE_WCTYPE_H) || defined(BDK_HAVE_WCHAR_H)) && !defined(X_LOCALE)
#  ifdef BDK_HAVE_WCTYPE_H
#    include <wctype.h>
#  else
#    ifdef BDK_HAVE_WCHAR_H
#      include <wchar.h>
#    endif
#  endif
#  define bdk_iswalnum(c) iswalnum(c)
#  define bdk_iswspace(c) iswspace(c)
#else
#  include <ctype.h>
#  define bdk_iswalnum(c) ((wchar_t)(c) <= 0xFF && isalnum(c))
#  define bdk_iswspace(c) ((wchar_t)(c) <= 0xFF && isspace(c))
#endif

#endif /* BDK_DISABLE_DEPRECATED */

#endif /* __BDK_I18N_H__ */
