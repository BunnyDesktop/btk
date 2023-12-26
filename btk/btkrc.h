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

#ifndef __BTK_RC_H__
#define __BTK_RC_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkstyle.h>

G_BEGIN_DECLS

/* Forward declarations */
typedef struct _BtkIconFactory  BtkIconFactory;
typedef struct _BtkRcContext    BtkRcContext;

typedef struct _BtkRcStyleClass BtkRcStyleClass;

#define BTK_TYPE_RC_STYLE              (btk_rc_style_get_type ())
#define BTK_RC_STYLE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BTK_TYPE_RC_STYLE, BtkRcStyle))
#define BTK_RC_STYLE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_RC_STYLE, BtkRcStyleClass))
#define BTK_IS_RC_STYLE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BTK_TYPE_RC_STYLE))
#define BTK_IS_RC_STYLE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_RC_STYLE))
#define BTK_RC_STYLE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_RC_STYLE, BtkRcStyleClass))

typedef enum
{
  BTK_RC_FG		= 1 << 0,
  BTK_RC_BG		= 1 << 1,
  BTK_RC_TEXT		= 1 << 2,
  BTK_RC_BASE		= 1 << 3
} BtkRcFlags;

struct _BtkRcStyle
{
  GObject parent_instance;

  /*< public >*/

  gchar *name;
  gchar *bg_pixmap_name[5];
  BangoFontDescription *font_desc;

  BtkRcFlags color_flags[5];
  BdkColor   fg[5];
  BdkColor   bg[5];
  BdkColor   text[5];
  BdkColor   base[5];

  gint xthickness;
  gint ythickness;

  /*< private >*/
  GArray *rc_properties;

  /* list of RC style lists including this RC style */
  GSList *rc_style_lists;

  GSList *icon_factories;

  guint engine_specified : 1;	/* The RC file specified the engine */
};

struct _BtkRcStyleClass
{
  GObjectClass parent_class;

  /* Create an empty RC style of the same type as this RC style.
   * The default implementation, which does
   * g_object_new (G_OBJECT_TYPE (style), NULL);
   * should work in most cases.
   */
  BtkRcStyle * (*create_rc_style) (BtkRcStyle *rc_style);

  /* Fill in engine specific parts of BtkRcStyle by parsing contents
   * of brackets. Returns G_TOKEN_NONE if successful, otherwise returns
   * the token it expected but didn't get.
   */
  guint     (*parse)  (BtkRcStyle   *rc_style,
		       BtkSettings  *settings,
		       GScanner     *scanner);

  /* Combine RC style data from src into dest. If overridden, this
   * function should chain to the parent.
   */
  void      (*merge)  (BtkRcStyle *dest,
		       BtkRcStyle *src);

  /* Create an empty style suitable to this RC style
   */
  BtkStyle * (*create_style) (BtkRcStyle *rc_style);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};

#ifdef G_OS_WIN32
/* Reserve old names for DLL ABI backward compatibility */
#define btk_rc_add_default_file btk_rc_add_default_file_utf8
#define btk_rc_set_default_files btk_rc_set_default_files_utf8
#define btk_rc_parse btk_rc_parse_utf8
#endif

void	  _btk_rc_init			 (void);
GSList*   _btk_rc_parse_widget_class_path (const gchar *pattern);
void      _btk_rc_free_widget_class_path (GSList       *list);
gboolean  _btk_rc_match_widget_class     (GSList       *list,
                                          gint          length,
                                          gchar        *path,
                                          gchar        *path_reversed);

void      btk_rc_add_default_file	(const gchar *filename);
void      btk_rc_set_default_files      (gchar **filenames);
gchar**   btk_rc_get_default_files      (void);
BtkStyle* btk_rc_get_style		(BtkWidget   *widget);
BtkStyle* btk_rc_get_style_by_paths     (BtkSettings *settings,
					 const char  *widget_path,
					 const char  *class_path,
					 GType        type);

gboolean btk_rc_reparse_all_for_settings (BtkSettings *settings,
					  gboolean     force_load);
void     btk_rc_reset_styles             (BtkSettings *settings);

gchar*   btk_rc_find_pixmap_in_path (BtkSettings  *settings,
				     GScanner     *scanner,
				     const gchar  *pixmap_file);

void	  btk_rc_parse			(const gchar *filename);
void	  btk_rc_parse_string		(const gchar *rc_string);
gboolean  btk_rc_reparse_all		(void);

#ifndef BTK_DISABLE_DEPRECATED
void	  btk_rc_add_widget_name_style	(BtkRcStyle   *rc_style,
					 const gchar  *pattern);
void	  btk_rc_add_widget_class_style (BtkRcStyle   *rc_style,
					 const gchar  *pattern);
void	  btk_rc_add_class_style	(BtkRcStyle   *rc_style,
					 const gchar  *pattern);
#endif /* BTK_DISABLE_DEPRECATED */


GType       btk_rc_style_get_type   (void) G_GNUC_CONST;
BtkRcStyle* btk_rc_style_new        (void);
BtkRcStyle* btk_rc_style_copy       (BtkRcStyle *orig);

#ifndef BTK_DISABLE_DEPRECATED
void        btk_rc_style_ref        (BtkRcStyle *rc_style);
void        btk_rc_style_unref      (BtkRcStyle *rc_style);
#endif

gchar*		btk_rc_find_module_in_path	(const gchar 	*module_file);
gchar*		btk_rc_get_theme_dir		(void);
gchar*		btk_rc_get_module_dir		(void);
gchar*		btk_rc_get_im_module_path	(void);
gchar*		btk_rc_get_im_module_file	(void);

/* private functions/definitions */
typedef enum {
  BTK_RC_TOKEN_INVALID = G_TOKEN_LAST,
  BTK_RC_TOKEN_INCLUDE,
  BTK_RC_TOKEN_NORMAL,
  BTK_RC_TOKEN_ACTIVE,
  BTK_RC_TOKEN_PRELIGHT,
  BTK_RC_TOKEN_SELECTED,
  BTK_RC_TOKEN_INSENSITIVE,
  BTK_RC_TOKEN_FG,
  BTK_RC_TOKEN_BG,
  BTK_RC_TOKEN_TEXT,
  BTK_RC_TOKEN_BASE,
  BTK_RC_TOKEN_XTHICKNESS,
  BTK_RC_TOKEN_YTHICKNESS,
  BTK_RC_TOKEN_FONT,
  BTK_RC_TOKEN_FONTSET,
  BTK_RC_TOKEN_FONT_NAME,
  BTK_RC_TOKEN_BG_PIXMAP,
  BTK_RC_TOKEN_PIXMAP_PATH,
  BTK_RC_TOKEN_STYLE,
  BTK_RC_TOKEN_BINDING,
  BTK_RC_TOKEN_BIND,
  BTK_RC_TOKEN_WIDGET,
  BTK_RC_TOKEN_WIDGET_CLASS,
  BTK_RC_TOKEN_CLASS,
  BTK_RC_TOKEN_LOWEST,
  BTK_RC_TOKEN_BTK,
  BTK_RC_TOKEN_APPLICATION,
  BTK_RC_TOKEN_THEME,
  BTK_RC_TOKEN_RC,
  BTK_RC_TOKEN_HIGHEST,
  BTK_RC_TOKEN_ENGINE,
  BTK_RC_TOKEN_MODULE_PATH,
  BTK_RC_TOKEN_IM_MODULE_PATH,
  BTK_RC_TOKEN_IM_MODULE_FILE,
  BTK_RC_TOKEN_STOCK,
  BTK_RC_TOKEN_LTR,
  BTK_RC_TOKEN_RTL,
  BTK_RC_TOKEN_COLOR,
  BTK_RC_TOKEN_UNBIND,
  BTK_RC_TOKEN_LAST
} BtkRcTokenType;

GScanner* btk_rc_scanner_new	(void);
guint	  btk_rc_parse_color	(GScanner	     *scanner,
				 BdkColor	     *color);
guint	  btk_rc_parse_color_full (GScanner	     *scanner,
                                   BtkRcStyle        *style,
				   BdkColor	     *color);
guint	  btk_rc_parse_state	(GScanner	     *scanner,
				 BtkStateType	     *state);
guint	  btk_rc_parse_priority	(GScanner	     *scanner,
				 BtkPathPriorityType *priority);

/* rc properties
 * (structure forward declared in btkstyle.h)
 */
struct _BtkRcProperty
{
  /* quark-ified property identifier like "BtkScrollbar::spacing" */
  GQuark type_name;
  GQuark property_name;

  /* fields similar to BtkSettingsValue */
  gchar *origin;
  GValue value;
};
const BtkRcProperty* _btk_rc_style_lookup_rc_property (BtkRcStyle *rc_style,
						       GQuark      type_name,
						       GQuark      property_name);
void	      _btk_rc_style_set_rc_property	      (BtkRcStyle *rc_style,
						       BtkRcProperty *property);
void	      _btk_rc_style_unset_rc_property	      (BtkRcStyle *rc_style,
						       GQuark      type_name,
						       GQuark      property_name);

GSList     * _btk_rc_style_get_color_hashes        (BtkRcStyle *rc_style);

const gchar* _btk_rc_context_get_default_font_name (BtkSettings *settings);
void         _btk_rc_context_destroy               (BtkSettings *settings);

G_END_DECLS

#endif /* __BTK_RC_H__ */
