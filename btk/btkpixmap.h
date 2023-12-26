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

#if !defined (BTK_DISABLE_DEPRECATED) || defined (__BTK_PIXMAP_C__)

#ifndef __BTK_PIXMAP_H__
#define __BTK_PIXMAP_H__

#include <btk/btk.h>


B_BEGIN_DECLS

#define BTK_TYPE_PIXMAP			 (btk_pixmap_get_type ())
#define BTK_PIXMAP(obj)			 (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_PIXMAP, BtkPixmap))
#define BTK_PIXMAP_CLASS(klass)		 (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_PIXMAP, BtkPixmapClass))
#define BTK_IS_PIXMAP(obj)		 (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_PIXMAP))
#define BTK_IS_PIXMAP_CLASS(klass)	 (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_PIXMAP))
#define BTK_PIXMAP_GET_CLASS(obj)        (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_PIXMAP, BtkPixmapClass))


typedef struct _BtkPixmap	BtkPixmap;
typedef struct _BtkPixmapClass	BtkPixmapClass;

struct _BtkPixmap
{
  BtkMisc misc;

  BdkPixmap *pixmap;
  BdkBitmap *mask;

  BdkPixmap *pixmap_insensitive;
  buint build_insensitive : 1;
};

struct _BtkPixmapClass
{
  BtkMiscClass parent_class;
};


GType	   btk_pixmap_get_type	 (void) B_GNUC_CONST;
BtkWidget* btk_pixmap_new	 (BdkPixmap  *pixmap,
				  BdkBitmap  *mask);
void	   btk_pixmap_set	 (BtkPixmap  *pixmap,
				  BdkPixmap  *val,
				  BdkBitmap  *mask);
void	   btk_pixmap_get	 (BtkPixmap  *pixmap,
				  BdkPixmap **val,
				  BdkBitmap **mask);

void       btk_pixmap_set_build_insensitive (BtkPixmap *pixmap,
		                             bboolean   build);


B_END_DECLS

#endif /* __BTK_PIXMAP_H__ */

#endif /* BTK_DISABLE_DEPRECATED */
