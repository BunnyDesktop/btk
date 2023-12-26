/* BTK - The GIMP Toolkit
 * btkprintsettings.h: Print Settings
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

#ifndef __BTK_PRINT_SETTINGS_H__
#define __BTK_PRINT_SETTINGS_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkpapersize.h>

B_BEGIN_DECLS

typedef struct _BtkPrintSettings BtkPrintSettings;

#define BTK_TYPE_PRINT_SETTINGS    (btk_print_settings_get_type ())
#define BTK_PRINT_SETTINGS(obj)    (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_PRINT_SETTINGS, BtkPrintSettings))
#define BTK_IS_PRINT_SETTINGS(obj) (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_PRINT_SETTINGS))

typedef void  (*BtkPrintSettingsFunc)  (const gchar *key,
					const gchar *value,
					gpointer     user_data);

typedef struct _BtkPageRange BtkPageRange;
struct _BtkPageRange
{
  gint start;
  gint end;
};

GType             btk_print_settings_get_type                (void) B_GNUC_CONST;
BtkPrintSettings *btk_print_settings_new                     (void);

BtkPrintSettings *btk_print_settings_copy                    (BtkPrintSettings     *other);

BtkPrintSettings *btk_print_settings_new_from_file           (const gchar          *file_name,
							      GError              **error);
gboolean          btk_print_settings_load_file               (BtkPrintSettings     *settings,
							      const gchar          *file_name,
							      GError              **error);
gboolean          btk_print_settings_to_file                 (BtkPrintSettings     *settings,
							      const gchar          *file_name,
							      GError              **error);
BtkPrintSettings *btk_print_settings_new_from_key_file       (GKeyFile             *key_file,
							      const gchar          *group_name,
							      GError              **error);
gboolean          btk_print_settings_load_key_file           (BtkPrintSettings     *settings,
							      GKeyFile             *key_file,
							      const gchar          *group_name,
							      GError              **error);
void              btk_print_settings_to_key_file             (BtkPrintSettings     *settings,
							      GKeyFile             *key_file,
							      const gchar          *group_name);
gboolean          btk_print_settings_has_key                 (BtkPrintSettings     *settings,
							      const gchar          *key);
const gchar *     btk_print_settings_get                 (BtkPrintSettings     *settings,
							      const gchar          *key);
void              btk_print_settings_set                     (BtkPrintSettings     *settings,
							      const gchar          *key,
							      const gchar          *value);
void              btk_print_settings_unset                   (BtkPrintSettings     *settings,
							      const gchar          *key);
void              btk_print_settings_foreach                 (BtkPrintSettings     *settings,
							      BtkPrintSettingsFunc  func,
							      gpointer              user_data);
gboolean          btk_print_settings_get_bool                (BtkPrintSettings     *settings,
							      const gchar          *key);
void              btk_print_settings_set_bool                (BtkPrintSettings     *settings,
							      const gchar          *key,
							      gboolean              value);
gdouble           btk_print_settings_get_double              (BtkPrintSettings     *settings,
							      const gchar          *key);
gdouble           btk_print_settings_get_double_with_default (BtkPrintSettings     *settings,
							      const gchar          *key,
							      gdouble               def);
void              btk_print_settings_set_double              (BtkPrintSettings     *settings,
							      const gchar          *key,
							      gdouble               value);
gdouble           btk_print_settings_get_length              (BtkPrintSettings     *settings,
							      const gchar          *key,
							      BtkUnit               unit);
void              btk_print_settings_set_length              (BtkPrintSettings     *settings,
							      const gchar          *key,
							      gdouble               value,
							      BtkUnit               unit);
gint              btk_print_settings_get_int                 (BtkPrintSettings     *settings,
							      const gchar          *key);
gint              btk_print_settings_get_int_with_default    (BtkPrintSettings     *settings,
							      const gchar          *key,
							      gint                  def);
void              btk_print_settings_set_int                 (BtkPrintSettings     *settings,
							      const gchar          *key,
							      gint                  value);

#define BTK_PRINT_SETTINGS_PRINTER          "printer"
#define BTK_PRINT_SETTINGS_ORIENTATION      "orientation"
#define BTK_PRINT_SETTINGS_PAPER_FORMAT     "paper-format"
#define BTK_PRINT_SETTINGS_PAPER_WIDTH      "paper-width"
#define BTK_PRINT_SETTINGS_PAPER_HEIGHT     "paper-height"
#define BTK_PRINT_SETTINGS_N_COPIES         "n-copies"
#define BTK_PRINT_SETTINGS_DEFAULT_SOURCE   "default-source"
#define BTK_PRINT_SETTINGS_QUALITY          "quality"
#define BTK_PRINT_SETTINGS_RESOLUTION       "resolution"
#define BTK_PRINT_SETTINGS_USE_COLOR        "use-color"
#define BTK_PRINT_SETTINGS_DUPLEX           "duplex"
#define BTK_PRINT_SETTINGS_COLLATE          "collate"
#define BTK_PRINT_SETTINGS_REVERSE          "reverse"
#define BTK_PRINT_SETTINGS_MEDIA_TYPE       "media-type"
#define BTK_PRINT_SETTINGS_DITHER           "dither"
#define BTK_PRINT_SETTINGS_SCALE            "scale"
#define BTK_PRINT_SETTINGS_PRINT_PAGES      "print-pages"
#define BTK_PRINT_SETTINGS_PAGE_RANGES      "page-ranges"
#define BTK_PRINT_SETTINGS_PAGE_SET         "page-set"
#define BTK_PRINT_SETTINGS_FINISHINGS       "finishings"
#define BTK_PRINT_SETTINGS_NUMBER_UP        "number-up"
#define BTK_PRINT_SETTINGS_NUMBER_UP_LAYOUT "number-up-layout"
#define BTK_PRINT_SETTINGS_OUTPUT_BIN       "output-bin"
#define BTK_PRINT_SETTINGS_RESOLUTION_X     "resolution-x"
#define BTK_PRINT_SETTINGS_RESOLUTION_Y     "resolution-y"
#define BTK_PRINT_SETTINGS_PRINTER_LPI      "printer-lpi"

#define BTK_PRINT_SETTINGS_OUTPUT_FILE_FORMAT  "output-file-format"
#define BTK_PRINT_SETTINGS_OUTPUT_URI          "output-uri"

#define BTK_PRINT_SETTINGS_WIN32_DRIVER_VERSION "win32-driver-version"
#define BTK_PRINT_SETTINGS_WIN32_DRIVER_EXTRA   "win32-driver-extra"

/* Helpers: */

const gchar *btk_print_settings_get_printer           (BtkPrintSettings   *settings);
void                  btk_print_settings_set_printer           (BtkPrintSettings   *settings,
								const gchar        *printer);
BtkPageOrientation    btk_print_settings_get_orientation       (BtkPrintSettings   *settings);
void                  btk_print_settings_set_orientation       (BtkPrintSettings   *settings,
								BtkPageOrientation  orientation);
BtkPaperSize *        btk_print_settings_get_paper_size        (BtkPrintSettings   *settings);
void                  btk_print_settings_set_paper_size        (BtkPrintSettings   *settings,
								BtkPaperSize       *paper_size);
gdouble               btk_print_settings_get_paper_width       (BtkPrintSettings   *settings,
								BtkUnit             unit);
void                  btk_print_settings_set_paper_width       (BtkPrintSettings   *settings,
								gdouble             width,
								BtkUnit             unit);
gdouble               btk_print_settings_get_paper_height      (BtkPrintSettings   *settings,
								BtkUnit             unit);
void                  btk_print_settings_set_paper_height      (BtkPrintSettings   *settings,
								gdouble             height,
								BtkUnit             unit);
gboolean              btk_print_settings_get_use_color         (BtkPrintSettings   *settings);
void                  btk_print_settings_set_use_color         (BtkPrintSettings   *settings,
								gboolean            use_color);
gboolean              btk_print_settings_get_collate           (BtkPrintSettings   *settings);
void                  btk_print_settings_set_collate           (BtkPrintSettings   *settings,
								gboolean            collate);
gboolean              btk_print_settings_get_reverse           (BtkPrintSettings   *settings);
void                  btk_print_settings_set_reverse           (BtkPrintSettings   *settings,
								gboolean            reverse);
BtkPrintDuplex        btk_print_settings_get_duplex            (BtkPrintSettings   *settings);
void                  btk_print_settings_set_duplex            (BtkPrintSettings   *settings,
								BtkPrintDuplex      duplex);
BtkPrintQuality       btk_print_settings_get_quality           (BtkPrintSettings   *settings);
void                  btk_print_settings_set_quality           (BtkPrintSettings   *settings,
								BtkPrintQuality     quality);
gint                  btk_print_settings_get_n_copies          (BtkPrintSettings   *settings);
void                  btk_print_settings_set_n_copies          (BtkPrintSettings   *settings,
								gint                num_copies);
gint                  btk_print_settings_get_number_up         (BtkPrintSettings   *settings);
void                  btk_print_settings_set_number_up         (BtkPrintSettings   *settings,
								gint                number_up);
BtkNumberUpLayout     btk_print_settings_get_number_up_layout  (BtkPrintSettings   *settings);
void                  btk_print_settings_set_number_up_layout  (BtkPrintSettings   *settings,
								BtkNumberUpLayout   number_up_layout);
gint                  btk_print_settings_get_resolution        (BtkPrintSettings   *settings);
void                  btk_print_settings_set_resolution        (BtkPrintSettings   *settings,
								gint                resolution);
gint                  btk_print_settings_get_resolution_x      (BtkPrintSettings   *settings);
gint                  btk_print_settings_get_resolution_y      (BtkPrintSettings   *settings);
void                  btk_print_settings_set_resolution_xy     (BtkPrintSettings   *settings,
								gint                resolution_x,
								gint                resolution_y);
gdouble               btk_print_settings_get_printer_lpi       (BtkPrintSettings   *settings);
void                  btk_print_settings_set_printer_lpi       (BtkPrintSettings   *settings,
								gdouble             lpi);
gdouble               btk_print_settings_get_scale             (BtkPrintSettings   *settings);
void                  btk_print_settings_set_scale             (BtkPrintSettings   *settings,
								gdouble             scale);
BtkPrintPages         btk_print_settings_get_print_pages       (BtkPrintSettings   *settings);
void                  btk_print_settings_set_print_pages       (BtkPrintSettings   *settings,
								BtkPrintPages       pages);
BtkPageRange *        btk_print_settings_get_page_ranges       (BtkPrintSettings   *settings,
								gint               *num_ranges);
void                  btk_print_settings_set_page_ranges       (BtkPrintSettings   *settings,
								BtkPageRange       *page_ranges,
								gint                num_ranges);
BtkPageSet            btk_print_settings_get_page_set          (BtkPrintSettings   *settings);
void                  btk_print_settings_set_page_set          (BtkPrintSettings   *settings,
								BtkPageSet          page_set);
const gchar *         btk_print_settings_get_default_source    (BtkPrintSettings   *settings);
void                  btk_print_settings_set_default_source    (BtkPrintSettings   *settings,
								const gchar        *default_source);
const gchar *         btk_print_settings_get_media_type        (BtkPrintSettings   *settings);
void                  btk_print_settings_set_media_type        (BtkPrintSettings   *settings,
								const gchar        *media_type);
const gchar *         btk_print_settings_get_dither            (BtkPrintSettings   *settings);
void                  btk_print_settings_set_dither            (BtkPrintSettings   *settings,
								const gchar        *dither);
const gchar *         btk_print_settings_get_finishings        (BtkPrintSettings   *settings);
void                  btk_print_settings_set_finishings        (BtkPrintSettings   *settings,
								const gchar        *finishings);
const gchar *         btk_print_settings_get_output_bin        (BtkPrintSettings   *settings);
void                  btk_print_settings_set_output_bin        (BtkPrintSettings   *settings,
								const gchar        *output_bin);

B_END_DECLS

#endif /* __BTK_PRINT_SETTINGS_H__ */
