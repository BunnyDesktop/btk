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

#ifndef __BTK_DEBUG_H__
#define __BTK_DEBUG_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <bunnylib.h>

B_BEGIN_DECLS

typedef enum {
  BTK_DEBUG_MISC        = 1 << 0,
  BTK_DEBUG_PLUGSOCKET  = 1 << 1,
  BTK_DEBUG_TEXT        = 1 << 2,
  BTK_DEBUG_TREE        = 1 << 3,
  BTK_DEBUG_UPDATES     = 1 << 4,
  BTK_DEBUG_KEYBINDINGS = 1 << 5,
  BTK_DEBUG_MULTIHEAD   = 1 << 6,
  BTK_DEBUG_MODULES     = 1 << 7,
  BTK_DEBUG_GEOMETRY    = 1 << 8,
  BTK_DEBUG_ICONTHEME   = 1 << 9,
  BTK_DEBUG_PRINTING	= 1 << 10,
  BTK_DEBUG_BUILDER	= 1 << 11
} BtkDebugFlag;

#ifdef G_ENABLE_DEBUG

#define BTK_NOTE(type,action)                B_STMT_START { \
    if (btk_debug_flags & BTK_DEBUG_##type)                 \
       { action; };                          } B_STMT_END

#else /* !G_ENABLE_DEBUG */

#define BTK_NOTE(type, action)

#endif /* G_ENABLE_DEBUG */

#ifdef G_OS_WIN32
#  ifdef BTK_COMPILATION
#    define BTKVAR extern __declspec(dllexport)
#  else
#    define BTKVAR extern __declspec(dllimport)
#  endif
#else
#  define BTKVAR extern
#endif

BTKVAR buint btk_debug_flags;

B_END_DECLS

#endif /* __BTK_DEBUG_H__ */
