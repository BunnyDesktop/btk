/* BTK - The GIMP Toolkit
 * btkpagesetup.h: Page Setup
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

#ifndef __BTK_PAGE_SETUP_H__
#define __BTK_PAGE_SETUP_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkpapersize.h>


B_BEGIN_DECLS

typedef struct _BtkPageSetup BtkPageSetup;

#define BTK_TYPE_PAGE_SETUP    (btk_page_setup_get_type ())
#define BTK_PAGE_SETUP(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_PAGE_SETUP, BtkPageSetup))
#define BTK_IS_PAGE_SETUP(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_PAGE_SETUP))

GType              btk_page_setup_get_type          (void) B_GNUC_CONST;
BtkPageSetup *     btk_page_setup_new               (void);
BtkPageSetup *     btk_page_setup_copy              (BtkPageSetup       *other);
BtkPageOrientation btk_page_setup_get_orientation   (BtkPageSetup       *setup);
void               btk_page_setup_set_orientation   (BtkPageSetup       *setup,
						     BtkPageOrientation  orientation);
BtkPaperSize *     btk_page_setup_get_paper_size    (BtkPageSetup       *setup);
void               btk_page_setup_set_paper_size    (BtkPageSetup       *setup,
						     BtkPaperSize       *size);
gdouble            btk_page_setup_get_top_margin    (BtkPageSetup       *setup,
						     BtkUnit             unit);
void               btk_page_setup_set_top_margin    (BtkPageSetup       *setup,
						     gdouble             margin,
						     BtkUnit             unit);
gdouble            btk_page_setup_get_bottom_margin (BtkPageSetup       *setup,
						     BtkUnit             unit);
void               btk_page_setup_set_bottom_margin (BtkPageSetup       *setup,
						     gdouble             margin,
						     BtkUnit             unit);
gdouble            btk_page_setup_get_left_margin   (BtkPageSetup       *setup,
						     BtkUnit             unit);
void               btk_page_setup_set_left_margin   (BtkPageSetup       *setup,
						     gdouble             margin,
						     BtkUnit             unit);
gdouble            btk_page_setup_get_right_margin  (BtkPageSetup       *setup,
						     BtkUnit             unit);
void               btk_page_setup_set_right_margin  (BtkPageSetup       *setup,
						     gdouble             margin,
						     BtkUnit             unit);

void btk_page_setup_set_paper_size_and_default_margins (BtkPageSetup    *setup,
							BtkPaperSize    *size);

/* These take orientation, but not margins into consideration */
gdouble            btk_page_setup_get_paper_width   (BtkPageSetup       *setup,
						     BtkUnit             unit);
gdouble            btk_page_setup_get_paper_height  (BtkPageSetup       *setup,
						     BtkUnit             unit);


/* These take orientation, and margins into consideration */
gdouble            btk_page_setup_get_page_width    (BtkPageSetup       *setup,
						     BtkUnit             unit);
gdouble            btk_page_setup_get_page_height   (BtkPageSetup       *setup,
						     BtkUnit             unit);

/* Saving and restoring page setup */
BtkPageSetup	  *btk_page_setup_new_from_file	    (const gchar         *file_name,
						     GError              **error);
gboolean	   btk_page_setup_load_file	    (BtkPageSetup        *setup,
						     const char          *file_name,
						     GError             **error);
gboolean	   btk_page_setup_to_file	    (BtkPageSetup        *setup,
						     const char          *file_name,
						     GError             **error);
BtkPageSetup	  *btk_page_setup_new_from_key_file (GKeyFile            *key_file,
						     const gchar         *group_name,
						     GError             **error);
gboolean           btk_page_setup_load_key_file     (BtkPageSetup        *setup,
				                     GKeyFile            *key_file,
				                     const gchar         *group_name,
				                     GError             **error);
void		   btk_page_setup_to_key_file	    (BtkPageSetup        *setup,
						     GKeyFile            *key_file,
						     const gchar         *group_name);

B_END_DECLS

#endif /* __BTK_PAGE_SETUP_H__ */
