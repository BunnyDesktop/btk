/* BTK - The GIMP Toolkit
 * btkpapersize.h: Paper Size
 * Copyright (C) 2006, Red Hat, Inc.
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

#ifndef __BTK_PAPER_SIZE_H__
#define __BTK_PAPER_SIZE_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkenums.h>


B_BEGIN_DECLS

typedef struct _BtkPaperSize BtkPaperSize;

#define BTK_TYPE_PAPER_SIZE    (btk_paper_size_get_type ())

/* Common names, from PWG 5101.1-2002 PWG: Standard for Media Standardized Names */
#define BTK_PAPER_NAME_A3 "iso_a3"
#define BTK_PAPER_NAME_A4 "iso_a4"
#define BTK_PAPER_NAME_A5 "iso_a5"
#define BTK_PAPER_NAME_B5 "iso_b5"
#define BTK_PAPER_NAME_LETTER "na_letter"
#define BTK_PAPER_NAME_EXECUTIVE "na_executive"
#define BTK_PAPER_NAME_LEGAL "na_legal"

GType btk_paper_size_get_type (void) B_GNUC_CONST;

BtkPaperSize *btk_paper_size_new          (const gchar  *name);
BtkPaperSize *btk_paper_size_new_from_ppd (const gchar  *ppd_name,
					   const gchar  *ppd_display_name,
					   gdouble       width,
					   gdouble       height);
BtkPaperSize *btk_paper_size_new_custom   (const gchar  *name,
					   const gchar  *display_name,
					   gdouble       width,
					   gdouble       height,
					   BtkUnit       unit);
BtkPaperSize *btk_paper_size_copy         (BtkPaperSize *other);
void          btk_paper_size_free         (BtkPaperSize *size);
gboolean      btk_paper_size_is_equal     (BtkPaperSize *size1,
					   BtkPaperSize *size2);

GList        *btk_paper_size_get_paper_sizes (gboolean include_custom);

/* The width is always the shortest side, measure in mm */
const gchar *btk_paper_size_get_name         (BtkPaperSize *size);
const gchar *btk_paper_size_get_display_name (BtkPaperSize *size);
const gchar *btk_paper_size_get_ppd_name     (BtkPaperSize *size);

gdouble  btk_paper_size_get_width        (BtkPaperSize *size, BtkUnit unit);
gdouble  btk_paper_size_get_height       (BtkPaperSize *size, BtkUnit unit);
gboolean btk_paper_size_is_custom        (BtkPaperSize *size);

/* Only for custom sizes: */
void    btk_paper_size_set_size                  (BtkPaperSize *size, 
                                                  gdouble       width, 
                                                  gdouble       height, 
                                                  BtkUnit       unit);

gdouble btk_paper_size_get_default_top_margin    (BtkPaperSize *size,
						  BtkUnit       unit);
gdouble btk_paper_size_get_default_bottom_margin (BtkPaperSize *size,
						  BtkUnit       unit);
gdouble btk_paper_size_get_default_left_margin   (BtkPaperSize *size,
						  BtkUnit       unit);
gdouble btk_paper_size_get_default_right_margin  (BtkPaperSize *size,
						  BtkUnit       unit);

const gchar *btk_paper_size_get_default (void);

BtkPaperSize *btk_paper_size_new_from_key_file (GKeyFile    *key_file,
					        const gchar *group_name,
					        GError     **error);
void     btk_paper_size_to_key_file            (BtkPaperSize *size,
					        GKeyFile     *key_file,
					        const gchar  *group_name);

B_END_DECLS

#endif /* __BTK_PAPER_SIZE_H__ */
