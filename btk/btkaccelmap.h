/* BTK - The GIMP Toolkit
 * Copyright (C) 1998, 2001 Tim Janik
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

#ifndef __BTK_ACCEL_MAP_H__
#define __BTK_ACCEL_MAP_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkaccelgroup.h>

G_BEGIN_DECLS

/* --- global BtkAccelMap object --- */
#define BTK_TYPE_ACCEL_MAP                (btk_accel_map_get_type ())
#define BTK_ACCEL_MAP(accel_map)	  (G_TYPE_CHECK_INSTANCE_CAST ((accel_map), BTK_TYPE_ACCEL_MAP, BtkAccelMap))
#define BTK_ACCEL_MAP_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_ACCEL_MAP, BtkAccelMapClass))
#define BTK_IS_ACCEL_MAP(accel_map)	  (G_TYPE_CHECK_INSTANCE_TYPE ((accel_map), BTK_TYPE_ACCEL_MAP))
#define BTK_IS_ACCEL_MAP_CLASS(klass)	  (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_ACCEL_MAP))
#define BTK_ACCEL_MAP_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_ACCEL_MAP, BtkAccelMapClass))

typedef struct _BtkAccelMap      BtkAccelMap;
typedef struct _BtkAccelMapClass BtkAccelMapClass;

/* --- notifier --- */
typedef void (*BtkAccelMapForeach)		(gpointer	 data,
						 const gchar	*accel_path,
						 guint           accel_key,
						 BdkModifierType accel_mods,
						 gboolean	 changed);


/* --- public API --- */

#ifdef G_OS_WIN32
/* Reserve old names for DLL ABI backward compatibility */
#define btk_accel_map_load btk_accel_map_load_utf8
#define btk_accel_map_save btk_accel_map_save_utf8
#endif

void	   btk_accel_map_add_entry	(const gchar		*accel_path,
					 guint			 accel_key,
					 BdkModifierType         accel_mods);
gboolean   btk_accel_map_lookup_entry	(const gchar		*accel_path,
					 BtkAccelKey		*key);
gboolean   btk_accel_map_change_entry	(const gchar		*accel_path,
					 guint			 accel_key,
					 BdkModifierType	 accel_mods,
					 gboolean		 replace);
void	   btk_accel_map_load		(const gchar		*file_name);
void	   btk_accel_map_save		(const gchar		*file_name);
void	   btk_accel_map_foreach	(gpointer		 data,
					 BtkAccelMapForeach	 foreach_func);
void	   btk_accel_map_load_fd	(gint			 fd);
void	   btk_accel_map_load_scanner	(GScanner		*scanner);
void	   btk_accel_map_save_fd	(gint			 fd);

void       btk_accel_map_lock_path      (const gchar            *accel_path);
void       btk_accel_map_unlock_path    (const gchar            *accel_path);

/* --- filter functions --- */
void	btk_accel_map_add_filter	 (const gchar		*filter_pattern);
void	btk_accel_map_foreach_unfiltered (gpointer		 data,
					  BtkAccelMapForeach	 foreach_func);

/* --- notification --- */
GType        btk_accel_map_get_type (void) G_GNUC_CONST;
BtkAccelMap *btk_accel_map_get      (void);


/* --- internal API --- */
void		_btk_accel_map_init		(void);

void            _btk_accel_map_add_group	 (const gchar   *accel_path,
						  BtkAccelGroup *accel_group);
void            _btk_accel_map_remove_group 	 (const gchar   *accel_path,
						  BtkAccelGroup *accel_group);
gboolean	_btk_accel_path_is_valid	 (const gchar	*accel_path);


G_END_DECLS

#endif /* __BTK_ACCEL_MAP_H__ */
