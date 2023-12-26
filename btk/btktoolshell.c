/* btktoolshell.c
 * Copyright (C) 2007  Openismus GmbH
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
 *
 * Author:
 *   Mathias Hasselmann
 */

#include "config.h"
#include "btktoolshell.h"
#include "btkwidget.h"
#include "btkintl.h"
#include "btkalias.h"

/**
 * SECTION:btktoolshell
 * @Short_description: Interface for containers containing BtkToolItem widgets
 * @Title: BtkToolShell
 *
 * The #BtkToolShell interface allows container widgets to provide additional
 * information when embedding #BtkToolItem widgets.
 *
 * @see_also: #BtkToolbar, #BtkToolItem
 */

/**
 * BtkToolShell:
 *
 * Dummy structure for accessing instances of #BtkToolShellIface.
 */

GType
btk_tool_shell_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      type = g_type_register_static_simple (G_TYPE_INTERFACE, I_("BtkToolShell"),
                                            sizeof (BtkToolShellIface),
                                            NULL, 0, NULL, 0);
      g_type_interface_add_prerequisite (type, BTK_TYPE_WIDGET);
    }

  return type;
}

/**
 * btk_tool_shell_get_icon_size:
 * @shell: a #BtkToolShell
 *
 * Retrieves the icon size for the tool shell. Tool items must not call this
 * function directly, but rely on btk_tool_item_get_icon_size() instead.
 *
 * Return value: (type int): the current size for icons of @shell
 *
 * Since: 2.14
 **/
BtkIconSize
btk_tool_shell_get_icon_size (BtkToolShell *shell)
{
  return BTK_TOOL_SHELL_GET_IFACE (shell)->get_icon_size (shell);
}

/**
 * btk_tool_shell_get_orientation:
 * @shell: a #BtkToolShell
 *
 * Retrieves the current orientation for the tool shell. Tool items must not
 * call this function directly, but rely on btk_tool_item_get_orientation()
 * instead.
 *
 * Return value: the current orientation of @shell
 *
 * Since: 2.14
 **/
BtkOrientation
btk_tool_shell_get_orientation (BtkToolShell *shell)
{
  return BTK_TOOL_SHELL_GET_IFACE (shell)->get_orientation (shell);
}

/**
 * btk_tool_shell_get_style:
 * @shell: a #BtkToolShell
 *
 * Retrieves whether the tool shell has text, icons, or both. Tool items must
 * not call this function directly, but rely on btk_tool_item_get_style()
 * instead.
 *
 * Return value: the current style of @shell
 *
 * Since: 2.14
 **/
BtkToolbarStyle
btk_tool_shell_get_style (BtkToolShell *shell)
{
  return BTK_TOOL_SHELL_GET_IFACE (shell)->get_style (shell);
}

/**
 * btk_tool_shell_get_relief_style:
 * @shell: a #BtkToolShell
 *
 * Returns the relief style of buttons on @shell. Tool items must not call this
 * function directly, but rely on btk_tool_item_get_relief_style() instead.
 *
 * Return value: The relief style of buttons on @shell.
 *
 * Since: 2.14
 **/
BtkReliefStyle
btk_tool_shell_get_relief_style (BtkToolShell *shell)
{
  BtkToolShellIface *iface = BTK_TOOL_SHELL_GET_IFACE (shell);

  if (iface->get_relief_style)
    return iface->get_relief_style (shell);

  return BTK_RELIEF_NONE;
}

/**
 * btk_tool_shell_rebuild_menu:
 * @shell: a #BtkToolShell
 *
 * Calling this function signals the tool shell that the overflow menu item for
 * tool items have changed. If there is an overflow menu and if it is visible
 * when this function it called, the menu will be rebuilt.
 *
 * Tool items must not call this function directly, but rely on
 * btk_tool_item_rebuild_menu() instead.
 *
 * Since: 2.14
 **/
void
btk_tool_shell_rebuild_menu (BtkToolShell *shell)
{
  BtkToolShellIface *iface = BTK_TOOL_SHELL_GET_IFACE (shell);

  if (iface->rebuild_menu)
    iface->rebuild_menu (shell);
}

/**
 * btk_tool_shell_get_text_orientation:
 * @shell: a #BtkToolShell
 *
 * Retrieves the current text orientation for the tool shell. Tool items must not
 * call this function directly, but rely on btk_tool_item_get_text_orientation()
 * instead.
 *
 * Return value: the current text orientation of @shell
 *
 * Since: 2.20
 **/
BtkOrientation
btk_tool_shell_get_text_orientation (BtkToolShell *shell)
{
  BtkToolShellIface *iface = BTK_TOOL_SHELL_GET_IFACE (shell);

  if (iface->get_text_orientation)
    return BTK_TOOL_SHELL_GET_IFACE (shell)->get_text_orientation (shell);

  return BTK_ORIENTATION_HORIZONTAL;
}

/**
 * btk_tool_shell_get_text_alignment:
 * @shell: a #BtkToolShell
 *
 * Retrieves the current text alignment for the tool shell. Tool items must not
 * call this function directly, but rely on btk_tool_item_get_text_alignment()
 * instead.
 *
 * Return value: the current text alignment of @shell
 *
 * Since: 2.20
 **/
gfloat
btk_tool_shell_get_text_alignment (BtkToolShell *shell)
{
  BtkToolShellIface *iface = BTK_TOOL_SHELL_GET_IFACE (shell);

  if (iface->get_text_alignment)
    return BTK_TOOL_SHELL_GET_IFACE (shell)->get_text_alignment (shell);

  return 0.5f;
}

/**
 * btk_tool_shell_get_ellipsize_mode
 * @shell: a #BtkToolShell
 *
 * Retrieves the current ellipsize mode for the tool shell. Tool items must not
 * call this function directly, but rely on btk_tool_item_get_ellipsize_mode()
 * instead.
 *
 * Return value: the current ellipsize mode of @shell
 *
 * Since: 2.20
 **/
BangoEllipsizeMode
btk_tool_shell_get_ellipsize_mode (BtkToolShell *shell)
{
  BtkToolShellIface *iface = BTK_TOOL_SHELL_GET_IFACE (shell);

  if (iface->get_ellipsize_mode)
    return BTK_TOOL_SHELL_GET_IFACE (shell)->get_ellipsize_mode (shell);

  return BANGO_ELLIPSIZE_NONE;
}

/**
 * btk_tool_shell_get_text_size_group:
 * @shell: a #BtkToolShell
 *
 * Retrieves the current text size group for the tool shell. Tool items must not
 * call this function directly, but rely on btk_tool_item_get_text_size_group()
 * instead.
 *
 * Return value: (transfer none): the current text size group of @shell
 *
 * Since: 2.20
 **/
BtkSizeGroup *
btk_tool_shell_get_text_size_group (BtkToolShell *shell)
{
  BtkToolShellIface *iface = BTK_TOOL_SHELL_GET_IFACE (shell);

  if (iface->get_text_size_group)
    return BTK_TOOL_SHELL_GET_IFACE (shell)->get_text_size_group (shell);

  return NULL;
}

#define __BTK_TOOL_SHELL_C__
#include "btkaliasdef.c"
