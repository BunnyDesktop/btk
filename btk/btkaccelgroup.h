/* BTK - The GIMP Toolkit
 * Copyright (C) 1998, 2001 Tim Janik
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

#ifndef __BTK_ACCEL_GROUP_H__
#define __BTK_ACCEL_GROUP_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <bdk/bdk.h>
#include <btk/btkenums.h>

G_BEGIN_DECLS


/* --- type macros --- */
#define BTK_TYPE_ACCEL_GROUP              (btk_accel_group_get_type ())
#define BTK_ACCEL_GROUP(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BTK_TYPE_ACCEL_GROUP, BtkAccelGroup))
#define BTK_ACCEL_GROUP_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_ACCEL_GROUP, BtkAccelGroupClass))
#define BTK_IS_ACCEL_GROUP(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BTK_TYPE_ACCEL_GROUP))
#define BTK_IS_ACCEL_GROUP_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_ACCEL_GROUP))
#define BTK_ACCEL_GROUP_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_ACCEL_GROUP, BtkAccelGroupClass))


/* --- accel flags --- */
typedef enum
{
  BTK_ACCEL_VISIBLE        = 1 << 0,	/* display in BtkAccelLabel? */
  BTK_ACCEL_LOCKED         = 1 << 1,	/* is it removable? */
  BTK_ACCEL_MASK           = 0x07
} BtkAccelFlags;


/* --- typedefs & structures --- */
typedef struct _BtkAccelGroup	   BtkAccelGroup;
typedef struct _BtkAccelGroupClass BtkAccelGroupClass;
typedef struct _BtkAccelKey        BtkAccelKey;
typedef struct _BtkAccelGroupEntry BtkAccelGroupEntry;
typedef gboolean (*BtkAccelGroupActivate) (BtkAccelGroup  *accel_group,
					   GObject        *acceleratable,
					   guint           keyval,
					   BdkModifierType modifier);

/**
 * BtkAccelGroupFindFunc:
 * @key: 
 * @closure: 
 * @data: 
 * 
 * Since: 2.2
 */
typedef gboolean (*BtkAccelGroupFindFunc) (BtkAccelKey    *key,
					   GClosure       *closure,
					   gpointer        data);

/**
 * BtkAccelGroup:
 * 
 * An object representing and maintaining a group of accelerators.
 */
struct _BtkAccelGroup
{
  GObject             parent;

  guint               GSEAL (lock_count);
  BdkModifierType     GSEAL (modifier_mask);
  GSList             *GSEAL (acceleratables);
  guint	              GSEAL (n_accels);
  BtkAccelGroupEntry *GSEAL (priv_accels);
};

struct _BtkAccelGroupClass
{
  GObjectClass parent_class;

  void	(*accel_changed)	(BtkAccelGroup	*accel_group,
				 guint           keyval,
				 BdkModifierType modifier,
				 GClosure       *accel_closure);
  
  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};

struct _BtkAccelKey
{
  guint           accel_key;
  BdkModifierType accel_mods;
  guint           accel_flags : 16;
};


/* -- Accelerator Groups --- */
GType          btk_accel_group_get_type           (void) G_GNUC_CONST;
BtkAccelGroup* btk_accel_group_new	      	  (void);
gboolean       btk_accel_group_get_is_locked      (BtkAccelGroup  *accel_group);
BdkModifierType 
               btk_accel_group_get_modifier_mask  (BtkAccelGroup  *accel_group);
void	       btk_accel_group_lock		  (BtkAccelGroup  *accel_group);
void	       btk_accel_group_unlock		  (BtkAccelGroup  *accel_group);
void	       btk_accel_group_connect		  (BtkAccelGroup  *accel_group,
						   guint	   accel_key,
						   BdkModifierType accel_mods,
						   BtkAccelFlags   accel_flags,
						   GClosure	  *closure);
void           btk_accel_group_connect_by_path    (BtkAccelGroup  *accel_group,
						   const gchar	  *accel_path,
						   GClosure	  *closure);
gboolean       btk_accel_group_disconnect	  (BtkAccelGroup  *accel_group,
						   GClosure	  *closure);
gboolean       btk_accel_group_disconnect_key	  (BtkAccelGroup  *accel_group,
						   guint	   accel_key,
						   BdkModifierType accel_mods);
gboolean       btk_accel_group_activate           (BtkAccelGroup   *accel_group,
                                                   GQuark	   accel_quark,
                                                   GObject	  *acceleratable,
                                                   guint	   accel_key,
                                                   BdkModifierType accel_mods);


/* --- BtkActivatable glue --- */
void		_btk_accel_group_attach		(BtkAccelGroup	*accel_group,
						 GObject	*object);
void		_btk_accel_group_detach		(BtkAccelGroup	*accel_group,
						 GObject	*object);
gboolean        btk_accel_groups_activate      	(GObject	*object,
						 guint		 accel_key,
						 BdkModifierType accel_mods);
GSList*	        btk_accel_groups_from_object    (GObject	*object);
BtkAccelKey*	btk_accel_group_find		(BtkAccelGroup	      *accel_group,
						 BtkAccelGroupFindFunc find_func,
						 gpointer              data);
BtkAccelGroup*	btk_accel_group_from_accel_closure (GClosure    *closure);


/* --- Accelerators--- */
gboolean btk_accelerator_valid		      (guint	        keyval,
					       BdkModifierType  modifiers) G_GNUC_CONST;
void	 btk_accelerator_parse		      (const gchar     *accelerator,
					       guint	       *accelerator_key,
					       BdkModifierType *accelerator_mods);
gchar*	 btk_accelerator_name		      (guint	        accelerator_key,
					       BdkModifierType  accelerator_mods);
gchar*   btk_accelerator_get_label            (guint           accelerator_key,
                                               BdkModifierType accelerator_mods);
void	 btk_accelerator_set_default_mod_mask (BdkModifierType  default_mod_mask);
guint	 btk_accelerator_get_default_mod_mask (void);


/* --- internal --- */
BtkAccelGroupEntry*	btk_accel_group_query	(BtkAccelGroup	*accel_group,
						 guint		 accel_key,
						 BdkModifierType accel_mods,
						 guint          *n_entries);

void		     _btk_accel_group_reconnect (BtkAccelGroup *accel_group,
						 GQuark         accel_path_quark);

struct _BtkAccelGroupEntry
{
  BtkAccelKey  key;
  GClosure    *closure;
  GQuark       accel_path_quark;
};


#ifndef BTK_DISABLE_DEPRECATED
/**
 * btk_accel_group_ref:
 * 
 * Deprecated equivalent of g_object_ref().
 * 
 * Returns: the accel group that was passed in
 */
#define	btk_accel_group_ref	g_object_ref

/**
 * btk_accel_group_unref:
 * 
 * Deprecated equivalent of g_object_unref().
 */
#define	btk_accel_group_unref	g_object_unref
#endif /* BTK_DISABLE_DEPRECATED */

G_END_DECLS


#endif /* __BTK_ACCEL_GROUP_H__ */
