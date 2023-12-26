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
 * Modified by the BTK+ Team and others 1997-2001.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/.
 */

#ifndef __BTK_BUTTON_H__
#define __BTK_BUTTON_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkbin.h>
#include <btk/btkimage.h>


G_BEGIN_DECLS

#define BTK_TYPE_BUTTON                 (btk_button_get_type ())
#define BTK_BUTTON(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_BUTTON, BtkButton))
#define BTK_BUTTON_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_BUTTON, BtkButtonClass))
#define BTK_IS_BUTTON(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_BUTTON))
#define BTK_IS_BUTTON_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_BUTTON))
#define BTK_BUTTON_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_BUTTON, BtkButtonClass))

typedef struct _BtkButton        BtkButton;
typedef struct _BtkButtonClass   BtkButtonClass;

struct _BtkButton
{
  BtkBin bin;

  BdkWindow *GSEAL (event_window);

  gchar *GSEAL (label_text);

  guint GSEAL (activate_timeout);

  guint GSEAL (constructed) : 1;
  guint GSEAL (in_button) : 1;
  guint GSEAL (button_down) : 1;
  guint GSEAL (relief) : 2;
  guint GSEAL (use_underline) : 1;
  guint GSEAL (use_stock) : 1;
  guint GSEAL (depressed) : 1;
  guint GSEAL (depress_on_activate) : 1;
  guint GSEAL (focus_on_click) : 1;
};

struct _BtkButtonClass
{
  BtkBinClass        parent_class;
  
  void (* pressed)  (BtkButton *button);
  void (* released) (BtkButton *button);
  void (* clicked)  (BtkButton *button);
  void (* enter)    (BtkButton *button);
  void (* leave)    (BtkButton *button);
  void (* activate) (BtkButton *button);
  
  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};


GType          btk_button_get_type          (void) G_GNUC_CONST;
BtkWidget*     btk_button_new               (void);
BtkWidget*     btk_button_new_with_label    (const gchar    *label);
BtkWidget*     btk_button_new_from_stock    (const gchar    *stock_id);
BtkWidget*     btk_button_new_with_mnemonic (const gchar    *label);
#ifndef BTK_DISABLE_DEPRECATED
void           btk_button_pressed           (BtkButton      *button);
void           btk_button_released          (BtkButton      *button);
#endif
void           btk_button_clicked           (BtkButton      *button);
#ifndef BTK_DISABLE_DEPRECATED
void           btk_button_enter             (BtkButton      *button);
void           btk_button_leave             (BtkButton      *button);
#endif

void                  btk_button_set_relief         (BtkButton      *button,
						     BtkReliefStyle  newstyle);
BtkReliefStyle        btk_button_get_relief         (BtkButton      *button);
void                  btk_button_set_label          (BtkButton      *button,
						     const gchar    *label);
const gchar *         btk_button_get_label          (BtkButton      *button);
void                  btk_button_set_use_underline  (BtkButton      *button,
						     gboolean        use_underline);
gboolean              btk_button_get_use_underline  (BtkButton      *button);
void                  btk_button_set_use_stock      (BtkButton      *button,
						     gboolean        use_stock);
gboolean              btk_button_get_use_stock      (BtkButton      *button);
void                  btk_button_set_focus_on_click (BtkButton      *button,
						     gboolean        focus_on_click);
gboolean              btk_button_get_focus_on_click (BtkButton      *button);
void                  btk_button_set_alignment      (BtkButton      *button,
						     gfloat          xalign,
						     gfloat          yalign);
void                  btk_button_get_alignment      (BtkButton      *button,
						     gfloat         *xalign,
						     gfloat         *yalign);
void                  btk_button_set_image          (BtkButton      *button,
					             BtkWidget      *image);
BtkWidget*            btk_button_get_image          (BtkButton      *button);
void                  btk_button_set_image_position (BtkButton      *button,
						     BtkPositionType position);
BtkPositionType       btk_button_get_image_position (BtkButton      *button);

BdkWindow*            btk_button_get_event_window   (BtkButton      *button);

void _btk_button_set_depressed             (BtkButton          *button,
					    gboolean            depressed);
void _btk_button_paint                     (BtkButton          *button,
					    const BdkRectangle *area,
					    BtkStateType        state_type,
					    BtkShadowType       shadow_type,
					    const gchar        *main_detail,
					    const gchar        *default_detail);

G_END_DECLS

#endif /* __BTK_BUTTON_H__ */
