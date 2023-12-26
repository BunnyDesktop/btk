/* BTK - The GIMP Toolkit
 * btksizegroup.h:
 * Copyright (C) 2000 Red Hat Software
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

#ifndef __BTK_SIZE_GROUP_H__
#define __BTK_SIZE_GROUP_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkwidget.h>

B_BEGIN_DECLS

#define BTK_TYPE_SIZE_GROUP            (btk_size_group_get_type ())
#define BTK_SIZE_GROUP(obj)            (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_SIZE_GROUP, BtkSizeGroup))
#define BTK_SIZE_GROUP_CLASS(klass)    (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_SIZE_GROUP, BtkSizeGroupClass))
#define BTK_IS_SIZE_GROUP(obj)         (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_SIZE_GROUP))
#define BTK_IS_SIZE_GROUP_CLASS(klass) (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_SIZE_GROUP))
#define BTK_SIZE_GROUP_GET_CLASS(obj)  (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_SIZE_GROUP, BtkSizeGroupClass))


typedef struct _BtkSizeGroup       BtkSizeGroup;
typedef struct _BtkSizeGroupClass  BtkSizeGroupClass;

struct _BtkSizeGroup
{
  BObject parent_instance;

  /* <private> */
  GSList *GSEAL (widgets);

  guint8 GSEAL (mode);

  guint GSEAL (have_width) : 1;
  guint GSEAL (have_height) : 1;
  guint GSEAL (ignore_hidden) : 1;

  BtkRequisition GSEAL (requisition);
};

struct _BtkSizeGroupClass
{
  BObjectClass parent_class;

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};

/**
 * BtkSizeGroupMode:
 * @BTK_SIZE_GROUP_NONE: group has no effect
 * @BTK_SIZE_GROUP_HORIZONTAL: group affects horizontal requisition
 * @BTK_SIZE_GROUP_VERTICAL: group affects vertical requisition
 * @BTK_SIZE_GROUP_BOTH: group affects both horizontal and vertical requisition
 *
 * The mode of the size group determines the directions in which the size
 * group affects the requested sizes of its component widgets.
 **/
typedef enum {
  BTK_SIZE_GROUP_NONE,
  BTK_SIZE_GROUP_HORIZONTAL,
  BTK_SIZE_GROUP_VERTICAL,
  BTK_SIZE_GROUP_BOTH
} BtkSizeGroupMode;

GType            btk_size_group_get_type      (void) B_GNUC_CONST;

BtkSizeGroup *   btk_size_group_new           (BtkSizeGroupMode  mode);
void             btk_size_group_set_mode      (BtkSizeGroup     *size_group,
					       BtkSizeGroupMode  mode);
BtkSizeGroupMode btk_size_group_get_mode      (BtkSizeGroup     *size_group);
void             btk_size_group_set_ignore_hidden (BtkSizeGroup *size_group,
						   gboolean      ignore_hidden);
gboolean         btk_size_group_get_ignore_hidden (BtkSizeGroup *size_group);
void             btk_size_group_add_widget    (BtkSizeGroup     *size_group,
					       BtkWidget        *widget);
void             btk_size_group_remove_widget (BtkSizeGroup     *size_group,
					       BtkWidget        *widget);
GSList *         btk_size_group_get_widgets   (BtkSizeGroup     *size_group);


void _btk_size_group_get_child_requisition (BtkWidget      *widget,
					    BtkRequisition *requisition);
void _btk_size_group_compute_requisition   (BtkWidget      *widget,
					    BtkRequisition *requisition);
void _btk_size_group_queue_resize          (BtkWidget      *widget);

B_END_DECLS

#endif /* __BTK_SIZE_GROUP_H__ */
