/* BTK+: btkfilechooserbutton.h
 *
 * Copyright (c) 2004 James M. Cape <jcape@ignore-your.tv>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __BTK_FILE_CHOOSER_BUTTON_H__
#define __BTK_FILE_CHOOSER_BUTTON_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkhbox.h>
#include <btk/btkfilechooser.h>

B_BEGIN_DECLS

#define BTK_TYPE_FILE_CHOOSER_BUTTON            (btk_file_chooser_button_get_type ())
#define BTK_FILE_CHOOSER_BUTTON(obj)            (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_FILE_CHOOSER_BUTTON, BtkFileChooserButton))
#define BTK_FILE_CHOOSER_BUTTON_CLASS(klass)    (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_FILE_CHOOSER_BUTTON, BtkFileChooserButtonClass))
#define BTK_IS_FILE_CHOOSER_BUTTON(obj)         (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_FILE_CHOOSER_BUTTON))
#define BTK_IS_FILE_CHOOSER_BUTTON_CLASS(klass) (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_FILE_CHOOSER_BUTTON))
#define BTK_FILE_CHOOSER_BUTTON_GET_CLASS(obj)  (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_FILE_CHOOSER_BUTTON, BtkFileChooserButtonClass))

typedef struct _BtkFileChooserButton        BtkFileChooserButton;
typedef struct _BtkFileChooserButtonPrivate BtkFileChooserButtonPrivate;
typedef struct _BtkFileChooserButtonClass   BtkFileChooserButtonClass;

struct _BtkFileChooserButton
{
  /*< private >*/
  BtkHBox parent;

  BtkFileChooserButtonPrivate *GSEAL (priv);
};

struct _BtkFileChooserButtonClass
{
  /*< private >*/
  BtkHBoxClass parent_class;

  void (* file_set) (BtkFileChooserButton *fc);

  void *__btk_reserved1;
  void *__btk_reserved2;
  void *__btk_reserved3;
  void *__btk_reserved4;
  void *__btk_reserved5;
  void *__btk_reserved6;
  void *__btk_reserved7;
};


GType                 btk_file_chooser_button_get_type         (void) B_GNUC_CONST;
BtkWidget *           btk_file_chooser_button_new              (const bchar          *title,
								BtkFileChooserAction  action);

#ifndef BTK_DISABLE_DEPRECATED
BtkWidget *           btk_file_chooser_button_new_with_backend (const bchar          *title,
								BtkFileChooserAction  action,
								const bchar          *backend);
#endif /* BTK_DISABLE_DEPRECATED */

BtkWidget *           btk_file_chooser_button_new_with_dialog  (BtkWidget            *dialog);
const bchar *         btk_file_chooser_button_get_title        (BtkFileChooserButton *button);
void                  btk_file_chooser_button_set_title        (BtkFileChooserButton *button,
								const bchar          *title);
bint                  btk_file_chooser_button_get_width_chars  (BtkFileChooserButton *button);
void                  btk_file_chooser_button_set_width_chars  (BtkFileChooserButton *button,
								bint                  n_chars);
bboolean              btk_file_chooser_button_get_focus_on_click (BtkFileChooserButton *button);
void                  btk_file_chooser_button_set_focus_on_click (BtkFileChooserButton *button,
                                                                  bboolean              focus_on_click);

B_END_DECLS

#endif /* !__BTK_FILE_CHOOSER_BUTTON_H__ */
