/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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
 * Modified by the BTK+ Team and others 1997-1999.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/.
 */

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#ifndef __BTK_VERSION_H__
#define __BTK_VERSION_H__

/* compile time version
 */
#define BTK_MAJOR_VERSION				(2)
#define BTK_MINOR_VERSION				(24)
#define BTK_MICRO_VERSION				(33)
#define BTK_BINARY_AGE					(2433)
#define BTK_INTERFACE_AGE				(33)

/* check whether a Btk+ version equal to or greater than
 * major.minor.micro is present.
 */
#define	BTK_CHECK_VERSION(major,minor,micro)	\
    (BTK_MAJOR_VERSION > (major) || \
     (BTK_MAJOR_VERSION == (major) && BTK_MINOR_VERSION > (minor)) || \
     (BTK_MAJOR_VERSION == (major) && BTK_MINOR_VERSION == (minor) && \
      BTK_MICRO_VERSION >= (micro)))

#endif /* __BTK_VERSION_H__ */
