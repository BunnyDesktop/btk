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

#undef BTK_DISABLE_DEPRECATED

#include	<config.h>
#include	"btksignal.h"
#include "btkalias.h"

/* the real parameter limit is of course given by GSignal, bu we need
 * an upper limit for the implementations. so this should be adjusted
 * with any future changes on the GSignal side of things.
 */
#define	SIGNAL_MAX_PARAMS	12


/* --- functions --- */
buint
btk_signal_newv (const bchar         *name,
		 BtkSignalRunType     signal_flags,
		 GType                object_type,
		 buint                function_offset,
		 GSignalCMarshaller   marshaller,
		 GType                return_val,
		 buint                n_params,
		 GType               *params)
{
  GClosure *closure;
  
  g_return_val_if_fail (n_params < SIGNAL_MAX_PARAMS, 0);
  
  closure = function_offset ? g_signal_type_cclosure_new (object_type, function_offset) : NULL;
  
  return g_signal_newv (name, object_type, (GSignalFlags)signal_flags, closure,
			NULL, NULL, marshaller, return_val, n_params, params);
}

buint
btk_signal_new (const bchar         *name,
		BtkSignalRunType     signal_flags,
		GType                object_type,
		buint                function_offset,
		GSignalCMarshaller   marshaller,
		GType                return_val,
		buint                n_params,
		...)
{
  GType *params;
  buint signal_id;
  
  if (n_params)
    {
      va_list args;
      buint i;
      
      params = g_new (GType, n_params);
      va_start (args, n_params);
      for (i = 0; i < n_params; i++)
	params[i] = va_arg (args, GType);
      va_end (args);
    }
  else
    params = NULL;
  signal_id = btk_signal_newv (name,
			       signal_flags,
			       object_type,
			       function_offset,
			       marshaller,
			       return_val,
			       n_params,
			       params);
  g_free (params);
  
  return signal_id;
}

void
btk_signal_emit_stop_by_name (BtkObject   *object,
			      const bchar *name)
{
  g_return_if_fail (BTK_IS_OBJECT (object));
  
  g_signal_stop_emission (object, g_signal_lookup (name, B_OBJECT_TYPE (object)), 0);
}

void
btk_signal_connect_object_while_alive (BtkObject    *object,
				       const bchar  *name,
				       GCallback     func,
				       BtkObject    *alive_object)
{
  g_return_if_fail (BTK_IS_OBJECT (object));
  
  g_signal_connect_closure_by_id (object,
				  g_signal_lookup (name, B_OBJECT_TYPE (object)), 0,
				  g_cclosure_new_object_swap (func, B_OBJECT (alive_object)),
				  FALSE);
}

void
btk_signal_connect_while_alive (BtkObject    *object,
				const bchar  *name,
				GCallback     func,
				bpointer      func_data,
				BtkObject    *alive_object)
{
  GClosure *closure;

  g_return_if_fail (BTK_IS_OBJECT (object));

  closure = g_cclosure_new (func, func_data, NULL);
  g_object_watch_closure (B_OBJECT (alive_object), closure);
  g_signal_connect_closure_by_id (object,
				  g_signal_lookup (name, B_OBJECT_TYPE (object)), 0,
				  closure,
				  FALSE);
}

bulong
btk_signal_connect_full (BtkObject           *object,
			 const bchar         *name,
			 GCallback            func,
			 BtkCallbackMarshal   unsupported,
			 bpointer             data,
			 GDestroyNotify       destroy_func,
			 bint                 object_signal,
			 bint                 after)
{
  g_return_val_if_fail (BTK_IS_OBJECT (object), 0);
  g_return_val_if_fail (unsupported == NULL, 0);
  
  return g_signal_connect_closure_by_id (object,
					 g_signal_lookup (name, B_OBJECT_TYPE (object)), 0,
					 (object_signal
					  ? g_cclosure_new_swap
					  : g_cclosure_new) (func,
							     data,
							     (GClosureNotify) destroy_func),
					 after);
}

void
btk_signal_compat_matched (BtkObject       *object,
			   GCallback        func,
			   bpointer         data,
			   GSignalMatchType match,
			   buint            action)
{
  buint n_handlers;
  
  g_return_if_fail (BTK_IS_OBJECT (object));

  switch (action)
    {
    case 0:  n_handlers = g_signal_handlers_disconnect_matched (object, match, 0, 0, NULL, (bpointer) func, data);	 break;
    case 1:  n_handlers = g_signal_handlers_block_matched (object, match, 0, 0, NULL, (bpointer) func, data);	 break;
    case 2:  n_handlers = g_signal_handlers_unblock_matched (object, match, 0, 0, NULL, (bpointer) func, data);	 break;
    default: n_handlers = 0;										 break;
    }
  
  if (!n_handlers)
    g_warning ("unable to find signal handler for object(%s:%p) with func(%p) and data(%p)",
	       B_OBJECT_TYPE_NAME (object), object, func, data);
}

static inline bboolean
btk_arg_to_value (BtkArg *arg,
		  BValue *value)
{
  switch (B_TYPE_FUNDAMENTAL (arg->type))
    {
    case B_TYPE_CHAR:		b_value_set_char (value, BTK_VALUE_CHAR (*arg));	break;
    case B_TYPE_UCHAR:		b_value_set_uchar (value, BTK_VALUE_UCHAR (*arg));	break;
    case B_TYPE_BOOLEAN:	b_value_set_boolean (value, BTK_VALUE_BOOL (*arg));	break;
    case B_TYPE_INT:		b_value_set_int (value, BTK_VALUE_INT (*arg));		break;
    case B_TYPE_UINT:		b_value_set_uint (value, BTK_VALUE_UINT (*arg));	break;
    case B_TYPE_LONG:		b_value_set_long (value, BTK_VALUE_LONG (*arg));	break;
    case B_TYPE_ULONG:		b_value_set_ulong (value, BTK_VALUE_ULONG (*arg));	break;
    case B_TYPE_ENUM:		b_value_set_enum (value, BTK_VALUE_ENUM (*arg));	break;
    case B_TYPE_FLAGS:		b_value_set_flags (value, BTK_VALUE_FLAGS (*arg));	break;
    case B_TYPE_FLOAT:		b_value_set_float (value, BTK_VALUE_FLOAT (*arg));	break;
    case B_TYPE_DOUBLE:		b_value_set_double (value, BTK_VALUE_DOUBLE (*arg));	break;
    case B_TYPE_STRING:		b_value_set_string (value, BTK_VALUE_STRING (*arg));	break;
    case B_TYPE_BOXED:		b_value_set_boxed (value, BTK_VALUE_BOXED (*arg));	break;
    case B_TYPE_POINTER:	b_value_set_pointer (value, BTK_VALUE_POINTER (*arg));	break;
    case B_TYPE_OBJECT:		b_value_set_object (value, BTK_VALUE_POINTER (*arg));	break;
    default:
      return FALSE;
    }
  return TRUE;
}

static inline bboolean
btk_arg_static_to_value (BtkArg *arg,
			 BValue *value)
{
  switch (B_TYPE_FUNDAMENTAL (arg->type))
    {
    case B_TYPE_CHAR:		b_value_set_char (value, BTK_VALUE_CHAR (*arg));		break;
    case B_TYPE_UCHAR:		b_value_set_uchar (value, BTK_VALUE_UCHAR (*arg));		break;
    case B_TYPE_BOOLEAN:	b_value_set_boolean (value, BTK_VALUE_BOOL (*arg));		break;
    case B_TYPE_INT:		b_value_set_int (value, BTK_VALUE_INT (*arg));			break;
    case B_TYPE_UINT:		b_value_set_uint (value, BTK_VALUE_UINT (*arg));		break;
    case B_TYPE_LONG:		b_value_set_long (value, BTK_VALUE_LONG (*arg));		break;
    case B_TYPE_ULONG:		b_value_set_ulong (value, BTK_VALUE_ULONG (*arg));		break;
    case B_TYPE_ENUM:		b_value_set_enum (value, BTK_VALUE_ENUM (*arg));		break;
    case B_TYPE_FLAGS:		b_value_set_flags (value, BTK_VALUE_FLAGS (*arg));		break;
    case B_TYPE_FLOAT:		b_value_set_float (value, BTK_VALUE_FLOAT (*arg));		break;
    case B_TYPE_DOUBLE:		b_value_set_double (value, BTK_VALUE_DOUBLE (*arg));		break;
    case B_TYPE_STRING:		b_value_set_static_string (value, BTK_VALUE_STRING (*arg));	break;
    case B_TYPE_BOXED:		b_value_set_static_boxed (value, BTK_VALUE_BOXED (*arg));	break;
    case B_TYPE_POINTER:	b_value_set_pointer (value, BTK_VALUE_POINTER (*arg));		break;
    case B_TYPE_OBJECT:		b_value_set_object (value, BTK_VALUE_POINTER (*arg));		break;
    default:
      return FALSE;
    }
  return TRUE;
}

static inline bboolean
btk_arg_set_from_value (BtkArg  *arg,
			BValue  *value,
			bboolean copy_string)
{
  switch (B_TYPE_FUNDAMENTAL (arg->type))
    {
    case B_TYPE_CHAR:		BTK_VALUE_CHAR (*arg) = b_value_get_char (value);	break;
    case B_TYPE_UCHAR:		BTK_VALUE_UCHAR (*arg) = b_value_get_uchar (value);	break;
    case B_TYPE_BOOLEAN:	BTK_VALUE_BOOL (*arg) = b_value_get_boolean (value);	break;
    case B_TYPE_INT:		BTK_VALUE_INT (*arg) = b_value_get_int (value);		break;
    case B_TYPE_UINT:		BTK_VALUE_UINT (*arg) = b_value_get_uint (value);	break;
    case B_TYPE_LONG:		BTK_VALUE_LONG (*arg) = b_value_get_long (value);	break;
    case B_TYPE_ULONG:		BTK_VALUE_ULONG (*arg) = b_value_get_ulong (value);	break;
    case B_TYPE_ENUM:		BTK_VALUE_ENUM (*arg) = b_value_get_enum (value);	break;
    case B_TYPE_FLAGS:		BTK_VALUE_FLAGS (*arg) = b_value_get_flags (value);	break;
    case B_TYPE_FLOAT:		BTK_VALUE_FLOAT (*arg) = b_value_get_float (value);	break;
    case B_TYPE_DOUBLE:		BTK_VALUE_DOUBLE (*arg) = b_value_get_double (value);	break;
    case B_TYPE_BOXED:		BTK_VALUE_BOXED (*arg) = b_value_get_boxed (value);	break;
    case B_TYPE_POINTER:	BTK_VALUE_POINTER (*arg) = b_value_get_pointer (value);	break;
    case B_TYPE_OBJECT:		BTK_VALUE_POINTER (*arg) = b_value_get_object (value);	break;
    case B_TYPE_STRING:		if (copy_string)
      BTK_VALUE_STRING (*arg) = b_value_dup_string (value);
    else
      BTK_VALUE_STRING (*arg) = (char *) b_value_get_string (value);
    break;
    default:
      return FALSE;
    }
  return TRUE;
}

static inline bboolean
btk_argloc_set_from_value (BtkArg  *arg,
			   BValue  *value,
			   bboolean copy_string)
{
  switch (B_TYPE_FUNDAMENTAL (arg->type))
    {
    case B_TYPE_CHAR:		*BTK_RETLOC_CHAR (*arg) = b_value_get_char (value);	  break;
    case B_TYPE_UCHAR:		*BTK_RETLOC_UCHAR (*arg) = b_value_get_uchar (value);	  break;
    case B_TYPE_BOOLEAN:	*BTK_RETLOC_BOOL (*arg) = b_value_get_boolean (value);	  break;
    case B_TYPE_INT:		*BTK_RETLOC_INT (*arg) = b_value_get_int (value);	  break;
    case B_TYPE_UINT:		*BTK_RETLOC_UINT (*arg) = b_value_get_uint (value);	  break;
    case B_TYPE_LONG:		*BTK_RETLOC_LONG (*arg) = b_value_get_long (value);	  break;
    case B_TYPE_ULONG:		*BTK_RETLOC_ULONG (*arg) = b_value_get_ulong (value);	  break;
    case B_TYPE_ENUM:		*BTK_RETLOC_ENUM (*arg) = b_value_get_enum (value);	  break;
    case B_TYPE_FLAGS:		*BTK_RETLOC_FLAGS (*arg) = b_value_get_flags (value);	  break;
    case B_TYPE_FLOAT:		*BTK_RETLOC_FLOAT (*arg) = b_value_get_float (value);	  break;
    case B_TYPE_DOUBLE:		*BTK_RETLOC_DOUBLE (*arg) = b_value_get_double (value);	  break;
    case B_TYPE_BOXED:		*BTK_RETLOC_BOXED (*arg) = b_value_get_boxed (value);	  break;
    case B_TYPE_POINTER:	*BTK_RETLOC_POINTER (*arg) = b_value_get_pointer (value); break;
    case B_TYPE_OBJECT:		*BTK_RETLOC_POINTER (*arg) = b_value_get_object (value);  break;
    case B_TYPE_STRING:		if (copy_string)
      *BTK_RETLOC_STRING (*arg) = b_value_dup_string (value);
    else
      *BTK_RETLOC_STRING (*arg) = (char *) b_value_get_string (value);
    break;
    default:
      return FALSE;
    }
  return TRUE;
}

void
btk_signal_emitv (BtkObject *object,
		  buint      signal_id,
		  BtkArg    *args)
{
  GSignalQuery query;
  BValue params[SIGNAL_MAX_PARAMS + 1] = { { 0, }, };
  BValue rvalue = { 0, };
  buint i;
  
  g_return_if_fail (BTK_IS_OBJECT (object));
  
  g_signal_query (signal_id, &query);
  g_return_if_fail (query.signal_id != 0);
  g_return_if_fail (g_type_is_a (BTK_OBJECT_TYPE (object), query.itype));
  g_return_if_fail (query.n_params < SIGNAL_MAX_PARAMS);
  if (query.n_params > 0)
    g_return_if_fail (args != NULL);
  
  b_value_init (params + 0, BTK_OBJECT_TYPE (object));
  b_value_set_object (params + 0, B_OBJECT (object));
  for (i = 0; i < query.n_params; i++)
    {
      BValue *value = params + 1 + i;
      BtkArg *arg = args + i;
      
      b_value_init (value, arg->type & ~G_SIGNAL_TYPE_STATIC_SCOPE);
      if (!btk_arg_static_to_value (arg, value))
	{
	  g_warning ("%s: failed to convert arg type `%s' to value type `%s'",
		     B_STRLOC, g_type_name (arg->type & ~G_SIGNAL_TYPE_STATIC_SCOPE),
		     g_type_name (G_VALUE_TYPE (value)));
	  return;
	}
    }
  if (query.return_type != B_TYPE_NONE)
    b_value_init (&rvalue, query.return_type);
  
  g_signal_emitv (params, signal_id, 0, &rvalue);
  
  if (query.return_type != B_TYPE_NONE)
    {
      btk_argloc_set_from_value (args + query.n_params, &rvalue, TRUE);
      b_value_unset (&rvalue);
    }
  for (i = 0; i < query.n_params; i++)
    b_value_unset (params + 1 + i);
  b_value_unset (params + 0);
}

void
btk_signal_emit (BtkObject *object,
		 buint      signal_id,
		 ...)
{
  va_list var_args;
  
  g_return_if_fail (BTK_IS_OBJECT (object));

  va_start (var_args, signal_id);
  g_signal_emit_valist (B_OBJECT (object), signal_id, 0, var_args);
  va_end (var_args);
}

void
btk_signal_emit_by_name (BtkObject   *object,
			 const bchar *name,
			 ...)
{
  GSignalQuery query;
  va_list var_args;
  
  g_return_if_fail (BTK_IS_OBJECT (object));
  g_return_if_fail (name != NULL);
  
  g_signal_query (g_signal_lookup (name, BTK_OBJECT_TYPE (object)), &query);
  g_return_if_fail (query.signal_id != 0);
  
  va_start (var_args, name);
  g_signal_emit_valist (B_OBJECT (object), query.signal_id, 0, var_args);
  va_end (var_args);
}

void
btk_signal_emitv_by_name (BtkObject   *object,
			  const bchar *name,
			  BtkArg      *args)
{
  g_return_if_fail (BTK_IS_OBJECT (object));
  
  btk_signal_emitv (object, g_signal_lookup (name, BTK_OBJECT_TYPE (object)), args);
}

#define __BTK_SIGNAL_C__
#include "btkaliasdef.c"
