/* BTK - The GIMP Toolkit
 * btkprintoperation.c: Print Operation
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

#ifndef _MSC_VER
#define _WIN32_WINNT 0x0500
#define WINVER _WIN32_WINNT
#endif

#include "config.h"
#include "btkprint-win32.h"
#include "btkalias.h"

void
btk_print_win32_devnames_free (BtkPrintWin32Devnames *devnames)
{
  g_free (devnames->driver);
  g_free (devnames->device);
  g_free (devnames->output);
  g_free (devnames);
}

BtkPrintWin32Devnames *
btk_print_win32_devnames_from_win32 (HGLOBAL global)
{
  LPDEVNAMES win = GlobalLock (global);
  gunichar2 *data = (gunichar2 *)win;
  BtkPrintWin32Devnames *devnames = g_new (BtkPrintWin32Devnames, 1);
  
  devnames->driver = g_utf16_to_utf8 (data + win->wDriverOffset, 
				      -1, NULL, NULL, NULL);
  devnames->device = g_utf16_to_utf8 (data + win->wDeviceOffset, 
				      -1, NULL, NULL, NULL);
  devnames->output = g_utf16_to_utf8 (data + win->wOutputOffset, 
				      -1, NULL, NULL, NULL);
  devnames->flags = win->wDefault;
  
  GlobalUnlock (global);

  return devnames;
}

HGLOBAL 
btk_print_win32_devnames_to_win32 (const BtkPrintWin32Devnames *devnames)
{
  HGLOBAL global;
  LPDEVNAMES windevnames;
  gunichar2 *data;
  gunichar2 *driver, *device, *output;
  blong driver_len, device_len, output_len;
  int i;

  driver = g_utf8_to_utf16 (devnames->driver, -1, NULL, &driver_len, NULL);
  device = g_utf8_to_utf16 (devnames->device, -1, NULL, &device_len, NULL);
  output = g_utf8_to_utf16 (devnames->output, -1, NULL, &output_len, NULL);

  global = GlobalAlloc (GMEM_MOVEABLE, 
			sizeof (DEVNAMES) + 
			(driver_len + 1) * 2 + 
			(device_len + 1) * 2 +
			(output_len + 1) * 2);

  windevnames = GlobalLock (global);
  data = (gunichar2 *)windevnames;
  i = sizeof(DEVNAMES) / sizeof (gunichar2);

  windevnames->wDriverOffset = i;
  memcpy (data + i, driver, (driver_len + 1) * sizeof (gunichar2));
  i += driver_len + 1;
  windevnames->wDeviceOffset = i;
  memcpy (data + i, device, (device_len + 1) * sizeof (gunichar2));
  i += device_len + 1;
  windevnames->wOutputOffset = i;
  memcpy (data + i, output, (output_len + 1) * sizeof (gunichar2));
  i += output_len + 1;
  windevnames->wDefault = devnames->flags;
  GlobalUnlock (global);

  g_free (driver);
  g_free (device);
  g_free (output);

  return global;
}

HGLOBAL 
btk_print_win32_devnames_to_win32_from_printer_name (const char *printer_name)
{
  HGLOBAL global;
  BtkPrintWin32Devnames *devnames;

  devnames = btk_print_win32_devnames_from_printer_name(printer_name);
  if (devnames)
    {
      global = btk_print_win32_devnames_to_win32 (devnames);
      btk_print_win32_devnames_free (devnames);
    }
  else
    global = NULL;

  return global;
}

/*
 * Used to get printer device information from a printer name.  This
 * can fail if the user has no right to read printer properties, so
 * this function can return NULL.
 */
BtkPrintWin32Devnames *
btk_print_win32_devnames_from_printer_name (const char *printer_name)
{
  HANDLE hprinter;
  gunichar2* win32_printer_name;
  BtkPrintWin32Devnames *devnames;

  win32_printer_name = g_utf8_to_utf16 (printer_name, -1, NULL, NULL, NULL);
  if (OpenPrinterW (win32_printer_name, &hprinter, NULL))
    {
      DWORD needed;
      PRINTER_INFO_2W* printer_info;

      GetPrinterW (hprinter, 2, NULL, 0, &needed);
      printer_info = (PRINTER_INFO_2W* )g_malloc ((bsize) needed);
      GetPrinterW (hprinter, 2, (LPBYTE) printer_info, needed, &needed);
      devnames = g_new (BtkPrintWin32Devnames, 1);
      devnames->driver = g_utf16_to_utf8 (printer_info->pDriverName, -1, NULL, NULL, NULL);
      devnames->device = g_strdup (printer_name);
      devnames->output = g_utf16_to_utf8 (printer_info->pPortName, -1, NULL, NULL, NULL);
      devnames->flags  = 0;
      ClosePrinter (hprinter);
      g_free (printer_info);
    }
  else
    {
      /* Could not open printer */
      devnames = NULL;
    }
  g_free (win32_printer_name);

  return devnames;
}


#define __BTK_PRINT_WIN32_C__
#include "btkaliasdef.c"
