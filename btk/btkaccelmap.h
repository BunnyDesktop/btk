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

B_BEGIN_DECLS

/* --- global BtkAccelMap object --- */
#define BTK_TYPE_ACCEL_MAP                (btk_accel_map_get_type ())
#define BTK_ACCEL_MAP(accel_map)	  (B_TYPE_CHECK_INSTANCE_CAST ((accel_map), BTK_TYPE_ACCEL_MAP, BtkAccelMap))
#define BTK_ACCEL_MAP_CLASS(klass)	  (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_ACCEL_MAP, BtkAccelMapClass))
#define BTK_IS_ACCEL_MAP(accel_map)	  (B_TYPE_CHECK_INSTANCE_TYPE ((accel_map), BTK_TYPE_ACCEL_MAP))
#define BTK_IS_ACCEL_MAP_CLASS(klass)	  (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_ACCEL_MAP))
#define BTK_ACCEL_MAP_GET_CLASS(obj)      (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_ACCEL_MAP, BtkAccelMapClass))

typedef struct _BtkAccelMap      BtkAccelMap;
typedef struct _BtkAccelMapClass BtkAccelMapClass;

/* --- notifier --- */
typedef void (*BtkAccelMapForeach)		(bpointer	 data,
						 const bchar	*accel_path,
						 buint           accel_key,
						 BdkModifierType accel_mods,
						 bboolean	 changed);


/* --- public API --- */

#ifdef G_OS_WIN32
/* Reserve old names for DLL ABI backward compatibility */
#define btk_accel_map_load btk_accel_map_load_utf8
#define btk_accel_map_save btk_accel_map_save_utf8
#endif

void	   btk_accel_map_add_entry	(const bchar		*accel_path,
					 buint			 accel_key,
					 BdkModifierType         accel_mods);
bboolean   btk_accel_map_lookup_entry	(const bchar		*accel_path,
					 BtkAccelKey		*key);
bboolean   btk_accel_map_change_entry	(const bchar		*accel_path,
					 buint			 accel_key,
					 BdkModifierType	 accel_mods,
					 bboolean		 replace);
void	   btk_accel_map_load		(const bchar		*file_name);
void	   btk_accel_map_save		(const bchar		*file_name);
void	   btk_accel_map_foreach	(bpointer		 data,
					 BtkAccelMapForeach	 foreach_func);
void	   btk_accel_map_load_fd	(bint			 fd);
void	   btk_accel_map_load_scanner	(GScanner		*scanner);
void	   btk_accel_map_save_fd	(bint			 fd);

void       btk_accel_map_lock_path      (const bchar            *accel_path);
void       btk_accel_map_unlock_path    (const bchar            *accel_path);

/* --- filter functions --- */
void	btk_accel_map_add_filter	 (const bchar		*filter_pattern);
void	btk_accel_map_foreach_unfiltered (bpointer		 data,
					  BtkAccelMapForeach	 foreach_func);

/* --- notification --- */
GType        btk_accel_map_get_type (void) B_GNUC_CONST;
BtkAccelMap *btk_accel_map_get      (void);


/* --- internal API --- */
void		_btk_accel_map_init		(void);

void            _btk_accel_map_add_group	 (const bchar   *accel_path,
						  BtkAccelGroup *accel_group);
void            _btk_accel_map_remove_group 	 (const bchar   *accel_path,
						  BtkAccelGroup *accel_group);
bboolean	_btk_accel_path_is_valid	 (const bchar	*accel_path);


B_END_DECLS

#endif /* __BTK_ACCEL_MAP_H__ */
