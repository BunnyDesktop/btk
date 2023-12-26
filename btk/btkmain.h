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

#ifndef __BTK_MAIN_H__
#define __BTK_MAIN_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <bdk/bdk.h>
#include <btk/btkwidget.h>
#ifdef G_PLATFORM_WIN32
#include <btk/btkbox.h>
#include <btk/btkwindow.h>
#endif

G_BEGIN_DECLS

/* Priorities for redrawing and resizing
 */
#define BTK_PRIORITY_RESIZE     (G_PRIORITY_HIGH_IDLE + 10)

#ifndef BTK_DISABLE_DEPRECATED

/* Use BDK_PRIORITY_REDRAW */
#define BTK_PRIORITY_REDRAW     (G_PRIORITY_HIGH_IDLE + 20)

/* Deprecated. Use G_PRIORITY #define's instead
 */
#define BTK_PRIORITY_HIGH       G_PRIORITY_HIGH
#define BTK_PRIORITY_INTERNAL   BTK_PRIORITY_REDRAW
#define BTK_PRIORITY_DEFAULT	G_PRIORITY_DEFAULT_IDLE
#define BTK_PRIORITY_LOW	G_PRIORITY_LOW

#endif /* BTK_DISABLE_DEPRECATED */

typedef gint	(*BtkKeySnoopFunc)	    (BtkWidget	  *grab_widget,
					     BdkEventKey  *event,
					     gpointer	   func_data);

/* Btk version.
 */
#ifdef G_PLATFORM_WIN32
#ifdef BTK_COMPILATION
#define BTKMAIN_C_VAR extern __declspec(dllexport)
#else
#define BTKMAIN_C_VAR extern __declspec(dllimport)
#endif
#else
#define BTKMAIN_C_VAR extern
#endif

BTKMAIN_C_VAR const guint btk_major_version;
BTKMAIN_C_VAR const guint btk_minor_version;
BTKMAIN_C_VAR const guint btk_micro_version;
BTKMAIN_C_VAR const guint btk_binary_age;
BTKMAIN_C_VAR const guint btk_interface_age;
const gchar* btk_check_version (guint	required_major,
			        guint	required_minor,
			        guint	required_micro);


/* Initialization, exit, mainloop and miscellaneous routines
 */

gboolean btk_parse_args           (int    *argc,
				   char ***argv);

void     btk_init                 (int    *argc,
                                   char ***argv);

gboolean btk_init_check           (int    *argc,
                                   char ***argv);
  
gboolean btk_init_with_args       (int            *argc,
				   char         ***argv,
				   const char     *parameter_string,
				   GOptionEntry   *entries,
				   const char     *translation_domain,
				   GError        **error);

GOptionGroup *btk_get_option_group (gboolean open_default_display);
  
#ifdef G_PLATFORM_WIN32

/* Variants that are used to check for correct struct packing
 * when building BTK+-using code.
 */
void	 btk_init_abi_check       (int	  *argc,
				   char	***argv,
				   int     num_checks,
				   size_t  sizeof_BtkWindow,
				   size_t  sizeof_BtkBox);
gboolean btk_init_check_abi_check (int	  *argc,
				   char	***argv,
				   int     num_checks,
				   size_t  sizeof_BtkWindow,
				   size_t  sizeof_BtkBox);

#define btk_init(argc, argv) btk_init_abi_check (argc, argv, 2, sizeof (BtkWindow), sizeof (BtkBox))
#define btk_init_check(argc, argv) btk_init_check_abi_check (argc, argv, 2, sizeof (BtkWindow), sizeof (BtkBox))

#endif

#ifndef BTK_DISABLE_DEPRECATED
void     btk_exit                 (gint    error_code);
gchar *        btk_set_locale           (void);
#endif /* BTK_DISABLE_DEPRECATED */

void           btk_disable_setlocale    (void);
BangoLanguage *btk_get_default_language (void);
gboolean       btk_events_pending       (void);

/* The following is the event func BTK+ registers with BDK
 * we expose it mainly to allow filtering of events between
 * BDK and BTK+.
 */
void 	   btk_main_do_event	   (BdkEvent           *event);

void	   btk_main		   (void);
guint	   btk_main_level	   (void);
void	   btk_main_quit	   (void);
gboolean   btk_main_iteration	   (void);
/* btk_main_iteration() calls btk_main_iteration_do(TRUE) */
gboolean   btk_main_iteration_do   (gboolean blocking);

gboolean   btk_true		   (void) G_GNUC_CONST;
gboolean   btk_false		   (void) G_GNUC_CONST;

void	   btk_grab_add		   (BtkWidget	       *widget);
BtkWidget* btk_grab_get_current	   (void);
void	   btk_grab_remove	   (BtkWidget	       *widget);

#if !defined (BTK_DISABLE_DEPRECATED) || defined (BTK_COMPILATION)
void	   btk_init_add		   (BtkFunction	       function,
				    gpointer	       data);
void	   btk_quit_add_destroy	   (guint	       main_level,
				    BtkObject	      *object);
guint	   btk_quit_add		   (guint	       main_level,
				    BtkFunction	       function,
				    gpointer	       data);
guint	   btk_quit_add_full	   (guint	       main_level,
				    BtkFunction	       function,
				    BtkCallbackMarshal marshal,
				    gpointer	       data,
				    GDestroyNotify     destroy);
void	   btk_quit_remove	   (guint	       quit_handler_id);
void	   btk_quit_remove_by_data (gpointer	       data);
guint	   btk_timeout_add	   (guint32	       interval,
				    BtkFunction	       function,
				    gpointer	       data);
guint	   btk_timeout_add_full	   (guint32	       interval,
				    BtkFunction	       function,
				    BtkCallbackMarshal marshal,
				    gpointer	       data,
				    GDestroyNotify     destroy);
void	   btk_timeout_remove	   (guint	       timeout_handler_id);

guint	   btk_idle_add		   (BtkFunction	       function,
				    gpointer	       data);
guint	   btk_idle_add_priority   (gint	       priority,
				    BtkFunction	       function,
				    gpointer	       data);
guint	   btk_idle_add_full	   (gint	       priority,
				    BtkFunction	       function,
				    BtkCallbackMarshal marshal,
				    gpointer	       data,
				    GDestroyNotify     destroy);
void	   btk_idle_remove	   (guint	       idle_handler_id);
void	   btk_idle_remove_by_data (gpointer	       data);
guint	   btk_input_add_full	   (gint	       source,
				    BdkInputCondition  condition,
				    BdkInputFunction   function,
				    BtkCallbackMarshal marshal,
				    gpointer	       data,
				    GDestroyNotify     destroy);
void	   btk_input_remove	   (guint	       input_handler_id);
#endif /* BTK_DISABLE_DEPRECATED */

guint	   btk_key_snooper_install (BtkKeySnoopFunc snooper,
				    gpointer	    func_data);
void	   btk_key_snooper_remove  (guint	    snooper_handler_id);

BdkEvent*       btk_get_current_event       (void);
guint32         btk_get_current_event_time  (void);
gboolean        btk_get_current_event_state (BdkModifierType *state);

BtkWidget* btk_get_event_widget	   (BdkEvent	   *event);


/* Private routines internal to BTK+ 
 */
void       btk_propagate_event     (BtkWidget         *widget,
				    BdkEvent          *event);

gboolean _btk_boolean_handled_accumulator (GSignalInvocationHint *ihint,
                                   GValue                *return_accu,
                                   const GValue          *handler_return,
                                   gpointer               dummy);

gchar *_btk_get_lc_ctype (void);

gboolean _btk_module_has_mixed_deps (GModule *module);


G_END_DECLS

#endif /* __BTK_MAIN_H__ */
