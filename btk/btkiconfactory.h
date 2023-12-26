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

#ifndef __BTK_ICON_FACTORY_H__
#define __BTK_ICON_FACTORY_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <bdk/bdk.h>
#include <btk/btkrc.h>

B_BEGIN_DECLS

typedef struct _BtkIconFactoryClass BtkIconFactoryClass;

#define BTK_TYPE_ICON_FACTORY              (btk_icon_factory_get_type ())
#define BTK_ICON_FACTORY(object)           (B_TYPE_CHECK_INSTANCE_CAST ((object), BTK_TYPE_ICON_FACTORY, BtkIconFactory))
#define BTK_ICON_FACTORY_CLASS(klass)      (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_ICON_FACTORY, BtkIconFactoryClass))
#define BTK_IS_ICON_FACTORY(object)        (B_TYPE_CHECK_INSTANCE_TYPE ((object), BTK_TYPE_ICON_FACTORY))
#define BTK_IS_ICON_FACTORY_CLASS(klass)   (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_ICON_FACTORY))
#define BTK_ICON_FACTORY_GET_CLASS(obj)    (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_ICON_FACTORY, BtkIconFactoryClass))
#define BTK_TYPE_ICON_SET                  (btk_icon_set_get_type ())
#define BTK_TYPE_ICON_SOURCE               (btk_icon_source_get_type ())

struct _BtkIconFactory
{
  BObject parent_instance;

  GHashTable *GSEAL (icons);
};

struct _BtkIconFactoryClass
{
  BObjectClass parent_class;

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};

#ifdef G_OS_WIN32
/* Reserve old names for DLL ABI backward compatibility */
#define btk_icon_source_set_filename btk_icon_source_set_filename_utf8
#define btk_icon_source_get_filename btk_icon_source_get_filename_utf8
#endif

GType           btk_icon_factory_get_type (void) B_GNUC_CONST;
BtkIconFactory* btk_icon_factory_new      (void);
void            btk_icon_factory_add      (BtkIconFactory *factory,
                                           const gchar    *stock_id,
                                           BtkIconSet     *icon_set);
BtkIconSet*     btk_icon_factory_lookup   (BtkIconFactory *factory,
                                           const gchar    *stock_id);

/* Manage the default icon factory stack */

void        btk_icon_factory_add_default     (BtkIconFactory  *factory);
void        btk_icon_factory_remove_default  (BtkIconFactory  *factory);
BtkIconSet* btk_icon_factory_lookup_default  (const gchar     *stock_id);

/* Get preferred real size from registered semantic size.  Note that
 * themes SHOULD use this size, but they aren't required to; for size
 * requests and such, you should get the actual pixbuf from the icon
 * set and see what size was rendered.
 *
 * This function is intended for people who are scaling icons,
 * rather than for people who are displaying already-scaled icons.
 * That is, if you are displaying an icon, you should get the
 * size from the rendered pixbuf, not from here.
 */

#ifndef BDK_MULTIHEAD_SAFE
gboolean btk_icon_size_lookup              (BtkIconSize  size,
					    gint        *width,
					    gint        *height);
#endif /* BDK_MULTIHEAD_SAFE */
gboolean btk_icon_size_lookup_for_settings (BtkSettings *settings,
					    BtkIconSize  size,
					    gint        *width,
					    gint        *height);

BtkIconSize           btk_icon_size_register       (const gchar *name,
                                                    gint         width,
                                                    gint         height);
void                  btk_icon_size_register_alias (const gchar *alias,
                                                    BtkIconSize  target);
BtkIconSize           btk_icon_size_from_name      (const gchar *name);
const gchar *         btk_icon_size_get_name       (BtkIconSize  size);

/* Icon sets */

GType       btk_icon_set_get_type        (void) B_GNUC_CONST;
BtkIconSet* btk_icon_set_new             (void);
BtkIconSet* btk_icon_set_new_from_pixbuf (BdkPixbuf       *pixbuf);

BtkIconSet* btk_icon_set_ref             (BtkIconSet      *icon_set);
void        btk_icon_set_unref           (BtkIconSet      *icon_set);
BtkIconSet* btk_icon_set_copy            (BtkIconSet      *icon_set);

/* Get one of the icon variants in the set, creating the variant if
 * necessary.
 */
BdkPixbuf*  btk_icon_set_render_icon     (BtkIconSet      *icon_set,
                                          BtkStyle        *style,
                                          BtkTextDirection direction,
                                          BtkStateType     state,
                                          BtkIconSize      size,
                                          BtkWidget       *widget,
                                          const char      *detail);


void           btk_icon_set_add_source   (BtkIconSet          *icon_set,
                                          const BtkIconSource *source);

void           btk_icon_set_get_sizes    (BtkIconSet          *icon_set,
                                          BtkIconSize        **sizes,
                                          gint                *n_sizes);

GType          btk_icon_source_get_type                 (void) B_GNUC_CONST;
BtkIconSource* btk_icon_source_new                      (void);
BtkIconSource* btk_icon_source_copy                     (const BtkIconSource *source);
void           btk_icon_source_free                     (BtkIconSource       *source);

void           btk_icon_source_set_filename             (BtkIconSource       *source,
                                                         const gchar         *filename);
void           btk_icon_source_set_icon_name            (BtkIconSource       *source,
                                                         const gchar         *icon_name);
void           btk_icon_source_set_pixbuf               (BtkIconSource       *source,
                                                         BdkPixbuf           *pixbuf);

const gchar* btk_icon_source_get_filename  (const BtkIconSource *source);
const gchar* btk_icon_source_get_icon_name (const BtkIconSource *source);
BdkPixbuf*            btk_icon_source_get_pixbuf    (const BtkIconSource *source);

void             btk_icon_source_set_direction_wildcarded (BtkIconSource       *source,
                                                           gboolean             setting);
void             btk_icon_source_set_state_wildcarded     (BtkIconSource       *source,
                                                           gboolean             setting);
void             btk_icon_source_set_size_wildcarded      (BtkIconSource       *source,
                                                           gboolean             setting);
gboolean         btk_icon_source_get_size_wildcarded      (const BtkIconSource *source);
gboolean         btk_icon_source_get_state_wildcarded     (const BtkIconSource *source);
gboolean         btk_icon_source_get_direction_wildcarded (const BtkIconSource *source);
void             btk_icon_source_set_direction            (BtkIconSource       *source,
                                                           BtkTextDirection     direction);
void             btk_icon_source_set_state                (BtkIconSource       *source,
                                                           BtkStateType         state);
void             btk_icon_source_set_size                 (BtkIconSource       *source,
                                                           BtkIconSize          size);
BtkTextDirection btk_icon_source_get_direction            (const BtkIconSource *source);
BtkStateType     btk_icon_source_get_state                (const BtkIconSource *source);
BtkIconSize      btk_icon_source_get_size                 (const BtkIconSource *source);


/* ignore this */
void _btk_icon_set_invalidate_caches (void);
GList* _btk_icon_factory_list_ids (void);
void _btk_icon_factory_ensure_default_icons (void);

B_END_DECLS

#endif /* __BTK_ICON_FACTORY_H__ */
