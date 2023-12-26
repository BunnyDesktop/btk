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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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

#ifndef __BDK_TYPES_H__
#define __BDK_TYPES_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BDK_H_INSIDE__) && !defined (BDK_COMPILATION)
#error "Only <bdk/bdk.h> can be included directly."
#endif

/* BDK uses "bunnylib". (And so does BTK).
 */
#include <bunnylib.h>
#include <bango/bango.h>
#include <bunnylib-object.h>

#ifdef G_OS_WIN32
#  ifdef BDK_COMPILATION
#    define BDKVAR extern __declspec(dllexport)
#  else
#    define BDKVAR extern __declspec(dllimport)
#  endif
#else
#  define BDKVAR extern
#endif

/* The system specific file bdkconfig.h contains such configuration
 * settings that are needed not only when compiling BDK (or BTK)
 * itself, but also occasionally when compiling programs that use BDK
 * (or BTK). One such setting is what windowing API backend is in use.
 */
#include <bdkconfig.h>

/* some common magic values */
#define BDK_CURRENT_TIME     0L
#define BDK_PARENT_RELATIVE  1L



G_BEGIN_DECLS


/* Type definitions for the basic structures.
 */
typedef struct _BdkPoint	      BdkPoint;
typedef struct _BdkRectangle	      BdkRectangle;
typedef struct _BdkSegment	      BdkSegment;
typedef struct _BdkSpan	              BdkSpan;

/*
 * Note that on some platforms the wchar_t type
 * is not the same as BdkWChar. For instance
 * on Win32, wchar_t is unsigned short.
 */
typedef guint32			    BdkWChar;

typedef struct _BdkAtom            *BdkAtom;

#define BDK_ATOM_TO_POINTER(atom) (atom)
#define BDK_POINTER_TO_ATOM(ptr)  ((BdkAtom)(ptr))

#ifdef BDK_NATIVE_WINDOW_POINTER
#define BDK_GPOINTER_TO_NATIVE_WINDOW(p) ((BdkNativeWindow) (p))
#else
#define BDK_GPOINTER_TO_NATIVE_WINDOW(p) GPOINTER_TO_UINT(p)
#endif

#define _BDK_MAKE_ATOM(val) ((BdkAtom)GUINT_TO_POINTER(val))
#define BDK_NONE            _BDK_MAKE_ATOM (0)

#ifdef BDK_NATIVE_WINDOW_POINTER
typedef gpointer BdkNativeWindow;
#else
typedef guint32 BdkNativeWindow;
#endif
 
/* Forward declarations of commonly used types
 */
typedef struct _BdkColor	      BdkColor;
typedef struct _BdkColormap	      BdkColormap;
typedef struct _BdkCursor	      BdkCursor;
typedef struct _BdkFont		      BdkFont;
typedef struct _BdkGC                 BdkGC;
typedef struct _BdkImage              BdkImage;
typedef struct _BdkRebunnyion             BdkRebunnyion;
typedef struct _BdkVisual             BdkVisual;

typedef struct _BdkDrawable           BdkDrawable;
typedef struct _BdkDrawable           BdkBitmap;
typedef struct _BdkDrawable           BdkPixmap;
typedef struct _BdkDrawable           BdkWindow;
typedef struct _BdkDisplay	      BdkDisplay;
typedef struct _BdkScreen	      BdkScreen;

typedef enum
{
  BDK_LSB_FIRST,
  BDK_MSB_FIRST
} BdkByteOrder;

/* Types of modifiers.
 */
typedef enum
{
  BDK_SHIFT_MASK    = 1 << 0,
  BDK_LOCK_MASK	    = 1 << 1,
  BDK_CONTROL_MASK  = 1 << 2,
  BDK_MOD1_MASK	    = 1 << 3,
  BDK_MOD2_MASK	    = 1 << 4,
  BDK_MOD3_MASK	    = 1 << 5,
  BDK_MOD4_MASK	    = 1 << 6,
  BDK_MOD5_MASK	    = 1 << 7,
  BDK_BUTTON1_MASK  = 1 << 8,
  BDK_BUTTON2_MASK  = 1 << 9,
  BDK_BUTTON3_MASK  = 1 << 10,
  BDK_BUTTON4_MASK  = 1 << 11,
  BDK_BUTTON5_MASK  = 1 << 12,

  /* The next few modifiers are used by XKB, so we skip to the end.
   * Bits 15 - 25 are currently unused. Bit 29 is used internally.
   */
  
  BDK_SUPER_MASK    = 1 << 26,
  BDK_HYPER_MASK    = 1 << 27,
  BDK_META_MASK     = 1 << 28,
  
  BDK_RELEASE_MASK  = 1 << 30,

  BDK_MODIFIER_MASK = 0x5c001fff
} BdkModifierType;

typedef enum
{
  BDK_INPUT_READ       = 1 << 0,
  BDK_INPUT_WRITE      = 1 << 1,
  BDK_INPUT_EXCEPTION  = 1 << 2
} BdkInputCondition;

typedef enum
{
  BDK_OK	  = 0,
  BDK_ERROR	  = -1,
  BDK_ERROR_PARAM = -2,
  BDK_ERROR_FILE  = -3,
  BDK_ERROR_MEM	  = -4
} BdkStatus;

/* We define specific numeric values for these constants,
 * since old application code may depend on them matching the X values
 * We don't actually depend on the matchup ourselves.
 */
typedef enum
{
  BDK_GRAB_SUCCESS         = 0,
  BDK_GRAB_ALREADY_GRABBED = 1,
  BDK_GRAB_INVALID_TIME    = 2,
  BDK_GRAB_NOT_VIEWABLE    = 3,
  BDK_GRAB_FROZEN          = 4
} BdkGrabStatus;

typedef void (*BdkInputFunction) (gpointer	    data,
				  gint		    source,
				  BdkInputCondition condition);

#ifndef BDK_DISABLE_DEPRECATED

typedef void (*BdkDestroyNotify) (gpointer data);

#endif /* BDK_DISABLE_DEPRECATED */

struct _BdkPoint
{
  gint x;
  gint y;
};

struct _BdkRectangle
{
  gint x;
  gint y;
  gint width;
  gint height;
};

struct _BdkSegment
{
  gint x1;
  gint y1;
  gint x2;
  gint y2;
};

struct _BdkSpan
{
  gint x;
  gint y;
  gint width;
};

G_END_DECLS


#endif /* __BDK_TYPES_H__ */
