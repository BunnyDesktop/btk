/* btkcellrendereraccel.h
 * Copyright (C) 2000  Red Hat, Inc.,  Jonathan Blandford <jrb@redhat.com>
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

#ifndef __BTK_CELL_RENDERER_ACCEL_H__
#define __BTK_CELL_RENDERER_ACCEL_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkcellrenderertext.h>

G_BEGIN_DECLS

#define BTK_TYPE_CELL_RENDERER_ACCEL            (btk_cell_renderer_accel_get_type ())
#define BTK_CELL_RENDERER_ACCEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_CELL_RENDERER_ACCEL, BtkCellRendererAccel))
#define BTK_CELL_RENDERER_ACCEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_CELL_RENDERER_ACCEL, BtkCellRendererAccelClass))
#define BTK_IS_CELL_RENDERER_ACCEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_CELL_RENDERER_ACCEL))
#define BTK_IS_CELL_RENDERER_ACCEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_CELL_RENDERER_ACCEL))
#define BTK_CELL_RENDERER_ACCEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_CELL_RENDERER_ACCEL, BtkCellRendererAccelClass))

typedef struct _BtkCellRendererAccel      BtkCellRendererAccel;
typedef struct _BtkCellRendererAccelClass BtkCellRendererAccelClass;


typedef enum
{
  BTK_CELL_RENDERER_ACCEL_MODE_BTK,
  BTK_CELL_RENDERER_ACCEL_MODE_OTHER
} BtkCellRendererAccelMode;


struct _BtkCellRendererAccel
{
  BtkCellRendererText parent;

  /*< private >*/
  guint GSEAL (accel_key);
  BdkModifierType GSEAL (accel_mods);
  guint GSEAL (keycode);
  BtkCellRendererAccelMode GSEAL (accel_mode);

  BtkWidget *GSEAL (edit_widget);
  BtkWidget *GSEAL (grab_widget);
  BtkWidget *GSEAL (sizing_label);
};

struct _BtkCellRendererAccelClass
{
  BtkCellRendererTextClass parent_class;

  void (* accel_edited)  (BtkCellRendererAccel *accel,
		 	  const gchar          *path_string,
			  guint                 accel_key,
			  BdkModifierType       accel_mods,
			  guint                 hardware_keycode);

  void (* accel_cleared) (BtkCellRendererAccel *accel,
			  const gchar          *path_string);

  /* Padding for future expansion */
  void (*_btk_reserved0) (void);
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};

GType            btk_cell_renderer_accel_get_type        (void) G_GNUC_CONST;
BtkCellRenderer *btk_cell_renderer_accel_new             (void);


G_END_DECLS


#endif /* __BTK_CELL_RENDERER_ACCEL_H__ */
