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

#ifndef __BTK_ITEM_H__
#define __BTK_ITEM_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkbin.h>


G_BEGIN_DECLS

#define BTK_TYPE_ITEM                  (btk_item_get_type ())
#define BTK_ITEM(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_ITEM, BtkItem))
#define BTK_ITEM_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_ITEM, BtkItemClass))
#define BTK_IS_ITEM(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_ITEM))
#define BTK_IS_ITEM_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_ITEM))
#define BTK_ITEM_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_ITEM, BtkItemClass))


typedef struct _BtkItem       BtkItem;
typedef struct _BtkItemClass  BtkItemClass;

struct _BtkItem
{
  BtkBin bin;
};

struct _BtkItemClass
{
  BtkBinClass parent_class;

  void (* select)   (BtkItem *item);
  void (* deselect) (BtkItem *item);
  void (* toggle)   (BtkItem *item);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};


GType btk_item_get_type (void) G_GNUC_CONST;
#if !defined(BTK_DISABLE_DEPRECATED) || defined(BTK_COMPILATION)
void  btk_item_select   (BtkItem *item);
void  btk_item_deselect (BtkItem *item);
void  btk_item_toggle   (BtkItem *item);
#endif

G_END_DECLS

#endif /* __BTK_ITEM_H__ */
