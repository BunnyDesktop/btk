/*
 * BTK - The GIMP Toolkit
 * Copyright (C) 1998, 1999 Red Hat, Inc.
 * All rights reserved.
 *
 * This Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* Color picker button for GNOME
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 *
 * Modified by the BTK+ Team and others 2003.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/.
 */

#ifndef __BTK_COLOR_BUTTON_H__
#define __BTK_COLOR_BUTTON_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkbutton.h>

G_BEGIN_DECLS


/* The BtkColorButton widget is a simple color picker in a button.
 * The button displays a sample of the currently selected color.  When
 * the user clicks on the button, a color selection dialog pops up.
 * The color picker emits the "color_set" signal when the color is set.
 */

#define BTK_TYPE_COLOR_BUTTON             (btk_color_button_get_type ())
#define BTK_COLOR_BUTTON(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_COLOR_BUTTON, BtkColorButton))
#define BTK_COLOR_BUTTON_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_COLOR_BUTTON, BtkColorButtonClass))
#define BTK_IS_COLOR_BUTTON(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_COLOR_BUTTON))
#define BTK_IS_COLOR_BUTTON_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_COLOR_BUTTON))
#define BTK_COLOR_BUTTON_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_COLOR_BUTTON, BtkColorButtonClass))

typedef struct _BtkColorButton          BtkColorButton;
typedef struct _BtkColorButtonClass     BtkColorButtonClass;
typedef struct _BtkColorButtonPrivate   BtkColorButtonPrivate;

struct _BtkColorButton {
  BtkButton button;

  /*< private >*/

  BtkColorButtonPrivate *GSEAL (priv);
};

struct _BtkColorButtonClass {
  BtkButtonClass parent_class;

  void (* color_set) (BtkColorButton *cp);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};


GType      btk_color_button_get_type       (void) G_GNUC_CONST;
BtkWidget *btk_color_button_new            (void);
BtkWidget *btk_color_button_new_with_color (const BdkColor *color);
void       btk_color_button_set_color      (BtkColorButton *color_button,
					    const BdkColor *color);
void       btk_color_button_set_alpha      (BtkColorButton *color_button,
					    guint16         alpha);
void       btk_color_button_get_color      (BtkColorButton *color_button,
					    BdkColor       *color);
guint16    btk_color_button_get_alpha      (BtkColorButton *color_button);
void       btk_color_button_set_use_alpha  (BtkColorButton *color_button,
					    gboolean        use_alpha);
gboolean   btk_color_button_get_use_alpha  (BtkColorButton *color_button);
void       btk_color_button_set_title      (BtkColorButton *color_button,
					    const gchar    *title);
const gchar *btk_color_button_get_title (BtkColorButton *color_button);


G_END_DECLS

#endif  /* __BTK_COLOR_BUTTON_H__ */
