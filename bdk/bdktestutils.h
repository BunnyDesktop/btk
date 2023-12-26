/* Bdk testing utilities
 * Copyright (C) 2007 Imendio AB
 * Authors: Tim Janik
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

#ifndef __BDK_TEST_UTILS_H__
#define __BDK_TEST_UTILS_H__

#if !defined (__BDK_H_INSIDE__) && !defined (BDK_COMPILATION)
#error "Only <bdk/bdk.h> can be included directly."
#endif

#include <bdk/bdkwindow.h>

B_BEGIN_DECLS

/* --- Bdk Test Utility API --- */
void            bdk_test_render_sync            (BdkWindow      *window);
bboolean        bdk_test_simulate_key           (BdkWindow      *window,
                                                 bint            x,
                                                 bint            y,
                                                 buint           keyval,
                                                 BdkModifierType modifiers,
                                                 BdkEventType    key_pressrelease);
bboolean        bdk_test_simulate_button        (BdkWindow      *window,
                                                 bint            x,
                                                 bint            y,
                                                 buint           button, /*1..3*/
                                                 BdkModifierType modifiers,
                                                 BdkEventType    button_pressrelease);

B_END_DECLS

#endif /* __BDK_TEST_UTILS_H__ */
