/* BtkPrintJob
 * Copyright (C) 2006 Red Hat,Inc.
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

#ifndef __BTK_PRINT_JOB_H__
#define __BTK_PRINT_JOB_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_UNIX_PRINT_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btkunixprint.h> can be included directly."
#endif

#include <bairo.h>

#include <btk/btk.h>
#include <btk/btkprinter.h>

B_BEGIN_DECLS

#define BTK_TYPE_PRINT_JOB                  (btk_print_job_get_type ())
#define BTK_PRINT_JOB(obj)                  (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_PRINT_JOB, BtkPrintJob))
#define BTK_PRINT_JOB_CLASS(klass)          (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_PRINT_JOB, BtkPrintJobClass))
#define BTK_IS_PRINT_JOB(obj)               (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_PRINT_JOB))
#define BTK_IS_PRINT_JOB_CLASS(klass)       (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_PRINT_JOB))
#define BTK_PRINT_JOB_GET_CLASS(obj)        (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_PRINT_JOB, BtkPrintJobClass))

typedef struct _BtkPrintJob          BtkPrintJob;
typedef struct _BtkPrintJobClass     BtkPrintJobClass;
typedef struct _BtkPrintJobPrivate   BtkPrintJobPrivate;

typedef void (*BtkPrintJobCompleteFunc) (BtkPrintJob *print_job,
                                         gpointer     user_data,
                                         GError      *error);

struct _BtkPrinter;

struct _BtkPrintJob
{
  BObject parent_instance;

  BtkPrintJobPrivate *GSEAL (priv);

  /* Settings the client has to implement:
   * (These are read-only, set at initialization)
   */
  BtkPrintPages GSEAL (print_pages);
  BtkPageRange *GSEAL (page_ranges);
  gint GSEAL (num_page_ranges);
  BtkPageSet GSEAL (page_set);
  gint GSEAL (num_copies);
  gdouble GSEAL (scale);
  guint GSEAL (rotate_to_orientation) : 1;
  guint GSEAL (collate)               : 1;
  guint GSEAL (reverse)               : 1;
  guint GSEAL (number_up);
  BtkNumberUpLayout GSEAL (number_up_layout);
};

struct _BtkPrintJobClass
{
  BObjectClass parent_class;

  void (*status_changed) (BtkPrintJob *job);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
  void (*_btk_reserved5) (void);
  void (*_btk_reserved6) (void);
  void (*_btk_reserved7) (void);
};

GType                    btk_print_job_get_type               (void) B_GNUC_CONST;
BtkPrintJob             *btk_print_job_new                    (const gchar              *title,
							       BtkPrinter               *printer,
							       BtkPrintSettings         *settings,
							       BtkPageSetup             *page_setup);
BtkPrintSettings        *btk_print_job_get_settings           (BtkPrintJob              *job);
BtkPrinter              *btk_print_job_get_printer            (BtkPrintJob              *job);
const gchar *            btk_print_job_get_title              (BtkPrintJob              *job);
BtkPrintStatus           btk_print_job_get_status             (BtkPrintJob              *job);
gboolean                 btk_print_job_set_source_file        (BtkPrintJob              *job,
							       const gchar              *filename,
							       GError                  **error);
bairo_surface_t         *btk_print_job_get_surface            (BtkPrintJob              *job,
							       GError                  **error);
void                     btk_print_job_set_track_print_status (BtkPrintJob              *job,
							       gboolean                  track_status);
gboolean                 btk_print_job_get_track_print_status (BtkPrintJob              *job);
void                     btk_print_job_send                   (BtkPrintJob              *job,
							       BtkPrintJobCompleteFunc   callback,
							       gpointer                  user_data,
							       GDestroyNotify            dnotify);

B_END_DECLS

#endif /* __BTK_PRINT_JOB_H__ */
