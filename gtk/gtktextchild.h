/* BTK - The GIMP Toolkit
 * btktextchild.h Copyright (C) 2000 Red Hat, Inc.
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

#ifndef __BTK_TEXT_CHILD_H__
#define __BTK_TEXT_CHILD_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <bdkconfig.h>
#include <bunnylib-object.h>

G_BEGIN_DECLS

/* A BtkTextChildAnchor is a spot in the buffer where child widgets
 * can be "anchored" (inserted inline, as if they were characters).
 * The anchor can have multiple widgets anchored, to allow for multiple
 * views.
 */

typedef struct _BtkTextChildAnchor      BtkTextChildAnchor;
typedef struct _BtkTextChildAnchorClass BtkTextChildAnchorClass;

#define BTK_TYPE_TEXT_CHILD_ANCHOR              (btk_text_child_anchor_get_type ())
#define BTK_TEXT_CHILD_ANCHOR(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BTK_TYPE_TEXT_CHILD_ANCHOR, BtkTextChildAnchor))
#define BTK_TEXT_CHILD_ANCHOR_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_TEXT_CHILD_ANCHOR, BtkTextChildAnchorClass))
#define BTK_IS_TEXT_CHILD_ANCHOR(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BTK_TYPE_TEXT_CHILD_ANCHOR))
#define BTK_IS_TEXT_CHILD_ANCHOR_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_TEXT_CHILD_ANCHOR))
#define BTK_TEXT_CHILD_ANCHOR_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_TEXT_CHILD_ANCHOR, BtkTextChildAnchorClass))

struct _BtkTextChildAnchor
{
  GObject parent_instance;

  gpointer GSEAL (segment);
};

struct _BtkTextChildAnchorClass
{
  GObjectClass parent_class;

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};

GType btk_text_child_anchor_get_type (void) G_GNUC_CONST;

BtkTextChildAnchor* btk_text_child_anchor_new (void);

GList*   btk_text_child_anchor_get_widgets (BtkTextChildAnchor *anchor);
gboolean btk_text_child_anchor_get_deleted (BtkTextChildAnchor *anchor);

G_END_DECLS

#endif
