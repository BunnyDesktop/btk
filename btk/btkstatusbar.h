/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * BtkStatusbar Copyright (C) 1998 Shawn T. Amundson
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

#ifndef __BTK_STATUSBAR_H__
#define __BTK_STATUSBAR_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkhbox.h>

G_BEGIN_DECLS

#define BTK_TYPE_STATUSBAR            (btk_statusbar_get_type ())
#define BTK_STATUSBAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_STATUSBAR, BtkStatusbar))
#define BTK_STATUSBAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_STATUSBAR, BtkStatusbarClass))
#define BTK_IS_STATUSBAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_STATUSBAR))
#define BTK_IS_STATUSBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_STATUSBAR))
#define BTK_STATUSBAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_STATUSBAR, BtkStatusbarClass))


typedef struct _BtkStatusbar      BtkStatusbar;
typedef struct _BtkStatusbarClass BtkStatusbarClass;

struct _BtkStatusbar
{
  BtkHBox parent_widget;

  BtkWidget *GSEAL (frame);
  BtkWidget *GSEAL (label);

  GSList *GSEAL (messages);
  GSList *GSEAL (keys);

  guint GSEAL (seq_context_id);
  guint GSEAL (seq_message_id);

  BdkWindow *GSEAL (grip_window);

  guint GSEAL (has_resize_grip) : 1;
};

struct _BtkStatusbarClass
{
  BtkHBoxClass parent_class;

  gpointer reserved;

  void	(*text_pushed)	(BtkStatusbar	*statusbar,
			 guint		 context_id,
			 const gchar	*text);
  void	(*text_popped)	(BtkStatusbar	*statusbar,
			 guint		 context_id,
			 const gchar	*text);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};


GType      btk_statusbar_get_type     	(void) G_GNUC_CONST;
BtkWidget* btk_statusbar_new          	(void);
/* If you don't want to use contexts, 0 is a predefined global
 * context_id you can pass to push/pop/remove
 */
guint	   btk_statusbar_get_context_id	(BtkStatusbar *statusbar,
					 const gchar  *context_description);
/* Returns message_id used for btk_statusbar_remove */
guint      btk_statusbar_push          	(BtkStatusbar *statusbar,
					 guint	       context_id,
					 const gchar  *text);
void       btk_statusbar_pop          	(BtkStatusbar *statusbar,
					 guint	       context_id);
void       btk_statusbar_remove        	(BtkStatusbar *statusbar,
					 guint	       context_id,
					 guint         message_id);
void       btk_statusbar_remove_all    	(BtkStatusbar *statusbar,
					 guint	       context_id);
					 

void     btk_statusbar_set_has_resize_grip (BtkStatusbar *statusbar,
					    gboolean      setting);
gboolean btk_statusbar_get_has_resize_grip (BtkStatusbar *statusbar);

BtkWidget* btk_statusbar_get_message_area  (BtkStatusbar *statusbar);

G_END_DECLS

#endif /* __BTK_STATUSBAR_H__ */
