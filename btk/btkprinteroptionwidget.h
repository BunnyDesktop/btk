/* BtkPrinterOptionWidget 
 * Copyright (C) 2006 John (J5) Palmieri <johnp@redhat.com>
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
#ifndef __BTK_PRINTER_OPTION_WIDGET_H__
#define __BTK_PRINTER_OPTION_WIDGET_H__

#include "btkprinteroption.h"
#include "btkhbox.h"

B_BEGIN_DECLS

#define BTK_TYPE_PRINTER_OPTION_WIDGET                  (btk_printer_option_widget_get_type ())
#define BTK_PRINTER_OPTION_WIDGET(obj)                  (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_PRINTER_OPTION_WIDGET, BtkPrinterOptionWidget))
#define BTK_PRINTER_OPTION_WIDGET_CLASS(klass)          (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_PRINTER_OPTION_WIDGET, BtkPrinterOptionWidgetClass))
#define BTK_IS_PRINTER_OPTION_WIDGET(obj)               (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_PRINTER_OPTION_WIDGET))
#define BTK_IS_PRINTER_OPTION_WIDGET_CLASS(klass)       (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_PRINTER_OPTION_WIDGET))
#define BTK_PRINTER_OPTION_WIDGET_GET_CLASS(obj)        (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_PRINTER_OPTION_WIDGET, BtkPrinterOptionWidgetClass))


typedef struct _BtkPrinterOptionWidget         BtkPrinterOptionWidget;
typedef struct _BtkPrinterOptionWidgetClass    BtkPrinterOptionWidgetClass;
typedef struct BtkPrinterOptionWidgetPrivate   BtkPrinterOptionWidgetPrivate;

struct _BtkPrinterOptionWidget
{
  BtkHBox parent_instance;

  BtkPrinterOptionWidgetPrivate *priv;
};

struct _BtkPrinterOptionWidgetClass
{
  BtkHBoxClass parent_class;

  void (*changed) (BtkPrinterOptionWidget *widget);
};

GType	     btk_printer_option_widget_get_type           (void) B_GNUC_CONST;

BtkWidget   *btk_printer_option_widget_new                (BtkPrinterOption       *source);
void         btk_printer_option_widget_set_source         (BtkPrinterOptionWidget *setting,
		 					   BtkPrinterOption       *source);
bboolean     btk_printer_option_widget_has_external_label (BtkPrinterOptionWidget *setting);
BtkWidget   *btk_printer_option_widget_get_external_label (BtkPrinterOptionWidget *setting);
const bchar *btk_printer_option_widget_get_value          (BtkPrinterOptionWidget *setting);

B_END_DECLS

#endif /* __BTK_PRINTER_OPTION_WIDGET_H__ */
