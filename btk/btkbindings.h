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


B_BEGIN_DECLS


/* Binding sets
 */

typedef struct _BtkBindingSet		BtkBindingSet;
typedef struct _BtkBindingEntry		BtkBindingEntry;
typedef struct _BtkBindingSignal	BtkBindingSignal;
typedef struct _BtkBindingArg		BtkBindingArg;

struct _BtkBindingSet
{
  bchar			*set_name;
  bint			 priority;
  GSList		*widget_path_pspecs;
  GSList		*widget_class_pspecs;
  GSList		*class_branch_pspecs;
  BtkBindingEntry	*entries;
  BtkBindingEntry	*current;
  buint                  parsed : 1; /* From RC content */
};

struct _BtkBindingEntry
{
  /* key portion
   */
  buint			 keyval;
  BdkModifierType	 modifiers;
  
  BtkBindingSet		*binding_set;
  buint			destroyed : 1;
  buint			in_emission : 1;
  buint                 marks_unbound : 1;
  BtkBindingEntry	*set_next;
  BtkBindingEntry	*hash_next;
  BtkBindingSignal	*signals;
};

struct _BtkBindingArg
{
  GType		 arg_type;
  union {
    blong	 long_data;
    bdouble	 double_data;
    bchar	*string_data;
  } d;
};

struct _BtkBindingSignal
{
  BtkBindingSignal	*next;
  bchar 		*signal_name;
  buint			 n_args;
  BtkBindingArg		*args;
};

/* Application-level methods */

BtkBindingSet*	btk_binding_set_new	(const bchar	*set_name);
BtkBindingSet*	btk_binding_set_by_class(bpointer	 object_class);
BtkBindingSet*	btk_binding_set_find	(const bchar	*set_name);
bboolean btk_bindings_activate		(BtkObject	*object,
					 buint		 keyval,
					 BdkModifierType modifiers);
bboolean btk_bindings_activate_event    (BtkObject      *object,
					 BdkEventKey    *event);
bboolean btk_binding_set_activate	(BtkBindingSet	*binding_set,
					 buint		 keyval,
					 BdkModifierType modifiers,
					 BtkObject	*object);

#ifndef BTK_DISABLE_DEPRECATED
#define	 btk_binding_entry_add		btk_binding_entry_clear
void	 btk_binding_entry_clear	(BtkBindingSet	*binding_set,
					 buint		 keyval,
					 BdkModifierType modifiers);
buint	 btk_binding_parse_binding      (GScanner       *scanner);
#endif /* BTK_DISABLE_DEPRECATED */

void	 btk_binding_entry_skip         (BtkBindingSet  *binding_set,
                                         buint           keyval,
                                         BdkModifierType modifiers);
void	 btk_binding_entry_add_signal   (BtkBindingSet  *binding_set,
                                         buint           keyval,
                                         BdkModifierType modifiers,
                                         const bchar    *signal_name,
                                         buint           n_args,
                                         ...);
void	 btk_binding_entry_add_signall	(BtkBindingSet	*binding_set,
					 buint		 keyval,
					 BdkModifierType modifiers,
					 const bchar	*signal_name,
					 GSList		*binding_args);
void	 btk_binding_entry_remove	(BtkBindingSet	*binding_set,
					 buint		 keyval,
					 BdkModifierType modifiers);

void	 btk_binding_set_add_path	(BtkBindingSet	*binding_set,
					 BtkPathType	 path_type,
					 const bchar	*path_pattern,
					 BtkPathPriorityType priority);


/* Non-public methods */

buint	 _btk_binding_parse_binding     (GScanner       *scanner);
void     _btk_binding_reset_parsed      (void);
void	 _btk_binding_entry_add_signall (BtkBindingSet  *binding_set,
					 buint		 keyval,
					 BdkModifierType modifiers,
					 const bchar	*signal_name,
					 GSList		*binding_args);

B_END_DECLS

#endif /* __BTK_BINDINGS_H__ */
