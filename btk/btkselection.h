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

#ifndef __BTK_SELECTION_H__
#define __BTK_SELECTION_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkwidget.h>
#include <btk/btktextiter.h>


B_BEGIN_DECLS

typedef struct _BtkTargetList    BtkTargetList;
typedef struct _BtkTargetEntry   BtkTargetEntry;

#define BTK_TYPE_SELECTION_DATA (btk_selection_data_get_type ())
#define BTK_TYPE_TARGET_LIST    (btk_target_list_get_type ())

/* The contents of a selection are returned in a BtkSelectionData
 * structure. selection/target identify the request.  type specifies
 * the type of the return; if length < 0, and the data should be
 * ignored. This structure has object semantics - no fields should be
 * modified directly, they should not be created directly, and
 * pointers to them should not be stored beyond the duration of a
 * callback. (If the last is changed, we'll need to add reference
 * counting.) The time field gives the timestamp at which the data was
 * sent.
 */

struct _BtkSelectionData
{
  BdkAtom       GSEAL (selection);
  BdkAtom       GSEAL (target);
  BdkAtom       GSEAL (type);
  bint          GSEAL (format);
  buchar       *GSEAL (data);
  bint          GSEAL (length);
  BdkDisplay   *GSEAL (display);
};

struct _BtkTargetEntry {
  bchar *target;
  buint  flags;
  buint  info;
};

/* These structures not public, and are here only for the convenience of
 * btkdnd.c
 */

typedef struct _BtkTargetPair BtkTargetPair;

/* This structure is a list of destinations, and associated buint id's */
struct _BtkTargetList {
  GList *list;
  buint ref_count;
};

struct _BtkTargetPair {
  BdkAtom   target;
  buint     flags;
  buint     info;
};

BtkTargetList *btk_target_list_new       (const BtkTargetEntry *targets,
					  buint                 ntargets);
BtkTargetList *btk_target_list_ref       (BtkTargetList  *list);
void           btk_target_list_unref     (BtkTargetList  *list);
void           btk_target_list_add       (BtkTargetList  *list,
				  	  BdkAtom         target,
					  buint           flags,
					  buint           info);
void           btk_target_list_add_text_targets      (BtkTargetList  *list,
                                                      buint           info);
void           btk_target_list_add_rich_text_targets (BtkTargetList  *list,
                                                      buint           info,
                                                      bboolean        deserializable,
                                                      BtkTextBuffer  *buffer);
void           btk_target_list_add_image_targets     (BtkTargetList  *list,
                                                      buint           info,
                                                      bboolean        writable);
void           btk_target_list_add_uri_targets       (BtkTargetList  *list,
                                                      buint           info);
void           btk_target_list_add_table (BtkTargetList        *list,
					  const BtkTargetEntry *targets,
					  buint                 ntargets);
void           btk_target_list_remove    (BtkTargetList  *list,
					  BdkAtom         target);
bboolean       btk_target_list_find      (BtkTargetList  *list,
					  BdkAtom         target,
					  buint          *info);

BtkTargetEntry * btk_target_table_new_from_list (BtkTargetList  *list,
                                                 bint           *n_targets);
void             btk_target_table_free          (BtkTargetEntry *targets,
                                                 bint            n_targets);

/* Public interface */

bboolean btk_selection_owner_set             (BtkWidget  *widget,
					      BdkAtom     selection,
					      buint32     time_);
bboolean btk_selection_owner_set_for_display (BdkDisplay *display,
					      BtkWidget  *widget,
					      BdkAtom     selection,
					      buint32     time_);

void     btk_selection_add_target    (BtkWidget            *widget,
				      BdkAtom               selection,
				      BdkAtom               target,
				      buint                 info);
void     btk_selection_add_targets   (BtkWidget            *widget,
				      BdkAtom               selection,
				      const BtkTargetEntry *targets,
				      buint                 ntargets);
void     btk_selection_clear_targets (BtkWidget            *widget,
				      BdkAtom               selection);
bboolean btk_selection_convert       (BtkWidget            *widget,
				      BdkAtom               selection,
				      BdkAtom               target,
				      buint32               time_);

BdkAtom       btk_selection_data_get_selection (BtkSelectionData *selection_data);
BdkAtom       btk_selection_data_get_target    (BtkSelectionData *selection_data);
BdkAtom       btk_selection_data_get_data_type (BtkSelectionData *selection_data);
bint          btk_selection_data_get_format    (BtkSelectionData *selection_data);
const buchar *btk_selection_data_get_data      (BtkSelectionData *selection_data);
bint          btk_selection_data_get_length    (BtkSelectionData *selection_data);
BdkDisplay   *btk_selection_data_get_display   (BtkSelectionData *selection_data);

void     btk_selection_data_set      (BtkSelectionData     *selection_data,
				      BdkAtom               type,
				      bint                  format,
				      const buchar         *data,
				      bint                  length);
bboolean btk_selection_data_set_text (BtkSelectionData     *selection_data,
				      const bchar          *str,
				      bint                  len);
buchar * btk_selection_data_get_text (BtkSelectionData     *selection_data);
bboolean btk_selection_data_set_pixbuf   (BtkSelectionData  *selection_data,
				          BdkPixbuf         *pixbuf);
BdkPixbuf *btk_selection_data_get_pixbuf (BtkSelectionData  *selection_data);
bboolean btk_selection_data_set_uris (BtkSelectionData     *selection_data,
				      bchar               **uris);
bchar  **btk_selection_data_get_uris (BtkSelectionData     *selection_data);

bboolean btk_selection_data_get_targets          (BtkSelectionData  *selection_data,
						  BdkAtom          **targets,
						  bint              *n_atoms);
bboolean btk_selection_data_targets_include_text (BtkSelectionData  *selection_data);
bboolean btk_selection_data_targets_include_rich_text (BtkSelectionData *selection_data,
                                                       BtkTextBuffer    *buffer);
bboolean btk_selection_data_targets_include_image (BtkSelectionData  *selection_data,
						   bboolean           writable);
bboolean btk_selection_data_targets_include_uri  (BtkSelectionData  *selection_data);
bboolean btk_targets_include_text                (BdkAtom       *targets,
						  bint           n_targets);
bboolean btk_targets_include_rich_text           (BdkAtom       *targets,
						  bint           n_targets,
                                                  BtkTextBuffer *buffer);
bboolean btk_targets_include_image               (BdkAtom       *targets,
						  bint           n_targets,
						  bboolean       writable);
bboolean btk_targets_include_uri                 (BdkAtom       *targets,
						  bint           n_targets);

/* Called when a widget is destroyed */

void btk_selection_remove_all      (BtkWidget *widget);

/* Event handlers */
#if !defined(BTK_DISABLE_DEPRECATED) || defined (BTK_COMPILATION)
bboolean btk_selection_clear		  (BtkWidget 	     *widget,
					   BdkEventSelection *event);
#endif
bboolean _btk_selection_request		  (BtkWidget  	     *widget,
					   BdkEventSelection *event);
bboolean _btk_selection_incr_event	  (BdkWindow         *window,
					   BdkEventProperty  *event);
bboolean _btk_selection_notify		  (BtkWidget         *widget,
					   BdkEventSelection *event);
bboolean _btk_selection_property_notify	  (BtkWidget         *widget,
					   BdkEventProperty  *event);

GType             btk_selection_data_get_type (void) B_GNUC_CONST;
BtkSelectionData *btk_selection_data_copy     (BtkSelectionData *data);
void		  btk_selection_data_free     (BtkSelectionData *data);

GType             btk_target_list_get_type    (void) B_GNUC_CONST;

B_END_DECLS

#endif /* __BTK_SELECTION_H__ */
