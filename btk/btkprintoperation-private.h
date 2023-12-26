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
  bchar *status_string;
  BtkPageSetup *default_page_setup;
  BtkPrintSettings *print_settings;
  bchar *job_name;
  bint nr_of_pages;
  bint nr_of_pages_to_print;
  bint page_position;
  bint current_page;
  BtkUnit unit;
  bchar *export_filename;
  buint use_full_page      : 1;
  buint track_print_status : 1;
  buint show_progress      : 1;
  buint cancelled          : 1;
  buint allow_async        : 1;
  buint is_sync            : 1;
  buint support_selection  : 1;
  buint has_selection      : 1;
  buint embed_page_setup   : 1;

  BtkPageDrawingState      page_drawing_state;

  buint print_pages_idle_id;
  buint show_progress_timeout_id;

  BtkPrintContext *print_context;
  
  BtkPrintPages print_pages;
  BtkPageRange *page_ranges;
  bint num_page_ranges;
  
  bint manual_num_copies;
  buint manual_collation   : 1;
  buint manual_reverse     : 1;
  buint manual_orientation : 1;
  double manual_scale;
  BtkPageSet manual_page_set;
  buint manual_number_up;
  BtkNumberUpLayout manual_number_up_layout;

  BtkWidget *custom_widget;
  bchar *custom_tab_label;
  
  bpointer platform_data;
  GDestroyNotify free_platform_data;

  GMainLoop *rloop; /* recursive mainloop */

  void (*start_page) (BtkPrintOperation *operation,
		      BtkPrintContext   *print_context,
		      BtkPageSetup      *page_setup);
  void (*end_page)   (BtkPrintOperation *operation,
		      BtkPrintContext   *print_context);
  void (*end_run)    (BtkPrintOperation *operation,
		      bboolean           wait,
		      bboolean           cancelled);
};


typedef void (* BtkPrintOperationPrintFunc) (BtkPrintOperation      *op,
					     BtkWindow              *parent,
					     bboolean                do_print,
					     BtkPrintOperationResult result);

BtkPrintOperationResult _btk_print_operation_platform_backend_run_dialog             (BtkPrintOperation           *operation,
										      bboolean                     show_dialog,
										      BtkWindow                   *parent,
										      bboolean                    *do_print);
void                    _btk_print_operation_platform_backend_run_dialog_async       (BtkPrintOperation           *op,
										      bboolean                     show_dialog,
										      BtkWindow                   *parent,
										      BtkPrintOperationPrintFunc   print_cb);
void                    _btk_print_operation_platform_backend_launch_preview         (BtkPrintOperation           *op,
										      bairo_surface_t             *surface,
										      BtkWindow                   *parent,
										      const char                  *filename);
bairo_surface_t *       _btk_print_operation_platform_backend_create_preview_surface (BtkPrintOperation           *op,
										      BtkPageSetup                *page_setup,
										      bdouble                     *dpi_x,
										      bdouble                     *dpi_y,
										      bchar                       **target);
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
				      const bchar       *string);

/* BtkPrintContext private functions: */

BtkPrintContext *_btk_print_context_new                             (BtkPrintOperation *op);
void             _btk_print_context_set_page_setup                  (BtkPrintContext   *context,
								     BtkPageSetup      *page_setup);
void             _btk_print_context_translate_into_margin           (BtkPrintContext   *context);
void             _btk_print_context_rotate_according_to_orientation (BtkPrintContext   *context);
void             _btk_print_context_set_hard_margins                (BtkPrintContext   *context,
								     bdouble            top,
								     bdouble            bottom,
								     bdouble            left,
								     bdouble            right);

B_END_DECLS

#endif /* __BTK_PRINT_OPERATION_PRIVATE_H__ */
