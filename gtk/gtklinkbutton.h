/* BTK - The GIMP Toolkit
 * btklinkbutton.h - an hyperlink-enabled button
 *
 * Copyright (C) 2005 Emmanuele Bassi <ebassi@gmail.com>
 * All rights reserved.
 *
 * Based on gnome-href code by:
 * 	James Henstridge <james@daa.com.au>
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

#ifndef __BTK_LINK_BUTTON_H__
#define __BTK_LINK_BUTTON_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkbutton.h>

G_BEGIN_DECLS

#define BTK_TYPE_LINK_BUTTON		(btk_link_button_get_type ())
#define BTK_LINK_BUTTON(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_LINK_BUTTON, BtkLinkButton))
#define BTK_IS_LINK_BUTTON(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_LINK_BUTTON))
#define BTK_LINK_BUTTON_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_LINK_BUTTON, BtkLinkButtonClass))
#define BTK_IS_LINK_BUTTON_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_LINK_BUTTON))
#define BTK_LINK_BUTTON_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_LINK_BUTTON, BtkLinkButtonClass))

typedef struct _BtkLinkButton		BtkLinkButton;
typedef struct _BtkLinkButtonClass	BtkLinkButtonClass;
typedef struct _BtkLinkButtonPrivate	BtkLinkButtonPrivate;

typedef void (*BtkLinkButtonUriFunc) (BtkLinkButton *button,
				      const gchar   *link_,
				      gpointer       user_data);

struct _BtkLinkButton
{
  BtkButton parent_instance;

  BtkLinkButtonPrivate *GSEAL (priv);
};

struct _BtkLinkButtonClass
{
  BtkButtonClass parent_class;

  void (*_btk_padding1) (void);
  void (*_btk_padding2) (void);
  void (*_btk_padding3) (void);
  void (*_btk_padding4) (void);
};

GType                 btk_link_button_get_type          (void) G_GNUC_CONST;

BtkWidget *           btk_link_button_new               (const gchar   *uri);
BtkWidget *           btk_link_button_new_with_label    (const gchar   *uri,
						         const gchar   *label);

const gchar *         btk_link_button_get_uri           (BtkLinkButton *link_button);
void                  btk_link_button_set_uri           (BtkLinkButton *link_button,
						         const gchar   *uri);

#ifndef BTK_DISABLE_DEPRECATED
BtkLinkButtonUriFunc  btk_link_button_set_uri_hook      (BtkLinkButtonUriFunc func,
							 gpointer             data,
							 GDestroyNotify       destroy);
#endif

gboolean              btk_link_button_get_visited       (BtkLinkButton *link_button);
void                  btk_link_button_set_visited       (BtkLinkButton *link_button,
                                                         gboolean       visited);


G_END_DECLS

#endif /* __BTK_LINK_BUTTON_H__ */
