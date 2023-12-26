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
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/.
 */

#ifndef __BTK_LABEL_H__
#define __BTK_LABEL_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkmisc.h>
#include <btk/btkwindow.h>
#include <btk/btkmenu.h>


G_BEGIN_DECLS

#define BTK_TYPE_LABEL		  (btk_label_get_type ())
#define BTK_LABEL(obj)		  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_LABEL, BtkLabel))
#define BTK_LABEL_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_LABEL, BtkLabelClass))
#define BTK_IS_LABEL(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_LABEL))
#define BTK_IS_LABEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_LABEL))
#define BTK_LABEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_LABEL, BtkLabelClass))


typedef struct _BtkLabel       BtkLabel;
typedef struct _BtkLabelClass  BtkLabelClass;

typedef struct _BtkLabelSelectionInfo BtkLabelSelectionInfo;

struct _BtkLabel
{
  BtkMisc misc;

  /*< private >*/
  gchar  *GSEAL (label);
  guint   GSEAL (jtype)            : 2;
  guint   GSEAL (wrap)             : 1;
  guint   GSEAL (use_underline)    : 1;
  guint   GSEAL (use_markup)       : 1;
  guint   GSEAL (ellipsize)        : 3;
  guint   GSEAL (single_line_mode) : 1;
  guint   GSEAL (have_transform)   : 1;
  guint   GSEAL (in_click)         : 1;
  guint   GSEAL (wrap_mode)        : 3;
  guint   GSEAL (pattern_set)      : 1;
  guint   GSEAL (track_links)      : 1;

  guint   GSEAL (mnemonic_keyval);

  gchar  *GSEAL (text);
  BangoAttrList *GSEAL (attrs);
  BangoAttrList *GSEAL (effective_attrs);

  BangoLayout *GSEAL (layout);

  BtkWidget *GSEAL (mnemonic_widget);
  BtkWindow *GSEAL (mnemonic_window);

  BtkLabelSelectionInfo *GSEAL (select_info);
};

struct _BtkLabelClass
{
  BtkMiscClass parent_class;

  void (* move_cursor)     (BtkLabel       *label,
			    BtkMovementStep step,
			    gint            count,
			    gboolean        extend_selection);
  void (* copy_clipboard)  (BtkLabel       *label);

  /* Hook to customize right-click popup for selectable labels */
  void (* populate_popup)   (BtkLabel       *label,
                             BtkMenu        *menu);

  gboolean (*activate_link) (BtkLabel       *label,
                             const gchar    *uri);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
};

GType                 btk_label_get_type          (void) G_GNUC_CONST;
BtkWidget*            btk_label_new               (const gchar   *str);
BtkWidget*            btk_label_new_with_mnemonic (const gchar   *str);
void                  btk_label_set_text          (BtkLabel      *label,
						   const gchar   *str);
const gchar *         btk_label_get_text          (BtkLabel      *label);
void                  btk_label_set_attributes    (BtkLabel      *label,
						   BangoAttrList *attrs);
BangoAttrList        *btk_label_get_attributes    (BtkLabel      *label);
void                  btk_label_set_label         (BtkLabel      *label,
						   const gchar   *str);
const gchar *         btk_label_get_label         (BtkLabel      *label);
void                  btk_label_set_markup        (BtkLabel      *label,
						   const gchar   *str);
void                  btk_label_set_use_markup    (BtkLabel      *label,
						   gboolean       setting);
gboolean              btk_label_get_use_markup    (BtkLabel      *label);
void                  btk_label_set_use_underline (BtkLabel      *label,
						   gboolean       setting);
gboolean              btk_label_get_use_underline (BtkLabel      *label);

void     btk_label_set_markup_with_mnemonic       (BtkLabel         *label,
						   const gchar      *str);
guint    btk_label_get_mnemonic_keyval            (BtkLabel         *label);
void     btk_label_set_mnemonic_widget            (BtkLabel         *label,
						   BtkWidget        *widget);
BtkWidget *btk_label_get_mnemonic_widget          (BtkLabel         *label);
void     btk_label_set_text_with_mnemonic         (BtkLabel         *label,
						   const gchar      *str);
void     btk_label_set_justify                    (BtkLabel         *label,
						   BtkJustification  jtype);
BtkJustification btk_label_get_justify            (BtkLabel         *label);
void     btk_label_set_ellipsize		  (BtkLabel         *label,
						   BangoEllipsizeMode mode);
BangoEllipsizeMode btk_label_get_ellipsize        (BtkLabel         *label);
void     btk_label_set_width_chars		  (BtkLabel         *label,
						   gint              n_chars);
gint     btk_label_get_width_chars                (BtkLabel         *label);
void     btk_label_set_max_width_chars    	  (BtkLabel         *label,
					  	   gint              n_chars);
gint     btk_label_get_max_width_chars  	  (BtkLabel         *label);
void     btk_label_set_pattern                    (BtkLabel         *label,
						   const gchar      *pattern);
void     btk_label_set_line_wrap                  (BtkLabel         *label,
						   gboolean          wrap);
gboolean btk_label_get_line_wrap                  (BtkLabel         *label);
void     btk_label_set_line_wrap_mode             (BtkLabel         *label,
						   BangoWrapMode     wrap_mode);
BangoWrapMode btk_label_get_line_wrap_mode        (BtkLabel         *label);
void     btk_label_set_selectable                 (BtkLabel         *label,
						   gboolean          setting);
gboolean btk_label_get_selectable                 (BtkLabel         *label);
void     btk_label_set_angle                      (BtkLabel         *label,
						   gdouble           angle);
gdouble  btk_label_get_angle                      (BtkLabel         *label);
void     btk_label_select_rebunnyion                  (BtkLabel         *label,
						   gint              start_offset,
						   gint              end_offset);
gboolean btk_label_get_selection_bounds           (BtkLabel         *label,
                                                   gint             *start,
                                                   gint             *end);

BangoLayout *btk_label_get_layout         (BtkLabel *label);
void         btk_label_get_layout_offsets (BtkLabel *label,
                                           gint     *x,
                                           gint     *y);

void         btk_label_set_single_line_mode  (BtkLabel *label,
                                              gboolean single_line_mode);
gboolean     btk_label_get_single_line_mode  (BtkLabel *label);

const gchar *btk_label_get_current_uri          (BtkLabel *label);
void         btk_label_set_track_visited_links  (BtkLabel *label,
                                                 gboolean  track_links);
gboolean     btk_label_get_track_visited_links  (BtkLabel *label);

#ifndef BTK_DISABLE_DEPRECATED

#define  btk_label_set           btk_label_set_text
void       btk_label_get           (BtkLabel          *label,
                                    gchar            **str);

/* Convenience function to set the name and pattern by parsing
 * a string with embedded underscores, and return the appropriate
 * key symbol for the accelerator.
 */
guint btk_label_parse_uline            (BtkLabel    *label,
					const gchar *string);

#endif /* BTK_DISABLE_DEPRECATED */

/* private */

void _btk_label_mnemonics_visible_apply_recursively (BtkWidget *widget,
                                                     gboolean   mnemonics_visible);

G_END_DECLS

#endif /* __BTK_LABEL_H__ */
