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

#ifndef BTK_DISABLE_DEPRECATED

#ifndef __BTK_SIGNAL_H__
#define __BTK_SIGNAL_H__

#include <btk/btkenums.h>
#include <btk/btktypeutils.h>
#include <btk/btkobject.h>
#include <btk/btkmarshal.h>

B_BEGIN_DECLS

#define	btk_signal_default_marshaller	g_cclosure_marshal_VOID__VOID


/* --- compat defines --- */
#define BTK_SIGNAL_OFFSET	                      G_STRUCT_OFFSET
#define	btk_signal_lookup(name,object_type)	                                       \
   g_signal_lookup ((name), (object_type))
#define	btk_signal_name(signal_id)                                                     \
   g_signal_name (signal_id)
#define	btk_signal_emit_stop(object,signal_id)                                         \
   g_signal_stop_emission ((object), (signal_id), 0)
#define	btk_signal_connect(object,name,func,func_data)                                 \
   btk_signal_connect_full ((object), (name), (func), NULL, (func_data), NULL, 0, 0)
#define	btk_signal_connect_after(object,name,func,func_data)                           \
   btk_signal_connect_full ((object), (name), (func), NULL, (func_data), NULL, 0, 1)
#define	btk_signal_connect_object(object,name,func,slot_object)                        \
   btk_signal_connect_full ((object), (name), (func), NULL, (slot_object), NULL, 1, 0)
#define	btk_signal_connect_object_after(object,name,func,slot_object)                  \
   btk_signal_connect_full ((object), (name), (func), NULL, (slot_object), NULL, 1, 1)
#define	btk_signal_disconnect(object,handler_id)                                       \
   g_signal_handler_disconnect ((object), (handler_id))
#define	btk_signal_handler_block(object,handler_id)                                    \
   g_signal_handler_block ((object), (handler_id))
#define	btk_signal_handler_unblock(object,handler_id)                                  \
   g_signal_handler_unblock ((object), (handler_id))
#define	btk_signal_disconnect_by_func(object,func,data)	                               \
   btk_signal_compat_matched ((object), (func), (data),                                \
			      (GSignalMatchType)(G_SIGNAL_MATCH_FUNC |                 \
						 G_SIGNAL_MATCH_DATA), 0)
#define	btk_signal_disconnect_by_data(object,data)                                     \
   btk_signal_compat_matched ((object), 0, (data), G_SIGNAL_MATCH_DATA, 0)
#define	btk_signal_handler_block_by_func(object,func,data)                             \
   btk_signal_compat_matched ((object), (func), (data),                                \
			      (GSignalMatchType)(G_SIGNAL_MATCH_FUNC |                 \
						 G_SIGNAL_MATCH_DATA), 1)
#define	btk_signal_handler_block_by_data(object,data)                                  \
   btk_signal_compat_matched ((object), 0, (data), G_SIGNAL_MATCH_DATA, 1)
#define	btk_signal_handler_unblock_by_func(object,func,data)                           \
   btk_signal_compat_matched ((object), (func), (data),                                \
			      (GSignalMatchType)(G_SIGNAL_MATCH_FUNC |                 \
						 G_SIGNAL_MATCH_DATA), 2)
#define	btk_signal_handler_unblock_by_data(object,data)                                \
   btk_signal_compat_matched ((object), 0, (data), G_SIGNAL_MATCH_DATA, 2)
#define	btk_signal_handler_pending(object,signal_id,may_be_blocked)                    \
   g_signal_has_handler_pending ((object), (signal_id), 0, (may_be_blocked))
#define	btk_signal_handler_pending_by_func(object,signal_id,may_be_blocked,func,data)  \
   (g_signal_handler_find ((object),                                                             \
			   (GSignalMatchType)(G_SIGNAL_MATCH_ID |                                \
                                              G_SIGNAL_MATCH_FUNC |                              \
                                              G_SIGNAL_MATCH_DATA |                              \
                                              ((may_be_blocked) ? 0 : G_SIGNAL_MATCH_UNBLOCKED)),\
                           (signal_id), 0, 0, (func), (data)) != 0)


/* --- compat functions --- */
buint	btk_signal_newv				(const bchar	    *name,
						 BtkSignalRunType    signal_flags,
						 GType               object_type,
						 buint		     function_offset,
						 GSignalCMarshaller  marshaller,
						 GType               return_val,
						 buint		     n_args,
						 GType              *args);
buint	btk_signal_new				(const bchar	    *name,
						 BtkSignalRunType    signal_flags,
						 GType               object_type,
						 buint		     function_offset,
						 GSignalCMarshaller  marshaller,
						 GType               return_val,
						 buint		     n_args,
						 ...);
void	btk_signal_emit_stop_by_name		(BtkObject	    *object,
						 const bchar	    *name);
void	btk_signal_connect_object_while_alive	(BtkObject	    *object,
						 const bchar        *name,
						 GCallback	     func,
						 BtkObject	    *alive_object);
void	btk_signal_connect_while_alive		(BtkObject	    *object,
						 const bchar        *name,
						 GCallback	     func,
						 bpointer	     func_data,
						 BtkObject	    *alive_object);
bulong	btk_signal_connect_full			(BtkObject	    *object,
						 const bchar	    *name,
						 GCallback	     func,
						 BtkCallbackMarshal  unsupported,
						 bpointer	     data,
						 GDestroyNotify      destroy_func,
						 bint		     object_signal,
						 bint		     after);
void	btk_signal_emitv			(BtkObject	    *object,
						 buint		     signal_id,
						 BtkArg		    *args);
void	btk_signal_emit				(BtkObject	    *object,
						 buint		     signal_id,
						 ...);
void	btk_signal_emit_by_name			(BtkObject	    *object,
						 const bchar	    *name,
						 ...);
void	btk_signal_emitv_by_name		(BtkObject	    *object,
						 const bchar	    *name,
						 BtkArg		    *args);
void	btk_signal_compat_matched		(BtkObject	    *object,
						 GCallback 	     func,
						 bpointer      	     data,
						 GSignalMatchType    match,
						 buint               action);

B_END_DECLS

#endif /* __BTK_SIGNAL_H__ */

#endif /* BTK_DISABLE_DEPRECATED */
