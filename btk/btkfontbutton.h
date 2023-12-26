/* BTK - The GIMP Toolkit
 * Copyright (C) 1998 David Abilleira Freijeiro <odaf@nbexo.es>
 * All rights reserved
 * Based on bunny-color-picker by Federico Mena <federico@nuclecu.unam.mx>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Cambridge, MA 02139, USA.
 */
/*
 * Modified by the BTK+ Team and others 2003.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/.
 */

#ifndef __BTK_FONT_BUTTON_H__
#define __BTK_FONT_BUTTON_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkbutton.h>


B_BEGIN_DECLS

/* BtkFontButton is a button widget that allow user to select a font.
 */

#define BTK_TYPE_FONT_BUTTON             (btk_font_button_get_type ())
#define BTK_FONT_BUTTON(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_FONT_BUTTON, BtkFontButton))
#define BTK_FONT_BUTTON_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_FONT_BUTTON, BtkFontButtonClass))
#define BTK_IS_FONT_BUTTON(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_FONT_BUTTON))
#define BTK_IS_FONT_BUTTON_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_FONT_BUTTON))
#define BTK_FONT_BUTTON_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_FONT_BUTTON, BtkFontButtonClass))

typedef struct _BtkFontButton        BtkFontButton;
typedef struct _BtkFontButtonClass   BtkFontButtonClass;
typedef struct _BtkFontButtonPrivate BtkFontButtonPrivate;

struct _BtkFontButton {
  BtkButton button;

  /*< private >*/
  BtkFontButtonPrivate *GSEAL (priv);
};

struct _BtkFontButtonClass {
  BtkButtonClass parent_class;

  /* font_set signal is emitted when font is chosen */
  void (* font_set) (BtkFontButton *gfp);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};


GType                 btk_font_button_get_type       (void) B_GNUC_CONST;
BtkWidget            *btk_font_button_new            (void);
BtkWidget            *btk_font_button_new_with_font  (const gchar   *fontname);

const gchar *         btk_font_button_get_title      (BtkFontButton *font_button);
void                  btk_font_button_set_title      (BtkFontButton *font_button,
                                                      const gchar   *title);
gboolean              btk_font_button_get_use_font   (BtkFontButton *font_button);
void                  btk_font_button_set_use_font   (BtkFontButton *font_button,
                                                      gboolean       use_font);
gboolean              btk_font_button_get_use_size   (BtkFontButton *font_button);
void                  btk_font_button_set_use_size   (BtkFontButton *font_button,
                                                      gboolean       use_size);
const gchar *         btk_font_button_get_font_name  (BtkFontButton *font_button);
gboolean              btk_font_button_set_font_name  (BtkFontButton *font_button,
                                                      const gchar   *fontname);
gboolean              btk_font_button_get_show_style (BtkFontButton *font_button);
void                  btk_font_button_set_show_style (BtkFontButton *font_button,
                                                      gboolean       show_style);
gboolean              btk_font_button_get_show_size  (BtkFontButton *font_button);
void                  btk_font_button_set_show_size  (BtkFontButton *font_button,
                                                      gboolean       show_size);

B_END_DECLS


#endif /* __BTK_FONT_BUTTON_H__ */
