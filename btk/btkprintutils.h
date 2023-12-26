/* BTK - The GIMP Toolkit
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

#ifndef __BTK_PRINT_UTILS_H__
#define __BTK_PRINT_UTILS_H__


#include "btkenums.h"


B_BEGIN_DECLS

#define MM_PER_INCH 25.4
#define POINTS_PER_INCH 72

bdouble _btk_print_convert_to_mm   (bdouble len, BtkUnit unit);
bdouble _btk_print_convert_from_mm (bdouble len, BtkUnit unit);

B_END_DECLS

#endif /* __BTK_PRINT_UTILS_H__ */
