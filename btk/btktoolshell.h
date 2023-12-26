/* BTK - The GIMP Toolkit
 * Copyright (C) 2007  Openismus GmbH
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
 *
 * Author:
 *   Mathias Hasselmann
 */

#ifndef __BTK_TOOL_SHELL_H__
#define __BTK_TOOL_SHELL_H__


#if !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkenums.h>
#include <bango/bango.h>
#include <btk/btksizegroup.h>


B_BEGIN_DECLS

#define BTK_TYPE_TOOL_SHELL            (btk_tool_shell_get_type ())
#define BTK_TOOL_SHELL(obj)            (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_TOOL_SHELL, BtkToolShell))
#define BTK_IS_TOOL_SHELL(obj)         (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_TOOL_SHELL))
#define BTK_TOOL_SHELL_GET_IFACE(obj)  (B_TYPE_INSTANCE_GET_INTERFACE ((obj), BTK_TYPE_TOOL_SHELL, BtkToolShellIface))

typedef struct _BtkToolShell           BtkToolShell; /* dummy typedef */
typedef struct _BtkToolShellIface      BtkToolShellIface;

/**
 * BtkToolShellIface:
 * @get_icon_size:        mandatory implementation of btk_tool_shell_get_icon_size().
 * @get_orientation:      mandatory implementation of btk_tool_shell_get_orientation().
 * @get_style:            mandatory implementation of btk_tool_shell_get_style().
 * @get_relief_style:     optional implementation of btk_tool_shell_get_relief_style().
 * @rebuild_menu:         optional implementation of btk_tool_shell_rebuild_menu().
 * @get_text_orientation: optional implementation of btk_tool_shell_get_text_orientation().
 * @get_text_alignment:   optional implementation of btk_tool_shell_get_text_alignment().
 * @get_ellipsize_mode:   optional implementation of btk_tool_shell_get_ellipsize_mode().
 * @get_text_size_group:  optional implementation of btk_tool_shell_get_text_size_group().
 *
 * Virtual function table for the #BtkToolShell interface.
 */
struct _BtkToolShellIface
{
  /*< private >*/
  GTypeInterface g_iface;

  /*< public >*/
  BtkIconSize        (*get_icon_size)        (BtkToolShell *shell);
  BtkOrientation     (*get_orientation)      (BtkToolShell *shell);
  BtkToolbarStyle    (*get_style)            (BtkToolShell *shell);
  BtkReliefStyle     (*get_relief_style)     (BtkToolShell *shell);
  void               (*rebuild_menu)         (BtkToolShell *shell);
  BtkOrientation     (*get_text_orientation) (BtkToolShell *shell);
  gfloat             (*get_text_alignment)   (BtkToolShell *shell);
  BangoEllipsizeMode (*get_ellipsize_mode)   (BtkToolShell *shell);
  BtkSizeGroup *     (*get_text_size_group)  (BtkToolShell *shell);
};

GType              btk_tool_shell_get_type             (void) B_GNUC_CONST;

BtkIconSize        btk_tool_shell_get_icon_size        (BtkToolShell *shell);
BtkOrientation     btk_tool_shell_get_orientation      (BtkToolShell *shell);
BtkToolbarStyle    btk_tool_shell_get_style            (BtkToolShell *shell);
BtkReliefStyle     btk_tool_shell_get_relief_style     (BtkToolShell *shell);
void               btk_tool_shell_rebuild_menu         (BtkToolShell *shell);
BtkOrientation     btk_tool_shell_get_text_orientation (BtkToolShell *shell);
gfloat             btk_tool_shell_get_text_alignment   (BtkToolShell *shell);
BangoEllipsizeMode btk_tool_shell_get_ellipsize_mode   (BtkToolShell *shell);
BtkSizeGroup *     btk_tool_shell_get_text_size_group  (BtkToolShell *shell);

B_END_DECLS

#endif /* __BTK_TOOL_SHELL_H__ */
