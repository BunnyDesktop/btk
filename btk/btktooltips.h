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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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

#ifndef BTK_DISABLE_DEPRECATED

#ifndef __BTK_TOOLTIPS_H__
#define __BTK_TOOLTIPS_H__

#include <btk/btkwidget.h>
#include <btk/btkwindow.h>


B_BEGIN_DECLS

#define BTK_TYPE_TOOLTIPS                  (btk_tooltips_get_type ())
#define BTK_TOOLTIPS(obj)                  (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_TOOLTIPS, BtkTooltips))
#define BTK_TOOLTIPS_CLASS(klass)          (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_TOOLTIPS, BtkTooltipsClass))
#define BTK_IS_TOOLTIPS(obj)               (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_TOOLTIPS))
#define BTK_IS_TOOLTIPS_CLASS(klass)       (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_TOOLTIPS))
#define BTK_TOOLTIPS_GET_CLASS(obj)        (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_TOOLTIPS, BtkTooltipsClass))


typedef struct _BtkTooltips	 BtkTooltips;
typedef struct _BtkTooltipsClass BtkTooltipsClass;
typedef struct _BtkTooltipsData	 BtkTooltipsData;

struct _BtkTooltipsData
{
  BtkTooltips *tooltips;
  BtkWidget *widget;
  bchar *tip_text;
  bchar *tip_private;
};

struct _BtkTooltips
{
  BtkObject parent_instance;

  /*< private >*/
  BtkWidget *tip_window;
  BtkWidget *tip_label;
  BtkTooltipsData *active_tips_data;
  GList *tips_data_list; /* unused */

  buint   delay : 30;
  buint	  enabled : 1;
  buint   have_grab : 1;
  buint   use_sticky_delay : 1;
  bint	  timer_tag;
  GTimeVal last_popdown;
};

struct _BtkTooltipsClass
{
  BtkObjectClass parent_class;

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};

GType		 btk_tooltips_get_type	   (void) B_GNUC_CONST;
BtkTooltips*	 btk_tooltips_new	   (void);

void		 btk_tooltips_enable	   (BtkTooltips   *tooltips);
void		 btk_tooltips_disable	   (BtkTooltips   *tooltips);
void		 btk_tooltips_set_delay	   (BtkTooltips   *tooltips,
					    buint	   delay);
void		 btk_tooltips_set_tip	   (BtkTooltips   *tooltips,
					    BtkWidget	  *widget,
					    const bchar   *tip_text,
					    const bchar   *tip_private);
BtkTooltipsData* btk_tooltips_data_get	   (BtkWidget	  *widget);
void             btk_tooltips_force_window (BtkTooltips   *tooltips);

bboolean         btk_tooltips_get_info_from_tip_window (BtkWindow    *tip_window,
                                                        BtkTooltips **tooltips,
                                                        BtkWidget   **current_widget);

B_END_DECLS

#endif /* __BTK_TOOLTIPS_H__ */

#endif /* BTK_DISABLE_DEPRECATED */
