/* BAIL - The BUNNY Accessibility Implementation Library
 * Copyright 2001 Sun Microsystems Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */


#ifndef __BAIL_PRIVATE_MACROS_H__
#define __BAIL_PRIVATE_MACROS_H__

B_BEGIN_DECLS

/* Note: these macros are logic macros, not intended to warn on failure. */

#define bail_return_val_if_fail(a, b)  if (!(a)) return (b)
#define bail_return_if_fail(a) if (!(a)) return

B_END_DECLS

#endif /* __BAIL_PRIVATE_MACROS_H__ */
