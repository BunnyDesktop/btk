/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 1998 Elliot Lee
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

/* The BtkHandleBox is to allow widgets to be dragged in and out of
 * their parents.
 */

#ifndef __BTK_HANDLE_BOX_H__
#define __BTK_HANDLE_BOX_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkbin.h>


B_BEGIN_DECLS

#define BTK_TYPE_HANDLE_BOX            (btk_handle_box_get_type ())
#define BTK_HANDLE_BOX(obj)            (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_HANDLE_BOX, BtkHandleBox))
#define BTK_HANDLE_BOX_CLASS(klass)    (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_HANDLE_BOX, BtkHandleBoxClass))
#define BTK_IS_HANDLE_BOX(obj)         (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_HANDLE_BOX))
#define BTK_IS_HANDLE_BOX_CLASS(klass) (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_HANDLE_BOX))
#define BTK_HANDLE_BOX_GET_CLASS(obj)  (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_HANDLE_BOX, BtkHandleBoxClass))


typedef struct _BtkHandleBox       BtkHandleBox;
typedef struct _BtkHandleBoxClass  BtkHandleBoxClass;

struct _BtkHandleBox
{
  BtkBin bin;

  BdkWindow      *GSEAL (bin_window);	/* parent window for children */
  BdkWindow      *GSEAL (float_window);
  BtkShadowType   GSEAL (shadow_type);
  guint           GSEAL (handle_position) : 2;
  guint           GSEAL (float_window_mapped) : 1;
  guint           GSEAL (child_detached) : 1;
  guint           GSEAL (in_drag) : 1;
  guint           GSEAL (shrink_on_detach) : 1;

  signed int      GSEAL (snap_edge : 3); /* -1 == unset */

  /* Variables used during a drag
   */
  gint            GSEAL (deskoff_x); /* Offset between root relative coords */
  gint            GSEAL (deskoff_y); /* and deskrelative coords             */

  BtkAllocation   GSEAL (attach_allocation);
  BtkAllocation   GSEAL (float_allocation);
};

struct _BtkHandleBoxClass
{
  BtkBinClass parent_class;

  void	(*child_attached)	(BtkHandleBox	*handle_box,
				 BtkWidget	*child);
  void	(*child_detached)	(BtkHandleBox	*handle_box,
				 BtkWidget	*child);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};


GType         btk_handle_box_get_type             (void) B_GNUC_CONST;
BtkWidget*    btk_handle_box_new                  (void);
void          btk_handle_box_set_shadow_type      (BtkHandleBox    *handle_box,
                                                   BtkShadowType    type);
BtkShadowType btk_handle_box_get_shadow_type      (BtkHandleBox    *handle_box);
void          btk_handle_box_set_handle_position  (BtkHandleBox    *handle_box,
					           BtkPositionType  position);
BtkPositionType btk_handle_box_get_handle_position(BtkHandleBox    *handle_box);
void          btk_handle_box_set_snap_edge        (BtkHandleBox    *handle_box,
						   BtkPositionType  edge);
BtkPositionType btk_handle_box_get_snap_edge      (BtkHandleBox    *handle_box);
gboolean      btk_handle_box_get_child_detached   (BtkHandleBox    *handle_box);

B_END_DECLS

#endif /* __BTK_HANDLE_BOX_H__ */
