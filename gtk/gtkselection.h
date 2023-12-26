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


G_BEGIN_DECLS

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
  gint          GSEAL (format);
  guchar       *GSEAL (data);
  gint          GSEAL (length);
  BdkDisplay   *GSEAL (display);
};

struct _BtkTargetEntry {
  gchar *target;
  guint  flags;
  guint  info;
};

/* These structures not public, and are here only for the convenience of
 * btkdnd.c
 */

typedef struct _BtkTargetPair BtkTargetPair;

/* This structure is a list of destinations, and associated guint id's */
struct _BtkTargetList {
  GList *list;
  guint ref_count;
};

struct _BtkTargetPair {
  BdkAtom   target;
  guint     flags;
  guint     info;
};

BtkTargetList *btk_target_list_new       (const BtkTargetEntry *targets,
					  guint                 ntargets);
BtkTargetList *btk_target_list_ref       (BtkTargetList  *list);
void           btk_target_list_unref     (BtkTargetList  *list);
void           btk_target_list_add       (BtkTargetList  *list,
				  	  BdkAtom         target,
					  guint           flags,
					  guint           info);
void           btk_target_list_add_text_targets      (BtkTargetList  *list,
                                                      guint           info);
void           btk_target_list_add_rich_text_targets (BtkTargetList  *list,
                                                      guint           info,
                                                      gboolean        deserializable,
                                                      BtkTextBuffer  *buffer);
void           btk_target_list_add_image_targets     (BtkTargetList  *list,
                                                      guint           info,
                                                      gboolean        writable);
void           btk_target_list_add_uri_targets       (BtkTargetList  *list,
                                                      guint           info);
void           btk_target_list_add_table (BtkTargetList        *list,
					  const BtkTargetEntry *targets,
					  guint                 ntargets);
void           btk_target_list_remove    (BtkTargetList  *list,
					  BdkAtom         target);
gboolean       btk_target_list_find      (BtkTargetList  *list,
					  BdkAtom         target,
					  guint          *info);

BtkTargetEntry * btk_target_table_new_from_list (BtkTargetList  *list,
                                                 gint           *n_targets);
void             btk_target_table_free          (BtkTargetEntry *targets,
                                                 gint            n_targets);

/* Public interface */

gboolean btk_selection_owner_set             (BtkWidget  *widget,
					      BdkAtom     selection,
					      guint32     time_);
gboolean btk_selection_owner_set_for_display (BdkDisplay *display,
					      BtkWidget  *widget,
					      BdkAtom     selection,
					      guint32     time_);

void     btk_selection_add_target    (BtkWidget            *widget,
				      BdkAtom               selection,
				      BdkAtom               target,
				      guint                 info);
void     btk_selection_add_targets   (BtkWidget            *widget,
				      BdkAtom               selection,
				      const BtkTargetEntry *targets,
				      guint                 ntargets);
void     btk_selection_clear_targets (BtkWidget            *widget,
				      BdkAtom               selection);
gboolean btk_selection_convert       (BtkWidget            *widget,
				      BdkAtom               selection,
				      BdkAtom               target,
				      guint32               time_);

BdkAtom       btk_selection_data_get_selection (BtkSelectionData *selection_data);
BdkAtom       btk_selection_data_get_target    (BtkSelectionData *selection_data);
BdkAtom       btk_selection_data_get_data_type (BtkSelectionData *selection_data);
gint          btk_selection_data_get_format    (BtkSelectionData *selection_data);
const guchar *btk_selection_data_get_data      (BtkSelectionData *selection_data);
gint          btk_selection_data_get_length    (BtkSelectionData *selection_data);
BdkDisplay   *btk_selection_data_get_display   (BtkSelectionData *selection_data);

void     btk_selection_data_set      (BtkSelectionData     *selection_data,
				      BdkAtom               type,
				      gint                  format,
				      const guchar         *data,
				      gint                  length);
gboolean btk_selection_data_set_text (BtkSelectionData     *selection_data,
				      const gchar          *str,
				      gint                  len);
guchar * btk_selection_data_get_text (BtkSelectionData     *selection_data);
gboolean btk_selection_data_set_pixbuf   (BtkSelectionData  *selection_data,
				          BdkPixbuf         *pixbuf);
BdkPixbuf *btk_selection_data_get_pixbuf (BtkSelectionData  *selection_data);
gboolean btk_selection_data_set_uris (BtkSelectionData     *selection_data,
				      gchar               **uris);
gchar  **btk_selection_data_get_uris (BtkSelectionData     *selection_data);

gboolean btk_selection_data_get_targets          (BtkSelectionData  *selection_data,
						  BdkAtom          **targets,
						  gint              *n_atoms);
gboolean btk_selection_data_targets_include_text (BtkSelectionData  *selection_data);
gboolean btk_selection_data_targets_include_rich_text (BtkSelectionData *selection_data,
                                                       BtkTextBuffer    *buffer);
gboolean btk_selection_data_targets_include_image (BtkSelectionData  *selection_data,
						   gboolean           writable);
gboolean btk_selection_data_targets_include_uri  (BtkSelectionData  *selection_data);
gboolean btk_targets_include_text                (BdkAtom       *targets,
						  gint           n_targets);
gboolean btk_targets_include_rich_text           (BdkAtom       *targets,
						  gint           n_targets,
                                                  BtkTextBuffer *buffer);
gboolean btk_targets_include_image               (BdkAtom       *targets,
						  gint           n_targets,
						  gboolean       writable);
gboolean btk_targets_include_uri                 (BdkAtom       *targets,
						  gint           n_targets);

/* Called when a widget is destroyed */

void btk_selection_remove_all      (BtkWidget *widget);

/* Event handlers */
#if !defined(BTK_DISABLE_DEPRECATED) || defined (BTK_COMPILATION)
gboolean btk_selection_clear		  (BtkWidget 	     *widget,
					   BdkEventSelection *event);
#endif
gboolean _btk_selection_request		  (BtkWidget  	     *widget,
					   BdkEventSelection *event);
gboolean _btk_selection_incr_event	  (BdkWindow         *window,
					   BdkEventProperty  *event);
gboolean _btk_selection_notify		  (BtkWidget         *widget,
					   BdkEventSelection *event);
gboolean _btk_selection_property_notify	  (BtkWidget         *widget,
					   BdkEventProperty  *event);

GType             btk_selection_data_get_type (void) G_GNUC_CONST;
BtkSelectionData *btk_selection_data_copy     (BtkSelectionData *data);
void		  btk_selection_data_free     (BtkSelectionData *data);

GType             btk_target_list_get_type    (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __BTK_SELECTION_H__ */
