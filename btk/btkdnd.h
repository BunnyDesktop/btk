/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
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

#ifndef __BTK_DND_H__
#define __BTK_DND_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkwidget.h>
#include <btk/btkselection.h>


B_BEGIN_DECLS

typedef enum {
  BTK_DEST_DEFAULT_MOTION     = 1 << 0, /* respond to "drag_motion" */
  BTK_DEST_DEFAULT_HIGHLIGHT  = 1 << 1, /* auto-highlight */
  BTK_DEST_DEFAULT_DROP       = 1 << 2, /* respond to "drag_drop" */
  BTK_DEST_DEFAULT_ALL        = 0x07
} BtkDestDefaults;

/* Flags for the BtkTargetEntry on the destination side
 */
typedef enum {
  BTK_TARGET_SAME_APP = 1 << 0,    /*< nick=same-app >*/
  BTK_TARGET_SAME_WIDGET = 1 << 1, /*< nick=same-widget >*/
  BTK_TARGET_OTHER_APP = 1 << 2,   /*< nick=other-app >*/
  BTK_TARGET_OTHER_WIDGET = 1 << 3 /*< nick=other-widget >*/
} BtkTargetFlags;

/* Destination side */

void btk_drag_get_data (BtkWidget      *widget,
			BdkDragContext *context,
			BdkAtom         target,
			buint32         time_);
void btk_drag_finish   (BdkDragContext *context,
			bboolean        success,
			bboolean        del,
			buint32         time_);

BtkWidget *btk_drag_get_source_widget (BdkDragContext *context);

void btk_drag_highlight   (BtkWidget  *widget);
void btk_drag_unhighlight (BtkWidget  *widget);

void btk_drag_dest_set   (BtkWidget            *widget,
			  BtkDestDefaults       flags,
  		          const BtkTargetEntry *targets,
			  bint                  n_targets,
			  BdkDragAction         actions);

void btk_drag_dest_set_proxy (BtkWidget      *widget,
			      BdkWindow      *proxy_window,
			      BdkDragProtocol protocol,
			      bboolean        use_coordinates);

void btk_drag_dest_unset (BtkWidget          *widget);

BdkAtom        btk_drag_dest_find_target     (BtkWidget      *widget,
                                              BdkDragContext *context,
                                              BtkTargetList  *target_list);
BtkTargetList* btk_drag_dest_get_target_list (BtkWidget      *widget);
void           btk_drag_dest_set_target_list (BtkWidget      *widget,
                                              BtkTargetList  *target_list);
void           btk_drag_dest_add_text_targets  (BtkWidget    *widget);
void           btk_drag_dest_add_image_targets (BtkWidget    *widget);
void           btk_drag_dest_add_uri_targets   (BtkWidget    *widget);

void           btk_drag_dest_set_track_motion  (BtkWidget *widget,
						bboolean   track_motion);
bboolean       btk_drag_dest_get_track_motion  (BtkWidget *widget);

/* Source side */

void btk_drag_source_set  (BtkWidget            *widget,
			   BdkModifierType       start_button_mask,
			   const BtkTargetEntry *targets,
			   bint                  n_targets,
			   BdkDragAction         actions);

void btk_drag_source_unset (BtkWidget        *widget);

BtkTargetList* btk_drag_source_get_target_list (BtkWidget     *widget);
void           btk_drag_source_set_target_list (BtkWidget     *widget,
                                                BtkTargetList *target_list);
void           btk_drag_source_add_text_targets  (BtkWidget     *widget);
void           btk_drag_source_add_image_targets (BtkWidget    *widget);
void           btk_drag_source_add_uri_targets   (BtkWidget    *widget);

void btk_drag_source_set_icon        (BtkWidget   *widget,
				      BdkColormap *colormap,
				      BdkPixmap   *pixmap,
				      BdkBitmap   *mask);
void btk_drag_source_set_icon_pixbuf (BtkWidget   *widget,
				      BdkPixbuf   *pixbuf);
void btk_drag_source_set_icon_stock  (BtkWidget   *widget,
				      const bchar *stock_id);
void btk_drag_source_set_icon_name   (BtkWidget   *widget,
				      const bchar *icon_name);

/* There probably should be functions for setting the targets
 * as a BtkTargetList
 */

BdkDragContext *btk_drag_begin (BtkWidget         *widget,
				BtkTargetList     *targets,
				BdkDragAction      actions,
				bint               button,
				BdkEvent          *event);

/* Set the image being dragged around
 */
void btk_drag_set_icon_widget (BdkDragContext *context,
			       BtkWidget      *widget,
			       bint            hot_x,
			       bint            hot_y);
void btk_drag_set_icon_pixmap (BdkDragContext *context,
			       BdkColormap    *colormap,
			       BdkPixmap      *pixmap,
			       BdkBitmap      *mask,
			       bint            hot_x,
			       bint            hot_y);
void btk_drag_set_icon_pixbuf (BdkDragContext *context,
			       BdkPixbuf      *pixbuf,
			       bint            hot_x,
			       bint            hot_y);
void btk_drag_set_icon_stock  (BdkDragContext *context,
			       const bchar    *stock_id,
			       bint            hot_x,
			       bint            hot_y);
void btk_drag_set_icon_name   (BdkDragContext *context,
			       const bchar    *icon_name,
			       bint            hot_x,
			       bint            hot_y);

void btk_drag_set_icon_default (BdkDragContext    *context);

bboolean btk_drag_check_threshold (BtkWidget *widget,
				   bint       start_x,
				   bint       start_y,
				   bint       current_x,
				   bint       current_y);

/* Internal functions */
void _btk_drag_source_handle_event (BtkWidget *widget,
				    BdkEvent  *event);
void _btk_drag_dest_handle_event (BtkWidget *toplevel,
				  BdkEvent  *event);

#ifndef BTK_DISABLE_DEPRECATED
void btk_drag_set_default_icon (BdkColormap   *colormap,
				BdkPixmap     *pixmap,
				BdkBitmap     *mask,
			        bint           hot_x,
			        bint           hot_y);
#endif /* !BTK_DISABLE_DEPRECATED */

B_END_DECLS

#endif /* __BTK_DND_H__ */
