/* BTK - The GIMP Toolkit
 * btkrecentchooserwidget.h: embeddable recently used resources chooser widget
 * Copyright (C) 2006 Emmanuele Bassi
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

#ifndef __BTK_RECENT_CHOOSER_WIDGET_H__
#define __BTK_RECENT_CHOOSER_WIDGET_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkrecentchooser.h>
#include <btk/btkvbox.h>

B_BEGIN_DECLS

#define BTK_TYPE_RECENT_CHOOSER_WIDGET		  (btk_recent_chooser_widget_get_type ())
#define BTK_RECENT_CHOOSER_WIDGET(obj)		  (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_RECENT_CHOOSER_WIDGET, BtkRecentChooserWidget))
#define BTK_IS_RECENT_CHOOSER_WIDGET(obj)	  (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_RECENT_CHOOSER_WIDGET))
#define BTK_RECENT_CHOOSER_WIDGET_CLASS(klass)	  (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_RECENT_CHOOSER_WIDGET, BtkRecentChooserWidgetClass))
#define BTK_IS_RECENT_CHOOSER_WIDGET_CLASS(klass) (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_RECENT_CHOOSER_WIDGET))
#define BTK_RECENT_CHOOSER_WIDGET_GET_CLASS(obj)  (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_RECENT_CHOOSER_WIDGET, BtkRecentChooserWidgetClass))

typedef struct _BtkRecentChooserWidget        BtkRecentChooserWidget;
typedef struct _BtkRecentChooserWidgetClass   BtkRecentChooserWidgetClass;

typedef struct _BtkRecentChooserWidgetPrivate BtkRecentChooserWidgetPrivate;

struct _BtkRecentChooserWidget
{
  /*< private >*/
  BtkVBox parent_instance;

  BtkRecentChooserWidgetPrivate *GSEAL (priv);
};

struct _BtkRecentChooserWidgetClass
{
  BtkVBoxClass parent_class;
};

GType      btk_recent_chooser_widget_get_type        (void) B_GNUC_CONST;
BtkWidget *btk_recent_chooser_widget_new             (void);
BtkWidget *btk_recent_chooser_widget_new_for_manager (BtkRecentManager *manager);

B_END_DECLS

#endif /* __BTK_RECENT_CHOOSER_WIDGET_H__ */
