/* BTK - The GIMP Toolkit
 * btkprinteroption.h: printer option
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

#ifndef __BTK_PRINTER_OPTION_H__
#define __BTK_PRINTER_OPTION_H__

/* This is a "semi-private" header; it is meant only for
 * alternate BtkPrintDialog backend modules; no stability guarantees 
 * are made at this point
 */
#ifndef BTK_PRINT_BACKEND_ENABLE_UNSUPPORTED
#error "BtkPrintBackend is not supported API for general use"
#endif

#include <bunnylib-object.h>

B_BEGIN_DECLS

#define BTK_TYPE_PRINTER_OPTION             (btk_printer_option_get_type ())
#define BTK_PRINTER_OPTION(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_PRINTER_OPTION, BtkPrinterOption))
#define BTK_IS_PRINTER_OPTION(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_PRINTER_OPTION))

typedef struct _BtkPrinterOption       BtkPrinterOption;
typedef struct _BtkPrinterOptionClass  BtkPrinterOptionClass;

#define BTK_PRINTER_OPTION_GROUP_IMAGE_QUALITY "ImageQuality"
#define BTK_PRINTER_OPTION_GROUP_FINISHING "Finishing"

typedef enum {
  BTK_PRINTER_OPTION_TYPE_BOOLEAN,
  BTK_PRINTER_OPTION_TYPE_PICKONE,
  BTK_PRINTER_OPTION_TYPE_PICKONE_PASSWORD,
  BTK_PRINTER_OPTION_TYPE_PICKONE_PASSCODE,
  BTK_PRINTER_OPTION_TYPE_PICKONE_REAL,
  BTK_PRINTER_OPTION_TYPE_PICKONE_INT,
  BTK_PRINTER_OPTION_TYPE_PICKONE_STRING,
  BTK_PRINTER_OPTION_TYPE_ALTERNATIVE,
  BTK_PRINTER_OPTION_TYPE_STRING,
  BTK_PRINTER_OPTION_TYPE_FILESAVE
} BtkPrinterOptionType;

struct _BtkPrinterOption
{
  GObject parent_instance;

  char *name;
  char *display_text;
  BtkPrinterOptionType type;

  char *value;
  
  int num_choices;
  char **choices;
  char **choices_display;
  
  gboolean activates_default;

  gboolean has_conflict;
  char *group;
};

struct _BtkPrinterOptionClass
{
  GObjectClass parent_class;

  void (*changed) (BtkPrinterOption *option);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
  void (*_btk_reserved5) (void);
  void (*_btk_reserved6) (void);
  void (*_btk_reserved7) (void);
};

GType   btk_printer_option_get_type       (void) B_GNUC_CONST;

BtkPrinterOption *btk_printer_option_new                    (const char           *name,
							     const char           *display_text,
							     BtkPrinterOptionType  type);
void              btk_printer_option_set                    (BtkPrinterOption     *option,
							     const char           *value);
void              btk_printer_option_set_has_conflict       (BtkPrinterOption     *option,
							     gboolean              has_conflict);
void              btk_printer_option_clear_has_conflict     (BtkPrinterOption     *option);
void              btk_printer_option_set_boolean            (BtkPrinterOption     *option,
							     gboolean              value);
void              btk_printer_option_allocate_choices       (BtkPrinterOption     *option,
							     int                   num);
void              btk_printer_option_choices_from_array     (BtkPrinterOption     *option,
							     int                   num_choices,
							     char                 *choices[],
							     char                 *choices_display[]);
gboolean          btk_printer_option_has_choice             (BtkPrinterOption     *option,
							    const char           *choice);
void              btk_printer_option_set_activates_default (BtkPrinterOption     *option,
							    gboolean              activates);
gboolean          btk_printer_option_get_activates_default (BtkPrinterOption     *option);


B_END_DECLS

#endif /* __BTK_PRINTER_OPTION_H__ */


