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

#ifndef __BTK_BOX_H__
#define __BTK_BOX_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkcontainer.h>


B_BEGIN_DECLS


#define BTK_TYPE_BOX            (btk_box_get_type ())
#define BTK_BOX(obj)            (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_BOX, BtkBox))
#define BTK_BOX_CLASS(klass)    (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_BOX, BtkBoxClass))
#define BTK_IS_BOX(obj)         (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_BOX))
#define BTK_IS_BOX_CLASS(klass) (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_BOX))
#define BTK_BOX_GET_CLASS(obj)  (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_BOX, BtkBoxClass))


typedef struct _BtkBox	      BtkBox;
typedef struct _BtkBoxClass   BtkBoxClass;

struct _BtkBox
{
  BtkContainer container;

  /*< public >*/
  GList *GSEAL (children);
  bint16 GSEAL (spacing);
  buint GSEAL (homogeneous) : 1;
};

struct _BtkBoxClass
{
  BtkContainerClass parent_class;
};

/**
 * BtkBoxChild:
 * @widget: the child widget, packed into the BtkBox.
 * @padding: the number of extra pixels to put between this child and its
 *  neighbors, set when packed, zero by default.
 * @expand: flag indicates whether extra space should be given to this child.
 *  Any extra space given to the parent BtkBox is divided up among all children
 *  with this attribute set to %TRUE; set when packed, %TRUE by default.
 * @fill: flag indicates whether any extra space given to this child due to its
 *  @expand attribute being set is actually allocated to the child, rather than
 *  being used as padding around the widget; set when packed, %TRUE by default.
 * @pack: one of #BtkPackType indicating whether the child is packed with
 *  reference to the start (top/left) or end (bottom/right) of the BtkBox.
 * @is_secondary: %TRUE if the child is secondary
 *
 * The #BtkBoxChild holds a child widget of #BtkBox and describes how the child
 * is to be packed into the #BtkBox. All fields of this #BtkBoxChild should be
 * considered read-only and they should never be set directly by an application.
 * Use btk_box_query_child_packing() and btk_box_set_child_packing() to query
 * and set the #BtkBoxChild.padding, #BtkBoxChild.expand, #BtkBoxChild.fill and
 * #BtkBoxChild.pack fields.
 *
 * Deprecated: 2.22: Use btk_container_get_children() instead.
 */
#if !defined (BTK_DISABLE_DEPRECATED) || defined (BTK_COMPILATION)
typedef struct _BtkBoxChild   BtkBoxChild;
struct _BtkBoxChild
{
  BtkWidget *widget;
  buint16 padding;
  buint expand : 1;
  buint fill : 1;
  buint pack : 1;
  buint is_secondary : 1;
};
#endif

GType       btk_box_get_type            (void) B_GNUC_CONST;
BtkWidget* _btk_box_new                 (BtkOrientation  orientation,
                                         bboolean        homogeneous,
                                         bint            spacing);

void        btk_box_pack_start          (BtkBox         *box,
                                         BtkWidget      *child,
                                         bboolean        expand,
                                         bboolean        fill,
                                         buint           padding);
void        btk_box_pack_end            (BtkBox         *box,
                                         BtkWidget      *child,
                                         bboolean        expand,
                                         bboolean        fill,
                                         buint           padding);

#ifndef BTK_DISABLE_DEPRECATED
void        btk_box_pack_start_defaults (BtkBox         *box,
                                         BtkWidget      *widget);
void        btk_box_pack_end_defaults   (BtkBox         *box,
                                         BtkWidget      *widget);
#endif

void        btk_box_set_homogeneous     (BtkBox         *box,
                                         bboolean        homogeneous);
bboolean    btk_box_get_homogeneous     (BtkBox         *box);
void        btk_box_set_spacing         (BtkBox         *box,
                                         bint            spacing);
bint        btk_box_get_spacing         (BtkBox         *box);

void        btk_box_reorder_child       (BtkBox         *box,
                                         BtkWidget      *child,
                                         bint            position);

void        btk_box_query_child_packing (BtkBox         *box,
                                         BtkWidget      *child,
                                         bboolean       *expand,
                                         bboolean       *fill,
                                         buint          *padding,
                                         BtkPackType    *pack_type);
void        btk_box_set_child_packing   (BtkBox         *box,
                                         BtkWidget      *child,
                                         bboolean        expand,
                                         bboolean        fill,
                                         buint           padding,
                                         BtkPackType     pack_type);

/* internal API */
void        _btk_box_set_old_defaults   (BtkBox         *box);
bboolean    _btk_box_get_spacing_set    (BtkBox         *box);
void        _btk_box_set_spacing_set    (BtkBox         *box,
                                         bboolean        spacing_set);

B_END_DECLS

#endif /* __BTK_BOX_H__ */
