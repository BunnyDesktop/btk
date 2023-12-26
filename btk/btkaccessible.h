/* BTK - The GIMP Toolkit
 * Copyright 2001 Sun Microsystems Inc.
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

#ifndef __BTK_ACCESSIBLE_H__
#define __BTK_ACCESSIBLE_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <batk/batk.h>
#include <btk/btkwidget.h>

G_BEGIN_DECLS

#define BTK_TYPE_ACCESSIBLE                  (btk_accessible_get_type ())
#define BTK_ACCESSIBLE(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_ACCESSIBLE, BtkAccessible))
#define BTK_ACCESSIBLE_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_ACCESSIBLE, BtkAccessibleClass))
#define BTK_IS_ACCESSIBLE(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_ACCESSIBLE))
#define BTK_IS_ACCESSIBLE_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_ACCESSIBLE))
#define BTK_ACCESSIBLE_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_ACCESSIBLE, BtkAccessibleClass))

typedef struct _BtkAccessible                BtkAccessible;
typedef struct _BtkAccessibleClass           BtkAccessibleClass;

  /*
   * This object is a thin wrapper, in the BTK+ namespace, for BatkObject
   */
struct _BtkAccessible
{
  BatkObject parent;

  /*
   * The BtkWidget whose properties and features are exported via this 
   * accessible instance.
   */
  BtkWidget *GSEAL (widget);
};

struct _BtkAccessibleClass
{
  BatkObjectClass parent_class;

  void (*connect_widget_destroyed)              (BtkAccessible     *accessible);
  
  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};

GType btk_accessible_get_type (void) G_GNUC_CONST;

void        btk_accessible_set_widget                  (BtkAccessible     *accessible,
                                                        BtkWidget         *widget);
BtkWidget*  btk_accessible_get_widget                  (BtkAccessible     *accessible);
void        btk_accessible_connect_widget_destroyed    (BtkAccessible     *accessible);

G_END_DECLS

#endif /* __BTK_ACCESSIBLE_H__ */


