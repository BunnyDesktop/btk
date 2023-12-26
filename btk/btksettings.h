/* BTK - The GIMP Toolkit
 * Copyright (C) 2000 Red Hat, Inc.
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

#ifndef __BTK_SETTINGS_H__
#define __BTK_SETTINGS_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkrc.h>

G_BEGIN_DECLS


/* -- type macros --- */
#define BTK_TYPE_SETTINGS             (btk_settings_get_type ())
#define BTK_SETTINGS(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_SETTINGS, BtkSettings))
#define BTK_SETTINGS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_SETTINGS, BtkSettingsClass))
#define BTK_IS_SETTINGS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_SETTINGS))
#define BTK_IS_SETTINGS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_SETTINGS))
#define BTK_SETTINGS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_SETTINGS, BtkSettingsClass))


/* --- typedefs --- */
typedef struct    _BtkSettingsClass BtkSettingsClass;
typedef struct    _BtkSettingsValue BtkSettingsValue;
typedef struct    _BtkSettingsPropertyValue BtkSettingsPropertyValue; /* Internal */


/* --- structures --- */
struct _BtkSettings
{
  GObject parent_instance;

  GData  *GSEAL (queued_settings);	/* of type BtkSettingsValue* */
  BtkSettingsPropertyValue *GSEAL (property_values);

  BtkRcContext *GSEAL (rc_context);
  BdkScreen    *GSEAL (screen);
};

struct _BtkSettingsClass
{
  GObjectClass parent_class;
};

struct _BtkSettingsValue
{
  /* origin should be something like "filename:linenumber" for rc files,
   * or e.g. "XProperty" for other sources
   */
  gchar *origin;

  /* valid types are LONG, DOUBLE and STRING corresponding to the token parsed,
   * or a GSTRING holding an unparsed statement
   */
  GValue value;
};


/* --- functions --- */
GType		btk_settings_get_type		     (void) G_GNUC_CONST;
#ifndef BDK_MULTIHEAD_SAFE
BtkSettings*	btk_settings_get_default	     (void);
#endif
BtkSettings*	btk_settings_get_for_screen	     (BdkScreen *screen);

void		btk_settings_install_property	     (GParamSpec         *pspec);
void		btk_settings_install_property_parser (GParamSpec         *pspec,
						      BtkRcPropertyParser parser);

/* --- precoded parsing functions --- */
gboolean btk_rc_property_parse_color       (const GParamSpec *pspec,
					    const GString    *gstring,
					    GValue           *property_value);
gboolean btk_rc_property_parse_enum        (const GParamSpec *pspec,
					    const GString    *gstring,
					    GValue           *property_value);
gboolean btk_rc_property_parse_flags       (const GParamSpec *pspec,
					    const GString    *gstring,
					    GValue           *property_value);
gboolean btk_rc_property_parse_requisition (const GParamSpec *pspec,
					    const GString    *gstring,
					    GValue           *property_value);
gboolean btk_rc_property_parse_border      (const GParamSpec *pspec,
					    const GString    *gstring,
					    GValue           *property_value);

/*< private >*/
void		btk_settings_set_property_value	 (BtkSettings	*settings,
						  const gchar	*name,
						  const BtkSettingsValue *svalue);
void		btk_settings_set_string_property (BtkSettings	*settings,
						  const gchar	*name,
						  const gchar	*v_string,
						  const gchar   *origin);
void		btk_settings_set_long_property	 (BtkSettings	*settings,
						  const gchar	*name,
						  glong		 v_long,
						  const gchar   *origin);
void		btk_settings_set_double_property (BtkSettings	*settings,
						  const gchar	*name,
						  gdouble	 v_double,
						  const gchar   *origin);


/* implementation details */
void _btk_settings_set_property_value_from_rc (BtkSettings            *settings,
					       const gchar            *name,
					       const BtkSettingsValue *svalue);
void _btk_settings_reset_rc_values            (BtkSettings            *settings);

void                _btk_settings_handle_event        (BdkEventSetting    *event);
BtkRcPropertyParser _btk_rc_property_parser_from_type (GType               type);
gboolean	    _btk_settings_parse_convert       (BtkRcPropertyParser parser,
						       const GValue       *src_value,
						       GParamSpec         *pspec,
						       GValue	          *dest_value);


G_END_DECLS

#endif /* __BTK_SETTINGS_H__ */
