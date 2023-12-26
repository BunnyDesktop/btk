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

#ifndef __BTK_OBJECT_H__
#define __BTK_OBJECT_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <bdkconfig.h>
#include <btk/btkenums.h>
#include <btk/btktypeutils.h>
#include <btk/btkdebug.h>


B_BEGIN_DECLS

/* macros for casting a pointer to a BtkObject or BtkObjectClass pointer,
 * and to test whether `object' and `klass' are of type BTK_TYPE_OBJECT.
 * these are the standard macros for all BtkObject-derived classes.
 */
#define BTK_TYPE_OBJECT              (btk_object_get_type ())
#define BTK_OBJECT(object)           (B_TYPE_CHECK_INSTANCE_CAST ((object), BTK_TYPE_OBJECT, BtkObject))
#define BTK_OBJECT_CLASS(klass)      (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_OBJECT, BtkObjectClass))
#define BTK_IS_OBJECT(object)        (B_TYPE_CHECK_INSTANCE_TYPE ((object), BTK_TYPE_OBJECT))
#define BTK_IS_OBJECT_CLASS(klass)   (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_OBJECT))
#define BTK_OBJECT_GET_CLASS(object) (B_TYPE_INSTANCE_GET_CLASS ((object), BTK_TYPE_OBJECT, BtkObjectClass))

/* Macros for extracting various fields from BtkObject and BtkObjectClass.
 */
#ifndef BTK_DISABLE_DEPRECATED
/**
 * BTK_OBJECT_TYPE:
 * @object: a #BtkObject.
 *
 * Gets the type of an object.
 *
 * Deprecated: 2.20: Use B_OBJECT_TYPE() instead.
 */
#define BTK_OBJECT_TYPE                   B_OBJECT_TYPE
/**
 * BTK_OBJECT_TYPE_NAME:
 * @object: a #BtkObject.
 *
 * Gets the name of an object's type.
 *
 * Deprecated: 2.20: Use B_OBJECT_TYPE_NAME() instead.
 */
#define BTK_OBJECT_TYPE_NAME              B_OBJECT_TYPE_NAME
#endif

#if !defined (BTK_DISABLE_DEPRECATED) || defined (BTK_COMPILATION)
/* BtkObject only uses the first 4 bits of the flags field.
 * Derived objects may use the remaining bits. Though this
 * is a kinda nasty break up, it does make the size of
 * derived objects smaller.
 */
typedef enum
{
  BTK_IN_DESTRUCTION	= 1 << 0, /* Used internally during dispose */
  BTK_FLOATING		= 1 << 1,
  BTK_RESERVED_1	= 1 << 2,
  BTK_RESERVED_2	= 1 << 3
} BtkObjectFlags;

/* Macros for extracting the object_flags from BtkObject.
 */
#define BTK_OBJECT_FLAGS(obj)		  (BTK_OBJECT (obj)->flags)
#ifndef BTK_DISABLE_DEPRECATED
#define BTK_OBJECT_FLOATING(obj)	  (g_object_is_floating (obj))
#endif

/* Macros for setting and clearing bits in the object_flags field of BtkObject.
 */
#define BTK_OBJECT_SET_FLAGS(obj,flag)	  B_STMT_START{ (BTK_OBJECT_FLAGS (obj) |= (flag)); }B_STMT_END
#define BTK_OBJECT_UNSET_FLAGS(obj,flag)  B_STMT_START{ (BTK_OBJECT_FLAGS (obj) &= ~(flag)); }B_STMT_END
#endif

typedef struct _BtkObjectClass	BtkObjectClass;


struct _BtkObject
{
  GInitiallyUnowned parent_instance;

  /* 32 bits of flags. BtkObject only uses 4 of these bits and
   *  BtkWidget uses the rest. This is done because structs are
   *  aligned on 4 or 8 byte boundaries. If a new bitfield were
   *  used in BtkWidget much space would be wasted.
   */
  guint32 GSEAL (flags);
};

struct _BtkObjectClass
{
  GInitiallyUnownedClass parent_class;

  /* Non overridable class methods to set and get per class arguments */
  void (*set_arg) (BtkObject *object,
		   BtkArg    *arg,
		   guint      arg_id);
  void (*get_arg) (BtkObject *object,
		   BtkArg    *arg,
		   guint      arg_id);

  /* Default signal handler for the ::destroy signal, which is
   *  invoked to request that references to the widget be dropped.
   *  If an object class overrides destroy() in order to perform class
   *  specific destruction then it must still invoke its superclass'
   *  implementation of the method after it is finished with its
   *  own cleanup. (See btk_widget_real_destroy() for an example of
   *  how to do this).
   */
  void (*destroy)  (BtkObject *object);
};



/* Application-level methods */

GType btk_object_get_type (void) B_GNUC_CONST;

#ifndef BTK_DISABLE_DEPRECATED
void btk_object_sink	  (BtkObject *object);
#endif
void btk_object_destroy	  (BtkObject *object);

/****************************************************************/

#ifndef BTK_DISABLE_DEPRECATED

BtkObject*	btk_object_new		  (GType	       type,
					   const gchar	      *first_property_name,
					   ...);
BtkObject*	btk_object_ref		  (BtkObject	      *object);
void		btk_object_unref	  (BtkObject	      *object);
void btk_object_weakref	  (BtkObject	    *object,
			   GDestroyNotify    notify,
			   gpointer	     data);
void btk_object_weakunref (BtkObject	    *object,
			   GDestroyNotify    notify,
			   gpointer	     data);

/* Set 'data' to the "object_data" field of the object. The
 *  data is indexed by the "key". If there is already data
 *  associated with "key" then the new data will replace it.
 *  If 'data' is NULL then this call is equivalent to
 *  'btk_object_remove_data'.
 *  The btk_object_set_data_full variant acts just the same,
 *  but takes an additional argument which is a function to
 *  be called when the data is removed.
 *  `btk_object_remove_data' is equivalent to the above,
 *  where 'data' is NULL
 *  `btk_object_get_data' gets the data associated with "key".
 */
void	 btk_object_set_data	     (BtkObject	     *object,
				      const gchar    *key,
				      gpointer	      data);
void	 btk_object_set_data_full    (BtkObject	     *object,
				      const gchar    *key,
				      gpointer	      data,
				      GDestroyNotify  destroy);
void	 btk_object_remove_data	     (BtkObject	     *object,
				      const gchar    *key);
gpointer btk_object_get_data	     (BtkObject	     *object,
				      const gchar    *key);
void	 btk_object_remove_no_notify (BtkObject	     *object,
				      const gchar    *key);

/* Set/get the "user_data" object data field of "object". It should
 *  be noted that these functions are no different than calling
 *  `btk_object_set_data'/`btk_object_get_data' with a key of "user_data".
 *  They are merely provided as a convenience.
 */
void	 btk_object_set_user_data (BtkObject	*object,
				   gpointer	 data);
gpointer btk_object_get_user_data (BtkObject	*object);


/* Object-level methods */

/* Object data method variants that operate on key ids. */
void btk_object_set_data_by_id		(BtkObject	 *object,
					 GQuark		  data_id,
					 gpointer	  data);
void btk_object_set_data_by_id_full	(BtkObject	 *object,
					 GQuark		  data_id,
					 gpointer	  data,
					 GDestroyNotify   destroy);
gpointer btk_object_get_data_by_id	(BtkObject	 *object,
					 GQuark		  data_id);
void  btk_object_remove_data_by_id	(BtkObject	 *object,
					 GQuark		  data_id);
void  btk_object_remove_no_notify_by_id	(BtkObject	 *object,
					 GQuark		  key_id);
#define	btk_object_data_try_key	    g_quark_try_string
#define	btk_object_data_force_id    g_quark_from_string

/* BtkArg flag bits for btk_object_add_arg_type
 */
typedef enum
{
  BTK_ARG_READABLE	 = G_PARAM_READABLE,
  BTK_ARG_WRITABLE	 = G_PARAM_WRITABLE,
  BTK_ARG_CONSTRUCT	 = G_PARAM_CONSTRUCT,
  BTK_ARG_CONSTRUCT_ONLY = G_PARAM_CONSTRUCT_ONLY,
  BTK_ARG_CHILD_ARG	 = 1 << 4
} BtkArgFlags;
#define	BTK_ARG_READWRITE	(BTK_ARG_READABLE | BTK_ARG_WRITABLE)
void	btk_object_get		(BtkObject	*object,
				 const gchar	*first_property_name,
				 ...) B_GNUC_NULL_TERMINATED;
void	btk_object_set		(BtkObject	*object,
				 const gchar	*first_property_name,
				 ...) B_GNUC_NULL_TERMINATED;
void	btk_object_add_arg_type		(const gchar    *arg_name,
					 GType           arg_type,
					 guint           arg_flags,
					 guint           arg_id);

#endif /* BTK_DISABLE_DEPRECATED */

B_END_DECLS

#endif /* __BTK_OBJECT_H__ */
