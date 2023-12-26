/* BDK - The GIMP Drawing Kit
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

#ifndef __BDK_DND_H__
#define __BDK_DND_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BDK_H_INSIDE__) && !defined (BDK_COMPILATION)
#error "Only <bdk/bdk.h> can be included directly."
#endif

#include <bdk/bdktypes.h>

B_BEGIN_DECLS

typedef struct _BdkDragContext        BdkDragContext;

typedef enum
{
  BDK_ACTION_DEFAULT = 1 << 0,
  BDK_ACTION_COPY    = 1 << 1,
  BDK_ACTION_MOVE    = 1 << 2,
  BDK_ACTION_LINK    = 1 << 3,
  BDK_ACTION_PRIVATE = 1 << 4,
  BDK_ACTION_ASK     = 1 << 5
} BdkDragAction;

typedef enum
{
  BDK_DRAG_PROTO_MOTIF,
  BDK_DRAG_PROTO_XDND,
  BDK_DRAG_PROTO_ROOTWIN,	  /* A root window with nobody claiming
				   * drags */
  BDK_DRAG_PROTO_NONE,		  /* Not a valid drag window */
  BDK_DRAG_PROTO_WIN32_DROPFILES, /* The simple WM_DROPFILES dnd */
  BDK_DRAG_PROTO_OLE2,		  /* The complex OLE2 dnd (not implemented) */
  BDK_DRAG_PROTO_LOCAL            /* Intra-app */
} BdkDragProtocol;

/* Object that holds information about a drag in progress.
 * this is used on both source and destination sides.
 */

typedef struct _BdkDragContextClass BdkDragContextClass;

#define BDK_TYPE_DRAG_CONTEXT              (bdk_drag_context_get_type ())
#define BDK_DRAG_CONTEXT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BDK_TYPE_DRAG_CONTEXT, BdkDragContext))
#define BDK_DRAG_CONTEXT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BDK_TYPE_DRAG_CONTEXT, BdkDragContextClass))
#define BDK_IS_DRAG_CONTEXT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BDK_TYPE_DRAG_CONTEXT))
#define BDK_IS_DRAG_CONTEXT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BDK_TYPE_DRAG_CONTEXT))
#define BDK_DRAG_CONTEXT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), BDK_TYPE_DRAG_CONTEXT, BdkDragContextClass))

struct _BdkDragContext {
  GObject parent_instance;

  /*< public >*/
  
  BdkDragProtocol GSEAL (protocol);

  gboolean GSEAL (is_source);
  
  BdkWindow *GSEAL (source_window);
  BdkWindow *GSEAL (dest_window);

  GList *GSEAL (targets);
  BdkDragAction GSEAL (actions);
  BdkDragAction GSEAL (suggested_action);
  BdkDragAction GSEAL (action);

  guint32 GSEAL (start_time);

  /*< private >*/
  
  gpointer GSEAL (windowing_data);
};

struct _BdkDragContextClass {
  GObjectClass parent_class;

};

/* Drag and Drop */

GType            bdk_drag_context_get_type   (void) B_GNUC_CONST;
#if !defined (BDK_DISABLE_DEPRECATED) || defined (BDK_COMPILATION)
BdkDragContext * bdk_drag_context_new        (void);
#endif

GList           *bdk_drag_context_list_targets         (BdkDragContext *context);
BdkDragAction    bdk_drag_context_get_actions          (BdkDragContext *context);
BdkDragAction    bdk_drag_context_get_suggested_action (BdkDragContext *context);
BdkDragAction    bdk_drag_context_get_selected_action  (BdkDragContext *context);

BdkWindow       *bdk_drag_context_get_source_window    (BdkDragContext *context);
BdkWindow       *bdk_drag_context_get_dest_window      (BdkDragContext *context);
BdkDragProtocol  bdk_drag_context_get_protocol         (BdkDragContext *context);


#ifndef BDK_DISABLE_DEPRECATED
void             bdk_drag_context_ref        (BdkDragContext *context);
void             bdk_drag_context_unref      (BdkDragContext *context);
#endif

/* Destination side */

void             bdk_drag_status        (BdkDragContext   *context,
				         BdkDragAction     action,
					 guint32           time_);
void             bdk_drop_reply         (BdkDragContext   *context,
					 gboolean          ok,
					 guint32           time_);
void             bdk_drop_finish        (BdkDragContext   *context,
					 gboolean          success,
					 guint32           time_);
BdkAtom          bdk_drag_get_selection (BdkDragContext   *context);

/* Source side */

BdkDragContext * bdk_drag_begin      (BdkWindow      *window,
				      GList          *targets);

BdkNativeWindow bdk_drag_get_protocol_for_display (BdkDisplay       *display,
						   BdkNativeWindow   xid,
						   BdkDragProtocol  *protocol);

void    bdk_drag_find_window_for_screen   (BdkDragContext   *context,
					   BdkWindow        *drag_window,
					   BdkScreen        *screen,
					   gint              x_root,
					   gint              y_root,
					   BdkWindow       **dest_window,
					   BdkDragProtocol  *protocol);

#ifndef BDK_MULTIHEAD_SAFE
#ifndef BDK_DISABLE_DEPRECATED
BdkNativeWindow bdk_drag_get_protocol (BdkNativeWindow   xid,
				       BdkDragProtocol  *protocol);

void    bdk_drag_find_window  (BdkDragContext   *context,
			       BdkWindow        *drag_window,
			       gint              x_root,
			       gint              y_root,
			       BdkWindow       **dest_window,
			       BdkDragProtocol  *protocol);
#endif /* BDK_DISABLE_DEPRECATED */
#endif /* BDK_MULTIHEAD_SAFE */

gboolean        bdk_drag_motion      (BdkDragContext *context,
				      BdkWindow      *dest_window,
				      BdkDragProtocol protocol,
				      gint            x_root, 
				      gint            y_root,
				      BdkDragAction   suggested_action,
				      BdkDragAction   possible_actions,
				      guint32         time_);
void            bdk_drag_drop        (BdkDragContext *context,
				      guint32         time_);
void            bdk_drag_abort       (BdkDragContext *context,
				      guint32         time_);
gboolean        bdk_drag_drop_succeeded (BdkDragContext *context);

B_END_DECLS

#endif /* __BDK_DND_H__ */
