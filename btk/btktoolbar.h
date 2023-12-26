/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * BtkToolbar copyright (C) Federico Mena
 *
 * Copyright (C) 2002 Anders Carlsson <andersca@bunny.org>
 * Copyright (C) 2002 James Henstridge <james@daa.com.au>
 * Copyright (C) 2003 Soeren Sandmann <sandmann@daimi.au.dk>
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

#ifndef __BTK_TOOLBAR_H__
#define __BTK_TOOLBAR_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkcontainer.h>
#include <btk/btktooltips.h>
#include <btk/btktoolitem.h>

#ifndef BTK_DISABLE_DEPRECATED

/* Not needed, retained for compatibility -Yosh */
#include <btk/btkpixmap.h>
#include <btk/btksignal.h>

#endif /* BTK_DISABLE_DEPRECATED */

B_BEGIN_DECLS

#define BTK_TYPE_TOOLBAR            (btk_toolbar_get_type ())
#define BTK_TOOLBAR(obj)            (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_TOOLBAR, BtkToolbar))
#define BTK_TOOLBAR_CLASS(klass)    (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_TOOLBAR, BtkToolbarClass))
#define BTK_IS_TOOLBAR(obj)         (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_TOOLBAR))
#define BTK_IS_TOOLBAR_CLASS(klass) (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_TOOLBAR))
#define BTK_TOOLBAR_GET_CLASS(obj)  (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_TOOLBAR, BtkToolbarClass))

#ifndef BTK_DISABLE_DEPRECATED
typedef enum
{
  BTK_TOOLBAR_CHILD_SPACE,
  BTK_TOOLBAR_CHILD_BUTTON,
  BTK_TOOLBAR_CHILD_TOGGLEBUTTON,
  BTK_TOOLBAR_CHILD_RADIOBUTTON,
  BTK_TOOLBAR_CHILD_WIDGET
} BtkToolbarChildType;

typedef struct _BtkToolbarChild	     BtkToolbarChild;

struct _BtkToolbarChild
{
  BtkToolbarChildType type;
  BtkWidget *widget;
  BtkWidget *icon;
  BtkWidget *label;
};

#endif /* BTK_DISABLE_DEPRECATED */

typedef enum
{
  BTK_TOOLBAR_SPACE_EMPTY,
  BTK_TOOLBAR_SPACE_LINE
} BtkToolbarSpaceStyle;

typedef struct _BtkToolbar           BtkToolbar;
typedef struct _BtkToolbarClass      BtkToolbarClass;
typedef struct _BtkToolbarPrivate    BtkToolbarPrivate;

struct _BtkToolbar
{
  BtkContainer container;

  /*< public >*/
  bint             GSEAL (num_children);
  GList           *GSEAL (children);
  BtkOrientation   GSEAL (orientation);
  BtkToolbarStyle  GSEAL (style);
  BtkIconSize      GSEAL (icon_size);

#ifndef BTK_DISABLE_DEPRECATED
  BtkTooltips     *GSEAL (tooltips);
#else
  bpointer         GSEAL (_tooltips);
#endif

  /*< private >*/
  bint             GSEAL (button_maxw);		/* maximum width of homogeneous children */
  bint             GSEAL (button_maxh);		/* maximum height of homogeneous children */

  buint            _btk_reserved1;
  buint            _btk_reserved2;

  buint            GSEAL (style_set) : 1;
  buint            GSEAL (icon_size_set) : 1;
};

struct _BtkToolbarClass
{
  BtkContainerClass parent_class;

  /* signals */
  void     (* orientation_changed) (BtkToolbar       *toolbar,
				    BtkOrientation    orientation);
  void     (* style_changed)       (BtkToolbar       *toolbar,
				    BtkToolbarStyle   style);
  bboolean (* popup_context_menu)  (BtkToolbar       *toolbar,
				    bint              x,
				    bint              y,
				    bint              button_number);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
};

GType           btk_toolbar_get_type                (void) B_GNUC_CONST;
BtkWidget *     btk_toolbar_new                     (void);

void            btk_toolbar_insert                  (BtkToolbar      *toolbar,
						     BtkToolItem     *item,
						     bint             pos);

bint            btk_toolbar_get_item_index          (BtkToolbar      *toolbar,
						     BtkToolItem     *item);
bint            btk_toolbar_get_n_items             (BtkToolbar      *toolbar);
BtkToolItem *   btk_toolbar_get_nth_item            (BtkToolbar      *toolbar,
						     bint             n);

bboolean        btk_toolbar_get_show_arrow          (BtkToolbar      *toolbar);
void            btk_toolbar_set_show_arrow          (BtkToolbar      *toolbar,
						     bboolean         show_arrow);

BtkToolbarStyle btk_toolbar_get_style               (BtkToolbar      *toolbar);
void            btk_toolbar_set_style               (BtkToolbar      *toolbar,
						     BtkToolbarStyle  style);
void            btk_toolbar_unset_style             (BtkToolbar      *toolbar);

BtkIconSize     btk_toolbar_get_icon_size           (BtkToolbar      *toolbar);
void            btk_toolbar_set_icon_size           (BtkToolbar      *toolbar,
                                                     BtkIconSize      icon_size);
void            btk_toolbar_unset_icon_size         (BtkToolbar      *toolbar);

BtkReliefStyle  btk_toolbar_get_relief_style        (BtkToolbar      *toolbar);
bint            btk_toolbar_get_drop_index          (BtkToolbar      *toolbar,
						     bint             x,
						     bint             y);
void            btk_toolbar_set_drop_highlight_item (BtkToolbar      *toolbar,
						     BtkToolItem     *tool_item,
						     bint             index_);


/* internal functions */
bchar *         _btk_toolbar_elide_underscores      (const bchar         *original);
void            _btk_toolbar_paint_space_line       (BtkWidget           *widget,
						     BtkToolbar          *toolbar,
						     const BdkRectangle  *area,
						     const BtkAllocation *allocation);
bint            _btk_toolbar_get_default_space_size (void);



#ifndef BTK_DISABLE_DEPRECATED

BtkOrientation  btk_toolbar_get_orientation         (BtkToolbar      *toolbar);
void            btk_toolbar_set_orientation         (BtkToolbar      *toolbar,
						     BtkOrientation   orientation);
bboolean        btk_toolbar_get_tooltips            (BtkToolbar      *toolbar);
void            btk_toolbar_set_tooltips            (BtkToolbar      *toolbar,
						     bboolean         enable);

/* Simple button items */
BtkWidget* btk_toolbar_append_item   (BtkToolbar      *toolbar,
				      const char      *text,
				      const char      *tooltip_text,
				      const char      *tooltip_private_text,
				      BtkWidget       *icon,
				      GCallback        callback,
				      bpointer         user_data);
BtkWidget* btk_toolbar_prepend_item  (BtkToolbar      *toolbar,
				      const char      *text,
				      const char      *tooltip_text,
				      const char      *tooltip_private_text,
				      BtkWidget       *icon,
				      GCallback        callback,
				      bpointer         user_data);
BtkWidget* btk_toolbar_insert_item   (BtkToolbar      *toolbar,
				      const char      *text,
				      const char      *tooltip_text,
				      const char      *tooltip_private_text,
				      BtkWidget       *icon,
				      GCallback        callback,
				      bpointer         user_data,
				      bint             position);

/* Stock Items */
BtkWidget* btk_toolbar_insert_stock    (BtkToolbar      *toolbar,
					const bchar     *stock_id,
					const char      *tooltip_text,
					const char      *tooltip_private_text,
					GCallback        callback,
					bpointer         user_data,
					bint             position);

/* Space Items */
void       btk_toolbar_append_space    (BtkToolbar      *toolbar);
void       btk_toolbar_prepend_space   (BtkToolbar      *toolbar);
void       btk_toolbar_insert_space    (BtkToolbar      *toolbar,
					bint             position);
void       btk_toolbar_remove_space    (BtkToolbar      *toolbar,
                                        bint             position);
/* Any element type */
BtkWidget* btk_toolbar_append_element  (BtkToolbar      *toolbar,
					BtkToolbarChildType type,
					BtkWidget       *widget,
					const char      *text,
					const char      *tooltip_text,
					const char      *tooltip_private_text,
					BtkWidget       *icon,
					GCallback        callback,
					bpointer         user_data);

BtkWidget* btk_toolbar_prepend_element (BtkToolbar      *toolbar,
					BtkToolbarChildType type,
					BtkWidget       *widget,
					const char      *text,
					const char      *tooltip_text,
					const char      *tooltip_private_text,
					BtkWidget       *icon,
					GCallback        callback,
					bpointer         user_data);

BtkWidget* btk_toolbar_insert_element  (BtkToolbar      *toolbar,
					BtkToolbarChildType type,
					BtkWidget       *widget,
					const char      *text,
					const char      *tooltip_text,
					const char      *tooltip_private_text,
					BtkWidget       *icon,
					GCallback        callback,
					bpointer         user_data,
					bint             position);

/* Generic Widgets */
void       btk_toolbar_append_widget   (BtkToolbar      *toolbar,
					BtkWidget       *widget,
					const char      *tooltip_text,
					const char      *tooltip_private_text);
void       btk_toolbar_prepend_widget  (BtkToolbar      *toolbar,
					BtkWidget       *widget,
					const char      *tooltip_text,
					const char	*tooltip_private_text);
void       btk_toolbar_insert_widget   (BtkToolbar      *toolbar,
					BtkWidget       *widget,
					const char      *tooltip_text,
					const char      *tooltip_private_text,
					bint             position);

#endif /* BTK_DISABLE_DEPRECATED */


B_END_DECLS

#endif /* __BTK_TOOLBAR_H__ */
