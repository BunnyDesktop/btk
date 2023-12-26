/* bdkglobals-quartz.c
 *
 * Copyright (C) 2005 Imendio AB
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

#include "config.h"
#include "bdktypes.h"
#include "bdkprivate.h"
#include "bdkquartz.h"

BdkDisplay *_bdk_display = NULL;
BdkScreen *_bdk_screen = NULL;
BdkWindow *_bdk_root = NULL;

BdkOSXVersion
bdk_quartz_osx_version (void)
{
  static bint32 minor = BDK_OSX_UNSUPPORTED;

  if (minor == BDK_OSX_UNSUPPORTED)
    {
      OSErr err = Gestalt (gestaltSystemVersionMinor, (SInt32*)&minor);

      g_return_val_if_fail (err == noErr, BDK_OSX_UNSUPPORTED);
    }

  if (minor < BDK_OSX_MIN)
    return BDK_OSX_UNSUPPORTED;
  else if (minor > BDK_OSX_CURRENT)
    return BDK_OSX_NEW;
  else
    return minor;
}
