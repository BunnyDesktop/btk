/* BTK - The GIMP Toolkit
 * Copyright (C) 2000 Red Hat, Inc.
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

#ifndef __BTK_COLOR_SELECTION_H__
#define __BTK_COLOR_SELECTION_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkdialog.h>
#include <btk/btkvbox.h>

B_BEGIN_DECLS

#define BTK_TYPE_COLOR_SELECTION			(btk_color_selection_get_type ())
#define BTK_COLOR_SELECTION(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_COLOR_SELECTION, BtkColorSelection))
#define BTK_COLOR_SELECTION_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_COLOR_SELECTION, BtkColorSelectionClass))
#define BTK_IS_COLOR_SELECTION(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_COLOR_SELECTION))
#define BTK_IS_COLOR_SELECTION_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_COLOR_SELECTION))
#define BTK_COLOR_SELECTION_GET_CLASS(obj)              (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_COLOR_SELECTION, BtkColorSelectionClass))


typedef struct _BtkColorSelection       BtkColorSelection;
typedef struct _BtkColorSelectionClass  BtkColorSelectionClass;


typedef void (* BtkColorSelectionChangePaletteFunc) (const BdkColor    *colors,
                                                     gint               n_colors);
typedef void (* BtkColorSelectionChangePaletteWithScreenFunc) (BdkScreen         *screen,
							       const BdkColor    *colors,
							       gint               n_colors);

struct _BtkColorSelection
{
  BtkVBox parent_instance;

  /* < private_data > */
  gpointer GSEAL (private_data);
};

struct _BtkColorSelectionClass
{
  BtkVBoxClass parent_class;

  void (*color_changed)	(BtkColorSelection *color_selection);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};


/* ColorSelection */

GType      btk_color_selection_get_type                (void) B_GNUC_CONST;
BtkWidget *btk_color_selection_new                     (void);
gboolean   btk_color_selection_get_has_opacity_control (BtkColorSelection *colorsel);
void       btk_color_selection_set_has_opacity_control (BtkColorSelection *colorsel,
							gboolean           has_opacity);
gboolean   btk_color_selection_get_has_palette         (BtkColorSelection *colorsel);
void       btk_color_selection_set_has_palette         (BtkColorSelection *colorsel,
							gboolean           has_palette);


void     btk_color_selection_set_current_color   (BtkColorSelection *colorsel,
						  const BdkColor    *color);
void     btk_color_selection_set_current_alpha   (BtkColorSelection *colorsel,
						  guint16            alpha);
void     btk_color_selection_get_current_color   (BtkColorSelection *colorsel,
						  BdkColor          *color);
guint16  btk_color_selection_get_current_alpha   (BtkColorSelection *colorsel);
void     btk_color_selection_set_previous_color  (BtkColorSelection *colorsel,
						  const BdkColor    *color);
void     btk_color_selection_set_previous_alpha  (BtkColorSelection *colorsel,
						  guint16            alpha);
void     btk_color_selection_get_previous_color  (BtkColorSelection *colorsel,
						  BdkColor          *color);
guint16  btk_color_selection_get_previous_alpha  (BtkColorSelection *colorsel);

gboolean btk_color_selection_is_adjusting        (BtkColorSelection *colorsel);

gboolean btk_color_selection_palette_from_string (const gchar       *str,
                                                  BdkColor         **colors,
                                                  gint              *n_colors);
gchar*   btk_color_selection_palette_to_string   (const BdkColor    *colors,
                                                  gint               n_colors);

#ifndef BTK_DISABLE_DEPRECATED
#ifndef BDK_MULTIHEAD_SAFE
BtkColorSelectionChangePaletteFunc           btk_color_selection_set_change_palette_hook             (BtkColorSelectionChangePaletteFunc           func);
#endif
#endif

BtkColorSelectionChangePaletteWithScreenFunc btk_color_selection_set_change_palette_with_screen_hook (BtkColorSelectionChangePaletteWithScreenFunc func);

#ifndef BTK_DISABLE_DEPRECATED
/* Deprecated calls: */
void btk_color_selection_set_color         (BtkColorSelection *colorsel,
					    gdouble           *color);
void btk_color_selection_get_color         (BtkColorSelection *colorsel,
					    gdouble           *color);
void btk_color_selection_set_update_policy (BtkColorSelection *colorsel,
					    BtkUpdateType      policy);
#endif /* BTK_DISABLE_DEPRECATED */

B_END_DECLS

#endif /* __BTK_COLOR_SELECTION_H__ */
