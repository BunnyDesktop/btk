/* BTK - The GIMP Toolkit
 * btkprinteroptionset.h: printer option set
 * Copyright (C) 2006, Red Hat, Inc.
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

#ifndef __BTK_PRINTER_OPTION_SET_H__
#define __BTK_PRINTER_OPTION_SET_H__

/* This is a "semi-private" header; it is meant only for
 * alternate BtkPrintDialog backend modules; no stability guarantees
 * are made at this point
 */
#ifndef BTK_PRINT_BACKEND_ENABLE_UNSUPPORTED
#error "BtkPrintBackend is not supported API for general use"
#endif

#include <bunnylib-object.h>
#include "btkprinteroption.h"

B_BEGIN_DECLS

#define BTK_TYPE_PRINTER_OPTION_SET             (btk_printer_option_set_get_type ())
#define BTK_PRINTER_OPTION_SET(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_PRINTER_OPTION_SET, BtkPrinterOptionSet))
#define BTK_IS_PRINTER_OPTION_SET(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_PRINTER_OPTION_SET))

typedef struct _BtkPrinterOptionSet       BtkPrinterOptionSet;
typedef struct _BtkPrinterOptionSetClass  BtkPrinterOptionSetClass;

struct _BtkPrinterOptionSet
{
  GObject parent_instance;

  /*< private >*/
  GPtrArray *array;
  GHashTable *hash;
};

struct _BtkPrinterOptionSetClass
{
  GObjectClass parent_class;

  void (*changed) (BtkPrinterOptionSet *option);


  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
  void (*_btk_reserved5) (void);
  void (*_btk_reserved6) (void);
  void (*_btk_reserved7) (void);
};

typedef void (*BtkPrinterOptionSetFunc) (BtkPrinterOption  *option,
					 gpointer           user_data);


GType   btk_printer_option_set_get_type       (void) B_GNUC_CONST;

BtkPrinterOptionSet *btk_printer_option_set_new              (void);
void                 btk_printer_option_set_add              (BtkPrinterOptionSet     *set,
							      BtkPrinterOption        *option);
void                 btk_printer_option_set_remove           (BtkPrinterOptionSet     *set,
							      BtkPrinterOption        *option);
BtkPrinterOption *   btk_printer_option_set_lookup           (BtkPrinterOptionSet     *set,
							      const char              *name);
void                 btk_printer_option_set_foreach          (BtkPrinterOptionSet     *set,
							      BtkPrinterOptionSetFunc  func,
							      gpointer                 user_data);
void                 btk_printer_option_set_clear_conflicts  (BtkPrinterOptionSet     *set);
GList *              btk_printer_option_set_get_groups       (BtkPrinterOptionSet     *set);
void                 btk_printer_option_set_foreach_in_group (BtkPrinterOptionSet     *set,
							      const char              *group,
							      BtkPrinterOptionSetFunc  func,
							      gpointer                 user_data);

B_END_DECLS

#endif /* __BTK_PRINTER_OPTION_SET_H__ */
