/* BTK - The GIMP Toolkit
 *
 * Copyright (C) 2007 John Stowers, Neil Jagdish Patel.
 * Copyright (C) 2009 Bastien Nocera, David Zeuthen
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
 * Boston, MA  02111-1307, USA.
 *
 * Code adapted from egg-spinner
 * by Christian Hergert <christian.hergert@gmail.com>
 */

#ifndef __BTK_SPINNER_H__
#define __BTK_SPINNER_H__

#if !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkdrawingarea.h>

B_BEGIN_DECLS

#define BTK_TYPE_SPINNER           (btk_spinner_get_type ())
#define BTK_SPINNER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_SPINNER, BtkSpinner))
#define BTK_SPINNER_CLASS(obj)     (G_TYPE_CHECK_CLASS_CAST ((obj), BTK_SPINNER,  BtkSpinnerClass))
#define BTK_IS_SPINNER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_SPINNER))
#define BTK_IS_SPINNER_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((obj), BTK_TYPE_SPINNER))
#define BTK_SPINNER_GET_CLASS      (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_SPINNER, BtkSpinnerClass))

typedef struct _BtkSpinner      BtkSpinner;
typedef struct _BtkSpinnerClass BtkSpinnerClass;
typedef struct _BtkSpinnerPrivate  BtkSpinnerPrivate;

struct _BtkSpinner
{
  BtkDrawingArea parent;
  BtkSpinnerPrivate *priv;
};

struct _BtkSpinnerClass
{
  BtkDrawingAreaClass parent_class;
};

GType      btk_spinner_get_type  (void) B_GNUC_CONST;
BtkWidget *btk_spinner_new (void);
void       btk_spinner_start      (BtkSpinner *spinner);
void       btk_spinner_stop       (BtkSpinner *spinner);

B_END_DECLS

#endif /* __BTK_SPINNER_H__ */
