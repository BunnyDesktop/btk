/* BTK - The GIMP Toolkit
 * Copyright (C) 2005 Ronald S. Bultje
 * Copyright (C) 2006, 2007 Christian Persch
 * Copyright (C) 2006 Jan Arne Petersen
 * Copyright (C) 2007 Red Hat, Inc.
 *
 * Authors:
 * - Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * - Bastien Nocera <bnocera@redhat.com>
 * - Jan Arne Petersen <jpetersen@jpetersen.org>
 * - Christian Persch <chpe@svn.bunny.org>
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
 * Modified by the BTK+ Team and others 2007.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/.
 */

#ifndef __BTK_SCALE_BUTTON_H__
#define __BTK_SCALE_BUTTON_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkbutton.h>

B_BEGIN_DECLS

#define BTK_TYPE_SCALE_BUTTON                 (btk_scale_button_get_type ())
#define BTK_SCALE_BUTTON(obj)                 (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_SCALE_BUTTON, BtkScaleButton))
#define BTK_SCALE_BUTTON_CLASS(klass)         (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_SCALE_BUTTON, BtkScaleButtonClass))
#define BTK_IS_SCALE_BUTTON(obj)              (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_SCALE_BUTTON))
#define BTK_IS_SCALE_BUTTON_CLASS(klass)      (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_SCALE_BUTTON))
#define BTK_SCALE_BUTTON_GET_CLASS(obj)       (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_SCALE_BUTTON, BtkScaleButtonClass))

typedef struct _BtkScaleButton        BtkScaleButton;
typedef struct _BtkScaleButtonClass   BtkScaleButtonClass;
typedef struct _BtkScaleButtonPrivate BtkScaleButtonPrivate;

struct _BtkScaleButton
{
  BtkButton parent;

  BtkWidget *GSEAL (plus_button);
  BtkWidget *GSEAL (minus_button);

  /*< private >*/
  BtkScaleButtonPrivate *GSEAL (priv);
};

struct _BtkScaleButtonClass
{
  BtkButtonClass parent_class;

  /* signals */
  void	(* value_changed) (BtkScaleButton *button,
                           gdouble         value);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};

GType            btk_scale_button_get_type         (void) B_GNUC_CONST;
BtkWidget *      btk_scale_button_new              (BtkIconSize      size,
                                                    gdouble          min,
                                                    gdouble          max,
                                                    gdouble          step,
                                                    const gchar    **icons);
void             btk_scale_button_set_icons        (BtkScaleButton  *button,
                                                    const gchar    **icons);
gdouble          btk_scale_button_get_value        (BtkScaleButton  *button);
void             btk_scale_button_set_value        (BtkScaleButton  *button,
                                                    gdouble          value);
BtkAdjustment *  btk_scale_button_get_adjustment   (BtkScaleButton  *button);
void             btk_scale_button_set_adjustment   (BtkScaleButton  *button,
                                                    BtkAdjustment   *adjustment);
BtkWidget *      btk_scale_button_get_plus_button  (BtkScaleButton  *button);
BtkWidget *      btk_scale_button_get_minus_button (BtkScaleButton  *button);
BtkWidget *      btk_scale_button_get_popup        (BtkScaleButton  *button);

#ifndef BTK_DISABLE_DEPRECATED

BtkOrientation   btk_scale_button_get_orientation  (BtkScaleButton  *button);
void             btk_scale_button_set_orientation  (BtkScaleButton  *button,
                                                    BtkOrientation   orientation);

#endif /* BTK_DISABLE_DEPRECATED */

B_END_DECLS

#endif /* __BTK_SCALE_BUTTON_H__ */
