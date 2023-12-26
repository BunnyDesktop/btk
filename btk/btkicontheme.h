/* BtkIconTheme - a loader for icon themes
 * btk-icon-loader.h Copyright (C) 2002, 2003 Red Hat, Inc.
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

#ifndef __BTK_ICON_THEME_H__
#define __BTK_ICON_THEME_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <bdk-pixbuf/bdk-pixbuf.h>
#include <bdk/bdk.h>

B_BEGIN_DECLS

#define BTK_TYPE_ICON_INFO              (btk_icon_info_get_type ())

#define BTK_TYPE_ICON_THEME             (btk_icon_theme_get_type ())
#define BTK_ICON_THEME(obj)             (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_ICON_THEME, BtkIconTheme))
#define BTK_ICON_THEME_CLASS(klass)     (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_ICON_THEME, BtkIconThemeClass))
#define BTK_IS_ICON_THEME(obj)          (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_ICON_THEME))
#define BTK_IS_ICON_THEME_CLASS(klass)  (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_ICON_THEME))
#define BTK_ICON_THEME_GET_CLASS(obj)   (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_ICON_THEME, BtkIconThemeClass))

typedef struct _BtkIconInfo         BtkIconInfo;
typedef struct _BtkIconTheme        BtkIconTheme;
typedef struct _BtkIconThemeClass   BtkIconThemeClass;
typedef struct _BtkIconThemePrivate BtkIconThemePrivate;

struct _BtkIconTheme
{
  /*< private >*/
  BObject parent_instance;

  BtkIconThemePrivate *GSEAL (priv);
};

struct _BtkIconThemeClass
{
  BObjectClass parent_class;

  void (* changed)  (BtkIconTheme *icon_theme);
};

/**
 * BtkIconLookupFlags:
 * @BTK_ICON_LOOKUP_NO_SVG: Never return SVG icons, even if bdk-pixbuf
 *   supports them. Cannot be used together with %BTK_ICON_LOOKUP_FORCE_SVG.
 * @BTK_ICON_LOOKUP_FORCE_SVG: Return SVG icons, even if bdk-pixbuf
 *   doesn't support them.
 *   Cannot be used together with %BTK_ICON_LOOKUP_NO_SVG.
 * @BTK_ICON_LOOKUP_USE_BUILTIN: When passed to
 *   btk_icon_theme_lookup_icon() includes builtin icons
 *   as well as files. For a builtin icon, btk_icon_info_get_filename()
 *   returns %NULL and you need to call btk_icon_info_get_builtin_pixbuf().
 * @BTK_ICON_LOOKUP_GENERIC_FALLBACK: Try to shorten icon name at '-'
 *   characters before looking at inherited themes. For more general
 *   fallback, see btk_icon_theme_choose_icon(). Since 2.12.
 * @BTK_ICON_LOOKUP_FORCE_SIZE: Always return the icon scaled to the
 *   requested size. Since 2.14.
 * 
 * Used to specify options for btk_icon_theme_lookup_icon()
 **/
typedef enum
{
  BTK_ICON_LOOKUP_NO_SVG           = 1 << 0,
  BTK_ICON_LOOKUP_FORCE_SVG        = 1 << 1,
  BTK_ICON_LOOKUP_USE_BUILTIN      = 1 << 2,
  BTK_ICON_LOOKUP_GENERIC_FALLBACK = 1 << 3,
  BTK_ICON_LOOKUP_FORCE_SIZE       = 1 << 4
} BtkIconLookupFlags;

#define BTK_ICON_THEME_ERROR btk_icon_theme_error_quark ()

/**
 * BtkIconThemeError:
 * @BTK_ICON_THEME_NOT_FOUND: The icon specified does not exist in the theme
 * @BTK_ICON_THEME_FAILED: An unspecified error occurred.
 * 
 * Error codes for BtkIconTheme operations.
 **/
typedef enum {
  BTK_ICON_THEME_NOT_FOUND,
  BTK_ICON_THEME_FAILED
} BtkIconThemeError;

GQuark btk_icon_theme_error_quark (void);

#ifdef G_OS_WIN32
/* Reserve old name for DLL ABI backward compatibility */
#define btk_icon_theme_set_search_path btk_icon_theme_set_search_path_utf8
#define btk_icon_theme_get_search_path btk_icon_theme_get_search_path_utf8
#define btk_icon_theme_append_search_path btk_icon_theme_append_search_path_utf8
#define btk_icon_theme_prepend_search_path btk_icon_theme_prepend_search_path_utf8
#define btk_icon_info_get_filename btk_icon_info_get_filename_utf8
#endif

GType         btk_icon_theme_get_type              (void) B_GNUC_CONST;

BtkIconTheme *btk_icon_theme_new                   (void);
BtkIconTheme *btk_icon_theme_get_default           (void);
BtkIconTheme *btk_icon_theme_get_for_screen        (BdkScreen                   *screen);
void          btk_icon_theme_set_screen            (BtkIconTheme                *icon_theme,
						    BdkScreen                   *screen);

void          btk_icon_theme_set_search_path       (BtkIconTheme                *icon_theme,
						    const bchar                 *path[],
						    bint                         n_elements);
void          btk_icon_theme_get_search_path       (BtkIconTheme                *icon_theme,
						    bchar                      **path[],
						    bint                        *n_elements);
void          btk_icon_theme_append_search_path    (BtkIconTheme                *icon_theme,
						    const bchar                 *path);
void          btk_icon_theme_prepend_search_path   (BtkIconTheme                *icon_theme,
						    const bchar                 *path);

void          btk_icon_theme_set_custom_theme      (BtkIconTheme                *icon_theme,
						    const bchar                 *theme_name);

bboolean      btk_icon_theme_has_icon              (BtkIconTheme                *icon_theme,
						    const bchar                 *icon_name);
bint         *btk_icon_theme_get_icon_sizes        (BtkIconTheme                *icon_theme,
						    const bchar                 *icon_name);
BtkIconInfo * btk_icon_theme_lookup_icon           (BtkIconTheme                *icon_theme,
						    const bchar                 *icon_name,
						    bint                         size,
						    BtkIconLookupFlags           flags);
BtkIconInfo * btk_icon_theme_choose_icon           (BtkIconTheme                *icon_theme,
						    const bchar                 *icon_names[],
						    bint                         size,
						    BtkIconLookupFlags           flags);
BdkPixbuf *   btk_icon_theme_load_icon             (BtkIconTheme                *icon_theme,
						    const bchar                 *icon_name,
						    bint                         size,
						    BtkIconLookupFlags           flags,
						    GError                     **error);

BtkIconInfo * btk_icon_theme_lookup_by_gicon       (BtkIconTheme                *icon_theme,
                                                    GIcon                       *icon,
                                                    bint                         size,
                                                    BtkIconLookupFlags           flags);

GList *       btk_icon_theme_list_icons            (BtkIconTheme                *icon_theme,
						    const bchar                 *context);
GList *       btk_icon_theme_list_contexts         (BtkIconTheme                *icon_theme);
char *        btk_icon_theme_get_example_icon_name (BtkIconTheme                *icon_theme);

bboolean      btk_icon_theme_rescan_if_needed      (BtkIconTheme                *icon_theme);

void          btk_icon_theme_add_builtin_icon      (const bchar *icon_name,
					            bint         size,
					            BdkPixbuf   *pixbuf);

GType                 btk_icon_info_get_type           (void) B_GNUC_CONST;
BtkIconInfo *         btk_icon_info_copy               (BtkIconInfo  *icon_info);
void                  btk_icon_info_free               (BtkIconInfo  *icon_info);

BtkIconInfo *         btk_icon_info_new_for_pixbuf     (BtkIconTheme  *icon_theme,
                                                        BdkPixbuf     *pixbuf);

bint                  btk_icon_info_get_base_size      (BtkIconInfo   *icon_info);
const bchar *         btk_icon_info_get_filename       (BtkIconInfo   *icon_info);
BdkPixbuf *           btk_icon_info_get_builtin_pixbuf (BtkIconInfo   *icon_info);
BdkPixbuf *           btk_icon_info_load_icon          (BtkIconInfo   *icon_info,
							GError       **error);
void                  btk_icon_info_set_raw_coordinates (BtkIconInfo  *icon_info,
							 bboolean      raw_coordinates);

bboolean              btk_icon_info_get_embedded_rect (BtkIconInfo    *icon_info,
						       BdkRectangle   *rectangle);
bboolean              btk_icon_info_get_attach_points (BtkIconInfo    *icon_info,
						       BdkPoint      **points,
						       bint           *n_points);
const bchar *         btk_icon_info_get_display_name  (BtkIconInfo    *icon_info);

/* Non-public methods */
void _btk_icon_theme_check_reload                     (BdkDisplay *display);
void _btk_icon_theme_ensure_builtin_cache             (void);

B_END_DECLS

#endif /* __BTK_ICON_THEME_H__ */
