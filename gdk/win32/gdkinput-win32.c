/* BDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 1998-2007 Tor Lillqvist
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

/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "bdk.h"
#include "bdkinput.h"
#include "bdkinternals.h"
#include "bdkprivate-win32.h"
#include "bdkinput-win32.h"

#define WINTAB32_DLL "Wintab32.dll"

#define PACKETDATA (PK_CONTEXT | PK_CURSOR | PK_BUTTONS | PK_X | PK_Y  | PK_NORMAL_PRESSURE | PK_ORIENTATION)
/* We want everything in absolute mode */
#define PACKETMODE (0)
#include <pktdef.h>

#define DEBUG_WINTAB 1		/* Verbose debug messages enabled */

#define PROXIMITY_OUT_DELAY 200 /* In milliseconds, see set_ignore_core */

#define TWOPI (2.*G_PI)

/* Forward declarations */

static BdkDevicePrivate *bdk_input_find_dev_from_ctx (HCTX hctx,
						      UINT id);
static GList     *wintab_contexts = NULL;

static BdkWindow *wintab_window = NULL;

static BdkDevicePrivate *_bdk_device_in_proximity;

typedef UINT (WINAPI *t_WTInfoA) (UINT a, UINT b, LPVOID c);
typedef UINT (WINAPI *t_WTInfoW) (UINT a, UINT b, LPVOID c);
typedef BOOL (WINAPI *t_WTEnable) (HCTX a, BOOL b);
typedef HCTX (WINAPI *t_WTOpenA) (HWND a, LPLOGCONTEXTA b, BOOL c);
typedef BOOL (WINAPI *t_WTGetA) (HCTX a, LPLOGCONTEXTA b);
typedef BOOL (WINAPI *t_WTSetA) (HCTX a, LPLOGCONTEXTA b);
typedef BOOL (WINAPI *t_WTOverlap) (HCTX a, BOOL b);
typedef BOOL (WINAPI *t_WTPacket) (HCTX a, UINT b, LPVOID c);
typedef int (WINAPI *t_WTQueueSizeSet) (HCTX a, int b);

static t_WTInfoA p_WTInfoA;
static t_WTInfoW p_WTInfoW;
static t_WTEnable p_WTEnable;
static t_WTOpenA p_WTOpenA;
static t_WTGetA p_WTGetA;
static t_WTSetA p_WTSetA;
static t_WTOverlap p_WTOverlap;
static t_WTPacket p_WTPacket;
static t_WTQueueSizeSet p_WTQueueSizeSet;

static BdkDevicePrivate *
bdk_input_find_dev_from_ctx (HCTX hctx,
			     UINT cursor)
{
  GList *tmp_list = _bdk_input_devices;
  BdkDevicePrivate *bdkdev;

  while (tmp_list)
    {
      bdkdev = (BdkDevicePrivate *) (tmp_list->data);
      if (bdkdev->hctx == hctx && bdkdev->cursor == cursor)
	return bdkdev;
      tmp_list = tmp_list->next;
    }
  return NULL;
}


#if DEBUG_WINTAB

#ifdef G_ENABLE_DEBUG
static void
print_lc(LOGCONTEXT *lc)
{
  g_print ("lcName = %s\n", lc->lcName);
  g_print ("lcOptions =");
  if (lc->lcOptions & CXO_SYSTEM) g_print (" CXO_SYSTEM");
  if (lc->lcOptions & CXO_PEN) g_print (" CXO_PEN");
  if (lc->lcOptions & CXO_MESSAGES) g_print (" CXO_MESSAGES");
  if (lc->lcOptions & CXO_MARGIN) g_print (" CXO_MARGIN");
  if (lc->lcOptions & CXO_MGNINSIDE) g_print (" CXO_MGNINSIDE");
  if (lc->lcOptions & CXO_CSRMESSAGES) g_print (" CXO_CSRMESSAGES");
  g_print ("\n");
  g_print ("lcStatus =");
  if (lc->lcStatus & CXS_DISABLED) g_print (" CXS_DISABLED");
  if (lc->lcStatus & CXS_OBSCURED) g_print (" CXS_OBSCURED");
  if (lc->lcStatus & CXS_ONTOP) g_print (" CXS_ONTOP");
  g_print ("\n");
  g_print ("lcLocks =");
  if (lc->lcLocks & CXL_INSIZE) g_print (" CXL_INSIZE");
  if (lc->lcLocks & CXL_INASPECT) g_print (" CXL_INASPECT");
  if (lc->lcLocks & CXL_SENSITIVITY) g_print (" CXL_SENSITIVITY");
  if (lc->lcLocks & CXL_MARGIN) g_print (" CXL_MARGIN");
  g_print ("\n");
  g_print ("lcMsgBase = %#x, lcDevice = %#x, lcPktRate = %d\n",
	  lc->lcMsgBase, lc->lcDevice, lc->lcPktRate);
  g_print ("lcPktData =");
  if (lc->lcPktData & PK_CONTEXT) g_print (" PK_CONTEXT");
  if (lc->lcPktData & PK_STATUS) g_print (" PK_STATUS");
  if (lc->lcPktData & PK_TIME) g_print (" PK_TIME");
  if (lc->lcPktData & PK_CHANGED) g_print (" PK_CHANGED");
  if (lc->lcPktData & PK_SERIAL_NUMBER) g_print (" PK_SERIAL_NUMBER");
  if (lc->lcPktData & PK_CURSOR) g_print (" PK_CURSOR");
  if (lc->lcPktData & PK_BUTTONS) g_print (" PK_BUTTONS");
  if (lc->lcPktData & PK_X) g_print (" PK_X");
  if (lc->lcPktData & PK_Y) g_print (" PK_Y");
  if (lc->lcPktData & PK_Z) g_print (" PK_Z");
  if (lc->lcPktData & PK_NORMAL_PRESSURE) g_print (" PK_NORMAL_PRESSURE");
  if (lc->lcPktData & PK_TANGENT_PRESSURE) g_print (" PK_TANGENT_PRESSURE");
  if (lc->lcPktData & PK_ORIENTATION) g_print (" PK_ORIENTATION");
  if (lc->lcPktData & PK_ROTATION) g_print (" PK_ROTATION");
  g_print ("\n");
  g_print ("lcPktMode =");
  if (lc->lcPktMode & PK_CONTEXT) g_print (" PK_CONTEXT");
  if (lc->lcPktMode & PK_STATUS) g_print (" PK_STATUS");
  if (lc->lcPktMode & PK_TIME) g_print (" PK_TIME");
  if (lc->lcPktMode & PK_CHANGED) g_print (" PK_CHANGED");
  if (lc->lcPktMode & PK_SERIAL_NUMBER) g_print (" PK_SERIAL_NUMBER");
  if (lc->lcPktMode & PK_CURSOR) g_print (" PK_CURSOR");
  if (lc->lcPktMode & PK_BUTTONS) g_print (" PK_BUTTONS");
  if (lc->lcPktMode & PK_X) g_print (" PK_X");
  if (lc->lcPktMode & PK_Y) g_print (" PK_Y");
  if (lc->lcPktMode & PK_Z) g_print (" PK_Z");
  if (lc->lcPktMode & PK_NORMAL_PRESSURE) g_print (" PK_NORMAL_PRESSURE");
  if (lc->lcPktMode & PK_TANGENT_PRESSURE) g_print (" PK_TANGENT_PRESSURE");
  if (lc->lcPktMode & PK_ORIENTATION) g_print (" PK_ORIENTATION");
  if (lc->lcPktMode & PK_ROTATION) g_print (" PK_ROTATION");
  g_print ("\n");
  g_print ("lcMoveMask =");
  if (lc->lcMoveMask & PK_CONTEXT) g_print (" PK_CONTEXT");
  if (lc->lcMoveMask & PK_STATUS) g_print (" PK_STATUS");
  if (lc->lcMoveMask & PK_TIME) g_print (" PK_TIME");
  if (lc->lcMoveMask & PK_CHANGED) g_print (" PK_CHANGED");
  if (lc->lcMoveMask & PK_SERIAL_NUMBER) g_print (" PK_SERIAL_NUMBER");
  if (lc->lcMoveMask & PK_CURSOR) g_print (" PK_CURSOR");
  if (lc->lcMoveMask & PK_BUTTONS) g_print (" PK_BUTTONS");
  if (lc->lcMoveMask & PK_X) g_print (" PK_X");
  if (lc->lcMoveMask & PK_Y) g_print (" PK_Y");
  if (lc->lcMoveMask & PK_Z) g_print (" PK_Z");
  if (lc->lcMoveMask & PK_NORMAL_PRESSURE) g_print (" PK_NORMAL_PRESSURE");
  if (lc->lcMoveMask & PK_TANGENT_PRESSURE) g_print (" PK_TANGENT_PRESSURE");
  if (lc->lcMoveMask & PK_ORIENTATION) g_print (" PK_ORIENTATION");
  if (lc->lcMoveMask & PK_ROTATION) g_print (" PK_ROTATION");
  g_print ("\n");
  g_print ("lcBtnDnMask = %#x, lcBtnUpMask = %#x\n",
	  (guint) lc->lcBtnDnMask, (guint) lc->lcBtnUpMask);
  g_print ("lcInOrgX = %ld, lcInOrgY = %ld, lcInOrgZ = %ld\n",
	  lc->lcInOrgX, lc->lcInOrgY, lc->lcInOrgZ);
  g_print ("lcInExtX = %ld, lcInExtY = %ld, lcInExtZ = %ld\n",
	  lc->lcInExtX, lc->lcInExtY, lc->lcInExtZ);
  g_print ("lcOutOrgX = %ld, lcOutOrgY = %ld, lcOutOrgZ = %ld\n",
	  lc->lcOutOrgX, lc->lcOutOrgY, lc->lcOutOrgZ);
  g_print ("lcOutExtX = %ld, lcOutExtY = %ld, lcOutExtZ = %ld\n",
	  lc->lcOutExtX, lc->lcOutExtY, lc->lcOutExtZ);
  g_print ("lcSensX = %g, lcSensY = %g, lcSensZ = %g\n",
	  lc->lcSensX / 65536., lc->lcSensY / 65536., lc->lcSensZ / 65536.);
  g_print ("lcSysMode = %d\n", lc->lcSysMode);
  g_print ("lcSysOrgX = %d, lcSysOrgY = %d\n",
	  lc->lcSysOrgX, lc->lcSysOrgY);
  g_print ("lcSysExtX = %d, lcSysExtY = %d\n",
	  lc->lcSysExtX, lc->lcSysExtY);
  g_print ("lcSysSensX = %g, lcSysSensY = %g\n",
	  lc->lcSysSensX / 65536., lc->lcSysSensY / 65536.);
}

static void
print_cursor (int index)
{
  int size;
  int i;
  char *name;
  BOOL active;
  WTPKT wtpkt;
  BYTE buttons;
  BYTE buttonbits;
  char *btnnames;
  char *p;
  BYTE buttonmap[32];
  BYTE sysbtnmap[32];
  BYTE npbutton;
  UINT npbtnmarks[2];
  UINT *npresponse;
  BYTE tpbutton;
  UINT tpbtnmarks[2];
  UINT *tpresponse;
  DWORD physid;
  UINT mode;
  UINT minpktdata;
  UINT minbuttons;
  UINT capabilities;

  size = (*p_WTInfoA) (WTI_CURSORS + index, CSR_NAME, NULL);
  name = g_malloc (size + 1);
  (*p_WTInfoA) (WTI_CURSORS + index, CSR_NAME, name);
  g_print ("NAME: %s\n", name);
  (*p_WTInfoA) (WTI_CURSORS + index, CSR_ACTIVE, &active);
  g_print ("ACTIVE: %s\n", active ? "YES" : "NO");
  (*p_WTInfoA) (WTI_CURSORS + index, CSR_PKTDATA, &wtpkt);
  g_print ("PKTDATA: %#x:", (guint) wtpkt);
#define BIT(x) if (wtpkt & PK_##x) g_print (" " #x)
  BIT (CONTEXT);
  BIT (STATUS);
  BIT (TIME);
  BIT (CHANGED);
  BIT (SERIAL_NUMBER);
  BIT (BUTTONS);
  BIT (X);
  BIT (Y);
  BIT (Z);
  BIT (NORMAL_PRESSURE);
  BIT (TANGENT_PRESSURE);
  BIT (ORIENTATION);
  BIT (ROTATION);
#undef BIT
  g_print ("\n");
  (*p_WTInfoA) (WTI_CURSORS + index, CSR_BUTTONS, &buttons);
  g_print ("BUTTONS: %d\n", buttons);
  (*p_WTInfoA) (WTI_CURSORS + index, CSR_BUTTONBITS, &buttonbits);
  g_print ("BUTTONBITS: %d\n", buttonbits);
  size = (*p_WTInfoA) (WTI_CURSORS + index, CSR_BTNNAMES, NULL);
  g_print ("BTNNAMES:");
  if (size > 0)
    {
      btnnames = g_malloc (size + 1);
      (*p_WTInfoA) (WTI_CURSORS + index, CSR_BTNNAMES, btnnames);
      p = btnnames;
      while (*p)
	{
	  g_print (" %s", p);
	  p += strlen (p) + 1;
	}
    }
  g_print ("\n");
  (*p_WTInfoA) (WTI_CURSORS + index, CSR_BUTTONMAP, buttonmap);
  g_print ("BUTTONMAP:");
  for (i = 0; i < buttons; i++)
    g_print (" %d", buttonmap[i]);
  g_print ("\n");
  (*p_WTInfoA) (WTI_CURSORS + index, CSR_SYSBTNMAP, sysbtnmap);
  g_print ("SYSBTNMAP:");
  for (i = 0; i < buttons; i++)
    g_print (" %d", sysbtnmap[i]);
  g_print ("\n");
  (*p_WTInfoA) (WTI_CURSORS + index, CSR_NPBUTTON, &npbutton);
  g_print ("NPBUTTON: %d\n", npbutton);
  (*p_WTInfoA) (WTI_CURSORS + index, CSR_NPBTNMARKS, npbtnmarks);
  g_print ("NPBTNMARKS: %d %d\n", npbtnmarks[0], npbtnmarks[1]);
  size = (*p_WTInfoA) (WTI_CURSORS + index, CSR_NPRESPONSE, NULL);
  g_print ("NPRESPONSE:");
  if (size > 0)
    {
      npresponse = g_malloc (size);
      (*p_WTInfoA) (WTI_CURSORS + index, CSR_NPRESPONSE, npresponse);
      for (i = 0; i < size / sizeof (UINT); i++)
	g_print (" %d", npresponse[i]);
    }
  g_print ("\n");
  (*p_WTInfoA) (WTI_CURSORS + index, CSR_TPBUTTON, &tpbutton);
  g_print ("TPBUTTON: %d\n", tpbutton);
  (*p_WTInfoA) (WTI_CURSORS + index, CSR_TPBTNMARKS, tpbtnmarks);
  g_print ("TPBTNMARKS: %d %d\n", tpbtnmarks[0], tpbtnmarks[1]);
  size = (*p_WTInfoA) (WTI_CURSORS + index, CSR_TPRESPONSE, NULL);
  g_print ("TPRESPONSE:");
  if (size > 0)
    {
      tpresponse = g_malloc (size);
      (*p_WTInfoA) (WTI_CURSORS + index, CSR_TPRESPONSE, tpresponse);
      for (i = 0; i < size / sizeof (UINT); i++)
	g_print (" %d", tpresponse[i]);
    }
  g_print ("\n");
  (*p_WTInfoA) (WTI_CURSORS + index, CSR_PHYSID, &physid);
  g_print ("PHYSID: %#x\n", (guint) physid);
  (*p_WTInfoA) (WTI_CURSORS + index, CSR_CAPABILITIES, &capabilities);
  g_print ("CAPABILITIES: %#x:", capabilities);
#define BIT(x) if (capabilities & CRC_##x) g_print (" " #x)
  BIT (MULTIMODE);
  BIT (AGGREGATE);
  BIT (INVERT);
#undef BIT
  g_print ("\n");
  if (capabilities & CRC_MULTIMODE)
    {
      (*p_WTInfoA) (WTI_CURSORS + index, CSR_MODE, &mode);
      g_print ("MODE: %d\n", mode);
    }
  if (capabilities & CRC_AGGREGATE)
    {
      (*p_WTInfoA) (WTI_CURSORS + index, CSR_MINPKTDATA, &minpktdata);
      g_print ("MINPKTDATA: %d\n", minpktdata);
      (*p_WTInfoA) (WTI_CURSORS + index, CSR_MINBUTTONS, &minbuttons);
      g_print ("MINBUTTONS: %d\n", minbuttons);
    }
}
#endif
#endif

void
_bdk_input_wintab_init_check (void)
{
  static gboolean wintab_initialized = FALSE;
  BdkDevicePrivate *bdkdev;
  BdkWindowAttr wa;
  WORD specversion;
  HCTX *hctx;
  UINT ndevices, ncursors, ncsrtypes, firstcsr, hardware;
  BOOL active;
  DWORD physid;
  AXIS axis_x, axis_y, axis_npressure, axis_or[3];
  int i, k, n;
  int devix, cursorix;
  wchar_t devname[100], csrname[100];
  gchar *devname_utf8, *csrname_utf8;
  BOOL defcontext_done;
  HMODULE wintab32;
  char *wintab32_dll_path;
  char dummy;

  if (wintab_initialized)
    return;
  
  wintab_initialized = TRUE;
  
  wintab_contexts = NULL;

  if (_bdk_input_ignore_wintab)
    return;

  n = GetSystemDirectory (&dummy, 0);

  if (n <= 0)
    return;

  wintab32_dll_path = g_malloc (n + 1 + strlen (WINTAB32_DLL));
  k = GetSystemDirectory (wintab32_dll_path, n);
  
  if (k == 0 || k > n)
    {
      g_free (wintab32_dll_path);
      return;
    }

  if (!G_IS_DIR_SEPARATOR (wintab32_dll_path[strlen (wintab32_dll_path) -1]))
    strcat (wintab32_dll_path, G_DIR_SEPARATOR_S);
  strcat (wintab32_dll_path, WINTAB32_DLL);

  if ((wintab32 = LoadLibrary (wintab32_dll_path)) == NULL)
    return;

  if ((p_WTInfoA = (t_WTInfoA) GetProcAddress (wintab32, "WTInfoA")) == NULL)
    return;
  if ((p_WTInfoW = (t_WTInfoW) GetProcAddress (wintab32, "WTInfoW")) == NULL)
    return;
  if ((p_WTEnable = (t_WTEnable) GetProcAddress (wintab32, "WTEnable")) == NULL)
    return;
  if ((p_WTOpenA = (t_WTOpenA) GetProcAddress (wintab32, "WTOpenA")) == NULL)
    return;
  if ((p_WTGetA = (t_WTGetA) GetProcAddress (wintab32, "WTGetA")) == NULL)
    return;
  if ((p_WTSetA = (t_WTSetA) GetProcAddress (wintab32, "WTSetA")) == NULL)
    return;
  if ((p_WTOverlap = (t_WTOverlap) GetProcAddress (wintab32, "WTOverlap")) == NULL)
    return;
  if ((p_WTPacket = (t_WTPacket) GetProcAddress (wintab32, "WTPacket")) == NULL)
    return;
  if ((p_WTQueueSizeSet = (t_WTQueueSizeSet) GetProcAddress (wintab32, "WTQueueSizeSet")) == NULL)
    return;
    
  if (!(*p_WTInfoA) (0, 0, NULL))
    return;

  (*p_WTInfoA) (WTI_INTERFACE, IFC_SPECVERSION, &specversion);
  BDK_NOTE (INPUT, g_print ("Wintab interface version %d.%d\n",
			    HIBYTE (specversion), LOBYTE (specversion)));
  (*p_WTInfoA) (WTI_INTERFACE, IFC_NDEVICES, &ndevices);
  (*p_WTInfoA) (WTI_INTERFACE, IFC_NCURSORS, &ncursors);
#if DEBUG_WINTAB
  BDK_NOTE (INPUT, g_print ("NDEVICES: %d, NCURSORS: %d\n",
			    ndevices, ncursors));
#endif
  /* Create a dummy window to receive wintab events */
  wa.wclass = BDK_INPUT_OUTPUT;
  wa.event_mask = BDK_ALL_EVENTS_MASK;
  wa.width = 2;
  wa.height = 2;
  wa.x = -100;
  wa.y = -100;
  wa.window_type = BDK_WINDOW_TOPLEVEL;
  if ((wintab_window = bdk_window_new (NULL, &wa, BDK_WA_X|BDK_WA_Y)) == NULL)
    {
      g_warning ("bdk_input_wintab_init: bdk_window_new failed");
      return;
    }
  g_object_ref (wintab_window);
      
  for (devix = 0; devix < ndevices; devix++)
    {
      LOGCONTEXT lc;
      
      /* We open the Wintab device (hmm, what if there are several, or
       * can there even be several, probably not?) as a system
       * pointing device, i.e. it controls the normal Windows
       * cursor. This seems much more natural.
       */

      (*p_WTInfoW) (WTI_DEVICES + devix, DVC_NAME, devname);
      devname_utf8 = g_utf16_to_utf8 (devname, -1, NULL, NULL, NULL);
#ifdef DEBUG_WINTAB
      BDK_NOTE (INPUT, (g_print("Device %d: %s\n", devix, devname_utf8)));
#endif
      (*p_WTInfoA) (WTI_DEVICES + devix, DVC_NCSRTYPES, &ncsrtypes);
      (*p_WTInfoA) (WTI_DEVICES + devix, DVC_FIRSTCSR, &firstcsr);
      (*p_WTInfoA) (WTI_DEVICES + devix, DVC_HARDWARE, &hardware);
      (*p_WTInfoA) (WTI_DEVICES + devix, DVC_X, &axis_x);
      (*p_WTInfoA) (WTI_DEVICES + devix, DVC_Y, &axis_y);
      (*p_WTInfoA) (WTI_DEVICES + devix, DVC_NPRESSURE, &axis_npressure);
      (*p_WTInfoA) (WTI_DEVICES + devix, DVC_ORIENTATION, axis_or);

      defcontext_done = FALSE;
      if (HIBYTE (specversion) > 1 || LOBYTE (specversion) >= 1)
	{
	  /* Try to get device-specific default context */
	  /* Some drivers, e.g. Aiptek, don't provide this info */
	  if ((*p_WTInfoA) (WTI_DSCTXS + devix, 0, &lc) > 0)
	    defcontext_done = TRUE;
#if DEBUG_WINTAB
	  if (defcontext_done)
	    BDK_NOTE (INPUT, (g_print("Using device-specific default context\n")));
	  else
	    BDK_NOTE (INPUT, (g_print("Note: Driver did not provide device specific default context info despite claiming to support version 1.1\n")));
#endif
	}

      if (!defcontext_done)
	(*p_WTInfoA) (WTI_DEFSYSCTX, 0, &lc);
#if DEBUG_WINTAB
      BDK_NOTE (INPUT, (g_print("Default context:\n"), print_lc(&lc)));
#endif
      lc.lcOptions |= CXO_MESSAGES | CXO_CSRMESSAGES;
      lc.lcStatus = 0;
      lc.lcMsgBase = WT_DEFBASE;
      lc.lcPktRate = 0;
      lc.lcPktData = PACKETDATA;
      lc.lcPktMode = PACKETMODE;
      lc.lcMoveMask = PACKETDATA;
      lc.lcBtnUpMask = lc.lcBtnDnMask = ~0;
      lc.lcOutOrgX = axis_x.axMin;
      lc.lcOutOrgY = axis_y.axMin;
      lc.lcOutExtX = axis_x.axMax - axis_x.axMin + 1;
      lc.lcOutExtY = axis_y.axMax - axis_y.axMin + 1;
      lc.lcOutExtY = -lc.lcOutExtY; /* We want Y growing downward */
#if DEBUG_WINTAB
      BDK_NOTE (INPUT, (g_print("context for device %d:\n", devix),
			print_lc(&lc)));
#endif
      hctx = g_new (HCTX, 1);
      if ((*hctx = (*p_WTOpenA) (BDK_WINDOW_HWND (wintab_window), &lc, TRUE)) == NULL)
	{
	  g_warning ("bdk_input_wintab_init: WTOpen failed");
	  return;
	}
      BDK_NOTE (INPUT, g_print ("opened Wintab device %d %p\n",
				devix, *hctx));
      
      wintab_contexts = g_list_append (wintab_contexts, hctx);

      (*p_WTOverlap) (*hctx, TRUE);

#if DEBUG_WINTAB
      BDK_NOTE (INPUT, (g_print("context for device %d after WTOpen:\n", devix),
			print_lc(&lc)));
#endif
      /* Increase packet queue size to reduce the risk of lost packets.
       * According to the specs, if the function fails we must try again
       * with a smaller queue size.
       */
      BDK_NOTE (INPUT, g_print("Attempting to increase queue size\n"));
      for (i = 128; i >= 1; i >>= 1)
	{
	  if ((*p_WTQueueSizeSet) (*hctx, i))
	    {
	      BDK_NOTE (INPUT, g_print("Queue size set to %d\n", i));
	      break;
	    }
	}
      if (!i)
	BDK_NOTE (INPUT, g_print("Whoops, no queue size could be set\n"));
      for (cursorix = firstcsr; cursorix < firstcsr + ncsrtypes; cursorix++)
	{
#ifdef DEBUG_WINTAB
	  BDK_NOTE (INPUT, (g_print("Cursor %d:\n", cursorix), print_cursor (cursorix)));
#endif
	  active = FALSE;
	  (*p_WTInfoA) (WTI_CURSORS + cursorix, CSR_ACTIVE, &active);
	  if (!active)
	    continue;

	  /* Wacom tablets seem to report cursors corresponding to
	   * nonexistent pens or pucks. At least my ArtPad II reports
	   * six cursors: a puck, pressure stylus and eraser stylus,
	   * and then the same three again. I only have a
	   * pressure-sensitive pen. The puck instances, and the
	   * second instances of the styluses report physid zero. So
	   * at least for Wacom, skip cursors with physid zero.
	   */
	  (*p_WTInfoA) (WTI_CURSORS + cursorix, CSR_PHYSID, &physid);
	  if (wcscmp (devname, L"WACOM Tablet") == 0 && physid == 0)
	    continue;

	  bdkdev = g_object_new (BDK_TYPE_DEVICE, NULL);
	  (*p_WTInfoW) (WTI_CURSORS + cursorix, CSR_NAME, csrname);
	  csrname_utf8 = g_utf16_to_utf8 (csrname, -1, NULL, NULL, NULL);
	  bdkdev->info.name = g_strconcat (devname_utf8, " ", csrname_utf8, NULL);
	  g_free (csrname_utf8);
	  bdkdev->info.source = BDK_SOURCE_PEN;
	  bdkdev->info.mode = BDK_MODE_SCREEN;
	  bdkdev->info.has_cursor = TRUE;
	  bdkdev->hctx = *hctx;
	  bdkdev->cursor = cursorix;
	  (*p_WTInfoA) (WTI_CURSORS + cursorix, CSR_PKTDATA, &bdkdev->pktdata);
	  bdkdev->info.num_axes = 0;
	  if (bdkdev->pktdata & PK_X)
	    bdkdev->info.num_axes++;
	  if (bdkdev->pktdata & PK_Y)
	    bdkdev->info.num_axes++;
	  if (bdkdev->pktdata & PK_NORMAL_PRESSURE)
	    bdkdev->info.num_axes++;
	  /* The wintab driver for the Wacom ArtPad II reports
	   * PK_ORIENTATION in CSR_PKTDATA, but the tablet doesn't
	   * actually sense tilt. Catch this by noticing that the
	   * orientation axis's azimuth resolution is zero.
	   */
	  if ((bdkdev->pktdata & PK_ORIENTATION)
	      && axis_or[0].axResolution == 0)
	    bdkdev->pktdata &= ~PK_ORIENTATION;
	  
	  if (bdkdev->pktdata & PK_ORIENTATION)
	    bdkdev->info.num_axes += 2; /* x and y tilt */

	  bdkdev->info.axes = g_new (BdkDeviceAxis, bdkdev->info.num_axes);
	  bdkdev->axes = g_new (BdkAxisInfo, bdkdev->info.num_axes);
	  bdkdev->last_axis_data = g_new (gint, bdkdev->info.num_axes);
	  
	  k = 0;
	  if (bdkdev->pktdata & PK_X)
	    {
	      bdkdev->axes[k].resolution = axis_x.axResolution / 65535.;
	      bdkdev->axes[k].min_value = axis_x.axMin;
	      bdkdev->axes[k].max_value = axis_x.axMax;
	      bdkdev->info.axes[k].use = BDK_AXIS_X;
	      bdkdev->info.axes[k].min = axis_x.axMin;
	      bdkdev->info.axes[k].max = axis_x.axMax;
	      k++;
	    }
	  if (bdkdev->pktdata & PK_Y)
	    {
	      bdkdev->axes[k].resolution = axis_y.axResolution / 65535.;
	      bdkdev->axes[k].min_value = axis_y.axMin;
	      bdkdev->axes[k].max_value = axis_y.axMax;
	      bdkdev->info.axes[k].use = BDK_AXIS_Y;
	      bdkdev->info.axes[k].min = axis_y.axMin;
	      bdkdev->info.axes[k].max = axis_y.axMax;
	      k++;
	    }
	  if (bdkdev->pktdata & PK_NORMAL_PRESSURE)
	    {
	      bdkdev->axes[k].resolution = axis_npressure.axResolution / 65535.;
	      bdkdev->axes[k].min_value = axis_npressure.axMin;
	      bdkdev->axes[k].max_value = axis_npressure.axMax;
	      bdkdev->info.axes[k].use = BDK_AXIS_PRESSURE;
	      /* GIMP seems to expect values in the range 0-1 */
	      bdkdev->info.axes[k].min = 0.0; /*axis_npressure.axMin;*/
	      bdkdev->info.axes[k].max = 1.0; /*axis_npressure.axMax;*/
	      k++;
	    }
	  if (bdkdev->pktdata & PK_ORIENTATION)
	    {
	      BdkAxisUse axis;
	      
	      bdkdev->orientation_axes[0] = axis_or[0];
	      bdkdev->orientation_axes[1] = axis_or[1];
	      for (axis = BDK_AXIS_XTILT; axis <= BDK_AXIS_YTILT; axis++)
		{
		  /* Wintab gives us aximuth and altitude, which
		   * we convert to x and y tilt in the -1000..1000 range
		   */
		  bdkdev->axes[k].resolution = 1000;
		  bdkdev->axes[k].min_value = -1000;
		  bdkdev->axes[k].max_value = 1000;
		  bdkdev->info.axes[k].use = axis;
		  bdkdev->info.axes[k].min = -1.0;
		  bdkdev->info.axes[k].max = 1.0;
		  k++;
		}
	    }
	  bdkdev->info.num_keys = 0;
	  bdkdev->info.keys = NULL;
	  BDK_NOTE (INPUT, g_print ("device: (%d) %s axes: %d\n",
				    cursorix,
				    bdkdev->info.name,
				    bdkdev->info.num_axes));
	  for (i = 0; i < bdkdev->info.num_axes; i++)
	    BDK_NOTE (INPUT, g_print ("... axis %d: %d--%d@%d\n",
				      i,
				      bdkdev->axes[i].min_value, 
				      bdkdev->axes[i].max_value, 
				      bdkdev->axes[i].resolution));
	  _bdk_input_devices = g_list_append (_bdk_input_devices,
					      bdkdev);
	}
      g_free (devname_utf8);
    }
}

static void
decode_tilt (gint   *axis_data,
	     AXIS   *axes,
	     PACKET *packet)
{
  /* As I don't have a tilt-sensing tablet,
   * I cannot test this code.
   */
  
  double az, el;

  az = TWOPI * packet->pkOrientation.orAzimuth /
    (axes[0].axResolution / 65536.);
  el = TWOPI * packet->pkOrientation.orAltitude /
    (axes[1].axResolution / 65536.);
  
  /* X tilt */
  axis_data[0] = cos (az) * cos (el) * 1000;
  /* Y tilt */
  axis_data[1] = sin (az) * cos (el) * 1000;
}

static void
bdk_input_translate_coordinates (BdkDevicePrivate *bdkdev,
				 BdkWindow        *window,
				 gint             *axis_data,
				 gdouble          *axis_out,
				 gdouble          *x_out,
				 gdouble          *y_out)
{
  BdkWindowObject *priv, *impl_window;
  BdkWindowImplWin32 *root_impl;

  int i;
  int x_axis = 0;
  int y_axis = 0;

  double device_width, device_height, x_min, y_min;
  double x_offset, y_offset, x_scale, y_scale;

  priv = (BdkWindowObject *) window;
  impl_window = (BdkWindowObject *)_bdk_window_get_impl_window (window);

  for (i=0; i<bdkdev->info.num_axes; i++)
    {
      switch (bdkdev->info.axes[i].use)
	{
	case BDK_AXIS_X:
	  x_axis = i;
	  break;
	case BDK_AXIS_Y:
	  y_axis = i;
	  break;
	default:
	  break;
	}
    }
  
  device_width = bdkdev->axes[x_axis].max_value - bdkdev->axes[x_axis].min_value;
  x_min = bdkdev->axes[x_axis].min_value;
  device_height = bdkdev->axes[y_axis].max_value - bdkdev->axes[y_axis].min_value;
  y_min = bdkdev->axes[y_axis].min_value;

  if (bdkdev->info.mode == BDK_MODE_SCREEN) 
    {
      root_impl = BDK_WINDOW_IMPL_WIN32 (BDK_WINDOW_OBJECT (_bdk_root)->impl);
      x_scale = BDK_WINDOW_OBJECT (_bdk_root)->width / device_width;
      y_scale = BDK_WINDOW_OBJECT (_bdk_root)->height / device_height;

      x_offset = - impl_window->input_window->root_x - priv->abs_x;
      y_offset = - impl_window->input_window->root_y - priv->abs_y;
    }
  else				/* BDK_MODE_WINDOW */
    {
      double x_resolution = bdkdev->axes[x_axis].resolution;
      double y_resolution = bdkdev->axes[y_axis].resolution;
      double device_aspect = (device_height*y_resolution) / (device_width * x_resolution);

      if (device_aspect * priv->width >= priv->height)
	{
	  /* device taller than window */
	  x_scale = priv->width / device_width;
	  y_scale = (x_scale * x_resolution) / y_resolution;

	  x_offset = 0;
	  y_offset = -(device_height * y_scale - priv->height) / 2;
	}
      else
	{
	  /* window taller than device */
	  y_scale = priv->height / device_height;
	  x_scale = (y_scale * y_resolution)  / x_resolution;

	  y_offset = 0;
	  x_offset = - (device_width * x_scale - priv->width) / 2;
	}
    }

  for (i = 0; i < bdkdev->info.num_axes; i++)
    {
      switch (bdkdev->info.axes[i].use)
	{
	case BDK_AXIS_X:
	  axis_out[i] = x_offset + x_scale * axis_data[x_axis];
	  if (x_out)
	    *x_out = axis_out[i];
	  break;
	case BDK_AXIS_Y:
	  axis_out[i] = y_offset + y_scale * axis_data[y_axis];
	  if (y_out)
	    *y_out = axis_out[i];
	  break;
	default:
	  axis_out[i] =
	    (bdkdev->info.axes[i].max * (axis_data[i] - bdkdev->axes[i].min_value) +
	     bdkdev->info.axes[i].min * (bdkdev->axes[i].max_value - axis_data[i])) /
	    (bdkdev->axes[i].max_value - bdkdev->axes[i].min_value);
	  break;
	}
    }
}

static void
bdk_input_get_root_relative_geometry (HWND w,
				      int  *x_ret,
				      int  *y_ret)
{
  POINT pt;

  pt.x = 0;
  pt.y = 0;
  ClientToScreen (w, &pt);

  if (x_ret)
    *x_ret = pt.x + _bdk_offset_x;
  if (y_ret)
    *y_ret = pt.y + _bdk_offset_y;
}

void
_bdk_input_configure_event (BdkWindow         *window)
{
  BdkInputWindow *input_window;
  BdkWindowObject *impl_window;
  int root_x, root_y;

  g_return_if_fail (window != NULL);

  impl_window = (BdkWindowObject *)_bdk_window_get_impl_window (window);
  input_window = impl_window->input_window;

  bdk_input_get_root_relative_geometry (BDK_WINDOW_HWND (window),
					&root_x, &root_y);

  input_window->root_x = root_x;
  input_window->root_y = root_y;
}

/*
 * Get the currently active keyboard modifiers (ignoring the mouse buttons)
 * We could use bdk_window_get_pointer but that function does a lot of other
 * expensive things besides getting the modifiers. This code is somewhat based
 * on build_pointer_event_state from bdkevents-win32.c
 */
static guint
get_modifier_key_state (void)
{
  guint state;
  
  state = 0;
  /* High-order bit is up/down, low order bit is toggled/untoggled */
  if (GetKeyState (VK_CONTROL) < 0)
    state |= BDK_CONTROL_MASK;
  if (GetKeyState (VK_SHIFT) < 0)
    state |= BDK_SHIFT_MASK;
  if (GetKeyState (VK_MENU) < 0)
    state |= BDK_MOD1_MASK;
  if (GetKeyState (VK_CAPITAL) & 0x1)
    state |= BDK_LOCK_MASK;

  return state;
}

#if 0
static guint ignore_core_timer = 0;

static gboolean
ignore_core_timefunc (gpointer data)
{
  /* The delay has passed */
  _bdk_input_ignore_core = FALSE;
  ignore_core_timer = 0;

  return FALSE; /* remove timeout */
}

/*
 * Set or unset the _bdk_input_ignore_core variable that tells BDK
 * to ignore events for the core pointer when the tablet is in proximity
 * The unsetting is delayed slightly so that if a tablet event arrives
 * just after proximity out, it does not cause a core pointer event
 * which e.g. causes GIMP to switch tools.
 */
static void
set_ignore_core (gboolean ignore)
{
  if (ignore)
    {
      _bdk_input_ignore_core = TRUE;
      /* Remove any pending clear */
      if (ignore_core_timer)
        {
	  g_source_remove (ignore_core_timer);
	  ignore_core_timer = 0;
	}
    }
  else
    if (!ignore_core_timer)
      ignore_core_timer = bdk_threads_add_timeout (PROXIMITY_OUT_DELAY,
					 ignore_core_timefunc, NULL);
}

#endif

void
_bdk_input_update_for_device_mode (BdkDevicePrivate *bdkdev)
{
  LOGCONTEXT lc;

  if (bdkdev != _bdk_device_in_proximity)
    return;

  if (p_WTGetA (bdkdev->hctx, &lc))
    {
      if (bdkdev->info.mode == BDK_MODE_SCREEN &&
	  (lc.lcOptions & CXO_SYSTEM) == 0)
	{
	  lc.lcOptions |= CXO_SYSTEM;
	  p_WTSetA (bdkdev->hctx, &lc);
	}
      else if (bdkdev->info.mode == BDK_MODE_WINDOW &&
	       (lc.lcOptions & CXO_SYSTEM) != 0)
	{
	  lc.lcOptions &= ~CXO_SYSTEM;
	  p_WTSetA (bdkdev->hctx, &lc);
	}
    }
}

static BdkWindow *
find_window_for_input_event (MSG* msg, int *x, int *y)
{
  POINT pt;
  BdkWindow *window;
  HWND hwnd;
  RECT rect;

  pt = msg->pt;

  window = NULL;
  hwnd = WindowFromPoint (pt);
  if (hwnd != NULL)
    {
      POINT client_pt = pt;

      ScreenToClient (hwnd, &client_pt);
      GetClientRect (hwnd, &rect);
      if (PtInRect (&rect, client_pt))
	window = bdk_win32_handle_table_lookup ((BdkNativeWindow) hwnd);
    }

  /* need to also adjust the coordinates to the new window */
  if (window)
    ScreenToClient (BDK_WINDOW_HWND (window), &pt);

  *x = pt.x;
  *y = pt.y;

  if (window)
    return window;

  return _bdk_root;
}

gboolean 
_bdk_input_other_event (BdkEvent  *event,
			MSG       *msg,
			BdkWindow *window)
{
  BdkWindowObject *obj, *impl_window;
  BdkWindow *native_window;
  BdkInputWindow *input_window;
  BdkDevicePrivate *bdkdev = NULL;
  guint key_state;

  PACKET packet;
  gint k;
  gint x, y;
  guint translated_buttons, button_diff, button_mask;
  /* Translation from tablet button state to BDK button state for
   * buttons 1-3 - swap button 2 and 3.
   */
  static guint button_map[8] = {0, 1, 4, 5, 2, 3, 6, 7};

  if (window != wintab_window)
    {
      g_warning ("_bdk_input_other_event: not wintab_window?");
      return FALSE;
    }

  native_window = find_window_for_input_event (msg, &x, &y);

  BDK_NOTE (EVENTS_OR_INPUT,
	    g_print ("_bdk_input_other_event: native_window=%p %+d%+d\n",
		     BDK_WINDOW_HWND (native_window), x, y));

  if (msg->message == WT_PACKET || msg->message == WT_CSRCHANGE)
    {
      if (!(*p_WTPacket) ((HCTX) msg->lParam, msg->wParam, &packet))
	return FALSE;
    }

  switch (msg->message)
    {
    case WT_PACKET:
      /* Don't produce any button or motion events while a window is being
       * moved or resized, see bug #151090.
       */
      if (_modal_operation_in_progress)
	{
	  BDK_NOTE (EVENTS_OR_INPUT, g_print ("... ignored when moving/sizing\n"));
	  return FALSE;
	}

      if ((bdkdev = bdk_input_find_dev_from_ctx ((HCTX) msg->lParam,
						 packet.pkCursor)) == NULL)
	return FALSE;

      if (bdkdev->info.mode == BDK_MODE_DISABLED)
	return FALSE;
      
      k = 0;
      if (bdkdev->pktdata & PK_X)
	bdkdev->last_axis_data[k++] = packet.pkX;
      if (bdkdev->pktdata & PK_Y)
	bdkdev->last_axis_data[k++] = packet.pkY;
      if (bdkdev->pktdata & PK_NORMAL_PRESSURE)
	bdkdev->last_axis_data[k++] = packet.pkNormalPressure;
      if (bdkdev->pktdata & PK_ORIENTATION)
	{
	  decode_tilt (bdkdev->last_axis_data + k,
		       bdkdev->orientation_axes, &packet);
	  k += 2;
	}

      g_assert (k == bdkdev->info.num_axes);

      translated_buttons = button_map[packet.pkButtons & 0x07] | (packet.pkButtons & ~0x07);

      if (translated_buttons != bdkdev->button_state)
	{
	  /* At least one button has changed state so produce a button event
	   * If more than one button has changed state (unlikely),
	   * just care about the first and act on the next the next time
	   * we get a packet
	   */
	  button_diff = translated_buttons ^ bdkdev->button_state;
	  
	  /* Bdk buttons are numbered 1.. */
	  event->button.button = 1;

	  for (button_mask = 1; button_mask != 0x80000000;
	       button_mask <<= 1, event->button.button++)
	    {
	      if (button_diff & button_mask)
	        {
		  /* Found a button that has changed state */
		  break;
		}
	    }

	  if (!(translated_buttons & button_mask))
	    event->any.type = BDK_BUTTON_RELEASE;
	  else
	    event->any.type = BDK_BUTTON_PRESS;
	  bdkdev->button_state ^= button_mask;
	}
      else
	{
	  event->any.type = BDK_MOTION_NOTIFY;
	}

      if (native_window == _bdk_root)
	return FALSE;

      window = _bdk_window_get_input_window_for_event (native_window,
						       event->any.type,
						       bdkdev->button_state << 8,
						       x, y, 0);

      obj = BDK_WINDOW_OBJECT (window);

      if (window == NULL ||
	  obj->extension_events == 0)
	return FALSE;
      
      impl_window = (BdkWindowObject *)_bdk_window_get_impl_window (window);
      input_window = impl_window->input_window;

      g_assert (input_window != NULL);

      if (bdkdev->info.mode == BDK_MODE_WINDOW && 
	  (obj->extension_events & BDK_ALL_DEVICES_MASK) == 0)
	return FALSE;

      event->any.window = window;
      key_state = get_modifier_key_state ();
      if (event->any.type == BDK_BUTTON_PRESS || 
	  event->any.type == BDK_BUTTON_RELEASE)
	{
	  event->button.time = _bdk_win32_get_next_tick (msg->time);
	  event->button.device = &bdkdev->info;
	  
	  event->button.axes = g_new(gdouble, bdkdev->info.num_axes);

	  bdk_input_translate_coordinates (bdkdev, window,
					   bdkdev->last_axis_data,
					   event->button.axes,
					   &event->button.x, 
					   &event->button.y);

	  /* Also calculate root coordinates. Note that input_window->root_x
	     is in BDK root coordinates. */
	  event->button.x_root = event->button.x + input_window->root_x;
	  event->button.y_root = event->button.y + input_window->root_y;

	  event->button.state = ((bdkdev->button_state << 8)
				 & (BDK_BUTTON1_MASK | BDK_BUTTON2_MASK
				    | BDK_BUTTON3_MASK | BDK_BUTTON4_MASK
				    | BDK_BUTTON5_MASK))
				| key_state;
	  BDK_NOTE (EVENTS_OR_INPUT,
		    g_print ("WINTAB button %s:%d %g,%g\n",
			     (event->button.type == BDK_BUTTON_PRESS ?
			      "press" : "release"),
			     event->button.button,
			     event->button.x, event->button.y));
	}
      else
	{
	  event->motion.time = _bdk_win32_get_next_tick (msg->time);
	  event->motion.is_hint = FALSE;
	  event->motion.device = &bdkdev->info;

	  event->motion.axes = g_new(gdouble, bdkdev->info.num_axes);

	  bdk_input_translate_coordinates (bdkdev, window,
					   bdkdev->last_axis_data,
					   event->motion.axes,
					   &event->motion.x, 
					   &event->motion.y);

	  /* Also calculate root coordinates. Note that input_window->root_x
	     is in BDK root coordinates. */
	  event->motion.x_root = event->motion.x + input_window->root_x;
	  event->motion.y_root = event->motion.y + input_window->root_y;

	  event->motion.state = ((bdkdev->button_state << 8)
				 & (BDK_BUTTON1_MASK | BDK_BUTTON2_MASK
				    | BDK_BUTTON3_MASK | BDK_BUTTON4_MASK
				    | BDK_BUTTON5_MASK))
				| key_state;

	  BDK_NOTE (EVENTS_OR_INPUT,
		    g_print ("WINTAB motion: %g,%g\n",
			     event->motion.x, event->motion.y));
	}
      return TRUE;

    case WT_CSRCHANGE:
      if ((bdkdev = bdk_input_find_dev_from_ctx ((HCTX) msg->lParam,
						 packet.pkCursor)) == NULL)
	return FALSE;

      _bdk_device_in_proximity = bdkdev;

      _bdk_input_update_for_device_mode (bdkdev);

      window = NULL;
      if (native_window != _bdk_root)
	window = _bdk_window_get_input_window_for_event (native_window,
							 BDK_PROXIMITY_IN,
							 0,
							 x, y, 0);
      if (window)
	{
	  event->proximity.type = BDK_PROXIMITY_IN;
	  event->proximity.window = window;
	  event->proximity.time = _bdk_win32_get_next_tick (msg->time);
	  event->proximity.device = &_bdk_device_in_proximity->info;
	}

      BDK_NOTE (EVENTS_OR_INPUT,
		g_print ("WINTAB proximity in\n"));

      return TRUE;

    case WT_PROXIMITY:
      /* TODO: Set ignore_core if in input_window */
      if (LOWORD (msg->lParam) == 0)
	{
	  _bdk_input_in_proximity = FALSE;

	  window = NULL;
	  if (native_window != _bdk_root)
	    window = _bdk_window_get_input_window_for_event (native_window,
							     BDK_PROXIMITY_IN,
							     0,
							     x, y, 0);
	  if (window)
	    {
	      event->proximity.type = BDK_PROXIMITY_OUT;
	      event->proximity.window = window;
	      event->proximity.time = _bdk_win32_get_next_tick (msg->time);
	      event->proximity.device = &_bdk_device_in_proximity->info;
	    }

	  BDK_NOTE (EVENTS_OR_INPUT,
		    g_print ("WINTAB proximity out\n"));

	  return TRUE;
	}
      else
	_bdk_input_in_proximity = TRUE;

      _bdk_input_check_proximity ();

      break;
    }
  return FALSE;
}

void
_bdk_input_select_events (BdkWindow *impl_window)
{
  guint event_mask;
  BdkWindowObject *w;
  BdkInputWindow *iw;
  GList *l, *dev_list;
  

  iw = ((BdkWindowObject *)impl_window)->input_window;

  event_mask = 0;
  for (dev_list = _bdk_input_devices; dev_list; dev_list = dev_list->next)
    {
      BdkDevicePrivate *bdkdev = dev_list->data;

      if (!BDK_IS_CORE (bdkdev) &&
	  bdkdev->info.mode != BDK_MODE_DISABLED &&
	  iw != NULL)
	{
	  for (l = iw->windows; l != NULL; l = l->next)
	    {
	      w = l->data;
	      if (bdkdev->info.has_cursor || (w->extension_events & BDK_ALL_DEVICES_MASK))
		event_mask |= w->extension_events;
	    }
	}
    }
  
  event_mask &= ~BDK_ALL_DEVICES_MASK;
  if (event_mask)
    event_mask |= 
      BDK_PROXIMITY_OUT_MASK | BDK_BUTTON_PRESS_MASK | BDK_BUTTON_RELEASE_MASK;

  BDK_WINDOW_IMPL_WIN32 (((BdkWindowObject *)impl_window)->impl)->extension_events_mask = event_mask;
}

gint
_bdk_input_grab_pointer (BdkWindow    *window,
			 gint          owner_events,
			 BdkEventMask  event_mask,
			 BdkWindow    *confine_to,
			 guint32       time)
{
  BDK_NOTE (INPUT, g_print ("_bdk_input_grab_pointer: %p %d %p\n",
			   BDK_WINDOW_HWND (window),
			   owner_events,
			   (confine_to ? BDK_WINDOW_HWND (confine_to) : 0)));

  return BDK_GRAB_SUCCESS;
}

void 
_bdk_input_ungrab_pointer (guint32 time)
{

  BDK_NOTE (INPUT, g_print ("_bdk_input_ungrab_pointer\n"));

}

gboolean
_bdk_device_get_history (BdkDevice         *device,
			 BdkWindow         *window,
			 guint32            start,
			 guint32            stop,
			 BdkTimeCoord    ***events,
			 gint              *n_events)
{
  return FALSE;
}

void 
bdk_device_get_state (BdkDevice       *device,
		      BdkWindow       *window,
		      gdouble         *axes,
		      BdkModifierType *mask)
{
  g_return_if_fail (device != NULL);
  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_IS_CORE (device))
    {
      gint x_int, y_int;
      
      bdk_window_get_pointer (window, &x_int, &y_int, mask);

      if (axes)
	{
	  axes[0] = x_int;
	  axes[1] = y_int;
	}
    }
  else
    {
      BdkDevicePrivate *bdkdev;
      
      bdkdev = (BdkDevicePrivate *)device;
      /* For now just use the last known button and axis state of the device.
       * Since graphical tablets send an insane amount of motion events each
       * second, the info should be fairly up to date */
      if (mask)
	{
	  bdk_window_get_pointer (window, NULL, NULL, mask);
	  *mask &= 0xFF; /* Mask away core pointer buttons */
	  *mask |= ((bdkdev->button_state << 8)
		    & (BDK_BUTTON1_MASK | BDK_BUTTON2_MASK
		       | BDK_BUTTON3_MASK | BDK_BUTTON4_MASK
		       | BDK_BUTTON5_MASK));
	}
      /* For some reason, input_window is sometimes NULL when I use The GIMP 2
       * (bug #141543?). Avoid crashing if debugging is disabled. */
      if (axes && bdkdev->last_axis_data)
	bdk_input_translate_coordinates (bdkdev, window,
					 bdkdev->last_axis_data,
					 axes, NULL, NULL);
    }
}

void
_bdk_input_set_tablet_active (void)
{
  GList *tmp_list;
  HCTX *hctx;

  /* Bring the contexts to the top of the overlap order when one of the
   * application's windows is activated */
  
  if (!wintab_contexts)
    return; /* No tablet devices found, or Wintab not initialized yet */
  
  BDK_NOTE (INPUT, g_print ("_bdk_input_set_tablet_active: "
	"Bringing Wintab contexts to the top of the overlap order\n"));

  tmp_list = wintab_contexts;
  while (tmp_list)
    {
      hctx = (HCTX *) (tmp_list->data);
      (*p_WTOverlap) (*hctx, TRUE);
      tmp_list = tmp_list->next;
    }
}

void 
_bdk_input_init (BdkDisplay *display)
{
  _bdk_input_devices = NULL;

  _bdk_init_input_core (display);
#ifdef WINTAB_NO_LAZY_INIT
  /* Normally, Wintab is only initialized when the application performs
   * an action that requires it, such as enabling extended input events
   * for a window or enumerating the devices.
   */
  _bdk_input_wintab_init_check ();
#endif /* WINTAB_NO_LAZY_INIT */

  _bdk_input_devices = g_list_append (_bdk_input_devices, display->core_pointer);
}

