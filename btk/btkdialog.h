/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/.
 */

#ifndef __BTK_DIALOG_H__
#define __BTK_DIALOG_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkwindow.h>


B_BEGIN_DECLS

/* Parameters for dialog construction */
typedef enum
{
  BTK_DIALOG_MODAL               = 1 << 0, /* call btk_window_set_modal (win, TRUE) */
  BTK_DIALOG_DESTROY_WITH_PARENT = 1 << 1, /* call btk_window_set_destroy_with_parent () */
  BTK_DIALOG_NO_SEPARATOR        = 1 << 2  /* no separator bar above buttons */
} BtkDialogFlags;

/* Convenience enum to use for response_id's.  Positive values are
 * totally user-interpreted. BTK will sometimes return
 * BTK_RESPONSE_NONE if no response_id is available.
 *
 *  Typical usage is:
 *     if (btk_dialog_run(dialog) == BTK_RESPONSE_ACCEPT)
 *       blah();
 */
typedef enum
{
  /* BTK returns this if a response widget has no response_id,
   * or if the dialog gets programmatically hidden or destroyed.
   */
  BTK_RESPONSE_NONE = -1,

  /* BTK won't return these unless you pass them in
   * as the response for an action widget. They are
   * for your convenience.
   */
  BTK_RESPONSE_REJECT = -2,
  BTK_RESPONSE_ACCEPT = -3,

  /* If the dialog is deleted. */
  BTK_RESPONSE_DELETE_EVENT = -4,

  /* These are returned from BTK dialogs, and you can also use them
   * yourself if you like.
   */
  BTK_RESPONSE_OK     = -5,
  BTK_RESPONSE_CANCEL = -6,
  BTK_RESPONSE_CLOSE  = -7,
  BTK_RESPONSE_YES    = -8,
  BTK_RESPONSE_NO     = -9,
  BTK_RESPONSE_APPLY  = -10,
  BTK_RESPONSE_HELP   = -11
} BtkResponseType;


#define BTK_TYPE_DIALOG                  (btk_dialog_get_type ())
#define BTK_DIALOG(obj)                  (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_DIALOG, BtkDialog))
#define BTK_DIALOG_CLASS(klass)          (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_DIALOG, BtkDialogClass))
#define BTK_IS_DIALOG(obj)               (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_DIALOG))
#define BTK_IS_DIALOG_CLASS(klass)       (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_DIALOG))
#define BTK_DIALOG_GET_CLASS(obj)        (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_DIALOG, BtkDialogClass))


typedef struct _BtkDialog        BtkDialog;
typedef struct _BtkDialogClass   BtkDialogClass;

struct _BtkDialog
{
  BtkWindow window;

  /*< public >*/
  BtkWidget *GSEAL (vbox);
  BtkWidget *GSEAL (action_area);

  /*< private >*/
  BtkWidget *GSEAL (separator);
};

struct _BtkDialogClass
{
  BtkWindowClass parent_class;

  void (* response) (BtkDialog *dialog, bint response_id);

  /* Keybinding signals */

  void (* close)    (BtkDialog *dialog);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};


GType      btk_dialog_get_type (void) B_GNUC_CONST;
BtkWidget* btk_dialog_new      (void);

BtkWidget* btk_dialog_new_with_buttons (const bchar     *title,
                                        BtkWindow       *parent,
                                        BtkDialogFlags   flags,
                                        const bchar     *first_button_text,
                                        ...);

void       btk_dialog_add_action_widget (BtkDialog   *dialog,
                                         BtkWidget   *child,
                                         bint         response_id);
BtkWidget* btk_dialog_add_button        (BtkDialog   *dialog,
                                         const bchar *button_text,
                                         bint         response_id);
void       btk_dialog_add_buttons       (BtkDialog   *dialog,
                                         const bchar *first_button_text,
                                         ...) B_GNUC_NULL_TERMINATED;

void btk_dialog_set_response_sensitive (BtkDialog *dialog,
                                        bint       response_id,
                                        bboolean   setting);
void btk_dialog_set_default_response   (BtkDialog *dialog,
                                        bint       response_id);
BtkWidget* btk_dialog_get_widget_for_response (BtkDialog *dialog,
                                               bint       response_id);
bint btk_dialog_get_response_for_widget (BtkDialog *dialog,
					 BtkWidget *widget);

#if !defined (BTK_DISABLE_DEPRECATED) || defined (BTK_COMPILATION)
void     btk_dialog_set_has_separator (BtkDialog *dialog,
                                       bboolean   setting);
bboolean btk_dialog_get_has_separator (BtkDialog *dialog);
#endif

bboolean btk_alternative_dialog_button_order (BdkScreen *screen);
void     btk_dialog_set_alternative_button_order (BtkDialog *dialog,
						  bint       first_response_id,
						  ...);
void     btk_dialog_set_alternative_button_order_from_array (BtkDialog *dialog,
                                                             bint       n_params,
                                                             bint      *new_order);

/* Emit response signal */
void btk_dialog_response           (BtkDialog *dialog,
                                    bint       response_id);

/* Returns response_id */
bint btk_dialog_run                (BtkDialog *dialog);

BtkWidget * btk_dialog_get_action_area  (BtkDialog *dialog);
BtkWidget * btk_dialog_get_content_area (BtkDialog *dialog);

/* For private use only */
void _btk_dialog_set_ignore_separator (BtkDialog *dialog,
				       bboolean   ignore_separator);

B_END_DECLS

#endif /* __BTK_DIALOG_H__ */
