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

#ifndef __BDK_H__
#define __BDK_H__

#define __BDK_H_INSIDE__

#include <bdk/bdkapplaunchcontext.h>
#include <bdk/bdkbairo.h>
#include <bdk/bdkcolor.h>
#include <bdk/bdkcursor.h>
#include <bdk/bdkdisplay.h>
#include <bdk/bdkdisplaymanager.h>
#include <bdk/bdkdnd.h>
#include <bdk/bdkdrawable.h>
#include <bdk/bdkenumtypes.h>
#include <bdk/bdkevents.h>
#include <bdk/bdkfont.h>
#include <bdk/bdkgc.h>
#include <bdk/bdkimage.h>
#include <bdk/bdkinput.h>
#include <bdk/bdkkeys.h>
#include <bdk/bdkbango.h>
#include <bdk/bdkpixbuf.h>
#include <bdk/bdkpixmap.h>
#include <bdk/bdkproperty.h>
#include <bdk/bdkrebunnyion.h>
#include <bdk/bdkrgb.h>
#include <bdk/bdkscreen.h>
#include <bdk/bdkselection.h>
#include <bdk/bdkspawn.h>
#include <bdk/bdktestutils.h>
#include <bdk/bdktypes.h>
#include <bdk/bdkvisual.h>
#include <bdk/bdkwindow.h>

#undef __BDK_H_INSIDE__

B_BEGIN_DECLS


/* Initialization, exit and events
 */
#define	  BDK_PRIORITY_EVENTS		(G_PRIORITY_DEFAULT)
void 	  bdk_parse_args	   	(gint	   	*argc,
					 gchar        ***argv);
void 	  bdk_init		   	(gint	   	*argc,
					 gchar        ***argv);
gboolean  bdk_init_check   	        (gint	   	*argc,
					 gchar        ***argv);
void bdk_add_option_entries_libbtk_only (GOptionGroup *group);
void bdk_pre_parse_libbtk_only          (void);

#ifndef BDK_DISABLE_DEPRECATED
void  	  bdk_exit		   	(gint	    	 error_code);
gchar*	  bdk_set_locale	   	(void);
#endif /* BDK_DISABLE_DEPRECATED */

const char *         bdk_get_program_class (void);
void                 bdk_set_program_class (const char *program_class);

/* Push and pop error handlers for X errors
 */
void      bdk_error_trap_push           (void);
gint      bdk_error_trap_pop            (void);

#ifndef BDK_DISABLE_DEPRECATED
void	  bdk_set_use_xshm		(gboolean	 use_xshm);
gboolean  bdk_get_use_xshm		(void);
#endif /* BDK_DISABLE_DEPRECATED */

gchar*	                  bdk_get_display		(void);
const gchar*	          bdk_get_display_arg_name	(void);

#ifndef BDK_DISABLE_DEPRECATED
gint bdk_input_add_full	  (gint		     source,
			   BdkInputCondition condition,
			   BdkInputFunction  function,
			   gpointer	     data,
			   GDestroyNotify    destroy);
gint bdk_input_add	  (gint		     source,
			   BdkInputCondition condition,
			   BdkInputFunction  function,
			   gpointer	     data);
void bdk_input_remove	  (gint		     tag);
#endif /* BDK_DISABLE_DEPRECATED */

BdkGrabStatus bdk_pointer_grab       (BdkWindow    *window,
				      gboolean      owner_events,
				      BdkEventMask  event_mask,
				      BdkWindow    *confine_to,
				      BdkCursor    *cursor,
				      guint32       time_);
BdkGrabStatus bdk_keyboard_grab      (BdkWindow    *window,
				      gboolean      owner_events,
				      guint32       time_);

gboolean bdk_pointer_grab_info_libbtk_only (BdkDisplay *display,
					    BdkWindow **grab_window,
					    gboolean   *owner_events);
gboolean bdk_keyboard_grab_info_libbtk_only (BdkDisplay *display,
					     BdkWindow **grab_window,
					     gboolean   *owner_events);

#ifndef BDK_MULTIHEAD_SAFE
void          bdk_pointer_ungrab     (guint32       time_);
void          bdk_keyboard_ungrab    (guint32       time_);
gboolean      bdk_pointer_is_grabbed (void);

gint bdk_screen_width  (void) B_GNUC_CONST;
gint bdk_screen_height (void) B_GNUC_CONST;

gint bdk_screen_width_mm  (void) B_GNUC_CONST;
gint bdk_screen_height_mm (void) B_GNUC_CONST;

void bdk_beep (void);
#endif /* BDK_MULTIHEAD_SAFE */

void bdk_flush (void);

#ifndef BDK_MULTIHEAD_SAFE
void bdk_set_double_click_time             (guint       msec);
#endif

/* Rectangle utilities
 */
gboolean bdk_rectangle_intersect (const BdkRectangle *src1,
				  const BdkRectangle *src2,
				  BdkRectangle       *dest);
void     bdk_rectangle_union     (const BdkRectangle *src1,
				  const BdkRectangle *src2,
				  BdkRectangle       *dest);

GType bdk_rectangle_get_type (void) B_GNUC_CONST;

#define BDK_TYPE_RECTANGLE (bdk_rectangle_get_type ())

/* Conversion functions between wide char and multibyte strings. 
 */
#ifndef BDK_DISABLE_DEPRECATED
gchar     *bdk_wcstombs          (const BdkWChar   *src);
gint       bdk_mbstowcs          (BdkWChar         *dest,
				  const gchar      *src,
				  gint              dest_max);
#endif

/* Miscellaneous */
#ifndef BDK_MULTIHEAD_SAFE
gboolean bdk_event_send_client_message      (BdkEvent       *event,
					     BdkNativeWindow winid);
void     bdk_event_send_clientmessage_toall (BdkEvent  *event);
#endif
gboolean bdk_event_send_client_message_for_display (BdkDisplay *display,
						    BdkEvent       *event,
						    BdkNativeWindow winid);

void bdk_notify_startup_complete (void);

void bdk_notify_startup_complete_with_id (const gchar* startup_id);

/* Threading
 */

#if !defined (BDK_DISABLE_DEPRECATED) || defined (BDK_COMPILATION)
BDKVAR GMutex *bdk_threads_mutex; /* private */
#endif

BDKVAR GCallback bdk_threads_lock;
BDKVAR GCallback bdk_threads_unlock;

void     bdk_threads_enter                    (void);
void     bdk_threads_leave                    (void);
void     bdk_threads_init                     (void);
void     bdk_threads_set_lock_functions       (GCallback enter_fn,
					       GCallback leave_fn);

guint    bdk_threads_add_idle_full            (gint           priority,
		                               GSourceFunc    function,
		                               gpointer       data,
		                               GDestroyNotify notify);
guint    bdk_threads_add_idle                 (GSourceFunc    function,
		                               gpointer       data);
guint    bdk_threads_add_timeout_full         (gint           priority,
                                               guint          interval,
                                               GSourceFunc    function,
                                               gpointer       data,
                                               GDestroyNotify notify);
guint    bdk_threads_add_timeout              (guint          interval,
                                               GSourceFunc    function,
                                               gpointer       data);
guint    bdk_threads_add_timeout_seconds_full (gint           priority,
                                               guint          interval,
                                               GSourceFunc    function,
                                               gpointer       data,
                                               GDestroyNotify notify);
guint    bdk_threads_add_timeout_seconds      (guint          interval,
                                               GSourceFunc    function,
                                               gpointer       data);

#ifdef	G_THREADS_ENABLED
#  define BDK_THREADS_ENTER()	B_STMT_START {	\
      if (bdk_threads_lock)                 	\
        (*bdk_threads_lock) ();			\
   } B_STMT_END
#  define BDK_THREADS_LEAVE()	B_STMT_START { 	\
      if (bdk_threads_unlock)                 	\
        (*bdk_threads_unlock) ();		\
   } B_STMT_END
#else	/* !G_THREADS_ENABLED */
#  define BDK_THREADS_ENTER()
#  define BDK_THREADS_LEAVE()
#endif	/* !G_THREADS_ENABLED */

B_END_DECLS


#endif /* __BDK_H__ */
