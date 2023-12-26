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

#ifndef __BTK_CONTAINER_H__
#define __BTK_CONTAINER_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkwidget.h>
#include <btk/btkadjustment.h>


B_BEGIN_DECLS

#define BTK_TYPE_CONTAINER              (btk_container_get_type ())
#define BTK_CONTAINER(obj)              (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_CONTAINER, BtkContainer))
#define BTK_CONTAINER_CLASS(klass)      (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_CONTAINER, BtkContainerClass))
#define BTK_IS_CONTAINER(obj)           (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_CONTAINER))
#define BTK_IS_CONTAINER_CLASS(klass)   (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_CONTAINER))
#define BTK_CONTAINER_GET_CLASS(obj)    (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_CONTAINER, BtkContainerClass))

#define BTK_IS_RESIZE_CONTAINER(widget) (BTK_IS_CONTAINER (widget) && ((BtkContainer*) (widget))->resize_mode != BTK_RESIZE_PARENT)


typedef struct _BtkContainer	   BtkContainer;
typedef struct _BtkContainerClass  BtkContainerClass;

struct _BtkContainer
{
  BtkWidget widget;

  BtkWidget *GSEAL (focus_child);

  buint GSEAL (border_width) : 16;

  /*< private >*/
  buint GSEAL (need_resize) : 1;
  buint GSEAL (resize_mode) : 2;
  buint GSEAL (reallocate_redraws) : 1;
  buint GSEAL (has_focus_chain) : 1;
};

struct _BtkContainerClass
{
  BtkWidgetClass parent_class;

  void    (*add)       		(BtkContainer	 *container,
				 BtkWidget	 *widget);
  void    (*remove)    		(BtkContainer	 *container,
				 BtkWidget	 *widget);
  void    (*check_resize)	(BtkContainer	 *container);
  void    (*forall)    		(BtkContainer	 *container,
				 bboolean	  include_internals,
				 BtkCallback	  callback,
				 bpointer	  callback_data);
  void    (*set_focus_child)	(BtkContainer	 *container,
				 BtkWidget	 *widget);
  GType   (*child_type)		(BtkContainer	 *container);
  bchar*  (*composite_name)	(BtkContainer	 *container,
				 BtkWidget	 *child);
  void    (*set_child_property) (BtkContainer    *container,
				 BtkWidget       *child,
				 buint            property_id,
				 const BValue    *value,
				 BParamSpec      *pspec);
  void    (*get_child_property) (BtkContainer    *container,
                                 BtkWidget       *child,
				 buint            property_id,
				 BValue          *value,
				 BParamSpec      *pspec);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};

/* Application-level methods */

GType   btk_container_get_type		 (void) B_GNUC_CONST;
void    btk_container_set_border_width	 (BtkContainer	   *container,
					  buint		    border_width);
buint   btk_container_get_border_width   (BtkContainer     *container);
void    btk_container_add		 (BtkContainer	   *container,
					  BtkWidget	   *widget);
void    btk_container_remove		 (BtkContainer	   *container,
					  BtkWidget	   *widget);

void    btk_container_set_resize_mode    (BtkContainer     *container,
					  BtkResizeMode     resize_mode);
BtkResizeMode btk_container_get_resize_mode (BtkContainer     *container);

void    btk_container_check_resize       (BtkContainer     *container);

void     btk_container_foreach      (BtkContainer       *container,
				     BtkCallback         callback,
				     bpointer            callback_data);
#ifndef BTK_DISABLE_DEPRECATED
void     btk_container_foreach_full (BtkContainer       *container,
				     BtkCallback         callback,
				     BtkCallbackMarshal  marshal,
				     bpointer            callback_data,
				     GDestroyNotify      notify);
#endif

GList*   btk_container_get_children     (BtkContainer       *container);

#ifndef BTK_DISABLE_DEPRECATED
#define btk_container_children btk_container_get_children
#endif

void     btk_container_propagate_expose (BtkContainer   *container,
					 BtkWidget      *child,
					 BdkEventExpose *event);

void     btk_container_set_focus_chain  (BtkContainer   *container,
                                         GList          *focusable_widgets);
bboolean btk_container_get_focus_chain  (BtkContainer   *container,
					 GList         **focusable_widgets);
void     btk_container_unset_focus_chain (BtkContainer  *container);

/* Widget-level methods */

void   btk_container_set_reallocate_redraws (BtkContainer    *container,
					     bboolean         needs_redraws);
void   btk_container_set_focus_child	   (BtkContainer     *container,
					    BtkWidget	     *child);
BtkWidget *
       btk_container_get_focus_child	   (BtkContainer     *container);
void   btk_container_set_focus_vadjustment (BtkContainer     *container,
					    BtkAdjustment    *adjustment);
BtkAdjustment *btk_container_get_focus_vadjustment (BtkContainer *container);
void   btk_container_set_focus_hadjustment (BtkContainer     *container,
					    BtkAdjustment    *adjustment);
BtkAdjustment *btk_container_get_focus_hadjustment (BtkContainer *container);

void    btk_container_resize_children      (BtkContainer     *container);

GType   btk_container_child_type	   (BtkContainer     *container);


void         btk_container_class_install_child_property (BtkContainerClass *cclass,
							 buint		    property_id,
							 BParamSpec	   *pspec);
BParamSpec*  btk_container_class_find_child_property	(BObjectClass	   *cclass,
							 const bchar	   *property_name);
BParamSpec** btk_container_class_list_child_properties	(BObjectClass	   *cclass,
							 buint		   *n_properties);
void         btk_container_add_with_properties		(BtkContainer	   *container,
							 BtkWidget	   *widget,
							 const bchar	   *first_prop_name,
							 ...) B_GNUC_NULL_TERMINATED;
void         btk_container_child_set			(BtkContainer	   *container,
							 BtkWidget	   *child,
							 const bchar	   *first_prop_name,
							 ...) B_GNUC_NULL_TERMINATED;
void         btk_container_child_get			(BtkContainer	   *container,
							 BtkWidget	   *child,
							 const bchar	   *first_prop_name,
							 ...) B_GNUC_NULL_TERMINATED;
void         btk_container_child_set_valist		(BtkContainer	   *container,
							 BtkWidget	   *child,
							 const bchar	   *first_property_name,
							 va_list	    var_args);
void         btk_container_child_get_valist		(BtkContainer	   *container,
							 BtkWidget	   *child,
							 const bchar	   *first_property_name,
							 va_list	    var_args);
void	     btk_container_child_set_property		(BtkContainer	   *container,
							 BtkWidget	   *child,
							 const bchar	   *property_name,
							 const BValue	   *value);
void	     btk_container_child_get_property		(BtkContainer	   *container,
							 BtkWidget	   *child,
							 const bchar	   *property_name,
							 BValue		   *value);

#define BTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID(object, property_id, pspec) \
    B_OBJECT_WARN_INVALID_PSPEC ((object), "child property id", (property_id), (pspec))


void    btk_container_forall		     (BtkContainer *container,
					      BtkCallback   callback,
					      bpointer	    callback_data);

/* Non-public methods */
void	_btk_container_queue_resize	     (BtkContainer *container);
void    _btk_container_clear_resize_widgets   (BtkContainer *container);
bchar*	_btk_container_child_composite_name   (BtkContainer *container,
					      BtkWidget	   *child);
void   _btk_container_dequeue_resize_handler (BtkContainer *container);
GList *_btk_container_focus_sort             (BtkContainer     *container,
					      GList            *children,
					      BtkDirectionType  direction,
					      BtkWidget        *old_focus);

#ifndef BTK_DISABLE_DEPRECATED
#define	btk_container_border_width		btk_container_set_border_width
#endif /* BTK_DISABLE_DEPRECATED */

B_END_DECLS

#endif /* __BTK_CONTAINER_H__ */
