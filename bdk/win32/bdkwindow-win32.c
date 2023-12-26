/* BDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 1998-2004 Tor Lillqvist
 * Copyright (C) 2001-2009 Hans Breuer
 * Copyright (C) 2007-2009 Cody Russell
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

#include "bdk.h"
#include "bdkwindowimpl.h"
#include "bdkprivate-win32.h"
#include "bdkinput-win32.h"
#include "bdkenumtypes.h"

static BdkColormap* bdk_window_impl_win32_get_colormap (BdkDrawable *drawable);
static void         bdk_window_impl_win32_set_colormap (BdkDrawable *drawable,
							BdkColormap *cmap);

static void bdk_window_impl_win32_get_size   (BdkDrawable        *drawable,
                                              gint               *width,
                                              gint               *height);
static void bdk_window_impl_win32_init       (BdkWindowImplWin32      *window);
static void bdk_window_impl_win32_class_init (BdkWindowImplWin32Class *klass);
static void bdk_window_impl_win32_finalize   (BObject                 *object);

static gpointer parent_class = NULL;
static GSList *modal_window_stack = NULL;

typedef struct _FullscreenInfo FullscreenInfo;

struct _FullscreenInfo
{
  RECT  r;
  guint hint_flags;
  LONG  style;
};

static void     update_style_bits         (BdkWindow         *window);
static gboolean _bdk_window_get_functions (BdkWindow         *window,
                                           BdkWMFunction     *functions);

#define WINDOW_IS_TOPLEVEL(window)		   \
  (BDK_WINDOW_TYPE (window) != BDK_WINDOW_CHILD && \
   BDK_WINDOW_TYPE (window) != BDK_WINDOW_FOREIGN && \
   BDK_WINDOW_TYPE (window) != BDK_WINDOW_OFFSCREEN)

static void bdk_window_impl_iface_init (BdkWindowImplIface *iface);

BdkScreen *
BDK_WINDOW_SCREEN (BObject *win)
{
  return _bdk_screen;
}

GType
_bdk_window_impl_win32_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      const GTypeInfo object_info =
      {
        sizeof (BdkWindowImplWin32Class),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) bdk_window_impl_win32_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (BdkWindowImplWin32),
        0,              /* n_preallocs */
        (GInstanceInitFunc) bdk_window_impl_win32_init,
      };

      const GInterfaceInfo window_impl_info =
      {
	(GInterfaceInitFunc) bdk_window_impl_iface_init,
	NULL,
	NULL
      };
      
      object_type = g_type_register_static (BDK_TYPE_DRAWABLE_IMPL_WIN32,
                                            "BdkWindowImplWin32",
                                            &object_info, 0);
      g_type_add_interface_static (object_type,
				   BDK_TYPE_WINDOW_IMPL,
				   &window_impl_info);
    }
  
  return object_type;
}

GType
_bdk_window_impl_get_type (void)
{
  return _bdk_window_impl_win32_get_type ();
}

static void
bdk_window_impl_win32_get_size (BdkDrawable *drawable,
                                gint        *width,
                                gint        *height)
{
  BdkWindowObject *wrapper;
  BdkDrawableImplWin32 *draw_impl;

  g_return_if_fail (BDK_IS_WINDOW_IMPL_WIN32 (drawable));

  draw_impl = BDK_DRAWABLE_IMPL_WIN32 (drawable);
  wrapper = (BdkWindowObject*) draw_impl->wrapper;

  if (width)
    *width = wrapper->width;
  if (height)
    *height = wrapper->height;
}

static void
bdk_window_impl_win32_init (BdkWindowImplWin32 *impl)
{
  impl->toplevel_window_type = -1;
  impl->hcursor = NULL;
  impl->hicon_big = NULL;
  impl->hicon_small = NULL;
  impl->hint_flags = 0;
  impl->type_hint = BDK_WINDOW_TYPE_HINT_NORMAL;
  impl->extension_events_mask = 0;
  impl->transient_owner = NULL;
  impl->transient_children = NULL;
  impl->num_transients = 0;
  impl->changing_state = FALSE;
}

static void
bdk_window_impl_win32_class_init (BdkWindowImplWin32Class *klass)
{
  BObjectClass *object_class = B_OBJECT_CLASS (klass);
  BdkDrawableClass *drawable_class = BDK_DRAWABLE_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = bdk_window_impl_win32_finalize;

  drawable_class->set_colormap = bdk_window_impl_win32_set_colormap;
  drawable_class->get_colormap = bdk_window_impl_win32_get_colormap;
  drawable_class->get_size = bdk_window_impl_win32_get_size;
}

static void
bdk_window_impl_win32_finalize (BObject *object)
{
  BdkWindowObject *wrapper;
  BdkDrawableImplWin32 *draw_impl;
  BdkWindowImplWin32 *window_impl;
  
  g_return_if_fail (BDK_IS_WINDOW_IMPL_WIN32 (object));

  draw_impl = BDK_DRAWABLE_IMPL_WIN32 (object);
  window_impl = BDK_WINDOW_IMPL_WIN32 (object);
  
  wrapper = (BdkWindowObject*) draw_impl->wrapper;

  if (!BDK_WINDOW_DESTROYED (wrapper))
    {
      bdk_win32_handle_table_remove (draw_impl->handle);
    }

  if (window_impl->hcursor != NULL)
    {
      if (GetCursor () == window_impl->hcursor)
	SetCursor (NULL);

      GDI_CALL (DestroyCursor, (window_impl->hcursor));
      window_impl->hcursor = NULL;
    }

  if (window_impl->hicon_big != NULL)
    {
      GDI_CALL (DestroyIcon, (window_impl->hicon_big));
      window_impl->hicon_big = NULL;
    }

  if (window_impl->hicon_small != NULL)
    {
      GDI_CALL (DestroyIcon, (window_impl->hicon_small));
      window_impl->hicon_small = NULL;
    }

  B_OBJECT_CLASS (parent_class)->finalize (object);
}

void
_bdk_win32_adjust_client_rect (BdkWindow *window,
			       RECT      *rect)
{
  LONG style, exstyle;

  style = GetWindowLong (BDK_WINDOW_HWND (window), GWL_STYLE);
  exstyle = GetWindowLong (BDK_WINDOW_HWND (window), GWL_EXSTYLE);
  API_CALL (AdjustWindowRectEx, (rect, style, FALSE, exstyle));
}

static BdkColormap*
bdk_window_impl_win32_get_colormap (BdkDrawable *drawable)
{
  BdkDrawableImplWin32 *drawable_impl;
  
  g_return_val_if_fail (BDK_IS_WINDOW_IMPL_WIN32 (drawable), NULL);

  drawable_impl = BDK_DRAWABLE_IMPL_WIN32 (drawable);

  if (!((BdkWindowObject *) drawable_impl->wrapper)->input_only && 
      drawable_impl->colormap == NULL)
    {
      drawable_impl->colormap = bdk_screen_get_system_colormap (_bdk_screen);
      g_object_ref (drawable_impl->colormap);
    }
  
  return drawable_impl->colormap;
}

static void
bdk_window_impl_win32_set_colormap (BdkDrawable *drawable,
				    BdkColormap *cmap)
{
  BdkWindowImplWin32 *impl;
  BdkDrawableImplWin32 *draw_impl;
  
  g_return_if_fail (BDK_IS_WINDOW_IMPL_WIN32 (drawable));

  impl = BDK_WINDOW_IMPL_WIN32 (drawable);
  draw_impl = BDK_DRAWABLE_IMPL_WIN32 (drawable);

  /* chain up */
  BDK_DRAWABLE_CLASS (parent_class)->set_colormap (drawable, cmap);
  
  if (cmap)
    {
      /* XXX */
      g_print ("bdk_window_impl_win32_set_colormap: XXX\n");
    }
}

void
_bdk_root_window_size_init (void)
{
  BdkWindowObject *window_object;
  BdkRectangle rect;
  int i;

  window_object = BDK_WINDOW_OBJECT (_bdk_root);
  rect = _bdk_monitors[0].rect;
  for (i = 1; i < _bdk_num_monitors; i++)
    bdk_rectangle_union (&rect, &_bdk_monitors[i].rect, &rect);

  window_object->width = rect.width;
  window_object->height = rect.height;
}

void
_bdk_windowing_window_init (BdkScreen *screen)
{
  BdkWindowObject *private;
  BdkDrawableImplWin32 *draw_impl;

  g_assert (_bdk_root == NULL);
  
  _bdk_root = g_object_new (BDK_TYPE_WINDOW, NULL);
  private = (BdkWindowObject *)_bdk_root;
  private->impl = g_object_new (_bdk_window_impl_get_type (), NULL);
  private->impl_window = private;

  draw_impl = BDK_DRAWABLE_IMPL_WIN32 (private->impl);
  
  draw_impl->handle = GetDesktopWindow ();
  draw_impl->wrapper = BDK_DRAWABLE (private);
  draw_impl->colormap = bdk_screen_get_default_colormap (_bdk_screen);
  g_object_ref (draw_impl->colormap);
  
  private->window_type = BDK_WINDOW_ROOT;
  private->depth = bdk_visual_get_system ()->depth;

  _bdk_root_window_size_init ();

  private->x = 0;
  private->y = 0;
  private->abs_x = 0;
  private->abs_y = 0;
  /* width and height already initialised in _bdk_root_window_size_init() */
  private->viewable = TRUE;

  bdk_win32_handle_table_insert ((HANDLE *) &draw_impl->handle, _bdk_root);

  BDK_NOTE (MISC, g_print ("_bdk_root=%p\n", BDK_WINDOW_HWND (_bdk_root)));
}

static const gchar *
get_default_title (void)
{
  const char *title;
  title = g_get_application_name ();
  if (!title)
    title = g_get_prgname ();

  return title;
}

/* RegisterBdkClass
 *   is a wrapper function for RegisterWindowClassEx.
 *   It creates at least one unique class for every 
 *   BdkWindowType. If support for single window-specific icons
 *   is ever needed (e.g Dialog specific), every such window should
 *   get its own class
 */
static ATOM
RegisterBdkClass (BdkWindowType wtype, BdkWindowTypeHint wtype_hint)
{
  static ATOM klassTOPLEVEL   = 0;
  static ATOM klassDIALOG     = 0;
  static ATOM klassCHILD      = 0;
  static ATOM klassTEMP       = 0;
  static ATOM klassTEMPSHADOW = 0;
  static HICON hAppIcon = NULL;
  static HICON hAppIconSm = NULL;
  static WNDCLASSEXW wcl; 
  ATOM klass = 0;

  wcl.cbSize = sizeof (WNDCLASSEX);
  wcl.style = 0; /* DON'T set CS_<H,V>REDRAW. It causes total redraw
                  * on WM_SIZE and WM_MOVE. Flicker, Performance!
                  */
  wcl.lpfnWndProc = _bdk_win32_window_procedure;
  wcl.cbClsExtra = 0;
  wcl.cbWndExtra = 0;
  wcl.hInstance = _bdk_app_hmodule;
  wcl.hIcon = 0;
  wcl.hIconSm = 0;

  /* initialize once! */
  if (0 == hAppIcon && 0 == hAppIconSm)
    {
      gchar sLoc [MAX_PATH+1];

      if (0 != GetModuleFileName (_bdk_app_hmodule, sLoc, MAX_PATH))
        {
          ExtractIconEx (sLoc, 0, &hAppIcon, &hAppIconSm, 1);

          if (0 == hAppIcon && 0 == hAppIconSm)
            {
              if (0 != GetModuleFileName (_bdk_dll_hinstance, sLoc, MAX_PATH))
		{
		  ExtractIconEx (sLoc, 0, &hAppIcon, &hAppIconSm, 1);
		}
            }
        }

      if (0 == hAppIcon && 0 == hAppIconSm)
        {
          hAppIcon = LoadImage (NULL, IDI_APPLICATION, IMAGE_ICON,
                                GetSystemMetrics (SM_CXICON),
                                GetSystemMetrics (SM_CYICON), 0);
          hAppIconSm = LoadImage (NULL, IDI_APPLICATION, IMAGE_ICON,
                                  GetSystemMetrics (SM_CXSMICON),
                                  GetSystemMetrics (SM_CYSMICON), 0);
        }
    }

  if (0 == hAppIcon)
    hAppIcon = hAppIconSm;
  else if (0 == hAppIconSm)
    hAppIconSm = hAppIcon;

  wcl.lpszMenuName = NULL;

  /* initialize once per class */
  /*
   * HB: Setting the background brush leads to flicker, because we
   * don't get asked how to clear the background. This is not what
   * we want, at least not for input_only windows ...
   */
#define ONCE_PER_CLASS() \
  wcl.hIcon = CopyIcon (hAppIcon); \
  wcl.hIconSm = CopyIcon (hAppIconSm); \
  wcl.hbrBackground = NULL; \
  wcl.hCursor = LoadCursor (NULL, IDC_ARROW); 
  
  switch (wtype)
    {
    case BDK_WINDOW_TOPLEVEL:
      if (0 == klassTOPLEVEL)
	{
	  wcl.lpszClassName = L"bdkWindowToplevel";
	  
	  ONCE_PER_CLASS ();
	  klassTOPLEVEL = RegisterClassExW (&wcl);
	}
      klass = klassTOPLEVEL;
      break;
      
    case BDK_WINDOW_CHILD:
      if (0 == klassCHILD)
	{
	  wcl.lpszClassName = L"bdkWindowChild";
	  
	  wcl.style |= CS_PARENTDC; /* MSDN: ... enhances system performance. */
	  ONCE_PER_CLASS ();
	  klassCHILD = RegisterClassExW (&wcl);
	}
      klass = klassCHILD;
      break;
      
    case BDK_WINDOW_DIALOG:
      if (0 == klassDIALOG)
	{
	  wcl.lpszClassName = L"bdkWindowDialog";
	  wcl.style |= CS_SAVEBITS;
	  ONCE_PER_CLASS ();
	  klassDIALOG = RegisterClassExW (&wcl);
	}
      klass = klassDIALOG;
      break;
      
    case BDK_WINDOW_TEMP:
      if ((wtype_hint == BDK_WINDOW_TYPE_HINT_MENU) ||
          (wtype_hint == BDK_WINDOW_TYPE_HINT_DROPDOWN_MENU) ||
          (wtype_hint == BDK_WINDOW_TYPE_HINT_POPUP_MENU) ||
          (wtype_hint == BDK_WINDOW_TYPE_HINT_TOOLTIP))
        {
          if (klassTEMPSHADOW == 0)
            {
              wcl.lpszClassName = L"bdkWindowTempShadow";
              wcl.style |= CS_SAVEBITS;
              if (LOBYTE (g_win32_get_windows_version()) > 0x05 ||
		  LOWORD (g_win32_get_windows_version()) == 0x0105)
		{
		  /* Windows XP (5.1) or above */
		  wcl.style |= 0x00020000; /* CS_DROPSHADOW */
		}
              ONCE_PER_CLASS ();
              klassTEMPSHADOW = RegisterClassExW (&wcl);
            }

          klass = klassTEMPSHADOW;
        }
       else
        {
          if (klassTEMP == 0)
            {
              wcl.lpszClassName = L"bdkWindowTemp";
              wcl.style |= CS_SAVEBITS;
              ONCE_PER_CLASS ();
              klassTEMP = RegisterClassExW (&wcl);
            }

          klass = klassTEMP;
        }
      break;
      
    default:
      g_assert_not_reached ();
      break;
    }
  
  if (klass == 0)
    {
      WIN32_API_FAILED ("RegisterClassExW");
      g_error ("That is a fatal error");
    }
  return klass;
}

/*
 * Create native windows.
 *
 * With the default Bdk the created windows are mostly toplevel windows.
 * A lot of child windows are only created for BDK_NATIVE_WINDOWS.
 *
 * Placement of the window is derived from the passed in window,
 * except for toplevel window where OS/Window Manager placement
 * is used.
 *
 * The visual parameter, is based on BDK_WA_VISUAL if set already.
 * From attributes the only things used is: colormap, title, 
 * wmclass and type_hint. [1]. We are checking redundant information
 * and complain if that changes, which would break this implementation
 * again.
 *
 * [1] http://mail.bunny.org/archives/btk-devel-list/2010-August/msg00214.html
 */
void
_bdk_window_impl_new (BdkWindow     *window,
		      BdkWindow     *real_parent,
		      BdkScreen     *screen,
		      BdkVisual     *visual,
		      BdkEventMask   event_mask,
		      BdkWindowAttr *attributes,
		      gint           attributes_mask)
{
  HWND hwndNew;
  HANDLE hparent;
  ATOM klass = 0;
  DWORD dwStyle = 0, dwExStyle;
  RECT rect;
  BdkWindowObject *private;
  BdkWindowImplWin32 *impl;
  BdkDrawableImplWin32 *draw_impl;
  const gchar *title;
  wchar_t *wtitle;
  gboolean override_redirect;
  gint window_width, window_height;
  gint offset_x = 0, offset_y = 0;
  gint x, y, real_x = 0, real_y = 0;
  /* check consistency of redundant information */
  guint remaining_mask = attributes_mask;

  private = (BdkWindowObject *)window;

  BDK_NOTE (MISC,
	    g_print ("_bdk_window_impl_new: %s %s\n",
		     (private->window_type == BDK_WINDOW_TOPLEVEL ? "TOPLEVEL" :
		      (private->window_type == BDK_WINDOW_CHILD ? "CHILD" :
		       (private->window_type == BDK_WINDOW_DIALOG ? "DIALOG" :
			(private->window_type == BDK_WINDOW_TEMP ? "TEMP" :
			 "???")))),
		     (attributes->wclass == BDK_INPUT_OUTPUT ? "" : "input-only"))
			   );

  /* to ensure to not miss important information some additional check against
   * attributes which may silently work on X11 */
  if ((attributes_mask & BDK_WA_X) != 0)
    {
      g_assert (attributes->x == private->x);
      remaining_mask &= ~BDK_WA_X;
    }
  if ((attributes_mask & BDK_WA_Y) != 0)
    {
      g_assert (attributes->y == private->y);
      remaining_mask &= ~BDK_WA_Y;
    }
  override_redirect = FALSE;
  if ((attributes_mask & BDK_WA_NOREDIR) != 0)
    {
      override_redirect = !!attributes->override_redirect;
      remaining_mask &= ~BDK_WA_NOREDIR;
    }

  if ((remaining_mask & ~(BDK_WA_WMCLASS|BDK_WA_VISUAL|BDK_WA_CURSOR|BDK_WA_COLORMAP|BDK_WA_TITLE|BDK_WA_TYPE_HINT)) != 0)
    g_warning ("_bdk_window_impl_new: uexpected attribute 0x%X",
               remaining_mask & ~(BDK_WA_WMCLASS|BDK_WA_VISUAL|BDK_WA_CURSOR|BDK_WA_COLORMAP|BDK_WA_TITLE|BDK_WA_TYPE_HINT));

  hparent = BDK_WINDOW_HWND (real_parent);

  impl = g_object_new (_bdk_window_impl_get_type (), NULL);
  private->impl = (BdkDrawable *)impl;
  draw_impl = BDK_DRAWABLE_IMPL_WIN32 (impl);
  draw_impl->wrapper = BDK_DRAWABLE (window);

  if (attributes_mask & BDK_WA_VISUAL)
    g_assert (visual == attributes->visual);

  impl->extension_events_mask = 0;
  impl->override_redirect = override_redirect;

  /* wclass is not any longer set always, but if is ... */
  if ((attributes_mask & BDK_WA_WMCLASS) == BDK_WA_WMCLASS)
    g_assert ((attributes->wclass == BDK_INPUT_OUTPUT) == !private->input_only);

  if (!private->input_only)
    {
      dwExStyle = 0;

      private->input_only = FALSE;
      private->depth = visual->depth;
      
      if (attributes_mask & BDK_WA_COLORMAP)
	{
	  draw_impl->colormap = attributes->colormap;
	  g_object_ref (attributes->colormap);
	}
      else
	{
	  draw_impl->colormap = bdk_screen_get_system_colormap (_bdk_screen);
	  g_object_ref (draw_impl->colormap);
	}
    }
  else
    {
      /* I very much doubt using WS_EX_TRANSPARENT actually
       * corresponds to how X11 InputOnly windows work, but it appears
       * to work well enough for the actual use cases in btk.
       */
      dwExStyle = WS_EX_TRANSPARENT;
      private->depth = 0; /* xxx: was 0 for years */
      private->input_only = TRUE;
      draw_impl->colormap = bdk_screen_get_system_colormap (_bdk_screen);
      g_object_ref (draw_impl->colormap);
      BDK_NOTE (MISC, g_print ("... BDK_INPUT_ONLY, system colormap\n"));
    }

  if (attributes_mask & BDK_WA_TITLE)
    title = attributes->title;
  else
    title = get_default_title ();
  if (!title || !*title)
    title = "";

  impl->native_event_mask = BDK_STRUCTURE_MASK | event_mask;
      
  if (attributes_mask & BDK_WA_TYPE_HINT)
    bdk_window_set_type_hint (window, attributes->type_hint);

  if (impl->type_hint == BDK_WINDOW_TYPE_HINT_UTILITY)
    dwExStyle |= WS_EX_TOOLWINDOW;

  switch (private->window_type)
    {
    case BDK_WINDOW_TOPLEVEL:
    case BDK_WINDOW_DIALOG:
      if (BDK_WINDOW_TYPE (private->parent) != BDK_WINDOW_ROOT)
	{
	  /* The common code warns for this case. */
	  hparent = GetDesktopWindow ();
	}
      /* Children of foreign windows aren't toplevel windows */
      if (BDK_WINDOW_TYPE (real_parent) == BDK_WINDOW_FOREIGN)
	{
	  dwStyle = WS_CHILDWINDOW | WS_CLIPCHILDREN;
	}
      else
	{
	  if (private->window_type == BDK_WINDOW_TOPLEVEL)
	    dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
	  else
	    dwStyle = WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU | WS_CAPTION | WS_THICKFRAME | WS_CLIPCHILDREN;

	  offset_x = _bdk_offset_x;
	  offset_y = _bdk_offset_y;
	}
      break;

    case BDK_WINDOW_CHILD:
      dwStyle = WS_CHILDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
      break;

    case BDK_WINDOW_TEMP:
      /* A temp window is not necessarily a top level window */
      dwStyle = (_bdk_root == real_parent ? WS_POPUP : WS_CHILDWINDOW);
      dwStyle |= WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
      dwExStyle |= WS_EX_TOOLWINDOW | WS_EX_TOPMOST;
      offset_x = _bdk_offset_x;
      offset_y = _bdk_offset_y;
      break;

    default:
      g_assert_not_reached ();
    }

  if (private->window_type != BDK_WINDOW_CHILD)
    {
      rect.left = private->x;
      rect.top = private->y;
      rect.right = private->width + private->x;
      rect.bottom = private->height + private->y;

      AdjustWindowRectEx (&rect, dwStyle, FALSE, dwExStyle);

      real_x = private->x - offset_x;
      real_y = private->y - offset_y;

      if (private->window_type == BDK_WINDOW_TOPLEVEL ||
	  private->window_type == BDK_WINDOW_DIALOG)
	{
	  /* We initially place it at default so that we can get the
	     default window positioning if we want */
	  x = y = CW_USEDEFAULT;
	}
      else
	{
	  /* TEMP, FOREIGN: Put these where requested */
	  x = real_x;
	  y = real_y;
	}

      window_width = rect.right - rect.left;
      window_height = rect.bottom - rect.top;
    }
  else
    {
      /* adjust position relative to real_parent */
      window_width = private->width;
      window_height = private->height;
      /* use given position for initial placement, native coordinates */
      x = private->x + private->parent->abs_x - offset_x;
      y = private->y + private->parent->abs_y - offset_y;
    }

  klass = RegisterBdkClass (private->window_type, impl->type_hint);

  wtitle = g_utf8_to_utf16 (title, -1, NULL, NULL, NULL);
  
  hwndNew = CreateWindowExW (dwExStyle,
			     MAKEINTRESOURCEW (klass),
			     wtitle,
			     dwStyle,
			     x,
			     y,
			     window_width, window_height,
			     hparent,
			     NULL,
			     _bdk_app_hmodule,
			     window);
  if (BDK_WINDOW_HWND (window) != hwndNew)
    {
      g_warning ("bdk_window_new: bdk_event_translate::WM_CREATE (%p, %p) HWND mismatch.",
		 BDK_WINDOW_HWND (window),
		 hwndNew);

      /* HB: IHMO due to a race condition the handle was increased by
       * one, which causes much trouble. Because I can't find the 
       * real bug, try to workaround it ...
       * To reproduce: compile with MSVC 5, DEBUG=1
       */
# if 0
      bdk_win32_handle_table_remove (BDK_WINDOW_HWND (window));
      BDK_WINDOW_HWND (window) = hwndNew;
      bdk_win32_handle_table_insert (&BDK_WINDOW_HWND (window), window);
# else
      /* the old behaviour, but with warning */
      draw_impl->handle = hwndNew;
# endif

    }

  if (private->window_type != BDK_WINDOW_CHILD)
    {
      GetWindowRect (BDK_WINDOW_HWND (window), &rect);
      impl->initial_x = rect.left;
      impl->initial_y = rect.top;

      /* Now we know the initial position, move to actually specified position */
      if (real_x != x || real_y != y)
	{
	  API_CALL (SetWindowPos, (BDK_WINDOW_HWND (window), NULL,
				   real_x, real_y, 0, 0,
				   SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER));
	}
    }

  g_object_ref (window);
  bdk_win32_handle_table_insert (&BDK_WINDOW_HWND (window), window);

  BDK_NOTE (MISC, g_print ("... \"%s\" %dx%d@%+d%+d %p = %p\n",
			   title,
			   window_width, window_height,
			   private->x - offset_x,
			   private->y - offset_y, 
			   hparent,
			   BDK_WINDOW_HWND (window)));

  /* Add window handle to title */
  BDK_NOTE (MISC_OR_EVENTS, bdk_window_set_title (window, title));

  g_free (wtitle);

  if (draw_impl->handle == NULL)
    {
      WIN32_API_FAILED ("CreateWindowExW");
      g_object_unref (window);
      return;
    }

//  if (!from_set_skip_taskbar_hint && private->window_type == BDK_WINDOW_TEMP)
//    bdk_window_set_skip_taskbar_hint (window, TRUE);

  if (attributes_mask & BDK_WA_CURSOR)
    bdk_window_set_cursor (window, attributes->cursor);
}

BdkWindow *
bdk_window_foreign_new_for_display (BdkDisplay      *display,
                                    BdkNativeWindow  anid)
{
  return bdk_win32_window_foreign_new_for_display (display, anid);
}

BdkWindow *
bdk_win32_window_foreign_new_for_display (BdkDisplay      *display,
                                          BdkNativeWindow  anid)
{
  BdkWindow *window;
  BdkWindowObject *private;
  BdkWindowImplWin32 *impl;
  BdkDrawableImplWin32 *draw_impl;

  HANDLE parent;
  RECT rect;
  POINT point;

  g_return_val_if_fail (display == _bdk_display, NULL);

  window = g_object_new (BDK_TYPE_WINDOW, NULL);
  private = (BdkWindowObject *)window;
  private->impl = g_object_new (_bdk_window_impl_get_type (), NULL);
  impl = BDK_WINDOW_IMPL_WIN32 (private->impl);
  draw_impl = BDK_DRAWABLE_IMPL_WIN32 (private->impl);
  draw_impl->wrapper = BDK_DRAWABLE (window);
  parent = GetParent ((HWND)anid);
  
  private->parent = bdk_win32_handle_table_lookup ((BdkNativeWindow) parent);
  if (!private->parent || BDK_WINDOW_TYPE (private->parent) == BDK_WINDOW_FOREIGN)
    private->parent = (BdkWindowObject *)_bdk_root;
  
  private->parent->children = g_list_prepend (private->parent->children, window);

  draw_impl->handle = (HWND) anid;
  GetClientRect ((HWND) anid, &rect);
  point.x = rect.left;
  point.y = rect.right;
  ClientToScreen ((HWND) anid, &point);
  if (parent != GetDesktopWindow ())
    ScreenToClient (parent, &point);
  private->x = point.x;
  private->y = point.y;
  private->width = rect.right - rect.left;
  private->height = rect.bottom - rect.top;
  private->window_type = BDK_WINDOW_FOREIGN;
  private->destroyed = FALSE;
  private->event_mask = BDK_ALL_EVENTS_MASK; /* XXX */
  if (IsWindowVisible ((HWND) anid))
    private->state &= (~BDK_WINDOW_STATE_WITHDRAWN);
  else
    private->state |= BDK_WINDOW_STATE_WITHDRAWN;
  if (GetWindowLong ((HWND)anid, GWL_EXSTYLE) & WS_EX_TOPMOST)
    private->state |= BDK_WINDOW_STATE_ABOVE;
  else
    private->state &= (~BDK_WINDOW_STATE_ABOVE);
  private->state &= (~BDK_WINDOW_STATE_BELOW);
  private->viewable = TRUE;

  private->depth = bdk_visual_get_system ()->depth;

  g_object_ref (window);
  bdk_win32_handle_table_insert (&BDK_WINDOW_HWND (window), window);

  BDK_NOTE (MISC, g_print ("bdk_window_foreign_new_for_display: %p: %s@%+d%+d\n",
			   (HWND) anid,
			   _bdk_win32_drawable_description (window),
			   private->x, private->y));

  return window;
}

BdkWindow*
bdk_window_lookup (BdkNativeWindow hwnd)
{
  return (BdkWindow*) bdk_win32_handle_table_lookup (hwnd); 
}

void
_bdk_win32_window_destroy (BdkWindow *window,
			   gboolean   recursing,
			   gboolean   foreign_destroy)
{
  BdkWindowObject *private = (BdkWindowObject *)window;
  BdkWindowImplWin32 *window_impl = BDK_WINDOW_IMPL_WIN32 (private->impl);
  GSList *tmp;

  g_return_if_fail (BDK_IS_WINDOW (window));
  
  BDK_NOTE (MISC, g_print ("_bdk_win32_window_destroy: %p\n",
			   BDK_WINDOW_HWND (window)));

  /* Remove ourself from the modal stack */
  _bdk_remove_modal_window (window);

  /* Remove all our transient children */
  tmp = window_impl->transient_children;
  while (tmp != NULL)
    {
      BdkWindow *child = tmp->data;
      BdkWindowImplWin32 *child_impl = BDK_WINDOW_IMPL_WIN32 (BDK_WINDOW_OBJECT (child)->impl);

      child_impl->transient_owner = NULL;
      tmp = b_slist_next (tmp);
    }
  b_slist_free (window_impl->transient_children);
  window_impl->transient_children = NULL;

  /* Remove ourself from our transient owner */
  if (window_impl->transient_owner != NULL)
    {
      bdk_window_set_transient_for (window, NULL);
    }

  if (!recursing && !foreign_destroy)
    {
      _bdk_win32_drawable_finish (private->impl);

      private->destroyed = TRUE;
      DestroyWindow (BDK_WINDOW_HWND (window));
    }
}

void
_bdk_windowing_window_destroy_foreign (BdkWindow *window)
{
  /* It's somebody else's window, but in our hierarchy, so reparent it
   * to the desktop, and then try to destroy it.
   */
  bdk_window_hide (window);
  bdk_window_reparent (window, NULL, 0, 0);
  
  PostMessage (BDK_WINDOW_HWND (window), WM_CLOSE, 0, 0);
}

/* This function is called when the window really gone.
 */
void
bdk_window_destroy_notify (BdkWindow *window)
{
  g_return_if_fail (BDK_IS_WINDOW (window));

  BDK_NOTE (EVENTS,
	    g_print ("bdk_window_destroy_notify: %p%s\n",
		     BDK_WINDOW_HWND (window),
		     (BDK_WINDOW_DESTROYED (window) ? " (destroyed)" : "")));

  if (!BDK_WINDOW_DESTROYED (window))
    {
      if (BDK_WINDOW_TYPE (window) != BDK_WINDOW_FOREIGN)
	g_warning ("window %p unexpectedly destroyed",
		   BDK_WINDOW_HWND (window));

      _bdk_window_destroy (window, TRUE);
    }
  
  bdk_win32_handle_table_remove (BDK_WINDOW_HWND (window));
  g_object_unref (window);
}

static void
get_outer_rect (BdkWindow *window,
		gint       width,
		gint       height,
		RECT      *rect)
{
  rect->left = rect->top = 0;
  rect->right = width;
  rect->bottom = height;
      
  _bdk_win32_adjust_client_rect (window, rect);
}

static void
adjust_for_gravity_hints (BdkWindow *window,
			  RECT      *outer_rect,
			  gint		*x,
			  gint		*y)
{
	BdkWindowObject *obj;
	BdkWindowImplWin32 *impl;

	obj = BDK_WINDOW_OBJECT (window);
	impl = BDK_WINDOW_IMPL_WIN32 (obj->impl);

  if (impl->hint_flags & BDK_HINT_WIN_GRAVITY)
    {
#ifdef G_ENABLE_DEBUG
      gint orig_x = *x, orig_y = *y;
#endif

      switch (impl->hints.win_gravity)
	{
	case BDK_GRAVITY_NORTH:
	case BDK_GRAVITY_CENTER:
	case BDK_GRAVITY_SOUTH:
	  *x -= (outer_rect->right - outer_rect->left) / 2;
	  *x += obj->width / 2;
	  break;
	      
	case BDK_GRAVITY_SOUTH_EAST:
	case BDK_GRAVITY_EAST:
	case BDK_GRAVITY_NORTH_EAST:
	  *x -= outer_rect->right - outer_rect->left;
	  *x += obj->width;
	  break;

	case BDK_GRAVITY_STATIC:
	  *x += outer_rect->left;
	  break;

	default:
	  break;
	}

      switch (impl->hints.win_gravity)
	{
	case BDK_GRAVITY_WEST:
	case BDK_GRAVITY_CENTER:
	case BDK_GRAVITY_EAST:
	  *y -= (outer_rect->bottom - outer_rect->top) / 2;
	  *y += obj->height / 2;
	  break;

	case BDK_GRAVITY_SOUTH_WEST:
	case BDK_GRAVITY_SOUTH:
	case BDK_GRAVITY_SOUTH_EAST:
	  *y -= outer_rect->bottom - outer_rect->top;
	  *y += obj->height;
	  break;

	case BDK_GRAVITY_STATIC:
	  *y += outer_rect->top;
	  break;

	default:
	  break;
	}
      BDK_NOTE (MISC,
		(orig_x != *x || orig_y != *y) ?
		g_print ("adjust_for_gravity_hints: x: %d->%d, y: %d->%d\n",
			 orig_x, *x, orig_y, *y)
		  : (void) 0);
    }
}

static void
show_window_internal (BdkWindow *window,
                      gboolean   already_mapped,
		      gboolean   deiconify)
{
  BdkWindowObject *private;
  BdkWindowImplWin32 *window_impl;
  gboolean focus_on_map = FALSE;
  DWORD exstyle;

  private = (BdkWindowObject *) window;

  if (private->destroyed)
    return;

  BDK_NOTE (MISC, g_print ("show_window_internal: %p: %s%s\n",
			   BDK_WINDOW_HWND (window),
			   _bdk_win32_window_state_to_string (private->state),
			   (deiconify ? " deiconify" : "")));
  
  /* If asked to show (not deiconify) an withdrawn and iconified
   * window, do that.
   */
  if (!deiconify &&
      !already_mapped &&
      (private->state & BDK_WINDOW_STATE_ICONIFIED))
    {	
      ShowWindow (BDK_WINDOW_HWND (window), SW_SHOWMINNOACTIVE);
      return;
    }
  
  /* If asked to just show an iconified window, do nothing. */
  if (!deiconify && (private->state & BDK_WINDOW_STATE_ICONIFIED))
    return;
  
  /* If asked to deiconify an already noniconified window, do
   * nothing. (Especially, don't cause the window to rise and
   * activate. There are different calls for that.)
   */
  if (deiconify && !(private->state & BDK_WINDOW_STATE_ICONIFIED))
    return;
  
  /* If asked to show (but not raise) a window that is already
   * visible, do nothing.
   */
  if (!deiconify && !already_mapped && IsWindowVisible (BDK_WINDOW_HWND (window)))
    return;

  /* Other cases */
  
  if (!already_mapped)
    focus_on_map = private->focus_on_map;

  exstyle = GetWindowLong (BDK_WINDOW_HWND (window), GWL_EXSTYLE);

  /* Use SetWindowPos to show transparent windows so automatic redraws
   * in other windows can be suppressed.
   */
  if (exstyle & WS_EX_TRANSPARENT)
    {
      UINT flags = SWP_SHOWWINDOW | SWP_NOREDRAW | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER;

      if (BDK_WINDOW_TYPE (window) == BDK_WINDOW_TEMP || !focus_on_map)
	flags |= SWP_NOACTIVATE;

      SetWindowPos (BDK_WINDOW_HWND (window), HWND_TOP, 0, 0, 0, 0, flags);

      return;
    }

  /* For initial map of "normal" windows we want to emulate WM window
   * positioning behaviour, which means: 
   * + Use user specified position if BDK_HINT_POS or BDK_HINT_USER_POS
   * otherwise:
   * + default to the initial CW_USEDEFAULT placement,
   *   no matter if the user moved the window before showing it.
   * + Certain window types and hints have more elaborate positioning
   *   schemes.
   */
  window_impl = BDK_WINDOW_IMPL_WIN32 (private->impl);
  if (!already_mapped &&
      (BDK_WINDOW_TYPE (window) == BDK_WINDOW_TOPLEVEL ||
       BDK_WINDOW_TYPE (window) == BDK_WINDOW_DIALOG) &&
      (window_impl->hint_flags & (BDK_HINT_POS | BDK_HINT_USER_POS)) == 0 &&
      !window_impl->override_redirect)
    {
      gboolean center = FALSE;
      RECT window_rect, center_on_rect;
      int x, y;

      x = window_impl->initial_x;
      y = window_impl->initial_y;

      if (window_impl->type_hint == BDK_WINDOW_TYPE_HINT_SPLASHSCREEN)
	{
	  HMONITOR monitor;
	  MONITORINFO mi;

	  monitor = MonitorFromWindow (BDK_WINDOW_HWND (window), MONITOR_DEFAULTTONEAREST);
	  mi.cbSize = sizeof (mi);
	  if (monitor && GetMonitorInfo (monitor, &mi))
	    center_on_rect = mi.rcMonitor;
	  else
	    {
	      center_on_rect.left = 0;
	      center_on_rect.right = 0;
	      center_on_rect.right = GetSystemMetrics (SM_CXSCREEN);
	      center_on_rect.bottom = GetSystemMetrics (SM_CYSCREEN);
	    }
	  center = TRUE;
	}
      else if (window_impl->transient_owner != NULL &&
	       BDK_WINDOW_IS_MAPPED (window_impl->transient_owner))
	{
	  BdkWindowObject *owner = BDK_WINDOW_OBJECT (window_impl->transient_owner);
	  /* Center on transient parent */
	  center_on_rect.left = owner->x;
	  center_on_rect.top = owner->y;
	  center_on_rect.right = center_on_rect.left + owner->width;
	  center_on_rect.bottom = center_on_rect.top + owner->height;
	  _bdk_win32_adjust_client_rect (BDK_WINDOW (owner), &center_on_rect);
	  center = TRUE;
	}

      if (center) 
	{
	  window_rect.left = 0;
	  window_rect.top = 0;
	  window_rect.right = private->width;
	  window_rect.bottom = private->height;
	  _bdk_win32_adjust_client_rect (window, &window_rect);

	  x = center_on_rect.left + ((center_on_rect.right - center_on_rect.left) - (window_rect.right - window_rect.left)) / 2;
	  y = center_on_rect.top + ((center_on_rect.bottom - center_on_rect.top) - (window_rect.bottom - window_rect.top)) / 2;
	}

      API_CALL (SetWindowPos, (BDK_WINDOW_HWND (window), NULL,
			       x, y, 0, 0,
			       SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER));
    }

  if (!already_mapped &&
      (BDK_WINDOW_TYPE (window) == BDK_WINDOW_TOPLEVEL ||
       BDK_WINDOW_TYPE (window) == BDK_WINDOW_DIALOG) &&
      !window_impl->override_redirect)
    {
      /* Ensure new windows are fully onscreen */
      RECT window_rect;
      HMONITOR monitor;
      MONITORINFO mi;
      int x, y;

      GetWindowRect (BDK_WINDOW_HWND (window), &window_rect);

      monitor = MonitorFromWindow (BDK_WINDOW_HWND (window), MONITOR_DEFAULTTONEAREST);
      mi.cbSize = sizeof (mi);
      if (monitor && GetMonitorInfo (monitor, &mi))
	{
	  x = window_rect.left;
	  y = window_rect.top;

	  if (window_rect.right > mi.rcWork.right)
	    {
	      window_rect.left -= (window_rect.right - mi.rcWork.right);
	      window_rect.right -= (window_rect.right - mi.rcWork.right);
	    }

	  if (window_rect.bottom > mi.rcWork.bottom)
	    {
	      window_rect.top -= (window_rect.bottom - mi.rcWork.bottom);
	      window_rect.bottom -= (window_rect.bottom - mi.rcWork.bottom);
	    }

	  if (window_rect.left < mi.rcWork.left)
	    {
	      window_rect.right += (mi.rcWork.left - window_rect.left);
	      window_rect.left += (mi.rcWork.left - window_rect.left);
	    }

	  if (window_rect.top < mi.rcWork.top)
	    {
	      window_rect.bottom += (mi.rcWork.top - window_rect.top);
	      window_rect.top += (mi.rcWork.top - window_rect.top);
	    }

	  if (x != window_rect.left || y != window_rect.top)
	    API_CALL (SetWindowPos, (BDK_WINDOW_HWND (window), NULL,
				     window_rect.left, window_rect.top, 0, 0,
				     SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER));
	}
    }


  if (private->state & BDK_WINDOW_STATE_FULLSCREEN)
    {
      bdk_window_fullscreen (window);
    }
  else if (private->state & BDK_WINDOW_STATE_MAXIMIZED)
    {
      ShowWindow (BDK_WINDOW_HWND (window), SW_MAXIMIZE);
    }
  else if (private->state & BDK_WINDOW_STATE_ICONIFIED)
    {
      if (focus_on_map)
	ShowWindow (BDK_WINDOW_HWND (window), SW_RESTORE);
      else
	ShowWindow (BDK_WINDOW_HWND (window), SW_SHOWNOACTIVATE);
    }
  else if (BDK_WINDOW_TYPE (window) == BDK_WINDOW_TEMP || !focus_on_map)
    {
      if (!IsWindowVisible (BDK_WINDOW_HWND (window)))
        ShowWindow (BDK_WINDOW_HWND (window), SW_SHOWNOACTIVATE);
      else
        ShowWindow (BDK_WINDOW_HWND (window), SW_SHOWNA);
    }
  else if (!IsWindowVisible (BDK_WINDOW_HWND (window)))
    {
      ShowWindow (BDK_WINDOW_HWND (window), SW_SHOWNORMAL);
    }
  else
    {
      ShowWindow (BDK_WINDOW_HWND (window), SW_SHOW);
    }

  /* Sync STATE_ABOVE to TOPMOST */
  if (BDK_WINDOW_TYPE (window) != BDK_WINDOW_TEMP &&
      (((private->state & BDK_WINDOW_STATE_ABOVE) &&
       !(exstyle & WS_EX_TOPMOST)) ||
      (!(private->state & BDK_WINDOW_STATE_ABOVE) &&
       (exstyle & WS_EX_TOPMOST))))
    {
      API_CALL (SetWindowPos, (BDK_WINDOW_HWND (window),
			       (private->state & BDK_WINDOW_STATE_ABOVE)?HWND_TOPMOST:HWND_NOTOPMOST,
			       0, 0, 0, 0,
			       SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE));
    }
}

static void
bdk_win32_window_show (BdkWindow *window, 
		       gboolean already_mapped)
{
  show_window_internal (window, already_mapped, FALSE);
}

static void
bdk_win32_window_hide (BdkWindow *window)
{
  BdkWindowObject *private;
  
  private = (BdkWindowObject*) window;
  if (private->destroyed)
    return;

  BDK_NOTE (MISC, g_print ("bdk_win32_window_hide: %p: %s\n",
			   BDK_WINDOW_HWND (window),
			   _bdk_win32_window_state_to_string (private->state)));
  
  if (BDK_WINDOW_IS_MAPPED (window))
    bdk_synthesize_window_state (window,
				 0,
				 BDK_WINDOW_STATE_WITHDRAWN);
  
  _bdk_window_clear_update_area (window);
  
  if (BDK_WINDOW_TYPE (window) == BDK_WINDOW_TOPLEVEL)
    ShowOwnedPopups (BDK_WINDOW_HWND (window), FALSE);
  
  if (GetWindowLong (BDK_WINDOW_HWND (window), GWL_EXSTYLE) & WS_EX_TRANSPARENT)
    {
      SetWindowPos (BDK_WINDOW_HWND (window), HWND_BOTTOM,
		    0, 0, 0, 0,
		    SWP_HIDEWINDOW | SWP_NOREDRAW | SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE);
    }
  else
    {
      ShowWindow (BDK_WINDOW_HWND (window), SW_HIDE);
    }
}

static void
bdk_win32_window_withdraw (BdkWindow *window)
{
  BdkWindowObject *private;
  
  private = (BdkWindowObject*) window;
  if (private->destroyed)
    return;

  BDK_NOTE (MISC, g_print ("bdk_win32_window_withdraw: %p: %s\n",
			   BDK_WINDOW_HWND (window),
			   _bdk_win32_window_state_to_string (private->state)));
  
  bdk_window_hide (window);	/* ??? */
}

static void
bdk_win32_window_move (BdkWindow *window,
		       gint x, gint y)
{
  BdkWindowObject *private = (BdkWindowObject *)window;
  BdkWindowImplWin32 *impl;

  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_WINDOW_DESTROYED (window))
    return;

  BDK_NOTE (MISC, g_print ("bdk_win32_window_move: %p: %+d%+d\n",
                           BDK_WINDOW_HWND (window), x, y));

  impl = BDK_WINDOW_IMPL_WIN32 (private->impl);

  if (private->state & BDK_WINDOW_STATE_FULLSCREEN)
    return;

  /* Don't check BDK_WINDOW_TYPE (private) == BDK_WINDOW_CHILD.
   * Foreign windows (another app's windows) might be children of our
   * windows! Especially in the case of btkplug/socket.
   */
  if (GetAncestor (BDK_WINDOW_HWND (window), GA_PARENT) != GetDesktopWindow ())
    {
      _bdk_window_move_resize_child (window, x, y, private->width, private->height);
    }
  else
    {
      RECT outer_rect;

      get_outer_rect (window, private->width, private->height, &outer_rect);

      adjust_for_gravity_hints (window, &outer_rect, &x, &y);

      BDK_NOTE (MISC, g_print ("... SetWindowPos(%p,NULL,%d,%d,0,0,"
                               "NOACTIVATE|NOSIZE|NOZORDER)\n",
                               BDK_WINDOW_HWND (window),
                               x - _bdk_offset_x, y - _bdk_offset_y));

      API_CALL (SetWindowPos, (BDK_WINDOW_HWND (window), NULL,
                               x - _bdk_offset_x, y - _bdk_offset_y, 0, 0,
                               SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER));
    }
}

static void
bdk_win32_window_resize (BdkWindow *window,
			 gint width, gint height)
{
  BdkWindowObject *private = (BdkWindowObject*) window;
  BdkWindowImplWin32 *impl;

  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_WINDOW_DESTROYED (window))
    return;

  if (width < 1)
    width = 1;
  if (height < 1)
    height = 1;

  BDK_NOTE (MISC, g_print ("bdk_win32_window_resize: %p: %dx%d\n",
                           BDK_WINDOW_HWND (window), width, height));

  impl = BDK_WINDOW_IMPL_WIN32 (private->impl);

  if (private->state & BDK_WINDOW_STATE_FULLSCREEN)
    return;

  if (GetAncestor (BDK_WINDOW_HWND (window), GA_PARENT) != GetDesktopWindow ())
    {
      _bdk_window_move_resize_child (window, private->x, private->y, width, height);
    }
  else
    {
      RECT outer_rect;

      get_outer_rect (window, width, height, &outer_rect);

      BDK_NOTE (MISC, g_print ("... SetWindowPos(%p,NULL,0,0,%ld,%ld,"
                               "NOACTIVATE|NOMOVE|NOZORDER)\n",
                               BDK_WINDOW_HWND (window),
                               outer_rect.right - outer_rect.left,
                               outer_rect.bottom - outer_rect.top));

      API_CALL (SetWindowPos, (BDK_WINDOW_HWND (window), NULL,
                               0, 0,
                               outer_rect.right - outer_rect.left,
                               outer_rect.bottom - outer_rect.top,
                               SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER));
      private->resize_count += 1;
    }
}

static void
bdk_win32_window_move_resize_internal (BdkWindow *window,
				       gint       x,
				       gint       y,
				       gint       width,
				       gint       height)
{
  BdkWindowObject *private;
  BdkWindowImplWin32 *impl;

  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_WINDOW_DESTROYED (window))
    return;

  if (width < 1)
    width = 1;
  if (height < 1)
    height = 1;

  private = BDK_WINDOW_OBJECT (window);
  impl = BDK_WINDOW_IMPL_WIN32 (private->impl);

  if (private->state & BDK_WINDOW_STATE_FULLSCREEN)
    return;

  BDK_NOTE (MISC, g_print ("bdk_win32_window_move_resize: %p: %dx%d@%+d%+d\n",
                           BDK_WINDOW_HWND (window),
                           width, height, x, y));

  if (GetAncestor (BDK_WINDOW_HWND (window), GA_PARENT) != GetDesktopWindow ())
    {
      _bdk_window_move_resize_child (window, x, y, width, height);
    }
  else
    {
      RECT outer_rect;

      get_outer_rect (window, width, height, &outer_rect);

      adjust_for_gravity_hints (window, &outer_rect, &x, &y);

      BDK_NOTE (MISC, g_print ("... SetWindowPos(%p,NULL,%d,%d,%ld,%ld,"
                               "NOACTIVATE|NOZORDER)\n",
                               BDK_WINDOW_HWND (window),
                               x - _bdk_offset_x, y - _bdk_offset_y,
                               outer_rect.right - outer_rect.left,
                               outer_rect.bottom - outer_rect.top));

      API_CALL (SetWindowPos, (BDK_WINDOW_HWND (window), NULL,
                               x - _bdk_offset_x, y - _bdk_offset_y,
                               outer_rect.right - outer_rect.left,
                               outer_rect.bottom - outer_rect.top,
                               SWP_NOACTIVATE | SWP_NOZORDER));
    }
}

static void
bdk_win32_window_move_resize (BdkWindow *window,
			      gboolean   with_move,
			      gint       x,
			      gint       y,
			      gint       width,
			      gint       height)
{
  BdkWindowObject *private = (BdkWindowObject *)window;
  BdkWindowImplWin32 *window_impl;

  window_impl = BDK_WINDOW_IMPL_WIN32 (private->impl);
  window_impl->inhibit_configure = TRUE;

  /* We ignore changes to the window being moved or resized by the 
     user, as we don't want to fight the user */
  if (BDK_WINDOW_HWND (window) == _modal_move_resize_window)
    goto out;

  if (with_move && (width < 0 && height < 0))
    {
      bdk_win32_window_move (window, x, y);
    }
  else
    {
      if (with_move)
	{
	  bdk_win32_window_move_resize_internal (window, x, y, width, height);
	}
      else
	{
	  bdk_win32_window_resize (window, width, height);
	}
    }

 out:
  window_impl->inhibit_configure = FALSE;

  if (WINDOW_IS_TOPLEVEL (window))
    _bdk_win32_emit_configure_event (window);
}

static gboolean
bdk_win32_window_reparent (BdkWindow *window,
			   BdkWindow *new_parent,
			   gint       x,
			   gint       y)
{
  BdkWindowObject *window_private;
  BdkWindowObject *parent_private;
  BdkWindowObject *old_parent_private;
  BdkWindowImplWin32 *impl;
  gboolean was_toplevel;
  LONG style;

  if (!new_parent)
    new_parent = _bdk_root;

  window_private = (BdkWindowObject*) window;
  old_parent_private = (BdkWindowObject *) window_private->parent;
  parent_private = (BdkWindowObject*) new_parent;
  impl = BDK_WINDOW_IMPL_WIN32 (window_private->impl);

  BDK_NOTE (MISC, g_print ("bdk_win32_window_reparent: %p: %p\n",
			   BDK_WINDOW_HWND (window),
			   BDK_WINDOW_HWND (new_parent)));

  style = GetWindowLong (BDK_WINDOW_HWND (window), GWL_STYLE);

  was_toplevel = GetAncestor (BDK_WINDOW_HWND (window), GA_PARENT) == GetDesktopWindow ();
  if (was_toplevel && new_parent != _bdk_root)
    {
      /* Reparenting from top-level (child of desktop). Clear out
       * decorations.
       */
      style &= ~(WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
      style |= WS_CHILD;
      SetWindowLong (BDK_WINDOW_HWND (window), GWL_STYLE, style);
    }
  else if (new_parent == _bdk_root)
    {
      /* Reparenting to top-level. Add decorations. */
      style &= ~(WS_CHILD);
      style |= WS_OVERLAPPEDWINDOW;
      SetWindowLong (BDK_WINDOW_HWND (window), GWL_STYLE, style);
    }

  API_CALL (SetParent, (BDK_WINDOW_HWND (window),
			BDK_WINDOW_HWND (new_parent)));
  
  API_CALL (MoveWindow, (BDK_WINDOW_HWND (window),
			 x, y, window_private->width, window_private->height, TRUE));

  /* From here on, we treat parents of type BDK_WINDOW_FOREIGN like
   * the root window
   */
  if (BDK_WINDOW_TYPE (new_parent) == BDK_WINDOW_FOREIGN)
    new_parent = _bdk_root;
  
  window_private->parent = (BdkWindowObject *)new_parent;

  /* Switch the window type as appropriate */

  switch (BDK_WINDOW_TYPE (new_parent))
    {
    case BDK_WINDOW_ROOT:
      if (impl->toplevel_window_type != -1)
	BDK_WINDOW_TYPE (window) = impl->toplevel_window_type;
      else if (BDK_WINDOW_TYPE (window) == BDK_WINDOW_CHILD)
	BDK_WINDOW_TYPE (window) = BDK_WINDOW_TOPLEVEL;
      break;

    case BDK_WINDOW_TOPLEVEL:
    case BDK_WINDOW_CHILD:
    case BDK_WINDOW_DIALOG:
    case BDK_WINDOW_TEMP:
      if (WINDOW_IS_TOPLEVEL (window))
	{
	  /* Save the original window type so we can restore it if the
	   * window is reparented back to be a toplevel.
	   */
	  impl->toplevel_window_type = BDK_WINDOW_TYPE (window);
	  BDK_WINDOW_TYPE (window) = BDK_WINDOW_CHILD;
	}
    }

  if (old_parent_private)
    old_parent_private->children =
      g_list_remove (old_parent_private->children, window);

  parent_private->children = g_list_prepend (parent_private->children, window);

  return FALSE;
}

static void
erase_background (BdkWindow *window,
		  HDC        hdc)
{
  HDC bgdc = NULL;
  HBRUSH hbr = NULL;
  HPALETTE holdpal = NULL;
  RECT rect;
  COLORREF bg;
  BdkColormap *colormap;
  BdkColormapPrivateWin32 *colormap_private;
  int x, y;
  int x_offset, y_offset;
  
  if (((BdkWindowObject *) window)->input_only ||
      ((BdkWindowObject *) window)->bg_pixmap == BDK_NO_BG)
    {
      return;
    }

  colormap = bdk_drawable_get_colormap (window);

  if (colormap &&
      (colormap->visual->type == BDK_VISUAL_PSEUDO_COLOR ||
       colormap->visual->type == BDK_VISUAL_STATIC_COLOR))
    {
      int k;
	  
      colormap_private = BDK_WIN32_COLORMAP_DATA (colormap);

      if (!(holdpal = SelectPalette (hdc,  colormap_private->hpal, FALSE)))
        WIN32_GDI_FAILED ("SelectPalette");
      else if ((k = RealizePalette (hdc)) == GDI_ERROR)
	WIN32_GDI_FAILED ("RealizePalette");
      else if (k > 0)
	BDK_NOTE (COLORMAP, g_print ("erase_background: realized %p: %d colors\n",
				     colormap_private->hpal, k));
    }
  
  x_offset = y_offset = 0;
  while (window && ((BdkWindowObject *) window)->bg_pixmap == BDK_PARENT_RELATIVE_BG)
    {
      /* If this window should have the same background as the parent,
       * fetch the parent. (And if the same goes for the parent, fetch
       * the grandparent, etc.)
       */
      x_offset += ((BdkWindowObject *) window)->x;
      y_offset += ((BdkWindowObject *) window)->y;
      window = BDK_WINDOW (((BdkWindowObject *) window)->parent);
    }
  
  GetClipBox (hdc, &rect);

  if (((BdkWindowObject *) window)->bg_pixmap == NULL)
    {
      bg = _bdk_win32_colormap_color (BDK_DRAWABLE_IMPL_WIN32 (((BdkWindowObject *) window)->impl)->colormap,
				      ((BdkWindowObject *) window)->bg_color.pixel);
      
      if (!(hbr = CreateSolidBrush (bg)))
	WIN32_GDI_FAILED ("CreateSolidBrush");
      else if (!FillRect (hdc, &rect, hbr))
	WIN32_GDI_FAILED ("FillRect");
      if (hbr != NULL)
	DeleteObject (hbr);
    }
  else if (((BdkWindowObject *) window)->bg_pixmap != BDK_NO_BG)
    {
      BdkPixmap *pixmap = ((BdkWindowObject *) window)->bg_pixmap;
      BdkPixmapImplWin32 *pixmap_impl = BDK_PIXMAP_IMPL_WIN32 (BDK_PIXMAP_OBJECT (pixmap)->impl);
      
      if (x_offset == 0 && y_offset == 0 &&
	  pixmap_impl->width <= 8 && pixmap_impl->height <= 8)
	{
	  if (!(hbr = CreatePatternBrush (BDK_PIXMAP_HBITMAP (pixmap))))
	    WIN32_GDI_FAILED ("CreatePatternBrush");
	  else if (!FillRect (hdc, &rect, hbr))
	    WIN32_GDI_FAILED ("FillRect");
	  if (hbr != NULL)
	    DeleteObject (hbr);
	}
      else
	{
	  HGDIOBJ oldbitmap;

	  if (!(bgdc = CreateCompatibleDC (hdc)))
	    {
	      WIN32_GDI_FAILED ("CreateCompatibleDC");
	      return;
	    }
	  if (!(oldbitmap = SelectObject (bgdc, BDK_PIXMAP_HBITMAP (pixmap))))
	    {
	      WIN32_GDI_FAILED ("SelectObject");
	      DeleteDC (bgdc);
	      return;
	    }
	  x = -x_offset;
	  while (x < rect.right)
	    {
	      if (x + pixmap_impl->width >= rect.left)
		{
		  y = -y_offset;
		  while (y < rect.bottom)
		    {
		      if (y + pixmap_impl->height >= rect.top)
			{
			  if (!BitBlt (hdc, x, y,
				       pixmap_impl->width, pixmap_impl->height,
				       bgdc, 0, 0, SRCCOPY))
			    {
			      WIN32_GDI_FAILED ("BitBlt");
			      SelectObject (bgdc, oldbitmap);
			      DeleteDC (bgdc);
			      return;
			    }
			}
		      y += pixmap_impl->height;
		    }
		}
	      x += pixmap_impl->width;
	    }
	  SelectObject (bgdc, oldbitmap);
	  DeleteDC (bgdc);
	}
    }
}

static void
bdk_win32_window_clear_area (BdkWindow *window,
			     gint       x,
			     gint       y,
			     gint       width,
			     gint       height,
			     gboolean   send_expose)
{
  BdkWindowObject *private = (BdkWindowObject *)window;

  if (!BDK_WINDOW_DESTROYED (window))
    {
      HDC hdc;
      RECT rect;

      hdc = GetDC (BDK_WINDOW_HWND (window));

      if (!send_expose)
	{
	  if (width == 0)
	    width = private->width - x;
	  if (height == 0)
	    height = private->height - y;
	  BDK_NOTE (MISC, g_print ("_bdk_windowing_window_clear_area: %p: "
				   "%dx%d@%+d%+d\n",
				   BDK_WINDOW_HWND (window),
				   width, height, x, y));
	  IntersectClipRect (hdc, x, y, x + width, y + height);
	  erase_background (window, hdc);
	  GDI_CALL (ReleaseDC, (BDK_WINDOW_HWND (window), hdc));
	}
      else
	{
	  /* The background should be erased before the expose event is
	     generated */
	  IntersectClipRect (hdc, x, y, x + width, y + height);
	  erase_background (window, hdc);
	  GDI_CALL (ReleaseDC, (BDK_WINDOW_HWND (window), hdc));

	  rect.left = x;
	  rect.right = x + width;
	  rect.top = y;
	  rect.bottom = y + height;

	  GDI_CALL (InvalidateRect, (BDK_WINDOW_HWND (window), &rect, TRUE));
	  UpdateWindow (BDK_WINDOW_HWND (window));
	}
    }
}

static void
bdk_window_win32_clear_rebunnyion (BdkWindow *window,
			     BdkRebunnyion *rebunnyion,
			     gboolean   send_expose)
{
  BdkRectangle *rectangles;
  int n_rectangles, i;

  bdk_rebunnyion_get_rectangles  (rebunnyion,
			      &rectangles,
			      &n_rectangles);

  for (i = 0; i < n_rectangles; i++)
    bdk_win32_window_clear_area (window,
		rectangles[i].x, rectangles[i].y,
		rectangles[i].width, rectangles[i].height,
		send_expose);

  g_free (rectangles);
}

static void
bdk_win32_window_raise (BdkWindow *window)
{
  if (!BDK_WINDOW_DESTROYED (window))
    {
      BDK_NOTE (MISC, g_print ("bdk_win32_window_raise: %p\n",
			       BDK_WINDOW_HWND (window)));

      if (BDK_WINDOW_TYPE (window) == BDK_WINDOW_TEMP)
        API_CALL (SetWindowPos, (BDK_WINDOW_HWND (window), HWND_TOPMOST,
	                         0, 0, 0, 0,
				 SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE));
      else if (((BdkWindowObject *)window)->accept_focus)
        /* Do not wrap this in an API_CALL macro as SetForegroundWindow might
         * fail when for example dragging a window belonging to a different
         * application at the time of a btk_window_present() call due to focus
         * stealing prevention. */
        SetForegroundWindow (BDK_WINDOW_HWND (window));
      else
        API_CALL (SetWindowPos, (BDK_WINDOW_HWND (window), HWND_TOP,
  			         0, 0, 0, 0,
			         SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE));
    }
}

static void
bdk_win32_window_lower (BdkWindow *window)
{
  if (!BDK_WINDOW_DESTROYED (window))
    {
      BDK_NOTE (MISC, g_print ("bdk_win32_window_lower: %p\n"
			       "... SetWindowPos(%p,HWND_BOTTOM,0,0,0,0,"
			       "NOACTIVATE|NOMOVE|NOSIZE)\n",
			       BDK_WINDOW_HWND (window),
			       BDK_WINDOW_HWND (window)));

      API_CALL (SetWindowPos, (BDK_WINDOW_HWND (window), HWND_BOTTOM,
			       0, 0, 0, 0,
			       SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE));
    }
}

void
bdk_window_set_hints (BdkWindow *window,
		      gint       x,
		      gint       y,
		      gint       min_width,
		      gint       min_height,
		      gint       max_width,
		      gint       max_height,
		      gint       flags)
{
  /* Note that this function is obsolete */

  BdkWindowImplWin32 *impl;

  g_return_if_fail (BDK_IS_WINDOW (window));
  
  if (BDK_WINDOW_DESTROYED (window))
    return;
  
  impl = BDK_WINDOW_IMPL_WIN32 (BDK_WINDOW_OBJECT (window)->impl);

  BDK_NOTE (MISC, g_print ("bdk_window_set_hints: %p: %dx%d..%dx%d @%+d%+d\n",
			   BDK_WINDOW_HWND (window),
			   min_width, min_height, max_width, max_height,
			   x, y));

  if (flags)
    {
      BdkGeometry geom;
      gint geom_mask = 0;

      geom.min_width  = min_width;
      geom.min_height = min_height;
      geom.max_width  = max_width;
      geom.max_height = max_height;

      if (flags & BDK_HINT_MIN_SIZE)
        geom_mask |= BDK_HINT_MIN_SIZE;

      if (flags & BDK_HINT_MAX_SIZE)
        geom_mask |= BDK_HINT_MAX_SIZE;

      bdk_window_set_geometry_hints (window, &geom, geom_mask);
    }
}

void
bdk_window_set_urgency_hint (BdkWindow *window,
			     gboolean   urgent)
{
  FLASHWINFO flashwinfo;
  typedef BOOL (WINAPI *PFN_FlashWindowEx) (FLASHWINFO*);
  PFN_FlashWindowEx flashWindowEx = NULL;

  g_return_if_fail (BDK_IS_WINDOW (window));
  g_return_if_fail (BDK_WINDOW_TYPE (window) != BDK_WINDOW_CHILD);
  
  if (BDK_WINDOW_DESTROYED (window))
    return;

  flashWindowEx = (PFN_FlashWindowEx) GetProcAddress (GetModuleHandle ("user32.dll"), "FlashWindowEx");

  if (flashWindowEx)
    {
      flashwinfo.cbSize = sizeof (flashwinfo);
      flashwinfo.hwnd = BDK_WINDOW_HWND (window);
      if (urgent)
	flashwinfo.dwFlags = FLASHW_ALL | FLASHW_TIMER;
      else
	flashwinfo.dwFlags = FLASHW_STOP;
      flashwinfo.uCount = 0;
      flashwinfo.dwTimeout = 0;
      
      flashWindowEx (&flashwinfo);
    }
  else
    {
      FlashWindow (BDK_WINDOW_HWND (window), urgent);
    }
}

static gboolean
get_effective_window_decorations (BdkWindow       *window,
                                  BdkWMDecoration *decoration)
{
  BdkWindowImplWin32 *impl;

  impl = (BdkWindowImplWin32 *)((BdkWindowObject *)window)->impl;

  if (bdk_window_get_decorations (window, decoration))
    return TRUE;
    
  if (((BdkWindowObject *) window)->window_type != BDK_WINDOW_TOPLEVEL &&
      ((BdkWindowObject *) window)->window_type != BDK_WINDOW_DIALOG)
    {
      return FALSE;
    }

  if ((impl->hint_flags & BDK_HINT_MIN_SIZE) &&
      (impl->hint_flags & BDK_HINT_MAX_SIZE) &&
      impl->hints.min_width == impl->hints.max_width &&
      impl->hints.min_height == impl->hints.max_height)
    {
      *decoration = BDK_DECOR_ALL | BDK_DECOR_RESIZEH | BDK_DECOR_MAXIMIZE;

      if (impl->type_hint == BDK_WINDOW_TYPE_HINT_DIALOG ||
	  impl->type_hint == BDK_WINDOW_TYPE_HINT_MENU ||
	  impl->type_hint == BDK_WINDOW_TYPE_HINT_TOOLBAR)
	{
	  *decoration |= BDK_DECOR_MINIMIZE;
	}
      else if (impl->type_hint == BDK_WINDOW_TYPE_HINT_SPLASHSCREEN)
	{
	  *decoration |= BDK_DECOR_MENU | BDK_DECOR_MINIMIZE;
	}

      return TRUE;
    }
  else if (impl->hint_flags & BDK_HINT_MAX_SIZE)
    {
      *decoration = BDK_DECOR_ALL | BDK_DECOR_MAXIMIZE;
      if (impl->type_hint == BDK_WINDOW_TYPE_HINT_DIALOG ||
	  impl->type_hint == BDK_WINDOW_TYPE_HINT_MENU ||
	  impl->type_hint == BDK_WINDOW_TYPE_HINT_TOOLBAR)
	{
	  *decoration |= BDK_DECOR_MINIMIZE;
	}

      return TRUE;
    }
  else
    {
      switch (impl->type_hint)
	{
	case BDK_WINDOW_TYPE_HINT_DIALOG:
	  *decoration = (BDK_DECOR_ALL | BDK_DECOR_MINIMIZE | BDK_DECOR_MAXIMIZE);
	  return TRUE;

	case BDK_WINDOW_TYPE_HINT_MENU:
	  *decoration = (BDK_DECOR_ALL | BDK_DECOR_RESIZEH | BDK_DECOR_MINIMIZE | BDK_DECOR_MAXIMIZE);
	  return TRUE;

	case BDK_WINDOW_TYPE_HINT_TOOLBAR:
	case BDK_WINDOW_TYPE_HINT_UTILITY:
	  bdk_window_set_skip_taskbar_hint (window, TRUE);
	  bdk_window_set_skip_pager_hint (window, TRUE);
	  *decoration = (BDK_DECOR_ALL | BDK_DECOR_MINIMIZE | BDK_DECOR_MAXIMIZE);
	  return TRUE;

	case BDK_WINDOW_TYPE_HINT_SPLASHSCREEN:
	  *decoration = (BDK_DECOR_ALL | BDK_DECOR_RESIZEH | BDK_DECOR_MENU |
			 BDK_DECOR_MINIMIZE | BDK_DECOR_MAXIMIZE);
	  return TRUE;
	  
	case BDK_WINDOW_TYPE_HINT_DOCK:
	  return FALSE;
	  
	case BDK_WINDOW_TYPE_HINT_DESKTOP:
	  return FALSE;

	default:
	  /* Fall thru */
	case BDK_WINDOW_TYPE_HINT_NORMAL:
	  *decoration = BDK_DECOR_ALL;
	  return TRUE;
	}
    }
    
  return FALSE;
}

void 
bdk_window_set_geometry_hints (BdkWindow         *window,
			       const BdkGeometry *geometry,
			       BdkWindowHints     geom_mask)
{
  BdkWindowImplWin32 *impl;
  FullscreenInfo *fi;

  g_return_if_fail (BDK_IS_WINDOW (window));
  
  if (BDK_WINDOW_DESTROYED (window))
    return;

  BDK_NOTE (MISC, g_print ("bdk_window_set_geometry_hints: %p\n",
			   BDK_WINDOW_HWND (window)));

  impl = BDK_WINDOW_IMPL_WIN32 (BDK_WINDOW_OBJECT (window)->impl);

  fi = g_object_get_data (B_OBJECT (window), "fullscreen-info");
  if (fi)
    fi->hint_flags = geom_mask;
  else
    impl->hint_flags = geom_mask;
  impl->hints = *geometry;

  if (geom_mask & BDK_HINT_POS)
    ; /* even the X11 mplementation doesn't care */

  if (geom_mask & BDK_HINT_MIN_SIZE)
    {
      BDK_NOTE (MISC, g_print ("... MIN_SIZE: %dx%d\n",
			       geometry->min_width, geometry->min_height));
    }
  
  if (geom_mask & BDK_HINT_MAX_SIZE)
    {
      BDK_NOTE (MISC, g_print ("... MAX_SIZE: %dx%d\n",
			       geometry->max_width, geometry->max_height));
    }

  if (geom_mask & BDK_HINT_BASE_SIZE)
    {
      BDK_NOTE (MISC, g_print ("... BASE_SIZE: %dx%d\n",
			       geometry->base_width, geometry->base_height));
    }
  
  if (geom_mask & BDK_HINT_RESIZE_INC)
    {
      BDK_NOTE (MISC, g_print ("... RESIZE_INC: (%d,%d)\n",
			       geometry->width_inc, geometry->height_inc));
    }
  
  if (geom_mask & BDK_HINT_ASPECT)
    {
      BDK_NOTE (MISC, g_print ("... ASPECT: %g--%g\n",
			       geometry->min_aspect, geometry->max_aspect));
    }

  if (geom_mask & BDK_HINT_WIN_GRAVITY)
    {
      BDK_NOTE (MISC, g_print ("... GRAVITY: %d\n", geometry->win_gravity));
    }

  update_style_bits (window);
}

void
bdk_window_set_title (BdkWindow   *window,
		      const gchar *title)
{
  wchar_t *wtitle;

  g_return_if_fail (BDK_IS_WINDOW (window));
  g_return_if_fail (title != NULL);

  if (BDK_WINDOW_DESTROYED (window))
    return;

  /* Empty window titles not allowed, so set it to just a period. */
  if (!title[0])
    title = ".";
  
  BDK_NOTE (MISC, g_print ("bdk_window_set_title: %p: %s\n",
			   BDK_WINDOW_HWND (window), title));
  
  BDK_NOTE (MISC_OR_EVENTS, title = g_strdup_printf ("%p %s", BDK_WINDOW_HWND (window), title));

  wtitle = g_utf8_to_utf16 (title, -1, NULL, NULL, NULL);
  API_CALL (SetWindowTextW, (BDK_WINDOW_HWND (window), wtitle));
  g_free (wtitle);

  BDK_NOTE (MISC_OR_EVENTS, g_free ((char *) title));
}

void          
bdk_window_set_role (BdkWindow   *window,
		     const gchar *role)
{
  g_return_if_fail (BDK_IS_WINDOW (window));
  
  BDK_NOTE (MISC, g_print ("bdk_window_set_role: %p: %s\n",
			   BDK_WINDOW_HWND (window),
			   (role ? role : "NULL")));
  /* XXX */
}

void
bdk_window_set_transient_for (BdkWindow *window, 
			      BdkWindow *parent)
{
  HWND window_id, parent_id;
  BdkWindowImplWin32 *window_impl = BDK_WINDOW_IMPL_WIN32 (BDK_WINDOW_OBJECT (window)->impl);
  BdkWindowImplWin32 *parent_impl = NULL;
  GSList *item;

  g_return_if_fail (BDK_IS_WINDOW (window));

  window_id = BDK_WINDOW_HWND (window);
  parent_id = parent != NULL ? BDK_WINDOW_HWND (parent) : NULL;

  BDK_NOTE (MISC, g_print ("bdk_window_set_transient_for: %p: %p\n", window_id, parent_id));

  if (BDK_WINDOW_DESTROYED (window) || (parent && BDK_WINDOW_DESTROYED (parent)))
    {
      if (BDK_WINDOW_DESTROYED (window))
	BDK_NOTE (MISC, g_print ("... destroyed!\n"));
      else
	BDK_NOTE (MISC, g_print ("... owner destroyed!\n"));

      return;
    }

  if (((BdkWindowObject *) window)->window_type == BDK_WINDOW_CHILD)
    {
      BDK_NOTE (MISC, g_print ("... a child window!\n"));
      return;
    }

  if (parent == NULL)
    {
      BdkWindowImplWin32 *trans_impl = BDK_WINDOW_IMPL_WIN32 (BDK_WINDOW_OBJECT (window_impl->transient_owner)->impl);
      if (trans_impl->transient_children != NULL)
        {
          item = b_slist_find (trans_impl->transient_children, window);
          item->data = NULL;
          trans_impl->transient_children = b_slist_delete_link (trans_impl->transient_children, item);
          trans_impl->num_transients--;

          if (!trans_impl->num_transients)
            {
              trans_impl->transient_children = NULL;
            }
        }
      g_object_unref (B_OBJECT (window_impl->transient_owner));
      g_object_unref (B_OBJECT (window));

      window_impl->transient_owner = NULL;
    }
  else
    {
      parent_impl = BDK_WINDOW_IMPL_WIN32 (BDK_WINDOW_OBJECT (parent)->impl);

      parent_impl->transient_children = b_slist_append (parent_impl->transient_children, window);
      g_object_ref (B_OBJECT (window));
      parent_impl->num_transients++;
      window_impl->transient_owner = parent;
      g_object_ref (B_OBJECT (parent));
    }

  /* This changes the *owner* of the window, despite the misleading
   * name. (Owner and parent are unrelated concepts.) At least that's
   * what people who seem to know what they talk about say on
   * USENET. Search on Google.
   */
  SetLastError (0);
  if (SetWindowLongPtr (window_id, GWLP_HWNDPARENT, (LONG_PTR) parent_id) == 0 &&
      GetLastError () != 0)
    WIN32_API_FAILED ("SetWindowLongPtr");
}

void
_bdk_push_modal_window (BdkWindow *window)
{
  modal_window_stack = b_slist_prepend (modal_window_stack,
                                        window);
}

void
_bdk_remove_modal_window (BdkWindow *window)
{
  GSList *tmp;

  g_return_if_fail (window != NULL);

  /* It's possible to be NULL here if someone sets the modal hint of the window
   * to FALSE before a modal window stack has ever been created. */
  if (modal_window_stack == NULL)
    return;

  /* Find the requested window in the stack and remove it.  Yeah, I realize this
   * means we're not a 'real stack', strictly speaking.  Sue me. :) */
  tmp = b_slist_find (modal_window_stack, window);
  if (tmp != NULL)
    {
      modal_window_stack = b_slist_delete_link (modal_window_stack, tmp);
    }
}

gboolean
_bdk_modal_blocked (BdkWindow *window)
{
  GSList *l;
  gboolean found_any = FALSE;

  for (l = modal_window_stack; l != NULL; l = l->next)
    {
      BdkWindow *modal = l->data;

      if (modal == window)
	return FALSE;

      if (BDK_WINDOW_IS_MAPPED (modal))
	found_any = TRUE;
    }

  return found_any;
}

BdkWindow *
_bdk_modal_current (void)
{
  GSList *l;

  for (l = modal_window_stack; l != NULL; l = l->next)
    {
      BdkWindow *modal = l->data;

      if (BDK_WINDOW_IS_MAPPED (modal))
	return modal;
    }

  return NULL;
}

static void
bdk_win32_window_set_background (BdkWindow      *window,
				 const BdkColor *color)
{
  BdkWindowObject *private = (BdkWindowObject *)window;
  
  BDK_NOTE (MISC, g_print ("bdk_win32_window_set_background: %p: %s\n",
			   BDK_WINDOW_HWND (window), 
			   _bdk_win32_color_to_string (color)));

  private->bg_color = *color;

  if (private->bg_pixmap &&
      private->bg_pixmap != BDK_PARENT_RELATIVE_BG &&
      private->bg_pixmap != BDK_NO_BG)
    {
      g_object_unref (private->bg_pixmap);
      private->bg_pixmap = NULL;
    }
}

static void
bdk_win32_window_set_back_pixmap (BdkWindow *window,
				  BdkPixmap *pixmap)
{
  BdkWindowObject *private = (BdkWindowObject *)window;

  if (pixmap != BDK_PARENT_RELATIVE_BG &&
      pixmap != BDK_NO_BG &&
      pixmap && !bdk_drawable_get_colormap (pixmap))
    {
      g_warning ("bdk_window_set_back_pixmap(): pixmap must have a colormap");
      return;
    }
  
  if (private->bg_pixmap &&
      private->bg_pixmap != BDK_PARENT_RELATIVE_BG &&
      private->bg_pixmap != BDK_NO_BG)
    g_object_unref (private->bg_pixmap);

  if (pixmap != BDK_PARENT_RELATIVE_BG &&
      pixmap != BDK_NO_BG &&
      pixmap)
    {
      g_object_ref (pixmap);
      private->bg_pixmap = pixmap;
    }
  else
    {
      private->bg_pixmap = pixmap;
    }
}

static void
bdk_win32_window_set_cursor (BdkWindow *window,
			     BdkCursor *cursor)
{
  BdkWindowImplWin32 *impl;
  BdkCursorPrivate *cursor_private;
  BdkWindowObject *parent_window;
  HCURSOR hcursor;
  HCURSOR hprevcursor;
  
  impl = BDK_WINDOW_IMPL_WIN32 (BDK_WINDOW_OBJECT (window)->impl);
  cursor_private = (BdkCursorPrivate*) cursor;
  
  if (BDK_WINDOW_DESTROYED (window))
    return;

  if (!cursor)
    hcursor = NULL;
  else
    hcursor = cursor_private->hcursor;
  
  BDK_NOTE (MISC, g_print ("bdk_win32_window_set_cursor: %p: %p\n",
			   BDK_WINDOW_HWND (window),
			   hcursor));

  /* First get the old cursor, if any (we wait to free the old one
   * since it may be the current cursor set in the Win32 API right
   * now).
   */
  hprevcursor = impl->hcursor;

  if (hcursor == NULL)
    impl->hcursor = NULL;
  else
    {
      /* We must copy the cursor as it is OK to destroy the BdkCursor
       * while still in use for some window. See for instance
       * gimp_change_win_cursor() which calls bdk_window_set_cursor
       * (win, cursor), and immediately afterwards bdk_cursor_destroy
       * (cursor).
       */
      if ((impl->hcursor = CopyCursor (hcursor)) == NULL)
	WIN32_API_FAILED ("CopyCursor");
      BDK_NOTE (MISC, g_print ("... CopyCursor (%p) = %p\n",
			       hcursor, impl->hcursor));
    }

  if (impl->hcursor != NULL)
    {
      /* If the pointer is over our window, set new cursor */
      BdkWindow *curr_window = bdk_window_get_pointer (window, NULL, NULL, NULL);
      if (curr_window == window ||
	  (curr_window && window == bdk_window_get_toplevel (curr_window)))
        SetCursor (impl->hcursor);
      else
	{
	  /* Climb up the tree and find whether our window is the
	   * first ancestor that has cursor defined, and if so, set
	   * new cursor.
	   */
	  BdkWindowObject *curr_window_obj = BDK_WINDOW_OBJECT (curr_window);
	  while (curr_window_obj &&
		 !BDK_WINDOW_IMPL_WIN32 (curr_window_obj->impl)->hcursor)
	    {
	      curr_window_obj = curr_window_obj->parent;
	      if (curr_window_obj == BDK_WINDOW_OBJECT (window))
		{
	          SetCursor (impl->hcursor);
		  break;
		}
	    }
	}
    }

  /* Destroy the previous cursor: Need to make sure it's no longer in
   * use before we destroy it, in case we're not over our window but
   * the cursor is still set to our old one.
   */
  if (hprevcursor != NULL)
    {
      if (GetCursor () == hprevcursor)
	{
	  /* Look for a suitable cursor to use instead */
	  hcursor = NULL;
          parent_window = BDK_WINDOW_OBJECT (window)->parent;
          while (hcursor == NULL)
	    {
	      if (parent_window)
		{
		  impl = BDK_WINDOW_IMPL_WIN32 (parent_window->impl);
		  hcursor = impl->hcursor;
		  parent_window = parent_window->parent;
		}
	      else
		{
		  hcursor = LoadCursor (NULL, IDC_ARROW);
		}
	    }
          SetCursor (hcursor);
        }

      BDK_NOTE (MISC, g_print ("... DestroyCursor (%p)\n", hprevcursor));
      
      API_CALL (DestroyCursor, (hprevcursor));
    }
}

static void
bdk_win32_window_get_geometry (BdkWindow *window,
			       gint      *x,
			       gint      *y,
			       gint      *width,
			       gint      *height,
			       gint      *depth)
{
  if (!window)
    window = _bdk_root;
  
  if (!BDK_WINDOW_DESTROYED (window))
    {
      RECT rect;

      API_CALL (GetClientRect, (BDK_WINDOW_HWND (window), &rect));

      if (window != _bdk_root)
	{
	  POINT pt;
	  BdkWindow *parent = bdk_window_get_parent (window);

	  pt.x = rect.left;
	  pt.y = rect.top;
	  ClientToScreen (BDK_WINDOW_HWND (window), &pt);
	  ScreenToClient (BDK_WINDOW_HWND (parent), &pt);
	  rect.left = pt.x;
	  rect.top = pt.y;

	  pt.x = rect.right;
	  pt.y = rect.bottom;
	  ClientToScreen (BDK_WINDOW_HWND (window), &pt);
	  ScreenToClient (BDK_WINDOW_HWND (parent), &pt);
	  rect.right = pt.x;
	  rect.bottom = pt.y;

	  if (parent == _bdk_root)
	    {
	      rect.left += _bdk_offset_x;
	      rect.top += _bdk_offset_y;
	      rect.right += _bdk_offset_x;
	      rect.bottom += _bdk_offset_y;
	    }
	}

      if (x)
	*x = rect.left;
      if (y)
	*y = rect.top;
      if (width)
	*width = rect.right - rect.left;
      if (height)
	*height = rect.bottom - rect.top;
      if (depth)
	*depth = bdk_drawable_get_visual (window)->depth;

      BDK_NOTE (MISC, g_print ("bdk_win32_window_get_geometry: %p: %ldx%ldx%d@%+ld%+ld\n",
			       BDK_WINDOW_HWND (window),
			       rect.right - rect.left, rect.bottom - rect.top,
			       bdk_drawable_get_visual (window)->depth,
			       rect.left, rect.top));
    }
}

static gint
bdk_win32_window_get_root_coords (BdkWindow *window,
				  gint       x,
				  gint       y,
				  gint      *root_x,
				  gint      *root_y)
{
  gint tx;
  gint ty;
  POINT pt;

  pt.x = x;
  pt.y = y;
  ClientToScreen (BDK_WINDOW_HWND (window), &pt);
  tx = pt.x;
  ty = pt.y;
  
  if (root_x)
    *root_x = tx + _bdk_offset_x;
  if (root_y)
    *root_y = ty + _bdk_offset_y;

  BDK_NOTE (MISC, g_print ("bdk_win32_window_get_root_coords: %p: %+d%+d %+d%+d\n",
			   BDK_WINDOW_HWND (window),
			   x, y,
			   tx + _bdk_offset_x, ty + _bdk_offset_y));
  return 1;
}

static gboolean
bdk_win32_window_get_deskrelative_origin (BdkWindow *window,
					  gint      *x,
					  gint      *y)
{
  return bdk_win32_window_get_root_coords (window, 0, 0, x, y);
}

static void
bdk_win32_window_restack_under (BdkWindow *window,
				GList *native_siblings)
{
  GList *list;

  /* input order is bottom-most first */
  for (list = native_siblings;;)
    {
      HWND lower = list->data, upper;

      list = list->next;
      if (!list)
	break;
      upper = list->data;
      API_CALL (SetWindowPos, (upper, lower, 0, 0, 0, 0, 
		  SWP_NOMOVE|SWP_NOSIZE|SWP_NOREDRAW));
    }

}

static void
bdk_win32_window_restack_toplevel (BdkWindow *window,
				   BdkWindow *sibling,
				   gboolean   above)
{
  HWND lower = above ? BDK_WINDOW_HWND (sibling) : BDK_WINDOW_HWND (window);
  HWND upper = above ? BDK_WINDOW_HWND (window) : BDK_WINDOW_HWND (sibling);

  API_CALL (SetWindowPos, (upper, lower, 0, 0, 0, 0, 
	      SWP_NOMOVE|SWP_NOSIZE|SWP_NOREDRAW));
}

void
bdk_window_get_root_origin (BdkWindow *window,
			    gint      *x,
			    gint      *y)
{
  BdkRectangle rect;

  g_return_if_fail (BDK_IS_WINDOW (window));

  bdk_window_get_frame_extents (window, &rect);

  if (x)
    *x = rect.x;

  if (y)
    *y = rect.y;

  BDK_NOTE (MISC, g_print ("bdk_window_get_root_origin: %p: %+d%+d\n",
			   BDK_WINDOW_HWND (window), rect.x, rect.y));
}

void
bdk_window_get_frame_extents (BdkWindow    *window,
                              BdkRectangle *rect)
{
  BdkWindowObject *private;
  HWND hwnd;
  RECT r;

  g_return_if_fail (BDK_IS_WINDOW (window));
  g_return_if_fail (rect != NULL);

  private = BDK_WINDOW_OBJECT (window);

  rect->x = 0;
  rect->y = 0;
  rect->width = 1;
  rect->height = 1;
  
  if (BDK_WINDOW_DESTROYED (window))
    return;

  /* FIXME: window is documented to be a toplevel BdkWindow, so is it really
   * necessary to walk its parent chain?
   */
  while (private->parent && ((BdkWindowObject*) private->parent)->parent)
    private = (BdkWindowObject*) private->parent;

  hwnd = BDK_WINDOW_HWND (window);
  API_CALL (GetWindowRect, (hwnd, &r));

  rect->x = r.left + _bdk_offset_x;
  rect->y = r.top + _bdk_offset_y;
  rect->width = r.right - r.left;
  rect->height = r.bottom - r.top;

  BDK_NOTE (MISC, g_print ("bdk_window_get_frame_extents: %p: %ldx%ld@%+ld%+ld\n",
			   BDK_WINDOW_HWND (window),
			   r.right - r.left, r.bottom - r.top,
			   r.left, r.top));
}


static BdkModifierType
get_current_mask (void)
{
  BdkModifierType mask;
  BYTE kbd[256];

  GetKeyboardState (kbd);
  mask = 0;
  if (kbd[VK_SHIFT] & 0x80)
    mask |= BDK_SHIFT_MASK;
  if (kbd[VK_CAPITAL] & 0x80)
    mask |= BDK_LOCK_MASK;
  if (kbd[VK_CONTROL] & 0x80)
    mask |= BDK_CONTROL_MASK;
  if (kbd[VK_MENU] & 0x80)
    mask |= BDK_MOD1_MASK;
  if (kbd[VK_LBUTTON] & 0x80)
    mask |= BDK_BUTTON1_MASK;
  if (kbd[VK_MBUTTON] & 0x80)
    mask |= BDK_BUTTON2_MASK;
  if (kbd[VK_RBUTTON] & 0x80)
    mask |= BDK_BUTTON3_MASK;

  return mask;
}
    
static gboolean
bdk_window_win32_get_pointer (BdkWindow       *window,
			      gint            *x,
			      gint            *y,
			      BdkModifierType *mask)
{
  gboolean return_val;
  POINT point;
  HWND hwnd, hwndc;

  g_return_val_if_fail (window == NULL || BDK_IS_WINDOW (window), FALSE);
  
  return_val = TRUE;

  hwnd = BDK_WINDOW_HWND (window);
  GetCursorPos (&point);
  ScreenToClient (hwnd, &point);

  *x = point.x;
  *y = point.y;

  if (window == _bdk_root)
    {
      *x += _bdk_offset_x;
      *y += _bdk_offset_y;
    }

  hwndc = ChildWindowFromPoint (hwnd, point);
  if (hwndc != NULL && hwndc != hwnd &&
      !bdk_win32_handle_table_lookup ((BdkNativeWindow) hwndc))
    return_val = FALSE; /* Direct child unknown to bdk */

  *mask = get_current_mask ();
  
  return return_val;
}

void
_bdk_windowing_get_pointer (BdkDisplay       *display,
			    BdkScreen       **screen,
			    gint             *x,
			    gint             *y,
			    BdkModifierType  *mask)
{
  POINT point;

  g_return_if_fail (display == _bdk_display);
  
  *screen = _bdk_screen;
  GetCursorPos (&point);
  *x = point.x + _bdk_offset_x;
  *y = point.y + _bdk_offset_y;

  *mask = get_current_mask ();
}

void
bdk_display_warp_pointer (BdkDisplay *display,
			  BdkScreen  *screen,
			  gint        x,
			  gint        y)
{
  g_return_if_fail (display == _bdk_display);
  g_return_if_fail (screen == _bdk_screen);

  SetCursorPos (x - _bdk_offset_x, y - _bdk_offset_y);
}

static void
screen_to_client (HWND hwnd, POINT screen_pt, POINT *client_pt)
{
  *client_pt = screen_pt;
  ScreenToClient (hwnd, client_pt);
}

BdkWindow*
_bdk_windowing_window_at_pointer (BdkDisplay *display,
				  gint       *win_x,
				  gint       *win_y,
				  BdkModifierType *mask,
				  gboolean    get_toplevel)
{
  BdkWindow *window = NULL;
  POINT screen_pt, client_pt;
  HWND hwnd, hwndc;
  RECT rect;

  GetCursorPos (&screen_pt);

  if (get_toplevel)
    {
      /* Only consider visible children of the desktop to avoid the various
       * non-visible windows you often find on a running Windows box. These
       * might overlap our windows and cause our walk to fail. As we assume
       * WindowFromPoint() can find our windows, we follow similar logic
       * here, and ignore invisible and disabled windows.
       */
      hwnd = GetDesktopWindow ();
      do {
        window = bdk_win32_handle_table_lookup ((BdkNativeWindow) hwnd);

        if (window != NULL &&
            BDK_WINDOW_TYPE (window) != BDK_WINDOW_ROOT &&
            BDK_WINDOW_TYPE (window) != BDK_WINDOW_FOREIGN)
          break;

        screen_to_client (hwnd, screen_pt, &client_pt);
        hwndc = ChildWindowFromPointEx (hwnd, client_pt, CWP_SKIPDISABLED  |
                                                         CWP_SKIPINVISIBLE);

	/* Verify that we're really inside the client area of the window */
	if (hwndc != hwnd)
	  {
	    GetClientRect (hwndc, &rect);
	    screen_to_client (hwndc, screen_pt, &client_pt);
	    if (!PtInRect (&rect, client_pt))
	      hwndc = hwnd;
	  }

      } while (hwndc != hwnd && (hwnd = hwndc, 1));

    }
  else
    {
      hwnd = WindowFromPoint (screen_pt);

      /* Verify that we're really inside the client area of the window */
      GetClientRect (hwnd, &rect);
      screen_to_client (hwnd, screen_pt, &client_pt);
      if (!PtInRect (&rect, client_pt))
	hwnd = NULL;

      /* If we didn't hit any window at that point, return the desktop */
      if (hwnd == NULL)
        {
          if (win_x)
            *win_x = screen_pt.x + _bdk_offset_x;
          if (win_y)
            *win_y = screen_pt.y + _bdk_offset_y;
          return _bdk_root;
        }

      window = bdk_win32_handle_table_lookup ((BdkNativeWindow) hwnd);
    }

  if (window && (win_x || win_y))
    {
      if (win_x)
        *win_x = client_pt.x;
      if (win_y)
        *win_y = client_pt.y;
    }

  BDK_NOTE (MISC, g_print ("_bdk_windowing_window_at_pointer: %+d%+d %p%s\n",
			   *win_x, *win_y,
			   hwnd,
			   (window == NULL ? " NULL" : "")));

  return window;
}

static BdkEventMask  
bdk_win32_window_get_events (BdkWindow *window)
{
  BdkWindowImplWin32 *impl;

  if (BDK_WINDOW_DESTROYED (window))
    return 0;

  impl = BDK_WINDOW_IMPL_WIN32 (BDK_WINDOW_OBJECT (window)->impl);

  return impl->native_event_mask;
}

static void          
bdk_win32_window_set_events (BdkWindow   *window,
			     BdkEventMask event_mask)
{
  BdkWindowImplWin32 *impl;

  impl = BDK_WINDOW_IMPL_WIN32 (BDK_WINDOW_OBJECT (window)->impl);

  /* bdk_window_new() always sets the BDK_STRUCTURE_MASK, so better
   * set it here, too. Not that I know or remember why it is
   * necessary, will have to test some day.
   */
  impl->native_event_mask = BDK_STRUCTURE_MASK | event_mask;
}

static void
do_shape_combine_rebunnyion (BdkWindow *window,
			 HRGN	    hrgn,
			 gint       x, gint y)
{
  RECT rect;

  GetClientRect (BDK_WINDOW_HWND (window), &rect);
  _bdk_win32_adjust_client_rect (window, &rect);

  OffsetRgn (hrgn, -rect.left, -rect.top);
  OffsetRgn (hrgn, x, y);

  /* If this is a top-level window, add the title bar to the rebunnyion */
  if (BDK_WINDOW_TYPE (window) == BDK_WINDOW_TOPLEVEL)
    {
      HRGN tmp = CreateRectRgn (0, 0, rect.right - rect.left, -rect.top);
      CombineRgn (hrgn, hrgn, tmp, RGN_OR);
      DeleteObject (tmp);
    }
  
  SetWindowRgn (BDK_WINDOW_HWND (window), hrgn, TRUE);
}

static void
bdk_win32_window_shape_combine_mask (BdkWindow *window,
				     BdkBitmap *mask,
				     gint x, gint y)
{
  BdkWindowObject *private = (BdkWindowObject *)window;

  if (!mask)
    {
      BDK_NOTE (MISC, g_print ("bdk_window_shape_combine_mask: %p: none\n",
			       BDK_WINDOW_HWND (window)));
      SetWindowRgn (BDK_WINDOW_HWND (window), NULL, TRUE);

      private->shaped = FALSE;
    }
  else
    {
      HRGN hrgn;

      BDK_NOTE (MISC, g_print ("bdk_window_shape_combine_mask: %p: %p\n",
			       BDK_WINDOW_HWND (window),
			       BDK_WINDOW_HWND (mask)));

      /* Convert mask bitmap to rebunnyion */
      hrgn = _bdk_win32_bitmap_to_hrgn (mask);

      do_shape_combine_rebunnyion (window, hrgn, x, y);

      private->shaped = TRUE;
    }
}

void
bdk_window_set_override_redirect (BdkWindow *window,
				  gboolean   override_redirect)
{
  BdkWindowObject *private;
  BdkWindowImplWin32 *window_impl;

  g_return_if_fail (BDK_IS_WINDOW (window));

  private = (BdkWindowObject *) window;
  window_impl = BDK_WINDOW_IMPL_WIN32 (private->impl);

  window_impl->override_redirect = !!override_redirect;
}

void
bdk_window_set_accept_focus (BdkWindow *window,
			     gboolean accept_focus)
{
  BdkWindowObject *private;

  g_return_if_fail (BDK_IS_WINDOW (window));

  private = (BdkWindowObject *)window;  
  
  accept_focus = accept_focus != FALSE;

  if (private->accept_focus != accept_focus)
    private->accept_focus = accept_focus;
}

void
bdk_window_set_focus_on_map (BdkWindow *window,
			     gboolean focus_on_map)
{
  BdkWindowObject *private;

  g_return_if_fail (BDK_IS_WINDOW (window));

  private = (BdkWindowObject *)window;  
  
  focus_on_map = focus_on_map != FALSE;

  if (private->focus_on_map != focus_on_map)
    private->focus_on_map = focus_on_map;
}

void          
bdk_window_set_icon_list (BdkWindow *window,
			  GList     *pixbufs)
{
  BdkPixbuf *pixbuf, *big_pixbuf, *small_pixbuf;
  gint big_diff, small_diff;
  gint big_w, big_h, small_w, small_h;
  gint w, h;
  gint dw, dh, diff;
  HICON small_hicon, big_hicon;
  BdkWindowImplWin32 *impl;
  gint i, big_i, small_i;

  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_WINDOW_DESTROYED (window))
    return;

  impl = BDK_WINDOW_IMPL_WIN32 (BDK_WINDOW_OBJECT (window)->impl);

  /* ideal sizes for small and large icons */
  big_w = GetSystemMetrics (SM_CXICON);
  big_h = GetSystemMetrics (SM_CYICON);
  small_w = GetSystemMetrics (SM_CXSMICON);
  small_h = GetSystemMetrics (SM_CYSMICON);

  /* find closest sized icons in the list */
  big_pixbuf = NULL;
  small_pixbuf = NULL;
  big_diff = 0;
  small_diff = 0;
  i = 0;
  while (pixbufs)
    {
      pixbuf = (BdkPixbuf*) pixbufs->data;
      w = bdk_pixbuf_get_width (pixbuf);
      h = bdk_pixbuf_get_height (pixbuf);

      dw = ABS (w - big_w);
      dh = ABS (h - big_h);
      diff = dw*dw + dh*dh;
      if (big_pixbuf == NULL || diff < big_diff)
	{
	  big_pixbuf = pixbuf;
	  big_diff = diff;
	  big_i = i;
	}

      dw = ABS (w - small_w);
      dh = ABS (h - small_h);
      diff = dw*dw + dh*dh;
      if (small_pixbuf == NULL || diff < small_diff)
	{
	  small_pixbuf = pixbuf;
	  small_diff = diff;
	  small_i = i;
	}

      pixbufs = g_list_next (pixbufs);
      i++;
    }

  /* Create the icons */
  big_hicon = _bdk_win32_pixbuf_to_hicon (big_pixbuf);
  small_hicon = _bdk_win32_pixbuf_to_hicon (small_pixbuf);

  /* Set the icons */
  SendMessageW (BDK_WINDOW_HWND (window), WM_SETICON, ICON_BIG,
		(LPARAM)big_hicon);
  SendMessageW (BDK_WINDOW_HWND (window), WM_SETICON, ICON_SMALL,
		(LPARAM)small_hicon);

  /* Store the icons, destroying any previous icons */
  if (impl->hicon_big)
    GDI_CALL (DestroyIcon, (impl->hicon_big));
  impl->hicon_big = big_hicon;
  if (impl->hicon_small)
    GDI_CALL (DestroyIcon, (impl->hicon_small));
  impl->hicon_small = small_hicon;
}

void          
bdk_window_set_icon (BdkWindow *window, 
		     BdkWindow *icon_window,
		     BdkPixmap *pixmap,
		     BdkBitmap *mask)
{
  g_return_if_fail (BDK_IS_WINDOW (window));

  /* do nothing, use bdk_window_set_icon_list instead */
}

void
bdk_window_set_icon_name (BdkWindow   *window, 
			  const gchar *name)
{
  /* In case I manage to confuse this again (or somebody else does):
   * Please note that "icon name" here really *does* mean the name or
   * title of an window minimized as an icon on the desktop, or in the
   * taskbar. It has nothing to do with the freedesktop.org icon
   * naming stuff.
   */

  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_WINDOW_DESTROYED (window))
    return;
  
#if 0
  /* This is not the correct thing to do. We should keep both the
   * "normal" window title, and the icon name. When the window is
   * minimized, call SetWindowText() with the icon name, and when the
   * window is restored, with the normal window title. Also, the name
   * is in UTF-8, so we should do the normal conversion to either wide
   * chars or system codepage, and use either the W or A version of
   * SetWindowText(), depending on Windows version.
   */
  API_CALL (SetWindowText, (BDK_WINDOW_HWND (window), name));
#endif
}

BdkWindow *
bdk_window_get_group (BdkWindow *window)
{
  g_return_val_if_fail (BDK_IS_WINDOW (window), NULL);
  g_return_val_if_fail (BDK_WINDOW_TYPE (window) != BDK_WINDOW_CHILD, NULL);

  if (BDK_WINDOW_DESTROYED (window))
    return NULL;
  
  g_warning ("bdk_window_get_group not yet implemented");

  return NULL;
}

void          
bdk_window_set_group (BdkWindow *window, 
		      BdkWindow *leader)
{
  g_return_if_fail (BDK_IS_WINDOW (window));
  g_return_if_fail (BDK_WINDOW_TYPE (window) != BDK_WINDOW_CHILD);
  g_return_if_fail (leader == NULL || BDK_IS_WINDOW (leader));

  if (BDK_WINDOW_DESTROYED (window) || BDK_WINDOW_DESTROYED (leader))
    return;
  
  g_warning ("bdk_window_set_group not implemented");
}

static void
update_single_bit (LONG    *style,
                   gboolean all,
		   int      bdk_bit,
		   int      style_bit)
{
  /* all controls the interpretation of bdk_bit -- if all is TRUE,
   * bdk_bit indicates whether style_bit is off; if all is FALSE, bdk
   * bit indicate whether style_bit is on
   */
  if ((!all && bdk_bit) || (all && !bdk_bit))
    *style |= style_bit;
  else
    *style &= ~style_bit;
}

static void
update_style_bits (BdkWindow *window)
{
  BdkWindowObject *private = (BdkWindowObject *)window;
  BdkWindowImplWin32 *impl = (BdkWindowImplWin32 *)private->impl;
  BdkWMDecoration decorations;
  LONG old_style, new_style, old_exstyle, new_exstyle;
  gboolean all;
  RECT rect, before, after;

  if (private->state & BDK_WINDOW_STATE_FULLSCREEN)
    return;

  old_style = GetWindowLong (BDK_WINDOW_HWND (window), GWL_STYLE);
  old_exstyle = GetWindowLong (BDK_WINDOW_HWND (window), GWL_EXSTYLE);

  GetClientRect (BDK_WINDOW_HWND (window), &before);
  after = before;
  AdjustWindowRectEx (&before, old_style, FALSE, old_exstyle);

  new_style = old_style;
  new_exstyle = old_exstyle;

  if (private->window_type == BDK_WINDOW_TEMP)
    new_exstyle |= WS_EX_TOOLWINDOW | WS_EX_TOPMOST;
  else if (impl->type_hint == BDK_WINDOW_TYPE_HINT_UTILITY)
    new_exstyle |= WS_EX_TOOLWINDOW ;
  else
    new_exstyle &= ~WS_EX_TOOLWINDOW;

  if (get_effective_window_decorations (window, &decorations))
    {
      all = (decorations & BDK_DECOR_ALL);
      update_single_bit (&new_style, all, decorations & BDK_DECOR_BORDER, WS_BORDER);
      update_single_bit (&new_style, all, decorations & BDK_DECOR_RESIZEH, WS_THICKFRAME);
      update_single_bit (&new_style, all, decorations & BDK_DECOR_TITLE, WS_CAPTION);
      update_single_bit (&new_style, all, decorations & BDK_DECOR_MENU, WS_SYSMENU);
      update_single_bit (&new_style, all, decorations & BDK_DECOR_MINIMIZE, WS_MINIMIZEBOX);
      update_single_bit (&new_style, all, decorations & BDK_DECOR_MAXIMIZE, WS_MAXIMIZEBOX);
    }

  if (old_style == new_style && old_exstyle == new_exstyle )
    {
      BDK_NOTE (MISC, g_print ("update_style_bits: %p: no change\n",
			       BDK_WINDOW_HWND (window)));
      return;
    }

  if (old_style != new_style)
    {
      BDK_NOTE (MISC, g_print ("update_style_bits: %p: STYLE: %s => %s\n",
			       BDK_WINDOW_HWND (window),
			       _bdk_win32_window_style_to_string (old_style),
			       _bdk_win32_window_style_to_string (new_style)));
      
      SetWindowLong (BDK_WINDOW_HWND (window), GWL_STYLE, new_style);
    }

  if (old_exstyle != new_exstyle)
    {
      BDK_NOTE (MISC, g_print ("update_style_bits: %p: EXSTYLE: %s => %s\n",
			       BDK_WINDOW_HWND (window),
			       _bdk_win32_window_exstyle_to_string (old_exstyle),
			       _bdk_win32_window_exstyle_to_string (new_exstyle)));
      
      SetWindowLong (BDK_WINDOW_HWND (window), GWL_EXSTYLE, new_exstyle);
    }

  AdjustWindowRectEx (&after, new_style, FALSE, new_exstyle);

  GetWindowRect (BDK_WINDOW_HWND (window), &rect);
  rect.left += after.left - before.left;
  rect.top += after.top - before.top;
  rect.right += after.right - before.right;
  rect.bottom += after.bottom - before.bottom;

  SetWindowPos (BDK_WINDOW_HWND (window), NULL,
		rect.left, rect.top,
		rect.right - rect.left, rect.bottom - rect.top,
		SWP_FRAMECHANGED | SWP_NOACTIVATE | 
		SWP_NOREPOSITION | SWP_NOZORDER);

}

static void
update_single_system_menu_entry (HMENU    hmenu,
				 gboolean all,
				 int      bdk_bit,
				 int      menu_entry)
{
  /* all controls the interpretation of bdk_bit -- if all is TRUE,
   * bdk_bit indicates whether menu entry is disabled; if all is
   * FALSE, bdk bit indicate whether menu entry is enabled
   */
  if ((!all && bdk_bit) || (all && !bdk_bit))
    EnableMenuItem (hmenu, menu_entry, MF_BYCOMMAND | MF_ENABLED);
  else
    EnableMenuItem (hmenu, menu_entry, MF_BYCOMMAND | MF_GRAYED);
}

static void
update_system_menu (BdkWindow *window)
{
  BdkWMFunction functions;
  BOOL all;

  if (_bdk_window_get_functions (window, &functions))
    {
      HMENU hmenu = GetSystemMenu (BDK_WINDOW_HWND (window), FALSE);

      all = (functions & BDK_FUNC_ALL);
      update_single_system_menu_entry (hmenu, all, functions & BDK_FUNC_RESIZE, SC_SIZE);
      update_single_system_menu_entry (hmenu, all, functions & BDK_FUNC_MOVE, SC_MOVE);
      update_single_system_menu_entry (hmenu, all, functions & BDK_FUNC_MINIMIZE, SC_MINIMIZE);
      update_single_system_menu_entry (hmenu, all, functions & BDK_FUNC_MAXIMIZE, SC_MAXIMIZE);
      update_single_system_menu_entry (hmenu, all, functions & BDK_FUNC_CLOSE, SC_CLOSE);
    }
}

static GQuark
get_decorations_quark ()
{
  static GQuark quark = 0;
  
  if (!quark)
    quark = g_quark_from_static_string ("bdk-window-decorations");
  
  return quark;
}

void
bdk_window_set_decorations (BdkWindow      *window,
			    BdkWMDecoration decorations)
{
  BdkWMDecoration* decorations_copy;
  
  g_return_if_fail (BDK_IS_WINDOW (window));
  
  BDK_NOTE (MISC, g_print ("bdk_window_set_decorations: %p: %s %s%s%s%s%s%s\n",
			   BDK_WINDOW_HWND (window),
			   (decorations & BDK_DECOR_ALL ? "clearing" : "setting"),
			   (decorations & BDK_DECOR_BORDER ? "BORDER " : ""),
			   (decorations & BDK_DECOR_RESIZEH ? "RESIZEH " : ""),
			   (decorations & BDK_DECOR_TITLE ? "TITLE " : ""),
			   (decorations & BDK_DECOR_MENU ? "MENU " : ""),
			   (decorations & BDK_DECOR_MINIMIZE ? "MINIMIZE " : ""),
			   (decorations & BDK_DECOR_MAXIMIZE ? "MAXIMIZE " : "")));

  decorations_copy = g_malloc (sizeof (BdkWMDecoration));
  *decorations_copy = decorations;
  g_object_set_qdata_full (B_OBJECT (window), get_decorations_quark (), decorations_copy, g_free);

  update_style_bits (window);
}

gboolean
bdk_window_get_decorations (BdkWindow       *window,
			    BdkWMDecoration *decorations)
{
  BdkWMDecoration* decorations_set;
  
  g_return_val_if_fail (BDK_IS_WINDOW (window), FALSE);

  decorations_set = g_object_get_qdata (B_OBJECT (window), get_decorations_quark ());
  if (decorations_set)
    *decorations = *decorations_set;

  return (decorations_set != NULL);
}

static GQuark
get_functions_quark ()
{
  static GQuark quark = 0;
  
  if (!quark)
    quark = g_quark_from_static_string ("bdk-window-functions");
  
  return quark;
}

void
bdk_window_set_functions (BdkWindow    *window,
			  BdkWMFunction functions)
{
  BdkWMFunction* functions_copy;

  g_return_if_fail (BDK_IS_WINDOW (window));
  
  BDK_NOTE (MISC, g_print ("bdk_window_set_functions: %p: %s %s%s%s%s%s\n",
			   BDK_WINDOW_HWND (window),
			   (functions & BDK_FUNC_ALL ? "clearing" : "setting"),
			   (functions & BDK_FUNC_RESIZE ? "RESIZE " : ""),
			   (functions & BDK_FUNC_MOVE ? "MOVE " : ""),
			   (functions & BDK_FUNC_MINIMIZE ? "MINIMIZE " : ""),
			   (functions & BDK_FUNC_MAXIMIZE ? "MAXIMIZE " : ""),
			   (functions & BDK_FUNC_CLOSE ? "CLOSE " : "")));

  functions_copy = g_malloc (sizeof (BdkWMFunction));
  *functions_copy = functions;
  g_object_set_qdata_full (B_OBJECT (window), get_functions_quark (), functions_copy, g_free);

  update_system_menu (window);
}

gboolean
_bdk_window_get_functions (BdkWindow     *window,
		           BdkWMFunction *functions)
{
  BdkWMFunction* functions_set;
  
  functions_set = g_object_get_qdata (B_OBJECT (window), get_functions_quark ());
  if (functions_set)
    *functions = *functions_set;

  return (functions_set != NULL);
}

static gboolean 
bdk_win32_window_set_static_gravities (BdkWindow *window,
				 gboolean   use_static)
{
  g_return_val_if_fail (BDK_IS_WINDOW (window), FALSE);

  return !use_static;
}

void
bdk_window_begin_resize_drag (BdkWindow     *window,
                              BdkWindowEdge  edge,
                              gint           button,
                              gint           root_x,
                              gint           root_y,
                              guint32        timestamp)
{
  WPARAM winedge;
  
  g_return_if_fail (BDK_IS_WINDOW (window));
  
  if (BDK_WINDOW_DESTROYED (window))
    return;

  /* Tell Windows to start interactively resizing the window by pretending that
   * the left pointer button was clicked in the suitable edge or corner. This
   * will only work if the button is down when this function is called, and
   * will only work with button 1 (left), since Windows only allows window
   * dragging using the left mouse button.
   */
  if (button != 1)
    return;
  
  /* Must break the automatic grab that occured when the button was
   * pressed, otherwise it won't work.
   */
  bdk_display_pointer_ungrab (_bdk_display, 0);

  switch (edge)
    {
    case BDK_WINDOW_EDGE_NORTH_WEST:
      winedge = HTTOPLEFT;
      break;

    case BDK_WINDOW_EDGE_NORTH:
      winedge = HTTOP;
      break;

    case BDK_WINDOW_EDGE_NORTH_EAST:
      winedge = HTTOPRIGHT;
      break;

    case BDK_WINDOW_EDGE_WEST:
      winedge = HTLEFT;
      break;

    case BDK_WINDOW_EDGE_EAST:
      winedge = HTRIGHT;
      break;

    case BDK_WINDOW_EDGE_SOUTH_WEST:
      winedge = HTBOTTOMLEFT;
      break;

    case BDK_WINDOW_EDGE_SOUTH:
      winedge = HTBOTTOM;
      break;

    case BDK_WINDOW_EDGE_SOUTH_EAST:
    default:
      winedge = HTBOTTOMRIGHT;
      break;
    }

  DefWindowProcW (BDK_WINDOW_HWND (window), WM_NCLBUTTONDOWN, winedge,
		  MAKELPARAM (root_x - _bdk_offset_x, root_y - _bdk_offset_y));
}

void
bdk_window_begin_move_drag (BdkWindow *window,
                            gint       button,
                            gint       root_x,
                            gint       root_y,
                            guint32    timestamp)
{
  g_return_if_fail (BDK_IS_WINDOW (window));
  
  if (BDK_WINDOW_DESTROYED (window))
    return;

  /* Tell Windows to start interactively moving the window by pretending that
   * the left pointer button was clicked in the titlebar. This will only work
   * if the button is down when this function is called, and will only work
   * with button 1 (left), since Windows only allows window dragging using the
   * left mouse button.
   */
  if (button != 1)
    return;
  
  /* Must break the automatic grab that occured when the button was pressed,
   * otherwise it won't work.
   */
  bdk_display_pointer_ungrab (_bdk_display, 0);

  DefWindowProcW (BDK_WINDOW_HWND (window), WM_NCLBUTTONDOWN, HTCAPTION,
		  MAKELPARAM (root_x - _bdk_offset_x, root_y - _bdk_offset_y));
}


/*
 * Setting window states
 */
void
bdk_window_iconify (BdkWindow *window)
{
  HWND old_active_window;

  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_WINDOW_DESTROYED (window))
    return;

  BDK_NOTE (MISC, g_print ("bdk_window_iconify: %p: %s\n",
			   BDK_WINDOW_HWND (window),
			   _bdk_win32_window_state_to_string (((BdkWindowObject *) window)->state)));

  if (BDK_WINDOW_IS_MAPPED (window))
    {
      old_active_window = GetActiveWindow ();
      ShowWindow (BDK_WINDOW_HWND (window), SW_MINIMIZE);
      if (old_active_window != BDK_WINDOW_HWND (window))
	SetActiveWindow (old_active_window);
    }
  else
    {
      bdk_synthesize_window_state (window,
                                   0,
                                   BDK_WINDOW_STATE_ICONIFIED);
    }
}

void
bdk_window_deiconify (BdkWindow *window)
{
  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_WINDOW_DESTROYED (window))
    return;

  BDK_NOTE (MISC, g_print ("bdk_window_deiconify: %p: %s\n",
			   BDK_WINDOW_HWND (window),
			   _bdk_win32_window_state_to_string (((BdkWindowObject *) window)->state)));

  if (BDK_WINDOW_IS_MAPPED (window))
    {  
      show_window_internal (window, BDK_WINDOW_IS_MAPPED (window), TRUE);
    }
  else
    {
      bdk_synthesize_window_state (window,
                                   BDK_WINDOW_STATE_ICONIFIED,
                                   0);
    }
}

void
bdk_window_stick (BdkWindow *window)
{
  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_WINDOW_DESTROYED (window))
    return;

  /* FIXME: Do something? */
}

void
bdk_window_unstick (BdkWindow *window)
{
  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_WINDOW_DESTROYED (window))
    return;

  /* FIXME: Do something? */
}

void
bdk_window_maximize (BdkWindow *window)
{
  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_WINDOW_DESTROYED (window))
    return;

  BDK_NOTE (MISC, g_print ("bdk_window_maximize: %p: %s\n",
			   BDK_WINDOW_HWND (window),
			   _bdk_win32_window_state_to_string (((BdkWindowObject *) window)->state)));

  if (BDK_WINDOW_IS_MAPPED (window))
    ShowWindow (BDK_WINDOW_HWND (window), SW_MAXIMIZE);
  else
    bdk_synthesize_window_state (window,
				 0,
				 BDK_WINDOW_STATE_MAXIMIZED);
}

void
bdk_window_unmaximize (BdkWindow *window)
{
  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_WINDOW_DESTROYED (window))
    return;

  BDK_NOTE (MISC, g_print ("bdk_window_unmaximize: %p: %s\n",
			   BDK_WINDOW_HWND (window),
			   _bdk_win32_window_state_to_string (((BdkWindowObject *) window)->state)));

  if (BDK_WINDOW_IS_MAPPED (window))
    ShowWindow (BDK_WINDOW_HWND (window), SW_RESTORE);
  else
    bdk_synthesize_window_state (window,
				 BDK_WINDOW_STATE_MAXIMIZED,
				 0);
}

void
bdk_window_fullscreen (BdkWindow *window)
{
  gint x, y, width, height;
  FullscreenInfo *fi;
  BdkWindowObject *private = (BdkWindowObject *) window;
  HMONITOR monitor;
  MONITORINFO mi;

  g_return_if_fail (BDK_IS_WINDOW (window));

  fi = g_new (FullscreenInfo, 1);

  if (!GetWindowRect (BDK_WINDOW_HWND (window), &(fi->r)))
    g_free (fi);
  else
    {
      BdkWindowImplWin32 *impl = BDK_WINDOW_IMPL_WIN32 (private->impl);

      monitor = MonitorFromWindow (BDK_WINDOW_HWND (window), MONITOR_DEFAULTTONEAREST);
      mi.cbSize = sizeof (mi);
      if (monitor && GetMonitorInfo (monitor, &mi))
	{
	  x = mi.rcMonitor.left;
	  y = mi.rcMonitor.top;
	  width = mi.rcMonitor.right - x;
	  height = mi.rcMonitor.bottom - y;
	}
      else
	{
	  x = y = 0;
	  width = GetSystemMetrics (SM_CXSCREEN);
	  height = GetSystemMetrics (SM_CYSCREEN);
	}

      /* remember for restoring */
      fi->hint_flags = impl->hint_flags;
      impl->hint_flags &= ~BDK_HINT_MAX_SIZE;
      g_object_set_data (B_OBJECT (window), "fullscreen-info", fi);
      fi->style = GetWindowLong (BDK_WINDOW_HWND (window), GWL_STYLE);

      /* Send state change before configure event */
      bdk_synthesize_window_state (window, 0, BDK_WINDOW_STATE_FULLSCREEN);

      SetWindowLong (BDK_WINDOW_HWND (window), GWL_STYLE, 
                     (fi->style & ~WS_OVERLAPPEDWINDOW) | WS_POPUP);

      API_CALL (SetWindowPos, (BDK_WINDOW_HWND (window), HWND_TOP,
			       x, y, width, height,
			       SWP_NOCOPYBITS | SWP_SHOWWINDOW));
    }
}

void
bdk_window_unfullscreen (BdkWindow *window)
{
  FullscreenInfo *fi;
  BdkWindowObject *private = (BdkWindowObject *) window;

  g_return_if_fail (BDK_IS_WINDOW (window));

  fi = g_object_get_data (B_OBJECT (window), "fullscreen-info");
  if (fi)
    {
      BdkWindowImplWin32 *impl = BDK_WINDOW_IMPL_WIN32 (private->impl);

      bdk_synthesize_window_state (window, BDK_WINDOW_STATE_FULLSCREEN, 0);

      impl->hint_flags = fi->hint_flags;
      SetWindowLong (BDK_WINDOW_HWND (window), GWL_STYLE, fi->style);
      API_CALL (SetWindowPos, (BDK_WINDOW_HWND (window), HWND_NOTOPMOST,
			       fi->r.left, fi->r.top,
			       fi->r.right - fi->r.left, fi->r.bottom - fi->r.top,
			       SWP_NOCOPYBITS | SWP_SHOWWINDOW));
      
      g_object_set_data (B_OBJECT (window), "fullscreen-info", NULL);
      g_free (fi);
      update_style_bits (window);
    }
}

void
bdk_window_set_keep_above (BdkWindow *window,
			   gboolean   setting)
{
  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_WINDOW_DESTROYED (window))
    return;

  BDK_NOTE (MISC, g_print ("bdk_window_set_keep_above: %p: %s\n",
			   BDK_WINDOW_HWND (window),
			   setting ? "YES" : "NO"));

  if (BDK_WINDOW_IS_MAPPED (window))
    {
      API_CALL (SetWindowPos, (BDK_WINDOW_HWND (window),
			       setting ? HWND_TOPMOST : HWND_NOTOPMOST,
			       0, 0, 0, 0,
			       SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE));
    }

  bdk_synthesize_window_state (window,
			       setting ? BDK_WINDOW_STATE_BELOW : BDK_WINDOW_STATE_ABOVE,
			       setting ? BDK_WINDOW_STATE_ABOVE : 0);
}

void
bdk_window_set_keep_below (BdkWindow *window,
			   gboolean   setting)
{
  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_WINDOW_DESTROYED (window))
    return;

  BDK_NOTE (MISC, g_print ("bdk_window_set_keep_below: %p: %s\n",
			   BDK_WINDOW_HWND (window),
			   setting ? "YES" : "NO"));

  if (BDK_WINDOW_IS_MAPPED (window))
    {
      API_CALL (SetWindowPos, (BDK_WINDOW_HWND (window),
			       setting ? HWND_BOTTOM : HWND_NOTOPMOST,
			       0, 0, 0, 0,
			       SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE));
    }

  bdk_synthesize_window_state (window,
			       setting ? BDK_WINDOW_STATE_ABOVE : BDK_WINDOW_STATE_BELOW,
			       setting ? BDK_WINDOW_STATE_BELOW : 0);
}

void
bdk_window_focus (BdkWindow *window,
                  guint32    timestamp)
{
  BdkWindowObject *private;

  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_WINDOW_DESTROYED (window))
    return;

  private = (BdkWindowObject *)window;

  BDK_NOTE (MISC, g_print ("bdk_window_focus: %p: %s\n",
			   BDK_WINDOW_HWND (window),
			   _bdk_win32_window_state_to_string (((BdkWindowObject *) window)->state)));

  if (private->state & BDK_WINDOW_STATE_MAXIMIZED)
    ShowWindow (BDK_WINDOW_HWND (window), SW_SHOWMAXIMIZED);
  else if (private->state & BDK_WINDOW_STATE_ICONIFIED)
    ShowWindow (BDK_WINDOW_HWND (window), SW_RESTORE);
  else if (!IsWindowVisible (BDK_WINDOW_HWND (window)))
    ShowWindow (BDK_WINDOW_HWND (window), SW_SHOWNORMAL);
  else
    ShowWindow (BDK_WINDOW_HWND (window), SW_SHOW);

  SetFocus (BDK_WINDOW_HWND (window));
}

void
bdk_window_set_modal_hint (BdkWindow *window,
			   gboolean   modal)
{
  BdkWindowObject *private;

  g_return_if_fail (BDK_IS_WINDOW (window));
  
  if (BDK_WINDOW_DESTROYED (window))
    return;

  BDK_NOTE (MISC, g_print ("bdk_window_set_modal_hint: %p: %s\n",
			   BDK_WINDOW_HWND (window),
			   modal ? "YES" : "NO"));

  private = (BdkWindowObject*) window;

  if (modal == private->modal_hint)
    return;

  private->modal_hint = modal;

#if 0
  /* Not sure about this one.. -- Cody */
  if (BDK_WINDOW_IS_MAPPED (window))
    API_CALL (SetWindowPos, (BDK_WINDOW_HWND (window), 
			     modal ? HWND_TOPMOST : HWND_NOTOPMOST,
			     0, 0, 0, 0,
			     SWP_NOMOVE | SWP_NOSIZE));
#else

  if (modal)
    {
      _bdk_push_modal_window (window);
      bdk_window_raise (window);
    }
  else
    {
      _bdk_remove_modal_window (window);
    }

#endif
}

void
bdk_window_set_skip_taskbar_hint (BdkWindow *window,
				  gboolean   skips_taskbar)
{
  static BdkWindow *owner = NULL;
  //BdkWindowAttr wa;

  g_return_if_fail (BDK_IS_WINDOW (window));

  BDK_NOTE (MISC, g_print ("bdk_window_set_skip_taskbar_hint: %p: %s, doing nothing\n",
			   BDK_WINDOW_HWND (window),
			   skips_taskbar ? "YES" : "NO"));

  // ### TODO: Need to figure out what to do here.
  return;

  if (skips_taskbar)
    {
#if 0
      if (owner == NULL)
		{
		  wa.window_type = BDK_WINDOW_TEMP;
		  wa.wclass = BDK_INPUT_OUTPUT;
		  wa.width = wa.height = 1;
		  wa.event_mask = 0;
		  owner = bdk_window_new_internal (NULL, &wa, 0, TRUE);
		}
#endif

      SetWindowLongPtr (BDK_WINDOW_HWND (window), GWLP_HWNDPARENT, (LONG_PTR) BDK_WINDOW_HWND (owner));

#if 0 /* Should we also turn off the minimize and maximize buttons? */
      SetWindowLong (BDK_WINDOW_HWND (window), GWL_STYLE,
		     GetWindowLong (BDK_WINDOW_HWND (window), GWL_STYLE) & ~(WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_SYSMENU));
     
      SetWindowPos (BDK_WINDOW_HWND (window), NULL,
		    0, 0, 0, 0,
		    SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOMOVE |
		    SWP_NOREPOSITION | SWP_NOSIZE | SWP_NOZORDER);
#endif
    }
  else
    {
      SetWindowLongPtr (BDK_WINDOW_HWND (window), GWLP_HWNDPARENT, 0);
    }
}

void
bdk_window_set_skip_pager_hint (BdkWindow *window,
				gboolean   skips_pager)
{
  g_return_if_fail (BDK_IS_WINDOW (window));

  BDK_NOTE (MISC, g_print ("bdk_window_set_skip_pager_hint: %p: %s, doing nothing\n",
			   BDK_WINDOW_HWND (window),
			   skips_pager ? "YES" : "NO"));
}

void
bdk_window_set_type_hint (BdkWindow        *window,
			  BdkWindowTypeHint hint)
{
  g_return_if_fail (BDK_IS_WINDOW (window));
  
  if (BDK_WINDOW_DESTROYED (window))
    return;

  BDK_NOTE (MISC,
	    B_STMT_START{
	      static GEnumClass *class = NULL;
	      if (!class)
		class = g_type_class_ref (BDK_TYPE_WINDOW_TYPE_HINT);
	      g_print ("bdk_window_set_type_hint: %p: %s\n",
		       BDK_WINDOW_HWND (window),
		       g_enum_get_value (class, hint)->value_name);
	    }B_STMT_END);

  ((BdkWindowImplWin32 *)((BdkWindowObject *)window)->impl)->type_hint = hint;

  update_style_bits (window);
}

BdkWindowTypeHint
bdk_window_get_type_hint (BdkWindow *window)
{
  g_return_val_if_fail (BDK_IS_WINDOW (window), BDK_WINDOW_TYPE_HINT_NORMAL);
  
  if (BDK_WINDOW_DESTROYED (window))
    return BDK_WINDOW_TYPE_HINT_NORMAL;

  return BDK_WINDOW_IMPL_WIN32 (((BdkWindowObject *) window)->impl)->type_hint;
}

static void
bdk_win32_window_shape_combine_rebunnyion (BdkWindow       *window,
				       const BdkRebunnyion *shape_rebunnyion,
				       gint             offset_x,
				       gint             offset_y)
{
  if (BDK_WINDOW_DESTROYED (window))
    return;

  if (!shape_rebunnyion)
    {
      BDK_NOTE (MISC, g_print ("bdk_win32_window_shape_combine_rebunnyion: %p: none\n",
			       BDK_WINDOW_HWND (window)));
      SetWindowRgn (BDK_WINDOW_HWND (window), NULL, TRUE);
    }
  else
    {
      HRGN hrgn;

      hrgn = _bdk_win32_bdkrebunnyion_to_hrgn (shape_rebunnyion, 0, 0);
      
      BDK_NOTE (MISC, g_print ("bdk_win32_window_shape_combine_rebunnyion: %p: %p\n",
			       BDK_WINDOW_HWND (window),
			       hrgn));

      do_shape_combine_rebunnyion (window, hrgn, offset_x, offset_y);
    }
}

BdkWindow *
bdk_window_lookup_for_display (BdkDisplay      *display,
                               BdkNativeWindow  anid)
{
  return bdk_win32_window_lookup_for_display (display, anid);
}

BdkWindow *
bdk_win32_window_lookup_for_display (BdkDisplay      *display,
                                     BdkNativeWindow  anid)
{
  g_return_val_if_fail (display == _bdk_display, NULL);

  return bdk_window_lookup (anid);
}

void
bdk_window_enable_synchronized_configure (BdkWindow *window)
{
  g_return_if_fail (BDK_IS_WINDOW (window));
}

void
bdk_window_configure_finished (BdkWindow *window)
{
  g_return_if_fail (BDK_IS_WINDOW (window));
}

void
_bdk_windowing_window_beep (BdkWindow *window)
{
  bdk_display_beep (_bdk_display);
}

void
bdk_window_set_opacity (BdkWindow *window,
			gdouble    opacity)
{
  LONG exstyle;
  typedef BOOL (WINAPI *PFN_SetLayeredWindowAttributes) (HWND, COLORREF, BYTE, DWORD);
  PFN_SetLayeredWindowAttributes setLayeredWindowAttributes = NULL;

  g_return_if_fail (BDK_IS_WINDOW (window));
  g_return_if_fail (WINDOW_IS_TOPLEVEL (window));

  if (BDK_WINDOW_DESTROYED (window))
    return;

  if (opacity < 0)
    opacity = 0;
  else if (opacity > 1)
    opacity = 1;

  exstyle = GetWindowLong (BDK_WINDOW_HWND (window), GWL_EXSTYLE);

  if (!(exstyle & WS_EX_LAYERED))
    SetWindowLong (BDK_WINDOW_HWND (window),
		    GWL_EXSTYLE,
		    exstyle | WS_EX_LAYERED);

  setLayeredWindowAttributes = 
    (PFN_SetLayeredWindowAttributes)GetProcAddress (GetModuleHandle ("user32.dll"), "SetLayeredWindowAttributes");

  if (setLayeredWindowAttributes)
    {
      API_CALL (setLayeredWindowAttributes, (BDK_WINDOW_HWND (window),
					     0,
					     opacity * 0xff,
					     LWA_ALPHA));
    }
}

BdkRebunnyion *
_bdk_windowing_get_shape_for_mask (BdkBitmap *mask)
{
  BdkRebunnyion *rebunnyion;
  HRGN hrgn = _bdk_win32_bitmap_to_hrgn (mask);

  rebunnyion = _bdk_win32_hrgn_to_rebunnyion (hrgn);
  DeleteObject (hrgn);

  return rebunnyion;
}

void
_bdk_windowing_window_set_composited (BdkWindow *window, gboolean composited)
{
}

BdkRebunnyion *
_bdk_windowing_window_get_shape (BdkWindow *window)
{
  HRGN hrgn = CreateRectRgn (0, 0, 0, 0);
  int  type = GetWindowRgn (BDK_WINDOW_HWND (window), hrgn);

  if (type == SIMPLEREBUNNYION || type == COMPLEXREBUNNYION)
    {
      BdkRebunnyion *rebunnyion = _bdk_win32_hrgn_to_rebunnyion (hrgn);

      DeleteObject (hrgn);
      return rebunnyion;
    }

  return NULL;
}

BdkRebunnyion *
_bdk_windowing_window_get_input_shape (BdkWindow *window)
{
  /* CHECK: are these really supposed to be the same? */
  return _bdk_windowing_window_get_shape (window);
}

static gboolean
_bdk_win32_window_queue_antiexpose (BdkWindow *window,
				    BdkRebunnyion *area)
{
  HRGN hrgn = _bdk_win32_bdkrebunnyion_to_hrgn (area, 0, 0);

  BDK_NOTE (EVENTS, g_print ("_bdk_windowing_window_queue_antiexpose: ValidateRgn %p %s\n",
			     BDK_WINDOW_HWND (window),
			     _bdk_win32_bdkrebunnyion_to_string (area)));

  ValidateRgn (BDK_WINDOW_HWND (window), hrgn);

  DeleteObject (hrgn);

  return FALSE;
}

/*
 * queue_translation is meant to only move any outstanding invalid area
 * in the given area by dx,dy. A typical example of when its needed is an
 * app with two toplevels where one (A) overlaps the other (B). If the
 * app first moves A so that B is invalidated and then scrolls B before
 * handling the expose. The scroll operation will copy the invalid area
 * to a new position, but when the invalid area is then exposed it only
 * redraws the old areas not the place where the invalid data was copied
 * by the scroll.
 */
static void
_bdk_win32_window_queue_translation (BdkWindow *window,
				     BdkGC     *gc,
				     BdkRebunnyion *area,
				     gint       dx,
				     gint       dy)
{
  HRGN hrgn = CreateRectRgn (0, 0, 0, 0);
  int ret = GetUpdateRgn (BDK_WINDOW_HWND (window), hrgn, FALSE);
  if (ret == ERROR)
    WIN32_API_FAILED ("GetUpdateRgn");
  else if (ret != NULLREBUNNYION)
    {
      /* Get current updaterebunnyion, move any part of it that intersects area by dx,dy */
      HRGN update = _bdk_win32_bdkrebunnyion_to_hrgn (area, 0, 0);
      ret = CombineRgn (update, hrgn, update, RGN_AND);
      if (ret == ERROR)
        WIN32_API_FAILED ("CombineRgn");
      else if (ret != NULLREBUNNYION)
	{
	  OffsetRgn (update, dx, dy);
          API_CALL (InvalidateRgn, (BDK_WINDOW_HWND (window), update, TRUE));
	}
      DeleteObject (update);
    }
  DeleteObject (hrgn);
}

static void
bdk_win32_input_shape_combine_rebunnyion (BdkWindow *window,
				      const BdkRebunnyion *shape_rebunnyion,
				      gint offset_x,
				      gint offset_y)
{
  if (BDK_WINDOW_DESTROYED (window))
    return;
  /* CHECK: are these really supposed to be the same? */
  bdk_win32_window_shape_combine_rebunnyion (window, shape_rebunnyion, offset_x, offset_y);
}

void
_bdk_windowing_window_process_updates_recurse (BdkWindow *window,
					       BdkRebunnyion *rebunnyion)
{
  _bdk_window_process_updates_recurse (window, rebunnyion);
}

void
_bdk_windowing_before_process_all_updates (void)
{
}

void
_bdk_windowing_after_process_all_updates (void)
{
}

static void
bdk_window_impl_iface_init (BdkWindowImplIface *iface)
{
  iface->show = bdk_win32_window_show;
  iface->hide = bdk_win32_window_hide;
  iface->withdraw = bdk_win32_window_withdraw;
  iface->set_events = bdk_win32_window_set_events;
  iface->get_events = bdk_win32_window_get_events;
  iface->raise = bdk_win32_window_raise;
  iface->lower = bdk_win32_window_lower;
  iface->restack_under = bdk_win32_window_restack_under;
  iface->restack_toplevel = bdk_win32_window_restack_toplevel;
  iface->move_resize = bdk_win32_window_move_resize;
  iface->set_background = bdk_win32_window_set_background;
  iface->set_back_pixmap = bdk_win32_window_set_back_pixmap;
  iface->reparent = bdk_win32_window_reparent;
  iface->clear_rebunnyion = bdk_window_win32_clear_rebunnyion;
  iface->set_cursor = bdk_win32_window_set_cursor;
  iface->get_geometry = bdk_win32_window_get_geometry;
  iface->get_root_coords = bdk_win32_window_get_root_coords;
  iface->get_pointer = bdk_window_win32_get_pointer;
  iface->get_deskrelative_origin = bdk_win32_window_get_deskrelative_origin;
  iface->shape_combine_rebunnyion = bdk_win32_window_shape_combine_rebunnyion;
  iface->input_shape_combine_rebunnyion = bdk_win32_input_shape_combine_rebunnyion;
  iface->set_static_gravities = bdk_win32_window_set_static_gravities;
  iface->queue_antiexpose = _bdk_win32_window_queue_antiexpose;
  iface->queue_translation = _bdk_win32_window_queue_translation;
  iface->destroy = _bdk_win32_window_destroy;
  iface->input_window_destroy = _bdk_input_window_destroy;
  iface->input_window_crossing = _bdk_input_crossing_event;
  /* CHECK: we may not need set_pixmap anymore if setting FALSE */
  iface->supports_native_bg = TRUE;
}

gboolean
bdk_win32_window_is_win32 (BdkWindow *window)
{
  return BDK_WINDOW_IS_WIN32 (window);
}

HWND
bdk_win32_window_get_impl_hwnd (BdkWindow *window)
{
  if (BDK_WINDOW_IS_WIN32 (window))
    return BDK_WINDOW_HWND (window);
  return NULL;
}


BdkDrawable *
bdk_win32_begin_direct_draw_libbtk_only (BdkDrawable *drawable,
					 BdkGC *gc,
					 gpointer *priv_data,
					 gint *x_offset_out,
					 gint *y_offset_out)
{
  BdkDrawable *impl;

  impl = _bdk_drawable_begin_direct_draw (drawable,
					  gc,
					  priv_data,
					  x_offset_out,
					  y_offset_out);

  return impl;
}

void
bdk_win32_end_direct_draw_libbtk_only (gpointer priv_data)
{
  _bdk_drawable_end_direct_draw (priv_data);
}
