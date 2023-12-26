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

#include "config.h"

#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#undef BTK_DISABLE_DEPRECATED

#include "btkobject.h"
#include "btkintl.h"
#include "btkmarshalers.h"
#include "btkprivate.h"

#include "btkalias.h"


enum {
  DESTROY,
  LAST_SIGNAL
};
enum {
  PROP_0,
  PROP_USER_DATA
};


static void       btk_object_base_class_init     (BtkObjectClass *class);
static void       btk_object_base_class_finalize (BtkObjectClass *class);
static void       btk_object_class_init          (BtkObjectClass *klass);
static void       btk_object_init                (BtkObject      *object,
						  BtkObjectClass *klass);
static void	  btk_object_set_property	 (BObject	 *object,
						  guint           property_id,
						  const BValue   *value,
						  BParamSpec     *pspec);
static void	  btk_object_get_property	 (BObject	 *object,
						  guint           property_id,
						  BValue         *value,
						  BParamSpec     *pspec);
static void       btk_object_dispose            (BObject        *object);
static void       btk_object_real_destroy        (BtkObject      *object);
static void       btk_object_finalize            (BObject        *object);
static void       btk_object_notify_weaks        (BtkObject      *object);

static gpointer    parent_class = NULL;
static guint       object_signals[LAST_SIGNAL] = { 0 };
static GQuark      quark_weakrefs = 0;


/****************************************************
 * BtkObject type, class and instance initialization
 *
 ****************************************************/

GType
btk_object_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      const GTypeInfo object_info =
      {
	sizeof (BtkObjectClass),
	(GBaseInitFunc) btk_object_base_class_init,
	(GBaseFinalizeFunc) btk_object_base_class_finalize,
	(GClassInitFunc) btk_object_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (BtkObject),
	16,		/* n_preallocs */
	(GInstanceInitFunc) btk_object_init,
	NULL,		/* value_table */
      };
      
      object_type = g_type_register_static (B_TYPE_INITIALLY_UNOWNED, I_("BtkObject"), 
					    &object_info, B_TYPE_FLAG_ABSTRACT);
    }

  return object_type;
}

static void
btk_object_base_class_init (BtkObjectClass *class)
{
  /* reset instance specifc methods that don't get inherited */
  class->get_arg = NULL;
  class->set_arg = NULL;
}

static void
btk_object_base_class_finalize (BtkObjectClass *class)
{
}

static inline gboolean
btk_arg_set_from_value (BtkArg       *arg,
			const BValue *value,
			gboolean      copy_string)
{
  switch (B_TYPE_FUNDAMENTAL (arg->type))
    {
    case B_TYPE_CHAR:           BTK_VALUE_CHAR (*arg) = b_value_get_char (value);       break;
    case B_TYPE_UCHAR:          BTK_VALUE_UCHAR (*arg) = b_value_get_uchar (value);     break;
    case B_TYPE_BOOLEAN:        BTK_VALUE_BOOL (*arg) = b_value_get_boolean (value);    break;
    case B_TYPE_INT:            BTK_VALUE_INT (*arg) = b_value_get_int (value);         break;
    case B_TYPE_UINT:           BTK_VALUE_UINT (*arg) = b_value_get_uint (value);       break;
    case B_TYPE_LONG:           BTK_VALUE_LONG (*arg) = b_value_get_long (value);       break;
    case B_TYPE_ULONG:          BTK_VALUE_ULONG (*arg) = b_value_get_ulong (value);     break;
    case B_TYPE_ENUM:           BTK_VALUE_ENUM (*arg) = b_value_get_enum (value);       break;
    case B_TYPE_FLAGS:          BTK_VALUE_FLAGS (*arg) = b_value_get_flags (value);     break;
    case B_TYPE_FLOAT:          BTK_VALUE_FLOAT (*arg) = b_value_get_float (value);     break;
    case B_TYPE_DOUBLE:         BTK_VALUE_DOUBLE (*arg) = b_value_get_double (value);   break;
    case B_TYPE_BOXED:          BTK_VALUE_BOXED (*arg) = b_value_get_boxed (value);     break;
    case B_TYPE_POINTER:        BTK_VALUE_POINTER (*arg) = b_value_get_pointer (value); break;
    case B_TYPE_OBJECT:         BTK_VALUE_POINTER (*arg) = b_value_get_object (value);  break;
    case B_TYPE_STRING:         if (copy_string)
      BTK_VALUE_STRING (*arg) = b_value_dup_string (value);
    else
      BTK_VALUE_STRING (*arg) = (char *) b_value_get_string (value);
    break;
    default:
      return FALSE;
    }
  return TRUE;
}

static inline gboolean
btk_arg_to_value (BtkArg *arg,
		  BValue *value)
{
  switch (B_TYPE_FUNDAMENTAL (arg->type))
    {
    case B_TYPE_CHAR:           b_value_set_char (value, BTK_VALUE_CHAR (*arg));        break;
    case B_TYPE_UCHAR:          b_value_set_uchar (value, BTK_VALUE_UCHAR (*arg));      break;
    case B_TYPE_BOOLEAN:        b_value_set_boolean (value, BTK_VALUE_BOOL (*arg));     break;
    case B_TYPE_INT:            b_value_set_int (value, BTK_VALUE_INT (*arg));          break;
    case B_TYPE_UINT:           b_value_set_uint (value, BTK_VALUE_UINT (*arg));        break;
    case B_TYPE_LONG:           b_value_set_long (value, BTK_VALUE_LONG (*arg));        break;
    case B_TYPE_ULONG:          b_value_set_ulong (value, BTK_VALUE_ULONG (*arg));      break;
    case B_TYPE_ENUM:           b_value_set_enum (value, BTK_VALUE_ENUM (*arg));        break;
    case B_TYPE_FLAGS:          b_value_set_flags (value, BTK_VALUE_FLAGS (*arg));      break;
    case B_TYPE_FLOAT:          b_value_set_float (value, BTK_VALUE_FLOAT (*arg));      break;
    case B_TYPE_DOUBLE:         b_value_set_double (value, BTK_VALUE_DOUBLE (*arg));    break;
    case B_TYPE_STRING:         b_value_set_string (value, BTK_VALUE_STRING (*arg));    break;
    case B_TYPE_BOXED:          b_value_set_boxed (value, BTK_VALUE_BOXED (*arg));      break;
    case B_TYPE_POINTER:        b_value_set_pointer (value, BTK_VALUE_POINTER (*arg));  break;
    case B_TYPE_OBJECT:         b_value_set_object (value, BTK_VALUE_POINTER (*arg));   break;
    default:
      return FALSE;
    }
  return TRUE;
}

static void
btk_arg_proxy_set_property (BObject      *object,
			    guint         property_id,
			    const BValue *value,
			    BParamSpec   *pspec)
{
  BtkObjectClass *class = g_type_class_peek (pspec->owner_type);
  BtkArg arg;

  g_return_if_fail (class->set_arg != NULL);

  memset (&arg, 0, sizeof (arg));
  arg.type = G_VALUE_TYPE (value);
  btk_arg_set_from_value (&arg, value, FALSE);
  arg.name = pspec->name;
  class->set_arg (BTK_OBJECT (object), &arg, property_id);
}

static void
btk_arg_proxy_get_property (BObject     *object,
			    guint        property_id,
			    BValue      *value,
			    BParamSpec  *pspec)
{
  BtkObjectClass *class = g_type_class_peek (pspec->owner_type);
  BtkArg arg;

  g_return_if_fail (class->get_arg != NULL);

  memset (&arg, 0, sizeof (arg));
  arg.type = G_VALUE_TYPE (value);
  arg.name = pspec->name;
  class->get_arg (BTK_OBJECT (object), &arg, property_id);
  btk_arg_to_value (&arg, value);
}

void
btk_object_add_arg_type (const gchar *arg_name,
			 GType        arg_type,
			 guint        arg_flags,
			 guint        arg_id)
{
  BObjectClass *oclass;
  BParamSpec *pspec;
  gchar *type_name, *pname;
  GType type;
  
  g_return_if_fail (arg_name != NULL);
  g_return_if_fail (arg_type > B_TYPE_NONE);
  g_return_if_fail (arg_id > 0);
  g_return_if_fail (arg_flags & G_PARAM_READWRITE);
  if (arg_flags & G_PARAM_CONSTRUCT)
    g_return_if_fail ((arg_flags & G_PARAM_CONSTRUCT_ONLY) == 0);
  if (arg_flags & (G_PARAM_CONSTRUCT | G_PARAM_CONSTRUCT_ONLY))
    g_return_if_fail (arg_flags & G_PARAM_WRITABLE);
  g_return_if_fail ((arg_flags & ~(G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_NAME)) == 0);

  pname = strchr (arg_name, ':');
  g_return_if_fail (pname && pname[1] == ':');

  type_name = g_strndup (arg_name, pname - arg_name);
  pname += 2;
  type = g_type_from_name (type_name);
  g_free (type_name);
  g_return_if_fail (B_TYPE_IS_OBJECT (type));

  oclass = btk_type_class (type);
  if (arg_flags & G_PARAM_READABLE)
    {
      if (oclass->get_property && oclass->get_property != btk_arg_proxy_get_property)
	{
	  g_warning (B_STRLOC ": BtkArg compatibility code can't be mixed with customized %s.get_property() implementation",
		     g_type_name (type));
	  return;
	}
      oclass->get_property = btk_arg_proxy_get_property;
    }
  if (arg_flags & G_PARAM_WRITABLE)
    {
      if (oclass->set_property && oclass->set_property != btk_arg_proxy_set_property)
	{
	  g_warning (B_STRLOC ": BtkArg compatibility code can't be mixed with customized %s.set_property() implementation",
		     g_type_name (type));
	  return;
	}
      oclass->set_property = btk_arg_proxy_set_property;
    }
  switch (B_TYPE_FUNDAMENTAL (arg_type))
    {
    case B_TYPE_ENUM:
      pspec = g_param_spec_enum (pname, NULL, NULL, arg_type, 0, arg_flags);
      break;
    case B_TYPE_FLAGS:
      pspec = g_param_spec_flags (pname, NULL, NULL, arg_type, 0, arg_flags);
      break;
    case B_TYPE_CHAR:
      pspec = g_param_spec_char (pname, NULL, NULL, -128, 127, 0, arg_flags);
      break;
    case B_TYPE_UCHAR:
      pspec = g_param_spec_uchar (pname, NULL, NULL, 0, 255, 0, arg_flags);
      break;
    case B_TYPE_BOOLEAN:
      pspec = g_param_spec_boolean (pname, NULL, NULL, FALSE, arg_flags);
      break;
    case B_TYPE_INT:
      pspec = g_param_spec_int (pname, NULL, NULL, G_MININT, G_MAXINT, 0, arg_flags);
      break;
    case B_TYPE_UINT:
      pspec = g_param_spec_uint (pname, NULL, NULL, 0, G_MAXUINT, 0, arg_flags);
      break;
    case B_TYPE_FLOAT:
      pspec = g_param_spec_float (pname, NULL, NULL, -G_MAXFLOAT, G_MAXFLOAT, 0, arg_flags);
      break;
    case B_TYPE_DOUBLE:
      pspec = g_param_spec_double (pname, NULL, NULL, -G_MAXDOUBLE, G_MAXDOUBLE, 0, arg_flags);
      break;
    case B_TYPE_STRING:
      pspec = g_param_spec_string (pname, NULL, NULL, NULL, arg_flags);
      break;
    case B_TYPE_POINTER:
      pspec = g_param_spec_pointer (pname, NULL, NULL, arg_flags);
      break;
    case B_TYPE_OBJECT:
      pspec = g_param_spec_object (pname, NULL, NULL, arg_type, arg_flags);
      break;
    case B_TYPE_BOXED:
      if (!B_TYPE_IS_FUNDAMENTAL (arg_type))
	{
	  pspec = g_param_spec_boxed (pname, NULL, NULL, arg_type, arg_flags);
	  break;
	}
    default:
      g_warning (B_STRLOC ": Property type `%s' is not supported by the BtkArg compatibility code",
		 g_type_name (arg_type));
      return;
    }
  g_object_class_install_property (oclass, arg_id, pspec);
}

static guint (*bobject_floating_flag_handler) (BtkObject*,gint) = NULL;

static guint
btk_object_floating_flag_handler (BtkObject *object,
                                  gint       job)
{
  /* FIXME: remove this whole thing once BTK+ breaks ABI */
  if (!BTK_IS_OBJECT (object))
    return bobject_floating_flag_handler (object, job);
  switch (job)
    {
      guint32 oldvalue;
    case +1:    /* force floating if possible */
      do
        oldvalue = g_atomic_int_get (&object->flags);
      while (!g_atomic_int_compare_and_exchange ((gint *)&object->flags, oldvalue, oldvalue | BTK_FLOATING));
      return oldvalue & BTK_FLOATING;
    case -1:    /* sink if possible */
      do
        oldvalue = g_atomic_int_get (&object->flags);
      while (!g_atomic_int_compare_and_exchange ((gint *)&object->flags, oldvalue, oldvalue & ~(guint32) BTK_FLOATING));
      return oldvalue & BTK_FLOATING;
    default:    /* check floating */
      return 0 != (g_atomic_int_get (&object->flags) & BTK_FLOATING);
    }
}

static void
btk_object_class_init (BtkObjectClass *class)
{
  BObjectClass *bobject_class = B_OBJECT_CLASS (class);
  gboolean is_bunnylib_2_10_1;

  parent_class = g_type_class_ref (B_TYPE_OBJECT);

  is_bunnylib_2_10_1 = g_object_compat_control (3, &bobject_floating_flag_handler);
  if (!is_bunnylib_2_10_1)
    g_error ("this version of Btk+ requires GLib-2.10.1");
  g_object_compat_control (2, btk_object_floating_flag_handler);

  bobject_class->set_property = btk_object_set_property;
  bobject_class->get_property = btk_object_get_property;
  bobject_class->dispose = btk_object_dispose;
  bobject_class->finalize = btk_object_finalize;

  class->destroy = btk_object_real_destroy;

  g_object_class_install_property (bobject_class,
				   PROP_USER_DATA,
				   g_param_spec_pointer ("user-data", 
							 P_("User Data"),
							 P_("Anonymous User Data Pointer"),
							 BTK_PARAM_READWRITE));
  object_signals[DESTROY] =
    g_signal_new (I_("destroy"),
		  B_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_CLEANUP | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
		  G_STRUCT_OFFSET (BtkObjectClass, destroy),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  B_TYPE_NONE, 0);
}

static void
btk_object_init (BtkObject      *object,
		 BtkObjectClass *klass)
{
  gboolean was_floating;
  /* sink the GInitiallyUnowned floating flag */
  was_floating = bobject_floating_flag_handler (object, -1);
  /* set BTK_FLOATING via btk_object_floating_flag_handler */
  if (was_floating)
    g_object_force_floating (B_OBJECT (object));
}

/********************************************
 * Functions to end a BtkObject's life time
 *
 ********************************************/
void
btk_object_destroy (BtkObject *object)
{
  g_return_if_fail (object != NULL);
  g_return_if_fail (BTK_IS_OBJECT (object));
  
  if (!(BTK_OBJECT_FLAGS (object) & BTK_IN_DESTRUCTION))
    g_object_run_dispose (B_OBJECT (object));
}

static void
btk_object_dispose (BObject *bobject)
{
  BtkObject *object = BTK_OBJECT (bobject);

  /* guard against reinvocations during
   * destruction with the BTK_IN_DESTRUCTION flag.
   */
  if (!(BTK_OBJECT_FLAGS (object) & BTK_IN_DESTRUCTION))
    {
      BTK_OBJECT_SET_FLAGS (object, BTK_IN_DESTRUCTION);
      
      g_signal_emit (object, object_signals[DESTROY], 0);
      
      BTK_OBJECT_UNSET_FLAGS (object, BTK_IN_DESTRUCTION);
    }

  B_OBJECT_CLASS (parent_class)->dispose (bobject);
}

static void
btk_object_real_destroy (BtkObject *object)
{
  g_signal_handlers_destroy (object);
}

static void
btk_object_finalize (BObject *bobject)
{
  BtkObject *object = BTK_OBJECT (bobject);

  if (g_object_is_floating (object))
    {
      g_warning ("A floating object was finalized. This means that someone\n"
		 "called g_object_unref() on an object that had only a floating\n"
		 "reference; the initial floating reference is not owned by anyone\n"
		 "and must be removed with g_object_ref_sink().");
    }
  
  btk_object_notify_weaks (object);
  
  B_OBJECT_CLASS (parent_class)->finalize (bobject);
}

/*****************************************
 * BtkObject argument handlers
 *
 *****************************************/

static void
btk_object_set_property (BObject      *object,
			 guint         property_id,
			 const BValue *value,
			 BParamSpec   *pspec)
{
  switch (property_id)
    {
    case PROP_USER_DATA:
      g_object_set_data (B_OBJECT (object), I_("user_data"), b_value_get_pointer (value));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
btk_object_get_property (BObject     *object,
			 guint        property_id,
			 BValue      *value,
			 BParamSpec  *pspec)
{
  switch (property_id)
    {
    case PROP_USER_DATA:
      b_value_set_pointer (value, g_object_get_data (B_OBJECT (object), "user_data"));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

void
btk_object_sink (BtkObject *object)
{
  g_return_if_fail (BTK_IS_OBJECT (object));
  g_object_ref_sink (object);
  g_object_unref (object);
}

/*****************************************
 * Weak references.
 *
 * Weak refs are very similar to the old "destroy" signal.  They allow
 * one to register a callback that is called when the weakly
 * referenced object is finalized.
 *  
 * They are not implemented as a signal because they really are
 * special and need to be used with great care.  Unlike signals, which
 * should be able to execute any code whatsoever.
 * 
 * A weakref callback is not allowed to retain a reference to the
 * object.  Object data keys may be retrieved in a weak reference
 * callback.
 * 
 * A weakref callback is called at most once.
 *
 *****************************************/

typedef struct _BtkWeakRef	BtkWeakRef;

struct _BtkWeakRef
{
  BtkWeakRef	 *next;
  GDestroyNotify  notify;
  gpointer        data;
};

void
btk_object_weakref (BtkObject      *object,
		    GDestroyNotify  notify,
		    gpointer        data)
{
  BtkWeakRef *weak;

  g_return_if_fail (notify != NULL);
  g_return_if_fail (BTK_IS_OBJECT (object));

  if (!quark_weakrefs)
    quark_weakrefs = g_quark_from_static_string ("btk-weakrefs");

  weak = g_new (BtkWeakRef, 1);
  weak->next = g_object_get_qdata (B_OBJECT (object), quark_weakrefs);
  weak->notify = notify;
  weak->data = data;
  g_object_set_qdata (B_OBJECT (object), quark_weakrefs, weak);
}

void
btk_object_weakunref (BtkObject      *object,
		      GDestroyNotify  notify,
		      gpointer        data)
{
  BtkWeakRef *weaks, *w, **wp;

  g_return_if_fail (BTK_IS_OBJECT (object));

  if (!quark_weakrefs)
    return;

  weaks = g_object_get_qdata (B_OBJECT (object), quark_weakrefs);
  for (wp = &weaks; *wp; wp = &(*wp)->next)
    {
      w = *wp;
      if (w->notify == notify && w->data == data)
	{
	  if (w == weaks)
	    g_object_set_qdata (B_OBJECT (object), quark_weakrefs, w->next);
	  else
	    *wp = w->next;
	  g_free (w);
	  return;
	}
    }
}

static void
btk_object_notify_weaks (BtkObject *object)
{
  if (quark_weakrefs)
    {
      BtkWeakRef *w1, *w2;
      
      w1 = g_object_get_qdata (B_OBJECT (object), quark_weakrefs);
      
      while (w1)
	{
	  w1->notify (w1->data);
	  w2 = w1->next;
	  g_free (w1);
	  w1 = w2;
	}
    }
}

BtkObject*
btk_object_new (GType        object_type,
		const gchar *first_property_name,
		...)
{
  BtkObject *object;
  va_list var_args;

  g_return_val_if_fail (B_TYPE_IS_OBJECT (object_type), NULL);

  va_start (var_args, first_property_name);
  object = (BtkObject *)g_object_new_valist (object_type, first_property_name, var_args);
  va_end (var_args);

  return object;
}

void
btk_object_get (BtkObject   *object,
		const gchar *first_property_name,
		...)
{
  va_list var_args;
  
  g_return_if_fail (BTK_IS_OBJECT (object));
  
  va_start (var_args, first_property_name);
  g_object_get_valist (B_OBJECT (object), first_property_name, var_args);
  va_end (var_args);
}

void
btk_object_set (BtkObject   *object,
		const gchar *first_property_name,
		...)
{
  va_list var_args;
  
  g_return_if_fail (BTK_IS_OBJECT (object));
  
  va_start (var_args, first_property_name);
  g_object_set_valist (B_OBJECT (object), first_property_name, var_args);
  va_end (var_args);
}

/*****************************************
 * BtkObject object_data mechanism
 *
 *****************************************/

void
btk_object_set_data_by_id (BtkObject        *object,
			   GQuark	     data_id,
			   gpointer          data)
{
  g_return_if_fail (BTK_IS_OBJECT (object));
  
  g_datalist_id_set_data (&B_OBJECT (object)->qdata, data_id, data);
}

void
btk_object_set_data (BtkObject        *object,
		     const gchar      *key,
		     gpointer          data)
{
  g_return_if_fail (BTK_IS_OBJECT (object));
  g_return_if_fail (key != NULL);
  
  g_datalist_set_data (&B_OBJECT (object)->qdata, key, data);
}

void
btk_object_set_data_by_id_full (BtkObject      *object,
				GQuark		data_id,
				gpointer        data,
				GDestroyNotify  destroy)
{
  g_return_if_fail (BTK_IS_OBJECT (object));

  g_datalist_id_set_data_full (&B_OBJECT (object)->qdata, data_id, data, destroy);
}

void
btk_object_set_data_full (BtkObject      *object,
			  const gchar    *key,
			  gpointer        data,
			  GDestroyNotify  destroy)
{
  g_return_if_fail (BTK_IS_OBJECT (object));
  g_return_if_fail (key != NULL);

  g_datalist_set_data_full (&B_OBJECT (object)->qdata, key, data, destroy);
}

gpointer
btk_object_get_data_by_id (BtkObject   *object,
			   GQuark       data_id)
{
  g_return_val_if_fail (BTK_IS_OBJECT (object), NULL);

  return g_datalist_id_get_data (&B_OBJECT (object)->qdata, data_id);
}

gpointer
btk_object_get_data (BtkObject   *object,
		     const gchar *key)
{
  g_return_val_if_fail (BTK_IS_OBJECT (object), NULL);
  g_return_val_if_fail (key != NULL, NULL);

  return g_datalist_get_data (&B_OBJECT (object)->qdata, key);
}

void
btk_object_remove_data_by_id (BtkObject   *object,
			      GQuark       data_id)
{
  g_return_if_fail (BTK_IS_OBJECT (object));

  g_datalist_id_remove_data (&B_OBJECT (object)->qdata, data_id);
}

void
btk_object_remove_data (BtkObject   *object,
			const gchar *key)
{
  g_return_if_fail (BTK_IS_OBJECT (object));
  g_return_if_fail (key != NULL);

  g_datalist_remove_data (&B_OBJECT (object)->qdata, key);
}

void
btk_object_remove_no_notify_by_id (BtkObject      *object,
				   GQuark          key_id)
{
  g_return_if_fail (BTK_IS_OBJECT (object));

  g_datalist_id_remove_no_notify (&B_OBJECT (object)->qdata, key_id);
}

void
btk_object_remove_no_notify (BtkObject       *object,
			     const gchar     *key)
{
  g_return_if_fail (BTK_IS_OBJECT (object));
  g_return_if_fail (key != NULL);

  g_datalist_remove_no_notify (&B_OBJECT (object)->qdata, key);
}

void
btk_object_set_user_data (BtkObject *object,
			  gpointer   data)
{
  g_return_if_fail (BTK_IS_OBJECT (object));

  g_object_set_data (B_OBJECT (object), "user_data", data);
}

gpointer
btk_object_get_user_data (BtkObject *object)
{
  g_return_val_if_fail (BTK_IS_OBJECT (object), NULL);

  return g_object_get_data (B_OBJECT (object), "user_data");
}

BtkObject*
btk_object_ref (BtkObject *object)
{
  g_return_val_if_fail (BTK_IS_OBJECT (object), NULL);

  return (BtkObject*) g_object_ref ((BObject*) object);
}

void
btk_object_unref (BtkObject *object)
{
  g_return_if_fail (BTK_IS_OBJECT (object));

  g_object_unref ((BObject*) object);
}

#define __BTK_OBJECT_C__
#include "btkaliasdef.c"
