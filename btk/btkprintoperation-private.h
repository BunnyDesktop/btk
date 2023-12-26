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

#ifndef __BTK_PRINT_OPERATION_PRIVATE_H__
#define __BTK_PRINT_OPERATION_PRIVATE_H__

#include "btkprintoperation.h"

B_BEGIN_DECLS

/* Page drawing states */
typedef enum
{
  BTK_PAGE_DRAWING_STATE_READY,
  BTK_PAGE_DRAWING_STATE_DRAWING,
  BTK_PAGE_DRAWING_STATE_DEFERRED_DRAWING
} BtkPageDrawingState;

struct _BtkPrintOperationPrivate
{
  BtkPrintOperationAction action;
  BtkPrintStatus status;
  GError *error;
  gchar *status_string;
  BtkPageSetup *default_page_setup;
  BtkPrintSettings *print_settings;
  gchar *job_name;
  gint nr_of_pages;
  gint nr_of_pages_to_print;
  gint page_position;
  gint current_page;
  BtkUnit unit;
  gchar *export_filename;
  guint use_full_page      : 1;
  guint track_print_status : 1;
  guint show_progress      : 1;
  guint cancelled          : 1;
  guint allow_async        : 1;
  guint is_sync            : 1;
  guint support_selection  : 1;
  guint has_selection      : 1;
  guint embed_page_setup   : 1;

  BtkPageDrawingState      page_drawing_state;

  guint print_pages_idle_id;
  guint show_progress_timeout_id;

  BtkPrintContext *print_context;
  
  BtkPrintPages print_pages;
  BtkPageRange *page_ranges;
  gint num_page_ranges;
  
  gint manual_num_copies;
  guint manual_collation   : 1;
  guint manual_reverse     : 1;
  guint manual_orientation : 1;
  double manual_scale;
  BtkPageSet manual_page_set;
  guint manual_number_up;
  BtkNumberUpLayout manual_number_up_layout;

  BtkWidget *custom_widget;
  gchar *custom_tab_label;
  
  gpointer platform_data;
  GDestroyNotify free_platform_data;

  GMainLoop *rloop; /* recursive mainloop */

  void (*start_page) (BtkPrintOperation *operation,
		      BtkPrintContext   *print_context,
		      BtkPageSetup      *page_setup);
  void (*end_page)   (BtkPrintOperation *operation,
		      BtkPrintContext   *print_context);
  void (*end_run)    (BtkPrintOperation *operation,
		      gboolean           wait,
		      gboolean           cancelled);
};


typedef void (* BtkPrintOperationPrintFunc) (BtkPrintOperation      *op,
					     BtkWindow              *parent,
					     gboolean                do_print,
					     BtkPrintOperationResult result);

BtkPrintOperationResult _btk_print_operation_platform_backend_run_dialog             (BtkPrintOperation           *operation,
										      gboolean                     show_dialog,
										      BtkWindow                   *parent,
										      gboolean                    *do_print);
void                    _btk_print_operation_platform_backend_run_dialog_async       (BtkPrintOperation           *op,
										      gboolean                     show_dialog,
										      BtkWindow                   *parent,
										      BtkPrintOperationPrintFunc   print_cb);
void                    _btk_print_operation_platform_backend_launch_preview         (BtkPrintOperation           *op,
										      bairo_surface_t             *surface,
										      BtkWindow                   *parent,
										      const char                  *filename);
bairo_surface_t *       _btk_print_operation_platform_backend_create_preview_surface (BtkPrintOperation           *op,
										      BtkPageSetup                *page_setup,
										      gdouble                     *dpi_x,
										      gdouble                     *dpi_y,
										      gchar                       **target);
void                    _btk_print_operation_platform_backend_resize_preview_surface (BtkPrintOperation           *op,
										      BtkPageSetup                *page_setup,
										      bairo_surface_t             *surface);
void                    _btk_print_operation_platform_backend_preview_start_page     (BtkPrintOperation *op,
										      bairo_surface_t *surface,
										      bairo_t *cr);
void                    _btk_print_operation_platform_backend_preview_end_page       (BtkPrintOperation *op,
										      bairo_surface_t *surface,
										      bairo_t *cr);

void _btk_print_operation_set_status (BtkPrintOperation *op,
				      BtkPrintStatus     status,
				      const gchar       *string);

/* BtkPrintContext private functions: */

BtkPrintContext *_btk_print_context_new                             (BtkPrintOperation *op);
void             _btk_print_context_set_page_setup                  (BtkPrintContext   *context,
								     BtkPageSetup      *page_setup);
void             _btk_print_context_translate_into_margin           (BtkPrintContext   *context);
void             _btk_print_context_rotate_according_to_orientation (BtkPrintContext   *context);
void             _btk_print_context_set_hard_margins                (BtkPrintContext   *context,
								     gdouble            top,
								     gdouble            bottom,
								     gdouble            left,
								     gdouble            right);

B_END_DECLS

#endif /* __BTK_PRINT_OPERATION_PRIVATE_H__ */
