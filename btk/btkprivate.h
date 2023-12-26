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

#ifndef __BTK_PRIVATE_H__
#define __BTK_PRIVATE_H__

#include <btk/btkwidget.h>

B_BEGIN_DECLS

/* The private flags that are used in the private_flags member of BtkWidget.
 */
typedef enum
{
  PRIVATE_BTK_USER_STYLE	= 1 <<  0,
  PRIVATE_BTK_RESIZE_PENDING	= 1 <<  2,
  PRIVATE_BTK_HAS_POINTER	= 1 <<  3,   /* If the pointer is above a window belonging to the widget */
  PRIVATE_BTK_SHADOWED		= 1 <<  4,   /* If there is a grab in effect shadowing the widget */
  PRIVATE_BTK_HAS_SHAPE_MASK	= 1 <<  5,
  PRIVATE_BTK_IN_REPARENT       = 1 <<  6,
  PRIVATE_BTK_DIRECTION_SET     = 1 <<  7,   /* If the reading direction is not DIR_NONE */
  PRIVATE_BTK_DIRECTION_LTR     = 1 <<  8,   /* If the reading direction is DIR_LTR */
  PRIVATE_BTK_ANCHORED          = 1 <<  9,   /* If widget has a BtkWindow ancestor */
  PRIVATE_BTK_CHILD_VISIBLE     = 1 <<  10,  /* If widget should be mapped when parent is mapped */
  PRIVATE_BTK_REDRAW_ON_ALLOC   = 1 <<  11,  /* If we should queue a draw on the entire widget when it is reallocated */
  PRIVATE_BTK_ALLOC_NEEDED      = 1 <<  12,  /* If we we should allocate even if the allocation is the same */
  PRIVATE_BTK_REQUEST_NEEDED    = 1 <<  13   /* Whether we need to call btk_widget_size_request */
} BtkPrivateFlags;

/* Macros for extracting a widgets private_flags from BtkWidget.
 */
#define BTK_PRIVATE_FLAGS(wid)            (BTK_WIDGET (wid)->private_flags)
#define BTK_WIDGET_USER_STYLE(obj)	  ((BTK_PRIVATE_FLAGS (obj) & PRIVATE_BTK_USER_STYLE) != 0)
#define BTK_CONTAINER_RESIZE_PENDING(obj) ((BTK_PRIVATE_FLAGS (obj) & PRIVATE_BTK_RESIZE_PENDING) != 0)
#define BTK_WIDGET_HAS_POINTER(obj)	  ((BTK_PRIVATE_FLAGS (obj) & PRIVATE_BTK_HAS_POINTER) != 0)
#define BTK_WIDGET_SHADOWED(obj)	  ((BTK_PRIVATE_FLAGS (obj) & PRIVATE_BTK_SHADOWED) != 0)
#define BTK_WIDGET_HAS_SHAPE_MASK(obj)	  ((BTK_PRIVATE_FLAGS (obj) & PRIVATE_BTK_HAS_SHAPE_MASK) != 0)
#define BTK_WIDGET_IN_REPARENT(obj)	  ((BTK_PRIVATE_FLAGS (obj) & PRIVATE_BTK_IN_REPARENT) != 0)
#define BTK_WIDGET_DIRECTION_SET(obj)	  ((BTK_PRIVATE_FLAGS (obj) & PRIVATE_BTK_DIRECTION_SET) != 0)
#define BTK_WIDGET_DIRECTION_LTR(obj)     ((BTK_PRIVATE_FLAGS (obj) & PRIVATE_BTK_DIRECTION_LTR) != 0)
#define BTK_WIDGET_ANCHORED(obj)          ((BTK_PRIVATE_FLAGS (obj) & PRIVATE_BTK_ANCHORED) != 0)
#define BTK_WIDGET_CHILD_VISIBLE(obj)     ((BTK_PRIVATE_FLAGS (obj) & PRIVATE_BTK_CHILD_VISIBLE) != 0)
#define BTK_WIDGET_REDRAW_ON_ALLOC(obj)   ((BTK_PRIVATE_FLAGS (obj) & PRIVATE_BTK_REDRAW_ON_ALLOC) != 0)
#define BTK_WIDGET_ALLOC_NEEDED(obj)      ((BTK_PRIVATE_FLAGS (obj) & PRIVATE_BTK_ALLOC_NEEDED) != 0)
#define BTK_WIDGET_REQUEST_NEEDED(obj)    ((BTK_PRIVATE_FLAGS (obj) & PRIVATE_BTK_REQUEST_NEEDED) != 0)

/* Macros for setting and clearing private widget flags.
 * we use a preprocessor string concatenation here for a clear
 * flags/private_flags distinction at the cost of single flag operations.
 */
#define BTK_PRIVATE_SET_FLAG(wid,flag)    B_STMT_START{ (BTK_PRIVATE_FLAGS (wid) |= (PRIVATE_ ## flag)); }B_STMT_END
#define BTK_PRIVATE_UNSET_FLAG(wid,flag)  B_STMT_START{ (BTK_PRIVATE_FLAGS (wid) &= ~(PRIVATE_ ## flag)); }B_STMT_END

#if defined G_OS_WIN32 \
  || (defined BDK_WINDOWING_QUARTZ && defined QUARTZ_RELOCATION)

const gchar *_btk_get_datadir ();
const gchar *_btk_get_libdir ();
const gchar *_btk_get_sysconfdir ();
const gchar *_btk_get_localedir ();
const gchar *_btk_get_data_prefix ();

#undef BTK_DATADIR
#define BTK_DATADIR _btk_get_datadir ()
#undef BTK_LIBDIR
#define BTK_LIBDIR _btk_get_libdir ()
#undef BTK_LOCALEDIR
#define BTK_LOCALEDIR _btk_get_localedir ()
#undef BTK_SYSCONFDIR
#define BTK_SYSCONFDIR _btk_get_sysconfdir ()
#undef BTK_DATA_PREFIX
#define BTK_DATA_PREFIX _btk_get_data_prefix ()

#endif /* G_OS_WIN32 */

gboolean _btk_fnmatch (const char *pattern,
		       const char *string,
		       gboolean    no_leading_period);

#define BTK_PARAM_READABLE G_PARAM_READABLE|G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB
#define BTK_PARAM_WRITABLE G_PARAM_WRITABLE|G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB
#define BTK_PARAM_READWRITE G_PARAM_READWRITE|G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB

/* Many keyboard shortcuts for Mac are the same as for X
 * except they use Command key instead of Control (e.g. Cut,
 * Copy, Paste). This symbol is for those simple cases. */
#ifndef BDK_WINDOWING_QUARTZ
#define BTK_DEFAULT_ACCEL_MOD_MASK BDK_CONTROL_MASK
#define BTK_DEFAULT_ACCEL_MOD_MASK_VIRTUAL BDK_CONTROL_MASK
#else
#define BTK_DEFAULT_ACCEL_MOD_MASK BDK_MOD2_MASK
#define BTK_DEFAULT_ACCEL_MOD_MASK_VIRTUAL BDK_META_MASK
#endif

/* When any of these modifiers are active, a key
 * event cannot produce a symbol, so should be
 * skipped when handling text input
 */
#ifndef BDK_WINDOWING_QUARTZ
#define BTK_NO_TEXT_INPUT_MOD_MASK (BDK_MOD1_MASK | BDK_CONTROL_MASK)
#else
#define BTK_NO_TEXT_INPUT_MOD_MASK (BDK_MOD2_MASK | BDK_CONTROL_MASK)
#endif

#ifndef BDK_WINDOWING_QUARTZ
#define BTK_EXTEND_SELECTION_MOD_MASK BDK_SHIFT_MASK
#define BTK_MODIFY_SELECTION_MOD_MASK BDK_CONTROL_MASK
#else
#define BTK_EXTEND_SELECTION_MOD_MASK BDK_SHIFT_MASK
#define BTK_MODIFY_SELECTION_MOD_MASK BDK_MOD2_MASK
#endif

#ifndef BDK_WINDOWING_QUARTZ
#define BTK_TOGGLE_GROUP_MOD_MASK 0
#else
#define BTK_TOGGLE_GROUP_MOD_MASK BDK_MOD1_MASK
#endif

gboolean _btk_button_event_triggers_context_menu (BdkEventButton *event);

gboolean _btk_translate_keyboard_accel_state     (BdkKeymap       *keymap,
                                                  guint            hardware_keycode,
                                                  BdkModifierType  state,
                                                  BdkModifierType  accel_mask,
                                                  gint             group,
                                                  guint           *keyval,
                                                  gint            *effective_group,
                                                  gint            *level,
                                                  BdkModifierType *consumed_modifiers);


B_END_DECLS

#endif /* __BTK_PRIVATE_H__ */
