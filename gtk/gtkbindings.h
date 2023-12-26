/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * BtkBindingSet: Keybinding manager for BtkObjects.
 * Copyright (C) 1998 Tim Janik
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

#ifndef __BTK_BINDINGS_H__
#define __BTK_BINDINGS_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <bdk/bdk.h>
#include <btk/btkobject.h>


G_BEGIN_DECLS


/* Binding sets
 */

typedef struct _BtkBindingSet		BtkBindingSet;
typedef struct _BtkBindingEntry		BtkBindingEntry;
typedef struct _BtkBindingSignal	BtkBindingSignal;
typedef struct _BtkBindingArg		BtkBindingArg;

struct _BtkBindingSet
{
  gchar			*set_name;
  gint			 priority;
  GSList		*widget_path_pspecs;
  GSList		*widget_class_pspecs;
  GSList		*class_branch_pspecs;
  BtkBindingEntry	*entries;
  BtkBindingEntry	*current;
  guint                  parsed : 1; /* From RC content */
};

struct _BtkBindingEntry
{
  /* key portion
   */
  guint			 keyval;
  BdkModifierType	 modifiers;
  
  BtkBindingSet		*binding_set;
  guint			destroyed : 1;
  guint			in_emission : 1;
  guint                 marks_unbound : 1;
  BtkBindingEntry	*set_next;
  BtkBindingEntry	*hash_next;
  BtkBindingSignal	*signals;
};

struct _BtkBindingArg
{
  GType		 arg_type;
  union {
    glong	 long_data;
    gdouble	 double_data;
    gchar	*string_data;
  } d;
};

struct _BtkBindingSignal
{
  BtkBindingSignal	*next;
  gchar 		*signal_name;
  guint			 n_args;
  BtkBindingArg		*args;
};

/* Application-level methods */

BtkBindingSet*	btk_binding_set_new	(const gchar	*set_name);
BtkBindingSet*	btk_binding_set_by_class(gpointer	 object_class);
BtkBindingSet*	btk_binding_set_find	(const gchar	*set_name);
gboolean btk_bindings_activate		(BtkObject	*object,
					 guint		 keyval,
					 BdkModifierType modifiers);
gboolean btk_bindings_activate_event    (BtkObject      *object,
					 BdkEventKey    *event);
gboolean btk_binding_set_activate	(BtkBindingSet	*binding_set,
					 guint		 keyval,
					 BdkModifierType modifiers,
					 BtkObject	*object);

#ifndef BTK_DISABLE_DEPRECATED
#define	 btk_binding_entry_add		btk_binding_entry_clear
void	 btk_binding_entry_clear	(BtkBindingSet	*binding_set,
					 guint		 keyval,
					 BdkModifierType modifiers);
guint	 btk_binding_parse_binding      (GScanner       *scanner);
#endif /* BTK_DISABLE_DEPRECATED */

void	 btk_binding_entry_skip         (BtkBindingSet  *binding_set,
                                         guint           keyval,
                                         BdkModifierType modifiers);
void	 btk_binding_entry_add_signal   (BtkBindingSet  *binding_set,
                                         guint           keyval,
                                         BdkModifierType modifiers,
                                         const gchar    *signal_name,
                                         guint           n_args,
                                         ...);
void	 btk_binding_entry_add_signall	(BtkBindingSet	*binding_set,
					 guint		 keyval,
					 BdkModifierType modifiers,
					 const gchar	*signal_name,
					 GSList		*binding_args);
void	 btk_binding_entry_remove	(BtkBindingSet	*binding_set,
					 guint		 keyval,
					 BdkModifierType modifiers);

void	 btk_binding_set_add_path	(BtkBindingSet	*binding_set,
					 BtkPathType	 path_type,
					 const gchar	*path_pattern,
					 BtkPathPriorityType priority);


/* Non-public methods */

guint	 _btk_binding_parse_binding     (GScanner       *scanner);
void     _btk_binding_reset_parsed      (void);
void	 _btk_binding_entry_add_signall (BtkBindingSet  *binding_set,
					 guint		 keyval,
					 BdkModifierType modifiers,
					 const gchar	*signal_name,
					 GSList		*binding_args);

G_END_DECLS

#endif /* __BTK_BINDINGS_H__ */
