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

B_BEGIN_DECLS

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

typedef bint	(*BtkKeySnoopFunc)	    (BtkWidget	  *grab_widget,
					     BdkEventKey  *event,
					     bpointer	   func_data);

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

BTKMAIN_C_VAR const buint btk_major_version;
BTKMAIN_C_VAR const buint btk_minor_version;
BTKMAIN_C_VAR const buint btk_micro_version;
BTKMAIN_C_VAR const buint btk_binary_age;
BTKMAIN_C_VAR const buint btk_interface_age;
const bchar* btk_check_version (buint	required_major,
			        buint	required_minor,
			        buint	required_micro);


/* Initialization, exit, mainloop and miscellaneous routines
 */

bboolean btk_parse_args           (int    *argc,
				   char ***argv);

void     btk_init                 (int    *argc,
                                   char ***argv);

bboolean btk_init_check           (int    *argc,
                                   char ***argv);
  
bboolean btk_init_with_args       (int            *argc,
				   char         ***argv,
				   const char     *parameter_string,
				   GOptionEntry   *entries,
				   const char     *translation_domain,
				   GError        **error);

GOptionGroup *btk_get_option_group (bboolean open_default_display);
  
#ifdef G_PLATFORM_WIN32

/* Variants that are used to check for correct struct packing
 * when building BTK+-using code.
 */
void	 btk_init_abi_check       (int	  *argc,
				   char	***argv,
				   int     num_checks,
				   size_t  sizeof_BtkWindow,
				   size_t  sizeof_BtkBox);
bboolean btk_init_check_abi_check (int	  *argc,
				   char	***argv,
				   int     num_checks,
				   size_t  sizeof_BtkWindow,
				   size_t  sizeof_BtkBox);

#define btk_init(argc, argv) btk_init_abi_check (argc, argv, 2, sizeof (BtkWindow), sizeof (BtkBox))
#define btk_init_check(argc, argv) btk_init_check_abi_check (argc, argv, 2, sizeof (BtkWindow), sizeof (BtkBox))

#endif

#ifndef BTK_DISABLE_DEPRECATED
void     btk_exit                 (bint    error_code);
bchar *        btk_set_locale           (void);
#endif /* BTK_DISABLE_DEPRECATED */

void           btk_disable_setlocale    (void);
BangoLanguage *btk_get_default_language (void);
bboolean       btk_events_pending       (void);

/* The following is the event func BTK+ registers with BDK
 * we expose it mainly to allow filtering of events between
 * BDK and BTK+.
 */
void 	   btk_main_do_event	   (BdkEvent           *event);

void	   btk_main		   (void);
buint	   btk_main_level	   (void);
void	   btk_main_quit	   (void);
bboolean   btk_main_iteration	   (void);
/* btk_main_iteration() calls btk_main_iteration_do(TRUE) */
bboolean   btk_main_iteration_do   (bboolean blocking);

bboolean   btk_true		   (void) B_GNUC_CONST;
bboolean   btk_false		   (void) B_GNUC_CONST;

void	   btk_grab_add		   (BtkWidget	       *widget);
BtkWidget* btk_grab_get_current	   (void);
void	   btk_grab_remove	   (BtkWidget	       *widget);

#if !defined (BTK_DISABLE_DEPRECATED) || defined (BTK_COMPILATION)
void	   btk_init_add		   (BtkFunction	       function,
				    bpointer	       data);
void	   btk_quit_add_destroy	   (buint	       main_level,
				    BtkObject	      *object);
buint	   btk_quit_add		   (buint	       main_level,
				    BtkFunction	       function,
				    bpointer	       data);
buint	   btk_quit_add_full	   (buint	       main_level,
				    BtkFunction	       function,
				    BtkCallbackMarshal marshal,
				    bpointer	       data,
				    GDestroyNotify     destroy);
void	   btk_quit_remove	   (buint	       quit_handler_id);
void	   btk_quit_remove_by_data (bpointer	       data);
buint	   btk_timeout_add	   (buint32	       interval,
				    BtkFunction	       function,
				    bpointer	       data);
buint	   btk_timeout_add_full	   (buint32	       interval,
				    BtkFunction	       function,
				    BtkCallbackMarshal marshal,
				    bpointer	       data,
				    GDestroyNotify     destroy);
void	   btk_timeout_remove	   (buint	       timeout_handler_id);

buint	   btk_idle_add		   (BtkFunction	       function,
				    bpointer	       data);
buint	   btk_idle_add_priority   (bint	       priority,
				    BtkFunction	       function,
				    bpointer	       data);
buint	   btk_idle_add_full	   (bint	       priority,
				    BtkFunction	       function,
				    BtkCallbackMarshal marshal,
				    bpointer	       data,
				    GDestroyNotify     destroy);
void	   btk_idle_remove	   (buint	       idle_handler_id);
void	   btk_idle_remove_by_data (bpointer	       data);
buint	   btk_input_add_full	   (bint	       source,
				    BdkInputCondition  condition,
				    BdkInputFunction   function,
				    BtkCallbackMarshal marshal,
				    bpointer	       data,
				    GDestroyNotify     destroy);
void	   btk_input_remove	   (buint	       input_handler_id);
#endif /* BTK_DISABLE_DEPRECATED */

buint	   btk_key_snooper_install (BtkKeySnoopFunc snooper,
				    bpointer	    func_data);
void	   btk_key_snooper_remove  (buint	    snooper_handler_id);

BdkEvent*       btk_get_current_event       (void);
buint32         btk_get_current_event_time  (void);
bboolean        btk_get_current_event_state (BdkModifierType *state);

BtkWidget* btk_get_event_widget	   (BdkEvent	   *event);


/* Private routines internal to BTK+ 
 */
void       btk_propagate_event     (BtkWidget         *widget,
				    BdkEvent          *event);

bboolean _btk_boolean_handled_accumulator (GSignalInvocationHint *ihint,
                                   BValue                *return_accu,
                                   const BValue          *handler_return,
                                   bpointer               dummy);

bchar *_btk_get_lc_ctype (void);

bboolean _btk_module_has_mixed_deps (GModule *module);


B_END_DECLS

#endif /* __BTK_MAIN_H__ */
