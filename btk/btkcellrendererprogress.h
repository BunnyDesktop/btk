/* btkcellrendererprogress.h
 * Copyright (C) 2002 Naba Kumar <kh_naba@users.sourceforge.net>
 * modified by Jörgen Scheibengruber <mfcn@gmx.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the BTK+ Team and others 1997-2004.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/.
 */

#ifndef __BTK_CELL_RENDERER_PROGRESS_H__
#define __BTK_CELL_RENDERER_PROGRESS_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkcellrenderer.h>

B_BEGIN_DECLS

#define BTK_TYPE_CELL_RENDERER_PROGRESS (btk_cell_renderer_progress_get_type ())
#define BTK_CELL_RENDERER_PROGRESS(obj) (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_CELL_RENDERER_PROGRESS, BtkCellRendererProgress))
#define BTK_CELL_RENDERER_PROGRESS_CLASS(klass)	  (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_CELL_RENDERER_PROGRESS, BtkCellRendererProgressClass))
#define BTK_IS_CELL_RENDERER_PROGRESS(obj)	  (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_CELL_RENDERER_PROGRESS))
#define BTK_IS_CELL_RENDERER_PROGRESS_CLASS(klass) (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_CELL_RENDERER_PROGRESS))
#define BTK_CELL_RENDERER_PROGRESS_GET_CLASS(obj)  (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_CELL_RENDERER_PROGRESS, BtkCellRendererProgressClass))

typedef struct _BtkCellRendererProgress         BtkCellRendererProgress;
typedef struct _BtkCellRendererProgressClass    BtkCellRendererProgressClass;
typedef struct _BtkCellRendererProgressPrivate  BtkCellRendererProgressPrivate;

struct _BtkCellRendererProgress
{
  BtkCellRenderer parent_instance;

  /*< private >*/
  BtkCellRendererProgressPrivate *GSEAL (priv);
};

struct _BtkCellRendererProgressClass
{
  BtkCellRendererClass parent_class;

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};

GType		 btk_cell_renderer_progress_get_type (void) B_GNUC_CONST;
BtkCellRenderer* btk_cell_renderer_progress_new      (void);

B_END_DECLS

#endif  /* __BTK_CELL_RENDERER_PROGRESS_H__ */
