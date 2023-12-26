/* BTK - The GIMP Toolkit
 * btkprintoperation.h: Print Operation
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

#ifndef __BTK_PRINT_OPERATION_H__
#define __BTK_PRINT_OPERATION_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <bairo.h>
#include <btk/btkmain.h>
#include <btk/btkwindow.h>
#include <btk/btkpagesetup.h>
#include <btk/btkprintsettings.h>
#include <btk/btkprintcontext.h>
#include <btk/btkprintoperationpreview.h>


G_BEGIN_DECLS

#define BTK_TYPE_PRINT_OPERATION		(btk_print_operation_get_type ())
#define BTK_PRINT_OPERATION(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_PRINT_OPERATION, BtkPrintOperation))
#define BTK_PRINT_OPERATION_CLASS(klass)    	(G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_PRINT_OPERATION, BtkPrintOperationClass))
#define BTK_IS_PRINT_OPERATION(obj) 		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_PRINT_OPERATION))
#define BTK_IS_PRINT_OPERATION_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_PRINT_OPERATION))
#define BTK_PRINT_OPERATION_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_PRINT_OPERATION, BtkPrintOperationClass))

typedef struct _BtkPrintOperationClass   BtkPrintOperationClass;
typedef struct _BtkPrintOperationPrivate BtkPrintOperationPrivate;
typedef struct _BtkPrintOperation        BtkPrintOperation;

typedef enum {
  BTK_PRINT_STATUS_INITIAL,
  BTK_PRINT_STATUS_PREPARING,
  BTK_PRINT_STATUS_GENERATING_DATA,
  BTK_PRINT_STATUS_SENDING_DATA,
  BTK_PRINT_STATUS_PENDING,
  BTK_PRINT_STATUS_PENDING_ISSUE,
  BTK_PRINT_STATUS_PRINTING,
  BTK_PRINT_STATUS_FINISHED,
  BTK_PRINT_STATUS_FINISHED_ABORTED
} BtkPrintStatus;

typedef enum {
  BTK_PRINT_OPERATION_RESULT_ERROR,
  BTK_PRINT_OPERATION_RESULT_APPLY,
  BTK_PRINT_OPERATION_RESULT_CANCEL,
  BTK_PRINT_OPERATION_RESULT_IN_PROGRESS
} BtkPrintOperationResult;

typedef enum {
  BTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
  BTK_PRINT_OPERATION_ACTION_PRINT,
  BTK_PRINT_OPERATION_ACTION_PREVIEW,
  BTK_PRINT_OPERATION_ACTION_EXPORT
} BtkPrintOperationAction;


struct _BtkPrintOperation
{
  GObject parent_instance;

  BtkPrintOperationPrivate *GSEAL (priv);
};

struct _BtkPrintOperationClass
{
  GObjectClass parent_class;

  void     (*done)               (BtkPrintOperation *operation,
				  BtkPrintOperationResult result);
  void     (*begin_print)        (BtkPrintOperation *operation,
				  BtkPrintContext   *context);
  gboolean (*paginate)           (BtkPrintOperation *operation,
				  BtkPrintContext   *context);
  void     (*request_page_setup) (BtkPrintOperation *operation,
				  BtkPrintContext   *context,
				  gint               page_nr,
				  BtkPageSetup      *setup);
  void     (*draw_page)          (BtkPrintOperation *operation,
				  BtkPrintContext   *context,
				  gint               page_nr);
  void     (*end_print)          (BtkPrintOperation *operation,
				  BtkPrintContext   *context);
  void     (*status_changed)     (BtkPrintOperation *operation);

  BtkWidget *(*create_custom_widget) (BtkPrintOperation *operation);
  void       (*custom_widget_apply)  (BtkPrintOperation *operation,
				      BtkWidget         *widget);

  gboolean (*preview)	     (BtkPrintOperation        *operation,
			      BtkPrintOperationPreview *preview,
			      BtkPrintContext          *context,
			      BtkWindow                *parent);

  void     (*update_custom_widget) (BtkPrintOperation *operation,
                                    BtkWidget         *widget,
                                    BtkPageSetup      *setup,
                                    BtkPrintSettings  *settings);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
  void (*_btk_reserved5) (void);
  void (*_btk_reserved6) (void);
};

#define BTK_PRINT_ERROR btk_print_error_quark ()

typedef enum
{
  BTK_PRINT_ERROR_GENERAL,
  BTK_PRINT_ERROR_INTERNAL_ERROR,
  BTK_PRINT_ERROR_NOMEM,
  BTK_PRINT_ERROR_INVALID_FILE
} BtkPrintError;

GQuark btk_print_error_quark (void);

GType                   btk_print_operation_get_type               (void) G_GNUC_CONST;
BtkPrintOperation *     btk_print_operation_new                    (void);
void                    btk_print_operation_set_default_page_setup (BtkPrintOperation  *op,
								    BtkPageSetup       *default_page_setup);
BtkPageSetup *          btk_print_operation_get_default_page_setup (BtkPrintOperation  *op);
void                    btk_print_operation_set_print_settings     (BtkPrintOperation  *op,
								    BtkPrintSettings   *print_settings);
BtkPrintSettings *      btk_print_operation_get_print_settings     (BtkPrintOperation  *op);
void                    btk_print_operation_set_job_name           (BtkPrintOperation  *op,
								    const gchar        *job_name);
void                    btk_print_operation_set_n_pages            (BtkPrintOperation  *op,
								    gint                n_pages);
void                    btk_print_operation_set_current_page       (BtkPrintOperation  *op,
								    gint                current_page);
void                    btk_print_operation_set_use_full_page      (BtkPrintOperation  *op,
								    gboolean            full_page);
void                    btk_print_operation_set_unit               (BtkPrintOperation  *op,
								    BtkUnit             unit);
void                    btk_print_operation_set_export_filename    (BtkPrintOperation  *op,
								    const gchar        *filename);
void                    btk_print_operation_set_track_print_status (BtkPrintOperation  *op,
								    gboolean            track_status);
void                    btk_print_operation_set_show_progress      (BtkPrintOperation  *op,
								    gboolean            show_progress);
void                    btk_print_operation_set_allow_async        (BtkPrintOperation  *op,
								    gboolean            allow_async);
void                    btk_print_operation_set_custom_tab_label   (BtkPrintOperation  *op,
								    const gchar        *label);
BtkPrintOperationResult btk_print_operation_run                    (BtkPrintOperation  *op,
								    BtkPrintOperationAction action,
								    BtkWindow          *parent,
								    GError            **error);
void                    btk_print_operation_get_error              (BtkPrintOperation  *op,
								    GError            **error);
BtkPrintStatus          btk_print_operation_get_status             (BtkPrintOperation  *op);
const gchar *           btk_print_operation_get_status_string      (BtkPrintOperation  *op);
gboolean                btk_print_operation_is_finished            (BtkPrintOperation  *op);
void                    btk_print_operation_cancel                 (BtkPrintOperation  *op);
void                    btk_print_operation_draw_page_finish       (BtkPrintOperation  *op);
void                    btk_print_operation_set_defer_drawing      (BtkPrintOperation  *op);
void                    btk_print_operation_set_support_selection  (BtkPrintOperation  *op,
                                                                    gboolean            support_selection);
gboolean                btk_print_operation_get_support_selection  (BtkPrintOperation  *op);
void                    btk_print_operation_set_has_selection      (BtkPrintOperation  *op,
                                                                    gboolean            has_selection);
gboolean                btk_print_operation_get_has_selection      (BtkPrintOperation  *op);
void                    btk_print_operation_set_embed_page_setup   (BtkPrintOperation  *op,
 								    gboolean            embed);
gboolean                btk_print_operation_get_embed_page_setup   (BtkPrintOperation  *op);
gint                    btk_print_operation_get_n_pages_to_print   (BtkPrintOperation  *op);

BtkPageSetup           *btk_print_run_page_setup_dialog            (BtkWindow          *parent,
								    BtkPageSetup       *page_setup,
								    BtkPrintSettings   *settings);

typedef void  (* BtkPageSetupDoneFunc) (BtkPageSetup *page_setup,
					gpointer      data);

void                    btk_print_run_page_setup_dialog_async      (BtkWindow            *parent,
								    BtkPageSetup         *page_setup,
								    BtkPrintSettings     *settings,
                                                                    BtkPageSetupDoneFunc  done_cb,
                                                                    gpointer              data);

G_END_DECLS

#endif /* __BTK_PRINT_OPERATION_H__ */
